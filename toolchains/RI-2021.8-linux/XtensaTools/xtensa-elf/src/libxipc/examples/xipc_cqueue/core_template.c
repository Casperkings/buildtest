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
  int ok = 1;
  int my_procid = XMP_GET_PRID();
  xipc_status_t err;

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
    xmp_log("Concurrent queues are blocking and requires XIPC interrupts to be configured in the subsystem\n");
    exit(-1);
  }

  // Master proc initializes the cqueue
  if (my_procid == XMP_MASTER_PID) {
    // Master proc is the consumer, rest are producers
    xipc_pid_t prod_proc_ids[XMP_NUM_PROCS];
    xipc_pid_t cons_proc_ids[XMP_NUM_PROCS];
    int num_prods = 0;
    for (i = 0; i < XMP_NUM_PROCS; i++) {
      if (XMP_MASTER_PID != i)
        prod_proc_ids[num_prods++] = i;
    }
    cons_proc_ids[0] = XMP_MASTER_PID;

    // Initialize the queue
    err = xipc_cqueue_initialize(&cq_shared, 0, PKT_SIZE, NUM_PKTS,
                                 buffer, XMP_MASTER_PID,
                                 prod_proc_ids, num_prods,
                                 cons_proc_ids, 1, 1);
    if (err != XIPC_OK) {
      xmp_log("Error initializing xipc_cqueue, err: %d\n", err);
      exit(-1);
    }
  }

  // Initialize the XIPC library. Note, all procs implicitly synchronize
  // at the end of this call.
  xipc_initialize();

  // Master receives packets from multiple senders
  if (XT_RSR_PRID() == XMP_MASTER_PID) {
    // Receive NUM_ITERS from (XMP_NUM_PROCS-1) procs
    int i;
    for (i = 0; i < NUM_ITERS*(XMP_NUM_PROCS-1); i++) {
      xipc_pkt_t pkt;
      while (xipc_cqueue_try_recv(&cq_shared, &pkt) != XIPC_OK)
        ;
      // Receive latest queued packets as an array of shorts
      unsigned short *p = (unsigned short *)xipc_pkt_get_ptr(&pkt);
      xthal_dcache_region_invalidate(p, xipc_pkt_get_size(&pkt));
      unsigned j; 
      // The first item is the proc id of the slave
      short slave_pid = p[0];
      for (j = 1; j < xipc_pkt_get_size(&pkt)/2; j++) {
        if (p[j] != slave_pid*1000 + j) {
           xmp_log("Error in cqueue recv in pkt %d. Expected %d, got : %d\n",
                    i, slave_pid*1000 + j, p[j]);
           ok = 0;
        }
      }
      // Release the packet back to the queue
      xipc_cqueue_release(&cq_shared, &pkt, XIPC_SPIN_WAIT);
    }
  } else {
    // Send NUM_ITERS packets to the master
    int i; 
    for (i = 0; i < NUM_ITERS; i++) {
      xipc_pkt_t pkt;
      while (xipc_cqueue_try_allocate(&cq_shared, &pkt) != XIPC_OK)
        ;
      // Send packet as array of shorts
      unsigned short *p = (unsigned short *)xipc_pkt_get_ptr(&pkt);
      unsigned j; 
      p[0] = my_procid;
      for (j = 1; j < xipc_pkt_get_size(&pkt)/2; j++)
        p[j] = my_procid*1000 + j;
      xthal_dcache_region_writeback(p, xipc_pkt_get_size(&pkt));
      // Queue up the packet
      xipc_cqueue_send(&cq_shared, &pkt, XIPC_SPIN_WAIT);
    }
  }

  // Wait until all procs are done
  xipc_reset_sync(sync);

  if (!ok)
    xmp_log("Failure\n");
  else
    xmp_log("Success\n");

  return 0;
}
