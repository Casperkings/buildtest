//
// cache_asm.S - assembly language cache management routines
//
// $Id: //depot/dev/Foxhill/Xtensa/OS/hal/cache_asm.S#12 $

// Copyright (c) 1999-2015 Cadence Design Systems, Inc.
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

#include <xtensa/cacheasm.h>
#include <xtensa/cacheattrasm.h>
#include <xtensa/xtensa-versions.h>

	.begin	schedule	// turn on assembler scheduling for this file

//----------------------------------------------------------------------
//  Huge Range cache macros
//----------------------------------------------------------------------

	//  void  xthal_dcache_hugerange_<name>(void *addr, uint32_t size);
	//
	//  Invalidate and/or writeback dcache entries for an arbitrary large
	//  virtual address range with a single scan of the dcache.
	//  Assumes no address translation, i.e. virtual = physical.
	//
	//  a2 = ptr to range
	//  a3 = size of range
	//
	//  Note:  -128 is a valid immediate for ADDI, but +128 is not,
	//  and ADDI can relax to ADDMI for multiples of 256.  So scanning
	//  cache backwards (from end to start) allows all cache line sizes
	//  without creating an extra instruction for the ADDI.
	//
	.macro dcache_hugefunc  name, instruction, doit=1
	.text
	.align	4
	.type	xthal_dcache_hugerange_\name,@function
	.global	xthal_dcache_hugerange_\name
xthal_dcache_hugerange_\name:
	abi_entry
#if  XCHAL_DCACHE_SIZE > 0 \
	&& XCHAL_HAVE_DCACHE_TEST && \
	(XCHAL_HAVE_MINMAX || XCHAL_HAVE_NX) && \
	XCHAL_HAVE_LOOPS && \
	!XCHAL_HAVE_XLT_CACHEATTR && !XCHAL_HAVE_PTP_MMU && \
	((XCHAL_HAVE_DEPBITS && (XCHAL_DCACHE_LINES_PER_TAG_LOG2 < 2)) || \
	!XCHAL_HAVE_NX) && !XCHAL_HAVE_L2_CACHE && !XCHAL_HAVE_XEA5//todo
	.ifne \doit
	movi	a4, XCHAL_DCACHE_SIZE*2		// size at which to use huge algorithm
	movi	a7, -XCHAL_DCACHE_LINESIZE	// for rounding to cache line size
	bltu	a3, a4, 7f			// use normal (line-by-line hit) function
#if XCHAL_HAVE_PREFETCH
	movi	a11, 0
	xsr.prefctl a11		// temporarily disable prefetch (invalidates prefetch bufs!)
#endif
	add	a5, a3, a2			// a5 = end of range
	and	a4, a2, a7			// a4 = low end, rounded to containing cache line
#if	!XCHAL_HAVE_NX
	addi	a5, a5, /*XCHAL_DCACHE_LINESIZE*/-1
	and	a5, a5, a7			// a5 = high end, rounded to containing cache line
	movi	a7, XCHAL_DCACHE_SIZE/XCHAL_DCACHE_LINESIZE	// a7 = number of lines in dcache
	movi	a3, XCHAL_DCACHE_SIZE-XCHAL_DCACHE_LINESIZE	// way index
	mov	a6, a5
	movi	a8, -(1 << (XCHAL_DCACHE_SETWIDTH + XCHAL_DCACHE_LINEWIDTH))	// use if LDCT gives non-zero index bits
	movi	a10, (XCHAL_DCACHE_SIZE/XCHAL_DCACHE_WAYS) - 1

	loopgtz a7, 1f
	ldct	a7, a3				// a3 = cache tag for cache entry [a7]
	\instruction	a2, 0
	.begin schedule
	//extui	a9, a3, 0, XCHAL_DCACHE_SETWIDTH+XCHAL_DCACHE_LINEWIDTH
	and	a9, a3, a10
	addi	a3, a3, -XCHAL_DCACHE_LINESIZE
	.end schedule
	.begin schedule
	and	a7, a7, a8	// uncomment if LDCT reports non-zero index bits
	maxu	a6, a6, a4	// a4 = low end of range
	minu	a2, a6, a5	// a5 = high end of range
	or	a6, a7, a9
	.end schedule
1:

	\instruction	a2, 0
	maxu	a6, a6, a4
	minu	a2, a6, a5
	\instruction	a2, 0
