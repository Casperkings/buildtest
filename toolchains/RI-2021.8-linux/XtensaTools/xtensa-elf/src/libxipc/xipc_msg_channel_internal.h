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
#ifndef __XIPC_MSG_CHANNEL_INTERNAL_H__
#define __XIPC_MSG_CHANNEL_INTERNAL_H__

#include <stddef.h>
#include "xipc_msg_channel.h"

typedef uint32_t xipc_msg_channel_slot_ptr_t;

struct xipc_msg_channel_struct {
  xipc_msg_channel_slot_ptr_t *_output_port_read_ptr;
  xipc_msg_channel_slot_ptr_t *_input_port_write_ptr;
  int32_t                     *_input_port_buffer;
  volatile uint32_t            _input_port_connected;
  volatile uint32_t            _output_port_connected;
  uint32_t                     _num_words;
  uint16_t                     _channel_id;
  xipc_pid_t                   _src_pid;
  xipc_pid_t                   _dest_pid;
};

struct xipc_msg_channel_input_port_struct {
  volatile xipc_msg_channel_slot_ptr_t  _write_ptr;
  xipc_msg_channel_slot_ptr_t           _cached_read_ptr;
  xipc_msg_channel_slot_ptr_t          *_output_port_read_ptr;
  uint32_t                              _num_words;
  int32_t                              *_buffer;
  uint16_t                              _channel_id;
  xipc_pid_t                            _src_pid;
};

struct xipc_msg_channel_output_port_struct {
  volatile xipc_msg_channel_slot_ptr_t  _read_ptr;
  xipc_msg_channel_slot_ptr_t           _cached_write_ptr;
  xipc_msg_channel_slot_ptr_t          *_input_port_write_ptr;
  int32_t                              *_input_port_buffer;
  uint32_t                              _num_words;
  uint16_t                              _channel_id;
  xipc_pid_t                            _dest_pid;
};

typedef struct {
  union {
    xipc_msg_channel_input_port_t  *_ip;
    xipc_msg_channel_output_port_t *_op;
  } _p;
  uint32_t _n;
} xipc_msg_channel_cond_t;

extern void xipc_msg_channel_print_channel(xipc_msg_channel_t *ch);

extern void
xipc_msg_channel_print_input_port(xipc_msg_channel_input_port_t *ip);

extern void
xipc_msg_channel_print_output_port(xipc_msg_channel_output_port_t *op);

#endif /* __XIPC_MSG_CHANNEL_INTERNAL_H__ */
