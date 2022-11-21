/** @file */

// xos_mutex.h - XOS Mutex API interface and data structures.

// Copyright (c) 2015-2020 Cadence Design Systems, Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// NOTE: Do not include this file directly in your application. Including
// xos.h will automatically include this file.

#ifndef XOS_MUTEX_H
#define XOS_MUTEX_H

#include "xos_types.h"

#ifdef __cplusplus
extern "C" {
#endif


//-----------------------------------------------------------------------------
// Mutex flags.
//-----------------------------------------------------------------------------
#define XOS_MUTEX_WAIT_PRIORITY 0x0000U     ///< Wake waiters in priority order (default)
#define XOS_MUTEX_WAIT_FIFO     0x0001U     ///< Wake waiters in FIFO order
#define XOS_MUTEX_PRIO_PROTECT  0x0004U     // Use priority ceiling
#define XOS_MUTEX_PRIO_INHERIT  0x0008U     // Use priority inheritance


//-----------------------------------------------------------------------------
///
///  XosMutex object.
///
//-----------------------------------------------------------------------------
typedef struct XosMutex {
    XosThread * XOS_ATOMIC  owner;          ///< Owning thread (null if unlocked).
    XosThreadQueue          waitq;          ///< Queue of waiters.
    uint32_t                flags;          ///< Properties.
    int32_t                 lock_count;     ///< For recursive locking.
#if XOS_OPT_MUTEX_PRIORITY
    struct XosMutex *       next;           // Next mutex in owned list.
    int8_t                  priority;       // Priority ceiling for mutex.
    int8_t                  curr_pri;       // Current priority of mutex.
    uint16_t                reserved1;
#endif
    uint32_t                sig;            // Valid signature indicates inited.
} XosMutex;


//-----------------------------------------------------------------------------
///
///  Initialize a mutex object before first use.
///
///  \param     mutex           Pointer to mutex object.
///
///  \param     flags           Creation flags:
///                             - XOS_MUTEX_WAIT_FIFO -- Queue waiting threads
///                               in fifo order.
///                             - XOS_MUTEX_WAIT_PRIORITY -- Queue waiting threads
///                               by priority. This is the default.
///
///  \param     priority        Mutex's priority ceiling. This is used only if
///                             option XOS_OPT_MUTEX_PRIORITY is enabled. Legal
///                             values are from 0 .. (XOS_NUM_PRIORITY - 1).
///                             A value of zero disables priority ceiling.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_mutex_create(XosMutex * mutex, uint32_t flags, int8_t priority);


//-----------------------------------------------------------------------------
///
///  Destroy a mutex object. Must have been previously initialized by calling
///  xos_mutex_create().
///
///  \param     mutex           Pointer to mutex object.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_mutex_delete(XosMutex * mutex);


//-----------------------------------------------------------------------------
///
///  Take ownership of the mutex: block until the mutex is owned. The mutex
///  must have been initialized. Cannot be called from interrupt context.
///
///  \param     mutex           Pointer to mutex object.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_mutex_lock(XosMutex * mutex);


//-----------------------------------------------------------------------------
///
///  Take ownership of the mutex: block until the mutex is owned or the timeout
///  expires. The mutex must have been initialized. Cannot be called from 
///  interrupt context.
///
///  \param     mutex           Pointer to mutex object.
///
///  \param     to_cycles       Timeout in cycles. Convert from time to cycles
///                             using the helper functions provided in xos_timer.
///                             A value of zero indicates no timeout.
///
///  \return    Returns XOS_OK on success, XOS_ERR_TIMEOUT on timeout, else error code.
///
///  NOTE: If XOS_OPT_WAIT_TIMEOUT is not enabled, then the timeout value is
///  ignored, and no timeout will occur.
///
///  IMPORTANT: This function does not implement priority inheritance. The
///  combination of a timed wait and priority elevation can lead to scenarios
///  that are difficult to analyze and verify.
///
//-----------------------------------------------------------------------------
int32_t
xos_mutex_lock_timeout(XosMutex * mutex, uint64_t to_cycles);


//-----------------------------------------------------------------------------
///
///  Release ownership of the mutex. The mutex must have been initialized and
///  must be owned by the calling thread. Cannot be called from interrupt
///  context.
///
///  \param     mutex           Pointer to mutex object.
///
///  \return    Returns XOS_OK on success, else error code.   
///
//-----------------------------------------------------------------------------
int32_t
xos_mutex_unlock(XosMutex * mutex);


//-----------------------------------------------------------------------------
///
///  Try to take ownership of the mutex, but do not block if the mutex is taken.
///  Return immediately. The mutex must have been initialized. Cannot be called
///  from interrupt context.
///
///  \param     mutex           Pointer to mutex object.
///
///  \return    Returns XOS_OK on success (mutex owned), else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_mutex_trylock(XosMutex * mutex);


//-----------------------------------------------------------------------------
///
///  Return the state of the mutex (locked or unlocked) but do not attempt to
///  take ownership. The mutex must have been initialized.
///
///  \param     mutex           Pointer to mutex object.
///
///  \return    Returns 0 if the mutex is unlocked, 1 if it is locked, -1 on error.
///
//-----------------------------------------------------------------------------
int32_t
xos_mutex_test(const XosMutex * mutex);


//-----------------------------------------------------------------------------
// User-mode mutex support. Note these are accessible from both user and 
// privileged modes.
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
///
/// XosUMutex object.
///
//-----------------------------------------------------------------------------
typedef struct XosUMutex {
    XosMutex mtx;                           ///< Contained mutex object
    uint64_t key;                           ///< Unique ID for mutex
    uint32_t refs;                          ///< User-mode reference count
} XosUMutex;


//-----------------------------------------------------------------------------
///
///  Initialize a user-mode mutex object before first use. To be called from
///  privileged mode.
///
///  \param     key             A unique identifier for this mutex object.
///                             If zero, a new mutex will be created. If not
///                             zero, and an existing mutex with a matching
///                             key is found, that mutex will be returned.
///                             If no matching mutex is found, a new one will
///                             be created.
///
///  \param     priority        Mutex's priority ceiling. Legal values are from
///                             0 .. (XOS_NUM_PRIORITY - 1). A value of zero
///                             disables priority ceiling.
///
///  \param     umtx            Pointer to storage where the pointer to the
///                             created object will be returned. Must not be NULL.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_umutex_create_p(uint64_t key, int8_t priority, XosUMutex ** umtx);


//-----------------------------------------------------------------------------
///
///  Destroy a user-mode mutex object. Must have been previously created by
///  calling xos_umutex_create_p(). To be called from privileged mode.
///
///  \param     umtx            Pointer to user-mode mutex object.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_umutex_delete_p(XosUMutex * umtx);


//-----------------------------------------------------------------------------
///
///  Return the pointer to the mutex object associated with the user-mode
///  mutex. This pointer is to be used for all privileged-mode mutex calls
///  such as get/put etc.
///
///  \param     umtx            Pointer to user-mode mutex object.
///
///  \return    Returns the pointer to the mutex object if valid, else NULL.
///
//-----------------------------------------------------------------------------
XosMutex *
xos_umutex_mutex(XosUMutex * umtx);


#ifdef XOS_INCLUDE_INTERNAL
XosMutex *
xos_mutexq_to_mutex(XosThreadQueue * q);
#endif


#ifdef __cplusplus
}
#endif

#endif  //      XOS_MUTEX_H

