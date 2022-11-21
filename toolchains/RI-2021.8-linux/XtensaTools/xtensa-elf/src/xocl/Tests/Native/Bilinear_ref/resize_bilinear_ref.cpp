#include "xi_api_ref.h"

#define XI_ENABLE_BIT_EXACT_CREF
#ifdef XI_ENABLE_BIT_EXACT_CREF

#define XT_MIN(a, b) a < b ? a : b
#define XT_MAX(a, b) a < b ? b : a

XI_ERR_TYPE xiResizeBilinear_U8_Q13_18_ref(const xi_pTile src, xi_pTile dst,
                                           XI_Q13_18 xscale, XI_Q13_18 yscale,
                                           XI_Q13_18 xshift, XI_Q13_18 yshift) {
  /*if (!xiTileIsValid_U8_ref(src)) return XI_ERR_BADARG;
  if (!xiTileIsValid_U8_ref(dst)) return XI_ERR_BADARG;*/

  int xs = XI_TILE_GET_X_COORD(src);
  int ys = XI_TILE_GET_Y_COORD(src);
  int xd = XI_TILE_GET_X_COORD(dst);
  int yd = XI_TILE_GET_Y_COORD(dst);

  int xl = (xd * xscale + xshift + (xscale >> 1) - (1 << 17)) >> 18;
  int yl = (yd * yscale + yshift + (yscale >> 1) - (1 << 17)) >> 18;
  int xr = (((xd + XI_TILE_GET_WIDTH(dst) - 1) * xscale + xshift +
             (xscale >> 1) - (1 << 17)) >>
            18);
  int yr = (((yd + XI_TILE_GET_HEIGHT(dst) - 1) * yscale + yshift +
             (yscale >> 1) - (1 << 17)) >>
            18);

  int xmin = XT_MIN(xl, xr);
  int xmax = XT_MAX(xl, xr) + 1;
  int ymin = XT_MIN(yl, yr);
  int ymax = XT_MAX(yl, yr) + 1;

  if (xmin < xs - XI_TILE_GET_EDGE_WIDTH(src) ||
      ymin < ys - XI_TILE_GET_EDGE_HEIGHT(src))
    return XI_ERR_DATASIZE;
  if (xmax >= xs + XI_TILE_GET_WIDTH(src) + XI_TILE_GET_EDGE_WIDTH(src) ||
      ymax >= ys + XI_TILE_GET_HEIGHT(src) + XI_TILE_GET_EDGE_HEIGHT(src))
    return XI_ERR_DATASIZE;

  int sstride = XI_TILE_GET_PITCH(src);
  int dstride = XI_TILE_GET_PITCH(dst);

  uint8_t *psrc = (uint8_t *)XI_TILE_GET_DATA_PTR(src);
  uint8_t *pdst = (uint8_t *)XI_TILE_GET_DATA_PTR(dst);

  for (int i = 0; i < XI_TILE_GET_HEIGHT(dst); ++i) {
    XI_Q13_18 yoffs =
        ((yd + i) * yscale + yshift - (ys << 18) + (yscale >> 1) - (1 << 17)) +
        4; // +4 for yf calculation round in cycle
    int16_t yf = (yoffs & ((1 << 18) - 1)) >> 3;
    int16_t y0 = (yoffs >> 18);

    for (int j = 0; j < XI_TILE_GET_WIDTH(dst); ++j) {
      XI_Q13_18 x =
          (xd + j) * xscale + xshift - (xs << 18) + (xscale >> 1) - (1 << 17);
      int x0 = x >> 18;
      int xf = (x & ((1 << 18) - 1)) >> 3;

      int dst00 = psrc[(y0 + 0) * sstride + x0];
      int dst10 = psrc[(y0 + 1) * sstride + x0];
      int dst01 = psrc[(y0 + 0) * sstride + (x0 + 1)];
      int dst11 = psrc[(y0 + 1) * sstride + (x0 + 1)];

      dst00 = (dst00 * ((1 << 15) - yf) + dst10 * yf + (1 << 14)) >> 15;
      dst01 = (dst01 * ((1 << 15) - yf) + dst11 * yf + (1 << 14)) >> 15;
      dst00 = (dst00 * ((1 << 15) - xf) + dst01 * xf + (1 << 14)) >> 15;

      pdst[i * dstride + j] = dst00;
    }
  }

  return XI_ERR_OK;
}

