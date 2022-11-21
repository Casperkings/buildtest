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

// int-lowpri-dispatcher-sm.S -- Level-one interrupt dispatcher (user vectored handler)


#include <xtensa/coreasm.h>
#include "xtos-internal.h"
#include "xea2/interrupt-pri.h"

#if XCHAL_HAVE_XEA2 && XCHAL_HAVE_EXCEPTIONS && XCHAL_HAVE_INTERRUPTS


	/*
	 *  Macros to slightly reduce the number of #if statements in the code:
	 */

/*  This is set (for #if only) if there is only ONE interrupt configured at level one:  */
#if (defined XCHAL_INTLEVEL1_NUM)
# define XTOS_SINGLE_INT	1
#else
# define XTOS_SINGLE_INT	0
#endif

/*  Simplify the #if's around saving and restoring of SAR ('#' is a comment char):  */
#if ((XTOS_SUBPRI_ORDER == XTOS_SPO_ZERO_LO) || (XTOS_INT_FAIRNESS && XTOS_SUBPRI)) && !XTOS_SINGLE_INT
#  define NEEDSAR		/* need SAR saved early w/ints locked */
#  define LATESAR	#	/* need SAR saved late w/ints unlocked */
#else
#  define NEEDSAR	#	/* need SAR saved early w/ints locked */
#  define LATESAR		/* need SAR saved late w/ints unlocked */
#endif

/*  Simplify the #if's around fairness-specific code ('#' is a comment char):  */
#if XTOS_INT_FAIRNESS
#  define IFFAIR		/* for code enabled only for fairness */
#  define NOFAIR	#	/* for code enabled only without fairness */
#else
#  define IFFAIR	#	/* for code enabled only for fairness */
#  define NOFAIR		/* for code enabled only without fairness */
#endif
/*
 *  Note about implementation of "fairness".
 *  The fairness mask is meant to represent, among a set of interrupts pending
 *  at the same time, which ones haven't yet been serviced.  It's to avoid,
 *  among interrupts simultaneously pending at the same priority level,
 *  servicing an interrupt more than once before servicing another.
 *  Without the mask, if we for example just always serviced the lowest
 *  numbered pending interrupt, then a lower numbered interrupt might get
 *  serviced many times before servicing a higher numbered one, even though
 *  they're at the same priority and pending at the same time -- thus an
 *  "unfair" situation.
 *  The typical way to do this fairly is to loop looking at each interrupt
 *  number in sequence, determining whether that interrupt needs servicing,
 *  and to repeat looping if at least one interrupt was serviced (or at
 *  one remains pending).
 *  Using the mask is faster, as we only look at pending interrupts,
 *  instead of looping looking at all interrupts.
 */


	// On entry: a1 = secure SP, with frame allocated
	//           a2 = exccause
	//           a3 = clobbered
	//           a4 = unsaved
	//           old a1-a3 saved to secure stack frame

	.global	_need_user_vector_	// pull-in real user vector (tiny LSP)

	.text
	.align	4
	.global	secmon_l1int_handler
