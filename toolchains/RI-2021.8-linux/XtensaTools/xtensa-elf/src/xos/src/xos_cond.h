/** @file */

// xos_cond.h - XOS condition variables API interface and data structures.

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

#ifndef XOS_COND_H
#define XOS_COND_H

#include "xos_types.h"

#ifdef __cplusplus
extern "C" {
#endif


//-----------------------------------------------------------------------------
//
// The XosCondFunc function pointer type for condition callbacks is defined in
// xos_thread.h.
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
///
/// Condition object.
///
//-----------------------------------------------------------------------------
typedef struct XosCond {
  XosThreadQueue        queue;          ///< Queue of waiters.
  uint32_t              sig;            // Signature indicates valid object.
} XosCond;


//-----------------------------------------------------------------------------
///
///  Initialize a condition object before first use. The object must be
///  allocated by the caller.
///
///  \param     cond            Pointer to condition object.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_cond_create(XosCond * cond);


//-----------------------------------------------------------------------------
///
///  Destroy a condition object. Must have been previously created by calling
///  xos_cond_create().
///
///  \param     cond            Pointer to condition object.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_cond_delete(XosCond * cond);


//-----------------------------------------------------------------------------
///
///  Wait on a condition: block until the condition is satisfied. The condition
///  is satisfied when xos_cond_signal() or xos_cond_signal_one() is called on
///  this condition *and* the condition callback function returns non-zero.
///  If there is no callback function, then the condition is automatically satisfied.
///
///  The condition object must have been initialized before first use by
///  calling xos_cond_create().
///
///  \param     cond            Pointer to condition object.
///
///  \param     cond_fn         Pointer to a function, called by xos_cond_signal(),
///                             that should return non-zero if this thread is to
///                             be resumed. The function is invoked as:
///                             `(*cond_fn)(cond_arg, sig_value)`. May be NULL.
///
///  \param     cond_arg        Argument passed to cond_fn.
///
///  \return    Returns the value passed to xos_cond_signal(), or error code.
///
///  NOTE: Cannot be called from interrupt context.
///
//-----------------------------------------------------------------------------
int32_t
xos_cond_wait(XosCond * cond, XosCondFunc * cond_fn, void * cond_arg);


//-----------------------------------------------------------------------------
///
///  Atomically release the mutex and block until the condition is satisfied.
///  The condition is satisfied when xos_cond_signal() or xos_cond_signal_one()
///  is called on this condition. This function does not allow specifying a
///  condition callback function.
///
///  The condition and mutex objects must have been initialized before this call.
///
///  \param     cond            Pointer to condition object.
///
///  \param     mutex           Pointer to mutex object. The mutex must have
///                             been locked by the calling thread.
///
///  \return    Returns the value passed to xos_cond_signal(), or error code.
///
///  NOTE: Cannot be called from interrupt context.
///
//-----------------------------------------------------------------------------
int32_t
xos_cond_wait_mutex(XosCond * cond, struct XosMutex * mutex);


//-----------------------------------------------------------------------------
///
///  Atomically release the mutex and block until the condition is satisfied
///  or the timeout expires. When this function returns, the mutex has been
///  locked again and is owned by the calling thread.
///  The condition is satisfied when xos_cond_signal() or xos_cond_signal_one()
///  is called on this condition. This function does not allow specifying a
///  condition callback function.
///
///  The condition and mutex objects must have been initialized before this call.
///
///  \param     cond            Pointer to condition object.
///
///  \param     mutex           Pointer to mutex object. The mutex must have
///                             been locked by the calling thread.
///
///  \param     to_cycles       Timeout in cycles. Convert from time to cycles
///                             using the helper functions provided in xos_timer.
///                             A value of zero indicates no timeout.
///
///  \return    Returns the value passed to xos_cond_signal(), or error code.
///
///  NOTE: If XOS_OPT_WAIT_TIMEOUT is not enabled, then the timeout value is
///  ignored, and no timeout will occur. If the function does time out it will
///  still attempt to lock the mutex and so could wait for an indefinite period.
///
///  NOTE: Cannot be called from interrupt context.
///
//-----------------------------------------------------------------------------
int32_t
xos_cond_wait_mutex_timeout(XosCond * cond, struct XosMutex * mutex, uint64_t to_cycles);


//-----------------------------------------------------------------------------
//  INTERNAL FUNCTION: do not call directly.
//-----------------------------------------------------------------------------
int32_t
xos_cond_wakeup_(XosCond * cond, int32_t sig_value, bool single);


//-----------------------------------------------------------------------------
///
///  Trigger the condition: wake all threads waiting on the condition, if their
///  condition function evaluates to true (non-zero). If there is no condition
///  function for a thread then it is automatically awakened.
///
///  The condition object must have been initialized before first use by
///  calling xos_cond_create().
///
///  \param     cond            Pointer to condition object.
///
///  \param     sig_value       Value passed to all waiters, returned by
///                             xos_cond_wait().
///
///  \return    Returns number of woken threads on success, else error code.
///
///  NOTE: Signaling a condition that has no waiters has no effect on it, and
///  the signal is not remembered. Any thread that waits on it later must be
///  woken by another call to xos_cond_signal() or xos_cond_signal_one().
///
//-----------------------------------------------------------------------------
__attribute__((always_inline))
static inline int32_t
xos_cond_signal(XosCond * cond, int32_t sig_value)
{
    return xos_cond_wakeup_(cond, sig_value, false);
}


//-----------------------------------------------------------------------------
///
///  Trigger the condition: wake one thread waiting on the condition, if its
///  condition function evaluates to true (non-zero). If there is no condition
///  function for a thread then it is automatically awakened.
///
///  The condition object must have been initialized before first use by
///  calling xos_cond_create().
///
///  \param     cond            Pointer to condition object.
///
///  \param     sig_value       Value passed to woken thread, returned by
///                             xos_cond_wait().
///
///  \return    Returns 1 if a thread was woken, 0 if no threads were woken,
///             else error code.
///
///  NOTE: Signaling a condition that has no waiters has no effect on it, and
///  the signal is not remembered. Any thread that waits on it later must be
///  woken by another call to xos_cond_signal() or xos_cond_signal_one().
///
//-----------------------------------------------------------------------------
__attribute__((always_inline))
static inline int32_t
xos_cond_signal_one(XosCond * cond, int32_t sig_value)
{
    return xos_cond_wakeup_(cond, sig_value, true);
}


//-----------------------------------------------------------------------------
// User-mode condition support. Note these are accessible from both user and
// privileged modes.
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
///
/// XosUCond object.
///
//-----------------------------------------------------------------------------
typedef struct XosUCond {
    XosCond  cond;                          ///< Contained condition
    uint64_t key;                           ///< Unique ID for object
    uint32_t refs;                          ///< User-mode reference count
} XosUCond;


//-----------------------------------------------------------------------------
///
///  Initialize a user-mode condition object before first use. To be called
///  from privileged mode.
///
///  \param     key             A unique identifier for this condition object.
///                             If zero, a new condition will be created.
///                             If not zero, and an existing condition with a
///                             matching key is found, that condition will be
///                             returned. If no matching condition is found a
///                             new one will be created.
///
///  \param     ucond           Pointer to storage where the pointer to the
///                             created object will be returned. Must not be NULL.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_ucond_create_p(uint64_t key, XosUCond ** ucond);


//-----------------------------------------------------------------------------
///
///  Destroy a user-mode condition object. Must have been previously created
///  by calling xos_ucond_create_p(). To be called from privileged mode.
///
///  \param     ucond           Pointer to user-mode condition object.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_ucond_delete_p(XosUCond * cond);


//-----------------------------------------------------------------------------
///
///  Return the pointer to the condition object associated with the user-mode
///  condition. This pointer is to be used for all privileged-mode calls such
///  as wait/signal etc.
///
///  \param     ucond           Pointer to user-mode condition object.
///
///  \return    Returns the pointer to the condition object if valid, else NULL.
///
//-----------------------------------------------------------------------------
XosCond *
xos_ucond_cond(XosUCond * ucond);


#ifdef XOS_INCLUDE_INTERNAL
XosUCond *
xos_ucond_int_open(const char * name);

int32_t
xos_ucond_int_close(XosUCond * ucond);
#endif


#ifdef __cplusplus
}
#endif

#endif  //      XOS_COND_H

