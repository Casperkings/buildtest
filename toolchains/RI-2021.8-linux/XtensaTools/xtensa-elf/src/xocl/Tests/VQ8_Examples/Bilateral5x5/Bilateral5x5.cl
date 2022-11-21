#define TILE_W 256
#define TILE_H 64
#define W_EXT 2
#define H_EXT 2
#define IMAGE_PAD 2
#define DST_BIT_DEPTH 8
#define LUT_SHIFT 2

__kernel __attribute__((reqd_work_group_size(1, 1, 1)))
__attribute__((reqd_work_dim_size(2))) void
oclKernel(__global uchar *restrict Src, __constant uchar *restrict Coef,
          int SrcPitch, __global uchar *restrict Dst, const int DstPitch,
          int width, int height, int dstWidth, int dstHeight,
          __local uchar *restrict local_src, __local uchar *restrict local_dst,
          __global int *restrict err_codes) {
  int i, j;
  int idx, idy;

  int local_src_pitch, local_dst_pitch;
  int pad = IMAGE_PAD;
  int inTilePitch;
  int inTileW, inTileH, outTileW, outTileH;
  int dstBytes;
  int err = 0;

  inTileW = TILE_W;
  inTileH = TILE_H;
  inTilePitch = inTileW + 2 * W_EXT;
  local_src_pitch = inTileW + pad + pad;

  outTileW = TILE_W;
  outTileH = TILE_H;
  local_dst_pitch = outTileW;
  dstBytes = DST_BIT_DEPTH >> 3;

  i = get_global_id(0) * inTileH;
  j = get_global_id(1) * inTileW;

  event_t evtA = xt_async_work_group_2d_copy(
      local_src, Src + (i * SrcPitch) + j, local_src_pitch,
      (inTileH + pad + pad), local_src_pitch, SrcPitch, 0);
  wait_group_events(1, &evtA);
  ushort tempDiv[3 * (TILE_H+1) * (XCHAL_VISION_SIMD16*2)]
         __attribute__((aligned(XCHAL_VISION_SIMD16*2)));
#if NATIVE_KERNEL
  #include "Bilateral5x5_ivp.c"
#else
#ifdef XT_OCL_EXT
  #include "Bilateral5x5_ext.cl"
#else
  const ushort64 maskshift1 = {
    1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
    33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
    49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64};

  ushort64 maskshift2 = maskshift1 + (ushort64)1;
  ushort64 maskshift3 = maskshift2 + (ushort64)1;
  ushort64 maskshift4 = maskshift3 + (ushort64)1;

  // kernel using standard OpenCL C
  for (idx = 0; idx < inTileW; idx += 64) {
    __local uchar *pDst = local_dst + idx;
    for (int y = 0; y < inTileH; ++y) {
      uint64 Filt = (uint64)0;
      ushort64 wtSum = (ushort64)0;

      __local uchar *pSrc2 = local_src + (y + 2) * local_src_pitch + idx;
      ushort64 x20 = convert_ushort64(vload64(0, pSrc2));
      ushort64 xtail = convert_ushort64(vload64(1, pSrc2));
      ushort64 x22 = shuffle2(x20, xtail, maskshift2);

      __local uchar *pSrc = local_src + (y)*local_src_pitch + idx;
      __constant uchar64 *pCoef = (__constant uchar64 *)Coef;

      for (int iy = 0; iy < 5; iy++) {
        ushort64 x00 = convert_ushort64(vload64(0, pSrc));
        ushort64 xtail0 = convert_ushort64(vload64(1, pSrc));
        pSrc += local_src_pitch;
        ushort64 x0 = x00;
        ushort64 x1 = shuffle2(x00, xtail0, maskshift1);
        ushort64 x2 = shuffle2(x00, xtail0, maskshift2);
        ushort64 x3 = shuffle2(x00, xtail0, maskshift3);
        ushort64 x4 = shuffle2(x00, xtail0, maskshift4);

        ushort64 Coef_A0 = convert_ushort64(pCoef[0]);
        ushort64 Coef_B0 = convert_ushort64(pCoef[1]);

        ushort64 diff0 = abs_diff(x0, x22);
        diff0 = diff0 >> LUT_SHIFT;
        ushort64 coef0 = shuffle2(Coef_A0, Coef_B0, diff0);
        wtSum += coef0;
        ushort64 tmpMul0 = x0 * coef0;
        Filt += convert_uint64(tmpMul0);

        ushort64 Coef_A1 = convert_ushort64(pCoef[2]);
        ushort64 Coef_B1 = convert_ushort64(pCoef[3]);
        ushort64 diff1 = abs_diff(x1, x22);
        diff1 = diff1 >> LUT_SHIFT;
        ushort64 coef1 = shuffle2(Coef_A1, Coef_B1, diff1);
        wtSum += coef1;
        ushort64 tmpMul1 = x1 * coef1;
        Filt += convert_uint64(tmpMul1);

        ushort64 Coef_A2 = convert_ushort64(pCoef[4]);
        ushort64 Coef_B2 = convert_ushort64(pCoef[5]);
        ushort64 diff2 = abs_diff(x2, x22);
        diff2 = diff2 >> LUT_SHIFT;
        ushort64 coef2 = shuffle2(Coef_A2, Coef_B2, diff2);
        wtSum += coef2;
        ushort64 tmpMul2 = x2 * coef2;
        Filt += convert_uint64(tmpMul2);

        ushort64 Coef_A3 = convert_ushort64(pCoef[6]);
        ushort64 Coef_B3 = convert_ushort64(pCoef[7]);
        ushort64 diff3 = abs_diff(x3, x22);
        diff3 = diff3 >> LUT_SHIFT;
        ushort64 coef3 = shuffle2(Coef_A3, Coef_B3, diff3);
        wtSum += coef3;
        ushort64 tmpMul3 = x3 * coef3;
        Filt += convert_uint64(tmpMul3);

        ushort64 Coef_A4 = convert_ushort64(pCoef[8]);
        ushort64 Coef_B4 = convert_ushort64(pCoef[9]);
        ushort64 diff4 = abs_diff(x4, x22);
        diff4 = diff4 >> LUT_SHIFT;
        ushort64 coef4 = shuffle2(Coef_A4, Coef_B4, diff4);
        wtSum += coef4;
        ushort64 tmpMul4 = x4 * coef4;
        Filt += convert_uint64(tmpMul4);

        pCoef += 10;
      }

      uint64 Result = xt_div(Filt, convert_uint64(wtSum));
      vstore64(convert_uchar64(Result), 0, pDst);
      pDst += local_dst_pitch;
    }
  }
#endif
#endif
  event_t evtD = xt_async_work_group_2d_copy(
      Dst + (i * DstPitch) + j, local_dst, outTileW * dstBytes, inTileH,
      DstPitch * dstBytes, local_dst_pitch * dstBytes, 0);

  wait_group_events(1, &evtD);
}
