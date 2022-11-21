
// small_test.c - XOS test/demo program, built to minimize the exe size.

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


// Create 3 threads that repeatedly take the same semaphore. Put the
// semaphore a fixed number of times from a 4th thread. Verify that
// the cumulative get count for all 3 threads matches the put count.


#define NUM_THREADS  3
#define REP_COUNT    5000

#if !USE_MAIN_THREAD
XosThread small_test_tcb;
uint8_t   small_test_stk[STACK_SIZE];
#endif

XosThread test_threads[NUM_THREADS];
uint8_t   test_stacks[NUM_THREADS][STACK_SIZE];
char *    t_names[] =
{
    "Thread1",
    "Thread2",
    "Thread3",
};

static XosSem           test_sem_1;
static volatile int32_t flag = 0;


//-----------------------------------------------------------------------------
// Basic checks. This function is called from both normal thread context
// as well as interrupt context.
//-----------------------------------------------------------------------------
static int32_t
sem_basic_checks(int32_t from_int)
{
    XosSem  sem1;
    int32_t ret;

    // Check create
    ret = xos_sem_create(&sem1, 0, 0);
    if (ret != XOS_OK)
        return ret;

    // Check put -- put twice
    ret = xos_sem_put(&sem1);
    if (ret != XOS_OK)
        return ret;
    ret = xos_sem_put(&sem1);
    if (ret != XOS_OK)
        return ret;

    // Check test
    ret = xos_sem_test(&sem1);
    if (ret != 2)
        return 1;

    // Check nonblocking get
    ret = xos_sem_tryget(&sem1);
    if (ret != XOS_OK)
        return ret;

    // Check blocking get
    if (!from_int) {
        ret = xos_sem_get(&sem1);
        if (ret != XOS_OK)
            return ret;
    }

    // Check delete
    ret = xos_sem_delete(&sem1);
    return ret;
}


//-----------------------------------------------------------------------------
// Timer callback for test_basic.
//-----------------------------------------------------------------------------
void
sem_timer_cb(void * arg)
{
    int32_t * p = (int32_t *) arg;

    *p = sem_basic_checks(1);
    flag = 1;
}


//-----------------------------------------------------------------------------
// Thread function for multi-thread sem test.
//-----------------------------------------------------------------------------
static int32_t
sem_get_func(void * arg, int32_t unused)
{
    uint32_t * pval = (uint32_t *) arg; // Result storage
    uint32_t   cnt  = 0;
    //char       buf[64];

    while (cnt < REP_COUNT) {
        int32_t ret = xos_sem_get(&test_sem_1);

        if (ret != 0) {
            return cnt;
        }

        cnt++;
        // Now do something for long enough to encounter a context switch.
        // sprintf() is conveniently long.
        //sprintf(buf, "Thread %s got sem cnt = %u\n", xos_thread_get_name(xos_thread_id()), cnt);
        *pval = cnt;
    }

    return cnt;
}


//-----------------------------------------------------------------------------
// Test driver function.
//-----------------------------------------------------------------------------
int32_t
small_test(void * arg, int32_t unused)
{
#if XCHAL_NUM_TIMERS > 0
    XosTimer timer1;
#endif
    uint32_t total = 0;
    uint32_t cnt[3];
    int32_t  old_pri;
    int32_t  ret;
    int32_t  val;
    int32_t  i;

    // Run basic checks from thread
    ret = sem_basic_checks(0);
    if (ret) {
        return ret;
    }

    // Run basic checks from timer callback
#if XCHAL_NUM_TIMERS > 0
    xos_timer_init(&timer1);
    xos_timer_start(&timer1, 1000, 0, sem_timer_cb, (void *)&ret);

    while (!flag);
    if (ret != XOS_OK) {
        return ret;
    }
#endif

    ret = xos_sem_create(&test_sem_1, XOS_SEM_WAIT_PRIORITY, 0);

    cnt[0] = cnt[1] = cnt[2] = 0;

    for (i = 0; i < 3; i++) {
        ret = xos_thread_create(&test_threads[i],
                                0,
                                sem_get_func,
                                &cnt[i],
                                t_names[i],
                                test_stacks[i],
                                STACK_SIZE,
                                1,
                                0,
                                0);
        if (ret) {
            return ret;
        }
    }

    // Do some number with this thread at higher priority
    for (i = 0; i < REP_COUNT; i++) {
        xos_sem_put(&test_sem_1);
    }

    // Now lower the priority to be the same as the get threads
    old_pri = xos_thread_get_priority(XOS_THREAD_SELF);
    xos_thread_set_priority(XOS_THREAD_SELF, 1);

    for (i = 0; i < (2 * REP_COUNT); i++) {
        xos_sem_put(&test_sem_1);
        xos_thread_yield();
    }

    for (i = 0; i < 3; i++) {
        ret = xos_thread_join(&test_threads[i], &val);
        if (ret) {
            return ret;
        }
        total += val;
    }

    if (cnt[0] + cnt[1] + cnt[2] != total) {
        return 1;
    }
    if (total != 3*REP_COUNT) {
        return 1;
    }

    //xos_list_threads();
    xos_thread_set_priority(XOS_THREAD_SELF, old_pri);

    for (i = 0; i < 3; i++) {
        xos_thread_delete(&test_threads[i]);
    }

#if !USE_MAIN_THREAD
    exit(0);
#endif
    return 0;
}


//-----------------------------------------------------------------------------
// Main
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

    ret = small_test(0, 0);
    return ret;
#else
    ret = xos_thread_create(&small_test_tcb,
                            0,
                            small_test,
                            0,
                            "small_test",
                            small_test_stk,
                            STACK_SIZE,
                            7,
                            0,
                            0);

    xos_start_system_timer(-1, TICK_CYCLES);
    xos_start(0);
    return -1; // should never get here
#endif
}   

