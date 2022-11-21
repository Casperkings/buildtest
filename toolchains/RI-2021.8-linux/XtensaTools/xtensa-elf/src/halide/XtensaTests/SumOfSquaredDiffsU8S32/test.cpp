#include <cmath>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include "halide_benchmark.h"
#include "HalideBuffer.h"
#include "xi_api_ref.h"
using namespace Halide::Runtime;
using namespace Halide::Tools;

/* Input/Output dimensions     */
#define IMAGE_W 512
#define IMAGE_H 512
#define OUT_IMAGE_W (IMAGE_H)//(IMAGE_W/16)
#define OUT_IMAGE_H (IMAGE_W/16)//(IMAGE_H)
#define SRC_BIT_DEPTH 8
#define DST_BIT_DEPTH 32
#define TILE_W 128
#define TILE_H 64
#define OUT_TILE_W (TILE_H) //(TILE_W/16)
#define OUT_TILE_H (TILE_W/16)//(TILE_H)

#define W_EXT 0
#define H_EXT 0
#define IMAGE_PAD 0

XI_ERR_TYPE SumOfSquaredDiffsU8S32_Ref(const uint8_t *pSrc1, 
                                       const uint8_t *pSrc2, 
                                       int32_t *pDstRef, 
                                       int32_t width, int32_t nvectors, 
                                       int32_t nlength);

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

void run_reference(uint8_t *Src, uint8_t *Src1, uint32_t SrcPitch, uint8_t *Dst,
                   const uint32_t DstPitch, int32_t width, int32_t height,
                   int32_t dstWidth, int32_t dstHeight) {
 
  SumOfSquaredDiffsU8S32_Ref(Src1, Src, (int32_t *)Dst, width, height, 16);
}

#include "SumOfSquaredDiffsU8S32.h"

Buffer<uint32_t> SumOfSquaredDiffsU8S32_call(Buffer<uint8_t> in, 
                                             Buffer<uint8_t> in1) {
  Buffer<uint32_t> out(OUT_IMAGE_W, OUT_IMAGE_H);
  SumOfSquaredDiffsU8S32(in, in1, out);
  // Sync device execution if any.
  out.device_sync();
  out.copy_to_host();
  return out;
}

