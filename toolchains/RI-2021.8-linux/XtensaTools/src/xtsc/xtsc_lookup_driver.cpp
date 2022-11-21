// Copyright (c) 2006-2012 by Tensilica Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Tensilica Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Tensilica Inc.


#include <cctype>
#include <algorithm>
#include <xtsc/xtsc_lookup_driver.h>
#include <xtsc/xtsc_interrupt_distributor.h>



using namespace std;
#if SYSTEMC_VERSION >= 20050601
using namespace sc_core;
#endif
using namespace sc_dt;
using namespace xtsc;
using log4xtensa::INFO_LOG_LEVEL;
using log4xtensa::VERBOSE_LOG_LEVEL;



xtsc_component::xtsc_lookup_driver::xtsc_lookup_driver(sc_module_name module_name, const xtsc_lookup_driver_parms& driver_parms) :
  sc_module             (module_name),
  xtsc_module           (*(sc_module*)this),
  m_address             ("m_address"),
  m_req                 ("m_req"),
  m_data                ("m_data"),
  m_ready               ("m_ready"),
  m_lookup              ("m_lookup"),
  m_text                (log4xtensa::TextLogger::getInstance(name())),
  m_has_ready           (driver_parms.get_bool("has_ready")),
  m_test_vector_stream  (driver_parms.get_c_str("script_file"), "script_file",  name(), kind(), false),
  m_script_file         (m_test_vector_stream.name()),
  m_timeout             (sc_max_time()),
  m_address_width1      (driver_parms.get_non_zero_u32("address_bit_width")),
  m_data_width1         (driver_parms.get_non_zero_u32("data_bit_width")),
  m_lookup_address      (m_address_width1),
  m_lookup_address_bv   ((int)m_address_width1),
  m_pin_level           (driver_parms.get_bool("pin_level")),
  m_zero                (1),
  m_one                 (1),
  m_zero_bv             (1),
  m_one_bv              (1),
  m_sample_data         ("m_sample_data"),
  m_address_floating    ("m_address_floating", m_address_width1, m_text),
  m_req_floating        ("m_req_floating", 1, m_text),
  m_data_floating       ("m_data_floating", m_data_width1, m_text),
  m_ready_floating      ("m_ready_floating", 1, m_text)
{

  m_zero        = 0;
  m_one         = 1;
  m_zero_bv     = 0;
  m_one_bv      = 1;

  // Get clock period 
  m_time_resolution = sc_get_time_resolution();
  u32 clock_period = driver_parms.get_non_zero_u32("clock_period");
  if (clock_period == 0xFFFFFFFF) {
    m_clock_period = xtsc_get_system_clock_period();
  }
  else {
    m_clock_period = sc_get_time_resolution() * clock_period;
  }
  m_clock_period_value = m_clock_period.value();
  u32 posedge_offset = driver_parms.get_u32("posedge_offset");
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
  u32 sample_phase = driver_parms.get_u32("sample_phase");
  if (sample_phase >= m_clock_period.value()) {
    ostringstream oss;
    oss << "xtsc_lookup_driver '" << name() << "' parameter error:" << endl;
    oss << "  \"sample_phase\" (" << sample_phase
        << ") must be strictly less than \"clock_period\" (";
    if (clock_period == 0xFFFFFFFF) {
      oss << "0xFFFFFFFF => " << m_clock_period.value();
    }
    else {
      oss << clock_period;
    }
    oss << ")";
    throw xtsc_exception(oss.str());
  }
  m_sample_phase = sc_get_time_resolution() * sample_phase;

  // Get latency
  u32 latency = driver_parms.get_u32("latency");
  if (latency < 1) {
    ostringstream oss;
    oss << "xtsc_lookup_driver '" << name() << "' parameter error:" << endl;
    oss << "  \"latency\" cannot be 0.";
    throw xtsc_exception(oss.str());
  }
  m_latency = m_clock_period * latency;

  // Get delay before calling nb_is_ready
  m_poll_ready_delay = SC_ZERO_TIME;
  u32 poll_ready_delay = driver_parms.get_u32("poll_ready_delay");
  if (m_has_ready && !m_pin_level) {
    if (poll_ready_delay == 0xFFFFFFFF) {
      m_poll_ready_delay = m_clock_period * 0.5;
    }
    else {
      if (poll_ready_delay >= m_clock_period.value()) {
        ostringstream oss;
        oss << "xtsc_lookup_driver '" << name() << "' parameter error:" << endl;
        oss << "  \"poll_ready_delay\" (" << poll_ready_delay
            << ") must be strickly less than \"clock_period\" (";
        if (clock_period == 0xFFFFFFFF) {
          oss << "0xFFFFFFFF => " << m_clock_period.value();
        }
        else {
          oss << clock_period;
        }
        oss << ")";
        throw xtsc_exception(oss.str());
      }
      m_poll_ready_delay = sc_get_time_resolution() * poll_ready_delay;
    }
  }
  m_delay_after_ready = m_clock_period - m_poll_ready_delay;
  m_notify_delay = m_latency - m_poll_ready_delay;

  // Get how long after the ready signal is sampled that the req signal should be deasserted
  u32 deassert_delay = driver_parms.get_u32("deassert_delay");
  if (deassert_delay >= m_clock_period.value()) {
    ostringstream oss;
    oss << "  \"deassert_delay\" (" << deassert_delay
        << ") must be strictly less than \"clock_period\" (";
    if (clock_period == 0xFFFFFFFF) {
      oss << "0xFFFFFFFF => " << m_clock_period.value();
    }
    else {
      oss << clock_period;
    }
    oss << ")";
    throw xtsc_exception(oss.str());
  }
  m_deassert_delay = sc_get_time_resolution() * deassert_delay;


  SC_THREAD(script_thread);             m_process_handles.push_back(sc_get_current_process_handle());
  SC_THREAD(sample_data_thread);        m_process_handles.push_back(sc_get_current_process_handle());

  if (m_pin_level) {
    m_p_trace_file = static_cast<sc_trace_file*>(const_cast<void*>(driver_parms.get_void_pointer("vcd_handle")));
    SC_THREAD(sample_phase_thread);     m_process_handles.push_back(sc_get_current_process_handle());
    SC_THREAD(request_thread);          m_process_handles.push_back(sc_get_current_process_handle());
    m_lookup(*this);
    m_port_types["m_req"]     = WIDE_OUTPUT;
    m_port_types["m_address"] = WIDE_OUTPUT;
    m_port_types["m_data"]    = WIDE_INPUT;
    if (m_has_ready) {
    m_port_types["m_ready"]   = WIDE_INPUT;
    }
    else {
      m_ready(m_ready_floating);
    }
    if (m_p_trace_file) {
      sc_trace(m_p_trace_file, m_address, m_address .name());
      sc_trace(m_p_trace_file, m_req,     m_req     .name());
      sc_trace(m_p_trace_file, m_data,    m_data    .name());
      if (m_has_ready) {
      sc_trace(m_p_trace_file, m_ready,   m_ready   .name());
      }
    }
  }
  else {
    m_p_trace_file = NULL;
    m_address(m_address_floating);
    m_req(m_req_floating);
    m_data(m_data_floating);
    m_ready(m_ready_floating);
    m_port_types["m_lookup"]  = LOOKUP_PORT;
  }
  m_port_types["lookup"]      = PORT_TABLE;

  log4xtensa::LogLevel ll = xtsc_get_constructor_log_level();
  XTSC_LOG(m_text, ll, "Constructed xtsc_lookup_driver '" << name() << "':");
  XTSC_LOG(m_text, ll, " address_bit_width       = "   << m_address_width1);
  XTSC_LOG(m_text, ll, " data_bit_width          = "   << m_data_width1);
  XTSC_LOG(m_text, ll, " has_ready               = "   << boolalpha << m_has_ready);
  XTSC_LOG(m_text, ll, " latency                 = "   << latency << " (" << m_latency << ")");
  XTSC_LOG(m_text, ll, " pin_level               = "   << boolalpha << m_pin_level);
  if (m_pin_level) {
  XTSC_LOG(m_text, ll, " vcd_handle              = "   << m_p_trace_file);
  }
  XTSC_LOG(m_text, ll, " script_file             = "   << m_script_file);
  if (clock_period == 0xFFFFFFFF) {
  XTSC_LOG(m_text, ll, " clock_period            = 0xFFFFFFFF => " << m_clock_period.value() << " (" << m_clock_period << ")");
  } else {
  XTSC_LOG(m_text, ll, " clock_period            = "   << clock_period << " (" << m_clock_period << ")");
  }
  if (m_has_ready && !m_pin_level) {
  if (poll_ready_delay == 0xFFFFFFFF) {
  XTSC_LOG(m_text, ll, " poll_ready_delay        = 0xFFFFFFFF => " << m_poll_ready_delay.value() << " (" << m_poll_ready_delay << ")");
  } else {
  XTSC_LOG(m_text, ll, " poll_ready_delay        = "   << poll_ready_delay << " (" << m_poll_ready_delay << ")");
  }
  }
  if (posedge_offset == 0xFFFFFFFF) {
  XTSC_LOG(m_text, ll, " posedge_offset          = 0xFFFFFFFF => " << m_posedge_offset.value() << " (" << m_posedge_offset << ")");
  } else {
  XTSC_LOG(m_text, ll, " posedge_offset          = "   << posedge_offset << " (" << m_posedge_offset << ")");
  }
  XTSC_LOG(m_text, ll, " sample_phase            = "   << sample_phase << " (" << m_sample_phase << ")");
  XTSC_LOG(m_text, ll, " deassert_delay          = "   << deassert_delay << " (" << m_deassert_delay << ")");

  reset();

}



