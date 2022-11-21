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

#define UNUSED(x) ((void)(x))

static void swint_handler(void *arg)
{
    UNUSED(arg);
}

static int32_t swint_test(void)
{
    int32_t intnum, ret, cb_count, i, misc;

    ret = xtsm_init();
    if (ret != 0) {
        printf("Error: xtsm_init() FAILED (%d)\n", ret);
        return ret;
    }

    for (intnum = 0; intnum < XCHAL_NUM_INTERRUPTS; intnum++) {
        if ((Xthal_intlevel[intnum] == 1) ||
            (Xthal_intlevel[intnum] > XCHAL_EXCM_LEVEL) ||
            (Xthal_inttype[intnum] != XTHAL_INTTYPE_SOFTWARE)) {
            continue;
        }

        ret = xtsm_set_interrupt_handler(intnum, swint_handler, NULL);
        if (ret == 0) {
            printf("Error: SW interrupt %d: nonsecure registration should have failed (%d)\n", intnum, ret);
            return -1;
        }

        cb_count = 0;
        for (i = 0; i < EXP_NUM_INTS; i++) {
            printf("Triggering SW interrupt %d at level %d...\n", intnum, Xthal_intlevel[intnum]);
#if (XCHAL_NUM_MISC_REGS >= 1)
            misc = XT_RSR_MISC0();
            if (misc != 0) {
                printf("Error: SW interrupt flag (misc0) should be 0, was 0x%x\n", misc);
                return -2;
            }
#endif
            xthal_interrupt_trigger(intnum);
            __asm__ __volatile__ ("nop");       // need an extra instruction for int to fire
#if (XCHAL_NUM_MISC_REGS >= 1)
            misc = XT_RSR_MISC0();
            if (misc == (1 << intnum)) {
                cb_count++;
            } else {
                printf("Error: SW interrupt flag (misc0) should be 0x%x, was 0x%x\n", (1 << intnum), misc);
                return -3;
            }
            XT_WSR_MISC0(0);
#endif
        }

        if (xtos_interrupt_disable(intnum) != 0) {
            return -4;
        }

        printf("SW interrupt %d handled %d secure callbacks (%d expected)\n",
                intnum, cb_count, EXP_NUM_INTS);
        if (cb_count != EXP_NUM_INTS) {
            return -5;
        }

        ret = xtsm_set_interrupt_handler(intnum, NULL, NULL);
        if (ret == 0) {
            printf("Error: SW interrupt %d: de-registration should have failed (%d)\n", intnum, ret);
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

