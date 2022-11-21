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
#include <xtensa/config/tie.h>

#include <xtensa/core-macros.h>
#include <xtensa/xtruntime.h>
#include <stdio.h>


#define UNUSED(x) ((void)(x))

#if (XCHAL_CP_NUM != 0)

float       f1 = 2.35;
float       f2 = 9.1415;
float       quotient = 0.0;
const float exp_quotient = 3.89;
int32_t     cp0_count = 0;
uint32_t    fcr = 0;

// Nonsecure CP0 disabled handler
static void cp0_handler(void *arg)
{
    UNUSED(arg);
    cp0_count++;
    xthal_set_cpenable(1);
}

static int32_t coproc_test(void)
{
    quotient = f2 / f1;
    printf("Divide result: %f (expected %f)\n", quotient, exp_quotient);
    return (quotient == exp_quotient);
}

#endif


int main(int argc, char **argv)
{
#if (XCHAL_CP_NUM == 0)
    UNUSED(argc);
    UNUSED(argv);

    printf("No Coprocessors found, skipping test (PASSED)\n");
    return 0;
#else
    int32_t excnum = EXCCAUSE_CP0_DISABLED;
    int ret, ret1, ret2, ret3;

    UNUSED(argc);
    UNUSED(argv);

    printf("Hello, nonsecure world\n");
    xthal_set_cpenable(0);
    printf("CP exceptions disabled, attempting FPU access...\n");
    ret1 = coproc_test();
    if (cp0_count != 0) {
        printf("Error: unexpected nonsecure exception taken\n");
        ret1 = 1;
    }
    printf("1. Unhandled (secure) CP0 disabled exception test: %s (%d)\n\n", 
            ret1 ? "FAILED" : "PASSED", ret1);
    xthal_set_cpenable(0);
    quotient = 0.0;

    ret = xtsm_init();
    if (ret != 0) {
        printf("Error: xtsm_init() FAILED (%d)\n", ret);
        return ret;
    }

#if (XCHAL_HAVE_DFPU_SINGLE_ONLY) || (XCHAL_HAVE_DFPU_SINGLE_DOUBLE)
    // Register CP0 disabled exception handler for nonsecure handling 
    ret = xtsm_set_exception_handler(excnum, cp0_handler);
    if (ret != 0) {
        printf("Error: Register nonsecure handling of exception %d failed unexpectedly\n", excnum);
        return -1;
    }

    printf("CP exceptions disabled, attempting FPU access...\n");
    ret2 = coproc_test();
    if (cp0_count != 1) {
        printf("Error: Nonsecure exception handling: %d (expected 1)\n", cp0_count);
        ret2 = 2;
    }
    printf("2. Handled (nonsecure) CP0 disabled exception test: %s (%d)\n\n",
            ret2 ? "FAILED" : "PASSED", ret2);
    xthal_set_cpenable(0);
    quotient = 0.0;

    // Register CP0 disabled exception handler for nonsecure handling 
    ret = xtsm_set_exception_handler(excnum, NULL);
    if (ret != 0) {
        printf("Error: De-register nonsecure handling of exception %d failed unexpectedly\n", excnum);
        return -2;
    }

    printf("CP exceptions disabled, attempting FPU access...\n");
    ret3 = coproc_test();
    if (cp0_count != 1) {
        printf("Error: Nonsecure exception handling: %d (expected 1)\n", cp0_count);
        ret3 = 3;
    }
    printf("3. Handled (secure2) CP0 disabled exception test: %s (%d)\n\n", 
            ret3 ? "FAILED" : "PASSED", ret3);
    xthal_set_cpenable(0);
    quotient = 0.0;
#else
    ret = ret2 = ret3 = 0;
    UNUSED(excnum);
    cp0_handler(NULL);  // unused
#endif

    printf("Coprocessor disabled exception test %s\n", (ret1 || ret2 || ret3) ? "FAILED" : "PASSED");
    return ret1 || ret2 || ret3;
#endif
}

