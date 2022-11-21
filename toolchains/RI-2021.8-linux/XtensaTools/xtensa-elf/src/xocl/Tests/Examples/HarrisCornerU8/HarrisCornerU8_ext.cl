  const uchar64 maskshift1 = {
    1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
    33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
    49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64};
  const uchar64 maskshift2 = {
    2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17,
    18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
    34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
    50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65};
  const uchar64 maskshift3 = {
    3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18,
    19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34,
    35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
    51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66};
  const uchar64 maskshift4 = {
    4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
    36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67};

  // kernel using standard OpenCL C
  for (idx = 0; idx < row_size + 4; idx += 64) {
    int y = 0;
    __local uchar *pSrc0 = local_src + y * local_src_pitch + idx;
    uchar64 x00 = vload64(0, pSrc0);
    uchar64 xtail = vload64(0, pSrc0 + 64);
    uchar64 x01 = shuffle2(x00, xtail, maskshift1);
    uchar64 x02 = shuffle2(x00, xtail, maskshift2);

    __local uchar *pSrc1 = local_src + (y + 1) * local_src_pitch + idx;

    uchar64 x10 = vload64(0, pSrc1);
    xtail = vload64(0, pSrc1 + 64);
    uchar64 x11 = shuffle2(x10, xtail, maskshift1);
    uchar64 x12 = shuffle2(x10, xtail, maskshift2);

    int64 x0Sum_ = xt_mul24(x00, (char64)1);
    x0Sum_ = xt_mad24(x01, (char64)2, x0Sum_);
    x0Sum_ = xt_mad24(x02, (char64)1, x0Sum_);
    short64 x0Sum = xt_convert24_short64(x0Sum_);

    int64 x1Sum_ = xt_mul24(x10, (char64)1);
    x1Sum_ = xt_mad24(x11, (char64)2, x1Sum_);
    x1Sum_ = xt_mad24(x12, (char64)1, x1Sum_);
    short64 x1Sum = xt_convert24_short64(x1Sum_);
  
    int64 x0SumX0_ = xt_mul24(x00, (char64)-1);
    x0SumX0_ = xt_mad24(x02, (char64)1, x0SumX0_);
    short64 x0SumX0 = xt_convert24_short64(x0SumX0_);
  
    int64 x1SumX0_ = xt_mul24(x10, (char64)-1);
    x1SumX0_ = xt_mad24(x12, (char64)1, x1SumX0_);
    short64 x1SumX0 = xt_convert24_short64(x1SumX0_);
    
    for (int y = 0; y < num_rows + 4; ++y) {
      __local uchar *pSrc2 = local_src + (y + 2) * local_src_pitch + idx;
      uchar64 x20 = vload64(0, pSrc2);
      uchar64 x21 = vload64(0, pSrc2+1);
      uchar64 x22 = vload64(0, pSrc2+2);

      int64 x2Sum_ = xt_mul24(x20, (char64)1);
      x2Sum_ = xt_mad24(x21, (char64)2, x2Sum_);
      x2Sum_ = xt_mad24(x22, (char64)1, x2Sum_);
      short64 x2Sum = xt_convert24_short64(x2Sum_);
   
      int64 x2SumX0_ = xt_mul24(x20, (char64)-1);
      x2SumX0_ = xt_mad24(x22, (char64)1, x2SumX0_);
      short64 x2SumX0 = xt_convert24_short64(x2SumX0_);
   
      short64 resX = (x0SumX0 + (x1SumX0<<1) + x2SumX0);
      short64 resY = (x2Sum - x0Sum);

      resX = resX >> 3;
      resY = resY >> 3;
   
      vstore32(convert_char32(resX.lo), 0, 
               (char *)(Dx + y * DxDyPitch + idx));
      vstore32(convert_char32(resX.hi), 0, 
               (char *)(Dx + y * DxDyPitch + idx + 32));
      vstore32(convert_char32(resY.lo), 0, 
               (char *)(Dy + y * DxDyPitch + idx));
      vstore32(convert_char32(resY.hi), 0, 
               (char *)(Dy + y * DxDyPitch + idx + 32));
      
      x0Sum = x1Sum;
      x1Sum = x2Sum;
    
      x0SumX0 = x1SumX0;
      x1SumX0 = x2SumX0;  
    }
  }
 
  for (idx = 0; idx < row_size; idx += 64) {
    int64 AccDxDy = 0, AccDx2 = 0, AccDy2 = 0;
#pragma unroll
    for (int y = 0; y < 4; y += 1) {
      char *pSrcDx = Dx + y * DxDyPitch + idx;
      char64 x00Dx = vload64(0, pSrcDx);
      char64 xtailDx = vload64(0, pSrcDx + 64);
      char64 x01Dx = shuffle2(x00Dx, xtailDx, maskshift1);
      char64 x02Dx = shuffle2(x00Dx, xtailDx, maskshift2);
      char64 x03Dx = shuffle2(x00Dx, xtailDx, maskshift3);
      char64 x04Dx = shuffle2(x00Dx, xtailDx, maskshift4);
      
      char *pSrcDy = Dy + y * DxDyPitch + idx;
      char64 x00Dy = vload64(0, pSrcDy);
      char64 xtailDy = vload64(0, pSrcDy + 64);
      char64 x01Dy = shuffle2(x00Dy, xtailDy, maskshift1);
      char64 x02Dy = shuffle2(x00Dy, xtailDy, maskshift2);
      char64 x03Dy = shuffle2(x00Dy, xtailDy, maskshift3);
      char64 x04Dy = shuffle2(x00Dy, xtailDy, maskshift4);
      
      AccDx2 = xt_mad24(x00Dx, x00Dx, AccDx2);
      AccDx2 = xt_mad24(x01Dx, x01Dx, AccDx2);
      AccDx2 = xt_mad24(x02Dx, x02Dx, AccDx2);
      AccDx2 = xt_mad24(x03Dx, x03Dx, AccDx2);
      AccDx2 = xt_mad24(x04Dx, x04Dx, AccDx2);
      
      AccDy2 = xt_mad24(x00Dy, x00Dy, AccDy2);
      AccDy2 = xt_mad24(x01Dy, x01Dy, AccDy2);
      AccDy2 = xt_mad24(x02Dy, x02Dy, AccDy2);
      AccDy2 = xt_mad24(x03Dy, x03Dy, AccDy2);
      AccDy2 = xt_mad24(x04Dy, x04Dy, AccDy2);
      
      AccDxDy = xt_mad24(x00Dx, x00Dy, AccDxDy);
      AccDxDy = xt_mad24(x01Dx, x01Dy, AccDxDy);
      AccDxDy = xt_mad24(x02Dx, x02Dy, AccDxDy);
      AccDxDy = xt_mad24(x03Dx, x03Dy, AccDxDy);
      AccDxDy = xt_mad24(x04Dx, x04Dy, AccDxDy);  
    }

    __local uchar *pDst = (local_dst + idx);
    for (int y = 0; y < num_rows ; ++y) {
      char *pSrcDx = Dx + (y+4) * DxDyPitch + idx;
      char64 x00Dx = vload64(0, pSrcDx);
      char64 xtailDx = vload64(0, pSrcDx + 64);
      char64 x01Dx = shuffle2(x00Dx, xtailDx, maskshift1);
      char64 x02Dx = shuffle2(x00Dx, xtailDx, maskshift2);
      char64 x03Dx = shuffle2(x00Dx, xtailDx, maskshift3);
      char64 x04Dx = shuffle2(x00Dx, xtailDx, maskshift4);
      
      char *pSrcDy = Dy + (y+4) * DxDyPitch + idx;
      char64 x00Dy = vload64(0, pSrcDy);
      char64 xtailDy = vload64(0, pSrcDy + 64);
      char64 x01Dy = shuffle2(x00Dy, xtailDy, maskshift1);
      char64 x02Dy = shuffle2(x00Dy, xtailDy, maskshift2);
      char64 x03Dy = shuffle2(x00Dy, xtailDy, maskshift3);
      char64 x04Dy = shuffle2(x00Dy, xtailDy, maskshift4);
      
      AccDx2 = xt_mad24(x00Dx, x00Dx, AccDx2);
      AccDx2 = xt_mad24(x01Dx, x01Dx, AccDx2);
      AccDx2 = xt_mad24(x02Dx, x02Dx, AccDx2);
      AccDx2 = xt_mad24(x03Dx, x03Dx, AccDx2);
      AccDx2 = xt_mad24(x04Dx, x04Dx, AccDx2);
      
      AccDy2 = xt_mad24(x00Dy, x00Dy, AccDy2);
      AccDy2 = xt_mad24(x01Dy, x01Dy, AccDy2);
      AccDy2 = xt_mad24(x02Dy, x02Dy, AccDy2);
      AccDy2 = xt_mad24(x03Dy, x03Dy, AccDy2);
      AccDy2 = xt_mad24(x04Dy, x04Dy, AccDy2);
      
      AccDxDy = xt_mad24(x00Dx, x00Dy, AccDxDy);
      AccDxDy = xt_mad24(x01Dx, x01Dy, AccDxDy);
      AccDxDy = xt_mad24(x02Dx, x02Dy, AccDxDy);
      AccDxDy = xt_mad24(x03Dx, x03Dy, AccDxDy);
      AccDxDy = xt_mad24(x04Dx, x04Dy, AccDxDy);  

      short64 Dx2, Dy2, DxDy;
      Dx2 = xt_convert24_short64(AccDx2, 6);
      Dy2 = xt_convert24_short64(AccDy2, 6);
      DxDy = xt_convert24_short64(AccDxDy, 6);

      short64 SumDxDy = Dx2 + Dy2;
      SumDxDy = SumDxDy >> 5;
      int64 Acc;
      Acc = xt_mul32(Dx2, Dy2);
      Acc = xt_mad32(DxDy, -DxDy, Acc);
      Acc = xt_mad32(SumDxDy, (short64)-1, Acc);  
      short64 Result = xt_convert_short64(Acc, 8);
      int64 R = xt_mul24(Result, (char64)1);
      uchar64 Rs = xt_convert24_uchar64_sat_rnd(R, 0);
      vstore64(Rs, 0, pDst); 

      pDst += local_dst_pitch;    
    
      pSrcDx = Dx + y * DxDyPitch + idx;
      x00Dx = vload64(0, pSrcDx);
      xtailDx = vload64(0, pSrcDx + 64);
      x01Dx = shuffle2(x00Dx, xtailDx, maskshift1);
      x02Dx = shuffle2(x00Dx, xtailDx, maskshift2);
      x03Dx = shuffle2(x00Dx, xtailDx, maskshift3);
      x04Dx = shuffle2(x00Dx, xtailDx, maskshift4);
      
      pSrcDy = Dy + y * DxDyPitch + idx;
      x00Dy = vload64(0, pSrcDy);
      xtailDy = vload64(0, pSrcDy + 64);
      x01Dy = shuffle2(x00Dy, xtailDy, maskshift1);
      x02Dy = shuffle2(x00Dy, xtailDy, maskshift2);
      x03Dy = shuffle2(x00Dy, xtailDy, maskshift3);
      x04Dy = shuffle2(x00Dy, xtailDy, maskshift4);
      
      AccDx2 = xt_mad24(x00Dx, -x00Dx, AccDx2);
      AccDx2 = xt_mad24(x01Dx, -x01Dx, AccDx2);
      AccDx2 = xt_mad24(x02Dx, -x02Dx, AccDx2);
      AccDx2 = xt_mad24(x03Dx, -x03Dx, AccDx2);
      AccDx2 = xt_mad24(x04Dx, -x04Dx, AccDx2);
      
      AccDy2 = xt_mad24(x00Dy, -x00Dy, AccDy2);
      AccDy2 = xt_mad24(x01Dy, -x01Dy, AccDy2);
      AccDy2 = xt_mad24(x02Dy, -x02Dy, AccDy2);
      AccDy2 = xt_mad24(x03Dy, -x03Dy, AccDy2);
      AccDy2 = xt_mad24(x04Dy, -x04Dy, AccDy2);
      
      AccDxDy = xt_mad24(x00Dx, -x00Dy, AccDxDy);
      AccDxDy = xt_mad24(x01Dx, -x01Dy, AccDxDy);
      AccDxDy = xt_mad24(x02Dx, -x02Dy, AccDxDy);
      AccDxDy = xt_mad24(x03Dx, -x03Dy, AccDxDy);
      AccDxDy = xt_mad24(x04Dx, -x04Dy, AccDxDy);  
    }
  }
