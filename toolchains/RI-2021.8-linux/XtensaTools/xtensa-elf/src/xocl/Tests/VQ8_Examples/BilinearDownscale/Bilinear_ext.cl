  int128 xintseq = *(__constant int128 *)kxintseq;

  int xstep = max(1, min(1 * 128, (int)(((2 * 128) << 18) / (xscale))));

  xintseq += (int128)xd;

  int128 wx = xt_mul((int128)xscale, xintseq);

  int min = xd * xscale;
  XI_Q13_18 offset =
      ((xshift - ((xs) << 18) + (xscale >> 1) - (1 << 17))) - (xscale * xstep);

  wx += (int128)offset;
  min += offset;

  XI_Q13_18 xoffset0 = (xscale * xstep);

  // kernel using standard OpenCL C
  for (idx = 0; idx < outTileW; idx += xstep) {
    XI_Q13_18 ysrc =
        (yd * yscale + yshift - (ys << 18) + (yscale >> 1) - (1 << 17)) +
        4; // +4 for yf calculation round in cycle
    wx += (int128)xoffset0;
    min += xoffset0;
    int128 x = wx >> 18;

    short128 xf = convert_short128((wx & (int128)((1 << 18) - 1)) >> 3);
    short128 one_xf = ((short128)(1 << 15) - (xf));
    x -= (int128)(min >> 18);
    uchar128 x0sh = convert_uchar128(x);

    uchar128 x0nsh = x0sh + (uchar128)1;
    __local uchar *src0 = local_src + (min >> 18);
    __local uchar *dst = local_dst + idx;

    for (int y = 0; y < outTileH; ++y) {
      __local uchar *src00 = src0 + ((ysrc >> 18) * local_src_pitch);
      __local uchar *src1 = src00 + local_src_pitch;
      short yf = (ysrc & ((1 << 18) - 1)) >> 3;
      uchar128 x00 = vload128(0, src00);
      uchar128 x01 = vload128(0, src00 + 128);
      uchar128 x10 = vload128(0, src1);
      uchar128 x11 = vload128(0, src1 + 128);

      uchar128 dst00 = shuffle2(x00, x01, (x0sh));
      uchar128 dst01 = shuffle2(x00, x01, (x0nsh));
      uchar128 dst10 = shuffle2(x10, x11, (x0sh));
      uchar128 dst11 = shuffle2(x10, x11, (x0nsh));

      int128 dst00_int;

      dst00_int = xt_mul32((short128)((1 << 15) - yf), dst00);
      dst00_int = xt_mad32((short128)yf, dst10, dst00_int);
      dst00 = xt_convert_uchar128_sat_rnd(dst00_int, 15);
      // dst00 = (dst00 * ((1 << 15) - yf) + dst10 * yf + (1 << 14)) >> 15;

      dst00_int = xt_mul32((short128)((1 << 15) - yf), dst01);
      dst00_int = xt_mad32((short128)yf, dst11, dst00_int);
      dst01 = xt_convert_uchar128_sat_rnd(dst00_int, 15);
      // dst01 = (dst01 * ((1 << 15) - yf) + dst11 * yf + (1 << 14)) >> 15;

      dst00_int = xt_mul32(one_xf, dst00);
      dst00_int = xt_mad32(xf, dst01, dst00_int);
      uchar128 dst00_r = xt_convert_uchar128_sat_rnd(dst00_int, 15);
      // dst00 = (dst00 * ((1 << 15) - xf) + dst01 * xf + (1 << 14)) >> 15;

      vstore128(dst00_r, 0, dst);
      dst += outTileW;
      ysrc += yscale;
    }
  }
