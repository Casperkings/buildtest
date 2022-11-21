
// log_test.c - XOS log test

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


#include <stdio.h>
#include <stdlib.h>
#include <xtensa/config/system.h>

#include "test.h"


#define NUM_THREADS	20
#define NUM_WRITES      100


//-----------------------------------------------------------------------------
// Runs multiple threads, makes log entries from every thread.
// Checks log for consistency. Also measures the time taken by a log write.
//-----------------------------------------------------------------------------

#if !USE_MAIN_THREAD
XosThread slog_test_tcb;
uint8_t   slog_test_stk[STACK_SIZE];
#endif

XosThread test_threads[NUM_THREADS];
uint8_t   stacks[NUM_THREADS][STACK_SIZE];
char      names[NUM_THREADS][32];

uint32_t  total_cycles;
uint32_t  max_time = 0;
uint32_t  min_time = 1000000;
uint32_t  avg_time;
uint32_t  num_writes;


//-----------------------------------------------------------------------------
// Write to log and measure time taken.
//-----------------------------------------------------------------------------
void
write_log(uint32_t v1, uint32_t v2, uint32_t v3)
{
    uint32_t ps;
    uint32_t diff;
    uint32_t now = xos_get_ccount();

    xos_log_write(v1, v2, v3);
    diff = xos_get_ccount() - now;

    // Ignore large diffs due to possible interrupt or context switch.
    if (diff > 1000) {
        //printf("---- %u\n", diff);
        return;
    }

    ps = xos_disable_interrupts();
    if (diff > max_time)
        max_time = diff;
    if (diff < min_time)
        min_time = diff;
    total_cycles += diff;
    num_writes++;
    xos_restore_interrupts(ps);
}


//-----------------------------------------------------------------------------
// Thread function.
//-----------------------------------------------------------------------------
int32_t
test_func(void * arg, int32_t unused)
{
    int32_t  val = (int32_t) arg + 0x1000;
    uint32_t cnt = 0;

    printf("+++ Thread %s starting\n", xos_thread_get_name(xos_thread_id()));

    srand(val);

    while (cnt < NUM_WRITES) {
        write_log(val, cnt, cnt + 1);
        xos_thread_sleep((rand() % 10) * TICK_CYCLES);
        cnt++;
    }

    return cnt;
}


//-----------------------------------------------------------------------------
// Print one log entry.
//-----------------------------------------------------------------------------
void
print_log_entry(const XosLogEntry * entry)
{
    switch (entry->param1) {
    case XOS_SE_THREAD_CREATE:
        printf("%d: Thread_Create %p %d\n", entry->timestamp, entry->param2, entry->param3);
        break;
    case XOS_SE_THREAD_DELETE:
        printf("%d: Thread_Delete %p\n", entry->timestamp, entry->param2);
        break;
    case XOS_SE_THREAD_SWITCH:
        printf("%d: Thread_Switch %p -> %p\n", entry->timestamp, entry->param2, entry->param3);
        break;
    case XOS_SE_THREAD_YIELD:
        printf("%d: Thread_Yield %p\n", entry->timestamp, entry->param2);
        break;
    case XOS_SE_THREAD_BLOCK:
        printf("%d: Thread_Block %p (%s)\n", entry->timestamp, entry->param2, entry->param3);
        break;
    case XOS_SE_THREAD_WAKE:
        printf("%d: Thread_Wake %p %d\n", entry->timestamp, entry->param2, entry->param3);
        break;
    case XOS_SE_THREAD_PRI_CHANGE:
        printf("%d: Thread_Pri_Change: %p %d\n", entry->timestamp, entry->param2, entry->param3);
        break;
    case XOS_SE_THREAD_SUSPEND:
        printf("%d: Thread_Suspend: %p\n", entry->timestamp, entry->param2);
        break;
    case XOS_SE_THREAD_ABORT:
        printf("%d: Thread_Abort: %p %d\n", entry->timestamp, entry->param2, entry->param3);
        break;
    case XOS_SE_TIMER_INTR_START:
        printf("%d: Timer_Handler_Start\n", entry->timestamp);
        break;
    case XOS_SE_TIMER_INTR_END:
        printf("%d: Timer_Handler_End\n", entry->timestamp);
        break;
    case XOS_SE_MUTEX_LOCK:
        printf("%d: Mutex_Lock %p %d\n", entry->timestamp, entry->param2, entry->param3);
        break;
    case XOS_SE_MUTEX_UNLOCK:
        printf("%d: Mutex_Unlock %p %d\n", entry->timestamp, entry->param2, entry->param3);
        break;
    case XOS_SE_SEM_GET:
        printf("%d: Sem_Get %p %d\n", entry->timestamp, entry->param2, entry->param3);
        break;
    case XOS_SE_SEM_PUT:
        printf("%d: Sem_Put %p %d\n", entry->timestamp, entry->param2, entry->param3);
        break;
    case XOS_SE_EVENT_SET:
        printf("%d: Event_Set %p 0x%08x\n", entry->timestamp, entry->param2, entry->param3);
        break;
    case XOS_SE_EVENT_CLEAR:
        printf("%d: Event_Clear %p 0x%08x\n", entry->timestamp, entry->param2, entry->param3);
        break;
    case XOS_SE_EVENT_CLEAR_SET:
        printf("%d: Event_Clear_Set %p 0x%08x\n", entry->timestamp, entry->param2, entry->param3);
        break;
    default:
        printf("%d: Thread %p %d %d\n", entry->timestamp, entry->param1, entry->param2, entry->param3);
        break;
    }
}


