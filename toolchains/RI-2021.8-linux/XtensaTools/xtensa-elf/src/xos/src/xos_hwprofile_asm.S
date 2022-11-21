
// xos_hwprofile_asm.S - interface to Xtensa hw profiling library.

// Copyright (c) 2015-2020 Cadence Design Systems, Inc.
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

//-----------------------------------------------------------------------------
// The functions in this file implement the interface to the hw based profiling
// library. The library is automatically linked in when the xt-xcc option -hwpg
// is specified. See user guides for more details.
//-----------------------------------------------------------------------------


#include <xtensa/config/core.h>
#include <xtensa/coreasm.h>


#if XCHAL_HAVE_INTERRUPTS

//-----------------------------------------------------------------------------
// Data.
//-----------------------------------------------------------------------------

        .data
        .align  4
        .global hwp_cb

hwp_cb:                                 // Set by xt_profile_set_timer_handler()
        .word   0
save_a0:                                // Save area for high-priority wrapper
        .word   0


//-----------------------------------------------------------------------------
// XOS wrapper to dispatch high-priority timer interrupt to hw profiler library.
// The library routine requires return address in a0, and a2 available for use.
// We expect to get here from the XOS high-priority dispatch routine, which has
// saved a2 for us. a0 holds the return address and must be preserved.
//-----------------------------------------------------------------------------

        .text
        .align  4
        .global xos_hwprof_int_wrapper

xos_hwprof_int_wrapper:
        movi    a2, save_a0
        s32i    a0, a2, 0               // save a0
        movi    a2, hwp_cb
        l32i    a2, a2, 0               // a2 <- callback function address
        movi    a0, .Lret               // a0 <- where to return
        jx      a2
.Lret:
        movi    a2, save_a0
        l32i    a0, a2, 0               // restore a0
        ret

        .size   xos_hwprof_int_wrapper, . - xos_hwprof_int_wrapper


//-----------------------------------------------------------------------------
// XOS wrapper to dispatch low/med priority timer interrupt to hw profiler.
// This wrapper needs to be C-callable, but the profiler routine is not.
// In XEA3 configs this is used all the time since any interrupt can be sent
// to a C handler.
//
// For XEA3, we need to pass the EPC (the location that was interrupted)
// in a2, since the profiler handler cannot read EPCn like it used to.
// The EPC is saved in the interrupt frame which is on the thread stack, but
// this handler is running on the interrupt stack. We will recover the stack
// pointer of the thread by (a) reading it off the previous register window's
// a1 if windowed ABI, or (b) reading a15 if call0 ABI. Once we have the pointer,
// EPC will be 80 bytes below it (the SP points above the interrupt frame).
// NOTE also that this function is called with its argument in a2 so that
// has to be saved.
//-----------------------------------------------------------------------------

        .text
        .align  4
        .global xos_hwprof_cint_wrapper

xos_hwprof_cint_wrapper:
#ifdef __XTENSA_CALL0_ABI__
        addi    a1, a1, -16             // make room on stack
        mov     a3, a2                  // a3 <- profiler address
#if XCHAL_HAVE_XEA3
        addi    a2, a15, -80            // a2 <- user SP - 80
        l32i    a2, a2, 0               // a2 <- EPC
#endif
#else
        entry   a1, 48
        mov     a3, a2                  // a3 <- profiler address
#if XCHAL_HAVE_XEA3
        movi    a2, PS_DI
        xps     a2, a2                  // disable interrupts
        addi    a4, a1, -16
        movsp   a1, a4                  // force 'caller' regs to be present
        rotw    -1                      // rotate back to 'caller' window
        addi    a13, a1, -80            // a13 (a5) <- user SP - 80
        rotw    1                       // undo rotation, a13 now a5
        movi    a6, PS_DI
        xps     a2, a6                  // restore interrupt state
        l32i    a2, a5, 0               // a2 <- EPC
#endif
#endif

        s32i    a0, a1, 0               // save return address
        movi    a0, .Lret1              // a0 <- where to return
        jx      a3
.Lret1:
        l32i    a0, a1, 0               // restore a0
#ifdef __XTENSA_CALL0_ABI__
        addi    a1, a1, 16
        ret
#else
        retw
#endif

        .size   xos_hwprof_cint_wrapper, . - xos_hwprof_cint_wrapper

#endif

