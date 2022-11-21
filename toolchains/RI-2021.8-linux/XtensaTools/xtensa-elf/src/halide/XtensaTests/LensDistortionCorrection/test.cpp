#include <assert.h>
#include <fstream>
#include <iostream>
#include <math.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/times.h>
#include <time.h>
#include <iostream>
#include "LensDistortionCorrection_ref/LDC_parameters.h"
#include "LDC_Coef.h"
#include "xi_api_ref.h"
#include "halide_benchmark.h"
#include "HalideBuffer.h"

using namespace Halide::Runtime;
using namespace Halide::Tools;

/* Input/Output dimensions */
#define IMAGE_W       512
#define IMAGE_H       512
#define SRC_BIT_DEPTH 8
#define DST_BIT_DEPTH 8
#define TILE_W        128
#define TILE_H        64
#define OUTPUT_TILE_W RECT_SIZE
#define OUTPUT_TILE_H RECT_SIZE
#define W_EXT         0
#define H_EXT         0
#define IMAGE_PAD     0

void ref_image_transform_420sp(short *input_x, short *input_y,
                               const xi_pTile srcY, const xi_pTile srcUV,
                               xi_pTile dstY, xi_pTile dstUV, int bbx, int bby);

void run_reference(uint8_t *SrcY, uint8_t *SrcUV, uint32_t SrcPitch,
                   uint8_t *DstY, uint8_t *DstUV, const uint32_t DstPitch,
                   int32_t width, int32_t height, int32_t dstWidth,
                   int32_t dstHeight, uint16_t *InputX, uint16_t *InputY,
                   uint16_t *InputBB) {
  xi_tile srcY_t, srcUV_t;
  xi_tile dstY_t, dstUV_t;

  int16_t inputxt[4], inputyt[4];
  uint16_t *inputBB = InputBB;

  for (int32_t indy = 0; indy < height; indy += OUTPUT_TILE_H) {
    for (int32_t indx = 0; indx < width; indx += OUTPUT_TILE_W) {
      memcpy(inputxt, InputX, 4 * sizeof(short));
      InputX += 4;
      memcpy(inputyt, InputY, 4 * sizeof(short));
      InputY += 4;

      int32_t tlx, tly, inTileW, inTileH;
      tlx = inputBB[0];
      tly = inputBB[1];
      inTileW = inputBB[2];
      inTileH = inputBB[3];
      inputBB += 4;

      uint8_t *src_y, *src_uv, *dst_y, *dst_uv;
      src_y = SrcY + (tlx + tly * SrcPitch);
      src_uv = SrcUV + (tlx + (tly / 2) * SrcPitch);
      dst_y = DstY + (indx + indy * SrcPitch);
      dst_uv = DstUV + (indx + (indy / 2) * SrcPitch);

      // SRC
      XI_TILE_SET_BUFF_PTR((&srcY_t), src_y);
      XI_TILE_SET_DATA_PTR((&srcY_t), ((uint8_t *)src_y) +
                                          SrcPitch * IMAGE_PAD + IMAGE_PAD);
      XI_TILE_SET_BUFF_SIZE((&srcY_t), SrcPitch * (inTileH + 2 * IMAGE_PAD));
      XI_TILE_SET_WIDTH((&srcY_t), inTileW + 2 * IMAGE_PAD);
      XI_TILE_SET_HEIGHT((&srcY_t), inTileH + 2 * IMAGE_PAD);
      XI_TILE_SET_PITCH((&srcY_t), SrcPitch + 2 * IMAGE_PAD);
      XI_TILE_SET_TYPE((&srcY_t), XI_TILE_U8);
      XI_TILE_SET_EDGE_WIDTH((&srcY_t), IMAGE_PAD);
      XI_TILE_SET_EDGE_HEIGHT((&srcY_t), IMAGE_PAD);
      XI_TILE_SET_X_COORD((&srcY_t), 0);
      XI_TILE_SET_Y_COORD((&srcY_t), 0);

      XI_TILE_SET_BUFF_PTR((&srcUV_t), src_uv);
      XI_TILE_SET_DATA_PTR((&srcUV_t), ((uint8_t *)src_uv) +
                                           SrcPitch * IMAGE_PAD + IMAGE_PAD);
      XI_TILE_SET_BUFF_SIZE((&srcUV_t),
                            (SrcPitch * (inTileH + 2 * IMAGE_PAD)) / 2);
      XI_TILE_SET_WIDTH((&srcUV_t), inTileW + 2 * IMAGE_PAD);
      XI_TILE_SET_HEIGHT((&srcUV_t), (inTileH + 2 * IMAGE_PAD) / 2);
      XI_TILE_SET_PITCH((&srcUV_t), SrcPitch + 2 * IMAGE_PAD);
      XI_TILE_SET_TYPE((&srcUV_t), XI_TILE_U8);
      XI_TILE_SET_EDGE_WIDTH((&srcUV_t), IMAGE_PAD);
      XI_TILE_SET_EDGE_HEIGHT((&srcUV_t), IMAGE_PAD);
      XI_TILE_SET_X_COORD((&srcUV_t), 0);
      XI_TILE_SET_Y_COORD((&srcUV_t), 0);

      // DST_DX
      XI_TILE_SET_BUFF_PTR((&dstY_t), dst_y);
      XI_TILE_SET_DATA_PTR((&dstY_t), dst_y);
      XI_TILE_SET_BUFF_SIZE((&dstY_t), (DstPitch * (OUTPUT_TILE_H)));
      XI_TILE_SET_WIDTH((&dstY_t), OUTPUT_TILE_W);
      XI_TILE_SET_HEIGHT((&dstY_t), OUTPUT_TILE_H);
      XI_TILE_SET_PITCH((&dstY_t), (DstPitch));
      XI_TILE_SET_TYPE((&dstY_t), XI_TILE_U8);
      XI_TILE_SET_EDGE_WIDTH((&dstY_t), 0);
      XI_TILE_SET_EDGE_HEIGHT((&dstY_t), 0);
      XI_TILE_SET_X_COORD((&dstY_t), 0);
      XI_TILE_SET_Y_COORD((&dstY_t), 0);

      XI_TILE_SET_BUFF_PTR((&dstUV_t), dst_uv);
      XI_TILE_SET_DATA_PTR((&dstUV_t), dst_uv);
      XI_TILE_SET_BUFF_SIZE((&dstUV_t), (DstPitch * (OUTPUT_TILE_H)) / 2);
      XI_TILE_SET_WIDTH((&dstUV_t), OUTPUT_TILE_W);
      XI_TILE_SET_HEIGHT((&dstUV_t), OUTPUT_TILE_H / 2);
      XI_TILE_SET_PITCH((&dstUV_t), (DstPitch));
      XI_TILE_SET_TYPE((&dstUV_t), XI_TILE_U8);
      XI_TILE_SET_EDGE_WIDTH((&dstUV_t), 0);
      XI_TILE_SET_EDGE_HEIGHT((&dstUV_t), 0);
      XI_TILE_SET_X_COORD((&dstUV_t), 0);
      XI_TILE_SET_Y_COORD((&dstUV_t), 0);
      // printf("\n\nTILE_ %dx%d\n",indx/32, indy/32);
      ref_image_transform_420sp(inputxt, inputyt, &srcY_t, &srcUV_t, &dstY_t,
                                &dstUV_t, tlx, tly);
    }
  }
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
      printf("%d %x %x\n", i, tmpOut[i], tmpRef[i]);
      return -1;
    }
  }
  return 0;
}

