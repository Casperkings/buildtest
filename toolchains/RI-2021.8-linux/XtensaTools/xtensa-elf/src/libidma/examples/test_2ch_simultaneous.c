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

/* EXAMPLE:
   Test simultaneous use of 2 IDMA channels.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <xtensa/hal.h>

#if (XCHAL_IDMA_NUM_CHANNELS < 2)

int
test(void* arg, int unused)
{
   printf("INFO: Test not supported - needs 2-channel iDMA HW\n");
   exit(0);
   return -1;
}

#else

#define IDMA_USE_MULTICHANNEL   1

#include "idma_tests.h"

#define IMAGE_BUFFER_DEPTH      2    /* Circular buffer num of entries */
#define NUM_TRANSFERS           100   /* total num of idma transfers */
#define IDMA_XFER_SIZE          200  /* Size of each transfer */

#define IDMA_IMAGE_SIZE		IDMA_XFER_SIZE*NUM_TRANSFERS

#define IDMA_CH0  IDMA_CHANNEL_0
#define IDMA_CH1  IDMA_CHANNEL_1

//Define the circular buffer of descriptors
IDMA_BUFFER_DEFINE(buffer_ch0, 2, IDMA_1D_DESC);
IDMA_BUFFER_DEFINE(buffer_ch1, 2, IDMA_1D_DESC);

/* Destination for produced data (DMA fills it) */
ALIGNDCACHE char _mem[XCHAL_IDMA_NUM_CHANNELS][IDMA_IMAGE_SIZE];

/* Originaly generated data - stored for final compare */
ALIGNDCACHE char _image_src[XCHAL_IDMA_NUM_CHANNELS][IDMA_IMAGE_SIZE];

/* Double buffer in the Local Memory, CPU fills */
ALIGNDCACHE char _lbuf0[XCHAL_IDMA_NUM_CHANNELS][IDMA_XFER_SIZE] IDMA_DRAM;
ALIGNDCACHE char _lbuf1[XCHAL_IDMA_NUM_CHANNELS][IDMA_XFER_SIZE] IDMA_DRAM;

/* Sync using Callback */
char _cb_done[XCHAL_IDMA_NUM_CHANNELS] = {1, 1};
char _cb_err[XCHAL_IDMA_NUM_CHANNELS]  = {0, 0};

static int _xfer_num[XCHAL_IDMA_NUM_CHANNELS] = {0, 0};
static int _xfers_done[XCHAL_IDMA_NUM_CHANNELS] = {0, 0};

static void
idma_cb (int ch, idma_buffer_t* buf)
{
  DPRINT("CH%d callback : Copy request for buffer @ %p is done\n", ch, buf);
  int status = idma_buffer_status(ch);
  if(status < 0) {
    idma_error_details_t* error = idma_error_details(ch);
    printf("CH%d COPY FAILED, Error 0x%x at desc:%p, PIF src/dst=%x/%x\n", ch, error->err_type, (void*)error->currDesc, error->srcAddr, error->dstAddr);
    _cb_err[ch] = 1;
  }
  else
    _cb_done[ch] = 1;
}

static void
idma_cb_ch0 (void* data)
{
  idma_buffer_t* buf = (idma_buffer_t*) data;
  idma_cb(IDMA_CH0, buf);
}

static void
idma_cb_ch1 (void* data)
{
  idma_buffer_t* buf = (idma_buffer_t*) data;
  idma_cb(IDMA_CH1, buf);
}

static int
wait_for_cb ()
{
  DPRINT("Waiting for interrupt callback...\n");
  while (1) {
    if (_cb_done[IDMA_CH0] || _cb_done[IDMA_CH1]) return 1;
    if (_cb_err[IDMA_CH0] || _cb_err[IDMA_CH1]) return 0;
  }
}

/* CPU creates random data to serve as new image chunk */
void _prepare_next_chunk(int ch)
{
  DPRINT("CH%d: Create data chunk %d\n", ch, _xfer_num[ch]);

  char* lbuf = (_xfer_num[ch] & 1) ? _lbuf1[ch] : _lbuf0[ch];
  int memoffs = _xfer_num[ch]*IDMA_XFER_SIZE;

  bufRandomize(lbuf, IDMA_XFER_SIZE);
  memcpy(_image_src[ch] + memoffs, lbuf, IDMA_XFER_SIZE);
  idma_update_desc_dst(ch, _mem[ch] + memoffs);
}

