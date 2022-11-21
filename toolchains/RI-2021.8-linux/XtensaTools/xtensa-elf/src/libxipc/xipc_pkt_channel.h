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
#ifndef __XIPC_PKT_CHANNEL_H__
#define __XIPC_PKT_CHANNEL_H__

/*! @file */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "xipc_common.h"
#include "xipc_pkt.h"

/*! Size of packet channel request object in bytes that is used to schedule 
 *  asynchronous packet transfers using the DMA hardware, if available. */
#if XCHAL_HAVE_IDMA
#define XIPC_PKT_CHANNEL_REQUEST_STRUCT_SIZE     (112)
#else
#define XIPC_PKT_CHANNEL_REQUEST_STRUCT_SIZE     (16)
#endif

/*! Size of packet channel input port object in bytes. */
#define XIPC_PKT_CHANNEL_INPUT_PORT_STRUCT_SIZE  164

/*! Size of packet channel output port object in bytes. */
#define XIPC_PKT_CHANNEL_OUTPUT_PORT_STRUCT_SIZE 164

/*! Enum defining different attribute types settable on the input or the 
 *  output port of the packet channel. */
typedef enum {
  /*! The address of the packet buffer. This needs to be set on either the
   *  producer or consumer core, but not on both. */
  XIPC_PKT_CHANNEL_BUFFER_ADDR_ATTR = 1,
  /*! Number of packets in the packet buffer. This needs to be set on either 
   *  the producer or consumer end, but not on both. The number of packets 
   *  cannot exceed the maximum size of 15. */
  XIPC_PKT_CHANNEL_NUM_PACKETS_ATTR = 2,
  /*! The size of the packet in the packet buffer. This needs to be set on 
   *  either the producer or consumer end, but not on both. The maximum 
   * allowed packet size is 64Kbytes. */
  XIPC_PKT_CHANNEL_PACKET_SIZE_ATTR = 4
} xipc_pkt_channel_attr_type_t;

/*! Enum defining the status of the asynchronous copy request to copy user 
 *  data to/from the channel queue. */
typedef enum {
  /*! The transfer of the user data is in progress. */
  XIPC_PKT_CHANNEL_REQ_PENDING = 0,
  /*! The transfer of the user data is complete. */
  XIPC_PKT_CHANNEL_REQ_DONE    = 1,
  /*! An error was encountered when transferring the data. */
  XIPC_PKT_CHANNEL_REQ_ERROR   = 2
} xipc_pkt_channel_request_type_t;

#ifndef __XIPC_PKT_CHANNEL_INTERNAL_H__
struct xipc_pkt_channel_input_port_struct {
  char _[XIPC_PKT_CHANNEL_INPUT_PORT_STRUCT_SIZE];
};

struct xipc_pkt_channel_output_port_struct {
  char _[XIPC_PKT_CHANNEL_OUTPUT_PORT_STRUCT_SIZE];
};

struct xipc_pkt_channel_request_struct {
  char _[XIPC_PKT_CHANNEL_REQUEST_STRUCT_SIZE];
};
#endif

/*! Consumer of the packet channel uses a handle of this type to
 *  receive packets. Expected to be in local memory */
typedef struct xipc_pkt_channel_input_port_struct xipc_pkt_channel_input_port_t;

/*! Producer of the packet channel uses a handle of this type to
 *  send packets. Expected to be in local memory */
typedef struct xipc_pkt_channel_output_port_struct xipc_pkt_channel_output_port_t;

/*! Used to schedule asynchronous packet transfer operations using the \a
 *  send/recv_buf API functions */
typedef struct xipc_pkt_channel_request_struct xipc_pkt_channel_request_t;

/*!
 * Checks using the output port handle (on the producer core) if the packet 
 * channel is full.
 *
 * \param op Pointer to the packet channel output port.
 * \return   0 if full, else returns a non-zero value.
 */
extern int32_t xipc_pkt_channel_full(xipc_pkt_channel_output_port_t *op);

