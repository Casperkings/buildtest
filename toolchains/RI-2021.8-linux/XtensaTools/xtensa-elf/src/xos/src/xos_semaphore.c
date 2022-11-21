
// xos_semaphore.c - XOS Semaphore API interface and data structures.

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


#define XOS_SEM_SIG     0x73656d61U     // Signature of valid object


//-----------------------------------------------------------------------------
// Type conversion helpers.
//-----------------------------------------------------------------------------
__attribute__((always_inline))
static inline uint32_t
xos_semp_to_uint32(XosSem * arg)
{
    return (uint32_t) arg; // parasoft-suppress MISRAC2012-RULE_11_4-a-4 "Type conversion checked"
}


//-----------------------------------------------------------------------------
//  Initialize a semaphore object before first use.
//-----------------------------------------------------------------------------
int32_t
xos_sem_create(XosSem * sem, uint32_t flags, int32_t initial_count)
{
    if ((sem == XOS_NULL) || (initial_count < 0)) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    sem->count = initial_count;
    sem->flags = flags;
    sem->waitq.head  = XOS_NULL;
    sem->waitq.tail  = &(sem->waitq.head);
    sem->waitq.flags =
        ((flags & XOS_SEM_WAIT_FIFO) != 0U) ? XOS_Q_WAIT_FIFO : XOS_Q_WAIT_PRIO;
    sem->sig = XOS_SEM_SIG;

    return XOS_OK;
}


//-----------------------------------------------------------------------------
//  Destroy a semaphore object. Release any waiting threads.
//  In the worst case, this can leave interrupts disabled for quite a while,
//  but since this is expected to be a very infrequent operation, it should
//  be OK. Especially since in normal operation we shouldn't have any waiting
//  threads at delete time.
//-----------------------------------------------------------------------------
int32_t
xos_sem_delete(XosSem * sem)
{
    bool     flag = false;
    uint32_t ps;

    if ((sem == XOS_NULL) || (sem->sig != XOS_SEM_SIG)) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    // Make sure no one else can change semaphore state (get/put etc.).
    // Interrupt handlers are allowed to put so protect against that.
    ps = xos_critical_enter();

    // Release any waiting threads.
    if (sem->waitq.head != XOS_NULL) {
        XosThread * waiter;

        // Don't want unexpected context switching to happen here.
        (void) xos_preemption_disable();
        flag = true;

        while (xos_q_peek(&sem->waitq) != XOS_NULL) {
            waiter = xos_q_pop(&sem->waitq);
            xos_thread_wake(waiter, xos_blkon_sem, XOS_ERR_SEM_DELETE);
        }
    }

    // Must do this with the object protected against updates.
    sem->count = 0;
    sem->flags = 0;
    sem->sig   = 0;
    sem->waitq.head = XOS_NULL;
    sem->waitq.tail = XOS_NULL;

    xos_critical_exit(ps);

    if (flag) {
        // Unlock the scheduler. May cause immediate context switch.
        (void) xos_preemption_enable();
    }

    return XOS_OK;
}


//-----------------------------------------------------------------------------
//  Decrement the semaphore, block until possible.
//-----------------------------------------------------------------------------
int32_t
xos_sem_get(XosSem * sem)
{
    int32_t  ret = XOS_OK;
    int32_t  oldcnt;
    uint32_t ps;

    // Can't call this from interrupt context.
    if (INTERRUPT_CONTEXT) {
        xos_fatal_error(XOS_ERR_INTERRUPT_CONTEXT, XOS_NULL);
    }

    if ((sem == XOS_NULL) || (sem->sig != XOS_SEM_SIG)) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    while (ret == XOS_OK) {
#if XOS_OPT_C11_ATOMICS
        // Fast path, no locking.
        oldcnt = atomic_fetch_sub(&(sem->count), 1);
        if (oldcnt <= 0) {
            atomic_fetch_add(&(sem->count), 1);
        }
        else {
            break;
        }
#endif

        // Slow path.
        ps = xos_critical_enter();

#if XOS_OPT_C11_ATOMICS
        oldcnt = atomic_fetch_sub(&(sem->count), 1);
        if (oldcnt <= 0) {
            atomic_fetch_add(&(sem->count), 1);
        }
#else
        oldcnt = sem->count;
        if (oldcnt > 0) {
            sem->count--;
        }
#endif
        if (oldcnt > 0) {
            xos_critical_exit(ps);
            break;
        }

        // Put us on the wait queue and block. Use fifo or priority
        // order as specified.
        ret = xos_block(xos_blkon_sem,
                        &sem->waitq,
                        0,
                        ((sem->flags & XOS_SEM_WAIT_FIFO) != 0U) ? 0 : 1);
        xos_critical_exit(ps);
    }

    if (ret == XOS_OK) {
        xos_log_sysevent(XOS_SE_SEM_GET,
                         xos_semp_to_uint32(sem),
                         (uint32_t) sem->count);
    }

    return ret;
}


