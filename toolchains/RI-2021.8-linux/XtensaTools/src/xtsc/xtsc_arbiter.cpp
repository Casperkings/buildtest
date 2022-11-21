// Copyright (c) 2005-2018 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Cadence Design Systems, Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Cadence Design Systems, Inc.

// \w*clock_period\w*\|arbitration_phase\|m_arbitration_phase\|m_arbitration_phase_plus_one\|m_request_delay\|m_response_delay\|m_response_repeat\|m_recovery_time\|nacc_wait_time\|m_nacc_wait_time\|\<wait\>

// Not ignored if apb: request_delay\|response_delay\|delay_from_receipt\|recovery_time
// Ignored if apb:     one_at_a_time\|m_route_id\|route_id_lsb\|check_route_id_bits\|num_route_ids\|nacc_wait_time\|response_repeat

#include <xtsc/xtsc_arbiter.h>
#include <xtsc/xtsc_core.h>
#include <xtsc/xtsc_dma_engine.h>
#include <xtsc/xtsc_master.h>
#include <xtsc/xtsc_memory_trace.h>
#include <xtsc/xtsc_pin2tlm_memory_transactor.h>
#include <xtsc/xtsc_router.h>
#include <xtsc/xtsc_logging.h>
#include <algorithm>
#include <cassert>



/*
 *  Theory of Operation
 *  
 *  Incoming requests are received in the nb_request() method.  Typically nb_request()
 *  hands the request over to the do_request() method and there a copy is made of the
 *  request which is then added to the appropriate FIFO (based on port) in the
 *  m_request_deques array and m_arbiter_thread_event is notified.  Which thread
 *  is used to wait for this event depends on how xtsc_arbiter was configured:
 *     arbiter_pwc_thread         "master_byte_widths" specified
 *     arbiter_special_thread     "xfer_en_port" or "external_cbox" specified
 *     arbiter_apb_thread         "apb" specified
 *     arbiter_thread             None of the above
 *
 *  Incoming responses are received in the nb_respond() method.  Typically a copy is
 *  made of the response which is then added to m_response_fifo and the
 *  m_response_thread_event is notified.  When not operating as a PWC, this event is
 *  handled by response_thread and when operating as a PWC it is handled by
 *  response_pwc_thread.
 *
 *  When "immediate_timing" is true, the threads are not used ("immediate_timing" must
 *  be false when any of "apb", "master_byte_widths", "xfer_en_port", "external_cbox",
 *  or "arbitration_policy" is specified.
 *
 *  The arbiter can be locked to a port so that requests to other ports are rejected.
 *  When "dram_lock" and "apb" are false (the default) then the arbiter assumes it is
 *  being used as part of PIF/AXI interconnect and locking is based on the value returned by
 *  the xtsc_request::get_last_transfer() method.  When "dram_lock" is true then the arbiter
 *  assumes it is being used as part of a local memory DataRAM interconnect and locking
 *  is controlled by the nb_lock() call.  When "apb" is true, locking is not needed.
 *  
 *
 *  
 *  PWC Operation ("master_byte_widths" specified)
 *
 *  If a request is received on an upstream PIF which has the same width as the
 *  downstream PIF, then the request is passed downstream unchanged.  Otherwise, the
 *  convert_request() method is called to perform all required conversions, including
 *  combining multiple requests into a single request, converting a single request into
 *  multiple requests, and changing the request type (i.e from a block request to a
 *  non-block request or vice-versa) and various other fields of the request.
 *
 *  If a response is received from the downstream PIF which is targeted to an upstream
 *  PIF of the same width, then the response is passed upstream unchanged.  Otherwise,
 *  the convert_response() method is called to perform all required conversions,
 *  including combining multiple responses into a single response and converting a
 *  single response into multiple responses.
 *
 *  When a response comes in on the downstream PIF interface that is targeted to a
 *  upstream PIF of a different width, then the response has to be match up to the
 *  original request so that the necessary conversions can be made to the response.  The
 *  arbiter uses the request and response ID fields to enable this match-up and so has
 *  to reassign the request ID in requests sent downstream to ensure uniqueness (of
 *  course, the original request ID has to be used for the responses sent back
 *  upstream).
 *
 *  The main structures associated with PWC operation are:
 *
 *  1) class req_rsp_info: This class holds the state information necessary for the
 *     entire sequence from the receipt of the request (or the first transfer of a
 *     BLOCK_WRITE or RCW request) until the response (or last response if BLOCK_READ)
 *     is sent back upstream.  
 *
 *  2) m_req_rsp_table[]: This map holds all outstanding req_rsp_info objects.  It is
 *     indexed by the downstream request tag.
 *
 *  3) m_requests[]: This array holds all ready-to-send-downstream requests generated by
 *     the call from arbiter_pwc_thread() to convert_request().  The array is empty when
 *     convert_request() is called and may or may not be empty when convert_request()
 *     returns.  If it is not empty, the arbiter_pwc_thread() sends each of the requests
 *     in it downstream before clearing it and calling convert_request() again for the
 *     next incoming request.  If the array is empty, then m_p_nascent_request in
 *     req_rsp_info will point to the partially formed converted request.
 *
 *  4) m_responses[]: This array holds all ready-to-send-upstream responses generated by
 *     the call from response_pwc_thread() to convert_response().  The array is
 *     empty when convert_response() is called and may or may not be empty when
 *     convert_response() returns.  If it is not empty, the response_pwc_thread()
 *     sends each of the responses in it upstream before clearing it and calling
 *     convert_response() again for the next incoming response.  If the array is empty,
 *     then m_p_nascent_response in req_rsp_info will point to the partially formed
 *     converted response.
 *
 */



using namespace std;
#if SYSTEMC_VERSION >= 20050601
using namespace sc_core;
#endif
using namespace xtsc;
using log4xtensa::INFO_LOG_LEVEL;
using log4xtensa::VERBOSE_LOG_LEVEL;





namespace xtsc_component {

static bool start_address_less_then(const xtsc_address_range_entry* range1, const xtsc_address_range_entry* range2) {
  return (range1->m_start_address8 < range2->m_start_address8);
}

}


xtsc_component::xtsc_arbiter::xtsc_arbiter(sc_module_name module_name, const xtsc_arbiter_parms& arbiter_parms) :
  sc_module             (module_name),
  xtsc_module           (*(sc_module*)this),
  m_request_port        ("m_request_port"),
  m_respond_export      ("m_respond_export"),
  m_respond_impl        ("m_respond_impl", *this),
  m_response_fifo       ("m_response_fifo", arbiter_parms.get_non_zero_u32("response_fifo_depth")),
  m_p_apb_request_info  (NULL),
  m_arbiter_parms       (arbiter_parms),
  m_apb                 (arbiter_parms.get_bool("apb")),
  m_use_block_requests  (arbiter_parms.get_bool("use_block_requests")),
  m_slave_byte_width    (0),
  m_route_id_bits_mask  (0xFFFFFFFF),
  m_route_id_bits_clear (0xFFFFFFFF),
  m_route_id_bits_shift (arbiter_parms.get_u32("route_id_lsb")),
  m_num_route_ids       (arbiter_parms.get_u32("num_route_ids")),
  m_route_ids_used      (0),
  m_next_route_id       (0),
  m_routing_table       (0),
  m_downstream_route_id (0),
  m_read_only           (arbiter_parms.get_bool("read_only")),
  m_write_only          (arbiter_parms.get_bool("write_only")),
  m_log_peek_poke       (arbiter_parms.get_bool("log_peek_poke")),
  m_time_resolution     (sc_get_time_resolution()),
  m_ports_with_requests (0),
  m_use_lock_port_groups(false),
#if IEEE_1666_SYSTEMC >= 201101L
  m_arbiter_thread_event            ("m_arbiter_thread_event"),
  m_response_thread_event           ("m_response_thread_event"),
  m_apb_response_done_event         ("m_apb_response_done_event"),
  m_align_request_phase_thread_event("m_align_request_phase_thread_event"),
