
// stats_test.c - XOS stats test

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


#define XTUTIL_NO_OVERRIDE 1

#include "test.h"


// Stats test. Collects per-thread stats and prints them. Also does a
// sanity check on cycle counting and prints the delta from expected.

#if !XOS_OPT_STATS
#error This test requires XOS_OPT_STATS to be enabled.
#endif

#define COUNT 500

#if !USE_MAIN_THREAD
#define STATS_STACK_SIZE    (STACK_SIZE + 1024)
XosThread stats_test_tcb;
uint8_t   stats_test_stk[STATS_STACK_SIZE];
#endif

XosThread get1;
XosThread get2;
XosThread get3;

uint8_t get1_stack[STACK_SIZE];
uint8_t get2_stack[STACK_SIZE];
uint8_t get3_stack[STACK_SIZE];

XosSem test_sem_1;


int32_t
get_func(void * arg, int32_t unused)
{
    uint32_t *   pval = (uint32_t *) arg; // Result storage
    uint32_t     cnt  = 0;
    const char * name = xos_thread_get_name(xos_thread_id());

    printf("+++ Thread %s starting\n", name);

    while (cnt < COUNT) {
        int32_t ret = xos_sem_get(&test_sem_1);

        if (ret != 0) {
            printf("Error: xos_sem_get() ret %d\n", (unsigned int)ret);
            xos_fatal_error(-1, "halt\n");
        }

        cnt++;
        //printf("Thread %s got sem cnt = %u\n", name, cnt);
        *pval = cnt;
    }

    return cnt;
}


int32_t
cycle_check()
{
    // Test is over, read system cycle count now to avoid adding any subsequent
    // cycles to the test data.
    extern uint64_t xos_system_cycles;
    volatile uint64_t xsc = xos_system_cycles;

    XosThreadStats stats[16];
    uint64_t cycle_count = 0;
    int32_t  count = 16;
    int32_t  i;
    int32_t  ret;
    uint32_t pct_val;

    xos_get_cpu_load(stats, &count, 0);

    printf("  Thread          CPU Load    Cycles\n");
    for (i = 0; i < count; i++) {
        printf(" %-16s   %2.2d %%    %8u\n",
               stats[i].thread == XOS_THD_STATS_INTR ? "intr" : xos_thread_get_name(stats[i].thread),
               stats[i].cpu_pct,
               (uint32_t)stats[i].cycle_count);
        cycle_count += stats[i].cycle_count;
    }

    if (cycle_count > xsc) {
        printf("ERROR: cycle count from stats exceeds cycle count from timer\n");
        return -1;
    }

    printf("Total cycles from stats counters = %u\n", (uint32_t) cycle_count);
    printf("Total cycles from timer counters = %u\n", (uint32_t) xsc);
    printf("(delta = %u)\n", (uint32_t) xsc - (uint32_t) cycle_count);

    // Don't want to use floating point math or print functions.
    pct_val = (uint32_t) ((cycle_count*10000)/xsc);
    printf("Accounted for %2.2d.%2.2d%%\n", pct_val/100, pct_val % 100);

    // Call get_stats with a couple of special cases.
    ret = xos_thread_get_stats(XOS_THD_STATS_IDLE, &stats[0]);
    if (ret != XOS_OK) {
        printf("Error calling xos_thread_get_stats() for idle thread\n");
        return -1;
    }
    ret = xos_thread_get_stats(XOS_THD_STATS_INTR, &stats[0]);
    if (ret != XOS_OK) {
        printf("Error calling xos_thread_get_stats() for intr thread\n");
        return -1;
    }

    // These must fail.
    ret = xos_thread_get_stats(NULL, NULL);
    if (ret != XOS_ERR_INVALID_PARAMETER) {
        printf("Error invalid params not checked\n");
        return -1;
    }
    ret = xos_get_cpu_load(NULL, NULL, 0);
    if (ret != XOS_ERR_INVALID_PARAMETER) {
        printf("Error invalid params not checked\n");
        return -1;
    }
    // Buffer too small.
    count = 1;
    ret = xos_get_cpu_load(stats, &count, 0);
    if (ret != XOS_ERR_INVALID_PARAMETER) {
        printf("Error buffer size check failed\n");
        return -1;
    }

    // Now clear stats on this next call, and verify.
    count = 16;
    xos_get_cpu_load(stats, &count, 1);
    ret = xos_thread_get_stats(XOS_THD_STATS_IDLE, &stats[0]);
    if ((ret != XOS_OK) || (stats[0].cycle_count != 0)) {
        printf("Error stats not cleared\n");
        return -1;
    }

    return 0;
}


