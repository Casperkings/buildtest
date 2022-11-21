#include <xtensa/hal.h>
#include <xtensa/config/core.h>
#include <xtensa/tie/xt_core.h>

#define APB_BASE_ADDR_MASK UINT32_C(0xfffff000)

/* parasoft-begin-suppress MISRA2012-DIR-4_4 "Not code". */
/*
 * xthal_program_apb() programs the APB master register if configured:
 *
 * programs the APB region. Before calling this function any cached data
 * in the APB region should be invalidated.
 *
 * base_addr is the base address of the APB region. The base_addr must be
 * aligned to the power of 2 size of the APB region. Only the upper
 * 32 - log2_size bits of base_addr are used.
 *
 * log2_size is the log2 of the size in bytes of the APB region.
 * The APB region must be a power of 2 in length, and between 16k and 16M
 * (inclusive) implying that the valid values of the log2_size parameter are 14
 * to 24.
 *
 * Typical usage:
 *      xthal_program_apb((void*) 0xe6000000, XTHAL_APB_SZ_16M);
 *
 * Returns:
 *      XTHAL_SUCCESS on success
 *      XTHAL_INVALID if arguments are out of range
 *      XTHAL_UNSUPPORTED if APB0CFG register is not configured
 */
/* parasoft-end-suppress MISRA2012-DIR-4_4 "Not code". */
int32_t xthal_program_apb(const void* base_addr, uint8_t log2_size)
{
#if XCHAL_HAVE_PROGRAMMABLE_APB && !XCHAL_HAVE_XEA5 //todo
    // We mask to retain only the MSB's that are needed to specify
    // the base address for a given size.  If we mask to APB_BASE_ADDR_MASK,
    // then base might not be properly aligned and the could be
    // false failures when comparing the value read back from APB0CFG
    uint32_t base;
    if ((log2_size > ((uint8_t) XTHAL_APB_SZ_16M)) || (log2_size < ((uint8_t) XTHAL_APB_SZ_16K)))
    {
        return XTHAL_INVALID;
    }
    base = ((uint32_t) base_addr) & (~((UINT32_C(1)<<log2_size) - UINT32_C(1)));
    XT_MEMW();
    XT_WSR_APB0CFG(base | log2_size);
    if ((((uint32_t) XT_RSR_APB0CFG()) & APB_BASE_ADDR_MASK) != base)
    {
        return XTHAL_INVALID;
    }
    return xthal_mpu_set_region_attribute((void*)base, (UINT32_C(1)<<log2_size),
            XTHAL_MPU_USE_EXISTING_ACCESS_RIGHTS, (XTHAL_MEM_SYSTEM_SHAREABLE | XTHAL_MEM_DEVICE), 0);
#else
    UNUSED(base_addr);
    UNUSED(log2_size);
    return XTHAL_UNSUPPORTED;
#endif
}

/*
 * This function puts the current apb base address into the location pointed to
 * by apb_addr.
 *
 * It returns:
 *      XTHAL_INVALID           if apb_addr is NULL
 *      XTHAL_UNSUPPORTED       APB0CFG register is not configured
 *      XTHAL_SUCCESS           on success
 */
/* parasoft-begin-suppress MISRA2012-RULE-8_13_a "parameter 'apb_addr' is not const and should not be declared as such" */
int32_t xthal_get_apb_address(uint32_t* apb_addr)
{
#if XCHAL_HAVE_PROGRAMMABLE_APB && !XCHAL_HAVE_XEA5 // todo
    uint32_t v = XT_RSR_APB0CFG();
    v = v & APB_BASE_ADDR_MASK;
    if (apb_addr == UINT32_C(0))
    {
        return XTHAL_INVALID;
    }
    *apb_addr = v;
    return XTHAL_SUCCESS;
#else
    UNUSED(apb_addr);
    return XTHAL_UNSUPPORTED;
#endif
}
/* parasoft-end-suppress MISRA2012-RULE-8_13_a "parameter 'apb_addr' is not const and should not be declared as such" */
