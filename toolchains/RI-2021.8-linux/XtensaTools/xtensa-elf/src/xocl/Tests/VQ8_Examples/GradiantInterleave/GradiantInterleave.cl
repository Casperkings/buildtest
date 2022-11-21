#define TILE_W 256
#define TILE_H 64
#define IMAGE_PAD 1
#define DST_BIT_DEPTH 16

__kernel __attribute__((reqd_work_group_size(1, 1, 1)))
__attribute__((reqd_work_dim_size(2))) void
Oclkernel(__global uchar *restrict Src, int SrcPitch, 
          __global short *restrict DstXY,
          const int DstXYPitch,
          int width, int height, int dstWidth, int dstHeight,
          __local uchar *restrict local_src, 
          __local short *restrict local_dstxy,
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
  int outj;
  inTileW = TILE_W;
  inTileH = TILE_H;
  inTilePitch = inTileW + 2 * IMAGE_PAD;
  local_src_pitch = row_size + pad + pad;

  outTileW = TILE_W*2;
  outTileH = TILE_H;
  local_dst_pitch = outTileW ;
  dstBytes = DST_BIT_DEPTH >> 3;

  i = get_global_id(0) * num_rows;
  j = get_global_id(1) * row_size;
  outj = get_global_id(1) * outTileW;

  event_t evtA = xt_async_work_group_2d_copy(
      local_src, Src + (i * SrcPitch) + j, local_src_pitch,
      (num_rows + pad + pad), local_src_pitch, SrcPitch, 0);

  wait_group_events(1, &evtA);

#if NATIVE_KERNEL
  #include "GradiantInterleave_ivp.c"
#else
  // kernel using standard OpenCL C
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

  const ushort64 InterleaveA = {
    0, 64, 1, 65, 2, 66, 3, 67, 4, 68, 5, 69, 6, 70, 7, 71, 8,
    72, 9, 73, 10, 74, 11, 75, 12, 76, 13, 77, 14, 78, 15, 79, 16, 80, 17, 81,
    18, 82, 19, 83, 20, 84, 21, 85, 22, 86, 23, 87, 24, 88, 25, 89, 26, 90,
    27, 91, 28, 92, 29, 93, 30, 94, 31, 95};

  const ushort64 InterleaveB = {
    32, 96, 33, 97, 34, 98, 35, 99, 36, 100, 37, 101, 38, 102,
    39, 103, 40, 104, 41, 105, 42, 106, 43, 107, 44, 108, 45, 109, 46, 110,
    47, 111, 48, 112, 49, 113, 50, 114, 51, 115, 52, 116, 53, 117, 54, 118,
    55, 119, 56, 120, 57, 121, 58, 122, 59, 123, 60, 124, 61, 125, 62, 126,
    63, 127};

  for (idx = 0; idx < row_size; idx += 64) {
    int y = 0;
    __local uchar *pSrc0 = local_src + y * local_src_pitch + idx;
    short64 x00 = convert_short64(vload64(0, pSrc0));
    short64 xtail = convert_short64(vload64(0, pSrc0 + 64));
    short64 x01 = shuffle2(x00, xtail, maskshift1); 
    short64 x02 = shuffle2(x00, xtail, maskshift2);

    __local uchar *pSrc1 = local_src + (y + 1) * local_src_pitch + idx;
    short64 x10 = convert_short64(vload64(0, pSrc1));
    xtail = convert_short64(vload64(0, pSrc1 + 64));
    short64 x11 = shuffle2(x10, xtail, maskshift1);
    short64 x12 = shuffle2(x10, xtail, maskshift2);

    for (int y = 0; y < num_rows; y+=1) {
      __local uchar *pSrc2 = local_src + (y + 2) * local_src_pitch + idx;
      short64 x20 = convert_short64(vload64(0, pSrc2));
      xtail = convert_short64(vload64(0, pSrc2 + 64));
      short64 x21 = shuffle2(x20, xtail, maskshift1);
      short64 x22 = shuffle2(x20, xtail, maskshift2);
      
      short64 xDx = x12 - x10;
      short64 xDy = x21 - x01;

      short64 xOutA = shuffle2(xDx, xDy, InterleaveA);
      short64 xOutB = shuffle2(xDx, xDy, InterleaveB);
      
      __local short* pdstA = local_dstxy + y * outTileW + idx*2;
      __local short* pdstB = local_dstxy + y * outTileW + idx*2 + 64;
      vstore64(xOutA, 0, pdstA);
      vstore64(xOutB, 0, pdstB);
        
      //x00 = x10;   
      x01 = x11;
      //x02 = x12;
      
      x10 = x20; 
      x11 = x21;
      x12 = x22;
    }
  }

#endif
  event_t evtDxDy = xt_async_work_group_2d_copy(
      DstXY + (i * DstXYPitch) + outj, local_dstxy, outTileW * dstBytes, 
      num_rows, DstXYPitch * dstBytes, outTileW * dstBytes, 0);
  wait_group_events(1, &evtDxDy);
}
