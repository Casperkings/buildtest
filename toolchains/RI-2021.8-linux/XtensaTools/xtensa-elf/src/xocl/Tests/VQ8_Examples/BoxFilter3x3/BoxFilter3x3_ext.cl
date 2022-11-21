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

  // kernel using standard OpenCL C
  for (idx = 0; idx < row_size; idx += 128) {
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

    int128 row0 = xt_add32(x00, x01);
    row0 = xt_add32(x02, 0, row0);
    short128 row0_s = convert_short128(row0);

    int128 row1 = xt_add32(x10, x11);
    row1 = xt_add32(x12, 0, row1);
    short128 row1_s = convert_short128(row1);

    short128 Sum = row0_s + row1_s;
    
    for (int y = 0; y < num_rows; ++y) {
      __local uchar *pSrc2 = local_src + (y + 2) * local_src_pitch + idx;
      uchar128 x20 = vload128(0, pSrc2);
      xtail = vload128(0, pSrc2 + 128);
      uchar128 x21 = shuffle2(x20, xtail, maskshift1);
      uchar128 x22 = shuffle2(x20, xtail, maskshift2);

      int128 row2 = xt_add32(x20, x21);
      row2 = xt_add32(x22, 0, row2);
      short128 row2_s = convert_short128(row2);
   
      Sum += row2_s;

      int128 result = xt_mul32(Sum, (short128)Q15_1div9);
      result = xt_add32(0, (short128)(1 << 14), result);
      short128 res = xt_convert_short128(result, 15);
      uchar128 xout = convert_uchar128(res);
    
      vstore128(xout, 0, (__local uchar *)(local_dst + y * outTileW + idx));

      Sum -= row0_s;
      row0_s = row1_s;
      row1_s = row2_s;
    }
  }
