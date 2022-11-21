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
#ifndef __XIPC_CQUEUE_H__
#define __XIPC_CQUEUE_H__

/*! @file */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "xipc_common.h"
#include "xipc_pkt.h"

/*! Minimum size of cqueue object in bytes. */
#define XIPC_CQUEUE_STRUCT_SIZE  264

#ifndef __XIPC_CQUEUE_INTERNAL_H__
#ifdef XIPC_CUSTOM_SUBSYSTEM
#include "xmp_custom_subsystem.h"
#else
#include <xtensa/system/xmp_subsystem.h>
#endif
struct xipc_cqueue_struct {
  char _[((XIPC_CQUEUE_STRUCT_SIZE+XMP_MAX_DCACHE_LINESIZE-1)/XMP_MAX_DCACHE_LINESIZE)*XMP_MAX_DCACHE_LINESIZE];
}  __attribute__ ((aligned (XMP_MAX_DCACHE_LINESIZE)));
#endif

/*! Data structure used to setup a concurrent queue.
 *  The minimum size of an object of this type is 
 * \ref XIPC_CQUEUE_STRUCT_SIZE. For a cached subsystem, the size is rounded 
 * and aligned to the maximum dcache line size across all cores in the 
 * subsystem. */
typedef struct xipc_cqueue_struct xipc_cqueue_t;

/*! 
 * Checks if the queue is full.
 *
 * \param cq Pointer to the queue.
 * \return   0 if full, else returns a non-zero value.
 */
extern int32_t xipc_cqueue_full(xipc_cqueue_t *cq);

/*!
 * Checks if the queue is empty.
 *
 * \param cq Pointer to the queue.
 * \return   0 if empty, else returns a non-zero value.
 */
extern int32_t xipc_cqueue_empty(xipc_cqueue_t *cq);

/*! 
 * Allocate a packet in the cqueue. This call can block if the channel is full.
 * The consumer unblocks all the waiting producer cores using the 
 * \a XIPC-interrupt whenever it releases a packet.
 *
 *
 * \param cq  Pointer to cqueue.
 * \param pkt Pointer to packet object that gets initialized with the 
 *            address of the allocated packet in the queue.
 */
extern void
xipc_cqueue_allocate(xipc_cqueue_t *cq, xipc_pkt_t *pkt);

/*!
 * Queue up an allocated packet to the queue and optionally notify all the 
 * consumer cores using the \a XIPC-interrupt. This unblocks all the 
 * blocked consumer cores if it is blocked waiting for data to be available. 
 *
 * \param cq        Pointer to cqueue.
 * \param pkt       Pointer to packet that is to be sent.
 * \param wait_kind If set to \a sleep_wait, the producer core notifies the 
 *                  consumer using the \a XIPC-interrupt. If set to
 *                  \a spin_wait, it is expected that the consumer is using
 *                  the non-blocking \ref xipc_cqueue_try_recv.
 */ 
extern void
xipc_cqueue_send(xipc_cqueue_t *cq, 
                 xipc_pkt_t *pkt, 
                 xipc_wait_kind_t wait_kind);

/*!
 * Receive a packet on the consumer core from the queue. This call blocks if 
 * the channel is empty. The producer unblocks any waiting consumers using the
 * \a XIPC-interrupt whenever it receives a packet.
 *
 * \param cq  Pointer to cqueue.
 * \param pkt Pointer to packet object that gets initialized with the 
 *            address of the received packet in the queue.
 */
extern void
xipc_cqueue_recv(xipc_cqueue_t *cq, xipc_pkt_t *pkt);

/*!
 * Release the packet back to the channel and optionally notify the producer 
 * core using the \a XIPC-interrupt. This unblocks the consumer cores if 
 * it is blocked waiting for the queue to free up.
 *
 * \param cq        Pointer to cqueue.
 * \param pkt       Pointer to packet object that is to be returned to 
 *                  the queue.
 * \param wait_kind If set to \a sleep_wait, the consumer core notifies the 
 *                  producer using the \a XIPC-interrupt. If set to
 *                  \a spin_wait, it is expected that the producer is using
 *                  the non-blocking \ref xipc_cqueue_try_allocate.
 */
extern void
xipc_cqueue_release(xipc_cqueue_t *cq, 
                    xipc_pkt_t *pkt, 
                    xipc_wait_kind_t wait_kind);

/*!
 * Returns the number of allocated packets in the queue.
 *
 * \param cq pointer to the cqueue.
 * \return   Number of allocated packets.
 */ 
extern uint32_t
xipc_cqueue_alloc_count(xipc_cqueue_t *cq);

