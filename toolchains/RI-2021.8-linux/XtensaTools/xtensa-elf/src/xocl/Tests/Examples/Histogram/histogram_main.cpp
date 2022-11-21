#include "utils.h"
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

/* Input dimensions     */
#define IMAGE_W 512
#define IMAGE_H 512
#define TILE_W  128
#define TILE_H  64
#define SIMD_W  32

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

void launch_ocl_histogram(uint8_t *Src, uint16_t *Dst, int32_t width,
                          int32_t height, int32_t numHistBins) {
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

  cl_mem clSrc =
      clCreateBuffer(context, CL_MEM_READ_ONLY,
                     (sizeof(uint8_t) * width * height), NULL, &status);
  CHECK_ERROR(status);

  cl_mem clTmpDst = clCreateBuffer(
      context, CL_MEM_READ_WRITE,
      (sizeof(uint16_t) * numHistBins * (height / TILE_H) * (width / TILE_W)),
      NULL, &status);
  CHECK_ERROR(status);

  cl_mem clDst = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
                                sizeof(uint16_t) * numHistBins, NULL, &status);
  CHECK_ERROR(status);

  cl_event event = clCreateUserEvent(context, &status);
  CHECK_ERROR(status);

  const char *progSource1 = loadKernel("histogram.cl");
  cl_program program1 =
      clCreateProgramWithSource(context, 1, &progSource1, NULL, &status);
  CHECK_ERROR(status);

  status = clBuildProgram(program1, 1, devices, NULL, NULL, NULL);
  CHECK_ERROR(status);

  const char *progSource2 = loadKernel("reduce.cl");
  cl_program program2 =
      clCreateProgramWithSource(context, 1, &progSource2, NULL, &status);
  CHECK_ERROR(status);

  status = clBuildProgram(program2, 1, devices, NULL, NULL, NULL);
  CHECK_ERROR(status);

  std::cout << "width " << width << " height " << height << " NumHistogramBins "
            << numHistBins << std::endl;

  size_t local1[2] = {1, 1};
  size_t global1[2] = {(size_t)(height / TILE_H), (size_t)(width / TILE_W)};

  std::cout << "global work items (kernel1) " << global1[0] << " X "
            << global1[1] << "\n";
  std::cout << "local work items (kernel1) " << local1[0] << " X " << local1[1]
            << "\n";

  size_t local2 = 1;
  size_t global2 = 1;

  cl_kernel kernel1 = clCreateKernel(program1, "oclComputeHist", &status);
  CHECK_ERROR(status);

  cl_kernel kernel2 = clCreateKernel(program2, "oclComputeReduction", &status);
  CHECK_ERROR(status);

  error = clEnqueueWriteBuffer(queue, clSrc, CL_TRUE, 0,
                               (sizeof(uint8_t) * width * height), Src, 0, NULL,
                               NULL);
  CHECK_ERROR(error);

  CHECK(clSetKernelArg(kernel1, 0, sizeof(cl_mem), (void *)&clSrc));
  CHECK(clSetKernelArg(kernel1, 1, sizeof(cl_mem), (void *)&clTmpDst));
  CHECK(clSetKernelArg(kernel1, 2, sizeof(cl_int), (void *)&width));
  CHECK(clSetKernelArg(kernel1, 3, sizeof(cl_int), (void *)&height));
  CHECK(clSetKernelArg(kernel1, 4, sizeof(cl_int), (void *)&numHistBins));
  CHECK(clSetKernelArg(kernel1, 5, sizeof(uint8_t) * TILE_W * TILE_H,
                       (void *)NULL));
  CHECK(
      clSetKernelArg(kernel1, 6, sizeof(uint16_t) * numHistBins, (void *)NULL));
  CHECK(clSetKernelArg(kernel1, 7, sizeof(uint16_t) * numHistBins * SIMD_W,
                       (void *)NULL));

  CHECK(clSetKernelArg(kernel2, 0, sizeof(cl_mem), (void *)&clTmpDst));
  CHECK(clSetKernelArg(kernel2, 1, sizeof(cl_mem), (void *)&clDst));
  CHECK(clSetKernelArg(kernel2, 2, sizeof(cl_int), (void *)&width));
  CHECK(clSetKernelArg(kernel2, 3, sizeof(cl_int), (void *)&height));
  CHECK(clSetKernelArg(kernel2, 4, sizeof(cl_int), (void *)&numHistBins));
  CHECK(clSetKernelArg(kernel2, 5,
                       sizeof(uint16_t) * (width / TILE_W) * (height / TILE_H) *
                           numHistBins,
                       (void *)NULL));
  CHECK(
      clSetKernelArg(kernel2, 6, sizeof(uint16_t) * numHistBins, (void *)NULL));

  CHECK(clEnqueueNDRangeKernel(queue, kernel1, 2, NULL, global1, local1, 0,
                               NULL, &event));
  CHECK(clEnqueueNDRangeKernel(queue, kernel2, 1, NULL, &global2, &local2, 1,
                               &event, NULL));

  CHECK(clFinish(queue));

  CHECK(clEnqueueReadBuffer(queue, clDst, CL_TRUE, 0,
                            sizeof(cl_ushort) * numHistBins, Dst, 0, NULL,
                            NULL));

  CHECK(clFinish(queue));

  std::cout << "done!!" << std::endl;
}

void run_reference(uint8_t *Src, uint16_t *Dst, int32_t width, int32_t height) {
  for (int x = 0; x < width; ++x) {
    for (int y = 0; y < height; ++y) {
      Dst[Src[y * width + x]] += 1;
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
  uint8_t *gsrc;
  uint16_t *gdst;
  uint16_t *gdst_ref;

  // Image dimensions
  int32_t srcWidth, srcHeight;
  int32_t numHistBins = 256;

  // Set frame dimensions
  srcWidth = IMAGE_W;
  srcHeight = IMAGE_H;

  // Grey-scale I/O frame buffers
  gsrc = (uint8_t *)malloc(srcWidth * srcHeight);
  gdst = (uint16_t *)calloc(numHistBins, sizeof(uint16_t));
  gdst_ref = (uint16_t *)calloc(numHistBins, sizeof(uint16_t));
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

  for (indy = 0; indy < srcHeight; indy++) {
    uint8_t *ptr;
    ptr = gsrc + (indy * srcWidth);
    fread(ptr, 1, srcWidth, fpIn);
  }
  fclose(fpIn);

  launch_ocl_histogram(gsrc, gdst, srcWidth, srcHeight, numHistBins);

  run_reference(gsrc, gdst_ref, srcWidth, srcHeight);

  int fail = 0;
  int count = 0;
  for (int i = 0; i < numHistBins; ++i) {
    count += gdst_ref[i];
    if (gdst_ref[i] != gdst[i]) {
      printf("Expected at %d, %d, but got %d\n", i, gdst_ref[i], gdst[i]);
      printf("Compare: FAIL \n");
      fail = 1;
      // break;
    }
  }

  if (count != srcWidth * srcHeight) {
    printf("Expected count to be %d, but got %d\n", srcWidth * srcHeight,
           count);
  }

  if (!fail)
    printf("Compare: PASS \n");

  return 0;
}
