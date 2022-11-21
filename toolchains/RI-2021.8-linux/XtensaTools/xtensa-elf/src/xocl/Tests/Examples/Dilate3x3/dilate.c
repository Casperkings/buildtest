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

#define PATTERN_SIZE_MAX (31)

XI_ERR_TYPE xiDilate_3x3_U8(xi_tile const *src, xi_array const *dst) {

  int32_t sstride = XI_TILE_GET_PITCH(src);
  int32_t dstride = XI_ARRAY_GET_PITCH(dst);
  int32_t width = XI_TILE_GET_WIDTH(src);
  int32_t height = XI_TILE_GET_HEIGHT(src);

  xb_vec2Nx8U *psrc =
      OFFSET_PTR_2NX8U(XI_TILE_GET_DATA_PTR(src), 1, sstride, -1);
  xb_vec2Nx8U *pdst = (xb_vec2Nx8U *)(XI_ARRAY_GET_DATA_PTR(dst));
  xb_vec2Nx8U *restrict rdst;
  xb_vec2Nx8U *restrict rsrc;

  vbool2N unused;
  valign a_st = IVP_ZALIGN();

  int32_t j = 0;
  for (; j < (width - (2 * XCHAL_IVPN_SIMD_WIDTH));
       j += 4 * XCHAL_IVPN_SIMD_WIDTH) {
    xb_vec2Nx8U pp0, pp1;
    xb_vec2Nx8U qq0, qq1;

    rdst = pdst;

    { // row 0
      rsrc = OFFSET_PTR_2NX8U(psrc, -2, sstride, 0);
      valign a_load = IVP_LA2NX8U_PP(rsrc);
      xb_vec2Nx8U vsel0;
      IVP_LA2NX8U_IP(vsel0, a_load, rsrc);
      xb_vec2Nx8U vsel3;
      IVP_LAV2NX8U_XP(vsel3, a_load, rsrc,
                      width - j + 2 - 2 * XCHAL_IVPN_SIMD_WIDTH);
      xb_vec2Nx8U vtail;
      IVP_LAV2NX8U_XP(vtail, a_load, rsrc,
                      width - j + 2 - 4 * XCHAL_IVPN_SIMD_WIDTH);

      xb_vec2Nx8 vsel1, vsel2, vsel4, vsel5;
      IVP_DSEL2NX8I(vsel2, vsel1, vsel3, vsel0, IVP_DSELI_8B_ROTATE_RIGHT_2_1);
      IVP_DSEL2NX8I(vsel5, vsel4, vtail, vsel3, IVP_DSELI_8B_ROTATE_RIGHT_2_1);

      IVP_BMAXU2NX8U(unused, pp0, vsel0, vsel1);
      IVP_BMAXU2NX8U(unused, qq0, vsel3, vsel4);
      IVP_BMAXU2NX8U(unused, pp0, pp0, vsel2);
      IVP_BMAXU2NX8U(unused, qq0, qq0, vsel5);
    }

    { // row 1
      rsrc = OFFSET_PTR_2NX8U(psrc, -1, sstride, 0);
      valign a_load = IVP_LA2NX8U_PP(rsrc);
      xb_vec2Nx8U vsel0;
      IVP_LA2NX8U_IP(vsel0, a_load, rsrc);
      xb_vec2Nx8U vsel3;
      IVP_LAV2NX8U_XP(vsel3, a_load, rsrc,
                      width - j + 2 - 2 * XCHAL_IVPN_SIMD_WIDTH);
      xb_vec2Nx8U vtail;
      IVP_LAV2NX8U_XP(vtail, a_load, rsrc,
                      width - j + 2 - 4 * XCHAL_IVPN_SIMD_WIDTH);

      xb_vec2Nx8 vsel1, vsel2, vsel4, vsel5;
      IVP_DSEL2NX8I(vsel2, vsel1, vsel3, vsel0, IVP_DSELI_8B_ROTATE_RIGHT_2_1);
      IVP_DSEL2NX8I(vsel5, vsel4, vtail, vsel3, IVP_DSELI_8B_ROTATE_RIGHT_2_1);

      IVP_BMAXU2NX8U(unused, pp1, vsel0, vsel1);
      IVP_BMAXU2NX8U(unused, qq1, vsel3, vsel4);
      IVP_BMAXU2NX8U(unused, pp1, pp1, vsel2);
      IVP_BMAXU2NX8U(unused, qq1, qq1, vsel5);
      IVP_BMAXU2NX8U(unused, pp0, pp0, pp1);
      IVP_BMAXU2NX8U(unused, qq0, qq0, qq1);
    }

    rsrc = psrc;
    for (int32_t i = 0; i < height; i++) {
      xb_vec2Nx8U *nsrc = OFFSET_PTR_2NX8U(rsrc, 1, sstride, 0);
      xb_vec2Nx8U *ndst = OFFSET_PTR_2NX8U(rdst, 1, dstride, 0);
      valign a_load = IVP_LA2NX8U_PP(rsrc);
      xb_vec2Nx8U vsel0;
      IVP_LA2NX8U_IP(vsel0, a_load, rsrc);
      xb_vec2Nx8U vsel3;
      IVP_LAV2NX8U_XP(vsel3, a_load, rsrc,
                      width - j + 2 - 2 * XCHAL_IVPN_SIMD_WIDTH);
      xb_vec2Nx8U vtail;
      IVP_LAV2NX8U_XP(vtail, a_load, rsrc,
                      width - j + 2 - 4 * XCHAL_IVPN_SIMD_WIDTH);
      rsrc = nsrc;

      xb_vec2Nx8 vsel1, vsel2, vsel4, vsel5;
      vsel4 = IVP_SEL2NX8I(vtail, vsel3, IVP_SELI_8B_ROTATE_RIGHT_1);
      vsel5 = IVP_SEL2NX8I(vtail, vsel3, IVP_SELI_8B_ROTATE_RIGHT_2);
      IVP_DSEL2NX8I(vsel2, vsel1, vsel3, vsel0, IVP_DSELI_8B_ROTATE_RIGHT_2_1);

      xb_vec2Nx8U pp, qq;
      IVP_BMAXU2NX8U(unused, pp, vsel0, vsel1);
      IVP_BMAXU2NX8U(unused, pp, pp, vsel2);
      IVP_BMAXU2NX8U(unused, qq, vsel3, vsel4);
      IVP_BMAXU2NX8U(unused, qq, qq, vsel5);

      IVP_BMAXU2NX8U(unused, pp0, pp, pp0);
      IVP_BMAXU2NX8U(unused, qq0, qq, qq0);

      IVP_SA2NX8U_IP(pp0, a_st, rdst);
      IVP_SAV2NX8U_XP(qq0, a_st, rdst, width - j - 2 * XCHAL_IVPN_SIMD_WIDTH);
      IVP_SAPOS2NX8U_FP(a_st, rdst);
      rdst = ndst;

      IVP_BMAXU2NX8U(unused, pp0, pp1, pp);
      IVP_BMAXU2NX8U(unused, qq0, qq1, qq);

      pp1 = pp;
      qq1 = qq;
    }
    psrc += 2;
    pdst += 2;
  }

  if (j < width) {
    xb_vec2Nx8U pp0, pp1;

    rdst = pdst;

    { // row 0
      rsrc = OFFSET_PTR_2NX8U(psrc, -2, sstride, 0);
      valign a_load = IVP_LA2NX8U_PP(rsrc);
      xb_vec2Nx8U vsel0;
      IVP_LAV2NX8U_XP(vsel0, a_load, rsrc, width - j + 2);
      xb_vec2Nx8U vtail;
      IVP_LAV2NX8U_XP(vtail, a_load, rsrc,
                      width - j + 2 - 2 * XCHAL_IVPN_SIMD_WIDTH);

      xb_vec2Nx8 vsel1, vsel2;
      IVP_DSEL2NX8I(vsel2, vsel1, vtail, vsel0, IVP_DSELI_8B_ROTATE_RIGHT_2_1);

      IVP_BMAXU2NX8U(unused, pp0, vsel0, vsel1);
      IVP_BMAXU2NX8U(unused, pp0, pp0, vsel2);
    }

    { // row 1
      rsrc = OFFSET_PTR_2NX8U(psrc, -1, sstride, 0);
      valign a_load = IVP_LA2NX8U_PP(rsrc);
      xb_vec2Nx8U vsel0;
      IVP_LAV2NX8U_XP(vsel0, a_load, rsrc, width - j + 2);
      xb_vec2Nx8U vtail;
      IVP_LAV2NX8U_XP(vtail, a_load, rsrc,
                      width - j + 2 - 2 * XCHAL_IVPN_SIMD_WIDTH);

      xb_vec2Nx8 vsel1, vsel2;
      IVP_DSEL2NX8I(vsel2, vsel1, vtail, vsel0, IVP_DSELI_8B_ROTATE_RIGHT_2_1);

      IVP_BMAXU2NX8U(unused, pp1, vsel0, vsel1);
      IVP_BMAXU2NX8U(unused, pp1, pp1, vsel2);
    }

    rsrc = psrc;
    for (int32_t i = 0; i < height; i++) {
      xb_vec2Nx8U *nsrc = OFFSET_PTR_2NX8U(rsrc, 1, sstride, 0);
      xb_vec2Nx8U *ndst = OFFSET_PTR_2NX8U(rdst, 1, dstride, 0);
      valign a_load = IVP_LA2NX8U_PP(rsrc);
      xb_vec2Nx8U vsel0;
      IVP_LAV2NX8U_XP(vsel0, a_load, rsrc, width - j + 2);
      xb_vec2Nx8U vtail;
      IVP_LAV2NX8U_XP(vtail, a_load, rsrc,
                      width - j + 2 - 2 * XCHAL_IVPN_SIMD_WIDTH);
      rsrc = nsrc;

      xb_vec2Nx8 vsel1, vsel2;
      IVP_DSEL2NX8I(vsel2, vsel1, vtail, vsel0, IVP_DSELI_8B_ROTATE_RIGHT_2_1);

      xb_vec2Nx8U pp2;
      IVP_BMAXU2NX8U(unused, pp2, vsel0, vsel1);
      IVP_BMAXU2NX8U(unused, pp2, pp2, vsel2);

      IVP_BMAXU2NX8U(unused, pp0, pp0, pp2);
      IVP_BMAXU2NX8U(unused, pp0, pp0, pp1);

      IVP_SAV2NX8U_XP(pp0, a_st, rdst, width - j);
      IVP_SAPOS2NX8U_FP(a_st, rdst);
      rdst = ndst;

      pp0 = pp1;
      pp1 = pp2;
    }
  }

  return 0;
}
