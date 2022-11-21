
// MPU initialization.

// Copyright (c) 2021 Cadence Design Systems, Inc.
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


// Symbols defined by linker script.
extern char _memmap_mem_sram_start, _memmap_mem_sram_end, _memmap_mem_sram_max;

// Marks the end of sram used by supervisor mode (main app).
// _sram_max is set by the linker, we round up to next MPU aligned address.
#define SRAM_SMAX   (((uint32_t)&_memmap_mem_sram_max + XCHAL_MPU_ALIGN - 1) & -XCHAL_MPU_ALIGN)

// Start value for first MPU entry marking active user region.
#define SRAM_UMEM1  (SRAM_SMAX + XCHAL_MPU_ALIGN)

// Start value for second MPU entry marking active user region.
#define SRAM_UMEM2  (((uint32_t)&_memmap_mem_sram_end - XCHAL_MPU_ALIGN) & -XCHAL_MPU_ALIGN)

// The region available for user mode actually stretches from SRAM_SMAX
// up to the end of SRAM. The UMEM1/2 entries are offset inwards simply
// to avoid two entries with the same address. At runtime they change
// on every user-mode context switch so it doesn't really matter what
// values they start off with.
// If user-mode memory isolation is not required, these two entries can
// be disabled or removed. Then all user-mode memory will be visible to
// all user-mode threads.

// This table is constructed based off the "sim" LSP for this config.
// It will have to be adapted to the actual memory map of the target
// hardware. Be sure to adjust the table_size count to match.

// Note that window exception vectors are placed in iram, so the iram
// has been set up for privileged access only. You may wish to change
// that if user-mode access to iram is needed.

struct xthal_MPU_entry
mpu_table[XCHAL_MPU_ENTRIES] = {
  XTHAL_MPU_ENTRY(0x00000000, 1, XTHAL_AR_NONE, XTHAL_MEM_DEVICE), // unused
  XTHAL_MPU_ENTRY(0x3ffc0000, 1, XTHAL_AR_RWrw, XTHAL_MEM_NON_CACHEABLE), // dram1
  XTHAL_MPU_ENTRY(0x3ffe0000, 1, XTHAL_AR_RWrw, XTHAL_MEM_NON_CACHEABLE), // dram0
  XTHAL_MPU_ENTRY(0x40000000, 1, XTHAL_AR_RWX,  XTHAL_MEM_NON_CACHEABLE), // iram0
  XTHAL_MPU_ENTRY(0x40020000, 1, XTHAL_AR_NONE, XTHAL_MEM_DEVICE), // unused
  XTHAL_MPU_ENTRY(0x50000000, 1, XTHAL_AR_RX,   XTHAL_MEM_WRITEBACK), // srom
  XTHAL_MPU_ENTRY(0x51000000, 1, XTHAL_AR_NONE, XTHAL_MEM_DEVICE), // unused
  XTHAL_MPU_ENTRY(0x60000000, 1, XTHAL_AR_RWX,  XTHAL_MEM_WRITEBACK), // sram_start
  XTHAL_MPU_ENTRY(0,          1, XTHAL_AR_RWX,  XTHAL_MEM_WRITEBACK), // supervisor region end
  XTHAL_MPU_ENTRY(0,          1, XTHAL_AR_RWX,  XTHAL_MEM_WRITEBACK), // swap entry 1
  XTHAL_MPU_ENTRY(0,          1, XTHAL_AR_RWX,  XTHAL_MEM_WRITEBACK), // swap entry 2
  XTHAL_MPU_ENTRY(0x64000000, 1, XTHAL_AR_NONE, XTHAL_MEM_DEVICE), // end of sram
};

uint32_t mpu_table_size = 12;

void *   umem_start;
uint32_t umem_size;

// The first entry of the swap pair above will be at index 13.
// This is config-specific and is used by the main application.

uint32_t mpu_priv_mem_idx = 13;


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int32_t
mpu_init()
{
#if XCHAL_HAVE_MPU

    // These need to be set at runtime (not computable at compile time).
    mpu_table[8]  = (xthal_MPU_entry) XTHAL_MPU_ENTRY(SRAM_SMAX,  1, XTHAL_AR_RWX, XTHAL_MEM_WRITEBACK);
    mpu_table[9]  = (xthal_MPU_entry) XTHAL_MPU_ENTRY(SRAM_UMEM1, 1, XTHAL_AR_RWX, XTHAL_MEM_WRITEBACK);
    mpu_table[10] = (xthal_MPU_entry) XTHAL_MPU_ENTRY(SRAM_UMEM2, 1, XTHAL_AR_RWX, XTHAL_MEM_WRITEBACK);

#if XCHAL_MPU_LOCK
    // Use the full table.
    mpu_table_size = XCHAL_MPU_ENTRIES;
#endif

    // Verify that the map looks good.
    if (xthal_check_map(mpu_table, mpu_table_size) != XTHAL_SUCCESS) {
        return -1;
    }

    // Update the MPU.
    xthal_write_map(mpu_table, mpu_table_size);

    // Set the region start and size for the user-mode pool.
    umem_start = (void *) SRAM_SMAX;
    umem_size  = (void *)&_memmap_mem_sram_end - (void *)SRAM_SMAX;

    return 0;

#else

    return -1;

#endif
}

