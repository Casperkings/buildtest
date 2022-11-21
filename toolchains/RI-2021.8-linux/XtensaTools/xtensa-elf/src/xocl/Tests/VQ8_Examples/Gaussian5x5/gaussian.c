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

XI_ERR_TYPE xiGaussianBlur_5x5_U8(const xi_pTile src, xi_pArray dst) {

  int sstride = XI_TILE_GET_PITCH(src);
  int dstride = XI_TILE_GET_PITCH(dst);
  int width = XI_TILE_GET_WIDTH(src);
  int height = XI_TILE_GET_HEIGHT(src);

  xb_vec2Nx8U *restrict psrc =
      OFFSET_PTR_2NX8U(XI_TILE_GET_DATA_PTR(src), 0, 0, -2);
  xb_vec2Nx8U *restrict pdst = (xb_vec2Nx8U *)XI_TILE_GET_DATA_PTR(dst);
  xb_vec2Nx8U *restrict rdst;
  xb_vec2Nx8U *restrict rsrc;
  valign a_load, a_store = IVP_ZALIGN();

#if XCHAL_HAVE_VISION
#if XCHAL_VISION_TYPE == 6
#define LOAD_ROW_1x8U_x5(reg)                                                  \
  {                                                                            \
    xb_vec2Nx8 vsel0, vtail;                                                   \
    a_load = IVP_LA2NX8U_PP(rsrc);                                             \
    IVP_LAV2NX8_XP(vsel0, a_load, rsrc, width - j + 4);                        \
    IVP_LAV2NX8_XP(vtail, a_load, rsrc,                                        \
                   width - j + 4 - 2 * XCHAL_IVPN_SIMD_WIDTH);                 \
    xb_vec2Nx24 w1 = IVP_MULUS4T2N8XR8(vtail, vsel0, 0x04060401);              \
    IVP_ADDWUA2NX8(w1, 0,                                                      \
                   IVP_SEL2NX8I(vtail, vsel0, IVP_SELI_8B_ROTATE_RIGHT_4));    \
    reg##_0 = IVP_PACKL2NX24_0(w1);                                            \
    reg##_1 = IVP_PACKL2NX24_1(w1);                                            \
  }
#else
#define LOAD_ROW_1x8U_x5(reg)                                                  \
  {                                                                            \
    xb_vec2Nx8 vsel0, vsel1, vsel2, vsel3, vsel4, vtail;                       \
    a_load = IVP_LA2NX8U_PP(rsrc);                                             \
    IVP_LAV2NX8_XP(vsel0, a_load, rsrc, width - j + 4);                        \
    IVP_LAV2NX8_XP(vtail, a_load, rsrc,                                        \
                   width - j + 4 - 2 * XCHAL_IVPN_SIMD_WIDTH);                 \
    IVP_DSEL2NX8I(vsel2, vsel1, vtail, vsel0, IVP_DSELI_8B_ROTATE_RIGHT_2_1);  \
    IVP_DSEL2NX8I(vsel4, vsel3, vtail, vsel0, IVP_DSELI_8B_ROTATE_RIGHT_4_3);  \
    xb_vec2Nx24 w = IVP_ADDWU2NX8(vsel0, vsel4);                               \
    IVP_MULUUA2NX8(w, 4, vsel1);                                               \
    IVP_MULUUA2NX8(w, 6, vsel2);                                               \
    IVP_MULUUA2NX8(w, 4, vsel3);                                               \
    reg##_0 = IVP_PACKL2NX24_0(w);                                             \
    reg##_1 = IVP_PACKL2NX24_1(w);                                             \
  }
#endif
#else
#define LOAD_ROW_1x8U_x5(reg)                                                  \
  {                                                                            \
    xb_vec2Nx8 vsel0, vsel1, vsel2, vsel3, vsel4, vtail;                       \
    a_load = IVP_LA2NX8U_PP(rsrc);                                             \
    IVP_LAV2NX8_XP(vsel0, a_load, rsrc, width - j + 4);                        \
    IVP_LAV2NX8_XP(vtail, a_load, rsrc,                                        \
                   width - j + 4 - 2 * XCHAL_IVPN_SIMD_WIDTH);                 \
    IVP_DSEL2NX8I(vsel2, vsel1, vtail, vsel0, IVP_DSELI_8B_ROTATE_RIGHT_2_1);  \
    IVP_DSEL2NX8I(vsel4, vsel3, vtail, vsel0, IVP_DSELI_8B_ROTATE_RIGHT_4_3);  \
    xb_vec2Nx24 w = IVP_MULUU2NX8(1, vsel0);                                   \
    IVP_MULUUA2NX8(w, 4, vsel1);                                               \
    IVP_MULUUA2NX8(w, 6, vsel2);                                               \
    IVP_MULUUA2NX8(w, 4, vsel3);                                               \
    IVP_MULUUA2NX8(w, 1, vsel4);                                               \
    reg##_0 = IVP_PACKL2NX24_0(w);                                             \
    reg##_1 = IVP_PACKL2NX24_1(w);                                             \
  }
#endif

  for (int j = 0; j < width;
       j += 2 * XCHAL_IVPN_SIMD_WIDTH, psrc += 1, pdst += 1) {
    xb_vecNx16 pp0_0, pp1_0, pp2_0, pp3_0, pp4_0;
    xb_vecNx16 pp0_1, pp1_1, pp2_1, pp3_1, pp4_1;
    // row 0
    rsrc = OFFSET_PTR_2NX8U(psrc, -2, sstride, 0);
    LOAD_ROW_1x8U_x5(pp0);
    // row 1
    rsrc = OFFSET_PTR_2NX8U(psrc, -1, sstride, 0);
    LOAD_ROW_1x8U_x5(pp1);
    // row 2
    rsrc = OFFSET_PTR_2NX8U(psrc, 0, sstride, 0);
    LOAD_ROW_1x8U_x5(pp2);
    // row 3
    rsrc = OFFSET_PTR_2NX8U(psrc, 1, sstride, 0);
    LOAD_ROW_1x8U_x5(pp3);

#if XCHAL_HAVE_VISION && XCHAL_VISION_TYPE == 6
    int dtail = XT_MIN(width - j, 2 * XCHAL_IVPN_SIMD_WIDTH);
    int stail = XT_MIN(width - j + 4, 4 * XCHAL_IVPN_SIMD_WIDTH);
    int s = sstride - stail;
    int d = dstride - dtail;
    rsrc = OFFSET_PTR_2NX8U(psrc, (0 + 2), sstride, 0);
    rdst = OFFSET_PTR_2NX8U(pdst, 0, dstride, 0);
#endif

    for (int i = 0; i < height; i++) {
#if XCHAL_HAVE_VISION
#if XCHAL_VISION_TYPE == 6
      LOAD_ROW_1x8U_x5(pp4);
      rsrc = OFFSET_PTR_2NX8U(rsrc, 1, s, 0);
      pp0_1 = IVP_ADDNX16(pp0_1, pp4_1);
      pp0_0 = IVP_ADDNX16(pp0_0, pp4_0);
      xb_vec2Nx24 w = IVP_MULPI2NR8X16(0x0601, pp2_1, pp2_0, pp0_1, pp0_0);
      IVP_MULUSAI2NX8X16(w, 4, IVP_ADDNX16(pp1_1, pp3_1),
                         IVP_ADDNX16(pp1_0, pp3_0));
      IVP_SAV2NX8U_XP(IVP_PACKVRU2NX24(w, 8), a_store, rdst, (width - j));
      IVP_SAPOS2NX8U_FP(a_store, rdst);
      rdst = OFFSET_PTR_2NX8U(rdst, 1, d, 0);
#else
      rsrc = OFFSET_PTR_2NX8U(psrc, (i + 2), sstride, 0);
      LOAD_ROW_1x8U_x5(pp4);
      pp0_1 = IVP_ADDNX16(pp0_1, pp4_1);
      pp0_0 = IVP_ADDNX16(pp0_0, pp4_0);
      xb_vec2Nx24 w = IVP_MULUSI2NX8X16(1, pp0_1, pp0_0);
      IVP_MULUSAI2NX8X16(w, 6, pp2_1, pp2_0);
      IVP_MULUSAI2NX8X16(w, 4, IVP_ADDNX16(pp1_1, pp3_1),
                         IVP_ADDNX16(pp1_0, pp3_0));
      rdst = OFFSET_PTR_2NX8U(pdst, i, dstride, 0);
      IVP_SAV2NX8U_XP(IVP_PACKVRU2NX24(w, 8), a_store, rdst, (width - j));
      IVP_SAPOS2NX8U_FP(a_store, rdst);
#endif
#else
      rsrc = OFFSET_PTR_2NX8U(psrc, (i + 2), sstride, 0);
      LOAD_ROW_1x8U_x5(pp4);
      xb_vec2Nx24 w = IVP_MULUSI2NX8X16(1, pp0_1, pp0_0);
      IVP_MULUSAI2NX8X16(w, 6, pp2_1, pp2_0);
      IVP_MULUSAI2NX8X16(w, 4, IVP_ADDNX16(pp1_1, pp3_1),
                         IVP_ADDNX16(pp1_0, pp3_0));
      IVP_MULUSAI2NX8X16(w, 1, pp4_1, pp4_0);
      rdst = OFFSET_PTR_2NX8U(pdst, i, dstride, 0);
      IVP_SAV2NX8U_XP(IVP_PACKVRU2NX24(w, 8), a_store, rdst, (width - j));
      IVP_SAPOS2NX8U_FP(a_store, rdst);
#endif
      pp0_0 = pp1_0;
      pp1_0 = pp2_0;
      pp2_0 = pp3_0;
      pp3_0 = pp4_0;
      pp0_1 = pp1_1;
      pp1_1 = pp2_1;
      pp2_1 = pp3_1;
      pp3_1 = pp4_1;
    }
  }

  return 0; // XI_ERROR_STATUS();
}
