#include <cmath>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include "halide_benchmark.h"
#include "HalideBuffer.h"
#include "xi_api_ref.h"
using namespace Halide::Runtime;
using namespace Halide::Tools;

#define IMAGE_W    512
#define IMAGE_H    512
#define SRC_BIT_DEPTH 8
#define DST_BIT_DEPTH 32
#define TILE_W     128
#define TILE_H     64
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
void run_reference(uint8_t *Src, uint32_t SrcPitch, uint8_t *Dst,
                   const uint32_t DstPitch, int32_t width, int32_t height,
                   int32_t dstWidth, int32_t dstHeight) {
  uint32_t Col[TILE_H + 1] = {0}, Row[TILE_W+1]= {0};
  xi_tile src_col, src_row;
  XI_TILE_SET_BUFF_PTR((&src_col), Col);
  XI_TILE_SET_DATA_PTR((&src_col), Col);
  XI_TILE_SET_BUFF_SIZE((&src_col), (TILE_H + 1)*4);
  XI_TILE_SET_WIDTH((&src_col), 1);
  XI_TILE_SET_HEIGHT((&src_col), TILE_H + 1);
  XI_TILE_SET_PITCH((&src_col), 1);
  XI_TILE_SET_TYPE((&src_col), XI_TILE_U32);
  XI_TILE_SET_EDGE_WIDTH((&src_col), 0);
  XI_TILE_SET_EDGE_HEIGHT((&src_col), 0);

  XI_TILE_SET_BUFF_PTR((&src_row), Row);
  XI_TILE_SET_DATA_PTR((&src_row), Row);
  XI_TILE_SET_BUFF_SIZE((&src_row), (TILE_W + 1)*4);
  XI_TILE_SET_WIDTH((&src_row), TILE_W+ 1);
  XI_TILE_SET_HEIGHT((&src_row), 1);
  XI_TILE_SET_PITCH((&src_row), TILE_W+ 1);
  XI_TILE_SET_TYPE((&src_row), XI_TILE_U32);
  XI_TILE_SET_EDGE_WIDTH((&src_row), 0);
  XI_TILE_SET_EDGE_HEIGHT((&src_row), 0);
  
  xi_tile src_t;
  xi_tile dst_t;
  for (int32_t indy = 0; indy < height; indy += TILE_H) {
    for (int32_t indx = 0; indx < width; indx += TILE_W) {
      uint8_t *src_y;
      uint32_t *dst_y;
      src_y = Src + (indx + indy * SrcPitch);
      dst_y = ((uint32_t*)Dst) + (indx + indy * DstPitch);

      // SRC
      XI_TILE_SET_BUFF_PTR((&src_t), src_y);
      XI_TILE_SET_DATA_PTR((&src_t), ((uint8_t *)src_y) +
                                     SrcPitch * IMAGE_PAD + IMAGE_PAD);
      XI_TILE_SET_BUFF_SIZE((&src_t), SrcPitch * (TILE_H + 2 * IMAGE_PAD));
      XI_TILE_SET_WIDTH((&src_t), TILE_W + 2 * IMAGE_PAD);
      XI_TILE_SET_HEIGHT((&src_t), TILE_H + 2 * IMAGE_PAD);
      XI_TILE_SET_PITCH((&src_t), SrcPitch + 2 * IMAGE_PAD);
      XI_TILE_SET_TYPE((&src_t), XI_TILE_U8);
      XI_TILE_SET_EDGE_WIDTH((&src_t), IMAGE_PAD);
      XI_TILE_SET_EDGE_HEIGHT((&src_t), IMAGE_PAD);
      XI_TILE_SET_X_COORD((&src_t), indx);
      XI_TILE_SET_Y_COORD((&src_t), indy);

      // DST_DX
      XI_TILE_SET_BUFF_PTR((&dst_t), dst_y);
      XI_TILE_SET_DATA_PTR((&dst_t), dst_y);
      XI_TILE_SET_BUFF_SIZE((&dst_t), (DstPitch * (TILE_H)));
      XI_TILE_SET_WIDTH((&dst_t), TILE_W);
      XI_TILE_SET_HEIGHT((&dst_t), TILE_H);
      XI_TILE_SET_PITCH((&dst_t), (DstPitch));
      XI_TILE_SET_TYPE((&dst_t), XI_TILE_U8);
      XI_TILE_SET_EDGE_WIDTH((&dst_t), 0);
      XI_TILE_SET_EDGE_HEIGHT((&dst_t), 0);
      XI_TILE_SET_X_COORD((&dst_t), indx);
      XI_TILE_SET_Y_COORD((&dst_t), indy);
    
      xiIntegralImage_U8U32_ref((xi_pArray)&src_t, (xi_pArray)&src_col, 
                                (xi_pArray)&src_row, (xi_pArray)&dst_t);
    }
  }
}

#include "IntegralU8U32.h"

Buffer<uint32_t> IntegralU8U32_call(Buffer<uint8_t> in) {
  Buffer<uint32_t> out(TILE_W, TILE_H, IMAGE_W/TILE_W, IMAGE_H/TILE_H);
  IntegralU8U32(in, out);
  // Sync device execution if any.
  out.device_sync();
  out.copy_to_host();
  return out;
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

  gsrc = (uint8_t *)malloc(srcStride * 
                           (srcHeight + IMAGE_PAD + IMAGE_PAD) *
                           srcBytes);
  gdst = (uint8_t *)malloc(dstWidth * dstHeight * dstBytes);
  gdst_ref = (uint8_t *)malloc(dstWidth * dstHeight * dstBytes);
 
  if (!gsrc || !gdst || !gdst_ref) {
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
  
  run_reference(gsrc, srcStride, gdst_ref, dstWidth, srcWidth, 
                srcHeight, dstWidth, dstHeight);

  // load the input yuv image to halide buffer
  Buffer<uint8_t> input(TILE_W, TILE_H, IMAGE_W/TILE_W, IMAGE_H/TILE_H);

  for (int ty = 0; ty < IMAGE_H/TILE_H; ty++) {
    for (int tx = 0; tx < IMAGE_W/TILE_W; tx++) {
      for (int y = 0; y < TILE_H; y++) {
        for (int x = 0; x < TILE_W; x++) {
          input(x, y, tx, ty) = 
              gsrc[(y + ty * TILE_H)*IMAGE_W + x + tx * TILE_W];
        }
      }
    }
  }

  Buffer<uint32_t> halide = IntegralU8U32_call(input);

  // copy the output to the memory from halide buffer
  uint32_t *ptr = (uint32_t *)gdst;
  
  for (int ty = 0; ty < IMAGE_H/TILE_H; ty++) {
    for (int tx = 0; tx < IMAGE_W/TILE_W; tx++) {
      for (int y = 0; y < TILE_H; y++) {
        for (int x = 0; x < TILE_W; x++) {
          ptr[(y + ty * TILE_H)*IMAGE_W + x + tx * TILE_W] =
              halide(x, y, tx, ty);
        }
      }
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
    
  if (memcmp(gdst_ref, gdst, dstWidth*dstHeight*dstBytes) == 0)
    printf("COMPARE_TEST_PASS\n");
  else 
    printf("COMPARE_TEST_FAIL\n");
    
  free(gsrc);
  free(gdst);
}
