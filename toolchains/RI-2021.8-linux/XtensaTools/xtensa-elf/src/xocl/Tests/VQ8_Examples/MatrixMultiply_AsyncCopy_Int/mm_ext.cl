  int ic;
  int ib;
  int kb;
  __local uchar128 *local_B_vec = (__local uchar128 *)local_B;
  __local int128 *local_C_vec = (__local int128 *)local_C;

  int128 vecC0, vecC1;

  for (ic = 0; ic < 128; ic+=2) {
    vecC0 = vecC1 = 0;
    for (kb = 0; kb < heightB; kb+=8) {
      uchar128 B00 = local_B_vec[kb];
      uchar A00 = local_A[ic * heightB + kb];
      uchar A01 = local_A[(ic+1) * heightB + kb];
      vecC0 = xt_mad32((uchar128)A00, B00, vecC0);
      vecC1 = xt_mad32((uchar128)A01, B00, vecC1);

      uchar128 B10 = local_B_vec[kb+1];
      uchar A10 = local_A[ic * heightB + kb+1];
      uchar A11 = local_A[(ic+1) * heightB + kb+1];
      vecC0 = xt_mad32((uchar128)A10, B10, vecC0);
      vecC1 = xt_mad32((uchar128)A11, B10, vecC1);

      uchar128 B20 = local_B_vec[kb+2];
      uchar A20 = local_A[ic * heightB + kb+2];
      uchar A21 = local_A[(ic+1) * heightB + kb+2];
      vecC0 = xt_mad32((uchar128)A20, B20, vecC0);
      vecC1 = xt_mad32((uchar128)A21, B20, vecC1);

      uchar128 B30 = local_B_vec[kb+3];
      uchar A30 = local_A[ic * heightB + kb+3];
      uchar A31 = local_A[(ic+1) * heightB + kb+3];
      vecC0 = xt_mad32((uchar128)A30, B30, vecC0);
      vecC1 = xt_mad32((uchar128)A31, B30, vecC1);

      uchar128 B40 = local_B_vec[kb+4];
      uchar A40 = local_A[ic * heightB + kb+4];
      uchar A41 = local_A[(ic+1) * heightB + kb+4];
      vecC0 = xt_mad32((uchar128)A40, B40, vecC0);
      vecC1 = xt_mad32((uchar128)A41, B40, vecC1);

      uchar128 B50 = local_B_vec[kb+5];
      uchar A50 = local_A[ic * heightB + kb+5];
      uchar A51 = local_A[(ic+1) * heightB + kb+5];
      vecC0 = xt_mad32((uchar128)A50, B50, vecC0);
      vecC1 = xt_mad32((uchar128)A51, B50, vecC1);

      uchar128 B60 = local_B_vec[kb+6];
      uchar A60 = local_A[ic * heightB + kb+6];
      uchar A61 = local_A[(ic+1) * heightB + kb+6];
      vecC0 = xt_mad32((uchar128)A60, B60, vecC0);
      vecC1 = xt_mad32((uchar128)A61, B60, vecC1);

      uchar128 B70 = local_B_vec[kb+7];
      uchar A70 = local_A[ic * heightB + kb+7];
      uchar A71 = local_A[(ic+1) * heightB + kb+7];
      vecC0 = xt_mad32((uchar128)A70, B70, vecC0);
      vecC1 = xt_mad32((uchar128)A71, B70, vecC1);
    }
    local_C_vec[ic] = vecC0;
    local_C_vec[ic+1] = vecC1;
  }
