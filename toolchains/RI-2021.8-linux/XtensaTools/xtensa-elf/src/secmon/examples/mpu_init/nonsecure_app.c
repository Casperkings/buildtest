// Copyright (c) 2020-2021 Cadence Design Systems, Inc.
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

// nonsecure_app.c -- Nonsecure application example code


#include "xtensa/secmon.h"
#include "xtensa/secmon-common.h"

#include <xtensa/config/secure.h>
#include <xtensa/config/system.h>
#include <xtensa/core-macros.h>
#include <xtensa/corebits.h>
#include <xtensa/xtruntime.h>
#include <stdio.h>


#define UNUSED(x) ((void)(x))

const uint32_t mem_start[] = {
#if (defined XSHAL_RAM_VADDR)
    XSHAL_RAM_VADDR,
#endif
#if (defined XSHAL_ROM_VADDR)
    XSHAL_ROM_VADDR,
#endif
#if (defined XCHAL_INSTROM0_VADDR)
    XCHAL_INSTROM0_VADDR,
#endif
#if (defined XCHAL_INSTRAM0_VADDR)
    XCHAL_INSTRAM0_VADDR,
#endif
#if (defined XCHAL_INSTRAM1_VADDR)
    XCHAL_INSTRAM1_VADDR,
#endif
#if (defined XCHAL_DATAROM0_VADDR)
    XCHAL_DATAROM0_VADDR,
#endif
#if (defined XCHAL_DATARAM0_VADDR)
    XCHAL_DATARAM0_VADDR,
#endif
#if (defined XCHAL_DATARAM1_VADDR)
    XCHAL_DATARAM1_VADDR,
#endif
};
const uint32_t num_mems = sizeof(mem_start) / sizeof(uint32_t);

static int32_t print_map_entry (unsigned int index, const xthal_MPU_entry * me);
static int32_t print_map (const xthal_MPU_entry * me, unsigned n);
static int32_t mpu_init_test(void);
static int32_t is_mem_start(uint32_t addr);
static int32_t check_start(const xthal_MPU_entry * me, uint32_t addr, char *name, int32_t code);
static int32_t check_end(const xthal_MPU_entry * me, uint32_t addr, char *name, int32_t code);

static int32_t is_mem_start(uint32_t addr) {
    uint32_t i;
    for (i = 0; i < num_mems; i++) {
        if (addr == mem_start[i]) {
            return 1;
        }
    }
    return 0;
}

static int32_t check_start(const xthal_MPU_entry * me, uint32_t addr, char *name, int32_t code)
{
    int32_t ret = 0;
    if ((XTHAL_MPU_ENTRY_GET_VSTARTADDR(*me) == addr) &&
        ((XTHAL_MPU_ENTRY_GET_VALID(*me) != 1) || (XTHAL_MPU_ENTRY_GET_LOCK(*me) != 1))) {
        printf("ERROR: %s start: as 0x%x at 0x%x\n", name, me->at, me->as);
        ret = code;
    }
    return ret;
}

static int32_t check_end(const xthal_MPU_entry * me, uint32_t addr, char *name, int32_t code)
{
    int32_t ret = 0;
    if (XTHAL_MPU_ENTRY_GET_VSTARTADDR(*me) == addr) {
        if (!xtsm_secure_addr_overlap(addr, addr + 1) && is_mem_start(addr)) {
            // Nonsecure memory start shares secure memory end.  If entry is valid, it 
            // will be unlocked; if not valid, it will be locked.
            if (XTHAL_MPU_ENTRY_GET_VALID(*me) == XTHAL_MPU_ENTRY_GET_LOCK(*me)) {
                printf("ERROR: %s secure end (+ nonsecure start): as 0x%x at 0x%x\n",
                        name, me->at, me->as);
                ret = code;
            }
        } else if (xtsm_secure_addr_overlap(addr, addr + 1) && is_mem_start(addr)) {
            // Secure memory start shares secure memory end; entry must be valid and locked.
            if (!XTHAL_MPU_ENTRY_GET_VALID(*me) || !XTHAL_MPU_ENTRY_GET_LOCK(*me)) {
                printf("ERROR: %s secure end (+ secure start): as 0x%x at 0x%x\n",
                        name, me->at, me->as);
                ret = code;
            }
        } else if ((XTHAL_MPU_ENTRY_GET_VALID(*me) != 0) || (XTHAL_MPU_ENTRY_GET_LOCK(*me) != 1)) {
            printf("ERROR: %s secure end: as 0x%x at 0x%x\n", name, me->at, me->as);
            ret = code;
        }
    }
    return ret;
}

