// multi_test.c -- Multi-thread stress test.

// Copyright (c) 2015-2021 Cadence Design Systems, Inc.
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

#include <math.h>
#include "test.h"

XosThread test_main_tcb;
XosThread busy1_tcb;
XosThread busy2_tcb;
XosThread ping_tcb;
XosThread pong_tcb;
XosThread ptw_tcb;

char test_main_stack[STACK_SIZE];
char busy1_stack[STACK_SIZE];
char busy2_stack[STACK_SIZE];
char ping_stack[STACK_SIZE];
char pong_stack[STACK_SIZE];
char ptw_stack[STACK_SIZE];

XosTimer timer1;

XOS_MSGQ_ALLOC(pingq, 2, sizeof(uint32_t));
XOS_MSGQ_ALLOC(pongq, 2, sizeof(uint32_t));


//---------------------------------------------------------------------
// Check return value and bail if not OK.
//---------------------------------------------------------------------
void
check_ret(int32_t ret)
{
    if (ret != XOS_OK) {
        xos_fatal_error(-1, "Fatal error");
    }
}


//---------------------------------------------------------------------
// Ping thread entry point.
//---------------------------------------------------------------------
int32_t
ping_func(void * arg, int32_t unused)
{
    int32_t  ret;
    uint32_t i = 0;
    uint32_t v = 0;
    uint32_t r;

    while (++i) {
        ret = xos_msgq_put(pongq, &v);
        check_ret(ret);
        ret = xos_msgq_get(pingq, &r);
        check_ret(ret);
        // Returned value should be one more than we sent.
        if (r != v + 1) {
            printf("ERROR: ping got %d exp %d\n", r, v + 1);
            continue;
        }
        printf("ping got %d\n", r);
        v++;
    }

    return 0;
}


//---------------------------------------------------------------------
// Pong thread entry point.
//---------------------------------------------------------------------
int32_t
pong_func(void * arg, int32_t unused)
{
    int32_t  ret;
    uint32_t i = 0;
    uint32_t v;

    while (++i) {
        ret = xos_msgq_get(pongq, &v);
        check_ret(ret);
        printf("pong got %d\n", v);
        // Add one and return the new value.
        v++;
        ret = xos_msgq_put(pingq, &v);
        check_ret(ret);
    }

    return 0;
}


//---------------------------------------------------------------------
// Periodic timer callback 1.
//---------------------------------------------------------------------
void
timer1_cb(void * arg)
{
    static int32_t flag;

    if (flag) {
        xos_thread_suspend(&busy1_tcb);
        xos_thread_resume(&busy2_tcb);
        flag = 0;
    }
    else {
        xos_thread_suspend(&busy2_tcb);
        xos_thread_resume(&busy1_tcb);
        flag = 1;
    }
}


//---------------------------------------------------------------------
// Generate recursive calls as long as there is stack space. Try to
// use up as much of the stack as possible without going over.
//---------------------------------------------------------------------
__attribute__((noinline)) int32_t
deep_call(uint32_t * arg)
{
    uint32_t   tmp;
    uint32_t * p = &tmp;

    *arg = (uint32_t) arg;

    if ((void *)p <= (void *)xos_thread_id()->stack_base) {
        xos_fatal_error(-1, "Stack overflow");
    }

    if (((uint32_t)p - (uint32_t)xos_thread_id()->stack_base) > (XOS_FRAME_SIZE + 96)) {
        return deep_call(p) + 1;
    }

    return 0;
}


//---------------------------------------------------------------------
// Busy threads entry point.
//---------------------------------------------------------------------
int32_t
busy_func(void * arg, int32_t unused)
{
    uint32_t i = 0;
    uint32_t j = 0;
    uint32_t v = (uint32_t) arg;
    uint32_t d = (v == 1) ? 3851 : 5399;

    while (++i) {
       printf("Busy%d %d %d\n", v, i, deep_call(&j));
       xos_thread_sleep(d);
    }

    return 0;
}