#else
/*	NX efficient cache operations for very large regions:
	(region size > 2 * cache size)

	Functionally equivalent to xthal_dcache_region_XXX

	Only works in the 1 or 2 lines / tag case ...

	Constraints:
		1 or 2 cache lines x tag

	On entry:
		a2 -- region start
		a3 -- region size in bytes

	Implementation notes:

	1) optimum cache op performance requires 5 cycles between operations to
	the same tag. The penalty occurs even if the address in question is not
	in the cache.

	2) The general algorithum is to iterate through the cache by cache index,
	and do the cache op for any address that falls in the specified range.
	Thus the total time is proportional to the cache size rather than the
	region size.

	3) For improved performance:
		a) We process 4 cache tags (8 lines) each time through the loop
		(this allows the address calculation instructions to be flixed
		and the overhead halved).
		b) each time through the loop we:
		   i) do the cache op on the 2nd line from the previous tag
		   ii) caclculate the addresses for the new tag
		   iii) do the cache op on the 1st line from the new tag
		   This allows about 5 cycles between cacheops to the same tag
		c) Rather than explictly testing if the cache entries are valid
		or in the applicable range we calculate the addr for the tag and
		then use conditional move to use the address iterator if the
		cache tag address isn't in the specified region.
		d) To avoid having consecutive access to the same tag use an address
		iterator (for each iteration) as the address for the cacheop when
		the tag address is outside the region.
		e) To make all this work during setup we do the cache op on the
		lines at the start and end of the region such that the operating
		region (during the loop) is aligned to a double cache line boundary
		and that the first two and last two lines withing the operating
		region have already had the cache op done, before entering the loop.

	Reg
	a0: holds mask to get the way/index from the iterator
	a2: on entry start / later normalized start (start of operating region)
	a3: on entry length / later normalized end (end of operating region)
	a4: iteration 0: address for next cacheop
	a5: iteration 0: address/way/index iterator
	a6: iteration 0: temp
	a7: iteration 1: address for next cacheop
	a8: iteration 1: address/way/index iterator
	a9: iteration 1: temp
	a10: iteration 2: address for next cacheop
	a11: iteration 2: address/way/index iterator
	a12: iteration 2: temp
	a13: iteration 3: address for next cacheop
	a14: iteration 3: address/way/index iterator
	a15: iteration 3: temp

*/

#define NX_INDEX_BITS (XCHAL_DCACHE_SIZE_LOG2 - XCHAL_DCACHE_WAYS_LOG2)
#define NX_CACHE_TAG_ADDRESS_BITS (32 - NX_INDEX_BITS)
#define NX_SUPERLINE (XCHAL_DCACHE_LINESIZE * (1 << XCHAL_DCACHE_LINES_PER_TAG_LOG2))
#define LOOP_DECREMENT (-4 * NX_SUPERLINE)
#ifdef __XTENSA_CALL0_ABI__
	// registers a0-a12 are needed ... callee responsible for saving a12
	addi	a1, a1, -24
	s32i    a11, a1, 0
	s32i    a12, a1, 4
	s32i    a13, a1, 8
	s32i    a14, a1, 12
	s32i    a15, a1, 16
	s32i	a0, a1, 20
#else
	addi	a1, a1, -8
	s32i    a11, a1, 4
	s32i	a0, a1, 0
#endif
	addi	a5, a5, -1
	and	a3, a5, a7 // a5 = high end, rounded to containing cache line
	mov a2, a4

	movi	a8, XCHAL_DCACHE_SIZE - 1
	add	a7, a2, a8
	srli	a7, a7, XCHAL_DCACHE_SIZE_LOG2
	slli	a7, a7, XCHAL_DCACHE_SIZE_LOG2

	movi	a6, XCHAL_DCACHE_SIZE/ NX_SUPERLINE/ 4
	movi	a5, (XCHAL_DCACHE_SIZE - NX_SUPERLINE)
	add a5, a5, a7
	movi	a8, (XCHAL_DCACHE_SIZE - 2 * NX_SUPERLINE)
	add a8, a8, a7
	movi	a11, (XCHAL_DCACHE_SIZE - 3 * NX_SUPERLINE)
	add a11, a11, a7
	movi	a14, (XCHAL_DCACHE_SIZE - 4 * NX_SUPERLINE)
	add a14, a14, a7

#if (XCHAL_DCACHE_LINES_PER_TAG_LOG2 == 1)
	/* At this point a2 contains address of 1st line to be operated
	   on and a3 contains the address of the last line to be operated on

	   We need to inialize a4, a7, a10, and a13 (the cacheop registers)
	   to addresses within the region because the first time through the
	   loop, they will be used to do a cache-op before new operation
	   addresses are calculated from the cache tag.  We might as well
	   set them up to operate on the first and last lines of the region
	   so we eliminate any corner conditions (due to dual line tags).
	*/

	addi a4, a3, -XCHAL_DCACHE_LINESIZE
	mov  a10, a4
	addi     a7, a2, -XCHAL_DCACHE_LINESIZE
	mov     a13, a7
#else
	// we don't get free init in case of 1 line / tag
	// need to operate on last line of region
	\instruction a3, 0
