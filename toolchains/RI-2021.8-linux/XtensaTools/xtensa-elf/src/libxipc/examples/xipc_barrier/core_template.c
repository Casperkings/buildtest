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
#include <xtensa/xipc/xipc_primitives.h>
#include "shared.h"
#include "xmp_log.h"

extern void xmp_set_mpu_attrs();
extern void xmp_set_mpu_attrs_region_uncached(void *, size_t);

int main()
{
  int i;
  xipc_status_t err;
  int ok = 1;
  int my_procid = XT_RSR_PRID();
  int *p;

#if (!(XCHAL_HAVE_EXCLUSIVE || XCHAL_HAVE_S32C1I))
  xmp_log("This test expects requires either conditional store or exclusive load/stores \n");
  exit(-1);
#endif

#if XCHAL_HAVE_MPU
  xmp_set_mpu_attrs();
  /* Mark region to be uncached */
  xmp_set_mpu_attrs_region_uncached((void *)&_shared_sysram_uncached_data_start,
                                    16*1024);
#if XCHAL_L2RAM_ONLY
  xmp_set_mpu_attrs_region_uncached((void *)XMP_L2_RAM_ADDR, XMP_L2_RAM_SIZE);
#endif
#endif

  // Master proc initializes the barrier
  if (my_procid == XMP_XIPC_MASTER_PID) {
    xipc_pid_t proc_ids[] = {XMP_PROC_ARRAY_P(PID)};
    if ((err = xipc_barrier_init(&shared_barrier, 
                                 XIPC_SPIN_WAIT, 
                                 XMP_XIPC_NUM_PROCS,
                                 proc_ids,
                                 XMP_XIPC_NUM_PROCS)) != XIPC_OK) {
      xmp_log("Error initializing xipc_barrier, err: %d\n", err);
      exit(-1);
    }
#if XCHAL_L2RAM_ONLY
    if ((err = xipc_barrier_init(&shared_l2_barrier, 
                                 XIPC_SPIN_WAIT, 
                                 XMP_XIPC_NUM_PROCS,
                                 proc_ids,
                                 XMP_XIPC_NUM_PROCS)) != XIPC_OK) {
      xmp_log("Error initializing xipc_barrier, err: %d\n", err);
      exit(-1);
    }
#endif
  }

  // Initialize the XIPC library. Note, all procs implicitly synchronize
  // at the end of this call.
  xipc_initialize();

  // Write to this core's location
  p = (int *)&shared_data[my_procid * XMP_MAX_DCACHE_LINESIZE];
  *p = (my_procid+1)*100;

  // Writeback cache line to make it visible to other procs
  xthal_dcache_line_writeback(&shared_data[my_procid*XMP_MAX_DCACHE_LINESIZE]);

  // Wait for all procs to finish the write
  xipc_barrier_wait(&shared_barrier);

#if XCHAL_L2RAM_ONLY
  // Write to this core's location in shared L2
  shared_l2_data[my_procid] = (my_procid+1)*100;

  // Wait for all procs to finish the write
  xipc_barrier_wait(&shared_l2_barrier);
#endif

  // Check results
  for (i = XMP_XIPC_MASTER_PID; 
       i < (XMP_XIPC_MASTER_PID + XMP_XIPC_NUM_PROCS); ++i) {
    // Invalidate cache line before reading data written by another proc
    xthal_dcache_line_invalidate(&shared_data[i*XMP_MAX_DCACHE_LINESIZE]);
    int *p = (int *)&shared_data[i * XMP_MAX_DCACHE_LINESIZE];
    if (*p != (i + 1)*100) {
      xmp_log("Shared mem test: Expecting %d, but got %d\n", (i + 1)*100, 
              shared_data[i*XMP_MAX_DCACHE_LINESIZE]);
      ok = 0;
    }

#if XCHAL_L2RAM_ONLY
    if (shared_l2_data[i] != (i + 1)*100) {
      xmp_log("Shared l2 mem test: Expecting %d, but got %d\n", (i + 1)*100, 
              shared_l2_data[i]);
      ok = 0;
    }
#endif
  }

  // Wait for all procs to finish
  xipc_barrier_wait(&shared_barrier);

  if (!ok)
    xmp_log("Failure\n");
  else
    xmp_log("Success\n");

  if ((err = xipc_reset_sync(simple_sync)) != XIPC_OK) {
    xmp_log("Error in sync\n");
    exit(-1);
  }

  return 0;
}
