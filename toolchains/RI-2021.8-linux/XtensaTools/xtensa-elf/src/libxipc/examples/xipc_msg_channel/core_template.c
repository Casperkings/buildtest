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

/* The ports need to be in local memory */
xipc_msg_channel_output_port_t op __attribute__ ((section(".dram0.data")));
xipc_msg_channel_input_port_t ip __attribute__ ((section(".dram0.data")));
/* The buffer for the channel is also in local memory */
int buffer[32]  __attribute__ ((section(".dram0.data")));

// Enable below if you want to place the pre-defined message channel
// structures in L2 instead of local memory where the systembuilder places
// by default
#if 0
xipc_msg_channel_input_port_t
xmp_xipc_msg_channel_my_input_ports[XMP_XIPC_NUM_PROCS-1] __attribute__ ((section(".l2ram.data")));

xipc_msg_channel_output_port_t
xmp_xipc_msg_channel_my_output_ports[XMP_XIPC_NUM_PROCS-1] __attribute__ ((section(".l2ram.data")));

int32_t xmp_xipc_msg_channel_my_buffer[(XMP_XIPC_MSG_CHANNEL_DEFAULT_NUM_MSGS+1)*(XMP_XIPC_NUM_PROCS-1)] __attribute__ ((section(".l2ram.data")));
#endif

extern void xmp_set_mpu_attrs();
extern void xmp_set_mpu_attrs_region_uncached(void *, size_t);

