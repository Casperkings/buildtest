#define TILE_W 128
#define TILE_H 64
#define W_EXT 0
#define H_EXT 0
#define IMAGE_PAD 0
#define DST_BIT_DEPTH 32

void vstore64(uint64 v, __local uint *p) {
  vstore16(v.lo.lo, 0, p);
  vstore16(v.lo.hi, 0, p+16);
  vstore16(v.hi.lo, 0, p+32);
  vstore16(v.hi.hi, 0, p+48);
}

__kernel __attribute__((reqd_work_group_size(1, 1, 1)))
__attribute__((reqd_work_dim_size(2))) void
oclKernel(__global uchar *restrict Src, int SrcPitch,
          __global uint *restrict Dst, const int DstPitch, int width,
          int height, int dstWidth, int dstHeight,
          __local uchar *restrict local_src, __local uint *restrict local_dst,
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
  int i_out = get_global_id(0) * outTileH;
  int j_out = get_global_id(1) * outTileW;
  int once = 1;
  event_t evtA = xt_async_work_group_2d_copy(
      local_src, Src + (i * SrcPitch) + j, local_src_pitch,
      (num_rows + pad + pad), local_src_pitch, SrcPitch, 0);
  wait_group_events(1, &evtA);
#if NATIVE_KERNEL
  #include "IntegralU8U32_ivp.c"
#elif XT_OCL_EXT
  #include "IntegralU8U32_ext.cl"
#else
  // kernel using standard OpenCL C

  const ushort32 RotateLeft1 = {31, 32,33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62};
  const ushort32 RotateLeft2 = {30, 31,32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61};
  const ushort32 RotateLeft3 = {29, 30,31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60};
  const ushort32 RotateLeft4 = {28, 29,30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59};
  const ushort32 RotateLeft8 = {24, 25,26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55};
  const ushort32 RotateLeft16 = RotateLeft8 - (ushort32) 8;
  
  {
  __local uchar *pSrc = local_src; 
  __local uint *pDst = local_dst; 
  uint32 Col_carry = 0;
  for(int y = 0; y < inTileH; y++)
  { 
    ushort32 x00 = convert_ushort32(vload32(0, pSrc));
    pSrc += local_src_pitch;
    ushort32 x01 = shuffle2((ushort32)0, x00, RotateLeft1);
    ushort32 x02 = shuffle2((ushort32)0, x00, RotateLeft2);
    ushort32 x03 = shuffle2((ushort32)0, x00, RotateLeft3);
    
    x03 = x00 + x01 + x02 + x03;
    
    ushort32 x04 = shuffle2((ushort32)0, x03, RotateLeft4) + x03;
    ushort32 x05 = shuffle2((ushort32)0, x04, RotateLeft8) + x04;
    ushort32 x06 = shuffle2((ushort32)0, x05, RotateLeft16) + x05;
    Col_carry += convert_uint32(x06);
    
    vstore32(Col_carry, 0, pDst);
    pDst += local_dst_pitch;
  }
  }

  for (idx = 32; idx < inTileW; idx += 32) {   
   __local uchar *pSrc = local_src + idx; 
   __local uint *pDst = local_dst + idx; 
    uint32 Col_carry = 0;
    for (int y = 0; y < inTileH; y+=1) {
      ushort32 x00 = convert_ushort32(vload32(0, pSrc));
      pSrc += local_src_pitch;
      ushort32 x01 = shuffle2((ushort32)0, x00, RotateLeft1);
      ushort32 x02 = shuffle2((ushort32)0, x00, RotateLeft2);
      ushort32 x03 = shuffle2((ushort32)0, x00, RotateLeft3);
      
      x03 = x00 + x01 + x02 + x03;
      
      ushort32 x04 = shuffle2((ushort32)0, x03, RotateLeft4) + x03;
      ushort32 x05 = shuffle2((ushort32)0, x04, RotateLeft8) + x04;
      ushort32 x06 = shuffle2((ushort32)0, x05, RotateLeft16) + x05;
      Col_carry += convert_uint32(x06);
      uint32 Out = Col_carry + (uint32)(pDst[-1]);
      vstore32(Out, 0, pDst);
      pDst += local_dst_pitch;
    }
  }
#endif
  event_t evtD = xt_async_work_group_2d_copy(
      Dst + (i * DstPitch) + j , local_dst, outTileW * dstBytes, outTileH,
      DstPitch * dstBytes, local_dst_pitch * dstBytes, 0);

  wait_group_events(1, &evtD);
}
