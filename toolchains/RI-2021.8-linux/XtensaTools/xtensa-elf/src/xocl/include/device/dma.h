/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED. 
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of 
 * Cadence Design Systems Inc. Any rights to use, modify, and create 
 * derivative works of this file are set forth under the terms of your 
 * license agreement with Cadence Design Systems, Inc.
 */

#ifndef __DMA_H__
#define __DMA_H__

#include <stdint.h>
#include <stddef.h>
#include "xdyn_lib_callback_if.h"

// Initialize the dma callback interface
extern void
xocl_set_dma_callback_if(xdyn_lib_dma_callback_if *cb_if);

// Memcpy using dma. Returns 0 if successful.
extern unsigned xocl_dma_memcpy(void *dst, const void *src, size_t size);

// Initialize dma descriptors. Returns 0 if successful, else returns -1
extern int xocl_dma_desc_init();

// Allocate memory for dma descriptors. Returns 0 if successful, else returns -1
extern int xocl_dma_desc_buffer_init();

// Free memory allocated for dma descriptors
extern void xocl_dma_desc_buffer_deinit();

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

#endif // __DMA_H__
