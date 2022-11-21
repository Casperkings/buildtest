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

#ifdef _XOS
# define IDMA_APP_USE_XOS   //will use inlined disable/enable int
#endif
#ifdef _XTOS
# define IDMA_APP_USE_XTOS  //will use inlined disable/enable int
#endif

#include <xtensa/hal.h>

#include "idma_tests.h"

#define IMAGE_CHUNK_SIZE	200
#define IDMA_XFER_SIZE		IDMA_SIZE(IMAGE_CHUNK_SIZE)
#define NUM_ROWS                20

#define XFER_3D_SZ              32

#if (IDMA_USE_64B_DESC > 0)

#define PRED_MASK_SIZE	((NUM_ROWS + 7) / 8)
char row_mask[PRED_MASK_SIZE] __attribute__((aligned(4))) IDMA_DRAM;


void
idma_cb_func (void* data)
{
  DPRINT("CALLBACK: Copy request for task @ %p is done. Outstanding: %d\n", data, idma_task_status((idma_buffer_t*)data));
}

int compareBuffers_task(idma_buffer_t* task, char* src, char* dst, int size, void* row_mask, int row_size)
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

  if(src && dst) {
    if (row_mask)
        return compareRows(0, src, dst, size, row_size, (char*)row_mask);
    else
        return compareBuffers(0, src, dst, size);
  }
  return 0;
}

IDMA_BUFFER_DEFINE(task1, 1, IDMA_64B_DESC);
IDMA_BUFFER_DEFINE(task2, 1, IDMA_64B_DESC);
IDMA_BUFFER_DEFINE(task3, 1, IDMA_64B_DESC);

ALIGNDCACHE char dst[IDMA_XFER_SIZE*NUM_ROWS];
ALIGNDCACHE char src[IDMA_XFER_SIZE*NUM_ROWS] IDMA_DRAM;

ALIGNDCACHE char dst3d[XFER_3D_SZ*XFER_3D_SZ*2];
ALIGNDCACHE char src3d[XFER_3D_SZ*XFER_3D_SZ*2] IDMA_DRAM;

static int test_task_copy_1d(int wide)
{
   printf("\n START: %s\n", __FUNCTION__);
   bufRandomize(src, IDMA_XFER_SIZE);
   bufRandomize(dst, IDMA_XFER_SIZE);
   xthal_dcache_region_writeback_inv(src, IDMA_XFER_SIZE);
   xthal_dcache_region_writeback_inv(dst, IDMA_XFER_SIZE);

   uint64_t dstw = (uint64_t)(uint32_t)dst;
   uint64_t srcw = (uint64_t)(uint32_t)src;
   if (wide) {}

   idma_init(0, MAX_BLOCK_2, 16, TICK_CYCLES_8, 100000, idmaErrCB);

   DPRINT("Schedule IDMA request %p, CB:%p\n", task2, idma_cb_func);
   if (wide) {
#if (IDMA_USE_WIDE_API > 0)
     idma_copy_task64_wide(0, task2, &dstw, &srcw, IDMA_XFER_SIZE, 0, task2, idma_cb_func);
#endif
   }
   else {
     idma_copy_task64(0, task2, &dstw, &srcw, IDMA_XFER_SIZE, 0, task2, idma_cb_func);
   }

   idma_hw_wait_all();

   int ret = compareBuffers_task(task2, src, dst, IDMA_XFER_SIZE, NULL, 0 );
   printf(" '%s' %s\n\n", __FUNCTION__, ret ? "FAILED":"PASSED");
   return 0;
}

