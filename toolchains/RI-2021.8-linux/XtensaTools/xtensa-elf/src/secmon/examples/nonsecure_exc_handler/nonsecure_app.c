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
#define EXP_COUNT   5

#if (defined XCHAL_HAVE_SECURE_INSTRAM1) && XCHAL_HAVE_SECURE_INSTRAM1
#define SECURE_START    XCHAL_INSTRAM1_SECURE_START
#elif (defined XCHAL_HAVE_SECURE_INSTRAM0) && XCHAL_HAVE_SECURE_INSTRAM0
#define SECURE_START    XCHAL_INSTRAM0_SECURE_START
#elif (defined XCHAL_HAVE_SECURE_INSTROM0) && XCHAL_HAVE_SECURE_INSTROM0
#define SECURE_START    XCHAL_INSTROM0_SECURE_START
#endif

#if (defined XCHAL_HAVE_SECURE_DATARAM1) && XCHAL_HAVE_SECURE_DATARAM1
#define SECURE_DSTART   XCHAL_DATARAM1_SECURE_START
#elif (defined XCHAL_HAVE_SECURE_DATARAM0) && XCHAL_HAVE_SECURE_DATARAM0
#define SECURE_DSTART   XCHAL_DATARAM0_SECURE_START
#endif


volatile int ill_count  = 0;
volatile int ill2       = 0;

// Nonsecure illegal instruction handler callback
static void ill_handler(void *arg)
{
    UserFrame *ef = (UserFrame *)arg;
    ill_count++;

    // Skip past the illegal instruction
    ef->pc += 3;
}

static void ill_handler2(void *arg)
{
    ill2 = 1;
    ill_handler(arg);
}

static int exception_test(void)
{
    int i;
    int32_t ret;
    int32_t excnum;
    int32_t data[4];
    int32_t *intptr;

    printf("Testing nonsecure exception handling APIs...\n");

    ret = xtsm_init();
    if (ret != 0) {
        printf("Error: xtsm_init() FAILED (%d)\n", ret);
        return ret;
    }

    // Attempt calls with an exception that should not be routable to nonsecure code
    excnum = EXCCAUSE_INSTR_PROHIBITED;
    if (xtsm_set_exception_handler(excnum, ill_handler) == 0) {
        printf("Error: Register nonsecure handling of exception %d did not fail as expected\n", excnum);
        return -1;
    }
    if (xtsm_set_exception_handler(excnum, NULL) == 0) {
        printf("Error: Deregister of unregistered exception %d did not fail as expected\n", excnum);
        return -2;
    }
    excnum = EXCCAUSE_LEVEL1_INTERRUPT;
    if (xtsm_set_exception_handler(excnum, ill_handler) == 0) {
        printf("Error: Register nonsecure handling of exception %d did not fail as expected\n", excnum);
        return -3;
    }
    excnum = XCHAL_EXCCAUSE_NUM;
    if (xtsm_set_exception_handler(excnum, ill_handler) == 0) {
        printf("Error: Register nonsecure handling of exception %d did not fail as expected\n", excnum);
        return -4;
    }
    excnum = EXCCAUSE_UNALIGNED;
    if (xtsm_set_exception_handler(excnum, ill_handler) == 0) {
        printf("Error: Register nonsecure handling of exception %d did not fail as expected\n", excnum);
        return -5;
    }
    if (xtsm_set_exception_handler(excnum, (xtos_handler)SECURE_START) == 0) {
        printf("Error: Register nonsecure handling of exception %d with secure handler did not fail as expected\n", excnum);
        return -6;
    }

    // Already installed custom secure handler; attempt to override with nonsecure handler
    excnum = EXCCAUSE_LOAD_STORE_ERROR;
    if (xtsm_set_exception_handler(excnum, ill_handler) == 0) {
        printf("Error: Register nonsecure handling of secure exception %d did not fail as expected\n", excnum); 
        return -7;
    }
    // Trigger unaligned load-store exception and confirm handling didn't come to nonsecure code
    intptr = (int32_t *)&(((uint8_t *)data)[1]);
    data[1] = *intptr;
    if (ill_count != 0) {
        printf("Error: Nonsecure exception %d handler overrode secure handler\n", excnum);
        return -10;
    }

    // Attempt prohibited calls with an exception that can be routable to nonsecure code
    excnum = EXCCAUSE_ILLEGAL;
    if (xtsm_set_exception_handler(excnum, NULL) == 0) {
        printf("Error: Deregister of unregistered exception %d did not fail as expected\n", excnum);
        return -20;
    }
    if (xtsm_set_exception_handler(excnum, (xtos_handler)SECURE_START) == 0) {
        printf("Error: Register nonsecure handling of exception %d with secure handler did not fail as expected\n", excnum);
        return -21;
    }
    if (xtsm_set_exception_handler(excnum, (xtos_handler)SECURE_DSTART) == 0) {
        printf("Error: Register nonsecure handling of exception %d with secure handler did not fail as expected\n", excnum);
        return -22;
    }

    // Register illegal instruction exception handler with secure monitor for nonsecure handling 
    if (xtsm_set_exception_handler(excnum, ill_handler) != 0) {
        printf("Error: Register nonsecure handling of exception %d failed unexpectedly\n", excnum);
        return -30;
    }

    printf("INFO: Nonsecure exception handling API tests complete, no errors\n");

    for (i = 0; i < EXP_COUNT; i++) {
        if (i == EXP_COUNT - 1) {
            // Confirm reregistration of exception handling works
            if (xtsm_set_exception_handler(excnum, ill_handler2) != 0) {
                printf("Error: Re-register nonsecure handling of exception %d failed unexpectedly\n", excnum);
                return -31;
            }
        }
        __asm__ __volatile__ ( "ill" );
        printf("%s illegal instruction exception\n", (ill_count == i+1) ? "Caught" : "Missed");
    }

    if (ill2 != 1) {
        printf("Error: Re-registered exception handler never triggered\n");
        return -32;
    }

    // Unregister illegal instruction exception handler 
    if (xtsm_set_exception_handler(excnum, NULL) != 0) {
        printf("Error: Unregister nonsecure handling of exception %d failed unexpectedly\n", excnum);
        return -40;
    }
    if (xtsm_set_exception_handler(excnum, NULL) == 0) {
        printf("Error: Repeated unregister nonsecure handling of exception %d did not fail as expected\n", excnum);
        return -41;
    }

    // Uncomment the following to trigger an unhandled illegal instruction, which should stop the
    // test in the debugger.  Consider halted execution a pass, and continued execution a failure.
    //__asm__ __volatile__ ( "ill" );

    return (ill_count == EXP_COUNT) ? 0 : 1;
}

int main(int argc, char **argv)
{
    int ret;

    UNUSED(argc);
    UNUSED(argv);

    printf("Hello, nonsecure world\n");
    ret = exception_test();
    printf("Test %s (%d) -- handled %d nonsecure exceptions\n", 
            ret ? "FAILED" : "PASSED", ret, ill_count);
    return ret;
}

