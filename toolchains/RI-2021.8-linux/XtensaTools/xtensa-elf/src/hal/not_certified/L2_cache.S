//
// L2_cache.S - assembly language cache management routines
//
// Copyright (c) 1999-2018 Cadence Design Systems, Inc.
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

#include <xtensa/L2-cacheasm.h>
#include <xtensa/cacheasm.h>
#include <xtensa/cacheattrasm.h>
#include <xtensa/xtensa-versions.h>
#include <xtensa/L2-cc-regs.h>
#include <xtensa/hal.h>
#include <xtensa/mpuasm.h>
#if !XCHAL_HAVE_XEA5 // todo
//----------------------------------------------------------------------
// Asynchronous cache operations
//----------------------------------------------------------------------
// Applies cache ops L1OP to L1 cache region and L2OP to L2 cache region
//	addr	starting address for region (clobbered)
//	size	size in bytes of the region (clobbered)
//	aa-ah	temporary address registers
//	a_ret	a register for return value (a2)
//	L1OP	L1OP to be applied to L1 cache
//	L2OP	L2OP to be applied to L2 cache
//	wait	if true, then the L1 completes before L2 starts
//	(needed for writeback operations)
//
//	This macro checks for the L2 being busy, size > max,
//	and addr + size > address space and returns an appropriate error code
	.macro async_dcache_region	addr, size, aa, ab, ac, ad, ae,\
		 af, ag, ah, a_ret, L1OP, L2OP, wait
#if XCHAL_HAVE_L2_CACHE
	beqz  	\size, 9f		// skip if size==0
	// do range checks
	extui \ae, \addr, 0, XCHAL_L2CACHE_LINEWIDTH	// part of address to truncate
	sub   \ad, \addr, \ae			// remove from address (masks by L2CC_OP_ADDR_MASK)
	add   \ae, \size, \ae			// add address truncation to size
	addi  \ae, \ae, -1			// Now bit 7-31 of \ae contains
								//the proper size for L2 long mode
	extui \aa, \ae, 24, 8
	bnez \aa, 7f // return w/range error size > 16MB
	add \aa, \ad, \ae // check for wraparound
	bltu \aa, \ad, 7f // return w/range error tptr + tsz > address range
#if XCHAL_HAVE_MPU
/*
	Compute optimizations base on the memory type in this section.
	This requires that the state of the caches match the cacheablility
	of the memorytype

	If the region spans multiple mpu entries, then no attempt is made to
	do this optimization.

	When completed:
	a12 is a flag that indicates that L2 is cacheable
	a11 is the memory type shifted left by 12 bits ...
		if bit 19 is set, then L1 is cacheable
*/
	movi 	\ah, 0x1 // Set value to MT with L1 and L2 enabled
	pptlb 	\ag, \addr // probe mpu
	add	\aa, \addr, \size
	addi	\aa, \aa, -1 // last byte of region
	pptlb	\af, \aa
	xor 	\af, \ag, \af
	movi	\ab, 0xC07FFc1f // Mask for valid bits of PPTLB
	and	\af, \af, \ab
	bnez	\af, 2f // If we span multiple regions set memtype (\ag)
	//to a value that is cacheable in both L1 and L2 (suppress the opt).
	// Bits 20:12 of \ag contain the memory type
 	bbsi.l	\ag, 31, 1f // if mapped by fg
	// check if we are in same background entry?
	// iterate backwards through the background entries.  If there
	// exists a background entry that is in between the start and
	// end addresses then no memory type optimization can be done
	movi	\ab, XCHAL_MPU_BACKGROUND_ENTRIES
	movi	\ac, Xthal_mpu_bgmap + (XCHAL_MPU_BACKGROUND_ENTRIES-1) * 8
	floop   \ab, loop
	l32i	\af, \ac, 0   // read vaddr from mpu entry
	addi	\af, \af, -1
	bgeu	\addr, \af, 1f // if addr >= MPU entry address
	// start and end addresses map to same bg region so optimizations
	// is possible
	bgeu	\aa, \af, 2f // if start + end - 1 (last byte of region) >=
	// MPU entry address, then
	// no optimization possible because start and end are mapped by
	// different background regions
	addi	\ac, \ac, -8
	floopend \ab, loop
