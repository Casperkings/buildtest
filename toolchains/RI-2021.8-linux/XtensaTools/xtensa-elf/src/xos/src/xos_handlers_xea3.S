
// xos_handlers_xea3.S -- XEA3 system assembly routines.

// Copyright (c) 2003-2020 Cadence Design Systems, Inc.
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


#include <xtensa/coreasm.h>
#include <xtensa/corebits.h>
#include <xtensa/config/core.h>

#include "xos_common.h"


//-----------------------------------------------------------------------------
// xos_start_thread
//
// Start a new thread, don't attempt to save any context for caller.
// Callable from C (by OS). Does not return to caller.
//
// a2 -- pointer to incoming thread's TCB.
//-----------------------------------------------------------------------------

        .text
        .align  4
        .global xos_start_thread

xos_start_thread:
#ifdef __XTENSA_WINDOWED_ABI__
        entry   a1, 32
#endif
        movi    a3, PS_DI                       // disable interrupts
        xps     a3, a3
        movi    a3, xos_globals                 // a3 = pointer to global data
        l32i    a4, a2, TCB_RESUME_FN           // a4 = thread resume code
        s32i    a2, a3, XOS_NEXT_THREADPTR
        s32i    a2, a3, XOS_CURR_THREADPTR      // indicate switch to new thread
        jx      a4                              // jump to thread resume code

//-----------------------------------------------------------------------------
// xos_resume_by_restart
//
// Resume a thread by calling its entry point, mostly to start a new thread.
// 
// ASSUMES: XOS_CURR_THREADPTR already set, interrupts disabled via PS.DI.
// NO assumptions made about the contents of any AR register.
//-----------------------------------------------------------------------------

        .align  4
        .global xos_resume_by_restart

xos_resume_by_restart:
#ifdef __XTENSA_WINDOWED_ABI__
        movi    a0, 0x40000000                  // reset WB register so that we
        wsr.wb  a0                              // begin from first register window
        rsync
#endif

        // Register window may have moved so must load any values we need.

        movi    a3, xos_globals
        movi    a0, 0                           // also terminates stack backtrace
        l32i    a2, a3, XOS_CURR_THREADPTR      // a2 = current thread TCB
        s32i    a0, a2, TCB_RESUME_FN           // signal preemptive switch

#if XOS_OPT_STATS
        l32i    a4, a2, TCB_NORMAL_RESUMES      // increment # of normal resumes
#if XCHAL_HAVE_CCOUNT
        XOS_GET_CCOUNT(a5)                      // store current ccount
        s32i    a5, a2, TCB_RESUME_CCOUNT
#endif
        addi    a4, a4, 1
        s32i    a4, a2, TCB_NORMAL_RESUMES
#endif

#if XOS_OPT_THREAD_SAFE_CLIB
        l32i    a4, a2, TCB_CLIB_PTR            // a4 = thread's Clib ptr
        movi    a5, GLOBAL_CLIB_PTR             // a5 = global Clib ptr
#endif

#if XCHAL_CP_NUM > 0
        wsr.cpenable a0                         // disable all coprocessors to start
#endif
#if XCHAL_HAVE_LOOPS
        wsr.lcount a0                           // clear loop state just-in-case
#endif

#if XOS_OPT_THREAD_SAFE_CLIB
        s32i    a4, a5, 0                       // Global Clib ptr = thread's Clib context (see above)
#endif

#if XOS_OPT_STACK_CHECK && XCHAL_HAVE_KSL
        // Set up thread's stack limit 128 bytes above the stack base
        l32i    a4, a2, TCB_STACK_BASE          // base is at least 16 bytes aligned
        movi    a5, 128
        add     a4, a4, a5
        wsr.ksl a4
