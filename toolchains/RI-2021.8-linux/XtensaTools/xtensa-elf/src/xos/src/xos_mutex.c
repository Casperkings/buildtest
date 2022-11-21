
// xos_mutex.c - XOS Mutex API interface and data structures.

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


#define XOS_INCLUDE_INTERNAL
#include "xos.h"


#define XOS_MUTEX_SIG    0x6d757478U    // Signature of valid object


//-----------------------------------------------------------------------------
//  Initialize a mutex object before first use. Not thread safe.
//-----------------------------------------------------------------------------
int32_t
xos_mutex_create(XosMutex * mutex, uint32_t flags, int8_t priority)
{
    if ((mutex == XOS_NULL) ||
        (priority < 0)      ||
        (priority >= XOS_MAX_PRIORITY)) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    mutex->owner       = XOS_NULL;
    mutex->lock_count  = 0;
    mutex->waitq.head  = XOS_NULL;
    mutex->waitq.tail  = &(mutex->waitq.head);
    mutex->flags       = flags;
    mutex->sig         = XOS_MUTEX_SIG;

#if XOS_OPT_MUTEX_PRIORITY
    mutex->flags |= XOS_MUTEX_PRIO_INHERIT;
    if (priority > 0) {
        mutex->flags |= XOS_MUTEX_PRIO_PROTECT;
    }

    mutex->priority    = priority;
    mutex->next        = XOS_NULL;
    mutex->waitq.flags = XOS_Q_WAIT_PRIO;
#else
    UNUSED(priority);
    mutex->waitq.flags =
        ((flags & XOS_MUTEX_WAIT_FIFO) != 0U) ? XOS_Q_WAIT_FIFO : XOS_Q_WAIT_PRIO;
#endif

    return XOS_OK;
}


//-----------------------------------------------------------------------------
//  Destroy a mutex object. Release any waiting threads.
//-----------------------------------------------------------------------------
int32_t
xos_mutex_delete(XosMutex * mutex)
{
    XosThread * oldid = XOS_NULL;
    XosThread * myid  = xos_thread_id();
    XosThread * thread;
    bool        flag = false;
    uint32_t    ps;

    // Can't call this from interrupt context.
    if (INTERRUPT_CONTEXT) {
        xos_fatal_error(XOS_ERR_INTERRUPT_CONTEXT, XOS_NULL);
    }

    if ((mutex == XOS_NULL) || (mutex->sig != XOS_MUTEX_SIG)) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    ps = xos_critical_enter();

#if XOS_OPT_C11_ATOMICS
    bool res;

    do {
        res = atomic_compare_exchange_weak(&(mutex->owner), &oldid, myid);
    }
    while ((res == false) && (oldid == XOS_NULL));
#else
    oldid = mutex->owner;
#endif

    // Must be unlocked or locked by calling thread.
    if ((oldid != XOS_NULL) && (oldid != myid)) {
        xos_critical_exit(ps);
        return XOS_ERR_MUTEX_NOT_OWNED;
    }

#if XOS_OPT_MUTEX_PRIORITY
    // If mutex was owned, remove from thread's owned list.
    if (oldid != XOS_NULL) {
        xos_thread_mutex_rem(myid, mutex);
    }
    mutex->priority = 0;
    mutex->next     = XOS_NULL;
#endif

    // Release any waiting threads. Scheduler is disabled because we
    // don't want to context switch until all threads are woken.
    if (xos_q_peek(&mutex->waitq) != XOS_NULL) {
        (void) xos_preemption_disable();
        flag = true;
        while (xos_q_peek(&mutex->waitq) != XOS_NULL) {
            thread = xos_q_pop(&mutex->waitq);
            xos_thread_wake(thread, xos_blkon_mutex, XOS_ERR_MUTEX_DELETE);
        }
    }

    mutex->lock_count = 0;
    mutex->flags      = 0;
    mutex->waitq.head = XOS_NULL;
    mutex->waitq.tail = XOS_NULL;
    mutex->sig        = 0;

    // Clear the owner field last.
    mutex->owner = XOS_NULL;

    xos_critical_exit(ps);
    if (flag) {
        // May cause immediate context switch.
        (void) xos_preemption_enable();
    }

    return XOS_OK;
}


