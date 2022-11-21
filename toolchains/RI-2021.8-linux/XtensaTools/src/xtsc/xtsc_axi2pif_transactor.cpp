// Copyright (c) 2006-2018 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of
// Cadence Design Systems, Inc.  They may be adapted and modified by bona fide
// purchasers for internal use, but neither the original nor any adapted
// or modified version may be disclosed or distributed to third parties
// in any manner, medium, or form, in whole or in part, without the prior
// written consent of Cadence Design Systems, Inc.


/*
 *                                   Theory of operation
 *
 *
 * This transactor converts Xtensa AXI transactions to Xtensa PIF transactions. 
 * Basically this means it converts the xtsc_request_if API's AXI BURST_READ and BURST_WRITE to 
 * NON_AXI (PIF) READ, BLOCK_READ, BURST_READ and WRITE, WRITE_BLOCK, WRITE_BURST transactions. While
 * sending the PIF requests forward, it arbitrates between the requests on first come-first serve basis.
 * It also converts the xtsc_respond_if API's on the respond path. While sending the response back to 
 * AXI interface, it sends out READ responses on read_respond_ports and WRITE responses on 
 * write_respond_ports.
 *
 * All PIF read/write requests for a particular AXI transaction are sent together (in subsequent
 * cycles) without interleaving with read/write requests of other AXI transactions.
 * AXI read responses are sent through read_respond_ports and AXI write responses are sent through
 * write_respond_ports.
 *
 * The transactor adds latencies to the read and write request paths, which can be configured through
 * read_request_delay and write_request_delay.
 * Similarly, the transactor adds latencies to the read and write response paths, which can be 
 * configured through read_response_delay and write_response_delay.
 *
 * Both read and write AXI requests have respective fifo buffers which can be configured for depth 
 * using read_request_fifo_depth and write_request_fifo_depth parameters.
 * Similarly, both read and write AXI responses have respective fifo buffers which can be configured
 * for depth using read_response_fifo_depth and write_response_fifo_depth parameters.
 * 
 * The xtsc_request_if API's are : 
 * nb_request
 * nb_fast_access
 * nb_peek
 * nb_poke
 *
 * The xtsc_respond_if API's are:
 * nb_respond
 * 
 * On the request path, the transaction handling sequence is:
 *     -> nb_request -> nb_request_worker -> request_thread -> conv_rw_axi2pif_request ->
 *     handle_pif_request -> send_pif_req_worker ->
 * On the response path, the transaction handling sequence is:
 *     -> nb_respond -> TYPE_respond_worker -> TYPE_response_thread -> send_TYPE_response ->
 * 
 *    where, TYPE is either read or write
 *
 * * nb_request
 * In nb_request, few checks are made. 
 * First, we check if correct protocol i.e. AXI is set. If not, exception is raised. 
 * Second, we check if read requests arrive only on read_request_if and write requests arrive only on
 * write_request_if. If not, exception is raised.
 * Third, if byte_size for request is larger than AXI port width, exception is raised.
 * Last, if TYPE_request_fifo is full, than send NACC response and return.
 * If above checks pass, then a copy of the xtsc_request is made and passed into nb_request_worker.
 *
 * * nb_request_worker
 * A new axi_req_info is created from xtsc_request and pushed into TYPE_request_fifo. axi_req_info keeps
 * tracks of sched_time to accomodate TYPE_request_delay. Then, request thread is notified in the same
 * delta cycle with this delay. Also, any new tag is pushed into axi_request_order_fifo to maintain 
 * order of requests.
 *
 * * request_thread
 * When request_thread wakes up (or finishes the previous request), it gets the next request out of
 * m_TYPE_request_fifo. If m_write_request_active is true, then it waits for write requests for a 
 * particular AXI transaction to be sent downstream and does not check if we have any read requests
 * pending in m_read_request_fifo
 * It calls get_next_axi_request to get the next axi_req_info based on their arrival.
 * If sched time has not arrived, it waits.
 * If the AXI request has not been converted to pif requests, it does so. Else returns the corresponding
 * axi_trans_info.
 * Till pif_requests_vec is not empty, it picks up the front pif_request, and calls handle_pif_request
 * for that request. If exclusive request, exception is raised. 
 *
 * * handle_pif_request
 * It checks if type_name is correctly set. Then, it checks if address is aligned with request to its 
 * byte_size. Thirdly, it checks if size8 is not equal to m_pif_width8 for BLOCK and BURST pif requests.
 * In all cases, exception is raised if failed.
 * If all checks pass, send_pif_req_worker is called.
 *
 * * send_pif_req_worker
 * The send_pif_req_worker is a time consuming blocking function, which returns after PIF transaction has 
 * been issues succefully to the PIF slave interface. It calls nb_request to send the each PIF 
 * transaction to the Xtensa PIF target through relevant downstream PIF port. 
 *
 * * nb_respond
 * nb_respond is issued by the downstream PIF target in response to the requests issued. It uses the 
 * m_tag_2_axi_trans_info_map to get the corresponding axi_trans_info from pif request tag. It checks if
 * response is NACC and sets member variables accordingly. If TYPE_response_fifo is full, false is 
 * returned. Appropriate TYPE_response_worker is called.
 *
 * * TYPE_response_worker
 * After the expected number of AXI responses have been received, it creates AXI response and
 * axi_rsp_trans_info. axi_rsp_trans_info takes care of TYPE_response_delay. It is pushed into the 
 * m_TYPE_response_fifo and notifies TYPE_response_thread with the delay.
 *
 * * TYPE_response_thread
 * When TYPE_response_thread wakes up (or finishes the previous response), it gets the next transaction 
 * out of m_TYPE_response_fifo and calls send_TYPE_response() the appropriate number of times.
 *
 * * send_TYPE_response
 * send TYPE response back to the PIF Initiator. Keeps trying to send the response on next clock edge if 
 * RSP_NACC is received from the Initiator.
 *
 * * nb_fast_access:
 * If the upstream Xtensa TLM subsystem includes an xtsc_core which wants fast access (TurboXim) to
 * the downstream OSCI TLM2 subsystem, it calls this transactor's nb_fast_access method. which 
 * subsequently calls the downstream Xtensa AXI subsystem using nb_fast_access to attempt to give 
 * TurboXim raw fast access.
 *
 * * nb_peek, nb_poke:
 * If the upstream Xtensa TLM subsystem wants to peek/poke the downstream Xtensa AXI subsystem, it
 * calls this transactor's nb_peek or nb_poke method.  These methods both call the nb_peek or nb_poke 
 * method in the downstream Xtensa AXI subsystem 
 *
 *
 * Key Data structures involved :
 *
 * axi_req_info : Structure to keep track of request sched time based on TYPE_request_delay
 *
 * axi_trans_info : Structure to keep track of all the PIF requests for a particular AXI transaction. It
 * helps in ensuring all the correct incoming write requests for a particular transaction. It helps in
 * ensuring correct responses to the AXI master.
 * Not all of the fields are relevant for each request. 
 *
 * axi_rsp_trans_info : Structure to keep track of response sched time based on TYPE_response_delay.
 *
 * m_tag_2_axi_trans_info_map : std::map of AXI/PIF tags to corresponding axi_trans_info
 *
 * m_axi_request_order_fifo   : std::queue to keep track of the order of incoming AXI requests
 *
 */

#include <cerrno>
#include <algorithm>
#include <cmath>
#include <ostream>
#include <string>
#include <xtsc/xtsc_axi2pif_transactor.h>

using namespace std;
using namespace sc_core;
using namespace log4xtensa;
using namespace xtsc;

/********** xtsc_axi2pif_transactor ************/

