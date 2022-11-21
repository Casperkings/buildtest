// 
// cache.c -- cache management routines
//
// $Id: //depot/rel/Homewood/ib.8/Xtensa/OS/hal/not_certified/cache.c#1 $

// Copyright (c) 2002-2016 Tensilica Inc.
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
#include <xtensa/core-macros.h>
#include <xtensa/L2-cc-regs.h>
#include <xtensa/hal.h>
#define UNUSED(x) ((void)(x))

#if defined(__SPLIT__consts)

// size of the cache lines in log2(bytes)
const uint8_t Xthal_icache_linewidth = XCHAL_ICACHE_LINEWIDTH;
const uint8_t Xthal_dcache_linewidth = XCHAL_DCACHE_LINEWIDTH;

// size of the cache lines in bytes
const uint16_t Xthal_icache_linesize = XCHAL_ICACHE_LINESIZE;
const uint16_t Xthal_dcache_linesize = XCHAL_DCACHE_LINESIZE;

// number of cache sets in log2(lines per way)
const uint8_t Xthal_icache_setwidth = XCHAL_ICACHE_SETWIDTH;
const uint8_t Xthal_dcache_setwidth = XCHAL_DCACHE_SETWIDTH;

// cache set associativity (number of ways)
const uint32_t Xthal_icache_ways = XCHAL_ICACHE_WAYS;
const uint32_t Xthal_dcache_ways = XCHAL_DCACHE_WAYS;

// size of the caches in bytes (ways * 2^(linewidth + setwidth))
const uint32_t Xthal_icache_size = XCHAL_ICACHE_SIZE;
const uint32_t Xthal_dcache_size = XCHAL_DCACHE_SIZE;

// cache features
const uint8_t Xthal_dcache_is_writeback  = XCHAL_DCACHE_IS_WRITEBACK;
const uint8_t Xthal_icache_line_lockable = XCHAL_ICACHE_LINE_LOCKABLE;
const uint8_t Xthal_dcache_line_lockable = XCHAL_DCACHE_LINE_LOCKABLE;
#elif defined(__SPLIT__L2_vars)

# if XCHAL_HAVE_L2

#define DECLARE(var)	extern char _ ## var; \
			uint32_t var = (uint32_t) & _ ## var;

DECLARE(Xthal_L2cache_size)	// used by cache ops (in case L2 cache gets dynamically resized)
DECLARE(Xthal_L2ram_size)		// size of L2 ram

/*  L2 setup defaults:  */
XTHAL_WEAK_SET(_Xthal_L2cache_size, XTHAL_L2CACHE_DEFAULT_SIZE);
XTHAL_WEAK_SET(_Xthal_L2ram_size,   XTHAL_L2RAM_DEFAULT_SIZE);

# endif /* L2 */

#elif defined(__SPLIT__L2_reconfigure)
#if XCHAL_HAVE_L2_CACHE
//  Set size of L2 cache and L2 RAM.
//  NOTE:  changes these dynamically.
static uint32_t cache_bits(uint32_t cache_fraction, uint32_t *cache_fraction_sixteenths)
{
	switch (cache_fraction)
	{
	case 0:
	case 1:
		*cache_fraction_sixteenths = 0;
		return 0;
	case 2:
	case 3:
		*cache_fraction_sixteenths = 2;
		return 1;
	case 4:
	case 5:
	case 6:
	case 7:
		*cache_fraction_sixteenths = 4;
		return 2;
	case 16:
		*cache_fraction_sixteenths = 16;
		return 4;
	default:
		*cache_fraction_sixteenths = 8;
		return 3;
	}
}

static uint32_t ram_bits(uint32_t ram_fraction, uint32_t* data_size, uint32_t* reduction)
{
	switch(ram_fraction)
	{
	case 0:
		*data_size = 0;
		*reduction = 0;
		return 0;
	case 1:
		*data_size = 0;
		*reduction = 0;
		return 0;
	case 2:
		*data_size = XCHAL_L2_SIZE_LOG2 - 3;
		*reduction = 0;
		return 2;
	case 3:
		*data_size = XCHAL_L2_SIZE_LOG2 - 2;
		*reduction = 4;
		return 3;
	case 4:
		*data_size = XCHAL_L2_SIZE_LOG2 - 2;
		*reduction = 0;
		return 4;
	case 5:
		*data_size = XCHAL_L2_SIZE_LOG2 - 1;
		*reduction = 2;
		return 5;
	case 6:
		*data_size = XCHAL_L2_SIZE_LOG2 - 1;
		*reduction = 4;
		return 6;
	case 7:
		*data_size = XCHAL_L2_SIZE_LOG2 - 1;
		*reduction = 6;
		return 7;
	case 8:
		*data_size = XCHAL_L2_SIZE_LOG2 - 1;
		*reduction = 0;
		return 8;
	case 9:
		*data_size = XCHAL_L2_SIZE_LOG2;
		*reduction = 1;
		return 9;
	case 10:
		*data_size = XCHAL_L2_SIZE_LOG2;
		*reduction = 2;
		return 10;
	case 11:
		*data_size = XCHAL_L2_SIZE_LOG2;
		*reduction = 3;
		return 11;
	case 12:
		*data_size = XCHAL_L2_SIZE_LOG2;
		*reduction = 4;
		return 12;
	case 13:
		*data_size = XCHAL_L2_SIZE_LOG2;
		*reduction = 5;
		return 13;
	case 14:
		*data_size = XCHAL_L2_SIZE_LOG2;
		*reduction = 6;
		return 14;
	case 15:
		*data_size = XCHAL_L2_SIZE_LOG2;
		*reduction = 7;
		return 15;
	case 16:
		*data_size = XCHAL_L2_SIZE_LOG2;
		*reduction = 0;
		return 16;
	}
	/* Not Reached */
	return 0;
}
#endif
/*
 * Reconfigures an already initialized (by the reset vector) L2 cache
 *
 * ram_fraction: Fraction of L2 memory used as L2RAM, in sixteenths.
 * If the L2RAM does not support the requested fraction, the closest
 * smaller supported value is used.
 *
 * cache_fraction: Fraction of L2 memory used as L2 cache, in sixteenths.
 * If the L2 cache does not support the requested fraction, the closest
 * smaller supported value is used.
 *
 * There should be no address with an L2 writeback cacheable memory type
 * when this function is called.  Any dirty data in the L2 must be flushed
 * before calling it (or it will be lost).  This function invalidates
 * the L2 cache before returning.
 */