1:	// [addr, addr+size) is mapped by a single MPU entry
	// optimization according to memory type is ok
	movi	\aa, 0x1f0000 // looking for device 0b00000xxxx
	and	\aa, \ag, \aa // select just the high 5 bits of mt
	beqz	\aa, 9f // MT is device ... cacheop can be skipped
	movi	\aa, 0x1f8000 // looking for non cacheable 0b000011xxx
	and 	\ab, \aa, \ag
	movi	\aa, 0x018000
	beq	\aa, \ab, 9f //MT is non-cacheable ....cacheop can be skipped
	// Now check for case where L1 is cacheable and L2 Not
	// memory type = b01xxx1001
	movi	\aa, 0x18f000
	and	\ab, \aa, \ag
	movi	\ah, 0x89000
	xor	\ah, \ah, \ab // \ah now contains non-zero iff L2 cacheOps req
	j 	3f
2:	// No opt possible
	movi 	\ah, 1
	movi 	\ag, 0xffffffff
3:
#else
 	movi	\ag, 0xffffffff // fake a memtype value that disables opt
	movi 	\ah, 1
#endif
	beqz 	\ah, 5f //test if L2 operations can be skipped
	movi  	\af, XCHAL_L2_REGS_PADDR	/* NOTE: assumes virtual=physical */
	memw
	l32i 	\ab, \af, L2CC_REG_STATUS
#if L2CC_DUAL_CTRL_REG
	bbsi.l \ab, L2CC_ST_DWNGRD_DONE_SHIFT, 4f
#else
	bbsi.l 	\ab, L2CC_ST_OP_DONE_SHIFT, 4f // check for busy
	l32i 	\ab, \af, L2CC_REG_OP_CTRL
	extui 	\ab, \ab, 0, 4
	beqi 	\ab, L2CC_OP_HIT_PREFETCH, 4f // okay to interrupt prefetch
#endif
	j 	8f // return (w/error)
4:
	movi  	\aa, L2CC_OP_SIZE_MASK // mask reserved bits
	and	  \ae, \ae, \aa
	.if \wait
	bbci.l 	\ag, 19, 6f
5: // do L1 ops before starting L2 in case of writeback/writeback_inv
	\L1OP \addr, \size, \aa, \ab, \ag
	memw  // wait for previous L1 op/writeback to complete before starting L2
	.endif

	beqz \ah, 9f //test if L2 can be skipped
6: // do final part of L2
	addi  \ad, \ad, \L2OP  // add in op bits (lower 4 bits) ...
	s32i  \ae, \af, L2CC_REG_OP_SIZE // num lines minus one
	s32i  \ad, \af, L2CC_REG_OP_DOWNGRADE // initiate L2 cache op
	.ifeq \wait // if no wait (say invalidate) L1 op done here
	// L1 and L2 operations to proceed in parallel
	bbci.l 	\ag, 19, 9f
5: // do L1 ops last in case of invalidate
	\L1OP \addr, \size, \aa, \ab, \ag
	.endif
	j 9f
7:
	movi \a_ret, XTHAL_ASYNC_RANGE
	j 10f
8:
	movi \a_ret, XTHAL_ASYNC_BUSY
	j 10f
9:
	movi \a_ret, 0
10:
# else
	\L1OP \addr, \size, \aa, \ab, \ac
	movi \a_ret, 0
#endif
	.endm


	.macro async_dcache_all	tmp1, tmp2, tmp3, regbase, a_ret, L1OP, L2OP
# if XCHAL_HAVE_L2_CACHE
	movi  \regbase, XCHAL_L2_REGS_PADDR	/* NOTE: assumes virtual=physical */
	memw
	l32i \tmp1, \regbase, L2CC_REG_STATUS
#if L2CC_DUAL_CTRL_REG
	bbsi.l \tmp1, L2CC_ST_DWNGRD_DONE_SHIFT, 1f
