#include <iostream>
#include <math.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <iostream>
#include "halide_benchmark.h"
#include "HalideBuffer.h"
#include "FFT2D_ref/fft.h"

#define IMAGE_W       512
#define IMAGE_H       512
#define SRC_BIT_DEPTH 32
#define DST_BIT_DEPTH 32
#define TILE_W        128
#define TILE_H        64
#define OUTPUT_TILE_W 128
#define OUTPUT_TILE_H 64

using namespace Halide::Runtime;
using namespace Halide::Tools;

void memcpy2d(void *dst, int dstpitch, void *src, int srcpitch, int width,
              int height) {
  for (int i = 0; i < height; i++)
    memcpy(((char *)dst) + i * dstpitch, ((char *)src) + i * srcpitch, width);
}

void run_reference( int32_t *SrcY, uint32_t SrcPitch, 
                    int32_t *Final_Out_re, int32_t *Final_Out_im,
                    int32_t width, int32_t height,
                    int16_t *W64, int32_t *Out_idx) {
  int32_t *SrcTile = (int32_t *)malloc(sizeof(int32_t) * 64 * 64 * 2);
  int32_t *DstTile = (int32_t *)malloc(sizeof(int32_t) * 64 * 64 * 2);
  int32_t *Out_re = (int32_t *)malloc(sizeof(int32_t) * 64 * 64);
  int32_t *Out_im = (int32_t *)malloc(sizeof(int32_t) * 64 * 64);
  int32_t *pTemp = (int32_t *)malloc(sizeof(int32_t) * 64 * 64 * 4);
  
  for (int32_t indy = 0; indy < height; indy += OUTPUT_TILE_H) {
    for (int32_t indx = 0; indx < width; indx += OUTPUT_TILE_W) {
      int32_t *src_y;
      src_y   = SrcY + (indx + indy * SrcPitch);
      memcpy2d(SrcTile, 64 * 2 * 4, src_y, IMAGE_W * 4, 64 * 4 * 2, 64);
      fft_radix4_vertical_64(SrcTile, W64, Out_idx, pTemp, Out_re, Out_im);
      fft128_horizontal_complex_Cref(Out_re, Out_im, DstTile);
      // Copy to Out_re and Out_im
      for (int i = 0; i < 64; i++) {
        for (int j = 0; j < 64; j++) {
          Final_Out_re[(indy + i)*width/2 + (indx/2 + j)] = 
                                            DstTile[i*64*2 + j*2];
          Final_Out_im[(indy + i)*width/2 + (indx/2 + j)] = 
                                            DstTile[i*64*2 + j*2 + 1];
        }
      }
    }
  }
}

#include "FFT2D.h"

void run_device(int32_t *input, int16_t *W64,
                int32_t *Out_re, int32_t *Out_im,
                int32_t width, int32_t height ) {
  Buffer<int32_t> input_buf(IMAGE_W, IMAGE_H);
  Buffer<int32_t> W64_buf(64*2);
  Buffer<int32_t> W64_horiz_buf(64*2);
  Buffer<int32_t> FinalOut(64, 64, 2, width/128, height/64);
  
  for (int i = 0; i < IMAGE_H; i++) {
    for (int j = 0; j < IMAGE_W; j++) {
      input_buf(j, i) = input[i*IMAGE_W + j];
    }
  }
  
  for(int i = 0; i < 64; i++) {
    double arg  = (2 * PI * i) / 64;
    W64_horiz_buf(2*i+0) = (short) ((double) 32767.0 * cos(arg));
    W64_horiz_buf(2*i+1) = -(short) ((double) 32767.0 * sin(arg));
    
    int c = (int) (cos(2 * PI * i / 64) * (1 << 15));
    int s = (int) (-sin(2 * PI * i / 64) * (1 << 15));
    
    W64_buf(2*i + 0)      = (short) ((c > 32767) ? 32767 : c);
    W64_buf(2*i + 1)      = (short) ((s > 32767) ? 32767 : s);
  }

  W64_buf(0) += 1;
  
  FFT2D(input_buf, W64_buf, W64_horiz_buf, FinalOut);
  
  FinalOut.device_sync();
  FinalOut.copy_to_host();
  
  for (int i = 0; i < height/64; i++) {
    for (int j = 0; j < width/128; j++) {
      for (int k = 0; k < 64; k++) {
        for (int l = 0; l < 64; l++) {
          auto new_x = j*64 + l;
          auto new_y = i*64 + k;
          Out_re[new_y*width/2 + new_x] = FinalOut(l, k, 0, j, i);
          Out_im[new_y*width/2 + new_x] = FinalOut(l, k, 1, j, i);
        }
      }
    }
  }
}

int main() {
  int32_t srcWidth  = IMAGE_W;
  int32_t srcHeight = IMAGE_H;
  int32_t srcStride = IMAGE_W;
  
  int32_t dstWidth  = srcWidth;
  int32_t dstHeight = srcHeight;

  int32_t srcBytes  = SRC_BIT_DEPTH >> 3;
  int32_t dstBytes  = DST_BIT_DEPTH >> 3;

  int32_t *gsrc = (int32_t *)malloc(srcStride * srcHeight * srcBytes);
  uint8_t *gmem = (uint8_t *)malloc(srcStride * srcHeight * 1);
  
  if (!gsrc || !gmem) {
    fprintf(stderr, "Unable to allocate memory\n");
    exit(0);
  }

  FILE *fpIn = fopen("lady_with_cap.yuv", "rb");
  if (fpIn == NULL) {
    printf("Error opening input file: %s\n", "lady_with_cap.yuv");
    exit(0);
  }

  for (int indy = 0; indy < srcHeight; indy++) {
    uint8_t *ptr;
    ptr = gmem + indy * srcStride;
    fread(ptr, 1, srcWidth, fpIn);
  }
  fclose(fpIn);

  for (int indy = 0; indy < (srcHeight * srcStride); indy++) {
    gsrc[indy] = gmem[indy] * (1 << 23);
  }

  int32_t n = 64;
  int16_t W64[16 * 6 * 2];
  int32_t Out_idx[64];
 
  init_radix4Int_64(n, W64, Out_idx);
  
  int32_t *ref_Out_re = (int32_t *)
                        malloc(sizeof(int32_t) * dstHeight * dstWidth/2);
  int32_t *ref_Out_im = (int32_t *)
                        malloc(sizeof(int32_t) * dstHeight * dstWidth/2);
  
  run_reference(gsrc, srcStride, ref_Out_re, ref_Out_im, srcWidth, 
                srcHeight, W64, Out_idx);
  
  printf("==============================================================\n\n");
  
  int32_t *Out_re = (int32_t *)malloc(sizeof(int32_t) * dstHeight * dstWidth/2);
  int32_t *Out_im = (int32_t *)malloc(sizeof(int32_t) * dstHeight * dstWidth/2);
  
  run_device(gsrc, W64, Out_re, Out_im, srcWidth, srcHeight);
  
  if (!memcmp(Out_re, ref_Out_re, dstWidth/2 * dstHeight * dstBytes) && 
      !memcmp(Out_im, ref_Out_im, dstWidth/2 * dstHeight * dstBytes)) {
    printf("COMPARE_TEST_PASS \n");
  } else {
    printf("COMPARE_TEST_FAIL \n");
  }

  return 0;
}
