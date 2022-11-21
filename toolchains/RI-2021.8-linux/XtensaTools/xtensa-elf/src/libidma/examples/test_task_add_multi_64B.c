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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <xtensa/hal.h>

#include "idma_tests.h"

#define IMAGE_CHUNK_SIZE	200
#define IDMA_XFER_SIZE		IDMA_SIZE(IMAGE_CHUNK_SIZE)

#define XFER_3D_SZ 32

#if (IDMA_USE_64B_DESC > 0)

void
idma_cb_func (void* data)
{
  DPRINT("CALLBACK: Copy request for task @ %p is done. Outstanding: %d\n", data, idma_task_status((idma_buffer_t*)data));
}

int compareBuffers_task(idma_buffer_t* task, char* src, char* dst, int size)
{
  int status;

  if (dst && size)
    xthal_dcache_region_invalidate(dst, size);

  status = idma_task_status(task);
  printf("Task %p status: %s\n", task,
                    (status == IDMA_TASK_DONE)    ? "DONE"       :
                    (status  > IDMA_TASK_DONE)    ? "PENDING"    :
                    (status == IDMA_TASK_ABORTED) ? "ABORTED"    :
                    (status == IDMA_TASK_ERROR)   ? "ERROR"      :  "UNKNOWN"   );

  if(status == IDMA_TASK_ERROR) {
    idma_error_details_t* error = idma_error_details(IDMA_CH_ARG_SINGLE);
    printf("COPY FAILED, Error 0x%x at desc:%p, PIF src/dst=%x/%x\n", error->err_type, (void*)error->currDesc, error->srcAddr, error->dstAddr);
    return -1;
  }

  if(src && dst)
    return compareBuffers(0, src, dst, size);
  return 0;
}

ALIGNDCACHE char dst[2][IDMA_XFER_SIZE];
ALIGNDCACHE char src[2][IDMA_XFER_SIZE] IDMA_DRAM;
IDMA_BUFFER_DEFINE(task2, 2, IDMA_64B_DESC);

static int test_task_add_1d()
{
   printf("\n START: %s &src:%p, &dst:%p\n", __FUNCTION__, src, dst);
   bufRandomize(&src[0][0], IDMA_XFER_SIZE*2);
   bufRandomize(&dst[0][0], IDMA_XFER_SIZE*2);
   xthal_dcache_region_writeback_inv(src, IDMA_XFER_SIZE*2);
   xthal_dcache_region_writeback_inv(dst, IDMA_XFER_SIZE*2);

   uint32_t src1 = (uint32_t) &src[0];
   uint32_t src2 = (uint32_t) &src[1];
   uint32_t dst1 = (uint32_t) &dst[0];
   uint32_t dst2 = (uint32_t) &dst[1];

   idma_init(IDMA_CH_ARG  0, MAX_BLOCK_2, 16, TICK_CYCLES_8, 100000, idmaErrCB);
   idma_init_task(IDMA_CH_ARG  task2, IDMA_64B_DESC, 2, idma_cb_func, task2);

   DPRINT( "Setting IDMA request %p (2 descriptors)\n", task2);
   idma_add_desc64(task2, &dst1, &src1, IDMA_XFER_SIZE,  0 );
   idma_add_desc64(task2, &dst2, &src2, IDMA_XFER_SIZE,  0 );

   DPRINT("Schedule IDMA request %p, CB:%p\n", task2, idma_cb_func);

   idma_schedule_task(task2);

   idma_hw_wait_all();

   int ret = compareBuffers_task(task2, &src[0][0], &dst[0][0], IDMA_XFER_SIZE );
   printf(" '%s' %s\n\n", __FUNCTION__, ret ? "FAILED":"PASSED");
   ret = compareBuffers_task(task2, src[1], dst[1], IDMA_XFER_SIZE );
   printf(" '%s' %s\n\n", __FUNCTION__, ret ? "FAILED":"PASSED");
   return 0;
}

/* THIS IS THE EXAMPLE, main() CALLS IT */
int
test(void* arg, int unused)
{
   idma_log_handler(idmaLogHander);
   test_task_add_1d();

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
   test(0, 0);
#endif
  // test() exits so this is never reached
  return ret;
}

#endif
