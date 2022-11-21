// 
// cache_asm.S - assembly language cache management routines
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

//----------------------------------------------------------------------
//----------------------------------------------------------------------
// invalidate the icache
//----------------------------------------------------------------------

#if defined(__SPLIT__icache_all_invalidate) || \
    defined(__SPLIT__icache_all_invalidate_nw)

// void xthal_icache_all_invalidate(void);

DECLFUNC(xthal_icache_all_invalidate)
	abi_entry
	icache_invalidate_all	a2, a3
	isync_return_nop
	abi_return
	endfunc

//----------------------------------------------------------------------
// invalidate the dcache
//----------------------------------------------------------------------

#elif (defined(__SPLIT__dcache_all_invalidate) || \
      defined(__SPLIT__dcache_all_invalidate_nw) ) && \
      !(XCHAL_HAVE_L2_CACHE && XCHAL_DCACHE_IS_COHERENT)

// void xthal_dcache_all_invalidate(void);

DECLFUNC(xthal_dcache_all_invalidate)
	abi_entry
	L2_par_pre_all		tmp1=a3, regaddr=a7, size=a5, savesize=a6, smop=L2CC_SM_INV_IDX

	dcache_invalidate_all	a2, a3, a4
	L2_par_post_all		tmp1=a3, regaddr=a7, size=a5, savesize=a6

	abi_return
	endfunc

//----------------------------------------------------------------------
// write dcache dirty data
//----------------------------------------------------------------------

#elif (defined(__SPLIT__dcache_all_writeback) || \
      defined(__SPLIT__dcache_all_writeback_nw)) && \
      !(XCHAL_HAVE_L2_CACHE && XCHAL_DCACHE_IS_COHERENT)

// void xthal_dcache_all_writeback(void);

DECLFUNC(xthal_dcache_all_writeback)
	abi_entry
	dcache_writeback_all	a2, a3, a4
	L2_seq_post_all		tmp1=a2, tmp2=a3, tmp3=a4, regbase=a5, smop=L2CC_SM_WB_IDX
	abi_return
	endfunc

//----------------------------------------------------------------------
// write dcache dirty data and invalidate
//----------------------------------------------------------------------

#elif (defined(__SPLIT__dcache_all_writeback_inv) || \
      defined(__SPLIT__dcache_all_writeback_inv_nw)) && \
      !(XCHAL_HAVE_L2_CACHE && XCHAL_DCACHE_IS_COHERENT)

// void xthal_dcache_all_writeback_inv(void);

DECLFUNC(xthal_dcache_all_writeback_inv)
	abi_entry
	dcache_writeback_inv_all	a2, a3, a4
	L2_seq_post_all		tmp1=a2, tmp2=a3, tmp3=a4, regbase=a5, smop=L2CC_SM_WB_INV_IDX
	abi_return
	endfunc

#elif (defined(__SPLIT__dcache_L1_all_writeback) || \
      defined(__SPLIT__dcache_L1_all_writeback_nw))
//----------------------------------------------------------------------
// flush dirty data out of L1 only
//----------------------------------------------------------------------

// void xthal_dcache_L1_all_writeback(void);

DECLFUNC(xthal_dcache_L1_all_writeback)
	abi_entry
	dcache_writeback_all	a2, a3, a4
	abi_return
	endfunc

//----------------------------------------------------------------------
// flush and invalidate dirty data out of L1 only
//----------------------------------------------------------------------

#elif (defined(__SPLIT__dcache_L1_all_writeback_inv) || \
      defined(__SPLIT__dcache_L1_all_writeback_inv_nw))

// void xthal_dcache_L1_all_writeback_inv(void);

DECLFUNC(xthal_dcache_L1_all_writeback_inv)
	abi_entry
	dcache_writeback_inv_all	a2, a3, a4
	abi_return
	endfunc

//----------------------------------------------------------------------
// invalidate data in L1 only
//----------------------------------------------------------------------

#elif (defined(__SPLIT__dcache_L1_all_invalidate) || \
      defined(__SPLIT__dcache_L1_all_invalidate_nw))

