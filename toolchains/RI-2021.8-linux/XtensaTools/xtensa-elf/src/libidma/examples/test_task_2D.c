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

#include <xtensa/hal.h>

#include "idma_tests.h"

#define ROW_SIZE  32
#define NUM_ROWS  32
#define SRC_PITCH 512
#define DST_PITCH 512

#define IDMA_IMAGE_SIZE		NUM_ROWS*ROW_SIZE

ALIGNDCACHE char _lbuf1[DST_PITCH*NUM_ROWS] IDMA_DRAM ;
ALIGNDCACHE char _lbuf2[DST_PITCH*NUM_ROWS] IDMA_DRAM ;
ALIGNDCACHE char _mem1[SRC_PITCH*NUM_ROWS];
ALIGNDCACHE char _mem2[SRC_PITCH*NUM_ROWS];

IDMA_BUFFER_DEFINE(task1, 1, IDMA_2D_DESC);
IDMA_BUFFER_DEFINE(task2, 1, IDMA_2D_DESC);

void
idma_cb_func (void* data)
{
  DPRINT("CALLBACK: Copy request for task @ %p is done\n", data);
}


void compareBuffers_task(idma_buffer_t* task, char* src, char* dst, int size)
{
  int i ;
  int status;

  status = idma_task_status(task);
  DPRINT("Task %p status: %s\n", task,
                    (status == IDMA_TASK_DONE)    ? "DONE"       :
                    (status > IDMA_TASK_DONE)     ? "PENDING"    :
                    (status == IDMA_TASK_ABORTED) ? "ABORTED"    :
                    (status == IDMA_TASK_ERROR)   ? "ERROR"      :  "UNKNOWN"   );

  if(status == IDMA_TASK_ERROR) {
    idma_error_details_t* error = idma_error_details(IDMA_CH_ARG_SINGLE);
    printf("COPY FAILED, Error 0x%x at desc:%p, PIF src/dst=%x/%x\n", error->err_type, (void*)error->currDesc, error->srcAddr, error->dstAddr);
    return;
  }

  if(!src || !dst)
    return;

  /* Compare must skip the pitch for src and dst */
  for(i = 0 ; i < size; i++) {
    int src_i = (SRC_PITCH) * (i / ROW_SIZE)  + i % ROW_SIZE;
    int dst_i = (DST_PITCH) * (i / ROW_SIZE)  + i % ROW_SIZE;
    //DPRINT("byte src:%d dst:%d (dst:%x, src:%x)!!\n", src_i, dst_i, dst[dst_i], src[src_i]);
    if (dst[dst_i] != src[src_i]) {
     printf("COPY FAILED @ byte %d (dst:%x, src:%x, %x)!!\n", i, dst[dst_i], src[src_i], dst[dst_i+1]);
     return;
    }
  }
  printf("COPY OK (src:%p, dst:%p)\n", src, dst);
}

/* THIS IS THE EXAMPLE, main() CALLS IT */
int
test(void* arg, int unused)
{
  idma_log_handler(idmaLogHander);
  idma_init(IDMA_CH_ARG  0, MAX_BLOCK_2, 16, TICK_CYCLES_2, 100000, idmaErrCB);

  bufRandomize(_mem1, SRC_PITCH*NUM_ROWS);
  bufRandomize(_mem2, SRC_PITCH*NUM_ROWS);
  xthal_dcache_region_writeback_inv(_mem1, SRC_PITCH*NUM_ROWS);
  xthal_dcache_region_writeback_inv(_mem2, SRC_PITCH*NUM_ROWS);

  idma_init_task(IDMA_CH_ARG  task1, IDMA_2D_DESC, 1, idma_cb_func, task1);
  idma_init_task(IDMA_CH_ARG  task2, IDMA_2D_DESC, 1, idma_cb_func, task2);
  idma_add_2d_desc(task1, _lbuf1, _mem1, ROW_SIZE, DESC_NOTIFY_W_INT, NUM_ROWS, SRC_PITCH, DST_PITCH); 
  idma_add_2d_desc(task2, _lbuf2, _mem2, ROW_SIZE, DESC_NOTIFY_W_INT, NUM_ROWS, SRC_PITCH, DST_PITCH);
  
  DPRINT("  Schedule DMA %p -> %p\n", _mem2, _lbuf2);
  DPRINT("  Schedule DMA %p -> %p\n", _mem1, _lbuf1);

  idma_schedule_task(task1);    
  idma_schedule_task(task2); 

  DPRINT("  Waiting for DMAs to finish...\n");
  while (idma_task_status(task2) > 0)
     idma_sleep(IDMA_CH_ARG_SINGLE  );
  
  xthal_dcache_region_invalidate(_mem1, SRC_PITCH*NUM_ROWS);
  xthal_dcache_region_invalidate(_mem2, SRC_PITCH*NUM_ROWS);

  compareBuffers_task(task1, _mem1, _lbuf1, IDMA_IMAGE_SIZE);
  compareBuffers_task(task2, _mem2, _lbuf2, IDMA_IMAGE_SIZE);

  exit(0);
  return -1;
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

