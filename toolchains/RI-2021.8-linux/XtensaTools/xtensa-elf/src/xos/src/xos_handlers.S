
// xos_handlers.S - User and Kernel Vector Exception Handlers

// Copyright (c) 2015-2020 Cadence Design Systems, Inc.
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


//-----------------------------------------------------------------------------
// IMPLEMENTATION NOTES
//
//  This OS defines the following processor states:
//
//	State		UM  EXC sp  CUR	Description
//	-----------	--  --- --- ---	---------------------------------
//	<nocontext>	               	running outside the context of any thread
//						(on interrupts, no need to save state)
//						(except nested interrupts, not yet covered here)
//	  <idle>	1   noc  ?   0 	while running idle loop
//	  <restoring>	1   noc tsk tsk	while restoring thread state (sp=tsk even w/ESF not alloc!)
//	 <interrupt>	1  ?noc ISB ...	in interrupt handler (!?)
//	 <nested>	0  ?noc int ...	in nested interrupt handler
//	<thread>		1   tsk tsk tsk	running in the context of a specific thread
//	<masked>	x    x   x   x 	interrupts disabled, in critical section, transitioning
//
//	EXC = XHREG (noc = &xos_dispatch_from_idle, tsk = &xos_dispatch_from_thread)
//	sp = stack ptr (a1) (ISB = base of int stack, tsk = thread's stack, int = in int. stack)
//	UM = PS.UM (0 while handling interrupts, 1 all other times)
//
//  <idle>:
//	- XHREG = &xos_dispatch_from_idle
//	- interrupts are enabled
//	- processor is in the idle loop (see .Lidle)
//	- stack (a1) = one frame down in interrupt stack
//	- XOS_CURR_THREADPTR == idle thread TCB
//	- unhandled exceptions go to _xos_exc_dispatcher
//	  (no thread context for them, they're unexpected)
//
//  <restoring>
//	- similar to <idle>...
//	- processor is restoring thread state
//	- ... other differences? sp? XOS_CURR_THREADPTR? etc
//
//  <interrupt>:
//	- XHREG = &xos_dispatch_from_idle
//	- interrupts are enabled
//	- stack (a1) = base of interrupt stack
//
//  <thread>:
//	- XHREG = &xos_dispatch_from_thread
//	- interrupts are enabled
//	- on interrupts, must save thread state
//	- stack (a1) = ???
//
//  <masked>:
//	- interrupts are disabled
//	- everything else undefined? can transition between states...
//	- XHREG is undefined
//	- stack (a1) = ???
//	- processor just took exception or interrupt,
//	  is exiting one, or is in some other critical section...
//
//  TRANSITIONS:
//
//  o  Interrupt in <thread>:
//	- XHREG marked <nocontext>
//	- save thread state to:  thread sp - XOS_FRAME_SIZE
//	- move save area (thread_interrupt_from_thread)
//	- a1 = base of interrupt stack
//	- enable ints above level-1, resolve interrupt, and enable interrupts
//	==> now in <interrupt>
//	- dispatch (call handler)
//	- disable interrupts
//	- if current thread:  move back save area, restore sp (for purpose of
//		window spill? else sp restore is gratuitous)
//	- test for context-switch
//	- if switch:  window spill, set new current thread
// 		if switch from idle, no need to window spill
//	- if no current thread:  goto idle
//	- a1 = current thread's sp
//	- enable interrupts
//	==> now in <restoring>
//	- restore thread state
//	- disable interrupts
//	- XHREG marked <thread>
//	- return from interrupt
//
//  o  Interrupt in <nocontext>:
//	- if a1 == base of interrupt stack
//	  *  goto:  enable ints above level-1, resolve interrupts, enable...
//	- else:
//	  *  allocate stack frame
//	  *  goto:  move save area, a1 = base of interrupt stack, ...
//-----------------------------------------------------------------------------


#include <xtensa/coreasm.h>
#include <xtensa/corebits.h>
#include <xtensa/config/core.h>
#include "xos_common.h"


//-----------------------------------------------------------------------------
// Macro extract_msb - return the input with only the highest bit set.
//
// Input:  "ain"  - Input value, clobbered.
// Output: "aout" - Output value, has only one bit set, MSB of "ain".
// The two arguments must be different AR registers.
//-----------------------------------------------------------------------------

	.macro	extract_msb	aout ain
.Lemsb:
	addi	\aout, \ain, -1		// aout = ain - 1
	and	\ain, \ain, \aout	// ain  = ain & aout
	bnez	\ain, .Lemsb		// repeat until ain == 0
	addi	\aout, \aout, 1		// return aout + 1
	.endm


//-----------------------------------------------------------------------------
// High priority interrupt dispatcher routines. These dispatchers are called
// by the respective interrupt vectors (see xos_vectors.S). The current method
// of getting from the vector to the dispatcher requires them to be linked into
// the same executable.
// (High priority   - interrupt level > EXCM_LEVEL)
//
// When control arrives into the dispatcher, the original value of a0 has been
// saved into register EXCSAVE_N (N = level) and all other registers must be
// preserved. The dispatcher will call the handler function if the pointer to
// the handler is a nonzero value. It will save a2 so that the handler has one
// scratch register to work with. The handler must preserve all the registers
// except a0 and a2, and must return to the dispatcher.
// Handling sequence:
//   - CPU receives interrupt, execution transfers to vector
//   - Vector code jumps to dispatcher (here)
//   - Dispatcher calls handler (if installed) then returns from interrupt
//
// Normally, user code would install handlers as needed by calling the function
// xos_register_hp_interrupt_handler(). Using this method is more flexible but
// does take a few more instructions. If interrupt response time is extremely
// critical, then this two-step process can be bypassed by defining one or more
// custom dispatchers in user code, with the same names as the default ones.
// These custom dispatchers will override the default ones (which are defined
// as "weak"). E.g. if it is desired to override the level 3 dispatcher, then
// define a custom dispatcher named "xos_level3_interrupt_dispatch". This must
// be written in assembly.
//-----------------------------------------------------------------------------

#if XCHAL_HAVE_INTERRUPTS

//-----------------------------------------------------------------------------
// Dummy handler for high priority interrupts. Does nothing. Perhaps should 
// cause a fatal error if called.
//-----------------------------------------------------------------------------

	.text
	.global		xos_dummy_handler
	.align		4

xos_dummy_handler:
	ret

//-----------------------------------------------------------------------------
// Storage for pointers to user-installed handlers, one for each level.
//-----------------------------------------------------------------------------

	.data
	.global		xos_hp_dispatch_table
	.align		4