// void xthal_dcache_L1_all_invalidate(void);

DECLFUNC(xthal_dcache_L1_all_invalidate)
	abi_entry
	dcache_invalidate_all	a2, a3, a4
	abi_return
	endfunc

//----------------------------------------------------------------------
// unlock instructions from icache
//----------------------------------------------------------------------

#elif defined(__SPLIT__icache_all_unlock) || \
      defined(__SPLIT__icache_all_unlock_nw)

// void xthal_icache_all_unlock(void);

DECLFUNC(xthal_icache_all_unlock)
	abi_entry
	icache_unlock_all	a2, a3
	abi_return
	endfunc

//----------------------------------------------------------------------
// unlock data from dcache
//----------------------------------------------------------------------

#elif defined(__SPLIT__dcache_all_unlock) || \
      defined(__SPLIT__dcache_all_unlock_nw)

// void xthal_dcache_all_unlock(void);

DECLFUNC(xthal_dcache_all_unlock)
	abi_entry
	dcache_unlock_all	a2, a3
	abi_return
	endfunc

//----------------------------------------------------------------------
// invalidate the address range in the icache
//----------------------------------------------------------------------

#elif defined(__SPLIT__icache_region_invalidate) || \
      defined(__SPLIT__icache_region_invalidate_nw)

// void xthal_icache_region_invalidate( void *addr, unsigned size );

DECLFUNC(xthal_icache_region_invalidate)
	abi_entry
	icache_invalidate_region	a2, a3, a4
	isync_return_nop
	abi_return
	endfunc

//----------------------------------------------------------------------
// invalidate the address range in the dcache
//----------------------------------------------------------------------

#elif defined(__SPLIT__dcache_region_invalidate) || \
          defined(__SPLIT__dcache_region_invalidate_nw)

// void xthal_dcache_region_invalidate( void *addr, unsigned size );

DECLFUNC(xthal_dcache_region_invalidate)
	abi_entry
	L2_par_pre_region	ptr=a2, size=a3, temp=a4, savesize=a8, regbase=a7, tptr=a6, tsz=a5, smop=L2CC_SM_INV_ADDR
	dcache_invalidate_region	a2, a3, a4, a5, a6
	L2_par_post		regbase=a7, tmp=a4, savesize=a8
	abi_return
	endfunc

//----------------------------------------------------------------------
// write dcache region dirty data
//----------------------------------------------------------------------

#elif defined(__SPLIT__dcache_region_writeback) || \
          defined(__SPLIT__dcache_region_writeback_nw)

// void xthal_dcache_region_writeback( void *addr, unsigned size );

DECLFUNC(xthal_dcache_region_writeback)
	abi_entry
	L2_seq_pre_region	ptr=a2, size=a3, tptr=a7, tsz=a8
	dcache_writeback_region		a2, a3, a4, a5, a6
	L2_seq_post_region tmp=a4, regbase=a5, tptr=a7, tsz=a8, smop=L2CC_SM_WB_ADDR
	abi_return
	endfunc

//----------------------------------------------------------------------
// write dcache region dirty data and invalidate
//----------------------------------------------------------------------

#elif defined(__SPLIT__dcache_region_writeback_inv) || \
	  defined(__SPLIT__dcache_region_writeback_inv_nw)

// void xthal_dcache_region_writeback_inv( void *addr, unsigned size );

DECLFUNC(xthal_dcache_region_writeback_inv)
	abi_entry
	L2_seq_pre_region	ptr=a2, size=a3, tptr=a7, tsz=a8
	dcache_writeback_inv_region	a2, a3, a4, a5, a6
	L2_seq_post_region tmp=a4, regbase=a5, tptr=a7, tsz=a8, smop=L2CC_SM_WB_INV_ADDR
	abi_return
	endfunc


//----------------------------------------------------------------------
// L1-only invalidate the address range in the dcache
//----------------------------------------------------------------------

#elif defined(__SPLIT__dcache_L1_region_invalidate) || \
          defined(__SPLIT__dcache_L1_region_invalidate_nw)

