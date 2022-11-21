#define TILE_W 128
#define TILE_H 64
#define W_EXT 3
#define H_EXT 3
#define IMAGE_PAD 3
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

  outTileW = TILE_W;
  outTileH = TILE_H;
  local_dst_pitch = outTileW; 
  dstBytes = DST_BIT_DEPTH >> 3;

  i = get_global_id(0) * num_rows;
  j = get_global_id(1) * row_size;
  
  char Dx[(TILE_W + 64) * (TILE_H + IMAGE_PAD*2) + 128];
  char Dy[(TILE_W + 64) * (TILE_H + IMAGE_PAD*2) + 128];
  
  int DxDyPitch = TILE_W + 64;
  event_t evtA = xt_async_work_group_2d_copy(
      local_src, Src + (i * SrcPitch) + j, local_src_pitch,
      (num_rows + pad + pad), local_src_pitch, SrcPitch, 0);
  wait_group_events(1, &evtA);

#if NATIVE_KERNEL
  #include "HarrisCornerU8_ivp.c"
#elif XT_OCL_EXT
  #include "HarrisCornerU8_ext.cl"
#else
  const ushort32 maskshift1 = {
    1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32};
  const ushort32 maskshift2 = {
    2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17,
    18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33};  
  const ushort32 maskshift3 = {
    3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18,
    19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34};
  const ushort32 maskshift4 = {
    4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35};

   __local uchar * pSrc = local_src;
  // kernel using standard OpenCL C
  for (idx = 0; idx < row_size + 4; idx += 32) {
    int y = 0;
    __local uchar *pSrc0 = pSrc + y * local_src_pitch + idx;
    short32 x00 = convert_short32(vload32(0, pSrc0));
    short32 xtail = convert_short32(vload32(0, pSrc0 + 32));
    short32 x01 = shuffle2(x00, xtail, maskshift1);
    short32 x02 = shuffle2(x00, xtail, maskshift2);

    __local uchar *pSrc1 = pSrc + (y + 1) * local_src_pitch + idx;

    short32 x10 = convert_short32(vload32(0, pSrc1));
    xtail = convert_short32(vload32(0, pSrc1 + 32));
    short32 x11 = shuffle2(x10, xtail, maskshift1);
    short32 x12 = shuffle2(x10, xtail, maskshift2);

    __local uchar *pSrc3 = pSrc + (y + 3) * local_src_pitch + idx;
    short32 x30 = convert_short32(vload32(0, pSrc3));
    xtail = convert_short32(vload32(0, pSrc3 + 32));
    short32 x31 = shuffle2(x30, xtail, maskshift1);
    short32 x32 = shuffle2(x30, xtail, maskshift2);

    short32 x0Sum = x00 + (short32)2 * x01 + x02 ;
    short32 x1Sum = x10 + (short32)2 * x11 + x12 ;
    
    short32 x0SumX0 = - x00 + x02 ;
    short32 x1SumX0 = - x10 + x12;

    for (int y = 0; y < num_rows + 4; ++y) {
      __local uchar *pSrc2 = pSrc + (y + 2) * local_src_pitch + idx;
      short32 x20 = convert_short32(vload32(0, pSrc2));
      xtail = convert_short32(vload32(0, pSrc2 + 32));
      short32 x21 = shuffle2(x20, xtail, maskshift1);
      short32 x22 = shuffle2(x20, xtail, maskshift2);

      short32 x2Sum = x20 + (x21<<1) + x22;

      short32 x2SumX0 = - x20 + x22;
    
      short32 resX = (short32)(x0SumX0 + (x1SumX0<<1) + x2SumX0);

      short32 resY = (short32)(x2Sum - x0Sum);
  
      resX = resX >> 3;
      resY = resY >> 3;
    
      char32 xDx = convert_char32(resX);
      char32 xDy = convert_char32(resY);
      
      vstore32(xDx, 0, (char *)(Dx + y * DxDyPitch + idx));
      vstore32(xDy, 0, (char *)(Dy + y * DxDyPitch + idx));
      
      x0Sum = x1Sum;
      x1Sum = x2Sum;
    
      x0SumX0 = x1SumX0;
      x1SumX0 = x2SumX0;
    }
  }

  int DxDy_Buff[32*8], Dx2_Buff[32*8], Dy2_Buff[32*8];

  for (idx = 0; idx < row_size; idx += 32) {
    int32 AccDxDy = 0, AccDx2 = 0, AccDy2 = 0;
    int write = 0, read = 0;
    for (int y = 0; y < 4; y += 1) {
      char *pSrcDx = Dx + y * DxDyPitch + idx;
      short32 x00Dx = convert_short32(vload32(0, pSrcDx));
      short32 xtailDx = convert_short32(vload32(0, pSrcDx + 32));
      short32 x01Dx = shuffle2(x00Dx, xtailDx, maskshift1);
      short32 x02Dx = shuffle2(x00Dx, xtailDx, maskshift2);
      short32 x03Dx = shuffle2(x00Dx, xtailDx, maskshift3);
      short32 x04Dx = shuffle2(x00Dx, xtailDx, maskshift4);
       
      char *pSrcDy = Dy + y * DxDyPitch + idx;
      short32 x00Dy = convert_short32(vload32(0, pSrcDy));
      short32 xtailDy = convert_short32(vload32(0, pSrcDy + 32));
      short32 x01Dy = shuffle2(x00Dy, xtailDy, maskshift1);
      short32 x02Dy = shuffle2(x00Dy, xtailDy, maskshift2);
      short32 x03Dy = shuffle2(x00Dy, xtailDy, maskshift3);
      short32 x04Dy = shuffle2(x00Dy, xtailDy, maskshift4);
      
      int32 _AccDx2 = xt_mul32(x00Dx, x00Dx);
      _AccDx2 += xt_mul32(x01Dx, x01Dx);
      _AccDx2 += xt_mul32(x02Dx, x02Dx);
      _AccDx2 += xt_mul32(x03Dx, x03Dx);
      _AccDx2 += xt_mul32(x04Dx, x04Dx);
      
      int32 _AccDy2 = xt_mul32(x00Dy, x00Dy);
      _AccDy2 += xt_mul32(x01Dy, x01Dy);
      _AccDy2 += xt_mul32(x02Dy, x02Dy);
      _AccDy2 += xt_mul32(x03Dy, x03Dy);
      _AccDy2 += xt_mul32(x04Dy, x04Dy);
      
      int32 _AccDxDy = xt_mul32(x00Dx, x00Dy);
      _AccDxDy += xt_mul32(x01Dx, x01Dy);
      _AccDxDy += xt_mul32(x02Dx, x02Dy);
      _AccDxDy += xt_mul32(x03Dx, x03Dy);
      _AccDxDy += xt_mul32(x04Dx, x04Dy);  
      
      AccDx2 += _AccDx2;
      AccDy2 += _AccDy2;
      AccDxDy += _AccDxDy;
      
      vstore32(_AccDxDy, 0, DxDy_Buff + write*32);
      vstore32(_AccDx2, 0, Dx2_Buff + write*32);
      vstore32(_AccDy2, 0, Dy2_Buff + write*32);
      write++;
      write = write & 7;
    }

    __local uchar *pDst = (local_dst + idx);
    for (int y = 0; y < num_rows ; ++y) {
      char *pSrcDx = Dx + (y + 4) * DxDyPitch + idx;
      short32 x00Dx = convert_short32(vload32(0, pSrcDx));
      short32 xtailDx = convert_short32(vload32(0, pSrcDx + 32));
      short32 x01Dx = shuffle2(x00Dx, xtailDx, maskshift1);
      short32 x02Dx = shuffle2(x00Dx, xtailDx, maskshift2);
      short32 x03Dx = shuffle2(x00Dx, xtailDx, maskshift3);
      short32 x04Dx = shuffle2(x00Dx, xtailDx, maskshift4);
      
      char *pSrcDy = Dy + (y + 4) * DxDyPitch + idx;
      short32 x00Dy = convert_short32(vload32(0, pSrcDy));
      short32 xtailDy = convert_short32(vload32(0, pSrcDy + 32));
      short32 x01Dy = shuffle2(x00Dy, xtailDy, maskshift1);
      short32 x02Dy = shuffle2(x00Dy, xtailDy, maskshift2);
      short32 x03Dy = shuffle2(x00Dy, xtailDy, maskshift3);
      short32 x04Dy = shuffle2(x00Dy, xtailDy, maskshift4);
      
      int32 _AccDx2 = xt_mul32(x00Dx, x00Dx);
      _AccDx2 += xt_mul32(x01Dx, x01Dx);
      _AccDx2 += xt_mul32(x02Dx, x02Dx);
      _AccDx2 += xt_mul32(x03Dx, x03Dx);
      _AccDx2 += xt_mul32(x04Dx, x04Dx);
      
      int32 _AccDy2 = xt_mul32(x00Dy, x00Dy);
      _AccDy2 += xt_mul32(x01Dy, x01Dy);
      _AccDy2 += xt_mul32(x02Dy, x02Dy);
      _AccDy2 += xt_mul32(x03Dy, x03Dy);
      _AccDy2 += xt_mul32(x04Dy, x04Dy);
      
      int32 _AccDxDy = xt_mul32(x00Dx, x00Dy);
      _AccDxDy += xt_mul32(x01Dx, x01Dy);
      _AccDxDy += xt_mul32(x02Dx, x02Dy);
      _AccDxDy += xt_mul32(x03Dx, x03Dy);
      _AccDxDy += xt_mul32(x04Dx, x04Dy);
    
      AccDx2 += _AccDx2;
      AccDy2 += _AccDy2;
      AccDxDy += _AccDxDy;
      
      vstore32(_AccDxDy, 0, DxDy_Buff + write*32);
      vstore32(_AccDx2, 0, Dx2_Buff + write*32);
      vstore32(_AccDy2, 0, Dy2_Buff + write*32);
      write++;
      write = write & 7;
      
      short32 Dx2 = convert_short32((AccDx2 >> 6));
      short32 Dy2 = convert_short32((AccDy2 >> 6));
      short32 DxDy = convert_short32((AccDxDy >> 6));
      
      short32 SumDxDy = Dx2 + Dy2;
      SumDxDy = SumDxDy >> 5;
      int32 Acc = xt_mul32(Dx2, Dy2);
      Acc -= xt_mul32(DxDy, DxDy);
      Acc -= xt_mul32(SumDxDy, (short32) 1);
      Acc = Acc >> 8;
      short32 Result = convert_short32(Acc);
      Result = max(Result, 0);
      Result = min(Result, 255);
      
      uchar32 R = convert_uchar32(Result);
      vstore32(R, 0, pDst);      
      pDst += local_dst_pitch;      
      
      _AccDxDy = vload32(0,DxDy_Buff + read*32);
      _AccDx2 = vload32(0,Dx2_Buff + read*32);
      _AccDy2 = vload32(0,Dy2_Buff + read*32);
      
      AccDxDy -= _AccDxDy;
      AccDx2 -= _AccDx2;
      AccDy2 -= _AccDy2;
      
      read++;
      read = read & 7;
    }
  }
    
#endif
  event_t evtD = xt_async_work_group_2d_copy(
      Dst + (i * DstPitch) + j, local_dst, outTileW * dstBytes, num_rows,
      DstPitch * dstBytes, local_dst_pitch * dstBytes, 0);

  wait_group_events(1, &evtD);
}
