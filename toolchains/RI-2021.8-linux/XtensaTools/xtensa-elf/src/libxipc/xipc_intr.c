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
#include <stddef.h>
#include "xipc_intr.h"
#include "xipc_misc.h"
#include "xipc_common.h"

/* Externally defined in the subsystem */
extern const xipc_pid_t xmp_xipc_num_procs;
extern const unsigned xmp_xipc_max_intrs_per_proc;
extern xipc_proc_intr_desc_t xmp_xipc_proc_intr_descs[];

/* Function to trigger an interrupt at a target proc */
void
xipc_interrupt_proc(xipc_pid_t proc_id, uint32_t intr)
{
  if (proc_id >= xmp_xipc_num_procs)
    return;
  xipc_proc_intr_desc_t *desc = xmp_xipc_proc_intr_descs + 
                                proc_id*xmp_xipc_max_intrs_per_proc;
  while (desc->_binterrupt != -1) {
    // If the interrupt number matches, write the mask to the mmio address.
    // Assumes edge-triggered
    if (desc->_binterrupt == intr && desc->_intr_type == XIPC_INTR_TYPE_EDGE) {
      *(volatile unsigned *)desc->_mmio_addr = desc->_mask;
      *(volatile unsigned *)desc->_mmio_addr = 0;
      break;
    }
    ++desc;
  }
}
