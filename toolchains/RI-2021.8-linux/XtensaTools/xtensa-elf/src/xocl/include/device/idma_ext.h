/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED. 
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of 
 * Cadence Design Systems Inc. Any rights to use, modify, and create 
 * derivative works of this file are set forth under the terms of your 
 * license agreement with Cadence Design Systems, Inc.
 */

#ifndef __IDMA_EXT_H__
#define __IDMA_EXT_H__

#include <stdint.h>
#include <xtensa/idma.h>

#ifdef __cplusplus
extern "C" {
#endif

// Defines extensions to the idma library

static inline idma_status_t
idma_ext_add_2d_desc_ch(int32_t ch, void *dst, void *src,  size_t row_sz,  
                        uint32_t flags, uint32_t nrows, 
                        uint32_t src_pitch, uint32_t dst_pitch)
{
  idma_buf_t *buf;
  DECLARE_PS();

  IDMA_DISABLE_INTS();

  buf = idma_chan_buf_get(ch);

  set_desc_ctrl(buf->next_add_desc, flags, IDMA_2D_DESC_CODE);
  set_2d_fields(buf->ch, buf->next_add_desc, dst, src, row_sz, 
                nrows, src_pitch, dst_pitch);

  update_next_add_ptr(buf);

  IDMA_ENABLE_INTS();
  return IDMA_OK;
}

#if IDMA_USE_64B_DESC
static inline idma_status_t
idma_ext_add_2d_desc64_ch(int32_t ch, void *dst, void *src,  size_t row_sz,  
                          uint32_t flags, uint32_t nrows, 
                          uint32_t src_pitch, uint32_t dst_pitch)
{
  idma_buf_t *buf;
  DECLARE_PS();

  uint64_t dstw = (uint64_t)(uint32_t)dst;
  uint64_t srcw = (uint64_t)(uint32_t)src;

  IDMA_DISABLE_INTS();

  buf = idma_chan_buf_get(ch);

  set_desc64_ctrl(buf->next_add_desc, flags, 
                  IDMA_64B_DESC_CODE | SET_CONTROL_SUBTYPE(IDMA_64B_2D_TYPE));
  set_2d_desc64_fields(buf->ch, buf->next_add_desc, &dstw, &srcw, row_sz, 
                       nrows, src_pitch, dst_pitch);

  update_next_add_ptr(buf);

  IDMA_ENABLE_INTS();
  return IDMA_OK;
}
#endif

#ifdef __cplusplus
}
#endif

#endif // __IDMA_EXT_H__
