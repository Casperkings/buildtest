#define MIN4(a, b, c, d) (min(min(a, b), min(c, d)))
#define MAX4(a, b, c, d) (max(max(a, b), max(c, d)))

__constant ushort kxseq[] __attribute__((section(".dram0.data"))) = {
  0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7,
  8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15,
  16, 16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 
  23, 23, 24, 24, 25, 25, 26, 26, 27, 27, 28, 28, 29, 29,
  30, 30, 31, 31
};

__constant ushort kxseq1[] __attribute__((section(".dram0.data"))) = {
  32, 32, 33, 33, 34, 34, 35, 35, 36, 36, 37, 37, 38, 38, 39, 39, 40, 40,
  41, 41, 42, 42, 43, 43, 44, 44, 45, 45, 46, 46, 47, 47, 48, 48, 49, 49,
  50, 50, 51, 51, 52, 52, 53, 53, 54, 54, 55, 55, 56, 56, 57, 57, 58, 58,
  59, 59, 60, 60, 61, 61, 62, 62, 63, 63
};

uint32 mul_32x64(ushort64 a, uint32 b) {
  ushort64 tmp_lo = a * as_ushort64(b);
  ushort64 tmp_hi = mul_hi(a, as_ushort64(b));
  uint32 tmpHi = as_uint32(tmp_hi);
  tmpHi = tmpHi << 16;
  tmpHi += as_uint32(tmp_lo);
  return tmpHi;
}


__kernel __attribute__((reqd_work_group_size(1, 1, 1)))
__attribute__((reqd_work_dim_size(2))) void
oclKernel(__global uchar *restrict SrcY, int SrcPitch,
          __global uchar *restrict DstY, const int DstPitch, int width,
          int height, int dstWidth, int dstHeight, int outTileW, int outTileH,
          __constant uint *restrict perspective,
          __constant ushort *restrict InputBB,
          __local uchar *restrict local_srcY,
          __local uchar *restrict local_dstY,
          __global int *restrict err_codes) {
  uint i, j;
  uint idx, idy;
  uint local_src_pitch, local_dst_pitch;
  uint err = 0;
  uint tlx, tly, inTileW, inTileH;
  i = get_global_id(0);
  j = get_global_id(1);

  uint out_i, out_j;
  out_i = i * outTileH;
  out_j = j * outTileW;
  uint dim1size = get_global_size(1);
  uint once = 1;

  uint a11 = perspective[0];
  uint a12 = perspective[1];
  uint a13 = perspective[2];
  uint a21 = perspective[3];
  uint a22 = perspective[4];
  uint a23 = perspective[5];
  uint a31 = perspective[6];
  uint a32 = perspective[7];
  uint a33 = perspective[8];

  /*pre-Computed bonding Box for Tile DMA In*/
  uint startX = InputBB[(i * dim1size + j) * 4];
  uint startY = InputBB[((i * dim1size + j) * 4) + 1];
  inTileW = InputBB[((i * dim1size + j) * 4) + 2];
  inTileH = InputBB[((i * dim1size + j) * 4) + 3];

  local_src_pitch = inTileW;
  local_dst_pitch = outTileW;

  event_t evtC = xt_async_work_group_2d_copy(
      local_srcY, SrcY + (startY * SrcPitch) + startX, local_src_pitch, inTileH,
      local_src_pitch, SrcPitch, 0);
  wait_group_events(1, &evtC);
#if NATIVE_KERNEL
  #include "Perspective_ivp.c"
#elif XT_OCL_EXT
  #include "Perspective_ext.cl"
#else
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

  ushort64 xseq = {0, 0, 1, 1, 2,  2,  3,  3,  4,  4,  5,  5,  6,  6,  7,  7,
                   8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15,
                   16, 16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 
                   23, 23, 24, 24, 25, 25, 26, 26, 27, 27, 28, 28, 29, 29,
                   30, 30, 31, 31};

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

      uint64 ns;// = clz(D);
      ns.lo = clz(D.lo);
      ns.hi = clz(D.hi);
      
      uint64 as = (uint64)16 - ns;
      
      D = D << ns;
      // uint64 Q = X / (D >>16);
      uint64 Q = xt_div(X, (D >> 16));
      uint64 R = Q << ns;
      Q = Q >> as;

      ushort64 Xint = convert_ushort64(Q);
      ushort64 Xfrac = convert_ushort64(R);
      Xfrac = Xfrac >> 1;

      // Q = Y / (D >>16);
      Q = xt_div(Y, (D >> 16));
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

      uint64 val_A = (uint64)0x4000;
      val_A += xt_mul32(vecTR - vecTL, Xfrac);

      val_A = val_A >> 15;
      ushort64 val0 = convert_ushort64(val_A);
      val0 += vecTL;

      uint64 val_C = (uint64)0x4000;
      val_C += xt_mul32(vecBR - vecBL, Xfrac);
      val_C = val_C >> 15;
      ushort64 val1 = convert_ushort64(val_C);
      val1 += vecBL;

      val_A = (uint64)0x4000;
      val_A += xt_mul32(val1 - val0, Yfrac);
      val_A = val_A >> 15;
      ushort64 val = convert_ushort64(val_A);
      val += val0;
      vstore64(convert_uchar64(val), 0, dst_Y);
      dst_Y += local_dst_pitch;
    }
  }
#endif
  event_t evtF = xt_async_work_group_2d_copy(DstY + (out_i * DstPitch) + out_j,
                                             local_dstY, outTileW, outTileH,
                                             DstPitch, local_dst_pitch, 0);
  wait_group_events(1, &evtF);
}
