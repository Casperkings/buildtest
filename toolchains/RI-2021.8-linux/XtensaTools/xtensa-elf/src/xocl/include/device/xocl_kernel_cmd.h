/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED. 
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of 
 * Cadence Design Systems Inc. Any rights to use, modify, and create 
 * derivative works of this file are set forth under the terms of your 
 * license agreement with Cadence Design Systems, Inc.
 */

#ifndef __XOCL_KERNEL_CMD_H__
#define __XOCL_KERNEL_CMD_H__

#include "generic_container.h"
#include "host_interface.h"
#include "xocl_kernel_info.h"

struct __xocl_kernel_cmd_info;

typedef struct __xocl_buffer_arg_state {
  int lower, upper;
  int non_overlapped_lower, non_overlapped_upper;
  int is_promote;
  struct __xocl_kernel_cmd_info *parent; 
  struct __xocl_buffer_arg_state *paired_local;
  struct __buffer_arg_state *defined_by;
  unsigned char is_incorrect_query_result;
  struct __buffer_arg_state **users;
  unsigned char num_users;
  xocl_opencl_kernel_arg_info *arg_info;
  unsigned char arg_id;
  unsigned char buf_id;
  unsigned char dir;
  unsigned char cancelled_copy;
  unsigned char is_local;
  void *buf;
} xocl_buffer_arg_state;

typedef struct __xocl_kernel_cmd_info {
  xocl_launch_kernel_cmd *launchArg;
  char *grp_status;
  char processed;
  const xocl_opencl_kernel_descriptor *krnl_desc;
  unsigned char num_canceled_async_local_copy;
  unsigned char num_buf_args;
  xocl_buffer_arg_state *buf_arg_states;
  void const **krnl_args;
  struct xrp_buffer **krnl_args_xbuf;
  void **local_krnl_args;
  void **local_krnl_args_ptr;
  unsigned *promotable_bufs[2];
  unsigned num_promotable_bufs[2];
  unsigned *in_buf_id;
  unsigned *out_buf_id;
} xocl_kernel_cmd_info;

// For printfs in opencl kernel
typedef struct {
  char    *io_buf_ptr;
  unsigned io_buf_size;
  unsigned head;
  unsigned tail;
  unsigned used;
} XOCL_IO_BUF_INFO;

// Define a type xocl_vec_kcmd_info that has a pointer to a vector of
// xocl_kernel_cmd_info type
XOCL_VECTOR_TYPE(xocl_kernel_cmd_info, kcmd_info);

// Define a type xocl_vec_krnl_desc that has a pointer to a vector of
// xocl_opencl_kernel_descriptor* type
XOCL_VECTOR_TYPE(xocl_opencl_kernel_descriptor *, krnl_desc);

#endif // __XOCL_KERNEL_CMD_H__
