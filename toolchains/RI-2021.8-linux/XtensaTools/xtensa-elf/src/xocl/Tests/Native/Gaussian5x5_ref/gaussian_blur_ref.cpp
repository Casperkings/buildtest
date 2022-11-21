#include "xi_api_ref.h"

XI_ERR_TYPE xiGaussianBlur_5x5_U8_ref(const xi_pTile src, xi_pArray dst) {
  uint8_t kernel[] = {1,  4, 6, 4,  1,  4,  16, 24, 16, 4, 6, 24, 36,
                      24, 6, 4, 16, 24, 16, 4,  1,  4,  6, 4, 1};
  const uint8_t *src_img = (const uint8_t *)(XI_TILE_GET_DATA_PTR(src));
  uint8_t *dst_img = (uint8_t *)(XI_ARRAY_GET_DATA_PTR(dst));

  for (int j = 0; j < XI_TILE_GET_HEIGHT(src); j++) {
    for (int k = 0; k < XI_TILE_GET_WIDTH(src); k++) {
      uint16_t sum = 0;
      for (int i = -2; i <= 2; ++i) {
        for (int l = -2; l <= 2; ++l) {
          sum += src_img[(j + i) * XI_TILE_GET_PITCH(src) + k + l] *
                 kernel[(i + 2) * 5 + l + 2];
        }
      }
      sum = (sum + 128) >> 8;
      sum = sum > 255 ? 255 : sum;
      dst_img[j * XI_ARRAY_GET_PITCH(dst) + k] = sum;
    }
  }

  return XI_ERR_OK;
}
