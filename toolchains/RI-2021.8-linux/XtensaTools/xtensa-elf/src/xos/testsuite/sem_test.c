
// sem_test.c - XOS semaphore test

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
#define REP_COUNT    1000

XosThread sem_test_tcb;
uint8_t   sem_test_stk[STACK_SIZE];
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

#if XCHAL_NUM_TIMERS > 0
static int32_t          have_timer = 1;
#else
static int32_t          have_timer = 0;
#endif


//-----------------------------------------------------------------------------
// Basic checks. This function is called from both normal thread context
// as well as interrupt context.
//-----------------------------------------------------------------------------
static int32_t
sem_basic_checks(int32_t from_int)
{
    XosSem  sem1;
    int32_t ret;
    int32_t i;

    // Check create: case 1: invalid params
    ret = xos_sem_create(NULL, 0, 0);
    if (ret != XOS_ERR_INVALID_PARAMETER) {
        return -1;
    }

    // case 2: valid params
    ret = xos_sem_create(&sem1, 0, 0);
    if (ret != XOS_OK) {
        return ret;
    }

    // Check various operations with invalid params
    ret = xos_sem_put(NULL);
    if (ret != XOS_ERR_INVALID_PARAMETER) {
        return -1;
    }
    ret = xos_sem_put_max(NULL, 0);
    if (ret != XOS_ERR_INVALID_PARAMETER) {
        return -1;
    }
    ret = xos_sem_tryget(NULL);
    if (ret != XOS_ERR_INVALID_PARAMETER) {
        return -1;
    }
    ret = xos_sem_test(NULL);
    if (ret != -1) {
        return -1;
    }

    if (!from_int) {
        ret = xos_sem_get(NULL);
        if (ret != XOS_ERR_INVALID_PARAMETER) {
            return -1;
        }
        ret = xos_sem_get_timeout(NULL, 0);
        if (ret != XOS_ERR_INVALID_PARAMETER) {
            return -1;
        }
    }

    // Check put -- put twice
    ret = xos_sem_put(&sem1);
    if (ret != XOS_OK) {
        return ret;
    }
    ret = xos_sem_put(&sem1);
    if (ret != XOS_OK) {
        return ret;
    }

    // Check test
    ret = xos_sem_test(&sem1);
    if (ret != 2) {
        return -1;
    }

    // Check nonblocking get
    ret = xos_sem_tryget(&sem1);
    if (ret != XOS_OK) {
        return ret;
    }

    // Check blocking get
    if (!from_int) {
        ret = xos_sem_get(&sem1);
        if (ret != XOS_OK) {
            return ret;
        }
    }

    // Check put_max
    for (i = 0; i < 20; i++) {
        xos_sem_put_max(&sem1, 15);
    }
    if (xos_sem_test(&sem1) != 15) {
        return -1;
    }
    ret = xos_sem_put_max(&sem1, 15);
    if (ret != XOS_ERR_LIMIT) {
        return -1;
    }

    // Check delete: case 1: invalid param
    ret = xos_sem_delete(NULL);
    if (ret != XOS_ERR_INVALID_PARAMETER) {
        return -1;
    }

    // case 2: valid param
    ret = xos_sem_delete(&sem1);
    if (ret != XOS_OK) {
        return ret;
    }

    return 0;
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
    char       buf[64];

    //printf("+ Thread %s starting\n", xos_thread_get_name(xos_thread_id()));

    while (cnt < REP_COUNT) {
        int32_t ret = xos_sem_get(&test_sem_1);

        if (ret != 0) {
            printf("ERROR: xos_sem_get() ret %d\n", ret);
            return cnt;
        }

        cnt++;
        // Now do something for long enough to encounter a context switch.
        // sprintf() is conveniently long.
        sprintf(buf, "Thread %s got sem cnt = %u\n", xos_thread_get_name(xos_thread_id()), cnt);
        *pval = cnt;
    }

    return cnt;
}


