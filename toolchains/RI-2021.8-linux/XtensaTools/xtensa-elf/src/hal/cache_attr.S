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




//----------------------------------------------------------------------
// Read CACHEATTR register
//----------------------------------------------------------------------

#if defined(__SPLIT__get_cacheattr) ||\
    defined(__SPLIT__get_cacheattr_nw)

//  uint32_t xthal_get_cacheattr(void);

DECLFUNC(xthal_get_cacheattr)
DECLFUNC(xthal_get_dcacheattr)
# if XCHAL_HAVE_CACHEATTR	/* single CACHEATTR register used for both I and D */
DECLFUNC(xthal_get_icacheattr)
# endif
	abi_entry
# if XCHAL_HAVE_EXT_CA
	movi	a2, 0
# else
	dcacheattr_get
# endif
	abi_return
	endfunc

#elif defined(__SPLIT__get_icacheattr) ||\
      defined(__SPLIT__get_icacheattr_nw)

//  uint32_t xthal_get_icacheattr(void);

# if !XCHAL_HAVE_CACHEATTR	/* possibly independent CACHEATTR states used for I and D */
DECLFUNC(xthal_get_icacheattr)
	abi_entry
# if XCHAL_HAVE_EXT_CA
	movi	a2, 0
# else
	icacheattr_get
# endif
	abi_return
	endfunc
# endif

#endif /*split*/


//----------------------------------------------------------------------
//  Write CACHEATTR register, or equivalent.
//----------------------------------------------------------------------

/*
 *  Set CACHEATTR register in a safe manner.
 *
 *	void  xthal_set_cacheattr( uint32_t new_cacheattr );
 *	void  xthal_set_icacheattr( uint32_t new_cacheattr );
 *	void  xthal_set_dcacheattr( uint32_t new_cacheattr );
 */

#if 	defined(__SPLIT__set_cacheattr) ||\
	defined(__SPLIT__set_cacheattr_nw)

# if XCHAL_HAVE_CACHEATTR	/* single CACHEATTR register used for both I and D accesses */
DECLFUNC(xthal_set_icacheattr)
DECLFUNC(xthal_set_dcacheattr)
# endif
DECLFUNC(xthal_set_cacheattr)
	abi_entry
# if XCHAL_HAVE_EXT_CA
	/* Do nothing */
# else
	cacheattr_set
# endif
	abi_return
	endfunc

#endif /*split*/


#if XCHAL_HAVE_CACHEATTR

	/*
	 *  Already done above.
	 *
	 *  Since we can't enable/disable the icache and dcache independently,
	 *  and don't have a nice place to store a state which would enable
	 *  us to only enable them both when both have been requested to be
	 *  enabled, we simply enable both for any request to enable either,
	 *  and disable both for any request to disable either cache.
	 */

#elif XCHAL_HAVE_MIMIC_CACHEATTR || XCHAL_HAVE_XLT_CACHEATTR || (XCHAL_HAVE_PTP_MMU && XCHAL_HAVE_SPANNING_WAY)

# if 	defined(__SPLIT__set_icacheattr) \
	|| defined(__SPLIT__set_icacheattr_nw)

DECLFUNC(xthal_set_icacheattr)
	abi_entry
# if XCHAL_HAVE_EXT_CA
	/* Do nothing */
# else
	icacheattr_set
	isync_return_nop
# endif
	abi_return
	endfunc

#elif 	defined(__SPLIT__set_dcacheattr) \
	|| defined(__SPLIT__set_dcacheattr_nw)

DECLFUNC(xthal_set_dcacheattr)
	abi_entry
# if XCHAL_HAVE_EXT_CA
	/* Do nothing */
# else
	dcacheattr_set
# endif
	abi_return
	endfunc

# endif /*split*/

#else /* full MMU (pre-v3): */

# if 	defined(__SPLIT__set_idcacheattr) \
	|| defined(__SPLIT__set_idcacheattr_nw)

//  These functions aren't applicable to arbitrary MMU configurations.
//  Do nothing in this case.

DECLFUNC(xthal_set_icacheattr)
DECLFUNC(xthal_set_dcacheattr)
	abi_entry
	abi_return
	endfunc

# endif /*split*/

#endif /* cacheattr/MMU type */


//----------------------------------------------------------------------
// Determine (guess) whether caches are "enabled"
//----------------------------------------------------------------------

/*
 *  There is no "cache enable" bit in the Xtensa architecture,
 *  but we can use CACHEATTR (if it or its equivalent exists)
 *  as an indication that caches have been enabled.
 */

#if XCHAL_HAVE_CACHEATTR

# if defined(__SPLIT__idcache_is_enabled) || \
     defined(__SPLIT__idcache_is_enabled_nw)

DECLFUNC(xthal_icache_is_enabled)
DECLFUNC(xthal_dcache_is_enabled)
	abi_entry
	cacheattr_is_enabled	2f
	movi	a2, 0
	abi_return
2:	movi	a2, 1
	abi_return
	endfunc

# endif /*split*/

#elif XCHAL_HAVE_MIMIC_CACHEATTR || XCHAL_HAVE_XLT_CACHEATTR

# if defined(__SPLIT__icache_is_enabled) || \
     defined(__SPLIT__icache_is_enabled_nw)

DECLFUNC(xthal_icache_is_enabled)
	abi_entry
	icacheattr_is_enabled	2f
	movi	a2, 0
	abi_return
2:	movi	a2, 1
	abi_return
	endfunc

# elif defined(__SPLIT__dcache_is_enabled) || \
       defined(__SPLIT__dcache_is_enabled_nw)

DECLFUNC(xthal_dcache_is_enabled)
	abi_entry
	dcacheattr_is_enabled	2f
	movi	a2, 0
	abi_return
2:	movi	a2, 1
	abi_return
	endfunc

# endif /*split*/

#else

//  These functions aren't applicable to arbitrary MMU configurations.
//  Assume caches are enabled in this case (!).

# if defined(__SPLIT__idcache_is_enabled) || \
     defined(__SPLIT__idcache_is_enabled_nw)

DECLFUNC(xthal_icache_is_enabled)
DECLFUNC(xthal_dcache_is_enabled)
	abi_entry
#if XCHAL_HAVE_XEA5
	addi	a0, x0, 1
#else
	movi	a2, 1
#endif
	abi_return
	endfunc
# endif /*split*/

#endif


