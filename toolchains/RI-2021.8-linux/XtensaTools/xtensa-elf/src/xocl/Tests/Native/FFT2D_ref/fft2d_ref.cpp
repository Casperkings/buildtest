#include "fft.h"
#include <math.h>

#define TWIDDLE_FACTOR_Q 15
#define TWIDDLE_FACTOR_RND (1 << (TWIDDLE_FACTOR_Q - 1))

int log_4(int n) {
  int res;
  for (res = 0; n >= 4; res++)
    n = n >> 2;
  return res;
}

/********************************************************************************
********************************************************************************
*******Initialization of twiddle factors and output reorder indexing array******
********************************************************************************
********************************************************************************/

void init_radix4Int_64(int n, short *W_64, int *out_idx) {
  int i, j, k;
  int numStages;
  short W_re[64], W_im[64];

  // populate twiddle factor array with n-elements
  for (i = 0; i < n; i++) {
    int c, s;
    c = (int)(cos(2 * PI * i / n) *
              (1 << TWIDDLE_FACTOR_Q)); // Check what happens when it's
    s = (int)(-sin(2 * PI * i / n) * (1 << TWIDDLE_FACTOR_Q));

    W_re[i] = (short)((c > 32767) ? 32767 : c);
    W_im[i] = (short)((s > 32767) ? 32767 : s);
  }
  numStages = log_4(n);
  for (i = 0; i < n; i++) {
    int idx = 0;
    k = i;
    for (j = 1; j < numStages; j++) {
      int mul = n >> (j * 2);
      idx += (k & 0x3) * mul;
      k = k >> 2;
    }
    idx += (k & 0x3);
    out_idx[i] = idx;
  }

  {
    int decimation_factor, num_butterfly_groups, WPowerMultiplier, stage, group,
        q;
    int checkCount;
    short *pW = W_64;
    decimation_factor = n >> 2;
    WPowerMultiplier = 1;
    checkCount = 0;
    for (stage = 0; stage < numStages - 1; stage++) {
      num_butterfly_groups = n / decimation_factor / 4;
      for (group = 0; group < num_butterfly_groups; group++) {
        for (q = 1; q < decimation_factor; q++) {
          *pW++ = W_re[q * WPowerMultiplier];
          *pW++ = W_im[q * WPowerMultiplier];
          *pW++ = W_re[2 * q * WPowerMultiplier];
          *pW++ = W_im[2 * q * WPowerMultiplier];
          *pW++ = W_re[3 * q * WPowerMultiplier];
          *pW++ = W_im[3 * q * WPowerMultiplier];
          checkCount += 6;
        }
      }
      decimation_factor =
          decimation_factor >>
          2; // With subsequent stages decimation factor reduces by factor of 4
      WPowerMultiplier = WPowerMultiplier * 4;
    }
  }
  return;
}

