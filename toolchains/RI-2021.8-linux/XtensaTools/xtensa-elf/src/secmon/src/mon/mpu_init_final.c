// Copyright (c) 2004-2021 Cadence Design Systems, Inc.
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

// mpu_init_final.c -- MPU initialization and final validation


#include <xtensa/xtruntime.h>
#include <xtensa/tie/xt_mmu.h>

#include "xtensa/secmon-common.h"
#include "xtensa/secmon-defs.h"


//-----------------------------------------------------------------------------
//  Externs
//-----------------------------------------------------------------------------
extern const struct xthal_MPU_entry __xt_mpu_init_table[];
extern const uint32_t __xt_mpu_init_table_size;
extern const uint32_t __xt_mpu_init_table_size_secure;
extern void secmon_illegal_mpu_entry_trap(uint32_t entry, uint32_t at, uint32_t as);


//-----------------------------------------------------------------------------
//  Prototypes
//-----------------------------------------------------------------------------
void secmon_mpu_init_final(void);


//-----------------------------------------------------------------------------
//  Helper function imported from hal. Secure code cannot call libhal.a APIs
//  since it may have been compiled for windowed ABI.
//-----------------------------------------------------------------------------
inline static int32_t is_cacheable(uint32_t mt)
{
    int32_t rv = 0;
    if (((0x180U & mt) != 0U) || ((mt & 0x18U) == 0x10U) || ((mt & 0x30U) == 0x30U))
    {
        rv = 1;
    }
    return rv;
}


//-----------------------------------------------------------------------------
//  Retroactively validate MPU settings; they have already been set and locked.
//
//  All MPU entries mapping secure code must be uncached and locked, and all 
//  locked entries must be at the beginning of the table.  If not, a fatal 
//  error is raised.
//
//  NOTE: If the MPU table is modified to lock nonsecure entries, this logic
//  will need to be updated.
//-----------------------------------------------------------------------------
static void secmon_mpu_verify(void)
{
    int32_t secure = 1, i;
    for (i = 0; i < (int32_t)__xt_mpu_init_table_size; i++) {
        xthal_MPU_entry e;
        e.as = XT_RPTLB0(i);
        e.at = XT_RPTLB1(i);
        uint32_t addr = XTHAL_MPU_ENTRY_GET_VSTARTADDR(e);
        if (!XTHAL_MPU_ENTRY_GET_LOCK(e)) {
            // First unlocked entry should mark end of secure start (valid) and
            // secure end (!valid) entries.
            secure = 0;
        }
        if (secure) {
            uint32_t first, last;
            if (XTHAL_MPU_ENTRY_GET_VALID(e)) {
                // Check address at start entry
                first = addr;
                last = addr + 4;
            } else {
                // Check address prior to end entry
                first = addr - 4;
                last = addr - 1;
            }
            // Check #1: secure entries should point to secure memory,
            // be locked (already checked), and be uncached.
            if (!(xtsm_secure_addr_overlap(first, last)) ||
                (is_cacheable(XTHAL_MPU_ENTRY_GET_MEMORY_TYPE(e)))) {
                // Does not return
                secmon_illegal_mpu_entry_trap(i, e.at, e.as);
            }
        } else {
            // Check #2: entries at and after first unlocked entry should
            // all be nonsecure and unlocked.
            if ((xtsm_secure_addr_overlap(addr, addr + 4)) ||
                XTHAL_MPU_ENTRY_GET_LOCK(e)) {
                // Does not return
                secmon_illegal_mpu_entry_trap(i, e.at, e.as);
            }
        }
    }
}


//-----------------------------------------------------------------------------
//  Program nonsecure MPU entries, lock secure entries, and validate MPU table.
//-----------------------------------------------------------------------------
void secmon_mpu_init_final(void)
{
    uint32_t mpuenb = 0;
    int32_t i;
    // 1) Program nonsecure entries in reverse order, tracking new mpuenb bits
    // 2) Lock secure entries in reverse order (they reside at low end of table)
    // 3) Set new mpuenb after all entries are finalized
    // NOTE: both sizes validated to be > 0 in crt1-sm.S
    for (i = __xt_mpu_init_table_size - 1; i >= 0; i--) {
        xthal_MPU_entry e = __xt_mpu_init_table[i];
        e.at = (e.at & UINT32_C(0xffffffe0)) | (i & UINT32_C(0x1f));
        if (XTHAL_MPU_ENTRY_GET_VALID(e)) {
            mpuenb |= (1 << i);
        }
        if (i < (int32_t)__xt_mpu_init_table_size_secure) {
            XTHAL_MPU_ENTRY_SET_LOCK(e);
        }
        xthal_mpu_set_entry(e);
    }
    XT_WSR_MPUENB(mpuenb);
    secmon_mpu_verify();
}