//-----------------------------------------------------------------------------
// Test function.
//-----------------------------------------------------------------------------
int32_t
log_test(void * arg, int32_t unused)
{
    int32_t n;
    int32_t ret;
    int32_t pri = 0;
    char *  p = 0;
    const XosLogEntry * entry;
    uint64_t last_ts;

    // Allocate memory for log and init the log.
    void * logmem = malloc(XOS_LOG_SIZE(100));

    xos_log_init(logmem, 100, true);

    printf("Log test starting...\n");

    // Create test threads.
    for (n = 0; n < NUM_THREADS; ++n) {
        p = names[n];
        sprintf(p, "Thread_%d", n);
        ret = xos_thread_create(&(test_threads[n]), 0, test_func, (void *)n, p,
                                stacks[n], STACK_SIZE, (pri++ % 5), 0, 0);
        if (ret) {
            xos_fatal_error(-1, "FAIL: error creating test threads\n");
        }
    }

    // Wait for all threads to finish.
    for (n = 0; n < NUM_THREADS; n++) {
        xos_thread_join(&(test_threads[n]), &ret);
    }

    // Disable the log before reading so that it doesn't keep changing.
    xos_log_disable();

    // Read through log.
    ret   = 0;
    entry = xos_log_get_first();

    if (entry != XOS_NULL) {
        last_ts = entry->timestamp;
    }
    while (entry != XOS_NULL) {
        print_log_entry(entry);
        entry = xos_log_get_next(entry);
        if (entry) {
            if (entry->timestamp <= last_ts) {
                ret--;
                printf("FAIL: log entry timestamps going backwards\n");
            }
            last_ts = entry->timestamp;
        }
    }

    avg_time = total_cycles / num_writes;
    printf("Cycles per log write : avg %u min %u max %u\n", avg_time, min_time, max_time);

    if (ret == 0) 
        printf("PASS\n");

#if !USE_MAIN_THREAD
    exit(ret);
#endif
    return ret;
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

#if USE_MAIN_THREAD
    xos_start_main_ex("main", 7, STACK_SIZE);
    xos_start_system_timer(-1, 0);

    ret = log_test(0, 0);
    return ret;
#else
    ret = xos_thread_create(&slog_test_tcb,
                            0,
                            log_test,
                            0,
                            "log_test",
                            slog_test_stk,
                            STACK_SIZE,
                            7,
                            0,
                            0);

    xos_start_system_timer(-1, 0);
    xos_start(0);
    return -1; // should never get here
#endif
}

