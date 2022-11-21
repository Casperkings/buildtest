/*
 * Copyright (c) 2004-2018 Cadence Design Systems Inc.
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
#include <xtensa/core-macros.h>
#include <stdint.h>

#define UNUSED(x) ((void)(x))

#if XCHAL_HAVE_MPU
#include <xtensa/hal.h>
#include <string.h>
#include <stdlib.h>
#include <xtensa/tie/xt_mmu.h>
/*
 * General notes:
 * Wherever an address is represented as an uint32_t, it has only the 27 most significant bits.  This is how
 * the addresses are represented in the MPU.  It has the benefit that we don't need to worry about overflow.
 *
 * The asserts in the code are ignored unless an assert handler is set (as it is during testing).
 *
 * If an assert handler is set, then the MPU map is checked for correctness after every update.
 *
 * On some configs (actually all configs right now),  the MPU entries must be aligned to the background map.
 * The constant: XCHAL_MPU_ALIGN_REQ indicates if alignment is required:
 *
 * The rules for a valid map are:
 *
 * 1) The entries' vStartAddress fields must always be in non-descending order.
 * 2) The entries' memoryType and accessRights must contain valid values
 *
 * If XCHAL_MPU_ALIGN_REQ == 1 then the following additional rules are enforced:
 * 3) If entry0's Virtual Address Start field is nonzero, then that field must equal one of the
 *    Background Map's Virtual Address Start field values if software ever intends to assert entry0's MPUENB bit.
 * 4) If entryN's MPUENB bit will ever be negated while at the same time entryN+1's MPUENB bit is asserted,
 *    then entryN+1's Virtual Address Start field must equal one of the Background Map's Virtual Address Start field values.
 *
 * The internal function are first, and the external 'xthal_' functions are at the end.
 *
 */

/* implemented in assembly language */
extern void xthal_read_map_raw(const xthal_MPU_entry* fg);

#define MPU_VSTART_CORRECTNESS_MASK  ((0x1 << (XCHAL_MPU_ALIGN_BITS)) - 1)

#if defined(__SPLIT__mpu_basic)
inline static int32_t is_cacheable(uint32_t mt);

#if 0
static inline void read_map_entry(uint32_t en_num, xthal_MPU_entry* en)
{
    uint32_t as;
    uint32_t at0;
    uint32_t at1;
    as = en_num;
    __asm__ __volatile__("RPTLB0 %0, %1\n\t" : "+a" (at0) : "a" (as));
    __asm__ __volatile__("RPTLB1 %0, %1\n\t" : "+a" (at1) : "a" (as));
    en->as = at0;
    en->at = at1;
}
#endif

inline static int32_t is_cacheable(uint32_t mt)
{
    int32_t rv = 0;
    if (((0x180U & mt) != 0U) || ((mt & 0x18U) == 0x10U) || ((mt & 0x30U) == 0x30U))
    {
       rv = 1;
    }
    return rv;
}

inline static int32_t is_writeback(uint32_t mt)
{
    int32_t rv;
    if (((0x181U & mt) == 0x181U) || (((0x190U & mt) == 0x190U))) /* RWC and rwc */
    {
       rv = 1;
    }
    else if (((0x189U & mt) == 0x81U) || ((0x198U & mt) == 0x90U)) /* RWC and rwc */
    {
       rv = 1;
    }
    else if  (((mt & 0x1F0U) == 0x30U) || ((mt & 0x1F8U) == 0x10U)) /* RWC */
    {
       rv =  ((mt & 0x1U) == 0x1U) ? 1 : 0;
    }
    else if (((mt & 0x18FU) == 0x89U)) /* rwc */
    {
       rv = (((mt & 0x10U) == 0x10U)) ? 1 : 0;
    }
    else
    {
       rv = 0;
    }
    return rv;

}

inline static int32_t is_device(uint32_t mt)
{
    int32_t rv = 0;
    if ((mt & 0x1f0U) == 0U)
    {
       rv = 1;
    }
    return rv;
}

inline static int32_t is_kernel_readable(int32_t accessRights)
{
    int32_t rv = XTHAL_BAD_ACCESS_RIGHTS;
    switch (accessRights)
    {
    case XTHAL_AR_R:
    case XTHAL_AR_Rr:
    case XTHAL_AR_RX:
    case XTHAL_AR_RXrx:
    case XTHAL_AR_RW:
    case XTHAL_AR_RWX:
    case XTHAL_AR_RWr:
    case XTHAL_AR_RWrw:
    case XTHAL_AR_RWrwx:
    case XTHAL_AR_RWXrx:
    case XTHAL_AR_RWXrwx:
       rv = 1;
       break;
#if XCHAL_HAVE_MPU_EXECUTE_ONLY
    case XTHAL_AR_X:
    case XTHAL_AR_x:
#endif
    case XTHAL_AR_NONE:
    case XTHAL_AR_Ww:
       rv = 0;
       break;
    default:
       rv = XTHAL_BAD_ACCESS_RIGHTS;
	break;
    }
    return rv;
}