void _try_next_buffer(int ch)
{
  // check if previous one is done
  if (!_cb_done[ch])
    return;

  // mark prepared chunk as not done, and schedule it
  _cb_done[ch] = 0;
  idma_schedule_desc(ch, 1);
  _xfer_num[ch]++;

  // if this one was the last one don't prepare/schedule anymore
  if (_xfer_num[ch] == NUM_TRANSFERS) {
    _xfers_done[ch] = 1;
    return;
  }
  _prepare_next_chunk(ch);
}

/* THIS IS THE EXAMPLE, main() CALLS IT */
int test(void* arg, int unused)
{
  xthal_dcache_region_writeback_inv(_mem, IDMA_IMAGE_SIZE*2);
  xthal_dcache_region_writeback_inv(_image_src, IDMA_IMAGE_SIZE*2);

  idma_log_handler(idmaLogHander);
  idma_init(IDMA_CH0, IDMA_OCD_HALT_ON, MAX_BLOCK_2, 16, TICK_CYCLES_8, 100000, idmaErrCB_ch0);
  idma_init(IDMA_CH1, IDMA_OCD_HALT_ON, MAX_BLOCK_2, 16, TICK_CYCLES_8, 100000, idmaErrCB_ch1);

  /* Create 2 descriptors to support double buffer.
     Add JUMP from the 2nd to the 1st one.
     Each desc. destination points to the one double buffer entry.
  */
  DPRINT("Create descriptor double buffer for ch0\n");
  idma_init_loop(IDMA_CH0, buffer_ch0, IDMA_1D_DESC, IMAGE_BUFFER_DEPTH, buffer_ch0, idma_cb_ch0);

  DPRINT("Create descriptor double buffer for ch1\n");
  idma_init_loop(IDMA_CH1, buffer_ch1, IDMA_1D_DESC, IMAGE_BUFFER_DEPTH, buffer_ch1, idma_cb_ch1);

  uint32_t desc_opts =  DESC_NOTIFY_W_INT | DESC_IDMA_PRIOR_H ;
  idma_add_desc(buffer_ch0, _mem[IDMA_CH0], _lbuf0[IDMA_CH0], IDMA_XFER_SIZE,  desc_opts );
  idma_add_desc(buffer_ch0, _mem[IDMA_CH0], _lbuf1[IDMA_CH0], IDMA_XFER_SIZE,  desc_opts );

  idma_add_desc(buffer_ch1, _mem[IDMA_CH1], _lbuf0[IDMA_CH1], IDMA_XFER_SIZE,  desc_opts );
  idma_add_desc(buffer_ch1, _mem[IDMA_CH1], _lbuf1[IDMA_CH1], IDMA_XFER_SIZE,  desc_opts );

  _prepare_next_chunk(IDMA_CH0);
  _prepare_next_chunk(IDMA_CH1);

  while (!_xfers_done[IDMA_CH0] || !_xfers_done[IDMA_CH1]) {

     // Wait for Callback to set flag (initial set, so it proceeds immediatelly)
     DPRINT("  Waiting for DMA to finish a descriptor....\n");
     int ret = wait_for_cb();
     if(ret == 0) {
	printf("ERROR: Got error. TODO: process it here...\n");
     }
     _try_next_buffer(IDMA_CH0);
     _try_next_buffer(IDMA_CH1);
  }

  xthal_dcache_region_invalidate(_mem[IDMA_CH0], IDMA_IMAGE_SIZE);
  xthal_dcache_region_invalidate(_mem[IDMA_CH1], IDMA_IMAGE_SIZE);

  compareBuffers(0, _image_src[IDMA_CH0], _mem[IDMA_CH0], IDMA_IMAGE_SIZE);
  compareBuffers(0, _image_src[IDMA_CH1], _mem[IDMA_CH1], IDMA_IMAGE_SIZE);

  exit(0);
  return -1;
}
#endif //multichannel

int
main (int argc, char**argv)
{
   int ret = 0;
   printf("\n\n\nTest '%s'\n\n\n", argv[0]);
# if defined _XOS
   ret = test_xos();
# else
   ret = test(0, 0);
# endif
  return ret;
}
