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

#include <xtensa/config/secure.h>
#include <xtensa/core-macros.h>
#include <xtensa/xtruntime.h>
#include <stdio.h>


#define TIMER_KICK_CYCLES   10000
#define EXP_NUM_INTS        3
#define HANDLER_TEST_PARAM  0xcafe4500

#define UNUSED(x) ((void)(x))


const int32_t timer_ints[4] = {
    XCHAL_TIMER0_INTERRUPT,
    XCHAL_TIMER1_INTERRUPT,
    XCHAL_TIMER2_INTERRUPT,
    XCHAL_TIMER3_INTERRUPT
};

volatile int32_t cb_errors[XCHAL_NUM_TIMERS];
volatile int32_t cb_count[XCHAL_NUM_TIMERS];


// Nonsecure timer handler callback
static void timer_handler(void *arg)
{
    uint32_t timer = (uint32_t)arg & 0xff;
    if (((uint32_t)arg & 0xffffff00) != HANDLER_TEST_PARAM) {
        cb_errors[timer]++;
    }
    cb_count[timer]++;
    xthal_set_ccompare(timer, xthal_get_ccompare(timer) + TIMER_KICK_CYCLES);
}

static int32_t trigger_and_handle_int(int32_t timer, int32_t nonsecure_handling)
{
    int32_t ccount, stoptime;
    int32_t expected_ints = (nonsecure_handling ? EXP_NUM_INTS : 0);

    // Reset ccompare before enabling interrupt
    ccount = xthal_get_ccount();
    xthal_set_ccompare(timer, ccount + TIMER_KICK_CYCLES);
    stoptime = ccount + EXP_NUM_INTS * TIMER_KICK_CYCLES + (TIMER_KICK_CYCLES / 2);

    if (xtos_interrupt_enable(timer_ints[timer]) != 0) {
        return -10;
    }

    while (ccount < stoptime) {
        ccount = xthal_get_ccount();
    }

    if (xtos_interrupt_disable(timer_ints[timer]) != 0) {
        return -11;
    }

    printf("timer %d (int %d): handled %d nonsecure callbacks (%d expected), %d errors\n", 
            timer, timer_ints[timer], cb_count[timer], expected_ints, cb_errors[timer]);
    if ((cb_errors[timer] != 0) || (cb_count[timer] != expected_ints)) {
        return -12;
    }

    return 0;
}

static int32_t timer_test(void)
{
    int32_t timer = XCHAL_NUM_TIMERS, ret, i;

    for (i = 0; i < XCHAL_NUM_TIMERS; i++) {
        if (Xthal_intlevel[timer_ints[i]] <= XCHAL_EXCM_LEVEL) {
            // Test uses first timer of appropriate level
            timer = i;
        }
    }
    if (timer == XCHAL_NUM_TIMERS) {
        printf("INFO: timer <= EXCM_LEVEL required but not found; skipping test\n");
        return 0;
    }

    ret = xtsm_init();
    if (ret != 0) {
        printf("Error: xtsm_init() FAILED (%d)\n", ret);
        return ret;
    }

    // Register timer interrupt handler with secure monitor for nonsecure handling 
    ret = xtsm_set_interrupt_handler(timer_ints[timer], 
                                     timer_handler, 
                                     (void *)(HANDLER_TEST_PARAM | timer));
    if (ret != 0) {
        printf("Error: timer %d (int %d): nonsecure registration failed (%d)\n",
                timer, timer_ints[timer], ret);
        return -5;
    }

    // Trigger interrupt with expectation of nonsecure handling
    ret = trigger_and_handle_int(timer, 1);
    if (ret != 0) {
        printf("Error: trigger_and_handle_int(%d, 1): %d\n", timer, ret);
        return ret;
    }

    ret = xtsm_set_interrupt_handler(timer_ints[timer], NULL, NULL);
    if (ret != 0) {
        printf("Error: timer %d (int %d): de-registration failed (%d)\n",
                timer, timer_ints[timer], ret);
        return -20;
    }
    ret = xtsm_set_interrupt_handler(timer_ints[timer], NULL, NULL);
    if (ret == 0) {
        printf("Error: timer %d (int %d): repeat de-registration should have failed (%d)\n",
                timer, timer_ints[timer], ret);
        return -21;
    }

    // Force another interrupt after deregistration to ensure it doesn't 
    // come to nonsecure handler
    printf("Forcing interrupt post-deregistration; should trap in secure handler\n");
    ret = trigger_and_handle_int(timer, 0);

    // Execution here indicates a failure
    printf("Error: trigger_and_handle_int(%d, 0): %d (should not have returned)\n", timer, ret);
    return -30;
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

