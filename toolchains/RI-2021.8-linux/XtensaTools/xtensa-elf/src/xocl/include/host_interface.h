/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED. 
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of 
 * Cadence Design Systems Inc. Any rights to use, modify, and create 
 * derivative works of this file are set forth under the terms of your 
 * license agreement with Cadence Design Systems, Inc.
 */

#ifndef __HOST_INTERFACE_H__
#define __HOST_INTERFACE_H__

// This defines macros and types that are shared between the host and the
// device.

#define XOCL_MAX_DEVICE_CACHE_LINE_SIZE (128)
#define XOCL_NULL_PTR_BUF               (0xffffffff)

enum {
  XOCL_CMD_SET_KERNEL           = 1,
  XOCL_CMD_EXE_KERNEL           = 2,
  XOCL_CMD_QUERY_LOCAL_MEM_SIZE = 3,
};

typedef struct {
  unsigned buf_id;
  unsigned buf_size;
  unsigned offset;
} xocl_krnl_arg;

typedef struct {
  unsigned CMD;
  unsigned char proc_num;
  unsigned char proc_id;
  unsigned char master_proc_id;
  unsigned long long kernel_id;
  unsigned long long id;
  unsigned group_id[3];
  unsigned global_size[3];
  unsigned local_size[3];
  unsigned global_offset[3];
  unsigned work_dim;
  unsigned syn_buf_id;
  unsigned syn_buf_size;
  unsigned printf_buf_id;
  unsigned printf_buf_size;
  unsigned num_args;
  xocl_krnl_arg arguments[1];
} xocl_launch_kernel_cmd;

typedef struct {
  unsigned CMD;
} xocl_generic_cmd;

#endif // __HOST_INTERFACE_H__