static int32_t check_access(uint32_t addr, char *name, int32_t code)
{
    // Check that access rights don't include any of PS.RING==1 bits (i.e. rwx)
    // in order to confirm MPU table was successfully overridden from default
    uint8_t ar;
    int32_t status = xthal_region_access_rights((void *)addr, 1, &ar);
    if (status != XTHAL_SUCCESS) {
        printf("ERROR: %s xthal_region_access_rights(0x%x, 1, &ar) returned %d\n", name, addr, status);
        return code;
    }
    if (((ar & (XTHAL_R | XTHAL_W | XTHAL_X)) == 0) ||
        ((ar & (XTHAL_r | XTHAL_w | XTHAL_x)) != 0)) {
        printf("ERROR: Invalid %s access rights (0x%x) at address 0x%x\n", name, ar, addr);
        return code;
    }
    return 0;
}

static int32_t
print_map_entry (unsigned int index, const xthal_MPU_entry * me)
{
    int ret = 0;
    int i;
    for (i = 0; i < XCHAL_MPU_BACKGROUND_ENTRIES; i++)
        if (XTHAL_MPU_ENTRY_GET_VSTARTADDR(*me) >
            XTHAL_MPU_ENTRY_GET_VSTARTADDR (Xthal_mpu_bgmap[i])) {
            continue;
        }
    i--;
    printf ("%3d 0x%08x %01xV %01xL %2d %4d %2d %4d\n", index,
            XTHAL_MPU_ENTRY_GET_VSTARTADDR(*me),
            XTHAL_MPU_ENTRY_GET_VALID(*me),
            XTHAL_MPU_ENTRY_GET_LOCK(*me),
            XTHAL_MPU_ENTRY_GET_ACCESS(*me), 
            XTHAL_MPU_ENTRY_GET_MEMORY_TYPE(*me),
            XTHAL_MPU_ENTRY_GET_ACCESS(Xthal_mpu_bgmap[i]),
            XTHAL_MPU_ENTRY_GET_MEMORY_TYPE(Xthal_mpu_bgmap[i]));

#if (defined XCHAL_HAVE_SECURE_INSTRAM0) && XCHAL_HAVE_SECURE_INSTRAM0
    ret |= check_start(me, XCHAL_INSTRAM0_SECURE_START, "IRAM0", 0x1);
    ret |= check_end(me, XCHAL_INSTRAM0_SECURE_START + XCHAL_INSTRAM0_SECURE_SIZE, "IRAM0", 0x2);
    ret |= check_access(XCHAL_INSTRAM0_SECURE_START, "IRAM0", 0x3);
#endif
#if (defined XCHAL_HAVE_SECURE_INSTRAM1) && XCHAL_HAVE_SECURE_INSTRAM1
    ret |= check_start(me, XCHAL_INSTRAM1_SECURE_START, "IRAM1", 0x10);
    ret |= check_end(me, XCHAL_INSTRAM1_SECURE_START + XCHAL_INSTRAM1_SECURE_SIZE, "IRAM1", 0x20);
    ret |= check_access(XCHAL_INSTRAM1_SECURE_START, "IRAM1", 0x30);
#endif
#if (defined XCHAL_HAVE_SECURE_DATARAM0) && XCHAL_HAVE_SECURE_DATARAM0
    ret |= check_start(me, XCHAL_DATARAM0_SECURE_START, "DRAM0", 0x100);
    ret |= check_end(me, XCHAL_DATARAM0_SECURE_START + XCHAL_DATARAM0_SECURE_SIZE, "DRAM0", 0x200);
    ret |= check_access(XCHAL_DATARAM0_SECURE_START, "DRAM0", 0x300);
#endif
#if (defined XCHAL_HAVE_SECURE_DATARAM1) && XCHAL_HAVE_SECURE_DATARAM1
    ret |= check_start(me, XCHAL_DATARAM1_SECURE_START, "DRAM1", 0x1000);
    ret |= check_end(me, XCHAL_DATARAM1_SECURE_START + XCHAL_DATARAM1_SECURE_SIZE, "DRAM1", 0x2000);
    ret |= check_access(XCHAL_DATARAM1_SECURE_START, "DRAM1", 0x3000);
#endif
    return ret;
}

