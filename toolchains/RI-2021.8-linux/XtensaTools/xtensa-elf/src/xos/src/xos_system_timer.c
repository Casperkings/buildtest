
// xos_system_timer.c - System timer handling interface (overridable).

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

//-----------------------------------------------------------------------------
// The functions in this file implement the interface to the system timer.
// These functions can be replaced to interface to a custom timer, for example
// an external timer. They can also be replaced if you want to use the internal
// timers in a different way - perhaps use different logic to select which
// internal timer to use.
//-----------------------------------------------------------------------------


#include "xos.h"


//-----------------------------------------------------------------------------
// Timer number to use as the system timer.
//-----------------------------------------------------------------------------
static int32_t systimer_num = -1;


//-----------------------------------------------------------------------------
// Timer interrupt number.
//-----------------------------------------------------------------------------
static uint32_t systimer_intnum;


//-----------------------------------------------------------------------------
// Read the correct ccompare register depending on which one is being used for
// system timing.
//-----------------------------------------------------------------------------
__attribute__((always_inline))
static inline uint32_t
xos_get_ccompare(int32_t timer_num)
{
    switch (timer_num) {
    case 0:
        return xos_get_ccompare0();
#if XCHAL_NUM_TIMERS >= 2
    case 1:
        return xos_get_ccompare1();
#endif
#if XCHAL_NUM_TIMERS >= 3
    case 2:
        return xos_get_ccompare2();
#endif 
    default:
        xos_fatal_error(XOS_ERR_INTERNAL_ERROR, XOS_NULL);
        break;
    }

    // Should never get here
    return 0;
}


//-----------------------------------------------------------------------------
// Write the correct ccompare register depending on which one is being used
// for system timing.
//-----------------------------------------------------------------------------
static void
xos_set_ccompare(int32_t timer_num, uint32_t val)
{
    switch (timer_num) {
    case 0:
        xos_set_ccompare0(val);
        break;
#if XCHAL_NUM_TIMERS >= 2
    case 1:
        xos_set_ccompare1(val);
        break;
#endif
#if XCHAL_NUM_TIMERS >= 3
    case 2:
        xos_set_ccompare2(val);
        break;
#endif
    default:
        xos_fatal_error(XOS_ERR_INTERNAL_ERROR, XOS_NULL);
        break;
    }
}


//-----------------------------------------------------------------------------
// Timer interrupt handler. Just calls xos_tick_handler() to do all required
// processing. If you write your own interrupt handler, make sure to call
// xos_tick_handler() every time your handler is called.
//
// NOTE: Do not restart the timer in this function. The next tick will be
// scheduled from xos_tick_handler().
//-----------------------------------------------------------------------------
static void
timer_intr_handler(void * arg)
{
    // Suppress compiler warning about unused parameter.
    (void)(arg);

    xos_tick_handler();
}


//-----------------------------------------------------------------------------
// Read the system timer. Not required by the interface, just here in case we
// need it for debug.
//-----------------------------------------------------------------------------
__attribute__((weak)) uint32_t
xos_system_timer_get(void)
{
    return xos_get_ccompare(systimer_num);
}


//-----------------------------------------------------------------------------
// Write the system timer.
//-----------------------------------------------------------------------------
__attribute__((weak)) uint32_t
xos_system_timer_set(uint32_t cycles, bool from_now)
{
    // To prevent the (unlikely but not impossible) case where this operation
    // gets interrupted long enough for CCOUNT to increment past the CCOMPARE
    // value being set, we will disable interrupts around it.
    uint32_t level   = xos_disable_interrupts();
    uint32_t current = xos_get_ccount();
    uint32_t target;

    if (from_now) {
        target = cycles + current;
    }
    else {
        target = cycles + xos_get_ccompare(systimer_num);
    }

    xos_set_ccompare(systimer_num, target);
    xos_restore_interrupts(level);
    return target;
}


//-----------------------------------------------------------------------------
// Return true if the timer interrupt is pending.
//-----------------------------------------------------------------------------
__attribute__((weak)) bool
xos_system_timer_pending(void)
{
    return xos_interrupt_pending(systimer_intnum);
}


//-----------------------------------------------------------------------------
// Select the system timer. This is the default implementation. It will either
// use the provided timer ID, or if ID=-1, it will auto-select the timer from
// the available ones.
//-----------------------------------------------------------------------------
__attribute__((weak)) int32_t
xos_system_timer_select(int32_t timer_num, int32_t * psel)
{
#if (XCHAL_NUM_TIMERS == 0)

    UNUSED(timer_num);
    UNUSED(psel);
    return XOS_ERR_NO_TIMER;

#else

    int32_t timer_sel = timer_num;

    if (psel == XOS_NULL) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    // Auto-select timer number if so specified. We want to choose the one
    // that has the highest priority interrupt but is still <= EXCM_LEVEL.
    if (timer_sel == -1) {
        uint32_t l = 0;
        int32_t  i;

        for (i = 0; i < XCHAL_NUM_TIMERS; i++) {
            uint32_t level = Xthal_intlevel[Xthal_timer_interrupt[i]];

            if ((level <= (uint32_t) XCHAL_EXCM_LEVEL) && (level >= l)) {
                timer_sel = i;
                l         = level;
            }
        }

        if (timer_sel == -1) {
            return XOS_ERR_NO_TIMER;
        }
    }

    // Make sure we have a valid timer
    if ((timer_sel < 0) || (timer_sel >= XCHAL_NUM_TIMERS)) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    // Validate the timer interrupt level
    systimer_intnum = (uint32_t) Xthal_timer_interrupt[timer_sel];
    if (Xthal_intlevel[systimer_intnum] > (uint32_t) XCHAL_EXCM_LEVEL) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    systimer_num = timer_sel;
    *psel = timer_sel;

    return XOS_OK;

#endif
}


//-----------------------------------------------------------------------------
// Init and start the system timer.
//-----------------------------------------------------------------------------
__attribute__((weak)) uint32_t
xos_system_timer_init(uint32_t cycles)
{
    uint32_t ret;

    (void) xos_register_interrupt_handler(systimer_intnum, &timer_intr_handler, XOS_NULL);
    ret = xos_system_timer_set(cycles, true);
    (void) xos_interrupt_enable(systimer_intnum);
    return ret;
}

