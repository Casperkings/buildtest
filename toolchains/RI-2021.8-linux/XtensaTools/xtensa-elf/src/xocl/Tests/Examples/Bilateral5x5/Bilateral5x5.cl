#define TILE_W 128
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
  ushort tempDiv[3 * TILE_H * 64]__attribute__((aligned(64)));
#if NATIVE_KERNEL
  #include "Bilateral5x5_ivp.c"
#else
#ifdef XT_OCL_EXT
  #include "Bilateral5x5_ext.cl"
#else
  const ushort32 maskshift1 = {
    1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32};

  ushort32 maskshift2 = maskshift1 + (ushort32)1;
  ushort32 maskshift3 = maskshift2 + (ushort32)1;
  ushort32 maskshift4 = maskshift3 + (ushort32)1;

  // kernel using standard OpenCL C
  for (idx = 0; idx < inTileW; idx += 32) {
    __local uchar *pDst = local_dst + idx;
    for (int y = 0; y < inTileH; ++y) {
      uint32 Filt = (uint32)0;
      ushort32 wtSum = (ushort32)0;

      __local uchar *pSrc2 = local_src + (y + 2) * local_src_pitch + idx;
      ushort32 x20 = convert_ushort32(vload32(0, pSrc2));
      ushort32 xtail = convert_ushort32(vload32(1, pSrc2));
      ushort32 x22 = shuffle2(x20, xtail, maskshift2);

      __local uchar *pSrc = local_src + (y)*local_src_pitch + idx;
      __constant uchar32 *pCoef = (__constant uchar32 *)Coef;

      for (int iy = 0; iy < 5; iy++) {
        ushort32 x00 = convert_ushort32(vload32(0, pSrc));
        ushort32 xtail0 = convert_ushort32(vload32(1, pSrc));
        pSrc += local_src_pitch;
        ushort32 x0 = x00;
        ushort32 x1 = shuffle2(x00, xtail0, maskshift1);
        ushort32 x2 = shuffle2(x00, xtail0, maskshift2);
        ushort32 x3 = shuffle2(x00, xtail0, maskshift3);
        ushort32 x4 = shuffle2(x00, xtail0, maskshift4);

        ushort32 Coef_A0 = convert_ushort32(pCoef[0]);
        ushort32 Coef_B0 = convert_ushort32(pCoef[1]);

        ushort32 diff0 = abs_diff(x0, x22);
        diff0 = diff0 >> LUT_SHIFT;
        ushort32 coef0 = shuffle2(Coef_A0, Coef_B0, diff0);
        wtSum += coef0;
        ushort32 tmpMul0 = x0 * coef0;
        Filt += convert_uint32(tmpMul0);

        ushort32 Coef_A1 = convert_ushort32(pCoef[2]);
        ushort32 Coef_B1 = convert_ushort32(pCoef[3]);
        ushort32 diff1 = abs_diff(x1, x22);
        diff1 = diff1 >> LUT_SHIFT;
        ushort32 coef1 = shuffle2(Coef_A1, Coef_B1, diff1);
        wtSum += coef1;
        ushort32 tmpMul1 = x1 * coef1;
        Filt += convert_uint32(tmpMul1);

        ushort32 Coef_A2 = convert_ushort32(pCoef[4]);
        ushort32 Coef_B2 = convert_ushort32(pCoef[5]);
        ushort32 diff2 = abs_diff(x2, x22);
        diff2 = diff2 >> LUT_SHIFT;
        ushort32 coef2 = shuffle2(Coef_A2, Coef_B2, diff2);
        wtSum += coef2;
        ushort32 tmpMul2 = x2 * coef2;
        Filt += convert_uint32(tmpMul2);

        ushort32 Coef_A3 = convert_ushort32(pCoef[6]);
        ushort32 Coef_B3 = convert_ushort32(pCoef[7]);
        ushort32 diff3 = abs_diff(x3, x22);
        diff3 = diff3 >> LUT_SHIFT;
        ushort32 coef3 = shuffle2(Coef_A3, Coef_B3, diff3);
        wtSum += coef3;
        ushort32 tmpMul3 = x3 * coef3;
        Filt += convert_uint32(tmpMul3);

        ushort32 Coef_A4 = convert_ushort32(pCoef[8]);
        ushort32 Coef_B4 = convert_ushort32(pCoef[9]);
        ushort32 diff4 = abs_diff(x4, x22);
        diff4 = diff4 >> LUT_SHIFT;
        ushort32 coef4 = shuffle2(Coef_A4, Coef_B4, diff4);
        wtSum += coef4;
        ushort32 tmpMul4 = x4 * coef4;
        Filt += convert_uint32(tmpMul4);

        pCoef += 10;
      }

      uint32 Result = xt_div(Filt, convert_uint32(wtSum));
      vstore32(convert_uchar32(Result), 0, pDst);
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
