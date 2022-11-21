// overlay_asm.S -- Assembly routines used by overlay manager.
// $Id$

// Copyright (c) 2013-2015 Tensilica Inc.
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

// At this time we need windowed ABI for overlays.
#if defined(__XTENSA_WINDOWED_ABI__) && !XCHAL_HAVE_XEA3

// Variables used by overlay manager.

        .section .data, "aw"
        .global        _ovly_id
        .global        _mapping_id
        .align        4
_ovly_id:
        .short        -1
_mapping_id:
        .short        -1


// Common code to handle all overlay call-ins. The stubs for each overlay
// ID will jump here. The following conditions are assumed:
//
// - user code has executed a call8/callx8, but the register window has
//   not rotated (i.e. entry has not been executed yet).
// - register a8 holds overlay number, a9 holds target function address.
// - the stack pointer has been moved down 32 bytes (alloca) to provide
//   working storage.
// - the original contents of a8 are saved at (a1 + 12) on the stack.
// - the processor is NOT in exception mode (PS.EXCM is zero).
//
// NOTE: We will use the working storage as follows -
//
//    SP + 12 : saved original value of a8
//    SP +  8 : saved target address
//    SP +  4 : saved overlay ID
//    SP +  0 : saved PS
//
// NOTE: In this code we set EXCM to protect against context switching.
// The assumption is that interrupts higher than EXCM level are not used
// to directly context switch.

        .section .overlay.call.text, "ax"
        .global  _overlay_call_in_common
        .align   4

_overlay_call_in_common:
        s32i    a9, a1, 8          // save target addr into scratch space
        s32i    a8, a1, 4          // save overlay ID into scratch space

// The compare-and-go step has to be atomic, so need to disable interrupts.
// But we will have to be able to handle an alloca exception.
.Lchecki:
#if XCHAL_HAVE_INTERRUPTS
        rsil    a9, 15             // disable interrupts
#else
        rsr     a9, PS             // no interrupts to disable
#endif
        s32i    a9, a1, 0          // save old PS
        l32i    a8, a1, 4          // required overlay ID
        movi    a9, _ovly_id       // current active overlay ID
        l16si   a9, a9, 0
        bne     a9, a8, .Lmapi     // if not matching then start map
        l32i    a9, a1, 8          // good to go, restore a9
        addi    a8, a1, 32
        movsp   a1, a8             // restore stack state
        l32e    a8, a1, -32        // recover old PS value
        addi    a8, a8, 16         // set bit 4 - PS.EXCM
        wsr     a8, PS             // PS restored but with EXCM set
        l32e    a8, a1, -20        // restore a8
        wsr     a9, EPC_1          // set up target address
        rsync
        rfe                        // transfer to target function
.Lmapi:
#if XCHAL_HAVE_INTERRUPTS
        l32i    a9, a1, 0          // restore old PS
        wsr     a9, PS
        rsync
#endif
        call8   __entry
        j       .Lchecki

        .align   4
	.local  __entry
__entry:
        entry   a1, 32             // allocate new stack frame, a0-a7 live
        l32i    a10, a1, 36        // overlay number (saved above)
        call8   xt_overlay_map     // map overlay
        isync                      // we changed instruction memory
        retw

        .size        _overlay_call_in_common, . - _overlay_call_in_common


// Common code to handle all overlay call-outs. The stubs for each overlay
// number will jump here. The following conditions are assumed:
//
// - the overlay-specific stub has already called the target function.
// - register a14 holds overlay number, a4 holds the return address.
// - register a8 was trashed by the target function so is free for use.
// - the processor is NOT in exception mode (PS.EXCM is zero).
//
// This function will check to make sure that the needed overlay is mapped
// and then return to the caller.
// NOTE: return values from target function could be in a10-a13, these must
// be preserved and sent back to caller.

        .section .overlay.call.text, "ax"
        .global  _overlay_call_out_common
        .align  4

_overlay_call_out_common:

// The fast path is when the IDs match so we want to keep that as short
// as we can. The check-and-go should be atomic wrt context switching.
// We assume that interrupts at higher than EXCM level will not directly
// cause a context switch.

.Lcheck:
        rsr     a8, PS             // safe to use a8 and a9 here
        addi    a8, a8, 16
        wsr     a8, PS             // EXCM set, context switch disabled
        rsync
        movi    a9, _ovly_id
        l16si   a9, a9, 0          // load current overlay ID
        bne     a9, a14, .Lmap     // if not the right one then map
        wsr     a4, EPC_1          // set up return address
        rsync
        rfe                        // return to caller
.Lmap:
        movi    a9, .Lcall         // get out of exception mode
        wsr     a9, EPC_1
        rsync
        rfe
.Lcall:
        call8   __ococ1            // dummy call to save registers
        j       .Lcheck            // check and go

        .align   4
	.local  __ococ1
__ococ1:
        entry   a1, 32             // allocate a new stack frame
        mov     a10, a6            // a6 is now overlay number
        call8   xt_overlay_map     // map overlay
        isync                      // we changed instruction memory
        retw

        .size        _overlay_call_out_common, . - _overlay_call_out_common

#endif

