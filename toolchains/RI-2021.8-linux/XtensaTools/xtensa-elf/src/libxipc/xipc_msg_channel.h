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
#ifndef __XIPC_MSG_CHANNEL_H__
#define __XIPC_MSG_CHANNEL_H__

/*! @file */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "xipc_common.h"

/*! Minimum size of message channel object in bytes. */
#define XIPC_MSG_CHANNEL_STRUCT_SIZE             28

/*! Size of message channel input port object in bytes. */
#define XIPC_MSG_CHANNEL_INPUT_PORT_STRUCT_SIZE  24

/*! Size of message channel output port object in bytes. */
#define XIPC_MSG_CHANNEL_OUTPUT_PORT_STRUCT_SIZE 24

#ifndef __XIPC_MSG_CHANNEL_INTERNAL_H__
#ifdef XIPC_CUSTOM_SUBSYSTEM
#include "xmp_custom_subsystem.h"
#else
#include <xtensa/system/xmp_subsystem.h>
#endif
struct xipc_msg_channel_struct {
  char _[((XIPC_MSG_CHANNEL_STRUCT_SIZE+XMP_MAX_DCACHE_LINESIZE-1)/XMP_MAX_DCACHE_LINESIZE)*XMP_MAX_DCACHE_LINESIZE];
}  __attribute__ ((aligned (XMP_MAX_DCACHE_LINESIZE)));

struct xipc_msg_channel_input_port_struct {
  char _[XIPC_MSG_CHANNEL_INPUT_PORT_STRUCT_SIZE];
};

struct xipc_msg_channel_output_port_struct {
  char _[XIPC_MSG_CHANNEL_OUTPUT_PORT_STRUCT_SIZE];
};
#endif

/*! Data structure used to setup a message channel.  The minimum size of an 
 * object of this type is \ref XIPC_MSG_CHANNEL_STRUCT_SIZE. For a cache-based
 * subsystem, the size is rounded and aligned to the maximum dcache line size 
 * across all cores in the subsystem. */
typedef struct xipc_msg_channel_struct xipc_msg_channel_t;

/*! Data structure for representing a handle to the channel on the consumer 
 *  core. Expected to be in local memory. */
typedef struct xipc_msg_channel_input_port_struct xipc_msg_channel_input_port_t;

/*! Data structure for representing a handle to the channel on the producer
 *  core. Expected to be in local memory. */
typedef struct xipc_msg_channel_output_port_struct xipc_msg_channel_output_port_t;

/*!
 * Check the output port on the producer core of the message channel if the 
 * channel is full.
 *
 * \param op Pointer to message channel output port.
 * \return   0 if full, else a non-zero value.
 */
extern int32_t
xipc_msg_channel_full(xipc_msg_channel_output_port_t *op);

/*!
 * Check the input port on the consumer core of the message channel if the 
 * channel is empty.
 *
 * \param ip Pointer to message channel input port.
 * \return   0 if empty, else returns a non-zero value.
 */
extern int32_t
xipc_msg_channel_empty(xipc_msg_channel_input_port_t *ip);

/*!
 * Find the input port corresponding to the producer core of the channel.
 * The input port is used as the channel handle to receive messages from
 * the producer core.
 *
 * \param src_pid Processor ID the producer of the channel.
 * \return        Pointer to message channel input port.
 */
extern xipc_msg_channel_input_port_t *
xipc_msg_channel_get_input_port(xipc_pid_t src_pid);

/*!
 * Find the output port corresponding to the consumer core of the channel.
 * The output port is used as the channel handle to send messages to
 * the producer core.
 *
 * \param dest_pid Processor ID of the consumer of the channel.
 * \return         Pointer to message channel output port.
 */
extern xipc_msg_channel_output_port_t *
xipc_msg_channel_get_output_port(xipc_pid_t dest_pid);

