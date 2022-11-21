  const uint16 vec_sel0 = {0, 1, 2, 3, 16, 17, 18, 19,
                           4, 5, 6, 7, 20, 21, 22, 23};
  uint16 vec_sel1 = vec_sel0 + (uint16)8;

  const uint16 vec_sel2 = {0,  1,  2,  3,  4,  5,  6,  7,
                           16, 17, 18, 19, 20, 21, 22, 23};
  uint16 vec_sel3 = vec_sel2 + (uint16)8;

  const uint16 vec_sel4 = {0, 4, 8,  12, 1, 5, 9,  13,
                           2, 6, 10, 14, 3, 7, 11, 15};
  uint16 vec_sel5 = vec_sel4 + (uint16)2;

  const uint16 vec_sel6 = {0, 1, 16, 17, 2, 3, 18, 19,
                           4, 5, 20, 21, 6, 7, 22, 23};
  uint16 vec_sel7 = vec_sel6 + (uint16)8;

  const uint16 vec_sel8 = {0, 16, 4, 20, 8, 24, 12, 28,
                           1, 17, 5, 21, 9, 25, 13, 29};
  uint16 vec_sel9 = vec_sel8 + (uint16)2;

  const uint16 vec_sel10 = {0, 1, 16, 17, 2, 3, 18, 19,
                            4, 5, 20, 21, 6, 7, 22, 23};
  const uint16 vec_sel11 = {8,  9,  24, 25, 10, 11, 26, 27,
                            12, 13, 28, 29, 14, 15, 30, 31};

  __local int16 *pvec_inp0 = (__local int16 *)local_srcRe;
  __local int16 *pvec_inp1 = (__local int16 *)local_srcIm;
  __local int16 *pvec_out0 = (__local int16 *)Temp;

  __constant short *tw_ptr_s = (__constant short *)tw_ptr;
  __constant short32 *pvec_tw0;
  int16 vec_inp0, vec_inp1, vec_inp2, vec_inp3, vec_inp4, vec_inp5, vec_inp6,
      vec_inp7;
  int16 vec_data0, vec_data1, vec_data2, vec_data3, vec_data4, vec_data5,
      vec_data6, vec_data7;
  int16 vec_data00, vec_data01, vec_data10, vec_data11, vec_data20, vec_data21,
      vec_data30, vec_data31;
  short32 vec_tw0;
  int16 vec_temp1, vec_temp2;
  for (int iy = 0; iy < 64; iy++) {
    vec_inp0 = *pvec_inp0;
    pvec_inp0 += 1;
    vec_inp2 = *pvec_inp0;
    pvec_inp0 += 1;
    vec_inp4 = *pvec_inp0;
    pvec_inp0 += 1;
    vec_inp6 = *pvec_inp0;
    pvec_inp0 += 1;

    vec_inp1 = *pvec_inp1;
    pvec_inp1 += 1;
    vec_inp3 = *pvec_inp1;
    pvec_inp1 += 1;
    vec_inp5 = *pvec_inp1;
    pvec_inp1 += 1;
    vec_inp7 = *pvec_inp1;
    pvec_inp1 += 1;

    vec_inp0 = vec_inp0 >> 2;
    vec_inp1 = vec_inp1 >> 2;
    vec_inp2 = vec_inp2 >> 2;
    vec_inp3 = vec_inp3 >> 2;
    vec_inp4 = vec_inp4 >> 2;
    vec_inp5 = vec_inp5 >> 2;
    vec_inp6 = vec_inp6 >> 2;
    vec_inp7 = vec_inp7 >> 2;

    RADIX4_BTRFLY_ROW0_ROW2_32Bit(vec_inp0, vec_inp2, vec_inp4, vec_inp6,
                                  &vec_data00, &vec_data20, &vec_data1,
                                  &vec_data2); // mul factors are 1  1  1  1
    RADIX4_BTRFLY_ROW0_ROW2_32Bit(vec_inp1, vec_inp3, vec_inp5, vec_inp7,
                                  &vec_data01, &vec_data21, &vec_data1,
                                  &vec_data2); // mul factors are 1  1  1  1
    vec_data00 = vec_data00 >> 2;
    vec_data01 = vec_data01 >> 2;

    RADIX4_BTRFLY_ROW1_32Bit_real(vec_inp0, vec_inp3, vec_inp4, vec_inp7,
                                  &vec_data10,
                                  &vec_data1); // mul factors are 1 -j -1  j
    RADIX4_BTRFLY_ROW1_32Bit_imag(vec_inp1, vec_inp2, vec_inp5, vec_inp6,
                                  &vec_data11,
                                  &vec_data2); // mul factors are 1 -j -1  j
    pvec_tw0 = (__constant short32 *)tw_ptr_s;
    vec_tw0 = vload32(0, (__constant short *)pvec_tw0);
    pvec_tw0++;
    CMPLX_MUL_32_16Bit(vec_tw0, &vec_data10, &vec_data11);

    vec_tw0 = vload32(0, (__constant short *)pvec_tw0);
    pvec_tw0++;
    CMPLX_MUL_32_16Bit(vec_tw0, &vec_data20, &vec_data21);

    // mul factors are 1 -j -1  j
    RADIX4_BTRFLY_ROW3_32Bit_real(vec_inp0, vec_inp3, vec_inp4, vec_inp7,
                                  &vec_data30, &vec_data1);
    // mul factors are 1 -j -1  j
    RADIX4_BTRFLY_ROW3_32Bit_imag(vec_inp1, vec_inp2, vec_inp5, vec_inp6,
                                  &vec_data31, &vec_data2);

    vec_tw0 = vload32(0, (__constant short *)pvec_tw0);
    pvec_tw0++;
    CMPLX_MUL_32_16Bit(vec_tw0, &vec_data30, &vec_data31);

    vec_data0 = shuffle2(vec_data00, vec_data10, vec_sel0);
    vec_data1 = shuffle2(vec_data00, vec_data10, vec_sel1);
    vec_data2 = shuffle2(vec_data20, vec_data30, vec_sel0);
    vec_data3 = shuffle2(vec_data20, vec_data30, vec_sel1);
    vec_data00 = shuffle2(vec_data0, vec_data2, vec_sel2);
    vec_data10 = shuffle2(vec_data0, vec_data2, vec_sel3);
    vec_data20 = shuffle2(vec_data1, vec_data3, vec_sel2);
    vec_data30 = shuffle2(vec_data1, vec_data3, vec_sel3);
    vec_data0 = shuffle2(vec_data01, vec_data11, vec_sel0);
    vec_data1 = shuffle2(vec_data01, vec_data11, vec_sel1);
    vec_data2 = shuffle2(vec_data21, vec_data31, vec_sel0);
    vec_data3 = shuffle2(vec_data21, vec_data31, vec_sel1);
    vec_data01 = shuffle2(vec_data0, vec_data2, vec_sel2);
    vec_data11 = shuffle2(vec_data0, vec_data2, vec_sel3);
    vec_data21 = shuffle2(vec_data1, vec_data3, vec_sel2);
    vec_data31 = shuffle2(vec_data1, vec_data3, vec_sel3);

    RADIX4_BTRFLY_ROW0_ROW2_32Bit(vec_data00, vec_data10, vec_data20,
                                  vec_data30, &vec_inp0, &vec_inp4, &vec_temp1,
                                  &vec_temp2);
    RADIX4_BTRFLY_ROW0_ROW2_32Bit(vec_data01, vec_data11, vec_data21,
                                  vec_data31, &vec_inp1, &vec_inp5, &vec_temp1,
                                  &vec_temp2);
    RADIX4_BTRFLY_ROW1_32Bit_real(vec_data00, vec_data11, vec_data20,
                                  vec_data31, &vec_inp2, &vec_temp1);
    RADIX4_BTRFLY_ROW1_32Bit_imag(vec_data01, vec_data10, vec_data21,
                                  vec_data30, &vec_inp3, &vec_temp2);
    RADIX4_BTRFLY_ROW3_32Bit_real(vec_data00, vec_data11, vec_data20,
                                  vec_data31, &vec_inp6,
                                  &vec_temp1); // mul factors are 1 -j -1  j
    RADIX4_BTRFLY_ROW3_32Bit_imag(vec_data01, vec_data10, vec_data21,
                                  vec_data30, &vec_inp7,
                                  &vec_temp2); // mul factors are 1 -j -1  j
    *pvec_out0 = vec_inp0;
    pvec_out0 += 1;
    *pvec_out0 = vec_inp1;
    pvec_out0 += 1;
    *pvec_out0 = vec_inp2;
    pvec_out0 += 1;
    *pvec_out0 = vec_inp3;
    pvec_out0 += 1;
    *pvec_out0 = vec_inp4;
    pvec_out0 += 1;
    *pvec_out0 = vec_inp5;
    pvec_out0 += 1;
    *pvec_out0 = vec_inp6;
    pvec_out0 += 1;
    *pvec_out0 = vec_inp7;
    pvec_out0 += 1;
  }

  pvec_inp0 = (__local int16 *)Temp;
  pvec_out0 = (__local int16 *)local_dst;

  for (int iy = 0; iy < 64; iy++) {
    vec_inp0 = *pvec_inp0;
    pvec_inp0++;
    vec_inp1 = *pvec_inp0;
    pvec_inp0++;
    vec_inp2 = *pvec_inp0;
    pvec_inp0++;
    vec_inp3 = *pvec_inp0;
    pvec_inp0++;
    vec_inp4 = *pvec_inp0;
    pvec_inp0++;
    vec_inp5 = *pvec_inp0;
    pvec_inp0++;
    vec_inp6 = *pvec_inp0;
    pvec_inp0++;
    vec_inp7 = *pvec_inp0;
    pvec_inp0++;

    vec_inp0 = vec_inp0 >> 2;
    vec_inp1 = vec_inp1 >> 2;

    pvec_tw0 = (__constant short32 *)(tw_ptr_s + 96);

    vec_tw0 = *pvec_tw0;
    pvec_tw0++;
    CMPLX_MUL_32_16Bit(vec_tw0, &vec_inp2, &vec_inp3);

    vec_tw0 = *pvec_tw0;
    pvec_tw0++;
    CMPLX_MUL_32_16Bit(vec_tw0, &vec_inp4, &vec_inp5);

    vec_tw0 = *pvec_tw0;
    pvec_tw0++;
    CMPLX_MUL_32_16Bit(vec_tw0, &vec_inp6, &vec_inp7);

    vec_data0 = shuffle2(vec_inp0, vec_inp2, vec_sel8);
    vec_data2 = shuffle2(vec_inp0, vec_inp2, vec_sel9);
    vec_data4 = shuffle2(vec_inp4, vec_inp6, vec_sel8);
    vec_data6 = shuffle2(vec_inp4, vec_inp6, vec_sel9);

    vec_inp0 = shuffle2(vec_data0, vec_data4, vec_sel10);
    vec_inp2 = shuffle2(vec_data0, vec_data4, vec_sel11);

    vec_inp4 = shuffle2(vec_data2, vec_data6, vec_sel10);
    vec_inp6 = shuffle2(vec_data2, vec_data6, vec_sel11);

    vec_data1 = shuffle2(vec_inp1, vec_inp3, vec_sel8);
    vec_data3 = shuffle2(vec_inp1, vec_inp3, vec_sel9);
    vec_data5 = shuffle2(vec_inp5, vec_inp7, vec_sel8);
    vec_data7 = shuffle2(vec_inp5, vec_inp7, vec_sel9);

    vec_inp1 = shuffle2(vec_data1, vec_data5, vec_sel10);
    vec_inp3 = shuffle2(vec_data1, vec_data5, vec_sel11);

    vec_inp5 = shuffle2(vec_data3, vec_data7, vec_sel10);
    vec_inp7 = shuffle2(vec_data3, vec_data7, vec_sel11);

    RADIX4_BTRFLY_ROW0_ROW2_32Bit(vec_inp0, vec_inp2, vec_inp4, vec_inp6,
                                  &vec_data00, &vec_data20, &vec_data1,
                                  &vec_data2);
    RADIX4_BTRFLY_ROW0_ROW2_32Bit(vec_inp1, vec_inp3, vec_inp5, vec_inp7,
                                  &vec_data01, &vec_data21, &vec_data1,
                                  &vec_data2);

    RADIX4_BTRFLY_ROW1_32Bit_real(vec_inp0, vec_inp3, vec_inp4, vec_inp7,
                                  &vec_data10, &vec_data1);
    RADIX4_BTRFLY_ROW1_32Bit_imag(vec_inp1, vec_inp2, vec_inp5, vec_inp6,
                                  &vec_data11, &vec_data2);
    RADIX4_BTRFLY_ROW3_32Bit_real(vec_inp0, vec_inp3, vec_inp4, vec_inp7,
                                  &vec_data30,
                                  &vec_data1); // mul factors are 1 -j -1  j
    RADIX4_BTRFLY_ROW3_32Bit_imag(vec_inp1, vec_inp2, vec_inp5, vec_inp6,
                                  &vec_data31,
                                  &vec_data2); // mul factors are 1 -j -1  j

    vec_data0 = shuffle2(vec_data00, vec_data01, vec_sel8);
    vec_data1 = shuffle2(vec_data00, vec_data01, vec_sel9);
    vec_data2 = shuffle2(vec_data10, vec_data11, vec_sel8);
    vec_data3 = shuffle2(vec_data10, vec_data11, vec_sel9);
    vec_data4 = shuffle2(vec_data20, vec_data21, vec_sel8);
    vec_data5 = shuffle2(vec_data20, vec_data21, vec_sel9);
    vec_data6 = shuffle2(vec_data30, vec_data31, vec_sel8);
    vec_data7 = shuffle2(vec_data30, vec_data31, vec_sel9);

    *pvec_out0 = vec_data0;
    pvec_out0 += 1;
    *pvec_out0 = vec_data1;
    pvec_out0 += 1;
    *pvec_out0 = vec_data2;
    pvec_out0 += 1;
    *pvec_out0 = vec_data3;
    pvec_out0 += 1;
    *pvec_out0 = vec_data4;
    pvec_out0 += 1;
    *pvec_out0 = vec_data5;
    pvec_out0 += 1;
    *pvec_out0 = vec_data6;
    pvec_out0 += 1;
    *pvec_out0 = vec_data7;
    pvec_out0 += 1;
  }
