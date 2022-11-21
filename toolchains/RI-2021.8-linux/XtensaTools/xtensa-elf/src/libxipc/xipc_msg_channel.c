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
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <xtensa/tie/xt_core.h>
#include "xipc_addr.h"
#include "xipc_misc.h"
#include "xipc_common.h"
#include "xipc_msg_channel_internal.h"

/* Defined if channels have been setup */
extern uint8_t xipc_msg_channels_setup;

/* Externally defined in the subsystem */
extern xipc_msg_channel_input_port_t xmp_xipc_msg_channel_my_input_ports[];
extern xipc_msg_channel_output_port_t xmp_xipc_msg_channel_my_output_ports[];

/* Returns the number of allocated entries in the channel
 *
 * op : message channel output port
 *
 * Returns allocated count
 */
XIPC_INLINE uint32_t
xipc_msg_channel_output_port_count(xipc_msg_channel_output_port_t *op)
{
  return xipc_channel_count(op->_read_ptr, op->_cached_write_ptr, 
                            op->_num_words);
}

/* Returns the number of allocated entries in the channel
 *
 * ip : message channel input port
 *
 * Returns allocated count
 */
XIPC_INLINE uint32_t
xipc_msg_channel_input_port_count(xipc_msg_channel_input_port_t *ip)
{
  return xipc_channel_count(ip->_cached_read_ptr, ip->_write_ptr, 
                            ip->_num_words);
}

/* Check on the output port (src proc side) if the channel is full
 *
 * op : message channel output port
 *
 * Returns 0, if full, else returns a non-zero value
 */
XIPC_INLINE int32_t
xipc_msg_channel_full(xipc_msg_channel_output_port_t *op)
{
  /* Channel is full if the next write ptr == read ptr. The channel has
   * one unused entry to distinguish empty from full */
  xipc_msg_channel_slot_ptr_t read_ptr = op->_read_ptr;
  xipc_msg_channel_slot_ptr_t write_ptr = op->_cached_write_ptr;
  uint32_t next_write_ptr = xipc_modulo_add(write_ptr, 1, op->_num_words);
  return next_write_ptr - read_ptr;
}

/* Check on the input port (dest proc side) if the channel is empty 
 *
 * ip : message channel input port
 *
 * Returns 0, if empty, else returns a non-zero value
 */
XIPC_INLINE int32_t 
xipc_msg_channel_empty(xipc_msg_channel_input_port_t *ip)
{
  /* Channel is empty if read ptr == write ptr */
  xipc_msg_channel_slot_ptr_t read_ptr = ip->_cached_read_ptr;
  xipc_msg_channel_slot_ptr_t write_ptr = ip->_write_ptr;
  return read_ptr - write_ptr;
}

/* Checks if channel is empty 
 *
 * ip     : pointer to the channel input port
 * val    : ignored
 * thread : pointer to thread structure
 *
 * Returns 0 if the channel is empty, i.e the condition is not satisfied; else
 * returns non-zero value.
 */
static int32_t
xipc_cond_msg_channel_empty(void *ip, int32_t val, void *thread)
{
  return xipc_msg_channel_empty((xipc_msg_channel_input_port_t *)ip);
}

/* Checks if channel is full 
 *
 * op     : pointer to the channel output port
 * val    : ignored
 * thread : pointer to thread structure
 *
 * Returns 0 if the channel is full, i.e the condition is not satisfied; else
 * returns non-zero value.
 */
static int32_t
xipc_cond_msg_channel_full(void *op, int32_t val, void *thread)
{
  return xipc_msg_channel_full((xipc_msg_channel_output_port_t *)op);
}

/* Checks if channel has n allocated elements
 *
 * arg    : pointer to struct with channel input port and num elements
 * val    : ignored
 * thread : pointer to thread structure
 *
 * Returns 0 if the channel has less than n elements, i.e the condition is not 
 * satisfied; else returns non-zero value.
 */
static int32_t
xipc_cond_msg_channel_num_alloc(void *arg, int32_t val, void *thread)
{
  xipc_msg_channel_input_port_t *ip = ((xipc_msg_channel_cond_t *)arg)->_p._ip;
  uint32_t n = ((xipc_msg_channel_cond_t *)arg)->_n;
  return xipc_msg_channel_input_port_count(ip) < n ? 0 : 1;
}

/* Checks if channel has n free elements
 *
 * arg    : pointer to struct with channel output port and num elements
 * val    : ignored
 * thread : pointer to thread structure
 *
 * Returns 0 if the channel has less than n elements, i.e the condition is not 
 * satisfied; else returns non-zero value.
 */