/*! 
 * Checks using the input port handle (on the consumer core) if the packet 
 * channel is empty.
 *
 * \param ip Pointer to the packet channel input port.
 * \return   0 if empty, else returns a non-zero value.
 */
extern int32_t xipc_pkt_channel_empty(xipc_pkt_channel_input_port_t *ip);

/*! 
 * Connect the consumer core to the packet channel. This must be called after
 * the port is initialized using \ref xipc_pkt_channel_input_port_initialize 
 * and after the channel attributes are set using 
 * \ref xipc_pkt_channel_input_port_set_attr. 
 * This function uses the pre-defined message channel between the 
 * two cores to exchange 
 * the channel ID, channel attributes, port information, address of the queues,
 * etc. It is required that a corresponding call to 
 * \ref xipc_pkt_channel_output_port_connect to the same channel 
 * ID be called on the producer core.
 *
 * \param ip Pointer to packet channel input port.
 * \return   XIPC_OK, if successful else returns one of
 *           - XIPC_ERROR_CHANNEL_CONNECT,
 *           - XIPC_ERROR_PKT_CHANNEL_DUPL_ATTR,
 *           - XIPC_ERROR_PKT_CHANNEL_DUPL_ATTR,
 *           - XIPC_ERROR_PKT_CHANNEL_DUPL_ATTR,
 *           - XIPC_ERROR_INTERNAL
 *
 *           See ::xipc_status_t for a description of the above error codes.
 */
extern xipc_status_t
xipc_pkt_channel_input_port_connect(xipc_pkt_channel_input_port_t *ip);

/*! 
 * Connect the producer core to the packet channel. This must be called after 
 * the port is initialized using \ref xipc_pkt_channel_output_port_initialize 
 * and after the channel attributes are set using 
 * \ref xipc_pkt_channel_output_port_set_attr.
 * This function uses the pre-defined message channel 
 * between the two cores to exchange the
 * channel ID, channel attributes, port information, address of the queues, etc.
 * It is required that a corresponding call to 
 * \ref xipc_pkt_channel_input_port_connect to the same channel ID be called on 
 * the consumer core.
 *
 * \param op Pointer to packet channel output port.
 * \return   XIPC_OK, if successful else returns one of
 *           - XIPC_ERROR_CHANNEL_CONNECT,
 *           - XIPC_ERROR_PKT_CHANNEL_DUPL_ATTR,
 *           - XIPC_ERROR_PKT_CHANNEL_DUPL_ATTR,
 *           - XIPC_ERROR_PKT_CHANNEL_DUPL_ATTR,
 *           - XIPC_ERROR_INTERNAL
 *
 *           See \ref xipc_status_t for a description of the above error codes.
 */
extern xipc_status_t
xipc_pkt_channel_output_port_connect(xipc_pkt_channel_output_port_t *op);

/*!
 * Initialize the input port of the channel on the consumer processor end. 
 * The processor ID of the corresponding producer core to which the channel 
 * connection is established needs to be specified. A channel ID unique across
 * all such packet channels in the subsystem is used to identify the channel 
 * and is verified when establishing the connection. The channel ID needs to 
 * match the ID specified when initializing the corresponding output port.
 *
 * \param ip         Pointer to input port object.
 * \param channel_id Unique ID for the channel.
 * \param src_pid    Processor ID of the producer of the channel.
 * \return           XIPC_OK if successful, else returns one of
 *                   - XIPC_ERROR_INTERNAL 
 *                   - XIPC_ERROR_INVALID_ARG
 */ 
extern xipc_status_t
xipc_pkt_channel_input_port_initialize(xipc_pkt_channel_input_port_t *ip,
                                       uint16_t channel_id,
                                       xipc_pid_t src_pid);

/*! 
 * Initialize the output port of the channel on the producer processor end. 
 * The processor ID of the corresponding consumer core to which the channel 
 * connection is established needs to be specified. A channel ID unique 
 * across all such packet channels in the subsystem is used to identify the 
 * channel and is verified when establishing the connection. The channel ID 
 * needs to match the ID specified when initializing the corresponding input 
 * port.
 *
 * \param op         Pointer to output port object.
 * \param channel_id Unique ID of the channel.
 * \param dest_pid   Processor ID of the consumer of the channel.
 * \return           XIPC_OK if successful, else returns one of
 *                   - XIPC_ERROR_INTERNAL 
 *                   - XIPC_ERROR_INVALID_ARG
 */
