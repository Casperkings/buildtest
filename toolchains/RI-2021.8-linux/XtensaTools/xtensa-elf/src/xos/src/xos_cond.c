
// xos_cond.c - Condition variables API.

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


#define XOS_COND_SIG    0x636f6e64U     // Signature of valid object


//-----------------------------------------------------------------------------
// Initialize a condition object before first use.
//-----------------------------------------------------------------------------
int32_t
xos_cond_create(XosCond * cond)
{
    if (cond == XOS_NULL) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    cond->queue.head = XOS_NULL;
    cond->queue.tail = &(cond->queue.head);
    cond->sig = XOS_COND_SIG;
    return XOS_OK;
}


//-----------------------------------------------------------------------------
// Destroy a condition object. Release any waiting threads.
//-----------------------------------------------------------------------------
int32_t
xos_cond_delete(XosCond * cond)
{
    uint32_t ps;
    bool     flag = false;

    if ((cond == XOS_NULL) || (cond->sig != XOS_COND_SIG)) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    // Make sure no one else can change object state
    ps = xos_critical_enter();

    if (xos_q_peek(&cond->queue) != XOS_NULL) {
        // Release waiting threads
        XosThread * waiter;

        // Don't want unexpected context switching to happen here
        (void) xos_preemption_disable();
        flag = true;

        waiter = xos_q_pop(&cond->queue);
        while (waiter != XOS_NULL) {
            xos_thread_wake(waiter, xos_blkon_condition, XOS_ERR_COND_DELETE);
            waiter = xos_q_pop(&cond->queue);
        }
    }

    // Must do this with the object protected against updates
    cond->queue.head = XOS_NULL;
    cond->queue.tail = XOS_NULL;
    cond->sig        = 0;

    xos_critical_exit(ps);
    if (flag) {
        // May cause immediate context switch
        (void) xos_preemption_enable();
    }

    return XOS_OK;
}


//-----------------------------------------------------------------------------
// Wait on a condition.
//-----------------------------------------------------------------------------
int32_t
xos_cond_wait(XosCond * cond, XosCondFunc * cond_fn, void * cond_arg)
{
    XosThread * thread = xos_curr_threadptr;
    int32_t     retval = -1;
    uint32_t    ps;

    // Can't call this from interrupt context.
    if (INTERRUPT_CONTEXT) {
        xos_fatal_error(XOS_ERR_INTERRUPT_CONTEXT, XOS_NULL);
    }

    if ((cond == XOS_NULL) || (cond->sig != XOS_COND_SIG)) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    thread->cond_fn  = cond_fn;
    thread->cond_arg = cond_arg;

    ps = xos_critical_enter();
    retval = xos_block(xos_blkon_condition, &cond->queue, 0, 1);
    xos_critical_exit(ps);

    // Can't return an error code here since all possible return values
    // could be returned by the wakeup.
    return retval;
}


//-----------------------------------------------------------------------------
// Atomically release mutex and wait on condition.
//-----------------------------------------------------------------------------
int32_t
xos_cond_wait_mutex(XosCond * cond, XosMutex * mutex)
{
    XosThread * thread = xos_curr_threadptr;
    int32_t     retval = -1;
    int32_t     ret;
    uint32_t    ps;
    
    // Can't call this from interrupt context.
    if (INTERRUPT_CONTEXT) {
        xos_fatal_error(XOS_ERR_INTERRUPT_CONTEXT, XOS_NULL);
    }

    if ((cond == XOS_NULL) || (mutex == XOS_NULL)) {
        return XOS_ERR_INVALID_PARAMETER;
    }
    if (cond->sig != XOS_COND_SIG) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    // Mutex validity and ownership will be checked by the unlock call.

    // Enter a critical section and also lock the scheduler because we
    // don't want a context switch to happen between unlocking the mutex
    // and blocking on the condition.

    ps = xos_critical_enter();
    (void) xos_preemption_disable();

    // Unlock the scheduler but don't actually run the scheduler yet.
    // That will happen when this thread blocks.

    ret = xos_mutex_unlock(mutex);
    (void) xos_preemption_enable_ns();

    // Two possible errors: mutex is invalid or is not locked by caller.
    if (ret != XOS_OK) {
        xos_critical_exit(ps);
        return ret;
    }

    thread->cond_fn  = XOS_NULL;
    thread->cond_arg = XOS_NULL;

    retval = xos_block(xos_blkon_condition, &cond->queue, 0, 1);
    ret = xos_mutex_lock(mutex);
    if (ret != XOS_OK) {
        xos_fatal_error(ret, NULL);
    }

    xos_critical_exit(ps);

    // Can't return an error code here since all possible return values
    // could be returned by the wakeup.
    return retval;
}


