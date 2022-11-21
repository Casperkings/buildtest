#include "camera_pipe_parameters.h"
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
#define IMAGE_W 512
#define IMAGE_H 512
#define SRC_BIT_DEPTH 16
#define DST_BIT_DEPTH 24
#define TILE_W 128
#define TILE_H 64
#define W_EXT 8
#define H_EXT 8
#define IMAGE_PAD 8

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

short temp[2048 * 2048];
void camera_pipe_process(short *input, int inputPitch, int inputWidth,
                         int intputHeight, unsigned char *output,
                         int outputPitch, short *matrix, unsigned char *curve);
void makeLUT(float contrast, int blackLevel, float gamma, unsigned char *lut,
             unsigned short minRaw, unsigned short maxRaw);
void launch_ocl_kernel(int16_t *Src, uint32_t SrcPitch, uint8_t *Dst,
                       const uint32_t DstPitch, int32_t width, int32_t height,
                       int32_t dstWidth, int32_t dstHeight) {
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
      (sizeof(int16_t) * (SrcPitch * (height + IMAGE_PAD + IMAGE_PAD))), NULL,
      &status);
  CHECK_ERROR(status);

  cl_mem clSrc_matrix = clCreateBuffer(context, CL_MEM_READ_ONLY,
                                       (sizeof(int16_t) * 12), NULL, &status);
  CHECK_ERROR(status);

  cl_mem clSrc_curve = clCreateBuffer(context, CL_MEM_READ_ONLY,
                                      (sizeof(int8_t) * 1024), NULL, &status);
  CHECK_ERROR(status);

  cl_mem clDst = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
                                (sizeof(uint8_t) * DstPitch * dstHeight * 3),
                                NULL, &status);
  CHECK_ERROR(status);

  cl_mem clDst_debug =
      clCreateBuffer(context, CL_MEM_WRITE_ONLY,
                     (sizeof(uint16_t) * width * height), NULL, &status);
  CHECK_ERROR(status);

  cl_event event = clCreateUserEvent(context, &status);
  CHECK_ERROR(status);

  const char *progSource = loadKernel("Camera_pipe.cl");

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
                    (TILE_H + IMAGE_PAD + IMAGE_PAD) * sizeof(cl_short));
  local_dst_size = ((TILE_W) * (TILE_H) * sizeof(cl_uchar) * 3);

  std::cout << " SrcPitch = " << SrcPitch << "\n";
  std::cout << " DstPitch = " << DstPitch << "\n";

  size_t local[2] = {1, 1};
  size_t global[2] = {(size_t)(height / TILE_H), (size_t)(width / TILE_W)};

  std::cout << "global work items " << global[0] << " X " << global[1] << "\n";
  std::cout << "local work items " << local[0] << " X " << local[1] << "\n";

  cl_kernel kernel = clCreateKernel(program, "oclKernel", &status);
  CHECK_ERROR(status);

  error = clEnqueueWriteBuffer(
      queue, clSrc, CL_TRUE, 0,
      (sizeof(int16_t) * (SrcPitch * (height + IMAGE_PAD + IMAGE_PAD))), Src, 0,
      NULL, NULL);
  CHECK_ERROR(error);

  error = clEnqueueWriteBuffer(queue, clSrc_matrix, CL_TRUE, 0,
                               (sizeof(int16_t) * (12)), matrix, 0, NULL, NULL);
  CHECK_ERROR(error);

  error =
      clEnqueueWriteBuffer(queue, clSrc_curve, CL_TRUE, 0,
                           (sizeof(uint8_t) * (1024)), curve, 0, NULL, NULL);
  CHECK_ERROR(error);

  CHECK(clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&clSrc));
  CHECK(clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&clSrc_matrix));
  CHECK(clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&clSrc_curve));
  CHECK(clSetKernelArg(kernel, 3, sizeof(cl_int), (void *)&SrcPitch));
  CHECK(clSetKernelArg(kernel, 4, sizeof(cl_mem), (void *)&clDst));
  CHECK(clSetKernelArg(kernel, 5, sizeof(cl_int), (void *)&DstPitch));
  CHECK(clSetKernelArg(kernel, 6, sizeof(cl_int), (void *)&width));
  CHECK(clSetKernelArg(kernel, 7, sizeof(cl_int), (void *)&height));
  CHECK(clSetKernelArg(kernel, 8, sizeof(cl_int), (void *)&dstWidth));
  CHECK(clSetKernelArg(kernel, 9, sizeof(cl_int), (void *)&dstHeight));

  CHECK(clSetKernelArg(kernel, 10, local_src_size, (void *)NULL));
  CHECK(clSetKernelArg(kernel, 11, 12 * 2, (void *)NULL));
  CHECK(clSetKernelArg(kernel, 12, 1024, (void *)NULL));
  CHECK(clSetKernelArg(kernel, 13, local_dst_size, (void *)NULL));
  CHECK(clSetKernelArg(kernel, 14, sizeof(cl_mem), (void *)&clDst_debug));

  CHECK(clEnqueueNDRangeKernel(queue, kernel, 2, NULL, global, local, 0, NULL,
                               &event));

  CHECK(clFinish(queue));
  CHECK(clEnqueueReadBuffer(queue, clDst, CL_TRUE, 0,
                            (sizeof(cl_char) * DstPitch * dstHeight * 3), Dst,
                            0, NULL, NULL));

  CHECK(clFinish(queue));

  std::cout << "done!!" << std::endl;
}

