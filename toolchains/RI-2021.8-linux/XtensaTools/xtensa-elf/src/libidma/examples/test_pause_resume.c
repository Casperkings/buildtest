/*
 * Copyright (c) 2016 by Cadence Design Systems. ALL RIGHTS RESERVED.
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

#include <stdio.h>
#include <stdlib.h>
#include <xtensa/hal.h>

#include "idma_tests.h"

#define IDMA_XFER_SIZE		4000

ALIGNDCACHE char img_src[IDMA_XFER_SIZE];
ALIGNDCACHE char img_dst[IDMA_XFER_SIZE];
ALIGNDCACHE char IDMA_DRAM _lbuf[IDMA_XFER_SIZE] ;

IDMA_BUFFER_DEFINE(buffer, 1, IDMA_1D_DESC);

void
idma_cb_func (void* data)
{
  DPRINT("CALLBACK: Copy request for buffer @ %p is done\n", data);
}


/* THIS IS THE EXAMPLE, main() CALLS IT */
int
test(void* arg, int unused)
{
  idma_log_handler(idmaLogHander);
  idma_init(IDMA_CH_ARG  0, MAX_BLOCK_2, 16, TICK_CYCLES_2, 100000, idmaErrCB); 
  idma_init_loop(IDMA_CH_ARG  buffer, IDMA_1D_DESC, 2, buffer, idma_cb_func);

  bufRandomize(img_src, IDMA_XFER_SIZE);
  bufRandomize(_lbuf, IDMA_XFER_SIZE);
  bufRandomize(img_dst, IDMA_XFER_SIZE);
  xthal_dcache_region_writeback(img_dst, IDMA_XFER_SIZE);
  xthal_dcache_region_writeback(img_src, IDMA_XFER_SIZE);

  idma_copy_desc(IDMA_CH_ARG  _lbuf, img_src, IDMA_XFER_SIZE, 0 );
  do {
    idma_pause(IDMA_CH_ARG_SINGLE);
    idma_resume(IDMA_CH_ARG_SINGLE);
  }
  while (idma_buffer_status(IDMA_CH_ARG_SINGLE) > 0);
  compareBuffers(0, img_src, _lbuf, IDMA_XFER_SIZE);

  idma_copy_desc(IDMA_CH_ARG  img_dst, _lbuf, IDMA_XFER_SIZE, 0 );
  do {
    idma_pause(IDMA_CH_ARG_SINGLE);
    idma_resume(IDMA_CH_ARG_SINGLE);
  }
  while (idma_buffer_status(IDMA_CH_ARG_SINGLE) > 0);
  xthal_dcache_region_invalidate(img_dst, IDMA_XFER_SIZE);
  compareBuffers(0, img_src, img_dst, IDMA_XFER_SIZE);

  return 0;
}

int
main (int argc, char**argv)
{
   int ret = 0;
   printf("\n\n\nTest '%s'\n\n\n", argv[0]);

#if defined _XOS
   ret = test_xos();
#else
   ret = test(0, 0);
#endif
  // test() exits so this is never reached
  return ret;
}
