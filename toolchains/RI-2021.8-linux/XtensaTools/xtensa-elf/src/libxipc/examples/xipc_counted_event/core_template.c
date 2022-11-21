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

  // Master proc initializes the events
  if (my_procid == XMP_MASTER_PID) {
    err = xipc_counted_event_init(&prod_done, XIPC_SPIN_WAIT);
    if (err != XIPC_OK) {
      xmp_log("Error initializing xipc_counted_event, err: %d\n", err);
      exit(-1);
    }
    err = xipc_counted_event_init(&cons_done, XIPC_SPIN_WAIT);
    if (err != XIPC_OK) {
      xmp_log("Error initializing xipc_counted_event, err: %d\n", err);
      exit(-1);
    }
  }

  // Initialize the XIPC library. Note, all procs implicitly synchronize
  // at the end of this call.
  xipc_initialize();

  int i, j;
  for (i = 0; i < NUM_ITERS; i++) {
    // Master proc generates new data to buffer and notifies other procs
    if (my_procid == XMP_MASTER_PID) {
      for (j = 0; j < BUFFER_SIZE; j++)
        buffer[j] = i*1000+j;
      xthal_dcache_region_writeback(buffer, BUFFER_SIZE*sizeof(int));
      err = xipc_set_counted_event(&prod_done);
      if (err != XIPC_OK) {
        xmp_log("Error setting xipc_counted_event, err: %d\n", err);
        exit(-1);
      }
      // Wait for other procs to be done reading the i-th buffer
      xipc_wait_counted_event(&cons_done, (XMP_NUM_PROCS-1)*(i+1));
    } else {
      // Wait for the producer to generate the i-th buffer
      xipc_wait_counted_event(&prod_done, i+1);
      xthal_dcache_region_invalidate(buffer, BUFFER_SIZE*sizeof(int));
      for (j = 0; j < BUFFER_SIZE; j++) {
        if (buffer[j] != i*1000+j) {
          xmp_log("Error, expecting %d, but got %d\n", i*1000+j, buffer[j]);
          ok = 0;
        }
      }
      // Notify the master proc 
      xipc_set_counted_event(&cons_done);
    }
  }

  if (!ok)
    xmp_log("Failure\n");
  else
    xmp_log("Success\n");

  return 0;
}