static int32_t
xipc_cond_msg_channel_num_free(void *arg, int32_t val, void *thread)
{
  xipc_msg_channel_output_port_t *op = ((xipc_msg_channel_cond_t *)arg)->_p._op;
  uint32_t n = ((xipc_msg_channel_cond_t *)arg)->_n;
  uint32_t num_allocated = xipc_msg_channel_output_port_count(op);
  int32_t num_free = op->_num_words - 1 - num_allocated;
  return num_free < n ? 0 : 1;
}

/* Find the input port corresponding to the src proc of the channel.
 * The input port is used as the channel handle to receive messages from
 * the src proc of the channel.
 *
 * src_pid : src proc id
 *
 * Returns pointer to message channel input port
 */
xipc_msg_channel_input_port_t *
xipc_msg_channel_get_input_port(xipc_pid_t src_pid)
{
  if (xipc_msg_channels_setup == 0)
    return 0;
  int i;
  for (i = 0; i < xipc_get_num_procs()-1; i++) {
    if (xmp_xipc_msg_channel_my_input_ports[i]._src_pid == src_pid)
      return &xmp_xipc_msg_channel_my_input_ports[i];
  }
  return 0;
}

/* Find the output port corresponding to the destination proc of the channel.
 * The output port is used as the channel handle to send messages to
 * the dest proc of the channel.
 *
 * dest_pid : destination proc id
 *
 * Returns pointer to message channel output port
 */
xipc_msg_channel_output_port_t *
xipc_msg_channel_get_output_port(xipc_pid_t dest_pid)
{
  if (xipc_msg_channels_setup == 0)
    return 0;
  int i;
  for (i = 0; i < xipc_get_num_procs()-1; i++) {
    if (xmp_xipc_msg_channel_my_output_ports[i]._dest_pid == dest_pid)
      return &xmp_xipc_msg_channel_my_output_ports[i];
  }
  return 0;
}

/* Sends an 32-bit int message on the output port. Sleeps/spin waits if 
 * channel is full.
 *
 * op        : message channel output port on which message is send
 * src       : pointer to 32-bit int that is to be sent
 * wait_kind : Sleep or spin wait if channel is full. If spin waiting, 
 *             no interrupt is send to the dest proc. Expects the dest proc to
 *             not be blocked.
 *
 * Returns void
 */
void
xipc_msg_channel_send(xipc_msg_channel_output_port_t *op,
                      int32_t *src,
                      xipc_wait_kind_t wait_kind)
{
  XIPC_PROFILE_EVENT(XIPC_PROFILE_MSG_CHANNEL_SENDING, op->_channel_id);

  xipc_msg_channel_slot_ptr_t cached_write_ptr = op->_cached_write_ptr;
  int32_t *buf = op->_input_port_buffer;
  xipc_msg_channel_slot_ptr_t *ip_write_ptr = op->_input_port_write_ptr;
  uint32_t num_words = op->_num_words;
  while (xipc_msg_channel_full(op) == 0) {
    if (wait_kind == XIPC_SLEEP_WAIT) {
      uint32_t ps = xipc_disable_interrupts();
      if (xipc_msg_channel_full(op) == 0) {
        xipc_cond_thread_block(xipc_cond_msg_channel_full, op);
      }
      xipc_enable_interrupts(ps);
    }
  }
  /* Perform the data write to the circular buffer */
  int32_t *dest = buf + cached_write_ptr;
  *dest = *src;
  uint32_t next_write_ptr = xipc_modulo_add(cached_write_ptr, 1, num_words);
  /* Update local write ptr */
  op->_cached_write_ptr = next_write_ptr;
#pragma no_reorder
  /* Update write ptr on the dest proc */
  *ip_write_ptr = next_write_ptr;
  if (wait_kind == XIPC_SLEEP_WAIT) {
#pragma flush_memory
    xipc_proc_notify(op->_dest_pid);
  }

  XIPC_PROFILE_EVENT(XIPC_PROFILE_MSG_CHANNEL_SENT, op->_channel_id);
}

/* Receive a 32-bit int message on the input port. Sleeps/spin waits if channel 
 * is empty. 
 *
 * ip        : message channel input port on which message is received
 * dest      : pointer to 32-bit int where the message is received
 * wait_kind : Sleep or spin wait if channel is empty. If spin waiting, 
 *             no interrupt is send to the src proc. Expects the src proc to
 *             not be blocked.
 *
 * Returns void
 */