secmon_l1int_handler:
	//  HERE:  a2, a3 have been saved to exception stack frame allocated with a1 (sp).

	// Validate SP is still secure
	movi	a2, _stack_sentry
	addi	a2, a2, 32          // interrupt handler may need a 32B stack frame
	bltu	a1, a2, secmon_stack_overflow
	movi	a2, _heap_sentry
	bgeu	a1, a2, secmon_stack_overflow

	//  Set PS fields:
	//	PNSM     = 1 (L1 interrupts must have come from nonsecure code)
	//	EXCM     = 0
	//	WOE      = 0 (secure monitor is always call0)
	//	UM       = 1
	//	INTLEVEL = XCHAL_EXCM_LEVEL
	//	CALLINC  = 0 (secure monitor is always call0)
	//	OWB      = 0  (actual value is a don't care)

	movi	a2, PS_PNSM|PS_UM|PS_INTLEVEL(XCHAL_EXCM_LEVEL)
	rsr.epc1	a3
	xsr.ps	a2

	//  Secure Monitor: Windowed ABI code/comments removed

	//  Secure Monitor: Save and restore PS/EPC1 across handler
	s32i	a3, a1, UEXC_pc
	s32i	a2, a1, UEXC_ps

	//  Secure Monitor: a3: table of active nonsecure interrupt handlers
	movi	a3, secmon_nsm_interrupt_table

	s32i	a0, a1, UEXC_a0		// save the rest of the registers
	s32i	a4, a1, UEXC_a4		// a4 not saved by user vector
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


	// Secure Monitor -- Dispatch L1 interrupts at EXCMLEVEL to prevent
	// nonsecure interrupts at EXCMLEVEL from preempting us.

	/*****************  Dispatch low-priority interrupts to service  *****************/

#if XTOS_SINGLE_INT
	/*
	 *  Only one interrupt is configured to map to this vector.
	 *  This simplifies the code considerably -- no checking and resolving of INTERRUPT
	 *  register required.  Just call the handler and exit.
	 *
	 *  (With INTENABLE register virtualization, the simplification is
	 *   not as great, and not implemented separately above.)
	 */


# define XTOS_SINGLE_INT_NUM	XCHAL_INTLEVEL1_NUM
# define XTOS_SINGLE_INT_MASK	XCHAL_INTLEVEL1_MASK
# define XTOS_SINGLE_INT_CLEAR	((XTOS_SINGLE_INT_MASK & XCHAL_INTCLEARABLE_MASK) != 0)
# if XTOS_SINGLE_INT_CLEAR
	movi	a13, XCHAL_LOWPRI_MASK		// bit to clear in INTERRUPT register
# endif
	//  Get pointer to interrupt table entry for this vector's only interrupt:
	movi	a12, xtos_interrupt_table + MAPINT(XTOS_SINGLE_INT_NUM)*XIE_SIZE
	addi	a4, a3, XTOS_SINGLE_INT_NUM
	l8ui	a4, a4, 0       // a4 = 1 for nonsecure handler, 0 for secure handler
# if XTOS_SINGLE_INT_CLEAR
	bnez	a4, .L1_skip_intclear_3
	wsr.intclear	a13			// clear interrupt pending bit (if software or external-edge-triggered or write-error)
.L1_skip_intclear_3:
# endif



#else /* ie. if !XTOS_VIRTUAL_INTENABLE && !XTOS_SINGLE_INT */
	/*
	 *  Here, the INTENABLE register is NOT virtualized.  There are no xtos_enabled
	 *  or xtos_vpri_enabled global variables to track.  INTENABLE simply controls
	 *  which interrupts are active (eg. enabled once a handler is registered).
	 *
	 *  NOTE:  To ensure its coherency, it is still important to only modify the
	 *  INTENABLE register when interrupts at XTOS_LOCK_LEVEL and below are disabled,
	 *  that it never be modified by interrupts at levels above XTOS_LOCK_LEVEL,
	 *  and that it never be modified when the current interrupt level is below
	 *  XTOS_LOCK_LEVEL.  This is because modifications to INTENABLE generally
	 *  require an RSR/modify/WSR sequence to modify only selected bits.
	 *
	 *  NOTE:  Reading the INTERRUPT register *must* be done at PS.INTLEVEL <= 1
	 *  otherwise we might incorrectly see higher priority interrupts.
	 *
	 *  This option implies XEA2, because XEA1 always requires INTENABLE virtualization.
	 *  This option also implies SUBPRI is zero (no interrupt sub-prioritization in software).
	 */


	rsr.interrupt	a15			// interrupts pending
	rsr.intenable	a13			// interrupts enabled (directly; no virtualization)
	movi	a14, xtos_interrupt_table - IFNSA( (32-XCHAL_NUM_INTERRUPTS)*XIE_SIZE, 0 )
