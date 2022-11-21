
// xos_hwprofile.c - interface to Xtensa hw profiling library.

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
// The functions in this file implement the interface to the hw based profiling
// library. The library is automatically linked in when the xt-xcc option -hwpg
// is specified. See user guides for more details.
//-----------------------------------------------------------------------------


#include "xos.h"
#include "xos_regaccess.h"


// The need to interface with the existing hw profile library forces these suppressions.
// parasoft-begin-suppress MISRAC2012-RULE_1_1-b-2  "Identifier length dictated by API"
// parasoft-begin-suppress MISRAC2012-RULE_11_1-a-2 "Type conversion dictated by API"

//-----------------------------------------------------------------------------
// These functions are called by the HW profiling library so they have to be
// exported. Declare them here to avoid compiler warnings (there's no header
// from the HW profiling library that we can include).
//-----------------------------------------------------------------------------
void
xt_profile_set_interrupt_handler(uint32_t interrupt, void * handler);
void
xt_profile_interrupt_on(uint32_t interrupt);
void
xt_profile_interrupt_off(uint32_t interrupt);
void
xt_profile_set_timer_handler(uint32_t timer, void * handler);
void
xt_profile_set_timer(uint32_t timer, uint32_t cycles);


//-----------------------------------------------------------------------------
// Register profiling interrupt handler.
//-----------------------------------------------------------------------------
void
xt_profile_set_interrupt_handler(uint32_t interrupt, void * handler)
{
    (void) xos_register_interrupt_handler(interrupt, (XosIntFunc *) handler, XOS_NULL);
}


//-----------------------------------------------------------------------------
// Enable profiling/timer interrupt.
//-----------------------------------------------------------------------------
void
xt_profile_interrupt_on(uint32_t interrupt)
{
    (void) xos_interrupt_enable(interrupt);
}


//-----------------------------------------------------------------------------
// Disable profiling/timer interrupt.
//-----------------------------------------------------------------------------
void
xt_profile_interrupt_off(uint32_t interrupt)
{
    (void) xos_interrupt_disable(interrupt);
}


//-----------------------------------------------------------------------------
// Register timer interrupt handler for profiling.
//-----------------------------------------------------------------------------
void
xt_profile_set_timer_handler(uint32_t timer, void * handler)
{
    extern void   xos_hwprof_cint_wrapper(void * arg); // parasoft-suppress MISRAC2012-RULE_8_6-a-2 "Defined in assembly code"

#if XCHAL_HAVE_XEA3

    (void) xos_register_interrupt_handler((uint32_t) Xthal_timer_interrupt[timer],
                                          xos_hwprof_cint_wrapper, handler);

    // Raise the interrupt priority level to the max possible
    // (one less than NMI priority). This will have no effect
    // if the priority levels are hardwired.
    xthal_interrupt_pri_set((uint32_t) Xthal_timer_interrupt[timer], (uint8_t)(XCHAL_NUM_INTLEVELS - 1));

#else

    // parasoft-begin-suppress MISRAC2012-RULE_8_6-a-2 "Defined in assembly code"
    extern void   xos_hwprof_int_wrapper(void);
    extern void * hwp_cb;
    // parasoft-end-suppress MISRAC2012-RULE_8_6-a-2

    uint32_t level = Xthal_intlevel[Xthal_timer_interrupt[timer]];

    if (level > (uint8_t) XCHAL_EXCM_LEVEL) {
        (void) xos_register_hp_interrupt_handler(level, (void *)xos_hwprof_int_wrapper);
    }
    else {
        (void) xos_register_interrupt_handler((uint32_t)Xthal_timer_interrupt[timer],
                                              xos_hwprof_cint_wrapper, handler);
    }

    hwp_cb = handler;

#endif
}


//-----------------------------------------------------------------------------
// Set timer expiration for timer-based profiling.
//-----------------------------------------------------------------------------
void
xt_profile_set_timer(uint32_t timer, uint32_t cycles)
{
#if XCHAL_HAVE_CCOUNT

    uint32_t ccount = xos_get_ccount();

    switch(timer)
    {
#if XCHAL_NUM_TIMERS >= 1
    case 0:
        xos_set_ccompare0(ccount + cycles);
        break;
#endif
#if XCHAL_NUM_TIMERS >= 2
    case 1:
        xos_set_ccompare1(ccount + cycles);
        break;
#endif
#if XCHAL_NUM_TIMERS >= 3
    case 2:
        xos_set_ccompare2(ccount + cycles);
        break;
#endif
    default:
        // MISRA checker wants comment.
        break;
    }

#else

    UNUSED(timer);
    UNUSED(cycles);

#endif
}

// parasoft-end-suppress MISRAC2012-RULE_1_1-b-2
// parasoft-end-suppress MISRAC2012-RULE_11_1-a-2

