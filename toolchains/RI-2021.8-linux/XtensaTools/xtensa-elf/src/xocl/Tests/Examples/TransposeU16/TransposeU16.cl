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
  ushort32 seq = {
    0, 1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31};
  ushort32 seq1 = seq & (ushort32)3;
  seq1 =   seq1 <<1;
  ushort32 seq2 = seq >> 2;
  seq2 = seq2 * (ushort32)(2*local_src_pitch);
  ushort32 offsets =   seq2 + seq1;
  offsets = offsets >> 1;
  const ushort32 sel0 = {0,  2,  4,  6,  8,  10,  12,  14,  16,  18,  20,  22,  24,  26,  28,  30,  32,  34,  36,  38,  40,  42,  44,  46,  48,  50,  52,  54,  56,  58,  60,  62}; 
  const ushort32 sel2 = {0,  1,  4,  5,  8,  9,  12,  13,  16,  17,  20,  21,  24,  25,  28,  29,  32,  33,  36,  37,  40,  41,  44,  45,  48,  49,  52,  53,  56,  57,  60,  61};
  ushort32 sel1 = sel0 + (ushort32)1;
  ushort32 sel3 = sel2 + (ushort32)2;
  
  // kernel using standard OpenCL C
  for (idx = 0; idx < outTileW; idx += 32) {
    __local ushort* psrc0 = local_src + (idx + 0) * local_src_pitch;
    __local ushort* psrc1 = local_src + (idx + 8) * local_src_pitch;
    __local ushort* psrc2 = local_src + (idx + 16) * local_src_pitch;
    __local ushort* psrc3 = local_src + (idx + 24) * local_src_pitch;
      
    for (int y = 0; y < inTileW; y+=4) {
    
      ushort32 x0 = xt_gather32(psrc0, (ushort *)&offsets);
      ushort32 x1 = xt_gather32(psrc1, (ushort *)&offsets);
      ushort32 x2 = xt_gather32(psrc2, (ushort *)&offsets);
      ushort32 x3 = xt_gather32(psrc3, (ushort *)&offsets);
      
      ushort32 x00 = shuffle2( x0,  x1, sel2);
      ushort32 x01 = shuffle2( x0,  x1, sel3);
      
      ushort32 x02 = shuffle2( x2,  x3, sel2);
      ushort32 x03 = shuffle2( x2,  x3, sel3);
      
      ushort32 xd0 = shuffle2(x00, x02, sel0);
      ushort32 xd2 = shuffle2(x00, x02, sel1);
      
      ushort32 xd1 = shuffle2(x01, x03, sel0); 
      ushort32 xd3 = shuffle2(x01, x03, sel1);
        
      __local ushort* pdst0 = local_dst + (y) * local_dst_pitch + idx;
      __local ushort* pdst1 = pdst0 + local_dst_pitch;
      __local ushort* pdst2 = pdst1 + local_dst_pitch;
      __local ushort* pdst3 = pdst2 + local_dst_pitch;
      
      vstore32(xd0, 0, pdst0);
      vstore32(xd2, 0, pdst1);
      vstore32(xd1, 0, pdst2); 
      vstore32(xd3, 0, pdst3);
      
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