NEEDSAR	rsr.sar	a12
	and	a15, a15, a13			// a15 = INTERRUPT & INTENABLE

	_beqz	a15, spurious_int		// no interrupt to handle (spurious interrupt)
NEEDSAR	s32i	a12, a1, UEXC_sar

IFFAIR	movi	a2, -1				// initial fairness mask

.L1_loop0:
	//  Entry:
	//	a12 = (undefined)
	//	a13 = (undefined)
	//	a14 = interrupt table adjusted base (not used here)
	//	a15 = non-zero mask of interrupt bits to consider handling
	//  Exit:
	//	a12 = index
	//	a13 = (clobbered)
	//	a14 = (preserved)
	//	a15 = single bit corresponding to index
	//
	indexmask_int	a12, a15, a14_UNUSED, a13

	//  a12 = index of highest priority pending+enabled interrupt, to be processed.
	//  a15 = (1 << a12), ie. bit corresponding to interrupt to be processed.
IFFAIR	xor	a2, a2, a15		// update fairness mask - mask out this interrupt until recycling mask
# if XCHAL_HAVE_NSA
	neg	a4, a12
	addi	a4, a4, 31      // reverse bit order of index if nsau is present;
	add	a4, a4, a3      // nonsecure table ordering is always increasing
#else
	add	a4, a12, a3
#endif
	l8ui	a4, a4, 0       // a4 = 1 for nonsecure handler, 0 for secure handler
	bnez	a4, .L1_skip_intclear_4
	wsr.intclear	a15		// clear interrupt (if software or external edge-triggered or write-error)
.L1_skip_intclear_4:

	addx8	a12, a12, a14		// a12 = address in interrupt table for given interrupt number

.L1_loop1:
	//  a12 now contains pointer to interrupt table entry for interrupt to be processed

	//  HERE:
	//	a12 = pointer to interrupt entry in table
	//	a13, a15 = available for use
	//	a14 = available for use if virtual INTENABLE, else is pointer to interrupt table


#endif /* !XTOS_VIRTUAL_INTENABLE && !XTOS_SINGLE_INT */

	//  HERE:  a12 = pointer to interrupt entry in table


	//  Secure Monitor: a4 = 1 for nonsecure handlers.
	beqz	a4, .L1_call_secure_handler

	//  For nonsecure handling:
	//  1. Skip further state save 
	//  2. Bypass callx0 to handler
	//  3. Restore remaining state

# if XCHAL_HAVE_EXCLUSIVE
	// Clear exclusive monitors.
	clrex
# endif

NEEDSAR	l32i	a2, a1, UEXC_sar
NEEDSAR	wsr.sar	a2

	//  4. RFE to nonsecure code
	l32i	a2, a12, XIE_HANDLER	// a2 = address of nonsecure interrupt handler
	movi	a0, secmon_dispatch_L1int_to_nsm
	jx	a0


.L1_call_secure_handler:

	// (Possible enhancement: do at higher-level, to avoid doing it all the time? !?!?!?)
	save_loops_mac16	a1, a13, a15	// save LOOP & MAC16 regs, if configured

LATESAR	rsr.sar	a15

	l32i	a13, a12, XIE_HANDLER	// a13 = address of interrupt handler
LATESAR	s32i	a15, a1, UEXC_sar

	mov	a14, a2			// save fairness mask
	mov	a15, a3			// save secmon_nsm_interrupt_table
	l32i	a2, a12, XIE_ARG	// first arg
	mov	a3, a1			// second arg, exception stack frame

	callx0  a13			// call secure interrupt handler
	mov	a2, a14			// restore fairness mask
	mov	a3, a15			// restore secmon_nsm_interrupt_table

	// (Possible enhancement: do at higher-level, to avoid doing it all the time? !?!?!?)
	restore_loops_mac16	a1, a13, a14, a15	// restore LOOP & MAC16 regs, if configured

