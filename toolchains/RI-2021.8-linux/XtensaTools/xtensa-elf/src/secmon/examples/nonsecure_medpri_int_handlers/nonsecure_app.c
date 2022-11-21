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


#define EXP_NUM_INTS        3
#define HANDLER_TEST_PARAM  0xcafe4500

#define UNUSED(x) ((void)(x))


volatile int32_t cb_errors;
volatile int32_t cb_count;


// Nonsecure SW interrupt handler callback
static void swint_handler(void *arg)
{
    uint32_t intnum = (uint32_t)arg & 0xff;
    if (((uint32_t)arg & 0xffffff00) != HANDLER_TEST_PARAM) {
        cb_errors++;
    }
    xthal_interrupt_clear(intnum);
    cb_count++;
}

static int32_t swint_test(void)
{
    int32_t intnum, ret, i;

    ret = xtsm_init();
    if (ret != 0) {
        printf("Error: xtsm_init() FAILED (%d)\n", ret);
        return ret;
    }

    for (intnum = 0; intnum < XCHAL_NUM_INTERRUPTS; intnum++) {
        if ((Xthal_intlevel[intnum] != XCHAL_EXCM_LEVEL) ||
            (Xthal_inttype[intnum] != XTHAL_INTTYPE_SOFTWARE)) {
            continue;
        }

        cb_count = 0;

        // Register medium priority non-secure SW interrupt handler, if available
        ret = xtsm_set_interrupt_handler(intnum, swint_handler, (void *)(HANDLER_TEST_PARAM | intnum));
        if (ret != 0) {
            printf("Error: SW interrupt %d: nonsecure handler registration failed (%d)\n", intnum, ret);
            return -1;
        }
        ret = xtos_interrupt_enable(intnum);
        if (ret != 0) {
            printf("Error: SW interrupt %d: enable failed (%d)\n", intnum, ret);
            return -2;
        }

        for (i = 0; i < EXP_NUM_INTS; i++) {
            printf("Triggering SW interrupt %d at level %d...\n", intnum, Xthal_intlevel[intnum]);
            xthal_interrupt_trigger(intnum);
        }

        if (xtos_interrupt_disable(intnum) != 0) {
            return -3;
        }

        printf("SW interrupt %d handled %d nonsecure callbacks (%d expected), %d errors\n", 
                intnum, cb_count, EXP_NUM_INTS, cb_errors);
        if ((cb_errors != 0) || (cb_count != EXP_NUM_INTS)) {
            return -4;
        }

        ret = xtsm_set_interrupt_handler(intnum, NULL, NULL);
        if (ret != 0) {
            printf("Error: SW interrupt %d: de-registration failed (%d)\n", intnum, ret);
            return -5;
        }
        ret = xtsm_set_interrupt_handler(intnum, NULL, NULL);
        if (ret == 0) {
            printf("Error: SW interrupt %d: repeat de-registration should have failed (%d)\n", intnum, ret);
            return -6;
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
    ret = swint_test();
    printf("Test %s (%d)\n", ret ? "FAILED" : "PASSED", ret);
    return ret;
}

