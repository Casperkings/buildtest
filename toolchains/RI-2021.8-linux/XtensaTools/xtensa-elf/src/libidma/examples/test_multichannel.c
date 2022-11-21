/*
 * Copyright (c) 2018 by Cadence Design Systems. ALL RIGHTS RESERVED.
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

/* Example: test one or more IDMA channels using the multichannel API. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <xtensa/hal.h>

#if !defined(XCHAL_IDMA_NUM_CHANNELS) || (XCHAL_IDMA_NUM_CHANNELS < 1)

int
test(void * arg, int unused)
{
    printf("INFO: This config does not support IDMA.\n");
    return 0;
}

#else

// Must define this before including idma.h.
#define IDMA_USE_MULTICHANNEL

#include "idma_tests.h"

#if IDMA_USE_64B_DESC
#define DESC_TYPE               IDMA_64B_DESC
#else
#define DESC_TYPE               IDMA_1D_DESC
#endif

#define NUM_CHAN                XCHAL_IDMA_NUM_CHANNELS
#define NUM_DESC                2
#define NUM_TRANSFERS           64
#define IDMA_XFER_SIZE          512
#define IDMA_TOTAL_SIZE         (IDMA_XFER_SIZE * NUM_TRANSFERS)

// Hardcoded for now up to 4 channels. No way to define an array.
IDMA_BUFFER_DEFINE(buffer_ch0, 2, DESC_TYPE);
IDMA_BUFFER_DEFINE(buffer_ch1, 2, DESC_TYPE);
#if (NUM_CHAN > 2)
IDMA_BUFFER_DEFINE(buffer_ch2, 2, DESC_TYPE);
#endif
#if (NUM_CHAN > 3)
IDMA_BUFFER_DEFINE(buffer_ch3, 2, DESC_TYPE);
#endif

idma_buffer_t * descbuf[] = {
    buffer_ch0,
    buffer_ch1,
#if (NUM_CHAN > 2)
    buffer_ch2,
#endif
#if (NUM_CHAN > 3)
    buffer_ch3,
#endif
};

// Source buffers in local memory.
ALIGNDCACHE int8_t src_buf0[NUM_CHAN][IDMA_XFER_SIZE] IDMA_DRAM;
ALIGNDCACHE int8_t src_buf1[NUM_CHAN][IDMA_XFER_SIZE] IDMA_DRAM;

// Destination buffers in external memory.
int8_t * dst_buf[NUM_CHAN];

// Destination buffers for compare (filled by CPU).
int8_t * dst_ref[NUM_CHAN];

// Per-channel counts and flags.
uint8_t xfer_count[NUM_CHAN];
uint8_t xfer_done[NUM_CHAN];
uint8_t cb_done[NUM_CHAN];
uint8_t cb_err[NUM_CHAN];

// IDMA completion callback.
static void
idma_cb(void * arg)
{
    int32_t status;
    int32_t ch = (int32_t) arg;

    DPRINT("ch%d callback\n", ch);
    status = idma_buffer_status(ch);
    if (status < 0) {
        idma_error_details_t * error = idma_error_details(ch);
        printf("ch%d COPY FAILED, Error 0x%x at desc:%p, PIF src/dst=%x/%x\n",
            ch, error->err_type, (void *)error->currDesc, error->srcAddr, error->dstAddr);
        cb_err[ch] = 1;
    }
    else {
        cb_done[ch] = 1;
    }
}

// IDMA error callback.
static void
idma_err_cb(const idma_error_details_t * data)
{
    printf("FAIL, Error 0x%x at desc:%p, PIF src/dst=%x/%x\n",
           data->err_type, (void *) data->currDesc, data->srcAddr, data->dstAddr);
}

static int
wait_for_cb(int32_t ch)
{
    DPRINT("Waiting for ch%d callback...\n", ch);
    while (1) {
        if (cb_done[ch]) {
            return 1;
        }
        if (cb_err[ch]) {
            return 0;
        }
    }
}

static void
prepare_chunk(int32_t ch)
{
    DPRINT("ch%d: prepare_chunk %d\n", ch, xfer_count[ch]);

    int8_t * src = (xfer_count[ch] & 1) ? src_buf1[ch] : src_buf0[ch];
    int32_t  ofs = xfer_count[ch] * IDMA_XFER_SIZE;
#if IDMA_USE_64B_DESC
    int8_t * dst = dst_buf[ch] + ofs;
#endif

    // Generate random data for this chunk.
    bufRandomize((char *) src, IDMA_XFER_SIZE);
    // Make a reference copy.
    memcpy(dst_ref[ch] + ofs, src, IDMA_XFER_SIZE);
#if IDMA_USE_64B_DESC
    idma_update_desc64_dst(ch, &dst);
#else
    idma_update_desc_dst(ch, dst_buf[ch] + ofs);
#endif
}

static void
send_next_buf(int32_t ch)
{
    if (xfer_done[ch]) {
        return;
    }

    cb_done[ch] = 0;
    DPRINT("ch%d: schedule_desc %d\n", ch, xfer_count[ch]);
    idma_schedule_desc(ch, 1);
    xfer_count[ch]++;
    if (xfer_count[ch] == NUM_TRANSFERS) {
        xfer_done[ch] = 1;
        return;
    }

    prepare_chunk(ch);
}

// Test loop.
int
test(void * arg, int unused)
{
    uint32_t desc_opts = DESC_NOTIFY_W_INT | DESC_IDMA_PRIOR_H ;
    int32_t  ch;
    int32_t  done;
#if IDMA_USE_64B_DESC
    int8_t * src_mem1;
    int8_t * src_mem2;
#endif
    int8_t * dst_mem1;
    int8_t * dst_mem2;
    int32_t  ret;

    idma_log_handler(idmaLogHander);

    for (ch = 0; ch < NUM_CHAN; ch++) {
        dst_buf[ch] = (int8_t *) malloc(IDMA_TOTAL_SIZE + IDMA_DCACHE_ALIGN);
        dst_ref[ch] = (int8_t *) malloc(IDMA_TOTAL_SIZE + IDMA_DCACHE_ALIGN);

        if (!dst_buf[ch] || !dst_ref[ch]) {
            printf("INFO: not enough memory to run test.\n");
            exit(0);
        }
        else {
            // Align to dcache line boundary to prevent accidental sharing with other data.
            dst_buf[ch] = (int8_t *) (((uint32_t)dst_buf[ch] + IDMA_DCACHE_ALIGN - 1) & ~(IDMA_DCACHE_ALIGN - 1));
            dst_ref[ch] = (int8_t *) (((uint32_t)dst_ref[ch] + IDMA_DCACHE_ALIGN - 1) & ~(IDMA_DCACHE_ALIGN - 1));
        }

#if IDMA_USE_64B_DESC
        src_mem1 = src_buf0[ch];
        src_mem2 = src_buf1[ch];
#endif
        dst_mem1 = dst_buf[ch];
        dst_mem2 = dst_buf[ch] + IDMA_XFER_SIZE;

        DPRINT("Init ch%d\n", ch);
        ret = idma_init(ch, IDMA_OCD_HALT_ON, MAX_BLOCK_2, 16, TICK_CYCLES_8, 100000, idma_err_cb);
        if (ret != IDMA_OK) {
            printf("FAIL: idma_init failed for ch %d: %d\n", ch, ret);
            exit(-1);
        }

        DPRINT("Set up ch%d desc buffer\n", ch);
        ret = idma_init_loop(ch, descbuf[ch], DESC_TYPE, NUM_DESC, (void *) ch, idma_cb);
        if (ret != IDMA_OK) {
            printf("FAIL: idma_init_loop failed for ch %d: %d\n", ch, ret);
            exit(-1);
        }

#if IDMA_USE_64B_DESC
        idma_add_desc64(descbuf[ch], &dst_mem1, &src_mem1, IDMA_XFER_SIZE, desc_opts);
        idma_add_desc64(descbuf[ch], &dst_mem2, &src_mem2, IDMA_XFER_SIZE, desc_opts);
#else
        idma_add_desc(descbuf[ch], dst_mem1, src_buf0[ch], IDMA_XFER_SIZE, desc_opts);
        idma_add_desc(descbuf[ch], dst_mem2, src_buf1[ch], IDMA_XFER_SIZE, desc_opts);
#endif
    }

    //  Send the first buffer.
    for (ch = 0; ch < NUM_CHAN; ch++) {
        prepare_chunk(ch);
        send_next_buf(ch);
    }

    done = 0;
    while (!done) {
        for (ch = 0; ch < NUM_CHAN; ch++) {
            int32_t ret = wait_for_cb(ch);

            if (ret == 0) {
                DPRINT("ch%d: cb Error\n", ch);
            }
            else {
                DPRINT("ch%d: cb OK\n", ch);
            }
            send_next_buf(ch);
        }

        done = 1;
        for (ch = 0; ch < NUM_CHAN; ch++) {
            if (!xfer_done[ch]) {
                done = 0;
            }
        }
    }

    for (ch = 0; ch < NUM_CHAN; ch++) {
        // Must invalidate DMA dst buffers else we may get stale data from dcache.
        xthal_dcache_region_invalidate(dst_buf[ch], IDMA_TOTAL_SIZE);
        compareBuffers(0, (char *) dst_buf[ch], (char *) dst_ref[ch], IDMA_TOTAL_SIZE);
    }

    return 0;
}

#endif // XCHAL_IDMA_NUM_CHANNELS

int
main(int argc, char * argv[])
{
    int ret = 0;

    printf("\n\n\nTest '%s'\n\n\n", argv[0]);
#if defined _XOS
    ret = test_xos();
#else
    ret = test(0, 0);
#endif
    return ret;
}

