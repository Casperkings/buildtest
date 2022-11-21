/** @file */

// xos_syscall.h - XOS syscall interface for user-mode threads.

// Copyright (c) 2015-2021 Cadence Design Systems, Inc.
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


#ifndef XOS_SYSCALL_H
#define XOS_SYSCALL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <xtensa/tie/xt_exceptions.h>


//-----------------------------------------------------------------------------
// XOS syscall request identifiers.
//-----------------------------------------------------------------------------

#define XOS_SC_FIRST    0x0F000000

typedef enum xos_scid {
    XOS_SC_THREAD_CREATE = XOS_SC_FIRST,
    XOS_SC_THREAD_DELETE,
    XOS_SC_THREAD_JOIN,
    XOS_SC_THREAD_EXIT,
    XOS_SC_THREAD_YIELD,
    XOS_SC_THREAD_ID,
    XOS_SC_THREAD_SLEEP,
    XOS_SC_THREAD_SLEEP_MSEC,
    XOS_SC_THREAD_SLEEP_USEC,

    XOS_SC_SEM_CREATE = XOS_SC_FIRST + 0x100,
    XOS_SC_SEM_DELETE,
    XOS_SC_SEM_GET,
    XOS_SC_SEM_GET_TIMEOUT,
    XOS_SC_SEM_PUT,
    XOS_SC_SEM_PUT_MAX,

    XOS_SC_MTX_CREATE = XOS_SC_FIRST + 0x200,
    XOS_SC_MTX_DELETE,
    XOS_SC_MTX_LOCK,
    XOS_SC_MTX_LOCK_TIMEOUT,
    XOS_SC_MTX_UNLOCK,

    XOS_SC_COND_CREATE = XOS_SC_FIRST + 0x300,
    XOS_SC_COND_DELETE,
    XOS_SC_COND_WAIT,
    XOS_SC_COND_WAIT_MTX,
    XOS_SC_COND_WAIT_MTX_TIMEOUT,
    XOS_SC_COND_SIGNAL,
    XOS_SC_COND_SIGNAL_ONE,
} xos_scid;


//-----------------------------------------------------------------------------
// Register name aliases for syscall inline assembly code. SC_ID is the syscall
// request ID. RETVAL is the return value if any. ARG1/4 are the arguments, if
// any. XEA3 uses a8-a12 because these registers are always available in the
// exception frame (do not require a window spill).
//-----------------------------------------------------------------------------

#if XCHAL_HAVE_XEA2
#define SC_ID       "a2"
#define RETVAL      "a2"
#define ARG1        "a3"
#define ARG2        "a4"
#define ARG3        "a5"
#define ARG4        "a6"
#endif

#if XCHAL_HAVE_XEA3
#define SC_ID       "a8"
#define RETVAL      "a8"
#define ARG1        "a9"
#define ARG2        "a10"
#define ARG3        "a11"
#define ARG4        "a12"
#endif


//-----------------------------------------------------------------------------
// Execute syscall with no arguments.
//-----------------------------------------------------------------------------
static inline int32_t
xos_do_syscall_noargs(xos_scid scid)
{
    register uint32_t a2  __asm__ (SC_ID) = (uint32_t) scid;
    register int32_t  ret __asm__ (RETVAL);

    __asm__ volatile ("syscall"
             : "=a" (ret)
             : "a" (a2));

    return ret;
}


//-----------------------------------------------------------------------------
// Execute syscall with arguments. The current limit is 4 argument values.
//-----------------------------------------------------------------------------
static inline int32_t
xos_do_syscall(xos_scid scid,
               uint32_t arg1,
               uint32_t arg2,
               uint32_t arg3,
               uint32_t arg4)
{
    register uint32_t a2  __asm__ (SC_ID) = (uint32_t) scid;
    register uint32_t a3  __asm__ (ARG1)  = arg1;
    register uint32_t a4  __asm__ (ARG2)  = arg2;
    register uint32_t a5  __asm__ (ARG3)  = arg3;
    register uint32_t a6  __asm__ (ARG4)  = arg4;
    register int32_t  ret __asm__ (RETVAL);

    __asm__ volatile ("syscall"
             : "=a" (ret)
             : "a" (a2), "a" (a3), "a" (a4), "a" (a5), "a" (a6));

    return ret;
}


//-----------------------------------------------------------------------------
// Defines and types used by syscalls.
//-----------------------------------------------------------------------------
typedef uint64_t xos_key;               ///< Unique object ID
#define XOS_UM_KEY_ZERO     0ULL        ///< "Zero" object ID


//-----------------------------------------------------------------------------
// Thread syscalls.
//-----------------------------------------------------------------------------