xtsc_component::xtsc_lookup_driver::~xtsc_lookup_driver(void) {
}



u32 xtsc_component::xtsc_lookup_driver::get_bit_width(const string& port_name, u32 interface_num) const {
  if (m_pin_level) {
    if (port_name == "m_req")     return 1;
    if (port_name == "m_address") return m_address_width1;
    if (port_name == "m_data")    return m_data_width1;
    if (m_has_ready) {
    if (port_name == "m_ready")   return 1;
    }
  }
  else {
    if (port_name == "m_lookup") return ((interface_num == 0) ?  m_data_width1 : m_address_width1);
  }
  ostringstream oss;
  oss << kind() << " \"" << name() << "\" has no port named \"" << port_name << "\"" << endl;
  throw xtsc_exception(oss.str());
}



sc_object *xtsc_component::xtsc_lookup_driver::get_port(const string& port_name) {
  if (m_pin_level) {
    if (port_name == "m_req")     return &m_req;
    if (port_name == "m_address") return &m_address;
    if (port_name == "m_data")    return &m_data;
    if (m_has_ready) {
    if (port_name == "m_ready")   return &m_ready;
    }
  }
  else {
    if (port_name == "m_lookup") return &m_lookup;
  }
  ostringstream oss;
  oss << kind() << " \"" << name() << "\" has no port named \"" << port_name << "\"" << endl;
  throw xtsc_exception(oss.str());
}



