{
  extern void conv5x5xD8b_P6(
      __local char *__restrict pIn, __local char *__restrict pCoeff,
      __local char *__restrict pOut, int width, int height, int depth,
      int pitch, int odepth, int shift);

  conv5x5xD8b_P6(local_src, local_coef, local_dst, inTileW + pad + pad,
                 inTileH + pad + pad, depth, inTileW + pad + pad, depth, shift);
}