#endif

        l32i    a1, a2, TCB_STACK_END           // set up thread stack
        movi    a4, PS_STACK_FIRSTKER           // initial PS value
        wsr.ps  a4                              // this enables interrupts
        rsync

        l8ui    a4, a2, TCB_EXIT_FLAG           // a4 = thread exit flag
        bnez    a4, xos_thread_abort_stub       // abort if exit flag set

        l32i    a4, a2, TCB_STARTUP_ENTRY       // a4 = thread entry function ptr
        l32i    a5, a2, TCB_FLAGS               // a5 = thread flags
#ifdef __XTENSA_CALL0_ABI__
        l32i    a3, a2, TCB_RETVALUE            // pass wake value as 2nd arg (currently unused)
        l32i    a2, a2, TCB_STARTUP_ARG         // load arg to startup function
#if XOS_OPT_UM
        bbci.l  a5, 14, 1f                      // user-mode if set
        call0   .Lum                            // set up for user-mode
#endif
1:
        callx0  a4                              // call thread entry function
        j       xos_thread_exit_trap
#else
        l32i    a11, a2, TCB_RETVALUE           // pass wake value as 2nd arg (currently unused)
        l32i    a10, a2, TCB_STARTUP_ARG        // load arg to startup function
#if XOS_OPT_UM
        bbci    a5, 14,  2f                     // user-mode if set
        call8   .Lum                            // set up for user-mode
#endif
2:
        callx8  a4                              // call thread entry function
        j       xos_thread_exit_trap
#endif

#if XOS_OPT_UM
        .align  4
        .local  .Lum

        // Set up stack frame and jump to dispatch code. The exit/restore
        // path in the dispatch code will restore thread state and branch
        // to the thread entry function (which is in user-mode code).
        // The PS, when restored, will set RING=1 and force the core into
        // user-mode.
.Lum:
        addi    a1, a1, -XOS_FRAME_SIZE         // Construct exception frame
        s32i    a4, a1, XT_STK_PC               // PC value to restore
        rsr.ps  a5
        addi    a5, a5, (1 << PS_RING_SHIFT)
        s32i    a5, a1, XT_STK_PS               // PS value to restore
        movi    a5, xos_thread_exit_trap
#ifdef __XTENSA_CALL0_ABI__
        s32i    a5, a1, XT_STK_A0
        s32i    a2, a1, XT_STK_A2
        s32i    a3, a1, XT_STK_A3
#else
        s32i    a5, a1, XT_STK_A8
        s32i    a10, a1, XT_STK_A10
        s32i    a11, a1, XT_STK_A11
#endif
        movi    a5, PS_DI
        xps     a5, a5                          // disable interrupts
        addi    a1, a1, XOS_FRAME_SIZE          // restore SP
        mov     a15, a1                         // dispatch code expects this
        movi    a5, _xt_dispatch + 3
        jx      a5
#endif

//-----------------------------------------------------------------------------
// If we get here, thread function has returned. Go to exit handling.
//-----------------------------------------------------------------------------

        .global xos_thread_exit_trap

xos_thread_exit_trap:
#ifdef __XTENSA_CALL0_ABI__
        call0   xos_thread_exit                 // exit thread (exit code in a2)
#else
        call8   xos_thread_exit                 // exit thread (exit code in a10)
#endif

#if XCHAL_HAVE_DEBUG
        break   1, 15                           // if we get here, it's fatal
#endif
        j       xos_thread_exit_trap

//-----------------------------------------------------------------------------
// xos_thread_abort_stub
//
// Execution comes here if a thread is aborted. This stub sets the exit value
// as the return code and then starts exit processing.
//-----------------------------------------------------------------------------

        .align  4
        .global xos_thread_abort_stub

xos_thread_abort_stub:
        movi    a2, xos_globals
        l32i    a3, a2, XOS_CURR_THREADPTR      // a3 = ptr to current thread
#ifdef __XTENSA_CALL0_ABI__
        l32i    a2, a3, TCB_RETVALUE            // a2 = thread->wake_value
