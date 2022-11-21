// Copyright (c) 2006-2018 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of
// Cadence Design Systems, Inc.  They may be adapted and modified by bona fide
// purchasers for internal use, but neither the original nor any adapted
// or modified version may be disclosed or distributed to third parties
// in any manner, medium, or form, in whole or in part, without the prior
// written consent of Cadence Design Systems, Inc.

// TODO  AXI and m_has_pif_width
// TODO  AXI and throw_invalid_pif_bus_width
// TODO  AXI and early termination of request
// TODO  AXI and cache_mx
// TODO  AXI and evict 

/*
 *                                   Theory of operation
 *
 *
 * This transactor converts Xtensa TLM transactions to OSCI TLM2 transactions.  Basically this means
 * it converts the xtsc_request_if API's to the tlm_fw_transport_if API's on the request path and it
 * converts the tlm_bw_transport_if API's to the xtsc_respond_if API's on the respond path.
 * 
 * On the request path:
 *
 * The xtsc_request_if API's are:
 *   nb_request
 *   nb_fast_access
 *   nb_peek
 *   nb_poke
 * 
 * The tlm_fw_transport_if API's are:
 *   b_transport
 *   nb_transport_fw
 *   get_direct_mem_ptr
 *   transport_dbg
 *
 * On the respond path:
 *
 * The tlm_bw_transport_if API's are:
 *   nb_transport_bw
 *   invalidate_direct_mem_ptr
 *
 * The xtsc_respond_if API's are:
 *   nb_respond
 * 
 * The use of b_transport vs nb_transport is controlled by the m_using_nb_transport flag which in turn
 * is controlled by the "use_nb_transport" and "turbo_switch" parameters.
 *
 * The implementation of the xtsc_request_if and tlm_bw_transport_if API's and associated methods is
 * discussed below.
 *
 *
 *
 * The upstream Xtensa TLM subsystem sends transactions to the downstream OSCI TLM2 subsystem by 
 * calling the nb_request method of this transactor (one per port).  What happens in the nb_request
 * method depends upon whether or not m_using_nb_transport is true and whether or not m_worker_fifo
 * is being used.
 *
 *
 *                                  m_using_nb_transport false
 *
 * When m_using_nb_transport is false (and for all RCW requests), the transaction handling sequence is:
 *     nb_request, worker_thread, do_TYPE, do_b_transport, send_response
 *
 * If m_worker_fifo is being used ("request_fifo_depth" is non-zero) and the fifo is not full then
 * the xtsc_request object is copied and stored in m_worker_fifo for the corresponding port and
 * worker_thread is notified.  If m_worker_fifo is full for that port then an RSP_NACC is sent
 * upstream.
 *
 * If m_worker_fifo is NOT being used ("request_fifo_depth" is 0) then the nb_request method accepts
 * only one request at a time (per port).  When nb_request is called and there is no current request
 * the xtsc_request object is copied to the m_request entry for the corresponding port and
 * worker_thread is notified.  All subsequent nb_request calls for that port receive RSP_NACC until
 * worker_thread finishes with the current request.
 *
 * When worker_thread wakes up (because nb_request notified it), it creates an empty xtsc_response
 * for the new xtsc_request object, and then calls one of the do_TYPE methods to actually handle the
 * request and response(s).  When the do_TYPE method returns, worker_thread either checks for another
 * request in the fifo (if m_worker_fifo is being used) or else it clears its m_busy flag and goes
 * back to sleep to wait for the next request.
 *
 * The do_TYPE methods are: 
 *       do_read
 *       do_block_read
 *       do_burst_read
 *       do_rcw
 *       do_write
 *       do_block_write
 *       do_burst_write
 *
 * The do_TYPE methods all work basically the same way.  They create an TLM2 transaction object
 * (p_trans) corresponding to the xtsc_request object (or to a part of it) and then pass p_trans to
 * the do_b_transport method for delivery to the downstream OSCI TLM2 subsystem.  The p_trans 
 * object will be passed multiple times to do_b_transport in the cases of Xtensa TLM burst and block
 * requests when m_use_tlm2_burst is false.  When the do_b_transport call returns, the do_TYPE
 * methods call send_response one or more times (as required by the request type).  After all
 * responses are sent, the do_TYPE methods return to worker_thread.
 * 
 * As its name suggests, the do_b_transport method calls b_transport using the appropriate socket
 * (based on m_width8).  The do_b_transport method is also responsible for resubmitting the
 * transaction if it is not accepted and the transactor is configured with m_use_tlm2_busy true.
 *
 * The send_response method uses nb_respond() to send the xtsc_response object to the upstream Xtensa
 * TLM subsystem.  The send_response method is responsible for re-sending the response as required
 * until it is accepted.
 *
 *
 *                                  m_using_nb_transport true
 *
 * When m_using_nb_transport is true, the typical transaction handling sequence is:
 *     nb_request, request_thread, do_nb_TYPE, do_nb_transport_fw, nb_transport_bw, respond_thread, send_response
 *
 * In nb_request, a copy of the xtsc_request is made and put into m_request_fifo and
 * m_request_thread_event is notified.
 *
 * When request_thread wakes up (or finishes the previous request), it gets the next request out of
 * m_request_fifo and calls new_transaction_info with it to create a transaction_info object used to
 * track the transaction (the exception is for non-first BLOCK_WRITE|BURST_WRITE transactions when
 * "use_tlm2_burst" is true, in which case the previously created transaction_info object is used).
 * request_thread then calls the appropriate do_nb_TYPE method depending upon the request type.
 *
 * The do_nb_TYPE methods are: 
 *       do_nb_read
 *       do_nb_block_read
 *       do_nb_burst_read
 *       do_nb_rcw              (never called, RCW requets are handled by b_transport)
 *       do_nb_write
 *       do_nb_block_write
 *       do_nb_burst_write
 *
 * The do_nb_TYPE methods do some checks and set tlm_generic_payload fields as appropriate for the
 * request type and then call do_nb_transport_fw to send the transaction to the downstream TLM2
 * subsystem (except for non-last BLOCK_WRITE|BURST_WRITE requests when "use_tlm2_burst" is true).
 *
 * The TLM2 subsystem may return TLM_COMPLETED to the nb_transport_fw() call, in which case the
 * do_nb_TYPE methods add the transaction to m_respond_thread_peq for handling by respond_thread,
 * otherwise, the transactors does nothing with the transaction until nb_transport_bw is called.
 *
 * When nb_transport_bw is called for the BEGIN_RESP phase, it adds the transaction to 
 * m_respond_thread_peq.
 *
 * When respond_thread wakes up (or finishes the previous response), it gets the next transaction out
 * of m_respond_thread_peq and calls send_response() the appropriate number of times (num_responses)
 * for the transaction.  num_responses may be 0 (for non-last BLOCK_WRITE|BURST_WRITE transactions
 * when "use_tlm2_burst" is false), or more than 1 (for BLOCK_READ|BURST_READ when "use_tlm2_burst"
 * is true), or exactly 1 for a read request with "split_tlm2_phases" set to true and for all other 
 * cases.
 *
 *
 *
 * nb_fast_access, do_get_direct_mem_ptr, invalidate_direct_mem_ptr:
 *
 * If the upstream Xtensa TLM subsystem includes an xtsc_core which wants fast access (TurboXim) to
 * the downstream OSCI TLM2 subsystem, it calls this transactor's nb_fast_access method.  Based upon
 * the "turbo_support" and "deny_fast_access" parameters, this transactor may deny fast access, 
 * allow peek/poke fast access, or call the downstream OSCI TLM2 subsystem using get_direct_mem_ptr()
 * to attempt to get a DMI pointer with which to give TurboXim raw fast access.  After granting a
 * DMI pointer, the downstream OSCI TLM2 subsystem may elect to cancel it by calling this transactor's
 * invalidate_direct_mem_ptr() method.
 *
 *
 *
 * nb_peek, nb_poke, do_transport_dbg:
 *
 * If the upstream Xtensa TLM subsystem wants to peek/poke the downstream OSCI TLM2 subsystem, it
 * calls this transactor's nb_peek or nb_poke method.  These methods both call the do_transport_dbg
 * method which is responsible for calling transport_dbg in the downstream OSCI TLM2 subsystem using
 * the appropriate socket (based on m_width8).
 *
 */

#include <cerrno>
#include <algorithm>
#include <ostream>
#include <string>
#include <xtsc/xtsc_xttlm2tlm2_transactor.h>
#include <xtsc/xtsc_tlm2.h>
#include <xtsc/xtsc_core.h>
#include <xtsc/xtsc_arbiter.h>
#include <xtsc/xtsc_master.h>
#include <xtsc/xtsc_memory_trace.h>
#include <xtsc/xtsc_router.h>

using namespace std;
using namespace sc_core;
using namespace sc_dt;
using namespace tlm;
using namespace log4xtensa;
using namespace xtsc;


static const char *exclusive_support(u32 option) {
  const char *support[3] = {
    "Throw exception",                  // 0
    "Ignore request",                   // 1
    "Use TLM2 ignorable extensions",    // 2
  };
  if (option > 2) return NULL;
  return support[option];
}



static const char *rcw_support(u32 option) {
  const char *support[4] = {
    "Use transport_dbg()",              // 0
    "Use b_transport() and throw",      // 1
    "Use b_transport() and warn",       // 2
    "Use b_transport() and info"        // 3
  };
  if (option > 3) return NULL;
  return support[option];
}



static const char *turbo_support(u32 option) {
  const char *support[4] = {
    "No load/store fast access",        // 0
    "TurboXim peek/poke access",        // 1
    "TurboXim raw access",              // 2
  };
  if (option > 3) return NULL;
  return support[option];
}



xtsc_component::xtsc_xttlm2tlm2_transactor_parms::xtsc_xttlm2tlm2_transactor_parms(const xtsc_core& core,
                                                                                   const char      *memory_interface,
                                                                                   u32              num_ports)
{
  xtsc_core::memory_port mem_port = xtsc_core::get_memory_port(memory_interface, &core); 
  if (!core.has_memory_port(mem_port)) {
    ostringstream oss;
    oss << "xtsc_xttlm2tlm2_transactor_parms: core '" << core.name() << "' has no " << memory_interface << " memory interface.";
    throw xtsc_exception(oss.str());
  }

  // Infer number of ports if num_ports is 0
  if (!num_ports) {
    u32 mpc = core.get_multi_port_count(mem_port);
    // Normally be single-ported ...
    num_ports = 1;
    // ; however, if this core interface is multi-ported (e.g. has banks or has multiple LD/ST units without CBox) AND ...
    if (mpc > 1) {
      // this is the first port (port 0) of the interface AND ...
      if (xtsc_core::is_multi_port_zero(mem_port)) {
        // if all the other ports of the interface appear to not be connected (this is not reliable--they might be delay bound), then ...
        bool found_a_connected_port = false;
        for (u32 n=1; n<mpc; ++n) {
          xtsc_core::memory_port mem_port_n = core.get_nth_multi_port(mem_port, n);
          if (core.get_request_port(mem_port_n).get_interface()) {
            found_a_connected_port = true;
            break;
          }
        }
        if (!found_a_connected_port) {
          // infer multi-ported
          num_ports = mpc;
        }
      }
    }
  }

  const xtsc_core_parms& core_parms = core.get_parms();

  u32 width8 = core.get_memory_byte_width(mem_port);

  init(memory_interface, width8, num_ports);

  set("clock_period", core_parms.get_u32("SimClockFactor")*xtsc_get_system_clock_factor());
}



