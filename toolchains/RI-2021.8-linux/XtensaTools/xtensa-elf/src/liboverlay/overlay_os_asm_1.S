// overlay_os_asm_1.S -- Assembly routines intended for OS use.
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
#include "overlay_os_asm.h"

// At this time we need windowed ABI for overlays.
#if defined(__XTENSA_WINDOWED_ABI__)

// C-callable equivalent of _xt_overlay_get_state macro.
//
// unsigned xt_overlay_get_state(unsigned pc);
//
// Returns the overlay state of the current (interrupted) task.
// "pc" is the PC at which the task got interrupted. The return
// value must be stored with the task context and handed to the
// function xt_overlay_check_map() at task resumption.

        .section .text, "ax"
        .global  xt_overlay_get_state
        .type    xt_overlay_get_state,@function
        .align   4

xt_overlay_get_state:
        entry   a1, 32
        _xt_overlay_get_state a2, a3, a4
        retw

        .size    xt_overlay_get_state, . - xt_overlay_get_state


// C-callable equivalent of _xt_overlay_check_map macro.
//
// void xt_overlay_check_map(unsigned * pc, unsigned * ps,
//                           unsigned ovstate, unsigned sp);
//
// where "pc" is the PC at which the task got interrupted, "ps" is
// the task's PS, "ovstate" is the value returned from calling 
// xt_overlay_get_state(), and "sp" is the task's stack pointer.
// This function returns the adjusted PC at which the task should
// be resumed, and the adjusted PS for resumption.

        .section .text, "ax"
        .global  xt_overlay_check_map
        .type    xt_overlay_check_map,@function
        .align   4

xt_overlay_check_map:
        entry   a1, 32
        l32i    a6, a2, 0
        l32i    a7, a3, 0
        _xt_overlay_check_map a6 a7 a4 a5 a8
        s32i    a6, a2, 0
        s32i    a7, a3, 0
        retw

        .size    xt_overlay_check_map, . - xt_overlay_check_map

#endif