#else
	bbsi.l \tmp1, L2CC_ST_OP_DONE_SHIFT, 1f // if done
	l32i \tmp1, \regbase, L2CC_REG_OP_CTRL
	extui \tmp1, \tmp1, 0, 4
	beqi \tmp1, L2CC_OP_HIT_PREFETCH, 1f // okay to interrupt prefetch
#endif
	movi \a_ret, XTHAL_ASYNC_BUSY
	j 2f
1: // not busy
	\L1OP \tmp1, \tmp2, \tmp3
	movi  \tmp2, Xthal_L2cache_size
	l32i  \tmp2, \tmp2, 0
	beqz  \tmp2, 2f
	addi \tmp2, \tmp2, -XCHAL_L2CACHE_LINESIZE
	movi  \tmp1, \L2OP
	s32i  \tmp2, \regbase, L2CC_REG_OP_SIZE
	memw	// wait for L1 writeback to complete before starting L2
	s32i  \tmp1, \regbase, L2CC_REG_OP_DOWNGRADE
	movi  \a_ret, 0
2:
#else
	\L1OP \tmp1, \tmp2, \tmp3
# endif
	.endm


// Applies L2 cache region
//	addr	starting address for region (clobbered)
//	size	size in bytes of the region (clobbered)
//	aa-af	temporary address registers
//	a_ret	a register for return value (a2)
//	L2OP	L2OP to be applied to L2 cache
//
//	This macro checks for the L2 being busy, size > max,
//	and addr + size > address space and returns an appropriate error code
	.macro async_L2_region	addr, size, aa, ab, ac, ad, ae,\
		 af, a_ret, L2OP, L2CTRL
#if (XCHAL_HAVE_L2_CACHE && XCHAL_L2CACHE_LOCKABLE)
	beqz  	\size, 9f		// skip if size==0
	// do range checks
	extui \ae, \addr, 0, XCHAL_L2CACHE_LINEWIDTH	// part of address to truncate
	sub   \ad, \addr, \ae			// remove from address (masks by L2CC_OP_ADDR_MASK)
	add   \ae, \size, \ae			// add address truncation to size
	addi  \ae, \ae, -1			// Now bit 7-31 of \ae contains
								//the proper size for L2 long mode
	extui \aa, \ae, 24, 8
	bnez \aa, 7f // return w/range error size > 16MB
	add \aa, \ad, \ae // check for wraparound
	bltu \aa, \ad, 7f // return w/range error tptr + tsz > address range

	movi  	\af, XCHAL_L2_REGS_PADDR	/* NOTE: assumes virtual=physical */
	memw
	l32i 	\ab, \af, L2CC_REG_STATUS
#if L2CC_DUAL_CTRL_REG
	bbsi.l \ab, L2CC_ST_DWNGRD_DONE_SHIFT, 4f // check for busy
#else
	bbsi.l 	\ab, L2CC_ST_OP_DONE_SHIFT, 4f // check for busy
	l32i 	\ab, \af, L2CC_REG_OP_CTRL
	extui 	\ab, \ab, 0, 4
	beqi 	\ab, L2CC_OP_HIT_PREFETCH, 4f // okay to interrupt prefetch
#endif
	j 	8f // return (w/error)
4:
	movi  	\aa, L2CC_OP_SIZE_MASK // mask reserved bits
	and	  \ae, \ae, \aa
	addi  \ad, \ad, \L2OP  // add in op bits (lower 4 bits) ...
	s32i  \ae, \af, L2CC_REG_OP_SIZE // num lines minus one
	s32i  \ad, \af, \L2CTRL // initiate L2 cache op
	j 9f
7:
	movi \a_ret, XTHAL_ASYNC_RANGE
	j 10f
8:
	movi \a_ret, XTHAL_ASYNC_BUSY
	j 10f
9:
	movi \a_ret, 0
10:
	#else
	movi \a_ret, 0
#endif
	.endm

	.macro async_L2_all tmp1, tmp2, tmp3, regbase, a_ret, L2OP, L2CTRL