xtsc_component::xtsc_xttlm2tlm2_transactor::xtsc_xttlm2tlm2_transactor(
    sc_module_name                               module_name,
    const xtsc_xttlm2tlm2_transactor_parms&      parms
    ) :
  sc_module                     (module_name),
  xtsc_module                   (*(sc_module*)this),
  m_request_exports             (NULL),
  m_respond_ports               (NULL),
  m_initiator_sockets_4         (NULL),
  m_initiator_sockets_8         (NULL),
  m_initiator_sockets_16        (NULL),
  m_initiator_sockets_32        (NULL),
  m_initiator_sockets_64        (NULL),
  m_request_impl                (NULL),
  m_tlm_bw_transport_if_impl    (NULL),
  m_nb_mm                       (*this),
  m_next_worker_port_num        (0),
  m_next_respond_port_num       (0),
  m_num_ports                   (parms.get_non_zero_u32 ("num_ports")),
  m_memory_interface_str        (parms.get_c_str        ("memory_interface")),
  m_memory_port                 (xtsc_core::MEM_PIF),
  m_width8                      (parms.get_u32          ("byte_width")),
  m_request_fifo_depth          (parms.get_u32          ("request_fifo_depth")),
  m_max_burst_beats             (parms.get_u32          ("max_burst_beats")),
  m_use_nb_transport            (parms.get_bool         ("use_nb_transport")),
  m_turbo_switch                (parms.get_bool         ("turbo_switch")),
  m_revoke_on_dmi_hint          (parms.get_bool         ("revoke_on_dmi_hint")),
  m_using_nb_transport          (m_use_nb_transport),
  m_use_tlm2_burst              (parms.get_bool         ("use_tlm2_burst")),
  m_use_tlm2_busy               (parms.get_bool         ("use_tlm2_busy")),
  m_enable_extensions           (parms.get_bool         ("enable_extensions")),
  m_exclusive_support           (parms.get_u32          ("exclusive_support")),
  m_ignore_exclusive            (parms.get_bool         ("ignore_exclusive")),
  m_immediate_timing            (parms.get_bool         ("immediate_timing")),
  m_split_tlm2_phases           (parms.get_bool         ("split_tlm2_phases")),
  m_rcw_support                 (parms.get_u32          ("rcw_support")),
  m_turbo_support               (parms.get_u32          ("turbo_support")),
  m_deny_fast_access            (parms.get_u64_vector   ("deny_fast_access")),
  m_worker_thread_event         (NULL),
  m_request_thread_event        (NULL),
  m_request_phase_ended_event   (NULL),
  m_respond_thread_peq          (NULL),
  m_request                     (NULL),
  m_worker_fifo                 (NULL),
  m_request_fifo                (NULL),
  m_busy                        (NULL),
  m_mode_switch_pending         (false),
  m_dmi_invalidated             (false),
  m_can_revoke_fast_access      (true),
  m_rcw_have_first_transfer     (NULL),
  m_first_block_write           (NULL),
  m_rcw_compare_data            (NULL),
  m_burst_data                  (NULL),
  m_byte_enables                (NULL),
  m_burst_index                 (NULL),
  m_burst_start_time            (NULL),
  m_prev_response_time          (NULL),
  m_transaction_count           (0),
  m_transaction_info_count      (0),
  m_request_count               (0),
  m_buffer_count                (0),
  m_allowed_ranges              (NULL),
  m_text                        (TextLogger::getInstance(name()))
{

  u32 n = xtsc_address_nibbles();

  if (m_split_tlm2_phases) {
    if (!m_use_nb_transport) {
      ostringstream oss;
      oss << kind() << " '" << name() << "': \"split_tlm2_phases\" cannot be true when \"use_nb_transport\" is false";
      throw xtsc_exception(oss.str());
    }
    if (!m_use_tlm2_burst) {
      ostringstream oss;
      oss << kind() << " '" << name() << "': \"use_tlm2_burst\" cannot be false when \"split_tlm2_phases\" is true";
      throw xtsc_exception(oss.str());
    }
  }
  if (m_use_nb_transport) {
    if (m_request_fifo_depth == 0) {
      ostringstream oss;
      oss << kind() << " '" << name() << "': \"request_fifo_depth\" cannot be 0 when \"use_nb_transport\" is true";
      throw xtsc_exception(oss.str());
    }
    if (m_immediate_timing) {
      ostringstream oss;
      oss << kind() << " '" << name() << "': \"immediate_timing\" cannot be true when \"use_nb_transport\" is true";
      throw xtsc_exception(oss.str());
    }
  }
  else {
    if (m_turbo_switch) {
      ostringstream oss;
      oss << kind() << " '" << name() << "': \"turbo_switch\" cannot be true when \"use_nb_transport\" is false";
      throw xtsc_exception(oss.str());
    }
  }

  if (m_revoke_on_dmi_hint && !m_turbo_switch) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': \"revoke_on_dmi_hint\" cannot be true when \"turbo_switch\" or \"use_nb_transport\" are false";
    throw xtsc_exception(oss.str());
  }

  m_has_pif_width = ((m_width8 == 4) || (m_width8 == 8) || (m_width8 == 16) || (m_width8 == 32));

  if ((m_width8 != 4) && (m_width8 != 8) && (m_width8 != 16) && (m_width8 != 32) && (m_width8 != 64)) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': Invalid \"byte_width\" parameter value (" << m_width8 << ").  Expected 4|8|16|32|64.";
    throw xtsc_exception(oss.str());
  }

  //XXXSS: m_memory_port is private and is never used.
  m_memory_port = xtsc_core::get_memory_port(m_memory_interface_str);

  if (!strcmp(m_memory_interface_str, "axi") || !strcmp(m_memory_interface_str, "idma")) { //AXI with INCR supports 256 burst length
    m_max_transfers = 256;
  } else {
    m_max_transfers = 32; // for others shouldnt it be 16?
  }
  m_max_payload_bytes = m_max_transfers * xtsc::xtsc_max_bus_width8;

  // Create all the "per-port" stuff

  m_request_exports             = new sc_export<xtsc_request_if>*       [m_num_ports];
  m_respond_ports               = new sc_port<xtsc_respond_if>*         [m_num_ports];

       if (m_width8 ==  4) { m_initiator_sockets_4  = new initiator_socket_4 *[m_num_ports]; }
  else if (m_width8 ==  8) { m_initiator_sockets_8  = new initiator_socket_8 *[m_num_ports]; }
  else if (m_width8 == 16) { m_initiator_sockets_16 = new initiator_socket_16*[m_num_ports]; }
  else if (m_width8 == 32) { m_initiator_sockets_32 = new initiator_socket_32*[m_num_ports]; }
  else if (m_width8 == 64) { m_initiator_sockets_64 = new initiator_socket_64*[m_num_ports]; }

  m_request_impl                = new xtsc_request_if_impl*             [m_num_ports];
  m_tlm_bw_transport_if_impl    = new tlm_bw_transport_if_impl*         [m_num_ports];

  m_rcw_compare_data            = new u8*                               [m_num_ports];
  m_burst_data                  = new u8*                               [m_num_ports];
  m_byte_enables                = new u8*                               [m_num_ports];
  m_allowed_ranges              = new list<address_range*>*             [m_num_ports];

  m_worker_thread_event         = new sc_event*                         [m_num_ports];
  m_request_thread_event        = new sc_event*                         [m_num_ports];
  m_request_phase_ended_event   = new sc_event*                         [m_num_ports];
  m_respond_thread_peq          = new peq*                              [m_num_ports];
  m_request                     = new xtsc_request*                     [m_num_ports];
  m_rcw_have_first_transfer     = new bool                              [m_num_ports];
  m_first_block_write           = new bool                              [m_num_ports];
  m_burst_index                 = new u32                               [m_num_ports];
  m_burst_start_time            = new sc_time                           [m_num_ports];
  m_prev_response_time          = new sc_time                           [m_num_ports];
  if (m_request_fifo_depth) {
    m_worker_fifo               = new sc_fifo<xtsc_request*>*           [m_num_ports];
    m_request_fifo              = new sc_fifo<xtsc_request*>*           [m_num_ports];
  }
  else {
    m_busy                      = new bool                              [m_num_ports];
  }

  for (u32 i=0; i<m_num_ports; ++i) {

    ostringstream oss1;
    oss1 << "m_request_exports[" << i << "]";
    m_request_exports[i] = new sc_export<xtsc_request_if>(oss1.str().c_str());

    ostringstream oss2;
    oss2 << "m_respond_ports[" << i << "]";
    m_respond_ports[i] = new sc_port<xtsc_respond_if>(oss2.str().c_str());

    ostringstream oss3;
    oss3 << "m_initiator_sockets_" << m_width8 << "[" << i << "]";
         if (m_width8 ==  4) { m_initiator_sockets_4 [i] = new initiator_socket_4 (oss3.str().c_str()); }
    else if (m_width8 ==  8) { m_initiator_sockets_8 [i] = new initiator_socket_8 (oss3.str().c_str()); }
    else if (m_width8 == 16) { m_initiator_sockets_16[i] = new initiator_socket_16(oss3.str().c_str()); }
    else if (m_width8 == 32) { m_initiator_sockets_32[i] = new initiator_socket_32(oss3.str().c_str()); }
    else if (m_width8 == 64) { m_initiator_sockets_64[i] = new initiator_socket_64(oss3.str().c_str()); }

    ostringstream oss4;
    oss4 << "m_request_impl[" << i << "]";
    m_request_impl[i] = new xtsc_request_if_impl(oss4.str().c_str(), *this, i);

    ostringstream oss5;
    oss5 << "m_tlm_bw_transport_if_impl[" << i << "]";
    m_tlm_bw_transport_if_impl[i] = new tlm_bw_transport_if_impl(oss5.str().c_str(), *this, i);

    (*m_request_exports[i])(*m_request_impl[i]);

         if (m_width8 ==  4) { (*m_initiator_sockets_4 [i])(*m_tlm_bw_transport_if_impl[i]); }
    else if (m_width8 ==  8) { (*m_initiator_sockets_8 [i])(*m_tlm_bw_transport_if_impl[i]); }
    else if (m_width8 == 16) { (*m_initiator_sockets_16[i])(*m_tlm_bw_transport_if_impl[i]); }
    else if (m_width8 == 32) { (*m_initiator_sockets_32[i])(*m_tlm_bw_transport_if_impl[i]); }
    else if (m_width8 == 64) { (*m_initiator_sockets_64[i])(*m_tlm_bw_transport_if_impl[i]); }

    m_rcw_compare_data          [i] = new u8[xtsc::xtsc_max_bus_width8];
    m_burst_data                [i] = new u8[m_max_payload_bytes];
    m_byte_enables              [i] = new u8[m_max_payload_bytes];
    m_allowed_ranges            [i] = new list<address_range*>;

    if (m_request_fifo_depth) {
      ostringstream oss1;
      oss1 << "m_worker_fifo[" << i << "]";
      m_worker_fifo[i] = new sc_fifo<xtsc_request*>(oss1.str().c_str(), m_request_fifo_depth);
      ostringstream oss2;
      oss2 << "m_request_fifo[" << i << "]";
      m_request_fifo[i] = new sc_fifo<xtsc_request*>(oss2.str().c_str(), m_request_fifo_depth);
    }
    else {
      m_busy                    [i] = false;
    }

    ostringstream oss6;
    oss6 << "worker_thread_" << i;
#if IEEE_1666_SYSTEMC >= 201101L
    m_worker_thread_event[i] = new sc_event((oss6.str() + "_event").c_str());
#else
    m_worker_thread_event[i] = new sc_event;
    xtsc_event_register(*m_worker_thread_event[i], oss6.str() + "_event", this);
#endif
    declare_thread_process(worker_thread_handle, oss6.str().c_str(), SC_CURRENT_USER_MODULE, worker_thread);
    m_process_handles.push_back(sc_get_current_process_handle());

    ostringstream oss7;
    oss7 << "request_thread_" << i;
#if IEEE_1666_SYSTEMC >= 201101L
    m_request_thread_event[i] = new sc_event((oss7.str() + "_event").c_str());
#else
    m_request_thread_event[i] = new sc_event;
    xtsc_event_register(*m_request_thread_event[i], oss7.str() + "_event", this);
#endif
    declare_thread_process(request_thread_handle, oss7.str().c_str(), SC_CURRENT_USER_MODULE, request_thread);
    m_process_handles.push_back(sc_get_current_process_handle());

    ostringstream oss8;
    oss8 << "request_phase_ended_event_" << i;
#if IEEE_1666_SYSTEMC >= 201101L
    m_request_phase_ended_event[i] = new sc_event((oss8.str()).c_str());
#else
    m_request_phase_ended_event[i] = new sc_event;
    xtsc_event_register(*m_request_phase_ended_event[i], oss8.str());
#endif

    ostringstream oss9;
    oss9 << "respond_thread_" << i;
    m_respond_thread_peq[i] = new peq((oss9.str() + "_peq").c_str());
    declare_thread_process(respond_thread_handle, oss9.str().c_str(), SC_CURRENT_USER_MODULE, respond_thread);
    m_process_handles.push_back(sc_get_current_process_handle());

  }

  if (rcw_support(m_rcw_support) == NULL) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': Invalid \"rcw_support\" parameter value (" << m_rcw_support << ").  Expected 0|1|2|3.";
    throw xtsc_exception(oss.str());
  }


  if (turbo_support(m_turbo_support) == NULL) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': Invalid \"turbo_support\" parameter value (" << m_turbo_support << ").  Expected 0|1|2.";
    throw xtsc_exception(oss.str());
  }

  if (exclusive_support(m_exclusive_support) == NULL) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': Invalid \"exclusive_support\" parameter value (" << m_exclusive_support << ").  Expected 0|1|2.";
    throw xtsc_exception(oss.str());
  }

  if (m_ignore_exclusive  && m_exclusive_support != 0) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': Both parameters \"exclusive_support\" and \"ignore_exclusive\" can't be used together.";
    oss << " \"ignore_exclusive\" is deprecated so preferably use \"exclusive_support\"";
    throw xtsc_exception(oss.str());
  }

  // Get clock period 
  u32 clock_period = parms.get_u32("clock_period");
  if (clock_period == 0xFFFFFFFF) {
    m_clock_period = xtsc_get_system_clock_period();
  }
  else {
    m_clock_period = sc_get_time_resolution() * clock_period;
  }

  if (m_turbo_switch) {
    m_using_nb_transport = !xtsc_get_xtsc_initialize_parms().get_bool("turbo");
    xtsc_register_mode_switch_interface(xtsc_switch_registration(*this, *this, "default"));
  }

  if (m_deny_fast_access.size()) {
    if (m_deny_fast_access.size() & 0x1) {
      ostringstream oss;
      oss << kind() << " '" << name() << "': " << "\"deny_fast_access\" parameter has " << m_deny_fast_access.size()
          << " elements which is not an even number as it must be.";
      throw xtsc_exception(oss.str());
    }
    for (u32 i=0; i<m_deny_fast_access.size(); i+=2) {
      xtsc_address begin = m_deny_fast_access[i];
      xtsc_address end   = m_deny_fast_access[i+1];
      if (end < begin) {
        ostringstream oss;
        oss << kind() << " '" << name() << "': " << "\"deny_fast_access\" range [0x" << hex << setfill('0') << setw(n) << begin
            << "-0x" << setw(n) << end << "] is invalid.";
        throw xtsc_exception(oss.str());
      }
    }
  }

  for (u32 i=0; i<m_num_ports; ++i) {
    ostringstream oss;
    oss.str(""); oss << "m_request_exports" << "[" << i << "]"; m_port_types[oss.str()] = REQUEST_EXPORT;
    oss.str(""); oss << "m_respond_ports"   << "[" << i << "]"; m_port_types[oss.str()] = RESPOND_PORT;
    oss.str(""); oss << "slave_port"        << "[" << i << "]"; m_port_types[oss.str()] = PORT_TABLE;
    oss.str(""); oss << "m_initiator_sockets_" << m_width8 << "[" << i << "]";
    switch (m_width8) {
      case  4:  m_port_types[oss.str()] = INITIATOR_SOCKET_4;  break;
      case  8:  m_port_types[oss.str()] = INITIATOR_SOCKET_8;  break;
      case 16:  m_port_types[oss.str()] = INITIATOR_SOCKET_16; break;
      case 32:  m_port_types[oss.str()] = INITIATOR_SOCKET_32; break;
      case 64:  m_port_types[oss.str()] = INITIATOR_SOCKET_64; break;
    }
    oss.str(""); oss << "initiator_socket[" << i << "]"; m_port_types[oss.str()] = PORT_TABLE;
  }

  m_port_types["slave_ports"      ] = PORT_TABLE;
  m_port_types["initiator_sockets"] = PORT_TABLE;

  if (m_num_ports == 1) {
    m_port_types["slave_port"      ] = PORT_TABLE;
    m_port_types["initiator_socket"] = PORT_TABLE;
  }

  xtsc_register_command(*this, *this, "change_clock_period", 1, 1,
      "change_clock_period <ClockPeriodFactor>", 
      "Call xtsc_xttlm2tlm2_transactor::change_clock_period(<ClockPeriodFactor>)."
      "  Return previous <ClockPeriodFactor> for this device."
  );

  xtsc_register_command(*this, *this, "peek", 2, 2,
      "peek <StartAddress> <NumBytes>", 
      "Peek <NumBytes> of memory starting at <StartAddress>."
  );

  xtsc_register_command(*this, *this, "poke", 2, -1,
      "poke <StartAddress> <NumBytes> <Byte1> <Byte2> . . . <ByteN>", 
      "Poke <NumBytes> (=N) of memory starting at <StartAddress>."
  );

  xtsc_register_command(*this, *this, "reset", 0, 0,
      "reset", 
      "Call xtsc_xttlm2tlm2_transactor::reset()."
  );

  LogLevel ll = xtsc_get_constructor_log_level();
  XTSC_LOG(m_text, ll,        "Constructed xtsc_xttlm2tlm2_transactor '" << name() << "':");
  XTSC_LOG(m_text, ll,        " memory_interface        = "   << m_memory_interface_str);
  XTSC_LOG(m_text, ll,        " byte_width              = "   << m_width8);
  XTSC_LOG(m_text, ll,        " num_ports               = "   << m_num_ports);
  XTSC_LOG(m_text, ll,        " request_fifo_depth      = "   << m_request_fifo_depth);
  XTSC_LOG(m_text, ll,        " max_burst_beats         = "   << m_max_burst_beats);
  XTSC_LOG(m_text, ll,        " use_nb_transport        = "   << boolalpha << m_use_nb_transport);
  XTSC_LOG(m_text, ll,        " turbo_switch            = "   << boolalpha << m_turbo_switch);
  XTSC_LOG(m_text, ll,        " revoke_on_dmi_hint      = "   << boolalpha << m_revoke_on_dmi_hint);
  XTSC_LOG(m_text, ll,        " use_tlm2_burst          = "   << boolalpha << m_use_tlm2_burst);
  XTSC_LOG(m_text, ll,        " use_tlm2_busy           = "   << boolalpha << m_use_tlm2_busy);
  XTSC_LOG(m_text, ll,        " rcw_support             = "   << m_rcw_support << " (" << rcw_support(m_rcw_support) << ")");
  XTSC_LOG(m_text, ll,        " turbo_support           = "   << m_turbo_support << " (" << turbo_support(m_turbo_support) << ")");
  XTSC_LOG(m_text, ll,        " deny_fast_access       :");
  for (u32 i=0; i<m_deny_fast_access.size(); i+=2) {
  XTSC_LOG(m_text, ll, hex << "  [0x" << setfill('0') << setw(n) << m_deny_fast_access[i] << "-0x"
                                                      << setw(n) << m_deny_fast_access[i+1] << "]");
  }
  XTSC_LOG(m_text, ll,        " enable_extensions       = "   << boolalpha << m_enable_extensions);
  XTSC_LOG(m_text, ll,        " exclusive_support       = "   << m_exclusive_support << " (" << exclusive_support(m_exclusive_support) << ")");
  XTSC_LOG(m_text, ll,        " ignore_exclusive        = "   << boolalpha << m_ignore_exclusive);
  XTSC_LOG(m_text, ll,        " immediate_timing        = "   << boolalpha << m_immediate_timing);
  XTSC_LOG(m_text, ll,        " split_tlm2_phases       = "   << boolalpha << m_split_tlm2_phases);
  if (clock_period == 0xFFFFFFFF) {
  XTSC_LOG(m_text, ll, hex << " clock_period            = 0x" << clock_period << " (" << m_clock_period << ")");
  } else {
  XTSC_LOG(m_text, ll,        " clock_period            = "   << clock_period << " (" << m_clock_period << ")");
  }

  reset();
}



xtsc_component::xtsc_xttlm2tlm2_transactor::~xtsc_xttlm2tlm2_transactor(void) {
}



void xtsc_component::xtsc_xttlm2tlm2_transactor::end_of_simulation(void) {
  LogLevel ll = log4xtensa::INFO_LOG_LEVEL;
  bool mismatch = ((m_transaction_pool     .size() != m_transaction_count      ) ||
                   (m_transaction_info_pool.size() != m_transaction_info_count ) ||
                   (m_request_pool         .size() != m_request_count          ));
  if (mismatch) {
    ll = log4xtensa::ERROR_LOG_LEVEL;
    XTSC_LOG(m_text, ll, "Error: 1 or more mismatches between pool sizes and counts which indicates a leak:");
  }
  XTSC_LOG(m_text, ll, m_transaction_pool     .size() << "/" << m_transaction_count      << " (m_transaction_pool"      "/m_transaction_count)");
  XTSC_LOG(m_text, ll, m_transaction_info_pool.size() << "/" << m_transaction_info_count << " (m_transaction_info_pool" "/m_transaction_info_count)");
  XTSC_LOG(m_text, ll, m_request_pool         .size() << "/" << m_request_count          << " (m_request_pool"          "/m_request_count)");
//XTSC_LOG(m_text, ll, m_buffer_pool          .size() << "/" << m_buffer_count           << " (m_buffer_pool"           "/m_buffer_count)");
}



u32 xtsc_component::xtsc_xttlm2tlm2_transactor::get_bit_width(const string& port_name, u32 interface_num) const {
  return m_width8*8;
}



sc_object *xtsc_component::xtsc_xttlm2tlm2_transactor::get_port(const string& port_name) {
  string name_portion;
  u32    index;
  xtsc_parse_port_name(port_name, name_portion, index);

  if ( (name_portion != "m_request_exports")                           &&
       (name_portion != "m_respond_ports")                             &&
      ((name_portion != "m_initiator_sockets_4" ) || (m_width8 !=  4)) &&
      ((name_portion != "m_initiator_sockets_8" ) || (m_width8 !=  8)) &&
      ((name_portion != "m_initiator_sockets_16") || (m_width8 != 16)) &&
      ((name_portion != "m_initiator_sockets_32") || (m_width8 != 32)) &&
      ((name_portion != "m_initiator_sockets_64") || (m_width8 != 64)))
  {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\" has no port named \"" << port_name << "\"" << endl;
    throw xtsc_exception(oss.str());
  }

  if (index > m_num_ports - 1) {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\": Invalid index: \"" << port_name << "\".  Valid range: 0-" << (m_num_ports-1) << endl;
    throw xtsc_exception(oss.str());
  }

       if (name_portion == "m_request_exports")       return m_request_exports     [index];
  else if (name_portion == "m_respond_ports")         return m_respond_ports       [index];
  else if (name_portion == "m_initiator_sockets_4" )  return m_initiator_sockets_4 [index];
  else if (name_portion == "m_initiator_sockets_8" )  return m_initiator_sockets_8 [index];
  else if (name_portion == "m_initiator_sockets_16")  return m_initiator_sockets_16[index];
  else if (name_portion == "m_initiator_sockets_32")  return m_initiator_sockets_32[index];
  else if (name_portion == "m_initiator_sockets_64")  return m_initiator_sockets_64[index];
  else {
    throw xtsc_exception("Program Bug in xtsc_xttlm2tlm2_transactor.cpp");
  }
}



xtsc_port_table xtsc_component::xtsc_xttlm2tlm2_transactor::get_port_table(const string& port_table_name) const {
  xtsc_port_table table;

  if (port_table_name == "slave_ports") {
    for (u32 i=0; i<m_num_ports; ++i) {
      ostringstream oss;
      oss << "slave_port[" << i << "]";
      table.push_back(oss.str());
    }
    return table;
  }
  
  if (port_table_name == "initiator_sockets") {
    for (u32 i=0; i<m_num_ports; ++i) {
      ostringstream oss;
      oss << "initiator_socket[" << i << "]";
      table.push_back(oss.str());
    }
    return table;
  }

  string name_portion;
  u32    index;
  bool   indexed = xtsc_parse_port_name(port_table_name, name_portion, index);

  if (((m_num_ports > 1) && !indexed) || ((name_portion != "slave_port") && (name_portion != "initiator_socket"))) {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\" has no port table named \"" << port_table_name << "\"" << endl;
    throw xtsc_exception(oss.str());
  }

  if (index > m_num_ports - 1) {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\": Invalid index: \"" << port_table_name << "\".  Valid range: 0-" << (m_num_ports-1) << endl;
    throw xtsc_exception(oss.str());
  }

  ostringstream oss;

  if (name_portion == "slave_port") {
    oss.str(""); oss << "m_request_exports" << "[" << index << "]"; table.push_back(oss.str());
    oss.str(""); oss << "m_respond_ports"   << "[" << index << "]"; table.push_back(oss.str());
  }
  else {
    oss.str(""); oss << "m_initiator_sockets_" << m_width8 << "[" << index<< "]"; table.push_back(oss.str());
  }

  return table;
}



bool xtsc_component::xtsc_xttlm2tlm2_transactor::prepare_to_switch_sim_mode(xtsc_sim_mode mode) {
  m_mode_switch_pending = true;

  if (!m_transaction_info_map.empty()) {
    XTSC_INFO(m_text, "prepare_to_switch_sim_mode returning false because nb_transport transactions are in progress");
    return false;
  }

  for (u32 i=0; i < m_num_ports; ++i) {
    if (m_worker_fifo [i]->num_free() != (int)m_request_fifo_depth) {
      XTSC_INFO(m_text, "prepare_to_switch_sim_mode returning false because m_worker_fifo[" << i << "] has transactions");
      return false;
    }
    if (m_request_fifo[i]->num_free() != (int)m_request_fifo_depth) {
      XTSC_INFO(m_text, "prepare_to_switch_sim_mode returning false because m_request_fifo[" << i << "] has transactions");
      return false;
    }
  }

  return true;
}



bool xtsc_component::xtsc_xttlm2tlm2_transactor::switch_sim_mode(xtsc_sim_mode mode) {
  m_using_nb_transport = (mode == XTSC_CYCLE_ACCURATE);
  m_mode_switch_pending = false;
  return true;
}



