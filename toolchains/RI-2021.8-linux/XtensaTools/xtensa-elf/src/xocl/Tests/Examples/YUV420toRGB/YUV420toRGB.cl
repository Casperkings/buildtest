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

__constant uchar kMask0[] __attribute__((section(".dram0.data"))) = 
                      {0,  1,  64, 2,  3,  65, 4,  5,  66, 6,  7,  67, 8,
                       9,  68, 10, 11, 69, 12, 13, 70, 14, 15, 71, 16, 17,
                       72, 18, 19, 73, 20, 21, 74, 22, 23, 75, 24, 25, 76,
                       26, 27, 77, 28, 29, 78, 30, 31, 79, 32, 33, 80, 34,
                       35, 81, 36, 37, 82, 38, 39, 83, 40, 41, 84, 42};

__constant uchar kMask1[] __attribute__((section(".dram0.data"))) =
                      {0,   85, 1,  2,   86, 3,  4,   87,  5,  6,   88,  7,  8,
                       89,  9,  10, 90,  11, 12, 91,  13,  14, 92,  15,  16, 93,
                       17,  18, 94, 19,  20, 95, 21,  22,  96, 23,  24,  97, 25,
                       26,  98, 27, 28,  99, 29, 30,  100, 31, 32,  101, 33, 34,
                       102, 35, 36, 103, 37, 38, 104, 39,  40, 105, 41,  42};

__constant uchar kMask2[] __attribute__((section(".dram0.data"))) =
                      {106, 0,   1,   107, 2,   3,   108, 4,   5,   109, 6,
                       7,   110, 8,   9,   111, 10,  11,  112, 12,  13,  113,
                       14,  15,  114, 16,  17,  115, 18,  19,  116, 20,  21,
                       117, 22,  23,  118, 24,  25,  119, 26,  27,  120, 28,
                       29,  121, 30,  31,  122, 32,  33,  123, 34,  35,  124,
                       36,  37,  125, 38,  39,  126, 40,  41,  127};

__constant uchar kInterleaveRGb[] __attribute__((section(".dram0.data"))) = {
    85,  22, 86,  23, 87,  24, 88,  25, 89,  26, 90,  27, 91,  28, 92,  29,
    93,  30, 94,  31, 95,  32, 96,  33, 97,  34, 98,  35, 99,  36, 100, 37,
    101, 38, 102, 39, 103, 40, 104, 41, 105, 42, 106, 43, 107, 44, 108, 45,
    109, 46, 110, 47, 111, 48, 112, 49, 113, 50, 114, 51, 115, 52, 116, 53};