/*!
 * Returns the number of free packets in the queue.
 *
 * \param cq Pointer to the queue.
 * \return   Number of free packets.
 */ 
extern uint32_t
xipc_cqueue_free_count(xipc_cqueue_t *cq);

/*!
 * This function is used to initialize the \ref xipc_cqueue_t object. This has
 * to be called prior to using the cqueue. The buffer that is used to hold 
 * the packets in the queue can be either cached or uncached address 
 * space accessible from all the cores.
 *
 * \param cq                         Pointer to cqueue to be initialized.
 * \param cqueue_id                  Unique id for the queue.
 * \param packet_size                Size of each packet in bytes.
 * \param num_packets                Number of packets in the queue. The number
 *                                   of packets cannot exceed 8.
 * \param buffer                     Pointer to user provided buffer for the 
 *                                   queue. The size of the buffer has to 
 *                                   be <em>packet_size*num_packets</em>.
 * \param buffer_pid                 If the buffer is in the local memory of 
 *                                   the processor, its processor id. If the 
 *                                   buffer is in shared memory, set this to -1.
 * \param prod_proc_pids             Pointer to an array of producer processors.
 * \param prod_num_procs             Number of entries in the \a prod_proc_pids 
 *                                   array.
 * \param cons_proc_pids             Pointer to an array of consumer processors.
 * \param cons_num_procs             Number of entries in the \a cons_proc_pids 
 *                                   array.
 * \param in_non_coherent_cached_mem Is the cq object in a memory space that is
 *                                   cached with no coherence. The protocol to
 *                                   allow for multiple reads/writes is more 
 *                                   efficient if the queue is located in 
 *                                   uncached address space (local memories, 
 *                                   for instance).
 * \return                           XIPC_OK if successful else returns one of
 *                                   - XIPC_ERROR_SUBSYSTEM_UNSUPPORTED if the
 *                                     subsystem violates the requirements 
 *                                     listed above.
 *                                   - XIPC_ERROR_INVALID_ARG if 
 *                                     \a prod_num_procs or \a cons_num_procs
 *                                     is 0 or exceeds the
 *                                     number of processors in the subsystem,
 *                                     if \a num_packets exceeds 8, or if 
 *                                     \a buffer_pid is non-negative and exceeds
 *                                     the maximum id of all the processors in
 *                                     the subsystem.
 *                                   - XIPC_ERROR_INTERNAL, if the library 
 *                                     encountered an internal error
 */
xipc_status_t
xipc_cqueue_initialize(xipc_cqueue_t *cq,
                       uint16_t cqueue_id,
                       uint16_t packet_size,
                       uint16_t num_packets,
                       void *buffer,
                       int buffer_pid,
                       xipc_pid_t *prod_proc_pids,
                       uint32_t prod_num_procs,
                       xipc_pid_t *cons_proc_pids,
                       uint32_t cons_num_procs,
                       uint32_t in_non_coherent_cached_mem);

/*!
 * Attempt to atomically allocate a packet from the queue. Returns error if a 
 * new packet could not be atomically allocated, either because the queue is 
 * full or if another producer allocated the packet while this core was 
 * trying to do the allocate.
 * 
 * \param cq  Pointer to the cqueue.
 * \param pkt Pointer to packet object that gets initialized with the address 
 *            of the allocated packet in the queue. Is valid if return value 
 *            is XIPC_OK.
 * \return    XIPC_ERROR_CQUEUE_ALLOCATE if a new packet could not be 
 *            atomically allocated. XIPC_OK if a new packet is allocated 
 *            into pkt.
 */

xipc_status_t
xipc_cqueue_try_allocate(xipc_cqueue_t *cq, xipc_pkt_t *pkt);

/*! 
 * Attempt to atomically receive a packet on the consumer core from the queue. 
 * Returns error if a new packet could not be atomically received, either 
 * because the queue is empty or if another consumer received the packet 
 * while this core was trying to do the receive.
 *
 * \param cq  Pointer to the cqueue.
 * \param pkt Pointer to packet object that gets initialized with the address 
 *            of the received packet in the queue. Is valid if return value is 
 *            XIPC_OK.
 * \return    XIPC_ERROR_CQUEUE_RECV if a new packet could not be atomically 
 *            received. XIPC_OK if a new packet is received into pkt.
 */
xipc_status_t
xipc_cqueue_try_recv(xipc_cqueue_t *cq, xipc_pkt_t *pkt);

/*!
 *  Block until a packet is available in the queue
 *
 * \param cq  Pointer to the cqueue.
 */
void
xipc_cqueue_wait_recv(xipc_cqueue_t *cq);

#ifdef __cplusplus
}
#endif

#endif /* __XIPC_CQUEUE_H__ */