void xtsc_component::xtsc_xttlm2tlm2_transactor::reset(bool /* hard_reset */) {
  XTSC_INFO(m_text, kind() << "::reset()");

  m_mode_switch_pending     = false;

  m_next_worker_port_num  = 0;
  m_next_request_port_num = 0;
  m_next_respond_port_num = 0;

  for (u32 port_num=0; port_num<m_num_ports; ++port_num) {
    m_rcw_have_first_transfer[port_num] = false;
    m_first_block_write      [port_num] = true;
    m_burst_index            [port_num] = 0xFFFFFFFF;   // Detect 1st-transfer for BURST_WRITE (this special value is not used for BLOCK_WRITE)
    m_prev_response_time     [port_num] = SC_ZERO_TIME;

    if (!m_request_fifo_depth) {
      m_busy[port_num] = false;
    }

    m_worker_thread_event      [port_num]->cancel();
    m_request_thread_event     [port_num]->cancel();
    m_request_phase_ended_event[port_num]->cancel();
#if ((defined(SC_API_VERSION_STRING) && (SC_API_VERSION_STRING != sc_api_version_2_2_0)) || IEEE_1666_SYSTEMC >= 201101L)    
    m_respond_thread_peq       [port_num]->cancel_all();
#endif
  }

  // TODO: m_transaction_info_map

  xtsc_reset_processes(m_process_handles);
}



void xtsc_component::xtsc_xttlm2tlm2_transactor::change_clock_period(u32 clock_period_factor) {
  m_clock_period = sc_get_time_resolution() * clock_period_factor;
  XTSC_INFO(m_text, "change_clock_period(" << clock_period_factor << ") => " << m_clock_period);
}



void xtsc_component::xtsc_xttlm2tlm2_transactor::execute(const string&           cmd_line, 
                                                         const vector<string>&   words,
                                                         const vector<string>&   words_lc,
                                                         ostream&                result)
{
  ostringstream res;

  if (false) {
  }
  else if (words[0] == "change_clock_period") {
    u32 clock_period_factor = xtsc_command_argtou32(cmd_line, words, 1);
    res << m_clock_period.value();
    change_clock_period(clock_period_factor);
  }
  else if (words[0] == "peek") {
    u64 start_address = xtsc_command_argtou64(cmd_line, words, 1);
    u32 num_bytes     = xtsc_command_argtou32(cmd_line, words, 2);
    u8 *buffer = new u8[num_bytes];
    m_request_impl[0]->nb_peek(start_address, num_bytes, buffer);
    ostringstream oss;
    oss << setfill('0') << hex;
    for (u32 i=0; i<num_bytes; ++i) {
      oss << "0x" << setw(2) << (u32) buffer[i];
      if (i < num_bytes-1) {
        oss << " ";
      }
    }
    res << oss.str();
    delete [] buffer;
  }
  else if (words[0] == "poke") {
    u64 start_address = xtsc_command_argtou64(cmd_line, words, 1);
    u32 num_bytes     = xtsc_command_argtou32(cmd_line, words, 2);
    if (words.size() != num_bytes+3) {
      ostringstream oss;
      oss << "Command '" << cmd_line << "' specifies num_bytes=" << num_bytes << " but contains " << (words.size()-3)
          << " byte arguments.";
      throw xtsc_exception(oss.str());
    }
    if (num_bytes) {
      u8 *buffer = new u8[num_bytes];
      for (u32 i=0; i<num_bytes; ++i) {
        buffer[i] = (u8) xtsc_command_argtou32(cmd_line, words, 3+i);
      }
      m_request_impl[0]->nb_poke(start_address, num_bytes, buffer);
      delete [] buffer;
    }
  }
  else if (words[0] == "reset") {
    reset();
  }
  else {
    ostringstream oss;
    oss << name() << "::" << __FUNCTION__ << "() called for unknown command '" << cmd_line << "'.";
    throw xtsc_exception(oss.str());
  }

  result << res.str();
}



void xtsc_component::xtsc_xttlm2tlm2_transactor::connect(xtsc_arbiter& arbiter, u32 port_num) {
  if (port_num >= m_num_ports) {
    ostringstream oss;
    oss << "Invalid port_num=" << port_num << " in connect(): " << endl;
    oss << kind() << " '" << name() << "' has " << m_num_ports << " ports numbered from 0 to " << m_num_ports-1 << endl;
    throw xtsc_exception(oss.str());
  }
  arbiter.m_request_port(*m_request_exports[port_num]);
  (*m_respond_ports[port_num])(arbiter.m_respond_export);
}



xtsc::u32 xtsc_component::xtsc_xttlm2tlm2_transactor::connect(xtsc_core&    core,
                                                              const char   *memory_port_name,
                                                              u32           port_num,
                                                              bool          single_connect)
{
  if (port_num >= m_num_ports) {
    ostringstream oss;
    oss << "Invalid port_num=" << port_num << " in connect(): " << endl;
    oss << kind() << " '" << name() << "' has " << m_num_ports << " ports numbered from 0 to " << m_num_ports-1 << endl;
    throw xtsc_exception(oss.str());
  }
  xtsc_core::memory_port mem_port = xtsc_core::get_memory_port(memory_port_name, &core);
  u32 width8 = core.get_memory_byte_width(mem_port);
  if (m_width8 && (width8 != m_width8)) {
    ostringstream oss;
    oss << "Memory interface data width mismatch: " << endl;
    oss << kind() << " '" << name() << "' is " << m_width8 << " bytes wide, but '" << memory_port_name << "' interface of" << endl;
    oss << "xtsc_core '" << core.name() << "' is " << width8 << " bytes wide.";
    throw xtsc_exception(oss.str());
  }
  core.get_request_port(memory_port_name)(*m_request_exports[port_num]);
  (*m_respond_ports[port_num])(core.get_respond_export(memory_port_name));
  u32 num_connected = 1;
  // Should we connect other ports of multi-port interface?
  u32 mpc = core.get_multi_port_count(mem_port);
  if (!single_connect && (port_num+mpc <= m_num_ports) && (mpc > 1) && xtsc_core::is_multi_port_zero(mem_port)) {
    bool found_a_connected_port = false;
    for (u32 n=1; n<mpc && !found_a_connected_port; ++n) {
      xtsc_core::memory_port mem_port_n = core.get_nth_multi_port(mem_port, n);
      // Don't connect if our multi-port or the core's multi-port has already been connected.
      // The get_interface() test is not reliable so we'll catch any errors below and carry on (which
      // works for the OSCI simulator--but maybe not for others).
      if (m_request_impl[port_num+n]->is_connected() || core.get_request_port(mem_port_n).get_interface()) {
        found_a_connected_port = true;
        break;
      }
    }
    if (!found_a_connected_port) {
      for (u32 n=1; n<mpc; ++n) {
        xtsc_core::memory_port mem_port_n = core.get_nth_multi_port(mem_port, n);
        sc_port<xtsc_request_if, NSPP>& multi_port = core.get_request_port(mem_port_n);
        try {
          multi_port(*m_request_exports[port_num+n]);
          (*m_respond_ports[port_num+n])(core.get_respond_export(mem_port_n));
          num_connected += 1;
        } catch (...) {
          XTSC_INFO(m_text, "Core '" << core.name() << "' memory multi-port '" << xtsc_core::get_memory_port_name(mem_port_n) <<
                            "' is already bond.");
          if (n > 1) throw;  // Don't ignore failure after 2nd port is connected.
          break;
        }
      }
    }
  }
  return num_connected;
}



void xtsc_component::xtsc_xttlm2tlm2_transactor::connect(xtsc_master& master, u32 port_num) {
  if (port_num >= m_num_ports) {
    ostringstream oss;
    oss << "Invalid port_num=" << port_num << " in connect(): " << endl;
    oss << kind() << " '" << name() << "' has " << m_num_ports << " ports numbered from 0 to " << m_num_ports-1 << endl;
    throw xtsc_exception(oss.str());
  }
  master.m_request_port(*m_request_exports[port_num]);
  (*m_respond_ports[port_num])(master.m_respond_export);
}



u32 xtsc_component::xtsc_xttlm2tlm2_transactor::connect(xtsc_memory_trace&  memory_trace,
                                                        u32                 trace_port,
                                                        u32                 port_num,
                                                        bool                single_connect)
{
  u32 trace_ports = memory_trace.get_num_ports();
  if (trace_port >= trace_ports) {
    ostringstream oss;
    oss << "Invalid trace_port=" << trace_port << " in connect(): " << endl;
    oss << memory_trace.kind() << " '" << memory_trace.name() << "' has " << trace_ports << " ports numbered from 0 to "
        << trace_ports-1 << endl;
    throw xtsc_exception(oss.str());
  }
  if (port_num >= m_num_ports) {
    ostringstream oss;
    oss << "Invalid port_num=" << port_num << " in connect(): " << endl;
    oss << kind() << " '" << name() << "' has " << m_num_ports << " ports numbered from 0 to " << m_num_ports-1 << endl;
    throw xtsc_exception(oss.str());
  }

  u32 num_connected = 0;

  while ((trace_port < trace_ports) && (port_num < m_num_ports)) {

    (*memory_trace.m_request_ports[trace_port])(*m_request_exports[port_num]);
    (*m_respond_ports[port_num])(*memory_trace.m_respond_exports[trace_port]);

    port_num += 1;
    trace_port += 1;
    num_connected += 1;

    if (single_connect) break;
    if (trace_port >= trace_ports) break;
    if (port_num >= m_num_ports) break;
  }

  return num_connected;
}



void xtsc_component::xtsc_xttlm2tlm2_transactor::connect(xtsc_router& router, u32 router_port, u32 port_num) {
  if (port_num >= m_num_ports) {
    ostringstream oss;
    oss << "Invalid port_num=" << port_num << " in connect(): " << endl;
    oss << kind() << " '" << name() << "' has " << m_num_ports << " ports numbered from 0 to " << m_num_ports-1 << endl;
    throw xtsc_exception(oss.str());
  }
  u32 num_slaves = router.get_num_slaves();
  if (router_port >= num_slaves) {
    ostringstream oss;
    oss << "Invalid router_port=" << router_port << " in xtsc_xttlm2tlm2_transactor::connect(): " << endl;
    oss << router.kind() << " '" << router.name() << "' has " << num_slaves << " ports numbered from 0 to " << num_slaves-1 << endl;
    throw xtsc_exception(oss.str());
  }
  (*router.m_request_ports[router_port])(*m_request_exports[port_num]);
  (*m_respond_ports[port_num])(*router.m_respond_exports[router_port]);
}



void xtsc_component::xtsc_xttlm2tlm2_transactor::validate_port_and_width(u32 port_num, u32 width8) {

  if (port_num >= m_num_ports) {
    ostringstream oss;
    oss << " '" << name() << "': xtsc_xttlm2tlm2_transactor::get_initiator_socket_" << width8<< "() called with port_num=" << port_num
        << " which is outside valid range of [0-" << (m_num_ports-1) << "]";
    throw xtsc_exception(oss.str());
  }

  if (width8 != m_width8) {
    ostringstream oss;
    oss << " '" << name() << "': xtsc_xttlm2tlm2_transactor::get_initiator_socket_" << width8<< "() called but bus width is " << m_width8
        << " so you may only call get_initiator_socket_" << m_width8 << "()";
    throw xtsc_exception(oss.str());
  }

}



xtsc_component::xtsc_xttlm2tlm2_transactor::initiator_socket_4&
xtsc_component::xtsc_xttlm2tlm2_transactor::get_initiator_socket_4(u32 port_num) {
  validate_port_and_width(port_num, 4);
  return *m_initiator_sockets_4[port_num];
}



xtsc_component::xtsc_xttlm2tlm2_transactor::initiator_socket_8&
xtsc_component::xtsc_xttlm2tlm2_transactor::get_initiator_socket_8(u32 port_num) {
  validate_port_and_width(port_num, 8);
  return *m_initiator_sockets_8[port_num];
}



xtsc_component::xtsc_xttlm2tlm2_transactor::initiator_socket_16&
xtsc_component::xtsc_xttlm2tlm2_transactor::get_initiator_socket_16(u32 port_num) {
  validate_port_and_width(port_num, 16);
  return *m_initiator_sockets_16[port_num];
}



xtsc_component::xtsc_xttlm2tlm2_transactor::initiator_socket_32&
xtsc_component::xtsc_xttlm2tlm2_transactor::get_initiator_socket_32(u32 port_num) {
  validate_port_and_width(port_num, 32);
  return *m_initiator_sockets_32[port_num];
}



xtsc_component::xtsc_xttlm2tlm2_transactor::initiator_socket_64&
xtsc_component::xtsc_xttlm2tlm2_transactor::get_initiator_socket_64(u32 port_num) {
  validate_port_and_width(port_num, 64);
  return *m_initiator_sockets_64[port_num];
}



// Used by b_transport, nb_peek, nb_poke, and nb_fast_access
tlm_generic_payload *xtsc_component::xtsc_xttlm2tlm2_transactor::new_transaction() {
  if (m_transaction_pool.empty()) {
    tlm_generic_payload *p_trans = new tlm_generic_payload(this);
    m_transaction_count += 1;
    XTSC_DEBUG(m_text, "Created new tlm_generic_payload #" << m_transaction_count << ": " << p_trans);
    p_trans->acquire();
    return p_trans;
  }
  else {
    tlm_generic_payload *p_trans = m_transaction_pool.back();
    m_transaction_pool.pop_back();
    XTSC_DEBUG(m_text, "Reusing tlm_generic_payload " << p_trans);
    p_trans->acquire();
    return p_trans;
  }
}



// Used by b_transport, nb_peek, nb_poke, and nb_fast_access
void xtsc_component::xtsc_xttlm2tlm2_transactor::free(tlm_generic_payload *p_trans) {
  XTSC_DEBUG(m_text, "Recycling tlm_generic_payload " << p_trans);
  if (p_trans->get_ref_count()) {
    ostringstream oss;
    oss << name() << " - xtsc_xttlm2tlm2_transactor::free() called for a transaction with a non-zero reference count of "
        << p_trans->get_ref_count();
    throw xtsc_exception(oss.str());
  }
  p_trans->reset();
  m_transaction_pool.push_back(p_trans);
}



// Used by nb_transport
xtsc_component::xtsc_xttlm2tlm2_transactor::
transaction_info *xtsc_component::xtsc_xttlm2tlm2_transactor::new_transaction_info(xtsc_request *p_request) {
  transaction_info *p_info = NULL;
  if (m_transaction_info_pool.empty()) {
    m_transaction_info_count += 1;
    XTSC_DEBUG(m_text, "Creating new transaction_info #" << m_transaction_info_count);
    p_info = new transaction_info;
    tlm_generic_payload *p_trans = new tlm_generic_payload(&m_nb_mm);
    p_trans->set_data_ptr       (new_buffer());
    p_trans->set_byte_enable_ptr(new_buffer());
    p_info->m_p_trans = p_trans;
  }
  else {
    p_info = m_transaction_info_pool.back();
    m_transaction_info_pool.pop_back();
  }
  p_info->m_p_trans->set_response_status       (TLM_INCOMPLETE_RESPONSE);
  p_info->m_p_trans->set_dmi_allowed           (false);
  p_info->m_p_trans->acquire                   ();
  p_info->m_last_tlm2_beat                   = true;
  p_info->m_p_request                        = p_request;
  // enum tlm_phase_enum { UNINITIALIZED_PHASE=0, BEGIN_REQ=1, END_REQ, BEGIN_RESP, END_RESP };
  p_info->m_phase = BEGIN_REQ;
  p_info->m_p_response = new xtsc_response(*p_request, xtsc_response::RSP_OK, true);
  m_transaction_info_map[p_info->m_p_trans]  = p_info;
  return p_info;
}



// Used by nb_transport
xtsc::u8 *xtsc_component::xtsc_xttlm2tlm2_transactor::new_buffer() {
  u8 *p_buffer = NULL;
  if (m_buffer_pool.empty()) {
    m_buffer_count += 1;
    XTSC_DEBUG(m_text, "Creating new u8 buffer #" << m_buffer_count);
    p_buffer = new u8[m_width8*m_max_transfers];
  }
  else {
    p_buffer = m_buffer_pool.back();
    m_buffer_pool.pop_back();
  }
  return p_buffer;
}



// Used by nb_transport
void xtsc_component::xtsc_xttlm2tlm2_transactor::delete_buffer(u8*& p_buffer) {
  m_buffer_pool.push_back(p_buffer);
  p_buffer = 0;
}



xtsc_request *xtsc_component::xtsc_xttlm2tlm2_transactor::new_request(const xtsc_request& request) {
  xtsc_request *p_request = NULL;
  if (m_request_pool.empty()) {
    m_request_count += 1;
    XTSC_DEBUG(m_text, "Creating new xtsc_request #" << m_request_count);
    p_request = new xtsc_request();
  }
  else {
    p_request = m_request_pool.back();
    m_request_pool.pop_back();
  }
  *p_request = request;
  return p_request;
}



void xtsc_component::xtsc_xttlm2tlm2_transactor::delete_request(xtsc_request*& p_request) {
  m_request_pool.push_back(p_request);
  p_request = 0;
}



void xtsc_component::xtsc_xttlm2tlm2_transactor::worker_thread(void) {

  // Get the port number for this "instance" of worker_thread
  u32 port_num = m_next_worker_port_num++;

  try {

    // Handle re-entry after reset
    if (sc_time_stamp() != SC_ZERO_TIME) {
      if (m_worker_fifo) {
        wait(SC_ZERO_TIME);  // Allow one delta cycle to ensure we can completely empty m_worker_fifo (an sc_fifo)
        while (m_worker_fifo[port_num]->num_available()) {
          xtsc_request *p_request = m_worker_fifo[port_num]->read();
          delete_request(p_request);
        }
      }
    }

    while (true) {
      if (m_busy) {
        m_busy[port_num] = false;
      }
      XTSC_TRACE(m_text, "worker_thread[" << port_num << "] going to sleep.");
      wait(*m_worker_thread_event[port_num]);
      while (!m_worker_fifo || m_worker_fifo[port_num]->num_available()) {
        XTSC_TRACE(m_text, "worker_thread[" << port_num << "] woke up.");
        if (m_request_fifo_depth) {
          m_worker_fifo[port_num]->nb_read(m_request[port_num]);
        }
        xtsc_response response(*m_request[port_num], xtsc_response::RSP_OK, true);
        switch (m_request[port_num]->get_type()) {
          case xtsc_request::READ:          { do_read       (response, port_num); break; }
          case xtsc_request::BLOCK_READ:    { do_block_read (response, port_num); break; }
          case xtsc_request::BURST_READ:    { do_burst_read (response, port_num); break; }
          case xtsc_request::RCW:           { do_rcw        (response, port_num); break; }
          case xtsc_request::WRITE:         { do_write      (response, port_num); break; }
          case xtsc_request::BLOCK_WRITE:   { do_block_write(response, port_num); break; }
          case xtsc_request::BURST_WRITE:   { do_burst_write(response, port_num); break; }
          default: {
            throw xtsc_exception("Unrecognized request type in xtsc_xttlm2tlm2_transactor::worker_thread()");
          }
        }
        delete_request(m_request[port_num]);
        if (!m_worker_fifo) break;
      }
    }

  }
  catch (const sc_unwind_exception& error) { throw; }
  catch (const exception& error) {
    ostringstream oss;
    oss << "std::exception caught in worker_thread[" << port_num << "] of " << kind() << " '" << name() << "'." << endl;
    oss << "what(): " << error.what() << endl;
    xtsc_log_multiline(m_text, FATAL_LOG_LEVEL, oss.str(), 2);
    throw;
  }

}



