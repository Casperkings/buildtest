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

extern void put(buffer_t *buffer, int x);
extern int take(buffer_t *buffer);

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

  uint32_t has_xipc_interrupts[] = {XMP_PROC_ARRAY_P(HAS_XIPC_INTR)};
  if (has_xipc_interrupts[0] == 0) {
    xmp_log("Conditional variables are blocking and requires XIPC interrupts to be configured in the subsystem\n");
    exit(-1);
  }

  // Master proc initializes the condition variable and the associated
  // mutex
  if (my_procid == XMP_MASTER_PID) {
    xipc_pid_t proc_ids[] = {XMP_PROC_ARRAY_P(PID)};
    if ((err = xipc_barrier_init(&shared_barrier,
                                 XIPC_SPIN_WAIT,
                                 XMP_NUM_PROCS,
                                 proc_ids,
                                 XMP_NUM_PROCS)) != XIPC_OK) {
      xmp_log("Error initializing xipc_barrier, err: %d\n", err);
      exit(-1);
    }
    if ((err = xipc_mutex_init(&buffer.lock, XIPC_SPIN_WAIT)) != XIPC_OK) {
      xmp_log("Error initializing lock: err: %d\n", err);
      exit(-1);
    }
    if ((err = xipc_cond_init(&buffer.not_full, XIPC_SPIN_WAIT)) != XIPC_OK) {
      xmp_log("Error initializing condition not full: err: %d\n", err);
      exit(-1);
    }
    if ((err = xipc_cond_init(&buffer.not_empty, XIPC_SPIN_WAIT)) != XIPC_OK) {
      xmp_log("Error initializing condition not empty: err: %d\n", err);
      exit(-1);
    }
    buffer.count = 0;
    buffer.put_ptr = 0;
    buffer.take_ptr = 0;
    XIPC_WRITE_BACK_ELEMENT(&buffer);
    xipc_atomic_int_init(&sum, 0);
  }

  // Initialize the XIPC library. Note, all procs implicitly synchronize
  // at the end of this call.
  xipc_initialize();

  // The shared memory operations work only for configs with either 
  // conditional stores or exclusives
  int i, j;
  // On every iteration, send NUM_ITERS*(num procs - 1) data items
  if (my_procid == XMP_MASTER_PID) {
    for (i = 0; i < NUM_ITERS; i++) {
      for (j = 1; j < XMP_NUM_PROCS; j++) {
        put(&buffer, i*100+j);
      }
    }
    // Wait for all other procs
    xipc_barrier_wait(&shared_barrier);
  } else {
    // Each of the other procs receives NUM_ITERS data items from master proc
    // Add up the received values.
    for (i = 0; i < NUM_ITERS; i++) {
      int d = take(&buffer);
      xipc_atomic_int_increment(&sum, d);
    }
    int expected_sum = 0;
    for (i = 0; i < NUM_ITERS; i++) {
      for (j = 1; j < XMP_NUM_PROCS; j++) {
        expected_sum += i*100+j;
      }
    }
    // Wait for all other procs
    xipc_barrier_wait(&shared_barrier);
    // Check results
    if (xipc_atomic_int_value(&sum) != expected_sum) {
        xmp_log("Error, expecting %d, but got %d\n", 
                expected_sum, xipc_atomic_int_value(&sum));
        ok = 0;
    }
  }

  if (!ok)
    xmp_log("Failure\n");
  else
    xmp_log("Success\n");

  return 0;
}
