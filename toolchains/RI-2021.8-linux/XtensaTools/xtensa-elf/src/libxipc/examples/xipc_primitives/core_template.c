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

  // All cores increment the shared counter
  for (i = 0; i < 100; i++)
    xipc_atomic_int_increment(&counter, 1);

  // Acquire lock to protect access to shared_data
  xipc_simple_spinlock_acquire(&lock);

  // Write to this core's location
  shared_data[my_procid] = (my_procid+1)*100;

  // Writeback cache line to make it visible to other procs
  xthal_dcache_line_writeback(&shared_data[my_procid]);

  // Release lock
  xipc_simple_spinlock_release(&lock);

  // Wait for all procs to finish the write
  if ((err = xipc_reset_sync(sync1)) != XIPC_OK) {
    xmp_log("Error in xipc_reset_sync\n");
    exit(-1);
  }

  // Check results
  for (i = 0; i < XMP_NUM_PROCS; i++) {
    // Invalidate cache line before reading data written by another proc
    xthal_dcache_line_invalidate(&shared_data[i]);
    if (shared_data[i] != (i + 1)*100) {
      xmp_log("Expecting %d, but got %d\n", (i + 1)*100, shared_data[i]);
      ok = 0;
    }
  }

  if (counter != XMP_NUM_PROCS*100) {
    xmp_log("Expecting counter to be %d, but got %d\n", 
            XMP_NUM_PROCS*100, counter);
    ok = 0;
  }

  if (!ok)
    xmp_log("Failure\n");
  else
    xmp_log("Success\n");

  return 0;
}
