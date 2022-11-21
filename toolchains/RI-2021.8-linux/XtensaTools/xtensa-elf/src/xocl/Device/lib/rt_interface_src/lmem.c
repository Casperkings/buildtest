/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED. 
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of 
 * Cadence Design Systems Inc. Any rights to use, modify, and create 
 * derivative works of this file are set forth under the terms of your 
 * license agreement with Cadence Design Systems, Inc.
 */

#include <xtensa/config/core-isa.h>
#include "lmem.h"
#include "misc.h"

// Structure to represent internal memory bank
typedef struct {
  void *orig_start;   // Start address of bank
  void *start;        // Aligned start address of bank
  size_t size;        // Number of bytes
  uint32_t allocated; // Maintain a current allocation offset in bank
} xocl_lmem;

// This provides a wrapper interface to allocate memory used by the 
// device runtime. This pre-allocates memory from the
// local memory bank manager for the banks and manages them internally.

// Pointer to table of external callback functions for local memory bank
// allocation
static xdyn_lib_xmem_bank_callback_if *xmem_bank_cb_if XOCL_LMEM_ATTR;

// If using a stack allocation policy for local memory, check point 
// the local memory bank state for each of the banks
static xmem_bank_checkpoint_t lmem0_checkpoint;
static xmem_bank_checkpoint_t lmem1_checkpoint;

// Has local memory been already initialized ?
static int lmem_initialized = 0;

// Internal manager for the 2 banks. Note we always use 2 banks of local
// memory. If there is a signle DATARAM in the config, it is split into
// lower and upper banks.
static xocl_lmem local_memory_banks[2] XOCL_LMEM_ATTR;

// Align offset to alignment boundary and return the aligned offset
static inline uint32_t get_aligned_offset(uint32_t offset, unsigned alignment) 
{
  uint32_t new_offset = offset + alignment - 1;
  return new_offset - (new_offset & (alignment - 1));
}

// Allocate from the internal local memory manager.
// Returns NULL if fail, else returns pointer to allocated memory
static inline void *
_aligned_alloc_lmem(XOCL_LMEM_ID id, size_t *size, int allow_shrink, 
                    unsigned align) 
{
  size_t alloc_size = *size;
  if (alloc_size == 0)
    return NULL;
  xocl_lmem *lmem = &local_memory_banks[id];
  uint32_t lmem_align_allocated = get_aligned_offset(lmem->allocated, align);
  // No space left
  if (lmem->size <= lmem_align_allocated) {
    XOCL_DPRINTF("_aligned_alloc_lmem: No space left\n", id);
    if (allow_shrink)
      *size = 0;
    return NULL;
  }
  size_t left = lmem->size - lmem_align_allocated;
  if (alloc_size > left) {
    XOCL_DPRINTF("_aligned_alloc_lmem: Exceed allocable lmem%d size - "
                 "remains %u, total required %u\n", id, left, alloc_size);
    if (!allow_shrink)
      return NULL;
    alloc_size = left;
  }

  void *buf = (void *)((uintptr_t)lmem->start + lmem_align_allocated);
  XOCL_DPRINTF("_aligned_alloc_lmem: alloc dram%d: %p size: %d, align: %d, "
               "allocated: %d, new allocated %d, %sleft: %u, new left: %u\n",
               id, buf, alloc_size, align, lmem->allocated, 
               lmem_align_allocated+alloc_size,
               lmem->allocated != lmem_align_allocated ? "align, " : "", left,
               left-alloc_size);
  lmem_align_allocated += alloc_size;
  *size = alloc_size;
  lmem->allocated = lmem_align_allocated;
  return buf;
}

// Allocate aligned to load/store width.
// Returns NULL if fail, else returns pointer to allocated memory
void *xocl_aligned_alloc_lmem(XOCL_LMEM_ID id, size_t *size, int allow_shrink) 
{
  return _aligned_alloc_lmem(id, size, allow_shrink, XCHAL_DATA_WIDTH);
}

