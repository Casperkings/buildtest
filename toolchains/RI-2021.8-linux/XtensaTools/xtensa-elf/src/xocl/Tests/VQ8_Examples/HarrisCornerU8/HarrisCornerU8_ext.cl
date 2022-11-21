  const uchar128 maskshift1 = {
    1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
    33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
    49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64,
    65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80,
    81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96,
    97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
    112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125,
    126, 127, 128};
  const uchar128 maskshift2 = {
    2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17,
    18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
    34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
    50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65,
    66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80,
    81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96,
    97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
    112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125,
    126, 127, 128, 129};
  const uchar128 maskshift3 = {
    3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18,
    19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34,
    35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
    51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66,
    67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80,
    81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96,
    97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
    112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125,
    126, 127, 128, 129, 130};
  const uchar128 maskshift4 = {
    4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
    36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67,
    68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80,
    81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96,
    97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
    112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125,
    126, 127, 128, 129, 130, 131};

  // kernel using standard OpenCL C
  for (idx = 0; idx < row_size + 4; idx += 128) {
    int y = 0;
    __local uchar *pSrc0 = local_src + y * local_src_pitch + idx;
    uchar128 x00 = vload128(0, pSrc0);
    uchar128 xtail = vload128(0, pSrc0 + 128);
    uchar128 x01 = shuffle2(x00, xtail, maskshift1);
    uchar128 x02 = shuffle2(x00, xtail, maskshift2);

    __local uchar *pSrc1 = local_src + (y + 1) * local_src_pitch + idx;

    uchar128 x10 = vload128(0, pSrc1);
    xtail = vload128(0, pSrc1 + 128);
    uchar128 x11 = shuffle2(x10, xtail, maskshift1);
    uchar128 x12 = shuffle2(x10, xtail, maskshift2);

    int128 x0Sum_ = xt_mul32(x00, (char128)1);
    x0Sum_ = xt_mad32(x01, (char128)2, x0Sum_);
    x0Sum_ = xt_mad32(x02, (char128)1, x0Sum_);
    short128 x0Sum = convert_short128(x0Sum_);

    int128 x1Sum_ = xt_mul32(x10, (char128)1);
    x1Sum_ = xt_mad32(x11, (char128)2, x1Sum_);
    x1Sum_ = xt_mad32(x12, (char128)1, x1Sum_);
    short128 x1Sum = convert_short128(x1Sum_);
  
    int128 x0SumX0_ = xt_mul32(x00, (char128)-1);
    x0SumX0_ = xt_mad32(x02, (char128)1, x0SumX0_);
    short128 x0SumX0 = convert_short128(x0SumX0_);
  
    int128 x1SumX0_ = xt_mul32(x10, (char128)-1);
    x1SumX0_ = xt_mad32(x12, (char128)1, x1SumX0_);
    short128 x1SumX0 = convert_short128(x1SumX0_);
    
    for (int y = 0; y < num_rows + 4; ++y) {
      __local uchar *pSrc2 = local_src + (y + 2) * local_src_pitch + idx;
      uchar128 x20 = vload128(0, pSrc2);
      uchar128 x21 = vload128(0, pSrc2+1);
      uchar128 x22 = vload128(0, pSrc2+2);

      int128 x2Sum_ = xt_mul32(x20, (char128)1);
      x2Sum_ = xt_mad32(x21, (char128)2, x2Sum_);
      x2Sum_ = xt_mad32(x22, (char128)1, x2Sum_);
      short128 x2Sum = convert_short128(x2Sum_);
   
      int128 x2SumX0_ = xt_mul32(x20, (char128)-1);
      x2SumX0_ = xt_mad32(x22, (char128)1, x2SumX0_);
      short128 x2SumX0 = convert_short128(x2SumX0_);
   
      short128 resX = (x0SumX0 + (x1SumX0<<1) + x2SumX0);
      short128 resY = (x2Sum - x0Sum);

      resX = resX >> 3;
      resY = resY >> 3;
   
      vstore64(convert_char64(resX.lo), 0, 
               (char *)(Dx + y * DxDyPitch + idx));
      vstore64(convert_char64(resX.hi), 0, 
               (char *)(Dx + y * DxDyPitch + idx + 64));
      vstore64(convert_char64(resY.lo), 0, 
               (char *)(Dy + y * DxDyPitch + idx));
      vstore64(convert_char64(resY.hi), 0, 
               (char *)(Dy + y * DxDyPitch + idx + 64));
      
      x0Sum = x1Sum;
      x1Sum = x2Sum;
    
      x0SumX0 = x1SumX0;
      x1SumX0 = x2SumX0;  
    }
  }
 
  for (idx = 0; idx < row_size; idx += 128) {
    int128 AccDxDy = 0, AccDx2 = 0, AccDy2 = 0;
#pragma unroll
    for (int y = 0; y < 4; y += 1) {
      char *pSrcDx = Dx + y * DxDyPitch + idx;
      char128 x00Dx = vload128(0, pSrcDx);
      char128 xtailDx = vload128(0, pSrcDx + 128);
      char128 x01Dx = shuffle2(x00Dx, xtailDx, maskshift1);
      char128 x02Dx = shuffle2(x00Dx, xtailDx, maskshift2);
      char128 x03Dx = shuffle2(x00Dx, xtailDx, maskshift3);
      char128 x04Dx = shuffle2(x00Dx, xtailDx, maskshift4);
      
      char *pSrcDy = Dy + y * DxDyPitch + idx;
      char128 x00Dy = vload128(0, pSrcDy);
      char128 xtailDy = vload128(0, pSrcDy + 128);
      char128 x01Dy = shuffle2(x00Dy, xtailDy, maskshift1);
      char128 x02Dy = shuffle2(x00Dy, xtailDy, maskshift2);
      char128 x03Dy = shuffle2(x00Dy, xtailDy, maskshift3);
      char128 x04Dy = shuffle2(x00Dy, xtailDy, maskshift4);
      
      AccDx2 = xt_mad32(x00Dx, x00Dx, AccDx2);
      AccDx2 = xt_mad32(x01Dx, x01Dx, AccDx2);
      AccDx2 = xt_mad32(x02Dx, x02Dx, AccDx2);
      AccDx2 = xt_mad32(x03Dx, x03Dx, AccDx2);
      AccDx2 = xt_mad32(x04Dx, x04Dx, AccDx2);
      
      AccDy2 = xt_mad32(x00Dy, x00Dy, AccDy2);
      AccDy2 = xt_mad32(x01Dy, x01Dy, AccDy2);
      AccDy2 = xt_mad32(x02Dy, x02Dy, AccDy2);
      AccDy2 = xt_mad32(x03Dy, x03Dy, AccDy2);
      AccDy2 = xt_mad32(x04Dy, x04Dy, AccDy2);
      
      AccDxDy = xt_mad32(x00Dx, x00Dy, AccDxDy);
      AccDxDy = xt_mad32(x01Dx, x01Dy, AccDxDy);
      AccDxDy = xt_mad32(x02Dx, x02Dy, AccDxDy);
      AccDxDy = xt_mad32(x03Dx, x03Dy, AccDxDy);
      AccDxDy = xt_mad32(x04Dx, x04Dy, AccDxDy);  
    }

    __local uchar *pDst = (local_dst + idx);
    for (int y = 0; y < num_rows ; ++y) {
      char *pSrcDx = Dx + (y+4) * DxDyPitch + idx;
      char128 x00Dx = vload128(0, pSrcDx);
      char128 xtailDx = vload128(0, pSrcDx + 128);
      char128 x01Dx = shuffle2(x00Dx, xtailDx, maskshift1);
      char128 x02Dx = shuffle2(x00Dx, xtailDx, maskshift2);
      char128 x03Dx = shuffle2(x00Dx, xtailDx, maskshift3);
      char128 x04Dx = shuffle2(x00Dx, xtailDx, maskshift4);
      
      char *pSrcDy = Dy + (y+4) * DxDyPitch + idx;
      char128 x00Dy = vload128(0, pSrcDy);
      char128 xtailDy = vload128(0, pSrcDy + 128);
      char128 x01Dy = shuffle2(x00Dy, xtailDy, maskshift1);
      char128 x02Dy = shuffle2(x00Dy, xtailDy, maskshift2);
      char128 x03Dy = shuffle2(x00Dy, xtailDy, maskshift3);
      char128 x04Dy = shuffle2(x00Dy, xtailDy, maskshift4);
      
      AccDx2 = xt_mad32(x00Dx, x00Dx, AccDx2);
      AccDx2 = xt_mad32(x01Dx, x01Dx, AccDx2);
      AccDx2 = xt_mad32(x02Dx, x02Dx, AccDx2);
      AccDx2 = xt_mad32(x03Dx, x03Dx, AccDx2);
      AccDx2 = xt_mad32(x04Dx, x04Dx, AccDx2);
      
      AccDy2 = xt_mad32(x00Dy, x00Dy, AccDy2);
      AccDy2 = xt_mad32(x01Dy, x01Dy, AccDy2);
      AccDy2 = xt_mad32(x02Dy, x02Dy, AccDy2);
      AccDy2 = xt_mad32(x03Dy, x03Dy, AccDy2);
      AccDy2 = xt_mad32(x04Dy, x04Dy, AccDy2);
      
      AccDxDy = xt_mad32(x00Dx, x00Dy, AccDxDy);
      AccDxDy = xt_mad32(x01Dx, x01Dy, AccDxDy);
      AccDxDy = xt_mad32(x02Dx, x02Dy, AccDxDy);
      AccDxDy = xt_mad32(x03Dx, x03Dy, AccDxDy);
      AccDxDy = xt_mad32(x04Dx, x04Dy, AccDxDy);  

      short128 Dx2, Dy2, DxDy;
      Dx2 = xt_convert_short128(AccDx2, 6);
      Dy2 = xt_convert_short128(AccDy2, 6);
      DxDy = xt_convert_short128(AccDxDy, 6);

      short128 SumDxDy = Dx2 + Dy2;
      SumDxDy = SumDxDy >> 5;
      int128 Acc;
      Acc = xt_mul32(Dx2, Dy2);
      Acc = xt_mad32(DxDy, -DxDy, Acc);
      Acc = xt_mad32(SumDxDy, (short128)-1, Acc);  
      short128 Result = xt_convert_short128(Acc, 8);
      int128 R = xt_mul32(Result, (char128)1);
      uchar128 Rs = xt_convert_uchar128_sat_rnd(R, 0);
      vstore128(Rs, 0, pDst); 

      pDst += local_dst_pitch;    
    
      pSrcDx = Dx + y * DxDyPitch + idx;
      x00Dx = vload128(0, pSrcDx);
      xtailDx = vload128(0, pSrcDx + 128);
      x01Dx = shuffle2(x00Dx, xtailDx, maskshift1);
      x02Dx = shuffle2(x00Dx, xtailDx, maskshift2);
      x03Dx = shuffle2(x00Dx, xtailDx, maskshift3);
      x04Dx = shuffle2(x00Dx, xtailDx, maskshift4);
      
      pSrcDy = Dy + y * DxDyPitch + idx;
      x00Dy = vload128(0, pSrcDy);
      xtailDy = vload128(0, pSrcDy + 128);
      x01Dy = shuffle2(x00Dy, xtailDy, maskshift1);
      x02Dy = shuffle2(x00Dy, xtailDy, maskshift2);
      x03Dy = shuffle2(x00Dy, xtailDy, maskshift3);
      x04Dy = shuffle2(x00Dy, xtailDy, maskshift4);
      
      AccDx2 = xt_mad32(x00Dx, -x00Dx, AccDx2);
      AccDx2 = xt_mad32(x01Dx, -x01Dx, AccDx2);
      AccDx2 = xt_mad32(x02Dx, -x02Dx, AccDx2);
      AccDx2 = xt_mad32(x03Dx, -x03Dx, AccDx2);
      AccDx2 = xt_mad32(x04Dx, -x04Dx, AccDx2);
      
      AccDy2 = xt_mad32(x00Dy, -x00Dy, AccDy2);
      AccDy2 = xt_mad32(x01Dy, -x01Dy, AccDy2);
      AccDy2 = xt_mad32(x02Dy, -x02Dy, AccDy2);
      AccDy2 = xt_mad32(x03Dy, -x03Dy, AccDy2);
      AccDy2 = xt_mad32(x04Dy, -x04Dy, AccDy2);
      
      AccDxDy = xt_mad32(x00Dx, -x00Dy, AccDxDy);
      AccDxDy = xt_mad32(x01Dx, -x01Dy, AccDxDy);
      AccDxDy = xt_mad32(x02Dx, -x02Dy, AccDxDy);
      AccDxDy = xt_mad32(x03Dx, -x03Dy, AccDxDy);
      AccDxDy = xt_mad32(x04Dx, -x04Dy, AccDxDy);  
    }
  }
