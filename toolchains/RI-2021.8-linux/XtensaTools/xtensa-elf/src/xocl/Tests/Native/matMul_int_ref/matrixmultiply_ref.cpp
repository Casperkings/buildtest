#include "xi_api_ref.h"

static inline int clampi(int64_t a, int64_t b, int64_t v) {
  if (v > b)
    return b;
  if (v < a)
    return a;
  return v;
}

XI_ERR_TYPE xiMatrixMultiply_S8S32_ref(const xi_pArray src0,
                                       const xi_pArray src1, xi_pArray dst) {
  // if (!xiArrayIsValid_U8_ref(src0))        return XI_ERR_BADARG;
  // if (!xiArrayIsValid_U8_ref(src1))        return XI_ERR_BADARG;
  // if (!xiArrayIsValid_S32_ref(dst))         return XI_ERR_BADARG;

  if (!(XI_ARRAY_GET_WIDTH(src0) == XI_ARRAY_GET_HEIGHT(src1)))
    return XI_ERR_BADARG;
  if (!(XI_ARRAY_GET_HEIGHT(src0) == XI_ARRAY_GET_HEIGHT(dst)))
    return XI_ERR_BADARG;
  if (!(XI_ARRAY_GET_WIDTH(src1) == XI_ARRAY_GET_WIDTH(dst)))
    return XI_ERR_BADARG;

  const uint8_t *src0_ptr = (uint8_t *)XI_TILE_GET_DATA_PTR(src0);
  const uint8_t *src1_ptr = (uint8_t *)XI_TILE_GET_DATA_PTR(src1);

  uint32_t *dst_ptr = (uint32_t *)XI_TILE_GET_DATA_PTR(dst);

  const int stride_src0 = XI_TILE_GET_PITCH(src0);
  const int stride_src1 = XI_TILE_GET_PITCH(src1);
  const int stride_dst = XI_TILE_GET_PITCH(dst);

  int w_src0 = XI_TILE_GET_WIDTH(src0);
  int h_src0 = XI_TILE_GET_HEIGHT(src0);
  int w_src1 = XI_TILE_GET_WIDTH(src1);

  for (int y = 0; y < h_src0; y++) {
    for (int x = 0; x < w_src1; x++) {
      uint32_t mul_out = 0;
      for (int i = 0; i < w_src0; i++) {
        mul_out +=
            src0_ptr[y * stride_src0 + i] * src1_ptr[x + i * stride_src1];
      }
      dst_ptr[y * stride_dst + x] = mul_out;
    }
  }
  return XI_ERR_OK;
}