typedef uint32_t xos_uthread;           ///< User-mode thread handle type

//-----------------------------------------------------------------------------
///
///  User-mode thread creation parameters.
///
//-----------------------------------------------------------------------------
typedef struct xos_uthread_params {
    void *       entry;                 ///< Thread entry point function
    void *       arg;                   ///< Entry point function argument
    char *       name;                  ///< Thread name
    void *       stack;                 ///< Thread stack base address
    uint32_t     stack_size;            ///< Thread stack size in bytes
    int8_t       priority;              ///< Thread priority
    uint32_t     flags;                 ///< Thread flags
} xos_uthread_params;

//-----------------------------------------------------------------------------
///
///  Create a new user-mode thread.
///
///  \param     params          Pointer to create params structure.
///
///  \param     pthd            Pointer to storage for thread handle.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
static inline int32_t
xos_uthread_create(xos_uthread_params * params, xos_uthread * pthd)
{
    return xos_do_syscall(XOS_SC_THREAD_CREATE,
                          (uint32_t) params,
                          (uint32_t) pthd,
                          0,
                          0);
}

//-----------------------------------------------------------------------------
///
///  Destroy a user-mode thread, and free up its kernel resources.
///
///  \param     thd             Thread handle.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
static inline int32_t
xos_uthread_delete(xos_uthread thd)
{
    return xos_do_syscall(XOS_SC_THREAD_DELETE,
                          (uint32_t) thd,
                          0,
                          0,
                          0);
}

//-----------------------------------------------------------------------------
///
///  Wait until the specified thread exits and get its exit code. If the thread
///  has exited already an error will be returned.
///
///  \param     thd             Handle of thread to wait for. Cannot be "self".
///
///  \param     exitcode        Pointer to storage for the exit code.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
static inline int32_t
xos_uthread_join(xos_uthread thd, int32_t * exitcode)
{
    return xos_do_syscall(XOS_SC_THREAD_JOIN,
                          (uint32_t) thd,
                          (uint32_t) exitcode,
                          0,
                          0);
}

//-----------------------------------------------------------------------------
///
///  Exit the current thread.
///
///  \param     exitcode        Exit code to be returned to any waiting thread.
///
///  \return    This function does not return.
///
///  NOTE: Thread exit is automatically called if the thread returns from its
///  entry function. The entry function's return value will be passed along as
///  the exit code.
///
//-----------------------------------------------------------------------------
static inline void
xos_uthread_exit(int32_t exitcode)
{
    (void) xos_do_syscall(XOS_SC_THREAD_EXIT,
                          (uint32_t) exitcode,
                          0,
                          0,
                          0);
}

//-----------------------------------------------------------------------------
///
///  Yield the CPU to the next thread in line. The calling thread remains ready
///  and is placed at the tail of the ready queue at its current priority level.
///  If there are no threads at the same priority level that are ready to run,
///  then this call will return immediately.
///
///  \return    Returns nothing.
///
//-----------------------------------------------------------------------------
static inline void
xos_uthread_yield(void)
{
    (void) xos_do_syscall_noargs(XOS_SC_THREAD_YIELD);
}

//-----------------------------------------------------------------------------
///
///  Return the ID (handle) of the current thread.
///
///  \return    Returns the handle of the current thread. This handle can be
///             used in all XOS system calls.
///
//-----------------------------------------------------------------------------
static inline xos_uthread
xos_uthread_id(void)
{
    return (xos_uthread) xos_do_syscall_noargs(XOS_SC_THREAD_ID);
}

//-----------------------------------------------------------------------------
///
///  Put calling thread to sleep for at least the specified number of cycles.
///  The actual number of cycles spent sleeping may be larger depending upon
///  the granularity of the system timer. Once the specified time has elapsed
///  the thread will be woken and made ready.
///
///  \param     cycles          Number of system clock cycles to sleep.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
static inline int32_t
xos_uthread_sleep(uint64_t cycles)
{
    uint32_t lc = (uint32_t)(cycles & 0xFFFFFFFF);
    uint32_t hc = (uint32_t)(cycles >> 32);

    return xos_do_syscall(XOS_SC_THREAD_SLEEP,
                          hc,
                          lc,
                          0,
                          0);
}

