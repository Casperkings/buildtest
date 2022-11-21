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

// reset-vector-xea2-sm.S -- Xtensa Reset Vector


#include <xtensa/config/core.h>

#if !(XCHAL_HAVE_XEA2)
#error Secure LX configurations require XEA2
#endif
#if !(XCHAL_HAVE_MPU)
#error Secure LX configurations require MPU
#endif
#if !(XCHAL_HAVE_EXCEPTIONS)
#error Secure LX configurations require exceptions
#endif
#if (XCHAL_HW_MIN_VERSION < XTENSA_HWVERSION_RI_2020_5)
#error Secure LX configurations require RI-2020.5 or newer HW
#endif

#include <xtensa/coreasm.h>
#include <xtensa/corebits.h>
#include <xtensa/cacheasm.h>
#include <xtensa/cacheattrasm.h>
#include <xtensa/xtensa-xer.h>
#include <xtensa/xdm-regs.h>
#include <xtensa/config/system.h>	/* for XSHAL_USE_ABSOLUTE_LITERALS only */
#include <xtensa/xtruntime-core-state.h>
#include <xtensa/config/core-isa.h>
#include <xtensa/xnne.h>

#if defined (SIMULATOR)
#include <xtensa/simcall.h>
#endif

#include "xtos-internal.h"


	.weakref	_pso_savearea, _xtos_pso_savearea
	.weakref	_core_restore_nw, xtos_core_restore_nw


// The following reset vector avoids initializing certain registers already
// initialized by processor reset.  But it does initialize some of them
// anyway, for minimal support of warm restart (restarting in software by
// jumping to the reset vector rather than asserting hardware reset).

	.begin	prefer-l32r

	.begin	literal_prefix	.ResetVector
	.section		.ResetVector.text, "ax"

	.align	4
	.global	_ResetVector
_ResetVector:

#if XCHAL_HAVE_IMEM_LOADSTORE
	//  NOTE:
	//
	//  IMPORTANT:  If you move the _ResetHandler portion to a section
	//  other than .ResetVector.text that is outside the range of
	//  the reset vector's 'j' instruction, the _ResetHandler symbol
	//  and a more elaborate j/movi/jx sequence are needed in
	//  .ResetVector.text to dispatch to the new location.

	j	_ResetHandler

	.size	_ResetVector, . - _ResetVector

	.extern	__memctl_default
	.extern	__memctl_default_post
	.extern	__prefctl_default

	.align	4
	.literal_position	// tells the assembler/linker to place literals here

	.align	4
	.global	_ResetHandler
_ResetHandler:
#endif

	// Secure Boot: Mask nonsecure interrupts but enable debug interrupt

#if XCHAL_HAVE_INTERRUPTS && XCHAL_HAVE_DEBUG
	rsil	a2, XCHAL_EXCM_LEVEL		// lower PS.INTLEVEL here to make reset vector easier to debug
#endif

	// Secure Boot (step #1): verify core in secure mode; halt if nonsecure or
	// exceptions are not configured.  Can be modified to signal an external agent.

	rsr.ps	a2
	movi	a3, PS_NSM
	bnone	a2, a3, .Lin_secure_mode

.Lnot_in_secure_mode:
#if defined (SIMULATOR)
	// ISS expects exit code in a3
	movi	a3, 13
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
	j	.Lnot_in_secure_mode

.Lin_secure_mode:

	// Secure Boot (step #2): Initialize core state as usual, including caches.

	/*
	 *  Even if the processor supports the non-PC-relative L32R option,
	 *  it will always start up in PC-relative mode.  We take advantage of
	 *  this, and use PC-relative mode at least until we're sure the .lit4
	 *  section is in place (which is sometimes only after unpacking).
	 */
	.begin	no-absolute-literals

	/* There are times when we need some special-purpose code to run
	 * very early, e.g. a power-on self-test. Such code is usually
	 * supplied by the developer. We do not want to force the developer
	 * to recompile this file (or the XTOS library) so a user hook is
	 * provided for here. If a user hook is found it is called, and if
	 * the hook function returns here then normal startup is resumed.
	 * The hook function could of course take over the entire boot
	 * process and never return here.
	 *
	 * IMPORTANT: Any user hook applied here must make NO assumptions
	 * about the state of the core or the system. Notice that no core
	 * init has been done at this point.
	 */

	.weak	__reset_hook		// optional user hook

	movi	a2, __reset_hook
	beqz	a2, 1f			// no user hook
	callx0	a2			// execute user hook
