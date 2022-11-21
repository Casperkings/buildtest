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

#define XI__SEL2NX8I(tail, body, unused)                                       \
  IVP_MOV2NX8U_FROMNX16(IVP_SELNX16I(IVP_MOVNX16_FROM2NX8U(tail),              \
                                     IVP_MOVNX16_FROM2NX8U(body),              \
                                     IVP_SELI_ROTATE_RIGHT_1))

#define SORT_PIX(p0, p1)                                                       \
  temp = IVP_MINNX16(p0, p1);                                                  \
  p1 = IVP_MAXNX16(p0, p1);                                                    \
  p0 = temp;

#define SORT_ROW(v0, v1, v2)                                                   \
  {                                                                            \
    xb_vecNx16 temp;                                                           \
    SORT_PIX(v1, v2);                                                          \
    SORT_PIX(v0, v1);                                                          \
    SORT_PIX(v1, v2);                                                          \
  }

// This macro calculates the median of values {v0,...,v8}
// NOTE: sets (v0, v1, v2), (v3, v4, v5), (v6, v7, v8) have to be sorted
#define GET_MEDIAN(v0, v1, v2, v3, v4, v5, v6, v7, v8, vtmp, vmed)             \
  {                                                                            \
    vtmp = IVP_MAXNX16(v0, v3);                                                \
    v0 = IVP_MAXNX16(v4, v7);                                                  \
    vmed = IVP_MINNX16(v4, v7);                                                \
    vmed = IVP_MAXNX16(v1, vmed);                                              \
    v1 = IVP_MINNX16(v5, v8);                                                  \
    v2 = IVP_MINNX16(v2, v1);                                                  \
    vmed = IVP_MINNX16(vmed, v0);                                              \
    v1 = IVP_MINNX16(vmed, v2);                                                \
    v2 = IVP_MAXNX16(vmed, v2);                                                \
    vtmp = IVP_MAXNX16(vtmp, v6);                                              \
    vmed = IVP_MAXNX16(vtmp, v1);                                              \
    vmed = IVP_MINNX16(vmed, v2);                                              \
  }

//////////////

#define SORT_PIX_8U2(p0, p1)                                                   \
  IVP_BMINU2NX8(unused, temp, p0, p1);                                         \
  IVP_BMAXU2NX8(unused, p1, p0, p1);                                           \
  p0 = temp;

#define SORT_ROW_8U2(v0, v1, v2)                                               \
  {                                                                            \
    vbool2N unused;                                                            \
    xb_vec2Nx8 temp;                                                           \
    SORT_PIX_8U2(v1, v2);                                                      \
    SORT_PIX_8U2(v0, v1);                                                      \
    SORT_PIX_8U2(v1, v2);                                                      \
  }

#define GET_MEDIAN_8U2(v0, v1, v2, v3, v4, v5, v6, v7, v8, vtmp, vmed)         \
  {                                                                            \
    vbool2N unused;                                                            \
    IVP_BMAXU2NX8(unused, vtmp, v0, v3);                                       \
    IVP_BMAXU2NX8(unused, v0, v4, v7);                                         \
    IVP_BMINU2NX8(unused, vmed, v4, v7);                                       \
    IVP_BMAXU2NX8(unused, vmed, v1, vmed);                                     \
    IVP_BMINU2NX8(unused, v1, v5, v8);                                         \
    IVP_BMINU2NX8(unused, v2, v2, v1);                                         \
    IVP_BMINU2NX8(unused, vmed, vmed, v0);                                     \
    IVP_BMINU2NX8(unused, v1, vmed, v2);                                       \
    IVP_BMAXU2NX8(unused, v2, vmed, v2);                                       \
    IVP_BMAXU2NX8(unused, vtmp, vtmp, v6);                                     \
    IVP_BMAXU2NX8(unused, vmed, vtmp, v1);                                     \
    IVP_BMINU2NX8(unused, vmed, vmed, v2);                                     \
  }

