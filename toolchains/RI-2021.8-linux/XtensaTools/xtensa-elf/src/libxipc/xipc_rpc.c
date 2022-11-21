#include <stdlib.h>
#include <string.h>
#include "xipc_msg_channel_internal.h"
#include "xipc_rpc_internal.h"
#include "xipc_addr.h"
#include "xipc_misc.h"

/* Pointer to list of namespace handlers */
static xipc_rpc_ns_handler_t *ns_handlers = NULL;

/* Helper function to register nsid with a handler */
static XIPC_INLINE void add_ns_handler(uint32_t nsid, 
                                       xipc_rpc_ns_handler *handler) 
{
  /* Initialize a new handler and add to start of list */
  xipc_rpc_ns_handler_t *nsh = calloc(1, sizeof(xipc_rpc_ns_handler_t));
  nsh->_nsid = nsid;
  nsh->_handler = handler;
  nsh->_next = ns_handlers;
  ns_handlers = nsh;
  XIPC_LOG("add_handler: Registering handler %p for namespace id %d\n", 
           handler, nsid);
}

/* Register namespace handler with a handler id.
 *
 * nsid    : Namespace handler id. Has to be > XIPC_RPC_SYS_CMD_LAST
 * handler : Namespace handler function
 *
 * Returns XIPC_OK if successful, else one of XIPC_ERROR_RPC_INVALID_NSID,
 * XIPC_ERROR_RPC_DUPLICATE_NSID, or XIPC_ERROR_INVALID_ARG
 */
xipc_status_t xipc_rpc_register_namespace_handler(uint32_t nsid, 
                                                  xipc_rpc_ns_handler *handler)
{
  if (nsid <= XIPC_RPC_SYS_CMD_LAST) {
    XIPC_LOG("xipc_rpc_register_namespace_handler: "
             "Namespace id has to be >= XIPC_RPC_SYS_CMD_LAST(%d)\n", 
             XIPC_RPC_SYS_CMD_LAST);
    return XIPC_ERROR_RPC_INVALID_NSID;
  }

  if (!handler)
    return XIPC_ERROR_INVALID_ARG;

  xipc_rpc_ns_handler_t *nsh = ns_handlers;
  while (nsh && nsh->_nsid != nsid) {
    nsh = nsh->_next;
  }
  if (nsh) {
    XIPC_LOG("xipc_rpc_register_namespace_handler: "
             "Namespace id %d already registered\n", nsid);
    return XIPC_ERROR_RPC_DUPLICATE_NSID;
  }

  add_ns_handler(nsid, handler);

  return XIPC_OK;
}

/* Default command handler. Declared weak for user to override.
 *
 * cmd : Command enqueued for execution
 *
 * Returns XIPC_ERROR_RPC_CMD_EXEC_FAIL by default. The user needs to
 * return XIPC_OK for a successful execution of command.
 */
xipc_status_t xipc_rpc_execute_cmd(void *cmd, uint32_t cmd_size,
                                   void *output, uint32_t output_size) 
{
  (void)cmd;
  (void)cmd_size;
  (void)output;
  (void)output_size;
  XIPC_LOG("Executing default xipc_rpc_execute_cmd handler\n");
  return XIPC_ERROR_RPC_CMD_EXEC_FAIL;
}

/* Return namespace handler given the namespace id */
static XIPC_INLINE xipc_rpc_ns_handler *get_ns_handler(uint32_t nsid) 
{
  xipc_rpc_ns_handler_t *nsh;
  for (nsh = ns_handlers; nsh && nsh->_nsid != nsid; nsh = nsh->_next)
    ;
  if (nsh) {
    XIPC_LOG("get_ns_handler: Using handler %p for nsid %d\n", 
             nsh->_handler, nsid);
    return nsh->_handler;
  }

  XIPC_LOG("get_ns_handler: Using default handler %p\n", xipc_rpc_execute_cmd);

  /* Return the default handler if no match */
  return &xipc_rpc_execute_cmd;
}

/* Handler for shutdown system command */
static xipc_status_t xipc_rpc_execute_sys_cmd_shutdown(void *cmd, 
                                                       uint32_t cmd_size,
                                                       void *output, 
                                                       uint32_t output_size) 
{
  (void)cmd;
  (void)cmd_size;
  (void)output;
  (void)output_size;
  XIPC_LOG("Executing xipc_rpc_execute_sys_cmd_shutdown handler\n");
  return XIPC_ERROR_RPC_SHUTDOWN;
}

/*
 * Enqueue a command from the client to be executed by the server.
 *
 * queue       : Pointer to command queue structure in shared memory
 * nsid        : Namespace id. Default handler uses 0.
 * cmd         : Pointer to the command to be enqueued
 * cmd_size    : Size of the command in bytes
 * output      : Pointer to the output param passed to xipc_rpc_execute_cmd
 *               Optional. Can be NULL.
 * output_size : Size of the output param in bytes
 *               If output is NULL, set this to 0.
 * event       : Pointer to event structure to query for the 
 *               completion status of the command
 *
 * Returns XIPC_OK if successful, else one of XIPC_ERROR_INVALID_ARG or
 * XIPC_ERROR_RPC_ENQUEUE
 */