#if (XCHAL_HAVE_L2_CACHE && XCHAL_L2CACHE_LOCKABLE)
	movi  \regbase, XCHAL_L2_REGS_PADDR	/* NOTE: assumes virtual=physical */
	memw
	l32i \tmp1, \regbase, L2CC_REG_STATUS
#if L2CC_DUAL_CTRL_REG
	bbsi.l \tmp1, L2CC_ST_DWNGRD_DONE_SHIFT, 1f // check for busy
#else
	bbsi.l \tmp1, L2CC_ST_OP_DONE_SHIFT, 1f // if done
	l32i \tmp1, \regbase, L2CC_REG_OP_CTRL
	extui \tmp1, \tmp1, 0, 4
	beqi \tmp1, L2CC_OP_HIT_PREFETCH, 1f // okay to interrupt prefetch
#endif
	movi \a_ret, XTHAL_ASYNC_BUSY
	j 2f
1: // not busy
	movi  \tmp2, Xthal_L2cache_size
	l32i  \tmp2, \tmp2, 0
	beqz  \tmp2, 2f
	addi \tmp2, \tmp2, -XCHAL_L2CACHE_LINESIZE
	movi  \tmp1, \L2OP
	s32i  \tmp2, \regbase, L2CC_REG_OP_SIZE
	s32i  \tmp1, \regbase, \L2CTRL
	movi  \a_ret, 0
2:
#else
	movi \a_ret, 0
#endif
	.endm

// Waits for asynchronous operation to complet
	.macro async_dcache_wait aa ab
# if XCHAL_HAVE_L2_CACHE
	movi \aa, XCHAL_L2_REGS_PADDR /* NOTE: assumes virtual=physical */
#if !L2CC_DUAL_CTRL_REG
	l32i \ab, \aa, L2CC_REG_OP_CTRL // read control reg to get last op
	extui \ab, \ab, 0, 4 // extract operation type
	beqi \ab, L2CC_OP_HIT_PREFETCH, 2f // go ahead and return if a prefetch
#endif
	memw
1:
	l32i \ab, \aa, L2CC_REG_STATUS
#if L2CC_DUAL_CTRL_REG
	bbci.l \ab, L2CC_ST_DWNGRD_DONE_SHIFT, 1b
#else
	bbci.l \ab, L2CC_ST_OP_DONE_SHIFT, 1b
#endif
	bbci.l \ab, L2CC_ST_WRITE_Q_EMPTY, 1b // loop until Done and Write Q empty
2:
# endif
	.endm

//----------------------------------------------------------------------
// L2 Prefetch operations
//----------------------------------------------------------------------

//	void xthal_async_L2_prefetch(void* add, unsigned size);
#if defined(__SPLIT__L2_prefetch) ||\
	defined(__SPLIT__L2_prefetch_nw)
DECLFUNC(xthal_async_L2_prefetch)
	abi_entry
# if XCHAL_HAVE_L2_CACHE
	beqz   a3, 1f
	movi  a5, XCHAL_L2_REGS_PADDR	/* NOTE: assumes virtual=physical */
	memw // ensure that any previous cache op has had a chance to set done
	l32i a4, a5, L2CC_REG_STATUS
	bbci.l a4, L2CC_ST_OP_DONE_SHIFT, 1f /* return if busy */
	extui a4, a2, 0, XCHAL_L2CACHE_LINEWIDTH // part of address to truncate
	sub   a2, a2, a4	// remove from address (aligns to cache line size)
	add   a3, a3, a4	// add address truncation to size
	addi  a3, a3, -1	//
	addi  a2, a2, L2CC_OP_HIT_PREFETCH
	movi  a4, L2CC_OP_SIZE_MASK
	and	  a3, a3, a4
	s32i  a3, a5, L2CC_REG_OP_SIZE // size in bytes ...
	s32i  a2, a5, L2CC_REG_OP_PREFETCH
1:
# endif
	abi_return
	endfunc

//	void xthal_async_L2_prefetch_wait();
#elif 	defined(__SPLIT__L2_wait) ||\
	defined(__SPLIT__L2_wait_nw)