XI_ERR_TYPE xiMedianBlur_3x3_U8(const xi_pTile src, xi_pArray dst) {

  int sstride = XI_TILE_GET_PITCH(src);
  int dstride = XI_ARRAY_GET_PITCH(dst);

  int width = XI_TILE_GET_WIDTH(src);
  int height = XI_TILE_GET_HEIGHT(src);

  xb_vec2Nx8U *psrc =
      OFFSET_PTR_2NX8U(XI_TILE_GET_DATA_PTR(src), 1, sstride, -1);
  xb_vec2Nx8U *pdst = (xb_vec2Nx8U *)XI_ARRAY_GET_DATA_PTR(dst);
  xb_vec2Nx8U *restrict rsrc;
  xb_vec2Nx8U *restrict rdst;
  valign a_store = IVP_ZALIGN();

  for (int x = 0; x < width; x += 2 * XCHAL_IVPN_SIMD_WIDTH, psrc++, pdst++) {
    xb_vec2Nx8 v0, v1, v2, v3, v4, v5, v6, v7, v8;
    {
      xb_vec2Nx8U vtail;

      rsrc = OFFSET_PTR_2NX8U(psrc, -2, sstride, 0);
      valign a_load = IVP_LA2NX8U_PP(rsrc);
      IVP_LAV2NX8_XP(v0, a_load, rsrc, width - x + 2);
      IVP_LAV2NX8U_XP(vtail, a_load, rsrc,
                      width - x + 2 - 2 * XCHAL_IVPN_SIMD_WIDTH);

      IVP_DSEL2NX8I(v2, v1, vtail, v0, IVP_DSELI_8B_ROTATE_RIGHT_2_1);
      SORT_ROW_8U2(v0, v1, v2)
    }
    {
      xb_vec2Nx8U vtail;

      rsrc = OFFSET_PTR_2NX8U(psrc, -1, sstride, 0);
      valign a_load = IVP_LA2NX8U_PP(rsrc);
      IVP_LAV2NX8_XP(v3, a_load, rsrc, width - x + 2);
      IVP_LAV2NX8U_XP(vtail, a_load, rsrc,
                      width - x + 2 - 2 * XCHAL_IVPN_SIMD_WIDTH);

      IVP_DSEL2NX8I(v5, v4, vtail, v3, IVP_DSELI_8B_ROTATE_RIGHT_2_1);
      SORT_ROW_8U2(v3, v4, v5)
    }
    rsrc = psrc;
    rdst = pdst;

    sstride -= XT_MIN(4 * XCHAL_IVPN_SIMD_WIDTH, width - x + 2);
    dstride -= XT_MIN(2 * XCHAL_IVPN_SIMD_WIDTH, width - x);
    for (int y = 0; y < height; y++) {
      xb_vec2Nx8U vtail;
      valign a_load = IVP_LA2NX8U_PP(rsrc);
      IVP_LAV2NX8_XP(v6, a_load, rsrc, width - x + 2);
      IVP_LAV2NX8U_XP(vtail, a_load, rsrc,
                      width - x + 2 - 2 * XCHAL_IVPN_SIMD_WIDTH);
      rsrc = OFFSET_PTR_2NX8U(rsrc, 1, sstride, 0);

      IVP_DSEL2NX8I(v8, v7, vtail, v6, IVP_DSELI_8B_ROTATE_RIGHT_2_1);
      SORT_ROW_8U2(v6, v7, v8)

      xb_vec2Nx8 vtmp, vmed;
      GET_MEDIAN_8U2(v0, v1, v2, v3, v4, v5, v6, v7, v8, vtmp, vmed)

#if 0 // XCHAL_HAVE_VISION
            xb_vec2Nx24 w1, w2;
            w1 = IVP_CVT24U2NX16(IVP_MOVNX16_FROM2NX8U(v4), IVP_MOVNX16_FROM2NX8U(v3));
            w2 = IVP_CVT24U2NX16(0, IVP_MOVNX16_FROM2NX8U(v5));

            v0 = IVP_MOV2NX8U_FROMNX16(IVP_CVT16U2NX24L(w1));
            v1 = IVP_MOV2NX8U_FROMNX16(IVP_CVT16U2NX24H(w1));
            v2 = IVP_MOV2NX8U_FROMNX16(IVP_CVT16U2NX24L(w2));
#else
      v0 = v3;
      v1 = v4;
      v2 = v5;
#endif
      v3 = v6;
      v5 = v8;
      v4 = v7;

      IVP_SAV2NX8U_XP(vmed, a_store, rdst, width - x);
      IVP_SAPOS2NX8U_FP(a_store, rdst);
      rdst = OFFSET_PTR_2NX8U(rdst, 1, dstride, 0);
    }
    sstride += XT_MIN(4 * XCHAL_IVPN_SIMD_WIDTH, width - x + 2);
    dstride += XT_MIN(2 * XCHAL_IVPN_SIMD_WIDTH, width - x);
  }

  return 0;
}
