{
  extern void xvFFT64_radix4_S32_OS32_vp6(
      __local int *A_re, __constant short *W_64, __constant int *out_idx,
      int *pTemp, __local int *Out_re, __local int *Out_im);

  xvFFT64_radix4_S32_OS32_vp6(local_src, W64, Out_idx, TempMem, local_dstRe,
                              local_dstIm);
}
