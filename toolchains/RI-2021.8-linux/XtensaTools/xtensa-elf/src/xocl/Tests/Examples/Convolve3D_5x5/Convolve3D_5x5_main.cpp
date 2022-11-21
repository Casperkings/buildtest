#include "utils.h"
#include "xi_api_ref.h"
#include "xi_tile3d_manager.h"
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
#define IMAGE_D 64
#define IMAGE_W 20
#define IMAGE_H 10
#define SRC_BIT_DEPTH (8 * IMAGE_D)
#define DST_BIT_DEPTH (8 * IMAGE_D)
#define TILE_W 10
#define TILE_H 5
#define W_EXT 2
#define H_EXT 2
#define IMAGE_PAD 2

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

void conv5x5xD8b_Ref(xi_pTile3D pSrc, xi_pTile3D pDst,
                     int8_t *__restrict pCoeff, int shift);

void launch_ocl_kernel(int8_t *Src, int8_t *Coef, uint32_t SrcPitch,
                       int8_t *Dst, const uint32_t DstPitch, int32_t width,
                       int32_t height, int32_t dstWidth, int32_t dstHeight,
                       int32_t depth, int32_t shift, int32_t srcBytes,
                       int32_t dstBytes) {
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
                     (srcBytes * (SrcPitch * (height + IMAGE_PAD + IMAGE_PAD))),
                     NULL, &status);
  CHECK_ERROR(status);

  cl_mem clDst =
      clCreateBuffer(context, CL_MEM_WRITE_ONLY,
                     (dstBytes * DstPitch * dstHeight), NULL, &status);
  CHECK_ERROR(status);

  cl_mem clFiltercoef = clCreateBuffer(context, CL_MEM_READ_ONLY,
                                       (5 * 5 * depth * depth), NULL, &status);
  CHECK_ERROR(status);
  cl_event event = clCreateUserEvent(context, &status);
  CHECK_ERROR(status);

  const char *progSource = loadKernel("Convolve3D_5x5.cl");
  cl_program program =
      clCreateProgramWithSource(context, 1, &progSource, NULL, &status);
  CHECK_ERROR(status);

  status = clBuildProgram(program, 1, devices, NULL, NULL, NULL);
  CHECK_ERROR(status);

  std::cout << SrcPitch << "  " << DstPitch << "  " << width << "  " << height
            << "  " << dstWidth << "  " << dstHeight << std::endl;

  int local_src_size;
  int local_dst_size;
  int local_coef_size;
  local_src_size = ((TILE_W + IMAGE_PAD + IMAGE_PAD) *
                    (TILE_H + IMAGE_PAD + IMAGE_PAD) * srcBytes);
  local_dst_size = ((TILE_W) * (TILE_H)*dstBytes);

  local_coef_size = 5 * 5 * depth * depth;

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
      (srcBytes * (SrcPitch * (height + IMAGE_PAD + IMAGE_PAD))), Src, 0, NULL,
      NULL);
  CHECK_ERROR(error);
  error = clEnqueueWriteBuffer(queue, clFiltercoef, CL_TRUE, 0,
                               (5 * 5 * depth * depth), Coef, 0, NULL, NULL);
  CHECK_ERROR(error);

  CHECK(clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&clSrc));
  CHECK(clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&clFiltercoef));
  CHECK(clSetKernelArg(kernel, 2, sizeof(cl_int), (void *)&SrcPitch));
  CHECK(clSetKernelArg(kernel, 3, sizeof(cl_mem), (void *)&clDst));
  CHECK(clSetKernelArg(kernel, 4, sizeof(cl_int), (void *)&DstPitch));
  CHECK(clSetKernelArg(kernel, 5, sizeof(cl_int), (void *)&width));
  CHECK(clSetKernelArg(kernel, 6, sizeof(cl_int), (void *)&height));
  CHECK(clSetKernelArg(kernel, 7, sizeof(cl_int), (void *)&dstWidth));
  CHECK(clSetKernelArg(kernel, 8, sizeof(cl_int), (void *)&dstHeight));
  CHECK(clSetKernelArg(kernel, 9, sizeof(cl_int), (void *)&depth));
  CHECK(clSetKernelArg(kernel, 10, sizeof(cl_int), (void *)&shift));
  CHECK(clSetKernelArg(kernel, 11, local_src_size, (void *)NULL));
  CHECK(clSetKernelArg(kernel, 12, local_dst_size, (void *)NULL));
  CHECK(clSetKernelArg(kernel, 13, local_coef_size, (void *)NULL));
  CHECK(clSetKernelArg(kernel, 14, sizeof(cl_mem), (void *)&clErrCodes));

  CHECK(clEnqueueNDRangeKernel(queue, kernel, 2, NULL, global, local, 0, NULL,
                               &event));

  CHECK(clFinish(queue));
  CHECK(clEnqueueReadBuffer(queue, clDst, CL_TRUE, 0,
                            (dstBytes * DstPitch * dstHeight), Dst, 0, NULL,
                            NULL));
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

