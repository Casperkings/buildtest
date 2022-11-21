
// xos_exc_handler.S - Exception dispatcher and dispatch table.

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


#include <xtensa/coreasm.h>

#include "xos_common.h"


//-----------------------------------------------------------------------------
//  Table of C exception handlers. Provides entries for all possible types of
//  exceptions. Note that some exceptions (e.g. coprocessor exceptions) will
//  never be handled through this table, because they are captured and handled
//  by the OS.
//  XOS starts up with the default exception handler installed for all the
//  table entries.
//-----------------------------------------------------------------------------

        .data
        .global xos_exc_handler_table
        .align  4
xos_exc_handler_table:
        .rept   XCHAL_EXCCAUSE_NUM
        .word   xos_default_exception_handler
        .endr


#if XCHAL_HAVE_XEA2

//-----------------------------------------------------------------------------
//  Exception handler wrapper - sets up for C handler dispatch for most of the
//  exceptions. Some exceptions are handled specially, because of the need for
//  speed and also because they cannot be taken over by user handlers.
//
//  At entry, we have this state:
//  -- a0 has been swapped with XHREG.
//  -- a1 has been decremented to allocate the exception frame.
//  -- a2 and a3 have been saved into the exception frame.
//  -- exception PC and PS have been saved into the exception frame.
//  -- a3 holds the exception cause.
//
//  Registers a4..a15 will ba saved and restored if we touch them here. They
//  will be spilled as needed once window exceptions get enabled.
//-----------------------------------------------------------------------------

        .text
        .align  4

#ifndef __XTENSA_CALL0_ABI__
.Lalloca:
        movi     a2,  _xos_alloca_handler       // handler address
        l32i     a3,  a1, FRAME_A3              // restore a3
        addi     a1,  a1, XOS_FRAME_SIZE        // restore a1
        xsr_xhreg(a0)                           // restore a0
        jx       a2                             // jump to alloca handler
#endif

        .align  4
        .global _xos_exc_dispatcher

_xos_exc_dispatcher:

        // First, check for some specific exceptions and handle them as special
        // cases. These are in some way impractical to handle in C.

#ifndef __XTENSA_CALL0_ABI__
        beqi     a3,  EXCCAUSE_ALLOCA, .Lalloca
#endif

        xsr_xhreg(a0)                           // restore a0 and xhreg
        s32i     a0,  a1, FRAME_A0              // save a0 and remaining ARs
        s32i     a4,  a1, FRAME_AR(4)
        s32i     a5,  a1, FRAME_AR(5)
        s32i     a6,  a1, FRAME_AR(6)
        s32i     a7,  a1, FRAME_AR(7)
        s32i     a8,  a1, FRAME_AR(8)
        s32i     a9,  a1, FRAME_AR(9)
        s32i    a10,  a1, FRAME_AR(10)
        s32i    a11,  a1, FRAME_AR(11)
        s32i    a12,  a1, FRAME_AR(12)
        s32i    a13,  a1, FRAME_AR(13)
        s32i    a14,  a1, FRAME_AR(14)
        s32i    a15,  a1, FRAME_AR(15)

        // From here on we can use a0, a2-a15

#if XCHAL_CP_NUM > 0

        // Check for and handle coprocessor exception. If not a coprocessor
        // exception then fall through to default handling code.

        blti     a3,  EXCCAUSE_CP0_DISABLED, .L1 // not CP exception
        addi     a2,  a3, -EXCCAUSE_CP0_DISABLED // a2 -- CP index
        bgei     a2,  XCHAL_CP_MAX, .L1          // not a CP exception

.L_xos_coproc_exc:

        rsync                                   // needed to pass on ISS
        call0   _xos_coproc_handler

.L_xos_coproc_done:
        // Restore saved registers (see above)
        l32i     a6,  a1, FRAME_AR(6)
        l32i     a7,  a1, FRAME_AR(7)
        l32i     a8,  a1, FRAME_AR(8)
        l32i     a9,  a1, FRAME_AR(9)
        l32i    a10,  a1, FRAME_AR(10)
        l32i    a11,  a1, FRAME_AR(11)
        l32i    a12,  a1, FRAME_AR(12)
        l32i    a13,  a1, FRAME_AR(13)
        l32i    a14,  a1, FRAME_AR(14)
        l32i    a15,  a1, FRAME_AR(15)
        bnez     a2,  .L1                       // not OK, go to default handler

        l32i     a4,  a1, FRAME_AR(4)           // restore remaining registers
        l32i     a5,  a1, FRAME_AR(5)
        l32i     a3,  a1, FRAME_A3
        l32i     a2,  a1, FRAME_A2
        l32i     a0,  a1, FRAME_PC              // set up return PC
        wsr.epc1 a0
        l32i     a0,  a1, FRAME_PS              // restore PS
        wsr.ps   a0
        rsync
        l32i     a0,  a1, FRAME_A0              // restore a0
        addi     a1,  a1, XOS_FRAME_SIZE        // restore SP
        rfe
