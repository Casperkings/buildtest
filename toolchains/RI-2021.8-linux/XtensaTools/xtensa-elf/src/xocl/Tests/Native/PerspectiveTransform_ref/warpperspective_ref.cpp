#include "xi_api_ref.h"
#define XT_MIN(a, b) ((a) < (b) ? (a) : (b))
#define XT_MAX(a, b) ((a) > (b) ? (a) : (b))
#define XI_ENABLE_BIT_EXACT_CREF
#define CLIP(min, max, value) XT_MAX(min, XT_MIN(max, value))

#define XCHAL_IVPN_SIMD_WIDTH 32

/* quo = (dvdnd << 32) / (hi(dvsr), 0) */
#define INT32_DIV_Q15_16(quo, dvdnd, dvsr)                                     \
  {                                                                            \
    quo = (int32_t)((((int64_t)(dvdnd)) << 32) /                               \
                    ((dvsr) & (((1 << 16) - 1) << 16)));                       \
  }

#define INT32_SRA(a, sht) (((sht) > 0) ? ((a) >> (sht)) : ((a) << (-(sht))))
#define INT32_SLA(a, sht) (((sht) > 0) ? ((a) << (sht)) : ((a) >> (-(sht))))

#define INT32_MULAPACKQ(a, b, c) (a += (((b) * (c) + (1 << 14)) >> 15))

#define INT32_MULSPACKQ(a, b, c) (a -= (((b) * (c) + (1 << 14)) >> 15))

#if 1
static inline int32_t NSAu32(uint32_t num) {
  int count = 0;
  uint32_t mask = 0x80000000;
  while (!(num & mask)) {
    count++;
    mask = mask >> 1;
    if (mask == 0)
      break;
  }
  return count;
}

