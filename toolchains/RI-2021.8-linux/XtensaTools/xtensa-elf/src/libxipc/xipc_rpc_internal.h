#ifndef __XIPC_RPC_INTERNAL_H__
#define __XIPC_RPC_INTERNAL_H__

#include "xipc_cqueue_internal.h"
#include "xipc_pkt_channel_internal.h"
#include "xipc_rpc.h"

#define XIPC_RPC_DEFAULT_CH_ID  (0)

#define XIPC_RPC_SYS_CMD_SHUTDOWN (1)

/* Structure of RPC event */
struct xipc_rpc_event_struct {
  volatile uint32_t _done;
  xipc_pid_t        _pid;
};

/* Structure of a RPC command  */
typedef struct {
  uint32_t          _nsid;
  xipc_rpc_event_t *_event;
  uint32_t          _cmd_size;
  void             *_output;
  uint32_t          _output_size;
  char              _cmd[0];
} xipc_rpc_cmd_t;

/* Structure of a RPC queue */
struct xipc_rpc_queue_struct {
  union {
    struct {
      uint32_t _shared;
      uint32_t _cmd_size;
      uint32_t _num_cmds;
      union {
        struct {
          xipc_pkt_channel_input_port_t  _ip;
          xipc_pkt_channel_output_port_t _op;
        } _uq;
        struct {
          xipc_cqueue_t _cqueue;
        } _sq;
      } _q;
    } _header;
    char _[XIPC_RPC_CMD_QUEUE_MIN_SIZE];
  } _u;
  char _queue[0];
};

/* Set of events */
struct xipc_cond_rpc_event_struct {
  xipc_rpc_event_t **_events;
  uint32_t           _num_events;
};

/* Structure to represent list of registered namespace handlers */
typedef struct xipc_rpc_ns_handler_struct {
  uint32_t                           _nsid;
  xipc_rpc_ns_handler               *_handler;
  struct xipc_rpc_ns_handler_struct *_next;
} xipc_rpc_ns_handler_t;

#endif