// Allocate with default alignment of 4-bytes.
// Returns NULL if fail, else returns pointer to allocated memory
void *xocl_alloc_lmem(XOCL_LMEM_ID id, size_t size) 
{
  return _aligned_alloc_lmem(id, &size, XOCL_FALSE, 4);
}

// Allocate from the internal local memory manager.
// Returns NULL if fail, else returns pointer to allocated memory
void *xocl_alloc_lmem_internal(size_t size) 
{
  return _aligned_alloc_lmem(XOCL_LMEM0, &size, XOCL_FALSE, 4);
}

// Free from the internal local memory manager. Nothing to do.
void xocl_free_lmem_internal(void *p) 
{ 
  (void)p; 
}

// Allocate from the external local memory manager.
// Returns NULL if fail, else returns pointer to allocated memory
void *xocl_alloc_lmem_external(size_t size) 
{
  xmem_bank_status_t status;
  void *p = xmem_bank_cb_if->xmem_bank_alloc(0, size, 4, &status);
  XOCL_DPRINTF("xocl_alloc_lmem_external: Allocating %d bytes at %p\n",
               size, p);
  return p;
}

// Free from the external local memory manager
void xocl_free_lmem_external(void *p) 
{
  // Only for heap; for stack, the caller is expected to save/restore 
  // stack state
  if (xmem_bank_cb_if->xmem_bank_get_alloc_policy() == XMEM_BANK_HEAP_ALLOC) {
    xmem_bank_status_t status;
    XOCL_DPRINTF("xocl_free_lmem_external: Freeing %p\n", p);
    status = xmem_bank_cb_if->xmem_bank_free(0, p);
    XOCL_ASSERT(status == XMEM_BANK_OK, 
                "xocl_free_lmem_external: xmem_bank_free failed\n");
  }
}

// Returns the free size in bank with given id
size_t xocl_get_lmem_free_size(XOCL_LMEM_ID id) 
{
  return local_memory_banks[id].size - local_memory_banks[id].allocated;
}

// Returns the free size in bank from the external local memory manager
size_t xocl_get_lmem_free_size_external(unsigned bank) 
{
  return xmem_bank_cb_if->xmem_bank_get_free_space(bank, 1, NULL, NULL, NULL);
}

// Returns the current allocation offset of the bank
uint32_t xocl_get_allocated(XOCL_LMEM_ID id) 
{
  xocl_lmem *lmem = &local_memory_banks[id];
  XOCL_DPRINTF("xocl_get_allocated: lmem%d: %p + allocated: %d = %p\n",
               id, lmem->start, lmem->allocated, lmem->start + lmem->allocated);
  return lmem->allocated;
}

// Set the current allocation offset of the bank
void xocl_set_allocated(XOCL_LMEM_ID id, uint32_t allocated) 
{
  xocl_lmem *lmem = &local_memory_banks[id];
  XOCL_DEBUG_ASSERT(allocated <= lmem->size);
  lmem->allocated = allocated;
  XOCL_DPRINTF("xocl_set_allocated: lmem%d: %p + allocated: %d = %p\n",
               id, lmem->start, allocated, lmem->start + allocated);
}

// Check if p is within the internal banks
// Returns 0 if true, else -1
int xocl_lmem_bounds_check(XOCL_LMEM_ID id, void *p)
{
  xocl_lmem *lmem = &local_memory_banks[id];
  if ((uintptr_t)p >= (uintptr_t)lmem->start &&
      (uintptr_t)p < ((uintptr_t)lmem->start + lmem->size))
    return 0;
  return -1;
}