//---------------------------------------------------------------------
// Periodic timer wait thread entry point.
//---------------------------------------------------------------------
int32_t
ptw_func(void * arg, int32_t unused)
{
    int32_t  ret;
    uint32_t cnt;
    uint32_t last;
    uint32_t diff;
    uint64_t vsum;
    uint32_t vmin;
    uint32_t vmax;
    uint32_t vavg;
    uint32_t sumd;
    XosTimer ptw_timer;

    xos_timer_init(&ptw_timer);
    ret = xos_timer_start(&ptw_timer,
                          3 * TICK_CYCLES,
                          XOS_TIMER_FROM_NOW | XOS_TIMER_PERIODIC,
                          NULL,
                          0);
    check_ret(ret);

    cnt  = 0;
    vsum = 0;
    vmin = 0xFFFFFFFF;
    vmax = 0;
    vavg = 0;
    sumd = 0;
    last = xos_get_ccount();

    while (1) {
        ret  = xos_timer_wait(&ptw_timer);
        diff = xos_get_ccount() - last;
        check_ret(ret);
        vsum += diff;
        cnt++;
        vavg = (uint32_t)(vsum/cnt);

        sumd += ((int32_t)vavg - (int32_t)diff) * ((int32_t)vavg - (int32_t)diff);
        if ((cnt % 16) == 0) {
            uint32_t stddev = (uint32_t) sqrt(sumd/cnt);

            printf("delta %u, period %u, min/max/avg/stddev (%u/%u/%u/%u)\n",
                   diff, 3 * TICK_CYCLES, vmin, vmax, vavg, stddev);
        }

        if (diff < vmin) {
            vmin = diff;
        }
        if (diff > vmax) {
            vmax = diff;
        }
        last += diff;
    }

    return 0;
}


//---------------------------------------------------------------------
// Create and launch all the test threads.
//---------------------------------------------------------------------
int32_t
test_main(void * arg, int32_t unused)
{
    int32_t ret;
    int32_t i;

    // Create pingpong test threads and queues
    xos_msgq_create(pingq, 2, sizeof(uint32_t), 0);
    xos_msgq_create(pongq, 2, sizeof(uint32_t), 0);

    ret = xos_thread_create(&ping_tcb,
                            0,
                            ping_func,
                            0,
                            "ping",
                            ping_stack,
                            STACK_SIZE,
                            2,
                            0,
                            0);
    check_ret(ret);

    ret = xos_thread_create(&pong_tcb,
                            0,
                            pong_func,
                            0,
                            "pong",
                            pong_stack,
                            STACK_SIZE,
                            2,
                            0,
                            0);
    check_ret(ret);

    // Create deep call threads
    ret = xos_thread_create(&busy1_tcb,
                            0,
                            busy_func,
                            (void *) 1,
                            "busy1",
                            busy1_stack,
                            STACK_SIZE,
                            3,
                            0,
                            0);
    check_ret(ret);

    ret = xos_thread_create(&busy2_tcb,
                            0,
                            busy_func,
                            (void *) 2,
                            "busy2",
                            busy2_stack,
                            STACK_SIZE,
                            3,
                            0,
                            0);
    check_ret(ret);

    // Create the periodic timer wait thread.
    // Priority is one less than the main thread.
    ret = xos_thread_create(&ptw_tcb,
                            0,
                            ptw_func,
                            (void *) 0,
                            "ptw",
                            ptw_stack,
                            STACK_SIZE,
                            14,
                            0,
                            0);
    check_ret(ret);

    // Start the periodic timer.
    xos_timer_init(&timer1);
    ret = xos_timer_start(&timer1,
                          5 * TICK_CYCLES,
                          XOS_TIMER_PERIODIC,
                          timer1_cb,
                          0);
    check_ret(ret);

    // List all threads
    xos_list_threads();

    i = 0;
    while (++i) {
        xos_thread_sleep(100 * TICK_CYCLES);
        printf("test_main: I have woken\n");
    }

    return 0;
}


//---------------------------------------------------------------------
// Main.
//---------------------------------------------------------------------
int
main()
{
    int32_t ret;

#if XCHAL_HAVE_MPU
    extern int32_t xos_mpu_init(void);

    if (xos_mpu_init() != 0) {
        printf("MPU init failed\n");
        return -1;
    }
#endif

#if defined BOARD
    xos_set_clock_freq(xtbsp_clock_freq_hz());
#else
    xos_set_clock_freq(XOS_CLOCK_FREQ);
#endif

    ret = xos_thread_create(&test_main_tcb,
                            0,
                            test_main,
                            0,
                            "test_main",
                            test_main_stack,
                            STACK_SIZE,
                            15,
                            0,
                            0);
    check_ret(ret);
    
    xos_start_system_timer(-1, TICK_CYCLES);
    xos_start(0);
    return -1; // should never get here
}

