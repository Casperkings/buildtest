// Copyright (c) 2019 Cadence Design Systems
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

#include <xtensa/hal.h>
#include <xtensa/L2-cc-regs.h>
#include <xtensa/xtensa-types.h>

/*
 * This function safely makes the address space non-cacheable.
 * 1) The D-cache (L1 and L2) is written back
 * 2) All regions are made non-cacheable
 * 3) The L1 I-cache and D-cache are invalidated
 *
 * This function assumes that the foreground map of the MPU is active.
 * (No address is mapped by an MPU background with a cacheable memory type).
 */
#if XCHAL_DCACHE_IS_COHERENT && XCHAL_HAVE_MPU && XCHAL_HAVE_L2
static inline void set_all_noncacheable()
{
#if XCHAL_HAVE_CACHEADRDIS
#error Coherence and CacheAdrdis is an unsupported conbination
#endif
    xthal_MPU_entry fg[XCHAL_MPU_ENTRIES];
    xthal_read_map(fg);
    int32_t i;

    xthal_dcache_all_writeback();
    for (i = 0; i < XCHAL_MPU_ENTRIES; i++)
    {
        XTHAL_MPU_ENTRY_SET_MEMORY_TYPE(fg[i], XCHAL_CA_BYPASS);
    }
    xthal_write_map_raw(fg, XCHAL_MPU_ENTRIES);
    xthal_dcache_L1_all_writeback_inv();
    xthal_icache_all_invalidate();
}
#endif


/*
 * Opt out (for current core) of the cache coherency protocol. A typical use
 * model is to call this function before shutting a core down.
 *
 * This function:
 * 1) turns off prefetch
 * 2) sets all memory to non-cacheable and invalidates the caches.
 * 3) Turns of coherence (memctl special register and L2CC (if applicable).
 *
 * NOTE: When using this function in a system with an L2 cache controller,
 * only one core may call this function at a time. (In other words, this function
 * should be protected by a mutex or the equivalent).
 *
 * This function assumes that if an MPU is configured then the foreground
 * map of the MPU is active.
 */
void xthal_cache_coherence_optout(void)
{
#if XCHAL_DCACHE_IS_COHERENT
#if XCHAL_HAVE_MPU && XCHAL_HAVE_L2
    xthal_set_cache_prefetch(XTHAL_DCACHE_PREFETCH_OFF);

    set_all_noncacheable();
    /*  Wait for everything to settle.  */
    __asm__ __volatile__("memw\n\t");
    xthal_dcache_sync();
    xthal_icache_sync();
    /*  Opt-out of cache coherency protocol.  */
    xthal_cache_coherence_off();

#elif XCHAL_HAVE_EXTERN_REGS
    /*
     *  Opt-out of cache coherence ... non MPU
     *
     *  Caveat:  on a core with full MMU, cache attribute handling done here only
     *  works well with the default (reset) TLB mapping of eight 512MB regions.
     *  It likely won't work correctly when other page sizes are in use (it may
     *  appear to work but be open to race conditions, depending on situation).
     */
    uint32_t ca = xthal_get_cacheattr();
    /*  Writeback all dirty entries.  Writethru mode avoids new dirty entries.  */
    xthal_set_region_attribute(0,0xFFFFFFFF, XCHAL_CA_WRITETHRU, XTHAL_CAFLAG_EXPAND);
    xthal_dcache_all_writeback();
    /*  Invalidate all cache entries.  Cache-bypass mode avoids new entries.  */
    xthal_set_region_attribute(0,0xFFFFFFFF, XCHAL_CA_BYPASS, XTHAL_CAFLAG_EXPAND);
    xthal_dcache_all_writeback_inv();
    /*  Wait for everything to settle.  */
    __asm__ __volatile__("memw\n\t");
    xthal_dcache_sync();
    xthal_icache_sync();
    /*  Opt-out of cache coherency protocol.  */
    xthal_cache_coherence_off();
    /*  Restore cache attributes, as of entry to this function.  */
    xthal_set_cacheattr(ca);
#endif
#endif
}

/*
 * In a subsystem where the cores are numbered 0 ... N-1
 * xthal_run_cores() is used to start some subset of cores 1 ... N-1
 * (Core 0 is started automatically at reset).
 *
 * The cores parameter is a bit mask of the cores that should be
 * started.  For example to start cores 1 and 3 (but no others)
 * then cores should equal 0xa.  The constant XTSUB_RUN_ALL_CORES can be used
 * to start all the cores.
 *
 * On success this function returns XTHAL_SUCCESS
 * (Note that xtos_barrier or other synchronization objects are typically
 * used to make sure that all cores have started and reached 'main'.
 * xtos_barrier assumes that all cores are running so if only a subset of cores
 * are run, a different synchronization construct is needed.)
 *
 * If the parameter, cores, is invalid then XTHAL_INVALID is returned.
 *
 * If the core is not part of a subsystem, then XTHAL_UNSUPPORTED is returned.
 *
 */
int32_t xthal_run_cores(uint32_t cores)
{
#if XCHAL_HAVE_PROGRAMMABLE_APB && (XCHAL_NUM_CORES_IN_CLUSTER > 1)
    uint32_t apb_addr;
    volatile uint32_t* RunOnReset;
    int32_t rv;
    if ((cores > XTSUB_RUN_ALL_CORES)
            || ((cores & UINT32_C(0x1)) == UINT32_C(1)))
        return XTHAL_INVALID;
    if (((rv = xthal_get_apb_address(&apb_addr)) != XTHAL_SUCCESS))
        return rv;
    RunOnReset = (volatile uint32_t*) (apb_addr + XTSUB_BASE
            + XTSUB_RUN_ON_RESET);
    *RunOnReset = cores;
    return XTHAL_SUCCESS;
#else
    UNUSED(cores);
    return XTHAL_UNSUPPORTED;
#endif
}