void
xipc_msg_channel_recv(xipc_msg_channel_input_port_t *ip,
                      int32_t *dest,
                      xipc_wait_kind_t wait_kind)
{
  XIPC_PROFILE_EVENT(XIPC_PROFILE_MSG_CHANNEL_RECEIVING, ip->_channel_id);

  xipc_msg_channel_slot_ptr_t cached_read_ptr = ip->_cached_read_ptr;
  int32_t *buf = ip->_buffer;
  xipc_msg_channel_slot_ptr_t *op_read_ptr = ip->_output_port_read_ptr;
  uint32_t num_words = ip->_num_words;
  while (xipc_msg_channel_empty(ip) == 0) {
    if (wait_kind == XIPC_SLEEP_WAIT) {
      uint32_t ps = xipc_disable_interrupts();
      if (xipc_msg_channel_empty(ip) == 0) {
        xipc_cond_thread_block(xipc_cond_msg_channel_empty, ip);
      }
      xipc_enable_interrupts(ps);
    }
  }
  /* Perfrom the read from the circular buffer */
  int32_t *src = buf + cached_read_ptr;
  *dest = *src;
  uint32_t next_read_ptr = xipc_modulo_add(cached_read_ptr, 1, num_words);
  /* Update local read ptr */
  ip->_cached_read_ptr = next_read_ptr;
#pragma no_reorder
  /* Update read ptr on the src proc */
  *op_read_ptr = next_read_ptr;
  if (wait_kind == XIPC_SLEEP_WAIT) {
#pragma flush_memory
    xipc_proc_notify(ip->_src_pid);
  }

  XIPC_PROFILE_EVENT(XIPC_PROFILE_MSG_CHANNEL_RECEIVED, ip->_channel_id);
}

/* Sends 'nwords' 32-bit int messages on the output port. Sleeps/spin waits if 
 * channel has less than nwords free space.
 *
 * op        : message channel output port on which message is send
 * src       : pointer to 32-bit int that is to be sent
 * nwords    : number of words to be sent
 * wait_kind : Sleep or spin wait. If spin waiting, 
 *             no interrupt is send to the dest proc. Expects the dest proc to
 *             not be blocked.
 *
 * Returns XIPC_OK, if successful, else returns XIPC_ERROR_INVALID_ARG
 */
xipc_status_t
xipc_msg_channel_send_n(xipc_msg_channel_output_port_t *op,
                        int32_t *src,
                        size_t nwords,
                        xipc_wait_kind_t wait_kind)
{
  XIPC_PROFILE_EVENT(XIPC_PROFILE_MSG_CHANNEL_SENDING, op->_channel_id);

  if (nwords > op->_num_words-1) {
#pragma frequency_hint NEVER
    XIPC_LOG("xipc_msg_channel_send_n: nwords (%d) cannot exceed %d\n", nwords,
             op->_num_words-1);
    return XIPC_ERROR_INVALID_ARG;
  }
  xipc_msg_channel_slot_ptr_t cached_write_ptr = op->_cached_write_ptr;
  int32_t *buf = op->_input_port_buffer;
  xipc_msg_channel_slot_ptr_t *ip_write_ptr = op->_input_port_write_ptr;
  while (op->_num_words - 1 - xipc_msg_channel_output_port_count(op) < 
         nwords) {
    if (wait_kind == XIPC_SLEEP_WAIT) {
      uint32_t ps = xipc_disable_interrupts();
      if (op->_num_words - 1 - xipc_msg_channel_output_port_count(op) < 
          nwords) {
        xipc_msg_channel_cond_t s = {._p._op = op, ._n = nwords};
        xipc_cond_thread_block(xipc_cond_msg_channel_num_free, &s);
      }
      xipc_enable_interrupts(ps);
    }
  }
  /* Perform the data write to the circular buffer */
  int32_t *dest = buf;
  int k = cached_write_ptr;
  int i;
#pragma loop_count min=1
  for (i = 0; i < nwords; i++) {
    dest[k] = src[i];
    k = xipc_modulo_add(k, 1, op->_num_words);
  }
  /* Update local write ptr */
  op->_cached_write_ptr = k;
#pragma no_reorder
  /* Update write ptr on the dest proc */
  *ip_write_ptr = k;
  if (wait_kind == XIPC_SLEEP_WAIT) {
#pragma flush_memory
    xipc_proc_notify(op->_dest_pid);
  }

  XIPC_PROFILE_EVENT(XIPC_PROFILE_MSG_CHANNEL_SENT, op->_channel_id);

  return XIPC_OK;
}

/* Receive nwords 32-bit int messages on the input port. Sleeps/spin waits if 
 * channel has less than nwords allocated.
 *
 * ip        : message channel input port on which message is received
 * dest      : pointer to 32-bit int where the message is received
 * nwords    : number of words to be received
 * wait_kind : Sleep or spin wait. If spin waiting, 
 *             no interrupt is send to the src proc. Expects the src proc to
 *             not be blocked.
 *
 * Returns XIPC_OK, if successful, else returns XIPC_ERROR_INVALID_ARG
 */
