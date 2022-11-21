
// timer_test.c - XOS Timer API unit test.

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


#if XCHAL_NUM_TIMERS == 0
#error This test requires at least one timer to be present at < EXCM_LEVEL
#endif


#define CCOUNT_HACKVAL   0xFC000000
#define TEST_STK_SIZE    (STACK_SIZE + 1000)
#if !USE_MAIN_THREAD
XosThread timer_test_tcb;
uint8_t   timer_test_stk[TEST_STK_SIZE];
#endif

// Semaphore used for signaling between interrupt handler and thread.
XosSem    timer_sem;


//-----------------------------------------------------------------------------
// Timer callback for test_basic.
//-----------------------------------------------------------------------------
void
timer_cb1(void * arg)
{
    uint32_t * p = (uint32_t *) arg;

    *p = xos_get_ccount();
    xos_sem_put(&timer_sem);
}


//-----------------------------------------------------------------------------
// Set timers for random intervals and verify that they expire.
//-----------------------------------------------------------------------------
void
test_basic()
{
    static volatile uint32_t t2;
    uint32_t delta;
    uint32_t t1;
    int32_t  i;
    XosTimer timer1;

    xos_timer_init(&timer1);

    xos_sem_create(&timer_sem, 0, 0);

    for (i = 0; i < 10; i++) {
        delta = ((rand() % 50) + 1) * TICK_CYCLES * 10;
        t2 = 0;

        // Line up with a tick interval
        xos_thread_sleep(1000);

        t1 = xos_get_ccount();

        // Set a timer for the near future.
        xos_timer_start(&timer1, delta, 0, timer_cb1, (void *)&t2);

        // Wait for timer expiry
        xos_sem_get(&timer_sem);

        printf("diff is %u, delta is %u\n", (unsigned int)(t2-t1), (unsigned int)delta);
        if ((t2 - t1) - delta > (TICK_CYCLES + TICK_CYCLES/4))
            printf("Timer basic test FAIL\n");
        else
            printf("Timer basic test PASS\n");
    }
}


// Used by test_sequence().
static volatile uint32_t check;


//-----------------------------------------------------------------------------
// Timer callback used by test_sequence.
//-----------------------------------------------------------------------------
void
timer_cb2(void * arg)
{
    uint32_t count = (uint32_t) arg;

    if (check != (count + 1)) {
        // Something went wrong
        printf("Timer called out of sequence, check %u arg %u FAIL\n",
               (unsigned int)check, (unsigned int)count);
        exit(-1);
    }

    printf("cb2: %u\n", (unsigned int)count);
    check = count;
}


//-----------------------------------------------------------------------------
// Verify that timers fire in the right sequence. Set the timers from the
// longest to the shortest.
//-----------------------------------------------------------------------------
void
test_sequence()
{
    XosTimer stimers[20];
    uint64_t when = 20000000;

    check = 0;

    while (when) {
        xos_timer_init(&(stimers[check]));
        xos_timer_start(&(stimers[check]), when, 0, timer_cb2, (void *)check);
        check++;
        when -= 1000000;
    }

    while (check);

    printf("Timer sequence test PASS\n");
}


//-----------------------------------------------------------------------------
// Timer callback for test_rollover.
//-----------------------------------------------------------------------------
void
timer_cb_rollover(void * arg)
{
    uint32_t * p = (uint32_t *) arg;

    *p = 1;
}


//-----------------------------------------------------------------------------
// Test for CCOUNT rollover being handled. This is hacked - we force CCOUNT
// to a large number to avoid a *long* wait.
//-----------------------------------------------------------------------------
void
test_rollover_hack()
{
    static volatile uint32_t flag;
    XosTimer timer;
    uint64_t when;
    uint32_t cc1;
    uint32_t cc2;

    cc1 = xos_get_ccount();
    if (cc1 < CCOUNT_HACKVAL) {
        printf("CCOUNT already rolled over\n");
        return;
    }

    flag = 0;
    when = xos_get_system_cycles() + (0xFFFFFFFF - cc1) + 0x10000;

    xos_timer_init(&timer);
    xos_timer_start(&timer, when, XOS_TIMER_ABSOLUTE, timer_cb_rollover, (void *)&flag);
    while(!flag);

    cc2 = xos_get_ccount();
    if (cc2 >= cc1) {
        printf("Something went wrong\n");
        return;
    }

    printf("timer rollover test PASS\n");
}


#if XOS_OPT_TIMER_WAIT

//-----------------------------------------------------------------------------
// Verify that we can block on a timer and be woken on expiry.
//-----------------------------------------------------------------------------
void
test_timer_wait()
{
    XosTimer timer;
    int32_t ret;

    printf("Testing timer wait...\n");

    xos_timer_init(&timer);
    ret = xos_timer_start(&timer, 2*TICK_CYCLES, 0, 0, 0);
    if (ret == XOS_OK)
        ret = xos_timer_wait(&timer);

    printf("Timer wait test %s\n", ret == XOS_OK ? "PASS" : "FAIL");
}

#endif


//-----------------------------------------------------------------------------
// Test function.
//-----------------------------------------------------------------------------
int32_t
timer_test(void * arg, int32_t unused)
{
    test_basic();

    test_sequence();

#if XOS_OPT_TIMER_WAIT
    test_timer_wait();
#endif

    // Do this last.
    test_rollover_hack();

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

    // Push CCOUNT forward so rollover happens sooner.
    XT_WSR_CCOUNT(CCOUNT_HACKVAL);
    
    // Auto-select system timer, use dynamic ticks if specified.
    // This can be done before calling xos_start(). The interrupt
    // will not be enabled until xos_start() is called.
#if USE_DYNAMIC_TICKS
    xos_start_system_timer(-1, 0);
#else
    xos_start_system_timer(-1, TICK_CYCLES);
#endif

#if USE_MAIN_THREAD
    xos_start_main_ex("main", 7, TEST_STK_SIZE);
    ret = timer_test(0, 0);
    return ret;
#else
    ret = xos_thread_create(&timer_test_tcb,
                            0,
                            timer_test,
                            0,
                            "timer_test",
                            timer_test_stk,
                            TEST_STK_SIZE,
                            7,
                            0,
                            0);

    xos_start(0);
    return -1; // should never get here
#endif
}

