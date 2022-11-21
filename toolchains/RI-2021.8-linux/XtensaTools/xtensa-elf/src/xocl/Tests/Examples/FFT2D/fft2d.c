#include <math.h>
#include "fft.h"
#include <xtensa/tie/xt_ivpn.h>

#define TWIDDLE_FACTOR_Q   15
#define TWIDDLE_FACTOR_RND (1 << (TWIDDLE_FACTOR_Q - 1))

#define RADIX4_BTRFLY_ROW0_ROW2_32Bit(in0, in1, in2, in3, res0, res1, tmp0,    \
                                      tmp1)                                    \
  tmp0 = IVP_ADDN_2X32(in0, in2);                                              \
  tmp1 = IVP_ADDN_2X32(in1, in3);                                              \
  res0 = IVP_ADDN_2X32(tmp0, tmp1);                                            \
  res1 = IVP_SUBN_2X32(tmp0, tmp1); // mul factors are 1  1  1  1

#define RADIX4_BTRFLY_ROW0_32Bit(in0, in1, in2, in3, tmp0, tmp1)               \
  tmp0 = IVP_ADDN_2X32(in0, in1);                                              \
  tmp1 = IVP_ADDN_2X32(in2, in3);                                              \
  tmp0 = IVP_ADDN_2X32(tmp0, tmp1); // mul factors are 1  1  1  1

#define RADIX4_BTRFLY_ROW2_32Bit(in0, in1, in2, in3, tmp0, tmp1)               \
  tmp0 = IVP_ADDN_2X32(in0, in2);                                              \
  tmp1 = IVP_ADDN_2X32(in1, in3);                                              \
  tmp0 = IVP_SUBN_2X32(tmp0, tmp1); // mul factors are 1  1  1  1

#define RADIX4_BTRFLY_ROW1_32Bit_real(in0, in1, in2, in3, tmp0, tmp1)          \
  tmp0 = IVP_ADDN_2X32(in0, in1);                                              \
  tmp1 = IVP_ADDN_2X32(in2, in3);                                              \
  tmp0 = IVP_SUBN_2X32(tmp0, tmp1);

#define RADIX4_BTRFLY_ROW1_32Bit_imag(in0, in1, in2, in3, tmp0, tmp1)          \
  tmp0 = IVP_ADDN_2X32(in0, in3);                                              \
  tmp1 = IVP_ADDN_2X32(in1, in2);                                              \
  tmp0 = IVP_SUBN_2X32(tmp0, tmp1);

#define RADIX4_BTRFLY_ROW3_32Bit_real(in0, in1, in2, in3, tmp0, tmp1)          \
  tmp0 = IVP_ADDN_2X32(in0, in3);                                              \
  tmp1 = IVP_ADDN_2X32(in1, in2);                                              \
  tmp0 = IVP_SUBN_2X32(tmp0, tmp1);

#define RADIX4_BTRFLY_ROW3_32Bit_imag(in0, in1, in2, in3, tmp0, tmp1)          \
  tmp0 = IVP_ADDN_2X32(in0, in1);                                              \
  tmp1 = IVP_ADDN_2X32(in2, in3);                                              \
  tmp0 = IVP_SUBN_2X32(tmp0, tmp1);

#define CMPLX_MUL_32Bit(t0, inreal, inimag)                                    \
  {                                                                            \
    xb_vecN_2x64w wvec1, wvec2;                                                \
    wvec1 = IVP_MULN_2X16X32_0(t0, inreal);                                    \
    IVP_MULSN_2X16X32_1(wvec1, t0, inimag);                                    \
    wvec2 = IVP_MULN_2X16X32_1(t0, inreal);                                    \
    IVP_MULAN_2X16X32_0(wvec2, t0, inimag);                                    \
    inreal = IVP_PACKVRN_2X64W(wvec1, 17);                                     \
    inimag = IVP_PACKVRN_2X64W(wvec2, 17);                                     \
  }

#define STORE_N_2X32_XP(a, b, c)                                               \
  vecTemp = a;                                                                 \
  IVP_SVN_2X32_XP(a, b, c);
#define STORE_N_2X32_X(a, b, c)                                                \
  vecTemp = a;                                                                 \
  IVP_SVN_2X32_X(a, b, c);

/*********************************************************************

        int A : Pointer to 32b complex input in interleaved format :
                 [2K] - Real, [2k+1] - imag

        int out_re : Pointer to 32b complex output real part

        int out_im : Pointer to 32b complex output imag part

        pTemp : Pointer to temp int buffer size : (64 * 2 * 2)

        W_64 : Pointer to twiddle factors array - computed in init_radix4Int_64

        out_idx: Reordering index array - computed in init_radix4Int_64

*********************************************************************/

