#include "xi_api_ref.h"

XI_ERR_TYPE xiBoxFilter_U8_ref(const xi_pTile src, xi_pTile dst, int ksize) {

  int N_div_2 = ksize / 2;

  int divisor = ksize * ksize;
  int bias = divisor / 2;

  uint8_t *psrc = (uint8_t *)(XI_TILE_GET_DATA_PTR(src));
  uint8_t *pdst = (uint8_t *)(XI_TILE_GET_DATA_PTR(dst));

  int sstride = XI_TILE_GET_PITCH(src);
  int dstride = XI_TILE_GET_PITCH(dst);
  int height = XI_TILE_GET_HEIGHT(src);
  int width = XI_TILE_GET_WIDTH(src);

  for (int i = 0; i < height; i++)
    for (int j = 0; j < width; j++) {
      int sum = bias;

      for (int k = -N_div_2; k <= N_div_2; k++)
        for (int l = -N_div_2; l <= N_div_2; l++)
          sum += psrc[(i + k) * sstride + (j + l)];

      pdst[i * dstride + j] = sum / divisor;
    }

  return XI_ERR_OK;
}
XI_ERR_TYPE xiBoxFilter_3x3_U8_ref(const xi_pTile src, xi_pTile dst) {
  int ksize = 3;
  int N_div_2 = ksize / 2;

  int divisor = ksize * ksize;
  int bias = divisor / 2;

  uint8_t *psrc = (uint8_t *)(XI_TILE_GET_DATA_PTR(src));
  uint8_t *pdst = (uint8_t *)(XI_TILE_GET_DATA_PTR(dst));

  int sstride = XI_TILE_GET_PITCH(src);
  int dstride = XI_TILE_GET_PITCH(dst);
  int height = XI_TILE_GET_HEIGHT(src);
  int width = XI_TILE_GET_WIDTH(src);

  for (int i = 0; i < height; i++)
    for (int j = 0; j < width; j++) {
      int sum = bias;

      for (int k = -N_div_2; k <= N_div_2; k++)
        for (int l = -N_div_2; l <= N_div_2; l++)
          sum += psrc[(i + k) * sstride + (j + l)];

      pdst[i * dstride + j] = sum / divisor;
    }

  return XI_ERR_OK;
}
