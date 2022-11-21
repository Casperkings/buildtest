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
#ifndef __XIPC_COMMON_H__
#define __XIPC_COMMON_H__

/*! @file */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/*! Maximum supported processors in the subsystem */
#define XIPC_MAX_NUM_PROCS       16

/*! Type for processor id */
typedef uint8_t xipc_pid_t;

/*! Defines the boolean values true and false */
typedef enum {
  XIPC_FALSE = 0,
  XIPC_TRUE  = 1,
} xipc_bool_t;

/*!
 * This is used by the XIPC APIs to represent different error conditions.
 */
typedef enum {
  /*! No Error */ 
  XIPC_OK                                       =  0, 
  /*! Arguments to the API functions are not valid  */
  XIPC_ERROR_INVALID_ARG                        = -1, 
  /*! Attempting to release a mutex not acquired by the core */
  XIPC_ERROR_MUTEX_NOT_OWNER                    = -2,
  /*! Mutex has already been acquired */
  XIPC_ERROR_MUTEX_ACQUIRED                     = -3,
  /*! Maximum number of waiters limit reached on mutex */
  XIPC_ERROR_MUTEX_MAX_WAITERS                  = -4,
  /*! Could not allocate data in the concurrent queue */
  XIPC_ERROR_CQUEUE_ALLOCATE                    = -5,
  /*! Could not allocate data in the concurrent queue */
  XIPC_ERROR_CQUEUE_RECV                        = -6,
  /*! Event counter overflowed */
  XIPC_ERROR_EVENT_SET                          = -7,
  /*! Attributes were duplicated at both ends of the channel */
  XIPC_ERROR_PKT_CHANNEL_DUPL_ATTR              = -8,
  /*! Attribute type not supported for the channel */
  XIPC_ERROR_PKT_CHANNEL_UNKNOWN_ATTR           = -9,
  /*! The buffer address was not set on either ends of the channel */
  XIPC_ERROR_PKT_CHANNEL_UNDEF_BUFFER_ADDR_ATTR = -10,
  /*! The packet size not set on either ends of the channel */
  XIPC_ERROR_PKT_CHANNEL_UNDEF_PACKET_SIZE_ATTR = -11,
  /*! Error encountered when transferring packets using the send/recv_buf API */
  XIPC_ERROR_ASYNC_COPY                         = -12,
  /*! Error encountered when transferring packets using the send/recv_buf API
   * The subsystem configuration maybe violating one of the system 
   * requirements listed under \ref Subsystem_Requirements */
  XIPC_ERROR_SUBSYSTEM_UNSUPPORTED              = -13,
  /*! The channel IDs of the producer or the consumer cores of the packet 
   * channel does not match when establishing connection to the channel. */
  XIPC_ERROR_CHANNEL_CONNECT                    = -14,
  /*! Maximum number of waiters limit reached on condition object */
  XIPC_ERROR_COND_MAX_WAITERS                   = -15,
  /*! Maximum number of waiters limit reached on semaphore object */
  XIPC_ERROR_SEM_MAX_WAITERS                    = -16,
  /*! Semaphore has already been locked */
  XIPC_ERROR_SEM_LOCKED                         = -17,
  /*! Error initializing RPC */
  XIPC_ERROR_RPC_INIT                           = -18,
  /*! RPC command queue is empty */
  XIPC_ERROR_RPC_CMDQ_EMPTY                     = -19,
  /*! RPC command execution failure */
  XIPC_ERROR_RPC_CMD_EXEC_FAIL                  = -20,
  /*! Error RPC enqueue */
  XIPC_ERROR_RPC_ENQUEUE                        = -21,
  /*! RPC shutdown */
  XIPC_ERROR_RPC_SHUTDOWN                       = -22,
  /*! RPC command pending */
  XIPC_ERROR_RPC_CMD_PENDING                    = -23,
  /*! RPC namespace id not valid */
  XIPC_ERROR_RPC_INVALID_NSID                   = -24,
  /*! RPC namespace id already in use */
  XIPC_ERROR_RPC_DUPLICATE_NSID                 = -25,
  /*! Encountered an internal error */
  XIPC_ERROR_INTERNAL                           = -100
} xipc_status_t;

/*!
 * This enum defines what a core needs to do if a synchronization object is not 
 * available. Examples include waiting for cores to reach a barrier, waiting 
 * for a mutex to be released, waiting for data to be available on a channel, 
 * etc. The core could choose either to spin-wait or do a sleep-wait using the 
 * \a WAITI (wait-for-interrupt) instruction (default behavior without an OS). 
 * The \a spin-wait is useful if the application expects the synchronization 
 * to happen rather quickly and wants to avoid the overheads of the block and 
 * wakeup. The sleep-wait provides a low-power alternative if the wait time 
 * is significant. If sleep-waiting, the core that makes the synchronization 
 * object available for example like releasing the mutex or generating new 
 * data, uses the \a XIPC-interrupt (see \ref Subsystem_Requirements) to  
 * unblock the suspended cores.
 */