void run_reference(int16_t *Src, uint32_t SrcPitch, uint8_t *Dst,
                   const uint32_t DstPitch, int32_t width, int32_t height,
                   int32_t dstWidth, int32_t dstHeight) {

  camera_pipe_process(Src, SrcPitch, width + 2 * IMAGE_PAD,
                      height + 2 * IMAGE_PAD, Dst, DstPitch * 3, matrix, curve);
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
    uint16_t *ptr;
    ptr = ((uint16_t *)gsrc) + (indy * srcStride) + IMAGE_PAD;
    fread(ptr, 1, srcWidth * 2, fpIn);
  }
  fclose(fpIn);
  uint16_t *ptr = (uint16_t *)gsrc;
  for (indy = 0; indy < (srcHeight + 2 * IMAGE_PAD); indy++) {
    for (int indx = 0; indx < srcWidth + 2 * IMAGE_PAD; indx++) {
      ptr[indy * (srcWidth + 2 * IMAGE_PAD) + indx] =
          0xFF & ptr[indy * (srcWidth + 2 * IMAGE_PAD) + indx];
    }
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

  // int color_temp = 3200;
  int gamma = 2;
  int contrast = 50;
  // int sharpen = 5;
  int blackLevel = 25;
  int whiteLevel = 1023;

  makeLUT((float)contrast, blackLevel, (float)gamma, curve, blackLevel,
          whiteLevel);

  launch_ocl_kernel((int16_t *)gsrc, srcStride, gdst, dstWidth, srcWidth,
                    srcHeight, dstWidth, dstHeight);

  run_reference((int16_t *)gsrc, srcStride, gdst_ref, dstWidth, srcWidth,
                srcHeight, dstWidth, dstHeight);

  FILE *fpOut;
  // Write result
  sprintf(fname, "Output.yuv");
  fpOut = fopen(fname, "wb");
  if (fpOut == NULL) {
    printf("Error opening output file: %s\n", fname);
    exit(0);
  }

  fwrite(gdst, dstWidth * dstHeight * dstBytes, 1, fpOut);
  fclose(fpOut);

  printf("Compare: ");
  if (memcmp(gdst_ref, gdst, dstWidth * dstHeight * dstBytes) == 0) {
    printf("PASS \n");
  } else {
    printf("FAIL \n");
  }

  return 0;
}
