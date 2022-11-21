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
#include <xtensa/core-macros.h>
#include <xtensa/hal.h>
#include <string.h>
#include <stdlib.h>

#if XCHAL_HAVE_MPU
#include <xtensa/tie/xt_mmu.h>
#define MPU_ADDRESS_MASK    (((~(UINT32_C(0)))  << XCHAL_MPU_ALIGN_BITS))
#define MPU_ALIGNMENT_MASK  (~MPU_ADDRESS_MASK)

static uint32_t entry_get_vstartaddr(xthal_MPU_entry en)
{
    return XTHAL_MPU_ENTRY_GET_VSTARTADDR(en);
}

static uint32_t entry_get_access(xthal_MPU_entry en)
{
    return XTHAL_MPU_ENTRY_GET_ACCESS(en);
}

static uint32_t entry_get_memory_type(xthal_MPU_entry en)
{
    return XTHAL_MPU_ENTRY_GET_MEMORY_TYPE(en);
}

static uint32_t entry_get_valid(xthal_MPU_entry en)
{
    return XTHAL_MPU_ENTRY_GET_VALID(en);
}

static void entry_set_vstartaddr(xthal_MPU_entry* en, uint32_t addr)
{
    XTHAL_MPU_ENTRY_SET_VSTARTADDR(*en, addr);
}

static void entry_set_access(xthal_MPU_entry* en, uint32_t ar)
{
    XTHAL_MPU_ENTRY_SET_ACCESS(*en, ar);
}

static void entry_set_memory_type(xthal_MPU_entry* en, uint32_t mt)
{
    /* parasoft-begin-suppress MISRA2012-RULE-10_4_b "Macro behavior OK, too many warnings from tool" */
    /* parasoft-begin-suppress MISRA2012-RULE-12_1_a "Macro behavior OK, too many warnings from tool" */
    /* parasoft-begin-suppress MISRA2012-RULE-10_7_b "Macro behavior OK, too many warnings from tool" */
    /* parasoft-begin-suppress MISRA2012-RULE-10_7_a "Macro behavior OK, too many warnings from tool" */
    XTHAL_MPU_ENTRY_SET_MEMORY_TYPE(*en, mt);
    /* parasoft-end-suppress MISRA2012-RULE-10_7_a "Macro behavior OK, too many warnings from tool" */
    /* parasoft-end-suppress MISRA2012-RULE-10_4_b "Macro behavior OK, too many warnings from tool" */
    /* parasoft-end-suppress MISRA2012-RULE-12_1_a "Macro behavior OK, too many warnings from tool" */
    /* parasoft-end-suppress MISRA2012-RULE-10_7_b "Macro behavior OK, too many warnings from tool" */
}

static void entry_set_valid(xthal_MPU_entry* en, uint32_t v)
{
    XTHAL_MPU_ENTRY_SET_VALID(*en, v);
}

/* This function updates the map entry as well as internal duplicate of the map
 * state in fg.  The assumption is that reading map entries could be somewhat
 * expensive in some situations so we are keeping a copy of the map in memory when
 * doing extensive map manipulations.
 */

static void write_map_entry(xthal_MPU_entry* fg, int32_t en_num, xthal_MPU_entry en)
{
    en.at = (en.at & UINT32_C(0xffffffe0)) | (((uint32_t) en_num) & UINT32_C(0x1f));
    xthal_mpu_set_entry(en);
    fg[en_num] = en;
}

/* This function takes bitwise or'd combination of access rights and memory type, and extracts
 * the access rights.  It returns the access rights, or XTHAL_BAD_ACCESS_RIGHTS.
 */
static uint32_t encode_access_rights(uint32_t cattr)
{
    cattr = cattr & UINT32_C(0xF);
#if XCHAL_HAVE_MPU_EXECUTE_ONLY
    if (cattr == UINT32_C(1))
#else
    if ((cattr > UINT32_C(0)) && (cattr <= UINT32_C(3)))
#endif
    {
        return (uint32_t) XTHAL_BAD_ACCESS_RIGHTS;
    }

    return cattr;
}

// checks if x (full 32bit) is mpu_aligned for MPU
static uint32_t mpu_aligned(uint32_t x)
{
    return ((x & MPU_ALIGNMENT_MASK) == UINT32_C(0)) ? UINT32_C(1) : UINT32_C(0);
}

static uint32_t mpu_align(uint32_t x, uint32_t roundUp)
{
    if (roundUp != UINT32_C(0))
    {
        return (x + MPU_ALIGNMENT_MASK) & MPU_ADDRESS_MASK;
    }
    else
    {
        return (x & MPU_ADDRESS_MASK);
    }
}

uint32_t xthal_convert_to_writethru_memtype(uint32_t mt)
{
  uint32_t rv;
  /* RWC and rwc */
  if ((((UINT32_C(0x180) & mt) == UINT32_C(0x180)) || \
          ((UINT32_C(0x188) & mt) == UINT32_C(0x80))))
    {
      rv = mt & UINT32_C(0xffffffee);
    }
  else
    /* RWC */
    if  (((mt & UINT32_C(0x1F0)) == UINT32_C(0x30)) || ((mt & UINT32_C(0x1F8)) == UINT32_C(0x10)))
      {
        rv =  mt & UINT32_C(0xfffffffe);
      }
    else
      /* rwc */
      if (((mt & UINT32_C(0x18F)) == UINT32_C(0x89)))
        {
          rv = mt & UINT32_C(0xffffffef);
        }
      else
        {
          rv = mt;
        }
  return rv;
}

extern void xthal_make_noncacheable(int32_t entry, int32_t entry_count, uint32_t new_mt);

static void flush_cache(int32_t i, int32_t n)
{
    uint32_t new_mt = (XTHAL_ENCODE_MEMORY_TYPE(XCHAL_CA_BYPASS) << UINT8_C(12))
            + (XTHAL_AR_RWX << UINT8_C(8));
    xthal_make_noncacheable(i, n, new_mt);
}


#if ! XCHAL_MPU_LOCK

#define XCHAL_MPU_WORST_CASE_ENTRIES_FOR_REGION             UINT32_C(3)

/* returns true if the supplied address (27msb) is in the background map. */
static int32_t in_bgmap(uint32_t address)
{
    int32_t i;
    for (i = 0; i < XCHAL_MPU_BACKGROUND_ENTRIES; i++)
    {
        if (XTHAL_MPU_ENTRY_GET_VSTARTADDR(Xthal_mpu_bgmap[i]) == address)
        {
            return 1;
        }
    }
    return 0;
}

