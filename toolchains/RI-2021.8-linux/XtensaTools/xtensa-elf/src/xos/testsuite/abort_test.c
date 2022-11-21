
// abort_test.c -- XOS thread abort test.

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


#include <xtensa/corebits.h>
#include <xtensa/tie/xt_core.h>

#include "test.h"


#define  EXIT_CODE   (0x0a0a0505)


#if !USE_MAIN_THREAD
XosThread abort_test_tcb;
uint8_t   abort_test_stk[STACK_SIZE];
#endif

XosThread thd1;
XosThread thd2;

uint8_t thd1_stack[STACK_SIZE];
uint8_t thd2_stack[STACK_SIZE];


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int32_t
test_func(void * arg, int32_t unused)
{
    int32_t          ret  = 0;
    volatile int32_t junk = 0;

    while (1) {
        junk++;
    }

    return ret;
}


//-----------------------------------------------------------------------------
//  exit handler for test thread.
//-----------------------------------------------------------------------------
int32_t
exit_handler(int32_t exitcode)
{
    return exitcode;
}


//-----------------------------------------------------------------------------
// Test driver function.
//-----------------------------------------------------------------------------
int32_t
abort_test(void * arg, int32_t unused)
{
    int32_t  ret;
    int32_t  cnt;
    int32_t  i;

    printf("Abort test starting\n");

    for (cnt = 0; cnt < 10; cnt++) {
        ret = xos_thread_create(&thd1, 0, test_func, (void *)0, "thd1",
                                thd1_stack, STACK_SIZE, 2, 0, 0);
        if (ret)
            xos_fatal_error(-1, "Error creating test thread\n");

        xos_thread_set_exit_handler(&thd1, exit_handler);

        // Let the test thread run
        xos_thread_sleep(xos_get_ccount() % 20000);

        // Now abort the test with an exit code
        ret = xos_thread_abort(&thd1, EXIT_CODE);
        xos_thread_sleep(20000);
        ret = xos_thread_join(&thd1, &i);

        if (ret)
            xos_fatal_error(-1, "Error joining thread thd1\n");

        if (i != EXIT_CODE)
            printf("Test 1 pass %d failed\n", cnt);
        else
            printf("Test 1 pass %d OK\n", cnt);

        xos_thread_delete(&thd1);
    }

#if !USE_MAIN_THREAD
    exit(0);
#endif
    return 0;
}


//-----------------------------------------------------------------------------
//  Main.
//-----------------------------------------------------------------------------
int
main()
{
    int32_t ret;

#if defined BOARD
    xos_set_clock_freq(xtbsp_clock_freq_hz());
#else
    xos_set_clock_freq(XOS_CLOCK_FREQ);
#endif

#if USE_MAIN_THREAD
    xos_start_main_ex("main", 7, STACK_SIZE);
    xos_start_system_timer(-1, TICK_CYCLES);

    ret = abort_test(0, 0);
    return ret;
#else
    ret = xos_thread_create(&abort_test_tcb,
                            0,
                            abort_test,
                            0,
                            "abort_test",
                            abort_test_stk,
                            STACK_SIZE,
                            7,
                            0,
                            0);

    xos_start_system_timer(-1, TICK_CYCLES);
    xos_start(0);
    return -1; // should never get here
#endif
}

