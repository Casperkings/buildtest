// overlay_os_asm.S -- Overlay manager OS support assembly routines.
// $Id$

// Copyright (c) 2013 Tensilica Inc.
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

// At this time overlays are not supported without windowing.
#if defined(__XTENSA_WINDOWED_ABI__) && !XCHAL_HAVE_XEA3


// This function auto-restores an overlay if needed before resuming an
// interrupted task. This is meant to be invoked indirectly from the
// context switch handler of a multitasking system. As written this
// function is meant to work with XOS, but should require little change
// to work with any other RTOS.
// The following state is expected at entry. Parts of this state need
// to be set up by invoking the xt_overlay_check_map assembly macro or
// its equivalent C function, immediately before jumping here.
// NOTE that this code is not meant to be jumped to directly. The call
// to xt_overlay_check_map will set the resumption PC for the task to
// point here, if needed.
//
// Expected state at entry:
// - Interrupts are disabled (via PS), PS.EXCM not set
// - Can execute privileged instructions
// - All registers are restored to the task's state (except PC/PS)
// - Three values are stored below the stack pointer as follows:
//    (a1 - 24)    overlay ID
//    (a1 - 28)    PC to resume task at
//    (a1 - 32)    task PS to restore
//
// Control will transfer to the "return PC" after the overlay is
// successfully mapped.
//
// NOTE: This code will use 32 bytes of stack space for local store.

        .section .text, "ax"
        .global  xt_overlay_restore
        .align   4

xt_overlay_restore:

// This sequence may trigger a window or alloca exception, but we
// should be fine since interrupts are disabled and we're not in
// exception mode.

        s32e    a8, a1, -36                // Save a8
        s32e    a9, a1, -40                // Save a9
        addi    a8, a1, -64                // Make room local storage
        movsp   a1, a8

// Check to see if the desired overlay is mapped, and if so jump to
// the task code atomically. We don't want to take any interrupts
// between checking the overlay ID and transferring to the task code
// since the "return PC" could be inside the overlay.

.Lxtor_check:
        l32i    a8, a1, 32                // Get task's saved PS
        movi    a9, 16                    // PS.EXCM - Bit 4
        or      a8, a8, a9                // Set PS.EXCM
        wsr     a8, PS
        rsync                             // PS.INTLEVEL restored

        l32i    a8, a1, 40                // Target overlay ID
        movi    a9, _ovly_id
        l16si   a9, a9, 0                 // Current overlay ID
        beq     a8, a9, .Lxtor_go         // Have the right overlay
        movi    a8, .Lxtor_load           // No, must load it
        wsr     a8, EPC_1
        rsync
        rfe

// Map the desired overlay. This code is run with interrupts enabled
// as we don't want to keep interrupts disabled over this potentially
// time consuming step. If we get interrupted and some task brings in
// another overlay, the check above will catch it and repeat the map.

.Lxtor_load:
        s32i    a10, a1, 20                // Save a10-a15
        s32i    a11, a1, 16
        s32i    a12, a1, 12
        s32i    a13, a1, 8
        s32i    a14, a1, 4
        s32i    a15, a1, 0

        l32i    a10, a1, 40                // Overlay ID
        call8   xt_overlay_map             // a8-a15 trashed

        l32i    a10, a1, 20                // Restore a10-a15
        l32i    a11, a1, 16
        l32i    a12, a1, 12
        l32i    a13, a1, 8
        l32i    a14, a1, 4
        l32i    a15, a1, 0
        j       .Lxtor_check

.Lxtor_go:

// Done. Restore task state and go. At this point the task's PS
// has been restored but PS.EXCM has been set (see above).
// But since we want to mess with the SP, we'll have to get out
// of exception mode for a bit. Once we move the SP we want to
// not take any interrupts since we will have live data below
// the SP.

        l32i    a9, a1, 32                // Saved task PS
        movi    a8, 15
        bbci.l  a9, 4, .L1
        addi    a9, a9, -16               // Clear EXCM (Bit 4) if set
.L1:
        or      a8, a9, a8                // Task PS -> Int level 15
        wsr     a8, PS
        rsync                             // Int level 15, no EXCM
        addi    a8, a1, 64                // Free local storage
        movsp   a1, a8
        movi    a8, 16                    // PS.EXCM - Bit 4
        or      a8, a9, a8                // Task PS -> EXCM set
        wsr     a8, PS
        rsync

        l32e    a8, a1, -28               // Return PC
        wsr     a8, EPC_1                 // Set up return address
        rsync
        l32e    a8, a1, -36
        l32e    a9, a1, -40
        rfe

        .size    xt_overlay_restore, . - xt_overlay_restore

#endif

