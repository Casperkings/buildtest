
// cond_test.c - XOS condition object test

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


#define REP_COUNT    100

XosThread cond_test_tcb;
uint8_t   cond_test_stk[STACK_SIZE + 500];
XosThread test_thread;
uint8_t   test_stack[STACK_SIZE + 500];

static XosCond  test_cond;
static XosMutex test_mutex;
static XosSem   test_sem;

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
cond_basic_checks(int32_t from_int)
{
    int32_t ret;

    // Try create and delete with invalid parameter
    ret = xos_cond_create(NULL);
    if (ret != XOS_ERR_INVALID_PARAMETER) {
        return -1;
    }
    ret = xos_cond_delete(NULL);
    if (ret != XOS_ERR_INVALID_PARAMETER) {
        return -1;
    }

    ret = xos_cond_create(&test_cond);
    if (ret != XOS_OK) {
        puts("Fail: create failed");
        return -1;
    }

    ret = xos_cond_signal(0, 0);
    if (ret == XOS_OK) {
        puts("Fail: wait with NULL cond");
        return -1;
    }

    ret = xos_cond_signal(&test_cond, 0);
    if (ret != XOS_OK) {
        puts("Fail: signal with valid cond");
        return -1;
    }

    ret = xos_cond_signal_one(&test_cond, 0);
    if (ret != XOS_OK) {
        puts("Fail: signal_1 with valid cond");
        return -1;
    }

    if (!from_int && have_timer) {
        XosMutex mtx;

        ret = xos_cond_wait_mutex_timeout(NULL, NULL, 5000);
        if (ret != XOS_ERR_INVALID_PARAMETER) {
            puts("Fail: wait_timeout");
            return -1;
        }

        xos_mutex_create(&mtx, 0, 0);
        ret = xos_cond_wait_mutex_timeout(&test_cond, &mtx, 5000);
        if (ret != XOS_ERR_MUTEX_NOT_OWNED) {
            puts("Fail: wait_timeout");
            return -1;
        }

        xos_mutex_lock(&mtx);
        ret = xos_cond_wait_mutex_timeout(&test_cond, &mtx, 5000);
        if (ret != XOS_ERR_TIMEOUT) {
            puts("Fail: wait_timeout");
            return -1;
        }
        xos_mutex_unlock(&mtx);

        ret = xos_cond_wait(0, 0, 0);
        if (ret == XOS_OK) {
            puts("Fail: wait with NULL cond");
            return -1;
        }

        ret = xos_cond_wait_mutex(0, 0);
        if (ret == XOS_OK) {
            puts("Fail: wait with NULL cond/mutex");
            return -1;
        }

        ret = xos_cond_wait_mutex(&test_cond, 0);
        if (ret == XOS_OK) {
            puts("Fail: wait with NULL mutex");
            return -1;
        }

        ret = xos_cond_wait_mutex(&test_cond, &mtx);
        if (ret == XOS_OK) {
            puts("Fail: wait with unlocked mutex");
            return -1;
        }

        xos_mutex_delete(&mtx);
    }

    ret = xos_cond_delete(&test_cond);
    return ret;
}


//-----------------------------------------------------------------------------
// Timer callback for test_basic.
//-----------------------------------------------------------------------------
void
cond_timer_cb(void * arg)
{
    int32_t * p = (int32_t *) arg;

    *p = cond_basic_checks(1);
    flag = 1;
}


//-----------------------------------------------------------------------------
// Condition callback.
//-----------------------------------------------------------------------------
static int32_t
cond_cb(void * arg, int32_t sig_value, XosThread * thread)
{
    int32_t * p = (int32_t *) arg;

    (*p)++;
    return 1;
}