#include "LDC.h"

int LDC_call(Buffer<uint8_t> in_srcY,
             Buffer<uint8_t> inUV,
             Buffer<uint16_t> inX,
             Buffer<uint16_t> inY,
             Buffer<uint16_t> inBB,
             Buffer<uint8_t> &outY)
{
  LDC(in_srcY, inUV, inX, inY, inBB, outY);
  outY.device_sync();
  outY.copy_to_host();
  return 1;
}

int main(int argc, char **argv) {
  // Grey-scale frame
  uint8_t *gsrcY;
  uint8_t *gsrcUV;
  uint8_t *gdstY;
  uint8_t *gdstUV;
  uint8_t *gdstY_ref;
  uint8_t *gdstUV_ref;

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

  // Grey-scale I/O frame buffers
  gsrcY = (uint8_t *)malloc(srcStride * (srcHeight + IMAGE_PAD + IMAGE_PAD) *
                            srcBytes);
  gsrcUV = (uint8_t *)malloc((srcStride * (srcHeight + IMAGE_PAD + IMAGE_PAD) *
                              srcBytes) / 2);
  gdstY = (uint8_t *)malloc(dstWidth * dstHeight * dstBytes);
  gdstUV = (uint8_t *)malloc((dstWidth * dstHeight * dstBytes) / 2);
  gdstY_ref = (uint8_t *)malloc(dstWidth * dstHeight * dstBytes);
  gdstUV_ref = (uint8_t *)malloc((dstWidth * dstHeight * dstBytes) / 2);
  if (!gsrcY || !gdstY || !gdstY_ref || !gsrcUV || !gdstUV || !gdstUV_ref) {
    fprintf(stderr, "Unable to allocate memory\n");
    exit(0);
  }

// Read input
  int indy;
  char fname[256]; // input image name
  // For image FILE IO
  FILE *fpIn;
  // Read input
  sprintf(fname, "building.raw");
  fpIn = fopen(fname, "rb");
  if (fpIn == NULL) {
    printf("Error opening input file: %s\n", fname);
    exit(0);
  }

  for (indy = IMAGE_PAD; indy < (srcHeight + IMAGE_PAD); indy++) {
    uint8_t *ptr;
    ptr = gsrcY + (indy * srcStride) + IMAGE_PAD;
    fread(ptr, 1, srcWidth, fpIn);
  }
  fclose(fpIn);

  fpIn = fopen(fname, "rb");
  if (fpIn == NULL) {
    printf("Error opening input file: %s\n", fname);
    exit(0);
  }

  for (indy = IMAGE_PAD; indy < (srcHeight + IMAGE_PAD) / 2; indy++) {
    uint8_t *ptr;
    ptr = gsrcUV + (indy * srcStride) + IMAGE_PAD;
    fread(ptr, 1, srcWidth, fpIn);
  }
  fclose(fpIn);

  // Read Src_y
  Buffer<uint8_t> input_srcY(IMAGE_W, IMAGE_H);
  for (int y = 0; y < input_srcY.height(); y++) {
    for (int x = 0; x < input_srcY.width(); x++)
        input_srcY(x, y) = gsrcY[y*input_srcY.width() + x];
  }

  // Read Src_UV 
  Buffer<uint8_t> inputUV(IMAGE_W, IMAGE_H/2);
  for (int y = 0; y < inputUV.height(); y++) {
    for (int x = 0; x < inputUV.width(); x++)
        inputUV(x, y) = gsrcUV[y*inputUV.width() + x];
  }
  
  int Tile_w = 16;
  int Tile_h = 16;
  int Tile_width  = 32;
  int Tile_height = 32; 

  // Read InputX
  Buffer<uint16_t> inputX(4, Tile_w, Tile_h);
  for (int y = 0; y < Tile_h; y++) {
    for (int x = 0; x < Tile_w; x++) {
      inputX(0, x, y) = InputX[(y * Tile_w + x) * 4];
      inputX(1, x, y) = InputX[(y * Tile_w + x) * 4 + 1];
      inputX(2, x, y) = InputX[(y * Tile_w + x) * 4 + 2];
      inputX(3, x, y) = InputX[(y * Tile_w + x) * 4 + 3];
    }
  }

  // Read InputY
  Buffer<uint16_t> inputY(4, Tile_w, Tile_h);
  for (int y = 0; y < Tile_h; y++) {
    for (int x = 0; x < Tile_w; x++) {
      inputY(0, x, y) = InputY[(y * Tile_w + x) * 4];
      inputY(1, x, y) = InputY[(y * Tile_w + x) * 4 + 1];
      inputY(2, x, y) = InputY[(y * Tile_w + x) * 4 + 2];
      inputY(3, x, y) = InputY[(y * Tile_w + x) * 4 + 3];
    }
  }

  // Read InputBB
  Buffer<uint16_t> inputBB(Tile_w, Tile_h, 4);
  for (int y = 0; y < Tile_h; y++) {
    for (int x = 0; x < Tile_w; x++) {
      inputBB(x, y, 0) = InputBB[(y * Tile_w + x) * 4];
      inputBB(x, y, 1) = InputBB[(y * Tile_w + x) * 4 + 1];
      inputBB(x, y, 2) = InputBB[(y * Tile_w + x) * 4 + 2];
      inputBB(x, y, 3) = InputBB[(y * Tile_w + x) * 4 + 3];
    }
  }


  //**********   Dst_y ************
 
  Buffer<uint8_t> output(Tile_width, Tile_height, Tile_w, Tile_h, 2);

  LDC_call(input_srcY, inputUV, inputX, inputY, inputBB, output);
  // printf("\n\nBASELINE\n\n\n");
  run_reference(gsrcY, gsrcUV, srcStride, gdstY_ref, gdstUV_ref, dstWidth,
                srcWidth, srcHeight, dstWidth, dstHeight, InputX, InputY,
                InputBB);
  // Write Dst_y
  for (int y = 0; y < Tile_h; y++) {             // Tile in Y axis
    for (int x = 0; x < Tile_w; x++) {           // Tile in X axis
      for (int h = 0; h < Tile_height; h++) {  // Tile_height
        for (int w = 0; w < Tile_width; w++) { // Tile width
          gdstY[(y * IMAGE_W * Tile_height) +
                (x * Tile_width) +  
                (h *   IMAGE_W ) +
                w] = output(w, h, x, y, 0);
        } 
      }
    }
  }

  // Write Dst_UV
  for (int y = 0; y < Tile_h; y++) {              // Tile in Y axis
    for (int x = 0; x < Tile_w; x++) {            // Tile in X axis
      for (int h = 0; h < (Tile_height/2); h++) { // Tile_height
        for (int w = 0; w < Tile_width; w++) {    // Tile width
          gdstUV[(y * IMAGE_W * (Tile_height/2)) +
                                (x * Tile_width) +  
                                (h *   IMAGE_W ) +
                                               w] = output(w, h, x, y, 1);
        }
      }
    }
  }

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

  sprintf(fname, "OutputUV.yuv");
  fpOut = fopen(fname, "wb");
  if (fpOut == NULL) {
    printf("Error opening output file: %s\n", fname);
    exit(0);
  }

  fwrite(gdstUV, dstWidth * dstHeight / 2, 1, fpOut);
  fclose(fpOut);

  printf("Compare: ");
  if (memcmp(gdstY_ref, gdstY, dstWidth * dstHeight) == 0 &&
      memcmp(gdstUV_ref, gdstUV, dstWidth * dstHeight / 2) == 0) {
    printf("COMPARE_TEST_PASS \n");
  } else {
    printf("COMPARE_TEST_FAIL \n");
  }

  return 0;
}
