//
// misc.c - miscellaneous constants
//

// Copyright (c) 2004-2020 Cadence Design Systems Inc.
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
#include <xtensa/tie/xt_core.h>
#if XCHAL_HAVE_XEA3
#include <xtensa/tie/xt_exception_dispatch.h>
#endif

#if XCHAL_HAVE_FXLK || XCHAL_HAVE_WWDT
#include <xtensa/config/key.h>
#endif

// Software release info (not configuration-specific!):
const uint32_t  Xthal_release_major		= XTHAL_RELEASE_MAJOR;
const uint32_t  Xthal_release_minor		= XTHAL_RELEASE_MINOR;
const char * const  Xthal_release_name		= XTHAL_RELEASE_NAME;
#ifdef XTHAL_RELEASE_INTERNAL
const char * const  Xthal_release_internal	= XTHAL_RELEASE_INTERNAL;
#else
const char * const  Xthal_release_internal	= NULL;
#endif
/*  Old format, for backward compatibility:  */
const uint32_t Xthal_rev_no = (((uint32_t) XTHAL_MAJOR_REV) << 16U)
        | ((uint32_t) XTHAL_MINOR_REV);

// number of registers in register window, or number of registers if not windowed
const uint32_t  Xthal_num_aregs		= XCHAL_NUM_AREGS;
const uint8_t Xthal_num_aregs_log2	= XCHAL_NUM_AREGS_LOG2;

const uint8_t Xthal_memory_order		= XCHAL_MEMORY_ORDER;
const uint8_t Xthal_have_windowed		= XCHAL_HAVE_WINDOWED;
const uint8_t Xthal_have_density		= XCHAL_HAVE_DENSITY;
const uint8_t Xthal_have_booleans		= XCHAL_HAVE_BOOLEANS;
const uint8_t Xthal_have_loops		= XCHAL_HAVE_LOOPS;
const uint8_t Xthal_have_nsa		= XCHAL_HAVE_NSA;
const uint8_t Xthal_have_minmax		= XCHAL_HAVE_MINMAX;
const uint8_t Xthal_have_sext		= XCHAL_HAVE_SEXT;
const uint8_t Xthal_have_clamps		= XCHAL_HAVE_CLAMPS;
const uint8_t Xthal_have_mac16		= XCHAL_HAVE_MAC16;
const uint8_t Xthal_have_mul16		= XCHAL_HAVE_MUL16;
const uint8_t Xthal_have_fp		= XCHAL_HAVE_FP;
const uint8_t Xthal_have_speculation	= XCHAL_HAVE_SPECULATION;
const uint8_t Xthal_have_exceptions	= XCHAL_HAVE_EXCEPTIONS;
const uint8_t Xthal_xea_version		= XCHAL_XEA_VERSION;
const uint8_t Xthal_have_interrupts	= XCHAL_HAVE_INTERRUPTS;
#if XCHAL_HAVE_XEA5 // todo check this
const uint8_t Xthal_have_highlevel_interrupts	= 0;
#else
const uint8_t Xthal_have_highlevel_interrupts	= XCHAL_HAVE_HIGHLEVEL_INTERRUPTS;
#endif
const uint8_t Xthal_have_nmi		= XCHAL_HAVE_NMI;
const uint8_t Xthal_have_prid		= XCHAL_HAVE_PRID;
const uint8_t Xthal_have_release_sync	= XCHAL_HAVE_RELEASE_SYNC;
const uint8_t Xthal_have_s32c1i		= XCHAL_HAVE_S32C1I;
const uint8_t Xthal_have_threadptr	= XCHAL_HAVE_THREADPTR;

const uint8_t Xthal_have_pif		= XCHAL_HAVE_PIF;
const uint16_t Xthal_num_writebuffer_entries	= XCHAL_NUM_WRITEBUFFER_ENTRIES;