typedef enum {
  /*! Spins checking for the synchronization object to be available */
  XIPC_SPIN_WAIT  = 0, 
  /*! Blocks waiting for the XIPC-interrupt */
  XIPC_SLEEP_WAIT = 1
} xipc_wait_kind_t;

/*! 
 * The enum lists the event id used for generating XIPC library call trace.
 * The traces are generated when libxipc is compiled with -DXIPC_PROFILE and
 * run with xtsc simcall tracing that generates the event trace in csv format.
 * The simcall_csv output format is (see Xtensa SystemC (XTSC) Reference Manual)
 * SimulationTime,"CoreName",CycleCount,CCOUNT, followed by 6 integer arguments
 * where the arguments are
 * xipc object id, xipc_profile_event_type_t, thread_id, unused, unused, unused. 
 * For xipc_mutex_t, xipc_barrier_t, xipc_sem_t, the object id is the address
 * in memory of the respective object.
 * For xipc_msg_channel_t, xipc_pkt_channel_t, and xipc_cqueue_t, 
 * the object id is the unique channel/cqueue id.
 * The thread_id is unused under XTOS, else it is the unique thread
 * of the calling thread. 
 * To generate the profile data, compile library with BUILD_PROFILE=1 that
 * turns on -DXIPC_PROFILE, and in xtsc-run simulation, pass 
 * -set_xtsc_parm=simcall_csv_file=csv file name (or '-' for stdout) and
 * -set_xtsc_parm=simcall_csv_format=1,0,3,3,3,3 to xtsc-run
 * (within 'Extra Args' if run from Xplorer->'Run'->'Run Configurations')
 * which prints the first argument (xipc object id) in hex,
 * the second argument (event_type) in decimal, and the rest are not 
 * printed. If XOS is used, the third argument to simcall_csv_format can be
 * a '0' to print the thread_id in decimal.
 */
typedef enum {
  XIPC_PROFILE_BARRIER_ENTER               = 1,
  XIPC_PROFILE_BARRIER_LEAVE               = 2,
  XIPC_PROFILE_MUTEX_ACQUIRING             = 3,
  XIPC_PROFILE_MUTEX_ACQUIRED              = 4,
  XIPC_PROFILE_MUTEX_RELEASE               = 5,
  XIPC_PROFILE_SEM_ACQUIRING               = 6,
  XIPC_PROFILE_SEM_ACQUIRED                = 7,
  XIPC_PROFILE_SEM_RELEASE                 = 8,
  XIPC_PROFILE_MSG_CHANNEL_SENDING         = 9,
  XIPC_PROFILE_MSG_CHANNEL_SENT            = 10,
  XIPC_PROFILE_MSG_CHANNEL_RECEIVING       = 11,
  XIPC_PROFILE_MSG_CHANNEL_RECEIVED        = 12,
  XIPC_PROFILE_MSG_CHANNEL_ALLOCATING      = 13,
  XIPC_PROFILE_MSG_CHANNEL_ALLOCATED       = 14,
  XIPC_PROFILE_MSG_CHANNEL_COMMIT          = 15,
  XIPC_PROFILE_MSG_CHANNEL_RELEASED        = 16,
  XIPC_PROFILE_PKT_CHANNEL_ALLOCATING      = 17,
  XIPC_PROFILE_PKT_CHANNEL_ALLOCATED       = 18,
  XIPC_PROFILE_PKT_CHANNEL_SENT            = 19,
  XIPC_PROFILE_PKT_CHANNEL_RECEIVING       = 20,
  XIPC_PROFILE_PKT_CHANNEL_RECEIVED        = 21,
  XIPC_PROFILE_PKT_CHANNEL_RELEASED        = 22,
  XIPC_PROFILE_PKT_CHANNEL_COPYING         = 23,
  XIPC_PROFILE_CQUEUE_ALLOCATING           = 24,
  XIPC_PROFILE_CQUEUE_ALLOCATED            = 25,
  XIPC_PROFILE_CQUEUE_SENT                 = 26,
  XIPC_PROFILE_CQUEUE_RECEIVING            = 27,
  XIPC_PROFILE_CQUEUE_RECEIVED             = 28,
  XIPC_PROFILE_CQUEUE_RELEASED             = 29,
  XIPC_PROFILE_COUNTED_EVENT_SET           = 30,
  XIPC_PROFILE_COUNTED_EVENT_RESET         = 31,
  XIPC_PROFILE_COUNTED_EVENT_WAIT          = 32,
  XIPC_PROFILE_COUNTED_EVENT_RECEIVED      = 33,
  XIPC_PROFILE_COND_WAIT                   = 34,
  XIPC_PROFILE_COND_WAKEUP                 = 35,
  XIPC_PROFILE_COND_SIGNAL                 = 36,
  XIPC_PROFILE_COND_SIGNAL_DONE            = 37,
} xipc_profile_event_type_t;

#ifdef __cplusplus
}
#endif

#endif /* __XIPC_COMMON_H__ */
