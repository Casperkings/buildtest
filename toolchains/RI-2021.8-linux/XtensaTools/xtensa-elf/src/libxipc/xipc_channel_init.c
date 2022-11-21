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
#include <stdint.h>
#include <xtensa/config/core-isa.h>
#include "xipc_addr.h"
#include "xipc_misc.h"
#include "xipc_common.h"
#include "xipc_barrier_internal.h"
#include "xipc_msg_channel_internal.h"

uint8_t xipc_msg_channels_setup XIPC_DRAM;

/* Externally defined in the subsystem */
extern xipc_msg_channel_input_port_t xmp_xipc_msg_channel_my_input_ports[];
extern xipc_msg_channel_output_port_t xmp_xipc_msg_channel_my_output_ports[];
extern const uint32_t xmp_xipc_msg_channel_num_msgs;
extern int32_t xmp_xipc_msg_channel_my_buffer[];

/* Defined in shared memory */
extern xipc_msg_channel_t *xmp_xipc_msg_channels;

/* Establish inter-processor message channels. This sets up message
 * channels between every pair of processors. This needs to be called
 * prior to using the message/packet channels.
 * Note, the packet channels uses the message channels for initial handshake.
 * Needs to be invoked after xipc_initialize.
 *
 * Returns void
 */
void 
xipc_setup_channels()
{
  xipc_pid_t pid = xipc_get_proc_id();

  /* Each message channel is rounded to max cache line size across all 
   * processors in the subsystem */
  uint32_t max_dcache_linesize = xipc_get_max_dcache_linesize();
  uint32_t msg_ch_size = 
            ((sizeof(xipc_msg_channel_t) + 
              max_dcache_linesize - 1)/max_dcache_linesize) * 
            max_dcache_linesize;

  int num_my_input_ports = 0;
  int num_my_output_ports = 0;
  int num_msg_channels = (xipc_get_num_procs()-1)*xipc_get_num_procs();
  int i;
  /* Connect the channels between all processors */
  for (i = 0; i < num_msg_channels; i++) {
    xipc_msg_channel_t *ch = 
      (xipc_msg_channel_t *)((uintptr_t )xmp_xipc_msg_channels + msg_ch_size*i);
    if (pid == xipc_msg_channel_get_src_pid(ch)) {
      xipc_msg_channel_output_port_t *op = 
               &xmp_xipc_msg_channel_my_output_ports[num_my_output_ports];
      xipc_msg_channel_output_port_initialize(
                                   op, xipc_msg_channel_get_dest_pid(ch));
      xipc_msg_channel_output_port_connect(ch, op);
      while (!xipc_msg_channel_input_port_connected(ch))
        ;
      xipc_msg_channel_output_port_accept(ch, op);
      num_my_output_ports++;
    }
    if (pid == xipc_msg_channel_get_dest_pid(ch)) {
      xipc_msg_channel_input_port_t *ip = 
               &xmp_xipc_msg_channel_my_input_ports[num_my_input_ports];
      xipc_msg_channel_input_port_initialize(
        ip, xipc_msg_channel_get_src_pid(ch),
        &xmp_xipc_msg_channel_my_buffer[num_my_input_ports*
                                        xmp_xipc_msg_channel_num_msgs],
        xmp_xipc_msg_channel_num_msgs
      );
      while (!xipc_msg_channel_output_port_connected(ch))
        ;
      xipc_msg_channel_input_port_connect(ch, ip);
      xipc_msg_channel_input_port_accept(ch, ip);
      num_my_input_ports++;
    }
  }

  xipc_msg_channels_setup = 1;
}
