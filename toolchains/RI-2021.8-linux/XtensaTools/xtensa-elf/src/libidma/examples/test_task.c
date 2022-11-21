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
   Wait for task completion by looping or each task status.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _XOS
# define IDMA_APP_USE_XOS // will use inlined disable/enable int
#endif
#ifdef _XTOS
# define IDMA_APP_USE_XTOS // will use inlined disable/enable int
#endif

#include <xtensa/hal.h>

#include "idma_tests.h"

#define IMAGE_CHUNK_SIZE	200
#define IDMA_XFER_SIZE		IDMA_SIZE(IMAGE_CHUNK_SIZE)
#define IDMA_IMAGE_SIZE		IMAGE_CHUNK_SIZE*500

void
idma_cb_func (void* data)
{
  DPRINT("CALLBACK: Copy request for task @ %p is done. Outstanding: %d\n", data, idma_task_status((idma_buffer_t*)data));
}


void compareBuffers_task(idma_buffer_t* task, char* src, char* dst, int size)
{
  int status;

  if (dst && size)
    xthal_dcache_region_invalidate(dst, size);

  status = idma_task_status(task);
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

  if(src && dst)
    compareBuffers(0, src, dst, size);
}

IDMA_BUFFER_DEFINE(task1, 2, IDMA_1D_DESC);
IDMA_BUFFER_DEFINE(task2, 5, IDMA_1D_DESC);
IDMA_BUFFER_DEFINE(taskE, 1, IDMA_1D_DESC);
IDMA_BUFFER_DEFINE(task3, 1, IDMA_1D_DESC);
IDMA_BUFFER_DEFINE(task4, 2, IDMA_1D_DESC);

ALIGNDCACHE char dst1_1[IDMA_XFER_SIZE];
ALIGNDCACHE char src1_1[IDMA_XFER_SIZE] IDMA_DRAM;
ALIGNDCACHE char dst1_2[IDMA_XFER_SIZE];
ALIGNDCACHE char src1_2[IDMA_XFER_SIZE] IDMA_DRAM;
ALIGNDCACHE char dst1_3[IDMA_XFER_SIZE];
ALIGNDCACHE char src1_3[IDMA_XFER_SIZE] IDMA_DRAM;

ALIGNDCACHE char dstE[IDMA_XFER_SIZE];
ALIGNDCACHE char srcE[IDMA_XFER_SIZE];

ALIGNDCACHE char dst2_1[IDMA_XFER_SIZE];
ALIGNDCACHE char src2_1[IDMA_XFER_SIZE] IDMA_DRAM;
ALIGNDCACHE char dst2_2[IDMA_XFER_SIZE];
ALIGNDCACHE char src2_2[IDMA_XFER_SIZE] IDMA_DRAM ;
ALIGNDCACHE char dst2_3[IDMA_XFER_SIZE];
ALIGNDCACHE char src2_3[IDMA_XFER_SIZE] IDMA_DRAM;
ALIGNDCACHE char dst2_4[IDMA_XFER_SIZE];
ALIGNDCACHE char src2_4[IDMA_XFER_SIZE] IDMA_DRAM;

ALIGNDCACHE char dst3[IDMA_XFER_SIZE];
ALIGNDCACHE char src3[IDMA_XFER_SIZE] IDMA_DRAM;

ALIGNDCACHE char dst4_1[IDMA_XFER_SIZE];
ALIGNDCACHE char src4_1[IDMA_XFER_SIZE] IDMA_DRAM;
ALIGNDCACHE char dst4_2[IDMA_XFER_SIZE];
ALIGNDCACHE char src4_2[IDMA_XFER_SIZE] IDMA_DRAM;


