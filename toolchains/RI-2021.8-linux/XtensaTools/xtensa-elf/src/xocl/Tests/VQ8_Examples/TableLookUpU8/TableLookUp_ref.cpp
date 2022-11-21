/*
 * Copyright (c) 2014-2017 Tensilica Inc. ALL RIGHTS RESERVED.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "xi_api_ref.h"

#define XI_MAX(a, b) (a) > (b) ? (a) : (b)

XI_ERR_TYPE xiLUT_U8_ref(const xi_pTile src, xi_pTile dst, uint8_t *lut,
                         int size) {
  const int sstride = XI_TILE_GET_PITCH(src);
  const int dstride = XI_TILE_GET_PITCH(dst);
  int width = XI_TILE_GET_WIDTH(src);
  int height = XI_TILE_GET_HEIGHT(src);

  uint8_t *src_ptr = (uint8_t *)XI_TILE_GET_DATA_PTR(src);
  uint8_t *dst_ptr = (uint8_t *)XI_TILE_GET_DATA_PTR(dst);

  int n = 512 / size;
  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j++) {
      int id = src_ptr[i * sstride + j] / n;
      dst_ptr[i * dstride + j] = lut[id];
    }
  }

  return XI_ERR_OK;
}
