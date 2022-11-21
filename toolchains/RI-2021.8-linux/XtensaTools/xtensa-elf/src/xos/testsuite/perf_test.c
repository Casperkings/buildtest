
// perf_test.c - measure XOS operation timings.

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


#include <string.h>
#include "test.h"


// Use dynamic ticks mode to avoid timer interrupts adding an unknown
// amount of overhead into the measurements.
#define USE_DYNAMIC_TICKS

#define TEST_ITER       5000
#define TEST_ITER2      500


XosThread perf_test_tcb;
uint8_t   perf_test_stk[STACK_SIZE];

XosThread thd1;
XosThread thd2;
XosThread thd3;

uint8_t stack1[STACK_SIZE];
uint8_t stack2[STACK_SIZE];
uint8_t stack3[STACK_SIZE];

uint32_t clock_vals[TEST_ITER2 * 3];
volatile uint32_t indx;

XosSem   test_sem_1;
XosMutex test_mtx_1;
XosEvent test_event_1;

volatile uint32_t test_total;
volatile uint32_t test_max;
volatile uint32_t test_start;

// Test queue has 16 elements of size 16 bytes each.
XOS_MSGQ_ALLOC(test_msgq, 16, 16);

// Pool and memory for block memory pool test.
XosBlockPool pool;
uint8_t      pmem[32 * 16];

// Print format flag
int32_t csv = 0;


//-----------------------------------------------------------------------------
// Conditional print in CSV or readable format.
//-----------------------------------------------------------------------------
void
cprint(char * testname, char * desc, uint32_t avgc, uint32_t maxc)
{
    if (csv) {
        printf("%s_Avg,%u,,\"%s (avg)\"\n", testname, avgc, desc);
        printf("%s_Max,%u,,\"%s (max)\"\n", testname, maxc, desc);
    }
    else {
        printf("%-40s: avg %u max %u cycles\n", desc, avgc, maxc);
    }
}


//-----------------------------------------------------------------------------
// Thread function for yield test. Store current cycle count and yield the
// CPU right away.
//-----------------------------------------------------------------------------
int32_t
yield_func(void * arg, int32_t unused)
{
    int32_t i;

    for (i = 0; i < TEST_ITER2; i++) {
        clock_vals[indx++] = xos_get_ccount();
        xos_thread_yield();
    }

    return 0;
}


//-----------------------------------------------------------------------------
// Yield test - runs in main thread. Start 3 threads to measure the context
// switch time. Wait for them all to exit. Then compute the average and worst
// case switch times.
//-----------------------------------------------------------------------------
int
yield_test()
{
    int32_t  ret;
    uint32_t oh_cycles;
    uint32_t sum = 0;
    uint32_t max = 0;
    uint32_t cnt;
    int32_t  i;
    
    if (!csv) {
        printf("\nYield timing test"
               "\n-----------------\n");
    }

    // Measure the overhead of updating the cycle count

    for (i = 0; i < 10; i++) {
        uint32_t c1 = xos_get_ccount();

        clock_vals[indx++] = c1;
        cnt = xos_get_ccount() - c1;
    }

//    if (!csv)
//        printf("Overhead is %u cycles\n", cnt);

    oh_cycles = cnt;
    indx = 0; 
    
    // Launch test threads

    ret = xos_thread_create(&thd1, 0, yield_func, 0, "thd1",
                            stack1, STACK_SIZE, 5, 0, 0);
    if (ret) {
        xos_fatal_error(-1, "Error creating test threads\n");
    }
    
    ret = xos_thread_create(&thd2, 0, yield_func, 0, "thd2",
                            stack2, STACK_SIZE, 5, 0, 0);
    if (ret) {
        xos_fatal_error(-1, "Error creating test threads\n");
    }
    
    ret = xos_thread_create(&thd3, 0, yield_func, 0, "thd3",
                            stack3, STACK_SIZE, 5, 0, 0);
    if (ret) {
        xos_fatal_error(-1, "Error creating test threads\n");
    }

    // Wait for them all to finish
    ret = xos_thread_join(&thd1, &ret);
    if (ret) {
        xos_fatal_error(-1, "Error joining thread thd1\n");
    }
    ret = xos_thread_join(&thd2, &ret);
    if (ret) {
        xos_fatal_error(-1, "Error joining thread thd2\n");
    }
    ret = xos_thread_join(&thd3, &ret);
    if (ret) {
        xos_fatal_error(-1, "Error joining thread thd3\n");
    }

    // Compute average and worst case values
    for (i = 1; i < indx; i++) {
        uint32_t delta = clock_vals[i] - clock_vals[i-1];

        sum += delta;
        if (delta > max)
            max = delta;
    }

    cprint("Sol_CtxSw", "Solicited context switch time",
           ((sum + indx - 2) / (indx - 1)) - oh_cycles, max - oh_cycles);

    xos_thread_delete(&thd1);
    xos_thread_delete(&thd2);
    xos_thread_delete(&thd3);
    return 0;
}


//-----------------------------------------------------------------------------
// Helper thread for semaphore tests.
//-----------------------------------------------------------------------------
int32_t
sem_get(void * arg, int32_t unused)
{
    uint32_t end;
    uint32_t i;

    for(i = 0; i < TEST_ITER; i++) {
        xos_sem_get(&test_sem_1);
        end = xos_get_ccount();
        if (test_start) {
            uint32_t delta = end - test_start;

            test_total += delta;
            test_max = test_max >= delta ? test_max : delta;
        }
    }

    return 0;
}


