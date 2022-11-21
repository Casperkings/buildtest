/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED. 
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of 
 * Cadence Design Systems Inc. Any rights to use, modify, and create 
 * derivative works of this file are set forth under the terms of your 
 * license agreement with Cadence Design Systems, Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <xtensa/corebits.h>
#include <xtensa/tie/xt_interrupt.h>
#include <xtensa/xtruntime.h>
#include <xtensa/xmem_bank.h>
#define IDMA_USE_INTR 0
#define IDMA_USE_MULTICHANNEL 1
#include <xtensa/idma.h>
#include "idma_ext.h"
#include "xrp_api.h"
#include "xrp_dsp_hw.h"
#include "xrp_dsp_user.h"
#include "xdyn_lib_callback_if.h"
#ifndef XOCL_DYN_LIB
#include "xrp_opencl_ns.h"
#endif // XOCL_DYN_LIB
#include "xdyn_lib.h"
#include <xtensa/xos.h>

#if XCHAL_NUM_DATARAM > 1
  #if XCHAL_DATARAM1_PADDR < XCHAL_DATARAM0_PADDR
  #  define DRAM_ATTR __attribute__((section(".dram1.data")))
  #else
  #  define DRAM_ATTR __attribute__((section(".dram0.data")))
  #endif
#else
  #  define DRAM_ATTR __attribute__((section(".dram0.data")))
#endif

#if XCHAL_HAVE_PRID
#define GET_PROC_ID XT_RSR_PRID()
#else
#define GET_PROC_ID 0 
#endif // XCHAL_HAVE_PRID

// TODO: provide defintions for xos/xtos
static uint32_t rtos_enable_preemption()  {
  return 0;
}
static uint32_t rtos_disable_preemption() {
  return 0;
}

// If building standalone, define the init, fini, and xrp namespace
// functions
typedef void *(*xocl_init_p)(xdyn_lib_callback_if *);
typedef void *(*xocl_fini_p)(void);
// Pointer to the xrp namespace handler of the dynamic library
typedef enum xrp_status 
(*xrp_run_command_p)(void *context, const void *in_data,
                     size_t in_data_size, void *out_data, size_t out_data_size,
                     struct xrp_buffer_group *buffer_group);
void *xocl_start(unsigned fid);

// Conditionally add 64-byte variants
#if IDMA_USE_64B_DESC
#define IDMA_ADD_DESC64               idma_add_desc64
#define IDMA_ADD_2D_DESC64            idma_add_2d_desc64
#define IDMA_ADD_2D_PRED_DESC64       idma_add_2d_pred_desc64
#define IDMA_ADD_3D_DESC64            idma_add_3d_desc64
#define IDMA_COPY_DESC64              idma_copy_desc64
#define IDMA_COPY_2D_DESC64           idma_copy_2d_desc64
#define IDMA_EXT_ADD_2D_DESC64_CH     idma_ext_add_2d_desc64_ch
#define IDMA_COPY_2D_PRED_DESC64      idma_copy_2d_pred_desc64
#define IDMA_COPY_3D_DESC64           idma_copy_3d_desc64
#else
#define IDMA_ADD_DESC64               NULL
#define IDMA_ADD_2D_DESC64            NULL
#define IDMA_ADD_2D_PRED_DESC64       NULL
#define IDMA_ADD_3D_DESC64            NULL
#define IDMA_COPY_DESC64              NULL
#define IDMA_COPY_2D_DESC64           NULL
#define IDMA_EXT_ADD_2D_DESC64_CH     NULL
#define IDMA_COPY_2D_PRED_DESC64      NULL
#define IDMA_COPY_3D_DESC64           NULL
#endif

