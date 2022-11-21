#include <CL/cl.h>
#include <string.h>
#include <iostream>
#include <assert.h>
#include <stdio.h>
#include "utils.h"

#define NUM  1024
#define SIZE (NUM * sizeof(int))

#define PRINT_MAT 0

#define CHECK_ERROR(err)                                                       \
  do {                                                                         \
    if (err != CL_SUCCESS) {                                                   \
      printf("Failed at #%d, err=%d\n", __LINE__, err);                        \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

float *a, *b, *cref, *copt;
unsigned int wA, hA, wB, hB, wC, hC, l0, l1;

void matrix_multiply_ref() {
  for (unsigned i = 0; i < hC; i += 1) {
    for (unsigned j = 0; j < wC; j += 1) {
      cref[i * wC + j] = 0;
      for (unsigned k = 0; k < wA; k += 1) {
        cref[i * wC + j] += (a[i * wA + k] * b[k * wB + j]);
      }
    }
  }
}

void matrix_multiply_ocl() {
  cl_int status;

  cl_platform_id *platform = NULL;
  cl_uint PlatformCout = 0;
  CHECK_ERROR(clGetPlatformIDs(0, NULL, &PlatformCout));
  platform = (cl_platform_id *)malloc(PlatformCout * sizeof(cl_platform_id));
  CHECK_ERROR(clGetPlatformIDs(PlatformCout, platform, NULL));

  cl_device_id *devices = NULL;

  cl_uint num_devices = 0;
  CHECK_ERROR(
      clGetDeviceIDs(platform[0], CL_DEVICE_TYPE_ALL, 0, NULL, &num_devices));
  devices = (cl_device_id *)malloc(num_devices * sizeof(cl_device_id));
  CHECK_ERROR(clGetDeviceIDs(platform[0], CL_DEVICE_TYPE_ALL, num_devices,
                             devices, NULL));

  char deviceName[1024];
  CHECK_ERROR(clGetDeviceInfo(devices[0], CL_DEVICE_NAME, sizeof(deviceName),
                              deviceName, NULL));

  cl_context context =
      clCreateContext(NULL, num_devices, devices, NULL, NULL, &status);
  CHECK_ERROR(status);
  cl_command_queue queue =
      clCreateCommandQueue(context, devices[0], 0, &status);
  CHECK_ERROR(status);

  cl_mem clMatA = clCreateBuffer(context, CL_MEM_READ_ONLY,
                                 (sizeof(float) * hA * wA), NULL, &status);
  CHECK_ERROR(status);

  cl_mem clMatB = clCreateBuffer(context, CL_MEM_READ_ONLY,
                                 (sizeof(float) * hB * wB), NULL, &status);
  CHECK_ERROR(status);

  cl_mem clMatC = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
                                 (sizeof(float) * hC * wC), NULL, &status);
  CHECK_ERROR(status);

  const char *progSource = loadKernel("mm.cl");
  cl_program program =
      clCreateProgramWithSource(context, 1, &progSource, NULL, &status);
  CHECK_ERROR(status);

  CHECK_ERROR(clBuildProgram(program, 1, devices, NULL, NULL, NULL));

  cl_kernel kernel = clCreateKernel(program, "matMulAsync", &status);
  CHECK_ERROR(status);

  CHECK_ERROR(clEnqueueWriteBuffer(
      queue, clMatA, CL_TRUE, 0, (sizeof(float) * wA * hA), a, 0, NULL, NULL));
  CHECK_ERROR(clEnqueueWriteBuffer(
      queue, clMatB, CL_TRUE, 0, (sizeof(float) * wB * hB), b, 0, NULL, NULL));

  CHECK_ERROR(clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&clMatA));
  CHECK_ERROR(clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&clMatB));
  CHECK_ERROR(clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&clMatC));
  CHECK_ERROR(clSetKernelArg(kernel, 3, sizeof(cl_int), (void *)&wA));
  CHECK_ERROR(clSetKernelArg(kernel, 4, sizeof(cl_int), (void *)&wB));
  CHECK_ERROR(clSetKernelArg(kernel, 5, sizeof(cl_int), (void *)&wC));

  CHECK_ERROR(
      clSetKernelArg(kernel, 6, (64 * wA * sizeof(cl_float)), (void *)NULL));
  CHECK_ERROR(
      clSetKernelArg(kernel, 7, (64 * hB * sizeof(cl_float)), (void *)NULL));

  size_t local[2] = {l0, l1};
  size_t global[2] = {hC / 64, wC / 64};

  std::cout << "Launch Kernel " << global[0] << " X " << global[1] << "\n";
  CHECK_ERROR(clEnqueueNDRangeKernel(queue, kernel, 2, NULL, global, local, 0,
                                     NULL, NULL));

  CHECK_ERROR(clFlush(queue));
  CHECK_ERROR(clFinish(queue));

  CHECK_ERROR(clEnqueueReadBuffer(queue, clMatC, CL_TRUE, 0,
                                  (sizeof(float) * wC * hC), copt, 0, NULL,
                                  NULL));
}