extern xipc_status_t
xipc_pkt_channel_output_port_initialize(xipc_pkt_channel_output_port_t *op,
                                        uint16_t channel_id,
                                        xipc_pid_t dest_pid);

/*!
 * Allocate a packet on the producer core using the output port handle of the 
 * channel. This call can block if the channel is full. 
 * The caller can either \a spin-wait or \a sleep-wait. If 
 * sleep-waiting the consumer of the channel notifies the 
 * producer using the \a XIPC-interrupt whenever it
 * releases a packet. Note, the consumer needs to do a corresponding 
 * \a XIPC_SLEEP_WAIT to allow it to notify the producer. 
 *
 * \param op        Pointer to packet channel output port
 * \param pkt       Pointer to packet object that gets initialized with the 
 *                  address of the allocated packet in the queue.
 * \param wait_kind Defines the kind of wait a core performs when waiting if 
 *                  channel is full. The 
 *                  core can \a spin-wait or do a \a sleep-wait. If set to 
 *                  sleep-waiting, the consumer unblocks the producer using 
 *                  the \a XIPC-interrupt.
 */
extern void
xipc_pkt_channel_allocate(xipc_pkt_channel_output_port_t *op, 
                          xipc_pkt_t *pkt,
                          xipc_wait_kind_t wait_kind);

/*! 
 * Queue up an allocated packet to the channel. If the \a wait_kind 
 * parameter is \a XIPC_SLEEP_WAIT, the consumer gets notified 
 * through the \a XIPC-interrupt. If \a wait_kind is set to \a XIPC_SPIN_WAIT,
 * the consumer core is not notified and is expected that the consumer core is 
 * spin-waiting.
 *
 * \param op        Pointer to output port of the packet channel.
 * \param pkt       Pointer to packet to be sent.
 * \param wait_kind If set to \a XIPC_SPIN_WAIT, no interrupt is sent to the 
 *                  destination core, else notifies the destination core using
 *                  the \a XIPC-interrupt.
 * \return    XIPC_OK if successful, else returns XIPC_ERROR_INVALID_ARG.
 */ 
extern xipc_status_t
xipc_pkt_channel_send(xipc_pkt_channel_output_port_t *op, 
                      xipc_pkt_t *pkt,
                      xipc_wait_kind_t wait_kind);

/*!
 * Send a user-provided buffer to the consumer processor. The size of the 
 * buffer has to be less than or equal to the maximum packet size of the 
 * channel. The function blocks if the channel is full. If the \a req parameter
 * is not NULL, the function performs an asynchronous copy using the DMA 
 * hardware. The function returns right away after scheduling the copy. 
 * The status of the copy can be queried using 
 * \ref xipc_pkt_channel_check_request
 * or the \ref xipc_pkt_channel_wait_request APIs. At the end of the copy, the
 * consumer core is notified using the \a XIPC-interrupt, which can unblock the
 * consumer if it is waiting for data. If the \a req parameter is NULL (or if
 * the DMA is not configured), \a memcpy is used to copy the data from the 
 * user buffer to the queue. If the buffer happens to be in shared cacheable 
 * memory, the relevant cache lines are flushed after the \a memcpy.
 * The \a wait_kind paramter determines whether to spin-wait or sleep-wait
 * if the channel is full and also notify the consumer if the consumer is
 * blocked waiting for data. Note, the consumer needs to have a matching \a
 * wait_kind.
 *
 * \param op        Pointer to output port of the packet channel.
 * \param p         Pointer to buffer to be sent.
 * \param size      Size of the buffer in bytes to be sent. 
 *                  Cannot exceed maximum packet size.
 * \param req       Request structure used if performing asynchronous copy 
 *                  using the DMA hardware, if present. If DMA is not 
 *                  configured of req is NULL, \a memcpy is used to do a 
 *                  synchronous copy. The request object needs to be in 
 *                  local memory.
 * \param wait_kind If set to \a XIPC_SLEEP_WAIT, the producer sleep waits
 *                  if the channel is full and notifies the consumer 
 *                  using the \a XIPC-interrupt.
 * \return          XIPC_OK if successful, else returns one of
 *                    - XIPC_ERROR_ASYNC_COPY if there was an error in copy
 *                    - XIPC_ERROR_INVALID_ARG  if the size exceeds the maximum 
 *                      packet size.
 */