xipc_status_t
xipc_msg_channel_recv_n(xipc_msg_channel_input_port_t *ip, 
                        int32_t *dest,
                        size_t nwords,
                        xipc_wait_kind_t wait_kind)
{
  XIPC_PROFILE_EVENT(XIPC_PROFILE_MSG_CHANNEL_RECEIVING, ip->_channel_id);

  if (nwords > ip->_num_words - 1) {
#pragma frequency_hint NEVER
    XIPC_LOG("xipc_msg_channel_recv_n: nwords (%d) cannot exceed %d\n", nwords,
             ip->_num_words-1);
    return XIPC_ERROR_INVALID_ARG;
  }
  xipc_msg_channel_slot_ptr_t cached_read_ptr = ip->_cached_read_ptr;
  int32_t *buf = ip->_buffer;
  xipc_msg_channel_slot_ptr_t *op_read_ptr = ip->_output_port_read_ptr;
  while (xipc_msg_channel_input_port_count(ip) < nwords) {
    if (wait_kind == XIPC_SLEEP_WAIT) {
      uint32_t ps = xipc_disable_interrupts();
      if (xipc_msg_channel_input_port_count(ip) < nwords) {
        xipc_msg_channel_cond_t s = {._p._ip = ip, ._n = nwords};
        xipc_cond_thread_block(xipc_cond_msg_channel_num_alloc, &s);
      }
      xipc_enable_interrupts(ps);
    }
  }
  /* Perfrom the read from the circular buffer */
  int32_t *src = buf;
  int k = cached_read_ptr;
  int i;
#pragma loop_count min=1
  for (i = 0; i < nwords; i++) {
    dest[i] = src[k];
    k = xipc_modulo_add(k, 1, ip->_num_words);
  }
  /* Update local read ptr */
  ip->_cached_read_ptr = k;
#pragma no_reorder
  /* Update read ptr on the src proc */
  *op_read_ptr = k;
  if (wait_kind == XIPC_SLEEP_WAIT) {
#pragma flush_memory
    xipc_proc_notify(ip->_src_pid);
  }

  XIPC_PROFILE_EVENT(XIPC_PROFILE_MSG_CHANNEL_RECEIVED, ip->_channel_id);

  return XIPC_OK;
}

/* Allocate 'nwords' 32-bit int messages on the output port. Sleeps/spin waits 
 * if channel has less than nwords free space.
 *
 * op        : message channel output port on which message is allocated
 * nwords    : number of words to be allocated
 * wait_kind : Sleep or spin wait. 
 *
 * Returns pointer to buffer of successful, else 0
 */
int32_t *
xipc_msg_channel_allocate(xipc_msg_channel_output_port_t *op,
                          size_t nwords,
                          xipc_wait_kind_t wait_kind)
{
  XIPC_PROFILE_EVENT(XIPC_PROFILE_MSG_CHANNEL_ALLOCATING, op->_channel_id);

  if (nwords > op->_num_words-1) {
#pragma frequency_hint NEVER
    XIPC_LOG("xipc_msg_channel_allocate: nwords (%d) cannot exceed %d\n", 
             nwords, op->_num_words-1);
    return 0;
  }
  xipc_msg_channel_slot_ptr_t cached_write_ptr = op->_cached_write_ptr;
  int32_t *buf = op->_input_port_buffer;
  while (op->_num_words - 1 - xipc_msg_channel_output_port_count(op) < 
         nwords) {
    if (wait_kind == XIPC_SLEEP_WAIT) {
      uint32_t ps = xipc_disable_interrupts();
      if (op->_num_words - 1 - xipc_msg_channel_output_port_count(op) < 
          nwords) {
        xipc_msg_channel_cond_t s = {._p._op = op, ._n = nwords};
        xipc_cond_thread_block(xipc_cond_msg_channel_num_free, &s);
      }
      xipc_enable_interrupts(ps);
    }
  }

  XIPC_PROFILE_EVENT(XIPC_PROFILE_MSG_CHANNEL_ALLOCATED, op->_channel_id);

  return &buf[cached_write_ptr];
}

/* Sends 'nwords' 32-bit int messages on the output port. 
 *
 * op        : message channel output port on which message is send
 * nwords    : number of words to be sent
 * wait_kind : Sleep or spin wait. If spin waiting, 
 *             no interrupt is send to the dest proc. Expects the dest proc to
 *             not be blocked.
 *
 * Returns XIPC_OK, if successful, else returns XIPC_ERROR_INVALID_ARG
 */
