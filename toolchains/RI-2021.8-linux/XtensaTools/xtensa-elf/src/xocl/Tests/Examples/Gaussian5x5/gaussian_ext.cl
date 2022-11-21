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

    int64 row0 = xt_mul24(x00, (char64)1);
    row0 = xt_mad24(x01, (char64)4, row0);
    row0 = xt_mad24(x02, (char64)6, row0);
    row0 = xt_mad24(x03, (char64)4, row0);
    row0 = xt_mad24(x04, (char64)1, row0);
    short64 row0_s = xt_convert24_short64(row0);

    int64 row1 = xt_mul24(x10, (char64)1);
    row1 = xt_mad24(x11, (char64)4, row1);
    row1 = xt_mad24(x12, (char64)6, row1);
    row1 = xt_mad24(x13, (char64)4, row1);
    row1 = xt_mad24(x14, (char64)1, row1);
    short64 row1_s = xt_convert24_short64(row1);

    int64 row2 = xt_mul24(x20, (char64)1);
    row2 = xt_mad24(x21, (char64)4, row2);
    row2 = xt_mad24(x22, (char64)6, row2);
    row2 = xt_mad24(x23, (char64)4, row2);
    row2 = xt_mad24(x24, (char64)1, row2);
    short64 row2_s = xt_convert24_short64(row2);

    int64 row3 = xt_mul24(x30, (char64)1);
    row3 = xt_mad24(x31, (char64)4, row3);
    row3 = xt_mad24(x32, (char64)6, row3);
    row3 = xt_mad24(x33, (char64)4, row3);
    row3 = xt_mad24(x34, (char64)1, row3);
    short64 row3_s = xt_convert24_short64(row3);
    
    for (int y = 0; y < num_rows; ++y) {
      __local uchar *pSrc4 = local_src + (y + 4) * local_src_pitch + idx;
      uchar64 x40 = vload64(0, pSrc4);
      uchar64 x41 = vload64(0, pSrc4+1);
      uchar64 x42 = vload64(0, pSrc4+2);
      uchar64 x43 = vload64(0, pSrc4+3);
      uchar64 x44 = vload64(0, pSrc4+4);

      int64 row4 = xt_mul24(x40, (char)1);
      row4 = xt_mad24(x41, (char)4, row4);
      row4 = xt_mad24(x42, (char)6, row4);
      row4 = xt_mad24(x43, (char)4, row4);
      row4 = xt_add24(x44, (uchar64)0, row4);
      short64 row4_s = xt_convert24_short64(row4);

#if XT_OCL_VQ7
      int64 result = xt_mul32(row0_s+row4_s, (char)1);
      result = xt_mad32(row1_s, (char)4, result);
      result = xt_mad32(row3_s, (char)4, result);
      result = xt_mad32(row2_s, (char)6, result);
      *((__local uchar64 *)(local_dst + y * outTileW + idx)) =
          convert_uchar64(xt_convert_short64_sat_rnd(result, 8));
#else
      int64 result = xt_mul24(row0_s+row4_s, (char64)1);
      result = xt_mad24(row1_s+row3_s, (char64)4, result);
      result = xt_mad24(row2_s, (char64)6, result);
      *((__local uchar64 *)(local_dst + y * outTileW + idx)) =
          xt_convert24_uchar64_sat_rnd(result, 8);
#endif

      row0_s = row1_s;
      row1_s = row2_s;
      row2_s = row3_s;
      row3_s = row4_s;
    }
  }