//-----------------------------------------------------------------------------
// Semaphore operations timing test.
//-----------------------------------------------------------------------------
int32_t
sem_test(void * arg, int32_t unused)
{
    uint32_t start;
    uint32_t delta;
    uint32_t total = 0;
    uint32_t max   = 0;
    uint32_t i;
    int32_t  ret;

    if (!csv) {
        printf("\nSemaphore timing test"
               "\n---------------------\n");
    }

    xos_sem_create(&test_sem_1, XOS_SEM_WAIT_PRIORITY, 0);

    // First, just measure how long it takes to get and put a semaphore
    // with no contention and no waits/wakeups.

    for(i = 0; i < TEST_ITER; i++) {
        start = xos_get_ccount();
        xos_sem_put(&test_sem_1);
        delta = xos_get_ccount() - start;
        total += delta;
        max = max >= delta ? max : delta;
    }

    cprint("Sem_Put_Nowake", "Semaphore put with no wake", total/TEST_ITER, max);

    // Measure time to get a semaphore with no contention and no waits/
    // wakeups.
    total = 0;
    max = 0;

    for(i = 0; i < TEST_ITER; i++) {
        start = xos_get_ccount();
        xos_sem_get(&test_sem_1);
        delta = xos_get_ccount() - start;
        total += delta;
        max = max >= delta ? max : delta;
    }

    cprint("Sem_Get_NoCont", "Semaphore get with no contention", total/TEST_ITER, max);

    // Now measure the time taken to put a semaphore when a lower priority
    // thread has to be unblocked.

    ret = xos_thread_create(&thd2, 0, sem_get, 0, "sem_get",
                            stack2, STACK_SIZE, 4, 0, 0);
    if (ret) {
        printf("FAIL: error creating test threads\n");
        exit(-1);
    }

    total = 0;
    max = 0;

    for(i = 0; i < TEST_ITER; i++) {
        // Yield CPU so that lower priority thread can run and block itself
        // on the semaphore.
        xos_thread_sleep(2000);
        start = xos_get_ccount();
        xos_sem_put(&test_sem_1);
        delta = xos_get_ccount() - start;
        total += delta;
        max = max >= delta ? max : delta;
    }

    ret = xos_thread_join(&thd2, &ret);
    if (ret) {
        printf("FAIL: error while waiting for test thread.\n");
        exit(-1);
    }

    cprint("Sem_Put_Wake", "Semaphore put with thread wake", total/TEST_ITER, max);

    // Now measure the time taken to put a semaphore + context switch when
    // a higher priority thread is unblocked.

    ret = xos_thread_delete(&thd2);
    ret = xos_thread_create(&thd2, 0, sem_get, 0, "sem_get",
                            stack2, STACK_SIZE, 6, 0, 0);
    if (ret) {
        printf("FAIL: error creating test threads\n");
        exit(-1);
    }

    test_total = 0;
    test_max = 0;

    for(i = 0; i < TEST_ITER; i++) {
        // No need to yield CPU here since the other thread is at a higher priority.
        test_start = xos_get_ccount();
        xos_sem_put(&test_sem_1);
    }

    ret = xos_thread_join(&thd2, &ret);
    if (ret) {
        printf("FAIL: error while waiting for test thread.\n");
        exit(-1);
    }

    cprint("Sem_Put_CtxSw", "Semaphore put with context switch", test_total/TEST_ITER, test_max);

    return 0;
}


//-----------------------------------------------------------------------------
// Semaphore test.
//-----------------------------------------------------------------------------
int32_t
sem_perf_test()
{
    int32_t  ret;

    ret = xos_thread_create(&thd1, 0, sem_test, 0, "sem_test",
                            stack1, STACK_SIZE, 5, 0, 0);
    if (ret) {
        printf("FAIL: Error creating test threads\n");
        exit(-1);
    }

    ret = xos_thread_join(&thd1, &ret);
    if (ret) {
        printf("FAIL: Error joining thread sem_test\n");
        exit(-1);
    }

    //xos_list_threads();
    xos_thread_delete(&thd1);
    xos_thread_delete(&thd2);
    return 0;
}


//-----------------------------------------------------------------------------
// Helper thread for mutex tests.
//-----------------------------------------------------------------------------
int32_t
mutex_get(void * arg, int32_t unused)
{
    uint32_t i; 

    for(i = 0; i < TEST_ITER; i++) {
        xos_mutex_lock(&test_mtx_1);
        xos_mutex_unlock(&test_mtx_1);
    }

    return 0;
}


//-----------------------------------------------------------------------------
// Helper thread 2 for mutex tests.
//-----------------------------------------------------------------------------
int32_t
mutex_get2(void * arg, int32_t unused)
{
    uint32_t delta;
    uint32_t i;

    for(i = 0; i < TEST_ITER; i++) {
        // First get the semaphore to sync with lower priority thread
        xos_sem_get(&test_sem_1);
        // Now block on the mutex
        xos_mutex_lock(&test_mtx_1);
        delta = xos_get_ccount() - test_start;
        test_total += delta;
        test_max = test_max >= delta ? test_max : delta;
        xos_mutex_unlock(&test_mtx_1);
    }

    return 0;
}