static xthal_MPU_entry get_entry(const xthal_MPU_entry* fg,
        uint32_t addr, int32_t* infgmap)
{
    int32_t i;
    for (i = XCHAL_MPU_ENTRIES - 1; i >= 0; i--)
    {
        if (entry_get_vstartaddr(fg[i]) <= addr)
        {
            if (entry_get_valid(fg[i]) != UINT32_C(0))
            {
                if (infgmap != NULL)
                {
                    *infgmap = 1;
                }
                return fg[i];
            }
            else
            {
                break;
            }
        }
    }
    for (i = XCHAL_MPU_BACKGROUND_ENTRIES - 1; i >= 0; i--)
    {
        if (entry_get_vstartaddr(Xthal_mpu_bgmap[i]) <= addr)
        {
            if (infgmap != NULL)
            {
                *infgmap = 0;
            }
            return Xthal_mpu_bgmap[i];
        }
    }
    return Xthal_mpu_bgmap[0]; // never reached ... just to get rid of compilation warning
}

/*
 * General notes:
 * Wherever an address is represented as an uint32_t, it has only the 27 most significant bits.  This is how
 * the addresses are represented in the MPU.  It has the benefit that we don't need to worry about overflow.
 *
 * The rules for a valid map are:
 *
 * 1) The entries' vStartAddress fields must always be in non-descending order.
 * 2) The entries' memoryType and accessRights must contain valid values
 * 3) If entry0's Virtual Address Start field is nonzero, then that field must equal one of the
 *    Background Map's Virtual Address Start field values if software ever intends to assert entry0's MPUENB bit.
 * 4) If entryN's MPUENB bit will ever be negated while at the same time entryN+1's MPUENB bit is asserted,
 *    then entryN+1's Virtual Address Start field must equal one of the Background Map's Virtual Address Start field values.
 *
 * The internal function are first, and the external 'xthal_' functions are at the end.
 *
 */

static void move_map_down(xthal_MPU_entry* fg, int32_t dup, int32_t idx)
{
    /* moves the map entry list down one (leaving duplicate entries at idx and idx+1.  This function assumes that the last
     * entry is invalid ... call MUST check this
     */
    int32_t i;
    for (i = dup; i > idx; i--)
    {
        write_map_entry(fg, i, fg[i - 1]);
    }
}

static void move_map_up(xthal_MPU_entry* fg, int32_t dup, int32_t idx)
{
    /* moves the map entry list up one (leaving duplicate entries at idx and idx-1, removing the entry at dup
     */
    int32_t i;
    for (i = dup; i < (idx - 1); i++)
    {
        write_map_entry(fg, i, fg[i + 1]);
    }
}

static int32_t bubble_free_to_ip(xthal_MPU_entry* fg, int32_t ip, int32_t req)
{
    /* This function shuffles the entries in the MPU to get at least 'req' free entries at
     * the insertion point 'ip'.  This function returns the new insertion point (after all the shuffling).
     */
    int32_t i;
    int32_t rv = ip;
    int32_t required = req;
    if (required < 1)
    {
        return ip;
    }
    /* first we search for duplicate or unused entries at an index less than 'ip'.  We start looking at ip-1
     * (rather than 0) to minimize the number of shuffles required.
     */
    i = ip - 2;
    while((i >= 0) && (required != 0))
    {
        if (entry_get_vstartaddr(fg[i]) == entry_get_vstartaddr(fg[i + 1]))
        {
            move_map_up(fg, i, ip);
            rv--;
            required--;
        }
        i--;
    }
    // if there are any invalid entries at top of the map, we can remove them to make space
    while (required != 0)
    {
        if (entry_get_valid(fg[0]) == UINT32_C(0))
        {
            move_map_up(fg, 0, ip);
            rv--;
            required--;
        }
        else
        {
            break;
        }
    }
    /* If there are not enough unneeded entries at indexes less than ip, then we search at indexes > ip.
     * We start the search at ip+1 and move down, again to minimize the number of shuffles required.
     */
    i = ip + 1;
    while ((i < XCHAL_MPU_ENTRIES) && (required != 0))
    {
        if (entry_get_vstartaddr(fg[i]) == entry_get_vstartaddr(fg[i - 1]))
        {
            move_map_down(fg, i, ip);
            required--;
        }
        else
        {
            i++;
        }
    }
    return rv;
}


/* This function removes 'inaccessible' entries from the MPU map (those that are hidden by previous entries
 * in the map). It leaves any entries that match background entries in place.
 */
static void remove_inaccessible_entries(xthal_MPU_entry* fg)
{
    int32_t i;
    for (i = 1; i < XCHAL_MPU_ENTRIES; i++)
    {
        if ((((((((entry_get_valid(fg[i]) == entry_get_valid(fg[i - 1]))
                && (entry_get_vstartaddr(fg[i]) > entry_get_vstartaddr(fg[i - 1])))
                && (entry_get_memory_type(fg[i]) == entry_get_memory_type(fg[i - 1])))
                && (entry_get_access(fg[i]) == entry_get_access(fg[i - 1])))
                /* we can only remove the foreground map entry (at at the background
                 * map address) if either background alignment is not required, or
                 * if the previous entry is enabled.
                 */
                && ((in_bgmap(entry_get_vstartaddr(fg[i]))) == 0))))// parasoft-suppress MISRA2012-DIR-4_7_a "entry_get_vstartaddr does not return error."
                || (((entry_get_valid(fg[i]) == UINT32_C(0))
                        && ((entry_get_valid(fg[i - 1]) == UINT32_C(0))
                                && (in_bgmap(entry_get_vstartaddr(fg[i])) == 0))))) // parasoft-suppress MISRA2012-DIR-4_7_a "entry_get_vstartaddr does not return error."
        {
            write_map_entry(fg, i, fg[i - 1]);
        }
    }
}


/*
 * returns the largest value rv, such that for every index < rv,
 * entrys[index].vStartAddress <= first.
 *
 * Assumes an ordered entry array (even disabled entries must be ordered).
 * value returned is in the range [0, XCHAL_MPU_ENTRIES].
 *
 */
static int32_t find_entry(const xthal_MPU_entry* fg, uint32_t first)
{
    int32_t i;
    for (i = XCHAL_MPU_ENTRIES - 1; i >= 0; i--)
    {
        if (entry_get_vstartaddr(fg[i]) <= first)
        {
            return i + 1;
        }
    }
    return 0; // if it is less than all existing entries return 0
}

/*
 * This function returns 1 if there is an exact match for first and last
 * so that no manipulations are necessary before updating the attributes
 * for [first, Last). The the first and end entries
 * must be valid, as well as all the entries in between.  Otherwise the memory
 * type might change across the region and we wouldn't be able to safe the caches.
 *
 * An alternative would be to require alignment regions in this case, but that seems
 * more wasteful.
 */
