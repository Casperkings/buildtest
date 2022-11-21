#include "xi_api_ref.h"

XI_ERR_TYPE xiPyrDown_U8_ref(const xi_pTile src, xi_pArray dst) {
  XI_Q15 kernel[] = {
      1,  4, 6, 4,  1,  4,  16, 24, 16, 4, 6, 24, 36,
      24, 6, 4, 16, 24, 16, 4,  1,  4,  6, 4, 1,
  };

  const uint8_t ksize = 5;
  const uint8_t N_div_2 = ksize / 2;

  const uint8_t *src_img = (const uint8_t *)(XI_TILE_GET_DATA_PTR(src));
  uint8_t *dst_img = (uint8_t *)(XI_ARRAY_GET_DATA_PTR(dst));

  for (int i = 0; i < XI_TILE_GET_HEIGHT(src); i += 2)
    for (int j = 0; j < XI_TILE_GET_WIDTH(src); j += 2) {
      int32_t sum = 0;
      for (int k = -N_div_2, kk = 0; k <= N_div_2; k++, kk++)
        for (int l = -N_div_2, ll = 0; l <= N_div_2; l++, ll++)
          sum += src_img[(i + k) * XI_TILE_GET_PITCH(src) + (j + l)] *
                 kernel[kk * ksize + ll];

      dst_img[(i / 2) * XI_ARRAY_GET_PITCH(dst) + (j / 2)] = (sum + 128) / 256;
    }

  return XI_ERR_OK;
}