//-----------------------------------------------------------------------------
//  Timer callback to handle wait timeouts.
//-----------------------------------------------------------------------------
#if XOS_OPT_WAIT_TIMEOUT
static void
cond_wakeup_cb(void * arg)
{
    XosThread * thread = xos_voidp_to_threadp(arg);

    XOS_ASSERT(thread != XOS_NULL);

    // If the thread is already unblocked or blocked on something else
    // that will be handled inside xos_thread_wake().
    xos_thread_wake(thread, xos_blkon_condition, XOS_ERR_TIMEOUT);
}   
#endif


//-----------------------------------------------------------------------------
// Atomically release mutex and wait on condition until it is satisfied or the
// wait times out.
//-----------------------------------------------------------------------------
int32_t
xos_cond_wait_mutex_timeout(XosCond * cond, XosMutex * mutex, uint64_t to_cycles)
{
    XosThread * thread = xos_curr_threadptr;
    int32_t     retval = XOS_OK;
    int32_t     ret;
    uint32_t    ps;
#if XOS_OPT_WAIT_TIMEOUT
    XosTimer    timer;
#endif

    // Can't call this from interrupt context.
    if (INTERRUPT_CONTEXT) {
        xos_fatal_error(XOS_ERR_INTERRUPT_CONTEXT, XOS_NULL);
    }
    
    if ((cond == XOS_NULL) || (mutex == XOS_NULL)) {
        return XOS_ERR_INVALID_PARAMETER;
    }
    if (cond->sig != XOS_COND_SIG) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    // Mutex validity and ownership will be checked by the unlock call.

    // Enter a critical section and also lock the scheduler because we
    // don't want a context switch to happen between unlocking the mutex
    // and blocking on the condition.

    ps = xos_critical_enter();
    (void) xos_preemption_disable();

    // Unlock the scheduler but don't actually run the scheduler yet.
    // That will happen when this thread blocks.

    ret = xos_mutex_unlock(mutex);
    (void) xos_preemption_enable_ns();

    // Two possible errors: mutex is invalid or is not locked by caller.
    if (ret != XOS_OK) {
        xos_critical_exit(ps);
        return ret;
    }

    thread->cond_fn  = XOS_NULL;
    thread->cond_arg = XOS_NULL;

#if XOS_OPT_WAIT_TIMEOUT
    // If specified and enabled, set up a timeout for the wait.
    (void) xos_timer_init(&timer);
    if (to_cycles > 0ULL) {
        retval = xos_timer_start(&timer,
                                 to_cycles,
                                 XOS_TIMER_DELTA | XOS_TIMER_FROM_NOW,
                                 &cond_wakeup_cb,
                                 xos_curr_threadptr);
    }
#else
    UNUSED(to_cycles);
#endif

    if (retval == XOS_OK) {
        retval = xos_block(xos_blkon_condition, &cond->queue, 0, 1);
    }
#if XOS_OPT_WAIT_TIMEOUT
    (void) xos_timer_stop(&timer);
#endif
    ret = xos_mutex_lock(mutex);
    if (ret != XOS_OK) {
        xos_fatal_error(ret, NULL);
    }

    xos_critical_exit(ps);

    // Can't return an error code here since all possible return values
    // could be returned by the wakeup.
    return retval;
}


