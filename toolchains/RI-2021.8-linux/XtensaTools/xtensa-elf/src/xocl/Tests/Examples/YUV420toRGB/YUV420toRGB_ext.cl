const uchar64 InterleaveRGa = {
    0,  64, 1,  65, 2,  66, 3,  67, 4,  68, 5,  69, 6,  70, 7,  71,
    8,  72, 9,  73, 10, 74, 11, 75, 12, 76, 13, 77, 14, 78, 15, 79,
    16, 80, 17, 81, 18, 82, 19, 83, 20, 84, 21, 85, 22, 86, 23, 87,
    24, 88, 25, 89, 26, 90, 27, 91, 28, 92, 29, 93, 30, 94, 31, 95};
const uchar64 InterleaveRGb = *(__constant uchar64 *)kInterleaveRGb;
const uchar64 InterleaveRGc = *(__constant uchar64 *)kInterleaveRGc;

const uchar64 Mask0 = *(__constant uchar64 *)kMask0;
const uchar64 Mask1 = *(__constant uchar64 *)kMask1;
const uchar64 Mask2 = *(__constant uchar64 *)kMask2;

const uchar64 even_Mask = {
    0,  64,  2,  66,  4,  68,  6,  70,  8,  72,  10, 74,  12, 76,  14, 78,
    16, 80,  18, 82,  20, 84,  22, 86,  24, 88,  26, 90,  28, 92,  30, 94,
    32, 96,  34, 98,  36, 100, 38, 102, 40, 104, 42, 106, 44, 108, 46, 110,
    48, 112, 50, 114, 52, 116, 54, 118, 56, 120, 58, 122, 60, 124, 62, 126};

// kernel using standard OpenCL C
#pragma nounroll
for (idx = 0; idx < inTileW; idx += 64) {

  __local uchar *pSrcY = (local_srcY + idx);
  __local uchar *pSrcU = (local_srcU + idx / 2);
  __local uchar *pSrcV = (local_srcV + idx / 2);
  __local uchar *pDst = (__local uchar *)(local_dst + idx * 3);
  for (int y = 0; y < inTileH; y += 2) {

    __local uchar *pSrc = (local_srcY + local_src_pitch * y + idx);
    uchar64 x00 = vload64(0, pSrc);
    pSrc = (local_srcY + local_src_pitch * (y + 1) + idx);
    uchar64 x01 = vload64(0, pSrc);

    pSrc = (local_srcU + (local_src_pitch >> 1) * (y >> 1) + (idx >> 1));
    short32 x00u = convert_short32(vload32(0, pSrc));

    pSrc = (local_srcV + (local_src_pitch >> 1) * (y >> 1) + (idx >> 1));
    short32 x00v = convert_short32(vload32(0, pSrc));

    x00u -= (short32)128;
    x00v -= (short32)128;

    char64 x00u_char = as_char64(x00u);
    char64 x00v_char = as_char64(x00v);

    x00u_char = shuffle2(x00u_char, x00u_char, even_Mask);
    x00v_char = shuffle2(x00v_char, x00v_char, even_Mask);

    int64 AccR0 = xt_mul24((short64)Q15_0_7874, x00v_char);
    int64 AccR1 = AccR0;

    AccR0 = xt_mad24((short64)Q15_0_5000, x00, AccR0);
    AccR1 = xt_mad24((short64)Q15_0_5000, x01, AccR1);

    uchar64 Rout0 = xt_convert24_uchar64_sat_rnd(AccR0, 14);
    uchar64 Rout1 = xt_convert24_uchar64_sat_rnd(AccR1, 14);

    int64 AccB0 = xt_mul24((short64)Q15_0_9278, x00u_char);
    int64 AccB1 = AccB0;

    AccB0 = xt_mad24((short64)Q15_0_5000, x00, AccB0);
    AccB1 = xt_mad24((short64)Q15_0_5000, x01, AccB1);

    uchar64 Bout0 = xt_convert24_uchar64_sat_rnd(AccB0, 14);
    uchar64 Bout1 = xt_convert24_uchar64_sat_rnd(AccB1, 14);

    int64 AccG0 = xt_mul24((short64)-Q15_0_2340, x00v_char);
    AccG0 = xt_mad24((short64)-Q15_0_0936, x00u_char, AccG0);
    int64 AccG1 = AccG0;

    AccG0 = xt_mad24((short64)Q15_0_5000, x00, AccG0);
    AccG1 = xt_mad24((short64)Q15_0_5000, x01, AccG1);

    uchar64 Gout0 = xt_convert24_uchar64_sat_rnd(AccG0, 14);
    uchar64 Gout1 = xt_convert24_uchar64_sat_rnd(AccG1, 14);

    uchar64 RGB[] = {Rout0, Gout0, Bout0};
    xt_interleave_x3(RGB, pDst);
    pDst += local_dst_pitch;

    uchar64 RGB1[] = {Rout1, Gout1, Bout1};
    xt_interleave_x3(RGB1, pDst);
    pDst += local_dst_pitch;
  }
}
