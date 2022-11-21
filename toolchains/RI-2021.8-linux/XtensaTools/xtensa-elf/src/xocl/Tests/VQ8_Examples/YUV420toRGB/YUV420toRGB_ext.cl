const uchar128 even_Mask = {
    0, 128, 2, 130, 4, 132, 6, 134, 8, 136, 10, 138, 12, 140, 14, 142,
    16, 144, 18, 146, 20, 148, 22, 150, 24, 152, 26, 154, 28, 156, 30, 158,
    32, 160, 34, 162, 36, 164, 38, 166, 40, 168, 42, 170, 44, 172, 46, 174,
    48, 176, 50, 178, 52, 180, 54, 182, 56, 184, 58, 186, 60, 188, 62, 190,
    64, 192, 66, 194, 68, 196, 70, 198, 72, 200, 74, 202, 76, 204, 78, 206,
    80, 208, 82, 210, 84, 212, 86, 214, 88, 216, 90, 218, 92, 220, 94, 222,
    96, 224, 98, 226, 100, 228, 102, 230, 104, 232, 106, 234, 108, 236,
    110, 238, 112, 240, 114, 242, 116, 244, 118, 246, 120, 248, 122, 250,
    124, 252, 126, 254};

// kernel using standard OpenCL C
#pragma nounroll
for (idx = 0; idx < inTileW; idx += 128) {

  __local uchar *pSrcY = (local_srcY + idx);
  __local uchar *pSrcU = (local_srcU + idx / 2);
  __local uchar *pSrcV = (local_srcV + idx / 2);
  __local uchar *pDst = (__local uchar *)(local_dst + idx * 3);
  for (int y = 0; y < inTileH; y += 2) {

    __local uchar *pSrc = (local_srcY + local_src_pitch * y + idx);
    uchar128 x00 = vload128(0, pSrc);
    pSrc = (local_srcY + local_src_pitch * (y + 1) + idx);
    uchar128 x01 = vload128(0, pSrc);

    pSrc = (local_srcU + (local_src_pitch >> 1) * (y >> 1) + (idx >> 1));
    short64 x00u = convert_short64(vload64(0, pSrc));

    pSrc = (local_srcV + (local_src_pitch >> 1) * (y >> 1) + (idx >> 1));
    short64 x00v = convert_short64(vload64(0, pSrc));

    x00u -= (short64)128;
    x00v -= (short64)128;

    char128 x00u_char = as_char128(x00u);
    char128 x00v_char = as_char128(x00v);

    x00u_char = shuffle2(x00u_char, x00u_char, even_Mask);
    x00v_char = shuffle2(x00v_char, x00v_char, even_Mask);

    int128 AccR0 = xt_mul32((short128)Q15_0_7874, x00v_char);
    int128 AccR1 = AccR0;

    AccR0 = xt_mad32((short128)Q15_0_5000, x00, AccR0);
    AccR1 = xt_mad32((short128)Q15_0_5000, x01, AccR1);

    uchar128 Rout0 = xt_convert_uchar128_sat_rnd(AccR0, 14);
    uchar128 Rout1 = xt_convert_uchar128_sat_rnd(AccR1, 14);

    int128 AccB0 = xt_mul32((short128)Q15_0_9278, x00u_char);
    int128 AccB1 = AccB0;

    AccB0 = xt_mad32((short128)Q15_0_5000, x00, AccB0);
    AccB1 = xt_mad32((short128)Q15_0_5000, x01, AccB1);

    uchar128 Bout0 = xt_convert_uchar128_sat_rnd(AccB0, 14);
    uchar128 Bout1 = xt_convert_uchar128_sat_rnd(AccB1, 14);

    int128 AccG0 = xt_mul32((short128)-Q15_0_2340, x00v_char);
    AccG0 = xt_mad32((short128)-Q15_0_0936, x00u_char, AccG0);
    int128 AccG1 = AccG0;

    AccG0 = xt_mad32((short128)Q15_0_5000, x00, AccG0);
    AccG1 = xt_mad32((short128)Q15_0_5000, x01, AccG1);

    uchar128 Gout0 = xt_convert_uchar128_sat_rnd(AccG0, 14);
    uchar128 Gout1 = xt_convert_uchar128_sat_rnd(AccG1, 14);

    uchar128 RGB[] = {Rout0, Gout0, Bout0};
    xt_interleave_x3(RGB, pDst);
    pDst += local_dst_pitch;

    uchar128 RGB1[] = {Rout1, Gout1, Bout1};
    xt_interleave_x3(RGB1, pDst);
    pDst += local_dst_pitch;
  }
}
