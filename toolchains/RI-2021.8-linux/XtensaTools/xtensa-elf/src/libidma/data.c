/*
 * Copyright (c) 2018-2020 Cadence Design Systems. ALL RIGHTS RESERVED.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#define IDMA_BUILD

#include <assert.h>
#include <stdint.h>

#include "idma_internal.h"


#define static_assert _Static_assert


idma_cntrl_t g_idma_cntrl[XCHAL_IDMA_NUM_CHANNELS]    IDMA_DRAM __attribute__ ((weak));
idma_buf_t * g_idma_buf_ptr[XCHAL_IDMA_NUM_CHANNELS]  IDMA_DRAM __attribute__ ((weak));

// Only works in C11 or later.
#if defined (__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
static_assert(sizeof(idma_cntrl_t) == (uint32_t)IDMA_CONTROL_STRUCT_SIZE_, "Opaque control struct size doesn't match the real size");
#endif