int32_t
stats_test(void * arg, int32_t unused)
{
    XosThreadStats mystats = {0};

    uint32_t cnt1 = 0;
    uint32_t cnt2 = 0;
    uint32_t cnt3 = 0;
    uint32_t ps;
    int32_t  ret  = 0;
    int32_t  total;
    int32_t  i;

    printf("Stats test starting\n");

    // Check cycle counting for current thread. This should work
    // even when there are no context switches or interrupts.

    ps = xos_disable_interrupts();
    xos_thread_get_stats(xos_thread_id(), &mystats);
    cnt1 = mystats.cycle_count;
    printf("Testing thread cycle counter advance...\n");
    xos_thread_get_stats(xos_thread_id(), &mystats);
    cnt2 = mystats.cycle_count;
    xos_restore_interrupts(ps);

    if (cnt2 <= cnt1) {
        printf("Thread cycle counter not advancing.\n");
        exit(-1);
    }

    // Now run the multi-thread test.

    xos_sem_create(&test_sem_1, XOS_SEM_WAIT_PRIORITY, 0);

    ret |= xos_thread_create(&get1, 0, get_func, &cnt1, "get1",
                             get1_stack, STACK_SIZE, 1, 0, 0);
    ret |= xos_thread_create(&get2, 0, get_func, &cnt2, "get2",
                             get2_stack, STACK_SIZE, 1, 0, 0);
    ret |= xos_thread_create(&get3, 0, get_func, &cnt3, "get3",
                             get3_stack, STACK_SIZE, 1, 0, 0);
    if (ret) {
        printf("Error creating test threads\n");
        exit(-1);
    }

    for (i = 0; i < COUNT; i++) {
        xos_sem_put(&test_sem_1);
    }
    xos_thread_sleep(1000000);
    for (i = COUNT; i < (COUNT*3); i++) {
        xos_sem_put(&test_sem_1);
    }

    total = 0;
    ret   = 0;

    ret |= xos_thread_join(&get1, &total);
    if (total != cnt1) {
        printf("Test failed: cnt1 mismatch\n");
    }
    printf("Finished wait for thread 1\n");

    ret |= xos_thread_join(&get2, &total);
    if (total != cnt2) {
        printf("Test failed: cnt2 mismatch\n");
    }
    printf("Finished wait for thread 2\n");

    ret |= xos_thread_join(&get3, &total);
    if (total != cnt3) {
        printf("Test failed: cnt3 mismatch\n");
    }
    printf("Finished wait for thread 3\n");

    if (cnt1 + cnt2 + cnt3 != (COUNT*3)) {
        printf("Test failed: total count mismatch\n");
        exit(-1);
    }

    if (ret == 0) {
        ret = cycle_check();
    }

    if (ret != 0) {
        printf("FAIL\n");
    }

    exit(ret);
    return ret; // keep compiler happy
}


int32_t
main()
{
    int32_t ret;

#if defined BOARD
    xos_set_clock_freq(xtbsp_clock_freq_hz());
#else
    xos_set_clock_freq(XOS_CLOCK_FREQ);
#endif

#if USE_MAIN_THREAD
    xos_start_main_ex("main", 7, STATS_STACK_SIZE);
    xos_start_system_timer(-1, TICK_CYCLES);

    ret = stats_test(0, 0);
    return ret;
#else
    ret = xos_thread_create(&stats_test_tcb,
                            0,
                            stats_test,
                            0,
                            "stats_test",
                            stats_test_stk,
                            STATS_STACK_SIZE,
                            7,
                            0,
                            0);

    xos_start_system_timer(-1, TICK_CYCLES);
    xos_start(0);
    return -1; // should never get here
#endif
}

