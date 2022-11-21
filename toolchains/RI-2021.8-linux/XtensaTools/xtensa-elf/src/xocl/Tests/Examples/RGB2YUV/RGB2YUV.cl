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
#include "RGB2YUV_ivp.c"
#else
#ifdef XT_OCL_EXT
#include "RGB2YUV_ext.cl"
#else

  const ushort32 maskR0 = {0,  3,  6,  9,  12, 15, 18, 21, 24, 27, 30,
                           33, 36, 39, 42, 45, 48, 51, 54, 57, 60, 63,
                           0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
  const ushort32 maskR1 = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
                           11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
                           34, 37, 40, 43, 46, 49, 52, 55, 58, 61};

  const ushort32 maskG0 = {1,  4,  7,  10, 13, 16, 19, 22, 25, 28, 31,
                           34, 37, 40, 43, 46, 49, 52, 55, 58, 61, 0,
                           0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
  const ushort32 maskG1 = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
                           11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 32,
                           35, 38, 41, 44, 47, 50, 53, 56, 59, 62};

  const ushort32 maskB0 = {2,  5,  8,  11, 14, 17, 20, 23, 26, 29, 32,
                           35, 38, 41, 44, 47, 50, 53, 56, 59, 62, 0,
                           0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
  const ushort32 maskB1 = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
                           11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 33,
                           36, 39, 42, 45, 48, 51, 54, 57, 60, 63};

  // kernel using standard OpenCL C
  for (idx = 0; idx < outTileW; idx += 32) {
    __local uchar *pSrc = (local_src + idx * 3);
    __local uchar *pDstY = (local_dstY + idx);
    __local uchar *pDstU = (local_dstU + idx);
    __local uchar *pDstV = (local_dstV + idx);

    for (int y = 0; y < outTileH; ++y) {
      ushort32 x00 = convert_ushort32(vload32(0, pSrc));
      pSrc += 32;
      ushort32 x01 = convert_ushort32(vload32(0, pSrc));
      pSrc += 32;
      ushort32 x02 = convert_ushort32(vload32(0, pSrc));
      pSrc += local_src_pitch - 32 - 32;

      ushort32 xRb = shuffle2(x00, x01, maskR0);
      ushort32 xGb = shuffle2(x00, x01, maskG0);
      ushort32 xBb = shuffle2(x00, x01, maskB0);

      ushort32 xR = shuffle2(xRb, x02, maskR1);
      ushort32 xG = shuffle2(xGb, x02, maskG1);
      ushort32 xB = shuffle2(xBb, x02, maskB1);

      /* Y part*/
      uint32 ColorY = (uint32)(1 << 14);
      ColorY += xt_mul32(xR, (ushort32)Q15_0_2126);
      ColorY += xt_mul32(xG, (ushort32)Q15_0_7152);
      ColorY += xt_mul32(xB, (ushort32)Q15_0_0722);
      ColorY = ColorY >> 15;
      ushort32 xY = convert_ushort32(ColorY);
      uchar32 Y = convert_uchar32(xY);

      /* U part*/
      uint32 ColorU = (uint32)(1 << 14);
      ColorU += xt_mul32(xB, (ushort32)Q15_0_5389);
      ColorU -= xt_mul32(xY, (ushort32)Q15_0_5389);
      ColorU = ColorU >> 15;
      ushort32 xU = convert_ushort32(ColorU);
      xU += (ushort32)128;
      uchar32 U = convert_uchar32(xU);

      /* V part*/
      uint32 ColorV = (uint32)(1 << 14);
      ColorV += xt_mul32(xR, (ushort32)Q15_0_6350);
      ColorV -= xt_mul32(xY, (ushort32)Q15_0_6350);
      ColorV = ColorV >> 15;
      ushort32 xV = convert_ushort32(ColorV);
      xV += (ushort32)128;
      uchar32 V = convert_uchar32(xV);

      vstore32(Y, 0, pDstY);
      pDstY += local_dst_pitch;
      vstore32(U, 0, pDstU);
      pDstU += local_dst_pitch;
      vstore32(V, 0, pDstV);
      pDstV += local_dst_pitch;
    }
  }
#endif
#endif

  event_t evtDY = xt_async_work_group_2d_copy(DstY + (out_i * DstPitch) + out_j,
                                              local_dstY, outTileW, outTileH,
                                              DstPitch, local_dst_pitch, 0);

  event_t evtDU = xt_async_work_group_2d_copy(DstU + (out_i * DstPitch) + out_j,
                                              local_dstU, outTileW, outTileH,
                                              DstPitch, local_dst_pitch, 0);

  event_t evtDV = xt_async_work_group_2d_copy(DstV + (out_i * DstPitch) + out_j,
                                              local_dstV, outTileW, outTileH,
                                              DstPitch, local_dst_pitch, 0);

  wait_group_events(1, &evtDY);
  wait_group_events(1, &evtDU);
  wait_group_events(1, &evtDV);
}
