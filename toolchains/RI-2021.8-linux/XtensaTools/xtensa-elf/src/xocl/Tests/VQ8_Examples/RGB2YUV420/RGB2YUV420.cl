#define SRC_BIT_DEPTH 8
#define DST_BIT_DEPTH 8
#define TILE_W 128
#define INPUT_TILE_W TILE_W * 3
#define TILE_H 64
#define W_EXT 0
#define H_EXT 0
#define IMAGE_PAD 0

#define Q15_0_2126 6966
#define Q15_0_7152 23436
#define Q15_0_0722 2366

#define Q15_0_5389 17659
#define Q15_0_6350 20808

__constant uchar kmaskR0[] __attribute__((section(".dram0.data"))) = {
    0,  3,  6,   9,   12,  15,  18,  21,  24,  27,  30,  33, 36, 39, 42, 45,
    48, 51, 54,  57,  60,  63,  66,  69,  72,  75,  78,  81, 84, 87, 90, 93,
    96, 99, 102, 105, 108, 111, 114, 117, 120, 123, 126, 0,  0,  0,  0,  0,
    0,  0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  0,  0,  0,  0};

__constant uchar kmaskR1[] __attribute__((section(".dram0.data"))) = {
    0,  1,  2,  3,  4,  5,  6,  7,   8,   9,   10,  11,  12,  13,  14,  15,
    16, 17, 18, 19, 20, 21, 22, 23,  24,  25,  26,  27,  28,  29,  30,  31,
    32, 33, 34, 35, 36, 37, 38, 39,  40,  41,  42,  65,  68,  71,  74,  77,
    80, 83, 86, 89, 92, 95, 98, 101, 104, 107, 110, 113, 116, 119, 122, 125};

__constant uchar kmaskG1[] __attribute__((section(".dram0.data"))) = {
    0,  1,  2,  3,  4,  5,  6,  7,   8,   9,   10,  11,  12,  13,  14,  15,
    16, 17, 18, 19, 20, 21, 22, 23,  24,  25,  26,  27,  28,  29,  30,  31,
    32, 33, 34, 35, 36, 37, 38, 39,  40,  41,  42,  66,  69,  72,  75,  78,
    81, 84, 87, 90, 93, 96, 99, 102, 105, 108, 111, 114, 117, 120, 123, 126};

__constant uchar kmaskB1[] __attribute__((section(".dram0.data"))) = {
    0,  1,  2,  3,  4,  5,  6,   7,   8,   9,   10,  11,  12,  13,  14,  15,
    16, 17, 18, 19, 20, 21, 22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
    32, 33, 34, 35, 36, 37, 38,  39,  40,  41,  64,  67,  70,  73,  76,  79,
    82, 85, 88, 91, 94, 97, 100, 103, 106, 109, 112, 115, 118, 121, 124, 127};

