/*
 * functions.c
 *
 *  Created on: Jan 21, 2016
 *      Author: kamleshp
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include "xi_api_ref.h"

#define LUT_SHIFT 2

/* *****************************************************************************************
 * FUNCTION: GenGaussianCoeff5x5S()
 *
 *   Generates 5x5 Gaussian spatial kernel for bilateral filter
 *
 *   DESCRIPTION
 *
 *      This function generates a 5x5 Gaussian spatial kernel for the bilateral
 *filter function based on a user defined sigma. The function outputs both a
 *fixed-point and floating-point version of the kernel to be used by the
 *corresponding fixed/floating-point functions.
 *
 *  INPUTS:
 *
 *    float sigma             Pointer to user-defined sigma
 *
 *  OUTPUTS:
 *
 *    float   *ptrGauss       Pointer to floating point spatial kernel
 *
 *    int16_t *ptrGaussFixed  Pointer to fixed point spatial kernel
 *
 ********************************************************************************************/
void GenGaussianCoeff5x5S(float *ptrGauss, int16_t *ptrGaussFixed,
                          float sigma) {
  float scaledVar, sum, scale;
  int16_t x, y, coeffFixed;

  // sum is for normalization
  sum = 0.0;
  scaledVar = 2.0 * sigma * sigma;

  // generate 5x5 kernel
  for (x = -2; x <= 2; x++) {
    for (y = -2; y <= 2; y++) {
      ptrGauss[((x + 2) * 5) + (y + 2)] = exp(-((x * x) + (y * y)) / scaledVar);
      sum += ptrGauss[((x + 2) * 5) + (y + 2)];
    }
  }

  // We let all S-coeff sum up to 256. Scaling by 32 is needed to compensate for
  // >> 10 in PackP We split >> 10 as >> 5 absorbed in S-coeff, and >> 5
  // absorbed in I coeff This is the scaling factor for fixed point
  // representation
  scale = (256.0 * 32.0) / sum;

  for (x = 0; x < 5; ++x) {
    for (y = 0; y < 5; ++y) {
      coeffFixed = (int16_t)((scale * ptrGauss[(x * 5) + y]) + 0.5);
      ptrGaussFixed[(x * 5) + y] = coeffFixed;
    }
  }

  return;
}

/* *****************************************************************************************
 * FUNCTION: GenGaussianCoeffI()
 *
 *   Generates Gaussian range kernel for bilateral filter
 *
 *   DESCRIPTION
 *
 *      This function generates a Gaussian range kernel for the bilateral
 *filter. The function outputs both a fixed-point and floating-point version of
 *the kernel to be used by the corresponding fixed/floating-point functions.
 *
 *  INPUTS:
 *
 *    float   sigma           Pointer to user-defined sigma
 *
 *    int32_t len             Quantization length of difference range for fixed
 *point kernel
 *
 *  OUTPUTS:
 *
 *    float   *ptrGauss       Pointer to floating point range kernel
 *
 *    int16_t *ptrGaussFixed  Pointer to fixed point range kernel
 *
 ********************************************************************************************/
void GenGaussianCoeffI(float *ptrGauss, int16_t *ptrGaussFixed, float sigma,
                       int32_t len) {
  float scaledVar, scale, tempFloat;
  int16_t coeffFixed;
  int32_t indx, skip, indxMod;

  // sum is for normalization
  scaledVar = 2.0 * sigma * sigma;

  // Max value of fixed point I-Coeff should be < 32, Scaling by 32 is needed to
  // compensate for >> 10 in PackP. We split >> 10 as >> 5 absorbed in S-coeff,
  // and >> 5 absorbed in I coeff
  // This is the scaling factor for fixed point representation
  for (indx = 0; indx < 256; indx++) {
    ptrGauss[indx] = exp(-((double)(indx * indx)) / scaledVar);
  }

  scale = (double)((32 * 32) - 1);
  skip = 256 / len;

  for (indx = 0; indx < len; indx++) {
    indxMod = indx * skip; // increment of 1 in ptrGaussFixed[] corresponds to
                           // increment of skip in ptrGauss[]
    tempFloat = exp(-((float)(indxMod * indxMod)) / scaledVar);
    coeffFixed = (int16_t)((tempFloat * scale) + 0.5);
    ptrGaussFixed[indx] = coeffFixed;
  }

  // printf("\n");

  return;
}

