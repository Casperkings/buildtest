   const uchar128 EvenMask = {
      0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 
      38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62, 64, 66, 68, 70, 72, 
      74, 76, 78, 80, 82, 84, 86, 88, 90, 92, 94, 96, 98, 100, 102, 104, 106, 
      108, 110, 112, 114, 116, 118, 120, 122, 124, 126, 128, 130, 132, 134, 
      136, 138, 140, 142, 144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 
      164, 166, 168, 170, 172, 174, 176, 178, 180, 182, 184, 186, 188, 190, 
      192, 194, 196, 198, 200, 202, 204, 206, 208, 210, 212, 214, 216, 218, 
      220, 222, 224, 226, 228, 230, 232, 234, 236, 238, 240, 242, 244, 246, 
      248, 250, 252, 254};
   
   const uchar128 OddMask = EvenMask + (uchar128)1;

  for (idx = 0; idx < outTileW; idx += 128) {
    int y = 0;
    __local uchar *pSrc0 = local_src + y * local_src_pitch + idx * 2;
    uchar128 xtail = vload128(0, pSrc0);
    uchar128 xtail1 = vload128(0, pSrc0 + 128);
    uchar128 x00 = shuffle2(xtail, xtail1, EvenMask);
    uchar128 x01 = shuffle2(xtail, xtail1, OddMask);
    xtail = vload128(0, pSrc0 + 2);
    xtail1 = vload128(0, pSrc0 + 128 + 2);  
    uchar128 x02 = shuffle2(xtail, xtail1, EvenMask);
    uchar128 x03 = shuffle2(xtail, xtail1, OddMask);
    xtail = vload128(0, pSrc0 + 4);
    xtail1 = vload128(0, pSrc0 + 128 + 4);  
    uchar128 x04 = shuffle2(xtail, xtail1, EvenMask);

    __local uchar *pSrc1 = local_src + (y + 1) * local_src_pitch + idx * 2;

    xtail = vload128(0, pSrc1);
    xtail1 = vload128(0, pSrc1 + 128);
    uchar128 x10 = shuffle2(xtail, xtail1, EvenMask);
    uchar128 x11 = shuffle2(xtail, xtail1, OddMask);
    xtail = vload128(0, pSrc1 + 2);
    xtail1 = vload128(0, pSrc1 + 128 + 2);  
    uchar128 x12 = shuffle2(xtail, xtail1, EvenMask);
    uchar128 x13 = shuffle2(xtail, xtail1, OddMask);
    xtail = vload128(0, pSrc1 + 4);
    xtail1 = vload128(0, pSrc1 + 128 + 4);  
    uchar128 x14 = shuffle2(xtail, xtail1, EvenMask);

    __local uchar *pSrc2 = local_src + (y + 2) * local_src_pitch + idx * 2;
    xtail = vload128(0, pSrc2);
    xtail1 = vload128(0, pSrc2 + 128);
    uchar128 x20 = shuffle2(xtail, xtail1, EvenMask);
    uchar128 x21 = shuffle2(xtail, xtail1, OddMask);
    xtail = vload128(0, pSrc2 + 2);
    xtail1 = vload128(0, pSrc2 + 128 + 2);  
    uchar128 x22 = shuffle2(xtail, xtail1, EvenMask);
    uchar128 x23 = shuffle2(xtail, xtail1, OddMask);
    xtail = vload128(0, pSrc2 + 4);
    xtail1 = vload128(0, pSrc2 + 128 + 4);  
    uchar128 x24 = shuffle2(xtail, xtail1, EvenMask);
  
    int128 row0 = xt_mul32(x00, (char128)1);
    row0 = xt_mad32(x01, (char128)4, row0);
    row0 = xt_mad32(x02, (char128)6, row0);
    row0 = xt_mad32(x03, (char128)4, row0);
    row0 = xt_mad32(x04, (char128)1, row0);
    short128 row0_s = convert_short128(row0);

    int128 row1 = xt_mul32(x10, (char128)1);
    row1 = xt_mad32(x11, (char128)4, row1);
    row1 = xt_mad32(x12, (char128)6, row1);
    row1 = xt_mad32(x13, (char128)4, row1);
    row1 = xt_mad32(x14, (char128)1, row1);
    short128 row1_s = convert_short128(row1);

    int128 row2 = xt_mul32(x20, (char128)1);
    row2 = xt_mad32(x21, (char128)4, row2);
    row2 = xt_mad32(x22, (char128)6, row2);
    row2 = xt_mad32(x23, (char128)4, row2);
    row2 = xt_mad32(x24, (char128)1, row2);
    short128 row2_s = convert_short128(row2);

    for (int y = 0; y < inTileH; y += 2) {
    
      __local uchar *pSrc3 = local_src + (y + 3) * local_src_pitch + idx * 2;
      xtail = vload128(0, pSrc3);
      xtail1 = vload128(0, pSrc3 + 128);
      uchar128 x30 = shuffle2(xtail, xtail1, EvenMask);
      uchar128 x31 = shuffle2(xtail, xtail1, OddMask);
      xtail = vload128(0, pSrc3 + 2);
      xtail1 = vload128(0, pSrc3 + 128 + 2);  
      uchar128 x32 = shuffle2(xtail, xtail1, EvenMask);
      uchar128 x33 = shuffle2(xtail, xtail1, OddMask);
      xtail = vload128(0, pSrc3 + 4);
      xtail1 = vload128(0, pSrc3 + 128 + 4);  
      uchar128 x34 = shuffle2(xtail, xtail1, EvenMask);

      int128 row3 = xt_mul32(x30, (char128)1);
      row3 = xt_mad32(x31, (char128)4, row3);
      row3 = xt_mad32(x32, (char128)6, row3);
      row3 = xt_mad32(x33, (char128)4, row3);
      row3 = xt_mad32(x34, (char128)1, row3);
      short128 row3_s = convert_short128(row3);
  
      __local uchar *pSrc4 = local_src + (y + 4) * local_src_pitch + idx * 2;
      xtail = vload128(0, pSrc4);
      xtail1 = vload128(0, pSrc4 + 128);
      uchar128 x40 = shuffle2(xtail, xtail1, EvenMask);
      uchar128 x41 = shuffle2(xtail, xtail1, OddMask);
      xtail = vload128(0, pSrc4 + 2);
      xtail1 = vload128(0, pSrc4 + 128 + 2);  
      uchar128 x42 = shuffle2(xtail, xtail1, EvenMask);
      uchar128 x43 = shuffle2(xtail, xtail1, OddMask);
      xtail = vload128(0, pSrc4 + 4);
      xtail1 = vload128(0, pSrc4 + 128 + 4);  
      uchar128 x44 = shuffle2(xtail, xtail1, EvenMask);

      int128 row4 = xt_mul32(x40, (char128)1);
      row4 = xt_mad32(x41, (char128)4, row4);
      row4 = xt_mad32(x42, (char128)6, row4);
      row4 = xt_mad32(x43, (char128)4, row4);
      row4 = xt_mad32(x44, (char128)1, row4);
      short128 row4_s = convert_short128(row4);

      int128 result = xt_mul32(row0_s+row4_s, (char128)1);
      result = xt_mad32(row1_s+row3_s, (char128)4, result);
      result = xt_mad32(row2_s, (char128)6, result);

      *((__local uchar128 *)(local_dst + (y>>1) * outTileW + idx)) =
          xt_convert_uchar128_sat_rnd(result, 8);

      row0_s = row2_s;
      row1_s = row3_s;
      row2_s = row4_s;
    }
  }
