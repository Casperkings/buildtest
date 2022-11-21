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

// Max of NUM_PKTS outstanding packets of size PKT_SIZE
#define NUM_PKTS 4
// Note, since different packets are being read/written by the two cores, for 
// non-coherent caches, each packet is assumed to cache line size to avoid
// false sharing
#define PKT_SIZE XMP_MAX_DCACHE_LINESIZE

// Output ports for proc0 to send packets to all other procs. 
// Defined in local memory.
xipc_pkt_channel_output_port_t ops[XMP_NUM_PROCS-1] __attribute__ ((section(".dram0.data")));

// Input port for each proc to receive packet from proc0. 
// Defined in local memory
xipc_pkt_channel_input_port_t ip __attribute__ ((section(".dram0.data")));

// Packet buffer defined in local memory of this proc
char buffer[NUM_PKTS*PKT_SIZE] __attribute__ ((section(".dram0.data")));

extern void xmp_set_mpu_attrs();
extern void xmp_set_mpu_attrs_region_uncached(void *, size_t);

int main()
{
  int i, j;
  unsigned k;
  int ok = 1;
  xipc_status_t err;
  int my_procid = XMP_GET_PRID();

#if XCHAL_HAVE_MPU
  xmp_set_mpu_attrs();
  /* Mark region to be uncached */
  xmp_set_mpu_attrs_region_uncached((void *)&_shared_sysram_uncached_data_start,
                                    16*1024);
#endif

  // Initialize the XIPC library. Note, all procs implicitly synchronize
  // at the end of this call.
  xipc_initialize();

  // Setup the inter-processor message channels
  xipc_setup_channels();

  if (my_procid == 0) {
    // Initialize output ports. 
    for (i = 1; i < XMP_NUM_PROCS; i++) {
      err = xipc_pkt_channel_output_port_initialize(&ops[i-1], i, i);
      if (err != XIPC_OK) {
        xmp_log("Error initializing output port, err : %d\n", err);
        exit(-1);
      }

      // Attempt connection to destination proc
      err = xipc_pkt_channel_output_port_connect(&ops[i-1]);
      if (err != XIPC_OK) {
        xmp_log("Error connecting to dest proc, err: %d\n", err);
        exit(-1);
      }
    }
  } else {
    // Initialize input port.
    err = xipc_pkt_channel_input_port_initialize(&ip, my_procid, 0);
    if (err != XIPC_OK) {
      xmp_log("Error initializing input port, err: %d\n", err);
      exit(-1);
    } 

    // Set attributes at the input.
    // Set the address of the buffer in this proc's local memory
    err = xipc_pkt_channel_input_port_set_attr(
                     &ip, XIPC_PKT_CHANNEL_BUFFER_ADDR_ATTR, (uint32_t)buffer);
    if (err != XIPC_OK) {
      xmp_log("Error setting buffer addr attr, err: %d\n", err);
      exit(-1);
    }
  
    // Sets the number of outstanding packets in packet buffer
    err = xipc_pkt_channel_input_port_set_attr(
                             &ip, XIPC_PKT_CHANNEL_NUM_PACKETS_ATTR, NUM_PKTS);
    if (err != XIPC_OK) {
      xmp_log("Error setting num pkts attr, err: %d\n", err);
      exit(-1);
    }

    // Sets the per packet size
    err = xipc_pkt_channel_input_port_set_attr(&ip,
                                              XIPC_PKT_CHANNEL_PACKET_SIZE_ATTR,
                                              PKT_SIZE);
    if (err != XIPC_OK) {
     xmp_log("Error setting pkt size attr, err: %d", err);
     exit(-1);
    }

    // Attempt connection to source proc
    err = xipc_pkt_channel_input_port_connect(&ip);
    if (err != XIPC_OK) {
      xmp_log("Error connecting to src proc, err: %d\n", err);
      exit(-1);
    }
  }

  if (my_procid == 0) {
    // On every iteration, send a packet to other procs.
    for (i = 0; i < NUM_ITERS; i++) {
      for (j = 1; j < XMP_NUM_PROCS; j++) {
        xipc_pkt_t pkt;
        // Allocate packet from packet buffer
        xipc_pkt_channel_allocate(&ops[j-1], &pkt, XIPC_SPIN_WAIT);
        // Allocated as an array of shorts.
        short *p = (short *)xipc_pkt_get_ptr(&pkt);
        for (k = 0; k < xipc_pkt_get_size(&pkt)/2; k++)
          p[k] = i*1000 + j*100 + k;
        // Queue up the allocated packet
        err = xipc_pkt_channel_send(&ops[j-1], &pkt, XIPC_SPIN_WAIT); 
        if (err != XIPC_OK) {
          xmp_log("Error sending packets, err: %d\n", err);
          exit(-1);
        }
      }
    }
  } else {
    // Each of the other procs receives NUM_ITERS packets from proc 0
    for (i = 0; i < NUM_ITERS; i++) {
      xipc_pkt_t pkt;
      // Receive earliest queued packet
      xipc_pkt_channel_recv(&ip, &pkt, XIPC_SPIN_WAIT);
      // Received as an array of shorts.
      unsigned short *p = (unsigned short *)xipc_pkt_get_ptr(&pkt);
      for (k = 0; k < xipc_pkt_get_size(&pkt)/2; k++) {
        if (p[k] != i*1000 + my_procid*100 + k) {
          xmp_log("Error, expecting %d, but got %d\n", 
                  i*1000+my_procid*100+k, p[k]);
          ok = 0;
        }
      }
      // Free up packet back to the packet buffer
      xipc_pkt_channel_release(&ip, &pkt, XIPC_SPIN_WAIT);
    }
  }

  if (!ok)
    xmp_log("Failure\n");
  else
    xmp_log("Success\n");

  // Wait until all procs are done
  xipc_reset_sync(sync);

  return 0;
}