//-----------------------------------------------------------------------------
//  Timer callback to handle wait timeouts.
//-----------------------------------------------------------------------------
#if XOS_OPT_WAIT_TIMEOUT
static void
sem_wakeup_cb(void * arg)
{
    XosThread * thread = xos_voidp_to_threadp(arg);

    XOS_ASSERT(thread != XOS_NULL);

    // If the thread is already unblocked or blocked on something else
    // that will be handled inside xos_thread_wake().
    xos_thread_wake(thread, xos_blkon_sem, XOS_ERR_TIMEOUT);
}
#endif


//-----------------------------------------------------------------------------
//  Decrement the semaphore, block until possible or until timeout occurs.
//-----------------------------------------------------------------------------
int32_t
xos_sem_get_timeout(XosSem * sem, uint64_t to_cycles)
{
    int32_t  ret = XOS_OK;
    int32_t  oldcnt;
    uint32_t ps;

#if XOS_OPT_WAIT_TIMEOUT
    XosTimer timer;
    bool     timer_started = false;
#endif

    // Can't call this from interrupt context.
    if (INTERRUPT_CONTEXT) {
        xos_fatal_error(XOS_ERR_INTERRUPT_CONTEXT, XOS_NULL);
    }

    if ((sem == XOS_NULL) || (sem->sig != XOS_SEM_SIG)) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    while (ret == XOS_OK) {
#if XOS_OPT_C11_ATOMICS
        // Fast path, no locking.
        oldcnt = atomic_fetch_sub(&(sem->count), 1);
        if (oldcnt <= 0) {
            atomic_fetch_add(&(sem->count), 1);
        }
        else {
            break;
        }
#endif

        // Slow path.
        ps = xos_critical_enter();

#if XOS_OPT_C11_ATOMICS
        oldcnt = atomic_fetch_sub(&(sem->count), 1);
        if (oldcnt <= 0) {
            atomic_fetch_add(&(sem->count), 1);
        }
#else
        oldcnt = sem->count;
        if (oldcnt > 0) {
            sem->count--;
        }
#endif
        if (oldcnt > 0) {
            xos_critical_exit(ps);
            break;
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
                                      &sem_wakeup_cb,
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
        ret = xos_block(xos_blkon_sem,
                        &sem->waitq,
                        0,
                        ((sem->flags & XOS_SEM_WAIT_FIFO) != 0U) ? 0 : 1);
        xos_critical_exit(ps);
    }

    if (ret == XOS_OK) {
        xos_log_sysevent(XOS_SE_SEM_GET,
                         xos_semp_to_uint32(sem),
                         (uint32_t) sem->count);
    }

#if XOS_OPT_WAIT_TIMEOUT
    if (timer_started) {
        (void) xos_timer_stop(&timer);
    }
#endif

    return ret;
}


//-----------------------------------------------------------------------------
//  Increment the semaphore count.
//  This will also wake up one waiting thread, if there is one. Note that the
//  woken thread may not actually complete its wait, because by the time it
//  gets to run, it is possible that another thread has decremented the count
//  already. In that case the woken thread will go back to being blocked.
//-----------------------------------------------------------------------------
int32_t
xos_sem_put(XosSem * sem)
{
    uint32_t    ps;
    XosThread * waiter;

    if ((sem == XOS_NULL) || (sem->sig != XOS_SEM_SIG)) {
        return XOS_ERR_INVALID_PARAMETER;
    }

#if XOS_OPT_C11_ATOMICS
    atomic_fetch_add(&(sem->count), 1);
#else
    ps = xos_critical_enter();
    sem->count++;
    xos_critical_exit(ps);
#endif

    xos_log_sysevent(XOS_SE_SEM_PUT,
                     xos_semp_to_uint32(sem),
                     (uint32_t) sem->count);

    ps = xos_critical_enter();
    waiter = xos_q_pop(&sem->waitq);
    if (waiter != XOS_NULL) {
        xos_thread_wake(waiter, xos_blkon_sem, XOS_OK);
    }
    xos_critical_exit(ps);

    return XOS_OK;
}


//-----------------------------------------------------------------------------
//  Increment semaphore count only if count will not exceed 'max'.
//  See notes for xos_sem_put().
//-----------------------------------------------------------------------------
int32_t
xos_sem_put_max(XosSem * sem, int32_t max)
{
    int32_t     ret = XOS_ERR_LIMIT;
    uint32_t    ps;
    XosThread * waiter;
#if XOS_OPT_C11_ATOMICS
    int32_t     oldcnt;
#endif

    if ((sem == XOS_NULL) || (sem->sig != XOS_SEM_SIG)) {
        return XOS_ERR_INVALID_PARAMETER;
    }
    if (max <= 0) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    // Quick check.
    if (sem->count >= max) {
        return XOS_ERR_LIMIT;
    }

#if XOS_OPT_C11_ATOMICS
    oldcnt = atomic_load(&(sem->count));
    while (oldcnt < max) {
        if (atomic_compare_exchange_weak(&(sem->count), &oldcnt, oldcnt + 1)) {
            ret = XOS_OK;
            break;
        }
    }
#else
    ps = xos_critical_enter();
    if (sem->count < max) {
        sem->count++;
        ret = XOS_OK;
    }
    xos_critical_exit(ps);
#endif

    if (ret == XOS_OK) {
        xos_log_sysevent(XOS_SE_SEM_PUT,
                         xos_semp_to_uint32(sem),
                         (uint32_t) sem->count);

        ps = xos_critical_enter();
        waiter = xos_q_pop(&sem->waitq);
        if (waiter != XOS_NULL) {
            xos_thread_wake(waiter, xos_blkon_sem, XOS_OK);
        }
        xos_critical_exit(ps);
    }

    return ret;
}


//-----------------------------------------------------------------------------
//  Try to decrement the semaphore, but do not block if failed.
//-----------------------------------------------------------------------------
int32_t
xos_sem_tryget(XosSem * sem)
{
    int32_t  ret = XOS_OK;
    int32_t  oldcnt;
    uint32_t ps;

    if ((sem == XOS_NULL) || (sem->sig != XOS_SEM_SIG)) {
        return XOS_ERR_INVALID_PARAMETER;
    }

#if XOS_OPT_C11_ATOMICS
        oldcnt = atomic_fetch_sub(&(sem->count), 1);
        if (oldcnt <= 0) {
            atomic_fetch_add(&(sem->count), 1);
        }
        UNUSED(ps);
#else
        ps = xos_critical_enter();
        oldcnt = sem->count;
        if (sem->count > 0) {
            sem->count--;
        }
        xos_critical_exit(ps);
#endif

    if (oldcnt > 0) {
        xos_log_sysevent(XOS_SE_SEM_GET,
                         xos_semp_to_uint32(sem),
                         (uint32_t) sem->count);
    }
    else {
        // Can't take, return.
        ret = XOS_ERR_SEM_BUSY;
    }

    return ret;
}


//-----------------------------------------------------------------------------
//  Return semaphore count.
//-----------------------------------------------------------------------------
int32_t
xos_sem_test(const XosSem * sem)
{
    if ((sem == XOS_NULL) || (sem->sig != XOS_SEM_SIG)) {
        return -1;
    }

    return sem->count;
}


//-----------------------------------------------------------------------------
//  Return semaphore given its contained wait queue (if valid object).
//-----------------------------------------------------------------------------
XosSem *
xos_semq_to_sem(XosThreadQueue * q)
{   
    XosSem * sem;

    if (q != XOS_NULL) {
        sem = (XosSem *)(xos_voidp_to_uint32(q) - offsetof(XosSem, waitq));
        if (sem->sig == XOS_SEM_SIG) {
            return sem;
        }
    }

    return NULL;
}


//-----------------------------------------------------------------------------
//  User-mode support.
//-----------------------------------------------------------------------------

#if XOS_OPT_UM

extern XosUSem xos_um_sems[XOS_UM_NUM_SEM];

//-----------------------------------------------------------------------------
//  Search semaphore table for a match. The match for a key value of zero will
//  be the first free entry (if any).
//-----------------------------------------------------------------------------
static XosUSem *
xos_usem_find(uint64_t key)
{
    uint32_t i;

    for (i = 0; i < XOS_UM_NUM_SEM; i++) {
        if (xos_um_sems[i].key == key) {
            return &(xos_um_sems[i]);
        }
    }

    return XOS_NULL;
}


//-----------------------------------------------------------------------------
//  Create user-mode semaphore. Or find and return existing, if key matches.
//-----------------------------------------------------------------------------
int32_t
xos_usem_create_p(uint64_t   key,
                  int32_t    initial_count,
                  XosUSem ** usem)
{
    XosUSem * s;
    int32_t   ret;
    uint32_t  ps;

    if (usem == XOS_NULL) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    ps = xos_critical_enter();

    if (key != XOS_KEY_ZERO) {
        s = xos_usem_find(key);
        if (s != XOS_NULL) {
            s->refs++;
            *usem = s;
            xos_critical_exit(ps);
            return XOS_OK;
        }
    }

    s = xos_usem_find(XOS_KEY_ZERO);
    if (s == XOS_NULL) {
        ret = XOS_ERR_LIMIT;
    }
    else {
        ret = xos_sem_create(&(s->sem),
                             XOS_SEM_WAIT_PRIORITY,
                             initial_count);
        if (ret == XOS_OK) {
            s->key  = (key == XOS_KEY_ZERO) ? (uint64_t) s : key;
            s->refs = 1;
            *usem   = s;
        }
    }

    xos_critical_exit(ps);
    return ret;
}


//-----------------------------------------------------------------------------
//  Destroy user-mode semaphore.
//-----------------------------------------------------------------------------
int32_t
xos_usem_delete_p(XosUSem * usem)
{
    uint32_t i;
    int32_t  ret;
    uint32_t ps;

    if (usem == XOS_NULL) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    ps = xos_critical_enter();

    // Delete if found in sem table.
    ret = XOS_ERR_NOT_FOUND;
    for (i = 0; i < XOS_UM_NUM_SEM; i++) {
        XosUSem * s = xos_um_sems + i;

        if (s == usem) {
            XOS_ASSERT(s->refs > 0);
            if (s->refs > 0) {
                s->refs--;
            }
            if (s->refs == 0) {
                ret = xos_sem_delete(&(s->sem));
                if (ret == XOS_OK) {
                    s->key = XOS_KEY_ZERO;
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
//  Return pointer to semaphore given pointer to user-mode semaphore.
//-----------------------------------------------------------------------------
XosSem *
xos_usem_sem(XosUSem * usem)
{
    if ((usem != XOS_NULL) && (usem->sem.sig == XOS_SEM_SIG)) {
        // Range check - must lie within the array of user-mode sems.
        if (usem < xos_um_sems) {
            return XOS_NULL;
        }
        if (usem > (xos_um_sems + XOS_UM_NUM_SEM - 1)) {
            return XOS_NULL;
        }
        return &(usem->sem);
    }

    return XOS_NULL;
}


#endif // XOS_OPT_UM

