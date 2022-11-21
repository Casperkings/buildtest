#include <assert.h>
#include <fstream>
#include <iostream>
#include <math.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/times.h>
#include <time.h>
#include <iostream>
#include "xi_api_ref.h"
#include "halide_benchmark.h"
#include "HalideBuffer.h"

#define IMAGE_W       512
#define IMAGE_H       512
#define SRC_BIT_DEPTH 8
#define DST_BIT_DEPTH 8
#define TILE_W        128
#define TILE_H        64
#define W_EXT         1
#define H_EXT         1
#define IMAGE_PAD     1
#define Level         8

#define DEBUG_DUMP    0

using namespace Halide::Runtime;
using namespace Halide::Tools;

extern short remap_table[8 * 256];

XI_ERR_TYPE xiLocalLaplacian_Gray_U8_FixPt_ref(const xi_pTile src, xi_pTile dst,
                                               int levels, int pyramid_levels,
                                               int16_t *remap_table);

void run_reference(uint8_t *Src, uint32_t SrcPitch, uint8_t *Dst,
                   const uint32_t DstPitch, int32_t width, int32_t height,
                   int32_t dstWidth, int32_t dstHeight) {
  xi_tile src_t;
  xi_tile dst_t;

  // SRC
  XI_TILE_SET_BUFF_PTR((&src_t), Src);
  XI_TILE_SET_DATA_PTR((&src_t),
                       ((uint8_t *)Src) + SrcPitch * IMAGE_PAD + IMAGE_PAD);
  XI_TILE_SET_BUFF_SIZE((&src_t), SrcPitch * (height + 2 * IMAGE_PAD));
  XI_TILE_SET_WIDTH((&src_t), width);
  XI_TILE_SET_HEIGHT((&src_t), height);
  XI_TILE_SET_PITCH((&src_t), SrcPitch);
  XI_TILE_SET_TYPE((&src_t), XI_TILE_U8);
  XI_TILE_SET_EDGE_WIDTH((&src_t), IMAGE_PAD);
  XI_TILE_SET_EDGE_HEIGHT((&src_t), IMAGE_PAD);
  XI_TILE_SET_X_COORD((&src_t), 0);
  XI_TILE_SET_Y_COORD((&src_t), 0);

  // DST_DX
  XI_TILE_SET_BUFF_PTR((&dst_t), Dst);
  XI_TILE_SET_DATA_PTR((&dst_t), Dst);
  XI_TILE_SET_BUFF_SIZE((&dst_t), (DstPitch * (dstHeight)));
  XI_TILE_SET_WIDTH((&dst_t), dstWidth);
  XI_TILE_SET_HEIGHT((&dst_t), dstHeight);
  XI_TILE_SET_PITCH((&dst_t), (DstPitch));
  XI_TILE_SET_TYPE((&dst_t), XI_TILE_U8);
  XI_TILE_SET_EDGE_WIDTH((&dst_t), 0);
  XI_TILE_SET_EDGE_HEIGHT((&dst_t), 0);
  XI_TILE_SET_X_COORD((&dst_t), 0);
  XI_TILE_SET_Y_COORD((&dst_t), 0);

  xiLocalLaplacian_Gray_U8_FixPt_ref(&src_t, &dst_t, 8, 3, &remap_table[0]);
}

int compare_ref(const char *fname, int size, uint8_t *tmpOut)
{
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
      printf("%dx%d %x %x\n", i%512,i/512, tmpOut[i], tmpRef[i]);
      return -1;
    }
  }
  return 0;
}

#include "InputGaussian.h"
#include "ContrastLevel.h"
#include "GaussianPyr.h"
#include "LaplacianPyr.h"
#include "FuseLaplacianPyr.h"
#include "FusedPyrUpSample.h"
#include "FusedPyrUpSampleClip.h"