//-----------------------------------------------------------------------------
///
///  Put calling thread to sleep for at least the specified number of msec.
///  The actual amount of time spent sleeping may be larger depending upon
///  the granularity of the system timer. Once the specified time has elapsed
///  the thread will be woken and made ready.
///
///  \param     msecs           The number of milliseconds to sleep.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
static inline int32_t
xos_uthread_sleep_msec(uint64_t msecs)
{
    uint32_t lc = (uint32_t)(msecs & 0xFFFFFFFF);
    uint32_t hc = (uint32_t)(msecs >> 32);

    return xos_do_syscall(XOS_SC_THREAD_SLEEP_MSEC,
                          hc,
                          lc,
                          0,
                          0);
}

//-----------------------------------------------------------------------------
///
///  Put calling thread to sleep for at least the specified number of usec.
///  The actual amount of time spent sleeping may be larger depending upon
///  the granularity of the system timer. Once the specified time has elapsed
///  the thread will be woken and made ready.
///
///  \return    usecs           The number of microseconds to sleep.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
static inline int32_t
xos_uthread_sleep_usec(uint64_t usecs)
{
    uint32_t lc = (uint32_t)(usecs & 0xFFFFFFFF);
    uint32_t hc = (uint32_t)(usecs >> 32);

    return xos_do_syscall(XOS_SC_THREAD_SLEEP_USEC,
                          hc,
                          lc,
                          0,
                          0);
}


//-----------------------------------------------------------------------------
// Semaphore syscalls.
//-----------------------------------------------------------------------------

typedef uint32_t xos_usem;              ///< User-mode semaphore handle type

//-----------------------------------------------------------------------------
///
///  Create (or find) a user-mode semaphore. If an existing semaphore matches
///  the key value then its reference count will be incremented and a handle
///  to it will be returned.
///
///  \param     key             Unique identifier. If non-zero, will be used
///                             to look for a match. If zero, no search will
///                             be done.
///
///  \param     initial_count   Initial count for semaphore on creation. Must
///                             be >= 0.
///
///  \param     psem            Pointer to storage for semaphore handle.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
static inline int32_t
xos_usem_create(xos_key key, int8_t initial_count, xos_usem * psem)
{
    uint32_t lk = (uint32_t)(key & 0xFFFFFFFF);
    uint32_t hk = (uint32_t)(key >> 32);

    return xos_do_syscall(XOS_SC_SEM_CREATE,
                          hk,
                          lk,
                          (uint32_t) initial_count,
                          (uint32_t)psem);
}

//-----------------------------------------------------------------------------
///
///  Destroy semaphore object. The reference count will be decremented and the
///  object will not be actually destroyed until the count goes to zero.
///
///  \param     sem             Semaphore handle.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
static inline int32_t
xos_usem_delete(xos_usem sem)
{
    return xos_do_syscall(XOS_SC_SEM_DELETE,
                          sem,
                          0,
                          0,
                          0);
}

//-----------------------------------------------------------------------------
///
///  Decrement the semaphore count: block until the decrement is possible.
///
///  \param     sem             Semaphore handle.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
static inline int32_t
xos_usem_get(xos_usem sem)
{
    return xos_do_syscall(XOS_SC_SEM_GET,
                          sem,
                          0,
                          0,
                          0);
}

//-----------------------------------------------------------------------------
///
///  Decrement the semaphore count: block until the decrement is possible or
///  the timeout expires.
///
///  \param     sem             Semaphore handle.
///
///  \param     usecs           Timeout in microseconds. Zero indicates no
///                             timeout.
///
///  \return    Returns XOS_OK on success, XOS_ERR_TIMEOUT on timeout, else error code.
///
///  NOTE: If XOS_OPT_WAIT_TIMEOUT is not enabled, then the timeout value is
///  ignored, and no timeout will occur.
///
//-----------------------------------------------------------------------------
static inline int32_t
xos_usem_get_timeout(xos_usem sem, uint64_t usecs)
{
    uint32_t lc = (uint32_t)(usecs & 0xFFFFFFFF);
    uint32_t hc = (uint32_t)(usecs >> 32);

    return xos_do_syscall(XOS_SC_SEM_GET_TIMEOUT,
                          sem,
                          hc,
                          lc,
                          0);
}

//-----------------------------------------------------------------------------
///
///  Increment the semaphore count.
///
///  \param     sem             Semaphore handle.
///
///  \return    Returns XOS_OK on success, else error code.   
///
//-----------------------------------------------------------------------------
static inline int32_t
xos_usem_put(xos_usem sem)
{
    return xos_do_syscall(XOS_SC_SEM_PUT,
                          sem,
                          0,
                          0,
                          0);
}