//-----------------------------------------------------------------------------
// (Internal) Trigger the given condition and wake up one or more threads.
//-----------------------------------------------------------------------------
int32_t
xos_cond_wakeup_(XosCond * cond, int32_t sig_value, bool single)
{
    XosThread * waiter;
    XosThread * next;
    int32_t     num_woken = 0;
    uint32_t    ps;

    if ((cond == XOS_NULL) || (cond->sig != XOS_COND_SIG)) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    ps = xos_critical_enter();

    // We don't want a higher priority thread waking up and preempting
    // us before we are done, so disable preemption here.

    (void) xos_preemption_disable();

    waiter = xos_q_peek(&cond->queue);
    while (waiter != XOS_NULL) {
        next = waiter->r_next;
        if ((waiter->cond_fn != XOS_NULL) &&
            ((*waiter->cond_fn)(waiter->cond_arg, sig_value, waiter) == 0)) {
            // Do nothing
        }
        else {
            xos_q_remove(&cond->queue, waiter);
            xos_thread_wake(waiter, xos_blkon_condition, sig_value);
            num_woken++;
        }

        if (single && (num_woken == 1)) {
            break;
        }

        waiter = next;
    }

    xos_critical_exit(ps);
    // Re-enabling may cause an immediate context switch.
    (void) xos_preemption_enable();

    return num_woken;
}


//-----------------------------------------------------------------------------
//  User-mode support.
//-----------------------------------------------------------------------------

#if XOS_OPT_UM

extern XosUCond xos_um_conds[XOS_UM_NUM_COND];

//-----------------------------------------------------------------------------
//  Search condition table for a match. The match for a key value of zero will
//  be the first free entry (if any).
//-----------------------------------------------------------------------------
static XosUCond *
xos_ucond_find(uint64_t key)
{
    uint32_t i;

    for (i = 0; i < XOS_UM_NUM_COND; i++) {
        if (xos_um_conds[i].key == key) {
            return &(xos_um_conds[i]);
        }
    }

    return XOS_NULL;
}


//-----------------------------------------------------------------------------
//  Create user-mode condition. Or find and return existing one if key matches.
//-----------------------------------------------------------------------------
int32_t
xos_ucond_create_p(uint64_t     key,
                   XosUCond **  ucond)
{
    XosUCond * c;
    int32_t    ret;
    uint32_t   ps;

    if (ucond == XOS_NULL) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    ps = xos_critical_enter();

    if (key != XOS_KEY_ZERO) {
        c = xos_ucond_find(key);
        if (c != XOS_NULL) {
            c->refs++;
            *ucond = c;
            xos_critical_exit(ps);
            return XOS_OK;
        }
    }

    c = xos_ucond_find(XOS_KEY_ZERO);
    if (c == XOS_NULL) {
        ret = XOS_ERR_LIMIT;
    }
    else {
        ret = xos_cond_create(&(c->cond));
        if (ret == XOS_OK) {
            c->key  = (key == XOS_KEY_ZERO) ? (uint64_t) c : key;
            c->refs = 1;
            *ucond  = c;
        }
    }

    xos_critical_exit(ps);
    return ret;
}


//-----------------------------------------------------------------------------
//  Destroy user-mode condition.
//-----------------------------------------------------------------------------
int32_t
xos_ucond_delete_p(XosUCond * ucond)
{
    uint32_t i;
    int32_t  ret;
    uint32_t ps;

    if (ucond == XOS_NULL) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    ps = xos_critical_enter();

    // Delete if found in condition table.
    ret = XOS_ERR_NOT_FOUND;
    for (i = 0; i < XOS_UM_NUM_COND; i++) {
        XosUCond * c = xos_um_conds + i;

        if (c == ucond) {
            XOS_ASSERT(c->refs > 0);
            if (c->refs > 0) {
                c->refs--;
            }
            if (c->refs == 0) {
                ret = xos_cond_delete(&(c->cond));
                if (ret == XOS_OK) {
                    c->key = XOS_KEY_ZERO;
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
//  Return pointer to condition given pointer to user-mode condition.
//-----------------------------------------------------------------------------
XosCond *
xos_ucond_cond(XosUCond * ucond)
{
    if ((ucond != XOS_NULL) && (ucond->cond.sig == XOS_COND_SIG)) {
        // Range check - must lie within the array of user-mode conditions.
        if (ucond < xos_um_conds) {
            return XOS_NULL;
        }
        if (ucond > (xos_um_conds + XOS_UM_NUM_COND - 1)) {
            return XOS_NULL;
        }
        return &(ucond->cond);
    }

    return XOS_NULL;
}


#endif // XOS_OPT_UM