//-----------------------------------------------------------------------------
// Mutex operations timing test.
//-----------------------------------------------------------------------------
int32_t
mutex_test(void * arg, int32_t unused)
{
    uint32_t start;
    uint32_t delta;
    uint32_t total;
    uint32_t max;
    uint32_t i;
    int32_t  ret;

    if (!csv) {
        printf("\nMutex timing test"
               "\n-----------------\n");
    }

    xos_mutex_create(&test_mtx_1, XOS_MUTEX_WAIT_PRIORITY, 0);
    xos_sem_create(&test_sem_1, XOS_SEM_WAIT_PRIORITY, 0);

    // First, just measure how long it takes to lock a mutex with no contention
    // and no waits/wakeups. Also measure how long it takes to unlock.

    test_total = 0;
    test_max = 0;
    total = 0;
    max = 0;

    for(i = 0; i < TEST_ITER; i++) {
        start = xos_get_ccount();
        xos_mutex_lock(&test_mtx_1);
        delta = xos_get_ccount() - start;
        test_total += delta;
        test_max = test_max >= delta ? test_max : delta;

        start = xos_get_ccount();
        xos_mutex_unlock(&test_mtx_1);
        delta = xos_get_ccount() - start;
        total += delta;
        max = max >= delta ? max : delta;
    }

    cprint("Mtx_Lock_NoCont", "Mutex lock with no contention",
           test_total/TEST_ITER, test_max);
    cprint("Mtx_Unlock_NoWake", "Mutex unlock with no wake",
           total/TEST_ITER, max);

    // Now measure the time taken to unlock a mutex when a lower priority
    // thread has to be unblocked.

    ret = xos_thread_create(&thd2, 0, mutex_get, 0, "mutex_get",
                            stack2, STACK_SIZE, 4, 0, 0);
    if (ret) {
        printf("FAIL: error creating test threads\n");
        exit(-1);
    }

    test_total = 0;
    test_max = 0;

    for(i = 0; i < TEST_ITER; i++) {
        xos_mutex_lock(&test_mtx_1);
        // Let the other thread run so that it can block on the mutex
        xos_thread_sleep(2000);
        start = xos_get_ccount();
        xos_mutex_unlock(&test_mtx_1);
        delta = xos_get_ccount() - start;
        test_total += delta;
        test_max = test_max >= delta ? test_max : delta;
    }

    ret = xos_thread_join(&thd2, &ret);
    if (ret) {
        printf("FAIL: error while waiting for test thread.\n");
        exit(-1);
    }

    cprint("Mtx_Unlock_Wake", "Mutex unlock with thread wake",
           test_total/TEST_ITER, test_max);

    // Now measure the time taken to unlock a mutex + context switch when
    // a higher priority thread is unblocked.

    ret = xos_thread_delete(&thd2);
    ret = xos_thread_create(&thd2, 0, mutex_get2, 0, "mutex_get2",
                            stack2, STACK_SIZE, 6, 0, 0);
    if (ret) {
        printf("FAIL: error creating test threads\n");
        exit(-1);
    }

    test_total = 0;
    test_max = 0;

    for(i = 0; i < TEST_ITER; i++) {
        xos_mutex_lock(&test_mtx_1);
        // Now signal the other thread with the semaphore. This will cause an
        // immdeiate switch to the other thread, which will then block on the
        // mutex and give back control to here.
        xos_sem_put(&test_sem_1);
        test_start = xos_get_ccount();
        xos_mutex_unlock(&test_mtx_1);
    }

    ret = xos_thread_join(&thd2, &ret);
    if (ret) {
        printf("FAIL: error while waiting for test thread.\n");
        exit(-1);
    }

    cprint("Mtx_Unlock_CtxSw", "Mutex unlock with context switch",
           test_total/TEST_ITER, test_max);

    return 0;
}


//-----------------------------------------------------------------------------
// Mutex test.
//-----------------------------------------------------------------------------
int32_t
mutex_perf_test()
{   
    int32_t  ret;

    ret = xos_thread_create(&thd1, 0, mutex_test, 0, "mutex_test",
                            stack1, STACK_SIZE, 5, 0, 0);
    if (ret) {
        printf("FAIL: Error creating test threads\n");
        exit(-1);
    }

    ret = xos_thread_join(&thd1, &ret);
    if (ret) {
        printf("FAIL: Error joining thread mutex_test\n");
        exit(-1);
    }

    //xos_list_threads();
    xos_thread_delete(&thd1);
    xos_thread_delete(&thd2);
    return 0;
}


//-----------------------------------------------------------------------------
// Helper thread for event tests.
//-----------------------------------------------------------------------------
int32_t
event_get(void * arg, int32_t unused)
{
    uint32_t i; 
    
    for(i = 0; i < TEST_ITER; i++) {
        xos_event_wait_any(&test_event_1, 0xFFFF);
        xos_event_clear(&test_event_1, XOS_EVENT_BITS_ALL);
    }
    
    return 0;
}


//-----------------------------------------------------------------------------
// Helper thread 2 for event tests.
//-----------------------------------------------------------------------------
int32_t
event_get2(void * arg, int32_t unused)
{
    uint32_t delta;
    uint32_t i;

    for(i = 0; i < TEST_ITER; i++) {
        // First get the semaphore to sync with lower priority thread
        xos_sem_get(&test_sem_1);
        // Now block on the event
        xos_event_wait_any(&test_event_1, 0xFFFF);
        delta = xos_get_ccount() - test_start;
        test_total += delta;
        test_max = test_max >= delta ? test_max : delta;
        xos_event_clear(&test_event_1, XOS_EVENT_BITS_ALL);
    }

    return 0;
}


