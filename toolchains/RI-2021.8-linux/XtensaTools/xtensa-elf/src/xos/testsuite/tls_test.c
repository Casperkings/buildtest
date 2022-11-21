
// tls_test.c - XOS thread local storage test

// Copyright (c) 2019 Cadence Design Systems, Inc.
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

// This test only does something useful if thread local storage is enabled.
#if XOS_OPT_THREAD_LOCAL_STORAGE

#define NUM_THREADS	20
#define NUM_ITERS       20


// Creates multiple threads over and over, passes them a pointer to
// a static variable. Each thread saves the pointer in a TLS key and
// the pointer is used by the key's cleanup function when the thread
// exits, incrementing the variable each time. Any incorrect sharing
// of pointers between threads will result in a bad final counter
// value.

#if !USE_MAIN_THREAD
XosThread tls_test_tcb;
uint8_t   tls_test_stk[STACK_SIZE];
#endif
XosThread test_threads[NUM_THREADS];
uint8_t   stacks[NUM_THREADS][STACK_SIZE];


uint32_t   key;
uint32_t   oldkey;
uint32_t   keys[XOS_TLS_NUM_KEYS];


// TLS destructor function
void
tls_func(void * p)
{
    uint32_t * pu = (uint32_t *) p;

    (*pu)++;
}


// Basic API checks
static void
check_api(void)
{
    uint32_t k;
    int32_t  ret;
    int32_t  i;
    void *   p;

    // Check invalid params to API functions

    ret = xos_tls_create(NULL, NULL);
    if (ret == XOS_OK) {
        printf("FAIL: expected create error for null key ptr\n");
        exit(-1);
    }

    p = xos_tls_get(XOS_TLS_NUM_KEYS);
    if (p != XOS_NULL) {
        printf("FAIL: get returned non-null for invalid key\n");
        exit(-1);
    }

    ret = xos_tls_set(XOS_TLS_NUM_KEYS, NULL);
    if (ret == XOS_OK) {
        printf("FAIL: expected set error for invalid key\n");
        exit(-1);
    }

    ret = xos_tls_delete(XOS_TLS_NUM_KEYS);
    if (ret == XOS_OK) {
        printf("FAIL: expected delete error for invalid key\n");
        exit(-1);
    }

    // Create and set all valid keys

    for (i = 0; i < XOS_TLS_NUM_KEYS; i++) {
        ret = xos_tls_create(&keys[i], tls_func);
        if (ret != XOS_OK) {
            printf("FAIL: key create %d failed\n", i);
            exit(-1);
        }
        ret = xos_tls_set(keys[i], (void *) i);
        if (ret != XOS_OK) {
            printf("FAIL: error setting key %d\n", i);
            exit(-1);
        }
    }

    // Try to create one more, should fail

    ret = xos_tls_create(&k, NULL);
    if (ret != XOS_ERR_LIMIT) {
        printf("FAIL: expected limit error on create\n");
        exit(-1);
    }

    // Check previously set values

    for (i = 0; i < XOS_TLS_NUM_KEYS; i++) {
        p = xos_tls_get(keys[i]);
        if (p != (void *) i) {
            printf("FAIL: get value mismatch %d\n", i);
            exit(-1);
        }
    }

    // Keys will be deleted by caller.
}


// Test thread function
int32_t
test_func(void * arg, int32_t unused)
{
    if (xos_tls_get(key) != XOS_NULL) {
        printf("FAIL: key value not NULL on thread entry\n");
        exit(-1);
    }

    xos_tls_set(key, arg);
    return 0;
}


// Run TLS test
int32_t
tls_test(void * arg, int32_t unused)
{
    static uint32_t * counters;

    int32_t i;
    int32_t n;
    int32_t ret;
    int32_t pri = 0;

    // Basic API checks
    check_api();

    // Keys have been created in check_api
    key = keys[0];

    for (i = 0; i < NUM_ITERS; i++) {
        // Allocate new memory for counters
        counters = calloc(NUM_THREADS, sizeof(uint32_t));

        // Create a new batch of threads
        for (n = 0; n < NUM_THREADS; n++) {
            ret = xos_thread_create(&(test_threads[n]),
                                    0,
                                    test_func,
                                    &counters[n],
                                    "xxx",
                                    stacks[n],
                                    STACK_SIZE,
                                    (pri++ % 5),
                                    0,
                                    0);
            if (ret) {
                xos_fatal_error(-1, "Error creating test threads\n");
            }
        }

        // Now wait for them to finish
        for (n = 0; n < NUM_THREADS; n++) {
            xos_thread_join(&(test_threads[n]), &ret);

            if (test_threads[n].tls[key] != XOS_NULL) {
                printf("FAIL: thread TLS value not cleared at thread exit\n");
                exit(-1);
            }
        }

        if ((i == NUM_ITERS/2) && (XOS_TLS_NUM_KEYS > 1)) {
            // Use another key
            oldkey = key;
            key = keys[1];
        }

        // Check counters
        for (n = 0; n < NUM_THREADS; n++) {
            if (counters[n] != 1) {
                printf("FAIL: counters[%d] = %u exp 1\n", n, counters[n]);
                exit(-1);
            }
        }

        // Don't free, ensure that new allocation is at a different address.
        // free(counters);
    }

    // Delete keys
    for (i = 0; i < XOS_TLS_NUM_KEYS; i++) {
        ret = xos_tls_delete(keys[i]);
        if (ret != XOS_OK) {
            printf("FAIL: error on delete %d\n", i);
            exit(-1);
        }
    }

    printf("PASS\n");
    exit(0);

#if !USE_MAIN_THREAD
    exit(0);
#endif
    return 0;
}

#endif // XOS_OPT_THREAD_LOCAL_STORAGE

int
main()
{
#if XOS_OPT_THREAD_LOCAL_STORAGE

    int32_t ret;

#if defined BOARD
    xos_set_clock_freq(xtbsp_clock_freq_hz());
#else
    xos_set_clock_freq(XOS_CLOCK_FREQ);
#endif

#if USE_MAIN_THREAD
    xos_start_main_ex("main", 7, STACK_SIZE);
    xos_start_system_timer(-1, xos_get_clock_freq()/100);

    ret = tls_test(0, 0);
    return ret;
#else
    ret = xos_thread_create(&tls_test_tcb,
                            0,
                            tls_test,
                            0,
                            "tls_test",
                            tls_test_stk,
                            STACK_SIZE,
                            7,
                            0,
                            0);

    xos_start_system_timer(-1, TICK_CYCLES);
    xos_start(0);
    return -1; // should never get here
#endif

#else // No TLS

    printf("Thread local storage not enabled, nothing to do.\n");
    printf("PASS\n");
    return 0;

#endif
}

