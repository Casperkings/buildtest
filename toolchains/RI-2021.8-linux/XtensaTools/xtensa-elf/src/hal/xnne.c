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

#include <xtensa/xnne.h>
#include <xtensa/hal.h>
#include <xtensa/config/core.h>

#if XCHAL_HAVE_XNNE
#include <xtensa/tie/xt_xcxnne.h>

/* Internal function -- no argument checking. */
inline static void xnne_tieq_wr(uint8_t sblk, uint32_t offset, uint32_t value)
{
    XNNE_WR(XNNE_TIEQ_SBLK(sblk) + offset, value);
}

/* Internal function -- no argument checking. */
inline static uint32_t xnne_tieq_rd(uint8_t sblk, uint32_t offset)
{
    return XNNE_RD(XNNE_TIEQ_SBLK(sblk) + offset);
}
#endif

#ifndef XNNE_BASE
#define XNNE_BASE 0
#define XNNE_SLAVE_SIZE 0
#endif

/*
 * This function loads program code and data onto the VPU and starts it.
 * This function should only be called while the VPU is still in a reset state.
 *
 *      sblk            specifies which VPU is loaded
 *      code            points to code to be loaded on VPU instruction ram
 *                      (there are no alignment requirements)
 *      code_size       length of code in bytes
 *      data            points to data to be loaded on VPU data ram
 *                      (there are no alignment requirements)
 *      data_size       length of data in bytes
 *
 *      Returns:
 *      XTHAL_SUCCESS           VPU loaded and running
 *      XTHAL_INVALID           Arguments are invalid, VPU not changed
 *      XTHAL_UNSUPPORTED       XNNE not configured
 *
 */
int32_t
xthal_xnne_load_with_base (uint8_t sblk, const void* code, uint32_t code_size, const void* data,
		 uint32_t data_size, uint32_t xnne_base, uint32_t xnne_slave_size)
{
#if XCHAL_HAVE_XNNE

  if ((sblk >= (uint8_t) XCHAL_XNNE_NUM_SBLKS) || ((code_size != 0U) && (code == NULL)) ||
      ((data_size != 0U) && (data == NULL)) || (code_size > XNNE_IRAM_MAX) ||
      (data_size > XNNE_DRAM_MAX))
  {
    return XTHAL_INVALID;
  }
  /* Make XNNE Slave Address region device (non-cacheable).*/
  int32_t rv = xthal_mpu_set_region_attribute((void*) xnne_base, xnne_slave_size, // parasoft-suppress MISRA2012-RULE-11_6-2 "Conversion necessary and checked"
                                 XTHAL_MPU_USE_EXISTING_ACCESS_RIGHTS,
                                 XCHAL_CA_BYPASS, 0);
  if (rv != XTHAL_SUCCESS)
  {
      return rv;
  }
  rv = xthal_xnne_enable_sblk(sblk); // Enable sblk
  if (rv != XTHAL_SUCCESS)
  {
      return rv;
  }
  // VPUCtrl: De-assert reset and leave RunStall asserted
  XNNE_WR(XNNE_TIEQ_VPU_CTRL(sblk), XNNE_VPU_CTRL_RUNSTALL);

  // Load VPU IRAM
  (void) xthal_memcpy((uint8_t*) XNNE_MP_SBLK_IRAM_BASE(xnne_base, sblk), code, code_size); // parasoft-suppress MISRA2012-RULE-11_4-4 "Conversion necessary and checked"
  // Load VPU DRAM
  (void) xthal_memcpy((uint8_t*) XNNE_MP_SBLK_DRAM_BASE(xnne_base, sblk), data, data_size); // parasoft-suppress MISRA2012-RULE-11_4-4 "Conversion necessary and checked"

  // VPUCtrl: De-assert RunStall
  XNNE_WR(XNNE_TIEQ_VPU_CTRL(sblk), 0x0);

  return XTHAL_SUCCESS;
#else
  UNUSED (sblk);
  UNUSED (code);
  UNUSED (data);
  UNUSED (code_size);
  UNUSED (data_size);
  UNUSED (xnne_base);
  UNUSED (xnne_slave_size);
  return XTHAL_UNSUPPORTED;
#endif
}

/*
 * This function loads program code and data onto the VPU and starts it.
 * This function should only be called while the VPU is still in a reset state.
 *
 *      sblk            specifies which VPU is loaded
 *      code            points to code to be loaded on VPU instruction ram
 *                      (there are no alignment requirements)
 *      code_size       length of code in bytes
 *      data            points to data to be loaded on VPU data ram
 *                      (there are no alignment requirements)
 *      data_size       length of data in bytes
 *
 *      Returns:
 *      XTHAL_SUCCESS           VPU loaded and running
 *      XTHAL_INVALID           Arguments are invalid, VPU not changed
 *      XTHAL_UNSUPPORTED       XNNE not configured
 *
 */
