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

#if (!(XCHAL_HAVE_EXCLUSIVE || XCHAL_HAVE_S32C1I))
  xmp_log("This test expects requires either conditional store or exclusive load/stores \n");
  exit(-1);
#endif

#if XCHAL_HAVE_MPU
  xmp_set_mpu_attrs();
  /* Mark region to be uncached */
  xmp_set_mpu_attrs_region_uncached((void *)&_shared_sysram_uncached_data_start,
                                    16*1024);
#endif

  // Master proc initializes the semaphore
  if (my_procid == XMP_MASTER_PID) {
    if ((err = xipc_sem_init(&shared_sem, 1, XIPC_SPIN_WAIT)) != XIPC_OK) {
      xmp_log("Error initializing semaphore: err: %d\n", err);
      exit(-1);
    }
  }

  // Initialize the XIPC library. Note, all procs implicitly synchronize
  // at the end of this call.
  xipc_initialize();

  for (i = 0; i < NUM_ITERS; i++) {
    // Acquire semaphore before entering critical section
    if ((err = xipc_sem_wait(&shared_sem)) != XIPC_OK) {;
      xmp_log("Error acquiring semaphore, err: %d\n", err);
      exit(-1);
    }
    xthal_dcache_line_invalidate(&shared_data);
    shared_data++;
    xthal_dcache_line_writeback(&shared_data);
    // Release semaphore after writing data to memory
    xipc_sem_post(&shared_sem);
  }

  // Wait for all procs to finish
  if ((err = xipc_reset_sync(simple_sync)) != XIPC_OK) {
    xmp_log("Error in sync\n");
    exit(-1);
  }

  // Check results
  for (i = 0; i < XMP_NUM_PROCS; i++) {
    xthal_dcache_region_invalidate(&shared_data, sizeof(int));
    if (shared_data != XMP_NUM_PROCS*NUM_ITERS) {
      xmp_log("Expecting %d, but got %d\n", XMP_NUM_PROCS*NUM_ITERS, 
              shared_data);
      ok = 0;
    }
  }

  if (!ok)
    xmp_log("Failure\n");
  else
    xmp_log("Success\n");

  return 0;
}