// Initialize internal local memory manager using the local mem bank manager.
// All available data in the local memory banks are reserved for internal use.
//
// Returns 0 if successful, else returns -1
int xocl_init_local_mem()
{
  xmem_bank_status_t status;

  if (lmem_initialized) {
    XOCL_DPRINTF("xocl_init_local_mem: Already initialized\n");
    return 0;
  }

  // Needs 1 or 2 banks
  uint32_t num_banks = xmem_bank_cb_if->xmem_bank_get_num_banks();
  if (num_banks != 1 && num_banks != 2)
    return -1;

  xmem_bank_alloc_policy_t alloc_policy = 
                           xmem_bank_cb_if->xmem_bank_get_alloc_policy();
   
  XOCL_DPRINTF("xocl_init_local_mem: Initializing\n");

  if (num_banks == 1) {
    // Use available bank space to setup for internal allocation
    if (alloc_policy == XMEM_BANK_STACK_ALLOC) {
      // Check point current bank state for stack based allocator
      xmem_bank_cb_if->xmem_bank_checkpoint_save(0, &lmem0_checkpoint);
    }

    // Find free space in bank-0
    void *start, *end;
    size_t size = xmem_bank_cb_if->xmem_bank_get_free_space(0, 1,
                                                            &start, &end, 
                                                            &status);
    if (status != XMEM_BANK_OK)
      return -1;

    // Allocate all of the free space in bank-0
    void *p = xmem_bank_cb_if->xmem_bank_alloc(0, size, 1, &status);
    if (status != XMEM_BANK_OK)
      return -1;
    XOCL_ASSERT(p == start, "xocl_init_local_mem: Expecting to allocate at "
                "start of free space p: %p, start: %p\n", p, start);

    XOCL_DPRINTF("xocl_init_local_mem: Upfront allocation from bank 0 %d "
                 "bytes at %p\n", size, start);

    // Split the available space into 2 internal banks
    local_memory_banks[0].orig_start = start;
    // Align the start to load/store width
    local_memory_banks[0].start = (void *)get_aligned_offset((uint32_t)start, 
                                                             XCHAL_DATA_WIDTH);
    // Find remaining space in bank-0 after alignment
    uint32_t rem_size = (uintptr_t)end - 
                        (uintptr_t)local_memory_banks[0].start + 1;
    local_memory_banks[0].size = rem_size / 2;
    local_memory_banks[0].allocated = 0;

    uint32_t b1_start = (uintptr_t)local_memory_banks[0].start + rem_size/2;
    local_memory_banks[1].start = (void *)get_aligned_offset((uint32_t)b1_start,
                                                             XCHAL_DATA_WIDTH);
    // Find remaining space after alignment
    local_memory_banks[1].size = (uintptr_t)end - 
                                 (uintptr_t)local_memory_banks[1].start + 1;
    local_memory_banks[1].allocated = 0;
  } else {
    if (alloc_policy == XMEM_BANK_STACK_ALLOC) {
      // Check point current bank state for stack based allocator
      xmem_bank_cb_if->xmem_bank_checkpoint_save(0, &lmem0_checkpoint);
      xmem_bank_cb_if->xmem_bank_checkpoint_save(1, &lmem1_checkpoint);
    }

    for (int i = 0; i < 2; ++i) {
      void *start, *end;
      size_t size = xmem_bank_cb_if->xmem_bank_get_free_space(i, 
                                                              1, 
                                                              &start, &end,
                                                              &status);
      if (status != XMEM_BANK_OK) {
        // Free earlier successful allocation
        if (i == 1) {
          if (alloc_policy == XMEM_BANK_STACK_ALLOC)
           xmem_bank_cb_if->xmem_bank_checkpoint_restore(0, &lmem0_checkpoint);
          else
            xmem_bank_cb_if->xmem_bank_free(0, 
                                            local_memory_banks[0].orig_start);
        }
        return -1;
      }

      // Allocate all of the free space in bank-i
      void *p = xmem_bank_cb_if->xmem_bank_alloc(i, size, 1, &status);
      if (status != XMEM_BANK_OK) {
        // Free earlier successful allocation
        if (i == 1) {
          if (alloc_policy == XMEM_BANK_STACK_ALLOC)
           xmem_bank_cb_if->xmem_bank_checkpoint_restore(0, &lmem0_checkpoint);
          else
            xmem_bank_cb_if->xmem_bank_free(0, 
                                            local_memory_banks[0].orig_start);
        }
        return -1;
      }
      XOCL_ASSERT(p == start, "xocl_init_local_mem: Expecting to allocate at "
                  "start of free space p: %p, start: %p\n", p, start);

      XOCL_DPRINTF("xocl_init_local_mem: Upfront allocation from bank %d %d "
                   "bytes at %p\n", i, size, start);

      local_memory_banks[i].orig_start = start;
      // Align the start to load/store width
      local_memory_banks[i].start = 
            (void *)get_aligned_offset((uint32_t)start, XCHAL_DATA_WIDTH);
      // Find remaining space after alignment
      local_memory_banks[i].size = (uintptr_t)end - 
                                   (uintptr_t)local_memory_banks[i].start + 1;
      local_memory_banks[i].allocated = 0;
    }
  }

#ifdef XOCL_DEBUG
  for (int i = 0; i < 2; ++i) {
    XOCL_DPRINTF("xocl_init_local_mem:  lmem%d: start = %p, "
                 "end = %p, size %d\n", i, 
                 local_memory_banks[i].start, 
                 (void *)((uintptr_t)local_memory_banks[i].start + 
                          local_memory_banks[i].size - 1),
                 local_memory_banks[i].size);
  }
#endif // XOCL_DEBUG

  lmem_initialized = 1;

  return 0;
}

