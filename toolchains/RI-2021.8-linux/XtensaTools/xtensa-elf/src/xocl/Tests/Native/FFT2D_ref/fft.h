#ifndef FFT_H
#define FFT_H

#define PI 3.141592653589793 /* definition of pi */

typedef struct {
  int real;
  int imag;
} COMPLEX32;
#define NUMPOINTS 64
typedef struct {
  short real;
  short imag;
} COMPLEX;

typedef struct {
  double real;
  double imag;
} fCOMPLEX;

void init_radix4Int_64(int n, short *W_64, int *out_idx);
void idftDoublePass(int N, fCOMPLEX *X);
void fft_radix4_vertical_64(int *A_re, short *W_64, int *out_idx, int *pTemp,
                            int *out_re, int *out_im);
void fft128_horizontal_complex_Cref(int *Out_re, int *Out_im, int *Out_complex);

#endif /*  */
