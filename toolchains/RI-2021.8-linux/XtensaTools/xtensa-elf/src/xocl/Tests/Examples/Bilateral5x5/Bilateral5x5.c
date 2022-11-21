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

#define IVP_SIMD_WIDTH 32
#define ACC_SHIFT      8

#define IFILT_DSEL(v0, v1, vx, LUTvec, outvec0, outvec1)                       \
  {                                                                            \
    xb_vec2Nx8U dvecDiff0, dvecDiff1;                                          \
    dvecDiff0 = IVP_ABSSUBU2NX8(v0, vx);                                       \
    dvecDiff1 = IVP_ABSSUBU2NX8(v1, vx);                                       \
    dvecDiff0 = dvecDiff0 >> 2;                                                \
    dvecDiff1 = dvecDiff1 >> 2;                                                \
    outvec0 = IVP_SHFL2NX8(LUTvec, dvecDiff0);                                 \
    outvec1 = IVP_SHFL2NX8(LUTvec, dvecDiff1);                                 \
  }
#define IFILT_SEL(v0, vx, LUTvec, outVec)                                      \
  {                                                                            \
    xb_vec2Nx8U dvecDiff;                                                      \
    dvecDiff = IVP_ABSSUBU2NX8(v0, vx);                                        \
    dvecDiff = dvecDiff >> 2;                                                  \
    outVec = IVP_SHFL2NX8(LUTvec, dvecDiff);                                   \
  }