/*********************************************************************

        int A : Pointer to 32b complex input in interleaved format :
                 [2K] - Real, [2k+1] - imag

        int out_re : Pointer to 32b complex output real part

        int out_im : Pointer to 32b complex output imag part

        pTemp : Pointer to temp int buffer size : (64 * 2 * 2)

        W_64 : Pointer to twiddle factors array - computed in init_radix4Int_64

        out_idx: Reordering index array - computed in init_radix4Int_64

*********************************************************************/
void fft_radix4_vertical_64(int *A_re, short *W_64, int *out_idx, int *pTemp,
                            int *out_re, int *out_im) {
  int stage;
  int decimation_factor;
  int num_butterfly_groups;
  int WPowerMultiplier;
  int q = 0, group, x;
  int start_idx = 0;

  int *Temp_re[2];
  int *Temp_im[2];
  int *pIn_re, *pIn_im, *pOut_re, *pOut_im;
  long long r0, r1, r2, r3;
  long long i0, i1, i2, i3;
  long long pr0, pr1, pr2, pr3;
  long long pi0, pi1, pi2, pi3;
  int *A_im = (int *)A_re + 1;

  Temp_re[0] = pTemp;
  Temp_im[0] = Temp_re[0] + 64;
  Temp_re[1] = Temp_im[0] + 64;
  Temp_im[1] = Temp_re[1] + 64;

  int stride = 128;
  for (x = 0; x < 64; x += 1) {
    decimation_factor = 16;
    WPowerMultiplier = 1;

    // stage 0
    pIn_re = A_re + (2 * x);
    pIn_im = A_im + (2 * x);
    pOut_re = Temp_re[0];
    pOut_im = Temp_im[0];

    short *pCoeff = W_64;
    stage = 0;
    num_butterfly_groups = 1;
    r0 = pIn_re[0] >> 2;
    r1 = pIn_re[decimation_factor * stride] >> 2;
    r2 = pIn_re[decimation_factor * 2 * stride] >> 2;
    r3 = pIn_re[decimation_factor * 3 * stride] >> 2;
    i0 = pIn_im[0] >> 2;
    i1 = pIn_im[decimation_factor * stride] >> 2;
    i2 = pIn_im[decimation_factor * 2 * stride] >> 2;
    i3 = pIn_im[decimation_factor * 3 * stride] >> 2;
    // [1 1 1 1]
    pOut_re[0] = (r0 + r1 + r2 + r3 + 1) >> 2;
    pOut_im[0] = (i0 + i1 + i2 + i3 + 1) >> 2;
    // [1 -j -1 j]
    pOut_re[decimation_factor] = (r0 + i1 - r2 - i3 + 1) >> 2;
    pOut_im[decimation_factor] = (i0 - r1 - i2 + r3 + 1) >> 2;
    // [1 -1 1 -1]
    pOut_re[decimation_factor * 2] = (r0 - r1 + r2 - r3 + 1) >> 2;
    pOut_im[decimation_factor * 2] = (i0 - i1 + i2 - i3 + 1) >> 2;
    // [1 j -1 -j]
    pOut_re[decimation_factor * 3] = (r0 - i1 - r2 + i3 + 1) >> 2;
    pOut_im[decimation_factor * 3] = (i0 + r1 - i2 - r3 + 1) >> 2;

#ifdef PRINTF_DEBUG
    q = 0;
    printf("pOut_re pOut_im : %d  %d  %d  %d  %d  %d  %d  %d\n", pOut_re[q],
           pOut_im[q], pOut_re[decimation_factor + q],
           pOut_im[decimation_factor + q], pOut_re[(2 * decimation_factor) + q],
           pOut_im[(2 * decimation_factor) + q],
           pOut_re[(3 * decimation_factor) + q],
           pOut_im[(3 * decimation_factor) + q]);
#endif

    for (q = 1; q < decimation_factor; q++) {
      short w1_re, w2_re, w3_re;
      short w1_im, w2_im, w3_im;

      r0 = pIn_re[q * stride] >> 2;
      r1 = pIn_re[(q + decimation_factor) * stride] >> 2;
      r2 = pIn_re[(q + decimation_factor * 2) * stride] >> 2;
      r3 = pIn_re[(q + decimation_factor * 3) * stride] >> 2;
      i0 = pIn_im[q * stride] >> 2;
      i1 = pIn_im[(q + decimation_factor) * stride] >> 2;
      i2 = pIn_im[(q + decimation_factor * 2) * stride] >> 2;
      i3 = pIn_im[(q + decimation_factor * 3) * stride] >> 2;

      // [1 1 1 1]
      pr0 = r0 + r1 + r2 + r3;
      pi0 = i0 + i1 + i2 + i3;
      // [1 -j -1 j]
      pr1 = r0 + i1 - r2 - i3;
      pi1 = i0 - r1 - i2 + r3;
      // [1 -1 1 -1]
      pr2 = r0 - r1 + r2 - r3;
      pi2 = i0 - i1 + i2 - i3;
      // [1 j -1 -j]
      pr3 = r0 - i1 - r2 + i3;
      pi3 = i0 + r1 - i2 - r3;

      w1_re = *pCoeff++; // W_re[q*WPowerMultiplier];
      w1_im = *pCoeff++; // W_im[q*WPowerMultiplier];
      w2_re = *pCoeff++; // W_re[2*q*WPowerMultiplier];
      w2_im = *pCoeff++; // W_im[2*q*WPowerMultiplier];
      w3_re = *pCoeff++; // W_re[3*q*WPowerMultiplier];
      w3_im = *pCoeff++; // W_im[3*q*WPowerMultiplier];

      pOut_re[q] = (pr0 + 1) >> 2;
      pOut_re[q + decimation_factor] =
          (int)((pr1 * w1_re) >> 17) - ((pi1 * w1_im) >> 17);
      pOut_re[q + decimation_factor * 2] =
          (int)((pr2 * w2_re) >> 17) - ((pi2 * w2_im) >> 17);
      pOut_re[q + decimation_factor * 3] =
          (int)((pr3 * w3_re) >> 17) - ((pi3 * w3_im) >> 17);
      pOut_im[q] = (pi0 + 1) >> 2;
      pOut_im[q + decimation_factor] =
          (int)((pi1 * w1_re) >> 17) + ((pr1 * w1_im) >> 17);
      pOut_im[q + decimation_factor * 2] =
          (int)((pi2 * w2_re) >> 17) + ((pr2 * w2_im) >> 17);
      pOut_im[q + decimation_factor * 3] =
          (int)((pi3 * w3_re) >> 17) + ((pr3 * w3_im) >> 17);

#ifdef PRINTF_DEBUG
      printf("pOut_re pOut_im : %d	%d	%d	%d	%d	"
             "%d	%d	%d\n",
             pOut_re[q], pOut_im[q], pOut_re[decimation_factor + q],
             pOut_im[decimation_factor + q],
             pOut_re[(2 * decimation_factor) + q],
             pOut_im[(2 * decimation_factor) + q],
             pOut_re[(3 * decimation_factor) + q],
             pOut_im[(3 * decimation_factor) + q]);
#endif
    }
    decimation_factor =
        decimation_factor >>
        2; // With subsequent stages decimation factor reduces by factor of 4
    WPowerMultiplier = WPowerMultiplier * 4;
    pIn_re = pOut_re;
    pIn_im = pOut_im;
    stage++;
    pOut_re = Temp_re[stage & 0x1];
    pOut_im = Temp_im[stage & 0x1];

    // Stage 1
    num_butterfly_groups = 64 / decimation_factor / 4;
    for (group = 0; group < num_butterfly_groups; group++) {
      start_idx =
          group * 64 / num_butterfly_groups; // start-index for the current
                                             // group in the current stage
      q = 0;
      r0 = pIn_re[start_idx];
      r1 = pIn_re[start_idx + decimation_factor];
      r2 = pIn_re[start_idx + decimation_factor * 2];
      r3 = pIn_re[start_idx + decimation_factor * 3];
      i0 = pIn_im[start_idx];
      i1 = pIn_im[start_idx + decimation_factor];
      i2 = pIn_im[start_idx + decimation_factor * 2];
      i3 = pIn_im[start_idx + decimation_factor * 3];
      // [1 1 1 1]
      pOut_re[start_idx] = (r0 + r1 + r2 + r3 + 1) >> 2;
      pOut_im[start_idx] = (i0 + i1 + i2 + i3 + 1) >> 2;
      // [1 -j -1 j]
      pOut_re[start_idx + decimation_factor] = (r0 + i1 - r2 - i3 + 1) >> 2;
      pOut_im[start_idx + decimation_factor] = (i0 - r1 - i2 + r3 + 1) >> 2;
      // [1 -1 1 -1]
      pOut_re[start_idx + decimation_factor * 2] = (r0 - r1 + r2 - r3 + 1) >> 2;
      pOut_im[start_idx + decimation_factor * 2] = (i0 - i1 + i2 - i3 + 1) >> 2;
      // [1 j -1 -j]
      pOut_re[start_idx + decimation_factor * 3] = (r0 - i1 - r2 + i3 + 1) >> 2;
      pOut_im[start_idx + decimation_factor * 3] = (i0 + r1 - i2 - r3 + 1) >> 2;
#ifdef PRINTF_DEBUG
      q = 0;
      printf("pOut_re pOut_im : %d	%d	%d	%d	%d	"
             "%d	%d	%d\n",
             pOut_re[start_idx + q], pOut_im[start_idx + q],
             pOut_re[decimation_factor + start_idx + q],
             pOut_im[decimation_factor + start_idx + q],
             pOut_re[(2 * decimation_factor) + start_idx + q],
             pOut_im[(2 * decimation_factor) + start_idx + q],
             pOut_re[(3 * decimation_factor) + start_idx + q],
             pOut_im[(3 * decimation_factor) + start_idx + q]);
#endif

      for (q = 1; q < decimation_factor; q++) {
        short w1_re, w2_re, w3_re;
        short w1_im, w2_im, w3_im;

        r0 = pIn_re[start_idx + q];
        r1 = pIn_re[start_idx + q + decimation_factor];
        r2 = pIn_re[start_idx + q + decimation_factor * 2];
        r3 = pIn_re[start_idx + q + decimation_factor * 3];
        i0 = pIn_im[start_idx + q];
        i1 = pIn_im[start_idx + q + decimation_factor];
        i2 = pIn_im[start_idx + q + decimation_factor * 2];
        i3 = pIn_im[start_idx + q + decimation_factor * 3];

        // [1 1 1 1]
        pr0 = r0 + r1 + r2 + r3;
        pi0 = i0 + i1 + i2 + i3;
        // [1 -j -1 j]
        pr1 = r0 + i1 - r2 - i3;
        pi1 = i0 - r1 - i2 + r3;
        // [1 -1 1 -1]
        pr2 = r0 - r1 + r2 - r3;
        pi2 = i0 - i1 + i2 - i3;
        // [1 j -1 -j]
        pr3 = r0 - i1 - r2 + i3;
        pi3 = i0 + r1 - i2 - r3;

        w1_re = *pCoeff++; // W_re[q*WPowerMultiplier];
        w1_im = *pCoeff++; // W_im[q*WPowerMultiplier];
        w2_re = *pCoeff++; // W_re[2*q*WPowerMultiplier];
        w2_im = *pCoeff++; // W_im[2*q*WPowerMultiplier];
        w3_re = *pCoeff++; // W_re[3*q*WPowerMultiplier];
        w3_im = *pCoeff++; // W_im[3*q*WPowerMultiplier];

        pOut_re[start_idx + q] = (pr0 + 1) >> 2;
        pOut_re[start_idx + q + decimation_factor] =
            (int)((pr1 * w1_re) >> 17) - ((pi1 * w1_im) >> 17);
        pOut_re[start_idx + q + decimation_factor * 2] =
            (int)((pr2 * w2_re) >> 17) - ((pi2 * w2_im) >> 17);
        pOut_re[start_idx + q + decimation_factor * 3] =
            (int)((pr3 * w3_re) >> 17) - ((pi3 * w3_im) >> 17);
        pOut_im[start_idx + q] = (pi0 + 1) >> 2;
        pOut_im[start_idx + q + decimation_factor] =
            (int)((pi1 * w1_re) >> 17) + ((pr1 * w1_im) >> 17);
        pOut_im[start_idx + q + decimation_factor * 2] =
            (int)((pi2 * w2_re) >> 17) + ((pr2 * w2_im) >> 17);
        pOut_im[start_idx + q + decimation_factor * 3] =
            (int)((pi3 * w3_re) >> 17) + ((pr3 * w3_im) >> 17);
#ifdef PRINTF_DEBUG
        printf("pOut_re pOut_im : %d	%d	%d	%d	%d	"
               "%d	%d	%d\n",
               pOut_re[start_idx + q], pOut_im[start_idx + q],
               pOut_re[decimation_factor + start_idx + q],
               pOut_im[decimation_factor + start_idx + q],
               pOut_re[(2 * decimation_factor) + start_idx + q],
               pOut_im[(2 * decimation_factor) + start_idx + q],
               pOut_re[(3 * decimation_factor) + start_idx + q],
               pOut_im[(3 * decimation_factor) + start_idx + q]);
#endif
      }
    }
    decimation_factor =
        decimation_factor >>
        2; // With subsequent stages decimation factor reduces by factor of 4
    WPowerMultiplier = WPowerMultiplier * 4;
    pIn_re = pOut_re;
    pIn_im = pOut_im;
    stage++;
    pOut_re = out_re; // +(x * 64);
    pOut_im = out_im; // +(x * 64);

    // Stage 2
    // pOut_re = A_re;
    // pOut_im = A_im;
    // decimation_factor = 1
    num_butterfly_groups = 64 / 4;
    for (group = 0; group < num_butterfly_groups; group++) {
      int start_idx =
          group * 4; // start-index for the current group in the current stage
      r0 = pIn_re[start_idx];
      r1 = pIn_re[start_idx + 1];
      r2 = pIn_re[start_idx + 2];
      r3 = pIn_re[start_idx + 3];
      i0 = pIn_im[start_idx];
      i1 = pIn_im[start_idx + 1];
      i2 = pIn_im[start_idx + 2];
      i3 = pIn_im[start_idx + 3];
      // [1 1 1 1]
      pOut_re[out_idx[start_idx] * 64 + x] = (r0 + r1 + r2 + r3);
      pOut_im[out_idx[start_idx] * 64 + x] = (i0 + i1 + i2 + i3);
      // [1 -j -1 j]
      pOut_re[out_idx[start_idx + 1] * 64 + x] = (r0 + i1 - r2 - i3);
      pOut_im[out_idx[start_idx + 1] * 64 + x] = (i0 - r1 - i2 + r3);
      // [1 -1 1 -1]
      pOut_re[out_idx[start_idx + 2] * 64 + x] = (r0 - r1 + r2 - r3);
      pOut_im[out_idx[start_idx + 2] * 64 + x] = (i0 - i1 + i2 - i3);
      // [1 j -1 -j]
      pOut_re[out_idx[start_idx + 3] * 64 + x] = (r0 - i1 - r2 + i3);
      pOut_im[out_idx[start_idx + 3] * 64 + x] = (i0 + r1 - i2 - r3);

#ifdef PRINTF_DEBUG
      printf("pOut_re pOut_im : %d	%d	%d	%d	%d	"
             "%d	%d	%d\n",
             pOut_re[start_idx], pOut_im[start_idx],
             pOut_re[decimation_factor + start_idx],
             pOut_im[decimation_factor + start_idx],
             pOut_re[(2 * decimation_factor) + start_idx],
             pOut_im[(2 * decimation_factor) + start_idx],
             pOut_re[(3 * decimation_factor) + start_idx],
             pOut_im[(3 * decimation_factor) + start_idx]);
#endif
    }

    // Initialize at q position to begin with in input
  }
}

