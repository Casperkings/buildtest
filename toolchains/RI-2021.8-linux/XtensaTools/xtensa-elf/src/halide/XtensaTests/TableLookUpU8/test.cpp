#include <cmath>
#include <cstdint>
#include <cstdio>
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
#include "halide_benchmark.h"
#include "HalideBuffer.h"
#include <iostream>

using namespace Halide::Runtime;
using namespace Halide::Tools;

#define IMAGE_W    512
#define IMAGE_H    512
#define SRC_BIT_DEPTH 8
#define DST_BIT_DEPTH 8
#define TILE_W     128
#define TILE_H     64
#define W_EXT    0
#define H_EXT    0
#define IMAGE_PAD  0

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

uint8_t LUT_U8[256] = 
{ 218,149, 25,187,146, 63, 67,157,  9,146,255, 25,129,110, 43,146,201,208,144, 77,254, 95, 49, 73, 71,115, 39,247,189,101,132, 78,
114,115, 29,138, 37, 53,102, 35,235,103,206, 42,167,169, 63, 66,193,113,156, 64,159,141,101,159,  2,237, 11,209, 82, 48,172,165,
 41,181,203,174, 60, 79,191,237,120,153, 48,183, 75,200,221,  3, 69,224,170,140, 45,119,114, 12,238,245,243, 75, 82, 25, 41,101,
129,255,247, 90,251,163,204,166,201, 52, 45, 76,194,195,163, 53,239,170, 63,158,184, 17, 54,157,  4,  8,224,220, 12,120,155,229,
127,121,121, 50,137,250,244, 68, 92,146,254, 29,225,170, 10, 64,202,119, 88,104, 23, 32, 49,134,164, 67,212,137,188,  1,178,122,
234,159, 29,100, 63,208,132,115, 16, 95,113, 22, 37, 71,115,152, 82,148,150,224, 10,161,  7, 17,194, 32,171, 28, 83, 57, 84, 49,
156,188,118, 51,141, 11,168, 86,131,117,151,174, 72,187, 48,  5,191, 56,191,212,  9, 74,136,137, 64,245,187,129,217, 36,220,184,
 12, 97, 87,  9,222, 27, 25,127,120,162,159,171,234,238, 39,139,147, 92,254, 24,249,184, 19,245,  0,140,253, 34,  5,120,228,114,
};

