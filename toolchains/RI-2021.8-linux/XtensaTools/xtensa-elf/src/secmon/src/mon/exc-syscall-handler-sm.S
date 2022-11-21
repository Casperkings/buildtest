// Copyright (c) 1999-2021 Cadence Design Systems, Inc.
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

// exc-syscall-handler-sm.S -- secure monitor syscall instruction handler


/*
 * The SYSCALL instruction is typically used to implement system calls.
 * By convention, register a2 identifies the requested system call.
 * Typically, other parameters are passed in registers a3 and up,
 * and results are returned in a2.
 *
 * The Xtensa windowed ABI reserves the value zero of register a2
 * as a request to force register windows to the stack.  The call0 ABI,
 * which has no equivalent operation, reserves this value as a no-op.
 *
 * Generally, only code that traverses the stack in unusual ways needs
 * to force (spill) register windows to the stack.  In generic C or C++,
 * there are four cases, and they all use the standard SYSCALL mechanism:
 *
 * 1. C++ exceptions
 * 2. setjmp and longjmp
 * 3. functions using the GNU extension "__builtin_return_address"
 * 4. functions using the GNU extension "nonlocal goto"
 *
 * NOTE:  Specific operating systems often need to spill register windows
 * to the stack in other situations such as context-switching, passing
 * Unix-like signals to threads, displaying stack tracebacks, etc.
 * They may choose to use the SYSCALL mechanism to do so, or use other
 * means such as calling xthal_window_spill() or other methods.
 *
 * If you want to handle other system calls, you can modify this file, or
 * use the C version of it in exc-syscall-handler.c .  The Xtensa ABIs only
 * define system call zero; the behavior of other system calls is up to you.
 */

#include <xtensa/coreasm.h>
#include "xtos-internal.h"

#include "xtensa/secmon-defs.h"


	.global	_need_user_vector_	// pull-in real user vector (tiny LSP)


	// The SYSCALL handler is entered when the processor
	// executes the SYSCALL instruction.
	// By convention, the system call to execute is specified in a2.
	//
	// On entry: a1 = secure SP, with frame allocated (not yet checked)
	//           a2 = exccause
	//           a3 = clobbered
	//           a4 = unsaved
	//           old a1-a3 saved to secure stack frame

	.text
	.align	4
	.global	secmon_syscall_handler
secmon_syscall_handler:

	//  HERE:  a2, a3 have been saved to the exception stack frame allocated
	//  with a1 (sp).
	rsr.epc1	a3

	// If the SYSCALL instruction was the last instruction in the body of
	// a zero-overhead loop, and the loop will execute again, we would
	// normally need to decrement the loop count and resume execution at 
	// the head of the loop, as is done in XTOS.
	//
	// However, as it could present a potential security issue, this logic
	// has been removed.  It should not impact correctness as any syscall
	// instruction should be followed by code to check its return value.

	l32i	a2, a1, UEXC_a2	// get the system call number
	addi	a3, a3, 3	// increment EPC to skip the SYSCALL instruction

	s32i	a4, a1, UEXC_a4		// a4 not saved by user vector
	s32i	a5, a1, UEXC_a5		// a5 not saved by user vector

	// Secure monitor: handle syscalls to register/unregister handlers
	movi	a5, SECMON_SYSCALL_MASK
	movi	a4, SECMON_SYSCALL_FIRST
	and	a5, a2, a5

	bne	a5, a4, .Lunhandled_secure_syscall
	wsr.epc1	a3	// update EPC1 past SYSCALL

	// Validate SP is still secure before continuing.  Done after unhandled 
	// syscall check for performance reasons.
	movi	a3, _stack_sentry
	addi	a3, a3, 96          // syscall handling requires 3 32B stack frames
	bltu	a1, a3, secmon_stack_overflow
	movi	a3, _heap_sentry
	bgeu	a1, a3, secmon_stack_overflow

	// Secure Monitor: save remaining state so syscall can be handled in C
	//
	// NOTE: some SRs are re-read here to keep the fast-path optimized;
	// performace of registering secure handlers is not time-critical.
	//
	// NOTE: Windowed ABI (syscall 0) not handled.

