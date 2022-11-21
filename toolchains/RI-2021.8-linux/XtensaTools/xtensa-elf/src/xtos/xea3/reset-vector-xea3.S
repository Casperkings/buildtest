
// reset-vector-xea3.S  --  Xtensa Reset Vector/Handler for XEA3

// Copyright (c) 1999-2020 Cadence Design Systems, Inc.
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

#include <xtensa/config/core.h>
#include <xtensa/coreasm.h>
#include <xtensa/corebits.h>
#include <xtensa/cacheasm.h>
#include <xtensa/cacheattrasm.h>
#include <xtensa/xtensa-xer.h>
#include <xtensa/xdm-regs.h>
#include <xtensa/config/specreg.h>
#include <xtensa/xtruntime-core-state.h>
#include <xtensa/simcall.h>
#include "../xtos-internal.h"

#if XCHAL_HAVE_MPU
#include <xtensa/mpuasm.h>
#endif

# if XCHAL_HAVE_CACHEADRDIS
#  define IFCAD
# else
#  define IFCAD #
# endif

//-----------------------------------------------------------------------------
// Reset Vector.
//-----------------------------------------------------------------------------

	.section	.ResetVector.text, "ax"
	.global	_ResetVector
_ResetVector:
	j	_ResetHandler


//-----------------------------------------------------------------------------
// Reset Handler.
//
// IMPORTANT: this MUST be in J instruction range of .ResetVector.text .
// Otherwise the vector needs to trampoline, like this, or similar:
//	_ResetVector:	J 1f
//		.align	4
//	.Lhand:	.word	_ResetHandler
//	1:	L32R	a0, .Lhand
//		JX	a0
//-----------------------------------------------------------------------------

	.section	.ResetHandler.text, "ax"
	.literal_position		// literals go here

	.extern		__memctl_default
	.extern		__memctl_default_post
	.extern		__prefctl_default
	.weakref	_interrupt_table, _xt_interrupt_table

	//  For MPU empty background map -- see XCHAL_HAVE_MPU code further below.
	//  Cannot put this in .rodata (not unpacked before MPU init).
#if XCHAL_HAVE_MPU && XCHAL_MPU_ENTRIES >= 8 && XCHAL_MPU_BACKGROUND_ENTRIES <= 2
	.global _xtos_mpu_attribs
	.align 4
_xtos_mpu_attribs:
	.word   0x00006000+XCHAL_MPU_ENTRIES-8	// Illegal	(---)
	.word   0x000F7700+XCHAL_MPU_ENTRIES-8	// Writeback	(rwx Cacheable Non-shareable wb rd-alloc wr-alloc)
	.word   0x000D5700+XCHAL_MPU_ENTRIES-8	// WBNA		(rwx Cacheable Non-shareable wb rd-alloc)
	.word   0x000C4700+XCHAL_MPU_ENTRIES-8	// Writethru	(rwx Cacheable Non-shareable wt rd-alloc)
	.word   0x00006700+XCHAL_MPU_ENTRIES-8	// Bypass	(rwx Device non-interruptible system-shareable)
#endif

	.align	4
        .global	_ResetHandler
_ResetHandler:

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

#if XCHAL_HAVE_ICACHE_DYN_WAYS || XCHAL_HAVE_DCACHE_DYN_WAYS
	// If we have dynamic cache way support, init the caches as soon
	// as we can.  (PSO wakeup, however, needs to be processed first.)
	// Enable and invalidate all ways of both caches. If there is no
	// dynamic way support then this write will have no effect.

	movi	a0, __memctl_default
	wsr.memctl	a0
#endif

	// Keep zero in a0, used to initialize lots of things.
	movi	a0, 0

#if XCHAL_HAVE_L2_CACHE
	.extern _Xthal_L2_ctrl_init

	//  For single-image multicore, only core 0 will do L2 init.
#if XCHAL_HAVE_PRID
	xt_core_id a3
	bnez	a3, 1f