1:

	// If we have dynamic cache way support, init the caches as soon
	// as we can, which is now. Except, if we are waking up from a
	// PSO event, then we need to do this slightly later.

#if XCHAL_HAVE_ICACHE_DYN_WAYS || XCHAL_HAVE_DCACHE_DYN_WAYS
# if XCHAL_HAVE_PSO_CDM && !XCHAL_HAVE_PSO_FULL_RETENTION
	// Do this later on in the code -- see below
# else
	movi	a0, __memctl_default
	wsr.memctl	a0
# endif
#endif

	// If we have PSO support, then we must check for a warm start with
	// caches left powered on. If the caches had been left powered on, 
	// we must restore the state of MEMCTL to the saved state if any.
	// Note that MEMCTL may not be present depending on config.

#if XCHAL_HAVE_PSO_CDM && !XCHAL_HAVE_PSO_FULL_RETENTION
	movi	a2, XDM_MISC_PWRSTAT		// Read PWRSTAT
	movi	a3, _pso_savearea		// Save area address - retained for later
	movi	a5, CORE_STATE_SIGNATURE	// Signature for compare - retained for later
	rer	a7, a2				// PWRSTAT value - retained for later
	extui	a4, a7, 1, 2			// Now bottom 2 bits are core wakeup and cache power lost
	bnei	a4, 1, .Lcold_start		// a4==1 means PSO wakeup, caches did not lose power
	beqz	a3, .Lcold_start		// If no PSO save area then cold start
	l32i	a4, a3, CS_SA_signature		// Load save area signature field
	sub	a4, a4, a5
	bnez	a4, .Lcold_start		// If signature mismatch then do cold start
#if XCHAL_USE_MEMCTL
	l32i	a4, a3, CS_SA_memctl		// Load saved MEMCTL value
	movi	a0, ~MEMCTL_INV_EN
	and	a0, a4, a0			// Clear invalidate bit
	wsr.memctl	a0
#endif
	j	.Lwarm_start

.Lcold_start:

#if XCHAL_HAVE_ICACHE_DYN_WAYS || XCHAL_HAVE_DCACHE_DYN_WAYS 
	// Enable and invalidate all ways of both caches. If there is no
	// dynamic way support then this write will have no effect.

	movi	a0, __memctl_default
	wsr.memctl	a0
#endif

.Lwarm_start:

#endif

	movi	a0, 0		// a0 is always 0 in this code, used to initialize lots of things

#if XCHAL_HAVE_INTERRUPTS       // technically this should be under !FULL_RESET, assuming hard reset
	wsr.intenable	a0	// make sure that interrupts are shut off (*before* we lower PS.INTLEVEL and PS.EXCM!)
#endif

#if XCHAL_HAVE_XNNE
# if XCHAL_XNNE_NUM_SBLKS > 0
	movi a2, 1
	movi a3, XNNE_TIEQ_SBLK(0) + XNNE_SBLK_DISABLE
	xnne_wr a3, a2
#  if XCHAL_XNNE_NUM_SBLKS > 1
	movi a3, XNNE_TIEQ_SBLK(1) + XNNE_SBLK_DISABLE
	xnne_wr a3, a2
#   if XCHAL_XNNE_NUM_SBLKS > 2
	movi a3, XNNE_TIEQ_SBLK(2) + XNNE_SBLK_DISABLE
	xnne_wr a3, a2
