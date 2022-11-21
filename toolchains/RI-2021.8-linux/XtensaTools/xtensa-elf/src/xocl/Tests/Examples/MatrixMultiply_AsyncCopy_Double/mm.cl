__kernel void matMulAsync(__global double* restrict matA,
                          __global double8* restrict matB,
                          __global double8* restrict matC,
                          int widthA,
                          int widthB,
                          int widthC,
                          __local double* restrict local_A,
                          __local double8* restrict local_B)
{
  int i, j;
  int N = widthC;
  double8 vecA;
  double8 vecA1;
  int heightB = widthA;
  int localWidthA = widthA * sizeof(double);
  int localWidthB = 2 * sizeof(double8);
  int localWidthC = 2 * sizeof(double8);
  __local double8 local_C[16*2];

  i = get_global_id(0) * 16;
  j = get_global_id(1) * 2;

  event_t evtA = xt_async_work_group_2d_copy(
                       local_A, 
                       matA + (i * widthA),
                       localWidthA, 16,
                       localWidthA, widthA * sizeof(double), 0);
             
  event_t evtB = xt_async_work_group_2d_copy(
                       local_B,
                       matB + j,
                       localWidthB, heightB,
                       localWidthB, widthB * sizeof(double), 0);

  wait_group_events(1, &evtA);
  wait_group_events(1, &evtB);

  int ic;
  int ib;
  for (ib = i, ic = 0; ib < i + 16; ib += 4, ic += 8) {
    double8 local_C_tmp0 = 0;
    double8 local_C_tmp1 = 0;
    double8 local_C_tmp2 = 0;
    double8 local_C_tmp3 = 0;
    double8 local_C_tmp4 = 0;
    double8 local_C_tmp5 = 0;
    double8 local_C_tmp6 = 0;
    double8 local_C_tmp7 = 0;

    int kb = 0;
    for (kb = 0; kb < heightB; kb += 2) {
      vecA  = local_A[(ib-i)*widthA+kb];
      vecA1 = local_A[(ib-i)*widthA+kb+1];

      local_C_tmp0 += (vecA * local_B[2*kb]);
      local_C_tmp0 += (vecA1 * local_B[2*(kb+1)]);
      local_C_tmp1 += (vecA * local_B[2*kb+1]);
      local_C_tmp1 += (vecA1 * local_B[2*(kb+1)+1]);
        
      vecA  = local_A[(ib+1-i)*widthA+kb];
      vecA1 = local_A[(ib+1-i)*widthA+kb+1];
      local_C_tmp2 += (vecA * local_B[2*kb]);
      local_C_tmp2 += (vecA1 * local_B[2*(kb+1)]);
      local_C_tmp3 += (vecA * local_B[2*kb+1]);
      local_C_tmp3 += (vecA1 * local_B[2*(kb+1)+1]);
        
      vecA  = local_A[(ib+2-i)*widthA+kb];
      vecA1 = local_A[(ib+2-i)*widthA+kb+1];
      local_C_tmp4 += (vecA * local_B[2*kb]);
      local_C_tmp4 += (vecA1 * local_B[2*(kb+1)]);
      local_C_tmp5 += (vecA * local_B[2*kb+1]);
      local_C_tmp5 += (vecA1 * local_B[2*(kb+1)+1]);
        
      vecA  = local_A[(ib+3-i)*widthA+kb];
      vecA1 = local_A[(ib+3-i)*widthA+kb+1];
      local_C_tmp6 += (vecA * local_B[2*kb]);
      local_C_tmp6 += (vecA1 * local_B[2*(kb+1)]);
      local_C_tmp7 += (vecA * local_B[2*kb+1]);
      local_C_tmp7 += (vecA1 * local_B[2*(kb+1)+1]);
    }

    local_C[ic] = local_C_tmp0;
    local_C[ic+1] = local_C_tmp1;
    local_C[ic+2] = local_C_tmp2;
    local_C[ic+3] = local_C_tmp3;
    local_C[ic+4] = local_C_tmp4;
    local_C[ic+5] = local_C_tmp5;
    local_C[ic+6] = local_C_tmp6;
    local_C[ic+7] = local_C_tmp7;
  }

  event_t evtC = xt_async_work_group_2d_copy(
                       matC + (widthC/8*i+j),
                       local_C,
                       localWidthC, 16,
                       widthC * sizeof(double), localWidthC, 0);
  wait_group_events(1, &evtC);
}
