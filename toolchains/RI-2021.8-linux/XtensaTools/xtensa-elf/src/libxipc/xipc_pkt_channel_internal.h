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
#ifndef __XIPC_PKT_CHANNEL_INTERNAL_H__
#define __XIPC_PKT_CHANNEL_INTERNAL_H__

#include <stdint.h>
#if XCHAL_HAVE_IDMA
#include <xtensa/idma.h>
#endif
#include "xipc_common.h"
#include "xipc_pkt_channel.h"

#ifndef XIPC_PKT_CHANNEL_NUM_SLOTS
#define XIPC_PKT_CHANNEL_NUM_SLOTS 15
#endif

typedef uint32_t  xipc_pkt_slot_ptr_t;

typedef struct {
  uint32_t _pkt_ptr;
  uint16_t _size;
  uint16_t _num_pkts;
} xipc_pkt_slot_t;

typedef struct {
  uint32_t _mask;
  struct {
    void     *_buffer;
    uint16_t  _packet_size;
    uint8_t   _num_packets;
  } _val;
} xipc_pkt_channel_attr_t; 

struct xipc_pkt_channel_input_port_struct {
  xipc_pkt_slot_ptr_t           _head;
  volatile xipc_pkt_slot_ptr_t  _tail;
  xipc_pkt_slot_ptr_t           _cached_output_port_tail;
  xipc_pkt_channel_attr_t       _attr;
  xipc_pkt_slot_t              *_output_port_free_packets;
  xipc_pkt_slot_ptr_t          *_output_port_tail;
  xipc_pkt_slot_t               _ready_packets[XIPC_PKT_CHANNEL_NUM_SLOTS+1];
  uint16_t                      _channel_id;
  xipc_pid_t                    _src_pid;
};

struct xipc_pkt_channel_output_port_struct {
  xipc_pkt_slot_ptr_t           _head;
  volatile xipc_pkt_slot_ptr_t  _tail;
  xipc_pkt_slot_ptr_t           _cached_input_port_tail;
  xipc_pkt_slot_t              *_input_port_ready_packets;
  xipc_pkt_slot_ptr_t          *_input_port_tail;
  xipc_pkt_channel_attr_t       _attr;
  xipc_pkt_slot_t               _free_packets[XIPC_PKT_CHANNEL_NUM_SLOTS+1];
  uint16_t                      _channel_id;
  xipc_pid_t                    _dest_pid;
};

struct xipc_pkt_channel_request_struct {
  union {
    xipc_pkt_channel_input_port_t  *_ip;
    xipc_pkt_channel_output_port_t *_op;
  } _port;
#if XCHAL_HAVE_IDMA
  idma_buffer_t   _idma_task[IDMA_BUFFER_SIZE(1, IDMA_1D_DESC)/sizeof(idma_buffer_t)];
#endif
  xipc_pkt_t       _pkt;
  xipc_wait_kind_t _wait_kind;
};

typedef struct {
  union {
    xipc_pkt_channel_input_port_t  *_ip;
    xipc_pkt_channel_output_port_t *_op;
  } _p;
  uint32_t _n;
} xipc_pkt_channel_cond_t;

#endif /* __XIPC_PKT_CHANNEL_INTERNAL_H__ */