//-----------------------------------------------------------------------------
//  Take ownership of the mutex: block until the mutex is owned.
//-----------------------------------------------------------------------------
int32_t
xos_mutex_lock(XosMutex * mutex)
{
    int32_t  ret = XOS_OK;
    uint32_t ps;

    // Can't call this from interrupt context.
    if (INTERRUPT_CONTEXT) {
        xos_fatal_error(XOS_ERR_INTERRUPT_CONTEXT, XOS_NULL);
    }

    if ((mutex == XOS_NULL) || (mutex->sig != XOS_MUTEX_SIG)) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    // Can't allow the mutex state to change between the check and the
    // thread block (if needed). Else we may end up blocking on a free
    // mutex and nothing to wake us.
    ps = xos_critical_enter();

    while (ret == XOS_OK) {
        XosThread * oldid = XOS_NULL;
        XosThread * myid  = xos_thread_id();

#if XOS_OPT_C11_ATOMICS
        bool res;

        do {
            res = atomic_compare_exchange_weak(&(mutex->owner), &oldid, myid);
        }
        while ((res == false) && (oldid == XOS_NULL));
#else
        oldid = mutex->owner;
#endif
        if (oldid == XOS_NULL) {
            XOS_ASSERT(mutex->lock_count == 0);

#if !XOS_OPT_C11_ATOMICS
            mutex->owner      = myid;
#endif
            mutex->lock_count = 1;
#if XOS_OPT_MUTEX_PRIORITY
            // Add to thread's owned list and reprioritize if needed.
            xos_thread_mutex_add(mutex->owner, mutex);
#endif
            break;
        }

        if (oldid == myid) {
            // Mutex already owned, bump lock count.
            if (mutex->lock_count < INT32_MAX) {
                mutex->lock_count++;
                break;
            }
            else {
                xos_critical_exit(ps);
                return XOS_ERR_LIMIT;
            }
        }

#if XOS_OPT_MUTEX_PRIORITY
        // Before blocking, raise the owner thread's priority if it
        // is lower than the current thread's (inheritance).
        if (mutex->owner->curr_priority < myid->curr_priority) {
            xos_thread_mutex_bump(mutex->owner, mutex, myid->curr_priority);
        }
#endif

        // Put us on the wait queue and block. Use fifo or priority
        // order as specified.
        ret = xos_block(xos_blkon_mutex,
                        &mutex->waitq,
                        0,
                        ((mutex->waitq.flags & XOS_Q_WAIT_PRIO) != 0U) ? 1 : 0);
    }

    if (ret == XOS_OK) {
        xos_log_sysevent(XOS_SE_MUTEX_LOCK,
                         xos_threadp_to_uint32(mutex->owner),
                         (uint32_t) mutex->lock_count);
    }

    xos_critical_exit(ps);
    return ret;
}


//-----------------------------------------------------------------------------
//  Timer callback to handle wait timeouts.
//-----------------------------------------------------------------------------
#if XOS_OPT_WAIT_TIMEOUT
static void
mutex_wakeup_cb(void * arg)
{
    XosThread * thread = xos_voidp_to_threadp(arg);

    XOS_ASSERT(thread != XOS_NULL);

    // If the thread is already unblocked or blocked on something else
    // that will be handled inside xos_thread_wake().
    xos_thread_wake(thread, xos_blkon_mutex, XOS_ERR_TIMEOUT);
}
#endif


