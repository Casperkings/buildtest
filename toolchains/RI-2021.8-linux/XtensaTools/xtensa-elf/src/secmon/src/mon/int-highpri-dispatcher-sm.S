// Copyright (c) 2004-2021 Cadence Design Systems, Inc.
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

// int-highpri-dispatcher-sm.S -- High-Priority Interrupt Dispatcher Template


//
// This file allows writing high-priority interrupt handlers in C,
// providing convenience at a significant cost in performance.
//
// By default, this file is included by inth-template.S .
// The default Makefile defines _INTERRUPT_LEVEL when assembling
// inth-template.S for each medium and high priority interrupt level.
//
// To use this template file, define a macro called _INTERRUPT_LEVEL
// to be the interrupt priority level of the vector, then include this file.


#include <xtensa/coreasm.h>
#include "xtos-internal.h"


#if XCHAL_HAVE_INTERRUPTS && (XCHAL_HAVE_XEA1 || XCHAL_HAVE_XEA2)

#define INTERRUPT_MASK		XCHAL_INTLEVEL_MASK(_INTERRUPT_LEVEL)
#define SINGLE_INTERRUPT	((INTERRUPT_MASK & (INTERRUPT_MASK - 1)) == 0)
#define SINGLE_INT_NUM		XCHAL_INTLEVEL_NUM(_INTERRUPT_LEVEL)


#define INTLEVEL_N_MASK		INTERRUPT_MASK	// mask of interrupts at this priority
#define INTLEVEL_N_NUM		SINGLE_INT_NUM	// interrupt number if there is only one
#define INTLEVEL_N_BELOW_MASK	XCHAL_INTLEVEL_ANDBELOW_MASK(_INTERRUPT_LEVEL)

//  Indicates whether there are multiple interrupts at this interrupt
//  priority, ie. mapped to this interrupt vector.
//  If there is only one, its number is INTLEVEL_N_NUM

#define MULTIPLE_INTERRUPTS	(!SINGLE_INTERRUPT)

//  Current compilers only use ACC (not MRn) when MAC16 is enabled, 
//  so you can leave this 0 for performance:

#define SECMON_SAVE_ALL_MAC16   0   // set to save/restore MAC16 MRn registers

//
//  High priority interrupt stack frame:
//

STRUCT_BEGIN
STRUCT_FIELD (long,4,HESF_,SAR)
//STRUCT_FIELD (long,4,HESF_,WINDOWSTART)
//STRUCT_FIELD (long,4,HESF_,WINDOWBASE)
STRUCT_FIELD (long,4,HESF_,EPC1)
STRUCT_FIELD (long,4,HESF_,EXCCAUSE)
STRUCT_FIELD (long,4,HESF_,EXCVADDR)
STRUCT_FIELD (long,4,HESF_,EXCSAVE1)
STRUCT_FIELD (long,4,HESF_,PS)
STRUCT_FIELD (long,4,HESF_,EPSN)
STRUCT_FIELD (long,4,HESF_,EPCN)
//STRUCT_FIELD (long,4,HESF_,VPRI)	/* (XEA1 only) */
#if XCHAL_HAVE_MAC16
STRUCT_FIELD (long,4,HESF_,ACCLO)
STRUCT_FIELD (long,4,HESF_,ACCHI)
STRUCT_AFIELD(long,4,HESF_,MR, 4)
#endif
#if XCHAL_HAVE_LOOPS
STRUCT_FIELD (long,4,HESF_,LCOUNT)
STRUCT_FIELD (long,4,HESF_,LBEG)
STRUCT_FIELD (long,4,HESF_,LEND)
#endif
STRUCT_AFIELD(long,4,HESF_,AREG, 64)	/* address registers ar0..ar63 */
#define HESF_AR(n)	HESF_AREG+((n)*4)
STRUCT_END(HighPriFrame)
#define HESF_TOTALSIZE	HighPriFrameSize+32	/* 32 bytes for interrupted code's save areas under SP */


#if XCHAL_HAVE_XEA1 && HAVE_XSR		/* could be made true for T1040 and T1050 */
# error "high-priority interrupt stack frame needs adjustment if HAVE_XSR is allowed with XEA1"
#endif


#define PRI_N_STACK_SIZE	1024	/* default to 1 kB stack for each level-N handling */


	//  Allocate save area and stack:
	//  (must use .bss, not .comm, because the subsequent .set does not work otherwise)
	.section	.bss, "aw"
	.align	16