inline static int32_t is_kernel_writeable(int32_t accessRights)
{
    int32_t rv =  XTHAL_BAD_ACCESS_RIGHTS;
    switch (accessRights)
    {
    case XTHAL_AR_RW:
    case XTHAL_AR_RWX:
    case XTHAL_AR_RWr:
    case XTHAL_AR_RWrw:
    case XTHAL_AR_RWrwx:
    case XTHAL_AR_RWXrx:
    case XTHAL_AR_RWXrwx:
    case XTHAL_AR_Ww:
        rv =  1;
        break;
#if XCHAL_HAVE_MPU_EXECUTE_ONLY
    case XTHAL_AR_X:
    case XTHAL_AR_x:
#endif
    case XTHAL_AR_NONE:
    case XTHAL_AR_R:
    case XTHAL_AR_Rr:
    case XTHAL_AR_RX:
    case XTHAL_AR_RXrx:
        rv = 0;
        break;
    default:
       rv = XTHAL_BAD_ACCESS_RIGHTS;
	break;
    }
    return rv;
}

inline static int32_t is_kernel_executable(int32_t accessRights)
{
    int32_t rv =  XTHAL_BAD_ACCESS_RIGHTS;
    switch (accessRights)
    {
    case XTHAL_AR_RX:
    case XTHAL_AR_RXrx:
    case XTHAL_AR_RWX:
    case XTHAL_AR_RWXrx:
    case XTHAL_AR_RWXrwx:
#if XCHAL_HAVE_MPU_EXECUTE_ONLY
    case XTHAL_AR_X:
#endif
        rv = 1;
        break;
#if XCHAL_HAVE_MPU_EXECUTE_ONLY
    case XTHAL_AR_x:
#endif
    case XTHAL_AR_NONE:
    case XTHAL_AR_Ww:
    case XTHAL_AR_R:
    case XTHAL_AR_Rr:
    case XTHAL_AR_RW:
    case XTHAL_AR_RWr:
    case XTHAL_AR_RWrw:
    case XTHAL_AR_RWrwx:
        rv = 0;
        break;
    default:
       rv = XTHAL_BAD_ACCESS_RIGHTS;
        break;
    }
    return rv;
}

inline static int32_t is_user_readable(int32_t accessRights)
{
    int32_t rv =  XTHAL_BAD_ACCESS_RIGHTS;
    switch (accessRights)
    {
    case XTHAL_AR_Rr:
    case XTHAL_AR_RXrx:
    case XTHAL_AR_RWr:
    case XTHAL_AR_RWrw:
    case XTHAL_AR_RWrwx:
    case XTHAL_AR_RWXrx:
    case XTHAL_AR_RWXrwx:
        rv = 1;
        break;
#if XCHAL_HAVE_MPU_EXECUTE_ONLY
    case XTHAL_AR_X:
    case XTHAL_AR_x:
#endif
    case XTHAL_AR_R:
    case XTHAL_AR_RX:
    case XTHAL_AR_RW:
    case XTHAL_AR_RWX:
    case XTHAL_AR_NONE:
    case XTHAL_AR_Ww:
        rv = 0;
        break;
    default:
       rv = XTHAL_BAD_ACCESS_RIGHTS;
        break;
    }
    return rv;
}

inline static int32_t is_user_writeable(int32_t accessRights)
{
    int32_t rv =  XTHAL_BAD_ACCESS_RIGHTS;
    switch (accessRights)
    {
    case XTHAL_AR_Ww:
    case XTHAL_AR_RWrw:
    case XTHAL_AR_RWrwx:
    case XTHAL_AR_RWXrwx:
        rv = 1;
        break;
#if XCHAL_HAVE_MPU_EXECUTE_ONLY
    case XTHAL_AR_X:
    case XTHAL_AR_x:
#endif
    case XTHAL_AR_NONE:
    case XTHAL_AR_R:
    case XTHAL_AR_Rr:
    case XTHAL_AR_RX:
    case XTHAL_AR_RXrx:
    case XTHAL_AR_RW:
    case XTHAL_AR_RWX:
    case XTHAL_AR_RWr:
    case XTHAL_AR_RWXrx:
        rv =  0;
        break;
    default:
       rv = XTHAL_BAD_ACCESS_RIGHTS;
        break;
    }
    return rv;
}

