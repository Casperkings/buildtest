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
#ifndef __XIPC_COUNTED_EVENT_INTERNAL_H__
#define __XIPC_COUNTED_EVENT_INTERNAL_H__

#include <stdint.h>
#include "xipc_counted_event.h"

struct xipc_counted_event_struct {
  volatile uint32_t _lock;
  volatile uint32_t _count;
  xipc_wait_kind_t  _wait_kind;
  volatile uint8_t  _wait_pids[XIPC_MAX_NUM_PROCS];
  volatile uint32_t _wait_count[XIPC_MAX_NUM_PROCS];
};

struct xipc_cond_counted_event_struct {
  xipc_counted_event_t **_events;
  uint32_t               _num_events;
};

#endif /* __XIPC_COUNTED_EVENT_INTERNAL_H__ */