LABEL(_Pri_,_Stack):	.space	PRI_N_STACK_SIZE + HESF_TOTALSIZE

#if HAVE_XSR
	.data
	.align	4
	.global	LABEL(_Pri_,_HandlerAddress)
LABEL(_Pri_,_HandlerAddress):	.space 4
#endif


	.text
	.align	4
	.global	LABEL(_Level,FromVector)
LABEL(_Level,FromVector):
	movi	a2, LABEL(_Pri_,_Stack) + PRI_N_STACK_SIZE	// get ptr to save area
	// interlock

	//  Save a few registers so we can do some work:
	s32i	a0,  a2, HESF_AR(0)

	// Secure Monitor: Do not use excsaveN to maintain handler address,
	// as it is not protected from nonsecure code modifying it.
	readsr	excsave _INTERRUPT_LEVEL a0		// get saved a2
	s32i	a1,  a2, HESF_AR(1)
	s32i	a0,  a2, HESF_AR(2)
	s32i	a3,  a2, HESF_AR(3)
	s32i	a4,  a2, HESF_AR(4)

	//  Save/restore all exception state
	//  (IMPORTANT:  this code assumes no general exceptions occur
	//   during the execution of this dispatcher until this state
	//   is completely saved and from the point it is restored.)
	//
	//  Exceptions that may normally occur within the C handler
	//  include window exceptions (affecting EPC1), alloca exceptions
	//  (affecting EPC1/EXCCAUSE and its handling uses EXCSAVE1),
	//  and possibly others depending on the particular C handler
	//  (possibly needing save/restore of EXCVADDR; and EXCVADDR
	//   is also possibly corrupted by any access thru an auto-refill
	//   way on a processor with a full MMU).
	//
	rsr.epc1	a3
	rsr.exccause	a4
	s32i	a3, a2, HESF_EPC1
	s32i	a4, a2, HESF_EXCCAUSE
	rsr.excvaddr	a3
	s32i	a3, a2, HESF_EXCVADDR
	rsr.excsave1	a4
	s32i	a4, a2, HESF_EXCSAVE1

	//  Secure Monitor: Windowed ABI save/restore code removed for clarity

	s32i	a5,  a2, HESF_AR(5)
	s32i	a6,  a2, HESF_AR(6)
	s32i	a7,  a2, HESF_AR(7)

1:
	s32i	a8,  a2, HESF_AR(8)
	s32i	a9,  a2, HESF_AR(9)
	s32i	a10, a2, HESF_AR(10)
	s32i	a11, a2, HESF_AR(11)
	s32i	a12, a2, HESF_AR(12)
	s32i	a13, a2, HESF_AR(13)
	s32i	a14, a2, HESF_AR(14)
	s32i	a15, a2, HESF_AR(15)

	movi	a1, LABEL(_Pri_,_Stack) + PRI_N_STACK_SIZE	// get ptr to save area (is also initial stack ptr)
	movi	a0, 0						// mark start of call frames in stack

	//  Critical state saved.
	//  Secure Monitor: window support will remain disabled; C dispatchers in secure space.
	//  Secure Monitor: Save PS.PNSM/EPCn/EPSn in case user handler modifies them.
	rsr.ps	a4
	s32i	a4, a2, HESF_PS
	readsr	epc _INTERRUPT_LEVEL a4
	s32i	a4, a2, HESF_EPCN
	readsr	eps _INTERRUPT_LEVEL a4
	s32i	a4, a2, HESF_EPSN

	//  We now have a C-coherent stack.
	//  Still have to fix PS while making sure interrupts stay disabled
	//  at the appropriate level (ie. level 2 and below are disabled in this case).

	//  Secure Monitor: XEA1 code removed for clarity

	//  Load PS for C code, clear EXCM
	movi	a2, PS_UM + _INTERRUPT_LEVEL	// UM=1, INTLEVEL=N, EXCM=0, RING=0
	wsr.ps	a2
	rsync

	//  Okay, window exceptions can now happen (although we have to call
	//  deep before any will happen because we've reset WINDOWSTART).

	//  Save other state that might get clobbered by C code:

//////////////////  COMMON DISPATCH CODE BEGIN

	rsr.sar	a14
	s32i	a14, a1, HESF_SAR
#if XCHAL_HAVE_LOOPS
	rsr.lcount	a14
	s32i		a14, a1, HESF_LCOUNT
	rsr.lbeg	a14
	s32i		a14, a1, HESF_LBEG
	rsr.lend	a14
	s32i		a14, a1, HESF_LEND