#endif
	movi a0, (XCHAL_DCACHE_SIZE - 1)
	loopgtz a6, 1f
	and a6, a5, a0
	and a9, a8, a0
	and a12, a11, a0
	and a15, a14, a0
	ldct	a6, a6
	ldct	a9, a9
	ldct	a12, a12
	ldct	a15, a15
#if NX_INDEX_BITS <= 16
/* If way size is <= 64k then we can use depbits directly */
	depbits a6, a5, 0, NX_INDEX_BITS
	depbits a9, a8, 0, NX_INDEX_BITS
	depbits a12, a11, 0, NX_INDEX_BITS
	depbits a15, a14, 0, NX_INDEX_BITS
#elif NX_INDEX_BITS <= XCHAL_DCACHE_LINE_WIDTH + 16
/* If way size is <= 4M then we can shift to the right by
   the cache line width (and then use the shift operand
   to depbits to shift back to the left */
	movi a0, (-1) - ((1<<NX_INDEX_BITS) - 1)
	srli a0, a5, XCHAL_DCACHE_LINE_WIDTH
	depbits a6, a0, XCHAL_DCACHE_LINE_WIDTH, NX_INDEX_BITS - XCHAL_DCACHE_LINE_WIDTH
	srli a0, a8, XCHAL_DCACHE_LINE_WIDTH
	depbits a9, a0, XCHAL_DCACHE_LINE_WIDTH, NX_INDEX_BITS - XCHAL_DCACHE_LINE_WIDTH
	srli a0, a11, XCHAL_DCACHE_LINE_WIDTH
	depbits a12, a0, XCHAL_DCACHE_LINE_WIDTH, NX_INDEX_BITS - XCHAL_DCACHE_LINE_WIDTH
	srli a0, a14, XCHAL_DCACHE_LINE_WIDTH
	depbits a15, a0, XCHAL_DCACHE_LINE_WIDTH, NX_INDEX_BITS - XCHAL_DCACHE_LINE_WIDTH
	movi a0, XCHAL_DCACHE_SIZE - 1
#else
#error "unsupported cache size"
#endif
#if (XCHAL_DCACHE_LINES_PER_TAG_LOG2 == 1)
	\instruction a4, XCHAL_DCACHE_LINESIZE
	\instruction a7, XCHAL_DCACHE_LINESIZE
	\instruction a10, XCHAL_DCACHE_LINESIZE
	\instruction a13, XCHAL_DCACHE_LINESIZE
#endif
.begin schedule
	addi.a a4, a6, 0
	addi.a a7, a9, 0
	addi.a a10, a12, 0
	addi.a a13, a15, 0
	sub a6, a4, a2
	movltz a4, a5, a6
	sub a9, a7, a2
	movltz a7, a8, a9
	sub a12, a10, a2
	movltz a10, a11, a12
	sub a15, a13, a2
	movltz a13, a14, a15
	sub a6, a4, a3
	movgez a4, a5, a6
	sub a9, a7, a3
	movgez a7, a8, a9
	sub a12, a10, a3
	movgez a10, a11, a12
	sub a15, a13, a3
	movgez a13, a14, a15
	\instruction a4, 0
	\instruction a7, 0
	\instruction a10, 0
	\instruction a13, 0
	addi.a a5, a5, LOOP_DECREMENT
	addi.a a8, a8, LOOP_DECREMENT
	addi.a a11, a11, LOOP_DECREMENT
	addi.a a14, a14, LOOP_DECREMENT
.end schedule
1:
#if (XCHAL_DCACHE_LINES_PER_TAG_LOG2 == 1)
	\instruction a4, XCHAL_DCACHE_LINESIZE
	\instruction a7, XCHAL_DCACHE_LINESIZE
	\instruction a10, XCHAL_DCACHE_LINESIZE
	\instruction a13, XCHAL_DCACHE_LINESIZE
#endif

#ifdef __XTENSA_CALL0_ABI__
	l32i	a0, a1, 20
	l32i    a15, a1, 16
	l32i    a14, a1, 12
	l32i    a13, a1, 8
	l32i    a12, a1, 4
	l32i    a11, a1, 0
	addi	a1, a1, 24

#else
	l32i    a11, a1, 4
	l32i	a0, a1, 0
	addi	a1, a1, 8
#endif
#endif
#if XCHAL_HAVE_PREFETCH
	wsr.prefctl a11		// restore prefetch
#endif
	isync_return_nop
	abi_return
	.endif
