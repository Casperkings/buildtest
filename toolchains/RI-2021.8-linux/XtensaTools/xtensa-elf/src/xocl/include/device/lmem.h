/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED. 
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of 
 * Cadence Design Systems Inc. Any rights to use, modify, and create 
 * derivative works of this file are set forth under the terms of your 
 * license agreement with Cadence Design Systems, Inc.
 */

#ifndef __LMEM_H__
#define __LMEM_H__

// Internal memory management routines

#include <stdint.h>
#include <stddef.h>
#include "xdyn_lib_callback_if.h"

// Enum to represent internal memory banks
typedef enum {
  XOCL_LMEM0 = 0,
  XOCL_LMEM1,
} XOCL_LMEM_ID;

// Initialize local memory
extern int xocl_init_local_mem();

// De-initialize local memory
extern void xocl_deinit_local_mem();

// Initialize the local memory manager callback interface
extern void 
xocl_set_xmem_bank_callback_if(xdyn_lib_xmem_bank_callback_if *cb_if);

// Allocate aligned to load/store width from internal local memory manager
extern void *xocl_aligned_alloc_lmem(XOCL_LMEM_ID id, size_t *size, 
                                     int allow_shrink);

// Allocate with default alignment of 4-bytes from internal local memory manager
extern void *xocl_alloc_lmem(XOCL_LMEM_ID id, size_t size);

// Allocate from the external local memory manager
extern void *xocl_alloc_lmem_external(size_t size);

// Free from the external local memory manager
extern void xocl_free_lmem_external(void *p);

// Allocate from the internal local memory manager
extern void *xocl_alloc_lmem_internal(size_t size);

// Free from the internal local memory manager
extern void xocl_free_lmem_internal(void *p);

// Returns the free size in bank with given id
extern size_t xocl_get_lmem_free_size(XOCL_LMEM_ID id);

// Returns the free size in bank from the external local memory manager
extern size_t xocl_get_lmem_free_size_external(unsigned bank);

// Returns the current allocation offset of the bank
extern uint32_t xocl_get_allocated(XOCL_LMEM_ID id);

// Set the current allocation offset of the bank
extern void xocl_set_allocated(XOCL_LMEM_ID id, uint32_t allocated);

// Check if p is within the internal banks
// Returns 0 if true, else -1
extern int xocl_lmem_bounds_check(XOCL_LMEM_ID id, void *p);

// Returns the amount, start, and end of free space in given data ram bank
extern size_t xocl_get_allocated_local_mem(unsigned local_mem_id, void **start,
                                           void **end);

#endif // __LMEM_H__
