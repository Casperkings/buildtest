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

#define Q15_0_2126 6966
#define Q15_0_7152 23436
#define Q15_0_0722 2366

#define Q15_0_5389 17659
#define Q15_0_6350 20808
XI_ERR_TYPE xiCvtColor_U8_RGB2YUV_BT709(const xi_pArray rgb, xi_pArray y,
                                        xi_pArray u, xi_pArray v) {
  const uint8_t *psrc_rgb = (uint8_t *)XI_ARRAY_GET_DATA_PTR(rgb);
  const uint8_t *pdst_y = (uint8_t *)XI_ARRAY_GET_DATA_PTR(y);
  const uint8_t *pdst_u = (uint8_t *)XI_ARRAY_GET_DATA_PTR(u);
  const uint8_t *pdst_v = (uint8_t *)XI_ARRAY_GET_DATA_PTR(v);

  const int width = XI_ARRAY_GET_WIDTH(y);
  const int height = XI_ARRAY_GET_HEIGHT(y);

  xb_vec2Nx8U *restrict rsrc_rgb;
  xb_vec2Nx8U *restrict rdst_y;
  xb_vec2Nx8U *restrict rdst_u;
  xb_vec2Nx8U *restrict rdst_v;

  valign a_load;
  valign a_store = IVP_ZALIGN();
#if XCHAL_VISION_TYPE == 6 && XCHAL_HAVE_VISION

  int x = 0;
  for (; x <= width - 4 * XCHAL_IVPN_SIMD_WIDTH;
       x += 4 * XCHAL_IVPN_SIMD_WIDTH) {
    int WX = width - XT_MAX(x, width - 4 * XCHAL_IVPN_SIMD_WIDTH);

    int shift_y = XI_ARRAY_GET_PITCH(y) - WX;
    int shift_u = XI_ARRAY_GET_PITCH(u) - WX;
    int shift_v = XI_ARRAY_GET_PITCH(v) - WX;

    rsrc_rgb = OFFSET_PTR_2NX8U(psrc_rgb, 0, 0, 3 * x);
    int shift_rgb = XI_ARRAY_GET_PITCH(rgb) - 3 * WX;
    rdst_y = OFFSET_PTR_2NX8U(pdst_y, 0, 0, x);
    rdst_u = OFFSET_PTR_2NX8U(pdst_u, 0, 0, x);
    rdst_v = OFFSET_PTR_2NX8U(pdst_v, 0, 0, x);

    for (int y = 0; y < height; y++) {
      xb_vec2Nx8U vec0, vec1, vec2;
      xb_vec2Nx8U vec3, vec4, vec5;

      a_load = IVP_LA2NX8U_PP(rsrc_rgb);
      IVP_LA2NX8U_IP(vec0, a_load, rsrc_rgb);
      IVP_LA2NX8U_IP(vec1, a_load, rsrc_rgb);
      IVP_LA2NX8U_IP(vec2, a_load, rsrc_rgb);
      IVP_LA2NX8U_IP(vec3, a_load, rsrc_rgb);
      IVP_LA2NX8U_IP(vec4, a_load, rsrc_rgb);
      IVP_LA2NX8U_IP(vec5, a_load, rsrc_rgb);

      rsrc_rgb = OFFSET_PTR_2NX8U(rsrc_rgb, 1, shift_rgb, 0);

      xb_vec2Nx8 vec_r0, vec_g0, vec_b0, tmp0, tmp1;
      xb_vec2Nx8 vec_r1, vec_g1, vec_b1, tmp2, tmp3;

      IVP_DSEL2NX8I(vec_b0, tmp0, vec1, vec0,
                    IVP_DSELI_8B_DEINTERLEAVE_C3_STEP_0);
      IVP_DSEL2NX8I_H(vec_b0, tmp1, vec2, vec1,
                      IVP_DSELI_8B_DEINTERLEAVE_C3_STEP_1);
      IVP_DSEL2NX8I(vec_g0, vec_r0, tmp1, tmp0, IVP_DSELI_8B_DEINTERLEAVE_1);

      xb_vec2Nx24 vec_y0 = IVP_MULUSI2NX8X16(vec_b0, Q15_0_0722, Q15_0_0722);
      IVP_MULUSPA2N8XR16(vec_y0, vec_r0, vec_g0,
                         ((int)Q15_0_2126 << 16) | Q15_0_7152);

      xb_vec2Nx8U vec_y_8u_0 = IVP_PACKVRU2NX24(vec_y0, 15);

      xb_vec2Nx24 w_u0 = IVP_MULUSP2N8XR16(
          vec_y_8u_0, vec_b0, (-(Q15_0_5389 << 16)) | (Q15_0_5389));
      xb_vec2Nx24 w_v0 = IVP_MULUSP2N8XR16(
          vec_y_8u_0, vec_r0, (-(Q15_0_6350 << 16)) | (Q15_0_6350));

      xb_vec2Nx8U vdst_u0 = IVP_ADD2NX8(IVP_PACKVR2NX24(w_u0, 15), 128);
      xb_vec2Nx8U vdst_v0 = IVP_ADD2NX8(IVP_PACKVR2NX24(w_v0, 15), 128);

      IVP_DSEL2NX8I(vec_b1, tmp2, vec4, vec3,
                    IVP_DSELI_8B_DEINTERLEAVE_C3_STEP_0);
      IVP_DSEL2NX8I_H(vec_b1, tmp3, vec5, vec4,
                      IVP_DSELI_8B_DEINTERLEAVE_C3_STEP_1);
      IVP_DSEL2NX8I(vec_g1, vec_r1, tmp3, tmp2, IVP_DSELI_8B_DEINTERLEAVE_1);

      xb_vec2Nx24 vec_y1 = IVP_MULUSI2NX8X16(vec_b1, Q15_0_0722, Q15_0_0722);
      IVP_MULUSPA2N8XR16(vec_y1, vec_r1, vec_g1,
                         ((int)Q15_0_2126 << 16) | Q15_0_7152);

      xb_vec2Nx8U vec_y_8u_1 = IVP_PACKVRU2NX24(vec_y1, 15);

      xb_vec2Nx24 w_u1 = IVP_MULUSP2N8XR16(
          vec_y_8u_1, vec_b1, (-(Q15_0_5389 << 16)) | (Q15_0_5389));
      xb_vec2Nx24 w_v1 = IVP_MULUSP2N8XR16(
          vec_y_8u_1, vec_r1, (-(Q15_0_6350 << 16)) | (Q15_0_6350));

      xb_vec2Nx8U vdst_u1 = IVP_ADD2NX8(IVP_PACKVR2NX24(w_u1, 15), 128);
      xb_vec2Nx8U vdst_v1 = IVP_ADD2NX8(IVP_PACKVR2NX24(w_v1, 15), 128);

      IVP_SA2NX8U_IP(vec_y_8u_0, a_store, rdst_y);
      IVP_SA2NX8U_IP(vec_y_8u_1, a_store, rdst_y);
      IVP_SAPOS2NX8U_FP(a_store, rdst_y);

      IVP_SA2NX8U_IP(vdst_u0, a_store, rdst_u);
      IVP_SA2NX8U_IP(vdst_u1, a_store, rdst_u);
      IVP_SAPOS2NX8U_FP(a_store, rdst_u);

      IVP_SA2NX8U_IP(vdst_v0, a_store, rdst_v);
      IVP_SA2NX8U_IP(vdst_v1, a_store, rdst_v);
      IVP_SAPOS2NX8U_FP(a_store, rdst_v);

      rdst_v = OFFSET_PTR_2NX8U(rdst_v, 1, shift_v, 0);
      rdst_u = OFFSET_PTR_2NX8U(rdst_u, 1, shift_u, 0);
      rdst_y = OFFSET_PTR_2NX8U(rdst_y, 1, shift_y, 0);
    }
  }
  for (; x <= width - 2 * XCHAL_IVPN_SIMD_WIDTH;
       x += 2 * XCHAL_IVPN_SIMD_WIDTH) {
    int WX = width - XT_MAX(x, width - 2 * XCHAL_IVPN_SIMD_WIDTH);
    int shift_y = XI_ARRAY_GET_PITCH(y) - WX;
    int shift_u = XI_ARRAY_GET_PITCH(u) - WX;
    int shift_v = XI_ARRAY_GET_PITCH(v) - WX;

    rsrc_rgb = OFFSET_PTR_2NX8U(psrc_rgb, 0, 0, 3 * x);
    int shift_rgb = XI_ARRAY_GET_PITCH(rgb) - 3 * WX;
    rdst_y = OFFSET_PTR_2NX8U(pdst_y, 0, 0, x);
    rdst_u = OFFSET_PTR_2NX8U(pdst_u, 0, 0, x);
    rdst_v = OFFSET_PTR_2NX8U(pdst_v, 0, 0, x);

    for (int y = 0; y < height; y++) {
      xb_vec2Nx8U vec0, vec1, vec2;
      a_load = IVP_LA2NX8U_PP(rsrc_rgb);
      IVP_LA2NX8U_IP(vec0, a_load, rsrc_rgb);
      IVP_LA2NX8U_IP(vec1, a_load, rsrc_rgb);
      IVP_LA2NX8U_IP(vec2, a_load, rsrc_rgb);
      rsrc_rgb = OFFSET_PTR_2NX8U(rsrc_rgb, 1, shift_rgb, 0);

      xb_vec2Nx8 vec_r0, vec_g0, vec_b0, tmp0, tmp1;
      IVP_DSEL2NX8I(vec_b0, tmp0, vec1, vec0,
                    IVP_DSELI_8B_DEINTERLEAVE_C3_STEP_0);
      IVP_DSEL2NX8I_H(vec_b0, tmp1, vec2, vec1,
                      IVP_DSELI_8B_DEINTERLEAVE_C3_STEP_1);
      IVP_DSEL2NX8I(vec_g0, vec_r0, tmp1, tmp0, IVP_DSELI_8B_DEINTERLEAVE_1);

      xb_vec2Nx24 vec_y0 = IVP_MULUSI2NX8X16(vec_b0, Q15_0_0722, Q15_0_0722);
      IVP_MULUSPA2N8XR16(vec_y0, vec_r0, vec_g0,
                         ((int)Q15_0_2126 << 16) | Q15_0_7152);

      xb_vec2Nx8U vec_y_8u_0 = IVP_PACKVRU2NX24(vec_y0, 15);

      xb_vec2Nx24 w_u0 = IVP_MULUSP2N8XR16(
          vec_y_8u_0, vec_b0, (-(Q15_0_5389 << 16)) | (Q15_0_5389));
      xb_vec2Nx24 w_v0 = IVP_MULUSP2N8XR16(
          vec_y_8u_0, vec_r0, (-(Q15_0_6350 << 16)) | (Q15_0_6350));

      xb_vec2Nx8U vdst_u0 = IVP_ADD2NX8(IVP_PACKVR2NX24(w_u0, 15), 128);
      xb_vec2Nx8U vdst_v0 = IVP_ADD2NX8(IVP_PACKVR2NX24(w_v0, 15), 128);

      IVP_SA2NX8U_IP(vec_y_8u_0, a_store, rdst_y);
      IVP_SAPOS2NX8U_FP(a_store, rdst_y);
      rdst_y = OFFSET_PTR_2NX8U(rdst_y, 1, shift_y, 0);

      IVP_SA2NX8U_IP(vdst_u0, a_store, rdst_u);
      IVP_SAPOS2NX8U_FP(a_store, rdst_u);
      rdst_u = OFFSET_PTR_2NX8U(rdst_u, 1, shift_u, 0);

      IVP_SA2NX8U_IP(vdst_v0, a_store, rdst_v);
      IVP_SAPOS2NX8U_FP(a_store, rdst_v);
      rdst_v = OFFSET_PTR_2NX8U(rdst_v, 1, shift_v, 0);
    }
  }
  for (; x < width; x += 2 * XCHAL_IVPN_SIMD_WIDTH) {
    rsrc_rgb = OFFSET_PTR_2NX8U(psrc_rgb, 0, 0, 3 * x);
    rdst_y = OFFSET_PTR_2NX8U(pdst_y, 0, 0, x);
    rdst_u = OFFSET_PTR_2NX8U(pdst_u, 0, 0, x);
    rdst_v = OFFSET_PTR_2NX8U(pdst_v, 0, 0, x);

    int WX = width - XT_MAX(x, width - 2 * XCHAL_IVPN_SIMD_WIDTH);

    int shift_rgb = XI_ARRAY_GET_PITCH(rgb) - 3 * WX;
    int shift_y = XI_ARRAY_GET_PITCH(y) - WX;
    int shift_u = XI_ARRAY_GET_PITCH(u) - WX;
    int shift_v = XI_ARRAY_GET_PITCH(v) - WX;

    for (int iy = 0; iy < height; iy += 1) {
      a_load = IVP_LA2NX8U_PP(rsrc_rgb);
      xb_vec2Nx8U vec0;
      IVP_LAV2NX8U_XP(vec0, a_load, rsrc_rgb, 3 * WX);
      xb_vec2Nx8U vec1;
      IVP_LAV2NX8U_XP(vec1, a_load, rsrc_rgb,
                      3 * WX - 2 * XCHAL_IVPN_SIMD_WIDTH);
      xb_vec2Nx8U vec2;
      IVP_LAV2NX8U_XP(vec2, a_load, rsrc_rgb,
                      3 * WX - 4 * XCHAL_IVPN_SIMD_WIDTH);
      rsrc_rgb = OFFSET_PTR_2NX8U(rsrc_rgb, 1, shift_rgb, 0);

      xb_vec2Nx8 vec_r0, vec_g0, vec_b0, tmp0, tmp1;
      IVP_DSEL2NX8I(vec_b0, tmp0, vec1, vec0,
                    IVP_DSELI_8B_DEINTERLEAVE_C3_STEP_0);
      IVP_DSEL2NX8I_H(vec_b0, tmp1, vec2, vec1,
                      IVP_DSELI_8B_DEINTERLEAVE_C3_STEP_1);
      IVP_DSEL2NX8I(vec_g0, vec_r0, tmp1, tmp0, IVP_DSELI_8B_DEINTERLEAVE_1);

      xb_vec2Nx24 vec_y0 = IVP_MULUSI2NX8X16(vec_b0, Q15_0_0722, Q15_0_0722);
      IVP_MULUSAI2NX8X16(vec_y0, vec_g0, Q15_0_7152, Q15_0_7152);
      IVP_MULUSAI2NX8X16(vec_y0, vec_r0, Q15_0_2126, Q15_0_2126);

      xb_vec2Nx8U vec_y_8u_0 = IVP_PACKVRU2NX24(vec_y0, 15);

      xb_vec2Nx24 w_u0 = IVP_MULUSI2NX8X16(vec_b0, Q15_0_5389, Q15_0_5389);
      xb_vec2Nx24 w_v0 = IVP_MULUSI2NX8X16(vec_r0, Q15_0_6350, Q15_0_6350);

      IVP_MULUSAI2NX8X16(w_u0, vec_y_8u_0, -Q15_0_5389, -Q15_0_5389);
      IVP_MULUSAI2NX8X16(w_v0, vec_y_8u_0, -Q15_0_6350, -Q15_0_6350);

      xb_vec2Nx8U vdst_u0 = IVP_ADD2NX8(IVP_PACKVR2NX24(w_u0, 15), 128);
      xb_vec2Nx8U vdst_v0 = IVP_ADD2NX8(IVP_PACKVR2NX24(w_v0, 15), 128);

      IVP_SAV2NX8U_XP(vec_y_8u_0, a_store, rdst_y, WX);
      IVP_SAPOS2NX8U_FP(a_store, rdst_y);
      rdst_y = OFFSET_PTR_2NX8U(rdst_y, 1, shift_y, 0);

      IVP_SAV2NX8U_XP(vdst_v0, a_store, rdst_v, WX);
      IVP_SAPOS2NX8U_FP(a_store, rdst_v);
      rdst_v = OFFSET_PTR_2NX8U(rdst_v, 1, shift_v, 0);

      IVP_SAV2NX8U_XP(vdst_u0, a_store, rdst_u, WX);
      IVP_SAPOS2NX8U_FP(a_store, rdst_u);
      rdst_u = OFFSET_PTR_2NX8U(rdst_u, 1, shift_u, 0);
    }
  }
#else
  for (int ix = 0; ix < width; ix += 2 * XCHAL_IVPN_SIMD_WIDTH) {
    rsrc_rgb = OFFSET_PTR_2NX8U(psrc_rgb, 0, 0, 3 * ix);
    rdst_y = OFFSET_PTR_2NX8U(pdst_y, 0, 0, ix);
    rdst_u = OFFSET_PTR_2NX8U(pdst_u, 0, 0, ix);
    rdst_v = OFFSET_PTR_2NX8U(pdst_v, 0, 0, ix);

    int WX = width - XT_MAX(ix, width - 2 * XCHAL_IVPN_SIMD_WIDTH);

    int shift_rgb = XI_ARRAY_GET_PITCH(rgb) - 3 * WX;
    int shift_y = XI_ARRAY_GET_PITCH(y) - WX;
    int shift_u = XI_ARRAY_GET_PITCH(u) - WX;
    int shift_v = XI_ARRAY_GET_PITCH(v) - WX;

#if XCHAL_HAVE_VISION

    for (int iy = 0; iy < height; iy += 1) {
      a_load = IVP_LA2NX8U_PP(rsrc_rgb);
      xb_vec2Nx8U vec0;
      IVP_LAV2NX8U_XP(vec0, a_load, rsrc_rgb, 3 * WX);
      xb_vec2Nx8U vec1;
      IVP_LAV2NX8U_XP(vec1, a_load, rsrc_rgb,
                      3 * WX - 2 * XCHAL_IVPN_SIMD_WIDTH);
      xb_vec2Nx8U vec2;
      IVP_LAV2NX8U_XP(vec2, a_load, rsrc_rgb,
                      3 * WX - 4 * XCHAL_IVPN_SIMD_WIDTH);
      rsrc_rgb = OFFSET_PTR_2NX8U(rsrc_rgb, 1, shift_rgb, 0);

      xb_vec2Nx8 vec_r0, vec_g0, vec_b0, tmp0, tmp1;
      IVP_DSEL2NX8I(vec_b0, tmp0, vec1, vec0,
                    IVP_DSELI_8B_DEINTERLEAVE_C3_STEP_0);
      IVP_DSEL2NX8I_H(vec_b0, tmp1, vec2, vec1,
                      IVP_DSELI_8B_DEINTERLEAVE_C3_STEP_1);
      IVP_DSEL2NX8I(vec_g0, vec_r0, tmp1, tmp0, IVP_DSELI_8B_DEINTERLEAVE_1);

      xb_vec2Nx24 vec_y0 = IVP_MULUSI2NX8X16(vec_b0, Q15_0_0722, Q15_0_0722);
      IVP_MULUSAI2NX8X16(vec_y0, vec_g0, Q15_0_7152, Q15_0_7152);
      IVP_MULUSAI2NX8X16(vec_y0, vec_r0, Q15_0_2126, Q15_0_2126);

      xb_vec2Nx8U vec_y_8u_0 = IVP_PACKVRU2NX24(vec_y0, 15);
      xb_vec2Nx24 w_u0 = IVP_MULUSI2NX8X16(vec_b0, Q15_0_5389, Q15_0_5389);
      xb_vec2Nx24 w_v0 = IVP_MULUSI2NX8X16(vec_r0, Q15_0_6350, Q15_0_6350);

      IVP_MULUSAI2NX8X16(w_u0, vec_y_8u_0, -Q15_0_5389, -Q15_0_5389);
      IVP_MULUSAI2NX8X16(w_v0, vec_y_8u_0, -Q15_0_6350, -Q15_0_6350);

      xb_vec2Nx8U vdst_u0 = IVP_ADD2NX8(IVP_PACKVR2NX24(w_u0, 15), 128);
      xb_vec2Nx8U vdst_v0 = IVP_ADD2NX8(IVP_PACKVR2NX24(w_v0, 15), 128);

      IVP_SAV2NX8U_XP(vec_y_8u_0, a_store, rdst_y, WX);
      IVP_SAPOS2NX8U_FP(a_store, rdst_y);
      rdst_y = OFFSET_PTR_2NX8U(rdst_y, 1, shift_y, 0);

      IVP_SAV2NX8U_XP(vdst_v0, a_store, rdst_v, WX);
      IVP_SAPOS2NX8U_FP(a_store, rdst_v);
      rdst_v = OFFSET_PTR_2NX8U(rdst_v, 1, shift_v, 0);

      IVP_SAV2NX8U_XP(vdst_u0, a_store, rdst_u, WX);
      IVP_SAPOS2NX8U_FP(a_store, rdst_u);
      rdst_u = OFFSET_PTR_2NX8U(rdst_u, 1, shift_u, 0);
    }
#else
    for (int iy = 0; iy < height; iy += 1) {
      a_load = IVP_LA2NX8U_PP(rsrc_rgb);
      xb_vec2Nx8U vec0;
      IVP_LAV2NX8U_XP(vec0, a_load, rsrc_rgb, 3 * WX);
      xb_vec2Nx8U vec1;
      IVP_LAV2NX8U_XP(vec1, a_load, rsrc_rgb,
                      3 * WX - 2 * XCHAL_IVPN_SIMD_WIDTH);
      xb_vec2Nx8U vec2;
      IVP_LAV2NX8U_XP(vec2, a_load, rsrc_rgb,
                      3 * WX - 4 * XCHAL_IVPN_SIMD_WIDTH);
      rsrc_rgb = OFFSET_PTR_2NX8U(rsrc_rgb, 1, shift_rgb, 0);

      xb_vec2Nx8 vec_r, vec_g, vec_b, tmp0, tmp1;
      IVP_DSEL2NX8I(vec_b, tmp0, vec1, vec0,
                    IVP_DSELI_8B_DEINTERLEAVE_C3_STEP_0);
      IVP_DSEL2NX8I_H(vec_b, tmp1, vec2, vec1,
                      IVP_DSELI_8B_DEINTERLEAVE_C3_STEP_1);
      IVP_DSEL2NX8I(vec_g, vec_r, tmp1, tmp0, IVP_DSELI_8B_DEINTERLEAVE_1);

      xb_vec2Nx24 vec_y = IVP_MULUSI2NX8X16(vec_b, Q15_0_0722, Q15_0_0722);
      IVP_MULUSAI2NX8X16(vec_y, vec_g, Q15_0_7152, Q15_0_7152);
      IVP_MULUSAI2NX8X16(vec_y, vec_r, Q15_0_2126, Q15_0_2126);
      IVP_SAV2NX8U_XP(IVP_PACKVRU2NX24(vec_y, 15), a_store, rdst_y, WX);
      IVP_SAPOS2NX8U_FP(a_store, rdst_y);
      xb_vec2Nx8U vec_y1 = IVP_PACKVRU2NX24(vec_y, 15);
      vbool2N bu = IVP_GEU2NX8U(vec_y1, vec_b);
      vbool2N bv = IVP_GEU2NX8U(vec_y1, vec_r);

      xb_vec2Nx24 vec_u = IVP_MULUSI2NX8X16(IVP_ABSSUBU2NX8(vec_b, vec_y1),
                                            Q15_0_5389, Q15_0_5389);
      xb_vec2Nx24 vec_v = IVP_MULUSI2NX8X16(IVP_ABSSUBU2NX8(vec_r, vec_y1),
                                            Q15_0_6350, Q15_0_6350);

      xb_vec2Nx8U vdst0 =
          IVP_MOV2NX8T(IVP_SUB2NX8U(128, IVP_PACKVR2NX24(vec_u, 15)),
                       IVP_ADD2NX8(IVP_PACKVR2NX24(vec_u, 15), 128), bu);
      xb_vec2Nx8U vdst1 =
          IVP_MOV2NX8T(IVP_SUB2NX8U(128, IVP_PACKVR2NX24(vec_v, 15)),
                       IVP_ADD2NX8(IVP_PACKVR2NX24(vec_v, 15), 128), bv);

      IVP_SAV2NX8U_XP(vdst0, a_store, rdst_u, WX);
      IVP_SAPOS2NX8U_FP(a_store, rdst_u);

      IVP_SAV2NX8U_XP(vdst1, a_store, rdst_v, WX);
      IVP_SAPOS2NX8U_FP(a_store, rdst_v);

      rdst_y = OFFSET_PTR_2NX8U(rdst_y, 1, shift_y, 0);
      rdst_u = OFFSET_PTR_2NX8U(rdst_u, 1, shift_u, 0);
      rdst_v = OFFSET_PTR_2NX8U(rdst_v, 1, shift_v, 0);
    }
#endif
  }
#endif
  return 0;
}
