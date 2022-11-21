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
#include <xtensa/config/core-isa.h>
#include <xtensa/xos.h>
#if XCHAL_HAVE_IDMA
#include <xtensa/idma.h>
#endif

#if XCHAL_NUM_DATARAM > 0
#  if XCHAL_NUM_DATARAM > 1 && XCHAL_DATARAM1_PADDR < XCHAL_DATARAM0_PADDR
#    define XIPC_XOS_DRAM __attribute__((section(".dram1.data")))
#  else
#    define XIPC_XOS_DRAM __attribute__((section(".dram0.data")))
#  endif
#else
#define XIPC_XOS_DRAM
#endif

/* Condition to wake up threads waiting on the XIPC interrupt */
XosCond xipc_xos_cond XIPC_XOS_DRAM;

/* Interrupt handler for the xipc interrupt. Notifies the xipc condition 
 * 
 * arg : unused
 */
static void xipc_interrupt_handler(void *arg) 
{
  xos_cond_signal(&xipc_xos_cond, 0);
}

/* Creates a condition that is triggered with the XIPC interrupt. Threads that
 * block using SLEEP_WAIT gets unblocked using the xipc condition.
 *
 * interrupt_id : XIPC interrupt id used for unblocking this thread
 */
void xipc_set_interrupt(int interrupt_id)
{
  xos_cond_create(&xipc_xos_cond);
  xos_register_interrupt_handler(interrupt_id, &xipc_interrupt_handler, 0);
#if XCHAL_HAVE_INTERRUPTS
  xos_interrupt_enable(interrupt_id);
#endif
}

