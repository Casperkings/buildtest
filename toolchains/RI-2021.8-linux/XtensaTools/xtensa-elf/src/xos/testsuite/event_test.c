
// event_test.c - XOS event test

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


#include "test.h"


#define MAX_CNT		50

// Event tests
// NOTE: This test is written to work even without a system timer.

// 1) Create 3 threads that signal each other using the same event object.
//    Each thread uses a separate group of bits to wait on and to signal
//    the next thread.


#if !USE_MAIN_THREAD
XosThread event_test_tcb;
uint8_t   event_test_stk[STACK_SIZE];
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
event_test_basic(void * arg, int32_t unused)
{
    int32_t  ret;

    printf("Basic event test starting\n");

    xos_event_create(&event_1, 0x000fffff, 0);

    ret = xos_thread_create(&thread1, 0, thread_func, (void *)0,
                            "thread1", th1_stack, STACK_SIZE, 1, 0, 0);
    if (ret) {
        xos_fatal_error(-1, "Error creating test threads\n");
    }
    ret = xos_thread_create(&thread2, 0, thread_func, (void *)1,
                            "thread2", th2_stack, STACK_SIZE, 1, 0, 0);
    if (ret) {
        xos_fatal_error(-1, "Error creating test threads\n");
    }
    ret = xos_thread_create(&thread3, 0, thread_func, (void *)2,
                            "thread3", th3_stack, STACK_SIZE, 1, 0, 0);
    if (ret) {
        xos_fatal_error(-1, "Error creating test threads\n");
    }

    int32_t total = 0;

    ret = xos_thread_join(&thread1, &total);
    if (ret) {
        xos_fatal_error(-1, "Error joining thread 1\n");
    }

    ret = xos_thread_join(&thread2, &total);
    if (ret) {
        xos_fatal_error(-1, "Error joining thread 2\n");
    }

    ret = xos_thread_join(&thread3, &total);
    if (ret) {
        xos_fatal_error(-1, "Error joining thread 3\n");
    }

    xos_list_threads();

    xos_thread_delete(&thread1);
    xos_thread_delete(&thread2);
    xos_thread_delete(&thread3);
    xos_event_delete(&event_1);

    printf("Basic event test PASS\n");
    return 0;
}


int32_t 
ac_func(void * arg, int32_t unused)
{   
    int32_t  flag = (int32_t) arg;
    uint32_t bits;

    while (1) { 
        switch(flag) {
        case 0:
            // Wait on our private set of bits
            xos_event_wait_any(&event_1, 0x0800f00f);
            // Check that the bits that woke us have been auto-cleared
            xos_event_get(&event_1, &bits);
            if (xos_thread_get_event_bits() & bits) {
                xos_fatal_error(-1, "Autoclear test failed\n");
            }
            // Signal the controller thread
            xos_event_set(&event_1, 0x01000000);
            break;
        case 1:
            xos_event_wait_any(&event_1, 0x080f00f0);
            xos_event_get(&event_1, &bits);
            if (xos_thread_get_event_bits() & bits) {
                xos_fatal_error(-1, "Autoclear test failed\n");
            }
            xos_event_set(&event_1, 0x02000000);
            break;
        case 2:
            xos_event_wait_any(&event_1, 0x08f00f00);
            xos_event_get(&event_1, &bits);
            if (xos_thread_get_event_bits() & bits) {
                xos_fatal_error(-1, "Autoclear test failed\n");
            }
            xos_event_set(&event_1, 0x04000000);
            break;
        }

        if (xos_thread_get_event_bits() & 0x08000000) {
            // End of test
            break;
        }
    }

    return 0;
}


int32_t
event_test_autoclear(void * arg, int32_t unused)
{
    int32_t  ret;
    int32_t  total;
    uint32_t mask;

    printf("Event autoclear test starting\n");

    xos_event_create(&event_1, 0x0fffffff, XOS_EVENT_AUTO_CLEAR);

    ret = xos_thread_create(&thread1, 0, ac_func, (void *)0,
                            "ac1", th1_stack, STACK_SIZE, 4, 0, 0);
    if (ret) {
        xos_fatal_error(-1, "Error creating test threads\n");
    }
    ret = xos_thread_create(&thread2, 0, ac_func, (void *)1,
                            "ac2", th2_stack, STACK_SIZE, 4, 0, 0);
    if (ret) {
        xos_fatal_error(-1, "Error creating test threads\n");
    }
    ret = xos_thread_create(&thread3, 0, ac_func, (void *)2,
                            "ac3", th3_stack, STACK_SIZE, 4, 0, 0);
    if (ret) {
        xos_fatal_error(-1, "Error creating test threads\n");
    }

    // Signal various bit patterns - these will wake one or more
    // threads. Then wait for them to signal back.
    for (mask = 1; mask <= 0xffffff; mask = (mask << 1) + 1) {
        xos_event_set(&event_1, mask);
        xos_event_wait_any(&event_1, 0x07000000);
    }

    // Wake all listeners and tell them to terminate
    xos_event_set(&event_1, 0x08ffffff);

    ret = xos_thread_join(&thread1, &total);
    if (ret) {
        xos_fatal_error(-1, "Error joining thread 1\n");
    }

    ret = xos_thread_join(&thread2, &total);
    if (ret) {
        xos_fatal_error(-1, "Error joining thread 2\n");
    }

    ret = xos_thread_join(&thread3, &total);
    if (ret) {
        xos_fatal_error(-1, "Error joining thread 3\n");
    }

    xos_list_threads();

    xos_thread_delete(&thread1);
    xos_thread_delete(&thread2);
    xos_thread_delete(&thread3);
    xos_event_delete(&event_1);

    printf("Event autoclear test PASS\n");
    return 0;
}


int32_t
event_test(void * arg, int32_t unused)
{
    event_test_basic(arg, unused);
    event_test_autoclear(arg, unused);

#if !USE_MAIN_THREAD
    exit(0);
#else
    return 0;
#endif
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
    xos_start_main_ex("main", 3, STACK_SIZE);
    xos_start_system_timer(-1, TICK_CYCLES);

    ret = event_test(0, 0);
    return ret;
#else
    ret = xos_thread_create(&event_test_tcb,
                            0,
                            event_test,
                            0,
                            "event_test",
                            event_test_stk,
                            STACK_SIZE,
                            3,
                            0,
                            0);

    xos_start_system_timer(-1, TICK_CYCLES);
    xos_start(0);
    return -1; // should never get here
#endif
}

