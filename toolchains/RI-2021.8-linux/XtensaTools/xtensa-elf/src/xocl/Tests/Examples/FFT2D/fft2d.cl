#define SRC_BIT_DEPTH 32
#define DST_BIT_DEPTH 32

int16 mulpack(int16 a, int16 b) {
  int16 mulLo = xt_mul(a, b);
  int16 mulHi = mul_hi(a, b);
  uint16 _mulLo = ((as_uint16(mulLo)) >> 17);
  uint16 _mulHi = ((as_uint16(mulHi)) << 15);
  _mulLo |= _mulHi;
  mulLo = as_int16(_mulLo);
  return mulLo;
}

long16 mul64(short b, int16 a) {
  int32 aa;
  aa.lo = a;
  aa.hi = a;
  long32 Mulresult = xt_mul64(aa, (short32)b);
  return Mulresult.lo;
}

int16 mulspack(short b, int16 a, long16 c) {
  int32 aa;
  aa.lo = a;
  aa.hi = a;
  long32 cc;
  cc.lo = c;
  cc.hi = c;
  long32 Mulresult = xt_msub64(aa, (short32)b, cc);
  int16 out = xt_convert_int16_sat_rnd(Mulresult.lo, 17);
  return out;
}

int16 mulapack(short b, int16 a, long16 c) {
  int32 aa;
  aa.lo = a;
  aa.hi = a;
  long32 cc;
  cc.lo = c;
  cc.hi = c;
  long32 Mulresult = xt_mad64(aa, (short32)b, cc);
  int16 out = xt_convert_int16_sat_rnd(Mulresult.lo, 17);
  return out;
}