#endif
  m_profile_buffers     (arbiter_parms.get_bool("profile_buffers")),
  m_text                (log4xtensa::TextLogger::getInstance(name())),
  m_binary              (log4xtensa::BinaryLogger::getInstance(name()))
{

  m_num_masters                 = arbiter_parms.get_non_zero_u32("num_masters");
  m_one_at_a_time               = arbiter_parms.get_bool("one_at_a_time");
  m_align_request_phase         = arbiter_parms.get_bool("align_request_phase");
  m_check_route_id_bits         = arbiter_parms.get_bool("check_route_id_bits");

  m_log_data_binary             = true;
  m_next_slot                   = 0;

  m_delay_from_receipt          = arbiter_parms.get_bool("delay_from_receipt");
  m_dram_lock                   = arbiter_parms.get_bool("dram_lock");
  m_external_cbox               = arbiter_parms.get_bool("external_cbox");
  m_xfer_en_port                = arbiter_parms.get_u32 ("xfer_en_port");
  m_immediate_timing            = arbiter_parms.get_bool("immediate_timing");
  if (m_immediate_timing) {
    m_one_at_a_time             = false;
    m_align_request_phase       = false;
  }

  m_fail_port_mask              = arbiter_parms.get_u32 ("fail_port_mask");
  m_fail_percentage             = arbiter_parms.get_u32 ("fail_percentage");
  compute_let_through();
  m_fail_seed                   = arbiter_parms.get_u32 ("fail_seed");
  m_z = m_fail_seed;
  m_w = m_fail_seed;

  if (m_align_request_phase) {
    m_phase_delay_fifo = new sc_fifo<request_info*>("m_phase_delay_fifo", 2);
  }

  if (m_profile_buffers) {
    m_max_num_requests           = new u32[m_num_masters];
    m_max_num_requests_tag       = new u64[m_num_masters];
    for (u32 i=0; i<m_num_masters; i++) {
      m_max_num_requests[i]      = 0;
      m_max_num_requests_tag[i]  = 0;
    }
    m_max_num_requests_timestamp = new sc_time[m_num_masters];
    m_max_num_responses          = 0;
    m_max_num_responses_tag      = 0;
  }
  
  // Get clock period 
  u32 clock_period = arbiter_parms.get_u32("clock_period");
  if (clock_period == 0xFFFFFFFF) {
    m_clock_period = xtsc_get_system_clock_period();
  }
  else {
    m_clock_period = m_time_resolution * clock_period;
  }

  u32 posedge_offset = m_arbiter_parms.get_u32("posedge_offset");
  if (posedge_offset == 0xFFFFFFFF) {
    m_posedge_offset = xtsc_get_system_clock_posedge_offset();
  }
  else {
    m_posedge_offset = posedge_offset * m_time_resolution;
  }
  if (m_posedge_offset >= m_clock_period) {
    ostringstream oss;
    oss << kind() << " '" << name() << "' parameter error:" << endl;
    oss << "\"posedge_offset\" (0x" << hex << posedge_offset << "=>" << m_posedge_offset
        << ") must be strictly less than \"clock_period\" (0x" << clock_period << "=>" << m_clock_period << ")";
    throw xtsc_exception(oss.str());
  }
  m_posedge_offset_value = m_posedge_offset.value();
  m_has_posedge_offset   = (m_posedge_offset != SC_ZERO_TIME);

  // Get arbitration phase
  u32 arbitration_phase = arbiter_parms.get_u32("arbitration_phase");
  if (arbitration_phase == 0xFFFFFFFF) {
    m_arbitration_phase = 0.5 * m_clock_period;
  }
  else {
    m_arbitration_phase = m_time_resolution * arbitration_phase;
  }

  u32 nacc_wait_time    = arbiter_parms.get_u32("nacc_wait_time");

  compute_delays();

  u32 mask = 0x80000000;
  bool found_one = false;
  u32 num_bits = 0;     // = ceil(log2(m_num_masters))
  for (u32 i=32; i>0; i--, mask >>= 1) {
    if (m_num_masters & mask) {
      if (found_one) {
        num_bits += 1;
        break;
      }
      num_bits = i-1;
      found_one = true;
    }
  }
  if (m_apb) { /* do nothing */ }
  else if (m_route_id_bits_shift == 0xFFFFFFFF) {
    m_routing_table       = new u32[m_num_route_ids][2];
    m_downstream_route_id = new u32[m_num_masters];
  }
  else if (m_route_id_bits_shift + num_bits <= 32) {
    m_route_id_bits_mask = ((1 << num_bits) - 1) << m_route_id_bits_shift;
    m_route_id_bits_clear = ~m_route_id_bits_mask;
    XTSC_LOG(m_text, xtsc_get_constructor_log_level(), "Using route_id mask of 0x" << hex << m_route_id_bits_mask);
  }
  else {
    ostringstream oss;
    oss << "xtsc_arbiter '" << name() << "': \"route_id_lsb\"=" << m_route_id_bits_shift
        << " does not leave enough bits for \"num_masters\"=" << m_num_masters << " (which requires " << num_bits << " bits)";
    throw xtsc_exception(oss.str());
  }

  m_master_byte_widths = arbiter_parms.get_u32_vector("master_byte_widths");
  m_is_pwc = (m_master_byte_widths.size() != 0);
  if (m_is_pwc) {
    if (m_immediate_timing) {
      ostringstream oss;
      oss << "xtsc_arbiter '" << name() << "': \"immediate_timing\" must be false if \"master_byte_widths\" is set";
      throw xtsc_exception(oss.str());
    }
    if (m_master_byte_widths.size() != m_num_masters) {
      ostringstream oss;
      oss << "xtsc_arbiter '" << name() << "': size of \"master_byte_widths\" (" << m_master_byte_widths.size()
          << ") != \"num_masters\" (" << m_num_masters << ")";
      throw xtsc_exception(oss.str());
    }
    for (u32 i=0; i<m_num_masters; ++i) {
      if ((m_master_byte_widths[i] != 4) && (m_master_byte_widths[i] != 8) && (m_master_byte_widths[i] != 16) && (m_master_byte_widths[i] != 32)) {
        ostringstream oss;
        oss << "xtsc_arbiter '" << name() << "': master_byte_widths[" << i << "]=" << m_master_byte_widths[i]
            << " which is not one of the valid values of 4|8|16|32.";
        throw xtsc_exception(oss.str());
      }
    }
    m_slave_byte_width = arbiter_parms.get_u32("slave_byte_width");
    if ((m_slave_byte_width != 4) && (m_slave_byte_width != 8) && (m_slave_byte_width != 16) && (m_slave_byte_width != 32)) {
      ostringstream oss;
      oss << "xtsc_arbiter '" << name() << "': slave_byte_width=" << m_slave_byte_width 
          << " which is not one of the valid non-default values of 4|8|16|32.";
      throw xtsc_exception(oss.str());
    }
  }

  m_request_exports = new sc_export<xtsc_request_if>*[m_num_masters];
  m_request_impl    = new xtsc_request_if_impl*      [m_num_masters];
  for (u32 i=0; i<m_num_masters; i++) {
    ostringstream oss1;
    oss1 << "m_request_exports[" << i << "]";
    m_request_exports[i] = new sc_export<xtsc_request_if>(oss1.str().c_str());
    m_port_types[oss1.str()] = REQUEST_EXPORT;
    ostringstream oss2;
    oss2 << "m_request_impl[" << i << "]";
    m_request_impl   [i] = new xtsc_request_if_impl(oss2.str().c_str(), *this, i);
    (*m_request_exports[i])(*m_request_impl[i]);
  }

  vector<u32> request_fifo_depths = arbiter_parms.get_u32_vector("request_fifo_depths");
  if ((request_fifo_depths.size() != 0) && (request_fifo_depths.size() != m_num_masters)) {
    ostringstream oss;
    oss << "xtsc_arbiter '" << name() << "': size of \"request_fifo_depths\" (" << request_fifo_depths.size()
        << ") != \"num_masters\" (" << m_num_masters << ")";
    throw xtsc_exception(oss.str());
  }
  u32 request_fifo_depth = arbiter_parms.get_u32("request_fifo_depth");
  m_request_deques = new deque<request_info*>*[m_num_masters];
  m_request_fifos = new sc_fifo<int>*[m_num_masters];
  for (u32 i=0; i<m_num_masters; i++) {
    ostringstream oss;
    u32 depth = (request_fifo_depths.size() ? request_fifo_depths[i] : request_fifo_depth);
    if (depth == 0) {
      ostringstream oss;
      oss << "xtsc_arbiter '" << name() << "': depth from \"request_fifo_depths\" or \"request_fifo_depth\" cannot be 0";
      throw xtsc_exception(oss.str());
    }
    m_request_deques[i] = new deque<request_info*>();
    oss << "m_request_fifos[" << i << "]";
    m_request_fifos[i] = new sc_fifo<int>(oss.str().c_str(), depth);
  }

  m_respond_ports = new sc_port<xtsc_respond_if>*[m_num_masters];
  for (u32 i=0; i<m_num_masters; i++) {
    ostringstream oss;
    oss << "m_respond_ports[" << i << "]";
    m_respond_ports[i] = new sc_port<xtsc_respond_if>(oss.str().c_str());
    m_port_types[oss.str()] = RESPOND_PORT;
    oss.str(""); oss << "slave_port[" << i << "]";
    m_port_types[oss.str()] = PORT_TABLE;
  }

  if (m_num_masters == 1) {
  m_port_types["slave_port"]       = PORT_TABLE;
  }
  m_port_types["slave_ports"]      = PORT_TABLE;
  m_port_types["m_request_port"]   = REQUEST_PORT;
  m_port_types["m_respond_export"] = RESPOND_EXPORT;
  m_port_types["master_port"]      = PORT_TABLE;

  m_respond_export(m_respond_impl);

  if (m_dram_lock) {
    if (m_immediate_timing) {
      ostringstream oss;
      oss << kind() << " '" << name() << "' parameters \"dram_lock\" and \"immediate_timing\" cannot both be true";
      throw xtsc_exception(oss.str());
    }
    if (m_is_pwc) {
      ostringstream oss;
      oss << kind() << " '" << name() << "' parameters \"dram_lock\" cannot be true when acting as a PIF-width converter";
      throw xtsc_exception(oss.str());
    }
    for (u32 i=0; i < m_num_masters; ++i) {
      m_dram_locks.push_back(false);
    }
  }

  if (m_external_cbox) {
    if (m_immediate_timing) {
      ostringstream oss;
      oss << kind() << " '" << name() << "' parameters \"external_cbox\" and \"immediate_timing\" cannot both be true";
      throw xtsc_exception(oss.str());
    }
    if (m_is_pwc) {
      ostringstream oss;
      oss << kind() << " '" << name() << "' parameters \"external_cbox\" cannot be true when acting as a PIF-width converter";
      throw xtsc_exception(oss.str());
    }
    if (m_num_masters != 2) {
      ostringstream oss;
      oss << kind() << " '" << name() << "' parameters \"external_cbox\" cannot be true unless \"num_masters\"[="
          << m_num_masters << "] is 2.";
      throw xtsc_exception(oss.str());
    }
  }

  if (m_xfer_en_port != 0xFFFFFFFF) {
    if (m_immediate_timing) {
      ostringstream oss;
      oss << kind() << " '" << name() << "' parameters \"xfer_en_port\" and \"immediate_timing\" cannot both be set";
      throw xtsc_exception(oss.str());
    }
    if (m_is_pwc) {
      ostringstream oss;
      oss << kind() << " '" << name() << "' parameter \"xfer_en_port\" cannot be set when acting as a PIF-width converter";
      throw xtsc_exception(oss.str());
    }
    if (m_external_cbox) {
      ostringstream oss;
      oss << kind() << " '" << name() << "' parameters \"xfer_en_port\" and \"external_cbox\" cannot both be set";
      throw xtsc_exception(oss.str());
    }
  }

  if (m_apb) {
    ostringstream oss;
    oss << " parameter must be left at its default value when \"apb\" is true in xtsc_arbiter_parms for xtsc_arbiter '" << name() << "'";

    if (m_immediate_timing    != false         ) { throw xtsc_exception("\"immediate_timing\""    + oss.str()); }
    if (m_dram_lock           != false         ) { throw xtsc_exception("\"dram_lock\""           + oss.str()); }
    if (m_external_cbox       != false         ) { throw xtsc_exception("\"external_cbox\""       + oss.str()); }
    if (m_fail_port_mask      != false         ) { throw xtsc_exception("\"fail_port_mask\""      + oss.str()); }
    if (m_is_pwc              != false         ) { throw xtsc_exception("\"master_byte_widths\""  + oss.str()); }
    if (m_check_route_id_bits != true          ) { throw xtsc_exception("\"check_route_id_bits\"" + oss.str()); }
    if (m_route_id_bits_shift != 0             ) { throw xtsc_exception("\"route_id_lsb\""        + oss.str()); }
    if (m_num_route_ids       != 16            ) { throw xtsc_exception("\"num_route_ids\""       + oss.str()); }
    if (m_response_repeat     != m_clock_period) { throw xtsc_exception("\"response_repeat\""     + oss.str()); }
    if (m_xfer_en_port        != 0xFFFFFFFF    ) { throw xtsc_exception("\"xfer_en_port\""        + oss.str()); }
    if (  nacc_wait_time      != 0xFFFFFFFF    ) { throw xtsc_exception("\"nacc_wait_time\""      + oss.str()); }

    m_one_at_a_time             = false;
  }

  const char *arbitration_policy = arbiter_parms.get_c_str("arbitration_policy");
  if (arbitration_policy && arbitration_policy[0]) {
    m_arbitration_policy = arbitration_policy;
    if (m_immediate_timing) {
      ostringstream oss;
      oss << kind() << " '" << name() << "' parameters \"arbitration_policy\" and \"immediate_timing\" cannot both be set";
      throw xtsc_exception(oss.str());
    }
    if (m_is_pwc) {
      ostringstream oss;
      oss << kind() << " '" << name() << "' parameter \"arbitration_policy\" cannot be set when acting as a PIF-width converter";
      throw xtsc_exception(oss.str());
    }
    if (m_external_cbox) {
      ostringstream oss;
      oss << kind() << " '" << name() << "' parameters \"arbitration_policy\" and \"external_cbox\" cannot both be set";
      throw xtsc_exception(oss.str());
    }
    if (m_xfer_en_port != 0xFFFFFFFF) {
      ostringstream oss;
      oss << kind() << " '" << name() << "' parameters \"arbitration_policy\" and \"xfer_en_port\" cannot both be set";
      throw xtsc_exception(oss.str());
    }
    port_policy_info *p_info = NULL;
    string::size_type beg = 0;
    string port_policy;
    bool no_more = false;
    for (u32 i=0; i<m_num_masters; ++i) {
      if (no_more) {
        m_port_policy_table.push_back(new port_policy_info(*p_info));
      }
      else {
        string::size_type next = m_arbitration_policy.find_first_of(";", beg);
        if (next == string::npos) {
          no_more = true;
          port_policy = m_arbitration_policy.substr(beg);
        }
        else {
          port_policy = m_arbitration_policy.substr(beg, next - beg);
          beg = next + 1;
        }
        const char *port_policy_value_names[3] = { "<StartPriority>", "<EndPriority>", "Decrement" };
        vector<string> port_policy_string;
        vector<u32>    port_policy_u32;
        string::size_type pos1 = port_policy.find_first_of(",");
        if (pos1 == string::npos) {
          ostringstream oss;
          oss << kind() << " '" << name() << "' Syntax error: No comma found in <Port" << i << "Policy> (" << port_policy
              << ") in \"arbitration_policy\": " << m_arbitration_policy;
          throw xtsc_exception(oss.str());
        }
        string::size_type pos2 = port_policy.find_first_of(",", pos1+1);
        if (pos2 == string::npos) {
          ostringstream oss;
          oss << kind() << " '" << name() << "' Syntax error: Only 1 comma found in <Port" << i << "Policy> (" << port_policy
              << ") in \"arbitration_policy\": " << m_arbitration_policy;
          throw xtsc_exception(oss.str());
        }
        string::size_type pos3 = port_policy.find_first_of(",", pos2+1);
        if (pos3 != string::npos) {
          ostringstream oss;
          oss << kind() << " '" << name() << "' Syntax error: Extra comma found in <Port" << i << "Policy> (" << port_policy
              << ") in \"arbitration_policy\": " << m_arbitration_policy;
          throw xtsc_exception(oss.str());
        }
        if (pos1 == 0) {
          ostringstream oss;
          oss << kind() << " '" << name() << "' Syntax error: <StartPriority> is empty in <Port" << i << "Policy> (" << port_policy
              << ") in \"arbitration_policy\": " << m_arbitration_policy;
          throw xtsc_exception(oss.str());
        }
        if (pos2 == pos1+1) {
          ostringstream oss;
          oss << kind() << " '" << name() << "' Syntax error: <EndPriority> is empty in <Port" << i << "Policy> (" << port_policy
              << ") in \"arbitration_policy\": " << m_arbitration_policy;
          throw xtsc_exception(oss.str());
        }
        if (pos2 == port_policy.length() - 1) {
          ostringstream oss;
          oss << kind() << " '" << name() << "' Syntax error: <Decrement> is empty in <Port" << i << "Policy> (" << port_policy
              << ") in \"arbitration_policy\": " << m_arbitration_policy;
          throw xtsc_exception(oss.str());
        }
        port_policy_string.push_back(port_policy.substr(0,      pos1       ));
        port_policy_string.push_back(port_policy.substr(pos1+1, pos2-pos1-1));
        port_policy_string.push_back(port_policy.substr(pos2+1             ));
        for (u32 j=0; j<3; ++j) {
          try {
            port_policy_u32.push_back(xtsc_strtou32(port_policy_string[j]));
          }
          catch (const xtsc_exception&) {
            ostringstream oss;
            oss << kind() << " '" << name() << "' Cannot convert " << port_policy_value_names[j] << " (" << port_policy_string[j]
                << ") to number in <Port" << i << "Policy> (" << port_policy << ") in \"arbitration_policy\": " << m_arbitration_policy;
            throw xtsc_exception(oss.str());
          }
        }
        p_info = new port_policy_info;
        p_info->m_start_priority   = port_policy_u32[0];
        p_info->m_end_priority     = port_policy_u32[1];
        p_info->m_decrement        = port_policy_u32[2];
        p_info->m_current_priority = port_policy_u32[0];
        m_port_policy_table.push_back(p_info);
        if (p_info->m_end_priority > p_info->m_start_priority) {
          ostringstream oss;
          oss << kind() << " '" << name() << "' <EndPriority> (" << port_policy_string[1]
              << ") must be numerically less than or equal to <StartPriority> (" << port_policy_string[0] << ") in <Port" << i
              << "Policy> (" << port_policy << ") in \"arbitration_policy\": " << m_arbitration_policy;
          throw xtsc_exception(oss.str());
        }
        if ((p_info->m_end_priority == p_info->m_start_priority) && (p_info->m_decrement != 0)) {
          ostringstream oss;
          oss << kind() << " '" << name() << "' <Decrement> (" << port_policy_string[2] << ") must be zero if <StartPriority> ("
              << port_policy_string[0] << ") and <EndPriority> (" << port_policy_string[1] << ") are equal in <Port" << i
              << "Policy> (" << port_policy << ") in \"arbitration_policy\": " << m_arbitration_policy;
          throw xtsc_exception(oss.str());
        }
      }
    }
    if (!no_more) {
      ostringstream oss;
      oss << kind() << " '" << name()
          << "' parameter \"arbitration_policy\" syntax error: Extra characters or too many <PortNPolicy> entries found ("
          << m_num_masters << " expected): " << m_arbitration_policy;
      throw xtsc_exception(oss.str());
    }
    m_ports_with_requests = new u32[m_num_masters];
  }

  const char *lock_port_groups = arbiter_parms.get_c_str("lock_port_groups");
  if (lock_port_groups && lock_port_groups[0]) {
    m_lock_port_groups = lock_port_groups;
    m_use_lock_port_groups = true;
    if (!m_dram_lock) {
      ostringstream oss;
      oss << kind() << " '" << name() << "' setting parameter \"lock_port_groups\" requires setting \"dram_lock\" to true";
      throw xtsc_exception(oss.str());
    }
    if (m_arbitration_policy == "") {
      ostringstream oss;
      oss << kind() << " '" << name() << "' setting parameter \"lock_port_groups\" requires also setting \"arbitration_policy\""
          << " (if you only want fair round-robin you can set it to \"0,0,0\" to comply with this requirement)";
      throw xtsc_exception(oss.str());
    }
    for (u32 i=0; i<m_num_masters; ++i) {
      m_lock_port_group_table.push_back(NULL);
    }
    string::size_type group_beg = 0;
    string lock_port_group;
    bool more_groups = true;
    u32 group_cnt = 0;
    do {
      string::size_type next_group = m_lock_port_groups.find_first_of(";", group_beg);
      if (next_group == string::npos) {
        lock_port_group = m_lock_port_groups.substr(group_beg);
        more_groups = false;
      }
      else {
        lock_port_group = m_lock_port_groups.substr(group_beg, next_group - group_beg);
        group_beg = next_group + 1;
      }
      group_cnt += 1;
      string::size_type port_beg = 0;
      string port;
      bool more_ports = true;
      u32 port_cnt = 0;
      vector<u32> *group = new vector<u32>;
      do {
        string::size_type next_port = lock_port_group.find_first_of(",", port_beg);
        if (next_port == string::npos) {
          port = lock_port_group.substr(port_beg);
          more_ports = false;
        }
        else {
          port = lock_port_group.substr(port_beg, next_port - port_beg);
          port_beg = next_port + 1;
        }
        port_cnt += 1;
        u32 p = 0;
        try {
          p = xtsc_strtou32(port);
        }
        catch (const xtsc_exception&) {
          ostringstream oss;
          oss << kind() << " '" << name() << "' Cannot convert <Port> #" << port_cnt << " (" << port
              << ") to number in <LockPortGroup> #" << group_cnt << " (" << lock_port_group << ") in \"lock_port_groups\": "
              << m_lock_port_groups;
          throw xtsc_exception(oss.str());
        }
        if (p >= m_num_masters) {
          ostringstream oss;
          oss << kind() << " '" << name() << "' <Port> #" << port_cnt << " (" << port << ") must be less than \"num_masters\" ("
              << m_num_masters << ") in <LockPortGroup> #" << group_cnt << " (" << lock_port_group << ") in \"lock_port_groups\": "
              << m_lock_port_groups;
          throw xtsc_exception(oss.str());
        }
        if (m_lock_port_group_table[p]) {
          ostringstream oss;
          oss << kind() << " '" << name() << "' Port #" << p << " appears multiple times in \"lock_port_groups\": "
              << m_lock_port_groups;
          throw xtsc_exception(oss.str());
        }
        m_lock_port_group_table[p] = group;
        group->push_back(p);
      } while (more_ports);
    } while (more_groups);
    for (u32 p=0; p<m_num_masters; ++p) {
      if (m_lock_port_group_table[p] == NULL) {
        vector<u32> *group = new vector<u32>;
        group->push_back(p);
        m_lock_port_group_table[p] = group;
      }
    }
  }

  // Build address translation tables?
  m_do_translation = false;
  const char *translation_file = arbiter_parms.get_c_str("translation_file");
  string tran_file;
  if (translation_file && translation_file[0]) {
    u32 n = xtsc_address_nibbles();
    m_do_translation = true;
    for (u32 i=0; i<m_num_masters; ++i) {
      m_translation_tables.push_back(new vector<xtsc_address_range_entry*>);
    }
    xtsc_script_file    file(translation_file, "translation_file", name(), kind(), false);
    u32                 line_count;
    vector<string>      words;
    string              line;
    tran_file = file.name();
    while ((line_count = file.get_words(words, line)) != 0) {
      u32 num_words = words.size();
      if ((num_words != 3) && (num_words != 4)) {
        ostringstream oss;
        oss << "Found " << num_words << " words (expected 3 or 4):" << endl;
        oss << line;
        oss << file.info_for_exception();
        throw xtsc_exception(oss.str());
      }
      u64 value[4];
      for (u32 i=0; i<num_words; ++i) {
        string word = words[i];
        try {
          value[i] = xtsc_strtou64(word);
        }
        catch (const xtsc_exception&) {
          ostringstream oss;
          oss << "Cannot convert word #" << i+1 << " '" << word << "' to number:" << endl;
          oss << line;
          oss << file.info_for_exception();
          throw xtsc_exception(oss.str());
        }
      }
      u32          port_num         = value[0];
      xtsc_address low_address      = value[1];
      xtsc_address high_address     = ((num_words == 4) ? value[2] : XTSC_MAX_ADDRESS);
      xtsc_address new_base_address = ((num_words == 4) ? value[3] : value[2]);
      xtsc_address delta            = new_base_address - low_address;
      if (port_num >= m_num_masters) {
        ostringstream oss;
        oss << "<PortNum>=" << port_num << " must be less than \"num_masters\"=" << m_num_masters << ":" << endl;
        oss << line;
        oss << file.info_for_exception();
        throw xtsc_exception(oss.str());
      }
      if (high_address < low_address) {
        ostringstream oss;
        oss << "<HighAddress>=0x" << hex << setw(n) << high_address << " cannot be less than <LowAddress>=0x" << setw(n) << low_address << ":" << endl;
        oss << line;
        oss << file.info_for_exception();
        throw xtsc_exception(oss.str());
      }
      xtsc_address_range_entry *p_entry = new xtsc_address_range_entry(low_address, high_address, port_num, delta);
      vector<xtsc_address_range_entry*> *p_translation_table = m_translation_tables[port_num];
      p_translation_table->push_back(p_entry);
    }
    // Sort each translation table and check for overlap
    for (u32 i=0; i<m_num_masters; ++i) {
      vector<xtsc_address_range_entry*> *p_translation_table = m_translation_tables[i];
      sort(p_translation_table->begin(), p_translation_table->end(), start_address_less_then);
      vector<xtsc_address_range_entry*>::iterator itt1 = p_translation_table->begin();
      if (itt1 != p_translation_table->end()) {
        XTSC_DEBUG(m_text, "translate: " << **itt1);
        vector<xtsc_address_range_entry*>::iterator itt2 = itt1;
        for (++itt2; itt2 != p_translation_table->end(); itt1 = itt2++) {
          XTSC_DEBUG(m_text, "translate: " << **itt2);
          if ((*itt1)->m_end_address8 >= (*itt2)->m_start_address8) {
            ostringstream oss;
            oss << kind() << " '" << name() << "':  Two address translations overlap for port #" << i << ": "
                << **itt1 << " and " << **itt2;
            throw xtsc_exception(oss.str());
          }
        }
      }
    }
  }

  if (!m_immediate_timing) {
    if (m_is_pwc) {
      SC_THREAD(arbiter_pwc_thread);                            m_process_handles.push_back(sc_get_current_process_handle());
      SC_THREAD(response_pwc_thread);                           m_process_handles.push_back(sc_get_current_process_handle());
    }
    else if (m_external_cbox || (m_xfer_en_port != 0xFFFFFFFF)) {
      SC_THREAD(arbiter_special_thread);                        m_process_handles.push_back(sc_get_current_process_handle());
      SC_THREAD(response_thread);                               m_process_handles.push_back(sc_get_current_process_handle());
    }
    else if (m_apb) {
      SC_THREAD(arbiter_apb_thread);                            m_process_handles.push_back(sc_get_current_process_handle());
      SC_THREAD(response_thread);                               m_process_handles.push_back(sc_get_current_process_handle());
    }
    else {
      SC_THREAD(arbiter_thread);                                m_process_handles.push_back(sc_get_current_process_handle());
      SC_THREAD(response_thread);                               m_process_handles.push_back(sc_get_current_process_handle());
    }
    if (m_align_request_phase && !m_apb) {
      SC_THREAD(align_request_phase_thread);                    m_process_handles.push_back(sc_get_current_process_handle());
    }
  }

  xtsc_register_command(*this, *this, "change_clock_period", 1, 2,
      "change_clock_period <ClockPeriodFactor> [<ArbitrationPhaseFactor>]", 
      "Call xtsc_arbiter::change_clock_period(<ClockPeriodFactor>, <ArbitrationPhaseFactor>)."
      "  Default <ArbitrationPhaseFactor> is a proportional change.  Return previous <ClockPeriodFactor> and <<ArbitrationPhaseFactor>."
  );

  xtsc_register_command(*this, *this, "dump_profile_results", 0, 0,
      "dump_profile_results",
      "Dump max used buffers for request and response fifos."
  );
  
  xtsc_register_command(*this, *this, "reset", 0, 1,
      "reset", 
      "Call xtsc_arbiter::reset()."
  );
  
    
  xtsc_event_register(m_arbiter_thread_event,             "m_arbiter_thread_event",             this);
  xtsc_event_register(m_response_thread_event,            "m_response_thread_event",            this);
  xtsc_event_register(m_apb_response_done_event,          "m_apb_response_done_event",          this);
  xtsc_event_register(m_align_request_phase_thread_event, "m_align_request_phase_thread_event", this);

  log4xtensa::LogLevel ll = xtsc_get_constructor_log_level();
  XTSC_LOG(m_text, ll, "Constructed xtsc_arbiter '" << name() << "':");
  XTSC_LOG(m_text, ll, " apb                    = "   << boolalpha << m_apb);
  XTSC_LOG(m_text, ll, " num_masters            = "   << m_num_masters);
  if (m_apb) { /* do nothing */ }
  else if (m_route_id_bits_shift == 0xFFFFFFFF) {
  XTSC_LOG(m_text, ll, " route_id_lsb           = 0xFFFFFFFF => Use routing table");
  XTSC_LOG(m_text, ll, " num_route_ids          = "   << m_num_route_ids);
  }
  else {
  XTSC_LOG(m_text, ll, " route_id_lsb           = "   << m_route_id_bits_shift);
  XTSC_LOG(m_text, ll, " check_route_id_bits    = "   << boolalpha << m_check_route_id_bits);
  }
  { ostringstream oss; if (m_is_pwc) for (u32 i=0; i<m_num_masters; ++i) oss << (i ? "," : "") << m_master_byte_widths[i];
  XTSC_LOG(m_text, ll, " master_byte_widths     = "   << oss.str());
  }
  if (m_is_pwc) {
  XTSC_LOG(m_text, ll, " slave_byte_width       = "   << m_slave_byte_width);
  XTSC_LOG(m_text, ll, " use_block_requests     = "   << boolalpha << m_use_block_requests);
  }
  XTSC_LOG(m_text, ll, " translation_file       = "   << tran_file);
  XTSC_LOG(m_text, ll, " dram_lock              = "   << (m_dram_lock        ? "true" : "false"));
  XTSC_LOG(m_text, ll, " lock_port_groups       = "   << m_lock_port_groups);
  XTSC_LOG(m_text, ll, " arbitration_policy     = "   << m_arbitration_policy);
  XTSC_LOG(m_text, ll, " external_cbox          = "   << (m_external_cbox    ? "true" : "false"));
  if (m_xfer_en_port == 0xFFFFFFFF) {
  XTSC_LOG(m_text, ll, " xfer_en_port           = 0xFFFFFFFF => (xtsc_request::get_xfer_en() will not be used)");
  } else {
  XTSC_LOG(m_text, ll, " xfer_en_port           = "   << m_xfer_en_port);
  }
  XTSC_LOG(m_text, ll, " read_only              = "   << boolalpha << m_read_only);
  XTSC_LOG(m_text, ll, " write_only             = "   << boolalpha << m_write_only);
  XTSC_LOG(m_text, ll, " log_peek_poke          = "   << boolalpha << m_log_peek_poke);
  XTSC_LOG(m_text, ll, " immediate_timing       = "   << (m_immediate_timing ? "true" : "false"));
  XTSC_LOG(m_text, ll, " request_fifo_depth     = "   << arbiter_parms.get_u32("request_fifo_depth"));
  {
    ostringstream oss;
    for (u32 i=0; request_fifo_depths.size() && i<m_num_masters; i++) {
      if (i) oss << ",";
      oss << request_fifo_depths[i];
    }
    if (request_fifo_depths.size() && m_one_at_a_time) {
      oss << " (Did you intend to leave \"one_at_a_time\" true?)";
    }
  XTSC_LOG(m_text, ll, " request_fifo_depths    = "   << oss.str());
  }
  XTSC_LOG(m_text, ll, " response_fifo_depth    = "   << arbiter_parms.get_u32("response_fifo_depth"));
  if (!m_immediate_timing) {
  if (clock_period == 0xFFFFFFFF) {
  XTSC_LOG(m_text, ll, " clock_period           = 0xFFFFFFFF => " << m_clock_period.value() << " (" << m_clock_period << ")");
  } else {
  XTSC_LOG(m_text, ll, " clock_period           = "   << clock_period << " (" << m_clock_period << ")");
  }
  if (posedge_offset == 0xFFFFFFFF) {
  XTSC_LOG(m_text, ll, " posedge_offset         = 0xFFFFFFFF => " << m_posedge_offset.value() << " (" << m_posedge_offset << ")");
  } else {
  XTSC_LOG(m_text, ll, " posedge_offset         = "   << posedge_offset << " (" << m_posedge_offset << ")");
  }
  if (arbitration_phase == 0xFFFFFFFF) {
  XTSC_LOG(m_text, ll, " arbitration_phase      = 0xFFFFFFFF => " << m_arbitration_phase.value() << " (" << m_arbitration_phase << ")");
  } else {
  XTSC_LOG(m_text, ll, " arbitration_phase      = "   << arbitration_phase << " (" << m_arbitration_phase << ")");
  }
  if (!m_apb) {
  if (nacc_wait_time == 0xFFFFFFFF) {
  XTSC_LOG(m_text, ll, " nacc_wait_time         = 0xFFFFFFFF => " << m_nacc_wait_time.value() << " (" << m_nacc_wait_time << ")");
  } else {
  XTSC_LOG(m_text, ll, " nacc_wait_time         = "   << nacc_wait_time << " (" << m_nacc_wait_time << ")");
  }
  XTSC_LOG(m_text, ll, " one_at_a_time          = "   << boolalpha << m_one_at_a_time);
  }
  XTSC_LOG(m_text, ll, " delay_from_receipt     = "   << (m_delay_from_receipt ? "true" : "false"));
  XTSC_LOG(m_text, ll, " request_delay          = "   << arbiter_parms.get_u32("request_delay"));
  XTSC_LOG(m_text, ll, " response_delay         = "   << arbiter_parms.get_u32("response_delay"));
  if (!m_apb) {
  XTSC_LOG(m_text, ll, " response_repeat        = "   << arbiter_parms.get_u32("response_repeat"));
  }
  XTSC_LOG(m_text, ll, " recovery_time          = "   << arbiter_parms.get_u32("recovery_time"));
  XTSC_LOG(m_text, ll, " align_request_phase    = "   << boolalpha << m_align_request_phase);
  }
  XTSC_LOG(m_text, ll, " fail_port_mask         = 0x" << hex << m_fail_port_mask);
  if (m_fail_port_mask) {
  XTSC_LOG(m_text, ll, " fail_percentage        = "   << m_fail_percentage);
  XTSC_LOG(m_text, ll, " fail_seed              = "   << m_fail_seed);
  XTSC_LOG(m_text, ll, " Maximum random value   = 0xFFFFFFFF");
  }
  XTSC_LOG(m_text, ll, " profile_buffers        = "   << m_profile_buffers);
  
  if (m_is_pwc) {
    for (u32 i=0; i<8; ++i) {
      m_requests[i] = NULL;
      m_responses[i] = NULL;
    }
  }

  reset(true);

}



