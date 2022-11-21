// Copyright (c) 2002-2021 Cadence Design Systems, Inc.
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

// exc-return-sm.S -- Shared exception/interrupt return code


#include <xtensa/coreasm.h>
#include <xtensa/corebits.h>
#include "xtos-internal.h"

	.text
	.align	4
	.global	secmon_return_from_exc
secmon_return_from_exc:

    // Also catch secure C (XTOS) exception handling returns
	.global	_xtos_return_from_exc
_xtos_return_from_exc:

# if XCHAL_HAVE_EXCLUSIVE
	// Clear exclusive monitors.
	clrex
# endif

	l32i	a0, a1, UEXC_a0		// restore general registers, pc, ps
	l32i	a4, a1, UEXC_a4
	l32i	a5, a1, UEXC_a5
	l32i	a6, a1, UEXC_a6
	l32i	a7, a1, UEXC_a7
	l32i	a8, a1, UEXC_a8
	l32i	a9, a1, UEXC_a9
	l32i	a10, a1, UEXC_a10
	l32i	a11, a1, UEXC_a11
	l32i	a12, a1, UEXC_a12
	l32i	a13, a1, UEXC_a13
	l32i	a14, a1, UEXC_a14
	l32i	a15, a1, UEXC_a15

	l32i	a2, a1, UEXC_pc
	l32i	a3, a1, UEXC_ps
	wsr.epc1	a2
	wsr.ps	a3
	movi	a2, 0
	wsr.excsave1	a2  // wipe excsave1

	l32i	a2, a1, UEXC_a2
	l32i	a3, a1, UEXC_a3
	
	rsync				// wait for WSR to PS to complete

	// Restore previous SP (secure or nonsecure)
	l32i	a1, a1, UEXC_vpri
	rfe

	.size	secmon_return_from_exc, . - secmon_return_from_exc


	.align	4
	.global	secmon_dispatch_L1int_to_nsm
secmon_dispatch_L1int_to_nsm:

	// Secure Monitor: a2 = nonsecure L1 handler
	l32i	a0, a1, UEXC_a0		// restore general registers, pc, ps
	l32i	a4, a1, UEXC_a4
	l32i	a5, a1, UEXC_a5
	l32i	a6, a1, UEXC_a6
	l32i	a7, a1, UEXC_a7
	l32i	a8, a1, UEXC_a8
	l32i	a9, a1, UEXC_a9
	l32i	a10, a1, UEXC_a10
	l32i	a11, a1, UEXC_a11
	l32i	a12, a1, UEXC_a12
	l32i	a13, a1, UEXC_a13
	l32i	a14, a1, UEXC_a14
	l32i	a15, a1, UEXC_a15

	l32i	a3, a1, UEXC_pc
	wsr.epc1	a2		// epc1 <- a2 (nonsecure L1 handler)
	wsr.excsave1	a3		// excsave1 <- a3 (nonsecure EPC1)

	// Restore PS, but at PS.INTLEVEL(EXCM_LEVEL) and with PS.EXCM set
	// Prevents exceptions after RFE but before user handler gains control
	l32i	a3, a1, UEXC_ps
	movi	a2, ~PS_INTLEVEL_MASK
	and	a3, a3, a2
	bbci.l	a3, 6, 1f
	addi	a3, a3, 0x8
	addi	a3, a3, -PS_RING(1)
1:
	movi	a2, PS_EXCM|PS_INTLEVEL(XCHAL_EXCM_LEVEL)
	or	a3, a3, a2
	wsr.ps	a3

	// Ensure exccause is correct in case an exception occurred during dispatch
	// since PS.EXCM was clear
	movi	a2, EXCCAUSE_LEVEL1_INTERRUPT
	wsr.exccause	a2

	l32i	a2, a1, UEXC_a2
	l32i	a3, a1, UEXC_a3

	rsync				// wait for WSR to PS to complete

	// Done using secure stack; a1 <- nonsecure SP
	l32i	a1, a1, UEXC_vpri
	rfe

	.size	secmon_dispatch_L1int_to_nsm, . - secmon_dispatch_L1int_to_nsm


	.align	4
	.global	secmon_dispatch_exc_to_nsm
secmon_dispatch_exc_to_nsm:

	// On entry: a1 = secure SP, with frame allocated
	//           a2 = exccause
	//           a3 = clobbered
	//           a4 = unsaved
	//           old a1-a3 saved to secure stack frame

	s32i	a4, a1, UEXC_a4		// a4 not saved by user vector

	// If PS.PNSM is clear, secure code generated this exception, so
	// it cannot be handled by nonsecure code; trap instead.
	rsr.ps	a4
	extui	a3, a4, PS_PNSM_SHIFT, 1
	beqz	a3, secmon_exception_trap

	// xtos_c_handler_table[] simply contains nonsecure entry point for exception
	// save/dispatch/restore.  Table is reused to optimize memory usage.
	movi	a3, xtos_c_handler_table
	addx4	a3, a2, a3
	l32i	a3, a3, 0		// a3 <- nonsecure exception entry point

	rsr.epc1	a2
	wsr.excsave1	a2		// excsave1 <- old epc1
	wsr.epc1	a3		// epc1 <- nonsecure entry point

	// Set PS.INTLEVEL(EXCM_LEVEL); PS.EXCM and PS.PNSM already set
	// Prevents exceptions after RFE but before user handler gains control
	movi	a2, ~PS_INTLEVEL_MASK
	movi	a3, PS_INTLEVEL(XCHAL_EXCM_LEVEL)
	and	a4, a4, a2
	or	a4, a4, a3
	bbci.l	a4, 6, 1f
	addi	a4, a4, 0x8
	addi	a4, a4, -PS_RING(1)
1:
	wsr.ps	a4

# if XCHAL_HAVE_EXCLUSIVE
	// Clear exclusive monitors.
	clrex
# endif

	l32i	a2, a1, UEXC_a2
	l32i	a3, a1, UEXC_a3
	l32i	a4, a1, UEXC_a4
	
	rsync				// wait for WSR to PS to complete

	// Done using secure stack; a1 <- original epc1
	l32i	a1, a1, UEXC_vpri
	rfe

	.size	secmon_dispatch_exc_to_nsm, . - secmon_dispatch_exc_to_nsm

