// Copyright (c) 2006-2012 by Tensilica Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Tensilica Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Tensilica Inc.


#include <cerrno>
#include <algorithm>
#include <ostream>
#include <string>
#include <xtsc/xtsc_lookup_pin.h>
#include <xtsc/xtsc_logging.h>


// xtsc_lookup_pin does binary logging of lookup events at verbose log level and xtsc_core does it
// at info log level.  This is the reverse of the normal arrangement because xtsc_core knows the
// PC and the port number, but xtsc_lookup_pin knows neither.

using namespace std;
#if SYSTEMC_VERSION >= 20050601
using namespace sc_core;
#endif
using namespace sc_dt;
using namespace xtsc;
using log4xtensa::INFO_LOG_LEVEL;
using log4xtensa::VERBOSE_LOG_LEVEL;
using log4xtensa::UNKNOWN;
using log4xtensa::UNKNOWN_PC;
using log4xtensa::LOOKUP_KEY;
using log4xtensa::RESPONSE_VALUE;


xtsc_component::xtsc_lookup_pin::xtsc_lookup_pin(sc_module_name module_name, const xtsc_lookup_pin_parms& lookup_parms) :
  sc_module             (module_name),
  xtsc_module           (*(sc_module*)this),
  m_address             ("m_address"),
  m_req                 ("m_req"),
  m_data                ("m_data"),
  m_ready               ("m_ready"),
  m_ready_floating      ("m_ready_floating", 1, log4xtensa::TextLogger::getInstance(name())),
  m_zero                (1),
  m_one                 (1),
#if IEEE_1666_SYSTEMC >= 201101L
  m_ready_event         ("m_ready_event"),
  m_data_event          ("m_data_event"),
  m_timeout_event       ("m_timeout_event"),
