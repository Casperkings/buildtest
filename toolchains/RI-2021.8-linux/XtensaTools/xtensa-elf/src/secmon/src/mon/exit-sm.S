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

// exit-sm.S -- Secure Monitor exit handling


#include <xtensa/coreasm.h>
#include <xtensa/config/core-isa.h>
#include <xtensa/simcall.h>

// Secure Boot: always use call0 ABI
#define CALL	call0


	.text
	.align	4
	.global	secmon_exit
	.type	secmon_exit, @function
secmon_exit:
	abi_entry	0, 4

	// If secmon_unpack encountered an error, exit with an error code
	beqz	a2, _invalid_nsm_entry

	// Secure Boot (step #8): Wipe all core state to prevent leakage to nonsecure mode.
	movi	a15, 0
	movi	a14, 0
	movi	a13, 0
	movi	a12, 0
	movi	a11, 0
	movi	a10, 0
	movi	a9, 0
	movi	a8, 0
	movi	a7, 0
	movi	a6, 0
	movi	a5, 0
	movi	a0, 0

	wsr.sar	a0
#if XCHAL_HAVE_LOOPS
	wsr.lcount	a0
	wsr.lbeg	a0
	wsr.lend	a0
#endif

	// PS.WOE is already clear, PS.PNSM is already set, interrupts already disabled.
	// Just need to clear PS.UM
	rsr.ps	a4
	movi	a3, ~PS_UM
	and	a4, a4, a3
	wsr.ps	a4

	// Save secure SP and reset a1.  Secure SP is uncached so no need to writeback.
	movi	a4, secmon_secure_sp
	s32i	a1, a4, 0
	movi	a1, 0

	movi	a4, 0
	movi	a3, 0

	// Set a0 in case nonsecure application returns without CRT init.
	// Doing so will trigger a security violation.
	movi	a0, secmon_post_exit

	// Secure Boot (step #9): Exit secure mode and jump to nonsecure entry point,
	// which remains in a2 (as main() return value).
	wsr.epc1	a2
	movi	a2, 0

	rfe

_invalid_nsm_entry:
	movi	a2, -11


	.global	secmon_post_exit
secmon_post_exit:

#if (defined RTL_SIM)
	// Exit RTL simulation with a "pass" status
	movi	a2, 1
	CALL	set_diag_status_fast
#endif

	// sync queues (if any, only for LX and later):
#if XCHAL_HW_MIN_VERSION >= XTENSA_HWVERSION_RA_2004_1	/* LX or later? */
	extw
#endif
	// can break to debug monitor, go to sleep with waiti, or just spin in a loop
.L0:
#if XCHAL_HW_MIN_VERSION >= XTENSA_HWVERSION_RE_2013_2	/* SIMCALL is NOP in hw? */
	// ISS expects exit code in a3
# if (defined __XTENSA_CALL0_ABI__)
	mov	a3, a12	
# else
	mov	a3, a2
# endif
	mov	a4, a3		// save exitcode for the debugger, as simcall will erase a3
	movi	a2, SYS_exit
	simcall			// exit if in simulator, else NOP
	mov	a2, a4
#endif
#if XCHAL_HAVE_DEBUG
	break	1, 15		// back to debugger, if one is attached
#endif
#if XCHAL_HAVE_INTERRUPTS
	waiti	15
#endif
	j	.L0

	.size	secmon_exit, . - secmon_exit


#if (defined RTL_SIM)

	.begin	literal_prefix	.DebugExceptionVector
	.section		.DebugExceptionVector.text, "ax"
	.align	4
	.global	_DebugExceptionVector
_DebugExceptionVector:

	// Exit RTL simulation with a "pass" status
	movi	a2, 1
	CALL	set_diag_status_fast

	.end	literal_prefix
	.size	_DebugExceptionVector, . - _DebugExceptionVector

#endif

