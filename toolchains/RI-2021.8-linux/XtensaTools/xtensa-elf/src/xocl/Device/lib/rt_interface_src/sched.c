/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED. 
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of 
 * Cadence Design Systems Inc. Any rights to use, modify, and create 
 * derivative works of this file are set forth under the terms of your 
 * license agreement with Cadence Design Systems, Inc.
 */

#include "misc.h"
#include "lmem.h"
#include "dma.h"
#include "sync.h"
#include "sys.h"
#include "generic_container.h"
#include "xdyn_lib_callback_if.h"
#include "host_interface.h"
#include "xocl_kernel_cmd.h"
#include "xrp_api.h"
#include "xdyn_lib.h"
#include <xt_mld_loader.h>

#define XOCL_CHECK_XRP_STATUS(API, ...) { \
  (API);                                  \
  if (xs != XRP_STATUS_SUCCESS) {         \
    XOCL_DPRINTF(__VA_ARGS__);            \
    return -1;                            \
  }                                       \
} 

#define XOCL_CHECK_XRP_STATUS_RES(API, RES, LABEL, ...) { \
  (API);                                                  \
  if (xs != XRP_STATUS_SUCCESS) {                         \
    err = RES;                                            \
    XOCL_DPRINTF(__VA_ARGS__);                            \
    goto LABEL;                                           \
  }                                                       \
} 

#define XOCL_CHECK(C, RES, LABEL, ...) { \
  if (!(C)) {                            \
    err = RES;                           \
    XOCL_DPRINTF(__VA_ARGS__);           \
    goto LABEL;                          \
  }                                      \
}

// Pointers to table of external callback functions for xrp
static xdyn_lib_xrp_callback_if *xrp_cb_if XOCL_LMEM_ATTR;

// Pointers to table of external callback functions for local memory manager 
static xdyn_lib_xmem_bank_callback_if *xmem_bank_cb_if XOCL_LMEM_ATTR;

// Pointers to table of external callback functions for generic utilities
xdyn_lib_utils_callback_if *utils_cb_if;

// Register all kernels. This function is defined when compiling
// the kernels
extern int xt_ocl_register_all_kernels();

// Calls to the device runtime available as part of the config dependent library
extern int32_t xocl_dma_promotion_scheduler(xocl_kernel_cmd_info *kcmd_info,
                                            int grp_rng_start, int grp_rng_end);
extern void xocl_build_promotable_bufs(xocl_vec_kcmd_info *kcmd_infos, 
                                       unsigned krnl_idx);
extern void xocl_build_buffer_args_states(xocl_kernel_cmd_info *krnl_list, 
                                          unsigned krnl_idx);

// For printfs in kernel
XOCL_IO_BUF_INFO io_buf_info = {NULL, 0, 0, 0, 0};

// Define set of vector operations for xocl_vec_kcmd_info element type.
// Note, memory is allocated/freed for this container from the internal
// local memory manager
XOCL_VECTOR_NAME(xocl_kernel_cmd_info, kcmd_info,
                 xocl_alloc_lmem_internal, xocl_free_lmem_internal);

// Define set of vector operations for xocl_vec_krnl_desc element type.
// Note, memory is allocated/freed for this container from the external
// local memory manager
XOCL_VECTOR_NAME(xocl_opencl_kernel_descriptor *, krnl_desc, 
                 xocl_alloc_lmem_external, xocl_free_lmem_external);

// Create a vector of xocl_vec_kcmd_info in local memory
static xocl_vec_kcmd_info xocl_kcmd_info_vector XOCL_LMEM_ATTR;

// Create a vector of xocl_vec_krnl_desc in local memory
static xocl_vec_krnl_desc xocl_krnl_desc_vector XOCL_LMEM_ATTR;

// Check point local memory manager state (for stack based allocated only)
// to restore any data allocated by the library.
xmem_bank_checkpoint_t lmem0_checkpoint;

#if defined(XOCL_TIME_KERNEL)
// Measure overheads due to waiting for idma and synchronizing across cores
unsigned xocl_wait_dma_cycles XOCL_LMEM_ATTR;
unsigned xocl_sync_wait_time  XOCL_LMEM_ATTR;
#endif

static unsigned get_group_num(xocl_launch_kernel_cmd *krnl_cmd) 
{
  unsigned group_num = (krnl_cmd->global_size[0] / krnl_cmd->local_size[0])
                       * (krnl_cmd->global_size[1] / krnl_cmd->local_size[1])
                       * (krnl_cmd->global_size[2] / krnl_cmd->local_size[2]);
  return group_num;
}