int32_t xthal_L2_repartition(uint32_t ram_fraction, uint32_t cache_fraction)
{
#if XCHAL_HAVE_L2_CACHE
	if (ram_fraction + cache_fraction > 16)
		return -1;
	{
		uint32_t cache_fraction_sixteenths;
		uint32_t cache_fraction_bits;
		uint32_t data_size;
		uint32_t reduction;
		uint32_t ram_ctrl;
		volatile uint32_t* l2cc_reg_ram_ctrl = (uint32_t*) (L2CC_REG_RAM_CTRL + XCHAL_L2_REGS_PADDR);
		volatile uint32_t* l2cc_reg_op_size = (uint32_t*) (L2CC_REG_OP_SIZE + XCHAL_L2_REGS_PADDR);
		volatile uint32_t* l2cc_reg_op_ctrl = (uint32_t*) (L2CC_REG_OP_DOWNGRADE + XCHAL_L2_REGS_PADDR);
		cache_fraction_bits = cache_bits(cache_fraction, &cache_fraction_sixteenths);
		Xthal_L2cache_size = (XCHAL_L2_SIZE / 16) * cache_fraction_sixteenths;
		Xthal_L2ram_size = (XCHAL_L2_SIZE / 16) * ram_bits(ram_fraction, &data_size, &reduction);
		ram_ctrl = *l2cc_reg_ram_ctrl;
		ram_ctrl = (ram_ctrl & 0xffff0000) | (cache_fraction_bits << 8) | (reduction << 5) | (data_size);
		xthal_async_dcache_wait(); // wait for any uncompleted L2 operations
		*l2cc_reg_ram_ctrl = ram_ctrl;
		xthal_async_dcache_wait();
		// Now invalidate cache
		if (Xthal_L2cache_size)
		{
			*l2cc_reg_op_size = Xthal_L2cache_size-XCHAL_L2CACHE_LINESIZE;
			*l2cc_reg_op_ctrl = L2CC_OP_INIT; // tag initialization
			xthal_async_dcache_wait();
		}
	return 0;
	}
#else
	UNUSED(ram_fraction);
	UNUSED(cache_fraction);
	return 0;
#endif
}

#elif defined(__SPLIT__L2ram_base)
int32_t xthal_L2ram_base(uint32_t* base)
{
#if XCHAL_HAVE_L2
    if (!base)
        return XTHAL_INVALID;
    else
        *base = L2CC_RAM_BASE(*((uint32_t*) (XCHAL_L2_REGS_PADDR + L2CC_REG_RAM_CTRL)));
    return XTHAL_SUCCESS;
#else
    UNUSED(base);
    return XTHAL_UNSUPPORTED;
#endif
}

#elif  defined(__SPLIT__dcache_all_writeback) && XCHAL_HAVE_L2_CACHE && XCHAL_DCACHE_IS_COHERENT
void xthal_dcache_all_writeback()
{
    xthal_async_dcache_all_writeback();
    xthal_async_dcache_wait();
}
#elif  defined(__SPLIT__dcache_all_writeback_inv) && XCHAL_HAVE_L2_CACHE && XCHAL_DCACHE_IS_COHERENT
void xthal_dcache_all_writeback_inv()
{
    xthal_async_dcache_all_writeback_inv();
    xthal_async_dcache_wait();
}
#elif  defined(__SPLIT__dcache_all_invalidate) && XCHAL_HAVE_L2_CACHE && XCHAL_DCACHE_IS_COHERENT

void xthal_dcache_all_invalidate()
{
    xthal_async_dcache_all_invalidate();
    xthal_async_dcache_wait();
}

#elif defined(__SPLIT__mpu_wmapl2) && XCHAL_HAVE_L2_CACHE

/* Assembly language helper function for internal use only */
extern void xthal_write_map_helper_l2(const struct xthal_MPU_entry* entries, uint32_t n);

void xthal_write_map(const xthal_MPU_entry* entries, uint32_t n)
{
#if XCHAL_HAVE_MPU
#if XCHAL_HAVE_CACHEADRDIS
    uint32_t cacheadrdis = xthal_calc_cacheadrdis(entries, n);
#endif
    xthal_write_map_helper_l2(entries, n);
#if XCHAL_HAVE_CACHEADRDIS
    XT_WSR_CACHEADRDIS(cacheadrdis);
#endif
#else
    UNUSED(entries);
    UNUSED(n);
#endif
}

#endif /*split*/
