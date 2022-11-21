/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED. 
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of 
 * Cadence Design Systems Inc. Any rights to use, modify, and create 
 * derivative works of this file are set forth under the terms of your 
 * license agreement with Cadence Design Systems, Inc.
 */

#include <stdint.h>
#include <stddef.h>
#include "lmem.h"
#include "misc.h"

// Number of dma descriptors
#define XOCL_DMA_NUM_DESCS (48)

// Pick 64-byte descriptors if supported in hardware
#if IDMA_USE_64B_DESC
#define XOCL_IDMA_DESC_TYPE IDMA_64B_DESC
#else
#define XOCL_IDMA_DESC_TYPE IDMA_2D_DESC
#endif

// This provides wrappers around idma library calls made by the device
// runtime.

// Pointer to table of external callback functions for dma
static xdyn_lib_dma_callback_if *dma_cb_if XOCL_LMEM_ATTR = 0;

// iDMA descriptor buffer
static idma_buffer_t *xocl_dma_desc_buf XOCL_LMEM_ATTR;

// Has dma been already initialized ?
static int dma_initialized = 0;

#if defined(XOCL_TIME_KERNEL)
// Collect dma wait time statistics
extern uint32_t xocl_wait_dma_cycles;
#endif

// Initialize the dma callback interface
void xocl_set_dma_callback_if(xdyn_lib_dma_callback_if *cb_if)
{
  XOCL_DPRINTF("xocl_set_dma_callback_if...\n");
  dma_cb_if = cb_if;
}

#if XCHAL_HAVE_IDMA
// Initialize dma descriptors. Returns 0 if successful, else returns -1
int xocl_dma_desc_init()
{
  XOCL_DPRINTF("xocl_dma_desc_init: Initializing dma desc buffer at %p\n",
               xocl_dma_desc_buf);

  // Re-init dma's descriptor buffer, by first calling idma_buffer_status
  // to clear any previously bound buffer (from another module)
  if (dma_initialized)
    dma_cb_if->idma_buffer_status(IDMA_CHANNEL_0);

  idma_status_t s = dma_cb_if->idma_init_loop(IDMA_CHANNEL_0, 
                                              xocl_dma_desc_buf, 
                                              XOCL_IDMA_DESC_TYPE,
                                              XOCL_DMA_NUM_DESCS, NULL, NULL);
  if (s != IDMA_OK)
    return -1;

  dma_initialized = 1;

  return 0;
}

// Allocate space for dma descriptors. Returns 0 if successful, else returns -1
int xocl_dma_desc_buffer_init()
{
  // Allocate space for descriptors
  unsigned dma_desc_buf_size = IDMA_BUFFER_SIZE(XOCL_DMA_NUM_DESCS, 
                                                XOCL_IDMA_DESC_TYPE);
  xocl_dma_desc_buf = (idma_buffer_t *)
                      xocl_alloc_lmem_external(dma_desc_buf_size);
  if (!xocl_dma_desc_buf) 
    return -1;

  XOCL_DPRINTF("xocl_dma_init: Allocating dma buffer of size %d at %p\n", 
               dma_desc_buf_size, xocl_dma_desc_buf);
  return 0;
}

// Free space allocated for the dma descriptors
void xocl_dma_desc_buffer_deinit()
{
  xocl_free_lmem_external(xocl_dma_desc_buf);
}

// Return number of dma descriptors
unsigned xocl_dma_get_num_descs()
{
  return XOCL_DMA_NUM_DESCS;
}

// Return the dma 2D descriptor buffer
idma_buffer_t *xocl_dma_get_desc_buffer()
{
  return xocl_dma_desc_buf;
}

// Wait for a particular dma descriptor to finish. Return 0 if successful,
// else returns idma_hw_error_t error code.
unsigned xocl_wait_dma(unsigned desc) 
{
  //idma_hw_error_t err = dma_cb_if->idma_buffer_check_errors(IDMA_CHANNEL_0);
  //if (err) {
  //  XOCL_DPRINTF("xocl_wait_dma: dma in error, err code: %d\n", err);
  //  return err;
  //}
  XOCL_DPRINTF("xocl_wait_dma: desc idx: %d\n", desc);
#if defined(XOCL_TIME_KERNEL)
  uint32_t stm = _xocl_time_stamp();
#endif
  if (desc)
    while (dma_cb_if->idma_desc_done(IDMA_CHANNEL_0, desc) == 0)
      ;
    //while (idma_desc_done(IDMA_CHANNEL_0, desc) == 0)
    //  ;
#if defined(XOCL_TIME_KERNEL)
  uint32_t etm = _xocl_time_stamp();
  xocl_wait_dma_cycles += (etm - stm);
#endif 
  return 0;
}

