
// reset-vector-xea5.S  --  Xtensa Reset Vector/Handler for XEA5

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


//-----------------------------------------------------------------------------
// Reset Vector.
//-----------------------------------------------------------------------------

	.section	.ResetVector.text, "ax"
	.global	_ResetVector
_ResetVector:
	la	t0, _ResetHandler
	jr	t0


//-----------------------------------------------------------------------------
// Reset Handler.
//-----------------------------------------------------------------------------

	.section	.ResetHandler.text, "ax"

	.extern		__memctl_default
	.extern		__memctl_default_post
	.extern		__prefctl_default
	.weakref	_interrupt_table, _xt_interrupt_table

	// PMPPMA attributes

	.p2align	2
        .global	_ResetHandler
_ResetHandler:

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

	la	t0, __reset_hook
	beq	t0, x0, 1f		// no user hook
	jalr	t0			// execute user hook
1:

#if XCHAL_HAVE_ICACHE_DYN_WAYS || XCHAL_HAVE_DCACHE_DYN_WAYS
	// FIXME
#endif

#if XCHAL_HAVE_L2_CACHE
	.extern _Xthal_L2_ctrl_init

	//  For single-image multicore, only core 0 will do L2 init.
#if XCHAL_HAVE_PRID
	xt_core_id t0
	bne	t0, x0, 1f
#endif
	li	t0, XCHAL_L2_REGS_PADDR		// L2 controller base address
	li	t1, _Xthal_L2_ctrl_init		// software-configured L2 setup (with TAGINIT set)
	sw	t1, t0, L2CC_REG_RAM_CTRL_INIT
1:
#endif

	la	t0, _xtos_dispatch
	csrw	mtvec, t0			// set up mtvec

	la	t0, _interrupt_table
	csrw	mtvt, t0			// set up sw vector table

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
	xt_core_id t0
	bne	t0, x0, 2f
#endif
	// FIXME future
2:
#endif

	// FIXME set mmemctl/uprefctl for caches as required.

	// FIXME set up PMPPMA. For now, set up like so:
	// entry 0: from addr 0 up to sysram start, uncached
	// entry 1: span sysram, cached
	// entry 2: from sysram end to top of mem, uncached

	csrr	t0, mmmiobase
	li	t1, 0x00402400
	add	t0, t0, t1		// t0 <- PMA regs base addr

	la	t1, _memmap_mem_sram_start
	srli	t1, t1, 2
	li	t3, 0x06
	csrw	pmpaddr0, t1
	sh	t3, t0, 0

	la	t1, _memmap_mem_sram_end
	srli	t1, t1, 2
	li	t3, 0xd0
	csrw	pmpaddr1, t1
	sh	t3, t0, 2

	li	t1, 0xFFFFFFFF
	li	t3, 0x06
	csrw	pmpaddr2, t1
	sh	t3, t0, 4

	li	t2, 0x004F4F4F		// Enable RWX, set bit 6 for DMA/GS
	csrw	pmpcfg0, t2		// pmpcfg0 has 4 entries 
	fence	15, 15

	// If there is a user-provided MPU table, then we will program the MPU
	// with it now. Can't call xthal_write_map_raw() because code sections
	// haven't been unpacked yet. For romable images, the MPU table values
	// and the table size must reside in a section that does not need to be
	// unpacked (.ResetHandler.text or .srom.text).
	// NOTE: This will set CACHEADRDIS to all zeros, because computing a
	// useful nonzero value from the user settings is too complex and slow
	// to implement here.
#if 0
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
IFCAD	wsr.cacheadrdis a9			// update cacheadrdis
.Lno_default_mpu:
#endif

	/*  Post-init MEMCTL setup. By default this includes snoop enable, loop    */
	/*  buffer enable and branch prediction enable. Can be overridden by user. */
#if 0 // FIXME XCHAL_USE_MEMCTL
	rsr.memctl	a2
	movi	a3, __memctl_default_post
	or	a2, a2, a3
	wsr.memctl	a2
#endif

	// At this point we can unpack code and data (e.g. copy segments from
	// ROM to RAM, vectors into their proper location, etc.). However,
	//
	// 1) the destination of the unpack may require some setup,
	//    for instance a DDR controller may need to be initialized
	//    and enabled before anything is unpacked into DDR.
	// 2) users may wish to provide their own unpack code which works
	//    faster or in a different way than the default unpack code.
	//
	// To support such uses, we provide a user hook at this point.
	// If the user hook function is defined, then it is called from
	// here, and its return value (in a2) is checked. If the return
	// value is non-zero, then we assume that code unpacking has been
	// completed. The user hook function must be written in assembly
	// and should make minimal assumptions about system state.

	.weak	__reset_user_init

	la	t0, __reset_user_init
	beq	t0, x0, 1f		// no user hook
	jalr	t0			// execute user hook
	bne	a0, x0, unpackdone	// if a0 != 0 then unpack is done
1:

#if defined(XTOS_UNPACK)
	la	t0, _rom_store_table
	beq	t0, x0, unpackdone
unpack:
	lw	t1, t0, 0	// start vaddr
	lw	t2, t0, 4	// end vaddr
	lw	t3, t0, 8	// store vaddr
	addi	t0, t0, 12
	bgeu	t1, t2, upnext	// skip unless start < end
uploop:
	lw 	t4, t3, 0
	addi	t3, t3, 4
	sw	t4, t1, 0
	addi	t1, t1, 4
	bltu	t1, t2, uploop
	j	unpack
upnext:
	bne	t1, x0, unpack
	bne	t3, x0, unpack
#endif /* XTOS_UNPACK */

unpackdone:

#if defined(XTOS_UNPACK) || defined(XTOS_MP)
	// If writeback caches are configured and enabled, unpacked data must be
	// written out to memory before trying to execute it:

	dcache_writeback_all	t0, t1, t2, 0
	icache_sync		t0	// ensure data written back is visible to i-fetch
#endif

	// Jump to _start to finish remaining initialization
	// (e.g. CRT setup) and call main.

	la	t0, _start
	jr	t0

	.size	_ResetHandler, . - _ResetHandler

	.text
	.global xthals_hw_configid0, xthals_hw_configid1
	.global xthals_release_major, xthals_release_minor

