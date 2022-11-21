
// xos_event.c - XOS Event API interface and data structures.

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


//-----------------------------------------------------------------------------
// Internal defines.
//-----------------------------------------------------------------------------
#define XOS_EVENT_SIG           0x65766e74U // Valid object signature


//-----------------------------------------------------------------------------
// Type conversion helpers.
//-----------------------------------------------------------------------------
__attribute__((always_inline))
static inline uint32_t
xos_eventp_to_uint32(XosEvent * arg)
{
    return (uint32_t) arg; // parasoft-suppress MISRAC2012-RULE_11_4-a-4 "Type conversion checked"
}


//-----------------------------------------------------------------------------
//  Initialize an event object before first use.
//-----------------------------------------------------------------------------
int32_t
xos_event_create(XosEvent * event, uint32_t mask, uint16_t flags)
{
    if (event == XOS_NULL) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    event->bits       = 0;
    event->mask       = mask;
    event->flags      = flags;
    event->waitq.head = XOS_NULL;
    event->waitq.tail = &(event->waitq.head);
    event->sig        = XOS_EVENT_SIG;

    return XOS_OK;
}


//-----------------------------------------------------------------------------
//  Destroy an event object.
//-----------------------------------------------------------------------------
int32_t
xos_event_delete(XosEvent * event)
{
    bool     flag = false;
    uint32_t ps;

    if ((event == XOS_NULL) || (event->sig != XOS_EVENT_SIG)) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    // Make sure no one else can change object state
    ps = xos_critical_enter();

    if (xos_q_peek(&event->waitq) != XOS_NULL) {
        // Release waiting threads
        XosThread * waiter;

        // Don't want unexpected context switching to happen here
        (void) xos_preemption_disable();
        flag = true;

        while (xos_q_peek(&event->waitq) != XOS_NULL) {
            waiter = xos_q_pop(&event->waitq);
            xos_thread_wake(waiter, xos_blkon_event, XOS_ERR_EVENT_DELETE);
        }
    }

    // Must do this with the object protected against updates
    event->bits       = 0;
    event->mask       = 0;
    event->flags      = 0;
    event->waitq.head = XOS_NULL;
    event->waitq.tail = XOS_NULL;
    event->sig        = 0;

    xos_critical_exit(ps);
    if (flag) {
        // May cause immediate context switch
        (void) xos_preemption_enable();
    }

    return XOS_OK;
}


//-----------------------------------------------------------------------------
//  Update event flags for all waiting threads, and wake the ones that have had
//  their wait condition satisfied.
//-----------------------------------------------------------------------------
static void
xos_event_update_and_wake(XosEvent * event)
{
    XosThread * waiter;
    XosThread * next;
    bool        wake;
    uint32_t    cbits = 0;

    XOS_ASSERT(event != XOS_NULL);

    // Don't want a context switch until we're done processing.
    (void) xos_preemption_disable();

    waiter = xos_q_peek(&event->waitq);
    while (waiter != XOS_NULL) {
        next = waiter->r_next;
        waiter->event_bits = event->bits;
        wake = false;

        if ((waiter->event_flags & XOS_EVENT_WAIT_ALL) != 0U) {
            if ((waiter->event_bits & waiter->event_mask) == waiter->event_mask) {
                wake = true;
            }
        }
        else {
            if ((waiter->event_bits & waiter->event_mask) != 0U) {
                wake = true;
            }
        }

        if (wake) {
            // Keep track of which bits to auto-clear
            cbits |= waiter->event_bits & waiter->event_mask;
            xos_q_remove(&event->waitq, waiter);
            xos_thread_wake(waiter, xos_blkon_event, XOS_OK);
        }

        waiter = next;
    }

    // If the event object is auto-clear, then clear all the bits
    // that caused a thread wakeup. This function has been called
    // inside a critical section so we need not fear interrupts.
    if ((event->flags & XOS_EVENT_AUTO_CLEAR) != 0U) {
        event->bits &= ~cbits;
    }

    (void) xos_preemption_enable();
}