inline static int32_t is_user_executable(int32_t accessRights)
{
    int32_t rv =  XTHAL_BAD_ACCESS_RIGHTS;
    switch (accessRights)
    {
    case XTHAL_AR_RXrx:
    case XTHAL_AR_RWrwx:
    case XTHAL_AR_RWXrx:
    case XTHAL_AR_RWXrwx:
#if XCHAL_HAVE_MPU_EXECUTE_ONLY
    case XTHAL_AR_x:
#endif
        rv = 1;
        break;
#if XCHAL_HAVE_MPU_EXECUTE_ONLY
    case XTHAL_AR_X:
#endif
    case XTHAL_AR_RW:
    case XTHAL_AR_RWX:
    case XTHAL_AR_RWr:
    case XTHAL_AR_RWrw:
    case XTHAL_AR_R:
    case XTHAL_AR_Rr:
    case XTHAL_AR_RX:
    case XTHAL_AR_NONE:
    case XTHAL_AR_Ww:
        rv =  0;
        break;
    default:
       rv = XTHAL_BAD_ACCESS_RIGHTS;
        break;
    }
    return rv;
}

#endif

#if defined(__SPLIT__mpu_check)
#if ! XCHAL_MPU_LOCK
/* returns true if the supplied address (27msb) is in the background map. */
static int32_t in_bgmap(uint32_t address)
{
    int32_t i;
    for (i = 0; i < XCHAL_MPU_BACKGROUND_ENTRIES; i++)
        if (XTHAL_MPU_ENTRY_GET_VSTARTADDR(Xthal_mpu_bgmap[i]) == address)
            return 1;
    return 0;
}
#endif

static int32_t bad_accessRights(uint32_t ar)
{
    if (ar == 0 || (ar >= 4 && ar <= 15))
        return 0;
    else
        return 1;
}

/* this function checks if the supplied map 'fg' is a valid MPU map using 3 criteria:
 * 		1) if an entry is valid, then that entries accessRights must be defined (0 or 4-15).
 * 		2) The map entries' 'vStartAddress's must be in increasing order.
 * 		3) If the architecture requires background map alignment then:
 * 			a) If entry0's 'vStartAddress' field is nonzero, then that field must equal
 * 			one of the Background Map's 'vStartAddress' field values if the entry 0's valid bit is set.
 * 			b) If entryN's 'valid' bit is 0 and entry[N+1]'s 'valid' bit is 1, then
 * 			 entry[N+1]'s 'vStartAddress' field must equal one of the Background Map's 'vStartAddress' field values.
 *
 *		This function returns XTHAL_SUCCESS if the map satisfies the condition, otherwise it returns
 *		XTHAL_BAD_ACCESS_RIGHTS, XTHAL_OUT_OF_ORDER_MAP, or XTHAL_MAP_NOT_ALIGNED.
 *
 *		NOTE: This function uses explicit and unflixed L32I loads to ensure proper
 *		operation when the MPU table is in iram.
 */
static int32_t check_map(const xthal_MPU_entry* fg, uint32_t n)
{
    uint32_t i;
    uint32_t current = 0;
    xthal_MPU_entry e;

    if (!n)
        return XTHAL_SUCCESS;
    if (n > XCHAL_MPU_ENTRIES)
        return XTHAL_OUT_OF_ENTRIES;
    for (i = 0; i < n; i++)
    {
        XT_MEMW();
        e.as = XT_L32I((const int32_t *)&(fg[i].as), 0);
        XT_MEMW();
        e.at = XT_L32I((const int32_t *)&(fg[i].at), 0);

        if (XTHAL_MPU_ENTRY_GET_VALID(e) && bad_accessRights(XTHAL_MPU_ENTRY_GET_ACCESS(e)))
            return XTHAL_BAD_ACCESS_RIGHTS;
#if ! XCHAL_MPU_LOCK
        if ((XTHAL_MPU_ENTRY_GET_VSTARTADDR(e) < current))
            return XTHAL_OUT_OF_ORDER_MAP;
#endif
        if (XTHAL_MPU_ENTRY_GET_VSTARTADDR(e) & MPU_VSTART_CORRECTNESS_MASK)
             return XTHAL_MAP_NOT_ALIGNED;
        current = XTHAL_MPU_ENTRY_GET_VSTARTADDR(e);
    }
#if ! XCHAL_MPU_LOCK
    XT_MEMW();
    e.as = XT_L32I((const int32_t *)&(fg[0].as), 0);
    XT_MEMW();
    e.at = XT_L32I((const int32_t *)&(fg[0].at), 0);

    if (XCHAL_MPU_ALIGN_REQ && XTHAL_MPU_ENTRY_GET_VALID(e) && XTHAL_MPU_ENTRY_GET_VSTARTADDR(e)
            && !in_bgmap(XTHAL_MPU_ENTRY_GET_VSTARTADDR(e)))
        return XTHAL_MAP_NOT_ALIGNED;
    for (i = 0; i < n- 1; i++)
    {
        xthal_MPU_entry e2;

        XT_MEMW();
        e.as = XT_L32I((const int32_t *)&(fg[i].as), 0);
        XT_MEMW();
        e.at = XT_L32I((const int32_t *)&(fg[i].at), 0);
        XT_MEMW();
        e2.as = XT_L32I((const int32_t *)&(fg[i + 1].as), 0);
        XT_MEMW();
        e2.at = XT_L32I((const int32_t *)&(fg[i + 1].at), 0);

        if (XCHAL_MPU_ALIGN_REQ && !XTHAL_MPU_ENTRY_GET_VALID(e) && XTHAL_MPU_ENTRY_GET_VALID(e2)
                && !in_bgmap(XTHAL_MPU_ENTRY_GET_VSTARTADDR(e2)))
            return XTHAL_MAP_NOT_ALIGNED;
    }
#endif
    return XTHAL_SUCCESS;
}



