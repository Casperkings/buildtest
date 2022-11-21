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
XI_ERR_TYPE SumOfSquaredDiffsU8S32(const uint8_t *src1, const uint8_t *src2,
                                   int32_t *dst, int32_t width,
                                   int32_t nvectors, int32_t nlength) {
  int32_t x, vind, k;
  xb_vecNx8U *restrict psrc1, *restrict psrc2;
  xb_vecNx16U v1_0, v2_0, v3_0, vres_0, v1_1, v2_1, v3_1, vres_1;
  valign a_load, a_load2;
  uint32_t res1, res2;
  int32_t *restrict pdst1, *restrict pdst2, *restrict pdst3, *restrict pdst4;
  vboolN vb1, vb2;
  xb_vecNx16 vc, vm;

  psrc1 = OFFSET_PTR_NX8U(src1, 0, 0, 0);
  a_load = IVP_LANX8U_PP(psrc1);

  vc = IVP_SEQNX16();
  IVP_BMINNX16(vb1, vm, vc, 32);
  IVP_BMAXNX16(vb2, vm, vc, 31);
  k = 0;
  for (x = 0; x <= width - 2 * XCHAL_IVPN_SIMD_WIDTH;
       x += 2 * XCHAL_IVPN_SIMD_WIDTH) {
    IVP_LAVNX8U_XP(v1_0, a_load, psrc1, XCHAL_IVPN_SIMD_WIDTH);
    IVP_LAVNX8U_XP(v1_1, a_load, psrc1, XCHAL_IVPN_SIMD_WIDTH);
    pdst1 = dst + k;
    pdst2 = dst + k + nvectors;
    pdst3 = dst + k + (2 * nvectors);
    pdst4 = dst + k + (3 * nvectors);
    for (vind = 0; vind < nvectors; vind++) {
      psrc2 = (xb_vecNx8U *)(&src2[vind * width + x]);
      a_load2 = IVP_LANX8U_PP(psrc2);
      IVP_LAVNX8U_XP(v2_0, a_load2, psrc2, XCHAL_IVPN_SIMD_WIDTH);
      IVP_LAVNX8U_XP(v2_1, a_load2, psrc2, XCHAL_IVPN_SIMD_WIDTH);
      v3_0 = IVP_ABSSUBUNX16(v1_0, v2_0);
      v3_1 = IVP_ABSSUBUNX16(v1_1, v2_1);
      vres_0 = IVP_MULNX16UPACKL(v3_0, v3_0);
      vres_1 = IVP_MULNX16UPACKL(v3_1, v3_1);
      res1 = IVP_RADDUNX16T(vres_0, vb1);
      res2 = IVP_RADDUNX16T(vres_0, vb2);
      XT_S32I(res1, pdst1, 0);
      XT_S32I(res2, pdst2, 0);
      res1 = IVP_RADDUNX16T(vres_1, vb1);
      res2 = IVP_RADDUNX16T(vres_1, vb2);
      XT_S32I(res1, pdst3, 0);
      XT_S32I(res2, pdst4, 0);
      pdst1 += 1;
      pdst2 += 1;
      pdst3 += 1;
      pdst4 += 1;
    }
    k += (4 * nvectors);
  }
  if (x <= width - XCHAL_IVPN_SIMD_WIDTH) {
    IVP_LAVNX8U_XP(v1_0, a_load, psrc1, XCHAL_IVPN_SIMD_WIDTH);
    pdst1 = dst + k;
    pdst2 = dst + k + nvectors;
    for (vind = 0; vind < nvectors; vind++) {
      psrc2 = (xb_vecNx8U *)(&src2[vind * width + x]);
      a_load2 = IVP_LANX8U_PP(psrc2);
      IVP_LAVNX8U_XP(v2_0, a_load2, psrc2, XCHAL_IVPN_SIMD_WIDTH);
      v3_0 = IVP_ABSSUBUNX16(v1_0, v2_0);
      vres_0 = IVP_MULNX16UPACKL(v3_0, v3_0);
      res1 = IVP_RADDUNX16T(vres_0, vb1);
      res2 = IVP_RADDUNX16T(vres_0, vb2);
      XT_S32I(res1, pdst1, 0);
      XT_S32I(res2, pdst2, 0);
      pdst1 += 1;
      pdst2 += 1;
    }
    k += (2 * nvectors);
    x += XCHAL_IVPN_SIMD_WIDTH;
  }
  if (x < width) {
    IVP_LAVNX8U_XP(v1_0, a_load, psrc1, width - x);
    pdst1 = dst + k;
    for (vind = 0; vind < nvectors; vind++) {
      psrc2 = (xb_vecNx8U *)(&src2[vind * width + x]);
      a_load2 = IVP_LANX8U_PP(psrc2);
      IVP_LAVNX8U_XP(v2_0, a_load2, psrc2, width - x);
      v3_0 = IVP_ABSSUBUNX16(v1_0, v2_0);
      vres_0 = IVP_MULNX16UPACKL(v3_0, v3_0);
      res1 = IVP_RADDUNX16T(vres_0, vb1);
      XT_S32I(res1, pdst1, 0);
      pdst1 += 1;
    }
  }

  return 0;
}