void xvFFT64_radix4_S32_OS32_vp6(int *A_re, short *W_64, int *out_idx,
                                 int *pTemp, int *Out_re, int *Out_im) {
  int x, decimation_factor, WPowerMultiplier;
  int q, group;
  int *Temp_re[2];
  int *Temp_im[2];
  int numElements = 16;
  int stride = 64, strideInp;
  int start_idx, strideS32;
  int stride_in = stride * 4 * 2; // interleaved format

  xb_vecN_2x32v *__restrict pvecIn;
  xb_vecN_2x32v *__restrict pvecInR, *__restrict pvecInI;
  xb_vecN_2x32v *__restrict pvecOutR, *__restrict pvecOutI;
  xb_vecN_2x32v *__restrict pvecInR1, *__restrict pvecInI1;

  xb_vecN_2x32v *__restrict pvecOutR0, *__restrict pvecOutI0;
  xb_vecN_2x32v *__restrict pvecOutR1, *__restrict pvecOutI1;

  Temp_re[0] = pTemp;
  Temp_im[0] = Temp_re[0] + 64 * 16;
  Temp_re[1] = Temp_im[0] + 64 * 16;
  Temp_im[1] = Temp_re[1] + 64 * 16;

  int *A_im = (int *)A_re + 128;

  for (x = 0; x < 64; x += 16) { // stage 0
    xb_vecN_2x32v vecR0, vecR1, vecR2, vecR3, vecI0, vecI1, vecI2, vecI3;
    xb_vecNx16 vecWR0, vecWI0, vecWR1, vecWR2, vecWR3, vecWI1, vecWI2, vecWI3;
    xb_vecN_2x32v vec0, vec1, vec2, vec3;
    xb_vecN_2x32v vecD0, vecD1, vecD2, vecD3;
    xb_vecN_2x32v vecTemp;
    xb_vecN_2x64w acc2, acc3;
    xb_vecN_2x64w wvec0, wvec1, wvec2, wvec3;
    xb_vecN_2x32v vecOne = 1;
    short *pCoeff = W_64;

    decimation_factor = 16;
    strideInp = (stride_in * decimation_factor) - 64;
    strideS32 = 16 * decimation_factor * 4; // ranjith
    pvecOutR = (xb_vecN_2x32v *)(Temp_re[0]);
    pvecOutI = (xb_vecN_2x32v *)(Temp_im[0]);
    pvecIn = (xb_vecN_2x32v *)(A_re + (2 * x));
    IVP_LVN_2X32_IP(vec0, pvecIn, 64);
    IVP_LVN_2X32_XP(vec1, pvecIn, strideInp);
    IVP_LVN_2X32_IP(vec2, pvecIn, 64);
    IVP_LVN_2X32_XP(vec3, pvecIn, strideInp);
    IVP_LVN_2X32_IP(vecD0, pvecIn, 64);
    IVP_LVN_2X32_XP(vecD1, pvecIn, strideInp);
    IVP_LVN_2X32_IP(vecD2, pvecIn, 64);
    IVP_LVN_2X32_XP(vecD3, pvecIn, strideInp);

    vec0 = vec0 >> 2;
    vec1 = vec1 >> 2;
    vec2 = vec2 >> 2;
    vec3 = vec3 >> 2;
    vecD0 = vecD0 >> 2;
    vecD1 = vecD1 >> 2;
    vecD2 = vecD2 >> 2;
    vecD3 = vecD3 >> 2;

    IVP_DSELN_2X32I(vecI0, vecR0, vec1, vec0, IVP_DSELI_DEINTERLEAVE_2);
    IVP_DSELN_2X32I(vecI1, vecR1, vec3, vec2, IVP_DSELI_DEINTERLEAVE_2);
    IVP_DSELN_2X32I(vecI2, vecR2, vecD1, vecD0, IVP_DSELI_DEINTERLEAVE_2);
    IVP_DSELN_2X32I(vecI3, vecR3, vecD3, vecD2, IVP_DSELI_DEINTERLEAVE_2);

    vec0 = vecR0 + vecR2;
    vec1 = vecR1 + vecR3;
    vec2 = vecI0 + vecI2;
    vec3 = vecI1 + vecI3;
    vecD0 = vecR0 - vecR2;
    vecD1 = vecR1 - vecR3;
    vecD2 = vecI0 - vecI2;
    vecD3 = vecI1 - vecI3;

    STORE_N_2X32_XP(IVP_SRAIN_2X32(vec0 + vec1 + vecOne, 2), pvecOutR,
                    strideS32); //(r0 + r1+r2+r3)<<3
    STORE_N_2X32_XP(IVP_SRAIN_2X32(vec2 + vec3 + vecOne, 2), pvecOutI,
                    strideS32); //(i0 + i1+i2+i3)<<3
    STORE_N_2X32_XP(IVP_SRAIN_2X32(vecD0 + vecD3 + vecOne, 2), pvecOutR,
                    strideS32); //(r0 + i1-r2-i3)<<3
    STORE_N_2X32_XP(IVP_SRAIN_2X32(vecD2 - vecD1 + vecOne, 2), pvecOutI,
                    strideS32); //(i0 - r1-i2+r3)<<3
    STORE_N_2X32_XP(IVP_SRAIN_2X32(vec0 - vec1 + vecOne, 2), pvecOutR,
                    strideS32); //(r0 - r1+r2-r3)<<3
    STORE_N_2X32_XP(IVP_SRAIN_2X32(vec2 - vec3 + vecOne, 2), pvecOutI,
                    strideS32); //(i0 - i1+i2-i3)<<3
    STORE_N_2X32_XP(IVP_SRAIN_2X32(vecD0 - vecD3 + vecOne, 2), pvecOutR,
                    strideS32); //(r0 - i1-r2+i3)<<3
    STORE_N_2X32_XP(IVP_SRAIN_2X32(vecD2 + vecD1 + vecOne, 2), pvecOutI,
                    strideS32); //(i0 + r1-i2-r3)<<3

    for (q = 1; q < 16; q++) {
      xb_vecN_2x32v vecPR0, vecPR1, vecPR2, vecPR3, vecPI0, vecPI1, vecPI2,
          vecPI3;
      xb_vecN_2x64w acc0, acc1;
      //  strideInp         = (stride_in *  decimation_factor) - 64 ;
      pvecIn = (xb_vecN_2x32v *)((int *)A_re + (2 * x) + (q * stride * 2));
      pvecOutR = (xb_vecN_2x32v *)((int *)Temp_re[0] + q * numElements);
      pvecOutI = (xb_vecN_2x32v *)((int *)Temp_im[0] + q * numElements);
      vecWR1 = *pCoeff++; // W_re[q];
      vecWI1 = *pCoeff++; // W_im[q];
      vecWR2 = *pCoeff++; // W_re[2*q];
      vecWI2 = *pCoeff++; // W_im[2*q];
      vecWR3 = *pCoeff++; // W_re[3*q];
      vecWI3 = *pCoeff++; // W_im[3*q];

      IVP_LVN_2X32_XP(vec0, pvecIn, 64);
      IVP_LVN_2X32_XP(vec1, pvecIn, strideInp);
      IVP_LVN_2X32_XP(vec2, pvecIn, 64);
      IVP_LVN_2X32_XP(vec3, pvecIn, strideInp);
      IVP_LVN_2X32_XP(vecD0, pvecIn, 64);
      IVP_LVN_2X32_XP(vecD1, pvecIn, strideInp);
      IVP_LVN_2X32_XP(vecD2, pvecIn, 64);
      IVP_LVN_2X32_XP(vecD3, pvecIn, strideInp);

      vec0 = vec0 >> 2;
      vec1 = vec1 >> 2;
      vec2 = vec2 >> 2;
      vec3 = vec3 >> 2;
      vecD0 = vecD0 >> 2;
      vecD1 = vecD1 >> 2;
      vecD2 = vecD2 >> 2;
      vecD3 = vecD3 >> 2;

      IVP_DSELN_2X32I(vecI0, vecR0, vec1, vec0, IVP_DSELI_DEINTERLEAVE_2);
      IVP_DSELN_2X32I(vecI1, vecR1, vec3, vec2, IVP_DSELI_DEINTERLEAVE_2);
      IVP_DSELN_2X32I(vecI2, vecR2, vecD1, vecD0, IVP_DSELI_DEINTERLEAVE_2);
      IVP_DSELN_2X32I(vecI3, vecR3, vecD3, vecD2, IVP_DSELI_DEINTERLEAVE_2);

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
      STORE_N_2X32_XP(IVP_SRAIN_2X32(vecPR0 + vecOne, 2), pvecOutR,
                      strideS32); // r0*8
      STORE_N_2X32_XP(IVP_SRAIN_2X32(vecPI0 + vecOne, 2), pvecOutI,
                      strideS32); // i0*8
      acc0 = IVP_MULN_2X16X32_0((vecWR1), vecPR1);
      IVP_MULSN_2X16X32_0(acc0, (vecWI1), vecPI1);
      vecTemp = IVP_PACKVRN_2X64W(acc0, 17);
      STORE_N_2X32_XP(IVP_PACKVRN_2X64W(acc0, 17), pvecOutR,
                      strideS32); // r1*wr1-i1*wi1
      acc1 =
          IVP_MULN_2X16X32_0((vecWR1), vecPI1); // IVP_MULNX16(vecPI1, vecWR1);
      IVP_MULAN_2X16X32_0(acc1, (vecWI1),
                          vecPR1); // IVP_MULANX16(acc1, vecPR1, vecWI1);
      STORE_N_2X32_XP(IVP_PACKVRN_2X64W(acc1, 17), pvecOutI,
                      strideS32); // i1*wr1+r1*wi1
      acc0 =
          IVP_MULN_2X16X32_0((vecWR2), vecPR2); // IVP_MULNX16(vecPR2, vecWR2);
      IVP_MULSN_2X16X32_0(acc0, (vecWI2),
                          vecPI2); // IVP_MULSNX16(acc0, vecPI2, vecWI2);
      STORE_N_2X32_XP(IVP_PACKVRN_2X64W(acc0, 17), pvecOutR,
                      strideS32); // r2*wr2-i2*wi2
      acc1 =
          IVP_MULN_2X16X32_0((vecWR2), vecPI2); // IVP_MULNX16(vecPI2, vecWR2);
      IVP_MULAN_2X16X32_0(acc1, (vecWI2),
                          vecPR2); // IVP_MULANX16(acc1, vecPR2, vecWI2);

      STORE_N_2X32_XP(IVP_PACKVRN_2X64W(acc1, 17), pvecOutI,
                      strideS32); // i2*wr2+r2*wi2
      acc0 =
          IVP_MULN_2X16X32_0((vecWR3), vecPR3); // IVP_MULNX16(vecPR3, vecWR3);
      IVP_MULSN_2X16X32_0(acc0, (vecWI3),
                          vecPI3); // IVP_MULSNX16(acc0, vecPI3, vecWI3);
      STORE_N_2X32_XP(IVP_PACKVRN_2X64W(acc0, 17), pvecOutR,
                      strideS32); // r3*wr3-i3*wi3
      acc1 =
          IVP_MULN_2X16X32_0((vecWR3), vecPI3); // IVP_MULNX16(vecPI3, vecWR3);
      IVP_MULAN_2X16X32_0(acc1, (vecWI3),
                          vecPR3); // IVP_MULANX16(acc1, vecPR3, vecWI3);
      STORE_N_2X32_XP(IVP_PACKVRN_2X64W(acc1, 17), pvecOutI,
                      strideS32); // i3*wr3+r3*wi3
    }

    decimation_factor =
        4; // With subsequent stages decimation factor reduces by factor of 4
    WPowerMultiplier = 4;
    // Stage 1
    strideS32 = numElements * decimation_factor * 4;
    for (group = 0, start_idx = 0; group < 4; group++, start_idx += 16) {
      pvecInR = (xb_vecN_2x32v *)((int *)Temp_re[0] + start_idx * numElements);
      pvecInI = (xb_vecN_2x32v *)((int *)Temp_im[0] + start_idx * numElements);
      pvecOutR = (xb_vecN_2x32v *)((int *)Temp_re[1] + start_idx * numElements);
      pvecOutI = (xb_vecN_2x32v *)((int *)Temp_im[1] + start_idx * numElements);

      IVP_LVN_2X32_XP(vecR0, pvecInR, strideS32);
      IVP_LVN_2X32_XP(vecR1, pvecInR, strideS32);
      IVP_LVN_2X32_XP(vecR2, pvecInR, strideS32);
      IVP_LVN_2X32_XP(vecR3, pvecInR, strideS32);
      IVP_LVN_2X32_XP(vecI0, pvecInI, strideS32);
      IVP_LVN_2X32_XP(vecI1, pvecInI, strideS32);
      IVP_LVN_2X32_XP(vecI2, pvecInI, strideS32);
      IVP_LVN_2X32_XP(vecI3, pvecInI, strideS32);

      vec0 = vecR0 + vecR2;
      vec1 = vecR1 + vecR3;
      vec2 = vecI0 + vecI2;
      vec3 = vecI1 + vecI3;
      vecD0 = vecR0 - vecR2;
      vecD1 = vecR1 - vecR3;
      vecD2 = vecI0 - vecI2;
      vecD3 = vecI1 - vecI3;

      STORE_N_2X32_XP(IVP_SRAIN_2X32(vec0 + vec1 + vecOne, 2), pvecOutR,
                      strideS32); //(r0 + r1+r2+r3)>>2;
      STORE_N_2X32_XP(IVP_SRAIN_2X32(vec2 + vec3 + vecOne, 2), pvecOutI,
                      strideS32); //(i0 + i1+i2+i3)>>2;
      STORE_N_2X32_XP(IVP_SRAIN_2X32(vecD0 + vecD3 + vecOne, 2), pvecOutR,
                      strideS32); //(r0 + i1-r2-i3)>>2;
      STORE_N_2X32_XP(IVP_SRAIN_2X32(vecD2 - vecD1 + vecOne, 2), pvecOutI,
                      strideS32); //(i0 - r1-i2+r3)>>2;
      STORE_N_2X32_XP(IVP_SRAIN_2X32(vec0 - vec1 + vecOne, 2), pvecOutR,
                      strideS32); //(r0 - r1+r2-r3)>>2;
      STORE_N_2X32_XP(IVP_SRAIN_2X32(vec2 - vec3 + vecOne, 2), pvecOutI,
                      strideS32); //(i0 - i1+i2-i3)>>2;
      STORE_N_2X32_XP(IVP_SRAIN_2X32(vecD0 - vecD3 + vecOne, 2), pvecOutR,
                      strideS32); //(r0 - i1-r2+i3)>>2;
      STORE_N_2X32_XP(IVP_SRAIN_2X32(vecD2 + vecD1 + vecOne, 2), pvecOutI,
                      strideS32); //(i0 + r1-i2-r3)>>2;

      for (q = 1; q < decimation_factor; q++) { // 32:84 cycles
        xb_vecN_2x32v vecPR0, vecPR1, vecPR2, vecPR3, vecPI0, vecPI1, vecPI2,
            vecPI3;
        xb_vecN_2x64w acc0, acc1;
        pvecInR = (xb_vecN_2x32v *)((int *)Temp_re[0] +
                                    start_idx * numElements + q * numElements);
        pvecInI = (xb_vecN_2x32v *)((int *)Temp_im[0] +
                                    start_idx * numElements + q * numElements);
        pvecOutR = (xb_vecN_2x32v *)((int *)Temp_re[1] +
                                     start_idx * numElements + q * numElements);
        pvecOutI = (xb_vecN_2x32v *)((int *)Temp_im[1] +
                                     start_idx * numElements + q * numElements);
        vecWR1 = *pCoeff++; // W_re[q];
        vecWI1 = *pCoeff++; // W_im[q];
        vecWR2 = *pCoeff++; // W_re[2*q];
        vecWI2 = *pCoeff++; // W_im[2*q];
        vecWR3 = *pCoeff++; // W_re[3*q];
        vecWI3 = *pCoeff++; // W_im[3*q];
        IVP_LVN_2X32_XP(vecR0, pvecInR, strideS32);
        IVP_LVN_2X32_XP(vecR1, pvecInR, strideS32);
        IVP_LVN_2X32_XP(vecR2, pvecInR, strideS32);
        IVP_LVN_2X32_XP(vecR3, pvecInR, strideS32);
        IVP_LVN_2X32_XP(vecI0, pvecInI, strideS32);
        IVP_LVN_2X32_XP(vecI1, pvecInI, strideS32);
        IVP_LVN_2X32_XP(vecI2, pvecInI, strideS32);
        IVP_LVN_2X32_XP(vecI3, pvecInI, strideS32);
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

        STORE_N_2X32_XP(IVP_SRAIN_2X32(vecPR0 + vecOne, 2), pvecOutR,
                        strideS32);
        STORE_N_2X32_XP(IVP_SRAIN_2X32(vecPI0 + vecOne, 2), pvecOutI,
                        strideS32);
        acc0 = IVP_MULN_2X16X32_0((vecWR1), vecPR1);
        IVP_MULSN_2X16X32_0(acc0, (vecWI1), vecPI1);
        STORE_N_2X32_XP(IVP_PACKVRN_2X64W(acc0, 17), pvecOutR, strideS32);
        acc1 = IVP_MULN_2X16X32_0((vecWR1),
                                  vecPI1); // IVP_MULNX16(vecPI1, vecWR1);
        IVP_MULAN_2X16X32_0(acc1, (vecWI1),
                            vecPR1); // IVP_MULANX16(acc1, vecPR1, vecWI1);
        STORE_N_2X32_XP(IVP_PACKVRN_2X64W(acc1, 17), pvecOutI, strideS32);
        acc0 = IVP_MULN_2X16X32_0((vecWR2),
                                  vecPR2); // IVP_MULNX16(vecPR2, vecWR2);
        IVP_MULSN_2X16X32_0(acc0, (vecWI2),
                            vecPI2); // IVP_MULSNX16(acc0, vecPI2, vecWI2);
        STORE_N_2X32_XP(IVP_PACKVRN_2X64W(acc0, 17), pvecOutR, strideS32);
        acc1 = IVP_MULN_2X16X32_0((vecWR2),
                                  vecPI2); // IVP_MULNX16(vecPI2, vecWR2);
        IVP_MULAN_2X16X32_0(acc1, (vecWI2),
                            vecPR2); // IVP_MULANX16(acc1, vecPR2, vecWI2);
        STORE_N_2X32_XP(IVP_PACKVRN_2X64W(acc1, 17), pvecOutI, strideS32);
        acc0 = IVP_MULN_2X16X32_0((vecWR3),
                                  vecPR3); // IVP_MULNX16(vecPR3, vecWR3);
        IVP_MULSN_2X16X32_0(acc0, (vecWI3),
                            vecPI3); // IVP_MULSNX16(acc0, vecPI3, vecWI3);
        STORE_N_2X32_XP(IVP_PACKVRN_2X64W(acc0, 17), pvecOutR, strideS32);
        acc1 = IVP_MULN_2X16X32_0((vecWR3),
                                  vecPI3); // IVP_MULNX16(vecPI3, vecWR3);
        IVP_MULAN_2X16X32_0(acc1, (vecWI3),
                            vecPR3); // IVP_MULANX16(acc1, vecPR3, vecWI3);
        STORE_N_2X32_XP(IVP_PACKVRN_2X64W(acc1, 17), pvecOutI, strideS32);
      }
    }

    // Stage 2
    // decimation_factor = 1
    strideS32 = numElements * 4;

    // 16*16
    // 16:64 cycles
    // 8 store, 8 load, 8*3 adds, 8 shift, 4 out_idx loads
    for (group = 0, start_idx = 0; group < 16; group++, start_idx += 4) {

      pvecInR = (xb_vecN_2x32v *)((int *)Temp_re[1] + start_idx * numElements);
      pvecInI = (xb_vecN_2x32v *)((int *)Temp_im[1] + start_idx * numElements);
      pvecOutR = (xb_vecN_2x32v *)((int *)Out_re + x);
      pvecOutI = (xb_vecN_2x32v *)((int *)Out_im + x);

      IVP_LVN_2X32_XP(vecR0, pvecInR, strideS32);
      IVP_LVN_2X32_XP(vecR1, pvecInR, strideS32);
      IVP_LVN_2X32_XP(vecR2, pvecInR, strideS32);
      IVP_LVN_2X32_XP(vecR3, pvecInR, strideS32);
      IVP_LVN_2X32_XP(vecI0, pvecInI, strideS32);
      IVP_LVN_2X32_XP(vecI1, pvecInI, strideS32);
      IVP_LVN_2X32_XP(vecI2, pvecInI, strideS32);
      IVP_LVN_2X32_XP(vecI3, pvecInI, strideS32);
      vec0 = vecR0 + vecR2;
      vec1 = vecR1 + vecR3;
      vec2 = vecI0 + vecI2;
      vec3 = vecI1 + vecI3;
      vecD0 = vecR0 - vecR2;
      vecD1 = vecR1 - vecR3;
      vecD2 = vecI0 - vecI2;
      vecD3 = vecI1 - vecI3;

      STORE_N_2X32_X((vec0 + vec1), pvecOutR,
                     out_idx[start_idx] * strideS32 * 4); //(r0 + r1+r2+r3)>>1;
      STORE_N_2X32_X((vec2 + vec3), pvecOutI,
                     out_idx[start_idx] * strideS32 * 4); //(i0 + i1+i2+i3)>>1;
      STORE_N_2X32_X((vecD0 + vecD3), pvecOutR,
                     out_idx[start_idx + 1] * strideS32 *
                         4); //(r0 + i1-r2-i3)>>1;
      STORE_N_2X32_X((vecD2 - vecD1), pvecOutI,
                     out_idx[start_idx + 1] * strideS32 *
                         4); //(i0 - r1-i2+r3)>>1;
      STORE_N_2X32_X((vec0 - vec1), pvecOutR,
                     out_idx[start_idx + 2] * strideS32 *
                         4); //(r0 - r1+r2-r3)>>1;
      STORE_N_2X32_X((vec2 - vec3), pvecOutI,
                     out_idx[start_idx + 2] * strideS32 *
                         4); //(i0 - i1+i2-i3)>>1;
      STORE_N_2X32_X((vecD0 - vecD3), pvecOutR,
                     out_idx[start_idx + 3] * strideS32 *
                         4); //(r0 - i1-r2+i3)>>1;
      STORE_N_2X32_X((vecD2 + vecD1), pvecOutI,
                     out_idx[start_idx + 3] * strideS32 *
                         4); //(i0 + r1-i2-r3)>>1;
    }
  }
}

