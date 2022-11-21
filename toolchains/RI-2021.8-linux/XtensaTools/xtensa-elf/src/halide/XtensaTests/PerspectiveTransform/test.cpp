#include <iostream>
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
#define TILE_H        128
#define OUTPUT_TILE_W 128
#define OUTPUT_TILE_H 64
#define W_EXT         0
#define H_EXT         0
#define IMAGE_PAD     0

#define MIN2(a, b)       ((a) < (b) ? (a) : (b))
#define MIN4(a, b, c, d) (MIN2(MIN2(a, b), MIN2(c, d)))
#define MAX2(a, b)       ((a) > (b) ? (a) : (b))
#define MAX4(a, b, c, d) (MAX2(MAX2(a, b), MAX2(c, d)))

#define TO_Q13_18(val)   ((int)((val) * (1 << 18) + 0.5))

void run_reference(uint8_t *SrcY, uint32_t SrcPitch, uint8_t *DstY,
                   const uint32_t DstPitch, int32_t width, int32_t height,
                   int32_t dstWidth, int32_t dstHeight, uint32_t *perspective,
                   uint16_t *InputBB) {
  xi_tile srcY_t;
  xi_tile dstY_t;
  xi_perspective_fpt *pperspective = (xi_perspective_fpt *)perspective;
  XI_Q13_18 a11 = pperspective->a11;
  XI_Q13_18 a12 = pperspective->a12;
  XI_Q13_18 a13 = pperspective->a13;
  XI_Q13_18 a21 = pperspective->a21;
  XI_Q13_18 a22 = pperspective->a22;
  XI_Q13_18 a23 = pperspective->a23;
  XI_Q13_18 a31 = pperspective->a31;
  XI_Q13_18 a32 = pperspective->a32;
  XI_Q13_18 a33 = pperspective->a33;

  uint16_t *inputBB = InputBB;

  for (int32_t indy = 0; indy < height; indy += OUTPUT_TILE_H) {
    for (int32_t indx = 0; indx < width; indx += OUTPUT_TILE_W) {
      int32_t inTileW, inTileH;
      int32_t InputBBx[4], InputBBy[4];
      int32_t xoffsets[4] = {0, OUTPUT_TILE_W, 0, OUTPUT_TILE_W};
      int32_t yoffsets[4] = {0, 0, OUTPUT_TILE_H, OUTPUT_TILE_H};
      for (int inx = 0; inx < 4; inx++) {
        int32_t xsrc =
	        (int32_t)((int64_t)a11 * (indx + xoffsets[inx]) +
		                (int64_t)a12 * (indy + yoffsets[inx]) + (int64_t)a13);
        int32_t ysrc =
   	      (int32_t)((int64_t)a21 * (indx + xoffsets[inx]) +
		                (int64_t)a22 * (indy + yoffsets[inx]) + (int64_t)a23);
        int32_t dvsr =
	        (int32_t)((int64_t)a31 * (indx + xoffsets[inx]) +
		                (int64_t)a32 * (indy + yoffsets[inx]) + (int64_t)a33);
        InputBBx[inx] = xsrc / (dvsr);
        InputBBy[inx] = ysrc / (dvsr);
      }
      int32_t startX = MIN4(InputBBx[0], InputBBx[1], 
                            InputBBx[2], InputBBx[3]) - 2;
      int32_t startY = MIN4(InputBBy[0], InputBBy[1], 
                            InputBBy[2], InputBBy[3]) - 2;
      inTileW = MAX4(InputBBx[0], InputBBx[1], 
                     InputBBx[2], InputBBx[3]) - startX + 2;
      inTileH = MAX4(InputBBy[0], InputBBy[1], 
                     InputBBy[2], InputBBy[3]) - startY + 2;

      inputBB[0] = startX;
      inputBB[1] = startY;
      inputBB[2] = inTileW;
      inputBB[3] = inTileH;
      inputBB += 4;
      uint8_t *src_y, *dst_y;
      src_y = SrcY + (startX + startY * SrcPitch);
      dst_y = DstY + (indx + indy * DstPitch);

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
      XI_TILE_SET_X_COORD((&srcY_t), startX);
      XI_TILE_SET_Y_COORD((&srcY_t), startY);

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
      XI_TILE_SET_X_COORD((&dstY_t), indx);
      XI_TILE_SET_Y_COORD((&dstY_t), indy);

      xiWarpPerspective_U8_Q13_18_ref(&srcY_t, &dstY_t,
                                      (xi_perspective_fpt *)perspective);
    }
  }
}

#include "perspective.h"

int TILE_W_= IMAGE_W/OUTPUT_TILE_W;
int TILE_H_= IMAGE_H/OUTPUT_TILE_H;

Buffer<uint8_t> perspective_call(Buffer<uint8_t> Input,
                                 Buffer<uint32_t> Perspective,
                                 Buffer<uint16_t> InputBB )
{
  Buffer<uint8_t> Output (OUTPUT_TILE_W,OUTPUT_TILE_H,TILE_W_,TILE_H_);  
  perspective(Input,Perspective,InputBB,Output);
  Output.device_sync();
  Output.copy_to_host();
  return Output;
}