xipc_status_t xipc_rpc_run_cmd(xipc_rpc_queue_t *queue, 
                               uint32_t nsid,
                               void *cmd, 
                               uint32_t cmd_size, 
                               void *output,
                               uint32_t output_size,
                               xipc_rpc_event_t *event) 
{
  xipc_status_t err;

  if (queue == NULL) {
    _Pragma("frequency_hint NEVER");
    XIPC_LOG("xipc_rpc_run_cmd: queue is null");
    return XIPC_ERROR_INVALID_ARG;
  }

  if (cmd == NULL) {
    _Pragma("frequency_hint NEVER");
    XIPC_LOG("xipc_rpc_run_cmd: cmd is null");
    return XIPC_ERROR_INVALID_ARG;
  }

  /* Initialize event */
  if (event) {
    event->_pid = xipc_get_my_pid();
    xipc_store(0, &event->_done);
  }

  xipc_pkt_t pkt;
  xipc_cqueue_t *cq = &queue->_u._header._q._sq._cqueue;
  xipc_pkt_channel_output_port_t *op = &queue->_u._header._q._uq._op;
  uint32_t shared = queue->_u._header._shared;

  /* Get packet from channel */
  if (shared) {
    xipc_cqueue_allocate(cq, &pkt);
  } else {
    xipc_pkt_channel_allocate(op, &pkt, XIPC_SPIN_WAIT);
  }

  if (cmd_size > (pkt._size - XIPC_RPC_CMD_MIN_SIZE)) {
    XIPC_LOG("xipc_rpc_run_cmd: cmd size %d exceeds max. size of %d\n",
             cmd_size, pkt._size - XIPC_RPC_CMD_MIN_SIZE);
    return XIPC_ERROR_INVALID_ARG;
  }

  /* Fill up the cmd structure */
  xipc_rpc_cmd_t *rpc_cmd = (xipc_rpc_cmd_t *)xipc_pkt_get_ptr(&pkt);
  rpc_cmd->_nsid = nsid;
  /* Translate local event address to system address space */
  rpc_cmd->_event = xipc_get_sys_addr(event, xipc_get_my_pid());
  rpc_cmd->_cmd_size = cmd_size;
  rpc_cmd->_output = output;
  rpc_cmd->_output_size = output_size;
  memcpy(rpc_cmd->_cmd, cmd, cmd_size);

  /* Enqueue packet */
  if (shared) {
    xipc_cqueue_send(cq, &pkt, XIPC_SLEEP_WAIT);
    err = XIPC_OK;
  } else {
    err = xipc_pkt_channel_send(op, &pkt, XIPC_SLEEP_WAIT);
  }
  if (err != XIPC_OK) {
    _Pragma("frequency_hint NEVER");
    XIPC_LOG("xipc_rpc_run_cmd: Error enqueueing packet, err: %d\n", err);
    return XIPC_ERROR_RPC_ENQUEUE;
  }

  XIPC_LOG("Enqueuing command with event %p\n", event);

  return XIPC_OK;
}

/*
 * Enqueue a synchronous command from the client to be executed by the server.
 *
 * queue       : Pointer to command queue structure in shared memory
 * nsid        : Namespace id. Default handler uses 0.
 * cmd         : Pointer to the command to be enqueued
 * cmd_size    : Size of the command in bytes
 * output      : Pointer to the output param passed to xipc_rpc_execute_cmd
 *               Optional. Can be NULL.
 * output_size : Size of the output param in bytes
 *               If output is NULL, set this to 0.
 *
 * Returns XIPC_OK if successful, else one of XIPC_ERROR_INVALID_ARG or
 * XIPC_ERROR_RPC_ENQUEUE
 */
xipc_status_t xipc_rpc_run_cmd_sync(xipc_rpc_queue_t *queue, 
                                    uint32_t nsid,
                                    void *cmd, 
                                    uint32_t cmd_size, 
                                    void *output,
                                    uint32_t output_size)
{
  xipc_status_t err;

  if (queue == NULL) {
    _Pragma("frequency_hint NEVER");
    XIPC_LOG("xipc_rpc_run_cmd_sync: queue is null");
    return XIPC_ERROR_INVALID_ARG;
  }

  if (cmd == NULL) {
    _Pragma("frequency_hint NEVER");
    XIPC_LOG("xipc_rpc_run_cmd_sync: cmd is null");
    return XIPC_ERROR_INVALID_ARG;
  }

  /* Initialize local event aligned and sized to max cache line size */
  uint32_t align = xmp_xipc_max_dcache_linesize;
  char ebuf[align*2];
  xipc_rpc_event_t *event = (void *)
                            (((uintptr_t)ebuf + (align - 1)) & ~(align - 1));
  event->_pid = xipc_get_my_pid();
  xipc_store(0, &event->_done);

  xipc_pkt_t pkt;
  xipc_cqueue_t *cq = &queue->_u._header._q._sq._cqueue;
  xipc_pkt_channel_output_port_t *op = &queue->_u._header._q._uq._op;
  uint32_t shared = queue->_u._header._shared;

  /* Get packet from channel */
  if (shared) {
    xipc_cqueue_allocate(cq, &pkt);
  } else {
    xipc_pkt_channel_allocate(op, &pkt, XIPC_SPIN_WAIT);
  }

  if (cmd_size > (pkt._size - XIPC_RPC_CMD_MIN_SIZE)) {
    XIPC_LOG("xipc_rpc_run_cmd_sync: cmd size %d exceeds max. size of %d\n",
             cmd_size, pkt._size - XIPC_RPC_CMD_MIN_SIZE);
    return XIPC_ERROR_INVALID_ARG;
  }

  /* Fill up the cmd structure */
  xipc_rpc_cmd_t *rpc_cmd = (xipc_rpc_cmd_t *)xipc_pkt_get_ptr(&pkt);
  rpc_cmd->_nsid = nsid;
  /* Translate local event address to system address space */
  rpc_cmd->_event = xipc_get_sys_addr(event, xipc_get_my_pid());
  rpc_cmd->_cmd_size = cmd_size;
  rpc_cmd->_output = output;
  rpc_cmd->_output_size = output_size;
  memcpy(rpc_cmd->_cmd, cmd, cmd_size);

  /* Enqueue packet */
  if (shared) {
    xipc_cqueue_send(cq, &pkt, XIPC_SLEEP_WAIT);
    err = XIPC_OK;
  } else {
    err = xipc_pkt_channel_send(op, &pkt, XIPC_SLEEP_WAIT);
  }
  if (err != XIPC_OK) {
    _Pragma("frequency_hint NEVER");
    XIPC_LOG("xipc_rpc_run_cmd_sync: Error enqueueing packet, err: %d\n", err);
    return XIPC_ERROR_RPC_ENQUEUE;
  }

  XIPC_LOG("Enqueuing sync command with event %p\n", event);

  /* Wait for command to complete */
  return xipc_rpc_wait_event(event);
}