int main()
{
  int i;
  int ok = 1;
  int my_procid = XT_RSR_PRID();
  xipc_status_t err;

#if XCHAL_HAVE_MPU
  xmp_set_mpu_attrs();
  /* Mark region to be uncached */
  xmp_set_mpu_attrs_region_uncached((void *)&_shared_sysram_uncached_data_start,
                                    16*1024);
#endif
#if XCHAL_L2RAM_ONLY
  xmp_set_mpu_attrs_region_uncached((void *)XMP_L2_RAM_ADDR, XMP_L2_RAM_SIZE);
#endif

  // Core 0 creates custom message channel from proc0 -> proc1 -> ... -> proc0
  if (my_procid == 0) {
    for (i = 0; i < (XMP_NUM_PROCS-1); i++) {
      if ((err = xipc_msg_channel_initialize(&msg_channels[i], 
                                             i, 
                                             i+1, 
                                             100+i)) != XIPC_OK) {
        xmp_log("Error initializing msg channel %d, err: %d\n", i, err);
        exit(-1);
      }
    }
    if ((err = xipc_msg_channel_initialize(&msg_channels[i], 
                                           XMP_NUM_PROCS-1, 
                                           0, 
                                           100+i)) != XIPC_OK) {
      xmp_log("Error initializing msg channel %d, err: %d\n", i, err);
      exit(-1);
    }
  }

  // Initialize the XIPC library. Note, all procs implicitly synchronize
  // at the end of this call.
  xipc_initialize();

  // Setup the inter-processor message channels
  xipc_setup_channels();

  /* Establish custom channel connection */
  for (i = 0; i < XMP_NUM_PROCS; i++) {
    xipc_msg_channel_t *ch = &msg_channels[i];
    if (my_procid == xipc_msg_channel_get_src_pid(ch)) {
      /* The producer of the channel initializes the output port */
      if ((err = xipc_msg_channel_output_port_initialize(
                               &op, 
                               xipc_msg_channel_get_dest_pid(ch))) != XIPC_OK) {
        xmp_log("Error initializing output port, err %d\n", err);
        exit(-1);
      }
      /* Establish connection with the consumer */
      xipc_msg_channel_output_port_connect(ch, &op);
      /* Wait for consumer to establish connection */
      while (!xipc_msg_channel_input_port_connected(ch))
        ;
      /* Accept connection request from consumer */
      xipc_msg_channel_output_port_accept(ch, &op);
    }
    if (my_procid == xipc_msg_channel_get_dest_pid(ch)) {
      /* The consumer of the channel initializes the input port. The buffer
         for the channel is in consumer's local memory */
      if ((err = xipc_msg_channel_input_port_initialize(
                                             &ip, 
                                             xipc_msg_channel_get_src_pid(ch),
                                             buffer,
                                             32)) != XIPC_OK) {
        xmp_log("Error initializing input port, err %d\n", err);
        exit(-1);
      }
      /* Wait for producer to establish connection */
      while (!xipc_msg_channel_output_port_connected(ch))
        ;
      /* Establish connection with producer */
      xipc_msg_channel_input_port_connect(ch, &ip);
      /* Accept connection request from consumer */
      xipc_msg_channel_input_port_accept(ch, &ip);
    }
  }


  // Send message from proc0 -> proc1 -> proc2 -> .. -> proc0
  xipc_pid_t dest_pid = ((my_procid+1) == XMP_NUM_PROCS) ? 0 : my_procid+1;
  xipc_pid_t src_pid  = (my_procid == 0) ? XMP_NUM_PROCS-1 : my_procid-1;

  for (i = 0; i < NUM_ITERS; i++) {
    if (my_procid == XMP_MASTER_PID) {
      // Master sends to 'next' proc and receives from the 'previous' proc
      // and compares the sent data to the received data.
      int sdata = 100*i;
      int rdata;

      xipc_msg_channel_send(xipc_msg_channel_get_output_port(dest_pid),
                            &sdata,
                            XIPC_SPIN_WAIT);
      xipc_msg_channel_recv(xipc_msg_channel_get_input_port(src_pid),
                            &rdata,
                            XIPC_SPIN_WAIT);
      if (sdata != rdata) {
        xmp_log("Expecting %d, but got %d\n", sdata, rdata);
        ok = 0;
      }
    } else {
      // All other cores just forwards from the 'previous' to the 'next'.
      int rdata;
      xipc_msg_channel_recv(xipc_msg_channel_get_input_port(src_pid),
                            &rdata,
                            XIPC_SPIN_WAIT);
      xipc_msg_channel_send(xipc_msg_channel_get_output_port(dest_pid),
                            &rdata,
                            XIPC_SPIN_WAIT);
    }
  }

  // Similar test as above, but using custom created channels
  for (i = 0; i < NUM_ITERS; i++) {
    if (my_procid == XMP_MASTER_PID) {
      // Master sends to 'next' proc and receives from the 'previous' proc
      // and compares the sent data to the received data.
      // Perform an explicit allocate
      int *sdata = xipc_msg_channel_allocate(&op, 1, XIPC_SPIN_WAIT);
      if (sdata == 0) {
        xmp_log("Error allocating from channel\n");
        exit(-1);
      }
      int d = 100*i;
      *sdata = d;
      // Notify consumer
      if ((err = xipc_msg_channel_commit(&op, 1, XIPC_SPIN_WAIT)) != XIPC_OK) {
        xmp_log("Error msg channel commit %d\n", err);
        exit(-1);
      }
      
      // Receive a ptr to data
      int *rdata = xipc_msg_channel_get(&ip, 1, XIPC_SPIN_WAIT);
      if (rdata == 0) {
        xmp_log("Error receiving from channel\n");
        exit(-1);
      }
      if (d != *rdata) {
        xmp_log("Expecting %d, but got %d\n", d, *rdata);
        ok = 0;
      }
      if ((err = xipc_msg_channel_release(&ip, 1, XIPC_SPIN_WAIT)) != XIPC_OK) {
        xmp_log("Error msg channel release %d\n", err);
        exit(-1);
      }
    } else {
      // All other cores just forwards from the 'previous' to the 'next'.
      int *rdata = xipc_msg_channel_get(&ip, 1, XIPC_SPIN_WAIT);
      if (rdata == 0) {
        xmp_log("Error receiving from channel\n");
        exit(-1);
      }
      int *sdata = xipc_msg_channel_allocate(&op, 1, XIPC_SPIN_WAIT);
      if (sdata == 0) {
        xmp_log("Error allocating from channel\n");
        exit(-1);
      }
      *sdata = *rdata;
      if ((err = xipc_msg_channel_commit(&op, 1, XIPC_SPIN_WAIT)) != XIPC_OK) {
        xmp_log("Error msg channel commit %d\n", err);
        exit(-1);
      }
      if ((err = xipc_msg_channel_release(&ip, 1, XIPC_SPIN_WAIT)) != XIPC_OK) {
        xmp_log("Error msg channel release %d\n", err);
        exit(-1);
      }
    }
  }

  if (my_procid == XMP_MASTER_PID) {
    if (!ok)
      xmp_log("Failure\n");
    else
      xmp_log("Success\n");
  }

  return 0;
}
