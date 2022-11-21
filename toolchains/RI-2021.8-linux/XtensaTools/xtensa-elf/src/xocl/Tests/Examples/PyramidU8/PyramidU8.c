#include "xi_lib.h"
#include <xtensa/tie/xt_ivpn.h>
#include <xtensa/tie/xt_misc.h>
#include <math.h>
//#include "xi_imgproc.h"
//#include "xi_intrin.h"
//#include "xi_wide_arithm.h"

#define OFFSET_PTR_N_2X32U(ptr, nrows, stride, in_row_offset)                  \
  ((xb_vecN_2x32Uv *)((uint32_t *)(ptr) + (in_row_offset) + (nrows) * (stride)))
#define OFFSET_PTR_N_2X32(ptr, nrows, stride, in_row_offset)                   \
  ((xb_vecN_2x32v *)((int32_t *)(ptr) + (in_row_offset) + (nrows) * (stride)))
#define OFFSET_PTR_NX16U(ptr, nrows, stride, in_row_offset)                    \
  ((xb_vecNx16U *)((uint16_t *)(ptr) + (in_row_offset) + (nrows) * (stride)))
#define OFFSET_PTR_NX16(ptr, nrows, stride, in_row_offset)                     \
  ((xb_vecNx16 *)((int16_t *)(ptr) + (in_row_offset) + (nrows) * (stride)))
#define OFFSET_PTR_N_2X16(ptr, nrows, stride, in_row_offset)                   \
  ((xb_vecN_2x16 *)((int16_t *)(ptr) + (in_row_offset) + (nrows) * (stride)))
#define OFFSET_PTR_NX8U(ptr, nrows, stride, in_row_offset)                     \
  ((xb_vecNx8U *)((uint8_t *)(ptr) + (in_row_offset) + (nrows) * (stride)))
#define OFFSET_PTR_NX8(ptr, nrows, stride, in_row_offset)                      \
  ((xb_vecNx8 *)((int8_t *)(ptr) + (in_row_offset) + (nrows) * (stride)))
#define OFFSET_PTR_2NX8U(ptr, nrows, stride, in_row_offset)                    \
  ((xb_vec2Nx8U *)((uint8_t *)(ptr) + (in_row_offset) + (nrows) * (stride)))
#define OFFSET_PTR_2NX8(ptr, nrows, stride, in_row_offset)                     \
  ((xb_vec2Nx8 *)((int8_t *)(ptr) + (in_row_offset) + (nrows) * (stride)))

#define IVP_SAVNX8UPOS_FP IVP_SAPOSNX8U_FP

#define XCHAL_IVPN_SIMD_WIDTH_ XCHAL_IVPN_SIMD_WIDTH