int main(int argc, char *argv[]) {
  int i, j;
  int error = 0;

  if (argc < 6) {
    std::cout << " Usage: <matMul> <dim1> <dim2> <dim3> <local size 0> <local "
                 "size 1>\n";
    return -1;
  }

  hA = hC = strtol(argv[1], NULL, 10);
  wA = hB = strtol(argv[2], NULL, 10);
  wB = wC = strtol(argv[3], NULL, 10);
  l0 = strtol(argv[4], NULL, 10);
  l1 = strtol(argv[5], NULL, 10);

  // init matrix A
  a = (float *)malloc(sizeof(float) * hA * wA);
  for (i = 0; i < (int)hA; i++) {
    for (j = 0; j < (int)wA; j++) {
      a[i * wA + j] = (2 * i - j) % 4;
    }
  }

#if PRINT_MAT
  std::cout << "Input A\n";
  for (i = 0; i < (int)hA; i++) {
    std::cout << "\n";
    for (j = 0; j < (int)wA; j++) {
      std::cout << a[i * wA + j] << " ";
    }
  }
#endif

  // init matrix B
  b = (float *)malloc(sizeof(float) * hB * wB);
  for (i = 0; i < (int)hB; i++) {
    for (j = 0; j < (int)wB; j++) {
      b[i * wB + j] = (-11 * i + 3 * j) % 4;
    }
  }

  cref = (float *)malloc(sizeof(float) * hC * wC);
  copt = (float *)malloc(sizeof(float) * hC * wC);
  memset(copt, 0, (sizeof(float) * hC * wC));

#if PRINT_MAT
  std::cout << "Input B\n";
  for (i = 0; i < (int)hB; i++) {
    std::cout << "\n";
    for (j = 0; j < (int)wB; j++) {
      std::cout << b[i * wB + j] << " ";
    }
  }
#endif

  matrix_multiply_ref();

#if PRINT_MAT
  std::cout << "Reference Output\n";
  for (i = 0; i < (int)hC; i++) {
    std::cout << "\n";
    for (j = 0; j < (int)wC; j++) {
      std::cout << cref[i * wC + j] << " ";
    }
  }
  std::cout << "\n";
#endif

  matrix_multiply_ocl();

#if PRINT_MAT
  std::cout << "OCL Output\n";
  for (i = 0; i < (int)hC; i++) {
    std::cout << "\n";
    for (j = 0; j < (int)wC; j++) {
      std::cout << copt[i * wC + j] << " ";
    }
  }
  std::cout << "\n";
#endif

  for (i = 0; i < (int)hC; i++) {
    for (j = 0; j < (int)wC; j++) {
      if (copt[i * wC + j] != cref[i * wC + j]) {
        error = 1;
        // break;
        std::cout << "Error " << i << " " << j << " " << copt[i * wC + j] << " "
                  << cref[i * wC + j] << "\n";
      }
      if (error)
        break;
    }
  }

  if (error == 0)
    std::cout << "\nPASS\n";
  else
    std::cout << "\nFAIL\n";
}