//-----------------------------------------------------------------------------
// Semaphore multi-thread test.
//-----------------------------------------------------------------------------
static int32_t
sem_thread_test(int32_t flag)
{
    uint32_t total = 0;
    uint32_t check = 0;
    uint32_t cnt[NUM_THREADS];
    int32_t  old_pri;
    int32_t  ret;
    int32_t  val;
    int32_t  i;


    if (flag) {
        ret = xos_sem_create(&test_sem_1, XOS_SEM_WAIT_FIFO, 0);
    }
    else {
        ret = xos_sem_create(&test_sem_1, XOS_SEM_WAIT_PRIORITY, 0);
    }
    if (ret != XOS_OK) {
        return ret;
    }

    for (i = 0; i < NUM_THREADS; i++) {
        cnt[i] = 0;
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
            printf("ERROR: creating test threads\n");
            return ret;
        }
    }

    // Do some number with this thread at higher priority
    for (i = 0; i < REP_COUNT; i++) {
        ret = xos_sem_put(&test_sem_1);
        if (ret != 0) {
            printf("ERROR: xos_sem_put failed\n");
            return ret;
        }
    }

    // Now lower the priority to be the same as the get threads
    old_pri = xos_thread_get_priority(XOS_THREAD_SELF);
    xos_thread_set_priority(XOS_THREAD_SELF, 1);

    for (i = 0; i < (2 * REP_COUNT); i++) {
        ret = xos_sem_put_max(&test_sem_1, 2 * REP_COUNT);
        if (ret != 0) {
            printf("ERROR: xos_sem_put failed\n");
            return ret;
        }
        xos_thread_yield();
    }

    for (i = 0; i < NUM_THREADS; i++) {
        ret = xos_thread_join(&test_threads[i], &val);
        if (ret) {
            printf("ERROR: joining thread\n");
            return ret;
        }
        total += val;
        check += cnt[i];
    }

    if (check != total) {
        printf("ERROR: sum of counts mismatch\n");
        return 1;
    }
    if (total != NUM_THREADS * REP_COUNT) {
        printf("ERROR: total count mismatch\n");
        return 1;
    }

    xos_list_threads();
    xos_thread_set_priority(XOS_THREAD_SELF, old_pri);

    for (i = 0; i < NUM_THREADS; i++) {
        xos_thread_delete(&test_threads[i]);
    }

    return 0;
}


//-----------------------------------------------------------------------------
// Thread function to check semaphore timeout operation.
//-----------------------------------------------------------------------------
static int32_t
sem_timeout_func(void * arg, int32_t unused)
{
    int32_t  ret;
    uint32_t cycles = 120000;
    XosSem * sem    = (XosSem *) arg;

    // Check that sem count is zero.
    ret = xos_sem_test(sem);
    if (ret != 0) {
        return -1;
    }

    // These should all fail.
    while (cycles > 100) {
        ret = xos_sem_get_timeout(sem, cycles);
        if (ret != XOS_ERR_TIMEOUT) {
            return -1;
        }
        cycles /= 2;
    }

    // This should also fail.
    ret = xos_sem_tryget(sem);
    if (ret != XOS_ERR_SEM_BUSY) {
        return -1;
    }

    return 0;
}


//-----------------------------------------------------------------------------
// Thread function to check semaphore unblock on delete.
//-----------------------------------------------------------------------------
static int32_t
sem_block_func(void * arg, int32_t unused)
{
    int32_t  ret;
    XosSem * sem = (XosSem *) arg;

    // Block on sem until deleted.
    ret = xos_sem_get(sem);
    if (ret != XOS_ERR_SEM_DELETE) {
        return -1;
    }

    return 0;
}


