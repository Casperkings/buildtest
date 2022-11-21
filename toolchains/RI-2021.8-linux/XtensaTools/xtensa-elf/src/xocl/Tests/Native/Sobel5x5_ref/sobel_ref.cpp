#include "xi_api_ref.h"

/* Sobel 5x5 filters */
static XI_ERR_TYPE xiSobelX_5x5_U8S16_ref(const xi_pTile src, xi_pArray dst,
                                          xi_bool normalize) {
  /*if (!xiTileIsValid_U8_ref(src))  return XI_ERR_BADARG;
  if (!xiArrayIsValid_S16_ref(dst)) return XI_ERR_BADARG;
  if (!xiArraysHaveSameSize_ref((xi_pArray)src, dst)) return XI_ERR_DATASIZE;*/

  XI_Q15 kernel[] = {-1, -2, 0,  2,  1, -4, -8, 0,  8,  4, -6, -12, 0,
                     12, 6,  -4, -8, 0, 8,  4,  -1, -2, 0, 2,  1};

  int N_div_2 = 2;
  if (XI_TILE_GET_EDGE_WIDTH(src) < N_div_2 ||
      XI_TILE_GET_EDGE_HEIGHT(src) < N_div_2)
    return XI_ERR_EDGE;

  const uint8_t *src_img = (const uint8_t *)(XI_TILE_GET_DATA_PTR(src));
  int16_t *dst_img = (int16_t *)(XI_ARRAY_GET_DATA_PTR(dst));

  for (int i = 0; i < XI_TILE_GET_HEIGHT(src); i++)
    for (int j = 0; j < XI_TILE_GET_WIDTH(src); j++) {
      int32_t sum = 0;

      for (int k = -N_div_2, kk = 0; k <= N_div_2; k++, kk++)
        for (int l = -N_div_2, ll = 0; l <= N_div_2; l++, ll++)
          sum += src_img[(i + k) * XI_TILE_GET_PITCH(src) + (j + l)] *
                 kernel[kk * 5 + ll];

      dst_img[i * XI_ARRAY_GET_PITCH(dst) + j] = sum;
    }

  return XI_ERR_OK;
}

static XI_ERR_TYPE xiSobelY_5x5_U8S16_ref(const xi_pTile src, xi_pArray dst,
                                          xi_bool normalize) {
  /* if (!xiTileIsValid_U8_ref(src))  return XI_ERR_BADARG;
   if (!xiArrayIsValid_S16_ref(dst)) return XI_ERR_BADARG;
   if (!xiArraysHaveSameSize_ref((xi_pArray)src, dst)) return XI_ERR_DATASIZE;*/

  XI_Q15 kernel[] = {-1, -4, -6, -4, -1, -2, -8, -12, -8, -2, 0, 0, 0,
                     0,  0,  2,  8,  12, 8,  2,  1,   4,  6,  4, 1};

  int N_div_2 = 2;
  if (XI_TILE_GET_EDGE_WIDTH(src) < N_div_2 ||
      XI_TILE_GET_EDGE_HEIGHT(src) < N_div_2)
    return XI_ERR_EDGE;

  const uint8_t *src_img = (const uint8_t *)(XI_TILE_GET_DATA_PTR(src));
  int16_t *dst_img = (int16_t *)(XI_ARRAY_GET_DATA_PTR(dst));

  for (int i = 0; i < XI_TILE_GET_HEIGHT(src); i++)
    for (int j = 0; j < XI_TILE_GET_WIDTH(src); j++) {
      int32_t sum = 0;

      for (int k = -N_div_2, kk = 0; k <= N_div_2; k++, kk++)
        for (int l = -N_div_2, ll = 0; l <= N_div_2; l++, ll++)
          sum += src_img[(i + k) * XI_TILE_GET_PITCH(src) + (j + l)] *
                 kernel[kk * 5 + ll];

      dst_img[i * XI_ARRAY_GET_PITCH(dst) + j] = sum;
    }

  return XI_ERR_OK;
}

XI_ERR_TYPE xiSobel_5x5_U8S16_ref(const xi_pTile src, xi_pArray dst_dx,
                                  xi_pArray dst_dy, xi_bool normalize) {
  XI_ERR_TYPE err = xiSobelX_5x5_U8S16_ref(src, dst_dx, normalize);
  if (err == XI_ERR_OK)
    err = xiSobelY_5x5_U8S16_ref(src, dst_dy, normalize);
  return err;
}
