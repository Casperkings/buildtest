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
#define OUTPUT_IMAGE_W (IMAGE_W * 3)
#define IMAGE_H 512
#define SRC_BIT_DEPTH 8
#define DST_BIT_DEPTH 8
#define TILE_W 128
#define OUTPUT_TILE_W (TILE_W * 3)
#define TILE_H 64
#define W_EXT 0
#define H_EXT 0
#define IMAGE_PAD 0

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

void launch_ocl_kernel(uint8_t *Dst, const uint32_t DstPitch, uint8_t *SrcY,
                       uint8_t *SrcU, uint8_t *SrcV, uint32_t SrcPitch,
                       int32_t width, int32_t height, int32_t dstWidth,
                       int32_t dstHeight) {
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

  cl_mem clDst = clCreateBuffer(
      context,CL_MEM_WRITE_ONLY ,
      (sizeof(uint8_t) * (DstPitch * dstHeight)), NULL,
      &status);
  CHECK_ERROR(status);

  cl_mem clSrcY =
      clCreateBuffer(context, CL_MEM_READ_ONLY,
                     (sizeof(uint8_t) * SrcPitch * height), NULL, &status);
  CHECK_ERROR(status);

  cl_mem clSrcU =
      clCreateBuffer(context, CL_MEM_READ_ONLY,
                     (sizeof(uint8_t) * SrcPitch * height/4), NULL, &status);
  CHECK_ERROR(status);

  cl_mem clSrcV =
      clCreateBuffer(context, CL_MEM_READ_ONLY,
                     (sizeof(uint8_t) * SrcPitch * height/4), NULL, &status);
  CHECK_ERROR(status);

  cl_event event = clCreateUserEvent(context, &status);
  CHECK_ERROR(status);

  const char *progSource = loadKernel("YUV420toRGB.cl");
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
  local_dst_size = ((OUTPUT_TILE_W) * (TILE_H) * sizeof(cl_uchar));
  // local_src_size = width * height * sizeof(cl_uchar);
  // local_dst_size = dstWidth * dstHeight * sizeof(cl_uchar);

  std::cout << " SrcPitch = " << SrcPitch << "\n";
  std::cout << " DstPitch = " << DstPitch << "\n";

  size_t local[2] = {1, 1};
  size_t global[2] = {(size_t)(height / TILE_H),
                      (size_t)(width / TILE_W)};

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
      (sizeof(uint8_t) * (SrcPitch * (height + IMAGE_PAD + IMAGE_PAD))), SrcY, 0,
      NULL, NULL);
  CHECK_ERROR(error);
  
   error = clEnqueueWriteBuffer(
      queue, clSrcU, CL_TRUE, 0,
      (sizeof(uint8_t) * (SrcPitch * (height + IMAGE_PAD + IMAGE_PAD))/4), SrcU, 0,
      NULL, NULL);
  CHECK_ERROR(error);
  
   error = clEnqueueWriteBuffer(
      queue, clSrcV, CL_TRUE, 0,
      (sizeof(uint8_t) * (SrcPitch * (height + IMAGE_PAD + IMAGE_PAD))/4), SrcV, 0,
      NULL, NULL);
  CHECK_ERROR(error);

  CHECK(clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&clDst));
  CHECK(clSetKernelArg(kernel, 1, sizeof(cl_int), (void *)&DstPitch));
  CHECK(clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&clSrcY));
  CHECK(clSetKernelArg(kernel, 3, sizeof(cl_mem), (void *)&clSrcU));
  CHECK(clSetKernelArg(kernel, 4, sizeof(cl_mem), (void *)&clSrcV));
  CHECK(clSetKernelArg(kernel, 5, sizeof(cl_int), (void *)&SrcPitch));
  CHECK(clSetKernelArg(kernel, 6, sizeof(cl_int), (void *)&width));
  CHECK(clSetKernelArg(kernel, 7, sizeof(cl_int), (void *)&height));
  CHECK(clSetKernelArg(kernel, 8, sizeof(cl_int), (void *)&dstWidth));
  CHECK(clSetKernelArg(kernel, 9, sizeof(cl_int), (void *)&dstHeight));

  CHECK(clSetKernelArg(kernel, 10, local_src_size, (void *)NULL));
  CHECK(clSetKernelArg(kernel, 11, local_src_size/4, (void *)NULL));
  CHECK(clSetKernelArg(kernel, 12, local_src_size/4, (void *)NULL));
  CHECK(clSetKernelArg(kernel, 13, local_dst_size, (void *)NULL));
  CHECK(clSetKernelArg(kernel, 14, sizeof(cl_mem), (void *)&clErrCodes));

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