DECLFUNC(xthal_async_L2_wait)
DECLFUNC(xthal_async_L2_prefetch_wait) /* Deprecated */
	abi_entry
# if XCHAL_HAVE_L2_CACHE
	movi a2, XCHAL_L2_REGS_PADDR	/* NOTE: assumes virtual=physical */
	memw
1:
	l32i a3, a2, L2CC_REG_STATUS
	bbci.l a3, L2CC_ST_OP_DONE_SHIFT, 1b
# endif
	abi_return
	endfunc

//	void xthal_async_L2_prefetch_end();
#elif 	defined(__SPLIT__L2_prefetch_end) ||\
	defined(__SPLIT__L2_prefetch_end_nw)
DECLFUNC(xthal_async_L2_prefetch_end)
	abi_entry
# if XCHAL_HAVE_L2_CACHE
	movi a3, XCHAL_L2_REGS_PADDR	/* NOTE: assumes virtual=physical */
	memw
#if L2CC_DUAL_CTRL_REG
	bbsi.l a3, L2CC_ST_PREFTCH_DONE_SHIFT, 1f
#else
	l32i a2, a3, L2CC_REG_OP_CTRL
	extui a2, a2, 0, 4
	bnei a2, L2CC_OP_HIT_PREFETCH, 1f
#endif
	movi a2, XCHAL_L2_REGS_PADDR + L2CC_REG_CANCEL
	s32i a2, a2, 0 // note that we can write anything to cancel reg
1:
# endif
	abi_return
	endfunc

//----------------------------------------------------------------------
// Asynchronous cache operations
//----------------------------------------------------------------------

// int xthal_async_dcache_region_writeback(void * ptr, unsigned size, unsigned flags)
#elif 	defined(__SPLIT__async_dcache_region_writeback) ||\
	defined(__SPLIT__async_dcache_region_writeback_nw)
DECLFUNC(xthal_async_dcache_region_writeback)
	abi_entry
	async_dcache_region	addr=a2, size=a3, aa=a4, ab=a5, ac=a6, ad=a7, ae=a8, af=a9, ag=a10, ah=a11, a_ret=a2, L1OP=dcache_writeback_region, L2OP=L2CC_OP_HIT_WRITEBACK, wait=1
	abi_return
	endfunc

// int xthal_async_dcache_region_writeback_inv(void *, unsigned long, unsigned flags)
#elif 	defined(__SPLIT__async_dcache_region_writeback_inv) ||\
	defined(__SPLIT__async_dcache_region_writeback_inv_nw)
DECLFUNC(xthal_async_dcache_region_writeback_inv)
	abi_entry
	async_dcache_region	addr=a2, size=a3, aa=a4, ab=a5, ac=a6, ad=a7, ae=a8, af=a9, ag=a10, ah=a11, a_ret=a2, L1OP=dcache_writeback_inv_region, L2OP=L2CC_OP_HIT_WBACK_INV, wait=1
	abi_return
	endfunc

// int xthal_async_dcache_region_invalidate(void *, unsigned long, unsigned flags)
#elif 	defined(__SPLIT__async_dcache_region_invalidate) ||\
	defined(__SPLIT__async_dcache_region_invalidate_nw)
DECLFUNC(xthal_async_dcache_region_invalidate)
	abi_entry
	async_dcache_region	addr=a2, size=a3, aa=a4, ab=a5, ac=a6, ad=a7, ae=a8, af=a9, ag=a10, ah=a11, a_ret=a2, L1OP=dcache_invalidate_region, L2OP=L2CC_OP_HIT_INVAL, wait=0
	abi_return
	endfunc

// int xthal_async_dcache_all_writeback_inv()
#elif 	defined(__SPLIT__async_dcache_all_writeback) ||\
	defined(__SPLIT__async_dcache_all_writeback_nw)
DECLFUNC(xthal_async_dcache_all_writeback)
	abi_entry
	async_dcache_all	a2, a3, a4, a5, a2, dcache_writeback_all, L2CC_OP_INDEX_WRITEBACK
	abi_return
	endfunc