/*
 * Get a pointer to the command buffer from the command queue
 *
 * queue : Pointer to command queue structure in shared memory
 * event : Pointer to xipc_status_t
 *
 * Returns pointer to command buffer and sets status to XIPC_OK if successful, 
 * else returns NULL with status set to XIPC_ERROR_INVALID_ARG
 */
void *xipc_rpc_alloc_cmd_buf(xipc_rpc_queue_t *queue, xipc_status_t *status)
{
  if (status == NULL) {
    _Pragma("frequency_hint NEVER");
    XIPC_LOG("xipc_rpc_alloc_cmd_buf: status is null");
    return NULL;
  }

  if (queue == NULL) {
    _Pragma("frequency_hint NEVER");
    XIPC_LOG("xipc_rpc_alloc_cmd_buf: queue is null");
    *status = XIPC_ERROR_INVALID_ARG;
    return NULL;
  }

  xipc_pkt_t pkt;
  xipc_cqueue_t *cq = &queue->_u._header._q._sq._cqueue;
  xipc_pkt_channel_output_port_t *op = &queue->_u._header._q._uq._op;
  uint32_t shared = queue->_u._header._shared;

  /* Get packet from channel */
  if (shared) {
    xipc_cqueue_allocate(cq, &pkt);
  } else {
    xipc_pkt_channel_allocate(op, &pkt, XIPC_SPIN_WAIT);
  }

  *status = XIPC_OK;

  char *pkt_buf = (char *)xipc_pkt_get_ptr(&pkt);
  return (void *)(pkt_buf + XIPC_RPC_CMD_MIN_SIZE);
}

/*
 * Enqueue a command buffer from the client to be executed by the server.
 *
 * queue       : Pointer to command queue structure in shared memory
 * nsid        : Namespace id. Default handler uses 0.
 * cmd         : Pointer to the command to be enqueued
 * output      : Pointer to the output param passed to xipc_rpc_execute_cmd
 *               Optional. Can be NULL.
 * output_size : Size of the output param in bytes
 *               If output is NULL, set this to 0.
 * event       : Pointer to event structure to query for the 
 *               completion status of the command
 *
 * Returns XIPC_OK if successful, else one of XIPC_ERROR_INVALID_ARG or
 * XIPC_ERROR_RPC_ENQUEUE
 */
xipc_status_t xipc_rpc_enqueue_cmd_buf(xipc_rpc_queue_t *queue, 
                                       uint32_t nsid,
                                       void *cmd, 
                                       void *output,
                                       uint32_t output_size,
                                       xipc_rpc_event_t *event) 
{
  xipc_status_t err;

  if (queue == NULL) {
    _Pragma("frequency_hint NEVER");
    XIPC_LOG("xipc_rpc_enqueue_cmd_buf: queue is null");
    return XIPC_ERROR_INVALID_ARG;
  }

  if (cmd == NULL) {
    _Pragma("frequency_hint NEVER");
    XIPC_LOG("xipc_rpc_enqueue_cmd_buf: cmd is null");
    return XIPC_ERROR_INVALID_ARG;
  }

  /* Initialize event */
  if (event) {
    event->_pid = xipc_get_my_pid();
    xipc_store(0, &event->_done);
  }

  xipc_pkt_t pkt;
  xipc_cqueue_t *cq = &queue->_u._header._q._sq._cqueue;
  xipc_pkt_channel_output_port_t *op = &queue->_u._header._q._uq._op;
  uint32_t shared = queue->_u._header._shared;
  uint32_t cmd_size = queue->_u._header._cmd_size;

  /* Fill up the cmd structure */
  xipc_rpc_cmd_t *rpc_cmd = (xipc_rpc_cmd_t *)((char *)cmd - 
                                               XIPC_RPC_CMD_MIN_SIZE);
  rpc_cmd->_nsid = nsid;
  /* Translate local event address to system address space */
  rpc_cmd->_event = xipc_get_sys_addr(event, xipc_get_my_pid());
  rpc_cmd->_cmd_size = cmd_size;
  rpc_cmd->_output = output;
  rpc_cmd->_output_size = output_size;

  /* Set up the packet */
  pkt._pkt_ptr = (uintptr_t)rpc_cmd;
  pkt._size = XIPC_RPC_CMD_MIN_SIZE + cmd_size;

  /* Enqueue packet */
  if (shared) {
    xipc_cqueue_send(cq, &pkt, XIPC_SLEEP_WAIT);
    err = XIPC_OK;
  } else {
    err = xipc_pkt_channel_send(op, &pkt, XIPC_SLEEP_WAIT);
  }
  if (err != XIPC_OK) {
    _Pragma("frequency_hint NEVER");
    XIPC_LOG("xipc_rpc_enqueue_cmd_buf: Error enqueueing packet, err: %d\n", 
             err);
    return XIPC_ERROR_RPC_ENQUEUE;
  }

  XIPC_LOG("Enqueuing command with event %p\n", event);

  return XIPC_OK;
}

