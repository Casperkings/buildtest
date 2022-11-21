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
 
/* EXAMPLE:
  Execute 2D transfer from main to the local memory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "idma_tests.h"

#define COPY_SIZE    2048
#define IMAGE_BUFFER_DEPTH 1

ALIGNDCACHE char _lbuf[COPY_SIZE] IDMA_DRAM ;
ALIGNDCACHE char _mem[COPY_SIZE];

IDMA_BUFFER_DEFINE(buffer, IMAGE_BUFFER_DEPTH, IDMA_1D_DESC);

void
idma_cb_func (void* data)
{
  printf("CALLBACK: Copy request for task @ %p is done\n", data);
}

int
test(void* arg, int unused)
{
  idma_log_handler(idmaLogHander);

  printf("  Schedule DMA %p -> %p\n", _mem, _lbuf);

  idma_init(IDMA_CH_ARG  0, MAX_BLOCK_2, 16, TICK_CYCLES_2, 100000, idmaErrCB);

  bufRandomize(_mem, COPY_SIZE);
  xthal_dcache_region_writeback_inv(_mem, COPY_SIZE);

  idma_init_loop(IDMA_CH_ARG  buffer, IDMA_1D_DESC, IMAGE_BUFFER_DEPTH, NULL, NULL);
  
  /* Execute using convinience function */
  idma_copy_desc(IDMA_CH_ARG  _lbuf, _mem, COPY_SIZE, 0);    
  idma_hw_wait_all(IDMA_CH_ARG_SINGLE);
  
  xthal_dcache_region_invalidate(_mem, COPY_SIZE);
  
  if(idma_buffer_status(IDMA_CH_ARG_SINGLE) < 0) {
     idma_error_details_t* error = idma_error_details(IDMA_CH_ARG_SINGLE);
     printf("COPY FAILED, Error 0x%x at desc:%p, PIF src/dst=%x/%x\n", error->err_type, (void*)error->currDesc, error->srcAddr, error->dstAddr);
     exit(0);
  }

  compareBuffers(0, _lbuf, _mem, COPY_SIZE);

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
