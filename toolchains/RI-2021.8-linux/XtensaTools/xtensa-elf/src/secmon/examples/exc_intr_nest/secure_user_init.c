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
#include <string.h>


#define N_SW_INTS           1

#define TEST_ITER           1
#define TEST_STATUS_SHIFT   24
#define TEST_STATUS_MASK    0xff000000

#define UNUSED(x)           ((void)(x))


// Redefine a couple of HAL arrays to avoid libhal dependency
const uint8_t testhal_intlevel[XTHAL_MAX_INTERRUPTS] = {
    XCHAL_INT_LEVELS
};
const uint8_t testhal_inttype[XTHAL_MAX_INTERRUPTS] = {
    XCHAL_INT_TYPES
};

int32_t test_orig_secure = -1;
int32_t test_swint_num = -1;


static void set_misc0(uint32_t val)
{
#if (XCHAL_NUM_MISC_REGS > 0)
    __asm__ __volatile__ ("wsr.misc0 %0" :: "a"(val));
#endif
}

static uint32_t get_misc0(void)
{
    uint32_t ret = 0xeeeeeeee;
#if (XCHAL_NUM_MISC_REGS > 0)
    __asm__ __volatile__ ("rsr.misc0 %0" : "=a"(ret));
#endif
    return ret;
}

static uint32_t is_secure(void)
{
    uint32_t ret;
    __asm__ __volatile__ ("rsr.ps %0" : "=a"(ret));
    return (ret & PS_NSM_MASK) ? 0 : 1;
}

static uint32_t was_secure(void)
{
    uint32_t ret;
    __asm__ __volatile__ ("rsr.ps %0" : "=a"(ret));
    return (ret & PS_PNSM_MASK) ? 0 : 1;
}

static uint32_t check_ps(uint32_t pnsm_should_be_secure)
{
    if (pnsm_should_be_secure) {
        if (!is_secure() || !was_secure()) {
            return 1;
        }
    } else {
        if (!is_secure() || was_secure()) {
            return 1;
        }
    }
    return 0;
}

static void detect_report_errors(uint32_t shift)
{
    uint32_t pnsm_should_be_secure = (test_orig_secure || (shift == 8));
    uint32_t errors = check_ps(pnsm_should_be_secure);
    uint32_t misc0, prev;

    // Tally completions/errors
    misc0 = get_misc0();
    if (errors == 0) {
        prev = (misc0 >> shift) & 0xff;
        misc0 &= ~(0xff << shift);
        misc0 |= (prev + 1) << shift;
    }
    if (shift == 16) {
        misc0 |= TEST_STATUS_MASK;
    }
    set_misc0(misc0);
}


// Secure SW interrupt handler callback
static void swint_handler(void *arg)
{
    uint32_t intnum = (uint32_t)arg & 0xff;
    detect_report_errors(8);
    xthal_interrupt_clear(intnum);
}


static void illegal_exception_handler(void *arg)
{
    UserFrame * ef = (UserFrame *)arg;

    // Skip past offending illegal instruction
    ef->pc += 3;

    detect_report_errors(0);
    xthal_interrupt_trigger(test_swint_num);
    detect_report_errors(16);
}


static void trigger_exception(void)
{
    set_misc0(0);
    __asm__ __volatile__ ("ill");
}


// Hook into secure monitor init: set up some example secure interrupt handlers
int32_t secmon_user_init(void)
{
    for (test_swint_num = 0; test_swint_num < XCHAL_NUM_INTERRUPTS; test_swint_num++) {
        if ((testhal_inttype[test_swint_num] == XTHAL_INTTYPE_SOFTWARE) && 
            (testhal_intlevel[test_swint_num] > XCHAL_EXCM_LEVEL)) {
            break;
        }
    }
    if (test_swint_num != XCHAL_NUM_INTERRUPTS) {
        if (xtos_set_interrupt_handler(test_swint_num, swint_handler, (void *)test_swint_num, NULL) != 0) {
            // ERROR: interrupt handler registration
            return -1;
        }
        if (xtos_interrupt_enable(test_swint_num) != 0) {
            // ERROR: interrupt enable
            return -2;
        }
    }

    // Install secure exception handler for EXCCAUSE_ILLEGAL for test purposes
    secmon_nonsecure_exceptions[EXCCAUSE_ILLEGAL] = 1;
    if (xtos_set_exception_handler(EXCCAUSE_ILLEGAL, illegal_exception_handler, NULL) != 0) {
        // ERROR: exception handler registration
        return -3;
    }

    // Test #1: trigger nested exception/interrupt from secure code
    test_orig_secure = 1;
    trigger_exception();
    while (get_misc0() == 0) {
    }

    // Test #2: trigger nested exception/interrupt from nonsecure code (later)
    test_orig_secure = 0;
    return 0;
}