XI_ERR_TYPE xiPyrDown_U8(const xi_pTile src, xi_pArray dst) {
  const int width = XI_ARRAY_GET_WIDTH(dst);
  const int height = XI_ARRAY_GET_HEIGHT(dst);

  xb_vecNx8U *restrict pdst = (xb_vecNx8U *)XI_ARRAY_GET_DATA_PTR(dst);
  xb_vecNx8U *restrict rdst;
  valign a_store = IVP_ZALIGN();

#if XCHAL_HAVE_VISION      // VisionP
#if XCHAL_VISION_TYPE == 6 // VisionP6
  xb_vec2Nx8U *restrict psrc = OFFSET_PTR_2NX8U(XI_TILE_GET_DATA_PTR(src), 0,
                                                XI_TILE_GET_PITCH(src), -2);
  xb_vec2Nx8U *restrict rsrc;

  int sstride = XI_TILE_GET_PITCH(src);
  const int dstride = XI_ARRAY_GET_PITCH(dst);

  int j = 0;
  for (; j <= width - 2 * XCHAL_IVPN_SIMD_WIDTH;
       j += 2 * XCHAL_IVPN_SIMD_WIDTH, psrc += 2, pdst += 2) {
    xb_vec2Nx8U vsrc0_0, vsrc0_1, vtail0_0, vtail0_1;
    xb_vec2Nx8U vsrc1_0, vsrc1_1, vtail1_0, vtail1_1;
    xb_vec2Nx8U vsrc2_0, vsrc2_1, vtail2_0, vtail2_1;
    xb_vec2Nx8U vsrc3_0, vsrc3_1, vtail3_0, vtail3_1;
    xb_vec2Nx8U vsrc4_0, vsrc4_1, vtail4_0, vtail4_1;
    xb_vec2Nx8U vsrc5_0, vsrc5_1, vtail5_0, vtail5_1;

    rsrc = OFFSET_PTR_2NX8U(psrc, -2, sstride, 0);
    sstride -= XT_MIN(6 * XCHAL_IVPN_SIMD_WIDTH, 2 * (width - j) + 3);

    int dstep =
        dstride - XT_MIN(2 * XCHAL_IVPN_SIMD_WIDTH, 1 * (width - j) + 0);
    rdst = OFFSET_PTR_NX8U(pdst, -1, dstep, 0);

    { // row -2
      valign a_load = IVP_LA2NX8U_PP(rsrc);
      IVP_LA2NX8U_IP(vsrc0_0, a_load, rsrc);
      IVP_LA2NX8U_IP(vsrc0_1, a_load, rsrc);
      IVP_LAV2NX8U_XP(vtail0_0, a_load, rsrc,
                      sizeof(uint8_t) *
                          (2 * (width - j) + 3 - 4 * XCHAL_IVPN_SIMD_WIDTH));
      rsrc = OFFSET_PTR_2NX8U(rsrc, 1, sstride, 0);

      IVP_DSEL2NX8UI(vsrc0_1, vsrc0_0, vsrc0_1, vsrc0_0,
                     IVP_DSELI_8B_DEINTERLEAVE_1);
      IVP_DSEL2NX8UI(vtail0_1, vtail0_0, 0, vtail0_0,
                     IVP_DSELI_8B_DEINTERLEAVE_1);
    }

    { // row -1
      valign a_load = IVP_LA2NX8U_PP(rsrc);
      IVP_LA2NX8U_IP(vsrc1_0, a_load, rsrc);
      IVP_LA2NX8U_IP(vsrc1_1, a_load, rsrc);
      IVP_LAV2NX8U_XP(vtail1_0, a_load, rsrc,
                      sizeof(uint8_t) *
                          (2 * (width - j) + 3 - 4 * XCHAL_IVPN_SIMD_WIDTH));
      rsrc = OFFSET_PTR_2NX8U(rsrc, 1, sstride, 0);

      IVP_DSEL2NX8UI(vsrc1_1, vsrc1_0, vsrc1_1, vsrc1_0,
                     IVP_DSELI_8B_DEINTERLEAVE_1);
      IVP_DSEL2NX8UI(vtail1_1, vtail1_0, 0, vtail1_0,
                     IVP_DSELI_8B_DEINTERLEAVE_1);
    }

    { // row 0
      valign a_load = IVP_LA2NX8U_PP(rsrc);
      IVP_LA2NX8U_IP(vsrc2_0, a_load, rsrc);
      IVP_LA2NX8U_IP(vsrc2_1, a_load, rsrc);
      IVP_LAV2NX8U_XP(vtail2_0, a_load, rsrc,
                      sizeof(uint8_t) *
                          (2 * (width - j) + 3 - 4 * XCHAL_IVPN_SIMD_WIDTH));
      rsrc = OFFSET_PTR_2NX8U(rsrc, 1, sstride, 0);

      IVP_DSEL2NX8UI(vsrc2_1, vsrc2_0, vsrc2_1, vsrc2_0,
                     IVP_DSELI_8B_DEINTERLEAVE_1);
      IVP_DSEL2NX8UI(vtail2_1, vtail2_0, 0, vtail2_0,
                     IVP_DSELI_8B_DEINTERLEAVE_1);
    }

    { // row 1
      valign a_load = IVP_LA2NX8U_PP(rsrc);
      IVP_LA2NX8U_IP(vsrc3_0, a_load, rsrc);
      IVP_LA2NX8U_IP(vsrc3_1, a_load, rsrc);
      IVP_LAV2NX8U_XP(vtail3_0, a_load, rsrc,
                      sizeof(uint8_t) *
                          (2 * (width - j) + 3 - 4 * XCHAL_IVPN_SIMD_WIDTH));
      rsrc = OFFSET_PTR_2NX8U(rsrc, 1, sstride, 0);

      IVP_DSEL2NX8UI(vsrc3_1, vsrc3_0, vsrc3_1, vsrc3_0,
                     IVP_DSELI_8B_DEINTERLEAVE_1);
      IVP_DSEL2NX8UI(vtail3_1, vtail3_0, 0, vtail3_0,
                     IVP_DSELI_8B_DEINTERLEAVE_1);
    }

    for (int i = 0; i < height; i++) {
      {
        valign a_load = IVP_LA2NX8U_PP(rsrc);
        IVP_LA2NX8U_IP(vsrc4_0, a_load, rsrc);
        IVP_LA2NX8U_IP(vsrc4_1, a_load, rsrc);
        IVP_LAV2NX8U_XP(vtail4_0, a_load, rsrc,
                        sizeof(uint8_t) *
                            (2 * (width - j) + 3 - 4 * XCHAL_IVPN_SIMD_WIDTH));
        rsrc = OFFSET_PTR_2NX8U(rsrc, 1, sstride, 0);

        IVP_DSEL2NX8UI(vsrc4_1, vsrc4_0, vsrc4_1, vsrc4_0,
                       IVP_DSELI_8B_DEINTERLEAVE_1);
        IVP_DSEL2NX8UI(vtail4_1, vtail4_0, 0, vtail4_0,
                       IVP_DSELI_8B_DEINTERLEAVE_1);
      }

      {
        valign a_load = IVP_LA2NX8U_PP(rsrc);
        IVP_LA2NX8U_IP(vsrc5_0, a_load, rsrc);
        IVP_LA2NX8U_IP(vsrc5_1, a_load, rsrc);
        IVP_LAV2NX8U_XP(vtail5_0, a_load, rsrc,
                        sizeof(uint8_t) *
                            (2 * (width - j) + 3 - 4 * XCHAL_IVPN_SIMD_WIDTH));
        rsrc = OFFSET_PTR_2NX8U(rsrc, 1, sstride, 0);

        IVP_DSEL2NX8UI(vsrc5_1, vsrc5_0, vsrc5_1, vsrc5_0,
                       IVP_DSELI_8B_DEINTERLEAVE_1);
        IVP_DSEL2NX8UI(vtail5_1, vtail5_0, 0, vtail5_0,
                       IVP_DSELI_8B_DEINTERLEAVE_1);
      }

      xb_vec2Nx24 wblurred = IVP_MULUS4T2N8XR8(vtail0_0, vsrc0_0, 0x010601);
      IVP_MULUS4TA2N8XR8(wblurred, vtail0_1, vsrc0_1, 0x0404);
      IVP_MULUS4TA2N8XR8(wblurred, vtail1_0, vsrc1_0, 0x041804);
      IVP_MULUS4TA2N8XR8(wblurred, vtail1_1, vsrc1_1, 0x1010);
      IVP_MULUS4TA2N8XR8(wblurred, vtail2_0, vsrc2_0, 0x062406);
      IVP_MULUS4TA2N8XR8(wblurred, vtail2_1, vsrc2_1, 0x1818);
      IVP_MULUS4TA2N8XR8(wblurred, vtail3_0, vsrc3_0, 0x041804);
      IVP_MULUS4TA2N8XR8(wblurred, vtail3_1, vsrc3_1, 0x1010);
      IVP_MULUS4TA2N8XR8(wblurred, vtail4_0, vsrc4_0, 0x010601);
      IVP_MULUS4TA2N8XR8(wblurred, vtail4_1, vsrc4_1, 0x0404);

      xb_vecNx16U vblurred0 = IVP_PACKVRU2NX24_0(wblurred, 8);
      xb_vecNx16U vblurred1 = IVP_PACKVRU2NX24_1(wblurred, 8);
      IVP_DSELNX16UI(vblurred1, vblurred0, vblurred1, vblurred0,
                     IVP_DSELI_INTERLEAVE_1);

      rdst = OFFSET_PTR_NX8U(rdst, 1, dstep, 0);
      IVP_SANX8U_IP(vblurred0, a_store, rdst);
      IVP_SANX8U_IP(vblurred1, a_store, rdst);
      IVP_SAVNX8UPOS_FP(a_store, rdst);

      vsrc0_0 = vsrc2_0;
      vsrc0_1 = vsrc2_1;
      vtail0_0 = vtail2_0;
      vtail0_1 = vtail2_1;
      vsrc1_0 = vsrc3_0;
      vsrc1_1 = vsrc3_1;
      vtail1_0 = vtail3_0;
      vtail1_1 = vtail3_1;
      vsrc2_0 = vsrc4_0;
      vsrc2_1 = vsrc4_1;
      vtail2_0 = vtail4_0;
      vtail2_1 = vtail4_1;
      vsrc3_0 = vsrc5_0;
      vsrc3_1 = vsrc5_1;
      vtail3_0 = vtail5_0;
      vtail3_1 = vtail5_1;
    }
    sstride += XT_MIN(6 * XCHAL_IVPN_SIMD_WIDTH, 2 * (width - j) + 3);
  }
  for (; j < width; j += XCHAL_IVPN_SIMD_WIDTH, psrc += 1, pdst += 1) {
    xb_vec2Nx8U vsrc0_0, vsrc0_1, vtail0_0, vtail0_1;
    xb_vec2Nx8U vsrc1_0, vsrc1_1, vtail1_0, vtail1_1;
    xb_vec2Nx8U vsrc2_0, vsrc2_1, vtail2_0, vtail2_1;
    xb_vec2Nx8U vsrc3_0, vsrc3_1, vtail3_0, vtail3_1;
    xb_vec2Nx8U vsrc4_0, vsrc4_1, vtail4_0, vtail4_1;
    xb_vec2Nx8U vsrc5_0, vsrc5_1, vtail5_0, vtail5_1;
    xb_vec2Nx8U vsrc6_0, vsrc6_1, vtail6_0, vtail6_1;
    xb_vec2Nx8U vsrc7_0, vsrc7_1, vtail7_0, vtail7_1;

    rsrc = OFFSET_PTR_2NX8U(psrc, -2, sstride, 0);
    sstride -= XT_MIN(4 * XCHAL_IVPN_SIMD_WIDTH, 2 * (width - j) + 3);
    int dstep =
        dstride - XT_MIN(1 * XCHAL_IVPN_SIMD_WIDTH, 1 * (width - j) + 0);
    rdst = OFFSET_PTR_NX8U(pdst, -1, dstep, 0);

    xb_vecN_2x32v vloadtail0 =
        sizeof(uint8_t) * (2 * (width - j) + 3 - 0 * XCHAL_IVPN_SIMD_WIDTH);
    xb_vecN_2x32v vloadtail1 =
        sizeof(uint8_t) * (2 * (width - j) + 3 - 2 * XCHAL_IVPN_SIMD_WIDTH);
    vboolN_2 vselector = IVP_EQN_2X32(IVP_SEQN_2X32(), 1);
    xb_vecN_2x32v vloadtails =
        IVP_MOVN_2X32T(vloadtail1, vloadtail0, vselector);

    { // row -2
      valign a_load = IVP_LA2NX8U_PP(rsrc);
      IVP_LAV2NX8U_XP(vsrc0_0, a_load, rsrc,
                      sizeof(uint8_t) *
                          (2 * (width - j) + 3 - 0 * XCHAL_IVPN_SIMD_WIDTH));
      IVP_LAV2NX8U_XP(vtail0_0, a_load, rsrc,
                      sizeof(uint8_t) *
                          (2 * (width - j) + 3 - 2 * XCHAL_IVPN_SIMD_WIDTH));
      rsrc = OFFSET_PTR_2NX8U(rsrc, 1, sstride, 0);

      IVP_DSEL2NX8UI(vsrc0_1, vsrc0_0, vtail0_0, vsrc0_0,
                     IVP_DSELI_8B_DEINTERLEAVE_1);
      IVP_DSEL2NX8UI(vtail0_1, vtail0_0, 0, vtail0_0,
                     IVP_DSELI_8B_DEINTERLEAVE_1);
    }

    { // row -1
      valign a_load = IVP_LA2NX8U_PP(rsrc);
      IVP_LAV2NX8U_XP(vsrc1_0, a_load, rsrc,
                      sizeof(uint8_t) *
                          (2 * (width - j) + 3 - 0 * XCHAL_IVPN_SIMD_WIDTH));
      IVP_LAV2NX8U_XP(vtail1_0, a_load, rsrc,
                      sizeof(uint8_t) *
                          (2 * (width - j) + 3 - 2 * XCHAL_IVPN_SIMD_WIDTH));
      rsrc = OFFSET_PTR_2NX8U(rsrc, 1, sstride, 0);

      IVP_DSEL2NX8UI(vsrc1_1, vsrc1_0, vtail1_0, vsrc1_0,
                     IVP_DSELI_8B_DEINTERLEAVE_1);
      IVP_DSEL2NX8UI(vtail1_1, vtail1_0, 0, vtail1_0,
                     IVP_DSELI_8B_DEINTERLEAVE_1);
    }

    { // row 0
      valign a_load = IVP_LA2NX8U_PP(rsrc);
      IVP_LAV2NX8U_XP(vsrc2_0, a_load, rsrc,
                      sizeof(uint8_t) *
                          (2 * (width - j) + 3 - 0 * XCHAL_IVPN_SIMD_WIDTH));
      IVP_LAV2NX8U_XP(vtail2_0, a_load, rsrc,
                      sizeof(uint8_t) *
                          (2 * (width - j) + 3 - 2 * XCHAL_IVPN_SIMD_WIDTH));
      rsrc = OFFSET_PTR_2NX8U(rsrc, 1, sstride, 0);

      IVP_DSEL2NX8UI(vsrc2_1, vsrc2_0, vtail2_0, vsrc2_0,
                     IVP_DSELI_8B_DEINTERLEAVE_1);
      IVP_DSEL2NX8UI(vtail2_1, vtail2_0, 0, vtail2_0,
                     IVP_DSELI_8B_DEINTERLEAVE_1);
    }

    { // row 1
      valign a_load = IVP_LA2NX8U_PP(rsrc);
      IVP_LAV2NX8U_XP(vsrc3_0, a_load, rsrc,
                      sizeof(uint8_t) *
                          (2 * (width - j) + 3 - 0 * XCHAL_IVPN_SIMD_WIDTH));
      IVP_LAV2NX8U_XP(vtail3_0, a_load, rsrc,
                      sizeof(uint8_t) *
                          (2 * (width - j) + 3 - 2 * XCHAL_IVPN_SIMD_WIDTH));
      rsrc = OFFSET_PTR_2NX8U(rsrc, 1, sstride, 0);

      IVP_DSEL2NX8UI(vsrc3_1, vsrc3_0, vtail3_0, vsrc3_0,
                     IVP_DSELI_8B_DEINTERLEAVE_1);
      IVP_DSEL2NX8UI(vtail3_1, vtail3_0, 0, vtail3_0,
                     IVP_DSELI_8B_DEINTERLEAVE_1);
    }

    int i = 0;
    for (; i < height - 1; i += 2) {
      vloadtails = IVP_ANDN_2X32(vloadtails, -1);

      {
        valign a_load = IVP_LA2NX8U_PP(rsrc);
        IVP_LAV2NX8U_XP(vsrc4_0, a_load, rsrc, IVP_EXTRN_2X32(vloadtails, 0));
        IVP_LAV2NX8U_XP(vtail4_0, a_load, rsrc, IVP_EXTRN_2X32(vloadtails, 1));
        rsrc = OFFSET_PTR_2NX8U(rsrc, 1, sstride, 0);

        IVP_DSEL2NX8UI(vsrc4_1, vsrc4_0, vtail4_0, vsrc4_0,
                       IVP_DSELI_8B_DEINTERLEAVE_1);
        IVP_DSEL2NX8UI(vtail4_1, vtail4_0, 0, vtail4_0,
                       IVP_DSELI_8B_DEINTERLEAVE_1);
      }

      {
        valign a_load = IVP_LA2NX8U_PP(rsrc);
        IVP_LAV2NX8U_XP(vsrc5_0, a_load, rsrc, IVP_EXTRN_2X32(vloadtails, 0));
        IVP_LAV2NX8U_XP(vtail5_0, a_load, rsrc, IVP_EXTRN_2X32(vloadtails, 1));
        rsrc = OFFSET_PTR_2NX8U(rsrc, 1, sstride, 0);

        IVP_DSEL2NX8UI(vsrc5_1, vsrc5_0, vtail5_0, vsrc5_0,
                       IVP_DSELI_8B_DEINTERLEAVE_1);
        IVP_DSEL2NX8UI(vtail5_1, vtail5_0, 0, vtail5_0,
                       IVP_DSELI_8B_DEINTERLEAVE_1);
      }

      {
        valign a_load = IVP_LA2NX8U_PP(rsrc);
        IVP_LAV2NX8U_XP(vsrc6_0, a_load, rsrc, IVP_EXTRN_2X32(vloadtails, 0));
        IVP_LAV2NX8U_XP(vtail6_0, a_load, rsrc, IVP_EXTRN_2X32(vloadtails, 1));
        rsrc = OFFSET_PTR_2NX8U(rsrc, 1, sstride, 0);

        IVP_DSEL2NX8UI(vsrc6_1, vsrc6_0, vtail6_0, vsrc6_0,
                       IVP_DSELI_8B_DEINTERLEAVE_1);
        IVP_DSEL2NX8UI(vtail6_1, vtail6_0, 0, vtail6_0,
                       IVP_DSELI_8B_DEINTERLEAVE_1);
      }

      {
        valign a_load = IVP_LA2NX8U_PP(rsrc);
        IVP_LAV2NX8U_XP(vsrc7_0, a_load, rsrc, IVP_EXTRN_2X32(vloadtails, 0));
        IVP_LAV2NX8U_XP(vtail7_0, a_load, rsrc, IVP_EXTRN_2X32(vloadtails, 1));
        rsrc = OFFSET_PTR_2NX8U(rsrc, 1, sstride, 0);

        IVP_DSEL2NX8UI(vsrc7_1, vsrc7_0, vtail7_0, vsrc7_0,
                       IVP_DSELI_8B_DEINTERLEAVE_1);
        IVP_DSEL2NX8UI(vtail7_1, vtail7_0, 0, vtail7_0,
                       IVP_DSELI_8B_DEINTERLEAVE_1);
      }

      xb_vec2Nx24 wblurred0 = IVP_MULUS4T2N8XR8(vtail0_0, vsrc0_0, 0x010601);
      IVP_MULUS4TA2N8XR8(wblurred0, vtail0_1, vsrc0_1, 0x0404);
      IVP_MULUS4TA2N8XR8(wblurred0, vtail1_0, vsrc1_0, 0x041804);
      IVP_MULUS4TA2N8XR8(wblurred0, vtail1_1, vsrc1_1, 0x1010);
      IVP_MULUS4TA2N8XR8(wblurred0, vtail2_0, vsrc2_0, 0x062406);
      IVP_MULUS4TA2N8XR8(wblurred0, vtail2_1, vsrc2_1, 0x1818);
      IVP_MULUS4TA2N8XR8(wblurred0, vtail3_0, vsrc3_0, 0x041804);
      IVP_MULUS4TA2N8XR8(wblurred0, vtail3_1, vsrc3_1, 0x1010);
      IVP_MULUS4TA2N8XR8(wblurred0, vtail4_0, vsrc4_0, 0x010601);
      IVP_MULUS4TA2N8XR8(wblurred0, vtail4_1, vsrc4_1, 0x0404);

      xb_vec2Nx8U vblurred0 = IVP_PACKVRU2NX24(wblurred0, 8);
      xb_vecNx16U vblurred0_0 =
          IVP_ANDNX16U(IVP_MOVNX16_FROM2NX8U(vblurred0), 255);
      xb_vecNx16U vblurred0_1 =
          IVP_SRLINX16U(IVP_MOVNX16_FROM2NX8U(vblurred0), 8);
      vblurred0_0 =
          IVP_SELNX16UI(vblurred0_1, vblurred0_0, IVP_SELI_INTERLEAVE_1_LO);

      xb_vec2Nx24 wblurred1 = IVP_MULUS4T2N8XR8(vtail2_0, vsrc2_0, 0x010601);
      IVP_MULUS4TA2N8XR8(wblurred1, vtail2_1, vsrc2_1, 0x0404);
      IVP_MULUS4TA2N8XR8(wblurred1, vtail3_0, vsrc3_0, 0x041804);
      IVP_MULUS4TA2N8XR8(wblurred1, vtail3_1, vsrc3_1, 0x1010);
      IVP_MULUS4TA2N8XR8(wblurred1, vtail4_0, vsrc4_0, 0x062406);
      IVP_MULUS4TA2N8XR8(wblurred1, vtail4_1, vsrc4_1, 0x1818);
      IVP_MULUS4TA2N8XR8(wblurred1, vtail5_0, vsrc5_0, 0x041804);
      IVP_MULUS4TA2N8XR8(wblurred1, vtail5_1, vsrc5_1, 0x1010);
      IVP_MULUS4TA2N8XR8(wblurred1, vtail6_0, vsrc6_0, 0x010601);
      IVP_MULUS4TA2N8XR8(wblurred1, vtail6_1, vsrc6_1, 0x0404);

      xb_vec2Nx8U vblurred1 = IVP_PACKVRU2NX24(wblurred1, 8);
      xb_vecNx16U vblurred1_0 =
          IVP_ANDNX16U(IVP_MOVNX16_FROM2NX8U(vblurred1), 255);
      xb_vecNx16U vblurred1_1 =
          IVP_SRLINX16U(IVP_MOVNX16_FROM2NX8U(vblurred1), 8);
      vblurred1_0 =
          IVP_SELNX16UI(vblurred1_1, vblurred1_0, IVP_SELI_INTERLEAVE_1_LO);

      rdst = OFFSET_PTR_NX8U(rdst, 1, dstep, 0);
      IVP_SAVNX8U_XP(vblurred0_0, a_store, rdst, sizeof(uint8_t) * (width - j));
      IVP_SAVNX8UPOS_FP(a_store, rdst);

      rdst = OFFSET_PTR_NX8U(rdst, 1, dstep, 0);
      IVP_SAVNX8U_XP(vblurred1_0, a_store, rdst, sizeof(uint8_t) * (width - j));
      IVP_SAVNX8UPOS_FP(a_store, rdst);

      vsrc0_0 = vsrc4_0;
      vsrc0_1 = vsrc4_1;
      vtail0_0 = vtail4_0;
      vtail0_1 = vtail4_1;
      vsrc1_0 = vsrc5_0;
      vsrc1_1 = vsrc5_1;
      vtail1_0 = vtail5_0;
      vtail1_1 = vtail5_1;
      vsrc2_0 = vsrc6_0;
      vsrc2_1 = vsrc6_1;
      vtail2_0 = vtail6_0;
      vtail2_1 = vtail6_1;
      vsrc3_0 = vsrc7_0;
      vsrc3_1 = vsrc7_1;
      vtail3_0 = vtail7_0;
      vtail3_1 = vtail7_1;
    }
    if (height & 1) {
      {
        valign a_load = IVP_LA2NX8U_PP(rsrc);
        IVP_LAV2NX8U_XP(vsrc4_0, a_load, rsrc,
                        sizeof(uint8_t) *
                            (2 * (width - j) + 3 - 0 * XCHAL_IVPN_SIMD_WIDTH));
        IVP_LAV2NX8U_XP(vtail4_0, a_load, rsrc,
                        sizeof(uint8_t) *
                            (2 * (width - j) + 3 - 2 * XCHAL_IVPN_SIMD_WIDTH));
        rsrc = OFFSET_PTR_2NX8U(rsrc, 1, sstride, 0);

        IVP_DSEL2NX8UI(vsrc4_1, vsrc4_0, vtail4_0, vsrc4_0,
                       IVP_DSELI_8B_DEINTERLEAVE_1);
        IVP_DSEL2NX8UI(vtail4_1, vtail4_0, 0, vtail4_0,
                       IVP_DSELI_8B_DEINTERLEAVE_1);
      }

      {
        valign a_load = IVP_LA2NX8U_PP(rsrc);
        IVP_LAV2NX8U_XP(vsrc5_0, a_load, rsrc,
                        sizeof(uint8_t) *
                            (2 * (width - j) + 3 - 0 * XCHAL_IVPN_SIMD_WIDTH));
        IVP_LAV2NX8U_XP(vtail5_0, a_load, rsrc,
                        sizeof(uint8_t) *
                            (2 * (width - j) + 3 - 2 * XCHAL_IVPN_SIMD_WIDTH));
        rsrc = OFFSET_PTR_2NX8U(rsrc, 1, sstride, 0);

        IVP_DSEL2NX8UI(vsrc5_1, vsrc5_0, vtail5_0, vsrc5_0,
                       IVP_DSELI_8B_DEINTERLEAVE_1);
        IVP_DSEL2NX8UI(vtail5_1, vtail5_0, 0, vtail5_0,
                       IVP_DSELI_8B_DEINTERLEAVE_1);
      }

      xb_vec2Nx24 wblurred = IVP_MULUS4T2N8XR8(vtail0_0, vsrc0_0, 0x010601);
      IVP_MULUS4TA2N8XR8(wblurred, vtail0_1, vsrc0_1, 0x0404);
      IVP_MULUS4TA2N8XR8(wblurred, vtail1_0, vsrc1_0, 0x041804);
      IVP_MULUS4TA2N8XR8(wblurred, vtail1_1, vsrc1_1, 0x1010);
      IVP_MULUS4TA2N8XR8(wblurred, vtail2_0, vsrc2_0, 0x062406);
      IVP_MULUS4TA2N8XR8(wblurred, vtail2_1, vsrc2_1, 0x1818);
      IVP_MULUS4TA2N8XR8(wblurred, vtail3_0, vsrc3_0, 0x041804);
      IVP_MULUS4TA2N8XR8(wblurred, vtail3_1, vsrc3_1, 0x1010);
      IVP_MULUS4TA2N8XR8(wblurred, vtail4_0, vsrc4_0, 0x010601);
      IVP_MULUS4TA2N8XR8(wblurred, vtail4_1, vsrc4_1, 0x0404);

      xb_vec2Nx8U vblurred = IVP_PACKVRU2NX24(wblurred, 8);
      xb_vecNx16U vblurred0 =
          IVP_ANDNX16U(IVP_MOVNX16_FROM2NX8U(vblurred), 255);
      xb_vecNx16U vblurred1 = IVP_SRLINX16U(IVP_MOVNX16_FROM2NX8U(vblurred), 8);
      vblurred0 = IVP_SELNX16UI(vblurred1, vblurred0, IVP_SELI_INTERLEAVE_1_LO);

      rdst = OFFSET_PTR_NX8U(rdst, 1, dstep, 0);
      IVP_SAVNX8U_XP(vblurred0, a_store, rdst, sizeof(uint8_t) * (width - j));
      IVP_SAVNX8UPOS_FP(a_store, rdst);
    }
    sstride += XT_MIN(4 * XCHAL_IVPN_SIMD_WIDTH, 2 * (width - j) + 3);
  }
#else // VisionP and not VisionP6
  xb_vec2Nx8U *restrict psrc = OFFSET_PTR_2NX8U(XI_TILE_GET_DATA_PTR(src), 0,
                                                XI_TILE_GET_PITCH(src), -2);
  xb_vec2Nx8U *restrict rsrc;

#define LOAD_ROW_2x8x2_x5(reg0, reg1)                                          \
  {                                                                            \
    xb_vec2Nx8U vsel0_0, vsel1_0, vsel2_0, vsel3_0, vsel4_0, vtail0_0,         \
        vtail1_0;                                                              \
    valign a_load = IVP_LA2NX8U_PP(rsrc);                                      \
    IVP_LA2NX8U_IP(vsel0_0, a_load, rsrc);                                     \
    IVP_LAV2NX8U_XP(vsel1_0, a_load, rsrc,                                     \
                    2 * (width - j) + 3 - 2 * XCHAL_IVPN_SIMD_WIDTH);          \
    IVP_LAV2NX8U_XP(vtail0_0, a_load, rsrc,                                    \
                    2 * (width - j) + 3 - 4 * XCHAL_IVPN_SIMD_WIDTH);          \
    rsrc = OFFSET_PTR_2NX8U(rsrc, 1, sstep, 0);                                \
    IVP_DSEL2NX8I(vsel1_0, vsel0_0, vsel1_0, vsel0_0,                          \
                  IVP_DSELI_8B_DEINTERLEAVE_1);                                \
    IVP_DSEL2NX8I(vtail1_0, vtail0_0, 0, vtail0_0,                             \
                  IVP_DSELI_8B_DEINTERLEAVE_1);                                \
    IVP_DSEL2NX8I(vsel4_0, vsel2_0, vtail0_0, vsel0_0,                         \
                  IVP_DSELI_8B_ROTATE_RIGHT_2_1);                              \
    vsel3_0 = IVP_SEL2NX8I(vtail1_0, vsel1_0, IVP_SELI_8B_ROTATE_RIGHT_1);     \
    xb_vec2Nx8U vsel0_1, vsel1_1, vsel2_1, vsel3_1, vsel4_1, vtail0_1,         \
        vtail1_1;                                                              \
    a_load = IVP_LA2NX8U_PP(rsrc);                                             \
    IVP_LA2NX8U_IP(vsel0_1, a_load, rsrc);                                     \
    IVP_LAV2NX8U_XP(vsel1_1, a_load, rsrc,                                     \
                    2 * (width - j) + 3 - 2 * XCHAL_IVPN_SIMD_WIDTH);          \
    IVP_LAV2NX8U_XP(vtail0_1, a_load, rsrc,                                    \
                    2 * (width - j) + 3 - 4 * XCHAL_IVPN_SIMD_WIDTH);          \
    rsrc = OFFSET_PTR_2NX8U(rsrc, 1, sstep, 0);                                \
    IVP_DSEL2NX8I(vsel1_1, vsel0_1, vsel1_1, vsel0_1,                          \
                  IVP_DSELI_8B_DEINTERLEAVE_1);                                \
    IVP_DSEL2NX8I(vtail1_1, vtail0_1, 0, vtail0_1,                             \
                  IVP_DSELI_8B_DEINTERLEAVE_1);                                \
    IVP_DSEL2NX8I(vsel4_1, vsel2_1, vtail0_1, vsel0_1,                         \
                  IVP_DSELI_8B_ROTATE_RIGHT_2_1);                              \
    vsel3_1 = IVP_SEL2NX8I(vtail1_1, vsel1_1, IVP_SELI_8B_ROTATE_RIGHT_1);     \
    xb_vec2Nx24 w_0 = IVP_MULUU2NX8(4, vsel1_0);                               \
    IVP_MULUUA2NX8(w_0, 6, vsel2_0);                                           \
    IVP_MULUUA2NX8(w_0, 4, vsel3_0);                                           \
    IVP_ADDWUA2NX8(w_0, vsel0_0, vsel4_0);                                     \
    xb_vec2Nx24 w_1 = IVP_MULUU2NX8(4, vsel1_1);                               \
    IVP_MULUUA2NX8(w_1, 6, vsel2_1);                                           \
    IVP_MULUUA2NX8(w_1, 4, vsel3_1);                                           \
    IVP_ADDWUA2NX8(w_1, vsel0_1, vsel4_1);                                     \
    reg0##_0 = IVP_CVT16U2NX24L(w_0);                                          \
    reg1##_0 = IVP_CVT16U2NX24L(w_1);                                          \
    reg0##_1 = IVP_CVT16U2NX24H(w_0);                                          \
    reg1##_1 = IVP_CVT16U2NX24H(w_1);                                          \
  }

#define LOAD_ROW_1x8x2_x5(reg0, reg1)                                          \
  {                                                                            \
    xb_vec2Nx8U vsel0_0, vsel1_0, vsel2_0, vsel3_0, vsel4_0, vtail_0;          \
    valign a_load = IVP_LA2NX8U_PP(rsrc);                                      \
    IVP_LAV2NX8U_XP(vsel0_0, a_load, rsrc, 2 * (width - j) + 3);               \
    IVP_LAV2NX8U_XP(vtail_0, a_load, rsrc,                                     \
                    2 * (width - j) + 3 - 2 * XCHAL_IVPN_SIMD_WIDTH);          \
    rsrc = OFFSET_PTR_2NX8U(rsrc, 1, sstep, 0);                                \
    IVP_DSEL2NX8I(vsel1_0, vsel0_0, vtail_0, vsel0_0,                          \
                  IVP_DSELI_8B_DEINTERLEAVE_1);                                \
    IVP_DSEL2NX8I(vsel4_0, vsel2_0, 0, vsel0_0,                                \
                  IVP_DSELI_8B_ROTATE_RIGHT_2_1);                              \
    vsel3_0 = IVP_SEL2NX8I(0, vsel1_0, IVP_SELI_8B_ROTATE_RIGHT_1);            \
    xb_vec2Nx8U vsel0_1, vsel1_1, vsel2_1, vsel3_1, vsel4_1, vtail_1;          \
    a_load = IVP_LA2NX8U_PP(rsrc);                                             \
    IVP_LAV2NX8U_XP(vsel0_1, a_load, rsrc, 2 * (width - j) + 3);               \
    IVP_LAV2NX8U_XP(vtail_1, a_load, rsrc,                                     \
                    2 * (width - j) + 3 - 2 * XCHAL_IVPN_SIMD_WIDTH);          \
    rsrc = OFFSET_PTR_2NX8U(rsrc, 1, sstep, 0);                                \
    IVP_DSEL2NX8I(vsel1_1, vsel0_1, vtail_1, vsel0_1,                          \
                  IVP_DSELI_8B_DEINTERLEAVE_1);                                \
    IVP_DSEL2NX8I(vsel4_1, vsel2_1, 0, vsel0_1,                                \
                  IVP_DSELI_8B_ROTATE_RIGHT_2_1);                              \
    vsel3_1 = IVP_SEL2NX8I(0, vsel1_1, IVP_SELI_8B_ROTATE_RIGHT_1);            \
    xb_vec2Nx24 w_0 = IVP_MULUU2NX8(4, vsel1_0);                               \
    IVP_MULUUA2NX8(w_0, 6, vsel2_0);                                           \
    IVP_MULUUA2NX8(w_0, 4, vsel3_0);                                           \
    IVP_ADDWUA2NX8(w_0, vsel0_0, vsel4_0);                                     \
    xb_vec2Nx24 w_1 = IVP_MULUU2NX8(4, vsel1_1);                               \
    IVP_MULUUA2NX8(w_1, 6, vsel2_1);                                           \
    IVP_MULUUA2NX8(w_1, 4, vsel3_1);                                           \
    IVP_ADDWUA2NX8(w_1, vsel0_1, vsel4_1);                                     \
    reg0 = IVP_CVT16U2NX24L(w_0);                                              \
    reg1 = IVP_CVT16U2NX24L(w_1);                                              \
  }
  int j = 0;
  for (; j < width - XCHAL_IVPN_SIMD_WIDTH;
       j += 2 * XCHAL_IVPN_SIMD_WIDTH, psrc += 2, pdst += 2) {
    xb_vecNx16 pp0_0, pp1_0, pp2_0, pp3_0, pp4_0, pp5_0, pp6_0, pp7_0;
    xb_vecNx16 pp0_1, pp1_1, pp2_1, pp3_1, pp4_1, pp5_1, pp6_1, pp7_1;
    rsrc = OFFSET_PTR_2NX8U(psrc, -2, XI_TILE_GET_PITCH(src), 0);
    rdst = pdst;

    int sstep = XI_TILE_GET_PITCH(src) -
                XT_MIN(6 * XCHAL_IVPN_SIMD_WIDTH, 2 * (width - j) + 3);
    int dstep =
        XI_ARRAY_GET_PITCH(dst) - XT_MIN(2 * XCHAL_IVPN_SIMD_WIDTH_, width - j);

    LOAD_ROW_2x8x2_x5(pp0, pp1);
    LOAD_ROW_2x8x2_x5(pp2, pp3);

    int i = 0;
    for (; i < height - 1; i += 2) {
      LOAD_ROW_2x8x2_x5(pp4, pp5);
      LOAD_ROW_2x8x2_x5(pp6, pp7);

      xb_vec2Nx24 w0 = IVP_MULUSI2NR8X16(6, pp2_1, pp2_0);
      IVP_MULUSAI2NX8X16(w0, 4, IVP_ADDNX16(pp1_1, pp3_1),
                         IVP_ADDNX16(pp1_0, pp3_0));
      IVP_MULUSAI2NX8X16(w0, 1, IVP_ADDNX16(pp0_1, pp4_1),
                         IVP_ADDNX16(pp0_0, pp4_0));

      xb_vec2Nx24 w1 = IVP_MULUSI2NR8X16(6, pp4_1, pp4_0);
      IVP_MULUSAI2NX8X16(w1, 4, IVP_ADDNX16(pp3_1, pp5_1),
                         IVP_ADDNX16(pp3_0, pp5_0));
      IVP_MULUSAI2NX8X16(w1, 1, IVP_ADDNX16(pp2_1, pp6_1),
                         IVP_ADDNX16(pp2_0, pp6_0));

      IVP_SANX8U_IP(IVP_PACKVRU2NX24_0(w0, 8), a_store, rdst);
      IVP_SAVNX8U_XP(IVP_PACKVRU2NX24_1(w0, 8), a_store, rdst,
                     width - j - XCHAL_IVPN_SIMD_WIDTH);
      IVP_SAVNX8UPOS_FP(a_store, rdst);
      rdst = OFFSET_PTR_NX8U(rdst, 1, dstep, 0);

      IVP_SANX8U_IP(IVP_PACKVRU2NX24_0(w1, 8), a_store, rdst);
      IVP_SAVNX8U_XP(IVP_PACKVRU2NX24_1(w1, 8), a_store, rdst,
                     width - j - XCHAL_IVPN_SIMD_WIDTH);
      IVP_SAVNX8UPOS_FP(a_store, rdst);
      rdst = OFFSET_PTR_NX8U(rdst, 1, dstep, 0);

      pp0_0 = pp4_0;
      pp1_0 = pp5_0;
      pp2_0 = pp6_0;
      pp3_0 = pp7_0;
      pp0_1 = pp4_1;
      pp1_1 = pp5_1;
      pp2_1 = pp6_1;
      pp3_1 = pp7_1;
    }
    if (i < height) {
      {
        xb_vec2Nx8U vsel0_0, vsel1_0, vsel2_0, vsel3_0, vsel4_0, vtail0_0,
            vtail1_0;
        valign a_load = IVP_LA2NX8U_PP(rsrc);
        IVP_LA2NX8U_IP(vsel0_0, a_load, rsrc);
        IVP_LAV2NX8U_XP(vsel1_0, a_load, rsrc,
                        2 * (width - j) + 3 - 2 * XCHAL_IVPN_SIMD_WIDTH);
        IVP_LAV2NX8U_XP(vtail0_0, a_load, rsrc,
                        2 * (width - j) + 3 - 4 * XCHAL_IVPN_SIMD_WIDTH);
        rsrc = OFFSET_PTR_2NX8U(rsrc, 1, sstep, 0);
        IVP_DSEL2NX8I(vsel1_0, vsel0_0, vsel1_0, vsel0_0,
                      IVP_DSELI_8B_DEINTERLEAVE_1);
        IVP_DSEL2NX8I(vtail1_0, vtail0_0, 0, vtail0_0,
                      IVP_DSELI_8B_DEINTERLEAVE_1);
        IVP_DSEL2NX8I(vsel4_0, vsel2_0, vtail0_0, vsel0_0,
                      IVP_DSELI_8B_ROTATE_RIGHT_2_1);
        vsel3_0 = IVP_SEL2NX8I(vtail1_0, vsel1_0, IVP_SELI_8B_ROTATE_RIGHT_1);
        xb_vec2Nx24 w_0 = IVP_MULUU2NX8(4, vsel1_0);
        IVP_MULUUA2NX8(w_0, 6, vsel2_0);
        IVP_MULUUA2NX8(w_0, 4, vsel3_0);
        IVP_ADDWUA2NX8(w_0, vsel0_0, vsel4_0);
        pp4_0 = IVP_CVT16U2NX24L(w_0);
        pp4_1 = IVP_CVT16U2NX24H(w_0);
      }

      xb_vec2Nx24 w0 = IVP_MULUSI2NR8X16(1, pp0_0, pp0_0);
      IVP_MULUSAI2NX8X16(w0, 4, pp1_0, pp1_0);
      IVP_MULUSAI2NX8X16(w0, 6, pp2_0, pp2_0);
      IVP_MULUSAI2NX8X16(w0, 4, pp3_0, pp3_0);
      IVP_MULUSAI2NR8X16(w0, 1, pp4_0, pp4_0);

      xb_vec2Nx24 w1 = IVP_MULUSI2NR8X16(1, pp0_1, pp0_1);
      IVP_MULUSAI2NX8X16(w1, 4, pp1_1, pp1_1);
      IVP_MULUSAI2NX8X16(w1, 6, pp2_1, pp2_1);
      IVP_MULUSAI2NX8X16(w1, 4, pp3_1, pp3_1);
      IVP_MULUSAI2NR8X16(w1, 1, pp4_1, pp4_1);

      IVP_SAVNX8U_XP(IVP_PACKVRU2NX24_0(w0, 8), a_store, rdst, width - j);
      IVP_SAVNX8U_XP(IVP_PACKVRU2NX24_0(w1, 8), a_store, rdst,
                     width - j - XCHAL_IVPN_SIMD_WIDTH);
      IVP_SAVNX8UPOS_FP(a_store, rdst);
    }
  }

  if (j < width) {
    xb_vecNx16 pp0, pp1, pp2, pp3, pp4, pp5, pp6, pp7;
    rsrc = OFFSET_PTR_2NX8U(psrc, -2, XI_TILE_GET_PITCH(src), 0);
    rdst = pdst;

    int sstep = XI_TILE_GET_PITCH(src) -
                XT_MIN(4 * XCHAL_IVPN_SIMD_WIDTH, 2 * (width - j) + 3);
    int dstep =
        XI_ARRAY_GET_PITCH(dst) - XT_MIN(XCHAL_IVPN_SIMD_WIDTH_, width - j);

    LOAD_ROW_1x8x2_x5(pp0, pp1);
    LOAD_ROW_1x8x2_x5(pp2, pp3);

    int i = 0;
    for (; i < height - 1; i += 2) {
      LOAD_ROW_1x8x2_x5(pp4, pp5);
      LOAD_ROW_1x8x2_x5(pp6, pp7);

      xb_vecNx16 s14_0 = IVP_ADDNX16(IVP_SLLINX16(IVP_ADDNX16(pp1, pp3), 2),
                                     IVP_ADDNX16(pp0, pp4));
      xb_vecNx16 s14_1 = IVP_ADDNX16(IVP_SLLINX16(IVP_ADDNX16(pp3, pp5), 2),
                                     IVP_ADDNX16(pp2, pp6));

      IVP_DSELNX16I(s14_1, s14_0, s14_1, s14_0, IVP_DSELI_INTERLEAVE_1);

      xb_vec2Nx24 w = IVP_CVT24U2NX16(s14_1, s14_0);
      IVP_MULUSAI2NX8X16(w, 6, pp4, pp2);

      IVP_SAVNX8U_XP(IVP_PACKVRU2NX24_0(w, 8), a_store, rdst, width - j);
      IVP_SAVNX8UPOS_FP(a_store, rdst);
      rdst = OFFSET_PTR_NX8U(rdst, 1, dstep, 0);

      IVP_SAVNX8U_XP(IVP_PACKVRU2NX24_1(w, 8), a_store, rdst, width - j);
      IVP_SAVNX8UPOS_FP(a_store, rdst);
      rdst = OFFSET_PTR_NX8U(rdst, 1, dstep, 0);

      pp0 = pp4;
      pp1 = pp5;
      pp2 = pp6;
      pp3 = pp7;
    }
    if (i < height) {
      {
        xb_vec2Nx8U vsel0_0, vsel1_0, vsel2_0, vsel3_0, vsel4_0, vtail_0;
        valign a_load = IVP_LA2NX8U_PP(rsrc);
        IVP_LAV2NX8U_XP(vsel0_0, a_load, rsrc, 2 * (width - j) + 3);
        IVP_LAV2NX8U_XP(vtail_0, a_load, rsrc,
                        2 * (width - j) + 3 - 2 * XCHAL_IVPN_SIMD_WIDTH);
        rsrc = OFFSET_PTR_2NX8U(rsrc, 1, sstep, 0);
        IVP_DSEL2NX8I(vsel1_0, vsel0_0, vtail_0, vsel0_0,
                      IVP_DSELI_8B_DEINTERLEAVE_1);
        IVP_DSEL2NX8I(vsel4_0, vsel2_0, 0, vsel0_0,
                      IVP_DSELI_8B_ROTATE_RIGHT_2_1);
        vsel3_0 = IVP_SEL2NX8I(0, vsel1_0, IVP_SELI_8B_ROTATE_RIGHT_1);
        xb_vec2Nx24 w_0 = IVP_MULUU2NX8(4, vsel1_0);
        IVP_MULUUA2NX8(w_0, 6, vsel2_0);
        IVP_MULUUA2NX8(w_0, 4, vsel3_0);
        IVP_ADDWUA2NX8(w_0, vsel0_0, vsel4_0);
        pp4 = IVP_CVT16U2NX24L(w_0);
      }

      xb_vec2Nx24 w = IVP_MULUSI2NR8X16(1, pp0, pp0);
      IVP_MULUSAI2NX8X16(w, 4, pp1, pp1);
      IVP_MULUSAI2NX8X16(w, 6, pp2, pp2);
      IVP_MULUSAI2NX8X16(w, 4, pp3, pp3);
      IVP_MULUSAI2NR8X16(w, 1, pp4, pp4);

      IVP_SAVNX8U_XP(IVP_PACKVRU2NX24_0(w, 8), a_store, rdst, width - j);
      IVP_SAVNX8UPOS_FP(a_store, rdst);
    }
  }

#undef LOAD_ROW_2x8x2_x5
#undef LOAD_ROW_1x8x2_x5
#endif // VisionP and not VisionP6
#else  // Not VisionP

#endif // Not VisionP

  return 0;
}
