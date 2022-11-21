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

  outTileW = TILE_W>>1;
  outTileH = TILE_H>>1;
  local_dst_pitch = outTileW; // + 2*W_EXT;
  dstBytes = DST_BIT_DEPTH >> 3;

  i = get_global_id(0) * num_rows;
  j = get_global_id(1) * row_size;  
  int i_out = get_global_id(0) * outTileH;
  int j_out = get_global_id(1) * outTileW;

  event_t evtA = xt_async_work_group_2d_copy(
      local_src, Src + (i * SrcPitch) + j, local_src_pitch,
      (num_rows + pad + pad), local_src_pitch, SrcPitch, 0);
  wait_group_events(1, &evtA);

#if NATIVE_KERNEL
  #include "PyramidU8_ivp.c"
#elif XT_OCL_EXT
  #include "PyramidU8_ext.cl"
#else
  // kernel using standard OpenCL C
  for (idx = 0; idx < outTileW; idx += 32) {
    int y = 0;
    __local uchar *pSrc0 = local_src + y * local_src_pitch + idx * 2;
    ushort32 x00 = (vload32(0, (__local ushort *)pSrc0));
    ushort32 x01 = x00 >> 8;
    ushort32 x02 = (vload32(0, (__local ushort *)(pSrc0 + 2)));
    ushort32 x03 = x02 >> 8;
    ushort32 x04 = (vload32(0, (__local ushort *)(pSrc0 + 4)));
    x00 = x00 & (ushort32)0xFF;
    x02 = x02 & (ushort32)0xFF;
    x04 = x04 & (ushort32)0xFF;

    __local uchar *pSrc1 = local_src + (y + 1) * local_src_pitch + idx * 2;
    ushort32 x10 = (vload32(0, (__local ushort *)pSrc1));
    ushort32 x11 = x10 >> 8;
    ushort32 x12 = (vload32(0, (__local ushort *)(pSrc1 + 2)));
    ushort32 x13 = x12 >> 8;
    ushort32 x14 = (vload32(0, (__local ushort *)(pSrc1 + 4)));
    x10 = x10 & (ushort32)0xFF;
    x12 = x12 & (ushort32)0xFF;
    x14 = x14 & (ushort32)0xFF;

    __local uchar *pSrc2 = local_src + (y + 2) * local_src_pitch + idx * 2;
    ushort32 x20 = (vload32(0, (__local ushort *)pSrc2));
    ushort32 x21 = x20 >> 8;
    ushort32 x22 = (vload32(0, (__local ushort *)(pSrc2 + 2)));
    ushort32 x23 = x22 >> 8;
    ushort32 x24 = (vload32(0, (__local ushort *)(pSrc2 + 4)));
    x20 = x20 & (ushort32)0xFF;
    x22 = x22 & (ushort32)0xFF;
    x24 = x24 & (ushort32)0xFF;
 
    ushort32 row0 = 
        x00 + x01 * (ushort32)4 + x02 * (ushort32)6 + x03 * (ushort32)4 + x04;
    ushort32 row1 =
        x10 + x11 * (ushort32)4 + x12 * (ushort32)6 + x13 * (ushort32)4 + x14;
    ushort32 row2 =
        x20 + x21 * (ushort32)4 + x22 * (ushort32)6 + x23 * (ushort32)4 + x24;
   
    for (int y = 0; y < inTileH; y+=2) {
      __local uchar *pSrc3 = local_src + (y + 3) * local_src_pitch + idx * 2;
      ushort32 x30 = (vload32(0, (__local ushort *)pSrc3));
      ushort32 x31 = x30 >> 8;
      ushort32 x32 = (vload32(0, (__local ushort *)(pSrc3 + 2)));
      ushort32 x33 = x32 >> 8;
      ushort32 x34 = (vload32(0, (__local ushort *)(pSrc3 + 4)));
      x30 = x30 & (ushort32)0xFF;
      x32 = x32 & (ushort32)0xFF;
      x34 = x34 & (ushort32)0xFF;
    
      ushort32 row3 =
        x30 + x31 * (ushort32)4 + x32 * (ushort32)6 + x33 * (ushort32)4 + x34;
  
      __local uchar *pSrc4 = local_src + (y + 4) * local_src_pitch + idx * 2;
      ushort32 x40 = (vload32(0, (__local ushort *)pSrc4));
      ushort32 x41 = x40 >> 8;
      ushort32 x42 = (vload32(0, (__local ushort *)(pSrc4 + 2)));
      ushort32 x43 = x42 >> 8;
      ushort32 x44 = (vload32(0, (__local ushort *)(pSrc4 + 4)));
      x40 = x40 & (ushort32)0xFF;
      x42 = x42 & (ushort32)0xFF;
      x44 = x44 & (ushort32)0xFF;     

      ushort32 row4 =
          x40 + x41 * (ushort32)4 + x42 * (ushort32)6 + x43 * (ushort32)4 + x44;

      ushort32 result = row0 + row1 * (ushort32)4 + row2 * (ushort32)6 +
                        row3 * (ushort32)4 + row4;
            
      result = (result + (ushort32)128) >> 8;
    
      __local uchar *dst = local_dst + (y>>1) * outTileW + idx;
      vstore32(convert_uchar32(result), 0, dst);
    
      row0 = row2;
      row1 = row3;
      row2 = row4;
    }
  }
#endif

  event_t evtD = xt_async_work_group_2d_copy(
      Dst + (i_out * DstPitch) + j_out , local_dst, outTileW * dstBytes, 
      outTileH, DstPitch * dstBytes, local_dst_pitch * dstBytes, 0);
  wait_group_events(1, &evtD);
}
