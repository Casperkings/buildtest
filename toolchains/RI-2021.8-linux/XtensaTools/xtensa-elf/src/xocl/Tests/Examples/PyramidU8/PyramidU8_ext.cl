   const uchar64 EvenMask = {0,  2,  4,  6,  8,  10,  12,  14,  16,  18,  20,
                             22,  24,  26,  28,  30,  32,  34,  36,  38,  40,  
                             42,  44,  46,  48,  50,  52,  54,  56,  58,  60,  
                             62,  64,  66,  68,  70,  72,  74,  76,  78,  80,
                             82,  84,  86,  88,  90,  92,  94,  96,  98,  100,
                             102,  104,  106,  108,  110,  112,  114,  116, 
                             118,  120,  122,  124,  126};
   
   const uchar64 OddMask = EvenMask + (uchar64)1;

  for (idx = 0; idx < outTileW; idx += 64) {
    int y = 0;
    __local uchar *pSrc0 = local_src + y * local_src_pitch + idx * 2;
    uchar64 xtail = vload64(0, pSrc0);
    uchar64 xtail1 = vload64(0, pSrc0 + 64);
    uchar64 x00 = shuffle2(xtail, xtail1, EvenMask);
    uchar64 x01 = shuffle2(xtail, xtail1, OddMask);
    xtail = vload64(0, pSrc0 + 2);
    xtail1 = vload64(0, pSrc0 + 64 + 2);  
    uchar64 x02 = shuffle2(xtail, xtail1, EvenMask);
    uchar64 x03 = shuffle2(xtail, xtail1, OddMask);
    xtail = vload64(0, pSrc0 + 4);
    xtail1 = vload64(0, pSrc0 + 64 + 4);  
    uchar64 x04 = shuffle2(xtail, xtail1, EvenMask);

    __local uchar *pSrc1 = local_src + (y + 1) * local_src_pitch + idx * 2;

    xtail = vload64(0, pSrc1);
    xtail1 = vload64(0, pSrc1 + 64);
    uchar64 x10 = shuffle2(xtail, xtail1, EvenMask);
    uchar64 x11 = shuffle2(xtail, xtail1, OddMask);
    xtail = vload64(0, pSrc1 + 2);
    xtail1 = vload64(0, pSrc1 + 64 + 2);  
    uchar64 x12 = shuffle2(xtail, xtail1, EvenMask);
    uchar64 x13 = shuffle2(xtail, xtail1, OddMask);
    xtail = vload64(0, pSrc1 + 4);
    xtail1 = vload64(0, pSrc1 + 64 + 4);  
    uchar64 x14 = shuffle2(xtail, xtail1, EvenMask);

    __local uchar *pSrc2 = local_src + (y + 2) * local_src_pitch + idx * 2;
    xtail = vload64(0, pSrc2);
    xtail1 = vload64(0, pSrc2 + 64);
    uchar64 x20 = shuffle2(xtail, xtail1, EvenMask);
    uchar64 x21 = shuffle2(xtail, xtail1, OddMask);
    xtail = vload64(0, pSrc2 + 2);
    xtail1 = vload64(0, pSrc2 + 64 + 2);  
    uchar64 x22 = shuffle2(xtail, xtail1, EvenMask);
    uchar64 x23 = shuffle2(xtail, xtail1, OddMask);
    xtail = vload64(0, pSrc2 + 4);
    xtail1 = vload64(0, pSrc2 + 64 + 4);  
    uchar64 x24 = shuffle2(xtail, xtail1, EvenMask);
  
    int64 row0 = xt_mul24(x00, (char64)1);
    row0 = xt_mad24(x01, (char64)4, row0);
    row0 = xt_mad24(x02, (char64)6, row0);
    row0 = xt_mad24(x03, (char64)4, row0);
    row0 = xt_mad24(x04, (char64)1, row0);
    short64 row0_s = xt_convert24_short64(row0);

    int64 row1 = xt_mul24(x10, (char64)1);
    row1 = xt_mad24(x11, (char64)4, row1);
    row1 = xt_mad24(x12, (char64)6, row1);
    row1 = xt_mad24(x13, (char64)4, row1);
    row1 = xt_mad24(x14, (char64)1, row1);
    short64 row1_s = xt_convert24_short64(row1);

    int64 row2 = xt_mul24(x20, (char64)1);
    row2 = xt_mad24(x21, (char64)4, row2);
    row2 = xt_mad24(x22, (char64)6, row2);
    row2 = xt_mad24(x23, (char64)4, row2);
    row2 = xt_mad24(x24, (char64)1, row2);
    short64 row2_s = xt_convert24_short64(row2);

    for (int y = 0; y < inTileH; y += 2) {
    
      __local uchar *pSrc3 = local_src + (y + 3) * local_src_pitch + idx * 2;
      xtail = vload64(0, pSrc3);
      xtail1 = vload64(0, pSrc3 + 64);
      uchar64 x30 = shuffle2(xtail, xtail1, EvenMask);
      uchar64 x31 = shuffle2(xtail, xtail1, OddMask);
      xtail = vload64(0, pSrc3 + 2);
      xtail1 = vload64(0, pSrc3 + 64 + 2);  
      uchar64 x32 = shuffle2(xtail, xtail1, EvenMask);
      uchar64 x33 = shuffle2(xtail, xtail1, OddMask);
      xtail = vload64(0, pSrc3 + 4);
      xtail1 = vload64(0, pSrc3 + 64 + 4);  
      uchar64 x34 = shuffle2(xtail, xtail1, EvenMask);

      int64 row3 = xt_mul24(x30, (char64)1);
      row3 = xt_mad24(x31, (char64)4, row3);
      row3 = xt_mad24(x32, (char64)6, row3);
      row3 = xt_mad24(x33, (char64)4, row3);
      row3 = xt_mad24(x34, (char64)1, row3);
      short64 row3_s = xt_convert24_short64(row3);
  
      __local uchar *pSrc4 = local_src + (y + 4) * local_src_pitch + idx * 2;
      xtail = vload64(0, pSrc4);
      xtail1 = vload64(0, pSrc4 + 64);
      uchar64 x40 = shuffle2(xtail, xtail1, EvenMask);
      uchar64 x41 = shuffle2(xtail, xtail1, OddMask);
      xtail = vload64(0, pSrc4 + 2);
      xtail1 = vload64(0, pSrc4 + 64 + 2);  
      uchar64 x42 = shuffle2(xtail, xtail1, EvenMask);
      uchar64 x43 = shuffle2(xtail, xtail1, OddMask);
      xtail = vload64(0, pSrc4 + 4);
      xtail1 = vload64(0, pSrc4 + 64 + 4);  
      uchar64 x44 = shuffle2(xtail, xtail1, EvenMask);

      int64 row4 = xt_mul24(x40, (char64)1);
      row4 = xt_mad24(x41, (char64)4, row4);
      row4 = xt_mad24(x42, (char64)6, row4);
      row4 = xt_mad24(x43, (char64)4, row4);
      row4 = xt_mad24(x44, (char64)1, row4);
      short64 row4_s = xt_convert24_short64(row4);

      int64 result = xt_mul24(row0_s+row4_s, (char64)1);
      result = xt_mad24(row1_s+row3_s, (char64)4, result);
      result = xt_mad24(row2_s, (char64)6, result);

      *((__local uchar64 *)(local_dst + (y>>1) * outTileW + idx)) =
          xt_convert24_uchar64_sat_rnd(result, 8);

      row0_s = row2_s;
      row1_s = row3_s;
      row2_s = row4_s;
    }
  }