void fft_radix4_32Bit(int N, COMPLEX32 *X) {
  COMPLEX WF[64];
  double arg;
  long long acc_real, acc_imag;
  int cnt;
  for (cnt = 0; cnt < N; cnt++) {
    arg = (2 * PI * cnt) / N;
    WF[cnt].real = (short)((double)32767.0 * cos(arg));
    WF[cnt].imag = -(short)((double)32767.0 * sin(arg));
  }

  // generate stage 1 output here
  COMPLEX32 a_16[NUMPOINTS / 4], b_16[NUMPOINTS / 4], c_16[NUMPOINTS / 4],
      d_16[NUMPOINTS / 4];
  // g_64(cnt)+g_64(cnt+16)+g_64(cnt+32)+g_64(cnt+48)
  for (cnt = 0; cnt < (N / 4); cnt++) {
    a_16[cnt].real = (X[cnt].real + X[cnt + 16].real + X[cnt + 32].real +
                      X[cnt + 48].real) >>
                     2; // (g_64(cnt)+g_64(cnt+16)+g_64(cnt+32)+g_64(cnt+48))
    a_16[cnt].imag = (X[cnt].imag + X[cnt + 16].imag + X[cnt + 32].imag +
                      X[cnt + 48].imag) >>
                     2; // (g_64(cnt)+g_64(cnt+16)+g_64(cnt+32)+g_64(cnt+48))
  }
  // g_64(cnt)-1i*g_64(cnt+16)-g_64(cnt+32)+1i*g_64(cnt+48)
  for (cnt = 0; cnt < (N / 4); cnt++) {
    b_16[cnt].real =
        (X[cnt].real + X[cnt + 16].imag - X[cnt + 32].real -
         X[cnt + 48]
             .imag); // (g_64(cnt)+g_64(cnt+16)+g_64(cnt+32)+g_64(cnt+48))
    b_16[cnt].imag =
        (X[cnt].imag - X[cnt + 16].real - X[cnt + 32].imag +
         X[cnt + 48]
             .real); // (g_64(cnt)+g_64(cnt+16)+g_64(cnt+32)+g_64(cnt+48))
  }
  // temp_data(cnt)*w_64(cnt)
  for (cnt = 0; cnt < (N / 4); cnt++) {
    acc_real = ((WF[cnt].real * (long long)b_16[cnt].real) >> 17) -
               ((WF[cnt].imag * (long long)b_16[cnt].imag) >> 17);
    acc_imag = ((WF[cnt].imag * (long long)b_16[cnt].real) >> 17) +
               ((WF[cnt].real * (long long)b_16[cnt].imag) >> 17);
    b_16[cnt].real = (int)(acc_real); //>> 15 does not work for double number
    b_16[cnt].imag = (int)(acc_imag);
  }
  //(g_64(cnt)-g_64(cnt+16)+g_64(cnt+32)-g_64(cnt+48))
  for (cnt = 0; cnt < (N / 4); cnt++) {
    c_16[cnt].real =
        X[cnt].real - X[cnt + 16].real + X[cnt + 32].real -
        X[cnt + 48].real; // (g_64(cnt)+g_64(cnt+16)+g_64(cnt+32)+g_64(cnt+48))
    c_16[cnt].imag =
        X[cnt].imag - X[cnt + 16].imag + X[cnt + 32].imag -
        X[cnt + 48].imag; // (g_64(cnt)+g_64(cnt+16)+g_64(cnt+32)+g_64(cnt+48))
  }
  // temp_data(cnt)*w_64(cnt*2-1)
  for (cnt = 0; cnt < (N / 4); cnt++) {
    acc_real = ((WF[cnt * 2].real * (long long)c_16[cnt].real) >> 17) -
               ((WF[cnt * 2].imag * (long long)c_16[cnt].imag) >> 17);
    acc_imag = ((WF[cnt * 2].imag * (long long)c_16[cnt].real) >> 17) +
               ((WF[cnt * 2].real * (long long)c_16[cnt].imag) >> 17);
    c_16[cnt].real = (int)(acc_real); //>> 15 does not work for double number
    c_16[cnt].imag = (int)(acc_imag);
  }

  //(g_64(cnt)+1i*g_64(cnt+16)-g_64(cnt+32)-1i*g_64(cnt+48))
  for (cnt = 0; cnt < (N / 4); cnt++) {
    d_16[cnt].real =
        X[cnt].real - X[cnt + 16].imag - X[cnt + 32].real +
        X[cnt + 48].imag; // (g_64(cnt)+g_64(cnt+16)+g_64(cnt+32)+g_64(cnt+48))
    d_16[cnt].imag =
        X[cnt].imag + X[cnt + 16].real - X[cnt + 32].imag -
        X[cnt + 48].real; // (g_64(cnt)+g_64(cnt+16)+g_64(cnt+32)+g_64(cnt+48))
  }
  // temp_data(cnt)*w_64(cnt*3-2)
  for (cnt = 0; cnt < (N / 4); cnt++) {
    acc_real = ((WF[cnt * 3].real * (long long)d_16[cnt].real) >> 17) -
               ((WF[cnt * 3].imag * (long long)d_16[cnt].imag) >> 17);
    acc_imag = ((WF[cnt * 3].imag * (long long)d_16[cnt].real) >> 17) +
               ((WF[cnt * 3].real * (long long)d_16[cnt].imag) >> 17);
    d_16[cnt].real = (int)(acc_real); //>> 15 does not work for double number
    d_16[cnt].imag = (int)(acc_imag);
  }
  ////end of stage 1 output generation --a_16 b_16 c_16 d_16
  COMPLEX32 S0[4], S1[4], S2[4], S3[4], S4[4], S5[4], S6[4], S7[4], S8[4],
      S9[4], S10[4], S11[4], S12[4], S13[4], S14[4], S15[4];
  COMPLEX32 SX[64];
  //(a1_16(cnt)+a1_16(cnt+4)+a1_16(cnt+8)+a1_16(cnt+12))

  // stage2 --part1
  for (cnt = 0; cnt < ((N) / 16); cnt++) {
    S0[cnt].real = (a_16[cnt].real + a_16[cnt + 4].real + a_16[cnt + 8].real +
                    a_16[cnt + 12].real) >>
                   2;
    S0[cnt].imag = (a_16[cnt].imag + a_16[cnt + 4].imag + a_16[cnt + 8].imag +
                    a_16[cnt + 12].imag) >>
                   2;
    SX[cnt + 0].real = S0[cnt].real;
    SX[cnt + 0].imag = S0[cnt].imag;
  }

  //(a1_16(cnt)-1i*a1_16(cnt+4)-a1_16(cnt+8)+1i*a1_16(cnt+12))
  for (cnt = 0; cnt < ((N) / 16); cnt++) {
    S1[cnt].real = a_16[cnt].real + a_16[cnt + 4].imag - a_16[cnt + 8].real -
                   a_16[cnt + 12].imag;
    S1[cnt].imag = a_16[cnt].imag - a_16[cnt + 4].real - a_16[cnt + 8].imag +
                   a_16[cnt + 12].real;
  }
  // temp_data(1:4)*w_64(1:4:16)
  for (cnt = 0; cnt < ((N) / 16); cnt++) {
    acc_real = ((WF[4 * cnt].real * (long long)S1[cnt].real) >> 17) -
               ((WF[4 * cnt].imag * (long long)S1[cnt].imag) >> 17);
    acc_imag = ((WF[4 * cnt].imag * (long long)S1[cnt].real) >> 17) +
               ((WF[4 * cnt].real * (long long)S1[cnt].imag) >> 17);
    SX[cnt + 4].real = (int)(acc_real); //>> 15 does not work for double number
    SX[cnt + 4].imag = (int)(acc_imag);
  }

  //(a1_16(cnt)-a1_16(cnt+4)+a1_16(cnt+8)-a1_16(cnt+12))
  for (cnt = 0; cnt < ((N) / 16); cnt++) {
    S2[cnt].real = a_16[cnt].real - a_16[cnt + 4].real + a_16[cnt + 8].real -
                   a_16[cnt + 12].real;
    S2[cnt].imag = a_16[cnt].imag - a_16[cnt + 4].imag + a_16[cnt + 8].imag -
                   a_16[cnt + 12].imag;
  }
  // temp_data(1:4)*w_64(1:8:32)
  for (cnt = 0; cnt < ((N) / 16); cnt++) {
    acc_real = ((WF[8 * cnt].real * (long long)S2[cnt].real) >> 17) -
               ((WF[8 * cnt].imag * (long long)S2[cnt].imag) >> 17);
    acc_imag = ((WF[8 * cnt].imag * (long long)S2[cnt].real) >> 17) +
               ((WF[8 * cnt].real * (long long)S2[cnt].imag) >> 17);
    SX[cnt + 8].real = (int)(acc_real); //>> 15 does not work for double number
    SX[cnt + 8].imag = (int)(acc_imag);
  }

  //(a1_16(cnt)+1i*a1_16(cnt+4)-a1_16(cnt+8)-1i*a1_16(cnt+12))
  for (cnt = 0; cnt < ((N) / 16); cnt++) {
    S3[cnt].real = a_16[cnt].real - a_16[cnt + 4].imag - a_16[cnt + 8].real +
                   a_16[cnt + 12].imag;
    S3[cnt].imag = a_16[cnt].imag + a_16[cnt + 4].real - a_16[cnt + 8].imag -
                   a_16[cnt + 12].real;
  }
  // temp_data(1:4)*w_64(1:12:48)
  for (cnt = 0; cnt < ((N) / 16); cnt++) {
    acc_real = ((WF[12 * cnt].real * (long long)S3[cnt].real) >> 17) -
               ((WF[12 * cnt].imag * (long long)S3[cnt].imag) >> 17);
    acc_imag = ((WF[12 * cnt].imag * (long long)S3[cnt].real) >> 17) +
               ((WF[12 * cnt].real * (long long)S3[cnt].imag) >> 17);
    SX[cnt + 12].real = (int)(acc_real); //>> 15 does not work for double number
    SX[cnt + 12].imag = (int)(acc_imag);
  }

  // stage 2 --part2

  for (cnt = 0; cnt < ((N) / 16); cnt++) {
    S4[cnt].real = (b_16[cnt].real + b_16[cnt + 4].real + b_16[cnt + 8].real +
                    b_16[cnt + 12].real) >>
                   2;
    S4[cnt].imag = (b_16[cnt].imag + b_16[cnt + 4].imag + b_16[cnt + 8].imag +
                    b_16[cnt + 12].imag) >>
                   2;
    SX[cnt + 16].real = (int)S4[cnt].real;
    SX[cnt + 16].imag = (int)S4[cnt].imag;
  }
  //(a1_16(cnt)-1i*a1_16(cnt+4)-a1_16(cnt+8)+1i*a1_16(cnt+12))
  for (cnt = 0; cnt < ((N) / 16); cnt++) {
    S5[cnt].real = b_16[cnt].real + b_16[cnt + 4].imag - b_16[cnt + 8].real -
                   b_16[cnt + 12].imag;
    S5[cnt].imag = b_16[cnt].imag - b_16[cnt + 4].real - b_16[cnt + 8].imag +
                   b_16[cnt + 12].real;
  }
  // temp_data(1:4)*w_64(1:4:16)
  for (cnt = 0; cnt < ((N) / 16); cnt++) {
    acc_real = ((WF[4 * cnt].real * (long long)S5[cnt].real) >> 17) -
               ((WF[4 * cnt].imag * (long long)S5[cnt].imag) >> 17);
    acc_imag = ((WF[4 * cnt].imag * (long long)S5[cnt].real) >> 17) +
               ((WF[4 * cnt].real * (long long)S5[cnt].imag) >> 17);
    SX[cnt + 20].real = (int)(acc_real); //>> 15 does not work for double number
    SX[cnt + 20].imag = (int)(acc_imag);
  }

  //(a1_16(cnt)-a1_16(cnt+4)+a1_16(cnt+8)-a1_16(cnt+12))
  for (cnt = 0; cnt < ((N) / 16); cnt++) {
    S6[cnt].real = b_16[cnt].real - b_16[cnt + 4].real + b_16[cnt + 8].real -
                   b_16[cnt + 12].real;
    S6[cnt].imag = b_16[cnt].imag - b_16[cnt + 4].imag + b_16[cnt + 8].imag -
                   b_16[cnt + 12].imag;
  }
  // temp_data(1:4)*w_64(1:8:32)
  for (cnt = 0; cnt < ((N) / 16); cnt++) {
    acc_real = ((WF[8 * cnt].real * (long long)S6[cnt].real) >> 17) -
               ((WF[8 * cnt].imag * (long long)S6[cnt].imag) >> 17);
    acc_imag = ((WF[8 * cnt].imag * (long long)S6[cnt].real) >> 17) +
               ((WF[8 * cnt].real * (long long)S6[cnt].imag) >> 17);
    SX[cnt + 24].real = (int)(acc_real); //>> 15 does not work for double number
    SX[cnt + 24].imag = (int)(acc_imag);
  }

  //(a1_16(cnt)+1i*a1_16(cnt+4)-a1_16(cnt+8)-1i*a1_16(cnt+12))
  for (cnt = 0; cnt < ((N) / 16); cnt++) {
    S7[cnt].real = b_16[cnt].real - b_16[cnt + 4].imag - b_16[cnt + 8].real +
                   b_16[cnt + 12].imag;
    S7[cnt].imag = b_16[cnt].imag + b_16[cnt + 4].real - b_16[cnt + 8].imag -
                   b_16[cnt + 12].real;
  }
  // temp_data(1:4)*w_64(1:12:48)
  for (cnt = 0; cnt < ((N) / 16); cnt++) {
    acc_real = ((WF[12 * cnt].real * (long long)S7[cnt].real) >> 17) -
               ((WF[12 * cnt].imag * (long long)S7[cnt].imag) >> 17);
    acc_imag = ((WF[12 * cnt].imag * (long long)S7[cnt].real) >> 17) +
               ((WF[12 * cnt].real * (long long)S7[cnt].imag) >> 17);
    SX[cnt + 28].real = (int)(acc_real); //>> 15 does not work for double number
    SX[cnt + 28].imag = (int)(acc_imag);
  }

  /// compute stage 2 out - part 3

  for (cnt = 0; cnt < ((N) / 16); cnt++) {
    S8[cnt].real = (c_16[cnt].real + c_16[cnt + 4].real + c_16[cnt + 8].real +
                    c_16[cnt + 12].real) >>
                   2;
    S8[cnt].imag = (c_16[cnt].imag + c_16[cnt + 4].imag + c_16[cnt + 8].imag +
                    c_16[cnt + 12].imag) >>
                   2;
    SX[cnt + 32].real = (int)S8[cnt].real;
    SX[cnt + 32].imag = (int)S8[cnt].imag;
  }

  //(a1_16(cnt)-1i*a1_16(cnt+4)-a1_16(cnt+8)+1i*a1_16(cnt+12))
  for (cnt = 0; cnt < ((N) / 16); cnt++) {
    S9[cnt].real = (c_16[cnt].real + c_16[cnt + 4].imag - c_16[cnt + 8].real -
                    c_16[cnt + 12].imag);
    S9[cnt].imag = (c_16[cnt].imag - c_16[cnt + 4].real - c_16[cnt + 8].imag +
                    c_16[cnt + 12].real);
  }
  // temp_data(1:4)*w_64(1:4:16)
  for (cnt = 0; cnt < ((N) / 16); cnt++) {
    acc_real = ((WF[4 * cnt].real * (long long)S9[cnt].real) >> 17) -
               ((WF[4 * cnt].imag * (long long)S9[cnt].imag) >> 17);
    acc_imag = ((WF[4 * cnt].imag * (long long)S9[cnt].real) >> 17) +
               ((WF[4 * cnt].real * (long long)S9[cnt].imag) >> 17);
    SX[cnt + 36].real =
        (int)((acc_real)); //>> 15 does not work for double number
    SX[cnt + 36].imag = (int)((acc_imag));
  }

  //(a1_16(cnt)-a1_16(cnt+4)+a1_16(cnt+8)-a1_16(cnt+12))
  for (cnt = 0; cnt < ((N) / 16); cnt++) {
    S10[cnt].real = c_16[cnt].real - c_16[cnt + 4].real + c_16[cnt + 8].real -
                    c_16[cnt + 12].real;
    S10[cnt].imag = c_16[cnt].imag - c_16[cnt + 4].imag + c_16[cnt + 8].imag -
                    c_16[cnt + 12].imag;
  }
  // temp_data(1:4)*w_64(1:8:32)
  for (cnt = 0; cnt < ((N) / 16); cnt++) {
    acc_real = ((WF[8 * cnt].real * (long long)S10[cnt].real) >> 17) -
               ((WF[8 * cnt].imag * (long long)S10[cnt].imag) >> 17);
    acc_imag = ((WF[8 * cnt].imag * (long long)S10[cnt].real) >> 17) +
               ((WF[8 * cnt].real * (long long)S10[cnt].imag) >> 17);
    SX[cnt + 40].real = (int)(acc_real); //>> 15 does not work for double number
    SX[cnt + 40].imag = (int)(acc_imag);
  }

  //(a1_16(cnt)+1i*a1_16(cnt+4)-a1_16(cnt+8)-1i*a1_16(cnt+12))
  for (cnt = 0; cnt < ((N) / 16); cnt++) {
    S11[cnt].real = c_16[cnt].real - c_16[cnt + 4].imag - c_16[cnt + 8].real +
                    c_16[cnt + 12].imag;
    S11[cnt].imag = c_16[cnt].imag + c_16[cnt + 4].real - c_16[cnt + 8].imag -
                    c_16[cnt + 12].real;
  }
  // temp_data(1:4)*w_64(1:12:48)
  for (cnt = 0; cnt < ((N) / 16); cnt++) {
    acc_real = ((WF[12 * cnt].real * (long long)S11[cnt].real) >> 17) -
               ((WF[12 * cnt].imag * (long long)S11[cnt].imag) >> 17);
    acc_imag = ((WF[12 * cnt].imag * (long long)S11[cnt].real) >> 17) +
               ((WF[12 * cnt].real * (long long)S11[cnt].imag) >> 17);
    SX[cnt + 44].real = (int)(acc_real); //>> 15 does not work for double number
    SX[cnt + 44].imag = (int)(acc_imag);
  }

  /// compute stage 2 out - part 4

  for (cnt = 0; cnt < ((N) / 16); cnt++) {
    S12[cnt].real = (d_16[cnt].real + d_16[cnt + 4].real + d_16[cnt + 8].real +
                     d_16[cnt + 12].real) >>
                    2;
    S12[cnt].imag = (d_16[cnt].imag + d_16[cnt + 4].imag + d_16[cnt + 8].imag +
                     d_16[cnt + 12].imag) >>
                    2;
    SX[cnt + 48].real = (int)S12[cnt].real;
    SX[cnt + 48].imag = (int)S12[cnt].imag;
  }

  //(a1_16(cnt)-1i*a1_16(cnt+4)-a1_16(cnt+8)+1i*a1_16(cnt+12))
  for (cnt = 0; cnt < ((N) / 16); cnt++) {
    S13[cnt].real = d_16[cnt].real + d_16[cnt + 4].imag - d_16[cnt + 8].real -
                    d_16[cnt + 12].imag;
    S13[cnt].imag = d_16[cnt].imag - d_16[cnt + 4].real - d_16[cnt + 8].imag +
                    d_16[cnt + 12].real;
  }
  // temp_data(1:4)*w_64(1:4:16)
  for (cnt = 0; cnt < ((N) / 16); cnt++) {
    acc_real = ((WF[4 * cnt].real * (long long)S13[cnt].real) >> 17) -
               ((WF[4 * cnt].imag * (long long)S13[cnt].imag) >> 17);
    acc_imag = ((WF[4 * cnt].imag * (long long)S13[cnt].real) >> 17) +
               ((WF[4 * cnt].real * (long long)S13[cnt].imag) >> 17);
    SX[cnt + 52].real = (int)(acc_real); //>> 15 does not work for double number
    SX[cnt + 52].imag = (int)(acc_imag);
  }

  //(a1_16(cnt)-a1_16(cnt+4)+a1_16(cnt+8)-a1_16(cnt+12))
  for (cnt = 0; cnt < ((N) / 16); cnt++) {
    S14[cnt].real = d_16[cnt].real - d_16[cnt + 4].real + d_16[cnt + 8].real -
                    d_16[cnt + 12].real;
    S14[cnt].imag = d_16[cnt].imag - d_16[cnt + 4].imag + d_16[cnt + 8].imag -
                    d_16[cnt + 12].imag;
  }
  // temp_data(1:4)*w_64(1:8:32)
  for (cnt = 0; cnt < ((N) / 16); cnt++) {
    acc_real = ((WF[8 * cnt].real * (long long)S14[cnt].real) >> 17) -
               ((WF[8 * cnt].imag * (long long)S14[cnt].imag) >> 17);
    acc_imag = ((WF[8 * cnt].imag * (long long)S14[cnt].real) >> 17) +
               ((WF[8 * cnt].real * (long long)S14[cnt].imag) >> 17);
    SX[cnt + 56].real = (int)(acc_real); //>> 15 does not work for double number
    SX[cnt + 56].imag = (int)(acc_imag);
  }

  //(a1_16(cnt)+1i*a1_16(cnt+4)-a1_16(cnt+8)-1i*a1_16(cnt+12))
  for (cnt = 0; cnt < ((N) / 16); cnt++) {
    S15[cnt].real = d_16[cnt].real - d_16[cnt + 4].imag - d_16[cnt + 8].real +
                    d_16[cnt + 12].imag;
    S15[cnt].imag = d_16[cnt].imag + d_16[cnt + 4].real - d_16[cnt + 8].imag -
                    d_16[cnt + 12].real;
  }
  // temp_data(1:4)*w_64(1:12:48)
  for (cnt = 0; cnt < ((N) / 16); cnt++) {
    acc_real = ((WF[12 * cnt].real * (long long)S15[cnt].real) >> 17) -
               ((WF[12 * cnt].imag * (long long)S15[cnt].imag) >> 17);
    acc_imag = ((WF[12 * cnt].imag * (long long)S15[cnt].real) >> 17) +
               ((WF[12 * cnt].real * (long long)S15[cnt].imag) >> 17);
    SX[cnt + 60].real = (int)(acc_real); //>> 15 does not work for double number
    SX[cnt + 60].imag = (int)(acc_imag);
  }

  for (cnt = 0; cnt < N; cnt += 4) {
    // res(cnt)    =  s_all(cnt) +     s_all(cnt+1) +  s_all(cnt+2) +
    // s_all(cnt+3);
    X[cnt].real =
        (SX[cnt].real + SX[cnt + 1].real + SX[cnt + 2].real + SX[cnt + 3].real);
    X[cnt].imag =
        (SX[cnt].imag + SX[cnt + 1].imag + SX[cnt + 2].imag + SX[cnt + 3].imag);

    // res(cnt+1)  =  s_all(cnt) -  1i*s_all(cnt+1) -  s_all(cnt+2) +
    // 1i*s_all(cnt+3);
    X[cnt + 1].real =
        (SX[cnt].real + SX[cnt + 1].imag - SX[cnt + 2].real - SX[cnt + 3].imag);
    X[cnt + 1].imag =
        (SX[cnt].imag - SX[cnt + 1].real - SX[cnt + 2].imag + SX[cnt + 3].real);

    // res(cnt+2)  =  s_all(cnt) -     s_all(cnt+1) +  s_all(cnt+2) -
    // s_all(cnt+3);
    X[cnt + 2].real =
        (SX[cnt].real - SX[cnt + 1].real + SX[cnt + 2].real - SX[cnt + 3].real);
    X[cnt + 2].imag =
        (SX[cnt].imag - SX[cnt + 1].imag + SX[cnt + 2].imag - SX[cnt + 3].imag);

    // res(cnt+3)  =  s_all(cnt) +  1j*s_all(cnt+1) -  s_all(cnt+2) -
    // 1i*s_all(cnt+3);
    X[cnt + 3].real =
        (SX[cnt].real - SX[cnt + 1].imag - SX[cnt + 2].real + SX[cnt + 3].imag);
    X[cnt + 3].imag =
        (SX[cnt].imag + SX[cnt + 1].real - SX[cnt + 2].imag - SX[cnt + 3].real);
  }
}

