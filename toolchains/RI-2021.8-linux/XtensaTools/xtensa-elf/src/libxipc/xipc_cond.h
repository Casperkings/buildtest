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
#ifndef __XIPC_COND_H__
#define __XIPC_COND_H__

/*! @file */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __XIPC_COND_INTERNAL_H__
#include "xipc_mutex.h"
#endif
#include "xipc_common.h"

/*! Minimum size of a condition variable object in bytes. */
#define XIPC_COND_STRUCT_SIZE     (76)

#ifndef __XIPC_COND_INTERNAL_H__
#ifdef XIPC_CUSTOM_SUBSYSTEM
#include "xmp_custom_subsystem.h"
#else
#include <xtensa/system/xmp_subsystem.h>
#endif
struct xipc_cond_struct {
  char _[((XIPC_COND_STRUCT_SIZE+XMP_MAX_DCACHE_LINESIZE-1)/XMP_MAX_DCACHE_LINESIZE)*XMP_MAX_DCACHE_LINESIZE];
}  __attribute__ ((aligned (XMP_MAX_DCACHE_LINESIZE)));
#endif

/*! Condition variable object type. The minimum size of an 
 * object of this type is \ref XIPC_COND_STRUCT_SIZE. For a cached 
 * subsystem, the size is rounded and aligned to the maximum dcache line size 
 * across all cores in the subsystem. */
typedef struct xipc_cond_struct xipc_cond_t;

/*! 
 * Initialize the conditional variable. Needs to be called prior to use.
 *
 * \param cond Pointer to condition variable.
 * \param wait_kind Spin wait or sleep when waiting on the condition.
 * \return     XIPC_OK if successful, else returns one of
 *             - XIPC_ERROR_INTERNAL
 *             - XIPC_ERROR_SUBSYSTEM_UNSUPPORTED
 *             - XIPC_ERROR_INVALID_ARG
 */
extern xipc_status_t 
xipc_cond_init(xipc_cond_t *cond, xipc_wait_kind_t wait_kind);

/*!
 * This function is used to sleep-wait on a condition. Prior to calling this 
 * function, the mutex that is passed as a parameter needs to be acquired by 
 * the caller. This function releases the mutex before blocking. When 
 * signaled, the blocked thread/core reacquires the mutex on wake-up.
 * The maximum allowable waiters at a mutex is 16.
 *
 * \param cond  Pointer to condition variable.
 * \param mutex Pointer to mutex that is expected to be locked.
 * \return      XIPC_OK if successful, else returns XIPC_ERROR_MUTEX_NOT_OWNER
 *              if the mutex is not owned by this processor.
 */
extern xipc_status_t 
xipc_cond_wait(xipc_cond_t *cond, xipc_mutex_t *mutex);

/*! 
 * This function is used to signal one of the cores waiting on the condition.
 * The caller has to lock the mutex that is used by the \ref xipc_cond_wait 
 * function prior to this call.
 *
 * \param cond Pointer to condition variable.
 */
extern void 
xipc_cond_signal(xipc_cond_t *cond);

/*! 
 * This function is used to signal all the cores waiting on the condition.
 * The caller has to lock the mutex used by the \ref xipc_cond_wait function 
 * prior to this call.
 *
 * \param cond Pointer to condition variable.
 */
extern void 
xipc_cond_broadcast(xipc_cond_t *cond);

#ifdef __cplusplus
}
#endif

#endif /* __XIPC_COND_H__ */