/*!
 * This function is used by the producer core to send a 32-bit integer message 
 * to the consumer core using the output port handle of the channel. If the 
 * channel is full, this call blocks. The caller can either \a spin-wait or 
 * \a sleep-wait. If sleep-waiting, the consumer of channel has to have its 
 * \a wait_kind parameter in its call to \a xipc_msg_channel_recv set to 
 * \a XIPC_SLEEP_WAIT to allow it to notify the producer whenever there is free
 * space available on the channel. If the caller's (producer) \a wait_kind 
 * parameter is set to \a XIPC_SLEEP_WAIT, the producer notifies the consumer 
 * using the \a XIPC-interrupt when it generates new data. Note, the producer 
 * and the consumer cores needs to have the same \a wait_kind type. Note, no 
 * cache writebacks of the channel buffer are done by this API.
 *
 * \param op        Pointer to output port of the channel.
 * \param src       Pointer to 32-bit int that is to be sent.
 * \param wait_kind Defines the kind of wait a core performs when blocking if 
 *                  channel is full. The core can \a spin-wait or do a 
 *                  \a sleep-wait. If set to \a sleep-waiting, the producer 
 *                  notifies the consumer using the \a XIPC-interrupt at the 
 *                  end of the call.
 */
extern void
xipc_msg_channel_send(xipc_msg_channel_output_port_t *op,
                      int32_t *src,
                      xipc_wait_kind_t wait_kind);

/*!
 * This function is used by the consumer to receive a 32-bit integer message 
 * from the producer on the input port handle of the channel. If the channel 
 * is empty this call blocks. The caller can either \a spin-wait or 
 * \a sleep-wait. If sleep-waiting, the producer of the channel has to 
 * have its \a wait_kind parameter in its call set to \a xipc_msg_channel_send 
 * set to \a XIPC_SLEEP_WAIT to allow it to notify the consumer whenever it 
 * generates new data on the channel. If the caller's (consumer) \a wait_kind 
 * parameter is set to \a XIPC_SLEEP_WAIT, the consumer notifies the 
 * producer using the \a XIPC-interrupt when it consumes new data. Note, the 
 * producer and the consumer cores needs to have the same \a wait_kind type. 
 * Note, no cache invalidate of the channel buffer is done by this API.
 *
 * \param ip        Pointer to input port of the channel.
 * \param dest      Pointer to 32-bit int where the message is received.
 * \param wait_kind Defines the kind of wait a core performs when blocking if 
 *                  channel is full. The core can \a spin-wait or do a 
 *                  \a sleep-wait. If set to \a sleep-waiting, the producer 
 *                  notifies the consumer using the \a XIPC-interrupt at the 
 *                  end of the call.
 */
extern void
xipc_msg_channel_recv(xipc_msg_channel_input_port_t *ip,
                      int32_t *dest,
                      xipc_wait_kind_t wait_kind);

/*!
 * This function is used by the producer core to send one or more 32-bit 
 * integer messages to the consumer core using the output port handle of the 
 * channel. If the channel does not have free \a nwords, this call 
 * blocks. The caller can either 
 * \a spin-wait or \a sleep-wait. If sleep-waiting the consumer of channel has 
 * to have its \a wait_kind parameter in its call to \a xipc_msg_channel_recv 
 * set to \a XIPC_SLEEP_WAIT to allow it to notify the producer whenever there 
 * is free space available on the channel. If the caller's (producer) 
 * \a wait_kind parameter is set to \a XIPC_SLEEP_WAIT, the producer notifies 
 * the consumer using the \a XIPC-interrupt when it generates new data. Note, 
 * the producer and the consumer cores needs to have the same \a wait_kind 
 * type. Note, no cache writebacks of the channel buffer are done by this API.
 *
 * \param op        Pointer to output port of the message channel.
 * \param src       Pointer to 32-bit int that is to be sent.
 * \param nwords    Number of words to be sent. Expected to be > 0 and not to
 *                  exceed channel capacity.
 * \param wait_kind Defines the kind of wait a core performs when blocking if 
 *                  channel is full. The core can \a spin-wait or do a 
 *                  \a sleep-wait. If set to \a sleep-waiting, the producer 
 *                  notifies the consumer using the \a XIPC-interrupt at the 
 *                  end of the call.
 * \return          XIPC_OK, if successful, else returns XIPC_ERROR_INVALID_ARG.
 */
extern xipc_status_t
xipc_msg_channel_send_n(xipc_msg_channel_output_port_t *op,
                        int32_t *src,
                        size_t nwords,
                        xipc_wait_kind_t wait_kind);