// Return 0 on success, -1 on failure
static int32_t scheduleKernel(xocl_kernel_cmd_info *cmd) 
{
  xocl_launch_kernel_cmd *krnl_cmd = cmd->launchArg;
  unsigned proc_num = krnl_cmd->proc_num;
  unsigned group_num = get_group_num(krnl_cmd);
  int proc_id = krnl_cmd->proc_id;
  int rest_grp = group_num % proc_num;
  int grp_rng_start = proc_id * (group_num / proc_num) +
                      (rest_grp > proc_id ? proc_id : rest_grp);
  int grp_rng_end   = grp_rng_start + (group_num / proc_num) +
                      (rest_grp > proc_id ? 1 : 0);

  XOCL_DPRINTF("scheduleKernel: proc_id %d proc_num %d group_num %d "
               "grp_rng_start %d grp_rng_end %d\n",
               proc_id, proc_num, group_num, grp_rng_start, grp_rng_end);

  XOCL_TPRINTF("%u: Start executing kernel '%s' groups's %d to %d of "
               "%d groups\n", _xocl_time_stamp(), cmd->krnl_desc->kernel_name,
               grp_rng_start, grp_rng_end, group_num);

  int32_t res = xocl_dma_promotion_scheduler(cmd, grp_rng_start, grp_rng_end);

  XOCL_TPRINTF("%u: Done executing kernel '%s'\n", _xocl_time_stamp(),
               cmd->krnl_desc->kernel_name);
  return res;
}

// Return -1 (unsigned) if error, else returns the total size of allocated
// local buffers
static unsigned alloc_local_buffers(xocl_kernel_cmd_info *krnl_list, 
                                    unsigned krnl) 
{
  unsigned total_size = 0;
  XOCL_LMEM_ID lmem = XOCL_LMEM0;
  xocl_kernel_cmd_info *kcmd_info = &krnl_list[krnl];
  xocl_launch_kernel_cmd *krnl_cmd = kcmd_info->launchArg;
  const xocl_opencl_kernel_descriptor *krnl_desc = krnl_list[krnl].krnl_desc;
  const unsigned num_args = krnl_cmd->num_args;

  XOCL_DPRINTF("alloc_local_buffers: Allocate local buffer for kernel: %s_%s\n",
               krnl_desc->source_hash, krnl_desc->kernel_name);

  for (unsigned i = 0; i < num_args; ++i) {
    if (krnl_desc->arg_info[i].address_qualifier !=
        XOCL_CL_KERNEL_ARG_ADDRESS_LOCAL)
      continue;

    lmem = (lmem == XOCL_LMEM0) ? XOCL_LMEM1 : XOCL_LMEM0;
    unsigned localSize = krnl_desc->arg_info[i].arg_size;

    // localSize comes from run-time when setting arg.
    // see: Kernel::SetArgument
    if (!localSize) {
      localSize = krnl_cmd->arguments[i].buf_size;
      krnl_desc->arg_info[i].arg_size = localSize;
    }

    kcmd_info->krnl_args[i] = (void *)xocl_aligned_alloc_lmem(lmem, &localSize, 
                                                              XOCL_FALSE);
    if (kcmd_info->krnl_args[i] == NULL)
      return (unsigned)-1;

    total_size += localSize;
    XOCL_DPRINTF("alloc_local_buffers: Set local %p with size = %d "
                 "for arg %d\n", kcmd_info->krnl_args[i], localSize, i);
  }
  return total_size;
}

static int is_local_buffer_arg(const xocl_opencl_kernel_descriptor *desc,
                                unsigned arg) 
{
  unsigned addrspace = desc->arg_info[arg].address_qualifier;
  return addrspace == XOCL_CL_KERNEL_ARG_ADDRESS_LOCAL;
}