xipc_status_t
xipc_pkt_channel_send_buf(xipc_pkt_channel_output_port_t *op,
                          void *p,
                          size_t size,
                          xipc_pkt_channel_request_t *req,
                          xipc_wait_kind_t wait_kind);

/*!
 * Receive a packet on the consumer core using the input port handle of the 
 * channel. This call blocks if the channel is empty. 
 * The caller can either \a spin-wait or 
 * \a sleep-wait. If sleep-waiting, the producer of the channel has to 
 * have its \a wait_kind parameter in its call set to xipc_pkt_channel_send 
 * routines set to \a XIPC_SLEEP_WAIT to allow it to 
 * notify the consumer using the \a XIPC-interrupt whenever it 
 * generates a packet.
 *
 * \param ip        Pointer to input port of the packet channel.
 * \param pkt       Pointer to packet object that gets initialized with 
 *                  the address of the received packet in the queue.
 * \param wait_kind Defines the kind of wait a core performs when blocking if 
 *                  channel is empty. The core can \a spin-wait or do a 
 *                  \a sleep-wait. If set to \a sleep-waiting, the producer 
 *                  notifies the consumer using the \a XIPC-interrupt at the 
 *                  end of the call.
 */
extern void
xipc_pkt_channel_recv(xipc_pkt_channel_input_port_t *ip, 
                      xipc_pkt_t *pkt,
                      xipc_wait_kind_t wait_kind);

/*!
 * Receive a packet from the queue to a user provided buffer. The size of the 
 * buffer has to be less than or equal to the maximum packet size of the 
 * channel. The function blocks if the channel is empty. If the \a req 
 * parameter is not NULL, the function performs an asynchronous copy using the 
 * DMA hardware. The function returns right away after scheduling the copy. 
 * The status of the copy can be queried 
 * using \ref xipc_pkt_channel_check_request
 * or the \ref xipc_pkt_channel_wait_request APIs. At the end of the copy, the 
 * producer core is notified using the \a XIPC-interrupt, which can unblock the
 * producer if it is waiting for the channel to free up. If the \a req 
 * parameter is NULL (or if the DMA is not configured), \a memcpy is used to 
 * copy the data from the queue to the user buffer. If the buffer happens to 
 * be in shared cacheable memory, the relevant cache lines are invalidated 
 * prior to the \a memcpy.
 * The \a wait_kind paramter determines whether to spin-wait or sleep-wait
 * if the channel is empty and also notify the producer if the producer is
 * blocked waiting for channel to be not full. Note, the producer needs to 
 * have a matching \a wait_kind.
 *
 * \param ip        Pointer to input port of the packet channel
 * \param p         Pointer to the user buffer to receive packet from channel.
 * \param size      Size of the buffer in bytes. Cannot exceed maximum 
 *                  packet size.
 * \param req       Request structure used if performing asynchronous copy 
 *                  using the DMA hardware, if present. If DMA is not 
 *                  configured of req is 
 *                  NULL, \a memcpy is used to do a synchronous copy.
 * \param wait_kind If set to \a XIPC_SLEEP_WAIT, the consumer sleep waits
 *                  if the channel is empty and notifies the producer 
 *                  using the \a XIPC-interrupt.
 * \return          XIPC_OK if successful, else returns one of
 *                    - XIPC_ERROR_ASYNC_COPY if there was an error in copy
 *                    - XIPC_ERROR_INVALID_ARG  if the size exceeds the maximum 
 *                      packet size.
 */
