  int ic;
  int ib;
  int kb;
  __local uchar64 *local_B_vec = (__local uchar64 *)local_B;
  __local int64 *local_C_vec = (__local int64 *)local_C;

  int64 vecC0, vecC1;

  for (ic = 0; ic < 64; ic+=2) {
    vecC0 = vecC1 = 0;
    // Assumes the below loop will not exceed 24-bit accumulation
#pragma unroll 8
    for (kb = 0; kb < heightB; kb++) {
      uchar64 B00 = local_B_vec[kb];
      uchar A00 = local_A[ic * heightB + kb];
      uchar A01 = local_A[(ic+1) * heightB + kb];
      vecC0 = xt_mad24((uchar64)A00, B00, vecC0);
      vecC1 = xt_mad24((uchar64)A01, B00, vecC1);
    }
    local_C_vec[ic] = vecC0;
    local_C_vec[ic+1] = vecC1;
  }
