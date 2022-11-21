#define TILE_W 128
#define TILE_H 64
#define W_EXT 2
#define H_EXT 2
#define IMAGE_PAD 2
#define DST_BIT_DEPTH 8

__kernel __attribute__((reqd_work_group_size(1, 1, 1)))
__attribute__((reqd_work_dim_size(2))) void
oclKernel(__global uchar *restrict Src, int SrcPitch,
          __global uchar *restrict Dst, const int DstPitch, int width,
          int height, int dstWidth, int dstHeight,
          __local uchar *restrict local_src, __local uchar *restrict local_dst,
          __global int *restrict err_codes,
          __constant ushort *restrict filtCoeff) {
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
    #include "Filter2D_ivp.c"
#elif XT_OCL_EXT
    #include "Filter2D_ext.cl"
#else
  const ushort64 maskshift1 = {
    1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
    33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
    49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64};

  ushort64 maskshift2 = maskshift1 + (ushort64)1;
  ushort64 maskshift3 = maskshift1 + (ushort64)2;
  ushort64 maskshift4 = maskshift1 + (ushort64)3;

  // kernel using standard OpenCL C
  for (idx = 0; idx < row_size; idx += 64) {
    int y = 0;
    __local uchar *pSrc0 = local_src + y * local_src_pitch + idx;
    ushort64 x00 = convert_ushort64(vload64(0, pSrc0));
    ushort64 xtail = convert_ushort64(vload64(0, pSrc0 + 64));
    ushort64 x01 = shuffle2(x00, xtail, maskshift1);
    ushort64 x02 = shuffle2(x00, xtail, maskshift2);
    ushort64 x03 = shuffle2(x00, xtail, maskshift3);
    ushort64 x04 = shuffle2(x00, xtail, maskshift4);

    __local uchar *pSrc1 = local_src + (y + 1) * local_src_pitch + idx;

    ushort64 x10 = convert_ushort64(vload64(0, pSrc1));
    xtail = convert_ushort64(vload64(0, pSrc1 + 64));
    ushort64 x11 = shuffle2(x10, xtail, maskshift1);
    ushort64 x12 = shuffle2(x10, xtail, maskshift2);
    ushort64 x13 = shuffle2(x10, xtail, maskshift3);
    ushort64 x14 = shuffle2(x10, xtail, maskshift4);

    __local uchar *pSrc2 = local_src + (y + 2) * local_src_pitch + idx;
    ushort64 x20 = convert_ushort64(vload64(0, pSrc2));
    xtail = convert_ushort64(vload64(0, pSrc2 + 64));
    ushort64 x21 = shuffle2(x20, xtail, maskshift1);
    ushort64 x22 = shuffle2(x20, xtail, maskshift2);
    ushort64 x23 = shuffle2(x20, xtail, maskshift3);
    ushort64 x24 = shuffle2(x20, xtail, maskshift4);

    __local uchar *pSrc3 = local_src + (y + 3) * local_src_pitch + idx;
    ushort64 x30 = convert_ushort64(vload64(0, pSrc3));
    xtail = convert_ushort64(vload64(0, pSrc3 + 64));
    ushort64 x31 = shuffle2(x30, xtail, maskshift1);
    ushort64 x32 = shuffle2(x30, xtail, maskshift2);
    ushort64 x33 = shuffle2(x30, xtail, maskshift3);
    ushort64 x34 = shuffle2(x30, xtail, maskshift4);

    __local uchar *pSrc4 = local_src + (y + 4) * local_src_pitch + idx;

    for (int y = 0; y < num_rows; ++y) {
      ushort64 x40 = convert_ushort64(vload64(0, pSrc4));
      ushort64 xtail = convert_ushort64(vload64(0, pSrc4 + 64));
      pSrc4 += local_src_pitch;
      ushort64 x41 = shuffle2(x40, xtail, maskshift1);
      ushort64 x42 = shuffle2(x40, xtail, maskshift2);
      ushort64 x43 = shuffle2(x40, xtail, maskshift3);
      ushort64 x44 = shuffle2(x40, xtail, maskshift4);

      ushort64 row0 =
          x00 * (ushort64)filtCoeff[0] + x01 * (ushort64)filtCoeff[1] +
          x02 * (ushort64)filtCoeff[2] + x03 * (ushort64)filtCoeff[3] +
          x04 * (ushort64)filtCoeff[4];
      ushort64 row1 =
          x10 * (ushort64)filtCoeff[5] + x11 * (ushort64)filtCoeff[6] +
          x12 * (ushort64)filtCoeff[7] + x13 * (ushort64)filtCoeff[8] +
          x14 * (ushort64)filtCoeff[9];
      ushort64 row2 =
          x20 * (ushort64)filtCoeff[10] + x21 * (ushort64)filtCoeff[11] +
          x22 * (ushort64)filtCoeff[12] + x23 * (ushort64)filtCoeff[13] +
          x24 * (ushort64)filtCoeff[14];
      ushort64 row3 =
          x30 * (ushort64)filtCoeff[15] + x31 * (ushort64)filtCoeff[16] +
          x32 * (ushort64)filtCoeff[17] + x33 * (ushort64)filtCoeff[18] +
          x34 * (ushort64)filtCoeff[19];
      ushort64 row4 =
          x40 * (ushort64)filtCoeff[20] + x41 * (ushort64)filtCoeff[21] +
          x42 * (ushort64)filtCoeff[22] + x43 * (ushort64)filtCoeff[23] +
          x44 * (ushort64)filtCoeff[24];
      ushort64 result = row0 + row1 + row2 + row3 + row4;
      result = (result) >> 8;

      vstore64(convert_uchar64(result), 0, local_dst + y * outTileW + idx);

      x00 = x10;
      x01 = x11;
      x02 = x12;
      x03 = x13;
      x04 = x14;
      x10 = x20;
      x11 = x21;
      x12 = x22;
      x13 = x23;
      x14 = x24;
      x20 = x30;
      x21 = x31;
      x22 = x32;
      x23 = x33;
      x24 = x34;
      x30 = x40;
      x31 = x41;
      x32 = x42;
      x33 = x43;
      x34 = x44;
    }
  }
#endif
  event_t evtD = xt_async_work_group_2d_copy(
      Dst + (i * DstPitch) + j, local_dst, outTileW * dstBytes, num_rows,
      DstPitch * dstBytes, local_dst_pitch * dstBytes, 0);

  wait_group_events(1, &evtD);
}