xipc_status_t
xipc_pkt_channel_recv_buf(xipc_pkt_channel_input_port_t *ip,
                          void *p,
                          size_t *size,
                          xipc_pkt_channel_request_t *req,
                          xipc_wait_kind_t wait_kind);

/*!
 * Release the packet back to the channel
 * If the \a wait_kind parameters is \a XIPC_SLEEP_WAIT, the producer 
 * gets notified through the \a XIPC-interrupt. If \a wait_kind is set to 
 * \a XIPC_SPIN_WAIT, the producer core is not notified and is expected that 
 * the producer core is spin-waiting.
 *
 * \param ip        Pointer to input port of the channel.
 * \param pkt       Pointer to packet object that is to be returned 
 *                  to the queue.
 * \param wait_kind If set to \a XIPC_SPIN_WAIT, no interrupt is sent to 
 *                  the producer core, else notifies the producer core using
 *                  the \a XIPC-interrupt.
 */
extern void
xipc_pkt_channel_release(xipc_pkt_channel_input_port_t *ip, 
                         xipc_pkt_t *pkt,
                         xipc_wait_kind_t wait_kind);

/*!
 * Sets the packet channel attribute on the input port. The attributes include 
 * the address of the queue, the number of packets, and size per packet. See 
 * \ref xipc_pkt_channel_attr_type_t for the valid values of the 
 * attribute. The attribute can be set only at either the input port or the 
 * output port end, but not both.
 *  
 * \param ip   Pointer to input port of the packet channel.
 * \param attr Attribute to be set on the input port.
 * \param val  Value of the attribute.
 * \return     XIPC_OK if successful, else returns one of
 *             - XIPC_ERROR_INVALID_ARG if the number of packets exceed the 
 *               allowed maximum.
 *             - XIPC_ERROR_PKT_CHANNEL_UNKNOWN_ATTR if the attribute is 
 *               undefined.
 */ 
extern xipc_status_t
xipc_pkt_channel_input_port_set_attr(xipc_pkt_channel_input_port_t *ip,
                                     xipc_pkt_channel_attr_type_t attr,
                                     uint32_t val);

/*! 
 * Sets the packet channel attribute at the output port. The attributes 
 * include the address of the queue, the number of packets, and size per 
 * packet. See \ref xipc_pkt_channel_attr_type_t for the valid values 
 * of the attribute. The attribute can be set only at either the input port or 
 * the output port end, but not both.
 *
 * \param op   Pointer to output port of the packet channel.
 * \param attr Attribute to be set on the output port.
 * \param val  Value of the attribute.
 * \return     XIPC_OK if successful else returns one of
 *             - XIPC_ERROR_INVALID_ARG if the number of packets exceed the 
 *               allowed maximum.
 *             - XIPC_ERROR_PKT_CHANNEL_UNKNOWN_ATTR if the attribute is 
 *               undefined.
 */ 
extern xipc_status_t
xipc_pkt_channel_output_port_set_attr(xipc_pkt_channel_output_port_t *op,
                                      xipc_pkt_channel_attr_type_t attr,
                                      uint32_t val);

/*! 
 * Checks the status of the asynchronous copy request of the 
 * \ref xipc_pkt_channel_send_buf and \ref xipc_pkt_channel_recv_buf functions.
 *
 * \param req Request structure that was passed to the \a send/recv_buf 
 *            functions for performing asynchronous copy.
 * \return    Status of the request.
 */
extern xipc_pkt_channel_request_type_t 
xipc_pkt_channel_check_request(xipc_pkt_channel_request_t *req);

/*! 
 * The \ref xipc_pkt_channel_send_buf and \ref xipc_pkt_channel_recv_buf 
 * functions can be 
 * used to perform asynchronous copy using the hardware DMA of user data 
 * to/from the queue. This API allows to wait for the copy requests to 
 * complete. Based on the \a sleep_wait parameter, the caller can either 
 * perform a \a spin-wait or do \a sleep-wait.
 *
 * \param req       Request structure that was passed to the \a send/recv_buf 
 *                  functions for performing the asynchronous copy.
 * \param wait_kind Spin-wait or sleep-wait.
 * \return          XIPC_OK, if successful, else returns 
 *                  XIPC_ERROR_ASYNC_COPY if there was a DMA copy error.
 */