xtsc_port_table xtsc_component::xtsc_lookup_driver::get_port_table(const string& port_table_name) const {
  xtsc_port_table table;

  if (port_table_name == "lookup") {
    if (m_pin_level) {
      table.push_back("m_req");
      table.push_back("m_address");
      table.push_back("m_data");
      if (m_has_ready) {
      table.push_back("m_ready");
      }
    }
    else {
      table.push_back("m_lookup");
    }
  }
  else {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\" has no port table named \"" << port_table_name << "\"" << endl;
    throw xtsc_exception(oss.str());
  }

  return table;
}



void xtsc_component::xtsc_lookup_driver::connect(xtsc_interrupt_distributor& distributor, const char *lookup_name) {
  if (m_pin_level) {
    ostringstream oss;
    oss << "xtsc_lookup_driver '" << name() << "' has \"pin_level\" true, so the connect() method should not be called.";
    throw xtsc_exception(oss.str());
  }
  u32 addr_width = distributor.get_address_bit_width();
  if (m_address_width1 != addr_width) {
    ostringstream oss;
    oss << "Width mismatch:  cannot connect xtsc_lookup_driver '" << name() 
        << "' with address width of " << m_address_width1 << " bits to lookup '" << lookup_name 
        << "' of xtsc_interrupt_distributor '" << name() << "' with address width of " << addr_width << " bits";
    throw xtsc_exception(oss.str());
  }
  u32 data_width = distributor.get_data_bit_width();
  if (m_data_width1 != data_width) {
    ostringstream oss;
    oss << "Width mismatch:  cannot connect xtsc_lookup_driver '" << name() 
        << "' with data width of " << m_data_width1 << " bits to lookup '" << lookup_name 
        << "' of xtsc_interrupt_distributor '" << name() << "' with data width of " << data_width << " bits";
    throw xtsc_exception(oss.str());
  }
  m_lookup(distributor.get_lookup(lookup_name));
}



