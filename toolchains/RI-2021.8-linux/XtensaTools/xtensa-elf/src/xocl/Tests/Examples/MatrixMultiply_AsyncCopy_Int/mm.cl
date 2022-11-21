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
  int localWidthB = 64;
  int localWidthC = 64 * sizeof(int);

  i = get_global_id(0) * 64;
  j = get_global_id(1) * 64;

  const ushort32 maskinterleavea = {
    0, 32, 1, 33, 2,  34, 3,  35, 4,  36, 5,  37, 6,  38, 7,  39,
    8, 40, 9, 41, 10, 42, 11, 43, 12, 44, 13, 45, 14, 46, 15, 47};

  const ushort32 maskinterleaveb = {
    16, 48, 17, 49, 18, 50, 19, 51, 20, 52, 21, 53, 22, 54, 23, 55,
    24, 56, 25, 57, 26, 58, 27, 59, 28, 60, 29, 61, 30, 62, 31, 63};

  event_t evtA =
      xt_async_work_group_2d_copy(local_A, matA + (i * widthA), localWidthA, 64,
                                  localWidthA, widthA * sizeof(char), 0);

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
  uint32 vecC0, vecC1;
  int ic;
  int ib;
  __local uchar32 *local_B_vec = (__local uchar32 *)local_B;
  __local uint32 *local_C_vec = (__local uint32 *)local_C;

  for (ic = 0; ic < 64; ic++) {
    int kb = 0;
    vecC0 = 0;
    vecC1 = 0;

    for (kb = 0; kb < heightB; kb += 2) {
      ushort32 B00 = convert_ushort32(local_B_vec[2 * kb]);
      ushort32 B01 = convert_ushort32(local_B_vec[2 * kb + 1]);

      ushort32 A00 = local_A[ic * heightB + kb];
      ushort32 A01 = local_A[ic * heightB + kb + 1];

      ushort32 I1 = A00 * B00;
      ushort32 I2 = A00 * B01;
      vecC0 += convert_uint32(I1);
      vecC1 += convert_uint32(I2);

      B00 = convert_ushort32(local_B_vec[2 * (kb + 1)]);
      B01 = convert_ushort32(local_B_vec[2 * (kb + 1) + 1]);

      I1 = A01 * B00;
      I2 = A01 * B01;
      vecC0 += convert_uint32(I1);
      vecC1 += convert_uint32(I2);
    }

    local_C_vec[(2 * ic)] = vecC0;
    local_C_vec[(2 * ic) + 1] = vecC1;
  }
#endif
  event_t evtC = xt_async_work_group_2d_copy(
      (__global short *)(matC + (i * widthC + j) * sizeof(int)),
      (__local short *)local_C, localWidthC, 64, widthC * sizeof(int),
      localWidthC, 0);
  wait_group_events(1, &evtC);
}
