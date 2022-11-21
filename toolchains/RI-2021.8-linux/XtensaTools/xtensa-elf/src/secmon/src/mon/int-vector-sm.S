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

// int-vector-sm.S -- Interrupt Vector Template (for levels > 1)


// To use this template file, define a macro called _INTERRUPT_LEVEL
// to be the interrupt level of the vector, include "xtos-internal.h",
// then include this file.  We use the same template for both high-level
// and medium-level interrupts, but not debug level.


#define _ASMLANGUAGE
#include "xtos-internal.h"
#include <xtensa/coreasm.h>
#include <xtensa/config/system.h>


#if XCHAL_HAVE_INTERRUPTS && (XCHAL_HAVE_XEA1 || XCHAL_HAVE_XEA2) && \
	(_INTERRUPT_LEVEL <= XCHAL_NUM_INTLEVELS) && (!IS_DEBUGLEVEL(_INTERRUPT_LEVEL))

	.begin	literal_prefix	LABEL(.Level,InterruptVector)
	.section		LABEL(.Level,InterruptVector.text), "ax"
	.align 4
	.global	LABEL(_Level,Vector)
LABEL(_Level,Vector):
	// Medium and high priority interrupt vector:

	// Secure Monitor: Cannot rely on excsaveN containing handler address,
	// as it is not protected from nonsecure code modifying it.
	writesr	excsave	_INTERRUPT_LEVEL a2
	movi	a2, LABEL(_Level,FromVector)
	jx	a2

	.size	LABEL(_Level,Vector), . - LABEL(_Level,Vector)
	.text
	.end	literal_prefix

#endif /* interrupt at that level */