// int xthal_async_dcache_all_writeback()
#elif 	defined(__SPLIT__async_dcache_all_writeback_inv) ||\
	defined(__SPLIT__async_dcache_all_writeback_inv_nw)
DECLFUNC(xthal_async_dcache_all_writeback_inv)
	abi_entry
	async_dcache_all	a2, a3, a4, a5, a2, dcache_writeback_inv_all, L2CC_OP_INDEX_WBACK_INV
	abi_return
	endfunc

// int xthal_async_dcache_all_invalidate()
#elif 	defined(__SPLIT__async_dcache_all_invalidate) ||\
	defined(__SPLIT__async_dcache_all_invalidate_nw)
DECLFUNC(xthal_async_dcache_all_invalidate)
	abi_entry
	async_dcache_all	a2, a3, a4, a5, a2, dcache_invalidate_all, L2CC_OP_INDEX_INVAL
	abi_return
	endfunc

// void xthal_async_wait();
#elif 	defined(__SPLIT__async_dcache_wait) ||\
	defined(__SPLIT__async_dcache_wait_nw)
DECLFUNC(xthal_async_wait)
DECLFUNC(xthal_async_dcache_wait)
	abi_entry
	async_dcache_wait a2 a3
	abi_return
	endfunc

// int xthal_async_busy();
#elif 	defined(__SPLIT__async_dcache_busy) ||\
	defined(__SPLIT__async_dcache_busy_nw)
DECLFUNC(xthal_async_dcache_busy)
	abi_entry
	movi a2, 0
# if XCHAL_HAVE_L2_CACHE
	movi a4, XCHAL_L2_REGS_PADDR	/* NOTE: assumes virtual=physical */
	memw
	l32i a3, a4, L2CC_REG_STATUS // check status register for done bit
	// if done and write q empty then we are free
#if L2CC_DUAL_CTRL_REG
	movi a5, (1<<L2CC_ST_DWNGRD_DONE_SHIFT) + (1<<L2CC_ST_WRITE_Q_EMPTY)
	ball a3, a5, 1f
#else
	movi a5, (1<<L2CC_ST_OP_DONE_SHIFT) + (1<<L2CC_ST_WRITE_Q_EMPTY)
	ball a3, a5, 1f
	l32i a3, a4, L2CC_REG_OP_CTRL // read control reg to get last op
	extui a3, a3, 0, 4 // extract operation type
	beqi a3, L2CC_OP_HIT_PREFETCH, 1f
#endif
	movi a2, 1
#endif
1:
	abi_return
	endfunc

// void xthal_async_abort();
#elif 	defined(__SPLIT__async_dcache_abort) ||\
	defined(__SPLIT__async_dcache_abort_nw)
	DECLFUNC(xthal_async_dcache_abort)
	abi_entry
#if XCHAL_HAVE_L2_CACHE
	movi a3, XCHAL_L2_REGS_PADDR	/* NOTE: assumes virtual=physical */
	memw
	l32i a4, a3, L2CC_REG_STATUS
#if L2CC_DUAL_CTRL_REG
	bbsi.l a4, L2CC_ST_DWNGRD_DONE_SHIFT, 1f
#else
	l32i a2, a3, L2CC_REG_OP_CTRL // read control reg to get last op
	extui a2, a2, 0, 4 // extract operation type
	beqi a2, L2CC_OP_HIT_PREFETCH, 1f
#endif
	movi a2, XCHAL_L2_REGS_PADDR + L2CC_REG_CANCEL
	s32i a2, a2, 0 // note that we can write anything to cancel reg
1:
#endif
	abi_return
	endfunc


//----------------------------------------------------------------------
// L2 cache locking support (synchronous)
// lock prefetches into cache and locks the line into the cache
// does nothing unless a LOCKABLE L2 cache is configured
//----------------------------------------------------------------------
// (single cache line)
//----------------------------------------------------------------------

#elif 	defined(__SPLIT__L2_line_lock) || \
	defined(__SPLIT__L2_line_lock_nw)

// void xthal_L2_line_lock(void *addr);

