#include "xi_api_ref.h"
//#include "xi_lib.h"

#define XI_ENABLE_BIT_EXACT_CREF
#ifdef XI_ENABLE_BIT_EXACT_CREF
#define Q15_0_2126 6966
#define Q15_0_7152 23436
#define Q15_0_0722 2366

#define Q15_0_5389 17659
#define Q15_0_6350 20808

#else
static float clampf(float value, float min, float max) {
  if (value < min) {
    value = min;
  }
  if (value > max) {
    value = max;
  }
  return value;
}
#endif

static int clamp(int value, int min, int max) {
  if (value < min) {
    value = min;
  }
  if (value > max) {
    value = max;
  }
  return value;
}

#define Q15_0_2126 6966
#define Q15_0_7152 23436
#define Q15_0_0722 2366

#define Q15_0_5389 17659
#define Q15_0_6350 20808

#ifdef XI_ENABLE_BIT_EXACT_CREF

XI_ERR_TYPE xiCvtColor_U8_RGB2YUV_BT709_ref(const xi_pArray rgb, xi_pArray y,
                                            xi_pArray u, xi_pArray v) {
  /*if (!xiArrayIsValid_U8_ref(rgb)) return XI_ERR_BADARG;
  if (!xiArrayIsValid_U8_ref(y))   return XI_ERR_BADARG;
  if (!xiArrayIsValid_U8_ref(u))   return XI_ERR_BADARG;
  if (!xiArrayIsValid_U8_ref(v))   return XI_ERR_BADARG;

  if (!xiArraysHaveSameSize_ref(u, v)) return XI_ERR_DATASIZE;
  if (!xiArraysHaveSameSize_ref(y, v)) return XI_ERR_DATASIZE;*/

  uint8_t *src_rgb_ptr = (uint8_t *)XI_ARRAY_GET_DATA_PTR(rgb);
  uint8_t *dst_y_ptr = (uint8_t *)XI_ARRAY_GET_DATA_PTR(y);
  uint8_t *dst_u_ptr = (uint8_t *)XI_ARRAY_GET_DATA_PTR(u);
  uint8_t *dst_v_ptr = (uint8_t *)XI_ARRAY_GET_DATA_PTR(v);

  int stride_src_rgb = XI_ARRAY_GET_PITCH(rgb);
  int stride_dst_y = XI_ARRAY_GET_PITCH(y);
  int stride_dst_u = XI_ARRAY_GET_PITCH(u);
  int stride_dst_v = XI_ARRAY_GET_PITCH(v);

  int width = XI_ARRAY_GET_WIDTH(y);
  int height = XI_ARRAY_GET_HEIGHT(y);
  for (int iy = 0; iy < height; iy++)
    for (int ix = 0; ix < width; ix++) {
      int r = src_rgb_ptr[iy * stride_src_rgb + 3 * ix + 0];
      int g = src_rgb_ptr[iy * stride_src_rgb + 3 * ix + 1];
      int b = src_rgb_ptr[iy * stride_src_rgb + 3 * ix + 2];

      int color_y =
          b * Q15_0_0722 + g * Q15_0_7152 + r * Q15_0_2126 + (1 << 14);
      uint8_t y_tmp = color_y >> 15;

      int w_u = b * Q15_0_5389 - y_tmp * Q15_0_5389 + (1 << 14);
      int w_v = r * Q15_0_6350 - y_tmp * Q15_0_6350 + (1 << 14);

      uint8_t u_tmp = (w_u >> 15) + 128;
      uint8_t v_tmp = (w_v >> 15) + 128;

      u_tmp = clamp(u_tmp, 0, 255);
      v_tmp = clamp(v_tmp, 0, 255);

      dst_y_ptr[iy * stride_dst_y + ix] = y_tmp;
      dst_u_ptr[iy * stride_dst_u + ix] = u_tmp;
      dst_v_ptr[iy * stride_dst_v + ix] = v_tmp;
    }

  return XI_ERR_OK;
}
#else
XI_ERR_TYPE xiCvtColor_U8_RGB2YUV_BT709_ref(const xi_pArray rgb, xi_pArray y,
                                            xi_pArray u, xi_pArray v) {
  /*if (!xiArrayIsValid_U8_ref(rgb)) return XI_ERR_BADARG;
  if (!xiArrayIsValid_U8_ref(y))   return XI_ERR_BADARG;
  if (!xiArrayIsValid_U8_ref(u))   return XI_ERR_BADARG;
  if (!xiArrayIsValid_U8_ref(v))   return XI_ERR_BADARG;

  if (!xiArraysHaveSameSize_ref(u, v)) return XI_ERR_DATASIZE;
  if (!xiArraysHaveSameSize_ref(y, v)) return XI_ERR_DATASIZE;*/

  uint8_t *src_rgb_ptr = (uint8_t *)XI_ARRAY_GET_DATA_PTR(rgb);
  uint8_t *dst_y_ptr = (uint8_t *)XI_ARRAY_GET_DATA_PTR(y);
  uint8_t *dst_u_ptr = (uint8_t *)XI_ARRAY_GET_DATA_PTR(u);
  uint8_t *dst_v_ptr = (uint8_t *)XI_ARRAY_GET_DATA_PTR(v);

  int stride_src_rgb = XI_ARRAY_GET_PITCH(rgb);
  int stride_dst_y = XI_ARRAY_GET_PITCH(y);
  int stride_dst_u = XI_ARRAY_GET_PITCH(u);
  int stride_dst_v = XI_ARRAY_GET_PITCH(v);

  int width = XI_ARRAY_GET_WIDTH(y);
  int height = XI_ARRAY_GET_HEIGHT(y);

  for (int iy = 0; iy < height; iy++)
    for (int ix = 0; ix < width; ix++) {
      int r = src_rgb_ptr[iy * stride_src_rgb + 3 * ix + 0];
      int g = src_rgb_ptr[iy * stride_src_rgb + 3 * ix + 1];
      int b = src_rgb_ptr[iy * stride_src_rgb + 3 * ix + 2];

#ifndef XI_ENABLE_BIT_EXACT_CREF
      float color_y = 0.2126f * r + 0.7152f * g + 0.0722f * b;
      float color_u = (b - color_y) * 0.5389f + 128.0f;
      float color_v = (r - color_y) * 0.6350f + 128.0f;

      color_u = clampf(color_u, 0.0f, 255.0f);
      color_v = clampf(color_v, 0.0f, 255.0f);

      dst_y_ptr[iy * stride_dst_y + ix] = (int)(color_y + 0.5f);
      dst_u_ptr[iy * stride_dst_u + ix] = (int)(color_u);
      dst_v_ptr[iy * stride_dst_v + ix] = (int)(color_v);
#else
      int color_y =
          (Q15_0_2126 * r + Q15_0_7152 * g + Q15_0_0722 * b + (1 << 14)) >> 15;
      int color_u = (((b - color_y) * Q15_0_5389 + (1 << 14)) >> 15) + 128;
      int color_v = (((r - color_y) * Q15_0_6350 + (1 << 14)) >> 15) + 128;

      color_u = clamp(color_u, 0, 255);
      color_v = clamp(color_v, 0, 255);

      dst_y_ptr[iy * stride_dst_y + ix] = color_y;
      dst_u_ptr[iy * stride_dst_u + ix] = color_u;
      dst_v_ptr[iy * stride_dst_v + ix] = color_v;
#endif
    }

  return XI_ERR_OK;
}
#endif