#endif
#if XCHAL_HAVE_MAC16
	rsr.acclo	a14
	s32i		a14, a1, HESF_ACCLO
	rsr.acchi	a14
	s32i		a14, a1, HESF_ACCHI
#if SECMON_SAVE_ALL_MAC16
	rsr.m0		a14
	s32i		a14, a1, HESF_MR + 0
	rsr.m1		a14
	s32i		a14, a1, HESF_MR + 4
	rsr.m2		a14
	s32i		a14, a1, HESF_MR + 8
	rsr.m3		a14
	s32i		a14, a1, HESF_MR + 12
#endif
#endif

#if MULTIPLE_INTERRUPTS		/* > 1 interrupts at this priority */	// _split_ multi_setup
#define TABLE_OFS	0

	rsr.interrupt	a15		// mask of pending interrupts
	rsr.intenable	a12		// mask of enabled interrupts
	movi	a13, INTLEVEL_N_MASK	// mask of interrupts at this priority level
	and	a15, a15, a12
	and	a15, a15, a13		// enabled & pending interrupts at this priority
	_beqz	a15, LABEL(Pri_,_spurious)	// handle spurious interrupts (eg. level-trig.)
LABEL(Pri_,_loop):				// handle all enabled & pending interrupts
	neg	a14, a15
	and	a14, a14, a15		// single-out least-significant bit set in mask
	wsr.intclear	a14		// clear if edge-trig. or s/w or wr/err (else no effect)

	//  Compute pointer to interrupt table entry, given mask a14 with single bit set:

# if XCHAL_HAVE_NSA
	movi	a12, xtos_interrupt_table - (32-XCHAL_NUM_INTERRUPTS)*8
	nsau	a14, a14		// get index of bit in a14, numbered from msbit
	addx8	a12, a14, a12
# else /* XCHAL_HAVE_NSA */
	movi	a12, xtos_interrupt_table	// pointer to interrupt table
	bltui	a14, 0x10000, 1f	// in 16 lsbits? (if so, check them)
	addi	a12, a12, 16*8		// no, index is at least 16 entries further
	// (the above ADDI expands to an ADDI+ADDMI sequence, +128 is outside its range)
	extui	a14, a14, 16,16		// shift right upper 16 bits
1:
	bltui	a14, 0x100, 1f		// in 8 lsbits? (if so, check them)
	addi	a12, a12, 8*8		// no, index is at least 8 entries further
	srli	a14, a14, 8		// shift right upper 8 bits
1:
	bltui	a14, 0x10, 1f		// in 4 lsbits? (if so, check them)
	addi	a12, a12, 4*8		// no, index is at least 4 entries further
	srli	a14, a14, 4		// shift right 4 bits
1:
	bltui	a14, 0x4, 1f		// in 2 lsbits? (if so, check them)
	addi	a12, a12, 2*8		// no, index is at least 2 entries further
	srli	a14, a14, 2		// shift right 2 bits
1:
	bltui	a14, 0x2, 1f		// is it the lsbit?
	addi	a12, a12, 1*8		// no, index is one entry further
1:					// done! a12 points to interrupt's table entry
# endif /* XCHAL_HAVE_NSA */

#else /* !MULTIPLE_INTERRUPTS */

# if XCHAL_HAVE_NSA
#  define TABLE_OFS	8 * (XCHAL_NUM_INTERRUPTS - 1 - INTLEVEL_N_NUM)
# else
#  define TABLE_OFS	8 * INTLEVEL_N_NUM
# endif

	movi	a13, INTLEVEL_N_MASK	// (if interrupt is s/w or edge-triggered or write/err only)
	movi	a12, xtos_interrupt_table	// get pointer to its interrupt table entry
	wsr.intclear	a13		// clear the interrupt (if s/w or edge or wr/err only)

#endif /* ifdef MULTIPLE_INTERRUPTS */

	l32i	a13, a12, TABLE_OFS + 0	// get pointer to handler from table entry
	l32i	a2, a12, TABLE_OFS + 4	// pass single argument to C handler
	callx0	a13			// call interrupt's C handler
	//  Secure Monitor: Windowed ABI save/restore code removed for clarity