// Wait for all outstanding dmas to finish. Return 0 if successful,
// else returns idma_hw_error_t error code.
unsigned xocl_wait_all_dma() 
{
  //idma_hw_error_t err = dma_cb_if->idma_buffer_check_errors(IDMA_CHANNEL_0);
  //if (err) {
  //  XOCL_DPRINTF("xocl_wait_dma: dma in error, err code: %d\n", err);
  //  return err;
  //}
  XOCL_DPRINTF("  xocl_wait_all_dma\n");
  while (dma_cb_if->idma_buffer_status(IDMA_CHANNEL_0) > 0)
    ;
  //while (idma_buffer_status(IDMA_CHANNEL_0) > 0)
  //  ;
  return 0;
}

// Schedule num descriptors. Returns the descriptor index
int xocl_dma_schedule(unsigned num) 
{
  int r = dma_cb_if->idma_schedule_desc(IDMA_CHANNEL_0, num);
  //int r = idma_schedule_desc(IDMA_CHANNEL_0, num);
  XOCL_DPRINTF("xocl_dma_schedule: num descs: %d, desc idx: %d\n", num, r);
  return r;
}

// 2-D dma
void
xocl_dma_add_2d(void *dst, void *src, size_t row_size, unsigned flags,
                unsigned num_rows, unsigned src_pitch, unsigned dst_pitch) 
{
  XOCL_DPRINTF("xocl_dma_add_2d: dst:%p src:%p row_size:%d, num_rows:%d, "
               "src_pitch:%d dst_pitch:%d\n", dst, src, row_size, num_rows,
               src_pitch, dst_pitch);
#if IDMA_USE_64B_DESC
  dma_cb_if->idma_ext_add_2d_desc64_ch(IDMA_CHANNEL_0, dst, src, row_size, 
                                       flags, num_rows, src_pitch, dst_pitch);
  //idma_ext_add_2d_desc64_ch(IDMA_CHANNEL_0, dst, src, row_size, flags, 
  //                          num_rows, src_pitch, dst_pitch);
#else
  dma_cb_if->idma_ext_add_2d_desc_ch(IDMA_CHANNEL_0, dst, src, row_size, flags, 
                                     num_rows, src_pitch, dst_pitch);
  //idma_ext_add_2d_desc_ch(IDMA_CHANNEL_0, dst, src, row_size, flags, 
  //                        num_rows, src_pitch, dst_pitch);
#endif

#if IDMA_USE_64B_DESC
  // uint64_t dstw = (uint64_t)(uint32_t)dst;
  // uint64_t srcw = (uint64_t)(uint32_t)src;
  //dma_cb_if->idma_add_2d_desc64(xocl_dma_desc_buf, &dstw, &srcw, row_size, 
  //                              flags, num_rows, src_pitch, dst_pitch);
  //idma_add_2d_desc64(xocl_dma_desc_buf, dst, src, row_size, flags, 
  //                   num_rows, src_pitch, dst_pitch);
#else
  //dma_cb_if->idma_add_2d_desc(xocl_dma_desc_buf, dst, src, row_size, flags, 
  //                            num_rows, src_pitch, dst_pitch);
  //idma_add_2d_desc(xocl_dma_desc_buf, dst, src, row_size, flags, 
  //                 num_rows, src_pitch, dst_pitch);
#endif
}

// 2-D dma
int
xocl_dma_schedule_2d(void *dst, void *src, size_t row_size, unsigned flags,
                     unsigned num_rows, unsigned src_pitch, unsigned dst_pitch) 
{
  XOCL_DPRINTF("xocl_dma_schedule_2d: dst:%p src:%p row_size:%d, num_rows:%d, "
               "src_pitch:%d dst_pitch:%d\n", dst, src, row_size, num_rows,
               src_pitch, dst_pitch);
#if IDMA_USE_64B_DESC
   
  uint64_t dstw = (uint64_t)(uint32_t)dst;
  uint64_t srcw = (uint64_t)(uint32_t)src;

  return dma_cb_if->idma_copy_2d_desc64(IDMA_CHANNEL_0, &dstw, &srcw, row_size, 
                                        flags, num_rows, src_pitch, dst_pitch);
  //return idma_copy_2d_desc64(IDMA_CHANNEL_0, &dstw, &srcw, row_size, flags, 
  //                           num_rows, src_pitch, dst_pitch);
#else
  return dma_cb_if->idma_copy_2d_desc(IDMA_CHANNEL_0, dst, src, row_size, 
                                      flags, num_rows, src_pitch, dst_pitch);
  //return idma_copy_2d_desc(IDMA_CHANNEL_0, dst, src, row_size, flags, 
  //                         num_rows, src_pitch, dst_pitch);
#endif
}

