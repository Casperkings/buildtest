
// xos_timeofday.c - implements get/settimeofday() functions.

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
//  This file implements the gettimeofday() and settimeofday() functions that
//  support getting and setting the system time. Time is maintained globally,
//  i.e. there is no concept of thread-specific time.
//  NOTE: Timekeeping will only work if timer services have been enabled by
//  calling xos_start_system_timer().
//-----------------------------------------------------------------------------


#include <sys/time.h>
#include <sys/reent.h>

#define XOS_INCLUDE_INTERNAL
#include "xos.h"


// parasoft-begin-suppress MISRAC2012-RULE_21_2-a-2 "C lib support functions need to use reserved names"

//-----------------------------------------------------------------------------
//  Declarations to avoid compiler warnings.
//-----------------------------------------------------------------------------
#if XSHAL_CLIB == XTHAL_CLIB_UCLIBC
struct timezone
{
    int32_t tz_minuteswest;     // minutes west of Greenwich
    int32_t tz_dsttime;         // type of DST correction
};
#endif
int32_t
_gettimeofday_r(struct _reent *reent, struct timeval *tv, struct timezone *tz);
int32_t
settimeofday(const struct timeval *tv , const struct timezone *tz);

//-----------------------------------------------------------------------------
//  Initial time in seconds (since the Epoch - (00:00:00 UTC, January 1, 1970).
//  Starts off as zero, can be set via settimeofday().
//  If this is zero, we handle it specially, see below.
//-----------------------------------------------------------------------------
static uint64_t g_time_in_secs;

//-----------------------------------------------------------------------------
//  Initial system cycle count. Starts off as zero, and is updated anytime
//  settimeofday() is called. Used to track how much time has passed since
//  the time was set.
//-----------------------------------------------------------------------------
static uint64_t g_start_system_cycles;

//-----------------------------------------------------------------------------
//  Time zone information. Set by settimeofday() and returned by gettimeofday()
//  but not used internally. Starts off as zero, which is Greenwich time.
//-----------------------------------------------------------------------------
static int32_t  g_tz_minuteswest;


//-----------------------------------------------------------------------------
//  This function is meant to be called only from the C runtime library code.
//  Application code calls gettimeofday() which in turn calls this function.
//-----------------------------------------------------------------------------
int32_t
_gettimeofday_r(struct _reent *reent, struct timeval *tv, struct timezone *tz)
{
    if (tv != XOS_NULL) {
        uint64_t delta = xos_get_system_cycles() - g_start_system_cycles;
        uint64_t usecs = xos_cycles_to_usecs(delta);
        uint64_t secs  = g_time_in_secs;

        // If the time has not been set, we start counting from Jan 1, 2014
        // instead of Jan 1, 1970.
        if (secs == 0ULL) {
            secs = 1388534400ULL;
        }

        tv->tv_sec  = (time_t) (secs + (usecs / 1000000UL)); // parasoft-suppress MISRAC2012-RULE_10_8-a-2 "Type conv OK"
        tv->tv_usec = (suseconds_t) (usecs % 1000000UL);     // parasoft-suppress MISRAC2012-RULE_10_8-a-2 "Type conv OK"
    }

    // tz_dsttime always returned as zero.
    if (tz != XOS_NULL) {
        tz->tz_minuteswest = g_tz_minuteswest;
        tz->tz_dsttime = 0;
    }

    if (reent != XOS_NULL) {
        reent->_errno = 0;
    }

    return 0;
}


//-----------------------------------------------------------------------------
//  This function is not in POSIX and not in the provided C runtime libraries.
//  It is provided here to allow applications to set the current time (e.g.
//  by reading it from some external source at startup) and then be able to
//  keep track of elapsed time.
//-----------------------------------------------------------------------------
int32_t
settimeofday(const struct timeval *tv , const struct timezone *tz)
{
    if (tv != XOS_NULL) {
        g_time_in_secs = (uint64_t) tv->tv_sec;
        g_start_system_cycles = xos_get_system_cycles();
    }

    // Always ignore tz_dsttime. Doesn't work on Linux, doesn't work here.
    if (tz != XOS_NULL) {
        g_tz_minuteswest = tz->tz_minuteswest;
    }

    return 0;
}

// parasoft-end-suppress MISRAC2012-RULE_21_2-a-2