#endif
  m_address_bit_width   (lookup_parms.get_non_zero_u32("address_bit_width")),
  m_data_bit_width      (lookup_parms.get_non_zero_u32("data_bit_width")),
  m_next_address        (m_address_bit_width),
  m_default_data        ((int)m_data_bit_width),
  m_data_registered     ((int)m_data_bit_width),
  m_text                (log4xtensa::TextLogger::getInstance(name())),
  m_binary              (log4xtensa::BinaryLogger::getInstance(name()))
{

  m_zero                = 0;
  m_one                 = 1;
  m_line_count          = 0;
  m_next_address        = "0x0";
  m_default_data        = lookup_parms.get_c_str("default_data");
  m_p_trace_file        = static_cast<sc_trace_file*>(const_cast<void*>(lookup_parms.get_void_pointer("vcd_handle")));
  m_has_ready           = lookup_parms.get_bool("has_ready");
  m_file                = 0;
  m_delay_timeout       = SC_ZERO_TIME;

  m_time_resolution     = sc_get_time_resolution();

  // Get clock period 
  u32 clock_period = lookup_parms.get_non_zero_u32("clock_period");
  if (clock_period == 0xFFFFFFFF) {
    m_clock_period = xtsc_get_system_clock_period();
  }
  else {
    m_clock_period = m_time_resolution * clock_period;
  }
  m_clock_period_value = m_clock_period.value();
  u32 posedge_offset = lookup_parms.get_u32("posedge_offset");
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

  // Get clock phase when the req signal is to be sampled
  u32 sample_phase = lookup_parms.get_u32("sample_phase");
  if (sample_phase >= m_clock_period_value) {
    ostringstream oss;
    oss << "xtsc_lookup_pin '" << name() << "' parameter error:" << endl;
    oss << "  \"sample_phase\" (" << sample_phase
        << ") must be strictly less than \"clock_period\" (";
    if (clock_period == 0xFFFFFFFF) {
      oss << "0xFFFFFFFF => " << m_clock_period_value;
    }
    else {
      oss << clock_period;
    }
    oss << ")";
    throw xtsc_exception(oss.str());
  }
  m_sample_phase = m_time_resolution * sample_phase;
  m_sample_phase_value = m_sample_phase.value();

  u32 latency           = lookup_parms.get_non_zero_u32("latency");
  m_latency             = latency * m_clock_period;
  m_delay               = lookup_parms.get_u32("delay");

  u32 pipeline_depth    = lookup_parms.get_u32("pipeline_depth");
  m_pipeline_depth      = (pipeline_depth ? pipeline_depth : latency);
  m_pipeline_depth     += 1;  // +1 so we don't need a pipeline full flag
  m_pipeline_data       = new sc_bv_base*[m_pipeline_depth];
  m_pipeline_times      = new sc_time[m_pipeline_depth];

  for (u32 i=0; i<m_pipeline_depth; ++i) {
    m_pipeline_data[i] = 0;
  }

  const char *lookup_table = lookup_parms.get_c_str("lookup_table");

  SC_THREAD(request_thread);    m_process_handles.push_back(sc_get_current_process_handle());
  if (m_has_ready) {
    SC_THREAD(ready_thread);    m_process_handles.push_back(sc_get_current_process_handle());
  }
  SC_THREAD(data_thread);       m_process_handles.push_back(sc_get_current_process_handle());


  if (!lookup_table || !lookup_table[0]) {
    // No lookup_table specified, all addresses will return m_default_data
    m_lookup_table = "";
  }
  else {

    m_file = new xtsc_script_file(lookup_table, "lookup_table",  name(), kind(), false);
    m_lookup_table = m_file->name();

    XTSC_LOG(m_text, xtsc_get_constructor_log_level(), "Loading lookup table from file '" << m_lookup_table << "'.");

    while ((m_line_count = m_file->get_words(m_words, m_line, false)) != 0) {

      u32 num_words = m_words.size();

      if ((num_words < 1) || (num_words > 3)) {
        ostringstream oss;
        oss << "Invalid number of words (expected 1, 2, or 3):" << endl;
        oss << m_line;
        oss << m_file->info_for_exception();
        throw xtsc_exception(oss.str());
      }

      sc_bv_base address((int)m_address_bit_width);
      sc_bv_base *p_data = new sc_bv_base((int)m_data_bit_width);
      u32 delay = m_delay;
      if (num_words == 1) {
        address = m_next_address;
        get_sc_bv_base(0, *p_data);
      }
      else {
        if (m_words[num_words-1][0] == '@') {
          try {
            delay = xtsc_strtou32(m_words[num_words-1].substr(1));
          }
          catch (...) {
            ostringstream oss;
            oss << "Invalid delay:" << endl;
            oss << m_line;
            oss << m_file->info_for_exception();
            throw xtsc_exception(oss.str());
          }
          get_sc_bv_base(num_words-2, *p_data);
          if (num_words == 2) {
            address = m_next_address;
          }
          else {
            get_sc_bv_base(0, address);
          }
        }
        else {
          if (num_words == 3) {
            ostringstream oss;
            oss << "3rd word is not a delay (it must start with @):" << endl;
            oss << m_line;
            oss << m_file->info_for_exception();
            throw xtsc_exception(oss.str());
          }
          get_sc_bv_base(0, address);
          get_sc_bv_base(1, *p_data);
        }
      }

      ostringstream oss;
      oss << hex << "0x" << address;
      map<string, sc_bv_base*>::iterator imap = m_data_map.find(oss.str());
      if (imap == m_data_map.end()) {
        m_data_map[oss.str()] = p_data;
        m_delay_map[oss.str()] = delay;
      }
      else {
        ostringstream oss;
        oss << "Found duplicate address=" << address.to_string(SC_HEX) << ":" << endl;
        oss << m_line;
        oss << m_file->info_for_exception();
        throw xtsc_exception(oss.str());
      }

      m_next_address = address;
      m_next_address += 1;
      XTSC_VERBOSE(m_text, "Line " << dec << m_line_count << ": 0x" << hex << address << " => 0x" << *p_data << " delay=" << delay);
    }

  }

  m_port_types["m_req"]     = WIDE_INPUT;
  m_port_types["m_address"] = WIDE_INPUT;
  m_port_types["m_data"]    = WIDE_OUTPUT;
  if (m_has_ready) {
  m_port_types["m_ready"]   = WIDE_OUTPUT;
  }
  m_port_types["lookup"]    = PORT_TABLE;

  log4xtensa::LogLevel ll = xtsc_get_constructor_log_level();
  XTSC_LOG(m_text, ll, "Constructed xtsc_lookup_pin '" << name() << "':");
  XTSC_LOG(m_text, ll, " address_bit_width       = "   << m_address_bit_width);
  XTSC_LOG(m_text, ll, " data_bit_width          = "   << m_data_bit_width);
  XTSC_LOG(m_text, ll, " lookup_table            = "   << m_lookup_table);
  XTSC_LOG(m_text, ll, " pipeline_depth          = "   << pipeline_depth);
  XTSC_LOG(m_text, ll, " latency                 = "   << latency);
  XTSC_LOG(m_text, ll, " delay                   = "   << m_delay);
  XTSC_LOG(m_text, ll, " default_data            = 0x" << hex << m_default_data);
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
  XTSC_LOG(m_text, ll, " sample_phase            = "   << sample_phase << " (" << m_sample_phase << ")");
  XTSC_LOG(m_text, ll, " vcd_handle              = "   << m_p_trace_file);

  reset();
}