//-----------------------------------------------------------------------------
// Event operations timing test.
//-----------------------------------------------------------------------------
int32_t
event_test(void * arg, int32_t unused)
{   
    uint32_t start;
    uint32_t delta;
    uint32_t total;
    uint32_t max;
    uint32_t i;
    int32_t  ret;

    if (!csv) {
        printf("\nEvent timing test"
               "\n-----------------\n");
    }

    xos_event_create(&test_event_1, 0xFFFF, 0);
    xos_sem_create(&test_sem_1, XOS_SEM_WAIT_PRIORITY, 0);

    // First, just measure how long it takes to set an event with no contention
    // and no waits/wakeups. Also measure how long it takes to clear.
    
    test_total = 0;
    test_max = 0;
    total = 0; 
    max = 0;

    for(i = 0; i < TEST_ITER; i++) {
        start = xos_get_ccount();
        xos_event_set(&test_event_1, 0xFFFF);
        delta = xos_get_ccount() - start;
        test_total += delta;
        test_max = test_max >= delta ? test_max : delta;

        start = xos_get_ccount();
        xos_event_clear(&test_event_1, XOS_EVENT_BITS_ALL);
        delta = xos_get_ccount() - start;
        total += delta;
        max = max >= delta ? max : delta;
    }
    
    cprint("Event_Set_NoWake", "Event set with no wake",
           test_total/TEST_ITER, test_max);
    cprint("Event_Clr_NoWake", "Event clear with no wake",
           total/TEST_ITER, max);

    // Now measure the time taken to set an event when a lower priority
    // thread has to be unblocked.
    
    ret = xos_thread_create(&thd2, 0, event_get, 0, "event_get",
                            stack2, STACK_SIZE, 4, 0, 0);
    if (ret) {
        printf("FAIL: error creating test threads\n");
        exit(-1);
    }

    test_total = 0;
    test_max = 0;

    for(i = 0; i < TEST_ITER; i++) {
        xos_event_clear(&test_event_1, XOS_EVENT_BITS_ALL);
        // Let the other thread run so that it can block on the event
        xos_thread_sleep(10000);
        start = xos_get_ccount();
        xos_event_set(&test_event_1, 0xFFFF);
        delta = xos_get_ccount() - start;
        test_total += delta;
        test_max = test_max >= delta ? test_max : delta;
    }

    ret = xos_thread_join(&thd2, &ret);
    if (ret) {
        printf("FAIL: error while waiting for test thread.\n");
        exit(-1);
    }

    cprint("Event_Set_Wake", "Event set with thread wake",
           test_total/TEST_ITER, test_max);

    // Now measure the time taken to set an event + context switch when
    // a higher priority thread is unblocked.

    ret = xos_thread_delete(&thd2);
    ret = xos_thread_create(&thd2, 0, event_get2, 0, "event_get2",
                            stack2, STACK_SIZE, 6, 0, 0);
    if (ret) {
        printf("FAIL: error creating test threads\n");
        exit(-1);
    }

    test_total = 0;
    test_max = 0;

    for(i = 0; i < TEST_ITER; i++) {
        xos_event_clear(&test_event_1, XOS_EVENT_BITS_ALL);
        // Now signal the other thread with the semaphore. This will cause an
        // immediate switch to the other thread, which will then block on the
        // event and give back control to here.
        xos_sem_put(&test_sem_1);
        test_start = xos_get_ccount();
        xos_event_set(&test_event_1, 0xFFFF);
    }

    ret = xos_thread_join(&thd2, &ret);
    if (ret) {
        printf("FAIL: error while waiting for test thread.\n");
        exit(-1);
    }

    cprint("Event_Set_CtxSw", "Event set with context switch",
           test_total/TEST_ITER, test_max);

    return 0;
}


//-----------------------------------------------------------------------------
// Event test.
//-----------------------------------------------------------------------------
int32_t
event_perf_test()
{
    int32_t  ret;

    ret = xos_thread_create(&thd1, 0, event_test, 0, "event_test",
                            stack1, STACK_SIZE, 5, 0, 0);
    if (ret) {
        printf("FAIL: Error creating test threads\n");
        exit(-1);
    }

    ret = xos_thread_join(&thd1, &ret);
    if (ret) {
        printf("FAIL: Error joining thread event_test\n");
        exit(-1);
    }

    //xos_list_threads();
    xos_thread_delete(&thd1);
    xos_thread_delete(&thd2);
    return 0;
}


//-----------------------------------------------------------------------------
// Helper thread for message queue tests.
//-----------------------------------------------------------------------------
int32_t
msg_get(void * arg, int32_t unused)
{
    uint32_t i;
    uint32_t msg[4];

    for(i = 0; i < TEST_ITER; i++) {
        xos_msgq_get(test_msgq, msg);
    }

    return 0;
}


//-----------------------------------------------------------------------------
// Helper thread 2 for message queue tests.
//-----------------------------------------------------------------------------
int32_t
msg_get2(void * arg, int32_t unused)
{
    uint32_t delta;
    uint32_t i;
    uint32_t msg[4];

    for(i = 0; i < TEST_ITER; i++) {
        // First get the semaphore to sync with lower priority thread
        xos_sem_get(&test_sem_1);
        // Now block on the queue
        xos_msgq_get(test_msgq, msg);
        delta = xos_get_ccount() - test_start;
        test_total += delta;
        test_max = test_max >= delta ? test_max : delta;
        xos_event_clear(&test_event_1, XOS_EVENT_BITS_ALL);
    }

    return 0;
}   