xos_hp_dispatch_table:
	.word		xos_dummy_handler	// Level 2
	.word		xos_dummy_handler	// Level 3
	.word		xos_dummy_handler	// Level 4
	.word		xos_dummy_handler	// Level 5
	.word		xos_dummy_handler	// Level 6
	.text

//-----------------------------------------------------------------------------
// One dispatcher for each level if it exists AND is not the debug level.
//-----------------------------------------------------------------------------

	.macro	int_dispatch	level
	.global	xos_level&level&_interrupt_dispatch
	.weak	xos_level&level&_interrupt_dispatch

	.data
	.align	4

xos_hp_dispatch_save&level&:
	.word	0				// register save area

	.text
	.align	4

xos_level&level&_interrupt_dispatch:
	HWERR_487_FIX
	movi	a0, xos_hp_dispatch_save&level&
	s32i	a2, a0, 0			// Save a2 so handler can use it
	movi	a0, xos_hp_dispatch_table	// Get handler table address
	l32i	a0, a0, (\level - 2) * 4	// Load the right entry
	beqz	a0, .L&level&			// Skip if zero
	callx0	a0				// Call handler
.L&level&:
	movi	a2, xos_hp_dispatch_save&level&
	l32i	a2, a2, 0			// Restore a2
	readsr	excsave \level a0		// Restore a0
	rfi	\level				// Return from interrupt

	.size   xos_level&level&_interrupt_dispatch, . - xos_level&level&_interrupt_dispatch
	.endm

	.text

//-----------------------------------------------------------------------------
// Level 2
//-----------------------------------------------------------------------------

#if XCHAL_NUM_INTLEVELS >= 2 && XCHAL_DEBUGLEVEL != 2
	int_dispatch	2
#endif

//-----------------------------------------------------------------------------
// Level 3
//-----------------------------------------------------------------------------

#if XCHAL_NUM_INTLEVELS >= 3 && XCHAL_DEBUGLEVEL != 3
        int_dispatch    3
#endif

//-----------------------------------------------------------------------------
// Level 4
//-----------------------------------------------------------------------------

#if XCHAL_NUM_INTLEVELS >= 4 && XCHAL_DEBUGLEVEL != 4
        int_dispatch    4
#endif

//-----------------------------------------------------------------------------
// Level 5
//-----------------------------------------------------------------------------

#if XCHAL_NUM_INTLEVELS >= 5 && XCHAL_DEBUGLEVEL != 5
        int_dispatch    5
#endif

//-----------------------------------------------------------------------------
// Level 6
//-----------------------------------------------------------------------------

#if XCHAL_NUM_INTLEVELS >= 6 && XCHAL_DEBUGLEVEL != 6
        int_dispatch    6
#endif


//-----------------------------------------------------------------------------
// NMI dispatcher. If there is a user NMI handler installed, then jump to it.
// If none, cause a debug exception.
//-----------------------------------------------------------------------------

#if XCHAL_HAVE_NMI

	.data
	.global		xos_nmi_dispatch_table
	.align		4

xos_nmi_dispatch_table:
	.word		0		// No default handler

	.text
	.global		xos_nmi_dispatch
	.align		4

xos_nmi_dispatch:
	HWERR_487_FIX
	writesr	excsave XCHAL_NMILEVEL a0	// save a0
	movi	a0, xos_nmi_dispatch_table	// location of handler address
	l32i	a0, a0, 0			// address of handler
	beqz	a0, .Lnmitrap
	callx0	a0				// call user handler
	readsr	excsave XCHAL_NMILEVEL a0	// restore a0
	rfi	XCHAL_NMILEVEL			// return
.Lnmitrap:
#if XCHAL_HAVE_DEBUG
	break	1, 15				// trap
#else
	j	.Lnmitrap			// halt
#endif

	.size	xos_nmi_dispatch, . - xos_nmi_dispatch

#endif	// XCHAL_HAVE_NMI

#endif  // XCHAL_HAVE_INTERRUPTS


//-----------------------------------------------------------------------------
// xos_dispatch_from_idle:
// Interrupt / exception occurred while idle.
//      a0 = this PC          (original a0 in XHREG)
//      a1 = original a1 - XOS_FRAME_SIZE   (to be restored before enabling ints)
//      a2 = EPS with EXCM=0  (original a2 saved in [a1+FRAME_A2])
//      a3 = EPC              (original a3 saved in [a1+FRAME_A3])
//  EXCCAUSE = per exception, or LEVEL1_INTERRUPT for medium-pri interrupts
//-----------------------------------------------------------------------------

	.align	4
	.global	xos_dispatch_from_idle

xos_dispatch_from_idle:

	s32i	a3, a1, FRAME_PC
	addi	a2, a2, PS_EXCM
	s32i	a2, a1, FRAME_PS

#if XOS_OPT_STATS
#if XCHAL_HAVE_CCOUNT
	l32i	a2, a1, FRAME_A2		// recover thread pointer
	update_cycle_count a2 a0 a3		// update idle thread cycle count
	movi	a3, xos_inth_start_ccount	// start counting cycles for interrupt
	XOS_GET_CCOUNT(a0)
	s32i	a0, a3, 0
#endif
#endif

	rsr.exccause	a3
	bnei	a3, EXCCAUSE_LEVEL1_INTERRUPT, _xos_exc_dispatcher

#if XCHAL_HAVE_INTERRUPTS
	movi	a0, xos_dispatch_from_nest
	movi	a2, 0				// indicate in idle loop with zero old SP
	wsr_xhreg(a0)				// mark as in interrupt handling
	movi	a0, 0
	movi	a1, xos_int_stack_end
	movi	a3, PS_WOE_ABI | PS_UM | PS_INTLEVEL(XOS_MAX_OS_INTLEVEL)
	j	take_interrupt_from_idle
#else
	// something bad happened, let exception dispatch deal with it
	j	_xos_exc_dispatcher
#endif


//-----------------------------------------------------------------------------
// xos_dispatch_from_restore:
// Interrupt/exception occurred during thread context restore.
//      a0 = this PC          (original a0 in XHREG)
//      a1 = original a1 - XOS_FRAME_SIZE   (to be restored before enabling ints)
//      a2 = EPS with EXCM=0  (original a2 saved in [a1+FRAME_A2])
//      a3 = EPC              (original a3 saved in [a1+FRAME_A3])
//  EXCCAUSE = per exception, or LEVEL1_INTERRUPT for medium-pri interrupts
//
//  NOTE: because we already started counting cycles for the thread about to
//  be restored, we must stop that here and switch to counting cycles for the
//  interrupt.
//-----------------------------------------------------------------------------

	.align	4
	.global	xos_dispatch_from_restore

xos_dispatch_from_restore:

	// Don't save a2 or a3 here

