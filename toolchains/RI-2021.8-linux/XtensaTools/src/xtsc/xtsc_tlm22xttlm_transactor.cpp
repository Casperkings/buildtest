// Copyright (c) 2006-2019 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of
// Cadence Design Systems, Inc.  They may be adapted and modified by bona fide
// purchasers for internal use, but neither the original nor any adapted
// or modified version may be disclosed or distributed to third parties
// in any manner, medium, or form, in whole or in part, without the prior
// written consent of Cadence Design Systems, Inc.

/*
 *                                   Theory of Operation
 *
 *
 * This transactor converts OSCI TLM2 transactions to Xtensa TLM transactions.  Basically this means
 * it converts the tlm_fw_transport_if API's to the xtsc_request_if API's.  
 * 
 * The tlm_fw_transport_if API's are:
 *   b_transport
 *   nb_transport_fw
 *   get_direct_mem_ptr
 *   transport_dbg
 * 
 * The xtsc_request_if API's are:
 *   nb_request
 *   nb_fast_access
 *   nb_peek
 *   nb_poke
 *
 * The implementation of the tlm_fw_transport_if API's and associated methods is discussed below.
 *
 *
 *
 * b_transport => transport_helper => worker_thread => nb_respond:
 *
 * When b_transport of this transactor is called by the upstream OSCI TLM2 subsystem, it simply
 * forwards the call to transport_helper with blocking set to true.  When transport_helper returns,
 * b_transport returns to the upstream caller.
 *
 *
 *
 * nb_transport_fw => nb_thread => transport_helper => worker_thread => nb_respond => respond_helper:
 *
 * When nb_transport_fw of this transactor is called by the upstream OSCI TLM2 subsystem, it hands the
 * TLM2 transaction off to nb_thread by putting it into payload event queue (m_peq) for handling at
 * the time implied by the sc_time argument to the nb_transport_fw call and then immediately returns
 * to the upstream caller (as is required by the non-blocking nature of nb_transport_fw).
 *
 *
 *
 * nb_thread:
 *
 * When nb_thread is awoke by the TLM2 transaction coming out of the payload event queue, it calls
 * the transport_helper method with the transaction and with blocking set to false.  When
 * transport_helper returns, nb_thread gets the next transaction from the payload event queue (or
 * goes back to sleep if there are none).
 *
 *
 *
 * transport_helper:
 *
 * When tranport_helper is called, it creates a transaction_info object to store the TLM2 transaction
 * in and then also converts the TLM2 transaction into a sequence of 1 or more xtsc_request objects
 * which it stores in the m_requests vector of the transaction_info object. Each xtsc_request object
 * is assigned a unique xtsc_request ID (except that a sequence of BLOCK_WRITE or BURST_WRITE
 * requests all have the same ID).  If split_tlm2_phases=true, these sequence of xtsc_requests are 
 * created as and when the corresponding write data transfers are received in TLM2 payload using 
 * additional TLM2 phases.  The transport_helper method then notifies worker_thread (using
 * m_worker_thread_event) to send out the requests.  What happens next depends on whether 
 * transport_helper was called with blocking false or true.
 *
 * blocking=false (i.e. transport_helper called from nb_thread after a nb_transport_fw call):
 * The tranport_helper method returns at this point.
 *
 * blocking=true (i.e. transport_helper called from b_transport)
 * The transport_helper method now waits for all requests to go out and for all responses to come
 * back (m_port_done_event).  When m_port_done_event is notified, transport_helper returns to
 * b_transport which returns to the upstream caller.
 *
 *
 *
 * worker_thread:
 *
 * When worker_thread is notified by transport_helper that it has some work to do, it processes each
 * transaction_info object in sequence by calling the nb_request method for each xtsc_request object
 * in the m_requests vector of transaction_info.  worker_thread is responsible to ensure that
 * nb_request calls don't occur more often then one per m_clock_period.  worker_thread is also
 * responsible for repeating the nb_request call if it receives an RSP_NACC within one
 * m_clock_period.  After all requests are sent out, if all responses have already been received,
 * then worker_thread either notifies the m_port_done_event (transaction_info::m_blocking is true) or
 * calls respond_helper (otherwise).
 *
 *
 *
 * nb_respond:
 *
 * The downstream Xtensa TLM subsystem responds to nb_request calls from worker_thread by calling
 * this transactors nb_respond method with an xtsc_response object.  The nb_respond method uses
 * the ID in the xtsc_response object to find the corresponding transaction_info object and then
 * copies any read data from the xtsc_response object to the tlm_generic_paylod object in the TLM2
 * transaction stored in the transaction_info object.  When all responses are received, and if
 * worker_thread has completed its wait for any potential RSP_NACC for the final request, then
 * nb_respond decides what to do depending on transaction_info::m_blocking:
 * If m_blocking is true, then nb_respond notifies b_transport via the m_port_done_event.
 * If m_blocking is false, then nb_respond calls respond_helper().
 *
 *
 *
 * respond_helper:
 *
 * The respond_helper method calls nb_transport_bw of the upsteam TLM2 subsysem with the completed
 * TLM2 transaction and then deletes the transaction_info object.
 *
 *
 *
 * get_direct_mem_ptr:
 *
 * When get_direct_mem_ptr of this transactor is called by the upstream OSCI TLM2 subsystem, it calls
 * nb_fast_access of the downstream Xtensa TLM subsystem to see if it can get raw fast access.
 *
 *
 *
 * transport_dbg:
 *
 * When transport_dbg of this transactor is called by the upstream OSCI TLM2 subsystem, it calls
 * either nb_peek or nb_poke of the downstream Xtensa TLM subsystem as appropriate.
 *
 */

#include <cerrno>
#include <algorithm>
#include <ostream>
#include <string>
#include <xtsc/xtsc_tlm22xttlm_transactor.h>
#include <xtsc/xtsc_tlm2.h>
#include <xtsc/xtsc_core.h>
#include <xtsc/xtsc_master_tlm2.h>
#include <xtsc/xtsc_xttlm2tlm2_transactor.h>

using namespace std;
using namespace sc_core;
using namespace sc_dt;
using namespace tlm;
using namespace tlm_utils;
using namespace log4xtensa;
using namespace xtsc;

static const char *exclusive_support(u32 option) {
  const char *support[2] = {
    "Not supported",                    // 0
    "Use TLM2 ignorable extensions",    // 1
  };
  if (option > 1) return NULL;
  return support[option];
}