const uint32_t  Xthal_build_unique_id	= XCHAL_BUILD_UNIQUE_ID;
// Release info for hardware targeted by software upgrades:
/* parasoft-begin-suppress MISRA2012-RULE-7_2 "XCHAL_HW_CONFIGID0 shared with assembly can't have U suffix" */
const uint32_t  Xthal_hw_configid0		= ((uint32_t) XCHAL_HW_CONFIGID0);
/* parasoft-begin-end MISRA2012-RULE-7_2 "XCHAL_HW_CONFIGID0 shared with assembly can't have U suffix" */
const uint32_t  Xthal_hw_configid1		= ((uint32_t) XCHAL_HW_CONFIGID1);
const uint32_t  Xthal_hw_release_major	= XCHAL_HW_VERSION_MAJOR;
const uint32_t  Xthal_hw_release_minor	= XCHAL_HW_VERSION_MINOR;
const char * const  Xthal_hw_release_name	= XCHAL_HW_VERSION_NAME;
const uint32_t  Xthal_hw_min_version_major	= XCHAL_HW_MIN_VERSION_MAJOR;
const uint32_t  Xthal_hw_min_version_minor	= XCHAL_HW_MIN_VERSION_MINOR;
const uint32_t  Xthal_hw_max_version_major	= XCHAL_HW_MAX_VERSION_MAJOR;
const uint32_t  Xthal_hw_max_version_minor	= XCHAL_HW_MAX_VERSION_MINOR;
#ifdef XCHAL_HW_RELEASE_INTERNAL
const char * const  Xthal_hw_release_internal	= XCHAL_HW_RELEASE_INTERNAL;
#else
const char * const  Xthal_hw_release_internal	= NULL;
#endif

/*  MMU related info...  */

const uint8_t Xthal_have_spanning_way	= XCHAL_HAVE_SPANNING_WAY;
const uint8_t Xthal_have_identity_map	= XCHAL_HAVE_IDENTITY_MAP;
const uint8_t Xthal_have_mimic_cacheattr	= XCHAL_HAVE_MIMIC_CACHEATTR;
const uint8_t Xthal_have_xlt_cacheattr	= XCHAL_HAVE_XLT_CACHEATTR;
const uint8_t Xthal_have_cacheattr	= XCHAL_HAVE_CACHEATTR;
const uint8_t Xthal_have_tlbs		= XCHAL_HAVE_TLBS;
#if XCHAL_HAVE_MPU
const uint8_t Xthal_mmu_asid_bits		= 0;
const uint8_t Xthal_mmu_asid_kernel	= 0;
const uint8_t Xthal_mmu_rings		= 0;
const uint8_t Xthal_mmu_ring_bits		= 0;
const uint8_t Xthal_mmu_sr_bits		= 0;
const uint8_t Xthal_mmu_ca_bits		= 0;
#else
const uint8_t Xthal_mmu_asid_bits		= XCHAL_MMU_ASID_BITS;
const uint8_t Xthal_mmu_asid_kernel	= XCHAL_MMU_ASID_KERNEL;
const uint8_t Xthal_mmu_rings		= XCHAL_MMU_RINGS;
const uint8_t Xthal_mmu_ring_bits		= XCHAL_MMU_RING_BITS;
const uint8_t Xthal_mmu_sr_bits		= XCHAL_MMU_SR_BITS;
const uint8_t Xthal_mmu_ca_bits		= XCHAL_MMU_CA_BITS;
#endif
#if XCHAL_HAVE_TLBS
const uint32_t  Xthal_mmu_max_pte_page_size	= XCHAL_MMU_MAX_PTE_PAGE_SIZE;
const uint32_t  Xthal_mmu_min_pte_page_size	= XCHAL_MMU_MIN_PTE_PAGE_SIZE;
const uint8_t Xthal_itlb_way_bits	= XCHAL_ITLB_WAY_BITS;
const uint8_t Xthal_itlb_ways	= XCHAL_ITLB_WAYS;
const uint8_t Xthal_itlb_arf_ways	= XCHAL_ITLB_ARF_WAYS;
const uint8_t Xthal_dtlb_way_bits	= XCHAL_DTLB_WAY_BITS;
const uint8_t Xthal_dtlb_ways	= XCHAL_DTLB_WAYS;
const uint8_t Xthal_dtlb_arf_ways	= XCHAL_DTLB_ARF_WAYS;
#else
const uint32_t  Xthal_mmu_max_pte_page_size	= 0;
const uint32_t  Xthal_mmu_min_pte_page_size	= 0;
const uint8_t Xthal_itlb_way_bits	= 0;
const uint8_t Xthal_itlb_ways	= 0;
const uint8_t Xthal_itlb_arf_ways	= 0;
const uint8_t Xthal_dtlb_way_bits	= 0;
const uint8_t Xthal_dtlb_ways	= 0;
const uint8_t Xthal_dtlb_arf_ways	= 0;
#endif