/*!
 * This function is used by the consumer to receive one or more 32-bit integer 
 * messages from the producer on the output port handle of the channel. If the 
 * channel is does not have \a nwords, this call blocks.
 * The caller can either \a spin-wait or 
 * \a sleep-wait. If sleep-waiting, the producer of the channel has to have its 
 * \a wait_kind parameter in its call to \a xipc_msg_channel_send set to 
 * \a XIPC_SLEEP_WAIT to allow it to notify the consumer whenever it generates 
 * new data to the channel. If the caller's (consumer) \a wait_kind parameter 
 * is set to \a XIPC_SLEEP_WAIT, the consumer notifies the producer using the 
 * \a XIPC-interrupt when it consumes new data. Note, the producer and the 
 * consumer cores needs to have the same \a wait_kind type. Note, no cache 
 * invalidate of the channel buffer is done by this API.
 *
 * \param ip        Pointer to input port of the message channel.
 * \param dest      Pointer to 32-bit int where the message is received.
 * \param nwords    Number of words to be received. Expected to be > 0 and not 
 *                  to exceed channel capacity.
 * \param wait_kind Defines the kind of wait a core performs when blocking if 
 *                  channel is full. The core can \a spin-wait or do a 
 *                  \a sleep-wait. If set to \a sleep-waiting, the producer 
 *                  notifies the consumer using the \a XIPC-interrupt at the 
 *                  end of the call.
 *
 * \return          XIPC_OK, if successful, else returns XIPC_ERROR_INVALID_ARG.
 */
extern xipc_status_t
xipc_msg_channel_recv_n(xipc_msg_channel_input_port_t *ip,
                        int32_t *dest,
                        size_t nwords,
                        xipc_wait_kind_t wait_kind);

/*!
 *  Returns the number of allocated entries in the channel.
 *
 * \param op Pointer to message channel output port.
 *
 * \return   Number of allocated entries in the channel.
 */
extern uint32_t
xipc_msg_channel_output_port_count(xipc_msg_channel_output_port_t *op);

/*!
 * Returns the number of allocated entries in the channel.
 *
 * \param ip Pointer to message channel input port.
 *
 * \return   Number of allocated entries in the channel.
 */
extern uint32_t
xipc_msg_channel_input_port_count(xipc_msg_channel_input_port_t *ip);

/*!
 * Initiate connection to the source core of the message channel. The message 
 * channel structure is updated with information of the input port. 
 *
 * \param ch Pointer to message channel object.
 * \param ip Pointer to message channel input port.
 */
extern void
xipc_msg_channel_input_port_connect(xipc_msg_channel_t *ch,
                                    xipc_msg_channel_input_port_t *ip);

/*!
 * Accept connection request from the source core of the message channel. The 
 * input port is updated with the information from the output port 
 * communicated via the message channel.
 *  
 * \param ch Pointer to message channel object.
 * \param ip Pointer to message channel input port.
 */
extern void
xipc_msg_channel_input_port_accept(xipc_msg_channel_t *ch,
                                   xipc_msg_channel_input_port_t *ip);

/*! 
 * Initiate connection to the destination of the message channel. The message 
 * channel structure is updated with information of the output port.
 *
 * \param ch Pointer to message channel object.
 * \param op Pointer to message channel output port.
 */ 
extern void
xipc_msg_channel_output_port_connect(xipc_msg_channel_t *ch,
                                     xipc_msg_channel_output_port_t *op);

/*!
 * Accept connection request from the destination of the message channel. The 
 * output port is updated with the information from the input communicated 
 * via the message channel.
 *
 * \param ch Pointer to message channel object.
 * \param op Pointer to message channel output port.
 */
extern void
xipc_msg_channel_output_port_accept(xipc_msg_channel_t *ch,
                                    xipc_msg_channel_output_port_t *op);

/*!
 * Initializes the message channel. The message channel structure is used 
 * for initial handshake between the producer and consumer cores of the 
 * channel. Once the connection is established, the channel structure is no 
 * longer necessary and the producer/consumer cores uses the port handles for 
 * subsequent communication.
 *
 * \param msg_channel Message channel object to be initialized. 
 *                    Can be in cached or uncached address space that is 
 *                    accessible from the producer and consumer cores.
 * \param src_pid     Id of the source core of the message channel.
 * \param dest_pid    Id of the destination core of the message channel.
 * \param channel_id  Unique ID of the channel. Note, the subsystem reserves 
 *                    pre-defined inter-core message channels between every 
 *                    pair of cores in the subsystem. So the channel ID between
 *                    0 and (number of processor in the subsystem *
 *                           (number of processors in the subsystem - 1)
 *                    are reserved and should not be used.
 * \return            XIPC_OK if successful, else returns one of
 *                    - XIPC_ERROR_INTERNAL
 *                    - XIPC_ERROR_INVALID_ARG
 */
