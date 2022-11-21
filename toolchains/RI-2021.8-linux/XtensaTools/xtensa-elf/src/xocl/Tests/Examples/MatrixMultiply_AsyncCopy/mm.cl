__kernel void matMulAsync(__global float* restrict matA,
                          __global float16* restrict matB,
                          __global float16* restrict matC,
                          int widthA,
                          int widthB,
                          int widthC,
                          __local float* restrict local_A,
                          __local float16* restrict local_B)
{
  int i, j;
  int N = widthC;
  float16 vecA;
  float16 vecA1;
  int heightB = widthA;
  int localWidthA = widthA * sizeof(float);
  int localWidthB = 2 * sizeof(float16);
  int localWidthC = 2 * sizeof(float16);
  __local float16 local_C[32*2];

  i = get_global_id(0) * 32;
  j = get_global_id(1) * 2;

  event_t evtA = xt_async_work_group_2d_copy(
                       local_A, 
                       matA + (i * widthA),
                       localWidthA, 32,
                       localWidthA, widthA * sizeof(float), 0);
             
  event_t evtB = xt_async_work_group_2d_copy(
                       local_B,
                       matB + j,
                       localWidthB, heightB,
                       localWidthB, widthB * sizeof(float), 0);

  wait_group_events(1, &evtA);
  wait_group_events(1, &evtB);

  int ic;
  int ib;
  for (ib = i, ic = 0; ib < i + 32; ib += 4, ic += 8) {
    float16 local_C_tmp0 = 0;
    float16 local_C_tmp1 = 0;
    float16 local_C_tmp2 = 0;
    float16 local_C_tmp3 = 0;
    float16 local_C_tmp4 = 0;
    float16 local_C_tmp5 = 0;
    float16 local_C_tmp6 = 0;
    float16 local_C_tmp7 = 0;

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
                       matC + (widthC/16*i+j),
                       local_C,
                       localWidthC, 32,
                       widthC * sizeof(float), localWidthC, 0);
  wait_group_events(1, &evtC);
}
