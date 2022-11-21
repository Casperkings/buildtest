/* Copyright (c) 2003-2015 Cadence Design Systems, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef __XMEM_MGR_H__
#define __XMEM_MGR_H__

#include <stddef.h>
#include <stdint.h>
#include "xmem.h"

/* Stack memory manager
 *
 * This provides a simple stack based allocation of buffers from a user 
 * provided memory pool. Freeing of the allocated buffers is not supported. 
 * The expected use case is to perform buffer allocation from the pool
 * and free up the entire pool when all of the allocated items are
 * no longer required. The allocation is performed linearly
 * from the start of the user provided memory pool. A current allocation pointer
 * is maintained that starts at the beginning of the pool. For every request,
 * the current pointer is adjusted to the requested alignment. If the requested
 * size + the current pointer is within the bounds of the memory
 * pool, allocation succeeds and the current pointer is returned. If not, the 
 * allocation fails and returns NULL. Check pointing and restoring of the stack
 * state is provided that allows freeing the most recently allocated contents.
 */
struct xmem_stack_mgr_struct {
  volatile void *_lock;         // user provided lock
  void          *_buffer;       // user provided pool
  size_t         _buffer_size;  // number of bytes in the pool
  uintptr_t      _buffer_p;     // track allocation
  uintptr_t      _buffer_end;   // end of buffer
  size_t         _unused_bytes; // unused bytes in the pool
};

/* Stack memory manager checkpoint
 *
 * This records the current state of the stack and is used to restore
 * the stack to this state
 */
struct xmem_stack_mgr_checkpoint_struct {
  uintptr_t _buffer_p;     // track allocation
  size_t    _unused_bytes; // unused bytes in the pool
};

/* Heap memory manager
 *
 * This scheme supports both allocation and free of buffers from a user 
 * provided memory pool. The allocated and free blocks are maintained as a
 * list that gets dynamically populated based on the allocation request.
 * A block array that holds book-keeping information for each 
 * allocated/free block is maintained outside the pool. At initialization, the
 * number of such blocks needs to be provided (else, a default of
 * XMEM_DEFAULT_NUM_BLOCKS is assumed).  The user can either provide a 
 * separate buffer space for maintaining the block array. If not, part of the 
 * pool is dedicted to maintaining the block array and this space is not 
 * available for allocation. 
 */

/* Struct to maintain list of free/allocated blocks */
typedef struct xmem_heap_block_struct {
  /* next block in the list */
  struct xmem_heap_block_struct *_next_block;     // next block in the list
  uint32_t                       _block_size;     // size of the block
  void                          *_buffer;         // ptr to the buffer
  void                          *_aligned_buffer; // aligned ptr to buffer
} xmem_heap_block_t;

struct xmem_heap_mgr_struct {
  volatile void     *_lock;                // user provided lock
  void              *_buffer;              // user provided pool for allocation
  uint32_t           _buffer_size;         // num bytes in pool
  uint32_t           _free_bytes;          // free bytes in the heap
  uint32_t           _allocated_bytes;     // allocated bytes in the heap
  uint32_t           _unused_bytes;        // unused bytes in the heap
  xmem_heap_block_t  _free_list_head;      // sentinel for the free list
  xmem_heap_block_t  _free_list_tail;      // sentinel for the free list
  xmem_heap_block_t  _alloc_list_head;     // sentinel for the allocated list
  xmem_heap_block_t *_blocks;              // list of blocks
  uint32_t           _num_blocks;          // number of blocks 
  uint32_t          *_block_free_bitvec;   // bitvec to mark free/alloc blocks
  uint16_t           _header_size;         // size of header in bytes
  uint16_t           _has_external_header; // is the block info part of buffer
};

#define XMEM_HEAP_DEFAULT_NUM_BLOCKS      (64)
#define XMEM_HEAP_MIN_ALLOC_SIZE          (4)
#define XMEM_HEAP_BLOCK_STRUCT_SIZE_LOG2  (4)

#endif // __XMEM_MGR_H__
