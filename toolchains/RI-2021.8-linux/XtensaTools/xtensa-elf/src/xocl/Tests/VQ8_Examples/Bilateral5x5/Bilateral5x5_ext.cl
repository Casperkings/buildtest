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

uchar128 Coef_A = vload128(0, Coef);
uchar128 Coef_B = vload128(0, Coef + 128);
uchar128 Coef_C = vload128(0, Coef + 256);
uchar128 Coef_D = vload128(0, Coef + 512);
uchar128 Coef_E = vload128(0, Coef + 640);
uchar coeff8 = Coef[8 * 128];

for (idx = 0; idx < inTileW; idx += 128) {
  __local uchar *pDst = local_dst + idx;
  ushort *tempDiv1 = tempDiv;
  for (int y = 0; y < inTileH; ++y) {
    int128 Filt;
    int128 wtSum;

    __local uchar *pSrc2 = local_src + (y + 2) * local_src_pitch + idx;
    uchar128 x20 = vload128(0, pSrc2);
    uchar128 xtail = vload128(1, pSrc2);
    uchar128 x22 = shuffle2(x20, xtail, maskshift2);

    __local uchar *pSrc = local_src + (y)*local_src_pitch + idx;
    __constant uchar128 *pCoef = (__constant uchar128 *)Coef;

    uchar128 x00 = vload128(0, pSrc);
    uchar128 xtail0 = vload128(1, pSrc);
    pSrc += local_src_pitch;
    uchar128 x0 = x00;
    uchar128 x1 = shuffle2(x00, xtail0, maskshift1);
    uchar128 x2 = shuffle2(x00, xtail0, maskshift2);
    uchar128 x3 = shuffle2(x00, xtail0, maskshift3);
    uchar128 x4 = shuffle2(x00, xtail0, maskshift4);

    uchar128 diff = abs_diff(x0, x22);
    diff = diff >> LUT_SHIFT;
    uchar128 coef = shuffle(Coef_A, diff);
    Filt = xt_mul32(x0, coef);

    diff = abs_diff(x1, x22);
    diff = diff >> LUT_SHIFT;
    uchar128 coef1 = shuffle(Coef_B, diff);
    Filt = xt_mad32(x1, coef1, Filt);
    wtSum = xt_add32(coef, coef1);

    diff = abs_diff(x2, x22);
    diff = diff >> LUT_SHIFT;
    coef = shuffle(Coef_C, diff);
    Filt = xt_mad32(x2, coef, Filt);

    diff = abs_diff(x3, x22);
    diff = diff >> LUT_SHIFT;
    coef1 = shuffle(Coef_B, diff);
    Filt = xt_mad32(x3, coef1, Filt);
    wtSum = xt_add32(coef, coef1, wtSum);

    diff = abs_diff(x4, x22);
    diff = diff >> LUT_SHIFT;
    coef = shuffle(Coef_A, diff);
    Filt = xt_mad32(x4, coef, Filt);

    pCoef += 5;

    x00 = vload128(0, pSrc);
    xtail0 = vload128(1, pSrc);
    pSrc += local_src_pitch;
    x0 = x00;
    x1 = shuffle2(x00, xtail0, maskshift1);
    x2 = shuffle2(x00, xtail0, maskshift2);
    x3 = shuffle2(x00, xtail0, maskshift3);
    x4 = shuffle2(x00, xtail0, maskshift4);

    diff = abs_diff(x0, x22);
    diff = diff >> LUT_SHIFT;
    coef1 = shuffle(Coef_B, diff);
    Filt = xt_mad32(x0, coef1, Filt);

    wtSum = xt_add32(coef, coef1, wtSum);

    diff = abs_diff(x1, x22);
    diff = diff >> LUT_SHIFT;
    coef1 = shuffle(Coef_D, diff);
    Filt = xt_mad32(x1, coef1, Filt);

    diff = abs_diff(x2, x22);
    diff = diff >> LUT_SHIFT;
    coef = shuffle(Coef_E, diff);
    Filt = xt_mad32(x2, coef, Filt);

    wtSum = xt_add32(coef, coef1, wtSum);

    diff = abs_diff(x3, x22);
    diff = diff >> LUT_SHIFT;
    coef1 = shuffle(Coef_D, diff);
    Filt = xt_mad32(x3, coef1, Filt);

    diff = abs_diff(x4, x22);
    diff = diff >> LUT_SHIFT;
    coef = shuffle(Coef_B, diff);
    Filt = xt_mad32(x4, coef, Filt);
    wtSum = xt_add32(coef, coef1, wtSum);

    pCoef += 5;

    x00 = vload128(0, pSrc);
    xtail0 = vload128(1, pSrc);
    pSrc += local_src_pitch;
    x0 = x00;
    x1 = shuffle2(x00, xtail0, maskshift1);
    x2 = shuffle2(x00, xtail0, maskshift2);
    x3 = shuffle2(x00, xtail0, maskshift3);
    x4 = shuffle2(x00, xtail0, maskshift4);

    diff = abs_diff(x0, x22);
    diff = diff >> LUT_SHIFT;
    coef = shuffle(Coef_C, diff);
    Filt = xt_mad32(x0, coef, Filt);

    diff = abs_diff(x1, x22);
    diff = diff >> LUT_SHIFT;
    coef1 = shuffle(Coef_E, diff);
    Filt = xt_mad32(x1, coef1, Filt);
    wtSum = xt_add32(coef, coef1, wtSum);

    // Coef_A = (pCoef[2]);
    // diff = abs_diff(x2, x22);
    // diff = diff >> LUT_SHIFT;
    // coef1 = shuffle(Coef_A, diff);
    Filt = xt_mad32(x2, (uchar128)coeff8, Filt);
    wtSum = xt_add32((uchar128)0, (uchar128)coeff8, wtSum);

    diff = abs_diff(x3, x22);
    diff = diff >> LUT_SHIFT;
    coef1 = shuffle(Coef_E, diff);
    Filt = xt_mad32(x3, coef1, Filt);

    diff = abs_diff(x4, x22);
    diff = diff >> LUT_SHIFT;
    coef = shuffle(Coef_C, diff);
    Filt = xt_mad32(x4, coef, Filt);

    wtSum = xt_add32(coef, coef1, wtSum);

    ushort128 wtsumst = convert_ushort128(wtSum);
    vstore128(wtsumst, 0, (ushort *)tempDiv1);
    int64 FltA, FltB;
    FltA = Filt.lo;
    FltB = Filt.hi;
    vstore32(FltA.lo, 0, (int *)(tempDiv1 + 128));
    vstore32(FltA.hi, 0, (int *)(tempDiv1 + 192));
    vstore32(FltB.lo, 0, (int *)(tempDiv1 + 256));
    vstore32(FltB.hi, 0, (int *)(tempDiv1 + 320));
    tempDiv1 += 384;
  }

  tempDiv1 = tempDiv;
  for (int y = 0; y < inTileH; ++y) {
    int128 Filt;  // = (int128)0;
    int128 wtSum; // = (int128)0;

    short128 wtSumA = vload128(0, (short *)tempDiv1);

    wtSum = xt_mul32(wtSumA, (char128)1);

    int64 FiltA, FiltB;
    FiltA.lo = vload32(0, (int *)(tempDiv1 + 128));
    FiltA.hi = vload32(0, (int *)(tempDiv1 + 192));
    FiltB.lo = vload32(0, (int *)(tempDiv1 + 256));
    FiltB.hi = vload32(0, (int *)(tempDiv1 + 320));
    Filt.lo = FiltA;
    Filt.hi = FiltB;
    tempDiv1 += 384;
    __local uchar *pSrc2 = local_src + (y + 2) * local_src_pitch + idx;
    uchar128 x20 = vload128(0, pSrc2);
    uchar128 xtail = vload128(1, pSrc2);
    uchar128 x22 = shuffle2(x20, xtail, maskshift2);

    __local uchar *pSrc = local_src + (y + 3) * local_src_pitch + idx;
    __constant uchar128 *pCoef = (__constant uchar128 *)Coef;
    pCoef += 15;

    uchar128 x00 = vload128(0, pSrc);
    uchar128 xtail0 = vload128(1, pSrc);
    pSrc += local_src_pitch;
    uchar128 x0 = x00;
    uchar128 x1 = shuffle2(x00, xtail0, maskshift1);
    uchar128 x2 = shuffle2(x00, xtail0, maskshift2);
    uchar128 x3 = shuffle2(x00, xtail0, maskshift3);
    uchar128 x4 = shuffle2(x00, xtail0, maskshift4);

    uchar128 diff = abs_diff(x0, x22);
    diff = diff >> LUT_SHIFT;
    uchar128 coef = shuffle(Coef_B, diff);
    Filt = xt_mad32(x0, coef, Filt);

    diff = abs_diff(x1, x22);
    diff = diff >> LUT_SHIFT;
    uchar128 coef1 = shuffle(Coef_D, diff);
    Filt = xt_mad32(x1, coef1, Filt);
    wtSum = xt_add32(coef, coef1, wtSum);

    diff = abs_diff(x2, x22);
    diff = diff >> LUT_SHIFT;
    coef = shuffle(Coef_E, diff);
    Filt = xt_mad32(x2, coef, Filt);

    diff = abs_diff(x3, x22);
    diff = diff >> LUT_SHIFT;
    coef1 = shuffle(Coef_D, diff);
    Filt = xt_mad32(x3, coef1, Filt);
    wtSum = xt_add32(coef, coef1, wtSum);

    diff = abs_diff(x4, x22);
    diff = diff >> LUT_SHIFT;
    coef = shuffle(Coef_B, diff);
    Filt = xt_mad32(x4, coef, Filt);

    pCoef += 5;

    x00 = vload128(0, pSrc);
    xtail0 = vload128(1, pSrc);
    pSrc += local_src_pitch;
    x0 = x00;
    x1 = shuffle2(x00, xtail0, maskshift1);
    x2 = shuffle2(x00, xtail0, maskshift2);
    x3 = shuffle2(x00, xtail0, maskshift3);
    x4 = shuffle2(x00, xtail0, maskshift4);

    diff = abs_diff(x0, x22);
    diff = diff >> LUT_SHIFT;
    coef1 = shuffle(Coef_A, diff);
    Filt = xt_mad32(x0, coef1, Filt);

    wtSum = xt_add32(coef, coef1, wtSum);

    diff = abs_diff(x1, x22);
    diff = diff >> LUT_SHIFT;
    coef1 = shuffle(Coef_B, diff);
    Filt = xt_mad32(x1, coef1, Filt);

    diff = abs_diff(x2, x22);
    diff = diff >> LUT_SHIFT;
    coef = shuffle(Coef_C, diff);
    Filt = xt_mad32(x2, coef, Filt);

    wtSum = xt_add32(coef, coef1, wtSum);

    diff = abs_diff(x3, x22);
    diff = diff >> LUT_SHIFT;
    coef1 = shuffle(Coef_B, diff);
    Filt = xt_mad32(x3, coef1, Filt);

    diff = abs_diff(x4, x22);
    diff = diff >> LUT_SHIFT;
    coef = shuffle(Coef_A, diff);
    Filt = xt_mad32(x4, coef, Filt);
    wtSum = xt_add32(coef, coef1, wtSum);

    int128 Result;
    short128 wtSum16b = xt_convert_short128(wtSum, 0);
    Result = xt_div(Filt, wtSum16b);
    vstore128(xt_convert_char128(Result, 0), 0, (__local char *)pDst);
    pDst += local_dst_pitch;
  }
}
