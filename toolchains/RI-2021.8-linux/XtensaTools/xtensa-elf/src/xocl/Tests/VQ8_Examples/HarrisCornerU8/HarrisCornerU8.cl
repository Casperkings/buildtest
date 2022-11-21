#define TILE_W 256
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
  
  char Dx[(TILE_W + 128) * (TILE_H + IMAGE_PAD*2) + 128];
  char Dy[(TILE_W + 128) * (TILE_H + IMAGE_PAD*2) + 128];
  
  int DxDyPitch = TILE_W + 128;
  event_t evtA = xt_async_work_group_2d_copy(
      local_src, Src + (i * SrcPitch) + j, local_src_pitch,
      (num_rows + pad + pad), local_src_pitch, SrcPitch, 0);
  wait_group_events(1, &evtA);

#if NATIVE_KERNEL
  #include "HarrisCornerU8_ivp.c"
#elif XT_OCL_EXT
  #include "HarrisCornerU8_ext.cl"
#else
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

   __local uchar * pSrc = local_src;
  // kernel using standard OpenCL C
  for (idx = 0; idx < row_size + 4; idx += 64) {
    int y = 0;
    __local uchar *pSrc0 = pSrc + y * local_src_pitch + idx;
    short64 x00 = convert_short64(vload64(0, pSrc0));
    short64 xtail = convert_short64(vload64(0, pSrc0 + 64));
    short64 x01 = shuffle2(x00, xtail, maskshift1);
    short64 x02 = shuffle2(x00, xtail, maskshift2);

    __local uchar *pSrc1 = pSrc + (y + 1) * local_src_pitch + idx;

    short64 x10 = convert_short64(vload64(0, pSrc1));
    xtail = convert_short64(vload64(0, pSrc1 + 64));
    short64 x11 = shuffle2(x10, xtail, maskshift1);
    short64 x12 = shuffle2(x10, xtail, maskshift2);

    __local uchar *pSrc3 = pSrc + (y + 3) * local_src_pitch + idx;
    short64 x30 = convert_short64(vload64(0, pSrc3));
    xtail = convert_short64(vload64(0, pSrc3 + 64));
    short64 x31 = shuffle2(x30, xtail, maskshift1);
    short64 x32 = shuffle2(x30, xtail, maskshift2);

    short64 x0Sum = x00 + (short64)2 * x01 + x02 ;
    short64 x1Sum = x10 + (short64)2 * x11 + x12 ;
    
    short64 x0SumX0 = - x00 + x02 ;
    short64 x1SumX0 = - x10 + x12;

    for (int y = 0; y < num_rows + 4; ++y) {
      __local uchar *pSrc2 = pSrc + (y + 2) * local_src_pitch + idx;
      short64 x20 = convert_short64(vload64(0, pSrc2));
      xtail = convert_short64(vload64(0, pSrc2 + 64));
      short64 x21 = shuffle2(x20, xtail, maskshift1);
      short64 x22 = shuffle2(x20, xtail, maskshift2);

      short64 x2Sum = x20 + (x21<<1) + x22;

      short64 x2SumX0 = - x20 + x22;
    
      short64 resX = (short64)(x0SumX0 + (x1SumX0<<1) + x2SumX0);

      short64 resY = (short64)(x2Sum - x0Sum);
  
      resX = resX >> 3;
      resY = resY >> 3;
    
      char64 xDx = convert_char64(resX);
      char64 xDy = convert_char64(resY);
      
      vstore64(xDx, 0, (char *)(Dx + y * DxDyPitch + idx));
      vstore64(xDy, 0, (char *)(Dy + y * DxDyPitch + idx));
      
      x0Sum = x1Sum;
      x1Sum = x2Sum;
    
      x0SumX0 = x1SumX0;
      x1SumX0 = x2SumX0;
    }
  }

  int DxDy_Buff[64*8], Dx2_Buff[64*8], Dy2_Buff[64*8];

  for (idx = 0; idx < row_size; idx += 64) {
    int64 AccDxDy = 0, AccDx2 = 0, AccDy2 = 0;
    int write = 0, read = 0;
    for (int y = 0; y < 4; y += 1) {
      char *pSrcDx = Dx + y * DxDyPitch + idx;
      short64 x00Dx = convert_short64(vload64(0, pSrcDx));
      short64 xtailDx = convert_short64(vload64(0, pSrcDx + 64));
      short64 x01Dx = shuffle2(x00Dx, xtailDx, maskshift1);
      short64 x02Dx = shuffle2(x00Dx, xtailDx, maskshift2);
      short64 x03Dx = shuffle2(x00Dx, xtailDx, maskshift3);
      short64 x04Dx = shuffle2(x00Dx, xtailDx, maskshift4);
       
      char *pSrcDy = Dy + y * DxDyPitch + idx;
      short64 x00Dy = convert_short64(vload64(0, pSrcDy));
      short64 xtailDy = convert_short64(vload64(0, pSrcDy + 64));
      short64 x01Dy = shuffle2(x00Dy, xtailDy, maskshift1);
      short64 x02Dy = shuffle2(x00Dy, xtailDy, maskshift2);
      short64 x03Dy = shuffle2(x00Dy, xtailDy, maskshift3);
      short64 x04Dy = shuffle2(x00Dy, xtailDy, maskshift4);
      
      int64 _AccDx2 = xt_mul32(x00Dx, x00Dx);
      _AccDx2 += xt_mul32(x01Dx, x01Dx);
      _AccDx2 += xt_mul32(x02Dx, x02Dx);
      _AccDx2 += xt_mul32(x03Dx, x03Dx);
      _AccDx2 += xt_mul32(x04Dx, x04Dx);
      
      int64 _AccDy2 = xt_mul32(x00Dy, x00Dy);
      _AccDy2 += xt_mul32(x01Dy, x01Dy);
      _AccDy2 += xt_mul32(x02Dy, x02Dy);
      _AccDy2 += xt_mul32(x03Dy, x03Dy);
      _AccDy2 += xt_mul32(x04Dy, x04Dy);
      
      int64 _AccDxDy = xt_mul32(x00Dx, x00Dy);
      _AccDxDy += xt_mul32(x01Dx, x01Dy);
      _AccDxDy += xt_mul32(x02Dx, x02Dy);
      _AccDxDy += xt_mul32(x03Dx, x03Dy);
      _AccDxDy += xt_mul32(x04Dx, x04Dy);  
      
      AccDx2 += _AccDx2;
      AccDy2 += _AccDy2;
      AccDxDy += _AccDxDy;
      
      vstore64(_AccDxDy, 0, DxDy_Buff + write*64);
      vstore64(_AccDx2, 0, Dx2_Buff + write*64);
      vstore64(_AccDy2, 0, Dy2_Buff + write*64);
      write++;
      write = write & 7;
    }

    __local uchar *pDst = (local_dst + idx);
    for (int y = 0; y < num_rows ; ++y) {
      char *pSrcDx = Dx + (y + 4) * DxDyPitch + idx;
      short64 x00Dx = convert_short64(vload64(0, pSrcDx));
      short64 xtailDx = convert_short64(vload64(0, pSrcDx + 64));
      short64 x01Dx = shuffle2(x00Dx, xtailDx, maskshift1);
      short64 x02Dx = shuffle2(x00Dx, xtailDx, maskshift2);
      short64 x03Dx = shuffle2(x00Dx, xtailDx, maskshift3);
      short64 x04Dx = shuffle2(x00Dx, xtailDx, maskshift4);
      
      char *pSrcDy = Dy + (y + 4) * DxDyPitch + idx;
      short64 x00Dy = convert_short64(vload64(0, pSrcDy));
      short64 xtailDy = convert_short64(vload64(0, pSrcDy + 64));
      short64 x01Dy = shuffle2(x00Dy, xtailDy, maskshift1);
      short64 x02Dy = shuffle2(x00Dy, xtailDy, maskshift2);
      short64 x03Dy = shuffle2(x00Dy, xtailDy, maskshift3);
      short64 x04Dy = shuffle2(x00Dy, xtailDy, maskshift4);
      
      int64 _AccDx2 = xt_mul32(x00Dx, x00Dx);
      _AccDx2 += xt_mul32(x01Dx, x01Dx);
      _AccDx2 += xt_mul32(x02Dx, x02Dx);
      _AccDx2 += xt_mul32(x03Dx, x03Dx);
      _AccDx2 += xt_mul32(x04Dx, x04Dx);
      
      int64 _AccDy2 = xt_mul32(x00Dy, x00Dy);
      _AccDy2 += xt_mul32(x01Dy, x01Dy);
      _AccDy2 += xt_mul32(x02Dy, x02Dy);
      _AccDy2 += xt_mul32(x03Dy, x03Dy);
      _AccDy2 += xt_mul32(x04Dy, x04Dy);
      
      int64 _AccDxDy = xt_mul32(x00Dx, x00Dy);
      _AccDxDy += xt_mul32(x01Dx, x01Dy);
      _AccDxDy += xt_mul32(x02Dx, x02Dy);
      _AccDxDy += xt_mul32(x03Dx, x03Dy);
      _AccDxDy += xt_mul32(x04Dx, x04Dy);
    
      AccDx2 += _AccDx2;
      AccDy2 += _AccDy2;
      AccDxDy += _AccDxDy;
      
      vstore64(_AccDxDy, 0, DxDy_Buff + write*64);
      vstore64(_AccDx2, 0, Dx2_Buff + write*64);
      vstore64(_AccDy2, 0, Dy2_Buff + write*64);
      write++;
      write = write & 7;
      
      short64 Dx2 = convert_short64((AccDx2 >> 6));
      short64 Dy2 = convert_short64((AccDy2 >> 6));
      short64 DxDy = convert_short64((AccDxDy >> 6));
      
      short64 SumDxDy = Dx2 + Dy2;
      SumDxDy = SumDxDy >> 5;
      int64 Acc = xt_mul32(Dx2, Dy2);
      Acc -= xt_mul32(DxDy, DxDy);
      Acc -= xt_mul32(SumDxDy, (short64)1);
      Acc = Acc >> 8;
      short64 Result = convert_short64(Acc);
      Result = max(Result, 0);
      Result = min(Result, 255);
      
      uchar64 R = convert_uchar64(Result);
      vstore64(R, 0, pDst);      
      pDst += local_dst_pitch;      
      
      _AccDxDy = vload64(0,DxDy_Buff + read*64);
      _AccDx2 = vload64(0,Dx2_Buff + read*64);
      _AccDy2 = vload64(0,Dy2_Buff + read*64);
      
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
