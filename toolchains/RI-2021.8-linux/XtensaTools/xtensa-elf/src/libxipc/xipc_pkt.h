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
#ifndef __XIPC_PKT_H__
#define __XIPC_PKT_H__

/*! @file */

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

struct xipc_pkt_struct {
  uintptr_t _pkt_ptr;
  size_t    _size;
};

/*!
 * A packet in the packet channel or the concurrent queue is represented using 
 * an object of this type. The user provides a pointer to such an object to 
 * the packet channel/cqueue APIs. The functions \ref xipc_pkt_get_ptr and 
 * \ref xipc_pkt_get_size are used to get to the address and size of the 
 * packet, respectively. 
 */
typedef struct xipc_pkt_struct xipc_pkt_t;

/*!
 * Returns the address of the packet buffer.
 *
 * \param pkt Pointer to the packet object.
 * \return    Address of the packet buffer.
 */
static inline void *xipc_pkt_get_ptr(xipc_pkt_t *pkt) {
  return (void *)pkt->_pkt_ptr;
}

/*!
 * Sets the address of the packet buffer.
 *
 * \param pkt Pointer to the packet object.
 * \param buf Address of the packet buffer.
 *
 */
static inline void xipc_pkt_set_ptr(xipc_pkt_t *pkt, void *buf) {
  pkt->_pkt_ptr = (uintptr_t)buf;
}

/*!
 * Returns the size of the packet. Although the maximum size of the channel is 
 * fixed at the time of channel initialization, the user can set the size of 
 * the data to be transferred in the packet to a value less than the maximum 
 * size if required. Only the size specified (defaults to the maximum size) is 
 * copied in the \ref xipc_pkt_channel_send_buf and 
 * \ref xipc_pkt_channel_recv_buf APIs.
 *
 * \param pkt Pointer to the packet object.
 * \return    Size of the packet in bytes.
 */
static inline size_t xipc_pkt_get_size(xipc_pkt_t *pkt) {
  return pkt->_size;
}

/*!
 * Sets the size of the packet. Although the maximum size of the channel is 
 * fixed at the time of channel initialization, the user can set the size of 
 * the data to be transferred in the packet to a value less than the maximum 
 * size if required. Only the size specified (defaults to the maximum size) is 
 * copied in the \ref xipc_pkt_channel_send_buf and 
 * \ref xipc_pkt_channel_recv_buf APIs.
 *
 * \param pkt  Pointer to the packet object.
 * \param size Size of the packet in bytes.
 */
static inline void xipc_pkt_set_size(xipc_pkt_t *pkt, size_t size) {
  pkt->_size = size;
}

#ifdef __cplusplus
}
#endif

#endif /* __XIPC_PKT_H__ */