//-----------------------------------------------------------------------------
// Thread function for cond thread test.
//-----------------------------------------------------------------------------
static int32_t
cond_wait_func(void * arg, int32_t unused)
{
    int32_t ret;
    int32_t i;
    int32_t j;

    // 2-step loop: wait_mutex, wait_mutex_timeout
    for (i = 0; i < REP_COUNT; i++) {

        // Step 1: wait_mutex
        // Try to wait without locking the mutex, should fail
        ret = xos_cond_wait_mutex(&test_cond, &test_mutex);
        if (ret != XOS_ERR_MUTEX_NOT_OWNED) {
            break;
        }

        // Ensure no contention for flag
        xos_mutex_lock(&test_mutex);
        flag = 1;

        // Wake sender thread
        xos_sem_put(&test_sem);

        // Release mutex and wait
        ret = xos_cond_wait_mutex(&test_cond, &test_mutex);
        if (ret != 0) {
            break;
        }
        xos_mutex_unlock(&test_mutex);

        // Step 2: wait_mutex_timeout
        xos_mutex_lock(&test_mutex);
        flag = 1;

        xos_sem_put(&test_sem);
        if (have_timer) {
            ret = xos_cond_wait_mutex_timeout(&test_cond, &test_mutex, 1000000);
        }
        else {
            ret = xos_cond_wait_mutex(&test_cond, &test_mutex);
        }
        if (ret != 0) {
            break;
        }
        xos_mutex_unlock(&test_mutex);
    }

    if (i < REP_COUNT) {
        //puts("fail");
        // Delete sem to wait up waiter
        xos_sem_delete(&test_sem);
        return -1;
    }

    // Now check the cond_wait call with callback function
    j = -1;
    for (i = 0; i < REP_COUNT; i++) {
        ret = xos_cond_wait(&test_cond, cond_cb, &j);
        if ((ret != i) || (j != i)) {
            return -1;
        }
    }

    // Now block one last time and be woken when cond is deleted
    ret = xos_cond_wait(&test_cond, NULL, NULL);
    if (ret != XOS_ERR_COND_DELETE) {
        return -1;
    }

    //puts("done");
    return 0;
}


//-----------------------------------------------------------------------------
// Test driver function.
//-----------------------------------------------------------------------------
int32_t
cond_test(void * arg, int32_t unused)
{
    XosTimer timer1;
    uint32_t cnt;
    int32_t  ret;
    int32_t  i;

    puts("Condition test starting");

    // Run basic checks from thread
    ret = cond_basic_checks(0);
    if (ret) {
        puts("ERROR: basic checks failed");
        exit(ret);
    }
    puts("### COND basic checks 1 ..................... PASS");

    // Run basic checks from timer callback
    if (have_timer) {
        xos_timer_init(&timer1);
        xos_timer_start(&timer1, 1000, 0, cond_timer_cb, (void *)&ret);

        while (!flag);
        if (ret != XOS_OK) {
            puts("ERROR: basic checks from intr callback failed");
            exit(ret);
        }
        flag = 0;
    }
    puts("### COND basic checks 2 ..................... PASS");

    xos_cond_create(&test_cond);
    ret = xos_mutex_create(&test_mutex, 0, 0);
    ret = xos_sem_create(&test_sem, 0, 0);

    ret = xos_thread_create(&test_thread,
                            0,
                            cond_wait_func,
                            &cnt,
                            "thread2",
                            test_stack,
                            STACK_SIZE,
                            7,
                            0,
                            0);
    if (ret) {
        puts("ERROR: creating test thread");
        exit(ret);
    }

    for (i = 0; i < (REP_COUNT * 2); i++) {
        // If receiver thread exited then we had an error
        if (xos_thread_get_state(&test_thread) == XOS_THREAD_STATE_EXITED) {
            puts("ERROR: test thread exited");
            exit(-1);
        }

        // Sync with receiver thread
        ret = xos_sem_get(&test_sem);
        if (ret != XOS_OK) {
            puts("ERROR: sem wait failed");
            exit(ret);
        }

        // Check and clear flag protected
        xos_mutex_lock(&test_mutex);
        if (flag != 1) {
            puts("ERROR: flag value incorrect");
            exit(-1);
        }
        flag = 0;
        xos_mutex_unlock(&test_mutex);

        // Wake receiver
        xos_cond_signal(&test_cond, 0);
    }

    for (i = 0; i < REP_COUNT; i++) {
        xos_thread_state_t state;

        while (1) {
            state = xos_thread_get_state(&test_thread);

            if (state == XOS_THREAD_STATE_EXITED) {
                puts("ERROR: test thread exited");
                exit(-1);
            }
            if (state == XOS_THREAD_STATE_BLOCKED) {
                break;
            }
            xos_thread_yield();    
        }

        xos_cond_signal_one(&test_cond, i);
    }

    // Check that blocked thread is unblocked when cond is deleted
    while (xos_thread_get_state(&test_thread) != XOS_THREAD_STATE_BLOCKED) {
        xos_thread_yield();
    }

    xos_cond_delete(&test_cond);
    ret = xos_thread_join(&test_thread, &i);
    if ((ret != XOS_OK) || (i != 0)) {
        puts("ERROR: test thread termination");
        exit(-1);
    }

    xos_thread_delete(&test_thread);
    puts("### COND thread test ........................ PASS");

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

    ret = xos_start_system_timer(-1, TICK_CYCLES);
    if (ret != XOS_OK) {
        have_timer = 0;
    }

    return cond_test(0, 0);
#else
    ret = xos_thread_create(&cond_test_tcb,
                            0,
                            cond_test,
                            0,
                            "cond_test",
                            cond_test_stk,
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