xipc_status_t
xipc_msg_channel_commit(xipc_msg_channel_output_port_t *op,
                        size_t nwords,
                        xipc_wait_kind_t wait_kind)
{
  if (nwords > op->_num_words-1) {
#pragma frequency_hint NEVER
    XIPC_LOG("xipc_msg_channel_commit: nwords (%d) cannot exceed %d\n", 
             nwords, op->_num_words-1);
    return XIPC_ERROR_INVALID_ARG;
  }

  xipc_msg_channel_slot_ptr_t cached_write_ptr = op->_cached_write_ptr;
  xipc_msg_channel_slot_ptr_t *ip_write_ptr = op->_input_port_write_ptr;
  cached_write_ptr = xipc_modulo_add(cached_write_ptr, nwords, op->_num_words);
  /* Update local write ptr */
  op->_cached_write_ptr = cached_write_ptr;
#pragma no_reorder
  /* Update write ptr on the dest proc */
  *ip_write_ptr = cached_write_ptr;
  if (wait_kind == XIPC_SLEEP_WAIT) {
#pragma flush_memory
    xipc_proc_notify(op->_dest_pid);
  }

  XIPC_PROFILE_EVENT(XIPC_PROFILE_MSG_CHANNEL_COMMIT, op->_channel_id);

  return XIPC_OK;
}

/* Receives nwords 32-bit int messages on the input port. 
 *
 * ip        : message channel input port on which message is to be received
 * nwords    : number of words to be received
 * wait_kind : Sleep or spin wait. 
 *
 * Returns pointer to buffer of successful, else 0
 */
int32_t *
xipc_msg_channel_get(xipc_msg_channel_input_port_t *ip, 
                     size_t nwords,
                     xipc_wait_kind_t wait_kind)
{
  XIPC_PROFILE_EVENT(XIPC_PROFILE_MSG_CHANNEL_RECEIVING, ip->_channel_id);

  if (nwords > ip->_num_words - 1) {
#pragma frequency_hint NEVER
    XIPC_LOG("xipc_msg_channel_get: nwords (%d) cannot exceed %d\n", nwords,
             ip->_num_words-1);
    return 0;
  }
  xipc_msg_channel_slot_ptr_t cached_read_ptr = ip->_cached_read_ptr;
  int32_t *buf = ip->_buffer;
  while (xipc_msg_channel_input_port_count(ip) < nwords) {
    if (wait_kind == XIPC_SLEEP_WAIT) {
      uint32_t ps = xipc_disable_interrupts();
      if (xipc_msg_channel_input_port_count(ip) < nwords) {
        xipc_msg_channel_cond_t s = {._p._ip = ip, ._n = nwords};
        xipc_cond_thread_block(xipc_cond_msg_channel_num_alloc, &s);
      }
      xipc_enable_interrupts(ps);
    }
  }

  XIPC_PROFILE_EVENT(XIPC_PROFILE_MSG_CHANNEL_RECEIVED, ip->_channel_id);

  return &buf[cached_read_ptr];
}

/* Releases nwords 32-bit int messages on the input port. Note, the release
 * has to be done in the order of the corresponding 'get'.
 *
 * ip        : message channel input port on which message is released
 * nwords    : number of words to be released
 * wait_kind : Sleep or spin wait. If spin waiting, 
 *             no interrupt is send to the src proc. Expects the src proc to
 *             not be blocked.
 *
 * Returns XIPC_OK, if successful, else returns XIPC_ERROR_INVALID_ARG
 */
xipc_status_t
xipc_msg_channel_release(xipc_msg_channel_input_port_t *ip, 
                         size_t nwords,
                         xipc_wait_kind_t wait_kind)
{
  if (nwords > ip->_num_words - 1) {
#pragma frequency_hint NEVER
    XIPC_LOG("xipc_msg_channel_release: nwords (%d) cannot exceed %d\n", nwords,
             ip->_num_words-1);
    return XIPC_ERROR_INVALID_ARG;
  }
  xipc_msg_channel_slot_ptr_t cached_read_ptr = ip->_cached_read_ptr;
  xipc_msg_channel_slot_ptr_t *op_read_ptr = ip->_output_port_read_ptr;
  cached_read_ptr = xipc_modulo_add(cached_read_ptr, nwords, ip->_num_words);
  /* Update local read ptr */
  ip->_cached_read_ptr = cached_read_ptr;
#pragma no_reorder
  /* Update read ptr on the src proc */
  *op_read_ptr = cached_read_ptr;
  if (wait_kind == XIPC_SLEEP_WAIT) {
#pragma flush_memory
    xipc_proc_notify(ip->_src_pid);
  }

  XIPC_PROFILE_EVENT(XIPC_PROFILE_MSG_CHANNEL_RELEASED, ip->_channel_id);

  return XIPC_OK;
}

