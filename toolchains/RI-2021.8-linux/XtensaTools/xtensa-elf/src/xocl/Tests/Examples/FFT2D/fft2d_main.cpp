#include "../../Native/FFT2D_ref/fft.h"
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
#define SRC_BIT_DEPTH 32
#define DST_BIT_DEPTH 32
#define TILE_W 128
#define TILE_H 64
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

#define TO_Q13_18(val) ((int)((val) * (1 << 18) + 0.5))
#if NATIVE_KERNEL || XT_OCL_EXT
extern int16_t twiddle_factor_64[];
#define twiddle_factor_type int16_t
#else
extern int32_t twiddle_factor_64[];
#define twiddle_factor_type int32_t
#endif
void launch_ocl_kernel(int32_t *Src, const uint32_t SrcPitch, int32_t *Dst,
                       const uint32_t DstPitch, const int32_t width,
                       const int32_t height, const int32_t dstWidth,
                       const int32_t dstHeight, int16_t *W64, int32_t *Out_idx,
                       twiddle_factor_type *twiddle_factor_64) {
  cl_int status;
  int32_t TileW = TILE_W;
  int32_t TileH = TILE_H;
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
      (sizeof(int32_t) * (SrcPitch * (height + IMAGE_PAD + IMAGE_PAD))), NULL,
      &status);
  CHECK_ERROR(status);

  cl_mem clW64 = clCreateBuffer(context, CL_MEM_READ_ONLY,
                                (sizeof(int16_t) * 16 * 6 * 2), NULL, &status);
  CHECK_ERROR(status);

  cl_mem clOut_idx = clCreateBuffer(context, CL_MEM_READ_ONLY,
                                    (sizeof(int32_t) * 64), NULL, &status);
  CHECK_ERROR(status);

  cl_mem cltwiddle_factor_64 = clCreateBuffer(
      context, CL_MEM_READ_ONLY, (sizeof(twiddle_factor_type) * 96 * 2), 
      NULL, &status);
  CHECK_ERROR(status);

  cl_mem clDstPass1Re = clCreateBuffer(
      context, CL_MEM_READ_WRITE,
      (sizeof(int32_t) * (DstPitch >> 1) * dstHeight), NULL, &status);
  CHECK_ERROR(status);

  cl_mem clDstPass1Im = clCreateBuffer(
      context, CL_MEM_READ_WRITE,
      (sizeof(int32_t) * (DstPitch >> 1) * dstHeight), NULL, &status);
  CHECK_ERROR(status);

  cl_mem clDst =
      clCreateBuffer(context, CL_MEM_WRITE_ONLY,
                     (sizeof(int32_t) * DstPitch * dstHeight), NULL, &status);
  CHECK_ERROR(status);

  cl_event event = clCreateUserEvent(context, &status);
  CHECK_ERROR(status);

  const char *progSource = loadKernel("fft2d.cl");
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
                    (TILE_H + IMAGE_PAD + IMAGE_PAD) * sizeof(cl_int));
  local_dst_size = ((TILE_W) * (TILE_H) * sizeof(cl_int));
  int local_dst_size_pass1 = local_dst_size >> 1;
  std::cout << " SrcPitch = " << SrcPitch << "\n";
  std::cout << " DstPitch = " << DstPitch << "\n";

  size_t local[2] = {1, 1};
  size_t global[2] = {(size_t)(height / TILE_H), (size_t)(width / TILE_W)};

  std::cout << "global work items " << global[0] << " X " << global[1] << "\n";
  std::cout << "local work items " << local[0] << " X " << local[1] << "\n";

  cl_kernel kernel_verticalFFT =
      clCreateKernel(program, "oclKernel_verticalFFT", &status);
  CHECK_ERROR(status);

  cl_kernel kernel_horizontalFFT =
      clCreateKernel(program, "oclKernel_horizontalFFT", &status);
  CHECK_ERROR(status);

  error = clEnqueueWriteBuffer(
      queue, clSrc, CL_TRUE, 0,
      (sizeof(int32_t) * (SrcPitch * (height + IMAGE_PAD + IMAGE_PAD))), Src, 0,
      NULL, NULL);
  CHECK_ERROR(error);

  error =
      clEnqueueWriteBuffer(queue, clW64, CL_TRUE, 0,
                           (sizeof(int16_t) * 16 * 6 * 2), W64, 0, NULL, NULL);
  CHECK_ERROR(error);

  error = clEnqueueWriteBuffer(queue, clOut_idx, CL_TRUE, 0,
                               (sizeof(int32_t) * 64), Out_idx, 0, NULL, NULL);
  CHECK_ERROR(error);

  error = clEnqueueWriteBuffer(queue, cltwiddle_factor_64, CL_TRUE, 0,
                               sizeof(twiddle_factor_type) * 96 * 2, 
                               twiddle_factor_64, 0, NULL, NULL);
  CHECK_ERROR(error);

  CHECK(clSetKernelArg(kernel_verticalFFT, 0, sizeof(cl_mem), (void *)&clSrc));
  CHECK(
      clSetKernelArg(kernel_verticalFFT, 1, sizeof(cl_int), (void *)&SrcPitch));
  CHECK(clSetKernelArg(kernel_verticalFFT, 2, sizeof(cl_mem),
                       (void *)&clDstPass1Re));
  CHECK(clSetKernelArg(kernel_verticalFFT, 3, sizeof(cl_mem),
                       (void *)&clDstPass1Im));
  CHECK(
      clSetKernelArg(kernel_verticalFFT, 4, sizeof(cl_int), (void *)&DstPitch));
  CHECK(clSetKernelArg(kernel_verticalFFT, 5, sizeof(cl_int), (void *)&width));
  CHECK(clSetKernelArg(kernel_verticalFFT, 6, sizeof(cl_int), (void *)&height));
  CHECK(
      clSetKernelArg(kernel_verticalFFT, 7, sizeof(cl_int), (void *)&dstWidth));
  CHECK(clSetKernelArg(kernel_verticalFFT, 8, sizeof(cl_int),
                       (void *)&dstHeight));
  CHECK(clSetKernelArg(kernel_verticalFFT, 9, sizeof(cl_int), (void *)&TileW));
  CHECK(clSetKernelArg(kernel_verticalFFT, 10, sizeof(cl_int), (void *)&TileH));
  CHECK(clSetKernelArg(kernel_verticalFFT, 11, sizeof(cl_mem), (void *)&clW64));
  CHECK(clSetKernelArg(kernel_verticalFFT, 12, sizeof(cl_mem),
                       (void *)&clOut_idx));
  CHECK(clSetKernelArg(kernel_verticalFFT, 13, local_src_size, (void *)NULL));
  CHECK(clSetKernelArg(kernel_verticalFFT, 14, local_dst_size_pass1,
                       (void *)NULL));
  CHECK(clSetKernelArg(kernel_verticalFFT, 15, local_dst_size_pass1,
                       (void *)NULL));
  CHECK(clSetKernelArg(kernel_verticalFFT, 16, 64*16*sizeof(cl_int),
                       (void *)NULL));
  CHECK(clSetKernelArg(kernel_verticalFFT, 17, 64*16*sizeof(cl_int),
                       (void *)NULL));
  CHECK(clSetKernelArg(kernel_verticalFFT, 18, 64*16*sizeof(cl_int),
                       (void *)NULL));
  CHECK(clSetKernelArg(kernel_verticalFFT, 19, 64*16*sizeof(cl_int),
                       (void *)NULL));

  CHECK(clSetKernelArg(kernel_horizontalFFT, 0, sizeof(cl_mem),
                       (void *)&clDstPass1Re));
  CHECK(clSetKernelArg(kernel_horizontalFFT, 1, sizeof(cl_mem),
                       (void *)&clDstPass1Im));
  CHECK(clSetKernelArg(kernel_horizontalFFT, 2, sizeof(cl_int),
                       (void *)&SrcPitch));
  CHECK(
      clSetKernelArg(kernel_horizontalFFT, 3, sizeof(cl_mem), (void *)&clDst));
  CHECK(clSetKernelArg(kernel_horizontalFFT, 4, sizeof(cl_int),
                       (void *)&DstPitch));
  CHECK(
      clSetKernelArg(kernel_horizontalFFT, 5, sizeof(cl_int), (void *)&width));
  CHECK(
      clSetKernelArg(kernel_horizontalFFT, 6, sizeof(cl_int), (void *)&height));
  CHECK(clSetKernelArg(kernel_horizontalFFT, 7, sizeof(cl_int),
                       (void *)&dstWidth));
  CHECK(clSetKernelArg(kernel_horizontalFFT, 8, sizeof(cl_int),
                       (void *)&dstHeight));
  CHECK(
      clSetKernelArg(kernel_horizontalFFT, 9, sizeof(cl_int), (void *)&TileW));
  CHECK(
      clSetKernelArg(kernel_horizontalFFT, 10, sizeof(cl_int), (void *)&TileH));
  CHECK(clSetKernelArg(kernel_horizontalFFT, 11, sizeof(cl_mem),
                       (void *)&cltwiddle_factor_64));
  CHECK(clSetKernelArg(kernel_horizontalFFT, 12, local_dst_size_pass1,
                       (void *)NULL));
  CHECK(clSetKernelArg(kernel_horizontalFFT, 13, local_dst_size_pass1,
                       (void *)NULL));
  CHECK(clSetKernelArg(kernel_horizontalFFT, 14, local_dst_size, (void *)NULL));
  CHECK(clSetKernelArg(kernel_horizontalFFT, 15, 64*64*4*sizeof(cl_int), 
                       (void *)NULL));

  CHECK(clEnqueueNDRangeKernel(queue, kernel_verticalFFT, 2, NULL, global,
                               local, 0, NULL, &event));
  CHECK(clEnqueueNDRangeKernel(queue, kernel_horizontalFFT, 2, NULL, global,
                               local, 0, NULL, &event));

  CHECK(clFinish(queue));

  int evt_status;
  clGetEventInfo(event, CL_EVENT_COMMAND_EXECUTION_STATUS, sizeof(evt_status),
                 &evt_status, NULL);
  CHECK_ERROR(evt_status);
                 
  CHECK(clEnqueueReadBuffer(queue, clDst, CL_TRUE, 0,
                            (sizeof(cl_int) * DstPitch * dstHeight), Dst, 0,
                            NULL, NULL));

  // CHECK(clEnqueueReadBuffer(queue, clDstPass1Re, CL_TRUE, 0,
  //                          (sizeof(cl_int) * (DstPitch>>1) * dstHeight),
  //                          Dst, 0, NULL, NULL));
  // CHECK(clEnqueueReadBuffer(queue, clDstPass1Im, CL_TRUE, 0,
  //                          (sizeof(cl_int) * (DstPitch>>1) * dstHeight),
  //                          Dst+(DstPitch>>1) * dstHeight, 0, NULL, NULL));

  CHECK(clFinish(queue));

  std::cout << "done!!" << std::endl;
}