void run_reference(uint8_t*      Src,
				          uint32_t       SrcPitch,
                  uint8_t*       Dst,
                  const uint32_t DstPitch,
                  int32_t        width,
                  int32_t        height,
                  int32_t        dstWidth,
                  int32_t        dstHeight) {
	xi_tile src_t;
	xi_tile dst_t;

	// SRC
	XI_TILE_SET_BUFF_PTR    ((&src_t), Src);
  XI_TILE_SET_DATA_PTR    ((&src_t), 
                           ((uint8_t*)Src) + SrcPitch*IMAGE_PAD + IMAGE_PAD);
	XI_TILE_SET_BUFF_SIZE   ((&src_t), SrcPitch*(height + 2*IMAGE_PAD));
	XI_TILE_SET_WIDTH       ((&src_t), width+ 2*IMAGE_PAD);
	XI_TILE_SET_HEIGHT      ((&src_t), height+ 2*IMAGE_PAD);
	XI_TILE_SET_PITCH       ((&src_t), width + 2*IMAGE_PAD);
	XI_TILE_SET_TYPE        ((&src_t), XI_TILE_U8);
	XI_TILE_SET_EDGE_WIDTH  ((&src_t), IMAGE_PAD);
	XI_TILE_SET_EDGE_HEIGHT ((&src_t), IMAGE_PAD);
	XI_TILE_SET_X_COORD     ((&src_t), 0);
	XI_TILE_SET_Y_COORD     ((&src_t), 0);

	// DST_DX
	XI_TILE_SET_BUFF_PTR    ((&dst_t), Dst);
  XI_TILE_SET_DATA_PTR    ((&dst_t), Dst);
	XI_TILE_SET_BUFF_SIZE	  ((&dst_t), (DstPitch*(dstHeight)));
	XI_TILE_SET_WIDTH		    ((&dst_t), dstWidth);
	XI_TILE_SET_HEIGHT      ((&dst_t), dstHeight);
	XI_TILE_SET_PITCH       ((&dst_t), (DstPitch ));
	XI_TILE_SET_TYPE        ((&dst_t), XI_TILE_U8);
	XI_TILE_SET_EDGE_WIDTH  ((&dst_t), 0);
	XI_TILE_SET_EDGE_HEIGHT ((&dst_t), 0);
	XI_TILE_SET_X_COORD     ((&dst_t), 0);
	XI_TILE_SET_Y_COORD     ((&dst_t), 0);

	xiLUT_U8_ref(&src_t, &dst_t, LUT_U8, 256);
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

#include "TableLookUpU8.h"

int TableLookUpU8_call(Buffer<uint8_t> in, Buffer<uint8_t> lut, Buffer<uint8_t> &out) {
  TableLookUpU8(in, lut, out);
  out.device_sync();
  out.copy_to_host();
  return 1;
}

int main(int argc, char **argv) {
  // Grey-scale frame
  uint8_t *gsrc;
  uint8_t *gdst;
  uint8_t *gdst_ref;

  // Image dimensions and bit depth
  int32_t srcWidth, srcHeight, srcBytes;
  int32_t dstWidth, dstHeight, dstBytes;
  int32_t srcStride;

  // Set frame dimensions
  srcWidth  = IMAGE_W;
  srcHeight = IMAGE_H;
  srcStride = (IMAGE_W + IMAGE_PAD + IMAGE_PAD);
  srcBytes  = SRC_BIT_DEPTH>>3;
  dstWidth  = srcWidth;
  dstHeight = srcHeight;
  dstBytes  = DST_BIT_DEPTH>>3;

  gsrc = (uint8_t*)malloc(srcStride * 
                          (srcHeight + IMAGE_PAD + IMAGE_PAD) * srcBytes);
  gdst = (uint8_t*)malloc(dstWidth * dstHeight * dstBytes);
  gdst_ref = (uint8_t*)malloc(dstWidth * dstHeight * dstBytes);
  if (!gsrc || !gdst || !gdst_ref) {
    fprintf(stderr, "Unable to allocate memory\n");
    exit(0);
  }
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
  if (fpOutIn == NULL) {
    printf("Error opening output file: %s\n", fname);
    exit(0);
  }
  fwrite(gsrc, srcStride*(srcHeight+IMAGE_PAD+IMAGE_PAD), 1, fpOutIn);
  fclose(fpOutIn);

  // load the input yuv image to halide buffer
  const int width  = srcWidth + IMAGE_PAD + IMAGE_PAD;
  const int height = srcHeight + IMAGE_PAD + IMAGE_PAD;
  Buffer<uint8_t> input(width, height);
  for (int y = 0; y < input.height(); y++) {
    for (int x = 0; x < input.width(); x++)
      input(x, y) = gsrc[y*input.height() + x];
  }
  
  Buffer<uint8_t> Lut(256);
  for (int x = 0; x < 256; x++)
    Lut(x) = LUT_U8[x];

  Buffer<uint8_t> output(IMAGE_W, IMAGE_H);
  TableLookUpU8_call(input, Lut, output);

  for (int y = 0; y < output.height(); y++) {
    for (int x = 0; x < output.width(); x++) 
      gdst[y*output.height() + x] = output(x, y);
  }

  run_reference(gsrc, srcStride, gdst_ref, dstWidth, srcWidth, 
                srcHeight, dstWidth, dstHeight);

  FILE *fpOut;
  // Write result
  sprintf(fname, "Output.yuv");
  fpOut = fopen(fname, "wb");
  if (fpOut == NULL) {
    printf("Error opening output file: %s\n", fname);
    exit(0);
  }
  fwrite(gdst, IMAGE_W * IMAGE_H, 1, fpOut);
  fclose(fpOut);

  printf("Compare: ");
  if (memcmp(gdst_ref, gdst, dstWidth*dstHeight) == 0) {
		printf("COMPARE_TEST_PASS\n");
	} else {
		printf("COMPARE_TEST_FAIL\n");
	}
  free(gsrc);
  free(gdst);
}