static int test_task_copy_2d(int wide, void* row_mask)
{
   printf("\n START: %s  (wide:%d, row_mask:%p)\n", __FUNCTION__, wide, row_mask);
   bufRandomize(src, IDMA_XFER_SIZE*NUM_ROWS);
   bufRandomize(dst, IDMA_XFER_SIZE*NUM_ROWS);
   xthal_dcache_region_writeback_inv(src, IDMA_XFER_SIZE*NUM_ROWS);
   xthal_dcache_region_writeback_inv(dst, IDMA_XFER_SIZE*NUM_ROWS);

   uint64_t dstw = (uint64_t)(uint32_t)dst;
   uint64_t srcw = (uint64_t)(uint32_t)src;
   if (wide) {}

   idma_init(0, MAX_BLOCK_2, 16, TICK_CYCLES_8, 100000, idmaErrCB);

   DPRINT("Schedule IDMA request %p, CB:%p\n", task2, idma_cb_func);
   if (wide) {
#if (IDMA_USE_WIDE_API > 0)
     if (row_mask) {
         idma_copy_2d_pred_task64_wide(0, task2, &dstw, &srcw, IDMA_XFER_SIZE, 0, row_mask, NUM_ROWS, IDMA_XFER_SIZE, IDMA_XFER_SIZE, NULL, NULL);
     } else {
         idma_copy_2d_task64_wide(0, task2, &dstw, &srcw, IDMA_XFER_SIZE, 0, NUM_ROWS, IDMA_XFER_SIZE, IDMA_XFER_SIZE, NULL, NULL);
     }
#endif
   } else {
     if (row_mask) {
         idma_copy_2d_pred_task64(0, task2, &dstw, &srcw, IDMA_XFER_SIZE, 0, row_mask, NUM_ROWS, IDMA_XFER_SIZE, IDMA_XFER_SIZE, NULL, NULL);
     } else {
         idma_copy_2d_task64(0, task2, &dstw, &srcw, IDMA_XFER_SIZE, 0, NUM_ROWS, IDMA_XFER_SIZE, IDMA_XFER_SIZE, NULL, NULL);
     }
   }

   idma_hw_wait_all();

   int ret = compareBuffers_task(task2, src, dst, IDMA_XFER_SIZE*NUM_ROWS, row_mask, IDMA_XFER_SIZE);
   printf(" '%s' %s\n\n", __FUNCTION__, ret ? "FAILED":"PASSED");
   return 0;
}

static int test_task_copy_3d(int wide)
{
   printf("\n START: %s\n", __FUNCTION__);
   bufRandomize(src3d, XFER_3D_SZ*XFER_3D_SZ*2);
   bufRandomize(dst3d, XFER_3D_SZ*XFER_3D_SZ*2);
   xthal_dcache_region_writeback_inv(src3d, XFER_3D_SZ*XFER_3D_SZ*2);
   xthal_dcache_region_writeback_inv(dst3d, XFER_3D_SZ*XFER_3D_SZ*2);

   uint64_t dstw = (uint64_t)(uint32_t)dst3d;
   uint64_t srcw = (uint64_t)(uint32_t)src3d;
   if (wide) {}

   idma_init(0, MAX_BLOCK_2, 16, TICK_CYCLES_8, 100000, idmaErrCB);

   DPRINT( "Setting IDMA request %p (1 descriptors)\n", task2);
   if (wide) {
#if (IDMA_USE_WIDE_API > 0)
     idma_add_3d_desc64_wide(task2, &dstw, &srcw, 0, XFER_3D_SZ, XFER_3D_SZ, 2, XFER_3D_SZ, XFER_3D_SZ, XFER_3D_SZ*XFER_3D_SZ, XFER_3D_SZ*XFER_3D_SZ);
#endif
   }
   else {
     idma_add_3d_desc64(task2, &dstw, &srcw, 0, XFER_3D_SZ, XFER_3D_SZ, 2, XFER_3D_SZ, XFER_3D_SZ, XFER_3D_SZ*XFER_3D_SZ, XFER_3D_SZ*XFER_3D_SZ);

   }
   DPRINT("Schedule IDMA request %p, CB:%p\n", task2, idma_cb_func);

   idma_schedule_task(task2);

   idma_hw_wait_all();

   int ret = compareBuffers_task(task2, src3d, dst3d, XFER_3D_SZ*XFER_3D_SZ*2, NULL, 0 );
   printf(" '%s' %s\n\n", __FUNCTION__, ret ? "FAILED":"PASSED");
   return 0;
}

/* THIS IS THE EXAMPLE, main() CALLS IT */
int
test(void* arg, int unused)
{
  memset(row_mask, 0xF, PRED_MASK_SIZE);
  idma_log_handler(idmaLogHander);

  test_task_copy_1d(0);
  test_task_copy_2d(0, NULL);
  test_task_copy_2d(0, row_mask);
  test_task_copy_3d(0);
#if (IDMA_USE_WIDE_API > 0)
  test_task_copy_1d(1);
  test_task_copy_2d(1, NULL);
  test_task_copy_2d(1, row_mask);
  test_task_copy_3d(1);
#endif
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