int main(int argc, char **argv) {
 // Grey-scale frame
  uint8_t *gsrc;
  uint8_t *gsrc1;
  uint8_t *gdst;
  uint8_t *gdst_ref;

  // Image dimensions and bit depth
  int32_t srcWidth, srcHeight, srcBytes;
  int32_t dstWidth, dstHeight, dstBytes;
  int32_t srcStride;

  // Set frame dimensions
  srcWidth = IMAGE_W;
  srcHeight = IMAGE_H;
  srcStride = (IMAGE_W + IMAGE_PAD + IMAGE_PAD);
  srcBytes = SRC_BIT_DEPTH >> 3;
  dstWidth = OUT_IMAGE_W;
  dstHeight = OUT_IMAGE_H;
  dstBytes = DST_BIT_DEPTH >> 3;

  // Grey-scale I/O frame buffers
  gsrc = (uint8_t *)malloc(srcStride * (srcHeight + IMAGE_PAD + IMAGE_PAD) * 
                           srcBytes);
  gsrc1 = (uint8_t *)malloc(srcStride * srcBytes);
  gdst = (uint8_t *)malloc(dstWidth * dstHeight * dstBytes);
  gdst_ref = (uint8_t *)malloc(dstWidth * dstHeight * dstBytes);
  if (!gsrc || !gsrc1 || !gdst || !gdst_ref) {
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
    ptr = gsrc + (indy * srcStride) + IMAGE_PAD;
    fread(ptr, 1, srcWidth, fpIn);
  }
  fclose(fpIn);
  
  fpIn = fopen(fname, "rb");
  if (fpIn == NULL) {
    printf("Error opening input file: %s\n", fname);
    exit(0);
  }

  for (indy = IMAGE_PAD; indy < (srcHeight/2 + IMAGE_PAD); indy++) {
    uint8_t *ptr;
    ptr = gsrc1;// + (indy * srcStride) + IMAGE_PAD;
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
      memcpy(ptrD, ptrS, IMAGE_PAD);
    else
      memset(ptrD, 0, IMAGE_PAD);

    // pad right
    ptrD = gsrc + (i * srcStride) + IMAGE_PAD + srcWidth;
    ptrS = ptrD - 1;
    if (padding_type)
      memcpy(ptrD, ptrS, IMAGE_PAD);
    else
      memset(ptrD, 0, IMAGE_PAD);
  }

  ptrD = gsrc;
  ptrS = gsrc + IMAGE_PAD * srcStride;
  // pad top rows
  for (i = 0; i < IMAGE_PAD; i++) {
    if (padding_type)
      memcpy(ptrD, ptrS, srcStride);
    else
      memset(ptrD, 0, srcStride);
    ptrD += srcStride;
  }

  // pad bottom rows
  ptrD = gsrc + (srcHeight + IMAGE_PAD) * srcStride;
  ptrS = gsrc + (srcHeight - 1 + IMAGE_PAD) * srcStride;
  // pad top rows
  for (i = 0; i < IMAGE_PAD; i++) {
    if (padding_type)
      memcpy(ptrD, ptrS, srcStride);
    else
      memset(ptrD, 0, srcStride);
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
  fwrite(gsrc, srcStride * (srcHeight + IMAGE_PAD + IMAGE_PAD), 1, fpOutIn);
  fclose(fpOutIn);

  run_reference(gsrc, gsrc1, srcStride, gdst_ref, dstWidth, srcWidth, srcHeight,
                dstWidth, dstHeight);

  // load the input yuv image to halide buffer
  Buffer<uint8_t> inputsrc(IMAGE_W, IMAGE_H);
  Buffer<uint8_t> inputsrc1(IMAGE_W);

  for (int y = 0; y < IMAGE_H; y++) {
    for (int x = 0; x < IMAGE_W/16; x++) {
      for (int l = 0; l < 16; l++) 
        inputsrc(x*16 + l, y) = gsrc[y*IMAGE_W + x*16 + l];
    }
  }
  
  for (int x = 0; x < IMAGE_W/16; x++) {
    for (int l = 0; l < 16; l++) 
      inputsrc1(x*16 + l) = gsrc1[x*16 + l];
  }

  Buffer<uint32_t> halide = SumOfSquaredDiffsU8S32_call(inputsrc, inputsrc1);

  // copy the output to the memory from halide buffer
  uint32_t *ptr = (uint32_t *)gdst;
  
  for (int y = 0; y < OUT_IMAGE_H; y++) {
    for (int x = 0; x < OUT_IMAGE_W; x++) {
      ptr[y * OUT_IMAGE_W + x ] = halide(x, y);
    }
  }

  FILE *fpOut;
  // Write result
  sprintf(fname, "Output.yuv");
  fpOut = fopen(fname, "wb");
  if (fpOut == NULL) {
    printf("Error opening output file: %s\n", fname);
    exit(0);
  }
  fwrite(gdst, dstWidth*dstHeight*dstBytes, 1, fpOut);
  fclose(fpOut);
  
  // Write result
  sprintf(fname, "Output_ref.yuv");
  fpOut = fopen(fname, "wb");
  if (fpOut == NULL) {
    printf("Error opening output file: %s\n", fname);
    exit(0);
  }
  fwrite(gdst_ref, dstWidth*dstHeight*dstBytes, 1, fpOut);
  fclose(fpOut);
    
  if (memcmp(gdst_ref, gdst, dstWidth*dstHeight*dstBytes) == 0)
    printf("COMPARE_TEST_PASS\n");
  else 
    printf("COMPARE_TEST_FAIL\n");
    
  free(gsrc);
  free(gdst);
}