void memcpy2d(void *dst, int dstpitch, void *src, int srcpitch, int width,
              int height) {
  for (int i = 0; i < height; i++)
    memcpy(((char *)dst) + i * dstpitch, ((char *)src) + i * srcpitch, width);
}

void run_reference(int32_t *SrcY, uint32_t SrcPitch, int32_t *DstY,
                   const uint32_t DstPitch, int32_t width, int32_t height,
                   int32_t dstWidth, int32_t dstHeight, int16_t *W64,
                   int32_t *Out_idx, int32_t *twiddle_factor_64) {

  int32_t *SrcTile = (int32_t *)malloc(sizeof(int32_t) * 64 * 64 * 2);
  int32_t *DstTile = (int32_t *)malloc(sizeof(int32_t) * 64 * 64 * 2);
  int32_t *Out_re = (int32_t *)malloc(sizeof(int32_t) * 64 * 64);
  int32_t *Out_im = (int32_t *)malloc(sizeof(int32_t) * 64 * 64);
  int32_t *pTemp = (int32_t *)malloc(sizeof(int32_t) * 64 * 64 * 4);
  for (int32_t indy = 0; indy < height; indy += OUTPUT_TILE_H) {
    for (int32_t indx = 0; indx < width; indx += OUTPUT_TILE_W) {
      int32_t *src_y, *dst_y;
      src_y = SrcY + (indx + indy * SrcPitch);
      dst_y = DstY + (indx + indy * DstPitch);

      memcpy2d(SrcTile, 64 * 2 * 4, src_y, IMAGE_W * 4, 64 * 4 * 2, 64);
      fft_radix4_vertical_64(SrcTile, W64, Out_idx, pTemp, Out_re, Out_im);
      fft128_horizontal_complex_Cref(Out_re, Out_im, DstTile);
      memcpy2d(dst_y, IMAGE_W * 4, DstTile, 64 * 2 * 4, 64 * 4 * 2, 64);
    }
  }
}

