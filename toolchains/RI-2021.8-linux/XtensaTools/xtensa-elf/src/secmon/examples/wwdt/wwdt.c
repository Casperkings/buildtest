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

// wwdt.c -- Windowed watchdog timer functions


#include <xtensa/secmon.h>

#include "wwdt.h"


volatile int32_t timer_expired;		/* Loop protection timeout flag */


/******************************************************************************
 * Helper functions
 *****************************************************************************/

static void nmi_trigger_init(void)
{
    int32_t m0 = 0;
    __asm__ __volatile__ ("wsr.misc0 %0" :: "a"(m0));
}


static int32_t nmi_trigger_check(void)
{
    int32_t m0;
    __asm__ __volatile__ ("rsr.misc0 %0" : "=a"(m0));
    return m0;
}


static void timer_handler(void *arg)
{
    UNUSED(arg);
    timer_expired = 1;
    XT_WSR_CCOMPARE0(WWDT_FAIL_TIME + XT_RSR_CCOUNT());
}


static int32_t timer_init(uint32_t cnt)
{
    int32_t timer_num = 0;
    int32_t int_num = Xthal_timer_interrupt[timer_num];
    int32_t rv;

    timer_expired = 0;

    rv = xtsm_set_interrupt_handler(int_num, (xtsm_handler_t *)timer_handler, (void *)timer_num);
    if (rv) {
        printf("ERROR: xtsm_set_interrupt_handler(%d, %p, %p): %d\n",
                int_num, (void *)timer_handler, (void *)timer_num, rv);
        return -1;
    }
    xtos_interrupt_enable(int_num);

    XT_WSR_CCOMPARE0(cnt + XT_RSR_CCOUNT());
    return 0;
}


static int32_t wwdt_safe_kick(uint32_t bound)
{
    int32_t rv;

    if (XT_RER(XTHAL_WWDT_WD_COUNTDOWN) < bound) {
        rv = xthal_wwdt_kick();
        return rv;
    }
    return WWDT_TEST_OUTOFWINDOW;
}


static uint32_t wwdt_wait_for_fault(void)
{
    if (timer_init(WWDT_FAIL_TIME)) {
        return WWDT_TEST_INITFAIL;
    }
    while (!nmi_trigger_check() && !timer_expired) {
    }

    return timer_expired? WWDT_STATUS_TIMEOUT : WWDT_READ_STATUS();
}


/******************************************************************************
 * Test functions
 *****************************************************************************/

int32_t test_xthal_wwdt_initialize(uint32_t initial, uint32_t bound, uint32_t reset)
{
    int32_t rv;
    uint32_t status;

    nmi_trigger_init();
    rv = xthal_wwdt_initialize(initial, bound, reset);
    if (rv != XTHAL_SUCCESS) {
        return WWDT_TEST_INITFAIL;
    }

    status = wwdt_wait_for_fault();
    if (status == WWDT_STATUS_NORMAL_EXPIRY) {
        return WWDT_TEST_OK;
    }
    return WWDT_TEST_TIMEOUT;
}


int32_t test_xthal_wwdt_kick(uint32_t initial, uint32_t bound, uint32_t reset)
{
    int32_t rv;
    uint32_t status;
    int32_t kick_count = 10;

    nmi_trigger_init();
    rv = xthal_wwdt_initialize(initial, bound, reset);
    if (rv != XTHAL_SUCCESS) {
        return WWDT_TEST_INITFAIL;
    }

    while (kick_count) {
        wwdt_safe_kick(bound);
        kick_count--;
    }

    status = wwdt_wait_for_fault();
    if (status == WWDT_STATUS_NORMAL_EXPIRY) {
        return WWDT_TEST_OK;
    }
    return WWDT_TEST_TIMEOUT;
}


int32_t test_xthal_wwdt_inject_error(uint32_t initial, uint32_t bound, uint32_t reset)
{
    int32_t rv;
    uint32_t status;

    UNUSED(initial);
    UNUSED(bound);
    UNUSED(reset);

    nmi_trigger_init();
    rv = xthal_wwdt_inject_error();
    if (rv != XTHAL_SUCCESS) {
        return WWDT_TEST_INJECTFAIL;
    }

    status = wwdt_wait_for_fault();
    if (status == WWDT_STATUS_INJECTED_ERROR) {
        return WWDT_TEST_OK;
    }

    return WWDT_TEST_TIMEOUT;
}


int32_t test_xthal_wwdt_clear_error(uint32_t initial, uint32_t bound, uint32_t reset)
{
    int32_t rv;
    uint32_t status;

    nmi_trigger_init();
    rv = test_xthal_wwdt_inject_error(initial, bound, reset);
    if (rv != XTHAL_SUCCESS) {
        return WWDT_TEST_INJECTFAIL;
    }

    xthal_wwdt_clear_error();
    status = WWDT_READ_STATUS();
    if (status != 0) {
        return WWDT_TEST_INJECTFAIL;
    }

    rv = test_xthal_wwdt_initialize(initial, bound, reset);

    return rv;
}