void xtsc_component::xtsc_xttlm2tlm2_transactor::request_thread(void) {

  // Get the port number for this "instance" of request_thread
  u32 port_num = m_next_request_port_num++;

  try {

    // Handle re-entry after reset
    if (sc_time_stamp() != SC_ZERO_TIME) {
      wait(SC_ZERO_TIME);  // Allow one delta cycle to ensure we can completely empty m_request_fifo (an sc_fifo)
      while (m_request_fifo[port_num]->num_available()) {
        xtsc_request *p_request = m_request_fifo[port_num]->read();
        delete_request(p_request);
      }
    }

    transaction_info *p_info = NULL;
    while (true) {
      XTSC_TRACE(m_text, "request_thread[" << port_num << "] going to sleep.");
      wait(*m_request_thread_event[port_num]);
      while (m_request_fifo[port_num]->num_available()) {
        XTSC_TRACE(m_text, "request_thread[" << port_num << "] woke up.");
        xtsc_request *p_request = NULL;
        m_request_fifo[port_num]->nb_read(p_request);
        if (p_info == NULL) {
          p_info = new_transaction_info(p_request);
        }
        else {
          delete_request(p_info->m_p_request);
          p_info->m_p_request = p_request;
        }
        m_responses_received_map[p_request->get_tag()] = 0;
        bool aerr = false;
        switch (p_request->get_type()) {
          case xtsc_request::READ:          { aerr = do_nb_read       (p_info, port_num); break; }
          case xtsc_request::BLOCK_READ:    { aerr = do_nb_block_read (p_info, port_num); break; }
          case xtsc_request::BURST_READ:    { aerr = do_nb_burst_read (p_info, port_num); break; }
          case xtsc_request::RCW:           { aerr = do_nb_rcw        (p_info, port_num); break; }
          case xtsc_request::WRITE:         { aerr = do_nb_write      (p_info, port_num); break; }
          case xtsc_request::BLOCK_WRITE:   { aerr = do_nb_block_write(p_info, port_num); break; }
          case xtsc_request::BURST_WRITE:   { aerr = do_nb_burst_write(p_info, port_num); break; }
          default: {
            throw xtsc_exception("Unrecognized request type in xtsc_xttlm2tlm2_transactor::request_thread()");
          }
        }
        bool use_tlm2_burst = m_use_tlm2_burst && (p_request->get_burst_type() != xtsc_request::FIXED);
        bool last_transfer  = p_request->get_last_transfer();
        if (!use_tlm2_burst || last_transfer) {
          if (aerr) {
            delete_request(p_info->m_p_request);
            m_responses_received_map.erase(p_request->get_tag());
            p_info->m_p_trans->release();
          }
          p_info = NULL;
        }
      }
    }

  }
  catch (const sc_unwind_exception& error) { throw; }
  catch (const exception& error) {
    ostringstream oss;
    oss << "std::exception caught in request_thread[" << port_num << "] of " << kind() << " '" << name() << "'." << endl;
    oss << "what(): " << error.what() << endl;
    xtsc_log_multiline(m_text, FATAL_LOG_LEVEL, oss.str(), 2);
    throw;
  }

}



void xtsc_component::xtsc_xttlm2tlm2_transactor::respond_thread(void) {

  // Get the port number for this "instance" of respond_thread
  u32 port_num = m_next_respond_port_num++;

  try {

    sc_event &peq_event = m_respond_thread_peq[port_num]->get_event();

    while (true) {

      XTSC_TRACE(m_text, "respond_thread[" << port_num << "] going to sleep.");
      wait(peq_event);
      XTSC_TRACE(m_text, "respond_thread[" << port_num << "] woke up.");

      tlm_generic_payload *p_trans = NULL;
      while ((p_trans = m_respond_thread_peq[port_num]->get_next_transaction()) != NULL) {

        transaction_info_map::iterator i = m_transaction_info_map.find(p_trans);
        if (i == m_transaction_info_map.end()) {
          ostringstream oss;
          oss << name() << " xtsc_xttlm2tlm2_transactor::respond_thread[" << port_num << "] got unknown transaction: " << *p_trans;
          throw xtsc_exception(oss.str());
        }
        transaction_info *p_info = i->second;
        XTSC_DEBUG(m_text, *p_trans << " respond_thread[" << port_num << "]");

        tlm_response_status status  = p_trans->get_response_status();
        xtsc_request::burst_t burst = p_info->m_p_request->get_burst_type();
        bool okay                   = (status == TLM_OK_RESPONSE);
        bool read                   = (p_trans->get_command() == TLM_READ_COMMAND);
        bool fixed                  = (burst == xtsc_request::FIXED);
        bool wrap                   = (burst == xtsc_request::WRAP);
        u64  tag                    = p_info->m_p_request->get_tag();
        bool apb                    = p_info->m_p_response->is_apb_protocol();
        u32  num_transfers          = p_info->m_p_request->get_num_transfers();
        bool no_response            = (!read && (num_transfers > 1) && !m_use_tlm2_burst && !p_info->m_p_request->get_last_transfer());
        u32  total_responses        = ((read && m_use_tlm2_burst && !fixed) ? num_transfers : no_response ? 0 : 1);
        u32  size8                  = 0;
        u32  total_size8            = 0;
        u32  offset                 = 0;

        if (!okay && !no_response) {
          if (status == TLM_ADDRESS_ERROR_RESPONSE) {
            p_info->m_p_response->set_status(apb ? xtsc_response::SLVERR : xtsc_response::RSP_ADDRESS_ERROR);
            total_responses = 1;
          }
          else if (status == TLM_GENERIC_ERROR_RESPONSE) {
            p_info->m_p_response->set_status(apb ? xtsc_response::SLVERR : xtsc_response::RSP_ADDRESS_DATA_ERROR);
          }
          else {
            ostringstream oss;
            oss << name() << " Port #" << port_num << " (in xtsc_xttlm2tlm2_transactor::response_thread): nb_transport() failed:  " << *p_trans;
            throw xtsc_exception(oss.str());
          }
        }
        else if (read) {
          size8 = p_info->m_p_request->get_byte_size();
          if (total_responses > 1) {
            total_size8                  =  size8 * total_responses;
            bool can_wrap                = (p_info->m_p_request->get_type() == xtsc_request::BLOCK_READ) || wrap;
            u64 address8                 =  p_info->m_p_request->get_byte_address();
            xtsc_address upper_wrap_addr = ((address8 / total_size8) * total_size8) + total_size8;  
            if (m_split_tlm2_phases) {
              if (can_wrap) {
                offset  = ((address8 + m_responses_received_map[tag] * size8) >= upper_wrap_addr ) 
                                         ? ((address8 + m_responses_received_map[tag] * size8) - upper_wrap_addr )
                                         : (m_responses_received_map[tag] * size8) % total_size8 ;
              }
              else {
                offset  = m_responses_received_map[tag] * size8;
              }
            }
            else if (can_wrap) {
              offset  = (address8 + m_responses_received_map[tag] * size8) % total_size8;
            }
            else {
              offset  =  0;
            }
          }
        }

        exclusive_request *ext1 = nullptr;
        transaction_id    *ext2 = nullptr;
        if (m_exclusive_support == 2) {
          p_trans->get_extension(ext1);
          p_trans->get_extension(ext2);
        }
        u32 num_responses = (m_split_tlm2_phases && read ? 1 : total_responses); //send read responses 1 by 1 if m_split_tlm2_phases enabled
        for (u32 i=1; i<=num_responses; ++i) {
          if (read && okay) {
            memcpy(p_info->m_p_response->get_buffer(), p_trans->get_data_ptr() + offset, size8);
            XTSC_TRACE(m_text, "respond_thread[" << port_num << "] : offset : " << offset << 
                               ", transfer number : " << m_responses_received_map[tag]+1);
            bool last_transfer = ((m_use_tlm2_burst && !fixed) ? (m_responses_received_map[tag] == total_responses-1) : p_info->m_last_tlm2_beat);
            p_info->m_p_response->set_transfer_number(m_responses_received_map[tag]+1);
            p_info->m_p_response->set_last_transfer  (last_transfer);
            if (num_responses > 1) {
              offset = ((offset + size8) % total_size8);
            }
          }
          //Check for exclusive response status
          if (okay) {
            //Check if the transaction has request extensions
            if (ext1 && ext2) {
              exclusive_response *ext = nullptr;
              p_trans->get_extension(ext); // Retrieve the extension 
              if (ext) {
                XTSC_DEBUG(m_text, "respond_thread: Exclusive response received");
                p_info->m_p_response->set_exclusive_ok(true);
                // Release extension as slave doesnt have the opportunity to do in a nb_transport_fw, 
                // which is not called with END_RESP
                p_trans->release_extension<exclusive_response>();
              }
            }
          }
          send_response(*p_info->m_p_response, port_num);
          m_responses_received_map[tag]++;

          if (m_split_tlm2_phases && p_info->m_phase == BEGIN_RDATA) {
            p_info->m_phase = END_RDATA;
            tlm_sync_enum result = do_nb_transport_fw(*p_trans, p_info->m_phase, port_num);
            if (result == TLM_COMPLETED) {
              ostringstream oss;
              oss << name() << " Target completed transaction with some transfers pending for " << *p_trans;
              throw xtsc_exception(oss.str());
            }
          }
        }
        if (!read || (num_transfers <= 1) || p_info->m_p_response->get_last_transfer()) {
          delete_request(p_info->m_p_request);
          m_responses_received_map.erase(tag);
        }
        if (p_info->m_phase == BEGIN_RESP) {
          if (ext1 && ext2) {
            XTSC_TRACE(m_text, "respond_thread: Release request extensions");
            p_info->m_p_trans->release_extension<exclusive_request>();
            p_info->m_p_trans->release_extension<transaction_id>();
          }
          remove_extensions(p_info->m_p_trans, ext1 && ext2);
          p_info->m_p_trans->release();
        }
      }
    }

  }
  catch (const sc_unwind_exception& error) { throw; }
  catch (const exception& error) {
    ostringstream oss;
    oss << "std::exception caught in respond_thread[" << port_num << "] of " << kind() << " '" << name() << "'." << endl;
    oss << "what(): " << error.what() << endl;
    xtsc_log_multiline(m_text, FATAL_LOG_LEVEL, oss.str(), 2);
    throw;
  }

}



void xtsc_component::xtsc_xttlm2tlm2_transactor::send_response(xtsc_response& response, u32 port_num) {
  sc_time now(sc_time_stamp());
  if ((m_prev_response_time[port_num] + m_clock_period > now) && (m_prev_response_time[port_num] != SC_ZERO_TIME)) {
    sc_time delay(m_prev_response_time[port_num] + m_clock_period - now);
    XTSC_DEBUG(m_text, "Delaying " << delay << " to separate responses by 1 clock period.  Port #" << port_num);
    wait(delay);
  }
  XTSC_INFO(m_text, response << " Port #" << port_num);
  // Need to check return value because master might be busy
  while (!(*m_respond_ports[port_num])->nb_respond(response)) {
    XTSC_VERBOSE(m_text, "nb_respond() returned false; waiting one clock period to try again.  Port #" << port_num);
    wait(m_clock_period);
  }
  m_prev_response_time[port_num] = sc_time_stamp();
}



void xtsc_component::xtsc_xttlm2tlm2_transactor::throw_invalid_pif_bus_width(u32 port_num) {
  ostringstream oss;
  oss << "'" << name() << "' Port #" << port_num << " received PIF-only transaction " << m_request[port_num]->get_type_name()
      << " but this " << kind() << " was configured with \"byte_width\" of " << m_width8 << ", which is not a legal PIF bus width";
  throw xtsc_exception(oss.str());
}



void xtsc_component::xtsc_xttlm2tlm2_transactor::do_transport_dbg(tlm_generic_payload& trans, u32 port_num) {
  u32 count = 0;

       if (m_width8 ==  4) { count = (*m_initiator_sockets_4 [port_num])->transport_dbg(trans); }
  else if (m_width8 ==  8) { count = (*m_initiator_sockets_8 [port_num])->transport_dbg(trans); }
  else if (m_width8 == 16) { count = (*m_initiator_sockets_16[port_num])->transport_dbg(trans); }
  else if (m_width8 == 32) { count = (*m_initiator_sockets_32[port_num])->transport_dbg(trans); }
  else if (m_width8 == 64) { count = (*m_initiator_sockets_64[port_num])->transport_dbg(trans); }
  else { throw xtsc_exception("Program Bug in xtsc_xttlm2tlm2_transactor.cpp - unsupported bus width"); }

  if (count != trans.get_data_length()) {
    ostringstream oss;
    oss << name() << " Port #" << port_num << " (in xtsc_xttlm2tlm2_transactor::do_transport_dbg): transport_dbg() failed: " << trans
        << " count=" << count << " (possibly downstream model doesn't support data length of " << trans.get_data_length() << ")";
    throw xtsc_exception(oss.str());
  }
}



bool xtsc_component::xtsc_xttlm2tlm2_transactor::do_get_direct_mem_ptr(tlm_generic_payload& trans, tlm_dmi &dmi_data, u32 port_num) {
  bool allow = false;

       if (m_width8 ==  4) { allow = (*m_initiator_sockets_4 [port_num])->get_direct_mem_ptr(trans, dmi_data); }
  else if (m_width8 ==  8) { allow = (*m_initiator_sockets_8 [port_num])->get_direct_mem_ptr(trans, dmi_data); }
  else if (m_width8 == 16) { allow = (*m_initiator_sockets_16[port_num])->get_direct_mem_ptr(trans, dmi_data); }
  else if (m_width8 == 32) { allow = (*m_initiator_sockets_32[port_num])->get_direct_mem_ptr(trans, dmi_data); }
  else if (m_width8 == 64) { allow = (*m_initiator_sockets_64[port_num])->get_direct_mem_ptr(trans, dmi_data); }
  else { throw xtsc_exception("Program Bug in xtsc_xttlm2tlm2_transactor.cpp - unsupported bus width"); }

  return allow;
}



tlm_sync_enum xtsc_component::xtsc_xttlm2tlm2_transactor::do_nb_transport_fw(tlm_generic_payload& trans, tlm_phase& phase, u32 port_num) {
  // enum tlm_sync_enum { TLM_ACCEPTED, TLM_UPDATED, TLM_COMPLETED };
  tlm_sync_enum result = TLM_ACCEPTED;
  sc_time delay(SC_ZERO_TIME);

  XTSC_INFO(m_text, trans << " " << xtsc_tlm_phase_text(phase) << " (" << delay << ")" << " do_nb_transport_fw[" << port_num << "] ");
       if (m_width8 ==  4) { result = (*m_initiator_sockets_4 [port_num])->nb_transport_fw(trans, phase, delay); }
  else if (m_width8 ==  8) { result = (*m_initiator_sockets_8 [port_num])->nb_transport_fw(trans, phase, delay); }
  else if (m_width8 == 16) { result = (*m_initiator_sockets_16[port_num])->nb_transport_fw(trans, phase, delay); }
  else if (m_width8 == 32) { result = (*m_initiator_sockets_32[port_num])->nb_transport_fw(trans, phase, delay); }
  else if (m_width8 == 64) { result = (*m_initiator_sockets_64[port_num])->nb_transport_fw(trans, phase, delay); }
  else { throw xtsc_exception("Program Bug in xtsc_xttlm2tlm2_transactor.cpp - unsupported bus width"); }

  XTSC_VERBOSE(m_text, trans << " " << xtsc_tlm_phase_text(phase) << " (" << delay << ")" << " do_nb_transport_fw[" << port_num << "] " <<
                    xtsc_tlm_sync_enum_text(result));

  if ((result == TLM_ACCEPTED) && 
       (phase == BEGIN_REQ) || (m_split_tlm2_phases && (phase == BEGIN_WDATA || phase == BEGIN_WDATA_LAST))) {
    XTSC_VERBOSE(m_text, "xtsc_xttlm2tlm2_transactor::do_nb_transport_fw() begin wait(m_request_phase_ended_event[" << port_num << "])");
    wait(*m_request_phase_ended_event[port_num]);
    XTSC_VERBOSE(m_text, "xtsc_xttlm2tlm2_transactor::do_nb_transport_fw() end   wait(m_request_phase_ended_event[" << port_num << "])");
  }
  else if (delay != SC_ZERO_TIME) {
    XTSC_VERBOSE(m_text, "xtsc_xttlm2tlm2_transactor::do_nb_transport_fw() begin wait(" << delay << ")");
    wait(delay);
  }

  return result;
}



void xtsc_component::xtsc_xttlm2tlm2_transactor::do_b_transport(xtsc_response&          response,
                                                                tlm_generic_payload&    trans,
                                                                sc_time&                delay,
                                                                u32                     port_num)
{
  tlm_response_status status = TLM_INCOMPLETE_RESPONSE;

  do {
         if (m_width8 ==  4) { (*m_initiator_sockets_4 [port_num])->b_transport(trans, delay); }
    else if (m_width8 ==  8) { (*m_initiator_sockets_8 [port_num])->b_transport(trans, delay); }
    else if (m_width8 == 16) { (*m_initiator_sockets_16[port_num])->b_transport(trans, delay); }
    else if (m_width8 == 32) { (*m_initiator_sockets_32[port_num])->b_transport(trans, delay); }
    else if (m_width8 == 64) { (*m_initiator_sockets_64[port_num])->b_transport(trans, delay); }
    else { throw xtsc_exception("Program Bug in xtsc_xttlm2tlm2_transactor.cpp - unsupported bus width"); }
    status = trans.get_response_status();
    if (m_use_tlm2_busy && (status == TLM_INCOMPLETE_RESPONSE)) {
      wait(m_clock_period);
    }
  } while (m_use_tlm2_busy && (status == TLM_INCOMPLETE_RESPONSE));

  if (status != TLM_OK_RESPONSE) {
    bool apb = response.is_apb_protocol();
    if (status == TLM_ADDRESS_ERROR_RESPONSE) {
      response.set_status(apb ? xtsc_response::SLVERR : xtsc_response::RSP_ADDRESS_ERROR);
    }
    else if (status == TLM_GENERIC_ERROR_RESPONSE) {
      response.set_status(apb ? xtsc_response::SLVERR : xtsc_response::RSP_ADDRESS_DATA_ERROR);
    }
    else {
      ostringstream oss;
      oss << name() << " Port #" << port_num << " (in xtsc_xttlm2tlm2_transactor::do_b_transport): b_transport() failed:  " << trans;
      throw xtsc_exception(oss.str());
    }
  }
  //Check for exclusive response status
  if (status == TLM_OK_RESPONSE && m_exclusive_support == 2) {
    exclusive_request  *ext1 = nullptr;
    transaction_id     *ext2 = nullptr;
    trans.get_extension(ext1);  
    trans.get_extension(ext2);  
    if (ext1 && ext2) {
      exclusive_response* ext = nullptr;
      trans.get_extension(ext); // Retrieve the extension 
      if (ext) {
        XTSC_DEBUG(m_text, "Exclusive response received");
        response.set_exclusive_ok(true);
        // Release extension as slave doesnt have the opportunity to do in a b_transport
        trans.release_extension<exclusive_response>();
      }
    }
  }
  if (m_revoke_on_dmi_hint && trans.is_dmi_allowed() && m_can_revoke_fast_access) {
    uint64 start_range = trans.get_address();
    uint64 end_range   = start_range + trans.get_data_length() - 1;
    m_tlm_bw_transport_if_impl[port_num]->invalidate_direct_mem_ptr(start_range, end_range);
  }
  XTSC_VERBOSE(m_text, trans);
}