/* Initiate connection to the src side of the channel through the input port
 *
 * ch        : message channel for connect
 * ip        : input port 
 *
 * Returns void
 */
void
xipc_msg_channel_input_port_connect(xipc_msg_channel_t *ch,
                                    xipc_msg_channel_input_port_t *ip)
{
  /* Translate the address of the dest proc's write ptr to global space */
  xipc_msg_channel_slot_ptr_t *laddr = (xipc_msg_channel_slot_ptr_t *)
                                       &(ip->_write_ptr);
  xipc_msg_channel_slot_ptr_t *gaddr = (xipc_msg_channel_slot_ptr_t *)
                                       xipc_get_sys_addr((void *)laddr, 
                                                         xipc_get_proc_id());
  int32_t *laddr_b = ip->_buffer;
  int32_t *gaddr_b = (int32_t *)xipc_get_sys_addr((void *)laddr_b,
                                                  xipc_get_proc_id());

#if XCHAL_DCACHE_SIZE > 0 && !XCHAL_DCACHE_IS_COHERENT
  /* Invalidate the message channel and update its contents */
  xthal_dcache_region_invalidate((void *)ch, sizeof(xipc_msg_channel_t));
#endif

  ch->_input_port_write_ptr = gaddr;
  ch->_input_port_buffer = gaddr_b;
  ch->_num_words = ip->_num_words;
#pragma flush_memory
  ch->_input_port_connected = 1;

#if XCHAL_DCACHE_SIZE > 0 && !XCHAL_DCACHE_IS_COHERENT
  /* Writeback changes on the message channel */
  xthal_dcache_region_writeback((void *)ch, sizeof(xipc_msg_channel_t));
#endif
}

/* Accept connection from the src side of the channel using the input port
 *
 * ch : message channel for connect
 * ip : input port 
 *
 * Returns void
 */
void
xipc_msg_channel_input_port_accept(xipc_msg_channel_t *ch,
                                   xipc_msg_channel_input_port_t *ip)
{
  ip->_output_port_read_ptr = (xipc_msg_channel_slot_ptr_t *)
                              xipc_load((volatile uint32_t *)
                                        (void *)&ch->_output_port_read_ptr);
  ip->_channel_id = ch->_channel_id;
}

/* Initiate connection to the dest side of the channel through the output port
 *
 * ch        : message channel for connect
 * op        : output port 
 *
 * Returns void
 */
void
xipc_msg_channel_output_port_connect(xipc_msg_channel_t *ch,
                                     xipc_msg_channel_output_port_t *op)
{
  /* Translate the address of the src proc's read ptr to global space */
  xipc_msg_channel_slot_ptr_t *laddr = (xipc_msg_channel_slot_ptr_t *)
                                       &(op->_read_ptr);
  xipc_msg_channel_slot_ptr_t *gaddr = (xipc_msg_channel_slot_ptr_t *)
                                       xipc_get_sys_addr((void *)laddr,
                                                         xipc_get_proc_id());

  /* Invalidate the message channel and update its contents */
#if XCHAL_DCACHE_SIZE > 0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_region_invalidate((void *)ch, sizeof(xipc_msg_channel_t));
#endif

  ch->_output_port_read_ptr = gaddr;
#pragma flush_memory
  ch->_output_port_connected = 1;

  /* Writeback changes on the message channel */
#if XCHAL_DCACHE_SIZE > 0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_region_writeback((void *)ch, sizeof(xipc_msg_channel_t));
#endif
}

/* Accept connection from the dest side of the channel using the output port
 *
 * ch : message channel for connect
 * op : output port 
 *
 * Returns void
 */
void
xipc_msg_channel_output_port_accept(xipc_msg_channel_t *ch,
                                    xipc_msg_channel_output_port_t *op)
{
  op->_input_port_buffer = (void *)xipc_load((volatile uint32_t *)
                                             (void *)&ch->_input_port_buffer);
  op->_input_port_write_ptr = 
                       (void *)xipc_load((volatile uint32_t *)
                                         (void *)&ch->_input_port_write_ptr);
  op->_num_words = xipc_load(&ch->_num_words);
  op->_channel_id = ch->_channel_id;
}

/* Initialize the message channel.
 *
 * msg_channel : message channel to be initialized
 * src_pid     : proc id of the source of the channel
 * dest_pid    : proc id of the destination of the channel
 * channel_id  : unique channel id. Note, the subsystem reserves pre-defined
 *               inter-core message channels between every pair of cores in the
 *               subsystem. So the channel id between 0 and 
 *               (number of processor in the subsystem *
 *                 (number of processors in the subsystem - 1)
 *               are reserved and should not be used.
 *
 * Returns XIPC_OK, if successful, else returns XIPC_ERROR_INTERNAL
 */