#if IDMA_USE_WIDE_API
#define IDMA_ADD_DESC64_WIDE          idma_add_desc64_wide
#define IDMA_ADD_2D_DESC64_WIDE       idma_add_2d_desc64_wide
#define IDMA_ADD_2D_PRED_DESC64_WIDE  idma_add_2d_pred_desc64_wide
#define IDMA_ADD_3D_DESC64_WIDE       idma_add_3d_desc64_wide
#define IDMA_COPY_DESC64_WIDE         idma_copy_desc64_wide
#define IDMA_COPY_2D_DESC64_WIDE      idma_copy_2d_desc64_wide
#define IDMA_COPY_2D_PRED_DESC64_WIDE idma_copy_2d_pred_desc64_wide
#define IDMA_COPY_3D_DESC64_WIDE      idma_copy_3d_desc64_wide
#else
#define IDMA_ADD_DESC64_WIDE          NULL
#define IDMA_ADD_2D_DESC64_WIDE       NULL
#define IDMA_ADD_2D_PRED_DESC64_WIDE  NULL
#define IDMA_ADD_3D_DESC64_WIDE       NULL
#define IDMA_COPY_DESC64_WIDE         NULL
#define IDMA_COPY_2D_DESC64_WIDE      NULL
#define IDMA_COPY_2D_PRED_DESC64_WIDE NULL
#define IDMA_COPY_3D_DESC64_WIDE      NULL
#endif

// Define the set of callback functions for the dynamic library
static xdyn_lib_callback_if xdyn_lib_cb_if DRAM_ATTR = {
  .utils_cb_if     =  {xt_iss_profile_enable, xt_iss_profile_disable,
                       printf, snprintf, vsnprintf, abort, malloc, 
                       realloc, free},
  .dma_cb_if       =  {idma_init_loop, idma_add_desc, IDMA_ADD_DESC64,
                       IDMA_ADD_DESC64_WIDE, idma_add_2d_desc,
                       IDMA_ADD_2D_DESC64, IDMA_ADD_2D_DESC64_WIDE,
                       IDMA_ADD_2D_PRED_DESC64, IDMA_ADD_2D_PRED_DESC64_WIDE,
                       IDMA_ADD_3D_DESC64, IDMA_ADD_3D_DESC64_WIDE,
                       idma_copy_desc, IDMA_COPY_DESC64, IDMA_COPY_DESC64_WIDE,
                       idma_copy_2d_desc, IDMA_COPY_2D_DESC64,
                       IDMA_COPY_2D_DESC64_WIDE,
                       IDMA_COPY_2D_PRED_DESC64, IDMA_COPY_2D_PRED_DESC64_WIDE,
                       idma_ext_add_2d_desc_ch, IDMA_EXT_ADD_2D_DESC64_CH,
                       IDMA_COPY_3D_DESC64, IDMA_COPY_3D_DESC64_WIDE,
                       idma_schedule_desc, idma_sleep,
                       idma_buffer_status, idma_desc_done,
                       idma_buffer_check_errors},
  .xrp_cb_if       =  {xrp_get_buffer_from_group, xrp_release_buffer,
                       xrp_map_buffer, xrp_unmap_buffer},
  .xmem_bank_cb_if =  {xmem_bank_alloc, xmem_bank_free,
                       xmem_bank_get_free_space, xmem_bank_checkpoint_save,
                       xmem_bank_checkpoint_restore, xmem_bank_get_alloc_policy,
                       xmem_bank_get_num_banks},
  .rtos_cb_if      =  {rtos_enable_preemption, rtos_disable_preemption}
};

static void hang(void) __attribute__((noreturn));
static void hang(void)
{   
  for (;;)
    xrp_hw_panic();
} 
  
void abort(void)
{ 
  printf("abort() is called; halting\n");
  hang();
}

static void xos_exception(XosExcFrame *frame)
{
  printf("%s: EXCCAUSE = %ld, EXCVADDR = 0x%08lx, PS = 0x%08lx, "
         "EPC1 = 0x%08lx\n", __func__,
         (unsigned long)frame->exccause, (unsigned long)frame->excvaddr,
         (unsigned long)frame->ps, (unsigned long)frame->pc);
  hang();
}