//-----------------------------------------------------------------------------
///
///  Increment the semaphore count only if the specified maximum count will not
///  be exceeded.
///
///  \param     sem             Semaphore handle.
///
///  \param     max             Maximum count of semaphore to enforce. Must be
///                             > 0.
///
///  \return    Returns XOS_OK on success, XOS_ERR_LIMIT if the limit would be
///             exceeded, else error code.
///
//-----------------------------------------------------------------------------
static inline int32_t
xos_usem_put_max(xos_usem sem, int32_t max)
{
    return xos_do_syscall(XOS_SC_SEM_PUT_MAX,
                          sem,
                          (uint32_t)max,
                          0,
                          0);
}


//-----------------------------------------------------------------------------
// Mutex syscalls.
//-----------------------------------------------------------------------------

typedef uint32_t xos_umutex;            ///< User-mode mutex handle type

//-----------------------------------------------------------------------------
///
///  Create (or find) a user-mode mutex. If an existing mutex matches the key
///  value then its reference count will be incremented and a handle to it 
///  will be returned.
///
///  \param     key             Unique identifier. If non-zero, will be used
///                             to look for a match. If zero, no search will
///                             be done.
///
///  \param     priority        Mutex's priority ceiling. This is used only if
///                             option XOS_OPT_MUTEX_PRIORITY is enabled. Legal
///                             values are from 0 .. (XOS_NUM_PRIORITY - 1).
///                             A value of zero disables priority ceiling.
///
///  \param     pmtx            Pointer to storage for mutex handle.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
static inline int32_t
xos_umutex_create(xos_key key, int8_t priority, xos_umutex * pmtx)
{
    uint32_t lk = (uint32_t)(key & 0xFFFFFFFF);
    uint32_t hk = (uint32_t)(key >> 32);

    return xos_do_syscall(XOS_SC_MTX_CREATE,
                          hk,
                          lk,
                          priority,
                          (uint32_t)pmtx);
}

//-----------------------------------------------------------------------------
///
///  Destroy mutex object. The reference count will be decremented and the
///  object will not be actually destroyed until the count goes to zero.
///
///  \param     mtx             Mutex handle.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
static inline int32_t
xos_umutex_delete(xos_umutex mtx)
{
    return xos_do_syscall(XOS_SC_MTX_DELETE,
                          mtx,
                          0,
                          0,
                          0);
}

//-----------------------------------------------------------------------------
///
///  Take ownership of the mutex: block until the mutex is owned.
///
///  \param     mtx             Mutex handle.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
static inline int32_t
xos_umutex_lock(xos_umutex mtx)
{
    return xos_do_syscall(XOS_SC_MTX_LOCK,
                          mtx,
                          0,
                          0,
                          0);
}

//-----------------------------------------------------------------------------
///
///  Take ownership of the mutex: block until the mutex is owned or the timeout
///  expires.
///
///  \param     mtx             Mutex handle.
///
///  \param     usecs           Timeout in microseconds.
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
static inline int32_t
xos_umutex_lock_timeout(xos_umutex mtx, uint64_t usecs)
{
    uint32_t lc = (uint32_t)(usecs & 0xFFFFFFFF);
    uint32_t hc = (uint32_t)(usecs >> 32);

    return xos_do_syscall(XOS_SC_MTX_LOCK_TIMEOUT,
                          mtx,
                          hc,
                          lc,
                          0);
}

//-----------------------------------------------------------------------------
///
///  Release ownership of the mutex. The mutex must be owned by the calling
///  thread.
///
///  \param     mtx             Mutex handle.
///
///  \return    Returns XOS_OK on success, else error code.   
///
//-----------------------------------------------------------------------------
static inline int32_t
xos_umutex_unlock(xos_umutex mtx)
{
    return xos_do_syscall(XOS_SC_MTX_UNLOCK,
                          mtx,
                          0,
                          0,
                          0);
}


//-----------------------------------------------------------------------------
// Condition syscalls.
//-----------------------------------------------------------------------------

typedef uint32_t xos_ucond;             ///< User-mode condition object handle

//-----------------------------------------------------------------------------
///
///  Create (or find) a user-mode condition. If an existing condition matches
///  the key value then its reference count will be incremented and a handle
///  to it will be returned.
///
///  \param     key             Unique identifier. If non-zero, will be used
///                             to look for a match. If zero, no search will
///                             be done.
///
///  \param     pcond           Pointer to storage for condition handle.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
static inline int32_t
xos_ucond_create(xos_key key, xos_ucond * pcond)
{
    uint32_t lk = (uint32_t)(key & 0xFFFFFFFF);
    uint32_t hk = (uint32_t)(key >> 32);

    return xos_do_syscall(XOS_SC_COND_CREATE,
                          hk,
                          lk,
                          (uint32_t)pcond,
                          0);
}

