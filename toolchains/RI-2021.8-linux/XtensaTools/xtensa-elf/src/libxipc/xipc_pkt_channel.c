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
#include <string.h>
#if XCHAL_HAVE_IDMA
#include <xtensa/idma.h>
#endif
#include "xipc_addr.h"
#include "xipc_misc.h"
#include "xipc_msg_channel_internal.h"
#include "xipc_pkt_channel_internal.h"

/* Checks on the output port (src proc side) if the packet channel is full 
 *
 * op : pointer to the packet channel output port
 *
 * Returns 0, if full, else returns a non-zero value
 */ 
XIPC_INLINE int32_t xipc_pkt_channel_full(xipc_pkt_channel_output_port_t *op)
{
  /* Channel is full if the output port's free packet queue is empty */
  xipc_pkt_slot_ptr_t head = op->_head;
  xipc_pkt_slot_ptr_t tail = op->_tail;
  return head - tail;
}

/* Returns the number of free packets in the channel
 *
 * op : pointer to the packet channel output port
 *
 * Returns free packet count
 */
XIPC_INLINE uint32_t
xipc_pkt_channel_output_port_count(xipc_pkt_channel_output_port_t *op)
{
  return xipc_channel_count(op->_head, op->_tail, XIPC_PKT_CHANNEL_NUM_SLOTS+1);
}

/* Returns the number of allocated packets in the channel
 *
 * ip : pointer to the packet channel input port
 *
 * Returns allocated packet count
 */
XIPC_INLINE uint32_t
xipc_pkt_channel_input_port_count(xipc_pkt_channel_input_port_t *ip)
{
  return xipc_channel_count(ip->_head, ip->_tail, XIPC_PKT_CHANNEL_NUM_SLOTS+1);
}
  
/* Checks on the input port (dest proc side) if the packet channel is empty 
 *
 * ip : pointer to the packet channel input port
 *
 * Returns 0, if empty, else returns a non-zero value
 */ 