extern xipc_status_t
xipc_msg_channel_initialize(xipc_msg_channel_t *msg_channel,
                            xipc_pid_t src_pid,
                            xipc_pid_t dest_pid,
                            uint16_t channel_id);

/*! 
 * Initialize the message channel input port. The buffer can be in system 
 * memory (may be cached) or in local memory of the consumer processor. If in 
 * cached address space, it is the user's responsibility to maintain coherence 
 * by performing the appropriate cache invalidate and writebacks. The actual 
 * number of words available for allocation is one less than \a num_words.
 *
 * \param ip        Pointer to message channel input port.
 * \param src_pid   Processor ID of the source core.
 * \param buffer    Pointer to circular buffer from which allocation 
 *                  is performed.
 * \param num_words Number of 32-bit words in the circular buffer. The actual
 *                  number of words available for allocation is one 
 *                  less than this.
 * \return          XIPC_OK if successful else returns XIPC_ERROR_INVALID_ARG.
 */
extern xipc_status_t
xipc_msg_channel_input_port_initialize(xipc_msg_channel_input_port_t *ip,
                                       xipc_pid_t src_pid,
                                       int32_t *buffer,
                                       uint32_t num_words);

/*!
 *  Initialize the output port of the message channel.
 *
 * \param op       Pointer to message channel output port.
 * \param dest_pid Processor ID of the destination core.
 * \return         XIPC_OK if successful, else returns XIPC_ERROR_INVALID_ARG.
 */ 
extern xipc_status_t
xipc_msg_channel_output_port_initialize(xipc_msg_channel_output_port_t *op,
                                        xipc_pid_t dest_pid);

/*!
 * Check if the source of the channel is connected. This call is used by the 
 * consumer to poll if the producer has initiated connection.
 *
 * \param ch Pointer to message channel.
 * \return   1 if connected, else returns 0.
 */
extern uint32_t 
xipc_msg_channel_output_port_connected(xipc_msg_channel_t *ch);

/*!
 * Check if destination of the channel is connected. This call is used by the 
 * producer to poll if the consumer has initiated connection.
 *
 * \param ch Pointer to message channel.
 *
 * \return   1 if connected, else returns 0.
 */
extern uint32_t 
xipc_msg_channel_input_port_connected(xipc_msg_channel_t *ch);

/*! 
 * Allocates \a nwords (32-bits each) from the message circular buffer and 
 * returns a pointer to the buffer. If the channel has less than \a nwords, 
 * this call blocks. The caller can either \a spin-wait or \a sleep-wait. If 
 * sleep-waiting the consumer of the channel notifies the producer whenever it 
 * frees up buffer entries. Note, the consumer needs to do a corresponding 
 * \a XIPC_SLEEP_WAIT to allow it to notify the producer. 
 *
 * \param op        Pointer to message channel output port.
 * \param nwords    Number of words to be allocated.
 * \param wait_kind Defines the kind of wait a core performs when waiting if 
 *                  channel does not have the requested number of words. The 
 *                  core can \a spin-wait or do a \a sleep-wait. If set to 
 *                  sleep-waiting, the consumer unblocks the producer using 
 *                  the \a XIPC-interrupt.
 * \return          Pointer to buffer if successful, else 0.
 */
extern int32_t *
xipc_msg_channel_allocate(xipc_msg_channel_output_port_t *op,
                          size_t nwords,
                          xipc_wait_kind_t wait_kind);

/*!
 * Sends the previously allocated \a nwords to the consumer processor. If the 
 * \a wait_kind parameter is \a XIPC_SLEEP_WAIT, the consumer gets notified 
 * through the \a XIPC-interrupt. If \a wait_kind is set to \a XIPC_SPIN_WAIT,
 * the consumer core is not notified and is expected that the consumer core is 
 * spin-waiting.
 *
 * \param op        Pointer to message channel output port.
 * \param nwords    Number of words to be sent.
 * \param wait_kind If set to \a XIPC_SPIN_WAIT, no interrupt is sent to the 
 *                  destination core, else notifies the destination core using
 *                  the \a XIPC-interrupt.
 * \return          XIPC_OK if successful, else returns XIPC_ERROR_INVALID_ARG
 */
