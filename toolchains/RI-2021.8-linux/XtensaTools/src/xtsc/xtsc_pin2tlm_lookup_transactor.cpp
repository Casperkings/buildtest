// Copyright (c) 2006-2014 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Cadence Design Systems, Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Cadence Design Systems, Inc.


#include <cctype>
#include <algorithm>
#include <xtsc/xtsc_pin2tlm_lookup_transactor.h>


/*
 * TODO: Test with an interface that does not have a ready signal
 * TODO: Test with various values of latency
 * See TODO's below
 */

/*
 * Theory of Operation
 *
 * The request_thread wakes up once each m_clock_period starting at time: m_sample_phase + m_posedge_offset
 * When request_thread wakes up if the pin-level request (m_req) is asserted, then the pin-level address (m_address)
 * is read and sent to the downstream Xtensa TLM lookup via a call to nb_send_address().  Also the m_drive_data event
 * queue is notified for time m_latency-m_clock_period in the future so that drive_data_thread will wake up then.
 *
 * When drive_data_thread wakes up, it gets the data via a call to the downstream Xtensa TLM lookup using nb_get_data(),
 * and then drives that data onto the pin-level interface m_data.
 *
 * The transactor supports a TIE lookup interface that has a ready signal; however, the downstream Xtensa TLM lookup must
 * always return true to the nb_is_ready() call.
 * 
 */


using namespace std;
using namespace sc_core;
using namespace sc_dt;
using namespace xtsc;


xtsc_component::xtsc_pin2tlm_lookup_transactor::xtsc_pin2tlm_lookup_transactor(sc_module_name module_name,
                                                                               const xtsc_pin2tlm_lookup_transactor_parms& parms) :
  sc_module             (module_name),
  xtsc_module           (*(sc_module*)this),
  m_address             ("m_address"),
  m_req                 ("m_req"),
  m_data                ("m_data"),
  m_ready               ("m_ready"),
  m_lookup              ("m_lookup"),
  m_text                (log4xtensa::TextLogger::getInstance(name())),
  m_has_ready           (parms.get_bool("has_ready")),
  m_address_width1      (parms.get_non_zero_u32("address_bit_width")),
  m_data_width1         (parms.get_non_zero_u32("data_bit_width")),
  m_zero_bv             (1),
  m_one_bv              (1),
  m_drive_data          ("m_drive_data"),
  m_ready_floating      ("m_ready_floating", 1, m_text)
{

  m_zero_bv     = 0;
  m_one_bv      = 1;

  // Get clock period 
  m_time_resolution = sc_get_time_resolution();
  u32 clock_period = parms.get_non_zero_u32("clock_period");
  if (clock_period == 0xFFFFFFFF) {
    m_clock_period = xtsc_get_system_clock_period();
  }
  else {
    m_clock_period = sc_get_time_resolution() * clock_period;
  }
  m_clock_period_value = m_clock_period.value();
  u32 posedge_offset = parms.get_u32("posedge_offset");
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
  m_has_posedge_offset = (m_posedge_offset != SC_ZERO_TIME);

  // Get clock phase when the ready signal is to be sampled
  u32 sample_phase = parms.get_u32("sample_phase");
  if (sample_phase < m_clock_period_value) {
    m_sample_phase = sc_get_time_resolution() * sample_phase;
  }
  else if (sample_phase = 0xFFFFFFFF) {
    m_sample_phase = sc_get_time_resolution() * (m_clock_period_value - 1);
  }
  else {
    ostringstream oss;
    oss << "xtsc_pin2tlm_lookup_transactor '" << name() << "' parameter error:" << endl;
    oss << "  \"sample_phase\" (" << sample_phase
        << ") must be strictly less than \"clock_period\" (";
    if (clock_period == 0xFFFFFFFF) {
      oss << "0xFFFFFFFF => ";
    }
    oss << m_clock_period_value << ")";
    throw xtsc_exception(oss.str());
  }

  // Get latency
  u32 latency = parms.get_u32("latency");
  if (latency < 1) {
    ostringstream oss;
    oss << "xtsc_pin2tlm_lookup_transactor '" << name() << "' parameter error:" << endl;
    oss << "  \"latency\" cannot be 0.";
    throw xtsc_exception(oss.str());
  }
  m_latency = m_clock_period * latency;

  SC_THREAD(request_thread);    m_process_handles.push_back(sc_get_current_process_handle());
  SC_THREAD(drive_data_thread); m_process_handles.push_back(sc_get_current_process_handle());

  m_port_types["m_req"]     = WIDE_INPUT;
  m_port_types["m_address"] = WIDE_INPUT;
  m_port_types["m_data"]    = WIDE_OUTPUT;
  if (m_has_ready) {
  m_port_types["m_ready"]   = WIDE_OUTPUT;
  }
  else {
    m_ready(m_ready_floating);
  }
  m_port_types["m_lookup"]  = LOOKUP_PORT;
  m_port_types["lookup"]    = PORT_TABLE;

  m_p_trace_file = static_cast<sc_trace_file*>(const_cast<void*>(parms.get_void_pointer("vcd_handle")));
  if (m_p_trace_file) {
    sc_trace(m_p_trace_file, m_address, m_address .name());
    sc_trace(m_p_trace_file, m_req,     m_req     .name());
    sc_trace(m_p_trace_file, m_data,    m_data    .name());
    if (m_has_ready) {
    sc_trace(m_p_trace_file, m_ready,   m_ready   .name());
    }
  }

  log4xtensa::LogLevel ll = xtsc_get_constructor_log_level();
  XTSC_LOG(m_text, ll, "Constructed xtsc_pin2tlm_lookup_transactor '" << name() << "':");
  XTSC_LOG(m_text, ll, " address_bit_width       = "   << m_address_width1);
  XTSC_LOG(m_text, ll, " data_bit_width          = "   << m_data_width1);
  XTSC_LOG(m_text, ll, " has_ready               = "   << boolalpha << m_has_ready);
  XTSC_LOG(m_text, ll, " latency                 = "   << latency << " (" << m_latency << ")");
  XTSC_LOG(m_text, ll, " vcd_handle              = "   << m_p_trace_file);
  if (clock_period == 0xFFFFFFFF) {
  XTSC_LOG(m_text, ll, " clock_period            = 0xFFFFFFFF => " << m_clock_period.value() << " (" << m_clock_period << ")");
  } else {
  XTSC_LOG(m_text, ll, " clock_period            = "   << clock_period << " (" << m_clock_period << ")");
  }
  if (posedge_offset == 0xFFFFFFFF) {
  XTSC_LOG(m_text, ll, " posedge_offset          = 0xFFFFFFFF => " << m_posedge_offset.value() << " (" << m_posedge_offset << ")");
  } else {
  XTSC_LOG(m_text, ll, " posedge_offset          = "   << posedge_offset << " (" << m_posedge_offset << ")");
  }
  if (sample_phase == 0xFFFFFFFF) {
  XTSC_LOG(m_text, ll, " sample_phase            = 0xFFFFFFFF => " << m_sample_phase.value() << " (" << m_sample_phase << ")");
  } else {
  XTSC_LOG(m_text, ll, " sample_phase            = "   << sample_phase << " (" << m_sample_phase << ")");
  }

  reset();

}



