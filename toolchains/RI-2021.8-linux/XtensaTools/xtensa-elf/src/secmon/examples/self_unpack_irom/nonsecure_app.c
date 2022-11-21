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


#define TIMER_KICK_CYCLES   10000
#define TIMER_KICK_INCR     1000
#define TIMER_TEST_CYCLES   (TIMER_KICK_CYCLES * 10)

#define UNUSED(x) ((void)(x))


int timer_ints_handled[4]       = { 0, 0, 0, 0 };
int timer_ints_handled_late[4]  = { 0, 0, 0, 0 };
int timer_increments[4]         = { 
    TIMER_KICK_CYCLES,
    TIMER_KICK_CYCLES + TIMER_KICK_INCR,
    TIMER_KICK_CYCLES + TIMER_KICK_INCR * 2,
    TIMER_KICK_CYCLES + TIMER_KICK_INCR * 3
};


static int timer_test(void)
{
    int last_ccompare[4] = { 0, 0, 0, 0 };
    int start_ccount = xthal_get_ccount();
    int ccompare;
    int zero = 0;

    last_ccompare[0] = xthal_get_ccompare(0);

    // Collect interrupt handling data
    while (xthal_get_ccount() - start_ccount < TIMER_TEST_CYCLES) {
        ccompare = xthal_get_ccompare(0);
        if (ccompare - last_ccompare[0] == timer_increments[0]) {
            timer_ints_handled[0]++;
        } else if (ccompare - last_ccompare[0] != 0) {
            timer_ints_handled_late[0]++;
        }
        last_ccompare[0] = ccompare;
    }

    // Disable all interrupts and check test results
    __asm__ __volatile__ ("wsr.intenable %0" :: "a"(zero));
    return (timer_ints_handled[0] == 0) ? -1 : 0;
}

int main(int argc, char **argv)
{
    int ret;

    UNUSED(argc);
    UNUSED(argv);

    printf("Hello, nonsecure world\n");
    ret = timer_test();
    printf("Test %s (%d)\n", ret ? "FAILED" : "PASSED", ret);
    return ret;
}

