  const uchar64 maskR0 = *(__constant uchar64 *)kmaskR0;
  const uchar64 maskR1 = *(__constant uchar64 *)kmaskR1;

  const uchar64 maskG0 = maskR0 + (uchar64)1;
  const uchar64 maskG1 = *(__constant uchar64 *)kmaskG1;

  const uchar64 maskB0 = maskR0 + (uchar64)2;
  const uchar64 maskB1 = *(__constant uchar64 *)kmaskB1;

  // kernel using standard OpenCL C
  for (idx = 0; idx < outTileW; idx += 128) {
    __local uchar *pSrc = (local_src + idx * 3);
    __local uchar *pSrc1 = (local_src + (idx+64) * 3);
    __local uchar *pDstY = (local_dstY + idx);
    __local uchar *pDstU = (local_dstU + idx);
    __local uchar *pDstV = (local_dstV + idx);
    __local uchar *pDstY1 = (local_dstY + idx + 64);
    __local uchar *pDstU1 = (local_dstU + idx + 64);
    __local uchar *pDstV1 = (local_dstV + idx + 64);

    for (int y = 0; y < outTileH; ++y) {
      uchar64 xRGB[3];
      xt_deinterleave_x3(xRGB, pSrc);
      uchar64 xR, xG, xB;
      xR = xRGB[0];
      xG = xRGB[1];
      xB = xRGB[2];
  
      /* Y part*/
      int64 ColorY = xt_mul24((short64)Q15_0_2126, xR);
      ColorY = xt_mad24((short64)Q15_0_7152, xG, ColorY);
      ColorY = xt_mad24((short64)Q15_0_0722, xB, ColorY);
      uchar64 xY = xt_convert24_uchar64_sat_rnd(ColorY, 15);

      /* U part*/
      int64 ColorU = xt_mul24((short64)Q15_0_5389, xB);
      ColorU = xt_mad24((short64)-Q15_0_5389, xY, ColorU);
      char64 xU = xt_convert24_char64_sat_rnd(ColorU, 15);
      xU += (char64)128;
	  
      /* V part*/
      int64 ColorV = xt_mul24((short64)Q15_0_6350, xR);
      ColorV = xt_mad24((short64)-Q15_0_6350, xY, ColorV);
      char64 xV = xt_convert24_char64_sat_rnd(ColorV, 15);
      xV += (char64)128;

      uchar64 xRGB1[3];
      xt_deinterleave_x3(xRGB1, pSrc1);
      uchar64 xR1, xG1, xB1;
      xR1 = xRGB1[0];
      xG1 = xRGB1[1];
      xB1 = xRGB1[2];

      /* Y part */
      int64 ColorY1 = xt_mul24((short64)Q15_0_2126, xR1);
      ColorY1 = xt_mad24((short64)Q15_0_7152, xG1, ColorY1);
      ColorY1 = xt_mad24((short64)Q15_0_0722, xB1, ColorY1);
      uchar64 xY1 = xt_convert24_uchar64_sat_rnd(ColorY1, 15);

      /* U part */
      int64 ColorU1 = xt_mul24((short64)Q15_0_5389, xB1);
      ColorU1 = xt_mad24((short64)-Q15_0_5389, xY1, ColorU1);
      char64 xU1 = xt_convert24_char64_sat_rnd(ColorU1, 15);
      xU1 += (char64)128;
	  
      /* V part */
      int64 ColorV1 = xt_mul24((short64)Q15_0_6350, xR1);
      ColorV1 = xt_mad24((short64)-Q15_0_6350, xY1, ColorV1);
      char64 xV1 = xt_convert24_char64_sat_rnd(ColorV1, 15);
      xV1 += (char64)128;

      vstore64(xY, 0, (__local uchar*)pDstY);
      pDstY += local_dst_pitch;
      vstore64(xU, 0, (__local char*)pDstU);
      pDstU += local_dst_pitch;
      vstore64(xV, 0, (__local char*)pDstV);
      pDstV += local_dst_pitch;

      vstore64(xY1, 0, (__local uchar*)pDstY1);
      pDstY1 += local_dst_pitch;
      vstore64(xU1, 0, (__local char*)pDstU1);
      pDstU1 += local_dst_pitch;
      vstore64(xV1, 0, (__local char*)pDstV1);
      pDstV1 += local_dst_pitch;

      pSrc += local_src_pitch;
      pSrc1 += local_src_pitch;
    }
  }
