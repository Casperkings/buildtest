  const ushort64 InterleaveA = {
    0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30,
    32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62,
    64, 66, 68, 70, 72, 74, 76, 78, 80, 82, 84, 86, 88, 90, 92, 94, 96, 98, 100,
    102, 104, 106, 108, 110, 112, 114, 116, 118, 120, 122, 124, 126};

  const ushort64 InterleaveB = InterleaveA + (ushort64)1; 

  // kernel using standard OpenCL C
  for (idx = 0; idx < outTileW; idx += 128) {
    
    __local uchar *pDstY = (local_dstY + idx);
    __local uchar *pDstU = (local_dstU + (idx/2));
    __local uchar *pDstV = (local_dstV + (idx/2));

    for (int y = 0; y < outTileH; y+=2) {
      __local uchar *pSrc = (local_src  + y * local_src_pitch + idx * 3);
      uchar128 xRGB[3];
      uchar128 xR0, xG0, xB0;
      xt_deinterleave_x3(xRGB, pSrc);
      xR0 = xRGB[0];
      xG0 = xRGB[1];
      xB0 = xRGB[2];

      /* Y part */
      int128 ColorY = xt_mul32((short128)Q15_0_2126, xR0);
      ColorY = xt_mad32((short128)Q15_0_7152, xG0, ColorY);
      ColorY = xt_mad32((short128)Q15_0_0722, xB0, ColorY);
      uchar128 xY0 = xt_convert_uchar128_sat_rnd(ColorY, 15);
      vstore128(xY0, 0, (__local uchar*)pDstY);
      pDstY += local_dst_pitch;
    
      pSrc = (local_src  + (y+1) * local_src_pitch + (idx) * 3);
      uchar128 xRGB2[3];
      uchar128 xR2, xG2, xB2;
      xt_deinterleave_x3(xRGB2, pSrc);
      xR2 = xRGB2[0];
      xG2 = xRGB2[1];
      xB2 = xRGB2[2];

      /* Y part */
      ColorY = xt_mul32((short128)Q15_0_2126, xR2);
      ColorY = xt_mad32((short128)Q15_0_7152, xG2, ColorY);
      ColorY = xt_mad32((short128)Q15_0_0722, xB2, ColorY);
      uchar128 xY2 = xt_convert_uchar128_sat_rnd(ColorY, 15);
      vstore128(xY2, 0, (__local uchar*)pDstY);
      pDstY += local_dst_pitch;    
    
      /* U part */
      int128 xxB = xt_add32(xB0, xB2);
      short128 xB_all = convert_short128(xxB);
      short64 xB = shuffle2(xB_all.lo, xB_all.hi, InterleaveA);
      xB += shuffle2(xB_all.lo, xB_all.hi, InterleaveB);
        
      int128 xxR = xt_add32(xR0, xR2);
      short128 xR_all = convert_short128(xxR);
      short64 xR = shuffle2(xR_all.lo, xR_all.hi, InterleaveA);
      xR += shuffle2(xR_all.lo, xR_all.hi, InterleaveB);
  
      int128 xxY = xt_add32(xY0, xY2);
      short128 xY_all = convert_short128(xxY);
      short64 xY = shuffle2(xY_all.lo, xY_all.hi, InterleaveA);
      xY += shuffle2(xY_all.lo, xY_all.hi, InterleaveB);
      
      int64 ColorU;
      ColorU = xt_mul32((short64)Q15_0_5389, xB);
      ColorU = xt_mad32((short64)-Q15_0_5389, xY, ColorU);
    
      short64 xxU = xt_convert_short64_sat_rnd(ColorU, 17);
      xxU     += (short64)128;
    
      /* V part */
      int64 ColorV;
      ColorV = xt_mul32((short64)Q15_0_6350, xR);
      ColorV = xt_mad32((short64)-Q15_0_6350, xY, ColorV);
    
      short64 xxV = xt_convert_short64_sat_rnd(ColorV, 17);
      xxV     += (short64)128;

      vstore64(convert_char64(xxU), 0, (__local char*)pDstU);
      pDstU += local_dst_pitch/2;

      vstore64(convert_char64(xxV), 0, (__local char*)pDstV);
      pDstV += local_dst_pitch/2;
    }
  }
