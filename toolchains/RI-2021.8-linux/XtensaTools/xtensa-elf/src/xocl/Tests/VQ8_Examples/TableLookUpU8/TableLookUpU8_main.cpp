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
#include <string>
#include <sys/times.h>
#include <time.h>
/* Input/Output dimensions     */
#define IMAGE_W       512
#define IMAGE_H       512
#define SRC_BIT_DEPTH 8
#define DST_BIT_DEPTH 8
#define TILE_W        256
#define TILE_H        64
#define W_EXT         0
#define H_EXT         0
#define IMAGE_PAD     0

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

uint8_t LUT_U8[512] = {
    218, 149, 25,  187, 146, 63,  67,  157, 9,   146, 255, 25,  129, 110, 43,
    146, 201, 208, 144, 77,  254, 95,  49,  73,  71,  115, 39,  247, 189, 101,
    132, 78,  114, 115, 29,  138, 37,  53,  102, 35,  235, 103, 206, 42,  167,
    169, 63,  66,  193, 113, 156, 64,  159, 141, 101, 159, 2,   237, 11,  209,
    82,  48,  172, 165, 41,  181, 203, 174, 60,  79,  191, 237, 120, 153, 48,
    183, 75,  200, 221, 3,   69,  224, 170, 140, 45,  119, 114, 12,  238, 245,
    243, 75,  82,  25,  41,  101, 129, 255, 247, 90,  251, 163, 204, 166, 201,
    52,  45,  76,  194, 195, 163, 53,  239, 170, 63,  158, 184, 17,  54,  157,
    4,   8,   224, 220, 12,  120, 155, 229, 127, 121, 121, 50,  137, 250, 244,
    68,  92,  146, 254, 29,  225, 170, 10,  64,  202, 119, 88,  104, 23,  32,
    49,  134, 164, 67,  212, 137, 188, 1,   178, 122, 234, 159, 29,  100, 63,
    208, 132, 115, 16,  95,  113, 22,  37,  71,  115, 152, 82,  148, 150, 224,
    10,  161, 7,   17,  194, 32,  171, 28,  83,  57,  84,  49,  156, 188, 118,
    51,  141, 11,  168, 86,  131, 117, 151, 174, 72,  187, 48,  5,   191, 56,
    191, 212, 9,   74,  136, 137, 64,  245, 187, 129, 217, 36,  220, 184, 12,
    97,  87,  9,   222, 27,  25,  127, 120, 162, 159, 171, 234, 238, 39,  139,
    147, 92,  254, 24,  249, 184, 19,  245, 0,   140, 253, 34,  5,   120, 228,
    114,
    218, 149, 25,  187, 146, 63,  67,  157, 9,   146, 255, 25,  129, 110, 43,
    146, 201, 208, 144, 77,  254, 95,  49,  73,  71,  115, 39,  247, 189, 101,
    132, 78,  114, 115, 29,  138, 37,  53,  102, 35,  235, 103, 206, 42,  167,
    169, 63,  66,  193, 113, 156, 64,  159, 141, 101, 159, 2,   237, 11,  209,
    82,  48,  172, 165, 41,  181, 203, 174, 60,  79,  191, 237, 120, 153, 48,
    183, 75,  200, 221, 3,   69,  224, 170, 140, 45,  119, 114, 12,  238, 245,
    243, 75,  82,  25,  41,  101, 129, 255, 247, 90,  251, 163, 204, 166, 201,
    52,  45,  76,  194, 195, 163, 53,  239, 170, 63,  158, 184, 17,  54,  157,
    4,   8,   224, 220, 12,  120, 155, 229, 127, 121, 121, 50,  137, 250, 244,
    68,  92,  146, 254, 29,  225, 170, 10,  64,  202, 119, 88,  104, 23,  32,
    49,  134, 164, 67,  212, 137, 188, 1,   178, 122, 234, 159, 29,  100, 63,
    208, 132, 115, 16,  95,  113, 22,  37,  71,  115, 152, 82,  148, 150, 224,
    10,  161, 7,   17,  194, 32,  171, 28,  83,  57,  84,  49,  156, 188, 118,
    51,  141, 11,  168, 86,  131, 117, 151, 174, 72,  187, 48,  5,   191, 56,
    191, 212, 9,   74,  136, 137, 64,  245, 187, 129, 217, 36,  220, 184, 12,
    97,  87,  9,   222, 27,  25,  127, 120, 162, 159, 171, 234, 238, 39,  139,
    147, 92,  254, 24,  249, 184, 19,  245, 0,   140, 253, 34,  5,   120, 228,
    114,
};

