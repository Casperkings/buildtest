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
#ifndef __XIPC_CQUEUE_INTERNAL_H__
#define __XIPC_CQUEUE_INTERNAL_H__

#include <stdint.h>
#include "xipc_common.h"
#include "xipc_cqueue.h"

#ifndef XIPC_CQUEUE_NUM_SLOTS
#define XIPC_CQUEUE_NUM_SLOTS 8
#endif

typedef struct {
  uint32_t _pkt_ptr;
  uint16_t _size;
} xipc_qpkt_slot_t;

struct xipc_cqueue_struct {
  volatile uint32_t  _lock;
  volatile uint32_t  _ready_head;
  volatile uint32_t  _ready_tail;
  volatile uint32_t  _ready_tail_copy;
  volatile uint32_t  _free_head;
  volatile uint32_t  _free_tail;
  volatile uint32_t  _free_tail_copy;
  xipc_qpkt_slot_t   _free_packets[XIPC_CQUEUE_NUM_SLOTS];
  xipc_qpkt_slot_t   _ready_packets[XIPC_CQUEUE_NUM_SLOTS];
  xipc_pid_t         _prod_wait_pids[XIPC_MAX_NUM_PROCS];
  xipc_pid_t         _cons_wait_pids[XIPC_MAX_NUM_PROCS];
  uintptr_t          _buffer_proc_addrs[XIPC_MAX_NUM_PROCS];
  uint32_t           _in_non_coherent_cached_mem;
  uint16_t           _cqueue_id;
  uint16_t           _packet_size;
  uint8_t            _num_cons_procs;
  uint8_t            _num_prod_procs;
  uint8_t            _num_packets;
};

#endif /* __XIPC_CQUEUE_INTERNAL_H__ */