.Lsyscall_pre_save:
	//  HERE:  a2-a5 have been saved to exception stack frame allocated with a1 (sp).

	//  Set the following PS fields (all other state is preserved):
	//	WOE      = 0 (secure monitor is always call0)
	//	CALLINC  = 0 (secure monitor is always call0)
	//	OWB      = 0  (actual value is a don't care)
	//	UM       = 1
	//	EXCM     = 0
	//	INTLEVEL = EXCM_LEVEL
	rsr.ps	a4
	movi	a3, ~(PS_WOE|PS_CALLINC_MASK|PS_OWB_MASK|PS_EXCM|PS_INTLEVEL_MASK)
	movi	a5, (PS_UM|PS_INTLEVEL(XCHAL_EXCM_LEVEL))
	and	a3, a4, a3
	or	a3, a5, a3
	wsr.ps	a3
	rsr.epc1	a5

	//  Secure Monitor: Save and restore PS/EPC1 across handler
	s32i	a4, a1, UEXC_ps
	s32i	a5, a1, UEXC_pc

	s32i	a0, a1, UEXC_a0		// save the rest of the registers
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

#if XCHAL_HAVE_LOOPS
	rsr.lcount	a13
	rsr.lbeg	a14
	rsr.lend	a15
	s32i	a13, a1, UEXC_lcount
	s32i	a14, a1, UEXC_lbeg
	s32i	a15, a1, UEXC_lend
#endif

	rsr.sar	a12
	s32i	a12, a1, UEXC_sar

#if XCHAL_HAVE_MAC16
	rsr.acclo	a13
	rsr.acchi	a14
	s32i	a13, a1, UEXC_acclo
	s32i	a14, a1, UEXC_acchi
#endif

	// Secure Monitor: invoke syscall handler (in C) here (a2 still contains syscall ID)
	l32i	a3, a1, UEXC_a3
	l32i	a4, a1, UEXC_a4
	call0	handle_syscall		// preserve a2 -- contains return code

	// Secure Monitor: restore full state after C handler completes
.Lsyscall_post_restore:

	//  Three temp registers are required for this code to be optimal (no interlocks) in
	//  T2xxx microarchitectures with 7-stage pipe; otherwise only two
	//  registers would be needed.
	//
#if XCHAL_HAVE_LOOPS
	l32i	a13, a1, UEXC_lcount
	l32i	a14, a1, UEXC_lbeg
	l32i	a15, a1, UEXC_lend
	wsr.lcount	a13
	wsr.lbeg	a14
	wsr.lend	a15
#endif

#if XCHAL_HAVE_MAC16
	l32i	a13, a1, UEXC_acclo
	l32i	a14, a1, UEXC_acchi
	wsr.acclo	a13
	wsr.acchi	a14
#endif
	l32i	a15, a1, UEXC_sar
	wsr.sar	a15

	l32i	a0, a1, UEXC_a0

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
	wsr.epc1	a3

	// EXCM should still be set, but this could re-enable windowing; do it last
	l32i	a3, a1, UEXC_ps
	wsr.ps	a3

	// Do not restore a2 -- contains syscall return code

	rsync

.Lhandled_secure_syscall:
# if XCHAL_HAVE_EXCLUSIVE
	// Clear exclusive monitors.
	clrex
# endif

	movi	a3, 0
	wsr.excsave1	a3		// wipe excsave1
	l32i	a3, a1, UEXC_a3		// restore a3
	l32i	a1, a1, UEXC_vpri	// restore SP
	rfe_rfue

	// Here: a2 = syscall ID
	//       a3 = instruction after syscall (epc1 not yet updated)
	//       a4 = clobbered but saved to stack
	//       a5 = clobbered but saved to stack
.Lunhandled_secure_syscall:
	l32i	a5, a1, UEXC_a5		// restore a5
	movi	a4, secmon_nsm_syscall_handler
	l32i	a4, a4, 0
	beqz	a4, .Lunhandled_syscall
	rsr.exccause	a2		// non-secure dispatch expects exccause in a2
					// do not update epc1.  a3 can be clobbered 
					// (nonsecure syscall handling increments PC)
	and	a3, a4, a4
	l32i	a4, a1, UEXC_a4		// a4 must be restored prior to jump
	jx	a3

.Lunhandled_syscall:
	wsr.epc1	a3		// update EPC1 past SYSCALL
	l32i	a4, a1, UEXC_a4		// restore a4
	l32i	a3, a1, UEXC_a3		// restore a3
	movi	a2, 0
	wsr.excsave1    a2		// wipe excsave1
	movi	a2, -1 /*ENOSYS*/	// system call not supported
	l32i	a1, a1, UEXC_vpri	// restore SP
	rfe_rfue

	.size	secmon_syscall_handler, . - secmon_syscall_handler


	.data

	// non-secure syscall handler (disabled by default)
	.global	secmon_nsm_syscall_handler
	.align	4

secmon_nsm_syscall_handler:
	.word	0

