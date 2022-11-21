/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED. 
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of 
 * Cadence Design Systems Inc. Any rights to use, modify, and create 
 * derivative works of this file are set forth under the terms of your 
 * license agreement with Cadence Design Systems, Inc.
 */

#ifndef __XOCL_RUNTIME_H__
#define __XOCL_RUNTIME_H__

// XOCL runtime memory management interface

#include <stdint.h>
#include <stddef.h>

// Returns the amount, start, and end of free space in given data ram bank
extern size_t xocl_get_allocated_local_mem(unsigned local_mem_id, void **start,
                                           void **end);

// XOCL runtime DMA interface

// 2D dma
extern void
xocl_dma_add_2d(void *dst, void *src, size_t row_size, unsigned flags,
                unsigned num_rows, unsigned src_pitch, unsigned dst_pitch);

// Schedule num descriptors. Returns the descriptor index
extern int xocl_dma_schedule(unsigned num);

// Wait for a particular dma descriptor to finish. Return 0 if successful,
// else returns idma_hw_error_t error code.
extern unsigned xocl_wait_dma(unsigned desc);

// Wait for all outstanding dmas to finish. Return 0 if successful,
// else returns idma_hw_error_t error code.
extern unsigned xocl_wait_all_dma();

// Add and schedule 2D dma. Returns the descriptor index
extern int 
xocl_dma_schedule_2d(void *dst, void *src, size_t row_size, unsigned flags,
                     unsigned num_rows, unsigned src_pitch, unsigned dst_pitch);

// Return number of dma descriptors
extern unsigned xocl_dma_get_num_descs();

// Return the dma 2D descriptor buffer
extern idma_buffer_t *xocl_dma_get_desc_buffer();

#endif // __XOCL_RUNTIME_H__