#else
        l32i    a10, a3, TCB_RETVALUE           // a10 = thread->wake_value
#endif
        j       xos_thread_exit_trap

//-----------------------------------------------------------------------------
// xos_resume_idle_thread
//
// Resumes the idle 'thread'. We want to go into the XOS idle loop, but since
// that requires a change in the dispatch state, we'll jump into the dispatch
// code exit path, and let the exit path detect the idle state and set things
// up the right way.
//
// a2 - Pointer to incoming thread TCB.
// NOTE: Interrupts will be re-enabled in idle loop.
//-----------------------------------------------------------------------------

        .align  4
        .global xos_resume_idle_thread

xos_resume_idle_thread:
        movi    a3, xos_globals
        movi    a4, PS_STACK_KERNEL | PS_DI     // set PS.STACK = Kernel and
        movi    a5, PS_STACK_MASK | PS_DI_MASK  // disable interrupts.
        xps     a4, a5

#if XOS_OPT_STATS && XCHAL_HAVE_CCOUNT
        // Start counting cycles for interrupt. This is needed because we jump
        // to dispatch code below, and the exit path will stop counting cycles
        // for interrupt. If we don't start here, a bad count will result. The
        // cycle count attributed to interrupts ends up being slightly higher
        // than actual (but the counts are somewhat approximate anyway).

        movi    a3, xos_inth_start_ccount
        XOS_GET_CCOUNT(a4)
        s32i    a4, a3, 0
#endif

        movi    a0, _xt_dispatch + 3            // dispatch entry point
#ifdef __XTENSA_CALL0_ABI__
        mov     a15, a1                         // Dispatch code expects a15 = old a1
        movi    a2, 0                           // zero a2 for l32dis
#else
        movi    a10, 0                          // zero a10 for l32dis
#endif
        ret                                     // go to dispatch code

//-----------------------------------------------------------------------------
// xos_resume_cooperative_thread
//
// Resumes a thread that previously yielded. Since very little state needs to
// be restored for such a thread, we handle it here instead of in the dispatch
// handling return path.
//
// a2 - Pointer to incoming thread TCB.
// a3 - CCOUNT value to use as resume value.
// ASSUMES: XOS_CURR_THREADPTR already set, interrupts disabled via PS.DI.
// Interrupts may get enabled when PS is restored.
//-----------------------------------------------------------------------------

        .align  4
        .global xos_resume_cooperative_thread

xos_resume_cooperative_thread:

#if XOS_OPT_STATS
        l32i    a4, a2, TCB_NORMAL_RESUMES      // load resume count
#if XCHAL_HAVE_CCOUNT
        s32i    a3, a2, TCB_RESUME_CCOUNT       // start counting cycles for thread
#endif
        addi    a4, a4, 1
        s32i    a4, a2, TCB_NORMAL_RESUMES      // increment coop resume count
#endif

#if XOS_OPT_STACK_CHECK && XCHAL_HAVE_KSL
        l32i    a4, a2, TCB_STACK_BASE          // stack base at least 16-byte aligned
        movi    a5, 128                         // set thread's stack limit 128 bytes
        add     a4, a4, a5                      // above the stack base
        wsr.ksl a4
#endif

#if XOS_OPT_THREAD_SAFE_CLIB
        l32i    a4, a2, TCB_CLIB_PTR            // thread's C lib context pointer
        movi    a5, GLOBAL_CLIB_PTR             // global C lib context pointer
        s32i    a4, a5, 0                       // point global ptr to active thread
#endif

        l32i    a1, a2, TCB_STACK_ESF           // load stack ptr from TCB
        movi    a4, 0
        s32i    a4, a2, TCB_RESUME_FN           // indicate preemptive switch
        l32i    a0, a1, XT_STK_PC               // a0 <- return address
        l32i    a4, a1, XT_STK_PS               // a4 <- PS