#endif
	movi	a5, XCHAL_L2_REGS_PADDR		// L2 controller base address
	movi	a3, _Xthal_L2_ctrl_init		// software-configured L2 setup (with TAGINIT set)
	s32i	a3, a5, L2CC_REG_RAM_CTRL_INIT
1:
#endif

#if XCHAL_HAVE_INTERRUPTS
	// Use the address of the interrupt handler table as the ISB/ITB base.
	// Handler table must be named exactly the same for non-XTOS applications
	// as well, for this to work. Else, set properly in user-supplied code.
# if XCHAL_HAVE_ITB
	movi	a4, _interrupt_table
	wsr.itb	a4
# elif XCHAL_HAVE_ISB
	movi	a4, _interrupt_table
	wsr.isb	a4
# endif
#endif

#if XCHAL_HAVE_VECBASE
	// Vectors are relocatable, set up vecbase to point to dispatch code.
	movi	a2, _DispatchVector		// Start of dispatch code
	wsr.vecbase	a2
#endif

	// If either of the caches does not have dynamic way support, then
	// use the old (slow) method to init them. If the cache is absent
	// the macros will expand to empty.

#if !(XCHAL_HAVE_ICACHE_DYN_WAYS || XCHAL_HAVE_CME_DOWNGRADES)
	icache_reset	a2, a3
#endif
#if XCHAL_HAVE_DCACHE_DYN_WAYS || XCHAL_HAVE_CME_DOWNGRADES
	dcache_reset_data	a2, a3, a4
#else
	dcache_reset	a2, a3, a4
#endif

#if XCHAL_HAVE_L2_CACHE
        //  For multicore, only core 0 will initialize L2
#if XCHAL_HAVE_PRID
	xt_core_id a3
	bnez	a3, 2f
#endif

#if ! XCHAL_DCACHE_IS_COHERENT
	// If the core is not coherent and the L2 cache size is 0
	// Then we can skip waiting for completion
	movi	a4, Xthal_L2cache_size
	l32i	a4, a4, 0
	beqz	a4, 2f
#endif
	//  Wait for L2 cache initialization to complete.
	//  If coherent then both the L2 Cache and RAM tags
	//  are initialized
	movi	a4, XCHAL_L2_REGS_PADDR
1:
	memw
	l32i	a3, a4, L2CC_REG_STATUS			// init done ?
	bbci.l	a3, L2CC_ST_INIT_DONE_SHIFT, 1b		// busy wait until done
#if XCHAL_DCACHE_IS_COHERENT
	movi	a3, ((1 << XCHAL_L2CC_NUM_CORES)-1) << 8 // enable coherence for all cores
	s32i	a3, a4, L2CC_REG_COHERENCE_CTRL // Store to L2 Coherence Control Register
#endif
2:
#endif

#if (XCHAL_HAVE_ICACHE_DYN_ENABLE && ! XCHAL_HAVE_ICACHE_DYN_WAYS) || \
    (XCHAL_HAVE_DCACHE_DYN_ENABLE && ! XCHAL_HAVE_DCACHE_DYN_WAYS)
	// Caches need to be both initialized and enabled.  Now enable them.
	movi	a2, __memctl_default
	wsr.memctl	a2
#endif

#if XCHAL_HAVE_PREFETCH
	/* Set up PREFCTL register. By default enables cache prefetch. */
	movi	a2, __prefctl_default
	wsr.prefctl	a2
#endif

#if XCHAL_HAVE_WINDOWED 
# ifdef __XTENSA_CALL0_ABI__
	// If we have windowing support in hw, then WB at reset will
	// have WB.S set to '01'. Disable windowing if ABI is call0.
	wsr.wb  a0
# else
	// Still set initial value in case it was a soft reset.
	movi	a2, 1
	slli	a2, a2, 30
	wsr.wb	a2
# endif
	rsync
	movi	a0, 0				// possibly a different a0, clear it
