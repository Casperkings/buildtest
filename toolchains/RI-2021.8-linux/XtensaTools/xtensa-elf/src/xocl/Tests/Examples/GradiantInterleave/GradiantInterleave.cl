#define TILE_W 128
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
  const ushort32 maskshift1 = {
    1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32};

  const ushort32 maskshift2 = {
    2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17,
    18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33};

  const ushort32 InterleaveA = {
    0, 32, 1, 33, 2, 34, 3, 35, 4, 36, 5, 37, 6, 38, 7, 39, 8, 40, 9, 41, 
    10, 42, 11, 43, 12, 44, 13, 45, 14, 46, 15, 47};

  const ushort32 InterleaveB = {
    16, 48, 17, 49, 18, 50, 19, 51, 20, 52, 21, 53, 22, 54, 23, 55, 24, 
    56, 25, 57, 26, 58, 27, 59, 28, 60, 29, 61, 30, 62, 31, 63};

  for (idx = 0; idx < row_size; idx += 32) {
    int y = 0;
    __local uchar *pSrc0 = local_src + y * local_src_pitch + idx;
    short32 x00 = convert_short32(vload32(0, pSrc0));
    short32 xtail = convert_short32(vload32(0, pSrc0 + 32));
    short32 x01 = shuffle2(x00, xtail, maskshift1); 
    short32 x02 = shuffle2(x00, xtail, maskshift2);

    __local uchar *pSrc1 = local_src + (y + 1) * local_src_pitch + idx;
    short32 x10 = convert_short32(vload32(0, pSrc1));
    xtail = convert_short32(vload32(0, pSrc1 + 32));
    short32 x11 = shuffle2(x10, xtail, maskshift1);
    short32 x12 = shuffle2(x10, xtail, maskshift2);

    for (int y = 0; y < num_rows; y+=1) {
      __local uchar *pSrc2 = local_src + (y + 2) * local_src_pitch + idx;
      short32 x20 = convert_short32(vload32(0, pSrc2));
      xtail = convert_short32(vload32(0, pSrc2 + 32));
      short32 x21 = shuffle2(x20, xtail, maskshift1);
      short32 x22 = shuffle2(x20, xtail, maskshift2);
      
      short32 xDx = x12 - x10;
      short32 xDy = x21 - x01;

      short32 xOutA = shuffle2(xDx, xDy, InterleaveA);
      short32 xOutB = shuffle2(xDx, xDy, InterleaveB);
      
      __local short* pdstA = local_dstxy + y * outTileW + idx*2;
      __local short* pdstB = local_dstxy + y * outTileW + idx*2 + 32;
      vstore32(xOutA, 0, pdstA);
      vstore32(xOutB, 0, pdstB);
        
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
