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

#include <math.h>

#include <xtensa/tie/xt_ivpn.h>
#include <xtensa/tie/xt_misc.h>

#define IVP_SAVNX8UPOS_FP  IVP_SAPOSNX8U_FP
#define IVP_SAV2NX8UPOS_FP IVP_SAPOS2NX8U_FP
#define Q15_0_7874         25802
#define Q15_0_9278         30402
#define Q15_0_0936         3068
#define Q15_0_2340         7669

#define Q15_0_5000 16384

XI_ERR_TYPE xiCvtColor_U8_YUV2RGB_420_BT709(const xi_pArray y,
                                            const xi_pArray u,
                                            const xi_pArray v, xi_pArray rgb) {

  uint8_t *psrc_y = (uint8_t *)XI_TILE_GET_DATA_PTR(y);
  uint8_t *psrc_u = (uint8_t *)XI_TILE_GET_DATA_PTR(u);
  uint8_t *psrc_v = (uint8_t *)XI_TILE_GET_DATA_PTR(v);

  uint8_t *pdst_rgb = (uint8_t *)XI_TILE_GET_DATA_PTR(rgb);

  int stride_src_y = XI_TILE_GET_PITCH(y);
  int stride_src_u = XI_TILE_GET_PITCH(u);
  int stride_src_v = XI_TILE_GET_PITCH(v);

  int stride_dst_rgb = XI_TILE_GET_PITCH(rgb);

  int width = XI_TILE_GET_WIDTH(y);
  int height = XI_TILE_GET_HEIGHT(y);

  xb_vec2Nx8U *restrict rdst_rgb;

  xb_vec2Nx8U *restrict rsrc_y;
  xb_vecNx8U *restrict rsrc_u;
  xb_vecNx8U *restrict rsrc_v;

  valign a_load;
  valign a_store = IVP_ZALIGN();

  int ix = 0;
  for (; ix < width - XCHAL_IVPN_SIMD_WIDTH; ix += 2 * XCHAL_IVPN_SIMD_WIDTH) {
    rdst_rgb = OFFSET_PTR_2NX8U(pdst_rgb, 0, 0, 3 * ix);
    rsrc_y = OFFSET_PTR_2NX8U(psrc_y, 0, 0, ix);
    rsrc_u = OFFSET_PTR_NX8U(psrc_u, 0, 0, (ix >> 1));
    rsrc_v = OFFSET_PTR_NX8U(psrc_v, 0, 0, (ix >> 1));
    int WX = width - XT_MAX(ix, width - 2 * XCHAL_IVPN_SIMD_WIDTH);

    int shift_rgb = stride_dst_rgb - 3 * WX;
    int shift_y = stride_src_y - WX;
    int shift_u = stride_src_u - WX / 2;
    int shift_v = stride_src_v - WX / 2;

    for (int iy = 0; iy < height; iy += 2) {
      a_load = IVP_LA2NX8U_PP(rsrc_y);
      xb_vec2Nx8U vec_y0;
      IVP_LAV2NX8U_XP(vec_y0, a_load, rsrc_y, WX);
      rsrc_y = OFFSET_PTR_2NX8U(rsrc_y, 1, shift_y, 0);

      a_load = IVP_LA2NX8U_PP(rsrc_y);
      xb_vec2Nx8U vec_y1;
      IVP_LAV2NX8U_XP(vec_y1, a_load, rsrc_y, WX);
      rsrc_y = OFFSET_PTR_2NX8U(rsrc_y, 1, shift_y, 0);

      a_load = IVP_LANX8U_PP(rsrc_u);
      xb_vecNx16 v_u;
      IVP_LAVNX8U_XP(v_u, a_load, rsrc_u, WX / 2);
      a_load = IVP_LANX8U_PP(rsrc_v);
      xb_vecNx16 v_v;
      IVP_LAVNX8U_XP(v_v, a_load, rsrc_v, WX / 2);

      rsrc_u = OFFSET_PTR_NX8U(rsrc_u, 1, IVP_MOVAV16(shift_u), 0);
      rsrc_v = OFFSET_PTR_NX8U(rsrc_v, 1, IVP_MOVAV16(shift_v), 0);
      xb_vecNx16 vec_u_128 = IVP_SUBNX16(v_u, 128);
      xb_vecNx16 vec_v_128 = IVP_SUBNX16(v_v, 128);

      xb_vec2Nx8 vec_v = IVP_SEL2NX8I(IVP_MOV2NX8_FROMNX16(vec_v_128),
                                      IVP_MOV2NX8_FROMNX16(vec_v_128),
                                      IVP_SELI_8B_INTERLEAVE_1_EVEN);
      xb_vec2Nx8 vec_u = IVP_SEL2NX8I(IVP_MOV2NX8_FROMNX16(vec_u_128),
                                      IVP_MOV2NX8_FROMNX16(vec_u_128),
                                      IVP_SELI_8B_INTERLEAVE_1_EVEN);

      xb_vec2Nx24 w_vec_r0 = IVP_MULI2NX8X16(vec_v, Q15_0_7874, Q15_0_7874);
      xb_vec2Nx24 w_vec_r1 = w_vec_r0;

      IVP_MULUSAI2NX8X16(w_vec_r0, vec_y0, Q15_0_5000, Q15_0_5000);
      IVP_MULUSAI2NX8X16(w_vec_r1, vec_y1, Q15_0_5000, Q15_0_5000);
      xb_vec2Nx8U result_r0 = IVP_PACKVRU2NX24(w_vec_r0, 14);
      xb_vec2Nx8U result_r1 = IVP_PACKVRU2NX24(w_vec_r1, 14);

      xb_vec2Nx24 w_vec_b0 = IVP_MULI2NX8X16(vec_u, Q15_0_9278, Q15_0_9278);
      xb_vec2Nx24 w_vec_b1 = w_vec_b0;

      IVP_MULUSAI2NX8X16(w_vec_b0, vec_y0, Q15_0_5000, Q15_0_5000);
      IVP_MULUSAI2NX8X16(w_vec_b1, vec_y1, Q15_0_5000, Q15_0_5000);
      xb_vec2Nx8U result_b0 = IVP_PACKVRU2NX24(w_vec_b0, 14);
      xb_vec2Nx8U result_b1 = IVP_PACKVRU2NX24(w_vec_b1, 14);

      xb_vec2Nx24 w_vec_g0 = IVP_MULI2NX8X16(vec_v, -Q15_0_2340, -Q15_0_2340);
      IVP_MULAI2NX8X16(w_vec_g0, vec_u, -Q15_0_0936, -Q15_0_0936);
      xb_vec2Nx24 w_vec_g1 = w_vec_g0;
      IVP_MULUSAI2NX8X16(w_vec_g0, vec_y0, Q15_0_5000, Q15_0_5000);
      IVP_MULUSAI2NX8X16(w_vec_g1, vec_y1, Q15_0_5000, Q15_0_5000);
      xb_vec2Nx8U result_g0 = IVP_PACKVRU2NX24(w_vec_g0, 14);
      xb_vec2Nx8U result_g1 = IVP_PACKVRU2NX24(w_vec_g1, 14);

      xb_vec2Nx8 tmp1, tmp0, vdst0, vdst1, vdst2;
      IVP_DSEL2NX8I(tmp0, tmp1, result_g0, result_r0,
                    IVP_DSELI_8B_INTERLEAVE_1);
      IVP_DSEL2NX8I(vdst1, vdst0, result_b0, tmp1,
                    IVP_DSELI_8B_INTERLEAVE_C3_STEP_0);
      IVP_DSEL2NX8I_H(vdst1, vdst2, result_b0, tmp0,
                      IVP_DSELI_8B_INTERLEAVE_C3_STEP_1);

      IVP_SA2NX8U_IP(vdst0, a_store, rdst_rgb);
      IVP_SAV2NX8U_XP(vdst1, a_store, rdst_rgb,
                      3 * WX - 2 * XCHAL_IVPN_SIMD_WIDTH);
      IVP_SAV2NX8U_XP(vdst2, a_store, rdst_rgb,
                      3 * WX - 4 * XCHAL_IVPN_SIMD_WIDTH);
      IVP_SAV2NX8UPOS_FP(a_store, rdst_rgb);
      rdst_rgb = OFFSET_PTR_2NX8U(rdst_rgb, 1, shift_rgb, 0);

      IVP_DSEL2NX8I(tmp0, tmp1, result_g1, result_r1,
                    IVP_DSELI_8B_INTERLEAVE_1);
      IVP_DSEL2NX8I(vdst1, vdst0, result_b1, tmp1,
                    IVP_DSELI_8B_INTERLEAVE_C3_STEP_0);
      IVP_DSEL2NX8I_H(vdst1, vdst2, result_b1, tmp0,
                      IVP_DSELI_8B_INTERLEAVE_C3_STEP_1);

      IVP_SA2NX8U_IP(vdst0, a_store, rdst_rgb);
      IVP_SAV2NX8U_XP(vdst1, a_store, rdst_rgb,
                      3 * WX - 2 * XCHAL_IVPN_SIMD_WIDTH);
      IVP_SAV2NX8U_XP(vdst2, a_store, rdst_rgb,
                      3 * WX - 4 * XCHAL_IVPN_SIMD_WIDTH);
      IVP_SAV2NX8UPOS_FP(a_store, rdst_rgb);
      rdst_rgb = OFFSET_PTR_2NX8U(rdst_rgb, 1, shift_rgb, 0);
    }
  }

  if (ix < width) {
    rdst_rgb = OFFSET_PTR_2NX8U(pdst_rgb, 0, 0, 3 * ix);
    rsrc_y = OFFSET_PTR_2NX8U(psrc_y, 0, 0, ix);
    xb_vec2Nx8U *rsrc_u = OFFSET_PTR_2NX8U(psrc_u, 0, 0, (ix >> 1));
    xb_vec2Nx8U *rsrc_v = OFFSET_PTR_2NX8U(psrc_v, 0, 0, (ix >> 1));
    int WX = width - ix;

    int shift_rgb = stride_dst_rgb - 3 * WX;
    int shift_y = stride_src_y - WX;
    int shift_u = stride_src_u - WX / 2;
    int shift_v = stride_src_v - WX / 2;

    xb_vec2Nx8 sy = IVP_SEQ2NX8U();
    xb_vec2Nx8 s4 = IVP_SRLI2NX8U(IVP_AND2NX8U(sy, 0xDF), 1);
    IVP_ADD2NX8UT(sy, sy, XCHAL_IVPN_SIMD_WIDTH,
                  IVP_GEU2NX8U(sy, XCHAL_IVPN_SIMD_WIDTH));

    for (int iy = 0; iy < height; iy += 2) {
      a_load = IVP_LA2NX8U_PP(rsrc_y);
      xb_vec2Nx8U vec_y0;
      IVP_LAV2NX8U_XP(vec_y0, a_load, rsrc_y, WX);
      rsrc_y = OFFSET_PTR_2NX8U(rsrc_y, 1, shift_y, 0);
      a_load = IVP_LA2NX8U_PP(rsrc_y);
      xb_vec2Nx8U vec_y1;
      IVP_LAV2NX8U_XP(vec_y1, a_load, rsrc_y, WX);
      rsrc_y = OFFSET_PTR_2NX8U(rsrc_y, 1, shift_y, 0);
      vec_y0 = IVP_SEL2NX8(vec_y1, vec_y0, sy);

      a_load = IVP_LA2NX8U_PP(rsrc_u);
      xb_vec2Nx8U v_u;
      IVP_LAV2NX8U_XP(v_u, a_load, rsrc_u, WX / 2);
      a_load = IVP_LA2NX8U_PP(rsrc_v);
      xb_vec2Nx8U v_v;
      IVP_LAV2NX8U_XP(v_v, a_load, rsrc_v, WX / 2);

      rsrc_u = OFFSET_PTR_2NX8U(rsrc_u, 1, shift_u, 0);
      rsrc_v = OFFSET_PTR_2NX8U(rsrc_v, 1, shift_v, 0);

      xb_vec2Nx8 vec_v = IVP_SEL2NX8U(v_v, v_v, s4);
      xb_vec2Nx8 vec_u = IVP_SEL2NX8U(v_u, v_u, s4);
      vec_u = IVP_SUB2NX8(vec_u, 128);
      vec_v = IVP_SUB2NX8(vec_v, 128);

      xb_vec2Nx24 w_vec_r0 = IVP_MULI2NX8X16(vec_v, Q15_0_7874, Q15_0_7874);
      IVP_MULUSAI2NX8X16(w_vec_r0, vec_y0, Q15_0_5000, Q15_0_5000);
      xb_vec2Nx8U result_r0 = IVP_PACKVRU2NX24(w_vec_r0, 14);

      xb_vec2Nx24 w_vec_b0 = IVP_MULI2NX8X16(vec_u, Q15_0_9278, Q15_0_9278);
      IVP_MULUSAI2NX8X16(w_vec_b0, vec_y0, Q15_0_5000, Q15_0_5000);
      xb_vec2Nx8U result_b0 = IVP_PACKVRU2NX24(w_vec_b0, 14);

      xb_vec2Nx24 w_vec_g0 = IVP_MULI2NX8X16(vec_v, -Q15_0_2340, -Q15_0_2340);
      IVP_MULAI2NX8X16(w_vec_g0, vec_u, -Q15_0_0936, -Q15_0_0936);
      IVP_MULUSAI2NX8X16(w_vec_g0, vec_y0, Q15_0_5000, Q15_0_5000);
      xb_vec2Nx8U result_g0 = IVP_PACKVRU2NX24(w_vec_g0, 14);

      xb_vec2Nx8 tmp1, tmp0, vdst0, vdst1;
      IVP_DSEL2NX8I(tmp0, tmp1, result_g0, result_r0,
                    IVP_DSELI_8B_INTERLEAVE_1);
      IVP_DSEL2NX8I(vdst1, vdst0, result_b0, tmp1,
                    IVP_DSELI_8B_INTERLEAVE_C3_STEP_0);

      IVP_SAV2NX8U_XP(vdst0, a_store, rdst_rgb, 3 * WX);
      IVP_SAV2NX8U_XP(vdst1, a_store, rdst_rgb,
                      3 * WX - 2 * XCHAL_IVPN_SIMD_WIDTH);
      IVP_SAV2NX8UPOS_FP(a_store, rdst_rgb);
      rdst_rgb = OFFSET_PTR_2NX8U(rdst_rgb, 1, shift_rgb, 0);

      result_b0 =
          IVP_SEL2NX8I(result_b0, result_b0, IVP_SELI_8B_EXTRACT_HI_HALVES);
      IVP_DSEL2NX8I(vdst1, vdst0, result_b0, tmp0,
                    IVP_DSELI_8B_INTERLEAVE_C3_STEP_0);

      IVP_SAV2NX8U_XP(vdst0, a_store, rdst_rgb, 3 * WX);
      IVP_SAV2NX8U_XP(vdst1, a_store, rdst_rgb,
                      3 * WX - 2 * XCHAL_IVPN_SIMD_WIDTH);
      IVP_SAV2NX8UPOS_FP(a_store, rdst_rgb);
      rdst_rgb = OFFSET_PTR_2NX8U(rdst_rgb, 1, shift_rgb, 0);
    }
  }

  return 0;
}