#    if XCHAL_XNNE_NUM_SBLKS > 3
	movi a3, XNNE_TIEQ_SBLK(3) + XNNE_SBLK_DISABLE
	xnne_wr a3, a2
#endif
#endif
#endif
#endif
#endif

#if XCHAL_HAVE_ABSOLUTE_LITERALS
	//  Technically, this only needs to be done under !FULL_RESET, assuming hard reset:
	wsr.litbase	a0
	rsync
#endif

#if XCHAL_HAVE_PSO_CDM && ! XCHAL_HAVE_PSO_FULL_RETENTION
	// If we're powering up from a temporary power shut-off (PSO),
	// restore state saved just prior to shut-off. Note that the
	// MEMCTL register was already restored earlier, and as a side
	// effect, registers a3, a5, a7 are now preloaded with values
	// that we will use here.
	// a3 - pointer to save area base address (_xtos_pso_savearea)
	// a5 - saved state signature (CORE_STATE_SIGNATURE)
	// a7 - contents of PWRSTAT register

	beqz	a3, 1f				// No PSO save area, cold start
	l32i	a4, a3, CS_SA_signature		// load save area signature
	sub	a4, a4, a5			// compare signature with expected one
# if XTOS_PSO_TEST
	movi	a7, PWRSTAT_WAKEUP_RESET	// pretend PSO warm start with warm caches
# endif
	bbci.l	a7, PWRSTAT_WAKEUP_RESET_SHIFT, 1f	// wakeup from PSO? (branch if not)
	//  Yes, wakeup from PSO.  Check whether state was properly saved.
	addi	a5, a7, - PWRSTAT_WAKEUP_RESET		// speculatively clear PSO-wakeup bit
	movnez	a7, a5, a4	// if state not saved (corrupted?), mark as cold start
	bnez	a4, 1f		// if state not saved, just continue with reset
	//  Wakeup from PSO with good signature.  Now check cache status:
	bbci.l	a7, PWRSTAT_CACHES_LOST_POWER_SHIFT, .Lpso_restore	// if caches warm, restore now
	//  Caches got shutoff.  Continue reset, we'll end up initializing caches, and check again later for PSO.
1:
	//  Cold start.  (Not PSO wakeup.)  Proceed with normal full reset.
#endif

#if XCHAL_HAVE_EXTERN_REGS && XCHAL_HAVE_MP_RUNSTALL
	/* On core 0, this releases other cores.  On other cores this has no effect, because
	   runstall control is unconnected.  */
	movi	a2, XER_MPSCORE
	wer	a0, a2
#endif

	// Secure Boot (step #3): Initialize and lock VECBASE.

	/*
	 *  For processors with relocatable vectors, apply any alternate
	 *  vector base given to xt-genldscripts, which sets the
	 *  _memmap_vecbase_reset symbol accordingly.
	 */
#if XCHAL_HAVE_VECBASE
	movi	a2, _memmap_vecbase_reset	/* note: absolute symbol, not a ptr */
# if XCHAL_VECBASE_LOCK
	movi	a3, VECBASE_LOCK
	or	a2, a2, a3
# endif
	wsr.vecbase	a2
#endif

#if XCHAL_HAVE_ATOMCTL
# if XCHAL_DCACHE_IS_COHERENT
	movi	a3, 0x25		/* MX -- internal for writeback, RCW otherwise */
# else
	movi	a3, 0x15		/* non-MX -- always RCW */
# endif
	wsr.atomctl	a3
#endif

	/* If either of the caches does not have dynamic way support, then
	 * use the old (slow) method to init them. If the cache is absent
	 * the macros will expand to empty.
	 */
#if ! XCHAL_HAVE_ICACHE_DYN_WAYS
	icache_reset	a2, a3
#endif
#if XCHAL_HAVE_DCACHE_DYN_WAYS
	dcache_reset_data	a2, a3, a4
#else
	dcache_reset	a2, a3, a4
#endif

