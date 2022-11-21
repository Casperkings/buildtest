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

// secure_user_init.c -- Secure monitor example code


#include "xtensa/secmon-secure.h"
#include "xtensa/secmon-common.h"

#include <xtensa/core-macros.h>
#include <xtensa/xtruntime.h>


#define TIMER_KICK_CYCLES   10000
#define TIMER_KICK_INCR     1000


// Cannot use HAL functions in secure monitor, as they're currently not 
// rebuilt with call0 ABI.  Reimplementing a couple here.
static void set_ccompare(int32_t n, uint32_t val)
{
    switch (n) {
        case 0:
#if (XCHAL_NUM_TIMERS > 0)
            __asm__ __volatile__ ("wsr.ccompare0 %0" :: "a"(val));
#endif
            break;
        case 1:
#if (XCHAL_NUM_TIMERS > 1)
            __asm__ __volatile__ ("wsr.ccompare1 %0" :: "a"(val));
#endif
            break;
        case 2:
#if (XCHAL_NUM_TIMERS > 2)
            __asm__ __volatile__ ("wsr.ccompare2 %0" :: "a"(val));
#endif
            break;
    }
}

static int32_t get_ccompare(int32_t n)
{
    int32_t ret = 0;
    switch (n) {
        case 0:
#if (XCHAL_NUM_TIMERS > 0)
            __asm__ __volatile__ ("rsr.ccompare0 %0" : "=a"(ret));
#endif
            break;
        case 1:
#if (XCHAL_NUM_TIMERS > 1)
            __asm__ __volatile__ ("rsr.ccompare1 %0" : "=a"(ret));
#endif
            break;
        case 2:
#if (XCHAL_NUM_TIMERS > 2)
            __asm__ __volatile__ ("rsr.ccompare2 %0" : "=a"(ret));
#endif
            break;
    }
    return ret;
}

static int32_t get_ccount(void)
{
    int32_t ret = 0;
    __asm__ __volatile__ ("rsr.ccount %0" : "=a"(ret));
    return ret;
}


// Secure timer handler callback
// Restart timer for a unique number of cycles so nonsecure test 
// is able to detect and validate that handlers are running.
static void timer_handler(void *arg)
{
    int32_t timer_num = (int32_t)arg;
    int32_t increment = TIMER_KICK_CYCLES + TIMER_KICK_INCR * timer_num;
    int32_t new_ccompare = get_ccompare(timer_num) + increment;
    if (new_ccompare - get_ccount() < 0) {
        // Normal increment not sufficient; too many cycles have elapsed
        // NOTE: this is most likely to occur for L1 interrupts due to 
        // monitor's nonsecure application unpack logic running at EXCM_LEVEL.
        new_ccompare = get_ccount() + increment;
    }
    set_ccompare(timer_num, new_ccompare);
}


// Hook into secure monitor init: set up some example secure interrupt handlers
int32_t secmon_user_init(void)
{
    int32_t ret;
    int32_t stack_addr = (uint32_t)&ret;
    int32_t func_addr = (uint32_t)timer_handler;
    int32_t status = 0;

    if (!xtsm_secure_addr_overlap(stack_addr, stack_addr + 4) ||
        !xtsm_secure_addr_overlap(func_addr, func_addr + 4)) {
        // Flag a failure if stack or text are in non-secure space
        status = 1;
    }

    if (status == 0) {
        // Configure interrupt handler as a sign the test passed
        ret = xtos_set_interrupt_handler(XCHAL_TIMER0_INTERRUPT, timer_handler, (void *)0, NULL);
        if (ret != 0) {
            return -1;
        }
        timer_handler((void *)0);  // set ccompare
        ret = xtos_interrupt_enable(XCHAL_TIMER0_INTERRUPT);
        if (ret != 0) {
            return -2;
        }
    } else {
        // Do not trigger interrupts as a sign the test failed
    }

    return 0;
}
