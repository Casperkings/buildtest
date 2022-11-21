
// mutex_test.c - XOS mutex test

// Copyright (c) 2020 Cadence Design Systems, Inc.
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


// Mutex tests:
// (1) Check basic functions with valid and invalid parameters.
// (2) Create 3 threads that compete for a single mutex. Verify
//     that all threads complete successfully.
// (3) Repeat test with mutex wait policy = fifo.
// (4) Check mutex timeout functionality.
// (5) Check mutex priority inversion protection.


#define NUM_THREADS  3
#define REP_COUNT    1000
#define LOW_PRI      6
#define HIGH_PRI     8


#if !USE_MAIN_THREAD
// If main() is not converted into a thread, then allocate space
// for the test main thread.
XosThread mutex_test_tcb;
uint8_t   mutex_test_stk[STACK_SIZE];
#endif

XosThread test_threads[NUM_THREADS];
uint8_t   test_stacks[NUM_THREADS][STACK_SIZE];
char *    t_names[] =
{
    "Thread_1",
    "Thread_2",
    "Thread_3",
};

static XosMutex         test_mutex_1;
static XosMutex         mtx3;
static XosSem           sem3;

#if XCHAL_NUM_TIMERS > 0
static int32_t          have_timer = 1;
#else
static int32_t          have_timer = 0;
#endif


//-----------------------------------------------------------------------------
// Basic API checks.
//-----------------------------------------------------------------------------
static int32_t
mutex_basic_checks(void)
{
    XosMutex mtx1;
    XosSem   sem1;
    int32_t  ret;
    int32_t  i;

    // Check create: step 1: invalid parameters
    ret = xos_mutex_create(NULL, 0, 0);
    if (ret != XOS_ERR_INVALID_PARAMETER) {
        return -1;
    }
    ret = xos_mutex_create(&mtx1, 0, -1);
    if (ret != XOS_ERR_INVALID_PARAMETER) {
        return -1;
    }
    ret = xos_mutex_create(&mtx1, 0, XOS_MAX_PRIORITY + 1);
    if (ret != XOS_ERR_INVALID_PARAMETER) {
        return -1;
    }

    // step 2: valid parameters
    ret = xos_mutex_create(&mtx1, 0, 0);
    if (ret != XOS_OK) {
        return ret;
    }
    ret = xos_sem_create(&sem1, 0, 0);
    if (ret != XOS_OK) {
        return ret;
    }

    // Check state: step 1: invalid parameter
    ret = xos_mutex_test(NULL);
    if (ret != -1) {
        return -1;
    }

    // Check state: step 1a: valid parameter invalid signature
    ret = xos_mutex_test((XosMutex *)&sem1);
    if (ret != -1) {
        return -1;
    }

    // step 2: valid parameter (should be unlocked)
    ret = xos_mutex_test(&mtx1);
    if (ret != 0) {
        return -1;
    }

    // Check lock/unlock: step 1: invalid parameter
    ret = xos_mutex_lock(NULL);
    if (ret != XOS_ERR_INVALID_PARAMETER) {
        return -1;
    }
    ret = xos_mutex_unlock(NULL);
    if (ret != XOS_ERR_INVALID_PARAMETER) {
        return -1;
    }

    // Check lock/unlock: step 1a: valid parameter invalid signature
    ret = xos_mutex_lock((XosMutex *)&sem1);
    if (ret != XOS_ERR_INVALID_PARAMETER) {
        return -1;
    }
    ret = xos_mutex_unlock((XosMutex *)&sem1);
    if (ret != XOS_ERR_INVALID_PARAMETER) {
        return -1;
    }

    // step 2: valid parameter
    ret = xos_mutex_lock(&mtx1);
    if (ret != XOS_OK) {
        return ret;
    }
    ret = xos_mutex_test(&mtx1);
    if (ret != 1) { // should be locked
        return -1;
    }
    ret = xos_mutex_unlock(&mtx1);
    if (ret != XOS_OK) {
        return ret;
    }

    // Check recursive locking
    ret = xos_mutex_lock(&mtx1);
    if (ret != XOS_OK) {
        return ret;
    }

    for (i = 0; i < 5; i++) {
        ret = xos_mutex_lock(&mtx1);
        if (ret != XOS_OK) {
            return ret;
        }
    }

    for (i = 0; i < 6; i++) {
        ret = xos_mutex_unlock(&mtx1);
        if (ret != XOS_OK) {
            return ret;
        }
    }

    // Verify that mutex is now unlocked
    ret = xos_mutex_test(&mtx1);
    if (ret != 0) {
        return ret;
    }

    // Trylock: step 1: invalid parameter
    ret = xos_mutex_trylock(NULL);
    if (ret != XOS_ERR_INVALID_PARAMETER) {
        return -1;
    }

    // Check lock/unlock: step 1a: valid parameter invalid signature
    ret = xos_mutex_trylock((XosMutex *)&sem1);
    if (ret != XOS_ERR_INVALID_PARAMETER) {
        return -1;
    }

    // step 2: valid parameter
    ret = xos_mutex_trylock(&mtx1);
    if (ret != XOS_OK) {
        return ret;
    }

    // Recursive lock/unlock
    ret = xos_mutex_trylock(&mtx1);
    if (ret != XOS_OK) {
        return ret;
    }
    ret = xos_mutex_unlock(&mtx1);
    if (ret != XOS_OK) {
        return ret;
    }

    // Unlock
    ret = xos_mutex_unlock(&mtx1);
    if (ret != XOS_OK) {
        return ret;
    }

    // Destroy: step 1: invalid parameter
    ret = xos_mutex_delete(NULL);
    if (ret != XOS_ERR_INVALID_PARAMETER) {
        return -1;
    }

    // Check lock/unlock: step 1a: valid parameter invalid signature
    ret = xos_mutex_delete((XosMutex *)&sem1);
    if (ret != XOS_ERR_INVALID_PARAMETER) {
        return -1;
    }

    // step 2: valid parameter
    ret = xos_mutex_delete(&mtx1);
    return ret;
}


