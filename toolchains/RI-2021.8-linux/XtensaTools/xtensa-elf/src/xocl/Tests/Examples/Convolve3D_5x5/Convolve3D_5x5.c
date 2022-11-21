#include "xi_lib.h"

#include <xtensa/tie/xt_ivpn.h>
#include <xtensa/tie/xt_misc.h>

#define OFFSET_PTR_N_2X32U(ptr, nrows, stride, in_row_offset)                  \
  ((xb_vecN_2x32Uv *)((uint32_t *)(ptr) + (in_row_offset) + (nrows) * (stride)))
#define OFFSET_PTR_N_2X32(ptr, nrows, stride, in_row_offset)                   \
  ((xb_vecN_2x32v *)((int32_t *)(ptr) + (in_row_offset) + (nrows) * (stride)))
#define OFFSET_PTR_NX16U(ptr, nrows, stride, in_row_offset)                    \
  ((xb_vecNx16U *)((uint16_t *)(ptr) + (in_row_offset) + (nrows) * (stride)))
#define OFFSET_PTR_NX16(ptr, nrows, stride, in_row_offset)                     \
  ((xb_vecNx16 *)((int16_t *)(ptr) + (in_row_offset) + (nrows) * (stride)))
#define OFFSET_PTR_N_2X16(ptr, nrows, stride, in_row_offset)                   \
  ((xb_vecN_2x16 *)((int16_t *)(ptr) + (in_row_offset) + (nrows) * (stride)))
#define OFFSET_PTR_NX8U(ptr, nrows, stride, in_row_offset)                     \
  ((xb_vecNx8U *)((uint8_t *)(ptr) + (in_row_offset) + (nrows) * (stride)))
#define OFFSET_PTR_NX8(ptr, nrows, stride, in_row_offset)                      \
  ((xb_vecNx8 *)((int8_t *)(ptr) + (in_row_offset) + (nrows) * (stride)))
#define OFFSET_PTR_2NX8U(ptr, nrows, stride, in_row_offset)                    \
  ((xb_vec2Nx8U *)((uint8_t *)(ptr) + (in_row_offset) + (nrows) * (stride)))
#define OFFSET_PTR_2NX8(ptr, nrows, stride, in_row_offset)                     \
  ((xb_vec2Nx8 *)((int8_t *)(ptr) + (in_row_offset) + (nrows) * (stride)))

/*****************************************************************************************************************
 * Output-depth filters of size 5x5xinput-depth are applied to input to produce
 *     output-depth x (input-width - 4) x (input-height - 4) outputs.
 *     P6 version - 8b x 8b
 ******************************************************************************************************************/