.Lfail:
        ill

#endif // XCHAL_CP_NUM > 0

.L1:
        // Prepare for C handler dispatch.
        // Save exc cause and exc vaddr in exception frame for C handler
        rsr.excvaddr a2
        s32i     a3,  a1, FRAME_EXCCAUSE
        s32i     a2,  a1, FRAME_EXCVADDR

        // Save loop state and disable loops before clearing PS.EXCM
#if XCHAL_HAVE_LOOPS
        rsr.lbeg  a2
        rsr.lend  a3
        s32i    a2, a1, FRAME_LBEG
        s32i    a3, a1, FRAME_LEND
        movi    a2, 0
        xsr.lcount  a2
        s32i    a2, a1, FRAME_LCOUNT
#endif

        rsr.ps   a2
#ifdef __XTENSA_CALL0_ABI__
        movi     a3,  PS_UM | PS_INTLEVEL(XOS_MAX_OS_INTLEVEL)
#else
        movi     a3,  PS_WOE | PS_CALLINC(1) | PS_UM | PS_INTLEVEL(XOS_MAX_OS_INTLEVEL)  // CALL4 emulation
#endif
        extui    a2,  a2, PS_INTLEVEL_SHIFT, PS_INTLEVEL_BITS   // extract PS.INTLEVEL
        bltui    a2,  XOS_MAX_OS_INTLEVEL+1, .Lps   // at PS.INTLEVEL > XOS_MAX_OS_INTLEVEL ?
        addi     a3,  a3, -XOS_MAX_OS_INTLEVEL
        add      a3,  a3, a2
.Lps:
        wsr.ps   a3

        // Window overflows now enabled but not safe yet, so stick to a0..a3
        // until stack has been restored

#ifdef __XTENSA_CALL0_ABI__
        rsync                                   // wait for WSR to PS to complete
#else
        l32i     a2,  a1, FRAME_PC              // "return address"
        addi     a1,  a1, XOS_FRAME_SIZE        // restore sp (dealloc ESF)
        rsync                                   // wait for WSR to PS to complete

        // Make it look as if the code that got the exception made a CALL4
        // to the exception handling code.  (We call this the "pseudo-CALL".)
        // This "call" updates the stack pointer correctly to that window
        // exceptions can work properly. We'll also set the return PC to get
        // better stack traces.
        // IMPORTANT: Interrupts must stay disabled while doing the pseudo-CALL,
        // or at least until after the ENTRY instruction, because SP has been
        // restored to its original value that does not reflect the exception
        // stack frame's allocation. An interrupt taken here would corrupt the
        // exception stack frame (ie. allocate another over it).
        // High priority interrupts can remain enabled, they save and restore
        // all of their state and use their own stack or save area.

        movi     a4,  0xC0000000        // [for debug] for return PC computation below
        or       a2,  a4, a2            // [for debug] set upper two bits of return PC
        addx2    a4,  a4, a2            // [for debug] clear upper bit

        // This rotates the register window forward by 4, updates the stack ptr,
        // and the _ prefix prevents the assembler from aligning the instruction
        // and possibly causing a gap in the instruction stream.

        _entry   a1,  XOS_FRAME_SIZE    // as if after a CALL4 (PS.CALLINC set to 1 above)
#endif // __XTENSA_CALL0_ABI__

#if XCHAL_HAVE_INTERRUPTS
        // Restore PS.INTLEVEL to its saved value (re-enables interrupts
        // if they were enabled before taking the exception)

        l32i    a13,  a1, FRAME_PS
        rsr.ps  a14
        extui   a13, a13, PS_INTLEVEL_SHIFT, PS_INTLEVEL_BITS  // extract saved PS.INTLEVEL
        extui   a15, a14, PS_INTLEVEL_SHIFT, PS_INTLEVEL_BITS  // extract current PS.INTLEVEL
        xor     a14, a14, a15           // clear a14.INTLEVEL
        xor     a14, a14, a13           // replace with saved PS.INTLEVEL
        wsr.ps  a14                     // restore PS.INTLEVEL
