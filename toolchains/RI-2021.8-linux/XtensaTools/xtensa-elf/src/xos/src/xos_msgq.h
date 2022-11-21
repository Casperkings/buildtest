/** @file */

// xos_msgq.h - XOS Message Queue API and data structures.

// Copyright (c) 2015-2020 Cadence Design Systems, Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// NOTE: Do not include this file directly in your application. Including
// xos.h will automatically include this file.


#ifndef XOS_MSGQ_H
#define XOS_MSGQ_H

#include "xos_types.h"

#ifdef __cplusplus
extern "C" {
#endif


//-----------------------------------------------------------------------------
//  XosMsgQueue is a multi-writer multi-reader message queue implementation.
//  It is completely thread-safe and can be used by interrupt handlers.
//  Interrupt handlers are guaranteed not to block when trying to send or
//  receive a message. Messages are copied into the queue. The queue contains
//  storage for a fixed number of messages defined at queue creation time.
//  Messages must be a multiple of 4 bytes long (padded if necessary) and the
//  message buffers must be 4-byte aligned.
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//  Message Queue flags.
//-----------------------------------------------------------------------------
#define XOS_MSGQ_WAIT_PRIORITY  0x0000U ///< Wake waiters in priority order (default)
#define XOS_MSGQ_WAIT_FIFO      0x0001U ///< Wake waiters in FIFO order
#define XOS_MSGQ_FULL           0x0002U // Queue is full
#define XOS_MSGQ_DELETED        0x8000U // Queue is deleted


//-----------------------------------------------------------------------------
///
///  XosMsgQueue object.
///
//-----------------------------------------------------------------------------
typedef struct XosMsgQueue {
    uint16_t            flags;          ///< queue flags
    uint16_t            count;          ///< # of messages queue can hold
    uint32_t            msize;          ///< message size in bytes
    uint16_t            head;           ///< write pointer
    uint16_t            tail;           ///< read pointer
    XosThreadQueue      readq;          ///< reader wait queue
    XosThreadQueue      writeq;         ///< writer wait queue
    uint32_t            sig;            // debug signature
#if XOS_OPT_STATS
    uint32_t            num_send;       ///< # of messages put to queue
    uint32_t            num_recv;       ///< # of messages taken from queue
    uint32_t            num_send_blks;  ///< # of times thread blocked on send
    uint32_t            num_recv_blks;  ///< # of times thread blocked on recv
#endif
    uint32_t            msg[1];         ///< first word of message buffer
} XosMsgQueue;


// parasoft-begin-suppress MISRAC2012-DIR_4_9-a-4 "Macros retained for backward compatibility"
// parasoft-begin-suppress MISRAC2012-RULE_20_7-a-2 "Not all parameters can be parenthesized"

//-----------------------------------------------------------------------------
///
///  Use this macro to statically allocate a message queue.
///  For example, this allocates a queue named "testq", containing 10 messages,
///  each 16 bytes long.
///
///      XOS_MSGQ_ALLOC(testq, 10, 16);
///
///  \param     name            The queue name, i.e. the name of the pointer
///                             to the queue. Used as the queue handle in
///                             queue API calls.
///
///  \param     num             Number of messages to allocate in queue. Must be > 0.
///
///  \param     size            Message size in bytes. Must be > 0 and multiple of 4.
///
//-----------------------------------------------------------------------------
#define XOS_MSGQ_ALLOC(name, num, size) \
    struct name ## _t { \
        XosMsgQueue q;  \
        uint32_t    b[((num) * (size) / sizeof(uint32_t)) - 1]; \
    }; \
    static struct name ## _t name ## _buf; \
    XosMsgQueue * name = &(name ## _buf.q);

//-----------------------------------------------------------------------------
///
///  Use this macro to dynamically allocate a message queue.
///  For example, this allocates a queue named "testq", containing 10 messages,
///  each 16 bytes long.
///
///      XosMsgQueue * testq = malloc( XOS_MSGQ_SIZE(10, 16) );
///
///  \param     num             Number of messages to allocate in queue. Must be > 0.
///
///  \param     size            Message size in bytes. Must be > 0 and multiple of 4.
///
//-----------------------------------------------------------------------------
#define XOS_MSGQ_SIZE(num, size) \
    (sizeof(XosMsgQueue) + ((num) * (size)) - 4)

// parasoft-end-suppress MISRAC2012-DIR_4_9-a-4
// parasoft-end-suppress MISRAC2012-RULE_20_7-a-2


//-----------------------------------------------------------------------------
///
///  Create the message queue object. Memory for the queue must be allocated by
///  the caller, either statically or via dynamic allocation. See the macros
///  XOS_MSGQ_ALLOC and XOS_MSGQ_SIZE for examples.
///
///  \param     msgq            Handle (pointer) to message queue.
///
///  \param     num             Number of messages allocated in queue. Must be > 0.
///
///  \param     size            Message size in bytes. Must be > 0 and multiple of 4.
///
///  \param     flags           Queue flags:
///                             - XOS_MSGQ_WAIT_FIFO - blocked threads will be
///                               woken in FIFO order.
///                             - XOS_MSGQ_WAIT_PRIORITY - blocked threads will
///                               be woken in priority order (default).
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_msgq_create(XosMsgQueue * msgq, uint16_t num, uint32_t size, uint16_t flags);


//-----------------------------------------------------------------------------
///
///  Destroys the specified queue. Any waiting threads are unblocked with an 
///  error return. Any messages in the queue will be lost.
///
///  \param     msgq            Pointer to message queue.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_msgq_delete(XosMsgQueue * msgq);


//-----------------------------------------------------------------------------
///
///  Put a message into the queue. The message contents are copied into the next
///  available message slot. If no space is available, this function will block
///  if called from a thread, but will return immediately if called from an
///  interrupt handler.
///
///  \param     msgq            Pointer to message queue.
///
///  \param     msg             Pointer to message buffer.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_msgq_put(XosMsgQueue * msgq, const uint32_t * msg);


//-----------------------------------------------------------------------------
///
///  Put a message into the queue. The message contents are copied into the next
///  available message slot. If no space is available, this function will block
///  if called from a thread, but will return immediately if called from an
///  interrupt handler. The thread will be unblocked when space frees up in the
///  queue or the timeout expires.
///
///  \param     msgq            Pointer to message queue.
///
///  \param     msg             Pointer to message buffer.
///
///  \param     to_cycles       Timeout in cycles. Convert from time to cycles
///                             using the helper functions provided in xos_timer.
///                             A value of zero indicates no timeout.
///
///  \return    Returns XOS_OK on success, XOS_ERR_TIMEOUT on timeout, else error code.
///
///  NOTE: If XOS_OPT_WAIT_TIMEOUT is not enabled, then the timeout value is
///  ignored, and no timeout will occur.
///
//-----------------------------------------------------------------------------
int32_t
xos_msgq_put_timeout(XosMsgQueue * msgq, const uint32_t * msg, uint64_t to_cycles);


//-----------------------------------------------------------------------------
///
///  Get a message from the queue. The message contents are copied into the 
///  buffer that must be provided. If no message is available, this function
///  will block if called from a thread, but will return immediately if called
///  from an interrupt handler.
///
///  \param     msgq            Pointer to message queue.
///
///  \param     msg             Pointer to message buffer.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_msgq_get(XosMsgQueue * msgq, uint32_t * msg);


//-----------------------------------------------------------------------------
///
///  Get a message from the queue. The message contents are copied into the 
///  buffer that must be provided. If no message is available, this function
///  will block if called from a thread, but will return immediately if called
///  from an interrupt handler. The thread will be unblocked when a message
///  arrives in the queue or the timeout expires.
///
///  \param     msgq            Pointer to message queue.
///
///  \param     msg             Pointer to message buffer.
///
///  \param     to_cycles       Timeout in cycles. Convert from time to cycles
///                             using the helper functions provided in xos_timer.
///                             A value of zero indicates no timeout.
///
///  \return    Returns XOS_OK on success, XOS_ERR_TIMEOUT on timeout, else error code.
///
///  NOTE: If XOS_OPT_WAIT_TIMEOUT is not enabled, then the timeout value is
///  ignored, and no timeout will occur.
///
//-----------------------------------------------------------------------------
int32_t
xos_msgq_get_timeout(XosMsgQueue * msgq, uint32_t * msg, uint64_t to_cycles);


//-----------------------------------------------------------------------------
///
///  Check if the queue is empty.
///
///  \param     msgq            Pointer to message queue.
///
///  \return    Returns nonzero if queue is empty, zero if queue is not empty.
///
//-----------------------------------------------------------------------------
int32_t
xos_msgq_empty(const XosMsgQueue * msgq);


//-----------------------------------------------------------------------------
///
///  Check if the queue is full.
///
///  \param     msgq            Pointer to message queue.
///
///  \return    Returns nonzero if queue is full, zero if queue is not full.
///
//-----------------------------------------------------------------------------
int32_t
xos_msgq_full(const XosMsgQueue * msgq);


#ifdef __cplusplus
}
#endif

#endif // XOS_MSGQ_H

