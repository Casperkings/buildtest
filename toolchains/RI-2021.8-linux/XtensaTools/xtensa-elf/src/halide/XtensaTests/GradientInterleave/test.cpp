#include <cmath>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include "halide_benchmark.h"
#include "HalideBuffer.h"
#include "xi_api_ref.h"
using namespace Halide::Runtime;
using namespace Halide::Tools;
XI_ERR_TYPE xiHOGGradient_interleave_U8S16_ref(const xi_pTile src, xi_pArray dst_dx_dy);
#define IMAGE_W    512
#define IMAGE_H    512
#define SRC_BIT_DEPTH 8
#define DST_BIT_DEPTH 16
#define TILE_W     128
#define TILE_H     64
#define IMAGE_PAD  1

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

int compare_ref(const char *fname, int size, int16_t *tmpOut) {
  int32_t i;
  int16_t *tmpRef = (int16_t *) malloc(size*2);

  FILE *fpRef = fopen(fname, "r");
  if (fpRef == NULL) {
    printf("Error opening reference file: %s\n", fname);
    exit(0);
  }
  fread(tmpRef, 2,size, fpRef);
  for (i = 0; i < size; i++) {
    if (tmpOut[i] != tmpRef[i]) {
	    printf("%d %x %x\n", i, tmpOut[i], tmpRef[i]);
      return -1;
    }
  }
  return 0;
}

void run_reference(uint8_t *Src, uint32_t SrcPitch, int16_t *DstXY,
                   const uint32_t DstXYPitch, int32_t width, int32_t height,
                   int32_t dstWidth, int32_t dstHeight) {
  xi_tile src_t;
  xi_tile dstXY_t;

  // SRC
  XI_TILE_SET_BUFF_PTR((&src_t), Src);
  XI_TILE_SET_DATA_PTR((&src_t),
                       ((uint8_t *)Src) + SrcPitch * IMAGE_PAD + IMAGE_PAD);
  XI_TILE_SET_BUFF_SIZE((&src_t), SrcPitch * (height + 2 * IMAGE_PAD));
  XI_TILE_SET_WIDTH((&src_t), width);
  XI_TILE_SET_HEIGHT((&src_t), height);
  XI_TILE_SET_PITCH((&src_t), width + 2 * IMAGE_PAD);
  XI_TILE_SET_TYPE((&src_t), XI_TILE_U8);
  XI_TILE_SET_EDGE_WIDTH((&src_t), IMAGE_PAD);
  XI_TILE_SET_EDGE_HEIGHT((&src_t), IMAGE_PAD);
  XI_TILE_SET_X_COORD((&src_t), 0);
  XI_TILE_SET_Y_COORD((&src_t), 0);

  // DST_DX DY
  XI_TILE_SET_BUFF_PTR((&dstXY_t), DstXY);
  XI_TILE_SET_DATA_PTR((&dstXY_t), DstXY);
  XI_TILE_SET_BUFF_SIZE((&dstXY_t),
                        (DstXYPitch * (dstHeight) * sizeof(uint16_t)));
  XI_TILE_SET_WIDTH((&dstXY_t), dstWidth);
  XI_TILE_SET_HEIGHT((&dstXY_t), dstHeight);
  XI_TILE_SET_PITCH((&dstXY_t), (DstXYPitch));
  XI_TILE_SET_TYPE((&dstXY_t), XI_TILE_S16);
  XI_TILE_SET_EDGE_WIDTH((&dstXY_t), 0);
  XI_TILE_SET_EDGE_HEIGHT((&dstXY_t), 0);
  XI_TILE_SET_X_COORD((&dstXY_t), 0);
  XI_TILE_SET_Y_COORD((&dstXY_t), 0);

  

  xiHOGGradient_interleave_U8S16_ref(&src_t, (xi_pArray)&dstXY_t);
}

#include "GradiantInterleave.h"

int main(int argc, char **argv) {
  // Grey-scale frame
  uint8_t *gsrc;
  int16_t *gdst;
  int16_t *gdst_ref;

  // Image dimensions and bit depth 
  int32_t srcWidth, srcHeight, srcBytes;
  int32_t dstWidth, dstHeight, dstBytes;
  int32_t srcStride;

  // Set frame dimensions
  srcWidth  = IMAGE_W;
  srcHeight = IMAGE_H;
  srcStride = (IMAGE_W + IMAGE_PAD + IMAGE_PAD);
  srcBytes  = SRC_BIT_DEPTH>>3;
  dstWidth  = srcWidth*2;
  dstHeight = srcHeight;
  dstBytes  = DST_BIT_DEPTH>>3;

  gsrc = (uint8_t*)malloc(srcStride * 
                          (srcHeight + IMAGE_PAD + IMAGE_PAD) * 
                          srcBytes);

  gdst = (int16_t*)malloc(dstWidth * dstHeight * dstBytes*2);
  gdst_ref = (int16_t*)malloc(dstWidth * dstHeight * dstBytes*2);
 
  if (!gsrc || !gdst) { 
    fprintf(stderr, "Unable to allocate memory\n");
    exit(0);
  }

  // Read input
  int indy;
  // input image name
  char fname[256];     
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
    fread(ptr, 1, srcWidth, fpIn);
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
  if (fpOutIn== NULL) {
    printf("Error opening output file: %s\n", fname);
    exit(0);
  }
  fwrite(gsrc, 1, srcStride* (srcHeight+IMAGE_PAD+IMAGE_PAD), fpOutIn);
  fclose(fpOutIn);

  run_reference(gsrc, srcStride, gdst_ref, dstWidth,
                srcWidth, srcHeight, dstWidth, dstHeight);
				
  // load the input yuv image to buffer
  const int width = srcWidth + IMAGE_PAD + IMAGE_PAD;
  const int height = srcHeight + IMAGE_PAD + IMAGE_PAD;
  Buffer<uint8_t> input(width, height);
  for (int y = 0; y < input.height(); y++) {
      for (int x = 0; x < input.width(); x++)
          input(x, y) = gsrc[y*input.width() + x];
  }

	
  Buffer<int16_t> out((input.width()-2)*2, input.height()-2);

  GradiantInterleave(input, out);
  out.device_sync();
  out.copy_to_host();
      
  // copy the dx output to the memory from buffer   
  for (int y = 0; y < IMAGE_H; y++) {
    for (int x = 0; x < IMAGE_W*2; x++) {
      gdst[y*IMAGE_W*2 + x] = out(x, y);
  }}

  // Write dx result to file
  FILE *fpOut;
  sprintf(fname, "output.yuv");
  fpOut= fopen(fname, "wb");
  if (fpOut == NULL) {
    printf("Error opening output file: %s\n", fname);
    exit(0);
  }
  fwrite(gdst, 2, IMAGE_W * IMAGE_H * 2, fpOut);
  fclose(fpOut);

  
  printf("Compare: ");
  if (memcmp(gdst_ref, gdst, dstWidth*dstHeight * dstBytes*2) == 0) {
  	printf("COMPARE_TEST_PASS\n");
  } else {
  	printf("COMPARE_TEST_FAIL\n");
  }
    
  free(gsrc);
  free(gdst);
}
