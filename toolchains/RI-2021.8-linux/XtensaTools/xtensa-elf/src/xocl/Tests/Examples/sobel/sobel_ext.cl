  // sobel kernel using standard OpenCL C
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

  int64 subX0_w, subX1_w, subX2_w, subX3_w, subX4_w;
  int64 subY0_w, subY1_w, subY2_w, subY3_w, subY4_w;
  short64 subX0, subX1, subX2, subX3, subX4;
  short64 subY0, subY1, subY2, subY3, subY4;
  int64 resX, resX0, resX1, resX2, resX3, resX4;
  int64 resY;

  for (idx = 0; idx < row_size; idx += 64) {
    int y = 0;
    // row 0
    __local uchar *pSrc0 = local_src + y * local_src_pitch + idx;
    uchar64 x00 = vload64(0, pSrc0);
    uchar64 xtail = vload64(0, pSrc0 + 64);
    uchar64 x01 = shuffle2(x00, xtail, maskshift1); 
    uchar64 x02 = shuffle2(x00, xtail, maskshift2);
    uchar64 x03 = shuffle2(x00, xtail, maskshift3);
    uchar64 x04 = shuffle2(x00, xtail, maskshift4);

    subX0_w = xt_mul24(x00, (char)-1);
    subX0_w = xt_mad24(x01, (char)-2, subX0_w);
    subX0_w = xt_mad24(x02, (char)0, subX0_w);
    subX0_w = xt_mad24(x03, (char)2, subX0_w);
    subX0_w = xt_add24(x04, (uchar64)0, subX0_w);
    subX0 = xt_convert24_short64(subX0_w);

    subY0_w = xt_mul24(x00, (char)1);
    subY0_w = xt_mad24(x01, (char)4, subY0_w);
    subY0_w = xt_mad24(x02, (char)6, subY0_w);
    subY0_w = xt_mad24(x03, (char)4, subY0_w);
    subY0_w = xt_add24(x04, (uchar64)0, subY0_w);
    subY0 = xt_convert24_short64(subY0_w);

    // row 1
    __local uchar *pSrc1 = local_src + (y + 1) * local_src_pitch + idx;
    uchar64 x10 = vload64(0, pSrc1);
    xtail = vload64(0, pSrc1 + 64);
    uchar64 x11 = shuffle2(x10, xtail, maskshift1);
    uchar64 x12 = shuffle2(x10, xtail, maskshift2);
    uchar64 x13 = shuffle2(x10, xtail, maskshift3);
    uchar64 x14 = shuffle2(x10, xtail, maskshift4);

    subX1_w = xt_mul24(x10, (char)-1);
    subX1_w = xt_mad24(x11, (char)-2, subX1_w);
    subX1_w = xt_mad24(x12, (char)0, subX1_w);
    subX1_w = xt_mad24(x13, (char)2, subX1_w);
    subX1_w = xt_add24(x14, (uchar64)0, subX1_w);
    subX1 = xt_convert24_short64(subX1_w);

    subY1_w = xt_mul24(x10, (char)1);
    subY1_w = xt_mad24(x11, (char)4, subY1_w);
    subY1_w = xt_mad24(x12, (char)6, subY1_w);
    subY1_w = xt_mad24(x13, (char)4, subY1_w);
    subY1_w = xt_add24(x14, (uchar64)0, subY1_w);
    subY1 = xt_convert24_short64(subY1_w);

    // row 2
    __local uchar *pSrc2 = local_src + (y + 2) * local_src_pitch + idx;
    uchar64 x20 = vload64(0, pSrc2);
    xtail = vload64(0, pSrc2 + 64);
    uchar64 x21 = shuffle2(x20, xtail, maskshift1);
    uchar64 x22 = shuffle2(x20, xtail, maskshift2);
    uchar64 x23 = shuffle2(x20, xtail, maskshift3);
    uchar64 x24 = shuffle2(x20, xtail, maskshift4);

    subX2_w = xt_mul24(x20, (char)-1);
    subX2_w = xt_mad24(x21, (char)-2, subX2_w);
    subX2_w = xt_mad24(x22, (char)0, subX2_w);
    subX2_w = xt_mad24(x23, (char)2, subX2_w);
    subX2_w = xt_add24(x24, (uchar64)0, subX2_w);
    subX2 = xt_convert24_short64(subX2_w);

    subY2_w = xt_mul24(x20, (char)1);
    subY2_w = xt_mad24(x21, (char)4, subY2_w);
    subY2_w = xt_mad24(x22, (char)6, subY2_w);
    subY2_w = xt_mad24(x23, (char)4, subY2_w);
    subY2_w = xt_add24(x24, (uchar64)0, subY2_w);
    subY2 = xt_convert24_short64(subY2_w);

    // row 3
    __local uchar *pSrc3 = local_src + (y + 3) * local_src_pitch + idx;
    uchar64 x30 = vload64(0, pSrc3);
    xtail = vload64(0, pSrc3 + 64);
    uchar64 x31 = shuffle2(x30, xtail, maskshift1);
    uchar64 x32 = shuffle2(x30, xtail, maskshift2);
    uchar64 x33 = shuffle2(x30, xtail, maskshift3);
    uchar64 x34 = shuffle2(x30, xtail, maskshift4);

    subX3_w = xt_mul24(x30, (char)-1);
    subX3_w = xt_mad24(x31, (char)-2, subX3_w);
    subX3_w = xt_mad24(x32, (char)0, subX3_w);
    subX3_w = xt_mad24(x33, (char)2, subX3_w);
    subX3_w = xt_add24(x34, (uchar64)0, subX3_w);
    subX3 = xt_convert24_short64(subX3_w);

    subY3_w = xt_mul24(x30, (char)1);
    subY3_w = xt_mad24(x31, (char)4, subY3_w);
    subY3_w = xt_mad24(x32, (char)6, subY3_w);
    subY3_w = xt_mad24(x33, (char)4, subY3_w);
    subY3_w = xt_add24(x34, (uchar64)0, subY3_w);
    subY3 = xt_convert24_short64(subY3_w);

    for (int y = 0; y < num_rows; ++y) {
      __local uchar *pSrc4 = local_src + (y + 4) * local_src_pitch + idx;
      uchar64 x40 = vload64(0, pSrc4);
      xtail = vload64(0, pSrc4 + 64);

      uchar64 x41 = shuffle2(x40, xtail, maskshift1);
      uchar64 x42 = shuffle2(x40, xtail, maskshift2);
      uchar64 x43 = shuffle2(x40, xtail, maskshift3);
      uchar64 x44 = shuffle2(x40, xtail, maskshift4);

      subX4_w = xt_mul24(x40, (char)-1);
      subX4_w = xt_mad24(x41, (char)-2, subX4_w);
      subX4_w = xt_mad24(x42, (char)0, subX4_w);
      subX4_w = xt_mad24(x43, (char)2, subX4_w);
      subX4_w = xt_add24(x44, (uchar64)0, subX4_w);
      subX4 = xt_convert24_short64(subX4_w);

      subY4_w = xt_mul24(x40, (char)1);
      subY4_w = xt_mad24(x41, (char)4, subY4_w);
      subY4_w = xt_mad24(x42, (char)6, subY4_w);
      subY4_w = xt_mad24(x43, (char)4, subY4_w);
      subY4_w = xt_add24(x44, (uchar64)0, subY4_w);
      subY4 = xt_convert24_short64(subY4_w);

      short64 resY = subY4 - (subY0 - ((subY3 - subY1) << 1));

      int64 _resX = xt_mul24(subX0, (char64)1);
      _resX = xt_mad24(subX1, (char64)4, _resX);
      _resX = xt_mad24(subX2, (char64)6, _resX);
      _resX = xt_mad24(subX3, (char64)4, _resX);
      short64 resX = xt_convert24_short64(_resX) + subX4;

      *((__local short64 *)(local_dstx + y * outTileW + idx)) = resX;
      *((__local short64 *)(local_dsty + y * outTileW + idx)) = resY;

      subX0 = subX1;
      subX1 = subX2;
      subX2 = subX3;
      subX3 = subX4;

      subY0 = subY1;
      subY1 = subY2;
      subY2 = subY3;
      subY3 = subY4;
    }
  }