bool xtsc_component::xtsc_xttlm2tlm2_transactor::check_and_set_exclusive(xtsc_request *p_request, 
                                                                         tlm_generic_payload *trans) {
  if (p_request->get_exclusive()) {
    if (m_exclusive_support == 0 && !m_ignore_exclusive) {
      ostringstream oss;
      oss << kind() << " '" << name() << "': Received unsupported exclusive request: " << *p_request << " (See parameter \"exclusive_support\")";
      throw xtsc_exception(oss.str());

    }
    else if ((m_exclusive_support == 1 && !m_ignore_exclusive) ||
             (m_exclusive_support == 0 &&  m_ignore_exclusive)) { //Either one is set
      XTSC_DEBUG(m_text, "Ignoring exclusive access");
      return false;

    }
    else if (m_exclusive_support == 2 && !m_ignore_exclusive) { //Set extensions
      u64 addr      = trans->get_address();
      u32 len       = trans->get_data_length();
      u32 num_xfers = (len + m_width8 - 1) / m_width8;

      if (num_xfers > 16) {
        ostringstream oss;
        oss << kind() << " '" << name() << "': ";
        oss << "Exclusive request burst length/number of transfers=" << num_xfers << " cannot exceed 16.";
        throw xtsc_exception(oss.str());
      }

      if (len !=  1 && len !=  2 && len !=  4 && len !=   8 && 
          len != 16 && len != 32 && len != 64 && len != 128) {
        ostringstream oss;
        oss << kind() << " '" << name() << "': ";
        oss << "Invalid Exclusive request bytes to transfer=" << len << "Valid values are : 1|2|4|8|16|32|64|128.";
        throw xtsc_exception(oss.str());
      }

      if (addr % len) {
        ostringstream oss;
        oss << kind() << " '" << name() << "': ";
        oss << "Exclusive TLM2 request, Address=0x" << std::hex << addr << "unaligned to the data transfer length=0x" << len;
        throw xtsc_exception(oss.str());
      }

      exclusive_request *ext1 = new exclusive_request;
      XTSC_DEBUG(m_text, "Setting extension : exclusive_request");
      ext1->value = true;
      trans->set_extension( ext1 );

      transaction_id *ext2 = new transaction_id;
      XTSC_DEBUG(m_text, "Setting extension : transaction_id : " << p_request->get_transaction_id());
      ext2->value = (unsigned int)p_request->get_transaction_id();
      trans->set_extension( ext2 );     

      return true;
    }
  }
  return false;
}


void xtsc_component::xtsc_xttlm2tlm2_transactor::add_extensions(xtsc_request *p_request,
                                                                tlm_generic_payload *p_trans) {
  if (m_enable_extensions) {
    // Add supported extensions here
    transaction_id *ext = new transaction_id;
    XTSC_DEBUG(m_text, "Setting extension : transaction_id : " << p_request->get_transaction_id());
    ext->value = (unsigned int)p_request->get_transaction_id();
    p_trans->set_extension( ext );
  }
}


void xtsc_component::xtsc_xttlm2tlm2_transactor::remove_extensions(tlm_generic_payload *p_trans, 
                                                                   bool excl) {
  if (m_enable_extensions) {
    // Release supported ignorable extensions 
    transaction_id *ext = nullptr;
    p_trans->get_extension(ext);
    if (!ext) {
      XTSC_WARN(m_text, "Extension 'transaction_id' not found! Possibly removed by downstream TLM2 model.");
    } else {
      p_trans->release_extension<transaction_id>();
    }
  }

  if (excl) {
    // Release exclusive extensions 
    p_trans->release_extension<exclusive_request>();
    if (!m_enable_extensions) {
      p_trans->release_extension<transaction_id>();
    }
  }
}


// do_TYPE
void xtsc_component::xtsc_xttlm2tlm2_transactor::do_read(xtsc_response& response, u32 port_num) {
  u64  address8         = m_request[port_num]->get_byte_address();
  u32  size8            = m_request[port_num]->get_byte_size();
  XTSC_VERBOSE(m_text, "do_read() address=0x" << hex << address8 << " size=" << dec << size8 << " Port #" << port_num);
  if (((size8 !=  1) &&
       (size8 !=  2) &&
       (size8 !=  4) &&
       (size8 !=  8) &&
       (size8 != 16) &&
       (size8 != 32) &&
       (size8 != 64))            ||     // not a power of 2   .OR.
      (size8 > m_width8)         ||     // exceeds bus width  .OR.
      ((address8 % size8) != 0))        // not size8-aligned
  {
    bool apb = response.is_apb_protocol();
    XTSC_INFO(m_text, "do_read() returning " << (apb ? "SLVERR" : "RSP_ADDRESS_ERROR") << ": address=0x" << hex << address8 <<
                         " size=" << dec << size8 << " m_width8=" << m_width8 << " Port #" << port_num);
    response.set_status(apb ? xtsc_response::SLVERR : xtsc_response::RSP_ADDRESS_ERROR);
  }
  else {
    tlm_generic_payload *p_trans = new_transaction();
    p_trans->set_command                (TLM_READ_COMMAND);
    p_trans->set_address                (address8);
    p_trans->set_data_length            (size8);
    p_trans->set_streaming_width        (size8);
    p_trans->set_response_status        (TLM_INCOMPLETE_RESPONSE);
    p_trans->set_data_ptr               (response.get_buffer());
    p_trans->set_byte_enable_ptr        (m_byte_enables[port_num]);
    p_trans->set_byte_enable_length     (size8);
    p_trans->set_dmi_allowed            (false);
    xtsc_byte_enables be = m_request[port_num]->get_byte_enables();
    for (u32 i=0; i<size8; ++i) {
      m_byte_enables[port_num][i] = ((be & 1) ? 0xff : 0x00);
      be >>= 1;
    }

    bool excl = check_and_set_exclusive(m_request[port_num],p_trans);

    add_extensions(m_request[port_num],p_trans);

    sc_time delay(SC_ZERO_TIME);

    do_b_transport(response, *p_trans, delay, port_num);

    if (!m_immediate_timing && delay != SC_ZERO_TIME) {
      wait(delay);
    }

    remove_extensions(p_trans, excl);

    p_trans->release();
  }

  send_response(response, port_num);
}



// do_TYPE
void xtsc_component::xtsc_xttlm2tlm2_transactor::do_block_read(xtsc_response& response, u32 port_num) {
  if (!m_has_pif_width) throw_invalid_pif_bus_width(port_num);
  u64  address8         = m_request[port_num]->get_byte_address();
  u32  size8            = m_request[port_num]->get_byte_size();
  u32  num_transfers    = m_request[port_num]->get_num_transfers();
  u32  block_size       = size8 * num_transfers;                // CWF
  u64  base_address8    = address8 & ~(u64)(block_size - 1);    // CWF
  u64  wrap_address8    = base_address8 + block_size;           // CWF
  XTSC_VERBOSE(m_text, "do_block_read() address=0x" << hex << address8 << " num_transfers=" << dec << num_transfers <<
                       " Port #" << port_num);
  if ((size8 != m_width8) || ((address8 % m_width8) != 0)) {
    XTSC_INFO(m_text, "do_block_read() returning RSP_ADDRESS_ERROR: address=0x" << hex << address8 <<
                         " size=" << dec << size8 << " m_width8=" << m_width8 << " Port #" << port_num);
    response.set_status(xtsc_response::RSP_ADDRESS_ERROR);
    send_response(response, port_num);
    return;
  }
  response.set_last_transfer(false);

  tlm_generic_payload *p_trans = new_transaction();

  bool excl     = false;
  u32  size     = size8;
  u8  *p_buffer = response.get_buffer();
  sc_time total_delay(SC_ZERO_TIME);
  sc_time start_time(sc_time_stamp());

  for (u32 i=1; i <= num_transfers; ++i) {
    if ((i==1) || !m_use_tlm2_burst) {
      if (m_use_tlm2_burst) {
        size = m_width8 * num_transfers;
        p_buffer = m_burst_data[port_num];
      }

      p_trans->set_command                (TLM_READ_COMMAND);
      p_trans->set_address                (m_use_tlm2_burst ? base_address8 : address8);  // CWF
      p_trans->set_data_length            (size);
      p_trans->set_streaming_width        (size);
      p_trans->set_response_status        (TLM_INCOMPLETE_RESPONSE);
      p_trans->set_data_ptr               (p_buffer);
      p_trans->set_byte_enable_ptr        (0);
//    p_trans->set_byte_enable_length     (size);
      p_trans->set_dmi_allowed            (false);

      if (m_use_tlm2_burst) {
        excl = check_and_set_exclusive(m_request[port_num],p_trans);
      }

      add_extensions(m_request[port_num],p_trans);

      sc_time delay(SC_ZERO_TIME);

      do_b_transport(response, *p_trans, delay, port_num);

      total_delay += delay;
    }

    if (m_use_tlm2_burst) {
      memcpy(response.get_buffer(), &m_burst_data[port_num][address8 - base_address8], m_width8);
    }

    sc_time now(sc_time_stamp());
    if (!m_immediate_timing && ((now - start_time) < (m_clock_period * i))) {
      wait(start_time + m_clock_period * i - now);
    }

    if (i == num_transfers) {
      response.set_last_transfer(true);
      sc_time now(sc_time_stamp());
      if (!m_immediate_timing && ((now - start_time) < total_delay)) {
        wait(start_time + total_delay - now);
      }
    }

    xtsc_response::status_t status = response.get_status();
    if ((status == xtsc_response::RSP_ADDRESS_ERROR) || (status == xtsc_response::RSP_ADDRESS_DATA_ERROR)) {
      response.set_last_transfer(true);
    }

    response.set_transfer_number(i);

    send_response(response, port_num);
    address8 += size8;
    // For Critical Word First (CWF)
    if (address8 >= wrap_address8) {
      address8 = base_address8;
    }
    if ((status == xtsc_response::RSP_ADDRESS_ERROR) || (status == xtsc_response::RSP_ADDRESS_DATA_ERROR)) break;
  }

  remove_extensions(p_trans, excl);

  p_trans->release();
}



// do_TYPE
void xtsc_component::xtsc_xttlm2tlm2_transactor::do_burst_read(xtsc_response& response, u32 port_num) {
  if (!m_has_pif_width && !response.is_axi_protocol()) throw_invalid_pif_bus_width(port_num);
  u64  address8         = m_request[port_num]->get_byte_address();
  u32  size8            = m_request[port_num]->get_byte_size();
  u32  num_transfers    = m_request[port_num]->get_actual_transfers();
  u32  total_size8      = size8 * num_transfers; 
  u64  base_address8    = address8;
  bool axi              = m_request[port_num]->is_axi_protocol();
//bool cache_mx         = false;  // TODO: AXI and cache_mx
  bool fixed            = false;
  bool wrap             = false;
  u64  lower_wrap_addr  = 0;
  u64  upper_wrap_addr  = 0;
  if (axi) {
    xtsc_request::burst_t burst = m_request[port_num]->get_burst_type();
//  cache_mx        = m_request[port_num]->is_cache_maintenance();  // TODO: AXI and cache_mx
    fixed           = (burst == xtsc_request::FIXED);
    wrap            = (burst == xtsc_request::WRAP);
    lower_wrap_addr = (address8 / total_size8) * total_size8;
    upper_wrap_addr = lower_wrap_addr + total_size8;
    if (wrap) {
      base_address8 = lower_wrap_addr;
    }
  }
  bool use_tlm2_burst   = m_use_tlm2_burst && !fixed;
  XTSC_VERBOSE(m_text, "do_burst_read() address=0x" << hex << address8 << " num_transfers=" << dec << num_transfers <<
                       " Port #" << port_num);
  if (!axi && (size8 != m_width8)) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': ";
    oss << "Received BURST_READ tag=" << m_request[port_num]->get_tag() << " with byte size of " << size8
        << " (contradicts \"byte_width\" of " << m_width8 << ")";
    throw xtsc_exception(oss.str());
  }
  if (!axi && ((address8 % m_width8) != 0)) {
    XTSC_INFO(m_text, "do_burst_read() returning RSP_ADDRESS_ERROR: address=0x" << hex << address8 <<
                         " size=" << dec << size8 << " m_width8=" << m_width8 << " Port #" << port_num);
    response.set_status(xtsc_response::RSP_ADDRESS_ERROR);
    send_response(response, port_num);
    return;
  }
  if (axi && (size8 > m_width8)) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': ";
    oss << "Received BURST_READ tag=" << m_request[port_num]->get_tag() << " with byte size of " << size8
        << " (contradicts \"byte_width\" of " << m_width8 << ")";
    throw xtsc_exception(oss.str());
  }
  response.set_last_transfer(num_transfers == 1);

  tlm_generic_payload *p_trans = new_transaction();

  bool excl     = false;
  u32  size     = size8;
  u8  *p_buffer = response.get_buffer();
  sc_time total_delay(SC_ZERO_TIME);
  sc_time start_time(sc_time_stamp());

  for (u32 transfer_count=1; transfer_count <= num_transfers; ++transfer_count) {
    u64 addr8 = address8;
    if ((transfer_count==1) || !use_tlm2_burst) {
      if (use_tlm2_burst) {
        addr8    = base_address8;
        size     = size8 * num_transfers;
        p_buffer = m_burst_data[port_num];
      }

      p_trans->set_command                (TLM_READ_COMMAND);
      p_trans->set_address                (addr8);
      p_trans->set_data_length            (size);
      p_trans->set_streaming_width        (size);
      p_trans->set_response_status        (TLM_INCOMPLETE_RESPONSE);
      p_trans->set_data_ptr               (p_buffer);
      p_trans->set_byte_enable_ptr        (0);
//    p_trans->set_byte_enable_length     (size);
      p_trans->set_dmi_allowed            (false);

      sc_time delay(SC_ZERO_TIME);

      if (use_tlm2_burst) {
         excl = check_and_set_exclusive(m_request[port_num],p_trans);
      }

      add_extensions(m_request[port_num],p_trans);

      do_b_transport(response, *p_trans, delay, port_num);

      total_delay += delay;
    }

    if (use_tlm2_burst) {
      memcpy(response.get_buffer(), &m_burst_data[port_num][address8 - base_address8], size8);
    }

    sc_time now(sc_time_stamp());
    if (!m_immediate_timing && ((now - start_time) < (m_clock_period * transfer_count))) {
      wait(start_time + m_clock_period * transfer_count - now);
    }

    if (transfer_count == num_transfers) {
      response.set_last_transfer(true);
      sc_time now(sc_time_stamp());
      if (!m_immediate_timing && ((now - start_time) < total_delay)) {
        wait(start_time + total_delay - now);
      }
    }

    xtsc_response::status_t status = response.get_status();
    if (!axi && ((status == xtsc_response::RSP_ADDRESS_ERROR) || (status == xtsc_response::RSP_ADDRESS_DATA_ERROR))) {
      response.set_last_transfer(true);
    }

    response.set_transfer_number(transfer_count);

    send_response(response, port_num);
    // Adjust address for next transfer.
    if (!fixed) {
      address8 += size8;
      if (wrap) {
        if (address8 >= upper_wrap_addr ) {
          address8 -= total_size8;
        }
      }
    }
    if (!axi && ((status == xtsc_response::RSP_ADDRESS_ERROR) || (status == xtsc_response::RSP_ADDRESS_DATA_ERROR))) break;
  }

  remove_extensions(p_trans, excl);
  
  p_trans->release();
}



// do_TYPE
void xtsc_component::xtsc_xttlm2tlm2_transactor::do_rcw(xtsc_response& response, u32 port_num) {
  if (!m_has_pif_width) throw_invalid_pif_bus_width(port_num);
  u64  address8         = m_request[port_num]->get_byte_address();
  u32  size8            = m_request[port_num]->get_byte_size();
  bool last_transfer    = m_request[port_num]->get_last_transfer();
  XTSC_VERBOSE(m_text, "do_rcw() address=0x" << hex << address8 << " size=" << dec << size8);
  if (((size8 !=  1) &&
       (size8 !=  2) &&
       (size8 !=  4) &&
       (size8 !=  8) &&
       (size8 != 16))            ||     // not a power of 2   .OR.
      (size8 > m_width8)         ||     // exceeds bus width  .OR.
      ((address8 % size8) != 0))        // not size8-aligned
  {
    XTSC_INFO(m_text, "do_rcw() returning RSP_ADDRESS_ERROR: address=0x" << hex << address8 <<
                         " size=" << dec << size8 << " m_width8=" << m_width8 << " Port #" << port_num);
    response.set_status(xtsc_response::RSP_ADDRESS_ERROR);
  }
  else if (!last_transfer) {
    if (m_rcw_have_first_transfer[port_num]) {
      ostringstream oss;
      oss << name() << " Port #" << port_num
          << ": xtsc_xttlm2tlm2_transactor::do_rcw() received two non-last-transfer RCW's";
      throw xtsc_exception(oss.str());
    }
    m_rcw_have_first_transfer[port_num] = true;
    memcpy(m_rcw_compare_data[port_num], m_request[port_num]->get_buffer(), size8);
    m_burst_start_time[port_num] = sc_time_stamp();
    return;
  }
  else {
    if (!m_rcw_have_first_transfer[port_num]) {
      ostringstream oss;
      oss << name() << " Port #" << port_num
          << ": xtsc_xttlm2tlm2_transactor::do_rcw() received a last-transfer RCW without receiving a non-last-transfer RCW";
      throw xtsc_exception(oss.str());
    }
    //RCW may not need to check for exclusive?
    tlm_generic_payload *p_trans = new_transaction();
    p_trans->set_command                (TLM_READ_COMMAND);
    p_trans->set_address                (address8);
    p_trans->set_data_length            (size8);
    p_trans->set_streaming_width        (size8);
    p_trans->set_response_status        (TLM_INCOMPLETE_RESPONSE);
    p_trans->set_data_ptr               (response.get_buffer());
    p_trans->set_byte_enable_ptr        (m_byte_enables[port_num]);
    p_trans->set_byte_enable_length     (size8);
    p_trans->set_dmi_allowed            (false);
    xtsc_byte_enables be = m_request[port_num]->get_byte_enables();
    for (u32 i=0; i<size8; ++i) {
      // Note: This restriction could be removed by doing multiple single byte peek/pokes
      if (!(be & 1) && !m_rcw_support) {
        ostringstream oss;
        oss << kind() << " Port #" << port_num
            << ": RCW with byte lanes disabled is not allowed when using transport_dbg (\"rcw_support\"=0): " << *m_request[port_num];
        throw xtsc_exception(oss.str());
      }
      m_byte_enables[port_num][i] = ((be & 1) ? 0xff : 0x00);
      be >>= 1;
    }

    sc_time start_time(sc_time_stamp());
    u64     start_delta(sc_delta_count());
    sc_time delay(SC_ZERO_TIME);

    if (m_rcw_support == 0) {
      do_transport_dbg(*p_trans, port_num);
    }
    else {
      do_b_transport(response, *p_trans, delay, port_num);
    }

    u8 *p_data = response.get_buffer();
    bool compare_data_matches = true;
    for (u32 i = 0; i<size8; i++) {
      if (m_byte_enables[port_num][i] && (m_rcw_compare_data[port_num][i] != p_data[i])) {
        compare_data_matches = false;
        break;
      }
    }
    if (compare_data_matches) {
      p_trans->set_command              (TLM_WRITE_COMMAND);
      p_trans->set_response_status      (TLM_INCOMPLETE_RESPONSE);
      p_trans->set_data_ptr             (m_request[port_num]->get_buffer());

      if (m_rcw_support == 0) {
        do_transport_dbg(*p_trans, port_num);
      }
      else {
        do_b_transport(response, *p_trans, delay, port_num);
      }

      if (start_delta != sc_delta_count()) {
        ostringstream oss;
        oss << kind() << " '" << name() << "' " << " Port #" << port_num << " detected none atomic RCW: read occurred at "
            << start_time << " (delta cycle " << start_delta << "), write occurred at " << sc_time_stamp() << " (delta cycle "
            << sc_delta_count() << ").  Parameter \"rcw_support\"=" << m_rcw_support;
        if (m_rcw_support == 2) {
          XTSC_WARN(m_text, oss.str());
        }
        else if (m_rcw_support == 3) {
          XTSC_INFO(m_text, oss.str());
        }
        else {
          throw xtsc_exception(oss.str());
        }
      }
    }

    if (!m_immediate_timing && (delay != SC_ZERO_TIME)) {
      sc_time now(sc_time_stamp());
      if (m_burst_start_time[port_num] + delay > now) {
        XTSC_DEBUG(m_text, "delaying " << ((m_burst_start_time[port_num] + delay) - now) << " in do_rcw()  Port #" << port_num);
        wait((m_burst_start_time[port_num] + delay) - now);
      }
    }

    p_trans->release();
    m_rcw_have_first_transfer[port_num] = false;
  }
  send_response(response, port_num);
}