//-----------------------------------------------------------------------------
// Message queue test.
//-----------------------------------------------------------------------------
int32_t
msgq_test()
{
    int32_t  i;
    int32_t  ret;
    uint32_t msg[4];
    uint32_t sum = 0;
    uint32_t max = 0;
    uint32_t start;
    uint32_t delta;
    uint32_t get_avg, get_max, put_avg, put_max;

    if (!csv) {
        printf("\nMessage Queue timing test"
               "\n-------------------------\n");
    }

    // First, measure the time taken to put and get messages on the queue
    // when no thread wakeup/context switch is involved.

    xos_sem_create(&test_sem_1, XOS_SEM_WAIT_PRIORITY, 0);
    xos_msgq_create(test_msgq, 16, 16, 0);

    for (i = 0; i < 16; i++) {
        start = xos_get_ccount();
        xos_msgq_put(test_msgq, msg);
        delta = xos_get_ccount() - start;
        sum += delta;
        max = max >= delta ? max : delta;
    }

    put_avg = sum/16;
    put_max = max;

    sum = 0;
    max = 0;

    for (i = 0; i < 16; i++) {
        uint32_t start = xos_get_ccount();

        xos_msgq_get(test_msgq, msg);
        delta = xos_get_ccount() - start;
        sum += delta;
        max = max >= delta ? max : delta;
    }

    get_avg = sum/16;
    get_max = max;

    cprint("Msg_Put_NoWake", "Message put with no wake", put_avg, put_max);
    cprint("Msg_Get_NoCont", "Message get with no contention", get_avg, get_max);

    // Now measure the time taken to send a message when a lower priority
    // thread has to be unblocked.

    ret = xos_thread_create(&thd2, 0, msg_get, 0, "msg_get",
                            stack2, STACK_SIZE, 4, 0, 0);
    if (ret) {
        printf("FAIL: error creating test threads\n");
        exit(-1);
    }

    test_total = 0;
    test_max = 0;

    for(i = 0; i < TEST_ITER; i++) {
        // Let the other thread run so that it can block on the event
        xos_thread_sleep(10000);
        start = xos_get_ccount();
        xos_msgq_put(test_msgq, msg);
        delta = xos_get_ccount() - start;
        test_total += delta;
        test_max = test_max >= delta ? test_max : delta;
    }
    
    ret = xos_thread_join(&thd2, &ret);
    if (ret) {
        printf("FAIL: error while waiting for test thread.\n");
        exit(-1);
    }
    
    cprint("Msg_Put_Wake", "Message put with thread wake",
           test_total/TEST_ITER, test_max);

    // Now measure the time taken to set an event + context switch when
    // a higher priority thread is unblocked.

    ret = xos_thread_delete(&thd2);
    ret = xos_thread_create(&thd2, 0, msg_get2, 0, "msg_get2",
                            stack2, STACK_SIZE, 6, 0, 0);
    if (ret) {
        printf("FAIL: error creating test threads\n");
        exit(-1);
    }

    test_total = 0;
    test_max = 0;

    for(i = 0; i < TEST_ITER; i++) {
        // Now signal the other thread with the semaphore. This will cause an
        // immediate switch to the other thread, which will then block on the
        // msg queue and give back control to here.
        xos_sem_put(&test_sem_1);
        test_start = xos_get_ccount();
        xos_msgq_put(test_msgq, msg);
    }

    ret = xos_thread_join(&thd2, &ret);
    if (ret) {
        printf("FAIL: error while waiting for test thread.\n");
        exit(-1);
    }

    cprint("Msg_Put_CtxSw", "Message put with context switch",
           test_total/TEST_ITER, test_max);

    return 0;
}


//-----------------------------------------------------------------------------
// Message test.
//-----------------------------------------------------------------------------
int32_t
msgq_perf_test()
{
    int32_t ret;

    ret = xos_thread_create(&thd1, 0, msgq_test, 0, "msgq_test",
                            stack1, STACK_SIZE, 5, 0, 0);
    if (ret) {
        printf("FAIL: Error creating test threads\n");
        exit(-1);
    }

    ret = xos_thread_join(&thd1, &ret);
    if (ret) {
        printf("FAIL: Error joining thread msgq_test\n");
        exit(-1);
    }

    xos_thread_delete(&thd1);
    xos_thread_delete(&thd2);
    return 0;
}


//-----------------------------------------------------------------------------
// Helper thread for mem block tests.
//-----------------------------------------------------------------------------
int32_t
mblock_get(void * arg, int32_t unused)
{
    uint32_t i;
    void *   p;

    for(i = 0; i < TEST_ITER; i++) {
        p = xos_block_alloc(&pool);
        xos_block_free(&pool, p);
    }

    return 0;
}


//-----------------------------------------------------------------------------
// Helper thread 2 for mem block tests.
//-----------------------------------------------------------------------------
int32_t
mblock_get2(void * arg, int32_t unused)
{
    uint32_t delta;
    uint32_t i;
    void *   p;

    for(i = 0; i < TEST_ITER; i++) {
        // First get the semaphore to sync with lower priority thread
        xos_sem_get(&test_sem_1);
        // Now block on the mem pool
        p = xos_block_alloc(&pool);
        delta = xos_get_ccount() - test_start;
        test_total += delta;
        test_max = test_max >= delta ? test_max : delta;
        xos_block_free(&pool, p);
    }

    return 0;
}


//-----------------------------------------------------------------------------
// Block mem operations timing test.
//-----------------------------------------------------------------------------
int32_t
blockmem_test(void * arg, int32_t unused)
{
    uint32_t start;
    uint32_t delta;
    uint32_t total;
    uint32_t max;
    uint32_t i;
    int32_t  ret;
    void *       p;

    if (!csv) {
        printf("\nBlockmem timing test"
               "\n--------------------\n");
    }

    xos_sem_create(&test_sem_1, XOS_SEM_WAIT_PRIORITY, 0);
    xos_block_pool_init(&pool, pmem, 32, 16, 0);

    // First, just measure how long it takes to allocate and free a block
    // with no waits/wakeups involved.

    test_total = 0;
    test_max = 0;
    total = 0;
    max = 0;

    for(i = 0; i < TEST_ITER; i++) {
        start = xos_get_ccount();
        p = xos_block_alloc(&pool);
        delta = xos_get_ccount() - start;
        test_total += delta;
        test_max = test_max >= delta ? test_max : delta;

        start = xos_get_ccount();
        xos_block_free(&pool, p);
        delta = xos_get_ccount() - start;
        total += delta;
        max = max >= delta ? max : delta;
    }

    cprint("Blk_Alloc_NoCont", "Block alloc with no contention",
           test_total/TEST_ITER, test_max);
    cprint("Blk_Free_NoWake", "Block free with no wake",
           total/TEST_ITER, max);

    for (i = 0; i < 15; i++) {
        p = xos_block_alloc(&pool);
    }

    // Now measure the time taken to free a block when a lower priority
    // thread has to be unblocked.

    ret = xos_thread_create(&thd2, 0, mblock_get, &pool, "mblock_get",
                            stack2, STACK_SIZE, 4, 0, 0);
    if (ret) {
        printf("FAIL: error creating test threads\n");
        exit(-1);
    }

    test_total = 0;
    test_max   = 0;

    for(i = 0; i < TEST_ITER; i++) {
        p = xos_block_alloc(&pool);
        // Let the other thread run so that it can block on the pool
        xos_thread_sleep(2000);
        start = xos_get_ccount();
        xos_block_free(&pool, p);
        delta = xos_get_ccount() - start;
        test_total += delta;
        test_max = test_max >= delta ? test_max : delta;
    }

    ret = xos_thread_join(&thd2, &ret);
    if (ret) {
        printf("FAIL: error while waiting for test thread.\n");
        exit(-1);
    }

    cprint("Blk_Free_Wake", "Block free with thread wake",
           test_total/TEST_ITER, test_max);

    // Now measure the time taken to free a block + context switch when
    // a higher priority thread is unblocked.

    ret = xos_thread_delete(&thd2);
    ret = xos_thread_create(&thd2, 0, mblock_get2, 0, "mblock_get2",
                            stack2, STACK_SIZE, 6, 0, 0);
    if (ret) {
        printf("FAIL: error creating test threads\n");
        exit(-1);
    }

    test_total = 0;
    test_max   = 0;

    for(i = 0; i < TEST_ITER; i++) {
        xos_block_alloc(&pool);
        // Now signal the other thread with the semaphore. This will cause an
        // immediate switch to the other thread, which will then block on the
        // pool and give back control to here.
        xos_sem_put(&test_sem_1);
        test_start = xos_get_ccount();
        xos_block_free(&pool, p);
    }

    ret = xos_thread_join(&thd2, &ret);
    if (ret) {
        printf("FAIL: error while waiting for test thread.\n");
        exit(-1);
    }

    cprint("Blk_Free_CtxSw", "Block free with context switch",
           test_total/TEST_ITER, test_max);

    return 0;
}


