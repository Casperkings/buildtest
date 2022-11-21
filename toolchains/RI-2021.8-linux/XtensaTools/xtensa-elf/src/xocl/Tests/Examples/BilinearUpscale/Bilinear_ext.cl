  int64 xintseq = *(__constant int64 *)kxintseq;

  int xstep = max(1, min(1 * 64, (int)(((2 * 64) << 18) / (xscale))));

  xintseq += (int64)xd;

#if XT_OCL_VQ7
  int64 wx = (int64)xscale * xintseq;
#else
  int64 wx = xt_mul((int64)xscale, xintseq);
#endif

  int min = xd * xscale;
  XI_Q13_18 offset =
      ((xshift - ((xs) << 18) + (xscale >> 1) - (1 << 17))) - (xscale * xstep);

  wx += (int64)offset;
  min += offset;

  XI_Q13_18 xoffset0 = (xscale * xstep);

  // kernel using standard OpenCL C
  for (idx = 0; idx < outTileW; idx += xstep) {
    XI_Q13_18 ysrc =
        (yd * yscale + yshift - (ys << 18) + (yscale >> 1) - (1 << 17)) +
        4; // +4 for yf calculation round in cycle
    wx += (int64)xoffset0;
    min += xoffset0;
    int64 x = wx >> 18;

    short64 xf = convert_short64((wx & (int64)((1 << 18) - 1)) >> 3);
    short64 one_xf = ((short64)(1 << 15) - (xf));
    x -= (int64)(min >> 18);
    uchar64 x0sh = convert_uchar64(x);

    uchar64 x0nsh = x0sh + (uchar64)1;
    __local uchar *src0 = local_src + (min >> 18);
    __local uchar *dst = local_dst + idx;

    for (int y = 0; y < outTileH; ++y) {
      __local uchar *src00 = src0 + ((ysrc >> 18) * local_src_pitch);
      __local uchar *src1 = src00 + local_src_pitch;
      short yf = (ysrc & ((1 << 18) - 1)) >> 3;
      uchar64 x00 = vload64(0, src00);
      uchar64 x01 = vload64(0, src00 + 64);
      uchar64 x10 = vload64(0, src1);
      uchar64 x11 = vload64(0, src1 + 64);

      uchar64 dst00 = shuffle2(x00, x01, (x0sh));
      uchar64 dst01 = shuffle2(x00, x01, (x0nsh));
      uchar64 dst10 = shuffle2(x10, x11, (x0sh));
      uchar64 dst11 = shuffle2(x10, x11, (x0nsh));

      int64 dst00_int;

      dst00_int = xt_mul24((short64)((1 << 15) - yf), dst00);
      dst00_int = xt_mad24((short64)yf, dst10, dst00_int);
      dst00 = xt_convert24_uchar64_sat_rnd(dst00_int, 15);
      // int32 dst00 = ((int32)dst00 * (int32)((1 << 15) - yf) + (int32)dst10 *
      // (int32)yf + (int32)(1 << 14)) >> 15;

      dst00_int = xt_mul24((short64)((1 << 15) - yf), dst01);
      dst00_int = xt_mad24((short64)yf, dst11, dst00_int);
      dst01 = xt_convert24_uchar64_sat_rnd(dst00_int, 15);
      // int32 dst01 = ((int32)dst01 * (int32)((1 << 15) - yf) + (int32)dst11 *
      // (int32)yf + (int32)(1 << 14)) >> 15;

      dst00_int = xt_mul24(one_xf, dst00);
      dst00_int = xt_mad24(xf, dst01, dst00_int);
      uchar64 dst00_r = xt_convert24_uchar64_sat_rnd(dst00_int, 15);
      // dst00 = (dst00 * ((1 << 15) - xf) + dst01 * xf + (1 << 14)) >> 15;

      vstore64(dst00_r, 0, dst);
      dst += outTileW;
      ysrc += yscale;
    }
  }