#if MULTIPLE_INTERRUPTS		/* > 1 interrupts at this priority */
	rsr.interrupt	a15		// get pending interrupts
	rsr.intenable	a12		// get enabled interrupts
	movi	a13, INTLEVEL_N_MASK	// get mask of interrupts at this priority level
	and	a15, a15, a12
	and	a15, a15, a13		// pending+enabled interrupts at this priority
	_bnez	a15, LABEL(Pri_,_loop)	// if any remain, dispatch one
LABEL(Pri_,_spurious):
#endif /* MULTIPLE_INTERRUPTS */

	//  Restore everything, and return.

#if XCHAL_HAVE_EXCLUSIVE
	// Clear exclusive monitors.
	clrex
#endif

	//  Three temp registers are required for this code to be optimal (no interlocks) in
	//  T2xxx microarchitectures with 7-stage pipe; otherwise only two
	//  registers would be needed.
	//
#if XCHAL_HAVE_LOOPS
	l32i	a13, a1, HESF_LCOUNT
	l32i	a14, a1, HESF_LBEG
	l32i	a15, a1, HESF_LEND
	wsr.lcount	a13
	wsr.lbeg	a14
	wsr.lend	a15
#endif

#if XCHAL_HAVE_MAC16
	l32i	a13, a1, HESF_ACCLO
	l32i	a14, a1, HESF_ACCHI
	wsr.acclo	a13
	wsr.acchi	a14
#if SECMON_SAVE_ALL_MAC16
	l32i	a13, a1, HESF_MR + 0
	l32i	a14, a1, HESF_MR + 4
	wsr.m0	a13
	wsr.m1	a14
	l32i	a13, a1, HESF_MR + 8
	l32i	a14, a1, HESF_MR + 12
	wsr.m2	a13
	wsr.m3	a14
#endif
#endif
	l32i	a15, a1, HESF_SAR
	wsr.sar	a15

//////////////////  COMMON DISPATCH CODE END

	//  Secure Monitor: Restore PS.PNSM/EPCn/EPSn in case user handler modifies them.
	l32i	a4, a1, HESF_EPCN
	l32i	a3, a1, HESF_EPSN
	l32i	a2, a1, HESF_PS
	writesr	epc _INTERRUPT_LEVEL a4
	writesr	eps _INTERRUPT_LEVEL a3
	movi	a4, PS_PNSM_MASK
	and	a2, a2, a4			// Saved PS.PNSM

	//  Load PS for interrupt exit, set EXCM:
	movi	a4, 0x00030 + _INTERRUPT_LEVEL	// WOE=0, CALLINC=0, UM=1, INTLEVEL=N, EXCM=1, RING=0
	or	a2, a2, a4
	wsr.ps	a2				// update PS to disable window exceptions, etc as per above
	rsync

	//  NOTE:  here for XEA1, restore INTENABLE etc...

	//  Secure Monitor: Windowed ABI save/restore code removed for clarity

	l32i	a4, a1, HESF_AR(4)		// restore general registers
	l32i	a5, a1, HESF_AR(5)
	l32i	a6, a1, HESF_AR(6)
	l32i	a7, a1, HESF_AR(7)
	l32i	a8, a1, HESF_AR(8)
	l32i	a9, a1, HESF_AR(9)
	l32i	a10, a1, HESF_AR(10)
	l32i	a11, a1, HESF_AR(11)
	l32i	a12, a1, HESF_AR(12)
	l32i	a13, a1, HESF_AR(13)
	l32i	a14, a1, HESF_AR(14)
	l32i	a15, a1, HESF_AR(15)

	//  Restore exception state:
	l32i	a2, a1, HESF_EPC1
	l32i	a3, a1, HESF_EXCCAUSE
	wsr.epc1	a2
	wsr.exccause	a3
	l32i	a2, a1, HESF_EXCVADDR
	wsr.excvaddr	a2
	l32i	a3, a1, HESF_EXCSAVE1
	wsr.excsave1	a3

	l32i	a0,  a1, HESF_AR(0)
	l32i	a2,  a1, HESF_AR(2)
	l32i	a3,  a1, HESF_AR(3)
	l32i	a1,  a1, HESF_AR(1)
	rfi	_INTERRUPT_LEVEL

	.size	LABEL(_Level,FromVector), . - LABEL(_Level,FromVector)

	//  This symbol exists solely for the purpose of being able to pull-in this
	//  dispatcher using xtos_dispatch_level<n>() routines with the tiny-rt LSP:
	.global	LABEL(_Level,HandlerLabel)
	.set LABEL(_Level,HandlerLabel), 0

#endif /* XCHAL_HAVE_INTERRUPTS */