xtsc_component::xtsc_pin2tlm_lookup_transactor::~xtsc_pin2tlm_lookup_transactor(void) {
}



u32 xtsc_component::xtsc_pin2tlm_lookup_transactor::get_bit_width(const string& port_name, u32 interface_num) const {
  if (port_name == "m_req")     return 1;
  if (port_name == "m_address") return m_address_width1;
  if (port_name == "m_data")    return m_data_width1;
  if (m_has_ready) {
  if (port_name == "m_ready")   return 1;
  }
  if (port_name == "m_lookup")  return ((interface_num == 0) ?  m_data_width1 : m_address_width1);
  ostringstream oss;
  oss << kind() << " \"" << name() << "\" has no port named \"" << port_name << "\"" << endl;
  throw xtsc_exception(oss.str());
}



sc_object *xtsc_component::xtsc_pin2tlm_lookup_transactor::get_port(const string& port_name) {
  if (port_name == "m_req")     return &m_req;
  if (port_name == "m_address") return &m_address;
  if (port_name == "m_data")    return &m_data;
  if (m_has_ready) {
  if (port_name == "m_ready")   return &m_ready;
  }
  if (port_name == "m_lookup")  return &m_lookup;
  ostringstream oss;
  oss << kind() << " \"" << name() << "\" has no port named \"" << port_name << "\"" << endl;
  throw xtsc_exception(oss.str());
}



xtsc_port_table xtsc_component::xtsc_pin2tlm_lookup_transactor::get_port_table(const string& port_table_name) const {
  xtsc_port_table table;

  if (port_table_name == "lookup") {
    table.push_back("m_req");
    table.push_back("m_address");
    table.push_back("m_data");
    if (m_has_ready) {
    table.push_back("m_ready");
    }
  }
  else {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\" has no port table named \"" << port_table_name << "\"" << endl;
    throw xtsc_exception(oss.str());
  }

  return table;
}



void xtsc_component::xtsc_pin2tlm_lookup_transactor::reset(bool /*hard_reset*/) {
  XTSC_INFO(m_text, "xtsc_pin2tlm_lookup_transactor::reset()");

  xtsc_reset_processes(m_process_handles);
}



