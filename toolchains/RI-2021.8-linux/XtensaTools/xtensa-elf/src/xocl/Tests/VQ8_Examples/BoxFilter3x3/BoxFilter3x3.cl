#define TILE_W 256
#define TILE_H 64
#define W_EXT 1
#define H_EXT 1
#define IMAGE_PAD 1
#define DST_BIT_DEPTH 8

#define Q15_1div9      3641

__kernel __attribute__((reqd_work_group_size(1, 1, 1)))
__attribute__((reqd_work_dim_size(2))) void
oclKernel(__global uchar *restrict Src, int SrcPitch,
          __global uchar *restrict Dst, const int DstPitch, int width,
          int height, int dstWidth, int dstHeight,
          __local uchar *restrict local_src, __local uchar *restrict local_dst,
          __global int *err_codes) {
  int i, j;
  int idx, idy;
  int num_rows = TILE_H;
  int row_size = TILE_W;
  int local_src_pitch, local_dst_pitch;
  int pad = IMAGE_PAD;
  int inTilePitch;
  int inTileW, inTileH, outTileW, outTileH;
  int dstBytes;
  int err = 0;

  inTileW = TILE_W;
  inTileH = TILE_H;
  inTilePitch = inTileW + 2 * W_EXT;
  local_src_pitch = row_size + pad + pad;

  outTileW = TILE_W;
  outTileH = TILE_H;
  local_dst_pitch = outTileW; // + 2*W_EXT;
  dstBytes = DST_BIT_DEPTH >> 3;

  i = get_global_id(0) * num_rows;
  j = get_global_id(1) * row_size;
  event_t evtA = xt_async_work_group_2d_copy(
      local_src, Src + (i * SrcPitch) + j, local_src_pitch,
      (num_rows + pad + pad), local_src_pitch, SrcPitch, 0);
  wait_group_events(1, &evtA);
#if NATIVE_KERNEL
  #include "BoxFilter3x3_ivp.c"
#elif XT_OCL_EXT
  #include "BoxFilter3x3_ext.cl"
#else
  const ushort64 maskshift1 = {
    1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
    33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
    49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64};
  ushort64 maskshift2 = {
    2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17,
    18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
    34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
    50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65};

  // kernel using standard OpenCL C
  for (idx = 0; idx < row_size; idx += 64) {
    int y = 0;
    __local uchar *pSrc0 = local_src + y * local_src_pitch + idx;
    ushort64 x00 = convert_ushort64(vload64(0, pSrc0));
    ushort64 xtail = convert_ushort64(vload64(0, pSrc0 + 64));
    ushort64 x01 = shuffle2(x00, xtail, maskshift1);
    ushort64 x02 = shuffle2(x00, xtail, maskshift2);

    __local uchar *pSrc1 = local_src + (y + 1) * local_src_pitch + idx;

    ushort64 x10 = convert_ushort64(vload64(0, pSrc1));
    xtail = convert_ushort64(vload64(0, pSrc1 + 64));
    ushort64 x11 = shuffle2(x10, xtail, maskshift1);
    ushort64 x12 = shuffle2(x10, xtail, maskshift2);

    __local uchar *pSrc3 = local_src + (y + 3) * local_src_pitch + idx;
    ushort64 x30 = convert_ushort64(vload64(0, pSrc3));
    xtail = convert_ushort64(vload64(0, pSrc3 + 64));
    ushort64 x31 = shuffle2(x30, xtail, maskshift1);
    ushort64 x32 = shuffle2(x30, xtail, maskshift2);

    ushort64 row0 = x00 + x01 + x02;
    ushort64 row1 = x10 + x11 + x12;
  
    ushort64 Sum = row0 + row1;

    for (int y = 0; y < num_rows; ++y) {
      __local uchar *pSrc2 = local_src + (y + 2) * local_src_pitch + idx;
      ushort64 x20 = convert_ushort64(vload64(0, pSrc2));
      xtail = convert_ushort64(vload64(0, pSrc2 + 64));
      ushort64 x21 = shuffle2(x20, xtail, maskshift1);
      ushort64 x22 = shuffle2(x20, xtail, maskshift2);

      ushort64 row2 = x20 + x21 + x22;
    
      Sum += row2;
    
      uint64 result = xt_mul32(Sum, (ushort64)Q15_1div9);
      result += (uint64) (1<<14);
      result = result >> 15;
      uchar64 xout = convert_uchar64(convert_ushort64(result));
    
      vstore64(xout, 0, (__local uchar *)(local_dst + y * outTileW + idx));
    
      Sum -= row0;
      row0 = row1;
      row1 = row2;
    }
  }
#endif
  event_t evtD = xt_async_work_group_2d_copy(
      Dst + (i * DstPitch) + j, local_dst, outTileW * dstBytes, num_rows,
      DstPitch * dstBytes, local_dst_pitch * dstBytes, 0);

  wait_group_events(1, &evtD);
}