__kernel __attribute__((reqd_work_group_size(1, 1, 1)))
__attribute__((reqd_work_dim_size(2))) void
oclKernel_verticalFFT(__global int *restrict Src, int SrcPitch,
                      __global int *restrict DstRe,
                      __global int *restrict DstIm, const int DstPitch,
                      int width, int height, int dstWidth, int dstHeight,
                      int TileW, int TileH, __constant short *restrict W64,
                      __constant int *restrict Out_idx,
                      __local int *restrict local_src,
                      __local int *restrict local_dstRe,
                      __local int *restrict local_dstIm,
                      __local int *restrict Temp_re0,
                      __local int *restrict Temp_im0,
                      __local int *restrict Temp_re1,
                      __local int *restrict Temp_im1) {
  uint i, j;
  uint idx, idy;
  uint local_src_pitch = TileW, local_dst_pitch = TileW >> 1;

  int decimation_factor, WPowerMultiplier;
  int q, group;
  int numElements = 16;
  int stride = 64, strideInp;
  int start_idx, strideS32;
  int stride_in = stride * 4 * 2; // interleaved format

  __local int16 *Out_re = (__local int16 *)local_dstRe;
  __local int16 *Out_im = (__local int16 *)local_dstIm;

  __local int16 *pvecIn;
  __local int16 *pvecInR, *pvecInI;
  __local int16 *pvecOutR, *pvecOutI;
  __local int16 *pvecInR1, *pvecInI1;

  __local int16 *pvecOutR0, *pvecOutI0;
  __local int16 *pvecOutR1, *pvecOutI1;

 
  i = get_global_id(0);
  j = get_global_id(1);

  i = i * TileH;
  j = j * TileW;

  uint srcBytes = SRC_BIT_DEPTH >> 3;
  uint dstBytes = DST_BIT_DEPTH >> 3;

  event_t evtA = xt_async_work_group_2d_copy(
      (__local uchar *)local_src, (__global uchar *)(Src + (i * SrcPitch) + j),
      local_src_pitch * srcBytes, TileH, local_src_pitch * srcBytes,
      SrcPitch * srcBytes, 0);

  wait_group_events(1, &evtA);
#if NATIVE_KERNEL
  int TempMem[64 * 16 * 4];
	#include "fft2d_vertical_ivp.c"
#elif XT_OCL_EXT
  #include "fft2d_vertical_ext.cl"
#else
  const uint16 intlv32A = {0,  2,  4,  6,  8,  10, 12, 14,
                           16, 18, 20, 22, 24, 26, 28, 30};
  const uint16 intlv32B = {1,  3,  5,  7,  9,  11, 13, 15,
                           17, 19, 21, 23, 25, 27, 29, 31};

  __local int *A_re = local_src;
  __local int *A_im = local_src + 128;

  for (int x = 0; x < 64; x += 16) {
    int16 vecWR0, vecWI0, vecWR1, vecWR2, vecWR3, vecWI1, vecWI2, vecWI3;
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
      int16 acc0, acc1;

      pvecIn =
          (__local int16 *)((__local int *)A_re + (2 * x) + (q * stride * 2));
      pvecOutR = (__local int16 *)((__local int *)Temp_re0 + q * numElements);
      pvecOutI = (__local int16 *)((__local int *)Temp_im0 + q * numElements);

      vecWR1 = (int16)*pCoeff++; // W_re[q];
      vecWI1 = (int16)*pCoeff++; // W_im[q];
      vecWR2 = (int16)*pCoeff++; // W_re[2*q];
      vecWI2 = (int16)*pCoeff++; // W_im[2*q];
      vecWR3 = (int16)*pCoeff++; // W_re[3*q];
      vecWI3 = (int16)*pCoeff++; // W_im[3*q];

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

      acc0 = mulpack(vecWR1, vecPR1);
      acc0 -= mulpack(vecWI1, vecPI1);
      *pvecOutR = acc0;
      pvecOutR += 16;
      acc1 = mulpack(vecWR1, vecPI1);
      acc1 += mulpack(vecWI1, vecPR1);
      *pvecOutI = acc1;
      pvecOutI += 16;

      acc0 = mulpack(vecWR2, vecPR2);
      acc0 -= mulpack(vecWI2, vecPI2);
      *pvecOutR = acc0;
      pvecOutR += 16;
      acc1 = mulpack(vecWR2, vecPI2);
      acc1 += mulpack(vecWI2, vecPR2);
      *pvecOutI = acc1;
      pvecOutI += 16;

      acc0 = mulpack(vecWR3, vecPR3);
      acc0 -= mulpack(vecWI3, vecPI3);
      *pvecOutR = acc0;
      pvecOutR += 16;
      acc1 = mulpack(vecWR3, vecPI3);
      acc1 += mulpack(vecWI3, vecPR3);
      *pvecOutI = acc1;
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
        int16 acc0, acc1;
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

        acc0 = mulpack(vecWR1, vecPR1);
        acc0 -= mulpack(vecWI1, vecPI1);
        *pvecOutR = acc0;
        pvecOutR += 4;
        acc1 = mulpack(vecWR1, vecPI1);
        acc1 += mulpack(vecWI1, vecPR1);
        *pvecOutI = acc1;
        pvecOutI += 4;

        acc0 = mulpack(vecWR2, vecPR2);
        acc0 -= mulpack(vecWI2, vecPI2);
        *pvecOutR = acc0;
        pvecOutR += 4;
        acc1 = mulpack(vecWR2, vecPI2);
        acc1 += mulpack(vecWI2, vecPR2);
        *pvecOutI = acc1;
        pvecOutI += 4;

        acc0 = mulpack(vecWR3, vecPR3);
        acc0 -= mulpack(vecWI3, vecPI3);
        *pvecOutR = acc0;
        pvecOutR += 4;
        acc1 = mulpack(vecWR3, vecPI3);
        acc1 += mulpack(vecWI3, vecPR3);
        *pvecOutI = acc1;
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
#endif
  event_t evtE = xt_async_work_group_2d_copy(
      (__global uchar *)(DstRe + (i * (DstPitch >> 1)) + (j >> 1)),
      (__local uchar *)local_dstRe, local_dst_pitch * dstBytes, TileH,
      (DstPitch >> 1) * dstBytes, local_dst_pitch * dstBytes, 0);

  event_t evtD = xt_async_work_group_2d_copy(
      (__global uchar *)(DstIm + (i * (DstPitch >> 1)) + (j >> 1)),
      (__local uchar *)local_dstIm, local_dst_pitch * dstBytes, TileH,
      (DstPitch >> 1) * dstBytes, local_dst_pitch * dstBytes, 0);
  wait_group_events(1, &evtE);
  wait_group_events(1, &evtD);
}

void RADIX4_BTRFLY_ROW0_ROW2_32Bit(int16 in0, int16 in1, int16 in2, int16 in3,
                                   int16 *res0, int16 *res1, int16 *tmp0,
                                   int16 *tmp1) {
  *tmp0 = in0 + in2;
  *tmp1 = in1 + in3;
  *res0 = *tmp0 + *tmp1;
  // mul factors are 1  1  1  1
  *res1 = *tmp0 - *tmp1;
}

void RADIX4_BTRFLY_ROW1_32Bit_real(int16 in0, int16 in1, int16 in2, int16 in3,
                                   int16 *tmp0, int16 *tmp1) {
  *tmp0 = (in0 + in1);
  *tmp1 = (in2 + in3);
  *tmp0 = (*tmp0 - *tmp1);
}

void RADIX4_BTRFLY_ROW1_32Bit_imag(int16 in0, int16 in1, int16 in2, int16 in3,
                                   int16 *tmp0, int16 *tmp1) {
  *tmp0 = (in0 + in3);
  *tmp1 = (in1 + in2);
  *tmp0 = (*tmp0 - *tmp1);
}

void RADIX4_BTRFLY_ROW3_32Bit_imag(int16 in0, int16 in1, int16 in2, int16 in3,
                                   int16 *tmp0, int16 *tmp1) {
  *tmp0 = (in0 + in1);
  *tmp1 = (in2 + in3);
  *tmp0 = (*tmp0 - *tmp1);
}

void CMPLX_MUL_32Bit(int16 t0, int16 t1, int16 *inreal, int16 *inimag) {
  int16 outreal = mulpack(t0, *inreal);
  int16 outimag = mulpack(t1, *inreal);
  *inreal = outreal - mulpack(t1, *inimag);
  *inimag = outimag + mulpack(t0, *inimag);
}

#ifdef XT_OCL_EXT
static __constant ushort kmask[]  __attribute__((section(".dram0.data"))) = {
  0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30,
  1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31
};

void CMPLX_MUL_32_16Bit(short32 t0, int16 *inreal, int16 *inimag) {
  const ushort32 mask = *(__constant ushort32 *)&kmask;
  short32 t0_ri = shuffle(t0, mask);
  int32 inreal_c, inimag_c;
  inreal_c.lo = *inreal;
  inreal_c.hi = *inreal;
  inimag_c.lo = *inimag;
  inimag_c.hi = *inimag;

  int32 a = xt_convert_int32_sat_rnd(xt_mul64(inreal_c, t0_ri), 17);
  int32 b = xt_convert_int32_sat_rnd(xt_mul64(inimag_c, t0_ri), 17);

  a.lo -= b.hi;
  a.hi += b.lo;

  *inreal = a.lo;
  *inimag = a.hi;
}
#endif

void RADIX4_BTRFLY_ROW3_32Bit_real(int16 in0, int16 in1, int16 in2, int16 in3,
                                   int16 *tmp0, int16 *tmp1) {
  *tmp0 = (in0 + in3);
  *tmp1 = (in1 + in2);
  *tmp0 = (*tmp0 - *tmp1);
}

__kernel __attribute__((reqd_work_group_size(1, 1, 1)))
__attribute__((reqd_work_dim_size(2))) void
oclKernel_horizontalFFT(__global int * restrict SrcRe, 
                        __global int * restrict SrcIm, int SrcPitch,
                        __global int *restrict Dst, 
                        const int DstPitch, int width,
                        int height, int dstWidth, int dstHeight, int TileW,
                        int TileH, __constant int *restrict tw_ptr,
                        __local int *restrict local_srcRe, 
                        __local int *restrict local_srcIm,
                        __local int *restrict local_dst,
                        __local int *restrict Temp) {
  uint i, j;
  uint idx, idy;
  uint local_src_pitch = TileW >> 1, local_dst_pitch = TileW;

  i = get_global_id(0);
  j = get_global_id(1);

  i = i * TileH;
  j = j * TileW;

  uint srcBytes = SRC_BIT_DEPTH >> 3;
  uint dstBytes = DST_BIT_DEPTH >> 3;

  event_t evtA = xt_async_work_group_2d_copy(
      (__local uchar *)local_srcRe,
      (__global uchar *)(SrcRe + (i * (SrcPitch >> 1)) + (j >> 1)),
      local_src_pitch * srcBytes, TileH, local_src_pitch * srcBytes,
      (SrcPitch >> 1) * srcBytes, 0);

  event_t evtB = xt_async_work_group_2d_copy(
      (__local uchar *)local_srcIm,
      (__global uchar *)(SrcIm + (i * (SrcPitch >> 1)) + (j >> 1)),
      local_src_pitch * srcBytes, TileH, local_src_pitch * srcBytes,
      (SrcPitch >> 1) * srcBytes, 0);

  wait_group_events(1, &evtA);
  wait_group_events(1, &evtB);
#if NATIVE_KERNEL
  #include "fft2d_horizontal_ivp.c"
#elif XT_OCL_EXT
  #include "fft2d_horizontal_ext.cl"
#else
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

  __constant int16 *pvec_tw0;
  int16 vec_inp0, vec_inp1, vec_inp2, vec_inp3, vec_inp4, vec_inp5, vec_inp6,
      vec_inp7;
  int16 vec_data0, vec_data1, vec_data2, vec_data3, vec_data4, vec_data5,
      vec_data6, vec_data7;
  int16 vec_data00, vec_data01, vec_data10, vec_data11, vec_data20, vec_data21,
      vec_data30, vec_data31;
  int16 vec_tw0, vec_tw1;
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
    pvec_tw0 = (__constant int16 *)tw_ptr;
    vec_tw0 = vload16(0, (__constant int *)pvec_tw0);
    pvec_tw0 += 1;
    vec_tw1 = vload16(0, (__constant int *)pvec_tw0);
    pvec_tw0 += 1;
    CMPLX_MUL_32Bit(vec_tw0, vec_tw1, &vec_data10, &vec_data11);

    vec_tw0 = vload16(0, (__constant int *)pvec_tw0);
    pvec_tw0 += 1;
    vec_tw1 = vload16(0, (__constant int *)pvec_tw0);
    pvec_tw0 += 1;
    CMPLX_MUL_32Bit(vec_tw0, vec_tw1, &vec_data20, &vec_data21);

    // mul factors are 1 -j -1  j
    RADIX4_BTRFLY_ROW3_32Bit_real(vec_inp0, vec_inp3, vec_inp4, vec_inp7,
                                  &vec_data30, &vec_data1);
    // mul factors are 1 -j -1  j
    RADIX4_BTRFLY_ROW3_32Bit_imag(vec_inp1, vec_inp2, vec_inp5, vec_inp6,
                                  &vec_data31, &vec_data2);

    vec_tw0 = vload16(0, (__constant int *)pvec_tw0);
    pvec_tw0 += 1;
    vec_tw1 = vload16(0, (__constant int *)pvec_tw0);
    pvec_tw0 += 1;
    CMPLX_MUL_32Bit(vec_tw0, vec_tw1, &vec_data30, &vec_data31);

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

    pvec_tw0 = (__constant int16 *)(tw_ptr + 96);

    vec_tw0 = *pvec_tw0;
    pvec_tw0++;
    vec_tw1 = *pvec_tw0;
    pvec_tw0++;
    CMPLX_MUL_32Bit(vec_tw0, vec_tw1, &vec_inp2, &vec_inp3);

    vec_tw0 = *pvec_tw0;
    pvec_tw0++;
    vec_tw1 = *pvec_tw0;
    pvec_tw0++;
    CMPLX_MUL_32Bit(vec_tw0, vec_tw1, &vec_inp4, &vec_inp5);

    vec_tw0 = *pvec_tw0;
    pvec_tw0++;
    vec_tw1 = *pvec_tw0;
    pvec_tw0++;
    CMPLX_MUL_32Bit(vec_tw0, vec_tw1, &vec_inp6, &vec_inp7);

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
#endif
  event_t evtE = xt_async_work_group_2d_copy(
      (__global uchar *)(Dst + (i * DstPitch) + j), (__local uchar *)local_dst,
      local_dst_pitch * dstBytes, TileH, DstPitch * dstBytes,
      local_dst_pitch * dstBytes, 0);
  wait_group_events(1, &evtE);
}