int32_t
xthal_xnne_load (uint8_t sblk, const void* code, uint32_t code_size, const void* data,
                 uint32_t data_size)
{
    return xthal_xnne_load_with_base(sblk, code, code_size, data, data_size,
            XNNE_BASE, XNNE_SLAVE_SIZE);
}

/*
 * This function 'resets' the VPU by asserting both VPU reset and runstall.
 *
 *      sblk            specifies which VPU is loaded
 *
 *      Returns:
 *      XTHAL_SUCCESS           VPU loaded and running
 *      XTHAL_INVALID           Arguments are invalid, VPU not changed
 *      XTHAL_UNSUPPORTED       XNNE not configured
 *
 */
extern int32_t xthal_xnne_reset(uint8_t sblk)
{
#if XCHAL_HAVE_XNNE

    if ((sblk >= (uint8_t) XCHAL_XNNE_NUM_SBLKS))
    {
        return XTHAL_INVALID;
    }

    // VPUCtrl: Assert Reset + RunStall
    XNNE_WR(XNNE_TIEQ_VPU_CTRL(sblk),
    XNNE_VPU_CTRL_RESET | XNNE_VPU_CTRL_RUNSTALL);
    return XTHAL_SUCCESS;
#else
    UNUSED(sblk);
    return XTHAL_UNSUPPORTED;
#endif
}

/* parasoft-begin-suppress MISRA2012-DIR-4_4 "Not code". */
/*
 * This function enables the SBLK. Disabling the SBLK reduces
 * power consumption.  The SBLKs are disabled on reset (by the reset vector),
 * and then enabled by an enable or load call.
 *
 *  example:
 *  xthal_xnne_enable_sblk(0); - enables sblk 0
 *
 *      sblk            specifies which sblk is enabled
 */
/* parasoft-end-suppress MISRA2012-DIR-4_4 "Not code". */
extern int32_t xthal_xnne_enable_sblk(uint8_t sblk)
{
#if XCHAL_HAVE_XNNE
    if (sblk >= (uint8_t) XCHAL_XNNE_NUM_SBLKS)
    {
        return XTHAL_INVALID;
    }
    xnne_tieq_wr(sblk, XNNE_SBLK_DISABLE, 0);
    return XTHAL_SUCCESS;
#else
    UNUSED(sblk);
    return XTHAL_UNSUPPORTED;
#endif
}

/* parasoft-begin-suppress MISRA2012-DIR-4_4 "Not code". */
/*
 * This function disables the SBLK. Disabling the SBLK reduces
 * power consumption.  The SBLKs are disabled on reset (by the reset vector),
 * and can later be disabled.
 *
 *  example:
 *  xthal_xnne_disable_sblk(0); - disables sblk 0
 *
 *      sblk            specifies which sblk is disabled
 */
/* parasoft-end-suppress MISRA2012-DIR-4_4 "Not code". */
extern int32_t xthal_xnne_disable_sblk(uint8_t sblk)
{
#if XCHAL_HAVE_XNNE
    if (sblk >= (uint8_t) XCHAL_XNNE_NUM_SBLKS)
    {
        return XTHAL_INVALID;
    }
    xnne_tieq_wr(sblk, XNNE_SBLK_DISABLE, 1);
    return XTHAL_SUCCESS;
#else
    UNUSED(sblk);
    return XTHAL_UNSUPPORTED;
#endif
}

/* parasoft-begin-suppress MISRA2012-DIR-4_4 "Not code". */
/* This function binds the specified counter to the specified event ... it
 * does not enable or clear the counter.
 *
 *  example:
 *  xthal_xnne_init_counter(0, 0, XNNE_EVENT_Cycles);
 *
 *      sblk            specifies which VPU is used
 *      counter         counter number ... 0..3 (Not PERF_COUNTERX)
 *      event           xnne event (XNNE_EVENT_*) being counted
 *
 *  returns either XTHAL_SUCCESS or XTHAL_INVALID
*/
/* parasoft-end-suppress MISRA2012-DIR-4_4 "Not code". */
int32_t xthal_xnne_init_counter(uint8_t sblk, uint8_t counter, uint8_t event)
{
#if XCHAL_HAVE_XNNE
    if ((sblk >= (uint8_t) XCHAL_XNNE_NUM_SBLKS) || (event > ((uint8_t) XNNE_EVENT_MAX))
            || (counter >= XNNE_NUM_PERF_COUNTERS))
    {
        return XTHAL_INVALID;
    }
    xnne_tieq_wr(sblk, XNNE_PERF_EVENT_SELN((uint32_t) counter), (uint32_t) event);
    return XTHAL_SUCCESS;
#else
    UNUSED(sblk);
    UNUSED(counter);
    UNUSED(event);
    return XTHAL_UNSUPPORTED;
#endif
}


