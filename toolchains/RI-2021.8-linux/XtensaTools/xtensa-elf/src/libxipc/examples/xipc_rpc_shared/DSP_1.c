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
#include <xtensa/xipc/xipc.h>
#include <xtensa/system/xmp_subsystem.h>
#include "shared.h"
#include "xmp_log.h"

extern void xmp_set_mpu_attrs();
extern void xmp_set_mpu_attrs_region_uncached(void *, size_t);

xipc_status_t xipc_rpc_execute_cmd(void *cmd, uint32_t cmd_size,
                                   void *output, uint32_t output_size) {
  int *c = (int *)cmd;
  *((int *)output) = (*c + 1);
  // Invalidate stale copy of result
#if XCHAL_DCACHE_SIZE>0 && !XCHAL_DCACHE_IS_COHERENT
#if XCHAL_HAVE_L2_CACHE
  xthal_dcache_L1_region_writeback(output, output_size);
#else
  xthal_dcache_region_writeback(output, output_size);
#endif
#endif
  for (int i = 0; i < 1000000; i++)
    asm volatile ("_nop");
  return XIPC_OK;
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

  // Setup the rpc queue
  xipc_pid_t pids[] = {XMP_PROC_ARRAY_P(PID)};
  status = xipc_rpc_squeue_initialize(rpc_queue, pids, XMP_XIPC_NUM_PROCS,
                                      RPC_CMD_SIZE, RPC_NUM_CMDS);
  if (status != XIPC_OK) {
    xmp_log("Error initializing rpc_queue\n");
    return -1;
  }

  for (;;) {
    status = xipc_rpc_cmd_dispatch(rpc_queue);
    if (status == XIPC_ERROR_RPC_CMDQ_EMPTY)
      xipc_rpc_cmd_wait(rpc_queue);
    else if (status != XIPC_OK)
      break;
  }

  // Wait until all procs are done
  xipc_reset_sync(sync);

  return 0;
}
