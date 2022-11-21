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
#ifndef __XIPC_SYS_H__
#define __XIPC_SYS_H__

/* 
 * To port XIPC to a custom subsystem provide definitions of the following
 * for your target subsystem.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "xipc_common.h"

/*!
 * Returns the subsystem processor name given the processor id
 *
 * \param pid Processor ID.
 * \return    The name of the processor in the subsystem
 */
extern const char *xipc_get_proc_name(xipc_pid_t pid);

/*!
 * Returns the number of processors in the subsystem
 *
 * \return The number of processors in the subsystem
 */
extern xipc_pid_t xipc_get_num_procs();

/*!
 * Returns the id of the master processor in the subsystem
 *
 * \return The id of the master processor in the subsystem
 */
extern xipc_pid_t xipc_get_master_pid();

/*!
 * Returns the maximum dcache line size across all processors
 *
 * \return The maximum dcache line size across all processors
 */
extern uint32_t xipc_get_max_dcache_linesize();

/*!
 * Returns default interrupt for XIPC's inter-processor notification
 *
 * \return The default inter-processor interrupt for XIPC
 */
extern int32_t xipc_get_interrupt_id(xipc_pid_t pid);

/*!
 * Notify target processor with the default XIPC inter-processor interrupt 
 *
 * \param pid The target processor to notify
 */
extern void xipc_proc_notify(uint32_t pid);

/*!
 * Returns the id of this processor
 *
 * \return The id of this processor
 */
extern xipc_pid_t xipc_get_my_pid();

/*!
 * Returns the id of the processor based on its PRID
 *
 * \return The id of the master processor in the subsystem
 */
extern xipc_pid_t xipc_get_pid(uint32_t prid);


#ifdef __cplusplus
}
#endif

#endif /* __XIPC_SYS_H__ */