xtsc_component::xtsc_tlm22xttlm_transactor::xtsc_tlm22xttlm_transactor(
    sc_module_name                               module_name,
    const xtsc_tlm22xttlm_transactor_parms&      parms
    ) :
  sc_module                     (module_name),
  xtsc_module                   (*(sc_module*)this),
  m_request_ports               (NULL),
  m_respond_exports             (NULL),
  m_target_sockets_4            (NULL),
  m_target_sockets_8            (NULL),
  m_target_sockets_16           (NULL),
  m_target_sockets_32           (NULL),
  m_target_sockets_64           (NULL),
  m_xtsc_respond_if_impl        (NULL),
  m_tlm_fw_transport_if_impl    (NULL),
  m_next_port_num_worker        (0),
  m_next_port_num_nb_thread     (0),
  m_num_ports                   (parms.get_non_zero_u32 ("num_ports")),
  m_width8                      (parms.get_u32          ("byte_width")),
  m_enable_extensions           (parms.get_bool         ("enable_extensions")),
  m_exclusive_support           (parms.get_u32          ("exclusive_support")),
  m_priority                    (parms.get_u32          ("priority")),
  m_pc                          (parms.get_u32          ("pc")),
  m_max_burst_beats             (parms.get_u32          ("max_burst_beats")),
  m_apb                         (parms.get_bool         ("apb")),
  m_axi                         (parms.get_bool         ("axi")),
  m_use_axi_fixed               (parms.get_bool         ("use_axi_fixed")),
  m_use_pif_block               (parms.get_bool         ("use_pif_block")),
  m_use_pif_burst               (parms.get_bool         ("use_pif_burst")),
  m_allow_dmi                   (parms.get_bool         ("allow_dmi")),
  m_allow_transport_dbg         (parms.get_bool         ("allow_transport_dbg")),
  m_split_tlm2_phases           (parms.get_bool         ("split_tlm2_phases")),
  m_time_resolution             (sc_get_time_resolution()),
  m_worker_thread_event         (NULL),
  m_port_done_event             (NULL),
  m_peq                         (NULL),
  m_waiting_for_nacc            (NULL),
  m_request_got_nacc            (NULL),
  m_waiting_for_end_resp        (NULL),
  m_next_id                     (NULL),
  m_outstanding_req_count       (NULL),
  m_tag_to_transaction_info_tab (NULL),
  m_tag_to_request_tab          (NULL),
  m_trans_to_info_map           (NULL),
  m_pending_transaction_deque   (NULL),
  m_transaction_info_count      (0),
  m_request_count               (0),
  m_text                        (TextLogger::getInstance(name()))
{

  if (m_apb) {
    if (m_axi) {
      ostringstream oss;
      oss << kind() << " '" << name() << "': \"apb\" and \"axi\" parameters cannot both be true";
      throw xtsc_exception(oss.str());
    }

    if (m_width8 !=  4) {
      ostringstream oss;
      oss << kind() << " '" << name() << "': Invalid \"byte_width\" parameter value (" << m_width8 << ").  Expected 4 when \"apb\" is true.";
      throw xtsc_exception(oss.str());
    }

    m_priority = 0;
  }

  if ((m_width8 != 4) && (m_width8 != 8) && (m_width8 != 16) && (m_width8 != 32) && (m_width8 != 64)) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': Invalid \"byte_width\" parameter value (" << m_width8 << ").  Expected 4|8|16|32|64.";
    throw xtsc_exception(oss.str());
  }

  if (exclusive_support(m_exclusive_support) == NULL) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': Invalid \"exclusive_support\" parameter value (" << m_exclusive_support << ").  Expected 0|1.";
    throw xtsc_exception(oss.str());
  }

  if (m_exclusive_support != 0 && m_apb) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': \"exclusive_support\" parameter " << m_exclusive_support << "can't be used if apb is set.";
    throw xtsc_exception(oss.str());
  }

  if (m_width8 > 16) {
    m_use_pif_block = false;
    m_use_pif_burst = false;
  }

  // Create all the "per-port" stuff

  m_request_ports               = new sc_port  <xtsc_request_if>       *[m_num_ports];
  m_respond_exports             = new sc_export<xtsc_respond_if>       *[m_num_ports];

       if (m_width8 ==  4) { m_target_sockets_4  = new target_socket_4 *[m_num_ports]; }
  else if (m_width8 ==  8) { m_target_sockets_8  = new target_socket_8 *[m_num_ports]; }
  else if (m_width8 == 16) { m_target_sockets_16 = new target_socket_16*[m_num_ports]; }
  else if (m_width8 == 32) { m_target_sockets_32 = new target_socket_32*[m_num_ports]; }
  else if (m_width8 == 64) { m_target_sockets_64 = new target_socket_64*[m_num_ports]; }

  m_xtsc_respond_if_impl        = new xtsc_respond_if_impl             *[m_num_ports];
  m_tlm_fw_transport_if_impl    = new tlm_fw_transport_if_impl         *[m_num_ports];

  m_next_id                     = new u8                                [m_num_ports];
  m_outstanding_req_count       = new u32                               [m_num_ports];
  m_tag_to_transaction_info_tab = new map<u64,transaction_info*>        [m_num_ports];
  m_tag_to_request_tab          = new map<u64,int>                      [m_num_ports];
  m_trans_to_info_map           = new map<tlm_generic_payload*,
                                          transaction_info*>            [m_num_ports];
  m_pending_transaction_deque   = new deque<transaction_info*>         *[m_num_ports];

  m_worker_thread_event         = new sc_event                         *[m_num_ports];
  m_port_done_event             = new sc_event                         *[m_num_ports];
  m_peq                         = new peq                              *[m_num_ports];
  m_waiting_for_nacc            = new bool                              [m_num_ports];
  m_request_got_nacc            = new bool                              [m_num_ports];
  m_waiting_for_end_resp        = new bool                              [m_num_ports];

  for (u32 i=0; i<m_num_ports; ++i) {

    ostringstream oss1;
    oss1 << "m_request_ports[" << i << "]";
    m_request_ports[i] = new sc_port<xtsc_request_if>(oss1.str().c_str());

    ostringstream oss2;
    oss2 << "m_respond_exports[" << i << "]";
    m_respond_exports[i] = new sc_export<xtsc_respond_if>(oss2.str().c_str());

    ostringstream oss3;
    oss3 << "m_target_sockets_" << m_width8 << "[" << i << "]";
         if (m_width8 ==  4) { m_target_sockets_4 [i] = new target_socket_4 (oss3.str().c_str()); }
    else if (m_width8 ==  8) { m_target_sockets_8 [i] = new target_socket_8 (oss3.str().c_str()); }
    else if (m_width8 == 16) { m_target_sockets_16[i] = new target_socket_16(oss3.str().c_str()); }
    else if (m_width8 == 32) { m_target_sockets_32[i] = new target_socket_32(oss3.str().c_str()); }
    else if (m_width8 == 64) { m_target_sockets_64[i] = new target_socket_64(oss3.str().c_str()); }

    ostringstream oss4;
    oss4 << "m_xtsc_respond_if_impl[" << i << "]";
    m_xtsc_respond_if_impl[i] = new xtsc_respond_if_impl(oss4.str().c_str(), *this, i);

    ostringstream oss5;
    oss5 << "m_tlm_fw_transport_if_impl[" << i << "]";
    m_tlm_fw_transport_if_impl[i] = new tlm_fw_transport_if_impl(oss5.str().c_str(), *this, i);

    (*m_respond_exports[i])(*m_xtsc_respond_if_impl[i]);

         if (m_width8 ==  4) { (*m_target_sockets_4 [i])(*m_tlm_fw_transport_if_impl[i]); }
    else if (m_width8 ==  8) { (*m_target_sockets_8 [i])(*m_tlm_fw_transport_if_impl[i]); }
    else if (m_width8 == 16) { (*m_target_sockets_16[i])(*m_tlm_fw_transport_if_impl[i]); }
    else if (m_width8 == 32) { (*m_target_sockets_32[i])(*m_tlm_fw_transport_if_impl[i]); }
    else if (m_width8 == 64) { (*m_target_sockets_64[i])(*m_tlm_fw_transport_if_impl[i]); }

    m_pending_transaction_deque  [i] = new deque<transaction_info*>;

    ostringstream oss6;
    oss6 << "peq_" << i;
    m_peq                       [i] = new peq(oss6.str().c_str());

    ostringstream oss7;
    ostringstream oss9;
    oss7 << "worker_thread_" << i;
    oss9 << "port_" << i;
#if IEEE_1666_SYSTEMC >= 201101L
    m_worker_thread_event[i] = new sc_event((oss7.str() + "_event")     .c_str());
    m_port_done_event    [i] = new sc_event((oss9.str() + "_done_event").c_str());
#else
    m_worker_thread_event[i] = new sc_event;
    xtsc_event_register(*m_worker_thread_event[i], oss7.str() + "_event",      this);
    m_port_done_event    [i] = new sc_event;
    xtsc_event_register(*m_port_done_event[i],     oss9.str() + "_done_event", this);
#endif
    declare_thread_process(worker_thread_handle, oss7.str().c_str(), SC_CURRENT_USER_MODULE, worker_thread);
    m_process_handles.push_back(sc_get_current_process_handle());
    ostringstream oss8;
    oss8 << "nb_thread_" << i;
    declare_thread_process(nb_thread_handle, oss8.str().c_str(), SC_CURRENT_USER_MODULE, nb_thread);
    m_process_handles.push_back(sc_get_current_process_handle());

  }

  // Get clock period 
  u32 clock_period = parms.get_u32("clock_period");
  if (clock_period == 0xFFFFFFFF) {
    m_clock_period = xtsc_get_system_clock_period();
  }
  else {
    m_clock_period = m_time_resolution * clock_period;
  }
  m_clock_period_value = m_clock_period.value();

  compute_delays();

  for (u32 i=0; i<m_num_ports; ++i) {
    ostringstream oss;
    oss.str(""); oss << "m_request_ports"   << "[" << i << "]"; m_port_types[oss.str()] = REQUEST_PORT;
    oss.str(""); oss << "m_respond_exports" << "[" << i << "]"; m_port_types[oss.str()] = RESPOND_EXPORT;
    oss.str(""); oss << "master_port"       << "[" << i << "]"; m_port_types[oss.str()] = PORT_TABLE;
    oss.str(""); oss << "m_target_sockets_" << m_width8 << "[" << i << "]";
    switch (m_width8) {
      case  4:  m_port_types[oss.str()] = TARGET_SOCKET_4;  break;
      case  8:  m_port_types[oss.str()] = TARGET_SOCKET_8;  break;
      case 16:  m_port_types[oss.str()] = TARGET_SOCKET_16; break;
      case 32:  m_port_types[oss.str()] = TARGET_SOCKET_32; break;
      case 64:  m_port_types[oss.str()] = TARGET_SOCKET_64; break;
    }
    oss.str(""); oss << "target_socket[" << i << "]"; m_port_types[oss.str()] = PORT_TABLE;
  }

  m_port_types["target_sockets"] = PORT_TABLE;
  m_port_types["master_ports"  ] = PORT_TABLE;

  if (m_num_ports == 1) {
    m_port_types["target_socket"] = PORT_TABLE;
    m_port_types["master_port"  ] = PORT_TABLE;
  }

  xtsc_register_command(*this, *this, "allow_transport_dbg", 1, 1,
      "allow_transport_dbg <Allow>", 
      "Call xtsc_tlm22xttlm_transactor::allow_transport_dbg(<Allow>).  Where <Allow> is 0|1."
      "  Return previous <Allow> value for this device."
  );

  xtsc_register_command(*this, *this, "change_clock_period", 1, 1,
      "change_clock_period <ClockPeriodFactor>", 
      "Call xtsc_tlm22xttlm_transactor::change_clock_period(<ClockPeriodFactor>)."
      "  Return previous <ClockPeriodFactor> for this device."
  );

  LogLevel ll = xtsc_get_constructor_log_level();
  XTSC_LOG(m_text, ll,        "Constructed xtsc_tlm22xttlm_transactor '" << name() << "':");
  XTSC_LOG(m_text, ll,        " num_ports               = "   << m_num_ports);
  XTSC_LOG(m_text, ll,        " byte_width              = "   << m_width8);
  XTSC_LOG(m_text, ll,        " enable_extensions       = "   << boolalpha << m_enable_extensions);
  XTSC_LOG(m_text, ll,        " exclusive_support       = "   << m_exclusive_support << " (" << exclusive_support(m_exclusive_support) << ")");
  if (!m_apb) {
  XTSC_LOG(m_text, ll,        " priority                = "   << m_priority);
  }
  XTSC_LOG(m_text, ll, hex << " pc                      = 0x" << m_pc);
  XTSC_LOG(m_text, ll,        " apb                     = "   << boolalpha << m_apb);
  XTSC_LOG(m_text, ll,        " axi                     = "   << boolalpha << m_axi);
  if (m_axi) {
  XTSC_LOG(m_text, ll,        " use_axi_fixed           = "   << boolalpha << m_use_axi_fixed);
  } else if (!m_apb && (m_width8 <= 16)) {
  XTSC_LOG(m_text, ll,        " use_pif_block           = "   << boolalpha << m_use_pif_block);
  XTSC_LOG(m_text, ll,        " use_pif_burst           = "   << boolalpha << m_use_pif_burst);
  if (m_use_pif_burst) {
  XTSC_LOG(m_text, ll,        " max_burst_beats         = "   << m_max_burst_beats);
  }
  }
  XTSC_LOG(m_text, ll,        " allow_dmi               = "   << boolalpha << m_allow_dmi);
  XTSC_LOG(m_text, ll,        " allow_transport_dbg     = "   << boolalpha << m_allow_transport_dbg);
  if (clock_period == 0xFFFFFFFF) {
  XTSC_LOG(m_text, ll, hex << " clock_period            = 0x" << clock_period << " (" << m_clock_period << ")");
  } else {
  XTSC_LOG(m_text, ll,        " clock_period            = "   << clock_period << " (" << m_clock_period << ")");
  }

  reset();
}



xtsc_component::xtsc_tlm22xttlm_transactor::~xtsc_tlm22xttlm_transactor(void) {
  XTSC_INFO(m_text, "transaction_info: Created=" << m_transaction_info_count << " Available=" << m_transaction_info_pool.size());
  XTSC_INFO(m_text, "xtsc_request:     Created=" << m_request_count          << " Available=" << m_request_pool.size());
}



u32 xtsc_component::xtsc_tlm22xttlm_transactor::get_bit_width(const string& port_name, u32 interface_num) const {
  return m_width8*8;
}



sc_object *xtsc_component::xtsc_tlm22xttlm_transactor::get_port(const string& port_name) {
  string name_portion;
  u32    index;
  bool   indexed = xtsc_parse_port_name(port_name, name_portion, index);

  if (((m_num_ports > 1) && !indexed) ||
      ( (name_portion != "m_request_ports")                          &&
        (name_portion != "m_respond_exports")                        &&
       ((name_portion != "m_target_sockets_4" ) || (m_width8 !=  4)) &&
       ((name_portion != "m_target_sockets_8" ) || (m_width8 !=  8)) &&
       ((name_portion != "m_target_sockets_16") || (m_width8 != 16)) &&
       ((name_portion != "m_target_sockets_32") || (m_width8 != 32)) &&
       ((name_portion != "m_target_sockets_64") || (m_width8 != 64))))
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

       if (name_portion == "m_request_ports")      return m_request_ports    [index];
  else if (name_portion == "m_respond_exports")    return m_respond_exports  [index];
  else if (name_portion == "m_target_sockets_4" )  return m_target_sockets_4 [index];
  else if (name_portion == "m_target_sockets_8" )  return m_target_sockets_8 [index];
  else if (name_portion == "m_target_sockets_16")  return m_target_sockets_16[index];
  else if (name_portion == "m_target_sockets_32")  return m_target_sockets_32[index];
  else if (name_portion == "m_target_sockets_64")  return m_target_sockets_64[index];
  else {
    throw xtsc_exception("Program Bug in xtsc_tlm22xttlm_transactor.cpp");
  }
}



