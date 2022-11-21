/*
 * Copyright (c) 2004-2020 Cadence Design Systems Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <xtensa/hal.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifndef RSIZE_MAX
#define RSIZE_MAX 0x80000000
#endif

int32_t xthal_memcpy_s(void* dest, uint32_t destsz, const void* src,
        uint32_t count)
{
    // Check the following:
    // destination not NULL
    // source not NULL
    // dest buffer size <= RSIZE_MAX
    // - copy count <= dest buffer size
    // - buffers don't overlap
    // parasoft-begin-suppress MISRA2012-RULE-18_4 "pointer arith required."
    // parasoft-begin-suppress MISRA2012-RULE-11_5 "pointer conv required."
    if ((dest == NULL) || (src == NULL) || (destsz > RSIZE_MAX)
            || (count > destsz)
            || (((uint8_t *) dest < ((const uint8_t *) src + count))
                    && ((const uint8_t *) src < ((uint8_t *) dest + count))))
    {
        // If the dest buffer is valid then zero-fill
        // parasoft-end-suppress MISRA2012-RULE-18_4 "pointer arith required."
        // parasoft-end-suppress MISRA2012-RULE-11_5 "pointer conv required."
        if ((dest != NULL) && (destsz <= RSIZE_MAX))
        {
            (void) memset(dest, 0, destsz);
        }
        return XTHAL_INVALID;
    }
    (void) xthal_memcpy(dest, src, count);
    return 0;
}

