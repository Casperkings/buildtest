__kernel void matMulAsync(__global uchar *restrict matA,
                          __global uchar *restrict matB,
                          __global uchar *restrict matC, int widthA, int widthB,
                          int widthC, __local uchar *restrict local_A,
                          __local uchar *restrict local_B,
                          __local unsigned int *restrict local_C) {
  int i, j;
  int N = widthC;
  int heightB = widthA;
  int localWidthA = widthA * sizeof(uchar);
  int localWidthB = 128;
  int localWidthC = 128 * sizeof(int);

  i = get_global_id(0) * 128;
  j = get_global_id(1) * 128;

  event_t evtA =
      xt_async_work_group_2d_copy(local_A, matA + (i * widthA), localWidthA, 
                                  128, localWidthA, widthA * sizeof(char), 0);

  event_t evtB =
      xt_async_work_group_2d_copy(local_B, (matB + j), localWidthB, heightB,
                                  localWidthB, widthB * sizeof(char), 0);

  wait_group_events(1, &evtA);
  wait_group_events(1, &evtB);

#if NATIVE_KERNEL
  #include "mm_ivp.c"
#elif XT_OCL_EXT
  #include "mm_ext.cl"
#else
  uint64 vecC0, vecC1;
  int ic;
  int ib;
  __local uchar64 *local_B_vec = (__local uchar64 *)local_B;
  __local uint64 *local_C_vec = (__local uint64 *)local_C;

  for (ic = 0; ic < 128; ic++) {
    int kb = 0;
    vecC0 = 0;
    vecC1 = 0;

    for (kb = 0; kb < heightB; kb += 2) {
      ushort64 B00 = convert_ushort64(local_B_vec[2 * kb]);
      ushort64 B01 = convert_ushort64(local_B_vec[2 * kb + 1]);

      ushort64 A00 = local_A[ic * heightB + kb];
      ushort64 A01 = local_A[ic * heightB + kb + 1];

      ushort64 I1 = A00 * B00;
      ushort64 I2 = A00 * B01;
      vecC0 += convert_uint64(I1);
      vecC1 += convert_uint64(I2);

      B00 = convert_ushort64(local_B_vec[2 * (kb + 1)]);
      B01 = convert_ushort64(local_B_vec[2 * (kb + 1) + 1]);

      I1 = A01 * B00;
      I2 = A01 * B01;
      vecC0 += convert_uint64(I1);
      vecC1 += convert_uint64(I2);
    }

    local_C_vec[(2 * ic)] = vecC0;
    local_C_vec[(2 * ic) + 1] = vecC1;
  }
#endif
  event_t evtC = xt_async_work_group_2d_copy(
      (__global short *)(matC + (i * widthC + j) * sizeof(int)),
      (__local short *)local_C, localWidthC, 128, widthC * sizeof(int),
      localWidthC, 0);
  wait_group_events(1, &evtC);
}