static int32_t needed_entries_exist(const xthal_MPU_entry* fg, uint32_t first, uint32_t last)
{
    int32_t i;
    for (i = 0; i < XCHAL_MPU_ENTRIES; i++)
    {
        if (entry_get_vstartaddr(fg[i]) == first)
        {
            int32_t j;
            /* special case ... is last at the end of the address space
             * ... if so there is no end entry needed.
             */
            if (last == UINT32_C(0xFFFFFFFF))
            {
                int32_t k;
                for (k = i; k < XCHAL_MPU_ENTRIES; k++)
                {
                    if (entry_get_valid(fg[k]) == UINT32_C(0))
                    {
                        return 0;
                    }
                }
                return 1;
            }
            /* otherwise search for the end entry */
            for (j = i; j < XCHAL_MPU_ENTRIES; j++)
            {
                if (last == entry_get_vstartaddr(fg[j]))
                {
                    int32_t k;
                    for (k = i; k <= j; k++)
                    {
                        if (entry_get_valid(fg[k]) == UINT32_C(0))
                        {
                            return 0;
                        }
                    }
                    return 1;
                }
            }
            return 0;
        }
    }
    return 0;
}

/* This function computes the number of MPU entries that are available for use in creating a new
 * region.
 */
static uint32_t number_available(const xthal_MPU_entry* fg)
{
    int32_t i;
    uint32_t rv = 0;
    int32_t valid_seen = 0;
    for (i = 0; i < XCHAL_MPU_ENTRIES; i++)
    {
        if (valid_seen == 0)
        {
            if (entry_get_valid(fg[i]) != UINT32_C(0))
            {
                valid_seen = 1;
            }
            else
            {
                rv++;
                continue;
            }
        }
        else
        {
            if (entry_get_vstartaddr(fg[i]) == entry_get_vstartaddr(fg[i - 1]))
            {
                rv++;
            }
        }
    }
    return rv;
}

/*
 * This function returns index of the background map entry that maps the address 'first' if there are no
 * enabled/applicable foreground map entries.
 */
static int32_t get_bg_map_index(uint32_t first)
{
    int32_t i;
    for (i = XCHAL_MPU_BACKGROUND_ENTRIES - 1; i >= 0; i--)
    {
        if (first > entry_get_vstartaddr(Xthal_mpu_bgmap[i]))
        {
            return i;
        }
    }
    return 0;
}

/*
 * This function takes the region pointed to by ip, and makes it safe from the aspect of cache coherency, before
 * changing the memory type and possibly corrupting the cache. If wb is 0, then that indicates
 * that we should ignore uncommitted entries. If the inv argument is 0 that indicates that we shouldn't invalidate
 * the cache before switching to bypass. The specified region must always be in the foreground map.
 */
static void safe_region(const xthal_MPU_entry* fg, int32_t ip, int32_t n, uint32_t end_of_segment, uint32_t memoryType, int32_t wb, int32_t inv)
{
    uint32_t addr =  entry_get_vstartaddr(fg[ip]);
    uint32_t length = end_of_segment - addr;
    if (length == UINT32_C(0))
    {
        return; // if the region is empty, there is no need to safe it
    }

    int32_t mt_is_wb = xthal_is_writeback(memoryType);
    int32_t mt_is_ch = xthal_is_cacheable(memoryType);

    // nothing needs to be done in these cases
    if ((mt_is_wb != 0) || ((wb == 0) && ((inv == 0) || (mt_is_ch != 0))))
    {
        return;
    }
    flush_cache(ip, n);
}

/*
 * This function does a series of calls to safe_region() to ensure that no data will be corrupted when changing the memory type
 * of an MPU entry. These calls are made for every entry address in the range[first,end), as well as at any background region boundary
 * in the range[first,end).  In general it is necessary to safe at the background region boundaries, because the memory type could
 * change at that address.
 *
 * This function is written to reuse already needed entries for the background map 'safes' which complicates things somewhat.
 *
 * After the calls to safe region are complete, then the entry attributes are updated for every entry in the range [first,end).
 */
static void do_safe_and_commit_overlaped_regions(xthal_MPU_entry* fg,
        uint32_t first, uint32_t last,
        uint32_t memoryType, uint32_t accessRights, int32_t wb, int32_t inv)
{
    int32_t i;
    uint32_t base_addr;
    i = XCHAL_MPU_ENTRIES - 1;
    while( i >= 0)
    {
        /* Start at the end of the entry array, and work backwards until
         * we find the first entry that maps the requested entry.
         */
        if (entry_get_vstartaddr(fg[i]) < last)
        {
            int32_t j = i;
            while ((j>0) && (entry_get_vstartaddr(fg[j-1]) >= first))
            {
                j--;
            }
            safe_region(fg, j, (i-j)+1, last, memoryType, wb, inv);

            /*
             * The remainder of this function does the actual update of the MPU
             * to the requested memory type and access types.
             *
             * This portion is a good candidate for refactoring into a separate
             * function.
             *
             * One detail complicating the operation is that if the requested
             * region is adjacent to another region with the same memoryType and
             * accessRights, we need to merge those regions.  The reason is that
             * certain operations (DMA, instruction fetch) fail (or generate
             * exceptions) when crossing an MPU region boundary.
             */
            base_addr = first;
            /* if the region preceding, the requested region has the same
             * memoryType and accessRights, we walk backwards through the MPU
             * map to find the start (lowest indexed entry) mapping the
             * preceding region.
             */
            while ((i > 0) && (((entry_get_vstartaddr(fg[i - 1]) >= first)
                   || ((entry_get_memory_type(fg[i - 1]) == memoryType)
                   && (entry_get_access(fg[i - 1]) == accessRights)))))
            {
                i--;
                base_addr = (base_addr > entry_get_vstartaddr(fg[i])) ? entry_get_vstartaddr(fg[i]) : base_addr;
            }

            /* Then we work from the start (of the specified region extended
             * to include any identical adjacent regions) to the entry mapping
             * the end and update it.  All these entries get the same base
             * address to avoid fragmenting the region.
             */
            for (; (i < XCHAL_MPU_ENTRIES) && (entry_get_vstartaddr(fg[i]) < last);
                    i++)
            {
                entry_set_memory_type(&fg[i], memoryType);
                entry_set_access(&fg[i], accessRights);
                entry_set_valid(&fg[i], 1);
                entry_set_vstartaddr(&fg[i], base_addr);
                write_map_entry(fg, i, fg[i]);
            }
            /* Then we check at the end and if there are any identical adjacent
             * regions, we update the start address to be that same as was used
             * for the specified region.  Again, this is done to prevent fragmenting
             * the region.
             */
            for (; (i < XCHAL_MPU_ENTRIES) && (entry_get_valid(fg[i]) != UINT32_C(0))
                            && (entry_get_memory_type(fg[i]) == memoryType)
                            && (entry_get_access(fg[i]) == accessRights); i++)
            {
                entry_set_vstartaddr(&fg[i], base_addr);
                write_map_entry(fg, i, fg[i]);
            }
            break;
        }
        i--;
    }
}