xtsc_component::xtsc_axi2pif_transactor::xtsc_axi2pif_transactor(
  sc_module_name                       module_name,
  const xtsc_axi2pif_transactor_parms& parms
  ) :
    sc_module                            (module_name),
    xtsc_module                          (*(sc_module*)this),

    m_read_request_exports               (nullptr),
    m_read_respond_ports                 (nullptr),
    m_write_request_exports              (nullptr),
    m_write_respond_ports                (nullptr),
    m_request_ports                      (nullptr),
    m_respond_exports                    (nullptr),

    m_axi_width8                         (parms.get_non_zero_u32 ("axi_byte_width")),
    m_num_ports                          (parms.get_non_zero_u32 ("num_ports")),
    m_pif_width8                         (m_axi_width8),

    m_read_request_delay                 (parms.get_u32          ("read_request_delay")),
    m_write_request_delay                (parms.get_u32          ("write_request_delay")),
    m_read_response_delay                (parms.get_u32          ("read_response_delay")),
    m_write_response_delay               (parms.get_u32          ("write_response_delay")),

    m_time_resolution                    (sc_get_time_resolution()),

    m_xtsc_read_request_if_impl          (nullptr),
    m_xtsc_write_request_if_impl         (nullptr),
    m_xtsc_respond_if_impl               (nullptr),

    m_request_thread_event               (nullptr),
    m_read_response_thread_event         (nullptr),
    m_write_response_thread_event        (nullptr),

    m_read_request_fifo                  (nullptr),
    m_write_request_fifo                 (nullptr),
    m_read_response_fifo                 (nullptr),
    m_write_response_fifo                (nullptr),
    m_axi_request_order_fifo             (nullptr),

    m_axi_rd_response_order_fifo         (nullptr),
    m_axi_wr_response_order_fifo         (nullptr),

    m_read_request_fifo_depth            (parms.get_non_zero_u32 ("read_request_fifo_depth")),
    m_write_request_fifo_depth           (parms.get_non_zero_u32 ("write_request_fifo_depth")),
    m_read_response_fifo_depth           (parms.get_non_zero_u32 ("read_response_fifo_depth")),
    m_write_response_fifo_depth          (parms.get_non_zero_u32 ("write_response_fifo_depth")),

    m_pif_waiting_for_nacc               (nullptr),
    m_pif_request_got_nacc               (nullptr),

    m_critical_word_first                (parms.get_bool         ("critical_word_first")),

    m_outstanding_pif_request_ids        (parms.get_u32          ("outstanding_pif_request_ids")),
    m_read_request_id_pending            (nullptr),
    m_write_request_id_pending           (nullptr),
    m_num_read_pif_requests_pending      (nullptr),
    m_num_write_pif_requests_pending     (nullptr),
    m_last_read_req_id_selected          (nullptr),
    m_last_write_req_id_selected         (nullptr),

    m_write_request_active               (nullptr),
    m_write_request_active_tag           (nullptr),

    m_next_request_port_num              (0),
    m_next_read_response_port_num        (0),
    m_next_write_response_port_num       (0),

    m_secure_address_range               (parms.get_u64_vector("secure_address_range")),
    m_max_block_pif_byte_size            (32),
    m_non_secure_mode                    (false),

    m_rd_nsm_vld                         (nullptr),
    m_wr_nsm_vld                         (nullptr),

    m_request_count                      (0),
    m_mode_switch_pending                (false),
    m_text                               (TextLogger::getInstance(name()))
{
  ostringstream oss;

  if ((m_axi_width8 !=  4) && (m_axi_width8 !=  8) && (m_axi_width8 != 16) && (m_axi_width8 != 32) && (m_axi_width8 != 64)) {
    oss.str(""); oss << kind() << " '" << name() << "': Invalid \"axi_byte_width\" parameter value (" << m_axi_width8 << ").  Expected 4|8|16|32|64.";
    throw xtsc_exception(oss.str());
  }

  if (m_num_ports == 0) {
    oss.str(""); oss << kind() << " '" << name() << "': Invalid \"num_ports\" parameter value (" << m_num_ports << ").  Expected non zero value.";
    throw xtsc_exception(oss.str());
  }

  // Create all the "per-port" stuff
  m_read_request_exports                 = new sc_export<xtsc_request_if>              *[m_num_ports];
  m_read_respond_ports                   = new sc_port<xtsc_respond_if>                *[m_num_ports];
  m_write_request_exports                = new sc_export<xtsc_request_if>              *[m_num_ports];
  m_write_respond_ports                  = new sc_port<xtsc_respond_if>                *[m_num_ports];
  m_request_ports                        = new sc_port<xtsc_request_if>                *[m_num_ports];
  m_respond_exports                      = new sc_export<xtsc_respond_if>              *[m_num_ports];

  m_xtsc_read_request_if_impl            = new xtsc_request_if_impl                    *[m_num_ports];
  m_xtsc_write_request_if_impl           = new xtsc_request_if_impl                    *[m_num_ports];
  m_xtsc_respond_if_impl                 = new xtsc_respond_if_impl                    *[m_num_ports];
  
  m_request_thread_event                 = new sc_event                                *[m_num_ports];
  m_read_response_thread_event           = new sc_event                                *[m_num_ports];
  m_write_response_thread_event          = new sc_event                                *[m_num_ports];
  
  m_read_request_fifo                    = new std::queue<axi_req_info*>               *[m_num_ports];
  m_write_request_fifo                   = new std::queue<axi_req_info*>               *[m_num_ports];
  m_read_response_fifo                   = new std::queue<axi_rsp_trans_info*>         *[m_num_ports];
  m_write_response_fifo                  = new std::queue<axi_rsp_trans_info*>         *[m_num_ports];
  m_axi_request_order_fifo               = new std::queue<xtsc::u64>                   *[m_num_ports];
  
  m_axi_rd_response_order_fifo           = new std::queue<xtsc::u64>                   *[m_num_ports];
  m_axi_wr_response_order_fifo           = new std::queue<xtsc::u64>                   *[m_num_ports];
  
  m_pif_waiting_for_nacc                 = new bool                                     [m_num_ports];
  m_pif_request_got_nacc                 = new bool                                     [m_num_ports];  

  m_read_request_id_pending              = new bool                                    *[m_num_ports];
  m_write_request_id_pending             = new bool                                    *[m_num_ports];
  m_num_read_pif_requests_pending        = new xtsc::u32                                [m_num_ports];
  m_num_write_pif_requests_pending       = new xtsc::u32                                [m_num_ports];
  m_last_read_req_id_selected            = new xtsc::u8                                 [m_num_ports];
  m_last_write_req_id_selected           = new xtsc::u8                                 [m_num_ports];

  m_write_request_active                 = new bool                                     [m_num_ports];
  m_write_request_active_tag             = new xtsc::u64                                [m_num_ports];

  m_rd_nsm_vld                           = new bool                                     [m_num_ports];
  m_wr_nsm_vld                           = new bool                                     [m_num_ports];

  for (u32 i=0; i<m_num_ports; ++i) {

    oss.str(""); oss << "m_read_request_exports"       << "[" << i << "]";
    m_read_request_exports[i]       = new sc_export<xtsc_request_if>(oss.str().c_str());
    m_bit_width_map[oss.str()]     = m_axi_width8*8;

    oss.str(""); oss << "m_read_respond_ports"         << "[" << i << "]";
    m_read_respond_ports[i]         = new sc_port  <xtsc_respond_if>(oss.str().c_str());
    m_bit_width_map[oss.str()]      = m_axi_width8*8;

    oss.str(""); oss << "m_write_request_exports"      << "[" << i << "]";
    m_write_request_exports[i]      = new sc_export<xtsc_request_if>(oss.str().c_str());
    m_bit_width_map[oss.str()]      = m_axi_width8*8;

    oss.str(""); oss << "m_write_respond_ports"        << "[" << i << "]";
    m_write_respond_ports[i]        = new sc_port  <xtsc_respond_if>(oss.str().c_str());
    m_bit_width_map[oss.str()]      = m_axi_width8*8;

    oss.str(""); oss << "m_request_ports"              << "[" << i << "]";
    m_request_ports[i]              = new sc_port  <xtsc_request_if>(oss.str().c_str());
    m_bit_width_map[oss.str()]      = m_pif_width8*8;

    oss.str(""); oss << "m_respond_exports"            << "[" << i << "]";
    m_respond_exports[i]            = new sc_export<xtsc_respond_if>(oss.str().c_str());
    m_bit_width_map[oss.str()]      = m_pif_width8*8;

    oss.str(""); oss << "m_xtsc_read_request_if_impl"  << "[" << i << "]";
    m_xtsc_read_request_if_impl[i]  = new xtsc_request_if_impl(oss.str().c_str(), *this, i, "READ");
    m_bit_width_map[oss.str()] = m_axi_width8*8;

    oss.str(""); oss << "m_xtsc_write_request_if_impl" << "[" << i << "]";
    m_xtsc_write_request_if_impl[i] = new xtsc_request_if_impl(oss.str().c_str(), *this, i, "WRITE");

    oss.str(""); oss << "m_xtsc_respond_if_impl"       << "[" << i << "]";
    m_xtsc_respond_if_impl[i]       = new xtsc_respond_if_impl(oss.str().c_str(), *this, i);

    (*m_read_request_exports[i])(*m_xtsc_read_request_if_impl[i]);
    (*m_write_request_exports[i])(*m_xtsc_write_request_if_impl[i]);
    (*m_respond_exports[i])(*m_xtsc_respond_if_impl[i]);

    if (m_read_request_fifo_depth) {
      m_read_request_fifo[i] = new std::queue<axi_req_info*>();
    } else {
      oss.str(""); 
      oss << kind() << " '" << name() << "' Invalid \"read_request_fifo_depth\" parameter value (" << 
          m_read_request_fifo_depth << ").  Expected greather than 0.";
      throw xtsc_exception(oss.str());
    }

    if (m_write_request_fifo_depth) {
      m_write_request_fifo[i] = new std::queue<axi_req_info*>();
    } else {
      oss.str(""); 
      oss << kind() << " '" << name() << "' Invalid \"write_request_fifo_depth\" parameter value (" << 
          m_write_request_fifo_depth << ").  Expected greather than 0.";
      throw xtsc_exception(oss.str());
    }

    if (m_read_response_fifo_depth) {
      m_read_response_fifo[i] = new std::queue<axi_rsp_trans_info*>();
    } else {
      oss.str(""); 
      oss << kind() << " '" << name() << "' Invalid \"read_response_fifo_depth\" parameter value (" << 
          m_read_response_fifo_depth << ").  Expected greather than 0.";
      throw xtsc_exception(oss.str());
    }

    if (m_write_response_fifo_depth) {
      m_write_response_fifo[i] = new std::queue<axi_rsp_trans_info*>();
    } else {
      oss.str(""); 
      oss << kind() << " '" << name() << "' Invalid \"write_response_fifo_depth\" parameter value (" << 
          m_write_response_fifo_depth << ").  Expected greather than 0.";
      throw xtsc_exception(oss.str());
    }

    m_axi_request_order_fifo[i]           = new std::queue<xtsc::u64>();

    m_axi_rd_response_order_fifo[i]       = new std::queue<xtsc::u64>();
    m_axi_wr_response_order_fifo[i]       = new std::queue<xtsc::u64>();

    m_pif_waiting_for_nacc[i]             = false;
    m_pif_request_got_nacc[i]             = false;

    m_read_request_id_pending[i]          = new bool[m_outstanding_pif_request_ids];
    m_write_request_id_pending[i]         = new bool[m_outstanding_pif_request_ids];
    m_num_read_pif_requests_pending[i]    = 0;
    m_num_write_pif_requests_pending[i]   = 0;
    m_last_read_req_id_selected[i]        = (m_outstanding_pif_request_ids==0)
      ? 32
      : 32 + (m_outstanding_pif_request_ids-1);
    m_last_write_req_id_selected[i]       = (m_outstanding_pif_request_ids==0)
      ? 0
      : (m_outstanding_pif_request_ids-1);

    m_write_request_active[i]             = false;
    m_write_request_active_tag[i]         = 0;

    m_rd_nsm_vld[i]                       = false;
    m_wr_nsm_vld[i]                       = false;

    for (xtsc::u32 j=0; j<m_outstanding_pif_request_ids; j++)
    {
      m_read_request_id_pending[i][j]     = false;
      m_write_request_id_pending[i][j]    = false;
    }

    ostringstream oss1;
    oss1.str(""); oss1 << "request_thread_"        << i;
    ostringstream oss2;
    oss2.str(""); oss2 << "read_response_thread_"  << i;
    ostringstream oss3;
    oss3.str(""); oss3 << "write_response_thread_" << i;
#if IEEE_1666_SYSTEMC >= 201101L
    m_request_thread_event[i]               = new sc_event((oss1.str() + "event").c_str());
    m_read_response_thread_event[i]         = new sc_event((oss2.str() + "event").c_str());
    m_write_response_thread_event[i]        = new sc_event((oss3.str() + "event").c_str());
#else
    m_request_thread_event[i]               = new sc_event;
    xtsc_event_register(*m_request_thread_event[i],        oss1.str() + "_event", this);
    m_read_response_thread_event[i]         = new sc_event;
    xtsc_event_register(*m_read_response_thread_event[i],  oss2.str() + "_event", this);
    m_write_response_thread_event[i]        = new sc_event;
    xtsc_event_register(*m_write_response_thread_event[i], oss3.str() + "_event", this);
#endif
    declare_thread_process(request_thread_handle,        oss1.str().c_str(), SC_CURRENT_USER_MODULE, request_thread);
    m_process_handles.push_back(sc_get_current_process_handle());
    declare_thread_process(read__response_thread_handle, oss2.str().c_str(), SC_CURRENT_USER_MODULE, read_response_thread);
    m_process_handles.push_back(sc_get_current_process_handle());
    declare_thread_process(write_response_thread_handle, oss3.str().c_str(), SC_CURRENT_USER_MODULE, write_response_thread);
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

  u32 nacc_wait_time = parms.get_u32("nacc_wait_time");
  if (nacc_wait_time == 0xFFFFFFFF) {
    m_nacc_wait_time = m_clock_period;
  }
  else {
    m_nacc_wait_time = m_time_resolution * nacc_wait_time;
    if (m_nacc_wait_time > m_clock_period) {
      oss.str(""); oss << kind() << " '" << name() << "': \"nacc_wait_time\"=" << m_nacc_wait_time << " exceeds clock period of " << m_clock_period;
      throw xtsc_exception(oss.str());
    }
  }

  for (u32 i=0; i<m_num_ports; ++i) {
    oss.str(""); oss << "m_read_request_exports"         << "[" << i << "]"; m_port_types[oss.str()]   = REQUEST_EXPORT;
    oss.str(""); oss << "m_read_respond_ports"           << "[" << i << "]"; m_port_types[oss.str()]   = RESPOND_PORT;
    oss.str(""); oss << "m_write_request_exports"        << "[" << i << "]"; m_port_types[oss.str()]   = REQUEST_EXPORT;
    oss.str(""); oss << "m_write_respond_ports"          << "[" << i << "]"; m_port_types[oss.str()]   = RESPOND_PORT;
    oss.str(""); oss << "m_request_ports"                << "[" << i << "]"; m_port_types[oss.str()]   = REQUEST_PORT;
    oss.str(""); oss << "m_respond_exports"              << "[" << i << "]"; m_port_types[oss.str()]   = RESPOND_EXPORT;
    
    oss.str(""); oss << "slave_port"                     << "[" << i << "]"; m_port_types[oss.str()]   = PORT_TABLE;
    oss.str(""); oss << "master_port"                    << "[" << i << "]"; m_port_types[oss.str()]   = PORT_TABLE;
    oss.str(""); oss << "axislave_rd"                    << "[" << i << "]"; m_port_types[oss.str()]   = PORT_TABLE;
    oss.str(""); oss << "axislave_wr"                    << "[" << i << "]"; m_port_types[oss.str()]   = PORT_TABLE;
  }

  m_port_types["slave_ports"]      = PORT_TABLE;
  m_port_types["master_ports"]     = PORT_TABLE;
  
  if (m_num_ports == 1) {
    m_port_types["slave_port"]     = PORT_TABLE;
    m_port_types["master_port"]    = PORT_TABLE;
    m_port_types["axislave_rd"]    = PORT_TABLE;
    m_port_types["axislave_wr"]    = PORT_TABLE;
  }

  u32 n = xtsc_address_nibbles();

  if (m_secure_address_range.size()) {
    if (m_secure_address_range.size() & 0x1) {
      ostringstream oss;
      oss << kind() << " '" << name() << "': " << "\"secure_address_range\" parameter has " << m_secure_address_range.size()
          << " elements which is not an even number as it must be.";
      throw xtsc_exception(oss.str());
    }

    m_non_secure_mode = true;

    for (u32 i=0; i<m_secure_address_range.size(); i+=2) {
      xtsc_address begin = m_secure_address_range[i+0];
      xtsc_address end   = m_secure_address_range[i+1];
      
      if (end < begin)
      {
        ostringstream oss;
        oss << kind() << " '" << name() << "': " << "\"secure_address_range\" range [0x" << hex << setfill('0') << setw(n) << begin
            << "-0x" << setw(n) << end << "] is invalid as end_address is less than start_address." << endl;
        throw xtsc_exception(oss.str());
      }
    }
  }

  xtsc_register_command(*this, *this, "change_clock_period", 1, 1,
      "change_clock_period <ClockPeriodFactor>", 
      "Call xtsc_axi2pif_transactor::change_clock_period(<ClockPeriodFactor>)."
      "  Return previous <ClockPeriodFactor> for this device."
  );

  xtsc_register_command(*this, *this, "peek", 3, 3,
      "peek <PortNumber> <StartAddress> <NumBytes>", 
      "Peek <NumBytes> of memory starting at <StartAddress>."
  );

  xtsc_register_command(*this, *this, "poke", 3, -1,
      "poke <PortNumber> <StartAddress> <NumBytes> <Byte1> <Byte2> . . . <ByteN>", 
      "Poke <NumBytes> (=N) of memory starting at <StartAddress>."
  );

  xtsc_register_command(*this, *this, "reset", 0, 0,
      "reset", 
      "Call xtsc_axi2pif_transactor::reset()."
  );

  LogLevel ll = xtsc_get_constructor_log_level();
  
  XTSC_LOG(m_text, ll,        " Constructed xtsc_axi2pif_transactor_parms '" << name() << "':");
  XTSC_LOG(m_text, ll,        " axi_byte_width                  = "    << m_axi_width8);
  
  if (clock_period == 0xFFFFFFFF) {
  XTSC_LOG(m_text, ll, hex << " clock_period                    =  0x" << clock_period << " (" << m_clock_period << ")");
  } else {
  XTSC_LOG(m_text, ll,        " clock_period                    = "    << clock_period << " (" << m_clock_period << ")");
  }

  XTSC_LOG(m_text, ll,        " critical_word_first             = "    << m_critical_word_first);
  XTSC_LOG(m_text, ll,        " outstanding_pif_request_ids     = "    << m_outstanding_pif_request_ids);
  
  if (nacc_wait_time == 0xFFFFFFFF) {
  XTSC_LOG(m_text, ll,        " nacc_wait_time                  = 0xFFFFFFFF => " << m_nacc_wait_time.value() << " (" << m_nacc_wait_time << ")");
  } else {
  XTSC_LOG(m_text, ll,        " nacc_wait_time                  = "    << nacc_wait_time << " (" << m_nacc_wait_time << ")");
  } 

  XTSC_LOG(m_text, ll,        " num_ports                       = "    << m_num_ports);
  XTSC_LOG(m_text, ll,        " read_request_delay              = "    << m_read_request_delay);
  XTSC_LOG(m_text, ll,        " read_request_fifo_depth         = "    << m_read_request_fifo_depth);
  XTSC_LOG(m_text, ll,        " read_response_delay             = "    << m_read_response_delay);
  XTSC_LOG(m_text, ll,        " read_response_fifo_depth        = "    << m_read_response_fifo_depth);
  XTSC_LOG(m_text, ll,        " write_request_delay             = "    << m_write_request_delay);
  XTSC_LOG(m_text, ll,        " write_request_fifo_depth        = "    << m_write_request_fifo_depth);
  XTSC_LOG(m_text, ll,        " write_response_delay            = "    << m_write_response_delay);
  XTSC_LOG(m_text, ll,        " write_response_fifo_depth       = "    << m_write_response_fifo_depth);

  reset();
}

// Delete all requests of m_request_pool
xtsc_component::xtsc_axi2pif_transactor::~xtsc_axi2pif_transactor(void) {
  XTSC_TRACE(m_text, "Enter ::~xtsc_axi2pif_transactor()");

  for (u32 i=0; i < m_num_ports; i++) {
    delete m_read_request_exports[i];
    delete m_read_respond_ports[i];

    delete m_write_request_exports[i];
    delete m_write_respond_ports[i];

    delete m_request_ports[i];
    delete m_respond_exports[i];

    delete m_xtsc_read_request_if_impl[i];
    delete m_xtsc_write_request_if_impl[i];
    delete m_xtsc_respond_if_impl[i];

    delete m_request_thread_event[i];
    delete m_read_response_thread_event[i];
    delete m_write_response_thread_event[i];

    delete m_read_request_fifo[i];
    delete m_write_request_fifo[i];
    delete m_read_response_fifo[i];
    delete m_write_response_fifo[i];
    delete m_axi_request_order_fifo[i];
  }

  delete [] m_read_request_exports;
  delete [] m_read_respond_ports;

  delete [] m_write_request_exports;
  delete [] m_write_respond_ports;

  delete [] m_request_ports;
  delete [] m_respond_exports;

  delete [] m_xtsc_read_request_if_impl;
  delete [] m_xtsc_write_request_if_impl;
  delete [] m_xtsc_respond_if_impl;

  delete [] m_request_thread_event;
  delete [] m_read_response_thread_event;
  delete [] m_write_response_thread_event;

  delete [] m_read_request_fifo;
  delete [] m_write_request_fifo;
  delete [] m_read_response_fifo;
  delete [] m_write_response_fifo;
  delete [] m_axi_request_order_fifo;

  delete [] m_pif_waiting_for_nacc;
  delete [] m_pif_request_got_nacc;

  delete [] m_write_request_active;
  delete [] m_write_request_active_tag;

  while (m_request_pool.size()) {
    delete m_request_pool.back();
    m_request_pool.pop_back();
  }
}

void xtsc_component::xtsc_axi2pif_transactor::end_of_simulation(void) 
{
  XTSC_TRACE(m_text, "Enter ::end_of_simulation()");

  LogLevel ll = log4xtensa::INFO_LOG_LEVEL;
  bool mismatch = ( m_request_pool.size() != m_request_count );
  if (mismatch) {
    ll = log4xtensa::ERROR_LOG_LEVEL;
    XTSC_LOG(m_text, ll, "Error: 1 or more mismatches between pool sizes and counts which indicates a leak: ");
  }
  XTSC_LOG(m_text, ll, m_request_pool.size() << "/" << m_request_count << " (m_request_pool/m_request_count)");
}


u32 
xtsc_component::xtsc_axi2pif_transactor::get_bit_width(const string& port_name, u32 interface_num) const 
{
  XTSC_TRACE(m_text, "Enter ::get_bit_width(port_name:" << port_name << ", interface num:" 
      << interface_num << ")" );

  if (interface_num == 0) {
    map<string,u32>::const_iterator i = m_bit_width_map.find(port_name);
    if (i == m_bit_width_map.end()) {
      ostringstream oss;
      oss << kind() << " \"" << name() << "\" has no port named \"" << port_name << "\"" << endl;
      throw xtsc_exception(oss.str());
    }
    return i->second;
  } else {
    return 0;
  }
}


sc_object *
xtsc_component::xtsc_axi2pif_transactor::get_port(const string& port_name)
{
  XTSC_TRACE(m_text, "Enter ::get_port(port_name:" << port_name << ")");
  string name_portion;
  u32    index;
  
  //Function defined in source/xtsc.cpp
  //This function takes in port_name such as m_request_ports or m_request_ports[0]
  //If it is indexed, such as m_request_ports[0], returns true with
  //name_portion = m_request_ports and index = 0
  //Else returns false
  bool   indexed = xtsc_parse_port_name(port_name, name_portion, index);

  //Checking two conditions
  //First, for any port if name_portion does not match, throw exception
  //Second, for num_ports > 1, if not indexed, throw exception
  if (((m_num_ports > 1) && !indexed) ||
      ((name_portion != "m_read_request_exports")   &&
       (name_portion != "m_read_respond_ports")     &&
       (name_portion != "m_write_request_exports")  &&
       (name_portion != "m_write_respond_ports")    &&
       (name_portion != "m_request_ports")          &&
       (name_portion != "m_respond_exports")
     ))
  {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\" has no port named \"" << port_name << "\"" << endl;
    throw xtsc_exception(oss.str());
  }

  //Boundary condition for above check
  //Index should be 0 to num_ports-1
  //If not, throw exception
  if (index > m_num_ports - 1) {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\": Invalid index: \"" << port_name << "\".  Valid range: 0-" << (m_num_ports-1) << endl;
    throw xtsc_exception(oss.str());
  }

  //Check name_portion and return appropriate port
       if (name_portion == "m_read_request_exports")      return m_read_request_exports    [index];
  else if (name_portion == "m_read_respond_ports")        return m_read_respond_ports      [index];
  else if (name_portion == "m_write_request_exports")     return m_write_request_exports   [index];
  else if (name_portion == "m_write_respond_ports")       return m_write_respond_ports     [index];
  else if (name_portion == "m_request_ports")             return m_request_ports           [index];
  else if (name_portion == "m_respond_exports")           return m_respond_exports         [index];
  else {
    //If name_portion does not match any of above, throw exception
    //Check if redundant code
    ostringstream oss;
    oss << __FUNCTION__ << "Program Bug in xtsc_axi2pif_transactor.cpp" << endl;
    throw xtsc_exception(oss.str());
  }
}

xtsc_port_table 
xtsc_component::xtsc_axi2pif_transactor::get_port_table(const string& port_table_name) const 
{
  XTSC_TRACE(m_text, "Enter ::get_port_table(port_table_name:" << port_table_name << ")");
  xtsc_port_table table;

  //Beginning - slave/master_ports
  //Result - 
  //For num_ports=1, slave/master_port
  //Else list of slave/master_port[i]
  if (port_table_name == "slave_ports") {
    if (m_num_ports == 1) {
      ostringstream oss;
      oss << "slave_port";
      table.push_back(oss.str());
    } 
    else {    
      for (u32 i=0; i<m_num_ports; ++i) {
        ostringstream oss;
        oss << "slave_port[" << i << "]";
        table.push_back(oss.str());
      }
    } 

    return table;
  }

  if (port_table_name == "master_ports") {
    if (m_num_ports == 1) {
      ostringstream oss;
      oss << "master_port";
      table.push_back(oss.str());
    } 
    else {  
      for (u32 i=0; i<m_num_ports; ++i) {
        ostringstream oss;
        oss << "master_port[" << i << "]";
        table.push_back(oss.str());
      }
    }
    return table;
  }  

  //Getting name_portion for slave/master_port[i] or axislave_rd/wr[i]
  string name_portion;
  u32    index;
  bool   indexed = xtsc_parse_port_name(port_table_name, name_portion, index);

  //Check if indexed for num_ports>1
  //Check if name_portion matches
  if (((m_num_ports > 1) && !indexed) || 
      ((name_portion != "slave_port") && 
       (name_portion != "axislave_rd") && 
       (name_portion != "axislave_wr")  && 
       (name_portion != "master_port")
     )) 
  {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\" has no port table named \"" << port_table_name << "\". You can start with port_tables 'slave_ports' and 'master_ports'." << endl;
    throw xtsc_exception(oss.str());
  }

  if (index > m_num_ports - 1) {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\": Invalid index: \"" << port_table_name << "\".  Valid range: 0-" << (m_num_ports-1) << endl;
    throw xtsc_exception(oss.str());
  }

  ostringstream oss;
  //Initiator side is AXI so slave_port will have axislave_rd/wr
  if (name_portion == "slave_port") {
    oss.str(""); oss << "axislave_rd"               << "[" << index << "]"; table.push_back(oss.str());
    oss.str(""); oss << "axislave_wr"               << "[" << index << "]"; table.push_back(oss.str());
  } 
  if (name_portion == "axislave_rd") {
    oss.str(""); oss << "m_read_request_exports"    << "[" << index << "]"; table.push_back(oss.str());
    oss.str(""); oss << "m_read_respond_ports"      << "[" << index << "]"; table.push_back(oss.str());
  } 
  else if (name_portion == "axislave_wr") {
    oss.str(""); oss << "m_write_request_exports"   << "[" << index << "]"; table.push_back(oss.str());
    oss.str(""); oss << "m_write_respond_ports"     << "[" << index << "]"; table.push_back(oss.str());
  }
  else if (name_portion == "master_port") {
    oss.str(""); oss << "m_request_ports"           << "[" << index << "]"; table.push_back(oss.str());
    oss.str(""); oss << "m_respond_exports"         << "[" << index << "]"; table.push_back(oss.str());
  }

  return table;
}

void 
xtsc_component::xtsc_axi2pif_transactor::reset(bool /* hard_reset */)
{
  XTSC_TRACE(m_text, "Enter ::reset()");
  XTSC_INFO(m_text, "reset() called");

  m_next_request_port_num             = 0;
  m_next_read_response_port_num       = 0;
  m_next_write_response_port_num      = 0;

  for (u32 port_num=0; port_num<m_num_ports; ++port_num) {

    m_request_thread_event            [port_num]->cancel();
    m_read_response_thread_event      [port_num]->cancel();
    m_write_response_thread_event     [port_num]->cancel();

  }

  m_tag_2_axi_trans_info_map.clear();

  clear_fifos();

  xtsc_reset_processes(m_process_handles);
}

void 
xtsc_component::xtsc_axi2pif_transactor::change_clock_period(u32 clock_period_factor) 
{
  XTSC_TRACE(m_text, "Enter ::change_clock_period(clock_period_factor:" << clock_period_factor << ")");

  m_clock_period = sc_get_time_resolution() * clock_period_factor;
  XTSC_INFO(m_text, "Clock period changed by factor: " << clock_period_factor << 
      " to " << m_clock_period);
}



void 
xtsc_component::xtsc_axi2pif_transactor::execute(
  const string&           cmd_line, 
  const vector<string>&   words,
  const vector<string>&   words_lc,
  ostream&                result) 
{
  XTSC_TRACE(m_text, "Enter ::execute(cmd_line:" << cmd_line << ", words[0]:" << 
                      words[0] << ")");

  ostringstream res;

  if (words[0] == "change_clock_period") {
    u32 clock_period_factor = xtsc_command_argtou32(cmd_line, words, 1);
    res << m_clock_period.value();
    change_clock_period(clock_period_factor);
  }

  else if (words[0] == "peek") {
    u32 port_num      = xtsc_command_argtou32(cmd_line, words, 1);
    u64 start_address = xtsc_command_argtou64(cmd_line, words, 2);
    u32 num_bytes     = xtsc_command_argtou32(cmd_line, words, 3);
    u8 *buffer = new u8[num_bytes];
    m_xtsc_read_request_if_impl[port_num]->nb_peek(start_address, num_bytes, buffer);
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
    u32 port_num      = xtsc_command_argtou32(cmd_line, words, 1);
    u64 start_address = xtsc_command_argtou64(cmd_line, words, 2);
    u32 num_bytes     = xtsc_command_argtou32(cmd_line, words, 3);
    if (words.size() != num_bytes+4) {
      ostringstream oss;
      oss << "Command '" << cmd_line << "' specifies num_bytes=" << num_bytes << " but contains " << (words.size()-4)
          << " byte arguments.";
      throw xtsc_exception(oss.str());
    }
    if (num_bytes) {
      u8 *buffer = new u8[num_bytes];
      for (u32 i=0; i<num_bytes; ++i) {
        buffer[i] = (u8) xtsc_command_argtou32(cmd_line, words, 4+i);
      }
      m_xtsc_write_request_if_impl[port_num]->nb_poke(start_address, num_bytes, buffer);
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

bool 
xtsc_component::xtsc_axi2pif_transactor::prepare_to_switch_sim_mode(xtsc_sim_mode mode) 
{
  string str = (mode==xtsc::XTSC_CYCLE_ACCURATE) ? "XTSC_CYCLE_ACCURATE" : "XTSC_FUNCTIONAL";
  XTSC_TRACE(m_text, "Enter ::prepare_to_switch_sim_mode(mode:" << str << ")");

  m_mode_switch_pending = true;

  for (u32 i=0; i < m_num_ports; ++i) {
    if (!m_read_request_fifo[i]->empty()) {
      XTSC_INFO(m_text, "prepare_to_switch_sim_mode() returning false because m_read_request_fifo"   << "[" << i << "] not empty");
      return false;
    }
    if (!m_write_request_fifo[i]->empty()) {
      XTSC_INFO(m_text, "prepare_to_switch_sim_mode() returning false because m_write_request_fifo"  << "[" << i << "] not empty");
      return false;
    }
    if (!m_read_response_fifo[i]->empty()) {
      XTSC_INFO(m_text, "prepare_to_switch_sim_mode() returning false because m_read_response_fifo"  << "[" << i << "] not empty");
      return false;
    }    
    if (!m_write_response_fifo[i]->empty()) {
      XTSC_INFO(m_text, "prepare_to_switch_sim_mode() returning false because m_write_response_fifo" << "[" << i << "] not empty");
      return false;
    }    
  }

  XTSC_INFO(m_text, "prepare_to_switch_sim_mode() successful");
  return true;
}

bool 
xtsc_component::xtsc_axi2pif_transactor::switch_sim_mode(xtsc_sim_mode mode)
{
  string str = (mode==xtsc::XTSC_CYCLE_ACCURATE) ? "XTSC_CYCLE_ACCURATE" : "XTSC_FUNCTIONAL";
  XTSC_TRACE(m_text, "Enter ::switch_sim_mode(mode:" << str << ")");

  m_sim_mode            = mode;
  m_mode_switch_pending = false;

  XTSC_INFO(m_text, "switch_sim_mode() successful");
  return true;
}

/************* threads ***********/

void 
xtsc_component::xtsc_axi2pif_transactor::request_thread(void) 
{
  XTSC_TRACE(m_text, "Enter ::request_thread()");

  // Incrementing port_num each time we instantiate a new request thread.
  // The threads are instantiated with port_num=0,1,2,...
  u32 port_num = m_next_request_port_num++;

  assert(port_num < m_num_ports);
  
  bool drop_on_floor = false;
  xtsc_response::status_t status = xtsc_response::RSP_OK;

  try {

    while (true) {

      //It is possible that by the time the previous request was completed,
      //more requests have arrived. Till fifo is non empty, all requests will be served
      //
      // Cases handles below:
      // If write fifo is not empty, keep going
      // If read fifo is not empty, check if write is active right now. If yes,
      // let write complete first.
      // If write is active and write fifo is empty, dont go even if read fifo is not empty.
      // Wait for write requests to arrive
      while ((!m_read_request_fifo[port_num]->empty() && !m_write_request_active[port_num]) || !m_write_request_fifo[port_num]->empty()) {

        XTSC_DEBUG(m_text, "Reenter request_thread: Port #" << port_num);

        drop_on_floor = false;
        status = xtsc_response::RSP_OK;

        // Check which request will go now
        axi_req_info *i_axi_req_info = get_next_axi_request(port_num);

        //Wait if current request sched time is not arrived
        if (i_axi_req_info->m_sched_time > sc_time_stamp()) {
          XTSC_DEBUG(m_text, "Wait scheduled: " << i_axi_req_info->m_sched_time - sc_time_stamp());
          wait(i_axi_req_info->m_sched_time - sc_time_stamp());
        }
       
        xtsc_request *p_axi_request = i_axi_req_info->m_axi_request;

        if (m_non_secure_mode && check_non_secure_access(p_axi_request)) {
          if (p_axi_request->get_last_transfer()) {
            status = xtsc_response::RSP_DATA_ERROR;
            XTSC_WARN(m_text, *p_axi_request << " RSP_DATA_ERROR: Non-Secure Access to a secured address range");
          } else {
            drop_on_floor = true;
            status        = xtsc_response::RSP_DATA_ERROR;
          }
        }

        if (status != xtsc_response::RSP_OK) {
          bool is_rd = is_read_access(p_axi_request);

          if (drop_on_floor) {
            XTSC_INFO(m_text, "Dropping " << xtsc_response::get_status_name(status, true) << ": " << p_axi_request);
          } else {
            axi_trans_info *i_axi_trans_info = new axi_trans_info;
            i_axi_trans_info->m_axi_request = p_axi_request;
            i_axi_trans_info->m_is_nsm_error = true;
            i_axi_trans_info->m_is_read = is_rd;

            m_tag_2_axi_trans_info_map[p_axi_request->get_tag()].push(i_axi_trans_info);

            if (i_axi_trans_info->m_is_read) {
              m_read_response_thread_event[port_num]->notify();

              check_rd_nsm_rsp_id(port_num);
            } else {
              m_write_response_thread_event[port_num]->notify();

              check_wr_nsm_rsp_id(port_num);
            }
          }

          if (is_rd) {
            assert(m_axi_request_order_fifo[port_num]->front() == m_read_request_fifo[port_num]->front()->m_axi_request->get_tag());
            m_read_request_fifo[port_num]->pop();
          } else {
            assert(m_axi_request_order_fifo[port_num]->front() == m_write_request_fifo[port_num]->front()->m_axi_request->get_tag());
            m_write_request_fifo[port_num]->pop();
          }

          if (drop_on_floor) {
            delete_request(p_axi_request);
          } else {
            m_axi_request_order_fifo[port_num]->pop();
          }

          delete i_axi_req_info;
        } else {
          handle_axi_request(i_axi_req_info, port_num);
        }
      }

      XTSC_DEBUG(m_text, "request_thread[" << port_num << "] going to sleep.");
      wait(*m_request_thread_event[port_num]);
      XTSC_DEBUG(m_text, "request_thread[" << port_num << "] woke up.");
    } 
  }
  catch (const sc_unwind_exception& error) { throw; }
  catch (const exception& error) {
    ostringstream oss;
    oss << "std::exception caught in request_thread[" << port_num << "] of " << kind() << " '" <<
            name() << "'." << endl;
    oss << "what(): " << error.what() << endl;
    xtsc_log_multiline(m_text, FATAL_LOG_LEVEL, oss.str(), 2);
    throw;
  }
}

void 
xtsc_component::xtsc_axi2pif_transactor::read_response_thread(void) 
{
  XTSC_TRACE(m_text, "Enter ::read_response_thread()");
  // Get the port number for this "instance" of request_thread
  u32 port_num = m_next_read_response_port_num++;
  ostringstream oss;

  try {

    // Loop forever
    while (true) {

      while (!m_read_response_fifo[port_num]->empty() || (m_rd_nsm_vld[port_num])) {
        XTSC_DEBUG(m_text, "read_response_thread[" << port_num << "] get front from read_response_fifo of size " << 
                           m_read_response_fifo[port_num]->size() << ".");

        if (m_rd_nsm_vld[port_num]) {
          assert(m_non_secure_mode);
          send_rd_nsm_rsps(port_num);
        } else {
          axi_rsp_trans_info *i_axi_rsp_trans_info = m_read_response_fifo[port_num]->front();
          xtsc_response *p_axi_response = i_axi_rsp_trans_info->m_axi_response;
          
          if (i_axi_rsp_trans_info->m_sched_time > sc_time_stamp()) {
            XTSC_DEBUG(m_text, "read_response_thread[" << port_num << "] wait for " << 
                                i_axi_rsp_trans_info->m_sched_time - sc_time_stamp());
            wait(i_axi_rsp_trans_info->m_sched_time - sc_time_stamp());
          }

          send_read_response(*p_axi_response, port_num);

          m_tag_2_axi_trans_info_map[i_axi_rsp_trans_info->m_axi_request_tag].front()->m_num_axi_resp_sent++;
          m_read_response_fifo[port_num]->pop();

          delete_axi_rsp(i_axi_rsp_trans_info, port_num);
          
          wait(m_clock_period);
        }
          
        check_rd_nsm_rsp_id(port_num);
      }

      XTSC_DEBUG(m_text, "read_response_thread[" << port_num << "] going to sleep.");
      wait(*m_read_response_thread_event[port_num]);
      XTSC_DEBUG(m_text, "read_response_thread[" << port_num << "] woke up.");

    }
  }
  catch (const sc_unwind_exception& error) { throw; }
  catch (const exception& error) {
    ostringstream oss;
    oss << "std::exception caught in read_response_thread[" << port_num << "] of " << kind() << " '" 
        << name() << "'." << endl;
    oss << "what(): " << error.what() << endl;
    xtsc_log_multiline(m_text, FATAL_LOG_LEVEL, oss.str(), 2);
    throw;
  }
}

void xtsc_component::xtsc_axi2pif_transactor::write_response_thread(void) 
{
  XTSC_TRACE(m_text, "Enter ::write_response_thread()");
  // Get the port number for this "instance" of request_thread
  u32 port_num = m_next_write_response_port_num++;
  ostringstream oss;

  try {

    // Loop forever
    while (true) {

      while (!m_write_response_fifo[port_num]->empty() || (m_wr_nsm_vld[port_num])) {

        XTSC_DEBUG(m_text, "write_response_thread[" << port_num << "] get front from write_response_fifo of size " << 
                           m_write_response_fifo[port_num]->size() << ".");

        if (m_wr_nsm_vld[port_num]) {
          assert(m_non_secure_mode);
          send_wr_nsm_rsps(port_num);
        } else {
          axi_rsp_trans_info *i_axi_rsp_trans_info = m_write_response_fifo[port_num]->front();
          xtsc_response *p_axi_response = i_axi_rsp_trans_info->m_axi_response;
          
          if (i_axi_rsp_trans_info->m_sched_time > sc_time_stamp()) {
            XTSC_DEBUG(m_text, "write_response_thread[" << port_num << "] wait for " << 
                                i_axi_rsp_trans_info->m_sched_time - sc_time_stamp());
            wait(i_axi_rsp_trans_info->m_sched_time - sc_time_stamp());
          }

          send_write_response(*p_axi_response, port_num);
          m_write_response_fifo[port_num]->pop();

          delete_axi_rsp(i_axi_rsp_trans_info, port_num);
          
          wait(m_clock_period);
        }

        check_wr_nsm_rsp_id(port_num);
      }

      XTSC_DEBUG(m_text, "write_response_thread[" << port_num << "] going to sleep.");
      wait(*m_write_response_thread_event[port_num]);
      XTSC_DEBUG(m_text, "write_response_thread[" << port_num << "] woke up.");

    }
  }
  catch (const sc_unwind_exception& error) { throw; }
  catch (const exception& error) {
    ostringstream oss;
    oss << "std::exception caught in write_response_thread[" << port_num << "] of " << kind() << " '" 
        << name() << "'." << endl;
    oss << "what(): " << error.what() << endl;
    xtsc_log_multiline(m_text, FATAL_LOG_LEVEL, oss.str(), 2);
    throw;
  }
}

/****************** xtsc functions to send pif requests ********************/

// handle_axi_request
void xtsc_component::xtsc_axi2pif_transactor::handle_axi_request(axi_req_info *i_axi_req_info, u32 port_num) 
{
  XTSC_TRACE(m_text, "Enter ::handle_axi_request: tag=" << i_axi_req_info->m_axi_request->get_tag() << ", Port #" << port_num);
       
  xtsc_request *p_axi_request = i_axi_req_info->m_axi_request;

  // For reads, AXI request will be converted into pif requests, and stored in i_axi_trans_info->m_pif_requests_vec
  // For writes, each write request for a transaction is picked up from m_write_request_fifo, converted into
  // pif request, stored in m_pif_requests_vec and sent for scheduling
  assert(i_axi_req_info->m_pif_conv_pending);
  axi_trans_info *i_axi_trans_info = (i_axi_req_info->m_pif_conv_pending)
    ? conv_rw_axi2pif_request(p_axi_request, port_num)
    : m_tag_2_axi_trans_info_map[p_axi_request->get_tag()].front();

  assert(i_axi_trans_info != nullptr);

  XTSC_DEBUG(m_text, "i_axi_trans_info: " << i_axi_trans_info << ", tag=" << p_axi_request->get_tag());

  i_axi_req_info->m_pif_conv_pending = false;
  
  // For write, this should loop just once since each AXI request is converted into one pif request
  while (!i_axi_trans_info->m_pif_requests_vec.empty()) {
    xtsc_request *p_pif_request = i_axi_trans_info->m_pif_requests_vec.front();
    assert(p_pif_request != nullptr);
    assert(p_pif_request->get_transfer_number() >= 1);

    bool is_wr = !(i_axi_trans_info->m_is_read);

    if (!m_write_request_active[port_num] && is_wr) {
      XTSC_DEBUG(m_text, "Write Request Active[" << port_num << "] with tag=" << m_write_request_active_tag[port_num]);
      m_write_request_active[port_num] = true;
      m_write_request_active_tag[port_num] = p_axi_request->get_tag();

      // Extra check to confirm its first request for axi
      assert(p_pif_request->get_tag() == i_axi_trans_info->m_axi_request->get_tag());
      assert(p_pif_request->get_transfer_number() == 1);
    }

    i_axi_trans_info->m_pif_reqs_sent++;
    i_axi_trans_info->m_pif_requests_vec.pop();

    XTSC_DEBUG(m_text, "Popping pif_request with tag=" << p_pif_request->get_tag() << ", thus new m_pif_requests_vec.size() = " << i_axi_trans_info->m_pif_requests_vec.size() << ", pif_reqs_sent: " << i_axi_trans_info->m_pif_reqs_sent);

    if (i_axi_trans_info->m_pif_requests_vec.empty()) {
      if (is_wr) {
        assert(m_write_request_active[port_num] == true);
        assert(m_write_request_active_tag[port_num] == i_axi_trans_info->m_axi_request->get_tag());

        //only on last write
        if (i_axi_trans_info->m_pif_reqs_sent == p_axi_request->get_num_transfers()) {
          assert(i_axi_trans_info->m_total_pif_requests == i_axi_trans_info->m_pif_reqs_sent);

          XTSC_DEBUG(m_text, "Disable Write Request Active[" << port_num << "] with tag=" << m_write_request_active_tag[port_num]);
          i_axi_trans_info->m_all_pif_req_sent   = true;
          m_write_request_active[port_num]       = false;
          m_write_request_active_tag[port_num]   = 0;
          XTSC_DEBUG(m_text, "Popping m_axi_request_order_fifo[" << port_num << "]->front() " << m_axi_request_order_fifo[port_num]->front());
          m_axi_request_order_fifo[port_num]->pop();
        }
        
        XTSC_DEBUG(m_text, "Deleting axi_req_info: " << i_axi_req_info << " with tag=" << i_axi_req_info->m_axi_request->get_tag());
        if (p_axi_request->get_transfer_number() > 1) {
          delete_request(p_axi_request);
        }
        delete i_axi_req_info;
      } else {
        XTSC_DEBUG(m_text, "Popping m_axi_request_order_fifo[" << port_num << "]->front() " << m_axi_request_order_fifo[port_num]->front());
        assert(m_axi_request_order_fifo[port_num]->front() == m_read_request_fifo[port_num]->front()->m_axi_request->get_tag());

        XTSC_DEBUG(m_text, "Deleting axi_req_info: " << i_axi_req_info << " with tag=" << i_axi_req_info->m_axi_request->get_tag());
        delete i_axi_req_info;
        
        m_axi_request_order_fifo[port_num]->pop();
      }
    }

    handle_pif_request(p_pif_request, port_num);

    if (i_axi_trans_info->m_pif_requests_vec.empty()) {
      if (is_wr) {
        m_write_request_fifo[port_num]->pop();
      } else {
        m_read_request_fifo[port_num]->pop();
      }
    }

    delete_request(p_pif_request);
  }
}


// handle_pif_request
void xtsc_component::xtsc_axi2pif_transactor::handle_pif_request(xtsc_request *p_pif_request, u32 port_num) 
{
  XTSC_TRACE(m_text, "Enter ::handle_pif_request: tag=" << p_pif_request->get_tag() << ", Port #" << port_num);

  xtsc_address          address8        = p_pif_request->get_byte_address();
  u32                   size8           = p_pif_request->get_byte_size();
  u64                   pif_tag         = p_pif_request->get_tag();
  u32                   num_transfers   = p_pif_request->get_num_transfers();
  xtsc_byte_enables     byte_enables    = p_pif_request->get_byte_enables();
  u32                   route_id        = p_pif_request->get_route_id();
  u8                    id              = p_pif_request->get_id();
  u8                    priority        = p_pif_request->get_priority();
  xtsc_address          pc              = p_pif_request->get_pc();
  bool                  wrap            = false;  
  string                type_name       = (string)p_pif_request->get_type_name();
  bool                  is_block        = (type_name == "BLOCK_READ" || type_name == "BLOCK_WRITE");

  if (type_name != "READ" && type_name != "BLOCK_READ" && type_name != "WRITE" && type_name != "BLOCK_WRITE") {
    ostringstream oss;
    oss << kind() << " '" << name() << "' Incorrect type_name: " << type_name << "." << endl;
    throw xtsc_exception(oss.str());
  }

  if (is_block && (address8 % size8 != 0)) {
    ostringstream oss;
    oss << kind() << " '" << name() << "' Unaligned transfer for request type : " << type_name << endl;
    throw xtsc_exception(oss.str());
  }

  if (is_block && (size8 != m_pif_width8)) {
    ostringstream oss;
    oss << kind() << " '" << name() << "' Unsupported size of transfer : " << size8 << 
        " for request type : " << type_name << endl;
    throw xtsc_exception(oss.str());
  }

  // If m_outstanding_pif_request_ids is set to a non-zero integer, then we have limit for request ids
  // which can be sent forward. If limit is reached, we wait till any request id is freed.
  if (m_outstanding_pif_request_ids > 0) {
    xtsc::u32 m_pif_reqs_pending = is_read_access(p_pif_request)
      ? m_num_read_pif_requests_pending[port_num]
      : m_num_write_pif_requests_pending[port_num];

    while (m_pif_reqs_pending == m_outstanding_pif_request_ids) {
      wait(m_clock_period);
    }

    set_next_req_id(p_pif_request, port_num);
  }

  send_pif_req_worker(p_pif_request, port_num);
} 

void xtsc_component::xtsc_axi2pif_transactor::send_pif_req_worker(xtsc_request *p_pif_request, u32 port_num) {
  XTSC_TRACE(m_text, "Enter ::send_pif_req_worker: tag=" << p_pif_request->get_tag() << ", Port #" << port_num);

  //Send the request and wait 1 cycle for any potential RSP_NACC
  m_pif_waiting_for_nacc[port_num] = true;
  u32 try_count = 0;
  do {
    m_pif_request_got_nacc[port_num] = false;
    try_count++;
    XTSC_INFO(m_text, *p_pif_request << " Port #" << port_num << "  Try #" << try_count);
    (*m_request_ports[port_num])->nb_request(*p_pif_request);
    wait(m_nacc_wait_time);
  } while (m_pif_request_got_nacc[port_num]);
  m_pif_waiting_for_nacc[port_num] = false;
} 

// Converting axi request into corresponding pif request(s)
xtsc_component::xtsc_axi2pif_transactor::axi_trans_info * 
xtsc_component::xtsc_axi2pif_transactor::conv_rw_axi2pif_request(xtsc::xtsc_request* p_axi_request, xtsc::u32 port_num) 
{
  axi_trans_info *i_axi_trans_info = nullptr;
  if (is_read_access(p_axi_request)) {
    i_axi_trans_info = conv_read_axi2pif_request(p_axi_request, port_num);
  } else {
    i_axi_trans_info = conv_write_axi2pif_request(p_axi_request, port_num);
  }

  return i_axi_trans_info;
}

xtsc_component::xtsc_axi2pif_transactor::axi_trans_info *
xtsc_component::xtsc_axi2pif_transactor::conv_read_axi2pif_request(xtsc_request *p_axi_request, u32 port_num)
{
  // Protocol constraints
  // AXI
  // num_transfers= AXI3 => 2-16, AXI4 => 2-256
  // If WRAP, num_transfers <= 16
  // 
  // PIF
  // Unaligned is w.r.t. byte_size. Unaligned means address is aligned but "be" are used
  // For READ/WRITE, can be unaligned starting address
  // For BLOCK_RW, byte_size=axi_byte_width, num_transfers=2,4,8,16
  // For BLOCK_READ, be all set
  // For BLOCK_WRITE, should be aligned to block size
  // For BURST_RW, byte_size=data_width, num_transfers=2-16
  // For BURST_READ, unaligned_start_addr only
  // For BURST_WRITE, both unaligned start/end address
  // 
  // Conversion criteria
  // Default
  // BLOCK_RW 
  // if num_transfers=2,4,8,16,32,48,64,80,96,...,256
  //    address is aligned to axi_byte_width for read and block start address for write
  //    byte_size should be axi_byte_width
  // Else single PIF requests

  XTSC_TRACE(m_text, "Enter ::conv_read_axi2pif_request(tag=" << p_axi_request->get_tag() << ", Port #" << port_num);

  xtsc_address          address8                    = p_axi_request->get_byte_address();
  u32                   size8                       = p_axi_request->get_byte_size();
  u64                   axi_tag                     = p_axi_request->get_tag();
  u32                   num_transfers               = p_axi_request->get_num_transfers();
  xtsc_byte_enables     byte_enables                = p_axi_request->get_byte_enables();
  u32                   route_id                    = p_axi_request->get_route_id();
  u8                    id                          = p_axi_request->get_id();
  u8                    priority                    = p_axi_request->get_priority();
  xtsc_address          pc                          = p_axi_request->get_pc();
  string                type_name                   = (string)p_axi_request->get_type_name();
  xtsc_request::burst_t burst_type                  = p_axi_request->get_burst_type();
  u8                    prot_attr                   = p_axi_request->get_prot();
  
  //Utility calculations
  //General
  u32                   total_bytes_to_transfer     = size8 * num_transfers;
  bool                  num_transfers_2_4_8_16      = (num_transfers == 2) || (num_transfers == 4) || (num_transfers == 8) || (num_transfers == 16);
  
  //WRAP
  bool                  is_wrap                     = (burst_type == xtsc_request::WRAP);
  xtsc_address          lower_wrap_addr             = (address8 / total_bytes_to_transfer) * total_bytes_to_transfer;
  xtsc_address          upper_wrap_addr             = lower_wrap_addr + total_bytes_to_transfer - 1;
 
  //General
  u32                   max_num_xfers               = 16; //For PIF burst
  xtsc_byte_enables     be                          = 0xFFFFFFFFFFFFFFFFull >> (64-size8);

  if (is_wrap) {
    //For AXI WRAP, num_transfers can be 2,4,8 or 16
    assert(num_transfers_2_4_8_16);
  }

  assert(burst_type == xtsc_request::INCR || burst_type == xtsc_request::WRAP);

  bool is_normal_block = false;

  // Check if we can get block pifs
  bool vld_num_transfers = (num_transfers > 16)
    ? (num_transfers%16 == 0) // Multiple block reads of 16 transfers each
    : num_transfers_2_4_8_16; // Single block transfer 
  bool addr_aligned      = (address8 == lower_wrap_addr) 
    || (m_critical_word_first 
      ? false 
      : (is_wrap && (address8 == m_pif_width8)));
  bool byte_size_check   = (size8 == m_pif_width8) && (size8 <= m_max_block_pif_byte_size);

  if (vld_num_transfers && addr_aligned && byte_size_check) {
    is_normal_block      = true;
  } else {
    XTSC_DEBUG(m_text, "Block not possible as vld_num_transfers: " << vld_num_transfers << ", addr_aligned: " << addr_aligned << ", size8: " << size8 << ", m_pif_width8: " << m_pif_width8);
  }

  axi_trans_info *i_axi_trans_info                     = new axi_trans_info();
  i_axi_trans_info->m_is_read                          = true;
  i_axi_trans_info->m_axi_request                      = p_axi_request;

  XTSC_DEBUG(m_text, "Created new axi_trans_info: " << i_axi_trans_info << " with tag=" << axi_tag);

  xtsc_address curr_address8                           = address8;
  u32 total_data_pending                               = total_bytes_to_transfer;
  bool first_pif_req                                   = true;

  while(total_data_pending > 0) 
  {
    xtsc_request *p_pif_request                        = new_request();
    xtsc::u64 curr_pif_tag                             = first_pif_req ? axi_tag : 0;

    if (is_normal_block) {
      if (num_transfers <= 16) {
        p_pif_request->initialize(
            xtsc_request::BLOCK_READ,   //type_t            type
            curr_address8,              //xtsc_address      address8
            size8,                      //u32               size8
            axi_tag,                    //u64               tag
            num_transfers,              //u32               num_transfers
            byte_enables,               //xtsc_byte_enables byte_enables
            true,                       //bool              last_transfer
            route_id,                   //u32               route_id
            id,                         //u8                id
            priority,                   //u8                priority
            pc,                         //xtsc_address      pc
            xtsc_request::NON_AXI);     //burst_t           burst

        assert(total_data_pending == total_bytes_to_transfer);

        first_pif_req                             = false;
        total_data_pending                        = 0;

        i_axi_trans_info->m_num_pif_trans++;
        i_axi_trans_info->m_total_pif_requests    = num_transfers;
        i_axi_trans_info->m_num_pif_reqs_vec.push(num_transfers);
        i_axi_trans_info->m_pif_requests_tag_vec.push(axi_tag);
      } else {
        p_pif_request->initialize(
            xtsc_request::BLOCK_READ,   //type_t            type
            curr_address8,              //xtsc_address      address8
            size8,                  //u32               size8
            curr_pif_tag,               //u64               tag
            16,                         //u32               num_transfers
            byte_enables,               //xtsc_byte_enables byte_enables
            true,                       //bool              last_transfer
            route_id,                   //u32               route_id
            id,                         //u8                id
            priority,                   //u8                priority
            pc,                         //xtsc_address      pc
            xtsc_request::NON_AXI);     //burst_t           burst

        assert(total_data_pending >= 16*size8);

        first_pif_req                            = false;
        total_data_pending                      -= 16*size8;
        curr_address8                           += 16*size8;

        i_axi_trans_info->m_num_pif_trans++;
        i_axi_trans_info->m_total_pif_requests  += 16;
        i_axi_trans_info->m_num_pif_reqs_vec.push(16);
        i_axi_trans_info->m_pif_requests_tag_vec.push(p_pif_request->get_tag());
      }
    } else {
      p_pif_request->initialize(
          xtsc_request::READ,         //type_t            type
          curr_address8,              //xtsc_address      address8
          size8,                      //u32               size8
          curr_pif_tag,               //u64               tag
          1,                          //u32               num_transfers
          be,                         //xtsc_byte_enables byte_enables
          true,                       //bool              last_transfer
          route_id,                   //u32               route_id
          id,                         //u8                id
          priority,                   //u8                priority
          pc,                         //xtsc_address      pc
          xtsc_request::NON_AXI);     //burst_t           burst

      assert(total_data_pending > 0);

      first_pif_req                             = false;
      total_data_pending                       -= size8;
      curr_address8                            += size8;

      if (is_wrap) {
        curr_address8 = (curr_address8 > upper_wrap_addr) ? lower_wrap_addr : curr_address8;
      }

      i_axi_trans_info->m_num_pif_trans++;
      i_axi_trans_info->m_total_pif_requests++;
      i_axi_trans_info->m_num_pif_reqs_vec.push(1);
      i_axi_trans_info->m_pif_requests_tag_vec.push(p_pif_request->get_tag());
    }

    p_pif_request->set_instruction_fetch(p_axi_request->get_instruction_fetch());
    p_pif_request->set_exclusive(false);
    
    m_tag_2_axi_trans_info_map[p_pif_request->get_tag()].push(i_axi_trans_info);
    XTSC_DEBUG(m_text, "Created new tag xtsc_request: " << p_pif_request << ": " << *p_pif_request << " from rd tag=" << axi_tag << ", Count #" << i_axi_trans_info->m_num_pif_trans); 
    XTSC_TRACE(m_text, "map size: " << m_tag_2_axi_trans_info_map[p_pif_request->get_tag()].size());

    i_axi_trans_info->m_pif_requests_vec.push(p_pif_request);
  }

  XTSC_DEBUG(m_text, "i_axi_trans_info details -> Num PIF transactions: " << i_axi_trans_info->m_num_pif_trans << ", Num PIF requests: " << i_axi_trans_info->m_total_pif_requests);

  return i_axi_trans_info;
}

xtsc_component::xtsc_axi2pif_transactor::axi_trans_info *
xtsc_component::xtsc_axi2pif_transactor::conv_write_axi2pif_request(xtsc_request *p_axi_request, u32 port_num)
{
  // Protocol constraints
  // AXI
  // num_transfers= AXI3 => 2-16, AXI4 => 2-256
  // If WRAP, num_transfers <= 16
  // 
  // PIF
  // Unaligned is w.r.t. byte_size. Unaligned means address is aligned but "be" are used
  // For READ/WRITE, can be unaligned starting address
  // For BLOCK_RW, byte_size=axi_byte_width, num_transfers=2,4,8,16
  // For BLOCK_READ, be all set
  // For BLOCK_WRITE, should be aligned to block size
  // For BURST_RW, byte_size=data_width, num_transfers=2-16
  // For BURST_READ, unaligned_start_addr only
  // For BURST_WRITE, both unaligned start/end address
  // 
  // Conversion criteria
  // Default
  // BLOCK_RW 
  // if num_transfers=2,4,8,16,32,48,64,80,96,...,256
  //    address is aligned to axi_byte_width for read and block start address for write
  //    byte_size should be axi_byte_width
  // Else single PIF requests

  XTSC_TRACE(m_text, "Enter ::conv_write_axi2pif_request(tag=" << p_axi_request->get_tag() << ", Port #" << port_num);

  xtsc_address          address8                    = p_axi_request->get_byte_address();
  u32                   size8                       = p_axi_request->get_byte_size();
  u64                   axi_tag                     = p_axi_request->get_tag();
  u32                   num_transfers               = p_axi_request->get_num_transfers();
  u32                   transfer_num                = p_axi_request->get_transfer_number();
  xtsc_byte_enables     byte_enables                = p_axi_request->get_byte_enables();
  u32                   route_id                    = p_axi_request->get_route_id();
  u8                    id                          = p_axi_request->get_id();
  u8                    priority                    = p_axi_request->get_priority();
  xtsc_address          pc                          = p_axi_request->get_pc();
  string                type_name                   = (string)p_axi_request->get_type_name();
  xtsc_request::burst_t burst_type                  = p_axi_request->get_burst_type();
  u8                    prot_attr                   = p_axi_request->get_prot();
  
  //Utility calculations
  //General
  u32                   total_bytes_to_transfer     = size8 * num_transfers;
  bool                  num_transfers_2_4_8_16      = (num_transfers == 2) || (num_transfers == 4) || (num_transfers == 8) || (num_transfers == 16);
  
  //WRAP
  bool                  is_wrap                     = (burst_type == xtsc_request::WRAP);
  xtsc_address          lower_wrap_addr             = (address8 / total_bytes_to_transfer) * total_bytes_to_transfer;
  xtsc_address          upper_wrap_addr             = lower_wrap_addr + total_bytes_to_transfer - 1;
 
  //General
  u32                   max_num_xfers               = 16; //For PIF burst
  xtsc_byte_enables     be                          = 0xFFFFFFFFFFFFFFFFull >> (64-size8);

  if (is_wrap) {
    //For AXI WRAP, num_transfers can be 2,4,8 or 16
    assert(num_transfers_2_4_8_16);
  }

  assert(burst_type == xtsc_request::INCR || burst_type == xtsc_request::WRAP);

  axi_trans_info *i_axi_trans_info                  = nullptr;
  // If its second to last beats for a previous transaction
  // m_write_request_fifo already has the first request stored
  if (transfer_num == 1) {
    i_axi_trans_info                                = new axi_trans_info();
    i_axi_trans_info->m_is_read                     = false;
    i_axi_trans_info->m_axi_request                 = p_axi_request;

    XTSC_DEBUG(m_text, "Created new axi_trans_info: " << i_axi_trans_info << " with tag=" << axi_tag);
  } else {
    i_axi_trans_info                                = m_tag_2_axi_trans_info_map[axi_tag].back();
    
    assert(i_axi_trans_info != nullptr);
    assert(i_axi_trans_info->m_axi_request->get_tag() == p_axi_request->get_tag());

    XTSC_DEBUG(m_text, "Selected axi_trans_info: " << i_axi_trans_info << " with tag=" << axi_tag);
  }

  assert(i_axi_trans_info->m_pif_requests_vec.empty());

  // Just for testing purpose
  i_axi_trans_info->m_curr_axi_transfer_num++;
  assert(transfer_num == i_axi_trans_info->m_curr_axi_transfer_num);

  if (transfer_num == 1) {
    bool vld_num_transfers                         = (num_transfers > 16)
      ? (num_transfers%16 == 0) // Multiple block reads of 16 transfers each
      : num_transfers_2_4_8_16; // Single block transfer 
    bool addr_aligned                              = (address8 == lower_wrap_addr);
    bool byte_size_check                           = (size8 == m_pif_width8) && (size8 <= m_max_block_pif_byte_size);

    if (vld_num_transfers && addr_aligned && byte_size_check) {
      i_axi_trans_info->m_is_block_request         = true;
    } else {
      XTSC_DEBUG(m_text, "Block not possible as vld_num_transfers: " << vld_num_transfers << ", addr_aligned: " << addr_aligned << ", size8: " << size8 << ", m_pif_width8: " << m_pif_width8);
    }
  }

  xtsc_request *p_pif_request                    = new_request();
  xtsc::u64 curr_pif_tag                         = 0;

  if (i_axi_trans_info->m_is_block_request) {
    if (num_transfers <= 16) {

      bool first_transfer                      = (transfer_num == 1);
      bool last_transfer                       = (transfer_num == num_transfers);

      if (first_transfer) {
        p_pif_request->initialize(
            xtsc_request::BLOCK_WRITE,  //type_t            type
            address8,                   //xtsc_address      address8
            size8,                      //u32               size8
            axi_tag,                    //u64               tag
            num_transfers,              //u32               num_transfers
            byte_enables,               //xtsc_byte_enables byte_enables
            last_transfer,              //bool              last_transfer
            route_id,                   //u32               route_id
            id,                         //u8                id
            priority,                   //u8                priority
            pc,                         //xtsc_address      pc
            xtsc_request::NON_AXI);     //burst_t           burst

        i_axi_trans_info->m_num_pif_trans++;
        i_axi_trans_info->m_total_pif_requests += num_transfers;
        i_axi_trans_info->m_num_pif_reqs_vec.push(num_transfers);
        i_axi_trans_info->m_pif_requests_tag_vec.push(axi_tag);
      } else {
        p_pif_request->initialize(
            axi_tag,                    //u64               tag
            address8,                   //xtsc_address      address8
            size8,                      //u32               size8
            num_transfers,              //u32               num_transfers
            last_transfer,              //bool              last_transfer
            route_id,                   //u32               route_id
            id,                         //u8                id
            priority,                   //u8                priority
            pc);                        //xtsc_address      pc
      }
    } else {
      //First request should have transfer_num=1-16, Second 17-32, ...
      xtsc::u64 pif_req_num       = (transfer_num-1)/16 + 1;

      curr_pif_tag      = (transfer_num <= 16) ? axi_tag : 0;

      bool first_transfer         = (transfer_num%16 == 1);
      bool last_transfer          = (transfer_num%16 == 0);

      if (first_transfer) {
        p_pif_request->initialize(
            xtsc_request::BLOCK_WRITE,  //type_t            type
            address8,                   //xtsc_address      address8
            size8,                      //u32               size8
            curr_pif_tag,               //u64               tag
            16,                         //u32               num_transfers
            byte_enables,               //xtsc_byte_enables byte_enables
            false,                      //bool              last_transfer
            route_id,                   //u32               route_id
            id,                         //u8                id
            priority,                   //u8                priority
            pc,                         //xtsc_address      pc
            xtsc_request::NON_AXI);     //burst_t           burst

        i_axi_trans_info->m_num_pif_trans++;
        i_axi_trans_info->m_total_pif_requests += 16;
        i_axi_trans_info->m_num_pif_reqs_vec.push(16);
        i_axi_trans_info->m_pif_requests_tag_vec.push(p_pif_request->get_tag());
      } else {
        curr_pif_tag = i_axi_trans_info->m_pif_requests_tag_vec.back();

        p_pif_request->initialize(
            curr_pif_tag,               //u64               tag
            address8,                   //xtsc_address      address8
            size8,                      //u32               size8
            16,                         //u32               num_transfers
            last_transfer,              //bool              last_transfer
            route_id,                   //u32               route_id
            id,                         //u8                id
            priority,                   //u8                priority
            pc);                        //xtsc_address      pc
      }
    }
  } else {

    curr_pif_tag              = (transfer_num == 1) ? axi_tag : 0;

    p_pif_request->initialize(
        xtsc_request::WRITE,    //type_t            type
        address8,               //xtsc_address      address8
        size8,                  //u32               size8
        curr_pif_tag,           //u64               tag
        1,                      //u32               num_transfers
        byte_enables,           //xtsc_byte_enables byte_enables
        true,                   //bool              last_transfer
        route_id,               //u32               route_id
        id,                     //u8                id
        priority,               //u8                priority
        pc,                     //xtsc_address      pc
        xtsc_request::NON_AXI); //burst_t           burst

    i_axi_trans_info->m_num_pif_trans++;
    i_axi_trans_info->m_total_pif_requests++;
    i_axi_trans_info->m_num_pif_reqs_vec.push(1);
    i_axi_trans_info->m_pif_requests_tag_vec.push(p_pif_request->get_tag());
  }

  curr_pif_tag   = p_pif_request->get_tag();
  p_pif_request->set_instruction_fetch(p_axi_request->get_instruction_fetch());
  p_pif_request->set_exclusive(false);

  if (p_pif_request->get_transfer_number() == 1) {
    m_tag_2_axi_trans_info_map[curr_pif_tag].push(i_axi_trans_info);
    XTSC_DEBUG(m_text, "Created new tag xtsc_request: " << p_pif_request << ": " << *p_pif_request << " from wr tag=" << axi_tag << ", Count #" << i_axi_trans_info->m_num_pif_trans); 
    XTSC_DEBUG(m_text, "map size: " << m_tag_2_axi_trans_info_map[p_pif_request->get_tag()].size());
  } else {
    assert(m_tag_2_axi_trans_info_map[curr_pif_tag].back() == i_axi_trans_info);
    XTSC_DEBUG(m_text, "Created new xtsc_request: " << *p_pif_request << " from wr tag=" << axi_tag << ", Count #" << i_axi_trans_info->m_num_pif_trans); 
  }
    
  memcpy(p_pif_request->get_buffer(), p_axi_request->get_buffer(), size8);  
  i_axi_trans_info->m_pif_requests_vec.push(p_pif_request);

  XTSC_DEBUG(m_text, "i_axi_trans_info details -> Num PIF transactions: " << i_axi_trans_info->m_num_pif_trans << ", Num PIF requests: " << i_axi_trans_info->m_total_pif_requests);

  return i_axi_trans_info;
}

xtsc_component::xtsc_axi2pif_transactor::axi_req_info *
xtsc_component::xtsc_axi2pif_transactor::get_next_axi_request(u32 port_num) 
{
  XTSC_TRACE(m_text, "Enter ::get_next_axi_request(" << port_num << ")");

  axi_req_info *i_axi_req_info = nullptr;

  xtsc::u32 rd_size            = m_read_request_fifo[port_num]->size();
  xtsc::u32 wr_size            = m_write_request_fifo[port_num]->size();

  if (rd_size == 0 && wr_size == 0) {
    assert(0);
  } else if (rd_size == 0) {
    i_axi_req_info             = m_write_request_fifo[port_num]->front();
  } else if (wr_size == 0) {
    if (m_write_request_active[port_num]) {
      return i_axi_req_info;
    }
    i_axi_req_info             = m_read_request_fifo[port_num]->front();
  } else {
    if (m_write_request_active[port_num]) {
      i_axi_req_info           = m_write_request_fifo[port_num]->front();
      return i_axi_req_info;
    }

    xtsc::u64 rd_tag           = m_read_request_fifo[port_num]->front()->m_axi_request->get_tag();
    xtsc::u64 wr_tag           = m_write_request_fifo[port_num]->front()->m_axi_request->get_tag();

    if (rd_tag == m_axi_request_order_fifo[port_num]->front()) {
      i_axi_req_info           = m_read_request_fifo[port_num]->front();
    } else if (wr_tag == m_axi_request_order_fifo[port_num]->front()) {
      i_axi_req_info           = m_write_request_fifo[port_num]->front();
    } else {
      assert(0);
    }
  }

  assert(i_axi_req_info != nullptr);

  XTSC_DEBUG(m_text, "Selected axi_req_info: " << i_axi_req_info << " with tag=" << i_axi_req_info->m_axi_request->get_tag() << ", rd_size: " << rd_size << ", wr_size: " << wr_size << ", m_write_request_active: " << m_write_request_active[port_num]);

  return i_axi_req_info;
}

void xtsc_component::xtsc_axi2pif_transactor::set_next_req_id(xtsc_request *p_pif_request, u32 port_num)
{
  XTSC_TRACE(m_text, "Enter ::set_next_req_id(request: " << *p_pif_request << ", port_num: " << port_num << ")");

  bool is_rd = is_read_access(p_pif_request);

  xtsc::u8 *m_last_req_id_selected = is_rd
    ? m_last_read_req_id_selected
    : m_last_write_req_id_selected;

  bool *m_request_id_pending = is_rd
    ? m_read_request_id_pending[port_num]
    : m_write_request_id_pending[port_num];

  xtsc::u32 *m_num_pif_requests_pending = is_rd
    ? m_num_read_pif_requests_pending
    : m_num_write_pif_requests_pending;

  xtsc::u8 offset = is_rd
    ? 32
    : 0;

  xtsc::u8 next_id = (m_last_req_id_selected[port_num] - offset);

  //If this is first request of a transaction, then set a new request id 
  if (p_pif_request->get_transfer_number() == 1) {
    for (xtsc::u8 i=0; i < m_outstanding_pif_request_ids; i++) {
      next_id = ((m_last_req_id_selected[port_num]-offset) +i+1)%m_outstanding_pif_request_ids;

      if (!m_request_id_pending[next_id]) {
        XTSC_DEBUG(m_text, "Selected next_req_id: " << next_id+offset << " for " << (is_rd ? "rd" : "wr") << " request with tag=" << p_pif_request->get_tag());
        assert(m_request_id_pending[next_id] == false);

        p_pif_request->set_id(next_id + offset);
        m_last_req_id_selected[port_num] = next_id + offset;
        m_request_id_pending[next_id] = true;
        m_num_pif_requests_pending[port_num]++;
        break;
      }
    }
  } else {
    p_pif_request->set_id(next_id + offset);
    XTSC_DEBUG(m_text, "Selected old next_req_id: " << next_id+offset << " for " << (is_rd ? "rd" : "wr") << " request with tag=" << p_pif_request->get_tag());
  }

  // v_num_pif_requests_pending is for testing purposes only
  xtsc::u32 v_num_pif_requests_pending = 0;
  string display_used_req_ids = "";
  for (xtsc::u8 i=0; i < m_outstanding_pif_request_ids; i++) {
    display_used_req_ids = (m_request_id_pending[i] ? "1" : "0") + display_used_req_ids;

    if (m_request_id_pending[i]) {
      v_num_pif_requests_pending++;
    }
  }

  XTSC_DEBUG(m_text, (is_rd ? "m_num_read_pif_requests_pending[" : "m_num_write_pif_requests_pending[") << port_num << "]: " << m_num_pif_requests_pending[port_num] << " with values: " << hex << display_used_req_ids);

  //Verification
  assert(v_num_pif_requests_pending == m_num_pif_requests_pending[port_num]);
}

void xtsc_component::xtsc_axi2pif_transactor::send_read_response(xtsc_response& response, u32 port_num) 
{
  XTSC_TRACE(m_text, "Enter ::send_read_response(response:" << response << ", port_num:" << port_num << ")");
  u32 try_count = 1;
  XTSC_INFO(m_text, response << " Port #" << port_num << "  Try #" << try_count);
  // Need to check return value because master might be busy
  while (!(*m_read_respond_ports[port_num])->nb_respond(response)) {
    XTSC_INFO(m_text, "nb_respond() returned false; waiting one clock period to try again.  Port #" << port_num);
    wait(m_clock_period);
    try_count++;
    XTSC_INFO(m_text, response << " Port #" << port_num << "  Try #" << try_count);
  }
}

void xtsc_component::xtsc_axi2pif_transactor::send_write_response(xtsc_response& response, u32 port_num) 
{
  XTSC_TRACE(m_text, "Enter ::send_write_response(response:" << response << ", port_num:" << port_num << ")");
  u32 try_count = 1;
  XTSC_INFO(m_text, response << " Port #" << port_num << "  Try #" << try_count);
  // Need to check return value because master might be busy
  while (!(*m_write_respond_ports[port_num])->nb_respond(response)) {
    XTSC_INFO(m_text, "nb_respond() returned false; waiting one clock period to try again.  Port #" << port_num);
    wait(m_clock_period);
    try_count++;
    XTSC_INFO(m_text, response << " Port #" << port_num << "  Try #" << try_count);
  }
}

xtsc_request *xtsc_component::xtsc_axi2pif_transactor::copy_request(const xtsc_request& request) {
  XTSC_TRACE(m_text, "Enter ::copy_request(request:" << request << ")");
  xtsc_request *p_request = NULL;
  if (m_request_pool.empty()) {
    m_request_count += 1;
    XTSC_TRACE(m_text, "Created new xtsc_request, count #" << m_request_count << ". Copy");
    p_request = new xtsc_request();
  }
  else {
    p_request = m_request_pool.back();
    m_request_pool.pop_back();
    XTSC_TRACE(m_text, "Used xtsc_request from pool, size #" << m_request_pool.size());
  }
  *p_request = request; //Copy contents
  return p_request;
}


xtsc_request *xtsc_component::xtsc_axi2pif_transactor::new_request() {
  XTSC_TRACE(m_text, "Enter ::new_request()");
  xtsc_request *p_request = NULL;
  if (m_request_pool.empty()) {
    m_request_count += 1;
    p_request = new xtsc_request();
    XTSC_TRACE(m_text, "Created new xtsc_request, count #" << m_request_count << ". New");
  }
  else {
    p_request = m_request_pool.back();
    m_request_pool.pop_back();
    XTSC_TRACE(m_text, "Used xtsc_request from pool, size #" << m_request_pool.size());
  }
  return p_request;
}

void xtsc_component::xtsc_axi2pif_transactor::delete_request(xtsc_request*& p_request) {
  XTSC_TRACE(m_text, "Enter ::delete_request(*p_request:" << *p_request << ")");
  m_request_pool.push_back(p_request);
  XTSC_TRACE(m_text, "Returned xtsc_request to pool, size #" << m_request_pool.size());
  p_request = 0;
}

void xtsc_component::xtsc_axi2pif_transactor::clear_fifos(void)
{
  XTSC_TRACE(m_text, "Enter ::clear_fifos()");
 
  for (u32 port_num=0; port_num < m_num_ports; port_num++) {
    while (!m_read_request_fifo[port_num]->empty()) {
      axi_req_info *i_axi_req_info = m_read_request_fifo[port_num]->front();
      m_read_request_fifo[port_num]->pop();
      XTSC_DEBUG(m_text, "Deleting axi_req_info: " << i_axi_req_info << " with tag=" << i_axi_req_info->m_axi_request->get_tag());
      delete i_axi_req_info;
    }
    
    while (!m_write_request_fifo[port_num]->empty()) {
      axi_req_info *i_axi_req_info = m_write_request_fifo[port_num]->front();
      m_write_request_fifo[port_num]->pop();
      XTSC_DEBUG(m_text, "Deleting axi_req_info: " << i_axi_req_info << " with tag=" << i_axi_req_info->m_axi_request->get_tag());
      delete i_axi_req_info;
    }
    
    while (!m_read_response_fifo[port_num]->empty()) {
      axi_rsp_trans_info *i_axi_rsp_trans_info = m_read_response_fifo[port_num]->front();
      m_read_response_fifo[port_num]->pop();
      XTSC_DEBUG(m_text, "Deleting axi_rsp_trans_info: " << i_axi_rsp_trans_info << " with tag=" << i_axi_rsp_trans_info->m_axi_request_tag);
      delete_axi_rsp(i_axi_rsp_trans_info, port_num);
      delete i_axi_rsp_trans_info;
    }
    
    while (!m_write_response_fifo[port_num]->empty()) {
      axi_rsp_trans_info *i_axi_rsp_trans_info = m_write_response_fifo[port_num]->front();
      m_write_response_fifo[port_num]->pop();
      XTSC_DEBUG(m_text, "Deleting axi_rsp_trans_info: " << i_axi_rsp_trans_info << " with tag=" << i_axi_rsp_trans_info->m_axi_request_tag);
      delete_axi_rsp(i_axi_rsp_trans_info, port_num);
      delete i_axi_rsp_trans_info;
    }
  }
}

bool 
xtsc_component::xtsc_axi2pif_transactor::check_non_secure_access(xtsc_request* p_request) 
{
  XTSC_TRACE(m_text, "check_non_secure_access : " << *p_request);
  // Determine whether or not xtsc request is inside a "secure_address_range" block:
  // For a non-secure access, flag it
  if (!(p_request->get_prot() & 0x2)) {
    return false;
  }

  xtsc_address  address8                = p_request->get_byte_address();
  u32           size8                   = p_request->get_byte_size();
  u32           num_transfers           = p_request->get_num_transfers();
  bool          axi                     = p_request->is_axi_protocol();
  u32           total_bytes_to_transfer = num_transfers * size8;
  xtsc_address  start_address8          = (address8 / total_bytes_to_transfer) * 
                                          total_bytes_to_transfer;
  xtsc_address  end_address8            = start_address8 + total_bytes_to_transfer - 1;

  for (u32 i=0; i<m_secure_address_range.size(); i+=2) {
    xtsc_address begin   = m_secure_address_range[i+0];
    xtsc_address end     = m_secure_address_range[i+1];
    XTSC_DEBUG(m_text, "start_address8 : " << std::hex << start_address8 << ", end_address8 : " << end_address8 
                       << ", begin : " << begin << ", end : " << end << ", axi : " << axi << endl);
    if (((start_address8 <= begin) && (end_address8 >= begin)) || 
        ((start_address8 >= begin) && (end_address8 <= end  )) || 
        ((start_address8 <= end  ) && (end_address8 >= end  ))   ){
      // NSM Access
      u32 n = xtsc_address_nibbles();
      XTSC_WARN(m_text, "Attempted non secure access: addr=0x" << hex << setfill('0') << setw(n) << address8 <<
                                  " secure=[0x" << setw(n) << begin << ",0x" << setw(n) << end << "]");
      return true;
    }
  }
  return false;
}

void
xtsc_component::xtsc_axi2pif_transactor::check_rd_nsm_rsp_id(xtsc::u32 port_num)
{
  if (!m_non_secure_mode) {
    return;
  }

  m_rd_nsm_vld[port_num] = false;

  if (m_axi_rd_response_order_fifo[port_num]->empty()) {
    return;
  }

  xtsc::u64 tag = m_axi_rd_response_order_fifo[port_num]->front();

  if (m_tag_2_axi_trans_info_map.find(tag) == m_tag_2_axi_trans_info_map.end()) {
    return;
  }

  axi_trans_info *i_axi_trans_info = m_tag_2_axi_trans_info_map[tag].front();
  assert(i_axi_trans_info != nullptr);

  m_rd_nsm_vld[port_num] = i_axi_trans_info->m_is_nsm_error;

  XTSC_DEBUG(m_text, "m_rd_nsm_vld[" << port_num << "]: " << m_rd_nsm_vld[port_num]);
}

void
xtsc_component::xtsc_axi2pif_transactor::send_rd_nsm_rsps(xtsc::u32 port_num)
{
  xtsc::u64 tag = m_axi_rd_response_order_fifo[port_num]->front();
  axi_trans_info *i_axi_trans_info = m_tag_2_axi_trans_info_map[tag].front();
  assert(i_axi_trans_info->m_is_nsm_error);

  //send error response
  xtsc::u32 num_trfs = i_axi_trans_info->m_axi_request->get_num_transfers();
  xtsc_response *response = new xtsc_response(*i_axi_trans_info->m_axi_request, xtsc_response::RSP_DATA_ERROR, false);
  for (u32 trf_num=1; trf_num <= num_trfs; ++trf_num) {
    response->set_transfer_number(trf_num);

    bool last_trf = (trf_num == num_trfs);
    response->set_last_transfer(last_trf);

    send_read_response(*response, port_num);

    wait(m_clock_period);
  }

  delete response;
  delete_trans_info(i_axi_trans_info, port_num);
}

void
xtsc_component::xtsc_axi2pif_transactor::check_wr_nsm_rsp_id(xtsc::u32 port_num)
{
  if (!m_non_secure_mode) {
    return;
  }

  m_wr_nsm_vld[port_num] = false;

  if (m_axi_wr_response_order_fifo[port_num]->empty()) {
    return;
  }

  xtsc::u64 tag = m_axi_wr_response_order_fifo[port_num]->front();

  if (m_tag_2_axi_trans_info_map.find(tag) == m_tag_2_axi_trans_info_map.end()) {
    return;
  }

  axi_trans_info *i_axi_trans_info = m_tag_2_axi_trans_info_map[tag].front();
  assert(i_axi_trans_info != nullptr);

  m_wr_nsm_vld[port_num] = i_axi_trans_info->m_is_nsm_error;

  XTSC_DEBUG(m_text, "m_wr_nsm_vld[" << port_num << "]: " << m_wr_nsm_vld[port_num]);
}

void
xtsc_component::xtsc_axi2pif_transactor::send_wr_nsm_rsps(xtsc::u32 port_num)
{
  xtsc::u64 tag = m_axi_wr_response_order_fifo[port_num]->front();
  axi_trans_info *i_axi_trans_info = m_tag_2_axi_trans_info_map[tag].front();
  assert(i_axi_trans_info->m_is_nsm_error);

  //send error response
  xtsc::u32 num_trfs = i_axi_trans_info->m_axi_request->get_num_transfers();
  xtsc_response *response = new xtsc_response(*i_axi_trans_info->m_axi_request, xtsc_response::RSP_DATA_ERROR, true);
    
  send_write_response(*response, port_num);

  wait(m_clock_period);

  delete_trans_info(i_axi_trans_info, port_num);
}

void
xtsc_component::xtsc_axi2pif_transactor::delete_axi_rsp(axi_rsp_trans_info *i_axi_rsp_trans_info, xtsc::u32 port_num)
{
  XTSC_TRACE(m_text, "Enter xtsc_axi2pif_transactor::delete_axi_rsp(" << i_axi_rsp_trans_info << ")");

  assert(m_tag_2_axi_trans_info_map[i_axi_rsp_trans_info->m_axi_request_tag].size() > 0);
  axi_trans_info *i_axi_trans_info = m_tag_2_axi_trans_info_map[i_axi_rsp_trans_info->m_axi_request_tag].front();
  xtsc_response *i_response = i_axi_rsp_trans_info->m_axi_response;

  bool last_rsp = i_axi_trans_info->m_is_read
    ? (i_axi_trans_info->m_num_axi_resp_sent == i_axi_trans_info->m_axi_request->get_num_transfers())
    : true;

  if (last_rsp) {
    delete_trans_info(i_axi_trans_info, port_num);
  }

  delete i_response;
  XTSC_DEBUG(m_text, "Deleting axi_rsp_trans_info: " << i_axi_rsp_trans_info << ".");
  delete i_axi_rsp_trans_info;
}

void
xtsc_component::xtsc_axi2pif_transactor::delete_trans_info(axi_trans_info *p_axi_trans_info, xtsc::u32 port_num)
{
  XTSC_TRACE(m_text, "Enter xtsc_axi2pif_transactor::delete_trans_info(" << p_axi_trans_info << ")");
  assert(p_axi_trans_info->m_num_pif_reqs_vec.empty());
  assert(p_axi_trans_info->m_pif_requests_tag_vec.empty());
  assert(p_axi_trans_info->m_pif_requests_vec.empty());

  xtsc_request *p_axi_request = p_axi_trans_info->m_axi_request;
  assert(p_axi_request != nullptr);

  XTSC_DEBUG(m_text, "Deleting axi_trans_info: " << p_axi_trans_info << " with tag=" << p_axi_request->get_tag() << ".");
  assert(m_tag_2_axi_trans_info_map.find(p_axi_request->get_tag()) != m_tag_2_axi_trans_info_map.end());

  m_tag_2_axi_trans_info_map[p_axi_request->get_tag()].pop();
  if (m_tag_2_axi_trans_info_map[p_axi_request->get_tag()].size() == 0) {
    m_tag_2_axi_trans_info_map.erase(p_axi_request->get_tag());
  }

  if (p_axi_trans_info->m_is_read) {
    assert(!m_axi_rd_response_order_fifo[port_num]->empty());
    m_axi_rd_response_order_fifo[port_num]->pop();
  } else {
    assert(!m_axi_wr_response_order_fifo[port_num]->empty());
    m_axi_wr_response_order_fifo[port_num]->pop();
  }

  delete_request(p_axi_request);
  delete p_axi_trans_info;
}

/******** xtsc_request_if_impl **********/
// First function for any request
// Checks for multiple errors and if all okay, copies request
// and calls nb_request_worker

void xtsc_component::xtsc_axi2pif_transactor::xtsc_request_if_impl::nb_request(const xtsc_request& request) 
{
  XTSC_TRACE(m_transactor.m_text, "Enter ::nb_request(request:" << request << ") Port #"
                                  << m_port_num << ", Type: " << m_type);

  XTSC_INFO(m_transactor.m_text, request << " Port #" << m_port_num );

  bool is_rd   = (m_type == "READ") ? true : false;
  bool is_read = m_transactor.is_read_access(&request);

  //Check if input is in axi protocol
  if (!request.is_axi_protocol()) {
    ostringstream oss;
    oss << kind() << " '" << name() << "' Incorrect Burst type: " << request.get_burst_type() << ". Should be set to AXI";
    throw xtsc_exception(oss.str());
  }

  //Check if correct type request if received i.e. read request for read_request_if
  if ((is_read && !is_rd) || (!is_read && is_rd)) {
    ostringstream oss;
    oss << kind() << " '" << name() << "' Received " << request.get_type_name() << " tag=" << request.get_tag() <<
        " is_read=" << m_transactor.is_read_access(&request) << " but request_if_type=" << m_type;
    throw xtsc_exception(oss.str());
  }

  //Check if byte_size set on request is higher than axi_width
  //This check is important as port of varied widths can be connected together
  if (request.get_byte_size() > m_transactor.m_axi_width8) {
      ostringstream oss;
      oss << kind() << " '" << name() << "' Received " << request.get_type_name() << " tag=" << request.get_tag() <<  
          "with byte size of " << request.get_byte_size() << " (can't be more than \"axi_byte_width\" of  " << 
          m_transactor.m_axi_width8 << ")";
      throw xtsc_exception(oss.str());
  }

  //Check if fifo is full. If yes, return nacc
  if ((is_rd && (m_transactor.m_read_request_fifo[m_port_num]->size() == m_transactor.m_read_request_fifo_depth)) || 
      (!is_rd && (m_transactor.m_write_request_fifo[m_port_num]->size() == m_transactor.m_write_request_fifo_depth)) 
     ) {
    xtsc_response response(request, xtsc_response::RSP_NACC);
    XTSC_INFO(m_transactor.m_text, response << " Port #" << m_port_num << " (" << m_type << " Request fifo full)" );
    if (is_rd) {
      (*m_transactor.m_read_respond_ports[m_port_num])->nb_respond(response);
    } else {
      (*m_transactor.m_write_respond_ports[m_port_num])->nb_respond(response);
    }
    return;
  }

  if (request.get_exclusive()) {
    XTSC_WARN(m_transactor.m_text, "Received exclusive request. Treating as normal read request.");
    assert(is_rd);
  }

  // Its important to copy as initiator might delete the request when we return.
  // But we need the request info later as well.
  xtsc_request *p_axi_request = m_transactor.copy_request(request);
  XTSC_DEBUG(m_transactor.m_text, "Create a copy: " << p_axi_request << ": " << *p_axi_request << " of input AXI request.");

  assert(p_axi_request->get_transfer_number() >= 1);
  nb_request_worker(p_axi_request);
  return;
}

// Create a new axi_trans_info.
// For read, convert axi request into corresponding pif request(s).
// For write, each write request for a write transaction is converted into pif request(s)
// and pushed into the same m_pif_requests_vec
// Push into fifos and notify the request thread
void xtsc_component::xtsc_axi2pif_transactor::xtsc_request_if_impl::nb_request_worker(xtsc::xtsc_request* p_axi_request) 
{
  XTSC_TRACE(m_transactor.m_text, "Enter ::nb_request_worker(request:" << *p_axi_request << ") Port #" << m_port_num);

  axi_req_info *i_axi_req_info   = new axi_req_info();
  i_axi_req_info->m_axi_request  = p_axi_request;

  sc_time add_delay              = (m_type == "READ") 
    ? m_transactor.m_read_request_delay * m_transactor.m_clock_period
    : m_transactor.m_write_request_delay * m_transactor.m_clock_period;
  i_axi_req_info->m_sched_time = sc_time_stamp() + add_delay;

  XTSC_DEBUG(m_transactor.m_text, "Created new axi_req_info: " << i_axi_req_info << " with tag=" << p_axi_request->get_tag() << " and m_sched_time: " << i_axi_req_info->m_sched_time);

  if (m_type == "READ") {
    m_transactor.m_read_request_fifo[m_port_num]->push(i_axi_req_info);
  } else {
    m_transactor.m_write_request_fifo[m_port_num]->push(i_axi_req_info);
  }

  XTSC_DEBUG(m_transactor.m_text, "Pushed into m_" << m_type << "_request_fifo[" << m_port_num << "]" << 
      " rd free slots: " << m_transactor.m_read_request_fifo_depth - m_transactor.m_read_request_fifo[m_port_num]->size() << 
      " & wr free slots: " << m_transactor.m_write_request_fifo_depth - m_transactor.m_write_request_fifo[m_port_num]->size());
  
  m_transactor.m_request_thread_event[m_port_num]->notify(add_delay);

  XTSC_DEBUG(m_transactor.m_text, "Notified request thread with Port #" << m_port_num << ", delay: " << add_delay);

  // Queue to maintain order of requests
  if (m_type == "READ") {
    m_transactor.m_axi_request_order_fifo[m_port_num]->push(p_axi_request->get_tag());
    m_transactor.m_axi_rd_response_order_fifo[m_port_num]->push(p_axi_request->get_tag());

    XTSC_DEBUG(m_transactor.m_text, "Pushed tag=" << p_axi_request->get_tag() << " into m_axi_request_order_fifo: Port #" << m_port_num << ", Size: " << m_transactor.m_axi_request_order_fifo[m_port_num]->size());
    XTSC_DEBUG(m_transactor.m_text, "Pushed tag=" << p_axi_request->get_tag() << " into m_axi_rd_response_order_fifo: Port #" << m_port_num << ", Size: " << m_transactor.m_axi_rd_response_order_fifo[m_port_num]->size());
  } else {
    //What if tag of next wr is same as previous wr. So we need to mark it 0 when before current request is freed
    if (p_axi_request->get_transfer_number() == 1) {
      m_transactor.m_axi_request_order_fifo[m_port_num]->push(p_axi_request->get_tag());
      m_transactor.m_axi_wr_response_order_fifo[m_port_num]->push(p_axi_request->get_tag());

      XTSC_DEBUG(m_transactor.m_text, "Pushed tag=" << p_axi_request->get_tag() << " into m_axi_request_order_fifo: Port #" << m_port_num << ", Size: " << m_transactor.m_axi_request_order_fifo[m_port_num]->size());
      XTSC_DEBUG(m_transactor.m_text, "Pushed tag=" << p_axi_request->get_tag() << " into m_axi_wr_response_order_fifo: Port #" << m_port_num << ", Size: " << m_transactor.m_axi_wr_response_order_fifo[m_port_num]->size());
    }
  }

  return;  
}

void xtsc_component::xtsc_axi2pif_transactor::xtsc_request_if_impl::nb_peek(xtsc_address address8, u32 size8, u8 *buffer) {
  XTSC_TRACE(m_transactor.m_text, "Enter ::xtsc_request_if_impl::nb_peek(address8:0x" << std::hex << address8 << ", size8:" << 
                                  std::dec << size8 << ") Port #" << m_port_num);

  (*m_transactor.m_request_ports[m_port_num])->nb_peek(address8, size8, buffer);

  u32 buf_offset  = 0;
  ostringstream oss;
  oss << hex << setfill('0');
  for (u32 i = 0; i<size8; ++i) {
    oss << setw(2) << (u32) buffer[buf_offset] << " ";
    buf_offset += 1;
  }
  XTSC_INFO(m_transactor.m_text, "nb_peek: " << " [0x" << hex << address8 << "/" << dec << size8 << "] = " 
                                 << oss.str() << " Port #" << m_port_num);
} 


void xtsc_component::xtsc_axi2pif_transactor::xtsc_request_if_impl::nb_poke(xtsc_address address8, u32 size8, const u8 *buffer) {
  XTSC_TRACE(m_transactor.m_text, "Enter ::xtsc_request_if_impl::nb_poke(address8:0x" << std::hex << address8 << ", size8:" << 
      std::dec << size8 << ") Port #" << m_port_num);

  u32 buf_offset  = 0;
  ostringstream oss;
  oss << hex << setfill('0');
  for (u32 i = 0; i<size8; ++i) {
    oss << setw(2) << (u32) buffer[buf_offset] << " ";
    buf_offset += 1;
  }
  XTSC_INFO(m_transactor.m_text, "nb_poke: " << " [0x" << hex << address8 << "/" << dec << size8 << "] = " 
      << oss.str() << " Port #" << m_port_num);

  (*m_transactor.m_request_ports[m_port_num])->nb_poke(address8, size8, buffer);
} 


bool xtsc_component::xtsc_axi2pif_transactor::xtsc_request_if_impl::nb_fast_access(xtsc_fast_access_request &fast_request) {
  XTSC_TRACE(m_transactor.m_text, "Enter ::nb_fast_access() Port #" << m_port_num);
  XTSC_INFO(m_transactor.m_text, "nb_fast_access:  Port #" << m_port_num);
  return (*m_transactor.m_request_ports[m_port_num])->nb_fast_access(fast_request);
}
 

void xtsc_component::xtsc_axi2pif_transactor::xtsc_request_if_impl::register_port(sc_port_base& port, const char *if_typename) {
  XTSC_TRACE(m_transactor.m_text, "Enter ::xtsc_request_if_impl::register_port(port:" << port.name() << ") Port #" 
      << m_port_num);

  if (m_p_port) {
    ostringstream oss;
    oss << "Illegal multiple binding detected to xtsc_axi2pif_transactor::m_request_exports[" << m_port_num << "] of '"
        << m_transactor.name() << "': " << endl;
    oss << "  " << port.name() << endl;
    oss << "  " << m_p_port->name();
    throw xtsc_exception(oss.str());
  }
  XTSC_INFO(m_transactor.m_text, "Binding '" << port.name() << "' to xtsc_axi2pif_transactor::m_request_exports[" << 
      m_port_num << "]");
  
  m_p_port = &port;
}

/********** xtsc_respond_if_impl ***********/

bool xtsc_component::xtsc_axi2pif_transactor::xtsc_respond_if_impl::nb_respond(const xtsc_response& response) {

  XTSC_TRACE(m_transactor.m_text, "Enter xtsc_respond_if_impl::nb_respond(response:" << response << ") Port #" << m_port_num);

  const xtsc_response *p_pif_response    = &response;

  xtsc::u64 pif_tag                      = p_pif_response->get_tag();
  axi_trans_info *i_axi_trans_info       = m_transactor.m_tag_2_axi_trans_info_map[pif_tag].front();
  xtsc::u64 axi_tag                      = i_axi_trans_info->m_axi_request->get_tag();
  xtsc::u64 axi_id                       = i_axi_trans_info->m_axi_request->get_id();

  assert(i_axi_trans_info != nullptr);
  assert(i_axi_trans_info->m_num_pif_reqs_vec.size() > 0);
  assert(i_axi_trans_info->m_pif_requests_tag_vec.size() > 0);

  bool is_rd = m_transactor.is_read_access(i_axi_trans_info->m_axi_request);

  XTSC_INFO(m_transactor.m_text, response << " EXPORT #" << m_port_num << ", is_rd: " << is_rd);

  //If we get RSP_NACC, we need to set m_pif_request_got_nacc to true so that
  //nb_request knows to send the same request again.
  if (p_pif_response->get_status() == xtsc_response::RSP_NACC) {

    //If we get RSP_NACC, we must have m_pif_waiting_for_nacc set to true 
    //for that port
    if (m_transactor.m_pif_waiting_for_nacc[m_port_num]) {
      m_transactor.m_pif_request_got_nacc[m_port_num] = true;
      return true;
    } 
    else {
      //When we get RSP_NACC, but we are not waiting for any nacc
      //i.e. m_pif_waiting_for_nacc is false
      ostringstream oss;
      oss << m_transactor.kind() << " '" << m_transactor.name() << "' received nacc too late: " << *p_pif_response << endl;
      oss << " - Possibly something is wrong with the downstream device" << endl;
      oss << " - Possibly this device's \"clock_period\" needs to be adjusted";
      throw xtsc_exception(oss.str());
    }
  }

  //If m_response_fifo is full, then no space for another response, so return false
  if ((is_rd && m_transactor.m_read_response_fifo[m_port_num]->size() == m_transactor.m_read_response_fifo_depth)
      || (!is_rd && m_transactor.m_write_response_fifo[m_port_num]->size() == m_transactor.m_write_response_fifo_depth)) {
    XTSC_INFO(m_transactor.m_text, response << " EXPORT #" << m_port_num << " (Rejected: PIF " <<
        (is_rd ? "read" : "write") << " response fifo full)");
    return false;
  }

  //If zero, it means it is the first response for the axi_trans
  //Initializing response time signals
  if (i_axi_trans_info->m_num_pif_rsps_arrvd == 0) {
    i_axi_trans_info->m_curr_pif_tag                        = i_axi_trans_info->m_pif_requests_tag_vec.front();
    i_axi_trans_info->m_num_rsps_remaining_4_curr_pif_trans = i_axi_trans_info->m_num_pif_reqs_vec.front();
  }

  // In case of address error, we will get only one response. But we have to send all the required
  // responses at AXI interface.
  xtsc::u32 read_rsp_count = 1;
  if ((p_pif_response->get_status() == xtsc_response::RSP_ADDRESS_ERROR) ||
      (p_pif_response->get_status() == xtsc_response::RSP_ADDRESS_DATA_ERROR)) {
    read_rsp_count         = i_axi_trans_info->m_num_rsps_remaining_4_curr_pif_trans;
  }

  if (is_rd) {
    for (xtsc::u32 i=0; i < read_rsp_count; i++) {
      read_response_worker(p_pif_response);
    }
  } else {
    write_response_worker(p_pif_response);
  }

  return true; 
}

void xtsc_component::xtsc_axi2pif_transactor::xtsc_respond_if_impl::read_response_worker(const xtsc_response *p_pif_response)
{
  XTSC_TRACE(m_transactor.m_text, "Enter xtsc_respond_if_impl::read_response_worker( " << *p_pif_response << ").");

  xtsc::u64       pif_tag                  = p_pif_response->get_tag();
  axi_trans_info *i_axi_trans_info         = m_transactor.m_tag_2_axi_trans_info_map[pif_tag].front();

  xtsc::u32       size8                    = p_pif_response->get_byte_size();

  xtsc_request   *p_axi_request            = i_axi_trans_info->m_axi_request;
  assert(size8 == p_axi_request->get_byte_size());

  xtsc_response  *p_axi_response           = nullptr;

  i_axi_trans_info->m_num_pif_rsps_arrvd++;
  bool last = (i_axi_trans_info->m_total_pif_requests == i_axi_trans_info->m_num_pif_rsps_arrvd);

  p_axi_response = new xtsc_response(*p_axi_request, p_pif_response->get_status(), last);
  p_axi_response->set_transfer_number(i_axi_trans_info->m_num_pif_rsps_arrvd);
  p_axi_response->set_exclusive_ok(p_pif_response->get_exclusive_ok());
  memcpy(p_axi_response->get_buffer(), p_pif_response->get_buffer(), size8);

  XTSC_DEBUG(m_transactor.m_text, "Created new xtsc_response: " << *p_axi_response);

  // Now, we have p_axi_response ready to be sent back
  axi_rsp_trans_info *i_axi_rsp_trans_info = new axi_rsp_trans_info;
  i_axi_rsp_trans_info->m_axi_response     = p_axi_response;
  i_axi_rsp_trans_info->m_axi_request_tag  = p_axi_request->get_tag();

  sc_time delay                            = m_transactor.m_read_response_delay * m_transactor.m_clock_period;
  i_axi_rsp_trans_info->m_sched_time       = sc_time_stamp() + delay;
  m_transactor.m_read_response_thread_event[m_port_num]->notify(delay);

  m_transactor.m_read_response_fifo[m_port_num]->push(i_axi_rsp_trans_info);

  XTSC_DEBUG(m_transactor.m_text, "Created new axi_rsp_trans_info: " << i_axi_rsp_trans_info << " with tag=" << p_axi_response->get_tag());
  XTSC_DEBUG(m_transactor.m_text, "Notify m_read_response_thread_event[" << m_port_num << "] after a delay of: (" << m_transactor.m_read_response_delay << "*" << m_transactor.m_clock_period << ")");

  assert(i_axi_trans_info->m_num_rsps_remaining_4_curr_pif_trans > 0);
  i_axi_trans_info->m_num_rsps_remaining_4_curr_pif_trans--;

  if (i_axi_trans_info->m_num_rsps_remaining_4_curr_pif_trans == 0) {
    if (pif_tag != p_axi_request->get_tag()) { 
      assert(pif_tag == i_axi_trans_info->m_pif_requests_tag_vec.front());
      assert(m_transactor.m_tag_2_axi_trans_info_map.find(pif_tag) != m_transactor.m_tag_2_axi_trans_info_map.end());
      
      XTSC_DEBUG(m_transactor.m_text, "Deleting m_tag_2_axi_trans_info_map tag=" << pif_tag << ".");
      m_transactor.m_tag_2_axi_trans_info_map[pif_tag].pop();
      m_transactor.m_tag_2_axi_trans_info_map.erase(pif_tag);
    }

    if (m_transactor.m_outstanding_pif_request_ids > 0) {
      xtsc::u8 rsp_id = p_pif_response->get_id();
      assert(rsp_id >= 32);
      rsp_id -= 32;

      XTSC_DEBUG(m_transactor.m_text, "Clearing m_read_request_id_pending[" << m_port_num << "][" << xtsc::u32(rsp_id) << "].");
      assert(m_transactor.m_read_request_id_pending[m_port_num][rsp_id] == true); 

      m_transactor.m_read_request_id_pending[m_port_num][rsp_id] = false;
      m_transactor.m_num_read_pif_requests_pending[m_port_num]--;
    }

    i_axi_trans_info->m_num_pif_reqs_vec.pop();
    i_axi_trans_info->m_pif_requests_tag_vec.pop();

    i_axi_trans_info->m_num_rsps_remaining_4_curr_pif_trans = (!i_axi_trans_info->m_num_pif_reqs_vec.empty())
      ? i_axi_trans_info->m_num_pif_reqs_vec.front()
      : 0;

    i_axi_trans_info->m_curr_pif_tag = (!i_axi_trans_info->m_pif_requests_tag_vec.empty())
      ? i_axi_trans_info->m_pif_requests_tag_vec.front()
      : 0;
  }

  XTSC_DEBUG(m_transactor.m_text, "Num pif responses arrived: " << i_axi_trans_info->m_num_pif_rsps_arrvd << ", Num PIF responses remaining for current trans: " << i_axi_trans_info->m_num_rsps_remaining_4_curr_pif_trans);

  if (i_axi_trans_info->m_num_pif_rsps_arrvd == i_axi_trans_info->m_total_pif_requests) {
    assert(i_axi_trans_info->m_num_pif_reqs_vec.empty());
    assert(i_axi_trans_info->m_pif_requests_tag_vec.empty());

    assert(i_axi_trans_info->m_num_rsps_remaining_4_curr_pif_trans == 0);
    assert(i_axi_trans_info->m_curr_pif_tag == 0);
  } else {
    assert(i_axi_trans_info->m_num_rsps_remaining_4_curr_pif_trans != 0);
    assert(i_axi_trans_info->m_curr_pif_tag != 0);
  }
}

void xtsc_component::xtsc_axi2pif_transactor::xtsc_respond_if_impl::write_response_worker(const xtsc_response *p_pif_response)
{
  XTSC_TRACE(m_transactor.m_text, "Enter xtsc_respond_if_impl::write_response_worker( " << *p_pif_response << ").");

  xtsc::u64       pif_tag          = p_pif_response->get_tag();
  
  axi_trans_info *i_axi_trans_info = m_transactor.m_tag_2_axi_trans_info_map[pif_tag].front();
  assert(i_axi_trans_info != nullptr);

  xtsc_request   *p_axi_request    = i_axi_trans_info->m_axi_request;

  i_axi_trans_info->m_num_pif_rsps_arrvd++;

  xtsc::xtsc_response::status_t curr_status = p_pif_response->get_status();
  if ((curr_status != xtsc::xtsc_response::RSP_OK) && (i_axi_trans_info->m_curr_response_status != xtsc::xtsc_response::DECERR)) {
    if(curr_status == xtsc::xtsc_response::RSP_ADDRESS_ERROR || curr_status == xtsc::xtsc_response::RSP_ADDRESS_DATA_ERROR) {
      i_axi_trans_info->m_curr_response_status = xtsc::xtsc_response::DECERR;
    } else if (curr_status == xtsc::xtsc_response::RSP_DATA_ERROR) {
      i_axi_trans_info->m_curr_response_status = xtsc::xtsc_response::SLVERR;
    } else {
      assert(0);
    }
  }

  i_axi_trans_info->m_num_rsps_remaining_4_curr_pif_trans = 0;

  if (i_axi_trans_info->m_num_rsps_remaining_4_curr_pif_trans == 0) {
    if (m_transactor.m_outstanding_pif_request_ids > 0) {
      xtsc::u8 rsp_id = p_pif_response->get_id();

      XTSC_DEBUG(m_transactor.m_text, "Clearing m_write_request_id_pending[" << m_port_num << "][" << xtsc::u32(rsp_id) << "].");
      assert(m_transactor.m_write_request_id_pending[m_port_num][rsp_id] == true); 

      m_transactor.m_write_request_id_pending[m_port_num][rsp_id] = false;
      m_transactor.m_num_write_pif_requests_pending[m_port_num]--;
    }

    assert(!i_axi_trans_info->m_num_pif_reqs_vec.empty());
    i_axi_trans_info->m_num_pif_reqs_vec.pop();
    assert(!i_axi_trans_info->m_pif_requests_tag_vec.empty());
    i_axi_trans_info->m_pif_requests_tag_vec.pop();

    i_axi_trans_info->m_num_rsps_remaining_4_curr_pif_trans = (!i_axi_trans_info->m_num_pif_reqs_vec.empty())
      ? i_axi_trans_info->m_num_pif_reqs_vec.front()
      : 0;

    i_axi_trans_info->m_curr_pif_tag = (!i_axi_trans_info->m_pif_requests_tag_vec.empty())
      ? i_axi_trans_info->m_pif_requests_tag_vec.front()
      : 0;
  }

  if (pif_tag != i_axi_trans_info->m_axi_request->get_tag()) {
    XTSC_DEBUG(m_transactor.m_text, "Deleting m_tag_2_axi_trans_info_map tag=" << pif_tag << ".");
    m_transactor.m_tag_2_axi_trans_info_map[pif_tag].pop();
    m_transactor.m_tag_2_axi_trans_info_map.erase(pif_tag);
  }

  XTSC_DEBUG(m_transactor.m_text, "Num pif responses arrived: " << i_axi_trans_info->m_num_pif_rsps_arrvd << ", Num PIF responses remaining for current trans: " << i_axi_trans_info->m_num_rsps_remaining_4_curr_pif_trans);

  if ((i_axi_trans_info->m_all_pif_req_sent == true) && (i_axi_trans_info->m_num_pif_rsps_arrvd == i_axi_trans_info->m_num_pif_trans)) {
    assert(i_axi_trans_info->m_num_pif_reqs_vec.empty());
    assert(i_axi_trans_info->m_pif_requests_tag_vec.empty());

    assert(i_axi_trans_info->m_num_rsps_remaining_4_curr_pif_trans == 0);
    assert(i_axi_trans_info->m_curr_pif_tag == 0);

    // send the axi response back
    bool last = true;
    xtsc_response *p_axi_response = new xtsc_response(*p_axi_request,i_axi_trans_info->m_curr_response_status, last);
    p_axi_response->set_exclusive_ok(p_pif_response->get_exclusive_ok());

    XTSC_DEBUG(m_transactor.m_text, "Created new xtsc_response: " << *p_axi_response);
    
    axi_rsp_trans_info *i_axi_rsp_trans_info = new axi_rsp_trans_info;
    i_axi_rsp_trans_info->m_axi_response     = p_axi_response;
    i_axi_rsp_trans_info->m_axi_request_tag  = p_axi_request->get_tag();

    sc_time delay                            = m_transactor.m_write_response_delay * m_transactor.m_clock_period;
    i_axi_rsp_trans_info->m_sched_time       = sc_time_stamp() + delay;
    m_transactor.m_write_response_thread_event[m_port_num]->notify(delay);

    m_transactor.m_write_response_fifo[m_port_num]->push(i_axi_rsp_trans_info);

    XTSC_DEBUG(m_transactor.m_text, "Created new axi_rsp_trans_info: " << i_axi_rsp_trans_info << " with tag=" << p_axi_response->get_tag());
    XTSC_DEBUG(m_transactor.m_text, "Notify m_write_response_thread_event[" << m_port_num << "] after a delay of: (" << m_transactor.m_write_response_delay << "*" << m_transactor.m_clock_period << ")");
  } else {
    // No checks possible with reference to below case:
    // A transaction with multiple requests arrives. Each request is split into individual write transactions.
    // Right now, fifo depth is set to 1.
    // After first is sent forward, second arrives but is yet not converted to pif transaction. So m_pif_requests_vec is empty
    // At this time, if response arrives for first request, we will have m_num_rsps_remaining_4_curr_pif_trans=0 and m_curr_pif_tag=0 
    // even though write axi transaction requests are not yet completed
  }
}

void xtsc_component::xtsc_axi2pif_transactor::xtsc_respond_if_impl::register_port(sc_port_base& port, const char *if_typename) 
{
  XTSC_TRACE(m_transactor.m_text, "Enter ::xtsc_respond_if_impl::register_port(port:" << port.name() << ") Port #" 
      << m_port_num);

  if (m_p_port) {
    ostringstream oss;
    oss << "Illegal multiple binding detected to xtsc_axi2pif_transactor '" << m_transactor.name() << "' " << name() << ": " << endl;
    oss << "  " << port.name() << endl;
    oss << "  " << m_p_port->name();
    throw xtsc_exception(oss.str());
  }
  XTSC_INFO(m_transactor.m_text, "Binding '" << port.name() << "' to xtsc_axi2pif_transactor::" << name() << " Port #" << m_port_num);
  m_p_port = &port;
}
