
// xos_msgq.c - XOS Message Queue implementation.

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
//  Defines and macros.
//-----------------------------------------------------------------------------
#define XOS_MSGQ_SIG    0x4d534751U // Valid object signature


//-----------------------------------------------------------------------------
//  Initialize message queue.
//-----------------------------------------------------------------------------
int32_t
xos_msgq_create(XosMsgQueue * msgq, uint16_t num, uint32_t size, uint16_t flags)
{
    // Queue ptr cannot be null, message count cannot be zero, message size
    // must be nonzero and multiple of 4 bytes.

    if ((msgq == XOS_NULL) || (num == 0U) || (size == 0U) || ((size % 4U) != 0U)) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    msgq->flags = flags;
    msgq->count = num;
    msgq->msize = size;
    msgq->head  = 0;
    msgq->tail  = 0;
    msgq->sig   = XOS_MSGQ_SIG;

    msgq->readq.head   = XOS_NULL;
    msgq->readq.tail   = &(msgq->readq.head);
    msgq->readq.flags  = ((flags & XOS_MSGQ_WAIT_FIFO) != 0U) ? XOS_Q_WAIT_FIFO : XOS_Q_WAIT_PRIO;
    msgq->writeq.head  = XOS_NULL;
    msgq->writeq.tail  = &(msgq->writeq.head);
    msgq->writeq.flags = ((flags & XOS_MSGQ_WAIT_FIFO) != 0U) ? XOS_Q_WAIT_FIFO : XOS_Q_WAIT_PRIO;

#if XOS_OPT_STATS
    msgq->num_send = 0;
    msgq->num_recv = 0;
    msgq->num_send_blks = 0;
    msgq->num_recv_blks = 0;
#endif

    return XOS_OK;
}


//-----------------------------------------------------------------------------
//  Destroy a message queue. Release any waiting threads.
//-----------------------------------------------------------------------------
int32_t
xos_msgq_delete(XosMsgQueue * msgq)
{
    XosThread * waiter;
    uint32_t    ps;

    if ((msgq == XOS_NULL) || (msgq->sig != XOS_MSGQ_SIG)) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    // Disable interrupts to prevent any access to this queue.
    ps = xos_critical_enter();

    // Don't want unexpected context switching to happen here.
    (void) xos_preemption_disable();

    // Wake any waiting threads, readers or writers.
    while (xos_q_peek(&msgq->readq) != XOS_NULL) {
        waiter = xos_q_pop(&msgq->readq);
        xos_thread_wake(waiter, xos_blkon_msgq, XOS_ERR_MSGQ_DELETE);
    }

    while (xos_q_peek(&msgq->writeq) != XOS_NULL) {
        waiter = xos_q_pop(&msgq->writeq);
        xos_thread_wake(waiter, xos_blkon_msgq, XOS_ERR_MSGQ_DELETE);
    }

    // Clear queue state and mark as not for use.
    msgq->flags |= XOS_MSGQ_DELETED | XOS_MSGQ_FULL;
    msgq->count = 0;
    msgq->head  = 0;
    msgq->tail  = 0;
    msgq->sig   = 0;

    // May cause immediate context switch.
    xos_critical_exit(ps);
    (void) xos_preemption_enable();

    return XOS_OK;
}


//-----------------------------------------------------------------------------
//  Push a message into the queue and wake a reader if needed. This must not
//  be called directly from application code.
//  NOTE: Must be called with interrupts disabled.
//-----------------------------------------------------------------------------
static int32_t
xos_msgq_put_int(XosMsgQueue * msgq, const uint32_t * msg)
{
    int32_t ret;

    XOS_ASSERT(IN_CRITICAL_SECTION != 0);

    if ((msgq->flags & XOS_MSGQ_FULL) != 0U) {
        ret = XOS_ERR_MSGQ_FULL;
    }
    else {
        const uint32_t * src = msg;
        uint32_t * dst = &( msgq->msg[((msgq->head * msgq->msize) / 4U)] );
        uint32_t   cnt;

        for (cnt = 0; cnt < (msgq->msize / 4U); cnt++) {
            dst[cnt] = src[cnt];
        }

        msgq->head++;
        if (msgq->head >= msgq->count) {
            msgq->head = 0;
        }
        if (msgq->head == msgq->tail) {
            msgq->flags |= XOS_MSGQ_FULL;
        }
#if XOS_OPT_STATS
        msgq->num_send++;
#endif

        // If we have a read waiter then wake it. May cause immediate
        // context switch.
        if (msgq->readq.head != XOS_NULL) {
            xos_thread_wake(xos_q_pop(&msgq->readq), xos_blkon_msgq, XOS_OK);
        }

        ret = XOS_OK;
    }

    return ret;
}