// Free internally allocated local memory
void xocl_deinit_local_mem()
{
  if (xmem_bank_cb_if->xmem_bank_get_alloc_policy() == XMEM_BANK_STACK_ALLOC) {
    xmem_bank_cb_if->xmem_bank_checkpoint_restore(0, &lmem0_checkpoint);
    if (xmem_bank_cb_if->xmem_bank_get_num_banks() > 1) 
      xmem_bank_cb_if->xmem_bank_checkpoint_restore(1, &lmem1_checkpoint);
  } else {
    xmem_bank_cb_if->xmem_bank_free(0, local_memory_banks[0].orig_start);
    if (xmem_bank_cb_if->xmem_bank_get_num_banks() > 1) 
      xmem_bank_cb_if->xmem_bank_free(1, local_memory_banks[1].orig_start);

#ifdef XOCL_DEBUG
    XOCL_DPRINTF("xocl_deinit_local_mem: Releasing upfront allocation from "
                 "bank 0, %p\n", local_memory_banks[0].orig_start);
    if (xmem_bank_cb_if->xmem_bank_get_num_banks() > 1) {
      XOCL_DPRINTF("xocl_deinit_local_mem: Releasing upfront allocation from "
                   "bank 1, %p\n", local_memory_banks[1].orig_start);
    }
#endif
  }

  lmem_initialized = 0;
}

// Returns the amount, start, and end of free space in given data ram bank
size_t xocl_get_allocated_local_mem(unsigned local_mem_id, void **start, 
                                     void **end) {
  if (local_mem_id != 0 && local_mem_id != 1)
    return 0;
  xocl_lmem *lmem = &local_memory_banks[local_mem_id];
  uintptr_t end_addr = (uintptr_t)lmem->start + lmem->size - 1;
  uintptr_t start_addr = (uintptr_t)lmem->start + 
                         local_memory_banks[local_mem_id].allocated;
  size_t fs = end_addr - start_addr + 1;
  XOCL_DPRINTF("xt_ocl_get_allocated_local_mem: lmem%d: start: %x, end: %x, "
               "free bytes: %d\n", local_mem_id, start_addr, end_addr, fs);
  *end = (void *)end_addr;
  *start = (void *)start_addr;
  return fs;
}

// Initialize the local memory manager callback interface
void xocl_set_xmem_bank_callback_if(xdyn_lib_xmem_bank_callback_if *cb_if)
{
  XOCL_DPRINTF("xocl_set_xmem_bank_callback_if...\n");
  xmem_bank_cb_if = cb_if;
}