/*
 * Enqueue a command buffer from the client to be executed synchronously
 * by the server.
 *
 * queue       : Pointer to command queue structure in shared memory
 * nsid        : Namespace id. Default handler uses 0.
 * cmd         : Pointer to the command to be enqueued
 * output      : Pointer to the output param passed to xipc_rpc_execute_cmd
 *               Optional. Can be NULL.
 * output_size : Size of the output param in bytes
 *               If output is NULL, set this to 0.
 *
 * Returns XIPC_OK if successful, else one of XIPC_ERROR_INVALID_ARG or
 * XIPC_ERROR_RPC_ENQUEUE
 */
xipc_status_t xipc_rpc_enqueue_cmd_buf_sync(xipc_rpc_queue_t *queue, 
                                            uint32_t nsid,
                                            void *cmd, 
                                            void *output,
                                            uint32_t output_size)
{
  xipc_status_t err;

  if (queue == NULL) {
    _Pragma("frequency_hint NEVER");
    XIPC_LOG("xipc_rpc_enqueue_cmd_buf: queue is null");
    return XIPC_ERROR_INVALID_ARG;
  }

  if (cmd == NULL) {
    _Pragma("frequency_hint NEVER");
    XIPC_LOG("xipc_rpc_enqueue_cmd_buf: cmd is null");
    return XIPC_ERROR_INVALID_ARG;
  }

  /* Initialize local event aligned and sized to max cache line size */
  uint32_t align = xmp_xipc_max_dcache_linesize;
  char ebuf[align*2];
  xipc_rpc_event_t *event = (void *)
                            (((uintptr_t)ebuf + (align - 1)) & ~(align - 1));
  event->_pid = xipc_get_my_pid();
  xipc_store(0, &event->_done);

  xipc_cqueue_t *cq = &queue->_u._header._q._sq._cqueue;
  xipc_pkt_channel_output_port_t *op = &queue->_u._header._q._uq._op;
  uint32_t shared = queue->_u._header._shared;
  uint32_t cmd_size = queue->_u._header._cmd_size;

  /* Fill up the cmd structure */
  xipc_rpc_cmd_t *rpc_cmd = (xipc_rpc_cmd_t *)((char *)cmd - 
                                               XIPC_RPC_CMD_MIN_SIZE);
  rpc_cmd->_nsid = nsid;
  /* Translate local event address to system address space */
  rpc_cmd->_event = xipc_get_sys_addr(event, xipc_get_my_pid());
  rpc_cmd->_cmd_size = cmd_size;
  rpc_cmd->_output = output;
  rpc_cmd->_output_size = output_size;

  /* Set up the packet */
  xipc_pkt_t pkt;
  pkt._pkt_ptr = (uintptr_t)rpc_cmd;
  pkt._size = XIPC_RPC_CMD_MIN_SIZE + cmd_size;

  /* Enqueue packet */
  if (shared) {
    xipc_cqueue_send(cq, &pkt, XIPC_SLEEP_WAIT);
    err = XIPC_OK;
  } else {
    err = xipc_pkt_channel_send(op, &pkt, XIPC_SLEEP_WAIT);
  }
  if (err != XIPC_OK) {
    _Pragma("frequency_hint NEVER");
    XIPC_LOG("xipc_rpc_enqueue_cmd_buf: Error enqueueing packet, err: %d\n", 
             err);
    return XIPC_ERROR_RPC_ENQUEUE;
  }

  XIPC_LOG("Enqueuing command with event %p\n", event);

  /* Wait for command to complete */
  return xipc_rpc_wait_event(event);
}

/*
 * Enqueue a command from the client to shutdown server
 *
 * queue : Pointer to command queue structure in shared memory
 *
 * Returns XIPC_OK if successful, else one of XIPC_ERROR_INVALID_ARG or
 * XIPC_ERROR_RPC_ENQUEUE
 */
xipc_status_t xipc_rpc_client_shutdown(xipc_rpc_queue_t *queue) 
{
  XIPC_LOG("Sending RPC shutdown command\n");

  /* dummy command */
  int cmd;
  return xipc_rpc_run_cmd_sync(queue, XIPC_RPC_SYS_CMD_SHUTDOWN, 
                               &cmd, sizeof(cmd), NULL, 0);
}

/* Checks if event is full 
 *
 * event : Pointer to event structure to query for the 
 *         completion status of the command
 *
 * Returns 0 if the event is not set, i.e the condition is not satisfied; else
 * returns 1.
 */
static int32_t
xipc_rpc_cond_event_set(void *e, int32_t val, void *thread)
{
  xipc_rpc_event_t *re = (xipc_rpc_event_t *)e;
  return xipc_load(&re->_done);
}

