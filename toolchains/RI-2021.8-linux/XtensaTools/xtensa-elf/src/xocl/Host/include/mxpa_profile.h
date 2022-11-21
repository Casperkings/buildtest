/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED. 
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of 
 * Cadence Design Systems Inc. Any rights to use, modify, and create 
 * derivative works of this file are set forth under the terms of your 
 * license agreement with Cadence Design Systems, Inc.
 */

#ifndef __XOCL_PROFILE_H__
#define __XOCL_PROFILE_H__

#define XOCL_PLATFORM_NAME		   "XOCL"
#define XOCL_PLATFORM_VERSION	   "OpenCL 1.2 "
#define XOCL_PLATFORM_VENDOR	   "Cadence Design Systems, Inc."
#define XOCL_PLATFORM_EXTENSIONS "cl_khr_icd"
#define XOCL_ICD_SUFFIX_KHR      "_XOCL"
#define XOCL_DEVICE_TYPE         CL_DEVICE_TYPE_CPU

#define XOCL_PLATFORM_PROFILE    "EMBEDDED_PROFILE"

#define XOCL_DEVICE_NAME         "Xtensa DSP"
#define XOCL_DRIVER_VERSION      "1.2"
#define XOCL_DEVICE_VERSION      "OpenCL 1.2 XOCL"
#define XOCL_DEVICE_OCL_VERSION  "OpenCL C 1.2 "
#define XOCL_DEVICE_EXTENSIONS   "cl_khr_global_int32_base_atomics cl_khr_global_int32_extended_atomics cl_khr_local_int32_base_atomics cl_khr_local_int32_extended_atomics cl_khr_byte_addressable_store"

#define XOCL_PLATFORM_ID		     10
#define XOCL_CONTEXT_ID			     0
#define XOCL_DEVICE_ID			     1
#define XOCL_VENDOR_ID			     0
#define XOCL_BUILD_ERROR         "XOCL build error"
#define XOCL_BUILD_IN_PROGRESS   "XOCL build in progress"
#define XOCL_BUILD_SUCCESS       "XOCL build success"

#define XOCL_DEVICES_PREFERRED_VECTOR_WIDTH_CHAR   16
#define XOCL_DEVICES_PREFERRED_VECTOR_WIDTH_SHORT   8
#define XOCL_DEVICES_PREFERRED_VECTOR_WIDTH_INT     4
#define XOCL_DEVICES_PREFERRED_VECTOR_WIDTH_LONG    2
#define XOCL_DEVICES_PREFERRED_VECTOR_WIDTH_FLOAT   4
#define XOCL_DEVICES_PREFERRED_VECTOR_WIDTH_DOUBLE  0
#define XOCL_DEVICES_NATIVE_VECTOR_WIDTH_CHAR      16
#define XOCL_DEVICES_NATIVE_VECTOR_WIDTH_SHORT      8
#define XOCL_DEVICES_NATIVE_VECTOR_WIDTH_INT        4
#define XOCL_DEVICES_NATIVE_VECTOR_WIDTH_LONG       2
#define XOCL_DEVICES_NATIVE_VECTOR_WIDTH_FLOAT      4
#define XOCL_DEVICES_NATIVE_VECTOR_WIDTH_DOUBLE     0

// Half is internally represented as short 
#define XOCL_DEVICES_PREFERRED_VECTOR_WIDTH_HALF  \
        XOCL_DEVICES_PREFERRED_VECTOR_WIDTH_SHORT
#define XOCL_DEVICES_NATIVE_VECTOR_WIDTH_HALF     \
        XOCL_DEVICES_NATIVE_VECTOR_WIDTH_SHORT

#endif
