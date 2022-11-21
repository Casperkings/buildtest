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

extern void xipc_set_interrupt(int interrupt_id);

/* Defined in shared memory */
extern char *xmp_xipc_shared_data;

/* Message channel type used during initial channel setup */
xipc_msg_channel_t *xmp_xipc_msg_channels;

/* Top level initialization routine. Needs to be called prior to using
 * the XIPC primitives. All cores are synchronized after this call.
 *
 * Returns void
 */
void xipc_initialize()
{
  xipc_pid_t pid = xipc_get_proc_id();
  xipc_pid_t master_pid = xipc_get_master_pid();
  uint32_t num_procs = xipc_get_num_procs();
  
  /* Initialize interrupts, if defined, used by all XIPC components */
  if (xipc_get_interrupt_id(pid) != -1)
    xipc_set_interrupt(xipc_get_interrupt_id(pid));

  /* Initialize to start of the shared data section. Assumes that it is aligned
   * to the max cache line size across all cores */
  xipc_reset_sync_t *xmp_xipc_reset_sync = 
                     (xipc_reset_sync_t *)xmp_xipc_shared_data;

  /* Each xipc_reset_sync_t is of max cache line size */
  xmp_xipc_msg_channels = 
    (xipc_msg_channel_t *)(xmp_xipc_shared_data + 
                           xipc_get_max_dcache_linesize()*xipc_get_num_procs());

  /* Master proc does the initialization of all the channels */
  if (pid == master_pid) {
    /* Each message channel is rounded to max cache line size across all 
     * processors in the subsystem */
    uint32_t max_dcache_linesize = xipc_get_max_dcache_linesize();
    uint32_t msg_ch_size = 
      ((sizeof(xipc_msg_channel_t) + max_dcache_linesize - 1) / max_dcache_linesize) * max_dcache_linesize;
    /* Initialize msg_channels. There are n*(n-1) such channels for an 
     * n-processor subsystem */
    xipc_pid_t dest, src;
    int c = 0;
    for (src = master_pid; src < master_pid+num_procs; src++) {
      for (dest = master_pid; dest < master_pid+num_procs; dest++) {
        if (src == dest)
          continue;
        xipc_msg_channel_t *ch = 
            (xipc_msg_channel_t *)
            ((uintptr_t )xmp_xipc_msg_channels + msg_ch_size*c);
        xipc_msg_channel_initialize(ch, src, dest, c);
        c++;
      }
    }
  }

  /* Synchronize across all cores */
  xipc_reset_sync(xmp_xipc_reset_sync);
}