// void xthal_dcache_L1_region_invalidate( void *addr, unsigned size );

DECLFUNC(xthal_dcache_L1_region_invalidate)
	abi_entry
	dcache_invalidate_region	a2, a3, a4, a5, a6
	abi_return
	endfunc

//----------------------------------------------------------------------
// L1-only write dcache region dirty data
//----------------------------------------------------------------------

#elif defined(__SPLIT__dcache_L1_region_writeback) || \
          defined(__SPLIT__dcache_L1_region_writeback_nw)

// void xthal_dcache_L1_region_writeback( void *addr, unsigned size );

DECLFUNC(xthal_dcache_L1_region_writeback)
	abi_entry
	dcache_writeback_region		a2, a3, a4, a5, a6
	abi_return
	endfunc

//----------------------------------------------------------------------
// L1-only write dcache region dirty data and invalidate
//----------------------------------------------------------------------

#elif defined(__SPLIT__dcache_L1_region_writeback_inv) || \
	  defined(__SPLIT__dcache_L1_region_writeback_inv_nw)

// void xthal_dcache_L1_region_writeback_inv( void *addr, unsigned size );

DECLFUNC(xthal_dcache_L1_region_writeback_inv)
	abi_entry
	dcache_writeback_inv_region	a2, a3, a4, a5, a6
	abi_return
	endfunc



//----------------------------------------------------------------------
// lock instructions in icache region
//----------------------------------------------------------------------

#elif defined(__SPLIT__icache_region_lock) || \
	  defined(__SPLIT__icache_region_lock_nw)

// void xthal_icache_region_lock(void);

DECLFUNC(xthal_icache_region_lock)
	abi_entry
	icache_lock_region	a2, a3, a4
	abi_return
	endfunc

//----------------------------------------------------------------------
// lock data in dcache region
//----------------------------------------------------------------------

#elif defined(__SPLIT__dcache_region_lock) || \
	  defined(__SPLIT__dcache_region_lock_nw)

// void xthal_dcache_region_lock(void);

DECLFUNC(xthal_dcache_region_lock)
	abi_entry
	dcache_lock_region	a2, a3, a4
	abi_return
	endfunc

//----------------------------------------------------------------------
// unlock instructions from icache region
//----------------------------------------------------------------------

#elif defined(__SPLIT__icache_region_unlock) || \
	  defined(__SPLIT__icache_region_unlock_nw)

// void xthal_icache_region_unlock(void);

DECLFUNC(xthal_icache_region_unlock)
	abi_entry
	icache_unlock_region	a2, a3, a4
	abi_return
	endfunc

//----------------------------------------------------------------------
// unlock data from dcache region
//----------------------------------------------------------------------

#elif defined(__SPLIT__dcache_region_unlock) || \
	  defined(__SPLIT__dcache_region_unlock_nw)

// void xthal_dcache_region_unlock(void);

DECLFUNC(xthal_dcache_region_unlock)
	abi_entry
	dcache_unlock_region	a2, a3, a4
	abi_return
	endfunc

//----------------------------------------------------------------------
// invalidate single icache line
//----------------------------------------------------------------------

#elif	defined(__SPLIT__icache_line_invalidate) || \
	defined(__SPLIT__icache_line_invalidate_nw)

// void xthal_icache_line_invalidate(void *addr);

DECLFUNC(xthal_icache_line_invalidate)
	abi_entry
	icache_invalidate_line	a2, 0
	isync_return_nop
	abi_return
	endfunc


//----------------------------------------------------------------------
// invalidate single dcache line
//----------------------------------------------------------------------

#elif 	defined(__SPLIT__dcache_line_invalidate) || \
	defined(__SPLIT__dcache_line_invalidate_nw)

// void xthal_dcache_line_invalidate(void *addr);

DECLFUNC(xthal_dcache_line_invalidate)
	abi_entry
	L2_par_pre_line	ptr=a2, savesize=a4, regbase=a5, tptr=a6, smop=L2CC_SM_INV_ADDR
	dcache_invalidate_line	a2, 0
	L2_par_post	regbase=a5, tmp=a3, savesize=a4
	abi_return
	endfunc