#endif

        // Save remaining state

#if XCHAL_HAVE_MAC16
        rsr.acchi  a13
        rsr.acclo  a14
        s16i    a13, a1, FRAME_ACCHI
        s32i    a14, a1, FRAME_ACCLO
#endif
        rsr.sar a13
        s8i     a13, a1, FRAME_SAR

        // Now we have everything saved away and set up for C calls

        movi    a13,  xos_exc_handler_table
        l32i    a15,  a1, FRAME_EXCCAUSE
        addx4   a12, a15, a13           // a12 = table[exccause]
        l32i    a12, a12, 0             // handler address

        .global _GeneralException
_GeneralException:                      // label for stack trace

#ifdef __XTENSA_CALL0_ABI__
        beqz    a12,  .Lskip            // if null handler skip call
        mov      a2,  a1                // arg1: exception parameters
        callx0  a12                     // call C exception handler for this exception
#else
        beqz    a12,  .Lskip            // if null handler skip call
        mov      a6,  a1                // arg1: exception parameters
        callx4  a12                     // call C exception handler for this exception
#endif

.Lskip:
        // Restore state and return. Note that we don't restore LCOUNT here
        // because PS.EXCM is not set, so unintended loopback might occur if
        // the count becomes nonzero.

#if XCHAL_HAVE_LOOPS
        l32i    a13,  a1, FRAME_LBEG
        l32i    a14,  a1, FRAME_LEND
        wsr.lbeg    a13
        wsr.lend    a14
#endif
#if XCHAL_HAVE_MAC16
        l16si   a13,  a1, FRAME_ACCHI
        l32i    a14,  a1, FRAME_ACCLO
        wsr.acchi  a13
        wsr.acclo  a14
#endif
        l8ui    a13,  a1, FRAME_SAR
        wsr.sar a13

        // Disable interrupts while returning from the pseudo-CALL setup above.
        // We are going to be messing with the stack pointer below, so we don't
        // want anything to come along and write over the exception frame area
        // while the stack pointer is changing. The information in the frame is
        // needed to return from the exception.

#if XCHAL_HAVE_INTERRUPTS
        rsil    a12,  XOS_MAX_OS_INTLEVEL
#endif

#ifndef __XTENSA_CALL0_ABI__
        // Reverse the "call4" by doing a dummy return. Note that this will
        // roll back the stack pointer so we need to fix that up.

        movi     a0,  .Lret4            // construct return address
        movi    a13,  0xC0000000
        or       a0,  a0, a13           // set upper two bits
        addx2    a0, a13, a0            // clear upper bit
        retw

        .align  4
.Lret4:
        addi     a1,  a1, -XOS_FRAME_SIZE
#endif

        l32i     a4,  a1, FRAME_AR(4)   // restore a4-a15
        l32i     a5,  a1, FRAME_AR(5)
        l32i     a6,  a1, FRAME_AR(6)
        l32i     a7,  a1, FRAME_AR(7)
        l32i     a8,  a1, FRAME_AR(8)
        l32i     a9,  a1, FRAME_AR(9)
        l32i    a10,  a1, FRAME_AR(10)
        l32i    a11,  a1, FRAME_AR(11)
        l32i    a12,  a1, FRAME_AR(12)
        l32i    a13,  a1, FRAME_AR(13)
        l32i    a14,  a1, FRAME_AR(14)
        l32i    a15,  a1, FRAME_AR(15)

        // Now we are left with a0-a3, PS and PC. Interrupts are disabled
        // so we can finish everything up.

        l32i     a2,  a1, FRAME_A2      // restore a2-a3
        l32i     a3,  a1, FRAME_A3
        l32i     a0,  a1, FRAME_PC      // set up return PC
        wsr.epc1 a0
        l32i     a0,  a1, FRAME_PS      // restore PS
        wsr.ps   a0
        rsync
#if XCHAL_HAVE_LOOPS
        // Restore LCOUNT after PS.EXCM is set, to avoid unintended loopbacks.
        l32i     a0,  a1, FRAME_LCOUNT
        wsr.lcount a0
