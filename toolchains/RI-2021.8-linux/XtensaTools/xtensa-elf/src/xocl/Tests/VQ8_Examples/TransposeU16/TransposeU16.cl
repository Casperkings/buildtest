#define TILE_W 128
#define TILE_H 64
#define W_EXT 0
#define H_EXT 0
#define IMAGE_PAD 0
#define DST_BIT_DEPTH 16

__kernel __attribute__((reqd_work_group_size(1, 1, 1)))
__attribute__((reqd_work_dim_size(2))) void
oclKernel(__global ushort *restrict Src, int SrcPitch,
          __global ushort *restrict Dst, const int DstPitch, int width,
          int height, int dstWidth, int dstHeight,
          __local ushort *restrict local_src, 
          __local ushort *restrict local_dst,
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

  outTileW = TILE_H;
  outTileH = TILE_W;
  local_dst_pitch = outTileW; // + 2*W_EXT;
  dstBytes = DST_BIT_DEPTH >> 3;

  i = get_global_id(0) * num_rows;
  j = get_global_id(1) * row_size;
  int once = 1;
  event_t evtA = xt_async_work_group_2d_copy(
      local_src, Src + (i * SrcPitch) + j, local_src_pitch * dstBytes,
      (num_rows + pad + pad), local_src_pitch * dstBytes , SrcPitch* dstBytes , 0);
  wait_group_events(1, &evtA);
#if NATIVE_KERNEL
  #include "TransposeU16_ivp.c"
#else
  const ushort64 seq = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
    19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36,
    37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54,
    55, 56, 57, 58, 59, 60, 61, 62, 63};
  ushort64 seq1 = seq & (ushort64)3;
  seq1 = seq1 << 1;
  ushort64 seq2 = seq >> 2;
  seq2 = seq2 * (ushort64)(2*local_src_pitch);
  ushort64 offsets = seq2 + seq1;
  offsets = offsets >> 1;
  const ushort64 sel0 = {0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 
                         28, 30, 32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52, 
                         54, 56, 58, 60, 62, 64, 66, 68, 70, 72, 74, 76, 78, 
                         80, 82, 84, 86, 88, 90, 92, 94, 96, 98, 100, 102,
                         104, 106, 108, 110, 112, 114, 116, 118, 120, 122, 
                         124, 126};
  const ushort64 sel2 = {0, 1, 4, 5, 8, 9, 12, 13, 16, 17, 20, 21, 24, 25, 28,
                         29, 32, 33, 36, 37, 40, 41, 44, 45, 48, 49, 52, 53, 
                         56, 57, 60, 61, 64, 65, 68, 69, 72, 73, 76, 77, 80, 81,
                         84, 85, 88, 89, 92, 93, 96, 97, 100, 101, 104, 105,
                         108, 109, 112, 113, 116, 117, 120, 121, 124, 125};

  ushort64 sel1 = sel0 + (ushort64)1;
  ushort64 sel3 = sel2 + (ushort64)2;
  
  // kernel using standard OpenCL C
  for (idx = 0; idx < outTileW; idx += 64) {
    __local ushort* psrc0 = local_src + (idx + 0) * local_src_pitch;
    __local ushort* psrc1 = local_src + (idx + 16) * local_src_pitch;
    __local ushort* psrc2 = local_src + (idx + 32) * local_src_pitch;
    __local ushort* psrc3 = local_src + (idx + 48) * local_src_pitch;
      
    for (int y = 0; y < inTileW; y+=4) {
    
      ushort64 x0 = xt_gather64(psrc0, (ushort *)&offsets);
      ushort64 x1 = xt_gather64(psrc1, (ushort *)&offsets);
      ushort64 x2 = xt_gather64(psrc2, (ushort *)&offsets);
      ushort64 x3 = xt_gather64(psrc3, (ushort *)&offsets);
      
      ushort64 x00 = shuffle2(x0,  x1, sel2);
      ushort64 x01 = shuffle2(x0,  x1, sel3);
      
      ushort64 x02 = shuffle2(x2,  x3, sel2);
      ushort64 x03 = shuffle2(x2,  x3, sel3);
      
      ushort64 xd0 = shuffle2(x00, x02, sel0);
      ushort64 xd2 = shuffle2(x00, x02, sel1);
      
      ushort64 xd1 = shuffle2(x01, x03, sel0); 
      ushort64 xd3 = shuffle2(x01, x03, sel1);
        
      __local ushort* pdst0 = local_dst + (y) * local_dst_pitch + idx;
      __local ushort* pdst1 = pdst0 + local_dst_pitch;
      __local ushort* pdst2 = pdst1 + local_dst_pitch;
      __local ushort* pdst3 = pdst2 + local_dst_pitch;
      
      vstore64(xd0, 0, pdst0);
      vstore64(xd2, 0, pdst1);
      vstore64(xd1, 0, pdst2); 
      vstore64(xd3, 0, pdst3);
      
      psrc0 += 4;
      psrc1 += 4;
      psrc2 += 4;
      psrc3 += 4;  
    }
  }
#endif
  event_t evtD = xt_async_work_group_2d_copy(
      Dst + (j * DstPitch) + i, local_dst, outTileW * dstBytes, outTileH,
      DstPitch * dstBytes, local_dst_pitch * dstBytes, 0);

  wait_group_events(1, &evtD);
}
