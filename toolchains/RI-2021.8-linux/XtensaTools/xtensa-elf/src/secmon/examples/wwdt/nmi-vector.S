// Copyright (c) 2003-2021 Cadence Design Systems, Inc.
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

// The NMI exception vector handles non-maskable interrupts. The handler
// sets misc0 to 1 (to flag NMI was triggered).  Zeroing misc0 is handled
// in wwdt.c.

// nmi-vector.S  --  Standalone NMI Interrupt Vector/Handler


#include <xtensa/coreasm.h>
#include <xtensa/config/system.h>

#if XCHAL_HAVE_NMI

	.begin	literal_prefix	.NMIExceptionVector
	.section		.NMIExceptionVector.text, "ax"

	.align	4
	.global _NMIExceptionVector
_NMIExceptionVector:

	writesr	excsave XCHAL_NMILEVEL a0	# save a0

	# Notify application that NMI has been triggered
	movi	a0, 1
	wsr.misc0	a0

	readsr	excsave XCHAL_NMILEVEL a0	# restore a0
	rfi	XCHAL_NMILEVEL			# done

	.size	_NMIExceptionVector, . - _NMIExceptionVector
	.text
	.end	literal_prefix

#endif /* XCHAL_HAVE_NMI */

