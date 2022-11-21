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
#include <xtensa/config/core.h>
#include <xtensa/core-macros.h>
#include <xtensa/hal.h>
#include <string.h>
#include <stdlib.h>

/*
 * Copies the MPU background map into 'entries' which must point
 * to available memory of at least
 * sizeof(xthal_MPU_entry) * XCHAL_MPU_BACKGROUND_ENTRIES.
 *
 * This function returns XTHAL_SUCCESS.
 * XTHAL_INVALID, or
 * XTHAL_UNSUPPORTED.
 */
int32_t xthal_read_background_map(xthal_MPU_entry* bg_map)
{
#if XCHAL_HAVE_MPU
    if (bg_map == NULL)
    {
        return XTHAL_INVALID;
    }
    (void) memcpy(bg_map, Xthal_mpu_bgmap,
	   sizeof(xthal_MPU_entry) * (uint32_t) XCHAL_MPU_BACKGROUND_ENTRIES);
    return XTHAL_SUCCESS;
#else
    UNUSED(bg_map);
    return XTHAL_UNSUPPORTED;
#endif
}