// Return 0 if success, else returns -1
static int map_buffers(struct xrp_buffer_group *buffer_group,
                        xocl_kernel_cmd_info *kcmd_info) 
{
  enum xrp_status xs = XRP_STATUS_SUCCESS;

  const xocl_opencl_kernel_descriptor *desc = kcmd_info->krnl_desc;
  for (unsigned i = 0; i < kcmd_info->launchArg->num_args; ++i) {
    if (is_local_buffer_arg(kcmd_info->krnl_desc, i))
      continue;

    const xocl_krnl_arg *karg = &kcmd_info->launchArg->arguments[i];
    if (karg->buf_id == XOCL_NULL_PTR_BUF) {
      kcmd_info->krnl_args[i] = NULL;
      continue;
    }

    struct xrp_buffer *xbuf;
    XOCL_CHECK_XRP_STATUS(
      xbuf = xrp_cb_if->xrp_get_buffer_from_group(buffer_group,
                                                  karg->buf_id, &xs),
      "map_buffers: failed to get buffer from group %p, xrp_status %d\n", 
      buffer_group, xs);

    void *buf;
    XOCL_CHECK_XRP_STATUS(
      buf = xrp_cb_if->xrp_map_buffer(xbuf, karg->offset, karg->buf_size,
                                      XRP_READ_WRITE, &xs),
      "map_buffers: failed to map parameter %d from xrp buf %p, "
      "xrp_status %d\n", i, xbuf, xs);
    kcmd_info->krnl_args[i] = buf;
    kcmd_info->krnl_args_xbuf[i] = xbuf;
    XOCL_DPRINTF("map_buffers: mapping parameter %d from xrp buf %p to %p\n", i,
                 xbuf, buf);

    // Find private argument and put to local memory
    // Constant non-pointer arguments are always placed in local memory
    // Constants pointer arguments are placed only if size of pointed to
    // buffer is <= 8K
    unsigned address_qualifier = desc->arg_info[i].address_qualifier;
    if (address_qualifier == XOCL_CL_KERNEL_ARG_ADDRESS_PRIVATE ||
        (address_qualifier == XOCL_CL_KERNEL_ARG_ADDRESS_CONSTANT &&
         (strchr(desc->arg_info[i].type_name, '*') == NULL ||
          karg->buf_size <= 8*1024))) 
    {
      unsigned size = karg->buf_size;
      void *p_arg_ptr = xocl_aligned_alloc_lmem(XOCL_LMEM0, &size, XOCL_FALSE);
      if (p_arg_ptr == NULL) {
        XOCL_DPRINTF("map_buffers: Failed to allocate private/constant arg %d,"
                     " size %d in local mem\n", i, size);
        return -1;
      }
      XOCL_DPRINTF("map_buffers: Allocating private/constant arg %d, size %d "
                   "in local mem at %p\n", i, size, p_arg_ptr);
      int r = xocl_dma_memcpy(p_arg_ptr, (void *)kcmd_info->krnl_args[i], size);
      if (r) {
        XOCL_DPRINTF("map_buffers: Failed to dma memcpy private/constant arg"
                     " %p to %p, size %d\n", (void *)kcmd_info->krnl_args[i],
                     p_arg_ptr, size);
        return -1;
      }
      kcmd_info->krnl_args[i] = p_arg_ptr;
      // Prevent DMA promotion
      desc->arg_info[i].no_dma_promot = 1;
    }
  }
  return 0;
}

static void unmap_release_buffers(struct xrp_buffer_group *buffer_group,
                                  xocl_kernel_cmd_info *kcmd_info) 
{
  (void)buffer_group;
  for (unsigned i = 0; i < kcmd_info->launchArg->num_args; ++i) {
    if (is_local_buffer_arg(kcmd_info->krnl_desc, i))
      continue;

    const xocl_krnl_arg *karg = &kcmd_info->launchArg->arguments[i];
    if (karg->buf_id == XOCL_NULL_PTR_BUF)
      continue;

    struct xrp_buffer *xbuf = kcmd_info->krnl_args_xbuf[i];
    xrp_cb_if->xrp_unmap_buffer(xbuf, (void *)kcmd_info->krnl_args[i], NULL);
    xrp_cb_if->xrp_release_buffer(xbuf);
  }
}

// XRP namespace handler
enum xrp_status xt_ocl_run_command(void *context,
                                   const void *in_data, size_t in_data_size,
                                   void *out_data, size_t out_data_size,
                                   struct xrp_buffer_group *buffer_group) 
{
  (void)out_data_size;
  (void)context;
  int r;
  int err = 0;
  enum xrp_status xs;
  unsigned cmd = *(const unsigned *)in_data;
  struct xrp_buffer *xrp_io_buf = NULL;

  XOCL_ASSERT(in_data, "xt_ocl_run_command: in_data is null\n");
  XOCL_ASSERT(out_data, "xt_ocl_run_command: in_data is null\n");

