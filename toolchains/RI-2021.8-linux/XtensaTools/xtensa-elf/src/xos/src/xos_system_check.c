
// xos_system_check.c - XOS runtime integrity checking.

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
//  Check thread state and return an error if a problem is detected.
//-----------------------------------------------------------------------------
static int32_t
xos_thread_check_int(XosThread * thread)
{
    XOS_ASSERT((thread != XOS_NULL) && (thread->sig == XOS_THREAD_SIG));

    // If the thread is blocked on a mutex, the mutex must be locked and
    // the owner must be a valid thread.
    if (thread->block_cause == xos_blkon_mutex) {
        XosMutex *         mtx = xos_mutexq_to_mutex(thread->wq_ptr);
        xos_thread_state_t state;

        if (mtx == XOS_NULL) {
            return XOS_ERR_INTERNAL_ERROR;
        }
        if ((mtx->lock_count == 0) || (mtx->owner == XOS_NULL)) {
            DPRINTF("(%s) blocked on free mutex %p\n", thread->name, mtx);
            return XOS_ERR_SC_MTX_NOT_LOCKED;
        }

        state = xos_thread_get_state(mtx->owner);
        if ((state == XOS_THREAD_STATE_EXITED) || (state == XOS_THREAD_STATE_INVALID)) {
            DPRINTF("Mutex %p owner (%s) in bad/exited state\n", mtx, thread->name);
            return XOS_ERR_SC_MTX_BAD_OWNER;
        }

        // The mutex owner thread must not be blocked waiting for a mutex
        // which in turn is owned by the thread being checked.
        if (mtx->owner->block_cause == xos_blkon_mutex) {
            XosMutex * mtx2 = xos_mutexq_to_mutex(mtx->owner->wq_ptr);

            if (mtx2 == XOS_NULL) {
                return XOS_ERR_INTERNAL_ERROR;
            }
            if (mtx2->owner == thread) {
                DPRINTF("Deadlock detected (%s) <=> (%s)\n", thread->name, mtx->owner->name);
                return XOS_ERR_SC_MTX_DEADLOCK;
            }
        }
    }

    // If the thread is blocked on a semaphore, the semaphore count must be zero.
    if (thread->block_cause == xos_blkon_sem) {
        XosSem * sem = xos_semq_to_sem(thread->wq_ptr);

        if (sem == XOS_NULL) {
            return XOS_ERR_INTERNAL_ERROR;
        }
        if (sem->count != 0) {
            DPRINTF("(%s) blocked on sem %p count %u\n", thread->name, sem, sem->count);
            return XOS_ERR_SC_SEM_COUNT;
        }
    }

    // If the thread is blocked on an event, then the event bits must not match.
    if (thread->block_cause == xos_blkon_event) {
        XosEvent * event = xos_eventq_to_event(thread->wq_ptr);

        if (event == XOS_NULL) {
            return XOS_ERR_INTERNAL_ERROR;
        }
        if ((thread->event_flags & XOS_EVENT_WAIT_ALL) != 0U) {
            if ((thread->event_mask & event->mask) == thread->event_mask) {
                return XOS_ERR_SC_EVT_BITS;
            }
        }
        else {
            if ((thread->event_mask & event->mask) != 0U) {
                return XOS_ERR_SC_EVT_BITS;
            }
        }
    }

    return XOS_OK;
}


//-----------------------------------------------------------------------------
//  Check thread state and return an error if a problem is detected.
//-----------------------------------------------------------------------------
int32_t
xos_thread_check(XosThread * thread)
{
    int32_t  ret;
    uint32_t ps;

    if ((thread == XOS_NULL) || (thread->sig != XOS_THREAD_SIG)) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    ps  = xos_critical_enter();
    ret = xos_thread_check_int(thread);
    xos_critical_exit(ps);

    return ret;
}


//-----------------------------------------------------------------------------
//  Run system consistency checks.
//-----------------------------------------------------------------------------
int32_t
xos_system_check(XosThread ** ppthread, bool disable_p)
{
    XosThread * thread;
    uint32_t    ps;
    int32_t     ret = XOS_OK;

    if (disable_p) {
        (void) xos_preemption_disable();
    }

    for (thread = xos_all_threads; thread != XOS_NULL; thread = thread->all_next) {
        ps = xos_critical_enter();
        if (thread->ready) {
            ret = XOS_OK;
        }
        else {
            ret = xos_thread_check_int(thread);
        }
        xos_critical_exit(ps);

        if (ret != XOS_OK) {
            if (ppthread != XOS_NULL) {
                *ppthread = thread;
            }
            break;
        }
    }

    if (disable_p) {
        (void) xos_preemption_enable();
    }

    return ret;
}