//-----------------------------------------------------------------------------
//  Take ownership of the mutex: block until the mutex is owned or the timeout
//  expires.
//-----------------------------------------------------------------------------
int32_t
xos_mutex_lock_timeout(XosMutex * mutex, uint64_t to_cycles)
{
    int32_t  ret = XOS_OK;
    uint32_t ps;

#if XOS_OPT_WAIT_TIMEOUT
    XosTimer timer;
    bool     timer_started = false;
#endif

    // Can't call this from interrupt context.
    if (INTERRUPT_CONTEXT) {
        xos_fatal_error(XOS_ERR_INTERRUPT_CONTEXT, XOS_NULL);
    }

    if ((mutex == XOS_NULL) || (mutex->sig != XOS_MUTEX_SIG)) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    // Can't allow the mutex state to change between the check and the
    // thread block (if needed). Else we may end up blocking on a free
    // mutex and nothing to wake us.
    ps = xos_critical_enter();

    while (ret == XOS_OK) {
        XosThread * oldid = XOS_NULL;
        XosThread * myid  = xos_thread_id();

#if XOS_OPT_C11_ATOMICS
        bool res;

        do {
            res = atomic_compare_exchange_weak(&(mutex->owner), &oldid, myid);
        }
        while ((res == false) && (oldid == XOS_NULL));
#else
        oldid = mutex->owner;
#endif
        if (oldid == XOS_NULL) {
            XOS_ASSERT(mutex->lock_count == 0);

#if !XOS_OPT_C11_ATOMICS
            mutex->owner      = myid;
#endif
            mutex->lock_count = 1;
#if XOS_OPT_MUTEX_PRIORITY
            // Add to thread's owned list and reprioritize if needed.
            xos_thread_mutex_add(mutex->owner, mutex);
#endif
            break;
        }

        if (oldid == myid) {
            // Mutex already owned, bump lock count.
            if (mutex->lock_count < INT32_MAX) {
                mutex->lock_count++;
                break;
            }
            else {
                xos_critical_exit(ps);
                return XOS_ERR_LIMIT;
            }
        }

#if XOS_OPT_WAIT_TIMEOUT
        // If specified and enabled, set up a timeout for the wait.
        // Set the timer on the first pass and check it for expiry
        // on subsequent passes.
        if (to_cycles > 0ULL) {
            if (!timer_started) {
                (void) xos_timer_init(&timer);
                ret = xos_timer_start(&timer,
                                      to_cycles,
                                      XOS_TIMER_DELTA | XOS_TIMER_FROM_NOW,
                                      &mutex_wakeup_cb,
                                      xos_curr_threadptr);
                if (ret != XOS_OK) {
                    break;
                }

                timer_started = true;
            }
            else {
                // Timer was started, see if it expired.
                if (xos_timer_is_active(&timer) == 0) {
                    ret = XOS_ERR_TIMEOUT;
                    break;
                }
            }
        }
#else
        UNUSED(to_cycles);
#endif

        // Put us on the wait queue and block. Use fifo or priority
        // order as specified.
        ret = xos_block(xos_blkon_mutex,
                        &mutex->waitq,
                        0,
                        ((mutex->waitq.flags & XOS_Q_WAIT_PRIO) != 0U) ? 1 : 0);
    }

#if XOS_OPT_WAIT_TIMEOUT
    if (timer_started) {
        (void) xos_timer_stop(&timer);
    }
#endif

    if (ret == XOS_OK) {
        xos_log_sysevent(XOS_SE_MUTEX_LOCK,
                         xos_threadp_to_uint32(mutex->owner),
                         (uint32_t) mutex->lock_count);
    }

    xos_critical_exit(ps);
    return ret;
}