//-----------------------------------------------------------------------------
//  Set the specified bits in the specified event, update connected threads.
//-----------------------------------------------------------------------------
int32_t
xos_event_set(XosEvent * event, uint32_t bits)
{
    uint32_t ps;

    if ((event == XOS_NULL) || (event->sig != XOS_EVENT_SIG)) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    ps = xos_critical_enter();

    event->bits |= bits & event->mask;
    xos_log_sysevent(XOS_SE_EVENT_SET,
                     xos_eventp_to_uint32(event),
                     (uint32_t) event->bits);
    xos_event_update_and_wake(event);

    xos_critical_exit(ps);

    return XOS_OK;
}


//-----------------------------------------------------------------------------
//  Clear the specified bits in the specified event. Clearing bits will never
//  wake anything, so no need to update threads.
//-----------------------------------------------------------------------------
int32_t
xos_event_clear(XosEvent * event, uint32_t bits)
{
    uint32_t ps;

    if ((event == XOS_NULL) || (event->sig != XOS_EVENT_SIG)) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    ps = xos_critical_enter();

    event->bits &= ~bits;
    xos_log_sysevent(XOS_SE_EVENT_CLEAR,
                     xos_eventp_to_uint32(event),
                     (uint32_t) event->bits);

    xos_critical_exit(ps);

    return XOS_OK;
}


//-----------------------------------------------------------------------------
//  Clear and set the specified bits in the specified event.
//  Update connected threads.
//-----------------------------------------------------------------------------
int32_t
xos_event_clear_and_set(XosEvent * event, uint32_t clr_bits, uint32_t set_bits)
{
    uint32_t ps;

    if ((event == XOS_NULL) || (event->sig != XOS_EVENT_SIG)) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    ps = xos_critical_enter();

    event->bits &= ~clr_bits;
    event->bits |= set_bits & event->mask;
    xos_log_sysevent(XOS_SE_EVENT_CLEAR_SET,
                     xos_eventp_to_uint32(event),
                     (uint32_t) event->bits);
    xos_event_update_and_wake(event);

    xos_critical_exit(ps);

    return XOS_OK;
}


//-----------------------------------------------------------------------------
//  Get the current state of the event object.
//-----------------------------------------------------------------------------
int32_t
xos_event_get(const XosEvent * event, uint32_t * pstate)
{
    if ((event == XOS_NULL) || (event->sig != XOS_EVENT_SIG)) {
        return XOS_ERR_INVALID_PARAMETER;
    }
    if (pstate == XOS_NULL) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    *pstate = event->bits;
    return XOS_OK;
}


//-----------------------------------------------------------------------------
//  Timer callback to handle wait timeouts.
//-----------------------------------------------------------------------------
#if XOS_OPT_WAIT_TIMEOUT
static void
event_wakeup_cb(void * arg)
{
    XosThread * thread = xos_voidp_to_threadp(arg);

    XOS_ASSERT(thread != XOS_NULL);

    // If the thread is already unblocked or blocked on something else
    // that will be handled inside xos_thread_wake().
    xos_thread_wake(thread, xos_blkon_event, XOS_ERR_TIMEOUT);
}
#endif


//-----------------------------------------------------------------------------
//  Wait until all the specified bits in the wait mask become set.
//-----------------------------------------------------------------------------
int32_t
xos_event_wait_all(XosEvent * event, uint32_t bits)
{
    XosThread * thread = xos_thread_id();
    int32_t     ret;
    uint32_t    ps;

    // Can't call this from interrupt context.
    if (INTERRUPT_CONTEXT) {
        xos_fatal_error(XOS_ERR_INTERRUPT_CONTEXT, XOS_NULL);
    }

    if ((event == XOS_NULL) || (event->sig != XOS_EVENT_SIG) || (bits == 0U)) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    ps = xos_critical_enter();

    if ((event->bits & bits) == bits) {
        // Wait satisfied
        thread->event_bits = event->bits;
        // Auto-clear if specified
        if ((event->flags & XOS_EVENT_AUTO_CLEAR) != 0U) {
            event->bits &= ~bits;
        }
        xos_critical_exit(ps);
        return XOS_OK;
    }

    // Must block
    thread->event_mask = bits;
    thread->event_flags = XOS_EVENT_WAIT_ALL;
    ret = xos_block(xos_blkon_event, &event->waitq, 0, 0);

    xos_critical_exit(ps);
    return ret;
}


