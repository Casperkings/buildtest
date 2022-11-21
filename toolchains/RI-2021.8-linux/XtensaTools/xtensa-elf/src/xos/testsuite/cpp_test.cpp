
// cpp_test.cpp - XOS C++ test

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


#include <iostream>

#include "test.h"


#define MAX_CNT		50

// C++ test - make sure we can compile C++ code with XOS headers and lib.
// NOTE: This test is written to work even without a system timer.

// 1) Create 3 threads that signal each other using the same event object.
//    Each thread uses a separate group of bits to wait on and to signal
//    the next thread.


#if !USE_MAIN_THREAD
XosThread cpp_test_tcb;
uint8_t   cpp_test_stk[STACK_SIZE];
#endif

XosThread thread1;
XosThread thread2;
XosThread thread3;

uint8_t th1_stack[STACK_SIZE];
uint8_t th2_stack[STACK_SIZE];
uint8_t th3_stack[STACK_SIZE];

XosEvent event_1;


int32_t
thread_func(void * arg, int32_t unused)
{
    int32_t flag = (int32_t) arg;
    int32_t cnt  = 0;

    printf("+++ Thread %s starting\n", xos_thread_get_name(xos_thread_id()));

    while (cnt < MAX_CNT) {
        switch(flag) {
        case 0:
            // Clear all, set bits 0-3, wait on bits 8-11
            xos_event_clear(&event_1, XOS_EVENT_BITS_ALL);
            xos_event_set(&event_1, 0xf);
            xos_event_wait_all(&event_1, 0xf00);
            break;
        case 1:
            // Wait on bits 0-3, then set bits 16-19
            xos_event_wait_all(&event_1, 0xf);
            xos_event_clear(&event_1, 0xf);
            xos_event_set(&event_1, 0xf0000);
            break;
        case 2:
            // Wait on bits 16-19, then set bits 8-11
            xos_event_wait_all(&event_1, 0xf0000);
            xos_event_clear(&event_1, 0xf0000);
            xos_event_set(&event_1, 0xf00);
            break;
        }
        ++cnt;
    }

    return cnt;
}


int32_t
cpp_test(void * arg, int32_t unused)
{
    int32_t  ret;

    printf("cpptest starting\n");

    xos_event_create(&event_1, 0x000fffff, 0);

    ret = xos_thread_create(&thread1, 0, thread_func, (void *)0,
                            "Thread1", th1_stack, STACK_SIZE, 1, 0, 0);
    if (ret) {
        xos_fatal_error(-1, "Error creating test threads\n");
    }
    ret = xos_thread_create(&thread2, 0, thread_func, (void *)1,
                            "Thread2", th2_stack, STACK_SIZE, 1, 0, 0);
    if (ret) {
        xos_fatal_error(-1, "Error creating test threads\n");
    }
    ret = xos_thread_create(&thread3, 0, thread_func, (void *)2,
                            "Thread3", th3_stack, STACK_SIZE, 1, 0, 0);
    if (ret) {
        xos_fatal_error(-1, "Error creating test threads\n");
    }

    int32_t total = 0;

    ret = xos_thread_join(&thread1, &total);
    if (ret) {
        xos_fatal_error(-1, "Error joining thread 1\n");
    }
    printf("Finished wait for thread 1\n");

    ret = xos_thread_join(&thread2, &total);
    if (ret) {
        xos_fatal_error(-1, "Error joining thread 2\n");
    }
    printf("Finished wait for thread 2\n");

    ret = xos_thread_join(&thread3, &total);
    if (ret) {
        xos_fatal_error(-1, "Error joining thread 3\n");
    }
    printf("Finished wait for thread 3\n");

    xos_thread_delete(&thread1);
    xos_thread_delete(&thread2);
    xos_thread_delete(&thread3);

    xos_event_delete(&event_1);

#if !USE_MAIN_THREAD
    exit(0);
#endif
    return 0;
}


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

    ret = cpp_test(0, 0);
    return ret;
#else
    ret = xos_thread_create(&cpp_test_tcb,
                            0,
                            cpp_test,
                            0,
                            "cpp_test",
                            cpp_test_stk,
                            STACK_SIZE,
                            7,
                            0,
                            0);

    xos_start_system_timer(-1, TICK_CYCLES);
    xos_start(0);
    return -1; // should never get here
#endif
}