/* THIS IS THE EXAMPLE, main() CALLS IT */
int
test(void* arg, int unused)
{
    idma_status_t ret;

    bufRandomize(src1_1, IDMA_XFER_SIZE);
    bufRandomize(src1_2, IDMA_XFER_SIZE);
    bufRandomize(src1_3, IDMA_XFER_SIZE);
    bufRandomize(srcE,   IDMA_XFER_SIZE);
    bufRandomize(src2_1, IDMA_XFER_SIZE);
    bufRandomize(src2_2, IDMA_XFER_SIZE);
    bufRandomize(src2_3, IDMA_XFER_SIZE);
    bufRandomize(src2_4, IDMA_XFER_SIZE);
    bufRandomize(src3,   IDMA_XFER_SIZE);
    bufRandomize(src4_1, IDMA_XFER_SIZE);
    bufRandomize(src4_1, IDMA_XFER_SIZE);

    idma_log_handler(idmaLogHander);
    idma_init(IDMA_CH_ARG  0, MAX_BLOCK_2, 16, TICK_CYCLES_8, 100000, idmaErrCB);

    idma_init_task(IDMA_CH_ARG  task1, IDMA_1D_DESC, 2, idma_cb_func, task1);
    DPRINT( "Setting IDMA request %p (2 descriptors)\n", task1);
    idma_add_desc(task1, dst1_1, src1_1, IDMA_XFER_SIZE,  0 );
    idma_add_desc(task1, dst1_2, src1_2, IDMA_XFER_SIZE,  DESC_NOTIFY_W_INT );

    DPRINT( "Setting IDMA request %p (4 descriptors)\n", task2);
    idma_init_task(IDMA_CH_ARG  task2, IDMA_1D_DESC, 4, idma_cb_func, task2);
    idma_add_desc(task2, dst2_1, src2_1, IDMA_XFER_SIZE, 0 );
    idma_add_desc(task2, dst2_2, src2_2, IDMA_XFER_SIZE, 0 );
    idma_add_desc(task2, dst2_3, src2_3, IDMA_XFER_SIZE, 0 );
    idma_add_desc(task2, dst2_4, src2_4, IDMA_XFER_SIZE, DESC_NOTIFY_W_INT);

    DPRINT("Setting IDMA request %p (2 descriptors)\n", task4);
    idma_init_task(IDMA_CH_ARG  task4, IDMA_1D_DESC, 2, idma_cb_func, task4);
    idma_add_desc(task4, dst4_1, src4_1, IDMA_XFER_SIZE, 0);
    idma_add_desc(task4, dst4_2, src4_2, IDMA_XFER_SIZE, DESC_NOTIFY_W_INT );
  
    DPRINT("Schedule IDMA request %p, CB:%p\n", task1, idma_cb_func);
    DPRINT("Schedule IDMA request %p, CB:%p\n", task2, idma_cb_func);
    DPRINT("Schedule IDMA request %p, CB:%p\n", taskE, idma_cb_func);  
    DPRINT("Schedule IDMA request %p, CB:%p\n", task4, idma_cb_func);

    ret = idma_schedule_task(task1);
    if (ret != IDMA_OK) {
        printf("FAILED to schedule task1: %d\n", ret);
        exit(-1);
    }
    ret = idma_schedule_task(task2);
    if (ret != IDMA_OK) {
        printf("FAILED to schedule task2: %d\n", ret);
        exit(-1);
    }

    // The copy should fail, but this function should return OK.
    ret = idma_copy_task(IDMA_CH_ARG  taskE, dstE, srcE, IDMA_XFER_SIZE, DESC_NOTIFY_W_INT, taskE, idma_cb_func);
    if (ret != IDMA_OK) {
        printf("FAILED to schedule taskE: %d\n", ret);
        exit(-1);
    }

    // This call may or may not fail, depending on whether the previous task has
    // made it to the hardware. 'task4' should end up aborted or not scheduled.
    idma_schedule_task(task4);

    // Wait until everything settles.
    while (idma_task_status(task4) > 0) {
        idma_sleep(IDMA_CH_ARG_SINGLE );
    }

    compareBuffers_task(task1, src1_1, dst1_1, IDMA_XFER_SIZE );
    compareBuffers_task(task1, src1_2, dst1_2, IDMA_XFER_SIZE );
    compareBuffers_task(task2, src2_1, dst2_1, IDMA_XFER_SIZE );
    compareBuffers_task(task2, src2_2, dst2_2, IDMA_XFER_SIZE );
    compareBuffers_task(task2, src2_3, dst2_3, IDMA_XFER_SIZE );  

    // Check task4 is ABORTED or NOT scheduled
    compareBuffers_task(task4, NULL, NULL, IDMA_XFER_SIZE );

    // Init the channel again, this should clear previous error.
    idma_init(IDMA_CH_ARG  0, MAX_BLOCK_2, 16, TICK_CYCLES_8, 100000, idmaErrCB);

    idma_init_task(IDMA_CH_ARG  task4, IDMA_1D_DESC, 2, idma_cb_func, task4);
    idma_add_desc(task4, dst4_1, src4_1, IDMA_XFER_SIZE, 0);
    idma_add_desc(task4, dst4_2, src4_2, IDMA_XFER_SIZE, DESC_NOTIFY_W_INT );

    ret = idma_copy_task(IDMA_CH_ARG  task3, dst3, src3, IDMA_XFER_SIZE, 0, task3, idma_cb_func);
    if (ret != IDMA_OK) {
        printf("FAILED to schedule task3: %d\n", ret);
        exit(-1);
    }
    ret = idma_schedule_task(task4);
    if (ret != IDMA_OK) {
        printf("FAILED to schedule task4: %d\n", ret);
        exit(-1);
    }
    idma_process_tasks(IDMA_CH_ARG_SINGLE  );

    compareBuffers_task(task3, src3, dst3, IDMA_XFER_SIZE );
    compareBuffers_task(task4, src4_1, dst4_1, IDMA_XFER_SIZE );
    compareBuffers_task(task4, src4_2, dst4_2, IDMA_XFER_SIZE );

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