void conv5x5xD8b_P6(int8_t *__restrict pIn, int8_t *__restrict pCoeff,
                    int8_t *__restrict pOut, int width, int height, int depth,
                    int pitch, int odepth, int shift) {
  int owidth = width - 4, oheight = height - 4,
      pwidth = 2 * XCHAL_IVPN_SIMD_WIDTH;
  int w, h, d, id;
  xb_vec2Nx8 *__restrict pvecCoeff = (xb_vec2Nx8 *)pCoeff;
  xb_vec2Nx8 *__restrict pvecCoeff2 = (xb_vec2Nx8 *)&pCoeff[64];
  xb_vec2Nx8 *__restrict pvecOut = (xb_vec2Nx8 *)pOut;
  xb_vec2Nx8 vecC;
  xb_vec2Nx8 vecC2;
  xb_vecNx16 vecIn;
  int16_t feat_sh[2];
  int32_t feat;
  int8_t *__restrict pInCurr;

  for (h = 0; h < oheight; h++) {
    for (w = 0; w < owidth; w++) {
      for (d = 0; d < odepth;
           d += pwidth) { // producing pwidth outputs at a time
        pvecCoeff = (xb_vec2Nx8 *)pCoeff;
        pvecCoeff2 = (xb_vec2Nx8 *)&pCoeff[64];
        xb_vec2Nx24 wvec = IVP_MUL2N8XR16(vecC, 0);
        // idepth x 5 x 5 filter for pwidth outputs
        for (id = 0; id < depth; id++) {
          pInCurr = &pIn[id /**height*pitch*/ + h * pitch * depth + w * depth];
          // valign vaIn = IVP_LANX8S_PP((xb_vecNx8*)pInCurr);
          // IVP_LANX8S_XP(vecIn, vaIn, (xb_vecNx8*)pInCurr, pitch);
          // feat = IVP_EXTRN_2X32(IVP_MOVN_2X32_FROMNX16(vecIn), 0); // 0,1
          feat_sh[0] = *pInCurr;
          pInCurr += depth;
          feat_sh[1] = *pInCurr;
          pInCurr += depth;
          feat = *((int32_t *)&feat_sh);
          vecC = *pvecCoeff;
          pvecCoeff += 2;
          vecC2 = *pvecCoeff2;
          pvecCoeff2 += 2;
          IVP_MULPA2N8XR16(wvec, vecC2, vecC, feat);
          // feat = IVP_EXTRN_2X32(IVP_MOVN_2X32_FROMNX16(vecIn), 1); // 2,3
          feat_sh[0] = *pInCurr;
          pInCurr += depth;
          feat_sh[1] = *pInCurr;
          pInCurr += depth;
          feat = *((int32_t *)&feat_sh);
          vecC = *pvecCoeff;
          pvecCoeff += 2;
          vecC2 = *pvecCoeff2;
          pvecCoeff2 += 3;
          IVP_MULPA2N8XR16(wvec, vecC2, vecC, feat);
          // feat = IVP_EXTRNX16(vecIn, 4); // 4
          feat_sh[0] = *pInCurr;
          pInCurr += depth;
          feat = *((int32_t *)&feat_sh);
          vecC = *pvecCoeff++;
          IVP_MULA2N8XR16(wvec, vecC, feat);

          pInCurr =
              &pIn[id /**height*pitch*/ + (h + 1) * pitch * depth + w * depth];
          // vaIn = IVP_LANX8S_PP((xb_vecNx8*)pInCurr);
          // IVP_LANX8S_XP(vecIn, vaIn, (xb_vecNx8*)pInCurr, pitch);
          feat_sh[0] = *pInCurr;
          pInCurr += depth;
          feat_sh[1] = *pInCurr;
          pInCurr += depth;
          feat = *((int32_t *)&feat_sh);
          // feat = IVP_EXTRN_2X32(IVP_MOVN_2X32_FROMNX16(vecIn), 0); // 0,1
          vecC = *pvecCoeff;
          pvecCoeff += 2;
          vecC2 = *pvecCoeff2;
          pvecCoeff2 += 2;
          IVP_MULPA2N8XR16(wvec, vecC2, vecC, feat);
          // feat = IVP_EXTRN_2X32(IVP_MOVN_2X32_FROMNX16(vecIn), 1); // 2,3
          feat_sh[0] = *pInCurr;
          pInCurr += depth;
          feat_sh[1] = *pInCurr;
          pInCurr += depth;
          feat = *((int32_t *)&feat_sh);
          vecC = *pvecCoeff;
          pvecCoeff += 2;
          vecC2 = *pvecCoeff2;
          pvecCoeff2 += 3;
          IVP_MULPA2N8XR16(wvec, vecC2, vecC, feat);
          // feat = IVP_EXTRNX16(vecIn, 4); // 4
          feat_sh[0] = *pInCurr;
          pInCurr += depth;
          feat = *((int32_t *)&feat_sh);
          vecC = *pvecCoeff++;
          IVP_MULA2N8XR16(wvec, vecC, feat);

          pInCurr =
              &pIn[id /**height*pitch*/ + (h + 2) * pitch * depth + w * depth];
          feat_sh[0] = *pInCurr;
          pInCurr += depth;
          feat_sh[1] = *pInCurr;
          pInCurr += depth;
          feat = *((int32_t *)&feat_sh);
          // vaIn = IVP_LANX8S_PP((xb_vecNx8*)pInCurr);
          // IVP_LANX8S_XP(vecIn, vaIn, (xb_vecNx8*)pInCurr, pitch);
          // feat = IVP_EXTRN_2X32(IVP_MOVN_2X32_FROMNX16(vecIn), 0); // 0,1
          vecC = *pvecCoeff;
          pvecCoeff += 2;
          vecC2 = *pvecCoeff2;
          pvecCoeff2 += 2;
          IVP_MULPA2N8XR16(wvec, vecC2, vecC, feat);
          // feat = IVP_EXTRN_2X32(IVP_MOVN_2X32_FROMNX16(vecIn), 1); // 2,3
          vecC = *pvecCoeff;
          pvecCoeff += 2;
          vecC2 = *pvecCoeff2;
          pvecCoeff2 += 3;
          feat_sh[0] = *pInCurr;
          pInCurr += depth;
          feat_sh[1] = *pInCurr;
          pInCurr += depth;
          feat = *((int32_t *)&feat_sh);
          IVP_MULPA2N8XR16(wvec, vecC2, vecC, feat);
          // feat = IVP_EXTRNX16(vecIn, 4); // 4
          feat_sh[0] = *pInCurr;
          pInCurr += depth;
          feat = *((int32_t *)&feat_sh);
          vecC = *pvecCoeff++;
          IVP_MULA2N8XR16(wvec, vecC, feat);

          // vaIn = IVP_LANX8S_PP((xb_vecNx8*)pInCurr);
          // IVP_LANX8S_XP(vecIn, vaIn, (xb_vecNx8*)pInCurr, pitch);
          // feat = IVP_EXTRN_2X32(IVP_MOVN_2X32_FROMNX16(vecIn), 0); // 0,1
          pInCurr =
              &pIn[id /**height*pitch*/ + (h + 3) * pitch * depth + w * depth];
          feat_sh[0] = *pInCurr;
          pInCurr += depth;
          feat_sh[1] = *pInCurr;
          pInCurr += depth;
          feat = *((int32_t *)&feat_sh);
          vecC = *pvecCoeff;
          pvecCoeff += 2;
          vecC2 = *pvecCoeff2;
          pvecCoeff2 += 2;
          IVP_MULPA2N8XR16(wvec, vecC2, vecC, feat);
          feat_sh[0] = *pInCurr;
          pInCurr += depth;
          feat_sh[1] = *pInCurr;
          pInCurr += depth;
          feat = *((int32_t *)&feat_sh);
          // feat = IVP_EXTRN_2X32(IVP_MOVN_2X32_FROMNX16(vecIn), 1); // 2,3
          vecC = *pvecCoeff;
          pvecCoeff += 2;
          vecC2 = *pvecCoeff2;
          pvecCoeff2 += 3;
          IVP_MULPA2N8XR16(wvec, vecC2, vecC, feat);
          // feat = IVP_EXTRNX16(vecIn, 4); // 4
          feat_sh[0] = *pInCurr;
          pInCurr += depth;
          feat = *((int32_t *)&feat_sh);
          vecC = *pvecCoeff++;
          IVP_MULA2N8XR16(wvec, vecC, feat);

          // vaIn = IVP_LANX8S_PP((xb_vecNx8*)pInCurr);
          // IVP_LANX8S_XP(vecIn, vaIn, (xb_vecNx8*)pInCurr, pitch);
          // feat = IVP_EXTRN_2X32(IVP_MOVN_2X32_FROMNX16(vecIn), 0); // 0,1
          pInCurr =
              &pIn[id /**height*pitch*/ + (h + 4) * pitch * depth + w * depth];
          feat_sh[0] = *pInCurr;
          pInCurr += depth;
          feat_sh[1] = *pInCurr;
          pInCurr += depth;
          feat = *((int32_t *)&feat_sh);
          vecC = *pvecCoeff;
          pvecCoeff += 2;
          vecC2 = *pvecCoeff2;
          pvecCoeff2 += 2;
          IVP_MULPA2N8XR16(wvec, vecC2, vecC, feat);
          // feat = IVP_EXTRN_2X32(IVP_MOVN_2X32_FROMNX16(vecIn), 1); // 2,3
          feat_sh[0] = *pInCurr;
          pInCurr += depth;
          feat_sh[1] = *pInCurr;
          pInCurr += depth;
          feat = *((int32_t *)&feat_sh);
          vecC = *pvecCoeff;
          pvecCoeff += 2;
          vecC2 = *pvecCoeff2;
          pvecCoeff2 += 3;
          IVP_MULPA2N8XR16(wvec, vecC2, vecC, feat);
          // feat = IVP_EXTRNX16(vecIn, 4); // 4
          feat_sh[0] = *pInCurr;
          pInCurr += depth;
          feat = *((int32_t *)&feat_sh);
          vecC = *pvecCoeff++;
          IVP_MULA2N8XR16(wvec, vecC, feat);
        }
        xb_vec2Nx8 vecOut = IVP_PACKVR2NX24(wvec, shift);
        *pvecOut++ = vecOut;
      }
    }
  }
}