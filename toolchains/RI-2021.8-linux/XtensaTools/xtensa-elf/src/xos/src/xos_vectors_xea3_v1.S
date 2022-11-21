// reset-vector-xea3.S  --  Xtensa Reset Vector / Dispatch Code for XEA3

// Copyright (c) 1999-2020 Cadence Design Systems Inc.
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


#include <xtensa/config/core.h>
#include <xtensa/coreasm.h>

#include "xos_common.h"


// Uncomment this to disable interrupts during exception dispatch.
//#define XOS_DISABLE_INT_ON_EXC  1


//-----------------------------------------------------------------------------
// The entry point vectors are common for call0 and windowed configurations.
//-----------------------------------------------------------------------------

        .section .DispatchVector.text, "ax"
#if XCHAL_HAVE_VECBASE
        .align  64                      // 64-byte alignment needed when vecbase
#else                                   // is relocatable
        .align  4
#endif

        .org    0                       // Fixed offset for Reset Vector
        .global _DispatchVector
        .weak   _DispatchVector

_DispatchVector:
        j       _JumpToResetHandler

        .org    3                       // Reserved
        .local  _Reserved1

_Reserved1:
        ill

        .org    6                       // Fixed offset for Double Exception Vector
        .global _DoubleExceptionVector
        .weak   _DoubleExceptionVector

_DoubleExceptionVector:
        j       _DoubleExceptionHandler

        .org    9                       // Reserved
        .local  _Reserved2

_Reserved2:
        ill

//-----------------------------------------------------------------------------
// Start of dispatch code.
//-----------------------------------------------------------------------------

        .org    12                      // Fixed offset for Tailchain entry point
        .global _xt_dispatch
        .global _xt_dispatch_plus_3

_xt_dispatch:
#ifdef __XTENSA_CALL0_ABI__

        // NOTE: for call0, a15 is expected to be holding the previous stack pointer
        // when we get to the Tailchain segment.
#if XOS_USE_INT_WRAPPER
        l32dis   a2, a15, 2             // EPC <- wrapper address, a2 <- (intnum << 2)
#else
        l32dis   a2, a15, 1             // EPC <- handler_table[intnum].addr, a2 < (intnum << 2)
#endif

_xt_dispatch_plus_3:
#if !XOS_USE_INT_WRAPPER
        l32dis   a2, a15, 3             // a2 <- handler_table[intnum].arg
#endif
        s32stk   a4, a15, 2             // a1 <- Interrupt Stack Base (ISB) if interrupt
                                        // else a1 <- (a15-96) (a15 = old a1)
        s32dis   a0, a15, 0             // Jump to handler if interrupt else fall through
                                        // Note this also clears local exclusive monitor

#else // windowed

#if XOS_USE_INT_WRAPPER
        l32dis  a10,  a1, 2             // EPC <- wrapper address, a10 <- (intnum << 2)
#else
        l32dis  a10,  a1, 1             // EPC <- handler_table[intnum].addr, a10 < (intnum << 2)
#endif

_xt_dispatch_plus_3:
#if !XOS_USE_INT_WRAPPER
        l32dis  a10,  a1, 3             // a10 <- handler_table[intnum].arg
#endif
        s32stk  a12,  a1, 2             // a9 <- Interrupt Stack Base (ISB) if interrupt
                                        // else a9 <- (a1-96)
        s32dis   a8,  a1, 0             // Jump to handler if interrupt else fall through
                                        // Note this also clears local exclusive monitor

#endif

.Lexit:
        j       _xt_exit

#ifndef __XTENSA_CALL0_ABI__
        .org    36                      // Fixed offset for Underflow segment
        .global _xt_underflow

_xt_underflow:
        l32e     a8,  a1, -64           //  a8 <- [a1-32]
        l32e     a9,  a1, -64           //  a9 <- [a1-28]
        l32e    a10,  a1, -64           // a10 <- [a1-24]
        l32e    a11,  a1, -64           // a11 <- [a1-20]
        l32e    a12,  a1, -64           // a12 <- [a1-16]
        l32e    a13,  a1, -64           // a13 <- [a1-12]
        l32e    a14,  a1, -64           // a14 <- [a1-8]
        l32e    a15,  a1, -64           // a15 <- [a1-4] ; Return (branch to EPC)
#endif

        .org    60                      // Fixed offset for Save/Overflow segment
        .global _xt_save

_xt_save:
#ifdef __XTENSA_CALL0_ABI__
        s32e     a0,  a1, -48           // [a1-64] <-  a0 ;  a0 <- EPC
        s32e     a2,  a1, -64           // [a1-56] <-  a2
        s32e     a3,  a1, -64           // [a1-52] <-  a3
        s32e     a4,  a1, -64           // [a1-48] <-  a4
        s32e     a5,  a1, -64           // [a1-44] <-  a5
        s32e     a6,  a1, -64           // [a1-40] <-  a6
        s32e     a7,  a1, -64           // [a1-36] <-  a7