/*
 * this function checks that the bit-wise or-ed XTHAL_MEM_... bits in x correspond to a valid
 * MPU memoryType. If x is valid, then 0 is returned, otherwise XTHAL_BAD_MEMORY_TYPE is
 * returned.
 */
static int32_t check_memory_type(uint32_t x)
{
    uint32_t system_cache_type = I_XTHAL_MEM_CACHE_MASK(x);
    uint32_t processor_cache_type = (((x) & I_XTHAL_LOCAL_CACHE_BITS) >> 4);
    if ((system_cache_type > XTHAL_MEM_NON_CACHEABLE) || (processor_cache_type > XTHAL_MEM_NON_CACHEABLE))
        return XTHAL_BAD_MEMORY_TYPE;
    int32_t processor_cache_type_set = 1;
    if (!processor_cache_type)
    {
        processor_cache_type = system_cache_type << 4;
        processor_cache_type_set = 0;
    }
    uint32_t device = I_XTHAL_MEM_IS_DEVICE(x);
    uint32_t system_noncacheable = I_XTHAL_IS_SYSTEM_NONCACHEABLE(x);
#if XCHAL_HAVE_NX // system non cachable, but cacheable locally only supported on NX
    uint32_t local_noncacheable = (x & I_XTHAL_LOCAL_CACHE_BITS) ? (((x & I_XTHAL_LOCAL_CACHE_BITS) >> 4) & (XTHAL_MEM_NON_CACHEABLE == XTHAL_MEM_NON_CACHEABLE)) : system_noncacheable;
#else
    uint32_t local_noncacheable = system_noncacheable;
#endif
    if (device || (system_noncacheable && local_noncacheable))
    {
        if ((system_cache_type || processor_cache_type_set) && device)
            return XTHAL_BAD_MEMORY_TYPE;
        if (processor_cache_type_set)
            return XTHAL_BAD_MEMORY_TYPE; // if memory is device or non cacheable, then processor cache type should not be set
        if (system_noncacheable && (x & XTHAL_MEM_INTERRUPTIBLE))
            return XTHAL_BAD_MEMORY_TYPE;
        {
            uint32_t z = x & XTHAL_MEM_SYSTEM_SHAREABLE;
            if ((z == XTHAL_MEM_INNER_SHAREABLE) || (z == XTHAL_MEM_OUTER_SHAREABLE))
                return XTHAL_BAD_MEMORY_TYPE;
        }
    }
    else
    {
        if ((x & XTHAL_MEM_SYSTEM_SHAREABLE) == XTHAL_MEM_SYSTEM_SHAREABLE)
            return XTHAL_BAD_MEMORY_TYPE;
        if ((x & (XTHAL_MEM_BUFFERABLE | XTHAL_MEM_INTERRUPTIBLE)))
            return XTHAL_BAD_MEMORY_TYPE;
    }

    return 0;
}
#endif

#endif // is MPU

#if defined(__SPLIT__mpu_basic)
/*
 * These functions accept encoded access rights, and return 1 if the supplied memory type has the property specified by the function name.
 */
extern int32_t xthal_is_kernel_readable(uint32_t accessRights)
{
#if XCHAL_HAVE_MPU
    return is_kernel_readable(accessRights);
#else
    UNUSED(accessRights);
    return XTHAL_UNSUPPORTED;
#endif
}

extern int32_t xthal_is_kernel_writeable(uint32_t accessRights)
{
#if XCHAL_HAVE_MPU
    return is_kernel_writeable(accessRights);
#else
    UNUSED(accessRights);
    return XTHAL_UNSUPPORTED;
#endif
}