#endif

	/*
	 *  Now setup the memory attributes.  On some cores this "enables" caches.
	 *  We do this ahead of unpacking, so it can proceed more efficiently.
	 *
	 *  The _memmap_cacheattr_reset symbol's value (address) is defined
	 *  by the LSP's linker script, as generated by xt-genldscripts.
	 *  If defines 4-bit attributes for eight 512MB regions.
	 *
	 *  (NOTE:  for cores with the older MMU v1 or v2, or without any memory
	 *   protection mechanism, the following code has no effect.)
	 */
#if XCHAL_HAVE_MPU
	// If there is a user-provided MPU table, then we will program the MPU
	// with it now. Can't call xthal_write_map_raw() because code sections
	// haven't been unpacked yet. For romable images, the MPU table values
	// and the table size must reside in a section that does not need to be
	// unpacked (.ResetHandler.text or .srom.text).
	// NOTE: This will set CACHEADRDIS to all zeros, because computing a
	// useful nonzero value from the user settings is too complex and slow
	// to implement here.

	.weak	__xt_mpu_init_table		// Table of MPU entries
	.weak	__xt_mpu_init_table_size	// Number of entries in table
IFCAD	.weak	__xt_mpu_init_cacheadrdis	// value for cacheadrdis

	movi	a2, __xt_mpu_init_table		// non-zero if user defined
	movi	a3, __xt_mpu_init_table_size	// non-zero if user defined
IFCAD	movi	a4, __xt_mpu_init_cacheadrdis	// non-zero if user defined
	beqz	a2, .Lno_user_mpu
	beqz	a3, .Lno_user_mpu
	l32i	a3, a3, 0
	beqz	a3, .Lno_user_mpu		// skip if size = 0
	mpu_write_map	a2, a3, a9, a10, a11, a12, a13, a14, a15
IFCAD	beqz	a4, .Lno_default_mpu
IFCAD	l32i	a4, a4, 0
IFCAD	wsr.cacheadrdis	a4
	j	.Lno_default_mpu

.Lno_user_mpu:
	// If there's an empty background map, setup foreground maps to mimic
	// region protection.


# if XCHAL_MPU_ENTRIES >= 8 && XCHAL_MPU_BACKGROUND_ENTRIES <= 2
	// We assume reset state:  all MPU entries zeroed and disabled.
	// Otherwise we'd need a loop to zero everything.

	movi	a2, _memmap_cacheattr_reset	// note: absolute symbol, not a ptr
	movi	a3, _xtos_mpu_attribs		// see literal area at start of reset vector
	movi	a4, 0x20000000			// 512 MB delta
	movi	a6, 8
	movi	a7, 1				// MPU entry vaddr 0, with valid bit set
IFCAD	movi	a9, 0				// cacheadrdis value
IFCAD	wsr.cacheadrdis a9			// enable everything temporarily while MPU updates

        // Write eight MPU entries, from the last one going backwards (entries n-1 thru n-8)
	//
2:	extui	a8, a2, 28, 4			// get next attribute nibble (msb first)
IFCAD	extui	a5, a8, 0, 2			// lower two bit indicate whether cached
IFCAD	slli	a9, a9, 1			// add a bit to cacheadrdis...
IFCAD	addi	a10, a9, 1			// set that new bit if...
IFCAD	moveqz	a9, a10, a5			// ... that region is non-cacheable
	addx4	a5, a8, a3			// index into _xtos_mpu_attribs table
	addi	a8, a8, -5			// make valid attrib indices negative
	movgez	a5, a3, a8			// if not valid attrib, use Illegal
	l32i	a5, a5, 0			// load access rights, memtype from table entry
	slli	a2, a2, 4
	sub	a7, a7, a4			// next 512MB region (last to first)
	addi	a6, a6, -1
	add	a5, a5, a6			// add the index
	wptlb	a5, a7				// write the MPU entry
	bnez	a6, 2b				// loop until done