xtsc_component::xtsc_arbiter::~xtsc_arbiter(void) {

  if (m_profile_buffers) {
    delete [] m_max_num_requests;
    delete [] m_max_num_requests_timestamp;
    delete [] m_max_num_requests_tag;
  }
  
  if (m_request_exports) {
    for (u32 i=0; i<m_num_masters; i++) {
      if (m_request_exports[i]) {
        delete m_request_exports[i];
        m_request_exports[i] = 0;
      }
    }
    delete [] m_request_exports;
    m_request_exports = 0;
  }

  if (m_request_fifos) {
    for (u32 i=0; i<m_num_masters; i++) {
      if (m_request_fifos[i]) {
        delete m_request_fifos[i];
        m_request_fifos[i] = 0;
      }
    }
    delete [] m_request_fifos;
    m_request_fifos = 0;
  }

  if (m_request_deques) {
    for (u32 i=0; i<m_num_masters; i++) {
      if (m_request_deques[i]) {
        delete m_request_deques[i];
        m_request_deques[i] = 0;
      }
    }
    delete [] m_request_deques;
    m_request_deques = 0;
  }

  if (m_respond_ports) {
    for (u32 i=0; i<m_num_masters; i++) {
      if (m_respond_ports[i]) {
        delete m_respond_ports[i];
        m_respond_ports[i] = 0;
      }
    }
    delete [] m_respond_ports;
    m_respond_ports = 0;
  }

  // Delete each xtsc_address_range_entry and each translation table
  if (m_do_translation) {
    for (u32 i=0; i<m_num_masters; ++i) {
      vector<xtsc_address_range_entry*> *p_translation_table = m_translation_tables[i];
      vector<xtsc_address_range_entry*>::iterator itt = p_translation_table->begin();
      for (; itt != p_translation_table->end(); ++itt) {
        xtsc_address_range_entry *p_entry = *itt;
        delete p_entry;
      }
      delete p_translation_table;
    }
  }
}



u32 xtsc_component::xtsc_arbiter::get_bit_width(const string& port_name, u32 interface_num) const {
  if (!m_is_pwc) return 0;

       if (port_name == "m_request_port")   return m_slave_byte_width*8;
  else if (port_name == "m_respond_export") return m_slave_byte_width*8;

  string name_portion;
  u32    index;
  xtsc_parse_port_name(port_name, name_portion, index);

  if ((name_portion != "m_respond_ports") && (name_portion != "m_request_exports")) {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\" has no port named \"" << port_name << "\"" << endl;
    throw xtsc_exception(oss.str());
  }

  if (index > m_num_masters - 1) {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\": Invalid index: \"" << port_name << "\".  Valid range: 0-" << (m_num_masters-1) << endl;
    throw xtsc_exception(oss.str());
  }

  return m_master_byte_widths[index]*8;
}



sc_object *xtsc_component::xtsc_arbiter::get_port(const string& port_name) {
       if (port_name == "m_request_port")   return &m_request_port;
  else if (port_name == "m_respond_export") return &m_respond_export;

  string name_portion;
  u32    index;
  bool   indexed = xtsc_parse_port_name(port_name, name_portion, index);

  if (((m_num_masters > 1) && !indexed) ||
      ((name_portion != "m_respond_ports") && (name_portion != "m_request_exports")))
  {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\" has no port named \"" << port_name << "\"" << endl;
    throw xtsc_exception(oss.str());
  }

  if (index > m_num_masters - 1) {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\": Invalid index: \"" << port_name << "\".  Valid range: 0-" << (m_num_masters-1) << endl;
    throw xtsc_exception(oss.str());
  }

  return ((name_portion == "m_respond_ports") ? (sc_object*) m_respond_ports[index] : (sc_object*) m_request_exports[index]);
}



xtsc_port_table xtsc_component::xtsc_arbiter::get_port_table(const string& port_table_name) const {
  xtsc_port_table table;

  if (port_table_name == "master_port") {
    table.push_back("m_request_port");
    table.push_back("m_respond_export");
    return table;
  }

  string name_portion;
  u32    index;
  bool   indexed = xtsc_parse_port_name(port_table_name, name_portion, index);

  if ((name_portion == "slave_ports") && !indexed) {
    ostringstream oss;
    for (u32 i=0; i<m_num_masters; ++i) {
      oss.str(""); oss << "slave_port" << "[" << i << "]"; table.push_back(oss.str());
    }
    return table;
  }

  if ((name_portion != "slave_port") || ((m_num_masters > 1) && !indexed)) {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\" has no port table named \"" << port_table_name << "\"" << endl;
    throw xtsc_exception(oss.str());
  }

  if (index > m_num_masters - 1) {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\": Invalid index: \"" << port_table_name << "\".  Valid range: 0-" << (m_num_masters-1)
        << endl;
    throw xtsc_exception(oss.str());
  }

  ostringstream oss;
  oss.str(""); oss << "m_request_exports" << "[" << index << "]"; table.push_back(oss.str());
  oss.str(""); oss << "m_respond_ports"   << "[" << index << "]"; table.push_back(oss.str());

  return table;
}



void xtsc_component::xtsc_arbiter::reset(bool /*hard_reset*/) {
  XTSC_INFO(m_text, "xtsc_arbiter::reset()");

  m_last_request_time_stamp     = SC_ZERO_TIME - (m_delay_from_receipt ? m_recovery_time : m_request_delay);
  m_last_response_time_stamp    = SC_ZERO_TIME - (m_delay_from_receipt ? m_recovery_time : m_response_delay);

  m_p_apb_request_info          = NULL;
  m_waiting_for_nacc            = false;
  m_request_got_nacc            = false;
  m_token                       = m_num_masters - 1;
  m_lock                        = false;

  if (m_apb) { /* do nothing */ }
  else if (m_route_id_bits_shift == 0xFFFFFFFF) {
    m_route_ids_used = 0;
    for (u32 i=0; i<m_num_route_ids; ++i) {
      m_routing_table[i][0] = m_num_masters;
      m_routing_table[i][1] = 0xFFFFFFFF;
    }
    for (u32 i=0; i<m_num_masters; ++i) {
      m_downstream_route_id[i] = 0xFFFFFFFF;
    }
  }

  if (m_is_pwc) {
    m_pending_request_tag = 0;            // Indicates there is no pending request
    m_issued_split_requests.clear();
    m_req_rsp_table.clear();

    for (u32 i=0; (i<8) && (m_requests[i] != NULL); ++i) {
      delete_request_info(m_requests[i]);
    }
    for (u32 i=0; (i<8) && (m_responses[i] != NULL); ++i) {
      delete_response_info(m_responses[i]);
    }
  }

  if (m_arbitration_policy != "") {
    for (u32 i=0; i < m_num_masters; ++i) {
      m_port_policy_table[i]->m_current_priority = m_port_policy_table[i]->m_start_priority;
    }
  }

  if (m_dram_lock) {
    for (u32 i=0; i < m_num_masters; ++i) {
      m_dram_locks[i] = false;
    }
  }

  // Cancel notified events
  m_arbiter_thread_event            .cancel();
  m_response_thread_event           .cancel();
  m_apb_response_done_event         .cancel();
  m_align_request_phase_thread_event.cancel();

  xtsc_reset_processes(m_process_handles);
}



bool xtsc_component::xtsc_arbiter::arbitrate(u32 &port_num) {
  do {
    if (!m_lock) {
      port_num = (port_num + 1) % m_num_masters;
    }
    XTSC_DEBUG(m_text, "arbitrate() port_num=" << port_num << " m_token=" << m_token << 
                       " num_available()=" << m_request_fifos[port_num]->num_available());
    if (m_request_fifos[port_num]->num_available()) {
      XTSC_DEBUG(m_text, "arbitrate() got: " << m_request_deques[port_num]->front()->m_request);
      return true;
    }
  } while (port_num != m_token);
  return false;
}



bool xtsc_component::xtsc_arbiter::arbitrate_policy(u32 &port_num) {
  if (m_lock) {
    if (m_use_lock_port_groups) {
      vector<u32>& lock_port_group = *m_lock_port_group_table[port_num];
      for (vector<u32>::const_iterator i = lock_port_group.begin(); i != lock_port_group.end(); ++i) {
        if (m_request_fifos[*i]->num_available()) {
          port_num = *i;
          XTSC_DEBUG(m_text, "arbitrate_policy() locked to Port #" << *i << ": " << m_request_deques[*i]->front()->m_request);
          return true;
        }
      }
      XTSC_DEBUG(m_text, "arbitrate_policy() locked to Port #" << port_num << " but no request is present in its <LockPortGroup>");
      return false;
    }
    else {
      if (m_request_fifos[port_num]->num_available()) {
        XTSC_DEBUG(m_text, "arbitrate_policy() locked to Port #" << port_num << ": " << m_request_deques[port_num]->front()->m_request);
        return true;
      }
      else {
        XTSC_DEBUG(m_text, "arbitrate_policy() locked to Port #" << port_num << " but no request is present");
        return false;
      }
    }
  }
  else {
    u32 smallest_priority = 0xFFFFFFFF;
    u32 tentative_port    = 0xFFFFFFFF;
    u32 num_requests      = 0;
    for (u32 i=1; i<=m_num_masters; ++i) {
      u32 port = (port_num + i) % m_num_masters;
      if (m_request_fifos[port]->num_available()) {
        m_ports_with_requests[num_requests++] = port;
        if (m_port_policy_table[port]->m_current_priority < smallest_priority) {
          smallest_priority = m_port_policy_table[port]->m_current_priority;
          tentative_port = port;
        }
      }
    }
    if (!num_requests) {
      XTSC_DEBUG(m_text, "arbitrate_policy() No ports with requests");
      return false;
    }
    // Adjust current priority on ports which lost due to priority (but not those that lost just due to round-robin)
    for (u32 i=0; i<num_requests; ++i) {
      u32 port = m_ports_with_requests[i];
      if (m_port_policy_table[port]->m_current_priority > smallest_priority) {
        u32 delta = m_port_policy_table[port]->m_current_priority - m_port_policy_table[port]->m_end_priority;
        if (delta > m_port_policy_table[port]->m_decrement) {
          m_port_policy_table[port]->m_current_priority -= m_port_policy_table[port]->m_decrement;
        }
        else {
          m_port_policy_table[port]->m_current_priority = m_port_policy_table[port]->m_end_priority;
        }
      }
    }
    port_num = tentative_port;
    m_port_policy_table[port_num]->m_current_priority = m_port_policy_table[port_num]->m_start_priority;
    XTSC_DEBUG(m_text, "arbitrate_policy() Port #" << port_num << ": " << m_request_deques[port_num]->front()->m_request);
    return true;
  }
}



void xtsc_component::xtsc_arbiter::change_clock_period(u32 clock_period_factor, u32 arbitration_phase_factor) {
  m_clock_period      = m_time_resolution * clock_period_factor;
  m_arbitration_phase = m_time_resolution * arbitration_phase_factor;
  XTSC_INFO(m_text, "change_clock_period(" << clock_period_factor << ", " << arbitration_phase_factor << ") => " << m_clock_period <<
                    ", " << m_arbitration_phase);
  compute_delays();
}



void xtsc_component::xtsc_arbiter::compute_delays() {
  m_clock_period_value  = m_clock_period.value();

  u32 posedge_offset = m_arbiter_parms.get_u32("posedge_offset");
  if (m_posedge_offset >= m_clock_period) {
    ostringstream oss;
    oss << kind() << " '" << name() << "' parameter error:" << endl;
    oss << "\"posedge_offset\" (0x" << hex << posedge_offset << "=>" << m_posedge_offset
        << ") must be strictly less than clock period => " << m_clock_period << ")";
    throw xtsc_exception(oss.str());
  }

  if (m_arbitration_phase >= m_clock_period) {
    ostringstream oss;
    oss << "xtsc_arbiter '" << name() << "' \"arbitration_phase\"=" << m_arbitration_phase
        << " must be stricly less than \"clock_period=" << m_clock_period;
    throw xtsc_exception(oss.str());
  }
  m_arbitration_phase_plus_one = m_arbitration_phase + m_clock_period;

  m_request_delay       = m_clock_period * m_arbiter_parms.get_u32("request_delay");
  m_response_delay      = m_clock_period * m_arbiter_parms.get_u32("response_delay");
  m_response_repeat     = m_clock_period * m_arbiter_parms.get_u32("response_repeat");
  m_recovery_time       = m_clock_period * m_arbiter_parms.get_u32("recovery_time");

  u32 nacc_wait_time    = m_arbiter_parms.get_u32("nacc_wait_time");
  if (m_apb) { /* do nothing */ }
  else if (nacc_wait_time == 0xFFFFFFFF) {
    m_nacc_wait_time = m_clock_period;
  }
  else {
    m_nacc_wait_time = m_time_resolution * nacc_wait_time;
    if (m_nacc_wait_time > m_clock_period) {
      ostringstream oss;
      oss << "xtsc_arbiter '" << name() << "': \"nacc_wait_time\" of " << m_nacc_wait_time << " exceeds clock period of "
          << m_clock_period;
      throw xtsc_exception(oss.str());
    }
  }
}