//-----------------------------------------------------------------------------
// Semaphore timeout test.
//-----------------------------------------------------------------------------
static int32_t
sem_timeout_test(void)
{
    XosSem  sem;
    int32_t ret;
    int32_t val;

    // Create the mutex with an initial count and check that the get_timeout
    // call succeeds.
    ret = xos_sem_create(&sem, 0, 1);
    if (ret != XOS_OK) {
        return ret;
    }
    ret = xos_sem_get_timeout(&sem, 100);
    if (ret != XOS_OK) {
        return ret;
    }

    // Now the semaphore is at zero count. Create test thread.
    ret = xos_thread_create(&test_threads[0],
                            0,
                            sem_timeout_func,
                            &sem,
                            t_names[0],
                            test_stacks[0],
                            STACK_SIZE,
                            1,
                            0,
                            0);
    if (ret != XOS_OK) {
        return ret;
    }

    ret = xos_thread_join(&test_threads[0], &val);
    ret = xos_thread_delete(&test_threads[0]);

    if (val != 0) {
        return val;
    }

    // Create another thread that blocks on the semaphore.
    ret = xos_thread_create(&test_threads[0],
                            0,
                            sem_block_func,
                            &sem,
                            t_names[0],
                            test_stacks[0],
                            STACK_SIZE,
                            10,
                            0,
                            0);
    if (ret != XOS_OK) {
        return ret;
    }

    // Make sure it has blocked on the semaphore.
    while (xos_thread_get_state(&test_threads[0]) != XOS_THREAD_STATE_BLOCKED) {
        xos_thread_yield();
    }

    // Now destroy the semaphore. This should unblock the thread.
    ret = xos_sem_delete(&sem);
    if (ret != XOS_OK) {
        return ret;
    }

    ret = xos_thread_join(&test_threads[0], &val);
    ret = xos_thread_delete(&test_threads[0]);

    return val;
}


//-----------------------------------------------------------------------------
// Test driver function.
//-----------------------------------------------------------------------------
int32_t
sem_test(void * arg, int32_t unused)
{
    XosTimer timer1;
    int32_t  ret;

    printf("Semaphore test starting\n");

    // Run basic checks from thread
    ret = sem_basic_checks(0);
    if (ret) {
        printf("ERROR: basic checks failed\n");
        exit(ret);
    }
    printf("### SEM basic checks 1 ..................... PASS\n");

    // Run basic checks from timer callback
    if (have_timer) {
        xos_timer_init(&timer1);
        xos_timer_start(&timer1, 1000, 0, sem_timer_cb, (void *)&ret);

        while (!flag);
        if (ret != XOS_OK) {
            printf("ERROR: basic checks from intr callback failed\n");
            exit(ret);
        }
    }
    printf("### SEM basic checks 2 ..................... PASS\n");

    if (have_timer) {
        // Run timeout test
        ret = sem_timeout_test();
        if (ret != 0) {
            printf("ERROR: timeout test failed\n");
            exit(ret);
        }
    }
    printf("### SEM timeout test ....................... PASS\n");

    // Run thread test with wait mode = priority
    ret = sem_thread_test(0);
    if (ret != 0) {
        printf("ERROR: thread test 1 failed\n");
        exit(ret);
    }
    printf("### SEM thread test 1 ...................... PASS\n");

    // Run thread test with wait mode = fifo
    ret = sem_thread_test(1);
    if (ret != 0) {
        printf("ERROR: thread test 2 failed\n");
        exit(ret);
    }
    printf("### SEM thread test 2 ...................... PASS\n");

    exit(0);
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

    if (xos_start_system_timer(-1, TICK_CYCLES) != XOS_OK) {
        have_timer = 0;
    }

    ret = sem_test(0, 0);
    return ret;
#else
    ret = xos_thread_create(&sem_test_tcb,
                            0,
                            sem_test,
                            0,
                            "sem_test",
                            sem_test_stk,
                            STACK_SIZE,
                            7,
                            0,
                            0);

    if (xos_start_system_timer(-1, TICK_CYCLES) != XOS_OK) {
        have_timer = 0;
    }

    xos_start(0);
    return -1; // should never get here
#endif
}