//-----------------------------------------------------------------------------
// Block mem perf test.
//-----------------------------------------------------------------------------
int32_t
blockmem_perf_test()
{   
    int32_t  ret;

    ret = xos_thread_create(&thd1, 0, blockmem_test, 0, "blockmem_test",
                            stack1, STACK_SIZE, 5, 0, 0);
    if (ret) {
        printf("FAIL: Error creating test threads\n");
        exit(-1);
    }
    
    ret = xos_thread_join(&thd1, &ret);
    if (ret) {
        printf("FAIL: Error joining thread blockmem_test\n");
        exit(-1);
    }

    xos_thread_delete(&thd1);
    xos_thread_delete(&thd2);
    return 0;
}


//-----------------------------------------------------------------------------
// Print object size info.
//-----------------------------------------------------------------------------
void
print_sizes()
{
    printf("\nObject Sizes:"
           "\n-------------\n");
#if XOS_OPT_THREAD_SAFE_CLIB
    printf("Thread Control Block   : %5u bytes\n", sizeof(XosThread) - sizeof(struct _reent));
    printf("TCB with C lib reent   : %5u bytes\n", sizeof(XosThread));
    printf("    (C lib reent)      : %5u bytes\n", sizeof(struct _reent));
#else
    printf("Thread Control Block   : %5u bytes\n", sizeof(XosThread));
#endif

    printf("XOS_STACK_EXTRA        : %5u bytes\n", XOS_STACK_EXTRA);
    printf("XOS_STACK_EXTRA_NO_CP  : %5u bytes\n", XOS_STACK_EXTRA_NO_CP);
    printf("    XOS_FRAME_SIZE     : %5u bytes\n", XOS_FRAME_SIZE);
    printf("    XT_CP_SIZE         : %5u bytes\n", XT_CP_SIZE);
    printf("    XT_NCP_SIZE        : %5u bytes\n", XT_NCP_SIZE);
    printf("Min stack size         : %5u bytes\n", XOS_STACK_MIN_SIZE);
    printf("Min stack size (no CP) : %5u bytes\n", XOS_STACK_MIN_SIZE_NO_CP);

    printf("Semaphore              : %5u bytes\n", sizeof(XosSem));
    printf("Mutex                  : %5u bytes\n", sizeof(XosMutex));
    printf("Event                  : %5u bytes\n", sizeof(XosEvent));
    printf("Condition              : %5u bytes\n", sizeof(XosCond));
    printf("Timer                  : %5u bytes\n", sizeof(XosTimer));
    printf("Message Queue (16/16)  : %5u bytes\n", XOS_MSGQ_SIZE(16,16));
    printf("Memory Block Pool      : %5u bytes\n", sizeof(XosBlockPool));
}


#if XCHAL_HAVE_INTERRUPTS
//-----------------------------------------------------------------------------
// Interrupt latency test.
//-----------------------------------------------------------------------------
volatile uint32_t cnt1;
volatile uint32_t cnt2;
volatile uint32_t cnt3;
volatile uint32_t entry_total;
volatile uint32_t entry_max;
volatile uint32_t exit_total;
volatile uint32_t exit_max;
volatile uint32_t oh_total;
volatile uint32_t oh_max;


//-----------------------------------------------------------------------------
// Interrupt handler for latency test. Arg is timer number.
//-----------------------------------------------------------------------------
void
inthandler1(void * arg)
{
    uint32_t diff = xos_get_ccount() - cnt1;

    switch((int)arg) {
        case 0: xos_set_ccompare0(0); break;
#if XCHAL_NUM_TIMERS >= 2
        case 1: xos_set_ccompare1(0); break;
#endif
#if XCHAL_NUM_TIMERS >= 3
        case 2: xos_set_ccompare2(0); break;
#endif
    }

    entry_total += diff;
    entry_max = diff > entry_max ? diff : entry_max;
    cnt2 = xos_get_ccount();
}


//-----------------------------------------------------------------------------
// Interrupt handler for latency test. Arg is timer number.
//-----------------------------------------------------------------------------
void
inthandler2(void * arg)
{   
    uint32_t diff = xos_get_ccount() - cnt1;

    switch((int)arg) {
        case 0: xos_set_ccompare0(0); break;
#if XCHAL_NUM_TIMERS >= 2
        case 1: xos_set_ccompare1(0); break;
#endif
#if XCHAL_NUM_TIMERS >= 3
        case 2: xos_set_ccompare2(0); break;
#endif
    }

    entry_total += diff;
    entry_max = diff > entry_max ? diff : entry_max;

    xos_thread_resume(&perf_test_tcb);

    cnt2 = xos_get_ccount();
}


