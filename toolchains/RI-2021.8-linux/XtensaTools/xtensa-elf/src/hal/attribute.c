/*  attribute.c - Cache attribute (memory access mode) related functions  */

/*
 * Copyright (c) 2004-2020 Cadence Design Systems Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <xtensa/config/core.h>
#if XCHAL_HAVE_EXT_CA
#include <xtensa/tie/xt_core.h>
#include <xtensa/tie/xt_mmu.h>
#endif

/* parasoft-begin-suppress MISRA2012-DIR-4_4 "Not code". */
/*
 *  Set the "cache attribute" (encoded memory access modes)
 *  of the region of memory specified by <vaddr> and <size>.
 *
 *  This function is only supported on processor configurations
 *  with region protection (or XEA1).  It has no effect on
 *  a processor configured with an MMU (with autorefill).
 *
 *  SPECIFYING THE MEMORY REGION
 *  The full (4 GB) address space may be specified with an
 *  address of zero and a size of 0xFFFFFFFF (or -1);
 *  in fact whenever <vaddr>+<size> equal 0xFFFFFFFF, <size>
 *  is interpreted as one byte greater than that specified.
 *
 *  If the specified memory range exactly covers a series
 *  of consecutive 512 MB regions, the cache attributes of
 *  these regions are updated with the requested attribute.
 *  If this is not the case, e.g. if either or both the
 *  start and end of the range only partially cover a 512 MB
 *  region, one of three results are possible:
 *
 *	1.  By default, the cache attribute of all regions
 *	    covered, even just partially, is changed to
 *	    the requested attribute.
 *
 *	2.  If the XTHAL_CAFLAG_EXACT flag is specified,
 *	    a non-zero error code is returned.
 *
 *	3.  If the XTHAL_CAFLAG_NO_PARTIAL flag is specified
 *	    (but not the EXACT flag), only regions fully
 *	    covered by the specified range are updated with
 *	    the requested attribute.
 *
 *  WRITEBACK CACHE HANDLING
 *  This function automatically writes back dirty data when
 *  switching a region from writeback mode to a non-writeback mode.
 *  This writeback is done safely, ie. by first switching to writethrough
 *  mode, then invoking xthal_dcache_all_writeback(), then switching to
 *  the selected <cattr> mode.  Such a sequence is necessary to ensure
 *  there is no longer any dirty data in the memory region by the time
 *  this function returns, even in the presence of interrupts, speculation, etc.
 *  This avoids memory coherency problems when switching from writeback
 *  to bypass mode (in bypass mode, loads go directly to memory, ignoring
 *  any dirty data in the cache; also, such dirty data can still be castout
 *  due to seemingly unrelated stores).
 *  This automatic write-back can be disabled using the XTHAL_CAFLAG_NO_AUTO_WB flag.
 *
 *  CACHE DISABLE THEN ENABLE HANDLING
 *  To avoid cache coherency issues when the cache is disabled, then
 *  memory is modified, then then cache is re-enabled (thus making
 *  visible stale cache entries), this function automatically
 *  invalidates the cache when any region switches to bypass mode.
 *  For efficiency, the entire cache is invalidated -- this is done
 *  using writeback-invalidate operations to ensure coherency even
 *  when other regions still have write-back caches enabled.
 *  This automatic invalidate can be disabled using the XTHAL_CAFLAG_NO_AUTO_INV flag.
 *
 *  Parameters:
 *	vaddr	starting virtual address of region of memory
 *
 *	size	number of bytes in region of memory
 *		(see above, SPECIFYING THE MEMORY REGION)
 *
 *	cattr	cache attribute (encoded);
 *		typically taken from compile-time HAL constants
 *		XCHAL_CA_{BYPASS[BUF], WRITETHRU, WRITEBACK[_NOALLOC], ILLEGAL}
 *		(defined in <xtensa/config/core.h>);
 *		in XEA1, this corresponds to the value of a nibble
 *		in the CACHEATTR register;
 *		in XEA2, this corresponds to the value of the
 *		cache attribute (CA) field of each TLB entry
 *
 *		On MPU configurations, the cattr is composed of accessRights
 *		and memoryType.  The accessRights occupy bits 0..3 and are
 *		typically taken from the XTHAL_AR constants.  The memory type
 *		is specified by either a bitwise or-ing of the XTHAL_MEM_...
 *		constants or if none of the XTHAL_MEM_... constants are
 *		specified, bits 4..12 are used for the memory type (that
 *		allows a cattr obtained by xthal_v2p() to be passed directly.
 *
 *		In addition on MPU configurations if the
 *		XTHAL_MPU_USE_EXISTING_MEMORY_TYPE bit is set then the existing
 *		memoryType at the first address in the region is used for the
 *		memoryType of the new region.
 *
 *		Likewise, if the XTHAL_MPU_USE_EXISTING_ACCESS_RIGHTS bit is set
 *		in cattr, then the existing accessRights at the first address
 *		in the region are used for the accessRights of the new region.
 *
 *	flags	bitwise combination of flags XTHAL_CAFLAG_*
 *		(see xtensa/hal.h for brief description of each flag);
 *		(see also various descriptions above);
 *
 *		The XTHAL_CAFLAG_EXPAND flag prevents attribute changes
 *		to regions whose current cache attribute already provide
 *		greater access than the requested attribute.
 *		This ensures access to each region can only "expand",
 *		and thus continue to work correctly in most instances,
 *		possibly at the expense of performance.  This helps
 *		make this flag safer to use in a variety of situations.
 *		For the purposes of this flag, cache attributes are
 *		ordered (in "expansion" order, from least to greatest
 *		access) as follows:
 *			XCHAL_CA_ILLEGAL	no access allowed
 *			(various special and reserved attributes)
 *			XCHAL_CA_WRITEBACK	writeback cached
 *			XCHAL_CA_WRITEBACK_NOALLOC writeback no-write-alloc
 *			XCHAL_CA_WRITETHRU	writethrough cached
 *			XCHAL_CA_BYPASSBUF	bypass with write buffering 
 *			XCHAL_CA_BYPASS		bypass (uncached)
 *		This is consistent with requirements of certain
 *		devices that no caches be used, or in certain cases
 *		that writethrough caching is allowed but not writeback.
 *		Thus, bypass mode is assumed to work for most/all types
 *		of devices and memories (albeit at reduced performance
 *		compared to cached modes), and is ordered as providing
 *		greatest access (to most devices).
 *		Thus, this XTHAL_CAFLAG_EXPAND flag has no effect when
 *		requesting the XCHAL_CA_BYPASS attribute (one can always
 *		expand to bypass mode).  And at the other extreme,
 *		no action is ever taken by this function when specifying
 *		both the XTHAL_CAFLAG_EXPAND flag and the XCHAL_CA_ILLEGAL
 *		cache attribute.
 *
 *		The XTHAL_CAFLAG_EXPAND is not supported on MPU configurations.
 *
 *  Returns:
 *	0	successful, or size is zero
 *	-1	XTHAL_CAFLAG_NO_PARTIAL flag specified and address range
 *		is valid with a non-zero size, however no 512 MB region (or page)
 *		is completely covered by the range
 *	-2	XTHAL_CAFLAG_EXACT flag specified, and address range does
 *		not exactly specify a 512 MB region (or page)
 *	-3	invalid address range specified (wraps around the end of memory)
 *	-4	function not supported in this processor configuration
 */
