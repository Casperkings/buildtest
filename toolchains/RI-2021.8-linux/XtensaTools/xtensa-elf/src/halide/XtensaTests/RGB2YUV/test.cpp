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
#include "halide_benchmark.h"
#include "HalideBuffer.h"
#include <iostream>

using namespace Halide::Runtime;
using namespace Halide::Tools;

/* Input/Output dimensions */
#define IMAGE_W       512
#define INPUT_IMAGE_W (IMAGE_W*3)
#define IMAGE_H       512
#define SRC_BIT_DEPTH 8
#define DST_BIT_DEPTH 8
#define TILE_W        128
#define INPUT_TILE_W  (TILE_W*3)
#define TILE_H        64
#define W_EXT         0
#define H_EXT         0
#define IMAGE_PAD     0

void memset_cust(void *str, int c, size_t n) {
  size_t i;
  uint8_t *ptr = (uint8_t *)str;
  for (i = 0; i < n; i++)
    ptr[i] = c;
}

void memcpy_cust(void *dst, const void *src, size_t n) {
  size_t i;
  uint8_t *ptrD = (uint8_t *)dst;
  uint8_t *ptrS = (uint8_t *)src;
  for (i = 0; i < n; i++)
    ptrD[i] = ptrS[i];
}

void run_reference(uint8_t*       Src,
                   uint32_t       SrcPitch,
                   uint8_t*       DstY,
                   uint8_t*       DstU,
                   uint8_t*       DstV,
                   const uint32_t DstPitch,
                   int32_t        width,
                   int32_t        height,
                   int32_t        dstWidth,
                   int32_t        dstHeight) {
  xi_tile src_t;
  xi_tile dstY_t, dstU_t, dstV_t;

  // SRC
  XI_TILE_SET_BUFF_PTR    ((&src_t), Src);
  XI_TILE_SET_DATA_PTR    ((&src_t),
                           ((uint8_t*)Src) + SrcPitch*IMAGE_PAD + IMAGE_PAD);
  XI_TILE_SET_BUFF_SIZE   ((&src_t), SrcPitch*(height + 2*IMAGE_PAD));
  XI_TILE_SET_WIDTH       ((&src_t), width);
  XI_TILE_SET_HEIGHT      ((&src_t), height);
  XI_TILE_SET_PITCH       ((&src_t), width + 2*IMAGE_PAD);
  XI_TILE_SET_TYPE        ((&src_t), XI_TILE_U8);
  XI_TILE_SET_EDGE_WIDTH  ((&src_t), IMAGE_PAD);
  XI_TILE_SET_EDGE_HEIGHT ((&src_t), IMAGE_PAD);
  XI_TILE_SET_X_COORD     ((&src_t), 0);
  XI_TILE_SET_Y_COORD     ((&src_t), 0);

  // DST_Y
  XI_TILE_SET_BUFF_PTR    ((&dstY_t), DstY);
  XI_TILE_SET_DATA_PTR    ((&dstY_t), DstY);
  XI_TILE_SET_BUFF_SIZE   ((&dstY_t), (DstPitch*(dstHeight)));
  XI_TILE_SET_WIDTH       ((&dstY_t), dstWidth);
  XI_TILE_SET_HEIGHT      ((&dstY_t), dstHeight);
  XI_TILE_SET_PITCH       ((&dstY_t), (DstPitch ));
  XI_TILE_SET_TYPE        ((&dstY_t), XI_TILE_U8);
  XI_TILE_SET_EDGE_WIDTH  ((&dstY_t), 0);
  XI_TILE_SET_EDGE_HEIGHT ((&dstY_t), 0);
  XI_TILE_SET_X_COORD     ((&dstY_t), 0);
  XI_TILE_SET_Y_COORD     ((&dstY_t), 0);
  
  // DST_U
  XI_TILE_SET_BUFF_PTR    ((&dstU_t), DstU);
  XI_TILE_SET_DATA_PTR    ((&dstU_t), DstU);
  XI_TILE_SET_BUFF_SIZE   ((&dstU_t), (DstPitch*(dstHeight)));
  XI_TILE_SET_WIDTH       ((&dstU_t), dstWidth);
  XI_TILE_SET_HEIGHT      ((&dstU_t), dstHeight);
  XI_TILE_SET_PITCH       ((&dstU_t), (DstPitch ));
  XI_TILE_SET_TYPE        ((&dstU_t), XI_TILE_U8);
  XI_TILE_SET_EDGE_WIDTH  ((&dstU_t), 0);
  XI_TILE_SET_EDGE_HEIGHT ((&dstU_t), 0);
  XI_TILE_SET_X_COORD     ((&dstU_t), 0);
  XI_TILE_SET_Y_COORD     ((&dstU_t), 0);
  
  // DST_V
  XI_TILE_SET_BUFF_PTR    ((&dstV_t), DstV);
  XI_TILE_SET_DATA_PTR    ((&dstV_t), DstV);
  XI_TILE_SET_BUFF_SIZE   ((&dstV_t), (DstPitch*(dstHeight)));
  XI_TILE_SET_WIDTH       ((&dstV_t), dstWidth);
  XI_TILE_SET_HEIGHT      ((&dstV_t), dstHeight);
  XI_TILE_SET_PITCH       ((&dstV_t), (DstPitch ));
  XI_TILE_SET_TYPE        ((&dstV_t), XI_TILE_U8);
  XI_TILE_SET_EDGE_WIDTH  ((&dstV_t), 0);
  XI_TILE_SET_EDGE_HEIGHT ((&dstV_t), 0);
  XI_TILE_SET_X_COORD     ((&dstV_t), 0);
  XI_TILE_SET_Y_COORD     ((&dstV_t), 0);
  
  xiCvtColor_U8_RGB2YUV_BT709_ref((xi_pArray) &src_t, 
                                  (xi_pArray) &dstY_t,
                                  (xi_pArray) &dstU_t, 
                                  (xi_pArray) &dstV_t);
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

#include "rgb2yuv.h"

int rgb2yuv_call(Buffer<uint8_t> in, Buffer<uint8_t> &out) {   
  rgb2yuv(in, out);
  out.device_sync();
  out.copy_to_host();
  return 0;
}

int main()
{
  // Grey-scale frame
  uint8_t *gsrc;
  uint8_t *gdstY, *gdstU, *gdstV;
  uint8_t *gdstY_ref, *gdstU_ref, *gdstV_ref;
  
  // Image dimensions and bit depth
  int32_t srcWidth, srcHeight, srcBytes;
  int32_t dstWidth, dstHeight, dstBytes;
  int32_t srcStride;

  // Set frame dimensions
  srcWidth  = INPUT_IMAGE_W;
  srcHeight = IMAGE_H;
  srcStride = (INPUT_IMAGE_W + IMAGE_PAD + IMAGE_PAD);
  srcBytes  = SRC_BIT_DEPTH>>3;
  dstWidth  = IMAGE_W;
  dstHeight = srcHeight;
  dstBytes  = DST_BIT_DEPTH>>3;

  // Grey-scale I/O frame buffers
  gsrc = (uint8_t*)malloc(srcStride * 
                          (srcHeight + IMAGE_PAD + IMAGE_PAD) * srcBytes);
  gdstY = (uint8_t*)malloc(dstWidth * dstHeight * dstBytes);
  gdstU = (uint8_t*)malloc(dstWidth * dstHeight * dstBytes);
  gdstV = (uint8_t*)malloc(dstWidth * dstHeight * dstBytes);
  
  gdstY_ref = (uint8_t*)malloc(dstWidth * dstHeight * dstBytes);
  gdstU_ref = (uint8_t*)malloc(dstWidth * dstHeight * dstBytes);
  gdstV_ref = (uint8_t*)malloc(dstWidth * dstHeight * dstBytes);
  if (!gsrc || !gdstY || !gdstU || !gdstV || !gdstY_ref || 
      !gdstU_ref || !gdstV_ref) {
    fprintf(stderr, "Unable to allocate memory\n");
    exit(0);
  }

  // Read input
  int indy;
  char fname[256];     // input image name
  // For image FILE IO
  FILE *fpIn;
  // Read input
  sprintf(fname, "lady_with_cap.yuv");
  fpIn = fopen(fname, "rb");
  if (fpIn == NULL) {
    printf("Error opening input file: %s\n", fname);
    exit(0);
  }

  for (indy = IMAGE_PAD; indy < (srcHeight + IMAGE_PAD); indy++) {
    uint8_t *ptr;
    ptr = gsrc + (indy * srcStride) + IMAGE_PAD;
    fread(ptr, 1, srcWidth * srcBytes, fpIn);
  }
  fclose(fpIn);

  // pad the input buffer
  std::cout << "pad image buffer\n";
  int i;
  uint8_t *ptrS, *ptrD;
  int padding_type = 0;
  ptrD = gsrc + IMAGE_PAD * srcStride;
  ptrS = gsrc + IMAGE_PAD + IMAGE_PAD * srcStride;

  // pad left and right sides
  for (i = IMAGE_PAD; i < (srcHeight + IMAGE_PAD); i++) {
    // pad left
    ptrD = gsrc + (i * srcStride);
    ptrS = ptrD + IMAGE_PAD;
    if (padding_type)
      memcpy_cust(ptrD, ptrS, IMAGE_PAD);
    else
      memset_cust(ptrD, 0, IMAGE_PAD);

    // pad right
    ptrD = gsrc + (i * srcStride) + IMAGE_PAD + srcWidth;
    ptrS = ptrD - 1;
    if (padding_type)
      memcpy_cust(ptrD, ptrS, IMAGE_PAD);
    else
      memset_cust(ptrD, 0, IMAGE_PAD);
  }

  ptrD = gsrc;
  ptrS = gsrc + IMAGE_PAD * srcStride;
  // pad top rows
  for (i = 0; i < IMAGE_PAD; i++) {
    if (padding_type)
      memcpy_cust(ptrD, ptrS, srcStride);
    else
      memset_cust(ptrD, 0, srcStride);
    ptrD += srcStride;
  }

  // pad bottom rows
  ptrD = gsrc + (srcHeight + IMAGE_PAD) * srcStride;
  ptrS = gsrc + (srcHeight - 1 + IMAGE_PAD) * srcStride;
  // pad top rows
  for (i = 0; i < IMAGE_PAD; i++) {
    if (padding_type)
      memcpy_cust(ptrD, ptrS, srcStride);
    else
      memset_cust(ptrD, 0, srcStride);
    ptrD += srcStride;
  }
  
  FILE *fpOutIn;
  // Write result
  sprintf(fname, "Input.yuv");
  fpOutIn = fopen(fname, "wb");
  if (fpOutIn == NULL) {
    printf("Error opening output file: %s\n", fname);
    exit(0);
  }
  fwrite(gsrc, srcStride* (srcHeight+IMAGE_PAD+IMAGE_PAD), 1, fpOutIn);
  fclose(fpOutIn);
  

  const int width = srcStride + IMAGE_PAD + IMAGE_PAD;
  const int height = srcHeight + IMAGE_PAD + IMAGE_PAD;
  
  //Input buffer
  Buffer<uint8_t> input = Buffer<uint8_t>(3, IMAGE_W, IMAGE_H);
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x= x+3) {
      input(0, x/3, y) = gsrc[y*height * 3 + x + 0];
      input(1, x/3, y) = gsrc[y*height * 3 + x + 1];
      input(2, x/3, y) = gsrc[y*height * 3 + x + 2];
    }
  }

  // Output buffer
  Buffer<uint8_t> Out(IMAGE_W, IMAGE_H, 3);
  rgb2yuv_call(input, Out);

  for (int y = 0; y < IMAGE_H; y++) {
    for (int x = 0; x < IMAGE_W; x++) {
      gdstY[y*IMAGE_H + x] = Out(x, y, 0);
      gdstU[y*IMAGE_H + x] = Out(x, y, 1);
      gdstV[y*IMAGE_H + x] = Out(x, y, 2);
    }
  }
  
  run_reference(gsrc, srcStride, gdstY_ref, gdstU_ref, 
                gdstV_ref, dstWidth, srcWidth, srcHeight, dstWidth, dstHeight);
  
  FILE *fpOut;
  // Write result
  sprintf(fname, "OutputY.yuv");
  fpOut = fopen(fname, "wb");
  if (fpOut == NULL) {
    printf("Error opening output file: %s\n", fname);
    exit(0);
  }

  fwrite(gdstY, dstWidth * dstHeight, 1, fpOut);
  fclose(fpOut);
  
  // Write result
  sprintf(fname, "OutputU.yuv");
  fpOut = fopen(fname, "wb");
  if (fpOut == NULL) {
    printf("Error opening output file: %s\n", fname);
    exit(0);
  }

  fwrite(gdstU, dstWidth * dstHeight, 1, fpOut);
  fclose(fpOut);
  
  // Write result
  sprintf(fname, "OutputV.yuv");
  fpOut = fopen(fname, "wb");
  if (fpOut == NULL) {
    printf("Error opening output file: %s\n", fname);
    exit(0);
  }

  fwrite(gdstV, dstWidth * dstHeight, 1, fpOut);
  fclose(fpOut);
 
  int pass_count = 0;
  if (memcmp(gdstY_ref, gdstY, dstWidth*dstHeight) == 0) {
    printf("PASS Y\n");
    pass_count++;
  } else {
    printf("FAIL Y\n");
  }

  if (memcmp(gdstU_ref, gdstU, dstWidth*dstHeight) == 0) {
    printf("PASS U\n");
    pass_count++;
  } else {
    printf("FAIL U\n");
  }

  if (memcmp(gdstV_ref, gdstV, dstWidth*dstHeight) == 0) {
    printf("PASS V\n");
    pass_count++;
  } else {
    printf("FAIL V\n");
  }

  if (pass_count == 3) {
    printf("COMPARE_TEST_PASS\n");
    pass_count++;
  }
  printf("FINISH\n");

  free(gsrc);
  free(gdstY);
  free(gdstU);
  free(gdstV);
  free(gdstY_ref);
  free(gdstU_ref);
  free(gdstV_ref);
  return 0;
}
