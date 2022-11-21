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

// user-vector-sm.S -- User Vector for General Exceptions


#include <xtensa/coreasm.h>
#include <xtensa/config/system.h>
#include "xtos-internal.h"

#if XCHAL_HAVE_EXCEPTIONS

	// Vector code
	.begin	literal_prefix	.UserExceptionVector
	.section	.UserExceptionVector.text, "ax"
	.align	4
	.global	_UserExceptionVector

_UserExceptionVector:

	// We will be conservative here. This code may expand due to conversion
	// of movi -> l32r -> const16. Also due to workarounds being inserted
	// for some hardware versions. We don't force l32r usage here because
	// some customers may want only const16 to be used if available.

# if (XSHAL_USER_VECTOR_SIZE < 36)

	//  The vector may be as small as 12 bytes:

	wsr.excsave1	a2
	movi	a2, _UserExceptionFromVector	// load user exception handler address
	jx	a2				// jump to handler

	.size	_UserExceptionVector, . - _UserExceptionVector
	.end	literal_prefix

	//  Dispatch outside vector:
	.text
	.align	4
	.global	_UserExceptionFromVector
_UserExceptionFromVector:

# else

	//  There is space to dispatch right at the vector:

	// Secure Monitor: establish secure SP, save a1-a3.
	//
	// NOTE: This is different from XTOS, which saves a4 prior to 
	// jumping to handler, but there is insufficient space if const16
	// is used instead of L32R.
	//
	// If coming from nonsecure code, establish secure stack frame in a1,
	// otherwise allocate exception frame from a1 and check for overflow.
	// Secure stack overflow must be checked later (literal space limited).
	//
	// NOTE: saving a1 in user exception frame is required since monitor
	// uses a secure stack.  The frame struct (UEXC_) does not include
	// space for a1, but secure monitor doesn't support virtual interrupt
	// prioritization, so we store a1 there.  Can't use excsave1 as nested
	// exceptions will trash it.
	wsr.excsave1	a2

# endif

	rsr.ps	a2
	extui	a2, a2, PS_PNSM_SHIFT, PS_PNSM_BITS
	bnez	a2, .Lset_sp_secure
	mov	a2, a1			// PS.PNSM == 0
	j	.Lalloc_exc_frame

.Lset_sp_secure:
	movi	a2, secmon_secure_sp	// PS.PNSM == 1
	l32i	a2, a2, 0

.Lalloc_exc_frame:
	addi	a2, a2, -ESF_TOTALSIZE	// allocate exception stack frame, etc.
	s32i	a1, a2, UEXC_vpri	// UEXC_vpri not used in secure monitor
	or	a1, a2, a2

	rsr.excsave1	a2
	s32i	a2, a1, UEXC_a2
	s32i	a3, a1, UEXC_a3

	movi	a3, xtos_exc_handler_table
	rsr.exccause	a2		// get exception cause
	addx4	a3, a2, a3
	l32i	a3, a3, 0
	jx	a3			// jump to cause-specific handler


# if (XSHAL_USER_VECTOR_SIZE < 36)
	.size	_UserExceptionFromVector, . - _UserExceptionFromVector
#else
	.size	_UserExceptionVector, . - _UserExceptionVector
	.end	literal_prefix
#endif


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
	 */

	.data

	.global	xtos_exc_handler_table
	.align	4

xtos_exc_handler_table:
	.word	xtos_unhandled_exception	// 0 IllegalInstruction
	.word	secmon_syscall_handler		// 1 Syscall
	.word	xtos_unhandled_exception	// 2 InstructionFetchError
	.word	xtos_unhandled_exception	// 3 LoadStoreError
# if XCHAL_HAVE_INTERRUPTS
	.word	secmon_l1int_handler		// 4 Level1Interrupt
# else
	.word	xtos_unhandled_exception	// 4 Level1Interrupt (not configured)
# endif
	.word	xtos_unhandled_exception	// 5 Alloca (MOVSP) (must be handled in nonsecure space)
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
	.word	secmon_cpdisable_handler	//32-39 Coprocessor<n>Disabled (n = 0..7)
	.endr
	.rept	XCHAL_EXCCAUSE_NUM-40
	.word	xtos_unhandled_exception	//40-63 (reserved for TIE)
	.endr

	.size	xtos_exc_handler_table, . - xtos_exc_handler_table

	.text

#endif /* XCHAL_HAVE_EXCEPTIONS */

