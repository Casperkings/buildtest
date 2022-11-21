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
#define DST_BIT_DEPTH 32
#define TILE_W        128
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

void launch_ocl_Integral(uint8_t *Src, uint32_t SrcPitch, uint8_t *Dst,
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
      (sizeof(uint8_t) * (SrcPitch * (height + IMAGE_PAD + IMAGE_PAD))), NULL,
      &status);
  CHECK_ERROR(status);

  cl_mem clDst_RowTiled =
      clCreateBuffer(context, CL_MEM_READ_WRITE,
                     (sizeof(uint16_t) * DstPitch * dstHeight), NULL, &status);
  CHECK_ERROR(status);

  cl_mem clDst_LastColSums = clCreateBuffer(
      context, CL_MEM_READ_WRITE,
      (sizeof(uint32_t) * (DstPitch / TILE_W) * dstHeight), NULL, &status);
  CHECK_ERROR(status);

  cl_mem clDst_RowFull =
      clCreateBuffer(context, CL_MEM_READ_WRITE,
                     (sizeof(uint32_t) * DstPitch * dstHeight), NULL, &status);
  CHECK_ERROR(status);

  cl_mem clDst_ColTiled =
      clCreateBuffer(context, CL_MEM_READ_WRITE,
                     (sizeof(uint32_t) * DstPitch * dstHeight), NULL, &status);
  CHECK_ERROR(status);

  cl_mem clDst_LastRowSums = clCreateBuffer(
      context, CL_MEM_READ_WRITE,
      (sizeof(uint32_t) * DstPitch * (dstHeight / TILE_H)), NULL, &status);
  CHECK_ERROR(status);

  cl_mem clDst_ColFull =
      clCreateBuffer(context, CL_MEM_WRITE_ONLY,
                     (sizeof(uint32_t) * DstPitch * dstHeight), NULL, &status);
  CHECK_ERROR(status);

  cl_event event = clCreateUserEvent(context, &status);
  CHECK_ERROR(status);

  const char *progSource = loadKernel("IntegralU8U32.cl");
  cl_program program =
      clCreateProgramWithSource(context, 1, &progSource, NULL, &status);
  CHECK_ERROR(status);

  status = clBuildProgram(program, 1, devices, NULL, NULL, NULL);
  CHECK_ERROR(status);

  std::cout << SrcPitch << "  " << DstPitch << "  " << width << "  " << height
            << "  " << dstWidth << "  " << dstHeight << std::endl;

  int local_src_size;
  int local_dst_size;

  size_t local[2] = {1, 1};
  size_t global[2];

  cl_kernel kernelRowTiled =
      clCreateKernel(program, "oclKernelRowTiled", &status);
  CHECK_ERROR(status);

  cl_kernel kernelLastColSums =
      clCreateKernel(program, "oclKernelLastColSums", &status);
  CHECK_ERROR(status);

  cl_kernel kernelRowFull =
      clCreateKernel(program, "oclKernelRowFull", &status);
  CHECK_ERROR(status);

  cl_kernel kernelColTiled =
      clCreateKernel(program, "oclKernelColTiled", &status);
  CHECK_ERROR(status);

  cl_kernel kernelLastRowSums =
      clCreateKernel(program, "oclKernelLastRowSums", &status);
  CHECK_ERROR(status);

  cl_kernel kernelColFull =
      clCreateKernel(program, "oclKernelColFull", &status);
  CHECK_ERROR(status);

  error = clEnqueueWriteBuffer(
      queue, clSrc, CL_TRUE, 0,
      (sizeof(uint8_t) * (SrcPitch * (height + IMAGE_PAD + IMAGE_PAD))), Src, 0,
      NULL, NULL);
  CHECK_ERROR(error);

  local_src_size = ((TILE_W + IMAGE_PAD + IMAGE_PAD) *
                    (TILE_H + IMAGE_PAD + IMAGE_PAD) * sizeof(cl_uchar));
  local_dst_size = ((TILE_W) * (TILE_H) * sizeof(cl_ushort));

  std::cout << " SrcPitch = " << SrcPitch << "\n";
  std::cout << " DstPitch = " << DstPitch << "\n";

  global[0] = (size_t)(height / TILE_H);
  global[1] = (size_t)(width / TILE_W);

  std::cout << "global work items " << global[0] << " X " << global[1] << "\n";
  std::cout << "local work items " << local[0] << " X " << local[1] << "\n";

  CHECK(clSetKernelArg(kernelRowTiled, 0, sizeof(cl_mem), (void *)&clSrc));
  CHECK(clSetKernelArg(kernelRowTiled, 1, sizeof(cl_int), (void *)&SrcPitch));
  CHECK(clSetKernelArg(kernelRowTiled, 2, sizeof(cl_mem),
                       (void *)&clDst_RowTiled));
  CHECK(clSetKernelArg(kernelRowTiled, 3, sizeof(cl_int), (void *)&DstPitch));
  CHECK(clSetKernelArg(kernelRowTiled, 4, sizeof(cl_int), (void *)&width));
  CHECK(clSetKernelArg(kernelRowTiled, 5, sizeof(cl_int), (void *)&height));
  CHECK(clSetKernelArg(kernelRowTiled, 6, sizeof(cl_int), (void *)&dstWidth));
  CHECK(clSetKernelArg(kernelRowTiled, 7, sizeof(cl_int), (void *)&dstHeight));

  CHECK(clSetKernelArg(kernelRowTiled, 8, local_src_size, (void *)NULL));
  CHECK(clSetKernelArg(kernelRowTiled, 9, local_dst_size, (void *)NULL));

  CHECK(clEnqueueNDRangeKernel(queue, kernelRowTiled, 2, NULL, global, local, 0,
                               NULL, &event));

  local_src_size = ((IMAGE_W / TILE_W) * (IMAGE_H) * sizeof(cl_ushort));
  local_dst_size = ((IMAGE_W / TILE_W) * (IMAGE_H) * sizeof(cl_uint));

  std::cout << " SrcPitch = " << SrcPitch << "\n";
  std::cout << " DstPitch = " << DstPitch << "\n";

  global[0] = (size_t)1;
  global[1] = (size_t)1;

  std::cout << "global work items " << global[0] << " X " << global[1] << "\n";
  std::cout << "local work items " << local[0] << " X " << local[1] << "\n";

  CHECK(clSetKernelArg(kernelLastColSums, 0, sizeof(cl_mem),
                       (void *)&clDst_RowTiled));
  CHECK(
      clSetKernelArg(kernelLastColSums, 1, sizeof(cl_int), (void *)&DstPitch));
  CHECK(clSetKernelArg(kernelLastColSums, 2, sizeof(cl_mem),
                       (void *)&clDst_LastColSums));
  CHECK(
      clSetKernelArg(kernelLastColSums, 3, sizeof(cl_int), (void *)&DstPitch));
  CHECK(
      clSetKernelArg(kernelLastColSums, 4, sizeof(cl_int), (void *)&dstWidth));
  CHECK(
      clSetKernelArg(kernelLastColSums, 5, sizeof(cl_int), (void *)&dstHeight));
  CHECK(
      clSetKernelArg(kernelLastColSums, 6, sizeof(cl_int), (void *)&dstWidth));
  CHECK(
      clSetKernelArg(kernelLastColSums, 7, sizeof(cl_int), (void *)&dstHeight));
  CHECK(clSetKernelArg(kernelLastColSums, 8, local_src_size, (void *)NULL));
  CHECK(clSetKernelArg(kernelLastColSums, 9, local_dst_size, (void *)NULL));

  CHECK(clEnqueueNDRangeKernel(queue, kernelLastColSums, 2, NULL, global, local,
                               0, NULL, &event));
  CHECK(clFinish(queue));
  local_src_size = ((TILE_W) * (TILE_H) * sizeof(cl_ushort));

  int local_src_Col_size = ((TILE_H) * sizeof(cl_uint));

  local_dst_size = ((TILE_W) * (TILE_H) * sizeof(cl_uint));

  std::cout << " SrcPitch = " << SrcPitch << "\n";
  std::cout << " DstPitch = " << DstPitch << "\n";

  global[0] = (size_t)(height / TILE_H);
  global[1] = (size_t)(width / TILE_W);

  std::cout << "global work items " << global[0] << " X " << global[1] << "\n";
  std::cout << "local work items " << local[0] << " X " << local[1] << "\n";

  CHECK(clSetKernelArg(kernelRowFull, 0, sizeof(cl_mem),
                       (void *)&clDst_RowTiled));
  CHECK(clSetKernelArg(kernelRowFull, 1, sizeof(cl_mem),
                       (void *)&clDst_LastColSums));
  CHECK(clSetKernelArg(kernelRowFull, 2, sizeof(cl_int), (void *)&DstPitch));
  CHECK(
      clSetKernelArg(kernelRowFull, 3, sizeof(cl_mem), (void *)&clDst_RowFull));
  CHECK(clSetKernelArg(kernelRowFull, 4, sizeof(cl_int), (void *)&DstPitch));
  CHECK(clSetKernelArg(kernelRowFull, 5, sizeof(cl_int), (void *)&dstWidth));
  CHECK(clSetKernelArg(kernelRowFull, 6, sizeof(cl_int), (void *)&dstHeight));
  CHECK(clSetKernelArg(kernelRowFull, 7, sizeof(cl_int), (void *)&dstWidth));
  CHECK(clSetKernelArg(kernelRowFull, 8, sizeof(cl_int), (void *)&dstHeight));
  CHECK(clSetKernelArg(kernelRowFull, 9, local_src_size, (void *)NULL));
  CHECK(clSetKernelArg(kernelRowFull, 10, local_src_Col_size, (void *)NULL));
  CHECK(clSetKernelArg(kernelRowFull, 11, local_dst_size, (void *)NULL));

  CHECK(clEnqueueNDRangeKernel(queue, kernelRowFull, 2, NULL, global, local, 0,
                               NULL, &event));

  local_src_size = ((TILE_W + IMAGE_PAD + IMAGE_PAD) *
                    (TILE_H + IMAGE_PAD + IMAGE_PAD) * sizeof(cl_uint));
  local_dst_size = ((TILE_W) * (TILE_H) * sizeof(cl_uint));

  std::cout << " SrcPitch = " << SrcPitch << "\n";
  std::cout << " DstPitch = " << DstPitch << "\n";

  global[0] = (size_t)(height / TILE_H);
  global[1] = (size_t)(width / TILE_W);

  std::cout << "global work items " << global[0] << " X " << global[1] << "\n";
  std::cout << "local work items " << local[0] << " X " << local[1] << "\n";

  CHECK(clSetKernelArg(kernelColTiled, 0, sizeof(cl_mem),
                       (void *)&clDst_RowFull));
  CHECK(clSetKernelArg(kernelColTiled, 1, sizeof(cl_int), (void *)&DstPitch));
  CHECK(clSetKernelArg(kernelColTiled, 2, sizeof(cl_mem),
                       (void *)&clDst_ColTiled));
  CHECK(clSetKernelArg(kernelColTiled, 3, sizeof(cl_int), (void *)&DstPitch));
  CHECK(clSetKernelArg(kernelColTiled, 4, sizeof(cl_int), (void *)&dstWidth));
  CHECK(clSetKernelArg(kernelColTiled, 5, sizeof(cl_int), (void *)&dstHeight));
  CHECK(clSetKernelArg(kernelColTiled, 6, sizeof(cl_int), (void *)&dstWidth));
  CHECK(clSetKernelArg(kernelColTiled, 7, sizeof(cl_int), (void *)&dstHeight));
  CHECK(clSetKernelArg(kernelColTiled, 8, local_src_size, (void *)NULL));
  CHECK(clSetKernelArg(kernelColTiled, 9, local_dst_size, (void *)NULL));

  CHECK(clEnqueueNDRangeKernel(queue, kernelColTiled, 2, NULL, global, local, 0,
                               NULL, &event));
  // CHECK(clFinish(queue));

  local_src_size = ((IMAGE_W) * (IMAGE_H / TILE_H) * sizeof(cl_uint));
  local_dst_size = ((IMAGE_W) * (IMAGE_H / TILE_H) * sizeof(cl_uint));

  std::cout << " SrcPitch = " << SrcPitch << "\n";
  std::cout << " DstPitch = " << DstPitch << "\n";

  global[0] = (size_t)1;
  global[1] = (size_t)1;

  std::cout << "global work items " << global[0] << " X " << global[1] << "\n";
  std::cout << "local work items " << local[0] << " X " << local[1] << "\n";

  CHECK(clSetKernelArg(kernelLastRowSums, 0, sizeof(cl_mem),
                       (void *)&clDst_ColTiled));
  CHECK(
      clSetKernelArg(kernelLastRowSums, 1, sizeof(cl_int), (void *)&DstPitch));
  CHECK(clSetKernelArg(kernelLastRowSums, 2, sizeof(cl_mem),
                       (void *)&clDst_LastRowSums));
  CHECK(
      clSetKernelArg(kernelLastRowSums, 3, sizeof(cl_int), (void *)&DstPitch));
  CHECK(
      clSetKernelArg(kernelLastRowSums, 4, sizeof(cl_int), (void *)&dstWidth));
  CHECK(
      clSetKernelArg(kernelLastRowSums, 5, sizeof(cl_int), (void *)&dstHeight));
  CHECK(
      clSetKernelArg(kernelLastRowSums, 6, sizeof(cl_int), (void *)&dstWidth));
  CHECK(
      clSetKernelArg(kernelLastRowSums, 7, sizeof(cl_int), (void *)&dstHeight));
  CHECK(clSetKernelArg(kernelLastRowSums, 8, local_src_size, (void *)NULL));
  CHECK(clSetKernelArg(kernelLastRowSums, 9, local_dst_size, (void *)NULL));

  CHECK(clEnqueueNDRangeKernel(queue, kernelLastRowSums, 2, NULL, global, local,
                               0, NULL, &event));
  // CHECK(clFinish(queue));
  local_src_size = ((TILE_W) * (TILE_H) * sizeof(cl_uint));

  local_src_Col_size = ((TILE_W) * sizeof(cl_uint));

  local_dst_size = ((TILE_W) * (TILE_H) * sizeof(cl_uint));

  std::cout << " SrcPitch = " << SrcPitch << "\n";
  std::cout << " DstPitch = " << DstPitch << "\n";

  global[0] = (size_t)(height / TILE_H);
  global[1] = (size_t)(width / TILE_W);

  std::cout << "global work items " << global[0] << " X " << global[1] << "\n";
  std::cout << "local work items " << local[0] << " X " << local[1] << "\n";

  CHECK(clSetKernelArg(kernelColFull, 0, sizeof(cl_mem),
                       (void *)&clDst_ColTiled));
  CHECK(clSetKernelArg(kernelColFull, 1, sizeof(cl_mem),
                       (void *)&clDst_LastRowSums));
  CHECK(clSetKernelArg(kernelColFull, 2, sizeof(cl_int), (void *)&DstPitch));
  CHECK(
      clSetKernelArg(kernelColFull, 3, sizeof(cl_mem), (void *)&clDst_ColFull));
  CHECK(clSetKernelArg(kernelColFull, 4, sizeof(cl_int), (void *)&DstPitch));
  CHECK(clSetKernelArg(kernelColFull, 5, sizeof(cl_int), (void *)&dstWidth));
  CHECK(clSetKernelArg(kernelColFull, 6, sizeof(cl_int), (void *)&dstHeight));
  CHECK(clSetKernelArg(kernelColFull, 7, sizeof(cl_int), (void *)&dstWidth));
  CHECK(clSetKernelArg(kernelColFull, 8, sizeof(cl_int), (void *)&dstHeight));
  CHECK(clSetKernelArg(kernelColFull, 9, local_src_size, (void *)NULL));
  CHECK(clSetKernelArg(kernelColFull, 10, local_src_Col_size, (void *)NULL));
  CHECK(clSetKernelArg(kernelColFull, 11, local_dst_size, (void *)NULL));

  CHECK(clEnqueueNDRangeKernel(queue, kernelColFull, 2, NULL, global, local, 0,
                               NULL, &event));

  CHECK(clFinish(queue));
  CHECK(clEnqueueReadBuffer(queue, clDst_ColFull, CL_TRUE, 0,
                            (sizeof(cl_int) * DstPitch * dstHeight), Dst, 0,
                            NULL, NULL));

  CHECK(clFinish(queue));

  std::cout << "done!!" << std::endl;
}

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
  XI_TILE_SET_TYPE((&dst_t), XI_TILE_U32);
  XI_TILE_SET_EDGE_WIDTH((&dst_t), 0);
  XI_TILE_SET_EDGE_HEIGHT((&dst_t), 0);
  XI_TILE_SET_X_COORD((&dst_t), 0);
  XI_TILE_SET_Y_COORD((&dst_t), 0);

  xiIntegral_U8U32_ref((xi_pArray)&src_t, (xi_pArray)&dst_t);
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

  launch_ocl_Integral(gsrc, srcStride, gdst, dstWidth, srcWidth, srcHeight,
                      dstWidth, dstHeight);

  run_reference(gsrc, srcStride, gdst_ref, dstWidth, srcWidth, srcHeight,
                dstWidth, dstHeight);

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
