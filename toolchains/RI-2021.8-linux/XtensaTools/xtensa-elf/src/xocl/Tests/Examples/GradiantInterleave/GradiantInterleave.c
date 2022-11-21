#include "xi_lib.h"

#include <xtensa/tie/xt_ivpn.h>
#include <xtensa/tie/xt_misc.h>

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

#define IVP_SAVNX16POS_FP IVP_SAPOSNX16_FP

#define XI_TO_U15(val) ((uint16_t)((val) * (1 << 15) + 0.5))

#define XCHAL_IVPN_SIMD_WIDTH_ XCHAL_IVPN_SIMD_WIDTH

XI_ERR_TYPE xiHOGGradient_interleave_U8S16(const xi_pTile src,
                                           xi_pArray dst_dx_dy) {

  int width = XI_TILE_GET_WIDTH(src);
  int height = XI_TILE_GET_HEIGHT(src);

  xb_vecNx8U *restrict psrc =
      OFFSET_PTR_NX8U(XI_TILE_GET_DATA_PTR(src), -1, XI_TILE_GET_PITCH(src), 0);
  xb_vecNx16 *restrict pdst_dx_dy =
      (xb_vecNx16 *)(XI_ARRAY_GET_DATA_PTR(dst_dx_dy));
  xb_vecNx8U *restrict rsrc;
  xb_vecNx16 *restrict rdst_dx_dy;

  valign a_store_dx_dy = IVP_ZALIGN();

#if XCHAL_HAVE_VISION
  xb_vec2Nx8 rotate1 = IVP_MOV2NX8_FROMNX16(IVP_ADDNX16(IVP_SEQNX16(), 1));
  xb_vec2Nx8 rotate2 = IVP_MOV2NX8_FROMNX16(IVP_ADDNX16(IVP_SEQNX16(), 2));
  xb_vec2Nx8 rotate =
      IVP_SEL2NX8I(rotate2, rotate1, IVP_SELI_8B_INTERLEAVE_1_EVEN);
#else
  vsaN rotate1 = IVP_MOVVSV(IVP_ADDNX16(IVP_SEQNX16(), 1), 0);
  vsaN rotate2 = IVP_MOVVSV(IVP_ADDNX16(IVP_SEQNX16(), 2), 0);
#endif

  int sstride = XI_TILE_GET_PITCH(src);
  int xystride = XI_ARRAY_GET_PITCH(dst_dx_dy);

  int x = 0;
  for (; x < width - 3 * XCHAL_IVPN_SIMD_WIDTH;
       x += 4 * XCHAL_IVPN_SIMD_WIDTH, psrc += 4, pdst_dx_dy += 8) {
    xb_vecNx16U pp0_0, pp0_1, pp0_2, pp0_3;
    xb_vecNx16 pp1_0, pp1_1, pp1_2, pp1_3;
    xb_vecNx16 pp2_0, pp2_1, pp2_2, pp2_3;

    xb_vecNx16 qq1_0, qq1_1, qq1_2, qq1_3;

    int offs = sstride - XT_MIN(5 * XCHAL_IVPN_SIMD_WIDTH_, width - x + 2);
    int offxy =
        xystride - XT_MIN(8 * XCHAL_IVPN_SIMD_WIDTH_, (width - x + 0) * 2);

    rsrc = psrc;
    rdst_dx_dy = pdst_dx_dy;

    { // row 0
      valign a_load = IVP_LANX8U_PP(rsrc);

      IVP_LANX8U_IP(pp0_0, a_load, rsrc);
      IVP_LANX8U_IP(pp0_1, a_load, rsrc);
      IVP_LANX8U_IP(pp0_2, a_load, rsrc);
      IVP_LAVNX8U_XP(pp0_3, a_load, rsrc,
                     (width - x) - 3 * XCHAL_IVPN_SIMD_WIDTH);

      rsrc = OFFSET_PTR_NX8U(psrc, 1, sstride, -1);
    }

    { // row 1
      xb_vecNx16U vsel0_0, vsel0_1, vsel0_2, vsel0_3, vsel0_4;
      xb_vecNx16 vsel2_0, vsel2_1, vsel2_2, vsel2_3;
      valign a_load = IVP_LANX8U_PP(rsrc);

      IVP_LANX8U_IP(vsel0_0, a_load, rsrc);
      IVP_LANX8U_IP(vsel0_1, a_load, rsrc);
      IVP_LANX8U_IP(vsel0_2, a_load, rsrc);
      IVP_LAVNX8U_XP(vsel0_3, a_load, rsrc,
                     (width - x + 2) - 3 * XCHAL_IVPN_SIMD_WIDTH);
      IVP_LAVNX8U_XP(vsel0_4, a_load, rsrc,
                     (width - x + 2) - 4 * XCHAL_IVPN_SIMD_WIDTH);

      rsrc = OFFSET_PTR_NX8U(psrc, 2, sstride, -1);

#if XCHAL_HAVE_VISION
      IVP_DSELNX16U(vsel2_0, pp1_0, vsel0_1, vsel0_0, rotate);
      IVP_DSELNX16U(vsel2_1, pp1_1, vsel0_2, vsel0_1, rotate);
      IVP_DSELNX16U(vsel2_2, pp1_2, vsel0_3, vsel0_2, rotate);
      IVP_DSELNX16U(vsel2_3, pp1_3, vsel0_4, vsel0_3, rotate);
#else
      IVP_DSELNX16(vsel2_0, pp1_0, vsel0_1, vsel0_0, rotate2, rotate1);
      IVP_DSELNX16(vsel2_1, pp1_1, vsel0_2, vsel0_1, rotate2, rotate1);
      IVP_DSELNX16(vsel2_2, pp1_2, vsel0_3, vsel0_2, rotate2, rotate1);
      IVP_DSELNX16(vsel2_3, pp1_3, vsel0_4, vsel0_3, rotate2, rotate1);
#endif
      qq1_0 = IVP_SUBNX16(vsel2_0, vsel0_0);
      qq1_1 = IVP_SUBNX16(vsel2_1, vsel0_1);
      qq1_2 = IVP_SUBNX16(vsel2_2, vsel0_2);
      qq1_3 = IVP_SUBNX16(vsel2_3, vsel0_3);
    }

    for (int y = 0; y < height; y++) {
      xb_vecNx16U vsel0_0, vsel0_1, vsel0_2, vsel0_3, vsel0_4;
      xb_vecNx16 vsel2_0, vsel2_1, vsel2_2, vsel2_3;
      valign a_load = IVP_LANX8U_PP(rsrc);

      IVP_LANX8U_IP(vsel0_0, a_load, rsrc);
      IVP_LANX8U_IP(vsel0_1, a_load, rsrc);
      IVP_LANX8U_IP(vsel0_2, a_load, rsrc);
      IVP_LAVNX8U_XP(vsel0_3, a_load, rsrc,
                     (width - x + 2) - 3 * XCHAL_IVPN_SIMD_WIDTH);
      IVP_LAVNX8U_XP(vsel0_4, a_load, rsrc,
                     (width - x + 2) - 4 * XCHAL_IVPN_SIMD_WIDTH);

      rsrc = OFFSET_PTR_NX8U(rsrc, 1, offs, 0);

#if XCHAL_HAVE_VISION
      IVP_DSELNX16U(vsel2_0, pp2_0, vsel0_1, vsel0_0, rotate);
      IVP_DSELNX16U(vsel2_1, pp2_1, vsel0_2, vsel0_1, rotate);
      IVP_DSELNX16U(vsel2_2, pp2_2, vsel0_3, vsel0_2, rotate);
      IVP_DSELNX16U(vsel2_3, pp2_3, vsel0_4, vsel0_3, rotate);
#else
      IVP_DSELNX16(vsel2_0, pp2_0, vsel0_1, vsel0_0, rotate2, rotate1);
      IVP_DSELNX16(vsel2_1, pp2_1, vsel0_2, vsel0_1, rotate2, rotate1);
      IVP_DSELNX16(vsel2_2, pp2_2, vsel0_3, vsel0_2, rotate2, rotate1);
      IVP_DSELNX16(vsel2_3, pp2_3, vsel0_4, vsel0_3, rotate2, rotate1);
#endif
      xb_vecNx16 dx_0 = qq1_0;
      xb_vecNx16 dx_1 = qq1_1;
      xb_vecNx16 dx_2 = qq1_2;
      xb_vecNx16 dx_3 = qq1_3;

      xb_vecNx16 dy_0 = IVP_SUBNX16(pp2_0, pp0_0);
      xb_vecNx16 dy_1 = IVP_SUBNX16(pp2_1, pp0_1);
      xb_vecNx16 dy_2 = IVP_SUBNX16(pp2_2, pp0_2);
      xb_vecNx16 dy_3 = IVP_SUBNX16(pp2_3, pp0_3);

      xb_vecNx16 dxy0_0, dxy1_0, dxy0_1, dxy1_1, dxy0_2, dxy1_2, dxy0_3, dxy1_3;

      IVP_DSELNX16UI(dxy1_0, dxy0_0, dy_0, dx_0, IVP_DSELI_INTERLEAVE_1);
      IVP_DSELNX16UI(dxy1_1, dxy0_1, dy_1, dx_1, IVP_DSELI_INTERLEAVE_1);
      IVP_DSELNX16UI(dxy1_2, dxy0_2, dy_2, dx_2, IVP_DSELI_INTERLEAVE_1);
      IVP_DSELNX16UI(dxy1_3, dxy0_3, dy_3, dx_3, IVP_DSELI_INTERLEAVE_1);

      IVP_SANX16_IP(dxy0_0, a_store_dx_dy, rdst_dx_dy);
      IVP_SANX16_IP(dxy1_0, a_store_dx_dy, rdst_dx_dy);
      IVP_SANX16_IP(dxy0_1, a_store_dx_dy, rdst_dx_dy);
      IVP_SANX16_IP(dxy1_1, a_store_dx_dy, rdst_dx_dy);
      IVP_SANX16_IP(dxy0_2, a_store_dx_dy, rdst_dx_dy);
      IVP_SANX16_IP(dxy1_2, a_store_dx_dy, rdst_dx_dy);
      IVP_SAVNX16_XP(dxy0_3, a_store_dx_dy, rdst_dx_dy,
                     2 * 2 * (width - x - 3 * XCHAL_IVPN_SIMD_WIDTH));
      IVP_SAVNX16_XP(dxy1_3, a_store_dx_dy, rdst_dx_dy,
                     ((2 * (width - x - 3 * XCHAL_IVPN_SIMD_WIDTH)) -
                      XCHAL_IVPN_SIMD_WIDTH) *
                         2);
      IVP_SAVNX16POS_FP(a_store_dx_dy, rdst_dx_dy);

      rdst_dx_dy = OFFSET_PTR_NX16(rdst_dx_dy, 1, offxy, 0);

      pp0_0 = pp1_0;
      pp0_1 = pp1_1;
      pp0_2 = pp1_2;
      pp0_3 = pp1_3;
      pp1_0 = pp2_0;
      pp1_1 = pp2_1;
      pp1_2 = pp2_2;
      pp1_3 = pp2_3;

      qq1_0 = IVP_SUBNX16(vsel2_0, vsel0_0);
      qq1_1 = IVP_SUBNX16(vsel2_1, vsel0_1);
      qq1_2 = IVP_SUBNX16(vsel2_2, vsel0_2);
      qq1_3 = IVP_SUBNX16(vsel2_3, vsel0_3);
    }
  }

  if (x < width - 2 * XCHAL_IVPN_SIMD_WIDTH) {
    xb_vecNx16U pp0_0, pp0_1, pp0_2;
    xb_vecNx16 pp1_0, pp1_1, pp1_2;
    xb_vecNx16 pp2_0, pp2_1, pp2_2;

    xb_vecNx16 qq1_0, qq1_1, qq1_2;

    rsrc = psrc;
    rdst_dx_dy = pdst_dx_dy;

    { // row 0
      valign a_load = IVP_LANX8U_PP(rsrc);

      IVP_LANX8U_IP(pp0_0, a_load, rsrc);
      IVP_LANX8U_IP(pp0_1, a_load, rsrc);
      IVP_LAVNX8U_XP(pp0_2, a_load, rsrc,
                     (width - x) - 2 * XCHAL_IVPN_SIMD_WIDTH);

      rsrc = OFFSET_PTR_NX8U(psrc, 1, sstride, -1);
    }

    { // row 1
      xb_vecNx16U vsel0_0, vsel0_1, vsel0_2, vsel0_3;
      xb_vecNx16 vsel2_0, vsel2_1, vsel2_2;
      valign a_load = IVP_LANX8U_PP(rsrc);

      IVP_LANX8U_IP(vsel0_0, a_load, rsrc);
      IVP_LANX8U_IP(vsel0_1, a_load, rsrc);
      IVP_LAVNX8U_XP(vsel0_2, a_load, rsrc,
                     (width - x + 2) - 2 * XCHAL_IVPN_SIMD_WIDTH);
      IVP_LAVNX8U_XP(vsel0_3, a_load, rsrc,
                     (width - x + 2) - 3 * XCHAL_IVPN_SIMD_WIDTH);

      rsrc = OFFSET_PTR_NX8U(psrc, 2, sstride, -1);

#if XCHAL_HAVE_VISION
      IVP_DSELNX16U(vsel2_0, pp1_0, vsel0_1, vsel0_0, rotate);
      IVP_DSELNX16U(vsel2_1, pp1_1, vsel0_2, vsel0_1, rotate);
      IVP_DSELNX16U(vsel2_2, pp1_2, vsel0_3, vsel0_2, rotate);
#else
      IVP_DSELNX16(vsel2_0, pp1_0, vsel0_1, vsel0_0, rotate2, rotate1);
      IVP_DSELNX16(vsel2_1, pp1_1, vsel0_2, vsel0_1, rotate2, rotate1);
      IVP_DSELNX16(vsel2_2, pp1_2, vsel0_3, vsel0_2, rotate2, rotate1);
#endif
      qq1_0 = IVP_SUBNX16(vsel2_0, vsel0_0);
      qq1_1 = IVP_SUBNX16(vsel2_1, vsel0_1);
      qq1_2 = IVP_SUBNX16(vsel2_2, vsel0_2);
    }

    for (int y = 0; y < height; y++) {
      xb_vecNx16U vsel0_0, vsel0_1, vsel0_2, vsel0_3;
      xb_vecNx16 vsel2_0, vsel2_1, vsel2_2;
      valign a_load = IVP_LANX8U_PP(rsrc);

      IVP_LANX8U_IP(vsel0_0, a_load, rsrc);
      IVP_LANX8U_IP(vsel0_1, a_load, rsrc);
      IVP_LAVNX8U_XP(vsel0_2, a_load, rsrc,
                     (width - x + 2) - 2 * XCHAL_IVPN_SIMD_WIDTH);
      IVP_LAVNX8U_XP(vsel0_3, a_load, rsrc,
                     (width - x + 2) - 3 * XCHAL_IVPN_SIMD_WIDTH);

      rsrc = OFFSET_PTR_NX8U(psrc, 2 + y + 1, sstride, -1);

#if XCHAL_HAVE_VISION
      IVP_DSELNX16U(vsel2_0, pp2_0, vsel0_1, vsel0_0, rotate);
      IVP_DSELNX16U(vsel2_1, pp2_1, vsel0_2, vsel0_1, rotate);
      IVP_DSELNX16U(vsel2_2, pp2_2, vsel0_3, vsel0_2, rotate);
#else
      IVP_DSELNX16(vsel2_0, pp2_0, vsel0_1, vsel0_0, rotate2, rotate1);
      IVP_DSELNX16(vsel2_1, pp2_1, vsel0_2, vsel0_1, rotate2, rotate1);
      IVP_DSELNX16(vsel2_2, pp2_2, vsel0_3, vsel0_2, rotate2, rotate1);
#endif
      xb_vecNx16 dx_0 = qq1_0;
      xb_vecNx16 dx_1 = qq1_1;
      xb_vecNx16 dx_2 = qq1_2;

      xb_vecNx16 dy_0 = IVP_SUBNX16(pp2_0, pp0_0);
      xb_vecNx16 dy_1 = IVP_SUBNX16(pp2_1, pp0_1);
      xb_vecNx16 dy_2 = IVP_SUBNX16(pp2_2, pp0_2);

      xb_vecNx16 dxy0_0, dxy1_0, dxy0_1, dxy1_1, dxy0_2, dxy1_2;

      IVP_DSELNX16UI(dxy1_0, dxy0_0, dy_0, dx_0, IVP_DSELI_INTERLEAVE_1);
      IVP_DSELNX16UI(dxy1_1, dxy0_1, dy_1, dx_1, IVP_DSELI_INTERLEAVE_1);
      IVP_DSELNX16UI(dxy1_2, dxy0_2, dy_2, dx_2, IVP_DSELI_INTERLEAVE_1);

      IVP_SANX16_IP(dxy0_0, a_store_dx_dy, rdst_dx_dy);
      IVP_SANX16_IP(dxy1_0, a_store_dx_dy, rdst_dx_dy);
      IVP_SANX16_IP(dxy0_1, a_store_dx_dy, rdst_dx_dy);
      IVP_SANX16_IP(dxy1_1, a_store_dx_dy, rdst_dx_dy);
      IVP_SAVNX16_XP(dxy0_2, a_store_dx_dy, rdst_dx_dy,
                     2 * 2 * (width - x - 2 * XCHAL_IVPN_SIMD_WIDTH));
      IVP_SAVNX16_XP(dxy1_2, a_store_dx_dy, rdst_dx_dy,
                     ((2 * (width - x - 2 * XCHAL_IVPN_SIMD_WIDTH)) -
                      XCHAL_IVPN_SIMD_WIDTH) *
                         2);
      IVP_SAVNX16POS_FP(a_store_dx_dy, rdst_dx_dy);

      rdst_dx_dy = OFFSET_PTR_NX16(pdst_dx_dy, 0 + y + 1, xystride, 0);

      pp0_0 = pp1_0;
      pp0_1 = pp1_1;
      pp0_2 = pp1_2;
      pp1_0 = pp2_0;
      pp1_1 = pp2_1;
      pp1_2 = pp2_2;

      qq1_0 = IVP_SUBNX16(vsel2_0, vsel0_0);
      qq1_1 = IVP_SUBNX16(vsel2_1, vsel0_1);
      qq1_2 = IVP_SUBNX16(vsel2_2, vsel0_2);
    }
  } else if (x < width - XCHAL_IVPN_SIMD_WIDTH) {
    xb_vecNx16U pp0_0, pp0_1;
    xb_vecNx16 pp1_0, pp1_1, pp2_0, pp2_1;
    xb_vecNx16 qq1_0, qq1_1;

    rsrc = psrc;
    rdst_dx_dy = pdst_dx_dy;

    { // row 0
      valign a_load = IVP_LANX8U_PP(rsrc);

      IVP_LANX8U_IP(pp0_0, a_load, rsrc);
      IVP_LAVNX8U_XP(pp0_1, a_load, rsrc,
                     (width - x) - 1 * XCHAL_IVPN_SIMD_WIDTH);

      rsrc = OFFSET_PTR_NX8U(psrc, 1, sstride, -1);
    }

    { // row 1
      xb_vecNx16U vsel0_0, vsel0_1, vsel0_2;
      xb_vecNx16 vsel2_0, vsel2_1;
      valign a_load = IVP_LANX8U_PP(rsrc);

      IVP_LANX8U_IP(vsel0_0, a_load, rsrc);
      IVP_LAVNX8U_XP(vsel0_1, a_load, rsrc,
                     (width - x + 2) - 1 * XCHAL_IVPN_SIMD_WIDTH);
      IVP_LAVNX8U_XP(vsel0_2, a_load, rsrc,
                     (width - x + 2) - 2 * XCHAL_IVPN_SIMD_WIDTH);

      rsrc = OFFSET_PTR_NX8U(psrc, 2, sstride, -1);

#if XCHAL_HAVE_VISION
      IVP_DSELNX16U(vsel2_0, pp1_0, vsel0_1, vsel0_0, rotate);
      IVP_DSELNX16U(vsel2_1, pp1_1, vsel0_2, vsel0_1, rotate);
#else
      IVP_DSELNX16(vsel2_0, pp1_0, vsel0_1, vsel0_0, rotate2, rotate1);
      IVP_DSELNX16(vsel2_1, pp1_1, vsel0_2, vsel0_1, rotate2, rotate1);
#endif
      qq1_0 = IVP_SUBNX16(vsel2_0, vsel0_0);
      qq1_1 = IVP_SUBNX16(vsel2_1, vsel0_1);
    }

    for (int y = 0; y < height; y++) {
      xb_vecNx16U vsel0_0, vsel0_1, vsel0_2;
      xb_vecNx16 vsel2_0, vsel2_1;
      valign a_load = IVP_LANX8U_PP(rsrc);

      IVP_LANX8U_IP(vsel0_0, a_load, rsrc);
      IVP_LAVNX8U_XP(vsel0_1, a_load, rsrc,
                     (width - x + 2) - 1 * XCHAL_IVPN_SIMD_WIDTH);
      IVP_LAVNX8U_XP(vsel0_2, a_load, rsrc,
                     (width - x + 2) - 2 * XCHAL_IVPN_SIMD_WIDTH);

      rsrc = OFFSET_PTR_NX8U(psrc, 2 + y + 1, sstride, -1);

#if XCHAL_HAVE_VISION
      IVP_DSELNX16U(vsel2_0, pp2_0, vsel0_1, vsel0_0, rotate);
      IVP_DSELNX16U(vsel2_1, pp2_1, vsel0_2, vsel0_1, rotate);
#else
      IVP_DSELNX16(vsel2_0, pp2_0, vsel0_1, vsel0_0, rotate2, rotate1);
      IVP_DSELNX16(vsel2_1, pp2_1, vsel0_2, vsel0_1, rotate2, rotate1);
#endif
      xb_vecNx16 dx_0 = qq1_0;
      xb_vecNx16 dx_1 = qq1_1;

      xb_vecNx16 dy_0 = IVP_SUBNX16(pp2_0, pp0_0);
      xb_vecNx16 dy_1 = IVP_SUBNX16(pp2_1, pp0_1);

      xb_vecNx16 dxy0_0, dxy1_0, dxy0_1, dxy1_1;

      IVP_DSELNX16UI(dxy1_0, dxy0_0, dy_0, dx_0, IVP_DSELI_INTERLEAVE_1);
      IVP_DSELNX16UI(dxy1_1, dxy0_1, dy_1, dx_1, IVP_DSELI_INTERLEAVE_1);

      IVP_SANX16_IP(dxy0_0, a_store_dx_dy, rdst_dx_dy);
      IVP_SANX16_IP(dxy1_0, a_store_dx_dy, rdst_dx_dy);
      IVP_SAVNX16_XP(dxy0_1, a_store_dx_dy, rdst_dx_dy,
                     2 * 2 * (width - x - 1 * XCHAL_IVPN_SIMD_WIDTH));
      IVP_SAVNX16_XP(dxy1_1, a_store_dx_dy, rdst_dx_dy,
                     ((2 * (width - x - 1 * XCHAL_IVPN_SIMD_WIDTH)) -
                      XCHAL_IVPN_SIMD_WIDTH) *
                         2);
      IVP_SAVNX16POS_FP(a_store_dx_dy, rdst_dx_dy);

      rdst_dx_dy = OFFSET_PTR_NX16(pdst_dx_dy, 0 + y + 1, xystride, 0);

      pp0_0 = pp1_0;
      pp0_1 = pp1_1;
      pp1_0 = pp2_0;
      pp1_1 = pp2_1;

      qq1_0 = IVP_SUBNX16(vsel2_0, vsel0_0);
      qq1_1 = IVP_SUBNX16(vsel2_1, vsel0_1);
    }
  } else if (x < width) {
    xb_vecNx16U pp0;
    xb_vecNx16 qq1, pp1, pp2;

    rsrc = psrc;

    { // row 0
      valign a_load = IVP_LANX8U_PP(rsrc);

      IVP_LAVNX8U_XP(pp0, a_load, rsrc, width - x);

      rsrc = OFFSET_PTR_NX8U(psrc, 1, sstride, -1);
    }

    rdst_dx_dy = (xb_vecNx16 *)(pdst_dx_dy);

    { // row 1
      xb_vecNx16U vsel0, vtail;
      xb_vecNx16 vsel2;
      valign a_load = IVP_LANX8U_PP(rsrc);

      IVP_LAVNX8U_XP(vsel0, a_load, rsrc, width - x + 2);
      IVP_LAVNX8U_XP(vtail, a_load, rsrc,
                     width - x + 2 - XCHAL_IVPN_SIMD_WIDTH);

      rsrc = OFFSET_PTR_NX8U(psrc, 2, sstride, -1);

#if XCHAL_HAVE_VISION
      IVP_DSELNX16U(vsel2, pp1, vtail, vsel0, rotate);
#else
      IVP_DSELNX16(vsel2, pp1, vtail, vsel0, rotate2, rotate1);
#endif
      qq1 = IVP_SUBNX16(vsel2, vsel0);
    }

    for (int y = 0; y < height; y++) {
      xb_vecNx16U vsel0, vtail;
      xb_vecNx16 vsel2;
      valign a_load = IVP_LANX8U_PP(rsrc);

      IVP_LAVNX8U_XP(vsel0, a_load, rsrc, width - x + 2);
      IVP_LAVNX8U_XP(vtail, a_load, rsrc,
                     width - x + 2 - XCHAL_IVPN_SIMD_WIDTH);

      rsrc = OFFSET_PTR_NX8U(psrc, 2 + y + 1, sstride, -1);

#if XCHAL_HAVE_VISION
      IVP_DSELNX16U(vsel2, pp2, vtail, vsel0, rotate);
#else
      IVP_DSELNX16(vsel2, pp2, vtail, vsel0, rotate2, rotate1);
#endif
      xb_vecNx16 dx = qq1;

      xb_vecNx16 dy = IVP_SUBNX16(pp2, pp0);

      xb_vecNx16 dxy0, dxy1;

      IVP_DSELNX16UI(dxy1, dxy0, dy, dx, IVP_DSELI_INTERLEAVE_1);

      IVP_SAVNX16_XP(dxy0, a_store_dx_dy, rdst_dx_dy, 2 * 2 * (width - x));
      IVP_SAVNX16_XP(dxy1, a_store_dx_dy, rdst_dx_dy,
                     ((2 * (width - x)) - XCHAL_IVPN_SIMD_WIDTH) * 2);
      IVP_SAVNX16POS_FP(a_store_dx_dy, rdst_dx_dy);

      rdst_dx_dy = OFFSET_PTR_NX16(pdst_dx_dy, 0 + y + 1, xystride, 0);

      pp0 = pp1;
      pp1 = pp2;
      qq1 = IVP_SUBNX16(vsel2, vsel0);
    }
  }

  return 0;
}
