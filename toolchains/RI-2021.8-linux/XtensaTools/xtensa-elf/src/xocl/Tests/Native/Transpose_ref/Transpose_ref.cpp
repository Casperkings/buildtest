#include "xi_api_ref.h"

#define XI_MAX(a, b) (a) > (b) ? (a) : (b)

XI_ERR_TYPE xiTranspose_S16_ref(const xi_pArray src, xi_pArray dst) {

  int16_t *src_ptr = (int16_t *)XI_ARRAY_GET_DATA_PTR(src);
  int16_t *dst_ptr = (int16_t *)XI_ARRAY_GET_DATA_PTR(dst);

  for (int i = 0; i < XI_ARRAY_GET_HEIGHT(src); i++)
    for (int j = 0; j < XI_ARRAY_GET_WIDTH(src); j++)
      dst_ptr[j * XI_ARRAY_GET_PITCH(dst) + i] =
          src_ptr[i * XI_ARRAY_GET_PITCH(src) + j];

  return XI_ERR_OK;
}