#if XOS_OPT_STATS
#if XCHAL_HAVE_CCOUNT
	movi	a0, xos_globals			// stop counting cycles for thread
	l32i	a0, a0, XOS_CURR_THREADPTR
	update_cycle_count a0 a2 a3
	movi	a3, xos_inth_start_ccount	// start counting cycles for interrupt
	XOS_GET_CCOUNT(a0)
	s32i	a0, a3, 0
#endif
#endif

	rsr.exccause	a3
	bnei	a3, EXCCAUSE_LEVEL1_INTERRUPT, _xos_exc_dispatcher

#if XCHAL_HAVE_INTERRUPTS
	movi	a0, xos_dispatch_from_nest
	mov	a2, a1				// restore has real a1
	movi	a1, xos_int_stack_end
	wsr_xhreg(a0)				// mark as in interrupt handling
	movi	a0, 0
	// NOTE: need to jump where we copy the a0-a3 save area (keep it under sp)
	j	take_interrupt_from_restore
#else
	// something bad happened, let exception dispatcher deal with it
	j	_xos_exc_dispatcher
#endif


//-----------------------------------------------------------------------------
// xos_dispatch_from_nest:
// Nested-interrupt or exception (occurred during interrupt handling).
//      a0 = this PC          (original a0 in XHREG)
//      a1 = original a1 - XOS_FRAME_SIZE   (to be restored before enabling ints)
//      a2 = EPS with EXCM=0  (original a2 saved in [a1+FRAME_A2])
//      a3 = EPC              (original a3 saved in [a1+FRAME_A3])
//  EXCCAUSE = per exception, or LEVEL1_INTERRUPT for medium-pri interrupts
//-----------------------------------------------------------------------------

	.align	4
	.global	xos_dispatch_from_nest

xos_dispatch_from_nest:

	HWERR_487_FIX
	s32i	a3, a1, FRAME_PC
	rsr.exccause	a3
	addi	a2, a2, PS_EXCM			// need this for return
	s32i	a2, a1, FRAME_PS
	bnei	a3, EXCCAUSE_LEVEL1_INTERRUPT, _xos_exc_dispatcher
	movi	a2, 0				// nested

#if XCHAL_HAVE_INTERRUPTS
	j	interrupt_dispatch
#else
	j	_xos_exc_dispatcher
#endif


//-----------------------------------------------------------------------------
// xos_dispatch_from_thread:
// Interrupt/exception occurred in thread.
//      a0 = this PC          (original a0 in XHREG)
//      a1 = original a1 - XOS_FRAME_SIZE   (to be restored before enabling ints)
//      a2 = EPS with EXCM=0  (original a2 saved in [a1+FRAME_A2])
//      a3 = EPC              (original a3 saved in [a1+FRAME_A3])
//  EXCCAUSE = per exception, or LEVEL1_INTERRUPT for medium-pri interrupts
//-----------------------------------------------------------------------------

	.align	4
	.global	xos_dispatch_from_thread

xos_dispatch_from_thread:

	HWERR_487_FIX
	s32i	a3, a1, FRAME_PC
	addi	a2, a2, PS_EXCM			// need this for return
	s32i	a2, a1, FRAME_PS
	rsr.exccause	a3
	bnei	a3, EXCCAUSE_LEVEL1_INTERRUPT, _xos_exc_dispatcher
	movi	a2, 1				// not nested

#if XCHAL_HAVE_INTERRUPTS

interrupt_dispatch:
	//  a2 = 0 if nested interrupt, 1 if non-nested interrupt (interrupted a thread).

	movi	a0, xos_dispatch_from_nest	// mark as in interrupt handling
	s32i	a4, a1, FRAME_AR(4)
	xsr_xhreg(a0)
	s32i	a0, a1, FRAME_A0

	s32i	a5, a1, FRAME_AR(5)
	s32i	a6, a1, FRAME_AR(6)
	s32i	a7, a1, FRAME_AR(7)
	s32i	a8, a1, FRAME_AR(8)
	s32i	a9, a1, FRAME_AR(9)
	s32i	a10, a1, FRAME_AR(10)
	s32i	a11, a1, FRAME_AR(11)
	s32i	a12, a1, FRAME_AR(12)
	s32i	a13, a1, FRAME_AR(13)
	s32i	a14, a1, FRAME_AR(14)
	s32i	a15, a1, FRAME_AR(15)

#if XCHAL_HAVE_LOOPS
	rsr.lbeg  a0
	rsr.lend  a3
	s32i	a0, a1, FRAME_LBEG
	movi	a0, 0
	xsr.lcount a0				// zero LCOUNT just in case
	s32i	a3, a1, FRAME_LEND
	s32i	a0, a1, FRAME_LCOUNT
#endif
#if XCHAL_HAVE_MAC16
	rsr.acchi  a0
	rsr.acclo  a3
	s16i	a0, a1, FRAME_ACCHI
	s32i	a3, a1, FRAME_ACCLO
#endif
	rsr.sar	a0
	movi	a3, xos_globals			// must never be NULL
	s8i	a0, a1, FRAME_SAR

#if XCHAL_HAVE_EXCLUSIVE
	movi	a0, 0
	getex	a0				// a0 = ATOMCTL[8], ATOMCTL[8] = 0
	s32i	a0, a1, FRAME_ATOMCTL		// save ATOMCTL[8]
#endif

	//  Save virtual (software) interrupt level mask.
	l32i	a0, a3, XOS_INTLEVEL_MASK
	s32i	a0, a1, FRAME_LEVELMASK

	//  If we're the only active interrupt (ie. non-nested):
	//  update thread TCB, prepare for context-switch, and move to interrupt stack.
	//
	beqz	a2, 1f				// skip thread chk if nested interrupt
	l32i	a2, a3, XOS_CURR_THREADPTR	// get ptr to current thread
	s32i	a2, a3, XOS_NEXT_THREADPTR	// by default, is next to return to
	s32i	a1, a2, TCB_STACK_ESF		// save thread stack frame ptr

#if XOS_OPT_STATS
#if XCHAL_HAVE_CCOUNT
	update_cycle_count a2 a3 a0		// stop counting cycles for thread
	movi	a3, xos_inth_start_ccount	// start counting cycles for interrupt
	XOS_GET_CCOUNT(a0)
	s32i	a0, a3, 0
#endif
#endif

	//  Move to interrupt stack.
	mov	a2, a1
	movi	a1, xos_int_stack_end

1:	moveqz	a2, a1, a2			// save old a1 (if branched here from
						//  beqz above, where a2 == 0)

take_interrupt_from_restore:
#ifndef __XTENSA_CALL0_ABI__			// windowed ABI ?
	//  We're changing SP, so move save area with it (like MOVSP might do):
	//
	//  a2 = old SP (interruptee's SP minus XOS_FRAME_SIZE)
	//		===>  possibly interrupted restore code's SP
	//  a1 = new SP (interrupt stack, or same as a2)
	//  a0, a3 = undefined

