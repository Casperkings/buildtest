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
#ifndef __XIPC_BARRIER_H__
#define __XIPC_BARRIER_H__

/*! @file */

#ifdef __cplusplus
extern "C" {
#endif

#include "xipc_common.h"

/*! Minimum size of a barrier object in bytes. */
#define XIPC_BARRIER_STRUCT_SIZE     (32)

/*! Minimum size of a synchronization object in bytes. */
#define XIPC_RESET_SYNC_STRUCT_SIZE  (4)

#ifndef __XIPC_BARRIER_INTERNAL_H__
#ifdef XIPC_CUSTOM_SUBSYSTEM
#include "xmp_custom_subsystem.h"
#else
#include <xtensa/system/xmp_subsystem.h>
#endif
struct xipc_barrier_struct {
  char _[((XIPC_BARRIER_STRUCT_SIZE+XMP_MAX_DCACHE_LINESIZE-1)/XMP_MAX_DCACHE_LINESIZE)*XMP_MAX_DCACHE_LINESIZE];
} __attribute__((aligned(XMP_MAX_DCACHE_LINESIZE)));

struct xipc_reset_sync_struct {
  char _[((XIPC_RESET_SYNC_STRUCT_SIZE+XMP_MAX_DCACHE_LINESIZE-1)/XMP_MAX_DCACHE_LINESIZE)*XMP_MAX_DCACHE_LINESIZE];
} __attribute__((aligned(XMP_MAX_DCACHE_LINESIZE)));
#endif

/*! Barrier object type. The minimum size of an object of this type is 
 * \ref XIPC_BARRIER_STRUCT_SIZE. For a cached subsystem, the size is rounded 
 * and aligned to the maximum dcache line size across all cores in the 
 * subsystem. */
typedef struct xipc_barrier_struct xipc_barrier_t;

/*! Synchronization object type. The minimum size of an object of this type is 
 * \ref XIPC_RESET_SYNC_STRUCT_SIZE. For a cached subsystem, the size is 
 * rounded and aligned to the maximum dcache line size across all cores in the 
 * subsystem. */
typedef struct xipc_reset_sync_struct xipc_reset_sync_t;

/*!
 * This function initializes the barrier object. The barrier needs to be 
 * initialized prior to its use by one of the cores. The \a wait_kind 
 * parameter determines if the cores that wait at the barrier \a spin-wait or 
 * \a sleep-wait.  A subset of cores within the MP-subsystem can participate 
 * in the barrier. The processor IDs of these cores is specified in the 
 * \a proc_ids parameter. The number of processors cannot exceed the total 
 * number of cores as specified in the subsystem. The \a num_waiters allows
 * for more than one thread per core to wait at a barrier.
 *
 * \param barrier     Pointer to the xipc_barrier object.
 * \param wait_kind   Spin wait or sleep when waiting for other cores at 
 *                    the barrier.
 * \param num_procs   Number of cores participating in this barrier.
 * \param proc_ids    Processor IDs of these participating cores.
 * \param num_waiters Number of waiters at this barrier. Has to be >= 
 *                    \a num_procs.
 * \return            XIPC_OK if successful else returns one of 
 *                    - XIPC_ERROR_INVALID_ARG 
 *                    - XIPC_ERROR_INTERNAL
 *                    - XIPC_ERROR_SUBSYSTEM_UNSUPPORTED
 */
extern xipc_status_t
xipc_barrier_init(xipc_barrier_t  *barrier,
                  xipc_wait_kind_t wait_kind,
                  uint8_t          num_procs,
                  xipc_pid_t      *proc_ids,
                  uint8_t          num_waiters);

/*! 
 * Wait at the barrier until all cores reach the barrier. The core would 
 * either \a spin-wait or do a \a sleep-wait based on how the barrier was 
 * initialized. If sleep-waiting, the last core to reach the barrier triggers 
 * the \a XIPC-interrupt to unblock all other cores.
 * 
 * \param barrier Pointer to barrier object.
 */
extern void xipc_barrier_wait(xipc_barrier_t *barrier);

/*!
 * This is a special barrier call that allows for all cores in the subsystem 
 * to synchronize using the input \a sync structure. 
 * Unlike the \ref xipc_barrier_t 
 * APIs, no initialization is required prior to its use. This function is 
 * typically used for synchronization at bootup/reset time. This allows each 
 * core to perform initial setup like enabling interrupts, setting interrupt 
 * handlers, initializing shared data including the XIPC primitives like 
 * barriers, mutexes etc. The core then invokes \a xipc_reset_sync to 
 * synchronize with all other cores before proceeding further. Note, the 
 * \ref xipc_initialize API internally calls \a xipc_reset_sync to synchronize 
 * after performing some libxipc specific initialization. One limitation of 
 * this API is that it can be used only once in the program for a given input 
 * sync structure.
 *
 * \param sync Array of \ref xipc_reset_sync_t structures, one per core for 
 *             initial synchronization. Note, each \ref xipc_reset_sync_t 
 *             element in the array is assumed to be of maximum dache line 
 *             size across all processors in the subsystem.
 * \return     XIPC_OK if successful, else returns XIPC_ERROR_INTERNAL
 */
extern xipc_status_t xipc_reset_sync(volatile xipc_reset_sync_t *sync);

#ifdef __cplusplus
}
#endif

#endif /* __XIPC_BARRIER_H__ */