# else
IFCAD	movi	a9, XCHAL_MPU_BG_CACHEADRDIS	// default value of CACHEADRDIS for bgnd map
# endif
IFCAD	wsr.cacheadrdis a9			// update cacheadrdis
.Lno_default_mpu:
#elif XCHAL_HAVE_CACHEATTR || XCHAL_HAVE_MIMIC_CACHEATTR || XCHAL_HAVE_XLT_CACHEATTR \
		|| (XCHAL_HAVE_PTP_MMU && XCHAL_HAVE_SPANNING_WAY)
	movi	a2, _memmap_cacheattr_reset	/* note: absolute symbol, not a ptr */
	cacheattr_set				/* set CACHEATTR from a2 (clobbers a3-a8) */
#endif

	/*  Now that caches are initialized, cache coherency can be enabled.
	only enable coherence in memctrl register here ... L2 is done separately */
	cache_coherence_on_memctl a2, a3

	/*  Post-init MEMCTL setup. By default this includes snoop enable, loop    */
	/*  buffer enable and branch prediction enable. Can be overridden by user. */
#if XCHAL_USE_MEMCTL
	rsr.memctl	a2
	movi	a3, __memctl_default_post
	or	a2, a2, a3
	wsr.memctl	a2
#endif

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

	.weak	__reset_user_init

	movi	a2, __reset_user_init
	beqz	a2, 1f			// no user hook
	callx0	a2			// execute user hook
	movi	a0, 0			// ensure a0 continues to hold 0
	bnez	a2, unpackdone		// if a2 != 0 then unpack is done
1:

#if defined(XTOS_UNPACK)
	movi	a2, _rom_store_table
	beqz	a2, unpackdone
unpack:	l32i	a3, a2, 0	// start vaddr
	l32i	a4, a2, 4	// end vaddr
	l32i	a5, a2, 8	// store vaddr
	addi	a2, a2, 12
	bgeu	a3, a4, upnext	// skip unless start < end
uploop:	l32i 	a6, a5, 0
	addi	a5, a5, 4
	s32i	a6, a3, 0
	addi	a3, a3, 4
	bltu	a3, a4, uploop
	j	unpack
upnext:	bnez	a3, unpack
	bnez	a5, unpack
#endif /* XTOS_UNPACK */

unpackdone:

#if defined(XTOS_UNPACK) || defined(XTOS_MP)
	/*
	 *  If writeback caches are configured and enabled, unpacked data must be
	 *  written out to memory before trying to execute it:
	 */
	dcache_writeback_all	a2, a3, a4, 0
	icache_sync		a2	// ensure data written back is visible to i-fetch
	/*
	 *  Note:  no need to invalidate the i-cache after the above, because we
	 *  already invalidated it further above and did not execute anything within
	 *  unpacked regions afterwards.  [Strictly speaking, if an unpacked region
	 *  follows this code very closely, it's possible for cache-ahead to have
	 *  cached a bit of that unpacked region, so in the future we may need to
	 *  invalidate the entire i-cache here again anyway.]
	 */
#endif

	.end	no-absolute-literals		// we can now start using absolute literals

	// Set MS.DISPST to Normal, clear other MS fields

	wsr.ms  a0
	rsync

	/*
	 *  Complete reset initialization outside the vector,
	 *  to avoid requiring a vector that is larger than necessary.
	 *  This 2nd-stage startup code sets up the C Run-Time (CRT) and calls main().
	 *  
	 *  Here we use call0 not because we expect any return, but
	 *  because the assembler/linker dynamically sizes call0 as
	 *  needed (with -mlongcalls) which it doesn't with j or jx.
	 *  Note:  This needs to be call0 regardless of the selected ABI.
	 */
	call0	_start		// jump to _start (in crt1-*.S)
	/* does not return */


	.size	_ResetHandler, . - _ResetHandler

	.text
	.global xthals_hw_configid0, xthals_hw_configid1
	.global xthals_release_major, xthals_release_minor

