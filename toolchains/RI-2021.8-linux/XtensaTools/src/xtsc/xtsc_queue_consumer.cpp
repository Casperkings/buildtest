// Copyright (c) 2006-2017 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Cadence Design Systems, Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Cadence Design Systems, Inc.


#include <cctype>
#include <algorithm>
#include <xtsc/xtsc_queue_consumer.h>
#include <xtsc/xtsc_tx_loader.h>



using namespace std;
#if SYSTEMC_VERSION >= 20050601
using namespace sc_core;
#endif
using namespace sc_dt;
using namespace xtsc;
using log4xtensa::INFO_LOG_LEVEL;
using log4xtensa::VERBOSE_LOG_LEVEL;



xtsc_component::xtsc_queue_consumer::xtsc_queue_consumer(sc_module_name module_name, const xtsc_queue_consumer_parms& consumer_parms) :
  sc_module             (module_name),
  xtsc_module           (*(sc_module*)this),
  m_pop                 ("m_pop"),
  m_data                ("m_data"),
  m_poppable            ("m_poppable"),
  m_empty               ("m_empty"),
  m_queue               ("m_queue"),
  m_text                (log4xtensa::TextLogger::getInstance(name())),
  m_wraparound          (consumer_parms.get_bool("wraparound")),
  m_test_vector_stream  (consumer_parms.get_c_str("script_file"), "script_file",  name(), kind(), m_wraparound),
  m_script_file         (m_test_vector_stream.name()),
  m_width1              (consumer_parms.get_non_zero_u32("bit_width")),
  m_nx                  (consumer_parms.get_bool("nx")),
  m_pin_level           (consumer_parms.get_bool("pin_level")),
  m_pop_count           (0),
  m_ticket              ((u64)-1),
  m_peek_value          (m_width1),
  m_value               (m_width1),
  m_value_bv            ((int)m_width1),
  m_zero_bv             (1),
  m_one_bv              (1),
#if IEEE_1666_SYSTEMC >= 201101L
  m_next_request        ("m_next_request"),
  m_assert              ("m_assert"),
  m_deassert            ("m_deassert"),
