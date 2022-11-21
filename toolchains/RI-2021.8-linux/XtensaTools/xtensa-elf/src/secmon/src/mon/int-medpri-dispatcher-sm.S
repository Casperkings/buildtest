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

// int-medpri-dispatcher-sm.S -- Medium-Priority Interrupt Dispatcher Template


//
// By default, this file is included by inth-template.S .
// The default Makefile defines _INTERRUPT_LEVEL when assembling
// inth-template.S for each medium and high priority interrupt level.
//
// To use this template file, define a macro called _INTERRUPT_LEVEL
// to be the interrupt priority level of the vector, then include this file.


#include <xtensa/coreasm.h>
#include "xtos-internal.h"


#if XCHAL_HAVE_INTERRUPTS

#define INTERRUPT_MASK		XCHAL_INTLEVEL_MASK(_INTERRUPT_LEVEL)
#define SINGLE_INTERRUPT	((INTERRUPT_MASK & (INTERRUPT_MASK - 1)) == 0)
#define SINGLE_INT_NUM		XCHAL_INTLEVEL_NUM(_INTERRUPT_LEVEL)


//  Strict non-preemptive prioritization

//
//  Medium priority interrupt stack frame:
//

STRUCT_BEGIN
STRUCT_AFIELD(long,4,MESF_,pad,ESF_TOTALSIZE/4)
STRUCT_FIELD (long,4,MESF_,EPC1)
STRUCT_FIELD (long,4,MESF_,EXCCAUSE)
STRUCT_FIELD (long,4,MESF_,EXCVADDR)
STRUCT_FIELD (long,4,MESF_,EXCSAVE1)
STRUCT_FIELD (long,4,MESF_,EPSN)
STRUCT_FIELD (long,4,MESF_,EPCN)
STRUCT_AFIELD(long,4,MESF_,AREG, 16)	/* Only need space for call0 ABI */
#define MESF_AR(n)	MESF_AREG+((n)*4)
STRUCT_END(MedPriFrame)
#define MESF_TOTALSIZE	MedPriFrameSize+32	/* 32 bytes for interrupted code's save areas under SP */

#define MED_PRI_STACK_SIZE	1024	/* default to 1 kB stack for each level-N handling */


	//  Allocate save area and stack:
	//  (must use .bss, not .comm, because the subsequent .set does not work otherwise)
	.section	.bss, "aw"
	.align	16
LABEL(_Pri_,_Stack):	.space	MED_PRI_STACK_SIZE + MESF_TOTALSIZE

#if HAVE_XSR
	.data
	.align	4
	.global	LABEL(_Pri_,_HandlerAddress)
LABEL(_Pri_,_HandlerAddress):	.space	4
#endif


	.text
	.align	4
	.global	LABEL(_Level,FromVector)
LABEL(_Level,FromVector):
	movi	a2, LABEL(_Pri_,_Stack) + MED_PRI_STACK_SIZE	// get ptr to save area
	// interlock

	//  Save a few registers so we can do some work:
	s32i	a0,  a2, MESF_AR(0)

	// Secure Monitor: Do not use excsaveN to maintain handler address,
	// as it is not protected from nonsecure code modifying it.
	readsr	excsave _INTERRUPT_LEVEL a0		// get saved a2
	s32i	a1,  a2, MESF_AR(1)
	s32i	a0,  a2, MESF_AR(2)
	s32i	a3,  a2, MESF_AR(3)

	// Secure Monitor: Use dedicated secure L2 stack
	mov	a1,  a2

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
	rsr.exccause	a2
	s32i	a3, a1, MESF_EPC1
	s32i	a2, a1, MESF_EXCCAUSE
	rsr.excvaddr	a3
	s32i	a3, a1, MESF_EXCVADDR
	rsr.excsave1	a2
	s32i	a2, a1, MESF_EXCSAVE1

	// Secure Monitor: Preserve PS.PNSM, clear PS.EXCM, set PS.UM and mask interrupts < EXCM_LEVEL
	rsr.ps	a3
	movi	a2, PS_PNSM_MASK
	s32i	a3, a1, UEXC_ps
	and	a3, a3, a2
	movi	a2, PS_UM|PS_INTLEVEL(XCHAL_EXCM_LEVEL)
	or	a3, a3, a2
	wsr.ps	a3
	rsync

	s32i	a4, a1, MESF_AR(4)
	s32i	a5, a1, MESF_AR(5)
	s32i	a6, a1, MESF_AR(6)
	s32i	a7, a1, MESF_AR(7)
	s32i	a8, a1, MESF_AR(8)
	s32i	a9, a1, MESF_AR(9)
	s32i	a10, a1, MESF_AR(10)
	s32i	a11, a1, MESF_AR(11)
	s32i	a12, a1, MESF_AR(12)
	s32i	a13, a1, MESF_AR(13)
	s32i	a14, a1, MESF_AR(14)
	s32i	a15, a1, MESF_AR(15)
	movi	a0, 0			/* terminate stack frames */

	// Secure Monitor: cannot lower interrupt level below EXCM_LEVEL
	// or nonsecure code could potentially interrupt us here.
	// Since EPCN/EPSN need to be preserved in-place during secure to
	// nonsecure transition, medium priority interrupts will be serviced
	// initally at EXCM_LEVEL in nonsecure code, which can then lower 
	// PS.INTLEVEL if needed.

	//  Secure Monitor: Save EPCn/EPSn in case user handler modifies them.
	readsr	epc _INTERRUPT_LEVEL a4
	s32i	a4, a1, MESF_EPCN
	readsr	eps _INTERRUPT_LEVEL a4
	s32i	a4, a1, MESF_EPSN

	//  Secure Monitor: a3: table of active nonsecure interrupt handlers
	movi	a3, secmon_nsm_interrupt_table