/*
 * Wait for event to be done
 *
 * event : Pointer to event structure to query for the 
 *         completion status of the command
 *
 * Returns XIPC_OK if successful, else XIPC_ERROR_INVALID_ARG
 */
xipc_status_t xipc_rpc_wait_event(xipc_rpc_event_t *event)
{
  if (event == NULL) {
    _Pragma("frequency_hint NEVER");
    XIPC_LOG("xipc_rpc_wait_event: event is null");
    return XIPC_ERROR_INVALID_ARG;
  }

  XIPC_LOG("Waiting on command completion event %p\n", event);

  /* Block while the event is not set */
  while (xipc_load(&event->_done) == 0) {
    uint32_t ps = xipc_disable_interrupts();
    if (xipc_load(&event->_done) == 0) {
      xipc_cond_thread_block(xipc_rpc_cond_event_set, event);
    }
    xipc_enable_interrupts(ps);
  }
  
  return XIPC_OK;
}

/*
 * Check for event to be done
 *
 * event : Pointer to event structure to query for the 
 *         completion status of the command
 *
 * Returns XIPC_OK if successful, else XIPC_ERROR_INVALID_ARG or
 * XIPC_ERROR_RPC_CMD_PENDING.
 */
xipc_status_t xipc_rpc_check_event(xipc_rpc_event_t *event)
{
  if (event == NULL) {
    _Pragma("frequency_hint NEVER");
    XIPC_LOG("xipc_rpc_wait_event: event is null");
    return XIPC_ERROR_INVALID_ARG;
  }

  XIPC_LOG("Checking on command completion event %p\n", event);
  
  if (xipc_load(&event->_done))
    return XIPC_OK;

  return XIPC_ERROR_RPC_CMD_PENDING;
}

/* Checks if any event in an event set is set.
 * Can be called from within interrupt context when the XosCond object 
 * gets signalled.
 *
 * arg    : pointer to the event structure that holds the event set
 * val    : ignored
 * thread : pointer to thread structure
 *
 * Returns 0 if none of the events are set, i.e the condition 
 * is not satisfied; else returns 1.
 */
static int32_t
xipc_cond_rpc_event_any_set(void *arg, int32_t val, void *thread)
{
  int i;
  struct xipc_cond_rpc_event_struct *eset =
                                    (struct xipc_cond_rpc_event_struct *)arg;
  for (i = 0; i < eset->_num_events; i++) {
    if (xipc_load(&eset->_events[i]->_done))
      return 1;
  }
  return 0;
}

/*
 * Wait for any event to be done
 *
 * event      : Event set to wait for
 * num_events : Number of events in the event set
 * status     : Set to XIPC_OK if successful else XIPC_ERROR_INVALID_ARG
 *
 * Returns index of earliest event that is done
 */
uint32_t xipc_rpc_wait_event_any(xipc_rpc_event_t *events[],
                                 uint32_t num_events,
                                 xipc_status_t *status)
{
  int i;

  if (!status) 
    return -1;

  for (i = 0; i < num_events; ++i) {
    if (events[i] == NULL) {
      *status = XIPC_ERROR_INVALID_ARG;
      return -1;
    }
    /* Return right away if any event is set */
    if (xipc_load(&events[i]->_done)) {
      *status = XIPC_OK;
      return i;
    }
  }

  xipc_rpc_event_t *ev = 0;
  uint32_t idx = 0;
  while (ev == 0) {
    uint32_t ps = xipc_disable_interrupts();
    for (i = 0; i < num_events; i++) {
      if (xipc_load(&events[i]->_done)) {
        idx = i;
        ev = events[i];
        break;
      }
    }
    if (ev == 0) {
      XIPC_LOG("Sleep waiting on RPC events\n");
      struct xipc_cond_rpc_event_struct s = {._events = events,
                                             ._num_events = num_events};
      xipc_cond_thread_block(xipc_cond_rpc_event_any_set, &s);
      xipc_enable_interrupts(ps);
    }
  }

  *status = XIPC_OK;
  return idx;
}

/*
 * Client side initialization of the command queue structure.
 * The command queue structure has to reside in either local memory of 
 * the server, L2-RAM, or any other uncached memory shared between the 
 * client and the server. 
 *
 * queue      : Pointer to command queue structure in shared memory
 * client_pid : Processor ID of the unique server of the command queue
 *
 * Returns      XIPC_OK if successful, else returns one of 
 *              XIPC_ERROR_INVALID_ARG or XIPC_ERROR_RPC_INIT.
 */
xipc_status_t xipc_rpc_client_queue_initialize(xipc_rpc_queue_t *queue,
                                               xipc_pid_t server_pid) 
{
  xipc_status_t err;

  if (queue == NULL) {
    _Pragma("frequency_hint NEVER");
    XIPC_LOG("xipc_rpc_client_queue_initialize: queue is null");
    return XIPC_ERROR_INVALID_ARG;
  }

  xipc_pkt_channel_output_port_t *op = &queue->_u._header._q._uq._op;

  err = xipc_pkt_channel_output_port_initialize(op, XIPC_RPC_DEFAULT_CH_ID, 
                                                server_pid);
  if (err != XIPC_OK) {
    _Pragma("frequency_hint NEVER");
    XIPC_LOG("xipc_rpc_client_queue_initialize: Error initializing output port,"
             " err : %d\n", err);
    return XIPC_ERROR_RPC_INIT;
  }

  err = xipc_pkt_channel_output_port_connect(op);
  if (err != XIPC_OK) {
    _Pragma("frequency_hint NEVER");
    XIPC_LOG("xipc_rpc_client_queue_initialize: "
             "Error connecting to server proc, err: %d\n", err);
    return XIPC_ERROR_RPC_INIT;
  }

  XIPC_LOG("Initializing RPC client with server %s\n", 
           xipc_get_proc_name(server_pid));

  return XIPC_OK;
}

