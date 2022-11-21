//
// consts.c - definitions for some constant values


// Copyright (c) 1999-2018 Cadence Design Systems, Inc.
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
#include <xtensa/core-macros.h>

#if   defined(__SPLIT__num_intlevels)

// the number of interrupt levels
const uint16_t Xthal_num_intlevels = XCHAL_NUM_INTLEVELS;

#elif defined(__SPLIT__num_interrupts)

// the number of interrupts
const uint16_t Xthal_num_interrupts = XCHAL_NUM_INTERRUPTS;

#elif defined(__SPLIT__excm_level)

#ifdef XCHAL_EXCM_LEVEL // todo
// the highest level of interrupts masked by PS.EXCM (if XEA2)
const uint16_t Xthal_excm_level = XCHAL_EXCM_LEVEL;
#else
const uint16_t Xthal_excm_level = 0;
#endif


#elif defined(__SPLIT__intlevel_mask)

#if XCHAL_HAVE_XEA2
// mask of interrupts at each intlevel
const uint32_t Xthal_intlevel_mask[XTHAL_MAX_INTLEVELS] = { 
    XCHAL_INTLEVEL_MASKS
};
#endif


#elif defined(__SPLIT__intlevel_andbelow_mask)

#if XCHAL_HAVE_XEA2
// mask for level 1 to N interrupts
const uint32_t Xthal_intlevel_andbelow_mask[XTHAL_MAX_INTLEVELS] = { 
    XCHAL_INTLEVEL_ANDBELOW_MASKS
};
#endif


#elif defined(__SPLIT__intlevel)

// level of each interrupt
// uint8_t just fits 0-255
#if XCHAL_HAVE_XEA2
const uint16_t Xthal_intlevel[XTHAL_MAX_INTERRUPTS] = {
    XCHAL_INT_LEVELS
};
#endif
#if XCHAL_HAVE_XEA3
const uint16_t Xthal_intlevel[XCHAL_NUM_INTERRUPTS] = {
    XCHAL_INT_LEVELS
};
#endif


#elif defined(__SPLIT__inttype)

// type of each interrupt
#if XCHAL_HAVE_XEA2
const uint8_t Xthal_inttype[XTHAL_MAX_INTERRUPTS] = {
    XCHAL_INT_TYPES
};
#endif
#if XCHAL_HAVE_XEA3
const uint8_t Xthal_inttype[XCHAL_NUM_INTERRUPTS] = {
    XCHAL_INT_TYPES
};
#endif


#elif defined(__SPLIT__inttype_mask)

#if XCHAL_HAVE_XEA2
const uint32_t Xthal_inttype_mask[XTHAL_MAX_INTTYPES] = {
    XCHAL_INTTYPE_MASKS
};
#endif


#elif defined(__SPLIT__timer_interrupt)

// interrupts assigned to each timer (CCOMPARE0 to CCOMPARE3), -1 if unassigned
const int32_t Xthal_timer_interrupt[XTHAL_MAX_TIMERS] = { 
    XCHAL_TIMER_INTERRUPTS
};

#endif