//-----------------------------------------------------------------------------
//  Timer callback to handle wait timeouts.
//-----------------------------------------------------------------------------
#if XOS_OPT_WAIT_TIMEOUT
static void
msgq_wakeup_cb(void * arg)
{
    XosThread * thread = xos_voidp_to_threadp(arg);

    XOS_ASSERT(thread != XOS_NULL);

    // If the thread is already unblocked or blocked on something else
    // that will be handled inside xos_thread_wake().
    xos_thread_wake(thread, xos_blkon_msgq, XOS_ERR_TIMEOUT);
}
#endif


//-----------------------------------------------------------------------------
//  Put a message into the queue. If called from thread context, block until
//  space is available.
//-----------------------------------------------------------------------------
int32_t
xos_msgq_put(XosMsgQueue * msgq, const uint32_t * msg)
{
    int32_t  ret = XOS_OK;
    uint32_t ps;

    if ((msgq == XOS_NULL) || (msgq->sig != XOS_MSGQ_SIG) || (msg == XOS_NULL)) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    ps  = xos_critical_enter();
    ret = xos_msgq_put_int(msgq, msg);

    if ((INTERRUPT_CONTEXT) || (ret == XOS_OK)) {
        xos_critical_exit(ps);
        return ret;
    }

    while (ret == XOS_ERR_MSGQ_FULL) {
        ret = xos_block(xos_blkon_msgq,
                        &msgq->writeq,
                        0,
                        ((msgq->flags & XOS_MSGQ_WAIT_FIFO) != 0U) ? 0 : 1);
#if XOS_OPT_STATS
        msgq->num_send_blks++;
#endif

        if (ret != XOS_OK) {
            break;
        }

        // This exit/enter sequence allows interrupts to be processed.
        xos_critical_exit(ps);
        ps  = xos_critical_enter();
        ret = xos_msgq_put_int(msgq, msg);
    }

    xos_critical_exit(ps);
    return ret;
}


//-----------------------------------------------------------------------------
//  Put a message into the queue. If called from thread context, block until 
//  space is available or the timeout expires.
//-----------------------------------------------------------------------------
int32_t
xos_msgq_put_timeout(XosMsgQueue * msgq, const uint32_t * msg, uint64_t to_cycles)
{
    int32_t  ret = XOS_OK;
    uint32_t ps;

#if XOS_OPT_WAIT_TIMEOUT
    XosTimer timer;
    bool     timer_started = false;
#endif

    if ((msgq == XOS_NULL) || (msgq->sig != XOS_MSGQ_SIG) || (msg == XOS_NULL)) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    ps  = xos_critical_enter();
    ret = xos_msgq_put_int(msgq, msg);

    if ((INTERRUPT_CONTEXT) || (ret == XOS_OK)) {
        xos_critical_exit(ps);
        return ret;
    }

    while (ret == XOS_ERR_MSGQ_FULL) {
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
                                      &msgq_wakeup_cb,
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

        ret = xos_block(xos_blkon_msgq,
                        &msgq->writeq,
                        0,
                        ((msgq->flags & XOS_MSGQ_WAIT_FIFO) != 0U) ? 0 : 1);
#if XOS_OPT_STATS
        msgq->num_send_blks++;
#endif

        if (ret != XOS_OK) {
            break;
        }

        // This exit/enter sequence allows interrupts to be processed.
        xos_critical_exit(ps);
        ps  = xos_critical_enter();
        ret = xos_msgq_put_int(msgq, msg);
    }

#if XOS_OPT_WAIT_TIMEOUT
    if (timer_started) {
        (void) xos_timer_stop(&timer);
    }
#endif

    xos_critical_exit(ps);
    return ret;
}


//-----------------------------------------------------------------------------
//  Pull a message from the queue and wake a writer if needed. This must not
//  be called directly from application code.
//  NOTE: Must be called with interrupts disabled.
//-----------------------------------------------------------------------------
static int32_t
xos_msgq_get_int(XosMsgQueue * msgq, uint32_t * msg)
{
    int32_t ret;

    XOS_ASSERT(IN_CRITICAL_SECTION != 0);

    // Is queue empty ?
    if ((msgq->tail == msgq->head) && ((msgq->flags & XOS_MSGQ_FULL) == 0U)) {
        ret = XOS_ERR_MSGQ_EMPTY;
    }
    else {
        uint32_t * src = &( msgq->msg[((msgq->tail * msgq->msize) / 4U)] );
        uint32_t * dst = msg;
        uint32_t   cnt;

        for (cnt = 0; cnt < (msgq->msize / 4U); cnt++) {
            dst[cnt] = src[cnt];
        }

        msgq->tail++;
        if (msgq->tail >= msgq->count) {
            msgq->tail = 0;
        }
        msgq->flags &= ~XOS_MSGQ_FULL;
#if XOS_OPT_STATS
        msgq->num_recv++;
#endif

        // If we have a write waiter then wake it. May cause immediate
        // context switch.
        if (msgq->writeq.head != XOS_NULL) {
            xos_thread_wake(xos_q_pop(&msgq->writeq), xos_blkon_msgq, XOS_OK);
        }

        ret = XOS_OK;
    }

    return ret;
}


