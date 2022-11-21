#include <CL/opencl.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include "utils.h"

#define LOCAL_SIZE 8192

#define CHECK_ERROR(err) \
    do { \
        if (err != CL_SUCCESS) { \
            printf("Failed at #%d, err=%d\n", __LINE__, err); \
            exit(1); \
        } \
    } while (0)

const unsigned num_elem = 128 * LOCAL_SIZE;
int input1[num_elem];
int input2[num_elem];
int output[num_elem];
int refout[num_elem];

static void referenceRun()
{
  for (unsigned i = 0; i < num_elem; i++)
    refout[i] = input1[i] + input2[i];
}

int main() 
{
  for (unsigned i = 0; i < num_elem; i++) {
    input1[i] = i;
    input2[i] = i+num_elem;
    output[i] = 0;
  }
  
  cl_platform_id id;
  int error = clGetPlatformIDs(1, &id, NULL);
  CHECK_ERROR(error);

  cl_device_id dev_id;
  error = clGetDeviceIDs(id, CL_DEVICE_TYPE_CPU, 1, &dev_id, NULL);
  CHECK_ERROR(error);

  cl_context ctx = clCreateContext(NULL, 1, &dev_id, NULL, NULL, &error);
  CHECK_ERROR(error);
  cl_command_queue cmdq = clCreateCommandQueue(ctx, dev_id, 0, &error);
  CHECK_ERROR(error);

  const char *pSource = loadKernel("vecAdd.cl");
  cl_program prog = clCreateProgramWithSource(ctx, 1, &pSource, NULL, &error);
  CHECK_ERROR(error);
  error = clBuildProgram(prog, 0, NULL, NULL, NULL, NULL);
  CHECK_ERROR(error);

  cl_mem input1Buf = clCreateBuffer(ctx, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                                    sizeof(input1), (void*)input1, &error);
  CHECK_ERROR(error);

  cl_mem input2Buf = clCreateBuffer(ctx, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                                    sizeof(input2), (void*)input2, &error);
  CHECK_ERROR(error);

  cl_mem outputBuf = clCreateBuffer(ctx, CL_MEM_WRITE_ONLY,
                                    sizeof(output), NULL, &error);
  CHECK_ERROR(error);

  cl_kernel kernel = clCreateKernel(prog, "vecAdd", &error);
  CHECK_ERROR(error);

  error = clSetKernelArg(kernel, 0, sizeof(cl_mem), &input1Buf);
  CHECK_ERROR(error);
  error = clSetKernelArg(kernel, 1, sizeof(cl_mem), &input2Buf);
  CHECK_ERROR(error);
  error = clSetKernelArg(kernel, 2, sizeof(cl_mem), &outputBuf);
  CHECK_ERROR(error);

  size_t global[1] = { num_elem/16 };
  size_t local[1] = { LOCAL_SIZE/16 };
  
  error = clEnqueueNDRangeKernel(cmdq, kernel, 1, NULL,
                                 global, local, 0, NULL, NULL);
  CHECK_ERROR(error);

  clFinish(cmdq);

  error = clEnqueueReadBuffer(cmdq, outputBuf, CL_TRUE, 0,
                              sizeof(output), output, 0, NULL, NULL);
  CHECK_ERROR(error);

  referenceRun();

  int result = CL_SUCCESS;
  unsigned i;
  for (i = 0; i < num_elem; i++) {
    if (output[i] != refout[i]) {
      result = CL_INVALID_VALUE;
      printf("!!! ref=%d out=%d at (%d)\n", refout[i], output[i], i);
	    break;
    }
  }
  
  if (i == num_elem)
	  printf("PASS\n");
  else
	  printf("FAIL\n");

  printf("\n\n\n");
  clReleaseKernel(kernel);
  clReleaseProgram(prog);

  clReleaseMemObject(input1Buf);
  clReleaseMemObject(input2Buf);
  clReleaseMemObject(outputBuf);
  clReleaseCommandQueue(cmdq);
  clReleaseContext(ctx);
  clReleaseDevice(dev_id);

  if (result != CL_SUCCESS)
    return -1;

  return 0;
}