static void safe_and_commit_overlaped_regions(xthal_MPU_entry* fg, uint32_t first,
        uint32_t last, uint32_t memoryType, uint32_t accessRights, int32_t wb, int32_t inv)
{
#if XCHAL_HAVE_CACHEADRDIS
    uint32_t cachedisadr;
    XT_WSR_CACHEADRDIS(0);
#endif
    do_safe_and_commit_overlaped_regions(fg, first, last, memoryType, accessRights, wb, inv);
#if XCHAL_HAVE_CACHEADRDIS
    cachedisadr = xthal_calc_cacheadrdis_internal(fg, XCHAL_MPU_ENTRIES);
    XT_WSR_CACHEADRDIS(cachedisadr);
#endif
}

static void handle_invalid_pred(xthal_MPU_entry* fg, const xthal_MPU_entry* bg, uint32_t first, int32_t ip)
{
    /* Handle the case where there is an invalid entry immediately preceding the entry we
     * are creating.  If the entries addresses correspond to the same bg map, then we
     * make the previous entry valid with same attributes as the background map entry.
     *
     * The case where an invalid entry exists immediately preceding whose address corresponds to a different
     * background map entry is handled by create_aligning_entries_if_required(), so nothing is done here.
     */
    /* todo ... optimization opportunity,  the following block loops through the background map up to 4 times,
     *
     */
    if ((ip == 0) || (entry_get_valid(fg[ip - 1]) != UINT32_C(0)))
    {
        return;
    }

    {
        int32_t i;
        uint32_t fgipm1_addr = entry_get_vstartaddr(fg[ip - 1]);
        int32_t first_in_bg_map = 0;
        int32_t first_bg_map_index = -1;
        int32_t fgipm1_bg_map_index = -1;
        for (i = XCHAL_MPU_BACKGROUND_ENTRIES - 1; i >= 0; i--)
        {
            uint32_t addr = entry_get_vstartaddr(bg[i]);
            if (addr == first)
            {
                first_in_bg_map = 1;
            }
            if ((addr < fgipm1_addr) && (fgipm1_bg_map_index == -1))
            {
                fgipm1_bg_map_index = i;
            }
            if ((addr < first) && (first_bg_map_index == -1))
            {
                first_bg_map_index = i;
            }
        }
        if ((first_in_bg_map == 0) && (first_bg_map_index == fgipm1_bg_map_index))
        {
            // There should be a subsequent entry that falls in the address range of same
            // background map entry ... if not, we have a problem because the following
            // will corrupt the memory map
            xthal_MPU_entry temp = get_entry(fg, entry_get_vstartaddr(fg[ip - 1]), NULL);// parasoft-suppress MISRA2012-DIR-4_7_a "entry_get_vstartaddr does not return error."
            entry_set_vstartaddr(&temp, entry_get_vstartaddr(fg[ip - 1])); // parasoft-suppress MISRA2012-DIR-4_7_a "entry_get_vstartaddr does not return error."
            write_map_entry(fg, ip - 1, temp);
        }
    }
}

/* This function inserts a entry (unless it already exists) with vStartAddress of first.  The new entry has
 * the same accessRights and memoryType as the address first had before the call.
 *
 * If 'invalid' is specified, then insert an invalid region if no foreground entry exists for the address 'first'.
 */
static int32_t insert_entry_if_needed_with_existing_attr(xthal_MPU_entry* fg, const xthal_MPU_entry* bg,
        uint32_t first, int32_t invalid)
{
    int32_t i;
    int32_t ip;
    int32_t infg = 0;
    int32_t found = 0;

    for (i = XCHAL_MPU_ENTRIES - 1; i >= 0; i--)
    {
        if (entry_get_vstartaddr(fg[i]) == first)
        {
            if ((entry_get_valid(fg[i]) != UINT32_C(0)) || (invalid != 0))
            {
                return XTHAL_SUCCESS;
            }
            else
            {
                found = 1;
                ip = i;
                break;
            }
        }
    }
    if (found == 0)
    {
        if (number_available(fg) == UINT32_C(0))
        {
            return XTHAL_OUT_OF_ENTRIES;
        }

        ip = find_entry(fg, first);
        ip = bubble_free_to_ip(fg, ip, 1);
    }
    if (invalid == 0)
    {
        handle_invalid_pred(fg, bg, first, ip);
    }
    xthal_MPU_entry n;
    n = get_entry(fg, first, &infg);

    if ((invalid != 0) && (infg == 0)) // If the entry mapping is currently in the foreground we can't make
        // the entry invalid without corrupting the attributes of the following entry.
    {
        entry_set_valid(&n, 0);
    }
    entry_set_vstartaddr(&n,first);
    write_map_entry(fg, ip, n);
    return XTHAL_SUCCESS;
}

static uint32_t smallest_entry_greater_than_equal(const xthal_MPU_entry* fg, uint32_t x)
{
    int32_t i;
    for (i = 0; i < XCHAL_MPU_ENTRIES; i++)
    {
        if (entry_get_vstartaddr(fg[i]) >= x)
        {
            return entry_get_vstartaddr(fg[i]);
        }
    }
    return 0;
}

