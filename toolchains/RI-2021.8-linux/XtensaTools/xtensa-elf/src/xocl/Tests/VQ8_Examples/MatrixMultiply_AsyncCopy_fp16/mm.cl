__kernel void matMulAsync(__global float* restrict matA,
                          __global float64* restrict matB,
                          __global float64* restrict matC,
                          int widthA,
                          int widthB,
                          int widthC,
                          __local float* restrict local_A,
                          __local float64* restrict local_B)
{
  int i, j;
  int N = widthC;
  half64 vecA;
  half64 vecA1;
  int heightB = widthA;
  int localWidthA = widthA * sizeof(float);
  int localWidthB = 2 * sizeof(float64);
  int localWidthC = 2 * sizeof(float64);
  __local float64 local_C[128*2];

  i = get_global_id(0) * 128;
  j = get_global_id(1) * 2;

  event_t evtA = xt_async_work_group_2d_copy(
                       local_A, 
                       matA + (i * widthA),
                       localWidthA, 128,
                       localWidthA, widthA * sizeof(float), 0);
             
  event_t evtB = xt_async_work_group_2d_copy(
                       (__local float *) local_B,
                       (__global float *) (matB + j),
                       localWidthB, heightB,
                       localWidthB, widthB * sizeof(float), 0);

  wait_group_events(1, &evtA);
  wait_group_events(1, &evtB);

  int ic;
  int ib;
  for (ib = i, ic = 0; ib < i + 128; ib += 4, ic += 8) {
    half64 local_C0 = 0;
    half64 local_C1 = 0;
    half64 local_C2 = 0;
    half64 local_C3 = 0;
    half64 local_C4 = 0;
    half64 local_C5 = 0;
    half64 local_C6 = 0;
    half64 local_C7 = 0;

    int kb = 0;
    for (kb = 0; kb < heightB; kb += 2) {
      vecA  = local_A[(ib-i)*widthA+kb];
      vecA1 = local_A[(ib-i)*widthA+kb+1];
      local_C0 += (vecA * convert_half64(local_B[2*kb]));
      local_C0 += (vecA1 * convert_half64(local_B[2*(kb+1)]));
      local_C1 += (vecA * convert_half64(local_B[2*kb+1]));
      local_C1 += (vecA1 * convert_half64(local_B[2*(kb+1)+1]));
        
      vecA  = local_A[(ib+1-i)*widthA+kb];
      vecA1 = local_A[(ib+1-i)*widthA+kb+1];
      local_C2 += (vecA * convert_half64(local_B[2*kb]));
      local_C2 += (vecA1 * convert_half64(local_B[2*(kb+1)]));
      local_C3 += (vecA * convert_half64(local_B[2*kb+1]));
      local_C3 += (vecA1 * convert_half64(local_B[2*(kb+1)+1]));
        
      vecA  = local_A[(ib+2-i)*widthA+kb];
      vecA1 = local_A[(ib+2-i)*widthA+kb+1];
      local_C4 += (vecA * convert_half64(local_B[2*kb]));
      local_C4 += (vecA1 * convert_half64(local_B[2*(kb+1)]));
      local_C5 += (vecA * convert_half64(local_B[2*kb+1]));
      local_C5 += (vecA1 * convert_half64(local_B[2*(kb+1)+1]));
        
      vecA  = local_A[(ib+3-i)*widthA+kb];
      vecA1 = local_A[(ib+3-i)*widthA+kb+1];
      local_C6 += (vecA * convert_half64(local_B[2*kb]));
      local_C6 += (vecA1 * convert_half64(local_B[2*(kb+1)]));
      local_C7 += (vecA * convert_half64(local_B[2*kb+1]));
      local_C7 += (vecA1 * convert_half64(local_B[2*(kb+1)+1]));
    }

    local_C[ic] = convert_float64(local_C0);
    local_C[ic + 1] = convert_float64(local_C1);
    local_C[ic + 2] = convert_float64(local_C2);
    local_C[ic + 3] = convert_float64(local_C3);
    local_C[ic + 4] = convert_float64(local_C4);
    local_C[ic + 5] = convert_float64(local_C5);
    local_C[ic + 6] = convert_float64(local_C6);
    local_C[ic + 7] = convert_float64(local_C7);
  }


  event_t evtC = xt_async_work_group_2d_copy(
                       (__global float *) (matC + (widthC/64*i+j)),
                       (__local float *) local_C,
                       localWidthC, 128,
                       widthC * sizeof(float), localWidthC, 0);
  wait_group_events(1, &evtC);
}
