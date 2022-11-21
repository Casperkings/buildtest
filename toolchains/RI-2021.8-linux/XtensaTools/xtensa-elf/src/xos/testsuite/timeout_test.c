
// timeout_test.c - XOS wait timeout test

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


#define NUM_ITER     100
#define NUM_THREADS  4


#if !USE_MAIN_THREAD
XosThread to_test_tcb;
uint8_t   to_test_stk[STACK_SIZE];
#endif

XosThread test_threads[NUM_THREADS];
uint8_t   test_stacks[NUM_THREADS][STACK_SIZE];
char *    t_names[] =
{
    "Thread0",
    "Thread1",
    "Thread2",
    "Thread3",
};

uint32_t cnt[NUM_THREADS];

static XosSem        sem;
static XosMutex      mtx;
// Test queue, 40 messages
XOS_MSGQ_ALLOC(msgq, 40, sizeof(uint32_t));


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int32_t
test_func(void * arg, int32_t unused)
{
    uint32_t * p = (uint32_t *) arg;
    uint32_t   x;
    int32_t    ret;
    int32_t    i;

    for (i = 0; i < NUM_ITER; i++) {
        uint64_t cycles = (xos_get_clock_freq() / 1000) * (xos_get_ccount() % 9 + 1);

        ret = xos_sem_get_timeout(&sem, cycles);
        if (ret != XOS_ERR_TIMEOUT)
            puts("FAIL: sem wait did not time out");

        ret = xos_mutex_lock_timeout(&mtx, cycles);
        if (ret != XOS_ERR_TIMEOUT)
            puts("FAIL: mutex wait did not time out");

        ret = xos_msgq_get_timeout(msgq, &x, cycles);
        if (ret != XOS_ERR_TIMEOUT)
            puts("FAIL: msgq wait did not time out");

        (*p)++;
    }

    return 0;
}


//-----------------------------------------------------------------------------
// Test driver function.
//-----------------------------------------------------------------------------
int32_t
timeout_test(void * arg, int32_t unused)
{
    int32_t ret;
    int32_t i;
    int32_t val;

    // Create the waitable objects to test with
    ret = xos_msgq_create(msgq, 40, sizeof(uint32_t), 0);

    ret = xos_sem_create(&sem, XOS_SEM_WAIT_PRIORITY, 0);

    ret = xos_mutex_create(&mtx, XOS_MUTEX_WAIT_PRIORITY, 0);

    // Make the mutex be unavailable
    ret = xos_mutex_lock(&mtx);

    // Create test threads
    for (i = 0; i < NUM_THREADS; i++) {
        ret = xos_thread_create(&test_threads[i],
                                0,
                                test_func,
                                &cnt[i],
                                t_names[i],
                                test_stacks[i],
                                STACK_SIZE,
                                1,
                                0,
                                0);
        if (ret) {
            printf("ERROR: creating test threads\n");
            return ret;
        }
    }

    for (i = 0; i < NUM_THREADS; i++) {
        ret = xos_thread_join(&test_threads[i], &val);
        if (ret) {
            printf("ERROR: joining thread\n");
            return ret;
        }
        if (cnt[i] != NUM_ITER) {
            printf("ERROR: test count mismatch\n");
            return ret;
        }
    }

    for (i = 0; i < NUM_THREADS; i++) {
        xos_thread_delete(&test_threads[i]);
    }

    printf("Timeout test PASSED\n");

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
    xos_start_system_timer(-1, 0);

    ret = timeout_test(0, 0);
    return ret;
#else
    ret = xos_thread_create(&to_test_tcb,
                            0,
                            timeout_test,
                            0,
                            "timeout_test",
                            to_test_stk,
                            STACK_SIZE,
                            7,
                            0,
                            0);

    xos_start_system_timer(-1, 0);
    xos_start(0);
    return -1; // should never get here
#endif
}   

