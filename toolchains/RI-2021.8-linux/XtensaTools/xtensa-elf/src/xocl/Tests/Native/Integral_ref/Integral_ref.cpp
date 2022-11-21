#include "xi_api_ref.h"

XI_ERR_TYPE xiIntegralImage_U8U32_ref(const xi_pArray src,
                                      const xi_pArray dst_left,
                                      const xi_pArray dst_top, xi_pArray dst) {

  uint8_t *psrc = (uint8_t *)XI_ARRAY_GET_DATA_PTR(src);
  uint32_t *pdst = (uint32_t *)XI_ARRAY_GET_DATA_PTR(dst);
  uint32_t *pdst_top = (uint32_t *)XI_ARRAY_GET_DATA_PTR(dst_top);
  uint32_t *pdst_left = (uint32_t *)XI_ARRAY_GET_DATA_PTR(dst_left);

  int sstride = XI_ARRAY_GET_PITCH(src);
  int dstride = XI_ARRAY_GET_PITCH(dst);
  int dleft_stride = XI_ARRAY_GET_PITCH(dst_left);

  int width = XI_ARRAY_GET_WIDTH(src);
  int height = XI_ARRAY_GET_HEIGHT(src);

  pdst_top += 1;

  // row 0
  uint32_t v = (pdst_left[0 * dleft_stride] - pdst_top[-1]);
  for (int i = 0; i < width; ++i) {
    v += psrc[i];
    pdst[i] = v + pdst_top[i];
  }

  // remaining rows
  for (int j = 1; j < height; ++j) {
    v = pdst_left[j * dleft_stride] - pdst_left[(j - 1) * dleft_stride];
    for (int i = 0; i < width; ++i) {
      v += psrc[j * sstride + i];
      pdst[j * dstride + i] = v + pdst[(j - 1) * dstride + i];
    }
  }

  return XI_ERR_OK;
}

XI_ERR_TYPE xiIntegral_U8U32_ref(const xi_pArray src, xi_pArray dst) {
  uint8_t *psrc = (uint8_t *)XI_ARRAY_GET_DATA_PTR(src);
  uint32_t *pdst = (uint32_t *)XI_ARRAY_GET_DATA_PTR(dst);

  int sstride = XI_ARRAY_GET_PITCH(src);
  int dstride = XI_ARRAY_GET_PITCH(dst);

  int width = XI_ARRAY_GET_WIDTH(src);
  int height = XI_ARRAY_GET_HEIGHT(src);

  // row 0
  uint32_t v = 0;
  for (int i = 0; i < width; ++i) {
    v += psrc[i];
    pdst[i] = v;
  }

  // remaining rows
  for (int j = 1; j < height; ++j) {
    v = 0;
    for (int i = 0; i < width; ++i) {
      v += psrc[j * sstride + i];
      pdst[j * dstride + i] = v + pdst[(j - 1) * dstride + i];
    }
  }

  return XI_ERR_OK;
}