//----------------------------------------------------------------------
// write single dcache line dirty data
//----------------------------------------------------------------------

#elif 	defined(__SPLIT__dcache_line_writeback) || \
	defined(__SPLIT__dcache_line_writeback_nw)

// void xthal_dcache_line_writeback(void *addr);

DECLFUNC(xthal_dcache_line_writeback)
	abi_entry
	dcache_writeback_line	a2, 0
	L2_seq_post_line ptr=a2, tmp=a4, regbase=a5, tptr=a6, smop=L2CC_SM_WB_ADDR
	abi_return
	endfunc

//----------------------------------------------------------------------
// write single dcache line dirty data and invalidate
//----------------------------------------------------------------------

#elif 	defined(__SPLIT__dcache_line_writeback_inv) || \
	defined(__SPLIT__dcache_line_writeback_inv_nw)

// void xthal_dcache_line_writeback_inv(void *addr);

DECLFUNC(xthal_dcache_line_writeback_inv)
	abi_entry
	dcache_writeback_inv_line	a2, 0
	L2_seq_post_line ptr=a2, tmp=a4, regbase=a5, tptr=a6, smop=L2CC_SM_WB_INV_ADDR
	abi_return
	endfunc

//----------------------------------------------------------------------
// lock instructions in icache line
//----------------------------------------------------------------------

#elif 	defined(__SPLIT__icache_line_lock) || \
	defined(__SPLIT__icache_line_lock_nw)

// void xthal_icache_line_lock(void);

DECLFUNC(xthal_icache_line_lock)
	abi_entry
	icache_lock_line	a2, 0
	abi_return
	endfunc

//----------------------------------------------------------------------
// lock data in dcache line
//----------------------------------------------------------------------

#elif 	defined(__SPLIT__dcache_line_lock) || \
	defined(__SPLIT__dcache_line_lock_nw)

// void xthal_dcache_line_lock(void);

DECLFUNC(xthal_dcache_line_lock)
	abi_entry
	dcache_lock_line	a2, 0
	abi_return
	endfunc

//----------------------------------------------------------------------
// unlock instructions from icache line
//----------------------------------------------------------------------

#elif 	defined(__SPLIT__icache_line_unlock) || \
	defined(__SPLIT__icache_line_unlock_nw)

// void xthal_icache_line_unlock(void);

DECLFUNC(xthal_icache_line_unlock)
	abi_entry
	icache_unlock_line	a2, 0
	abi_return
	endfunc

//----------------------------------------------------------------------
// unlock data from dcache line
//----------------------------------------------------------------------

#elif 	defined(__SPLIT__dcache_line_unlock) || \
	defined(__SPLIT__dcache_line_unlock_nw)

// void xthal_dcache_line_unlock(void);

DECLFUNC(xthal_dcache_line_unlock)
	abi_entry
	dcache_unlock_line	a2, 0
	abi_return
	endfunc

//----------------------------------------------------------------------
// sync icache and memory
//----------------------------------------------------------------------

#elif 	defined(__SPLIT__icache_sync) || \
	defined(__SPLIT__icache_sync_nw)

// void xthal_icache_sync(void);

DECLFUNC(xthal_icache_sync)
	abi_entry
	icache_sync	a2
	isync_return_nop
	abi_return
	endfunc

//----------------------------------------------------------------------
// sync dcache and memory
//----------------------------------------------------------------------

#elif 	defined(__SPLIT__dcache_sync) || \
	defined(__SPLIT__dcache_sync_nw)

// void xthal_dcache_sync(void);

DECLFUNC(xthal_dcache_sync)
	abi_entry
	dcache_sync	a2
	abi_return
	endfunc

//----------------------------------------------------------------------
// Get/Set icache number of ways enabled
//----------------------------------------------------------------------

#elif 	defined (__SPLIT__icache_get_ways) || \
	defined (__SPLIT__icache_get_ways_nw)

// unsigned int xthal_icache_get_ways(void);