void run_reference(uint8_t *Dst, uint32_t DstPitch, uint8_t *SrcY,
                   uint8_t *SrcU, uint8_t *SrcV, const uint32_t SrcPitch,
                   int32_t srcWidth, int32_t srcHeight, int32_t dstWidth,
                   int32_t dstHeight) {
  xi_tile dst_t;
  xi_tile srcY_t, srcU_t, srcV_t;

  // SRC
  XI_TILE_SET_BUFF_PTR((&dst_t), Dst);
  XI_TILE_SET_DATA_PTR((&dst_t),
                       ((uint8_t *)Dst) + DstPitch * IMAGE_PAD + IMAGE_PAD);
  XI_TILE_SET_BUFF_SIZE((&dst_t), DstPitch * (dstHeight + 2 * IMAGE_PAD));
  XI_TILE_SET_WIDTH((&dst_t), dstWidth);
  XI_TILE_SET_HEIGHT((&dst_t), dstHeight);
  XI_TILE_SET_PITCH((&dst_t), dstWidth + 2 * IMAGE_PAD);
  XI_TILE_SET_TYPE((&dst_t), XI_TILE_U8);
  XI_TILE_SET_EDGE_WIDTH((&dst_t), IMAGE_PAD);
  XI_TILE_SET_EDGE_HEIGHT((&dst_t), IMAGE_PAD);
  XI_TILE_SET_X_COORD((&dst_t), 0);
  XI_TILE_SET_Y_COORD((&dst_t), 0);

  // DST_Y
  XI_TILE_SET_BUFF_PTR((&srcY_t), SrcY);
  XI_TILE_SET_DATA_PTR((&srcY_t), SrcY);
  XI_TILE_SET_BUFF_SIZE((&srcY_t), (SrcPitch * (srcHeight)));
  XI_TILE_SET_WIDTH((&srcY_t), srcWidth);
  XI_TILE_SET_HEIGHT((&srcY_t), srcHeight);
  XI_TILE_SET_PITCH((&srcY_t), (SrcPitch));
  XI_TILE_SET_TYPE((&srcY_t), XI_TILE_U8);
  XI_TILE_SET_EDGE_WIDTH((&srcY_t), 0);
  XI_TILE_SET_EDGE_HEIGHT((&srcY_t), 0);
  XI_TILE_SET_X_COORD((&srcY_t), 0);
  XI_TILE_SET_Y_COORD((&srcY_t), 0);

  // DST_U
  XI_TILE_SET_BUFF_PTR((&srcU_t), SrcU);
  XI_TILE_SET_DATA_PTR((&srcU_t), SrcU);
  XI_TILE_SET_BUFF_SIZE((&srcU_t), (SrcPitch * (srcHeight))/4);
  XI_TILE_SET_WIDTH((&srcU_t), srcWidth/2);
  XI_TILE_SET_HEIGHT((&srcU_t), srcHeight/2);
  XI_TILE_SET_PITCH((&srcU_t), (SrcPitch)/2);
  XI_TILE_SET_TYPE((&srcU_t), XI_TILE_U8);
  XI_TILE_SET_EDGE_WIDTH((&srcU_t), 0);
  XI_TILE_SET_EDGE_HEIGHT((&srcU_t), 0);
  XI_TILE_SET_X_COORD((&srcU_t), 0);
  XI_TILE_SET_Y_COORD((&srcU_t), 0);

  // DST_V
  XI_TILE_SET_BUFF_PTR((&srcV_t), SrcV);
  XI_TILE_SET_DATA_PTR((&srcV_t), SrcV);
  XI_TILE_SET_BUFF_SIZE((&srcV_t), (SrcPitch * (srcHeight))/4);
  XI_TILE_SET_WIDTH((&srcV_t), srcWidth/2);
  XI_TILE_SET_HEIGHT((&srcV_t), srcHeight/2);
  XI_TILE_SET_PITCH((&srcV_t), (SrcPitch)/2);
  XI_TILE_SET_TYPE((&srcV_t), XI_TILE_U8);
  XI_TILE_SET_EDGE_WIDTH((&srcV_t), 0);
  XI_TILE_SET_EDGE_HEIGHT((&srcV_t), 0);
  XI_TILE_SET_X_COORD((&srcV_t), 0);
  XI_TILE_SET_Y_COORD((&srcV_t), 0);

  xiCvtColor_U8_YUV2RGB_420_BT709_ref((xi_pArray)&srcY_t,
                                  (xi_pArray)&srcU_t, (xi_pArray)&srcV_t, (xi_pArray)&dst_t);
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
  uint8_t *gdst;
  uint8_t *gdst_ref;
  uint8_t *gsrcY, *gsrcU, *gsrcV;

  // Image dimensions and bit depth
  int32_t srcWidth, srcHeight, srcBytes;
  int32_t dstWidth, dstHeight, dstBytes;
  int32_t srcStride;

  // Set frame dimensions
  srcWidth = IMAGE_W;
  srcHeight = IMAGE_H;
  srcStride = (IMAGE_W + IMAGE_PAD + IMAGE_PAD);
  srcBytes = SRC_BIT_DEPTH >> 3;
  dstWidth = OUTPUT_IMAGE_W;
  dstHeight = srcHeight;
  dstBytes = DST_BIT_DEPTH >> 3;

  // Grey-scale I/O frame buffers
  gdst = (uint8_t *)malloc(dstWidth * dstHeight * dstBytes);
  gsrcY = (uint8_t *)malloc(srcStride * srcHeight * srcBytes);
  gsrcU = (uint8_t *)malloc((srcStride * srcHeight * srcBytes)/4);
  gsrcV = (uint8_t *)malloc((srcStride * srcHeight * srcBytes)/4);

  gdst_ref = (uint8_t *)malloc(dstWidth * dstHeight * dstBytes);
  if (!gdst || !gsrcY || !gsrcU || !gsrcV ) {
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
    fread(ptr, 1, srcWidth * srcBytes, fpIn);
	ptr = gsrcU + (indy/2 * srcStride/2) + IMAGE_PAD;
    fread(ptr, 1, srcWidth * srcBytes/2, fpIn);
	ptr = gsrcV + (indy/2 * srcStride/2) + IMAGE_PAD;
    fread(ptr, 1, srcWidth * srcBytes/2, fpIn);
  }
  fclose(fpIn);

  
 
  launch_ocl_kernel(gdst,dstWidth,  gsrcY, gsrcU, gsrcV, srcStride, srcWidth,
                    srcHeight, dstWidth, dstHeight);

  run_reference(gdst_ref, dstWidth, gsrcY, gsrcU, gsrcV, srcStride,
                srcWidth, srcHeight, dstWidth, dstHeight);

  FILE *fpOut;
  // Write result
  sprintf(fname, "OutputRGB.yuv");
  fpOut = fopen(fname, "wb");
  if (fpOut == NULL) {
    printf("Error opening output file: %s\n", fname);
    exit(0);
  }

  fwrite(gdst, dstWidth * dstHeight , 1, fpOut);
  fclose(fpOut);
 

  if (memcmp(gdst_ref, gdst, dstWidth * dstHeight) == 0) {
    printf("PASS RGB\n");
  } else {
    printf("FAIL RGB\n");
  }


  return 0;
}
