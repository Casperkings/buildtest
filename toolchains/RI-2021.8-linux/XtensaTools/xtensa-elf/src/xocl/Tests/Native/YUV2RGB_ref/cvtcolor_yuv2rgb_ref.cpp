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

#define Q15_0_7874 25802
#define Q15_0_9278 30402
#define Q15_0_0936 3068
#define Q15_0_2340 7669
XI_ERR_TYPE xiCvtColor_U8_YUV2RGB_420_BT709_ref(const xi_pArray y,
                                                const xi_pArray u,
                                                const xi_pArray v,
                                                xi_pArray rgb) {

  uint8_t *src_y_ptr = (uint8_t *)XI_ARRAY_GET_DATA_PTR(y);
  uint8_t *src_u_ptr = (uint8_t *)XI_ARRAY_GET_DATA_PTR(u);
  uint8_t *src_v_ptr = (uint8_t *)XI_ARRAY_GET_DATA_PTR(v);

  uint8_t *dst_rgb_ptr = (uint8_t *)XI_ARRAY_GET_DATA_PTR(rgb);

  int stride_src_y = XI_ARRAY_GET_PITCH(y);
  int stride_src_u = XI_ARRAY_GET_PITCH(u);
  int stride_src_v = XI_ARRAY_GET_PITCH(v);

  int stride_dst_rgb = XI_ARRAY_GET_PITCH(rgb);

  int width = XI_ARRAY_GET_WIDTH(y);
  int height = XI_ARRAY_GET_HEIGHT(y);

  for (int iy = 0; iy < height; iy++)
    for (int ix = 0; ix < width; ix++) {
      int color_y = src_y_ptr[iy * stride_src_y + ix];
      int color_u = src_u_ptr[iy / 2 * stride_src_u + ix / 2];
      int color_v = src_v_ptr[iy / 2 * stride_src_v + ix / 2];

      int A = Q15_0_7874 * (color_v - 128);
      int B = Q15_0_2340 * (color_v - 128);
      int C = Q15_0_0936 * (color_u - 128);
      int D = Q15_0_9278 * (color_u - 128);

      int color_r = ((color_y << 14) + A + (1 << 13)) >> 14;
      int color_g = ((color_y << 14) - C - B + (1 << 13)) >> 14;
      int color_b = ((color_y << 14) + D + (1 << 13)) >> 14;

      color_r = clamp(color_r, 0, 255);
      color_g = clamp(color_g, 0, 255);
      color_b = clamp(color_b, 0, 255);

      dst_rgb_ptr[iy * stride_dst_rgb + 3 * ix + 0] = color_r;
      dst_rgb_ptr[iy * stride_dst_rgb + 3 * ix + 1] = color_g;
      dst_rgb_ptr[iy * stride_dst_rgb + 3 * ix + 2] = color_b;
    }

  return XI_ERR_OK;
}