//-----------------------------------------------------------------------------
//  Get a message from the queue. If in thread context, block until a message
//  is available.
//-----------------------------------------------------------------------------
int32_t
xos_msgq_get(XosMsgQueue * msgq, uint32_t * msg)
{
    int32_t  ret = XOS_OK;
    uint32_t ps;

    if ((msgq == XOS_NULL) || (msgq->sig != XOS_MSGQ_SIG) || (msg == XOS_NULL)) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    ps  = xos_critical_enter();
    ret = xos_msgq_get_int(msgq, msg);

    if ((INTERRUPT_CONTEXT) || (ret == XOS_OK)) {
        xos_critical_exit(ps);
        return ret;
    }

    while (ret == XOS_ERR_MSGQ_EMPTY) {
        ret = xos_block(xos_blkon_msgq,
                        &msgq->readq,
                        0,
                        ((msgq->flags & XOS_MSGQ_WAIT_FIFO) != 0U) ? 0 : 1);
#if XOS_OPT_STATS
        msgq->num_recv_blks++;
#endif

        if (ret != XOS_OK) {
            break;
        }

        // This exit/enter sequence allows interrupts to be processed.
        xos_critical_exit(ps);
        ps  = xos_critical_enter();
        ret = xos_msgq_get_int(msgq, msg);
    }

    xos_critical_exit(ps);
    return ret;
}


//-----------------------------------------------------------------------------
//  Get a message from the queue. If in thread context, block until a message
//  is available or the timeout expires.
//-----------------------------------------------------------------------------
int32_t
xos_msgq_get_timeout(XosMsgQueue * msgq, uint32_t * msg, uint64_t to_cycles)
{
    int32_t  ret = XOS_OK;
    uint32_t ps;

#if XOS_OPT_WAIT_TIMEOUT
    XosTimer timer;
    bool     timer_started = false;
#endif

    if ((msgq == XOS_NULL) || (msgq->sig != XOS_MSGQ_SIG) || (msg == XOS_NULL)) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    ps  = xos_critical_enter();
    ret = xos_msgq_get_int(msgq, msg);

    if ((INTERRUPT_CONTEXT) || (ret == XOS_OK)) {
        xos_critical_exit(ps);
        return ret;
    }

    while (ret == XOS_ERR_MSGQ_EMPTY) {
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
                                      &msgq_wakeup_cb,
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

        ret = xos_block(xos_blkon_msgq,
                        &msgq->readq,
                        0,
                        ((msgq->flags & XOS_MSGQ_WAIT_FIFO) != 0U) ? 0 : 1);
#if XOS_OPT_STATS
        msgq->num_recv_blks++;
#endif

        if (ret != XOS_OK) {
            break;
        }

        // This exit/enter sequence allows interrupts to be processed.
        xos_critical_exit(ps);
        ps  = xos_critical_enter();
        ret = xos_msgq_get_int(msgq, msg);
    }

#if XOS_OPT_WAIT_TIMEOUT
    if (timer_started) {
        (void) xos_timer_stop(&timer);
    }
#endif

    xos_critical_exit(ps);
    return ret;
}


//-----------------------------------------------------------------------------
//  Check if queue is empty.
//-----------------------------------------------------------------------------
int32_t
xos_msgq_empty(const XosMsgQueue * msgq)
{
    int32_t ret = 1;

    if ((msgq != XOS_NULL) && (msgq->sig == XOS_MSGQ_SIG)) {
        if ((msgq->tail != msgq->head) || ((msgq->flags & XOS_MSGQ_FULL) != 0U)) {
            ret = 0;
        }
    }

    return ret;
}


//-----------------------------------------------------------------------------
//  Check if queue is full.
//-----------------------------------------------------------------------------
int32_t
xos_msgq_full(const XosMsgQueue * msgq)
{
    int32_t ret = 0;

    if ((msgq != XOS_NULL) && (msgq->sig == XOS_MSGQ_SIG)) {
        if ((msgq->flags & XOS_MSGQ_FULL) != 0U) {
            ret = 1;
        }
    }

    return ret;
}