extern int32_t xthal_is_kernel_executable(uint32_t accessRights)
{
#if XCHAL_HAVE_MPU
    return is_kernel_executable(accessRights);
#else
    UNUSED(accessRights);
    return XTHAL_UNSUPPORTED;
#endif
}

extern int32_t xthal_is_user_readable(uint32_t accessRights)
{
#if XCHAL_HAVE_MPU
    return is_user_readable(accessRights);
#else
    UNUSED(accessRights);
    return XTHAL_UNSUPPORTED;
#endif
}

extern int32_t xthal_is_user_writeable(uint32_t accessRights)
{
#if XCHAL_HAVE_MPU
    return is_user_writeable(accessRights);
#else
    UNUSED(accessRights);
    return XTHAL_UNSUPPORTED;
#endif
}

extern int32_t xthal_is_user_executable(uint32_t accessRights)
{
#if XCHAL_HAVE_MPU
    return is_user_executable(accessRights);
#else
    UNUSED(accessRights);
    return XTHAL_UNSUPPORTED;
#endif
}

/*
 * These functions accept either an encoded or unencoded memory type, and
 * return 1 if the supplied memory type has property specified by the
 * function name.
 */
int32_t xthal_is_cacheable(uint32_t mt)
{
#if XCHAL_HAVE_MPU
    return is_cacheable(mt);
#else
    UNUSED(mt);
    return XTHAL_UNSUPPORTED;
#endif
}

int32_t xthal_is_writeback(uint32_t mt)
{
#if XCHAL_HAVE_MPU
    return is_writeback(mt);
#else
    UNUSED(mt);
    return XTHAL_UNSUPPORTED;
#endif
}

int32_t xthal_is_device(uint32_t mt)
{
#if XCHAL_HAVE_MPU
    return is_device(mt);
#else
    UNUSED(mt);
    return XTHAL_UNSUPPORTED;
#endif
}
#endif

/*
 * This function converts a bit-wise combination of the XTHAL_MEM_.. constants
 * to the corresponding MPU memory type (9-bits).
 *
 * If none of the XTHAL_MEM_.. bits are present in the argument, then
 * bits 4-12 (9-bits) are returned ... this supports using an already encoded
 * memoryType (perhaps obtained from an xthal_MPU_entry structure) as input
 * to xthal_set_region_attribute().
 *
 * This function first checks that the supplied constants are a valid and
 * supported combination.  If not, it returns XTHAL_BAD_MEMORY_TYPE.
 */
#if defined(__SPLIT__mpu_check)
int32_t xthal_encode_memory_type(uint32_t x)
{
#if XCHAL_HAVE_MPU
    const uint32_t MemoryTypeMask = 0x1ff0;
    const uint32_t MemoryFlagMask = 0xffffe000;
    /*
     * Encodes the memory type bits supplied in an | format (XCHAL_CA_PROCESSOR_CACHE_WRITEALLOC | XCHAL_CA_PROCESSOR_CACHE_WRITEBACK)
     */
    uint32_t memoryFlags = x & MemoryFlagMask;
    if (!memoryFlags)
        return (x & MemoryTypeMask) >> XTHAL_AR_WIDTH;
    else
    {
        int32_t chk = check_memory_type(memoryFlags);
        if (chk < 0)
            return chk;
        else
            return XTHAL_ENCODE_MEMORY_TYPE(memoryFlags);
    }
#else
    UNUSED(x);
    return XTHAL_UNSUPPORTED;
#endif
}
#endif

#if defined(__SPLIT__mpu_rmap)

/*
 * Copies the current MPU entry list into 'entries' which
 * must point to available memory of at least
 * sizeof(xthal_MPU_entry) * XCHAL_MPU_ENTRIES.
 *
 * This function returns XTHAL_SUCCESS.
 * XTHAL_INVALID, or
 * XTHAL_UNSUPPORTED.
 */
int32_t xthal_read_map(xthal_MPU_entry* fg_map)
{
#if XCHAL_HAVE_MPU
    if (!fg_map)
        return XTHAL_INVALID;
    xthal_read_map_raw(fg_map);
    return XTHAL_SUCCESS;
#else
    UNUSED(fg_map);
    return XTHAL_UNSUPPORTED;
#endif
}

