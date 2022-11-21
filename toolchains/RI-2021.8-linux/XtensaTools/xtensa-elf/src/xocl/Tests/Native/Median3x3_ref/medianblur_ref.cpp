#include "xi_api_ref.h"

#define PATTERN_SIZE_MAX (9)

static void bubble_sort_U8(uint8_t *data, int n) {
  uint8_t flg = 1;
  while (flg) {
    flg = 0;
    for (int i = 0; i < n - 1; i++)
      if (data[i] > data[i + 1]) {
        uint8_t temp = data[i];
        data[i] = data[i + 1];
        data[i + 1] = temp;
        flg = 1;
      }
  }
}

static uint8_t get_nxn_median_pattern_U8(const uint8_t *img, const int stride,
                                         const uint8_t *mask, int width_mask,
                                         int height_mask) {
  uint8_t values[PATTERN_SIZE_MAX * PATTERN_SIZE_MAX];

  int indent_left = width_mask / 2;
  int indent_right = (width_mask - 1) / 2;
  int indent_top = height_mask / 2;
  int indent_bottom = (height_mask - 1) / 2;

  int num = 0;
  for (int i = -indent_top; i <= indent_bottom; i++) {
    for (int j = -indent_left; j <= indent_right; j++) {
      int tmp = mask[(indent_top + i) * width_mask + (indent_left + j)];

      if (tmp != 0) {
        values[num] = img[stride * i + j];
        num++;
      }
    }
  }

  bubble_sort_U8(values, num);

  return values[(num - 1) / 2];
}

XI_ERR_TYPE xiMedianBlurPattern_MxN_U8_ref(const xi_pTile src, xi_pArray dst,
                                           const uint8_t *mask, int width_mask,
                                           int height_mask) {
  /*if (!xiTileIsValid_U8_ref(src))         return XI_ERR_BADARG;
  if (!xiArrayIsValid_U8_ref(dst))        return XI_ERR_BADARG;
  if (!xiTilesHaveSameSize_ref(src, dst)) return XI_ERR_DATASIZE;

  if (height_mask > PATTERN_SIZE_MAX || width_mask > PATTERN_SIZE_MAX)
      return XI_ERR_KSIZE;
*/
  // int indent_left = width_mask / 2;
  // int indent_right = (width_mask - 1)  / 2;
  // int indent_top = height_mask / 2;
  // int indent_bottom = (height_mask - 1) / 2;

  /*if (XI_TILE_GET_EDGE_WIDTH(src) < indent_left ||
     XI_TILE_GET_EDGE_HEIGHT(src) < indent_top) return XI_ERR_EDGE;
*/

  const uint8_t *src_img = (const uint8_t *)(XI_TILE_GET_DATA_PTR(src));
  uint8_t *dst_img = (uint8_t *)(XI_ARRAY_GET_DATA_PTR(dst));

  const int stride_src = XI_TILE_GET_PITCH(src);
  const int stride_dst = XI_ARRAY_GET_PITCH(dst);

  const int height = XI_TILE_GET_HEIGHT(src);
  const int width = XI_TILE_GET_WIDTH(src);

  for (int i = 0; i < height; i++)
    for (int j = 0; j < width; j++)
      dst_img[i * stride_dst + j] =
          get_nxn_median_pattern_U8(src_img + i * stride_src + j, stride_src,
                                    mask, width_mask, height_mask);

  return XI_ERR_OK;
}

XI_ERR_TYPE xiMedianBlur_3x3_U8_ref(const xi_pTile src, xi_pArray dst) {
  int ksize = 3;
  uint8_t mask[] = {1, 1, 1, 1, 1, 1, 1, 1, 1};

  return xiMedianBlurPattern_MxN_U8_ref(src, dst, mask, ksize, ksize);
}
