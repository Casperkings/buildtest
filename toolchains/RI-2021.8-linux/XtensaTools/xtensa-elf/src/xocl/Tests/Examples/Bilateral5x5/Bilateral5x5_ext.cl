const uchar64 maskshift1 = {1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13,
                            14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
                            27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
                            40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52,
                            53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64};
const uchar64 maskshift2 = {2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14,
                            15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27,
                            28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
                            41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53,
                            54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65};
const uchar64 maskshift3 = {3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,
                            16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28,
                            29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41,
                            42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54,
                            55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66};
const uchar64 maskshift4 = {4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                            17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
                            30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42,
                            43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55,
                            56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67};

uchar64 Coef_A = vload64(0, Coef);
uchar64 Coef_B = vload64(0, Coef + 64);
uchar64 Coef_C = vload64(0, Coef + 128);
uchar64 Coef_D = vload64(0, Coef + 256);
uchar64 Coef_E = vload64(0, Coef + 320);
uchar coeff8 = Coef[8 * 64];

for (idx = 0; idx < inTileW; idx += 64) {
  __local uchar *pDst = local_dst + idx;
  ushort *tempDiv1 = tempDiv;
  for (int y = 0; y < inTileH; ++y) {
    int64 Filt;
    int64 wtSum;

    __local uchar *pSrc2 = local_src + (y + 2) * local_src_pitch + idx;
    uchar64 x20 = vload64(0, pSrc2);
    uchar64 xtail = vload64(1, pSrc2);
    uchar64 x22 = shuffle2(x20, xtail, maskshift2);

    __local uchar *pSrc = local_src + (y)*local_src_pitch + idx;
    __constant uchar64 *pCoef = (__constant uchar64 *)Coef;

    uchar64 x00 = vload64(0, pSrc);
    uchar64 xtail0 = vload64(1, pSrc);
    pSrc += local_src_pitch;
    uchar64 x0 = x00;
    uchar64 x1 = shuffle2(x00, xtail0, maskshift1);
    uchar64 x2 = shuffle2(x00, xtail0, maskshift2);
    uchar64 x3 = shuffle2(x00, xtail0, maskshift3);
    uchar64 x4 = shuffle2(x00, xtail0, maskshift4);

    uchar64 diff = abs_diff(x0, x22);
    diff = diff >> LUT_SHIFT;
    uchar64 coef = shuffle(Coef_A, diff);
    Filt = xt_mul24(x0, coef);

    diff = abs_diff(x1, x22);
    diff = diff >> LUT_SHIFT;
    uchar64 coef1 = shuffle(Coef_B, diff);
    Filt = xt_mad24(x1, coef1, Filt);
    wtSum = xt_add24(coef, coef1);

    diff = abs_diff(x2, x22);
    diff = diff >> LUT_SHIFT;
    coef = shuffle(Coef_C, diff);
    Filt = xt_mad24(x2, coef, Filt);

    diff = abs_diff(x3, x22);
    diff = diff >> LUT_SHIFT;
    coef1 = shuffle(Coef_B, diff);
    Filt = xt_mad24(x3, coef1, Filt);
    wtSum = xt_add24(coef, coef1, wtSum);

    diff = abs_diff(x4, x22);
    diff = diff >> LUT_SHIFT;
    coef = shuffle(Coef_A, diff);
    Filt = xt_mad24(x4, coef, Filt);

    pCoef += 5;

    x00 = vload64(0, pSrc);
    xtail0 = vload64(1, pSrc);
    pSrc += local_src_pitch;
    x0 = x00;
    x1 = shuffle2(x00, xtail0, maskshift1);
    x2 = shuffle2(x00, xtail0, maskshift2);
    x3 = shuffle2(x00, xtail0, maskshift3);
    x4 = shuffle2(x00, xtail0, maskshift4);

    diff = abs_diff(x0, x22);
    diff = diff >> LUT_SHIFT;
    coef1 = shuffle(Coef_B, diff);
    Filt = xt_mad24(x0, coef1, Filt);

    wtSum = xt_add24(coef, coef1, wtSum);

    diff = abs_diff(x1, x22);
    diff = diff >> LUT_SHIFT;
    coef1 = shuffle(Coef_D, diff);
    Filt = xt_mad24(x1, coef1, Filt);

    diff = abs_diff(x2, x22);
    diff = diff >> LUT_SHIFT;
    coef = shuffle(Coef_E, diff);
    Filt = xt_mad24(x2, coef, Filt);

    wtSum = xt_add24(coef, coef1, wtSum);

    diff = abs_diff(x3, x22);
    diff = diff >> LUT_SHIFT;
    coef1 = shuffle(Coef_D, diff);
    Filt = xt_mad24(x3, coef1, Filt);

    diff = abs_diff(x4, x22);
    diff = diff >> LUT_SHIFT;
    coef = shuffle(Coef_B, diff);
    Filt = xt_mad24(x4, coef, Filt);
    wtSum = xt_add24(coef, coef1, wtSum);

    pCoef += 5;

    x00 = vload64(0, pSrc);
    xtail0 = vload64(1, pSrc);
    pSrc += local_src_pitch;
    x0 = x00;
    x1 = shuffle2(x00, xtail0, maskshift1);
    x2 = shuffle2(x00, xtail0, maskshift2);
    x3 = shuffle2(x00, xtail0, maskshift3);
    x4 = shuffle2(x00, xtail0, maskshift4);

    diff = abs_diff(x0, x22);
    diff = diff >> LUT_SHIFT;
    coef = shuffle(Coef_C, diff);
    Filt = xt_mad24(x0, coef, Filt);

    diff = abs_diff(x1, x22);
    diff = diff >> LUT_SHIFT;
    coef1 = shuffle(Coef_E, diff);
    Filt = xt_mad24(x1, coef1, Filt);
    wtSum = xt_add24(coef, coef1, wtSum);

    // Coef_A = (pCoef[2]);
    // diff = abs_diff(x2, x22);
    // diff = diff >> LUT_SHIFT;
    // coef1 = shuffle(Coef_A, diff);
    Filt = xt_mad24(x2, (uchar64)coeff8, Filt);
    wtSum = xt_add24((uchar64)0, (uchar64)coeff8, wtSum);

    diff = abs_diff(x3, x22);
    diff = diff >> LUT_SHIFT;
    coef1 = shuffle(Coef_E, diff);
    Filt = xt_mad24(x3, coef1, Filt);

    diff = abs_diff(x4, x22);
    diff = diff >> LUT_SHIFT;
    coef = shuffle(Coef_C, diff);
    Filt = xt_mad24(x4, coef, Filt);

    wtSum = xt_add24(coef, coef1, wtSum);

    ushort64 wtsumst = xt_convert24_ushort64(wtSum);
    vstore64(wtsumst, 0, (ushort *)tempDiv1);
    int32 FltA, FltB;
    FltA = Filt.lo;
    FltB = Filt.hi;
    vstore16(FltA.lo, 0, (int *)(tempDiv1 + 64));
    vstore16(FltA.hi, 0, (int *)(tempDiv1 + 96));
    vstore16(FltB.lo, 0, (int *)(tempDiv1 + 128));
    vstore16(FltB.hi, 0, (int *)(tempDiv1 + 160));
    tempDiv1 += 192;
  }

  tempDiv1 = tempDiv;
  for (int y = 0; y < inTileH; ++y) {
    int64 Filt;  // = (int64)0;
    int64 wtSum; // = (int64)0;

    short64 wtSumA = vload64(0, (short *)tempDiv1);

    wtSum = xt_mul24(wtSumA, (char64)1);

    int32 FiltA, FiltB;
    FiltA.lo = vload16(0, (int *)(tempDiv1 + 64));
    FiltA.hi = vload16(0, (int *)(tempDiv1 + 96));
    FiltB.lo = vload16(0, (int *)(tempDiv1 + 128));
    FiltB.hi = vload16(0, (int *)(tempDiv1 + 160));
    Filt.lo = FiltA;
    Filt.hi = FiltB;
    tempDiv1 += 192;
    __local uchar *pSrc2 = local_src + (y + 2) * local_src_pitch + idx;
    uchar64 x20 = vload64(0, pSrc2);
    uchar64 xtail = vload64(1, pSrc2);
    uchar64 x22 = shuffle2(x20, xtail, maskshift2);

    __local uchar *pSrc = local_src + (y + 3) * local_src_pitch + idx;
    __constant uchar64 *pCoef = (__constant uchar64 *)Coef;
    pCoef += 15;

    uchar64 x00 = vload64(0, pSrc);
    uchar64 xtail0 = vload64(1, pSrc);
    pSrc += local_src_pitch;
    uchar64 x0 = x00;
    uchar64 x1 = shuffle2(x00, xtail0, maskshift1);
    uchar64 x2 = shuffle2(x00, xtail0, maskshift2);
    uchar64 x3 = shuffle2(x00, xtail0, maskshift3);
    uchar64 x4 = shuffle2(x00, xtail0, maskshift4);

    uchar64 diff = abs_diff(x0, x22);
    diff = diff >> LUT_SHIFT;
    uchar64 coef = shuffle(Coef_B, diff);
    Filt = xt_mad24(x0, coef, Filt);

    diff = abs_diff(x1, x22);
    diff = diff >> LUT_SHIFT;
    uchar64 coef1 = shuffle(Coef_D, diff);
    Filt = xt_mad24(x1, coef1, Filt);
    wtSum = xt_add24(coef, coef1, wtSum);

    diff = abs_diff(x2, x22);
    diff = diff >> LUT_SHIFT;
    coef = shuffle(Coef_E, diff);
    Filt = xt_mad24(x2, coef, Filt);

    diff = abs_diff(x3, x22);
    diff = diff >> LUT_SHIFT;
    coef1 = shuffle(Coef_D, diff);
    Filt = xt_mad24(x3, coef1, Filt);
    wtSum = xt_add24(coef, coef1, wtSum);

    diff = abs_diff(x4, x22);
    diff = diff >> LUT_SHIFT;
    coef = shuffle(Coef_B, diff);
    Filt = xt_mad24(x4, coef, Filt);

    pCoef += 5;

    x00 = vload64(0, pSrc);
    xtail0 = vload64(1, pSrc);
    pSrc += local_src_pitch;
    x0 = x00;
    x1 = shuffle2(x00, xtail0, maskshift1);
    x2 = shuffle2(x00, xtail0, maskshift2);
    x3 = shuffle2(x00, xtail0, maskshift3);
    x4 = shuffle2(x00, xtail0, maskshift4);

    diff = abs_diff(x0, x22);
    diff = diff >> LUT_SHIFT;
    coef1 = shuffle(Coef_A, diff);
    Filt = xt_mad24(x0, coef1, Filt);

    wtSum = xt_add24(coef, coef1, wtSum);

    diff = abs_diff(x1, x22);
    diff = diff >> LUT_SHIFT;
    coef1 = shuffle(Coef_B, diff);
    Filt = xt_mad24(x1, coef1, Filt);

    diff = abs_diff(x2, x22);
    diff = diff >> LUT_SHIFT;
    coef = shuffle(Coef_C, diff);
    Filt = xt_mad24(x2, coef, Filt);

    wtSum = xt_add24(coef, coef1, wtSum);

    diff = abs_diff(x3, x22);
    diff = diff >> LUT_SHIFT;
    coef1 = shuffle(Coef_B, diff);
    Filt = xt_mad24(x3, coef1, Filt);

    diff = abs_diff(x4, x22);
    diff = diff >> LUT_SHIFT;
    coef = shuffle(Coef_A, diff);
    Filt = xt_mad24(x4, coef, Filt);
    wtSum = xt_add24(coef, coef1, wtSum);

    int64 Result;
    short64 wtSum16b = xt_convert24_short64(wtSum, 0);
    Result = xt_div(Filt, wtSum16b);
    vstore64(xt_convert24_char64(Result, 0), 0, (__local char *)pDst);
    pDst += local_dst_pitch;
  }
}
