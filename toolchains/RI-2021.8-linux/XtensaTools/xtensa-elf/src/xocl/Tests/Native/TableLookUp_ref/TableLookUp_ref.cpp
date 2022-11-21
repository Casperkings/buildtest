#include "xi_api_ref.h"

#define XI_MAX(a, b) (a) > (b) ? (a) : (b)

XI_ERR_TYPE xiLUT_U8_ref(const xi_pTile src, xi_pTile dst, uint8_t *lut,
                         int size) {
  const int sstride = XI_TILE_GET_PITCH(src);
  const int dstride = XI_TILE_GET_PITCH(dst);
  int width = XI_TILE_GET_WIDTH(src);
  int height = XI_TILE_GET_HEIGHT(src);

  uint8_t *src_ptr = (uint8_t *)XI_TILE_GET_DATA_PTR(src);
  uint8_t *dst_ptr = (uint8_t *)XI_TILE_GET_DATA_PTR(dst);

  int n = 256 / size;
  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j++) {
      int id = src_ptr[i * sstride + j] / n;
      dst_ptr[i * dstride + j] = lut[id];
    }
  }

  return XI_ERR_OK;
}
