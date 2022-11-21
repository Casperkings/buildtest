
// mpu_init.c -- MPU initialization.

// Copyright (c) 2015-2020 Cadence Design Systems, Inc.
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
#include <xtensa/config/core.h>
#include <xtensa/config/system.h>


//-----------------------------------------------------------------------------
//  The xos_mpu_init() function constructs a table of MPU entries from the 
//  memory map of the executable and sets up the MPU with appropriate regions
//  and permissions. Each memory (dataram, sysram, etc.) is divided into two
//  regions - the part used by this executable and the part not used. The used
//  parts are set up with supervisor only privileges. The unused parts are set
//  up with user privileges but with supervisor execute privilege disabled.
//
//  The MPU init is not enabled by default (and this file is not included in
//  the XOS library build either). The reason is that the default MPU setup
//  may not be suitable for your needs, and it is very likely that this file
//  will need to be modified before you can use it. This file is provided as
//  a reference and a quick start point for you to customize.
//
//  Note that the regions are derived from the static information extracted
//  at link time. This does not account for any dynamic allocation that your
//  application might be doing at run time. For example, perhaps the entire
//  dataram is unused statically, but you intend to use it at run time with
//  a local memory allocator. In that case, the default datraam setup will
//  not work.
//
//  Also note that the region boundaries need to be adjusted to align to 
//  the MPU minimum region size and alignment. See the code below.
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// The following sets of symbols are defined by the linker script. The symbols
// for a specific memory are defined only if that memory exists, e.g. the irom0
// symbols will be defined only if irom0 is configured. They are all declared
// weak here so the ones that are undefined will be set to zero.
//
// For each memory there are 3 symbols. The _start symbol defines the starting
// address for that memory, and the _end symbol defines the ending (actually
// one past the last addressable byte). The _max symbol defines the last used
// byte position. The space from _start to _max is used by this program, and
// the space from _max to _end is unused. Note this does not account for any
// dynamic allocation that might happen at runtime.
//
// If you create a custom LSP with additions to the memory map, then those
// additions will not automatically show up here. For example if you add a
// device region in the memory map, you must manually add the symbols for that
// region below, both in the definitions and the table.
//-----------------------------------------------------------------------------

__attribute__((weak))
extern char _memmap_mem_dram0_start, _memmap_mem_dram0_end, _memmap_mem_dram0_max;
__attribute__((weak))
extern char _memmap_mem_dram1_start, _memmap_mem_dram1_end, _memmap_mem_dram1_max;

__attribute__((weak))
extern char _memmap_mem_drom0_start, _memmap_mem_drom0_end, _memmap_mem_drom0_max;
__attribute__((weak))
extern char _memmap_mem_drom1_start, _memmap_mem_drom1_end, _memmap_mem_drom1_max;

__attribute__((weak))
extern char _memmap_mem_iram0_start, _memmap_mem_iram0_end, _memmap_mem_iram0_max;
__attribute__((weak))
extern char _memmap_mem_iram1_start, _memmap_mem_iram1_end, _memmap_mem_iram1_max;

__attribute__((weak))
extern char _memmap_mem_irom0_start, _memmap_mem_irom0_end, _memmap_mem_irom0_max;
__attribute__((weak))
extern char _memmap_mem_irom1_start, _memmap_mem_irom1_end, _memmap_mem_irom1_max;

__attribute__((weak))
extern char _memmap_mem_sram_start, _memmap_mem_sram_end, _memmap_mem_sram_max;
__attribute__((weak))
extern char _memmap_mem_srom_start, _memmap_mem_srom_end, _memmap_mem_srom_max;


//-----------------------------------------------------------------------------
// Structure that defines a memory region and its properties.
//-----------------------------------------------------------------------------

typedef struct mregion {
    char *   start;    // Start address
    char *   end;      // End address
    uint32_t ar;       // Access rights
    uint32_t mt;       // Memory type
} mregion;


//-----------------------------------------------------------------------------
// Table of memory regions for this executable. The addresses are filled in at
// link time. This table assigns default access rights and memory type values
// to each region, but these may not always be suitable for your application.
// You must edit the table and modify it to suit your needs.
//
// IMPORTANT: These addresses may not always align perfectly with the MPU min
// region alignment requirements. The code below tries to align the addresses
// before setting up the MPU. It expands the used regions if needed, because
// shrinking the used regions will not work.
//-----------------------------------------------------------------------------

#if XCHAL_HAVE_MPU

