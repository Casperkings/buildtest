#include "utils.h"
#include "xi_api_ref.h"
#include <CL/cl.h>
#include <CL/opencl.h>
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

/* Input/Output dimensions     */
#define IMAGE_W 512
#define IMAGE_H 512
#define SRC_BIT_DEPTH 8
#define DST_BIT_DEPTH 8
#define TILE_W 128
#define TILE_H 128
#define OUTPUT_TILE_W 128
#define OUTPUT_TILE_H 64
#define W_EXT 0
#define H_EXT 0
#define IMAGE_PAD 0

#define MIN2(a, b) ((a) < (b) ? (a) : (b))
#define MIN4(a, b, c, d) (MIN2(MIN2(a, b), MIN2(c, d)))
#define MAX2(a, b) ((a) > (b) ? (a) : (b))
#define MAX4(a, b, c, d) (MAX2(MAX2(a, b), MAX2(c, d)))

#define CHECK(API)                                                             \
  do {                                                                         \
    status = (API);                                                            \
    if (status != CL_SUCCESS) {                                                \
      printf("Failed at #%d, status=%d\n", __LINE__, status);                  \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

#define CHECK_ERROR(err)                                                       \
  do {                                                                         \
    if (err != CL_SUCCESS) {                                                   \
      printf("Failed at #%d, err=%d\n", __LINE__, err);                        \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

#define TO_Q13_18(val) ((int)((val) * (1 << 18) + 0.5))

void launch_ocl_kernel(uint8_t *SrcY, uint32_t SrcPitch, uint8_t *DstY,
                       const uint32_t DstPitch, int32_t width, int32_t height,
                       int32_t dstWidth, int32_t dstHeight,
                       uint32_t *perspective, uint16_t *InputBB) {
  cl_int status;
  int32_t OutputTileW = OUTPUT_TILE_W;
  int32_t OutputTileH = OUTPUT_TILE_H;
  cl_platform_id *platform = NULL;
  cl_uint PlatformCout = 0;
  int error = clGetPlatformIDs(0, NULL, &PlatformCout);
  CHECK_ERROR(error);

  platform = (cl_platform_id *)malloc(PlatformCout * sizeof(cl_platform_id));
  error = clGetPlatformIDs(PlatformCout, platform, NULL);
  CHECK_ERROR(error);

  cl_device_id *devices = NULL;

  cl_uint num_devices = 0;
  error =
      clGetDeviceIDs(platform[0], CL_DEVICE_TYPE_ALL, 0, NULL, &num_devices);
  CHECK_ERROR(error);

  devices = (cl_device_id *)malloc(num_devices * sizeof(cl_device_id));
  error = clGetDeviceIDs(platform[0], CL_DEVICE_TYPE_ALL, num_devices, devices,
                         NULL);
  CHECK_ERROR(error);

  char deviceName[1024];
  error = clGetDeviceInfo(devices[0], CL_DEVICE_NAME, sizeof(deviceName),
                          deviceName, NULL);
  CHECK_ERROR(error);

  cl_context context =
      clCreateContext(NULL, num_devices, devices, NULL, NULL, &status);
  CHECK_ERROR(status);
  cl_command_queue queue = clCreateCommandQueue(
      context, devices[0], CL_QUEUE_PROFILING_ENABLE, &status);
  CHECK_ERROR(status);

  cl_mem clSrcY = clCreateBuffer(
      context, CL_MEM_READ_ONLY,
      (sizeof(uint8_t) * (SrcPitch * (height + IMAGE_PAD + IMAGE_PAD))), NULL,
      &status);
  CHECK_ERROR(status);

  cl_mem clPerspective = clCreateBuffer(context, CL_MEM_READ_ONLY,
                                        (sizeof(uint32_t) * 9), NULL, &status);
  CHECK_ERROR(status);

  cl_mem clInputBB =
      clCreateBuffer(context, CL_MEM_READ_ONLY,
                     (sizeof(uint16_t) * (height / OUTPUT_TILE_H) *
                      (width / OUTPUT_TILE_W) * 4),
                     NULL, &status);
  CHECK_ERROR(status);

  cl_mem clDstY =
      clCreateBuffer(context, CL_MEM_WRITE_ONLY,
                     (sizeof(uint8_t) * DstPitch * dstHeight), NULL, &status);
  CHECK_ERROR(status);

  cl_event event = clCreateUserEvent(context, &status);
  CHECK_ERROR(status);

  const char *progSource = loadKernel("Perspective.cl");
  cl_program program =
      clCreateProgramWithSource(context, 1, &progSource, NULL, &status);
  CHECK_ERROR(status);

  status = clBuildProgram(program, 1, devices, NULL, NULL, NULL);
  CHECK_ERROR(status);

  std::cout << SrcPitch << "  " << DstPitch << "  " << width << "  " << height
            << "  " << dstWidth << "  " << dstHeight << std::endl;

  int local_srcY_size;
  int local_dstY_size;
  local_srcY_size = ((TILE_W + IMAGE_PAD + IMAGE_PAD) *
                     (TILE_H + IMAGE_PAD + IMAGE_PAD) * sizeof(cl_uchar));
  local_dstY_size = ((OUTPUT_TILE_W) * (OUTPUT_TILE_H) * sizeof(cl_uchar));

  std::cout << " SrcPitch = " << SrcPitch << "\n";
  std::cout << " DstPitch = " << DstPitch << "\n";

  size_t local[2] = {1, 1};
  size_t global[2] = {(size_t)(height / OUTPUT_TILE_H),
                      (size_t)(width / OUTPUT_TILE_W)};

  std::cout << "global work items " << global[0] << " X " << global[1] << "\n";
  std::cout << "local work items " << local[0] << " X " << local[1] << "\n";

  cl_mem clErrCodes =
      clCreateBuffer(context, CL_MEM_WRITE_ONLY,
                     (sizeof(int) * global[0] * global[1]), NULL, &status);
  CHECK_ERROR(status);

  int *error_codes;
  error_codes = (int *)malloc(sizeof(int) * global[0] * global[1]);
  memset(error_codes, 0, sizeof(int) * global[0] * global[1]);

  cl_kernel kernel = clCreateKernel(program, "oclKernel", &status);
  CHECK_ERROR(status);

  error = clEnqueueWriteBuffer(
      queue, clSrcY, CL_TRUE, 0,
      (sizeof(uint8_t) * (SrcPitch * (height + IMAGE_PAD + IMAGE_PAD))), SrcY,
      0, NULL, NULL);
  CHECK_ERROR(error);

  error =
      clEnqueueWriteBuffer(queue, clPerspective, CL_TRUE, 0,
                           (sizeof(uint32_t) * 9), perspective, 0, NULL, NULL);
  CHECK_ERROR(error);

  error = clEnqueueWriteBuffer(queue, clInputBB, CL_TRUE, 0,
                               (sizeof(uint16_t) * (height / OUTPUT_TILE_H) *
                                (width / OUTPUT_TILE_W) * 4),
                               InputBB, 0, NULL, NULL);
  CHECK_ERROR(error);

  CHECK(clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&clSrcY));
  CHECK(clSetKernelArg(kernel, 1, sizeof(cl_int), (void *)&SrcPitch));
  CHECK(clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&clDstY));
  CHECK(clSetKernelArg(kernel, 3, sizeof(cl_int), (void *)&DstPitch));
  CHECK(clSetKernelArg(kernel, 4, sizeof(cl_int), (void *)&width));
  CHECK(clSetKernelArg(kernel, 5, sizeof(cl_int), (void *)&height));
  CHECK(clSetKernelArg(kernel, 6, sizeof(cl_int), (void *)&dstWidth));
  CHECK(clSetKernelArg(kernel, 7, sizeof(cl_int), (void *)&dstHeight));
  CHECK(clSetKernelArg(kernel, 8, sizeof(cl_int), (void *)&OutputTileW));
  CHECK(clSetKernelArg(kernel, 9, sizeof(cl_int), (void *)&OutputTileH));
  CHECK(clSetKernelArg(kernel, 10, sizeof(cl_mem), (void *)&clPerspective));
  CHECK(clSetKernelArg(kernel, 11, sizeof(cl_mem), (void *)&clInputBB));
  CHECK(clSetKernelArg(kernel, 12, local_srcY_size, (void *)NULL));
  CHECK(clSetKernelArg(kernel, 13, local_dstY_size, (void *)NULL));
  CHECK(clSetKernelArg(kernel, 14, sizeof(cl_mem), (void *)&clErrCodes));

  CHECK(clEnqueueNDRangeKernel(queue, kernel, 2, NULL, global, local, 0, NULL,
                               &event));

  CHECK(clFinish(queue));
  CHECK(clEnqueueReadBuffer(queue, clDstY, CL_TRUE, 0,
                            (sizeof(cl_char) * DstPitch * dstHeight), DstY, 0,
                            NULL, NULL));
  CHECK(clEnqueueReadBuffer(queue, clErrCodes, CL_TRUE, 0,
                            (sizeof(int) * global[0] * global[1]), error_codes,
                            0, NULL, NULL));

  CHECK(clFinish(queue));

  if (error_codes[0]) {
    int i;
    for (i = 0; i < ((int)(global[0] * global[1])); i++) {
      std::cout << "Work group errors " << error_codes[i] << std::endl;
    }
  }
  std::cout << "done!!" << std::endl;
}

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
      int32_t startX =
          MIN4(InputBBx[0], InputBBx[1], InputBBx[2], InputBBx[3]) - 2;
      int32_t startY =
          MIN4(InputBBy[0], InputBBy[1], InputBBy[2], InputBBy[3]) - 2;
      inTileW =
          MAX4(InputBBx[0], InputBBx[1], InputBBx[2], InputBBx[3]) - startX + 2;
      inTileH =
          MAX4(InputBBy[0], InputBBy[1], InputBBy[2], InputBBy[3]) - startY + 2;

      inputBB[0] = startX;
      inputBB[1] = startY;
      inputBB[2] = inTileW;
      inputBB[3] = inTileH;
      inputBB += 4;
      // printf("\n X = %d, Y = %d, Width = %d, Height = %d,",startX, startY,
      // inTileW, inTileH);
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
    ptr = gsrcY + (indy * srcStride) + IMAGE_PAD;
    fread(ptr, 1, srcWidth, fpIn);
  }
  fclose(fpIn);

  uint32_t perspective[] =
      //{TO_Q13_18(0.6), TO_Q13_18(0.6), TO_Q13_18(15.31), TO_Q13_18(-0.6),
      //TO_Q13_18(0.6), TO_Q13_18(70.5), TO_Q13_18(0.0009), TO_Q13_18(0.0003),
      //TO_Q13_18(1.021)};
      {TO_Q13_18(0.9f),    TO_Q13_18(0.05f),   TO_Q13_18(15.0f),
       TO_Q13_18(0.05f),   TO_Q13_18(0.9f),    TO_Q13_18(15.0f),
       TO_Q13_18(0.0001f), TO_Q13_18(0.0001f), TO_Q13_18(1.1f)};
  uint16_t InputBB[5 * 32];

  run_reference(gsrcY, srcStride, gdstY_ref, dstWidth, srcWidth, srcHeight,
                dstWidth, dstHeight, perspective, InputBB);

  launch_ocl_kernel(gsrcY, srcStride, gdstY, dstWidth, srcWidth, srcHeight,
                    dstWidth, dstHeight, perspective, InputBB);

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
    printf("PASS \n");
  } else {
    printf("FAIL \n");
  }

  return 0;
}
