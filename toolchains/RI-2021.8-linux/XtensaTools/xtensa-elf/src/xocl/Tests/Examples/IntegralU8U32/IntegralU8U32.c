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
#define IVP_SAVNX16POS_FP IVP_SAPOSNX16_FP
#define IVP_SANX16POS_FP  IVP_SAPOSNX16_FP

// select/shuffle indexes
#define XI_DSEL_16B_ROTATE_LEFT(n)                                             \
  IVP_AVGU2NX8(IVP_SEQ2NX8(),                                                  \
               IVP_MOV2NX8_FROMNX16((0x4000 - (2 * (((n) << 8) + (n))))))
#define XI_DSEL_16B_ROTATE_RIGHT(n)                                            \
  IVP_AVGU2NX8(IVP_SEQ2NX8(),                                                  \
               IVP_MOV2NX8_FROMNX16((0x3F00 + (2 * (((n) << 8) + (n))))))

#define XI_DSEL_16B_ROTATE_RIGHT_2_1                                           \
  IVP_AVGU2NX8(IVP_SEQ2NX8(), IVP_MOV2NX8_FROMNX16(2 * (1 + ((1 + 1) << 8))))
#define XI_DSEL_16B_ROTATE_RIGHT_4_3                                           \
  IVP_AVGU2NX8(IVP_SEQ2NX8(), IVP_MOV2NX8_FROMNX16(2 * (3 + ((3 + 1) << 8))))
