/*
 * Copyright (c) 2014 by Cadence Design Systems. ALL RIGHTS RESERVED.
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

// Test predicated 2D transfers.

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <xtensa/hal.h>

#include "idma_tests.h"

#define IMAGE_CHUNK_SIZE	256
#define IDMA_XFER_SIZE		IDMA_SIZE(IMAGE_CHUNK_SIZE)
#define NUM_ROWS                32

IDMA_BUFFER_DEFINE(buffer1, 1, IDMA_64B_DESC);

ALIGNDCACHE char dst[IDMA_XFER_SIZE * NUM_ROWS];
ALIGNDCACHE char src[IDMA_XFER_SIZE * NUM_ROWS] IDMA_DRAM;


#if (IDMA_USE_64B_DESC > 0)

#define PRED_MASK_SIZE	        ((NUM_ROWS + 7) / 8)

char row_mask[PRED_MASK_SIZE] __attribute__((aligned(4))) IDMA_DRAM;


static int32_t
test_desc_copy_2d(int32_t wide,
                  int32_t row_size,
                  int32_t src_pitch,
                  int32_t dst_pitch,
                  char *  row_mask)
{
    int32_t  ret;
    uint64_t srcw;
    uint64_t dstw;

    if (row_size > IDMA_XFER_SIZE) {
        return -1;
    }

    printf("Test: wide %d row_size %d src_pitch %d dst_pitch %d\n",
           wide, row_size, src_pitch, dst_pitch);
    printf("    row_mask: %02x %02x %02x %02x\n",
           row_mask[0], row_mask[1], row_mask[2], row_mask[3]);

    bufRandomize(src, IDMA_XFER_SIZE * NUM_ROWS);
    bufRandomize(dst, IDMA_XFER_SIZE * NUM_ROWS);

    // 'src' is in local memory no need to writeback dcache.
    xthal_dcache_region_writeback(dst, IDMA_XFER_SIZE * NUM_ROWS);

    idma_init(0, MAX_BLOCK_2, 16, TICK_CYCLES_8, 100000, idmaErrCB);
    idma_init_loop(buffer1, IDMA_64B_DESC, 1, NULL, NULL);

    dstw = (uint64_t)((uint32_t) dst);
    srcw = (uint64_t)((uint32_t) src);

    if (wide) {
#if (IDMA_USE_WIDE_API > 0)
        if (row_mask) {
            idma_copy_2d_pred_desc64_wide(0, &dstw, &srcw, row_size, 0, row_mask, NUM_ROWS, src_pitch, dst_pitch);
        } else {
            idma_copy_2d_desc64_wide(0, &dstw, &srcw, row_size, 0, NUM_ROWS, src_pitch, dst_pitch);
        }
#endif
    } else {
        if (row_mask) {
            idma_copy_2d_pred_desc64(0, &dstw, &srcw, row_size, 0, row_mask, NUM_ROWS, src_pitch, dst_pitch);
        } else {
            idma_copy_2d_desc64(0, &dstw, &srcw, row_size, 0, NUM_ROWS, src_pitch, dst_pitch);
        }
    }

   idma_hw_wait_all();
   
    if (row_mask) {
        xthal_dcache_region_invalidate(dst, IDMA_XFER_SIZE * NUM_ROWS);
        ret = compareRowsPred(0, src, dst, IDMA_XFER_SIZE * NUM_ROWS, row_size, row_mask, NUM_ROWS, src_pitch, dst_pitch);
    }
    else {
        xthal_dcache_region_invalidate(dst, IDMA_XFER_SIZE);
        ret = compareBuffers(0, src, dst, IDMA_XFER_SIZE);
    }

    printf(" '%s' %s\n\n", __FUNCTION__, ret ? "FAILED":"PASSED");
    return 0;
}

/* THIS IS THE EXAMPLE, main() CALLS IT */
int
test(void* arg, int unused)
{
    memset(row_mask, 0xAA, PRED_MASK_SIZE);
    idma_log_handler(idmaLogHander);

    test_desc_copy_2d(0,  75, 256, 200, row_mask);
    row_mask[0] = 0xCF;
    test_desc_copy_2d(0, 128, 256, 128, row_mask);
    row_mask[0] = 0x00;
    test_desc_copy_2d(0, 150, 256, 200, row_mask);
    row_mask[1] = 0x11;
    test_desc_copy_2d(0, 200, 256, 256, row_mask);

#if 0 //(IDMA_USE_WIDE_API > 0)
   test_desc_copy_2d(1, row_mask);
#endif
    exit(0);
    return -1;
}

int
main(int argc, char *argv[])
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

#endif