int Local_Laplacian_call(Buffer<uint8_t> in, Buffer<uint8_t> &out) {
#if DEBUG_DUMP
  char fname[256];
#endif
  Buffer<uint8_t> src_1(IMAGE_W/2, IMAGE_H/2);
  Buffer<uint8_t> src_2(IMAGE_W/4, IMAGE_H/4);
  Buffer<int16_t> GPYr_0(IMAGE_W, IMAGE_H,Level);
  Buffer<int16_t> remap_tab(Level*256, 1);
  Buffer<int16_t> GPYr_1(IMAGE_W/2, IMAGE_H/2,Level);
  Buffer<int16_t> GPYr_2(IMAGE_W/4, IMAGE_H/4,Level);
  Buffer<int16_t> LPYr_1(IMAGE_W/2, IMAGE_H/2, Level);
  Buffer<int16_t> Dst_1(IMAGE_W/2, IMAGE_H/2);
  Buffer<int16_t> L_0(IMAGE_W, IMAGE_H);
  Buffer<int16_t> L_1(IMAGE_W/2, IMAGE_H/2);
  Buffer<int16_t> L_2(IMAGE_W/4, IMAGE_H/4);
  Buffer<int16_t> LPYr_0(IMAGE_W, IMAGE_H, Level);

  // level_1
  InputGaussian(in, src_1);
  src_1.device_sync();
  src_1.copy_to_host();

  #if DEBUG_DUMP
  {
    uint8_t *out_src_1;
    out_src_1 = (uint8_t *)malloc((IMAGE_W/2) * (IMAGE_H/2) * sizeof(char));  
    for (int y = 0; y < src_1.height(); y++) {
      for (int x = 0; x < src_1.width(); x++) 
        out_src_1[y*src_1.width() + x] = src_1(x, y);
    }
    FILE *fpOut;
    // Write result
    sprintf(fname, "halide_src_1.yuv");
    fpOut = fopen(fname, "wb");
    if (fpOut == NULL) {
      printf("Error opening output file: %s\n", fname);
      exit(0);
    }

    fwrite(out_src_1, src_1.height() * src_1.width(), 1, fpOut);
    fclose(fpOut);
  }
  #endif

  // level_2
  InputGaussian(src_1, src_2);
  src_2.device_sync();
  src_2.copy_to_host();
  #if DEBUG_DUMP
  {
    uint8_t *out_src_2;
    out_src_2 = (uint8_t *)malloc((IMAGE_W/4) * (IMAGE_H/4) * sizeof(uint8_t)); 
    for (int y = 0; y < src_2.height(); y++) {
      for (int x = 0; x < src_2.width(); x++) 
        out_src_2[y*src_2.width() + x] = src_2(x, y);
    }
    FILE *fpOut;
    //Write result
    sprintf(fname, "halide_src_2.yuv");
    fpOut = fopen(fname, "wb");
    if (fpOut == NULL) {
      printf("Error opening output file: %s\n", fname);
      exit(0);
    }

    fwrite(out_src_2, src_2.height() * src_2.width(), 1, fpOut);
    fclose(fpOut);
  }
  #endif
  
  // level_3
  int y = 0;
  for (int x = 0; x < Level * 256; x++)
    remap_tab(x, y) = remap_table[x];
  ContrastLevel(in, remap_tab, GPYr_0);
  GPYr_0.device_sync();
  GPYr_0.copy_to_host();

  #if DEBUG_DUMP
  {
    int16_t *out_GPYr;
    out_GPYr = (int16_t *)malloc((IMAGE_W) * (IMAGE_H) * sizeof(int16_t));
    for (int level = 0; level < Level; level++) {  
      for (int y = 0; y < IMAGE_H; y++) {
        for (int x = 0; x < IMAGE_W; x++) {
          out_GPYr[y*IMAGE_W + x] = GPYr_0(x, y, level);
        }
      }
      FILE *fpOut;
      // Write result
      sprintf(fname, "halide_GPyrLevel0_%d.yuv", level);
      fpOut = fopen(fname, "wb");
      if (fpOut == NULL) {
        printf("Error opening output file: %s\n", fname);
        exit(0);
      }

      fwrite(out_GPYr, IMAGE_W * IMAGE_H * 2, 1, fpOut);
      fclose(fpOut);
    }
  }
  #endif

  // level_4
  GaussianPyr(GPYr_0, GPYr_1);
  GPYr_1.device_sync();
  GPYr_1.copy_to_host();

  #if DEBUG_DUMP
  {
    int16_t *out_GPYr_1;
    out_GPYr_1 = (int16_t *)malloc((IMAGE_W/2) * (IMAGE_H/2) * sizeof(int16_t));
    for (int level = 0; level < Level; level++) {  
      for (int y = 0; y < (IMAGE_H/2); y++) {
        for (int x = 0; x < (IMAGE_W/2); x++) {
          out_GPYr_1[y*(IMAGE_W/2) + x] = GPYr_1(x, y, level);
        }
      }
      FILE *fpOut;
      // Write result
      sprintf(fname, "halide_GPyrLevel1_%d.yuv", level);
      fpOut = fopen(fname, "wb");
      if (fpOut == NULL) {
        printf("Error opening output file: %s\n", fname);
        exit(0);
      }

      fwrite(out_GPYr_1, (IMAGE_W/2) * (IMAGE_H/2) * 2, 1, fpOut);
      fclose(fpOut);
    }
  }
  #endif


  // level_5
  GaussianPyr(GPYr_1, GPYr_2);
  GPYr_2.device_sync();
  GPYr_2.copy_to_host();

  #if DEBUG_DUMP
  {
    int16_t *out_GPYr_2;
    out_GPYr_2 = (int16_t *)malloc((IMAGE_W/4) * (IMAGE_H/4) * sizeof(int16_t));
    for (int level = 0; level < Level; level++) {  
      for (int y = 0; y < (IMAGE_H/4); y++) {
        for (int x = 0; x < (IMAGE_W/4); x++) {
          out_GPYr_2[y*(IMAGE_W/4) + x] = GPYr_2(x, y, level);
        }
      }
      FILE *fpOut;
      // Write result
      sprintf(fname, "halide_GPyrLevel2_%d.yuv", level);
      fpOut = fopen(fname, "wb");
      if (fpOut == NULL) {
        printf("Error opening output file: %s\n", fname);
        exit(0);
      }

      fwrite(out_GPYr_2, (IMAGE_W/4) * (IMAGE_H/4) * 2, 1, fpOut);
      fclose(fpOut);
    }
  }
  #endif

  // level_6
  LaplacianPyr(GPYr_1, GPYr_2, LPYr_1);
  LPYr_1.device_sync();
  LPYr_1.copy_to_host();

  #if DEBUG_DUMP
  {
    int16_t *out_LPYr_1;
    out_LPYr_1 = (int16_t *)malloc((IMAGE_W/2) * (IMAGE_H/2) * sizeof(int16_t));
    for (int level = 0; level < Level; level++ ){  
      for (int y = 0; y < (IMAGE_H/2); y++) {
        for (int x = 0; x < (IMAGE_W/2); x++) {
          out_LPYr_1[y*(IMAGE_W/2) + x] = LPYr_1(x, y, level);
        }
      }
      FILE *fpOut;
      // Write result
      sprintf(fname, "halide_LPyrLevel1_%d.yuv", level);
      fpOut = fopen(fname, "wb");
      if (fpOut == NULL) {
        printf("Error opening output file: %s\n", fname);
        exit(0);
      }

      fwrite(out_LPYr_1, (IMAGE_W/2) * (IMAGE_H/2) * 2, 1, fpOut);
      fclose(fpOut);
    }
  }
  #endif

  // level_7
  LaplacianPyr(GPYr_0, GPYr_1, LPYr_0);
  LPYr_0.device_sync();
  LPYr_0.copy_to_host();

  #if DEBUG_DUMP
  {
    int16_t *out_LPYr_0;
    out_LPYr_0 = (int16_t *)malloc((IMAGE_W) * (IMAGE_H) * sizeof(int16_t));
    for (int level = 0; level < Level; level++) {  
      for (int y = 0; y < (IMAGE_H); y++) {
        for (int x = 0; x < (IMAGE_W); x++) {
          out_LPYr_0[y*(IMAGE_W) + x] = LPYr_0(x, y, level);
        }
      }
      FILE *fpOut;
      // Write result
      sprintf(fname, "halide_LPyrLevel0_%d.yuv", level);
      fpOut = fopen(fname, "wb");
      if (fpOut == NULL) {
        printf("Error opening output file: %s\n", fname);
        exit(0);
      }

      fwrite(out_LPYr_0, (IMAGE_W) * (IMAGE_H) * 2, 1, fpOut);
      fclose(fpOut);
      }
  }
  #endif

  // level_8
  FuseLaplacianPyr(GPYr_2, src_2, L_2);
  L_2.device_sync();
  L_2.copy_to_host();

  #if DEBUG_DUMP
  {
    int16_t *out_L_2;
    out_L_2 = (int16_t *)malloc((IMAGE_W/4) * (IMAGE_H/4) * sizeof(int16_t)); 
    for (int y = 0; y < (IMAGE_H/4); y++) {
      for (int x = 0; x < (IMAGE_W/4); x++) {
        out_L_2[y*(IMAGE_W/4) + x] = L_2(x, y);
      }
    }
    FILE *fpOut;
    // Write result
    sprintf(fname, "halide_Lap2Dlevel2.raw");
    fpOut = fopen(fname, "wb");
    if (fpOut == NULL) {
      printf("Error opening output file: %s\n", fname);
      exit(0);
    }

    fwrite(out_L_2, (IMAGE_W/4) * (IMAGE_H/4) * 2, 1, fpOut);
    fclose(fpOut);
  }
  #endif
  
  // level_9
  FuseLaplacianPyr(LPYr_1, src_1, L_1);
  L_1.device_sync();
  L_1.copy_to_host();

  #if DEBUG_DUMP
  {
    int16_t *out_L_1;
    out_L_1 = (int16_t *)malloc((IMAGE_W/2) * (IMAGE_H/2) * sizeof(int16_t)); 
    for (int y = 0; y < (IMAGE_H/2); y++) {
      for (int x = 0; x < (IMAGE_W/2); x++) {
        out_L_1[y*(IMAGE_W/2) + x] = L_1(x, y);
      }
    }
    FILE *fpOut;
    // Write result
    sprintf(fname, "halide_Lap2Dlevel1.raw");
    fpOut = fopen(fname, "wb");
    if (fpOut == NULL) {
      printf("Error opening output file: %s\n", fname);
      exit(0);
    }

    fwrite(out_L_1, (IMAGE_W/2) * (IMAGE_H/2) * 2, 1, fpOut);
    fclose(fpOut);
  }
  #endif

  // level_10
  FuseLaplacianPyr(LPYr_0, in, L_0);
  L_0.device_sync();
  L_0.copy_to_host();

  #if DEBUG_DUMP
  {
    int16_t *out_L_0;
    out_L_0 = (int16_t *)malloc((IMAGE_W) * (IMAGE_H) * sizeof(int16_t)); 
    for (int y = 0; y < (IMAGE_H); y++) {
      for (int x = 0; x < (IMAGE_W); x++) {
        out_L_0[y*(IMAGE_W) + x] = L_0(x, y);
      }
    }
    FILE *fpOut;
    // Write result
    sprintf(fname, "halide_Lap2Dlevel0.raw");
    fpOut = fopen(fname, "wb");
    if (fpOut == NULL) {
      printf("Error opening output file: %s\n", fname);
      exit(0);
    }

    fwrite(out_L_0, (IMAGE_W) * (IMAGE_H) * 2, 1, fpOut);
    fclose(fpOut);
  }
  #endif


  // level_11
  FusedPyrUpSample(L_1, L_2, Dst_1);
  Dst_1.device_sync();
  Dst_1.copy_to_host();

  #if DEBUG_DUMP
  {
    int16_t *out_Dst_1;
    out_Dst_1 = (int16_t *)malloc((IMAGE_W/2) * (IMAGE_H/2) * sizeof(int16_t)); 
    for (int y = 0; y < (IMAGE_H/2); y++) {
      for (int x = 0; x < (IMAGE_W/2); x++) {
        out_Dst_1[y*(IMAGE_W/2) + x] = Dst_1(x, y);
        // printf(" %d  ", GPYr_0(level, x, y));
      }
    }
    FILE *fpOut;
    // Write result
    sprintf(fname, "halide_Dst_1.raw");
    fpOut = fopen(fname, "wb");
    if (fpOut == NULL) {
      printf("Error opening output file: %s\n", fname);
      exit(0);
    }

    fwrite(out_Dst_1, (IMAGE_W/2) * (IMAGE_H/2) * 2, 1, fpOut);
    fclose(fpOut);
  }
  #endif

  // level_12
  FusedPyrUpSampleClip(L_0, Dst_1, out);
  out.device_sync();
  out.copy_to_host();

  return 0;
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

  gsrc = (uint8_t*)malloc(IMAGE_W * IMAGE_H * srcBytes);
  gdst = (uint8_t*)malloc(dstWidth * dstHeight * dstBytes);
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

  for (indy = 0 ; indy < IMAGE_H ; indy++) {
    uint8_t *ptr;
    ptr = gsrc + (indy * IMAGE_H);
    fread(ptr, 1, srcWidth, fpIn);
  }
  fclose(fpIn);

  // Copy input into Buffer
  Buffer<uint8_t> input(IMAGE_W, IMAGE_H);
  for (int y = 0; y < input.height(); y++) {
    for (int x = 0; x < input.width(); x++)
      input(x, y) = gsrc[y*input.width() + x];
  }
  
  Buffer<uint8_t> output(IMAGE_W, IMAGE_H);
  
  Local_Laplacian_call(input, output);
  
  // copy the output to the memory from halide buffer
  for (int y = 0; y < output.height(); y++) {
    for (int x = 0; x < output.width(); x++) 
      gdst[y*output.width() + x] = output(x, y);
  }
  
  // write Halide Output
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
  
  // Read input to run reference code 
  free(gsrc);
  gsrc = (uint8_t *)malloc(srcStride * (srcHeight + IMAGE_PAD + IMAGE_PAD) *
                           srcBytes);
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
  int padding_type = 1;
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

  // run reference code
  run_reference(gsrc, srcStride, gdst_ref, dstWidth, srcWidth, srcHeight,
                dstWidth, dstHeight);

  sprintf(fname, "Output.yuv");
  if ((compare_ref(fname, dstWidth * dstHeight, gdst_ref) == 0) &&
      (memcmp(gdst_ref, gdst, dstWidth * dstHeight) == 0))
    printf("COMPARE_TEST_PASS\n");
  else 
    printf("COMPARE_TEST_FAIL\n");
    
  free(gsrc);
  free(gdst);
}
