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
#ifndef __XIPC_BARRIER_INTERNAL_H__
#define __XIPC_BARRIER_INTERNAL_H__

#include <stdint.h>
#include "xipc_common.h"
#include "xipc_barrier.h"

typedef enum {
  XIPC_BARRIER_ENTER,
  XIPC_BARRIER_EXIT
} xipc_barrier_state_t;

struct xipc_barrier_struct {
  xipc_wait_kind_t              _wait_kind;
  volatile xipc_barrier_state_t _state;
  volatile uint32_t             _count;
  uint8_t                       _wait_pids[XIPC_MAX_NUM_PROCS];
  uint8_t                       _num_procs;
  uint8_t                       _num_waiters;
};

struct xipc_reset_sync_struct {
  volatile uint32_t _loc;
};

#endif /* __XIPC_BARRIER_INTERNAL_H__ */
