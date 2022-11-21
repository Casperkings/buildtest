  const uint16 intlv32A = {0,  2,  4,  6,  8,  10, 12, 14,
                           16, 18, 20, 22, 24, 26, 28, 30};
  const uint16 intlv32B = {1,  3,  5,  7,  9,  11, 13, 15,
                           17, 19, 21, 23, 25, 27, 29, 31};

  __local int *A_re = local_src;
  __local int *A_im = local_src + 128;

  for (int x = 0; x < 64; x += 16) {
    short vecWR0, vecWI0, vecWR1, vecWR2, vecWR3, vecWI1, vecWI2, vecWI3;
    int16 vec0, vec1, vec2, vec3;
    int16 vecD0, vecD1, vecD2, vecD3;
    int16 vecR0, vecR1, vecR2, vecR3, vecI0, vecI1, vecI2, vecI3;

    int16 vecOne = (int16)1;
    __constant short *pCoeff = W64;

    pvecOutR = (__local int16 *)(Temp_re0);
    pvecOutI = (__local int16 *)(Temp_im0);
    pvecIn = (__local int16 *)(A_re + (2 * x));

    vec0 = *pvecIn;
    pvecIn += 1;
    vec1 = *pvecIn;
    pvecIn += 127;
    vec2 = *pvecIn;
    pvecIn += 1;
    vec3 = *pvecIn;
    pvecIn += 127;
    vecD0 = *pvecIn;
    pvecIn += 1;
    vecD1 = *pvecIn;
    pvecIn += 127;
    vecD2 = *pvecIn;
    pvecIn += 1;
    vecD3 = *pvecIn;
    pvecIn += 127;

    vec0 = vec0 >> 2;
    vec1 = vec1 >> 2;
    vec2 = vec2 >> 2;
    vec3 = vec3 >> 2;
    vecD0 = vecD0 >> 2;
    vecD1 = vecD1 >> 2;
    vecD2 = vecD2 >> 2;
    vecD3 = vecD3 >> 2;

    vecR0 = shuffle2(vec0, vec1, intlv32A);
    vecI0 = shuffle2(vec0, vec1, intlv32B);

    vecR1 = shuffle2(vec2, vec3, intlv32A);
    vecI1 = shuffle2(vec2, vec3, intlv32B);

    vecR2 = shuffle2(vecD0, vecD1, intlv32A);
    vecI2 = shuffle2(vecD0, vecD1, intlv32B);

    vecR3 = shuffle2(vecD2, vecD3, intlv32A);
    vecI3 = shuffle2(vecD2, vecD3, intlv32B);

    vec0 = vecR0 + vecR2;
    vec1 = vecR1 + vecR3;
    vec2 = vecI0 + vecI2;
    vec3 = vecI1 + vecI3;
    vecD0 = vecR0 - vecR2;
    vecD1 = vecR1 - vecR3;
    vecD2 = vecI0 - vecI2;
    vecD3 = vecI1 - vecI3;

    *pvecOutR = (vec0 + vec1 + vecOne) >> 2;
    pvecOutR += 16;
    *pvecOutI = (vec2 + vec3 + vecOne) >> 2;
    pvecOutI += 16;
    *pvecOutR = (vecD0 + vecD3 + vecOne) >> 2;
    pvecOutR += 16;
    *pvecOutI = (vecD2 - vecD1 + vecOne) >> 2;
    pvecOutI += 16;
    *pvecOutR = (vec0 - vec1 + vecOne) >> 2;
    pvecOutR += 16;
    *pvecOutI = (vec2 - vec3 + vecOne) >> 2;
    pvecOutI += 16;
    *pvecOutR = (vecD0 - vecD3 + vecOne) >> 2;
    pvecOutR += 16;
    *pvecOutI = (vecD2 + vecD1 + vecOne) >> 2;
    pvecOutI += 16;

    for (q = 1; q < 16; q++) {
      int16 vecPR0, vecPR1, vecPR2, vecPR3, vecPI0, vecPI1, vecPI2, vecPI3;
      long16 acc0, acc1;
      int16 acc;

      pvecIn =
          (__local int16 *)((__local int *)A_re + (2 * x) + (q * stride * 2));
      pvecOutR = (__local int16 *)((__local int *)Temp_re0 + q * numElements);
      pvecOutI = (__local int16 *)((__local int *)Temp_im0 + q * numElements);

      vecWR1 = *pCoeff++; // W_re[q];
      vecWI1 = *pCoeff++; // W_im[q];
      vecWR2 = *pCoeff++; // W_re[2*q];
      vecWI2 = *pCoeff++; // W_im[2*q];
      vecWR3 = *pCoeff++; // W_re[3*q];
      vecWI3 = *pCoeff++; // W_im[3*q];

      vec0 = *pvecIn;
      pvecIn += 1;
      vec1 = *pvecIn;
      pvecIn += 127;
      vec2 = *pvecIn;
      pvecIn += 1;
      vec3 = *pvecIn;
      pvecIn += 127;
      vecD0 = *pvecIn;
      pvecIn += 1;
      vecD1 = *pvecIn;
      pvecIn += 127;
      vecD2 = *pvecIn;
      pvecIn += 1;
      vecD3 = *pvecIn;
      pvecIn += 127;

      vec0 = vec0 >> 2;
      vec1 = vec1 >> 2;
      vec2 = vec2 >> 2;
      vec3 = vec3 >> 2;
      vecD0 = vecD0 >> 2;
      vecD1 = vecD1 >> 2;
      vecD2 = vecD2 >> 2;
      vecD3 = vecD3 >> 2;

      vecR0 = shuffle2(vec0, vec1, intlv32A);
      vecI0 = shuffle2(vec0, vec1, intlv32B);

      vecR1 = shuffle2(vec2, vec3, intlv32A);
      vecI1 = shuffle2(vec2, vec3, intlv32B);

      vecR2 = shuffle2(vecD0, vecD1, intlv32A);
      vecI2 = shuffle2(vecD0, vecD1, intlv32B);

      vecR3 = shuffle2(vecD2, vecD3, intlv32A);
      vecI3 = shuffle2(vecD2, vecD3, intlv32B);

      vec0 = vecR0 + vecR2;
      vec1 = vecR1 + vecR3;
      vec2 = vecI0 + vecI2;
      vec3 = vecI1 + vecI3;
      vecD0 = vecR0 - vecR2;
      vecD1 = vecR1 - vecR3;
      vecD2 = vecI0 - vecI2;
      vecD3 = vecI1 - vecI3;
      vecPR0 = vec0 + vec1;
      vecPI0 = vec2 + vec3;
      vecPR1 = vecD0 + vecD3;
      vecPI1 = vecD2 - vecD1;
      vecPR2 = vec0 - vec1;
      vecPI2 = vec2 - vec3;
      vecPR3 = vecD0 - vecD3;
      vecPI3 = vecD2 + vecD1;

      *pvecOutR = (vecPR0 + vecOne) >> 2;
      pvecOutR += 16;
      *pvecOutI = (vecPI0 + vecOne) >> 2;
      pvecOutI += 16;

      acc0 = mul64(vecWR1, vecPR1);
      acc = mulspack(vecWI1, vecPI1, acc0);
      *pvecOutR = acc;
      pvecOutR += 16;
      acc1 = mul64(vecWR1, vecPI1);
      acc = mulapack(vecWI1, vecPR1, acc1);
      *pvecOutI = acc;
      pvecOutI += 16;

      acc0 = mul64(vecWR2, vecPR2);
      acc = mulspack(vecWI2, vecPI2, acc0);
      *pvecOutR = acc;
      pvecOutR += 16;
      acc1 = mul64(vecWR2, vecPI2);
      acc = mulapack(vecWI2, vecPR2, acc1);
      *pvecOutI = acc;
      pvecOutI += 16;

      acc0 = mul64(vecWR3, vecPR3);
      acc = mulspack(vecWI3, vecPI3, acc0);
      *pvecOutR = acc;
      pvecOutR += 16;
      acc1 = mul64(vecWR3, vecPI3);
      acc = mulapack(vecWI3, vecPR3, acc1);
      *pvecOutI = acc;
      pvecOutI += 16;
    }

    WPowerMultiplier = 4;
    // Stage 1
    for (group = 0, start_idx = 0; group < 4; group++, start_idx += 16) {
      pvecInR =
          (__local int16 *)((__local int *)Temp_re0 + start_idx * numElements);
      pvecInI =
          (__local int16 *)((__local int *)Temp_im0 + start_idx * numElements);
      pvecOutR =
          (__local int16 *)((__local int *)Temp_re1 + start_idx * numElements);
      pvecOutI =
          (__local int16 *)((__local int *)Temp_im1 + start_idx * numElements);

      vecR0 = *pvecInR;
      pvecInR += 4;
      vecR1 = *pvecInR;
      pvecInR += 4;
      vecR2 = *pvecInR;
      pvecInR += 4;
      vecR3 = *pvecInR;
      pvecInR += 4;
      vecI0 = *pvecInI;
      pvecInI += 4;
      vecI1 = *pvecInI;
      pvecInI += 4;
      vecI2 = *pvecInI;
      pvecInI += 4;
      vecI3 = *pvecInI;
      pvecInI += 4;

      vec0 = vecR0 + vecR2;
      vec1 = vecR1 + vecR3;
      vec2 = vecI0 + vecI2;
      vec3 = vecI1 + vecI3;
      vecD0 = vecR0 - vecR2;
      vecD1 = vecR1 - vecR3;
      vecD2 = vecI0 - vecI2;
      vecD3 = vecI1 - vecI3;

      *pvecOutR = (vec0 + vec1 + vecOne) >> 2;
      pvecOutR += 4;
      *pvecOutI = (vec2 + vec3 + vecOne) >> 2;
      pvecOutI += 4;
      *pvecOutR = (vecD0 + vecD3 + vecOne) >> 2;
      pvecOutR += 4;
      *pvecOutI = (vecD2 - vecD1 + vecOne) >> 2;
      pvecOutI += 4;
      *pvecOutR = (vec0 - vec1 + vecOne) >> 2;
      pvecOutR += 4;
      *pvecOutI = (vec2 - vec3 + vecOne) >> 2;
      pvecOutI += 4;
      *pvecOutR = (vecD0 - vecD3 + vecOne) >> 2;
      pvecOutR += 4;
      *pvecOutI = (vecD2 + vecD1 + vecOne) >> 2;
      pvecOutI += 4;

      for (q = 1; q < 4; q++) {
        int16 vecPR0, vecPR1, vecPR2, vecPR3, vecPI0, vecPI1, vecPI2, vecPI3;
        long16 acc0, acc1;
        int16 acc;
        pvecInR = (__local int16 *)((__local int *)Temp_re0 +
                                    start_idx * numElements + q * numElements);
        pvecInI = (__local int16 *)((__local int *)Temp_im0 +
                                    start_idx * numElements + q * numElements);
        pvecOutR = (__local int16 *)((__local int *)Temp_re1 +
                                     start_idx * numElements + q * numElements);
        pvecOutI = (__local int16 *)((__local int *)Temp_im1 +
                                     start_idx * numElements + q * numElements);
        vecWR1 = *pCoeff++; // W_re[q];
        vecWI1 = *pCoeff++; // W_im[q];
        vecWR2 = *pCoeff++; // W_re[2*q];
        vecWI2 = *pCoeff++; // W_im[2*q];
        vecWR3 = *pCoeff++; // W_re[3*q];
        vecWI3 = *pCoeff++; // W_im[3*q];

        vecR0 = *pvecInR;
        pvecInR += 4;
        vecR1 = *pvecInR;
        pvecInR += 4;
        vecR2 = *pvecInR;
        pvecInR += 4;
        vecR3 = *pvecInR;
        pvecInR += 4;
        vecI0 = *pvecInI;
        pvecInI += 4;
        vecI1 = *pvecInI;
        pvecInI += 4;
        vecI2 = *pvecInI;
        pvecInI += 4;
        vecI3 = *pvecInI;
        pvecInI += 4;

        vec0 = vecR0 + vecR2;
        vec1 = vecR1 + vecR3;
        vec2 = vecI0 + vecI2;
        vec3 = vecI1 + vecI3;
        vecD0 = vecR0 - vecR2;
        vecD1 = vecR1 - vecR3;
        vecD2 = vecI0 - vecI2;
        vecD3 = vecI1 - vecI3;
        vecPR0 = vec0 + vec1;
        vecPI0 = vec2 + vec3;
        vecPR1 = vecD0 + vecD3;
        vecPI1 = vecD2 - vecD1;
        vecPR2 = vec0 - vec1;
        vecPI2 = vec2 - vec3;
        vecPR3 = vecD0 - vecD3;
        vecPI3 = vecD2 + vecD1;

        *pvecOutR = (vecPR0 + vecOne) >> 2;
        pvecOutR += 4;
        *pvecOutI = (vecPI0 + vecOne) >> 2;
        pvecOutI += 4;

        acc0 = mul64(vecWR1, vecPR1);
        acc = mulspack(vecWI1, vecPI1, acc0);
        *pvecOutR = acc;
        pvecOutR += 4;
        acc1 = mul64(vecWR1, vecPI1);
        acc = mulapack(vecWI1, vecPR1, acc1);
        *pvecOutI = acc;
        pvecOutI += 4;

        acc0 = mul64(vecWR2, vecPR2);
        acc = mulspack(vecWI2, vecPI2, acc0);
        *pvecOutR = acc;
        pvecOutR += 4;
        acc1 = mul64(vecWR2, vecPI2);
        acc = mulapack(vecWI2, vecPR2, acc1);
        *pvecOutI = acc;
        pvecOutI += 4;

        acc0 = mul64(vecWR3, vecPR3);
        acc = mulspack(vecWI3, vecPI3, acc0);
        *pvecOutR = acc;
        pvecOutR += 4;
        acc1 = mul64(vecWR3, vecPI3);
        acc = mulapack(vecWI3, vecPR3, acc1);
        *pvecOutI = acc;
        pvecOutI += 4;
      }
    }

    // Stage 2

    // 16*16
    // 16:64 cycles
    // 8 store, 8 load, 8*3 adds, 8 shift, 4 out_idx loads
    for (group = 0, start_idx = 0; group < 16; group++, start_idx += 4) {
      pvecInR =
          (__local int16 *)((__local int *)Temp_re1 + start_idx * numElements);
      pvecInI =
          (__local int16 *)((__local int *)Temp_im1 + start_idx * numElements);
      pvecOutR = (__local int16 *)((__local int *)Out_re + x);
      pvecOutI = (__local int16 *)((__local int *)Out_im + x);

      vecR0 = *pvecInR;
      pvecInR += 1;
      vecR1 = *pvecInR;
      pvecInR += 1;
      vecR2 = *pvecInR;
      pvecInR += 1;
      vecR3 = *pvecInR;
      pvecInR += 1;
      vecI0 = *pvecInI;
      pvecInI += 1;
      vecI1 = *pvecInI;
      pvecInI += 1;
      vecI2 = *pvecInI;
      pvecInI += 1;
      vecI3 = *pvecInI;
      pvecInI += 1;

      vec0 = vecR0 + vecR2;
      vec1 = vecR1 + vecR3;
      vec2 = vecI0 + vecI2;
      vec3 = vecI1 + vecI3;
      vecD0 = vecR0 - vecR2;
      vecD1 = vecR1 - vecR3;
      vecD2 = vecI0 - vecI2;
      vecD3 = vecI1 - vecI3;

      *(pvecOutR + Out_idx[start_idx] * 4) = (vec0 + vec1);
      *(pvecOutI + Out_idx[start_idx] * 4) = (vec2 + vec3);
      *(pvecOutR + Out_idx[start_idx + 1] * 4) = (vecD0 + vecD3);
      *(pvecOutI + Out_idx[start_idx + 1] * 4) = (vecD2 - vecD1);
      *(pvecOutR + Out_idx[start_idx + 2] * 4) = (vec0 - vec1);
      *(pvecOutI + Out_idx[start_idx + 2] * 4) = (vec2 - vec3);
      *(pvecOutR + Out_idx[start_idx + 3] * 4) = (vecD0 - vecD3);
      *(pvecOutI + Out_idx[start_idx + 3] * 4) = (vecD2 + vecD1);
    }
  }