#endif /* dcache supports hugerange */
// Jump to non-huge routine
#if XCHAL_HAVE_XEA5
7:	jal	xthal_dcache_region_\name + ABI_ENTRY_MINSIZE
#else
7:	j.l	xthal_dcache_region_\name + ABI_ENTRY_MINSIZE, a4
#endif
	.size xthal_dcache_hugerange_\name, . - xthal_dcache_hugerange_\name
	.endm



	//  void  xthal_icache_hugerange_<name>(void *addr, uint32_t size);
	//
	//  Invalidate icache entries for an arbitrary large
	//  virtual address range with a single scan of the icache.
	//  Assumes no address translation, i.e. virtual = physical.
	//
	//  a2 = ptr to range
	//  a3 = size of range
	//
	//  Note:  -128 is a valid immediate for ADDI, but +128 is not,
	//  and ADDI can relax to ADDMI for multiples of 256.  So scanning
	//  cache backwards (from end to start) allows all cache line sizes
	//  without creating an extra instruction for the ADDI.
	//
	.macro icache_hugefunc  name, instruction, doit=1
	.text
	.align	4
	.type	xthal_icache_hugerange_\name,@function
	.global	xthal_icache_hugerange_\name
xthal_icache_hugerange_\name:
	abi_entry
#if !XCHAL_HAVE_NX && XCHAL_ICACHE_SIZE > 0 && \
	XCHAL_HAVE_ICACHE_TEST && XCHAL_HAVE_MINMAX && XCHAL_HAVE_LOOPS && !XCHAL_HAVE_XLT_CACHEATTR && ! XCHAL_HAVE_XEA5 // todo
	.ifne \doit
	movi	a4, XCHAL_ICACHE_SIZE*2		// size at which to use huge algorithm
	movi	a7, -XCHAL_ICACHE_LINESIZE	// for rounding to cache line size
	bltu	a3, a4, 7f			// use normal (line-by-line hit) function
	add	a5, a3, a2			// a5 = end of range
	and	a4, a2, a7			// a4 = low end, rounded to containing cache line
	addi	a5, a5, XCHAL_ICACHE_LINESIZE-1
	and	a5, a5, a7			// a5 = high end, rounded to containing cache line
	movi	a7, XCHAL_ICACHE_SIZE/XCHAL_ICACHE_LINESIZE	// a7 = number of lines in dcache
	movi	a3, XCHAL_ICACHE_SIZE-XCHAL_ICACHE_LINESIZE	// way index
	mov	a6, a5
	//movi	a8, -XCHAL_ICACHE_SETSIZE	// use if LICT gives non-zero index bits
	movi	a10, (XCHAL_ICACHE_SIZE/XCHAL_ICACHE_WAYS) - 1

	loopgtz a7, 1f
	lict	a7, a3				// a3 = cache tag for cache entry [a7]
	\instruction	a2, 0
	.begin schedule
	//extui	a9, a3, 0, XCHAL_ICACHE_SETWIDTH+XCHAL_ICACHE_LINEWIDTH
	and	a9, a3, a10
	addi	a3, a3, -XCHAL_ICACHE_LINESIZE
	.end schedule
	.begin schedule
	//and	a7, a7, a8	// uncomment if LDCT reports non-zero index bits
	maxu	a6, a6, a4	// a4 = low end of range
	minu	a2, a6, a5	// a5 = high end of range
	or	a6, a7, a9
	.end schedule
1:

	\instruction	a2, 0
	maxu	a6, a6, a4
	minu	a2, a6, a5
	\instruction	a2, 0
	isync_return_nop
	abi_return
	.endif
#endif /* icache supports hugerange */
#if XCHAL_HAVE_XEA5
7:	jal	xthal_icache_region_\name + ABI_ENTRY_MINSIZE
#else
7:	j.l	xthal_icache_region_\name + ABI_ENTRY_MINSIZE, a4
#endif
	.size xthal_icache_hugerange_\name, . - xthal_icache_hugerange_\name
	.endm


#if defined(__SPLIT__icache_hugerange_invalidate)

// void xthal_icache_hugerange_invalidate( void *addr, uint32_t size );
icache_hugefunc	invalidate,	ihi

#elif defined(__SPLIT__icache_hugerange_unlock)

// void xthal_icache_hugerange_unlock( void *addr, uint32_t size );
icache_hugefunc	unlock,		ihu, XCHAL_ICACHE_LINE_LOCKABLE


#elif defined(__SPLIT__dcache_hugerange_invalidate)

// void xthal_dcache_hugerange_invalidate( void *addr, uint32_t size );
dcache_hugefunc	invalidate, dhi

#elif defined(__SPLIT__dcache_hugerange_unlock)

// void xthal_dcache_hugerange_unlock( void *addr, uint32_t size );
dcache_hugefunc	unlock,	dhu, doit=XCHAL_DCACHE_LINE_LOCKABLE

#elif defined(__SPLIT__dcache_hugerange_writeback)

// void xthal_dcache_hugerange_writeback( void *addr, uint32_t size );
dcache_hugefunc	writeback,	dhwb

#elif defined(__SPLIT__dcache_hugerange_writeback_inv)

// void xthal_dcache_hugerange_writeback_inv( void *addr, uint32_t size );
dcache_hugefunc	writeback_inv,	dhwbi
#endif
.end	schedule
