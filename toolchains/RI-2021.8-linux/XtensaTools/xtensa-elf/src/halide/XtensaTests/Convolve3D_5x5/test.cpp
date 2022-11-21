#include <CL/opencl.h>
#include <CL/cl.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <assert.h>
#include <stdlib.h>
#include <memory.h>
#include <time.h>
#include <sys/times.h>
#include <math.h>
#include "xi_api_ref.h"
#include "xi_tile3d_manager.h"
#include "halide_benchmark.h"
#include "HalideBuffer.h"

using namespace Halide::Runtime;
using namespace Halide::Tools;

/* Input/Output dimensions     */
#define IMAGE_D 64
#define IMAGE_W    20
#define IMAGE_H    10
#define SRC_BIT_DEPTH (8*IMAGE_D)
#define DST_BIT_DEPTH (8*IMAGE_D)
#define TILE_W     10
#define TILE_H     5
#define W_EXT    2
#define H_EXT    2
#define IMAGE_PAD  2

void conv5x5xD8b_Ref(xi_pTile3D pSrc, xi_pTile3D pDst, 
                     int8_t * __restrict pCoeff, int shift);

void run_reference(int8_t*        Src,
                   int8_t*        Coef,
                   uint32_t       SrcPitch,
                   int8_t*        Dst,
                   const uint32_t DstPitch,
                   int32_t        width,
                   int32_t        height,
                   int32_t        dstWidth,
                   int32_t        dstHeight,
                   int32_t        depth,
                   int32_t        shift,
                   int32_t        srcBytes,
                   int32_t        dstBytes) {
  xi_tile3D src_t;
  xi_tile3D dst_t;

  // Input
  XI_TILE3D_SET_DIM1(&src_t, width + IMAGE_PAD + IMAGE_PAD);
  XI_TILE3D_SET_DIM2(&src_t, height + IMAGE_PAD + IMAGE_PAD);
  XI_TILE3D_SET_DIM3(&src_t, depth);

  XI_TILE3D_SET_DIM1_PITCH(&src_t, SrcPitch);
  XI_TILE3D_SET_DATA_PTR(&src_t, (void *)Src);

  // Output_Ref
  XI_TILE3D_SET_DIM1(&dst_t, dstWidth);
  XI_TILE3D_SET_DIM2(&dst_t, dstHeight);
  XI_TILE3D_SET_DIM3(&dst_t, depth);

  XI_TILE3D_SET_DIM1_PITCH(&dst_t, DstPitch);
  XI_TILE3D_SET_DATA_PTR(&dst_t, (void *)Dst);

  conv5x5xD8b_Ref(&src_t, &dst_t, Coef, shift);
}

int compare_ref(const char *fname, int size, uint8_t *tmpOut) {
  int32_t i;
  uint8_t *tmpRef = (uint8_t *)malloc(size);
  FILE *fpRef = fopen(fname, "r");
  if (fpRef == NULL) {
    printf("Error opening reference file: %s\n", fname);
    exit(0);
  }
  fread(tmpRef, size, 1, fpRef);
  for (i = 0; i < size; i++) {
    if (tmpOut[i] != tmpRef[i]) {
      printf("%d %x %x\n", i, tmpOut[i], tmpRef[i]);
      return -1;
    }
  }
  return 0;
}

#include "convolve3D.h"

int convolve3D_call(Buffer<int8_t> in, Buffer<int8_t> ker, Buffer<int8_t> &out)
{ 
  convolve3D(in, ker, out);
  out.device_sync();
  out.copy_to_host();
  return 0;
}