__kernel __attribute__((reqd_work_group_size(1, 1, 1)))
__attribute__((reqd_work_dim_size(2))) void
oclKernel(__global uchar *restrict Src, int SrcPitch,
          __global uchar *restrict DstY, __global uchar *restrict DstU,
          __global uchar *restrict DstV, const int DstPitch, int width,
          int height, int dstWidth, int dstHeight,
          __local uchar *restrict local_src, __local uchar *restrict local_dstY,
          __local uchar *restrict local_dstU,
          __local uchar *restrict local_dstV, __global int *err_codes) {
  int in_i, in_j;
  int out_i, out_j;
  int idx, idy;
  int local_src_pitch, local_dst_pitch;
  int inTileW, inTileH, outTileW, outTileH;

  inTileW = INPUT_TILE_W;
  inTileH = TILE_H;
  local_src_pitch = inTileW + 2 * W_EXT;

  outTileW = TILE_W;
  outTileH = TILE_H;
  local_dst_pitch = outTileW;

  in_i = get_global_id(0) * inTileH;
  in_j = get_global_id(1) * inTileW;

  out_i = get_global_id(0) * outTileH;
  out_j = get_global_id(1) * outTileW;

  event_t evtA = xt_async_work_group_2d_copy(
      local_src, Src + (in_i * SrcPitch) + in_j, local_src_pitch, (inTileH),
      local_src_pitch, SrcPitch, 0);
  wait_group_events(1, &evtA);

#if NATIVE_KERNEL
#include "RGB2YUV420_ivp.c"
#else
#ifdef XT_OCL_EXT
#include "RGB2YUV420_ext.cl"
#else

  const ushort64 InterleaveA = {
    0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30,
    32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62,
    64, 66, 68, 70, 72, 74, 76, 78, 80, 82, 84, 86, 88, 90, 92, 94, 96, 98, 100,
    102, 104, 106, 108, 110, 112, 114, 116, 118, 120, 122, 124, 126};

  const ushort64 InterleaveB = InterleaveA + (ushort64)1;

  const ushort64 maskR0 = {0,   3,   6,   9,   12,  15,  18,  21,  24,  27, 30,
                           33,  36,  39,  42,  45,  48,  51,  54,  57,  60, 63,
                           66,  69,  72,  75,  78,  81,  84,  87,  90,  93, 96, 
                           99,  102, 105, 108, 111, 114, 117, 120, 123, 126, 0,
                           0  , 0,   0,   0,   0,   0,   0,   0,   0,   0,  0,
                           0,   0,   0,   0,   0,   0,   0,   0,   0};
  const ushort64 maskR1 = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
                           11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
                           22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 
                           33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 65, 
                           68, 71, 74, 77, 80, 83, 86, 89, 92, 95, 98,
                           101, 104, 107, 110, 113, 116, 119, 122, 125};

  const ushort64 maskG0 = {1,   4,   7,   10,  13,  16,  19,  22,  25,  28, 31,
                           34,  37,  40,  43,  46,  49,  52,  55,  58,  61, 64,
                           67,  70,  73,  76,  79,  82,  85,  88,  91,  94, 97,
                           100, 103, 106, 109, 112, 115, 118, 121, 124, 127,0,
                           0  , 0,   0,   0,   0,   0,   0,   0,   0,   0,  0,
                           0,   0,   0,   0,   0,   0,   0,   0,   0};
  const ushort64 maskG1 = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
                           11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
                           22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 
                           33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 66, 
                           69, 72, 75, 78, 81, 84, 87, 90, 93, 96, 99,
                           102, 105, 108, 111, 114, 117, 120, 123, 126};

  const ushort64 maskB0 = {2,   5,   8,   11,  14,  17,  20,  23,  26,  29, 32,
                           35,  38,  41,  44,  47,  50,  53,  56,  59,  62, 65,
                           68,  71,  74,  77,  80,  83,  86,  89,  92,  95, 98,
                           101, 104, 107, 110, 113, 116, 119, 122, 125, 0,  0,
                           0  , 0,   0,   0,   0,   0,   0,   0,   0,   0,  0,
                           0,   0,   0,   0,   0,   0,   0,   0,   0};
  const ushort64 maskB1 = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
                           11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
                           22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 
                           33, 34, 35, 36, 37, 38, 39, 40, 41, 64, 67, 
                           70, 73, 76, 79, 82, 85, 88, 91, 94, 97, 100,
                           103, 106, 109, 112, 115, 118, 121, 124, 127};

  // kernel using standard OpenCL C
  for (idx = 0; idx < outTileW; idx += 128) {

    __local uchar *pDstY = (local_dstY + idx);
    __local uchar *pDstU = (local_dstU + idx / 2);
    __local uchar *pDstV = (local_dstV + idx / 2);

    for (int y = 0; y < outTileH; y += 2) {

      __local uchar *pSrc = (local_src + local_src_pitch * y + idx * 3);
      ushort64 x00 = convert_ushort64(vload64(0, pSrc));
      pSrc += 64;
      ushort64 x01 = convert_ushort64(vload64(0, pSrc));
      pSrc += 64;
      ushort64 x02 = convert_ushort64(vload64(0, pSrc));

      ushort64 xRb = shuffle2(x00, x01, maskR0);
      ushort64 xGb = shuffle2(x00, x01, maskG0);
      ushort64 xBb = shuffle2(x00, x01, maskB0);

      ushort64 xR0 = shuffle2(xRb, x02, maskR1);
      ushort64 xG0 = shuffle2(xGb, x02, maskG1);
      ushort64 xB0 = shuffle2(xBb, x02, maskB1);

      /* Y part*/
      uint64 ColorY = (uint64)(1 << 14);
      //ColorY += xt_mul32(xR0, (ushort64)Q15_0_2126);
      //ColorY += xt_mul32(xG0, (ushort64)Q15_0_7152);
      //ColorY += xt_mul32(xB0, (ushort64)Q15_0_0722);
      ColorY = xt_mad32(xR0, (ushort64)Q15_0_2126, ColorY);
      ColorY = xt_mad32(xG0, (ushort64)Q15_0_7152, ColorY);
      ColorY = xt_mad32(xB0, (ushort64)Q15_0_0722, ColorY);
      ColorY = ColorY >> 15;
      ushort64 xY0 = convert_ushort64(ColorY);
      uchar64 Y0 = convert_uchar64(xY0);
      vstore64(Y0, 0, pDstY);
      pDstY += 64;

      pSrc = (local_src + local_src_pitch * y + (idx + 64) * 3);
      x00 = convert_ushort64(vload64(0, pSrc));
      pSrc += 64;
      x01 = convert_ushort64(vload64(0, pSrc));
      pSrc += 64;
      x02 = convert_ushort64(vload64(0, pSrc));

      xRb = shuffle2(x00, x01, maskR0);
      xGb = shuffle2(x00, x01, maskG0);
      xBb = shuffle2(x00, x01, maskB0);

      ushort64 xR1 = shuffle2(xRb, x02, maskR1);
      ushort64 xG1 = shuffle2(xGb, x02, maskG1);
      ushort64 xB1 = shuffle2(xBb, x02, maskB1);

      /* Y part*/
      ColorY = (uint64)(1 << 14);
      //ColorY += xt_mul32(xR1, (ushort64)Q15_0_2126);
      //ColorY += xt_mul32(xG1, (ushort64)Q15_0_7152);
      //ColorY += xt_mul32(xB1, (ushort64)Q15_0_0722);
      ColorY = xt_mad32(xR1, (ushort64)Q15_0_2126, ColorY);
      ColorY = xt_mad32(xG1, (ushort64)Q15_0_7152, ColorY);
      ColorY = xt_mad32(xB1, (ushort64)Q15_0_0722, ColorY);
      ColorY = ColorY >> 15;
      ushort64 xY1 = convert_ushort64(ColorY);
      uchar64 Y1 = convert_uchar64(xY1);
      vstore64(Y1, 0, pDstY);
      pDstY += local_dst_pitch - 64;

      pSrc = (local_src + local_src_pitch * (y + 1) + idx * 3);
      x00 = convert_ushort64(vload64(0, pSrc));
      pSrc += 64;
      x01 = convert_ushort64(vload64(0, pSrc));
      pSrc += 64;
      x02 = convert_ushort64(vload64(0, pSrc));

      xRb = shuffle2(x00, x01, maskR0);
      xGb = shuffle2(x00, x01, maskG0);
      xBb = shuffle2(x00, x01, maskB0);

      ushort64 xR2 = shuffle2(xRb, x02, maskR1);
      ushort64 xG2 = shuffle2(xGb, x02, maskG1);
      ushort64 xB2 = shuffle2(xBb, x02, maskB1);

      /* Y part*/
      ColorY = (uint64)(1 << 14);
      //ColorY += xt_mul32(xR2, (ushort64)Q15_0_2126);
      //ColorY += xt_mul32(xG2, (ushort64)Q15_0_7152);
      //ColorY += xt_mul32(xB2, (ushort64)Q15_0_0722);
      ColorY = xt_mad32(xR2, (ushort64)Q15_0_2126, ColorY);
      ColorY = xt_mad32(xG2, (ushort64)Q15_0_7152, ColorY);
      ColorY = xt_mad32(xB2, (ushort64)Q15_0_0722, ColorY);
      ColorY = ColorY >> 15;
      ushort64 xY2 = convert_ushort64(ColorY);
      uchar64 Y2 = convert_uchar64(xY2);
      vstore64(Y2, 0, pDstY);
      pDstY += 64;

      pSrc = (local_src + local_src_pitch * (y + 1) + (idx + 64) * 3);
      x00 = convert_ushort64(vload64(0, pSrc));
      pSrc += 64;
      x01 = convert_ushort64(vload64(0, pSrc));
      pSrc += 64;
      x02 = convert_ushort64(vload64(0, pSrc));

      xRb = shuffle2(x00, x01, maskR0);
      xGb = shuffle2(x00, x01, maskG0);
      xBb = shuffle2(x00, x01, maskB0);

      ushort64 xR3 = shuffle2(xRb, x02, maskR1);
      ushort64 xG3 = shuffle2(xGb, x02, maskG1);
      ushort64 xB3 = shuffle2(xBb, x02, maskB1);

      /* Y part*/
      ColorY = (uint64)(1 << 14);
      //ColorY += xt_mul32(xR3, (ushort64)Q15_0_2126);
      //ColorY += xt_mul32(xG3, (ushort64)Q15_0_7152);
      //ColorY += xt_mul32(xB3, (ushort64)Q15_0_0722);
      ColorY = xt_mad32(xR3, (ushort64)Q15_0_2126, ColorY);
      ColorY = xt_mad32(xG3, (ushort64)Q15_0_7152, ColorY);
      ColorY = xt_mad32(xB3, (ushort64)Q15_0_0722, ColorY);
      ColorY = ColorY >> 15;
      ushort64 xY3 = convert_ushort64(ColorY);
      uchar64 Y3 = convert_uchar64(xY3);
      vstore64(Y3, 0, pDstY);
      pDstY += local_dst_pitch - 64;

      /* U part*/
      ushort64 xxB0 = shuffle2(xB0, xB1, InterleaveA);
      ushort64 xxB1 = shuffle2(xB0, xB1, InterleaveB);
      ushort64 xxB2 = shuffle2(xB2, xB3, InterleaveA);
      ushort64 xxB3 = shuffle2(xB2, xB3, InterleaveB);
      ushort64 xB = xxB0 + xxB1 + xxB2 + xxB3;
      ushort64 xxY0 = shuffle2(xY0, xY1, InterleaveA);
      ushort64 xxY1 = shuffle2(xY0, xY1, InterleaveB);
      ushort64 xxY2 = shuffle2(xY2, xY3, InterleaveA);
      ushort64 xxY3 = shuffle2(xY2, xY3, InterleaveB);
      ushort64 xY = xxY0 + xxY1 + xxY2 + xxY3;
      uint64 ColorU = (uint64)(65535);
      ColorU += xt_mul32(xB, (ushort64)Q15_0_5389);
      ColorU -= xt_mul32(xY, (ushort64)Q15_0_5389);
      ColorU = ColorU >> 17;
      ushort64 xU = convert_ushort64(ColorU);
      xU += (ushort64)128;
      uchar64 U = convert_uchar64(xU);

      /* V part*/
      ushort64 xxR0 = shuffle2(xR0, xR1, InterleaveA);
      ushort64 xxR1 = shuffle2(xR0, xR1, InterleaveB);
      ushort64 xxR2 = shuffle2(xR2, xR3, InterleaveA);
      ushort64 xxR3 = shuffle2(xR2, xR3, InterleaveB);
      ushort64 xR = xxR0 + xxR1 + xxR2 + xxR3;
      uint64 ColorV = (uint64)(65535);
      ColorV += xt_mul32(xR, (ushort64)Q15_0_6350);
      ColorV -= xt_mul32(xY, (ushort64)Q15_0_6350);
      ColorV = ColorV >> 17;
      ushort64 xV = convert_ushort64(ColorV);
      xV += (ushort64)128;
      uchar64 V = convert_uchar64(xV);

      vstore64(U, 0, pDstU);
      pDstU += local_dst_pitch / 2;
      vstore64(V, 0, pDstV);
      pDstV += local_dst_pitch / 2;
    }
  }
#endif
#endif

  event_t evtDY = xt_async_work_group_2d_copy(DstY + (out_i * DstPitch) + out_j,
                                              local_dstY, outTileW, outTileH,
                                              DstPitch, local_dst_pitch, 0);

  event_t evtDU = xt_async_work_group_2d_copy(
      DstU + ((out_i / 2) * (DstPitch / 2)) + (out_j / 2), local_dstU,
      outTileW / 2, outTileH / 2, DstPitch / 2, local_dst_pitch / 2, 0);

  event_t evtDV = xt_async_work_group_2d_copy(
      DstV + ((out_i / 2) * (DstPitch / 2)) + (out_j / 2), local_dstV,
      outTileW / 2, outTileH / 2, DstPitch / 2, local_dst_pitch / 2, 0);

  wait_group_events(1, &evtDY);
  wait_group_events(1, &evtDU);
  wait_group_events(1, &evtDV);
}
