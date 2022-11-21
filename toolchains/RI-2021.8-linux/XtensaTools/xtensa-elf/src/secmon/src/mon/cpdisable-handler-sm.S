// Copyright (c) 1999-2021 Cadence Design Systems, Inc.
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

// cpdisable-handler-sm.S -- Default coprocessor disabled handler


#include <xtensa/coreasm.h>
#include "xtos-internal.h"

#include "xtensa/secmon-defs.h"


	// Implement a basic coprocessor disabled handler to set the 
	// corresponding bit in the cpenable register.
	//
	// On entry: a1 = secure SP, with frame allocated
	//           a2 = exccause
	//           a3 = clobbered
	//           a4 = unsaved
	//           old a1-a3 saved to secure stack frame

	.text
	.align	4
	.global	secmon_cpdisable_handler
secmon_cpdisable_handler:
	//  HERE:  a2, a3 have been saved to the exception stack frame allocated with a1 (sp).
	//  a2 already contains exccause; reuse it.

#if XCHAL_HAVE_CP

	movi	a3, EXCCAUSE_CP0_DISABLED
	sub	a2, a2, a3
	rsr.sar	a3
	ssl	a2
	movi	a2, 1
	sll	a2, a2
	wsr.sar	a3
	rsr.cpenable	a3
	or	a3, a3, a2
	wsr.cpenable	a3

#endif

	movi	a2, 0               // wipe excsave1
	wsr.excsave1	a2

	l32i	a3, a1, UEXC_a3
	l32i	a2, a1, UEXC_a2
	l32i	a1, a1, UEXC_vpri   // restore nonsecure SP, EPC1 unchanged
	rfe

	.size	secmon_cpdisable_handler, . - secmon_cpdisable_handler

