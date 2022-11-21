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

#define XI_LUT256U8N2 XI_LUT8NX8U
#define XI_LUT8NX8U(vsrc, vdst, lut0, lut1, lut2, lut3)                        \
  {                                                                            \
    vdst = IVP_SEL2NX8(lut1, lut0, vsrc);                                      \
    IVP_SEL2NX8T(vdst, lut3, lut2, vsrc,                                       \
                 IVP_LTU2NX8(4 * XCHAL_IVPN_SIMD_WIDTH - 1, vsrc));            \
  }

XI_ERR_TYPE xiLUT256_U8(const xi_pArray src, xi_pArray dst,
                        const uint8_t *lut256) {
  const int sstride = XI_TILE_GET_PITCH(src);
  const int dstride = XI_TILE_GET_PITCH(dst);
  const int width = XI_TILE_GET_WIDTH(src);
  const int height = XI_TILE_GET_HEIGHT(src);

  xb_vec2Nx8U *restrict vpsrc;
  xb_vec2Nx8U *restrict vpdst;

  xb_vec2Nx8U lut0, lut1, lut2, lut3;
  {
    xb_vec2Nx8U *vplut = (xb_vec2Nx8U *)lut256;
    valign va = IVP_LA2NX8U_PP(vplut);
    IVP_LA2NX8U_IP(lut0, va, vplut);
    IVP_LA2NX8U_IP(lut1, va, vplut);
    IVP_LA2NX8U_IP(lut2, va, vplut);
    IVP_LA2NX8U_IP(lut3, va, vplut);
  }

  uint8_t *const src_data = (uint8_t *)XI_TILE_GET_DATA_PTR(src);
  uint8_t *const dst_data = (uint8_t *)XI_TILE_GET_DATA_PTR(dst);
  valign vadst = IVP_ZALIGN();

  int x = 0;

  for (; x < width - 2 * XCHAL_IVPN_SIMD_WIDTH;
       x += 4 * XCHAL_IVPN_SIMD_WIDTH) {
    uint8_t *src_ptr = (uint8_t *)OFFSET_PTR_NX8U(src_data, 0, sstride, x);
    uint8_t *dst_ptr = (uint8_t *)OFFSET_PTR_NX8U(dst_data, 0, dstride, x);

    for (int y = 0; y < height; ++y) {
      vpsrc = OFFSET_PTR_2NX8U(src_ptr, y, sstride, 0);
      vpdst = OFFSET_PTR_2NX8U(dst_ptr, y, dstride, 0);
      valign vasrc = IVP_LA2NX8U_PP(vpsrc);
      xb_vec2Nx8U vsrc0;
      IVP_LA2NX8U_IP(vsrc0, vasrc, vpsrc);
      xb_vec2Nx8U vsrc1;
      IVP_LAV2NX8U_XP(vsrc1, vasrc, vpsrc,
                      width - x - 2 * XCHAL_IVPN_SIMD_WIDTH);

      xb_vec2Nx8U vdst0;
      XI_LUT256U8N2(vsrc0, vdst0, lut0, lut1, lut2, lut3);
      xb_vec2Nx8U vdst1;
      XI_LUT256U8N2(vsrc1, vdst1, lut0, lut1, lut2, lut3);

      IVP_SA2NX8U_IP(vdst0, vadst, vpdst);
      IVP_SAV2NX8U_XP(vdst1, vadst, vpdst,
                      width - x - 2 * XCHAL_IVPN_SIMD_WIDTH);
      IVP_SAPOS2NX8U_FP(vadst, vpdst);
    }
  }

  if (x < width) {
    uint8_t *src_ptr = (uint8_t *)OFFSET_PTR_NX8U(src_data, 0, sstride, x);
    uint8_t *dst_ptr = (uint8_t *)OFFSET_PTR_NX8U(dst_data, 0, dstride, x);

    for (int y = 0; y < height; ++y) {
      vpsrc = OFFSET_PTR_2NX8U(src_ptr, y, sstride, 0);
      vpdst = OFFSET_PTR_2NX8U(dst_ptr, y, dstride, 0);

      valign vasrc = IVP_LA2NX8U_PP(vpsrc);
      xb_vec2Nx8U vsrc;
      IVP_LAV2NX8U_XP(vsrc, vasrc, vpsrc, width - x);

      xb_vec2Nx8U vdst;
      XI_LUT256U8N2(vsrc, vdst, lut0, lut1, lut2, lut3);

      IVP_SAV2NX8U_XP(vdst, vadst, vpdst, width - x);
      IVP_SAPOS2NX8U_FP(vadst, vpdst);
    }
  }
  return 0;
}