// Use dma to do memcpy
// Returns 0 if successful, else returns idma_hw_error_t error code.
unsigned xocl_dma_memcpy(void *dst, const void *src, size_t size)
{
  if (!size)
    return 0;
  xocl_dma_add_2d(dst, (void*)src, size, 0, 1, size, size);
  return xocl_wait_dma(xocl_dma_schedule(1));
}

// Perform 1-d copy from global to local mem. 
// Called from runtime after promotion
int async_copy_to_local(void *dst, void *src, unsigned size) 
{
  if (!dst)
    return 0;
  XOCL_DPRINTF("async_copy_to_local: (d, s, size) = (%p, %p, %d)\n",
               dst, src, size);
  xocl_dma_add_2d(dst, src, size, 0, 1, size, size);
  return 0;
}

// Perform 1-d copy from local to global mem. 
// Called from runtime after promotion
int async_copy_to_global(void *dst, void *src, unsigned size) 
{
  XOCL_DPRINTF("async_copy_to_global: (d, s, size) = (%p, %p, %d)\n",
               dst, src, size);
  xocl_dma_add_2d(dst, src, size, 0, 1, size, size);
  return 0;
}

// Perform 2-d copy from global to local mem. 
// Called from runtime after promotion
int async_2d_copy_to_local(void *dst, void *src,
                           unsigned row_size, unsigned num_rows,
                           unsigned dst_pitch, unsigned src_pitch) 
{
  if (!dst)
    return 0;
  XOCL_DPRINTF("async_2d_copy_to_local: (d, s) = (%p, %p)\n", dst, src);
  xocl_dma_add_2d(dst, src, row_size, 0, num_rows, src_pitch, dst_pitch);
  return 0;
}

// Perform 2-d copy from local to global mem. 
// Called from runtime after promotion
int async_2d_copy_to_global(void *dst, void *src,
                            unsigned row_size, unsigned num_rows,
                            unsigned dst_pitch, unsigned src_pitch) 
{
  XOCL_DPRINTF("async_2d_copy_to_global: (d, s) = (%p, %p)\n", dst, src);
  xocl_dma_add_2d(dst, src, row_size, 0, num_rows, src_pitch, dst_pitch);
  return 0;
}

// Below presents dummy functions that are inserted during the process of
// DMA promotion and if the compiler wasn't able to promote these, may remain
// in the generated kernels. 

void async_dma_run(int task)
{
  (void)task;
  xocl_wait_dma(xocl_dma_schedule(1));
}

void async_2d_dma_run(int task)
{
  (void)task;
  xocl_wait_dma(xocl_dma_schedule(1));
}

void async_dma_wait(int task)
{
  (void)task;
}

void async_2d_dma_wait(int task)
{
  (void)task;
}

void async_dma_init(int in_num, int out_num)
{
  (void)in_num;
  (void)out_num;
}

void async_2d_dma_init(int in_num, int out_num)
{
  (void)in_num;
  (void)out_num;
}

#else
// No dma; use regular memcpy
unsigned xocl_dma_memcpy(void *dst, const void *src, size_t size)
{
  if (!size)
    return;
  XOCL_DPRINTF("xocl_dma_memcpy: dst: %p, src: %p, size: %u\n", dst, src, size);
  memcpy(dst, src, size);
  return 0;
}

// Dummy versions of the above functions, if XCHAL_HAVE_IDMA is not defined

int xocl_dma_desc_buffer_init(xdyn_lib_xmem_callback_if *xmem_bank_cb_if)
{
  return 0;
}

int xocl_dma_desc_init() 
{
  return 0;
}

unsigned xocl_wait_dma(unsigned desc) 
{
  return 0;
}

int xocl_dma_schedule(unsigned num) 
{
  return 0;
}

void
xocl_dma_add_2d(void *dst, void *src, size_t row_size, unsigned flags,
                 unsigned num_rows, unsigned src_pitch, unsigned dst_pitch)
{
  return;
}

unsigned xocl_dma_memcpy(void *dst, const void *src, size_t size)
{
  return 0;
}

int
xocl_dma_schedule_2d(void *dst, void *src, size_t row_size, unsigned flags,
                     unsigned num_rows, unsigned src_pitch, unsigned dst_pitch) 
{
  return 0;
}

unsigned xocl_dma_get_num_descs()
{
  return 0;
}

idma_buffer_t *xocl_dma_get_desc_buffer()
{
  return NULL;
}
#endif // XCHAL_HAVE_IDMA
