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

#include <alloca.h>


#define ALLOCA_SIZE     200
#define UNUSED(x)       ((void)(x))

#if (defined XCHAL_HAVE_SECURE_INSTRAM1) && XCHAL_HAVE_SECURE_INSTRAM1
#define SECURE_START    XCHAL_INSTRAM1_SECURE_START
#elif (defined XCHAL_HAVE_SECURE_INSTRAM0) && XCHAL_HAVE_SECURE_INSTRAM0
#define SECURE_START    XCHAL_INSTRAM0_SECURE_START
#elif (defined XCHAL_HAVE_SECURE_INSTROM0) && XCHAL_HAVE_SECURE_INSTROM0
#define SECURE_START    XCHAL_INSTROM0_SECURE_START
#endif


void * alloca_test(int size);

// alloca() pointers declared global to fool the optimizer
void *g1 = NULL;
void *g2 = NULL;
int32_t errors = 0;

void * alloca_test(int size)
{
    g1 = alloca(size);
    g2 = alloca(size);
    printf("alloca_test: %p %p\n", g1, g2);
    if (!g1 || !g2 || (g1 == g2)) {
        printf("Error\n");
        errors++;
    }
    return (g1);
}

int main(int argc, char **argv)
{
    void *pa = NULL;
    void *pb = NULL;
    int ret;

    UNUSED(argc);
    UNUSED(argv);

    printf("Hello, nonsecure world\n");

    // Calling alloca() prior to xtsm_init() is fatal; uncomment to test manually...
    // printf("This call should trigger an unhandled exception in secure monitor\n");
    // pa = alloca_test(ALLOCA_SIZE);

    printf("Testing XTOS alloca handling\n");
    ret = xtsm_init();
    printf("xtsm_init(): %s\n", (ret == 0) ? "OK" : "Error");

    if (ret == 0) {
        pa = alloca_test(ALLOCA_SIZE);
        pb = alloca_test(ALLOCA_SIZE);
        // alloca() pointers are freed after the function returns, so both 
        // alloca_test() calls should return the same value.
        ret = (pa == pb) ? 0 : 1;
    } else {
        printf("skipping remainder of test...\n");
    }
    printf("alloca() exception test (%p %p), ret %d: %s\n", 
            pa, pb, ret, ((ret == 0) && (errors == 0)) ? "PASSED" : "FAILED");
    return ret;
}

