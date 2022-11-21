#include "../../Native/LensDistortionCorrection_ref/LDC_parameters.h"

#define MIN4(a, b, c, d) (min(min(a, b), min(c, d)))
#define MAX4(a, b, c, d) (max(max(a, b), max(c, d)))

__constant ushort seq[] __attribute__((section(".dram0.data"))) = {
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31};

__constant ushort seq_[] __attribute__((section(".dram0.data"))) = {
    0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30,
    0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30};

__constant ushort seq_64_i_c[] __attribute__((section(".dram0.data"))) = {
    63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
    59, 59, 59, 59, 59, 59, 59, 59, 59, 59, 59, 59, 59, 59, 59, 59};

__constant ushort seq_i_c[] __attribute__((section(".dram0.data"))) = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5};

__kernel __attribute__((reqd_work_group_size(1, 1, 1)))
__attribute__((reqd_work_dim_size(2))) void
oclKernel(
    __global uchar *restrict SrcY, __global uchar *restrict SrcUV, int SrcPitch,
    __global uchar *restrict DstY, __global uchar *restrict DstUV,
    const int DstPitch, int width, int height, int dstWidth, int dstHeight,
    int outTileW, int outTileH, __constant ushort *restrict InputX,
    __constant ushort *restrict InputY, __constant ushort *restrict InputBB,
    __local uchar *restrict local_srcY, __local uchar *restrict local_srcUV,
    __local uchar *restrict local_dstY, __local uchar *restrict local_dstUV,
    __global int *restrict err_codes) {
  int i, j, imod, jmod;
  int idx, idy;
  int local_src_pitch, local_dst_pitch;
  int err = 0;
  int tlx, tly, inTileW, inTileH;
  i = get_global_id(0);
  j = get_global_id(1);

  int out_i, out_j;
  out_i = i * outTileH;
  out_j = j * outTileW;
  int dim0size = get_global_size(0);

  const uchar64 interleave64 = {
    0,  64,  2,  66,  4,  68,  6,  70,  8,  72,  10, 74,  12, 76,  14, 78,
    16, 80,  18, 82,  20, 84,  22, 86,  24, 88,  26, 90,  28, 92,  30, 94,
    32, 96,  34, 98,  36, 100, 38, 102, 40, 104, 42, 106, 44, 108, 46, 110,
    48, 112, 50, 114, 52, 116, 54, 118, 56, 120, 58, 122, 60, 124, 62, 126};

  ushort inputxt[4], inputyt[4];

  inputxt[0] = InputX[(i * dim0size + j) * 4];
  inputxt[1] = InputX[(i * dim0size + j) * 4 + 1];
  inputxt[2] = InputX[(i * dim0size + j) * 4 + 2];
  inputxt[3] = InputX[(i * dim0size + j) * 4 + 3];
  inputyt[0] = InputY[(i * dim0size + j) * 4];
  inputyt[1] = InputY[(i * dim0size + j) * 4 + 1];
  inputyt[2] = InputY[(i * dim0size + j) * 4 + 2];
  inputyt[3] = InputY[(i * dim0size + j) * 4 + 3];

  /*pre-Computed bonding Box for Tile DMA In*/
  tlx = InputBB[(i * dim0size + j) * 4];
  tly = InputBB[((i * dim0size + j) * 4) + 1];
  inTileW = InputBB[((i * dim0size + j) * 4) + 2];
  inTileH = InputBB[((i * dim0size + j) * 4) + 3];

  local_src_pitch = inTileW;
  local_dst_pitch = outTileW;

  event_t evtC = xt_async_work_group_2d_copy(
      local_srcY, SrcY + (tly * SrcPitch) + tlx, local_src_pitch, inTileH,
      local_src_pitch, SrcPitch, 0);
  event_t evtD = xt_async_work_group_2d_copy(
      local_srcUV, SrcUV + ((tly >> 1) * SrcPitch) + tlx, local_src_pitch,
      inTileH >> 1, local_src_pitch, SrcPitch, 0);
  wait_group_events(1, &evtC);
  wait_group_events(1, &evtD);
#if NATIVE_KERNEL
  #include "LDC_ivp.c"
#else
  inputxt[0] -= tlx << PIXEL_Q;
  inputxt[1] -= tlx << PIXEL_Q;
  inputxt[2] -= tlx << PIXEL_Q;
  inputxt[3] -= tlx << PIXEL_Q;
  inputyt[0] -= tly << PIXEL_Q;
  inputyt[1] -= tly << PIXEL_Q;
  inputyt[2] -= tly << PIXEL_Q;
  inputyt[3] -= tly << PIXEL_Q;

  __local uchar *dst_Y = (__local uchar *)local_dstY;

  ushort32 seq32 = *(__constant ushort32 *)&seq[0];
  ushort32 seq_32 = (ushort32)32 - seq32;
  ushort32 x0 = (((ushort32)inputxt[0] * seq_32) +
                 ((ushort32)inputxt[1] * seq32) + (ushort32)(outTileW >> 1)) >>
                5;
  ushort32 x1 = (((ushort32)inputxt[2] * seq_32) +
                 ((ushort32)inputxt[3] * seq32) + (ushort32)(outTileW >> 1)) >>
                5;
  ushort32 y0 = (((ushort32)inputyt[0] * seq_32) +
                 ((ushort32)inputyt[1] * seq32) + (ushort32)(outTileW >> 1)) >>
                5;
  ushort32 y1 = (((ushort32)inputyt[2] * seq_32) +
                 ((ushort32)inputyt[3] * seq32) + (ushort32)(outTileW >> 1)) >>
                5;

  for (int y = 0; y < outTileH; y += 1) {
    ushort32 y_32 = (ushort32)32 - (ushort32)y;
    ushort32 x_pos =
        ((y_32 * x0 + (ushort32)y * x1) + (ushort32)(outTileW >> 1)) >> 5;
    ushort32 xint = x_pos >> PIXEL_Q;
    ushort32 xfrac = x_pos & (ushort32)(PIXEL_Q_MASK);

    ushort32 y_pos =
        ((y_32 * y0 + (ushort32)y * y1) + (ushort32)(outTileW >> 1)) >> 5;
    ushort32 yint = y_pos >> PIXEL_Q;
    ushort32 yfrac = y_pos & (ushort32)(PIXEL_Q_MASK);

    ushort32 vecAddr = yint * (ushort32)local_src_pitch + xint;

    ushort32 vecAddrR = vecAddr + (ushort32)1;
    ushort32 vecTL =
        convert_ushort32(xt_gather32(local_srcY, (ushort *)&vecAddr));
    ushort32 vecTR =
        convert_ushort32(xt_gather32(local_srcY, (ushort *)&vecAddrR));
    vecAddr += (ushort32)local_src_pitch;
    vecAddrR += (ushort32)local_src_pitch;
    ushort32 vecBL =
        convert_ushort32(xt_gather32(local_srcY, (ushort *)&vecAddr));
    ushort32 vecBR =
        convert_ushort32(xt_gather32(local_srcY, (ushort *)&vecAddrR));

    ushort32 vecT = ((((ushort32)(PIXEL_DEC)-xfrac) * vecTL + xfrac * vecTR) +
                     (ushort32)(PIXEL_DEC >> 1)) >> 4;
    ushort32 vecB = ((((ushort32)(PIXEL_DEC)-xfrac) * vecBL + xfrac * vecBR) +
                     (ushort32)(PIXEL_DEC >> 1)) >> 4;

    ushort32 vecOut = ((((ushort32)(PIXEL_DEC)-yfrac) * vecT + yfrac * vecB) +
                       (ushort32)(PIXEL_DEC >> 1)) >> 4;
    vstore32(convert_uchar32(vecOut), 0, dst_Y);
    dst_Y += local_dst_pitch;
  }

  __local uchar *dst_UV = (__local uchar *)local_dstUV;
  const __local uchar *src_UV = (const __local uchar *)local_srcUV;

  ushort32 seq32_ = *(__constant ushort32 *)seq_;

  seq_32 = (ushort32)32 - seq32_;
  x0 = (((ushort32)inputxt[0] * seq_32) + ((ushort32)inputxt[1] * seq32_) +
        (ushort32)(outTileW >> 1)) >> 5;
  x1 = (((ushort32)inputxt[2] * seq_32) + ((ushort32)inputxt[3] * seq32_) +
        (ushort32)(outTileW >> 1)) >> 5;
  y0 = (((ushort32)inputyt[0] * seq_32) + ((ushort32)inputyt[1] * seq32_) +
        (ushort32)(outTileW >> 1)) >> 5;
  y1 = (((ushort32)inputyt[2] * seq_32) + ((ushort32)inputyt[3] * seq32_) +
        (ushort32)(outTileW >> 1)) >> 5;

  ushort32 seq_64_i = *(__constant ushort32 *)seq_64_i_c;
  ushort32 seq_i = *(__constant ushort32 *)seq_i_c;
  local_src_pitch = local_src_pitch >> 1;
  for (int y = 1; y < 2 * outTileH; y += 8) {
    ushort32 y_32 = seq_64_i;
    ushort32 iseq = seq_i;
    seq_i += (ushort32)8;
    seq_64_i -= (ushort32)8;

    ushort32 x_pos = ((y_32 * x0 + iseq * x1) + (ushort32)(outTileW)) >> 6;
    ushort32 xint = x_pos >> (PIXEL_Q + 1);
    ushort32 xfrac = x_pos & (ushort32)(PIXEL_Q_MASK_UV);

    ushort32 y_pos = ((y_32 * y0 + iseq * y1) + (ushort32)(outTileW)) >> 6;
    ushort32 yint = y_pos >> (PIXEL_Q + 1);
    ushort32 yfrac = y_pos & (ushort32)(PIXEL_Q_MASK_UV);

    ushort32 vecAddr = yint * (ushort32)local_src_pitch + (xint);
    ushort32 vecTL =
        xt_gather32((const __local ushort *)src_UV, (ushort *)&vecAddr);
    ushort32 vecAddrR = vecAddr + (ushort32)1;
    ushort32 vecTR =
        xt_gather32((const __local ushort *)src_UV, (ushort *)&(vecAddrR));
    vecAddr += (ushort32)(local_src_pitch);
    vecAddrR = vecAddr + (ushort32)1;
    ushort32 vecBL =
        xt_gather32((const __local ushort *)src_UV, (ushort *)&vecAddr);
    ushort32 vecBR =
        xt_gather32((const __local ushort *)src_UV, (ushort *)&(vecAddrR));

    ushort32 vecTL_U = vecTL >> 8;
    ushort32 vecTL_V = vecTL & (ushort32)0xFF;

    ushort32 vecTR_U = vecTR >> 8;
    ushort32 vecTR_V = vecTR & (ushort32)0xFF;

    ushort32 vecBL_U = vecBL >> 8;
    ushort32 vecBL_V = vecBL & (ushort32)0xFF;

    ushort32 vecBR_U = vecBR >> 8;
    ushort32 vecBR_V = vecBR & (ushort32)0xFF;

    ushort32 vecT_U =
        ((((ushort32)(2 * PIXEL_DEC) - xfrac) * vecTL_U + xfrac * vecTR_U) +
         (ushort32)(PIXEL_DEC)) >> 5;
    ushort32 vecB_U =
        ((((ushort32)(2 * PIXEL_DEC) - xfrac) * vecBL_U + xfrac * vecBR_U) +
         (ushort32)(PIXEL_DEC)) >> 5;

    ushort32 vecOut_U =
        ((((ushort32)(2 * PIXEL_DEC) - yfrac) * vecT_U + yfrac * vecB_U) +
         (ushort32)(PIXEL_DEC)) >> 5;

    ushort32 vecT_V =
        ((((ushort32)(2 * PIXEL_DEC) - xfrac) * vecTL_V + xfrac * vecTR_V) +
         (ushort32)(PIXEL_DEC)) >> 5;
    ushort32 vecB_V =
        ((((ushort32)(2 * PIXEL_DEC) - xfrac) * vecBL_V + xfrac * vecBR_V) +
         (ushort32)(PIXEL_DEC)) >> 5;

    ushort32 vecOut_V =
        ((((ushort32)(2 * PIXEL_DEC) - yfrac) * vecT_V + yfrac * vecB_V) +
         (ushort32)(PIXEL_DEC)) >> 5;

    uchar64 vecOut =
        shuffle2(as_uchar64(vecOut_V), as_uchar64(vecOut_U), interleave64);

    vstore64(vecOut, 0, dst_UV);
    dst_UV += 2 * local_dst_pitch;
  }
#endif
  event_t evtF = xt_async_work_group_2d_copy(DstY + (out_i * DstPitch) + out_j,
                                             local_dstY, outTileW, outTileH,
                                             DstPitch, local_dst_pitch, 0);
  event_t evtG = xt_async_work_group_2d_copy(
      DstUV + ((out_i >> 1) * DstPitch) + out_j, local_dstUV, outTileW,
      outTileH >> 1, DstPitch, local_dst_pitch, 0);
  wait_group_events(1, &evtF);
  wait_group_events(1, &evtG);
}