XI_ERR_TYPE xiIntegral_U8U32(xi_array const *src, xi_array const *dst) {
  const int32_t sstride = XI_ARRAY_GET_PITCH(src);
  const int32_t dstride = XI_ARRAY_GET_PITCH(dst);
  const int32_t width = XI_ARRAY_GET_WIDTH(src);
  const uint16_t height = XI_ARRAY_GET_HEIGHT(src);

  uint8_t *psrc = (uint8_t *)XI_ARRAY_GET_DATA_PTR(src);
  uint32_t *pdst = (uint32_t *)XI_ARRAY_GET_DATA_PTR(dst);

  xb_vecNx8U *restrict vpsrc;
  xb_vecNx16 *restrict vpdst;

  valign a_st = IVP_ZALIGN();
  int32_t x = ((width - 1) & ((2 * XCHAL_IVPN_SIMD_WIDTH) - 1)) + 1;

  xb_vec2Nx8U *restrict vpsrc2;

  xb_vec2Nx8 l4 = XI_DSEL_16B_ROTATE_LEFT(4);
  xb_vec2Nx8 l16 = XI_DSEL_16B_ROTATE_LEFT(16);

  vboolN b4 = IVP_NOTBN(IVP_LTRNI(4));
  vboolN b16 = IVP_NOTBN(IVP_LTRNI(16));
  xb_vec2Nx8 rotate = IVP_ADD2NX8(61, IVP_SEQ2NX8U());
  xb_vec2Nx8 rotate3 = IVP_SUB2NX8(IVP_SEQ2NX8U(), 3);
  vpdst = (xb_vecNx16 *)pdst;

  if (x <= XCHAL_IVPN_SIMD_WIDTH) {
    vpsrc = (xb_vecNx8U *)psrc;

    // compute "integral image" for the first x <= 32 columns
    xb_vecNx48 column_carry = IVP_ZERONX48();
    for (int32_t y = 0; y < height; y++) {
      valign a_ld = IVP_LANX8U_PP(vpsrc);
      xb_vecNx16U v0;
      IVP_LAVNX8U_XP(v0, a_ld, vpsrc, x);
      vpsrc = OFFSET_PTR_NX8U(vpsrc, 1, sstride - x, 0);

      xb_vecNx16 v1 = IVP_SELNX16I(v0, 0, IVP_SELI_ROTATE_LEFT_1);
      xb_vecNx16 v2 = IVP_SELNX16I(v0, 0, IVP_SELI_ROTATE_LEFT_2);
      xb_vecNx16 v3 = IVP_SELNX16I(v0, 0, IVP_SELI_ROTATE_LEFT_3);

      xb_vecNx48 w = IVP_ADDWNX16(v0, v1);
      IVP_ADDWANX16(w, v3, v2);
      v3 = IVP_PACKLNX48(w);

      xb_vecNx16 v4 =
          IVP_ADDNX16(v3, IVP_SELNX16I(v3, 0, IVP_SELI_ROTATE_LEFT_4));
      xb_vecNx16 v5 =
          IVP_ADDNX16(v4, IVP_SELNX16I(v4, 0, IVP_SELI_ROTATE_LEFT_8));
      IVP_ADDWANX16(column_carry, v5,
                    IVP_SELNX16I(v5, 0, IVP_SELI_ROTATE_LEFT_16));

      IVP_SAVNX16_XP(IVP_MOVNX16_FROMN_2X32(IVP_CVT32UNX48L(column_carry)),
                     a_st, vpdst, 4 * x);
      IVP_SAVNX16_XP(IVP_MOVNX16_FROMN_2X32(IVP_CVT32UNX48H(column_carry)),
                     a_st, vpdst, 4 * x - 2 * XCHAL_IVPN_SIMD_WIDTH);
      IVP_SAVNX16POS_FP(a_st, vpdst);
      vpdst = OFFSET_PTR_NX16(vpdst, 1, 2 * (dstride - x), 0);
    }
  } else {
    vpsrc2 = (xb_vec2Nx8U *)psrc;

    // compute "integral image" for the first 32 < x <= 64 columns
    xb_vecNx48 column_carry_lo = IVP_ZERONX48();
    xb_vecNx48 column_carry_hi = IVP_ZERONX48();

    for (int32_t y = 0; y < height; y++) {
      valign a_ld = IVP_LA2NX8U_PP(vpsrc2);
      xb_vec2Nx8U v0;
      IVP_LAV2NX8U_XP(v0, a_ld, vpsrc2, x);
      vpsrc2 = OFFSET_PTR_2NX8U(vpsrc2, 1, sstride - x, 0);

      xb_vec2Nx8U vs = IVP_SEL2NX8(0, v0, rotate3);
      xb_vec2Nx8U vr = IVP_SEL2NX8(0, v0, rotate);
      xb_vec2Nx24 w = IVP_MULUS4T2N8XR8(vr, vs, 0x01010101);

      xb_vecNx16 vs0 = IVP_CVT16S2NX24L(w);
      xb_vecNx16 vs1 = IVP_CVT16S2NX24H(w);

      xb_vecNx16 v4_0, v4_1;
      v4_0 = IVP_SELNX16I(vs0, 0, IVP_SELI_ROTATE_LEFT_4);
      v4_1 = IVP_SELNX16I(vs1, vs0, IVP_SELI_ROTATE_LEFT_4);
      vs0 = IVP_ADDNX16(v4_0, vs0);
      vs1 = IVP_ADDNX16(v4_1, vs1);

      xb_vecNx16 v8_0, v8_1;
      v8_0 = IVP_SELNX16I(vs0, 0, IVP_SELI_ROTATE_LEFT_8);
      v8_1 = IVP_SELNX16I(vs1, vs0, IVP_SELI_ROTATE_LEFT_8);
      vs0 = IVP_ADDNX16(v8_0, vs0);
      vs1 = IVP_ADDNX16(v8_1, vs1);

      xb_vecNx16 v16_0, v16_1;
      IVP_DSELNX16(v16_1, v16_0, vs1, vs0, l16);
      IVP_ADDNX16T(vs0, v16_0, vs0, b16);
      vs1 = IVP_ADDNX16(v16_1, vs1);

      IVP_ADDWUANX16(column_carry_hi, vs1, vs0);

      IVP_MULUUANX16(column_carry_lo, 1, vs0);

      IVP_SANX16_IP(IVP_MOVNX16_FROMN_2X32(IVP_CVT32UNX48L(column_carry_lo)),
                    a_st, vpdst);
      IVP_SANX16_IP(IVP_MOVNX16_FROMN_2X32(IVP_CVT32UNX48H(column_carry_lo)),
                    a_st, vpdst);
      IVP_SAVNX16_XP(IVP_MOVNX16_FROMN_2X32(IVP_CVT32UNX48L(column_carry_hi)),
                     a_st, vpdst, 4 * x - 4 * XCHAL_IVPN_SIMD_WIDTH);
      IVP_SAVNX16_XP(IVP_MOVNX16_FROMN_2X32(IVP_CVT32UNX48H(column_carry_hi)),
                     a_st, vpdst, 4 * x - 6 * XCHAL_IVPN_SIMD_WIDTH);
      IVP_SAVNX16POS_FP(a_st, vpdst);
      vpdst = OFFSET_PTR_NX16(vpdst, 1, 2 * (dstride - x), 0);
    }
  }

  // repeat the process for remaining columns (64 at time) with difference
  // in adding carry from previously computed columns
  for (; x < width; x += 2 * XCHAL_IVPN_SIMD_WIDTH) {
    vpsrc2 = OFFSET_PTR_2NX8U(psrc, 0, 0, x);
    vpdst = OFFSET_PTR_NX16(pdst, 0, 0, x * 2);

    xb_vecNx48 column_carry_lo = IVP_ZERONX48();
    xb_vecNx48 column_carry_hi = IVP_ZERONX48();

    for (int32_t y = 0; y < height; y++) {
      xb_vecN_2x32v row_carry =
          IVP_LSRN_2X32_I((int32_t *)vpdst, -(int32_t)(sizeof(int32_t)));

      valign a_ld = IVP_LA2NX8U_PP(vpsrc2);
      xb_vec2Nx8U v0;
      IVP_LA2NX8U_IP(v0, a_ld, vpsrc2);
      vpsrc2 =
          OFFSET_PTR_2NX8U(vpsrc2, 1, sstride - (2 * XCHAL_IVPN_SIMD_WIDTH), 0);

      xb_vec2Nx8U v1;
      v1 = IVP_SEL2NX8I(v0, 0, IVP_SELI_8B_ROTATE_LEFT_1);
      xb_vec2Nx8U v2;
      v2 = IVP_SEL2NX8I(v0, 0, IVP_SELI_8B_ROTATE_LEFT_2);
      xb_vec2Nx8U v3;
      v3 = IVP_SEL2NX8I(v2, 0, IVP_SELI_8B_ROTATE_LEFT_1);

      xb_vec2Nx24 w = IVP_ADDWU2NX8U(v0, v1);
      IVP_ADDWUA2NX8U(w, v3, v2);

      xb_vecNx16 vs0 = IVP_CVT16S2NX24L(w);
      xb_vecNx16 vs1 = IVP_CVT16S2NX24H(w);

      xb_vecNx16 v4_0, v4_1;
      IVP_DSELNX16(v4_1, v4_0, vs1, vs0, l4);
      IVP_ADDNX16T(vs0, v4_0, vs0, b4);
      vs1 = IVP_ADDNX16(v4_1, vs1);

      xb_vecNx16 v8_0 = IVP_SELNX16I(vs0, 0, IVP_SELI_ROTATE_LEFT_8);
      xb_vecNx16 v8_1 = IVP_SELNX16I(vs1, vs0, IVP_SELI_ROTATE_LEFT_8);
      vs0 = IVP_ADDNX16(v8_0, vs0);
      vs1 = IVP_ADDNX16(v8_1, vs1);

      xb_vecNx16 v16_0, v16_1;
      IVP_DSELNX16(v16_1, v16_0, vs1, vs0, l16);
      IVP_ADDNX16T(vs0, v16_0, vs0, b16);

      vs1 = IVP_ADDNX16(v16_1, vs1);
      vs1 = IVP_ADDNX16(vs0, vs1);

      IVP_MULUUANX16(column_carry_lo, 1, vs0);
      IVP_MULUUANX16(column_carry_hi, 1, vs1);

      xb_vecNx16 res0_lo = IVP_MOVNX16_FROMN_2X32(
          IVP_ADDN_2X32(IVP_CVT32UNX48L(column_carry_lo), row_carry));
      xb_vecNx16 res0_hi = IVP_MOVNX16_FROMN_2X32(
          IVP_ADDN_2X32(IVP_CVT32UNX48H(column_carry_lo), row_carry));
      xb_vecNx16 res1_lo = IVP_MOVNX16_FROMN_2X32(
          IVP_ADDN_2X32(IVP_CVT32UNX48L(column_carry_hi), row_carry));
      xb_vecNx16 res1_hi = IVP_MOVNX16_FROMN_2X32(
          IVP_ADDN_2X32(IVP_CVT32UNX48H(column_carry_hi), row_carry));

      IVP_SANX16_IP(res0_lo, a_st, vpdst);
      IVP_SANX16_IP(res0_hi, a_st, vpdst);
      IVP_SANX16_IP(res1_lo, a_st, vpdst);
      IVP_SANX16_IP(res1_hi, a_st, vpdst);
      IVP_SANX16POS_FP(a_st, vpdst);
      vpdst = OFFSET_PTR_NX16(vpdst, 1,
                              2 * (dstride - (2 * XCHAL_IVPN_SIMD_WIDTH)), 0);
    }
  }

  return 0;
}
