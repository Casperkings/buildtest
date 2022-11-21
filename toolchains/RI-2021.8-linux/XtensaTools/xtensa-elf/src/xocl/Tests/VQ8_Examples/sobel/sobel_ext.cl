  // sobel kernel using standard OpenCL C
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

  int128 subX0_w, subX1_w, subX2_w, subX3_w, subX4_w;
  int128 subY0_w, subY1_w, subY2_w, subY3_w, subY4_w;
  short128 subX0, subX1, subX2, subX3, subX4;
  short128 subY0, subY1, subY2, subY3, subY4;
  int128 resX, resX0, resX1, resX2, resX3, resX4;
  int128 resY;

  for (idx = 0; idx < row_size; idx += 128) {
    int y = 0;
    // row 0
    __local uchar *pSrc0 = local_src + y * local_src_pitch + idx;
    uchar128 x00 = vload128(0, pSrc0);
    uchar128 xtail = vload128(0, pSrc0 + 128);
    uchar128 x01 = shuffle2(x00, xtail, maskshift1); 
    uchar128 x02 = shuffle2(x00, xtail, maskshift2);
    uchar128 x03 = shuffle2(x00, xtail, maskshift3);
    uchar128 x04 = shuffle2(x00, xtail, maskshift4);

    subX0_w = xt_mul32(x00, (char)-1);
    subX0_w = xt_mad32(x01, (char)-2, subX0_w);
    subX0_w = xt_mad32(x02, (char)0, subX0_w);
    subX0_w = xt_mad32(x03, (char)2, subX0_w);
    subX0_w = xt_add32(x04, (uchar128)0, subX0_w);
    subX0 = convert_short128(subX0_w);

    subY0_w = xt_mul32(x00, (char)1);
    subY0_w = xt_mad32(x01, (char)4, subY0_w);
    subY0_w = xt_mad32(x02, (char)6, subY0_w);
    subY0_w = xt_mad32(x03, (char)4, subY0_w);
    subY0_w = xt_add32(x04, (uchar128)0, subY0_w);
    subY0 = convert_short128(subY0_w);

    // row 1
    __local uchar *pSrc1 = local_src + (y + 1) * local_src_pitch + idx;
    uchar128 x10 = vload128(0, pSrc1);
    xtail = vload128(0, pSrc1 + 128);
    uchar128 x11 = shuffle2(x10, xtail, maskshift1);
    uchar128 x12 = shuffle2(x10, xtail, maskshift2);
    uchar128 x13 = shuffle2(x10, xtail, maskshift3);
    uchar128 x14 = shuffle2(x10, xtail, maskshift4);

    subX1_w = xt_mul32(x10, (char)-1);
    subX1_w = xt_mad32(x11, (char)-2, subX1_w);
    subX1_w = xt_mad32(x12, (char)0, subX1_w);
    subX1_w = xt_mad32(x13, (char)2, subX1_w);
    subX1_w = xt_add32(x14, (uchar128)0, subX1_w);
    subX1 = convert_short128(subX1_w);

    subY1_w = xt_mul32(x10, (char)1);
    subY1_w = xt_mad32(x11, (char)4, subY1_w);
    subY1_w = xt_mad32(x12, (char)6, subY1_w);
    subY1_w = xt_mad32(x13, (char)4, subY1_w);
    subY1_w = xt_add32(x14, (uchar128)0, subY1_w);
    subY1 = convert_short128(subY1_w);

    // row 2
    __local uchar *pSrc2 = local_src + (y + 2) * local_src_pitch + idx;
    uchar128 x20 = vload128(0, pSrc2);
    xtail = vload128(0, pSrc2 + 128);
    uchar128 x21 = shuffle2(x20, xtail, maskshift1);
    uchar128 x22 = shuffle2(x20, xtail, maskshift2);
    uchar128 x23 = shuffle2(x20, xtail, maskshift3);
    uchar128 x24 = shuffle2(x20, xtail, maskshift4);

    subX2_w = xt_mul32(x20, (char)-1);
    subX2_w = xt_mad32(x21, (char)-2, subX2_w);
    subX2_w = xt_mad32(x22, (char)0, subX2_w);
    subX2_w = xt_mad32(x23, (char)2, subX2_w);
    subX2_w = xt_add32(x24, (uchar128)0, subX2_w);
    subX2 = convert_short128(subX2_w);

    subY2_w = xt_mul32(x20, (char)1);
    subY2_w = xt_mad32(x21, (char)4, subY2_w);
    subY2_w = xt_mad32(x22, (char)6, subY2_w);
    subY2_w = xt_mad32(x23, (char)4, subY2_w);
    subY2_w = xt_add32(x24, (uchar128)0, subY2_w);
    subY2 = convert_short128(subY2_w);

    // row 3
    __local uchar *pSrc3 = local_src + (y + 3) * local_src_pitch + idx;
    uchar128 x30 = vload128(0, pSrc3);
    xtail = vload128(0, pSrc3 + 128);
    uchar128 x31 = shuffle2(x30, xtail, maskshift1);
    uchar128 x32 = shuffle2(x30, xtail, maskshift2);
    uchar128 x33 = shuffle2(x30, xtail, maskshift3);
    uchar128 x34 = shuffle2(x30, xtail, maskshift4);

    subX3_w = xt_mul32(x30, (char)-1);
    subX3_w = xt_mad32(x31, (char)-2, subX3_w);
    subX3_w = xt_mad32(x32, (char)0, subX3_w);
    subX3_w = xt_mad32(x33, (char)2, subX3_w);
    subX3_w = xt_add32(x34, (uchar128)0, subX3_w);
    subX3 = convert_short128(subX3_w);

    subY3_w = xt_mul32(x30, (char)1);
    subY3_w = xt_mad32(x31, (char)4, subY3_w);
    subY3_w = xt_mad32(x32, (char)6, subY3_w);
    subY3_w = xt_mad32(x33, (char)4, subY3_w);
    subY3_w = xt_add32(x34, (uchar128)0, subY3_w);
    subY3 = convert_short128(subY3_w);

    for (int y = 0; y < num_rows; ++y) {
      __local uchar *pSrc4 = local_src + (y + 4) * local_src_pitch + idx;
      uchar128 x40 = vload128(0, pSrc4);
      xtail = vload128(0, pSrc4 + 128);

      uchar128 x41 = shuffle2(x40, xtail, maskshift1);
      uchar128 x42 = shuffle2(x40, xtail, maskshift2);
      uchar128 x43 = shuffle2(x40, xtail, maskshift3);
      uchar128 x44 = shuffle2(x40, xtail, maskshift4);

      subX4_w = xt_mul32(x40, (char)-1);
      subX4_w = xt_mad32(x41, (char)-2, subX4_w);
      subX4_w = xt_mad32(x42, (char)0, subX4_w);
      subX4_w = xt_mad32(x43, (char)2, subX4_w);
      subX4_w = xt_add32(x44, (uchar128)0, subX4_w);
      subX4 = convert_short128(subX4_w);

      subY4_w = xt_mul32(x40, (char)1);
      subY4_w = xt_mad32(x41, (char)4, subY4_w);
      subY4_w = xt_mad32(x42, (char)6, subY4_w);
      subY4_w = xt_mad32(x43, (char)4, subY4_w);
      subY4_w = xt_add32(x44, (uchar128)0, subY4_w);
      subY4 = convert_short128(subY4_w);

      short128 resY = subY4 - (subY0 - ((subY3 - subY1) << 1));

      int128 _resX = xt_mul32(subX0, (char128)1);
      _resX = xt_mad32(subX1, (char128)4, _resX);
      _resX = xt_mad32(subX2, (char128)6, _resX);
      _resX = xt_mad32(subX3, (char128)4, _resX);
      short128 resX = convert_short128(_resX) + subX4;

      *((__local short128 *)(local_dstx + y * outTileW + idx)) = resX;
      *((__local short128 *)(local_dsty + y * outTileW + idx)) = resY;

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
