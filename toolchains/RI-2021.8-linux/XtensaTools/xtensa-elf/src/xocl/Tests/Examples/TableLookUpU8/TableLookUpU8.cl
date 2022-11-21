#define TILE_W 128
#define TILE_H 64
#define W_EXT 0
#define H_EXT 0
#define IMAGE_PAD 0
#define DST_BIT_DEPTH 8

__kernel __attribute__((reqd_work_group_size(1, 1, 1)))
__attribute__((reqd_work_dim_size(2))) void
oclKernel(__global uchar *restrict Src, int SrcPitch,
          __global uchar *restrict Dst, const int DstPitch, int width,
          int height, int dstWidth, int dstHeight,
          __constant uchar *restrict lut,
          __local uchar *restrict local_src, 
          __local uchar *restrict local_dst,
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
  #include "TableLookUpU8_ivp.c"
#else
  uchar64 lut0 = vload64(0,lut);
  uchar64 lut1 = vload64(0,lut+64);
  uchar64 lut2 = vload64(0,lut+128);
  uchar64 lut3 = vload64(0,lut+192);

  // kernel using standard OpenCL C
  for (idx = 0; idx < row_size; idx += 64*2) {   
    for (int y = 0; y < num_rows; ++y) {
      __local uchar *pSrc0 = local_src + y * local_src_pitch + idx;
      uchar64 xsrc0 = vload64(0, pSrc0);      
      uchar64 xdst0a = shuffle2(lut0, lut1, xsrc0);
      uchar64 xdst0b = shuffle2(lut2, lut3, xsrc0);
      char64 xsrc0s = *((char64*)&xsrc0);    
      xsrc0s = xsrc0s >> 7;
      xsrc0 = *((uchar64*)&xsrc0s);
      xdst0b = xdst0b & xsrc0;
      xdst0a = xdst0a & (~xsrc0);
      uchar64 xdst0 = xdst0a | xdst0b;
      vstore64(xdst0, 0, local_dst + y * local_dst_pitch + idx);
      
      uchar64 xsrc1 = vload64(0, pSrc0 + 64);
      uchar64 xdst1a = shuffle2(lut0, lut1, xsrc1);
      uchar64 xdst1b = shuffle2(lut2, lut3, xsrc1);
      char64 xsrc1s = *((char64*)&xsrc1);
      xsrc1s = xsrc1s >> 7;
      xsrc1 = *((uchar64*)&xsrc1s);
      xdst1b = xdst1b & xsrc1;
      xdst1a = xdst1a & (~xsrc1);
      uchar64 xdst1 = xdst1a | xdst1b;
      vstore64(xdst1, 0, local_dst + y * local_dst_pitch + idx + 64);
    }
  }
#endif
  event_t evtD = xt_async_work_group_2d_copy(
      Dst + (i * DstPitch) + j, local_dst, outTileW * dstBytes, num_rows,
      DstPitch * dstBytes, local_dst_pitch * dstBytes, 0);

  wait_group_events(1, &evtD);
}
