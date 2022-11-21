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

    int128 row0 = xt_mul32(x00, (char128)1);
    row0 = xt_mad32(x01, (char128)4, row0);
    row0 = xt_mad32(x02, (char128)6, row0);
    row0 = xt_mad32(x03, (char128)4, row0);
    row0 = xt_mad32(x04, (char128)1, row0);
    short128 row0_s = convert_short128(row0);

    int128 row1 = xt_mul32(x10, (char128)1);
    row1 = xt_mad32(x11, (char128)4, row1);
    row1 = xt_mad32(x12, (char128)6, row1);
    row1 = xt_mad32(x13, (char128)4, row1);
    row1 = xt_mad32(x14, (char128)1, row1);
    short128 row1_s = convert_short128(row1);

    int128 row2 = xt_mul32(x20, (char128)1);
    row2 = xt_mad32(x21, (char128)4, row2);
    row2 = xt_mad32(x22, (char128)6, row2);
    row2 = xt_mad32(x23, (char128)4, row2);
    row2 = xt_mad32(x24, (char128)1, row2);
    short128 row2_s = convert_short128(row2);

    int128 row3 = xt_mul32(x30, (char128)1);
    row3 = xt_mad32(x31, (char128)4, row3);
    row3 = xt_mad32(x32, (char128)6, row3);
    row3 = xt_mad32(x33, (char128)4, row3);
    row3 = xt_mad32(x34, (char128)1, row3);
    short128 row3_s = convert_short128(row3);
    
    for (int y = 0; y < num_rows; ++y) {
      __local uchar *pSrc4 = local_src + (y + 4) * local_src_pitch + idx;
      uchar128 x40 = vload128(0, pSrc4);
      uchar128 x41 = vload128(0, pSrc4+1);
      uchar128 x42 = vload128(0, pSrc4+2);
      uchar128 x43 = vload128(0, pSrc4+3);
      uchar128 x44 = vload128(0, pSrc4+4);

      int128 row4 = xt_mul32(x40, (char)1);
      row4 = xt_mad32(x41, (char)4, row4);
      row4 = xt_mad32(x42, (char)6, row4);
      row4 = xt_mad32(x43, (char)4, row4);
      row4 = xt_add32(x44, (uchar128)0, row4);
      short128 row4_s = convert_short128(row4);

#if 0
      int128 result = xt_mul32(row0_s+row4_s, (char)1);
      result = xt_mad32(row1_s, (char)4, result);
      result = xt_mad32(row3_s, (char)4, result);
      result = xt_mad32(row2_s, (char)6, result);
      *((__local uchar128 *)(local_dst + y * outTileW + idx)) =
         convert_uchar128(xt_convert_short128_sat_rnd(result, 8));
         // xt_convert_uchar128_sat_rnd(result, 8);
#else
      int128 result = xt_mul32(row0_s+row4_s, (char128)1);
      result = xt_mad32(row1_s+row3_s, (char128)4, result);
      result = xt_mad32(row2_s, (char128)6, result);
      *((__local uchar128 *)(local_dst + y * outTileW + idx)) =
          xt_convert_uchar128_sat_rnd(result, 8);
#endif

      row0_s = row1_s;
      row1_s = row2_s;
      row2_s = row3_s;
      row3_s = row4_s;
    }
  }