int32_t compareIDFTvsInput(int32_t *Input, int32_t *Out_FFT, int32_t SrcPitch,
                           int32_t width, int32_t height) {
  int32_t maxErrorReal = 0;
  int32_t maxErrorImag = 0;
  int refReal, ivpReal, refImag, ivpImag;
  fCOMPLEX Gx[64 * 64];
  for (int32_t indy = 0; indy < height; indy += OUTPUT_TILE_H) {
    for (int32_t indx = 0; indx < width; indx += OUTPUT_TILE_W) {
      int32_t *fft, *in;
      fft = Out_FFT + (indx + indy * SrcPitch);
      in = Input + (indx + indy * SrcPitch);

      // inverse reference DFT computation
      for (int32_t j = 0; j < 64; j++) {
        for (int32_t i = 0; i < 64; i++) {
          Gx[(j * 64) + i].real =
              ((double)fft[(j * SrcPitch) + (2 * i)]) * 4096.0;
          Gx[(j * 64) + i].imag =
              ((double)fft[(j * SrcPitch) + (2 * i) + 1]) * 4096.0;
        }
      }
      idftDoublePass(64, Gx);

      // compare inverse DFT output with the original input

      for (int32_t j = 0; j < 64; j++) {
        for (int32_t i = 0; i < 64; i++) {

          refReal = (in[(j * SrcPitch) + (2 * i)] + (1 << 22)) >> 23;
          refImag = (in[(j * SrcPitch) + (2 * i) + 1] + (1 << 22)) >> 23;
          ivpReal = (int)Gx[(j * 64) + i].real;
          ivpReal = (ivpReal + (1 << 22)) >> 23;
          ivpImag = (int)Gx[(j * 64) + i].imag;
          ivpImag = (ivpImag + (1 << 22)) >> 23;

          if (abs(refReal - ivpReal) > maxErrorReal)
            maxErrorReal = abs(refReal - ivpReal);
          if (abs(refImag - ivpImag) > maxErrorImag)
            maxErrorImag = abs(refImag - ivpImag);
        }
      }
    }
  }
  return (maxErrorImag + maxErrorReal);
}