xtsc_component::xtsc_lookup_pin::~xtsc_lookup_pin(void) {
  map<string, sc_bv_base*>::iterator imap = m_data_map.begin();
  for (; imap != m_data_map.end(); ++imap) {
    sc_bv_base *p_data = (*imap).second;
    delete p_data;
  }
  delete m_pipeline_data;
  delete m_pipeline_times;
  if (m_file) {
    delete m_file;
  }
}



xtsc::u32 xtsc_component::xtsc_lookup_pin::get_bit_width(const string& port_name, u32 interface_num) const {
  if (port_name == "m_req")     return 1;
  if (port_name == "m_address") return m_address_bit_width;
  if (port_name == "m_data")    return m_data_bit_width;
  if (m_has_ready) {
  if (port_name == "m_ready")   return 1;
  }
  ostringstream oss;
  oss << kind() << " \"" << name() << "\" has no port named \"" << port_name << "\"" << endl;
  throw xtsc_exception(oss.str());
}



sc_object *xtsc_component::xtsc_lookup_pin::get_port(const string& port_name) {
  if (port_name == "m_req")     return &m_req;
  if (port_name == "m_address") return &m_address;
  if (port_name == "m_data")    return &m_data;
  if (m_has_ready) {
  if (port_name == "m_ready")   return &m_ready;
  }
  ostringstream oss;
  oss << kind() << " \"" << name() << "\" has no port named \"" << port_name << "\"" << endl;
  throw xtsc_exception(oss.str());
}



