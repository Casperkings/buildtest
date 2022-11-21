
// clib_test.c - XOS C library context switch test

// Copyright (c) 2015 Cadence Design Systems, Inc.
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


#include <stdio.h>
#include <stdlib.h>
#include <xtensa/config/system.h>

#include "test.h"

#if XSHAL_CLIB == XTHAL_CLIB_XCLIB
#include <sys/reent.h>
#elif XSHAL_CLIB == XTHAL_CLIB_NEWLIB
#include <sys/reent.h>
#elif XSHAL_CLIB == XTHAL_CLIB_UCLIBC
#warning UCLIBC not supported -- this test will do nothing
#else
#error Unknown C runtime library
#endif

// This test only does something useful if thread safety is enabled.
#if XOS_OPT_THREAD_SAFE_CLIB

#define NUM_THREADS	20


// Runs multiple threads, checks to make sure that each thread's clib
// context pointer always points to its correct context.

#if !USE_MAIN_THREAD
XosThread clib_test_tcb;
uint8_t   clib_test_stk[STACK_SIZE];
#endif

XosThread test_threads[NUM_THREADS];
uint8_t   stacks[NUM_THREADS][STACK_SIZE];


int32_t
test_func(void * arg, int32_t unused)
{
    int32_t  val = (int32_t) arg;
    uint32_t cnt = 0;
    void *   test_p;

    printf("+++ Thread %s starting\n", xos_thread_get_name(xos_thread_id()));

    srand(val);

    while (cnt < 400) {

#if XSHAL_CLIB == XTHAL_CLIB_XCLIB
        if (_reent_ptr != xos_thread_id()->clib_ptr) {
            xos_fatal_error(-1, "Bad reent ptr");
        }
#elif XSHAL_CLIB == XTHAL_CLIB_NEWLIB
        if (_impure_ptr != xos_thread_id()->clib_ptr) {
            xos_fatal_error(-1, "Bad reent ptr");
        }
#else
  #error Unsupported C library
#endif

        test_p = malloc(rand() % 500);
        if (!test_p) {
            xos_fatal_error(-1, "malloc() failed");
        }

#if XCHAL_NUM_TIMERS == 0
        xos_thread_yield();
#else
        xos_thread_sleep(4 * TICK_CYCLES);
#endif
        free(test_p);
        cnt++;
    }

    return cnt;
}


int32_t
clib_test(void * arg, int32_t unused)
{
    int32_t n = 0;
    int32_t ret;
    int32_t pri = 0;
    char *  p;

    for (n = 0; n < NUM_THREADS; ++n) {
#if XSHAL_CLIB == XTHAL_CLIB_XCLIB
        p = test_threads[n].xclib_reent._tbuf;
#elif XSHAL_CLIB == XTHAL_CLIB_NEWLIB
        p = test_threads[n].newlib_reent._emergency;
#else
#endif
        ret = xos_thread_create(&(test_threads[n]), 0, test_func, (void *)n, p,
                                stacks[n], STACK_SIZE, (pri++ % 5), 0, 0);
        if (ret) {
            xos_fatal_error(-1, "Error creating test threads\n");
        }
        sprintf(p, "Thread_%d", n);
    }

    for (n = 0; n < NUM_THREADS; n++) {
        xos_thread_join(&(test_threads[n]), &ret);
    }

    xos_list_threads();
    printf("PASS\n");

#if !USE_MAIN_THREAD
    exit(0);
#endif
    return 0;
}

#endif // XOS_OPT_THREAD_SAFE_CLIB

int
main()
{
#if XOS_OPT_THREAD_SAFE_CLIB

    int32_t ret;

#if defined BOARD
    xos_set_clock_freq(xtbsp_clock_freq_hz());
#else
    xos_set_clock_freq(XOS_CLOCK_FREQ);
#endif

#if USE_MAIN_THREAD
    xos_start_main_ex("main", 7, STACK_SIZE);
    // Don't use dynamic ticks because we want lots of
    // preemption activity driven by the timer.
    xos_start_system_timer(-1, TICK_CYCLES);

    ret = clib_test(0, 0);
    return ret;
#else
    ret = xos_thread_create(&clib_test_tcb,
                            0,
                            clib_test,
                            0,
                            "clib_test",
                            clib_test_stk,
                            STACK_SIZE,
                            7,
                            0,
                            0);

    xos_start_system_timer(-1, TICK_CYCLES);
    xos_start(0);
    return -1; // should never get here
#endif

#else // No clib thread safety

    printf("Thread safety not enabled, nothing to do.\n");
    printf("PASS\n");
    return 0;

#endif
}

