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


#define N_SW_INTS           3
#define TIMEOUT_CYCLES      1000000

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

static uint32_t check_ps(void)
{
    if (is_secure() || was_secure()) {
        return 1;
    }
    return 0;
}

// Nonsecure SW interrupt handler
static void nonsec_swint_handler(void *arg)
{
    int32_t sec_sw_int = (int32_t)arg;
    cb_errors = check_ps();
    printf("INFO: Non-secure SW interrupt triggering secure SW interrupt %d\n", sec_sw_int);
    xthal_interrupt_trigger(sec_sw_int);
}

static int intr_nest_test(int32_t sec_sw_int, int32_t nonsec_sw_int)
{
    // Test triggers the lowest-numbered SW interrupt and waits on misc0
    // to reflect test completion (bits 31:24 == 0xff) and status, as follows:
    // bits 7:0 indicate # SW intr A serviced correctly; 
    // bits 15:8 indicate # SW intr B serviced correctly;
    // bits 23:16 indicate # SW intr C serviced correctly.
    uint32_t misc0 = (TEST_ITER << TEST_STATUS_SHIFT);
    int32_t errors = check_ps();
    int32_t secure = (nonsec_sw_int == -1);
    int32_t ret;
    uint32_t start;

    set_misc0(misc0);
    if (secure) {
        printf("Starting secure nested interrupt test with SW int %d (level %d)...\n", 
                sec_sw_int, Xthal_intlevel[sec_sw_int]);
        xthal_interrupt_trigger(sec_sw_int);
    } else {
        printf("Starting non-secure nested interrupt test with SW int %d (level %d)...\n", 
                nonsec_sw_int, Xthal_intlevel[nonsec_sw_int]);
        ret = xtsm_set_interrupt_handler(nonsec_sw_int, nonsec_swint_handler, (void *)sec_sw_int);
        if (ret) {
            printf("Error: register nonsecure interrupt handler: %d\n", ret);
            return 1;
        }
        ret = xtos_interrupt_enable(nonsec_sw_int);
        if (ret) {
            printf("Error: enable nonsecure interrupt: %d\n", ret);
            return 1;
        }
        xthal_interrupt_trigger(nonsec_sw_int);
    }

    start = xthal_get_ccount();
    while ((misc0 & TEST_STATUS_MASK) != TEST_STATUS_MASK) {
        errors += check_ps();
        misc0 = get_misc0();
        if (xthal_get_ccount() - start > TIMEOUT_CYCLES) {
            printf("Error: timeout\n");
            errors++;
            break;
        }
    }
    errors += check_ps();
    printf("Interrupt nest: %d errors (%d in callback) during test\n", errors, cb_errors);
    errors += cb_errors;

    for (uint32_t i = 0; i < N_SW_INTS; i++) {
        uint32_t val = ((misc0 >> (i * 8)) & 0xff);
        if (val != TEST_ITER) {
            printf("ERROR: interrupt %d should be %d but was %d\n", i, TEST_ITER, val);
            errors++;
        }
    }

    printf("Nested interrupt test complete (0x%x): %d errors\n", misc0, errors);
    return errors;
}


int main(int argc, char **argv)
{
    int32_t errors = 0;

#if (XCHAL_NUM_MISC_REGS < 1)
    printf("INFO: test requires at least 1 misc reg; skipping\n");
#else
    int32_t i;
    int32_t n_sw_ints = 0;
    int32_t sw_ints_by_level[XCHAL_NUM_INTLEVELS + 1];
    int32_t lowest_sw_int_level = XCHAL_NUM_INTLEVELS;
    int32_t l1_sw_int = -1;

    errors = xtsm_init();
    if (errors) {
        printf("ERROR: xtsm_init(): %d\n", errors);
    }

    memset(sw_ints_by_level, -1, (XCHAL_NUM_INTLEVELS + 1) * sizeof(int32_t));
    for (i = 0; i < XCHAL_NUM_INTERRUPTS; i++) {
        if ((Xthal_inttype[i] == XTHAL_INTTYPE_SOFTWARE) && 
            (Xthal_intlevel[i] >= XCHAL_EXCM_LEVEL) &&
            (n_sw_ints < N_SW_INTS) &&
            (sw_ints_by_level[Xthal_intlevel[i]] == -1)) {
            sw_ints_by_level[Xthal_intlevel[i]] = i;
            if (Xthal_intlevel[i] < lowest_sw_int_level) { 
                lowest_sw_int_level = Xthal_intlevel[i];
            }
            ++n_sw_ints;
        } else if ((Xthal_inttype[i] == XTHAL_INTTYPE_SOFTWARE) && 
                   (Xthal_intlevel[i] == 1) && (l1_sw_int == -1)) {
            l1_sw_int = i;
        }
    }

    if (n_sw_ints < N_SW_INTS) {
        printf("INFO: Secure test requires at least 3 SW interrupts >= EXCM_LEVEL; skipping\n");
    } else {
        for (i = 0; i <= XCHAL_NUM_INTLEVELS; i++) {
            if (sw_ints_by_level[i] >= 0) {
                printf("INFO: Using SW interrupt %d (level %d)\n", sw_ints_by_level[i], i);
            }
        }
        errors = intr_nest_test(sw_ints_by_level[lowest_sw_int_level], -1);

        if (l1_sw_int == -1) {
            printf("INFO: Nonsecure test requires 1 L1 SW interrupt; skipping\n");
        } else {
            errors += intr_nest_test(sw_ints_by_level[lowest_sw_int_level], l1_sw_int);
        }
    }
#endif

    printf("Test %s (%d)\n", errors ? "FAILED" : "PASSED", errors);

    UNUSED(argc);
    UNUSED(argv);

    return errors;
}

