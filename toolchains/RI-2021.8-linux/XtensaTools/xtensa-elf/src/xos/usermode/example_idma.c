
// Copyright (c) 2021 Cadence Design Systems, Inc.
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


// Simple dynamic library to demonstrate user-mode iDMA. 

// DO NOT try dynamic memory allocation in this library.
// There is no heap provided for position-independent libraries.


#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <xt_mld_loader.h>
#include <xtensa/xtutil.h>

#define IDMA_USE_MULTICHANNEL
#include <xtensa/idma.h>

// Specify section attributes for code and data

XTMLD_DATA_MEM_REQ(xtmld_load_normal, xtmld_load_normal, xtmld_load_normal, xtmld_load_normal);
XTMLD_CODE_MEM_REQ(xtmld_load_normal, xtmld_load_normal, xtmld_load_normal, xtmld_load_normal);

// Static data

#define IDMA_XFER_SIZE 1024
#define CNT            2

// Buffer in local memory
ALIGNDCACHE int8_t lmem_buf[IDMA_XFER_SIZE] IDMA_DRAM;

// Buffer in system memory
ALIGNDCACHE int8_t smem_buf[IDMA_XFER_SIZE];

// Descriptor buffer
IDMA_BUFFER_DEFINE(desc_buf, 2, IDMA_1D_DESC);


// Thread entry point (also library entry point). You could have
// this function return a pointer to another thread entry point,
// or to something else. The caller has to know what the inputs
// and outputs of the _start function are.

int32_t
_start(void * arg)
{
    idma_status_t ret;
    uint32_t      val = 0;

    int32_t i;
    int32_t j;
    int32_t ch = (int32_t) arg;

    // NOTE: assume idma_init() has been called for the channel,
    // by the supervisor code that loaded us.

    ret = idma_init_loop(ch,
                         desc_buf,
                         IDMA_1D_DESC,
                         2,
                         NULL,
                         NULL);
    if (ret != IDMA_OK) {
        printf("idma init err %d\n", ret);
        return -1;
    }

    for (i = 0; i < CNT; i++) {
        puts("idma: start transfer");

        // Generate source pattern, clear dest buffer
        for (j = 0; j < IDMA_XFER_SIZE/4; j++) {
            ((uint32_t *) lmem_buf)[j] = val;
            val++;
        }
        memset(smem_buf, 0x00, IDMA_XFER_SIZE);
        xthal_dcache_region_writeback_inv(smem_buf, IDMA_XFER_SIZE);

        // Schedule transfers
        idma_add_desc(desc_buf,
                      smem_buf,
                      lmem_buf,
                      IDMA_XFER_SIZE/2,
                      0);
        idma_add_desc(desc_buf,
                      smem_buf + IDMA_XFER_SIZE/2,
                      lmem_buf + IDMA_XFER_SIZE/2,
                      IDMA_XFER_SIZE/2,
                      0);
        idma_schedule_desc(ch, 2);

        // Wait for completion
        while ((j = idma_buffer_status(ch)) > 0);
        if (j < 0) {
            printf("idma_buffer_status error %d\n", j);
            return j;
        }

        // Check result
        if (memcmp(smem_buf, lmem_buf, IDMA_XFER_SIZE) != 0) {
            puts("memory compare failed");
            return -1;
        }

        puts("idma: transfer OK");
    }

    return 0;
}