extern xipc_status_t
xipc_msg_channel_commit(xipc_msg_channel_output_port_t *op,
                        size_t nwords,
                        xipc_wait_kind_t wait_kind);

/*! 
 * Receives a pointer to the earliest committed buffer of size \a nwords. If 
 * the channel does not have \a nwords available, the call blocks. 
 * The caller can
 * either \a spin-wait or \a sleep-wait. If sleep-waiting the producer of the 
 * channel notifies the consumer whenever it commits buffer entries. Note, the 
 * producer needs to do a corresponding \a XIPC_SLEEP_WAIT to allow it to 
 * notify the consumer. 
 *
 * \param ip        Pointer to message channel input port.
 * \param nwords    Number of words to be received.
 * \param wait_kind Defines the kind of wait a core performs when waiting if 
 *                  channel does not have the requested number of words 
 *                  queued. The core can \a spin-wait or do a \a sleep-wait. If 
 *                  set to sleep-waiting, the producer unblocks the 
 *                  consumer using the \a XIPC-interrupt.
 * \return          Pointer to buffer if successful, else 0.
 */
extern int32_t *
xipc_msg_channel_get(xipc_msg_channel_input_port_t *ip,
                     size_t nwords,
                     xipc_wait_kind_t wait_kind);

/*!
 * Release the previously received \a nwords to the producer processor for 
 * reuse. If the \a wait_kind parameters is \a XIPC_SLEEP_WAIT, the producer 
 * gets notified through the \a XIPC-interrupt. If \a wait_kind is set to 
 * \a XIPC_SPIN_WAIT, the producer core is not notified and is expected that 
 * the producer core is spin-waiting.
 *
 * \param ip        Pointer to message channel input port.
 * \param nwords    Number of words to be released.
 * \param wait_kind If set to \a XIPC_SPIN_WAIT, no interrupt is sent to 
 *                  the producer core, else notifies the producer core using
 *                  the \a XIPC-interrupt.
 * \return          XIPC_OK if successful, else returns XIPC_ERROR_INVALID_ARG
 */
extern xipc_status_t
xipc_msg_channel_release(xipc_msg_channel_input_port_t *ip,
                         size_t nwords,
                         xipc_wait_kind_t wait_kind);

/*!
 * Returns the buffer address. If the buffer is in the local memory of the 
 * destination core, its address in the global address space is returned.
 *
 * \param op Pointer to message channel output port.
 * \return   Buffer address.
 */
extern int32_t *
xipc_msg_channel_output_port_buffer(xipc_msg_channel_output_port_t *op);

/*!
 * Returns the buffer address.
 *
 * \param ip Pointer to message channel input port.
 * \return   Buffer address.
 */
extern int32_t *
xipc_msg_channel_input_port_buffer(xipc_msg_channel_input_port_t *ip);

/*!
 * Returns the ID of the source processor of the channel
 *
 * \param ch Pointer to message channel object.
 * \return   Processor ID.
 */
extern xipc_pid_t xipc_msg_channel_get_src_pid(xipc_msg_channel_t *ch);

/*!
 * Returns the ID of the destination processor of the channel
 *
 * \param ch Pointer to message channel object.
 * \return   Processor ID.
 */
extern xipc_pid_t xipc_msg_channel_get_dest_pid(xipc_msg_channel_t *ch);

/*!
 * Returns channel ID
 *
 * \param ch Pointer to message channel object.
 * \return   Channel ID.
 */
extern uint16_t xipc_msg_channel_get_id(xipc_msg_channel_t *ch);

/*!
 * Returns channel ID
 *
 * \param ip Pointer to message channel input port.
 * \return   Channel ID.
 */
extern uint16_t 
xipc_msg_channel_input_port_get_id(xipc_msg_channel_input_port_t *ip);

/*!
 * Returns channel ID
 *
 * \param op Pointer to message channel output port.
 * \return   Channel ID.
 */
extern uint16_t
xipc_msg_channel_output_port_get_id(xipc_msg_channel_output_port_t *op);

#ifdef __cplusplus
}
#endif

#endif /* __XIPC_MSG_CHANNEL_H__ */