/* This function creates background map aligning entries if required.*/
static int32_t create_aligning_entries_if_required(xthal_MPU_entry* fg, const xthal_MPU_entry* bg,
        uint32_t x)
{
    int32_t i;
    int32_t rv;
    uint32_t next_entry_address = 0;
    uint32_t next_entry_valid = 0;
    int32_t preceding_bg_entry_index_x = get_bg_map_index(x);
    uint32_t preceding_bg_entry_x_addr = entry_get_vstartaddr(bg[preceding_bg_entry_index_x]);
    for (i = XCHAL_MPU_ENTRIES - 1; i >= 0; i--)
    {
        if (entry_get_vstartaddr(fg[i]) < x)
        {
            if (entry_get_valid(fg[i]) != UINT32_C(0))
            {
                return XTHAL_SUCCESS; // If there is a valid entry immediately before the proposed new entry
            }
            // ... then no aligning entries are required
            break;
        }
        else
        {
            next_entry_address = entry_get_vstartaddr(fg[i]);
            next_entry_valid = entry_get_valid(fg[i]);
        }
    }

    /*
     * before creating the aligning entry, we may need to create an entry or entries a higher
     * addresses to limit the scope of the aligning entry.
     */
    if ((next_entry_address == UINT32_C(0)) || (next_entry_valid == UINT32_C(0)) || (in_bgmap(next_entry_address) != 0))
    {
        /* in this case, we can just create an invalid entry at the start of the new region because
         * a valid entry could have an alignment problem.  An invalid entry is safe because we know that
         * the next entry is either invalid, or is on a bg map entry
         */
        rv = insert_entry_if_needed_with_existing_attr(fg, bg, x, 1);
        if (rv != XTHAL_SUCCESS)
        {
            return rv;
        }
    }
    else
    {
        uint32_t next_bg_entry_index;
        for (next_bg_entry_index = 0; next_bg_entry_index < ((uint32_t) XCHAL_MPU_BACKGROUND_ENTRIES); next_bg_entry_index++)
        {
            if (entry_get_vstartaddr(bg[next_bg_entry_index]) > x)
            {
                break;
            }
        }
        if (next_entry_address == entry_get_vstartaddr(bg[next_bg_entry_index])) // In this case there is no intervening bg entry
        // between the new entry x, and the next existing entry so, we don't need any limiting entry
        // (the existing next_entry serves as the limiting entry)
        { /* intentionally empty */
        }
        else
        {
            // In this case we need to create a valid region at the background entry that immediately precedes
            // next_entry_address, and then create an invalid entry at the background entry immediately after
            // x
            rv = insert_entry_if_needed_with_existing_attr(fg, bg, entry_get_vstartaddr(get_entry(fg, x, NULL)), 0);// parasoft-suppress MISRA2012-DIR-4_7_a "entry_get_vstartaddr does not return error."
            if (rv != XTHAL_SUCCESS)
            {
                return rv;
            }
            rv = insert_entry_if_needed_with_existing_attr(fg, bg, entry_get_vstartaddr(get_entry(fg, entry_get_vstartaddr(bg[next_bg_entry_index]), NULL)), 1);// parasoft-suppress MISRA2012-DIR-4_7_a "entry_get_vstartaddr does not return error."
            if (rv != XTHAL_SUCCESS)
            {
                return rv;
            }
        }
    }

    /* now we are finally ready to create the aligning entry.*/
    if (!(x == preceding_bg_entry_x_addr))
    {
        rv = insert_entry_if_needed_with_existing_attr(fg, bg, preceding_bg_entry_x_addr, 0);
        if (rv != XTHAL_SUCCESS)
        {
            return rv;
        }
    }
    return XTHAL_SUCCESS;
}

static uint32_t start_initial_region(const xthal_MPU_entry* fg, const xthal_MPU_entry* bg, uint32_t first,
        uint32_t end)
{
    int32_t i;
    uint32_t addr;
    UNUSED(fg);
    for (i = XCHAL_MPU_BACKGROUND_ENTRIES - 1; i >= 0; i--)
    {
        addr = entry_get_vstartaddr(bg[i]);
        if (addr <= first)
        {
            break;
        }
        if (addr < end)
        {
            return addr;
        }
    }
    return first;
}

static int32_t safe_add_region(uint32_t first, uint32_t last, uint32_t accessRights, uint32_t memoryType,
        int32_t writeback, int32_t invalidate, int32_t locked)
{
    /* This function sets the memoryType and accessRights on a region of memory. If necessary additional MPU entries
     * are created so that the attributes of any memory outside the specified region are not changed.
     *
     * This function has 2 stages:
     * 	1) The map is updated one entry at a time to create (if necessary) new entries to mark the beginning and end of the
     * 	   region as well as addition alignment entries if needed.  During this stage the map is always correct, and the memoryType
     * 	   and accessRights for every address remain the same.
     * 	2) The entries inside the update region are then safed for cache consistency (if necessary) and then written with
     * 	   the new accessRights, and memoryType.
     *
     * If the function fails (by running out of available map entries) during stage 1 then everything is still consistent and
     * it is safe to return an error code.
     *
     * extra entries are create if needed to satisfy these alignment conditions:
     *
     * 1) If entry0's Virtual Address Start field is nonzero, then that field must equal one of the Background Map's
     *    Virtual Address Start field values if software ever intends to assert entry0's MPUENB bit.
     * 2) If entryN's MPUENB bit will ever be negated while at the same time entryN+1's MPUENB bit is
     *    asserted, then entryN+1's Virtual Address Start field must equal one of the Background Map's Virtual Address Start field values.
     *
     * Between 0 and 3 available entries will be used by this function.
     *
     * This function keeps a copy of the current map in 'fg'.  This is kept in sync with contents of the MPU at all times.
     *
     */

    int32_t rv;

    xthal_MPU_entry fg[XCHAL_MPU_ENTRIES];
    UNUSED(locked);
    rv = xthal_read_map(fg);
    if (rv != XTHAL_SUCCESS)
    {
        return rv;
    }
    /* First we check and see if consecutive entries at first, and first + size already exist.
     * in this important special case we don't need to do anything but safe and update the entries [first, first+size).
     *
     */

    if (needed_entries_exist(fg, first, last)==0)
    {
        uint32_t x;
        uint32_t pbg;

        /*
         * If we are tight on entries, the first step is to remove any redundant entries in the MPU
         * to make room to ensure that there is room for the new entries we need.
         *
         * We need to call it here ... once we have started transforming the map it is too late
         * (the process involves creating inaccessible entries that could potentially get removed).
         */
        if (number_available(fg) < XCHAL_MPU_WORST_CASE_ENTRIES_FOR_REGION)
        {
            remove_inaccessible_entries(fg);
        }
        // First we create foreground entries that 'duplicate' background entries to aide in
        // maintaining proper alignment.
        rv = create_aligning_entries_if_required(fg, Xthal_mpu_bgmap, first);
        if (rv != XTHAL_SUCCESS)
        {
            return rv;
        }

        // First we write the terminating entry for our region
        // 5 cases:
        // 1) end is at the end of the address space, then we don't need to do anything ... takes 0 entries
        // 2) There is an existing entry at end ... another nop ... 0 entries
        // 3) end > than any existing entry ... in this case we just create a new invalid entry at end to mark
        //    end of the region.  No problem with alignment ... this takes 1 entry
        // 4) otherwise if there is a background map boundary between end and x ,the smallest existing entry that is
        //    greater than end, then we first create an equivalent foreground map entry for the background map entry that immediately
        //    precedes x, and then we write an invalid entry for end. Takes 2 entries
        // 5) otherwise x is in the same background map entry as end, in this case we write a new foreground entry with the existing
        //    attributes at end

        if (last == UINT32_C(0xFFFFFFFF))
        { /* the end is the end of the address space ... do nothing */
        }
        else
        {
            x = smallest_entry_greater_than_equal(fg, last);
            if (last == x)
            { /* another nop */
            }
            else if (last > x)
            { /* there is no entry that has a start after the new region ends
             ... we handle this by creating an invalid entry at the end point */
                rv = insert_entry_if_needed_with_existing_attr(fg, Xthal_mpu_bgmap, last, 1);
                if (rv != XTHAL_SUCCESS)
                {
                    return rv;
                }
            }
            else
            {
                pbg = entry_get_vstartaddr(fg[get_bg_map_index(x)]);
                /* so there is an existing entry we must deal with.  We next need to find
                 * if there is an existing background entry in between the end of
                 * the new region and beginning of the next.
                 */
                if ((pbg != x) && (pbg > last))
                {
                    /* okay ... there is an intervening background map entry.  We need
                     * to handle this by inserting an aligning entry (if the architecture requires it)
                     * and then placing writing an invalid entry at end.
                     */
                    rv = insert_entry_if_needed_with_existing_attr(fg,
                            Xthal_mpu_bgmap, pbg, 0);
                    if (rv != XTHAL_SUCCESS)
                    {
                        return rv;
                    }
                    rv = insert_entry_if_needed_with_existing_attr(fg,
                            Xthal_mpu_bgmap, last, 1);
                    if (rv != XTHAL_SUCCESS)
                    {
                        return rv;
                    }
                }
                else
                {
                    /* ok so there are no background map entry in between end and x, in this case
                     * we just need to create a new entry at end writing the existing attributes.
                     */
                    rv = insert_entry_if_needed_with_existing_attr(fg, Xthal_mpu_bgmap, last, 1);
                }
                if (rv != XTHAL_SUCCESS)
                {
                    return rv;
                }
                else
                {
                    ; /* No action required - ; is optional */
                }
            }
        }

        /* last, but not least we need to insert a entry at the starting address for our new region */
        rv = insert_entry_if_needed_with_existing_attr(fg, Xthal_mpu_bgmap, start_initial_region(fg, Xthal_mpu_bgmap, first, last), 0);
        if (rv != XTHAL_SUCCESS)
        {
            return rv;
        }
    }
    // up to this point, the attributes of every byte in the address space should be the same as when this function
    // was called.
    safe_and_commit_overlaped_regions(fg, first, last, memoryType, accessRights, writeback, invalidate);
    return XTHAL_SUCCESS;
}



