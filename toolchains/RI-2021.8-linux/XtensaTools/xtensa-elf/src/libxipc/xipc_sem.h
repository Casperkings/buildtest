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
#ifndef __XIPC_SEM_H__
#define __XIPC_SEM_H__

/*! @file */

#ifdef __cplusplus
extern "C" {
#endif

#include "xipc_common.h"

/*! Minimum size of a semaphore object in bytes. */
#define XIPC_SEM_STRUCT_SIZE     (84)

#ifndef __XIPC_SEM_INTERNAL_H__
#ifdef XIPC_CUSTOM_SUBSYSTEM
#include "xmp_custom_subsystem.h"
#else
#include <xtensa/system/xmp_subsystem.h>
#endif
struct xipc_sem_struct {
  char _[((XIPC_SEM_STRUCT_SIZE+XMP_MAX_DCACHE_LINESIZE-1)/XMP_MAX_DCACHE_LINESIZE)*XMP_MAX_DCACHE_LINESIZE];
}  __attribute__((aligned(XMP_MAX_DCACHE_LINESIZE)));
#endif

/*! Semaphore object type. The minimum size of an object of this type is 
 * \ref XIPC_SEM_STRUCT_SIZE. For a cached subsystem, the size is rounded 
 * and aligned to the maximum dcache line size across all cores in the 
 * subsystem. */
typedef struct xipc_sem_struct xipc_sem_t;

/*!
 * Initialize semaphore object. The semaphore needs to be initialized pior
 * to use.
 *
 * \param sem       Pointer to semaphore object.
 * \param count     Initial semaphore count.
 * \param wait_kind Spin wait or sleep when waiting on the semaphore.
 * \return          XIPC_OK if successful, else returns one of
 *                 - XIPC_ERROR_INTERNAL
 *                 - XIPC_ERROR_INVALID_ARG
 *                 - XIPC_ERROR_SUBSYSTEM_UNSUPPORTED
 */
extern xipc_status_t 
xipc_sem_init(xipc_sem_t      *sem, 
              uint32_t         count,
              xipc_wait_kind_t wait_kind);

/*!
 * This function releases the semaphore that was acquired using a prior call to 
 * \ref xipc_sem_wait. If there are other blocked cores/threads waiting to 
 * acquire the semaphore, this unblocks one of the waiters.
 *
 * \param sem Pointer to semaphore object.
 */
extern void xipc_sem_post(xipc_sem_t *sem);

/*! 
 * This function acquires the semaphore object. If the semaphore count is 0, 
 * the core that does the acquire either \a spin-waits or \a sleep-waits until
 * the count is positive. If blocked, the core that releases the semaphore 
 * triggers the \a XIPC-interrupt to unblock this core. The maximum allowable
 * waiters at a semaphore is 16.
 *
 * \param sem Pointer to semaphore object.
 * \return    XIPC_OK if successful, else returns XIPC_ERROR_SEM_MAX_WAITERS.
 */
extern xipc_status_t xipc_sem_wait(xipc_sem_t *sem);

/*!
 * This function attempts to acquire the semaphore and returns an error code
 * if it cannot. On success, the semaphore is locked. Unlike \ref xipc_sem_wait,
 * this does not block.
 * 
 * \param sem Pointer to semaphore object.
 * \return    XIPC_OK if successful, else returns XIPC_ERROR_SEM_LOCKED 
 *            if semaphore count is 0.
 */
extern xipc_status_t xipc_sem_try_wait(xipc_sem_t *sem);

#ifdef __cplusplus
}
#endif

#endif /* __XIPC_SEM_H__ */
