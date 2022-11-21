
// blockmem_test.c - XOS BlockMem API unit test.

// Copyright (c) 2015 Cadence Design Systems, Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


#include "test.h"


#if XCHAL_NUM_TIMERS == 0
#error This test requires at least one timer to be present at < EXCM_LEVEL
#endif


#define TEST_STK_SIZE    (STACK_SIZE + 1000)
#if !USE_MAIN_THREAD
XosThread test_tcb;
uint8_t   test_stk[TEST_STK_SIZE];
#endif
XosThread tcb[3];
uint8_t   stack[3][STACK_SIZE];

#define BLKSIZE          32
#define NBLKS            32

XosBlockPool pool;
uint8_t      pool_mem[BLKSIZE * NBLKS];


//-----------------------------------------------------------------------------
// Timer callback for this test.
//-----------------------------------------------------------------------------
void
timer_cb(void * arg)
{
    int32_t    ret;
    int32_t    i;
    void *     p;
    uint32_t * q = (uint32_t *) arg;

    for (i = 0; i < 10; i++) {
        p = xos_block_try_alloc(&pool);
        if (p == XOS_NULL) {
            printf("Error allocating from intr handler\n");
            exit(-1);
        }

        ret = xos_block_free(&pool, p);
        if (ret != XOS_OK) {
            printf("Error %d freeing block from intr handler\n", ret);
            exit(-1);
        }
    }

    *q = 1;
}


//-----------------------------------------------------------------------------
// Thread function for pool priority test.
//-----------------------------------------------------------------------------
int32_t
thread_func(void * arg, int32_t unused)
{
    uint32_t * p = (uint32_t *) xos_block_alloc(&pool);

    if (p == XOS_NULL) {
        printf("Error: failed to allocate block from thread\n");
        exit(-1);
    }

    p[0] = 0xa5a5a5a5;
    p[1] = 0xdadadada;

    (void) xos_block_free(&pool, p);
    return 0;
}


