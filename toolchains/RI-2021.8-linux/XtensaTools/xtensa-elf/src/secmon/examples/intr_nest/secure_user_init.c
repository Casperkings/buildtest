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


#define N_SW_INTS           3

#define TEST_ITER           1
#define TEST_STATUS_SHIFT   24
#define TEST_STATUS_MASK    0xff000000

#define UNUSED(x)           ((void)(x))


// Table of SW interrupts indexed by level; should have 3 set.
int32_t  sw_ints_by_level[XCHAL_NUM_INTLEVELS + 1];
int32_t  shift[XCHAL_NUM_INTLEVELS + 1];
uint32_t lowest_intlevel = 0;


// Redefine a couple of HAL arrays to avoid libhal dependency
const uint8_t testhal_intlevel[XTHAL_MAX_INTERRUPTS] = {
    XCHAL_INT_LEVELS
};
const uint8_t testhal_inttype[XTHAL_MAX_INTERRUPTS] = {
    XCHAL_INT_TYPES
};


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

static uint32_t check_ps(uint32_t level)
{
    if (level == lowest_intlevel) {
        if (!is_secure() || was_secure()) {
            return 1;
        }
    } else {
        if (!is_secure() || !was_secure()) {
            return 1;
        }
    }
    return 0;
}


// Secure SW interrupt handler callback
static void swint_handler(void *arg)
{
    uint32_t intnum = (uint32_t)arg & 0xff;
    uint32_t level = testhal_intlevel[intnum];
    uint32_t misc0, prev, errors = 0;;

    errors += check_ps(level);

    // Clear SW interrupt and trigger next higher level from ISR
    xthal_interrupt_clear(intnum);
    for (int32_t i = testhal_intlevel[intnum] + 1; i <= XCHAL_NUM_INTLEVELS; i++) {
        if (sw_ints_by_level[i] != -1) {
            xthal_interrupt_trigger(sw_ints_by_level[i]);
            break;
        }
    }

    errors += check_ps(level);

    // Tally completions/errors
    misc0 = get_misc0();
    if ((errors == 0) && (shift[level] >= 0)) {
        prev = (misc0 >> shift[level]) & 0xff;
        misc0 &= ~(0xff << shift[level]);
        misc0 |= (prev + 1) << shift[level];
    }
    if (level == lowest_intlevel) {
        // Test complete
        misc0 |= TEST_STATUS_MASK;
    }
    set_misc0(misc0);
}


// Hook into secure monitor init: set up some example secure interrupt handlers
int32_t secmon_user_init(void)
{
    int32_t n_sw_ints = 0;
    int32_t shift_bits = 0;
    memset(sw_ints_by_level, -1, (XCHAL_NUM_INTLEVELS + 1) * sizeof(int32_t));
    memset(shift, -1, (XCHAL_NUM_INTLEVELS + 1) * sizeof(int32_t));
    for (int32_t i = 0; i < XCHAL_NUM_INTERRUPTS; i++) {
        if ((testhal_inttype[i] == XTHAL_INTTYPE_SOFTWARE) && 
            (testhal_intlevel[i] >= XCHAL_EXCM_LEVEL) &&
            (sw_ints_by_level[testhal_intlevel[i]] == -1)) {
            sw_ints_by_level[testhal_intlevel[i]] = i;
            if (xtos_set_interrupt_handler(i, swint_handler, (void *)i, NULL) != 0) {
                // ERROR: interrupt handler registration
                return -1;
            }
            if (xtos_interrupt_enable(i) != 0) {
                // ERROR: interrupt enable
                return -2;
            }
            if (++n_sw_ints == N_SW_INTS) {
                break;
            }
        }
    }
    if (n_sw_ints == N_SW_INTS) {
        for (int32_t i = 0; i <= XCHAL_NUM_INTLEVELS; i++) {
            if (sw_ints_by_level[i] != -1) {
                shift[i] = shift_bits;
                shift_bits += 8;
                if (lowest_intlevel == 0) {
                    lowest_intlevel = i;
                }
            }
        }
    }
    return 0;
}

