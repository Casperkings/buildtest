{
  extern void xvfft128_horizontal_complex(__local int *A_re, __local int *A_im,
                                          __local int *Out, __local int *Temp,
                                          __constant int *tw_ptr);

  xvfft128_horizontal_complex(local_srcRe, local_srcIm, local_dst, Temp,
                              tw_ptr);
}