/* parasoft-end-suppress MISRA2012-DIR-4_4 "Not code". */
int32_t xthal_set_region_attribute( void *vaddr, uint32_t size, uint32_t cattr, uint32_t flags )// parasoft-suppress MISRA2012-RULE-8_13_a "vaddr cannont be passed 'const' due to potential cache invalidation"
{
#if !XCHAL_HAVE_XEA5 // temporary, see TENX-59798
#if XCHAL_HAVE_MPU
    if ((cattr & UINT32_C(0xffffe000)) != 0) { // check if XTHAL mem flags were supplied
        // in this case just pass cattr as the memType parameter
       return xthal_mpu_set_region_attribute(vaddr, size, cattr, cattr, flags);
    }
    else {
       // otherwise we take the bits 0-3 for accessRights and bits 4-13 as the memoryType
       return xthal_mpu_set_region_attribute(vaddr, size,
               (cattr & UINT32_C(0xf)), ((cattr & UINT32_C(0x1ff0)) >> 4), flags);
    }
#elif XCHAL_HAVE_PTP_MMU && !XCHAL_HAVE_SPANNING_WAY
    UNUSED(vaddr);
    UNUSED(size);
    UNUSED(cattr);
    UNUSED(flags);
    return -4;		/* full MMU not supported */
#else
/*  These cache attribute encodings are valid for XEA1 and region protection only:  */
# if XCHAL_HAVE_PTP_MMU
#  define CA_BYPASS		XCHAL_CA_BYPASS
# ifdef XCHAL_CA_BYPASSBUF
#  define CA_BYPASSBUF		XCHAL_CA_BYPASSBUF
# else
#  define CA_BYPASSBUF		XCHAL_CA_BYPASS
# endif
#  define CA_WRITETHRU		XCHAL_CA_WRITETHRU
#  define CA_WRITEBACK		XCHAL_CA_WRITEBACK
#  define CA_WRITEBACK_NOALLOC	XCHAL_CA_WRITEBACK_NOALLOC
#  define CA_ILLEGAL		XCHAL_CA_ILLEGAL
# else
/*  Hardcode these, because they get remapped when caches or writeback not configured:  */
#  define CA_BYPASS		2
#  define CA_BYPASSBUF		6
#  define CA_WRITETHRU		1
#  define CA_WRITEBACK		4
#  define CA_WRITEBACK_NOALLOC	5
#  define CA_ILLEGAL		15
# endif
# define CA_MASK	0xF	/*((1L<<XCHAL_CA_BITS)-1)*/	/* mask of cache attribute bits */
# define IS_CACHED(attr) (((attr) == ((uint32_t) CA_BYPASS)) ||  ((attr) == ((uint32_t) CA_BYPASSBUF)))

    int32_t start_region, end_region, i;
    uint32_t start_offset, end_vaddr, end_offset;
#if !XCHAL_HAVE_EXT_CA
    uint32_t cacheattr, cachewrtr, disabled_cache = 0;
#endif

    if (size == UINT32_C(0))
    {
	return 0;
    }
    end_vaddr = (uint32_t)vaddr + size - UINT32_C(1);
    if (end_vaddr < (uint32_t)vaddr) // parasoft-suppress MISRA2012-RULE-11_6 "Pointer conv OK"
    {
	return -3;		/* address overflow/wraparound error */
    }
    if (end_vaddr == UINT32_C(0xFFFFFFFE) /*&& (uint32_t)vaddr == 0*/ )
    {
        end_vaddr = UINT32_C(0xFFFFFFFF);       /* allow specifying 4 GB */
    }
    start_region = ((uint32_t)vaddr >> UINT32_C(29));
    start_offset = ((uint32_t)vaddr & UINT32_C(0x1FFFFFFF));
    end_region = (end_vaddr >> UINT32_C(29));
    end_offset = ((end_vaddr + UINT32_C(1)) & UINT32_C(0x1FFFFFFF));
    if ((flags & XTHAL_CAFLAG_EXACT) != UINT32_C(0)) {
	if ((start_offset != UINT32_C(0)) || (end_offset != UINT32_C(0)))
	{
	    return -2;		/* not an exact-sized range */
	}
    } else if ((flags & ((uint32_t) XTHAL_CAFLAG_NO_PARTIAL)) != UINT32_C(0)) {
	if (start_offset != UINT32_C(0))
	{
	    start_region++;
	}
	if (end_offset != UINT32_C(0))
	{
	    end_region--;
	}
	if (start_region > end_region)
	{
	    return -1;		/* nothing fully covered by specified range */
	}
    } else
    { /* Intentionally empty to satify MISRA */ }

# if XCHAL_HAVE_EXT_CA

    /* Note XTHAL_CAFLAG_EXPAND ignored for this case */

#define EXTCA_MASK    0xF0F

    cattr &= EXTCA_MASK;

    /* Directly update TLB entries in the spanning way. The assumption
       is that the spanning way is in use for 8X512MB 1-1 mapping */

# if ((XCHAL_DCACHE_SIZE > 0) && XCHAL_DCACHE_IS_WRITEBACK)
    if ((flags & XTHAL_CAFLAG_NO_AUTO_WB) == 0) {
        /* Set all the affected regions to writethrough mode
           and write back all dirty data in the dcache */
        for (i = start_region; i <= end_region; i++) {
            uint32_t as = (i << 29) | XCHAL_SPANNING_WAY;
            uint32_t at = (i << 29) | XCHAL_CA_WRITETHRU;

            XT_WITLB(at, as);
            XT_WDTLB(at, as);
        }
        XT_ISYNC();

        xthal_dcache_all_writeback();
    }
# endif

    /* Set final desired attributes */
    for (i = start_region; i <= end_region; i++) {
        uint32_t as = (i << 29) | XCHAL_SPANNING_WAY;
        uint32_t at = (i << 29) | cattr;

        XT_WITLB(at, as);
        XT_WDTLB(at, as);
    }
    XT_ISYNC();

    /* Invalidate all caches unless we were told not to */
    if ((flags & XTHAL_CAFLAG_NO_AUTO_INV) == 0) {
        xthal_dcache_all_writeback_inv();
        xthal_icache_all_invalidate();
    }

    return 0;

# else

    cacheattr = xthal_get_cacheattr();
    cachewrtr = cacheattr;
    cattr &= ((uint32_t) CA_MASK); // parasoft-suppress MISRA2012-RULE-17_8 "legacy code .. has been checked"
# if XCHAL_ICACHE_SIZE == 0 && XCHAL_DCACHE_SIZE == 0
    if ((cattr == ((uint32_t) CA_WRITETHRU)) || (cattr == ((uint32_t) CA_WRITEBACK)) || (cattr == ((uint32_t) CA_WRITEBACK_NOALLOC)))
    {
	cattr = CA_BYPASS;	/* no caches configured, only do protection */ // parasoft-suppress MISRA2012-RULE-17_8 "legacy code .. has been checked"
    }
# elif XCHAL_DCACHE_IS_WRITEBACK == 0
    if ((cattr == ((uint32_t) CA_WRITEBACK)) || (cattr == ((uint32_t) CA_WRITEBACK_NOALLOC)))
    {
        cattr = CA_WRITETHRU;	/* no writeback configured for data cache */ // parasoft-suppress MISRA2012-RULE-17_8 "legacy code .. has been checked"
    }
# endif
    for (i = start_region; i <= end_region; i++) {
	uint32_t sh = ((uint32_t) i << UINT32_C(2));		/* bit offset of nibble for region i */
	uint32_t oldattr = ((cacheattr >> sh) & ((uint32_t) CA_MASK)); // parasoft-suppress MISRA2012-RULE-12_2 "shift quantity is guaranteed to be in range"
	uint32_t newattr = cattr;
	if ((flags & (uint32_t) XTHAL_CAFLAG_EXPAND) != UINT32_C(0)) {
	    /*  This array determines whether a cache attribute can be changed
	     *  from <a> to <b> with the EXPAND flag; an attribute's "pri"
	     *  value (from this array) can only monotonically increase:  */
	    static const int8_t Xthal_ca_pri[16] = {[CA_ILLEGAL] = -1,
			[CA_WRITEBACK] = 3,
#if CA_WRITEBACK_NOALLOC != CA_WRITEBACK
			[CA_WRITEBACK_NOALLOC] = 3,
#endif
#if (CA_WRITETHRU != CA_WRITEBACK) && (CA_WRITETHRU != CA_WRITEBACK_NOALLOC)
			[CA_WRITETHRU] = 4,
#endif
#if CA_BYPASSBUF != CA_BYPASS
			  [CA_BYPASSBUF] = 8,
#endif
			  [CA_BYPASS] = 9 };
	    if (Xthal_ca_pri[newattr] < Xthal_ca_pri[oldattr])
	    {
		newattr = oldattr;	/* avoid going to lesser access */
	    }
	}
	if (IS_CACHED(newattr) && (!IS_CACHED(oldattr)))
	{
	    disabled_cache = 1;		/* we're disabling the cache for some region */
	}
# if XCHAL_DCACHE_IS_WRITEBACK
	{
	uint32_t tmpattr = newattr;
	if ((oldattr == ((uint32_t) CA_WRITEBACK)) || ((oldattr == ((uint32_t) CA_WRITEBACK_NOALLOC))
	     && (newattr != ((uint32_t) CA_WRITEBACK)) && (newattr != ((uint32_t) CA_WRITEBACK_NOALLOC))))	/* leaving writeback mode? */
	{
	    tmpattr = CA_WRITETHRU; /* leave it safely! */
	}
	cachewrtr = ((cachewrtr & ~(((uint32_t) CA_MASK) << sh)) | (tmpattr << sh)); // parasoft-suppress MISRA2012-RULE-12_2 "shift quantity is guarenteed to be in range"
	}
# endif
	cacheattr = ((cacheattr & ~(((uint32_t) CA_MASK) << sh)) | (newattr << sh)); // parasoft-suppress MISRA2012-RULE-12_2 "shift quantity is guarenteed to be in range"
    }
# if XCHAL_DCACHE_IS_WRITEBACK
    if ((cacheattr != cachewrtr)		/* need to leave writeback safely? */
	&& ((flags & ((uint32_t) XTHAL_CAFLAG_NO_AUTO_WB)) == UINT32_C(0))) {
	xthal_set_cacheattr(cachewrtr);	/* set to writethru first, to safely writeback any dirty data */
	xthal_dcache_all_writeback();	/* much quicker than scanning entire 512MB region(s) */
    }
# endif
    xthal_set_cacheattr(cacheattr);
    /*  After disabling the cache, invalidate cache entries
     *  to avoid coherency issues when later re-enabling it:  */
    if ((disabled_cache != UINT32_C(0)) && ((flags & ((uint32_t) XTHAL_CAFLAG_NO_AUTO_INV)) == UINT32_C(0))) {
	xthal_dcache_all_writeback_inv();	/* we might touch regions of memory still enabled write-back,
						   so must use writeback-invalidate, not just invalidate */
	xthal_icache_all_invalidate();
    }
    return( 0 );
# endif /* XCHAL_HAVE_EXT_CA */
#endif /* !(XCHAL_HAVE_PTP_MMU && !XCHAL_HAVE_SPANNING_WAY) */
#else /* XCHAL_HAVE_XEA5 */
    UNUSED(vaddr);
    UNUSED(size);
    UNUSED(cattr);
    UNUSED(flags);
    return 0;
#endif
}