//-----------------------------------------------------------------------------
// Basic memory pool tests.
//-----------------------------------------------------------------------------
int32_t
mblock_test(void * arg, int32_t unused)
{
    int32_t  ret1, ret2;
    int32_t  ret;
    int32_t  i;
    void *   p;
    void *   ptrs[NBLKS];
    void *   copy[NBLKS];
    XosTimer timer;
    volatile uint32_t tflag = 0;

    // Try to create pool with bad parameters
    ret1 = xos_block_pool_init(0, pool_mem, BLKSIZE, NBLKS, 0);
    ret2 = xos_block_pool_init(&pool, 0, BLKSIZE, NBLKS, 0);
    if ((ret1 == XOS_OK) || (ret2 == XOS_OK)) {
        printf("Error, pool inited with bad params\n");
        exit(-1);
    }

    ret1 = xos_block_pool_init(&pool, pool_mem, 0, NBLKS, 0);
    ret2 = xos_block_pool_init(&pool, pool_mem, BLKSIZE+1, NBLKS, 0);
    if ((ret1 == XOS_OK) || (ret2 == XOS_OK)) {
        printf("Error, pool inited with bad params\n");
        exit(-1);
    }

    ret = xos_block_pool_init(&pool, pool_mem, BLKSIZE, 0, 0);
    if (ret == XOS_OK) {
        printf("Error, pool inited with bad params\n");
        exit(-1);
    }

    // Create the test pool
    ret = xos_block_pool_init(&pool, pool_mem, BLKSIZE, NBLKS, XOS_BLOCKMEM_WAIT_PRIORITY);
    if (ret != XOS_OK) {
        printf("Error %d creating block pool\n", ret);
        exit(-1);
    }

    // Try some bad allocs
    if (xos_block_alloc(NULL) != XOS_NULL) {
        printf("Error: null alloc should have failed\n");
        exit(-1);
    }
    if (xos_block_try_alloc(NULL) != XOS_NULL) {
        printf("Error: null alloc should have failed\n");
        exit(-1);
    }
    if (xos_block_alloc((XosBlockPool *) pool_mem) != XOS_NULL) {
        printf("Error: invalid alloc should have failed\n");
        exit(-1);
    }
    if (xos_block_try_alloc((XosBlockPool *) pool_mem) != XOS_NULL) {
        printf("Error: invalid alloc should have failed\n");
        exit(-1);
    }

    // Allocate all the blocks
    for (i = 0; i < NBLKS; i++) {
        ptrs[i] = xos_block_alloc(&pool);
        if (ptrs[i] == XOS_NULL) {
            printf("Error trying to allocate all blocks\n");
            exit(-1);
        }
        copy[i] = ptrs[i];
    }

    // Verify that next allocation would fail
    p = xos_block_try_alloc(&pool);
    if (p != XOS_NULL) {
        printf("Error: alloc succeeded with no blocks in pool\n");
        exit(-1);
    }

    // Free all the blocks back to the pool
    for (i = 0; i < NBLKS; i++) {
        ret = xos_block_free(&pool, ptrs[i]);
        if (ret != XOS_OK) {
            printf("Error %d trying to free allocated block\n", ret);
            exit(-1);
        }
    }

    // Try to free a NULL pointer
    ret = xos_block_free(&pool, 0);
    if (ret != XOS_ERR_INVALID_PARAMETER) {
        printf("Error retcode %d trying to free NULL pointer\n", ret);
        exit(-1);
    }

    // Try to free a bad pointer
    ret = xos_block_free(&pool, (void *) &pool);
    if (ret != XOS_ERR_ILLEGAL_OPERATION) {
        printf("Error retcode %d trying to free bad pointer\n", ret);
        exit(-1);
    }

    // Allocate a few blocks. When freed, they should have gone into
    // the free list in LIFO order.
    for (i = 0; i < 5; i++) {
        ptrs[i] = xos_block_alloc(&pool);
        if (ptrs[i] != copy[NBLKS - 1 - i]) {
            printf("Error in free block ordering\n");
            exit(-1);
        }
    }

    for (i = 0; i < 5; i++) {
        xos_block_free(&pool, ptrs[i]);
    }

    // Set up an interrupt to check calls from interrupt context.
    xos_timer_init(&timer);
    xos_timer_start(&timer, 20000, 0, timer_cb, (void *)&tflag);
    while (!tflag);

    // If we got here then interrupt test passed. Create multiple
    // threads at different priorities. Have them all block on the
    // pool. Then unblock one at a time and verify that they were
    // released in priority order.

    // First allocate all the blocks in the pool
    for (i = 0; i < NBLKS; i++) {
        ptrs[i] = xos_block_alloc(&pool);
    }

    // Create higher priority threads
    for (i = 0; i < 3; i++) {
        ret = xos_thread_create(&tcb[i], 0, thread_func, 0, "test",
                                stack[i], STACK_SIZE, i+3, 0, 0);
    }

    // Check first one
    xos_block_free(&pool, ptrs[0]);
    if (xos_thread_get_state(&tcb[2]) != XOS_THREAD_STATE_EXITED) {
        printf("Error: highest pri thread did not get alloc\n");
        exit(-1);
    }

    xos_block_free(&pool, ptrs[1]);
    if (xos_thread_get_state(&tcb[1]) != XOS_THREAD_STATE_EXITED) {
        printf("Error: highest pri thread did not get alloc\n");
        exit(-1);
    }

    xos_block_free(&pool, ptrs[2]);
    if (xos_thread_get_state(&tcb[0]) != XOS_THREAD_STATE_EXITED) {
        printf("Error: highest pri thread did not get alloc\n");
        exit(-1);
    }

    // Free remaining blocks
    for (i = 3; i < NBLKS; i++) {
        ret = xos_block_free(&pool, ptrs[i]);
        if (ret != XOS_OK) {
            printf("Error %d trying to free allocated block\n", ret);
            exit(-1);
        }
    }

    ret = xos_block_pool_check(&pool);
    if (ret != XOS_OK) {
        printf("Error pool check failed\n");
        exit(-1);
    }

    // Corrupt an already-freed block
    tflag = ((uint32_t *) ptrs[0])[1];
    ((uint32_t *) ptrs[0])[1] = 0xdeadfeed;
    ret = xos_block_pool_check(&pool);
    if (ret == XOS_OK) {
        printf("Error pool check failed to catch error\n");
        exit(-1);
    }
    ((uint32_t *) ptrs[0])[1] = tflag;

    printf("PASS\n");
#if !USE_MAIN_THREAD
    exit(0);
#endif
    return 0;
}


int32_t
main()
{
    int32_t ret;

#if defined BOARD
    xos_set_clock_freq(xtbsp_clock_freq_hz());
#else
    xos_set_clock_freq(XOS_CLOCK_FREQ);
#endif

    // Auto-select system timer, use dynamic ticks if specified.
    // This can be done before calling xos_start(). The interrupt
    // will not be enabled until xos_start() is called.
#if USE_DYNAMIC_TICKS
    xos_start_system_timer(-1, 0);
#else
    xos_start_system_timer(-1, TICK_CYCLES);
#endif

#if USE_MAIN_THREAD
    xos_start_main_ex("main", 2, TEST_STK_SIZE);
    ret = mblock_test(0, 0);
    return ret;
#else
    ret = xos_thread_create(&test_tcb,
                            0,
                            mblock_test,
                            0,
                            "mblock_test",
                            test_stk,
                            TEST_STK_SIZE,
                            2,
                            0,
                            0);

    xos_start(0);
    return -1; // should never get here
#endif
}