/*  Internal memories...  */

const uint8_t Xthal_num_instrom	= XCHAL_NUM_INSTROM;
const uint8_t Xthal_num_instram	= XCHAL_NUM_INSTRAM;
const uint8_t Xthal_num_datarom	= XCHAL_NUM_DATAROM;
const uint8_t Xthal_num_dataram	= XCHAL_NUM_DATARAM;
const uint8_t Xthal_num_xlmi	= XCHAL_NUM_XLMI;

/*  Define arrays of internal memories' addresses and sizes:  */
/* parasoft-begin-suppress MISRA2012-RULE-20_7 "enclosing macro argument in ()'s not appropriate when token pasting" */
#define MEMTRIPLET(n,mem,memcap)	I_MEMTRIPLET(n,mem,memcap)
#define I_MEMTRIPLET(n,mem,memcap)	MEMTRIPLET##n(mem,memcap)
#define MEMTRIPLET0(mem,memcap) \
	const uint32_t  Xthal_##mem##_vaddr[1] = { 0 }; \
	const uint32_t  Xthal_##mem##_paddr[1] = { 0 }; \
	const uint32_t  Xthal_##mem##_size [1] = { 0 };
#define MEMTRIPLET1(mem,memcap) \
	const uint32_t  Xthal_##mem##_vaddr[1] = { XCHAL_##memcap##0_VADDR }; \
	const uint32_t  Xthal_##mem##_paddr[1] = { XCHAL_##memcap##0_PADDR }; \
	const uint32_t  Xthal_##mem##_size [1] = { XCHAL_##memcap##0_SIZE };
#define MEMTRIPLET2(mem,memcap) \
	const uint32_t  Xthal_##mem##_vaddr[2] = { XCHAL_##memcap##0_VADDR, XCHAL_##memcap##1_VADDR }; \
	const uint32_t  Xthal_##mem##_paddr[2] = { XCHAL_##memcap##0_PADDR, XCHAL_##memcap##1_PADDR }; \
	const uint32_t  Xthal_##mem##_size [2] = { XCHAL_##memcap##0_SIZE,  XCHAL_##memcap##1_SIZE };
/* parasoft-end-suppress MISRA2012-RULE-20_7 "enclosing macro argument in ()'s not appropriate when token pasting" */

MEMTRIPLET(XCHAL_NUM_INSTROM, instrom, INSTROM)
MEMTRIPLET(XCHAL_NUM_INSTRAM, instram, INSTRAM)
MEMTRIPLET(XCHAL_NUM_DATAROM, datarom, DATAROM)
MEMTRIPLET(XCHAL_NUM_DATARAM, dataram, DATARAM)
MEMTRIPLET(XCHAL_NUM_XLMI,    xlmi,    XLMI)

/*  Timer info...  */

const uint8_t Xthal_have_ccount	= XCHAL_HAVE_CCOUNT;
const uint8_t Xthal_num_ccompare	= XCHAL_NUM_TIMERS;

int32_t xthal_lock_vecbase(void)
{
#if XCHAL_VECBASE_LOCK && !XCHAL_HAVE_XEA5 // todo
    uint32_t x = (uint32_t) XT_RSR_VECBASE();
    x |= (uint32_t) VECBASE_LOCK;
    XT_WSR_VECBASE(x);
    return XTHAL_SUCCESS;
#else
    return XTHAL_UNSUPPORTED;
#endif
}

/* Enable (or disable) CSR parity (if configured) */
int32_t xthal_enable_csr_parity(uint8_t enable)
{
#if XCHAL_HAVE_CSR_PARITY
    uint32_t csrpctl = (uint32_t) XT_RSR_CSRPCTL();
    int32_t old_enable = (csrpctl & (uint32_t) (CSRPARITY_ENABLED)) != UINT32_C(0) ? 1 : 0;
    if (enable != 0U)
    {
        csrpctl |= (uint32_t) (CSRPARITY_ENABLED);
    }
    else
    {
        csrpctl &= ~((uint32_t) (CSRPARITY_ENABLED));
    }
    XT_WSR_CSRPCTL((int32_t) csrpctl);
    return old_enable;
#else
    UNUSED(enable);
    return XTHAL_UNSUPPORTED;
#endif
}
/* Split-Lock related functions */

