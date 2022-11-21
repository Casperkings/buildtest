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
#ifndef __XIPC_H__
#define __XIPC_H__

/*! @file */

#ifdef __cplusplus
extern "C" {
#endif

#include "xipc_addr.h"
#include "xipc_intr.h"
#include "xipc_mutex.h"
#include "xipc_barrier.h"
#include "xipc_msg_channel.h"
#include "xipc_pkt_channel.h"
#include "xipc_counted_event.h"
#include "xipc_cond.h"
#include "xipc_cqueue.h"
#include "xipc_sem.h"
#include "xipc_rpc.h"

/*! 
 * Top level initialization routine. This function needs to be called prior to 
 * using all the XIPC primitives. It initializes the \a XIPC-interrupt 
 * handlers. All cores are synchronized at the end of this call using the 
 * \ref xipc_reset_sync barrier function. Before using any primitives it is 
 * recommended to first initialize them from one of the cores in the subsystem,
 * using the corresponding init functions like \a xipc_barrier_init, 
 * \a xipc_mutex_init, \a xipc_counted_event_int, or \a xipc_cond_init. 
 * Following this, all cores invokes xipc_initialize to setup the 
 * \a XIPC-interrupt handlers and other library specific initialization. 
 * It is safe to call this before threads are initialized when using XOS.
 */
extern void xipc_initialize();

/*! 
 * Initializes pre-defined inter-processor message channels. 
 * This function sets up message
 * channels between every pair of processors, which needs to be called
 * prior to using the message/packet channels.
 * Note, the packet channels use the message channels for the initial handshake.
 * Invoke this function after \ref xipc_initialize.
 * It is safe to call this before threads are initialized when using XOS.
 */
extern void xipc_setup_channels();

#ifdef __cplusplus
}
#endif

#endif /* __XIPC_H__ */
