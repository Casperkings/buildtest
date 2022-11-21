/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED. 
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of 
 * Cadence Design Systems Inc. Any rights to use, modify, and create 
 * derivative works of this file are set forth under the terms of your 
 * license agreement with Cadence Design Systems, Inc.
 */

#ifndef __XOCL_KERNEL_INFO_H__
#define __XOCL_KERNEL_INFO_H__

// these define is copied from 'CL/cl.h', please sync with it.
#define XOCL_CL_KERNEL_ARG_ADDRESS_GLOBAL   0x119B
#define XOCL_CL_KERNEL_ARG_ADDRESS_LOCAL    0x119C
#define XOCL_CL_KERNEL_ARG_ADDRESS_CONSTANT 0x119D
#define XOCL_CL_KERNEL_ARG_ADDRESS_PRIVATE  0x119E

typedef void (*XOCL_LAUNCH_KERNEL)(unsigned *, unsigned *, unsigned *, 
                                    unsigned *, unsigned, void const **);
typedef int (*XOCL_ASYNC_CALLBACKS)(unsigned *, unsigned *, unsigned *, 
                                    unsigned *, unsigned, void const **);
typedef int (*XOCL_GET_UPPER_LOWER)(unsigned, unsigned *, unsigned *, 
                                    unsigned *, unsigned *, unsigned, 
                                    void const **);
typedef int (*XOCL_GET_ARG_DIR)(unsigned);
typedef int (*XOCL_GET_ARG_NR)(void);
typedef int (*XOCL_GET_PAIRED_LOCAL)(unsigned);

// Describes kernel argument
typedef struct {
  unsigned int address_qualifier;
  unsigned int access_qualifier;
  const char *type_name;
  unsigned int type_qualifier;
  const char *arg_name;
  unsigned arg_size;
  char no_dma_promot;
  char local_async_copy;
} xocl_opencl_kernel_arg_info;

// Description of a kernel
typedef struct {
  const char *source_hash;
  const char *kernel_name;
  unsigned long long kernel_id;
  XOCL_LAUNCH_KERNEL launch_kernel;
  XOCL_GET_UPPER_LOWER get_upper[2];
  XOCL_GET_UPPER_LOWER get_lower[2];
  XOCL_GET_ARG_DIR get_arg_dir[2];
  XOCL_GET_ARG_NR  get_arg_nr[2];
  unsigned workgroup[3];
  xocl_opencl_kernel_arg_info *arg_info;
  XOCL_GET_PAIRED_LOCAL get_paired_local;
  XOCL_ASYNC_CALLBACKS async_copy_to_local[2];
  XOCL_ASYNC_CALLBACKS async_copy_to_global[2];
  unsigned char async_to_local_num;
  unsigned char async_to_global_num;
  unsigned char async_2d_to_local_num;
  unsigned char async_2d_to_global_num;
  unsigned char isCallPrintfFun;
  unsigned int reqd_wd_size;
} xocl_opencl_kernel_descriptor;

#endif // __XOCL_KERNEL_INFO_H__
