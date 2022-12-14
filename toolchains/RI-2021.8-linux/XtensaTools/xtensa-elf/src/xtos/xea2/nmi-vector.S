// nmi-vector.S  --  Standalone NMI Interrupt Vector/Handler

// Copyright (c) 2003-2018 Cadence Design Systems, Inc.
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

// The NMI exception vector handles non-maskable interrupts.

#include <xtensa/coreasm.h>
#include <xtensa/config/system.h>

#if XCHAL_HAVE_NMI

	.begin	literal_prefix	.NMIExceptionVector
	.section		.NMIExceptionVector.text, "ax"

	.align	4
	.global _NMIExceptionVector
_NMIExceptionVector:

//  Insert any custom NMI handling code here.
//  For example:
//	writesr	excsave XCHAL_NMILEVEL a0
//	movi	a0, ...address of some save area specific to this code...
//	s32i	a1, a0, 0	// save whatever registers are needed
//	:
//	do something useful ...
//	:
//	l32i	a1, a0, 0	// restore whatever registers were saved
//	readsr	excsave XCHAL_NMILEVEL a0
//
//  This default NMI handler does not do anything.  It just returns
//  immediately upon any occurrence of a non-maskable interrupt.

	rfi	XCHAL_NMILEVEL

	.size	_NMIExceptionVector, . - _NMIExceptionVector
	.text
	.end	literal_prefix

#endif /* XCHAL_HAVE_NMI */

