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

  // kernel using standard OpenCL C
  for (idx = 0; idx < row_size; idx += 64) {
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

    int64 row0 = xt_add24(x00, x01);
    row0 = xt_add24(x02, 0, row0);
    short64 row0_s = xt_convert24_short64(row0);

    int64 row1 = xt_add24(x10, x11);
    row1 = xt_add24(x12, 0, row1);
    short64 row1_s = xt_convert24_short64(row1);

    short64 Sum = row0_s + row1_s;
    
    for (int y = 0; y < num_rows; ++y) {
      __local uchar *pSrc2 = local_src + (y + 2) * local_src_pitch + idx;
      uchar64 x20 = vload64(0, pSrc2);
      xtail = vload64(0, pSrc2 + 64);
      uchar64 x21 = shuffle2(x20, xtail, maskshift1);
      uchar64 x22 = shuffle2(x20, xtail, maskshift2);

      int64 row2 = xt_add24(x20, x21);
      row2 = xt_add24(x22, 0, row2);
      short64 row2_s = xt_convert24_short64(row2);
   
      Sum += row2_s;

      int64 result = xt_mul32(Sum, (short64)Q15_1div9);
      result =  xt_add32(0, (short64)(1 << 14), result);
      short64 res = xt_convert_short64(result, 15);
      uchar64 xout = convert_uchar64(res);
    
      vstore64(xout, 0, (__local uchar *)(local_dst + y * outTileW + idx));

      Sum -= row0_s;
      row0_s = row1_s;
      row1_s = row2_s;
    }
  }
