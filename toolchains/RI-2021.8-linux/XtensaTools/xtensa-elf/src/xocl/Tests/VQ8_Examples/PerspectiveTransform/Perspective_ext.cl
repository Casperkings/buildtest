  // move upper left corner to the beginning of the data to exclude negative
  // offsets
  uint xs = startX;
  uint ys = startY;
  uint xd = out_j;
  uint yd = out_i;

  // recalculate coefficients in order to get rid of (xs + 0.5)
  // xsrc = ((a11*(xd + 0.5 + x) + a12*(yd + 0.5 + y) + a13) / (a31*(xd + 0.5 +
  // x) + a32*(yd + 0.5 + y) + a33)) - xs - 0.5 =
  //       = (b11*(xd + x) + b12*(yd + y) + (b11 + b12)/2 + b13) / (a31*(xd + x)
  //       + a32*(yd + y) + (a31 + a32)/2 + a33)
  a11 = a11 - a31 * xs - a31 / 2;
  a12 = a12 - a32 * xs - a32 / 2;
  a13 = a13 - a33 * xs - a33 / 2;
  a21 = a21 - a31 * ys - a31 / 2;
  a22 = a22 - a32 * ys - a32 / 2;
  a23 = a23 - a33 * ys - a33 / 2;

  uint offset1 = a12 * yd + (a11 + a12) / 2 + a13;
  uint offset2 = a22 * yd + (a21 + a22) / 2 + a23;
  uint offset3 = a32 * yd + (a31 + a32) / 2 + a33;

  ushort64 xseq = *(__constant ushort64 *)kxseq;

  uint64 Xseq, Yseq, Dseq;
  xseq += (ushort64)xd;
  Xseq.lo = mul_32x64(xseq, (uint32)a11);
  Yseq.lo = mul_32x64(xseq, (uint32)a21);
  Dseq.lo = mul_32x64(xseq, (uint32)a31);

  xseq += (ushort64)32;
  Xseq.hi = mul_32x64(xseq, (uint32)a11);
  Yseq.hi = mul_32x64(xseq, (uint32)a21);
  Dseq.hi = mul_32x64(xseq, (uint32)a31);
  
  a11 *= 64;
  a21 *= 64;
  a31 *= 64;

  __local uchar *src_Y = (__local uchar *)local_srcY;
  __local uchar *src_Y_1 = (__local uchar *)local_srcY + local_src_pitch;

  for (int x = 0; x < outTileW;
       x += 64, offset1 += a11, offset2 += a21, offset3 += a31) {
    uint yoffset1 = offset1;
    uint yoffset2 = offset2;
    uint yoffset3 = offset3;
    __local uchar *dst_Y = (__local uchar *)local_dstY + x;

    for (int y = 0; y < outTileH;
         ++y, yoffset1 += a12, yoffset2 += a22, yoffset3 += a32) {
       
      uint64 X = Xseq + (uint64)yoffset1;
      uint64 Y = Yseq + (uint64)yoffset2;
      uint64 D = Dseq + (uint64)yoffset3;

      uint64 ns = clz(D);
      uint64 as = (uint64)16 - ns;
      D = D << ns;

      // uint64 Q = X / (D >>16);
      ushort64 D16b = convert_ushort64((D >> 16));
      uint64 Q = xt_div(X, D16b);
      uint64 R = Q << ns;
      Q = Q >> as;

      ushort64 Xint = convert_ushort64(Q);
      ushort64 Xfrac = convert_ushort64(R);
      Xfrac = Xfrac >> 1;

      // Q = Y / (D >>16);
      Q = xt_div(Y, D16b);

      R = Q << ns;
      Q = Q >> as;

      ushort64 Yint = convert_ushort64(Q);
      ushort64 Yfrac = convert_ushort64(R);
      Yfrac = Yfrac >> 1;

      ushort64 vecAddr = Yint * (ushort64)local_src_pitch + Xint;
      ushort64 vecAddrR = vecAddr + (ushort64)1;
      ushort64 vecTL = convert_ushort64(xt_gather64(src_Y, (ushort *)&vecAddr));
      ushort64 vecTR =
          convert_ushort64(xt_gather64(src_Y, (ushort *)&vecAddrR));
      ushort64 vecBL =
          convert_ushort64(xt_gather64(src_Y_1, (ushort *)&vecAddr));
      ushort64 vecBR =
          convert_ushort64(xt_gather64(src_Y_1, (ushort *)&vecAddrR));

      int64 val_A = xt_mul32(as_short64(vecTR - vecTL), as_short64(Xfrac));
      val_A = xt_add32(0, (short64)(1 << 14), val_A);
      ushort64 val0 = as_ushort64(xt_convert_short64(val_A, 15));
      val0 += vecTL;

      int64 val_C = xt_mul32(as_short64(vecBR - vecBL), as_short64(Xfrac));
      val_C = xt_add32(0, (short64)(1 << 14), val_C);
      ushort64 val1 = as_ushort64(xt_convert_short64(val_C, 15));
      val1 += vecBL;

      val_A = xt_mul32(as_short64(val1 - val0), as_short64(Yfrac));
      val_A = xt_add32(0, (short64)(1 << 14), val_A);
      ushort64 val = as_ushort64(xt_convert_short64(val_A, 15));
      val += val0;

      vstore64(convert_uchar64(val), 0, dst_Y);
      dst_Y += local_dst_pitch;
    }
  }
