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

#define DEBUG_DUMP 0
/* Input/Output dimensions     */
#define IMAGE_W 512
#define IMAGE_H 512
#define SRC_BIT_DEPTH 8
#define DST_BIT_DEPTH 8
#define TILE_W 128
#define TILE_H 64
#define W_EXT 1
#define H_EXT 1
#define IMAGE_PAD 1

#define CHECK(API)                                                             \
  do {                                                                         \
    status = (API);                                                            \
    if (status != CL_SUCCESS) {                                                \
      printf("Failed at #%d, status=%s\n", __LINE__, clGetErrorString(status));\
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

#define CHECK_ERROR(err)                                                       \
  do {                                                                         \
    if (err != CL_SUCCESS) {                                                   \
      printf("Failed at #%d, err=%s\n", __LINE__, clGetErrorString(err));      \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

XI_ERR_TYPE xiLocalLaplacian_Gray_U8_FixPt_ref(const xi_pTile src, xi_pTile dst,
                                               int levels, int pyramid_levels,
                                               int16_t *remap_table);

extern short remap_table[8 * 256];

unsigned char temp[514 * 514 * 2 * 8];

#define CHECK_COMMAND_STATUS(event) \
{\
  int evt_status; \
  clGetEventInfo(event, CL_EVENT_COMMAND_EXECUTION_STATUS, sizeof(evt_status), \
                 &evt_status, NULL); \
  CHECK_ERROR(evt_status); \
}\

#define PAD IMAGE_PAD
void launch_ocl_kernel(uint8_t *Src, uint32_t SrcPitch, uint8_t *Dst,
                       const uint32_t DstPitch, int32_t width, int32_t height,
                       int32_t dstWidth, int32_t dstHeight,
                       int16_t *remap_table) {
  cl_int status;
  int inPitch;
  int inWidth, inHeight, inEdge;
  int inTileW, inTileH, inTileEdge;

  int in1Pitch;
  int in1Width, in1Height, in1Edge;
  int in1TileW, in1TileH, in1TileEdge;

  int outPitch;
  int outWidth, outHeight, outEdge;
  int outTileW, outTileH, outTileEdge;

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

  cl_mem clSrc0 = clCreateBuffer(
      context, CL_MEM_READ_ONLY,
      (sizeof(uint8_t) * (SrcPitch * (height + IMAGE_PAD + IMAGE_PAD))), NULL,
      &status);
  CHECK_ERROR(status);

  cl_mem clremap_table = clCreateBuffer(
      context, CL_MEM_READ_ONLY, (sizeof(int16_t) * (8 * 256)), NULL, &status);
  CHECK_ERROR(status);

  cl_mem clSrc1 =
      clCreateBuffer(context, CL_MEM_READ_WRITE,
                     (sizeof(uint8_t) * (((width / 2) + 2 * IMAGE_PAD) *
                                         ((height / 2) + 2 * IMAGE_PAD))),
                     NULL, &status);
  CHECK_ERROR(status);

  cl_mem clSrc2 =
      clCreateBuffer(context, CL_MEM_READ_WRITE,
                     (sizeof(uint8_t) * (((width / 4) + 2 * IMAGE_PAD) *
                                         ((height / 4) + 2 * IMAGE_PAD))),
                     NULL, &status);
  CHECK_ERROR(status);

  cl_mem clGPyr0 = clCreateBuffer(
      context, CL_MEM_READ_WRITE,
      (sizeof(int16_t) * ((width + 2 * PAD) * (height + 2 * PAD) * 8)), NULL,
      &status);
  CHECK_ERROR(status);

  cl_mem clGPyr1 =
      clCreateBuffer(context, CL_MEM_READ_WRITE,
                     (sizeof(int16_t) *
                      (((width / 2) + 2 * PAD) * ((height / 2) + 2 * PAD) * 8)),
                     NULL, &status);
  CHECK_ERROR(status);

  cl_mem clGLPyr2 =
      clCreateBuffer(context, CL_MEM_READ_WRITE,
                     (sizeof(int16_t) *
                      (((width / 4) + 2 * PAD) * ((height / 4) + 2 * PAD) * 8)),
                     NULL, &status);
  CHECK_ERROR(status);

  cl_mem clLPyr0 = clCreateBuffer(
      context, CL_MEM_READ_WRITE,
      (sizeof(int16_t) * ((width + 2 * PAD) * (height + 2 * PAD) * 8)), NULL,
      &status);
  CHECK_ERROR(status);

  cl_mem clLPyr1 =
      clCreateBuffer(context, CL_MEM_READ_WRITE,
                     (sizeof(int16_t) *
                      (((width / 2) + 2 * PAD) * ((height / 2) + 2 * PAD) * 8)),
                     NULL, &status);
  CHECK_ERROR(status);

  cl_mem clL0 = clCreateBuffer(
      context, CL_MEM_READ_WRITE,
      (sizeof(int16_t) * ((width + 2 * PAD) * (height + 2 * PAD))), NULL,
      &status);
  CHECK_ERROR(status);

  cl_mem clL1 = clCreateBuffer(
      context, CL_MEM_READ_WRITE,
      (sizeof(int16_t) * (((width / 2) + 2 * PAD) * ((height / 2) + 2 * PAD))),
      NULL, &status);
  CHECK_ERROR(status);

  cl_mem clL2 = clCreateBuffer(
      context, CL_MEM_READ_WRITE,
      (sizeof(int16_t) * (((width / 4) + 2 * PAD) * ((height / 4) + 2 * PAD))),
      NULL, &status);
  CHECK_ERROR(status);

  cl_mem clDst1 = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                 (sizeof(int16_t) * ((dstWidth / 2) + 2 * PAD) *
                                  ((dstHeight / 2) + 2 * PAD)),
                                 NULL, &status);
  CHECK_ERROR(status);

  cl_mem clDst0 =
      clCreateBuffer(context, CL_MEM_WRITE_ONLY,
                     (sizeof(uint8_t) * DstPitch * dstHeight), NULL, &status);
  CHECK_ERROR(status);

  cl_event event = clCreateUserEvent(context, &status);
  CHECK_ERROR(status);

  const char *progSource = loadKernel("LocalLaplacian.cl");
  cl_program program =
      clCreateProgramWithSource(context, 1, &progSource, NULL, &status);
  CHECK_ERROR(status);

  status = clBuildProgram(program, 1, devices, NULL, NULL, NULL);
  CHECK_ERROR(status);

  std::cout << SrcPitch << "  " << DstPitch << "  " << width << "  " << height
            << "  " << dstWidth << "  " << dstHeight << std::endl;

  std::cout << " SrcPitch = " << SrcPitch << "\n";
  std::cout << " DstPitch = " << DstPitch << "\n";

  size_t local[2] = {1, 1};
  size_t global[2] = {(size_t)(height / TILE_H), (size_t)(width / TILE_W)};
  std::cout << "local work items " << local[0] << " X " << local[1] << "\n";

  // VI
  cl_kernel InputGaussian =
      clCreateKernel(program, "ocl_InputGaussian", &status);
  CHECK_ERROR(status);
  // I
  cl_kernel ContrastLevel =
      clCreateKernel(program, "ocl_ContrastLevel", &status);
  CHECK_ERROR(status);
  // II
  cl_kernel GaussianPyr = clCreateKernel(program, "ocl_GaussianPyr", &status);
  CHECK_ERROR(status);
  // III
  cl_kernel LaplacianPyr = clCreateKernel(program, "ocl_LaplacianPyr", &status);
  CHECK_ERROR(status);
  // IV
  cl_kernel FuseLaplacianPyr =
      clCreateKernel(program, "ocl_FuseLaplacianPyr", &status);
  CHECK_ERROR(status);
  // V
  cl_kernel FusePyr = clCreateKernel(program, "ocl_FusedPyrUpSample", &status);
  CHECK_ERROR(status);

  cl_kernel FusePyr1 =
      clCreateKernel(program, "ocl_FusedPyrUpSampleClip", &status);
  CHECK_ERROR(status);

  error = clEnqueueWriteBuffer(
      queue, clSrc0, CL_TRUE, 0,
      (sizeof(uint8_t) * (SrcPitch * (height + IMAGE_PAD + IMAGE_PAD))), Src, 0,
      NULL, NULL);
  CHECK_ERROR(error);

  error = clEnqueueWriteBuffer(queue, clremap_table, CL_TRUE, 0,
                               (sizeof(int16_t) * (8 * 256)), remap_table, 0,
                               NULL, NULL);
  CHECK_ERROR(error);
  /*Down sample & Gaussian of input image to level 1 (1)*/

  inPitch = SrcPitch;
  inWidth = width;
  inHeight = height;
  inEdge = PAD;
  inTileW = (TILE_W);
  inTileH = TILE_H;
  inTileEdge = PAD;

  outPitch = (width / 2) + 2 * PAD;
  outWidth = width / 2;
  outHeight = height / 2;
  outEdge = PAD;
  outTileW = (TILE_W >> 1);
  outTileH = TILE_H >> 1;
  outTileEdge = PAD;

  CHECK(clSetKernelArg(InputGaussian, 0, sizeof(cl_mem), (void *)&clSrc0));
  CHECK(clSetKernelArg(InputGaussian, 1, sizeof(cl_int), (void *)&inPitch));
  CHECK(clSetKernelArg(InputGaussian, 2, sizeof(cl_int), (void *)&inEdge));
  CHECK(clSetKernelArg(InputGaussian, 3, sizeof(cl_int), (void *)&inWidth));
  CHECK(clSetKernelArg(InputGaussian, 4, sizeof(cl_int), (void *)&inHeight));
  CHECK(clSetKernelArg(InputGaussian, 5, sizeof(cl_int), (void *)&inTileW));
  CHECK(clSetKernelArg(InputGaussian, 6, sizeof(cl_int), (void *)&inTileH));
  CHECK(clSetKernelArg(InputGaussian, 7, sizeof(cl_int), (void *)&inTileEdge));
  CHECK(clSetKernelArg(InputGaussian, 8, sizeof(cl_mem), (void *)&clSrc1));
  CHECK(clSetKernelArg(InputGaussian, 9, sizeof(cl_int), (void *)&outPitch));
  CHECK(clSetKernelArg(InputGaussian, 10, sizeof(cl_int), (void *)&outEdge));
  CHECK(clSetKernelArg(InputGaussian, 11, sizeof(cl_int), (void *)&outWidth));
  CHECK(clSetKernelArg(InputGaussian, 12, sizeof(cl_int), (void *)&outHeight));
  CHECK(clSetKernelArg(InputGaussian, 13, sizeof(cl_int), (void *)&outTileW));
  CHECK(clSetKernelArg(InputGaussian, 14, sizeof(cl_int), (void *)&outTileH));
  CHECK(
      clSetKernelArg(InputGaussian, 15, sizeof(cl_int), (void *)&outTileEdge));
  CHECK(clSetKernelArg(InputGaussian, 16,
                       (inTileW + 2 * inTileEdge) * (inTileH + 2 * inTileEdge),
                       (void *)NULL));
  CHECK(clSetKernelArg(InputGaussian, 17,
                       (outTileH + 2 * outTileEdge) *
                           (outTileW + 2 * outTileEdge),
                       (void *)NULL));

  global[0] = (size_t)(inHeight / inTileH);
  global[1] = (size_t)(inWidth / inTileW);
  std::cout << " InputGaussian global work items " << global[0] << " X "
            << global[1] << "  " << inTileW << " X " << inTileH << "\n";
  CHECK(clEnqueueNDRangeKernel(queue, InputGaussian, 2, NULL, global, local, 0,
                               NULL, &event));
#if DEBUG_DUMP
  {
    CHECK(clEnqueueReadBuffer(queue, clSrc1, CL_TRUE, 0,
                              outPitch * (outHeight + 2 * outEdge), temp, 0,
                              NULL, NULL));

    CHECK(clFinish(queue));

    FILE *fp;
    fp = fopen("Graylevel1.raw", "wb");
    fwrite(temp, 1, outPitch * (outHeight + 2 * outEdge), fp);
    fclose(fp);
  }
#endif
  /*Down sample & Gaussian of input image to level 2 (2)*/
  inPitch = (width / 2) + 2 * PAD;
  inWidth = width >> 1;
  inHeight = height >> 1;
  inEdge = PAD;
  inTileW = (TILE_W);
  inTileH = TILE_H;
  inTileEdge = PAD;

  outPitch = (width / 4) + 2 * PAD;
  outWidth = width / 4;
  outHeight = height / 4;
  outEdge = PAD;
  outTileW = (TILE_W >> 1);
  outTileH = TILE_H >> 1;
  outTileEdge = PAD;

  CHECK(clSetKernelArg(InputGaussian, 0, sizeof(cl_mem), (void *)&clSrc1));
  CHECK(clSetKernelArg(InputGaussian, 1, sizeof(cl_int), (void *)&inPitch));
  CHECK(clSetKernelArg(InputGaussian, 2, sizeof(cl_int), (void *)&inEdge));
  CHECK(clSetKernelArg(InputGaussian, 3, sizeof(cl_int), (void *)&inWidth));
  CHECK(clSetKernelArg(InputGaussian, 4, sizeof(cl_int), (void *)&inHeight));
  CHECK(clSetKernelArg(InputGaussian, 5, sizeof(cl_int), (void *)&inTileW));
  CHECK(clSetKernelArg(InputGaussian, 6, sizeof(cl_int), (void *)&inTileH));
  CHECK(clSetKernelArg(InputGaussian, 7, sizeof(cl_int), (void *)&inTileEdge));
  CHECK(clSetKernelArg(InputGaussian, 8, sizeof(cl_mem), (void *)&clSrc2));
  CHECK(clSetKernelArg(InputGaussian, 9, sizeof(cl_int), (void *)&outPitch));
  CHECK(clSetKernelArg(InputGaussian, 10, sizeof(cl_int), (void *)&outEdge));
  CHECK(clSetKernelArg(InputGaussian, 11, sizeof(cl_int), (void *)&outWidth));
  CHECK(clSetKernelArg(InputGaussian, 12, sizeof(cl_int), (void *)&outHeight));
  CHECK(clSetKernelArg(InputGaussian, 13, sizeof(cl_int), (void *)&outTileW));
  CHECK(clSetKernelArg(InputGaussian, 14, sizeof(cl_int), (void *)&outTileH));
  CHECK(
      clSetKernelArg(InputGaussian, 15, sizeof(cl_int), (void *)&outTileEdge));
  CHECK(clSetKernelArg(InputGaussian, 16,
                       (inTileW + 2 * inTileEdge) * (inTileH + 2 * inTileEdge),
                       (void *)NULL));
  CHECK(clSetKernelArg(InputGaussian, 17,
                       (outTileH + 2 * outTileEdge) *
                           (outTileW + 2 * outTileEdge),
                       (void *)NULL));

  global[0] = (size_t)(inHeight / inTileH);
  global[1] = (size_t)(inWidth / inTileW);
  std::cout << " InputGaussian global work items " << global[0] << " X "
            << global[1] << "  " << inTileW << " X " << inTileH << "\n";
  CHECK(clEnqueueNDRangeKernel(queue, InputGaussian, 2, NULL, global, local, 0,
                               NULL, &event));

  CHECK(clFinish(queue));
#if DEBUG_DUMP
  {
    CHECK(clEnqueueReadBuffer(queue, clSrc2, CL_TRUE, 0,
                              outPitch * (outHeight + 2 * outEdge), temp, 0,
                              NULL, NULL));

    CHECK(clFinish(queue));

    FILE *fp;
    fp = fopen("Graylevel2.raw", "wb");
    fwrite(temp, 1, outPitch * (outHeight + 2 * outEdge), fp);
    fclose(fp);
  }
#endif
  /*Contrast level generation interleaved by 8 (3)*/
  inPitch = (width) + 2 * PAD;
  inWidth = width;
  inHeight = height;
  inEdge = PAD;
  inTileW = TILE_W;
  inTileH = TILE_H;
  inTileEdge = 0;

  outPitch = ((width) + 2 * PAD);
  outWidth = width;
  outHeight = height;
  outEdge = PAD;
  outTileW = TILE_W;
  outTileH = TILE_H;
  outTileEdge = PAD;

  CHECK(clSetKernelArg(ContrastLevel, 0, sizeof(cl_mem), (void *)&clSrc0));
  CHECK(clSetKernelArg(ContrastLevel, 1, sizeof(cl_int), (void *)&inPitch));
  CHECK(clSetKernelArg(ContrastLevel, 2, sizeof(cl_int), (void *)&inEdge));
  CHECK(clSetKernelArg(ContrastLevel, 3, sizeof(cl_int), (void *)&inWidth));
  CHECK(clSetKernelArg(ContrastLevel, 4, sizeof(cl_int), (void *)&inHeight));
  CHECK(clSetKernelArg(ContrastLevel, 5, sizeof(cl_int), (void *)&inTileW));
  CHECK(clSetKernelArg(ContrastLevel, 6, sizeof(cl_int), (void *)&inTileH));
  CHECK(clSetKernelArg(ContrastLevel, 7, sizeof(cl_int), (void *)&inTileEdge));
  CHECK(clSetKernelArg(ContrastLevel, 8, sizeof(cl_mem), (void *)&clGPyr0));
  CHECK(clSetKernelArg(ContrastLevel, 9, sizeof(cl_int), (void *)&outPitch));
  CHECK(clSetKernelArg(ContrastLevel, 10, sizeof(cl_int), (void *)&outEdge));
  CHECK(clSetKernelArg(ContrastLevel, 11, sizeof(cl_int), (void *)&outWidth));
  CHECK(clSetKernelArg(ContrastLevel, 12, sizeof(cl_int), (void *)&outHeight));
  CHECK(clSetKernelArg(ContrastLevel, 13, sizeof(cl_int), (void *)&outTileW));
  CHECK(clSetKernelArg(ContrastLevel, 14, sizeof(cl_int), (void *)&outTileH));
  CHECK(
      clSetKernelArg(ContrastLevel, 15, sizeof(cl_int), (void *)&outTileEdge));
  CHECK(clSetKernelArg(ContrastLevel, 16, sizeof(cl_mem),
                       (void *)&clremap_table));
  CHECK(clSetKernelArg(ContrastLevel, 17, 8 * 256 * sizeof(int16_t),
                       (void *)NULL));
  CHECK(clSetKernelArg(ContrastLevel, 18,
                       (inTileW + 2 * inTileEdge) * (inTileH + 2 * inTileEdge),
                       (void *)NULL));
  CHECK(clSetKernelArg(ContrastLevel, 19,
                       (outTileH + 2 * outTileEdge) *
                           (outTileW + 2 * outTileEdge) * 8 * sizeof(int16_t),
                       (void *)NULL));

  global[0] = (size_t)(inHeight / inTileH);
  global[1] = (size_t)(inWidth / inTileW);
  std::cout << " ContrastLevel global work items " << global[0] << " X "
            << global[1] << "  " << inTileW << " X " << inTileH << "\n";
  CHECK(clEnqueueNDRangeKernel(queue, ContrastLevel, 2, NULL, global, local, 0,
                               NULL, &event));

  CHECK(clFinish(queue));
  CHECK_COMMAND_STATUS(event);
#if DEBUG_DUMP
  {
    CHECK(clEnqueueReadBuffer(queue, clGPyr0, CL_TRUE, 0,
                              outPitch * (outHeight + 2 * outEdge) * 8 *
                                  sizeof(int16_t),
                              temp, 0, NULL, NULL));

    CHECK(clFinish(queue));
    FILE *fp;
    char fname[256];
    for (int k = 0; k < 8; k++) {
      sprintf(fname, "GPyrLevel0_%d.yuv", k);
      fp = fopen(fname, "wb");
      for (int y = 0; y < outPitch * (outHeight + 2 * outEdge); y++) {
        fwrite(&temp[(y * 8 + k) * 2], 2, 1, fp);
      }
      fclose(fp);
    }
  }
#endif
  /*Generation of Gaussian pyr level 1 (4)*/
  inPitch = (width) + 2 * PAD;
  inWidth = width;
  inHeight = height;
  inEdge = PAD;
  inTileW = TILE_W;
  inTileH = TILE_H / 2;
  inTileEdge = PAD;

  outPitch = ((width / 2) + 2 * PAD);
  outWidth = width / 2;
  outHeight = height / 2;
  outEdge = PAD;
  outTileW = TILE_W / 2;
  outTileH = TILE_H / 4;
  outTileEdge = PAD;

  CHECK(clSetKernelArg(GaussianPyr, 0, sizeof(cl_mem), (void *)&clGPyr0));
  CHECK(clSetKernelArg(GaussianPyr, 1, sizeof(cl_int), (void *)&inPitch));
  CHECK(clSetKernelArg(GaussianPyr, 2, sizeof(cl_int), (void *)&inEdge));
  CHECK(clSetKernelArg(GaussianPyr, 3, sizeof(cl_int), (void *)&inWidth));
  CHECK(clSetKernelArg(GaussianPyr, 4, sizeof(cl_int), (void *)&inHeight));
  CHECK(clSetKernelArg(GaussianPyr, 5, sizeof(cl_int), (void *)&inTileW));
  CHECK(clSetKernelArg(GaussianPyr, 6, sizeof(cl_int), (void *)&inTileH));
  CHECK(clSetKernelArg(GaussianPyr, 7, sizeof(cl_int), (void *)&inTileEdge));
  CHECK(clSetKernelArg(GaussianPyr, 8, sizeof(cl_mem), (void *)&clGPyr1));
  CHECK(clSetKernelArg(GaussianPyr, 9, sizeof(cl_int), (void *)&outPitch));
  CHECK(clSetKernelArg(GaussianPyr, 10, sizeof(cl_int), (void *)&outEdge));
  CHECK(clSetKernelArg(GaussianPyr, 11, sizeof(cl_int), (void *)&outWidth));
  CHECK(clSetKernelArg(GaussianPyr, 12, sizeof(cl_int), (void *)&outHeight));
  CHECK(clSetKernelArg(GaussianPyr, 13, sizeof(cl_int), (void *)&outTileW));
  CHECK(clSetKernelArg(GaussianPyr, 14, sizeof(cl_int), (void *)&outTileH));
  CHECK(clSetKernelArg(GaussianPyr, 15, sizeof(cl_int), (void *)&outTileEdge));
  CHECK(clSetKernelArg(GaussianPyr, 16,
                       (inTileW + 2 * inTileEdge) * (inTileH + 2 * inTileEdge) *
                           8 * sizeof(int16_t),
                       (void *)NULL));
  CHECK(clSetKernelArg(GaussianPyr, 17,
                       (outTileH + 2 * outTileEdge) *
                           (outTileW + 2 * outTileEdge) * 8 * sizeof(int16_t),
                       (void *)NULL));

  global[0] = (size_t)(inHeight / inTileH);
  global[1] = (size_t)(inWidth / inTileW);
  std::cout << " GaussianPyr global work items " << global[0] << " X "
            << global[1] << "  " << inTileW << " X " << inTileH << "\n";
  CHECK(clEnqueueNDRangeKernel(queue, GaussianPyr, 2, NULL, global, local, 0,
                               NULL, &event));
  CHECK(clFinish(queue));
#if DEBUG_DUMP
  {
    CHECK(clEnqueueReadBuffer(queue, clGPyr1, CL_TRUE, 0,
                              outPitch * (outHeight + 2 * outEdge) * 8 *
                                  sizeof(int16_t),
                              temp, 0, NULL, NULL));

    CHECK(clFinish(queue));
    FILE *fp;
    char fname[256];
    for (int k = 0; k < 8; k++) {
      sprintf(fname, "GPyrLevel1_%d.yuv", k);
      fp = fopen(fname, "wb");
      for (int y = 0; y < outPitch * (outHeight + 2 * outEdge); y++) {
        fwrite(&temp[(y * 8 + k) * 2], 2, 1, fp);
      }
      fclose(fp);
    }
  }
#endif
  /*Generation of Gaussian pyr level 2 (5)*/
  inPitch = (width / 2) + 2 * PAD;
  inWidth = width / 2;
  inHeight = height / 2;
  inEdge = PAD;
  inTileW = TILE_W;
  inTileH = TILE_H / 2;
  inTileEdge = PAD;

  outPitch = ((width / 4) + 2 * PAD);
  outWidth = width / 4;
  outHeight = height / 4;
  outEdge = PAD;
  outTileW = TILE_W / 2;
  outTileH = TILE_H / 4;
  outTileEdge = PAD;

  CHECK(clSetKernelArg(GaussianPyr, 0, sizeof(cl_mem), (void *)&clGPyr1));
  CHECK(clSetKernelArg(GaussianPyr, 1, sizeof(cl_int), (void *)&inPitch));
  CHECK(clSetKernelArg(GaussianPyr, 2, sizeof(cl_int), (void *)&inEdge));
  CHECK(clSetKernelArg(GaussianPyr, 3, sizeof(cl_int), (void *)&inWidth));
  CHECK(clSetKernelArg(GaussianPyr, 4, sizeof(cl_int), (void *)&inHeight));
  CHECK(clSetKernelArg(GaussianPyr, 5, sizeof(cl_int), (void *)&inTileW));
  CHECK(clSetKernelArg(GaussianPyr, 6, sizeof(cl_int), (void *)&inTileH));
  CHECK(clSetKernelArg(GaussianPyr, 7, sizeof(cl_int), (void *)&inTileEdge));
  CHECK(clSetKernelArg(GaussianPyr, 8, sizeof(cl_mem), (void *)&clGLPyr2));
  CHECK(clSetKernelArg(GaussianPyr, 9, sizeof(cl_int), (void *)&outPitch));
  CHECK(clSetKernelArg(GaussianPyr, 10, sizeof(cl_int), (void *)&outEdge));
  CHECK(clSetKernelArg(GaussianPyr, 11, sizeof(cl_int), (void *)&outWidth));
  CHECK(clSetKernelArg(GaussianPyr, 12, sizeof(cl_int), (void *)&outHeight));
  CHECK(clSetKernelArg(GaussianPyr, 13, sizeof(cl_int), (void *)&outTileW));
  CHECK(clSetKernelArg(GaussianPyr, 14, sizeof(cl_int), (void *)&outTileH));
  CHECK(clSetKernelArg(GaussianPyr, 15, sizeof(cl_int), (void *)&outTileEdge));
  CHECK(clSetKernelArg(GaussianPyr, 16,
                       (inTileW + 2 * inTileEdge) * (inTileH + 2 * inTileEdge) *
                           8 * sizeof(int16_t),
                       (void *)NULL));
  CHECK(clSetKernelArg(GaussianPyr, 17,
                       (outTileH + 2 * outTileEdge) *
                           (outTileW + 2 * outTileEdge) * 8 * sizeof(int16_t),
                       (void *)NULL));

  global[0] = (size_t)(inHeight / inTileH);
  global[1] = (size_t)(inWidth / inTileW);
  std::cout << " GaussianPyr global work items " << global[0] << " X "
            << global[1] << "  " << inTileW << " X " << inTileH << "\n";
  CHECK(clEnqueueNDRangeKernel(queue, GaussianPyr, 2, NULL, global, local, 0,
                               NULL, &event));

  CHECK(clFinish(queue));
#if DEBUG_DUMP
  {
    CHECK(clEnqueueReadBuffer(queue, clGLPyr2, CL_TRUE, 0,
                              outPitch * (outHeight + 2 * outEdge) * 8 *
                                  sizeof(int16_t),
                              temp, 0, NULL, NULL));
    CHECK(clFinish(queue));
    FILE *fp;
    char fname[256];
    // Read input
    for (int k = 0; k < 8; k++) {
      sprintf(fname, "GPyrLevel2_%d.yuv", k);
      fp = fopen(fname, "wb");
      for (int y = 0; y < outPitch * (outHeight + 2 * outEdge); y++) {
        fwrite(&temp[(y * 8 + k) * 2], 2, 1, fp);
      }
      fclose(fp);
    }
  }
#endif
  /*Generation of laplacian pyr level 1 (6)*/
  inPitch = (width / 2) + 2 * PAD;
  inWidth = width / 2;
  inHeight = height / 2;
  inEdge = PAD;
  inTileW = TILE_W;
  inTileH = TILE_H / 2;
  inTileEdge = 0;

  in1Pitch = (width / 4) + 2 * PAD;
  in1Width = width / 4;
  in1Height = height / 4;
  in1Edge = PAD;
  in1TileW = TILE_W / 2;
  in1TileH = TILE_H / 4;
  in1TileEdge = PAD;

  outPitch = ((width / 2) + 2 * PAD);
  outWidth = width / 2;
  outHeight = height / 2;
  outEdge = PAD;
  outTileW = TILE_W;
  outTileH = TILE_H / 2;
  outTileEdge = PAD;

  CHECK(clSetKernelArg(LaplacianPyr, 0, sizeof(cl_mem), (void *)&clGPyr1));
  CHECK(clSetKernelArg(LaplacianPyr, 1, sizeof(cl_int), (void *)&inPitch));
  CHECK(clSetKernelArg(LaplacianPyr, 2, sizeof(cl_int), (void *)&inEdge));
  CHECK(clSetKernelArg(LaplacianPyr, 3, sizeof(cl_int), (void *)&inWidth));
  CHECK(clSetKernelArg(LaplacianPyr, 4, sizeof(cl_int), (void *)&inHeight));
  CHECK(clSetKernelArg(LaplacianPyr, 5, sizeof(cl_int), (void *)&inTileW));
  CHECK(clSetKernelArg(LaplacianPyr, 6, sizeof(cl_int), (void *)&inTileH));
  CHECK(clSetKernelArg(LaplacianPyr, 7, sizeof(cl_int), (void *)&inTileEdge));
  CHECK(clSetKernelArg(LaplacianPyr, 8, sizeof(cl_mem), (void *)&clGLPyr2));
  CHECK(clSetKernelArg(LaplacianPyr, 9, sizeof(cl_int), (void *)&in1Pitch));
  CHECK(clSetKernelArg(LaplacianPyr, 10, sizeof(cl_int), (void *)&in1Edge));
  CHECK(clSetKernelArg(LaplacianPyr, 11, sizeof(cl_int), (void *)&in1Width));
  CHECK(clSetKernelArg(LaplacianPyr, 12, sizeof(cl_int), (void *)&in1Height));
  CHECK(clSetKernelArg(LaplacianPyr, 13, sizeof(cl_int), (void *)&in1TileW));
  CHECK(clSetKernelArg(LaplacianPyr, 14, sizeof(cl_int), (void *)&in1TileH));
  CHECK(clSetKernelArg(LaplacianPyr, 15, sizeof(cl_int), (void *)&in1TileEdge));
  CHECK(clSetKernelArg(LaplacianPyr, 16, sizeof(cl_mem), (void *)&clLPyr1));
  CHECK(clSetKernelArg(LaplacianPyr, 17, sizeof(cl_int), (void *)&outPitch));
  CHECK(clSetKernelArg(LaplacianPyr, 18, sizeof(cl_int), (void *)&outEdge));
  CHECK(clSetKernelArg(LaplacianPyr, 19, sizeof(cl_int), (void *)&outWidth));
  CHECK(clSetKernelArg(LaplacianPyr, 20, sizeof(cl_int), (void *)&outHeight));
  CHECK(clSetKernelArg(LaplacianPyr, 21, sizeof(cl_int), (void *)&outTileW));
  CHECK(clSetKernelArg(LaplacianPyr, 22, sizeof(cl_int), (void *)&outTileH));
  CHECK(clSetKernelArg(LaplacianPyr, 23, sizeof(cl_int), (void *)&outTileEdge));
  CHECK(clSetKernelArg(LaplacianPyr, 24,
                       (inTileW + 2 * inTileEdge) * (inTileH + 2 * inTileEdge) *
                           8 * sizeof(int16_t),
                       (void *)NULL));
  CHECK(clSetKernelArg(LaplacianPyr, 25,
                       (in1TileW + 2 * in1TileEdge) *
                           (in1TileH + 2 * in1TileEdge) * 8 * sizeof(int16_t),
                       (void *)NULL));
  CHECK(clSetKernelArg(LaplacianPyr, 26,
                       (outTileH + 2 * outTileEdge) *
                           (outTileW + 2 * outTileEdge) * 8 * sizeof(int16_t),
                       (void *)NULL));

  global[0] = (size_t)(inHeight / inTileH);
  global[1] = (size_t)(inWidth / inTileW);
  std::cout << " LaplacianPyr global work items " << global[0] << " X "
            << global[1] << "  " << inTileW << " X " << inTileH << "\n";
  CHECK(clEnqueueNDRangeKernel(queue, LaplacianPyr, 2, NULL, global, local, 0,
                               NULL, &event));

  CHECK(clFinish(queue));
#if DEBUG_DUMP
  {
    CHECK(clEnqueueReadBuffer(queue, clLPyr1, CL_TRUE, 0,
                              outPitch * (outHeight + 2 * outEdge) * 8 *
                                  sizeof(int16_t),
                              temp, 0, NULL, NULL));
    CHECK(clFinish(queue));
    FILE *fp;
    char fname[256];
    for (int k = 0; k < 8; k++) {
      sprintf(fname, "LPyrLevel1_%d.yuv", k);
      fp = fopen(fname, "wb");
      for (int y = 0; y < outPitch * (outHeight + 2 * outEdge); y++) {
        fwrite(&temp[(y * 8 + k) * 2], 2, 1, fp);
      }
      fclose(fp);
    }
  }
#endif
  /*Generation of laplacian pyr level 0 (7)*/
  inPitch = (width) + 2 * PAD;
  inWidth = width;
  inHeight = height;
  inEdge = PAD;
  inTileW = TILE_W;
  inTileH = TILE_H / 2;
  inTileEdge = 0;

  in1Pitch = (width / 2) + 2 * PAD;
  in1Width = width / 2;
  in1Height = height / 2;
  in1Edge = PAD;
  in1TileW = TILE_W / 2;
  in1TileH = TILE_H / 4;
  in1TileEdge = PAD;

  outPitch = ((width) + 2 * PAD);
  outWidth = width;
  outHeight = height;
  outEdge = PAD;
  outTileW = TILE_W;
  outTileH = TILE_H / 2;
  outTileEdge = PAD;

  CHECK(clSetKernelArg(LaplacianPyr, 0, sizeof(cl_mem), (void *)&clGPyr0));
  CHECK(clSetKernelArg(LaplacianPyr, 1, sizeof(cl_int), (void *)&inPitch));
  CHECK(clSetKernelArg(LaplacianPyr, 2, sizeof(cl_int), (void *)&inEdge));
  CHECK(clSetKernelArg(LaplacianPyr, 3, sizeof(cl_int), (void *)&inWidth));
  CHECK(clSetKernelArg(LaplacianPyr, 4, sizeof(cl_int), (void *)&inHeight));
  CHECK(clSetKernelArg(LaplacianPyr, 5, sizeof(cl_int), (void *)&inTileW));
  CHECK(clSetKernelArg(LaplacianPyr, 6, sizeof(cl_int), (void *)&inTileH));
  CHECK(clSetKernelArg(LaplacianPyr, 7, sizeof(cl_int), (void *)&inTileEdge));
  CHECK(clSetKernelArg(LaplacianPyr, 8, sizeof(cl_mem), (void *)&clGPyr1));
  CHECK(clSetKernelArg(LaplacianPyr, 9, sizeof(cl_int), (void *)&in1Pitch));
  CHECK(clSetKernelArg(LaplacianPyr, 10, sizeof(cl_int), (void *)&in1Edge));
  CHECK(clSetKernelArg(LaplacianPyr, 11, sizeof(cl_int), (void *)&in1Width));
  CHECK(clSetKernelArg(LaplacianPyr, 12, sizeof(cl_int), (void *)&in1Height));
  CHECK(clSetKernelArg(LaplacianPyr, 13, sizeof(cl_int), (void *)&in1TileW));
  CHECK(clSetKernelArg(LaplacianPyr, 14, sizeof(cl_int), (void *)&in1TileH));
  CHECK(clSetKernelArg(LaplacianPyr, 15, sizeof(cl_int), (void *)&in1TileEdge));
  CHECK(clSetKernelArg(LaplacianPyr, 16, sizeof(cl_mem), (void *)&clLPyr0));
  CHECK(clSetKernelArg(LaplacianPyr, 17, sizeof(cl_int), (void *)&outPitch));
  CHECK(clSetKernelArg(LaplacianPyr, 18, sizeof(cl_int), (void *)&outEdge));
  CHECK(clSetKernelArg(LaplacianPyr, 19, sizeof(cl_int), (void *)&outWidth));
  CHECK(clSetKernelArg(LaplacianPyr, 20, sizeof(cl_int), (void *)&outHeight));
  CHECK(clSetKernelArg(LaplacianPyr, 21, sizeof(cl_int), (void *)&outTileW));
  CHECK(clSetKernelArg(LaplacianPyr, 22, sizeof(cl_int), (void *)&outTileH));
  CHECK(clSetKernelArg(LaplacianPyr, 23, sizeof(cl_int), (void *)&outTileEdge));
  CHECK(clSetKernelArg(LaplacianPyr, 24,
                       (inTileW + 2 * inTileEdge) * (inTileH + 2 * inTileEdge) *
                           8 * sizeof(int16_t),
                       (void *)NULL));
  CHECK(clSetKernelArg(LaplacianPyr, 25,
                       (in1TileW + 2 * in1TileEdge) *
                           (in1TileH + 2 * in1TileEdge) * 8 * sizeof(int16_t),
                       (void *)NULL));
  CHECK(clSetKernelArg(LaplacianPyr, 26,
                       (outTileH + 2 * outTileEdge) *
                           (outTileW + 2 * outTileEdge) * 8 * sizeof(int16_t),
                       (void *)NULL));

  global[0] = (size_t)(inHeight / inTileH);
  global[1] = (size_t)(inWidth / inTileW);
  std::cout << " LaplacianPyr global work items " << global[0] << " X "
            << global[1] << "  " << inTileW << " X " << inTileH << "\n";
  CHECK(clEnqueueNDRangeKernel(queue, LaplacianPyr, 2, NULL, global, local, 0,
                               NULL, &event));
  CHECK(clFinish(queue));
#if DEBUG_DUMP
  {
    CHECK(clEnqueueReadBuffer(queue, clLPyr0, CL_TRUE, 0,
                              outPitch * (outHeight + 2 * outEdge) * 8 *
                                  sizeof(int16_t),
                              temp, 0, NULL, NULL));

    CHECK(clFinish(queue));
    FILE *fp;
    char fname[256];

    for (int k = 0; k < 8; k++) {
      sprintf(fname, "LPyrLevel0_%d.yuv", k);
      fp = fopen(fname, "wb");
      for (int y = 0; y < outPitch * (outHeight + 2 * outEdge); y++) {
        fwrite(&temp[(y * 8 + k) * 2], 2, 1, fp);
      }
      fclose(fp);
    }
  }
#endif
  /*Fuse laplacian pyr level 2 (8)*/
  inPitch = (width / 4) + 2 * PAD;
  inWidth = width / 4;
  inHeight = height / 4;
  inEdge = PAD;
  inTileW = TILE_W;
  inTileH = TILE_H / 2;
  inTileEdge = 0;

  in1Pitch = (width / 4) + 2 * PAD;
  in1Width = width / 4;
  in1Height = height / 4;
  in1Edge = PAD;
  in1TileW = TILE_W;
  in1TileH = TILE_H / 2;
  in1TileEdge = 0;

  outPitch = ((width / 4) + 2 * PAD);
  outWidth = width / 4;
  outHeight = height / 4;
  outEdge = PAD;
  outTileW = TILE_W;
  outTileH = TILE_H / 2;
  outTileEdge = PAD;

  CHECK(clSetKernelArg(FuseLaplacianPyr, 0, sizeof(cl_mem), (void *)&clGLPyr2));
  CHECK(clSetKernelArg(FuseLaplacianPyr, 1, sizeof(cl_int), (void *)&inPitch));
  CHECK(clSetKernelArg(FuseLaplacianPyr, 2, sizeof(cl_int), (void *)&inEdge));
  CHECK(clSetKernelArg(FuseLaplacianPyr, 3, sizeof(cl_int), (void *)&inWidth));
  CHECK(clSetKernelArg(FuseLaplacianPyr, 4, sizeof(cl_int), (void *)&inHeight));
  CHECK(clSetKernelArg(FuseLaplacianPyr, 5, sizeof(cl_int), (void *)&inTileW));
  CHECK(clSetKernelArg(FuseLaplacianPyr, 6, sizeof(cl_int), (void *)&inTileH));
  CHECK(
      clSetKernelArg(FuseLaplacianPyr, 7, sizeof(cl_int), (void *)&inTileEdge));
  CHECK(clSetKernelArg(FuseLaplacianPyr, 8, sizeof(cl_mem), (void *)&clSrc2));
  CHECK(clSetKernelArg(FuseLaplacianPyr, 9, sizeof(cl_int), (void *)&in1Pitch));
  CHECK(clSetKernelArg(FuseLaplacianPyr, 10, sizeof(cl_int), (void *)&in1Edge));
  CHECK(
      clSetKernelArg(FuseLaplacianPyr, 11, sizeof(cl_int), (void *)&in1Width));
  CHECK(
      clSetKernelArg(FuseLaplacianPyr, 12, sizeof(cl_int), (void *)&in1Height));
  CHECK(
      clSetKernelArg(FuseLaplacianPyr, 13, sizeof(cl_int), (void *)&in1TileW));
  CHECK(
      clSetKernelArg(FuseLaplacianPyr, 14, sizeof(cl_int), (void *)&in1TileH));
  CHECK(clSetKernelArg(FuseLaplacianPyr, 15, sizeof(cl_int),
                       (void *)&in1TileEdge));
  CHECK(clSetKernelArg(FuseLaplacianPyr, 16, sizeof(cl_mem), (void *)&clL2));
  CHECK(
      clSetKernelArg(FuseLaplacianPyr, 17, sizeof(cl_int), (void *)&outPitch));
  CHECK(clSetKernelArg(FuseLaplacianPyr, 18, sizeof(cl_int), (void *)&outEdge));
  CHECK(
      clSetKernelArg(FuseLaplacianPyr, 19, sizeof(cl_int), (void *)&outWidth));
  CHECK(
      clSetKernelArg(FuseLaplacianPyr, 20, sizeof(cl_int), (void *)&outHeight));
  CHECK(
      clSetKernelArg(FuseLaplacianPyr, 21, sizeof(cl_int), (void *)&outTileW));
  CHECK(
      clSetKernelArg(FuseLaplacianPyr, 22, sizeof(cl_int), (void *)&outTileH));
  CHECK(clSetKernelArg(FuseLaplacianPyr, 23, sizeof(cl_int),
                       (void *)&outTileEdge));
  CHECK(clSetKernelArg(FuseLaplacianPyr, 24,
                       (inTileW + 2 * inTileEdge) * (inTileH + 2 * inTileEdge) *
                           8 * sizeof(int16_t),
                       (void *)NULL));
  CHECK(clSetKernelArg(FuseLaplacianPyr, 25,
                       (in1TileW + 2 * in1TileEdge) *
                           (in1TileH + 2 * in1TileEdge),
                       (void *)NULL));
  CHECK(clSetKernelArg(FuseLaplacianPyr, 26,
                       (outTileH + 2 * outTileEdge) *
                           (outTileW + 2 * outTileEdge) * sizeof(int16_t),
                       (void *)NULL));

  global[0] = (size_t)(inHeight / inTileH);
  global[1] = (size_t)(inWidth / inTileW);
  std::cout << " FuseLaplacianPyr global work items " << global[0] << " X "
            << global[1] << "  " << inTileW << " X " << inTileH << "\n";
  CHECK(clEnqueueNDRangeKernel(queue, FuseLaplacianPyr, 2, NULL, global, local,
                               0, NULL, &event));
  CHECK(clFinish(queue));
#if DEBUG_DUMP
  {
    CHECK(clEnqueueReadBuffer(queue, clL2, CL_TRUE, 0,
                              outPitch * (outHeight + 2 * outEdge) *
                                  sizeof(int16_t),
                              temp, 0, NULL, NULL));

    CHECK(clFinish(queue));

    FILE *fp;
    fp = fopen("Lap2Dlevel2.raw", "wb");
    fwrite(temp, 2, outPitch * (outHeight + 2 * outEdge), fp);
    fclose(fp);
  }
#endif
  /*Fuse laplacian pyr level 1 (9)*/
  inPitch = (width / 2) + 2 * PAD;
  inWidth = width / 2;
  inHeight = height / 2;
  inEdge = PAD;
  inTileW = TILE_W;
  inTileH = TILE_H / 2;
  inTileEdge = 0;

  in1Pitch = (width / 2) + 2 * PAD;
  in1Width = width / 2;
  in1Height = height / 2;
  in1Edge = PAD;
  in1TileW = TILE_W;
  in1TileH = TILE_H / 2;
  in1TileEdge = 0;

  outPitch = ((width / 2) + 2 * PAD);
  outWidth = width / 2;
  outHeight = height / 2;
  outEdge = PAD;
  outTileW = TILE_W;
  outTileH = TILE_H / 2;
  outTileEdge = PAD;

  CHECK(clSetKernelArg(FuseLaplacianPyr, 0, sizeof(cl_mem), (void *)&clLPyr1));
  CHECK(clSetKernelArg(FuseLaplacianPyr, 1, sizeof(cl_int), (void *)&inPitch));
  CHECK(clSetKernelArg(FuseLaplacianPyr, 2, sizeof(cl_int), (void *)&inEdge));
  CHECK(clSetKernelArg(FuseLaplacianPyr, 3, sizeof(cl_int), (void *)&inWidth));
  CHECK(clSetKernelArg(FuseLaplacianPyr, 4, sizeof(cl_int), (void *)&inHeight));
  CHECK(clSetKernelArg(FuseLaplacianPyr, 5, sizeof(cl_int), (void *)&inTileW));
  CHECK(clSetKernelArg(FuseLaplacianPyr, 6, sizeof(cl_int), (void *)&inTileH));
  CHECK(
      clSetKernelArg(FuseLaplacianPyr, 7, sizeof(cl_int), (void *)&inTileEdge));
  CHECK(clSetKernelArg(FuseLaplacianPyr, 8, sizeof(cl_mem), (void *)&clSrc1));
  CHECK(clSetKernelArg(FuseLaplacianPyr, 9, sizeof(cl_int), (void *)&in1Pitch));
  CHECK(clSetKernelArg(FuseLaplacianPyr, 10, sizeof(cl_int), (void *)&in1Edge));
  CHECK(
      clSetKernelArg(FuseLaplacianPyr, 11, sizeof(cl_int), (void *)&in1Width));
  CHECK(
      clSetKernelArg(FuseLaplacianPyr, 12, sizeof(cl_int), (void *)&in1Height));
  CHECK(
      clSetKernelArg(FuseLaplacianPyr, 13, sizeof(cl_int), (void *)&in1TileW));
  CHECK(
      clSetKernelArg(FuseLaplacianPyr, 14, sizeof(cl_int), (void *)&in1TileH));
  CHECK(clSetKernelArg(FuseLaplacianPyr, 15, sizeof(cl_int),
                       (void *)&in1TileEdge));
  CHECK(clSetKernelArg(FuseLaplacianPyr, 16, sizeof(cl_mem), (void *)&clL1));
  CHECK(
      clSetKernelArg(FuseLaplacianPyr, 17, sizeof(cl_int), (void *)&outPitch));
  CHECK(clSetKernelArg(FuseLaplacianPyr, 18, sizeof(cl_int), (void *)&outEdge));
  CHECK(
      clSetKernelArg(FuseLaplacianPyr, 19, sizeof(cl_int), (void *)&outWidth));
  CHECK(
      clSetKernelArg(FuseLaplacianPyr, 20, sizeof(cl_int), (void *)&outHeight));
  CHECK(
      clSetKernelArg(FuseLaplacianPyr, 21, sizeof(cl_int), (void *)&outTileW));
  CHECK(
      clSetKernelArg(FuseLaplacianPyr, 22, sizeof(cl_int), (void *)&outTileH));
  CHECK(clSetKernelArg(FuseLaplacianPyr, 23, sizeof(cl_int),
                       (void *)&outTileEdge));
  CHECK(clSetKernelArg(FuseLaplacianPyr, 24,
                       (inTileW + 2 * inTileEdge) * (inTileH + 2 * inTileEdge) *
                           8 * sizeof(int16_t),
                       (void *)NULL));
  CHECK(clSetKernelArg(FuseLaplacianPyr, 25,
                       (in1TileW + 2 * in1TileEdge) *
                           (in1TileH + 2 * in1TileEdge),
                       (void *)NULL));
  CHECK(clSetKernelArg(FuseLaplacianPyr, 26,
                       (outTileH + 2 * outTileEdge) *
                           (outTileW + 2 * outTileEdge) * sizeof(int16_t),
                       (void *)NULL));

  global[0] = (size_t)(inHeight / inTileH);
  global[1] = (size_t)(inWidth / inTileW);
  std::cout << " FuseLaplacianPyr global work items " << global[0] << " X "
            << global[1] << "  " << inTileW << " X " << inTileH << "\n";
  CHECK(clEnqueueNDRangeKernel(queue, FuseLaplacianPyr, 2, NULL, global, local,
                               0, NULL, &event));
  CHECK(clFinish(queue));
#if DEBUG_DUMP
  {
    CHECK(clEnqueueReadBuffer(queue, clL1, CL_TRUE, 0,
                              outPitch * (outHeight + 2 * outEdge) *
                                  sizeof(int16_t),
                              temp, 0, NULL, NULL));

    CHECK(clFinish(queue));

    FILE *fp;
    fp = fopen("Lap2Dlevel1.raw", "wb");
    fwrite(temp, 2, outPitch * (outHeight + 2 * outEdge), fp);
    fclose(fp);
  }
#endif

  /*Fuse laplacian pyr level 1 (10)*/
  inPitch = (width / 1) + 2 * PAD;
  inWidth = width / 1;
  inHeight = height / 1;
  inEdge = PAD;
  inTileW = TILE_W;
  inTileH = TILE_H / 2;
  inTileEdge = 0;

  in1Pitch = (width / 1) + 2 * PAD;
  in1Width = width / 1;
  in1Height = height / 1;
  in1Edge = PAD;
  in1TileW = TILE_W;
  in1TileH = TILE_H / 2;
  in1TileEdge = 0;

  outPitch = ((width / 1) + 2 * PAD);
  outWidth = width / 1;
  outHeight = height / 1;
  outEdge = PAD;
  outTileW = TILE_W;
  outTileH = TILE_H / 2;
  outTileEdge = PAD;

  CHECK(clSetKernelArg(FuseLaplacianPyr, 0, sizeof(cl_mem), (void *)&clLPyr0));
  CHECK(clSetKernelArg(FuseLaplacianPyr, 1, sizeof(cl_int), (void *)&inPitch));
  CHECK(clSetKernelArg(FuseLaplacianPyr, 2, sizeof(cl_int), (void *)&inEdge));
  CHECK(clSetKernelArg(FuseLaplacianPyr, 3, sizeof(cl_int), (void *)&inWidth));
  CHECK(clSetKernelArg(FuseLaplacianPyr, 4, sizeof(cl_int), (void *)&inHeight));
  CHECK(clSetKernelArg(FuseLaplacianPyr, 5, sizeof(cl_int), (void *)&inTileW));
  CHECK(clSetKernelArg(FuseLaplacianPyr, 6, sizeof(cl_int), (void *)&inTileH));
  CHECK(
      clSetKernelArg(FuseLaplacianPyr, 7, sizeof(cl_int), (void *)&inTileEdge));
  CHECK(clSetKernelArg(FuseLaplacianPyr, 8, sizeof(cl_mem), (void *)&clSrc0));
  CHECK(clSetKernelArg(FuseLaplacianPyr, 9, sizeof(cl_int), (void *)&in1Pitch));
  CHECK(clSetKernelArg(FuseLaplacianPyr, 10, sizeof(cl_int), (void *)&in1Edge));
  CHECK(
      clSetKernelArg(FuseLaplacianPyr, 11, sizeof(cl_int), (void *)&in1Width));
  CHECK(
      clSetKernelArg(FuseLaplacianPyr, 12, sizeof(cl_int), (void *)&in1Height));
  CHECK(
      clSetKernelArg(FuseLaplacianPyr, 13, sizeof(cl_int), (void *)&in1TileW));
  CHECK(
      clSetKernelArg(FuseLaplacianPyr, 14, sizeof(cl_int), (void *)&in1TileH));
  CHECK(clSetKernelArg(FuseLaplacianPyr, 15, sizeof(cl_int),
                       (void *)&in1TileEdge));
  CHECK(clSetKernelArg(FuseLaplacianPyr, 16, sizeof(cl_mem), (void *)&clL0));
  CHECK(
      clSetKernelArg(FuseLaplacianPyr, 17, sizeof(cl_int), (void *)&outPitch));
  CHECK(clSetKernelArg(FuseLaplacianPyr, 18, sizeof(cl_int), (void *)&outEdge));
  CHECK(
      clSetKernelArg(FuseLaplacianPyr, 19, sizeof(cl_int), (void *)&outWidth));
  CHECK(
      clSetKernelArg(FuseLaplacianPyr, 20, sizeof(cl_int), (void *)&outHeight));
  CHECK(
      clSetKernelArg(FuseLaplacianPyr, 21, sizeof(cl_int), (void *)&outTileW));
  CHECK(
      clSetKernelArg(FuseLaplacianPyr, 22, sizeof(cl_int), (void *)&outTileH));
  CHECK(clSetKernelArg(FuseLaplacianPyr, 23, sizeof(cl_int),
                       (void *)&outTileEdge));
  CHECK(clSetKernelArg(FuseLaplacianPyr, 24,
                       (inTileW + 2 * inTileEdge) * (inTileH + 2 * inTileEdge) *
                           8 * sizeof(int16_t),
                       (void *)NULL));
  CHECK(clSetKernelArg(FuseLaplacianPyr, 25,
                       (in1TileW + 2 * in1TileEdge) *
                           (in1TileH + 2 * in1TileEdge),
                       (void *)NULL));
  CHECK(clSetKernelArg(FuseLaplacianPyr, 26,
                       (outTileH + 2 * outTileEdge) *
                           (outTileW + 2 * outTileEdge) * sizeof(int16_t),
                       (void *)NULL));

  global[0] = (size_t)(inHeight / inTileH);
  global[1] = (size_t)(inWidth / inTileW);
  std::cout << " FuseLaplacianPyr global work items " << global[0] << " X "
            << global[1] << "  " << inTileW << " X " << inTileH << "\n";
  CHECK(clEnqueueNDRangeKernel(queue, FuseLaplacianPyr, 2, NULL, global, local,
                               0, NULL, &event));

  CHECK(clFinish(queue));
#if DEBUG_DUMP
  {
    CHECK(clEnqueueReadBuffer(queue, clL0, CL_TRUE, 0,
                              outPitch * (outHeight + 2 * outEdge) *
                                  sizeof(int16_t),
                              temp, 0, NULL, NULL));

    CHECK(clFinish(queue));

    FILE *fp;
    fp = fopen("Lap2Dlevel0.raw", "wb");
    fwrite(temp, 2, outPitch * (outHeight + 2 * outEdge), fp);
    fclose(fp);
  }
#endif
  /*Fuse laplacian pyr level 2 (11)*/
  inPitch = (width / 4) + 2 * PAD;
  inWidth = width / 4;
  inHeight = height / 4;
  inEdge = PAD;
  inTileW = TILE_W / 2;
  inTileH = TILE_H / 2;
  inTileEdge = PAD;

  in1Pitch = (width / 2) + 2 * PAD;
  in1Width = width / 2;
  in1Height = height / 2;
  in1Edge = PAD;
  in1TileW = TILE_W;
  in1TileH = TILE_H;
  in1TileEdge = 0;

  outPitch = ((width / 2) + 2 * PAD);
  outWidth = width / 2;
  outHeight = height / 2;
  outEdge = PAD;
  outTileW = TILE_W;
  outTileH = TILE_H;
  outTileEdge = PAD;

  CHECK(clSetKernelArg(FusePyr, 0, sizeof(cl_mem), (void *)&clL2));
  CHECK(clSetKernelArg(FusePyr, 1, sizeof(cl_int), (void *)&inPitch));
  CHECK(clSetKernelArg(FusePyr, 2, sizeof(cl_int), (void *)&inEdge));
  CHECK(clSetKernelArg(FusePyr, 3, sizeof(cl_int), (void *)&inWidth));
  CHECK(clSetKernelArg(FusePyr, 4, sizeof(cl_int), (void *)&inHeight));
  CHECK(clSetKernelArg(FusePyr, 5, sizeof(cl_int), (void *)&inTileW));
  CHECK(clSetKernelArg(FusePyr, 6, sizeof(cl_int), (void *)&inTileH));
  CHECK(clSetKernelArg(FusePyr, 7, sizeof(cl_int), (void *)&inTileEdge));
  CHECK(clSetKernelArg(FusePyr, 8, sizeof(cl_mem), (void *)&clL1));
  CHECK(clSetKernelArg(FusePyr, 9, sizeof(cl_int), (void *)&in1Pitch));
  CHECK(clSetKernelArg(FusePyr, 10, sizeof(cl_int), (void *)&in1Edge));
  CHECK(clSetKernelArg(FusePyr, 11, sizeof(cl_int), (void *)&in1Width));
  CHECK(clSetKernelArg(FusePyr, 12, sizeof(cl_int), (void *)&in1Height));
  CHECK(clSetKernelArg(FusePyr, 13, sizeof(cl_int), (void *)&in1TileW));
  CHECK(clSetKernelArg(FusePyr, 14, sizeof(cl_int), (void *)&in1TileH));
  CHECK(clSetKernelArg(FusePyr, 15, sizeof(cl_int), (void *)&in1TileEdge));
  CHECK(clSetKernelArg(FusePyr, 16, sizeof(cl_mem), (void *)&clDst1));
  CHECK(clSetKernelArg(FusePyr, 17, sizeof(cl_int), (void *)&outPitch));
  CHECK(clSetKernelArg(FusePyr, 18, sizeof(cl_int), (void *)&outEdge));
  CHECK(clSetKernelArg(FusePyr, 19, sizeof(cl_int), (void *)&outWidth));
  CHECK(clSetKernelArg(FusePyr, 20, sizeof(cl_int), (void *)&outHeight));
  CHECK(clSetKernelArg(FusePyr, 21, sizeof(cl_int), (void *)&outTileW));
  CHECK(clSetKernelArg(FusePyr, 22, sizeof(cl_int), (void *)&outTileH));
  CHECK(clSetKernelArg(FusePyr, 23, sizeof(cl_int), (void *)&outTileEdge));
  CHECK(clSetKernelArg(FusePyr, 24,
                       (inTileW + 2 * inTileEdge) * (inTileH + 2 * inTileEdge) *
                           sizeof(int16_t),
                       (void *)NULL));
  CHECK(clSetKernelArg(FusePyr, 25,
                       (in1TileW + 2 * in1TileEdge) *
                           (in1TileH + 2 * in1TileEdge) * sizeof(int16_t),
                       (void *)NULL));
  CHECK(clSetKernelArg(FusePyr, 26,
                       (outTileH + 2 * outTileEdge) *
                           (outTileW + 2 * outTileEdge) * sizeof(int16_t),
                       (void *)NULL));

  global[0] = (size_t)(inHeight / inTileH);
  global[1] = (size_t)(inWidth / inTileW);
  std::cout << " FusedPyrUpSample global work items " << global[0] << " X "
            << global[1] << "  " << inTileW << " X " << inTileH << "\n";
  CHECK(clEnqueueNDRangeKernel(queue, FusePyr, 2, NULL, global, local, 0, NULL,
                               &event));
#if DEBUG_DUMP
  {
    CHECK(clEnqueueReadBuffer(queue, clDst1, CL_TRUE, 0,
                              outPitch * (outHeight + 2 * outEdge) *
                                  sizeof(int16_t),
                              temp, 0, NULL, NULL));

    CHECK(clFinish(queue));

    FILE *fp;
    fp = fopen("out1.raw", "wb");
    fwrite(temp, 2, outPitch * (outHeight + 2 * outEdge), fp);
    fclose(fp);
  }
#endif
  /*Fuse laplacian pyr level 2 (12)*/
  inPitch = (width / 2) + 2 * PAD;
  inWidth = width / 2;
  inHeight = height / 2;
  inEdge = PAD;
  inTileW = TILE_W / 2;
  inTileH = TILE_H / 2;
  inTileEdge = PAD;

  in1Pitch = (width) + 2 * PAD;
  in1Width = width;
  in1Height = height;
  in1Edge = PAD;
  in1TileW = TILE_W;
  in1TileH = TILE_H;
  in1TileEdge = 0;

  outPitch = ((width));
  outWidth = width;
  outHeight = height;
  outEdge = 0;
  outTileW = TILE_W;
  outTileH = TILE_H;
  outTileEdge = 0;

  CHECK(clSetKernelArg(FusePyr1, 0, sizeof(cl_mem), (void *)&clDst1));
  CHECK(clSetKernelArg(FusePyr1, 1, sizeof(cl_int), (void *)&inPitch));
  CHECK(clSetKernelArg(FusePyr1, 2, sizeof(cl_int), (void *)&inEdge));
  CHECK(clSetKernelArg(FusePyr1, 3, sizeof(cl_int), (void *)&inWidth));
  CHECK(clSetKernelArg(FusePyr1, 4, sizeof(cl_int), (void *)&inHeight));
  CHECK(clSetKernelArg(FusePyr1, 5, sizeof(cl_int), (void *)&inTileW));
  CHECK(clSetKernelArg(FusePyr1, 6, sizeof(cl_int), (void *)&inTileH));
  CHECK(clSetKernelArg(FusePyr1, 7, sizeof(cl_int), (void *)&inTileEdge));
  CHECK(clSetKernelArg(FusePyr1, 8, sizeof(cl_mem), (void *)&clL0));
  CHECK(clSetKernelArg(FusePyr1, 9, sizeof(cl_int), (void *)&in1Pitch));
  CHECK(clSetKernelArg(FusePyr1, 10, sizeof(cl_int), (void *)&in1Edge));
  CHECK(clSetKernelArg(FusePyr1, 11, sizeof(cl_int), (void *)&in1Width));
  CHECK(clSetKernelArg(FusePyr1, 12, sizeof(cl_int), (void *)&in1Height));
  CHECK(clSetKernelArg(FusePyr1, 13, sizeof(cl_int), (void *)&in1TileW));
  CHECK(clSetKernelArg(FusePyr1, 14, sizeof(cl_int), (void *)&in1TileH));
  CHECK(clSetKernelArg(FusePyr1, 15, sizeof(cl_int), (void *)&in1TileEdge));
  CHECK(clSetKernelArg(FusePyr1, 16, sizeof(cl_mem), (void *)&clDst0));
  CHECK(clSetKernelArg(FusePyr1, 17, sizeof(cl_int), (void *)&outPitch));
  CHECK(clSetKernelArg(FusePyr1, 18, sizeof(cl_int), (void *)&outEdge));
  CHECK(clSetKernelArg(FusePyr1, 19, sizeof(cl_int), (void *)&outWidth));
  CHECK(clSetKernelArg(FusePyr1, 20, sizeof(cl_int), (void *)&outHeight));
  CHECK(clSetKernelArg(FusePyr1, 21, sizeof(cl_int), (void *)&outTileW));
  CHECK(clSetKernelArg(FusePyr1, 22, sizeof(cl_int), (void *)&outTileH));
  CHECK(clSetKernelArg(FusePyr1, 23, sizeof(cl_int), (void *)&outTileEdge));
  CHECK(clSetKernelArg(FusePyr1, 24,
                       (inTileW + 2 * inTileEdge) * (inTileH + 2 * inTileEdge) *
                           sizeof(int16_t),
                       (void *)NULL));
  CHECK(clSetKernelArg(FusePyr1, 25,
                       (in1TileW + 2 * in1TileEdge) *
                           (in1TileH + 2 * in1TileEdge) * sizeof(int16_t),
                       (void *)NULL));
  CHECK(clSetKernelArg(FusePyr1, 26,
                       (outTileH + 2 * outTileEdge) *
                           (outTileW + 2 * outTileEdge) * sizeof(uint8_t),
                       (void *)NULL));

  global[0] = (size_t)(inHeight / inTileH);
  global[1] = (size_t)(inWidth / inTileW);
  std::cout << " FusedPyrUpSampleClip global work items " << global[0] << " X "
            << global[1] << "  " << inTileW << " X " << inTileH << "\n";
  CHECK(clEnqueueNDRangeKernel(queue, FusePyr1, 2, NULL, global, local, 0, NULL,
                               &event));

  CHECK(clFinish(queue));
  CHECK(clEnqueueReadBuffer(queue, clDst0, CL_TRUE, 0,
                            (sizeof(cl_char) * DstPitch * dstHeight), Dst, 0,
                            NULL, NULL));

  CHECK(clFinish(queue));
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

  // xiMedianBlur_3x3_U8_ref(&src_t,(xi_pArray) &dst_t);
  xiLocalLaplacian_Gray_U8_FixPt_ref(&src_t, &dst_t, 8, 3, &remap_table[0]);
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

  launch_ocl_kernel(gsrc, srcStride, gdst, dstWidth, srcWidth, srcHeight,
                    dstWidth, dstHeight, &remap_table[0]);

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

  fwrite(gdst, dstWidth * dstHeight, 1, fpOut);
  fclose(fpOut);

  printf("Compare: ");
  if (memcmp(gdst_ref, gdst, dstWidth * dstHeight) == 0) {
    printf("PASS \n");
  } else {
    printf("FAIL \n");
  }

  return 0;
}