xipc_status_t
xipc_msg_channel_initialize(xipc_msg_channel_t *msg_channel,
                            xipc_pid_t src_pid,
                            xipc_pid_t dest_pid,
                            uint16_t channel_id)
{
  if (sizeof(xipc_msg_channel_t) != XIPC_MSG_CHANNEL_STRUCT_SIZE) {
    XIPC_LOG("xipc_msg_channel_init:  Internal Error!. Expecting sizeof(xipc_msg_channel_t) to be %d, but got size %d", XIPC_MSG_CHANNEL_STRUCT_SIZE, sizeof(xipc_msg_channel_t));
    return XIPC_ERROR_INTERNAL;
  }

  if (sizeof(xipc_msg_channel_input_port_t) != 
      XIPC_MSG_CHANNEL_INPUT_PORT_STRUCT_SIZE) {
    XIPC_LOG("xipc_msg_channel_init:  Internal Error!. Expecting sizeof(xipc_msg_channel_input_port_t) to be %d, but got size %d", XIPC_MSG_CHANNEL_INPUT_PORT_STRUCT_SIZE, sizeof(xipc_msg_channel_input_port_t));
    return XIPC_ERROR_INTERNAL;
  }

  if (sizeof(xipc_msg_channel_output_port_t) != 
      XIPC_MSG_CHANNEL_OUTPUT_PORT_STRUCT_SIZE) {
    XIPC_LOG("xipc_msg_channel_init:  Internal Error!. Expecting sizeof(xipc_msg_channel_output_port_t) to be %d, but got size %d", XIPC_MSG_CHANNEL_OUTPUT_PORT_STRUCT_SIZE, sizeof(xipc_msg_channel_output_port_t));
    return XIPC_ERROR_INTERNAL;
  }

  if (msg_channel == NULL) {
    XIPC_LOG("xipc_msg_channel_init: param msg_channel is NULL\n");
    return XIPC_ERROR_INVALID_ARG;
  }

  msg_channel->_src_pid = src_pid;
  msg_channel->_dest_pid = dest_pid;
  msg_channel->_output_port_read_ptr = 0;
  msg_channel->_input_port_write_ptr = 0;
  msg_channel->_input_port_buffer = 0;
  msg_channel->_input_port_connected = 0;
  msg_channel->_output_port_connected = 0;
  msg_channel->_num_words = 0;
  msg_channel->_channel_id = channel_id;
#if XCHAL_DCACHE_SIZE > 0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_region_writeback((void *)msg_channel, 
                                sizeof(xipc_msg_channel_t));
#endif
  return XIPC_OK;
}

/* Initialize the input port of the message channel
 *
 * ip        : input port to be initialized
 * src_pid   : proc id of the source of the channel
 * num_words : number of 4-byte words in the message channel. Note, the number
 *             words available for allocation will be 1 less than this.
 * buffer    : pointer to message buffer
 *
 * Returns XIPC_OK if successful else XIPC_ERROR_INVALID_ARG.
 */
xipc_status_t
xipc_msg_channel_input_port_initialize(xipc_msg_channel_input_port_t *ip,
                                       xipc_pid_t src_pid,
                                       int32_t *buffer,
                                       uint32_t num_words)
{
  if (ip == NULL) {
    XIPC_LOG("xipc_msg_channel_input_port_initialize: ip is NULL\n");
    return XIPC_ERROR_INVALID_ARG;
  }

  if (buffer == NULL) {
    XIPC_LOG("xipc_msg_channel_input_port_initialize: buffer is NULL\n");
    return XIPC_ERROR_INVALID_ARG;
  }

  if (num_words < 2) {
    XIPC_LOG("xipc_msg_channel_input_port_initialize: num_words %d needs to be atleast 2\n", num_words);
    return XIPC_ERROR_INVALID_ARG;
  }

  ip->_src_pid = src_pid;
  ip->_write_ptr = 0;
  ip->_cached_read_ptr = 0;
  ip->_output_port_read_ptr = 0;
  ip->_num_words = num_words;
  ip->_buffer = buffer;
  return XIPC_OK;
}

/* Initialize the output port of the message channel
 *
 * op       : output port to be initialized
 * dest_pid : proc id of the destination of the channel
 *
 * Returns XIPC_OK if successful else XIPC_ERROR_INVALID_ARG.
 */
