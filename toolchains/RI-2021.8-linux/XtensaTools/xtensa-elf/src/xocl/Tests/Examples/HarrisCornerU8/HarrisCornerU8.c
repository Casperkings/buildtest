#include "xi_lib.h"

#include <xtensa/tie/xt_ivpn.h>
#include <xtensa/tie/xt_misc.h>
#include <math.h>
//#include "xi_intrin.h"
//#include "xi_wide_arithm.h"
//#include "xi_imgproc.h"
//#include "xi_intrin.h"
//#include "xi_wide_arithm.h"

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

#define IVP_SAVNX8UPOS_FP   IVP_SAPOSNX8U_FP
#define IVP_SAV2NX8UPOS_FP  IVP_SAPOS2NX8U_FP
#define FIX_SOBEL_ROUNDING1 1

void xvCornerHarris5x5Tile(int8_t *p_s8Dx, int8_t *p_s8Dy, uint8_t *p_u8In,
                           uint8_t *p_u8Out, uint32_t u32Width,
                           uint32_t u32Height, int32_t s32SrcStride,
                           uint32_t u32Shift) {
  uint32_t i, j, k, count, totalCount = 0;
  xb_vec2Nx8 dvecSx0, dvecSy0, dvecSx1, dvecSy1, dvecS0, dvecS1, dvecS2, dvecS3,
      dvecT0, dvecT1, dvecT2, dvecT3, dvecT3P, dvecS3P;
  xb_vec2Nx8 *__restrict pdvecSx, *__restrict pdvecSy;
  xb_vec2Nx8 *__restrict pdvecSxTemp, *__restrict pdvecSyTemp;
  xb_vec2Nx8U *__restrict pdvecDst2N;
  xb_vecNx16 vecX0, vecX1, vecY0, vecY1, vecXY0, vecXY1, vecR0, vecR1;
  xb_vecNx48 accR0;
  xb_vec2Nx24 dAccX, dAccY, dAccXY;
  valign vaX, vaY;
  valign vaXTemp, vaYTemp;
  valign vaP, vaC, vaN;
  xb_vecNx16 vecSum0, vecSum1;
  xb_vec2Nx8U vecHarrisScore;
  vbool2N bvecThreshold;
  vboolN bvecThresholdL, bvecThresholdH;
  xb_vecNx16 vecIndex, vecIndexL, vecIndexH;
  valign vaStore = IVP_ZALIGN();

  /***************************
       Dx |-1 0 1|  Dy |-1 -2 -1|
       |-2 0 2|     | 0  0  0|
       |-1 0 1|     | 1  2  1|
       Loc |v0 v3 v5|
       |v1    v6|
       |v2 v4 v7| v4, v0, v3, v5,
   ****************************/
  uint32_t coef1 = 0x02fffeff;
  xb_vec2Nx8 *__restrict ptr_Dx = (xb_vec2Nx8 *)p_s8Dx;
  xb_vec2Nx8 *__restrict ptr_Dy = (xb_vec2Nx8 *)p_s8Dy;
  xb_vec2Nx8 vc0, vc1, vc2, vp0, vp1, vp2, vn0, vn1, vn2;
  xb_vec2Nx8 *__restrict ptr_im_c = (xb_vec2Nx8 *)(p_u8In + s32SrcStride);
  xb_vec2Nx8 *__restrict ptr_im_p = (xb_vec2Nx8 *)p_u8In;
  xb_vec2Nx8 *__restrict ptr_im_n = (xb_vec2Nx8 *)(p_u8In + 2 * s32SrcStride);

  vaC = IVP_LA2NX8_PP(ptr_im_c);
  vaP = IVP_LA2NX8_PP(ptr_im_p);
  vaN = IVP_LA2NX8_PP(ptr_im_n);

  vc0 = 0;
  vp0 = 0;
  vn0 = 0;
  IVP_LA2NX8_IP(vp1, vaP, ptr_im_p);
  IVP_LA2NX8_IP(vc1, vaC, ptr_im_c);
  IVP_LA2NX8_IP(vn1, vaN, ptr_im_n);
  uint32_t loop = ((u32Height + 4) * s32SrcStride + 63) >> 6;

  xb_vec2Nx24 acc_dx, acc_dy;
  xb_vec2Nx8 v0, v1, v2, v3, v4, v5, v6, v7;
  xb_vec2Nx8 outX, outY;

  for (i = 0; i < loop; i++) {
    IVP_LA2NX8_IP(vp2, vaP, ptr_im_p);
    IVP_LA2NX8_IP(vc2, vaC, ptr_im_c);
    IVP_LA2NX8_IP(vn2, vaN, ptr_im_n);
    v0 = IVP_SEL2NX8I(vp1, vp0, IVP_SELI_8B_ROTATE_LEFT_1);
    v1 = IVP_SEL2NX8I(vc1, vc0, IVP_SELI_8B_ROTATE_LEFT_1);
    v2 = IVP_SEL2NX8I(vn1, vn0, IVP_SELI_8B_ROTATE_LEFT_1);
    v3 = vp1;
    v4 = vn1;
    v5 = IVP_SEL2NX8I(vp2, vp1, IVP_SELI_8B_ROTATE_RIGHT_1);
    v6 = IVP_SEL2NX8I(vc2, vc1, IVP_SELI_8B_ROTATE_RIGHT_1);
    v7 = IVP_SEL2NX8I(vn2, vn1, IVP_SELI_8B_ROTATE_RIGHT_1);

    acc_dx = IVP_ADDWU2NX8(v5, v7);
    IVP_MULUSQA2N8XR8(acc_dx, v6, v0, v1, v2, coef1);
    acc_dy = IVP_ADDWU2NX8(v2, v7);
    IVP_MULUSQA2N8XR8(acc_dy, v4, v0, v3, v5, coef1);

#if defined(FIX_SOBEL_ROUNDING1)
    outX = IVP_PACKVRNR2NX24(acc_dx, 3);
    outY = IVP_PACKVRNR2NX24(acc_dy, 3);
#elif defined(FIX_SOBEL_ROUNDING)
    outX = IVP_PACKVR2NX24(acc_dx, 1);
    outY = IVP_PACKVR2NX24(acc_dy, 1);
    outX = IVP_MAX2NX8(outX, -127);
    outY = IVP_MAX2NX8(outY, -127);
#else
    outX = IVP_PACKVRNR2NX24(acc_dx, 1);
    outY = IVP_PACKVRNR2NX24(acc_dy, 1);
    outX = IVP_MAX2NX8(outX, -127);
    outY = IVP_MAX2NX8(outY, -127);
#endif

    *ptr_Dx++ = outX;
    *ptr_Dy++ = outY;
    vp0 = vp1;
    vc0 = vc1;
    vn0 = vn1;
    vp1 = vp2;
    vc1 = vc2;
    vn1 = vn2;
  }

  for (j = 0; j < u32Width /* && totalCount < u32nCornersMax*/; j += 64) {
    i = 0;

    pdvecSx = (xb_vec2Nx8 *)(p_s8Dx + j + 1);
    pdvecSy = (xb_vec2Nx8 *)(p_s8Dy + j + 1);
    pdvecDst2N = (xb_vec2Nx8U *)(p_u8Out + j);

    vaX = IVP_LA_PP(pdvecSx);
    vaY = IVP_LA_PP(pdvecSy);
    dAccX = IVP_MUL2NX8(0, 0);
    dAccY = IVP_MUL2NX8(0, 0);
    dAccXY = IVP_MUL2NX8(0, 0);
    for (k = 0; k < 5; k++) {
      vaX = IVP_LA_PP(pdvecSx);
      vaY = IVP_LA_PP(pdvecSy);

      IVP_LA2NX8_IP(dvecSx0, vaX, pdvecSx);
      IVP_LA2NX8_XP(dvecSx1, vaX, pdvecSx, (s32SrcStride - 64));

      IVP_LA2NX8_IP(dvecSy0, vaY, pdvecSy);
      IVP_LA2NX8_XP(dvecSy1, vaY, pdvecSy, (s32SrcStride - 64));

      dvecS0 = IVP_SEL2NX8I(dvecSx1, dvecSx0, IVP_SELI_8B_ROTATE_RIGHT_1);
      dvecS1 = IVP_SEL2NX8I(dvecSx1, dvecSx0, IVP_SELI_8B_ROTATE_RIGHT_2);
      dvecS2 = IVP_SEL2NX8I(dvecSx1, dvecSx0, IVP_SELI_8B_ROTATE_RIGHT_3);
      dvecS3 = IVP_SEL2NX8I(dvecSx1, dvecSx0, IVP_SELI_8B_ROTATE_RIGHT_4);

      dvecT0 = IVP_SEL2NX8I(dvecSy1, dvecSy0, IVP_SELI_8B_ROTATE_RIGHT_1);
      dvecT1 = IVP_SEL2NX8I(dvecSy1, dvecSy0, IVP_SELI_8B_ROTATE_RIGHT_2);
      dvecT2 = IVP_SEL2NX8I(dvecSy1, dvecSy0, IVP_SELI_8B_ROTATE_RIGHT_3);
      dvecT3 = IVP_SEL2NX8I(dvecSy1, dvecSy0, IVP_SELI_8B_ROTATE_RIGHT_4);

      // Square X
      IVP_MULPA2NX8(dAccX, dvecSx0, dvecSx0, dvecS0, dvecS0);
      IVP_MULPA2NX8(dAccX, dvecS1, dvecS1, dvecS2, dvecS2);
      IVP_MULA2NX8(dAccX, dvecS3, dvecS3);

      // Square Y
      IVP_MULPA2NX8(dAccY, dvecSy0, dvecSy0, dvecT0, dvecT0);
      IVP_MULPA2NX8(dAccY, dvecT1, dvecT1, dvecT2, dvecT2);
      IVP_MULA2NX8(dAccY, dvecT3, dvecT3);

      // Multiply XY
      IVP_MULPA2NX8(dAccXY, dvecSx0, dvecSy0, dvecS0, dvecT0);
      IVP_MULPA2NX8(dAccXY, dvecS1, dvecT1, dvecS2, dvecT2);
      IVP_MULA2NX8(dAccXY, dvecS3, dvecT3);
    }
#if defined(FIX_SOBEL_ROUNDING1)
#define ROUND_FACTOR_ORDER2 6
#else
#define ROUND_FACTOR_ORDER2 10
#endif

    vecX0 = IVP_PACKVRNR2NX24_0(dAccX, ROUND_FACTOR_ORDER2);
    vecX1 = IVP_PACKVRNR2NX24_1(dAccX, ROUND_FACTOR_ORDER2);

    vecY0 = IVP_PACKVRNR2NX24_0(dAccY, ROUND_FACTOR_ORDER2);
    vecY1 = IVP_PACKVRNR2NX24_1(dAccY, ROUND_FACTOR_ORDER2);

    vecXY0 = IVP_PACKVRNR2NX24_0(dAccXY, ROUND_FACTOR_ORDER2);
    vecXY1 = IVP_PACKVRNR2NX24_1(dAccXY, ROUND_FACTOR_ORDER2);

    vecSum0 = vecX0 + vecY0;
    vecSum0 = IVP_SRANX16(vecSum0, u32Shift);
    accR0 = IVP_MULPNX16(vecX0, vecY0, vecXY0, IVP_NEGNX16(vecXY0));
    IVP_MULANX16(accR0, vecSum0, -1);
    vecR0 = IVP_PACKVRNRNX48(accR0, 8);

    vecSum1 = vecX1 + vecY1;
    vecSum1 = IVP_SRANX16(vecSum1, u32Shift);
    accR0 = IVP_MULPNX16(vecX1, vecY1, vecXY1, IVP_NEGNX16(vecXY1));
    IVP_MULANX16(accR0, vecSum1, -1);
    vecR1 = IVP_PACKVRNRNX48(accR0, 8);

    vecHarrisScore = IVP_PACKVRU2NX24(IVP_MULI2NR8X16(1, vecR1, vecR0), 0);
    IVP_SV2NX8U_XP(vecHarrisScore, pdvecDst2N, u32Width);

    pdvecSxTemp = (xb_vec2Nx8 *)(p_s8Dx + j + 1);
    pdvecSyTemp = (xb_vec2Nx8 *)(p_s8Dy + j + 1);
    vaXTemp = IVP_LA_PP(pdvecSxTemp);
    vaYTemp = IVP_LA_PP(pdvecSyTemp);

    for (i = 1; i < u32Height; i++) {
      // 6th row
      vaX = IVP_LA_PP(pdvecSx);
      vaY = IVP_LA_PP(pdvecSy);

      IVP_LA2NX8_IP(dvecSx0, vaX, pdvecSx);
      IVP_LA2NX8_XP(dvecSx1, vaX, pdvecSx, (s32SrcStride - 64));

      IVP_LA2NX8_IP(dvecSy0, vaY, pdvecSy);
      IVP_LA2NX8_XP(dvecSy1, vaY, pdvecSy, (s32SrcStride - 64));

      dvecS0 = IVP_SEL2NX8I(dvecSx1, dvecSx0, IVP_SELI_8B_ROTATE_RIGHT_1);
      dvecS1 = IVP_SEL2NX8I(dvecSx1, dvecSx0, IVP_SELI_8B_ROTATE_RIGHT_2);
      dvecS2 = IVP_SEL2NX8I(dvecSx1, dvecSx0, IVP_SELI_8B_ROTATE_RIGHT_3);
      dvecS3 = IVP_SEL2NX8I(dvecSx1, dvecSx0, IVP_SELI_8B_ROTATE_RIGHT_4);

      dvecT0 = IVP_SEL2NX8I(dvecSy1, dvecSy0, IVP_SELI_8B_ROTATE_RIGHT_1);
      dvecT1 = IVP_SEL2NX8I(dvecSy1, dvecSy0, IVP_SELI_8B_ROTATE_RIGHT_2);
      dvecT2 = IVP_SEL2NX8I(dvecSy1, dvecSy0, IVP_SELI_8B_ROTATE_RIGHT_3);
      dvecT3 = IVP_SEL2NX8I(dvecSy1, dvecSy0, IVP_SELI_8B_ROTATE_RIGHT_4);

      // Square X
      IVP_MULPA2NX8(dAccX, dvecSx0, dvecSx0, dvecS0, dvecS0);
      IVP_MULPA2NX8(dAccX, dvecS1, dvecS1, dvecS2, dvecS2);

      // Square Y
      IVP_MULPA2NX8(dAccY, dvecSy0, dvecSy0, dvecT0, dvecT0);
      IVP_MULPA2NX8(dAccY, dvecT1, dvecT1, dvecT2, dvecT2);

      // Multiply XY
      IVP_MULPA2NX8(dAccXY, dvecSx0, dvecSy0, dvecS0, dvecT0);
      IVP_MULPA2NX8(dAccXY, dvecS1, dvecT1, dvecS2, dvecT2);

      // 0th row
      vaXTemp = IVP_LA_PP(pdvecSxTemp);
      vaYTemp = IVP_LA_PP(pdvecSyTemp);

      IVP_LA2NX8_IP(dvecSx0, vaXTemp, pdvecSxTemp);
      IVP_LA2NX8_XP(dvecSx1, vaXTemp, pdvecSxTemp, (s32SrcStride - 64));

      IVP_LA2NX8_IP(dvecSy0, vaYTemp, pdvecSyTemp);
      IVP_LA2NX8_XP(dvecSy1, vaYTemp, pdvecSyTemp, (s32SrcStride - 64));

      dvecS0 = IVP_SEL2NX8I(dvecSx1, dvecSx0, IVP_SELI_8B_ROTATE_RIGHT_1);
      dvecS1 = IVP_SEL2NX8I(dvecSx1, dvecSx0, IVP_SELI_8B_ROTATE_RIGHT_2);
      dvecS2 = IVP_SEL2NX8I(dvecSx1, dvecSx0, IVP_SELI_8B_ROTATE_RIGHT_3);
      dvecS3P = IVP_SEL2NX8I(dvecSx1, dvecSx0, IVP_SELI_8B_ROTATE_RIGHT_4);

      dvecT0 = IVP_SEL2NX8I(dvecSy1, dvecSy0, IVP_SELI_8B_ROTATE_RIGHT_1);
      dvecT1 = IVP_SEL2NX8I(dvecSy1, dvecSy0, IVP_SELI_8B_ROTATE_RIGHT_2);
      dvecT2 = IVP_SEL2NX8I(dvecSy1, dvecSy0, IVP_SELI_8B_ROTATE_RIGHT_3);
      dvecT3P = IVP_SEL2NX8I(dvecSy1, dvecSy0, IVP_SELI_8B_ROTATE_RIGHT_4);

      // Square X
      IVP_MULPA2NX8(dAccX, dvecSx0, IVP_NEG2NX8(dvecSx0), dvecS0,
                    IVP_NEG2NX8(dvecS0));
      IVP_MULPA2NX8(dAccX, dvecS1, IVP_NEG2NX8(dvecS1), dvecS2,
                    IVP_NEG2NX8(dvecS2));
      IVP_MULPA2NX8(dAccX, dvecS3, dvecS3, dvecS3P, IVP_NEG2NX8(dvecS3P));

      // Square Y
      IVP_MULPA2NX8(dAccY, dvecSy0, IVP_NEG2NX8(dvecSy0), dvecT0,
                    IVP_NEG2NX8(dvecT0));
      IVP_MULPA2NX8(dAccY, dvecT1, IVP_NEG2NX8(dvecT1), dvecT2,
                    IVP_NEG2NX8(dvecT2));
      IVP_MULPA2NX8(dAccY, dvecT3, dvecT3, dvecT3P, IVP_NEG2NX8(dvecT3P));

      // Multiply XY
      IVP_MULPA2NX8(dAccXY, dvecSx0, IVP_NEG2NX8(dvecSy0), dvecS0,
                    IVP_NEG2NX8(dvecT0));
      IVP_MULPA2NX8(dAccXY, dvecS1, IVP_NEG2NX8(dvecT1), dvecS2,
                    IVP_NEG2NX8(dvecT2));
      IVP_MULPA2NX8(dAccXY, dvecS3, dvecT3, dvecS3P, IVP_NEG2NX8(dvecT3P));
      // Pack Output values
      vecX0 = IVP_PACKVRNR2NX24_0(dAccX, ROUND_FACTOR_ORDER2);
      vecX1 = IVP_PACKVRNR2NX24_1(dAccX, ROUND_FACTOR_ORDER2);

      vecY0 = IVP_PACKVRNR2NX24_0(dAccY, ROUND_FACTOR_ORDER2);
      vecY1 = IVP_PACKVRNR2NX24_1(dAccY, ROUND_FACTOR_ORDER2);

      vecXY0 = IVP_PACKVRNR2NX24_0(dAccXY, ROUND_FACTOR_ORDER2);
      vecXY1 = IVP_PACKVRNR2NX24_1(dAccXY, ROUND_FACTOR_ORDER2);

      vecSum0 = vecX0 + vecY0;
      vecSum0 = IVP_SRANX16(vecSum0, u32Shift);
      accR0 = IVP_MULPNX16(vecX0, vecY0, vecXY0, IVP_NEGNX16(vecXY0));
      IVP_MULANX16(accR0, vecSum0, -1);
      vecR0 = IVP_PACKVRNRNX48(accR0, 8);

      vecSum1 = vecX1 + vecY1;
      vecSum1 = IVP_SRANX16(vecSum1, u32Shift);
      accR0 = IVP_MULPNX16(vecX1, vecY1, vecXY1, IVP_NEGNX16(vecXY1));
      IVP_MULANX16(accR0, vecSum1, -1);
      vecR1 = IVP_PACKVRNRNX48(accR0, 8);

      vecHarrisScore = IVP_PACKVRU2NX24(IVP_MULI2NR8X16(1, vecR1, vecR0), 0);
      IVP_SV2NX8U_XP(vecHarrisScore, pdvecDst2N, u32Width);
    }
  }
}
