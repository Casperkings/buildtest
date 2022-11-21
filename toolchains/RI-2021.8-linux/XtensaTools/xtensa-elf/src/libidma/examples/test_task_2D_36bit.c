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
 * Example: test 36-bit special 2-D DMA transfer. This requires copying to a
   bus address > 32 bits and then copying back to local memory. The core cannot
   directly access memory at higher than 4G address. Works on ISS, RTL may
   require special setup.
 */

// Define this to enable the 36-bit API. 
#define IDMA_USE_WIDE_ADDRESS_COMPILE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <xtensa/hal.h>

#include "idma_tests.h"


#define ROW_SIZE            32
#define NUM_ROWS            32
#define SRC_PITCH           128
#define DST_PITCH           256
#define IDMA_IMAGE_SIZE     IDMA_SIZE(NUM_ROWS * ROW_SIZE)


// Define 2 tasks with one desc each. The first will copy localmem to
// sysmem, the second will copy back sysmem to localmem.
IDMA_BUFFER_DEFINE(task1, 1, IDMA_2D_DESC);
IDMA_BUFFER_DEFINE(task2, 1, IDMA_2D_DESC);

// Source buffer and final destination buffer in local memory,
// intermediate buffer in system memory.
ALIGNDCACHE char src1[SRC_PITCH * NUM_ROWS] IDMA_DRAM;
ALIGNDCACHE char dst1[DST_PITCH * NUM_ROWS];
ALIGNDCACHE char dst2[DST_PITCH * NUM_ROWS] IDMA_DRAM;


void
idma_cb_func (void * data)
{
  DPRINT("CALLBACK: Copy request for task @ %p is done\n", data);
}


// This function assumes both src and dst are in local memory.
void
compareBuffers_task(idma_buffer_t * task, char * src, char * dst, int size)
{
  int i;
  int status = idma_task_status(task);

  DPRINT("Task %p status: %s\n",
         task,
         (status == IDMA_TASK_DONE)    ? "DONE"    :
         (status > IDMA_TASK_DONE)     ? "PENDING" :
         (status == IDMA_TASK_ABORTED) ? "ABORTED" :
         (status == IDMA_TASK_ERROR)   ? "ERROR"   : "UNKNOWN");

  if (status == IDMA_TASK_ERROR) {
    idma_error_details_t * error = idma_error_details(IDMA_CH_ARG_SINGLE);

    printf("COPY FAILED, Error 0x%x at desc:%p, PIF src/dst=%x/%x\n",
           error->err_type, (void*)error->currDesc, error->srcAddr, error->dstAddr);
    return;
  }

  if (!src || !dst)
    return;

  // Compare must skip the pitch for src and dst.
  for (i = 0 ; i < size; i++) {
    int src_i = (SRC_PITCH) * (i / ROW_SIZE)  + i % ROW_SIZE;
    int dst_i = (DST_PITCH) * (i / ROW_SIZE)  + i % ROW_SIZE;

    if (dst[dst_i] != src[src_i]) {
      printf("COPY FAILED @ byte %d (dst:%x, src:%x)!!\n", i, dst[dst_i], src[src_i]);
      return;
    }
  }
  printf("COPY OK (src:%p, dst:%p)\n", src, dst);
}


// Test function.
int
test(void * arg, int unused)
{
  // Construct wide pointers for the 3 buffers. Note that the addresses for the
  // two buffers in local memory must not have any of the high bits set or else
  // they'll look like sysmem addresses.
  uint64_t dst1_wide = ((uint64_t)2 << 32) | (uint32_t)&dst1;
  uint64_t src1_wide = ((uint64_t)0 << 32) | (uint32_t)&src1;
  uint64_t dst2_wide = ((uint64_t)0 << 32) | (uint32_t)&dst2;

  idma_log_handler(idmaLogHander);
  idma_init(IDMA_CH_ARG  0, MAX_BLOCK_2, 16, TICK_CYCLES_2, 100000, idmaErrCB);

  bufRandomize(src1, SRC_PITCH * NUM_ROWS);
  bufRandomize(dst2, DST_PITCH * NUM_ROWS);

  idma_init_task(IDMA_CH_ARG  task1, IDMA_2D_DESC, 1, idma_cb_func, task1);
  idma_init_task(IDMA_CH_ARG  task2, IDMA_2D_DESC, 1, idma_cb_func, task2);

  // Note pitch settings for the two descriptors.
  idma_add_2d_desc(task1, &dst1_wide, &src1_wide, ROW_SIZE, DESC_NOTIFY_W_INT, NUM_ROWS, SRC_PITCH, DST_PITCH); 
  idma_add_2d_desc(task2, &dst2_wide, &dst1_wide, ROW_SIZE, DESC_NOTIFY_W_INT, NUM_ROWS, DST_PITCH, DST_PITCH);
  
  DPRINT("Schedule DMA %llx -> %llx\n", src1_wide, dst1_wide);
  DPRINT("Schedule DMA %llx -> %llx\n", dst1_wide, dst2_wide);

  idma_schedule_task(task1);    
  idma_schedule_task(task2); 

  while (idma_task_status(task2) > 0)
    idma_sleep(IDMA_CH_ARG_SINGLE  );

  compareBuffers_task(task1, src1, dst2, IDMA_IMAGE_SIZE);

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