static int32_t
print_map (const xthal_MPU_entry * me, unsigned n)
{
    unsigned int i;
    int32_t ret = 0;
    for (i = 0; i < n; i++) {
        ret |= print_map_entry (i, &me[i]);
    }
    return ret;
}

static int32_t mpu_init_test(void)
{
    int32_t ret;
    printf("FG MAP:\n");
    xthal_MPU_entry me[XCHAL_MPU_ENTRIES];
    xthal_read_map (me);
    ret = print_map (me, XCHAL_MPU_ENTRIES);
    printf("BG MAP:\n");
    print_map (Xthal_mpu_bgmap, XCHAL_MPU_BACKGROUND_ENTRIES);
    return ret;
}

#if (XCHAL_HAVE_VECBASE) && (XCHAL_VECBASE_LOCK)
static int32_t vecbase_test(void)
{
    uint32_t vecbase = XT_RSR_VECBASE();
    uint32_t newvecbase; 
    XT_WSR_VECBASE(vecbase + 0x10000000);
    newvecbase = XT_RSR_VECBASE();
    XT_WSR_VECBASE(vecbase);
    return (vecbase == newvecbase) ? 0 : 1;
}
#endif


static int32_t mpu_nswrdis_test(void)
{
    uint32_t mpucfg = 0, mpuenb = 0;
    uint32_t orig_mpucfg, orig_mpuenb;
    int32_t  wrdis_set, ret;

    __asm__ __volatile__ ("rsr.mpucfg %0" : "=a"(orig_mpucfg));
    __asm__ __volatile__ ("rsr.mpuenb %0" : "=a"(orig_mpuenb));

    // Validate MPUCFG.NSWRDIS is set during secure boot
    wrdis_set = ((orig_mpucfg >> XTHAL_MPUCFG_NSWRDIR_SHIFT) & XTHAL_MPUCFG_NSWRDIR_MASK);
    printf("NSWRDIS bit is %sset, %s\n", wrdis_set ? "" : "NOT ", wrdis_set ? "OK" : "FAILED");
    if (!wrdis_set) {
        return -1;
    }

    // Validate MPUCFG.NSWRDIS and MPUENB are not changeable 
    mpuenb = ~orig_mpuenb;
    mpucfg = orig_mpucfg & ~(XTHAL_MPUCFG_NSWRDIR_MASK << XTHAL_MPUCFG_NSWRDIR_SHIFT);
    __asm__ __volatile__ ("wsr.mpuenb %0" :: "a"(mpuenb));
    __asm__ __volatile__ ("wsr.mpucfg %0" :: "a"(mpucfg));
    __asm__ __volatile__ ("rsr.mpuenb %0" : "=a"(mpuenb));
    __asm__ __volatile__ ("rsr.mpucfg %0" : "=a"(mpucfg));

    ret = ((mpuenb != orig_mpuenb) || (mpucfg != orig_mpucfg));
    printf("MPUENB is %schangeable, MPUCFG.NSWRDIS is %schangeable; %s\n",
            (mpuenb == orig_mpuenb) ? "NOT " : "",
            (mpucfg == orig_mpucfg) ? "NOT " : "",
            ret ? "FAILED" : "OK");
    return ret;
}


int main(int argc, char **argv)
{
    int ret = 0, ret1 = 0, ret2 = 0, ret3 = 0;
    UNUSED(argc);
    UNUSED(argv);

    printf("Hello, nonsecure world\n");
    ret = xtsm_init();
    if (ret != 0) {
        printf("Error: xtsm_init() FAILED (%d)\n", ret);
        return ret;
    }

    ret1 = mpu_init_test();
    printf("MPU test %s\n", ret1 ? "FAILED" : "OK");
#if (XCHAL_HAVE_VECBASE) && (XCHAL_VECBASE_LOCK)
    ret2 = vecbase_test();
    printf("VECBASE test %s\n", ret2 ? "FAILED" : "OK");
#else
    printf("VECBASE test skipped\n");
#endif
    ret3 = mpu_nswrdis_test();
    printf("MPU NSWRDIS test %s\n", ret3 ? "FAILED" : "OK");

    printf("\nOverall test result: %s\n", (ret1 || ret2 || ret3) ? "FAILED" : "PASSED");
    return (ret1 || ret2 || ret3);
}