#else

static void entry_set_lock(xthal_MPU_entry* en, uint32_t lock)
{
    if (lock != UINT32_C(0))
    {
        XTHAL_MPU_ENTRY_SET_LOCK(*en);
    }
}

static uint32_t entry_get_lock(xthal_MPU_entry en)
{
    return XTHAL_MPU_ENTRY_GET_LOCK(en);
}

static void update_entry(xthal_MPU_entry* fg, int32_t index, uint32_t addr, uint32_t ar,
        uint32_t mt, uint32_t locked, uint32_t valid)
{
    entry_set_vstartaddr(&fg[index], addr);
    entry_set_access(&fg[index], ar);
    entry_set_memory_type(&fg[index], mt);
    entry_set_lock(&fg[index], locked);
    entry_set_valid(&fg[index], valid);
    write_map_entry(fg, index, fg[index]);
}

static int32_t overlap(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
    return (((a >= c) && (a <= d)) ||  (((b >= c) && (b <= d))) || ((a <= c) && (b >= d))) ? 1 : 0;
}

static int32_t highest_locked_entry(const xthal_MPU_entry* fg)
{
    int32_t lock_base;
    for (lock_base = XCHAL_MPU_ENTRIES - 1; lock_base >= 0; lock_base--)
    {
        if (entry_get_lock(fg[lock_base]) != UINT32_C(0))
        {
            break;
        }
    }
    return lock_base;
}
/*
    Scan from entry 0 to the highest indexed locked entry.  We consider any
    address mapped by an entry in that range is locked.
 */
static int32_t is_region_locked(const xthal_MPU_entry* fg, int32_t lock_base, uint32_t first, uint32_t last)
{
    int32_t i;
    for (i = 0; i <= lock_base ; i++)
    {
        uint32_t addr = entry_get_vstartaddr(fg[i]);
        uint32_t enable = entry_get_valid(fg[i]);
        uint32_t lock = entry_get_lock(fg[i]);
        uint32_t next = ((i + 1) < XCHAL_MPU_ENTRIES) ? entry_get_vstartaddr(fg[i+1]) : UINT32_C(0xffffffff);
        if ((enable != UINT32_C(0)) && (lock != UINT32_C(0)) && (overlap(addr, next, first, last) != 0))
        {
            return 1;
        }
    }
    return 0;
}

static int32_t existing_region(const xthal_MPU_entry* fg, uint32_t lowaddr,
        uint32_t highaddr, int32_t i)
{
    int32_t rv = ((entry_get_vstartaddr(fg[i]) == lowaddr) && (entry_get_vstartaddr(fg[i+1]) == highaddr)) ? 1 : 0;
    return rv;
}


/* Safely update the region:
 * This is a bit tricky because the region being updated might currently
 * be composed of bits of several regions each of which may have different
 * memory types (and thus different requirements for cache flush/invalidation
 * */
static void safe_update_region(xthal_MPU_entry* fg, int32_t entry_num,
        uint32_t ar, uint32_t mt, int32_t wb, int32_t inv, int32_t locked,
        uint32_t lowaddr)
{
    int32_t mt_is_wb = xthal_is_writeback(mt);
    int32_t mt_is_ch = xthal_is_cacheable(mt);

    // Possibly need to flush / invalidate cache
    if (((mt_is_wb == 0) && (wb != 0)) || ((mt_is_ch == 0) && (inv != 0)))
    {
        flush_cache(entry_num, 1);
    }
    /* now the region is ready. Do the final update to the desired attributes
     */
    update_entry(fg, entry_num, lowaddr, ar, mt, (uint32_t) locked, 1);
}

static void write_disabled_entries(xthal_MPU_entry* fg, int32_t entry_num,
        uint32_t lowaddr, uint32_t highaddr)
{
    update_entry(fg, entry_num, lowaddr, 0, 0, 0, 0);
    update_entry(fg, entry_num + 1, highaddr, 0, 0, 0, 0);
}

static void copy_entry(xthal_MPU_entry* fg, int32_t e1, int32_t e2)
{
    fg[e2] = fg[e1];
    write_map_entry(fg, e2, fg[e2]);
}

/* Compare 2 MPU entries after masking off the entry-index as well
 * as the reserved bits.
 */
