// crt0-app.S
// Applications downloaded in RAM using a debug monitor (eg. XMON, RedBoot)
// start here at _app_reset.  Such applications don't have any vectors:
// all exceptions are handled by the debug monitor.
// Thus, this file essentially plays the role of the reset vector
// to setup a few things before jumping to _start (in crt1*.S).

// Copyright (c) 2005-2018 Cadence Design Systems, Inc.
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

#include <xtensa/cacheattrasm.h>
#include <xtensa/coreasm.h>
#include <xtensa/corebits.h>


//  Assumptions on entry to _app_reset:
//	- debug monitor handles all exceptions, has setup all vectors
//	- interrupt related state is initialized
//	  (interrupts disabled or enabled for debug monitor's purposes)
//	- debug option related state is initialized (for debug monitor)
//	- any MMU related state is initialized (all handled by debug monitor)
//	- caches are initialized (cache attributes not necessarily correct)
//	- entire application is loaded (no unpacking needed here)

//  Assumptions on exit from _app_reset, ie. when jumping to _start:
//	- low (level-one) and medium priority interrupts are disabled
//	- C calling context not initialized:
//	  - PS not fully initialized (eg. PS.WOE not set per ABI)
//	  - SP not initialized
//	- the following are initialized:
//	  - LITBASE, WindowBase, WindowStart, LCOUNT, CPENABLE, FP's FCR and FSR,
//	    cache attributes
//	- (XEA3) all interrupts except NMI disabled via PS.DI
//	- (XEA3) WB register initialized

/**************************************************************************/

	.text
	.global	_app_reset
_app_reset:
	/* _app_reset may be required to be located at the beginning of the text
	   segment.  However, the constant pool for _app_reset must be placed
	   before the code.  Jump over the constant pool to solve this. */
	j	.LpastInitialConstants

	.literal_position	// tells the assembler/linker to place literals here

.LpastInitialConstants:
	// Keep a0 zero.  It is used to initialize a few things.
	// It is also the return address, where zero indicates
	// that the frame used by _start is the bottommost frame.
	//
	movi	a0, 0		// keep this register zero.

#if XCHAL_HAVE_LOOPS
	wsr.lcount	a0		// loop count = 0
#endif /* XCHAL_HAVE_LOOPS */

	// Interrupts might be enabled, make sure at least medium and low priority
	// interrupts are disabled until WindowBase, WindowStart, SP, and the stack
	// are all properly setup (which will happen outside this file, after the
	// _start label).  We leave loops enabled on new exception architecture.
#if XCHAL_HAVE_EXCEPTIONS
# if XCHAL_HAVE_XEA3
	movi	a2, (PS_STACK_FIRSTINT | PS_DI)	// PS.STK=FirstInt, PS.DI=1
	wsr.ps	a2
# else
	movi	a2, XCHAL_EXCM_LEVEL
	wsr.ps	a2		// set PS.INTLEVEL=EXCM_LEVEL, PS.WOE=0, PS.EXCM=0
# endif
	rsync
#endif

	// If the hardware is enabled for windowing, initialize to a known state.
#if XCHAL_HAVE_WINDOWED
# if XCHAL_HAVE_XEA3
#  if (defined __XTENSA_WINDOWED_ABI__)
	movi	a2, 1			// set up wb for windowed
	slli	a2, a2, 30
	wsr.wb	a2			// set bit 30 = windowed reg option
#  else
	movi	a2, 0			// set up wb for call0
	wsr.wb	a2
#  endif
# else	// XEA1/XEA2
	movi	a2, 1			// not needed for call0 but won't hurt
	wsr.windowstart	a2		// windowstart = 1
	wsr.windowbase	a0		// windowbase  = 0
# endif
	rsync
	// a0 may have moved as a result of what we did, so clear it again.
	movi	a0, 0
#endif

	// Now, BEFORE we do any L32R (or MOVI with non-immediate
	// range which results in an L32R), ensure LITBASE is set
	// correctly.  This is necessary for RAM applications loaded
	// using a target-side debug monitor -- such applications
	// don't have a reset vector and start execution at _start.
	// (This part is unnecessary if running from a reset vector.)
	// The target-side debug monitor might have set LITBASE to
	// anything at all, so we cannot rely on its value here.
#if XCHAL_HAVE_ABSOLUTE_LITERALS
	wsr.litbase	a0			// force PC-relative L32R
	rsync
# if XSHAL_USE_ABSOLUTE_LITERALS
	.begin	no-absolute-literals		// use PC-rel L32R to load
	movi	a2, _lit4_start + 0x40001	// address of absolute literals
	.end	no-absolute-literals		// (see handlers/ResetVector.S
	wsr.litbase	a2			//  for explanation)
	rsync
# endif
#endif


	/*
	 *  Now "enable" the caches.
	 *
	 *  NOTE:  We don't *initialize* the caches here, because the loader
	 *  (eg. target debugger agent / debug monitor, boot code, etc)
	 *  is expected to have initialized them for us.
	 *
	 *  The _memmap_cacheattr_reset symbol's value (address) is defined
	 *  by the LSP's linker script, as generated by xt-genldscripts.
	 *
	 *  (NOTE:  for configs that don't have CACHEATTR or region protection,
	 *   ie. for full MMUs, there is no equivalent cache attribute layout,
	 *   and the following code has no effect.  We assume for now that the
	 *   application restricts itself to the static TLB entries, i.e. to
	 *   virtual addresses 0xD0000000 thru 0xFFFFFFFF.)
	 */
#if XCHAL_HAVE_CACHEATTR || XCHAL_HAVE_MIMIC_CACHEATTR || XCHAL_HAVE_XLT_CACHEATTR \
		|| (XCHAL_HAVE_PTP_MMU && XCHAL_HAVE_SPANNING_WAY)
	movi	a2, _memmap_cacheattr_reset	/* note: absolute symbol, not a ptr */
	cacheattr_set				/* set CACHEATTR from a2 (clobbers a3-a8) */
#endif



	// Coprocessor option initialization
#if XCHAL_HAVE_CP
	//movi	a2, XCHAL_CP_MASK	// enable existing CPs
	//  To allow creating new coprocessors using TC that are not known
	//  at GUI build time without having to explicitly enable them,
	//  all CPENABLE bits must be set, even though they may not always
	//  correspond to a coprocessor.
	movi	a2, 0xFF	// enable *all* bits, to allow dynamic TIE
	wsr.cpenable	a2
#endif

	// Floating point coprocessor option initialization
#if XCHAL_HAVE_FP
	rsync		/* wait for WSR to CPENABLE to complete before accessing FP coproc state */
	wur.fcr	a0     /* clear FCR (default rounding mode, round-nearest) */
	wur.fsr	a0     /* clear FSR */
#endif


	/*  NOTE:  Future releases may clear BSS here rather than in the CRT1.  */


	/*
	 *  Now jump to the application.  This is typically the
	 *  C run-time initialization ("CRT") which in turn calls main():
	 */
	movi	a4, _start
	jx	a4		// jump to _start

	.size	_app_reset, . - _app_reset

