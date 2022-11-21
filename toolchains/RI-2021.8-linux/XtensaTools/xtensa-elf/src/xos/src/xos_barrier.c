
// xos_barrier.c - Barrier API.

// Copyright (c) 2018-2020 Cadence Design Systems, Inc.
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


#define XOS_BARR_SIG    0x62617272U     // Signature of valid object


//-----------------------------------------------------------------------------
// Initialize a barrier object before first use.
//-----------------------------------------------------------------------------
int32_t
xos_barrier_create(XosBarrier * bar, uint32_t count)
{
    if ((bar != XOS_NULL) && (count > 0U)) {
        int32_t ret = xos_sem_create(&(bar->sem), XOS_SEM_WAIT_PRIORITY, 0);

        if (ret != XOS_OK) {
            return ret;
        }

        bar->cnt = count;
        bar->rem = count;
        bar->sig = XOS_BARR_SIG;
        return XOS_OK;
    }

    return XOS_ERR_INVALID_PARAMETER;
}


//-----------------------------------------------------------------------------
// Destroy a barrier object. Release any waiting threads.
//-----------------------------------------------------------------------------
int32_t
xos_barrier_delete(XosBarrier * bar)
{
    if ((bar == XOS_NULL) || (bar->sig != XOS_BARR_SIG)) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    // This releases any waiting threads. Nothing useful to be done
    // with the return code.
    (void) xos_sem_delete(&(bar->sem));
    bar->cnt = 0U;
    bar->rem = 0U;
    bar->sig = 0U;
    return XOS_OK;
}


//-----------------------------------------------------------------------------
// Wait on a barrier.
//-----------------------------------------------------------------------------
int32_t
xos_barrier_wait(XosBarrier * bar)
{
    uint32_t old_rem;

    // Can't call this from interrupt context.
    if (INTERRUPT_CONTEXT) {
        xos_fatal_error(XOS_ERR_INTERRUPT_CONTEXT, XOS_NULL);
    }

    if ((bar == XOS_NULL) || (bar->sig != XOS_BARR_SIG)) {
        return XOS_ERR_INVALID_PARAMETER;
    }

#if XOS_OPT_C11_ATOMICS
    old_rem = atomic_fetch_sub(&(bar->rem), 1U);
#else
    {
        uint32_t ps = xos_critical_enter();

        old_rem = bar->rem;
        bar->rem--;
        xos_critical_exit(ps);
    }
#endif

    if (old_rem == 1U) {
        // This thread is the last one to arrive at the barrier. Clear it
        // and wake all threads. We don't want a context switch before we
        // finish waking all the threads so disable preemption.
        uint32_t x;

        (void) xos_preemption_disable();

        for (x = 1; x < bar->cnt; x++) {
            if (xos_sem_put(&(bar->sem)) != XOS_OK) {
                xos_fatal_error(XOS_ERR_INTERNAL_ERROR, XOS_NULL);
            }
        }
        bar->cnt = 0U;

        // May cause immediate context switch.
        (void) xos_preemption_enable();
        return XOS_OK;
    }

    if ((old_rem == 0U) || (old_rem > bar->cnt)) {
        // Too many threads have tried to block at the barrier.
        xos_fatal_error(XOS_ERR_LIMIT, XOS_NULL);
    }

    // Must wait here.
    return xos_sem_get(&(bar->sem));
}