#if XCHAL_HAVE_MPU
#undef XCHAL_MPU_BGMAP // parasoft-suppress MISRA2012-RULE-20_5 "#undef is needed to define the background map."
#define XCHAL_MPU_BGMAP(s, vstart, vend, rights, mtype, x) XTHAL_MPU_ENTRY((vstart), 1, (rights), (mtype)),
const xthal_MPU_entry Xthal_mpu_bgmap[] = { XCHAL_MPU_BACKGROUND_MAP(0) };
#endif
#endif
/*
 * Writes the map pointed to by 'entries' to the MPU. Before updating
 * the map, it commits any uncommitted
 * cache writes, and invalidates the cache if necessary.
 *
 * This function does not check for the correctness of the map.  Generally
 * xthal_check_map() should be called first to check the map.
 *
 * If n == 0 then the existing map is cleared, and no new map is written
 * (useful for returning to reset state)
 *
 * If (n > 0 && n < XCHAL_MPU_ENTRIES) then a new map is written with
 * (XCHAL_MPU_ENTRIES-n) padding entries added to ensure a properly ordered
 * map.  The resulting foreground map will be equivalent to the map vector
 * fg, but the position of the padding entries should not be relied upon.
 *
 * If n == XCHAL_MPU_ENTRIES then the complete map as specified by fg is
 * written.
 *
 * xthal_write_map() disables the MPU foreground map during the MPU
 * update and relies on the background map.
 *
 * As a result any interrupt that does not meet the following conditions
 * must be disabled before calling xthal_write_map():
 *    1) All code and data needed for the interrupt must be
 *       mapped by the background map with sufficient access rights.
 *    2) The interrupt code must not access the MPU.
 *
 */
#if defined(__SPLIT__mpu_wmap) && !XCHAL_HAVE_L2_CACHE

/* Assembly language helper function for internal use only */
extern void xthal_write_map_helper(const struct xthal_MPU_entry* entries, uint32_t n);

void xthal_write_map(const xthal_MPU_entry* entries, uint32_t n)
{
#if XCHAL_HAVE_MPU

    xthal_write_map_helper(entries, n);
#if XCHAL_HAVE_CACHEADRDIS 
    uint32_t cacheadrdis = xthal_calc_cacheadrdis_internal(entries, n);
    XT_WSR_CACHEADRDIS(cacheadrdis);
#endif
#else
    UNUSED(entries);
    UNUSED(n);
#endif
}
#endif

#if defined(__SPLIT__mpu_check)
/*
 * Checks if entry vector 'fg' of length 'n' is a valid MPU access map.
 * Returns:
 *    XTHAL_SUCCESS if valid,
 *    XTHAL_OUT_OF_ENTRIES
 *    XTHAL_MAP_NOT_ALIGNED,
 *    XTHAL_BAD_ACCESS_RIGHTS,
 *    XTHAL_OUT_OF_ORDER_MAP, or
 *    XTHAL_UNSUPPORTED if config doesn't have an MPU.
 */
int32_t xthal_check_map(const xthal_MPU_entry* fg, uint32_t n)
{
#if XCHAL_HAVE_MPU
    return check_map(fg, n);
#else
    UNUSED(fg);
    UNUSED(n);
    return XTHAL_UNSUPPORTED;
#endif
}
#endif

#if defined(__SPLIT__mpu_basic)
/*
 * Returns the MPU entry that maps 'vaddr'. If 'infgmap' is non-NULL then it is
 * set to 1 if 'vaddr' is mapped by the foreground map, or 0 if 'vaddr'
 * is mapped by the background map.
 */
extern xthal_MPU_entry xthal_get_entry_for_address(void* paddr, int32_t* infgmap)
  {
#if XCHAL_HAVE_MPU
    xthal_MPU_entry e;
    uint32_t p;
    __asm__ __volatile__("PPTLB %0, %1\n\t" : "=a" (p) : "a" (paddr));
    if ((p & 0x80000000))
      {
	if (infgmap)
	   *infgmap = 1;
	e.at = (p & 0x1fffff);
	__asm__ __volatile__("RPTLB0 %0, %1\n\t" : "=a" (e.as) : "a" (p & 0x1f));
	return e;
      }
    else
      {
	int32_t i;
	if (infgmap)
	*infgmap = 0;
	for (i = XCHAL_MPU_BACKGROUND_ENTRIES - 1; i > 0; i--)
	  {
	    if (XTHAL_MPU_ENTRY_GET_VSTARTADDR(Xthal_mpu_bgmap[i]) <= (uint32_t) paddr)
	      {
		return Xthal_mpu_bgmap[i];
	      }
	  } // in background map
	return Xthal_mpu_bgmap[0];
      }
#else
    UNUSED(paddr);
    UNUSED(infgmap);
    xthal_MPU_entry e = {0,0};
    return e;
#endif
  }
#endif

#if defined(__SPLIT__mpu_cachedis)

#if XCHAL_HAVE_CACHEADRDIS