void xtsc_component::xtsc_lookup_driver::reset(bool /*hard_reset*/) {
  XTSC_INFO(m_text, "xtsc_lookup_driver::reset()");

  m_words.clear();
  m_line        = "";
  m_line_count  = 0;
  m_no_timeout  = false;
  m_test_vector_stream.reset();

  // Cancel notified events
  m_next_request.cancel();
  m_assert      .cancel();
  m_deassert    .cancel();
  m_sample_data .cancel_all();

  xtsc_reset_processes(m_process_handles);
}



void xtsc_component::xtsc_lookup_driver::script_thread(void) {

  try {

    while ((m_line_count = m_test_vector_stream.get_words(m_words, m_line, true)) != 0) {

      XTSC_DEBUG(m_text, "\"script_file\" line #" << m_line_count << ": " << m_line);

      if (m_words[0] == "wait") {
        if (m_words.size() != 2) {
          ostringstream oss;
          oss << "WAIT command has missing/extra arguments:" << endl;
          oss << m_line;
          oss << m_test_vector_stream.info_for_exception();
          throw xtsc_exception(oss.str());
        }
        double time = get_double(1, "duration");
        sc_time duration = time * m_clock_period;
        XTSC_DEBUG(m_text, "waiting " << duration);
        wait(duration);
        continue;
      }

      if ((m_words[0] == "sync") || (m_words[0] == "synchronize")) {
        if (m_words.size() != 2) {
          ostringstream oss;
          oss << "SYNC command has missing/extra arguments:" << endl;
          oss << m_line;
          oss << m_test_vector_stream.info_for_exception();
          throw xtsc_exception(oss.str());
        }
        double time = get_double(1, "time");
        if (time < 1.0) {
          sc_time sync_phase = time * m_clock_period;
          sc_time now = sc_time_stamp();
          sc_time phase_now = (now.value() % m_clock_period_value) * m_time_resolution;
          if (m_has_posedge_offset) {
            if (phase_now < m_posedge_offset) {
              phase_now += m_clock_period;
            }
            phase_now -= m_posedge_offset;
          }
          XTSC_DEBUG(m_text, "sync_phase=" << sync_phase << " phase_now=" << phase_now);
          sc_time delta = ((sync_phase >= phase_now) ? (sync_phase - phase_now) : (m_clock_period + sync_phase - phase_now));
          if (delta != SC_ZERO_TIME) {
            XTSC_DEBUG(m_text, "waiting " << delta << " to sync to phase " << sync_phase);
            wait(delta);
          }
        }
        else {
          sc_time absolute_time = time * m_clock_period;
          sc_time now = sc_time_stamp();
          if (absolute_time > now) {
            sc_time delta = absolute_time - now;
            XTSC_DEBUG(m_text, "waiting " << delta << " to sync to time " << absolute_time);
            wait(delta);
          }
        }
        continue;
      }

      if (m_words[0] == "note") {
        XTSC_NOTE(m_text, m_line);
        continue;
      }

      if (m_words[0] == "info") {
        XTSC_INFO(m_text, m_line);
        continue;
      }

      if ((m_words.size() != 2) && (m_words.size() != 3)) {
        ostringstream oss;
        oss << "Line is malformed" << endl;
        oss << m_line;
        oss << m_test_vector_stream.info_for_exception();
        throw xtsc_exception(oss.str());
      }

      if (m_words[0] != "now") {
          double delay = get_double(0, "delay");
          XTSC_DEBUG(m_text, "script_thread is delaying for " << delay << " clock periods");
          wait(delay * m_clock_period);
          XTSC_DEBUG(m_text, "script_thread done with delay");
      }

      if (m_words[1] == "stop") {
        XTSC_INFO(m_text, "script_thread calling sc_stop()");
        sc_stop();
        wait();
      }

      m_lookup_address = m_words[1].c_str();
      m_lookup_address_bv = m_lookup_address;

      double timeout = (m_has_ready ? -1 : 1);
      if (m_has_ready && (m_words.size() == 3)) {
         timeout = get_double(2, "timeout");
      }
      m_no_timeout =  (timeout < 0) ? true : false;

      if (m_pin_level) {
        // Pin-level interface
        m_assert.notify();      // Immediate notification
        if (!m_no_timeout) {
          XTSC_DEBUG(m_text, "script_thread is calling m_deassert.notify(" << (timeout * m_clock_period) << ")");
          m_deassert.notify(timeout * m_clock_period);
          m_timeout = sc_time_stamp() + (timeout * m_clock_period);
        }
        XTSC_DEBUG(m_text, "script_thread is waiting for m_next_request event");
        wait(m_next_request);
        XTSC_DEBUG(m_text, "script_thread received m_next_request event");
      }
      else {
        // TLM interface
        for (double count = 1; m_no_timeout || (count <= timeout); count += 1) {
          m_lookup->nb_send_address(m_lookup_address);
          if (m_has_ready) {
            wait(m_poll_ready_delay);
          }
          if (!m_has_ready || m_lookup->nb_is_ready()) {
            m_sample_data.notify(m_notify_delay);
            XTSC_INFO(m_text, "Lookup ready, address = 0x" << m_lookup_address.to_string(SC_HEX).substr(m_address_width1%4 ? 2 : 3));
            if (m_has_ready) {
              wait(m_delay_after_ready);
            }
            break;
          }
          XTSC_DEBUG(m_text, "Lookup is not ready for address = 0x" <<
                             m_lookup_address.to_string(SC_HEX).substr(m_address_width1%4 ? 2 : 3));
          wait(m_delay_after_ready);
        }
      }
    }

  }
  catch (const sc_unwind_exception& error) { throw; }
  catch (const exception& error) {
    ostringstream oss;
    oss << "std::exception caught in SC_THREAD of " << kind() << " '" << name() << "'." << endl;
    oss << "what(): " << error.what() << endl;
    xtsc_log_multiline(m_text, log4xtensa::FATAL_LOG_LEVEL, oss.str(), 2);
    throw;
  }

}



