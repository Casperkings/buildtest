  __constant char* Coef = (__constant char*)filtCoeff;
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
  for (idx = 0; idx < row_size; idx += 64) {
    int y = 0;
    __local uchar *pSrc0 = local_src + y * local_src_pitch + idx;
    uchar64 x00 = vload64(0, pSrc0);
    uchar64 xtail = vload64(0, pSrc0 + 64);
    uchar64 x01 = shuffle2(x00, xtail, maskshift1);
    uchar64 x02 = shuffle2(x00, xtail, maskshift2);
    uchar64 x03 = shuffle2(x00, xtail, maskshift3);
    uchar64 x04 = shuffle2(x00, xtail, maskshift4);

    __local uchar *pSrc1 = local_src + (y + 1) * local_src_pitch + idx;

    uchar64 x10 = vload64(0, pSrc1);
    xtail = vload64(0, pSrc1 + 64);
    uchar64 x11 = shuffle2(x10, xtail, maskshift1);
    uchar64 x12 = shuffle2(x10, xtail, maskshift2);
    uchar64 x13 = shuffle2(x10, xtail, maskshift3);
    uchar64 x14 = shuffle2(x10, xtail, maskshift4);

    __local uchar *pSrc2 = local_src + (y + 2) * local_src_pitch + idx;
    uchar64 x20 = vload64(0, pSrc2);
    xtail = vload64(0, pSrc2 + 64);
    uchar64 x21 = shuffle2(x20, xtail, maskshift1);
    uchar64 x22 = shuffle2(x20, xtail, maskshift2);
    uchar64 x23 = shuffle2(x20, xtail, maskshift3);
    uchar64 x24 = shuffle2(x20, xtail, maskshift4);

    __local uchar *pSrc3 = local_src + (y + 3) * local_src_pitch + idx;
    uchar64 x30 = vload64(0, pSrc3);
    xtail = vload64(0, pSrc3 + 64);
    uchar64 x31 = shuffle2(x30, xtail, maskshift1);
    uchar64 x32 = shuffle2(x30, xtail, maskshift2);
    uchar64 x33 = shuffle2(x30, xtail, maskshift3);
    uchar64 x34 = shuffle2(x30, xtail, maskshift4);
    
    for (int y = 0; y < num_rows; ++y) {
      __local uchar *pSrc4 = local_src + (y + 4) * local_src_pitch + idx;
      uchar64 x40 = vload64(0, pSrc4);
      uchar64 xtail = vload64(0, pSrc4 + 64);
      uchar64 x41 = shuffle2(x40, xtail, maskshift1);
      uchar64 x42 = shuffle2(x40, xtail, maskshift2);
      uchar64 x43 = shuffle2(x40, xtail, maskshift3);
      uchar64 x44 = shuffle2(x40, xtail, maskshift4);

      int64 result = xt_mul24(x00, Coef[0]);
      result = xt_mad24(x01, Coef[1], result);
      result = xt_mad24(x02, Coef[2], result);
      result = xt_mad24(x03, Coef[3], result);
      result = xt_mad24(x04, Coef[4], result);
     
      result = xt_mad24(x10, Coef[5], result);
      result = xt_mad24(x11, Coef[6], result);
      result = xt_mad24(x12, Coef[7], result);
      result = xt_mad24(x13, Coef[8], result);
      result = xt_mad24(x14, Coef[9], result);
     
      result = xt_mad24(x20, Coef[10], result);
      result = xt_mad24(x21, Coef[11], result);
      result = xt_mad24(x22, Coef[12], result);
      result = xt_mad24(x23, Coef[13], result);
      result = xt_mad24(x24, Coef[14], result);
     
      result = xt_mad24(x30, Coef[15], result);
      result = xt_mad24(x31, Coef[16], result);
      result = xt_mad24(x32, Coef[17], result);
      result = xt_mad24(x33, Coef[18], result);
      result = xt_mad24(x34, Coef[19], result);
      
      result = xt_mad24(x40, Coef[20], result);
      result = xt_mad24(x41, Coef[21], result);
      result = xt_mad24(x42, Coef[22], result);
      result = xt_mad24(x43, Coef[23], result);
      result = xt_mad24(x44, Coef[24], result);
      char64 result_ch = xt_convert24_char64(result, 8);

      *(__local char64 *)(local_dst + y * outTileW + idx) = result_ch;

      x00 = x10;
      x01 = x11;
      x02 = x12;
      x03 = x13;
      x04 = x14;
      x10 = x20;
      x11 = x21;
      x12 = x22;
      x13 = x23;
      x14 = x24;
      x20 = x30;
      x21 = x31;
      x22 = x32;
      x23 = x33;
      x24 = x34;
      x30 = x40;
      x31 = x41;
      x32 = x42;
      x33 = x43;
      x34 = x44;
    }
  }