int main() {
  // Grey-scale frame
  int32_t *gsrc;
  int32_t *gdst;
  int32_t *gdst_ref;

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
  gsrc = (int32_t *)malloc(srcStride * (srcHeight + IMAGE_PAD + IMAGE_PAD) *
                           srcBytes);
  gdst = (int32_t *)malloc(dstWidth * dstHeight * dstBytes);
  gdst_ref = (int32_t *)malloc(dstWidth * dstHeight * dstBytes);
  uint8_t *gmem =
      (uint8_t *)malloc(srcStride * (srcHeight + IMAGE_PAD + IMAGE_PAD) * 1);
  if (!gsrc || !gdst || !gdst_ref || !gmem) {
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
    ptr = gmem + (indy * srcStride) + IMAGE_PAD;
    fread(ptr, 1, srcWidth, fpIn);
  }
  fclose(fpIn);

  for (indy = IMAGE_PAD; indy < (srcHeight * srcStride); indy++) {
    gsrc[indy] = gmem[indy] * (1 << 23);
  }

  int32_t n = 64;
  int16_t W64[16 * 6 * 2];
  int32_t Out_idx[64];
 
  init_radix4Int_64(n, W64, Out_idx);

  run_reference(gsrc, srcStride, gdst_ref, dstWidth, srcWidth, srcHeight,
                dstWidth, dstHeight, W64, Out_idx, (int32_t*)twiddle_factor_64);
#if NATIVE_KERNEL || XT_OCL_EXT
  launch_ocl_kernel(gsrc, srcStride, gdst, dstWidth, srcWidth, srcHeight,
                    dstWidth, dstHeight, W64, Out_idx, twiddle_factor_64);
#else
	twiddle_factor_type tw_factors_rearranged[96 * 2];

  for (int x = 0; x < 96 * 2; x += 32) {
    for (int y = 0; y < 16; y++) {
      tw_factors_rearranged[x + y] = twiddle_factor_64[x + y * 2];
      tw_factors_rearranged[x + y + 16] = twiddle_factor_64[x + y * 2 + 1];
    }
  }
  launch_ocl_kernel(gsrc, srcStride, gdst, dstWidth, srcWidth, srcHeight,
                    dstWidth, dstHeight, W64, Out_idx, tw_factors_rearranged);
#endif

  int32_t Compare =
      compareIDFTvsInput(gsrc, gdst, srcStride, srcWidth, srcHeight);

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
  if (Compare == 0 
#if !NATIVE_KERNEL && !XT_OCL_EXT
  && memcmp(gdst, gdst_ref, dstWidth * dstHeight * dstBytes) == 0
#endif
  ) 
  {
    printf("PASS \n");
  } else {
    printf("FAIL \n");
  }

  return 0;
}
