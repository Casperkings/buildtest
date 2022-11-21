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


#include <xtensa/core-macros.h>

#include "xtensa/secmon-secure.h"


// Secure SW interrupt callback
static void swint_handler(void *arg)
{
    uint32_t intnum = (uint32_t)arg;
    uint32_t m0;

    // OK to reference in secure code since this is inlined
    xthal_interrupt_clear(intnum);

    // Flag for nonsecure test to register interrupt was serviced
#if (XCHAL_NUM_MISC_REGS >= 1)
    m0 = XT_RSR_MISC0();
    m0 |= (1 << intnum);
    XT_WSR_MISC0(m0);
#endif
}


static volatile int32_t fast_swint_count = 0;

// Secure fast_swint handler callback
// Trigger software interrupts back-to-back to exercise dispatch handler loop
// Errors will cause exceptions
static void fast_swint_handler(void *arg)
{
    int32_t intnum = (int32_t)arg;
    if (++fast_swint_count < 5) {
        xthal_interrupt_trigger(intnum);
    }
    // Call0 convention is all except a12-a15 are callee-saved
    __asm__ __volatile__ ( "movi a2, 0\n"
                           "movi a3, 0\n"
                           "movi a4, 0\n"
                           "movi a5, 0\n"
                           "movi a6, 0\n"
                           "movi a7, 0\n"
                           "movi a8, 0\n"
                           "movi a9, 0\n"
                           "movi a10, 0\n"
                           "movi a11, 0\n" );
}


const uint8_t inttype[XTHAL_MAX_INTERRUPTS] = { XCHAL_INT_TYPES };

// Hook into secure monitor init: set up some example secure interrupt handlers
int32_t secmon_user_init(void)
{
    // To avoid HAL dependency
    int32_t intnum, ret;
    uint32_t save_ps;

#if (XCHAL_NUM_MISC_REGS >= 1)
    XT_WSR_MISC0(0);
#endif
    for (intnum = 0; intnum < XCHAL_NUM_INTERRUPTS; intnum++) {
        if ((Xthal_intlevel[intnum] == 1) ||
            (Xthal_intlevel[intnum] > XCHAL_EXCM_LEVEL) ||
            (inttype[intnum] != XTHAL_INTTYPE_SOFTWARE)) {
            continue;
        }

        // 1: Register medium priority secure SW interrupt handler for fast back-to-back test
        ret = xtos_set_interrupt_handler(intnum, fast_swint_handler, (void *)intnum, NULL);
        if (ret != 0) {
            return -1;
        }
        ret = xtos_interrupt_enable(intnum);
        if (ret != 0) {
            return -2;
        }
        // Fast back-to-back interrupt testing runs at L0 to support all timers
        save_ps = (uint32_t) XT_RSIL(0);
        xthal_interrupt_trigger(intnum);
        while (fast_swint_count < 5) {
        }
        XT_WSR_PS(save_ps);
        // Fast timer test passed if execution gets here

        // 2: Re-register medium priority secure SW handler for full test
        ret = xtos_set_interrupt_handler(intnum, swint_handler, (void *)intnum, NULL);
        if (ret != 0) {
            return -3;
        }
    }
    return 0;
}