//-----------------------------------------------------------------------------
// Interrupt handler for round-robin context switch test. Arg is timer number.
//-----------------------------------------------------------------------------
void
inthandler3(void * arg)
{
    static uint32_t count = 0;

    uint32_t ccnt = xos_get_ccount();
    uint32_t diff = ccnt - cnt1;
    uint32_t next = ccnt + 5000;

    count++;
    if (count == TEST_ITER2) {
        // Terminate test.
        xos_sem_put(&test_sem_1);
        next = 0;
        cnt3 = 1;
    }

    switch((int)arg) {
        case 0: xos_set_ccompare0(next); break;
#if XCHAL_NUM_TIMERS >= 2
        case 1: xos_set_ccompare1(next); break;
#endif
#if XCHAL_NUM_TIMERS >= 3
        case 2: xos_set_ccompare2(next); break;
#endif
    }

    // Trigger round-robin switch. Actual switch happens during return
    // from interrupt.
    if (next) {
        xos_thread_yield();
    }

    entry_total += diff;
    entry_max = diff > entry_max ? diff : entry_max;

    cnt1 = next;

    diff = xos_get_ccount() - ccnt;
    oh_total += diff;
    oh_max = diff > oh_max ? diff : oh_max;

    cnt2 = xos_get_ccount();
}


//-----------------------------------------------------------------------------
// Helper thread for round-robin context switch test.
//-----------------------------------------------------------------------------
int32_t
rr_func(void * arg, int32_t unused)
{
    int32_t d = (int32_t) arg;

    if (d) {
        return rr_func((void *)(d - 1), 0);
    }

    cnt2 = (uint32_t) -1;

    while (!cnt3) {
        uint32_t cnt = xos_get_ccount();

        // If cnt2 != -1 then it has been set to the ccount at interrupt.
        if (cnt > cnt2) {
            uint32_t diff = cnt - cnt2;

            exit_total += diff;
            exit_max = diff > exit_max ? diff : exit_max;
            cnt2 = (uint32_t) -1;
        }
    }

    return 0;
}


//-----------------------------------------------------------------------------
// Measure interrupt latency using a timer interrupt, if available. Latency is
// measured from both thread state and idle state.
//-----------------------------------------------------------------------------
void
int_latency_test(void)
{
    int32_t  ret;
    int32_t  tnum;
    int32_t  intnum = -1;
    int32_t  i;
    uint32_t diff;

    // Find a usable timer, the same one that we'll use as the system timer.
    // System timer has not been set up yet, so this is safe.
    ret = xos_system_timer_select(-1, &tnum);
    if (ret != XOS_OK) {
        printf("No usable timer available, intr latency test skipped\n");
        return;
    }

    if (!csv) {
        printf("\nInterrupt latency test"
               "\n----------------------\n");
    }

    // Set up handler and enable interrupt.
    intnum = Xthal_timer_interrupt[tnum];
    xos_register_interrupt_handler(intnum, inthandler1, (void *)tnum);
    xos_interrupt_enable(intnum);

    // Initialize these values for the test, and also bring them into
    // the cache if relevant. Then we don't have to worry about cache
    // misses on reads/writes.
    entry_total = 0;
    entry_max   = 0;
    exit_total  = 0;
    exit_max    = 0;
    cnt1        = 0;
    cnt2        = 0;
    cnt3        = 0;

    // Test loop.
    for (i = 0; i < TEST_ITER2; i++) {
        uint32_t start = xos_get_ccount() + 2000;

        switch(tnum) {
            case 0: xos_set_ccompare0(start); break;
#if XCHAL_NUM_TIMERS >= 2
            case 1: xos_set_ccompare1(start); break;
#endif
#if XCHAL_NUM_TIMERS >= 3
            case 2: xos_set_ccompare2(start); break;
#endif
        }

        cnt1 = start;
        // Wait for interrupt. Note this is not idle mode so thread
        // state will be saved on interrupt.
        XT_WAITI(0);
        cnt3 = xos_get_ccount();
        diff = cnt3 - cnt2;
        exit_total += diff;
        exit_max = diff > exit_max ? diff : exit_max;
    }

    cprint("Thread_Entry_Lat", "Interrupt entry from thread",
           entry_total/TEST_ITER2, entry_max);
    cprint("Thread_Exit_Lat",  "Interrupt exit to thread",
           exit_total/TEST_ITER2, exit_max);

    // Clean up.
    xos_interrupt_disable(intnum);
    xos_unregister_interrupt_handler(intnum);

    // Now repeat the same thing for idle mode.
    xos_register_interrupt_handler(intnum, inthandler2, (void *)tnum);
    xos_interrupt_enable(intnum);

    // Initialize again.
    entry_total = 0;
    entry_max   = 0;
    exit_total  = 0;
    exit_max    = 0;
    cnt1        = 0;
    cnt2        = 0;
    cnt3        = 0;

    // Test loop.
    for (i = 0; i < TEST_ITER2; i++) {
        uint32_t start = xos_get_ccount() + 2000;

        switch(tnum) {
            case 0: xos_set_ccompare0(start); break;
#if XCHAL_NUM_TIMERS >= 2
            case 1: xos_set_ccompare1(start); break;
#endif
#if XCHAL_NUM_TIMERS >= 3
            case 2: xos_set_ccompare2(start); break;
#endif
        }

        cnt1 = start;
        // Go idle.
        xos_thread_suspend(XOS_THREAD_SELF);
        cnt3 = xos_get_ccount();
        diff = cnt3 - cnt2;
        exit_total += diff;
        exit_max = diff > exit_max ? diff : exit_max;
    }

    cprint("Idle_Entry_Lat", "Interrupt entry from idle",
           entry_total/TEST_ITER2, entry_max);
    cprint("Idle_Exit_Lat",  "Interrupt exit idle->thread",
           exit_total/TEST_ITER2, exit_max);

    // Clean up.
    xos_interrupt_disable(intnum);
    xos_unregister_interrupt_handler(intnum);

    // Now measure preemptive interrupt/context-switch times.

    xos_sem_create(&test_sem_1, XOS_SEM_WAIT_PRIORITY, 0);

    // Launch test threads at lower priority.

    ret = xos_thread_create(&thd1, 0, rr_func, (void *)(XCHAL_NUM_AREGS/8),
                            "thd1", stack1, STACK_SIZE, 5, 0, 0);
    if (ret) {
        xos_fatal_error(-1, "Error creating test threads\n");
    }
 
    ret = xos_thread_create(&thd2, 0, rr_func, (void *)(XCHAL_NUM_AREGS/8),
                            "thd2", stack2, STACK_SIZE, 5, 0, 0);
    if (ret) {
        xos_fatal_error(-1, "Error creating test threads\n");
    }

    xos_register_interrupt_handler(intnum, inthandler3, (void *)tnum);
    xos_interrupt_enable(intnum);

    // Initialize again.
    entry_total = 0;
    entry_max   = 0;
    exit_total  = 0;
    exit_max    = 0;
    oh_total    = 0;
    oh_max      = 0;
    cnt1        = 0;
    cnt2        = 0;
    cnt3        = 0;

    // Fire up the timer
    diff = xos_get_ccount() + 5000;
    switch(tnum) {
        case 0: xos_set_ccompare0(diff); break;
#if XCHAL_NUM_TIMERS >= 2
        case 1: xos_set_ccompare1(diff); break;
#endif
#if XCHAL_NUM_TIMERS >= 3
        case 2: xos_set_ccompare2(diff); break;
#endif
    }

    // Now block and wait for test to complete.
    cnt1 = diff;
    xos_sem_get(&test_sem_1);

    cprint("Pre_CtxSw",  "Preemptive context switch",
           (entry_total/TEST_ITER2 + exit_total/TEST_ITER2), entry_max + exit_max);
    cprint("PctxSw_OH",  "(interrupt handler overhead)",
           oh_total/TEST_ITER2, oh_max);

    // Wait for threads to finish
    ret = xos_thread_join(&thd1, &ret);
    if (ret) {
        xos_fatal_error(-1, "Error joining thread thd1\n");
    }
    ret = xos_thread_join(&thd2, &ret);
    if (ret) {
        xos_fatal_error(-1, "Error joining thread thd2\n");
    }

    // Clean up
    xos_thread_delete(&thd1);
    xos_thread_delete(&thd2);
    xos_interrupt_disable(intnum);
    xos_unregister_interrupt_handler(intnum);
    xos_sem_delete(&test_sem_1);
}
#endif


