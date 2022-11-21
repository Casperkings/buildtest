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

  ushort32 xseq = *(__constant ushort32 *)kxseq;

  uint32 Xseq, Yseq, Dseq;
  xseq += (ushort32)xd;
  Xseq.lo = mul_16x32(xseq, (uint16)a11);
  Yseq.lo = mul_16x32(xseq, (uint16)a21);
  Dseq.lo = mul_16x32(xseq, (uint16)a31);

  xseq += (ushort32)16;
  Xseq.hi = mul_16x32(xseq, (uint16)a11);
  Yseq.hi = mul_16x32(xseq, (uint16)a21);
  Dseq.hi = mul_16x32(xseq, (uint16)a31);
  
  a11 *= 32;
  a21 *= 32;
  a31 *= 32;

  __local uchar *src_Y = (__local uchar *)local_srcY;
  __local uchar *src_Y_1 = (__local uchar *)local_srcY + local_src_pitch;

  for (int x = 0; x < outTileW;
       x += 32, offset1 += a11, offset2 += a21, offset3 += a31) {
    uint yoffset1 = offset1;
    uint yoffset2 = offset2;
    uint yoffset3 = offset3;
    __local uchar *dst_Y = (__local uchar *)local_dstY + x;

    for (int y = 0; y < outTileH;
         ++y, yoffset1 += a12, yoffset2 += a22, yoffset3 += a32) {
       
      uint32 X = Xseq + (uint32)yoffset1;
      uint32 Y = Yseq + (uint32)yoffset2;
      uint32 D = Dseq + (uint32)yoffset3;

      uint32 ns = clz(D);
      uint32 as = (uint32)16 - ns;
      D = D << ns;

      // uint16 Q = X / (D >>16);
      ushort32 D16b = convert_ushort32((D >> 16));
      uint32 Q = xt_div(X, D16b);
      uint32 R = Q << ns;
      Q = Q >> as;

      ushort32 Xint = convert_ushort32(Q);
      ushort32 Xfrac = convert_ushort32(R);
      Xfrac = Xfrac >> 1;

      // Q = Y / (D >>16);
      Q = xt_div(Y, D16b);

      R = Q << ns;
      Q = Q >> as;

      ushort32 Yint = convert_ushort32(Q);
      ushort32 Yfrac = convert_ushort32(R);
      Yfrac = Yfrac >> 1;

      ushort32 vecAddr = Yint * (ushort32)local_src_pitch + Xint;
      ushort32 vecAddrR = vecAddr + (ushort32)1;
      ushort32 vecTL = convert_ushort32(xt_gather32(src_Y, (ushort *)&vecAddr));
      ushort32 vecTR =
          convert_ushort32(xt_gather32(src_Y, (ushort *)&vecAddrR));
      ushort32 vecBL =
          convert_ushort32(xt_gather32(src_Y_1, (ushort *)&vecAddr));
      ushort32 vecBR =
          convert_ushort32(xt_gather32(src_Y_1, (ushort *)&vecAddrR));

      int32 val_A = xt_mul32(as_short32(vecTR - vecTL), as_short32(Xfrac));
      val_A = xt_add32(0, (short32)(1 << 14), val_A);
      ushort32 val0 = as_ushort32(xt_convert_short32(val_A, 15));
      val0 += vecTL;

      int32 val_C = xt_mul32(as_short32(vecBR - vecBL), as_short32(Xfrac));
      val_C = xt_add32(0, (short32)(1 << 14), val_C);
      ushort32 val1 = as_ushort32(xt_convert_short32(val_C, 15));
      val1 += vecBL;

      val_A = xt_mul32(as_short32(val1 - val0), as_short32(Yfrac));
      val_A = xt_add32(0, (short32)(1 << 14), val_A);
      ushort32 val = as_ushort32(xt_convert_short32(val_A, 15));
      val += val0;

      vstore32(convert_uchar32(val), 0, dst_Y);
      dst_Y += local_dst_pitch;
    }
  }
