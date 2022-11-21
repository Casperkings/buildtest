/*
 * Copyright (c) 2019-2020 Cadence Design Systems. ALL RIGHTS RESERVED.
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

#define IDMA_BUILD

// All functions in this file to be compiled for wide address.
#define IDMA_USE_WIDE_ADDRESS_COMPILE   1

#include <stdint.h>
#include <stdarg.h>

#include "idma_internal.h"


// Private function not declared in headers.
extern idma_status_t queue_task(idma_buf_t * task);


INTERNAL_FUNC idma_status_t
idma_add_desc_wide_i(idma_buffer_t * bufh,
                     void *          dst,
                     void *          src,
                     size_t          size,
                     uint32_t        flags)
{
  idma_buf_t * buf;

  DECLARE_PS();
  IDMA_DISABLE_INTS();

  buf = convert_buffer_to_buf(bufh);

  set_desc_ctrl(buf->next_add_desc, flags, IDMA_1D_DESC_CODE);
  set_1d_fields(buf->ch, buf->next_add_desc, dst, src, size);
  update_next_add_ptr(buf);

  IDMA_ENABLE_INTS();
  return IDMA_OK;
}


INTERNAL_FUNC idma_status_t
idma_add_2d_desc_wide_i(idma_buffer_t * bufh,
                        void *          dst,
                        void *          src,
                        size_t          row_sz,
                        uint32_t        flags,
                        uint32_t        nrows,
                        uint32_t        src_pitch,
                        uint32_t        dst_pitch)
{
  idma_buf_t * buf;

  DECLARE_PS();
  IDMA_DISABLE_INTS();

  buf = convert_buffer_to_buf(bufh);

  set_desc_ctrl(buf->next_add_desc, flags, IDMA_2D_DESC_CODE);
  set_2d_fields(buf->ch, buf->next_add_desc, dst, src, row_sz, nrows, src_pitch, dst_pitch);
  update_next_add_ptr(buf);

  IDMA_ENABLE_INTS();
  return IDMA_OK;
}


idma_status_t
idma_copy_task_wide_i(int32_t          ch,
                      idma_buffer_t *  taskh,
                      void *           dst,
                      void *           src,
                      size_t           size,
                      uint32_t         flags,
                      void *           cb_data,
                      idma_callback_fn cb_func)
{
  idma_buf_t * task = convert_buffer_to_buf(taskh);

  // Return values from these two functions are not used.
  (void) idma_init_task_i(ch, taskh, IDMA_1D_DESC, 1, cb_func, cb_data);
  (void) idma_add_desc_wide_i(taskh, dst, src, size, flags);
  return queue_task(task);
}


idma_status_t
idma_copy_2d_task_wide_i(int32_t          ch,
                         idma_buffer_t *  taskh,
                         void *           dst,
                         void *           src,
                         size_t           row_sz,
                         uint32_t         flags,
                         uint32_t         nrows,
                         uint32_t         src_pitch,
                         uint32_t         dst_pitch,
                         void *           cb_data,
                         idma_callback_fn cb_func)
{
  idma_buf_t * task = convert_buffer_to_buf(taskh);

  // Return values from these two functions are not used.
  (void) idma_init_task_i(ch, taskh, IDMA_2D_DESC, 1, cb_func, cb_data);
  (void) idma_add_2d_desc_wide_i(taskh, dst, src, row_sz, flags, nrows, src_pitch, dst_pitch);
  return queue_task(task);
}

