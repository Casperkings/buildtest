#include "xi_tile3d_manager.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

void conv5x5xD8b_Ref(xi_pTile3D pSrc, xi_pTile3D pDst,
                     int8_t *__restrict pCoeff, int shift) {
  int8_t *__restrict pIn;
  int8_t *__restrict pOut;
  int depth;
  int pitch;
  int odepth;
  int w, h, d, id, fw, fh;
  int sum;

  pIn = (int8_t *)XI_TILE3D_GET_DATA_PTR(pSrc);
  pOut = (int8_t *)XI_TILE3D_GET_DATA_PTR(pDst);

  depth = XI_TILE3D_GET_DIM3(pSrc);
  pitch = XI_TILE3D_GET_DIM1_PITCH(pSrc);
  odepth = XI_TILE3D_GET_DIM3(pDst);

  int owidth = XI_TILE3D_GET_DIM1(pDst);
  int oheight = XI_TILE3D_GET_DIM2(pDst);

  for (h = 0; h < oheight; h++) {
    for (w = 0; w < owidth; w++) {
      for (d = 0; d < odepth; d++) {
        sum = 0;
        // idepth x 5 x 5 filter for current output
        for (id = 0; id < depth; id++) {
          for (fh = 0; fh < 5; fh++) {
            for (fw = 0; fw < 5; fw++) {
              sum +=
                  (int)pIn[id + (h + fh) * pitch * depth + (w + fw) * depth] *
                  pCoeff[id * odepth * 5 * 5 + odepth * (fh * 5 + fw) + d];
            }
          }
        }
        pOut[h * owidth * odepth + w * odepth + d] =
            ((int)((sum + (1 << (shift - 1))) >> shift));
      }
    }
  }
}