/*
 * Server side dispatch the command to the default xipc_rpc_execute_command
 * handler
 *
 * queue : Pointer to command queue structure in shared memory
 *
 * Returns XIPC_OK if command execution succeeds, else one of 
 *         XIPC_ERROR_INVALID_ARG, XIPC_ERROR_RPC_CMDQ_EMPTY,
 *         XIPC_ERROR_RPC_CMD_EXEC_FAIL, or XIPC_ERROR_RPC_SHUTDOWN
 */
xipc_status_t xipc_rpc_cmd_dispatch(xipc_rpc_queue_t *queue) 
{
  xipc_pkt_t pkt;

  if (queue == NULL) {
    _Pragma("frequency_hint NEVER");
    XIPC_LOG("xipc_rpc_cmd_dispatch: queue is null");
    return XIPC_ERROR_INVALID_ARG;
  }

  xipc_cqueue_t *cq = &queue->_u._header._q._sq._cqueue;
  xipc_pkt_channel_input_port_t *ip = &queue->_u._header._q._uq._ip;
  uint32_t shared = queue->_u._header._shared;

  /* Check if command has been enqueued. Return immediately if empty */
  if (shared) {
    if (!xipc_cqueue_empty(cq)) {
      XIPC_LOG("xipc_rpc_cmd_dispatch: Command queue empty\n");
      return XIPC_ERROR_RPC_CMDQ_EMPTY;
    }
  } else {
    if (!xipc_pkt_channel_empty(ip)) {
      XIPC_LOG("xipc_rpc_cmd_dispatch: Command queue empty\n");
      return XIPC_ERROR_RPC_CMDQ_EMPTY;
    }
  }

  if (shared) {
    xipc_cqueue_recv(cq, &pkt);
  } else {
    xipc_pkt_channel_recv(ip, &pkt, XIPC_SLEEP_WAIT);
  }
  xipc_rpc_cmd_t *p = (xipc_rpc_cmd_t *)xipc_pkt_get_ptr(&pkt);

  /* Get the command handler for the namespace */
  xipc_rpc_ns_handler *handler = get_ns_handler(p->_nsid);

  XIPC_LOG("Launching RPC handler %p for nsid %d\n", handler, p->_nsid);

  /* Execute the command */
  void *cmd = (void *)p->_cmd;
  uint32_t cmd_size = p->_cmd_size;
  void *output = p->_output;
  uint32_t output_size = p->_output_size;

  /* Invoke the handler */
  xipc_status_t status = (*handler)(cmd, cmd_size, output, output_size);

  /* Flush any outstanding writes */
#pragma flush_memory

  /* Set done event */
  xipc_rpc_event_t *event = (xipc_rpc_event_t *)p->_event;
  if (event) {
    xipc_store(1, &event->_done);
    xipc_proc_notify(event->_pid);
  }

  XIPC_LOG("xipc_rpc_execute_cmd with event %p done\n", event);

  if (shared) {
    xipc_cqueue_release(cq, &pkt, XIPC_SLEEP_WAIT);
  } else {
    xipc_pkt_channel_release(ip, &pkt, XIPC_SPIN_WAIT);
  }
  return status;
}

/*
 * Server side sleep wait until a command is enqueued.
 *
 * queue : Pointer to command queue structure in shared memory
 *
 * Returns XIPC_OK if successful, else XIPC_ERROR_INVALID_ARG
 */
xipc_status_t xipc_rpc_cmd_wait(xipc_rpc_queue_t *queue) 
{
  if (queue == NULL) {
    _Pragma("frequency_hint NEVER");
    XIPC_LOG("xipc_rpc_cmd_wait: queue is null");
    return XIPC_ERROR_INVALID_ARG;
  }
  xipc_cqueue_t *cq = &queue->_u._header._q._sq._cqueue;
  xipc_pkt_channel_input_port_t *ip = &queue->_u._header._q._uq._ip;
  uint32_t shared = queue->_u._header._shared;

  XIPC_LOG("Waiting for RPC command\n");

  if (shared) {
    xipc_cqueue_wait_recv(cq);
    return XIPC_OK;
  } else {
    return xipc_pkt_channel_wait_num_recv(ip, 1, XIPC_SLEEP_WAIT);
  }
}
  
/* 
 * Server side initialization of the commmand queue structure. 
 * The command queue structure has to reside in either local memory of 
 * the server, L2-RAM, or any other uncached memory shared between the 
 * client and the server. 
 *
 * queue      : Pointer to command queue structure in shared memory
 * client_pid : Processor ID of the unique client of the command queue
 * cmd_size   : Maximum size of the command structure to be enqueued in
 *              the command queue. Should match the size used when 
 *              declaring the command queue using the macros 
 *              ef XIPC_RPC_CMD_QUEUE_SIZE or XIPC_RPC_CREATE_CMD_QUEUE.
 * num_cmds   : Maximum number of outstanding commands in the queue.
 *              Should match the number used when declaring the command queue 
 *              using the macros XIPC_RPC_CMD_QUEUE_SIZE or 
 *              XIPC_RPC_CREATE_CMD_QUEUE.
 *
 * Returns XIPC_OK if successful, else returns one of XIPC_ERROR_INTERNAL,
           XIPC_ERROR_INVALID_ARG, or XIPC_ERROR_RPC_INIT.
 */
