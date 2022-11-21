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

using namespace Halide::Runtime;
using namespace Halide::Tools;

/* Input/Output dimensions */
#define IMAGE_W        512
#define IMAGE_H        512
#define SRC_BIT_DEPTH  8
#define DST_BIT_DEPTH  8
#define TILE_W         128
#define TILE_H         64
#define W_EXT          0
#define H_EXT          0
#define IMAGE_PAD      0
#define OUT_IMAGE_W    (256)
#define OUT_IMAGE_H    (280)
#define OUT_TILE_W     (64)
#define OUT_TILE_H     (35)

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

void run_reference(uint8_t        *Src,
				           uint32_t        SrcPitch,
          			   uint8_t        *Dst,
				           const uint32_t  DstPitch,
				           int32_t         width,
				           int32_t         height,
				           int32_t         dstWidth,
				           int32_t         dstHeight)
{
	xi_tile src_t;
	xi_tile dst_t;

	// SRC
	XI_TILE_SET_BUFF_PTR    ((&src_t), Src);
  XI_TILE_SET_DATA_PTR    ((&src_t), ((uint8_t*)Src) + 
                                     SrcPitch*IMAGE_PAD + IMAGE_PAD);
	XI_TILE_SET_BUFF_SIZE   ((&src_t), SrcPitch*(height + 2*IMAGE_PAD));
	XI_TILE_SET_WIDTH       ((&src_t), width);
	XI_TILE_SET_HEIGHT      ((&src_t), height);
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
	
	XI_Q13_18 scale_x = (src_t.width  << 18) / dst_t.width;
	XI_Q13_18 scale_y = (src_t.height << 18) / dst_t.height;
	
	xiResizeBilinear_U8_Q13_18_ref(&src_t, &dst_t, scale_x, scale_y, 0, 0);
}

#include "Bilinear_Downscale.h"

int Bilinear_Downscale_call(Buffer<uint8_t> in, Buffer<uint8_t> &out)
{
  Bilinear_Downscale(in, out);
  out.device_sync();
  out.copy_to_host();
  return 1;
}

int main()
{
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
  dstWidth  = OUT_IMAGE_W;
  dstHeight = OUT_IMAGE_H;
  dstBytes  = DST_BIT_DEPTH>>3;

  // Grey-scale I/O frame buffers
  gsrc = (uint8_t*)malloc(srcStride * 
                          (srcHeight + IMAGE_PAD + IMAGE_PAD) * srcBytes);
  gdst = (uint8_t*)malloc(dstWidth * dstHeight * dstBytes);
  gdst_ref = (uint8_t*)malloc(dstWidth * dstHeight * dstBytes);
  if (!gsrc || !gdst || !gdst_ref) {
    fprintf(stderr, "Unable to allocate memory\n");
    exit(0);
  }

  // Read input
  int indy;
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
  fwrite(gsrc, srcStride* (srcHeight+IMAGE_PAD+IMAGE_PAD), 1, fpOutIn);
  fclose(fpOutIn);

  const int width = srcWidth + IMAGE_PAD + IMAGE_PAD;
  const int height = srcHeight + IMAGE_PAD + IMAGE_PAD;
  Buffer<uint8_t> input(width, height);
  for (int y = 0; y < input.height(); y++) {
    for (int x = 0; x < input.width(); x++) {   
      input(x, y) = gsrc[y*input.width() + x];
    }
  }

  Buffer<uint8_t> output(OUT_IMAGE_W, OUT_IMAGE_H);

  Bilinear_Downscale_call(input, output);

  run_reference(gsrc, srcStride, gdst_ref, dstWidth, srcWidth, 
                srcHeight, dstWidth, dstHeight);

  for (int y = 0; y < output.height(); y++) {
    for (int x = 0; x < output.width(); x++) {
      gdst[y*output.width() + x] = output(x, y);
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

  fwrite(gdst, dstWidth * dstHeight, 1, fpOut);
  fclose(fpOut);
  
  if (memcmp(gdst_ref, gdst, dstWidth*dstHeight) == 0) {
	  printf("COMPARE_TEST_PASS\n");
	} else {
	  printf("COMPARE_TEST_FAIL\n");
	}
  return 0;
}