#endif
  m_pop_floating        ("m_pop_floating",      1,        m_text),
  m_data_floating       ("m_data_floating",     m_width1, m_text),
  m_poppable_floating   ("m_poppable_floating", 8,        m_text),
  m_empty_floating      ("m_empty_floating",    1,        m_text)
{

  m_zero_bv     = 0;
  m_one_bv      = 1;

  // Get clock period 
  m_time_resolution = sc_get_time_resolution();
  u32 clock_period = consumer_parms.get_non_zero_u32("clock_period");
  if (clock_period == 0xFFFFFFFF) {
    m_clock_period = xtsc_get_system_clock_period();
  }
  else {
    m_clock_period = sc_get_time_resolution() * clock_period;
  }
  m_clock_period_value = m_clock_period.value();
  u32 posedge_offset = consumer_parms.get_u32("posedge_offset");
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

  // Get clock phase when the empty signal is to be sampled
  u32 sample_phase = consumer_parms.get_u32("sample_phase");
  if (sample_phase >= m_clock_period.value()) {
    ostringstream oss;
    oss << "xtsc_queue_consumer '" << name() << "' parameter error:" << endl;
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

  // Get how long after the empty signal is sampled that the pop signal should be deasserted
  u32 deassert_delay = consumer_parms.get_u32("deassert_delay");
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


  SC_THREAD(script_thread);     m_process_handles.push_back(sc_get_current_process_handle());
  if (m_pin_level) {
    m_p_trace_file = static_cast<sc_trace_file*>(const_cast<void*>(consumer_parms.get_void_pointer("vcd_handle")));
    SC_THREAD(sample_thread);   m_process_handles.push_back(sc_get_current_process_handle());
    SC_THREAD(request_thread);  m_process_handles.push_back(sc_get_current_process_handle());
    m_queue(*this);
    if (m_p_trace_file) {
                { sc_trace(m_p_trace_file, m_pop,      m_pop     .name()); }
                { sc_trace(m_p_trace_file, m_data,     m_data    .name()); }
      if (m_nx) { sc_trace(m_p_trace_file, m_poppable, m_poppable.name()); }
      else      { sc_trace(m_p_trace_file, m_empty,    m_empty   .name()); }
    }
    if (!m_nx) { m_poppable(m_poppable_floating); }
    else       { m_empty(m_empty_floating); }
  }
  else {
    m_p_trace_file = NULL;
    m_pop(m_pop_floating);
    m_data(m_data_floating);
    m_poppable(m_poppable_floating);
    m_empty(m_empty_floating);
  }

  if (m_pin_level) {
              { m_port_types["m_pop"]      = WIDE_OUTPUT; }
              { m_port_types["m_data"]     = WIDE_INPUT;  }
    if (m_nx) { m_port_types["m_poppable"] = WIDE_INPUT;  }
    else      { m_port_types["m_empty"]    = WIDE_INPUT;  }
  }
  else {
    m_port_types["m_queue"] = QUEUE_POP_PORT;
  }
  m_port_types["queue_pop"] = PORT_TABLE;
  m_port_types["queue"]     = PORT_TABLE;

  xtsc_register_command(*this, *this, "can_pop", 0, 0,
      "can_pop", 
      "Return nb_can_pop() (TLM), or m_poppable>0 (NX pin-level), or !m_empty (LX pin-level)."
  );

  xtsc_register_command(*this, *this, "get_pop_count", 0, 0,
      "get_pop_count",
      "Return number of items popped since simulation began."
  );

  xtsc_register_command(*this, *this, "get_pop_data", 0, 0,
      "get_pop_data",
      "Return most recent popped data."
  );

  if (!m_pin_level) {
  xtsc_register_command(*this, *this, "get_pop_ticket", 0, 0,
      "get_pop_ticket",
      "Return ticket of most recent popped data."
  );
  }

  xtsc_register_command(*this, *this, "nb_peek", 0, 0,
      "nb_peek", 
      "Calls nb_peek(value, ticket) and returns value."
  );

  xtsc_register_command(*this, *this, "num_available_entries", 0, 0,
      "num_available_entries", 
      "Return nb_num_available_entries() (TLM), or m_poppable (NX pin-level), or 0|1 depending on the m_empty signal (LX pin-level)."
  );

  if (!m_pin_level) {
  xtsc_register_command(*this, *this, "pop", 0, 0,
      "pop", 
      "Calls nb_pop(value, ticket) and returns value if successful, else throws exception."
  );
  }

  xtsc_register_command(*this, *this, "reset", 0, 1,
      "reset [<Hard>]", 
      "Call reset(<Hard>).  Where <Hard> is 0|1 (default 0)."
  );

#if IEEE_1666_SYSTEMC < 201101L
  xtsc_event_register(m_next_request, "m_next_request", this);
  xtsc_event_register(m_assert,       "m_assert",       this);
  xtsc_event_register(m_deassert,     "m_deassert",     this);
#endif

  log4xtensa::LogLevel ll = xtsc_get_constructor_log_level();
  XTSC_LOG(m_text, ll, "Constructed xtsc_queue_consumer '" << name() << "':");
  XTSC_LOG(m_text, ll, " nx                      = "   << boolalpha << m_nx);
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
  if (posedge_offset == 0xFFFFFFFF) {
  XTSC_LOG(m_text, ll, " posedge_offset          = 0xFFFFFFFF => " << m_posedge_offset.value() << " (" << m_posedge_offset << ")");
  } else {
  XTSC_LOG(m_text, ll, " posedge_offset          = "   << posedge_offset << " (" << m_posedge_offset << ")");
  }
  XTSC_LOG(m_text, ll, " sample_phase            = "   << sample_phase << " (" << m_sample_phase << ")");
  XTSC_LOG(m_text, ll, " deassert_delay          = "   << deassert_delay << " (" << m_deassert_delay << ")");
  XTSC_LOG(m_text, ll, " wraparound              = "   << boolalpha << m_wraparound);

  reset();

}



xtsc_component::xtsc_queue_consumer::~xtsc_queue_consumer(void) {
}



xtsc::u32 xtsc_component::xtsc_queue_consumer::get_bit_width(const string& port_name, u32 interface_num) const {
  if (m_pin_level) {
    if ((port_name == "m_pop"     )         ) return 1;
    if ((port_name == "m_data"    )         ) return m_width1;
    if ((port_name == "m_poppable") &&  m_nx) return 8;
    if ((port_name == "m_empty"   ) && !m_nx) return 1;
  }
  else {
    if (port_name == "m_queue") return m_width1;
  }
  ostringstream oss;
  oss << kind() << " \"" << name() << "\" has no port named \"" << port_name << "\"" << endl;
  throw xtsc_exception(oss.str());
}



sc_object *xtsc_component::xtsc_queue_consumer::get_port(const string& port_name) {
  if (m_pin_level) {
    if ((port_name == "m_pop"     )         ) return &m_pop;
    if ((port_name == "m_data"    )         ) return &m_data;
    if ((port_name == "m_poppable") &&  m_nx) return &m_poppable;
    if ((port_name == "m_empty"   ) && !m_nx) return &m_empty;
  }
  else {
    if (port_name == "m_queue") return &m_queue;
  }
  ostringstream oss;
  oss << kind() << " \"" << name() << "\" has no port named \"" << port_name << "\"" << endl;
  throw xtsc_exception(oss.str());
}



xtsc_port_table xtsc_component::xtsc_queue_consumer::get_port_table(const string& port_table_name) const {
  if ((port_table_name != "queue_pop") && (port_table_name != "queue")) {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\" has no port table named \"" << port_table_name << "\"" << endl;
    throw xtsc_exception(oss.str());
  }

  xtsc_port_table table;

  if (m_pin_level) {
              { table.push_back("m_pop");      }
              { table.push_back("m_data");     }
    if (m_nx) { table.push_back("m_poppable"); }
    else      { table.push_back("m_empty");    }
  }
  else {
    table.push_back("m_queue");
  }

  return table;
}



void xtsc_component::xtsc_queue_consumer::reset(bool /*hard_reset*/) {
  XTSC_INFO(m_text, "xtsc_queue_consumer::reset()");

  m_words.clear();
  m_line        = "";
  m_line_count  = 0;
  m_test_vector_stream.reset();

  // Cancel notified events
  m_next_request.cancel();
  m_assert      .cancel();
  m_deassert    .cancel();

  xtsc_reset_processes(m_process_handles);
}



void xtsc_component::xtsc_queue_consumer::execute(const string&                 cmd_line, 
                                                  const vector<string>&         words,
                                                  const vector<string>&         words_lc,
                                                  ostream&                      result)
{
  ostringstream res;

  if (false) {
  }
  else if (words[0] == "can_pop") {
    if (m_pin_level) {
      if (m_nx) { res << (m_poppable.read().to_uint() > 0); }
      else      { res << (m_empty.read() == m_zero_bv); }
    }
    else {
      res << m_queue->nb_can_pop();
    }
  }
  else if (words[0] == "get_pop_count") {
    res << m_pop_count;
  }
  else if (words[0] == "get_pop_data") {
    if (m_pin_level) {
      res << "0x" << m_value_bv.to_string(SC_HEX).substr(m_width1%4 ? 2 : 3);
    }
    else {
      res << "0x" << m_value.to_string(SC_HEX).substr(m_width1%4 ? 2 : 3);
    }
  }
  else if ((words[0] == "get_pop_ticket") && !m_pin_level) {
    res << m_ticket;
  }
  else if (words[0] == "nb_peek") {
    u64 ticket = 0;
    m_queue->nb_peek(m_peek_value, ticket);
    res << "0x" << m_peek_value.to_string(SC_HEX).substr(m_width1%4 ? 2 : 3);
  }
  else if (words[0] == "num_available_entries") {
    if (m_pin_level) {
      if (m_nx) { res << m_poppable.read().to_uint(); }
      else      { res << ((m_empty.read() == m_zero_bv) ? 1 : 0); }
    }
    else {
      res << m_queue->nb_num_available_entries();
    }
  }
  else if ((words[0] == "pop") && !m_pin_level) {
    if (m_queue->nb_pop(m_value, m_ticket)) {
      m_pop_count += 1;
      res << "0x" << m_value.to_string(SC_HEX).substr(m_width1%4 ? 2 : 3);
    }
    else {
      ostringstream oss;
      oss << name() << "::" << __FUNCTION__ << "() nb_pop() returned false - pop command failed.";
      throw xtsc_exception(oss.str());
    }
  }
  else if (words[0] == "reset") {
    reset((words.size() == 1) ? false : xtsc_command_argtobool(cmd_line, words, 1));
  }
  else {
    ostringstream oss;
    oss << name() << "::" << __FUNCTION__ << "() called for unknown command '" << cmd_line << "'.";
    throw xtsc_exception(oss.str());
  }

  result << res.str();
}



void xtsc_component::xtsc_queue_consumer::connect(xtsc_tx_loader& loader) {
  if (loader.pin_level() || m_pin_level) {
    ostringstream oss;
    oss << "xtsc_tx_loader '" << loader.name() << "' has \"pin_level\" " << boolalpha << loader.pin_level()
        << " and " << kind() << " '" << name() << "' has \"pin_level\" " << m_pin_level
        << ", but this connect() method does not support pin-level connections.";
    throw xtsc_exception(oss.str());
  }
  m_queue(*loader.m_consumer);
}



void xtsc_component::xtsc_queue_consumer::script_thread(void) {

  try {

    // Make sure any other threads have started
    wait(SC_ZERO_TIME);

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

      if (m_words[0] == "test_empty") {
        if (m_words.size() != 1) {
          ostringstream oss;
          oss << "TEST_EMPTY command has extra arguments:" << endl;
          oss << m_line;
          oss << m_test_vector_stream.info_for_exception();
          throw xtsc_exception(oss.str());
        }
        bool not_empty = (m_pin_level ? (m_nx ? (m_poppable.read().to_uint() != 0) : (m_empty.read() == m_zero_bv)) : m_queue->nb_can_pop());
        XTSC_INFO(m_text, "TEST_EMPTY: " << (not_empty ? "Not Empty" : "Empty"));
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

      if (m_words[0] != "now") {
          double delay = get_double(0, "delay");
          XTSC_DEBUG(m_text, "script_thread is delaying for " << delay << " clock periods");
          wait(delay * m_clock_period);
          XTSC_DEBUG(m_text, "script_thread done with delay");
      }

      if ((m_words.size() > 1) && (m_words[1] == "stop")) {
        XTSC_INFO(m_text, "script_thread calling sc_stop()");
        sc_stop();
        wait();
      }

      bool peek = ((m_words.size() > 1) && (m_words[1] == "peek"));
      double timeout = ((m_words.size() == (peek ? 2 : 1)) ? -1 : get_double((peek ? 2 : 1), "timeout"));
      bool no_timeout =  (timeout < 0) ? true : false;
      bool fall_through = false;

      // Skip if LX pin-level pop  
      if (m_nx || !m_pin_level || peek) {
        for (double count = 1; no_timeout || (count <= timeout); count += 1) {
          if (m_pin_level && ((m_nx && (m_poppable->read().to_uint() != 0)) || (!m_nx && (m_empty.read() == m_zero_bv)))) {
            if (peek) {
              m_value_bv = m_data.read();
              XTSC_INFO(m_text, "Peeked 0x" << m_value_bv.to_string(SC_HEX).substr(m_width1%4 ? 2 : 3));
            }
            else {
              timeout = 1;
              no_timeout = false;
              fall_through = true;
            }
            break;
          }
          else if (!m_pin_level && m_queue->nb_can_pop()) {
            if (peek) {
              m_queue->nb_peek(m_value, m_ticket);
              // a variable substr is used to get rid of the pesky extra leading bit
              XTSC_INFO(m_text, "Peeked 0x" << m_value.to_string(SC_HEX).substr(m_width1%4 ? 2 : 3) << " ticket=" << dec << m_ticket);
            }
            else {
              m_queue->nb_pop(m_value, m_ticket);
              m_pop_count += 1;
              // a variable substr is used to get rid of the pesky extra leading bit
              XTSC_INFO(m_text, "Popped 0x" << m_value.to_string(SC_HEX).substr(m_width1%4 ? 2 : 3) << " ticket=" << dec << m_ticket <<
                                " pop_count=" << m_pop_count);
            }
            break;
          }
          else {
            XTSC_DEBUG(m_text, "Cannot " << (peek ? "peek" : "pop") << " now (queue is empty)");
          }
          wait(m_clock_period);
        }
        if (!fall_through) continue;
      }
      // LX/NX pin-level interface pop
      XTSC_DEBUG(m_text, "script_thread is notifying m_assert");
      m_assert.notify();      // Immediate notification
      if (!no_timeout) {
        m_deassert.notify(timeout * m_clock_period);
      }
      XTSC_DEBUG(m_text, "script_thread is waiting for m_next_request event");
      wait(m_next_request);
      XTSC_DEBUG(m_text, "script_thread received m_next_request event");
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



void xtsc_component::xtsc_queue_consumer::sample_thread(void) {

  try {

    wait(m_sample_phase + m_posedge_offset);

    while (true) {
      if (m_pop.read() != m_zero_bv) {
        if (m_nx && (m_poppable.read().to_uint() == 0)) {
          XTSC_WARN(m_text, "Pop commanded to empty NX queue");
        }
        if (m_nx || (m_empty.read() == m_zero_bv)) {
          m_value_bv = m_data.read();
          m_pop_count += 1;
          XTSC_INFO(m_text, "Popped 0x" << m_value_bv.to_string(SC_HEX).substr(m_width1%4 ? 2 : 3) << " pop_count=" << dec << m_pop_count);
          m_deassert.notify(m_deassert_delay);
        }
      }

      wait(m_clock_period);
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



void xtsc_component::xtsc_queue_consumer::request_thread(void) {
  static bool warned_posedge_pop = false;

  try {

    m_pop.write(m_zero_bv);

    while (true) {
      XTSC_DEBUG(m_text, "request_thread waiting for m_assert event");
      wait(m_assert);
      XTSC_DEBUG(m_text, "request_thread received m_assert event");
      sc_time phase_now = (sc_time_stamp().value() % m_clock_period_value) * m_time_resolution;
      if ((phase_now == m_posedge_offset) && !warned_posedge_pop) {
        ostringstream oss;
        oss << "Pin-level " << kind() << " doing queue pop at posedge of clock which is not recommended";
        if (getenv("XTSC_QUEUE_CONSUMER_ALLOW_POSEDGE_POP")) {
          warned_posedge_pop = true;
          XTSC_WARN(m_text, oss.str());
        }
        else {
          throw xtsc_exception(oss.str());
        }
      }
      m_pop.write(m_one_bv);
      wait(m_deassert);
      XTSC_DEBUG(m_text, "request_thread received m_deassert event");
      m_pop.write(m_zero_bv);
      m_next_request.notify();  // Immediate notification
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



u32 xtsc_component::xtsc_queue_consumer::get_u32(u32 index, const string& argument_name) {
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



double xtsc_component::xtsc_queue_consumer::get_double(u32 index, const string& argument_name) {
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



void xtsc_component::xtsc_queue_consumer::end_of_elaboration() {
  u32  width1 = (m_pin_level ? m_data->read().length() : m_queue->nb_get_bit_width());
  if (width1 != m_width1) {
    ostringstream oss;
    oss << "Width mismatch ERROR: xtsc_queue_consumer '" << name() << "' has configured width of " << m_width1
        << " bits bound to queue interface (channel) of width " << width1 << ".";
    throw xtsc_exception(oss.str());
  }
}



