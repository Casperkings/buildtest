#include "xi_lib.h"

#include <xtensa/tie/xt_ivpn.h>
#include <xtensa/tie/xt_misc.h>

#include <math.h>
//#include "xi_intrin.h"
//#include "xi_wide_arithm.h"
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

#define IVP_SAVNX8UPOS_FP  IVP_SAPOSNX8U_FP
#define IVP_SAV2NX8UPOS_FP IVP_SAPOS2NX8U_FP
//#include "xt_ivpn_plus.h"

#define FASTPATH_SMALL_KERNELS

#define Q15_1div9  3641
#define Q22_1div9  466034
#define Q17_1div25 5243
#define Q24_1div25 671089

#define Q15_1div25     1311
#define Q5_10_128div25 5243

#if !XCHAL_HAVE_VISION
#define IVP_CVT32SNX48L(w)  IVP_MOVN_2X32_FROMNX16(IVP_MOVSVWL(w))
#define IVP_CVT32SNX48H(w)  IVP_MOVN_2X32_FROMNX16(IVP_MOVSVWH(w))
#define IVP_CVT16U2NX24L(w) IVP_MOVV2WL(IVP_MOVNX48_FROM2NX24(w))
#define IVP_CVT16U2NX24H(w) IVP_MOVV2WH(IVP_MOVNX48_FROM2NX24(w))
#endif

#define XI_DIVIDE_BY_9_S19(out, in_h, in_l)                                    \
  {                                                                            \
    out = IVP_ADDNX16(Q15_1div9 - 9, IVP_MULNX16PACKL(in_h, Q15_1div9 * 2));   \
    IVP_MULANX16PACKQ(out, Q15_1div9, IVP_SUBNX16(in_l, 1 << 15));             \
    xb_vecNx16 tmp = IVP_SUBNX16(in_l, IVP_MULNX16PACKL(out, 9));              \
    IVP_MULANX16PACKQ(out, tmp, Q15_1div9);                                    \
  }

#define XI_DIVIDE_BY_25_S20(out, in_h, in_l)                                   \
  {                                                                            \
    out =                                                                      \
        IVP_ADDNX16(Q15_1div25 - 25, IVP_MULNX16PACKL(in_h, Q15_1div25 * 2));  \
    IVP_MULANX16PACKQ(out, Q15_1div25, IVP_SUBNX16(in_l, 1 << 15));            \
    xb_vecNx16 tmp = IVP_SUBNX16(in_l, IVP_MULNX16PACKL(out, 25));             \
    IVP_MULANX16PACKQ(out, tmp, Q15_1div25);                                   \
  }

#ifndef XI_ADDRESS_TRANSLATION
#define xiBoxFilter_LVNX16_X(addr, row, stride, offset)                        \
  IVP_LVNX16_X(addr, 2 * ((row) * (stride) + offset))
#else
#define xiBoxFilter_LVNX16_X(addr, row, stride, offset)                        \
  IVP_LVNX16_I(OFFSET_PTR_NX16(addr, row, stride, offset), 0)
#endif

#define XI__SEL2NX8I(tail, body, unused)                                       \
  IVP_MOV2NX8U_FROMNX16(IVP_SELNX16I(IVP_MOVNX16_FROM2NX8U(tail),              \
                                     IVP_MOVNX16_FROM2NX8U(body),              \
                                     IVP_SELI_ROTATE_RIGHT_1))