  switch (cmd) {
  // Set up kernels enqueued by the host
  case XOCL_CMD_SET_KERNEL: {
    // Disable context switching
    xocl_disable_preemption();

    utils_cb_if->xt_iss_profile_enable();

    XOCL_DPRINTF("xt_ocl_run_command: Executing XOCL_CMD_SET_KERNEL\n");

    // Initialize dma descriptor
    r = xocl_dma_desc_init();
    XOCL_CHECK(r == 0, XOCL_OUT_OF_RESOURCES, LABEL_FAIL1, 
               "xt_ocl_run_command: Error initializing dma\n");

    // Initialize internal local memory manager
    // All subsequent allocation should happen from the internal manager
    r = xocl_init_local_mem();
    XOCL_CHECK(r == 0, XOCL_OUT_OF_RESOURCES, LABEL_FAIL1,
               "xt_ocl_run_command: Error initializing local memory\n");

    // Check point local mem allocator state
    unsigned lmem0_allocated = xocl_get_allocated(XOCL_LMEM0);
    unsigned lmem1_allocated = xocl_get_allocated(XOCL_LMEM1);

    // Get the kernel cmd from the host
    const xocl_launch_kernel_cmd *launch_kernel_cmd = 
                                  (const xocl_launch_kernel_cmd *)in_data;

    XOCL_DPRINTF("xt_ocl_run_command: kernel_id: %llu\n",
                 launch_kernel_cmd->kernel_id);

    XOCL_DPRINTF("xt_ocl_run_command:: XOCL_CMD_SET_KERNEL: "
                  "instance id: %llx\n", launch_kernel_cmd->id);

    // Make a copy of the host command in xrp queue to local memory
    XOCL_DPRINTF("xt_ocl_run_command: XOCL_CMD_SET_KERNEL: "
                 "Copy launch_kernel_cmd (size: %d) for kernel %llu from "
                 "xrp queue to local mem\n", 
                 in_data_size, launch_kernel_cmd->kernel_id);

    xocl_launch_kernel_cmd *launch_kernel_cmd_copy =
        (xocl_launch_kernel_cmd *)xocl_alloc_lmem(XOCL_LMEM0, in_data_size);
    XOCL_CHECK(launch_kernel_cmd_copy, XOCL_OUT_OF_RESOURCES, LABEL_FAIL,
               "xt_ocl_run_command: Failed to allocate local memory for "
                "launch_kernel_cmd_copy\n");

    r = xocl_dma_memcpy(launch_kernel_cmd_copy, launch_kernel_cmd, 
                        in_data_size);
    XOCL_CHECK(r == 0, XOCL_OUT_OF_RESOURCES, LABEL_FAIL,
               "xt_ocl_run_command: Failed to dma memcpy "
               "launch_kernel_cmd_copy\n");

    // Allocate krnl_args
    XOCL_DPRINTF("xt_ocl_run_command: XOCL_CMD_SET_KERNEL: "
                 "Allocating krnl_args pointers (#num_args %d) in local mem\n",
                 launch_kernel_cmd_copy->num_args);
    unsigned arg_sizes = launch_kernel_cmd_copy->num_args * sizeof(void *);
    void **krnl_args = (void **)xocl_alloc_lmem(XOCL_LMEM0, arg_sizes);
    XOCL_CHECK(arg_sizes == 0 || krnl_args, XOCL_OUT_OF_RESOURCES, LABEL_FAIL, 
               "xt_ocl_run_command: Failed to allocate local memory for "
               "krnl_args\n");
    memset(krnl_args, 0, arg_sizes);

    // Allocate krnl_args xrp_buffer
    XOCL_DPRINTF("xt_ocl_run_command: XOCL_CMD_SET_KERNEL: Allocating "
                 "krnl_args_xbuf pointers (#num_args %d) in local mem\n",
                 launch_kernel_cmd_copy->num_args);
    arg_sizes = launch_kernel_cmd_copy->num_args * sizeof(struct xrp_buffer *);
    void **krnl_args_xbuf = (void **)xocl_alloc_lmem(XOCL_LMEM0, arg_sizes);
    XOCL_CHECK(arg_sizes == 0 || krnl_args_xbuf, 
               XOCL_OUT_OF_RESOURCES, LABEL_FAIL, 
               "xt_ocl_run_command: Failed to allocate local memory for "
               "krnl_args_xbuf\n");
    memset(krnl_args_xbuf, 0, arg_sizes);

    // Find the kernel in the vector of registered kernels
    unsigned kidx;
    for (kidx = 0; kidx < xocl_krnl_desc_vector.num_elem; ++kidx) {
      xocl_opencl_kernel_descriptor *kdesc = 
                         xocl_vec_at_krnl_desc(&xocl_krnl_desc_vector, kidx);
      if (kdesc->kernel_id == launch_kernel_cmd_copy->kernel_id)
        break;
    }
    XOCL_CHECK(kidx != xocl_krnl_desc_vector.num_elem, 
               XOCL_OUT_OF_RESOURCES, LABEL_FAIL, 
               "xt_ocl_run_command: Cannot find kernel id: %llu\n", 
               launch_kernel_cmd_copy->kernel_id);

    // Make a copy of the xocl_opencl_kernel_descriptor in local mem
    int kdesc_size = sizeof(xocl_opencl_kernel_descriptor);
    XOCL_DPRINTF("Allocating xocl_opencl_kernel_descriptor in local mem\n");
    xocl_opencl_kernel_descriptor *desc =
      (xocl_opencl_kernel_descriptor *)xocl_alloc_lmem(XOCL_LMEM0, kdesc_size);
    XOCL_CHECK(desc, XOCL_OUT_OF_RESOURCES, LABEL_FAIL,
               "xt_ocl_run_command: Failed to allocate local memory for "
                "xocl_opencl_kernel_descriptor\n");

    r = xocl_dma_memcpy((void *)desc, 
                        xocl_vec_at_krnl_desc(&xocl_krnl_desc_vector, kidx),
                        kdesc_size);
    XOCL_CHECK(r == 0, XOCL_OUT_OF_RESOURCES, LABEL_FAIL,
               "xt_ocl_run_command: Failed to dma memcpy "
               "xocl_opencl_kernel_descriptor\n");

    // Copy over the xocl_opencl_kernel_arg_info
    int arg_info_size = sizeof(xocl_opencl_kernel_arg_info) *
                        desc->get_arg_nr[0]();
    XOCL_DPRINTF("xt_ocl_run_command: Allocating kernel_arg_info in "
                 "local mem\n");
    xocl_opencl_kernel_arg_info *arg_info = 
      (xocl_opencl_kernel_arg_info *)xocl_alloc_lmem(XOCL_LMEM0, arg_info_size);
    XOCL_CHECK(arg_info_size == 0 || arg_info, 
               XOCL_OUT_OF_RESOURCES, LABEL_FAIL,
               "xt_ocl_run_command: Allocating kernel_arg_info in local memory "
               "failed\n");

    r = xocl_dma_memcpy((void *)arg_info, desc->arg_info, arg_info_size);
    XOCL_CHECK(r == 0, XOCL_OUT_OF_RESOURCES, LABEL_FAIL,
               "xt_ocl_run_command: Failed to dma memcpy "
               "xocl_opencl_kernel_arg_info\n");
    // Point to local memory copy
    desc->arg_info = arg_info;

    XOCL_DPRINTF("xt_ocl_run_command: g = {%d, %d, %d} \n", 
                 launch_kernel_cmd_copy->global_size[0],
                 launch_kernel_cmd_copy->global_size[1],
                 launch_kernel_cmd_copy->global_size[2]);

    XOCL_DPRINTF("xt_ocl_run_command: l = {%d, %d, %d} \n", 
                 launch_kernel_cmd_copy->local_size[0],
                 launch_kernel_cmd_copy->local_size[1],
                 launch_kernel_cmd_copy->local_size[2]);

    XOCL_DPRINTF("xt_ocl_run_command: Creating new xocl_kernel_cmd_info\n");
    xocl_kernel_cmd_info new_kcmd;
    memset(&new_kcmd, 0, sizeof(new_kcmd));
    unsigned group_num = get_group_num(launch_kernel_cmd_copy);
    unsigned grp_stat_size = group_num * sizeof(char);
    new_kcmd.launchArg = launch_kernel_cmd_copy;
    new_kcmd.processed = 0;
    new_kcmd.krnl_desc = desc;
    XOCL_DPRINTF("xt_ocl_run_command: Allocating grp_status in local mem\n");
    new_kcmd.grp_status = (char *)xocl_alloc_lmem(XOCL_LMEM0, grp_stat_size);
    XOCL_CHECK(new_kcmd.grp_status, XOCL_OUT_OF_RESOURCES, LABEL_FAIL,
               "xt_ocl_run_command: Cannot allocate local memory for "
               "grp_status\n");
    memset(new_kcmd.grp_status, 0, grp_stat_size);
    new_kcmd.krnl_args = (void const **)krnl_args;
    new_kcmd.krnl_args_xbuf = (struct xrp_buffer **)krnl_args_xbuf;

    int s = xocl_vec_push_back_kcmd_info(&xocl_kcmd_info_vector, new_kcmd);
    XOCL_CHECK(s == 0, XOCL_OUT_OF_RESOURCES, LABEL_FAIL,
               "xt_ocl_run_command: Cannot add xocl_kernel_cmd_info to "
               "xocl_kcmd_info_vector\n");

#ifdef XOCL_DEBUG
    // Dump the allocation offsets
    xocl_get_allocated(XOCL_LMEM0);
    xocl_get_allocated(XOCL_LMEM1);
#endif

LABEL_FAIL:
    // Restore allocated memory; return err;
    if (err == XOCL_OUT_OF_RESOURCES) {
      xocl_set_allocated(XOCL_LMEM0, lmem0_allocated);
      xocl_set_allocated(XOCL_LMEM0, lmem1_allocated);
    }

LABEL_FAIL1:
    *(int *)out_data = err;
    utils_cb_if->xt_iss_profile_disable();
    XOCL_DPRINTF("xt_ocl_run_command: XOCL_CMD_SET_KERNEL done\n", __func__);

    // Restore context switching
    xocl_enable_preemption();

    return XRP_STATUS_SUCCESS;
  }

  // Execute all kernels enqueued so far
  case XOCL_CMD_EXE_KERNEL: {
    // Disable context switching
    xocl_disable_preemption();

    utils_cb_if->xt_iss_profile_enable();

#if defined(XOCL_TIME_KERNEL)
    xocl_wait_dma_cycles = 0;
    unsigned stm = _xocl_time_stamp();
#endif

    XOCL_DPRINTF("xt_ocl_run_command: XOCL_CMD_EXE_KERNEL, "
                 "number of kernels = %d\n", xocl_kcmd_info_vector.num_elem);

    if (xocl_kcmd_info_vector.num_elem == 0)
      return XRP_STATUS_SUCCESS;

    const xocl_launch_kernel_cmd *launch_kernel_cmd = 
                                  (const xocl_launch_kernel_cmd *)in_data;

    // The master proc id is the PRID of the master proc. of the group
    // of processors executing the kernel
    unsigned master_proc_id = launch_kernel_cmd->master_proc_id;

    // Number of processors in the processor group
    unsigned proc_num = launch_kernel_cmd->proc_num;

    // Relative proc id of this processor in the group 
    // (in the range 0..proc_num-1)
    unsigned proc_id = launch_kernel_cmd->proc_id;

    // Set sync buffer.
    struct xrp_buffer *xrp_sync_buf = 
           xrp_cb_if->xrp_get_buffer_from_group(buffer_group,
                                                launch_kernel_cmd->syn_buf_id, 
                                                &xs);
    // Expect this to pass
    XOCL_ASSERT(xs == XRP_STATUS_SUCCESS,
                "xt_ocl_run_command: xrp get buffer of sync buf failed\n");

    xocl_sync_t *sync_buf =
                 xrp_cb_if->xrp_map_buffer(xrp_sync_buf, 0, 
                                           launch_kernel_cmd->syn_buf_size,
                                           XRP_WRITE, &xs);
    // Expect this to pass
    XOCL_ASSERT(xs == XRP_STATUS_SUCCESS, 
                "xt_ocl_run_command: xrp_map_buffer of sync failed");
    
    XOCL_DPRINTF("xt_ocl_run_command: sync buf = %p\n", sync_buf);

    // Initialize dma descriptor
    r = xocl_dma_desc_init();
    XOCL_CHECK(r == 0, XOCL_OUT_OF_RESOURCES, LABEL_FAIL2,
               "xt_ocl_run_command: Error initializing dma\n");

    XOCL_DPRINTF("xt_ocl_run_command: XOCL_CMD_EXE_KERNEL instance id: %llx\n",
                 launch_kernel_cmd->id);

    unsigned long long id = launch_kernel_cmd->id;

    // Map all the buffers for all kernels that have been enqueued
    // (XOCL_CMD_SET_KERNEL) for execution
    for (unsigned i = 0; i < xocl_kcmd_info_vector.num_elem; ++i) {
      xocl_kernel_cmd_info *kcmd_info = &xocl_kcmd_info_vector.data[i];
      if (id != kcmd_info->launchArg->id || kcmd_info->processed)
        continue;
      XOCL_DPRINTF("xt_ocl_run_command: mapping buffers for kernel %d\n", i);
      r = map_buffers(buffer_group, kcmd_info);
      XOCL_CHECK(r == 0, XOCL_OUT_OF_RESOURCES, LABEL_FAIL2,
                 "xt_ocl_run_command: Failed to map buffers for "
                 "kernel %d\n", i);
    }

    // Set printf buffer.
    if (launch_kernel_cmd->printf_buf_size) {
      io_buf_info.head = 0;
      io_buf_info.tail = 0;
      io_buf_info.used = 0;
      io_buf_info.io_buf_size = launch_kernel_cmd->printf_buf_size;
      XOCL_CHECK_XRP_STATUS_RES(
        xrp_io_buf = 
          xrp_cb_if->xrp_get_buffer_from_group(buffer_group,
                                               launch_kernel_cmd->printf_buf_id,
                                               &xs),
        XOCL_OUT_OF_RESOURCES, LABEL_FAIL2,
        "xt_ocl_run_command: Failed to get print buffer %d from "
        "buffer group %p, xrp_status %d\n", 
        launch_kernel_cmd->printf_buf_id, buffer_group, xs);

      XOCL_CHECK_XRP_STATUS_RES(
        io_buf_info.io_buf_ptr = 
            xrp_cb_if->xrp_map_buffer(xrp_io_buf, 0, 
                                      io_buf_info.io_buf_size,
                                      XRP_WRITE, &xs),
        XOCL_OUT_OF_RESOURCES, LABEL_FAIL2,
        "xt_ocl_run_command: Failed to get print buffer %d from "
        "xrp buffer %p, xrp_status %d\n", xrp_io_buf, xs);

      XOCL_DPRINTF("xt_ocl_run_command: print buf = %p\n", 
                   io_buf_info.io_buf_ptr);

      memset(io_buf_info.io_buf_ptr, 0, io_buf_info.io_buf_size);
    }

    // Allocate buffers and schedule all enqueued kernels
    for (unsigned krnl_idx = 0;
         krnl_idx < xocl_kcmd_info_vector.num_elem; ++krnl_idx) {
      xocl_kernel_cmd_info *kcmd_info = &xocl_kcmd_info_vector.data[krnl_idx];

      if (id != kcmd_info->launchArg->id || kcmd_info->processed)
        continue;

      XOCL_DPRINTF("xt_ocl_run_command: Processing kernel: %s\n", 
                   kcmd_info->krnl_desc->kernel_name);

      // Record current lmem allocated spaces before we start to allocate any
      // local mem for the function.
      unsigned lmem0_allocated = xocl_get_allocated(XOCL_LMEM0);
      unsigned lmem1_allocated = xocl_get_allocated(XOCL_LMEM1);

      XOCL_DPRINTF("xt_ocl_run_command: Checkpointing lmem0_allocated: %u, "
                   "lmem1_allocated: %u\n", lmem0_allocated, lmem1_allocated);

      unsigned res = alloc_local_buffers(xocl_kcmd_info_vector.data, krnl_idx);
      XOCL_CHECK(res != (unsigned)-1, XOCL_OUT_OF_RESOURCES, LABEL_FAIL2, 
                 "xt_ocl_run_command: Error alloc_local_buffers\n");

      xocl_build_buffer_args_states(xocl_kcmd_info_vector.data, krnl_idx);
      xocl_build_promotable_bufs(&xocl_kcmd_info_vector, krnl_idx);

      // Syncronize across all cores
      XOCL_DPRINTF("xt_ocl_run_command: xocl_wait_all_cores before launching "
                   "kernel: %s\n", kcmd_info->krnl_desc->kernel_name);
#if defined(XOCL_TIME_KERNEL)
      unsigned wait_stm = _xocl_time_stamp();
#endif
      xocl_wait_all_cores(sync_buf, master_proc_id, proc_num, proc_id);
#if defined(XOCL_TIME_KERNEL)
      unsigned wait_etm = _xocl_time_stamp();
      xocl_sync_wait_time += wait_etm - wait_stm;
#endif

      // Execute kernel
      res = scheduleKernel(kcmd_info);

      XOCL_CHECK(res == 0, XOCL_OUT_OF_RESOURCES, LABEL_FAIL2,
                 "xt_ocl_run_command: Error scheduleKernel\n");

      kcmd_info->processed = 1;

      // revert allocated memory
      xocl_set_allocated(XOCL_LMEM0, lmem0_allocated);
      xocl_set_allocated(XOCL_LMEM1, lmem1_allocated);

      XOCL_DPRINTF("xt_ocl_run_command: Restoring lmem0_allocated: %u, "
                   "lmem1_allocated: %u\n", lmem0_allocated, lmem1_allocated);
    }

    {
    // Synchronize across all cores
#if defined(XOCL_TIME_KERNEL)
    unsigned wait_stm = _xocl_time_stamp();
#endif
    xocl_wait_all_cores(sync_buf, master_proc_id, proc_num, proc_id);
#if defined(XOCL_TIME_KERNEL)
    unsigned wait_etm = _xocl_time_stamp();
    xocl_sync_wait_time += wait_etm - wait_stm;
#endif
    }

    // Unmap buffers processed in this round
    for (unsigned i = 0; i < xocl_kcmd_info_vector.num_elem; ++i) {
      xocl_kernel_cmd_info *kcmd_info = &xocl_kcmd_info_vector.data[i];
      if (id != kcmd_info->launchArg->id)
        continue;
      unmap_release_buffers(buffer_group, kcmd_info);
    }

    // Check if all commands are done
    int commands_done = 1;
    for (unsigned i = 0; i < xocl_kcmd_info_vector.num_elem; ++i) {
      xocl_kernel_cmd_info *kcmd_info = &xocl_kcmd_info_vector.data[i];
      if (!kcmd_info->processed) {
        commands_done = 0;
        break;
      }
    }
       
#if defined(XOCL_TIME_KERNEL)
    unsigned etm = _xocl_time_stamp();
    XOCL_PPRINTF("XOCL_CMD_EXE_KERNEL Cycles %u\n", 
                  etm-stm-xocl_sync_wait_time);
    XOCL_PPRINTF("XOCL_WAIT_DMA Cycles %u\n", xocl_wait_dma_cycles);
#endif

LABEL_FAIL2:
    // Set error condition in sync location
    if (err == XOCL_OUT_OF_RESOURCES)
      xocl_set_sync_error(sync_buf, master_proc_id, proc_num, proc_id);

    // Release sync buffer
    xrp_cb_if->xrp_unmap_buffer(xrp_sync_buf, (void *)sync_buf, NULL);
    xrp_cb_if->xrp_release_buffer(xrp_sync_buf);

    // Release print buffer
    if (xrp_io_buf) {
      xrp_cb_if->xrp_unmap_buffer(xrp_io_buf, io_buf_info.io_buf_ptr, NULL);
      xrp_cb_if->xrp_release_buffer(xrp_io_buf);
    }

    if (err == XOCL_OUT_OF_RESOURCES) {
      for (unsigned i = 0; i < xocl_kcmd_info_vector.num_elem; ++i) {
        xocl_kernel_cmd_info *kcmd_info = &xocl_kcmd_info_vector.data[i];
        unmap_release_buffers(buffer_group, kcmd_info);
      }
    }
  
    if (err == XOCL_OUT_OF_RESOURCES || commands_done) {
      xocl_vec_destroy_kcmd_info(&xocl_kcmd_info_vector);
      xocl_deinit_local_mem();
    }

    *(int *)out_data = err;
    utils_cb_if->xt_iss_profile_disable();
    XOCL_DPRINTF("xt_ocl_run_command: XOCL_CMD_EXE_KERNEL done\n");

    // Restore context switching
    xocl_enable_preemption();

    return XRP_STATUS_SUCCESS;
  }

  case XOCL_CMD_QUERY_LOCAL_MEM_SIZE: {
    // Disable context switching
    xocl_disable_preemption();

    // We allocate local buffer on bank-0 only, so we should only return
    // free size of bank-0. 
    XOCL_DPRINTF("xt_ocl_run_command: "
                 "Executing XOCL_CMD_QUERY_LOCAL_MEM_SIZE\n");
    size_t r = xocl_get_lmem_free_size_external(0);
    XOCL_DPRINTF("xt_ocl_run_command: %d bytes free in bank-0. "
                 "XOCL_CMD_QUERY_LOCAL_MEM_SIZE done\n", r);
    *(unsigned *)out_data = r;

    // Restore context switching
    xocl_enable_preemption();

    return XRP_STATUS_SUCCESS;
  }

  default:
    XOCL_DPRINTF("xt_ocl_run_command: Unknown command: %d\n", cmd);
    return XRP_STATUS_FAILURE;
  }
}

