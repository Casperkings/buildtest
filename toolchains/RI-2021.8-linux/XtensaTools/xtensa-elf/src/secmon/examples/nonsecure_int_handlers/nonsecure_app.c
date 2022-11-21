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

#if (defined XCHAL_HAVE_SECURE_INSTRAM1) && XCHAL_HAVE_SECURE_INSTRAM1
#define SECURE_START    XCHAL_INSTRAM1_SECURE_START
#elif (defined XCHAL_HAVE_SECURE_INSTRAM0) && XCHAL_HAVE_SECURE_INSTRAM0
#define SECURE_START    XCHAL_INSTRAM0_SECURE_START
#elif (defined XCHAL_HAVE_SECURE_INSTROM0) && XCHAL_HAVE_SECURE_INSTROM0
#define SECURE_START    XCHAL_INSTROM0_SECURE_START
#endif

#if (defined XCHAL_HAVE_SECURE_DATARAM1) && XCHAL_HAVE_SECURE_DATARAM1
#define SECURE_DSTART   XCHAL_DATARAM1_SECURE_START
#elif (defined XCHAL_HAVE_SECURE_DATARAM0) && XCHAL_HAVE_SECURE_DATARAM0
#define SECURE_DSTART   XCHAL_DATARAM0_SECURE_START
#endif

#define PROF_NUM        3


const int32_t timer_ints[4] = {
    XCHAL_TIMER0_INTERRUPT,
    XCHAL_TIMER1_INTERRUPT,
    XCHAL_TIMER2_INTERRUPT,
    XCHAL_TIMER3_INTERRUPT
};

volatile int32_t cb_errors[XCHAL_NUM_TIMERS];
volatile int32_t cb_count[XCHAL_NUM_TIMERS];
volatile int32_t cb2[XCHAL_NUM_TIMERS];
volatile int32_t cb_ccount;
int32_t prof_latency[PROF_NUM];
int32_t prof_variance;


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

static void timer_handler_latency(void *arg)
{
    cb_ccount = xthal_get_ccount(); // read ccount first
    timer_handler(arg);
}

static void timer_handler2(void *arg)
{
    uint32_t timer = (uint32_t)arg & 0xff;
    cb2[timer] = 1;
    timer_handler(arg);
}

static int32_t trigger_and_handle_int(int32_t timer, int32_t profile)
{
    int32_t ccount, stoptime;
    int32_t expected_ints = profile ? 1 : EXP_NUM_INTS;
    int32_t trig_ccount[16];
    int32_t i = 0, p, ret;

    for (p = 0; p < PROF_NUM; p++) {
        cb_ccount = 0;
        cb_count[timer] = 0;
        for (i = 0; i < 16; i++) {
            trig_ccount[i] = 0;
        }

        // Reset ccompare before enabling interrupt
        ccount = xthal_get_ccount();
        xthal_set_ccompare(timer, ccount + TIMER_KICK_CYCLES);
        stoptime = ccount + EXP_NUM_INTS * TIMER_KICK_CYCLES + (TIMER_KICK_CYCLES / 2);

        ret = xtos_interrupt_enable(timer_ints[timer]);
        if (ret != 0) {
            printf("xtos_interrupt_enable(): %d (int %d max %d)\n", 
                    ret, timer_ints[timer], XCHAL_NUM_INTERRUPTS);
            return -15;
        }

        if (profile) {
            // Don't check against stoptime for more accurate profiling
            i = 0;
            while (cb_ccount == 0) {
                trig_ccount[i] = xthal_get_ccount();
                i = (i + 1) & 15;
            }
        }
        else {
            while (ccount < stoptime) {
                ccount = xthal_get_ccount();
            }
        }

        ret = xtos_interrupt_disable(timer_ints[timer]);
        if (ret != 0) {
            printf("xtos_interrupt_disable(): %d\n", ret);
            return -16;
        }

        printf("timer %d (int %d): handled %d nonsecure callbacks (%d expected), %d errors\n", 
                timer, timer_ints[timer], cb_count[timer], expected_ints, cb_errors[timer]);
        if ((cb_errors[timer] != 0) || (cb_count[timer] != expected_ints)) {
            return -17;
        }

        if (!profile) {
            // Stop after 1 test if not profiling
            break;
        }

        for (i = 0; i < 16; i++) {
            if ((prof_variance == 0) && (i < 15) && trig_ccount[i] && trig_ccount[i+1] && 
                ((trig_ccount[i+1] - trig_ccount[i]) > 0)) {
                prof_variance = trig_ccount[i+1] - trig_ccount[i];
            }
            if (trig_ccount[i] < cb_ccount) {
                if (prof_latency[p] == 0) {
                    prof_latency[p] = cb_ccount - trig_ccount[i];
                } else if (cb_ccount - trig_ccount[i] < prof_latency[p]) {
                    prof_latency[p] = cb_ccount - trig_ccount[i];
                }
            }
        }
    }

    return 0;
}