void run_reference(int8_t *Src, int8_t *Coef, uint32_t SrcPitch, int8_t *Dst,
                   const uint32_t DstPitch, int32_t width, int32_t height,
                   int32_t dstWidth, int32_t dstHeight, int32_t depth,
                   int32_t shift, int32_t srcBytes, int32_t dstBytes) {
  xi_tile3D src_t;
  xi_tile3D dst_t;

  // Input
  XI_TILE3D_SET_DIM1(&src_t, width + IMAGE_PAD + IMAGE_PAD);
  XI_TILE3D_SET_DIM2(&src_t, height + IMAGE_PAD + IMAGE_PAD);
  XI_TILE3D_SET_DIM3(&src_t, depth);

  XI_TILE3D_SET_DIM1_PITCH(&src_t, SrcPitch);
  XI_TILE3D_SET_DATA_PTR(&src_t, (void *)Src);

  // Output_Ref
  XI_TILE3D_SET_DIM1(&dst_t, dstWidth);
  XI_TILE3D_SET_DIM2(&dst_t, dstHeight);
  XI_TILE3D_SET_DIM3(&dst_t, depth);

  XI_TILE3D_SET_DIM1_PITCH(&dst_t, DstPitch);
  XI_TILE3D_SET_DATA_PTR(&dst_t, (void *)Dst);

  conv5x5xD8b_Ref(&src_t, &dst_t, Coef, shift);
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

int my_rand()
{
  static unsigned int nSeed = 5323;
  nSeed = (8253729 * nSeed + 2396403);
  return nSeed % ((long long)RAND_MAX+1);
}

int main() {
  // Grey-scale frame
  int8_t *gsrc;
  int8_t *gcoef;
  int8_t *gdst;
  int8_t *gdst_ref;

  // Image dimensions and bit depth
  int32_t srcWidth, srcHeight, srcBytes;
  int32_t dstWidth, dstHeight, dstBytes;
  int32_t srcStride;

  int i, j, k, l;
  // Set frame dimensions
  srcWidth = IMAGE_W;
  srcHeight = IMAGE_H;
  srcStride = (IMAGE_W + IMAGE_PAD + IMAGE_PAD);
  srcBytes = SRC_BIT_DEPTH >> 3;
  dstWidth = srcWidth;
  dstHeight = srcHeight;
  dstBytes = DST_BIT_DEPTH >> 3;

  // Grey-scale I/O frame buffers
  gsrc = (int8_t *)malloc(srcStride * (srcHeight + IMAGE_PAD + IMAGE_PAD) *
                          srcBytes);
  gcoef = (int8_t *)malloc(5 * 5 * IMAGE_D * IMAGE_D);
  gdst = (int8_t *)malloc(dstWidth * dstHeight * dstBytes);
  gdst_ref = (int8_t *)malloc(dstWidth * dstHeight * dstBytes);
  if (!gsrc || !gdst || !gdst_ref || !gcoef) {
    fprintf(stderr, "Unable to allocate memory\n");
    exit(0);
  }
  
  // Read input
  char val = 1;
  for (i = 0; i < IMAGE_D; i++) {
    for (j = 0; j < IMAGE_H + (2*IMAGE_PAD); j++) {
      for (k = 0; k < IMAGE_W + (2*IMAGE_PAD); k++) {

        gsrc[(IMAGE_W + IMAGE_PAD * 2) * IMAGE_D * j + 
                                         IMAGE_D * k + 
                                                   i] = val * (my_rand() % 16);
      }
    }
    val = -val;
  }

  int sum[IMAGE_D];
  for (l = 0; l < IMAGE_D; l++) {
    sum[l] = 0;
  }
  for (i = 0; i < IMAGE_D; i++) {
    char val = 1;
    for (j = 0; j < 5; j++) {
      for (k = 0; k < 5; k++) {
        for (l = 0; l < IMAGE_D; l++) {
          gcoef[i * 5 * 5 * IMAGE_D + j * 5 * IMAGE_D + k * IMAGE_D + l] =
              val * (my_rand() %
                     16); //((j==2)&&(k==2))?l:0; val*(j*5+k); //l%4; //val; //
          sum[l] +=
              gcoef[i * 5 * 5 * IMAGE_D + j * 5 * IMAGE_D + k * IMAGE_D + l];
        }
        val = val * (-1);
      }
    }
  }

  for (i = 0; i < IMAGE_D; i++) {
    for (j = 0; j < 5; j++) {
      for (k = 0; k < 5; k++) {
        for (l = 0; l < IMAGE_D; l++) {
          float scale = (float)63.0 / sum[l];
          gcoef[i * 5 * 5 * IMAGE_D + j * 5 * IMAGE_D + k * IMAGE_D + l] =
              (float)gcoef[i * 5 * 5 * IMAGE_D + j * 5 * IMAGE_D + k * IMAGE_D +
                           l] *
              scale;
          // printf("%d,",gcoef[i*5*5*OUTDEPTH+j*5*OUTDEPTH+k*OUTDEPTH+l]);
        }
        // printf("\n");
      }
    }
  }
  int shift = 7;
  int depth = IMAGE_D;
  launch_ocl_kernel(gsrc, gcoef, srcStride, gdst, dstWidth, srcWidth, srcHeight,
                    dstWidth, dstHeight, depth, shift, srcBytes, dstBytes);

  run_reference(gsrc, gcoef, srcStride, gdst_ref, dstWidth, srcWidth, srcHeight,
                dstWidth, dstHeight, depth, shift, srcBytes, dstBytes);

  FILE *fpOut;
  char fname[200];
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
  int pass = 1;
  for (int i = 0; i < dstWidth * dstHeight * dstBytes; ++i) {
    if (gdst_ref[i] != gdst[i]) {
      printf("Difference found at %d, expected: %x, got: %x\n", i, 
             gdst_ref[i], gdst[i]);
      pass = 0;
      break;
    }
  }
  if (pass)
    printf("PASS \n");
  else
    printf("FAIL \n");

  return 0;
}