//-----------------------------------------------------------------------------
//  Wait until all the specified bits in the wait mask become set, or the 
//  timeout expires.
//-----------------------------------------------------------------------------
int32_t
xos_event_wait_all_timeout(XosEvent * event, uint32_t bits, uint64_t to_cycles)
{
    XosThread * thread = xos_thread_id();
    int32_t     ret;
    uint32_t    ps;

#if XOS_OPT_WAIT_TIMEOUT
    XosTimer    timer;
#endif

    // Can't call this from interrupt context.
    if (INTERRUPT_CONTEXT) {
        xos_fatal_error(XOS_ERR_INTERRUPT_CONTEXT, XOS_NULL);
    }

    if ((event == XOS_NULL) || (event->sig != XOS_EVENT_SIG) || (bits == 0U)) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    ps = xos_critical_enter();

    if ((event->bits & bits) == bits) {
        // Wait satisfied
        thread->event_bits = event->bits;
        // Auto-clear if specified
        if ((event->flags & XOS_EVENT_AUTO_CLEAR) != 0U) {
            event->bits &= ~bits;
        }
        xos_critical_exit(ps);
        return XOS_OK;
    }

    // Must block, set timeout if specified
#if XOS_OPT_WAIT_TIMEOUT
    (void) xos_timer_init(&timer);
    if (to_cycles > 0ULL) {
        ret = xos_timer_start(&timer,
                              to_cycles,
                              XOS_TIMER_DELTA | XOS_TIMER_FROM_NOW,
                              &event_wakeup_cb,
                              xos_curr_threadptr);
        if (ret != XOS_OK) {
            xos_critical_exit(ps);
            return ret;
        }
    }
#else
    UNUSED(to_cycles);
#endif

    thread->event_mask = bits;
    thread->event_flags = XOS_EVENT_WAIT_ALL;
    ret = xos_block(xos_blkon_event, &event->waitq, 0, 0);

#if XOS_OPT_WAIT_TIMEOUT
    if (to_cycles > 0ULL) {
        (void) xos_timer_stop(&timer);
    }
#endif

    xos_critical_exit(ps);
    return ret;
}


//-----------------------------------------------------------------------------
//  Wait until any of the specified bits in the wait mask become set.
//-----------------------------------------------------------------------------
int32_t
xos_event_wait_any(XosEvent * event, uint32_t bits)
{
    XosThread * thread = xos_thread_id();
    int32_t     ret;
    uint32_t    ps;

    // Can't call this from interrupt context.
    if (INTERRUPT_CONTEXT) {
        xos_fatal_error(XOS_ERR_INTERRUPT_CONTEXT, XOS_NULL);
    }

    if ((event == XOS_NULL) || (event->sig != XOS_EVENT_SIG) || (bits == 0U)) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    ps = xos_critical_enter();

    if ((event->bits & bits) != 0U) {
        // Wait satisfied
        thread->event_bits = event->bits;
        // Auto-clear if specified
        if ((event->flags & XOS_EVENT_AUTO_CLEAR) != 0U) {
            event->bits &= ~(event->bits & bits);
        }
        xos_critical_exit(ps);
        return XOS_OK;
    }

    // Must block
    thread->event_mask  = bits;
    thread->event_flags = XOS_EVENT_WAIT_ANY;
    ret = xos_block(xos_blkon_event, &event->waitq, 0, 0);

    xos_critical_exit(ps);
    return ret;
}


