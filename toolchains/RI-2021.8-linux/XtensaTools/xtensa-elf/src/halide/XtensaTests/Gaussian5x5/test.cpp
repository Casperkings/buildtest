#include <cmath>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include "halide_benchmark.h"
#include "HalideBuffer.h"

using namespace Halide::Runtime;
using namespace Halide::Tools;

#define IMAGE_W    512
#define IMAGE_H    512
#define SRC_BIT_DEPTH 8
#define DST_BIT_DEPTH 8
#define TILE_W     128
#define TILE_H     64
#define IMAGE_PAD  2

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

int compare_ref(const char *fname, int size, uint8_t *tmpOut) {
  int32_t i;
  uint8_t *tmpRef = (uint8_t *) malloc(size);
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

#include "gaussian5x5.h"

Buffer<uint8_t> gaussian5x5_call(Buffer<uint8_t> in) {
  Buffer<uint8_t> out(in.width()-4, in.height()-4);
  gaussian5x5(in, out);
  // Sync device execution if any.
  out.device_sync();
  out.copy_to_host();
  return out;
}

int main(int argc, char **argv) {
  // Grey-scale frame
  uint8_t *gsrc;
  uint8_t *gdst;

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
                          (srcHeight + IMAGE_PAD + IMAGE_PAD) *
                          srcBytes);
  gdst = (uint8_t*)malloc(dstWidth * dstHeight * dstBytes);
 
  if (!gsrc || !gdst ) {
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
  fpOutIn= fopen(fname, "wb");
  if (fpOutIn== NULL) {
    printf("Error opening output file: %s\n", fname);
    exit(0);
  }
  fwrite(gsrc, srcStride* (srcHeight+IMAGE_PAD+IMAGE_PAD), 1,fpOutIn);
  fclose(fpOutIn);

  // load the input yuv image to halide buffer
  const int width = srcWidth + IMAGE_PAD + IMAGE_PAD;
  const int height =srcHeight + IMAGE_PAD + IMAGE_PAD;
  Buffer<uint8_t> input(width, height);
  for (int y = 0; y < input.height(); y++) {
    for (int x = 0; x < input.width(); x++)
        input(x, y) = gsrc[y*input.height() + x];
  }

  Buffer<uint8_t> halide = gaussian5x5_call(input);

  // copy the output to the memory from halide buffer
  for (int y = 0; y < halide.height(); y++) {
    for (int x = 0; x < halide.width(); x++) 
      gdst[y*halide.height() + x] = halide(x, y);
  }

  FILE *fpOut;
  // Write result
  sprintf(fname, "Output.yuv");
  fpOut= fopen(fname, "wb");
  if (fpOut == NULL) {
    printf("Error opening output file: %s\n", fname);
    exit(0);
  }
  fwrite(gdst, IMAGE_W * IMAGE_H, 1, fpOut);
  fclose(fpOut);

  sprintf(fname, "Output_ref.yuv");
  if (compare_ref(fname, dstWidth*dstHeight, gdst) == 0)
    printf("COMPARE_TEST_PASS\n");
  else 
    printf("COMPARE_TEST_FAIL\n");
    
  free(gsrc);
  free(gdst);
}