#endif
        l32i     a0,  a1, FRAME_A0      // restore a0
        addi     a1,  a1, XOS_FRAME_SIZE // restore SP
        rfe

        .size   _xos_exc_dispatcher, . - _xos_exc_dispatcher

//-----------------------------------------------------------------------------
// Symbols for the debugger to use in identifying interrupt / exception frames.
//-----------------------------------------------------------------------------

        // Size of exc frame
        .global _GeneralExceptionFrameSize
        .set    _GeneralExceptionFrameSize, XOS_FRAME_SIZE

        // Register save offset in exc frame
        .global _GeneralExceptionRegisterSaveOffset
        .set    _GeneralExceptionRegisterSaveOffset, 92

        // EPC offset in exc frame
        .global _GeneralExceptionEPCSaveOffset
        .set    _GeneralExceptionEPCSaveOffset, 88

#endif // XCHAL_HAVE_XEA2

#if XCHAL_HAVE_XEA3

#if XCHAL_CP_NUM > 0

//-----------------------------------------------------------------------------
//  For XEA3, coprocessor exceptions come here. This is a wrapper function that
//  calls _xt_coproc_handler() to do the actual work. We don't want the handler
//  to be interrupted because that might cause a round-robin switch and leave
//  coprocessor context in a confused state. So interrupts are disabled before
//  calling the handler. They will be re-enabled on return from exception.
//-----------------------------------------------------------------------------

        .text
        .align  4
        .global xos_coproc_exc_handler

xos_coproc_exc_handler:
#ifdef __XTENSA_CALL0_ABI__
        addi    a1,  a1, -16                    // reserve 16 bytes on stack
        s32i    a0,  a1, 0                      // save return address
        s32i    a2,  a1, 4                      // save a2
        s32i   a15,  a1, 8                      // must save a15 (see dispatch)
        movi    a3,  PS_DI_MASK
        xps     a3,  a3                         // set PS.DI, disable interrupts
        l32i    a3,  a2, XT_STK_EXCCAUSE        // a3 <- exccause
        extui   a2,  a3, EXCCAUSE_SUBTYPE_SHIFT, EXCCAUSE_SUBTYPE_BITS  // a2 <- CP index
        call0   _xos_coproc_handler
        beqz    a2,  1f                         // a2 = 0, OK
        l32i    a2,  a1, 4
        call0   xos_default_exception_handler   // not OK, call default handler
1:
        l32i    a0,  a1, 0                      // restore return address
        l32i   a15,  a1, 8                      // restore a15
        addi    a1,  a1, 16
        ret
#else
        entry   a1,  48                         // reserve 16 bytes on stack
        s32i    a0,  a1, 0                      // save return address
        s32i    a2,  a1, 4                      // save a2
        movi    a3,  PS_DI_MASK
        xps     a3,  a3                         // set PS.DI, disable interrupts
        l32i    a3,  a2, XT_STK_EXCCAUSE        // a3 <- exccause
        extui   a2,  a3, EXCCAUSE_SUBTYPE_SHIFT, EXCCAUSE_SUBTYPE_BITS  // a2 <- CP index
        call0   _xos_coproc_handler
        beqz    a2,  1f                         // a2 = 0, OK
        l32i    a2,  a1, 4
        call8   xos_default_exception_handler   // not OK, call default handler
1:
        l32i    a0,  a1, 0                      // restore return address
        retw
#endif

        .size xos_coproc_exc_handler, . - xos_coproc_exc_handler

#endif // XCHAL_CP_NUM

#endif // XCHAL_HAVE_XEA3


//-----------------------------------------------------------------------------
// Define the symbol __os_flag to be nonzero, so that _sbrk_r() from the C lib
// does not try any stack checking. This stack checking will fail because the
// thread stacks are not where it expects them to be.
//-----------------------------------------------------------------------------

        .global __os_flag
        .equ    __os_flag, 1

//-----------------------------------------------------------------------------
//  These are defined for the sole purpose of generating link errors if XTOS
//  interrupt/exception functions get called by the application.
//  Such XTOS calls cannot be mixed with the use of XOS.
//-----------------------------------------------------------------------------

        .global _xtos_interrupt_table
        .equ _xtos_interrupt_table, 0
        .global _xtos_exc_handler_table
        .equ _xtos_exc_handler_table, 0
        .global xtos_interrupt_table
        .equ xtos_interrupt_table, 0
        .global xtos_exc_handler_table
        .equ xtos_exc_handler_table, 0

