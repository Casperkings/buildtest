  const int64 low = *(__constant int64 *)mask_lo;
  const int64 hi = *(__constant int64 *)mask_hi;
  __local uchar *pSrc1 = local_src1;
#pragma nounroll
  for (int y = 0; y < inTileH; y += 64) {
    __local uchar *pSrc = local_src + y * local_src_pitch;
    __local int *pDst = (__local int *)local_dst + y;
#pragma nounroll
    for (idx = 0; idx < inTileW; idx += 64) {
      short64 xT = convert_short64(*((__local uchar64 *)(pSrc1 + idx)));
      __local int *pDst_tmp = pDst;
      for (int x = 0; x < 64; ++x) {
        short64 xS = convert_short64(
                       *((__local uchar64 *)(pSrc + x*inTilePitch + idx)));
        ushort64 xS_T = abs_diff(xS, xT);
        ushort64 sqDiff = xS_T * xS_T;
        uint res_lo = xt_reduce_add_masked(sqDiff, low);
        uint res_hi = xt_reduce_add_masked(sqDiff, hi);
        *pDst_tmp = res_lo;
        *(pDst_tmp + local_dst_pitch) = res_hi;
        pDst_tmp++;
      }
      pDst += 2*local_dst_pitch;
    }
  }