//-----------------------------------------------------------------------------
///
///  Destroy condition object. The reference count will be decremented and the
///  object will not be actually destroyed until the count goes to zero.
///
///  \param     cond            Condition handle.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
static inline int32_t
xos_ucond_delete(xos_ucond cond)
{
    return xos_do_syscall(XOS_SC_COND_DELETE,
                          cond,
                          0,
                          0,
                          0);
}

//-----------------------------------------------------------------------------
///
///  Wait on a condition: block until the condition is satisfied. The condition
///  is satisfied when xos_ucond_signal() or xos_ucond_signal_one() is called
///  on this condition.
///
///  \param     cond            Condition handle.
///
///  \return    Returns the value passed to xos_ucond_signal(), or error code.
///
//-----------------------------------------------------------------------------
static inline int32_t
xos_ucond_wait(xos_ucond cond)
{
    return xos_do_syscall(XOS_SC_COND_WAIT,
                          cond,
                          0,
                          0,
                          0);
}

//-----------------------------------------------------------------------------
///
///  Atomically release the mutex and block until the condition is satisfied.
///  The condition is satisfied when xos_ucond_signal() or xos_ucond_signal_one()
///  is called on this condition.
///
///  \param     cond            Condition handle.
///
///  \param     mtx             Mutex handle.
///
///  \return    Returns the value passed to xos_ucond_signal(), or error code.
///
//-----------------------------------------------------------------------------
static inline int32_t
xos_ucond_wait_mtx(xos_ucond cond, xos_umutex mtx)
{
    return xos_do_syscall(XOS_SC_COND_WAIT_MTX,
                          cond,
                          mtx,
                          0,
                          0);
}

//-----------------------------------------------------------------------------
///
///  Atomically release the mutex and block until the condition is satisfied
///  or the timeout expires. When this function returns, the mutex has been
///  locked again and is owned by the calling thread.
///
///  \param     cond            Condition handle.
///
///  \param     mtx             Mutex handle.
///
///  \param     usecs           Timeout in microseconds.
///
///  \return    Returns the value passed to xos_ucond_signal(), or error code.
///
///  NOTE: If XOS_OPT_WAIT_TIMEOUT is not enabled, then the timeout value is
///  ignored, and no timeout will occur. If the function does time out it will
///  still attempt to lock the mutex and so could wait for an indefinite period.
///
//-----------------------------------------------------------------------------
static inline int32_t
xos_ucond_wait_mtx_timeout(xos_ucond cond, xos_umutex mtx, uint64_t usecs)
{
    uint32_t lc = (uint32_t)(usecs & 0xFFFFFFFF);
    uint32_t hc = (uint32_t)(usecs >> 32);

    return xos_do_syscall(XOS_SC_COND_WAIT_MTX_TIMEOUT,
                          cond,
                          mtx,
                          hc,
                          lc);
}

//-----------------------------------------------------------------------------
///
///  Trigger the condition: wake all threads waiting on the condition.
///
///  \param     cond            Condition handle.
///
///  \param     sig_value       Value passed to all waiters, returned by
///                             xos_ucond_wait().
///
///  \return    Returns number of woken threads on success, else error code.
///
///  NOTE: Signaling a condition that has no waiters has no effect on it, and
///  the signal is not remembered. Any thread that waits on it later must be
///  woken by another call to xos_ucond_signal() or xos_ucond_signal_one().
///
//-----------------------------------------------------------------------------
static inline int32_t
xos_ucond_signal(xos_ucond cond, int32_t sig_value)
{
    return xos_do_syscall(XOS_SC_COND_SIGNAL,
                          cond,
                          (uint32_t)sig_value,
                          0,
                          0);
}

//-----------------------------------------------------------------------------
///
///  Trigger the condition: wake one thread waiting on the condition.
///
///  \param     cond            Condition handle.
///
///  \param     sig_value       Value passed to woken thread, returned by
///                             xos_ucond_wait().
///
///  \return    Returns 1 if a thread was woken, 0 if no threads were woken,
///             else error code.
///
///  NOTE: Signaling a condition that has no waiters has no effect on it, and
///  the signal is not remembered. Any thread that waits on it later must be
///  woken by another call to xos_ucond_signal() or xos_ucond_signal_one().
///
//-----------------------------------------------------------------------------
static inline int32_t
xos_ucond_signal_one(xos_ucond cond, int32_t sig_value)
{
    return xos_do_syscall(XOS_SC_COND_SIGNAL_ONE,
                          cond,
                          (uint32_t)sig_value,
                          0,
                          0);
}


#ifdef __cplusplus
}
#endif

#endif // XOS_SYSCALL_H