#ifdef __XTENSA_CALL0_ABI__
        l32i    a12, a1, XT_STK_A12             // restore callee-saved registers
        l32i    a13, a1, XT_STK_A13
        l32i    a14, a1, XT_STK_A14
        l32i    a15, a1, XT_STK_A15
#endif

        addi    a1, a1, XOS_FRAME_SIZE          // deallocate stack frame
        wsr.ps  a4                              // this can enable interrupts
        l32i    a2, a2, TCB_RETVALUE            // a2 <- return value

#ifdef __XTENSA_CALL0_ABI__
        ret
#else
        retw
#endif

//-----------------------------------------------------------------------------
// xos_switch_thread
//
// Solicited switch to the specified thread from a normal thread. The incoming
// thread ptr must be non-zero and different from xos_curr_threadptr.
// Callable from C (by OS).
//
// a2 - Pointer to incoming thread TCB.
//-----------------------------------------------------------------------------

        .align  4
        .global xos_switch_thread

xos_switch_thread:
#ifdef __XTENSA_CALL0_ABI__
        // No need for any stack space for this function
#else   
        entry   a1, 32
        ssai    0
        spillw                                  // spill all registers
#endif  

        // Save register state into exception frame. This is safe to do with
        // interrupts enabled, but we will have to revert SP to point above
        // the exception frame because that is what the dispatch code expects.
        // Must disable interrupts before that.

        rsr.ps  a4
        addi.a  a1, a1, -XOS_FRAME_SIZE
        s32i    a0, a1, XT_STK_PC               // save return address
        s32i    a4, a1, XT_STK_PS               // save PS
#ifdef __XTENSA_CALL0_ABI__
        s32i    a12, a1, XT_STK_A12             // callee-saved registers
        s32i    a13, a1, XT_STK_A13
        s32i    a14, a1, XT_STK_A14
        s32i    a15, a1, XT_STK_A15
#endif

        // Need interrupts disabled for everything that follows.

        movi    a3, xos_globals
        movi    a4, PS_STACK_KERNEL | PS_DI     // set PS.STACK = Kernel and
        movi    a5, PS_STACK_MASK | PS_DI_MASK  // disable interrupts.
        xps     a4, a5

        movi    a5, xos_resume_cooperative_thread
        l32i    a4, a3, XOS_CURR_THREADPTR      // a4 <- current thread pointer
        s32i    a2, a3, XOS_NEXT_THREADPTR      // next thread pointer = a2
        s32i    a5, a4, TCB_RESUME_FN           // mark as coop switch
        s32i    a1, a4, TCB_STACK_ESF           // save frame ptr in TCB

#if XCHAL_CP_NUM > 0
        l32i    a6, a4, TCB_TIE_SAVE            // a6 <- ptr to TIE save area
        rsr.cpenable a5
        beqz    a5, .L1
        call0   _xos_coproc_savecs              // save callee-saved CP state
                                                // **may trash a7-a15**
        movi    a5, 0
        wsr.cpenable a5                         // clear CPENABLE, give up all CPs
.L1:
        beqz    a6, .L0
        s16i    a5, a6, XT_CPENABLE
.L0:
#endif

        l32i    a5, a2, TCB_RESUME_FN           // a5 <- incoming thread's resume fn
        beqz    a5, .L2                         // if none, go through dispatch code

#if XOS_OPT_STATS && XCHAL_HAVE_CCOUNT
        update_cycle_count a4 a6 a7 a0          // update cycle count for outgoing thread
                                                // NOTE: a4 must hold current TCB pointer
#endif
        s32i    a2, a3, XOS_CURR_THREADPTR      // set current thread ptr
        mov     a3, a0                          // a3 <- CCOUNT value from update above
        jx      a5                              // NOTE: a2 = incoming thread ptr