xipc_status_t xipc_rpc_server_queue_initialize(xipc_rpc_queue_t *queue,
                                               xipc_pid_t client_pid,
                                               uint32_t cmd_size,
                                               uint32_t num_cmds) 
{
  xipc_status_t err;

  if (sizeof(xipc_rpc_cmd_t) != XIPC_RPC_CMD_MIN_SIZE) {
    _Pragma("frequency_hint NEVER");
    XIPC_LOG("xipc_rpc_server_queue_initialize: "
             "Internal error, expecting sizeof(xipc_rpc_cmd_t) to be %d bytes, "
             "but got %d\n", XIPC_RPC_CMD_MIN_SIZE, sizeof(xipc_rpc_cmd_t));
    return XIPC_ERROR_INTERNAL;
  }

  if (sizeof(xipc_rpc_queue_t) != XIPC_RPC_CMD_QUEUE_MIN_SIZE) {
    _Pragma("frequency_hint NEVER");
    XIPC_LOG("xipc_rpc_server_queue_initialize: "
             "Internal error, expecting sizeof(xipc_rpc_queue_t) to be "
             "%d bytes, but got %d\n", XIPC_RPC_CMD_QUEUE_MIN_SIZE, 
             sizeof(xipc_rpc_queue_t));
    return XIPC_ERROR_INTERNAL;
  }

  if (queue == NULL) {
    _Pragma("frequency_hint NEVER");
    XIPC_LOG("xipc_rpc_server_queue_initialize: queue is null");
    return XIPC_ERROR_INVALID_ARG;
  }

  if (cmd_size == 0) {
    _Pragma("frequency_hint NEVER");
    XIPC_LOG("xipc_rpc_server_queue_initialize: cmd_size is 0");
    return XIPC_ERROR_INVALID_ARG;
  }

  if (num_cmds == 0) {
    _Pragma("frequency_hint NEVER");
    XIPC_LOG("xipc_rpc_server_queue_initialize: num_cmds is 0");
    return XIPC_ERROR_INVALID_ARG;
  }

  /* Not a shared queue */
  queue->_u._header._shared = 0;
  queue->_u._header._cmd_size = cmd_size;
  queue->_u._header._num_cmds = num_cmds;

  xipc_pkt_channel_input_port_t *ip = &queue->_u._header._q._uq._ip;
  void *queue_buffer = &queue->_queue;

  err = xipc_pkt_channel_input_port_initialize(ip, XIPC_RPC_DEFAULT_CH_ID,
                                               client_pid);
  if (err != XIPC_OK) {
    _Pragma("frequency_hint NEVER");
    XIPC_LOG("xipc_rpc_server_queue_initialize: "
             "Error initializing input port, err: %d\n", err);
    return XIPC_ERROR_RPC_INIT;
  }

  /* Sets the address of queue */
  err = xipc_pkt_channel_input_port_set_attr(
          ip, XIPC_PKT_CHANNEL_BUFFER_ADDR_ATTR, (uint32_t)queue_buffer);
  if (err != XIPC_OK) {
    _Pragma("frequency_hint NEVER");
    XIPC_LOG("xipc_rpc_server_queue_initialize: "
             "Error setting queue buffer addr attr, err: %d\n", err);
    return XIPC_ERROR_RPC_INIT;
  }

  /* Sets the number of outstanding commands in command queue */
  err = xipc_pkt_channel_input_port_set_attr(
            ip, XIPC_PKT_CHANNEL_NUM_PACKETS_ATTR, num_cmds);
  if (err != XIPC_OK) {
    _Pragma("frequency_hint NEVER");
    XIPC_LOG("xipc_rpc_server_queue_initialize: "
             "Error setting num packets attr, err: %d\n", err);
    return XIPC_ERROR_RPC_INIT;
  }

  /* Sets the size of command. It includes the command header */
  err = xipc_pkt_channel_input_port_set_attr(
          ip, XIPC_PKT_CHANNEL_PACKET_SIZE_ATTR, 
          XIPC_RPC_CMD_MIN_SIZE+cmd_size);
  if (err != XIPC_OK) {
    _Pragma("frequency_hint NEVER");
    XIPC_LOG("xipc_rpc_server_queue_initialize: "
             "Error setting packet size attr, err: %d", err);
    return XIPC_ERROR_RPC_INIT;
  }

  /* Connect to client proc */
  err = xipc_pkt_channel_input_port_connect(ip);
  if (err != XIPC_OK) {
    _Pragma("frequency_hint NEVER");
    XIPC_LOG("xipc_rpc_server_queue_initialize: "
             "Error connecting to client proc, err: %d\n", err);
    return XIPC_ERROR_RPC_INIT;
  }

  /* Register default namespace handlers */
  add_ns_handler(XIPC_RPC_SYS_CMD_SHUTDOWN, &xipc_rpc_execute_sys_cmd_shutdown);
  add_ns_handler(0, &xipc_rpc_execute_cmd);

  XIPC_LOG("Initializing RPC server with client %s, "
           "cmd_size: %d num_cmds: %d\n", xipc_get_proc_name(client_pid),
           cmd_size, num_cmds);

  return XIPC_OK;
}