#if XOS_OPT_UM
	//  Interruptee may have been running at lower privilege level.
	//  Make sure we are at ring 0 before we try any writes to the
	//  privileged stack.
	movi	a0, PS_RING_MASK
	neg	a0, a0
	addi	a0, a0, -1
	rsr.ps	a3
	and	a3, a3, a0
	wsr.ps	a3
	rsync
#endif
	l32i	a0, a2, FRAME_CWINSAVE
	s32e	a0, a1, -16
	l32i	a0, a2, FRAME_CWINSAVE + 4
	s32e	a0, a1, -12
	l32i	a0, a2, FRAME_CWINSAVE + 8
	s32e	a0, a1, -8
	l32i	a0, a2, FRAME_CWINSAVE + 12
	s32e	a0, a1, -4
#endif

	//  We've saved all thread state *except* for flushing register windows.
	//  Also, caller a0-a3 for that thread are now on interrupt stack
	//  instead of on that thread's stack, we'll have to swap that if there
	//  is a context switch.

	//  NOTE: Here we enter idle state with:
	//	CURR_THREADPTR set
	//	a1 = int. stack
	//	caller's a0-a3 under a1 (where they DON'T belong)
	//	windows NOT spilled
	//	ie. we're idle but on a switch need to:
	//		move a1 and its a0-a3, and flush reg-windows.
	//  NOTE:  if another interrupt pre-empts, we do forget about having tried
	//	to service this interrupt, but that's okay as we haven't disabled it
	//	or made it non-pending or serviced it at all so it will interrupt
	//	the cure again if still pending.

	//  Setup PS to allow window overflow, but don't enable interrupts just yet.
	//  (This is because we properly resolve+dispatch interrupts of any level.)
	//  For valid windowed ABI state, need to clear PS.EXCM and set PS.WOE.
	//  (PS.UM presumably irrelevant while interrupts disabled to EXCM LEVEL.)

	movi	a3, PS_WOE_ABI | PS_UM | PS_INTLEVEL(XOS_MAX_OS_INTLEVEL)
	l32i	a0, a2, FRAME_A0	// restore a0 for call tracebacks

take_interrupt_from_idle:
	//  a2 = old (interruptee) SP, or 0 if from idle (via xos_dispatch_from_idle)
	//  a3 = new PS value

	//  Update PS, now we can take high-priority interrupts (>EXCM_LEVEL).
	//  Save old PS for use below.

	xsr.ps	a3
	rsync

	//  Safe to take window exceptions from here.
	//  Now figure out which pending interrupt (if any) to dispatch.
	//  We want to inspect interrupts at current level only. Anything
	//  pending at a higher level will get taken as soon as we update
	//  PS below. Anything at a lower level may have been masked off
	//  by the application code, we don't know so we shouldn't look at
	//  those.

	movi	a4, Xthal_intlevel_mask	// HAL table of interrupt level masks
	extui	a3, a3, PS_INTLEVEL_SHIFT, PS_INTLEVEL_BITS	// extract interrupt level
	bnez	a3, 1f
	addi	a3, a3, 1		// PS.INTLEVEL is zero for L1 interrupts
1:
	addx4	a3, a3, a4		// a3 = &Xthal_intlevel_mask[level]
	l32i	a4, a3, 0		// a4 = Xthal_intlevel_mask[level]

	mov	a6, a2                  // in case we branch to .L_spurious
	rsr.interrupt	a7
	rsr.intenable	a3
	and	a7, a7, a3		// mask with INTENABLE
	and	a7, a7, a4		// mask with interrupts at current level
	movi	a3, xos_globals
	beqz	a7, .L_spurious

.L_take_another:

	//  For now, we take the priority order to be high bit -> low bit
	//  to match the priority masks defined in the HAL.
	extract_msb a5 a7		// extract highest set bit of a7
	wsr.intclear	a5		// clear interrupt (if software or edge-triggered)

	//  Convert interrupt bit to interrupt number.
	find_ms_setbit	a6, a5, a6, 0	// intr number in a6
	addx8	a5, a6, a3		// index into table of handlers
#if XOS_OPT_INTERRUPT_SWPRI
	addx8	a5, a6, a5		// (multiply by 16 for software-prioritized)
	//  Now a5 points to this interrupt's entry in the interrupt table.
	//  Prioritize in software among level 1..EXCMLEVEL interrupts:
	l32i	a7, a5, XOS_INTTAB_PRIMASK	// mask of interrupts to enable for virt. intlevel
# if XOS_MAX_OS_INTLEVEL > XCHAL_EXCM_LEVEL
	rsil	a6, XOS_MAX_OS_INTLEVEL	// protect updates to XOS_INTENABLE_MASK,INTENABLE
# endif
	l32i	a6, a3, XOS_INTENABLE_MASK
	s32i	a7, a3, XOS_INTLEVEL_MASK
	and	a7, a7, a6
	movi	a6, PS_WOE_ABI		// set PS.INTLEVEL=0 (using INTENABLE to mask
					// interrupts per software pri. level)
					// set PS.UM to 0 to show we're on intr stack
	wsr.intenable	a7
#else
	addx4	a5, a6, a5		// (multiply by 12 for hardware-prioritized)
	//  Now a5 points to this interrupt's entry in the interrupt table.
	l32i	a6, a5, XOS_INTTAB_PS	// get PS for this interrupt's level
#endif

	//  We're now officially handling interrupt (the one we from extract_msb)
	//  The write to PS below will enable all higher-priority interrupts,
	//  so if there is one pending it will get taken right away.

	wsr.ps	a6
	l32i	a7, a5, XOS_INTTAB_HANDLER	// get ptr to handler function
	rsync				// wait for WSR.PS to complete

	//  Call interrupt handler.  Might wake up a new thread, etc.

#ifdef __XTENSA_CALL0_ABI__			// windowed ABI?
	mov	a12, a2				// save a2 (old SP) in callee-saved reg
	mov	a13, a4
	l32i	a2, a5, XOS_INTTAB_ARG		// get arg to handler
	callx0	a7				// call interrupt handler
	mov	a2, a12				// restore a2 (old SP)
	mov	a4, a13
	movi	a3, xos_globals			// reload a3
#else
	mov	a3, a4
	l32i	a6, a5, XOS_INTTAB_ARG		// get arg to handler
	callx4	a7				// call interrupt handler
	mov	a4, a3
	movi	a3, xos_globals
#endif

	// Label for call traceback