XI_ERR_TYPE xiBoxFilter_3x3_U8(const xi_pTile src, xi_pArray dst) {

  int sstride = XI_TILE_GET_PITCH(src);
  int dstride = XI_ARRAY_GET_PITCH(dst);
  int width = XI_TILE_GET_WIDTH(src);
  int height = XI_TILE_GET_HEIGHT(src);

  xb_vec2Nx8U *psrc =
      OFFSET_PTR_2NX8U(XI_TILE_GET_DATA_PTR(src), -1, sstride, -1);
  xb_vecNx8U *pdst = (xb_vecNx8U *)XI_ARRAY_GET_DATA_PTR(dst);
  xb_vec2Nx8U *restrict rsrc;
  xb_vecNx8U *restrict rdst;

  valign a_st = IVP_ZALIGN();
  int j = 0;

#if XCHAL_HAVE_VISION
  xb_vec2Nx8U *restrict rdst2;
  for (; j < width - XCHAL_IVPN_SIMD_WIDTH;
       j += 2 * XCHAL_IVPN_SIMD_WIDTH, psrc += 1, pdst += 2) {
    xb_vecNx16 pp0, pp1;
    xb_vecNx16 qq0, qq1;
    int offs = sstride - XT_MIN(4 * XCHAL_IVPN_SIMD_WIDTH, width - j + 2);
    int offd = dstride - XT_MIN(2 * XCHAL_IVPN_SIMD_WIDTH, width - j);
    rsrc = psrc;
    rdst2 = (xb_vec2Nx8U *)pdst;

    { // row 0
      valign a_load = IVP_LA2NX8U_PP(rsrc);
      xb_vec2Nx8U vsel0;
      IVP_LAV2NX8U_XP(vsel0, a_load, rsrc, width - j + 2);
      xb_vec2Nx8U vtail;
      IVP_LAV2NX8U_XP(vtail, a_load, rsrc,
                      width - j + 2 - 2 * XCHAL_IVPN_SIMD_WIDTH);
      rsrc = OFFSET_PTR_2NX8U(rsrc, 1, offs, 0);

#if XCHAL_VISION_TYPE == 6
      xb_vec2Nx24 w = IVP_MULUS4T2N8XR8(vtail, vsel0, 0x00010101);
#else
      xb_vec2Nx8 vsel1, vsel2;
      IVP_DSEL2NX8I(vsel2, vsel1, vtail, vsel0, IVP_DSELI_8B_ROTATE_RIGHT_2_1);
      xb_vec2Nx24 w = IVP_ADDWU2NX8U(0, vsel0);
      IVP_ADDWUA2NX8U(w, vsel1, vsel2);
#endif
      pp0 = IVP_CVT16U2NX24L(w);
      qq0 = IVP_CVT16U2NX24H(w);
    }

    { // row 1
      valign a_load = IVP_LA2NX8U_PP(rsrc);
      xb_vec2Nx8U vsel0;
      IVP_LAV2NX8U_XP(vsel0, a_load, rsrc, width - j + 2);
      xb_vec2Nx8U vtail;
      IVP_LAV2NX8U_XP(vtail, a_load, rsrc,
                      width - j + 2 - 2 * XCHAL_IVPN_SIMD_WIDTH);
      rsrc = OFFSET_PTR_2NX8U(rsrc, 1, offs, 0);

#if XCHAL_VISION_TYPE == 6
      xb_vec2Nx24 w = IVP_MULUS4T2N8XR8(vtail, vsel0, 0x00010101);
#else
      xb_vec2Nx8 vsel1, vsel2;
      IVP_DSEL2NX8I(vsel2, vsel1, vtail, vsel0, IVP_DSELI_8B_ROTATE_RIGHT_2_1);
      xb_vec2Nx24 w = IVP_ADDWU2NX8U(0, vsel0);
      IVP_ADDWUA2NX8U(w, vsel1, vsel2);
#endif
      pp1 = IVP_CVT16U2NX24L(w);
      qq1 = IVP_CVT16U2NX24H(w);
    }

    for (int i = 0; i < height; i++) {
      valign a_load = IVP_LA2NX8U_PP(rsrc);
      xb_vec2Nx8U vsel0;
      IVP_LAV2NX8U_XP(vsel0, a_load, rsrc, width - j + 2);
      xb_vec2Nx8U vtail;
      IVP_LAV2NX8U_XP(vtail, a_load, rsrc,
                      width - j + 2 - 2 * XCHAL_IVPN_SIMD_WIDTH);
      rsrc = OFFSET_PTR_2NX8U(rsrc, 1, offs, 0);

#if XCHAL_VISION_TYPE == 6
      xb_vec2Nx24 w = IVP_MULUS4T2N8XR8(vtail, vsel0, 0x00010101);
#else
      xb_vec2Nx8 vsel1, vsel2;
      IVP_DSEL2NX8I(vsel2, vsel1, vtail, vsel0, IVP_DSELI_8B_ROTATE_RIGHT_2_1);
      xb_vec2Nx24 w = IVP_ADDWU2NX8U(0, vsel0);
      IVP_ADDWUA2NX8U(w, vsel1, vsel2);
#endif
      xb_vecNx16 pp2 = IVP_CVT16U2NX24L(w);
      xb_vecNx16 qq2 = IVP_CVT16U2NX24H(w);

      xb_vecNx16 vdstl =
          IVP_MULNX16PACKQ(Q15_1div9, IVP_ADDNX16(IVP_ADDNX16(pp1, pp0), pp2));
      xb_vecNx16 vdsth =
          IVP_MULNX16PACKQ(Q15_1div9, IVP_ADDNX16(IVP_ADDNX16(qq1, qq0), qq2));
      xb_vec2Nx8 vdst =
          IVP_SEL2NX8I(IVP_MOV2NX8_FROMNX16(vdsth), IVP_MOV2NX8_FROMNX16(vdstl),
                       IVP_SELI_8B_EXTRACT_1_OF_2_OFF_0);

      IVP_SAV2NX8U_XP(vdst, a_st, rdst2, width - j);
      IVP_SAV2NX8UPOS_FP(a_st, rdst2);
      rdst2 = OFFSET_PTR_2NX8U(rdst2, 1, offd, 0);

      pp0 = pp1;
      qq0 = qq1;
      pp1 = pp2;
      qq1 = qq2;
    }
  }

  if (j < width) {
    xb_vecNx16 pp0, pp1;

    int offs = sstride - (width - j + 2);
    int offd = dstride - (width - j);
    rsrc = psrc;
    rdst = pdst;
    { // row 0
      valign a_load = IVP_LA2NX8U_PP(rsrc);
      xb_vec2Nx8U vsel0;
      IVP_LAV2NX8U_XP(vsel0, a_load, rsrc, width - j + 2);
      rsrc = OFFSET_PTR_2NX8U(rsrc, 1, offs, 0);

#if XCHAL_VISION_TYPE == 6
      xb_vec2Nx24 w = IVP_MULUS4T2N8XR8(0, vsel0, 0x00010101);
#else
      xb_vec2Nx8U vsel1, vsel2;
      IVP_DSEL2NX8I(vsel2, vsel1, 0, vsel0, IVP_DSELI_8B_ROTATE_RIGHT_2_1);
      xb_vec2Nx24 w = IVP_ADDWU2NX8U(0, vsel0);
      IVP_ADDWUA2NX8U(w, vsel1, vsel2);
#endif
      pp0 = IVP_CVT16U2NX24L(w);
    }

    { // row 1
      valign a_load = IVP_LA2NX8U_PP(rsrc);
      xb_vec2Nx8U vsel0;
      IVP_LAV2NX8U_XP(vsel0, a_load, rsrc, width - j + 2);
      rsrc = OFFSET_PTR_2NX8U(rsrc, 1, offs, 0);

#if XCHAL_VISION_TYPE == 6
      xb_vec2Nx24 w = IVP_MULUS4T2N8XR8(0, vsel0, 0x00010101);
#else
      xb_vec2Nx8U vsel1, vsel2;
      IVP_DSEL2NX8I(vsel2, vsel1, 0, vsel0, IVP_DSELI_8B_ROTATE_RIGHT_2_1);
      xb_vec2Nx24 w = IVP_ADDWU2NX8U(0, vsel0);
      IVP_ADDWUA2NX8U(w, vsel1, vsel2);
#endif
      pp1 = IVP_CVT16U2NX24L(w);
    }

    for (int i = 0; i < height; i++) {
      xb_vec2Nx8U *nsrc = OFFSET_PTR_2NX8U(rsrc, 1, sstride, 0);
      valign a_load = IVP_LA2NX8U_PP(rsrc);
      xb_vec2Nx8U vsel0;
      IVP_LAV2NX8U_XP(vsel0, a_load, rsrc, width - j + 2);
      rsrc = nsrc;

#if XCHAL_VISION_TYPE == 6
      xb_vec2Nx24 w = IVP_MULUS4T2N8XR8(0, vsel0, 0x00010101);
#else
      xb_vec2Nx8U vsel1, vsel2;
      IVP_DSEL2NX8I(vsel2, vsel1, 0, vsel0, IVP_DSELI_8B_ROTATE_RIGHT_2_1);
      xb_vec2Nx24 w = IVP_ADDWU2NX8U(0, vsel0);
      IVP_ADDWUA2NX8U(w, vsel1, vsel2);
#endif
      xb_vecNx16 pp2 = IVP_CVT16U2NX24L(w);

      IVP_SAVNX8U_XP(
          IVP_MULNX16PACKQ(Q15_1div9, IVP_ADDNX16(IVP_ADDNX16(pp0, pp1), pp2)),
          a_st, rdst, width - j);
      IVP_SAVNX8UPOS_FP(a_st, rdst);
      rdst = OFFSET_PTR_NX8U(rdst, 1, offd, 0);

      pp0 = pp1;
      pp1 = pp2;
    }
  }
#else
  for (; j < width; j += 2 * XCHAL_IVPN_SIMD_WIDTH, psrc += 1, pdst += 2) {
    xb_vecNx16 pp0, pp1;
    xb_vecNx16 qq0, qq1;

    int offs =
        sstride - 2 - width + XT_MAX(width - 4 * XCHAL_IVPN_SIMD_WIDTH + 2, j);
    int offd = dstride - width + XT_MAX(width - 2 * XCHAL_IVPN_SIMD_WIDTH, j);
    rsrc = psrc;
    rdst = pdst;

    { // row 0
      valign a_load = IVP_LA2NX8U_PP(rsrc);
      xb_vec2Nx8U vsel0;
      IVP_LAV2NX8U_XP(vsel0, a_load, rsrc, width - j + 2);
      xb_vec2Nx8U vtail;
      IVP_LAV2NX8U_XP(vtail, a_load, rsrc,
                      width - j + 2 - 2 * XCHAL_IVPN_SIMD_WIDTH);
      rsrc = OFFSET_PTR_2NX8U(rsrc, 1, offs, 0);

      xb_vec2Nx8 vsel1, vsel2;
      IVP_DSEL2NX8I(vsel2, vsel1, vtail, vsel0, IVP_DSELI_8B_ROTATE_RIGHT_2_1);
      // vsel1 = IVP_SEL2NX8I(vtail, vsel0, IVP_SELI_8B_ROTATE_RIGHT_1);
      // vsel2 = XI__SEL2NX8I(vtail, vsel0, IVP_SELI_8B_ROTATE_RIGHT_2);

      xb_vec2Nx24 w = IVP_MULUU2NX8(1, vsel0);
      IVP_MULUUA2NX8(w, 1, vsel1);
      IVP_MULUUA2NX8(w, 1, vsel2);

      qq0 = IVP_CVT16U2NX24H(w);
      pp0 = IVP_CVT16U2NX24L(w);
    }

    { // row 1
      valign a_load = IVP_LA2NX8U_PP(rsrc);
      xb_vec2Nx8U vsel0;
      IVP_LAV2NX8U_XP(vsel0, a_load, rsrc, width - j + 2);
      xb_vec2Nx8U vtail;
      IVP_LAV2NX8U_XP(vtail, a_load, rsrc,
                      width - j + 2 - 2 * XCHAL_IVPN_SIMD_WIDTH);
      rsrc = OFFSET_PTR_2NX8U(rsrc, 1, offs, 0);

      xb_vec2Nx8 vsel1, vsel2;
      IVP_DSEL2NX8I(vsel2, vsel1, vtail, vsel0, IVP_DSELI_8B_ROTATE_RIGHT_2_1);
      // vsel1 = IVP_SEL2NX8I(vtail, vsel0, IVP_SELI_8B_ROTATE_RIGHT_1);
      // vsel2 = XI__SEL2NX8I(vtail, vsel0, IVP_SELI_8B_ROTATE_RIGHT_2);

      xb_vec2Nx24 w = IVP_MULUU2NX8(1, vsel0);
      IVP_MULUUA2NX8(w, 1, vsel1);
      IVP_MULUUA2NX8(w, 1, vsel2);

      qq1 = IVP_CVT16U2NX24H(w);
      pp1 = IVP_CVT16U2NX24L(w);
      pp0 = IVP_ADDNX16(pp0, pp1);
      qq0 = IVP_ADDNX16(qq0, qq1);
    }

    for (int i = 0; i < height; i++) {
      valign a_load = IVP_LA2NX8U_PP(rsrc);
      xb_vec2Nx8U vsel0;
      IVP_LAV2NX8U_XP(vsel0, a_load, rsrc, width - j + 2);
      xb_vec2Nx8U vtail;
      IVP_LAV2NX8U_XP(vtail, a_load, rsrc,
                      width - j + 2 - 2 * XCHAL_IVPN_SIMD_WIDTH);
      rsrc = OFFSET_PTR_2NX8U(rsrc, 1, offs, 0);

      xb_vec2Nx8 vsel1, vsel2;
      IVP_DSEL2NX8I(vsel2, vsel1, vtail, vsel0, IVP_DSELI_8B_ROTATE_RIGHT_2_1);
      // vsel1 = IVP_SEL2NX8I(vtail, vsel0, IVP_SELI_8B_ROTATE_RIGHT_1);
      // vsel2 = XI__SEL2NX8I(vtail, vsel0, IVP_SELI_8B_ROTATE_RIGHT_2);

      xb_vec2Nx24 w = IVP_MULUU2NX8(1, vsel0);
      IVP_MULUUA2NX8(w, 1, vsel1);
      IVP_MULUUA2NX8(w, 1, vsel2);

      xb_vecNx16 qq = IVP_CVT16U2NX24H(w);
      xb_vecNx16 pp = IVP_CVT16U2NX24L(w);

      IVP_SAVNX8U_XP(IVP_MULNX16PACKQ(IVP_ADDNX16(pp0, pp), Q15_1div9), a_st,
                     rdst, width - j);
      IVP_SAVNX8U_XP(IVP_MULNX16PACKQ(IVP_ADDNX16(qq0, qq), Q15_1div9), a_st,
                     rdst, width - j - XCHAL_IVPN_SIMD_WIDTH);
      IVP_SAVNX8UPOS_FP(a_st, rdst);
      rdst = OFFSET_PTR_NX8U(rdst, 1, offd, 0);

      pp0 = IVP_ADDNX16(pp1, pp);
      qq0 = IVP_ADDNX16(qq1, qq);
      pp1 = pp;
      qq1 = qq;
    }
  }
#endif

  return 0;
}