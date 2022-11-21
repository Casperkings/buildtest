  const ushort32 InterleaveA = {
    0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30,
    32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62};

  const ushort32 InterleaveB = InterleaveA + (ushort32)1; 

  const uchar64 maskR0 = *(__constant uchar64 *)kmaskR0;
  const uchar64 maskR1 = *(__constant uchar64 *)kmaskR1;
  
  const uchar64 maskG0 = maskR0 + (uchar64)1;
  const uchar64 maskG1 = *(__constant uchar64 *)kmaskG1;
    
  const uchar64 maskB0 = maskR0 + (uchar64)2;
  const uchar64 maskB1 = *(__constant uchar64 *)kmaskB1;

  // kernel using standard OpenCL C
  for (idx = 0; idx < outTileW; idx += 64) {
    
    __local uchar *pDstY = (local_dstY + idx);
    __local uchar *pDstU = (local_dstU + (idx/2));
    __local uchar *pDstV = (local_dstV + (idx/2));

    for (int y = 0; y < outTileH; y+=2) {
      __local uchar *pSrc = (local_src  + y * local_src_pitch + idx * 3);
      uchar64 xRGB[3];
      uchar64 xR0, xG0, xB0;
      xt_deinterleave_x3(xRGB, pSrc);
      xR0 = xRGB[0];
      xG0 = xRGB[1];
      xB0 = xRGB[2];

      /* Y part */
      int64 ColorY = xt_mul24((short64)Q15_0_2126, xR0);
      ColorY = xt_mad24((short64)Q15_0_7152, xG0, ColorY);
      ColorY = xt_mad24((short64)Q15_0_0722, xB0, ColorY);
      uchar64 xY0 = xt_convert24_uchar64_sat_rnd(ColorY, 15);
      vstore64(xY0, 0, (__local uchar*)pDstY);
      pDstY += local_dst_pitch;
    
      pSrc = (local_src  + (y+1) * local_src_pitch + (idx) * 3);
      uchar64 xRGB2[3];
      uchar64 xR2, xG2, xB2;
      xt_deinterleave_x3(xRGB2, pSrc);
      xR2 = xRGB2[0];
      xG2 = xRGB2[1];
      xB2 = xRGB2[2];

      /* Y part */
      ColorY = xt_mul24((short64)Q15_0_2126, xR2);
      ColorY = xt_mad24((short64)Q15_0_7152, xG2, ColorY);
      ColorY = xt_mad24((short64)Q15_0_0722, xB2, ColorY);
      uchar64 xY2 = xt_convert24_uchar64_sat_rnd(ColorY, 15);
      vstore64(xY2, 0, (__local uchar*)pDstY);
      pDstY += local_dst_pitch;    
    
      /* U part */
      int64 xxB = xt_add24(xB0, xB2);
      short64 xB_all = xt_convert24_short64(xxB);
      short32 xB = shuffle2(xB_all.lo, xB_all.hi, InterleaveA);
      xB += shuffle2(xB_all.lo, xB_all.hi, InterleaveB);
        
      int64 xxR = xt_add24(xR0, xR2);
      short64 xR_all = xt_convert24_short64(xxR);
      short32 xR = shuffle2(xR_all.lo, xR_all.hi, InterleaveA);
      xR += shuffle2(xR_all.lo, xR_all.hi, InterleaveB);
  
      int64 xxY = xt_add24(xY0, xY2);
      short64 xY_all = xt_convert24_short64(xxY);
      short32 xY = shuffle2(xY_all.lo, xY_all.hi, InterleaveA);
      xY += shuffle2(xY_all.lo, xY_all.hi, InterleaveB);
      
      int32 ColorU;
      ColorU = xt_mul32((short32)Q15_0_5389, xB);
      ColorU = xt_mad32((short32)-Q15_0_5389, xY, ColorU);
    
      short32 xxU = xt_convert_short32_sat_rnd(ColorU, 17);
      xxU     += (short32)128;
    
      /* V part */
      int32 ColorV;
      ColorV = xt_mul32((short32)Q15_0_6350, xR);
      ColorV = xt_mad32((short32)-Q15_0_6350, xY, ColorV);
    
      short32 xxV = xt_convert_short32_sat_rnd(ColorV, 17);
      xxV     += (short32)128;

      vstore32(convert_char32(xxU), 0, (__local char*)pDstU);
      pDstU += local_dst_pitch/2;

      vstore32(convert_char32(xxV), 0, (__local char*)pDstV);
      pDstV += local_dst_pitch/2;
    }
  }