return_from_interrupt:
	//  Okay.  Any other interrupts pending we can service?
	//  a4 still has level mask

	rsil	a5, XOS_MAX_OS_INTLEVEL
	rsr.interrupt	a7
	rsr.intenable	a5
	mov	a6, a2
	and	a7, a7, a5
	and	a7, a7, a4
	bnez	a7, .L_take_another

.L_spurious:

	//---------------------------------------------------------------------
	// Here:
	//	No pending enabled interrupt
	//	a1 = somewhere on interrupt stack (for calling above handler)
	//	a2 = old SP (interruptee SP minus XOS_FRAME_SIZE), or 0 if we took
	//	interrupt from idle. Matches a1 for nested interrupts, but not
	//	for other interrupts (the first interrupt moves to the interrupt
	//	stack)
	//	a3 = &xos_globals
	//	a6 = a2
	//---------------------------------------------------------------------

#if XCHAL_HAVE_EXCLUSIVE
	// Clear local exclusive monitor if present, since the interrupt handler(s)
	// could have done something with it.
	clrex
#endif

#if XOS_OPT_STATS
	movi	a4, xos_inth_intr_count	// increment number of intrs taken
	l32i	a5, a4, 0
	addi	a5, a5, 1
	s32i	a5, a4, 0
#endif

	l32i	a2, a3, XOS_NEXT_THREADPTR	// next thread to run
	beqz	a6, resume_thread		// came from idle, just resume to next thread

#ifndef __XTENSA_CALL0_ABI__			// windowed ABI ?
	//  a0-a3 of the interruptee's caller are still on interrupt stack,
	//  save them away to the interruptee's stack.
	l32e	a5, a1, -16
	s32i	a5, a6, FRAME_CWINSAVE
	l32e	a5, a1, -12
	s32i	a5, a6, FRAME_CWINSAVE + 4
	l32e	a5, a1, -8
	s32i	a5, a6, FRAME_CWINSAVE + 8
	l32e	a5, a1, -4
	s32i	a5, a6, FRAME_CWINSAVE + 12
#endif

	beq	a1, a6, return_nested_interrupt	// nested interrupt? return code is simpler

	//  Was not a nested interrupt, so need to consider possible context-switching.
	l32i	a4, a3, XOS_CURR_THREADPTR	// "current" thread (was running when interrupt taken)
	//  Note:  a6 == current thread stack == [a4 + TCB_STACK_ESF]

	addi	a1, a6, XOS_FRAME_SIZE		// valid SP is now curr thread's

#if XOS_OPT_STATS && XCHAL_HAVE_CCOUNT
	// Stop counting cycles for interrupt handling. Some cycles from here
	// on will go unallocated, best we can do for now.
	movi	a6, xos_inth_start_ccount	// a6 = &xos_inth_start_ccount
	movi	a8, xos_inth_cycle_count	// a8 = &xos_inth_cycle_count
	l32i	a6, a6, 0			// load last ccount
	XOS_GET_CCOUNT(a7)			// get current ccount
	sub	a7, a7, a6			// a7 = cycles spent
	l32i	a6, a8, LW_OFFSET		// a6 = low word of cycle count
	add	a7, a7, a6
	s32i	a7, a8, LW_OFFSET               // store low word
	bge	a7, a6, 8f			// no overflow, done
	l32i	a6, a8, HW_OFFSET		// a6 = high word of cycle count
	addi	a6, a6, 1			// increment high word if overflow
	s32i	a6, a8, HW_OFFSET		// store high word
8:
#endif

	//  Test for context switch (note, we're here from pre-empted thread,
	//  not from idle). If no switch then return to pre-empted thread, no
	//  TIE to be restored.

	beq	a2, a4, resume_preempted_thread_notie

	//  Context switch from pre-empted thread.
	//  NOTE:  can't do normal windowed call here, or we'd thrash the ESF
	//  (if returning from pre-empted thread).

#ifndef __XTENSA_CALL0_ABI__
        //  Spill any unsaved register windows to stack. Note that a4 is expected
	//  to be preserved across this code.

        movi    a3, PS_INTLEVEL(XOS_MAX_OS_INTLEVEL)
        wsr.ps  a3				// turn off PS.WOE, raise intr level
        mov     a5, a2                          // save away a2
        rsync
        call0   xthal_window_spill_nw
        mov     a2, a5                          // restore a2
        movi    a3, xos_globals                 // restore a3
#endif

#if XCHAL_CP_NUM > 0
	//  Save coprocessor enable state and disable all coprocessors.

	l32i	a6, a4, TCB_TIE_SAVE		// a6 = ptr to TIE save area
	rsr.cpenable	a5			// read CPENABLE
	beqz	a6, 3f				// skip if NULL
	s16i	a5, a6, XT_CPENABLE		// save CPENABLE state
3:
	movi	a5, 0				// disable all coprocessors
	wsr.cpenable	a5
#endif

	//  Save all non-coprocessor TIE state for this thread.

	l32i	a5, a4, TCB_TIE_SAVE		// a5 = ptr to TIE save area
	beqz	a5, skip_save			// TODO: should this be error ?
	l32i	a5, a5, XT_NCP_ASA		// a5 = ptr to aligned save area
	addi	a5, a5, XT_NCP_SA		// offset to non-CP TIE save area
	xchal_ncp_store	a5, a6,a7,a8,a9		// save non-coprocessor state

skip_save:

	//  NOTE: can only indicate switch (below) *after* completing
	//  the register window spill, which ensures all of the
	//  previous thread's state is saved.
	//  (Assuming interrupts have been enabled by this time...) (NOT !!)
	//  That's because NEXT not matching CURR is the only (heeded)
	//  indication that we have to spill, and we don't want to
	//  forget to complete spilling if an interrupt comes in
	//  (while spilling in idle state, so interrupt doesn't return).

resume_thread:
	l32i	a7, a2, TCB_RESUME_FN
	s32i	a2, a3, XOS_CURR_THREADPTR	// indicate switch
	jx	a7				// resume thread (restore thread state etc)

#endif // XCHAL_HAVE_INTERRUPTS


//-----------------------------------------------------------------------------
//  xos_resume_idle_thread -- this is the "idle thread".
//  Going idle so no need to restore any state. Set up registers as expected
//  for interrupts taken from idle.
//
//  a1 = interrupt stack if handled interrupt from idle to idle
//  OR
//  a1 = stack pointer of last thread, if switched from thread to idle
//  a2 = XOS_CURR_THREADPTR = points to idle thread TCB
//  a3 = &xos_globals
//  (last thread's caller a0-a3 saved correctly)
//  (last thread's windows spilled)
//-----------------------------------------------------------------------------

	.align	4
	.global	xos_resume_idle_thread

xos_resume_idle_thread:

#if XOS_OPT_STATS
#if XCHAL_HAVE_CCOUNT
	XOS_GET_CCOUNT(a7)			// store current ccount
	s32i    a7, a2, TCB_RESUME_CCOUNT
#endif
#endif

	movi	a1, xos_int_stack_end - XOS_FRAME_SIZE  // set up stack
	movi	a7, xos_dispatch_from_idle
	movi	a4, 0xFFFFFFFF			// enable all interrupts
	wsr_xhreg(a7)
#if XCHAL_HAVE_INTERRUPTS
	l32i	a5, a3, XOS_INTENABLE_MASK
	s32i	a4, a3, XOS_INTLEVEL_MASK	// virtually, all ints enabled
	and	a4, a4, a5
	wsr.intenable	a4
#endif
	movi	a6, PS_WOE_ABI | PS_UM
	wsr.ps	a6
	rsync

	//  Stack pointer in interrupt stack, a0 zero (?? or
	//  pointing back to idle start, save area having a0==0?),
	//  etc.

.Lidle:
#if XCHAL_HAVE_INTERRUPTS
	waiti	0				// take a nap
#endif
	j	.Lidle


//-----------------------------------------------------------------------------
// return_nested_interrupt:
// Return from nested interrupt.
//     a1 == a6 == ptr to interrupt stack frame (interruptee SP - XOS_FRAME_SIZE)
//     a3 == &xos_globals
//-----------------------------------------------------------------------------

#if XCHAL_HAVE_INTERRUPTS

return_nested_interrupt:
	l32i	a4, a1, FRAME_LEVELMASK
	l32i	a5, a3, XOS_INTENABLE_MASK
	s32i	a4, a3, XOS_INTLEVEL_MASK	// restore thread virtual interrupt level
	and	a4, a4, a5
	wsr.intenable	a4

	//  Adjust regs, and skip enabling of interrupts:
	mov	a8, a1
	addi	a1, a8, XOS_FRAME_SIZE
	movi	a9, xos_dispatch_from_nest	// to mark as in interrupt (stack)
	l32i	a2, a8, FRAME_A2
	l32i	a3, a8, FRAME_A3
	j	restore_common

#endif

	//  Note:  stay in "idle" state while restoring state,
	//  so that if an interrupt comes in during that time,
	//  it need not save any state (resume will just restart).

//-----------------------------------------------------------------------------
// xos_resume_preempted_thread:
//-----------------------------------------------------------------------------

	.global	xos_resume_preempted_thread

xos_resume_preempted_thread:

	//  Restore non-coprocessor TIE state from new thread's TIE save area.

	l32i	a5, a2, TCB_TIE_SAVE		// a5 = ptr to TIE save area
	beqz	a5, skip_restore		// TODO: should this be error ?
	l16ui	a4, a5, XT_CPENABLE		// a4 = saved CPENABLE or zero
	l32i	a5, a5, XT_NCP_ASA		// a5 = ptr to aligned save area
	addi	a5, a5, XT_NCP_SA		// offset to non-CP save area
	xchal_ncp_load	a5, a6,a7,a8,a9		// restore non-coprocessor state

#if XCHAL_CP_NUM > 0
	wsr.cpenable	a4			// restore CPENABLE
#endif

skip_restore:

resume_preempted_thread_notie:

// NOTE: can't move this up to before the TIE restore because sometimes we
// will skip the TIE restore and come here directly.

#if XOS_OPT_STATS
	l32i	a8, a2, TCB_PREEMPT_RESUMES	// increment # of preempt resumes
#if XCHAL_HAVE_CCOUNT
	XOS_GET_CCOUNT(a9)			// store current ccount
	s32i	a9, a2, TCB_RESUME_CCOUNT
#endif
	addi	a8, a8, 1
	s32i	a8, a2, TCB_PREEMPT_RESUMES
#endif

#if XOS_OPT_THREAD_SAFE_CLIB
	// Restore thread's C library context pointer
	l32i	a4, a2, TCB_CLIB_PTR
	movi	a5, GLOBAL_CLIB_PTR
	s32i	a4, a5, 0
#endif

	l32i	a2, a2, TCB_STACK_ESF		// get ptr to state to restore

	l32i	a4, a2, FRAME_LEVELMASK
#if XOS_MAX_OS_INTLEVEL > XCHAL_EXCM_LEVEL
	rsil	a7, XOS_MAX_OS_INTLEVEL
#endif

#if XCHAL_HAVE_INTERRUPTS
	l32i	a5, a3, XOS_INTENABLE_MASK
	s32i	a4, a3, XOS_INTLEVEL_MASK	// restore thread virtual interrupt level
	and	a4, a4, a5
	wsr.intenable	a4
#endif

#if XCHAL_HAVE_EXCLUSIVE
	l32i	a4, a2, FRAME_ATOMCTL
	getex	a4				// restore ATOMCTL[8]
#endif

	addi	a1, a2, XOS_FRAME_SIZE

	//  Okay, valid ISA state at this point.  Can enable all interrupts in "restore mode"
	//  while we restore thread state.
	//  IMPORTANT NOTE:  we must restore a2 and a3 and leave them intact during restore,
	//	because any interrupt will re-save them.

	mov	a8, a2
	l32i	a2, a8, FRAME_A2
	l32i	a3, a8, FRAME_A3

	movi	a7, xos_dispatch_from_restore
	movi	a4, PS_WOE_ABI | PS_UM
	wsr_xhreg(a7)
	wsr.ps	a4
	//rsync			// we can forego the rsync, no absolute need for
				// this WSR.PS to take effect, just there to lower latency
	//  Now we want PS.WOE set, PS.UM set...
	//	Here we've enabled all interrupts, via PS and INTENABLE;
	//	if something still pending, fine, it takes right away
	//	(in idle state, so no recursion)

	//  In this idle state:
	//	CURR_THREADPTR == thread being resumed
	//	a1 == stack ptr of that thread
	//	windows already spilled (though okay to try to spill them)
	//	caller's a0-a3 under a1 (where they belong)

	movi	a9, xos_dispatch_from_thread	// to mark as in thread (no longer idle)
restore_common:
	//  a9 = value to restore into XHREG
	l8ui	a4, a8, FRAME_SAR
#if XCHAL_HAVE_LOOPS
        //  Avoid unintended loopback, do not restore LCOUNT until PS.EXCM is set.
	l32i	a5, a8, FRAME_LBEG
	l32i	a6, a8, FRAME_LEND
	wsr.lbeg	a5
	wsr.lend	a6
#endif
#if XCHAL_HAVE_MAC16
	l16si	a5, a8, FRAME_ACCHI
	l32i	a6, a8, FRAME_ACCLO
	wsr.acchi	a5
	wsr.acclo	a6
