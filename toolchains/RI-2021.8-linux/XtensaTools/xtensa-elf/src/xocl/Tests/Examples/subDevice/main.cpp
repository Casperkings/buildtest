// This code tests sub device functionality. It computes Y = (A * B) + (A * B) +
// ... It assumes that there are two or more CUs in this device.
// 1. Creates as many sub devices as total number of CUs on device with one
// compute unit each. On each of the sub-device it computes A*B
// 2. Creates one sub device with total number of CUs on the device. On it
// compute addition of the products
// 3. Context is created with these (TOTAL_CUs + 1) sub-devices and command
// queue for each sub-device
// 4. Launch Mul0, Mul1 ...  Muln kernel on the sub-devices which are created
// with one CU each and clFlush is called to launch the kernel
// 5. Launch Add kernel in a loop to sum all the products

#include <CL/opencl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <fstream>
#include <assert.h>

#include "utils.h"

#define LOCAL_SIZE 2048

#define CHECK_ERROR(err)                                                       \
  do {                                                                         \
    if (err != CL_SUCCESS) {                                                   \
      printf("Failed at #%d, err=%d\n", __LINE__, err);                        \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

#define MAX_NUM_DEVICES 8
const unsigned int num_elem = 8 * LOCAL_SIZE;
int input1[num_elem];
int input2[num_elem];
int outA[MAX_NUM_DEVICES][num_elem];
int outB[num_elem];
int refout[num_elem];

cl_command_queue cmdq[MAX_NUM_DEVICES] = {0};

static void referenceRun(int cu_nums) {
  unsigned i;
  for (i = 0; i < num_elem; i++)
    refout[i] = (input1[i] * input2[i]) * cu_nums;
}

int main() {
  unsigned i;
  for (i = 0; i < num_elem; i++) {
    input1[i] = i;
    input2[i] = i + num_elem;
    outB[i] = 0;
    refout[i] = 0;
  }

  cl_platform_id id;
  int error = clGetPlatformIDs(1, &id, NULL);
  CHECK_ERROR(error);

  cl_device_id dev_id[MAX_NUM_DEVICES];
  cl_uint num_devices = MAX_NUM_DEVICES;
  error =
      clGetDeviceIDs(id, CL_DEVICE_TYPE_CPU, num_devices, dev_id, &num_devices);
  CHECK_ERROR(error);
  printf("Number of devices %d \n", num_devices);
  printf("Device ID %p \n", dev_id[0]);

  // query for number of CUs
  cl_uint cu_nums;
  error = clGetDeviceInfo(dev_id[0], CL_DEVICE_MAX_COMPUTE_UNITS,
                          sizeof(cu_nums), &cu_nums, NULL);
  CHECK_ERROR(error);
  printf("CL_DEVICE_MAX_COMPUTE_UNITS %d \n", cu_nums);

  // create sub devices with 1 CU
  cl_device_partition_property
      properties[MAX_NUM_DEVICES +
                 3]; //{ CL_DEVICE_PARTITION_BY_COUNTS, 1, 1, 1,
                     //CL_DEVICE_PARTITION_BY_COUNTS_LIST_END, 0 };
  properties[0] = CL_DEVICE_PARTITION_BY_COUNTS;
  for (i = 0; i < cu_nums; i++) {
    properties[i + 1] = 1;
  }
  properties[i + 1] = CL_DEVICE_PARTITION_BY_COUNTS_LIST_END;
  properties[i + 2] = 0;
  cl_device_id sub_device_id_1cu[MAX_NUM_DEVICES];
  cl_uint num_dev_ids_1cu;
  printf("\nCreate sub-devices with 1 CU each\n");
  error = clCreateSubDevices(dev_id[0], properties, MAX_NUM_DEVICES,
                             sub_device_id_1cu, &num_dev_ids_1cu);
  CHECK_ERROR(error);

  // print sub-device IDs
  printf("Number of sub-devices with 1 CU each %d\n", num_dev_ids_1cu);
  for (i = 0; i < num_dev_ids_1cu; i++) {
    printf("sub-device #%d %p\n", i, sub_device_id_1cu[i]);
  }

  // create sub devices with all number CUs
  cl_device_partition_property properties1[] = {
      CL_DEVICE_PARTITION_BY_COUNTS, -1, CL_DEVICE_PARTITION_BY_COUNTS_LIST_END,
      0};
  properties1[1] = cu_nums;
  cl_device_id sub_device_id_max_cu[MAX_NUM_DEVICES];
  cl_uint num_dev_ids_max_cu;
  printf("\nCreate a sub-device with %d CUs (maximum)\n", cu_nums);
  error = clCreateSubDevices(dev_id[0], properties1, MAX_NUM_DEVICES,
                             sub_device_id_max_cu, &num_dev_ids_max_cu);
  CHECK_ERROR(error);

  // print sub-device IDs
  for (i = 0; i < num_dev_ids_max_cu; i++) {
    printf("sub-device #%d %p\n", i, sub_device_id_max_cu[i]);
  }

  // fill the device IDs with devices containing 1CU and max CUs
  cl_device_id sub_device_ids[MAX_NUM_DEVICES];
  for (i = 0; i < cu_nums; i++) {
    sub_device_ids[i] = sub_device_id_1cu[i];
  }
  sub_device_ids[cu_nums] = sub_device_id_max_cu[0];

  // create context for all sub-devices
  cl_context ctx =
      clCreateContext(NULL, (cu_nums + 1), sub_device_ids, NULL, NULL, &error);
  CHECK_ERROR(error);

  {
    // query devices within context
    cl_uint check_num_sub_devices = 0;
    cl_device_id check_sub_devices[MAX_NUM_DEVICES] = {0};

    printf("\n***** Check context *****\n");
    // Get number of devices.
    error = clGetContextInfo(ctx, CL_CONTEXT_NUM_DEVICES, sizeof(cl_uint),
                             &check_num_sub_devices, 0);
    CHECK_ERROR(error);
    printf("Context check CL_CONTEXT_NUM_DEVICES %d\n", check_num_sub_devices);

    // Get the list of devices.
    error = clGetContextInfo(ctx, CL_CONTEXT_DEVICES,
                             check_num_sub_devices * sizeof(cl_device_id),
                             &check_sub_devices[0], 0);
    CHECK_ERROR(error);
    for (i = 0; i < check_num_sub_devices; i++) {
      printf("sub-devices in context@%d %p\n", i, check_sub_devices[i]);
    }
  }

  // create command queue for device with 1 CUs each
  for (i = 0; i < cu_nums + 1; i++) {
    cmdq[i] = clCreateCommandQueue(ctx, sub_device_ids[i], 0, &error);
    CHECK_ERROR(error);
  }

  const char *pSource = loadKernel("simpleMath.cl");
  cl_program prog = clCreateProgramWithSource(ctx, 1, &pSource, NULL, &error);
  CHECK_ERROR(error);
  error = clBuildProgram(prog, 0, NULL, NULL, NULL, NULL);
  CHECK_ERROR(error);

  cl_mem input1Buf = clCreateBuffer(ctx, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                                    sizeof(input1), (void *)input1, &error);
  CHECK_ERROR(error);

  cl_mem input2Buf = clCreateBuffer(ctx, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                                    sizeof(input2), (void *)input2, &error);
  CHECK_ERROR(error);

  cl_mem clOutA[MAX_NUM_DEVICES];
  for (i = 0; i < cu_nums; i++) {
    clOutA[i] = clCreateBuffer(ctx, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR,
                               sizeof(outA[i]), NULL, &error);
    CHECK_ERROR(error);
  }

  cl_kernel kernelA[MAX_NUM_DEVICES];
  for (i = 0; i < cu_nums; i++) {
    char kernel_name[16];
    sprintf(kernel_name, "vecMul%d", i);
    kernelA[i] = clCreateKernel(prog, kernel_name, &error);
    CHECK_ERROR(error);
  }

  for (i = 0; i < cu_nums; i++) {
    error = clSetKernelArg(kernelA[i], 0, sizeof(cl_mem), &input1Buf);
    CHECK_ERROR(error);
    error = clSetKernelArg(kernelA[i], 1, sizeof(cl_mem), &input2Buf);
    CHECK_ERROR(error);
    error = clSetKernelArg(kernelA[i], 2, sizeof(cl_mem), &clOutA[i]);
    CHECK_ERROR(error);
  }

  printf("\n***** Launch Kernels *****\n");
  size_t global[1] = {num_elem / 16};
  size_t local[1] = {LOCAL_SIZE / 16};

  for (i = 0; i < cu_nums; i++) {
    printf("Launch kernel A on sub-device ID = %p\n", sub_device_ids[i]);
    error = clEnqueueNDRangeKernel(cmdq[i], kernelA[i], 1, NULL, global, local,
                                   0, NULL, NULL);
    CHECK_ERROR(error);
    clFlush(cmdq[i]);
  }

  // wait for all 1 CU device command queues
  for (i = 0; i < cu_nums; i++) {
    clFinish(cmdq[i]);
  }

  // call kernel B, cu_num times
  cl_kernel kernelB = clCreateKernel(prog, "vecAdd", &error);
  CHECK_ERROR(error);
  cl_mem clOutB = clCreateBuffer(ctx, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR,
                                 sizeof(outB), NULL, &error);
  CHECK_ERROR(error);

  for (i = 0; i < cu_nums; i++) {
    error = clSetKernelArg(kernelB, 0, sizeof(cl_mem), &clOutA[i]);
    CHECK_ERROR(error);
    error = clSetKernelArg(kernelB, 1, sizeof(cl_mem), &clOutB);
    CHECK_ERROR(error);

    clEnqueueWriteBuffer(cmdq[cu_nums], clOutB, CL_TRUE, 0, sizeof(outB), outB,
                         0, NULL, NULL);

    printf("Launch kernel B on sub-device ID = %p\n", sub_device_ids[cu_nums]);
    error = clEnqueueNDRangeKernel(cmdq[cu_nums], kernelB, 1, NULL, global,
                                   local, 0, NULL, NULL);
    CHECK_ERROR(error);
    clFinish(cmdq[cu_nums]);
    error = clEnqueueReadBuffer(cmdq[cu_nums], clOutB, CL_TRUE, 0, sizeof(outB),
                                outB, 0, NULL, NULL);
    CHECK_ERROR(error);
  }

  referenceRun(cu_nums);

  int result = CL_SUCCESS;
  for (i = 0; i < num_elem; i++) {
    if (outB[i] != refout[i]) {
      result = CL_INVALID_VALUE;
      printf("!!! ref=%d out=%d at (%d)\n", refout[i], outB[i], i);
      break;
    }
  }

  if (i == num_elem)
    printf("PASS\n\n");
  else
    printf("FAIL\n\n");

  for (i = 0; i < cu_nums; i++) {
    clReleaseKernel(kernelA[i]);
    clReleaseMemObject(clOutA[i]);
    clReleaseCommandQueue(cmdq[i]);
  }
  clReleaseKernel(kernelB);
  clReleaseMemObject(clOutB);
  clReleaseCommandQueue(cmdq[cu_nums]);

  clReleaseProgram(prog);
  clReleaseContext(ctx);

  if (result != CL_SUCCESS)
    return -1;

  return 0;
}