void xtsc_component::xtsc_lookup_driver::sample_phase_thread(void) {

  try {

    wait(m_sample_phase + m_posedge_offset);

    while (true) {
      if ((m_req.read() != m_zero_bv) && (!m_has_ready || (m_ready.read() != m_zero_bv))) {
        m_sample_data.notify(m_latency);
        XTSC_INFO(m_text, "Lookup ready, address = 0x" << m_address.read().to_string(SC_HEX).substr(m_address_width1%4 ? 2 : 3));
        // Deassert request (but first ensure there isn't a new request after a timeout that we would be deasserting)
        if (m_no_timeout || ((sc_time_stamp() + m_deassert_delay) < m_timeout)) {
          XTSC_DEBUG(m_text, "sample_phase_thread is calling m_deassert.notify(" << m_deassert_delay << ")");
          m_deassert.notify(m_deassert_delay);
        }
      }
      wait(m_clock_period);
    }

  }
  catch (const sc_unwind_exception& error) { throw; }
  catch (const exception& error) {
    ostringstream oss;
    oss << "std::exception caught in SC_THREAD of " << kind() << " '" << name() << "'." << endl;
    oss << "what(): " << error.what() << endl;
    xtsc_log_multiline(m_text, log4xtensa::FATAL_LOG_LEVEL, oss.str(), 2);
    throw;
  }

}



void xtsc_component::xtsc_lookup_driver::sample_data_thread(void) {

  try {
    if (m_pin_level) {
      sc_bv_base data((int)m_data_width1);
      while (true) {
        wait(m_sample_data.default_event());
        data = m_data.read();
        XTSC_INFO(m_text, "Lookup data = 0x" << data.to_string(SC_HEX).substr(m_data_width1%4 ? 2 : 3));
      }
    }
    else {
      sc_unsigned data(m_data_width1);
      while (true) {
        wait(m_sample_data.default_event());
        data = m_lookup->nb_get_data();
        XTSC_INFO(m_text, "Lookup data = 0x" << data.to_string(SC_HEX).substr(m_data_width1%4 ? 2 : 3));
      }
    }
  }
  catch (const sc_unwind_exception& error) { throw; }
  catch (const exception& error) {
    ostringstream oss;
    oss << "std::exception caught in SC_THREAD of " << kind() << " '" << name() << "'." << endl;
    oss << "what(): " << error.what() << endl;
    xtsc_log_multiline(m_text, log4xtensa::FATAL_LOG_LEVEL, oss.str(), 2);
    throw;
  }

}