int main() {
  // Grey-scale frame
  uint8_t *gsrcY;
  uint8_t *gdstY;
  uint8_t *gdstY_ref;

  // Image dimensions and bit depth
  int32_t srcWidth, srcHeight, srcBytes;
  int32_t dstWidth, dstHeight, dstBytes;
  int32_t srcStride;

  // Set frame dimensions
  srcWidth = IMAGE_W;
  srcHeight = IMAGE_H;
  srcStride = (IMAGE_W + IMAGE_PAD + IMAGE_PAD);
  srcBytes = SRC_BIT_DEPTH >> 3;
  dstWidth = srcWidth;
  dstHeight = srcHeight;
  dstBytes = DST_BIT_DEPTH >> 3;

  // Grey-scale I/O frame buffers
  gsrcY = (uint8_t *)malloc(srcStride * (srcHeight + IMAGE_PAD + IMAGE_PAD) *
                            srcBytes);
  gdstY = (uint8_t *)malloc(dstWidth * dstHeight * dstBytes);
  gdstY_ref = (uint8_t *)malloc(dstWidth * dstHeight * dstBytes);
  if (!gsrcY || !gdstY || !gdstY_ref) {
    fprintf(stderr, "Unable to allocate memory\n");
    exit(0);
  }

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
    ptr = gsrcY + (indy * srcStride) + IMAGE_PAD;
    fread(ptr, 1, srcWidth, fpIn);
  }
  fclose(fpIn);

  uint32_t perspective[9] =
    //{TO_Q13_18(0.6), TO_Q13_18(0.6), TO_Q13_18(15.31), TO_Q13_18(-0.6),
    //TO_Q13_18(0.6), TO_Q13_18(70.5), TO_Q13_18(0.0009), TO_Q13_18(0.0003),
    //TO_Q13_18(1.021)};
    {TO_Q13_18(0.9f),    TO_Q13_18(0.05f),   TO_Q13_18(15.0f),
     TO_Q13_18(0.05f),   TO_Q13_18(0.9f),    TO_Q13_18(15.0f),
     TO_Q13_18(0.0001f), TO_Q13_18(0.0001f), TO_Q13_18(1.1f)};

  uint16_t InputBB[4 * 32];

  run_reference(gsrcY, srcStride, gdstY_ref, dstWidth, srcWidth, srcHeight,
                dstWidth, dstHeight, perspective, InputBB);

  Buffer<uint8_t> input(srcWidth,srcHeight);
  Buffer<uint16_t> inputBB(4,TILE_W_,TILE_H_);

  for (int i = 0; i < TILE_H_; i++) {
    for (int j = 0; j < TILE_W_; j++) {
      inputBB(0, j, i) = InputBB[0 + (j*4) + (i*TILE_W_*4)];
      inputBB(1, j, i) = InputBB[1 + (j*4) + (i*TILE_W_*4)];
      inputBB(2, j, i) = InputBB[2 + (j*4) + (i*TILE_W_*4)];
      inputBB(3, j, i) = InputBB[3 + (j*4) + (i*TILE_W_*4)];
    }
  }

  for (int y = 0; y < input.height(); y++) {
    for (int x = 0; x < input.width(); x++) {   
      input(x, y) = gsrcY[(y*input.width()) + x];
    }
  }
  
  Buffer<uint32_t> perspectiveBuffer(9, 1);
  perspectiveBuffer(0, 0) = TO_Q13_18(0.9f);
  perspectiveBuffer(1, 0) = TO_Q13_18(0.05f);
  perspectiveBuffer(2, 0) = TO_Q13_18(15.0f);
  perspectiveBuffer(3, 0) = TO_Q13_18(0.05f);
  perspectiveBuffer(4, 0) = TO_Q13_18(0.9f);
  perspectiveBuffer(5, 0) = TO_Q13_18(15.0f);
  perspectiveBuffer(6, 0) = TO_Q13_18(0.0001f);
  perspectiveBuffer(7, 0) = TO_Q13_18(0.0001f);
  perspectiveBuffer(8, 0) = TO_Q13_18(1.1f); 

  Buffer<uint8_t> output = perspective_call(input, perspectiveBuffer, inputBB);

  // copy the output to the memory from halide buffer
  for (int w = 0; w < 8; w++) {
    for (int z = 0; z < 4; z++) {
      for (int y = 0; y < output.height(); y++) {
        for (int x = 0; x < output.width(); x++) {
	        gdstY[(w*IMAGE_W*64) + (z*128) + y*IMAGE_W + x] = output(x, y, z, w);
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
  
  printf("Compare: ");
  if (memcmp(gdstY_ref, gdstY, dstWidth * dstHeight) == 0) {
    printf("COMPARE_TEST_PASS\n");
  } else {
    printf("COMPARE_TEST_FAIL\n");
  }
  free(gsrcY);
  free(gdstY);
  free(gdstY_ref);
  return 0;
}