static mregion mem_regions[] = {
#if (XCHAL_NUM_DATARAM > 0)
    // dram0 used by this application, supervisor RW
    {&_memmap_mem_dram0_start, &_memmap_mem_dram0_max, XTHAL_AR_RW,   XTHAL_MEM_NON_CACHEABLE},
    // unused dram0, user and supervisor RW
    {&_memmap_mem_dram0_max,   &_memmap_mem_dram0_end, XTHAL_AR_RWrw, XTHAL_MEM_NON_CACHEABLE},
#endif
#if (XCHAL_NUM_DATARAM > 1)
    // dram1 used by this application, supervisor RW
    {&_memmap_mem_dram1_start, &_memmap_mem_dram1_max, XTHAL_AR_RW,   XTHAL_MEM_NON_CACHEABLE},
    // unused dram1, user and supervisor RW
    {&_memmap_mem_dram1_max,   &_memmap_mem_dram1_end, XTHAL_AR_RWrw, XTHAL_MEM_NON_CACHEABLE},
#endif
#if (XCHAL_NUM_DATAROM > 0)
    // drom0 used by this application, supervisor R
    {&_memmap_mem_drom0_start, &_memmap_mem_drom0_max, XTHAL_AR_R,  XTHAL_MEM_NON_CACHEABLE},
    // unused drom0, user and supervisor R
    {&_memmap_mem_drom0_max,   &_memmap_mem_drom0_end, XTHAL_AR_Rr, XTHAL_MEM_NON_CACHEABLE},
#endif
#if (XCHAL_NUM_DATAROM > 1)
    // drom1 used by this application, supervisor R
    {&_memmap_mem_drom1_start, &_memmap_mem_drom1_max, XTHAL_AR_R,  XTHAL_MEM_NON_CACHEABLE},
    // unused drom1, user and supervisor R
    {&_memmap_mem_drom1_max,   &_memmap_mem_drom1_end, XTHAL_AR_Rr, XTHAL_MEM_NON_CACHEABLE},
#endif
#if (XCHAL_NUM_INSTRAM > 0)
    // iram0 used by this application, supervisor RX
    {&_memmap_mem_iram0_start, &_memmap_mem_iram0_max, XTHAL_AR_RX,    XTHAL_MEM_NON_CACHEABLE},
    // unused iram0, no execute for supervisor
    {&_memmap_mem_iram0_max,   &_memmap_mem_iram0_end, XTHAL_AR_RWrwx, XTHAL_MEM_NON_CACHEABLE},
#endif
#if (XCHAL_NUM_INSTRAM > 1)
    // iram1 used by this application, supervisor RX
    {&_memmap_mem_iram1_start, &_memmap_mem_iram1_max, XTHAL_AR_RX,    XTHAL_MEM_NON_CACHEABLE},
    // unused iram1, no execute for supervisor
    {&_memmap_mem_iram1_max,   &_memmap_mem_iram1_end, XTHAL_AR_RWrwx, XTHAL_MEM_NON_CACHEABLE},
#endif
#if (XCHAL_NUM_INSTROM > 0)
    // irom0 used by this application, supervisor RX
    {&_memmap_mem_irom0_start, &_memmap_mem_irom0_max, XTHAL_AR_RX,    XTHAL_MEM_NON_CACHEABLE},
    // unused irom0, no execute for supervisor
    {&_memmap_mem_irom0_max,   &_memmap_mem_irom0_end, XTHAL_AR_RWrwx, XTHAL_MEM_NON_CACHEABLE},
#endif
#if (XCHAL_NUM_INSTROM > 1)
    // irom1 used by this application, supervisor RX
    {&_memmap_mem_irom1_start, &_memmap_mem_irom1_max, XTHAL_AR_RX,    XTHAL_MEM_NON_CACHEABLE},
    // unused irom1, no execute for supervisor
    {&_memmap_mem_irom1_max,   &_memmap_mem_irom1_end, XTHAL_AR_RWrwx, XTHAL_MEM_NON_CACHEABLE},
#endif
#if defined (XSHAL_RAM_VADDR)
    // sysram used by this application, supervisor RWX
    {&_memmap_mem_sram_start, &_memmap_mem_sram_max, XTHAL_AR_RWX,   XTHAL_MEM_WRITEBACK},
    // unused sysram, no execute for supervisor
    {&_memmap_mem_sram_max,   &_memmap_mem_sram_end, XTHAL_AR_RWrwx, XTHAL_MEM_WRITEBACK},
#endif
#if defined (XSHAL_ROM_VADDR)
    // sysrom used by this application, supervisor RX
    {&_memmap_mem_srom_start, &_memmap_mem_srom_max, XTHAL_AR_RX,    XTHAL_MEM_WRITEBACK},
    // unused sysrom, no execute for supervisor
    {&_memmap_mem_srom_max,   &_memmap_mem_srom_end, XTHAL_AR_RWrwx, XTHAL_MEM_WRITEBACK},
#endif
};