XI_ERR_TYPE
xiWarpPerspective_U8_Q13_18_ref(const xi_pTile src, xi_pTile dst,
                                const xi_perspective_fpt *perspective) {
  int sstride = XI_TILE_GET_PITCH(src);
  int dstride = XI_TILE_GET_PITCH(dst);
  int width = XI_TILE_GET_WIDTH(dst);
  int height = XI_TILE_GET_HEIGHT(dst);
  int edge_width = XI_TILE_GET_EDGE_WIDTH(src);
  int edge_height = XI_TILE_GET_EDGE_HEIGHT(src);

  // move upper left corner to the beginning of the data to exclude negative
  // offsets
  int xs = XI_TILE_GET_X_COORD(src) - edge_width;
  int ys = XI_TILE_GET_Y_COORD(src) - edge_height;
  int xd = XI_TILE_GET_X_COORD(dst);
  int yd = XI_TILE_GET_Y_COORD(dst);

  short xmin = 0;
  short xmax = XT_MAX(0, XI_TILE_GET_WIDTH(src) + 2 * edge_width - 2);
  short ymin = 0;
  short ymax = XT_MAX(0, XI_TILE_GET_HEIGHT(src) + 2 * edge_height - 2);

  uint8_t *psrc =
      (uint8_t *)XI_TILE_GET_DATA_PTR(src) - edge_width - sstride * edge_height;
  uint8_t *pdst = (uint8_t *)(XI_TILE_GET_DATA_PTR(dst));

  XI_Q13_18 a11 = perspective->a11;
  XI_Q13_18 a12 = perspective->a12;
  XI_Q13_18 a13 = perspective->a13;
  XI_Q13_18 a21 = perspective->a21;
  XI_Q13_18 a22 = perspective->a22;
  XI_Q13_18 a23 = perspective->a23;
  XI_Q13_18 a31 = perspective->a31;
  XI_Q13_18 a32 = perspective->a32;
  XI_Q13_18 a33 = perspective->a33;

  // recalculate coefficients in order to get rid of (xs + 0.5)
  // xsrc = ((a11*(xd + 0.5 + x) + a12*(yd + 0.5 + y) + a13) / (a31*(xd + 0.5 +
  // x) + a32*(yd + 0.5 + y) + a33)) - xs - 0.5 =
  //       = (b11*(xd + x) + b12*(yd + y) + (b11 + b12)/2 + b13) / (a31*(xd + x)
  //       + a32*(yd + y) + (a31 + a32)/2 + a33)
  a11 = a11 - a31 * xs - a31 / 2;
  a12 = a12 - a32 * xs - a32 / 2;
  a13 = a13 - a33 * xs - a33 / 2;
  a21 = a21 - a31 * ys - a31 / 2;
  a22 = a22 - a32 * ys - a32 / 2;
  a23 = a23 - a33 * ys - a33 / 2;

  for (int x = 0; x < width; x += 32) {
    for (int y = 0; y < height; ++y) {
      for (int xv = x; (xv < (x + 32)); xv++) {
        unsigned int X =
            ((xv + xd) * a11) + ((y + yd) * a12) + (a11 + a12) / 2 + a13;
        unsigned int Y =
            ((xv + xd) * a21) + ((y + yd) * a22) + (a21 + a22) / 2 + a23;
        unsigned int D =
            ((xv + xd) * a31) + ((y + yd) * a32) + (a31 + a32) / 2 + a33;

        int ns, as;
        ns = NSAu32(D);

        as = 16 - ns;
        D <<= ns;

        unsigned int Q, R;

        // use upper 16 bits of normalized divisor
        Q = (unsigned int)(X / (D >> 16));
        // account for normalization of the divisor (by as=(16-ns))
        // Assumption: there are enough fractional bits in the quotient for good
        // bilinear interpolation
        R = Q << ns;
        Q >>= as;

        // lower 16 bits of Q and R holds the relevant part
        unsigned short Xint = Q & 0xFFFF;
        unsigned short Xrem = R & 0xFFFF;
        Xrem >>= 1; // Q15
        // use upper 16 bits of divisor
        Q = (int)(Y / (D >> 16));
        R = Q << ns;
        Q = Q >> as;

        unsigned short Yint = Q & 0xFFFF;
        unsigned short Yrem = R & 0xFFFF;
        Yrem >>= 1;

        if (Xint >= xmin && Xint <= xmax && Yint >= ymin && Yint <= ymax) {
          uint8_t val00 = psrc[Xint + (Yint * sstride)];
          uint8_t val01 = psrc[Xint + 1 + (Yint * sstride)];
          uint8_t val10 = psrc[Xint + ((Yint + 1) * sstride)];
          uint8_t val11 = psrc[Xint + 1 + ((Yint + 1) * sstride)];

          short curRowIntplVal =
              val00 + ((((val01 - val00) * Xrem) + 0x4000) >> 15);
          short nxtRowIntplVal =
              val10 + ((((val11 - val10) * Xrem) + 0x4000) >> 15);
          short intplValue =
              curRowIntplVal +
              ((((nxtRowIntplVal - curRowIntplVal) * Yrem) + 0x4000) >> 15);
          pdst[xv + (y * dstride)] = (uint8_t)(intplValue & 0xFF);
        } else {
          pdst[xv + (y * dstride)] = 0xFF;
        }
      }
    }
  }

  return (XI_ERR_OK);
}
#else
XI_ERR_TYPE
xiWarpPerspective_U8_Q13_18_ref(const xi_pTile src, xi_pTile dst,
                                const xi_perspective_fpt *perspective) {

  int sstride = XI_TILE_GET_PITCH(src);
  int dstride = XI_TILE_GET_PITCH(dst);
  int width = XI_TILE_GET_WIDTH(dst);
  int height = XI_TILE_GET_HEIGHT(dst);

  float xs = 0.5f + XI_TILE_GET_X_COORD(src);
  float ys = 0.5f + XI_TILE_GET_Y_COORD(src);
  float xd = 0.5f + XI_TILE_GET_X_COORD(dst);
  float yd = 0.5f + XI_TILE_GET_Y_COORD(dst);

  const int xmin = -XI_TILE_GET_EDGE_WIDTH(src);
  const int xmax = XI_TILE_GET_WIDTH(src) + XI_TILE_GET_EDGE_WIDTH(src) - 1;
  const int ymin = -XI_TILE_GET_EDGE_HEIGHT(src);
  const int ymax = XI_TILE_GET_HEIGHT(src) + XI_TILE_GET_EDGE_HEIGHT(src) - 1;

  float a11 = 1.0f * perspective->a11 / (1 << 18);
  float a12 = 1.0f * perspective->a12 / (1 << 18);
  float a13 = 1.0f * perspective->a13 / (1 << 18);
  float a21 = 1.0f * perspective->a21 / (1 << 18);
  float a22 = 1.0f * perspective->a22 / (1 << 18);
  float a23 = 1.0f * perspective->a23 / (1 << 18);
  float a31 = 1.0f * perspective->a31 / (1 << 18);
  float a32 = 1.0f * perspective->a32 / (1 << 18);
  float a33 = 1.0f * perspective->a33 / (1 << 18);

  uint8_t *psrc = (uint8_t *)XI_TILE_GET_DATA_PTR(src);
  uint8_t *pdst = (uint8_t *)XI_TILE_GET_DATA_PTR(dst);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      float xsrc = (a11 * (xd + x) + a12 * (yd + y) + a13);
      float ysrc = (a21 * (xd + x) + a22 * (yd + y) + a23);
      float dvsr = (a31 * (xd + x) + a32 * (yd + y) + a33);

      if (dvsr == 0)
        continue;

      xsrc = xsrc / dvsr - xs;
      ysrc = ysrc / dvsr - ys;

      int xi = xsrc >= 0 ? (int)xsrc : (int)xsrc - 1;
      int yi = ysrc >= 0 ? (int)ysrc : (int)ysrc - 1;
      float xf = xsrc - xi;
      float yf = ysrc - yi;

      int x0 = CLIP(xmin, xmax, xi);
      int x1 = CLIP(xmin, xmax, xi + 1);
      int y0 = CLIP(ymin, ymax, yi);
      int y1 = CLIP(ymin, ymax, yi + 1);

      float val1 =
          psrc[y0 * sstride + x0] * (1.0f - xf) + psrc[y0 * sstride + x1] * xf;
      float val2 =
          psrc[y1 * sstride + x0] * (1.0f - xf) + psrc[y1 * sstride + x1] * xf;

      float v = val1 * (1.0f - yf) + (val2)*yf;
      pdst[y * dstride + x] = (uint8_t)(v + 0.5);
    }
  }

  return XI_ERR_OK;
}

#endif