xtsc_port_table xtsc_component::xtsc_lookup_pin::get_port_table(const string& port_table_name) const {
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



void xtsc_component::xtsc_lookup_pin::before_end_of_elaboration() {
  if (m_ready.get_interface() == 0) {
    if (m_has_ready) {
      ostringstream oss;
      oss << "xtsc_lookup_pin '" << name()
          << "' was configured with \"has_ready\" parameter true; but the m_ready port has not been bound."; 
      throw xtsc_exception(oss.str());
    }
    // Cap m_ready 
    m_ready(m_ready_floating);
  }
  else {
    if (!m_has_ready) {
      ostringstream oss;
      oss << "xtsc_lookup_pin '" << name()
          << "' was configured with \"has_ready\" parameter false; but the m_ready port was externally bound."; 
      throw xtsc_exception(oss.str());
    }
  }
  if (m_p_trace_file) {
    sc_trace(m_p_trace_file, m_address,      m_address     .name());
    sc_trace(m_p_trace_file, m_req,          m_req         .name());
    sc_trace(m_p_trace_file, m_data,         m_data        .name());
    if (m_has_ready) {
    sc_trace(m_p_trace_file, m_ready,        m_ready       .name());
    }
  }
}



void xtsc_component::xtsc_lookup_pin::reset(bool /* hard_reset */) {
  XTSC_INFO(m_text, kind() << "::reset()");

  m_pipeline_wp         = 0;
  m_pipeline_rp         = 0;

  // Cancel notified events
  m_ready_event  .cancel();
  m_data_event   .cancel();
  m_timeout_event.cancel();

  xtsc_reset_processes(m_process_handles);
}



void xtsc_component::xtsc_lookup_pin::request_thread() {
  try {

    while (true) {

      if ((m_req.read() == m_zero) || ( m_has_ready && (m_ready.read() == m_zero))) {
        // (1) Wait for m_req or m_ready to change 
        wait(m_req.value_changed_event() | m_ready.value_changed_event());
      }

      // (2) Align to sample_phase
      u64 now_phase_value = sc_time_stamp().value() % m_clock_period_value;
      if (m_has_posedge_offset) {
        if (now_phase_value < m_posedge_offset_value) {
          now_phase_value += m_clock_period_value;
        }
        now_phase_value -= m_posedge_offset_value;
      }
      u64 align_delay = ((now_phase_value > m_sample_phase_value) ? m_clock_period_value : 0) + m_sample_phase_value - now_phase_value;
      wait(align_delay ? (align_delay * m_time_resolution) : m_clock_period);

      // (3) Ensure m_req and m_ready (if present) are still high  
      while ((m_req.read() != m_zero) && (!m_has_ready || (m_ready.read() != m_zero))) {

        // (4) Get address and lookup delay
        sc_bv_base address = m_address.read();

        // (5) Do lookup and schedule lookup data to be driven
        do_lookup(address, m_latency - m_clock_period);

        // (6) Wait one clock period before doing it again
        wait(m_clock_period);
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



void xtsc_component::xtsc_lookup_pin::ready_thread() {
  try {

    m_ready.write(m_zero);
    wait(m_delay * m_clock_period);
    m_ready.write(m_one);

    while (true) {

      XTSC_DEBUG(m_text, "ready_thread m_ready=" << m_ready.read() << ": waiting for m_ready_event");
      wait(m_ready_event);
      XTSC_DEBUG(m_text, "ready_thread m_ready=" << m_ready.read() << ": received m_ready_event");

      bool      full = pipeline_full();
      sc_time   now  = sc_time_stamp();

      if (full || (now < m_delay_timeout)) {
        XTSC_DEBUG(m_text, "ready_thread m_ready=" << m_ready.read() << ": writing m_zero, full=" << full);
        m_ready.write(m_zero);
        if (!full) {
          XTSC_DEBUG(m_text, "ready_thread m_ready=" << m_ready.read() << ": waiting for m_delay_timeout=" << m_delay_timeout);
          wait(m_delay_timeout - now);
          XTSC_DEBUG(m_text, "ready_thread m_ready=" << m_ready.read() << ": after m_delay_timeout, now writing m_one");
          m_ready.write(m_one);
        }
      }
      else {
        XTSC_DEBUG(m_text, "ready_thread m_ready=" << m_ready.read() << ": writing m_one");
        m_ready.write(m_one);
      }

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



void xtsc_component::xtsc_lookup_pin::data_thread() {
  try {

    m_data.write(m_zero);

    // Wait until we're suppose to drive the m_data signal for the first time
    wait(m_data_event);

    while (true) {

      if (!m_pipeline_data[m_pipeline_rp]) {
        ostringstream oss;
        oss << "xtsc_lookup_pin '" << name() << "' program bug in data_thread:  attempt to read undefined pipeline data. " 
            << " m_pipeline_rp=" << m_pipeline_rp;
        throw xtsc_exception(oss.str());
      }

      // Log it
      xtsc_log_lookup_event(m_binary, VERBOSE_LOG_LEVEL, UNKNOWN, RESPONSE_VALUE, UNKNOWN_PC, true,
                            *m_pipeline_data[m_pipeline_rp]);
      XTSC_VERBOSE(m_text, "Value=0x" << hex << *m_pipeline_data[m_pipeline_rp]);

      // If we were full, we aren't any longer
      if (pipeline_full()) {
        // Transition from full to not-full
        m_ready_event.notify(SC_ZERO_TIME);
        XTSC_DEBUG(m_text, "data_thread: transit from full to not-full so notifying m_ready_event");
      }

      // Register the output data
      m_data_registered = *m_pipeline_data[m_pipeline_rp];

      // Advance the pipeline
      m_pipeline_rp = (m_pipeline_rp + 1) % m_pipeline_depth;

      // Drive the registered data for 1 clock cycle
      m_data.write(m_data_registered);
      wait(m_clock_period);
      m_data.write(m_zero);

      // Is there anything in the pipeline?
      if (m_pipeline_rp == m_pipeline_wp) {
        // No, so wait until somebody notifies us
        wait(m_data_event);
      }
      else {
        // Yes, check if it is due now
        if (m_pipeline_times[m_pipeline_rp] <= sc_time_stamp()) {
          // It's due now, so just continue
          continue;
        }
        // It's due in the future, so wait until then
        wait(m_pipeline_times[m_pipeline_rp] - sc_time_stamp());
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



void xtsc_component::xtsc_lookup_pin::get_sc_bv_base(u32 index, sc_bv_base& value) {
  try {
    // Prevent sign extension that sc_bv_base does when assigned a hex string with the high bit set
    string word_no_sign_extend = m_words[index];
    if ((word_no_sign_extend.size() > 2) && (word_no_sign_extend.substr(0,2) == "0x")) {
      word_no_sign_extend.insert(2, "0");
    }
    // Prefix '0d' if required
    if ((word_no_sign_extend.size() < 3) ||
        (word_no_sign_extend[0] != '0')  ||
        (word_no_sign_extend.substr(1,1).find_first_of("bdoxc") == string::npos))
    {
      word_no_sign_extend = "0d" + word_no_sign_extend;
    }
    value = word_no_sign_extend.c_str();
  }
  catch (...) {
    ostringstream oss;
    oss << "Cannot convert word #" << index+1 << " to number:" << endl;
    oss << m_line;
    oss << m_file->info_for_exception();
    throw xtsc_exception(oss.str());
  }
}



bool xtsc_component::xtsc_lookup_pin::pipeline_full() {
  bool full = (((m_pipeline_wp + 1) % m_pipeline_depth) == m_pipeline_rp);
  if (full) {
    XTSC_DEBUG(m_text, "pipeline is full.");
  }
  return full;
}



void xtsc_component::xtsc_lookup_pin::do_lookup(const sc_bv_base& address, const sc_time& when) {
  XTSC_DEBUG(m_text, "do_lookup(when=" << when << ")");

  if (pipeline_full()) {
    if (m_has_ready) {
      ostringstream oss;
      oss << "xtsc_lookup_pin '" << name() << "' program bug:  attempt to add lookup to full pipeline when \"has_ready\"=true";
      throw xtsc_exception(oss.str());
    }
    else {
      ostringstream oss;
      oss << "xtsc_lookup_pin '" << name() << "': attempt to do another lookup when pipeline is already full.  " 
          << "\"pipeline_depth\"=" << m_pipeline_depth-1 << " \"latency\"=" << (m_latency / m_clock_period);
      throw xtsc_exception(oss.str());
    }
  }

  // Do actual lookup, compute next delay, and add lookup data to pipeline
  ostringstream oss;
  oss << "0x" << hex << address;
  map<string, sc_bv_base*>::iterator imap = m_data_map.find(oss.str());
  u32 delay = 0;
  if (imap == m_data_map.end()) {
    m_pipeline_data[m_pipeline_wp] = &m_default_data;
    delay = m_delay;
  }
  else {
    m_pipeline_data[m_pipeline_wp] = (*imap).second;
    delay =  m_delay_map.find(oss.str())->second;
  }

  // Handle any delay
  if (delay) {
    m_delay_timeout = sc_time_stamp() + (delay * m_clock_period);
    m_ready_event.notify(SC_ZERO_TIME);
  }
  
  // Add data drive time to pipeline
  m_pipeline_times[m_pipeline_wp] = sc_time_stamp() + when;

  // Notify data_thread
  m_data_event.notify(when);

  // Log it
  xtsc_log_lookup_event(m_binary, VERBOSE_LOG_LEVEL, UNKNOWN, LOOKUP_KEY, UNKNOWN_PC, true, address);
  XTSC_INFO(m_text, "Ready. address=0x" << hex << address << " data=0x" << *m_pipeline_data[m_pipeline_wp]);

  // Advance pipeline
  m_pipeline_wp = (m_pipeline_wp + 1) % m_pipeline_depth;

  if (pipeline_full()) {
    // Transition from not-full to full
    XTSC_DEBUG(m_text, "request_thread: notifying m_ready_event.  pipeline_full()=" << pipeline_full());
    m_ready_event.notify(SC_ZERO_TIME);
  }
}