// TODO: Is it possible to support nb_is_ready() returning false?
// Request thread: samples m_req and m_address then calls nb_send_address() and nb_is_ready() and then notifies drive_data_thread.
void xtsc_component::xtsc_pin2tlm_lookup_transactor::request_thread(void) {

  try {

    if (m_has_ready) {
      m_ready->write(m_one_bv);
    }

    wait(m_sample_phase + m_posedge_offset);
//  sc_bv_base address((int)m_address_width1);
    sc_unsigned address(m_address_width1);

    while (true) {
      if (m_req.read() != m_zero_bv) {
        address = m_address.read();
        m_lookup->nb_send_address(address);
        if (m_has_ready) {
          bool ready = m_lookup->nb_is_ready();
          if (!ready) {
            ostringstream oss;
            oss << kind() << " '" << name() << "': Xtensa TLM lookup is not ready.  This transactor does not support that situation.";
            throw xtsc_exception(oss.str());
          }
        }
        m_drive_data.notify(m_latency-m_clock_period);
        XTSC_INFO(m_text, "Lookup: addr = 0x" << address.to_string(SC_HEX).substr(m_address_width1%4 ? 2 : 3));
      }
      wait(m_clock_period);
    }

  }
  catch (const sc_unwind_exception& error) { throw; }
  catch (const exception& error) {
    ostringstream oss;
    oss << "std::exception caught in SC_THREAD " << __FUNCTION__ << " of " << kind() << " '" << name() << "'." << endl;
    oss << "what(): " << error.what() << endl;
    xtsc_log_multiline(m_text, log4xtensa::FATAL_LOG_LEVEL, oss.str(), 2);
    throw;
  }

}



// Thread to wait the appropriate delay for timing and latency then call nb_get_data() then drive m_data 
//  TODO: and then wait 1 clock cycle and then deassert m_data.
void xtsc_component::xtsc_pin2tlm_lookup_transactor::drive_data_thread(void) {
  try {
    m_drive_data.cancel_all();
    sc_bv_base data((int)m_data_width1);
    data = 0;
    m_data.write(data);
    while (true) {
      wait(m_drive_data.default_event());
      data = m_lookup->nb_get_data();
      XTSC_INFO(m_text, "Lookup: data = 0x" << data.to_string(SC_HEX).substr(m_data_width1%4 ? 2 : 3));
      m_data.write(data);
    }
  }
  catch (const sc_unwind_exception& error) { throw; }
  catch (const exception& error) {
    ostringstream oss;
    oss << "std::exception caught in SC_THREAD " << __FUNCTION__ << " of " << kind() << " '" << name() << "'." << endl;
    oss << "what(): " << error.what() << endl;
    xtsc_log_multiline(m_text, log4xtensa::FATAL_LOG_LEVEL, oss.str(), 2);
    throw;
  }
}



void xtsc_component::xtsc_pin2tlm_lookup_transactor::end_of_elaboration() {
  u32  address_width1 = m_address->read().length();
  u32  data_width1    = m_data->read().length();
  if (address_width1 != m_address_width1) {
    ostringstream oss;
    oss << "Width mismatch ERROR: xtsc_pin2tlm_lookup_transactor '" << name() << "' has configured address width of " << m_address_width1
        << " bits bound to pin-level lookup interface (channel) of width " << address_width1 << ".";
    throw xtsc_exception(oss.str());
  }
  if (data_width1 != m_data_width1) {
    ostringstream oss;
    oss << "Width mismatch ERROR: xtsc_pin2tlm_lookup_transactor '" << name() << "' has configured data width of " << m_data_width1
        << " bits bound to pin-level lookup interface (channel) of width " << data_width1 << ".";
    throw xtsc_exception(oss.str());
  }

  address_width1 = m_lookup->nb_get_address_bit_width();
  data_width1    = m_lookup->nb_get_data_bit_width();
  if (address_width1 != m_address_width1) {
    ostringstream oss;
    oss << "Width mismatch ERROR: xtsc_pin2tlm_lookup_transactor '" << name() << "' has configured address width of " << m_address_width1
        << " bits bound to Xtensa TLM lookup interface (channel) of width " << address_width1 << ".";
    throw xtsc_exception(oss.str());
  }
  if (data_width1 != m_data_width1) {
    ostringstream oss;
    oss << "Width mismatch ERROR: xtsc_pin2tlm_lookup_transactor '" << name() << "' has configured data width of " << m_data_width1
        << " bits bound to Xtensa TLM lookup interface (channel) of width " << data_width1 << ".";
    throw xtsc_exception(oss.str());
  }

}