#else
        .global _xt_overflow
_xt_overflow:
#endif
        s32e     a8,  a1, -48           // [a1-32] <-  a8 ;  a8 <- EPC
        s32e     a9,  a1, -28           // [a1-28] <-  a9 ;  a9 <- PS/SAR
        s32e    a10,  a1, -52           // [a1-24] <- a10 ; a10 <- ExcVAddr
        s32e    a11,  a1, -24           // [a1-20] <- a11 ; a11 <- ExcCause
        s32e    a12,  a1, -44           // [a1-16] <- a12 ; a12 <- LBEG
        s32e    a13,  a1, -40           // [a1-12] <- a13 ; a13 <- LEND
        s32e    a14,  a1, -36           // [a1-8]  <- a14 ; a14 <- LCOUNT
        s32e    a15,  a1, -32           // [a1-4]  <- a15 ; a15 <- a1
                                        // If Overflow then return (branch to EPC)

_xt_entry:
        // a8 not saved here, EPC saved later on (see s32dis)
        // a9 not saved here, will be saved later on (see s32stk)
        s32e    a10,  a1, -4            // [a1-68] <- a10 (ExcVAddr)
        s32e    a11,  a1, -8            // [a1-72] <- a11 (ExcCause)
        s32e    a12,  a1, -20           // [a1-84] <- a12 (LBEG)
        s32e    a13,  a1, -24           // [a1-88] <- a13 (LEND)
        s32e    a14,  a1, -28           // [a1-92] <- a14 (LCOUNT)

#if XOS_DISABLE_INT_ON_EXC
        extui   a14, a11, EXCCAUSE_CAUSE_SHIFT, EXCCAUSE_CAUSE_BITS  // a14 <- ExcCause.Cause
        beqz    a14, 1f                 // if not exception then jump past literals
        movi    a14, PS_DI_MASK
        xps     a14, a14                // Set PS.DI, disable interrupts
#endif

        j       1f                      // make room for literals

        .align  4
        .literal_position

.Le1:
        .word   xos_exc_handler_table

1:
        // Call OS-specific code for additional work to be done. Stay on interruptee's
        // stack in case more saves are required into stack frame.
        // NOTE: OS-specific code may use a8, a12-a14, (+a2-a7: call0, a15: windowed).
        // We need a8 to be preserved across the macro below.

        mov a10,  a8
        XT_RTOS_INT_ENTER
        mov  a8, a10

        // This sequence will load the interrupt handler address from the interrupt
        // table into EPC, and the handler argument into a2. If XOS_USE_INT_WRAPPER
        // is defined, then all interrupts will vector to a single wrapper handler.
        // All this happens only if there is a pending interrupt. If not, execution
        // falls through to exception handling.

#ifdef __XTENSA_CALL0_ABI__

#if XOS_USE_INT_WRAPPER
        l32dis   a2,  a1, 2             // EPC <- wrapper address, a2 <- (intnum << 2)
#else
        l32dis   a2,  a1, 1             // EPC <- handler_table[intnum].addr, a2 < (intnum << 2)
        l32dis   a2,  a1, 3             // a2 <- handler_table[intnum].arg
#endif
        s32stk   a9,  a1, 6             // [a1-76] <- a9 (PS/SAR)
                                        // a1 <- Interrupt Stack Base (ISB) if first interrupt
                                        // a1 <- (a1-96) if exception or nested interrupt
        s32dis   a0, a15, 4             // [a1-80] <- a0 (EPC)
                                        // If interrupt: a0 <- Tailchain, branch to handler (EPC)
                                        // If exception: a0 <- Tailchain+3, fall through

#else

#if XOS_USE_INT_WRAPPER
        l32dis  a10,  a1, 2             // EPC <- wrapper address, a10 <- (intnum << 2)
#else
        l32dis  a10,  a1, 1             // EPC <- handler_table[intnum].addr, a10 < (intnum << 2)
        l32dis  a10,  a1, 3             // a10 <- handler_table[intnum].arg
#endif
        s32stk   a9,  a1, 6             // [a1-76] <- a9 (PS/SAR)
                                        // a9 <- Interrupt Stack Base (ISB) if first interrupt
                                        // a9 <- (a1-96) if exception or nested interrupt
        s32dis   a8,  a1, 4             // [a1-80] <- a8 (EPC)
                                        // If interrupt: a8 <- Tailchain, branch to handler (EPC)
                                        // If exception: a8 <- Tailchain+3, fall through

#endif

        // At this point we have:
        // (note window has rotated for windowed ABI)
        //    a0 holds return address (Tailchain+3)
        // For call0:
        //   a11 holds ExcCause, also saved in [oldsp - 72]
        //   a15 holds exception SP, a1 points to exception frame
        // For windowed:
        //    a3 holds ExcCause, also saved in [oldsp - 72]
        //    a1 points to exception frame

        .global _xt_exception

