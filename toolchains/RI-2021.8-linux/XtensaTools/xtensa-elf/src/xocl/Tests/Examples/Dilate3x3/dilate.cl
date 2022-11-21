#define TILE_W 128
#define TILE_H 64
#define W_EXT 1
#define H_EXT 1
#define IMAGE_PAD 1
#define DST_BIT_DEPTH 8

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
  int once = 1;
  event_t evtA = xt_async_work_group_2d_copy(
      local_src, Src + (i * SrcPitch) + j, local_src_pitch,
      (num_rows + pad + pad), local_src_pitch, SrcPitch, 0);
  wait_group_events(1, &evtA);
#if NATIVE_KERNEL
  #include "dilate_ivp.c"
#else
  const uchar64 maskshift1 = {
    1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
    33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
    49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64};
  const uchar64 maskshift2 = {
    2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17,
    18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
    34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
    50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65};

  // kernel using standard OpenCL C
  for (idx = 0; idx < row_size; idx += 64) {
    int y = 0;
    __local uchar *pSrc0 = local_src + y * local_src_pitch + idx;
    uchar64 x00 = vload64(0, pSrc0);
    uchar64 xtail = vload64(0, pSrc0 + 64);
    uchar64 x01 = shuffle2(x00, xtail, maskshift1);
    uchar64 x02 = shuffle2(x00, xtail, maskshift2);
    uchar64 xR0 = max(max(x00, x01), x02);
  
    __local uchar *pSrc1 = local_src + (y + 1) * local_src_pitch + idx;
    uchar64 x10 = vload64(0, pSrc1);
    xtail = vload64(0, pSrc1 + 64);
    uchar64 x11 = shuffle2(x10, xtail, maskshift1);
    uchar64 x12 = shuffle2(x10, xtail, maskshift2);
    uchar64 xR1 = max(max(x10, x11), x12);
      
    for (int y = 0; y < num_rows; ++y) {
  
      __local uchar *pSrc2 = local_src + (y + 2) * local_src_pitch + idx;
      uchar64 x20 = vload64(0, pSrc2);
      xtail = vload64(0, pSrc2 + 64);
      uchar64 x21 = shuffle2(x20, xtail, maskshift1);
      uchar64 x22 = shuffle2(x20, xtail, maskshift2);
    
      uchar64 xR2 = max(max(x20, x21), x22);
      uchar64 xOut = max(max(xR0, xR1), xR2);

      vstore64(xOut, 0, (__local uchar *)(local_dst + y * outTileW + idx));

      xR0 = xR1;
      xR1 = xR2;
    }
  }
#endif
  event_t evtD = xt_async_work_group_2d_copy(
      Dst + (i * DstPitch) + j, local_dst, outTileW * dstBytes, num_rows,
      DstPitch * dstBytes, local_dst_pitch * dstBytes, 0);

  wait_group_events(1, &evtD);
}