LATESAR	l32i	a12, a1, UEXC_sar


#if XTOS_SINGLE_INT

# undef NEEDSAR
# define NEEDSAR

#else /* ie.  if !XTOS_VIRTUAL_INTENABLE && !XTOS_SINGLE_INT */
	/*  Here, INTENABLE register is NOT virtualized (implies XEA2).  */

	rsr.interrupt	a15			// interrupts pending
	rsr.intenable	a13			// interrupts enabled (directly; no virtualization)
	movi	a14, xtos_interrupt_table - IFNSA( (32-XCHAL_NUM_INTERRUPTS)*XIE_SIZE, 0 )
LATESAR	wsr.sar	a12
	and	a15, a15, a13			// a15 = INTERRUPT & INTENABLE

	//  a15 now contains the remaining pending+enabled interrupts.
	//  NOTE:  we MUST NOT consider interrupts potentially already being handled
	//  by another interrupt handler that we pre-empted.
	//  So we masked with saved vpri, ie. the set of interrupts enabled when we entered
	//  this handler, ie. the set of interrupts that can pre-empt the previous context.
NOFAIR	_bnez	a15, .L1_loop0			// more interrupt(s) to handle
IFFAIR	_bnez	a15, preloop			// more interrupt(s) to handle


	//  NOTE:
	//  Register allocation is why we didn't restore *HERE* the loop regs, MAC16, SAR, etc.
	//  (at least part of the reason)
	//  We only have one registers (a15), however with 7-stage pipe, three registers
	//  are required to avoid interlocks.  We could get 2 more registers at 1 cycle each [now only one?],
	//  but it isn't obvious whether paying these extra cycles are worth it...

NEEDSAR	l32i	a12, a1, UEXC_sar
#endif /* !XTOS_VIRTUAL_INTENABLE && !XTOS_SINGLE_INT */


	/***************************/

	//  Now exit the handler.

	/*
	 *  Leave interrupts disabled while returning from the pseudo-CALL setup above,
	 *  for the same reason they were disabled while doing the pseudo-CALL:
	 *  this sequence restores SP such that it doesn't reflect the allocation
	 *  of the exception stack frame, which is still needed to return from
	 *  the exception.
	 */

spurious_int:

# if XCHAL_HAVE_EXCLUSIVE
	// Clear exclusive monitors.
	clrex
# endif

	movi	a0, secmon_return_from_exc
NEEDSAR	wsr.sar	a12
	jx	a0



#if XTOS_INT_FAIRNESS
preloop:
	//  Lowering priority or recycling fairness-mask bits ...
	//  a14 = &_xtos_intstruct *or* interrupt table ptr
	//  a15 = non-zero mask of interrupt bits to consider handling

	and	a13, a15, a2		// a13 = interrupt bits to consider handling, masked for fairness
	movi	a12, -1			// (new fairness mask, all one's)
	moveqz	a2, a12, a13		// recycle fairness mask if all bits to consider are masked by fairness, and leave a15 intact
	movnez	a15, a13, a13		// otherwise set a15 = a13, ie. mask out bits for fairness (a15 is still non-zero)
	j	.L1_loop0
#endif /* XTOS_INT_FAIRNESS */

	/* FIXME: what about _LevelOneInterrupt ? */
	.size	secmon_l1int_handler, . - secmon_l1int_handler

	.data
	.align	4
	.global	secmon_nsm_interrupt_table
	.type	secmon_nsm_interrupt_table,@object


secmon_nsm_interrupt_table:
	.rept	XCHAL_NUM_INTERRUPTS
	.byte	0
	.endr

	.size	secmon_nsm_interrupt_table, . - secmon_nsm_interrupt_table

#endif /* XCHAL_HAVE_XEA2 && XCHAL_HAVE_EXCEPTIONS && XCHAL_HAVE_INTERRUPTS */

