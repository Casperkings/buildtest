/*
 * Copyright (c) 2020 by Cadence Design Systems. ALL RIGHTS RESERVED.
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

/* XOS example using idma task mode API. Creates 3 threads which schedule their
   own sets of DMA transfers using per-thread task buffers.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <xtensa/hal.h>
#include <xtensa/xos.h>

#define  IDMA_USE_MULTICHANNEL

#include "idma_tests.h"

#ifndef CH
#define CH              0    // idma channel number
#endif
#define STACK_SIZE      (XOS_STACK_MIN_SIZE + 0x800)
#define NUM_ITER        1000

#define SIZE_1D         1024
#define SIZE_2D         8192

#ifndef NO_INTS
#define NO_INTS         0    // 0 -> use completion ints, 1 -> don't use ints
#endif


static inline void
halt(void)
{
  exit(-1);
}

static void
idma_cb(void * data)
{
  //puts("cb");
}

//-----------------------------------------------------------------------------
// test1
//-----------------------------------------------------------------------------

XosThread tcb1;
char      stack1[STACK_SIZE];

// Task buffer.
IDMA_BUFFER_DEFINE(buffer1, 4, IDMA_1D_DESC);

// Source buffer, one transfer size.
char test1_srcbuf[SIZE_1D] IDMA_DRAM;

// Destination buffer.
ALIGNDCACHE char test1_dstbuf[SIZE_1D * 4];


// test1 - schedules 4 descriptors at a time, sleeps on completion
// if possible else polls.
int32_t
test1(void * arg, int32_t unused)
{
  int32_t  i;
  int32_t  ret;
  int32_t  val = 0xdeadbeef;
#if NO_INTS
  uint32_t desc_opts = 0;
#else
  uint32_t desc_opts = DESC_NOTIFY_W_INT | DESC_IDMA_PRIOR_H;
#endif

  // Init task buffer for this thread.
#if NO_INTS
  ret = idma_init_task(CH, buffer1, IDMA_1D_DESC, 4, idma_cb, (void *)xos_thread_id());
#else
  ret = idma_init_task(CH, buffer1, IDMA_1D_DESC, 4, NULL, NULL);
#endif
  if (ret != IDMA_OK) {
    printf("Error task init test1 (%d)\n", ret);
    halt();
  }

  // Generate source pattern.
  for (ret = 0; ret < SIZE_1D/4; ret++) {
    ((int32_t *) test1_srcbuf)[ret] = val;
  }

  for (i = 0; i < (int32_t)arg; i++) {
    // Clear the destination buffer to prep for next transfer.
    memset(test1_dstbuf, 0, SIZE_1D * 4);
    xthal_dcache_region_writeback_inv(test1_dstbuf, SIZE_1D * 4);

    // Schedule transfers.
    idma_add_desc(buffer1, test1_dstbuf, test1_srcbuf, SIZE_1D,  desc_opts );
    idma_add_desc(buffer1, test1_dstbuf + (1 * SIZE_1D), test1_srcbuf, SIZE_1D,  desc_opts );
    idma_add_desc(buffer1, test1_dstbuf + (2 * SIZE_1D), test1_srcbuf, SIZE_1D,  desc_opts );
    idma_add_desc(buffer1, test1_dstbuf + (3 * SIZE_1D), test1_srcbuf, SIZE_1D,  desc_opts );

    ret = idma_schedule_task(buffer1);
    if (ret != IDMA_OK) {
      printf("Error scheduling task test1 (%d)\n", ret);
      halt();
    }

    // Wait for completion.
    idma_sleep(CH);
    while ((ret = idma_task_status(buffer1)) > 0) {
      idma_process_tasks(CH);
    }
    if (ret < 0) {
      printf("Error waiting for test1\n");
      halt();
    }

    // Check the transfer results.
    for (ret = 0; ret < 4; ret++) {
      if (memcmp(test1_srcbuf, test1_dstbuf + (ret*SIZE_1D), SIZE_1D) != 0) {
        printf("Error test1 block %d\n", ret);
        halt();
      }
    }
    puts("test1 OK");
  }

  return 0;
}

//-----------------------------------------------------------------------------
// test2
//-----------------------------------------------------------------------------

XosThread tcb2;
char      stack2[STACK_SIZE];

// Task buffer.
IDMA_BUFFER_DEFINE(buffer2, 4, IDMA_1D_DESC);

// Source buffer, one transfer size.
char test2_srcbuf[SIZE_1D] IDMA_DRAM;

// Destination buffer.
ALIGNDCACHE char test2_dstbuf[SIZE_1D * 4];


// test2 - schedules 4 descriptors at a time, sleeps on completion
// if possible else polls.
int32_t
test2(void * arg, int32_t unused)
{
  int32_t  i;
  int32_t  ret;
  int32_t  val       = 5;
#if NO_INTS
  uint32_t desc_opts = 0;
#else
  uint32_t desc_opts = DESC_NOTIFY_W_INT | DESC_IDMA_PRIOR_L;
#endif

  // Init descriptor buffer for this thread.
#if NO_INTS
  ret = idma_init_task(CH, buffer2, IDMA_1D_DESC, 4, idma_cb, (void *)xos_thread_id());
#else
  ret = idma_init_task(CH, buffer2, IDMA_1D_DESC, 4, NULL, NULL);
#endif
  if (ret != IDMA_OK) {
    printf("Error init task test2 (%d)\n", ret);
    halt();
  }

  for (i = 0; i < (int32_t)arg; i++) {
    // Sleep to let other threads run and schedule transfers.
    xos_thread_sleep(20000);

    // Generate source pattern.
    memset(test2_srcbuf, val, SIZE_1D);

    // Schedule transfers.
    idma_add_desc(buffer2, test2_dstbuf, test2_srcbuf, SIZE_1D, 0);
    idma_add_desc(buffer2, test2_dstbuf + (1 * SIZE_1D), test2_srcbuf, SIZE_1D, 0);
    idma_add_desc(buffer2, test2_dstbuf + (2 * SIZE_1D), test2_srcbuf, SIZE_1D, 0);
    idma_add_desc(buffer2, test2_dstbuf + (3 * SIZE_1D), test2_srcbuf, SIZE_1D, desc_opts);

    ret = idma_schedule_task(buffer2);
    if (ret != IDMA_OK) {
      printf("Error scheduling task test2 (%d)\n", ret);
      halt();
    }

    // Wait for completion.
    idma_sleep(CH);
    while ((ret = idma_task_status(buffer2)) > 0) {
      idma_process_tasks(CH);
    }
    if (ret < 0) {
      printf("Error waiting for test2\n");
      halt();
    }

    // Check the transfer results.
    xthal_dcache_region_invalidate(test2_dstbuf, 4 * SIZE_1D);
    for (ret = 0; ret < 4; ret++) {
      if (memcmp(test2_srcbuf, test2_dstbuf + (ret*SIZE_1D), SIZE_1D) != 0) {
        printf("Error test2 block %d\n", ret);
        halt();
      }
    }
    puts("test2 OK");
    val += 5;
  }

  return 0;
}

//-------------------------------------------------------------------------
// test_2d. Run 2D transfers. Expects malloc() to allocate in system memory.
//-------------------------------------------------------------------------

#define SIZE_2D                 8192

// TCB and thread stack.
XosThread tcb3;
char      stack3[STACK_SIZE];

// Task buffer.
IDMA_BUFFER_DEFINE(buf2d, 1, IDMA_2D_DESC);

// Source buffer for 2D transfers (in dataram).
char srcbuf_2d[32] IDMA_DRAM;

// Destination buffer.
char * dstbuf_2d;

// test2d - schedules one 2D desc at a time. Does not use interrupts.
int32_t
test_2d(void * arg, int32_t unused)
{
  int32_t ret;
  volatile int32_t i;
  char *  p;

  // Init task buffer for this thread's 2D transfers.
  ret = idma_init_task(CH, buf2d, IDMA_2D_DESC, 1, NULL, NULL);
  if (ret != IDMA_OK) {
    printf("Error init task test2d (%d)\n", ret);
    halt();
  }

  for (i = 0; i < (int32_t)arg/2; i++) {
    // Allocate and align dest buffer.
    p = malloc(SIZE_2D + IDMA_DCACHE_ALIGN);
    dstbuf_2d = (char *)(((int32_t)p + IDMA_DCACHE_ALIGN - 1) & -IDMA_DCACHE_ALIGN);

    // Clear the destination buffer to prep for next transfer.
    memset(dstbuf_2d, 0, SIZE_2D);
    xthal_dcache_region_writeback_inv(dstbuf_2d, SIZE_2D);

    // Generate source pattern.
    memset(srcbuf_2d, i, 32);

    // Schedule the transfer.
    ret = idma_copy_2d_task(CH, buf2d, dstbuf_2d, srcbuf_2d, 32, 0, SIZE_2D/32, 0, 32, (void *)xos_thread_id(), idma_cb);
    if (ret != IDMA_OK) {
      printf("Error scheduling task test2d (%d)\n", ret);
      halt();
    }

    // Wait for completion.
    idma_sleep(CH);
    while ((ret = idma_task_status(buf2d)) > 0) {
      idma_process_tasks(CH);
    }
    if (ret < 0) {
      printf("Error waiting for 2D transfer\n");
      halt();
    }

    // Check result.
    for (ret = 0; ret < SIZE_2D/32; ret++) {
      if (memcmp(srcbuf_2d, dstbuf_2d + (ret*32), 32) != 0) {
        printf("Data mismatch in 2D transfer, block %d\n", ret);
        halt();
      }
    }

    printf("2D transfer OK (%d)\n", i);
    free(p);
  }

  // Wait for other threads to finish.
  xos_thread_join(&tcb1, &ret);
  xos_thread_join(&tcb2, &ret);

  // Terminate test.
  printf("PASS\n");
  exit(0);
}

#if NO_INTS
//-------------------------------------------------------------------------
// Lowest priority thread. Keeps the idma processing going while everyone
// else is asleep.
//-------------------------------------------------------------------------

// TCB and thread stack.
XosThread tcb4;
char      stack4[STACK_SIZE];

int32_t
idma_kick(void * arg, int32_t unused)
{
  while (1) {
    xos_preemption_disable();
    idma_process_tasks(CH);
    xos_preemption_enable();
  }

  return 0;
}
#endif

int
main (int argc, char**argv)
{
  int32_t ret = 0;
  int32_t count;

#if !NO_INTS && XCHAL_HAVE_XEA2 && (XCHAL_INT_LEVEL(XCHAL_IDMA_CH0_DONE_INTERRUPT) > XCHAL_EXCM_LEVEL)
  printf("XOS does not support idma interrupts at > EXCM_LEVEL, skip\n");
  printf("PASS\n");
  return 0;
#endif

  if (argc > 1) {
    count = atoi(argv[1]);
  }
  else {
    count = NUM_ITER;
  }

  // Disable prefetch. Else the prefetcher will bring in stale data from idma
  // buffers between the invalidate and the read, causing test failures.
  xthal_set_cache_prefetch_long(0);

  xos_set_clock_freq(XOS_CLOCK_FREQ);

  ret = xos_thread_create(&tcb1,
                          0,
                          test1,
                          (void *)count,
                          "test1",
                          stack1,
                          STACK_SIZE,
                          6,
                          0,
                          0);
  if (ret != XOS_OK) {
    printf("Error creating thread 1 : %d\n", ret);
    halt();
  }

  ret = xos_thread_create(&tcb2,
                          0,
                          test2,
                          (void *)count,
                          "test2",
                          stack2,
                          STACK_SIZE,
                          7,
                          0,
                          0);
  if (ret != XOS_OK) {
    printf("Error creating thread 2 : %d\n", ret);
    halt();
  }

  ret = xos_thread_create(&tcb3,
                          0,
                          test_2d,
                          (void *)count,
                          "test_2d",
                          stack3,
                          STACK_SIZE,
                          6,
                          0,
                          0);
  if (ret != XOS_OK) {
    printf("Error creating thread 3 : %d\n", ret);
    halt();
  }

#if NO_INTS
  ret = xos_thread_create(&tcb4,
                          0,
                          idma_kick,
                          NULL,
                          "idma_kick",
                          stack4,
                          STACK_SIZE,
                          1,
                          0,
                          0);
  if (ret != XOS_OK) {
    printf("Error creating thread kick : %d\n", ret);
    halt();
  }
#endif

  idma_init(CH, IDMA_OCD_HALT_ON, MAX_BLOCK_2, 16, TICK_CYCLES_8, 100000, idmaErrCB);
  idma_log_handler(idmaLogHander);

  xos_start_system_timer(-1, XOS_CLOCK_FREQ/100);
  xos_start(0);
  return ret;
}

