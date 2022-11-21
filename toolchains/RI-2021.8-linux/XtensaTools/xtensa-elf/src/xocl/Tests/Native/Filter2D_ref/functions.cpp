#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
//#include "xi_imgproc.h"
//#include "xi_intrin.h"
#include "xi_api_ref.h"
//#include "xi_wide_arithm.h"

// Actual coeff
int8_t filtCoeff[25] = {6, 17, 8,  13, 2, 3,  2,  10, 19, 15, 5,  1, 16,
                        3, 15, 18, 15, 6, 13, 20, 4,  16, 7,  15, 7};
// rearranged coeff for using with MULT
int8_t filtCoeff_1[25] = {6,  17, 8, 13, 3, 2,  10, 19, 5, 1,  16, 3,
                          18, 15, 6, 13, 4, 16, 7,  15, 2, 15, 15, 20};
int32_t filtCoeff_2[5] = {2, 15, 15, 20, 7};

int filt_REF_New(const xi_pTile src, xi_pTile dst) {
  uint8_t *inpBuff, *outBuffRef;

  /*if (!xiTileIsValid_U8_ref(src)) return XI_ERR_BADARG;
  if (!xiTileIsValid_U8_ref(dst)) return XI_ERR_BADARG;*/

  inpBuff = (uint8_t *)XI_TILE_GET_BUFF_PTR(src);
  outBuffRef = (uint8_t *)XI_TILE_GET_DATA_PTR(dst);
  uint32_t SrcPitch = XI_TILE_GET_PITCH(src);
  uint32_t DstPitch = XI_TILE_GET_PITCH(dst);
  int32_t x, y;
  uint32_t Acc;
  for (y = 0; y < XI_TILE_GET_HEIGHT(src) - 4; y++) {
    for (x = 0; x < XI_TILE_GET_WIDTH(src) - 4; x++) {
      Acc = filtCoeff[0] * inpBuff[y * SrcPitch + x] +
            filtCoeff[1] * inpBuff[y * SrcPitch + x + 1] +
            filtCoeff[2] * inpBuff[y * SrcPitch + x + 2] +
            filtCoeff[3] * inpBuff[y * SrcPitch + x + 3] +
            filtCoeff[4] * inpBuff[y * SrcPitch + x + 4] +
            filtCoeff[5] * inpBuff[(y + 1) * SrcPitch + x] +
            filtCoeff[6] * inpBuff[(y + 1) * SrcPitch + x + 1] +
            filtCoeff[7] * inpBuff[(y + 1) * SrcPitch + x + 2] +
            filtCoeff[8] * inpBuff[(y + 1) * SrcPitch + x + 3] +
            filtCoeff[9] * inpBuff[(y + 1) * SrcPitch + x + 4] +
            filtCoeff[10] * inpBuff[(y + 2) * SrcPitch + x] +
            filtCoeff[11] * inpBuff[(y + 2) * SrcPitch + x + 1] +
            filtCoeff[12] * inpBuff[(y + 2) * SrcPitch + x + 2] +
            filtCoeff[13] * inpBuff[(y + 2) * SrcPitch + x + 3] +
            filtCoeff[14] * inpBuff[(y + 2) * SrcPitch + x + 4] +
            filtCoeff[15] * inpBuff[(y + 3) * SrcPitch + x] +
            filtCoeff[16] * inpBuff[(y + 3) * SrcPitch + x + 1] +
            filtCoeff[17] * inpBuff[(y + 3) * SrcPitch + x + 2] +
            filtCoeff[18] * inpBuff[(y + 3) * SrcPitch + x + 3] +
            filtCoeff[19] * inpBuff[(y + 3) * SrcPitch + x + 4] +
            filtCoeff[20] * inpBuff[(y + 4) * SrcPitch + x] +
            filtCoeff[21] * inpBuff[(y + 4) * SrcPitch + x + 1] +
            filtCoeff[22] * inpBuff[(y + 4) * SrcPitch + x + 2] +
            filtCoeff[23] * inpBuff[(y + 4) * SrcPitch + x + 3] +
            filtCoeff[24] * inpBuff[(y + 4) * SrcPitch + x + 4];

      outBuffRef[y * DstPitch + x] = (uint8_t)(Acc >> 8);
    }
  }

  return XI_ERR_OK;
}
