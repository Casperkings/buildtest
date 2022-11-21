#include "xi_api_ref.h"

#define PATTERN_SIZE_MIN (1)
#define PATTERN_SIZE_MAX (31)

#define XI_MAX(a, b) (a) > (b) ? (a) : (b)
XI_ERR_TYPE xiDilatePattern_NxM_U8_ref(const xi_pTile src, xi_pArray dst,
                                       const uint8_t *mask, int width_mask,
                                       int height_mask) {

  int indent_left = width_mask / 2;
  int indent_right = (width_mask - 1) / 2;
  int indent_top = height_mask / 2;
  int indent_bottom = (height_mask - 1) / 2;

  uint8_t *psrc = (uint8_t *)(XI_TILE_GET_DATA_PTR(src));
  uint8_t *pdst = (uint8_t *)(XI_ARRAY_GET_DATA_PTR(dst));

  int sstride = XI_TILE_GET_PITCH(src);
  int dstride = XI_ARRAY_GET_PITCH(dst);
  int height = XI_TILE_GET_HEIGHT(src);
  int width = XI_TILE_GET_WIDTH(src);

  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j++) {
      int sum = 0;

      for (int y = -indent_top; y <= indent_bottom; y++) {
        for (int x = -indent_left; x <= indent_right; x++) {
          int tmp = mask[(indent_top + y) * width_mask + (indent_left + x)];

          if (tmp != 0) {
            sum = XI_MAX(psrc[(i + y) * sstride + x + j], sum);
          }
        }
      }

      pdst[i * dstride + j] = sum;
    }
  }

  return XI_ERR_OK;
}

XI_ERR_TYPE xiDilate_3x3_U8_ref(const xi_pTile src, xi_pArray dst) {
  int ksize = 3;
  if ((ksize & 0x1) == 0 || (ksize > PATTERN_SIZE_MAX) ||
      (ksize < PATTERN_SIZE_MIN))
    return XI_ERR_KSIZE;

  uint8_t mask[PATTERN_SIZE_MAX * PATTERN_SIZE_MAX];

  for (int i = 0; i < ksize; i++)
    for (int j = 0; j < ksize; j++)
      mask[i * ksize + j] = 1;

  return xiDilatePattern_NxM_U8_ref(src, dst, mask, ksize, ksize);
}
