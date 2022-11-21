
// xos_blockmem.h - XOS block memory pool manager.

// Copyright (c) 2015-2020 Cadence Design Systems, Inc.
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


#define XOS_INCLUDE_INTERNAL
#include "xos.h"


#define XOS_BLOCKPOOL_SIG   0x706F6F6CU     // Signature of block pool
#define XOS_BLOCKMEM_SIG    0x66726565U     // Signature of free block


//-----------------------------------------------------------------------------
//  Initialize the memory pool.
//-----------------------------------------------------------------------------
int32_t
xos_block_pool_init(XosBlockPool * pool,
                    void *         mem,
                    uint32_t       blocksize,
                    uint32_t       nblocks,
                    uint32_t       flags)
{
    uint32_t * ptr;
    int32_t    ret;
    uint32_t   i;

    // Check input parameters.
    if ((pool == XOS_NULL) || (mem == XOS_NULL) || (blocksize == 0U) || (nblocks == 0U)) {
        return XOS_ERR_INVALID_PARAMETER;
    }
    if ((blocksize % 4U) != 0U) {
        return XOS_ERR_INVALID_PARAMETER;
    }
    // Limited by semaphore.
    if (nblocks > (uint32_t) INT32_MAX) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    // Build the free block list.
    ptr = xos_voidp_to_uint32p(mem);
    for (i = 0; i < (nblocks - 1U); i++) {
        ptr[0] = xos_uint32p_to_uint32(ptr) + blocksize;
        ptr[1] = XOS_BLOCKMEM_SIG;
        ptr = xos_uint32_to_uint32p(ptr[0]);
    }
    ptr[0] = 0;
    ptr[1] = XOS_BLOCKMEM_SIG;

    // Init the pool object.
    pool->head  = xos_voidp_to_uint32p(mem);
    pool->first = xos_voidp_to_uint32p(mem);
    pool->last  = ptr;
    pool->nblks = nblocks;
    pool->flags = flags;
    pool->sig   = XOS_BLOCKPOOL_SIG;

    // Create the pool semaphore with correct properties and count.
    ret = xos_sem_create(&(pool->sem),
                         ((flags & XOS_BLOCKMEM_WAIT_FIFO) != 0U) ? XOS_SEM_WAIT_FIFO : XOS_SEM_WAIT_PRIORITY,
                         (int32_t) nblocks);
    if (ret != XOS_OK) {
        return ret;
    }

#if XOS_OPT_STATS
    pool->num_allocs = 0;
    pool->num_frees  = 0;
    pool->num_waits  = 0;
#endif

    return XOS_OK;
}


//-----------------------------------------------------------------------------
//  Allocate a single block from the pool, suspend until available.
//-----------------------------------------------------------------------------
void *
xos_block_alloc(XosBlockPool * pool)
{
    uint32_t * ptr;
    int32_t    ret;
    uint32_t   ps;

    // Can't call this from interrupt context.
    if (INTERRUPT_CONTEXT) {
        xos_fatal_error(XOS_ERR_INTERRUPT_CONTEXT, XOS_NULL);
    }

    if ((pool == XOS_NULL) || (pool->sig != XOS_BLOCKPOOL_SIG)) {
        return XOS_NULL;
    }

    // Decrement the semaphore. We'll block here if no free memory.
#if XOS_OPT_STATS
    if (xos_sem_test(&(pool->sem)) == 0) {
        pool->num_waits++;
    }
#endif
    ret = xos_sem_get(&(pool->sem));
    if (ret != XOS_OK) {
        return XOS_NULL;
    }

    // Once we have decremented the semaphore, we are guaranteed to find a
    // free memory block. Lock out interrupts while we update the free list.
    ps = xos_critical_enter();

    ptr = pool->head;
    XOS_ASSERT(ptr != XOS_NULL);
    pool->head = xos_uint32_to_uint32p(ptr[0]);
#if XOS_OPT_STATS
    pool->num_allocs++;
#endif

    xos_critical_exit(ps);
    return ptr;
}


//-----------------------------------------------------------------------------
//  Nonblocking version of xos_block_alloc(). Returns immediately on failure.
//-----------------------------------------------------------------------------
void *
xos_block_try_alloc(XosBlockPool * pool)
{
    uint32_t * ptr;
    int32_t    ret;
    uint32_t   ps;

    if ((pool == XOS_NULL) || (pool->sig != XOS_BLOCKPOOL_SIG)) {
        return XOS_NULL;
    }

    // Try to decrement the semaphore.
    ret = xos_sem_tryget(&(pool->sem));
    if (ret != XOS_OK) {
        return XOS_NULL;
    }

    // Once we have decremented the semaphore, we are guaranteed to find a
    // free memory block. Lock out interrupts while we update the free list.
    ps = xos_critical_enter();

    ptr = pool->head;
    XOS_ASSERT(ptr != XOS_NULL);
    pool->head = xos_uint32_to_uint32p(ptr[0]);
#if XOS_OPT_STATS
    pool->num_allocs++;
#endif

    xos_critical_exit(ps);
    return ptr;
}


//-----------------------------------------------------------------------------
//  Free a single block back to the pool.
//-----------------------------------------------------------------------------
int32_t
xos_block_free(XosBlockPool * pool, void * mem)
{
    uint32_t * ptr = xos_voidp_to_uint32p(mem);
    uint32_t   ps;

    if ((pool == XOS_NULL) || (pool->sig != XOS_BLOCKPOOL_SIG) || (mem == XOS_NULL)) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    // Pointer must belong to pool.
    if ((ptr < pool->first) || (ptr > pool->last)) {
        return XOS_ERR_ILLEGAL_OPERATION;
    }

    // Lock out interrupts while we update the free list.
    ps = xos_critical_enter();

    ptr[0] = xos_uint32p_to_uint32(pool->head);
    pool->head = ptr;
    ptr[1] = XOS_BLOCKMEM_SIG;
#if XOS_OPT_STATS
    pool->num_frees++;
#endif

    xos_critical_exit(ps);

    // Update the semaphore.
    if (xos_sem_put(&(pool->sem)) != XOS_OK) {
        return XOS_ERR_INTERNAL_ERROR;
    }

    return XOS_OK;
}


//-----------------------------------------------------------------------------
//  Verify that the state of the pool is consistent.
//-----------------------------------------------------------------------------
int32_t
xos_block_pool_check(const XosBlockPool * pool)
{
    uint32_t   cnt;
    uint32_t * p;

    if ((pool == XOS_NULL) || (pool->sig != XOS_BLOCKPOOL_SIG)) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    p   = pool->head;
    cnt = 0;

    while (p != XOS_NULL) {
        if (((xos_uint32p_to_uint32(p)) & 0x3U) != 0U) {
            // Bad pointer, do not use
            return XOS_ERR_INTERNAL_ERROR;
        }
        if (p[1] != XOS_BLOCKMEM_SIG) {
            return XOS_ERR_INTERNAL_ERROR;
        }
        cnt++;
        // Try to make sure loop terminates eventually
        if (cnt > pool->nblks) {
            break;
        }
        p = xos_uint32_to_uint32p(p[0]);
    }

    if ((cnt != (uint32_t) pool->sem.count) || (cnt > pool->nblks)) {
        return XOS_ERR_INTERNAL_ERROR;
    }

#if XOS_OPT_STATS
    if ((pool->num_allocs - pool->num_frees) != (pool->nblks - cnt)) {
        return XOS_ERR_INTERNAL_ERROR;
    }
#endif

    return XOS_OK;
}