XI_ERR_TYPE xiCvtColor_U8_RGB2YUV_420_BT709_ref(const xi_pArray rgb,
                                                xi_pArray y, xi_pArray u,
                                                xi_pArray v) {

  uint8_t *src_rgb_ptr = (uint8_t *)XI_ARRAY_GET_DATA_PTR(rgb);
  uint8_t *dst_y_ptr = (uint8_t *)XI_ARRAY_GET_DATA_PTR(y);
  uint8_t *dst_u_ptr = (uint8_t *)XI_ARRAY_GET_DATA_PTR(u);
  uint8_t *dst_v_ptr = (uint8_t *)XI_ARRAY_GET_DATA_PTR(v);

  int stride_src_rgb = XI_ARRAY_GET_PITCH(rgb);
  int stride_dst_y = XI_ARRAY_GET_PITCH(y);
  int stride_dst_u = XI_ARRAY_GET_PITCH(u);
  int stride_dst_v = XI_ARRAY_GET_PITCH(v);

  int width = XI_ARRAY_GET_WIDTH(y);
  int height = XI_ARRAY_GET_HEIGHT(y);

  for (int iy = 0; iy < height; iy++)
    for (int ix = 0; ix < width; ix++) {
      int r = src_rgb_ptr[iy * stride_src_rgb + 3 * ix + 0];
      int g = src_rgb_ptr[iy * stride_src_rgb + 3 * ix + 1];
      int b = src_rgb_ptr[iy * stride_src_rgb + 3 * ix + 2];

      int color_y =
          (Q15_0_2126 * r + Q15_0_7152 * g + Q15_0_0722 * b + (1 << 14)) >> 15;

      dst_y_ptr[iy * stride_dst_y + ix] = color_y;
    }

  for (int iy = 0; iy < height / 2; iy++)
    for (int ix = 0; ix < width / 2; ix++) {
      int64_t sum_v = 0;
      int64_t sum_u = 0;

      {
        int r =
            src_rgb_ptr[(2 * iy + 0) * stride_src_rgb + 3 * (2 * ix + 0) + 0];
        int g =
            src_rgb_ptr[(2 * iy + 0) * stride_src_rgb + 3 * (2 * ix + 0) + 1];
        int b =
            src_rgb_ptr[(2 * iy + 0) * stride_src_rgb + 3 * (2 * ix + 0) + 2];

        int color_y =
            (Q15_0_2126 * r + Q15_0_7152 * g + Q15_0_0722 * b + (1 << 14)) >>
            15;
        int color_u = ((b - color_y) * Q15_0_5389);
        int color_v = ((r - color_y) * Q15_0_6350);

        sum_u += color_u;
        sum_v += color_v;
      }
      {
        int r =
            src_rgb_ptr[(2 * iy + 0) * stride_src_rgb + 3 * (2 * ix + 1) + 0];
        int g =
            src_rgb_ptr[(2 * iy + 0) * stride_src_rgb + 3 * (2 * ix + 1) + 1];
        int b =
            src_rgb_ptr[(2 * iy + 0) * stride_src_rgb + 3 * (2 * ix + 1) + 2];

        int color_y =
            (Q15_0_2126 * r + Q15_0_7152 * g + Q15_0_0722 * b + (1 << 14)) >>
            15;
        int color_u = ((b - color_y) * Q15_0_5389);
        int color_v = ((r - color_y) * Q15_0_6350);

        sum_u += color_u;
        sum_v += color_v;
      }
      {
        int r =
            src_rgb_ptr[(2 * iy + 1) * stride_src_rgb + 3 * (2 * ix + 0) + 0];
        int g =
            src_rgb_ptr[(2 * iy + 1) * stride_src_rgb + 3 * (2 * ix + 0) + 1];
        int b =
            src_rgb_ptr[(2 * iy + 1) * stride_src_rgb + 3 * (2 * ix + 0) + 2];

        int color_y =
            (Q15_0_2126 * r + Q15_0_7152 * g + Q15_0_0722 * b + (1 << 14)) >>
            15;
        int color_u = ((b - color_y) * Q15_0_5389);
        int color_v = ((r - color_y) * Q15_0_6350);

        sum_u += color_u;
        sum_v += color_v;
      }
      {
        int r =
            src_rgb_ptr[(2 * iy + 1) * stride_src_rgb + 3 * (2 * ix + 1) + 0];
        int g =
            src_rgb_ptr[(2 * iy + 1) * stride_src_rgb + 3 * (2 * ix + 1) + 1];
        int b =
            src_rgb_ptr[(2 * iy + 1) * stride_src_rgb + 3 * (2 * ix + 1) + 2];

        int color_y =
            (Q15_0_2126 * r + Q15_0_7152 * g + Q15_0_0722 * b + (1 << 14)) >>
            15;
        int color_u = ((b - color_y) * Q15_0_5389);
        int color_v = ((r - color_y) * Q15_0_6350);

        sum_u += color_u;
        sum_v += color_v;
      }

      dst_u_ptr[iy * stride_dst_u + ix] =
          (uint8_t)(((sum_u + 65535) >> 17) + 128);
      dst_v_ptr[iy * stride_dst_v + ix] =
          (uint8_t)(((sum_v + 65535) >> 17) + 128);
    }

  return XI_ERR_OK;
}