//-----------------------------------------------------------------------------
//  Release ownership of the mutex.
//-----------------------------------------------------------------------------
int32_t
xos_mutex_unlock(XosMutex * mutex)
{
    int32_t  ret = XOS_OK;
    uint32_t ps;

    // Can't call this from interrupt context.
    if (INTERRUPT_CONTEXT) {
        xos_fatal_error(XOS_ERR_INTERRUPT_CONTEXT, XOS_NULL);
    }

    if ((mutex == XOS_NULL) || (mutex->sig != XOS_MUTEX_SIG)) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    // Disable interrupts and scheduler. Don't want a context switch until
    // this operation is complete.
    ps = xos_critical_enter();
    (void) xos_preemption_disable();

    if (mutex->owner == xos_thread_id()) {
        XOS_ASSERT(mutex->lock_count > 0);

        mutex->lock_count--;

        xos_log_sysevent(XOS_SE_MUTEX_UNLOCK,
                         xos_threadp_to_uint32(mutex->owner),
                         (uint32_t) mutex->lock_count);

        if (mutex->lock_count == 0) {
            XosThread * waiter;

#if XOS_OPT_MUTEX_PRIORITY
            // Remove from thread's owned list.
            xos_thread_mutex_rem(xos_thread_id(), mutex);
#endif

            // Wake one thread if any waiting. Scheduler is locked.
            waiter = xos_q_pop(&mutex->waitq);
            if (waiter != XOS_NULL) {
                xos_thread_wake(waiter, xos_blkon_mutex, XOS_OK);
            }

            // Mark as free.
            mutex->owner = XOS_NULL;
        }
    }
    else {
        ret = XOS_ERR_MUTEX_NOT_OWNED;
    }

    xos_critical_exit(ps);
    // May cause immediate context switch.
    (void) xos_preemption_enable();

    return ret;
}


//-----------------------------------------------------------------------------
//  Try to take ownership of the mutex, but do not block if the mutex is taken.
//-----------------------------------------------------------------------------
int32_t
xos_mutex_trylock(XosMutex * mutex)
{
    int32_t     ret   = XOS_OK;
    XosThread * oldid = XOS_NULL;
    XosThread * myid  = xos_thread_id();
    uint32_t    ps;

    // Can't call this from interrupt context.
    if (INTERRUPT_CONTEXT) {
        xos_fatal_error(XOS_ERR_INTERRUPT_CONTEXT, XOS_NULL);
    }

    if ((mutex == XOS_NULL) || (mutex->sig != XOS_MUTEX_SIG)) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    ps = xos_critical_enter();

#if XOS_OPT_C11_ATOMICS
    bool res;

    do {
        res = atomic_compare_exchange_weak(&(mutex->owner), &oldid, myid);
    }
    while ((res == false) && (oldid == XOS_NULL));
#else
    oldid = mutex->owner;
#endif
    if (oldid == XOS_NULL) {
        mutex->owner      = myid;
        mutex->lock_count = 1;
#if XOS_OPT_MUTEX_PRIORITY
            // Add to thread's owned list and reprioritize if needed.
            xos_thread_mutex_add(mutex->owner, mutex);
#endif
    }
    else if (oldid == myid) {
        // Bump lock count.
        if (mutex->lock_count < INT32_MAX) {
            mutex->lock_count++;
        }
        else {
            ret = XOS_ERR_LIMIT;
        }
    }
    else {
        ret = XOS_ERR_MUTEX_LOCKED;
    }

    if (ret == XOS_OK) {
        xos_log_sysevent(XOS_SE_MUTEX_LOCK,
                         xos_threadp_to_uint32(mutex->owner),
                         (uint32_t) mutex->lock_count);
    }

    xos_critical_exit(ps);
    return ret;
}


//-----------------------------------------------------------------------------
//  Return state of mutex.
//-----------------------------------------------------------------------------
int32_t
xos_mutex_test(const XosMutex * mutex)
{
    if ((mutex == XOS_NULL) || (mutex->sig != XOS_MUTEX_SIG)) {
        return -1;
    }

    return (mutex->owner != XOS_NULL) ? 1 : 0;
}


//-----------------------------------------------------------------------------
//  Return mutex given its contained wait queue (if valid object).
//-----------------------------------------------------------------------------
XosMutex *
xos_mutexq_to_mutex(XosThreadQueue * q)
{
    XosMutex * mutex;

    if (q != XOS_NULL) {
        mutex = (XosMutex *)(xos_voidp_to_uint32(q) - offsetof(XosMutex, waitq));
        if (mutex->sig == XOS_MUTEX_SIG) {
            return mutex;
        }
    }

    return NULL;
}


