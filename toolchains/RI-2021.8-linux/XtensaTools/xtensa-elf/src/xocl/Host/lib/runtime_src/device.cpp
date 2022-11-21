/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED. 
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of 
 * Cadence Design Systems Inc. Any rights to use, modify, and create 
 * derivative works of this file are set forth under the terms of your 
 * license agreement with Cadence Design Systems, Inc.
 */

#include "cl_internal_objects.h"
#include "mxpa_profile.h"
#include "mxpa_common.h"

cl_int _cl_device_id::getInfo(cl_device_info param_name,
                              size_t param_value_size,
                              void *param_value, size_t *param_value_size_ret) {
  switch (param_name) {
  case CL_DEVICE_TYPE:
    return setParam<cl_device_type>(param_value, XOCL_DEVICE_TYPE,
                                    param_value_size, param_value_size_ret);

  case CL_DEVICE_VENDOR_ID:
    return setParam<cl_uint>(param_value, XOCL_VENDOR_ID,
                             param_value_size, param_value_size_ret);

  case CL_DEVICE_MAX_COMPUTE_UNITS:
    return setParam<cl_uint>(param_value, getProcCount(),
                             param_value_size, param_value_size_ret);

  case CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS:
    return setParam<cl_uint>(param_value, 3,
                             param_value_size, param_value_size_ret);

  case CL_DEVICE_MAX_WORK_ITEM_SIZES: {
    ID3 max_work_item_size(1024, 1024, 1024);
    return setParam<ID3>(param_value, max_work_item_size,
                         param_value_size, param_value_size_ret);
  }

  case CL_DEVICE_MAX_WORK_GROUP_SIZE:
    return setParam<size_t>(param_value, 1024,
                            param_value_size, param_value_size_ret);

  case CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR:
    return setParam<cl_uint>(param_value,
                             XOCL_DEVICES_PREFERRED_VECTOR_WIDTH_CHAR,
                             param_value_size, param_value_size_ret);

  case CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT:
    return setParam<cl_uint>(param_value,
                             XOCL_DEVICES_PREFERRED_VECTOR_WIDTH_SHORT,
                             param_value_size, param_value_size_ret);

  case CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT:
    return setParam<cl_uint>(param_value,
                             XOCL_DEVICES_PREFERRED_VECTOR_WIDTH_INT,
                             param_value_size, param_value_size_ret);

  case CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG:
    return setParam<cl_uint>(param_value,
                             XOCL_DEVICES_PREFERRED_VECTOR_WIDTH_LONG,
                             param_value_size, param_value_size_ret);

  case CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT:
    return setParam<cl_uint>(param_value,
                             XOCL_DEVICES_PREFERRED_VECTOR_WIDTH_FLOAT,
                             param_value_size, param_value_size_ret);

  case CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE:
    return setParam<cl_uint>(param_value,
                             XOCL_DEVICES_PREFERRED_VECTOR_WIDTH_DOUBLE,
                             param_value_size, param_value_size_ret);

  case CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF:
    return setParam<cl_uint>(param_value,
                             XOCL_DEVICES_PREFERRED_VECTOR_WIDTH_HALF,
                             param_value_size, param_value_size_ret);

  case CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR:
    return setParam<cl_uint>(param_value,
                             XOCL_DEVICES_NATIVE_VECTOR_WIDTH_CHAR,
                             param_value_size, param_value_size_ret);

  case CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT:
    return setParam<cl_uint>(param_value,
                             XOCL_DEVICES_NATIVE_VECTOR_WIDTH_SHORT,
                             param_value_size, param_value_size_ret);

  case CL_DEVICE_NATIVE_VECTOR_WIDTH_INT:
    return setParam<cl_uint>(param_value,
                             XOCL_DEVICES_NATIVE_VECTOR_WIDTH_INT,
                             param_value_size, param_value_size_ret);

  case CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG:
    return setParam<cl_uint>(param_value,
                             XOCL_DEVICES_NATIVE_VECTOR_WIDTH_LONG,
                             param_value_size, param_value_size_ret);

  case CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT:
    return setParam<cl_uint>(param_value,
                             XOCL_DEVICES_NATIVE_VECTOR_WIDTH_FLOAT,
                             param_value_size, param_value_size_ret);

  case CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE:
    return setParam<cl_uint>(param_value,
                             XOCL_DEVICES_NATIVE_VECTOR_WIDTH_DOUBLE,
                             param_value_size, param_value_size_ret);

  case CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF:
    return setParam<cl_uint>(param_value,
                             XOCL_DEVICES_NATIVE_VECTOR_WIDTH_HALF,
                             param_value_size, param_value_size_ret);

  case CL_DEVICE_MAX_CLOCK_FREQUENCY:
    return setParam<cl_uint>(param_value, 100,
                             param_value_size, param_value_size_ret);

  case CL_DEVICE_ADDRESS_BITS:
    return setParam<cl_uint>(param_value, getDeviceAddressBits(),
                             param_value_size, param_value_size_ret);

  case CL_DEVICE_MAX_MEM_ALLOC_SIZE:
    return setParam<cl_ulong>(param_value,
                              std::max((cl_ulong)(256 * 1024 * 1024 / 4), 
                              (cl_ulong)256 * 1024 * 1024),
                              param_value_size, param_value_size_ret);

  // Image Info, not supported
  case CL_DEVICE_IMAGE_SUPPORT:
    return setParam<cl_bool>(param_value, CL_FALSE,
                             param_value_size, param_value_size_ret);

  case CL_DEVICE_MAX_READ_IMAGE_ARGS:
    return setParam<cl_uint>(param_value, 128,
                             param_value_size, param_value_size_ret);

  case CL_DEVICE_MAX_WRITE_IMAGE_ARGS:
    return setParam<cl_uint>(param_value, 8,
                             param_value_size, param_value_size_ret);

  case CL_DEVICE_IMAGE2D_MAX_WIDTH:
  case CL_DEVICE_IMAGE2D_MAX_HEIGHT:
    return setParam<size_t>(param_value, 8192,
                            param_value_size, param_value_size_ret);

  case CL_DEVICE_IMAGE3D_MAX_WIDTH:
  case CL_DEVICE_IMAGE3D_MAX_HEIGHT:
  case CL_DEVICE_IMAGE3D_MAX_DEPTH:
    return setParam<size_t>(param_value, 2048,
                            param_value_size, param_value_size_ret);

  case CL_DEVICE_IMAGE_MAX_BUFFER_SIZE:
    return setParam<size_t>(param_value, 2048,
                            param_value_size, param_value_size_ret);

  case CL_DEVICE_IMAGE_MAX_ARRAY_SIZE:
    return setParam<size_t>(param_value, 2048,
                            param_value_size, param_value_size_ret);

  case CL_DEVICE_MAX_SAMPLERS:
    return setParam<cl_uint>(param_value, 16,
                             param_value_size, param_value_size_ret);

  case CL_DEVICE_MAX_PARAMETER_SIZE:
    return setParam<size_t>(param_value, 256,
                            param_value_size, param_value_size_ret);

  case CL_DEVICE_MEM_BASE_ADDR_ALIGN:
    return setParam<cl_uint>(param_value, 16 * sizeof(long) * 8,
                             param_value_size, param_value_size_ret);

  // FIXME set fp_config properly
  case CL_DEVICE_SINGLE_FP_CONFIG:
    return setParam<cl_device_fp_config>(param_value, CL_FP_DENORM |
                                         CL_FP_ROUND_TO_NEAREST |
                                         CL_FP_INF_NAN,
                                         param_value_size,
                                         param_value_size_ret);

  case CL_DEVICE_DOUBLE_FP_CONFIG:
    return setParam<cl_device_fp_config>(param_value, 0,
                                         param_value_size,
                                         param_value_size_ret);

  // global mem
  case CL_DEVICE_GLOBAL_MEM_CACHE_TYPE:
    return setParam<cl_device_mem_cache_type>(param_value, CL_NONE,
                                              param_value_size,
                                              param_value_size_ret);

  case CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE:
    return setParam<cl_uint>(param_value, 8,
                             param_value_size, param_value_size_ret);

  case CL_DEVICE_GLOBAL_MEM_CACHE_SIZE:
    return setParam<cl_ulong>(param_value, 1,
                              param_value_size, param_value_size_ret);

  case CL_DEVICE_GLOBAL_MEM_SIZE:
    return setParam<cl_ulong>(param_value, 256 * 1024 * 1024,
                              param_value_size, param_value_size_ret);

  // constant & local mem, etc.
  case CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE:
    return setParam<cl_ulong>(param_value, 64 * 1024,
                              param_value_size, param_value_size_ret);

  case CL_DEVICE_MAX_CONSTANT_ARGS:
    return setParam<cl_uint>(param_value, 8,
                             param_value_size, param_value_size_ret);

  case CL_DEVICE_LOCAL_MEM_TYPE:
    return setParam<cl_device_local_mem_type>(param_value, CL_LOCAL,
                                              param_value_size,
                                              param_value_size_ret);

  case CL_DEVICE_LOCAL_MEM_SIZE:
    return setParam<cl_ulong>(param_value, getLocalMemSize(),
                              param_value_size, param_value_size_ret);

  case CL_DEVICE_ERROR_CORRECTION_SUPPORT:
    return setParam<cl_bool>(param_value, CL_FALSE,
                             param_value_size, param_value_size_ret);

  case CL_DEVICE_HOST_UNIFIED_MEMORY:
    return setParam<cl_bool>(param_value, CL_TRUE,
                             param_value_size, param_value_size_ret);

  case CL_DEVICE_PROFILING_TIMER_RESOLUTION:
    return setParam<size_t>(param_value, 0,
                            param_value_size, param_value_size_ret);

  case CL_DEVICE_ENDIAN_LITTLE:
    return setParam<cl_bool>(param_value, CL_TRUE,
                             param_value_size, param_value_size_ret);

  case CL_DEVICE_AVAILABLE:
    return setParam<cl_bool>(param_value, CL_TRUE,
                             param_value_size, param_value_size_ret);

  case CL_DEVICE_COMPILER_AVAILABLE:
    return setParam<cl_bool>(param_value, CL_TRUE,
                             param_value_size, param_value_size_ret);

  case CL_DEVICE_LINKER_AVAILABLE:
    return setParam<cl_bool>(param_value, CL_FALSE,
                             param_value_size, param_value_size_ret);

  case CL_DEVICE_EXECUTION_CAPABILITIES:
    return setParam<cl_device_exec_capabilities>(param_value, CL_EXEC_KERNEL,
                                                 param_value_size,
                                                 param_value_size_ret);

  case CL_DEVICE_QUEUE_PROPERTIES:
    return setParam<cl_command_queue_properties>(param_value,
                                                 CL_QUEUE_PROFILING_ENABLE,
                                                 param_value_size,
                                                 param_value_size_ret);

  case CL_DEVICE_BUILT_IN_KERNELS:
    return setParam<cl_char>(param_value, '\0', // TODO 
                             param_value_size, param_value_size_ret);

  case CL_DEVICE_PLATFORM:
    return setParam<cl_platform_id>(param_value, getPlatform(),
                                    param_value_size, param_value_size_ret);

  case CL_DEVICE_NAME:
    return setParamStr(param_value, XOCL_DEVICE_NAME, param_value_size,
                       param_value_size_ret);

  case CL_DEVICE_VENDOR:
    return setParamStr(param_value, XOCL_PLATFORM_VENDOR, param_value_size,
                       param_value_size_ret);

  case CL_DRIVER_VERSION:
    return setParamStr(param_value, XOCL_DRIVER_VERSION, param_value_size,
                       param_value_size_ret);

  case CL_DEVICE_PROFILE:
    return setParamStr(param_value, XOCL_PLATFORM_PROFILE, param_value_size,
                       param_value_size_ret);

  case CL_DEVICE_VERSION:
    return setParamStr(param_value, XOCL_DEVICE_VERSION, param_value_size,
                       param_value_size_ret);

  case CL_DEVICE_OPENCL_C_VERSION:
    return setParamStr(param_value, XOCL_DEVICE_OCL_VERSION, param_value_size,
                       param_value_size_ret);

  case CL_DEVICE_EXTENSIONS:
    return setParamStr(param_value, XOCL_DEVICE_EXTENSIONS, param_value_size,
                       param_value_size_ret);

  case CL_DEVICE_PRINTF_BUFFER_SIZE:
    // Maximum size of the internal buffer that holds the output of printf
    return setParam<size_t>(param_value, getPrintfBufferSize(),
                            param_value_size, param_value_size_ret);

  case CL_DEVICE_PREFERRED_INTEROP_USER_SYNC:
    return setParam<cl_bool>(param_value, CL_TRUE,
                             param_value_size, param_value_size_ret);

  case CL_DEVICE_PARENT_DEVICE:
    return setParam<cl_device_id>(param_value, NULL,
                                  param_value_size, param_value_size_ret);

  case CL_DEVICE_PARTITION_MAX_SUB_DEVICES:
    return setParam<cl_uint>(param_value, getProcCount(),
                             param_value_size, param_value_size_ret);

  case CL_DEVICE_PARTITION_TYPE: {
    int num = 0;
    while (getSubDeviceType()[num] != 0)
      ++num;
    ++num;
    if (param_value_size <
        sizeof(cl_device_partition_property) * num && param_value)
      return CL_INVALID_VALUE;
    if (param_value)
      memcpy(param_value, (void*)getSubDeviceType(),
             sizeof(cl_device_partition_property *) * num);
    if (param_value_size_ret)
      *param_value_size_ret = sizeof(cl_device_partition_property) * num;
    return CL_SUCCESS;
  }

  case CL_DEVICE_PARTITION_PROPERTIES: {
    if (param_value_size <
        sizeof(cl_device_partition_property) * 2 && param_value)
      return CL_INVALID_VALUE;
    if (param_value) {
      ((cl_device_partition_property *)param_value)[0] = 
                                       CL_DEVICE_PARTITION_EQUALLY;
      ((cl_device_partition_property *)param_value)[1] = 
                                       CL_DEVICE_PARTITION_BY_COUNTS;
    }
    if (param_value_size_ret)
      *param_value_size_ret = sizeof(cl_device_partition_property) * 2;
    return CL_SUCCESS;
  }

  case CL_DEVICE_PARTITION_AFFINITY_DOMAIN:
    return setParam<cl_device_affinity_domain>(param_value, 0,
                                               param_value_size,
                                               param_value_size_ret);

  // always return 1 for a root-level device
  case CL_DEVICE_REFERENCE_COUNT:
    return setParam<cl_uint>(param_value, 1,
                             param_value_size, param_value_size_ret);

  // deprecated in 1.2
  case CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE:
    return setParam<cl_uint>(param_value, 16 * sizeof(long) * 8,
                             param_value_size, param_value_size_ret);

  default:
    return CL_INVALID_VALUE;
  }
}