static int32_t timer_test(void)
{
    int32_t timer, ret, i;

    ret = xtsm_init();
    if (ret != 0) {
        printf("Error: xtsm_init() FAILED (%d)\n", ret);
        return ret;
    }

    for (timer = 0; timer < XCHAL_NUM_TIMERS; timer++) {
        if (Xthal_intlevel[timer_ints[timer]] <= XCHAL_EXCM_LEVEL) {
            // Attempt some registration calls that should fail
            ret = xtsm_set_interrupt_handler(timer_ints[timer], 
                                             (xtos_handler)SECURE_START, 
                                             NULL);
            if (ret == 0) {
                printf("Error: timer %d (int %d): secure handler registration should have failed (%d)\n",
                        timer, timer_ints[timer], ret);
                return -1;
            }
            ret = xtsm_set_interrupt_handler(timer_ints[timer], 
                                             (xtos_handler)SECURE_DSTART, 
                                             NULL);
            if (ret == 0) {
                printf("Error: timer %d (int %d): secure handler registration should have failed (%d)\n",
                        timer, timer_ints[timer], ret);
                return -2;
            }
            ret = xtsm_set_interrupt_handler(timer_ints[timer], NULL, NULL);
            if (ret == 0) {
                printf("Error: timer %d (int %d): de-registration should have failed (%d)\n",
                        timer, timer_ints[timer], ret);
                return -3;
            }
            ret = xtsm_set_interrupt_handler(XCHAL_NUM_INTERRUPTS, NULL, NULL);
            if (ret == 0) {
                printf("Error: timer %d (int %d): secure handler registration should have failed (%d)\n",
                        timer, XCHAL_NUM_INTERRUPTS, ret);
                return -4;
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
        } else {
            // Confirm registering a higher-priority timer fails as expected
            ret = xtsm_set_interrupt_handler(timer_ints[timer], 
                                             timer_handler, 
                                             (void *)(HANDLER_TEST_PARAM | timer));
            if (ret == 0) {
                printf("Error: timer %d (int %d): nonsecure registration should have failed (%d)\n",
                        timer, timer_ints[timer], ret);
                return -6;
            }
            continue;
        }

        // Trigger interrupt and confirm nonsecure handling
        ret = trigger_and_handle_int(timer, 0);
        if (ret != 0) {
            printf("Error: trigger_and_handle_int(%d, 0): %d\n", timer, ret);
            return ret;
        }

        // Test re-registering of nonsecure handler
        ret = xtsm_set_interrupt_handler(timer_ints[timer], 
                                         timer_handler2, 
                                         (void *)(HANDLER_TEST_PARAM | timer));
        if (ret != 0) {
            printf("Error: timer %d (int %d): nonsecure re-registration failed (%d)\n",
                    timer, timer_ints[timer], ret);
            return -6;
        }

        // Trigger interrupt and confirm nonsecure handling
        ret = trigger_and_handle_int(timer, 0);
        if (ret != 0) {
            printf("Error: trigger_and_handle_int(%d, 0): %d\n", timer, ret);
            return ret;
        }

        if (cb2[timer] != 1) {
            printf("Error: reregistring second handler failed\n");
            return -10;
        }

        // Register different handler to measure interrupt latency
        ret = xtsm_set_interrupt_handler(timer_ints[timer],
                                         timer_handler_latency,
                                         (void *)(HANDLER_TEST_PARAM | timer));
        if (ret != 0) {
            printf("Error: timer %d (int %d): nonsecure re-registration for latency failed (%d)\n",
                    timer, timer_ints[timer], ret);
            return -11;
        }

        if (timer == 0) {
            // Trigger interrupt in profile mode and confirm nonsecure handling
            ret = trigger_and_handle_int(timer, 1);
            if (ret != 0) {
                printf("Error: trigger_and_handle_int(%d, 1): %d\n", timer, ret);
                return ret;
            }

            printf("INFO: nonsecure interrupt trigger-to-C-handler latency (+/- %d cycles):\n",
                    prof_variance);
            for (i = 0; i < PROF_NUM; i++) {
                printf("      [%d] : %d cycles\n", i, prof_latency[i]);
            }
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