static uint32_t mask_cachedis(uint32_t current, uint32_t addr)
{
    uint32_t mask = ~(UINT32_C(1) << (addr >> UINT32_C(29)));
    return current & mask;

}

static uint32_t get_memtype_from_addr(const xthal_MPU_entry* fg, int32_t entries, uint32_t addr)
{
  int32_t i;
  for (i=0; i< entries; i++)
      if (XTHAL_MPU_ENTRY_GET_VALID(fg[i]) && (XTHAL_MPU_ENTRY_GET_VSTARTADDR(fg[i]) <= addr) &&
              ((i == XCHAL_MPU_ENTRIES - 1) || (XTHAL_MPU_ENTRY_GET_VSTARTADDR(fg[i+1]) > addr)))
          return XTHAL_MPU_ENTRY_GET_MEMORY_TYPE(fg[i]);

  for (i = XCHAL_MPU_BACKGROUND_ENTRIES - 1; i >= 0; i--)
    {
      if (XTHAL_MPU_ENTRY_GET_VSTARTADDR (Xthal_mpu_bgmap[i]) <= addr)
        {
          return XTHAL_MPU_ENTRY_GET_MEMORY_TYPE(Xthal_mpu_bgmap[i]);
        }
    }
  return XTHAL_MPU_ENTRY_GET_MEMORY_TYPE(Xthal_mpu_bgmap[0]);
}
#endif
/*
 * xthal_calc_cacheadrdis() computes the value that should be written
 * to the CACHEADRDIS register.  The return value has bits 0-7 set according as:
 * bit n: is zero if any part of the region [512MB * n, 512MB* (n-1)) is cacheable.
 *           is one  if NO part of the region [512MB * n, 512MB* (n-1)) is cacheable.
 *
 * This function looks at both the loops through both the foreground and background maps
 * to find cacheable area.  Once one cacheable area is found in a 512MB region, then we
 * skip to the next 512MB region.
 */
uint32_t xthal_calc_cacheadrdis(const xthal_MPU_entry* fg, uint32_t num_entries)
  {
#if XCHAL_HAVE_CACHEADRDIS
    uint32_t cachedis = 0xff;
    uint32_t addr;
    int32_t i;
    int32_t entries = num_entries;
    for(addr = 0, i = 0; i < 8; i++, addr += 0x20000000)
        if (xthal_is_cacheable(get_memtype_from_addr(fg, entries, addr)))
            cachedis = mask_cachedis(cachedis, addr);
    for(i = 0; i< entries; i++)
    {
        addr = XTHAL_MPU_ENTRY_GET_VSTARTADDR(fg[i]);
        if (xthal_is_cacheable(get_memtype_from_addr(fg, entries, addr)))
                    cachedis = mask_cachedis(cachedis, addr);
    }
    for(i = 1; i< XCHAL_MPU_BACKGROUND_ENTRIES; i++) // 0th bgmap entry has addr==0 so no need to check again.
    {
        addr = XTHAL_MPU_ENTRY_GET_VSTARTADDR(Xthal_mpu_bgmap[i]);
        if (xthal_is_cacheable(get_memtype_from_addr(fg, entries, addr)))
                    cachedis = mask_cachedis(cachedis, addr);
    }
    return cachedis;
#else
    UNUSED(num_entries);
    UNUSED(fg);
    return 0;
#endif
  }
#endif


#if defined(__SPLIT__mpu_cachedis_internal)

#if XCHAL_HAVE_CACHEADRDIS

static uint32_t mask_cachedis_internal(uint32_t current, uint32_t addr)
{
    uint32_t mask = ~(UINT32_C(1) << (addr >> UINT32_C(29)));
    return current & mask;

}

static uint32_t get_memtype_from_addr_internal(uint32_t addr)
{
    uint32_t p;
    __asm__ __volatile__("PPTLB %0, %1\n\t" : "=a" (p) : "a" (addr));
    return ((p & 0x1ff000) >> 12);
}

#endif
/*
 * xthal_calc_cacheadrdis_internal() computes the value that should be written
 * to the CACHEADRDIS register.  The return value has bits 0-7 set according as:
 * bit n: is zero if any part of the region [512MB * n, 512MB* (n-1)) is cacheable.
 *           is one  if NO part of the region [512MB * n, 512MB* (n-1)) is cacheable.
 *
 * This function looks at both the loops through both the foreground and background maps
 * to find cacheable area.  Once one cacheable area is found in a 512MB region, then we
 * skip to the next 512MB region.
 */
