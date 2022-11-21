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

// Sobel 5x5 filters
//
//        |-1  -2   0   2   1|        |-1  -4  -6  -4  -1|
//        |-4  -8   0   8   4|        |-2  -8 -12  -8  -2|
//   Gx = |-6 -12   0  12   6|   Gy = | 0   0   0   0   0|
//        |-4  -8   0   8   4|        | 2   8  12   8   2|
//        |-1  -2   0   2   1|        | 1   4   6   4   1|

void xiSobel_5x5_U8S16_unnormalized(const xi_pTile src, xi_pArray dst_dx,
                                    xi_pArray dst_dy) {
  int sstride = XI_TILE_GET_PITCH(src) - 4;
  int dstride_dx = XI_ARRAY_GET_PITCH(dst_dx);
  int dstride_dy = XI_ARRAY_GET_PITCH(dst_dy);
  int width = XI_TILE_GET_WIDTH(src);
  int height = XI_TILE_GET_HEIGHT(src);

  xb_vec2Nx8U *restrict psrc =
      OFFSET_PTR_2NX8U(XI_TILE_GET_DATA_PTR(src), -2, sstride + 4, -2);
  xb_vecNx16 *restrict pdst_dx = (xb_vecNx16 *)XI_ARRAY_GET_DATA_PTR(dst_dx);
  xb_vecNx16 *restrict pdst_dy = (xb_vecNx16 *)XI_ARRAY_GET_DATA_PTR(dst_dy);
  xb_vec2Nx8U *restrict rsrc;

  xb_vecNx16 *restrict rdst_dx;
  xb_vecNx16 *restrict rdst_dy;

  valign a_store_dx = IVP_ZALIGN();
  valign a_store_dy = IVP_ZALIGN();

  int j = 0;
  for (; j < width - XCHAL_IVPN_SIMD_WIDTH;
       j += 2 * XCHAL_IVPN_SIMD_WIDTH, psrc += 1, pdst_dx += 2, pdst_dy += 2) {
    xb_vecNx16 pp0l, pp1l, pp2l, pp3l;
    xb_vecNx16 qq0l, qq1l, qq2l, qq3l;

    xb_vecNx16 pp0h, pp1h, pp2h, pp3h;
    xb_vecNx16 qq0h, qq1h, qq2h, qq3h;

    int offs =
        sstride - width + XT_MAX(j, width + 4 - 4 * XCHAL_IVPN_SIMD_WIDTH);
    int offx =
        dstride_dx - width + XT_MAX(j, width - 2 * XCHAL_IVPN_SIMD_WIDTH);
    int offy =
        dstride_dy - width + XT_MAX(j, width - 2 * XCHAL_IVPN_SIMD_WIDTH);
    rsrc = psrc;
    rdst_dx = pdst_dx;
    rdst_dy = pdst_dy;

    { // row 0
      valign a_load = IVP_LA2NX8U_PP(rsrc);
      xb_vec2Nx8U vsel0;
      IVP_LAV2NX8U_XP(vsel0, a_load, rsrc, width - j + 4);
      xb_vec2Nx8U vtail;
      IVP_LAV2NX8U_XP(vtail, a_load, rsrc,
                      width - j + 4 - 2 * XCHAL_IVPN_SIMD_WIDTH);
      rsrc = OFFSET_PTR_2NX8U(rsrc, 1, offs, 0);
      xb_vec2Nx8U vsel5 =
          IVP_SEL2NX8I(vtail, vsel0, IVP_SELI_8B_ROTATE_RIGHT_4);

      // 1 0 6 0 1
      xb_vec2Nx24 wpp = IVP_MULUS4T2N8XR8(vtail, vsel0, 0x04060401);
      IVP_ADDWUA2NX8(wpp, 0, vsel5);
      pp0l = IVP_CVT16U2NX24L(wpp);
      pp0h = IVP_CVT16U2NX24H(wpp);

      // -1 -2 0 2 1
      xb_vec2Nx24 wqq = IVP_MULUS4T2N8XR8(vtail, vsel0, 0x0200FEFF);
      IVP_ADDWUA2NX8(wqq, 0, vsel5);
      qq0l = IVP_CVT16S2NX24L(wqq);
      qq0h = IVP_CVT16S2NX24H(wqq);
    }

    { // row 1
      valign a_load = IVP_LA2NX8U_PP(rsrc);
      xb_vec2Nx8U vsel1;
      IVP_LAV2NX8U_XP(vsel1, a_load, rsrc, width - j + 4);
      xb_vec2Nx8U vtail;
      IVP_LAV2NX8U_XP(vtail, a_load, rsrc,
                      width - j + 4 - 2 * XCHAL_IVPN_SIMD_WIDTH);
      rsrc = OFFSET_PTR_2NX8U(rsrc, 1, offs, 0);
      xb_vec2Nx8U vsel5 =
          IVP_SEL2NX8I(vtail, vsel1, IVP_SELI_8B_ROTATE_RIGHT_4);

      // 1 1 6 1 1
      xb_vec2Nx24 wpp = IVP_MULUS4T2N8XR8(vtail, vsel1, 0x04060401);
      IVP_ADDWUA2NX8(wpp, 0, vsel5);
      pp1l = IVP_CVT16U2NX24L(wpp);
      pp1h = IVP_CVT16U2NX24H(wpp);

      // -1 -2 0 2 1
      xb_vec2Nx24 wqq = IVP_MULUS4T2N8XR8(vtail, vsel1, 0x0200FEFF);
      IVP_ADDWUA2NX8(wqq, 0, vsel5);
      qq1l = IVP_CVT16S2NX24L(wqq);
      qq1h = IVP_CVT16S2NX24H(wqq);
    }

    { // row 2
      valign a_load = IVP_LA2NX8U_PP(rsrc);
      xb_vec2Nx8U vsel2;
      IVP_LAV2NX8U_XP(vsel2, a_load, rsrc, width - j + 4);
      xb_vec2Nx8U vtail;
      IVP_LAV2NX8U_XP(vtail, a_load, rsrc,
                      width - j + 4 - 2 * XCHAL_IVPN_SIMD_WIDTH);
      rsrc = OFFSET_PTR_2NX8U(rsrc, 1, offs, 0);
      xb_vec2Nx8U vsel5 =
          IVP_SEL2NX8I(vtail, vsel2, IVP_SELI_8B_ROTATE_RIGHT_4);

      // 1 2 6 2 1
      xb_vec2Nx24 wpp = IVP_MULUS4T2N8XR8(vtail, vsel2, 0x04060401);
      IVP_ADDWUA2NX8(wpp, 0, vsel5);
      pp2l = IVP_CVT16U2NX24L(wpp);
      pp2h = IVP_CVT16U2NX24H(wpp);

      // -1 -2 0 2 1
      xb_vec2Nx24 wqq = IVP_MULUS4T2N8XR8(vtail, vsel2, 0x0200FEFF);
      IVP_ADDWUA2NX8(wqq, 0, vsel5);
      qq2l = IVP_CVT16S2NX24L(wqq);
      qq2h = IVP_CVT16S2NX24H(wqq);
    }

    { // row 3
      valign a_load = IVP_LA2NX8U_PP(rsrc);
      xb_vec2Nx8U vsel3;
      IVP_LAV2NX8U_XP(vsel3, a_load, rsrc, width - j + 4);
      xb_vec2Nx8U vtail;
      IVP_LAV2NX8U_XP(vtail, a_load, rsrc,
                      width - j + 4 - 2 * XCHAL_IVPN_SIMD_WIDTH);
      rsrc = OFFSET_PTR_2NX8U(rsrc, 1, offs, 0);
      xb_vec2Nx8U vsel5 =
          IVP_SEL2NX8I(vtail, vsel3, IVP_SELI_8B_ROTATE_RIGHT_4);

      // 1 3 6 3 1
      xb_vec2Nx24 wpp = IVP_MULUS4T2N8XR8(vtail, vsel3, 0x04060401);
      IVP_ADDWUA2NX8(wpp, 0, vsel5);
      pp3l = IVP_CVT16U2NX24L(wpp);
      pp3h = IVP_CVT16U2NX24H(wpp);

      // -1 -2 0 2 1
      xb_vec2Nx24 wqq = IVP_MULUS4T2N8XR8(vtail, vsel3, 0x0200FEFF);
      IVP_ADDWUA2NX8(wqq, 0, vsel5);
      qq3l = IVP_CVT16S2NX24L(wqq);
      qq3h = IVP_CVT16S2NX24H(wqq);
    }

    for (int i = 0; i < height; i += 1) {
      valign a_load = IVP_LA2NX8U_PP(rsrc);
      xb_vec2Nx8U vsel4;
      IVP_LAV2NX8U_XP(vsel4, a_load, rsrc, width - j + 4);
      xb_vec2Nx8U vtail;
      IVP_LAV2NX8U_XP(vtail, a_load, rsrc,
                      width - j + 4 - 2 * XCHAL_IVPN_SIMD_WIDTH);
      xb_vec2Nx8U vsel5 =
          IVP_SEL2NX8I(vtail, vsel4, IVP_SELI_8B_ROTATE_RIGHT_4);

      // 1 4 6 4 1
      xb_vec2Nx24 wpp = IVP_MULUS4T2N8XR8(vtail, vsel4, 0x04060401);
      IVP_ADDWUA2NX8(wpp, 0, vsel5);
      xb_vecNx16 pp4l = IVP_CVT16U2NX24L(wpp);
      xb_vecNx16 pp4h = IVP_CVT16U2NX24H(wpp);

      // -1 -2 0 2 1
      xb_vec2Nx24 wqq = IVP_MULUS4T2N8XR8(vtail, vsel4, 0x0200FEFF);
      IVP_ADDWUA2NX8(wqq, 0, vsel5);
      xb_vecNx16 qq4l = IVP_CVT16S2NX24L(wqq);
      xb_vecNx16 qq4h = IVP_CVT16S2NX24H(wqq);

      xb_vecNx16 presl = IVP_SUBNX16(
          pp4l, IVP_SUBNX16(pp0l, IVP_SLLINX16(IVP_SUBNX16(pp3l, pp1l), 1)));
      xb_vecNx16 presh = IVP_SUBNX16(
          pp4h, IVP_SUBNX16(pp0h, IVP_SLLINX16(IVP_SUBNX16(pp3h, pp1h), 1)));

      wqq = IVP_MULI2NX8X16(1, qq0h, qq0l);
      IVP_MULAI2NX8X16(wqq, 4, qq1h, qq1l);
      IVP_MULAI2NX8X16(wqq, 6, qq2h, qq2l);
      IVP_MULAI2NX8X16(wqq, 4, qq3h, qq3l);
      xb_vecNx16 qresl = IVP_ADDNX16(qq4l, IVP_PACKVR2NX24_0(wqq, 0));
      xb_vecNx16 qresh = IVP_ADDNX16(qq4h, IVP_PACKVR2NX24_1(wqq, 0));

      IVP_SAVNX16_XP(qresl, a_store_dx, rdst_dx, 2 * (width - j));
      IVP_SAVNX16_XP(qresh, a_store_dx, rdst_dx,
                     2 * (width - j) - 2 * XCHAL_IVPN_SIMD_WIDTH);
      IVP_SAVNX16POS_FP(a_store_dx, rdst_dx);

      IVP_SAVNX16_XP(presl, a_store_dy, rdst_dy, 2 * (width - j));
      IVP_SAVNX16_XP(presh, a_store_dy, rdst_dy,
                     2 * (width - j) - 2 * XCHAL_IVPN_SIMD_WIDTH);
      IVP_SAVNX16POS_FP(a_store_dy, rdst_dy);

      rsrc = OFFSET_PTR_2NX8U(rsrc, 1, offs, 0);
      rdst_dx = OFFSET_PTR_NX16(rdst_dx, 1, offx, 0);
      rdst_dy = OFFSET_PTR_NX16(rdst_dy, 1, offy, 0);

      qq0l = qq1l;
      qq1l = qq2l;
      qq2l = qq3l;
      qq3l = qq4l;
      pp0l = pp1l;
      pp1l = pp2l;
      pp2l = pp3l;
      pp3l = pp4l;

      qq0h = qq1h;
      qq1h = qq2h;
      qq2h = qq3h;
      qq3h = qq4h;
      pp0h = pp1h;
      pp1h = pp2h;
      pp2h = pp3h;
      pp3h = pp4h;
    }
  }

  if (j < width) {
    xb_vecNx16 pp0l, pp1l, pp2l, pp3l;
    xb_vecNx16 qq0l, qq1l, qq2l, qq3l;

    int offs = sstride - width + j;
    int offx = dstride_dx - width + j;
    int offy = dstride_dy - width + j;

    rsrc = psrc;
    rdst_dx = pdst_dx;
    rdst_dy = pdst_dy;

    { // row 0
      valign a_load = IVP_LA2NX8U_PP(rsrc);
      xb_vec2Nx8U vsel0;
      IVP_LAV2NX8U_XP(vsel0, a_load, rsrc, width - j + 4);
      rsrc = OFFSET_PTR_2NX8U(rsrc, 1, offs, 0);
      xb_vec2Nx8U vsel5 = IVP_SEL2NX8I(0, vsel0, IVP_SELI_8B_ROTATE_RIGHT_4);

      // 1 0 6 0 1
      xb_vec2Nx24 wpp = IVP_MULUS4T2N8XR8(0, vsel0, 0x04060401);
      IVP_ADDWUA2NX8(wpp, 0, vsel5);
      pp0l = IVP_CVT16U2NX24L(wpp);

      // -1 -2 0 2 1
      xb_vec2Nx24 wqq = IVP_MULUS4T2N8XR8(0, vsel0, 0x0200FEFF);
      IVP_ADDWUA2NX8(wqq, 0, vsel5);
      qq0l = IVP_CVT16S2NX24L(wqq);
    }

    { // row 1
      valign a_load = IVP_LA2NX8U_PP(rsrc);
      xb_vec2Nx8U vsel1;
      IVP_LAV2NX8U_XP(vsel1, a_load, rsrc, width - j + 4);
      rsrc = OFFSET_PTR_2NX8U(rsrc, 1, offs, 0);
      xb_vec2Nx8U vsel5 = IVP_SEL2NX8I(0, vsel1, IVP_SELI_8B_ROTATE_RIGHT_4);

      // 1 1 6 1 1
      xb_vec2Nx24 wpp = IVP_MULUS4T2N8XR8(0, vsel1, 0x04060401);
      IVP_ADDWUA2NX8(wpp, 0, vsel5);
      pp1l = IVP_CVT16U2NX24L(wpp);

      // -1 -2 0 2 1
      xb_vec2Nx24 wqq = IVP_MULUS4T2N8XR8(0, vsel1, 0x0200FEFF);
      IVP_ADDWUA2NX8(wqq, 0, vsel5);
      qq1l = IVP_CVT16S2NX24L(wqq);
    }

    { // row 2
      valign a_load = IVP_LA2NX8U_PP(rsrc);
      xb_vec2Nx8U vsel2;
      IVP_LAV2NX8U_XP(vsel2, a_load, rsrc, width - j + 4);
      rsrc = OFFSET_PTR_2NX8U(rsrc, 1, offs, 0);
      xb_vec2Nx8U vsel5 = IVP_SEL2NX8I(0, vsel2, IVP_SELI_8B_ROTATE_RIGHT_4);

      // 1 2 6 2 1
      xb_vec2Nx24 wpp = IVP_MULUS4T2N8XR8(0, vsel2, 0x04060401);
      IVP_ADDWUA2NX8(wpp, 0, vsel5);
      pp2l = IVP_CVT16U2NX24L(wpp);

      // -1 -2 0 2 1
      xb_vec2Nx24 wqq = IVP_MULUS4T2N8XR8(0, vsel2, 0x0200FEFF);
      IVP_ADDWUA2NX8(wqq, 0, vsel5);
      qq2l = IVP_CVT16S2NX24L(wqq);
    }

    { // row 3
      valign a_load = IVP_LA2NX8U_PP(rsrc);
      xb_vec2Nx8U vsel3;
      IVP_LAV2NX8U_XP(vsel3, a_load, rsrc, width - j + 4);
      xb_vec2Nx8U vsel5 = IVP_SEL2NX8I(0, vsel3, IVP_SELI_8B_ROTATE_RIGHT_4);

      // 1 3 6 3 1
      xb_vec2Nx24 wpp = IVP_MULUS4T2N8XR8(0, vsel3, 0x04060401);
      IVP_ADDWUA2NX8(wpp, 0, vsel5);
      pp3l = IVP_CVT16U2NX24L(wpp);

      // -1 -2 0 2 1
      xb_vec2Nx24 wqq = IVP_MULUS4T2N8XR8(0, vsel3, 0x0200FEFF);
      IVP_ADDWUA2NX8(wqq, 0, vsel5);
      qq3l = IVP_CVT16S2NX24L(wqq);
    }

    for (int i = 0; i < height; i += 1) {
      rsrc = OFFSET_PTR_2NX8U(rsrc, 1, offs, 0);
      valign a_load = IVP_LA2NX8U_PP(rsrc);
      xb_vec2Nx8U vsel4;
      IVP_LAV2NX8U_XP(vsel4, a_load, rsrc, width - j + 4);
      xb_vec2Nx8U vsel5 = IVP_SEL2NX8I(0, vsel4, IVP_SELI_8B_ROTATE_RIGHT_4);

      // 1 4 6 4 1
      xb_vec2Nx24 wpp = IVP_MULUS4T2N8XR8(0, vsel4, 0x04060401);
      IVP_ADDWUA2NX8(wpp, 0, vsel5);
      xb_vecNx16 pp4l = IVP_CVT16U2NX24L(wpp);

      // -1 -2 0 2 1
      xb_vec2Nx24 wqq = IVP_MULUS4T2N8XR8(0, vsel4, 0x0200FEFF);
      IVP_ADDWUA2NX8(wqq, 0, vsel5);
      xb_vecNx16 qq4l = IVP_CVT16S2NX24L(wqq);

      xb_vecNx16 presl = IVP_SUBNX16(
          pp4l, IVP_SUBNX16(pp0l, IVP_SLLINX16(IVP_SUBNX16(pp3l, pp1l), 1)));

      xb_vecNx16 qresl = IVP_ADDNX16(qq1l, qq2l);
      qresl = IVP_ADDNX16(qresl, qq3l);
      qresl = IVP_ADDNX16(qresl << 1, qq2l);
      qresl = IVP_ADDNX16(qresl << 1, qq0l);
      qresl = IVP_ADDNX16(qresl, qq4l);

      IVP_SAVNX16_XP(qresl, a_store_dx, rdst_dx, 2 * (width - j));
      IVP_SAVNX16POS_FP(a_store_dx, rdst_dx);

      IVP_SAVNX16_XP(presl, a_store_dy, rdst_dy, 2 * (width - j));
      IVP_SAVNX16POS_FP(a_store_dy, rdst_dy);

      rdst_dx = OFFSET_PTR_NX16(rdst_dx, 1, offx, 0);
      rdst_dy = OFFSET_PTR_NX16(rdst_dy, 1, offy, 0);

      qq0l = qq1l;
      qq1l = qq2l;
      qq2l = qq3l;
      qq3l = qq4l;
      pp0l = pp1l;
      pp1l = pp2l;
      pp2l = pp3l;
      pp3l = pp4l;
    }
  }
}

XI_ERR_TYPE xiSobel_5x5_U8S16(const xi_pTile src, xi_pArray dst_dx,
                              xi_pArray dst_dy, xi_bool normalize) {
  // XI_ERROR_CHECKS()
  //{
  //    XI_CHECK_TILE_U8(src);
  //    XI_CHECK_TILE_EDGE(src, 2);
  //    XI_CHECK_ARRAY_S16(dst_dx);
  //    XI_CHECK_ARRAY_S16(dst_dy);
  //    XI_CHECK_ARRAY_SIZE_EQ(src, dst_dx);
  //    XI_CHECK_ARRAY_SIZE_EQ(src, dst_dy);
  //}

  xiSobel_5x5_U8S16_unnormalized(src, dst_dx, dst_dy);

  return 0; // XI_ERROR_STATUS();
}