void fft128_horizontal_complex_Cref(int *Out_re, int *Out_im,
                                    int *Out_complex) {
  int k;
  COMPLEX32 x_ref1[64];
  COMPLEX32 x_ref1_BR[64];
  int n = 64, j;
  int BitOrder[64] = {0,  16, 32, 48, 4,  20, 36, 52, 8,  24, 40, 56, 12,
                      28, 44, 60, 1,  17, 33, 49, 5,  21, 37, 53, 9,  25,
                      41, 57, 13, 29, 45, 61, 2,  18, 34, 50, 6,  22, 38,
                      54, 10, 26, 42, 58, 14, 30, 46, 62, 3,  19, 35, 51,
                      7,  23, 39, 55, 11, 27, 43, 59, 15, 31, 47, 63};

  for (j = 0; j < n; j++) {
    for (k = 0; k < 64; k++) {
      x_ref1[k].real = Out_re[(n * j) + k] >> 2;
      x_ref1[k].imag = Out_im[(n * j) + k] >> 2;
    }

    fft_radix4_32Bit(64, &x_ref1[0]);
    for (k = 0; k < 64; k++) {
      x_ref1_BR[k] = x_ref1[BitOrder[k]];
    }
    for (k = 0; k < 64; k++) {
      Out_complex[(n * j * 2) + (2 * k)] = x_ref1_BR[k].real;
      Out_complex[(n * j * 2) + (2 * k + 1)] = x_ref1_BR[k].imag;
    }
  }
}