extern xipc_status_t
xipc_pkt_channel_wait_request(xipc_pkt_channel_request_t *req,
                              xipc_wait_kind_t wait_kind);

/*!
 * Returns the number of free packets in the channel.
 *
 * \param op Pointer to the packet channel output port.
 * \return   Free packet count.
 */ 
extern uint32_t
xipc_pkt_channel_output_port_count(xipc_pkt_channel_output_port_t *op);

/*!
 *  Returns the number of allocated packets in the channel.
 *
 * \param ip Pointer to the packet channel input port.
 * \return   Allocated packet count.
 */
extern uint32_t
xipc_pkt_channel_input_port_count(xipc_pkt_channel_input_port_t *ip);

/*!
 * Allocate \a num_packets packets on the producer core using the output port 
 * handle of the channel. This call can block if the channel has less than 
 * \a num_packets free. The caller can either \a spin-wait or \a sleep-wait. 
 * If sleep-waiting the consumer of the channel notifies the 
 * producer using the \a XIPC-interrupt whenever it
 * releases a packet. Note, the consumer needs to do a corresponding 
 * \a XIPC_SLEEP_WAIT to allow it to notify the producer. 
 * The allocated packets need not be contiguous in memory.
 *
 * \param op          Pointer to packet channel output port.
 * \param pkt         Pointer to array of packet objects that gets initialized
 *                    with the address of the allocated \a num_packets packets 
 *                    in the queue. Expecting the number of entries to be 
 *                    atleast \a num_packets.
 * \param num_packets Number of packets to allocate. Has to > 0 and <= 15.
 * \param wait_kind   Defines the kind of wait a core performs when waiting if 
 *                    channel does not have \a num_packets . The 
 *                    core can \a spin-wait or do a \a sleep-wait. If set to 
 *                    sleep-waiting, the consumer unblocks the producer using 
 *                    the \a XIPC-interrupt.
 * \return            XIPC_OK if successful, else returns 
 *                    XIPC_ERROR_INVALID_ARG if \a num_packets exceeds the 
 *                    maximum allowable number of packets.
 */
extern xipc_status_t
xipc_pkt_channel_allocate_n(xipc_pkt_channel_output_port_t *op,
                            xipc_pkt_t *pkt,
                            uint32_t num_packets,
                            xipc_wait_kind_t wait_kind);

/*! 
 * Queue up the allocated \a num_packets packets to the channel. If the 
 * \a wait_kind parameter is \a XIPC_SLEEP_WAIT, the consumer gets notified 
 * through the \a XIPC-interrupt. If \a wait_kind is set to \a XIPC_SPIN_WAIT,
 * the consumer core is not notified and is expected that the consumer core is 
 * spin-waiting.
 *  
 * \param op          Pointer to output port of the packet channel.
 * \param pkt         Pointer to array of packets that is to be sent.
 *                    Expecting the number of 
 *                    entries to be atleast \a num_packets.
 * \param num_packets Number of packets to sent. Has to > 0 and <= 15.
 * \param wait_kind   If set to \a XIPC_SPIN_WAIT, no interrupt is sent to the 
 *                    destination core, else notifies the destination core using
 *                    the \a XIPC-interrupt.
 * \return            XIPC_OK if successful, else returns 
 *                    XIPC_ERROR_INVALID_ARG if \a num_packets exceeds the 
 *                    maximum allowable number of packets.
 */
extern xipc_status_t
xipc_pkt_channel_send_n(xipc_pkt_channel_output_port_t *op,
                        xipc_pkt_t *pkt,
                        uint32_t num_packets,
                        xipc_wait_kind_t wait_kind);