#endif
	wsr.sar	a4

	l32i	a4, a8, FRAME_AR(4)
	l32i	a5, a8, FRAME_AR(5)
	l32i	a6, a8, FRAME_AR(6)
	l32i	a7, a8, FRAME_AR(7)
	l32i	a10, a8, FRAME_AR(10)
	l32i	a11, a8, FRAME_AR(11)
	l32i	a12, a8, FRAME_AR(12)
	l32i	a13, a8, FRAME_AR(13)
	l32i	a14, a8, FRAME_AR(14)
	l32i	a0, a8, FRAME_PS
	l32i	a15, a8, FRAME_AR(15)

	//  Restoring PS sets PS.EXCM which disables interrupts.
	//  So it's then safe to restore critical state (EPC1 etc).
	//  Also safe to restore LCOUNT because loopback is disabled.
	wsr.ps		a0
	l32i	a0, a8, FRAME_PC
	rsync					// wait for WSR.PS to complete
	wsr.epc1	a0
	wsr_xhreg(a9)				// now in thread or interrupt (not idle)
#if XCHAL_HAVE_LOOPS
	l32i	a0, a8, FRAME_LCOUNT
	wsr.lcount	a0
#endif
	l32i	a0, a8, FRAME_A0
	l32i	a9, a8, FRAME_AR(9)
	l32i	a8, a8, FRAME_AR(8)
	rfe


//-----------------------------------------------------------------------------
// xos_resume_by_restart:
//
// At entry, XOS_CURR_THREADPTR should have been set to the current TCB.
//-----------------------------------------------------------------------------

	.align	4
	.global	xos_resume_by_restart

xos_resume_by_restart:

#ifndef __XTENSA_CALL0_ABI__			// windowed ABI ?
	movi	a2, 1				// reset register window state
	movi	a3, 0
	wsr.windowstart	a2
	wsr.windowbase	a3
#endif

	movi	a3, xos_globals			// a3 = global data pointer
	l32i	a2, a3, XOS_CURR_THREADPTR	// a2 = current thread TCB

#if XOS_OPT_STATS
	l32i	a8, a2, TCB_NORMAL_RESUMES	// increment # of normal resumes
#if XCHAL_HAVE_CCOUNT
	XOS_GET_CCOUNT(a9)			// store current ccount
	s32i	a9, a2, TCB_RESUME_CCOUNT
#endif
	addi	a8, a8, 1
	s32i	a8, a2, TCB_NORMAL_RESUMES
#endif

	movi	a11, xos_resume_preempted_thread
	s32i	a11, a2, TCB_RESUME_FN		// setup for possible preemptive switch

#if XOS_OPT_THREAD_SAFE_CLIB
	l32i	a4, a2, TCB_CLIB_PTR		// restore thread's C lib context pointer
	movi	a5, GLOBAL_CLIB_PTR
	s32i	a4, a5, 0
#endif

	movi	a0, 0
#if XCHAL_HAVE_LOOPS
	wsr.lcount	a0			// clear loop state just-in-case
#endif
#if XCHAL_CP_NUM > 0
	wsr.cpenable	a0			// disable all coprocessors to start
#endif

	movi	a4, xos_dispatch_from_thread	// to mark as in thread (no longer idle)
	l32i	a5, a2, TCB_FLAGS
	movi	a6, PS_WOECALL4_ABI | PS_UM | PS_EXCM
	wsr_xhreg(a4)				// now in thread (not idle)
	bbci.l	a5, 14, 1f			// user-mode if set
	addi	a6, a6, PS_RING(1)
1:
	l8ui	a4, a2, TCB_EXIT_FLAG		// load exit flag
	bnez	a4, xos_thread_abort_stub	// abort if exit flag set
	wsr.ps	a6

#if XCHAL_HAVE_INTERRUPTS
	movi	a4, 0xFFFFFFFF			// enable all interrupts
	l32i	a5, a3, XOS_INTENABLE_MASK
	s32i	a4, a3, XOS_INTLEVEL_MASK	// virtually, all ints enabled
	and	a4, a4, a5
	wsr.intenable	a4
#endif

	l32i	a1, a2, TCB_STACK_END		// load initial stack pointer
	l32i	a4, a2, TCB_STARTUP_ENTRY	// load ptr to startup function
	l32i	a3, a2, TCB_RETVALUE		// pass wake value as 2nd arg (currently unused)
	l32i	a2, a2, TCB_STARTUP_ARG		// load arg to startup function

	wsr.epc1	a4
#ifdef __XTENSA_CALL0_ABI__
	movi	a0, xos_thread_exit_trap
#else
	mov	a6, a2				// look like call4
	mov	a7, a3
	movi	a4, xos_thread_exit_trap
	movi	a5, (1 << 30)			// set top bits to look like call4
	slli	a4, a4, 2
	srli	a4, a4, 2
	add	a4, a4, a5
#endif
	rsync
	rfe

	//  If we get here, thread function has returned. Go to exit handling.

	.global	xos_thread_exit_trap

xos_thread_exit_trap:

	movi	a3, xos_globals
	l32i	a3, a3, XOS_CURR_THREADPTR
	l32i	a1, a3, TCB_STACK_END

#ifdef __XTENSA_CALL0_ABI__
	call0	xos_thread_exit			// exit thread (exit code in a2)
#else
	call4	xos_thread_exit			// exit thread (exit code in a6)
#endif

#if XCHAL_HAVE_DEBUG
	break	1, 15				// if we get here, it's fatal
#endif
	j	xos_thread_exit_trap


//-----------------------------------------------------------------------------
// xos_thread_abort_stub:
//
// Execution comes here if a thread is aborted. This stub sets the exit value
// as the return code and then starts exit processing.
//-----------------------------------------------------------------------------

	.align  4
	.global xos_thread_abort_stub

xos_thread_abort_stub:
	movi    a2, xos_globals
        l32i    a3, a2, XOS_CURR_THREADPTR	// a3 = ptr to current thread
#ifdef __XTENSA_CALL0_ABI__
	l32i    a2, a3, TCB_RETVALUE		// a2 = thread->wake_value
#else
	l32i    a6, a3, TCB_RETVALUE		// a6 = thread->wake_value
#endif
	j       xos_thread_exit_trap


//-----------------------------------------------------------------------------
// int xos_switch_thread(XosThread *next_thread);
//
// Solicited switch to the specified thread, from a normal thread. next_thread
// must be non-zero and different than xos_curr_threadptr (it may get stored
// here in xos_next_threadptr ?).
// Callable from C (by OS primitive, not application).
//
// a2 - pointer to next thread
//-----------------------------------------------------------------------------

	.align 4
	.global	xos_switch_thread

xos_switch_thread:
	abi_entry 32, 0				// XosCoopFrame equivalent