xtsc_port_table xtsc_component::xtsc_tlm22xttlm_transactor::get_port_table(const string& port_table_name) const {
  xtsc_port_table table;

  if (port_table_name == "target_sockets") {
    for (u32 i=0; i<m_num_ports; ++i) {
      ostringstream oss;
      oss << "target_socket[" << i << "]";
      table.push_back(oss.str());
    }
    return table;
  }
  
  if (port_table_name == "master_ports") {
    for (u32 i=0; i<m_num_ports; ++i) {
      ostringstream oss;
      oss << "master_port[" << i << "]";
      table.push_back(oss.str());
    }
    return table;
  }

  string name_portion;
  u32    index;
  bool   indexed = xtsc_parse_port_name(port_table_name, name_portion, index);

  if (((m_num_ports > 1) && !indexed) || ((name_portion != "master_port") && (name_portion != "target_socket"))) {
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

  if (name_portion == "master_port") {
    oss.str(""); oss << "m_request_ports"   << "[" << index << "]"; table.push_back(oss.str());
    oss.str(""); oss << "m_respond_exports" << "[" << index << "]"; table.push_back(oss.str());
  }
  else {
    oss.str(""); oss << "m_target_sockets_" << m_width8 << "[" << index<< "]"; table.push_back(oss.str());
  }

  return table;
}



bool xtsc_component::xtsc_tlm22xttlm_transactor::allow_transport_dbg(bool allow) {
  XTSC_INFO(m_text, "allow_transport_dbg(" << allow << "), was " << m_allow_transport_dbg);
  bool prev_allow = m_allow_transport_dbg;
  m_allow_transport_dbg = allow;
  return prev_allow;
}



void xtsc_component::xtsc_tlm22xttlm_transactor::change_clock_period(u32 clock_period_factor) {
  m_clock_period = m_time_resolution * clock_period_factor;
  m_clock_period_value = m_clock_period.value();
  XTSC_INFO(m_text, "change_clock_period(" << clock_period_factor << ") => " << m_clock_period);
  compute_delays();
}



void xtsc_component::xtsc_tlm22xttlm_transactor::compute_delays() {
}



void xtsc_component::xtsc_tlm22xttlm_transactor::execute(const string&          cmd_line, 
                                                         const vector<string>&  words,
                                                         const vector<string>&  words_lc,
                                                         ostream&               result)
{
  ostringstream res;

  if (false) {
  }
  else if (words[0] == "allow_transport_dbg") {
    bool allow = xtsc_command_argtobool(cmd_line, words, 1);
    res << allow_transport_dbg(allow);
  }
  else if (words[0] == "change_clock_period") {
    u32 clock_period_factor = xtsc_command_argtou32(cmd_line, words, 1);
    res << m_clock_period_value;
    change_clock_period(clock_period_factor);
  }
  else {
    ostringstream oss;
    oss << name() << "::" << __FUNCTION__ << "() called for unknown command '" << cmd_line << "'.";
    throw xtsc_exception(oss.str());
  }

  result << res.str();
}



void xtsc_component::xtsc_tlm22xttlm_transactor::connect(xtsc_core& core, u32 tlm22xttlm_port) {
  if (tlm22xttlm_port >= m_num_ports) {
    ostringstream oss;
    oss << "Invalid tlm22xttlm_port=" << tlm22xttlm_port << " in connect(): " << endl;
    oss << kind() << " '" << name() << "' has " << m_num_ports << " ports numbered from 0 to " << m_num_ports-1 << endl;
    throw xtsc_exception(oss.str());
  }
  u32 width8 = core.get_memory_byte_width(xtsc_core::get_memory_port("PIF", &core));
  if (width8 != m_width8) {
    ostringstream oss;
    oss << "Memory interface data width mismatch: " << endl;
    oss << kind() << " '" << name() << "' is " << m_width8 << " bytes wide, but the inbound PIF interface of" << endl;
    oss << core.kind() << " '" << core.name() << "' is " << width8 << " bytes wide.";
    throw xtsc_exception(oss.str());
  }
  core.get_respond_port("inbound_pif")(*m_respond_exports[tlm22xttlm_port]);
  (*m_request_ports[tlm22xttlm_port])(core.get_request_export("inbound_pif"));
}



void xtsc_component::xtsc_tlm22xttlm_transactor::connect(xtsc_master_tlm2& master_tlm2, u32 target_socket) {
  if (target_socket >= m_num_ports) {
    ostringstream oss;
    oss << "Invalid target_socket=" << target_socket << " in connect(): " << endl;
    oss << kind() << " '" << name() << "' has " << m_num_ports << " sockets numbered from 0 to " << m_num_ports-1 << endl;
    throw xtsc_exception(oss.str());
  }

  if (master_tlm2.get_byte_width() != m_width8) {
    ostringstream oss;
    oss << "Mismatched byte widths in connect(): " << endl;
    oss << kind() << " '" << name() << "' has \"byte_width\"=" << m_width8 << endl;
    oss << master_tlm2.kind() << " '" << master_tlm2.name() << "' has \"byte_width\"=" << master_tlm2.get_byte_width() << endl;
    throw xtsc_exception(oss.str());
  }

  switch (m_width8) {
    case  4: master_tlm2.get_initiator_socket_4 ()(get_target_socket_4 (target_socket)); break;
    case  8: master_tlm2.get_initiator_socket_8 ()(get_target_socket_8 (target_socket)); break;
    case 16: master_tlm2.get_initiator_socket_16()(get_target_socket_16(target_socket)); break;
    case 32: master_tlm2.get_initiator_socket_32()(get_target_socket_32(target_socket)); break;
    case 64: master_tlm2.get_initiator_socket_64()(get_target_socket_64(target_socket)); break;
    default: throw xtsc_exception("Unhandled m_width8 in xtsc_tlm22xttlm_transactor.cpp");
  }

}



u32 xtsc_component::xtsc_tlm22xttlm_transactor::connect(xtsc_xttlm2tlm2_transactor&     xttlm2tlm2,
                                                        u32                             initiator_socket,
                                                        u32                             target_socket,
                                                        bool                            single_connect)
{
  u32 initiator_sockets = xttlm2tlm2.get_num_ports();

  if (initiator_socket >= initiator_sockets) {
    ostringstream oss;
    oss << "Invalid initiator_socket=" << initiator_socket << " in connect(): " << endl;
    oss << kind() << " '" << name() << "' has " << initiator_sockets << " sockets numbered from 0 to " << initiator_sockets-1 << endl;
    throw xtsc_exception(oss.str());
  }

  if (target_socket >= m_num_ports) {
    ostringstream oss;
    oss << "Invalid target_socket=" << target_socket << " in connect(): " << endl;
    oss << kind() << " '" << name() << "' has " << m_num_ports << " sockets numbered from 0 to " << m_num_ports-1 << endl;
    throw xtsc_exception(oss.str());
  }

  if (xttlm2tlm2.get_byte_width() != m_width8) {
    ostringstream oss;
    oss << "Mismatched byte widths in connect(): " << endl;
    oss << kind() << " '" << name() << "' has \"byte_width\"=" << m_width8 << endl;
    oss << xttlm2tlm2.kind() << " '" << xttlm2tlm2.name() << "' has \"byte_width\"=" << xttlm2tlm2.get_byte_width() << endl;
    throw xtsc_exception(oss.str());
  }

  u32 num_connected = 0;

  while ((initiator_socket < initiator_sockets) && (target_socket < m_num_ports)) {

    switch (m_width8) {
      case  4: xttlm2tlm2.get_initiator_socket_4 (initiator_socket)(get_target_socket_4 (target_socket)); break;
      case  8: xttlm2tlm2.get_initiator_socket_8 (initiator_socket)(get_target_socket_8 (target_socket)); break;
      case 16: xttlm2tlm2.get_initiator_socket_16(initiator_socket)(get_target_socket_16(target_socket)); break;
      case 32: xttlm2tlm2.get_initiator_socket_32(initiator_socket)(get_target_socket_32(target_socket)); break;
      case 64: xttlm2tlm2.get_initiator_socket_64(initiator_socket)(get_target_socket_64(target_socket)); break;
      default: throw xtsc_exception("Unhandled m_width8 in xtsc_tlm22xttlm_transactor.cpp");
    }

    target_socket    += 1;
    initiator_socket += 1;
    num_connected    += 1;

    if (single_connect) break;
    if (initiator_socket >= initiator_sockets) break;
    if (target_socket >= m_num_ports) break;
  
    if (((m_width8== 4) && get_target_socket_4 (target_socket).get_base_port().get_interface()) ||
        ((m_width8== 8) && get_target_socket_8 (target_socket).get_base_port().get_interface()) ||
        ((m_width8==16) && get_target_socket_16(target_socket).get_base_port().get_interface()) ||
        ((m_width8==32) && get_target_socket_32(target_socket).get_base_port().get_interface()) ||
        ((m_width8==64) && get_target_socket_64(target_socket).get_base_port().get_interface())) break;

    if (((m_width8== 4) && xttlm2tlm2.get_initiator_socket_4 (initiator_socket).get_base_port().get_interface()) ||
        ((m_width8== 8) && xttlm2tlm2.get_initiator_socket_8 (initiator_socket).get_base_port().get_interface()) ||
        ((m_width8==16) && xttlm2tlm2.get_initiator_socket_16(initiator_socket).get_base_port().get_interface()) ||
        ((m_width8==32) && xttlm2tlm2.get_initiator_socket_32(initiator_socket).get_base_port().get_interface()) ||
        ((m_width8==64) && xttlm2tlm2.get_initiator_socket_64(initiator_socket).get_base_port().get_interface())) break;

  }

  return num_connected;
}



void xtsc_component::xtsc_tlm22xttlm_transactor::reset(bool /* hard_reset */) {
  XTSC_INFO(m_text, kind() << "::reset()");

  m_next_port_num_worker    = 0;
  m_next_port_num_nb_thread = 0;

  for (u32 port_num=0; port_num<m_num_ports; ++port_num) {
    m_waiting_for_nacc           [port_num] = false;
    m_request_got_nacc           [port_num] = false;
    m_waiting_for_end_resp       [port_num] = false;
    m_next_id                    [port_num] = 0;
    m_outstanding_req_count      [port_num] = 0;
    m_tag_to_transaction_info_tab[port_num].clear();
    m_tag_to_request_tab         [port_num].clear();
    m_trans_to_info_map          [port_num].clear();

    m_worker_thread_event[port_num]->cancel();
    while (!m_pending_transaction_deque[port_num]->empty()) {
      transaction_info *p_transaction_info = m_pending_transaction_deque[port_num]->front();
      m_pending_transaction_deque[port_num]->pop_front();
      delete_transaction_info(p_transaction_info);
    }
  }

  xtsc_reset_processes(m_process_handles);
}



void xtsc_component::xtsc_tlm22xttlm_transactor::validate_port_and_width(u32 port_num, u32 width8) {

  if (port_num >= m_num_ports) {
    ostringstream oss;
    oss << " '" << name() << "': xtsc_tlm22xttlm_transactor::get_target_socket_" << width8<< "() called with port_num=" << port_num
        << " which is outside valid range of [0-" << (m_num_ports-1) << "]";
    throw xtsc_exception(oss.str());
  }

  if (width8 != m_width8) {
    ostringstream oss;
    oss << " '" << name() << "': xtsc_tlm22xttlm_transactor::get_target_socket_" << width8<< "() called but bus width is " << m_width8
        << " so you may only call get_target_socket_" << m_width8 << "()";
    throw xtsc_exception(oss.str());
  }

}



xtsc_component::xtsc_tlm22xttlm_transactor::target_socket_4&
xtsc_component::xtsc_tlm22xttlm_transactor::get_target_socket_4(u32 port_num) {
  validate_port_and_width(port_num, 4);
  return *m_target_sockets_4[port_num];
}



xtsc_component::xtsc_tlm22xttlm_transactor::target_socket_8&
xtsc_component::xtsc_tlm22xttlm_transactor::get_target_socket_8(u32 port_num) {
  validate_port_and_width(port_num, 8);
  return *m_target_sockets_8[port_num];
}



xtsc_component::xtsc_tlm22xttlm_transactor::target_socket_16&
xtsc_component::xtsc_tlm22xttlm_transactor::get_target_socket_16(u32 port_num) {
  validate_port_and_width(port_num, 16);
  return *m_target_sockets_16[port_num];
}



xtsc_component::xtsc_tlm22xttlm_transactor::target_socket_32&
xtsc_component::xtsc_tlm22xttlm_transactor::get_target_socket_32(u32 port_num) {
  validate_port_and_width(port_num, 32);
  return *m_target_sockets_32[port_num];
}



xtsc_component::xtsc_tlm22xttlm_transactor::target_socket_64&
xtsc_component::xtsc_tlm22xttlm_transactor::get_target_socket_64(u32 port_num) {
  validate_port_and_width(port_num, 64);
  return *m_target_sockets_64[port_num];
}



void xtsc_component::xtsc_tlm22xttlm_transactor::worker_thread(void) {

  // Get the port number for this "instance" of worker_thread
  u32 port_num = m_next_port_num_worker++;

  try {

    while (true) {
      XTSC_TRACE(m_text, "worker_thread[" << port_num << "] going to sleep.");
      wait(*m_worker_thread_event[port_num]);
      XTSC_TRACE(m_text, "worker_thread[" << port_num << "] woke up.");

      // Dispatch the xtsc_request objects in the m_requests vector of the transaction_info objects in m_pending_transaction_deque
      while (!m_pending_transaction_deque[port_num]->empty()) {

        transaction_info *p_transaction_info = m_pending_transaction_deque[port_num]->front();
        m_pending_transaction_deque[port_num]->pop_front();
        bool request_last_transfer           = false;
        bool request_first_transfer          = m_split_tlm2_phases ? (p_transaction_info->m_requests_sent == 0  ? true : false) : true;

        for (vector<xtsc_request*>::iterator i  = p_transaction_info->m_requests.begin()+p_transaction_info->m_requests_sent; 
                                             i != p_transaction_info->m_requests.end(); ++i) {
          XTSC_TRACE(m_text, "worker_thread[" << port_num << "] : request_first_transfer : " << request_first_transfer <<
                             " m_requests_sent : " << p_transaction_info->m_requests_sent <<
                             " m_requests.size : " << p_transaction_info->m_requests.size());
          xtsc_request *p_request = *i;
          u64  tag                =  p_request->get_tag();
          if (m_tag_to_transaction_info_tab[port_num].find(tag) == m_tag_to_transaction_info_tab[port_num].end()) {
            m_tag_to_transaction_info_tab[port_num][tag] = p_transaction_info;
          }
          else if (m_tag_to_transaction_info_tab[port_num][tag] != p_transaction_info) {
            ostringstream oss;
            oss << "PROGRAM BUG: p_transaction_info mismatch in " << __FILE__ << ":" << __LINE__;
            throw xtsc_exception(oss.str());
          }

          //If exclusive or TLM2 extension used, then keep using the same id from tlm2 extension as set in transport_helper
          if (!p_request->get_exclusive() && !m_enable_extensions) {
            p_request->set_id(m_next_id[port_num]);
          }
          if (request_first_transfer) {
            m_tag_to_request_tab[port_num][tag] = i - p_transaction_info->m_requests.begin();
          }

          // After a last_transfer, if outstanding requests are less than 64 (or wait a cycle if no ID is available)
          if (p_request->get_last_transfer()) {
            request_first_transfer = true;
            if (!p_request->get_exclusive() && !m_enable_extensions) {
              m_next_id[port_num] = (m_next_id[port_num]+1) %  64;
            }
            m_outstanding_req_count[port_num]++;
            if (m_outstanding_req_count[port_num] == 64) {
              XTSC_INFO(m_text, "worker_thread[" << port_num << "] has outstanding requests more than 64. Waiting.");
              while (m_outstanding_req_count[port_num] == 64)
                wait(m_clock_period);
            }
          }
          else {
            request_first_transfer = false;
          }

          // Send the request and wait 1 cycle for any potential RSP_NACC
          m_waiting_for_nacc[port_num] = true;
          XTSC_INFO(m_text, *p_request);
          do {
            m_request_got_nacc[port_num] = false;
            (*m_request_ports[port_num])->nb_request(*p_request);
            wait(m_clock_period);
          } while (m_request_got_nacc[port_num]);
          m_waiting_for_nacc[port_num] = false;

          p_transaction_info->m_requests_sent  += m_split_tlm2_phases ? 1 : 0;
          request_last_transfer                 = p_request->get_last_transfer();
          if (m_split_tlm2_phases)
            // With m_split_tlm2_phases, each incoming transfer is pushed as an entry into m_pending_transaction_deque 
            break;      
        }
        // With 'm_split_tlm2_phases', worker_thread is notified for each transfer per request(1), so 
        // request_last_transfer is used to check is all transfers are sent
        p_transaction_info->m_all_requests_sent = m_split_tlm2_phases ? (request_last_transfer ? true : false) : true;
        if (p_transaction_info->m_pending_last_rsps == 0) {
          if (p_transaction_info->m_blocking) {
            m_port_done_event[p_transaction_info->m_port_num]->notify(SC_ZERO_TIME);
          }
          else {
            respond_helper(p_transaction_info);
          }
        }
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



void xtsc_component::xtsc_tlm22xttlm_transactor::nb_thread(void) {

  // Get the port number for this "instance" of nb_thread
  u32 port_num = m_next_port_num_nb_thread++;

#if ((defined(SC_API_VERSION_STRING) && (SC_API_VERSION_STRING != sc_api_version_2_2_0)) || IEEE_1666_SYSTEMC >= 201101L)
  m_peq[port_num]->cancel_all();
#endif

  try {

    sc_event& our_peq_event     = m_peq[port_num]->get_event();
    sc_time   time              = SC_ZERO_TIME;

    while (true) {
      XTSC_TRACE(m_text, "nb_thread[" << port_num << "] going to sleep.");
      wait(our_peq_event);
      XTSC_TRACE(m_text, "nb_thread[" << port_num << "] woke up.");

      // Drain all transactions from m_peq that are due now
      while (tlm_generic_payload *p_trans = m_peq[port_num]->get_next_transaction()) { 

//#define CONVERT_TO_BLOCKING
#ifdef CONVERT_TO_BLOCKING
        // I don't think anyone wants this old way of handling nb_transport_fw by forwarding the 
        // requests to b_transport; but if they do the block of code could easily be re-enabled
        // based on a parameter instead of a compile-time flag.

        // Forward each transaction to b_transport() for handling 
        time = SC_ZERO_TIME;
        m_tlm_fw_transport_if_impl[port_num]->b_transport(*p_trans, time);

        // Send each transaction back to the TLM2 initiator.
        // We don't care about the tlm_sync_enum return value nor the gp response status.
        time = SC_ZERO_TIME;
        tlm_phase phase = BEGIN_RESP;
        XTSC_INFO(m_text, *p_trans << " " << xtsc_tlm_phase_text(phase) << " (" << time << ")" << " nb_transport_bw[" << port_num << "]");

             if (m_width8 ==  4) { (*m_target_sockets_4 [port_num])->nb_transport_bw(*p_trans, phase, time); }
        else if (m_width8 ==  8) { (*m_target_sockets_8 [port_num])->nb_transport_bw(*p_trans, phase, time); }
        else if (m_width8 == 16) { (*m_target_sockets_16[port_num])->nb_transport_bw(*p_trans, phase, time); }
        else if (m_width8 == 32) { (*m_target_sockets_32[port_num])->nb_transport_bw(*p_trans, phase, time); }
        else if (m_width8 == 64) { (*m_target_sockets_64[port_num])->nb_transport_bw(*p_trans, phase, time); }
        else { throw xtsc_exception("Unsupported m_width8 in nb_thread()"); }

#else

        time = SC_ZERO_TIME;
        transport_helper(*p_trans, time, false, port_num);
#endif

      }
    }

  }
  catch (const sc_unwind_exception& error) { throw; }
  catch (const exception& error) {
    ostringstream oss;
    oss << "std::exception caught in nb_thread[" << port_num << "] of " << kind() << " '" << name() << "'." << endl;
    oss << "what(): " << error.what() << endl;
    xtsc_log_multiline(m_text, FATAL_LOG_LEVEL, oss.str(), 2);
    throw;
  }

}



void xtsc_component::xtsc_tlm22xttlm_transactor::respond_helper(transaction_info *p_transaction_info) {
  // Send each transaction back to the TLM2 initiator.
  // We don't care about the tlm_sync_enum return value nor the gp response status.
  u32                  port_num = p_transaction_info->m_port_num;
  tlm_generic_payload *p_trans  = p_transaction_info->m_p_gp;
  sc_time              time     = SC_ZERO_TIME;
  tlm_phase            phase    = BEGIN_RESP;
  if (m_split_tlm2_phases  &&
      p_transaction_info->m_p_gp->get_command() == TLM_READ_COMMAND  && 
      (p_transaction_info->m_pending_last_rsps != 0  ||  !p_transaction_info->m_all_requests_sent)) {
    phase = BEGIN_RDATA;
  }
  p_transaction_info->m_phase = phase;

  XTSC_INFO(m_text, *p_trans << " " << xtsc_tlm_phase_text(phase) << " (" << time << ")" << " nb_transport_bw[" << port_num << "]");

       if (m_width8 ==  4) { (*m_target_sockets_4 [port_num])->nb_transport_bw(*p_trans, phase, time); }
  else if (m_width8 ==  8) { (*m_target_sockets_8 [port_num])->nb_transport_bw(*p_trans, phase, time); }
  else if (m_width8 == 16) { (*m_target_sockets_16[port_num])->nb_transport_bw(*p_trans, phase, time); }
  else if (m_width8 == 32) { (*m_target_sockets_32[port_num])->nb_transport_bw(*p_trans, phase, time); }
  else if (m_width8 == 64) { (*m_target_sockets_64[port_num])->nb_transport_bw(*p_trans, phase, time); }
  else { throw xtsc_exception("Unsupported m_width8 in xtsc_tlm22xttlm_transactor::respond_helper()"); }

  if (m_split_tlm2_phases && phase == BEGIN_RESP) {
    m_waiting_for_end_resp[port_num] = true;
  }

  if ((p_transaction_info->m_pending_last_rsps == 0) && p_transaction_info->m_all_requests_sent) {
    delete_transaction_info(p_transaction_info);
    if (m_trans_to_info_map[port_num].find(p_trans) != m_trans_to_info_map[port_num].end()) {
      m_trans_to_info_map[port_num].erase(p_trans);
    }
  }
}



void xtsc_component::xtsc_tlm22xttlm_transactor::transaction_info::init(tlm_generic_payload *p_gp, bool blocking, u32 port_num) {
  m_p_gp                = p_gp;
  m_blocking            = blocking;
  m_port_num            = port_num;
  m_pending_last_rsps   = 0;
  m_write_requests_rcvd = 0;
  m_requests_sent       = 0;
  m_requests_tag        = 0;
  m_all_requests_sent   = false;
  m_phase               = END_REQ;  // This transaction_info is not created before END_REQ phase
  if (m_p_gp->has_mm()) {
    m_p_gp->acquire();
  }
}



void xtsc_component::xtsc_tlm22xttlm_transactor::transaction_info::fini() {
  for (vector<xtsc_request*>::iterator i = m_requests.begin(); i != m_requests.end(); ++i) {
    m_transactor.delete_request(*i);
  }

  m_requests            .clear();
  m_read_data_offsets   .clear();
  m_read_data_sizes     .clear();

  if (m_p_gp->has_mm()) {
    m_p_gp->release();
  }
}



xtsc_component::xtsc_tlm22xttlm_transactor::transaction_info *
xtsc_component::xtsc_tlm22xttlm_transactor::new_transaction_info(tlm_generic_payload *p_gp, bool blocking, u32 port_num) {
  if (m_transaction_info_pool.empty()) {
    XTSC_DEBUG(m_text, "Creating a new transaction_info");
    m_transaction_info_count += 1;
    return new transaction_info(*this, p_gp, blocking, port_num);
  }
  else {
    transaction_info *p_transaction_info = m_transaction_info_pool.back();
    m_transaction_info_pool.pop_back();
    p_transaction_info->init(p_gp, blocking, port_num);
    return p_transaction_info;
  }
}



void xtsc_component::xtsc_tlm22xttlm_transactor::delete_transaction_info(transaction_info*& p_transaction_info) {
  p_transaction_info->fini();
  m_transaction_info_pool.push_back(p_transaction_info);
  p_transaction_info = 0;
}



xtsc_request *xtsc_component::xtsc_tlm22xttlm_transactor::new_request() {
  if (m_request_pool.empty()) {
    XTSC_DEBUG(m_text, "Creating a new xtsc_request");
    m_request_count += 1;
    return new xtsc_request();
  }
  else {
    xtsc_request *p_request = m_request_pool.back();
    m_request_pool.pop_back();
    return p_request;
  }
}



void xtsc_component::xtsc_tlm22xttlm_transactor::delete_request(xtsc_request*& p_request) {
  m_request_pool.push_back(p_request);
  p_request = 0;
}



bool xtsc_component::xtsc_tlm22xttlm_transactor::xtsc_respond_if_impl::nb_respond(const xtsc_response& response) {
  XTSC_DEBUG(m_transactor.m_text, response << " Port #" << m_port_num);

  if (response.get_status() == xtsc_response::RSP_NACC) {
    if (m_transactor.m_apb) {
      ostringstream oss;
      oss << m_transactor.kind() << " '" << m_transactor.name() << "' received nacc when configured with \"apb\" true: " << response << endl;
      throw xtsc_exception(oss.str());
    }
    else if (m_transactor.m_waiting_for_nacc[m_port_num]) {
      m_transactor.m_request_got_nacc[m_port_num] = true;
      return true;
    }
    else {
      ostringstream oss;
      oss << m_transactor.kind() << " '" << m_transactor.name() << "' received nacc too late: " << response << endl;
      oss << " - Possibly something is wrong with the downstream device" << endl;
      oss << " - Possibly this device's \"clock_period\" needs to be adjusted";
      throw xtsc_exception(oss.str());
    }
  }

  u64 tag = response.get_tag();

  transaction_info *p_transaction_info = m_transactor.m_tag_to_transaction_info_tab[m_port_num][tag];
  int               request_idx        = m_transactor.m_tag_to_request_tab[m_port_num][tag];

  if (!p_transaction_info) {
    ostringstream oss;
    oss << m_transactor.kind() << " '" << m_transactor.name() << "' received response with no corresponding request: tag=" << tag 
        << endl << response << " Port #" << m_port_num << endl;
    throw xtsc_exception(oss.str());
  }

  //Adhere to phase sequence
  if (m_transactor.m_split_tlm2_phases) {
    if (p_transaction_info->m_phase == BEGIN_RDATA  || m_transactor.m_waiting_for_end_resp[m_port_num]) {
      XTSC_INFO(m_transactor.m_text, response << " (Rejecting response, previous response pending at TLM2 initiator!" 
                                              << " Port #" << m_port_num);
      return false;   
    }
  }

  bool send_tlm2_beat     = false;
  xtsc_request *p_request = p_transaction_info->m_requests[request_idx];

  if (!p_request) {
    ostringstream oss;
    oss << "PROGRAM BUG: Can't find request in " << __FILE__ << ":" << __LINE__ 
        << endl << m_transactor.kind() << " '" << m_transactor.name() << "' received response with no corresponding request: tag=" << tag 
        << endl << response << " Port #" << m_port_num;
    throw xtsc_exception(oss.str());
  }

  if (response.get_status() != xtsc_response::RSP_OK) {
    if (response.get_status() == xtsc_response::RSP_DATA_ERROR) {
      p_transaction_info->m_p_gp->set_response_status(TLM_GENERIC_ERROR_RESPONSE);
    }
    else {
      p_transaction_info->m_p_gp->set_response_status(TLM_ADDRESS_ERROR_RESPONSE);
    }
  }
  else if (p_transaction_info->m_p_gp->get_command() == TLM_READ_COMMAND) {
    // Move read data from response buffer to TLM2 generic payload data array 
    u8  *bep     = p_transaction_info->m_p_gp->get_byte_enable_ptr();
    u32  bel     = p_transaction_info->m_p_gp->get_byte_enable_length();
    u8   enabled = TLM_BYTE_ENABLED;
    if (!bep) {
      bep = &enabled;
      bel = 1;
    }

    const u8   *buf     = response.get_buffer();
    u8         *ptr     = p_transaction_info->m_p_gp->get_data_ptr();
    u32         offset  = p_transaction_info->m_read_data_offsets[request_idx];
    u32         size    = p_transaction_info->m_read_data_sizes[request_idx];

    for (u32 i=0; i < size; ++i) {
      if (bep[i % bel] == enabled) {
        ptr[offset + i] = buf[i];
      }
    }

    // Adjust offset for each response from single BLOCK_READ|BURST_READ request
    p_transaction_info->m_read_data_offsets[request_idx] = offset + size;

    if (m_transactor.m_split_tlm2_phases) {
      XTSC_TRACE(m_transactor.m_text, "Read data, offset + size : " << offset + size); 
      send_tlm2_beat          = (((offset + size) % m_transactor.m_width8) == 0);
    }
  }

  if (response.get_last_transfer()) {
    if (m_transactor.m_exclusive_support == 1) {
      if (response.get_status() == xtsc_response::RSP_OK  && response.get_exclusive_ok()) {
        exclusive_request *ext1 = nullptr;
        transaction_id    *ext2 = nullptr;
        p_transaction_info->m_p_gp->get_extension(ext1);
        p_transaction_info->m_p_gp->get_extension(ext2);
        if (ext1 && ext2) {
          exclusive_response* ext = new exclusive_response;
          p_transaction_info->m_p_gp->set_extension(ext);  
          XTSC_DEBUG(m_transactor.m_text, "Setting extension : exclusive_response");
          ext->value = true;
        }
      }
    }
    m_transactor.m_tag_to_transaction_info_tab[m_port_num][tag] = NULL;
    p_transaction_info->m_pending_last_rsps -= 1;
    m_transactor.m_outstanding_req_count[m_port_num]--;
    if ((p_transaction_info->m_pending_last_rsps == 0) && p_transaction_info->m_all_requests_sent) {
      if (p_transaction_info->m_blocking) {
        m_transactor.m_port_done_event[p_transaction_info->m_port_num]->notify(SC_ZERO_TIME);
      }
      else {
        m_transactor.respond_helper(p_transaction_info);
      }
    }
  }
  else if (send_tlm2_beat) {
    XTSC_TRACE(m_transactor.m_text, "Sending BEGIN_RDATA");
    m_transactor.respond_helper(p_transaction_info);
  }

  return true;
}



void xtsc_component::xtsc_tlm22xttlm_transactor::xtsc_respond_if_impl::register_port(sc_port_base& port, const char *if_typename) {
  if (m_p_port) {
    ostringstream oss;
    oss << "Illegal multiple binding detected to xtsc_tlm22xttlm_transactor '" << m_transactor.name() << "' " << name() << ": " << endl;
    oss << "  " << port.name() << endl;
    oss << "  " << m_p_port->name();
    throw xtsc_exception(oss.str());
  }
  XTSC_INFO(m_transactor.m_text, "Binding '" << port.name() << "' to xtsc_tlm22xttlm_transactor::" << name() <<
                                 " Port #" << m_port_num);
  m_p_port = &port;
}



tlm_sync_enum xtsc_component::xtsc_tlm22xttlm_transactor::tlm_fw_transport_if_impl::nb_transport_fw(tlm_generic_payload&  trans,
                                                                                                    tlm_phase&            phase,
                                                                                                    sc_time&              t)
{
  XTSC_INFO(m_transactor.m_text, trans << " " << xtsc_tlm_phase_text(phase) << " (" << t << ")" << " nb_transport_fw[" << m_port_num << "]");

  transaction_info *p_transaction_info = NULL;
  if (m_transactor.m_trans_to_info_map[m_port_num].find(&trans) != m_transactor.m_trans_to_info_map[m_port_num].end()) {
    p_transaction_info  = m_transactor.m_trans_to_info_map[m_port_num][&trans];
  }

  if (phase == BEGIN_REQ) {
    if (!m_transactor.m_split_tlm2_phases || trans.get_command() == TLM_READ_COMMAND) {
      m_transactor.m_peq[m_port_num]->notify(trans, t);
    }
    phase = END_REQ;
    return TLM_UPDATED;
  }
  else if (m_transactor.m_split_tlm2_phases && (phase == BEGIN_WDATA || phase == BEGIN_WDATA_LAST)) {
    m_transactor.m_peq[m_port_num]->notify(trans, t);
    if (phase == BEGIN_WDATA_LAST) {
      phase = END_WDATA_LAST;
    } else {
      phase = END_WDATA;
    }
    if (p_transaction_info != NULL) {
      p_transaction_info->m_phase = phase;
    }
    return TLM_UPDATED;
  }
  else if (m_transactor.m_split_tlm2_phases && phase == END_RDATA) {
    if (p_transaction_info != NULL) {
      p_transaction_info->m_phase = phase;
    }
    return TLM_ACCEPTED;
  }
  else if (phase == END_RESP) {
    if (m_transactor.m_split_tlm2_phases) {
      m_transactor.m_waiting_for_end_resp[m_port_num] = false;
    }
    return TLM_COMPLETED;
  }
  else if ((phase == END_REQ) || (phase == BEGIN_RESP) || 
           (m_transactor.m_split_tlm2_phases && 
            (phase == END_RDATA) || (phase == END_WDATA) || (phase == END_WDATA_LAST))) {
    // TLM-2.0.1 LRM, para 8.2.3.c, page 105; also para 8.2.13.4.c, page 124
    ostringstream oss;
    oss << m_transactor.kind() << " '" << m_transactor.name() << "': nb_transport_fw received illegal phase=" << phase
        << " for trans: " << trans;
    throw xtsc_exception(oss.str());
  }
  else {
    // TLM-2.0.1 LRM, para 8.2.5.b, page 110; also para 8.2.13.4.c, page 124
    XTSC_INFO(m_transactor.m_text, trans << " " << phase << " " << t << " nb_transport_fw[" << m_port_num << "] (Ignored)");
    if (p_transaction_info != NULL) {
      p_transaction_info->m_phase = phase;
    }
    return TLM_ACCEPTED;
  }
}



static xtsc_byte_enables get_beat_byte_enables(u32 offset, u32 length, u8 *bep, u32 bel) {
  xtsc_byte_enables be = 0;
  xtsc_byte_enables mask = 1ull;
  for (u32 i=0; i<length; ++i) {
    if (bep[(offset + i) % bel] == TLM_BYTE_ENABLED) {
      be |= mask;
    }
    mask <<= 1;
  }
  return be;
}


bool xtsc_component::xtsc_tlm22xttlm_transactor::check_exclusive(tlm_generic_payload& trans, u32 port_num) {
  XTSC_TRACE(m_text, __FUNCTION__ << "(" << trans <<  ", " << port_num << ")");
  if (m_exclusive_support == 1) {
    u64                 addr            = trans.get_address();
    u32                 len             = trans.get_data_length();
    u32                 num_xfers       = (len + m_width8 - 1) / m_width8;
    exclusive_request  *ext1            = nullptr;
    transaction_id     *ext2            = nullptr;

    trans.get_extension(ext1);  // Retrieve the extension 
    trans.get_extension(ext2);  // Retrieve the extension 
    if (ext1 && ext2) {
      XTSC_DEBUG(m_text, "Exclusive request received with transaction id=" << ext2->value);

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

      return true;
    }
  }
  return false;
}


bool xtsc_component::xtsc_tlm22xttlm_transactor::transport_helper(tlm_generic_payload& trans, sc_time& t, bool blocking, u32 port_num) {
  XTSC_TRACE(m_text, __FUNCTION__ << "(" << trans << ", " << t << ", " << blocking << ", " << port_num << ")");

  tlm_command   cmd     = trans.get_command();
  u64           adr     = trans.get_address();
  u8           *ptr     = trans.get_data_ptr();
  u32           len     = trans.get_data_length();
  u32           sw      = trans.get_streaming_width();
  u8           *bep     = trans.get_byte_enable_ptr();
  u32           bel     = trans.get_byte_enable_length();
  bool          read    = (cmd == TLM_READ_COMMAND);
  bool          write   = (cmd == TLM_WRITE_COMMAND);

  if ((cmd == TLM_IGNORE_COMMAND) || (len == 0)) {
    XTSC_INFO(m_text, "Port #" << port_num << ": " << __FUNCTION__ << ":   " << trans << ": returning TLM_OK_RESPONSE");
    trans.set_response_status(TLM_OK_RESPONSE);
    return true;
  }

  sc_time beg_time = sc_time_stamp();

  if (!read && !write) {
    ostringstream oss;
    oss << "Port #" << port_num << ": Unsupported command received in " << __FUNCTION__ << "() of " << kind() << " '"
        << name() << "': " << trans;
    throw xtsc_exception(oss.str());
  }

  if (m_apb && (len > 4)) {
    ostringstream oss;
    oss << "Port #" << port_num << ": Data length (" << len << ") cannot exceed 4 when \"apb\" is true in " << kind() << " '"
        << name() << "': " << trans;
    throw xtsc_exception(oss.str());
  }

  if (!m_axi && (sw < len)) {
    ostringstream oss;
    oss << "Port #" << port_num << ": Streaming transfers are not supported for non-AXI in " << __FUNCTION__ << "() of " << kind() << " '"
        << name() << "': " << trans;
    throw xtsc_exception(oss.str());
  }

  bool excl = check_exclusive(trans, port_num);

  trans.set_response_status(TLM_OK_RESPONSE);
  transaction_info *p_transaction_info = NULL;
  if (m_split_tlm2_phases) {
    if (m_trans_to_info_map[port_num].find(&trans) != m_trans_to_info_map[port_num].end()) {
      p_transaction_info  = m_trans_to_info_map[port_num][&trans];
    }
    else {
      p_transaction_info                    = new_transaction_info(&trans, blocking, port_num);
      m_trans_to_info_map[port_num][&trans] = p_transaction_info;
    }
  }
  else {
    p_transaction_info = new_transaction_info(&trans, blocking, port_num);
  }

  u8 enabled = TLM_BYTE_ENABLED;
  if (!bep) {
    bep = &enabled;
    bel = 1;
  }

  u32   num_xfers                         = (len + m_width8 - 1) / m_width8;
  bool  use_axi_incr                      = false;
  bool  use_axi_narrow                    = false;
  bool  use_axi_fixed                     = false;
  bool  use_pif_block                     = false;
  bool  use_pif_burst                     = false;
  bool  use_multiple_full_width_transfers = false;
  bool  use_single_non_axi_transfer       = false;
  bool  all_byte_lanes_enabled            = true;

  for (u32 i=0; i<len && i<bel; ++i) {
    if (bep[i] != TLM_BYTE_ENABLED) {
      all_byte_lanes_enabled = false;
      break;
    }
  }

  xtsc_address addr = (xtsc_address) adr;
  xtsc_address end  = addr + len - 1;

  if (m_apb) {
    use_single_non_axi_transfer = true;
  }
  else if (m_axi) {
    // This is AXI, should we use FIXED or INCR?
    if (m_use_axi_fixed && (sw < len) && (sw <= m_width8)) {
      // Make sure either 1) sw is bus-width and addr is bus-width aligned 
      //           or     2) the bytes implied by addr and sw are all on the same bus-width chunk
      if (((sw == m_width8) && ((addr % m_width8) == 0)) || (((addr + sw - 1) / m_width8) == (addr / m_width8))) {
        // len must be an integer multiple of sw and at most 16 beats are allowed
        u32 xfers = len / sw;
        if ((xfers * sw == len) && (xfers <= 16)) {
          use_axi_fixed = true;
          num_xfers = xfers;
        }
      }
    }
    if (!use_axi_fixed && (sw >= len)) {
      num_xfers = (end / m_width8) - (addr / m_width8) + 1;
      u32 alignment = addr % m_width8;
      if ((num_xfers > 1) && (alignment == 2 || alignment == 4 || alignment == 8 || alignment == 16)) {
        num_xfers = (end - addr + 1) / alignment;
        use_axi_narrow = true;
      }
      use_axi_incr = true;
    }
    // Safety checks to fall back to doing 1 transaction per byte:
    bool read_with_disable_bytes = (read && !all_byte_lanes_enabled);           // AXI has no read byte enables
    bool too_many_beats          = (num_xfers > 256);                           // INCR has 256 beat limit
    bool crosses_4KB             = ((end / 4096) > (addr / 4096));              // No transaction can cross a 4KB (4096=0x1000) boundary
    bool crosses_bus_width       = ((end / m_width8) > (addr / m_width8));
    u32  alignment               =  addr % m_width8;
    if (read_with_disable_bytes || too_many_beats || crosses_4KB || (alignment == 1 && !use_axi_fixed)) {
      use_axi_incr   = false;
      use_axi_fixed  = false;
      use_axi_narrow = false;
      num_xfers      = len;
    }
  }
  else {
    // Not APB/AXI
    // Should we use BLOCK_READ|BLOCK_WRITE?
    if (m_use_pif_block && ((num_xfers * m_width8) == len)) {
      // Check for supported block size
      if ((num_xfers == 2) || (num_xfers == 4) || (num_xfers == 8) || (num_xfers == 16)) {
        // Check for address aligned to block size
        if (!(addr & (num_xfers * m_width8 - 1))) {
          use_pif_block = true;
          // Check for supported byte enable pattern
          u32 i     = m_split_tlm2_phases ? p_transaction_info->m_write_requests_rcvd * m_width8 : 0;
          u32 count = m_split_tlm2_phases ? m_width8 : len;
          for (u32 i=0; i<count; ++i) {
            if (bep[i % bel] != TLM_BYTE_ENABLED) {
              use_pif_block = false;
              break;
            }
          }
        }
      }
    }
    // Should we use BURST_READ|BURST_WRITE?
    if (m_use_pif_burst && !use_pif_block) {
      // Check for supported burst size (integer multiple of bus width between 2 and m_max_burst_beats, inclusive)
      if ((num_xfers >= 2) && (num_xfers <= m_max_burst_beats) && ((len & (m_width8 - 1)) == 0)) {
        // Check for supported address alignment: bus width for read or write
        if (!(addr & (m_width8 - 1))) {
          // Check for byte enable pattern supported by BURST_READ|BURST_WRITE
          u32 change_cnt = 0;
          u8  prev_en = 0x00;
          bool first_change_ok  = false;
          bool second_change_ok = true;
          u32 i     = m_split_tlm2_phases ? p_transaction_info->m_write_requests_rcvd * m_width8 : 0;
          u32 count = m_split_tlm2_phases ? m_width8 : len;
          for (u32 i=0; i<count; ++i) {
            u8 en = bep[i % bel];
            if (en != prev_en) {
              change_cnt += 1;
              if (change_cnt == 1) {
                if (i < m_width8) {
                  first_change_ok = true;
                }
                else {
                  break;
                }
              }
              else if (change_cnt == 2) {
                if (i <= len - m_width8) {
                  second_change_ok = false;
                  break;
                }
              }
              else {
                break;
              }
              prev_en = en;
            }
          }
          if (first_change_ok && second_change_ok && (change_cnt <= 2)) {
            use_pif_burst = true;
          }
        }
      }
    }
    // Should we use multiple single transfers (READ|WRITE), each of a full bus width 
    if ((num_xfers > 1) && !use_pif_block && !use_pif_burst && ((len & (m_width8 - 1)) == 0) && all_byte_lanes_enabled) {
      // Starting address be bus-width aligned
      if ((addr % m_width8) == 0) {
        use_multiple_full_width_transfers = true;
      }
    }
    // Is this a single transfer with valid PIF byte-enables?
    if ((len <= m_width8)) {
      // Check for byte enable pattern supported by PIF
      u32 change_cnt   = 0;
      u32 first_en_pos = 0;
      u32 last_en_pos  = 0;
      u8  prev_en      = 0x00;
      for (u32 i=0; i<len; ++i) {
        u8 en = bep[i % bel];
        if (en != prev_en) {
          change_cnt += 1;
          if (en == TLM_BYTE_ENABLED) last_en_pos = i;
          if (change_cnt == 1) {
            first_en_pos = i;
          }
          else if (change_cnt > 2) {
            break;
          }
          prev_en = en;
        }
      }
      if ((change_cnt >= 1) && (change_cnt <= 2)) {
        u32 size = last_en_pos - first_en_pos + 1;
        // size must be a power of 2
        if ((size == 1) || (size == 2) || (size == 4) || (size == 8) || (size == 16)) {
          xtsc_address effective_addr = addr + first_en_pos;   // Address of first enabled byte
          // effective address must be size aligned
          if (!(effective_addr & (size - 1))) {
            use_single_non_axi_transfer = true;
          }
        }
      }
    }
  }
  XTSC_TRACE(m_text, "num_xfers : " << num_xfers << ", use_axi_incr : " << use_axi_incr << ", use_axi_narrow : " << use_axi_narrow << 
                     ", use_axi_fixed : " << use_axi_fixed << ", use_pif_block : " << use_pif_block << ", use_pif_burst : " << use_pif_burst); 
  XTSC_TRACE(m_text, "use_multiple_full_width_transfers : " << use_multiple_full_width_transfers << ", use_single_non_axi_transfer : " 
                      << use_single_non_axi_transfer << ", all_byte_lanes_enabled " << all_byte_lanes_enabled);

  xtsc_byte_enables be        = (1 << m_width8) - 1;    // Enable all byte lanes
  u32               route_id  = 0;
  u8                id        = 64;                     // Will get a real one applied in worker_thread
  u8                priority  = m_priority;
  xtsc_address      pc        = m_pc;

  if (m_enable_extensions) {
    //Get supported extensions here.
    transaction_id *ext = nullptr;
    trans.get_extension(ext);
    if (ext) {
      XTSC_DEBUG(m_text, "Get extension 'transaction_id' with value : " << ext->value);
      id = ext->value & 0xFF;
      route_id = ext->value >> 8;
    } else {
      XTSC_WARN(m_text, "Extension 'transaction_id' not found!");
    }
  }

  if (excl & !m_enable_extensions) {
    transaction_id *ext = nullptr;
    trans.get_extension(ext);
    if (ext) {
      XTSC_DEBUG(m_text, "Get extension 'transaction_id' with value : " << ext->value);
      id = ext->value & 0xFF;
      route_id = ext->value >> 8;
    }
  }

  if (m_axi) {
    xtsc_request::type_t type = (read ? xtsc_request::BURST_READ : xtsc_request::BURST_WRITE);
    if (use_axi_fixed) {
      if (read) {
        xtsc_request *p_request = new_request();
        p_request->initialize(type, addr, sw, 0, num_xfers, be, true, route_id, id, priority, pc, xtsc_request::FIXED);
        p_request->set_exclusive(excl);
        p_transaction_info->m_requests.push_back(p_request);
        p_transaction_info->m_read_data_offsets .push_back(0);
        p_transaction_info->m_read_data_sizes   .push_back(sw);
        p_transaction_info->m_pending_last_rsps += 1;
      }
      else {
        if (m_split_tlm2_phases) {
          u32  beat_index = p_transaction_info->m_write_requests_rcvd;
          u32  offset     = sw * beat_index;
          bool first      = (beat_index == 0);
          bool last       = (num_xfers-1 == beat_index);
          be              = get_beat_byte_enables(offset, sw, bep, bel);
          xtsc_request *p_request = new_request();
          if (first) {
            p_request->initialize(type, addr, sw, 0, num_xfers, be, last, route_id, id, priority, pc, xtsc_request::FIXED);
            p_transaction_info->m_requests_tag       = p_request->get_tag();
            p_transaction_info->m_pending_last_rsps += 1;
          }
          else {
            u64 tag  = p_transaction_info->m_requests_tag;
            p_request->initialize(addr, tag, addr+(xtsc_address)offset, sw, num_xfers, beat_index+1, be, route_id, id, priority, pc, xtsc_request::FIXED);
          }
          p_request->set_buffer(sw, &ptr[offset]);
          p_request->set_exclusive(excl);
          p_transaction_info->m_requests.push_back(p_request);
          p_transaction_info->m_write_requests_rcvd += 1;
          if (!last) {
            trans.set_response_status(TLM_INCOMPLETE_RESPONSE);
          }
        }
        else { // m_split_tlm2_phases=false
          be = get_beat_byte_enables(0, sw, bep, bel);
          xtsc_request *p_request = new_request();
          bool last = (num_xfers == 1);
          p_request->initialize(type, addr, sw, 0, num_xfers, be, last, route_id, id, priority, pc, xtsc_request::FIXED);
          p_request->set_buffer(sw, ptr);
          p_request->set_exclusive(excl);
          p_transaction_info->m_requests.push_back(p_request);
          u64 tag = p_request->get_tag();
          for (u32 i=1; i<num_xfers; ++i) {
            xtsc_request *p_request = new_request();
            u32 offset = sw * i;
            be = get_beat_byte_enables(offset, sw, bep, bel);
            p_request->initialize(addr, tag, addr, sw, num_xfers, i+1, be, route_id, id, priority, pc, xtsc_request::FIXED);
            p_request->set_buffer(sw, &ptr[offset]);
            p_request->set_exclusive(excl);
            p_transaction_info->m_requests.push_back(p_request);
          }
          p_transaction_info->m_pending_last_rsps += 1;
        }
      }
    }
    else if (use_axi_incr) {
      u32 size = ((len < m_width8) ? len : (use_axi_narrow ? addr % m_width8 : m_width8));
      if (use_axi_narrow && len > m_width8) {
        XTSC_DEBUG(m_text, trans << " (Using narrow beats of RD_INCR|WR_INCR)");
      }
      if (read) {
        xtsc_request *p_request = new_request();
        p_request->initialize(type, addr, size, 0, num_xfers, be, true, route_id, id, priority, pc, xtsc_request::INCR);
        p_request->set_exclusive(excl);
        p_transaction_info->m_requests.push_back(p_request);
        p_transaction_info->m_read_data_offsets .push_back(0);
        p_transaction_info->m_read_data_sizes   .push_back(size);
        p_transaction_info->m_pending_last_rsps += 1;
      }
      else {
        if (m_split_tlm2_phases) {
          u32  beat_index                = p_transaction_info->m_write_requests_rcvd;
          u32  num_xfers_per_recvd_beat  = (len < m_width8) ? 1 : (use_axi_narrow ? m_width8 / size : 1);
          bool first                     = (beat_index == 0);
          for (u32 i = 0; i < num_xfers_per_recvd_beat ; i++ ) {
            xtsc_request *p_request = new_request();
            u32  offset             = size * beat_index + (i * size);
            bool last               = (num_xfers_per_recvd_beat * beat_index + i == num_xfers-1);
            be                      = get_beat_byte_enables(offset, size, bep, bel);
            if (first) {
              p_request->initialize(type, addr, size, 0, num_xfers, be, last, route_id, id, priority, pc, xtsc_request::INCR);
              p_transaction_info->m_requests_tag       = p_request->get_tag();
              p_transaction_info->m_pending_last_rsps += 1;
              first    = false;
            }
            else {
              u64 tag  = p_transaction_info->m_requests_tag;
              p_request->initialize(addr, tag, addr+(xtsc_address)(offset), size, num_xfers, num_xfers_per_recvd_beat*beat_index+1+i, 
                                    be, route_id, id, priority, pc, xtsc_request::INCR);
            }
            p_request->set_buffer(size, &ptr[offset]);
            p_request->set_exclusive(excl);
            p_transaction_info->m_requests.push_back(p_request);
            if (!last) {
              trans.set_response_status(TLM_INCOMPLETE_RESPONSE);
            }
            XTSC_TRACE(m_text, "__FUNCTION__ : beat_index : " <<  beat_index <<  " offset : " << offset << " last : " << last << 
                               " i : " << i << " num_xfers_per_recvd_beat : " << num_xfers_per_recvd_beat << " p_request : " << *p_request);

            if (num_xfers_per_recvd_beat > 1 && i < num_xfers_per_recvd_beat-1) {
              // With narrow transfers, additional entries need to be pushed into m_pending_transaction_deque
              m_pending_transaction_deque[port_num]->push_back(p_transaction_info);
            }

          }
          p_transaction_info->m_write_requests_rcvd += 1;
        }
        else { // m_split_tlm2_phases=false
          be = get_beat_byte_enables(0, size, bep, bel);
          xtsc_request *p_request = new_request();
          bool last = (num_xfers == 1);
          p_request->initialize(type, addr, size, 0, num_xfers, be, last, route_id, id, priority, pc, xtsc_request::INCR);
          p_request->set_buffer(size, ptr);
          p_request->set_exclusive(excl);
          p_transaction_info->m_requests.push_back(p_request);
          u64 tag = p_request->get_tag();
          for (u32 i=1; i<num_xfers; ++i) {
            xtsc_request *p_request = new_request();
            u32 offset = size * i;
            be = get_beat_byte_enables(offset, size, bep, bel);
            p_request->initialize(addr, tag, addr+(size*i), size, num_xfers, i+1, be, route_id, id, priority, pc, xtsc_request::INCR);
            p_request->set_buffer(size, &ptr[offset]);
            p_request->set_exclusive(excl);
            p_transaction_info->m_requests.push_back(p_request);
          }
          p_transaction_info->m_pending_last_rsps += 1;
        }
      }
    }
    else {
      // One byte at a time
      XTSC_INFO(m_text, trans << " (Using 1 BURST_READ|BURST_WRITE per byte)");
      be = 1;
      for (u32 i=0; i < len; ++i) {
        if (bep[i % bel] == enabled) {
          xtsc_request *p_request = new_request();
          p_request->initialize(type, addr, 1, 0, 1, be, true, route_id, id, priority, pc, xtsc_request::INCR);
          p_request->set_exclusive(excl);
          if (read) {
            p_transaction_info->m_read_data_offsets .push_back(i);
            p_transaction_info->m_read_data_sizes   .push_back(1);
          }
          else {
            p_request->set_buffer(1, &ptr[i]);
          }
          p_transaction_info->m_requests.push_back(p_request);
          p_transaction_info->m_pending_last_rsps += 1;
        }
        addr += 1;
        if (((i + 1) % sw) == 0) {
          addr = (xtsc_address) adr;
        }
      }
    }
  }
  else if (use_single_non_axi_transfer) {
    xtsc_request     *p_request = new_request();
    be = get_beat_byte_enables(0, len, bep, bel);
    if (read) {
      p_request->initialize(xtsc_request::READ,  addr, len, 0, 1, be, true, route_id, id, priority, pc, xtsc_request::NON_AXI, m_apb);
    }
    else {
      p_request->initialize(xtsc_request::WRITE, addr, len, 0, 1, be, true, route_id, id, priority, pc, xtsc_request::NON_AXI, m_apb);
      p_request->set_buffer(len, ptr);
    }
    p_request->set_exclusive(excl);
    p_transaction_info->m_requests.push_back(p_request);
    if (read) {
      p_transaction_info->m_read_data_offsets .push_back(0);
      p_transaction_info->m_read_data_sizes   .push_back(len);
    }
    p_transaction_info->m_pending_last_rsps += 1;
  }
  else if (read && (use_pif_block || use_pif_burst)) {
    xtsc_request::type_t type = (use_pif_block ? xtsc_request::BLOCK_READ : xtsc_request::BURST_READ);
    xtsc_request *p_request = new_request();
    p_request->initialize(type, addr, m_width8, 0, num_xfers, be, true, route_id, id, priority, pc);
    p_request->set_exclusive(excl);
    p_transaction_info->m_requests.push_back(p_request);
    p_transaction_info->m_read_data_offsets .push_back(0);
    p_transaction_info->m_read_data_sizes   .push_back(m_width8);
    p_transaction_info->m_pending_last_rsps += 1;
  }
  else if (write && use_pif_block) {
    if (m_split_tlm2_phases) {
      u32  beat_index = p_transaction_info->m_write_requests_rcvd;
      u32  offset     = m_width8 * beat_index;
      bool first      = (beat_index == 0);
      bool last       = (num_xfers-1 == beat_index);
      xtsc_request *p_request = new_request();
      if (first) {
        p_request->initialize(xtsc_request::BLOCK_WRITE, addr, m_width8, 0, num_xfers, be, last, route_id, id, priority, pc);
        p_transaction_info->m_requests_tag       = p_request->get_tag();
        p_transaction_info->m_pending_last_rsps += 1;
      }
      else {
        u64 tag  = p_transaction_info->m_requests_tag;
        p_request->initialize(tag, addr+(xtsc_address)offset, m_width8, num_xfers, last, route_id, id, priority, pc);
      }
      p_request->set_buffer(m_width8, &ptr[offset]);
      p_request->set_exclusive(excl);
      p_transaction_info->m_requests.push_back(p_request);
      p_transaction_info->m_write_requests_rcvd += 1;
      if (!last) {
        trans.set_response_status(TLM_INCOMPLETE_RESPONSE);
      }
    }
    else { // m_split_tlm2_phases=false
      xtsc_request::type_t type = xtsc_request::BLOCK_WRITE;
      xtsc_request *p_request = new_request();
      p_request->initialize(type, addr, m_width8, 0, num_xfers, be, false, route_id, id, priority, pc);
      p_request->set_buffer(m_width8, ptr);
      p_request->set_exclusive(excl);
      p_transaction_info->m_requests.push_back(p_request);
      u64 tag = p_request->get_tag();
      for (u32 i=1; i<num_xfers; ++i) {
        xtsc_request *p_request = new_request();
        u32 offset = m_width8 * i;
        bool last = (i == (num_xfers - 1));
        p_request->initialize(tag, addr + offset, m_width8, num_xfers, last, route_id, id, priority, pc);
        p_request->set_buffer(m_width8, &ptr[offset]);
        p_request->set_exclusive(excl);
        p_transaction_info->m_requests.push_back(p_request);
      }
      p_transaction_info->m_pending_last_rsps += 1;
    }
  }
  else if (write && use_pif_burst) {
    if (m_split_tlm2_phases) {
      u32           beat_index = p_transaction_info->m_write_requests_rcvd;
      u32           offset     = m_width8 * beat_index;
      bool          first      = (beat_index == 0);
      bool          last       = (num_xfers-1 == beat_index);
      xtsc_address  hw_addr    = addr;
      xtsc_request *p_request  = new_request();
      be                       = (1 << m_width8) - 1;
      for (u32 i=0; i<m_width8; ++i) {
        if (bep[i % bel] == TLM_BYTE_ENABLED) {
          hw_addr = addr + i;
          break;
        }
        be ^= (1ull << i);
      }
      if (first) {
        p_request->initialize(xtsc_request::BURST_WRITE, addr, m_width8, 0, num_xfers, be, last, route_id, id, priority, pc);
        p_transaction_info->m_requests_tag       = p_request->get_tag();
        p_transaction_info->m_pending_last_rsps += 1;
      }
      else {
        u64 tag  = p_transaction_info->m_requests_tag;
        be       = (1 << m_width8) - 1;
        if (last) {
          be = 0;
          for (u32 j=0; j<m_width8; ++j) {
            if (bep[(offset + j) % bel] != TLM_BYTE_ENABLED) {
              break;
            }
            be |= (1ull << j);
          }
        }
        p_request->initialize(hw_addr, tag, addr+(xtsc_address)offset, m_width8, num_xfers, beat_index+1, be, route_id, id, priority, pc);
      }
      p_request->set_buffer(m_width8, &ptr[offset]);
      p_request->set_exclusive(excl);
      p_transaction_info->m_requests.push_back(p_request);
      p_transaction_info->m_write_requests_rcvd += 1;
      if (!last) {
        trans.set_response_status(TLM_INCOMPLETE_RESPONSE);
      }
    }
    else { // m_split_tlm2_phases=false
      xtsc_request::type_t type = xtsc_request::BURST_WRITE;
      xtsc_request *p_request = new_request();
      xtsc_address hw_addr = addr;
      be = (1 << m_width8) - 1;
      for (u32 i=0; i<m_width8; ++i) {
        if (bep[i % bel] == TLM_BYTE_ENABLED) {
          hw_addr = addr + i;
          break;
        }
        be ^= (1ull << i);
      }
      p_request->initialize(type, addr, m_width8, 0, num_xfers, be, false, route_id, id, priority, pc);
      p_request->set_buffer(m_width8, ptr);
      p_request->set_exclusive(excl);
      p_transaction_info->m_requests.push_back(p_request);
      u64 tag = p_request->get_tag();
      be = (1 << m_width8) - 1;
      for (u32 i=2; i<=num_xfers; ++i) {
        xtsc_request *p_request = new_request();
        u32 offset = m_width8 * (i - 1);
        bool last = (i == num_xfers);
        if (last) {
          be = 0;
          for (u32 j=0; j<m_width8; ++j) {
            if (bep[(offset + j) % bel] != TLM_BYTE_ENABLED) {
              break;
            }
            be |= (1ull << j);
          }
        }
        p_request->initialize(hw_addr, tag, addr + offset, m_width8, num_xfers, i, be, route_id, id, priority, pc);
        p_request->set_buffer(m_width8, &ptr[offset]);
        p_request->set_exclusive(excl);
        p_transaction_info->m_requests.push_back(p_request);
      }
      p_transaction_info->m_pending_last_rsps += 1;
    }
  }
  else if (use_multiple_full_width_transfers) {
    XTSC_INFO(m_text, trans << " (Using multiple full width transfers)");
    for (u32 i=0; i<num_xfers; ++i) {
      xtsc_request *p_request = new_request();
      if (read) {
        p_request->initialize(xtsc_request::READ,  addr+m_width8*i, m_width8, 0, 1, be, true, route_id, id, priority, pc);
      }
      else {
        if (m_split_tlm2_phases) {
          u32  beat_index = p_transaction_info->m_write_requests_rcvd;
          u32  offset     = m_width8 * beat_index;
          xtsc_request *p_request = new_request();
          p_request->initialize(xtsc_request::WRITE, addr+(xtsc_address)offset, m_width8, 0, 1, be, true, route_id, id, priority, pc);
          p_request->set_buffer(m_width8, &ptr[offset]);
          p_transaction_info->m_write_requests_rcvd += 1;
        } 
        else { //m_split_tlm2_phases=false
          p_request->initialize(xtsc_request::WRITE, addr+m_width8*i, m_width8, 0, 1, be, true, route_id, id, priority, pc);
          p_request->set_buffer(m_width8, &ptr[m_width8*i]);
        }
      }
      p_request->set_exclusive(excl);
      p_transaction_info->m_requests.push_back(p_request);
      if (read) {
        p_transaction_info->m_read_data_offsets .push_back(i*m_width8);
        p_transaction_info->m_read_data_sizes   .push_back(m_width8);
      }
      p_transaction_info->m_pending_last_rsps += 1;
    }
  }
  else {
    XTSC_INFO(m_text, trans << " (Using 1 READ|WRITE per byte)");
    be = 1;
    for (u32 i=0; i < len; ++i, ++addr) {
      if (bep[i % bel] == enabled) {
        xtsc_request *p_request = new_request();
        if (read) {
          p_request->initialize(xtsc_request::READ,  addr, 1, 0, 1, be, true, route_id, id, priority, pc);
        }
        else {
          p_request->initialize(xtsc_request::WRITE, addr, 1, 0, 1, be, true, route_id, id, priority, pc);
          p_request->set_buffer(1, &ptr[i]);
        }
        p_request->set_exclusive(excl);
        p_transaction_info->m_requests.push_back(p_request);
        if (read) {
          p_transaction_info->m_read_data_offsets .push_back(i);
          p_transaction_info->m_read_data_sizes   .push_back(1);
        }
        p_transaction_info->m_pending_last_rsps += 1;
      }
    }
  }

  m_pending_transaction_deque[port_num]->push_back(p_transaction_info);
  m_worker_thread_event[port_num]->notify(SC_ZERO_TIME);

  if (!blocking) { return false; }

  // Wait for all the requests to be sent and for all responses to come back
  sc_core::wait(*m_port_done_event[p_transaction_info->m_port_num]);

  delete_transaction_info(p_transaction_info);

  // Align to phase of beg_time
  sc_time end_time = sc_time_stamp();
  sc_time delta = end_time - beg_time;
  u64 partial = delta.value() % m_clock_period_value;
  if (partial != 0) {
    sc_time rem = (m_clock_period_value - partial) * m_time_resolution;
    XTSC_DEBUG(m_text, "transport_helper for port #" << port_num << ": wait(" << rem << ")");
    sc_core::wait(rem);
  }

  return false;
}



void xtsc_component::xtsc_tlm22xttlm_transactor::tlm_fw_transport_if_impl::b_transport(tlm_generic_payload& trans, sc_time& t) {
  XTSC_INFO(m_transactor.m_text, trans << " " << " (" << t << ")" << " b_transport[" << m_port_num << "]");
  if (m_transactor.m_split_tlm2_phases) {
    ostringstream oss;
    oss << "Port #" << m_port_num << ": Parameter split_tlm2_phases cant be enabled with b_transport() calls'"
        << m_transactor.name() << "': " << trans;
    throw xtsc_exception(oss.str());
  }
  m_transactor.transport_helper(trans, t, true, m_port_num);
}



bool xtsc_component::xtsc_tlm22xttlm_transactor::tlm_fw_transport_if_impl::get_direct_mem_ptr(tlm_generic_payload&  trans,
                                                                                              tlm_dmi&              dmi_data)
{
  xtsc_address  address8  = (xtsc_address) trans.get_address();
  bool          allow     = false;

  dmi_data.allow_read_write();

  if (m_transactor.m_allow_dmi) {
    xtsc_fast_access_request fast_access_request(m_transactor, address8, m_transactor.m_width8, false);

    (*m_transactor.m_request_ports[m_port_num])->nb_fast_access(fast_access_request);

    xtsc_fast_access_block block = fast_access_request.get_result_block();
    dmi_data.set_start_address(block.get_block_beg_address());
    dmi_data.set_end_address  (block.get_block_end_address());

    if (fast_access_request.get_access_type() == xtsc_fast_access_request::ACCESS_RAW) {
      if (trans.is_read() && fast_access_request.is_readable()) {
        if (!fast_access_request.is_writable()) {
          dmi_data.allow_read();
        }
        dmi_data.set_dmi_ptr((u8*)fast_access_request.get_raw_data());
        allow = true;
      }
      else if (trans.is_write() && fast_access_request.is_writable()) {
        if (!fast_access_request.is_readable()) {
          dmi_data.allow_write();
        }
        dmi_data.set_dmi_ptr((u8*)fast_access_request.get_raw_data());
        allow = true;
      }
    }
  }

  u32 n = xtsc_address_nibbles();

  XTSC_INFO(m_transactor.m_text, "get_direct_mem_ptr() address=0x" << hex << setfill('0') << setw(n) << address8 <<
                                 " is_read()=" << boolalpha << trans.is_read() <<
                                 " allow=" << allow <<
                                 " get_granted_access()=" << dmi_data.get_granted_access() <<
                                 " range=0x" << setw(n) << dmi_data.get_start_address() <<
                                 "-0x" << setw(n) << dmi_data.get_end_address() <<
                                 " ptr=" << (void*)dmi_data.get_dmi_ptr());

  return allow;
}



u32 xtsc_component::xtsc_tlm22xttlm_transactor::tlm_fw_transport_if_impl::transport_dbg(tlm_generic_payload& trans) {

  XTSC_VERBOSE(m_transactor.m_text, "Port #" << m_port_num << ": transport_dbg: " << trans);

  tlm_command   cmd     = trans.get_command();
  u64           adr     = trans.get_address();
  u8           *ptr     = trans.get_data_ptr();
  u32           len     = trans.get_data_length();

  if (!m_transactor.m_allow_transport_dbg || (cmd == TLM_IGNORE_COMMAND)) {
    XTSC_INFO(m_transactor.m_text, "Port #" << m_port_num << ": transport_dbg: " << trans << ": returning 0");
    return 0;
  }

  if ((cmd != TLM_READ_COMMAND) && (cmd != TLM_WRITE_COMMAND)) {
    ostringstream oss;
    oss << "Port #" << m_port_num << ": Unsupported command received in transport_dbg() of xtsc_tlm22xttlm_transactor '"
        << m_transactor.name() << "': " << trans;
    throw xtsc_exception(oss.str());
  }

  // This is NOT required
  trans.set_response_status(TLM_OK_RESPONSE);

  if (len == 0) {
    XTSC_INFO(m_transactor.m_text, "Port #" << m_port_num << ": transport_dbg: " << trans << ": returning 0");
    return 0;
  }

  try {
    if (cmd == TLM_READ_COMMAND) {
      (*m_transactor.m_request_ports[m_port_num])->nb_peek((xtsc_address) adr, len, ptr);
    }
    else {
      (*m_transactor.m_request_ports[m_port_num])->nb_poke((xtsc_address) adr, len, ptr);
    }
  }
  catch (const exception& error) {
    XTSC_INFO(m_transactor.m_text, "Port #" << m_port_num << ": transport_dbg got std::exception from call to " <<
              ((cmd == TLM_READ_COMMAND) ? "nb_peek" : "nb_poke") << " for " << len << " bytes at address 0x" << hex << adr << dec << endl <<
              "what(): " << error.what());
    return 0;
  }

  return len;
}



void xtsc_component::xtsc_tlm22xttlm_transactor::tlm_fw_transport_if_impl::register_port(sc_port_base& port, const char *if_typename) {
  if (m_p_port) {
    ostringstream oss;
    oss << "Illegal multiple binding detected to xtsc_tlm22xttlm_transactor '" << m_transactor.name() << "' " << name() << ": " << endl;
    oss << "  " << port.name() << endl;
    oss << "  " << m_p_port->name();
    throw xtsc_exception(oss.str());
  }
  XTSC_INFO(m_transactor.m_text, "Binding '" << port.name() << "' to xtsc_tlm22xttlm_transactor::" << name() <<
                                 " Port #" << m_port_num);
  m_p_port = &port;
}