static void register_exception_handlers(void)
{
#if XCHAL_HAVE_XEA3
  static const int cause[] = {
    EXCCAUSE_INSTRUCTION,
    EXCCAUSE_ADDRESS,
    EXCCAUSE_HARDWARE,
    EXCCAUSE_MEMORY,
  };
#else
  static const int cause[] = {
    EXCCAUSE_ILLEGAL,
    EXCCAUSE_INSTR_ERROR,
    EXCCAUSE_LOAD_STORE_ERROR,
    EXCCAUSE_DIVIDE_BY_ZERO,
    EXCCAUSE_PRIVILEGED,
    EXCCAUSE_UNALIGNED,
    EXCCAUSE_INSTR_DATA_ERROR,
    EXCCAUSE_LOAD_STORE_DATA_ERROR,
    EXCCAUSE_INSTR_ADDR_ERROR,
    EXCCAUSE_LOAD_STORE_ADDR_ERROR,
    EXCCAUSE_ITLB_MISS,
    EXCCAUSE_ITLB_MULTIHIT,
    EXCCAUSE_INSTR_RING,
    EXCCAUSE_INSTR_PROHIBITED,
    EXCCAUSE_DTLB_MISS,
    EXCCAUSE_DTLB_MULTIHIT,
    EXCCAUSE_LOAD_STORE_RING,
    EXCCAUSE_LOAD_PROHIBITED,
    EXCCAUSE_STORE_PROHIBITED,
  };
#endif // XCHAL_HAVE_XEA3
  unsigned i;

  for (i = 0; i < sizeof(cause) / sizeof(cause[0]); ++i) {
    xos_register_exception_handler(cause[i], xos_exception);
  }
}

// Main driver
int main(void) 
{
  enum xrp_status status;
  idma_status_t idma_status;
  xmem_bank_status_t xb_status;
  struct xrp_device *device;

  register_exception_handlers();

  xrp_hw_init();

  // Init xrp device
  device = xrp_open_device(0, &status);
  if (status != XRP_STATUS_SUCCESS) {
    printf("PROC%d: main: xrp_open_device failed!\n", GET_PROC_ID);
    return 1;
  }

  xrp_device_enable_cache(device, 1);

  // Init idma
  idma_status = idma_init(IDMA_CHANNEL_0, 0, MAX_BLOCK_16, 
                          XCHAL_IDMA_MAX_OUTSTANDING_REQ,
                          TICK_CYCLES_8, 100000, NULL);
  if (idma_status != IDMA_OK) {
    printf("PROC%d: main: Failed to initialize idma\n", GET_PROC_ID);
    return 1;
  }

  // Initialize local memory manager. Reserve 16Kb for stack
  xb_status = xmem_init_local_mem(XMEM_BANK_HEAP_ALLOC, 16*1024);
  if (xb_status != XMEM_BANK_OK &&
      xb_status != XMEM_BANK_ERR_STACK_RESERVE_FAIL) {
    printf("PROC%d: main: Failed to initialize local memory\n", GET_PROC_ID);
    return 1;
  }

#ifdef XOCL_DYN_LIB
  // Initialize dynamic library manager
  xdyn_lib_status_t xdyn_lib_status = xdyn_lib_init(device, &xdyn_lib_cb_if);
  if (xdyn_lib_status != XDYN_LIB_OK) {
    printf("PROC%d: main: Failed to initialize dynamic library manager\n",
           GET_PROC_ID);
    return 1;
  }
#else
  // Get the init and fini functions from the dynamic library
  xocl_init_p xocl_init = xocl_start(XDYN_LIB_INIT);
  if (xocl_init == NULL) {
    printf("PROC%d: main: Could not find init functions\n", GET_PROC_ID);
    return 1;
  }

  // Initialize the dynamic library. Get the xrp namespace handler
  // of the library
  xrp_run_command_p xrp_run_command = xocl_init(&xdyn_lib_cb_if);
  if (xrp_run_command == NULL) {
    printf("PROC%d: main: Could not get xrp namespace run command\n", 
           GET_PROC_ID);
  return 1;
  }
  unsigned char XRP_OPENCL_NSID[] = XRP_OPENCL_NSID_INITIALIZER;
  xrp_device_register_namespace(device, &XRP_OPENCL_NSID,
                                xrp_run_command, NULL, &status);
  if (status != XRP_STATUS_SUCCESS) {
    printf("PROC%d: main: Failed to register namespace\n", GET_PROC_ID);
    return 1;
  }
#endif

  static uint32_t main_priority[] = {0};
  status = xrp_user_create_queues(1, main_priority);

  xos_start(0);

  return 0;
}