// do_TYPE
void xtsc_component::xtsc_xttlm2tlm2_transactor::do_write(xtsc_response& response, u32 port_num) {
  u64  address8         = m_request[port_num]->get_byte_address();
  u32  size8            = m_request[port_num]->get_byte_size();
  XTSC_VERBOSE(m_text, "do_write() address=0x" << hex << address8 << " size8=" << dec << size8 << " Port #" << port_num);
  if (((size8 !=  1) &&
       (size8 !=  2) &&
       (size8 !=  4) &&
       (size8 !=  8) &&
       (size8 != 16) &&
       (size8 != 32) &&
       (size8 != 64))            ||     // not a power of 2   .OR.
      (size8 > m_width8)         ||     // exceeds bus width  .OR.
      ((address8 % size8) != 0))        // not size8-aligned
  {
    bool apb = response.is_apb_protocol();
    XTSC_INFO(m_text, "do_write() returning " << (apb ? "SLVERR" : "RSP_ADDRESS_ERROR") << ": address=0x" << hex << address8 <<
                         " size=" << dec << size8 << " m_width8=" << m_width8 << " Port #" << port_num);
    response.set_status(apb ? xtsc_response::SLVERR : xtsc_response::RSP_ADDRESS_ERROR);
  }
  else {
    tlm_generic_payload *p_trans = new_transaction();
    p_trans->set_command                (TLM_WRITE_COMMAND);
    p_trans->set_address                (address8);
    p_trans->set_data_length            (size8);
    p_trans->set_streaming_width        (size8);
    p_trans->set_response_status        (TLM_INCOMPLETE_RESPONSE);
    p_trans->set_data_ptr               (m_request[port_num]->get_buffer());
    p_trans->set_byte_enable_ptr        (m_byte_enables[port_num]);
    p_trans->set_byte_enable_length     (size8);
    p_trans->set_dmi_allowed            (false);
    xtsc_byte_enables be = m_request[port_num]->get_byte_enables();
    for (u32 i=0; i<size8; ++i) {
      m_byte_enables[port_num][i] = ((be & 1) ? 0xff : 0x00);
      be >>= 1;
    }

    bool excl = check_and_set_exclusive(m_request[port_num],p_trans);

    add_extensions(m_request[port_num],p_trans);

    sc_time delay(SC_ZERO_TIME);

    do_b_transport(response, *p_trans, delay, port_num);

    if (!m_immediate_timing && (delay != SC_ZERO_TIME)) {
      wait(delay);
    }

    remove_extensions(p_trans, excl);

    p_trans->release();
  }

  send_response(response, port_num);
}



// do_TYPE
void xtsc_component::xtsc_xttlm2tlm2_transactor::do_block_write(xtsc_response& response, u32 port_num) {
  if (!m_has_pif_width) throw_invalid_pif_bus_width(port_num);
  u64  address8         = m_request[port_num]->get_byte_address();
  u32  size8            = m_request[port_num]->get_byte_size();
  bool last_transfer    = m_request[port_num]->get_last_transfer();
  bool respond          = last_transfer;
  u32  num_transfers    = m_request[port_num]->get_num_transfers();
  XTSC_VERBOSE(m_text, "do_block_write() address=0x" << hex << address8 << " size8=" << dec << size8 << " Port #" << port_num);
  if (m_use_tlm2_burst) {
    m_burst_index[port_num] = (address8 % (m_width8 * num_transfers)) / m_width8;
  }
  // size must equal bus width and address must be bus width aligned
  if ((size8 != m_width8) || ((address8 % m_width8) != 0)) {
    response.set_status(xtsc_response::RSP_ADDRESS_ERROR);
  }
  else if (m_use_tlm2_burst && !last_transfer) {
    if (m_first_block_write[port_num]) {
      m_burst_start_time[port_num] = sc_time_stamp();
      m_first_block_write[port_num] = false;
    }
    memcpy(&m_burst_data[port_num][m_width8*m_burst_index[port_num]], m_request[port_num]->get_buffer(), m_width8);
    xtsc_byte_enables be = m_request[port_num]->get_byte_enables();
    for (u32 i=0; i<m_width8; ++i) {
      m_byte_enables[port_num][m_width8*m_burst_index[port_num] + i] = ((be & 1) ? 0xff : 0x00);
      be >>= 1;
    }
  }
  else {
    u8 *p_buffer = m_request[port_num]->get_buffer();
    u8 *p_byte_enables = m_byte_enables[port_num];
    bool excl = false;
    if (m_use_tlm2_burst) {
      memcpy(&m_burst_data[port_num][m_width8*m_burst_index[port_num]], p_buffer, m_width8);
      p_byte_enables            = &m_byte_enables[port_num][m_width8*m_burst_index[port_num]];
      size8                     = m_width8*num_transfers;
      p_buffer                  = m_burst_data[port_num];
      xtsc_address offset       = address8 % size8;
      address8                 -= offset;
      m_burst_index[port_num]   = 0xFFFFFFFF;
      m_first_block_write[port_num] = true;
    }
    tlm_generic_payload *p_trans = new_transaction();
    p_trans->set_command                (TLM_WRITE_COMMAND);
    p_trans->set_address                (address8);
    p_trans->set_data_length            (size8);
    p_trans->set_streaming_width        (size8);
    p_trans->set_response_status        (TLM_INCOMPLETE_RESPONSE);
    p_trans->set_data_ptr               (p_buffer);
    p_trans->set_byte_enable_ptr        (m_byte_enables[port_num]);
    p_trans->set_byte_enable_length     (size8);
    p_trans->set_dmi_allowed            (false);
    xtsc_byte_enables be = m_request[port_num]->get_byte_enables();
    for (u32 i=0; i<m_width8; ++i) {
      p_byte_enables[i] = ((be & 1) ? 0xff : 0x00);
      be >>= 1;
    }

    if (m_use_tlm2_burst) {
      excl = check_and_set_exclusive(m_request[port_num],p_trans);
    }

    add_extensions(m_request[port_num],p_trans);

    sc_time delay(SC_ZERO_TIME);

    do_b_transport(response, *p_trans, delay, port_num);

    if (!m_immediate_timing && (delay != SC_ZERO_TIME)) {
      if (m_use_tlm2_burst) {
        sc_time now(sc_time_stamp());
        if (m_burst_start_time[port_num] + delay > now) {
          XTSC_DEBUG(m_text, "delaying " << ((m_burst_start_time[port_num] + delay) - now) << " in do_block_write() Port #" << port_num);
          wait((m_burst_start_time[port_num] + delay) - now);
        }
      }
      else {
        wait(delay);
      }
    }

    remove_extensions(p_trans, excl);
    p_trans->release();
  }
  if (respond) {
    send_response(response, port_num);
  }
}



// do_TYPE
void xtsc_component::xtsc_xttlm2tlm2_transactor::do_burst_write(xtsc_response& response, u32 port_num) {
  if (!m_has_pif_width && !response.is_axi_protocol()) throw_invalid_pif_bus_width(port_num);
  u32  n                = xtsc_address_nibbles();
  u64  address8         = m_request[port_num]->get_byte_address();
  u32  size8            = m_request[port_num]->get_byte_size();
  u32  bel              = size8;
  u32  num_transfers    = m_request[port_num]->get_actual_transfers();
  u32  total_size8      = size8 * num_transfers; 
  bool last_transfer    = m_request[port_num]->get_last_transfer();
//bool evict            = m_request[port_num]->is_evict();   // TODO AXI and evict
  bool axi              = m_request[port_num]->is_axi_protocol();
  bool fixed            = false;
  bool wrap             = false;
  bool respond          = last_transfer;
  if (axi) {
    xtsc_request::burst_t burst = m_request[port_num]->get_burst_type();
    fixed = (burst == xtsc_request::FIXED);
    wrap  = (burst == xtsc_request::WRAP);
  }
  bool use_tlm2_burst   = m_use_tlm2_burst && !fixed && (total_size8 <= m_max_payload_bytes);
  XTSC_VERBOSE(m_text, "do_burst_write() address=0x" << hex << address8 << " size8=" << dec << size8 << " Port #" << port_num);

  if ((size8 != 1) && (size8 != 2) && (size8 != 4) && (size8 != 8) && (size8 != 16) && (size8 != 32) && (size8 != 64)) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': ";
    oss << "Received BURST_WRITE tag=" << m_request[port_num]->get_tag() << " with byte size of " << size8 << " (which is not a power of 2)";
    throw xtsc_exception(oss.str());
  }
  if (!axi && (size8 != m_width8)) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': ";
    oss << "Received BURST_WRITE tag=" << m_request[port_num]->get_tag() << " with byte size of " << size8
        << " (contradicts \"byte_width\" of " << m_width8 << ")";
    throw xtsc_exception(oss.str());
  }
  if (!axi && ((address8 % m_width8) != 0)) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': ";
    oss << "Received BURST_WRITE tag=" << m_request[port_num]->get_tag() << " with byte address of 0x" << hex << setw(n) << address8
        << " (which is not bus width (" << dec << m_width8 << ") aligned";
    throw xtsc_exception(oss.str());
  }
  if (axi && (size8 > m_width8)) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': ";
    oss << "Received BURST_WRITE tag=" << m_request[port_num]->get_tag() << " with byte size of " << size8 << " (exceeds \"byte_width\" of "
        << m_width8 << ")";
    throw xtsc_exception(oss.str());
  }

  if (use_tlm2_burst && (m_burst_index[port_num] == 0xFFFFFFFF)) {
    // First transfer
    m_burst_start_time[port_num] = sc_time_stamp();
    m_burst_index[port_num] = 0;
    if (wrap) {
      m_burst_index[port_num] = (((address8 | (size8-1)) % total_size8) / size8);
    }
  }
  if (use_tlm2_burst && !last_transfer) {
    // If PIF allows  8 =>  7 non-last =>  6 at this point in code
    // If PIF allows 16 => 15 non-last => 14 at this point in code
    if (!axi && (m_burst_index[port_num] > (m_max_burst_beats-2))) {
      ostringstream oss;
      oss << kind() << " '" << name() << "' Port #" << port_num << " received too many non-last BURST_WRITE transfers in a row: "
          << *m_request[port_num] << endl;
      oss << "\"max_burst_beats\"=" << m_max_burst_beats;
      throw xtsc_exception(oss.str());
    }
    memcpy(&m_burst_data[port_num][size8*m_burst_index[port_num]], m_request[port_num]->get_buffer(), size8);
    xtsc_byte_enables be = m_request[port_num]->get_byte_enables();
    for (u32 i=0; i<size8; ++i) {
      m_byte_enables[port_num][size8*m_burst_index[port_num] + i] = ((be & 1) ? 0xff : 0x00);
      be >>= 1;
    }
    m_burst_index[port_num] += 1;
    if (wrap && (m_burst_index[port_num] >= num_transfers)) {
      m_burst_index[port_num] = 0;
    }
  }
  else {
    u8 *p_buffer = m_request[port_num]->get_buffer();
    u8 *p_byte_enables = m_byte_enables[port_num];
    u32 size = size8;
    bool excl = false;
    if (use_tlm2_burst) {
      memcpy(&m_burst_data[port_num][size8*m_burst_index[port_num]], p_buffer, size8);
      address8                 -= size8*m_burst_index[port_num];
      p_byte_enables            = &m_byte_enables[port_num][size8*m_burst_index[port_num]];
      size                      = total_size8;
      p_buffer                  = m_burst_data[port_num];
      m_burst_index[port_num]   = 0xFFFFFFFF;
      bel                       = total_size8;
    }
    tlm_generic_payload *p_trans = new_transaction();
    p_trans->set_command                (TLM_WRITE_COMMAND);
    p_trans->set_address                (address8);
    p_trans->set_data_length            (size);
    p_trans->set_streaming_width        (size);
    p_trans->set_response_status        (TLM_INCOMPLETE_RESPONSE);
    p_trans->set_data_ptr               (p_buffer);
    p_trans->set_byte_enable_ptr        (m_byte_enables[port_num]);
    p_trans->set_byte_enable_length     (bel);
    p_trans->set_dmi_allowed            (false);
    xtsc_byte_enables be = m_request[port_num]->get_byte_enables();
    for (u32 i=0; i<size8; ++i) {
      p_byte_enables[i] = ((be & 1) ? 0xff : 0x00);
      be >>= 1;
    }

    if (use_tlm2_burst) {
      bool excl = check_and_set_exclusive(m_request[port_num],p_trans);
    }

    add_extensions(m_request[port_num],p_trans);

    sc_time delay(SC_ZERO_TIME);

    do_b_transport(response, *p_trans, delay, port_num);

    if (!m_immediate_timing && (delay != SC_ZERO_TIME)) {
      if (use_tlm2_burst) {
        sc_time now(sc_time_stamp());
        if (m_burst_start_time[port_num] + delay > now) {
          XTSC_DEBUG(m_text, "delaying " << ((m_burst_start_time[port_num] + delay) - now) << " in do_burst_write()" <<
                             " Port #" << port_num);
          wait((m_burst_start_time[port_num] + delay) - now);
        }
      }
      else {
        wait(delay);
      }
    }
    
    remove_extensions(p_trans, excl);
    p_trans->release();
  }
  if (respond) {
    send_response(response, port_num);
  }
}



// do_nb_TYPE
bool xtsc_component::xtsc_xttlm2tlm2_transactor::do_nb_read(transaction_info *p_info, u32 port_num) {
  u64  address8         = p_info->m_p_request->get_byte_address();
  u32  size8            = p_info->m_p_request->get_byte_size();
  XTSC_VERBOSE(m_text, "do_nb_read() address=0x" << hex << address8 << " size=" << dec << size8 << " Port #" << port_num);
  if (((size8 !=  1) &&
       (size8 !=  2) &&
       (size8 !=  4) &&
       (size8 !=  8) &&
       (size8 != 16) &&
       (size8 != 32) &&
       (size8 != 64))            ||     // not a power of 2   .OR.
      (size8 > m_width8)         ||     // exceeds bus width  .OR.
      ((address8 % size8) != 0))        // not size8-aligned
  {
    XTSC_INFO(m_text, "do_nb_read() returning RSP_ADDRESS_ERROR: address=0x" << hex << address8 <<
                         " size=" << dec << size8 << " m_width8=" << m_width8 << " Port #" << port_num);
    p_info->m_p_response->set_status(xtsc_response::RSP_ADDRESS_ERROR);
    send_response(*p_info->m_p_response, port_num);
    return true;
  }
  else {
    tlm_generic_payload *p_trans = p_info->m_p_trans;
    p_trans->set_command                (TLM_READ_COMMAND);
    p_trans->set_address                (address8);
    p_trans->set_data_length            (size8);
    p_trans->set_streaming_width        (size8);
    p_trans->set_byte_enable_length     (size8);
    xtsc_byte_enables be = p_info->m_p_request->get_byte_enables();
    u8 *p_byte_enables = p_trans->get_byte_enable_ptr();
    for (u32 i=0; i<size8; ++i) {
      p_byte_enables[i] = ((be & 1) ? 0xff : 0x00);
      be >>= 1;
    }
    bool excl = check_and_set_exclusive(p_info->m_p_request,p_trans);

    add_extensions(p_info->m_p_request,p_trans);

    tlm_sync_enum result = do_nb_transport_fw(*p_trans, p_info->m_phase, port_num);
    if (result == TLM_COMPLETED) {
      if (!m_split_tlm2_phases) {
        m_respond_thread_peq[port_num]->notify(*p_trans);
      } else {
        ostringstream oss;
        oss << name() << " Target completed transaction with response pending for " << *p_trans;
        throw xtsc_exception(oss.str());
      }      
    }
  }
  return false;
}



// do_nb_TYPE
bool xtsc_component::xtsc_xttlm2tlm2_transactor::do_nb_block_read(transaction_info *p_info, u32 port_num) {
  if (!m_has_pif_width) throw_invalid_pif_bus_width(port_num);
  u64  address8         = p_info->m_p_request->get_byte_address();
  u32  size8            = p_info->m_p_request->get_byte_size();
  u32  num_transfers    = p_info->m_p_request->get_num_transfers();
  u32  block_size       = size8 * num_transfers;                // CWF
  u32  total_size8      = m_width8 * num_transfers;
  u64  base_address8    = address8 & ~(u64)(block_size - 1);    // CWF
  u64  lower_wrap_addr  = (address8 / total_size8) * total_size8;
  u64  upper_wrap_addr  = lower_wrap_addr + total_size8;
  
  XTSC_VERBOSE(m_text, "do_nb_block_read() address=0x" << hex << address8 << " num_transfers=" << dec << num_transfers <<
                       " Port #" << port_num);
  if ((size8 != m_width8) || ((address8 % m_width8) != 0)) {
    XTSC_INFO(m_text, "do_nb_block_read() returning RSP_ADDRESS_ERROR: address=0x" << hex << address8 <<
                         " size=" << dec << size8 << " m_width8=" << m_width8 << " Port #" << port_num);
    p_info->m_p_response->set_status(xtsc_response::RSP_ADDRESS_ERROR);
    send_response(*p_info->m_p_response, port_num);
    return true;
  }
  p_info->m_p_response->set_last_transfer(false);

  tlm_generic_payload *p_trans = p_info->m_p_trans;

  u32  size        = m_use_tlm2_burst ? total_size8 : size8;
  // tlm2_beats is > 1, if !use_tlm2_burst or 
  // for BLOCK_READ CWF transaction, when split_tlm2_phase is enabled
  u32  tlm2_beats  = (m_split_tlm2_phases && (base_address8 != address8)) ? 2 : (m_use_tlm2_burst ? 1 : num_transfers); 

  for (u32 transfer_count=1; transfer_count <= tlm2_beats; ++transfer_count) {
    u64 addr8 = address8;
    if ((transfer_count==1) || !m_use_tlm2_burst || m_split_tlm2_phases) {
      if (m_split_tlm2_phases) {
        size     = transfer_count==1 ? (upper_wrap_addr - addr8) : (total_size8 - size);
      }
      else if (m_use_tlm2_burst) {
        addr8    = base_address8;
        size     = total_size8;
      }
    }
    p_trans->set_command                (TLM_READ_COMMAND);
    p_trans->set_address                (addr8);  
    p_trans->set_data_length            (size);
    p_trans->set_streaming_width        (size);
    p_trans->set_byte_enable_length     (size);
    memset(p_trans->get_byte_enable_ptr(), 0xff, size);
    p_info->m_last_tlm2_beat = (transfer_count == tlm2_beats);
    if (m_use_tlm2_burst) {
      bool excl = check_and_set_exclusive(p_info->m_p_request,p_trans);
    }
    add_extensions(p_info->m_p_request,p_trans);
    tlm_sync_enum result = do_nb_transport_fw(*p_trans, p_info->m_phase, port_num);
    if (result == TLM_COMPLETED) {
      if (!m_split_tlm2_phases) {
        m_respond_thread_peq[port_num]->notify(*p_trans);
      } else {
        ostringstream oss;
        oss << name() << " Target completed transaction with response pending for " << *p_trans;
        throw xtsc_exception(oss.str());
      }        
    }
    if (transfer_count < tlm2_beats) {
      xtsc_request *p_request = p_info->m_p_request;
      // Create new transaction_info for another tlm2_beat with same request
      p_info = new_transaction_info(p_request);
      p_trans = p_info->m_p_trans;
      address8 += size;
      if (address8 >= base_address8 + total_size8) {
        address8 -= total_size8;
      }
    }
  }
  return false;
}