/*!
 * Receive \a num_packets packet on the consumer core using the input port 
 * handle of the channel. This call blocks if the channel has less than 
 * \a num_packets available. The caller can either \a spin-wait or 
 * \a sleep-wait. If sleep-waiting, the producer of the channel has to 
 * have its \a wait_kind parameter in its call set to xipc_pkt_channel_send 
 * routines set to \a XIPC_SLEEP_WAIT to allow it to 
 * notify the consumer using the \a XIPC-interrupt whenever it 
 * generates a packet.
 * The received packets need not be contiguous in memory.
 *
 * \param ip          Pointer to input port of the packet channel.
 * \param pkt         Pointer to array of packet objects that gets 
 *                    initialized with the address of the received 
 *                    \a num_packets packets in the queue. Expecting
 *                    the number of entries to be atleast \a num_packets.
 * \param num_packets Number of packets to receive. Has to > 0 and <= 15.
 * \param wait_kind   Defines the kind of wait a core performs when blocking if 
 *                    channel is empty. The core can \a spin-wait or do a 
 *                    \a sleep-wait. If set to \a sleep-waiting, the producer 
 *                    notifies the consumer using the \a XIPC-interrupt at the 
 *                    end of the call.
 * \return            XIPC_OK if successful, else returns 
 *                    XIPC_ERROR_INVALID_ARG if \a num_packets exceeds the 
 *                    maximum allowable number of packets.
 */
extern xipc_status_t
xipc_pkt_channel_recv_n(xipc_pkt_channel_input_port_t *ip,
                        xipc_pkt_t *pkt,
                        uint32_t num_packets,
                        xipc_wait_kind_t wait_kind);

/*!
 * Release \a num_packets packets back to the channel.
 * If the \a wait_kind parameters is \a XIPC_SLEEP_WAIT, the producer 
 * gets notified through the \a XIPC-interrupt. If \a wait_kind is set to 
 * \a XIPC_SPIN_WAIT, the producer core is not notified and is expected that 
 * the producer core is spin-waiting.
 *
 * \param ip          Pointer to input port of the channel
 * \param pkt         Pointer to array of packet objects that is to be 
 *                    returned to the queue. Expecting the number of entries 
 *                    to be atleast \a num_packets.
 * \param num_packets Number of packets to be released. Has to > 0 and <= 15.
 * \param wait_kind   If set to \a XIPC_SPIN_WAIT, no interrupt is sent to 
 *                    the producer core, else notifies the producer core using
 *                    the \a XIPC-interrupt.
 * \return            XIPC_OK if successful, else returns 
 *                    XIPC_ERROR_INVALID_ARG if \a num_packets exceeds the 
 *                    maximum allowable number of packets.
 */
extern xipc_status_t
xipc_pkt_channel_release_n(xipc_pkt_channel_input_port_t *ip,
                           xipc_pkt_t *pkt,
                           uint32_t num_packets,
                           xipc_wait_kind_t wait_kind);

/*! 
 * Returns the number of queued packets. If channel is empty and block is 
 * true, this call blocks else returns 0.
 *
 * \param ip    Pointer to input port of the packet channel.
 * \param block Block waiting until channel is not empty
 * \return      Number of queued packets. Returns 0 if the channel is empty 
 *              and block is not true.
 */
extern uint8_t
xipc_pkt_channel_num_sent(xipc_pkt_channel_input_port_t *ip, uint8_t block);

/*! 
 * Sleep or spin wait until \a num_packets are received.
 *
 * \param ip          Pointer to input port of the packet channel.
 * \param num_packets Number of packets to be released. Has to > 0 and <= 15.
 * \param wait_kind   If set to \a XIPC_SLEEP_WAIT, expects the producer core
 *                    to notify the caller (consumer) using the 
 *                    \a XIPC-interrupt.
 * \return            XIPC_OK if successful, else returns 
 *                    XIPC_ERROR_INVALID_ARG if \a num_packets exceeds the 
 *                    maximum allowable number of packets.
 */
extern xipc_status_t
xipc_pkt_channel_wait_num_recv(xipc_pkt_channel_input_port_t *ip,
                               uint32_t num_packets,
                               xipc_wait_kind_t wait_kind);

#ifdef __cplusplus
}
#endif

#endif /* __XIPC_PKT_CHANNEL_H__ */