DECLFUNC(xthal_L2_line_lock)
	abi_entry
	#if (XCHAL_HAVE_L2_CACHE && XCHAL_L2CACHE_LOCKABLE)
	L2_seq_post_line ptr=a2, tmp=a4, regbase=a5, tptr=a6, smop=L2CC_SM_LOCK_ADDR
	#endif
	abi_return
	endfunc

#elif 	defined(__SPLIT__L2_line_unlock) || \
	defined(__SPLIT__L2_line_unlock_nw)

// void xthal_L2_line_unlock(void *addr);

DECLFUNC(xthal_L2_line_unlock)
	abi_entry
	#if (XCHAL_HAVE_L2_CACHE && XCHAL_L2CACHE_LOCKABLE)
	L2_seq_post_line ptr=a2, tmp=a4, regbase=a5, tptr=a6, smop=L2CC_SM_UNLOCK_ADDR
	#endif
	abi_return
	endfunc

//----------------------------------------------------------------------
// (region)
//----------------------------------------------------------------------

#elif defined(__SPLIT__L2_region_lock) || \
          defined(__SPLIT__L2_region_lock_nw)

// void xthal_L2_region_lock( void *addr, unsigned size );

DECLFUNC(xthal_L2_region_lock)
	abi_entry
	#if (XCHAL_HAVE_L2_CACHE && XCHAL_L2CACHE_LOCKABLE)
	L2_seq_pre_region	ptr=a2, size=a3, tptr=a7, tsz=a8
	L2_seq_post_region	tmp=a4, regbase=a5, tptr=a7, tsz=a8, smop=L2CC_SM_LOCK_ADDR
	#endif
	abi_return
	endfunc

#elif defined(__SPLIT__L2_region_unlock) || \
          defined(__SPLIT__L2_region_unlock_nw)

// void xthal_L2_region_unlock( void *addr, unsigned size );

DECLFUNC(xthal_L2_region_unlock)
	abi_entry
	#if (XCHAL_HAVE_L2_CACHE && XCHAL_L2CACHE_LOCKABLE)
	L2_seq_pre_region	ptr=a2, size=a3, tptr=a7, tsz=a8
	L2_seq_post_region	tmp=a4, regbase=a5, tptr=a7, tsz=a8, smop=L2CC_SM_UNLOCK_ADDR
	#endif
	abi_return
	endfunc

#elif defined(__SPLIT__L2_all_unlock) || \
      defined(__SPLIT__L2_all_unlock_nw)

// void xthal_L2_all_unlock();

DECLFUNC(xthal_L2_all_unlock)
	abi_entry
	#if (XCHAL_HAVE_L2_CACHE && XCHAL_L2CACHE_LOCKABLE)
	L2_seq_post_all tmp1=a2, tmp2=a3, tmp3=a4, regbase=a5, smop=L2CC_SM_UNLOCK_IDX
	#endif
	abi_return
	endfunc

//----------------------------------------------------------------------
// L2 cache locking support (asynchronous)
// lock prefetches into cache and locks the line into the cache
//
// must call xthal_L2_dcache_wait() to ensure operation completes.
//----------------------------------------------------------------------

// int xthal_async_L2_region_lock(void *, unsigned size)
#elif 	defined(__SPLIT__async_L2_region_lock) ||\
	defined(__SPLIT__async_L2_region_lock_nw)
DECLFUNC(xthal_async_L2_region_lock)
	abi_entry
	#if (XCHAL_HAVE_L2_CACHE && XCHAL_L2CACHE_LOCKABLE)
	async_L2_region addr=a2, size=a3, aa=a4, ab=a5, ac=a6, ad=a7, ae=a8, af=a9, a_ret=a2, L2OP=L2CC_OP_HIT_LOCK, L2CTRL=L2CC_REG_OP_PREFETCH
	#else
	movi a2, 0
	#endif
	abi_return
	endfunc

// int xthal_async_L2_region_unlock(void *, unsigned size)
#elif 	defined(__SPLIT__async_L2_region_unlock) ||\
	defined(__SPLIT__async_L2_region_unlock_nw)
