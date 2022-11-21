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
#define OUTPUT_IMAGE_W (IMAGE_W*3)
#define IMAGE_H       512
#define SRC_BIT_DEPTH 8
#define DST_BIT_DEPTH 8
#define TILE_W        128
#define OUTPUT_TILE_W  (TILE_W*3)
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

void run_reference(uint8_t *Dst, uint32_t DstPitch, uint8_t *SrcY,
                   uint8_t *SrcU, uint8_t *SrcV, const uint32_t SrcPitch,
                   int32_t srcWidth, int32_t srcHeight, int32_t dstWidth,
                   int32_t dstHeight) {
  xi_tile dst_t;
  xi_tile srcY_t, srcU_t, srcV_t;

  // SRC
  XI_TILE_SET_BUFF_PTR((&dst_t), Dst);
  XI_TILE_SET_DATA_PTR((&dst_t),
                       ((uint8_t *)Dst) + DstPitch * IMAGE_PAD + IMAGE_PAD);
  XI_TILE_SET_BUFF_SIZE((&dst_t), DstPitch * (dstHeight + 2 * IMAGE_PAD));
  XI_TILE_SET_WIDTH((&dst_t), dstWidth);
  XI_TILE_SET_HEIGHT((&dst_t), dstHeight);
  XI_TILE_SET_PITCH((&dst_t), dstWidth + 2 * IMAGE_PAD);
  XI_TILE_SET_TYPE((&dst_t), XI_TILE_U8);
  XI_TILE_SET_EDGE_WIDTH((&dst_t), IMAGE_PAD);
  XI_TILE_SET_EDGE_HEIGHT((&dst_t), IMAGE_PAD);
  XI_TILE_SET_X_COORD((&dst_t), 0);
  XI_TILE_SET_Y_COORD((&dst_t), 0);

  // DST_Y
  XI_TILE_SET_BUFF_PTR((&srcY_t), SrcY);
  XI_TILE_SET_DATA_PTR((&srcY_t), SrcY);
  XI_TILE_SET_BUFF_SIZE((&srcY_t), (SrcPitch * (srcHeight)));
  XI_TILE_SET_WIDTH((&srcY_t), srcWidth);
  XI_TILE_SET_HEIGHT((&srcY_t), srcHeight);
  XI_TILE_SET_PITCH((&srcY_t), (SrcPitch));
  XI_TILE_SET_TYPE((&srcY_t), XI_TILE_U8);
  XI_TILE_SET_EDGE_WIDTH((&srcY_t), 0);
  XI_TILE_SET_EDGE_HEIGHT((&srcY_t), 0);
  XI_TILE_SET_X_COORD((&srcY_t), 0);
  XI_TILE_SET_Y_COORD((&srcY_t), 0);

  // DST_U
  XI_TILE_SET_BUFF_PTR((&srcU_t), SrcU);
  XI_TILE_SET_DATA_PTR((&srcU_t), SrcU);
  XI_TILE_SET_BUFF_SIZE((&srcU_t), (SrcPitch * (srcHeight))/4);
  XI_TILE_SET_WIDTH((&srcU_t), srcWidth/2);
  XI_TILE_SET_HEIGHT((&srcU_t), srcHeight/2);
  XI_TILE_SET_PITCH((&srcU_t), (SrcPitch)/2);
  XI_TILE_SET_TYPE((&srcU_t), XI_TILE_U8);
  XI_TILE_SET_EDGE_WIDTH((&srcU_t), 0);
  XI_TILE_SET_EDGE_HEIGHT((&srcU_t), 0);
  XI_TILE_SET_X_COORD((&srcU_t), 0);
  XI_TILE_SET_Y_COORD((&srcU_t), 0);

  // DST_V
  XI_TILE_SET_BUFF_PTR((&srcV_t), SrcV);
  XI_TILE_SET_DATA_PTR((&srcV_t), SrcV);
  XI_TILE_SET_BUFF_SIZE((&srcV_t), (SrcPitch * (srcHeight))/4);
  XI_TILE_SET_WIDTH((&srcV_t), srcWidth/2);
  XI_TILE_SET_HEIGHT((&srcV_t), srcHeight/2);
  XI_TILE_SET_PITCH((&srcV_t), (SrcPitch)/2);
  XI_TILE_SET_TYPE((&srcV_t), XI_TILE_U8);
  XI_TILE_SET_EDGE_WIDTH((&srcV_t), 0);
  XI_TILE_SET_EDGE_HEIGHT((&srcV_t), 0);
  XI_TILE_SET_X_COORD((&srcV_t), 0);
  XI_TILE_SET_Y_COORD((&srcV_t), 0);

  xiCvtColor_U8_YUV2RGB_420_BT709_ref((xi_pArray)&srcY_t,
                                      (xi_pArray)&srcU_t, 
                                      (xi_pArray)&srcV_t, 
                                      (xi_pArray)&dst_t);
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

#include "yuv420torgb.h"

int yuv420torgb_call(Buffer<uint8_t> inY, Buffer<uint8_t> inU, 
                     Buffer<uint8_t> inV, Buffer<uint8_t> &out) {   
  yuv420torgb(inY, inU, inV, out);
  out.device_sync();
  out.copy_to_host();
  return 0;
}

int main()
{
   // Grey-scale frame
  uint8_t *gdst;
  uint8_t *gdst_ref;
  uint8_t *gsrcY, *gsrcU, *gsrcV;

  // Image dimensions and bit depth
  int32_t srcWidth, srcHeight, srcBytes;
  int32_t dstWidth, dstHeight, dstBytes;
  int32_t srcStride;

  // Set frame dimensions
  srcWidth = IMAGE_W;
  srcHeight = IMAGE_H;
  srcStride = (IMAGE_W + IMAGE_PAD + IMAGE_PAD);
  srcBytes = SRC_BIT_DEPTH >> 3;
  dstWidth = OUTPUT_IMAGE_W;
  dstHeight = srcHeight;
  dstBytes = DST_BIT_DEPTH >> 3;

  // Grey-scale I/O frame buffers
  gdst = (uint8_t *)malloc(dstWidth * dstHeight * dstBytes);
  gsrcY = (uint8_t *)malloc(srcStride * srcHeight * srcBytes);
  gsrcU = (uint8_t *)malloc((srcStride * srcHeight * srcBytes)/4);
  gsrcV = (uint8_t *)malloc((srcStride * srcHeight * srcBytes)/4);

  gdst_ref = (uint8_t *)malloc(dstWidth * dstHeight * dstBytes);
  if (!gdst || !gsrcY || !gsrcU || !gsrcV ) {
    fprintf(stderr, "Unable to allocate memory\n");
    exit(0);
  }

  // Read input
  int indy;
  char fname[256]; // input image name
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
    ptr = gsrcY + (indy * srcStride) + IMAGE_PAD;
    fread(ptr, 1, srcWidth * srcBytes, fpIn);
    ptr = gsrcU + (indy/2 * srcStride/2) + IMAGE_PAD;
    fread(ptr, 1, srcWidth * srcBytes/2, fpIn);
    ptr = gsrcV + (indy/2 * srcStride/2) + IMAGE_PAD;
    fread(ptr, 1, srcWidth * srcBytes/2, fpIn);
  }
  fclose(fpIn);

  const int width = srcStride + IMAGE_PAD + IMAGE_PAD;
  const int height = srcHeight + IMAGE_PAD + IMAGE_PAD;
  
  //Input buffer
  Buffer<uint8_t> inputY = Buffer<uint8_t>(IMAGE_W, IMAGE_H);
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x = x+1) {
      inputY(x, y) = gsrcY[y*height + x];
    }
  }
  
  Buffer<uint8_t> inputU = Buffer<uint8_t>(IMAGE_W/2, IMAGE_H/2);
  for (int y = 0; y < height/2; y++) {
    for (int x = 0; x < width/2; x = x+1) {
      inputU(x, y) = gsrcU[y*(height/2) + x];
    }
  }
  
  Buffer<uint8_t> inputV = Buffer<uint8_t>(IMAGE_W/2, IMAGE_H/2);
  for (int y = 0; y < height/2; y++) {
    for (int x = 0; x < width/2; x = x+1) {
      inputV(x, y) = gsrcV[y*(height/2) + x];
    }
  }

  // Output buffer
  Buffer<uint8_t> Out(3, IMAGE_W, IMAGE_H);
  yuv420torgb_call(inputY, inputU, inputV, Out);

  for (int y = 0; y < IMAGE_H; y++) {
    for (int x = 0; x < IMAGE_W; x++) {
      gdst[y*IMAGE_W*3 + x*3 + 0] = Out(0, x, y);
      gdst[y*IMAGE_W*3 + x*3 + 1] = Out(1, x, y);
      gdst[y*IMAGE_W*3 + x*3 + 2] = Out(2, x, y);
    }
  }
  
  run_reference(gdst_ref, dstWidth, gsrcY, gsrcU, gsrcV, srcStride,
                srcWidth, srcHeight, dstWidth, dstHeight);
        
  FILE *fpOut;
  // Write result
  sprintf(fname, "OutputRGB.yuv");
  fpOut = fopen(fname, "wb");
  if (fpOut == NULL) {
    printf("Error opening output file: %s\n", fname);
    exit(0);
  }

  fwrite(gdst, dstWidth * dstHeight , 1, fpOut);
  fclose(fpOut);
  
  if (memcmp(gdst_ref, gdst, dstWidth * dstHeight) == 0) {
    printf("COMPARE_TEST_PASS\n");
  } else {
    printf("COMPARE_TEST_FAIL\n");
  }

  return 0;
}
