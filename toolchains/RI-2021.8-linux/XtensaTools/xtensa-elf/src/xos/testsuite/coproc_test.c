
// coproc_test.c - XOS coprocessor context switch test.

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

/*
   coproc:  This test is designed to test multiple threads using a co-processor
   and being preempted while co-processor state is valid. Multiple threads will
   repeatedly load pre-saved CP context(s) and save to a separate area, then
   compare the two copies. Timeslicing will cause preemptive context switches.
   This approach guarantees usage of the coprocessor even if no floating point
   operations are supported. For configs that do have hardware FP support, the
   second part of the test will exercise it.

   This test takes a while to run because we need to make sure there are lots of
   timeslices to preempt the threads.
*/

#include <stdio.h>
#include <string.h>

#include "test.h"

#include <xtensa/hal.h>
#if XCHAL_HAVE_HWFP
#include <xtensa/tie/xt_FP.h>
#endif

#define NTHREADS        5
#define TEST_REPS       500


// Parameters are chosen to ensure different values in each thread so that
// if the co-processor context-switch is incorrect they will not converge
// on their expected values. The first param n determines how long the
// computation runs and thus how many context switches occur. The second
// param x determines the rate of convergence (slow enough, but converges
// before the end). The third parameter z is an initial value that keeps
// the sequences from different threads in disjoint regions. Some params
// are also chosen for configs without the floating-point co-processor so
// that this test will run on any config in reasonable time.

#if XCHAL_HAVE_HWFP
    #define THREAD_0_PARAMS   0, 50000, 0.9997,  2.0
    #define THREAD_1_PARAMS   1, 50000, 0.9996,  4.0
    #define THREAD_2_PARAMS   2, 50000, 0.9995,  6.0
    #define THREAD_3_PARAMS   3, 50000, 0.9994,  8.0
#else
    #define THREAD_0_PARAMS   0,   500, 0.97,   2.0
    #define THREAD_1_PARAMS   1,   500, 0.96,   4.0
    #define THREAD_2_PARAMS   2,   500, 0.95,   6.0
    #define THREAD_3_PARAMS   3,   500, 0.94,   8.0
#endif


#if !USE_MAIN_THREAD
XosThread      coproc_test_tcb;
uint8_t        coproc_test_stk[STACK_SIZE];
#endif

XosThread      test_threads[NTHREADS];
uint8_t        stacks[NTHREADS][STACK_SIZE];
volatile float result[NTHREADS];
float          expect[NTHREADS];

char *         name[] = {"Thd0", "Thd1", "Thd2", "Thd3", "Thd4"};


// Thread function for no-CP thread
int32_t
nocp_func(void * arg, int32_t unused)
{
    while (1) {
        xos_thread_sleep(xos_get_clock_freq()/100);
    }

    return 0;
}


// Shared function that does a convergent iterative floating point computation
// a specified number of iterations and returns the result.
//    n = number of iterations (integer >= 1, can be arbitrarily large).
//    x = initial coefficient in range [0,1), close to 1 slows convergence.
//    z = arbitrary initial value > 0.
static float
crunch(int32_t which, uint32_t n, float x, float z)
{
    uint32_t i = 0, j = 0;
    float mx = -x;
    float result = z;
    
    for (i=0; i<n; i+=j) {
        for (j=0; j < n>>3; ++j) {
            result += x * z;
            x = mx * x; 
        }

        // Solicit context-switch to exercise exception handler not saving state.
        // However compiler saves/restores state around function calls, so test 
        // won't fail if exception handler merely saves/restores incorrectly,
        // only if the code for those cases is severely broken causing a crash.
        xos_thread_yield();
    }

    return result;
}


// Shared function that will either read or write all CP/TIE state.
void
rd_wr_tie(uint8_t * buf, int32_t write)
{
    int32_t i;
    int32_t size;
    int32_t align;

    for (size = XCHAL_NCP_SA_SIZE, align = XCHAL_NCP_SA_ALIGN, i = -1; i < 8;) {
        // Align pointer as required.
        buf = (uint8_t *)(((int32_t)buf + align - 1) & -align);

        //printf("%s: %d %p (%d %d)\n", write ? "wr" : "rd", i, buf, size, align);

        if (write) {
            if (i < 0) xthal_save_extra(buf); else xthal_save_cpregs(buf, i);
        }
        else {
            if (i < 0) xthal_restore_extra(buf); else xthal_restore_cpregs(buf, i);
        }

#if defined (XT_WUR_FCR)
        // Reset FCR/FSR in case they got messed up by the restore.
        XT_WUR_FCR(0);
        XT_WUR_FSR(0);
#endif

        // Skip ahead and pick up next size/alignment if any.
        buf += size;
        i++;
        size  = Xthal_cpregs_size[i];
        align = Xthal_cpregs_align[i];
    }
}