DECLFUNC(xthal_async_L2_region_unlock)
	abi_entry
	#if (XCHAL_HAVE_L2_CACHE && XCHAL_L2CACHE_LOCKABLE)
	async_L2_region addr=a2, size=a3, aa=a4, ab=a5, ac=a6, ad=a7, ae=a8, af=a9, a_ret=a2, L2OP=L2CC_OP_HIT_UNLOCK, L2CTRL=L2CC_REG_OP_PREFETCH
	#else
	movi a2, 0
	#endif
	abi_return
	endfunc

// int xthal_async_L2_all_unlock()
#elif 	defined(__SPLIT__async_L2_all_unlock) ||\
	defined(__SPLIT__async_L2_all_unlock_nw)
DECLFUNC(xthal_async_L2_all_unlock)
	abi_entry
	#if (XCHAL_HAVE_L2_CACHE && XCHAL_L2CACHE_LOCKABLE)
	async_L2_all	a2, a3, a4, a5, a2, L2CC_OP_INDEX_UNLOCK, L2CTRL=L2CC_REG_OP_PREFETCH
	#else
	movi a2, 0
	#endif
	abi_return
	endfunc

// int xthal_L2ram_init()
#elif 	defined(__SPLIT__L2ram_init) ||\
	defined(__SPLIT__L2ram_init_nw)
DECLFUNC(xthal_L2ram_init)
	abi_entry
#if XCHAL_HAVE_L2_CACHE && XCHAL_L2CC_HAVE_ECC
	movi	a2, Xthal_L2ram_size
	l32i	a2, a2, 0	// Read L2 Ram Size
	srli	a2, a2, 2	// Get # Words
	movi	a3, XCHAL_L2_REGS_PADDR
	l32i	a3, a3, L2CC_REG_RAM_CTRL // L2Ram Address is top 16 bits
	movi	a4, L2CC_RAM_BASE_MASK
	and	a3, a3, a4
	floop	a2, .Ll2init
	l32i	a4, a3, 0 // Read and write entire L2 Ram
	s32i	a4, a3, 0
	addi	a3, a3, 4
	floopend a2, .Ll2init
#endif
	movi	a2, 0
	abi_return
	endfunc

// int xthal_L2ram_ecc_enable();
#elif 	defined(__SPLIT__L2ram_ecc_enable) ||\
	defined(__SPLIT__L2ram_ecc_enable_nw)
DECLFUNC(xthal_L2ram_ecc_enable)
		abi_entry
#if XCHAL_HAVE_L2_CACHE && XCHAL_L2CC_HAVE_ECC
	movi	a2, XCHAL_L2_REGS_PADDR
	l32i	a3, a2, L2CC_REG_TOP_CTRL
	movi	a4, L2CC_TOP_MEMORY_ECC_ENABLE
	or	a3, a3, a4
	s32i	a3, a2, L2CC_REG_TOP_CTRL
#endif
	movi	a2, 0
	abi_return
	endfunc
#endif

#if defined(__SPLIT__write_map_helper) && XCHAL_HAVE_L2_CACHE

/*
  void  xthal_write_map_helper_l2( const struct xthal_MPU_entry* map, unsigned n);

  Updates the MPU with the MPU entries provided:
	map	pointer to array of MPU entries
	n	number of entries in array (0 <= n <= XCHAL_MPU_ENTRIES)

  The entries provided must have monotonically increasing addresses.
  This function otherwise orders its updates to ensure the MPU always has
  all its entries in monotonically increasing sequence.

  on entry
  	a2 		=>	vector of MPU entries to write
  	a3		=>	number of entries to write
  	a4-a8	=>	destroyed
*/

DECLFUNC (xthal_write_map_helper_l2)
	abi_entry
	async_dcache_all	a4, a5, a6, a7, a4, dcache_writeback_inv_all, L2CC_OP_INDEX_WBACK_INV
	async_dcache_wait	a4, a5
	icache_invalidate_all a6, a7
	mpu_write_map a2, a3, a4, a5, a6, a7, a8 a9, a10
	isync
	abi_return
	endfunc

#endif
#endif