//-----------------------------------------------------------------------------
//  Wait until any of the specified bits in the wait mask become set, or the
//  timeout expires.
//-----------------------------------------------------------------------------
int32_t
xos_event_wait_any_timeout(XosEvent * event, uint32_t bits, uint64_t to_cycles)
{
    XosThread * thread = xos_thread_id();
    int32_t     ret;
    uint32_t    ps;

#if XOS_OPT_WAIT_TIMEOUT
    XosTimer    timer;
#endif

    // Can't call this from interrupt context.
    if (INTERRUPT_CONTEXT) {
        xos_fatal_error(XOS_ERR_INTERRUPT_CONTEXT, XOS_NULL);
    }

    if ((event == XOS_NULL) || (event->sig != XOS_EVENT_SIG) || (bits == 0U)) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    ps = xos_critical_enter();

    if ((event->bits & bits) != 0U) {
        // Wait satisfied
        thread->event_bits = event->bits;
        // Auto-clear if specified
        if ((event->flags & XOS_EVENT_AUTO_CLEAR) != 0U) {
            event->bits &= ~(event->bits & bits);
        }
        xos_critical_exit(ps);
        return XOS_OK;
    }

    // Must block, set timeout if specified
#if XOS_OPT_WAIT_TIMEOUT
    (void) xos_timer_init(&timer);
    if (to_cycles > 0ULL) {
        ret = xos_timer_start(&timer,
                              to_cycles,
                              XOS_TIMER_DELTA | XOS_TIMER_FROM_NOW,
                              &event_wakeup_cb,
                              xos_curr_threadptr);
        if (ret != XOS_OK) {
            xos_critical_exit(ps);
            return ret;
        }
    }
#else
    UNUSED(to_cycles);
#endif

    thread->event_mask = bits;
    thread->event_flags = XOS_EVENT_WAIT_ANY;
    ret = xos_block(xos_blkon_event, &event->waitq, 0, 0);

#if XOS_OPT_WAIT_TIMEOUT
    if (to_cycles > 0ULL) {
        (void) xos_timer_stop(&timer);
    }
#endif

    xos_critical_exit(ps);
    return ret;
}


//-----------------------------------------------------------------------------
//  Set some bits and then wait for another group of bits.
//-----------------------------------------------------------------------------
int32_t
xos_event_set_and_wait(XosEvent * event, uint32_t set_bits, uint32_t wait_bits)
{
    uint32_t ps;
    int32_t  ret;

    // Can't call this from interrupt context.
    if (INTERRUPT_CONTEXT) {
        xos_fatal_error(XOS_ERR_INTERRUPT_CONTEXT, XOS_NULL);
    }

    if ((event == XOS_NULL) || (event->sig != XOS_EVENT_SIG) || (wait_bits == 0U)) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    ps = xos_critical_enter();

    // Everything that this function would check is already checked.
    (void) xos_event_set(event, set_bits);
    ret = xos_event_wait_all(event, wait_bits);

    xos_critical_exit(ps);
    return ret; 
}


//-----------------------------------------------------------------------------
//  Clear some bits and then wait for another group of bits.
//-----------------------------------------------------------------------------
int32_t
xos_event_clear_and_wait(XosEvent * event, uint32_t clear_bits, uint32_t wait_bits)
{
    uint32_t ps;
    int32_t  ret;

    // Can't call this from interrupt context.
    if (INTERRUPT_CONTEXT) {
        xos_fatal_error(XOS_ERR_INTERRUPT_CONTEXT, XOS_NULL);
    }

    if ((event == XOS_NULL) || (event->sig != XOS_EVENT_SIG) || (wait_bits == 0U)) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    ps = xos_critical_enter();

    // Everything that this function would check is already checked.
    (void) xos_event_clear(event, clear_bits);
    ret = xos_event_wait_all(event, wait_bits);

    xos_critical_exit(ps);
    return ret;
}


//-----------------------------------------------------------------------------
//  Return event given its contained wait queue (if valid object).
//-----------------------------------------------------------------------------
XosEvent *
xos_eventq_to_event(XosThreadQueue * q)
{
    XosEvent * event;

    if (q != XOS_NULL) {
        event = (XosEvent *)(xos_voidp_to_uint32(q) - offsetof(XosEvent, waitq));
        if (event->sig == XOS_EVENT_SIG) {
            return event;
        }
    }

    return NULL;
}