XIPC_INLINE int32_t 
xipc_pkt_channel_empty(xipc_pkt_channel_input_port_t *ip)
{
  /* Channel is empty if the input port's ready packet queue is empty */
  xipc_pkt_slot_ptr_t head = ip->_head;
  xipc_pkt_slot_ptr_t tail = ip->_tail;
  return head - tail;
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
xipc_cond_pkt_channel_empty(void *ip, int32_t val, void *thread)
{
  return xipc_pkt_channel_empty((xipc_pkt_channel_input_port_t *)ip);
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
xipc_cond_pkt_channel_full(void *op, int32_t val, void *thread)
{
  return xipc_pkt_channel_full((xipc_pkt_channel_output_port_t *)op);
}

/* Checks if channel has n free elements
 *
 * arg    : pointer to struct with channel output port and num elements
 * val    : ignored
 * thread : pointer to thread structure
 *
 * Returns 0 if the channel has less than n elements , i.e the condition is 
 * not satisfied; else returns non-zero value.
 */
static int32_t
xipc_cond_pkt_channel_num_free(void *arg, int32_t val, void *thread)
{
  xipc_pkt_channel_output_port_t *op = ((xipc_pkt_channel_cond_t *)arg)->_p._op;
  uint32_t n = ((xipc_pkt_channel_cond_t *)arg)->_n;
  return xipc_pkt_channel_output_port_count(op) < n ? 0 : 1;
}

/* Checks if channel has n allocated elements
 *
 * arg    : pointer to struct with channel input port and num elements
 * val    : ignored
 * thread : pointer to thread structure
 *
 * Returns 0 if the channel has less than n elements , i.e the condition is 
 * not satisfied; else returns non-zero value.
 */
static int32_t
xipc_cond_pkt_channel_num_alloc(void *arg, int32_t val, void *thread)
{
  xipc_pkt_channel_input_port_t *ip = ((xipc_pkt_channel_cond_t *)arg)->_p._ip;
  uint32_t n = ((xipc_pkt_channel_cond_t *)arg)->_n;
  return xipc_pkt_channel_input_port_count(ip) < n ? 0 : 1;
}

/* Perform non-blocking copy
 *
 * dest                  : destination address of the copy
 * src                   : source address of the copy
 * num_bytes             : number of bytes to copy
 * post_copy_callback    : callback function invoked after copy
 * req                   : request structure to query for status of copy
 * notify_with_interrupt : interrupt at end of copy
 *
 * Returns XIPC_OK if successful, else returns XIPC_ERROR_ASYNC_COPY.
 */
#if XCHAL_HAVE_IDMA
static XIPC_INLINE xipc_status_t 
xipc_pkt_channel_async_copy(void *dest, 
                            void *src,
                            size_t num_bytes,
                            void (*post_copy_callback)(void *),
                            xipc_pkt_channel_request_t *req,
                            int8_t notify_with_interrupt)
{
  XIPC_LOG("xipc_pkt_channel_async_copy: dest: %p src: %p, num_bytes: %d\n",
           dest, src, num_bytes);
  // Invoke idma to do the copy 
  int32_t flags = DESC_IDMA_PRIOR_H|DESC_NOTIFY_W_INT;
  idma_status_t err = idma_copy_task(req->_idma_task, dest, src, num_bytes, 
                                flags, (void *)req, post_copy_callback);

  if (err != IDMA_OK)
    return XIPC_ERROR_ASYNC_COPY;

  if (idma_task_status(req->_idma_task) < 0)
    return XIPC_ERROR_ASYNC_COPY;

  return XIPC_OK;
}
#else
/* Performs synchronous memcpy and invoke the callback 
 *
 * dest          : destination address of the copy
 * src           : source address of the copy
 * num_bytes     : number of bytes to copy
 * callback_data : data to be passed to callback
 * callback_func : callback function invoked after copy
 */
static void
sync_copy(void *dest,
          void *src,
          size_t size,
          void *callback_data,
          void (*callback_func)(void *))
{
  memcpy(dest, src, size);
#pragma flush_memory
  (*callback_func)(callback_data);
}

static XIPC_INLINE xipc_status_t 
xipc_pkt_channel_async_copy(void *dest, 
                            void *src,
                            size_t num_bytes,
                            void (*post_copy_callback)(void *),
                            xipc_pkt_channel_request_t *req,
                            int8_t notify_with_interrupt)
{
  XIPC_LOG("xipc_pkt_channel_async_copy: dest: %p src: %p, num_bytes: %d\n",
           dest, src, num_bytes);
  // Peform synchronous copy
  sync_copy(dest, src, num_bytes, (void *)req, post_copy_callback);
  return XIPC_OK;
}
#endif

/* Initialize packet channel attribute associated with the input/output port.
 * The attributes are set by sending messages between the src/dest processors.
 * The attribute includes the location of the buffer, number of packets in
 * the buffer and the size of each packet. The attribute has to be set
 * either at the src or the dest. If set on both sides, 
 * XIPC_ERROR_PKT_CHANNEL_DUPL_ATTR error is returned. Simiarly, if the
 * attribute is not set on either side, XIPC_ERROR_PKT_CHANNEL_UNDEF_* error
 * is returned. If the number of packets are not set, the default value
 * value of XIPC_PKT_CHANNEL_NUM_SLOTS is used.
 *
 * attr : attributes at the caller, which can be a src or dest
 * from : message channel handle to receive a message from the proc at the
 *        other end of the packet channel
 * to   : message channel handle to send a message to the proc at the
 *        other end of the packet channel
 *
 * Retruns XIPC_OK, if successful else returns one of
 *         XIPC_ERROR_PKT_CHANNEL_DUPL_ATTR, 
 *         XIPC_ERROR_PKT_CHANNEL_UNDEF_BUFFER_ADDR_ATTR
 *         XIPC_ERROR_PKT_CHANNEL_UNDEF_PACKET_SIZE_ATTR
 */
static xipc_status_t 
xipc_pkt_channel_init_attr(xipc_pkt_channel_attr_t *attr, 
                           xipc_msg_channel_input_port_t *from,
                           xipc_msg_channel_output_port_t *to)
{
  void *gaddr;
  /* Send the attributes */
  xipc_msg_channel_send(to, (int32_t *)&attr->_mask, XIPC_SPIN_WAIT);

  /* Translate and send the address of the buffer */
  gaddr = xipc_get_sys_addr((void *)attr->_val._buffer, xipc_get_proc_id());
  xipc_msg_channel_send(to, (int32_t *)(void *)&gaddr, XIPC_SPIN_WAIT);

  /* Send the number of packets */
  int32_t num_packets = attr->_val._num_packets;
  xipc_msg_channel_send(to, &num_packets, XIPC_SPIN_WAIT);

  /* Send the packet size */
  int32_t packet_size = attr->_val._packet_size;
  xipc_msg_channel_send(to, &packet_size, XIPC_SPIN_WAIT);

  /* Receive the attributes */
  uint32_t attr_mask = 0;
  xipc_msg_channel_recv(from, (int32_t *)&attr_mask, XIPC_SPIN_WAIT);

  /* Receive the address of the buffer */
  void *buffer;
  xipc_msg_channel_recv(from, (int32_t *)(void *)&buffer, XIPC_SPIN_WAIT);

  /* Receive the number of packets */
  xipc_msg_channel_recv(from, &num_packets, XIPC_SPIN_WAIT);

  /* Receive the packet size */
  xipc_msg_channel_recv(from, &packet_size, XIPC_SPIN_WAIT);

  /* Buffer address attribute cannot be set on both the sides */
  if (((attr->_mask & XIPC_PKT_CHANNEL_BUFFER_ADDR_ATTR) != 0) && 
      ((attr_mask & XIPC_PKT_CHANNEL_BUFFER_ADDR_ATTR) != 0)) {
    XIPC_LOG("xipc_pkt_channel_init_attr: Duplicate buffer address\n");
    return XIPC_ERROR_PKT_CHANNEL_DUPL_ATTR;
  }

  /* Set buffer address attribute */
  if ((attr->_mask & XIPC_PKT_CHANNEL_BUFFER_ADDR_ATTR) == 0) {
    if ((attr_mask & XIPC_PKT_CHANNEL_BUFFER_ADDR_ATTR) == 0) {
      XIPC_LOG("xipc_pkt_channel_init_attr: Buffer address not defined\n");
      return XIPC_ERROR_PKT_CHANNEL_UNDEF_BUFFER_ADDR_ATTR;
    } else {
      attr->_val._buffer = buffer;
    }
  }

  /* Number of packets attribute cannot be set on both the sides */
  if (((attr->_mask & XIPC_PKT_CHANNEL_NUM_PACKETS_ATTR) != 0) && 
      ((attr_mask & XIPC_PKT_CHANNEL_NUM_PACKETS_ATTR) != 0)) {
    XIPC_LOG("xipc_pkt_channel_init_attr: Duplicate value for num packets\n");
    return XIPC_ERROR_PKT_CHANNEL_DUPL_ATTR;
  }

  /* Set number of packets attribute. Set to default if not defined at
   * both the ends */
  if ((attr->_mask & XIPC_PKT_CHANNEL_NUM_PACKETS_ATTR) == 0) {
    if ((attr_mask & XIPC_PKT_CHANNEL_NUM_PACKETS_ATTR) == 0) {
      attr->_val._num_packets = XIPC_PKT_CHANNEL_NUM_SLOTS;
    } else {
      attr->_val._num_packets = num_packets;
    }
  }

  /* Packet size attribute cannot be set on both the sides */
  if (((attr->_mask & XIPC_PKT_CHANNEL_PACKET_SIZE_ATTR) != 0) && 
      ((attr_mask & XIPC_PKT_CHANNEL_PACKET_SIZE_ATTR) != 0)) {
    XIPC_LOG("xipc_pkt_channel_init_attr: Duplicate value for packet size\n");
    return XIPC_ERROR_PKT_CHANNEL_DUPL_ATTR;
  }

  /* Set the packet size attribute */
  if ((attr->_mask & XIPC_PKT_CHANNEL_PACKET_SIZE_ATTR) == 0) {
    if ((attr_mask & XIPC_PKT_CHANNEL_PACKET_SIZE_ATTR) == 0) {
      XIPC_LOG("xipc_pkt_channel_init_attr: Packet size not defined\n");
      return XIPC_ERROR_PKT_CHANNEL_UNDEF_PACKET_SIZE_ATTR;
    } else {
      attr->_val._packet_size = packet_size;
    }
  }

  XIPC_LOG("Initializing packet channel attribute of %s with %s\n",
           xipc_get_proc_name(xipc_get_proc_id()), 
           xipc_get_proc_name(from->_src_pid));

  return XIPC_OK;
}

/* Connect the dest proc to the channel
 *
 * ip : handle on the dest side of the channel
 * 
 * Returns XIPC_OK, if successful else returns XIPC_ERROR_CHANNEL_CONNECT or
 * XIPC_ERROR_INTERNAL
 */
xipc_status_t
xipc_pkt_channel_input_port_connect(xipc_pkt_channel_input_port_t *ip)
{
  void *gaddr;
  /* Get a handle on the message channel to src proc of this packet channel */
  xipc_msg_channel_output_port_t *to_src =
                          xipc_msg_channel_get_output_port(ip->_src_pid);

  if (to_src == 0) {
    XIPC_LOG("xipc_pkt_channel_input_port_connect: Cannot find message channel to proc %s\n", xipc_get_proc_name(ip->_src_pid));
    return XIPC_ERROR_INTERNAL;
  }

  /* Get a handle on the message channel from src proc of this packet channel */
  xipc_msg_channel_input_port_t *from_src =
                         xipc_msg_channel_get_input_port(ip->_src_pid);

  if (from_src == 0) {
    XIPC_LOG("xipc_pkt_channel_input_port_connect: Cannot find message channel from proc %s\n", xipc_get_proc_name(ip->_src_pid));
    return XIPC_ERROR_INTERNAL;
  }

  /* Send the channel id. This is used to verify that the connection request
   * from the other end is to the same channel */
  int32_t cons_cid = ip->_channel_id;
  xipc_msg_channel_send(to_src, &cons_cid, XIPC_SPIN_WAIT);

  /* Receive the channel id from the producer core. */
  int32_t prod_cid;
  xipc_msg_channel_recv(from_src, &prod_cid, XIPC_SPIN_WAIT);

  /* Verify that the connection is to the same channel */
  if (cons_cid != prod_cid) {
    XIPC_LOG("xipc_pkt_channel_input_port_connect: Expecting connection request to channel id %d from producer proc %s, but got request to channel id %d\n", cons_cid, xipc_get_proc_name(ip->_src_pid), prod_cid);
    return XIPC_ERROR_CHANNEL_CONNECT;
  }

  /* Translate and send the address of the ready_packets queue to the 
   * src proc */
  gaddr = xipc_get_sys_addr((void *)&ip->_ready_packets[0], xipc_get_proc_id());
  xipc_msg_channel_send(to_src, (int32_t *)(void *)&gaddr, XIPC_SPIN_WAIT);

  /* Receive the address of the free packets queue from the src proc */
  xipc_pkt_slot_t *free_packets_ptr;
  xipc_msg_channel_recv(from_src, (int32_t *)(void *)&free_packets_ptr, 
                        XIPC_SPIN_WAIT);
  ip->_output_port_free_packets = free_packets_ptr;

  /* Translate and send the address of the tail to the src proc */
  gaddr = xipc_get_sys_addr((void *)&ip->_tail, xipc_get_proc_id());
  xipc_msg_channel_send(to_src, (int32_t *)(void *)&gaddr, XIPC_SPIN_WAIT);

  /* Receive the address of the tail from the src proc */
  xipc_pkt_slot_ptr_t *output_port_tail;
  xipc_msg_channel_recv(from_src, (int32_t *)(void *)&output_port_tail, 
                        XIPC_SPIN_WAIT);
  ip->_output_port_tail = output_port_tail;

  /* Handle the attributes  */
  xipc_status_t e = xipc_pkt_channel_init_attr(&ip->_attr, from_src, to_src);
  if (e)
    return e;

  /* Point tail to the slot after the last packet */
  ip->_cached_output_port_tail = ip->_attr._val._num_packets;

  XIPC_LOG("Connected packet channel (src:%s, dest:%s) input port (op_free_pkts: %p op_tail: %p), buffer: %p, num_packets: %d packet_size: %d\n",
           xipc_get_proc_name(ip->_src_pid), 
           xipc_get_proc_name(xipc_get_proc_id()),
           (void *)ip->_output_port_free_packets,
           (void *)ip->_output_port_tail,
           ip->_attr._val._buffer,
           ip->_attr._val._num_packets,
           ip->_attr._val._packet_size);

  return XIPC_OK;
}

/* Connect the src proc to the channel
 *
 * op : handle on the src side of the channel
 * 
 * Returns XIPC_OK, if successful else returns XIPC_ERROR_CHANNEL_CONNECT or
 * XIPC_ERROR_INTERNAL.
 */
xipc_status_t
xipc_pkt_channel_output_port_connect(xipc_pkt_channel_output_port_t *op)
{
  void *gaddr;
  /* Get a handle on the message channel to dest proc of this packet channel */
  xipc_msg_channel_output_port_t *to_dest =
                          xipc_msg_channel_get_output_port(op->_dest_pid);

  if (to_dest == 0) {
    XIPC_LOG("xipc_pkt_channel_output_port_connect: Cannot find message channel to proc %s\n", xipc_get_proc_name(op->_dest_pid));
    return XIPC_ERROR_INTERNAL;
  }
   
  /* Get a handle on the message channel from dest proc of this channel */
  xipc_msg_channel_input_port_t *from_dest =
                         xipc_msg_channel_get_input_port(op->_dest_pid);

  if (from_dest == 0) {
    XIPC_LOG("xipc_pkt_channel_output_port_connect: Cannot find message channel from proc %s\n", xipc_get_proc_name(op->_dest_pid));
    return XIPC_ERROR_INTERNAL;
  }

  /* Send the channel id. This is used to verify that the connection request
   * from the other end is to the same channel */
  int32_t prod_cid = op->_channel_id;
  xipc_msg_channel_send(to_dest, &prod_cid, XIPC_SPIN_WAIT);
    
  /* Receive the channel id from the consumer core. */
  int32_t cons_cid;
  xipc_msg_channel_recv(from_dest, &cons_cid, XIPC_SPIN_WAIT);
  
  /* Verify that the connection is to the same channel */
  if (cons_cid != prod_cid) {
    XIPC_LOG("xipc_pkt_channel_output_port_connect: Expecting connection request to channel id %d from consumer proc %s, but got request to channel id %d\n", prod_cid, xipc_get_proc_name(op->_dest_pid), cons_cid);
    return XIPC_ERROR_CHANNEL_CONNECT;
  }

  /* Translate and send the address of the free_packets queue to the 
   * dest proc */
  gaddr = xipc_get_sys_addr((void *)&op->_free_packets[0], xipc_get_proc_id());
  xipc_msg_channel_send(to_dest, (int32_t *)(void *)&gaddr, XIPC_SPIN_WAIT);

  /* Receive the address of the ready packets queue from the dest proc */
  xipc_pkt_slot_t *ready_packets_ptr;
  xipc_msg_channel_recv(from_dest, (int32_t *)(void *)&ready_packets_ptr, 
                        XIPC_SPIN_WAIT);
  op->_input_port_ready_packets = ready_packets_ptr;

  /* Translate and send the address of the tail to the dest proc */
  gaddr = xipc_get_sys_addr((void *)&op->_tail, xipc_get_proc_id());
  xipc_msg_channel_send(to_dest, (int32_t *)(void *)&gaddr, XIPC_SPIN_WAIT);

  /* Receive the address of the tail from the dest proc */
  xipc_pkt_slot_ptr_t *input_port_tail;
  xipc_msg_channel_recv(from_dest, (int32_t *)(void *)&input_port_tail, 
                        XIPC_SPIN_WAIT);
  op->_input_port_tail = input_port_tail;

  /* Handle the attributes  */
  xipc_status_t e = xipc_pkt_channel_init_attr(&op->_attr, from_dest, to_dest);
  if (e)
    return e;

  /* Initialize free_packets with the offset of each packet */
  uint16_t i;
  for (i = 0; i < op->_attr._val._num_packets; i++)
    op->_free_packets[i]._pkt_ptr = i * op->_attr._val._packet_size;

  /* Point tail to the slot after the last packet */
  op->_tail = op->_attr._val._num_packets;

  XIPC_LOG("Connected packet channel (src:%s, dest:%s) output port (ip_ready_pkts: %p ip_tail: %p), buffer: %p, num_packets: %d, packet_size: %d\n",
           xipc_get_proc_name(xipc_get_proc_id()), 
           xipc_get_proc_name(op->_dest_pid),
           op->_input_port_ready_packets,
           op->_input_port_tail,
           op->_attr._val._buffer,
           op->_attr._val._num_packets,
           op->_attr._val._packet_size);

  return XIPC_OK;
}

/* Initalize the input port of the packet channel
 *
 * ip         : input port to be initialized
 * channel_id : channel id that is unique across the subsystem
 * src_pid    : proc id of the src proc of the channel
 */
xipc_status_t 
xipc_pkt_channel_input_port_initialize(xipc_pkt_channel_input_port_t *ip,
                                       uint16_t channel_id,
                                       xipc_pid_t src_pid)
{
  if (sizeof(xipc_pkt_channel_input_port_t) != 
      XIPC_PKT_CHANNEL_INPUT_PORT_STRUCT_SIZE) {
    XIPC_LOG("xipc_pkt_channel_input_port_initialize:  Internal Error!. Expecting sizeof(xipc_pkt_channel_input_port_t) to be %d, but got size %d", XIPC_PKT_CHANNEL_INPUT_PORT_STRUCT_SIZE, sizeof(xipc_pkt_channel_input_port_t));
    return XIPC_ERROR_INTERNAL;
  }

  if (sizeof(xipc_pkt_channel_request_t) != 
      XIPC_PKT_CHANNEL_REQUEST_STRUCT_SIZE) {
    XIPC_LOG("xipc_pkt_channel_input_port_initialize:  Internal Error!. Expecting sizeof(xipc_pkt_channel_request_t) to be %d, but got size %d", XIPC_PKT_CHANNEL_REQUEST_STRUCT_SIZE, sizeof(xipc_pkt_channel_request_t));
    return XIPC_ERROR_INTERNAL;
  }

  if (ip == NULL) {
    XIPC_LOG("xipc_pkt_channel_input_port_initialize: ip is NULL\n");
    return XIPC_ERROR_INVALID_ARG;
  }

  ip->_src_pid = src_pid;
  ip->_channel_id = channel_id;
  ip->_head = 0;
  ip->_tail = 0;
  ip->_output_port_free_packets = 0;
  ip->_cached_output_port_tail = 0;
  ip->_output_port_tail = 0;
  xipc_pkt_slot_ptr_t i;
  for (i = 0; i < XIPC_PKT_CHANNEL_NUM_SLOTS+1; i++)
    ip->_ready_packets[i]._pkt_ptr = 0;
  ip->_attr._mask = 0;
  ip->_attr._val._packet_size = 0;
  ip->_attr._val._num_packets = 0;
  ip->_attr._val._buffer = 0;

  XIPC_LOG("Initializing packet channel (src:%s, dest:%s) input port\n",
           xipc_get_proc_name(ip->_src_pid), 
           xipc_get_proc_name(xipc_get_proc_id()));

  return XIPC_OK;
}

/* Initalize the output port of the packet channel
 *
 * op         : output port to be initialized
 * channel_id : channel id that is unique across the subsystem
 * dest_pid   : proc id of the dest proc of the channel
 */
xipc_status_t 
xipc_pkt_channel_output_port_initialize(xipc_pkt_channel_output_port_t *op,
                                        uint16_t channel_id,
                                        xipc_pid_t dest_pid)
{
  if (sizeof(xipc_pkt_channel_output_port_t) != 
      XIPC_PKT_CHANNEL_OUTPUT_PORT_STRUCT_SIZE) {
    XIPC_LOG("xipc_pkt_channel_output_port_initialize:  Internal Error!. Expecting sizeof(xipc_pkt_channel_output_port_t) to be %d, but got size %d", XIPC_PKT_CHANNEL_OUTPUT_PORT_STRUCT_SIZE, sizeof(xipc_pkt_channel_output_port_t));
    return XIPC_ERROR_INTERNAL;
  }

  if (sizeof(xipc_pkt_channel_request_t) != 
      XIPC_PKT_CHANNEL_REQUEST_STRUCT_SIZE) {
    XIPC_LOG("xipc_pkt_channel_output_port_initialize:  Internal Error!. Expecting sizeof(xipc_pkt_channel_request_t) to be %d, but got size %d", XIPC_PKT_CHANNEL_REQUEST_STRUCT_SIZE, sizeof(xipc_pkt_channel_request_t));
    return XIPC_ERROR_INTERNAL;
  }

  if (op == NULL) {
    XIPC_LOG("xipc_pkt_channel_output_port_initialize: op is NULL\n");
    return XIPC_ERROR_INVALID_ARG;
  }

  op->_dest_pid = dest_pid;
  op->_channel_id = channel_id;
  op->_head = 0;
  op->_tail = 0;
  op->_input_port_ready_packets = 0;
  op->_cached_input_port_tail = 0;
  op->_input_port_tail = 0;
  xipc_pkt_slot_ptr_t i;
  for (i = 0; i < XIPC_PKT_CHANNEL_NUM_SLOTS+1; i++)
    op->_free_packets[i]._pkt_ptr = 0;
  op->_attr._mask = 0;
  op->_attr._val._packet_size = 0;
  op->_attr._val._num_packets = 0;
  op->_attr._val._buffer = 0;

  XIPC_LOG("Initializing packet channel (src:%s, dest:%s) output port\n",
           xipc_get_proc_name(xipc_get_proc_id()),
           xipc_get_proc_name(op->_dest_pid));

  return XIPC_OK;
}

/* Allocate a packet from channel. Blocks if the channel is full.  
 *
 * op        : packet channel output port
 * pkt       : pointer to packet that is allocated
 * wait_kind : Sleep or spin wait.
 *
 * Returns void
 */
XIPC_INLINE void
xipc_pkt_channel_allocate(xipc_pkt_channel_output_port_t *op, 
                          xipc_pkt_t *pkt,
                          xipc_wait_kind_t wait_kind)
{
  XIPC_PROFILE_EVENT(XIPC_PROFILE_PKT_CHANNEL_ALLOCATING, op->_channel_id);

  xipc_pkt_slot_ptr_t head = op->_head;
  /* Block while the channel is full */
  while (xipc_pkt_channel_full(op) == 0) {
    if (wait_kind == XIPC_SLEEP_WAIT) {
      uint32_t ps = xipc_disable_interrupts();
      if (xipc_pkt_channel_full(op) == 0) {
        xipc_cond_thread_block(xipc_cond_pkt_channel_full, op);
      }
      xipc_enable_interrupts(ps);
    }
  }
  /* Get the head packet */
  xipc_pkt_slot_t *pkt_slot = &op->_free_packets[head];
  /* Initialize the pkt structure */
  pkt->_pkt_ptr = (uintptr_t)op->_attr._val._buffer + pkt_slot->_pkt_ptr;
  pkt->_size = op->_attr._val._packet_size;
  /* Modulo increment the head */
#if !((XIPC_PKT_CHANNEL_NUM_SLOTS+1) & XIPC_PKT_CHANNEL_NUM_SLOTS)
  op->_head = (head + 1) & XIPC_PKT_CHANNEL_NUM_SLOTS;
#else
  op->_head = xipc_modulo_add(head, 1, XIPC_PKT_CHANNEL_NUM_SLOTS+1);
#endif

  XIPC_PROFILE_EVENT(XIPC_PROFILE_PKT_CHANNEL_ALLOCATED, op->_channel_id);
}

/* Allocate n packets from channel. Blocks if the channel does not have
 * the request number of packets. The allocated packets may not be
 * contiguous in memory, if the packets are not released in the order in which
 * they are acquired.
 *
 * op          : packet channel output port
 * pkt         : pointer to array of packets
 * num_packets : number of packets requested
 * wait_kind   : sleep or spin-wait
 *
 * Returns XIPC_OK if successful, else returns XIPC_ERROR_INVALID_ARG
 */
xipc_status_t
xipc_pkt_channel_allocate_n(xipc_pkt_channel_output_port_t *op, 
                            xipc_pkt_t *pkt,
                            uint32_t num_packets,
                            xipc_wait_kind_t wait_kind)
{
  XIPC_PROFILE_EVENT(XIPC_PROFILE_PKT_CHANNEL_ALLOCATING, op->_channel_id);

  if (num_packets > XIPC_PKT_CHANNEL_NUM_SLOTS) {
#pragma frequency_hint NEVER
    XIPC_LOG("xipc_pkt_channel_allocate_n: num_packets (%d) cannot exceed %d\n",
              num_packets, XIPC_PKT_CHANNEL_NUM_SLOTS);
    return XIPC_ERROR_INVALID_ARG;
  }
  xipc_pkt_slot_ptr_t head = op->_head;
  /* Block while the channel does not have num_packets */
  while (xipc_pkt_channel_output_port_count(op) < num_packets) {
    if (wait_kind == XIPC_SLEEP_WAIT) {
      uint32_t ps = xipc_disable_interrupts();
      if (xipc_pkt_channel_output_port_count(op) < num_packets) {
        xipc_pkt_channel_cond_t s = {._p._op = op, ._n = num_packets};
        xipc_cond_thread_block(xipc_cond_pkt_channel_num_free, &s);
      }
      xipc_enable_interrupts(ps);
    }
  }
  /* Get num_packets starting from the head */
  uintptr_t buffer_addr = (uintptr_t)op->_attr._val._buffer;
  size_t pkt_size = op->_attr._val._packet_size;
  xipc_pkt_slot_t *start_slot = op->_free_packets;
  xipc_pkt_slot_ptr_t k = head;
  int i;
#pragma loop_count min=1
  for (i = 0; i < num_packets; i++) {
    xipc_pkt_slot_t *pkt_slot = start_slot + k;
    /* Initialize the pkt structure */
    pkt[i]._pkt_ptr = buffer_addr + pkt_slot->_pkt_ptr;
    pkt[i]._size = pkt_size;
    /* Modulo increment the head */
#if !((XIPC_PKT_CHANNEL_NUM_SLOTS+1) & XIPC_PKT_CHANNEL_NUM_SLOTS)
    k = (k + 1) & XIPC_PKT_CHANNEL_NUM_SLOTS;
#else
    k = xipc_modulo_add(k, 1, XIPC_PKT_CHANNEL_NUM_SLOTS+1);
#endif
  }
  /* Update the head */
  op->_head = k;

  XIPC_PROFILE_EVENT(XIPC_PROFILE_PKT_CHANNEL_ALLOCATED, op->_channel_id);

  return XIPC_OK;
}

/* Send an allocated packet to the dest proc of the channel
 *
 * op        : output port of the packet channel
 * pkt       : packet to be send
 * wait_kind : sleep or spin wait
 *
 * Returns XIPC_OK if successful, else returns XIPC_ERROR_INVALID_ARG
 */
XIPC_INLINE xipc_status_t
xipc_pkt_channel_send(xipc_pkt_channel_output_port_t *op, 
                      xipc_pkt_t *pkt,
                      xipc_wait_kind_t wait_kind)
{
  if (pkt->_size > op->_attr._val._packet_size) {
#pragma frequency_hint NEVER
    XIPC_LOG("xipc_pkt_channel_send: packet size %d cannot be > channel packet size %d\n", (int)pkt->_size, (int)op->_attr._val._packet_size);
    return XIPC_ERROR_INVALID_ARG;
  }
  /* Update the ready queue of the dest proc with the packet offset and size
   * of the packet being sent */
  xipc_pkt_slot_ptr_t tail = op->_cached_input_port_tail;
  xipc_pkt_slot_ptr_t *ip_tail = op->_input_port_tail;
  xipc_pkt_slot_t *s = op->_input_port_ready_packets + tail;
  s->_pkt_ptr = pkt->_pkt_ptr - (uintptr_t)op->_attr._val._buffer;
  s->_size = pkt->_size;
  s->_num_pkts = 1;
#if !((XIPC_PKT_CHANNEL_NUM_SLOTS+1) & XIPC_PKT_CHANNEL_NUM_SLOTS)
  uint32_t next_tail = (tail + 1) & XIPC_PKT_CHANNEL_NUM_SLOTS;
#else
  uint32_t next_tail = xipc_modulo_add(tail, 1, XIPC_PKT_CHANNEL_NUM_SLOTS+1);
#endif
  /* Update the local copy of the ready queue tail */
  op->_cached_input_port_tail = next_tail;
#pragma no_reorder
  /* Update the dest proc's ready queue tail */
  *ip_tail = next_tail;
  if (wait_kind == XIPC_SLEEP_WAIT) {
#pragma flush_memory
    xipc_proc_notify(op->_dest_pid);
  }

  XIPC_PROFILE_EVENT(XIPC_PROFILE_PKT_CHANNEL_SENT, op->_channel_id);

  return XIPC_OK;
}

/* Send n allocated packets to the dest proc of the channel
 *
 * op          : output port of the packet channel
 * pkt         : pointer to array of packets to be sent
 * num_packets : number of packets to be sent
 * wait_kind   : sleep or spin wait
 *
 * Returns XIPC_OK if successful, else returns XIPC_ERROR_INVALID_ARG
 */
xipc_status_t
xipc_pkt_channel_send_n(xipc_pkt_channel_output_port_t *op, 
                        xipc_pkt_t *pkt,
                        uint32_t num_packets,
                        xipc_wait_kind_t wait_kind)
{
  if (num_packets > XIPC_PKT_CHANNEL_NUM_SLOTS) {
#pragma frequency_hint NEVER
    XIPC_LOG("xipc_pkt_channel_send_n: num_packets (%d) cannot exceed %d\n",
              num_packets, XIPC_PKT_CHANNEL_NUM_SLOTS);
    return XIPC_ERROR_INVALID_ARG;
  }
  /* Update the ready queue of the dest proc with the packet offset and size
   * of the packet being sent */
  xipc_pkt_slot_ptr_t tail = op->_cached_input_port_tail;
  xipc_pkt_slot_ptr_t *ip_tail = op->_input_port_tail;
  uintptr_t buffer_addr = (uintptr_t)op->_attr._val._buffer;
  xipc_pkt_slot_t *start_slot = op->_input_port_ready_packets;
  xipc_pkt_slot_ptr_t k = tail;
  int i;
#pragma loop_count min=1
  for (i = 0; i < num_packets; i++) {
    xipc_pkt_slot_t *s = start_slot + k;
    s->_pkt_ptr = pkt[i]._pkt_ptr - buffer_addr;
    s->_size = pkt[i]._size;
    s->_num_pkts = num_packets-i;
#if !((XIPC_PKT_CHANNEL_NUM_SLOTS+1) & XIPC_PKT_CHANNEL_NUM_SLOTS)
    k = (k + 1) & XIPC_PKT_CHANNEL_NUM_SLOTS;
#else
    k = xipc_modulo_add(k, 1, XIPC_PKT_CHANNEL_NUM_SLOTS+1);
#endif
  }
  /* Update the local copy of the ready queue tail */
  op->_cached_input_port_tail = k;
#pragma no_reorder
  /* Update the dest proc's ready queue tail */
  *ip_tail = k;
  if (wait_kind == XIPC_SLEEP_WAIT) {
#pragma flush_memory
    xipc_proc_notify(op->_dest_pid);
  }

  XIPC_PROFILE_EVENT(XIPC_PROFILE_PKT_CHANNEL_SENT, op->_channel_id);

  return XIPC_OK;
}

/* Receive a packet from the channel. Block if the channel is empty.
 *
 * ip        : input port of the packet channel
 * pkt       : packet that is being received
 * wait_kind : sleep or spin wait
 *
 * Returns void
 */
XIPC_INLINE void 
xipc_pkt_channel_recv(xipc_pkt_channel_input_port_t *ip, 
                      xipc_pkt_t *pkt,
                      xipc_wait_kind_t wait_kind)
{
  XIPC_PROFILE_EVENT(XIPC_PROFILE_PKT_CHANNEL_RECEIVING, ip->_channel_id);

  xipc_pkt_slot_ptr_t head = ip->_head;
  /* Block while the channel is empty */
  while (xipc_pkt_channel_empty(ip) == 0) {
    if (wait_kind == XIPC_SLEEP_WAIT) {
      uint32_t ps = xipc_disable_interrupts();
      if (xipc_pkt_channel_empty(ip) == 0) {
        xipc_cond_thread_block(xipc_cond_pkt_channel_empty, ip);
      }
      xipc_enable_interrupts(ps);
    }
  }
  /* Get the head packet */
  xipc_pkt_slot_t *pkt_slot = &ip->_ready_packets[head];
  /* Initialize the pkt structure */
  pkt->_pkt_ptr = (uintptr_t)ip->_attr._val._buffer + pkt_slot->_pkt_ptr;
  pkt->_size = pkt_slot->_size;
  /* Modulo increment the head */
#if !((XIPC_PKT_CHANNEL_NUM_SLOTS+1) & XIPC_PKT_CHANNEL_NUM_SLOTS)
  ip->_head = (head + 1) & XIPC_PKT_CHANNEL_NUM_SLOTS;
#else
  ip->_head = xipc_modulo_add(head, 1, XIPC_PKT_CHANNEL_NUM_SLOTS+1);
#endif

  XIPC_PROFILE_EVENT(XIPC_PROFILE_PKT_CHANNEL_RECEIVED, ip->_channel_id);
}

/* Receive n packets from the channel. Block if the channel does not have n
 * packets.
 *
 * ip          : input port of the packet channel
 * pkt         : array of packets that are being received
 * num_packets : number of packets
 * wait_kind   : sleep or spin wait
 *
 * Returns XIPC_OK if successful, else returns XIPC_ERROR_INVALID_ARG
 */
xipc_status_t 
xipc_pkt_channel_recv_n(xipc_pkt_channel_input_port_t *ip, 
                        xipc_pkt_t *pkt,
                        uint32_t num_packets,
                        xipc_wait_kind_t wait_kind)
{
  XIPC_PROFILE_EVENT(XIPC_PROFILE_PKT_CHANNEL_RECEIVING, ip->_channel_id);

  if (num_packets > XIPC_PKT_CHANNEL_NUM_SLOTS) {
#pragma frequency_hint NEVER
    XIPC_LOG("xipc_pkt_channel_recv_n: num_packets (%d) cannot exceed %d\n",
              num_packets, XIPC_PKT_CHANNEL_NUM_SLOTS);
    return XIPC_ERROR_INVALID_ARG;
  }
  xipc_pkt_slot_ptr_t head = ip->_head;
  /* Block while the channel is empty */
  while (xipc_pkt_channel_input_port_count(ip) < num_packets) {
    if (wait_kind == XIPC_SLEEP_WAIT) {
      uint32_t ps = xipc_disable_interrupts();
      if (xipc_pkt_channel_input_port_count(ip) < num_packets) {
        xipc_pkt_channel_cond_t s = { ._p._ip = ip, ._n = num_packets };
        xipc_cond_thread_block(xipc_cond_pkt_channel_num_alloc, &s);
      }
      xipc_enable_interrupts(ps);
    }
  }
  xipc_pkt_slot_t *start_slot = ip->_ready_packets;
  uintptr_t buffer_addr = (uintptr_t)ip->_attr._val._buffer;
  xipc_pkt_slot_ptr_t k = head;
  int i;
#pragma loop_count min=1
  for (i = 0; i < num_packets; i++) {
    xipc_pkt_slot_t *pkt_slot = start_slot + k;
    /* Initialize the pkt structure */
    pkt[i]._pkt_ptr = buffer_addr + pkt_slot->_pkt_ptr;
    pkt[i]._size = pkt_slot->_size;
    /* Modulo increment the head */
#if !((XIPC_PKT_CHANNEL_NUM_SLOTS+1) & XIPC_PKT_CHANNEL_NUM_SLOTS)
    k = (k + 1) & XIPC_PKT_CHANNEL_NUM_SLOTS;
#else
    k = xipc_modulo_add(k, 1, XIPC_PKT_CHANNEL_NUM_SLOTS+1);
#endif
  }
  /* Update head */
  ip->_head = k;

  XIPC_PROFILE_EVENT(XIPC_PROFILE_PKT_CHANNEL_RECEIVED, ip->_channel_id);

  return XIPC_OK;
}

/* Release the packet back to the channel
 *
 * ip        : input port of the channel
 * pkt       : packet to be released
 * wait_kind : sleep or spin wait
 *
 * Returns void
 */
XIPC_INLINE void
xipc_pkt_channel_release(xipc_pkt_channel_input_port_t *ip, 
                         xipc_pkt_t *pkt,
                         xipc_wait_kind_t wait_kind)
{
  /* Update the free queue of the src proc with the packet offset of the 
   * packet being released */
  xipc_pkt_slot_ptr_t tail = ip->_cached_output_port_tail;
  xipc_pkt_slot_ptr_t *op_tail = ip->_output_port_tail;
  xipc_pkt_slot_t *s = ip->_output_port_free_packets + tail;
  s->_pkt_ptr = pkt->_pkt_ptr - (uintptr_t)ip->_attr._val._buffer;
#if !((XIPC_PKT_CHANNEL_NUM_SLOTS+1) & XIPC_PKT_CHANNEL_NUM_SLOTS)
  uint32_t next_tail = (tail + 1) & XIPC_PKT_CHANNEL_NUM_SLOTS;
#else
  uint32_t next_tail = xipc_modulo_add(tail, 1, XIPC_PKT_CHANNEL_NUM_SLOTS+1);
#endif
  /* Update the local copy of the free queue tail */
  ip->_cached_output_port_tail = next_tail;
#pragma no_reorder
  /* Update the src proc's free queue tail */
  *op_tail = next_tail;
  if (wait_kind == XIPC_SLEEP_WAIT) {
#pragma flush_memory
    xipc_proc_notify(ip->_src_pid);
  }

  XIPC_PROFILE_EVENT(XIPC_PROFILE_PKT_CHANNEL_RELEASED, ip->_channel_id);
}

/* Release n packets back to the channel
 *
 * ip          : input port of the channel
 * pkt         : array of packets to be released
 * num_packets : number of packets
 * wait_kind   : sleep or spin wait
 *
 * Returns XIPC_OK if successful, else returns XIPC_ERROR_INVALID_ARG
 */
xipc_status_t
xipc_pkt_channel_release_n(xipc_pkt_channel_input_port_t *ip, 
                           xipc_pkt_t *pkt,
                           uint32_t num_packets,
                           xipc_wait_kind_t wait_kind)
{
  if (num_packets > XIPC_PKT_CHANNEL_NUM_SLOTS) {
#pragma frequency_hint NEVER
    XIPC_LOG("xipc_pkt_channel_release_n: num_packets (%d) cannot exceed %d\n",
              num_packets, XIPC_PKT_CHANNEL_NUM_SLOTS);
    return XIPC_ERROR_INVALID_ARG;
  }
  /* Update the free queue of the src proc with the packet offset of the 
   * packet being released */
  xipc_pkt_slot_ptr_t tail = ip->_cached_output_port_tail;
  xipc_pkt_slot_t *start_slot = ip->_output_port_free_packets;
  xipc_pkt_slot_ptr_t *op_tail = ip->_output_port_tail;
  uintptr_t buffer_addr = (uintptr_t)ip->_attr._val._buffer;
  xipc_pkt_slot_ptr_t k = tail;
  int i;
#pragma loop_count min=1
  for (i = 0; i < num_packets; i++) {
    xipc_pkt_slot_t *s = start_slot + k;
    s->_pkt_ptr = pkt[i]._pkt_ptr - buffer_addr;
#if !((XIPC_PKT_CHANNEL_NUM_SLOTS+1) & XIPC_PKT_CHANNEL_NUM_SLOTS)
    k = (k + 1) & XIPC_PKT_CHANNEL_NUM_SLOTS;
#else
    k = xipc_modulo_add(k, 1, XIPC_PKT_CHANNEL_NUM_SLOTS+1);
#endif
  }
  /* Update the local copy of the free queue tail */
  ip->_cached_output_port_tail = k;
#pragma no_reorder
  /* Update the src proc's free queue tail */
  *op_tail = k;
  if (wait_kind == XIPC_SLEEP_WAIT) {
#pragma flush_memory
    xipc_proc_notify(ip->_src_pid);
  }

  XIPC_PROFILE_EVENT(XIPC_PROFILE_PKT_CHANNEL_RELEASED, ip->_channel_id);

  return XIPC_OK;
}

/* Find how many packets were queued. Optionally block until there is something
 * queued. If block is true, the consumer sleep waits.
 * Note, if block is true, the sender needs to have its wait_kind
 * parameter in its send routines set to XIPC_SLEEP_WAIT to allow for the
 * the producer to notify the consumer using the XIPC-interrupt.
 *
 * ip    : input port of the packet channel
 * block : block waiting until channel is not empty
 *
 * Returns number of queued packets. Returns 0 if the channel is empty and
 * block is not true.
 */
uint8_t 
xipc_pkt_channel_num_sent(xipc_pkt_channel_input_port_t *ip, uint8_t block)
{
  xipc_pkt_slot_ptr_t head = ip->_head;
  if (block) {
    /* Block while the channel is empty */
    while (xipc_pkt_channel_empty(ip) == 0) {
      uint32_t ps = xipc_disable_interrupts();
      if (xipc_pkt_channel_empty(ip) == 0) {
        xipc_cond_thread_block(xipc_cond_pkt_channel_empty, ip);
      }
      xipc_enable_interrupts(ps);
    }
  } else {
    /* Channel is empty */
    if (xipc_pkt_channel_empty(ip) == 0)
      return 0;
  }
  xipc_pkt_slot_t *s = &ip->_ready_packets[head];
  return s->_num_pkts;
}

/* Sleep or spin wait until num packets are received.
 *
 * ip          : input port of the packet channel
 * num_packets : number of packets
 * wait_kind   : sleep or spin wait
 *
 * Returns XIPC_OK if successful, else returns XIPC_ERROR_INVALID_ARG.
 */
xipc_status_t 
xipc_pkt_channel_wait_num_recv(xipc_pkt_channel_input_port_t *ip, 
                                uint32_t num_packets,
                                xipc_wait_kind_t wait_kind)
{
  if (num_packets > XIPC_PKT_CHANNEL_NUM_SLOTS) {
#pragma frequency_hint NEVER
    XIPC_LOG("xipc_pkt_channel_wait_num_recv: num_packets (%d) cannot exceed %d\n", num_packets, XIPC_PKT_CHANNEL_NUM_SLOTS);
    return XIPC_ERROR_INVALID_ARG;
  }

  while (xipc_pkt_channel_input_port_count(ip) < num_packets) {
    if (wait_kind == XIPC_SLEEP_WAIT) {
      uint32_t ps = xipc_disable_interrupts();
      if (xipc_pkt_channel_input_port_count(ip) < num_packets) {
        xipc_pkt_channel_cond_t s = { ._p._ip = ip, ._n = num_packets };
        xipc_cond_thread_block(xipc_cond_pkt_channel_num_alloc, &s);
      }
      xipc_enable_interrupts(ps);
    }
  }

  return XIPC_OK;
}
 
/* Call back used to queue the packet to the channel after the asynchronous
 * copy is done */
#if XCHAL_HAVE_IDMA
static void xipc_pkt_channel_send_buf_callback(void *arg)
{
#pragma flush_memory
  xipc_pkt_channel_request_t *req = (xipc_pkt_channel_request_t *)arg;
  if (idma_task_status(req->_idma_task) != IDMA_TASK_DONE)
    return;
  xipc_pkt_channel_send(req->_port._op, &req->_pkt, req->_wait_kind);
}
#else
static void xipc_pkt_channel_send_buf_callback(void *arg)
{
  xipc_pkt_channel_request_t *req = (xipc_pkt_channel_request_t *)arg;
  xipc_pkt_channel_send(req->_port._op, &req->_pkt, req->_wait_kind);
}
#endif

/* Send a user buffer to the channel. Its size is expected to be <= to 
 * the packet size of the channel. The copy of the user buffer to the channel
 * can synchronous (if the req parameter is NULL) or asynchronous (using uDMA).
 *
 * op        : output port of the packet channel
 * p         : pointer to the user buffer
 * size      : size of the buffer to send. Has to be <= the channel packet size
 * req       : request structure used for asynchronous copy. The function 
 *             returns after the copy is scheduled but before the copy 
 *             completes. The req structure can be used to query for the 
 *             status of the copy. If NULL, the function returns after the copy.
 * wait_kind : sleep or spin wait
 *
 * Returns XIPC_OK if successful else returns XIPC_ERROR_ASYNC_COPY
 */
xipc_status_t
xipc_pkt_channel_send_buf(xipc_pkt_channel_output_port_t *op,
                          void *p,
                          size_t size,
                          xipc_pkt_channel_request_t *req,
                          xipc_wait_kind_t wait_kind)
{
  xipc_pkt_t pkt;

  if (size > op->_attr._val._packet_size) {
    XIPC_LOG("xipc_pkt_channel_send_buf: size of buf %d cannot exceed channel packet size %d\n", (int)size, (int)op->_attr._val._packet_size);
    return XIPC_ERROR_INVALID_ARG;
  }
  xipc_pkt_channel_allocate(op, &pkt, wait_kind);
  pkt._size = size;

  XIPC_PROFILE_EVENT(XIPC_PROFILE_PKT_CHANNEL_COPYING, op->_channel_id);

  /* If req is null, do a regular copy, else do an async copy using the uDMA */
  if (req == 0) {
    memcpy((void *)pkt._pkt_ptr, p, size);
#if XCHAL_DCACHE_SIZE > 0 && !XCHAL_DCACHE_IS_COHERENT
    xthal_dcache_region_writeback((void *)pkt._pkt_ptr, pkt._size);
#endif
#pragma flush_memory
    xipc_pkt_channel_send(op, &pkt, wait_kind);
    return XIPC_OK;
  } else {
    req->_port._op = op;
    req->_pkt = pkt;
    req->_wait_kind = wait_kind;
    /* Schedule a copy and return right away. When the copy finishes, 
     * the packet gets queued in the channel using xipc_pkt_channel_send */
    return
      xipc_pkt_channel_async_copy((void *)pkt._pkt_ptr,
                                  p, 
                                  size,
                                  xipc_pkt_channel_send_buf_callback,
                                  req,
                                  1);
  }
}

/* Call back used to release the packet to the channel after the asynchronous
 * copy is done */
#if XCHAL_HAVE_IDMA
static void xipc_pkt_channel_recv_buf_callback(void *arg)
{
#pragma flush_memory
  xipc_pkt_channel_request_t *req = (xipc_pkt_channel_request_t *)arg;
  if (idma_task_status(req->_idma_task) != IDMA_TASK_DONE)
    return;
  xipc_pkt_channel_release(req->_port._ip, &req->_pkt, req->_wait_kind);
}
#else
static void xipc_pkt_channel_recv_buf_callback(void *arg)
{
  xipc_pkt_channel_request_t *req = (xipc_pkt_channel_request_t *)arg;
  xipc_pkt_channel_release(req->_port._ip, &req->_pkt, req->_wait_kind);
}
#endif

/* Receive from the channel to the user buffer. Its size is expected to be 
 * equal to the packet size of the channel. The copy of the user buffer from the
 * channel can synchronous (if the req parameter is NULL) or 
 * asynchronous (using uDMA).
 *
 * ip        : input port of the packet channel
 * p         : pointer to the user buffer
 * size      : returns the size of the received buffer. Will be <= the channel
 *             packet size
 * req       : request structure used for asynchronous copy. The function 
 *             returns after the copy is scheduled but before the copy 
 *             completes. The req structure can be used to query for the 
 *             status of the copy. If NULL, the function returns after the copy.
 * wait_kind : sleep or spin wait
 *
 * Returns XIPC_OK if successful else returns XIPC_ERROR_ASYNC_COPY
 */
xipc_status_t
xipc_pkt_channel_recv_buf(xipc_pkt_channel_input_port_t *ip,
                          void *p,
                          size_t *size,
                          xipc_pkt_channel_request_t *req,
                          xipc_wait_kind_t wait_kind)
{
  xipc_pkt_t pkt;
  xipc_pkt_channel_recv(ip, &pkt, wait_kind);
  *size = pkt._size;

  XIPC_PROFILE_EVENT(XIPC_PROFILE_PKT_CHANNEL_COPYING, ip->_channel_id);

  /* If req is null, do a regular copy, else do an async copy using the uDMA */
  if (req == 0) {
#if XCHAL_DCACHE_SIZE > 0 && !XCHAL_DCACHE_IS_COHERENT
    xthal_dcache_region_invalidate((void *)pkt._pkt_ptr, pkt._size);
#endif
    memcpy(p, (void *)pkt._pkt_ptr, pkt._size);
#pragma flush_memory
    xipc_pkt_channel_release(ip, &pkt, wait_kind);
    return XIPC_OK;
  } else {
    req->_port._ip = ip;
    req->_pkt = pkt;
    req->_wait_kind = wait_kind;
    /* Schedule a copy and return right away. When the copy finishes, the packet
     * is released to the channel using xipc_pkt_channel_release */
    return 
      xipc_pkt_channel_async_copy(p,
                                  (void *)pkt._pkt_ptr,
                                  pkt._size,
                                  xipc_pkt_channel_recv_buf_callback,
                                  req,
                                  1);
  }
}

/* Set packet channel attribute at the input port
 *
 * ip   : input port
 * attr : attribute type
 * val  : 32-bit value of the attribute
 *
 * Returns XIPC_OK if successful, 
 *                 else returns XIPC_ERROR_PKT_CHANNEL_UNKNOWN_ATTR,
 *                              XIPC_ERROR_INVALID_ARG
 */
xipc_status_t
xipc_pkt_channel_input_port_set_attr(xipc_pkt_channel_input_port_t *ip,
                                     xipc_pkt_channel_attr_type_t attr,
                                     uint32_t val)
{
  ip->_attr._mask |= attr;
  switch (attr) {
    case XIPC_PKT_CHANNEL_BUFFER_ADDR_ATTR:
      ip->_attr._val._buffer = (void *)val;
      break;
    case XIPC_PKT_CHANNEL_NUM_PACKETS_ATTR:
      if (val > XIPC_PKT_CHANNEL_NUM_SLOTS) {
        XIPC_LOG("xipc_pkt_channel_input_port_set_attr: num_packets attr should be <= %d\n", XIPC_PKT_CHANNEL_NUM_SLOTS);
        return XIPC_ERROR_INVALID_ARG;
      }
      ip->_attr._val._num_packets = val;
      break;
    case XIPC_PKT_CHANNEL_PACKET_SIZE_ATTR:
      ip->_attr._val._packet_size = val;
      break;
    default:
      XIPC_LOG("xipc_pkt_channel_input_port_set_attr: unknown attribute %d attr\n", attr);
      return XIPC_ERROR_PKT_CHANNEL_UNKNOWN_ATTR;
  }
  return XIPC_OK;
}

/* Set packet channel attribute at the output port
 *
 * op   : output port
 * attr : attribute type
 * val  : 32-bit value of the attribute
 *
 * Returns XIPC_OK if successful, 
 *                 else returns XIPC_ERROR_PKT_CHANNEL_UNKNOWN_ATTR,
 *                              XIPC_ERROR_INVALID_ARG
 */
xipc_status_t
xipc_pkt_channel_output_port_set_attr(xipc_pkt_channel_output_port_t *op,
                                      xipc_pkt_channel_attr_type_t attr,
                                      uint32_t val)
{
  op->_attr._mask |= attr;
  switch (attr) {
    case XIPC_PKT_CHANNEL_BUFFER_ADDR_ATTR:
      op->_attr._val._buffer = (void *)val;
      break;
    case XIPC_PKT_CHANNEL_NUM_PACKETS_ATTR:
      if (val > XIPC_PKT_CHANNEL_NUM_SLOTS) {
        XIPC_LOG("xipc_pkt_channel_input_port_set_attr: num_packets attr should be <= %d\n", XIPC_PKT_CHANNEL_NUM_SLOTS);
        return XIPC_ERROR_INVALID_ARG;
      }
      op->_attr._val._num_packets = val;
      break;
    case XIPC_PKT_CHANNEL_PACKET_SIZE_ATTR:
      op->_attr._val._packet_size = val;
      break;
    default:
      XIPC_LOG("xipc_pkt_channel_output_port_set_attr: unknown attribute %d attr\n", attr);
      return XIPC_ERROR_PKT_CHANNEL_UNKNOWN_ATTR;
  }
  return XIPC_OK;
}

/* Check the status of the asynchronous send/recv request
 *
 * req : request structure
 *
 * Returns the request status 
 */
#if XCHAL_HAVE_IDMA
xipc_pkt_channel_request_type_t 
xipc_pkt_channel_check_request(xipc_pkt_channel_request_t *req)
{
  int s = idma_task_status(req->_idma_task);
  if (s < 0)
    return XIPC_PKT_CHANNEL_REQ_ERROR;
  return s == 0 ? XIPC_PKT_CHANNEL_REQ_DONE : XIPC_PKT_CHANNEL_REQ_PENDING;
}
#else
xipc_pkt_channel_request_type_t 
xipc_pkt_channel_check_request(xipc_pkt_channel_request_t *req)
{
  return XIPC_PKT_CHANNEL_REQ_DONE;
}
#endif

/* Block while the request is pending
 *
 * req        : request structure 
 * wait_kind  : sleep or spin wait
 *
 * Returns XIPC_OK, if successful, else returns XIPC_ERROR_ASYNC_COPY
 */
#if XCHAL_HAVE_IDMA
xipc_status_t 
xipc_pkt_channel_wait_request(xipc_pkt_channel_request_t *req,
                              xipc_wait_kind_t wait_kind)
{
  int s;
  while ((s = idma_task_status(req->_idma_task)) > 0) {
    if (wait_kind == XIPC_SLEEP_WAIT)
      idma_sleep();
  }

  if (s != 0) {
    return XIPC_ERROR_ASYNC_COPY;
  }

  return XIPC_OK;
}
#else
xipc_status_t 
xipc_pkt_channel_wait_request(xipc_pkt_channel_request_t *req,
                              xipc_wait_kind_t sleep_wait)
{
  return XIPC_OK;
}
#endif