void xtsc_component::xtsc_lookup_driver::request_thread(void) {

  try {

    m_req.write(m_zero_bv);
    m_address.write(m_zero_bv);

    while (true) {
      wait(m_assert);
      XTSC_DEBUG(m_text, "request_thread received m_assert event.  m_lookup_address_bv=0x" << hex << m_lookup_address_bv);
      m_req.write(m_one_bv);
      m_address.write(m_lookup_address_bv);
      wait(m_deassert);
      XTSC_DEBUG(m_text, "request_thread received m_deassert event");
      m_req.write(m_zero_bv);
      m_address.write(m_zero_bv);
      m_next_request.notify();  // Immediate notification
    }

  }
  catch (const sc_unwind_exception& error) { throw; }
  catch (const exception& error) {
    ostringstream oss;
    oss << "std::exception caught in SC_THREAD of " << kind() << " '" << name() << "'." << endl;
    oss << "what(): " << error.what() << endl;
    xtsc_log_multiline(m_text, log4xtensa::FATAL_LOG_LEVEL, oss.str(), 2);
    throw;
  }

}



u32 xtsc_component::xtsc_lookup_driver::get_u32(u32 index, const string& argument_name) {
  u32 value = 0;
  if (index >= m_words.size()) {
    ostringstream oss;
    oss << argument_name << " argument (#" << index+1 << ") missing:" << endl;
    oss << m_line;
    oss << m_test_vector_stream.info_for_exception();
    throw xtsc_exception(oss.str());
  }
  try {
    value = xtsc_strtou32(m_words[index]);
  }
  catch (const xtsc_exception&) {
    ostringstream oss;
    oss << "Cannot convert " << argument_name << " argument (#" << index+1 << ") '" << m_words[index] << "' to number:" << endl;
    oss << m_line;
    oss << m_test_vector_stream.info_for_exception();
    throw xtsc_exception(oss.str());
  }
  return value;
}



double xtsc_component::xtsc_lookup_driver::get_double(u32 index, const string& argument_name) {
  double value = 0;
  if (index >= m_words.size()) {
    ostringstream oss;
    oss << argument_name << " argument (#" << index+1 << ") missing:" << endl;
    oss << m_line;
    oss << m_test_vector_stream.info_for_exception();
    throw xtsc_exception(oss.str());
  }
  try {
    value = xtsc_strtod(m_words[index]);
  }
  catch (const xtsc_exception&) {
    ostringstream oss;
    oss << "Cannot convert " << argument_name << " argument (#" << index+1 << ") '" << m_words[index] << "' to number:" << endl;
    oss << m_line;
    oss << m_test_vector_stream.info_for_exception();
    throw xtsc_exception(oss.str());
  }
  return value;
}



void xtsc_component::xtsc_lookup_driver::end_of_elaboration() {
  u32  address_width1 = 0;
  u32  data_width1    = 0;
  if (m_pin_level) {
    address_width1 = m_address->read().length();
    data_width1    = m_data->read().length();
  }
  else {
    address_width1 = m_lookup->nb_get_address_bit_width();
    data_width1    = m_lookup->nb_get_data_bit_width();
  }
  if (address_width1 != m_address_width1) {
    ostringstream oss;
    oss << "Width mismatch ERROR: xtsc_lookup_driver '" << name() << "' has configured address width of " << m_address_width1
        << " bits bound to lookup interface (channel) of width " << address_width1 << ".";
    throw xtsc_exception(oss.str());
  }
  if (data_width1 != m_data_width1) {
    ostringstream oss;
    oss << "Width mismatch ERROR: xtsc_lookup_driver '" << name() << "' has configured data width of " << m_data_width1
        << " bits bound to lookup interface (channel) of width " << data_width1 << ".";
    throw xtsc_exception(oss.str());
  }
}



