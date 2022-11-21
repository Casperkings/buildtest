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
#include <string.h>


#define N_TEST_FIELDS       3
#define TIMEOUT_CYCLES      100000

#define TEST_ITER           1
#define TEST_STATUS_SHIFT   24
#define TEST_STATUS_MASK    0xff000000

#define UNUSED(x)           ((void)(x))


int32_t cb_errors = 0;


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

static uint32_t get_test_results(uint32_t misc0)
{
    uint32_t errors = 0, test_swint_num;
    for (uint32_t i = 0; i < N_TEST_FIELDS; i++) {
        uint32_t val = ((misc0 >> (i * 8)) & 0xff);
        if (val != TEST_ITER) {
            if (i == 1) {
                // skip results for SW interrupt if no valid one was found
                for (test_swint_num = 0; test_swint_num < XCHAL_NUM_INTERRUPTS; test_swint_num++) {
                    if ((Xthal_inttype[test_swint_num] == XTHAL_INTTYPE_SOFTWARE) && 
                        (Xthal_intlevel[test_swint_num] > XCHAL_EXCM_LEVEL)) {
                        break;
                    }
                }
                if (test_swint_num == XCHAL_NUM_INTERRUPTS) {
                    printf("INFO: skipping test field %d; no valid SW interrupt\n", i);
                    continue;
                }
            }
            printf("ERROR: test field %d should be %d but was %d\n", i, TEST_ITER, val);
            errors += i;
        }
    }
    return errors;
}


static void trigger_exception(void)
{
    set_misc0(0);
    __asm__ __volatile__ ("ill");
}


static int32_t exc_intr_nest_test(void)
{
    // Test triggers an exception and waits on misc0
    // to reflect test completion (bits 31:24 == 0xff) and status, as follows:
    //
    // bits 7:0 indicate # exception entries correct;
    // bits 15:8 indicate # nested SW L1 intr correct;
    // bits 23:16 indicate # exception exits correct.
    //
    // NOTE: all exceptions and interrupts handled in secure space.

    uint32_t misc0 = get_misc0();
    uint32_t start, errors = 0;

    // Secure nest test (#1) has already been run; results in misc0
    printf("Secure nested interrupt test complete (0x%08x)\n", misc0);
    errors = get_test_results(misc0);
    printf("...test %s\n", errors == 0 ? "OK" : "FAILED");

    // Run nonsecure nest test (#2)
    printf("Nonsecure nested interrupt test running...\n");
    trigger_exception();
    misc0 = get_misc0();
    start = xthal_get_ccount();
    while ((misc0 & TEST_STATUS_MASK) != TEST_STATUS_MASK) {
        misc0 = get_misc0();
        if (xthal_get_ccount() - start > TIMEOUT_CYCLES) {
            printf("Error: timeout\n");
            errors++;
            break;
        }
    }
    errors += get_test_results(misc0);
    printf("...test %s (0x%08x)\n", errors == 0 ? "OK" : "FAILED", misc0);

    printf("Nested interrupt test complete (0x%x): %d errors\n", misc0, errors);
    return errors;
}


int main(int argc, char **argv)
{
    int32_t errors = 0;

#if (XCHAL_NUM_MISC_REGS < 1)
    printf("INFO: test requires at least 1 misc reg; skipping\n");
#else
    errors = exc_intr_nest_test();
#endif

    printf("Test %s (%d)\n", errors ? "FAILED" : "PASSED", errors);

    UNUSED(argc);
    UNUSED(argv);

    return errors;
}