void prepare5x5CoeffBuffer(int16_t *SCoeffFixed, int16_t *ICoeffFixed,
                           uint8_t *coeffBuffer) {
  /*************************************************************************
   * Precomputed product of range & spatial coefficients that will be used
   * during lookup operations
   *************************************************************************/
  int32_t index, i;
  uint8_t *__restrict pvecCoeff; // Used to load coefficient for pre-compute
  uint16_t Max_S, Max_I;
  float scale8b;
  Max_S = SCoeffFixed[12]; // mid value of 5x5 filter coeff is MAX
  Max_I = ICoeffFixed[0];  // first value is MAX
  scale8b = 255.0 / (Max_S * Max_I);

  pvecCoeff = (uint8_t *)coeffBuffer;

  for (index = 0; index < 5; index++) {
    for (i = 0; i < 64; i++) {
      int16_t coeffS = SCoeffFixed[0 + 5 * index];
      int16_t coeffI = ICoeffFixed[i];
      *pvecCoeff++ = (uint8_t)((((int32_t)coeffS * coeffI)) * scale8b);
    }
    for (i = 0; i < 64; i++) {
      *pvecCoeff++ = (uint8_t)(0);
    }

    for (i = 0; i < 64; i++) {
      int16_t coeffS = SCoeffFixed[1 + 5 * index];
      int16_t coeffI = ICoeffFixed[i];
      *pvecCoeff++ = (uint8_t)((((int32_t)coeffS * coeffI)) * scale8b);
    }
    for (i = 0; i < 64; i++) {
      *pvecCoeff++ = (uint8_t) (0);
    }

    for (i = 0; i < 64; i++) {
      int16_t coeffS = SCoeffFixed[2 + 5 * index];
      int16_t coeffI = ICoeffFixed[i];
      *pvecCoeff++ = (uint8_t)((((int32_t)coeffS * coeffI)) * scale8b);
    }
    for (i = 0; i < 64; i++) {
      *pvecCoeff++ = (uint8_t)(0);
    }
  }
}

/* *****************************************************************************************
 * FUNCTION: Filter5x5BilateralFixedPt()
 *
 *   Reference Fixed-Point 5x5 bilateral filter
 *
 *   DESCRIPTION
 *
 *      This function provides an edge preserving filter used for many image
 *processing and computer vision applications such as noise removal and depth
 *map filtering. The input to this function is an 8-bit unsigned grayscale
 *image, from which a 5x5 bilateral filter is executed.
 *
 *  INPUTS:
 *
 *    xi_pTile  src          Pointer to source tile structure
 *
 *    xi_pTile  dst          Pointer to destination tile structure
 *
 ********************************************************************************************/
void Filter5x5BilateralFixedPt(xi_pTile src, xi_pTile dst, int16_t *SCoeffFixed,
                               int16_t *ICoeffFixed) {

  uint8_t *filtInStart;
  uint8_t *filtOut;
  int32_t width;
  int32_t pitch;
  int32_t height;
  uint8_t *filtIn;
  // Image x,y loop index
  int32_t ix, iy;

  // Filter kernel x,y loop index
  int32_t ix1, iy1;

  // Current pixel index
  int32_t indx;

  // Neighboring pixel index
  int32_t indx1;

  // Spatial and Range coefficient index
  int32_t indxS, indxI;

  // Filtered output index
  int32_t indxOut;

  int16_t coeffS, coeffI; // filter coefficients
  uint8_t coeffProd;      // Range and Spatial coefficient product

  int32_t
      filtSumAcc; // represents the 24-bit accumulator of horizontal filter sum

  int16_t /* filtSum, */ wtSum; // partial accumulator results

  float scale8b;
  uint16_t Max_S, Max_I;
  Max_S = SCoeffFixed[12]; // mid value of 5x5 filter coeff is MAX
  Max_I = ICoeffFixed[0];  // first value is MAX
  scale8b = 255.0 / (Max_S * Max_I);
  iy = 0;

  filtInStart = (uint8_t *)XI_TILE_GET_BUFF_PTR(src);
  filtOut = (uint8_t *)XI_TILE_GET_DATA_PTR(dst);
  width = XI_TILE_GET_WIDTH(src);
  height = XI_TILE_GET_HEIGHT(src);
  pitch = XI_TILE_GET_PITCH(src);
  int32_t PadHeight = XI_TILE_GET_EDGE_HEIGHT(src);
  int32_t PadWidth = XI_TILE_GET_EDGE_WIDTH(src);

  // Set filtIn to start of data. Note padWidth = 64, padHeight = 2
  filtIn = filtInStart + (PadHeight * pitch) + PadWidth;
  for (iy = 0; iy < height; iy++) // skipping 2 boundary columns and rows
  {
    for (ix = 0; ix < width; ix++) {
      indx = (iy * pitch) + ix;
      indxOut = (iy * width) + ix;

      // filtSum    = 0;
      wtSum = 0;
      indxS = 0;
      filtSumAcc = 0;

      for (iy1 = iy - 2; iy1 <= iy + 2; iy1++) {
        for (ix1 = ix - 2; ix1 <= ix + 2;
             ix1++) { // Compute neighboring pixel index
          indx1 = (iy1 * (pitch)) + ix1;

          // Lookup spatial coefficient
          coeffS = SCoeffFixed[indxS];

          // Compute absolute difference for index to range kernel
          indxI = (int32_t)(filtIn[indx] - filtIn[indx1]);
          if (indxI < 0) {
            indxI = -indxI;
          }

          // Shift so that index is in range 0-63
          indxI = indxI >> LUT_SHIFT;

          // Lookup range coefficient
          coeffI = ICoeffFixed[indxI];

          // Compute coefficient product in the manner that IVP_MULPACKP
          // instruction would
          coeffProd = (uint8_t)((((int32_t)coeffS * coeffI)) * scale8b);

          filtSumAcc += (int32_t)filtIn[indx1] * (int32_t)coeffProd;
          wtSum += coeffProd;

          indxS++;
        }
      }

      filtOut[indxOut] = (uint8_t)(filtSumAcc / wtSum);
    }
  }

  return;
}