// Register kernels. Call to this function is generated by the compiler
// when compling the kernels.
// Returns 0 if successful, else -1.
int _xt_ocl_reg_krldes_func(int kernelNum,
                            xocl_opencl_kernel_descriptor *desc) 
{
  for (int i = 0; i < kernelNum; ++i) {
    XOCL_DPRINTF("_xt_ocl_reg_krldes_func: id = %llu, %s -> %s\n",
                 desc[i].kernel_id, desc[i].source_hash, desc[i].kernel_name);
    int err;
    // Note, this allocates memory using the external memory manager
    err = xocl_vec_push_back_krnl_desc(&xocl_krnl_desc_vector, &desc[i]);
    if (err)
      return -1;
  }
  XOCL_DPRINTF("_xt_ocl_reg_krldes_func done\n");
  return 0;
}

// Library init function. Called once at library load time.
//
// Returns handle to XRP command processing callback on success, else
// returns NULL.
void *xt_ocl_init(xdyn_lib_callback_if *cb_if)
{
  int err;

  // Initialize the call backs to different modules - idma, xrp, mem manager, 
  // utils
  xrp_cb_if = &(cb_if->xrp_cb_if);
  utils_cb_if = &(cb_if->utils_cb_if);
  xmem_bank_cb_if = &(cb_if->xmem_bank_cb_if);
  xocl_set_xmem_bank_callback_if(&(cb_if->xmem_bank_cb_if));
  xocl_set_dma_callback_if(&(cb_if->dma_cb_if));
  xocl_set_rtos_callback_if(&(cb_if->rtos_cb_if));

  XOCL_DPRINTF("xt_ocl_init:...\n");

  // Check point current bank state for stack based allocator
  if (xmem_bank_cb_if->xmem_bank_get_alloc_policy() == XMEM_BANK_STACK_ALLOC)
    xmem_bank_cb_if->xmem_bank_checkpoint_save(0, &lmem0_checkpoint);

  // Register all the kernels
  err = xt_ocl_register_all_kernels();
  if (err)
    goto fail;

  // Allocate the dma descriptor buffer
  err = xocl_dma_desc_buffer_init();
  if (err)
    goto fail;

  // Return handle to XRP main command processing callback
  return &xt_ocl_run_command;

fail:
  // Clean up
  xocl_vec_destroy_krnl_desc(&xocl_krnl_desc_vector);
  if (xmem_bank_cb_if->xmem_bank_get_alloc_policy() == XMEM_BANK_STACK_ALLOC)
    xmem_bank_cb_if->xmem_bank_checkpoint_restore(0, &lmem0_checkpoint);
  return NULL;
}

// Library de-init function. Called at library unload time.
void xt_ocl_fini()
{
  XOCL_DPRINTF("xt_ocl_fini...\n");

  // Clean up
  xocl_vec_destroy_krnl_desc(&xocl_krnl_desc_vector);
  xocl_dma_desc_buffer_deinit();

  if (xmem_bank_cb_if->xmem_bank_get_alloc_policy() == XMEM_BANK_STACK_ALLOC)
    xmem_bank_cb_if->xmem_bank_checkpoint_restore(0, &lmem0_checkpoint);
}
  
// Entry point to the dynamic library
// 
// Returns the 'init' or 'finalize' function
#ifdef XOCL_DYN_LIB
void *_start(xdyn_lib_funcs_t fid)
#else
void *xocl_start(xdyn_lib_funcs_t fid)
#endif // XOCL_DYN_LIB
{
  if (fid == XDYN_LIB_INIT)
    return xt_ocl_init;
  else if (fid == XDYN_LIB_FINI)
    return xt_ocl_fini;
  return 0;
}