DECLFUNC(xthal_icache_get_ways)
	abi_entry
	icache_get_ways	a2
	abi_return
	endfunc


#elif 	defined (__SPLIT__icache_set_ways) || \
	defined(__SPLIT__icache_set_ways_nw)

/// void xthal_icache_set_ways(unsigned int ways);

DECLFUNC(xthal_icache_set_ways)
	abi_entry
	icache_set_ways	a2 a3 a4
	abi_return
	endfunc

//----------------------------------------------------------------------
// Get/Set dcache number of ways enabled
//----------------------------------------------------------------------

#elif 	defined (__SPLIT__dcache_get_ways) || \
	defined (__SPLIT__dcache_get_ways_nw)

// unsigned int xthal_dcache_get_ways(void);

DECLFUNC(xthal_dcache_get_ways)
	abi_entry
	dcache_get_ways a2
	abi_return
	endfunc

#elif 	defined (__SPLIT__dcache_set_ways) || \
	defined (__SPLIT__dcache_set_ways_nw)

// void xthal_dcache_set_ways(unsigned int ways);

DECLFUNC(xthal_dcache_set_ways)
	abi_entry
	dcache_set_ways a2 a3 a4
	abi_return
	endfunc

//----------------------------------------------------------------------
// opt into and out of coherence
//----------------------------------------------------------------------

#elif 	defined(__SPLIT__cache_coherence_on) || \
	defined(__SPLIT__cache_coherence_on_nw)

// The opt-in routine assumes cache was initialized at reset,
// so it's equivalent to the low-level coherence_on routine.

// void xthal_cache_coherence_optin(void)
// void xthal_cache_coherence_on(void)

DECLFUNC(xthal_cache_coherence_optin)
DECLFUNC(xthal_cache_coherence_on)
	abi_entry
	cache_coherence_on	a2, a3, a4
	abi_return
	endfunc
	

#elif 	defined(__SPLIT__cache_coherence_off) || \
	defined(__SPLIT__cache_coherence_off_nw)

// The coherence_off routines should not normally be called directly.
// Use the xthal_cache_coherence_optout() C routine instead
// (which first empties the cache).

// void xthal_cache_coherence_off

DECLFUNC(xthal_cache_coherence_off)
	abi_entry
	cache_coherence_off	a2, a3, a4
	abi_return
	endfunc
	

//----------------------------------------------------------------------
// Control cache prefetch
//----------------------------------------------------------------------

#elif 	defined(__SPLIT__set_cache_prefetch_long) || \
	defined(__SPLIT__set_cache_prefetch_long_nw)

# if XCHAL_HAVE_BE
#  define aH a2	/* msb word = prefctl mask */
#  define aL a3 /* lsb word = prefctl value */
# else
#  define aH a3	/* msb word = prefctl mask */
#  define aL a2 /* lsb word = prefctl value */
# endif

// Set cache prefetch state (-1=enable, 0=disable, and see XTHAL_*PREFETCH_*),
// and return previous one.
//
// int  xthal_set_cache_prefetch_long( unsigned long long );
//
DECLFUNC(xthal_set_cache_prefetch_long)
	abi_entry
# if XCHAL_HAVE_PREFETCH
#  if XCHAL_HAVE_XEA5
	addi	a0, x0, 0
#  else
	movi	a5, XCHAL_CACHE_PREFCTL_DEFAULT
	addi	a4, aL, 1	// does prefctl value aL == -1 ?
	moveqz	aL, a5, a4	// if yes (XTHAL_PREFETCH_ENABLE), set it to default
	movgez  a2, aL, aL	// if the high bit is not set, then we want to transfer the contents of aL to prefctl
				// so we move it to a2
	bgez	aL, 1f		// high bit set indicates masked update
	ssai	16		// 16-bit right shifts
	src	a5, aL, aH	// get 16-bit-swapped 32-bit value
	src	a5, a5, a5	// get 32-bit value (rotate by 16)
	rsr.prefctl a4
	src	a3, aH, aL	// get 32-bit mask
	or	a4, a4, a3	// set masked bits
	xor	a4, a4, a3	// clear masked bits
	and	a5, a5, a3	// only use masked bits
	or	a2, a4, a5	// combine masked bits