void xtsc_component::xtsc_arbiter::execute(const string&                cmd_line, 
                                           const vector<string>&        words,
                                           const vector<string>&        words_lc,
                                           ostream&                     result)
{
  ostringstream res;

  if (false) {
  }
  else if (words[0] == "change_clock_period") {
    u32 clock_period_factor = xtsc_command_argtou32(cmd_line, words, 1);
    u32 arbitration_phase_factor = clock_period_factor * m_arbitration_phase.value() / m_clock_period.value();
    if (words.size() > 2) {
      arbitration_phase_factor = xtsc_command_argtou32(cmd_line, words, 2);
    }
    res << m_clock_period.value() << " " << m_arbitration_phase.value();
    change_clock_period(clock_period_factor, arbitration_phase_factor);
  }
  else if (words[0] == "dump_profile_results") {
    dump_profile_results(res); 
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



void xtsc_component::xtsc_arbiter::dump_profile_results(ostream& os) {
  if (m_profile_buffers) {
    sc_time sys_clock_period = xtsc_get_system_clock_period(); 
    double cycles;
    for (u32 i=0; i<m_num_masters; i++) {
      cycles =  m_max_num_requests_timestamp[i] / sys_clock_period;
      os  << "Max buffered requests[" << i << "]: " << setw(7) << m_max_num_requests[i] << " - " 
          << setw(8) << setprecision(1) << std::fixed << cycles << " - tag=" << m_max_num_requests_tag[i] << endl;         
    }
    cycles = m_max_num_responses_timestamp / sys_clock_period;
    os  << "Max buffered responses: " << setw(9) << m_max_num_responses << " - " 
        << setw(8) << setprecision(1) << std::fixed << cycles << " - tag=" << m_max_num_responses_tag << endl;  
  }
  else {
    os  << "\"profile_buffers\" is not enabled." << endl;
  } 
}



void xtsc_component::xtsc_arbiter::connect(xtsc_arbiter& arbiter, u32 port_num) {
  if (port_num >= m_num_masters) {
    ostringstream oss;
    oss << kind() << " '" << name() << "':  Invalid port_num specified to xtsc_arbiter::connect():  port_num=" << port_num;
    oss << ".  Valid range is 0 to " << (m_num_masters - 1) << ".";
    throw xtsc_exception(oss.str());
  }
  arbiter.m_request_port(*m_request_exports[port_num]);
  (*m_respond_ports[port_num])(arbiter.m_respond_export);
}



void xtsc_component::xtsc_arbiter::connect(xtsc_core& core, const char *memory_port_name, u32 port_num) {
  string lc = (memory_port_name ? memory_port_name : "");
  transform(lc.begin(), lc.end(), lc.begin(), ::tolower);
  if (lc == "inbound_pif" || lc == "snoop") {
    m_request_port(core.get_request_export(memory_port_name));
    core.get_respond_port(memory_port_name)(m_respond_export);
  }
  else {
    if (port_num >= m_num_masters) {
      ostringstream oss;
      oss << "xtsc_arbiter '" << name() << "':  Invalid port_num specified to xtsc_arbiter::connect():  port_num=" << port_num;
      oss << ".  Valid range is 0 to " << (m_num_masters - 1) << ".";
      throw xtsc_exception(oss.str());
    }
    core.get_request_port(memory_port_name)(*m_request_exports[port_num]);
    (*m_respond_ports[port_num])(core.get_respond_export(memory_port_name));
  }
}



void xtsc_component::xtsc_arbiter::connect(xtsc_dma_engine& dma_engine, u32 port_num) {
  if (port_num >= m_num_masters) {
    ostringstream oss;
    oss << kind() << " '" << name() << "':  Invalid port_num specified to xtsc_arbiter::connect():  port_num=" << port_num;
    oss << ".  Valid range is 0 to " << (m_num_masters - 1) << ".";
    throw xtsc_exception(oss.str());
  }
  dma_engine.m_request_port(*m_request_exports[port_num]);
  (*m_respond_ports[port_num])(dma_engine.m_respond_export);
}



void xtsc_component::xtsc_arbiter::connect(xtsc_master& master, u32 port_num) {
  if (port_num >= m_num_masters) {
    ostringstream oss;
    oss << kind() << " '" << name() << "':  Invalid port_num specified to xtsc_arbiter::connect():  port_num=" << port_num;
    oss << ".  Valid range is 0 to " << (m_num_masters - 1) << ".";
    throw xtsc_exception(oss.str());
  }
  master.m_request_port(*m_request_exports[port_num]);
  (*m_respond_ports[port_num])(master.m_respond_export);
}



void xtsc_component::xtsc_arbiter::connect(xtsc_memory_trace& memory_trace, u32 trace_port, u32 arbiter_port) {
  u32 num_slaves = memory_trace.get_num_ports();
  if (trace_port >= num_slaves) {
    ostringstream oss;
    oss << "Invalid trace_port=" << trace_port << " in connect(): " << endl;
    oss << memory_trace.kind() << " '" << memory_trace.name() << "' has " << num_slaves << " ports numbered from 0 to "
        << num_slaves-1 << endl;
    throw xtsc_exception(oss.str());
  }
  if (arbiter_port >= m_num_masters) {
    ostringstream oss;
    oss << kind() << " '" << name() << "':  Invalid arbiter_port specified to xtsc_arbiter::connect():  arbiter_port=" << arbiter_port;
    oss << ".  Valid range is 0 to " << (m_num_masters - 1) << ".";
    throw xtsc_exception(oss.str());
  }
  (*memory_trace.m_request_ports[trace_port])(*m_request_exports[arbiter_port]);
  (*m_respond_ports[arbiter_port])(*memory_trace.m_respond_exports[trace_port]);
}



void xtsc_component::xtsc_arbiter::connect(xtsc_pin2tlm_memory_transactor& pin2tlm, u32 tran_port, u32 arbiter_port) {
  u32 num_slaves = pin2tlm.get_num_ports();
  if (tran_port >= num_slaves) {
    ostringstream oss;
    oss << "Invalid tran_port=" << tran_port << " in connect(): " << endl;
    oss << pin2tlm.kind() << " '" << pin2tlm.name() << "' has " << num_slaves << " ports numbered from 0 to " << num_slaves-1 << endl;
    throw xtsc_exception(oss.str());
  }
  if (arbiter_port >= m_num_masters) {
    ostringstream oss;
    oss << kind() << " '" << name() << "':  Invalid arbiter_port specified to xtsc_arbiter::connect():  arbiter_port=" << arbiter_port;
    oss << ".  Valid range is 0 to " << (m_num_masters - 1) << ".";
    throw xtsc_exception(oss.str());
  }
  (*pin2tlm.m_request_ports[tran_port])(*m_request_exports[arbiter_port]);
  (*m_respond_ports[arbiter_port])(*pin2tlm.m_respond_exports[tran_port]);
}



void xtsc_component::xtsc_arbiter::connect(xtsc_router& router, u32 router_port, u32 arbiter_port) {
  u32 num_slaves = router.get_num_slaves();
  if (router_port >= num_slaves) {
    ostringstream oss;
    oss << "Invalid router_port=" << router_port << " in connect(): " << endl;
    oss << router.kind() << " '" << router.name() << "' has " << num_slaves << " ports numbered from 0 to " << num_slaves-1 << endl;
    throw xtsc_exception(oss.str());
  }
  if (arbiter_port >= m_num_masters) {
    ostringstream oss;
    oss << kind() << " '" << name() << "':  Invalid arbiter_port specified to xtsc_arbiter::connect():  arbiter_port=" << arbiter_port;
    oss << ".  Valid range is 0 to " << (m_num_masters - 1) << ".";
    throw xtsc_exception(oss.str());
  }
  (*router.m_request_ports[router_port])(*m_request_exports[arbiter_port]);
  (*m_respond_ports[arbiter_port])(*router.m_respond_exports[router_port]);
}



void xtsc_component::xtsc_arbiter::setup_random_rsp_nacc_responses(u32 port_mask, u32 fail_percentage) {
  if (m_apb) {
    throw xtsc_exception("The xtsc_arbiter::setup_random_rsp_nacc_responses() method is not supported when \"apb\" is true");
  }
  m_fail_port_mask  = port_mask;
  m_fail_percentage = fail_percentage;
  compute_let_through();
}



void xtsc_component::xtsc_arbiter::compute_let_through() {
  if (m_fail_percentage > 100) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': ";
    oss << "fail_percentage of " << m_fail_percentage << " is out of range (1-100).";
    throw xtsc_exception(oss.str());
  }
  u64 max = 0xffffffff;
  m_let_through = static_cast<u32>(max - ((max * m_fail_percentage) / 100));
  XTSC_DEBUG(m_text, "m_fail_percentage=" << m_fail_percentage << " => m_let_through=0x" << hex << m_let_through);
}



u32 xtsc_component::xtsc_arbiter::random() {
  m_z = 0x9069 * (m_z & 0xFFFF) + (m_z >> 16);
  m_w = 0x4650 * (m_w & 0xFFFF) + (m_w >> 16);
  return (m_z << 16) + m_w;
}



void xtsc_component::xtsc_arbiter::reset_fifos() {
  wait(SC_ZERO_TIME);
  // Flush FIFOs
  if (m_request_deques) {
    for (u32 i=0; i<m_num_masters; i++) {
      while (!m_request_deques[i]->empty()) {
        request_info *p_request_info = m_request_deques[i]->front();
        m_request_deques[i]->pop_front();
        delete_request_info(p_request_info);
      }
    }
  }
  if (m_request_fifos) {
    for (u32 i = 0; i < m_num_masters; i++) {
      while (m_request_fifos[i]->num_available()) {
        int dummy;
        m_request_fifos[i]->nb_read(dummy);
      }
    }
  }
  while (m_response_fifo.num_available()) {
    response_info *p_response_info;
    m_response_fifo.nb_read(p_response_info);
    delete_response_info(p_response_info);
  }
  if (m_align_request_phase) {
    while (m_phase_delay_fifo->num_available()) {
      request_info *p_request_info;
      m_phase_delay_fifo->nb_read(p_request_info);
      delete_request_info(p_request_info);
    }
  }
}



void xtsc_component::xtsc_arbiter::arbiter_thread(void) {
  bool use_policy = (m_arbitration_policy != "");

  try {
    // Reset internal fifos
    reset_fifos();

    while (true) {
      XTSC_DEBUG(m_text, __FUNCTION__ << " going to sleep.");
      wait(m_arbiter_thread_event);
      XTSC_DEBUG(m_text, __FUNCTION__ << " woke up.");
      bool request_granted;
      do {
        // Sync to arbitration phase of clock
        sc_time now = sc_time_stamp();
        sc_time phase_now = (now.value() % m_clock_period_value) * m_time_resolution;
        if (m_has_posedge_offset) {
          if (phase_now < m_posedge_offset) {
            phase_now += m_clock_period;
          }
          phase_now -= m_posedge_offset;
        }
        if (phase_now > m_arbitration_phase) {
          wait(m_arbitration_phase_plus_one - phase_now);
        }
        else if (phase_now < m_arbitration_phase) {
          wait(m_arbitration_phase - phase_now);
        }
        XTSC_DEBUG(m_text, __FUNCTION__ << " starting arbitration.");
        u32 port_num = m_token;
        request_granted = (use_policy ? arbitrate_policy(port_num) : arbitrate(port_num));
        if (request_granted) {
          // Get our current transaction
          request_info *p_request_info = m_request_deques[port_num]->front();
          m_request_deques[port_num]->pop_front();
          int dummy;
          m_request_fifos[port_num]->nb_read(dummy);
          XTSC_DEBUG(m_text, __FUNCTION__ << "() got: " << p_request_info->m_request);
          if (!m_one_at_a_time) {
            // Calculate delay (net => No Earlier Than time)
            sc_time receipt_net       = p_request_info->m_time_stamp + m_request_delay;
            sc_time last_request_net  = m_last_request_time_stamp + (m_delay_from_receipt ? m_recovery_time : m_request_delay);
            sc_time latest_net        = (receipt_net > last_request_net) ? receipt_net : last_request_net;
            sc_time now               = sc_time_stamp();
            sc_time delay             = (latest_net <= now) ? SC_ZERO_TIME : (latest_net - now);
            XTSC_DEBUG(m_text, __FUNCTION__ << "() doing wait for " << delay);
            wait(delay);
            XTSC_DEBUG(m_text, __FUNCTION__ << "() done with wait");
          }
          m_token = port_num;
          if (m_dram_lock) {
            if (!m_lock) {
              m_lock = m_dram_locks[m_token];
              if (m_lock) {
                m_request_port->nb_lock(true);
              }
            }
          }
          else {
            m_lock = !p_request_info->m_request.get_last_transfer();
          }
          handle_request(p_request_info, m_token, __FUNCTION__);
        }
        else if (m_lock && m_one_at_a_time) {
          nacc_remaining_requests();
        }
      } while (request_granted);
    }

  }
  catch (const sc_unwind_exception& error) { throw; }
  catch (const exception& error) {
    ostringstream oss;
    oss << "std::exception caught in " << __FUNCTION__ << ", an SC_THREAD of " << kind() << " '" << name() << "'." << endl;
    oss << "what(): " << error.what() << endl;
    oss << __FUNCTION__ << " now rethrowing:";
    xtsc_log_multiline(m_text, log4xtensa::FATAL_LOG_LEVEL, oss.str(), 2);
    throw;
  }

}



void xtsc_component::xtsc_arbiter::arbiter_apb_thread(void) {
  bool use_policy = (m_arbitration_policy != "");

  try {
    // Reset internal fifos
    reset_fifos();

    while (true) {
      XTSC_DEBUG(m_text, __FUNCTION__ << " going to sleep.");
      wait(m_arbiter_thread_event);
      XTSC_DEBUG(m_text, __FUNCTION__ << " woke up.");
      bool request_granted;
      do {
        // Sync to arbitration phase of clock
        sc_time now = sc_time_stamp();
        sc_time phase_now = (now.value() % m_clock_period_value) * m_time_resolution;
        if (m_has_posedge_offset) {
          if (phase_now < m_posedge_offset) {
            phase_now += m_clock_period;
          }
          phase_now -= m_posedge_offset;
        }
        if (phase_now > m_arbitration_phase) {
          wait(m_arbitration_phase_plus_one - phase_now);
        }
        else if (phase_now < m_arbitration_phase) {
          wait(m_arbitration_phase - phase_now);
        }
        XTSC_DEBUG(m_text, __FUNCTION__ << " starting arbitration.");
        u32 port_num = m_token;
        request_granted = (use_policy ? arbitrate_policy(port_num) : arbitrate(port_num));
        if (request_granted) {
          // Get our current transaction
          request_info *p_request_info = m_request_deques[port_num]->front();
          m_p_apb_request_info = p_request_info;
          m_request_deques[port_num]->pop_front();
          int dummy;
          m_request_fifos[port_num]->nb_read(dummy);
          XTSC_DEBUG(m_text, __FUNCTION__ << "() got: " << p_request_info->m_request);
          // Calculate delay (net => No Earlier Than time)
          sc_time receipt_net       = p_request_info->m_time_stamp + m_request_delay;
          sc_time last_request_net  = m_last_request_time_stamp + (m_delay_from_receipt ? m_recovery_time : m_request_delay);
          sc_time latest_net        = (receipt_net > last_request_net) ? receipt_net : last_request_net;
          sc_time now               = sc_time_stamp();
          sc_time delay             = (latest_net <= now) ? SC_ZERO_TIME : (latest_net - now);
          XTSC_DEBUG(m_text, __FUNCTION__ << "() doing wait for " << delay);
          wait(delay);
          XTSC_DEBUG(m_text, __FUNCTION__ << "() done with wait");
          m_token = port_num;
          if (m_align_request_phase) {
            // Calculate delay to match incoming phase
            u64 phase_delta = (sc_time_stamp().value() - p_request_info->m_time_stamp.value()) % m_clock_period_value;
            if (phase_delta) {
              sc_time delay = (m_clock_period_value - phase_delta) * m_time_resolution;
              XTSC_DEBUG(m_text, "Doing wait for " << delay << " to align with request phase");
              wait(delay);
            }
          }
          XTSC_INFO(m_text, p_request_info->m_request << " Port #" << port_num);
          m_request_port->nb_request(p_request_info->m_request);
          m_last_request_time_stamp = sc_time_stamp();
          while (m_p_apb_request_info) {
            wait(m_apb_response_done_event);
          }
        }
      } while (request_granted);
    }

  }
  catch (const sc_unwind_exception& error) { throw; }
  catch (const exception& error) {
    ostringstream oss;
    oss << "std::exception caught in " << __FUNCTION__ << ", an SC_THREAD of " << kind() << " '" << name() << "'." << endl;
    oss << "what(): " << error.what() << endl;
    oss << __FUNCTION__ << " now rethrowing:";
    xtsc_log_multiline(m_text, log4xtensa::FATAL_LOG_LEVEL, oss.str(), 2);
    throw;
  }

}



void xtsc_component::xtsc_arbiter::arbiter_pwc_thread(void) {

  try {
    // Reset internal fifos
    reset_fifos();

    while (true) {
      XTSC_DEBUG(m_text, __FUNCTION__ << " going to sleep.");
      wait(m_arbiter_thread_event);
      XTSC_DEBUG(m_text, __FUNCTION__ << " woke up.");
      bool request_granted;
      do {
        // Sync to arbitration phase of clock
        sc_time now = sc_time_stamp();
        sc_time phase_now = (now.value() % m_clock_period_value) * m_time_resolution;
        if (m_has_posedge_offset) {
          if (phase_now < m_posedge_offset) {
            phase_now += m_clock_period;
          }
          phase_now -= m_posedge_offset;
        }
        if (phase_now > m_arbitration_phase) {
          wait(m_arbitration_phase_plus_one - phase_now);
        }
        else if (phase_now < m_arbitration_phase) {
          wait(m_arbitration_phase - phase_now);
        }
        XTSC_DEBUG(m_text, __FUNCTION__ << " starting arbitration.");
        u32 port_num = m_token;
        if (request_granted = arbitrate(port_num)) {
          // Get our current transaction
          request_info *p_request_info = m_request_deques[port_num]->front();
          m_request_deques[port_num]->pop_front();
          int dummy;
          m_request_fifos[port_num]->nb_read(dummy);
          m_token = port_num;
          m_lock = !p_request_info->m_request.get_last_transfer();
          XTSC_DEBUG(m_text, __FUNCTION__ << "() got: " << p_request_info->m_request);

          u32 master_byte_width = m_master_byte_widths[port_num];

          if ((master_byte_width == m_slave_byte_width) || (p_request_info->m_request.is_axi_protocol() && master_byte_width < m_slave_byte_width)) {
            m_requests[0] = p_request_info;
          }
          else {
            convert_request(p_request_info, master_byte_width, port_num);
          }

          for (u32 i=0; (i<8) && (m_requests[i] != NULL); ++i) {
            if (!m_one_at_a_time) {
              // Calculate delay (net => No Earlier Than time)
              sc_time receipt_net       = m_requests[i]->m_time_stamp + m_request_delay;
              sc_time last_request_net  = m_last_request_time_stamp + (m_delay_from_receipt ? m_recovery_time : m_request_delay);
              sc_time latest_net        = (receipt_net > last_request_net) ? receipt_net : last_request_net;
              sc_time now               = sc_time_stamp();
              sc_time delay             = (latest_net <= now) ? SC_ZERO_TIME : (latest_net - now);
              XTSC_DEBUG(m_text, __FUNCTION__ << "() doing wait for " << delay);
              wait(delay);
              XTSC_DEBUG(m_text, __FUNCTION__ << "() done with wait");
            }
            handle_request(m_requests[i], port_num, __FUNCTION__);
          }
        }
        else if (m_lock && m_one_at_a_time) {
          nacc_remaining_requests();
        }
      } while (request_granted);
    }

  }
  catch (const sc_unwind_exception& error) { throw; }
  catch (const exception& error) {
    ostringstream oss;
    oss << "std::exception caught in " << __FUNCTION__ << ", an SC_THREAD of " << kind() << " '" << name() << "'." << endl;
    oss << "what(): " << error.what() << endl;
    oss << __FUNCTION__ << " now rethrowing:";
    xtsc_log_multiline(m_text, log4xtensa::FATAL_LOG_LEVEL, oss.str(), 2);
    throw;
  }

}



void xtsc_component::xtsc_arbiter::arbiter_special_thread(void) {

  try {

    // Reset internal fifos
    reset_fifos();

    while (true) {
      XTSC_DEBUG(m_text, __FUNCTION__ << " going to sleep.");
      wait(m_arbiter_thread_event);
      XTSC_DEBUG(m_text, __FUNCTION__ << " woke up.");
      bool request_granted;
      do {
        // Sync to arbitration phase of clock
        sc_time now = sc_time_stamp();
        sc_time phase_now = (now.value() % m_clock_period_value) * m_time_resolution;
        if (m_has_posedge_offset) {
          if (phase_now < m_posedge_offset) {
            phase_now += m_clock_period;
          }
          phase_now -= m_posedge_offset;
        }
        if (phase_now > m_arbitration_phase) {
          wait(m_arbitration_phase_plus_one - phase_now);
        }
        else if (phase_now < m_arbitration_phase) {
          wait(m_arbitration_phase - phase_now);
        }
        XTSC_DEBUG(m_text, __FUNCTION__ << " starting arbitration.");
        u32 port_num = m_token;
        request_granted = false;
        // Perform arbitration 
        do {
          if (!m_lock) {
            port_num = (port_num + 1) % m_num_masters;
          }
          XTSC_DEBUG(m_text, __FUNCTION__ << "() port_num=" << port_num << " m_token=" << m_token << 
                             " num_available()=" << m_request_fifos[port_num]->num_available());
          if (m_request_fifos[port_num]->num_available()) {
            // Get our current transaction
            request_info *p_request_info = m_request_deques[port_num]->front();
            // If operating as external CBox then decide if we have a simultaneous read/write to same address situation
            if (m_external_cbox && (p_request_info->m_request.get_type() == xtsc_request::WRITE) && !m_lock) {
              u32 other_port = ((port_num == 0) ? 1 : 0);
              if (m_request_fifos[other_port]->num_available()) {
                request_info *p_other_request_info = m_request_deques[other_port]->front();
                if (p_other_request_info->m_request.get_type() == xtsc_request::READ) {
                  xtsc_address  wr_beg = p_request_info      ->m_request.get_byte_address();
                  xtsc_address  rd_beg = p_other_request_info->m_request.get_byte_address();
                  u32           wr_end = p_request_info      ->m_request.get_byte_size() + wr_beg - 1;
                  u32           rd_end = p_other_request_info->m_request.get_byte_size() + rd_beg - 1;
                  if ((wr_end >= rd_beg) && (rd_end >= wr_beg)) {
                    // Other port has a READ to an overlapping address => switch to that port to give the READ priority
                    port_num = other_port;
                    p_request_info = p_other_request_info;
                  }
                }
              }
            }
            else if ((m_xfer_en_port != 0xFFFFFFFF) && (m_xfer_en_port != port_num)) {
              if (m_request_fifos[m_xfer_en_port]->num_available()) {
                request_info *p_other_request_info = m_request_deques[m_xfer_en_port]->front();
                if (p_other_request_info->m_request.get_xfer_en()) {
                  // The designated port has an xfer_en request => switch to that port to give it priority
                  port_num = m_xfer_en_port;
                  p_request_info = p_other_request_info;
                }
              }
            }
            m_request_deques[port_num]->pop_front();
            int dummy;
            m_request_fifos[port_num]->nb_read(dummy);
            request_granted = true;
            XTSC_DEBUG(m_text, __FUNCTION__ << "() got: " << p_request_info->m_request);
            if (!m_one_at_a_time) {
              // Calculate delay (net => No Earlier Than time)
              sc_time receipt_net       = p_request_info->m_time_stamp + m_request_delay;
              sc_time last_request_net  = m_last_request_time_stamp + (m_delay_from_receipt ? m_recovery_time : m_request_delay);
              sc_time latest_net        = (receipt_net > last_request_net) ? receipt_net : last_request_net;
              sc_time now               = sc_time_stamp();
              sc_time delay             = (latest_net <= now) ? SC_ZERO_TIME : (latest_net - now);
              XTSC_DEBUG(m_text, __FUNCTION__ << "() doing wait for " << delay);
              wait(delay);
              XTSC_DEBUG(m_text, __FUNCTION__ << "() done with wait");
            }
            m_token = port_num;
            if (m_dram_lock) {
              if (!m_lock) {
                m_lock = m_dram_locks[m_token];
                if (m_lock) {
                  m_request_port->nb_lock(true);
                }
              }
            }
            else {
              m_lock = !p_request_info->m_request.get_last_transfer();
            }
            handle_request(p_request_info, port_num, __FUNCTION__);
          }
          else if (m_lock && m_one_at_a_time) {
            nacc_remaining_requests();
          }
          if (m_lock && (m_xfer_en_port != 0xFFFFFFFF)) {
            ostringstream oss;
            oss << kind() << " '" << name() << "' is locked to a port but \"xfer_en_port\" was set.  This is not allowed.";
            throw xtsc_exception(oss.str());
          }
        } while (port_num != m_token);
      } while (request_granted);
    }

  }
  catch (const sc_unwind_exception& error) { throw; }
  catch (const exception& error) {
    ostringstream oss;
    oss << "std::exception caught in " << __FUNCTION__ << ", an SC_THREAD of " << kind() << " '" << name() << "'." << endl;
    oss << "what(): " << error.what() << endl;
    oss << __FUNCTION__ << " now rethrowing:";
    xtsc_log_multiline(m_text, log4xtensa::FATAL_LOG_LEVEL, oss.str(), 2);
    throw;
  }

}



// This method is used by arbiter_thread, arbiter_pwc_thread, and arbiter_special_thread; but not by arbiter_apb_thread
void xtsc_component::xtsc_arbiter::align_request_phase_thread(void) {
  request_info *p_request_info;

  try {

    while (true) {
      XTSC_DEBUG(m_text, __FUNCTION__ << " going to sleep.");
      wait(m_align_request_phase_thread_event);
      XTSC_DEBUG(m_text, __FUNCTION__ << " woke up.");
      while (m_phase_delay_fifo->num_available()) {
        // Get our current transaction
        m_phase_delay_fifo->nb_read(p_request_info);
        XTSC_DEBUG(m_text, __FUNCTION__ << "() got: " << p_request_info->m_request);
        // Calculate delay to match incoming phase
        u64 phase_delta = (sc_time_stamp().value() - p_request_info->m_time_stamp.value()) % m_clock_period_value;
        if (phase_delta) {
          sc_time delay = (m_clock_period_value - phase_delta) * m_time_resolution;
          XTSC_DEBUG(m_text, __FUNCTION__ << "() doing wait for " << delay);
          wait(delay);
        }
        xtsc_request *p_request = &p_request_info->m_request;
        u32 port_num = p_request_info->m_port_num;
        u32 tries = 0;
        do {
          m_request_got_nacc = false;
          tries += 1;
          XTSC_INFO(m_text, *p_request << " Port #" << port_num << "  Try #" << tries << " RteID=" << p_request->get_route_id());
          xtsc_log_memory_request_event(m_binary, INFO_LOG_LEVEL, false, 0, m_log_data_binary, *p_request);
          m_waiting_for_nacc = true;
          m_request_port->nb_request(*p_request);
          XTSC_DEBUG(m_text, __FUNCTION__ << " waiting for nacc wait time =" << m_nacc_wait_time);
          wait(m_nacc_wait_time);
          XTSC_DEBUG(m_text, __FUNCTION__ << " finished nacc wait time");
          m_waiting_for_nacc = false;
        } while (m_request_got_nacc);

        delete_request_info(p_request_info);
      }
    }

  }
  catch (const sc_unwind_exception& error) { throw; }
  catch (const exception& error) {
    ostringstream oss;
    oss << "std::exception caught in " << __FUNCTION__ << ", an SC_THREAD of " << kind() << " '" << name() << "'." << endl;
    oss << "what(): " << error.what() << endl;
    oss << __FUNCTION__ << " now rethrowing:";
    xtsc_log_multiline(m_text, log4xtensa::FATAL_LOG_LEVEL, oss.str(), 2);
    throw;
  }

}



void xtsc_component::xtsc_arbiter::convert_request(request_info*& p_request_info, u32 master_byte_width, u32 port_num) {
  req_rsp_info         *p_req_rsp_info  = NULL;
  bool                  first_transfer  = (m_pending_request_tag == 0);
  xtsc_request::type_t  type            = p_request_info->m_request.get_type();
  xtsc_request::burst_t burst_type      = p_request_info->m_request.get_burst_type();
  u32                   size            = p_request_info->m_request.get_byte_size();
  u32                   max_num_xfers   = (p_request_info->m_request.is_axi_protocol() && burst_type==xtsc_request::INCR) ? 256 : 16;

  XTSC_DEBUG(m_text, "Converting from " << master_byte_width << "-byte PIF: " << p_request_info->m_request);

  if ((type != xtsc_request::READ       ) &&
      (type != xtsc_request::BLOCK_READ ) &&
      (type != xtsc_request::WRITE      ) &&
      (type != xtsc_request::BLOCK_WRITE) &&
      (type != xtsc_request::RCW        ) &&
      !(type == xtsc_request::BURST_READ  && burst_type != xtsc_request::NON_AXI) &&
      !(type == xtsc_request::BURST_WRITE && burst_type != xtsc_request::NON_AXI))
  {
    ostringstream oss;
    oss << kind() << " '" << name() << "' received a " << xtsc_request::get_type_name(type)
        << " request which is not supported for PIF width conversion: " << p_request_info->m_request;
    throw xtsc_exception(oss.str());
  }

  if ((type == xtsc_request::RCW) && (size > m_slave_byte_width)) {
    ostringstream oss;
    oss << kind() << " '" << name() << "' received a RCW request with a size greater than downstream PIF width ("
        << m_slave_byte_width << "): " << p_request_info->m_request;
    throw xtsc_exception(oss.str());
  }

  if (first_transfer) {
    p_req_rsp_info = new_req_rsp_info(p_request_info);
    m_req_rsp_table.insert(pair<xtsc::u64, req_rsp_info*>(p_request_info->m_request.get_tag(), p_req_rsp_info));
    XTSC_DEBUG(m_text, "convert_request() adding request to the m_req_rsp_table map!" << p_request_info->m_request);
    request_info *p_req_info = copy_request_info(*p_request_info);

    p_req_rsp_info->m_num_last_xfer_rsp_expected = 1;   // Overrides below

    if (!p_request_info->m_request.get_last_transfer()) {
      m_pending_request_tag = p_request_info->m_request.get_tag();
    }
    if (master_byte_width > m_slave_byte_width) {
      // Master is wider than slave
      m_requests[0] = p_req_info;
      if (type == xtsc_request::READ || (type == xtsc_request::BURST_READ && (p_req_info->m_request.get_num_transfers() == 1))) {
        if (size > m_slave_byte_width) {
          p_req_info->m_request.set_byte_size(m_slave_byte_width);
          xtsc_byte_enables     mask    = ((size == 8) ? 0xFF : ((size == 16) ? 0xFFFF : 0xFFFFFFFF));
          xtsc_byte_enables     be      = p_request_info->m_request.get_byte_enables();
          u32                   reps    = size / m_slave_byte_width;
          // Optionally use BLOCK_READ if all bytes are enabled
          if (m_use_block_requests && ((be & mask) == mask)) {
            p_req_info->m_request.set_type((burst_type == xtsc_request::NON_AXI) ? xtsc_request::BLOCK_READ : xtsc_request::BURST_READ);
            p_req_info->m_request.set_num_transfers(reps);
            p_req_info->m_request.set_byte_enables((m_slave_byte_width == 4) ? 0xF : ((m_slave_byte_width == 8) ? 0xFF : 0xFFFF));
          }
          else {
            xtsc_byte_enables slave_mask = ((m_slave_byte_width == 4) ? 0xF : ((m_slave_byte_width == 8) ? 0xFF : 0xFFFF));
            p_req_info->m_request.set_byte_enables(be & slave_mask);
            xtsc_address address8 = p_req_info->m_request.get_byte_address();
            p_req_rsp_info->m_num_last_xfer_rsp_expected = reps;
            for (u32 i=1; i < reps; ++i) {
              m_requests[i] = copy_request_info(*p_req_info);
              be >>= m_slave_byte_width;
              address8 += m_slave_byte_width;
              m_requests[i]->m_request.set_byte_address(address8);
              m_requests[i]->m_request.set_byte_enables(be & slave_mask);
            }
          }
        }
      }
      else if (type == xtsc_request::BLOCK_READ || type == xtsc_request::BURST_READ) {
        u32 total_bytes   = master_byte_width * p_request_info->m_request.get_num_transfers();
        u32 num_transfers = total_bytes / m_slave_byte_width;
        u32 reps          = (num_transfers + max_num_xfers - 1) / max_num_xfers;
        p_req_info->m_request.set_byte_size(m_slave_byte_width);
        p_req_info->m_request.set_num_transfers(num_transfers < max_num_xfers ? num_transfers : max_num_xfers);
        p_req_info->m_request.set_byte_enables((m_slave_byte_width == 4) ? 0xF : ((m_slave_byte_width == 8) ? 0xFF : 0xFFFF));
        xtsc_address address8 = p_req_info->m_request.get_byte_address();
        confirm_alignment(total_bytes, address8, p_req_info->m_request);
        p_req_rsp_info->m_num_last_xfer_rsp_expected = reps;
        xtsc_address upper_boundary = (address8 % total_bytes == 0) ? (address8 + total_bytes) : ((address8 & -total_bytes) + total_bytes);

        for (u32 i=1; i < reps; ++i) {
          m_requests[i] = copy_request_info(*p_req_info);
          address8 = address8 & -(m_slave_byte_width * max_num_xfers);
          address8 += m_slave_byte_width * max_num_xfers;
          if (address8 >= upper_boundary)
            address8-=total_bytes;
          m_requests[i]->m_request.set_byte_address(address8);
        }

        //This information will be used to correctly reorder the memory resposnes before they are passed to the core.
        u64 tag = p_request_info->m_request.get_tag();
        if(m_issued_split_requests.find(tag) == m_issued_split_requests.end()) {
          xtsc::xtsc_address base_addr = p_req_info->m_request.get_byte_address()
                                         & ~((m_slave_byte_width * p_req_info->m_request.get_num_transfers()) - 1);
          xtsc::xtsc_address end_addr  = (base_addr + (m_slave_byte_width * (p_req_info->m_request.get_num_transfers()) - 1))
                                         & ~(master_byte_width - 1);

          u8 post_wrap_cnt = (u8)((p_req_info->m_request.get_byte_address() - base_addr) / master_byte_width);
          u8 pre_wrap_cnt  = post_wrap_cnt ? ((u8)((end_addr - p_req_info->m_request.get_byte_address()) / master_byte_width) + 1) : 0;
          split_request new_block_read = {p_req_info->m_request.get_byte_address(), pre_wrap_cnt, post_wrap_cnt};
          m_issued_split_requests.insert(pair<xtsc::u64, split_request>(tag, new_block_read));
        }
        else {
          ostringstream oss;
          oss << kind() << " '" << name() << "' PWC recieved duplicate tags in convert_request! " << p_request_info->m_request;
          throw xtsc_exception(oss.str());
        }
      }
      else if (type == xtsc_request::WRITE || (type == xtsc_request::BURST_WRITE && (p_req_info->m_request.get_num_transfers() == 1))) {
        if (size > m_slave_byte_width) {
          p_req_info->m_request.set_byte_size(m_slave_byte_width);
          xtsc_byte_enables     mask    = ((size == 8) ? 0xFF : ((size == 16) ? 0xFFFF : 0xFFFFFFFF));
          xtsc_byte_enables     be      = p_request_info->m_request.get_byte_enables();
          const u8             *p_src   = p_request_info->m_request.get_buffer();
          u32                   reps    = size / m_slave_byte_width;
          // Optionally use BLOCK_WRITE if all bytes are enabled
          if (m_use_block_requests && ((be & mask) == mask)) {
            p_req_info->m_request.set_type((burst_type == xtsc_request::NON_AXI) ? xtsc_request::BLOCK_WRITE : xtsc_request::BURST_WRITE);
            p_req_info->m_request.set_last_transfer(false);
            p_req_info->m_request.set_num_transfers(reps);
            p_req_info->m_request.set_byte_enables((m_slave_byte_width == 4) ? 0xF : ((m_slave_byte_width == 8) ? 0xFF : 0xFFFF));
            xtsc_address address8 = p_req_info->m_request.get_byte_address();
            for (u32 i=1; i < reps; ++i) {
              m_requests[i] = copy_request_info(*p_req_info);
              p_src += m_slave_byte_width;
              m_requests[i]->m_request.set_buffer(m_slave_byte_width, p_src);
              address8 += m_slave_byte_width;
              m_requests[i]->m_request.set_byte_address(address8);
            }
            m_requests[reps-1]->m_request.set_last_transfer(true);
          }
          else {
            xtsc_byte_enables slave_mask = ((m_slave_byte_width == 4) ? 0xF : ((m_slave_byte_width == 8) ? 0xFF : 0xFFFF));
            p_req_info->m_request.set_byte_enables(be & slave_mask);
            xtsc_address address8 = p_req_info->m_request.get_byte_address();
            p_req_rsp_info->m_num_last_xfer_rsp_expected = reps;
            for (u32 i=1; i < reps; ++i) {
              m_requests[i] = copy_request_info(*p_req_info);
              be >>= m_slave_byte_width;
              address8 += m_slave_byte_width;
              p_src += m_slave_byte_width;
              m_requests[i]->m_request.set_byte_address(address8);
              m_requests[i]->m_request.set_buffer(m_slave_byte_width, p_src);
              m_requests[i]->m_request.set_byte_enables(be & slave_mask);
            }
          }
        }
      }
      else if (type == xtsc_request::BLOCK_WRITE || type == xtsc_request::BURST_WRITE) {
        u32             total_bytes       = master_byte_width * p_request_info->m_request.get_num_transfers();
        u32             num_transfers     = total_bytes / m_slave_byte_width;
        u32             reps              = master_byte_width / m_slave_byte_width;
        const u8       *p_src             = p_request_info->m_request.get_buffer();
        u32             transfer_num_base = p_req_info->m_request.get_transfer_number()-1;
        p_req_info->m_request.set_byte_size(m_slave_byte_width);
        p_req_info->m_request.set_num_transfers(num_transfers < max_num_xfers ? num_transfers : max_num_xfers);
        p_req_info->m_request.set_byte_enables((m_slave_byte_width == 4) ? 0xF : ((m_slave_byte_width == 8) ? 0xFF : 0xFFFF));
        p_req_info->m_request.set_transfer_number(transfer_num_base*reps+1);
        p_req_rsp_info->m_num_last_xfer_rsp_expected = (num_transfers <= max_num_xfers ? 1 : (num_transfers / max_num_xfers));
        p_req_rsp_info->m_block_write_address = p_req_info->m_request.get_byte_address() + m_slave_byte_width;
        for (u32 i=1; i < reps; ++i) {
          m_requests[i] = copy_request_info(*p_req_info);
          p_src += m_slave_byte_width;
          m_requests[i]->m_request.set_buffer(m_slave_byte_width, p_src);
          m_requests[i]->m_request.set_byte_address(p_req_rsp_info->m_block_write_address);
          m_requests[i]->m_request.set_transfer_number(transfer_num_base*reps+i+1);
          p_req_rsp_info->m_block_write_address += m_slave_byte_width;
        }
        p_req_rsp_info->m_num_block_write_requests = reps;
      }
      else if (type == xtsc_request::RCW) {
        xtsc_byte_enables be         = p_request_info->m_request.get_byte_enables();
        xtsc_byte_enables slave_mask = ((m_slave_byte_width == 4) ? 0xF : ((m_slave_byte_width == 8) ? 0xFF : 0xFFFF));
        p_req_info->m_request.set_byte_enables(be & slave_mask);
      }
    }
    else {
      // Slave is wider than master
      // convert_request() is not called for BURST_READ and BURST_WRITE (i.e., AXI protocol) when slave is wider than master!
      if ((type == xtsc_request::READ) || (type == xtsc_request::WRITE) || (type == xtsc_request::RCW)) {
        m_requests[0] = p_req_info;
      }
      else if (type == xtsc_request::BLOCK_READ) {
        m_requests[0] = p_req_info;
        u32 total_bytes = master_byte_width * p_request_info->m_request.get_num_transfers();
        confirm_alignment(total_bytes, p_req_info->m_request.get_byte_address(), p_req_info->m_request);
        if (total_bytes <= m_slave_byte_width) {
          p_req_info->m_request.set_byte_size(total_bytes);
          p_req_info->m_request.set_type(xtsc_request::READ);
          p_req_info->m_request.set_num_transfers(1);
          p_req_info->m_request.set_byte_enables((total_bytes == 32) ? 0xFFFFFFFF : ((total_bytes == 16) ? 0xFFFF : 0xFF));
        }
        else {
          //Memory requests should be aligned to the slave_byte_width!
          p_req_info->m_request.set_byte_address((p_req_info->m_request.get_byte_address() & -m_slave_byte_width));
          p_req_info->m_request.set_byte_size(m_slave_byte_width);
          p_req_info->m_request.set_num_transfers(total_bytes / m_slave_byte_width);
          p_req_info->m_request.set_byte_enables((m_slave_byte_width == 32) ? 0xFFFFFFFF : ((m_slave_byte_width == 16) ? 0xFFFF : 0xFF));
        }
      }
      else if (type == xtsc_request::BLOCK_WRITE) {
        p_req_rsp_info->m_p_nascent_request = p_req_info;
        u32 total_bytes = master_byte_width * p_request_info->m_request.get_num_transfers();
        p_req_rsp_info->m_block_write_address = p_req_info->m_request.get_byte_address();
        if (total_bytes <= m_slave_byte_width) {
          p_req_info->m_request.set_byte_size(total_bytes);
          p_req_info->m_request.set_type(xtsc_request::WRITE);
          p_req_info->m_request.set_num_transfers(1);
          p_req_info->m_request.set_byte_enables((total_bytes == 32) ? 0xFFFFFFFF : ((total_bytes == 16) ? 0xFFFF : 0xFF));
        }
        else {
          p_req_info->m_request.set_byte_size(m_slave_byte_width);
          p_req_info->m_request.set_num_transfers(total_bytes / m_slave_byte_width);
          p_req_info->m_request.set_byte_enables((m_slave_byte_width == 32) ? 0xFFFFFFFF : ((m_slave_byte_width == 16) ? 0xFFFF : 0xFF));
        }
        p_req_rsp_info->m_num_block_write_requests = 1;
      }
    }
  }
  else {
    // Non-first transfer (BLOCK_WRITE or RCW)
    std::map<xtsc::u64, req_rsp_info*>::iterator itr = m_req_rsp_table.find(m_pending_request_tag);
    if (itr == m_req_rsp_table.end()) {
      ostringstream oss;
      oss << "Expected request tag = " << m_pending_request_tag << " was not found in m_pending_request_tag! ";
      throw xtsc_exception(oss.str());
    }
    else
      p_req_rsp_info = itr->second;

    xtsc_request::type_t original_type = p_req_rsp_info->m_p_first_request_info->m_request.get_type();
    if (type != original_type) {
      ostringstream oss;
      oss << "Expected " << xtsc_request::get_type_name(original_type) << " request but received: " << p_request_info->m_request;
      throw xtsc_exception(oss.str());
    }

    if (type == xtsc_request::RCW) {
      request_info *p_req_info = copy_request_info(*p_request_info);
      m_requests[0] = p_req_info;
    }
    else if (type == xtsc_request::BLOCK_WRITE || type == xtsc_request::BURST_WRITE) {
      u32 total_bytes = master_byte_width * p_request_info->m_request.get_num_transfers();
      u32 transfer_num_base = p_request_info->m_request.get_transfer_number()-1;
      if (master_byte_width > m_slave_byte_width) {
        // Master is wider than slave
        request_info *p_req_info = copy_request_info(*p_request_info);
        m_requests[0] = p_req_info;
        u32             num_transfers   = total_bytes / m_slave_byte_width;
        u32             reps            = master_byte_width / m_slave_byte_width;
        const u8       *p_src           = p_request_info->m_request.get_buffer();
        p_req_info->m_request.set_byte_address(p_req_rsp_info->m_block_write_address);
        p_req_rsp_info->m_block_write_address += m_slave_byte_width;
        p_req_info->m_request.set_byte_size(m_slave_byte_width);
        p_req_info->m_request.set_num_transfers(num_transfers < max_num_xfers ? num_transfers : max_num_xfers);
        p_req_info->m_request.set_byte_enables((m_slave_byte_width == 4) ? 0xF : ((m_slave_byte_width == 8) ? 0xFF : 0xFFFF));
        p_req_info->m_request.set_last_transfer(false);
        m_requests[0]->m_request.set_transfer_number(transfer_num_base*reps+1);
        for (u32 i=1; i < reps; ++i) {
          m_requests[i] = copy_request_info(*p_req_info);
          m_requests[i]->m_request.set_transfer_number(transfer_num_base*reps+i+1);
          p_src += m_slave_byte_width;
          m_requests[i]->m_request.set_buffer(m_slave_byte_width, p_src);
          m_requests[i]->m_request.set_byte_address(p_req_rsp_info->m_block_write_address);
          p_req_rsp_info->m_block_write_address += m_slave_byte_width;
        }
        p_req_rsp_info->m_num_block_write_requests += reps;
        if (p_request_info->m_request.get_last_transfer() || ((p_req_rsp_info->m_num_block_write_requests % 16) == 0 && p_request_info->m_request.is_axi_protocol() == false)) {
          m_requests[reps-1]->m_request.set_last_transfer(true);
        }
      }
      else {
        // Slave is wider than master
        request_info   *p_req_info      = p_req_rsp_info->m_p_nascent_request;
        u32             ratio           = m_slave_byte_width / master_byte_width ;
        u32             offset          = (p_req_rsp_info->m_num_block_write_requests % ratio) * master_byte_width;
        u32             next_offset     = offset + master_byte_width;
        u8             *p_dst           = p_req_info->m_request.get_buffer();
        const u8       *p_src           = p_request_info->m_request.get_buffer();
        bool            last            = p_request_info->m_request.get_last_transfer();
        memcpy(p_dst + offset, p_src, master_byte_width);
        if (last) {
          p_req_info->m_request.set_last_transfer(true);
        }
        p_req_rsp_info->m_num_block_write_requests += 1;
        if ((next_offset == m_slave_byte_width) || (next_offset == total_bytes)) {
          p_req_info->m_request.set_byte_address(p_req_rsp_info->m_block_write_address);
          p_req_rsp_info->m_block_write_address += m_slave_byte_width;
          m_requests[0] = p_req_info;
          p_req_rsp_info->m_p_nascent_request = (last ? NULL : copy_request_info(*p_req_rsp_info->m_p_nascent_request));
          m_requests[0]->m_time_stamp = p_request_info->m_time_stamp;
        }
      }
    }
    else {
      ostringstream oss;
      oss << "Program Bug: Only RCW|BLOCK_WRITE expected at line " << __LINE__ << " of " << __FILE__;
      throw xtsc_exception(oss.str());
    }

    if (p_request_info->m_request.get_last_transfer()) {
      m_pending_request_tag = 0;
    }

    // Note: When first_transfer is true p_request_info is deleted (as m_p_first_request_info) in delete_req_rsp_info()
    delete_request_info(p_request_info);
  }

}



void xtsc_component::xtsc_arbiter::confirm_alignment(u32 total_bytes, xtsc_address address8, xtsc_request &request) {
  if (address8 % request.get_byte_size()) {
    ostringstream oss;
    oss << kind() << " '" << name() << "' configured as PIF-width convertor received unaligned address: " << request;
    throw xtsc_exception(oss.str());
  }
}



u32 xtsc_component::xtsc_arbiter::get_available_route_id() {
  if (m_next_route_id == 0xFFFFFFFF) {
    ostringstream oss;
    oss << "ERROR: " << kind() << " '" << name() << "' has run out of route ID's (\"num_route_ids\"=" << m_num_route_ids << ")";
    throw xtsc_exception(oss.str());
  }
  u8 available_id = m_next_route_id;
  m_route_ids_used += 1;
  bool found = false;
  for (m_next_route_id++; m_next_route_id < available_id + m_num_route_ids; ++m_next_route_id) {
    if (m_routing_table[m_next_route_id % m_num_route_ids][1] == 0xFFFFFFFF) {
      found = true;
      m_next_route_id = m_next_route_id % m_num_route_ids;
      break;
    }
  }
  if (!found) {
    m_next_route_id = 0xFFFFFFFF;
  }
  XTSC_DEBUG(m_text, "get_available_route_id() returning " << available_id << " m_next_route_id=0x" << hex << m_next_route_id);
  return available_id;
}



// This method is used by arbiter_thread, arbiter_pwc_thread, and arbiter_special_thread; but not by arbiter_apb_thread
void xtsc_component::xtsc_arbiter::handle_request(request_info*& p_active_request_info, u32 port_num, const char *caller) {
  xtsc_request *p_request = &p_active_request_info->m_request;
  add_route_id_bits(*p_request, port_num);
  u32 tries = 0;
  bool delete_active_request_info = true;
  bool try_again = false;
  do {
    if (m_one_at_a_time) {
      // If we only support one request at a time, then RSP_NACC all other requests that came
      // while we were sleeping or waiting for the arbitration phase or waiting for nacc
      nacc_remaining_requests();
    }
    tries += 1;
    if (m_align_request_phase) {
      XTSC_VERBOSE(m_text, *p_request << " Port #" << port_num << (m_lock ? " Lock" : " ") << " Try #" << tries <<
                                         " RteID=" << p_request->get_route_id());
      if (m_phase_delay_fifo->num_free() == 0) {
        try_again = true;       // This is equivalent to align_request_phase_thread nacc'ing us
      }
      else {
        try_again = false;
        delete_active_request_info = false;
        m_phase_delay_fifo->nb_write(p_active_request_info);
        m_align_request_phase_thread_event.notify(SC_ZERO_TIME);
        m_last_request_time_stamp = sc_time_stamp();
      }
      XTSC_DEBUG(m_text, caller << " waiting for nacc wait time =" << m_nacc_wait_time);
      wait(m_nacc_wait_time);
      XTSC_DEBUG(m_text, caller << " finished nacc wait time");
    }
    else {
      m_request_got_nacc = false;
      XTSC_INFO(m_text, *p_request << " Port #" << port_num << (m_lock ? " Lock" : " ") << " Try #" << tries <<
                                      " RteID=" << p_request->get_route_id());
      xtsc_log_memory_request_event(m_binary, INFO_LOG_LEVEL, false, 0, m_log_data_binary, *p_request);
      m_waiting_for_nacc = true;
      m_request_port->nb_request(*p_request);
      if (m_immediate_timing) break;
      m_last_request_time_stamp = sc_time_stamp();
      XTSC_DEBUG(m_text, caller << " waiting for nacc wait time =" << m_nacc_wait_time);
      wait(m_nacc_wait_time);
      XTSC_DEBUG(m_text, caller << " finished nacc wait time");
      m_waiting_for_nacc = false;
      try_again = m_request_got_nacc;
    }
  } while (try_again);
  if (delete_active_request_info) {
    delete_request_info(p_active_request_info);
  }
}



void xtsc_component::xtsc_arbiter::nacc_remaining_requests() {
  for (u32 port_num=0; port_num<m_num_masters; ++port_num) {
    while (m_request_fifos[port_num]->num_available()) {
      request_info *p_request_info = m_request_deques[port_num]->front();
      m_request_deques[port_num]->pop_front();
      int dummy;
      m_request_fifos[port_num]->nb_read(dummy);
      xtsc_response response(p_request_info->m_request, xtsc_response::RSP_NACC);
      XTSC_VERBOSE(m_text, p_request_info->m_request  << " Will now get RSP_NACC");
      XTSC_VERBOSE(m_text, response << " Port #" << port_num << " (Busy)");
      xtsc_log_memory_response_event(m_binary, VERBOSE_LOG_LEVEL, false, port_num, false, response);
      (*m_respond_ports[port_num])->nb_respond(response);
      delete_request_info(p_request_info);
    }
  }
}



void xtsc_component::xtsc_arbiter::response_thread(void) {

  try {

    while (true) {
      wait(m_response_thread_event);
      XTSC_DEBUG(m_text, __FUNCTION__ << " woke up.");
      while (m_response_fifo.num_available()) {
        response_info *p_response_info;
        m_response_fifo.nb_read(p_response_info);
        u32 port_num = get_port_from_response(p_response_info->m_response);
        // Calculate delay (net => No Earlier Than time)
        sc_time receipt_net       = p_response_info->m_time_stamp + m_response_delay;
        sc_time last_response_net = m_last_response_time_stamp + (m_delay_from_receipt ? m_recovery_time : m_response_delay);
        sc_time latest_net        = (receipt_net > last_response_net) ? receipt_net : last_response_net;
        sc_time now               = sc_time_stamp();
        sc_time delay = (latest_net <= now) ? SC_ZERO_TIME : (latest_net - now);
        XTSC_DEBUG(m_text, __FUNCTION__ << "() doing wait for " << delay);
        wait(delay);
        // Forward the response
        u32 tries = 0;
        while (true) {
          tries += 1;
          XTSC_INFO(m_text, p_response_info->m_response << " Port #" << port_num << " Try #" << tries <<
                                                           " RteID=" << p_response_info->m_response.get_route_id());
          xtsc_log_memory_response_event(m_binary, INFO_LOG_LEVEL, false, port_num, m_log_data_binary, p_response_info->m_response);
          if ((*m_respond_ports[port_num])->nb_respond(p_response_info->m_response)) {
            break;
          }
          if (m_apb) {
            ostringstream oss;
            oss << "ERROR: nb_respond() call returned false which is not allowed with \"apb\" true: " << p_response_info->m_response;
            throw xtsc_exception(oss.str());
          }
          wait(m_response_repeat);
        }
        m_last_response_time_stamp = sc_time_stamp();
        delete_response_info(p_response_info);
        if (m_apb) {
          delete_request_info(m_p_apb_request_info);
          m_apb_response_done_event.notify(SC_ZERO_TIME);
        }
      }
    }
  }

  catch (const sc_unwind_exception& error) { throw; }
  catch (const exception& error) {
    ostringstream oss;
    oss << "std::exception caught in " << __FUNCTION__ << ", an SC_THREAD of " << kind() << " '" << name() << "'." << endl;
    oss << "what(): " << error.what() << endl;
    oss << __FUNCTION__ << " now rethrowing:";
    xtsc_log_multiline(m_text, log4xtensa::FATAL_LOG_LEVEL, oss.str(), 2);
    throw;
  }

}



void xtsc_component::xtsc_arbiter::response_pwc_thread(void) {
  //Only used when master_byte_width > slave_byte_width
  vector<xtsc::xtsc_response> buffered_xfers;
  //Only used when master_byte_width < slave_byte_width
  vector<xtsc::xtsc_response> buffered_first_xfer;
  u64 tag;
  u8 m_idx         = 1;
  u8 s_idx         = 1;
  u8 pre_wrap_cnt  = 0;
  u8 post_wrap_cnt = 0;
  bool last_xfer   = false;
  bool wrap_active = false;
  bool first_xfer  = true;

  try {
    while (true) {
      wait(m_response_thread_event);
      XTSC_DEBUG(m_text, __FUNCTION__ << " woke up.");

      while (m_response_fifo.num_available()) {
        response_info *p_response_info;
        m_response_fifo.nb_read(p_response_info);
        u32             port_num          = get_port_from_response(p_response_info->m_response);
        req_rsp_info   *p_req_rsp_info    = NULL;
        u32             master_byte_width = m_master_byte_widths[port_num];
        bool            fini              = true;

        if ((master_byte_width == m_slave_byte_width) || (p_response_info->m_response.is_axi_protocol() && (master_byte_width < m_slave_byte_width))) {
          m_responses[0] = p_response_info;
          fini = (p_response_info->m_response.get_last_transfer() == true);
        }
        else
          fini = convert_response(p_response_info, master_byte_width, p_req_rsp_info, last_xfer, wrap_active);

        map<xtsc::u64, split_request>::iterator itr;
        if (master_byte_width > m_slave_byte_width) {
          tag = p_response_info->m_response.get_tag();
          itr = m_issued_split_requests.find(tag);

          if (itr != m_issued_split_requests.end()) {
            pre_wrap_cnt = itr->second.pre_wrap_cnt;
            post_wrap_cnt = itr->second.post_wrap_cnt;

            if (pre_wrap_cnt + post_wrap_cnt == 0)
              wrap_active = false;
            else if (m_idx == (pre_wrap_cnt + post_wrap_cnt)) {
              last_xfer = true;
              wrap_active = true;
            }
            else {
              last_xfer = false;
              wrap_active = true;
            }
          }
        }

        //Number of data transfers is determined by number of sub-reqeusts (after width conversion) as well as channel width!
        u8 num_reps  = (m_slave_byte_width == master_byte_width ||
                       (p_response_info->m_response.is_axi_protocol() && m_slave_byte_width > master_byte_width))
                       ? 1 : p_req_rsp_info->m_num_last_xfer_rsp_expected;
        u8 num_xfers = ceil((float)(p_response_info->m_response.get_num_transfers() * m_slave_byte_width) / master_byte_width) * num_reps;
        sc_time delay;

        for (u32 i=0; (i<8) && (m_responses[i] != NULL); ++i) {

          // Calculate delay (net => No Earlier Than time)
          sc_time receipt_net       = m_responses[i]->m_time_stamp + m_response_delay;
          sc_time last_response_net = m_last_response_time_stamp + (m_delay_from_receipt ? m_recovery_time : m_response_delay);
          sc_time latest_net        = (receipt_net > last_response_net) ? receipt_net : last_response_net;
          sc_time now               = sc_time_stamp();
          delay = (latest_net <= now) ? SC_ZERO_TIME : (latest_net - now);
          XTSC_DEBUG(m_text, __FUNCTION__ << "() doing wait for " << delay);
          wait(delay);
          // Forward the response
          u32 tries = 0;
          while (true) {
            tries += 1;
            xtsc_address address = m_responses[i]->m_response.get_byte_address();

            if (master_byte_width > m_slave_byte_width &&
                itr != m_issued_split_requests.end() &&
                m_idx > pre_wrap_cnt &&
                m_idx <= (pre_wrap_cnt + post_wrap_cnt)) {
              buffered_xfers.push_back(m_responses[i]->m_response);
              m_idx++;
              break;
            }
            else if ((master_byte_width < m_slave_byte_width) &&
                     p_response_info->m_response.get_byte_size() > master_byte_width &&
                     first_xfer &&
                     (address % m_slave_byte_width) &&
                     ((address & -m_slave_byte_width) + (i * master_byte_width) < address)) {
              buffered_first_xfer.push_back(m_responses[i]->m_response);
              wrap_active = true;
              s_idx++;
              break;
            }
            else if ((*m_respond_ports[port_num])->nb_respond(m_responses[i]->m_response)) {
              XTSC_INFO(m_text, m_responses[i]->m_response << " Port #" << port_num << " Try #" << tries);
              xtsc_log_memory_response_event(m_binary, INFO_LOG_LEVEL, false, port_num, m_log_data_binary, m_responses[i]->m_response);
              first_xfer = (master_byte_width < m_slave_byte_width) ? false : true;
              (master_byte_width < m_slave_byte_width)? s_idx++ : (master_byte_width > m_slave_byte_width) ? m_idx++ : 0;
              break;
            }
            wait(m_response_repeat);
          }
          m_last_response_time_stamp = sc_time_stamp();
          delete_response_info(m_responses[i]);
        }
        if (p_req_rsp_info && fini) {
          wait(delay);
          //Once all the data trasfers are recieved (i.e., m_idx == num_xfers + 1) we should send the buffered responses!
          while (!buffered_xfers.empty() && m_idx == (num_xfers + 1)) {
            xtsc::xtsc_response buffered_rsp = buffered_xfers.front();
            buffered_xfers.erase(buffered_xfers.begin());

            u32 tries = 0;
            while (true) {
              tries++;
              if ((*m_respond_ports[port_num])->nb_respond(buffered_rsp)){
                XTSC_INFO(m_text, buffered_rsp << " Port #" << port_num << " Try #" << tries << "  - (reorder-buffer)");
                xtsc_log_memory_response_event(m_binary, INFO_LOG_LEVEL, false, port_num, m_log_data_binary, buffered_rsp);
                wait(delay);
                break;
              }
            }
          }

          if (!buffered_first_xfer.empty())
            buffered_first_xfer.back().set_last_transfer(true);

          while (!buffered_first_xfer.empty() && s_idx > buffered_first_xfer.front().get_num_transfers()) {
            xtsc::xtsc_response buffered_rsp = buffered_first_xfer.front();
            buffered_first_xfer.erase(buffered_first_xfer.begin());

            u32 tries = 0;
            while (true) {
              tries++;
              if ((*m_respond_ports[port_num])->nb_respond(buffered_rsp)){
                XTSC_INFO(m_text, buffered_rsp << " Port #" << port_num << " Try #" << tries << "  - (reorder-buffer)");
                xtsc_log_memory_response_event(m_binary, INFO_LOG_LEVEL, false, port_num, m_log_data_binary, buffered_rsp);
                wait(delay);
                break;
              }
            }
          }

          if (((master_byte_width > m_slave_byte_width) && m_idx == (num_xfers + 1)) || (buffered_xfers.empty() && fini)) {//!m_lock
            m_req_rsp_table.erase(p_response_info->m_response.get_tag());
            delete_response_info(p_response_info);
            delete_req_rsp_info(p_req_rsp_info);
            m_issued_split_requests.erase(tag);
            wrap_active = false;
            first_xfer = true;
            m_idx = 1;
            s_idx = 1;
          }
        }
      }
    }

  }
  catch (const sc_unwind_exception& error) { throw; }
  catch (const exception& error) {
    ostringstream oss;
    oss << "std::exception caught in " << __FUNCTION__ << ", an SC_THREAD of " << kind() << " '" << name() << "'." << endl;
    oss << "what(): " << error.what() << endl;
    oss << __FUNCTION__ << " now rethrowing:";
    xtsc_log_multiline(m_text, log4xtensa::FATAL_LOG_LEVEL, oss.str(), 2);
    throw;
  }

}



bool xtsc_component::xtsc_arbiter::convert_response(response_info*&     p_response_info,
                                                    u32                 master_byte_width,
                                                    req_rsp_info*&      p_req_rsp_info,
                                                    bool                last_xfer,
                                                    bool                wrap_active)
{
  u8 id = p_response_info->m_response.get_id();
  u64 tag = p_response_info->m_response.get_tag();
  std::map<xtsc::u64, req_rsp_info*>::iterator itr = m_req_rsp_table.find(p_response_info->m_response.get_tag());
  if (itr == m_req_rsp_table.end()) {
    ostringstream oss;
    oss << "Expected request tag = " << p_response_info->m_response.get_tag() << " was not found in m_pending_request_tag! ";
    throw xtsc_exception(oss.str());
  }
  else
    p_req_rsp_info = itr->second;

  bool last_transfer = p_response_info->m_response.get_last_transfer();
  bool fini = false;
  if (last_transfer) {
    p_req_rsp_info->m_num_last_xfer_rsp_received += 1;
    fini = (p_req_rsp_info->m_num_last_xfer_rsp_received == p_req_rsp_info->m_num_last_xfer_rsp_expected);
  }

  response_info *p_rsp_info = p_req_rsp_info->m_p_nascent_response ?
                              p_req_rsp_info->m_p_nascent_response :
                              new_response_info(p_req_rsp_info->m_p_first_request_info->m_request);

  if (!fini) {
    p_req_rsp_info->m_p_nascent_response = p_rsp_info;
  }

  /*
   * Handle single-response error responses
   *   RSP_ADDRESS_ERROR           single response
   *   RSP_ADDRESS_DATA_ERROR:     single response
   *   RSP_DATA_ERROR              normal number of responses
   */
  xtsc_response::status_t status = p_response_info->m_response.get_status();
  if (p_req_rsp_info->m_single_rsp_error_received ||
      ((status != xtsc_response::RSP_OK) && (status != xtsc_response::RSP_DATA_ERROR)))
  {
    if (p_req_rsp_info->m_single_rsp_error_received ||
        ((status == xtsc_response::RSP_ADDRESS_ERROR) || (status == xtsc_response::RSP_ADDRESS_DATA_ERROR)))
    {
      if (p_req_rsp_info->m_responses_sent) {
        ostringstream oss;
        oss << "Received address error response after some responses have already been sent upstream: " << p_response_info->m_response;
        throw xtsc_exception(oss.str());
      }
      if (!p_req_rsp_info->m_single_rsp_error_received) {
        p_req_rsp_info->m_single_rsp_error_received = true;
        p_rsp_info->m_response.set_status(status);
        p_rsp_info->m_response.set_last_transfer(true);
      }
      if (fini) {
        m_responses[0] = p_rsp_info;
        m_responses[0]->m_time_stamp = p_response_info->m_time_stamp;
      }
    }
    else {
      ostringstream oss;
      oss << "Received response with unsupported status: " << p_response_info->m_response;
      throw xtsc_exception(oss.str());
    }
  }
  else {
    if ((status == xtsc_response::RSP_DATA_ERROR) && (p_rsp_info->m_response.get_status() == xtsc_response::RSP_OK)) {
      p_rsp_info->m_response.set_status(xtsc_response::RSP_DATA_ERROR);
    }
    xtsc_request::type_t type = p_req_rsp_info->m_p_first_request_info->m_request.get_type();
    if ((type == xtsc_request::WRITE) || (type == xtsc_request::BLOCK_WRITE) || (type == xtsc_request::RCW) || (type == xtsc_request::BURST_WRITE)) {
      if (type == xtsc_request::RCW) {
        p_rsp_info->m_response.set_buffer(p_response_info->m_response.get_buffer());
      }
      if (fini) {
        m_responses[0] = p_rsp_info;
        m_responses[0]->m_time_stamp = p_response_info->m_time_stamp;
        m_responses[0]->m_response.set_exclusive_ok(p_response_info->m_response.get_exclusive_ok());
      }
    }
    else if ((type == xtsc_request::READ) || (type == xtsc_request::BLOCK_READ) || (type == xtsc_request::BURST_READ)) {
      const u8 *p_src = p_response_info->m_response.get_buffer();
      u32       size  = p_response_info->m_response.get_byte_size();
      if (master_byte_width > m_slave_byte_width) {
        // Master is wider than slave
        u8 *p_dst = p_rsp_info->m_response.get_buffer();
        u32 offset = (p_req_rsp_info->m_num_rsp_received * m_slave_byte_width) % master_byte_width;
        memcpy(p_dst+offset, p_src, size);
        if (fini || ((offset + size) == master_byte_width)) {
          m_responses[0] = p_rsp_info;
          m_responses[0]->m_time_stamp = p_response_info->m_time_stamp;

          //If the memory responses need to be reorderd (i.e., split requests), we need to change the last_transfer flag as well!
          u64 tag = p_response_info->m_response.get_tag();
          (m_issued_split_requests.find(tag) == m_issued_split_requests.end() || !wrap_active) ? m_responses[0]->m_response.set_last_transfer(fini)
                                                                                               : m_responses[0]->m_response.set_last_transfer(last_xfer);
        }
      }
      else {
        // Slave is wider than master
        p_rsp_info->m_response.set_buffer(p_src);
        m_responses[0] = p_rsp_info;
        if (type == xtsc_request::BLOCK_READ) {
          m_responses[0]->m_response.set_last_transfer(false);
          u32 reps = min(m_slave_byte_width, size) / master_byte_width;
          for (u32 i=1; i < reps; ++i) {
            m_responses[i] = new_response_info(p_rsp_info->m_response);
            m_responses[i]->m_response.set_buffer(p_src + master_byte_width*i);
            m_responses[i]->m_response.set_last_transfer(false);
          }
          if (fini && !wrap_active) {
            m_responses[reps-1]->m_response.set_last_transfer(true);
          }
        }
      }
    }
    else {
      ostringstream oss;
      oss << "Program Bug: " << xtsc_request::get_type_name(type) << " is not supported at line " << __LINE__ << " of " << __FILE__;
      throw xtsc_exception(oss.str());
    }
  }

  p_req_rsp_info->m_num_rsp_received += 1;
  if (m_responses[0] != NULL) {
    p_req_rsp_info->m_responses_sent     = true;
    p_req_rsp_info->m_p_nascent_response = NULL;
  }
  return fini;
}



// Note: This method may not change anything in the response except the route ID.  See mutable_response.
u32 xtsc_component::xtsc_arbiter::get_port_from_response(xtsc_response& response) {
  if (m_apb) {
    return m_p_apb_request_info->m_port_num;
  }
  u32 route_id = response.get_route_id();
  u32 port_num = 0;
  if (m_route_id_bits_shift == 0xFFFFFFFF) {
    if ((route_id >= m_num_route_ids) || (m_routing_table[route_id][0] >= m_num_masters)) {
      ostringstream oss;
      oss << kind() << " \"" << name() << "\": Invalid route ID=" << route_id << " in response: " << response;
      throw xtsc_exception(oss.str());
    }
    response.set_route_id(m_routing_table[route_id][1]);
    port_num = m_routing_table[route_id][0];
    if (response.get_last_transfer()) {
      XTSC_DEBUG(m_text, response << " Marking RteID=" << route_id << " as unused");
      m_routing_table[route_id][0] = m_num_masters;
      m_routing_table[route_id][1] = 0xFFFFFFFF;
      m_route_ids_used -= 1;
      if (m_next_route_id == 0xFFFFFFFF) {
        m_next_route_id = route_id;
      }
    }
  }
  else {
    port_num = ((route_id & m_route_id_bits_mask) >> m_route_id_bits_shift);
    if (port_num >= m_num_masters) {
      ostringstream oss;
      oss << kind() << " \"" << name() << "\": port_num out-of-range in route ID of response: " << response
          << " (Have you deconflicted the route ID bitfields using \"route_id_lsb\"?)";
      throw xtsc_exception(oss.str());
    }
    response.set_route_id(route_id & m_route_id_bits_clear); // clear our bit field before sending response back upstream
  }
  XTSC_DEBUG(m_text, response << " route_id=0x" << setfill('0') << hex << setw(xtsc_address_nibbles()) << route_id << " => Port #" << dec << port_num);
  return port_num;
}



void xtsc_component::xtsc_arbiter::add_route_id_bits(xtsc_request& request, u32 port_num) {
  u32 route_id = request.get_route_id();
  if (m_route_id_bits_shift == 0xFFFFFFFF) {
    if (request.is_axi_protocol() && request.get_exclusive()) {
      ostringstream oss;
      oss << kind() << " \"" << name() << "\": Received an AXI exclusive request (" << request
          << ") which is not supported when configured for autonomous routing (\"route_id_lsb\"=0xFFFFFFFF)";
      throw xtsc_exception(oss.str());
    }
    if (request.get_transfer_number() == 1) {
      u32 old_route_id = route_id;
      route_id = get_available_route_id();
      m_routing_table[route_id][0] = port_num;
      m_routing_table[route_id][1] = old_route_id;
      m_downstream_route_id[port_num] = route_id;
    }
    else {
      route_id = m_downstream_route_id[port_num];
    }
  }
  else {
    if (m_check_route_id_bits && (route_id & m_route_id_bits_mask)) {
      ostringstream oss;
      oss << kind() << " " << name() << " " << request << " RteID=0x" << setfill('0') << hex << setw(8) << route_id
          << " has bits already set in our bit-field: mask=0x" << setw(8) << m_route_id_bits_mask
          << " (see xtsc_arbiter_parms \"route_id_lsb\")";
      throw xtsc_exception(oss.str());
    }
    route_id &= m_route_id_bits_clear; // clear our bit field first
    route_id |= ((port_num << m_route_id_bits_shift) & m_route_id_bits_mask);
  }
  request.set_route_id(route_id);
  XTSC_DEBUG(m_text, request << " route_id=0x" << setfill('0') << hex << setw(8) << route_id);
}



xtsc_component::xtsc_arbiter::request_info *xtsc_component::xtsc_arbiter::new_request_info(u32 port_num, const xtsc_request& request) {
  if (m_request_pool.empty()) {
    request_info *p_request_info = new request_info(request, port_num);
    XTSC_DEBUG(m_text, "Creating a new request_info " << p_request_info << " for " << request);
    p_request_info->m_request.set_byte_address(translate(port_num, p_request_info->m_request.get_byte_address()));
    return p_request_info;
  }
  else {
    request_info *p_request_info = m_request_pool.back();
    XTSC_DEBUG(m_text, "Reusing request_info " << p_request_info << " for " << request);
    m_request_pool.pop_back();
    p_request_info->m_request = request;
    p_request_info->m_port_num = port_num;
    p_request_info->m_request.set_byte_address(translate(port_num, p_request_info->m_request.get_byte_address()));
    p_request_info->m_time_stamp = sc_time_stamp();
    return p_request_info;
  }
}



xtsc_component::xtsc_arbiter::request_info *xtsc_component::xtsc_arbiter::copy_request_info(const request_info& info) {
  if (m_request_pool.empty()) {
    request_info *p_request_info = new request_info(info);
    XTSC_DEBUG(m_text, "Creating a new request_info " << p_request_info << " for " << info.m_request);
    return p_request_info;
  }
  else {
    request_info *p_request_info = m_request_pool.back();
    XTSC_DEBUG(m_text, "Reusing request_info " << p_request_info << " for " << info.m_request);
    m_request_pool.pop_back();
    *p_request_info = info;
    return p_request_info;
  }
}



void xtsc_component::xtsc_arbiter::delete_request_info(request_info*& p_request_info) {
  XTSC_DEBUG(m_text, "Recycling request_info " << p_request_info);
  m_request_pool.push_back(p_request_info);
  p_request_info = 0;
}



xtsc_component::xtsc_arbiter::response_info *xtsc_component::xtsc_arbiter::new_response_info(const xtsc_response& response) {
  if (m_response_pool.empty()) {
    response_info *p_response_info = new response_info(response);
    XTSC_DEBUG(m_text, "Creating a new response_info " << p_response_info << " for " << response);
    return p_response_info;
  }
  else {
    response_info *p_response_info = m_response_pool.back();
    XTSC_DEBUG(m_text, "Reusing response_info " << p_response_info << " for " << response);
    m_response_pool.pop_back();
    p_response_info->m_response = response;
    p_response_info->m_response.set_exclusive_ok(response.get_exclusive_ok());
    p_response_info->m_time_stamp = sc_time_stamp();
    return p_response_info;
  }
}



xtsc_component::xtsc_arbiter::response_info *xtsc_component::xtsc_arbiter::new_response_info(const xtsc_request& request) {
  if (m_response_pool.empty()) {
    response_info *p_response_info = new response_info(request);
    XTSC_DEBUG(m_text, "Creating a new response_info " << p_response_info << " for " << request);
    return p_response_info;
  }
  else {
    response_info *p_response_info = m_response_pool.back();
    XTSC_DEBUG(m_text, "Reusing response_info " << p_response_info << " for " << request);
    m_response_pool.pop_back();
    p_response_info->m_response = request;
    p_response_info->m_response.set_exclusive_ok(request.get_exclusive());
    p_response_info->m_time_stamp = sc_time_stamp();
    return p_response_info;
  }
}



void xtsc_component::xtsc_arbiter::delete_response_info(response_info*& p_response_info) {
  XTSC_DEBUG(m_text, "Recycling response_info " << p_response_info);
  m_response_pool.push_back(p_response_info);
  p_response_info = 0;
}



xtsc_address xtsc_component::xtsc_arbiter::translate(u32 port_num, xtsc_address address8) {
  if (!m_do_translation) return address8;
  assert(port_num < m_num_masters);
  xtsc_address new_address8 = address8;
  vector<xtsc_address_range_entry*> *p_translation_table = m_translation_tables[port_num];
  vector<xtsc_address_range_entry*>::iterator itt = p_translation_table->begin();
  for (; itt != p_translation_table->end(); ++itt) {
    if ((*itt)->m_start_address8 > address8) break;
    if (((*itt)->m_start_address8 <= address8) && ((*itt)->m_end_address8 >= address8)) {
      new_address8 += (*itt)->m_delta;
      break;
    }
  }
  XTSC_DEBUG(m_text, "translate(" << port_num << ", 0x" << hex << setw(xtsc_address_nibbles()) << address8 << ") = 0x" <<
                     setw(xtsc_address_nibbles()) << new_address8);
  return new_address8;
}



void xtsc_component::xtsc_arbiter::xtsc_request_if_impl::nb_request(const xtsc_request& request) {
  XTSC_DEBUG(m_arbiter.m_text, "nb_request fifo on port #" << m_port_num << " has " <<
                               m_arbiter.m_request_fifos[m_port_num]->num_free() << " free.");
  XTSC_VERBOSE(m_arbiter.m_text, request << " Port # " << m_port_num << " RteID=" << request.get_route_id());
  xtsc_log_memory_request_event(m_arbiter.m_binary, VERBOSE_LOG_LEVEL, true, m_port_num, m_arbiter.m_log_data_binary, request);
  if (request.is_apb_protocol() != m_arbiter.m_apb) {
    ostringstream oss;
    oss << "xtsc_arbiter '" << m_arbiter.name() << "' received " << (request.is_apb_protocol() ? "APB" : "non-APB")
        << " request when configured with \"apb\" " << boolalpha << m_arbiter.m_apb << ": " << request << " Port #" << m_port_num;
    throw xtsc_exception(oss.str());
  }
  xtsc_request::type_t type = request.get_type();
  if (m_arbiter.m_apb) {
    if (!request.get_last_transfer() || ((type != xtsc_request::READ) && (type != xtsc_request::WRITE))) {
      ostringstream oss;
      oss << "xtsc_arbiter '" << m_arbiter.name() << "' received illegal/malformed request when configured with \"apb\" true: " << request;
      throw xtsc_exception(oss.str());
    }
    if (m_arbiter.m_request_deques[m_port_num]->size() ||
       (m_arbiter.m_p_apb_request_info && (m_arbiter.m_p_apb_request_info->m_port_num == m_port_num)))
    {
      ostringstream oss;
      oss << "xtsc_arbiter '" << m_arbiter.name() << "' received 2nd request on Port #" << m_port_num
          << " while 1st request is still outstanding when configured with \"apb\" true:" << endl;
      if (m_arbiter.m_request_deques[m_port_num]->size()) {
        oss << " 1st request: " << m_arbiter.m_request_deques[m_port_num]->front()->m_request << endl;
      }
      else {
        oss << " 1st request: " << m_arbiter.m_p_apb_request_info->m_request << endl;
      }
      oss << " 2nd request: " << request;
      throw xtsc_exception(oss.str());
    }
  }
  if (m_arbiter.m_read_only) {
    if ((type == xtsc_request::WRITE)       ||
        (type == xtsc_request::BLOCK_WRITE) ||
        (type == xtsc_request::BURST_WRITE) ||
        (type == xtsc_request::RCW))
    {
      ostringstream oss;
      oss << "read_only xtsc_arbiter '" << m_arbiter.name() << "' received request: " << request;
      throw xtsc_exception(oss.str());
    }
  }
  if (m_arbiter.m_write_only) {
    if ((type == xtsc_request::READ)       ||
        (type == xtsc_request::BLOCK_READ) ||
        (type == xtsc_request::BURST_READ) ||
        (type == xtsc_request::RCW))
    {
      ostringstream oss;
      oss << "write_only xtsc_arbiter '" << m_arbiter.name() << "' received request: " << request; 
      throw xtsc_exception(oss.str());
    }
  }
  if (m_arbiter.m_dram_lock) {
    xtsc_request::type_t type = request.get_type();
    if ((type == xtsc_request::BLOCK_WRITE) ||
        (type == xtsc_request::BURST_WRITE) ||
        (type == xtsc_request::BLOCK_READ)  ||
        (type == xtsc_request::BURST_READ)  ||
        (type == xtsc_request::RCW))
    {
      ostringstream oss;
      oss << "xtsc_arbiter '" << m_arbiter.name() << "' has \"dram_lock\" set true but received non-DataRAM request type: " << request; 
      throw xtsc_exception(oss.str());
    }
  }
  if (!m_arbiter.m_apb && (m_arbiter.m_route_ids_used == m_arbiter.m_num_route_ids)) {
    xtsc_response response(request, xtsc_response::RSP_NACC);
    XTSC_VERBOSE(m_arbiter.m_text, response << " Port #" << m_port_num << " (No available route ID's)");
    xtsc_log_memory_response_event(m_arbiter.m_binary, VERBOSE_LOG_LEVEL, false, m_port_num, false, response);
    (*m_arbiter.m_respond_ports[m_port_num])->nb_respond(response);
    return;
  }
  // Check if we should RSP_NACC this request for testing purposes
  if (m_arbiter.m_fail_port_mask                       &&
      ((1 << m_port_num) & m_arbiter.m_fail_port_mask) &&
      (m_arbiter.random() >= m_arbiter.m_let_through))
  {
    xtsc_response response(request, xtsc_response::RSP_NACC);
    XTSC_INFO(m_arbiter.m_text, response << " (Test RSP_NACC Port #" << m_port_num << ")");
    xtsc_log_memory_response_event(m_arbiter.m_binary, INFO_LOG_LEVEL, false, 0, false, response);
    (*m_arbiter.m_respond_ports[m_port_num])->nb_respond(response);
    return;
  }
  if (m_arbiter.m_immediate_timing) { 
    m_arbiter.do_request_immediate_timing(m_port_num, request);
  }
  else {
    m_arbiter.do_request(m_port_num, request);
  }
}



void xtsc_component::xtsc_arbiter::do_request_immediate_timing(u32 port_num, const xtsc_request& request) {
  // Check for lock on a different port
  if (m_lock && (port_num != m_token)) {
    xtsc_response response(request, xtsc_response::RSP_NACC);
    XTSC_VERBOSE(m_text, response << " Port #" << port_num << " (Arbiter locked to port #" << m_token << ")");
    xtsc_log_memory_response_event(m_binary, VERBOSE_LOG_LEVEL, false, port_num, false, response);
    (*m_respond_ports[port_num])->nb_respond(response);
    return;
  }
  // Create our copy of the request
  request_info *p_request_info = new_request_info(port_num, request);
  // Accept this one as our current request
  m_token = port_num;
  m_lock = !p_request_info->m_request.get_last_transfer();
  handle_request(p_request_info, port_num, __FUNCTION__);
}



void xtsc_component::xtsc_arbiter::do_request(u32 port_num, const xtsc_request& request) {
  // Check if we've got room for this request
  if (!m_request_fifos[port_num]->num_free()) {
    xtsc_response response(request, xtsc_response::RSP_NACC);
    XTSC_VERBOSE(m_text, response << " Port #" << port_num << " (Request fifo full)");
    xtsc_log_memory_response_event(m_binary, VERBOSE_LOG_LEVEL, false, port_num, false, response);
    (*m_respond_ports[port_num])->nb_respond(response);
    return;
  }
  // If we only support one request at a time, RSP_NACC while we are waiting for nacc
  if (m_one_at_a_time && m_waiting_for_nacc) {
    xtsc_response response(request, xtsc_response::RSP_NACC);
    XTSC_VERBOSE(m_text, response << " Port #" << port_num << " (Busy)");
    xtsc_log_memory_response_event(m_binary, VERBOSE_LOG_LEVEL, false, port_num, false, response);
    (*m_respond_ports[port_num])->nb_respond(response);
    return;
  }
  if (m_profile_buffers) {
    u32 fifo_size = (u32) m_request_fifos[port_num]->num_available();
    if (fifo_size >= m_max_num_requests[port_num]) {
      m_max_num_requests[port_num] = fifo_size + 1;
      m_max_num_requests_timestamp[port_num] = sc_time_stamp();
      m_max_num_requests_tag[port_num] = request.get_tag();
    }
  }
  // Create our copy of the request
  request_info *p_request_info = new_request_info(port_num, request);
  // Add to request fifo
  XTSC_DEBUG(m_text, request << " Port #" << port_num << " (nb_request: Added to request fifo)");
  m_request_deques[port_num]->push_back(p_request_info);
  m_request_fifos[port_num]->nb_write(0);
  m_arbiter_thread_event.notify(SC_ZERO_TIME);
}



xtsc_component::xtsc_arbiter::req_rsp_info *xtsc_component::xtsc_arbiter::new_req_rsp_info(request_info *p_first_request_info) {
  req_rsp_info *p_req_rsp_info;
  if (m_req_rsp_info_pool.empty()) {
    p_req_rsp_info = new req_rsp_info();
    XTSC_DEBUG(m_text, "Creating a new req_rsp_info " << p_req_rsp_info);
  }
  else {
    p_req_rsp_info = m_req_rsp_info_pool.back();
    XTSC_DEBUG(m_text, "Reusing req_rsp_info " << p_req_rsp_info);
    m_req_rsp_info_pool.pop_back();
  }
  p_req_rsp_info->m_p_first_request_info = p_first_request_info;
  return p_req_rsp_info;
}



void xtsc_component::xtsc_arbiter::delete_req_rsp_info(req_rsp_info*& p_req_rsp_info) {
  XTSC_DEBUG(m_text, "Recycling req_rsp_info " << p_req_rsp_info);
  delete_request_info(p_req_rsp_info->m_p_first_request_info);
  memset(p_req_rsp_info, 0, sizeof(req_rsp_info));
  m_req_rsp_info_pool.push_back(p_req_rsp_info);
  p_req_rsp_info = 0;
}



void xtsc_component::xtsc_arbiter::end_of_simulation() {
  if (m_profile_buffers) {
    ostringstream oss;
    dump_profile_results(oss);
    string log_str  = oss.str();
    log_str = log_str.substr(0, log_str.length()-1);
    xtsc_log_multiline(m_text, INFO_LOG_LEVEL, log_str);    
  }
}



void xtsc_component::xtsc_arbiter::xtsc_request_if_impl::nb_peek(xtsc_address address8, u32 size8, u8 *buffer) {
  xtsc_address original_addr = address8;
  m_arbiter.m_request_port->nb_peek(m_arbiter.translate(m_port_num, address8), size8, buffer);
  if (m_arbiter.m_log_peek_poke && xtsc_is_text_logging_enabled() && m_arbiter.m_text.isEnabledFor(VERBOSE_LOG_LEVEL)) {
    u8 *buf = buffer;
    u32 buf_offset = 0;
    ostringstream oss;
    oss << hex << setfill('0');
    for (u32 i = 0; i<size8; ++i) {
      oss << setw(2) << (u32) buf[buf_offset] << " ";
      buf_offset += 1;
    }
    XTSC_VERBOSE(m_arbiter.m_text, "peek: " << " [0x" << hex << original_addr << "/" << dec << size8 << "] = " << oss.str() <<
                                   " Port #" << m_port_num);
  }
}



void xtsc_component::xtsc_arbiter::xtsc_request_if_impl::nb_poke(xtsc_address address8, u32 size8, const u8 *buffer) {
  xtsc_address original_addr = address8;
  if (m_arbiter.m_log_peek_poke && xtsc_is_text_logging_enabled() && m_arbiter.m_text.isEnabledFor(VERBOSE_LOG_LEVEL)) {
    const u8 *buf = buffer;
    u32 buf_offset = 0;
    ostringstream oss;
    oss << hex << setfill('0');
    for (u32 i = 0; i<size8; ++i) {
      oss << setw(2) << (u32) buf[buf_offset] << " ";
      buf_offset += 1;
    }
    XTSC_VERBOSE(m_arbiter.m_text, "poke: " << " [0x" << hex << original_addr << "/" << dec << size8 << "] = " << oss.str() <<
                                   " Port #" << m_port_num);
  }
  m_arbiter.m_request_port->nb_poke(m_arbiter.translate(m_port_num, address8), size8, buffer);
}



bool xtsc_component::xtsc_arbiter::xtsc_request_if_impl::nb_peek_coherent(xtsc_address  virtual_address8,
                                                                          xtsc_address  physical_address8,
                                                                          u32           size8,
                                                                          u8           *buffer)
{
  xtsc_address original_addr = physical_address8;
  xtsc_address address8 = m_arbiter.translate(m_port_num, physical_address8);
  bool return_value = m_arbiter.m_request_port->nb_peek_coherent(virtual_address8, address8, size8, buffer);
  if (m_arbiter.m_log_peek_poke && xtsc_is_text_logging_enabled() && m_arbiter.m_text.isEnabledFor(VERBOSE_LOG_LEVEL)) {
    u8 *buf = buffer;
    u32 buf_offset = 0;
    ostringstream oss;
    oss << hex << setfill('0');
    for (u32 i = 0; i<size8; ++i) {
      oss << setw(2) << (u32) buf[buf_offset] << " ";
      buf_offset += 1;
    }
    XTSC_VERBOSE(m_arbiter.m_text, "cpeek:" << " [0x" << hex << original_addr << "/" << dec << size8 << "] = " << oss.str() <<
                                   " Port #" << m_port_num);
  }
  return return_value;
}



bool xtsc_component::xtsc_arbiter::xtsc_request_if_impl::nb_poke_coherent(xtsc_address  virtual_address8,
                                                                          xtsc_address  physical_address8,
                                                                          u32           size8,
                                                                          const u8     *buffer)
{
  xtsc_address original_addr = physical_address8;
  xtsc_address address8 = m_arbiter.translate(m_port_num, physical_address8);
  if (m_arbiter.m_log_peek_poke && xtsc_is_text_logging_enabled() && m_arbiter.m_text.isEnabledFor(VERBOSE_LOG_LEVEL)) {
    const u8 *buf = buffer;
    u32 buf_offset = 0;
    ostringstream oss;
    oss << hex << setfill('0');
    for (u32 i = 0; i<size8; ++i) {
      oss << setw(2) << (u32) buf[buf_offset] << " ";
      buf_offset += 1;
    }
    XTSC_VERBOSE(m_arbiter.m_text, "cpoke:" << " [0x" << hex << original_addr << "/" << dec << size8 << "] = " << oss.str() <<
                                   " Port #" << m_port_num);
  }
  return m_arbiter.m_request_port->nb_poke_coherent(virtual_address8, address8, size8, buffer);
}



bool xtsc_component::xtsc_arbiter::xtsc_request_if_impl::nb_fast_access(xtsc_fast_access_request &request) {
  xtsc_address orig_address8 = request.get_translated_request_address();
  xtsc_address address8 = m_arbiter.translate(m_port_num, orig_address8);
  request.translate_request_address(address8);

  if (!m_arbiter.m_request_port->nb_fast_access(request)) {
    return false;
  }

  /* remove anything mapped before this address hits. When the address
     hits, remove anything outside of its range */
  if (m_arbiter.m_do_translation) {
    const vector<xtsc_address_range_entry*> *p_translation_table 
      = m_arbiter.m_translation_tables[m_port_num];
    for (unsigned i = 0; i < p_translation_table->size(); ++i) {
      xtsc_address end_address8 = (*p_translation_table)[i]->m_end_address8;
      xtsc_address start_address8 = (*p_translation_table)[i]->m_start_address8;
      if (orig_address8 >= start_address8 && orig_address8 <= end_address8) {
        xtsc_fast_access_block min_block(orig_address8, start_address8, end_address8);
        if (!request.restrict_to_block(min_block)) {
          return false;
        }
        break;
      }
      if (!request.remove_address_range(orig_address8, start_address8, end_address8)) {
        return false;
      }
    }
  }
  
  return true;
}



void xtsc_component::xtsc_arbiter::xtsc_request_if_impl::nb_load_retired(xtsc_address address8) {
  address8 = m_arbiter.translate(m_port_num, address8);
  m_arbiter.m_request_port->nb_load_retired(address8);
}



void xtsc_component::xtsc_arbiter::xtsc_request_if_impl::nb_retire_flush() {
  m_arbiter.m_request_port->nb_retire_flush();
}



void xtsc_component::xtsc_arbiter::xtsc_request_if_impl::nb_lock(bool lock) {
  XTSC_INFO(m_arbiter.m_text, "nb_lock(" << boolalpha << lock << ") called on Port #" << m_port_num);
  if (m_arbiter.m_dram_lock) {
    bool transitioned_from_locked_to_unlocked = false;
    if (m_arbiter.m_use_lock_port_groups) {
      vector<u32>& lock_port_group = *m_arbiter.m_lock_port_group_table[m_port_num];
      for (vector<u32>::const_iterator i = lock_port_group.begin(); i != lock_port_group.end(); ++i) {
        m_arbiter.m_dram_locks[*i] = lock;
        if (!lock && (*i == m_arbiter.m_token)) {
          if (m_arbiter.m_lock) {
            transitioned_from_locked_to_unlocked = true;
          }
          m_arbiter.m_lock = false;
        }
      }
    }
    else {
      m_arbiter.m_dram_locks[m_port_num] = lock;
      if (!lock && (m_port_num == m_arbiter.m_token)) {
        if (m_arbiter.m_lock) {
          transitioned_from_locked_to_unlocked = true;
        }
        m_arbiter.m_lock = false;
      }
    }
    if (transitioned_from_locked_to_unlocked) {
      m_arbiter.m_request_port->nb_lock(false);
    }
  }
  else {
    m_arbiter.m_request_port->nb_lock(lock);
  }
}



void xtsc_component::xtsc_arbiter::xtsc_request_if_impl::register_port(sc_port_base& port, const char *if_typename) {
  if (m_p_port) {
    ostringstream oss;
    oss << "Illegal multiple binding detected to xtsc_arbiter '" << m_arbiter.name() << "' m_request_exports[" << m_port_num
        << "]: " << endl;
    oss << "  " << port.name() << endl;
    oss << "  " << m_p_port->name();
    throw xtsc_exception(oss.str());
  }
  XTSC_INFO(m_arbiter.m_text, "Binding '" << port.name() << "' to xtsc_arbiter::m_request_exports[" << m_port_num << "]");
  m_p_port = &port;
}



bool xtsc_component::xtsc_arbiter::xtsc_respond_if_impl::nb_respond(const xtsc_response& response) {
  XTSC_VERBOSE(m_arbiter.m_text, response << " RteID=" << response.get_route_id());
  xtsc_log_memory_response_event(m_arbiter.m_binary, VERBOSE_LOG_LEVEL, true, 0, m_arbiter.m_log_data_binary, response);
  if (m_arbiter.m_immediate_timing) {
    xtsc_response &mutable_response = const_cast<xtsc_response&>(response);
    u32 save_route_id = response.get_route_id();
    u32 port_num = m_arbiter.get_port_from_response(mutable_response);
    bool result = (*m_arbiter.m_respond_ports[port_num])->nb_respond(response);
    mutable_response.set_route_id(save_route_id);
    return result;
  }
  else {
    xtsc_response::status_t status = response.get_status();
    if (m_arbiter.m_apb) {
      if (!response.get_last_transfer() || ((status != xtsc_response::APB_OK) && (status != xtsc_response::SLVERR))) {
        ostringstream oss;
        oss << "xtsc_arbiter '" << m_arbiter.name() << "' received illegal/malformed response when configured with \"apb\" true: " << response;
        throw xtsc_exception(oss.str());
      }
    }
    else if (status == xtsc_response::RSP_NACC) {
      if (m_arbiter.m_waiting_for_nacc) {
        m_arbiter.m_request_got_nacc = true;
        return true;
      }
      else {
        ostringstream oss;
        oss << "xtsc_arbiter '" << m_arbiter.name() << "' received nacc too late: " << response << endl;
        oss << " - Possibly something is wrong with the downstream device" << endl;
        oss << " - If there are multiple arbiters, consider the Warning under \"arbitration_phase\" in xtsc_arbiter_parms.h" << endl;
        oss << " - Possibly this arbiter's \"nacc_wait_time\" needs to be adjusted";
        throw xtsc_exception(oss.str());
      }
    }
    if (!m_arbiter.m_response_fifo.num_free()) {
      if (m_arbiter.m_apb) {
        ostringstream oss;
        oss << "xtsc_arbiter '" << m_arbiter.name() << "' received response with full response fifo when configured with \"apb\" true: " << response;
        throw xtsc_exception(oss.str());
      }
      else {
        XTSC_VERBOSE(m_arbiter.m_text, response << " (Rejected: Response fifo full)");
        return false;
      }
    }
    if (m_arbiter.m_profile_buffers) {
      u32 fifo_size = (u32) m_arbiter.m_response_fifo.num_available();
      if (fifo_size >= m_arbiter.m_max_num_responses) {
        m_arbiter.m_max_num_responses = fifo_size + 1;
        m_arbiter.m_max_num_responses_timestamp = sc_time_stamp();
        m_arbiter.m_max_num_responses_tag = response.get_tag();
      }
    }
    response_info *p_response_info = m_arbiter.new_response_info(response);
    m_arbiter.m_response_fifo.nb_write(p_response_info);
    m_arbiter.m_response_thread_event.notify(SC_ZERO_TIME);
    return true;
  }
}



void xtsc_component::xtsc_arbiter::xtsc_respond_if_impl::register_port(sc_port_base& port, const char *if_typename) {
  if (m_p_port) {
    ostringstream oss;
    oss << "Illegal multiple binding detected to xtsc_arbiter '" << m_arbiter.name() << "' m_respond_export: " << endl;
    oss << "  " << port.name() << endl;
    oss << "  " << m_p_port->name();
    throw xtsc_exception(oss.str());
  }
  XTSC_INFO(m_arbiter.m_text, "Binding '" << port.name() << "' to xtsc_arbiter::m_respond_export");
  m_p_port = &port;
}