//-----------------------------------------------------------------------------
//  User-mode support.
//-----------------------------------------------------------------------------

#if XOS_OPT_UM

extern XosUMutex xos_um_mtxs[XOS_UM_NUM_MTX];

//-----------------------------------------------------------------------------
//  Search mutex table for a match. The match for a key value of zero will be
//  the first free entry (if any).
//-----------------------------------------------------------------------------
static XosUMutex *
xos_umutex_find(uint64_t key)
{
    uint32_t i;

    for (i = 0; i < XOS_UM_NUM_MTX; i++) {
        if (xos_um_mtxs[i].key == key) {
            return &(xos_um_mtxs[i]);
        }
    }

    return XOS_NULL;
}


//-----------------------------------------------------------------------------
//  Create user-mode mutex. Or find and return existing mutex if key matches.
//-----------------------------------------------------------------------------
int32_t
xos_umutex_create_p(uint64_t     key,
                    int8_t       priority,
                    XosUMutex ** umtx)
{
    XosUMutex * m;
    int32_t     ret;
    uint32_t    ps;

    if (umtx == XOS_NULL) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    ps = xos_critical_enter();

    if (key != XOS_KEY_ZERO) {
        m = xos_umutex_find(key);
        if (m != XOS_NULL) {
            m->refs++;
            *umtx = m;
            xos_critical_exit(ps);
            return XOS_OK;
        }
    }

    m = xos_umutex_find(XOS_KEY_ZERO);
    if (m == XOS_NULL) {
        ret = XOS_ERR_LIMIT;
    }
    else {
        ret = xos_mutex_create(&(m->mtx),
                               XOS_MUTEX_WAIT_PRIORITY,
                               priority);
        if (ret == XOS_OK) {
            m->key  = (key == XOS_KEY_ZERO) ? (uint64_t) m : key;
            m->refs = 1;
            *umtx   = m;
        }
    }

    xos_critical_exit(ps);
    return ret;
}


//-----------------------------------------------------------------------------
//  Destroy user-mode mutex.
//-----------------------------------------------------------------------------
int32_t
xos_umutex_delete_p(XosUMutex * umtx)
{
    int32_t  ret;
    uint32_t i;
    uint32_t ps;

    if (umtx == XOS_NULL) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    ps = xos_critical_enter();

    // Delete if found in mutex table.
    ret = XOS_ERR_NOT_FOUND;
    for (i = 0; i < XOS_UM_NUM_MTX; i++) {
        XosUMutex * m = xos_um_mtxs + i;

        if (m == umtx) {
            XOS_ASSERT(m->refs > 0);
            if (m->refs > 0) {
                m->refs--;
            }
            if (m->refs == 0) {
                ret = xos_mutex_delete(&(m->mtx));
                if (ret == XOS_OK) {
                    m->key = XOS_KEY_ZERO;
                }
            }
            else {
                ret = XOS_OK;
            }

            break;
        }
    }

    xos_critical_exit(ps);
    return ret;
}


//-----------------------------------------------------------------------------
//  Return pointer to mutex given pointer to user-mode mutex.
//-----------------------------------------------------------------------------
XosMutex *
xos_umutex_mutex(XosUMutex * umtx)
{
    if ((umtx != XOS_NULL) && (umtx->mtx.sig == XOS_MUTEX_SIG)) {
        // Range check - must lie within the array of user-mode mutexes.
        if (umtx < xos_um_mtxs) {
            return XOS_NULL;
        }
        if (umtx > (xos_um_mtxs + XOS_UM_NUM_MTX - 1)) {
            return XOS_NULL;
        }
        return &(umtx->mtx);
    }

    return XOS_NULL;
}


#endif // XOS_OPT_UM