1:
#   if XCHAL_HW_MIN_VERSION <= XTENSA_HWVERSION_RC_2010_1    /* for erratum #325 */
	j 1f
	.align 8
1:
	xsr.prefctl a2
	isync	// ensure XSR.PREFCTL;ISYNC wholly within an icache line
#   else
	xsr.prefctl a2
	isync
#   endif
#  endif
# else
#if XCHAL_HAVE_XEA5
	addi	a0, x0, 0
#else
	movi	a2, 0
#endif
# endif
	abi_return
	endfunc

//----------------------------------------------------------------------


#elif 	defined(__SPLIT__set_cache_prefetch) || \
	defined(__SPLIT__set_cache_prefetch_nw)

// FOR BACKWARD COMPATIBILITY WITH PRE-RF RELEASE OBJECT CODE ONLY.
// Set cache prefetch state (-1=enable, 0=disable, and see the
//   definitions of XTHAL_*PREFETCH_* with only the lower 32 bits set),
// and return previous one.
// int  xthal_set_cache_prefetch( int )
//
DECLFUNC(xthal_set_cache_prefetch)
	abi_entry
# if XCHAL_HAVE_PREFETCH
#  if XCHAL_HAVE_XEA5
	addi	a0, x0, 0
#  else
	movi	a3, XCHAL_CACHE_PREFCTL_DEFAULT
	addi	a4, a2, 1	// does a2 == -1 ?
	moveqz	a2, a3, a4	// if yes (XTHAL_PREFETCH_ENABLE), set it to default
	bbci.l	a2, 31, 1f	// high bit set indicates masked update
	rsr.prefctl a4
	extui	a5, a2, 16, 15
	or	a4, a4, a5	// set masked bits
	xor	a4, a4, a5	// clear masked bits
	and	a2, a2, a5	// only use masked bits
	or	a2, a4, a2	// combine masked bits
1:
#   if XCHAL_HW_MIN_VERSION <= XTENSA_HWVERSION_RC_2010_1    /* for erratum #325 */
	j 1f
	.align 8
1:
	xsr.prefctl a2
	isync	// ensure XSR.PREFCTL;ISYNC wholly within an icache line
#   else
	xsr.prefctl a2
	isync
#   endif
#  endif
# else
#if XCHAL_HAVE_XEA5
	addi	a0, x0, 0
#else
	movi	a2, 0
#endif
# endif
	abi_return
	endfunc

//----------------------------------------------------------------------

#elif 	defined(__SPLIT__get_cache_prefetch) ||\
	defined(__SPLIT__get_cache_prefetch_nw)

// Return current cache prefetch state.
// int  xthal_get_cache_prefetch( void )
DECLFUNC(xthal_get_cache_prefetch)
	abi_entry
# if XCHAL_HAVE_PREFETCH
#  if XCHAL_HAVE_XEA5
	addi	a0, x0, 0
#  else
	rsr.prefctl a2
#  endif
# else
#if XCHAL_HAVE_XEA5
	addi	a0, x0, 0
#else
	movi	a2, 0
#endif
# endif
	abi_return
	endfunc



//----------------------------------------------------------------------
// Misc configuration info
//----------------------------------------------------------------------
	
// Eventually these will move to their own file:
#elif defined(__SPLIT__hw_configid0)
	.set	xthals_hw_configid0, XCHAL_HW_CONFIGID0
#elif defined(__SPLIT__hw_configid1)
	.set	xthals_hw_configid1, XCHAL_HW_CONFIGID1
#elif defined(__SPLIT__release_major)
	.set	xthals_release_major, XTHAL_RELEASE_MAJOR
#elif defined(__SPLIT__release_minor)
	.set	xthals_release_minor, XTHAL_RELEASE_MINOR

#endif /*split*/

	.global	xthals_hw_configid0, xthals_hw_configid1
	.global	xthals_release_major, xthals_release_minor

//----------------------------------------------------------------------



