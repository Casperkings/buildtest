// Copyright (c) 2020-2021 Cadence Design Systems, Inc.
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

// nonsecure_app.c -- Nonsecure application example code


#include "xtensa/secmon.h"

#include <xtensa/core-macros.h>
#include <xtensa/xtruntime.h>
#include <stdio.h>


#define TIMER_KICK_CYCLES   10000
#define TIMER_KICK_INCR     1000
#define TIMER_TEST_CYCLES   (TIMER_KICK_CYCLES * 10)

#define UNUSED(x) ((void)(x))


int timer_ints_handled[4]       = { 0, 0, 0, 0 };
int timer_ints_handled_late[4]  = { 0, 0, 0, 0 };
int timer_increments[4]         = { 
    TIMER_KICK_CYCLES,
    TIMER_KICK_CYCLES + TIMER_KICK_INCR,
    TIMER_KICK_CYCLES + TIMER_KICK_INCR * 2,
    TIMER_KICK_CYCLES + TIMER_KICK_INCR * 3
};


int timer_ints[4] = {
    XCHAL_TIMER0_INTERRUPT,
    XCHAL_TIMER1_INTERRUPT,
    XCHAL_TIMER2_INTERRUPT,
    XCHAL_TIMER3_INTERRUPT
};


// Nonsecure timer handler callback
static void timer_handler(void *arg)
{
    UNUSED(arg);
}


static int timer_test(void)
{
    int last_ccompare[4] = { 0, 0, 0, 0 };
    int start_ccount = xthal_get_ccount();
    int ccompare;
    int ret = 0;
    int i;

    ret = xtsm_init();
    if (ret != 0) {
        printf("Error: xtsm_init() FAILED (%d)\n", ret);
        return ret;
    }

    // Attempt some registration calls that should fail
    for (i = 0; i < XCHAL_NUM_TIMERS; i++) {
        ret = xtsm_set_interrupt_handler(timer_ints[i], timer_handler, NULL);
        if (ret == 0) {
            printf("Error: timer %d (int %d) nonsecure registration should have failed\n", i, timer_ints[i]);
            return -1;
        }
        ret = xtsm_set_interrupt_handler(timer_ints[i], NULL, NULL);
        if (ret == 0) {
            printf("Error: timer %d (int %d) nonsecure deregistration should have failed\n", i, timer_ints[i]);
            return -2;
        }
    }

    for (i = 0; i < XCHAL_NUM_TIMERS; i++) {
        last_ccompare[i] = xthal_get_ccompare(i);
    }

    // Collect interrupt handling data
    while (xthal_get_ccount() - start_ccount < TIMER_TEST_CYCLES) {
        for (i = 0; i < XCHAL_NUM_TIMERS; i++) {
            ccompare = xthal_get_ccompare(i);
            if (ccompare - last_ccompare[i] == timer_increments[i]) {
                timer_ints_handled[i]++;
            } else if (ccompare - last_ccompare[i] != 0) {
                timer_ints_handled_late[i]++;
            }
            last_ccompare[i] = ccompare;
        }
    }

    // Disable all interrupts and check test results
    __asm__ __volatile__ ("wsr.intenable %0" :: "a"(i));
    for (i = 0; i < XCHAL_NUM_TIMERS; i++) {
        printf("Timer %d: %d secure ints handled on time, %d secure ints handled late\n",
                i, timer_ints_handled[i], timer_ints_handled_late[i]);
        if ((timer_ints_handled[i] == 0) && (timer_ints_handled_late[i] == 0)) {
            ret = -3;
        }
    }
    return 0;
}

int main(int argc, char **argv)
{
    int ret;

    UNUSED(argc);
    UNUSED(argv);

    printf("Hello, nonsecure world\n");
    ret = timer_test();
    printf("Test %s (%d)\n", ret ? "FAILED" : "PASSED", ret);
    return ret;
}