uint32_t xthal_calc_cacheadrdis_internal(const xthal_MPU_entry* fg, uint32_t num_entries)
  {
#if XCHAL_HAVE_CACHEADRDIS
    uint32_t cachedis = 0xff;
    uint32_t addr;
    int32_t i;
    int32_t entries = num_entries;
    for(addr = 0, i = 0; i < 8; i++, addr += 0x20000000)
        if (xthal_is_cacheable(get_memtype_from_addr_internal(addr)))
            cachedis = mask_cachedis_internal(cachedis, addr);
    for(i = 0; i< entries; i++)
    {
        addr = XTHAL_MPU_ENTRY_GET_VSTARTADDR(fg[i]);
        if (xthal_is_cacheable(get_memtype_from_addr_internal(addr)))
                    cachedis = mask_cachedis_internal(cachedis, addr);
    }
    for(i = 1; i< XCHAL_MPU_BACKGROUND_ENTRIES; i++) // 0th bgmap entry has addr==0 so no need to check again.
    {
        addr = XTHAL_MPU_ENTRY_GET_VSTARTADDR(Xthal_mpu_bgmap[i]);
        if (xthal_is_cacheable(get_memtype_from_addr_internal(addr)))
                    cachedis = mask_cachedis_internal(cachedis, addr);
    }
    return cachedis;
#else
    UNUSED(num_entries);
    UNUSED(fg);
    return 0;
#endif
  }
#endif

#if defined(__SPLIT__mpu_region_access_rights)
#if XCHAL_HAVE_MPU
static uint8_t convert_ar_to_bits(int32_t accessRights)
{
    uint8_t rv = 0;
    rv |= xthal_is_kernel_readable(accessRights) ? XTHAL_R : 0;
    rv |= xthal_is_kernel_writeable(accessRights) ? XTHAL_W : 0;
    rv |= xthal_is_kernel_executable(accessRights) ? XTHAL_X : 0;
    rv |= xthal_is_user_readable(accessRights) ? XTHAL_r : 0;
    rv |= xthal_is_user_writeable(accessRights) ? XTHAL_w : 0;
    rv |= xthal_is_user_executable(accessRights) ? XTHAL_x : 0;
    return rv;
}
#endif

/* This function takes a memory region, and returns the read/write/execute
   permissions for the region.  If the specified region overlaps multiple
   MPU entries then the access rights that are common to the entire region
   is returned in 'ar' as a bit-wise or of XTHAL_R, XTHAL_W, ... */
int32_t xthal_region_access_rights(void* vaddr, uint32_t size, uint8_t* ar)
{
#if XCHAL_HAVE_MPU
    uint32_t first;
    uint32_t last;
    if (((uint32_t) ar == UINT32_C(0)))
        return XTHAL_INVALID;
    if (size == UINT32_C(0))
    {
        return XTHAL_ZERO_SIZED_REGION;
    }
    first = (uint32_t) vaddr; // parasoft-suppress MISRA2012-RULE-11_6 "Pointer conv OK"
    last = first + size;
    if ((last != UINT32_C(0xFFFFFFFF)) && (size > 1))
    {
        last--;
    }
    if (first >= last)
    {
        return XTHAL_INVALID_ADDRESS_RANGE; // Wraps around
    }
    xthal_MPU_entry x = xthal_get_entry_for_address(vaddr, 0);
    uint32_t rights = convert_ar_to_bits(XTHAL_MPU_ENTRY_GET_ACCESS(x));
    int32_t i;
    xthal_MPU_entry map[XCHAL_MPU_ENTRIES];
    xthal_read_map(map);
    for (i = 0; i < XCHAL_MPU_ENTRIES; i++)
    {
        uint32_t vstart = XTHAL_MPU_ENTRY_GET_VSTARTADDR(map[i]);
        if (XTHAL_MPU_ENTRY_GET_VALID(map[i]) && (vstart < last) && (vstart > first) &&
                ((i == XCHAL_MPU_ENTRIES - 1) || (XTHAL_MPU_ENTRY_GET_VSTARTADDR(map[i+1]) > vstart)))
        rights &= convert_ar_to_bits(XTHAL_MPU_ENTRY_GET_ACCESS(xthal_get_entry_for_address((void*) vstart, 0)));
    }
    for (i = XCHAL_MPU_BACKGROUND_ENTRIES - 1; i >= 0; i--)
    {
        uint32_t vstart = XTHAL_MPU_ENTRY_GET_VSTARTADDR(Xthal_mpu_bgmap[i]);
        if ((vstart > first) && (vstart < last))
            rights &= convert_ar_to_bits(
                    XTHAL_MPU_ENTRY_GET_ACCESS(xthal_get_entry_for_address((void*) vstart, 0)));
    }
    *ar = rights;
    return XTHAL_SUCCESS;
#else
    UNUSED(vaddr);
    UNUSED(size);
    UNUSED(ar);
    return XTHAL_UNSUPPORTED;
#endif
}
#endif
