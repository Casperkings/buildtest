#define SRC_BIT_DEPTH 8
#define DST_BIT_DEPTH 8
#define TILE_W 128
#define OUTPUT_TILE_W TILE_W * 3
#define TILE_H 64
#define W_EXT 0
#define H_EXT 0
#define IMAGE_PAD 0

#define Q15_0_7874 25802
#define Q15_0_9278 30402
#define Q15_0_0936 3068
#define Q15_0_2340 7669

#define Q15_0_5000 16384

__kernel __attribute__((reqd_work_group_size(1, 1, 1)))
__attribute__((reqd_work_dim_size(2))) void
oclKernel(__global uchar *restrict Dst, int DstPitch,
          __global uchar *restrict SrcY, __global uchar *restrict SrcU,
          __global uchar *restrict SrcV, const int SrcPitch, int width,
          int height, int dstWidth, int dstHeight,
          __local uchar *restrict local_srcY,
          __local uchar *restrict local_srcU,
          __local uchar *restrict local_srcV, __local uchar *restrict local_dst,
          __global int *err_codes) {
  int in_i, in_j;
  int out_i, out_j;
  int idx, idy;
  int local_src_pitch, local_dst_pitch;
  int inTileW, inTileH, outTileW, outTileH;

  inTileW = TILE_W;
  inTileH = TILE_H;
  local_src_pitch = inTileW + 2 * W_EXT;

  outTileW = OUTPUT_TILE_W;
  outTileH = TILE_H;
  local_dst_pitch = outTileW;

  in_i = get_global_id(0) * inTileH;
  in_j = get_global_id(1) * inTileW;

  out_i = get_global_id(0) * outTileH;
  out_j = get_global_id(1) * outTileW;

  event_t evtA = xt_async_work_group_2d_copy(
      local_srcY, SrcY + (in_i * SrcPitch) + in_j, local_src_pitch, (inTileH),
      local_src_pitch, SrcPitch, 0);

  event_t evtB = xt_async_work_group_2d_copy(
      local_srcV, SrcV + ((in_i / 2) * (SrcPitch / 2)) + in_j / 2,
      local_src_pitch / 2, (inTileH / 2), local_src_pitch / 2, SrcPitch / 2, 0);

  event_t evtC = xt_async_work_group_2d_copy(
      local_srcU, SrcU + ((in_i / 2) * (SrcPitch / 2)) + in_j / 2,
      local_src_pitch / 2, (inTileH / 2), local_src_pitch / 2, SrcPitch / 2, 0);

  wait_group_events(1, &evtA);
  wait_group_events(1, &evtB);
  wait_group_events(1, &evtC);

#if NATIVE_KERNEL
#include "YUV420toRGB_ivp.c"
#else
#ifdef XT_OCL_EXT
#include "YUV420toRGB_ext.cl"
#else
  const ushort64 maskL = {
      0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7,
      8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15,
      16, 16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22,
      23, 23, 24, 24, 25, 25, 26, 26, 27, 27, 28, 28, 29, 29,
      30, 30, 31, 31};
  const ushort64 maskH = maskL + (ushort64)32;

  // kernel using standard OpenCL C
  for (idx = 0; idx < inTileW; idx += 128) {
    __local uchar *pSrcY = (local_srcY + idx);
    __local uchar *pSrcU = (local_srcU + idx / 2);
    __local uchar *pSrcV = (local_srcV + idx / 2);
    __local uchar *pDst = (__local uchar *)(local_dst + idx * 3);

    for (int y = 0; y < inTileH; y += 2) {
      __local uchar *pSrc = (local_srcY + local_src_pitch * y + idx);
      short64 x00a = convert_short64(vload64(0, pSrc));
      pSrc += 64;
      short64 x00b = convert_short64(vload64(0, pSrc));

      pSrc = (local_srcY + local_src_pitch * (y + 1) + idx);
      short64 x01a = convert_short64(vload64(0, pSrc));
      pSrc += 64;
      short64 x01b = convert_short64(vload64(0, pSrc));

      pSrc = (local_srcU + (local_src_pitch >> 1) * (y >> 1) + (idx >> 1));
      short64 x00u = convert_short64(vload64(0, pSrc));

      pSrc = (local_srcV + (local_src_pitch >> 1) * (y >> 1) + (idx >> 1));
      short64 x00v = convert_short64(vload64(0, pSrc));

      x00u -= (short64)128;
      x00v -= (short64)128;

      int64 A = xt_mul32((short64)Q15_0_7874, x00v) + (int64)(1 << 13);
      int64 C = -xt_mul32((short64)Q15_0_2340, x00v) -
                xt_mul32((short64)Q15_0_0936, x00u) + (int64)(1 << 13);
      int64 D = xt_mul32((short64)Q15_0_9278, x00u) + (int64)(1 << 13);

      short64 R = convert_short64(A >> 14);
      short64 G = convert_short64(C >> 14);
      short64 B = convert_short64(D >> 14);

      short64 Ra = shuffle2(R, (short64)0, maskL);
      short64 Rb = shuffle2(R, (short64)0, maskH);
      short64 Ga = shuffle2(G, (short64)0, maskL);
      short64 Gb = shuffle2(G, (short64)0, maskH);
      short64 Ba = shuffle2(B, (short64)0, maskL);
      short64 Bb = shuffle2(B, (short64)0, maskH);

      short128 R00, R01, G00, G01, B00, B01;
      R00.lo = Ra + x00a;
      R00.hi = Rb + x00b;
      R01.lo = Ra + x01a;
      R01.hi = Rb + x01b;

      G00.lo = Ga + x00a;
      G00.hi = Gb + x00b;
      G01.lo = Ga + x01a;
      G01.hi = Gb + x01b;

      B00.lo = Ba + x00a;
      B00.hi = Bb + x00b;
      B01.lo = Ba + x01a;
      B01.hi = Bb + x01b;

      R00 = min(R00, 255);
      R00 = max(R00, 0);
      R01 = min(R01, 255);
      R01 = max(R01, 0);

      G00 = min(G00, 255);
      G00 = max(G00, 0);
      G01 = min(G01, 255);
      G01 = max(G01, 0);

      B00 = min(B00, 255);
      B00 = max(B00, 0);
      B01 = min(B01, 255);
      B01 = max(B01, 0);

      uchar128 Rout0, Gout0, Bout0;
      uchar128 Rout1, Gout1, Bout1;

      Rout0 = convert_uchar128(R00);
      Rout1 = convert_uchar128(R01);
      Gout0 = convert_uchar128(G00);
      Gout1 = convert_uchar128(G01);
      Bout0 = convert_uchar128(B00);
      Bout1 = convert_uchar128(B01);
  
      // Use the xtensa extensions to perform the interleave
      // instead of using shuffles

      uchar128 RGB[] = {Rout0, Gout0, Bout0};
      xt_interleave_x3(RGB, pDst);
      pDst += local_dst_pitch;

      uchar128 RGB1[] = {Rout1, Gout1, Bout1};
      xt_interleave_x3(RGB1, pDst);
      pDst += local_dst_pitch;
    }
  }
#endif
#endif
  event_t evtD = xt_async_work_group_2d_copy(Dst + (out_i * DstPitch) + out_j,
                                             local_dst, outTileW, outTileH,
                                             DstPitch, local_dst_pitch, 0);

  wait_group_events(1, &evtD);
}