// do_nb_TYPE
bool xtsc_component::xtsc_xttlm2tlm2_transactor::do_nb_burst_read(transaction_info *p_info, u32 port_num) {
  if (!m_has_pif_width) throw_invalid_pif_bus_width(port_num);
  u64  address8         = p_info->m_p_request->get_byte_address();
  u32  size8            = p_info->m_p_request->get_byte_size();
  u32  num_transfers    = p_info->m_p_request->get_actual_transfers();
  u32  total_size8      = size8 * num_transfers; 
  u64  base_address8    = address8;
  bool axi              = p_info->m_p_request->is_axi_protocol();
//bool cache_mx         = false;  // TODO: AXI and cache_mx
  bool fixed            = false;
  bool wrap             = false;
  u64  lower_wrap_addr  = 0;
  u64  upper_wrap_addr  = 0;

  if (axi) {
    xtsc_request::burst_t burst = p_info->m_p_request->get_burst_type();
//  cache_mx                    = p_info->m_p_request->is_cache_maintenance();  // TODO: AXI and cache_mx
    fixed                       = (burst == xtsc_request::FIXED);
    wrap                        = (burst == xtsc_request::WRAP);
    lower_wrap_addr             = (address8 / total_size8) * total_size8;
    upper_wrap_addr             = lower_wrap_addr + total_size8;
    if (wrap) {
      base_address8             = lower_wrap_addr;
    }
  }

  bool use_tlm2_burst   = m_use_tlm2_burst && !fixed;
  XTSC_VERBOSE(m_text, "do_nb_burst_read() address=0x" << hex << address8 << " num_transfers=" << dec << num_transfers <<
                       " Port #" << port_num);
  if (use_tlm2_burst && (num_transfers > m_max_transfers)) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': ";
    oss << "Received BURST_READ tag=" << p_info->m_p_request->get_tag() << " with get_actual_transfers() of " << num_transfers
        << " (exceeds m_max_transfers of " << m_max_transfers << ")";
    throw xtsc_exception(oss.str());
  }
  if (!axi && (size8 != m_width8)) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': ";
    oss << "Received BURST_READ tag=" << p_info->m_p_request->get_tag() << " with byte size of " << size8
        << " (contradicts \"byte_width\" of " << m_width8 << ")";
    throw xtsc_exception(oss.str());
  }
  if (!axi && ((address8 % m_width8) != 0)) {
    XTSC_INFO(m_text, "do_nb_burst_read() returning RSP_ADDRESS_ERROR: address=0x" << hex << address8 <<
                         " size=" << dec << size8 << " m_width8=" << m_width8 << " Port #" << port_num);
    p_info->m_p_response->set_status(xtsc_response::RSP_ADDRESS_ERROR);
    send_response(*p_info->m_p_response, port_num);
    return true;
  }
  if (axi && (size8 > m_width8)) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': ";
    oss << "Received BURST_READ tag=" << p_info->m_p_request->get_tag() << " with byte size of " << size8
        << " (contradicts \"byte_width\" of " << m_width8 << ")";
    throw xtsc_exception(oss.str());
  }

  p_info->m_p_response->set_last_transfer(num_transfers == 1);

  tlm_generic_payload *p_trans = p_info->m_p_trans;
  u32  size        = size8;
  // tlm2_beats is > 1, if !use_tlm2_burst or 
  // for RD_WRAP transaction with address unaligned to the wrap boundary, when split_tlm2_phase is enabled
  u32  tlm2_beats  = (m_split_tlm2_phases && (base_address8 != address8)) ? 2 : (use_tlm2_burst ? 1 : num_transfers); 

  for (u32 transfer_count=1; transfer_count <= tlm2_beats; ++transfer_count) {
    u64 addr8 = address8;
    if ((transfer_count==1) || !use_tlm2_burst || m_split_tlm2_phases) {
      if (m_split_tlm2_phases && wrap) {
        size     = transfer_count==1 ? (upper_wrap_addr - addr8) : (total_size8 - size);
      }
      else if (use_tlm2_burst) {
        addr8    = base_address8;
        size     = total_size8;
      }

      p_trans->set_command                (TLM_READ_COMMAND);
      p_trans->set_address                (addr8);
      p_trans->set_data_length            (size);
      p_trans->set_streaming_width        (size);
      p_trans->set_byte_enable_length     (size);
      memset(p_trans->get_byte_enable_ptr(), 0xff, size);
      p_info->m_last_tlm2_beat = (transfer_count == tlm2_beats);
      if (m_use_tlm2_burst) {
        bool excl = check_and_set_exclusive(p_info->m_p_request,p_trans);
      }
      add_extensions(p_info->m_p_request,p_trans);
      tlm_sync_enum result = do_nb_transport_fw(*p_trans, p_info->m_phase, port_num);
      if (result == TLM_COMPLETED) {
        if (!m_split_tlm2_phases) {
          m_respond_thread_peq[port_num]->notify(*p_trans);
        } else {
          ostringstream oss;
          oss << name() << " Target completed transaction with response pending for " << *p_trans;
          throw xtsc_exception(oss.str());
        }         
      }
    }

    if (transfer_count < tlm2_beats) {
      xtsc_request *p_request = p_info->m_p_request;
      // Create new transaction_info for another tlm2_beat with same request
      p_info  = new_transaction_info(p_request);
      p_trans = p_info->m_p_trans;
      // Adjust address for next transfer.
      if (!fixed) {
        address8 += size;
        if (wrap) {
          if (address8 >= upper_wrap_addr ) {
            address8 -= total_size8;
          }
        }
      }
    }
  }
  return false;
}



// do_nb_TYPE
bool xtsc_component::xtsc_xttlm2tlm2_transactor::do_nb_rcw(transaction_info *p_info, u32 port_num) {
  ostringstream oss;
  oss << "Program Bug: " << kind() << " '" << name() << "' does not support RCW using nb_transport: " << *p_info->m_p_request;
  throw xtsc_exception(oss.str());
  return false;
}



// do_nb_TYPE
bool xtsc_component::xtsc_xttlm2tlm2_transactor::do_nb_write(transaction_info *p_info, u32 port_num) {
  u64  address8         = p_info->m_p_request->get_byte_address();
  u32  size8            = p_info->m_p_request->get_byte_size();
  XTSC_VERBOSE(m_text, "do_nb_write() address=0x" << hex << address8 << " size8=" << dec << size8 << " Port #" << port_num);
  if (((size8 !=  1) &&
       (size8 !=  2) &&
       (size8 !=  4) &&
       (size8 !=  8) &&
       (size8 != 16) &&
       (size8 != 32) &&
       (size8 != 64))            ||     // not a power of 2   .OR.
      (size8 > m_width8)         ||     // exceeds bus width  .OR.
      ((address8 % size8) != 0))        // not size8-aligned
  {
    XTSC_INFO(m_text, "do_nb_write() returning RSP_ADDRESS_ERROR: address=0x" << hex << address8 <<
                         " size=" << dec << size8 << " m_width8=" << m_width8 << " Port #" << port_num);
    p_info->m_p_response->set_status(xtsc_response::RSP_ADDRESS_ERROR);
    send_response(*p_info->m_p_response, port_num);
    return true;
  }
  else {
    tlm_generic_payload *p_trans = p_info->m_p_trans;
    p_trans->set_command                (TLM_WRITE_COMMAND);
    p_trans->set_address                (address8);
    p_trans->set_data_length            (size8);
    p_trans->set_streaming_width        (size8);
    p_trans->set_byte_enable_length     (size8);
    memcpy(p_trans->get_data_ptr(), p_info->m_p_request->get_buffer(), size8);
    xtsc_byte_enables be = p_info->m_p_request->get_byte_enables();
    u8 *p_byte_enables = p_trans->get_byte_enable_ptr();
    for (u32 i=0; i<size8; ++i) {
      p_byte_enables[i] = ((be & 1) ? 0xff : 0x00);
      be >>= 1;
    }
    bool excl = check_and_set_exclusive(p_info->m_p_request,p_trans);
    add_extensions(p_info->m_p_request,p_trans);
    tlm_sync_enum result = do_nb_transport_fw(*p_trans, p_info->m_phase, port_num);
    if (result == TLM_COMPLETED) {
      if (!m_split_tlm2_phases) {
        m_respond_thread_peq[port_num]->notify(*p_trans);
      } else {
        ostringstream oss;
        oss << name() << " Target completed transaction with response pending for " << *p_trans;
        throw xtsc_exception(oss.str());
      }
    } else { 
      if (m_split_tlm2_phases) {
        p_info->m_phase = BEGIN_WDATA_LAST;
        tlm_sync_enum result = do_nb_transport_fw(*p_trans, p_info->m_phase, port_num);
        if (result == TLM_COMPLETED) {
          ostringstream oss;
          oss << name() << " Target completed transaction with response pending for " << *p_trans;
          throw xtsc_exception(oss.str());
        }
      }
    }
  }
  return false;
}



// do_nb_TYPE
bool xtsc_component::xtsc_xttlm2tlm2_transactor::do_nb_block_write(transaction_info *p_info, u32 port_num) {
  if (!m_has_pif_width) throw_invalid_pif_bus_width(port_num);
  u64  address8         = p_info->m_p_request->get_byte_address();
  u32  size8            = p_info->m_p_request->get_byte_size();
  bool last_transfer    = p_info->m_p_request->get_last_transfer();
  u32  num_transfers    = p_info->m_p_request->get_num_transfers();
  u32  transfer_num     = p_info->m_p_request->get_transfer_number();
  u32  offset           = (m_use_tlm2_burst ? (address8 % (m_width8 * num_transfers)) : 0);
  u8  *p_data           = p_info->m_p_trans->get_data_ptr();
  u8  *p_byte_enables   = p_info->m_p_trans->get_byte_enable_ptr();
  XTSC_VERBOSE(m_text, "do_nb_block_write() address=0x" << hex << address8 << " size8=" << dec << size8 << " Port #" << port_num);
  // size must equal bus width and address must be bus width aligned
  if ((size8 != m_width8) || ((address8 % m_width8) != 0)) {
    if (last_transfer) {
      XTSC_INFO(m_text, "do_nb_block_write() returning RSP_ADDRESS_ERROR: address=0x" << hex << address8 <<
                        " size=" << dec << size8 << " m_width8=" << m_width8 << " Port #" << port_num);
      p_info->m_p_response->set_status(xtsc_response::RSP_ADDRESS_ERROR);
      send_response(*p_info->m_p_response, port_num);
    }
    return true;;
  }
  else {
    memcpy(p_data+offset, p_info->m_p_request->get_buffer(), m_width8);
    xtsc_byte_enables be = p_info->m_p_request->get_byte_enables();
    for (u32 i=0; i<m_width8; ++i) {
      p_byte_enables[offset + i] = ((be & 1) ? 0xff : 0x00);
      be >>= 1;
    }
    if (!m_use_tlm2_burst || last_transfer || m_split_tlm2_phases) {
      if (m_use_tlm2_burst) {
        size8     = m_width8*num_transfers;
        address8  = address8 - (address8 % size8);
      }
      tlm_generic_payload *p_trans = p_info->m_p_trans;
      p_trans->set_command                (TLM_WRITE_COMMAND);
      p_trans->set_address                (address8);
      p_trans->set_data_length            (size8);
      p_trans->set_streaming_width        (size8);
      p_trans->set_byte_enable_length     (size8);
      if (m_use_tlm2_burst) {
        bool excl = check_and_set_exclusive(p_info->m_p_request,p_trans);
      } 
      add_extensions(p_info->m_p_request,p_trans);
      tlm_sync_enum result = do_nb_transport_fw(*p_trans, p_info->m_phase, port_num);
      if (result == TLM_COMPLETED) {
        if (!m_split_tlm2_phases) {
          m_respond_thread_peq[port_num]->notify(*p_trans);
        } else {
          ostringstream oss;
          oss << name() << " Target completed transaction with response pending for " << *p_trans;
          throw xtsc_exception(oss.str());
        }
      } 
      else { 
        if (m_split_tlm2_phases) {
          if (p_info->m_phase == END_REQ) {
            if (last_transfer) {
              p_info->m_phase = BEGIN_WDATA_LAST;
            } else {
              p_info->m_phase = BEGIN_WDATA;
            }	
            tlm_sync_enum result = do_nb_transport_fw(*p_trans, p_info->m_phase, port_num);
            if (result == TLM_COMPLETED) {
              ostringstream oss;
              oss << name() << " Target completed transaction with response pending for " << *p_trans;
              throw xtsc_exception(oss.str());
            }
          } 
          if (p_info->m_phase == END_WDATA) {
            if (transfer_num == num_transfers-1) { //Set phase for nxt beat
              p_info->m_phase = BEGIN_WDATA_LAST; 
            } else {
              p_info->m_phase = BEGIN_WDATA; 
            }
          }
        }
      }
    }
  }
  return false;
}



// do_nb_TYPE
bool xtsc_component::xtsc_xttlm2tlm2_transactor::do_nb_burst_write(transaction_info *p_info, u32 port_num) {
  if (!m_has_pif_width) throw_invalid_pif_bus_width(port_num);
  u64  address8         = p_info->m_p_request->get_byte_address();
  u32  size8            = p_info->m_p_request->get_byte_size();
  u32  num_transfers    = p_info->m_p_request->get_actual_transfers();
  u32  transfer_num     = p_info->m_p_request->get_transfer_number();
  u32  total_size8      = size8 * num_transfers; 
  u64  base_address8    = address8 - size8 * (transfer_num - 1);
  bool last_transfer    = p_info->m_p_request->get_last_transfer();
//bool evict            = p_info->m_p_request->is_evict();   // TODO AXI and evict
  bool axi              = p_info->m_p_request->is_axi_protocol();
  bool fixed            = false;
  bool wrap             = false;
  u8  *p_data           = p_info->m_p_trans->get_data_ptr();
  u8  *p_byte_enables   = p_info->m_p_trans->get_byte_enable_ptr();
  if (axi) {
    xtsc_request::burst_t burst = p_info->m_p_request->get_burst_type();
    fixed = (burst == xtsc_request::FIXED);
    wrap  = (burst == xtsc_request::WRAP);
    if (wrap) {
      base_address8    = (address8 / total_size8) * total_size8;
    }
  }
  bool use_tlm2_burst   = m_use_tlm2_burst && !fixed && (total_size8 <= m_max_payload_bytes);
  u32  offset           = (use_tlm2_burst ? (address8 - base_address8) : 0);
  XTSC_VERBOSE(m_text, "do_nb_burst_write() address=0x" << hex << address8 << " size8=" << dec << size8 << " Port #" << port_num);
  // XTSC_NOTE(m_text,  "size8=" << size8 << " offset=" << offset << " num_transfers=" << num_transfers << " transfer_num=" << transfer_num << hex << " address8=0x" << address8 << " base_address8=0x" << base_address8 );

  if (use_tlm2_burst && (num_transfers > m_max_transfers)) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': ";
    oss << "Received BURST_WRITE tag=" << p_info->m_p_request->get_tag() << " with get_actual_transfers() of " << num_transfers
        << " (exceeds m_max_transfers of " << m_max_transfers << ")";
    throw xtsc_exception(oss.str());
  }
  if ((size8 != 1) && (size8 != 2) && (size8 != 4) && (size8 != 8) && (size8 != 16) && (size8 != 32) && (size8 != 64)) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': ";
    oss << "Received BURST_WRITE tag=" << p_info->m_p_request->get_tag() << " with byte size of " << size8 << " (which is not a power of 2)";
    throw xtsc_exception(oss.str());
  }
  if (!axi && (size8 != m_width8)) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': ";
    oss << "Received BURST_WRITE tag=" << p_info->m_p_request->get_tag() << " with byte size of " << size8
        << " (contradicts \"byte_width\" of " << m_width8 << ")";
    throw xtsc_exception(oss.str());
  }
  if (!axi && ((address8 % m_width8) != 0)) {
    XTSC_INFO(m_text, *p_info->m_p_request << " Port #" << port_num << " Unaligned address - " <<
                       (last_transfer ? "Returning RSP_ADDRESS_ERROR: " : "Dropping on floor: "));
    if (!last_transfer) return false;
    p_info->m_p_response->set_status(xtsc_response::RSP_ADDRESS_ERROR);
    send_response(*p_info->m_p_response, port_num);
    return true;
  }
  if (axi && (size8 > m_width8)) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': ";
    oss << "Received BURST_WRITE tag=" << p_info->m_p_request->get_tag() << " with byte size of " << size8 << " (exceeds \"byte_width\" of "
        << m_width8 << ")";
    throw xtsc_exception(oss.str());
  }

  memcpy(p_data+offset, p_info->m_p_request->get_buffer(), size8);
  xtsc_byte_enables be = p_info->m_p_request->get_byte_enables();
  for (u32 i=0; i<size8; ++i) {
    p_byte_enables[offset + i] = ((be & 1) ? 0xff : 0x00);
    be >>= 1;
  }
  if (!use_tlm2_burst || last_transfer || m_split_tlm2_phases) {
    u32 size  = (use_tlm2_burst ? total_size8 : size8);
    u64 addr8 = (use_tlm2_burst ? base_address8 : address8);
    tlm_generic_payload *p_trans = p_info->m_p_trans;
    p_trans->set_command                (TLM_WRITE_COMMAND);
    p_trans->set_address                (addr8);
    p_trans->set_data_length            (size);
    p_trans->set_streaming_width        (size);
    p_trans->set_byte_enable_length     (size);
    if (use_tlm2_burst) {
      bool excl = check_and_set_exclusive(p_info->m_p_request,p_trans);
    } 
    add_extensions(p_info->m_p_request,p_trans);
    tlm_sync_enum result = do_nb_transport_fw(*p_trans, p_info->m_phase, port_num);
    if (result == TLM_COMPLETED) {
      if (!m_split_tlm2_phases) {
        m_respond_thread_peq[port_num]->notify(*p_trans);
      } else {
        ostringstream oss;
        oss << name() << " Target completed transaction with response pending for " << *p_trans;
        throw xtsc_exception(oss.str());
      }
    }
    else { 
      if (m_split_tlm2_phases) {
        if (p_info->m_phase == END_REQ) {
          if (last_transfer) {
            p_info->m_phase = BEGIN_WDATA_LAST;
          } else {
            p_info->m_phase = BEGIN_WDATA;
          }
          tlm_sync_enum result = do_nb_transport_fw(*p_trans, p_info->m_phase, port_num);
          if (result == TLM_COMPLETED) {
            ostringstream oss;
            oss << name() << " Target completed transaction with response pending for " << *p_trans;
            throw xtsc_exception(oss.str());
          }
        } 
        if (p_info->m_phase == END_WDATA) {
          if (transfer_num == num_transfers-1) { //Set phase for nxt beat
            p_info->m_phase = BEGIN_WDATA_LAST; 
          } else {
            p_info->m_phase = BEGIN_WDATA; 
          }
        }
      }
    }
  }
  
  return false;
}



