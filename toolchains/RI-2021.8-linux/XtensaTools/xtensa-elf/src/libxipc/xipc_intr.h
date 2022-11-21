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
#ifndef __XIPC_INTR_H__
#define __XIPC_INTR_H__

/*! @file */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "xipc_common.h"

#define XIPC_INTR_TYPE_EDGE  0
#define XIPC_INTR_TYPE_LEVEL 1

/*! Data structure defining the mmio addresses and masks for proc interrupts */
struct xipc_proc_intr_desc_struct {
  int32_t _binterrupt;  /*! Interrupt number */
  uint32_t _mask;       /*! Mask for triggering interrupt */
  uintptr_t _mmio_addr; /*! Address of MMIO */
  uint8_t _intr_type;   /*! XIPC_INTR_TYPE_EDGE or XIPC_INTR_TYPE_LEVEL */
};

/*! Data structure defining the mmio address and mask for triggering 
 *  interrupt
 */
typedef struct xipc_proc_intr_desc_struct xipc_proc_intr_desc_t;

/*! 
 * Function to trigger an interrupt at a target processor.
 *
 * \param proc_id Target processor id.
 * \param intr    Interrupt on the target processor
 */
extern void xipc_interrupt_proc(xipc_pid_t proc_id, uint32_t intr);

#ifdef __cplusplus
}
#endif

#endif /* __XIPC_INTR_H__ */
