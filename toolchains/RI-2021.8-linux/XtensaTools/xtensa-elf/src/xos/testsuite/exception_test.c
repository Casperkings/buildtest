
// exception_test.c - XOS exception handling tests

// Copyright (c) 2015 Cadence Design Systems, Inc.
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


#include <xtensa/corebits.h>
#include <xtensa/tie/xt_core.h>

#include "test.h"


#define  EXIT_CODE   (0x0a0a0505)
#define  TEST_COUNT  10

#if !USE_MAIN_THREAD
XosThread exc_test_tcb;
uint8_t   exc_test_stk[STACK_SIZE];
#endif

XosThread thd1;
XosThread thd2;

uint8_t thd1_stack[STACK_SIZE];
uint8_t thd2_stack[STACK_SIZE];

volatile int32_t exc_count = 0;


//-----------------------------------------------------------------------------
//  Test function. Will force an illegal instruction inside a loop. Exception
//  handler will (user) fix up the PC or (default) terminate the thread.
//-----------------------------------------------------------------------------
int32_t
test_func(void * arg, int32_t unused)
{
    int32_t i;

    // Force an illegal instruction. The 3-byte form of the illegal
    // instruction should be present in all configs. The handler will
    // push the PC past the offending instruction, so the loop should
    // complete successfully.
 
    for (i = 0; i < TEST_COUNT; i++) {
        __asm__ volatile 
           ("movi    a4, exc_count\n"
            "_ill\n"
            "l32i    a5, a4, 0\n"
            "addi    a5, a5, 1\n" 
            "s32i    a5, a4, 0\n" : :: "a4", "a5");
    }

    return i;
}


//-----------------------------------------------------------------------------
//  Catch illegal instruction exception, verify that the exception type is as
//  expected and fix up the exception PC.
//-----------------------------------------------------------------------------
void
exchandler1(XosExcFrame * frame)
{
#if XCHAL_HAVE_XEA2
    if (frame->exccause != EXCCAUSE_ILLEGAL) {
#else
    if ((frame->exccause & EXCCAUSE_FULLTYPE_MASK) != EXC_TYPE_ILL_INST) {
#endif
        printf("Error: exception cause does not match expected cause.\n");
        xos_thread_exit(-1);
    }

    // Move the PC past the illegal (3-byte) instruction.
    frame->pc += 3;
}


//-----------------------------------------------------------------------------
//  Test1 -- make sure that the illegal instruction exceptions are caught and
//  handled by the user handler.
//-----------------------------------------------------------------------------
int32_t
test1()
{
    int32_t  ret;
    int32_t  i;
    XosExcHandlerFunc * old;

    printf("Exception test 1 starting\n");

#if XCHAL_HAVE_XEA2
    old = xos_register_exception_handler(EXCCAUSE_ILLEGAL, exchandler1);
#else
    old = xos_register_exception_handler(EXCCAUSE_INSTRUCTION, exchandler1);
#endif
    if (old == XOS_NULL) {
        xos_fatal_error(-1, "Error registering exception handler\n");
    }

    ret = xos_thread_create(&thd1, 0, test_func, (void *)0, "thd1",
                            thd1_stack, STACK_SIZE, 2, 0, 0);
    if (ret) {
        xos_fatal_error(-1, "Error creating test threads\n");
    }

    ret = xos_thread_join(&thd1, &i);

    if (ret) {
        xos_fatal_error(-1, "Error joining thread thd1\n");
    }

    if (i != TEST_COUNT)
        printf("Test 1 failed\n");
    else
        printf("Test 1 passed\n");

    xos_list_threads();
    xos_thread_delete(&thd1);

#if XCHAL_HAVE_XEA2
    xos_register_exception_handler(EXCCAUSE_ILLEGAL, old);
#else
    xos_register_exception_handler(EXCCAUSE_INSTRUCTION, old);
#endif
    return 0;
}


//-----------------------------------------------------------------------------
//  Test2 -- run illegal instruction test and make sure it is caught by the
//  default handler.
//-----------------------------------------------------------------------------
int32_t
test2()
{
    int32_t  ret;
    int32_t  i;

    printf("Exception test 2 starting\n");

    ret = xos_thread_create(&thd1, 0, test_func, (void *)0, "thd1",
                            thd1_stack, STACK_SIZE, 1, 0, 0);
    if (ret) {
        xos_fatal_error(-1, "Error creating test threads\n");
    }

    ret = xos_thread_join(&thd1, &i);

    if (ret) {
        xos_fatal_error(-1, "Error joining thread thd1\n");
    }

    printf("Test 2 %s\n", i == XOS_ERR_UNHANDLED_EXCEPTION ? "passed" : "failed");

    xos_list_threads();
    xos_thread_delete(&thd1);

    return 0;
}


//-----------------------------------------------------------------------------
//  Test driver.
//-----------------------------------------------------------------------------
int32_t
exc_test(void * arg, int32_t unused)
{
    test1();
    test2();

#if !USE_MAIN_THREAD
    exit(0);
#endif
    return 0;
}


//-----------------------------------------------------------------------------
//  Main.
//-----------------------------------------------------------------------------
int
main()
{
    int32_t ret;

#if defined BOARD
    xos_set_clock_freq(xtbsp_clock_freq_hz());
#else
    xos_set_clock_freq(XOS_CLOCK_FREQ);
#endif

#if USE_MAIN_THREAD
    xos_start_main_ex("main", 7, STACK_SIZE);

    ret = exc_test(0, 0);
    return ret;
#else
    ret = xos_thread_create(&exc_test_tcb,
                            0,
                            exc_test,
                            0,
                            "exc_test",
                            exc_test_stk,
                            STACK_SIZE,
                            7,
                            0,
                            0);

    xos_start(0);
    return -1; // should never get here
#endif
}