int main()
{
  // Grey-scale frame
  int8_t *gsrc;
  int8_t *gcoef;
  int8_t *gdst;
  int8_t *gdst_ref;
  
  // Image dimensions and bit depth
  int32_t srcWidth, srcHeight, srcBytes;
  int32_t dstWidth, dstHeight, dstBytes;
  int32_t srcStride;

  int i, j, k, l;
  // Set frame dimensions
  srcWidth  = IMAGE_W;
  srcHeight = IMAGE_H;
  srcStride = (IMAGE_W + IMAGE_PAD + IMAGE_PAD);
  srcBytes  = SRC_BIT_DEPTH>>3;
  dstWidth  = srcWidth;
  dstHeight = srcHeight;
  dstBytes  = DST_BIT_DEPTH>>3;

  // Grey-scale I/O frame buffers
  gsrc = (int8_t *)malloc(srcStride * 
                          (srcHeight + IMAGE_PAD + IMAGE_PAD) * srcBytes);
  gcoef = (int8_t *)malloc(5 * 5 * IMAGE_D * IMAGE_D);
  gdst = (int8_t *)malloc(dstWidth * dstHeight * dstBytes);
  gdst_ref = (int8_t *)malloc(dstWidth * dstHeight * dstBytes);
  if (!gsrc || !gdst || !gdst_ref || !gcoef) {
    fprintf(stderr, "Unable to allocate memory\n");
    exit(0);
  }

  Buffer <int8_t> input(IMAGE_D, srcStride, srcHeight + IMAGE_PAD + IMAGE_PAD);
  Buffer <int8_t> kernel(IMAGE_D, 5, 5, IMAGE_D);
  Buffer <int8_t> output(IMAGE_D, IMAGE_W, IMAGE_H);

  // generate input
  char val = 1;
  for (i = 0; i < IMAGE_D; i++) {
    for (j = 0; j < IMAGE_H + IMAGE_PAD * 2; j++) {
      for (k = 0; k < IMAGE_W + IMAGE_PAD * 2; k++) {
       
        gsrc[(IMAGE_W + IMAGE_PAD * 2) * IMAGE_D * j + 
                                         IMAGE_D * k + 
                                                   i] = val*(rand()%16);
        input(i, k, j) = gsrc[(IMAGE_W + IMAGE_PAD * 2) * IMAGE_D * j + 
                                                          IMAGE_D * k + 
                                                                    i]; 
      }
    }
    val = -val;
  }

  int sum[IMAGE_D];
  for (l = 0; l < IMAGE_D; l++) {
    sum[l] = 0;
  }
  for (i = 0; i < IMAGE_D; i++) {
    char val = 1;
    for (j = 0; j < 5; j++) {
      for (k = 0; k < 5; k++) {
        for (l = 0; l < IMAGE_D; l++) {
            gcoef[IMAGE_D * 5 * 5 * i +
                      IMAGE_D * 5 * j +
                          IMAGE_D * k +
                                    l ] = val*(rand()%16);

            sum[l] += gcoef[IMAGE_D * 5 * 5 * i + // Kernel Depth
                                IMAGE_D * 5 * j + // Kernel Height
                                    IMAGE_D * k + // Kernel Width
                                              l ];// No of Kernels
        }
        val = val*(-1);
      }
    }
  }
  
  for (i = 0; i < IMAGE_D; i++) {
    for (j = 0; j < 5; j++) {
      for (k = 0; k < 5; k++) {
        for (l = 0; l < IMAGE_D; l++) {
          float scale = (float)63.0/sum[l];
          gcoef[IMAGE_D * 5 * 5 * i +
                    IMAGE_D * 5 * j +
                        IMAGE_D * k +
                                  l ] = (float)gcoef[IMAGE_D * 5 * 5 * i +
                                                         IMAGE_D * 5 * j +
                                                             IMAGE_D * k +
                                                                       l ] * scale;

          kernel( l, k, j, i) = gcoef[IMAGE_D * 5 * 5 * i +
                                          IMAGE_D * 5 * j +
                                              IMAGE_D * k +
                                                        l ];
        }
      }
    }
  }

  int shift = 7;
  int depth = IMAGE_D;

  convolve3D_call(input, kernel, output);

  run_reference(gsrc, gcoef, srcStride, gdst_ref, dstWidth, srcWidth,
                srcHeight, dstWidth, dstHeight, depth, shift, 
                srcBytes, dstBytes);

  // To output
  for (i = 0; i < IMAGE_D; i++) {
    for (j = 0; j < IMAGE_H; j++) {
      for (k = 0; k < IMAGE_W; k++) {
         gdst[IMAGE_W * IMAGE_D * j +
                        IMAGE_D * k +
                                  i ] = output(i, k, j);
      }
    }
 }

  FILE *fpOut;
  char fname[200];
  // write input
  sprintf(fname, "Input.yuv");
  fpOut = fopen(fname, "wb");
  if (fpOut == NULL) {
    printf("Error opening output file: %s\n", fname);
    exit(0);
  }
  fwrite(gsrc, srcStride * (srcHeight + IMAGE_PAD + IMAGE_PAD) * srcBytes, 
         1, fpOut);
  fclose(fpOut);

  // Write result
  sprintf(fname, "Output_halide.yuv");
  fpOut = fopen(fname, "wb");
  if (fpOut == NULL) {
    printf("Error opening output file: %s\n", fname);
    exit(0);
  }
  fwrite(gdst, dstWidth * dstHeight * dstBytes, 1, fpOut);
  fclose(fpOut);
  
  sprintf(fname, "Output_native.yuv");
  fpOut = fopen(fname, "wb");
  if (fpOut == NULL) {
    printf("Error opening output file: %s\n", fname);
    exit(0);
  }
  fwrite(gdst_ref, dstWidth * dstHeight * dstBytes, 1, fpOut);
  fclose(fpOut);
 
  printf("Compare: ");
  if (memcmp(gdst_ref, gdst, dstWidth*dstHeight*dstBytes) == 0) {
    printf("COMPARE_TEST_PASS \n");
  } else {
    printf("COMPARE_TEST_FAIL \n");
  }

  return 0;
}

