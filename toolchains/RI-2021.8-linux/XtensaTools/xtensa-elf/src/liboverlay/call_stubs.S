// call_stubs.S -- Overlay call-in and call-out stubs.
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
#if defined(__XTENSA_WINDOWED_ABI__)

// Macros to implement the overlay call-in and call-out stubs. These expand to
// one call-in and one call-out for each overlay number defined in the system.
// See overlay_asm.S for more.


#ifndef OV_ID
#define OV_ID    0
#endif

#define xstr(x)   str(x)
#define str(x)    #x
#define LAB1      xstr(OV_ID)
#define LAB2      _


// The call-in stub saves register a8 on the stack, loads the overlay number
// into a8, and jumps to the common handler. The common handler will make sure
// that the called function is loaded into memory before calling it.

        .macro	overlay_call_in    num lab1 lab2
        .section .overlay.call.text, "ax"
        .global  _overlay_call_in_\lab1\lab2
        .align   4

_overlay_call_in_\lab1\lab2:
        s32e	a8, a1, -48
        movi	a8, \num
        j	_overlay_call_in_common
        .endm

        overlay_call_in OV_ID LAB1 LAB2


// The call-out stub first calls the target function, then loads the overlay
// number into register a11 and jumps to the common handler. The handler will
// make sure that the caller function is present in memory before returning.

        .macro overlay_call_out     num lab1 lab2
        .section .overlay.call.text, "ax"
        .global  _overlay_call_out_\lab1\lab2
        .align   4

_overlay_call_out_\lab1\lab2:
        callx8  a9
        movi    a11, \num
        j       _overlay_call_out_common
        .endm

        overlay_call_out OV_ID LAB1 LAB2

#endif