#if XCHAL_HAVE_PSO_CDM && ! XCHAL_HAVE_PSO_FULL_RETENTION
	//  Here, a7 still contains status from the power status register,
	//  or zero if signature check failed.
	bbci.l	a7, PWRSTAT_WAKEUP_RESET_SHIFT, .Lcoldstart	// wakeup from PSO with good signature?
	//  Yes, wakeup from PSO.  Caches had been powered down, now are initialized.
.Lpso_restore:
	//  Assume memory still initialized, so all code still unpacked etc.
	//  So we can just jump/call to relevant state restore code (wherever located).
	movi	a4, _core_restore_nw		// restore routine address
	movi	a2, 0				// make shutoff routine return zero
	movi	a3, _pso_savearea
	beqz	a4, .Lcoldstart			// no restore routine
	callx0	a4				// does not return
.Lcoldstart:
#endif

#if XCHAL_HAVE_PREFETCH
	/* Set up PREFCTL register. By default enables cache prefetch. */
	movi	a2, __prefctl_default
	wsr.prefctl	a2
#endif

	// Secure Boot (step #4): copy SM from ROM or external secure to local secure memory.

	/*
	 * At this point we can unpack code and data (e.g. copy segments from
	 * ROM to RAM, vectors into their proper location, etc.). However,
	 *
	 * 1) the destination of the unpack may require some setup,
	 *    for instance a DDR controller may need to be initialized
	 *    and enabled before anything is unpacked into DDR.
	 * 2) users may wish to provide their own unpack code which works
	 *    faster or in a different way than the default unpack code.
	 *
	 * To support such uses, we provide a user hook at this point.
	 * If the user hook function is defined, then it is called from
	 * here, and its return value (in a2) is checked. If the return
	 * value is non-zero, then we assume that code unpacking has been
	 * completed. The user hook function must be written in assembly
	 * and should make minimal assumptions about system state.
	 */
	.weak	__reset_user_init	// optional user hook

	movi	a2, __reset_user_init
	beqz	a2, 1f			// no user hook
	callx0	a2			// execute user hook
	movi	a0, 0			// ensure a0 continues to hold 0
	bnez	a2, _unpackdone		// if a2 != 0 then unpack is done
1:

#if defined(XTOS_UNPACK)
	movi	a2, _rom_store_table
	beqz	a2, _unpackdone
_unpack:	l32i	a3, a2, 0	// start vaddr
	l32i	a4, a2, 4	// end vaddr
	l32i	a5, a2, 8	// store vaddr
	addi	a2, a2, 12
	bgeu	a3, a4, upnext	// skip unless start < end
uploop:	l32i	a6, a5, 0
	addi	a5, a5, 4
	s32i	a6, a3, 0
	addi	a3, a3, 4
	bltu	a3, a4, uploop
	j	_unpack
upnext:	bnez	a3, _unpack
	bnez	a5, _unpack
#endif /* XTOS_UNPACK */

_unpackdone:

	// Note: No need to flush DCache nor flush ICache as boot-loader space is uncached.

	/*  Now that caches are initialized, cache coherency can be enabled.  */
#if XCHAL_DCACHE_IS_COHERENT
# if XCHAL_HAVE_EXTERN_REGS && XCHAL_HAVE_MX && (XCHAL_HW_MIN_VERSION < XTENSA_HWVERSION_RE_2012_0)
	/* Opt into coherence for MX (for backward compatibility / testing).  */
	movi	a3, 1
	movi	a2, XER_CCON
	wer	a3, a2
# endif
#endif

	/*  Post init MEMCTL setup. This by default includes snoop enable and */
	/*  loop buffer enable. Can be overridden by user. */
#if XCHAL_USE_MEMCTL
	rsr.memctl	a2
	movi	a3, __memctl_default_post
	or	a2, a2, a3
#if XCHAL_ERRATUM_453
	srli	a2, a2, 1			/* clear bit 0 (ZOL buffer enable) */
	slli	a2, a2, 1
#endif
	wsr.memctl	a2
#endif

	/* Caches are all up and running, clear PWRCTL.ShutProcOffOnPWait. */