#else  // not bit-exact code

XI_ERR_TYPE xiResizeBilinear_U8_Q13_18_ref(const xi_pTile src, xi_pTile dst,
                                           XI_Q13_18 xscale, XI_Q13_18 yscale,
                                           XI_Q13_18 xshift, XI_Q13_18 yshift) {
  if (!xiTileIsValid_U8_ref(src))
    return XI_ERR_BADARG;
  if (!xiTileIsValid_U8_ref(dst))
    return XI_ERR_BADARG;

  int xs = XI_TILE_GET_X_COORD(src);
  int ys = XI_TILE_GET_Y_COORD(src);
  int xd = XI_TILE_GET_X_COORD(dst);
  int yd = XI_TILE_GET_Y_COORD(dst);

  int xl = (xd * xscale + xshift + (xscale >> 1) - (1 << 17)) >> 18;
  int yl = (yd * yscale + yshift + (yscale >> 1) - (1 << 17)) >> 18;
  int xr = (((xd + XI_TILE_GET_WIDTH(dst) - 1) * xscale + xshift +
             (xscale >> 1) - (1 << 17)) >>
            18);
  int yr = (((yd + XI_TILE_GET_HEIGHT(dst) - 1) * yscale + yshift +
             (yscale >> 1) - (1 << 17)) >>
            18);

  int xmin = XT_MIN(xl, xr);
  int xmax = XT_MAX(xl, xr) + 1;
  int ymin = XT_MIN(yl, yr);
  int ymax = XT_MAX(yl, yr) + 1;

  if (xmin < xs - XI_TILE_GET_EDGE_WIDTH(src) ||
      ymin < ys - XI_TILE_GET_EDGE_HEIGHT(src))
    return XI_ERR_DATASIZE;
  if (xmax >= xs + XI_TILE_GET_WIDTH(src) + XI_TILE_GET_EDGE_WIDTH(src) ||
      ymax >= ys + XI_TILE_GET_HEIGHT(src) + XI_TILE_GET_EDGE_HEIGHT(src))
    return XI_ERR_DATASIZE;

  uint8_t *psrc = (uint8_t *)XI_TILE_GET_DATA_PTR(src);
  uint8_t *pdst = (uint8_t *)XI_TILE_GET_DATA_PTR(dst);

  int sstride = XI_TILE_GET_PITCH(src);
  int dstride = XI_TILE_GET_PITCH(dst);

  for (int i = 0; i < XI_TILE_GET_HEIGHT(dst); ++i) {
    XI_Q13_18 y =
        (yd + i) * yscale + yshift - (ys << 18) + (yscale >> 1) - (1 << 17);
    int y0 = y >> 18;
    XI_Q13_18 wy = y & ((1 << 18) - 1);

    for (int j = 0; j < XI_TILE_GET_WIDTH(dst); ++j) {
      XI_Q13_18 x =
          (xd + j) * xscale + xshift - (xs << 18) + (xscale >> 1) - (1 << 17);
      int x0 = x >> 18;
      XI_Q13_18 wx = x & ((1 << 18) - 1);

      XI_Q13_18 val0 = psrc[(y0 + 0) * sstride + x0] * ((1 << 18) - wx) +
                       psrc[(y0 + 0) * sstride + x0 + 1] * wx;
      XI_Q13_18 val1 = psrc[(y0 + 1) * sstride + x0] * ((1 << 18) - wx) +
                       psrc[(y0 + 1) * sstride + x0 + 1] * wx;

      val0 = (val0 + (1 << 12)) >> 13; // round to Q8.5
      val1 = (val1 + (1 << 12)) >> 13;

      pdst[i * dstride + j] =
          (val0 * ((1 << 18) - wy) + val1 * wy + (1 << 22)) >> 23;
    }
  }

  return XI_ERR_OK;
}
#endif // #ifdef XI_ENABLE_BIT_EXACT_CREF
