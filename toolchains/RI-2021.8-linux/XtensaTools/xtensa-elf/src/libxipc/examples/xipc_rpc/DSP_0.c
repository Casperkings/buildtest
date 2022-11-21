/* Copyright (c) 2003-2015 Cadence Design Systems, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <stdlib.h>
#include <xtensa/xtutil.h>
#include <xtensa/hal.h>
#include <xtensa/system/xmp_subsystem.h>
#include <xtensa/xipc/xipc.h>
#include <xtensa/xipc/xipc_sys.h>
#include "shared.h"
#include "xmp_log.h"

extern void xmp_set_mpu_attrs();
extern void xmp_set_mpu_attrs_region_uncached(void *, size_t);

int test1()
{
  xipc_status_t status;
  xipc_rpc_event_t event;

  // Enqueue a command to DSP1
  int cmd = 1000;
  int result  __attribute__((aligned(XCHAL_DCACHE_LINESIZE)));

  // Get system address space address in case if object in local memory
  void *result_addr = xipc_get_sys_addr(&result, xipc_get_my_pid());

  // Enqueue command. Use default namespace handler on target.
  status = xipc_rpc_run_cmd(rpc_queue0, 0, &cmd, sizeof(cmd), 
                            result_addr, sizeof(result), &event);
  if (status != XIPC_OK) {
    xmp_log("Error enqueuing command to rpc_queue0\n");
    return -1;
  }
  
  // Wait for command to finish
  status = xipc_rpc_wait_event(&event);
  if (status != XIPC_OK) {
    xmp_log("Error waiting for event\n");
    return -1;
  }

  // Invalidate stale copy of result
#if XCHAL_DCACHE_SIZE>0 && !XCHAL_DCACHE_IS_COHERENT
#if XCHAL_HAVE_L2_CACHE
  xthal_dcache_L1_region_invalidate(&result, sizeof(result));
#else
  xthal_dcache_region_invalidate(&result, sizeof(result));
#endif
#endif

  if (result != (cmd+1)) {
    xmp_log("Expected %x, but got %x\n", cmd+1, result);
    return -1;
  } 

  return 0;
}

int test2()
{
  xipc_status_t status;
  xipc_rpc_event_t event;

  // Enqueue a command to DSP1
  int cmd = 2000;
  int result  __attribute__((aligned(XCHAL_DCACHE_LINESIZE)));

  // Allocate from command buffer
  int *buf = xipc_rpc_alloc_cmd_buf(rpc_queue0, &status);
  if (status != XIPC_OK) {
    xmp_log("Error enqueuing command to rpc_queue0\n");
    return -1;
  }

  // Direct write to command buffer
  *buf = cmd;

  // Get system address space address in case if object in local memory
  void *result_addr = xipc_get_sys_addr(&result, xipc_get_my_pid());

  // Enqueue command. Use default namespace handler on target.
  status = xipc_rpc_enqueue_cmd_buf(rpc_queue0, 0, buf,
                                    result_addr, sizeof(result), &event);
  if (status != XIPC_OK) {
    xmp_log("Error enqueuing command to rpc_queue0\n");
    return -1;
  }
  
  // Wait for command to finish
  status = xipc_rpc_wait_event(&event);
  if (status != XIPC_OK) {
    xmp_log("Error waiting for event\n");
    return -1;
  }

  // Invalidate stale copy of result
#if XCHAL_DCACHE_SIZE>0 && !XCHAL_DCACHE_IS_COHERENT
#if XCHAL_HAVE_L2_CACHE
  xthal_dcache_L1_region_invalidate(&result, sizeof(result));
#else
  xthal_dcache_region_invalidate(&result, sizeof(result));
#endif
#endif

  if (result != (cmd+1)) {
    xmp_log("Expected %x, but got %x\n", cmd+1, result);
    return -1;
  } 

  return 0;
}

int test3()
{
  xipc_status_t status;

  // Enqueue a synchronous command to DSP1
  int cmd = 3000;
  int result  __attribute__((aligned(XCHAL_DCACHE_LINESIZE)));

  // Get system address space address in case if object in local memory
  void *result_addr = xipc_get_sys_addr(&result, xipc_get_my_pid());

  // Enqueue command. Use default namespace handler on target.
  status = xipc_rpc_run_cmd_sync(rpc_queue0, 0, &cmd, sizeof(cmd), 
                                 result_addr, sizeof(result));
  if (status != XIPC_OK) {
    xmp_log("Error enqueuing command to rpc_queue0\n");
    return -1;
  }
  
  // Invalidate stale copy of result
#if XCHAL_DCACHE_SIZE>0 && !XCHAL_DCACHE_IS_COHERENT
#if XCHAL_HAVE_L2_CACHE
  xthal_dcache_L1_region_invalidate(&result, sizeof(result));
#else
  xthal_dcache_region_invalidate(&result, sizeof(result));
#endif
#endif

  if (result != (cmd+1)) {
    xmp_log("Expected %x, but got %x\n", cmd+1, result);
    return -1;
  } 

  return 0;
}

int test4()
{
  xipc_status_t status;

  // Enqueue a synchronous command to DSP1
  int cmd = 4000;
  int result  __attribute__((aligned(XCHAL_DCACHE_LINESIZE)));

  // Get system address space address in case if object in local memory
  void *result_addr = xipc_get_sys_addr(&result, xipc_get_my_pid());

  // Enqueue command. Use default namespace handler on target.
  status = xipc_rpc_run_cmd_sync(rpc_queue0, USER_NSID, &cmd, sizeof(cmd), 
                                 result_addr, sizeof(result));
  if (status != XIPC_OK) {
    xmp_log("Error enqueuing command to rpc_queue0\n");
    return -1;
  }
  
  // Invalidate stale copy of result
#if XCHAL_DCACHE_SIZE>0 && !XCHAL_DCACHE_IS_COHERENT
#if XCHAL_HAVE_L2_CACHE
  xthal_dcache_L1_region_invalidate(&result, sizeof(result));
#else
  xthal_dcache_region_invalidate(&result, sizeof(result));
#endif
#endif

  if (result != (cmd*2)) {
    xmp_log("Expected %x, but got %x\n", cmd*2, result);
    return -1;
  } 

  return 0;
}

int main()
{
  xipc_status_t status;

  xmp_set_mpu_attrs();
  /* Mark region to be uncached */
  xmp_set_mpu_attrs_region_uncached((void *)&_shared_sysram_uncached_data_start,
                                    16*1024);

  // Initialize the XIPC library. Note, all procs implicitly synchronize
  // at the end of this call.
  xipc_initialize();

  // Setup the inter-processor message channels
  xipc_setup_channels();

  // Initialize the RPC command queue for DSP1 and DSP2.

  status = xipc_rpc_client_queue_initialize(rpc_queue0, XMP_DSP_1_PID);
  if (status != XIPC_OK) {
    xmp_log("Error initializing rpc_queue0\n");
    return -1;
  }

  xipc_rpc_client_queue_initialize(rpc_queue1, XMP_DSP_2_PID);
  if (status != XIPC_OK) {
    xmp_log("Error initializing rpc_queue1\n");
    return -1;
  }

  int result = 0;
  result = test1();
  result |= test2();
  result |= test3();
  result |= test4();

  if (result)
    xmp_log("Fail\n");
  else
    xmp_log("Pass\n");

  xipc_rpc_client_shutdown(rpc_queue0);

  // Wait until all procs are done
  xipc_reset_sync(sync);

  return 0;
}
