  __constant char* Coef = (__constant char*)filtCoeff;
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
  for (idx = 0; idx < row_size; idx += 128) {
    int y = 0;
    __local uchar *pSrc0 = local_src + y * local_src_pitch + idx;
    uchar128 x00 = vload128(0, pSrc0);
    uchar128 xtail = vload128(0, pSrc0 + 128);
    uchar128 x01 = shuffle2(x00, xtail, maskshift1);
    uchar128 x02 = shuffle2(x00, xtail, maskshift2);
    uchar128 x03 = shuffle2(x00, xtail, maskshift3);
    uchar128 x04 = shuffle2(x00, xtail, maskshift4);

    __local uchar *pSrc1 = local_src + (y + 1) * local_src_pitch + idx;

    uchar128 x10 = vload128(0, pSrc1);
    xtail = vload128(0, pSrc1 + 128);
    uchar128 x11 = shuffle2(x10, xtail, maskshift1);
    uchar128 x12 = shuffle2(x10, xtail, maskshift2);
    uchar128 x13 = shuffle2(x10, xtail, maskshift3);
    uchar128 x14 = shuffle2(x10, xtail, maskshift4);

    __local uchar *pSrc2 = local_src + (y + 2) * local_src_pitch + idx;
    uchar128 x20 = vload128(0, pSrc2);
    xtail = vload128(0, pSrc2 + 128);
    uchar128 x21 = shuffle2(x20, xtail, maskshift1);
    uchar128 x22 = shuffle2(x20, xtail, maskshift2);
    uchar128 x23 = shuffle2(x20, xtail, maskshift3);
    uchar128 x24 = shuffle2(x20, xtail, maskshift4);

    __local uchar *pSrc3 = local_src + (y + 3) * local_src_pitch + idx;
    uchar128 x30 = vload128(0, pSrc3);
    xtail = vload128(0, pSrc3 + 128);
    uchar128 x31 = shuffle2(x30, xtail, maskshift1);
    uchar128 x32 = shuffle2(x30, xtail, maskshift2);
    uchar128 x33 = shuffle2(x30, xtail, maskshift3);
    uchar128 x34 = shuffle2(x30, xtail, maskshift4);
    
    for (int y = 0; y < num_rows; ++y) {
      __local uchar *pSrc4 = local_src + (y + 4) * local_src_pitch + idx;
      uchar128 x40 = vload128(0, pSrc4);
      uchar128 xtail = vload128(0, pSrc4 + 128);
      uchar128 x41 = shuffle2(x40, xtail, maskshift1);
      uchar128 x42 = shuffle2(x40, xtail, maskshift2);
      uchar128 x43 = shuffle2(x40, xtail, maskshift3);
      uchar128 x44 = shuffle2(x40, xtail, maskshift4);

      int128 result = xt_mul32(x00, Coef[0]);
      result = xt_mad32(x01, Coef[1], result);
      result = xt_mad32(x02, Coef[2], result);
      result = xt_mad32(x03, Coef[3], result);
      result = xt_mad32(x04, Coef[4], result);
     
      result = xt_mad32(x10, Coef[5], result);
      result = xt_mad32(x11, Coef[6], result);
      result = xt_mad32(x12, Coef[7], result);
      result = xt_mad32(x13, Coef[8], result);
      result = xt_mad32(x14, Coef[9], result);
     
      result = xt_mad32(x20, Coef[10], result);
      result = xt_mad32(x21, Coef[11], result);
      result = xt_mad32(x22, Coef[12], result);
      result = xt_mad32(x23, Coef[13], result);
      result = xt_mad32(x24, Coef[14], result);
     
      result = xt_mad32(x30, Coef[15], result);
      result = xt_mad32(x31, Coef[16], result);
      result = xt_mad32(x32, Coef[17], result);
      result = xt_mad32(x33, Coef[18], result);
      result = xt_mad32(x34, Coef[19], result);
      
      result = xt_mad32(x40, Coef[20], result);
      result = xt_mad32(x41, Coef[21], result);
      result = xt_mad32(x42, Coef[22], result);
      result = xt_mad32(x43, Coef[23], result);
      result = xt_mad32(x44, Coef[24], result);
      char128 result_ch = xt_convert_char128(result, 8);

      *(__local char128 *)(local_dst + y * outTileW + idx) = result_ch;

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
