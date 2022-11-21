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

// int-medpri-handlers-nsm.S -- Non-secure interrupt handlers (for 1 < levels <= EXCM_LEVEL)


// To use this template file, define a macro called _INTERRUPT_LEVEL
// to be the interrupt priority level of the vector, then include this file.
// The default Makefile defines _INTERRUPT_LEVEL when assembling this file
// for each medium and high priority interrupt level.


#include <xtensa/coreasm.h>
#include "xtos-internal.h"


#if XCHAL_HAVE_INTERRUPTS

// Only generate XTSM per-level virtual vectors for 1 < level <= EXCM_LEVEL
# if INTERRUPT_IS_MED(_INTERRUPT_LEVEL)

	// Vector code
	//
	// Secure Boot: Non-secure applications require a per-level "vector" and
	// handler for each level <= EXCM_LEVEL that is to be serviced by nonsecure code.
	// This permits the secure monitor to steer medium-priority interrupts 
	// to nonsecure code.
	//
	// NOTE: The standard vector section names cannot be moved without 
	// causing problems for xt-genldscripts, so we implement this in .text;
	// size restrictions are no longer a concern, which simplifies things.

	.section		.text, "ax"
	.align	4
	.global	LABEL(xtsmLevel,Vector)

LABEL(xtsmLevel,Vector):

	addi	a1, a1, -ESF_TOTALSIZE
	s32i	a2, a1, UEXC_a2
	s32i	a3, a1, UEXC_a3

	// State after RFE from Secure Monitor:
	// epc1 is in excsaveN, EPSN/EPCN are accurate, 
	// PS.EXCM = 0, PS.INTLEVEL = EXCM_LEVEL
	readsr	excsave _INTERRUPT_LEVEL a3
	wsr.epc1	a3
	rsr.ps	a3
	movi	a2, ~PS_INTLEVEL_MASK
	and	a3, a3, a2
	movi	a2, PS_EXCM|PS_INTLEVEL(_INTERRUPT_LEVEL)
	or	a3, a3, a2
	wsr.ps	a3
	// At this point, epc1 is restored, PS.EXCM = 1, PS.INTLEVEL = N


	// Nonsecure _Level<n>FromVector (XTOS) expects a2 to be saved in excsaveN
	// Restore remaining state so interrupt handling happens as usual
	l32i	a3, a1, UEXC_a3
	l32i	a2, a1, UEXC_a2
	addi	a1, a1, ESF_TOTALSIZE
	writesr	excsave _INTERRUPT_LEVEL a2

	movi	a2, LABEL(_Level,FromVector)
	jx	a2

	.size	LABEL(xtsmLevel,Vector), . - LABEL(xtsmLevel,Vector)

# endif

#endif /* XCHAL_HAVE_INTERRUPTS */