#if SINGLE_INTERRUPT  /* if only one interrupt at this priority level... */

	// Preserve the SAR, loop, and MAC16 regs.  Also, clear the interrupt.

	rsr.sar	a14
	movi	a12, INTERRUPT_MASK
	s32i	a14, a1, UEXC_sar

	addi	a4, a3, SINGLE_INT_NUM
	l8ui	a4, a4, 0	// a4 = 1 for nonsecure handler, 0 for secure handler
	bnez	a4, .L2_skip_intclear_1
	wsr.intclear	a12	// clear if edge-trig or s/w or wr/err (else no effect)
.L2_skip_intclear_1:

	save_loops_mac16	a1, a13, a14

	// Load the handler from the table, initialize two args (interrupt
	// number and exception stack frame), then call the interrupt handler.
	// Note: The callx12 preserves the original user task's a4..a15.

	movi	a12, xtos_interrupt_table +  MAPINT(SINGLE_INT_NUM)*XIE_SIZE
	beqz	a4, .Lm_call_secure_handler

	//  For nonsecure handling:
	//  1. Bypass callx0 to handler
	//  2. Restore remaining state
	//  3. RFI to nonsecure code
	l32i	a2, a12, XIE_HANDLER	// a2 = address of interrupt handler
	movi	a0, LABEL(secmon_dispatch_L,int_to_nsm)
	jx	a0

.Lm_call_secure_handler:

	l32i	a13, a12, XIE_HANDLER
	l32i	a2, a12, XIE_ARG
	mov	a15, a3			// save secmon_nsm_interrupt_table
	mov	a3, a1
	callx0	a13
	mov	a3, a15			// restore secmon_nsm_interrupt_table

#else /* > 1 interrupts at this priority level */

	// Get bit list of pending interrupts at the current interrupt priority level.
	// If bit list is empty, interrupt is spurious (can happen if a
	// genuine interrupt brings control this direction, but the interrupt
	// goes away before we read the INTERRUPT register).  Also save off
	// sar, loops, and mac16 registers.

	rsr.interrupt	a15
	rsr.intenable	a12
	movi	a13, INTERRUPT_MASK
	and	a15, a15, a12
	and	a15, a15, a13
	rsr.sar	a14
	_beqz	a15, LABEL(spurious,int)
	s32i	a14, a1, UEXC_sar
	save_loops_mac16	a1, a13, a14

	// Loop to handle all pending interrupts.

LABEL(.L1,_loop0):
	neg	a12, a15
	and	a12, a12, a15
	mov	a11, a12
	movi	a13, xtos_interrupt_table
	find_ms_setbit	a15, a12, a14, 0
	mov	a10, a15            // a10 contains ms bit set for nonsecure table index
	mapint	a15

	add	a4, a10, a3     // nonsecure table ordering is always increasing
	l8ui	a4, a4, 0       // a4 = 1 for nonsecure handler, 0 for secure handler
	addx8	a12, a15, a13   // a12 = pointer to int table entry 
	beqz	a4, .Lm_call_secure_handler

	//  For nonsecure handling:
	//  1. Bypass callx0 to handler
	//  2. Restore remaining state
	//  3. RFI to nonsecure code
	//
	//  NOTE: Once a nonsecure interrupt is taken, control cannot return to
	//  this loop to pick up remaining interrupts.  They will be serviced on 
	//  on the next interrupt.
	l32i	a2, a12, XIE_HANDLER    // a2 = address of interrupt handler
	movi	a0, LABEL(secmon_dispatch_L,int_to_nsm)
	jx	a0

.Lm_call_secure_handler:

	wsr.intclear	a11	// clear if edge-trig or s/w or wr/err (else no effect)

	l32i	a13, a12, XIE_HANDLER
	l32i	a2, a12, XIE_ARG
	mov	a15, a3			// save secmon_nsm_interrupt_table
	mov	a3, a1
	callx0	a13
	mov	a3, a15			// restore secmon_nsm_interrupt_table
	rsr.interrupt	a15
	rsr.intenable	a12
	movi	a13, INTERRUPT_MASK
	and	a15, a15, a12
	and	a15, a15, a13
	_bnez	a15, LABEL(.L1,_loop0)

#endif /* SINGLE_INTERRUPT */

	// Restore everything, and return.

	restore_loops_mac16	a1, a13, a14, a15
	l32i	a14, a1, UEXC_sar
LABEL(spurious,int):

#if XCHAL_HAVE_EXCLUSIVE
	// Clear exclusive monitors.
	clrex