/* Helper function to return the pid with smallest id */
static XIPC_INLINE xipc_pid_t find_least_pid(xipc_pid_t *pids, 
                                             uint32_t num_pids) 
{
  xipc_pid_t lpid = 0xff;
  int i;
  for (i = 0; i < num_pids; ++i) {
    if (pids[i] < lpid)
      lpid = pids[i];
  }
  return lpid;
}
  
/*! 
 * Initialization of the shared commmand queue structure. 
 * The shared command queue structure has to reside in L2-RAM, 
 * or any other uncached memory shared among all the 
 * users of the queue. This method needs to be invoked by all processors
 * sharing the queue.
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
xipc_status_t xipc_rpc_squeue_initialize(xipc_rpc_queue_t *queue,
                                         xipc_pid_t *pids,
                                         uint32_t num_pids,
                                         uint32_t cmd_size,
                                         uint32_t num_cmds) 
{
  xipc_status_t err;
  int i;

  if (sizeof(xipc_rpc_cmd_t) != XIPC_RPC_CMD_MIN_SIZE) {
    _Pragma("frequency_hint NEVER");
    XIPC_LOG("xipc_rpc_squeue_initialize: "
             "Internal error, expecting sizeof(xipc_rpc_cmd_t) to be %d bytes, "
             "but got %d\n", XIPC_RPC_CMD_MIN_SIZE, sizeof(xipc_rpc_cmd_t));
    return XIPC_ERROR_INTERNAL;
  }

  if (sizeof(xipc_rpc_queue_t) != XIPC_RPC_CMD_QUEUE_MIN_SIZE) {
    _Pragma("frequency_hint NEVER");
    XIPC_LOG("xipc_rpc_squeue_initialize: "
             "Internal error, expecting sizeof(xipc_rpc_queue_t) to be "
             "%d bytes, but got %d\n", XIPC_RPC_CMD_QUEUE_MIN_SIZE, 
             sizeof(xipc_rpc_queue_t));
    return XIPC_ERROR_INTERNAL;
  }

  if (queue == NULL) {
    _Pragma("frequency_hint NEVER");
    XIPC_LOG("xipc_rpc_squeue_initialize: queue is null");
    return XIPC_ERROR_INVALID_ARG;
  }

  if (pids == NULL) {
    _Pragma("frequency_hint NEVER");
    XIPC_LOG("xipc_rpc_squeue_initialize: pids is null");
    return XIPC_ERROR_INVALID_ARG;
  }

  if (num_pids == 0) {
    _Pragma("frequency_hint NEVER");
    XIPC_LOG("xipc_rpc_squeue_initialize: num_pids is null");
    return XIPC_ERROR_INVALID_ARG;
  }

  if (cmd_size == 0) {
    _Pragma("frequency_hint NEVER");
    XIPC_LOG("xipc_rpc_squeue_initialize: cmd_size is 0");
    return XIPC_ERROR_INVALID_ARG;
  }

  if (num_cmds == 0) {
    _Pragma("frequency_hint NEVER");
    XIPC_LOG("xipc_rpc_squeue_initialize: num_cmds is 0");
    return XIPC_ERROR_INVALID_ARG;
  }

  /* Designate the pid with least id as the master. The master is responsible
   * for initialzing the queue */
  xipc_pid_t master_pid = find_least_pid(pids, num_pids);

  if (xipc_get_my_pid() == master_pid) {
    /* Shared queue */
    queue->_u._header._shared = 1;
    queue->_u._header._cmd_size = cmd_size;
    queue->_u._header._num_cmds = num_cmds;

    xipc_cqueue_t *cq = &queue->_u._header._q._sq._cqueue;
    void *queue_buffer = &queue->_queue;

    err = xipc_cqueue_initialize(cq, XIPC_RPC_DEFAULT_CH_ID, 
                                 XIPC_RPC_CMD_MIN_SIZE + cmd_size, num_cmds,
                                 queue_buffer, -1, pids, num_pids,
                                 pids, num_pids, 0);

    /* Send err code to other procs */
    for (i = 0; i < num_pids; ++i) {
      if (pids[i] == master_pid)
        continue;
      xipc_msg_channel_output_port_t *port = 
                                      xipc_msg_channel_get_output_port(pids[i]);
      xipc_msg_channel_send(port, &err, XIPC_SPIN_WAIT);
    }
  } else {
    /* Read err code from the master proc */
    xipc_msg_channel_input_port_t *port = 
                                   xipc_msg_channel_get_input_port(master_pid);
    xipc_msg_channel_recv(port, &err, XIPC_SPIN_WAIT);
  }

  if (err != XIPC_OK) {
    _Pragma("frequency_hint NEVER");
    XIPC_LOG("xipc_rpc_squeue_initialize: "
             "Error initializing shared queue, err: %d\n", err);
    return XIPC_ERROR_RPC_INIT;
  }

  /* Register default namespace handlers */
  add_ns_handler(XIPC_RPC_SYS_CMD_SHUTDOWN, &xipc_rpc_execute_sys_cmd_shutdown);
  add_ns_handler(0, &xipc_rpc_execute_cmd);

  XIPC_LOG("Initializing RPC shared queue, cmd_size: %d num_cmds: %d\n", 
           cmd_size, num_cmds);

  return XIPC_OK;
}
