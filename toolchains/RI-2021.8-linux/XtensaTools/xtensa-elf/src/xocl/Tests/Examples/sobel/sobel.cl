#define TILE_W 128
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
  const ushort32 maskshift1 = {
    1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32};

  ushort32 maskshift2 = {
    2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17,
    18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33};

  const ushort32 maskshift3 = {
    3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18,
    19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34};

  const ushort32 maskshift4 = {
    4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35};

  short32 subX0, subX1, subX2, subX3, subX4;
  short32 subY0, subY1, subY2, subY3, subY4;
  short32 resX, resX0, resX1, resX2, resX3, resX4;
  short32 resY;

  for (idx = 0; idx < row_size; idx += 32) {
    int y = 0;
    __local uchar *pSrc0 = local_src + y * local_src_pitch + idx;
    short32 x00 = convert_short32(vload32(0, pSrc0));
    short32 xtail = convert_short32(vload32(0, pSrc0 + 32));
    short32 x01 = shuffle2(x00, xtail, maskshift1); 
    short32 x02 = shuffle2(x00, xtail, maskshift2);
    short32 x03 = shuffle2(x00, xtail, maskshift3);
    short32 x04 = shuffle2(x00, xtail, maskshift4);

    __local uchar *pSrc1 = local_src + (y + 1) * local_src_pitch + idx;
    short32 x10 = convert_short32(vload32(0, pSrc1));
    xtail = convert_short32(vload32(0, pSrc1 + 32));
    short32 x11 = shuffle2(x10, xtail, maskshift1);
    short32 x12 = shuffle2(x10, xtail, maskshift2);
    short32 x13 = shuffle2(x10, xtail, maskshift3);
    short32 x14 = shuffle2(x10, xtail, maskshift4);

    __local uchar *pSrc2 = local_src + (y + 2) * local_src_pitch + idx;
    short32 x20 = convert_short32(vload32(0, pSrc2));
    xtail = convert_short32(vload32(0, pSrc2 + 32));
    short32 x21 = shuffle2(x20, xtail, maskshift1);
    short32 x22 = shuffle2(x20, xtail, maskshift2);
    short32 x23 = shuffle2(x20, xtail, maskshift3);
    short32 x24 = shuffle2(x20, xtail, maskshift4);

    __local uchar *pSrc3 = local_src + (y + 3) * local_src_pitch + idx;
    short32 x30 = convert_short32(vload32(0, pSrc3));
    xtail = convert_short32(vload32(0, pSrc3 + 32));
    short32 x31 = shuffle2(x30, xtail, maskshift1);
    short32 x32 = shuffle2(x30, xtail, maskshift2);
    short32 x33 = shuffle2(x30, xtail, maskshift3);
    short32 x34 = shuffle2(x30, xtail, maskshift4);

    short32 x0Sum = x00 + (short32)4 * x01 + (short32)6 * x02 + 
                    (short32)4 * x03 + x04;
    short32 x1Sum = x10 + (short32)4 * x11 + (short32)6 * x12 + 
                    (short32)4 * x13 + x14;
    short32 x2Sum = x20 + (short32)4 * x21 + (short32)6 * x22 + 
                    (short32)4 * x23 + x24;
    short32 x3Sum = x30 + (short32)4 * x31 + (short32)6 * x32 + 
                    (short32)4 * x33 + x34;
    
    short32 x0SumX0 = - x00 - x01 - x01 + x04 + x03 + x03;
    short32 x1SumX0 = - x10 - x11 - x11 + x14 + x13 + x13;
    short32 x2SumX0 = - x20 - x21 - x21 + x24 + x23 + x23;
    short32 x3SumX0 = - x30 - x31 - x31 + x34 + x33 + x33;

    for (int y = 0; y < num_rows; y+=1) {
      __local uchar *pSrc4 = local_src + (y + 4) * local_src_pitch + idx;
      short32 x40 = convert_short32(vload32(0, pSrc4));
      xtail = convert_short32(vload32(0, pSrc4 + 32));
      short32 x41 = shuffle2(x40, xtail, maskshift1);
      short32 x42 = shuffle2(x40, xtail, maskshift2);
      short32 x43 = shuffle2(x40, xtail, maskshift3);
      short32 x44 = shuffle2(x40, xtail, maskshift4);

      short32 x4Sum = x40 + (x41<<2) + (short32)6 * x42 +  (x43<<2) + x44;

      short32 x4SumX0 = - x40 - x41 - x41  + x44 + x43 + x43;
    
      short32 resX = (short32)(x0SumX0 + (x1SumX0<<2) +
                      (short32)6 * x2SumX0 + ( x3SumX0<<2) + x4SumX0);

      short32 resY = (short32)(x4Sum + x3Sum + x3Sum - x1Sum - x1Sum - x0Sum);

      *((__local short32 *)(local_dstx + y * outTileW + idx)) = resX;
      *((__local short32 *)(local_dsty + y * outTileW + idx)) = resY;
    
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