//-----------------------------------------------------------------------------
// Measure how long it takes to create a thread.
//-----------------------------------------------------------------------------
void
thread_create_test()
{
    int32_t   ret;
    uint32_t  cc2;
    uint32_t  cc1 = xos_get_ccount();

    ret = xos_thread_create(&thd1,
                            0,
                            yield_func,
                            0,
                            "",
                            stack1,
                            STACK_SIZE,
                            1,
                            0,
                            0);
    cc2 = xos_get_ccount();

    if (ret == XOS_OK) {
        xos_thread_delete(&thd1);
        printf("Thread Create      : %u cycles\n", cc2 - cc1);
    }
    else {
        printf("Error (%d) in xos_thread_create()\n");
    }
}


//-----------------------------------------------------------------------------
// Test driver (function or thread depending upon options).
//-----------------------------------------------------------------------------
int32_t
perf_test(void * arg, int32_t unused)
{
    if (csv) {
        printf("Testname,Cycles,CodeSize,Description\n");
    }
    else {
        printf("XOS Performance Test\n"
               "XOS Version : %s\n"
               "Core Config : %s\n",
               XOS_VERSION_STRING,
               XCHAL_CORE_ID);
    }

    // Various data structure sizes for this config.
    if (!csv)
        print_sizes();

#if XCHAL_HAVE_INTERRUPTS
    // Run this before starting timer.
    int_latency_test();
#endif

#if XCHAL_NUM_TIMERS > 0
    if (!csv)
        printf("\nStarting system timer\n");
#if defined USE_DYNAMIC_TICKS
    if (!csv)
        printf("(using dynamic tick period)\n\n");
    xos_start_system_timer(-1, 0);
#else
    if (!csv)
        printf("(using fixed-duration tick period)\n\n");
    xos_start_system_timer(-1, TICK_CYCLES);
#endif

    yield_test();
    sem_perf_test();
    mutex_perf_test();
    event_perf_test();
    msgq_perf_test();
    blockmem_perf_test();
#else
    printf("No timer, skipping perf tests\n");
#endif

    if (!csv) {
        printf("\nMisc Tests\n----------\n");
        thread_create_test();
    }

    exit(0);
    return 0;
}


int
main(int argc, char *argv[])
{
    int32_t  ret;
    uint32_t appcfg;
    uint32_t libcfg;

    if (argc > 1) {
        if (strcmp(argv[1], "-csv") == 0)
            csv = 1;
    }

    // Check build config

    if (xos_check_build_config(&appcfg, &libcfg) == true) {
        printf("Build config check OK\n");
    }
    else {
        printf("Error: build config mismatch lib 0x%08x, app 0x%08x\n", libcfg, appcfg);
        return -1;
    }

#if defined BOARD
    xos_set_clock_freq(xtbsp_clock_freq_hz());
#else
    xos_set_clock_freq(XOS_CLOCK_FREQ);
#endif

    ret = xos_thread_create(&perf_test_tcb,
                            0,
                            perf_test,
                            0,
                            "perf_test",
                            perf_test_stk,
                            STACK_SIZE,
                            7,
                            0,
                            0);

    xos_start(0);
    return -1; // should never get here
}