/* Inject an error for testing purposes.
 * core         : 0 or 1 (the core on which the error is to be injected)
 * bit_pos      : 15 bit value for bit position to inject error
 */
int32_t xthal_fxlk_inject_error(uint32_t core, uint32_t bit_pos)
{
#if XCHAL_HAVE_FXLK
    if ((core <= 1) && (bit_pos < XT_RER(XTHAL_FXLK_NUM_OUTPUTS_REGNO)))
    {
        uint32_t val = (((uint32_t) XCHAL_FXLK_EKEY) << ((uint32_t) XTHAL_FXLK_KEY_SHIFT))
                | ((core & UINT32_C(0x1)) << 19) | (bit_pos & UINT32_C(0x7FFF));
        XT_WER(val, (uint32_t) XTHAL_FXLK_ERROR_INJECTION_REGNO);
        return XTHAL_SUCCESS;
    }
    else
    {
        return XTHAL_INVALID;
    }
#else
    UNUSED(core);
    UNUSED(bit_pos);
    return XTHAL_UNSUPPORTED;
#endif
}

/*
 * Clear an injected error before system reset.
 */
int32_t xthal_fxlk_clear_error(void)
{
#if XCHAL_HAVE_FXLK
    XT_WER(((uint32_t) XCHAL_FXLK_C1KEY) << ((uint32_t) XTHAL_FXLK_KEY_SHIFT),
    (uint32_t) XTHAL_FXLK_CLEAR_REGNO);
    XT_WER(((uint32_t) XCHAL_FXLK_C2KEY) << ((uint32_t) XTHAL_FXLK_KEY_SHIFT),
    (uint32_t) XTHAL_FXLK_CLEAR_REGNO);
    return XTHAL_SUCCESS;
#else
    return XTHAL_UNSUPPORTED;
#endif
}

/* set the 20-bit countdown timer between error detection and system reset. */
int32_t xthal_fxlk_set_countdown(uint32_t val)
{
#if XCHAL_HAVE_FXLK
    if (val < (UINT32_C(1) << 20U))
    {
        XT_WER(((((uint32_t) XCHAL_FXLK_RKEY) << ((uint32_t) XTHAL_FXLK_KEY_SHIFT))
                        | (val & UINT32_C(0xFFFFF))),
                (uint32_t) XTHAL_FXLK_RESET_COUNTDOWN_REGNO);
        return XTHAL_SUCCESS;
    }
    else
    {
        return XTHAL_INVALID;
    }
#else
    UNUSED(val);
    return XTHAL_UNSUPPORTED;
#endif
}

/*
 * sets the split-lock compare state. Setting to 0 suppresses the detector of
 * any mismatches between the cores.
 */
int32_t xthal_fxlk_set_cmp_enable(uint32_t enable)
{
#if XCHAL_HAVE_FXLK
    enable = enable & UINT32_C(1);
    uint32_t enabled = (XT_RER(((uint32_t) XTHAL_FXLK_STATUS_REGNO)) & XTHAL_REG_STATUS_CMP_ENABLE) ? UINT32_C(1) : UINT32_C(0);
    if ((enable ^ enabled) != UINT32_C(0))
    {
        XT_WER(((uint32_t) XCHAL_FXLK_T1KEY) << ((uint32_t) XTHAL_FXLK_KEY_SHIFT), (uint32_t) XTHAL_FXLK_CMP_EN_TOGGLE_REGNO);
        XT_WER(((uint32_t) XCHAL_FXLK_T2KEY) << ((uint32_t) XTHAL_FXLK_KEY_SHIFT), (uint32_t) XTHAL_FXLK_CMP_EN_TOGGLE_REGNO);
    }
    return (int32_t) enabled;
#else
    UNUSED(enable);
    return XTHAL_UNSUPPORTED;
#endif
}
/* This function sets the initial counter values and initializes the timer. It
   can only be called once, and is irreversible */
