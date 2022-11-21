// Copyright (c) 2003-2021 Cadence Design Systems, Inc.
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
// Filename:    secmon-common.h
//
// Contents:    Secure monitor definitions and types shared by secure monitor
//              and nonsecure library.
//-----------------------------------------------------------------------------
#ifndef __SECMON_COMMON_H
#define __SECMON_COMMON_H

#include <xtensa/config/secure.h>


//-----------------------------------------------------------------------------
//  Secure memory region type and table
//-----------------------------------------------------------------------------
typedef struct {
    uint32_t first;
    uint32_t size;
} xtsm_address_range_t;


//-----------------------------------------------------------------------------
//  Determine whether address range [a..b] overlaps with range [c..d].
//-----------------------------------------------------------------------------
static inline int32_t xtsm_overlap(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
    return (((a >= c) && (a <= d)) ||  ((b >= c && b <= d)) || ((a <= c) && (b >= d)));
}


//-----------------------------------------------------------------------------
//  Determine whether the specified address range lies either partially or 
//  fully within a secure memory region.
//-----------------------------------------------------------------------------
static inline int32_t xtsm_secure_addr_overlap(uint32_t first, uint32_t last)
{
    static const xtsm_address_range_t secure_regions[] = XCHAL_SECURE_MEM_LIST;
    static const uint32_t num_secure_regions = (sizeof(secure_regions) / 
            sizeof(xtsm_address_range_t));

    uint32_t i;
    for (i = 0; i < num_secure_regions; i++) {
        if (xtsm_overlap(first, last, secure_regions[i].first, 
                    secure_regions[i].first + secure_regions[i].size - 1)) {
            return 1;
        }
    }
    return 0;
}

#endif /* __SECMON_COMMON_H */

