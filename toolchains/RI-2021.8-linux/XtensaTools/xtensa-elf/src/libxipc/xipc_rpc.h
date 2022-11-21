/* Copyright (c) 2003-2021 Cadence Design Systems, Inc.
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

#ifndef __XIPC_RPC_H__
#define __XIPC_RPC_H__

/*! @file */

#ifdef __cplusplus
extern "C" {
#endif

#include "xipc_pkt_channel.h"
#include "xipc_common.h"

/*! Minimum size of a RPC event object in bytes. */
#define XIPC_RPC_EVENT_STRUCT_SIZE  (8)

/*! Minimum command queue structure header size in bytes */
#define XIPC_RPC_CMD_QUEUE_MIN_SIZE (512)

/*! Minimum command header size in bytes */
#define XIPC_RPC_CMD_MIN_SIZE       (20)

/*! All custom namespace handlers should follow this id */
#define XIPC_RPC_SYS_CMD_LAST       (255)

/*! Macro to compute size of the command queue. It takes as parameters the
 *  the max size of each command (commands all have fixed sizes) and the number
 *  of outstanding commands. This macro can be used to dynamically 
 *  create a command queue of type \ref xipc_rpc_queue_t. */
#define XIPC_RPC_CMD_QUEUE_SIZE(CMD_SIZE, NUM_CMDS)                           \
        (XIPC_RPC_CMD_QUEUE_MIN_SIZE +                                        \
         ((XIPC_RPC_CMD_MIN_SIZE+(CMD_SIZE))*NUM_CMDS))

/*! Macro used to statically create a command queue of a given of type
 *  \ref xipc_rpc_queue_t with a given name, max command size, 
 *  and max number of outstanding commands. Note, the command queue structure
 *  has to reside in either local memory of the server, L2-RAM, or any other
 *  uncached memory shared between the client and the server. */
#define XIPC_RPC_CREATE_CMD_QUEUE(NAME, CMD_SIZE, NUM_CMDS) \
        xipc_rpc_queue_t NAME[XIPC_RPC_CMD_QUEUE_SIZE(CMD_SIZE, NUM_CMDS)];

#ifndef __XIPC_RPC_INTERNAL_H__
#ifdef XIPC_CUSTOM_SUBSYSTEM
#include "xmp_custom_subsystem.h"
#else
#include <xtensa/system/xmp_subsystem.h>
#endif

struct xipc_rpc_queue_struct {
  char _;
};

struct xipc_rpc_event_struct {
  char _[((XIPC_RPC_EVENT_STRUCT_SIZE+XMP_MAX_DCACHE_LINESIZE-1)/XMP_MAX_DCACHE_LINESIZE)*XMP_MAX_DCACHE_LINESIZE];
}  __attribute__ ((aligned (XMP_MAX_DCACHE_LINESIZE)));

#endif /* __XIPC_RPC_INTERNAL_H__ */

/*! RPC APIs uses handle of this type to refer to a command queue.  */
typedef struct xipc_rpc_queue_struct xipc_rpc_queue_t;

/*! RPC APIs uses handle of this type to refer to a command completion event  */
typedef struct xipc_rpc_event_struct xipc_rpc_event_t;

/*! RPC namespace handler */
typedef xipc_status_t (xipc_rpc_ns_handler)(void *, uint32_t,
                                            void *, uint32_t);

/*! 
 * Server side initialization of the commmand queue structure. 
 * The command queue structure has to reside in either local memory of 
 * the server, L2-RAM, or any other uncached memory shared between the 
 * client and the server. 
 *
 * \param queue      Pointer to command queue structure in shared memory
 * \param client_pid Processor ID of the unique client of the command queue
 * \param cmd_size   Maximum size of the command structure to be enqueued in
 *                   the command queue. Should match the size used when 
 *                   declaring the command queue using the macros 
 *                   \ref XIPC_RPC_CMD_QUEUE_SIZE or
 *                   \ref XIPC_RPC_CREATE_CMD_QUEUE.
 * \param num_cmds   Maximum number of outstanding commands in the queue.
 *                   Should match the number used when declaring
 *                   the command queue using the macros 
 *                   \ref XIPC_RPC_CMD_QUEUE_SIZE or
 *                   \ref XIPC_RPC_CREATE_CMD_QUEUE.
 *
 * \return           XIPC_OK if successful, else returns one of
 *                   - XIPC_ERROR_INTERNAL,
 *                   - XIPC_ERROR_INVALID_ARG, or 
 *                   - XIPC_ERROR_RPC_INIT
 */
extern xipc_status_t 
xipc_rpc_server_queue_initialize(xipc_rpc_queue_t *queue,
                                 xipc_pid_t client_pid,
                                 uint32_t cmd_size,
                                 uint32_t num_cmds);

/*! 
 * Initialization of the shared commmand queue structure. 
 * The shared command queue structure has to reside in L2-RAM, 
 * local memory, or any other uncached memory shared among all the
 * users of the queue. This method needs to be invoked by all processors
 * sharing the queue. Note, that the queue needs to reside in a memory space
 * that supports atomics and all users needs to see the same
 * shared address.
 *
 * \param queue      Pointer to command queue structure in shared memory
 * \param pids       Array of processor IDs of all processors sharing the queue
 * \param num_pids   Number of pids in the \a pids array
 * \param cmd_size   Maximum size of the command structure to be enqueued in
 *                   the command queue. Should match the size used when 
 *                   declaring the command queue using the macros 
 *                   \ref XIPC_RPC_CMD_QUEUE_SIZE or
 *                   \ref XIPC_RPC_CREATE_CMD_QUEUE.
 * \param num_cmds   Maximum number of outstanding commands in the queue.
 *                   Should match the number used when declaring
 *                   the command queue using the macros 
 *                   \ref XIPC_RPC_CMD_QUEUE_SIZE or
 *                   \ref XIPC_RPC_CREATE_CMD_QUEUE.
 *
 * \return           XIPC_OK if successful, else returns one of
 *                   - XIPC_ERROR_INTERNAL,
 *                   - XIPC_ERROR_INVALID_ARG, or 
 *                   - XIPC_ERROR_RPC_INIT
 */
extern xipc_status_t 
xipc_rpc_squeue_initialize(xipc_rpc_queue_t *queue,
                           xipc_pid_t *pids,
                           uint32_t num_pids,
                           uint32_t cmd_size,
                           uint32_t num_cmds);

/*!
 * Server side dispatch of the command to the default xipc_rpc_execute_cmd
 * handler
 *
 * \param queue Pointer to command queue structure in shared memory
 *
 * \return      XIPC_OK if command execution is successful, else one of
 *              - XIPC_ERROR_INVALID_ARG,
 *              - XIPC_ERROR_RPC_CMDQ_EMPTY,
 *              - XIPC_ERROR_RPC_CMD_EXEC_FAIL
 *              - XIPC_ERROR_RPC_SHUTDOWN
 */
extern xipc_status_t 
xipc_rpc_cmd_dispatch(xipc_rpc_queue_t *queue);

/*!
 * Client side initialization of the command queue structure.
 * The command queue structure has to reside in either local memory of 
 * the server, L2-RAM, or any other uncached memory shared between the 
 * client and the server. 
 *
 * \param queue      Pointer to command queue structure in shared memory
 * \param client_pid Processor ID of the unique server of the command queue
 *
 * \return           XIPC_OK if successful, else returns one of
 *                   - XIPC_ERROR_INVALID_ARG, or 
 *                   - XIPC_ERROR_RPC_INIT
 */
extern xipc_status_t 
xipc_rpc_client_queue_initialize(xipc_rpc_queue_t *queue,
                                 xipc_pid_t server_pid);

/*!
 * Enqueue a command from the client to be executed by the server.
 *
 * \param queue       Pointer to command queue structure in shared memory
 * \param nsid        Namespace id. Set to 0 to use the default 
 *                    xipc_rpc_execute_cmd handler
 * \param cmd         Pointer to the command to be enqueued
 * \param cmd_size    Size of the command in bytes
 * \param output      Pointer to the output param passed to xipc_rpc_execute_cmd
 *                    Can be NULL. Note, if output is in non-coherent
 *                    cached space, dcache writebacks need to be done after
 *                    command is executed on the server side, and dcache
 *                    invalidates needs to be done prior to reading the results
 *                    on the client side.
 * \param output_size Size of the output param in bytes. If \a output is NULL,
 *                    set this to 0.
 * \param event       Pointer to event structure to query for the 
 *                    completion status of the command. Can be NULL.
 *
 * \return            XIPC_OK if successful, else one of 
 *                    - XIPC_ERROR_INVALID_ARG
 *                    - XIPC_ERROR_RPC_ENQUEUE
 */
extern xipc_status_t 
xipc_rpc_run_cmd(xipc_rpc_queue_t *queue, uint32_t nsid, void *cmd, 
                 uint32_t cmd_size, void *output, uint32_t output_size,
                 xipc_rpc_event_t *event);

/*!
 * Enqueue a synchronous command from the client to be executed by the server.
 *
 * \param queue       Pointer to command queue structure in shared memory
 * \param nsid        Namespace id. Set to 0 to use the default 
 *                    xipc_rpc_execute_cmd handler
 * \param cmd         Pointer to the command to be enqueued
 * \param cmd_size    Size of the command in bytes
 * \param output      Pointer to the output param passed to xipc_rpc_execute_cmd
 *                    Can be NULL. Note, if output is in non-coherent
 *                    cached space, dcache writebacks need to be done after
 *                    command is executed on the server side, and dcache
 *                    invalidates needs to be done prior to reading the results
 *                    on the client side.
 * \param output_size Size of the output param in bytes. If \a output is NULL,
 *                    set this to 0.
 *
 * \return            XIPC_OK if successful, else one of 
 *                    - XIPC_ERROR_INVALID_ARG
 *                    - XIPC_ERROR_RPC_ENQUEUE
 */
extern xipc_status_t 
xipc_rpc_run_cmd_sync(xipc_rpc_queue_t *queue, uint32_t nsid, void *cmd, 
                      uint32_t cmd_size, void *output, uint32_t output_size);

/*!
 * Allocate a pointer to the command buffer from the command queue. This
 * provides zero-copy command dispatch.
 *
 * \param queue       Pointer to command queue structure in shared memory
 * \param event       Pointer to \a xipc_status_t object
 *
 * \return            Pointer to command buffer and sets \a status to XIPC_OK 
 *                    if successful, else returns NULL and sets \a status to
 *                    XIPC_ERROR_INVALID_ARG
 */
extern void * 
xipc_rpc_alloc_cmd_buf(xipc_rpc_queue_t *queue, xipc_status_t *status);

/*!
 * Enqueue a command buffer from the client to be executed by the server.
 * The cmd needs to be allocated using the corresponding 
 * \ref xipc_rpc_alloc_cmd_buf API.
 *
 * \param queue       Pointer to command queue structure in shared memory
 * \param nsid        Namespace id. Set to 0 to use the default 
 *                    xipc_rpc_execute_cmd handler
 * \param cmd         Pointer to the command to be enqueued
 * \param output      Pointer to the output param passed to xipc_rpc_execute_cmd
 *                    Can be NULL. Note, if output is in non-coherent
 *                    cached space, dcache writebacks need to be done after
 *                    command is executed on the server side, and dcache
 *                    invalidates needs to be done prior to reading the results
 *                    on the client side.
 * \param output_size Size of the output param in bytes. If \a output is NULL,
 *                    set this to 0.
 * \param event       Pointer to event structure to query for the 
 *                    completion status of the command. Can be NULL.
 *
 * \return            XIPC_OK if successful, else one of 
 *                    - XIPC_ERROR_INVALID_ARG
 *                    - XIPC_ERROR_RPC_ENQUEUE
 */
extern xipc_status_t 
xipc_rpc_enqueue_cmd_buf(xipc_rpc_queue_t *queue, uint32_t nsid, void *cmd, 
                         void *output, uint32_t output_size,
                         xipc_rpc_event_t *event);

/*!
 * Enqueue a command buffer from the client to be executed synchronous 
 * by the server. The cmd needs to be allocated using the corresponding 
 * \ref xipc_rpc_alloc_cmd_buf API.
 *
 * \param queue       Pointer to command queue structure in shared memory
 * \param nsid        Namespace id. Set to 0 to use the default 
 *                    xipc_rpc_execute_cmd handler
 * \param cmd         Pointer to the command to be enqueued
 * \param output      Pointer to the output param passed to xipc_rpc_execute_cmd
 *                    Can be NULL. Note, if output is in non-coherent
 *                    cached space, dcache writebacks need to be done after
 *                    command is executed on the server side, and dcache
 *                    invalidates needs to be done prior to reading the results
 *                    on the client side.
 * \param output_size Size of the output param in bytes. If \a output is NULL,
 *                    set this to 0.
 *
 * \return            XIPC_OK if successful, else one of 
 *                    - XIPC_ERROR_INVALID_ARG
 *                    - XIPC_ERROR_RPC_ENQUEUE
 */
extern xipc_status_t 
xipc_rpc_enqueue_cmd_buf_sync(xipc_rpc_queue_t *queue, uint32_t nsid, 
                              void *cmd, void *output, uint32_t output_size);

/*!
 * Enqueue a command from the client to shutdown the server.
 *
 * \param queue Pointer to command queue structure in shared memory
 *
 * \return      XIPC_OK if successful, else one of 
 *              - XIPC_ERROR_INVALID_ARG
 *              - XIPC_ERROR_RPC_ENQUEUE
 */
extern xipc_status_t 
xipc_rpc_client_shutdown(xipc_rpc_queue_t *queue);

/*!
 * Server side sleep wait until a command is enqueued.
 *
 * \param queue Pointer to command queue structure in shared memory
 *
 * \return      XIPC_OK if successful, else XIPC_ERROR_INVALID_ARG
 */
extern xipc_status_t xipc_rpc_cmd_wait(xipc_rpc_queue_t *queue);

/*!
 * Sleep wait for event to be done
 *
 * \param event Pointer to event structure to query for the 
 *              completion status of the command
 *
 * \return      XIPC_OK if successful, else XIPC_ERROR_INVALID_ARG
 */
extern xipc_status_t 
xipc_rpc_wait_event(xipc_rpc_event_t *event);

/*!
 * Sleep wait for any event to be done
 *
 * \param event      Event set to wait for
 * \param num_events Number of events in the event set
 * \param status     Set to XIPC_OK if successful else XIPC_ERROR_INVALID_ARG
 *
 * \return           Index of earliest event that is done. 
 */
extern uint32_t 
xipc_rpc_wait_event_any(xipc_rpc_event_t *events[], uint32_t num_events,
                        xipc_status_t *status);

/*!
 * Check if event is done
 *
 * \param event Pointer to event structure to query for the 
 *              completion status of the command
 *
 * \return      XIPC_OK if successful, else returns one of
 *              - XIPC_ERROR_INVALID_ARG
 *              - XIPC_ERROR_RPC_CMD_PENDING
 */
extern xipc_status_t 
xipc_rpc_check_event(xipc_rpc_event_t *event);

/*! 
 * Command handler that is called by the \ref xipc_rpc_cmd_dispatch API.
 * User needs to override this and provide a more concrete implementation.
 * Note, if the output object is placed in cached memory, the server will
 * need to perform appropriate dcache writebacks after writing into
 * the output buffer.
 *
 * \param cmd         Pointer to command enqueued in the underlying 
 *                    command queue buffer
 * \param cmd_size    Size of command enqueued by the client in bytes
 * \param output      Pointer to the output object enqueued by client
 * \param output_size Size of output object enqueued by the client in bytes
 * 
 * \return            The default implementation returns 
 *                    \a XIPC_ERROR_RPC_CMD_EXEC_FAIL.
 *                    The user needs to return \a XIPC_OK at the end of 
 *                    command execution. The return value 
 *                    of the command handler is returned by the
 *                    \ref xipc_rpc_cmd_dispatch API.
 */
extern 
xipc_status_t xipc_rpc_execute_cmd(void *cmd, uint32_t cmd_size,
                                   void *output, uint32_t output_size)
                                   __attribute__((weak));

/*!
 * Register a handler for a given namespace
 *
 * \param nsid    Namespace handler id. Has to be > XIPC_RPC_SYS_CMD_LAST
 * \param handler Namespace handler function
 *
 * \return        XIPC_OK if successful, else one of 
 *                - XIPC_ERROR_RPC_INVALID_NSID
 *                - XIPC_ERROR_RPC_DUPLICATE_NSID
 *                - XIPC_ERROR_INVALID_ARG
 */
extern
xipc_status_t xipc_rpc_register_namespace_handler(uint32_t nsid,
                                                  xipc_rpc_ns_handler *handler);

#ifdef __cplusplus
}
#endif

#endif /* __XIPC_RPC_H__ */