static int32_t entries_equal(const xthal_MPU_entry* fg, int32_t e1, int32_t e2)
{
    if ((fg[e1].at & UINT32_C(0x001fff00)) != (fg[e2].at & UINT32_C(0x001fff00)))
    {
        return 0;
    }
    if ((fg[e1].as & UINT32_C(0xffffffe1)) != (fg[e2].as & UINT32_C(0xffffffe1)))
    {
        return 0;
    }
    return 1;
}

/* compact the entries by shifting consecutive identical entries to attempt to
 * put at least 3 consecutive identical entries at the base, base+1, and base+2.
 * Also any entries that have a zero start address and are not enabled, can be
 * safely removed to make room for the new entries.*/
static int32_t compact(xthal_MPU_entry* fg, int32_t base)
{
    int32_t i;
    for (i = XCHAL_MPU_ENTRIES - 2; i >= base; i--)
    {
        int32_t k;
        int32_t j = i;
        while ((j > base) && ((entries_equal(fg, j - 1, i) != 0)
                || (((entry_get_vstartaddr(fg[j - 1]) == UINT32_C(0))
                        && (entry_get_valid(fg[j - 1]) == UINT32_C(0))))))
        {
            j--;
            if (j == base)
            {
                return XTHAL_SUCCESS;
            }
        }
        if (j > 0)
        {
           for (k = j; k < i; k++)
           {
              copy_entry(fg, j - 1, k);
           }
        }
    }
    if ((entries_equal(fg, base, base + 1) != 0) && (entries_equal(fg, base, base + 2) != 0))
    {
        return XTHAL_SUCCESS;
    }
    else
    {
        return XTHAL_OUT_OF_ENTRIES;
    }
}

struct regions
{
    int32_t num;
    uint32_t start[XCHAL_MPU_ENTRIES];
    uint32_t end[XCHAL_MPU_ENTRIES];
};

static void zero(xthal_MPU_entry* fg, int32_t index)
{
    update_entry(fg, index, 0, 0, 0, 0, 0);
}

static int32_t enclosed(const struct regions* r, uint32_t start, uint32_t end)
{
    int32_t i;
    for (i = 0; i < r->num; i++)
    {
        if ((start >= r->start[i]) && (end <= r->end[i]))
        {
            return 1;
        }
    }
    return 0;
}

static void add_to_region(struct regions* r, uint32_t start, uint32_t end)
{
    int32_t i;
    for (i = 0; i < r->num; i++)
    {
        // new region covers old region
        if ((start <= r->start[i]) && (end >= r->end[i]))
        {
            r->start[i] = start;
            r->end[i] = end;
            return;
        }
        else if ((start <= r->start[i]) && (end >= r->start[i]))
        {
            r->start[i] = start;
            return;
        }
        else if ((end >= r->end[i]) && (start <= r->end[i]))
        {
            r->end[i] = end;
            return;
        }
        else
        {
            /* Intentionally blank */
        }
    }
    r->end[i] = end;
    r->start[i] = start;
    r->num++;
    return;
}

/* After repeated calls to xthal_set_region_attribute(), the MPU map can become
 * cluttered with unused entries (because they are eclipsed by entries with lower
 * indexes).  This function finds those and cleans them out.
 */
static void clear_eclipsed_regions(xthal_MPU_entry* fg, int32_t base)
{
    int32_t i;
    struct regions r;
    (void) memset(&r, 0, sizeof(r));
    int32_t previous_active = 0;
    for (i = base; i < (XCHAL_MPU_ENTRIES - 1); i++)
    {
        uint32_t start = entry_get_vstartaddr(fg[i]);
        uint32_t end = entry_get_vstartaddr(fg[i + 1]);
        int32_t active = ((entry_get_valid(fg[i]) != UINT32_C(0)) && (end > start)) ? 1 : 0;
        if (active != 0)
        {
            if (enclosed(&r, start, end) > 0)
            {
                if (previous_active == 0)
                {
                    zero(fg, i);
                }
                previous_active = 0;
            }
            else
            {
                add_to_region(&r, start, end);
                previous_active = 1;
            }
        }
        else
        {
            if (previous_active == 0)
            {
                zero(fg, i);
            }
            previous_active = 0;
        }
    }
}

static int32_t make_space_for_new_region(xthal_MPU_entry* fg, int32_t base)
{
    int32_t rv = compact(fg, base);
    if (rv == XTHAL_SUCCESS)
    {
        return rv;
    }
    clear_eclipsed_regions(fg, base);
    return compact(fg, base);
}

static int32_t safe_add_region_(xthal_MPU_entry* fg, uint32_t first, uint32_t last, uint32_t ar, uint32_t mt,
        int32_t wb, int32_t inv, int32_t locked)
{
    int32_t rv;
    int32_t lock_base = highest_locked_entry(fg); /* the highest level locked
    entry ... we will only change higher numbered entries */
    // There need to be at least two unlocked entries with higher indexes
    // than any locked entry
    if (lock_base > (XCHAL_MPU_ENTRIES - 3))
    {
        return XTHAL_MPU_LOCKED;
    }
    if (is_region_locked(fg, lock_base, first, last) != 0)
        {
            return XTHAL_MPU_LOCKED;
        }
    // Optimize the common case where the region we need exists and is at the top
    // of the unlocked entries.
    if (existing_region(fg, first, last, lock_base+1) != 0)
    {
        safe_update_region(fg, lock_base + 1, ar, mt, wb, inv, locked, first);
        rv = XTHAL_SUCCESS;
    }
    else
    {
        rv = make_space_for_new_region(fg, lock_base + 1);
        if (rv != XTHAL_SUCCESS)
        {
            return rv;
        }
        write_disabled_entries(fg, lock_base+1, first, last);
        safe_update_region(fg, lock_base+1, ar, mt, wb, inv, locked, first);
    }

    return rv;
}

static int32_t safe_add_region(uint32_t first, uint32_t last, uint32_t ar, uint32_t mt,
        int32_t wb, int32_t inv, int32_t locked)
{
#if XCHAL_HAVE_CACHEADRDIS
    uint32_t cachedisadr;
    XT_WSR_CACHEADRDIS(0);
#endif
    int32_t rv;
    xthal_MPU_entry fg[XCHAL_MPU_ENTRIES];
    rv = xthal_read_map(fg);
    if (rv != XTHAL_SUCCESS)
    {
        return rv;
    }
    rv = safe_add_region_(fg, first, last, ar, mt, wb, inv, locked);
#if XCHAL_HAVE_CACHEADRDIS
    cachedisadr = xthal_calc_cacheadrdis_internal(fg, XCHAL_MPU_ENTRIES);
    XT_WSR_CACHEADRDIS(cachedisadr);
#endif
    return rv;
}

#endif
#endif