void xvFilter5x5Bilateral(xi_pTile src, xi_pTile dst, void *tempDiv,
                          const uint8_t *coeffBuffer) {

  uint8_t *filtIn;
  uint8_t *filtOut;
  int32_t width;
  int32_t widthExt;
  int32_t height;
  int32_t edge_width;

  filtIn = XI_TILE_GET_BUFF_PTR(src);
  filtOut = XI_TILE_GET_DATA_PTR(dst);
  width = XI_TILE_GET_WIDTH(src) >> 1;
  widthExt = XI_TILE_GET_PITCH(src) >> 1;
  height = XI_TILE_GET_HEIGHT(src);
  edge_width = XI_TILE_GET_EDGE_WIDTH(src);

  int32_t ix, iy, widthExp, xIterations;
  xb_vec2Nx8U dvecZero = 0;
  /*************************************************************************
   * Center pixel
   *************************************************************************/
  xb_vec2Nx8U dvecPx;
  xb_vec2Nx8U *__restrict pdvecPx;
  uint32_t uPtrRefPix;
  /*************************************************************************
   * Input pixels to select op to obtain pixels to the left/right &
   * above/below center
   *************************************************************************/
  uint32_t uPtrFiltIn;
  /*************************************************************************
   * Output of select operation that obtains pixels to the left/right &
   * above/below center
   *************************************************************************/
  xb_vec2Nx8U dvecPp0, dvecPp1, dvecPp2, dvecPp3, dvecPp4, dvecPp5, dvecPp1_tmp;
  xb_vec2Nx8U *__restrict pdvecPp;
  /*************************************************************************
   * Accumulators that store partial filter and coefficient weights
   *************************************************************************/
  xb_vec2Nx24 daccFilt, daccCoeffSum;

  /*************************************************************************
   *  Vectors for storing filter results
   *************************************************************************/
  xb_vecNx16U vecResultEven, vecResultOdd;
  /*********************s****************************************************
   *  Vectors for storing weight results
   *************************************************************************/
  xb_vecNx16U vecWtEvn, vecWtOdd;
  /*************************************************************************
   * Product of Gaussian range and spatial coefficient for the current pixel
   * difference
   *************************************************************************/
  xb_vec2Nx8U dvecFiltCoeff0, dvecFiltCoeff1, dvecFiltCoeff2, dvecFiltCoeff3;
  /*************************************************************************
   * Vectors used during division operations (partial remainders & quotients)
   *************************************************************************/
  xb_vecNx16 vecPartRem, vecPartRem1;
  xb_vecNx16 vecQuotientEven, vecQuotientOdd, vecQuotient1;
  /*************************************************************************
   * Final filter output
   *************************************************************************/
  xb_vecNx8U *__restrict pvecResult;
  xb_vecNx16 vecResultL; // 1st 32 bytes of output
  xb_vecNx16 vecResultH; // 2nd 32 bytes of output
  uint32_t uPtrFiltOut;

  xb_vecNx16U *__restrict pvecTemp = (xb_vecNx16U * __restrict) tempDiv;

  widthExp = width << 1;
  widthExt = (widthExt << 1);
  uPtrFiltIn = (uint32_t)(filtIn + (edge_width - 2));
  uPtrRefPix = (uint32_t)(filtIn) + (widthExt * 2) + edge_width;
  uPtrFiltOut = (uint32_t)filtOut;
  xIterations = (widthExp / (2 * IVP_SIMD_WIDTH));
  /*************************************************************************
   * Precomputed product of range & spatial coefficients that will be used
   * during lookup operations
   *************************************************************************/
  xb_vec2Nx8U *__restrict pvecCoeff; // Used to load coefficient for pre-compute
  pvecCoeff = (xb_vec2Nx8U *)coeffBuffer;
  xb_vecNx16 vecPattern, vecSeq, vecSeq1;
  vecSeq = IVP_SEQNX16U();
  vecSeq1 = vecSeq >> 1;
  xb_vecN_2x32Uv vec1 = 16 << 16;
  vecPattern = vecSeq1 + IVP_MOVNX16_FROMN_2X32U(vec1);
  // xb_vec2Nx8U dvecC0, dvecC1, dvecC2, dvecC3, dvecC4, dvecC5, dvecC6, dvecC7;
  xb_vec2Nx8U dvecC0, dvecC1, dvecC2, dvecC4, dvecC5;

  /*  C0	C1	C2	C1	C0
C3	C4	C5	C4	C3
C6	C7	C8	C7	C6
C3	C4	C5	C4	C3
C0	C1	C2	C1	C0
   */
  //	dvecC0 = *pvecCoeff++;
  //	dvecC1 = *pvecCoeff++;
  //	dvecC2 = *pvecCoeff++;
  //	dvecC3 = *pvecCoeff++;
  //	dvecC4 = *pvecCoeff++;
  //	dvecC5 = *pvecCoeff++;
  //	dvecC6 = *pvecCoeff++;
  //	dvecC7 = *pvecCoeff++;

  dvecC0 = pvecCoeff[0];
  dvecC1 = pvecCoeff[1];
  dvecC2 = pvecCoeff[2];
  dvecC4 = pvecCoeff[4];
  dvecC5 = pvecCoeff[5];

  int8_t coeff8 = *(int8_t *)(&pvecCoeff[8]);
  for (ix = 0; ix < xIterations; ix++) {
    // Get the current center
    pdvecPx = (xb_vec2Nx8U *)uPtrRefPix;
    pdvecPx += ix;
    // Set the peripheral pointers
    pdvecPp = (xb_vec2Nx8U *)uPtrFiltIn;
    pdvecPp += ix;
    /*		pdacc = (xb_vec2Nx24 * __restrict)tempDiv ;*/
    pvecTemp = (xb_vecNx16U * __restrict) tempDiv;

    int sstep =
        widthExt - 2 * IVP_SIMD_WIDTH -
        XT_MIN(2 * IVP_SIMD_WIDTH,
               widthExp - (ix * 2 * IVP_SIMD_WIDTH) - (2 * IVP_SIMD_WIDTH) + 4);
    int sstep2 =
        -widthExt - 2 * IVP_SIMD_WIDTH -
        XT_MIN(2 * IVP_SIMD_WIDTH,
               widthExp - (ix * 2 * IVP_SIMD_WIDTH) - (2 * IVP_SIMD_WIDTH) + 4);
    int sstep3 =
        -2 * IVP_SIMD_WIDTH -
        XT_MIN(2 * IVP_SIMD_WIDTH,
               widthExp - (ix * 2 * IVP_SIMD_WIDTH) - (2 * IVP_SIMD_WIDTH) + 4);
    valign va1;
    for (iy = 0; iy < height; iy++) {
      // Load the center & increment to next line - IVP_LV2NX8_XP +widthExt
      va1 = IVP_LA2NX8U_PP(pdvecPx);
      IVP_LA2NX8U_XP(dvecPx, va1, pdvecPx, widthExt);
      // IVP_LV2NX8_XP(dvecPx, pdvecPx /*inout*/, widthExt);
      // Set the peripheral pointer to top-left of 5x5 region around current
      // center Process 5 rows

      /* ---------------------------------------------------*/
      /* --------------FIRST ROW ---------------------------*/
      /* ---------------------------------------------------*/
      // Load the peripheral 0 & process- increment to next pixel +1
#ifdef IVP_SEL2NX8I_S0

      valign va = IVP_LA2NX8U_PP(pdvecPp);
      IVP_LA2NX8U_IP(dvecPp0, va, pdvecPp);
      IVP_LAV2NX8U_XP(dvecPp1_tmp, va, pdvecPp,
                      widthExp - (ix * 2 * IVP_SIMD_WIDTH) + 4 -
                          (2 * IVP_SIMD_WIDTH));
      // IVP_LA2NX8U_XP(dvecPp1_tmp, va,pdvecPp,-64+3);
      dvecPp1 = IVP_SEL2NX8I(dvecPp1_tmp, dvecPp0, IVP_SELI_8B_ROTATE_RIGHT_1);
      dvecPp2 = IVP_SEL2NX8I(dvecPp1_tmp, dvecPp0, IVP_SELI_8B_ROTATE_RIGHT_2);
      dvecPp3 = IVP_SEL2NX8I(dvecPp1_tmp, dvecPp0, IVP_SELI_8B_ROTATE_RIGHT_3);
      // IVP_L2U2NX8_XP(dvecPp3 /*out*/, pdvecPp /*inout*/,
      // (IVP_SIMD_WIDTH*2*3)+64-3);//use dual loads for
      // IVP_SELI_8B_ROTATE_RIGHT_3
      dvecPp4 = IVP_SEL2NX8I(dvecPp1_tmp, dvecPp0, IVP_SELI_8B_ROTATE_RIGHT_4);
      pdvecPp = OFFSET_PTR_2NX8U(pdvecPp, 0, 0, sstep);
#else
      // printf("coming2\n");
      IVP_L2U2NX8_XP(dvecPp0 /*out*/, pdvecPp /*inout*/, 1);
      IVP_L2U2NX8_XP(dvecPp1 /*out*/, pdvecPp /*inout*/, 1);
      IVP_L2U2NX8_XP(dvecPp2 /*out*/, pdvecPp /*inout*/, 1);
      IVP_L2U2NX8_XP(dvecPp3 /*out*/, pdvecPp /*inout*/, 1);
      IVP_L2U2NX8_XP(dvecPp4 /*out*/, pdvecPp /*inout*/, widthExt - 4);
#endif
      IFILT_DSEL(dvecPp0, dvecPp4, dvecPx, dvecC0, dvecFiltCoeff0,
                 dvecFiltCoeff1);
#ifdef ENABLE_MULP_P6

      daccFilt =
          IVP_MULUUP2NX8(dvecPp0, dvecPp4, dvecFiltCoeff0, dvecFiltCoeff1);
#else

      daccFilt = IVP_MULUU2NX8(dvecPp0, dvecFiltCoeff0);
      IVP_MULUUA2NX8(daccFilt, dvecPp4, dvecFiltCoeff1);
#endif
      daccCoeffSum = IVP_ADDWU2NX8(dvecFiltCoeff0, dvecFiltCoeff1);

      IFILT_DSEL(dvecPp1, dvecPp3, dvecPx, dvecC1, dvecFiltCoeff0,
                 dvecFiltCoeff1);
#ifdef ENABLE_MULP_P6
      IVP_MULUUPA2NX8(daccFilt, dvecPp1, dvecPp3, dvecFiltCoeff0,
                      dvecFiltCoeff1);
#else
      IVP_MULUUA2NX8(daccFilt, dvecPp1, dvecFiltCoeff0);
      IVP_MULUUA2NX8(daccFilt, dvecPp3, dvecFiltCoeff1);
#endif
      IVP_ADDWUA2NX8(daccCoeffSum, dvecFiltCoeff0, dvecFiltCoeff1);

      IFILT_SEL(dvecPp2, dvecPx, dvecC2, dvecFiltCoeff2);

      /* ---------------------------------------------------*/
      /* --------------SECOND ROW ---------------------------*/
      /* ---------------------------------------------------*/
      // Load the peripheral 0 & process- increment to next pixel +1
#ifdef IVP_SEL2NX8I_S0
      va = IVP_LA2NX8U_PP(pdvecPp);
      IVP_LA2NX8U_IP(dvecPp0, va, pdvecPp);
      IVP_LAV2NX8U_XP(dvecPp1_tmp, va, pdvecPp,
                      widthExp - (ix * 2 * IVP_SIMD_WIDTH) + 4 -
                          (2 * IVP_SIMD_WIDTH));
      // IVP_LA2NX8U_XP(dvecPp1_tmp, va,pdvecPp,-64+3);
      dvecPp1 = IVP_SEL2NX8I(dvecPp1_tmp, dvecPp0, IVP_SELI_8B_ROTATE_RIGHT_1);
      dvecPp5 = IVP_SEL2NX8I(dvecPp1_tmp, dvecPp0, IVP_SELI_8B_ROTATE_RIGHT_2);
      dvecPp3 = IVP_SEL2NX8I(dvecPp1_tmp, dvecPp0, IVP_SELI_8B_ROTATE_RIGHT_3);
      // IVP_L2U2NX8_XP(dvecPp3 /*out*/, pdvecPp /*inout*/,
      // (IVP_SIMD_WIDTH*2*3)+64-3); //use dual loads for
      // IVP_SELI_8B_ROTATE_RIGHT_3
      dvecPp4 = IVP_SEL2NX8I(dvecPp1_tmp, dvecPp0, IVP_SELI_8B_ROTATE_RIGHT_4);
      pdvecPp = OFFSET_PTR_2NX8U(pdvecPp, 0, 0, sstep);
#else
      IVP_L2U2NX8_XP(dvecPp0 /*out*/, pdvecPp /*inout*/, 1);
      IVP_L2U2NX8_XP(dvecPp1 /*out*/, pdvecPp /*inout*/, 1);
      IVP_L2U2NX8_XP(dvecPp5 /*out*/, pdvecPp /*inout*/, 1);
      IVP_L2U2NX8_XP(dvecPp3 /*out*/, pdvecPp /*inout*/, 1);
      IVP_L2U2NX8_XP(dvecPp4 /*out*/, pdvecPp /*inout*/, widthExt - 4);
#endif
      // IFILT_DSEL(dvecPp0, dvecPp4, dvecPx, dvecC3, dvecFiltCoeff0,
      // dvecFiltCoeff1);
      IFILT_DSEL(dvecPp0, dvecPp4, dvecPx, dvecC1, dvecFiltCoeff0,
                 dvecFiltCoeff1);
#ifdef ENABLE_MULP_P6
      IVP_MULUUPA2NX8(daccFilt, dvecPp0, dvecPp4, dvecFiltCoeff0,
                      dvecFiltCoeff1);
#else
      IVP_MULUUA2NX8(daccFilt, dvecPp0, dvecFiltCoeff0);
      IVP_MULUUA2NX8(daccFilt, dvecPp4, dvecFiltCoeff1);
#endif
      IVP_ADDWUA2NX8(daccCoeffSum, dvecFiltCoeff0, dvecFiltCoeff1);

      IFILT_DSEL(dvecPp1, dvecPp3, dvecPx, dvecC4, dvecFiltCoeff0,
                 dvecFiltCoeff1);
#ifdef ENABLE_MULP_P6
      IVP_MULUUPA2NX8(daccFilt, dvecPp1, dvecPp3, dvecFiltCoeff0,
                      dvecFiltCoeff1);
#else
      IVP_MULUUA2NX8(daccFilt, dvecPp1, dvecFiltCoeff0);
      IVP_MULUUA2NX8(daccFilt, dvecPp3, dvecFiltCoeff1);
#endif

      IVP_ADDWUA2NX8(daccCoeffSum, dvecFiltCoeff0, dvecFiltCoeff1);

      IFILT_SEL(dvecPp5, dvecPx, dvecC5, dvecFiltCoeff3);
#ifdef ENABLE_MULP_P6
      IVP_MULUUPA2NX8(daccFilt, dvecPp2, dvecPp5, dvecFiltCoeff2,
                      dvecFiltCoeff3);
#else
      IVP_MULUUA2NX8(daccFilt, dvecPp2, dvecFiltCoeff2);
      IVP_MULUUA2NX8(daccFilt, dvecPp5, dvecFiltCoeff3);
#endif

      IVP_ADDWUA2NX8(daccCoeffSum, dvecFiltCoeff2, dvecFiltCoeff3);

      /* ---------------------------------------------------*/
      /* --------------THIRD ROW ---------------------------*/
      /* ---------------------------------------------------*/
#ifdef IVP_SEL2NX8I_S0
      va = IVP_LA2NX8U_PP(pdvecPp);
      IVP_LA2NX8U_IP(dvecPp0, va, pdvecPp);
      IVP_LAV2NX8U_XP(dvecPp1_tmp, va, pdvecPp,
                      widthExp - (ix * 2 * IVP_SIMD_WIDTH) + 4 -
                          (2 * IVP_SIMD_WIDTH));
      // IVP_LA2NX8U_XP(dvecPp1_tmp, va,pdvecPp,-64+3);
      dvecPp1 = IVP_SEL2NX8I(dvecPp1_tmp, dvecPp0, IVP_SELI_8B_ROTATE_RIGHT_1);
      dvecPp3 = IVP_SEL2NX8I(dvecPp1_tmp, dvecPp0, IVP_SELI_8B_ROTATE_RIGHT_3);
      // IVP_L2U2NX8_XP(dvecPp3 /*out*/, pdvecPp /*inout*/,
      // ((-IVP_SIMD_WIDTH*2)-widthExt)+64-3); //use dual loads for
      // IVP_SELI_8B_ROTATE_RIGHT_3
      dvecPp4 = IVP_SEL2NX8I(dvecPp1_tmp, dvecPp0, IVP_SELI_8B_ROTATE_RIGHT_4);
      pdvecPp = OFFSET_PTR_2NX8U(pdvecPp, 0, 0, sstep2);
#else
      IVP_L2U2NX8_XP(dvecPp0 /*out*/, pdvecPp /*inout*/, 1);
      IVP_L2U2NX8_XP(dvecPp1 /*out*/, pdvecPp /*inout*/, 2);
      // IVP_L2U2NX8_XP(dvecPp2 /*out*/, pdvecPp /*inout*/, 1); // centre pixel
      IVP_L2U2NX8_XP(dvecPp3 /*out*/, pdvecPp /*inout*/, 1);
      IVP_L2U2NX8_XP(dvecPp4 /*out*/, pdvecPp /*inout*/, -4 - widthExt);
#endif
      // IFILT_DSEL(dvecPp0, dvecPp4, dvecPx, dvecC6, dvecFiltCoeff0,
      // dvecFiltCoeff1);
      IFILT_DSEL(dvecPp0, dvecPp4, dvecPx, dvecC2, dvecFiltCoeff0,
                 dvecFiltCoeff1);
#ifdef ENABLE_MULP_P6
      IVP_MULUUPA2NX8(daccFilt, dvecPp0, dvecPp4, dvecFiltCoeff0,
                      dvecFiltCoeff1);
#else
      IVP_MULUUA2NX8(daccFilt, dvecPp0, dvecFiltCoeff0);
      IVP_MULUUA2NX8(daccFilt, dvecPp4, dvecFiltCoeff1);
#endif

      IVP_ADDWUA2NX8(daccCoeffSum, dvecFiltCoeff0, dvecFiltCoeff1);

      // IFILT_DSEL(dvecPp1, dvecPp3, dvecPx, dvecC7, dvecFiltCoeff0,
      // dvecFiltCoeff1);
      IFILT_DSEL(dvecPp1, dvecPp3, dvecPx, dvecC5, dvecFiltCoeff0,
                 dvecFiltCoeff1);
#ifdef ENABLE_MULP_P6
      IVP_MULUUPA2NX8(daccFilt, dvecPp1, dvecPp3, dvecFiltCoeff0,
                      dvecFiltCoeff1);
#else
      IVP_MULUUA2NX8(daccFilt, dvecPp1, dvecFiltCoeff0);
      IVP_MULUUA2NX8(daccFilt, dvecPp3, dvecFiltCoeff1);
#endif

      IVP_ADDWUA2NX8(daccCoeffSum, dvecFiltCoeff0, dvecFiltCoeff1);

      // IFILT_SEL(dvecPp2,dvecPx, dvecC8, dvecFiltCoeff2);

      IVP_MULUUA2NX8(daccFilt, dvecPx,
                     coeff8); // use scalar load to save 1 vec or extract
      IVP_ADDWUA2NX8(daccCoeffSum, coeff8, dvecZero);

      xb_vecNx16U vecCoeffSumL = IVP_CVT16U2NX24L(daccCoeffSum);
      xb_vecNx16U vecCoeffSumH = IVP_CVT16U2NX24H(daccCoeffSum);
      IVP_SVNX16U_IP(vecCoeffSumL, pvecTemp, 64);
      IVP_SVNX16U_IP(vecCoeffSumH, pvecTemp, 64);

      xb_vecN_2x32v vecResultLL, vecResultLH, vecResultHL, vecResultHH;
      vecResultLL = IVP_CVT32S2NX24LL(daccFilt);
      vecResultLH = IVP_CVT32S2NX24LH(daccFilt);
      vecResultHL = IVP_CVT32S2NX24HL(daccFilt);
      vecResultHH = IVP_CVT32S2NX24HH(daccFilt);
      IVP_SVN_2X32_IP(vecResultLL, (xb_vecN_2x32v *)pvecTemp, 64);
      IVP_SVN_2X32_IP(vecResultLH, (xb_vecN_2x32v *)pvecTemp, 64);
      IVP_SVN_2X32_IP(vecResultHL, (xb_vecN_2x32v *)pvecTemp, 64);
      IVP_SVN_2X32_IP(vecResultHH, (xb_vecN_2x32v *)pvecTemp, 64);
    }

#pragma no_reorder
    // Get the current center
    pdvecPx = (xb_vec2Nx8U *)uPtrRefPix;
    pdvecPx += ix;
    pvecResult = (xb_vecNx8U *)uPtrFiltOut;
    pvecResult += ix * 2;
    pdvecPp = (xb_vec2Nx8U *)((int)uPtrFiltIn + (int)(3 * widthExt));
    pdvecPp += ix;
    /*		pdacc = (xb_vec2Nx24 * __restrict)tempDiv ;*/
    pvecTemp = (xb_vecNx16U * __restrict) tempDiv;

    for (iy = 0; iy < height; iy++) {

      // Load the center & increment to next line - IVP_LV2NX8_XP +widthExt
      va1 = IVP_LA2NX8U_PP(pdvecPx);
      IVP_LA2NX8U_XP(dvecPx, va1, pdvecPx, widthExt);
      // IVP_LV2NX8_XP(dvecPx, pdvecPx /*inout*/, widthExt);
      // Set the peripheral pointer to top-left of 5x5 region around current
      // center Process 5 rows
      /* ---------------------------------------------------*/
      /* --------------FOURTH ROW ---------------------------*/
      /* ---------------------------------------------------*/
#ifdef IVP_SEL2NX8I_S0
      valign va = IVP_LA2NX8U_PP(pdvecPp);
      IVP_LA2NX8U_IP(dvecPp0, va, pdvecPp);
      IVP_LAV2NX8U_XP(dvecPp1_tmp, va, pdvecPp,
                      widthExp - (ix * 2 * IVP_SIMD_WIDTH) + 4 -
                          (2 * IVP_SIMD_WIDTH));
      // IVP_LA2NX8U_XP(dvecPp1_tmp, va,pdvecPp,-64+3);
      dvecPp1 = IVP_SEL2NX8I(dvecPp1_tmp, dvecPp0, IVP_SELI_8B_ROTATE_RIGHT_1);
      dvecPp2 = IVP_SEL2NX8I(dvecPp1_tmp, dvecPp0, IVP_SELI_8B_ROTATE_RIGHT_2);
      dvecPp3 = IVP_SEL2NX8I(dvecPp1_tmp, dvecPp0, IVP_SELI_8B_ROTATE_RIGHT_3);
      // IVP_L2U2NX8_XP(dvecPp3 /*out*/, pdvecPp /*inout*/,(IVP_SIMD_WIDTH*2*3)
      // +64-3); //use dual loads for IVP_SELI_8B_ROTATE_RIGHT_3
      dvecPp4 = IVP_SEL2NX8I(dvecPp1_tmp, dvecPp0, IVP_SELI_8B_ROTATE_RIGHT_4);
      pdvecPp = OFFSET_PTR_2NX8U(pdvecPp, 0, 0, sstep);
#else
      IVP_L2U2NX8_XP(dvecPp0 /*out*/, pdvecPp /*inout*/, 1);
      IVP_L2U2NX8_XP(dvecPp1 /*out*/, pdvecPp /*inout*/, 1);
      IVP_L2U2NX8_XP(dvecPp2 /*out*/, pdvecPp /*inout*/, 1);
      IVP_L2U2NX8_XP(dvecPp3 /*out*/, pdvecPp /*inout*/, 1);
      IVP_L2U2NX8_XP(dvecPp4 /*out*/, pdvecPp /*inout*/, widthExt - 4);
#endif
      // IFILT_DSEL(dvecPp0, dvecPp4, dvecPx, dvecC3, dvecFiltCoeff0,
      // dvecFiltCoeff1);
      IFILT_DSEL(dvecPp0, dvecPp4, dvecPx, dvecC1, dvecFiltCoeff0,
                 dvecFiltCoeff1);
#ifdef ENABLE_MULP_P6
      daccFilt =
          IVP_MULUUP2NX8(dvecPp0, dvecPp4, dvecFiltCoeff0, dvecFiltCoeff1);
#else
      daccFilt = IVP_MULUU2NX8(dvecPp0, dvecFiltCoeff0);
      IVP_MULUUA2NX8(daccFilt, dvecPp4, dvecFiltCoeff1);
#endif

      daccCoeffSum = IVP_ADDWU2NX8(dvecFiltCoeff0, dvecFiltCoeff1);

      IFILT_DSEL(dvecPp1, dvecPp3, dvecPx, dvecC4, dvecFiltCoeff0,
                 dvecFiltCoeff1);
#ifdef ENABLE_MULP_P6
      IVP_MULUUPA2NX8(daccFilt, dvecPp1, dvecPp3, dvecFiltCoeff0,
                      dvecFiltCoeff1);
#else
      IVP_MULUUA2NX8(daccFilt, dvecPp1, dvecFiltCoeff0);
      IVP_MULUUA2NX8(daccFilt, dvecPp3, dvecFiltCoeff1);
#endif

      IVP_ADDWUA2NX8(daccCoeffSum, dvecFiltCoeff0, dvecFiltCoeff1);

      IFILT_SEL(dvecPp2, dvecPx, dvecC5, dvecFiltCoeff2);

      /* ---------------------------------------------------*/
      /* --------------FIFTH ROW ---------------------------*/
      /* ---------------------------------------------------*/
      // Load the peripheral 0 & process- increment to next pixel +1
#ifdef IVP_SEL2NX8I_S0
      va = IVP_LA2NX8U_PP(pdvecPp);
      IVP_LA2NX8U_IP(dvecPp0, va, pdvecPp);
      IVP_LAV2NX8U_XP(dvecPp1_tmp, va, pdvecPp,
                      widthExp - (ix * 2 * IVP_SIMD_WIDTH) + 4 -
                          (2 * IVP_SIMD_WIDTH));
      // IVP_LA2NX8U_XP(dvecPp1_tmp, va,pdvecPp,(-IVP_SIMD_WIDTH*2) + 3);
      dvecPp1 = IVP_SEL2NX8I(dvecPp1_tmp, dvecPp0, IVP_SELI_8B_ROTATE_RIGHT_1);
      dvecPp5 = IVP_SEL2NX8I(dvecPp1_tmp, dvecPp0, IVP_SELI_8B_ROTATE_RIGHT_2);
      dvecPp3 = IVP_SEL2NX8I(dvecPp1_tmp, dvecPp0, IVP_SELI_8B_ROTATE_RIGHT_3);
      // IVP_L2U2NX8_XP(dvecPp3 /*out*/, pdvecPp /*inout*/, -3); //use dual
      // loads for IVP_SELI_8B_ROTATE_RIGHT_3
      dvecPp4 = IVP_SEL2NX8I(dvecPp1_tmp, dvecPp0, IVP_SELI_8B_ROTATE_RIGHT_4);
      pdvecPp = OFFSET_PTR_2NX8U(pdvecPp, 0, 0, sstep3);
#else
      IVP_L2U2NX8_XP(dvecPp0 /*out*/, pdvecPp /*inout*/, 1);
      IVP_L2U2NX8_XP(dvecPp1 /*out*/, pdvecPp /*inout*/, 1);
      IVP_L2U2NX8_XP(dvecPp5 /*out*/, pdvecPp /*inout*/, 1);
      IVP_L2U2NX8_XP(dvecPp3 /*out*/, pdvecPp /*inout*/, 1);
      IVP_L2U2NX8_XP(dvecPp4 /*out*/, pdvecPp /*inout*/, -4);
#endif

      IFILT_DSEL(dvecPp0, dvecPp4, dvecPx, dvecC0, dvecFiltCoeff0,
                 dvecFiltCoeff1);
#ifdef ENABLE_MULP_P6
      IVP_MULUUPA2NX8(daccFilt, dvecPp0, dvecPp4, dvecFiltCoeff0,
                      dvecFiltCoeff1);
#else
      IVP_MULUUA2NX8(daccFilt, dvecPp0, dvecFiltCoeff0);
      IVP_MULUUA2NX8(daccFilt, dvecPp4, dvecFiltCoeff1);
#endif

      IVP_ADDWUA2NX8(daccCoeffSum, dvecFiltCoeff0, dvecFiltCoeff1);

      IFILT_DSEL(dvecPp1, dvecPp3, dvecPx, dvecC1, dvecFiltCoeff0,
                 dvecFiltCoeff1);
#ifdef ENABLE_MULP_P6
      IVP_MULUUPA2NX8(daccFilt, dvecPp1, dvecPp3, dvecFiltCoeff0,
                      dvecFiltCoeff1);
#else
      IVP_MULUUA2NX8(daccFilt, dvecPp1, dvecFiltCoeff0);
      IVP_MULUUA2NX8(daccFilt, dvecPp3, dvecFiltCoeff1);
#endif

      IVP_ADDWUA2NX8(daccCoeffSum, dvecFiltCoeff0, dvecFiltCoeff1);

      IFILT_SEL(dvecPp5, dvecPx, dvecC2, dvecFiltCoeff3);
#ifdef ENABLE_MULP_P6
      IVP_MULUUPA2NX8(daccFilt, dvecPp2, dvecPp5, dvecFiltCoeff2,
                      dvecFiltCoeff3);
#else
      IVP_MULUUA2NX8(daccFilt, dvecPp2, dvecFiltCoeff2);
      IVP_MULUUA2NX8(daccFilt, dvecPp5, dvecFiltCoeff3);
#endif
      IVP_ADDWUA2NX8(daccCoeffSum, dvecFiltCoeff2, dvecFiltCoeff3);

      xb_vecNx16 vecCoeffSumL;
      IVP_LVNX16U_IP(vecCoeffSumL, pvecTemp, 64);
      xb_vecNx16 vecCoeffSumH;
      IVP_LVNX16U_IP(vecCoeffSumH, pvecTemp, 64);
      vecCoeffSumL += IVP_CVT16U2NX24L(daccCoeffSum);
      vecCoeffSumH += IVP_CVT16U2NX24H(daccCoeffSum);

      xb_vecN_2x32v vecResultLL, vecResultLH, vecResultHL, vecResultHH;
      IVP_LVN_2X32_IP(vecResultLL, (xb_vecN_2x32v *)pvecTemp, 64);
      IVP_LVN_2X32_IP(vecResultLH, (xb_vecN_2x32v *)pvecTemp, 64);
      IVP_LVN_2X32_IP(vecResultHL, (xb_vecN_2x32v *)pvecTemp, 64);
      IVP_LVN_2X32_IP(vecResultHH, (xb_vecN_2x32v *)pvecTemp, 64);
      vecResultLL += IVP_CVT32S2NX24LL(daccFilt);
      vecResultLH += IVP_CVT32S2NX24LH(daccFilt);
      vecResultHL += IVP_CVT32S2NX24HL(daccFilt);
      vecResultHH += IVP_CVT32S2NX24HH(daccFilt);

      xb_vecN_2x32Uv vecQuotientLE, vecQuotientLO, vecQuotientHE, vecQuotientHO;
      vecCoeffSumL = IVP_SHFLNX16(vecCoeffSumL, vecPattern);
      vecCoeffSumH = IVP_SHFLNX16(vecCoeffSumH, vecPattern);
      xb_vecN_2x32Uv tmpquo, tmprem;

      tmpquo = vecResultLL << 24;
      tmprem = vecResultLL >> 8;

      IVP_DIVN_2X32X16U_4STEP(tmpquo, tmprem, vecCoeffSumL, 0);
      IVP_DIVN_2X32X16U_4STEPN(tmpquo, tmprem, vecCoeffSumL, 0);
      vecQuotientLE = tmpquo;

      tmpquo = vecResultLH << 24;
      tmprem = vecResultLH >> 8;
      IVP_DIVN_2X32X16U_4STEP(tmpquo, tmprem, vecCoeffSumL, 1);
      IVP_DIVN_2X32X16U_4STEPN(tmpquo, tmprem, vecCoeffSumL, 1);
      vecQuotientLO = tmpquo;

      tmpquo = vecResultHL << 24;
      tmprem = vecResultHL >> 8;
      IVP_DIVN_2X32X16U_4STEP(tmpquo, tmprem, vecCoeffSumH, 0);
      IVP_DIVN_2X32X16U_4STEPN(tmpquo, tmprem, vecCoeffSumH, 0);
      vecQuotientHE = tmpquo;

      tmpquo = vecResultHH << 24;
      tmprem = vecResultHH >> 8;
      IVP_DIVN_2X32X16U_4STEP(tmpquo, tmprem, vecCoeffSumH, 1);
      IVP_DIVN_2X32X16U_4STEPN(tmpquo, tmprem, vecCoeffSumH, 1);
      vecQuotientHO = tmpquo;

      vecQuotientLE = IVP_SELN_2X32I(vecQuotientLO, vecQuotientLE,
                                     IVP_SELI_16B_EXTRACT_1_OF_2_OFF_0);
      vecQuotientHE = IVP_SELN_2X32I(vecQuotientHO, vecQuotientHE,
                                     IVP_SELI_16B_EXTRACT_1_OF_2_OFF_0);

      vecResultL = IVP_MOVNX16_FROMN_2X32U(vecQuotientLE);
      vecResultH = IVP_MOVNX16_FROMN_2X32U(vecQuotientHE);
      // store the final 8b filtering output
      IVP_SVNX8U_IP(vecResultL, pvecResult, 32);
      IVP_SVNX8U_XP(vecResultH, pvecResult, widthExp - 32);
    }
  }
}
