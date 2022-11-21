/*
 * Copyright (c) 2019 by Cadence Design Systems. ALL RIGHTS RESERVED.
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

/*
 * Example: test 36-bit special DMA transfer in fixed-buffer mode. This test
   copies data from local memory to system memory address > 32 bits and then
   copies back to local memory. Works on ISS, RTL may require special setup.
 */

// Use the multichannel API even though it is a single-channel test.
#define IDMA_USE_MULTICHANNEL           1

// Define this to enable the 36-bit API.
#define IDMA_USE_WIDE_ADDRESS_COMPILE   1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <xtensa/hal.h>

#include "idma_tests.h"


#define IMAGE_CHUNK_SIZE	512
#define IDMA_XFER_SIZE		IDMA_SIZE(IMAGE_CHUNK_SIZE)


// Define just one buffer with 2 descriptors. The first will copy localmem to sysmem,
// the second will copy sysmem to localmem.

IDMA_BUFFER_DEFINE(buff1, 2, IDMA_1D_DESC);

ALIGNDCACHE int8_t dst1[IDMA_XFER_SIZE];
ALIGNDCACHE int8_t src1[IDMA_XFER_SIZE] IDMA_DRAM;
ALIGNDCACHE int8_t dst2[IDMA_XFER_SIZE] IDMA_DRAM;


// IDMA completion callback.
static void
idma_cb(void * arg)
{
    int32_t status;
    int32_t ch = (int32_t) arg;

    status = idma_buffer_status(ch);
    if (status < 0) {
        idma_error_details_t * error = idma_error_details(ch);
        printf("ch%d COPY FAILED, Error 0x%x at desc:%p, PIF src/dst=%x/%x\n",
            ch, error->err_type, (void *)error->currDesc, error->srcAddr, error->dstAddr);
    }
    else {
    }
}

// Test function.
int
test(void * arg, int unused)
{
    // Construct wide pointers for the 3 buffers. Note that the addresses for the
    // two buffers in local memory must not have any of the high bits set or else
    // they'll look like sysmem addresses.
    uint64_t dst1_wide = ((uint64_t)1 << 32) | (uint32_t)&dst1;
    uint64_t src1_wide = ((uint64_t)0 << 32) | (uint32_t)&src1;
    uint64_t dst2_wide = ((uint64_t)0 << 32) | (uint32_t)&dst2;

    int32_t ch = 0;
    int32_t ret;
    int32_t i;

    idma_log_handler(idmaLogHander);
    ret = idma_init(ch, IDMA_OCD_HALT_ON, MAX_BLOCK_2, 16, TICK_CYCLES_8, 100000, idmaErrCB);
    if (ret != IDMA_OK) {
        printf("FAIL: idma_init failed for ch %d: %d\n", ch, ret);
        exit(-1);
    }

    ret = idma_init_loop(ch, buff1, IDMA_1D_DESC, 2, (void *) ch, idma_cb);
    if (ret != IDMA_OK) {
        printf("FAIL: idma_init_loop failed for ch %d: %d\n", ch, ret);
        exit(-1);
    }

    idma_add_desc(buff1, &dst1_wide, &src1_wide, IDMA_XFER_SIZE, DESC_NOTIFY_W_INT | DESC_IDMA_PRIOR_H);
    idma_add_desc(buff1, &dst2_wide, &dst1_wide, IDMA_XFER_SIZE, DESC_NOTIFY_W_INT | DESC_IDMA_PRIOR_H);

    for (i = 1; i <= 4; i++) {
        // Generate new data and make sure src != dst.
        bufRandomize((char *) src1, IDMA_XFER_SIZE);
        bufRandomize((char *) dst2, IDMA_XFER_SIZE);

        // Update the system memory address (dst addr) in the first desc.
        dst1_wide = ((uint64_t)i << 32) | (uint32_t)&dst1;
        idma_update_desc_dst(ch, &dst1_wide);

        ret = idma_schedule_desc(ch, 1);
        while (idma_desc_done(ch, ret) == 0);

        // Now update the src addr in the second desc.
        idma_update_desc_src(ch, &dst1_wide);

        ret = idma_schedule_desc(ch, 1);
        while (idma_desc_done(ch, ret) == 0);

        // Compare data.
        if (compareBuffers(0, (char *) src1, (char *) dst2, IDMA_XFER_SIZE) != 0) {
            exit(-1);
        }
    }

    exit(0);
    return -1;
}

int
main(int argc, char** argv)
{
    int ret = 0;

    printf("\n\n\nTest '%s'\n\n\n", argv[0]);

#if defined _XOS
    ret = test_xos();
#else
    test(0, 0);
#endif
    // test() exits so this is never reached
    return ret;
}

