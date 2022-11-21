
// xos_regaccess.h - Access routines for various processor special registers.

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

#ifndef XOS_REGACCESS_H
#define XOS_REGACCESS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xos_types.h"

#include <xtensa/config/core.h>


// parasoft-begin-suppress MISRAC2012-DIR_4_2-a-4 "Assembly code is clear and concise"

//-----------------------------------------------------------------------------
// Read CCOUNT register.
//-----------------------------------------------------------------------------
__attribute__((always_inline))
static inline uint32_t
xos_get_ccount(void)
{
#if XCHAL_HAVE_CCOUNT

    uint32_t ccount;

    __asm__ __volatile__ ( "rsr.ccount  %0" : "=a" (ccount) );
    return ccount;

#else

    return 0U;

#endif
}


//-----------------------------------------------------------------------------
// Read CCOMPARE0
//-----------------------------------------------------------------------------
__attribute__((always_inline))
static inline uint32_t
xos_get_ccompare0(void)
{
#if (XCHAL_NUM_TIMERS > 0)

    uint32_t ccompare0;

    __asm__ __volatile__ ( "rsr.ccompare0  %0" : "=a" (ccompare0));
    return ccompare0;

#else

    return 0U;

#endif
}


//-----------------------------------------------------------------------------
// Read CCOMPARE1
//-----------------------------------------------------------------------------
__attribute__((always_inline))
static inline uint32_t
xos_get_ccompare1(void)
{
#if (XCHAL_NUM_TIMERS > 1)

    uint32_t ccompare1;

    __asm__ __volatile__ ( "rsr.ccompare1  %0" : "=a" (ccompare1));
    return ccompare1;

#else

    return 0U;

#endif
}


//-----------------------------------------------------------------------------
// Read CCOMPARE2
//-----------------------------------------------------------------------------
__attribute__((always_inline))
static inline uint32_t
xos_get_ccompare2(void)
{
#if (XCHAL_NUM_TIMERS > 2)

    uint32_t ccompare2;

    __asm__ __volatile__ ( "rsr.ccompare2  %0" : "=a" (ccompare2));
    return ccompare2;

#else

    return 0U;

#endif
}


//-----------------------------------------------------------------------------
// Write CCOMPARE0
//-----------------------------------------------------------------------------
__attribute__((always_inline))
static inline void
xos_set_ccompare0(uint32_t val)
{
#if (XCHAL_NUM_TIMERS > 0)

    __asm__ __volatile__ (
      "wsr.ccompare0  %0\n"
      "rsync"
      :
      : "a" (val)
    );

#else

    // Suppress compiler warning.
    (void)(val);

#endif
}


//-----------------------------------------------------------------------------
// Write CCOMPARE1
//-----------------------------------------------------------------------------
__attribute__((always_inline))
static inline void
xos_set_ccompare1(uint32_t val)
{
#if (XCHAL_NUM_TIMERS > 1)

    __asm__ __volatile__ (
      "wsr.ccompare1  %0\n"
      "rsync"
      :
      : "a" (val)
    );

#else

    // Suppress compiler warning.
    (void)(val);

#endif
}


//-----------------------------------------------------------------------------
// Write CCOMPARE2
//-----------------------------------------------------------------------------
__attribute__((always_inline))
static inline void
xos_set_ccompare2(uint32_t val)
{
#if (XCHAL_NUM_TIMERS > 2)

    __asm__ __volatile__ (
      "wsr.ccompare2  %0\n"
      "rsync"
      :
      : "a" (val)
    );

#else

    // Suppress compiler warning.
    (void)(val);

#endif
}

// parasoft-end-suppress MISRAC2012-DIR_4_2-a-4


#ifdef __cplusplus
}
#endif

#endif	// XOS_REGACCESS_H

