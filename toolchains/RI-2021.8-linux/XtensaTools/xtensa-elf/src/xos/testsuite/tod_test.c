
// tod_test.c - Simple test to exercise the time-of-day API.

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

// Simple test to exercise the time-of-day support. Sleep for "1 second"
// and then get time and print. When running in simulation the actual
// interval may not be anywhere close to 1 second.

#include <stdio.h>
#include <time.h>
#include <sys/time.h>

#include "test.h"


extern int
settimeofday(const struct timeval *tv , const struct timezone *tz);


#if !USE_MAIN_THREAD
XosThread tod_test_tcb;
uint8_t   tod_test_stk[STACK_SIZE];
#endif

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int32_t
tod_test(void * arg, int32_t unused)
{
    struct timeval  tv;
    struct timezone tz;

    time_t   tt;
    uint32_t count = 500;

    tv.tv_sec  = 0;
    tv.tv_usec = 0;

    tz.tz_minuteswest = 480;
    tz.tz_dsttime     = 0;

    settimeofday(&tv, &tz);

    while (count--) {
        gettimeofday(&tv, &tz);
        tt = time(0);

        printf("tv -> %ld sec %ld usec, time() -> %ld\n",
               tv.tv_sec, tv.tv_usec, tt);
        printf("ctime : %s\n", ctime(&tt));
        if (tt != tv.tv_sec)
            printf("WARNING: time() and gettimeofday() do not match.\n");

        xos_thread_sleep(xos_secs_to_cycles(1));
    }

#if !USE_MAIN_THREAD
    exit(0);
#endif
    return 0;
}


//-----------------------------------------------------------------------------
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
    xos_start_system_timer(-1, xos_get_clock_freq()/100);

    ret = tod_test(0, 0);
    return ret;
#else
    ret = xos_thread_create(&tod_test_tcb,
                            0,
                            tod_test,
                            0,
                            "tod_test",
                            tod_test_stk,
                            STACK_SIZE,
                            7,
                            0,
                            0);

    xos_start_system_timer(-1, xos_get_clock_freq()/100);
    xos_start(0);
    return -1; // should never get here
#endif
}

