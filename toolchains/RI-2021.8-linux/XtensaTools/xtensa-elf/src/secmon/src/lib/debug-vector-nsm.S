// Copyright (c) 1998-2021 Cadence Design Systems, Inc.
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

// debug-vector-nsm.S -- Debug Vector for nonsecure monitor


#include <xtensa/coreasm.h>
#include <xtensa/config/system.h>
#include "xtos-internal.h"

#if XCHAL_HAVE_EXCEPTIONS

	// Virtual vector code
	//
	// Secure Boot: Non-secure applications require a virtual debug "vector"
	// so the secure monitor can steer debug events to nonsecure code.
	//
	// NOTE: The standard vector section names cannot be moved without 
	// causing problems for xt-genldscripts, so we implement this in .text;
	// size restrictions are no longer a concern, which simplifies things.

	.section		.text, "ax"
	.align	4
	.global	xtsmDebugExceptionVector

xtsmDebugExceptionVector:

	addi	a1, a1, -ESF_TOTALSIZE	// allocate exception stack frame, etc.
	s32i	a2, a1, UEXC_a2

	// Restore state after RFI from Secure Monitor, for DEBUGLEVEL == D:
	// epcD is in excsaveD, epsD is in misc0
	readsr	excsave XCHAL_DEBUGLEVEL a2
	writesr	epc XCHAL_DEBUGLEVEL a2
	rsr.misc0	a2
	writesr	eps XCHAL_DEBUGLEVEL a2

	// Restore a1, a2
	l32i	a2, a1, UEXC_a2
	addi	a1, a1, ESF_TOTALSIZE

	// Save a0 in excsaveD.  _DebugExceptionHandler must expect this
	writesr	excsave XCHAL_DEBUGLEVEL a0
	movi	a0, _DebugExceptionHandler
	jx	a0

	.size	xtsmDebugExceptionVector, . - xtsmDebugExceptionVector


	// Overridden by XMON
	.align	4
	.weak	_DebugExceptionHandler

_DebugExceptionHandler:
	j	_DebugExceptionHandler

	.size	_DebugExceptionHandler, . - _DebugExceptionHandler


#endif /* XCHAL_HAVE_EXCEPTIONS */

