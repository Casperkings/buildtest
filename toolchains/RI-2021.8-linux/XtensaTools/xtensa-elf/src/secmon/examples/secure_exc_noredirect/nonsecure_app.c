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


#define UNUSED(x)   ((void)(x))
#define INT_CYCLES  10000


volatile int ill_count  = 0;

// Nonsecure timer handler callback
static void ill_handler(void *arg)
{
    UserFrame *ef = (UserFrame *)arg;
    ill_count++;

    // Skip past the illegal instruction
    ef->pc += 3;
}

static int32_t exception_test(void)
{
    uint32_t start;
    int32_t  ret;

    printf("Testing nonsecure exception handler does not receive exceptions from secure code...\n");

    ret = xtsm_init();
    if (ret != 0) {
        printf("Error: xtsm_init() FAILED (%d)\n", ret);
        return ret;
    }

    // Register illegal instruction exception handler with secure monitor for nonsecure handling 
    if (xtsm_set_exception_handler(EXCCAUSE_ILLEGAL, ill_handler) != 0) {
        printf("Error: Register nonsecure handling of exception %d failed unexpectedly\n", EXCCAUSE_ILLEGAL);
        return -1;
    }

    __asm__ __volatile__ ( "ill" );
    printf("%s nonsecure illegal instruction exception\n", (ill_count == 1) ? "Caught" : "Missed");
    if (ill_count != 1) {
        return -2;
    }

    printf("\nNOTE: this test passes if the monitor's secure exception trap fires.\n");

    // Timer 0 secure handler is set up to generate an illegal instruction.
    // Kick it off...
    start = xthal_get_ccount();
    xthal_set_ccompare0(start + INT_CYCLES);
    if (xtos_interrupt_enable(XCHAL_TIMER0_INTERRUPT) != 0) {
        printf("Error: could not enable secure interrupt for timer0 (%d)\n", XCHAL_TIMER0_INTERRUPT);
        return -3;
    }
    while (xthal_get_ccount() - start < INT_CYCLES * 10) {
    }

    // If illegal exception gets handled by the nonsecure handler, flag a failure.
    // Although a secondary exception or double exception might be more likely...
    return -4;
}

int main(int argc, char **argv)
{
    int32_t ret;

    UNUSED(argc);
    UNUSED(argv);

    printf("Hello, nonsecure world\n");
    ret = exception_test();
    printf("Test %s (%d) -- handled %d nonsecure exceptions\n", 
            ret ? "FAILED" : "PASSED", ret, ill_count);
    return ret;
}