// Thread function
int32_t
thread_func(void * arg, int32_t unused)
{
    int32_t   i;
    int32_t   j;
    int8_t    opt    = (int32_t) arg + 9;
    uint8_t * srcbuf = malloc(XCHAL_TOTAL_SA_SIZE + XCHAL_TOTAL_SA_ALIGN);
    uint8_t * dstbuf = malloc(XCHAL_TOTAL_SA_SIZE + XCHAL_TOTAL_SA_ALIGN);
    uint8_t * srcp;
    uint8_t * dstp;

    // Both src and dst buffers must be base-aligned to the required total alignment.
    srcp = (uint8_t *)(((uint32_t)srcbuf + XCHAL_TOTAL_SA_ALIGN) & ~(XCHAL_TOTAL_SA_ALIGN - 1));
    dstp = (uint8_t *)(((uint32_t)dstbuf + XCHAL_TOTAL_SA_ALIGN) & ~(XCHAL_TOTAL_SA_ALIGN - 1));

    // Init the src and dst buffers.
    memset(srcp, opt, XCHAL_TOTAL_SA_SIZE);
    memset(dstp, opt, XCHAL_TOTAL_SA_SIZE);

    // Read the src buffer into TIE state and then save it out.
    // This generates the reference pattern. Make sure we are
    // not preempted during this operation.
    xos_preemption_disable();
    rd_wr_tie(srcp, 0);
    rd_wr_tie(srcp, 1);
    xos_preemption_enable();

    for (i = 0; i < TEST_REPS; i++) {
        // Read ref pattern.
        rd_wr_tie(srcp, 0);
        // Save to check area.
        rd_wr_tie(dstp, 1);
        // Compare.
        for (j= 0; j < XCHAL_TOTAL_SA_SIZE; j++) {
            if (srcp[j] != dstp[j]) {
                fprintf(stderr, "TIE buf mismatch srcp %p dstp %p offset %d\n", srcp, dstp, j);
                exit(-1);
            }
        }
    }

    // Run the crunch test, but clear FPU state first.
    memset(srcbuf, 0, XCHAL_TOTAL_SA_SIZE);
    rd_wr_tie(srcbuf, 0);

    // crunch
    switch(opt)
    {
    case 9:  result[0] = crunch(THREAD_0_PARAMS); break;
    case 10: result[1] = crunch(THREAD_1_PARAMS); break;
    case 11: result[2] = crunch(THREAD_2_PARAMS); break;
    case 12: result[3] = crunch(THREAD_3_PARAMS); break;
    default: break;
    }

    return 0;
}


int32_t
coproc_test(void * arg, int32_t unused)
{
    int32_t ret;
    int32_t n;

    // Inform user test is starting.
    puts("\nStarting Co-Processor Context-Switch Test...");

    // Initialize the results.
    for (n = 0; n < NTHREADS; n++)
        result[n] = 0.0;

    // Compute the expected values before threading starts (no CP exception).
    expect[0] = crunch(THREAD_0_PARAMS);
    expect[1] = crunch(THREAD_1_PARAMS);
    expect[2] = crunch(THREAD_2_PARAMS); 
    expect[3] = crunch(THREAD_3_PARAMS);

    // Create the N - 1 test threads
    for (n = 0; n < (NTHREADS - 1); n++) {
        ret = xos_thread_create(&(test_threads[n]),
                                0,
                                thread_func,
                                (void *)n,
                                name[n],
                                stacks[n],
                                STACK_SIZE,
                                5,
                                0,
                                0);
        if (ret) {
            xos_fatal_error(-1, "Error creating test threads\n");
        }
    }

    // Create one last thread that does not have any coprocessor save area.
    ret = xos_thread_create(&test_threads[n],
                            0,
                            nocp_func,
                            0,
                            "no_cp",
                            stacks[n],
                            STACK_SIZE,
                            6,
                            0,
                            XOS_THREAD_NO_CP);
    if (ret) {
        xos_fatal_error(-1, "Error creating test threads\n");
    }

    for (n = 0; n < (NTHREADS - 1); n++) {
        xos_thread_join(&test_threads[n], &ret);
        if (ret != 0) {
            puts("Co-Processor Context-Switch Test... FAIL");
            exit(-1);
        }
    }

    ret = 0;
    for (n = 0; n < (NTHREADS - 1); n++) {
        if (result[n] != expect[n]) {
            printf("FAIL: thread %d FP bad result\n", n);
            ret++;
        }
    }
    if (ret)
        exit(-1);

    puts("Co-Processor Context-Switch Test... PASS");

#if !USE_MAIN_THREAD
    exit(0);
#endif
    return 0;
}


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
    xos_start_system_timer(-1, TICK_CYCLES/2);

    ret = coproc_test(0, 0);
    return ret;
#else
    ret = xos_thread_create(&coproc_test_tcb,
                            0,
                            coproc_test,
                            0,
                            "coproc_test",
                            coproc_test_stk,
                            STACK_SIZE,
                            7,
                            0,
                            0);

    xos_start_system_timer(-1, TICK_CYCLES/2);
    xos_start(0);
    return -1; // should never get here
#endif
}

