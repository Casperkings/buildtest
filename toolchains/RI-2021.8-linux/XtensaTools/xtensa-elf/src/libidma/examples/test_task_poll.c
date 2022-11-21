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
   Schedule various tasks and wait for their completion.  
   Wait for task completion using polling wait.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "idma_tests.h"

#define IMAGE_CHUNK_SIZE	100
#define IDMA_XFER_SIZE		IDMA_SIZE(IMAGE_CHUNK_SIZE)
#define IDMA_IMAGE_SIZE		IMAGE_CHUNK_SIZE*200

IDMA_BUFFER_DEFINE(task, 3, IDMA_1D_DESC);

ALIGNDCACHE char dst1[IDMA_XFER_SIZE];
ALIGNDCACHE char src1[IDMA_XFER_SIZE] IDMA_DRAM;
ALIGNDCACHE char dst2[IDMA_XFER_SIZE];
ALIGNDCACHE char src2[IDMA_XFER_SIZE] IDMA_DRAM;
ALIGNDCACHE char dst3[IDMA_XFER_SIZE];
ALIGNDCACHE char src3[IDMA_XFER_SIZE] IDMA_DRAM;

void compareBuffers_task(idma_buffer_t* task, char* src, char* dst, int size)
{
  int status = idma_task_status(task);
  printf("Task %p status: %s\n", task,
                    (status == IDMA_TASK_DONE)    ? "DONE"       :
                    (status > IDMA_TASK_DONE)     ? "PENDING"    :
                    (status == IDMA_TASK_ABORTED) ? "ABORTED"    :
                    (status == IDMA_TASK_ERROR)   ? "ERROR"      :  "UNKNOWN"   );

  if(status == IDMA_TASK_ERROR) {
    idma_error_details_t* error = idma_error_details(IDMA_CH_ARG_SINGLE);
    printf("COPY FAILED, Error 0x%x at desc:%p, PIF src/dst=%x/%x\n", error->err_type, (void*)error->currDesc, error->srcAddr, error->dstAddr);
    return;
  }

  if(src && dst) {
    xthal_dcache_region_invalidate(dst, size);
    compareBuffers(0, src, dst, size);
  }
}

/* THIS IS THE EXAMPLE, main() CALLS IT */
int
test(void* arg, int unused)
{
   bufRandomize(src1, IDMA_XFER_SIZE);
   bufRandomize(src2, IDMA_XFER_SIZE);
   bufRandomize(src3, IDMA_XFER_SIZE);

   idma_log_handler(idmaLogHander);
   idma_init(IDMA_CH_ARG  0, MAX_BLOCK_2, 16, TICK_CYCLES_8, 100000, NULL);

   idma_init_task(IDMA_CH_ARG  task, IDMA_1D_DESC, 3, NULL, NULL);

   DPRINT("Setting IDMA request %p (4 descriptors)\n", task);
   idma_add_desc(task, dst1, src1, IDMA_XFER_SIZE, 0 );
   idma_add_desc(task, dst2, src2, IDMA_XFER_SIZE, 0 );
   idma_add_desc(task, dst3, src3, IDMA_XFER_SIZE, 0 );

   DPRINT("Schedule IDMA request\n");
   idma_schedule_task(task);

   while (idma_task_status(task) > 0)
      idma_process_tasks(IDMA_CH_ARG_SINGLE  );

   compareBuffers_task(task, src1, dst1, IDMA_XFER_SIZE );
   compareBuffers_task(task, src2, dst2, IDMA_XFER_SIZE );
   compareBuffers_task(task, src3, dst3, IDMA_XFER_SIZE );

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