.Lswitch_thread:
	// Context switch. Need to spill any unsaved register windows to stack.

	s32i	a0, a1, CFRAME_A0		// save a0 (this function's return PC)
	rsr.ps	a6				// save PS
	s32i	a6, a1, CFRAME_PS

#ifdef __XTENSA_CALL0_ABI__
	s32i	a12, a1, CFRAME_AREG + 0	// save callee-saved registers (a12-a15)
	s32i	a13, a1, CFRAME_AREG + 4
	s32i	a14, a1, CFRAME_AREG + 8
	s32i	a15, a1, CFRAME_AREG + 12
#if XCHAL_HAVE_INTERRUPTS
	rsil	a6, XOS_MAX_OS_INTLEVEL
#endif
#else
	// Spill register windows. Calling the windowed version uses extra cycles,
	// so we set things up to be able to call the _nw version.
	movi	a5,  ~(PS_WOE_MASK | PS_INTLEVEL_MASK)
	and	a5, a5, a6			// clear WOE, INTLEVEL
	addi	a5, a5, XOS_MAX_OS_INTLEVEL	// set INTLEVEL
	wsr.ps	a5
	rsync
	mov	a4, a2				// save a2 across call
	call0	xthal_window_spill_nw
	mov	a2, a4
#endif

	// Interrupts now disabled when we get here
	movi	a3, xos_globals
	movi	a6, xos_resume_cooperative_thread	// how to resume this thread
	l32i	a4, a3, XOS_CURR_THREADPTR		// a4 = ptr to current thread

	l32i	a5, a3, XOS_INTLEVEL_MASK
	s32i	a5, a1, CFRAME_LEVELMASK	// save intlevel mask
	s32i	a1, a4, TCB_STACK_ESF		// save ESF to current thread TCB
	s32i	a6, a4, TCB_RESUME_FN		// how to resume from this cooperative switch

#if XCHAL_CP_NUM > 0
	l32i	a6, a4, TCB_TIE_SAVE		// a6 = current thread's TIE save area
	rsr.cpenable a5
	beqz	a5, .L0
	call0	_xos_coproc_savecs		// save callee-saved CP state
	movi	a5, 0
	wsr.cpenable a5				// clear CPENABLE, give up all CPs
.L0:
	beqz	a6, .L1
	s16i	a5, a6, XT_CPENABLE
.L1:
#endif

#if XOS_OPT_STATS
#if XCHAL_HAVE_CCOUNT
	update_cycle_count a4 a8 a9		// update cycle count for outgoing thread
						// a4 must have current TCB pointer
#endif
#endif

	l32i	a8, a2, TCB_RESUME_FN
	s32i	a2, a3, XOS_NEXT_THREADPTR	// convenient place to set it
	s32i	a2, a3, XOS_CURR_THREADPTR	// indicate switch to new thread
	jx	a8				// jump to thread-specific resume code


//-----------------------------------------------------------------------------
// xos_start_thread
// Start a new thread, don't attempt to save any context for caller.
// Callable from C code. Does not return to caller.
//
// a2 -- pointer to incoming thread's TCB.
//-----------------------------------------------------------------------------

	.align	4
	.global	xos_start_thread

xos_start_thread:
	abi_entry 32, 0				// keep same as xos_switch_thread

	movi	a3, xos_globals			// a3 = pointer to global data
	l32i	a4, a2, TCB_RESUME_FN		// a4 = thread resume code
	s32i	a2, a3, XOS_NEXT_THREADPTR
	s32i	a2, a3, XOS_CURR_THREADPTR	// indicate switch to new thread
	jx	a4				// jump to thread resume code


//-----------------------------------------------------------------------------
// xos_resume_cooperative_thread:
//
// a2 -- pointer to incoming thread's TCB
//-----------------------------------------------------------------------------

	.align	4
	.global	xos_resume_cooperative_thread

xos_resume_cooperative_thread:

#if XOS_OPT_STATS
	l32i	a8, a2, TCB_NORMAL_RESUMES	// increment # of normal resumes
#if XCHAL_HAVE_CCOUNT
	XOS_GET_CCOUNT(a9)			// store current ccount
	s32i	a9, a2, TCB_RESUME_CCOUNT
#endif
	addi	a8, a8, 1
	s32i	a8, a2, TCB_NORMAL_RESUMES
#endif

	movi	a11, xos_resume_preempted_thread
	s32i	a11, a2, TCB_RESUME_FN		// setup for possible preemptive switch

#if XOS_OPT_THREAD_SAFE_CLIB
	// Restore thread's C library context pointer
	l32i	a4, a2, TCB_CLIB_PTR
	movi	a5, GLOBAL_CLIB_PTR
	s32i	a4, a5, 0
#endif

	//  Restore thread state.
	l32i	a1, a2, TCB_STACK_ESF		// get ptr to state to restore
	l32i	a2, a2, TCB_RETVALUE		// get return value

#if XCHAL_HAVE_INTERRUPTS
	l32i	a4, a1, CFRAME_LEVELMASK	// restore intlevel mask ...
	l32i	a5, a3, XOS_INTENABLE_MASK
	s32i	a4, a3, XOS_INTLEVEL_MASK	// restore thread virtual interrupt level
	and	a4, a4, a5
	wsr.intenable	a4
#endif

	//  Okay, valid ISA state at this point.

	movi	a4, 0
#if XCHAL_HAVE_LOOPS
	wsr.lcount	a4
#endif

	//  Restoring PS sets PS.EXCM which disables interrupts.
	//  So it's then safe to restore critical state (EPC1 etc).
	movi	a3, xos_dispatch_from_thread	// to mark as in thread (no longer idle)
	l32i	a0, a1, CFRAME_PS	// restore PS
	wsr_xhreg(a3)			// now in thread (not idle)
	wsr.ps	a0
	l32i	a0, a1, CFRAME_A0	// restore a0

#ifdef __XTENSA_CALL0_ABI__			// call0 ABI?
	l32i	a12, a1, CFRAME_AREG+0	// restore callee-saved registers (a12-a15) ...
	l32i	a13, a1, CFRAME_AREG+4	// ...
	l32i	a14, a1, CFRAME_AREG+8	// ...
	l32i	a15, a1, CFRAME_AREG+12	// ...
#endif
	rsync				// wait for WSR.PS to complete
	abi_return 32, 0


#if XOS_OPT_SECMON

//-----------------------------------------------------------------------------
// xos_xtsm_syscall:
//
// a2: syscall ID / return value
// a3: param1
// a4: param2
//-----------------------------------------------------------------------------

	.align	4
	.global	xos_xtsm_syscall

xos_xtsm_syscall:
	abi_entry 16, 0
	syscall
	abi_return 16, 0

#endif // XOS_OPT_SECMON

