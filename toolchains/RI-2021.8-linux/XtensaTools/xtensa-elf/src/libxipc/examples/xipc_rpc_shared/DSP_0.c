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

int main()
{
  xipc_status_t status;
  xipc_rpc_event_t event0, event1;

  xmp_set_mpu_attrs();
  /* Mark region to be uncached */
  xmp_set_mpu_attrs_region_uncached((void *)&_shared_sysram_uncached_data_start,
                                    16*1024);

  // Initialize the XIPC library. Note, all procs implicitly synchronize
  // at the end of this call.
  xipc_initialize();

  // Setup the inter-processor message channels
  xipc_setup_channels();

  // Initialize the shared RPC command queue 

  xipc_pid_t pids[] = {XMP_PROC_ARRAY_P(PID)};
  status = xipc_rpc_squeue_initialize(rpc_queue, pids, XMP_XIPC_NUM_PROCS,
                                      RPC_CMD_SIZE, RPC_NUM_CMDS);
  if (status != XIPC_OK) {
    xmp_log("Error initializing rpc_queue\n");
    return -1;
  }

  // Enqueue a command to DSP
  int cmd0 = 0xdeadbeef;
  int result0 __attribute__((aligned(XCHAL_DCACHE_LINESIZE)));
  // Get system address space address in case if object in local memory
  void *result0_addr = xipc_get_sys_addr(&result0, xipc_get_my_pid());
  // Enqueue command. Use default namespace handler on target.
  status = xipc_rpc_run_cmd(rpc_queue, 0, &cmd0, sizeof(cmd0), 
                            result0_addr, sizeof(result0), &event0);
  if (status != XIPC_OK) {
    xmp_log("Error enqueuing command to rpc_queue\n");
    return -1;
  }

  // Enqueue a command to DSP
  int cmd1 = 0xbaadbeef;
  int result1 __attribute__((aligned(XCHAL_DCACHE_LINESIZE)));
  // Get system address space address in case if object in local memory
  void *result1_addr = xipc_get_sys_addr(&result1, xipc_get_my_pid());
  // Enqueue command. Use default namespace handler on target.
  status = xipc_rpc_run_cmd(rpc_queue, 0, &cmd1, sizeof(cmd1), 
                            result1_addr, sizeof(result1), &event1);
  if (status != XIPC_OK) {
    xmp_log("Error enqueuing command to rpc_queue\n");
    return -1;
  }
  
  // Wait for command to finish
  status = xipc_rpc_wait_event(&event0);
  if (status != XIPC_OK) {
    xmp_log("Error waiting for event0\n");
    return -1;
  }

  status = xipc_rpc_wait_event(&event1);
  if (status != XIPC_OK) {
    xmp_log("Error waiting for event1\n");
    return -1;
  }

  // Invalidate stale copy of result
#if XCHAL_DCACHE_SIZE>0 && !XCHAL_DCACHE_IS_COHERENT
#if XCHAL_HAVE_L2_CACHE
  xthal_dcache_L1_region_invalidate(&result0, sizeof(result0));
  xthal_dcache_L1_region_invalidate(&result1, sizeof(result1));
#else
  xthal_dcache_region_invalidate(&result0, sizeof(result0));
  xthal_dcache_region_invalidate(&result1, sizeof(result1));
#endif
#endif

  if (result0 != (cmd0+1)) {
    xmp_log("Expected %x, but got %x\n", cmd0+1, result0);
    xmp_log("Fail\n");
    return -1;
  }
  if (result1 != (cmd1+1)) {
    xmp_log("Expected %x, but got %x\n", cmd1+1, result1);
    xmp_log("Fail\n");
    return -1;
  }


  xmp_log("Pass\n");

  xipc_rpc_client_shutdown(rpc_queue);
  xipc_rpc_client_shutdown(rpc_queue);

  // Wait until all procs are done
  xipc_reset_sync(sync);

  return 0;
}
