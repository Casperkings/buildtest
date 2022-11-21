#include "xi_lib.h"
#include <xtensa/tie/xt_ivpn.h>
#include <xtensa/tie/xt_misc.h>

int filt_IVP_New(const xi_pTile src, xi_pTile dst, int8_t *filtCoeff_1) {
  int32_t x, y, *__restrict s0s1;
  xb_vec2Nx8U *__restrict pvecImg, *__restrict pvecDst;
  xb_vec2Nx8U vec00, vec01, vec04;
  xb_vec2Nx8U vec10, vec11, vec14;
  xb_vec2Nx8U vec20, vec21, vec24;
  xb_vec2Nx8U vec30, vec31, vec34;
  xb_vec2Nx8U vec40, vec41, vec44;
  xb_vec2Nx24 vecAcc, vecAcc1;
  xb_vecNx8U *__restrict pvecDst_1;
  xb_vecNx16U vecEven, vecOdd, vecH, vecL;

  uint8_t *inpBuff, *outBuff;

  /*if (!xiTileIsValid_U8_ref(src)) return XI_ERR_BADARG;
  if (!xiTileIsValid_U8_ref(dst)) return XI_ERR_BADARG;*/

  inpBuff = (uint8_t *)XI_TILE_GET_BUFF_PTR(src);
  outBuff = (uint8_t *)XI_TILE_GET_DATA_PTR(dst);

  uint32_t Pitch = XI_TILE_GET_PITCH(src);
  uint32_t DstPitch = XI_TILE_GET_PITCH(dst);
  //############ IVP Code ############//
  s0s1 = (int32_t *)filtCoeff_1;

  for (x = 0; x < XI_TILE_GET_WIDTH(dst); x += 64) {
    pvecImg = (xb_vec2Nx8U *)(inpBuff + x);
    valign a = IVP_LA2NX8U_PP(pvecImg);
    IVP_LA2NX8_IP(vec00, a, pvecImg);
    IVP_LA2NX8_IP(vec01, a, pvecImg);
    // vec00 = *pvecImg++;	vec01 = *pvecImg;
    vec04 = IVP_SEL2NX8I(vec01, vec00, IVP_SELI_8B_ROTATE_RIGHT_4);

    pvecImg = (xb_vec2Nx8U *)(inpBuff + Pitch + x);
    // vec10 = *pvecImg++;	vec11 = *pvecImg;
    a = IVP_LA2NX8U_PP(pvecImg);
    IVP_LA2NX8_IP(vec10, a, pvecImg);
    IVP_LA2NX8_IP(vec11, a, pvecImg);
    vec14 = IVP_SEL2NX8I(vec11, vec10, IVP_SELI_8B_ROTATE_RIGHT_4);

    pvecImg = (xb_vec2Nx8U *)(inpBuff + 2 * Pitch + x);
    // vec20 = *pvecImg++;	vec21 = *pvecImg;
    a = IVP_LA2NX8U_PP(pvecImg);
    IVP_LA2NX8_IP(vec20, a, pvecImg);
    IVP_LA2NX8_IP(vec21, a, pvecImg);
    vec24 = IVP_SEL2NX8I(vec21, vec20, IVP_SELI_8B_ROTATE_RIGHT_4);

    pvecImg = (xb_vec2Nx8U *)(inpBuff + 3 * Pitch + x);
    // vec30 = *pvecImg++;	vec31 = *pvecImg;
    a = IVP_LA2NX8U_PP(pvecImg);
    IVP_LA2NX8_IP(vec30, a, pvecImg);
    IVP_LA2NX8_IP(vec31, a, pvecImg);
    vec34 = IVP_SEL2NX8I(vec31, vec30, IVP_SELI_8B_ROTATE_RIGHT_4);

    pvecDst = (xb_vec2Nx8U *)(outBuff + x);
    pvecDst_1 = (xb_vecNx8U *)(outBuff + x);
    pvecImg = (xb_vec2Nx8U *)(inpBuff + 4 * Pitch + x);
    for (y = 0; y < XI_TILE_GET_HEIGHT(dst); y++) {
      // load data for the 5th line of the filter mask
      valign a = IVP_LA2NX8U_PP(pvecImg);
      IVP_LA2NX8_IP(vec40, a, pvecImg);
      IVP_LA2NX8_XP(vec41, a, pvecImg, Pitch - 64);
      // vec41 = IVP_LV2NX8U_I(pvecImg, 64);
      // IVP_LV2NX8U_XP(vec40, pvecImg, Pitch);
      vec44 = IVP_SEL2NX8I(vec41, vec40, IVP_SELI_8B_ROTATE_RIGHT_4);
      // filtering operation

      vecAcc = IVP_MULUS4T2N8XR8(vec01, vec00, s0s1[0]);
      // IVP_MULUSA2N8XR16(vecAcc, vec04, filtCoeff_2[0]);
      IVP_MULUS4TA2N8XR8(vecAcc, vec11, vec10, s0s1[1]);
      // IVP_MULUSA2N8XR16(vecAcc, vec14, filtCoeff_2[1]);
      IVP_MULUS4TA2N8XR8(vecAcc, vec21, vec20, s0s1[2]);

      vecAcc1 = IVP_MULUS2N8XR16(vec44, filtCoeff_1[28]);
      // IVP_MULUSA2N8XR16(vecAcc1, vec24, filtCoeff_2[2]);
      IVP_MULUS4TA2N8XR8(vecAcc1, vec31, vec30, s0s1[3]);
      // IVP_MULUSA2N8XR16(vecAcc1, vec34, filtCoeff_2[3]);
      IVP_MULUS4TA2N8XR8(vecAcc1, vec41, vec40, s0s1[4]);
      IVP_MULUSQA2N8XR8(vecAcc1, vec34, vec24, vec14, vec04, s0s1[5]);

      vecEven = IVP_PACKVRNR2NX24_0(vecAcc, 0);
      vecOdd = IVP_PACKVRNR2NX24_1(vecAcc, 0);

      vecEven = vecEven + IVP_PACKVRNR2NX24_0(vecAcc1, 0);
      vecOdd = vecOdd + IVP_PACKVRNR2NX24_1(vecAcc1, 0);

      vecL = IVP_SELNX16I(vecOdd, vecEven, IVP_SELI_INTERLEAVE_1_LO);
      vecH = IVP_SELNX16I(vecOdd, vecEven, IVP_SELI_INTERLEAVE_1_HI);

      IVP_SVNX8U_XP(vecL >> 8, pvecDst_1, 32);
      IVP_SVNX8U_XP(vecH >> 8, pvecDst_1, 32);
      pvecDst_1 += (DstPitch >> 5) - 2;
      // swap the vectors for reuse
      vec00 = vec10;
      vec01 = vec11;
      vec04 = vec14;
      vec10 = vec20;
      vec11 = vec21;
      vec14 = vec24;
      vec20 = vec30;
      vec21 = vec31;
      vec24 = vec34;
      vec30 = vec40;
      vec31 = vec41;
      vec34 = vec44;
    }
  }
  return 0;
}