#endif

	//  Secure Monitor: Restore EPCn/EPSn in case user handler modifies them.
	l32i	a4, a1, MESF_EPCN
	l32i	a3, a1, MESF_EPSN
	writesr	epc _INTERRUPT_LEVEL a4
	writesr	eps _INTERRUPT_LEVEL a3

	//  Restore exception state:
	l32i	a2, a1, MESF_EPC1
	l32i	a3, a1, MESF_EXCCAUSE
	wsr.epc1	a2
	wsr.exccause	a3
	l32i	a2, a1, MESF_EXCVADDR
	wsr.excvaddr	a2
	l32i	a3, a1, MESF_EXCSAVE1
	wsr.excsave1	a3

	wsr.sar	a14
	l32i	a0, a1, MESF_AR(0)
	l32i	a2, a1, MESF_AR(2)
	l32i	a3, a1, MESF_AR(3)
	l32i	a4, a1, MESF_AR(4)
	l32i	a5, a1, MESF_AR(5)
	l32i	a6, a1, MESF_AR(6)
	l32i	a7, a1, MESF_AR(7)
	l32i	a8, a1, MESF_AR(8)
	l32i	a9, a1, MESF_AR(9)
	l32i	a10, a1, MESF_AR(10)
	l32i	a11, a1, MESF_AR(11)
	l32i	a12, a1, MESF_AR(12)
	l32i	a13, a1, MESF_AR(13)
	l32i	a14, a1, MESF_AR(14)
	l32i	a15, a1, MESF_AR(15)
	l32i	a1,  a1, MESF_AR(1)     // restore sp
	rfi	_INTERRUPT_LEVEL


	.size	LABEL(_Level,FromVector), . - LABEL(_Level,FromVector)

	//  This symbol exists solely for the purpose of being able to pull-in this
	//  dispatcher using xtos_dispatch_level<n>() routines with the tiny-rt LSP:
	.global	LABEL(_Level,HandlerLabel)
	.set	LABEL(_Level,HandlerLabel), 0


	.align	4
	.global	LABEL(secmon_dispatch_L,int_to_nsm)
LABEL(secmon_dispatch_L,int_to_nsm):

	restore_loops_mac16	a1, a13, a14, a15
	l32i	a14, a1, UEXC_sar

#if XCHAL_HAVE_EXCLUSIVE
	// Clear exclusive monitors.
	clrex
#endif

	//  Restore exception state:
	l32i	a3, a1, MESF_EXCCAUSE
	l32i	a4, a1, MESF_EXCVADDR
	l32i	a5, a1, MESF_EXCSAVE1
	wsr.exccause	a3
	wsr.excvaddr	a4
	wsr.excsave1	a5

	wsr.sar	a14
	l32i	a4, a1, MESF_AR(4)
	l32i	a5, a1, MESF_AR(5)
	l32i	a6, a1, MESF_AR(6)
	l32i	a7, a1, MESF_AR(7)
	l32i	a8, a1, MESF_AR(8)
	l32i	a9, a1, MESF_AR(9)
	l32i	a10, a1, MESF_AR(10)
	l32i	a11, a1, MESF_AR(11)
	l32i	a12, a1, MESF_AR(12)
	l32i	a13, a1, MESF_AR(13)
	l32i	a14, a1, MESF_AR(14)
	l32i	a15, a1, MESF_AR(15)

	// Secure Monitor: a2 = nonsecure handler
	// Prepare to RFE into nonsecure code.
	wsr.epc1	a2          // epc1 <- a2 (nonsecure handler)

	// Preserve EPCn/EPSn in-place so nonsecure handler
	// can return properly to interrupted code.  
	// Neither has been modified, so avoid save/restore.

	// Set PS.PNSM and PS.EXCM but at PS.INTLEVEL(EXCM_LEVEL)
	// Prevents exceptions after RFE but before user handler gains control
	l32i	a3, a1, UEXC_ps
	movi	a2, ~PS_INTLEVEL_MASK
	and	a3, a3, a2
	bbci.l	a3, 6, 1f
	addi	a3, a3, 0x8
	addi	a3, a3, -PS_RING(1)
1:
	movi	a2, PS_PNSM|PS_EXCM|PS_INTLEVEL(XCHAL_EXCM_LEVEL)
	or	a3, a3, a2
	wsr.ps	a3

	rsync				// wait for WSR to PS to complete
	l32i	a0, a1, MESF_AR(0)

	// Place old SP into excsaveN
	l32i	a2, a1, MESF_AR(1)
	writesr	excsave _INTERRUPT_LEVEL a2

	l32i	a2, a1, MESF_AR(2)
	l32i	a3, a1, MESF_AR(3)


	// Done using secure stack; a1 <- original epc1
	l32i	a1, a1, MESF_EPC1

	// excsaveN contains original SP, exchange with original epc1
	xchgsr	excsave _INTERRUPT_LEVEL a1
	rfe

	.size	LABEL(secmon_dispatch_L,int_to_nsm), . - LABEL(secmon_dispatch_L,int_to_nsm)


#endif /* XCHAL_HAVE_INTERRUPT */

