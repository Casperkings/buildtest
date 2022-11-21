/** @file */

// xos_semaphore.h - XOS Semaphore API interface and data structures.

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

#ifndef XOS_SEMAPHORE_H
#define XOS_SEMAPHORE_H

#include "xos_types.h"

#ifdef __cplusplus
extern "C" {
#endif


//-----------------------------------------------------------------------------
// Semaphore flags.
//-----------------------------------------------------------------------------
#define XOS_SEM_WAIT_PRIORITY       0x0000U ///< Wake waiters in priority order (default)
#define XOS_SEM_WAIT_FIFO           0x0001U ///< Wake waiters in FIFO order


//-----------------------------------------------------------------------------
///
/// XosSem object.
///
//-----------------------------------------------------------------------------
typedef struct XosSem {
  int32_t XOS_ATOMIC    count;          ///< Current count
  XosThreadQueue        waitq;          ///< Queue of waiters.
  uint32_t              flags;          ///< Properties.
  uint32_t              sig;            // Valid signature indicates inited.
} XosSem;


//-----------------------------------------------------------------------------
///
///  Initialize a semaphore object before first use.
///
///  \param     sem             Pointer to semaphore object.
///
///  \param     flags           Creation flags:
///                             - XOS_SEM_WAIT_FIFO -- queue waiting threads in
///                               fifo order.
///                             - XOS_SEM_WAIT_PRIORITY -- queue waiting threads
///                               by priority. This is the default.
///
///  \param     initial_count   Initial count for semaphore on creation. Must be
///                             >= 0.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_sem_create(XosSem * sem, uint32_t flags, int32_t initial_count);


//-----------------------------------------------------------------------------
///
///  Destroy a semaphore object. Must have been previously created by calling
///  xos_sem_create().
///
///  \param     sem             Pointer to semaphore object.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_sem_delete(XosSem * sem);


//-----------------------------------------------------------------------------
///
///  Decrement the semaphore count: block until the decrement is possible.
///  The semaphore must have been initialized. Cannot be called from interrupt
///  context.
///
///  \param     sem             Pointer to semaphore object.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_sem_get(XosSem * sem);


//-----------------------------------------------------------------------------
///
///  Decrement the semaphore count: block until the decrement is possible or
///  the timeout expires. The semaphore must have been initialized. Cannot be
///  called from interrupt context.
///
///  \param     sem             Pointer to semaphore object.
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
//-----------------------------------------------------------------------------
int32_t
xos_sem_get_timeout(XosSem * sem, uint64_t to_cycles);


//-----------------------------------------------------------------------------
///
///  Increment the semaphore count. The semaphore must have been initialized.
///  Remember that this action may wake up a waiting thread, and if that thread
///  is higher priority then there will be an immediate context switch.
///
///  \param     sem             Pointer to semaphore object.
///
///  \return    Returns XOS_OK on success, else error code.   
///
//-----------------------------------------------------------------------------
int32_t
xos_sem_put(XosSem * sem);


//-----------------------------------------------------------------------------
///
///  Increment the semaphore count only if the specified maximum count will not
///  be exceeded. The semaphore must have been initialized. This action could
///  wake up a waiting thread, and if that thread is higher priority there will
///  be an immediate context switch.
///
///  \param     sem             Pointer to semaphore object.
///
///  \param     max             Maximum count of semaphore to enforce. Must be
///                             > 0.
///
///  \return    Returns XOS_OK on success, XOS_ERR_LIMIT if the limit would be
///             exceeded, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_sem_put_max(XosSem * sem, int32_t max);


//-----------------------------------------------------------------------------
///
///  Try to decrement the semaphore, but do not block if the semaphore count is
///  zero. Return immediately. The semaphore must have been initialized.
///
///  \param     sem             Pointer to semaphore object.
///
///  \return    Returns XOS_OK on success (semaphore decremented), else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_sem_tryget(XosSem * sem);


//-----------------------------------------------------------------------------
///
///  Return the count of the semaphore but do not attempt to decrement it.
///  The semaphore must have been initialized.
///
///  \param     sem             Pointer to semaphore object.
///
///  \return    Returns semaphore count, (uint32_t) -1 on error.
///
//-----------------------------------------------------------------------------
int32_t
xos_sem_test(const XosSem * sem);


//-----------------------------------------------------------------------------
// User-mode semaphore support. Note these are accessible from both user and
// privileged modes.
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
///
/// XosUSem object.
///
//-----------------------------------------------------------------------------
typedef struct XosUSem {
    XosSem   sem;                           ///< Contained semaphore object
    uint64_t key;                           ///< Unique ID for semaphore
    uint32_t refs;                          ///< (User-mode) reference count
} XosUSem;


//-----------------------------------------------------------------------------
///
///  Initialize a user-mode semaphore object before first use. To be called
///  from privileged mode.
///
///  \param     key             A unique identifier for this semaphore object.
///                             If zero, a new semaphore will be created. If
///                             not zero, and an existing semaphore with a
///                             matching key is found, that semaphore will be
///                             returned. If no matching semaphore is found,
///                             a new one will be created.
///
///  \param     initial_count   Initial count for semaphore on creation. Must be
///                             >= 0.
///
///  \param     usem            Pointer to storage where the pointer to the 
///                             created object will be returned. Must not be NULL.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_usem_create_p(uint64_t key, int32_t initial_count, XosUSem ** usem);


//-----------------------------------------------------------------------------
///
///  Destroy a user-mode semaphore object. Must have been previously created by
///  calling xos_usem_create_p(). To be called from privileged mode.
///
///  \param     usem            Pointer to user-mode semaphore object.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_usem_delete_p(XosUSem * usem);


//-----------------------------------------------------------------------------
///
///  Return the pointer to the semaphore object associated with the user-mode
///  semaphore. This pointer is to be used for all privileged-mode semaphore
///  calls such as get/put etc.
///
///  \param     usem            Pointer to user-mode semaphore object.
///
///  \return    Returns the pointer to the semaphore object if valid, or NULL.
///
//-----------------------------------------------------------------------------
XosSem *
xos_usem_sem(XosUSem * usem);


#ifdef XOS_INCLUDE_INTERNAL
XosSem *
xos_semq_to_sem(XosThreadQueue * q);
#endif


#ifdef __cplusplus
}
#endif

#endif  //      XOS_SEMAPHORE_H

