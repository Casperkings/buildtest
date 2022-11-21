#define IMAGE_D 128
#define SRC_BIT_DEPTH (8 * IMAGE_D)
#define DST_BIT_DEPTH (8 * IMAGE_D)
#define TILE_W 10
#define TILE_H 5
#define W_EXT 2
#define H_EXT 2
#define IMAGE_PAD 2
#define KTIME 0

__kernel
__attribute__((reqd_work_group_size(1, 1, 1)))
__attribute__((reqd_work_dim_size(2)))
void
oclKernel(__global char *restrict Src, __global char *restrict Coef,
          int SrcPitch, __global char *restrict Dst, const int DstPitch,
          int width, int height, int dstWidth, int dstHeight, int depth,
          int shift, __local char *restrict local_src,
          __local char *restrict local_dst,
          __local char *restrict local_coef,
          __global int *restrict err_codes) {
  int i, j;
  int idx, idy;
  int local_src_pitch, local_dst_pitch;
  int pad = IMAGE_PAD;
  int inTileW, inTileH, outTileW, outTileH;
  int dstBytes = DST_BIT_DEPTH >> 3;
  int srcBytes = SRC_BIT_DEPTH >> 3;
  inTileW = TILE_W;
  inTileH = TILE_H;
  outTileW = TILE_W;
  outTileH = TILE_H;
  local_src_pitch = (inTileW + pad + pad) * srcBytes;
  local_dst_pitch = outTileW * dstBytes;
  int err = 0;

  i = get_global_id(0) * inTileH;
  j = get_global_id(1) * inTileW;

  const ushort64 maskinterleavea = {
    0, 64, 1, 65, 2, 66, 3, 67, 4, 68, 5, 69, 6, 70, 7, 71, 8,
    72, 9, 73, 10, 74, 11, 75, 12, 76, 13, 77, 14, 78, 15, 79, 16, 80, 17, 81,
    18, 82, 19, 83, 20, 84, 21, 85, 22, 86, 23, 87, 24, 88, 25, 89, 26, 90,
    27, 91, 28, 92, 29, 93, 30, 94, 31, 95};

  const ushort64 mask64 = {
    0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20,
    22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 44,
    46, 48, 50, 52, 54, 56, 58, 60, 62, 64, 66, 68,
    70, 72, 74, 76, 78, 80, 82, 84, 86, 88, 90, 92,
    94, 96, 98, 100, 102, 104, 106, 108, 110, 112,
    114, 116, 118, 120, 122, 124, 126};

  event_t evtA = xt_async_work_group_2d_copy(
      local_src, Src + (((i * SrcPitch) + j) * srcBytes), local_src_pitch,
      (inTileH + pad + pad), local_src_pitch, SrcPitch * srcBytes, 0);

  event_t evtB = xt_async_work_group_2d_copy(
      local_coef, Coef, 5 * 5 * depth, depth, 5 * 5 * depth, 5 * 5 * depth, 0);
  wait_group_events(1, &evtA);
  wait_group_events(1, &evtB);

#if NATIVE_KERNEL
  #include "Convolve3D_5x5_ivp.c"
#elif XT_OCL_EXT
  #include "Convolve3D_5x5_ext.cl"
#else
  ushort64 maskinterleaveb = maskinterleavea + (ushort64)16;

  __local char *pDst = local_dst;
  __local char *pCoef = local_coef;
  // kernel using standard OpenCL C
  for (int h = 0; h < outTileH; h++) {
    for (int w = 0; w < outTileW; w++) {
      for (int d = 0; d < depth; d += 64) {
        __local char *pvecCoeff = (__local char *)pCoef + d;

        int64 Acc = (int64)0;
        for (int id = 0; id < depth; id++) {
          for (int y = 0; y < 5; y++) {
            // 0
            __local char *pInCurr =
                &local_src[id + (h + y) * local_src_pitch + w * depth];
            short feat = *pInCurr;
            pInCurr += depth;
            short64 Coefvec = convert_short64(vload64(0, pvecCoeff));
            pvecCoeff += depth;
#ifdef XT_OCL_EXT
            Acc = xt_mad32(Coefvec, (short64)feat, Acc);
#else
            Acc += convert_int64(Coefvec * (short64)feat);
#endif

            // 1
            feat = *pInCurr;
            pInCurr += depth;
            Coefvec = convert_short64(vload64(0, pvecCoeff));
            pvecCoeff += depth;
#ifdef XT_OCL_EXT
            Acc = xt_mad32(Coefvec, (short64)feat, Acc);
#else
            Acc += convert_int64(Coefvec * (short64)feat);
#endif

            // 2
            feat = *pInCurr;
            pInCurr += depth;
            Coefvec = convert_short64(vload64(0, pvecCoeff));
            pvecCoeff += depth;
#ifdef XT_OCL_EXT
            Acc = xt_mad32(Coefvec, (short64)feat, Acc);
#else
            Acc += convert_int64(Coefvec * (short64)feat);
#endif

            // 3
            feat = *pInCurr;
            pInCurr += depth;
            Coefvec = convert_short64(vload64(0, pvecCoeff));
            pvecCoeff += depth;
#ifdef XT_OCL_EXT
            Acc = xt_mad32(Coefvec, (short64)feat, Acc);
#else
            Acc += convert_int64(Coefvec * (short64)feat);
#endif

            // 4
            feat = *pInCurr;
            Coefvec = convert_short64(vload64(0, pvecCoeff));
            pvecCoeff += depth;
#ifdef XT_OCL_EXT
            Acc = xt_mad32(Coefvec, (short64)feat, Acc);
#else
            Acc += convert_int64(Coefvec * (short64)feat);
#endif
          }
        }
#ifdef XT_OCL_EXT
        char64 res = convert_char64(xt_convert_short64_sat_rnd(Acc, shift));
#else
        Acc = Acc + (int64)(1 << (shift - 1));
        Acc = Acc >> shift;
        char64 res = convert_char64(Acc);
#endif
        vstore64(res, 0, pDst);
        pDst += 64;
      }
    }
  }
#endif
  event_t evtD = xt_async_work_group_2d_copy(
      Dst + (((i * DstPitch) + j) * dstBytes), local_dst, outTileW * dstBytes,
      outTileH, DstPitch * dstBytes, local_dst_pitch, 0);

  wait_group_events(1, &evtD);
}