void launch_ocl_tablelookup(uint8_t *Src, uint32_t SrcPitch, uint8_t *Dst,
                            const uint32_t DstPitch, int32_t width,
                            int32_t height, int32_t dstWidth, int32_t dstHeight,
                            uint8_t *lutu8) {
  cl_int status;

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

  cl_mem clSrc = clCreateBuffer(
      context, CL_MEM_READ_ONLY,
      (sizeof(uint8_t) * (SrcPitch * (height + IMAGE_PAD + IMAGE_PAD))), NULL,
      &status);
  CHECK_ERROR(status);

  cl_mem clLut = clCreateBuffer(context, CL_MEM_READ_ONLY,
                                (sizeof(uint8_t) * 512), NULL, &status);
  CHECK_ERROR(status);

  cl_mem clDst =
      clCreateBuffer(context, CL_MEM_WRITE_ONLY,
                     (sizeof(uint8_t) * DstPitch * dstHeight), NULL, &status);
  CHECK_ERROR(status);

  cl_event event = clCreateUserEvent(context, &status);
  CHECK_ERROR(status);

  const char *progSource = loadKernel("TableLookUpU8.cl");
  cl_program program =
      clCreateProgramWithSource(context, 1, &progSource, NULL, &status);
  CHECK_ERROR(status);

  status = clBuildProgram(program, 1, devices, NULL, NULL, NULL);
  CHECK_ERROR(status);

  std::cout << SrcPitch << "  " << DstPitch << "  " << width << "  " << height
            << "  " << dstWidth << "  " << dstHeight << std::endl;

  int local_src_size;
  int local_dst_size;
  local_src_size = ((TILE_W + IMAGE_PAD + IMAGE_PAD) *
                    (TILE_H + IMAGE_PAD + IMAGE_PAD) * sizeof(cl_uchar));
  local_dst_size = ((TILE_W) * (TILE_H) * sizeof(cl_uchar));

  std::cout << " SrcPitch = " << SrcPitch << "\n";
  std::cout << " DstPitch = " << DstPitch << "\n";

  size_t local[2] = {1, 1};
  size_t global[2] = {(size_t)(height / TILE_H), (size_t)(width / TILE_W)};

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
      queue, clSrc, CL_TRUE, 0,
      (sizeof(uint8_t) * (SrcPitch * (height + IMAGE_PAD + IMAGE_PAD))), Src, 0,
      NULL, NULL);
  CHECK_ERROR(error);

  error = clEnqueueWriteBuffer(queue, clLut, CL_TRUE, 0,
                               (sizeof(uint8_t) * 512), lutu8, 0, NULL, NULL);
  CHECK_ERROR(error);

  CHECK(clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&clSrc));
  CHECK(clSetKernelArg(kernel, 1, sizeof(cl_int), (void *)&SrcPitch));
  CHECK(clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&clDst));
  CHECK(clSetKernelArg(kernel, 3, sizeof(cl_int), (void *)&DstPitch));
  CHECK(clSetKernelArg(kernel, 4, sizeof(cl_int), (void *)&width));
  CHECK(clSetKernelArg(kernel, 5, sizeof(cl_int), (void *)&height));
  CHECK(clSetKernelArg(kernel, 6, sizeof(cl_int), (void *)&dstWidth));
  CHECK(clSetKernelArg(kernel, 7, sizeof(cl_int), (void *)&dstHeight));
  CHECK(clSetKernelArg(kernel, 8, sizeof(cl_mem), (void *)&clLut));
  CHECK(clSetKernelArg(kernel, 9, local_src_size, (void *)NULL));
  CHECK(clSetKernelArg(kernel, 10, local_dst_size, (void *)NULL));
  CHECK(clSetKernelArg(kernel, 11, sizeof(cl_mem), (void *)&clErrCodes));

  CHECK(clEnqueueNDRangeKernel(queue, kernel, 2, NULL, global, local, 0, NULL,
                               &event));

  CHECK(clFinish(queue));
  CHECK(clEnqueueReadBuffer(queue, clDst, CL_TRUE, 0,
                            (sizeof(cl_char) * DstPitch * dstHeight), Dst, 0,
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

void run_reference(uint8_t *Src, uint32_t SrcPitch, uint8_t *Dst,
                   const uint32_t DstPitch, int32_t width, int32_t height,
                   int32_t dstWidth, int32_t dstHeight, uint8_t *lutu8) {
  xi_tile src_t;
  xi_tile dst_t;

  // SRC
  XI_TILE_SET_BUFF_PTR((&src_t), Src);
  XI_TILE_SET_DATA_PTR((&src_t),
                       ((uint8_t *)Src) + SrcPitch * IMAGE_PAD + IMAGE_PAD);
  XI_TILE_SET_BUFF_SIZE((&src_t), SrcPitch * (height + 2 * IMAGE_PAD));
  XI_TILE_SET_WIDTH((&src_t), width);
  XI_TILE_SET_HEIGHT((&src_t), height);
  XI_TILE_SET_PITCH((&src_t), width + 2 * IMAGE_PAD);
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

  xiLUT_U8_ref(&src_t, &dst_t, lutu8, 512);
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
  uint8_t *gsrc;
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
  dstWidth = srcWidth;
  dstHeight = srcHeight;
  dstBytes = DST_BIT_DEPTH >> 3;

  // Grey-scale I/O frame buffers
  gsrc = (uint8_t *)malloc(srcStride * (srcHeight + IMAGE_PAD + IMAGE_PAD) *
                           srcBytes);
  gdst = (uint8_t *)malloc(dstWidth * dstHeight * dstBytes);
  gdst_ref = (uint8_t *)malloc(dstWidth * dstHeight * dstBytes);
  if (!gsrc || !gdst || !gdst_ref) {
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

  launch_ocl_tablelookup(gsrc, srcStride, gdst, dstWidth, srcWidth, srcHeight,
                         dstWidth, dstHeight, LUT_U8);

  run_reference(gsrc, srcStride, gdst_ref, dstWidth, srcWidth, srcHeight,
                dstWidth, dstHeight, LUT_U8);

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

  printf("Compare: ");
  sprintf(fname, "Output_ref.yuv");
  if (memcmp(gdst_ref, gdst, dstWidth * dstHeight) == 0) {
    printf("PASS \n");
  } else {
    printf("FAIL \n");
  }

  return 0;
}
