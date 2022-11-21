#include "xi_api_ref.h"

XI_ERR_TYPE xiHOGGradient_interleave_U8S16_ref(const xi_pTile src,
                                               xi_pArray dst_dx_dy) {

  int width = XI_TILE_GET_WIDTH(src);
  int height = XI_TILE_GET_HEIGHT(src);

  int sstride = XI_TILE_GET_PITCH(src);
  int xystride = XI_ARRAY_GET_PITCH(dst_dx_dy);

  uint8_t *psrc = (uint8_t *)(XI_TILE_GET_DATA_PTR(src));
  int16_t *pxydst = (int16_t *)(XI_ARRAY_GET_DATA_PTR(dst_dx_dy));

  for (int i = 0; i < width; ++i) {
    for (int j = 0; j < height; ++j) {
      pxydst[j * xystride + (2 * i)] =
          psrc[j * sstride + (i + 1)] - psrc[j * sstride + (i - 1)];
      pxydst[j * xystride + ((2 * i) + 1)] =
          psrc[(j + 1) * sstride + i] - psrc[(j - 1) * sstride + i];
    }
  }

  return XI_ERR_OK;
}