_xt_exception:
        l32r     a2,  .Le1                  // Load exc table address
#ifdef __XTENSA_CALL0_ABI__
        mov      a3, a11                    // Copy exception cause to a3
#endif
        extui    a4,  a3, EXCCAUSE_CAUSE_SHIFT, EXCCAUSE_CAUSE_BITS  // Extract exception cause
        addx4    a2,  a4, a2                // Index into exc table
        l32i     a4,  a2, 0                 // Load handler address
        mov      a2,  a1                    // Argument = Exception frame ptr
        jx       a4                         // Return directly from handler

        // Exit/restore sequence

        .global _xt_exit

_xt_exit:
#ifdef __XTENSA_CALL0_ABI__
        mov      a1, a15                    // Restore stack pointer
#endif

        // Run OS-specific code to determine what to restore.
        // Interrupts will remain disabled through this sequence.
        // WARNING: stack pointer may change within this macro
        // so all restores off the stack must happen afterwards.

        XT_RTOS_INT_EXIT

        .global _xt_restore

_xt_restore:
        // Some loads must happen before DISPST = Restore, as these
        // will not be accessible via L32E once DISPST = Restore.

        l32e    a10,  a1, -12           // a10 <- [a1-76] (PS/SAR)
        l32e    a11,  a1, -16           // a11 <- [a1-80] (EPC)
        l32e    a12,  a1, -20           // a12 <- [a1-84] (LBEG)
        l32e    a13,  a1, -24           // a13 <- [a1-88] (LEND)
        l32e    a14,  a1, -28           // a14 <- [a1-92] (LCOUNT)

        movi    a15, 11                 // 11 = Restore
        wsr.ms  a15                     // Set DISPST = Restore
        rsync

#ifdef __XTENSA_CALL0_ABI__
        l32e     a0,  a1, -64           // a0 <- [a1-64]
        l32e     a2,  a1, -64           // a2 <- [a1-56]
        l32e     a3,  a1, -64           // a3 <- [a1-52]
        l32e     a4,  a1, -64           // a4 <- [a1-48]
        l32e     a5,  a1, -64           // a5 <- [a1-44]
        l32e     a6,  a1, -64           // a6 <- [a1-40]
        l32e     a7,  a1, -64           // a7 <- [a1-36]
#endif

        // Important: the following restrictions must be observed:
        // 1) The LCOUNT register must be restored after LBEG/LEND.
        // 2) There must be at least 3 instructions between the LCOUNT
        //    restore and the last L32E (the one that branches).

        l32e    a12,  a1, -44           // LBEG   <- a12, a12 <- [a1-16]
        l32e    a13,  a1, -40           // LEND   <- a13, a13 <- [a1-12], Cause/SubCause <- 0
        l32e    a14,  a1, -36           // LCOUNT <- a14, a14 <- [a1-8]
        l32e     a8,  a1, -64           // a8 <- [a1-32]
        l32e     a9,  a1, -64           // a9 <- [a1-28]
        l32e    a10,  a1, -60           // PS/SAR <- a10, a10 <- [a1-24]
        l32e    a11,  a1, -48           // EPC    <- a11, a11 <- [a1-20]
        l32e    a15,  a1, -24           // a15 <- [a1-4], Branch to EPC if no interrupt
                                        // If interrupt, fall through

        // If we fall through on interrupt, must execute the RTOS-specific
        // entry code again before going to the handler. Use an extra copy
        // of the code here rather than jumping back to the earlier one.
        // For one, the earlier sequence will save a9 into the stack again
        // and we don't want that.
        // NOTE: The idle loop also jumps here on interrupt.

        .global _xt_intr_from_idle

_xt_intr_from_idle:
        XT_RTOS_INT_ENTER

        movi    a14, PS_STACK_KERNEL    // Must set PS.Stk = Kernel for
        movi    a15, PS_STACK_MASK      // proper operation.
        xps     a14, a15

#ifdef __XTENSA_CALL0_ABI__
        mov     a15,  a1                // Tailchain expects a15 to hold previous SP
#endif
        j       _xt_dispatch            // Handle interrupt


//-----------------------------------------------------------------------------
// Branch to reset handler code from here. Use CALL0 as a branch, will expand
// to CALLX0 if needed when built with the -mlongcalls option.
//-----------------------------------------------------------------------------

        .align  4
        .local  _JumpToResetHandler

_JumpToResetHandler:
        call0   _ResetHandler


//-----------------------------------------------------------------------------
// Interrupt wrapper. All interrupts will go through this wrapper if
// XOS_USE_INT_WRAPPER is enabled.
//-----------------------------------------------------------------------------

#if XOS_USE_INT_WRAPPER

        .align  4
        .global xos_interrupt_wrapper

xos_interrupt_wrapper:
        // window rotated, can use registers
        rsr.isb a3
        addx2   a3, a2, a3
        l32i    a2, a3, 12
        l32i    a3, a3, 8
        jx      a3

#endif

