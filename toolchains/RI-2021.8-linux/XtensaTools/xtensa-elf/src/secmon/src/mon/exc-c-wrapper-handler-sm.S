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

// exc-c-wrapper-handler-sm.S -- General Exception Handler that Dispatches C Handlers


#include <xtensa/coreasm.h>
#include <xtensa/corebits.h>
#include "xtos-internal.h"

#if XCHAL_HAVE_XEA2 && XCHAL_HAVE_EXCEPTIONS



//  This assembly-level handler causes the associated exception (usually causes 12-15)
//  to be handled as if it were exception cause 3 (load/store error exception).
//  This provides forward-compatibility with a possible future split of the
//  load/store error cause into multiple more specific causes.

	.align	4
	.global	xtos_cause3_handler

xtos_cause3_handler:
	movi	a2, EXCCAUSE_LOAD_STORE_ERROR
	j	xtos_c_wrapper_handler

	.size	xtos_cause3_handler, . - xtos_cause3_handler

	.align	4
.Lhi:
	addi	a2, a2, -XCHAL_EXCM_LEVEL
	add	a2, a2, a3
	j	.Lps


//  This is the general exception assembly-level handler that dispatches C handlers.

	.align	4
	.global	xtos_c_wrapper_handler
	.global	xtos_c_wrapper_dispatch

xtos_c_wrapper_handler:
	//  Redundantly de-allocate and re-allocate stack, so that GDB prologue
	//  analysis picks up the allocate part, and figures out how to traceback
	//  through the call stack through the exception. Interrupts higher than
	//  EXCM_LEVEL should not be using this stack so this should be safe.
	addi	a1, a1, ESF_TOTALSIZE	// de-allocate stack frame

xtos_c_wrapper_dispatch:
	//  GDB starts analyzing prologue after most recent global symbol, so here:
	addi	a1, a1, -ESF_TOTALSIZE	// re-allocate stack frame

	// Validate SP is still secure
	movi	a3, _stack_sentry
	addi	a3, a3, 32          // C exception handlers may need a 32B stack frame
	bltu	a1, a3, secmon_stack_overflow
	movi	a3, _heap_sentry
	bgeu	a1, a3, secmon_stack_overflow

	//  HERE:  a2, a3 have been saved to exception stack frame allocated with a1 (sp).
	//  a2 contains EXCCAUSE.
	s32i	a2, a1, UEXC_exccause

	//  At some point we may save EXCVADDR into the exception frame.
	//rsr.excvaddr	a3
	//s32i	a3, a1, UEXC_excvaddr

	s32i	a4, a1, UEXC_a4		// a4 not saved by user vector

	//  Set PS fields:
	//	EXCM     = 0
	//	WOE      = __XTENSA_CALL0_ABI__ ? 0 : 1
	//	UM       = 1
	//	INTLEVEL = MIN(INTLEVEL,EXCM_LEVEL)
	//	CALLINC  = __XTENSA_CALL0_ABI__ ? 0 : 1
	//	OWB      = 0 (really, a dont care if !__XTENSA_CALL0_ABI__)

	rsr.ps	a3
	movi	a4, PS_PNSM_MASK
	movi	a2, PS_WOECALL4_ABI|PS_UM|PS_INTLEVEL(XCHAL_EXCM_LEVEL)  // CALL4 emulation
	and	a4, a3, a4
	or	a2, a2, a4
	s32i	a3, a1, UEXC_ps
	extui	a3, a3, 0, 4			// extract PS.INTLEVEL
	bgeui	a3, XCHAL_EXCM_LEVEL+1, .Lhi	// at PS.INTLEVEL > EXCM_LEVEL ?
.Lps:
	rsr.epc1	a3
	wsr.ps	a2

	//  Window overflows enabled, but NOT SAFE because we're not quite
	//  in a valid windowed context (haven't restored a1 yet...);
	//  so don't cause any (keep to a0..a3) until we've saved critical
	//  state and restored a1.
	//  NOTE:  MUST SAVE EPC1 before causing any overflows, because
	//  overflows corrupt EPC1.

	s32i	a3, a1, UEXC_pc

	s32i	a0, a1, UEXC_a0		// save the rest of the registers
	s32i	a5, a1, UEXC_a5
	s32i	a6, a1, UEXC_a6
	s32i	a7, a1, UEXC_a7
	s32i	a8, a1, UEXC_a8
	s32i	a9, a1, UEXC_a9
	s32i	a10, a1, UEXC_a10
	s32i	a11, a1, UEXC_a11
	s32i	a12, a1, UEXC_a12
	s32i	a13, a1, UEXC_a13
	s32i	a14, a1, UEXC_a14
	s32i	a15, a1, UEXC_a15

	rsync				// wait for WSR to PS to complete

#if XCHAL_HAVE_INTERRUPTS
	//  Restore PS.INTLEVEL to its saved value (re-enables interrupts
	//  if they were enabled before taking the exception).
	//
	//  Secure Monitor: do not lower below EXCM_LEVEL
	l32i	a13, a1, UEXC_ps
	rsr.ps	a14
	extui	a13, a13, 0, 4		// extract saved PS.INTLEVEL
	extui	a15, a14, 0, 4		// extract current PS.INTLEVEL
	xor	a14, a14, a15		// clear a14.INTLEVEL
	bgeui	a13, XCHAL_EXCM_LEVEL+1, .Lrestore_intlevel
	movi	a13, XCHAL_EXCM_LEVEL
.Lrestore_intlevel:
	xor	a14, a14, a13		// replace with saved PS.INTLEVEL
	wsr.ps	a14			// restore PS.INTLEVEL
#endif

	rsr.sar	a12
	movi	a13, xtos_c_handler_table	// &table
	l32i	a15, a1, UEXC_exccause		// arg2: exccause

	s32i	a12, a1, UEXC_sar
	save_loops_mac16	a1, a12, a14	// save LOOP & MAC16 regs, if configured

	addx4	a12, a15, a13		// a12 = table[exccause]
	l32i	a12, a12, 0		// ...

	.global	_GeneralException

_GeneralException:			// this label makes tracebacks through exceptions look nicer

	.global	_GeneralExceptionFrameSize
	.set	_GeneralExceptionFrameSize, ESF_TOTALSIZE
	.global	_GeneralExceptionRegisterSaveOffset
	.set	_GeneralExceptionRegisterSaveOffset, UEXC_a0

	mov	a2, a1			// arg1: exception parameters
	mov	a3, a15			// arg2: exccause
	beqz	a12, 1f			// null handler => skip call
	callx0	a12			// call C exception handler for this exception

	.size	_GeneralException, . - _GeneralException

1:
	//  Now exit the handler. Restore special registers.

	restore_loops_mac16	a1, a13, a14, a15	// restore LOOP & MAC16 regs, if configured
	l32i	a14, a1, UEXC_sar

	//  Disable interrupts while returning from the pseudo-CALL setup above,
	//  for the same reason they were disabled while doing the pseudo-CALL:
	//  this sequence restores SP such that it doesn't reflect the allocation
	//  of the exception stack frame, which we still need to return from
	//  the exception.

#if XCHAL_HAVE_INTERRUPTS
	rsil	a12, XCHAL_EXCM_LEVEL
#endif
	wsr.sar	a14

	movi	a0, _xtos_return_from_exc
	jx	a0

	.size	xtos_c_wrapper_handler, . - xtos_c_wrapper_handler

#endif // XCHAL_HAVE_XEA2 && XCHAL_HAVE_EXCEPTIONS

