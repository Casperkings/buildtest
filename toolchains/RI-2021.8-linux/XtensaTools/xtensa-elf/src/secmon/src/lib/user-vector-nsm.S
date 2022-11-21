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

// user-vector-nsm.S -- User Vector for General Exceptions


#include <xtensa/coreasm.h>
#include <xtensa/config/system.h>
#include "xtos-internal.h"

#if XCHAL_HAVE_EXCEPTIONS

	// Vector code
	//
	// Secure Boot: Non-secure applications require a user "vector" and
	// handler so the secure monitor to steer L1 interrupts and exceptions
	// to nonsecure code.
	//
	// NOTE: The standard vector section names cannot be moved without 
	// causing problems for xt-genldscripts, so we implement this in .text;
	// size restrictions are no longer a concern, which simplifies things.
	//
	// .section		.UserExceptionVector.text, "ax"
	.section		.text, "ax"
	.align	4
	.global	xtsmUserExceptionVector

xtsmUserExceptionVector:

	// At entry, secure monitor has saved EPC1 in EXCSAVE1, PS.EXCM=0,
	// PS.INTLEVEL=EXCM_LEVEL (bits 0-2), RING (bit 3).

	addi	a1, a1, -ESF_TOTALSIZE	// allocate exception stack frame, etc.
	s32i	a2, a1, UEXC_a2
	s32i	a3, a1, UEXC_a3
	rsr.excsave1	a3
	wsr.epc1	a3		// restore epc1 from excsave1
	movi	a3, ~PS_INTLEVEL_MASK
	rsr.ps	a2
	bbci.l	a2, 3, 1f		// Bit 3 indicates PS.RING value
	and	a2, a2, a3		// ring 1
	movi	a3, PS_EXCM | PS_INTLEVEL(0) | PS_RING(1)
	or	a2, a2, a3
	j	2f
1:
	and	a2, a2, a3		// ring 0
	movi	a3, PS_EXCM | PS_INTLEVEL(0)
	or	a2, a2, a3
2:
	wsr.ps	a2			// PS.INTLEVEL = 0, PS.EXCM = 1

	movi	a3, xtos_exc_handler_table
	rsr.exccause	a2		// get exception cause
	//interlock
	addx4	a3, a2, a3
	l32i	a3, a3, 0
	s32i	a4, a1, UEXC_a4
	jx	a3			// jump to cause-specific handler

	.size	xtsmUserExceptionVector, . - xtsmUserExceptionVector


	.weak	xtos_cause3_handler

	/*
	 *  Table of assembly-level general-exception handlers
	 *  (quickly entered) for user vectored exceptions.
	 *  Provides entries for all possible 64 exception causes
	 *  currently allowed for in the EXCCAUSE register.
	 *
	 *  NOTE:  entries that have a corresponding C handler
	 *  (registered at run-time) point to xtos_c_wrapper_handler;
	 *  entries that have no handler point to xtos_unhandled_exception.
	 *
	 *  NOTE: Secure Boot: Not all of the following exception causes
	 *  can be routed to nonsecure handling, but are included here
	 *  for completeness and potential future expansion.
	 */

	.data

	.global	xtos_exc_handler_table
	.align	4

xtos_exc_handler_table:
	.word	xtos_unhandled_exception	// 0 IllegalInstruction
	.word	_xtos_syscall_handler		// 1 Syscall
	.word	xtos_unhandled_exception	// 2 InstructionFetchError
	.word	xtos_unhandled_exception	// 3 LoadStoreError
# if XCHAL_HAVE_INTERRUPTS
	.word	_xtos_l1int_handler		// 4 Level1Interrupt
# else
	.word	xtos_unhandled_exception	// 4 Level1Interrupt (not configured)
# endif
# if XCHAL_HAVE_WINDOWED && !defined(__XTENSA_CALL0_ABI__)
	.word	_xtos_alloca_handler		// 5 Alloca (MOVSP)
# else
	.word	xtos_unhandled_exception	// 5 Alloca (MOVSP) (not configured)
# endif
	.word	xtos_unhandled_exception	// 6 IntegerDivideByZero
	.word	xtos_unhandled_exception	// 7 Speculation
	.word	xtos_unhandled_exception	// 8 Privileged
	.word	xtos_unhandled_exception	// 9 Unaligned
	.word	xtos_unhandled_exception	//10 (reserved for Tensilica)
	.word	xtos_unhandled_exception	//11 (reserved for Tensilica)
	.word	xtos_cause3_handler		//12 PIF data error on fetch
	.word	xtos_cause3_handler		//13 PIF data error on ld/st
	.word	xtos_cause3_handler		//14 PIF address error on fetch
	.word	xtos_cause3_handler		//15 PIF address error on ld/st
	.word	xtos_unhandled_exception	//16 InstTLBMiss
	.word	xtos_unhandled_exception	//17 InstTLBMultiHit
	.word	xtos_unhandled_exception	//18 InstFetchPrivilege
	.word	xtos_unhandled_exception	//19 (reserved for Tensilica)
	.word	xtos_unhandled_exception	//20 InstFetchProhibited
	.word	xtos_unhandled_exception	//21 (reserved for Tensilica)
	.word	xtos_unhandled_exception	//22 (reserved for Tensilica)
	.word	xtos_unhandled_exception	//23 (reserved for Tensilica)
	.word	xtos_unhandled_exception	//24 LoadStoreTLBMiss
	.word	xtos_unhandled_exception	//25 LoadStoreTLBMultiHit
	.word	xtos_unhandled_exception	//26 LoadStorePrivilege
	.word	xtos_unhandled_exception	//27 (reserved for Tensilica)
	.word	xtos_unhandled_exception	//28 LoadProhibited
	.word	xtos_unhandled_exception	//29 StoreProhibited
	.word	xtos_unhandled_exception	//30 (reserved for Tensilica)
	.word	xtos_unhandled_exception	//31 (reserved for Tensilica)
	.rept	8
	.word	xtos_unhandled_exception	//32-39 Coprocessor<n>Disabled (n = 0..7)
	.endr
	.rept	XCHAL_EXCCAUSE_NUM-40
	.word	xtos_unhandled_exception	//40-63 (reserved for TIE)
	.endr

	.size	xtos_exc_handler_table, . - xtos_exc_handler_table

	.text

#endif /* XCHAL_HAVE_EXCEPTIONS */

