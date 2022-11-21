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

#include <xtensa/core-macros.h>
#include <xtensa/xtruntime.h>


#define TIMER_KICK_CYCLES   10000

int32_t timer_ints[4] = {
    XCHAL_TIMER0_INTERRUPT,
    XCHAL_TIMER1_INTERRUPT,
    XCHAL_TIMER2_INTERRUPT,
    XCHAL_TIMER3_INTERRUPT
};


// Cannot use HAL functions in secure monitor, as they're currently not 
// rebuilt with call0 ABI.  Reimplementing a couple here.
static void set_ccompare0(uint32_t val)
{
#if (XCHAL_NUM_TIMERS > 0)
    __asm__ __volatile__ ("wsr.ccompare0 %0" :: "a"(val));
#endif
}

static int32_t get_ccount(void)
{
    int32_t ret = 0;
    __asm__ __volatile__ ("rsr.ccount %0" : "=a"(ret));
    return ret;
}


// Secure timer handler callback
// Restart timer and inject illegal instruction for test purposes
static void timer_handler(void *arg)
{
    set_ccompare0(get_ccount() + (uint32_t)arg);
    __asm__ __volatile__ ("ill");
}


// Hook into secure monitor init: set up some example secure interrupt handlers
int32_t secmon_user_init(void)
{
    // Allow illegal instruction exceptions to be delegatable to nonsecure code
    secmon_nonsecure_exceptions[EXCCAUSE_ILLEGAL] = 1;

    if (xtos_set_interrupt_handler(XCHAL_TIMER0_INTERRUPT, timer_handler, (void *)TIMER_KICK_CYCLES, NULL) != 0) {
        return -1;
    }

    // Let nonsecure code kick off and enable interrupt when ready
    return 0;
}

