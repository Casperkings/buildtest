
// xos_display.c - XOS helper routines to display data structures etc.

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


#if XOS_OPT_STACK_CHECK
#define HD1    "    Stack Used   "
#define LN1    "  ---------------"
#else
#define HD1    ""
#define LN1    ""
#endif

#if XOS_OPT_STATS
#define HD2    "   Cycles     CtxSw"
#define LN2    " -----------  -----"
#else
#define HD2    ""
#define LN2    ""
#endif


// parasoft-begin-suppress MISRAC2012-RULE_17_7-a-2 "Not useful to check ret value of print function"

//-----------------------------------------------------------------------------
//  List all threads in the system. Includes threads that have exited but not
//  been destroyed yet.
//
//  arg --      Parameter passed on to the print function.
//
//  print_fn -- Pointer to function that does the actual printing (or whatever
//              else it wants to do).
//
//  Returns: Nothing.
//-----------------------------------------------------------------------------
void
xos_display_threads(void * arg, XosPrintFunc * print_fn)
{
    XosThread *    thread;
#if XOS_OPT_STATS
    XosThreadStats stats;
#endif
    int32_t  n  = 0;

    if (print_fn == XOS_NULL) {
        return;
    }

    (void) xos_preemption_disable();

    (*print_fn)(arg, "\nTHREAD TCB   Name            Pri     State "HD1" "HD2"\n");
    (*print_fn)(arg,   "---------- ---------------- ------  -------"LN1" "LN2"\n");

    for (thread = xos_all_threads; thread != XOS_NULL; thread = thread->all_next) {
        (*print_fn)(arg, "0x%8x %-16s %2d(%2d) %8s",
                thread,
                thread->name,
                thread->curr_priority,
                thread->base_priority,
                thread->ready ?
                    ((thread == xos_curr_threadptr) ? "running" : "ready") : thread->block_cause);

#if XOS_OPT_STACK_CHECK
        // See how deep the stack seems to have went, according to pattern saved in stack:
        {
            uint32_t stack_size;
            uint32_t percent_use;
            uint32_t * s;
            uint32_t * stack_base = xos_voidp_to_uint32p(thread->stack_base);
            uint32_t * stack_end  = xos_voidp_to_uint32p(thread->stack_end);

            for (s = stack_base; s < stack_end; s++) {
                if (*s != XOS_STACK_CHECKVAL) {
                    break;
                }
            }

            // Assume stack always smaller than 20 MB (2GB / 100):
            stack_size = (uint32_t)(stack_end - stack_base) * sizeof(uint32_t);
            if (stack_size == 0U) {
                (*print_fn)(arg, "  ---            ");
            }
            else {
                percent_use = (((uint32_t)(stack_end - s) * sizeof(uint32_t) * 100U) + (stack_size/2U)) / stack_size;
                (*print_fn)(arg, " %3d%% (of %5ub)", percent_use, stack_size);
            }
        }
#endif

#if XOS_OPT_STATS
        if (xos_thread_get_stats(thread, &stats) == XOS_OK) {
            (*print_fn)(arg, "  %11ld  %d/%d",
                (uint32_t)stats.cycle_count, stats.normal_switches, stats.preempt_switches);
        }
        else {
            (*print_fn)(arg, "<not available>");
        }
#endif

        (*print_fn)(arg, "\n");
        n++;
    }

    (*print_fn)(arg, "(%d threads)\n", n);
    (void) xos_preemption_enable();
}

// parasoft-end-suppress MISRAC2012-RULE_17_7-a-2