xipc_status_t
xipc_msg_channel_output_port_initialize(xipc_msg_channel_output_port_t *op,
                                        xipc_pid_t dest_pid)
{
  if (op == NULL) {
    XIPC_LOG("xipc_msg_channel_output_port_initialize: op is NULL\n");
    return XIPC_ERROR_INVALID_ARG;
  }
  op->_dest_pid = dest_pid;
  op->_read_ptr = 0;
  op->_cached_write_ptr = 0;
  op->_input_port_write_ptr = 0;
  op->_input_port_buffer = 0;
  return XIPC_OK;
}

/* Returns the src proc id of the channel */
xipc_pid_t xipc_msg_channel_get_src_pid(xipc_msg_channel_t *ch)
{
#if XCHAL_DCACHE_SIZE > 0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_line_invalidate(&ch->_src_pid);
#endif
  return ch->_src_pid;
}

/* Returns the dest proc id of the channel */
xipc_pid_t xipc_msg_channel_get_dest_pid(xipc_msg_channel_t *ch)
{
#if XCHAL_DCACHE_SIZE > 0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_line_invalidate(&ch->_dest_pid);
#endif
  return ch->_dest_pid;
}

/* Check if src side of the channel (output port) is connected 
 *
 * ch : message channel
 *
 * Returns 1 if connected, else returns 0
 */
uint32_t xipc_msg_channel_output_port_connected(xipc_msg_channel_t *ch)
{
  return xipc_load(&ch->_output_port_connected);
}

/* Check if dest side of the channel (input port) is connected 
 *
 * ch : message channel
 *
 * Returns 1 if connected, else returns 0
 */
uint32_t xipc_msg_channel_input_port_connected(xipc_msg_channel_t *ch)
{
  return xipc_load(&ch->_input_port_connected);
}

/* Returns pointer to underlying buffer on the src processor
 *
 * op : message channel output port
 * 
 * Returns pointer to buffer
 */
int32_t *
xipc_msg_channel_output_port_buffer(xipc_msg_channel_output_port_t *op)
{
  return op->_input_port_buffer;
}

/* Returns pointer to underlying buffer on the dest processor
 *
 * ip : message channel input port
 * 
 * Returns pointer to buffer
 */
int32_t *
xipc_msg_channel_input_port_buffer(xipc_msg_channel_input_port_t *ip)
{
  return ip->_buffer;
}

/* Returns the channel id of the channel */
uint16_t 
xipc_msg_channel_get_id(xipc_msg_channel_t *ch)
{
#if XCHAL_DCACHE_SIZE > 0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_region_invalidate((void *)ch, sizeof(xipc_msg_channel_t));
#endif
  return ch->_channel_id;
}

/* Returns the channel id of the channel */
uint16_t 
xipc_msg_channel_input_port_get_id(xipc_msg_channel_input_port_t *ip)
{
  return ip->_channel_id;
}

/* Returns the channel id of the channel */
uint16_t 
xipc_msg_channel_output_port_get_id(xipc_msg_channel_output_port_t *op)
{
  return op->_channel_id;
}

void xipc_msg_channel_print_input_port(xipc_msg_channel_input_port_t *ip)
{
  XIPC_LOG("src_pid: %d, write_ptr: %d(0x%x), cached_read_ptr: %d, op_read_ptr: 0x%x\n", (int)ip->_src_pid, (int)ip->_write_ptr, (int)&ip->_write_ptr, (int)ip->_cached_read_ptr, (int)ip->_output_port_read_ptr);
}

void xipc_msg_channel_print_output_port(xipc_msg_channel_output_port_t *op)
{
  XIPC_LOG("dest_pid: %d, read_ptr: %d(0x%x), cached_write_ptr: %d, ip_write_ptr: 0x%x\n", (int)op->_dest_pid, (int)op->_read_ptr, (int)&op->_read_ptr, (int)op->_cached_write_ptr, (int)op->_input_port_write_ptr);
}

#ifdef XIPC_DEBUG
void xipc_msg_channel_print_channel(xipc_msg_channel_t *ch)
{
#if XCHAL_DCACHE_SIZE > 0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_region_invalidate((void *)ch, sizeof(xipc_msg_channel_t));
#endif
  XIPC_LOG("ch.src_pid      %d\n", ch->_src_pid);
  XIPC_LOG("ch.dest_pid     %d\n", ch->_dest_pid);
  XIPC_LOG("ch.op_read_ptr  %p\n", ch->_output_port_read_ptr);
  XIPC_LOG("ch.ip_write_ptr %p\n", ch->_input_port_write_ptr);
  XIPC_LOG("ch.ip_buffer    %p\n", ch->_input_port_buffer);
  XIPC_LOG("ch.ip_connected %d\n", ch->_input_port_connected);
  XIPC_LOG("ch.op_connected %d\n", ch->_output_port_connected);
}
#endif