__constant uchar kInterleaveRGc[] __attribute__((section(".dram0.data"))) = {
    43, 107, 44, 108, 45, 109, 46, 110, 47, 111, 48, 112, 49, 113, 50, 114,
    51, 115, 52, 116, 53, 117, 54, 118, 55, 119, 56, 120, 57, 121, 58, 122,
    59, 123, 60, 124, 61, 125, 62, 126, 63, 127, 0,  0,   0,  0,   0,  0,
    0,  0,   0,  0,   0,  0,   0,  0,   0,  0,   0,  0,   0,  0,   0,  0};

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
  const ushort32 maskL = {0,  0,  1,  1,  2,  2,  3,  3,  4,  4,  5,
                          5,  6,  6,  7,  7,  8,  8,  9,  9,  10, 10,
                          11, 11, 12, 12, 13, 13, 14, 14, 15, 15};
  const ushort32 maskH = maskL + (ushort32)16;

  const uchar64 InterleaveRGa = {
      0,  64, 1,  65, 2,  66, 3,  67, 4,  68, 5,  69, 6,  70, 7,  71,
      8,  72, 9,  73, 10, 74, 11, 75, 12, 76, 13, 77, 14, 78, 15, 79,
      16, 80, 17, 81, 18, 82, 19, 83, 20, 84, 21, 85, 22, 86, 23, 87,
      24, 88, 25, 89, 26, 90, 27, 91, 28, 92, 29, 93, 30, 94, 31, 95};
  const uchar64 InterleaveRGb = {
      85,  22, 86,  23, 87,  24, 88,  25, 89,  26, 90,  27, 91,  28, 92,  29,
      93,  30, 94,  31, 95,  32, 96,  33, 97,  34, 98,  35, 99,  36, 100, 37,
      101, 38, 102, 39, 103, 40, 104, 41, 105, 42, 106, 43, 107, 44, 108, 45,
      109, 46, 110, 47, 111, 48, 112, 49, 113, 50, 114, 51, 115, 52, 116, 53};

  const uchar64 InterleaveRGc = {
      43, 107, 44, 108, 45, 109, 46, 110, 47, 111, 48, 112, 49, 113, 50, 114,
      51, 115, 52, 116, 53, 117, 54, 118, 55, 119, 56, 120, 57, 121, 58, 122,
      59, 123, 60, 124, 61, 125, 62, 126, 63, 127, 0,  0,   0,  0,   0,  0,
      0,  0,   0,  0,   0,  0,   0,  0,   0,  0,   0,  0,   0,  0,   0,  0};

  const uchar64 Mask0 = {0,  1,  64, 2,  3,  65, 4,  5,  66, 6,  7,  67, 8,
                         9,  68, 10, 11, 69, 12, 13, 70, 14, 15, 71, 16, 17,
                         72, 18, 19, 73, 20, 21, 74, 22, 23, 75, 24, 25, 76,
                         26, 27, 77, 28, 29, 78, 30, 31, 79, 32, 33, 80, 34,
                         35, 81, 36, 37, 82, 38, 39, 83, 40, 41, 84, 42};

  const uchar64 Mask1 = {
      0,  85,  1,  2,  86,  3,  4,  87,  5,  6,  88,  7,  8,  89,  9,   10,
      90, 11,  12, 91, 13,  14, 92, 15,  16, 93, 17,  18, 94, 19,  20,  95,
      21, 22,  96, 23, 24,  97, 25, 26,  98, 27, 28,  99, 29, 30,  100, 31,
      32, 101, 33, 34, 102, 35, 36, 103, 37, 38, 104, 39, 40, 105, 41,  42};

  const uchar64 Mask2 = {106, 0,   1,   107, 2,   3,   108, 4,   5,   109, 6,
                         7,   110, 8,   9,   111, 10,  11,  112, 12,  13,  113,
                         14,  15,  114, 16,  17,  115, 18,  19,  116, 20,  21,
                         117, 22,  23,  118, 24,  25,  119, 26,  27,  120, 28,
                         29,  121, 30,  31,  122, 32,  33,  123, 34,  35,  124,
                         36,  37,  125, 38,  39,  126, 40,  41,  127};

  // kernel using standard OpenCL C
  for (idx = 0; idx < inTileW; idx += 64) {
    __local uchar *pSrcY = (local_srcY + idx);
    __local uchar *pSrcU = (local_srcU + idx / 2);
    __local uchar *pSrcV = (local_srcV + idx / 2);
    __local char *pDst = (__local char *)(local_dst + idx * 3);

    for (int y = 0; y < inTileH; y += 2) {
      __local uchar *pSrc = (local_srcY + local_src_pitch * y + idx);
      short32 x00a = convert_short32(vload32(0, pSrc));
      pSrc += 32;
      short32 x00b = convert_short32(vload32(0, pSrc));

      pSrc = (local_srcY + local_src_pitch * (y + 1) + idx);
      short32 x01a = convert_short32(vload32(0, pSrc));
      pSrc += 32;
      short32 x01b = convert_short32(vload32(0, pSrc));

      pSrc = (local_srcU + (local_src_pitch >> 1) * (y >> 1) + (idx >> 1));
      short32 x00u = convert_short32(vload32(0, pSrc));

      pSrc = (local_srcV + (local_src_pitch >> 1) * (y >> 1) + (idx >> 1));
      short32 x00v = convert_short32(vload32(0, pSrc));

      x00u -= (short32)128;
      x00v -= (short32)128;

      int32 A = xt_mul32((short32)Q15_0_7874, x00v) + (int32)(1 << 13);
      int32 C = -xt_mul32((short32)Q15_0_2340, x00v) -
                xt_mul32((short32)Q15_0_0936, x00u) + (int32)(1 << 13);
      int32 D = xt_mul32((short32)Q15_0_9278, x00u) + (int32)(1 << 13);

      short32 R = convert_short32(A >> 14);
      short32 G = convert_short32(C >> 14);
      short32 B = convert_short32(D >> 14);

      short32 Ra = shuffle2(R, (short32)0, maskL);
      short32 Rb = shuffle2(R, (short32)0, maskH);
      short32 Ga = shuffle2(G, (short32)0, maskL);
      short32 Gb = shuffle2(G, (short32)0, maskH);
      short32 Ba = shuffle2(B, (short32)0, maskL);
      short32 Bb = shuffle2(B, (short32)0, maskH);

      short64 R00, R01, G00, G01, B00, B01;
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

      char64 Rout0, Gout0, Bout0;
      char64 Rout1, Gout1, Bout1;

      Rout0 = convert_char64(R00);
      Rout1 = convert_char64(R01);
      Gout0 = convert_char64(G00);
      Gout1 = convert_char64(G01);
      Bout0 = convert_char64(B00);
      Bout1 = convert_char64(B01);

      char64 Out00a = shuffle2(Rout0, Gout0, InterleaveRGa);
      Out00a = shuffle2(Out00a, Bout0, Mask0);
      char64 Out00b = shuffle2(Rout0, Gout0, InterleaveRGb);
      Out00b = shuffle2(Out00b, Bout0, Mask1);
      char64 Out00c = shuffle2(Rout0, Gout0, InterleaveRGc);
      Out00c = shuffle2(Out00c, Bout0, Mask2);

      vstore64(Out00a, 0, pDst);
      vstore64(Out00b, 0, pDst + 64);
      vstore64(Out00c, 0, pDst + 128);
      pDst += local_dst_pitch;

      char64 Out01a = shuffle2(Rout1, Gout1, InterleaveRGa);
      Out01a = shuffle2(Out01a, Bout1, Mask0);
      char64 Out01b = shuffle2(Rout1, Gout1, InterleaveRGb);
      Out01b = shuffle2(Out01b, Bout1, Mask1);
      char64 Out01c = shuffle2(Rout1, Gout1, InterleaveRGc);
      Out01c = shuffle2(Out01c, Bout1, Mask2);

      vstore64(Out01a, 0, pDst);
      vstore64(Out01b, 0, pDst + 64);
      vstore64(Out01c, 0, pDst + 128);
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
