// crt0-app.S (for XEA5)
// Applications downloaded in RAM using a debug monitor (eg. XMON)
// start here at _app_reset.  Such applications don't have any vectors:
// all exceptions are handled by the debug monitor.
// Thus, this file essentially plays the role of the reset vector
// to setup a few things before jumping to _start (in crt1*.S).

// Copyright (c) 2005-2021 Cadence Design Systems, Inc.
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

#include <xtensa/cacheattrasm.h>
#include <xtensa/coreasm.h>
#include <xtensa/corebits.h>


//  Assumptions on entry to _app_reset:
//	- debug monitor handles all exceptions, has setup all vectors
//	- interrupt related state is initialized
//	  (interrupts disabled or enabled for debug monitor's purposes)
//	- debug option related state is initialized (for debug monitor)
//	- any MMU related state is initialized (all handled by debug monitor)
//	- caches are initialized (cache attributes not necessarily correct)
//	- entire application is loaded (no unpacking needed here)

//  Assumptions on exit from _app_reset, ie. when jumping to _start:
//	- interrupts are disabled
//	- C calling context not initialized:
//	  - SP not initialized
//	- the following are initialized:
//	  - LCOUNT, CPENABLE, FP's FCR and FSR, cache attributes

/**************************************************************************/

	.text
	.global		_app_reset
	.p2align	2

_app_reset:

#if XCHAL_HAVE_LOOPS
	//wsr.lcount	x0		// loop count = 0
#endif

	// Disable all interrupts until stack is set up (outside this file).

	// Enable caches if needed (cache init is assumed to be done).

	// Enable all coprocessors.
#if XCHAL_HAVE_CP
	addi	x5, x0, 0xFF
	//wsr.cpenable	x5
#endif

	// Floating point coprocessor option initialization.
#if XCHAL_HAVE_FP
	//wur.fcr	x0	// clear FCR (default rounding mode, round-nearest)
	//wur.fsr	x0	// clear FSR
#endif

	// Jump to application entry (CRT init).
	la	x5, _start
	jalr	x0, x5, 0

	.size	_app_reset, . - _app_reset

