  const int32 low = *(__constant int32 *)mask_lo;
  const int32 hi = *(__constant int32 *)mask_hi;
  __local uchar *pSrc1 = local_src1;
#pragma nounroll
  for (int y = 0; y < inTileH; y += 32) {
    __local uchar *pSrc = local_src + y * local_src_pitch;
    __local int *pDst = (__local int *)local_dst + y;
#pragma nounroll
    for (idx = 0; idx < inTileW; idx += 32) {
      short32 xT = convert_short32(*((__local uchar32 *)(pSrc1 + idx)));
      __local int *pDst_tmp = pDst;
      for (int x = 0; x < 32; ++x) {
        short32 xS = convert_short32(
                       *((__local uchar32 *)(pSrc + x*inTilePitch + idx)));
        ushort32 xS_T = abs_diff(xS, xT);
        ushort32 sqDiff = xS_T * xS_T;
        uint res_lo = xt_reduce_add_masked(sqDiff, low);
        uint res_hi = xt_reduce_add_masked(sqDiff, hi);
        *pDst_tmp = res_lo;
        *(pDst_tmp + local_dst_pitch) = res_hi;
        pDst_tmp++;
      }
      pDst += 2*local_dst_pitch;
    }
  }