/*
 * This function is intended as an MPU specific version of
 * xthal_set_region_attributes(). xthal_set_region_attributes() calls
 * this function for MPU configurations.
 *
 * This function sets the attributes for the region [vaddr, vaddr+size)
 * in the MPU.
 *
 * Depending on the state of the MPU this function will require from
 * 0 to 3 unused MPU entries.
 *
 * This function typically will move, add, and subtract entries from
 * the MPU map during execution, so that the resulting map may
 * be quite different than when the function was called.
 *
 * This function does make the following guarantees:
 *    1) The MPU access map remains in a valid state at all times
 *       during its execution.
 *    2) At all points during (and after) completion the memoryType
 *       and accessRights remain the same for all addresses
 *       that are not in the range [vaddr, vaddr+size).
 *    3) If XTHAL_SUCCESS is returned, then the range
 *       [vaddr, vaddr+size) will have the accessRights and memoryType
 *       specified.
 *
 * The accessRights parameter should be either a 4-bit value corresponding
 * to an MPU access mode (as defined by the XTHAL_AR_.. constants), or
 * XTHAL_MPU_USE_EXISTING_ACCESS_RIGHTS.
 *
 * The memoryType parameter should be either a bit-wise or-ing of XTHAL_MEM_..
 * constants that represent a valid MPU memoryType, a 9-bit MPU memoryType
 * value, or XTHAL_MPU_USE_EXISTING_MEMORY_TYPE.
 *
 * In addition to the error codes that xthal_set_region_attribute()
 * returns, this function can also return: XTHAL_BAD_ACCESS_RIGHTS
 * (if the access rights bits map to an unsupported combination), or
 * XTHAL_OUT_OF_ENTRIES (if there are not enough unused MPU entries).
 *
 * If this function is called with an invalid MPU map, then this function
 * will return one of the codes that is returned by xthal_check_map().
 *
 * The flag, XTHAL_CAFLAG_EXPAND, is not supported.
 *
 */
int32_t xthal_mpu_set_region_attribute(void* vaddr, uint32_t size, uint32_t accessRights, uint32_t memoryType, uint32_t flags) // parasoft-suppress MISRA2012-RULE-8_13_a "vaddr cannont be passed 'const' due to potential cache invalidation"
{
#if XCHAL_HAVE_MPU
    uint32_t first;
    uint32_t last;
    int32_t rv;
    uint32_t ar =  accessRights;
    uint32_t mt =  memoryType;

    if ((flags & XTHAL_CAFLAG_EXPAND) != UINT32_C(0))
    {
        return XTHAL_UNSUPPORTED;
    }
    if (size == UINT32_C(0))
    {
        return XTHAL_ZERO_SIZED_REGION;
    }
    if (((flags & XTHAL_CAFLAG_MPU_LOCK) != UINT32_C(0)) && (XCHAL_MPU_LOCK == 0))
    {
        return XTHAL_UNSUPPORTED;
    }

    first = (uint32_t) vaddr; // parasoft-suppress MISRA2012-RULE-11_6 "Pointer conv OK"
    last = first + size;
    if (last != UINT32_C(0xFFFFFFFF))
    {
        last--;
    }
    if (first >= last)
    {
        return XTHAL_INVALID_ADDRESS_RANGE; // Wraps around
    }

    if ((ar & ((uint32_t) XTHAL_MPU_USE_EXISTING_ACCESS_RIGHTS)) != UINT32_C(0))
    {
        /* parasoft-begin-suppress MISRA2012-DIR-4_7_a "ar does not need to be checked." */
        ar = entry_get_access(xthal_get_entry_for_address(vaddr, NULL));
        /* parasoft-end-suppress MISRA2012-DIR-4_7_a "ar does not need to be checked." */
    }
    else
    {
        ar = encode_access_rights(ar);
        if (ar == ((uint32_t) XTHAL_BAD_ACCESS_RIGHTS))
        {
            return XTHAL_BAD_ACCESS_RIGHTS;
        }
    }
    if ((mt & ((uint32_t) XTHAL_MPU_USE_EXISTING_MEMORY_TYPE)) != UINT32_C(0))
    {
        mt = entry_get_memory_type(xthal_get_entry_for_address(vaddr, NULL));
    }
    else
    {
        if ((mt & UINT32_C(0xffffe000)) != UINT32_C(0)) // Tests if any of the XTHAL MEM flags are present
        {
           mt = (uint32_t) xthal_encode_memory_type(mt);
        }
        else
        {
            if ((mt & UINT32_C(0xfffffe00)) != UINT32_C(0)) // Tests if any of bits from 9 to 12 are set indicating
            {
                // that the mt was improperly shifted
                // we flag this as an error
               return XTHAL_BAD_MEMORY_TYPE;
            }
        }
        if (mt == ((uint32_t) XTHAL_BAD_MEMORY_TYPE))
        {
            return XTHAL_BAD_MEMORY_TYPE;
        }
    }
    if ((flags & XTHAL_CAFLAG_EXACT) != UINT32_C(0))
    {
        if ((mpu_aligned(first) == UINT32_C(0)) || (mpu_aligned(last + 1U) == UINT32_C(0)))
        {
            return XTHAL_INEXACT;
        }
    }

    first = mpu_align(first, (flags & XTHAL_CAFLAG_NO_PARTIAL));
    if (last != UINT32_C(0xffffffff))
    {
        last = mpu_align(last + 1U, (flags & XTHAL_CAFLAG_NO_PARTIAL) == UINT32_C(0) ? UINT32_C(1) : UINT32_C(0));
        if (first >= last)
        {
            return ((flags & XTHAL_CAFLAG_NO_PARTIAL) != UINT32_C(0)) ? XTHAL_ZERO_SIZED_REGION : 0;
        }
    }
    /* parasoft-begin-suppress MISRA2012-DIR-4_7_a "ar does not need to be checked." */

    rv = safe_add_region(first,
                         last,
                         ar,
                         mt,
                         ((flags & XTHAL_CAFLAG_NO_AUTO_WB)  == UINT32_C(0)) ? 1 : 0,
                         ((flags & XTHAL_CAFLAG_NO_AUTO_INV) == UINT32_C(0)) ? 1 : 0,
                         ((flags & XTHAL_CAFLAG_MPU_LOCK) == UINT32_C(0)) ? 0 : 1);
    /* parasoft-end-suppress MISRA2012-DIR-4_7_a "ar does not need to be checked." */
    xthal_icache_sync();
    return rv;
#else
    UNUSED(vaddr);
    UNUSED(size);
    UNUSED(accessRights);
    UNUSED(memoryType);
    UNUSED(flags);
    return XTHAL_UNSUPPORTED;
#endif
}

