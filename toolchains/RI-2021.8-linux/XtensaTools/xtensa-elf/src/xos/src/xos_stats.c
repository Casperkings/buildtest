
// xos_stats.c - XOS Statistics functions.

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
// Defines and macros.
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// External references.
//-----------------------------------------------------------------------------
#if XOS_OPT_STATS
extern uint32_t xos_inth_intr_count;  // parasoft-suppress MISRAC2012-RULE_8_6-a-2 "Defined in assembly code"
extern uint64_t xos_inth_cycle_count; // parasoft-suppress MISRAC2012-RULE_8_6-a-2 "Defined in assembly code"
#endif


//-----------------------------------------------------------------------------
//  Get the thread statistics for the specified thread.
//-----------------------------------------------------------------------------
int32_t
xos_thread_get_stats(const XosThread * thread, XosThreadStats * stats)
{
#if XOS_OPT_STATS

    const XosThread * th = thread;

    if ((th == XOS_NULL) || (stats == XOS_NULL)) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    // Special case for 'interrupt thread'
    if (th == XOS_THD_STATS_INTR) {
#if XCHAL_HAVE_INTERRUPTS
        stats->cycle_count      = xos_inth_cycle_count;
        stats->normal_switches  = xos_inth_intr_count;
#else
        stats->cycle_count      = 0;
        stats->normal_switches  = 0;
#endif
        stats->preempt_switches = 0;
        return XOS_OK;
    }

    // Special case for idle thread
    if (th == XOS_THD_STATS_IDLE) {
        th = &xos_idle_thread;
    }

    if (th->sig != XOS_THREAD_SIG) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    stats->normal_switches  = th->normal_resumes;
    stats->preempt_switches = th->preempt_resumes;
    stats->cycle_count      = th->cycle_count;

    // If querying the current active thread, account for the cycles
    // consumed since it was last resumed.
    if (th == XOS_THREAD_SELF) {
        stats->cycle_count += xos_get_ccount() - th->resume_ccount;
    }

#else

    UNUSED(thread);
    UNUSED(stats);

#endif

    return XOS_OK;
}


//-----------------------------------------------------------------------------
// Compute CPU loading data for each thread (and interrupt context).
// NOTE: This can take a while!
//-----------------------------------------------------------------------------
int32_t
xos_get_cpu_load(XosThreadStats * stats, int32_t * size, int32_t reset)
{
#if XOS_OPT_STATS

    uint32_t         ps;
    XosThreadStats * p;
    XosThread *      thread;
    int32_t          num_threads  = 0;
    uint64_t         total_cycles = 0;

    if ((stats == XOS_NULL) || (size == XOS_NULL) || (*size == 0)) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    // Make sure that the thread list doesn't change out under us.
    ps = xos_critical_enter();

    // Make sure the provided buffer is large enough.
    for (thread = xos_all_threads; thread != XOS_NULL; thread = thread->all_next) {
        num_threads++;
    }

    // Account for interrupt "thread".
    num_threads++;

    if (num_threads > *size) {
        // Buffer size too small, report how many entries needed.
        *size = num_threads;
        xos_critical_exit(ps);
        return XOS_ERR_INVALID_PARAMETER;
    }

    // Now run through the list again and gather data.
    p = stats;

    for (thread = xos_all_threads; thread != XOS_NULL; thread = thread->all_next) {
        p->thread           = thread;
        p->normal_switches  = thread->normal_resumes;
        p->preempt_switches = thread->preempt_resumes;
        p->cycle_count      = thread->cycle_count;

        // For the current thread, add in the cycles consumed from the last
        // time it was resumed.
        if (thread == xos_curr_threadptr) {
            p->cycle_count += (uint64_t) (xos_get_ccount() - thread->resume_ccount); // parasoft-suppress MISRAC2012-RULE_10_8-a-2 "Type conversion necessary here"
        }

        if (reset != 0) {
            thread->normal_resumes  = 0;
            thread->preempt_resumes = 0;
            thread->cycle_count     = 0;
        }

        total_cycles += p->cycle_count;
        p++;
    }

#if XCHAL_HAVE_INTERRUPTS

    // Account for interrupt processing.
    p->thread           = XOS_THD_STATS_INTR;
    p->normal_switches  = xos_inth_intr_count;
    p->preempt_switches = 0;
    p->cycle_count      = xos_inth_cycle_count;

    if (reset != 0) {
        xos_inth_intr_count  = 0;
        xos_inth_cycle_count = 0;
    }

#else

    p->thread           = XOS_THD_STATS_INTR;
    p->normal_switches  = 0;
    p->preempt_switches = 0;
    p->cycle_count      = 0;

#endif

    // Update total cycles.
    total_cycles += p->cycle_count;

    xos_critical_exit(ps);

    // Update the actual number of entries filled in.
    *size = num_threads;

    // Compute CPU use %.
    p = stats;
    for (; num_threads > 0; num_threads--) {
        p->cpu_pct = (uint32_t)((p->cycle_count * 100U) / total_cycles);
        p++;
    }

#else

    UNUSED(stats);
    UNUSED(reset);

    // Report zero entries
    if (size == XOS_NULL) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    *size = 0;

#endif

    return XOS_OK;
}