#if XCHAL_HAVE_PSO_CDM
	movi	a2, XDM_MISC_PWRCTL
	movi	a4, ~PWRCTL_CORE_SHUTOFF
	rer	a3, a2
	and	a3, a3, a4
	wer	a3, a2
#endif


	/*
	 *  Now that we know the .lit4 section is present (if got unpacked)
	 *  (and if absolute literals are used), initialize LITBASE to use it.
	 */
#if XCHAL_HAVE_ABSOLUTE_LITERALS && XSHAL_USE_ABSOLUTE_LITERALS
	/*
	 *  Switch from PC-relative to absolute (litbase-relative) L32R mode.
	 *  Set LITBASE to 256 kB beyond the start of the literals in .lit4
	 *  (aligns to the nearest 4 kB boundary, LITBASE does not have bits 1..11)
	 *  and set the enable bit (_lit4_start is assumed 4-byte aligned).
	 */
	movi	a2, _lit4_start + 0x40001
	wsr.litbase	a2
	rsync
#endif /* have and use absolute literals */
	.end	no-absolute-literals		// we can now start using absolute literals


	/*
	 *  Initialize medium and high priority interrupt dispatchers:
	 */
#if HAVE_XSR

/*  For asm macros; works for positive a,b smaller than 1000:  */
# define GREATERTHAN(a,b)	(((b)-(a)) & ~0xFFF)

# ifndef XCHAL_DEBUGLEVEL		/* debug option not selected? */
#  define XCHAL_DEBUGLEVEL	99	/* bogus value outside 2..6 */
# endif

	.macro	init_vector	level
	  .if GREATERTHAN(XCHAL_NUM_INTLEVELS+1,\level)
	    .if XCHAL_DEBUGLEVEL-\level
	      .weak	_Level&level&FromVector
	      movi	a4, _Level&level&FromVector
	      // Secure Monitor: Do not use excsaveN to maintain handler address,
	      // as it is not protected from nonsecure code modifying it.
	      .if GREATERTHAN(\level,XCHAL_EXCM_LEVEL)
	        movi	a5, _Pri_&level&_HandlerAddress
	        s32i	a4, a5, 0
	        /*  If user provides their own handler, that handler might
	         *  not provide its own _Pri_<n>_HandlerAddress variable for
	         *  linking handlers.  In that case, the reference below
	         *  would pull in the XTOS handler anyway, causing a conflict.
	         *  To avoid that, provide a weak version of it here:
	         */
	        .pushsection	.data, "aw"
	        .global	_Pri_&level&_HandlerAddress
	        .weak	_Pri_&level&_HandlerAddress
	        .align	4
	        _Pri_&level&_HandlerAddress:	.space 4
	        .popsection
	      .endif
	    .endif
	  .endif
	.endm

	init_vector	2
	init_vector	3
	init_vector	4
	init_vector	5
	init_vector	6

#endif /*HAVE_XSR*/

	// Secure Boot: Remaining steps happen in _start and unpack monitor functions

	/*
	 *  Complete reset initialization outside the vector,
	 *  to avoid requiring a vector that is larger than necessary.
	 *  This 2nd-stage startup code sets up the C Run-Time (CRT) and calls secmon_unpack().
	 *  
	 *  Here we use call0 not because we expect any return, but
	 *  because the assembler/linker dynamically sizes call0 as
	 *  needed (with -mlongcalls) which it doesn't with j or jx.
	 *  Note:  This needs to be call0 regardless of the selected ABI.
	 */
	call0	_start		// jump to _start (in crt1-*.S)
	/* does not return */


#if XCHAL_HAVE_IMEM_LOADSTORE
	.size	_ResetHandler, . - _ResetHandler
#else
	.size	_ResetVector, . - _ResetVector
#endif

	.text
	.global	xthals_hw_configid0, xthals_hw_configid1
	.global	xthals_release_major, xthals_release_minor
	.end	literal_prefix
	.end	prefer-l32r