int32_t xthal_wwdt_initialize(uint32_t initial, uint32_t bound, uint32_t reset_countdown)
{
#if XCHAL_HAVE_WWDT
    XT_WER(((bound >> XTHAL_WWDT_BOUND_SHIFT) & ((uint32_t) XTHAL_WWDT_BOUND_MASK)) |
            ((uint32_t) XCHAL_WWDT_BKEY << (uint32_t) XTHAL_WWDT_KEYSHIFT), XTHAL_WWDT_BOUND);
    XT_WER((reset_countdown & ((uint32_t) XTHAL_WWDT_RESET_COUNTDOWN_MASK)) |
            ((uint32_t) XCHAL_WWDT_RKEY << (uint32_t) XTHAL_WWDT_KEYSHIFT), XTHAL_WWDT_RESET_COUNTDOWN);
    XT_WER(((initial >> XTHAL_WWDT_INITIAL_SHIFT) & ((uint32_t) XTHAL_WWDT_INITIAL_MASK)) |
            ((uint32_t) XCHAL_WWDT_IKEY << (uint32_t) XTHAL_WWDT_KEYSHIFT), XTHAL_WWDT_INITIAL);
#if XCHAL_HAVE_XEA3
    xthal_interrupt_make_nmi(XCHAL_WWDT_INTERRUPT);
#endif
    return XTHAL_SUCCESS;
#else
    UNUSED(initial);
    UNUSED(bound);
    UNUSED(reset_countdown);
    return XTHAL_UNSUPPORTED;
#endif
}
/* 'kicks' the watchdog timer to keep the system alive. The kick must happen during
    the allowable window or else the system is reset. */
int32_t xthal_wwdt_kick(void)
{
#if XCHAL_HAVE_WWDT
    XT_WER((uint32_t) XCHAL_WWDT_KKEY  << (uint32_t) XTHAL_WWDT_KEYSHIFT, XTHAL_WWDT_KICK);
    if ((XT_RER(XTHAL_WWDT_STATUS) & XTHAL_WWDT_STATUS_INFO_ILL_KICK) != UINT32_C(0))
    {
        return XTHAL_INVALID;  
    }
    return XTHAL_SUCCESS;
#else
    return XTHAL_UNSUPPORTED;
#endif
}
/* Injects an error for testing purposes. It may only be called before xthal_wwdt_initialize. */
int32_t xthal_wwdt_inject_error(void)
{
#if XCHAL_HAVE_WWDT
    XT_WER((uint32_t) XCHAL_WWDT_EKEY << (uint32_t) XTHAL_WWDT_KEYSHIFT, XTHAL_WWDT_ERROR_INJECTION);
    return XTHAL_SUCCESS;
#else
    return XTHAL_UNSUPPORTED;
#endif

}
/* Clears an injected error. */
int32_t xthal_wwdt_clear_error(void)
{
#if XCHAL_HAVE_WWDT
    XT_WER(((uint32_t) XCHAL_WWDT_C1KEY << (uint32_t) XTHAL_WWDT_KEYSHIFT), XTHAL_WWDT_CLEAR);
    XT_WER(((uint32_t) XCHAL_WWDT_C2KEY << (uint32_t) XTHAL_WWDT_KEYSHIFT), XTHAL_WWDT_CLEAR);
    return XTHAL_SUCCESS;
#else
    return XTHAL_UNSUPPORTED;
#endif
}


int32_t xthal_set_opmode(uint32_t new_opmode, uint32_t* prev_opmode) // parasoft-suppress MISRA2012-RULE-8_13_a-4 "param is not const for all configurations"
{
#if (XCHAL_HW_MIN_VERSION >= XTENSA_HWVERSION_RI_2020_5) && !XCHAL_HAVE_XEA5 // todo
    if (prev_opmode != NULL)
    {
        *prev_opmode = ((uint32_t) XT_RSR_OPMODE());
    }
    XT_WSR_OPMODE((int32_t) new_opmode);
    return XTHAL_SUCCESS;
#else
    UNUSED(new_opmode);
    UNUSED(prev_opmode);
    return XTHAL_UNSUPPORTED;
#endif
}

int32_t xthal_get_opmode(uint32_t* opmode) // parasoft-suppress MISRA2012-RULE-8_13_a-4 "param is not const for all configurations"
{
#if (XCHAL_HW_MIN_VERSION >= XTENSA_HWVERSION_RI_2020_5) && !XCHAL_HAVE_XEA5 // todo
    if (opmode != NULL)
    {
        *opmode = ((uint32_t) XT_RSR_OPMODE());
        return XTHAL_SUCCESS;
    }
    else
    {
        return XTHAL_INVALID;
    }
#else
    UNUSED(opmode);
    return XTHAL_UNSUPPORTED;
#endif
}