.L2:
        addi    a1, a1, XOS_FRAME_SIZE          // back to original SP
        movi    a0, _xt_dispatch + 3            // dispatch code address

        // Call the interrupt entry macro to fake an entry sequence.
        // This keeps the dispatch code happy and avoids duplicate code.
        // NOTE: current thread ptr must be valid up to this point.
        // This macro uses a10, a12-a14.

        XT_RTOS_INT_ENTER

        movi    a4, 0
        s32i    a4, a3, XOS_CURR_THREADPTR      // clear current thread ptr
                                                // to skip further saves

#ifdef __XTENSA_CALL0_ABI__
        mov     a15, a1                         // Dispatch code expects a15 = old a1
#endif
        ret                                     // 'return' to dispatch code

//-----------------------------------------------------------------------------
// xos_idle
//
// XOS idle loop. No threads to run, so wait here until an interrupt occurs.
// There is no state to be saved on interrupt so we would like to take a
// shortcut to the tailchain code on interrupt, unfortunately that does not
// happen.
//-----------------------------------------------------------------------------

        .align  4
        .global xos_idle

xos_idle:
        movi    a0, 0                           // terminate backtrace

#if XCHAL_HAVE_KSL
        wsr.ksl a0                              // disable stack check
#endif

#if XOS_OPT_STATS && XCHAL_HAVE_CCOUNT
        movi    a2, xos_globals
        l32i    a2, a2, XOS_CURR_THREADPTR      // a2 = ptr to idle thread TCB
        XOS_GET_CCOUNT(a3)
        s32i    a3, a2, TCB_RESUME_CCOUNT       // resume thread count
#endif

        movi    a14, xos_int_stack_end
        movi    a15, _xt_intr_from_idle
        mov     a1, a14                         // a1 <- Top of interrupt stack
        movi    a14, 0                          // 0 = Normal
        wsr.ms  a14                             // Set DISPST = Normal
        rsync
        waiti   0                               // Wait for interrupt
        memw                                    // HW erratum 569

        .type   xos_idle,@function
        .size   xos_idle, . - xos_idle

//-----------------------------------------------------------------------------
// xos_sched_handler
//
// Scheduler interrupt handler. Triggered by context switch. At this time only
// useful for windowed ABI to spill register windows.
//-----------------------------------------------------------------------------

#if XOS_USE_SCHED_INT

        .align  4
        .global xos_sched_handler

xos_sched_handler:
#ifdef __XTENSA_WINDOWED_ABI__
        entry   a1, 32
        ssai    1
        spillw
        retw
#else
        ret
#endif

        .type   xos_sched_handler,@function
        .size   xos_sched_handler, . - xos_sched_handler

#endif

//-----------------------------------------------------------------------------
// xos_debug_exc_handler
//
// Debug exception handler. Mainly to catch stack overflow exceptions.
// This handler will first try to trap to the simulator, if present. If not
// running on the simulator, it will transfer control to the system default
// handler after disabling the kernel stack limit check. This ensures that
// the default handler will not take another stack exception, although it
// may end up corrupting memory below the thread stack.
//-----------------------------------------------------------------------------

        .align  4
        .global xos_debug_exc_handler

xos_debug_exc_handler:
        wsr.exccause a3                         // Restore ExcCause for ISS
        mov     a4, a2                          // Save a2
        movi    a2, -10                         // ISS trap code (user exc)
        simcall                                 // Trap to ISS if present
        mov     a2, a4                          // Restore exc frame pointer

        rsr.ps  a4
        extui   a4, a4, 5, 3                    // a4 <- PS.STK
        bnei    a4, 3, 1f                       // PS.STK == 3, thread stack
#if XCHAL_HAVE_KSL
        movi    a4, 0
        wsr.ksl a4                              // Disable KSL checking
#endif
1:
        movi    a5, xos_default_exception_handler
        jx      a5                              // Go to default exc handler

        .type   xos_debug_exc_handler,@function
        .size   xos_debug_exc_handler, . - xos_debug_exc_handler