void xvfft128_horizontal_complex(int *A_re, int *A_im, int *Out, int *Temp,
                                 short *tw_ptr) {
  const int select_pattern_interleave4[16] = {0, 1, 2, 3, 16, 17, 18, 19,
                                              4, 5, 6, 7, 20, 21, 22, 23};
  const int select_pattern_interleave8[16] = {0,  1,  2,  3,  4,  5,  6,  7,
                                              16, 17, 18, 19, 20, 21, 22, 23};
  const int select_pattern_32_rearrange[16] = {0, 4, 8,  12, 1, 5, 9,  13,
                                               2, 6, 10, 14, 3, 7, 11, 15};
  const int select_pattern_shuffle4[16] = {0, 16, 4, 20, 8, 24, 12, 28,
                                           1, 17, 5, 21, 9, 25, 13, 29};
  const int select_pattern_interleave2[16] = {0, 1, 16, 17, 2, 3, 18, 19,
                                              4, 5, 20, 21, 6, 7, 22, 23};
  const short bool_pattern[2] = {0xaaaa, 0xaaaa};
  xb_vecN_2x32v *__restrict pvec_inp0, *__restrict pvec_inp1;
  xb_vecNx16 *__restrict pvec_tw0;
  xb_vecN_2x32v *__restrict pvec_out0;
  xb_vecN_2x32v vec_data0, vec_data1, vec_data2, vec_data3, vec_data4,
      vec_data5, vec_data6, vec_data7;
  xb_vecN_2x32v vec_temp1, vec_temp2;
  xb_vecN_2x32v vec_inp0, vec_inp1, vec_inp2, vec_inp3, vec_inp4, vec_inp5,
      vec_inp6, vec_inp7;
  xb_vecNx16 vec_tw0;
  xb_vecN_2x32v vec_sel0, vec_sel1, vec_sel2, vec_sel3, vec_sel4, vec_sel5,
      vec_zero, vec_sel6, vec_sel7, vec_sel8, vec_sel9;
  xb_vecN_2x32v vec_data00, vec_data01, vec_data10, vec_data11, vec_data20,
      vec_data21, vec_data30, vec_data31;
  vboolN_2 vb_bool;
  int iy = 0;
  vb_bool = *(vboolN_2 *)(bool_pattern);

  vec_sel0 = *(xb_vecN_2x32v *)(select_pattern_interleave4);
  vec_sel1 = vec_sel0 + (xb_vecN_2x32v)8;

  vec_sel2 = *(xb_vecN_2x32v *)(select_pattern_interleave8);
  vec_sel3 = vec_sel2 + (xb_vecN_2x32v)8;
  vec_sel4 = *(xb_vecN_2x32v *)(select_pattern_32_rearrange);
  vec_sel5 = vec_sel4 + (xb_vecN_2x32v)2;
  vec_zero = (xb_vecN_2x32v)0;
  vec_sel6 = *(xb_vecN_2x32v *)(select_pattern_interleave2);
  vec_sel7 = vec_sel6 + (xb_vecN_2x32v)8;
  vec_sel8 = *(xb_vecN_2x32v *)(select_pattern_shuffle4);
  vec_sel9 = vec_sel8 + (xb_vecN_2x32v)2;

  pvec_inp0 = (xb_vecN_2x32v *)(&A_re[0]);
  pvec_inp1 = (xb_vecN_2x32v *)(&A_im[0]);
  pvec_out0 = (xb_vecN_2x32v *)(&Temp[0]);
  for (iy = 0; iy < 64; iy++) {
    IVP_LVN_2X32_IP(vec_inp0, pvec_inp0, 64); // 16 values of
    IVP_LVN_2X32_IP(vec_inp2, pvec_inp0, 64); // 16 values of
    IVP_LVN_2X32_IP(vec_inp4, pvec_inp0, 64); // 16 values of
    IVP_LVN_2X32_IP(vec_inp6, pvec_inp0, 64); // 16 values of

    IVP_LVN_2X32_IP(vec_inp1, pvec_inp1, 64); // 16 values of
    IVP_LVN_2X32_IP(vec_inp3, pvec_inp1, 64); // 16 values of
    IVP_LVN_2X32_IP(vec_inp5, pvec_inp1, 64); // 16 values of
    IVP_LVN_2X32_IP(vec_inp7, pvec_inp1, 64); // 16 values of
    vec_inp0 = vec_inp0 >> 2;
    vec_inp1 = vec_inp1 >> 2;
    vec_inp2 = vec_inp2 >> 2;
    vec_inp3 = vec_inp3 >> 2;
    vec_inp4 = vec_inp4 >> 2;
    vec_inp5 = vec_inp5 >> 2;
    vec_inp6 = vec_inp6 >> 2;
    vec_inp7 = vec_inp7 >> 2;

    pvec_tw0 = (xb_vecNx16 *)tw_ptr;
    RADIX4_BTRFLY_ROW0_ROW2_32Bit(vec_inp0, vec_inp2, vec_inp4, vec_inp6,
                                  vec_data00, vec_data20, vec_data1,
                                  vec_data2); // mul factors are 1  1  1  1
    RADIX4_BTRFLY_ROW0_ROW2_32Bit(vec_inp1, vec_inp3, vec_inp5, vec_inp7,
                                  vec_data01, vec_data21, vec_data1,
                                  vec_data2); // mul factors are 1  1  1  1
    vec_data00 = vec_data00 >> 2;
    vec_data01 = vec_data01 >> 2;

    RADIX4_BTRFLY_ROW1_32Bit_real(vec_inp0, vec_inp3, vec_inp4, vec_inp7,
                                  vec_data10,
                                  vec_data1); // mul factors are 1 -j -1  j
    RADIX4_BTRFLY_ROW1_32Bit_imag(vec_inp1, vec_inp2, vec_inp5, vec_inp6,
                                  vec_data11,
                                  vec_data2); // mul factors are 1 -j -1  j
    // multiply vec_data0 by w_64(0:15)
    IVP_LVNX16_IP(vec_tw0, pvec_tw0, 64);
    CMPLX_MUL_32Bit(vec_tw0, vec_data10, vec_data11)
        IVP_LVNX16_IP(vec_tw0, pvec_tw0, 64);
    CMPLX_MUL_32Bit(vec_tw0, vec_data20, vec_data21)
        RADIX4_BTRFLY_ROW3_32Bit_real(vec_inp0, vec_inp3, vec_inp4, vec_inp7,
                                      vec_data30,
                                      vec_data1); // mul factors are 1 -j -1  j
    RADIX4_BTRFLY_ROW3_32Bit_imag(vec_inp1, vec_inp2, vec_inp5, vec_inp6,
                                  vec_data31,
                                  vec_data2); // mul factors are 1 -j -1  j
    IVP_LVNX16_IP(vec_tw0, pvec_tw0, 64);
    CMPLX_MUL_32Bit(vec_tw0, vec_data30, vec_data31) vec_data0 =
        IVP_SELN_2X32(vec_data10, vec_data00, vec_sel0);
    vec_data1 = IVP_SELN_2X32(vec_data10, vec_data00, vec_sel1);
    vec_data2 = IVP_SELN_2X32(vec_data30, vec_data20, vec_sel0);
    vec_data3 = IVP_SELN_2X32(vec_data30, vec_data20, vec_sel1);
    vec_data00 = IVP_SELN_2X32(vec_data2, vec_data0, vec_sel2);
    vec_data10 = IVP_SELN_2X32(vec_data2, vec_data0, vec_sel3);
    vec_data20 = IVP_SELN_2X32(vec_data3, vec_data1, vec_sel2);
    vec_data30 = IVP_SELN_2X32(vec_data3, vec_data1, vec_sel3);
    vec_data0 = IVP_SELN_2X32(vec_data11, vec_data01, vec_sel0);
    vec_data1 = IVP_SELN_2X32(vec_data11, vec_data01, vec_sel1);
    vec_data2 = IVP_SELN_2X32(vec_data31, vec_data21, vec_sel0);
    vec_data3 = IVP_SELN_2X32(vec_data31, vec_data21, vec_sel1);
    vec_data01 = IVP_SELN_2X32(vec_data2, vec_data0, vec_sel2);
    vec_data11 = IVP_SELN_2X32(vec_data2, vec_data0, vec_sel3);
    vec_data21 = IVP_SELN_2X32(vec_data3, vec_data1, vec_sel2);
    vec_data31 = IVP_SELN_2X32(vec_data3, vec_data1, vec_sel3);

    RADIX4_BTRFLY_ROW0_ROW2_32Bit(vec_data00, vec_data10, vec_data20,
                                  vec_data30, vec_inp0, vec_inp4, vec_temp1,
                                  vec_temp2);
    RADIX4_BTRFLY_ROW0_ROW2_32Bit(vec_data01, vec_data11, vec_data21,
                                  vec_data31, vec_inp1, vec_inp5, vec_temp1,
                                  vec_temp2);
    RADIX4_BTRFLY_ROW1_32Bit_real(vec_data00, vec_data11, vec_data20,
                                  vec_data31, vec_inp2, vec_temp1);
    RADIX4_BTRFLY_ROW1_32Bit_imag(vec_data01, vec_data10, vec_data21,
                                  vec_data30, vec_inp3, vec_temp2);
    RADIX4_BTRFLY_ROW3_32Bit_real(vec_data00, vec_data11, vec_data20,
                                  vec_data31, vec_inp6,
                                  vec_temp1); // mul factors are 1 -j -1  j
    RADIX4_BTRFLY_ROW3_32Bit_imag(vec_data01, vec_data10, vec_data21,
                                  vec_data30, vec_inp7,
                                  vec_temp2); // mul factors are 1 -j -1  j

    IVP_SVN_2X32_IP(vec_inp0, pvec_out0, 64);
    IVP_SVN_2X32_IP(vec_inp1, pvec_out0, 64);
    IVP_SVN_2X32_IP(vec_inp2, pvec_out0, 64);
    IVP_SVN_2X32_IP(vec_inp3, pvec_out0, 64);
    IVP_SVN_2X32_IP(vec_inp4, pvec_out0, 64);
    IVP_SVN_2X32_IP(vec_inp5, pvec_out0, 64);
    IVP_SVN_2X32_IP(vec_inp6, pvec_out0, 64);
    IVP_SVN_2X32_IP(vec_inp7, pvec_out0, 64);
  }

  IVP_SCATTERW();

  pvec_inp0 = (xb_vecN_2x32v *)(&Temp[0]);
  pvec_out0 = (xb_vecN_2x32v *)&Out[0];
  for (iy = 0; iy < 64; iy++) {
    IVP_LVN_2X32_IP(vec_inp0, pvec_inp0, 64);
    IVP_LVN_2X32_IP(vec_inp1, pvec_inp0, 64);
    IVP_LVN_2X32_IP(vec_inp2, pvec_inp0, 64);
    IVP_LVN_2X32_IP(vec_inp3, pvec_inp0, 64);
    IVP_LVN_2X32_IP(vec_inp4, pvec_inp0, 64);
    IVP_LVN_2X32_IP(vec_inp5, pvec_inp0, 64);
    IVP_LVN_2X32_IP(vec_inp6, pvec_inp0, 64);
    IVP_LVN_2X32_IP(vec_inp7, pvec_inp0, 64);
    vec_inp0 = vec_inp0 >> 2;
    vec_inp1 = vec_inp1 >> 2;

    pvec_tw0 = (xb_vecNx16 *)(tw_ptr + 96);
    IVP_LVNX16_IP(vec_tw0, pvec_tw0, 64);
    CMPLX_MUL_32Bit(vec_tw0, vec_inp2, vec_inp3)
        IVP_LVNX16_IP(vec_tw0, pvec_tw0, 64);
    CMPLX_MUL_32Bit(vec_tw0, vec_inp4, vec_inp5)
        IVP_LVNX16_IP(vec_tw0, pvec_tw0, 64);
    CMPLX_MUL_32Bit(vec_tw0, vec_inp6, vec_inp7) vec_data0 =
        IVP_SELN_2X32(vec_inp2, vec_inp0, vec_sel8);
    vec_data2 = IVP_SELN_2X32(vec_inp2, vec_inp0, vec_sel9);
    vec_data4 = IVP_SELN_2X32(vec_inp6, vec_inp4, vec_sel8);
    vec_data6 = IVP_SELN_2X32(vec_inp6, vec_inp4, vec_sel9);
    IVP_DSELN_2X32I(vec_inp2, vec_inp0, vec_data4, vec_data0,
                    IVP_DSELI_INTERLEAVE_4);
    IVP_DSELN_2X32I(vec_inp6, vec_inp4, vec_data6, vec_data2,
                    IVP_DSELI_INTERLEAVE_4);
    vec_data1 = IVP_SELN_2X32(vec_inp3, vec_inp1, vec_sel8);
    vec_data3 = IVP_SELN_2X32(vec_inp3, vec_inp1, vec_sel9);
    vec_data5 = IVP_SELN_2X32(vec_inp7, vec_inp5, vec_sel8);
    vec_data7 = IVP_SELN_2X32(vec_inp7, vec_inp5, vec_sel9);
    IVP_DSELN_2X32I(vec_inp3, vec_inp1, vec_data5, vec_data1,
                    IVP_DSELI_INTERLEAVE_4);
    IVP_DSELN_2X32I(vec_inp7, vec_inp5, vec_data7, vec_data3,
                    IVP_DSELI_INTERLEAVE_4);
    RADIX4_BTRFLY_ROW0_ROW2_32Bit(vec_inp0, vec_inp2, vec_inp4, vec_inp6,
                                  vec_data00, vec_data20, vec_data1, vec_data2);
    RADIX4_BTRFLY_ROW0_ROW2_32Bit(vec_inp1, vec_inp3, vec_inp5, vec_inp7,
                                  vec_data01, vec_data21, vec_data1, vec_data2);
    RADIX4_BTRFLY_ROW1_32Bit_real(vec_inp0, vec_inp3, vec_inp4, vec_inp7,
                                  vec_data10, vec_data1);
    RADIX4_BTRFLY_ROW1_32Bit_imag(vec_inp1, vec_inp2, vec_inp5, vec_inp6,
                                  vec_data11, vec_data2);
    RADIX4_BTRFLY_ROW3_32Bit_real(vec_inp0, vec_inp3, vec_inp4, vec_inp7,
                                  vec_data30,
                                  vec_data1); // mul factors are 1 -j -1  j
    RADIX4_BTRFLY_ROW3_32Bit_imag(vec_inp1, vec_inp2, vec_inp5, vec_inp6,
                                  vec_data31,
                                  vec_data2); // mul factors are 1 -j -1  j
    vec_data0 = IVP_SELN_2X32(vec_data01, vec_data00, vec_sel8);
    vec_data1 = IVP_SELN_2X32(vec_data01, vec_data00, vec_sel9);
    vec_data2 = IVP_SELN_2X32(vec_data11, vec_data10, vec_sel8);
    vec_data3 = IVP_SELN_2X32(vec_data11, vec_data10, vec_sel9);
    vec_data4 = IVP_SELN_2X32(vec_data21, vec_data20, vec_sel8);
    vec_data5 = IVP_SELN_2X32(vec_data21, vec_data20, vec_sel9);
    vec_data6 = IVP_SELN_2X32(vec_data31, vec_data30, vec_sel8);
    vec_data7 = IVP_SELN_2X32(vec_data31, vec_data30, vec_sel9);
    IVP_SVN_2X32_IP(vec_data0, pvec_out0, 64);
    IVP_SVN_2X32_IP(vec_data1, pvec_out0, 64);
    IVP_SVN_2X32_IP(vec_data2, pvec_out0, 64);
    IVP_SVN_2X32_IP(vec_data3, pvec_out0, 64);
    IVP_SVN_2X32_IP(vec_data4, pvec_out0, 64);
    IVP_SVN_2X32_IP(vec_data5, pvec_out0, 64);
    IVP_SVN_2X32_IP(vec_data6, pvec_out0, 64);
    IVP_SVN_2X32_IP(vec_data7, pvec_out0, 64);
  }
}