#endif


//-----------------------------------------------------------------------------
// Generate a table of MPU entries from the mem region table and program the
// MPU.
//-----------------------------------------------------------------------------
int32_t
xos_mpu_init()
{
#if XCHAL_HAVE_MPU

    xthal_MPU_entry entries[XCHAL_MPU_ENTRIES];
    int32_t         index = 0;

    int32_t  count = sizeof(mem_regions) / sizeof(mem_regions[0]);
    int32_t  first = 1;
    int32_t  i;
    uint32_t beg;
    uint32_t end;

    // The first entry begins by mapping from address 0 with no access.
    entries[0] = (xthal_MPU_entry) XTHAL_MPU_ENTRY(0, 1, XTHAL_AR_NONE, XTHAL_MEM_DEVICE);

    // Iterate over the region table and select entries in ascending order
    // of start address. Take advantage of the fact that entries are paired.

    while (1) {
        char *    low  = (char *) 0xffffffff;
        mregion * lowp = NULL;

        for (i = 0; i < count; i += 2) {
            if ((mem_regions[i].start != 0) && (mem_regions[i].start < low)) {
                low  = mem_regions[i].start;
                lowp = &(mem_regions[i]);
            }
        }
        if (low == (char *)0xffffffff) {
            break;
        }

        // If the selected entry defines a region of zero size, skip it.
        if (lowp->start != lowp->end) {
            if (first) {
                beg = (uint32_t)lowp->start & -XCHAL_MPU_ALIGN;
                first = 0;
            }
            else {
                beg = ((uint32_t)lowp->start + XCHAL_MPU_ALIGN - 1) & -XCHAL_MPU_ALIGN;
            }
            end = ((uint32_t)lowp->end + XCHAL_MPU_ALIGN - 1) & -XCHAL_MPU_ALIGN;

            // If the last entry we wrote has the same start address, overwrite it.
            // Else write the next one.
            if (XTHAL_MPU_ENTRY_GET_VSTARTADDR(entries[index]) != beg) {
                index++;
            }
            entries[index] = (xthal_MPU_entry) XTHAL_MPU_ENTRY(beg, 1, lowp->ar, lowp->mt);
        }

        // Don't want to see this one again.
        lowp->start = 0;

        // Process the second entry of the pair, since we know they must be
        // back to back in address space.
        lowp++;
        if (lowp->start != lowp->end) {
            beg = ((uint32_t)lowp->start + XCHAL_MPU_ALIGN - 1) & -XCHAL_MPU_ALIGN;
            end = ((uint32_t)lowp->end + XCHAL_MPU_ALIGN - 1) & -XCHAL_MPU_ALIGN;

            // Again, decide whether to use the current entry or the next one.
            if (XTHAL_MPU_ENTRY_GET_VSTARTADDR(entries[index]) != beg) {
                index++;
            }
            entries[index] = (xthal_MPU_entry) XTHAL_MPU_ENTRY(beg, 1, lowp->ar, lowp->mt);
        }

        // Don't want to see this one again.
        lowp->start = 0;

        // Block off memory beyond the end of the region, for now. This entry
        // will get overwritten in the next round if needed.
        if (XTHAL_MPU_ENTRY_GET_VSTARTADDR(entries[index]) != end) {
            index++;
        }
        entries[index] = (xthal_MPU_entry) XTHAL_MPU_ENTRY(end, 1, XTHAL_AR_NONE, XTHAL_MEM_DEVICE);
    }

    // Skip past the last entry we wrote.
    index++;

#if XCHAL_MPU_LOCK
    // Clear the remaining entries.
    for (i = index; i < XCHAL_MPU_ENTRIES; i++) {
        entries[i].as = 0;
        entries[i].at = 0;
    }

    // Use the full table.
    index = XCHAL_MPU_ENTRIES;
#endif

    // Verify that the map looks good.
    if (xthal_check_map(entries, index) != XTHAL_SUCCESS) {
        return -1;
    }

    // Update the MPU.
    xthal_write_map(entries, index);

    return 0;

#else

    return -1;

#endif
}

