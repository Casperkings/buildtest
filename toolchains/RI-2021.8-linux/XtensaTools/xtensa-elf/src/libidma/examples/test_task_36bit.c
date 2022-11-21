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
 * Example: test 36-bit special DMA transfer. This requires copying to a bus
   address > 32 bits and then copying back to local memory. The core cannot
   directly access memory at higher than 4G address. Works on ISS, RTL may
   require special setup.
 */

// Define this to enable the 36-bit API.
#define IDMA_USE_WIDE_ADDRESS_COMPILE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <xtensa/hal.h>

#ifdef _XOS
# define IDMA_APP_USE_XOS // will use inlined disable/enable int
#endif
#ifdef _XTOS
# define IDMA_APP_USE_XTOS // will use inlined disable/enable int
#endif

#include "idma_tests.h"


#define IMAGE_CHUNK_SIZE	512
#define IDMA_XFER_SIZE		IDMA_SIZE(IMAGE_CHUNK_SIZE)


void
idma_cb_func(void * data)
{
  DPRINT("CALLBACK: Copy request for task @ %p is done. Outstanding: %d\n",
         data, idma_task_status((idma_buffer_t *) data));
}


// This function assumes both src and dst are in local memory.
void
compareBuffers_task(idma_buffer_t * task, char * src, char * dst, int size)
{
  int status = idma_task_status(task);

  printf("Task %p status: %s\n",
         task,
         (status == IDMA_TASK_DONE)    ? "DONE"    :
         (status  > IDMA_TASK_DONE)    ? "PENDING" :
         (status == IDMA_TASK_ABORTED) ? "ABORTED" :
         (status == IDMA_TASK_ERROR)   ? "ERROR"   : "UNKNOWN");

  if (status == IDMA_TASK_ERROR) {
    idma_error_details_t * error = idma_error_details(IDMA_CH_ARG_SINGLE);

    printf("COPY FAILED, Error 0x%x at desc:%p, PIF src/dst=%x/%x\n",
           error->err_type, (void*)error->currDesc, error->srcAddr, error->dstAddr);
    return;
  }

  if (src && dst) {
    compareBuffers(0, src, dst, size);
  }
}


// Define just one task with 2 descriptors. The first will copy localmem to sysmem,
// the second will copy sysmem to localmem.

IDMA_BUFFER_DEFINE(task1, 2, IDMA_1D_DESC);

ALIGNDCACHE char dst1[IDMA_XFER_SIZE];
ALIGNDCACHE char src1[IDMA_XFER_SIZE] IDMA_DRAM;
ALIGNDCACHE char dst2[IDMA_XFER_SIZE] IDMA_DRAM;


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

  bufRandomize(src1, IDMA_XFER_SIZE);
  bufRandomize(dst1, IDMA_XFER_SIZE);
  bufRandomize(dst2, IDMA_XFER_SIZE);

  idma_log_handler(idmaLogHander);
  idma_init(IDMA_CH_ARG  0, MAX_BLOCK_2, 16, TICK_CYCLES_8, 100000, idmaErrCB);

  idma_init_task(IDMA_CH_ARG  task1, IDMA_1D_DESC, 2, idma_cb_func, task1);
  DPRINT("Setting IDMA request %p (2 descriptors)\n", task1);
  idma_add_desc(task1, &dst1_wide, &src1_wide, IDMA_XFER_SIZE, 0);
  idma_add_desc(task1, &dst2_wide, &dst1_wide, IDMA_XFER_SIZE, DESC_NOTIFY_W_INT);

  DPRINT("Schedule IDMA request %p, CB:%p\n", task1, idma_cb_func);
  idma_schedule_task(task1);

  idma_hw_wait_all(IDMA_CH_ARG_SINGLE);

  compareBuffers_task(task1, dst2, src1, IDMA_XFER_SIZE );

  exit(0);
  return -1;
}

int
main(int argc, char** argv)
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