//-----------------------------------------------------------------------------
// Thread function. Waits on mutex and increments a counter every time
// it gets ownership until target count is reached.
//-----------------------------------------------------------------------------
static int32_t
mutex_get_func(void * arg, int32_t unused)
{
    uint32_t * pval = (uint32_t *) arg; // Result storage
    uint32_t   cnt  = 0;
    char       buf[64];

    while (cnt < REP_COUNT) {
        int32_t ret = xos_mutex_lock(&test_mutex_1);

        if (ret != XOS_OK) {
            printf("ERROR: xos_mutex_lock() ret %d\n", ret);
            return cnt;
        }

        cnt++;

        // Now do something for long enough to encounter a context switch.
        // sprintf() is conveniently long.
        sprintf(buf, "Thread %s got mutex cnt = %u\n", xos_thread_get_name(xos_thread_id()), cnt);
        *pval = cnt;
        xos_mutex_unlock(&test_mutex_1);
   }

    return cnt;
}


//-----------------------------------------------------------------------------
// Mutex multi-thread test.
//-----------------------------------------------------------------------------
static int32_t
mutex_thread_test(int32_t flag)
{
    uint32_t cnt[NUM_THREADS];
    uint32_t total = 0;
    int32_t  val;
    int32_t  ret;
    int32_t  i;
    
    if (flag) {
        ret = xos_mutex_create(&test_mutex_1, XOS_MUTEX_WAIT_FIFO, 0);
    }
    else {
        ret = xos_mutex_create(&test_mutex_1, XOS_MUTEX_WAIT_PRIORITY, 6);
    }
    
    if (ret != XOS_OK) {
        return ret;
    }

    for (i = 0; i < NUM_THREADS; i++) {
        cnt[i] = 0;
        ret = xos_thread_create(&test_threads[i],
                                0,
                                mutex_get_func,
                                &cnt[i],
                                t_names[i],
                                test_stacks[i],
                                STACK_SIZE,
                                flag ? 2 : 5,
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
        if (val != REP_COUNT) {
            printf("ERROR: thread count mismatch\n");
            return 1;
        }
        total += val;
    }

    if (total != NUM_THREADS * REP_COUNT) {
        printf("ERROR: total count mismatch\n");
        return 1;
    }

    xos_list_threads();

    for (i = 0; i < NUM_THREADS; i++) {
        xos_thread_delete(&test_threads[i]);
    }
    xos_mutex_delete(&test_mutex_1);

    return 0;
}


//-----------------------------------------------------------------------------
// Thread function to check mutex timeout operation.
//-----------------------------------------------------------------------------
static int32_t 
mutex_timeout_func(void * arg, int32_t unused)
{
    int32_t    ret;
    uint32_t   cycles = 120000;
    XosMutex * mtx    = (XosMutex *) arg;

    // Check that mutex is locked.
    ret = xos_mutex_test(mtx);
    if (ret != 1) {
        return -1;
    }

    // These should fail, because the parent thread already has the
    // mutex locked. So we should get a timeout return.
    while (cycles > 100) {
        ret = xos_mutex_lock_timeout(mtx, cycles);
        if (ret != XOS_ERR_TIMEOUT) {
            return -1;
        }
        cycles /= 2;
    }

    // This should also fail.
    ret = xos_mutex_trylock(mtx);
    if (ret != XOS_ERR_MUTEX_LOCKED) {
        return -1;
    }

    // Try to unlock the mutex. Should fail because currently locked
    // by another thread.
    ret = xos_mutex_unlock(mtx);
    if (ret != XOS_ERR_MUTEX_NOT_OWNED) {
        return -1;
    }

    // Try to destroy the mutex. Should fail because currently locked
    // by another thread.
    ret = xos_mutex_delete(mtx);
    if (ret != XOS_ERR_MUTEX_NOT_OWNED) {
        return -1;
    }

    return XOS_OK;
}


//-----------------------------------------------------------------------------
// Timeout test.
//-----------------------------------------------------------------------------
static int32_t
mutex_timeout_test(void)
{
    XosMutex mtx2;
    int32_t  ret;
    int32_t  val;

    ret = xos_mutex_create(&mtx2, 0, 0);
    if (ret != XOS_OK) {
        return ret;
    }

    // Attempt lock_timeout with invalid parameter.
    ret = xos_mutex_lock_timeout(NULL, 2000);
    if (ret != XOS_ERR_INVALID_PARAMETER) {
        return -1;
    }

    // Lock the mutex. Use the timeout function because we need to cover the
    // case where the lock_timeout succeeds.
    ret = xos_mutex_lock_timeout(&mtx2, 2000);
    if (ret != XOS_OK) {
        return ret;
    }

    // Lock it one more time to check that recursive lock_timeout works.
    ret = xos_mutex_lock_timeout(&mtx2, 2000);
    if (ret != XOS_OK) {
        return ret;
    }

    // Create the test thread
    ret = xos_thread_create(&test_threads[0],
                            0,
                            mutex_timeout_func,
                            &mtx2,
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
    if (ret != XOS_OK) {
        return ret;
    }
    ret = xos_thread_delete(&test_threads[0]);
    if (ret != XOS_OK) {
        return ret;
    }

    // Destroy while locked, should succeed.
    ret = xos_mutex_delete(&mtx2);
    if (ret != XOS_OK) {
        return ret;
    }

    return val;
}


//-----------------------------------------------------------------------------
// Low-pri thread function.
//-----------------------------------------------------------------------------
static int32_t
low_func(void * arg, int32_t unused)
{
    int32_t    cnt;
    int32_t    ret;

    for (cnt = 0; cnt < 10; cnt++) {
        if (xos_thread_get_priority(XOS_THREAD_SELF) != LOW_PRI) {
            printf("Fail: low thread not started at low pri.\n");
            exit(-1);
        }

        // Take the mutex. Then wait until thread priority is raised.
        xos_mutex_lock(&mtx3);
        xos_sem_put(&sem3);
        while (xos_thread_get_priority(XOS_THREAD_SELF) != HIGH_PRI);

        if (xos_thread_get_priority(XOS_THREAD_SELF) != HIGH_PRI) {
            printf("Fail: low thread not raised to high pri.\n");
            exit(-1);
        }

        // Release. This should lower priority back to original.
        ret = xos_mutex_unlock(&mtx3);
    }

    return ret;
}


//-----------------------------------------------------------------------------
// High-pri thread function.
//-----------------------------------------------------------------------------
static int32_t
high_func(void * arg, int32_t unused)
{
    int32_t    cnt;
    int32_t    ret;

    for (cnt = 0; cnt < 10; cnt++) {
        // First get the semaphore, to sync.
        xos_sem_get(&sem3);

        // This should block and raise the owner's priority.
        ret = xos_mutex_lock(&mtx3);

        // Now release the mutex.
        ret = xos_mutex_unlock(&mtx3);
    }

    // Get sem again.
    xos_sem_get(&sem3);

    ret = xos_mutex_lock(&mtx3);
    if (ret != XOS_ERR_MUTEX_DELETE) {
        return -1;
    }

    return XOS_OK;
}


//-----------------------------------------------------------------------------
// Priority inversion test.
//-----------------------------------------------------------------------------
static int32_t
mutex_priority_test(void)
{
    int32_t  ret;
    int32_t  val;

    // Create the mutex (without priority protection so that low thread's
    // priority actually changes only when high thread blocks) and the sem.
    xos_mutex_create(&mtx3, 0, 0);
    xos_sem_create(&sem3, 0, 0);

    // Create the low pri thread
    ret = xos_thread_create(&test_threads[0],
                            0,
                            low_func,
                            &mtx3,
                            "low",
                            test_stacks[0],
                            STACK_SIZE,
                            LOW_PRI,
                            0,
                            0);
    if (ret != XOS_OK) {
        return ret;
    }

    // Create the high pri thread
    ret = xos_thread_create(&test_threads[1],
                            0,
                            high_func,
                            &mtx3,
                            "high",
                            test_stacks[1],
                            STACK_SIZE,
                            HIGH_PRI,
                            0,
                            0);
    if (ret != XOS_OK) {
        return ret;
    }

    // Wait for low pri thread to finish
    ret = xos_thread_join(&test_threads[0], NULL);

    // Lock the mutex and wait until high pri thread is blocked on it
    ret = xos_mutex_lock(&mtx3);
    if (ret != XOS_OK) {
        return ret;
    }
    xos_sem_put(&sem3);
    while (mtx3.waitq.head == NULL) {
        xos_thread_yield();
    }

    if (xos_thread_get_state(&test_threads[1]) != XOS_THREAD_STATE_BLOCKED) {
        return -1;
    }

    // Destroy mutex and verify that blocked thread is unblocked and finishes
    ret = xos_mutex_delete(&mtx3);
    if (ret != XOS_OK) {
        return ret;
    }

    ret = xos_thread_join(&test_threads[1], &val);
    return val;
}


//-----------------------------------------------------------------------------
// Test main function.
//-----------------------------------------------------------------------------
int32_t
mutex_test(void * arg, int32_t unused)
{
    int32_t  ret;

    printf("Mutex test starting\n");

    // Run basic checks from thread
    ret = mutex_basic_checks();
    if (ret) {
        printf("ERROR: mutex basic checks failed\n");
        exit(ret);
    }
    printf("### MTX basic test 1 ....................... PASS\n");

    // Run timeout test if we have a timer
    if (have_timer) {
        ret = mutex_timeout_test();
        if (ret != XOS_OK) {
            printf("ERROR: mutex timeout test failed\n");
            exit(ret);
        }
    }
    printf("### MTX timeout test ....................... PASS\n");

    // Run thread test with wait mode = priority
    ret = mutex_thread_test(0);
    if (ret != XOS_OK) {
        printf("ERROR: mutex thread test 1 failed\n");
        exit(ret);
    }
    printf("### MTX thread test 1 ...................... PASS\n");

    // Run thread test with wait mode = fifo
    ret = mutex_thread_test(1);
    if (ret != XOS_OK) {
        printf("ERROR: mutex thread test 2 failed\n");
        exit(ret);
    }
    printf("### MTX thread test 2 ...................... PASS\n");

    ret = mutex_priority_test();
    if (ret != XOS_OK) {
        printf("ERROR: mutex priority test failed\n");
        exit(ret);
    }
    printf("### MTX priority test ...................... PASS\n");

    exit(0);
    return 0;
}


//-----------------------------------------------------------------------------
// Main.
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
    // This call converts main() into the init thread
    xos_start_main_ex("main", 7, STACK_SIZE);

    // Start timer if available
    if (xos_start_system_timer(-1, TICK_CYCLES) != XOS_OK) {
        have_timer = 0;
    }

    // Run test
    ret = mutex_test(0, 0);
    return ret;
#else
    // Create the main thread
    ret = xos_thread_create(&mutex_test_tcb,
                            0,
                            mutex_test,
                            0,
                            "mutex_test",
                            mutex_test_stk,
                            STACK_SIZE,
                            7,
                            0,
                            0);

    // Start timer after at least one thread has been created
    if (xos_start_system_timer(-1, TICK_CYCLES) != XOS_OK) {
        have_timer = 0;
    }

    // Start scheduling, does not return
    xos_start(0);
    return -1; // should never get here
#endif
}