// Note there a scaling by 1/N in iDFT
// Inplace computation

void idftDoublePass(int N, fCOMPLEX *X) {

  int n, k, i;
  double arg;
  double Xr[64];
  double Xi[64];
  double Wr, Wi;
  int stride = N;

  // first horizontal inverse DFT
  for (i = 0; i < N; i++) {

    for (k = 0; k < N; k++) {
      Xr[k] = 0;
      Xi[k] = 0;
      for (n = 0; n < N; n++) {
        arg = (2 * PI * k * n) / (double)N;
        Wr = cos(arg);
        Wi = sin(arg);
        Xr[k] = Xr[k] + (X[n + (i * stride)].real * Wr) -
                (X[n + (i * stride)].imag * Wi);
        Xi[k] = Xi[k] + (X[n + (i * stride)].imag * Wr) +
                (X[n + (i * stride)].real * Wi);
      }
    }

    for (k = 0; k < N; k++) {
      X[k + (i * stride)].real = Xr[k] / (double)N;
      X[k + (i * stride)].imag = Xi[k] / (double)N;
    }
  }

  // vertical inverse DFT
  for (i = 0; i < N; i++) {
    for (k = 0; k < N; k++) {
      Xr[k] = 0;
      Xi[k] = 0;
      for (n = 0; n < N; n++) {
        arg = (2 * PI * k * n) / (double)N;
        Wr = cos(arg);
        Wi = sin(arg);
        Xr[k] = Xr[k] + (X[(n * stride) + i].real * Wr) -
                (X[(n * stride) + i].imag * Wi);
        Xi[k] = Xi[k] + (X[(n * stride) + i].imag * Wr) +
                (X[(n * stride) + i].real * Wi);
      }
    }

    for (k = 0; k < N; k++) {
      X[(k * stride) + i].real = Xr[k] / (double)N;
      X[(k * stride) + i].imag = Xi[k] / (double)N;
    }
  }

  return;
}
