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
#ifndef __XIPC_MUTEX_H__
#define __XIPC_MUTEX_H__

/*! @file */

#ifdef __cplusplus
extern "C" {
#endif

#include "xipc_common.h"

/*! Minimum size of a mutex object in bytes. */
#define XIPC_MUTEX_STRUCT_SIZE     (84)

#ifndef __XIPC_MUTEX_INTERNAL_H__
#ifdef XIPC_CUSTOM_SUBSYSTEM
#include "xmp_custom_subsystem.h"
#else
#include <xtensa/system/xmp_subsystem.h>
#endif
struct xipc_mutex_struct {
  char _[((XIPC_MUTEX_STRUCT_SIZE+XMP_MAX_DCACHE_LINESIZE-1)/XMP_MAX_DCACHE_LINESIZE)*XMP_MAX_DCACHE_LINESIZE];
}  __attribute__((aligned(XMP_MAX_DCACHE_LINESIZE)));
#endif

/*! Mutex object type. The minimum size of an object of this type is 
 * \ref XIPC_MUTEX_STRUCT_SIZE. For a cached subsystem, the size is rounded 
 * and aligned to the maximum dcache line size across all cores in the 
 * subsystem. */
typedef struct xipc_mutex_struct xipc_mutex_t;

/*!
 * This function is used to initialize the mutex. The mutex object needs to 
 * be initialized prior to use. 
 *
 * \param mutex     Pointer to mutex object
 * \param wait_kind Spin wait or sleep when waiting on the mutex.
 * \return          XIPC_OK if successful, else returns one of
 *                 - XIPC_ERROR_INTERNAL
 *                 - XIPC_ERROR_INVALID_ARG
 *                 - XIPC_ERROR_SUBSYSTEM_UNSUPPORTED
 */
extern xipc_status_t 
xipc_mutex_init(xipc_mutex_t    *mutex,
                xipc_wait_kind_t wait_kind);

/*!
 * This function is used to acquire the mutex. If the mutex is not available, 
 * the core that does the acquire either \a spin-waits or \a sleep-waits until 
 * the mutex is released. If blocked, the core that releases the mutex 
 * triggers the \a XIPC-interrupt to unblock this core. Note, a core 
 * cannot reacquire a mutex that has already been acquired by the same core 
 * prior to a release. The maximum allowable waiters at a mutex is 16.
 * 
 * \param mutex Pointer to mutex object.
 * \return      XIPC_OK if successful, else returns 
 *              XIPC_ERROR_MUTEX_MAX_WAITERS.
 */
extern xipc_status_t xipc_mutex_acquire(xipc_mutex_t *mutex);

/*!
 * This function releases the mutex that was acquired using a prior call to 
 * \ref xipc_mutex_acquire. If there are other blocked cores waiting to acquire 
 * the mutex, the release unblocks one of the waiters. The mutex needs to be 
 * released by the core that originally acquired it.
 *
 * \param mutex Pointer to mutex object.
 * \return      XIPC_OK, if successful, else returns XIPC_ERROR_MUTEX_NOT_OWNER
 *              if attempting to release the mutex that was not acquired by 
 *              this core.
 */
extern xipc_status_t xipc_mutex_release(xipc_mutex_t *mutex);

/*!
 * This function attempts to acquire the mutex and returns an error code if it 
 * cannot. On success, the mutex is acquired. Unlike \ref xipc_mutex_acquire, 
 * this does not block.
 * 
 * \param mutex Pointer to mutex object.
 * \return      XIPC_OK if successful, else returns XIPC_ERROR_MUTEX_ACQUIRED 
 *              if mutex is already acquired.
 */
extern xipc_status_t xipc_mutex_try_acquire(xipc_mutex_t *mutex);

#ifdef __cplusplus
}
#endif

#endif /* __XIPC_MUTEX_H__ */