void xtsc_component::xtsc_xttlm2tlm2_transactor::xtsc_request_if_impl::nb_request(const xtsc_request& request) {
  XTSC_INFO(m_transactor.m_text, request << " Port #" << m_port_num);
  bool rcw = (request.get_type() == xtsc_request::RCW);
  if (m_transactor.m_using_nb_transport && !rcw) {
    // Check if we should NACC this request because we don't have room for it
    if (m_transactor.m_request_fifo[m_port_num]->num_free() == 0) {
      xtsc_response response(request, xtsc_response::RSP_NACC);
      XTSC_INFO(m_transactor.m_text, response << " (Request FIFO full Port #" << m_port_num << ")");
      (*m_transactor.m_respond_ports[m_port_num])->nb_respond(response);
      return;
    }
    xtsc_request *p_request= m_transactor.new_request(request);
    m_transactor.m_request_fifo[m_port_num]->nb_write(p_request);
    m_transactor.m_request_thread_event[m_port_num]->notify(SC_ZERO_TIME);
    return;
  }
  else if (m_transactor.m_worker_fifo) {
    // Check if we should NACC this request because we don't have room for it
    if (m_transactor.m_worker_fifo[m_port_num]->num_free() == 0) {
      xtsc_response response(request, xtsc_response::RSP_NACC);
      XTSC_INFO(m_transactor.m_text, response << " (Request FIFO full Port #" << m_port_num << ")");
      (*m_transactor.m_respond_ports[m_port_num])->nb_respond(response);
      return;
    }
    xtsc_request *p_request= m_transactor.new_request(request);
    m_transactor.m_worker_fifo[m_port_num]->nb_write(p_request);
  }
  else {
    // Check if we've already got a request
    if (m_transactor.m_busy[m_port_num]) {
      xtsc_response response(request, xtsc_response::RSP_NACC);
      XTSC_INFO(m_transactor.m_text, response << " Port #" << m_port_num<< " (Busy)");
      // No need to check return value; nb_respond with RSP_NACC cannot return false
      (*m_transactor.m_respond_ports[m_port_num])->nb_respond(response);
      return;
    }
    xtsc_request *p_request= m_transactor.new_request(request);
    m_transactor.m_request[m_port_num] = p_request;
    m_transactor.m_busy[m_port_num] = true;
  }
  if (m_transactor.m_immediate_timing) {
    m_transactor.m_worker_thread_event[m_port_num]->notify();
  }
  else {
    m_transactor.m_worker_thread_event[m_port_num]->notify(SC_ZERO_TIME);
  }
}



void xtsc_component::xtsc_xttlm2tlm2_transactor::xtsc_request_if_impl::nb_lock(bool lock) {
  const char *envvar = "XTSC_XTTLM2TLM2_TRANSACTOR_SQUELCH_NB_LOCK";
  if (getenv(envvar) == NULL) {
    XTSC_WARN(m_transactor.m_text, "nb_lock" << (lock ? "(true) " : "(false)") << " call ignored by " << m_transactor.kind() <<
                                   ".  You can define environment variable " << envvar << " to squelch this warning.");
  }
  else {
    XTSC_INFO(m_transactor.m_text, "nb_lock(" << boolalpha << lock << ") call ignored.");
  }
}



void xtsc_component::xtsc_xttlm2tlm2_transactor::xtsc_request_if_impl::nb_peek(xtsc_address address8, u32 size8, u8 *buffer) {
  XTSC_VERBOSE(m_transactor.m_text, "nb_peek addr=0x" << hex << address8 << " size8=0x" << size8 << " Port #" << m_port_num);

  tlm_generic_payload *p_trans = m_transactor.new_transaction();
  p_trans->set_command        (TLM_READ_COMMAND);
  p_trans->set_address        (address8);
  p_trans->set_data_length    (size8);
  p_trans->set_streaming_width(size8);
  p_trans->set_response_status(TLM_INCOMPLETE_RESPONSE);
  p_trans->set_data_ptr       (buffer);
  p_trans->set_byte_enable_ptr(0);

  try { m_transactor.do_transport_dbg(*p_trans, m_port_num); } catch (...) { p_trans->release(); throw; }

  p_trans->release();
}



void xtsc_component::xtsc_xttlm2tlm2_transactor::xtsc_request_if_impl::nb_poke(xtsc_address address8, u32 size8, const u8 *buffer) {
  XTSC_VERBOSE(m_transactor.m_text, "nb_poke addr=0x" << hex << address8 << " size8=0x" << size8 << " Port #" << m_port_num);

  tlm_generic_payload *p_trans = m_transactor.new_transaction();
  p_trans->set_command        (TLM_WRITE_COMMAND);
  p_trans->set_address        (address8);
  p_trans->set_data_length    (size8);
  p_trans->set_streaming_width(size8);
  p_trans->set_response_status(TLM_INCOMPLETE_RESPONSE);
  p_trans->set_data_ptr       (const_cast<u8*>(buffer));
  p_trans->set_byte_enable_ptr(0);

  try { m_transactor.do_transport_dbg(*p_trans, m_port_num); } catch (...) { p_trans->release(); throw; }

  p_trans->release();
}



bool xtsc_component::xtsc_xttlm2tlm2_transactor::xtsc_request_if_impl::nb_fast_access(xtsc_fast_access_request &request) {

  u32 n = xtsc_address_nibbles();

  if (m_transactor.m_turbo_support == 0) {
    request.deny_access();
    return true;
  }

  xtsc_address address8 = request.get_translated_request_address();

  if (m_transactor.m_turbo_support == 1) {
    XTSC_INFO(m_transactor.m_text, "nb_fast_access: addr=0x" << hex << setfill('0') << setw(n) << address8 << " peek/poke access" <<
                                   " Port #" << m_port_num);
    request.allow_peek_poke_access();
  }
  else if (m_transactor.m_turbo_support == 2) {
    xtsc_address req_addr = request.get_request_address();

    tlm_generic_payload *p_trans = m_transactor.new_transaction();

    tlm_dmi dmi_data;

    p_trans->set_command(TLM_READ_COMMAND);
    p_trans->set_address(address8);

    bool read_result = m_transactor.do_get_direct_mem_ptr(*p_trans, dmi_data, m_port_num);

    xtsc_address read_start_address = (xtsc_address) dmi_data.get_start_address();
    xtsc_address read_end_address   = (xtsc_address) dmi_data.get_end_address();
    xtsc_fast_access_block block(address8, read_start_address, read_end_address);

    XTSC_VERBOSE(m_transactor.m_text,   "nb_fast_access:" << hex << setfill('0') <<
                                        " req_addr=0x"    << setw(n) << req_addr <<
                                        " address8=0x"    << setw(n) << address8 <<
                                        "  read_start=0x" << setw(n) << read_start_address <<
                                        "  read_end=0x"   << setw(n) << read_end_address <<
                                        " result="        << read_result <<
                                        " read()="        << dmi_data.is_read_allowed() <<
                                        " write()="       << dmi_data.is_write_allowed() << " Port #" << m_port_num);

    bool read_allowed = dmi_data.is_read_allowed();

    if (!read_result || !read_allowed) {
      request.deny_access();
      request.restrict_to_block(block);  // Has to come after the deny_access call
      p_trans->release();

      xtsc_fast_access_block result_block = request.get_result_block();
      XTSC_INFO(m_transactor.m_text, "nb_fast_access: addr=0x" << hex << setfill('0') << setw(n) << result_block.get_target_address() <<
                                     " block=[0x" << setw(n) << result_block.get_block_beg_address() <<
                                     "-0x" << setw(n) << result_block.get_block_end_address() << "]" <<
                                     " access=" << request.get_access_type_c_str() << " Port #" << m_port_num);
      return true;
    }

    u8 *p_raw_data = dmi_data.get_dmi_ptr();
    bool readonly = false;

    if (!dmi_data.is_write_allowed() ) {
      dmi_data.init();

      p_trans->set_command(TLM_WRITE_COMMAND);
      p_trans->set_address(address8);

      m_transactor.m_dmi_invalidated = false;

      bool write_result = m_transactor.do_get_direct_mem_ptr(*p_trans, dmi_data, m_port_num);

      if (m_transactor.m_dmi_invalidated) {
        ostringstream oss;
        oss << m_transactor.kind() << ", '" << m_transactor.name() << "' cannot get DMI access to 0x" << hex << setfill('0') << setw(n)
            << address8 << " because my invalidate_direct_mem_ptr() was called from my call to downstream get_direct_mem_ptr()"
            << " Port #" << m_port_num;
        throw xtsc_exception(oss.str());
      }

      xtsc_address write_start_address = (xtsc_address) dmi_data.get_start_address();
      xtsc_address write_end_address   = (xtsc_address) dmi_data.get_end_address();

      XTSC_VERBOSE(m_transactor.m_text,   "nb_fast_access:" << hex << setfill('0') <<
                                          " address8=0x"    << setw(n) << address8 <<
                                          " write_start=0x" << setw(n) << write_start_address <<
                                          " write_end=0x"   << setw(n) << write_end_address <<
                                          " result="        << write_result <<
                                          " read()="        << dmi_data.is_read_allowed() <<
                                          " write()="       << dmi_data.is_write_allowed() << " Port #" << m_port_num);

      readonly = (!write_result                              ||
                  !dmi_data.is_write_allowed()               ||
                  (write_start_address > read_start_address) ||
                  (write_end_address   < read_end_address));
    }

    request.allow_raw_access(read_start_address, (u32*)p_raw_data, (read_end_address - read_start_address + 1), 0);

    if (readonly) {
      XTSC_VERBOSE(m_transactor.m_text, "Calling deny_write_access()" << " Port #" << m_port_num);
      request.deny_write_access();
    }

    request.restrict_to_block(block);

    m_transactor.m_allowed_ranges[m_port_num]->push_back(new address_range(read_start_address, read_end_address));

    p_trans->release();
  }
  else {
    ostringstream oss;
    oss << "PROGRAM BUG: m_turbo_support=" << m_transactor.m_turbo_support <<  " in " << __FILE__ << ":" << __LINE__
        << " Port #" << m_port_num;
    throw xtsc_exception(oss.str());
  }

  // Determine whether or not address8 is inside a "deny_fast_access" block:
  // - If it is, then deny fast access to the "deny_fast_access" block
  // - If it is not, then ensure grant does not include any "deny_fast_access" addresses.
  for (u32 i=0; i<m_transactor.m_deny_fast_access.size(); i+=2) {
    xtsc_address begin = m_transactor.m_deny_fast_access[i];
    xtsc_address end   = m_transactor.m_deny_fast_access[i+1];
    if ((address8 >= begin) && (address8 <= end)) {
      // Deny fast access
      request.deny_access();
      xtsc_fast_access_block deny_block(address8, begin, end);
      request.restrict_to_block(deny_block);
      XTSC_VERBOSE(m_transactor.m_text, "nb_fast_access: addr=0x" << hex << setfill('0') << setw(n) << address8 <<
                                        " deny=[0x" << setw(n) << begin << ",0x" << setw(n) << end << "]" << " Port #" << m_port_num);
      break;
    }
    request.remove_address_range(address8, begin, end);
    XTSC_VERBOSE(m_transactor.m_text, "nb_fast_access: addr=0x" << hex << setfill('0') << setw(n) << address8 <<
                                      " remove=[0x" << setw(n) << begin << "-0x" << setw(n) << end << "]" << " Port #" << m_port_num);
  }

  xtsc_fast_access_revocation_if *p_if = request.get_fast_access_revocation_if();

  if (p_if == NULL) {
    m_transactor.m_can_revoke_fast_access = false;
  }
  else {
    m_transactor.m_revocation_set.insert(p_if);
  }

  xtsc_fast_access_block result_block = request.get_result_block();
  XTSC_INFO(m_transactor.m_text, "nb_fast_access: addr=0x" << hex << setfill('0') << setw(n) << result_block.get_target_address() <<
                                 " block=[0x" << setw(n) << result_block.get_block_beg_address() <<
                                 "-0x" << setw(n) << result_block.get_block_end_address() << "]" <<
                                 " access=" << request.get_access_type_c_str() << " writable=" << request.is_writable() <<
                                 " Port #" << m_port_num);

  return true;
}



void xtsc_component::xtsc_xttlm2tlm2_transactor::xtsc_request_if_impl::register_port(sc_port_base& port, const char *if_typename) {
  if (m_p_port) {
    ostringstream oss;
    oss << "Illegal multiple binding detected to xtsc_xttlm2tlm2_transactor::m_request_exports[" << m_port_num << "] of '"
        << m_transactor.name() << "': " << endl;
    oss << "  " << port.name() << endl;
    oss << "  " << m_p_port->name();
    throw xtsc_exception(oss.str());
  }
  XTSC_INFO(m_transactor.m_text, "Binding '" << port.name() << "' to xtsc_xttlm2tlm2_transactor::m_request_exports[" << m_port_num <<
                                 "]");
  m_p_port = &port;
}



tlm_sync_enum xtsc_component::xtsc_xttlm2tlm2_transactor::tlm_bw_transport_if_impl::nb_transport_bw(tlm_generic_payload&      trans,
                                                                                                    tlm_phase&                phase,
                                                                                                    sc_time&                  time)
{
  XTSC_INFO(m_transactor.m_text, trans << " " << xtsc_tlm_phase_text(phase) << " (" << time << ") nb_transport_bw[" << m_port_num << "]");

  transaction_info_map::iterator i = m_transactor.m_transaction_info_map.find(&trans);
  if (i == m_transactor.m_transaction_info_map.end()) {
    ostringstream oss;
    oss << m_transactor.name() << " received nb_transport_bw call on port #" << m_port_num << " with unknown transaction: " << trans;
    throw xtsc_exception(oss.str());
  }
  transaction_info *p_info = i->second;
  if (m_transactor.m_split_tlm2_phases && trans.get_command() == TLM_WRITE_COMMAND) {
    ostringstream oss;
    //Adhere to phase sequence
    if ((p_info->m_phase == BEGIN_REQ) && phase != END_REQ) {
      oss << m_transactor.name() << " Received phase " << xtsc_tlm_phase_text(phase) << " for expecting END_REQ" 
                                 << " for transaction: " << trans;
      throw xtsc_exception(oss.str());
    }
    else if ((p_info->m_phase == BEGIN_WDATA) && phase != END_WDATA) {
      oss << m_transactor.name() << " Received phase " << xtsc_tlm_phase_text(phase) << " for expecting END_WDT" 
                                 << " for transaction: " << trans;
      throw xtsc_exception(oss.str());
    }
  }
  if ((p_info->m_phase == BEGIN_REQ) || 
      (m_transactor.m_split_tlm2_phases && 
       (p_info->m_phase == BEGIN_WDATA) || (p_info->m_phase == BEGIN_WDATA_LAST))) {
    m_transactor.m_request_phase_ended_event[m_port_num]->notify(SC_ZERO_TIME);
  }
  p_info->m_phase = phase;
  if ((phase == END_REQ) ||
      (m_transactor.m_split_tlm2_phases && 
       (p_info->m_phase == END_WDATA) || (p_info->m_phase == END_WDATA_LAST))) {
    return TLM_ACCEPTED;
  }
  if (m_transactor.m_split_tlm2_phases && phase == BEGIN_RDATA) {
    m_transactor.m_respond_thread_peq[m_port_num]->notify(trans, time);
    return TLM_ACCEPTED;
  }
  if (phase == BEGIN_RESP) {
    m_transactor.m_respond_thread_peq[m_port_num]->notify(trans, time);
  }
  else {
    ostringstream oss;
    oss << m_transactor.name() << " received nb_transport_bw call on port #" << m_port_num << " with unsupported phase="
        << xtsc_tlm_phase_text(phase) << " for transaction: " << trans;
    throw xtsc_exception(oss.str());
  }
  return TLM_COMPLETED;
}



void xtsc_component::xtsc_xttlm2tlm2_transactor::tlm_bw_transport_if_impl::invalidate_direct_mem_ptr(uint64 start_range,
                                                                                                     uint64 end_range)
{
  u32 n = xtsc_address_nibbles();
  m_transactor.m_dmi_invalidated = true;
  XTSC_INFO(m_transactor.m_text, "invalidate_direct_mem_ptr() range: 0x" << hex << setfill('0') << setw(n) << start_range << 
                                 "-0x" << setw(n) << end_range << " Port #" << dec << m_port_num);

  if (m_transactor.m_can_revoke_fast_access) {
    set<xtsc_fast_access_revocation_if*>::iterator i;
    for (i = m_transactor.m_revocation_set.begin(); i != m_transactor.m_revocation_set.end(); ++i) {
      (*i)->revoke_fast_access();
    }
    m_transactor.m_revocation_set.clear();
    return;
  }

  if (m_transactor.m_turbo_support == 2) {
    list<address_range*> *p_allowed_ranges = m_transactor.m_allowed_ranges[m_port_num];
    for (list<address_range*>::iterator i = p_allowed_ranges->begin(); i != p_allowed_ranges->end(); ) {
      address_range *p_range = *i;
      if ((end_range >= p_range->m_beg_range) && (start_range <= p_range->m_end_range)) {
        ostringstream oss;
        oss << m_transactor.kind() << ", '" << m_transactor.name() << "' (with parameter \"turbo_support\"=2): "
            << "invalidate_direct_mem_ptr(0x" << hex << setfill('0') << setw(n) << start_range << ", 0x" << setw(n) << end_range
            << ") called which invalidates previously granted range 0x" << setw(n) << p_range->m_beg_range << "-0x" << setw(n)
            << p_range->m_end_range << " Port #" << dec << m_port_num;
        throw xtsc_exception(oss.str());
      }
      else {
        ++i;
      }
    }
  }
}



void xtsc_component::xtsc_xttlm2tlm2_transactor::tlm_bw_transport_if_impl::register_port(sc_port_base& port, const char *if_typename) {
  if (m_p_port) {
    ostringstream oss;
    oss << "Illegal multiple binding detected to xtsc_xttlm2tlm2_transactor '" << m_transactor.name() << "' " << name() << ": " << endl;
    oss << "  " << port.name() << endl;
    oss << "  " << m_p_port->name();
    throw xtsc_exception(oss.str());
  }
  XTSC_INFO(m_transactor.m_text, "Binding '" << port.name() << "' to xtsc_xttlm2tlm2_transactor::" << name() << " Port #" << m_port_num);
  m_p_port = &port;
}



// Used by nb_transport
void xtsc_component::xtsc_xttlm2tlm2_transactor::nb_mm::free(tlm::tlm_generic_payload *p_trans) {
  transaction_info_map::iterator i = m_transactor.m_transaction_info_map.find(p_trans);
  if (i == m_transactor.m_transaction_info_map.end()) {
    ostringstream oss;
    oss << m_transactor.name() << " xtsc_xttlm2tlm2_transactor::nb_mm::free() called with unknown transaction: " << *p_trans;
    throw xtsc_exception(oss.str());
  }
  transaction_info *p_info = i->second;
  delete p_info->m_p_response;
  p_info->m_p_response = NULL;
  p_trans->reset();
  m_transactor.m_transaction_info_pool.push_back(p_info);
  m_transactor.m_transaction_info_map.erase(i);
}



