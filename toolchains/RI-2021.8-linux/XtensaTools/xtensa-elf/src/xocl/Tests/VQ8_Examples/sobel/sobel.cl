#define TILE_W 256
#define TILE_H 64
#define IMAGE_PAD 2
#define DST_BIT_DEPTH 16

// Sobel 5x5 filters
//
//        |-1  -2   0   2   1|        |-1  -4  -6  -4  -1|
//        |-4  -8   0   8   4|        |-2  -8 -12  -8  -2|
//   Gx = |-6 -12   0  12   6|   Gy = | 0   0   0   0   0|
//        |-4  -8   0   8   4|        | 2   8  12   8   2|
//        |-1  -2   0   2   1|        | 1   4   6   4   1|

__kernel __attribute__((reqd_work_group_size(1, 1, 1)))
__attribute__((reqd_work_dim_size(2))) void
Sobel5x5StdOcl(__global uchar *restrict Src, int SrcPitch, 
               __global short *restrict DstX,
               const int DstXPitch, __global short *restrict DstY, 
               const int DstYPitch,
               int width, int height, int dstWidth, int dstHeight,
               __local uchar *restrict local_src, 
               __local short *restrict local_dstx,
               __local short *restrict local_dsty,
                __global int *restrict err_codes) {
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
  inTilePitch = inTileW + 2 * IMAGE_PAD;
  local_src_pitch = row_size + pad + pad;

  outTileW = TILE_W;
  outTileH = TILE_H;
  local_dst_pitch = outTileW ;
  dstBytes = DST_BIT_DEPTH >> 3;

  i = get_global_id(0) * num_rows;
  j = get_global_id(1) * row_size;

  event_t evtA = xt_async_work_group_2d_copy(
      local_src, Src + (i * SrcPitch) + j, local_src_pitch,
      (num_rows + pad + pad), local_src_pitch, SrcPitch, 0);

  wait_group_events(1, &evtA);

#if NATIVE_KERNEL
  #include "sobel_ivp.c"
#elif XT_OCL_EXT
  #include "sobel_ext.cl"
#else
  // sobel kernel using standard OpenCL C
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
  const ushort64 maskshift3 = {
    3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18,
    19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34,
    35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
    51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66};
  const ushort64 maskshift4 = {
    4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
    36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67};

  short64 subX0, subX1, subX2, subX3, subX4;
  short64 subY0, subY1, subY2, subY3, subY4;
  short64 resX, resX0, resX1, resX2, resX3, resX4;
  short64 resY;

  for (idx = 0; idx < row_size; idx += 64) {
    int y = 0;
    __local uchar *pSrc0 = local_src + y * local_src_pitch + idx;
    short64 x00 = convert_short64(vload64(0, pSrc0));
    short64 xtail = convert_short64(vload64(0, pSrc0 + 64));
    short64 x01 = shuffle2(x00, xtail, maskshift1); 
    short64 x02 = shuffle2(x00, xtail, maskshift2);
    short64 x03 = shuffle2(x00, xtail, maskshift3);
    short64 x04 = shuffle2(x00, xtail, maskshift4);

    __local uchar *pSrc1 = local_src + (y + 1) * local_src_pitch + idx;
    short64 x10 = convert_short64(vload64(0, pSrc1));
    xtail = convert_short64(vload64(0, pSrc1 + 64));
    short64 x11 = shuffle2(x10, xtail, maskshift1);
    short64 x12 = shuffle2(x10, xtail, maskshift2);
    short64 x13 = shuffle2(x10, xtail, maskshift3);
    short64 x14 = shuffle2(x10, xtail, maskshift4);

    __local uchar *pSrc2 = local_src + (y + 2) * local_src_pitch + idx;
    short64 x20 = convert_short64(vload64(0, pSrc2));
    xtail = convert_short64(vload64(0, pSrc2 + 64));
    short64 x21 = shuffle2(x20, xtail, maskshift1);
    short64 x22 = shuffle2(x20, xtail, maskshift2);
    short64 x23 = shuffle2(x20, xtail, maskshift3);
    short64 x24 = shuffle2(x20, xtail, maskshift4);

    __local uchar *pSrc3 = local_src + (y + 3) * local_src_pitch + idx;
    short64 x30 = convert_short64(vload64(0, pSrc3));
    xtail = convert_short64(vload64(0, pSrc3 + 64));
    short64 x31 = shuffle2(x30, xtail, maskshift1);
    short64 x32 = shuffle2(x30, xtail, maskshift2);
    short64 x33 = shuffle2(x30, xtail, maskshift3);
    short64 x34 = shuffle2(x30, xtail, maskshift4);

    short64 x0Sum = x00 + (short64)4 * x01 + (short64)6 * x02 + 
                    (short64)4 * x03 + x04;
    short64 x1Sum = x10 + (short64)4 * x11 + (short64)6 * x12 + 
                    (short64)4 * x13 + x14;
    short64 x2Sum = x20 + (short64)4 * x21 + (short64)6 * x22 + 
                    (short64)4 * x23 + x24;
    short64 x3Sum = x30 + (short64)4 * x31 + (short64)6 * x32 + 
                    (short64)4 * x33 + x34;
    
    short64 x0SumX0 = - x00 - x01 - x01 + x04 + x03 + x03;
    short64 x1SumX0 = - x10 - x11 - x11 + x14 + x13 + x13;
    short64 x2SumX0 = - x20 - x21 - x21 + x24 + x23 + x23;
    short64 x3SumX0 = - x30 - x31 - x31 + x34 + x33 + x33;

    for (int y = 0; y < num_rows; y+=1) {
      __local uchar *pSrc4 = local_src + (y + 4) * local_src_pitch + idx;
      short64 x40 = convert_short64(vload64(0, pSrc4));
      xtail = convert_short64(vload64(0, pSrc4 + 64));
      short64 x41 = shuffle2(x40, xtail, maskshift1);
      short64 x42 = shuffle2(x40, xtail, maskshift2);
      short64 x43 = shuffle2(x40, xtail, maskshift3);
      short64 x44 = shuffle2(x40, xtail, maskshift4);

      short64 x4Sum = x40 + (x41<<2) + (short64)6 * x42 +  (x43<<2) + x44;

      short64 x4SumX0 = - x40 - x41 - x41  + x44 + x43 + x43;
    
      short64 resX = x0SumX0 + (x1SumX0<<2) + (short64)6 * x2SumX0 + 
                     (x3SumX0<<2) + x4SumX0;

      short64 resY = (short64)(x4Sum + x3Sum + x3Sum - x1Sum - x1Sum - x0Sum);

      *((__local short64 *)(local_dstx + y * outTileW + idx)) = resX;
      *((__local short64 *)(local_dsty + y * outTileW + idx)) = resY;
    
      x0Sum = x1Sum;
      x1Sum = x2Sum;
      x2Sum = x3Sum;
      x3Sum = x4Sum;
    
      x0SumX0 = x1SumX0;
      x1SumX0 = x2SumX0;
      x2SumX0 = x3SumX0;
      x3SumX0 = x4SumX0;
    }
  }

#endif
  event_t evtDx = xt_async_work_group_2d_copy(
      DstX + (i * DstXPitch) + j, local_dstx, outTileW * dstBytes, num_rows,
      DstXPitch * dstBytes, outTileW * dstBytes, 0);

  event_t evtDy = xt_async_work_group_2d_copy(
      DstY + (i * DstYPitch) + j, local_dsty, outTileW * dstBytes, num_rows,
      DstYPitch * dstBytes, outTileW * dstBytes, 0);

  wait_group_events(1, &evtDx);
  wait_group_events(1, &evtDy);
}