/* parasoft-begin-suppress MISRA2012-DIR-4_4 "Not code". */
/* This function clears and enables the specified counter(s). If more than one
 * counter is specified, then the all specified counters are atomically
 * enabled.
 *
 *  example (clears and enables counters 0 and 1):
 *  xthal_xnne_start_counters(0, XNNE_PERF_COUNTER0 | XNNE_PERF_COUNTER1);
 *
 *      sblk            specifies which VPU is used
 *      counter         bit-wise or of subset of
 *                      XNNE_PERF_COUNTER0 ... XNNE_PERFCOUNTER_3
 *
 *  returns either XTHAL_SUCCESS or XTHAL_INVALID
*/
/* parasoft-end-suppress MISRA2012-DIR-4_4 "Not code". */
int32_t
xthal_xnne_start_counters(uint8_t sblk, uint8_t counters)
{
#if XCHAL_HAVE_XNNE
    if ((sblk >= (uint8_t) XCHAL_XNNE_NUM_SBLKS) ||
            (counters >= (UINT32_C(1) << XNNE_NUM_PERF_COUNTERS)))
    {
        return XTHAL_INVALID;
    }
    xnne_tieq_wr(sblk, XNNE_PERF_COUNTER_CLEAR, counters);
    xnne_tieq_wr(sblk, XNNE_PERF_COUNTER_ENABLE, counters);
    return XTHAL_SUCCESS;
#else
    UNUSED(sblk);
    UNUSED(counters);
    return XTHAL_UNSUPPORTED;
#endif
}


/* Takes a snapshot of all performance counters.  This must be done before
 * reading them.
 *
 *      sblk            specifies which VPU is used
 *
 *  returns either XTHAL_SUCCESS or XTHAL_INVALID
*/
int32_t
xthal_xnne_counter_snapshot(uint8_t sblk)
{
#if XCHAL_HAVE_XNNE
    if ((sblk >= (uint8_t) XCHAL_XNNE_NUM_SBLKS))
    {
        return XTHAL_INVALID;
    }
    xnne_tieq_wr(sblk, XNNE_PERF_COUNTER_SNAPSHOT, 1);
    return XTHAL_SUCCESS;
#else
    UNUSED(sblk);
    return XTHAL_UNSUPPORTED;
#endif
}


/* parasoft-begin-suppress MISRA2012-DIR-4_4 "Not code". */
/* Reads the specified counter and puts the result in the location pointed to
 * by value.
 *
 *  example: xthal_xnne_read_counter(0, XNNE_PERF_COUNTER1, &value);
 *
 *      sblk            specifies which VPU is used
 *      counter         counter number ... 0..3 (Not PERF_COUNTERX)
 *      value           pointer to location where value is placed
 *
 *  returns either XTHAL_SUCCESS or XTHAL_INVALID
*/
/* parasoft-end-suppress MISRA2012-DIR-4_4 "Not code". */
int32_t
/* parasoft-begin-suppress MISRA2012-RULE-8_13_a "parameter 'value' is not const and should not be declared as such" */
xthal_xnne_read_counter(uint8_t sblk, uint8_t counter, uint32_t* value)
{
#if XCHAL_HAVE_XNNE
    if ((sblk >= (uint8_t) XCHAL_XNNE_NUM_SBLKS) || (counter >= XNNE_NUM_PERF_COUNTERS) ||
            (value == NULL))
    {
        return XTHAL_INVALID;
    }
    *value = xnne_tieq_rd(sblk, XNNE_PERF_COUNTER_ADDR((uint32_t) counter));
    return XTHAL_SUCCESS;
#else
    UNUSED(sblk);
    UNUSED(counter);
    UNUSED(value);
    return XTHAL_UNSUPPORTED;
#endif
}
/* parasoft-end-suppress MISRA2012-RULE-8_13_a "parameter 'value' is not const and should not be declared as such" */


#if XCHAL_HAVE_XNNE
/* force the constants in xnne_asm.S to get the debug constants.
 * needed to generate constants from an assembly file.
 */
__asm__(".set dummy_, xnne_base_");
#endif


