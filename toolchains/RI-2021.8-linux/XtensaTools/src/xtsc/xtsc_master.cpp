// Copyright (c) 2005-2018 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Cadence Design Systems, Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Cadence Design Systems, Inc.


#include <cctype>
#include <algorithm>
#include <xtsc/xtsc_master.h>
#include <xtsc/xtsc_core.h>
#include <xtsc/xtsc_mmio.h>
#include <xtsc/xtsc_wire_logic.h>

using namespace std;
#if SYSTEMC_VERSION >= 20050601
using namespace sc_core;
using namespace sc_dt;
#endif
using namespace xtsc;
using log4xtensa::INFO_LOG_LEVEL;
using log4xtensa::VERBOSE_LOG_LEVEL;



xtsc_component::xtsc_master::xtsc_master(sc_module_name module_name, const xtsc_master_parms& master_parms) :
  sc_module             (module_name),
  xtsc_module           (*(sc_module*)this),
  m_request_port        ("m_request_port"),
  m_respond_export      ("m_respond_export"),
  m_text                (log4xtensa::TextLogger::getInstance(name())),
  m_control             (master_parms.get_bool("control")),
  m_control_bound       (false),
  m_p_control           (NULL),
  m_p_write_impl        (NULL),
  m_control_value       (1),
  m_wraparound          (master_parms.get_bool  ("wraparound")),
  m_zero_delay_repeat   (master_parms.get_bool  ("zero_delay_repeat")),
  m_script_file         (master_parms.get_c_str ("script_file")),
  m_script_file_stream  (m_script_file.c_str(),  "script_file",  name(), kind(), m_wraparound),
  m_return_value_file   (""),
  m_p_return_value_file (0),
  m_format              (master_parms.get_non_zero_u32("format")),
  m_set_byte_enables    (false),
  m_do_dram_attribute   (false),
  m_do_pif_attribute    (false),
  m_do_domain           (false),
  m_do_cache            (false),
  m_do_prot             (false),
  m_do_snoop            (false),
  m_exclusive           (false),
  m_apb                 (false),
  m_burst               (xtsc_request::NON_AXI),
  m_dram_attribute      (160),
  m_pif_attribute       (0xFFFFFFFF),
  m_domain              (0),
  m_cache               (0),
  m_prot                (0),
  m_snoop               (0),
  m_byte_enables        (0),
#if IEEE_1666_SYSTEMC >= 201101L
  m_control_write_event ("m_control_write_event"),
  m_response_event      ("m_response_event"),
#endif
  m_p_port              (0)
{

  m_script_file = m_script_file_stream.name();

  m_response_history_depth = master_parms.get_u32("response_history_depth");

  if (m_control) {
    m_p_control = new wire_write_export("control");
    m_p_write_impl = new xtsc_wire_write_if_impl("control__impl", *this);
    (*m_p_control)(*m_p_write_impl);
  }
  m_control_value = 0;

  // Handle return value file
  const char *return_value_file = master_parms.get_c_str("return_value_file");
  if (return_value_file && return_value_file[0]) {
    m_p_return_value_file = new xtsc_script_file(return_value_file, "return_value_file",  name(), kind(), true);
    m_return_value_file   = m_p_return_value_file->name();
  }

  // Get clock period 
  m_time_resolution = sc_get_time_resolution();
  u32 clock_period = master_parms.get_u32("clock_period");
  if (clock_period == 0xFFFFFFFF) {
    m_clock_period = xtsc_get_system_clock_period();
  }
  else {
    m_clock_period = sc_get_time_resolution() * clock_period;
  }
  m_clock_period_value = m_clock_period.value();
  u32 posedge_offset = master_parms.get_u32("posedge_offset");
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

  SC_THREAD(request_thread);    m_process_handles.push_back(sc_get_current_process_handle());

  m_respond_export(*this);

  m_port_types["m_request_port"]   = REQUEST_PORT;
  m_port_types["m_respond_export"] = RESPOND_EXPORT;
  if (m_control) {
  m_port_types["control"]          = WIRE_WRITE_EXPORT;
  }
  m_port_types["master_port"]      = PORT_TABLE;

  xtsc_register_command(*this, *this, "dump_response_history", 0, 0,
      "dump_response_history", 
      "Return the os buffer from calling xtsc_master::dump_response_history(os)."
  );

  xtsc_register_command(*this, *this, "get_response", 1, 1,
      "get_response <N>", 
      "Return value from calling xtsc_master::get_response(<N>)."
  );

  xtsc_register_command(*this, *this, "get_response_history_count", 0, 0,
      "get_response_history_count", 
      "Return value from calling xtsc_master::get_response_history_count()."
  );

  xtsc_register_command(*this, *this, "get_response_history_depth", 0, 0,
      "get_response_history_depth", 
      "Return value from calling xtsc_master::get_response_history_depth()."
  );

  xtsc_register_command(*this, *this, "set_response_history_depth", 1, 1,
      "set_response_history_depth <Depth>", 
      "Call xtsc_master::set_response_history_depth(<Depth>)."
  );

#if IEEE_1666_SYSTEMC < 201101L
  xtsc_event_register(m_control_write_event, "m_control_write_event", this);
  xtsc_event_register(m_response_event,      "m_response_event",      this);
#endif

  log4xtensa::LogLevel ll = xtsc_get_constructor_log_level();
  XTSC_LOG(m_text, ll, "Constructed xtsc_master '" << name() << "':");
  XTSC_LOG(m_text, ll, " control                 = "   << boolalpha << m_control);
  XTSC_LOG(m_text, ll, " script_file             = "   << m_script_file);
  XTSC_LOG(m_text, ll, " wraparound              = "   << boolalpha << m_wraparound);
  XTSC_LOG(m_text, ll, " zero_delay_repeat       = "   << boolalpha << m_zero_delay_repeat);
  XTSC_LOG(m_text, ll, " return_value_file       = "   << m_return_value_file);
  XTSC_LOG(m_text, ll, " response_history_depth  = "   << m_response_history_depth);
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
  XTSC_LOG(m_text, ll, " format                  = "   << m_format);

  reset();

}



xtsc_component::xtsc_master::~xtsc_master(void) {
}



u32 xtsc_component::xtsc_master::get_bit_width(const string& port_name, u32 interface_num) const {
  return ((port_name == "control") ? 1 : 0);
}



sc_object *xtsc_component::xtsc_master::get_port(const string& port_name) {
       if (port_name == "m_request_port")       return &m_request_port;
  else if (port_name == "m_respond_export")     return &m_respond_export;
  else if (port_name == "control" && m_control) return m_p_control;
  else {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\" has no port named \"" << port_name << "\"" << endl;
    throw xtsc_exception(oss.str());
  }
}



xtsc_port_table xtsc_component::xtsc_master::get_port_table(const string& port_table_name) const {

  if (port_table_name != "master_port") {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\" has no port table named \"" << port_table_name << "\"" << endl;
    throw xtsc_exception(oss.str());
  }

  xtsc_port_table table;
  table.push_back("m_request_port");
  table.push_back("m_respond_export");

  return table;
}



void xtsc_component::xtsc_master::reset(bool /*hard_reset*/) {
  XTSC_INFO(m_text, "xtsc_master::reset()");

  m_line                        = "";
  m_return_value_line           = "";
  m_line_count                  = 0;
  m_return_value_line_count     = 0;
  m_return_value_index          = 0;
  m_block_write_tag             = 0L;
  m_burst_write_tag             = 0L;
  m_rcw_tag                     = 0L;
  m_last_request_tag            = 0L;
  m_last_request_got_response   = false;
  m_virtual_address_delta       = 0;
  m_last_request_got_nacc       = false;
  m_last_response_status        = xtsc_response::RSP_OK;
  m_fetch                       = false;
  m_use_coherent_peek_poke      = false;
  m_set_xfer_en                 = false;
  m_set_user_data               = false;
  m_set_last_transfer           = false;
  m_last_transfer               = false;
  m_do_dram_attribute           = false;
  m_do_pif_attribute            = false;
  m_do_domain                   = false;
  m_do_cache                    = false;
  m_do_prot                     = false;
  m_do_snoop                    = false;
  m_exclusive                   = false;
  m_dram_attribute              = 0;
  m_pif_attribute               = 0xFFFFFFFF;
  m_domain                      = 0;
  m_cache                       = 0;
  m_prot                        = 0;
  m_snoop                       = 0;

  m_words.clear();
  m_return_values.clear();

  m_script_file_stream.reset();

  if (m_p_return_value_file) {
    m_p_return_value_file->reset();
  }

  // Cancel notified events
  m_control_write_event.cancel();
  m_response_event     .cancel();

  xtsc_reset_processes(m_process_handles);
}



void xtsc_component::xtsc_master::set_response_history_depth(u32 depth) {
  if (m_response_history.size() > depth) {
    m_response_history.erase(m_response_history.begin()+depth, m_response_history.end());
  }
  m_response_history_depth = depth;
}



u32 xtsc_component::xtsc_master::get_response_history_depth() const {
  return m_response_history_depth;
}



u32 xtsc_component::xtsc_master::get_response_history_count() const {
  return m_response_history.size();
}



const string& xtsc_component::xtsc_master::get_response(u32 n) {
  u32 cnt = m_response_history.size();
  if ((n < 1) || (n > cnt)) {
    ostringstream oss;
    oss << "xtsc_master '" << name() << "' get_response(n) called with n=" << n;
    if (n == 0) { oss << " but response history table is indexed starting at 1."; }
    else        { oss << " but response history table has " << cnt << ((cnt==1) ? " entry." : " entries."); }
    throw xtsc_exception(oss.str());
  }
  return m_response_history[n-1];
}



void xtsc_component::xtsc_master::dump_response_history(std::ostream &os) {
  u32 n = 1;
  for (deque<string>::const_iterator i = m_response_history.begin(); i < m_response_history.end(); ++i, ++n) {
    os << n << ": " << *i << endl;
  }
}



void xtsc_component::xtsc_master::execute(const string&          cmd_line, 
                                          const vector<string>&  words,
                                          const vector<string>&  words_lc,
                                          ostream&               result)
{
  ostringstream res;

  if (false) {
  }
  else if (words[0] == "dump_response_history") {
    dump_response_history(res);
  }
  else if (words[0] == "get_response") {
    res << get_response(xtsc_command_argtou32(cmd_line, words, 1));
  }
  else if (words[0] == "get_response_history_count") {
    res << get_response_history_count();
  }
  else if (words[0] == "get_response_history_depth") {
    res << get_response_history_depth();
  }
  else if (words[0] == "set_response_history_depth") {
    set_response_history_depth(xtsc_command_argtou32(cmd_line, words, 1));
  }
  else {
    ostringstream oss;
    oss << "xtsc_master::execute called for unknown command '" << cmd_line << "'.";
    throw xtsc_exception(oss.str());
  }

  result << res.str();
}



sc_export<xtsc_wire_write_if>& xtsc_component::xtsc_master::get_control_input() const {
  if (!m_control) {
    ostringstream oss;
    oss << "xtsc_master '" << name() << "' has \"control\" false, so get_control_input() should not be called.";
    throw xtsc_exception(oss.str());
  }
  return *m_p_control;
}



void xtsc_component::xtsc_master::connect(xtsc_wire_logic& logic, const char *output_name) {
  if (!m_control) {
    ostringstream oss;
    oss << "'" << name() << "' has \"control\" false, so xtsc_master::connect(xtsc_wire_logic&, ...) should not be called.";
    throw xtsc_exception(oss.str());
  }
  u32 wo = logic.get_bit_width(output_name);
  u32 wi = 1;
  if (wo != wi) {
    ostringstream oss;
    oss << "Width mismatch:  cannot connect " << wo << "-bit output '" << output_name << "' of xtsc_wire_logic '" << logic.name()
        << "' to " << wi << "-bit control input of xtsc_master '" << name() << "'";
    throw xtsc_exception(oss.str());
  }
  logic.get_output(output_name)(*m_p_control);
}



void xtsc_component::xtsc_master::connect(xtsc_mmio& mmio, const char *output_name) {
  if (!m_control) {
    ostringstream oss;
    oss << "'" << name() << "' has \"control\" false, so xtsc_master::connect(xtsc_mmio&, ...) should not be called.";
    throw xtsc_exception(oss.str());
  }
  u32 wo = mmio.get_bit_width(output_name);
  u32 wi = 1;
  if (wo != wi) {
    ostringstream oss;
    oss << "Width mismatch:  cannot connect " << wo << "-bit output '" << output_name << "' of xtsc_mmio '" << mmio.name()
        << "' to " << wi << "-bit control input of xtsc_master '" << name() << "'";
    throw xtsc_exception(oss.str());
  }
  mmio.get_output(output_name)(*m_p_control);
}



void xtsc_component::xtsc_master::connect(xtsc_core& core, const char *port) {
  m_request_port(core.get_request_export(port));
  core.get_respond_port(port)(m_respond_export);
}



bool xtsc_component::xtsc_master::nb_respond(const xtsc_response& response) {
  bool return_value = true;
  xtsc_response::status_t status = response.get_status();
  if (status == xtsc_response::RSP_NACC) {
    m_last_response_status = status;
    if (response.get_tag() == m_last_request_tag) {
      m_last_request_got_response = true;
      m_last_request_got_nacc = true;
    }
  }
  else {
    return_value = get_return_value();
    if (return_value) {
      m_last_response_status = status;
      if (response.get_tag() == m_last_request_tag) {
        m_last_request_got_response = true;
      }
    }
    if (m_response_history_depth) {
      ostringstream oss;
      oss << response;
      m_response_history.push_front(oss.str());
      set_response_history_depth(m_response_history_depth);
    }
  }

  if (return_value) {
    m_response_event.notify(SC_ZERO_TIME);
  }

  XTSC_INFO(m_text, response << (return_value ? "" : " (nb_respond returning false)"));
  return return_value;
}



void xtsc_component::xtsc_master::request_thread(void) {

  try {

    xtsc_request request;
    double       delay             = 0;
    double       prev_req_delay    = 0;
    bool         prev_req_do_delay = false;
    sc_time      request_time      = SC_ZERO_TIME;
    u32          n                 = xtsc_address_nibbles();

    while (get_words() != 0) {

      XTSC_DEBUG(m_text, "\"script_file\" line #" << m_line_count << ": " << m_line);

      bool send     = true;
      bool poke     = false;
      bool peek     = false;
      bool stop     = false;
      bool lock     = false;
      bool lock_arg = false;
      bool retire   = false;
      bool flush    = false;
      bool is_req   = false;
      bool do_delay = (m_words[0] != "now");
      xtsc_address address8 = 0;
      u32          size = 0;

      if (m_words[0] == "wait") {
        send     = false;
        do_delay = false;
        if (m_words.size() < 2) {
          ostringstream oss;
          oss << "WAIT command is missing arguments: " << endl;
          oss << m_line;
          oss << m_script_file_stream.info_for_exception();
          throw xtsc_exception(oss.str());
        }
        if ((m_words[1] == "rsp") || (m_words[1] == "respond") || (m_words[1] == "response") || (m_words[1] == "tag")) {
          bool is_wait_tag = (m_words[1] == "tag");
          bool has_repeat_value = (m_words.size() > 2);
          u32 repeat = 0xFFFFFFFF;
          if (has_repeat_value) {
            repeat = get_u32(2, "repeat");
          }
          if (is_wait_tag) {
            XTSC_DEBUG(m_text, "waiting for response with tag=" << request.get_tag());
          }
          else {
            XTSC_DEBUG(m_text, "waiting for any response");
          }
          do {
            wait(m_response_event);
          } while (is_wait_tag && !m_last_request_got_response);
          if (repeat && (m_last_response_status == xtsc_response::RSP_NACC)) {
            do {
              // delay if previous request command had a delay
              if (prev_req_do_delay) {
                sc_time retry_delay = prev_req_delay * m_clock_period;
                XTSC_DEBUG(m_text, "Got RSP_NACC. Waiting before retry: " << retry_delay);
                wait(retry_delay);
              }
              else if (!m_zero_delay_repeat && (request_time == sc_time_stamp())) {
                wait(m_clock_period);
              }
              ostringstream oss;
              request.dump(oss, true);
              XTSC_INFO(m_text, oss.str());
              m_last_request_got_response = false;
              m_last_request_got_nacc = false;
              request_time = sc_time_stamp();
              m_request_port->nb_request(request);
              if (is_wait_tag) {
                XTSC_DEBUG(m_text, "waiting for response with tag=" << request.get_tag() << " again");
              }
              else {
                XTSC_DEBUG(m_text, "waiting for any response again");
              }
              do {
                wait(m_response_event);
              } while (is_wait_tag && !m_last_request_got_response);
              if (has_repeat_value) {
                repeat -= 1;
              }
            } while (repeat && (m_last_response_status == xtsc_response::RSP_NACC));
          }
        }
        else if ((m_words[1] == "nacc") || (m_words[1] == "rsp_nacc")) {
          bool has_timeout_value = (m_words.size() > 2);
          double timeout_factor = 1.0;
          if (has_timeout_value) {
            timeout_factor = get_double(2, "timeout");
          }
          bool has_repeat_value = (m_words.size() > 3);
          u32 repeat = 0xFFFFFFFF;
          if (has_repeat_value) {
            repeat = get_u32(3, "repeat");
          }
          sc_time timeout = timeout_factor * m_clock_period;
          XTSC_DEBUG(m_text, "waiting for response with timeout: " << timeout);
          wait(timeout);
          if (repeat && m_last_request_got_nacc) {
            do {
              // delay if previous request command had a delay
              if (prev_req_do_delay) {
                sc_time retry_delay = prev_req_delay * m_clock_period;
                if (retry_delay > timeout) {
                  retry_delay -= timeout;
                  XTSC_DEBUG(m_text, "Got RSP_NACC. Waiting before retry: " << retry_delay);
                  wait(retry_delay);
                }
              }
              ostringstream oss;
              request.dump(oss, true);
              XTSC_INFO(m_text, oss.str());
              m_last_request_got_response = false;
              m_last_request_got_nacc = false;
              request_time = sc_time_stamp();
              m_request_port->nb_request(request);
              XTSC_DEBUG(m_text, "waiting again for response with timeout: " << timeout);
              wait(timeout);
              if (has_repeat_value) {
                repeat -= 1;
              }
            } while (repeat && m_last_request_got_nacc);
          }
        }
        else if (m_words[1] == "control") {
          if (!m_control) {
            ostringstream oss;
            oss << "WAIT CONTROL command cannot be used unless the \"control\" parameter is set to true:" << endl;
            oss << m_line;
            oss << m_script_file_stream.info_for_exception();
            throw xtsc_exception(oss.str());
          }
          if (!m_control_bound) {
            ostringstream oss;
            oss << "WAIT CONTROL command cannot be used unless something is connected to the control input:" << endl;
            oss << m_line;
            oss << m_script_file_stream.info_for_exception();
            throw xtsc_exception(oss.str());
          }
          if (m_words.size() <= 4) {
            u32 count = 1;
            if (m_words.size() == 4) {
              count = get_u32(3, "count");
            }
            if ((m_words.size() == 2) || (m_words[2] == "write")) {
              u32 target_write_count = m_control_write_count + count;
              while (target_write_count != m_control_write_count) {
                wait(m_control_write_event);
              }
            }
            else if (m_words[2] == "change") {
              u32 target_change_count = m_control_change_count + count;
              while (target_change_count != m_control_change_count) {
                wait(m_control_write_event);
              }
            }
            else {
              u32 value = get_u32(2, "value");
              if (value > 1) {
                ostringstream oss;
                oss << "value = " << value << " is not allowed (must be 0 or 1) in command:" << endl;
                oss << m_line;
                oss << m_script_file_stream.info_for_exception();
                throw xtsc_exception(oss.str());
              }
              if (m_control_value.to_uint() == value) { count -= 1; }
              while (count) {
                wait(m_control_write_event);
                if (m_control_value.to_uint() == value) { count -= 1; }
              }
            }
          }
        }
        else {
          double time = get_double(1, "duration");
          sc_time duration = time * m_clock_period;
          XTSC_DEBUG(m_text, "waiting " << duration);
          wait(duration);
        }
      }
      else if ((m_words[0] == "sync") || (m_words[0] == "synchronize")) {
        send     = false;
        do_delay = false;
        if (m_words.size() != 2) {
          ostringstream oss;
          oss << "SYNC command has missing/extra arguments: " << endl;
          oss << m_line;
          oss << m_script_file_stream.info_for_exception();
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
      }
      else if (m_words[0] == "fetch") {
        send     = false;
        do_delay = false;
        if ((m_words.size() != 2) || ((m_words[1] != "on") && (m_words[1] != "off"))) {
          ostringstream oss;
          oss << "Syntax error (expected FETCH ON|OFF): " << endl;
          oss << m_line;
          oss << m_script_file_stream.info_for_exception();
          throw xtsc_exception(oss.str());
        }
        m_fetch = (m_words[1] == "on");
      }
      else if (m_words[0] == "coherent") {
        send     = false;
        do_delay = false;
        if ((m_words.size() != 2) || ((m_words[1] != "on") && (m_words[1] != "off"))) {
          ostringstream oss;
          oss << "Syntax error (expected COHERENT ON|OFF): " << endl;
          oss << m_line;
          oss << m_script_file_stream.info_for_exception();
          throw xtsc_exception(oss.str());
        }
        m_use_coherent_peek_poke = (m_words[1] == "on");
      }
      else if (m_words[0] == "dram_attribute") {
        send     = false;
        do_delay = false;
        if (m_words.size() != 2) {
          ostringstream oss;
          oss << "Syntax error (expected DRAM_ATTRIBUTE attr|OFF): " << endl;
          oss << m_line;
          oss << m_script_file_stream.info_for_exception();
          throw xtsc_exception(oss.str());
        }
        if (m_words[1] == "off") {
          m_do_dram_attribute = false;
        }
        else {
          m_do_dram_attribute = true;
          try {
            m_dram_attribute = m_words[1].c_str();
          }
          catch (const xtsc_exception&) {
            ostringstream oss;
            oss << "Cannot convert DRAM_ATTRIBUTE argument (#2) '" << m_words[1] << "' to sc_unsigned in:" << endl;
            oss << m_line;
            oss << m_script_file_stream.info_for_exception();
            throw xtsc_exception(oss.str());
          }
        }
      }
      else if ((m_words[0] == "pif_attribute") || (m_words[0] == "attribute")) {
        send     = false;
        do_delay = false;
        if (m_words.size() != 2) {
          ostringstream oss;
          oss << "Syntax error (expected PIF_ATTRIBUTE attr|OFF): " << endl;
          oss << m_line;
          oss << m_script_file_stream.info_for_exception();
          throw xtsc_exception(oss.str());
        }
        if (m_words[1] == "off") {
          m_do_pif_attribute = false;
        }
        else {
          m_do_pif_attribute = true;
          m_pif_attribute = get_u32(1, "attr");
        }
      }
      else if ((m_words[0] == "pif_domain") || (m_words[0] == "domain")) {
        send     = false;
        do_delay = false;
        if (m_words.size() != 2) {
          ostringstream oss;
          oss << "Syntax error (expected DOMAIN domain|OFF): " << endl;
          oss << m_line;
          oss << m_script_file_stream.info_for_exception();
          throw xtsc_exception(oss.str());
        }
        if (m_words[1] == "off") {
          m_do_domain = false;
        }
        else {
          m_do_domain = true;
          m_domain = (u8) get_u32(1, "domain");
        }
      }
      else if (m_words[0] == "xfer_en") {
        send     = false;
        do_delay = false;
        if ((m_words.size() != 2) || ((m_words[1] != "on") && (m_words[1] != "off"))) {
          ostringstream oss;
          oss << "Syntax error (expected XFER_EN ON|OFF): " << endl;
          oss << m_line;
          oss << m_script_file_stream.info_for_exception();
          throw xtsc_exception(oss.str());
        }
        m_set_xfer_en = (m_words[1] == "on");
      }
      else if (m_words[0] == "exclusive") {
        send     = false;
        do_delay = false;
        if ((m_words.size() != 2) || ((m_words[1] != "on") && (m_words[1] != "off"))) {
          ostringstream oss;
          oss << "Syntax error (expected EXCLUSIVE ON|OFF): " << endl;
          oss << m_line;
          oss << m_script_file_stream.info_for_exception();
          throw xtsc_exception(oss.str());
        }
        m_exclusive = (m_words[1] == "on");
      }
      else if (m_words[0] == "apb") {
        send     = false;
        do_delay = false;
             if ((m_words[1] == "0") || (m_words[1] == "f") || (m_words[1] == "false") || (m_words[1] == "off")) {
          m_apb = false;
        }
        else if ((m_words[1] == "1") || (m_words[1] == "t") || (m_words[1] == "true" ) || (m_words[1] == "on" )) {
          m_apb = true;
        }
        else {
          ostringstream oss;
          oss << "Syntax error (expected APB ON|OFF): " << endl;
          oss << m_line;
          oss << m_script_file_stream.info_for_exception();
          throw xtsc_exception(oss.str());
        }
      }
      else if (m_words[0] == "burst") {
        send     = false;
        do_delay = false;
        bool okay = true;
        if (m_words.size() == 2) {
               if (m_words[1] == "pif"  ) { m_burst = xtsc_request::NON_AXI; }
          else if (m_words[1] == "fixed") { m_burst = xtsc_request::FIXED;   }
          else if (m_words[1] == "fixd" ) { m_burst = xtsc_request::FIXED;   }
          else if (m_words[1] == "wrap" ) { m_burst = xtsc_request::WRAP;    }
          else if (m_words[1] == "incr" ) { m_burst = xtsc_request::INCR;    }
          else { okay = false; }
        }
        else { okay = false; }
        if (!okay) {
          ostringstream oss;
          oss << "Syntax error (expected BURST PIF|FIXED|WRAP|INCR)" << endl;
          oss << m_line;
          oss << m_script_file_stream.info_for_exception();
          throw xtsc_exception(oss.str());
        }
      }
      else if (m_words[0] == "cache") {
        send     = false;
        do_delay = false;
        if (m_words.size() != 2) {
          ostringstream oss;
          oss << "Syntax error (expected CACHE cache|OFF): " << endl;
          oss << m_line;
          oss << m_script_file_stream.info_for_exception();
          throw xtsc_exception(oss.str());
        }
        if (m_words[1] == "off") {
          m_do_cache = false;
        }
        else {
          m_do_cache = true;
          m_cache = (u8) get_u32(1, "cache");
        }
      }
      else if (m_words[0] == "prot") {
        send     = false;
        do_delay = false;
        if (m_words.size() != 2) {
          ostringstream oss;
          oss << "Syntax error (expected PROT prot|OFF): " << endl;
          oss << m_line;
          oss << m_script_file_stream.info_for_exception();
          throw xtsc_exception(oss.str());
        }
        if (m_words[1] == "off") {
          m_do_prot = false;
        }
        else {
          m_do_prot = true;
          m_prot = (u8) get_u32(1, "prot");
        }
      }
      else if (m_words[0] == "snoop") {
        send     = false;
        do_delay = false;
        if (m_words.size() != 2) {
          ostringstream oss;
          oss << "Syntax error (expected SNOOP snoop|MUEvict|CaMx_CS|CaMx_CI|CaMx_MI|OFF): " << endl;
          oss << m_line;
          oss << m_script_file_stream.info_for_exception();
          throw xtsc_exception(oss.str());
        }
        if (m_words[1] == "off") {
          m_do_snoop = false;
        }
        else {
          m_do_snoop = true;
               if (m_words[1] == "muevict") m_snoop = xtsc_request::MUEvict;
          else if (m_words[1] == "camx_cs") m_snoop = xtsc_request::CaMx_CS;
          else if (m_words[1] == "camx_ci") m_snoop = xtsc_request::CaMx_CI;
          else if (m_words[1] == "camx_mi") m_snoop = xtsc_request::CaMx_MI;
          else                              m_snoop = (u8) get_u32(1, "snoop");
        }
      }
      else if (m_words[0] == "user_data") {
        send     = false;
        do_delay = false;
        if (m_words.size() != 2) {
          ostringstream oss;
          oss << "Syntax error (expected USER_DATA value|OFF): " << endl;
          oss << m_line;
          oss << m_script_file_stream.info_for_exception();
          throw xtsc_exception(oss.str());
        }
        m_set_user_data = (m_words[1] != "off");
        if (m_set_user_data) {
#if defined(__LP64__) || defined(_WIN64)
            m_user_data =  (void*) get_u64(1, "value");
#else
            m_user_data =  (void*) get_u32(1, "value");
#endif
        }
      }
      else if (m_words[0] == "last_transfer") {
        send     = false;
        do_delay = false;
        bool found = false;
        if (m_words.size() == 2) {
               if ((m_words[1] == "0") || (m_words[1] == "f") || (m_words[1] == "false")) {
            m_set_last_transfer = true;
            m_last_transfer     = false;
            found               = true;
          }
          else if ((m_words[1] == "1") || (m_words[1] == "t") || (m_words[1] == "true" )) {
            m_set_last_transfer = true;
            m_last_transfer     = true;
            found               = true;
          }
          else if (m_words[1] == "off") {
            m_set_last_transfer = false;
            m_last_transfer     = false;
            found               = true;
          }
        }
        if (!found) {
          ostringstream oss;
          oss << "Syntax error (expected LAST_TRANSFER 0|1|OFF): " << endl;
          oss << m_line;
          oss << m_script_file_stream.info_for_exception();
          throw xtsc_exception(oss.str());
        }
      }
      else if (m_words[0] == "format") {
        send     = false;
        do_delay = false;
        if ((m_words.size() != 2) || ((m_words[1] != "1") && (m_words[1] != "2") && (m_words[1] != "3"))) {
          ostringstream oss;
          oss << "Syntax error (expected FORMAT 1|2|3): " << endl;
          oss << m_line;
          oss << m_script_file_stream.info_for_exception();
          throw xtsc_exception(oss.str());
        }
        m_format = ((m_words[1] == "1") ? 1 : ((m_words[1] == "2") ? 2 : 3));
      }
      else if (m_words[0] == "be") {
        send     = false;
        do_delay = false;
        if (m_words.size() != 2) {
          ostringstream oss;
          oss << "Syntax error (expected BE byte_enables|OFF): " << endl;
          oss << m_line;
          oss << m_script_file_stream.info_for_exception();
          throw xtsc_exception(oss.str());
        }
        if (m_words[1] == "off") {
          m_byte_enables     = 0;
          m_set_byte_enables = false;
        }
        else if (m_words.size() == 2) {
          m_byte_enables     = get_u64(1, "byte_enables");
          m_set_byte_enables = true;
        }
      }
      else if (m_words[0] == "info") {
        send     = false;
        do_delay = false;
        XTSC_INFO(m_text, m_line);
      }
      else if (m_words[0] == "note") {
        send     = false;
        do_delay = false;
        XTSC_NOTE(m_text, m_line);
      }
      else if (m_words[0] == "virtual") {
        send     = false;
        do_delay = false;
        if (m_words.size() != 2) {
          ostringstream oss;
          oss << "Syntax error (expected VIRTUAL addr_delta): " << endl;
          oss << m_line;
          oss << m_script_file_stream.info_for_exception();
          throw xtsc_exception(oss.str());
        }
        m_virtual_address_delta = get_u64(1, "addr_delta");
      }
      else {
        if (m_words.size() == 1) {
          ostringstream oss;
          oss << "Unrecognized command or command line has too few words: " << endl;
          oss << m_line;
          oss << m_script_file_stream.info_for_exception();
          throw xtsc_exception(oss.str());
        }
        if (m_words[1] == "stop") {
          // delay STOP
          send = false;
          stop = true;
        }
        else if (m_words[1] == "lock") {
          // delay LOCK lock
          if (m_words.size() < 3) {
            ostringstream oss;
            oss << "Line has too few words: " << endl;
            oss << m_line;
            oss << m_script_file_stream.info_for_exception();
            throw xtsc_exception(oss.str());
          }
          if ((m_words[2] == "true") || (m_words[2] == "t") || (m_words[2] == "1") || (m_words[2] == "on")) {
            lock_arg = true;
          }
          else if ((m_words[2] == "false") || (m_words[2] == "f") || (m_words[2] == "0") || (m_words[2] == "off")) {
            lock_arg = false;
          }
          send = false;
          lock = true;
        }
        else if (m_words[1] == "retire") {
          // delay RETIRE address
          if (m_words.size() < 3) {
            ostringstream oss;
            oss << "Line has too few words: " << endl;
            oss << m_line;
            oss << m_script_file_stream.info_for_exception();
            throw xtsc_exception(oss.str());
          }
          address8 = get_u64(2, "address");
          send = false;
          retire = true;
        }
        else if (m_words[1] == "flush") {
          // delay FLUSH
          send = false;
          flush = true;
        }
        else {
          if ((m_words[1] != "poke")        &&
              (m_words[1] != "peek")        &&
              (m_words[1] != "read")        &&
              (m_words[1] != "block_read")  &&
              (m_words[1] != "burst_read")  &&
              (m_words[1] != "write")       &&
              (m_words[1] != "block_write") &&
              (m_words[1] != "burst_write") &&
              (m_words[1] != "rcw")         &&
              (m_words[1] != "snoop"))
          {
            ostringstream oss;
            oss << "Unrecognized command '" << m_words[0] << "' or unrecognized request type '" << m_words[1] << "':" << endl;
            oss << m_line;
            oss << m_script_file_stream.info_for_exception();
            throw xtsc_exception(oss.str());
          }
          address8 = get_u64(2, "address");
          size     = get_u32(3, "size");
          if (m_words[1] == "poke") {
            // delay POKE address size b0 b1 ... bN
            set_buffer(4, size, true);
            send = false;
            poke = true;
          }
          else if (m_words[1] == "peek") {
            // delay PEEK address size 
            send = false;
            peek = true;
          }
          else if (m_words[1] == "read") {
            // 0     1    2       3    4        5  6        7  8
            // delay READ address size route_id id priority pc
            // delay READ address size route_id id priority pc coh
            u32          route_id = get_u32(4, "route_id");
            u8           id       = static_cast<u8>(get_u32(5, "id"));
            u8           priority = static_cast<u8>(get_u32(6, "priority"));
            xtsc_address pc       = get_u32(7, "pc");
            u32          offset   = (m_format > 1) ? 1 : 0;
            xtsc_byte_enables be  = (0xFFFFFFFFFFFFFFFFull >> (64 - size));
            request.initialize(xtsc_request::READ, address8, size, 0, 1, be , true, route_id, id, priority, pc, xtsc_request::NON_AXI, m_apb);
            request.set_instruction_fetch(m_fetch);
            check_for_too_many_parameters(8 + offset);
            is_req = true;
          }
          else if (m_words[1] == "block_read") {
            // 0     1          2       3    4        5  6        7  8         9
            // delay BLOCK_READ address size route_id id priority pc num_xfers 
            // delay BLOCK_READ address size route_id id priority pc coh       num_xfers 
            u32          route_id      = get_u32(4, "route_id");
            u8           id            = static_cast<u8>(get_u32(5, "id"));
            u8           priority      = static_cast<u8>(get_u32(6, "priority"));
            xtsc_address pc            = get_u32(7, "pc");
            u32          offset        = (m_format > 1) ? 1 : 0;
            u32          num_xfers     = get_u32(8+offset, "num_xfers");
            request.initialize(xtsc_request::BLOCK_READ, address8, size, 0, num_xfers, 0, true, route_id, id, priority, pc);
            request.set_instruction_fetch(m_fetch);
            check_for_too_many_parameters(9 + offset);
            is_req = true;
          }
          else if (m_words[1] == "burst_read") {
            // 0     1          2       3    4        5  6        7  8         9
            // delay BURST_READ address size route_id id priority pc num_xfers 
            // delay BURST_READ address size route_id id priority pc coh       num_xfers 
            u32          route_id      = get_u32(4, "route_id");
            u8           id            = static_cast<u8>(get_u32(5, "id"));
            u8           priority      = static_cast<u8>(get_u32(6, "priority"));
            xtsc_address pc            = get_u32(7, "pc");
            u32          offset        = (m_format > 1) ? 1 : 0;
            u32          num_xfers     = get_u32(8+offset, "num_xfers");
            request.initialize(xtsc_request::BURST_READ, address8, size, 0, num_xfers, (0xFFFFFFFFFFFFFFFFull >> (64 - size)), true,
                               route_id, id, priority, pc, m_burst);
            check_for_too_many_parameters(9 + offset);
            is_req = true;
          }
          else if (m_words[1] == "write") {
            // 0     1     2       3    4        5  6        7  8            9            10
            // delay WRITE address size route_id id priority pc byte_enables b0           b1 ... bN
            // delay WRITE address size route_id id priority pc coh          byte_enables b0 b1 ... bN
            u32               route_id = get_u32(4, "route_id");
            u8                id       = static_cast<u8>(get_u32(5, "id"));
            u8                priority = static_cast<u8>(get_u32(6, "priority"));
            xtsc_address      pc       = get_u32(7, "pc");
            u32               offset   = (m_format > 1) ? 1 : 0;
            xtsc_byte_enables b        = get_u64(8+offset, "byte_enables");
            request.initialize(xtsc_request::WRITE, address8, size, 0, 1, b, true, route_id, id, priority, pc, xtsc_request::NON_AXI, m_apb);
            set_buffer(request, 9+offset, size);
            check_for_too_many_parameters(9+offset+size);
            check_for_too_few_parameters(9+offset+size);
            is_req = true;
          }
          else if ((m_words[1] == "block_write") && (m_format <= 2)) {
            // 0     1           2       3    4        5  6        7  8         8         9          10         11 
            // delay BLOCK_WRITE address size route_id id priority pc num_xfers last_xfer first_xfer b0         b1 ... bN
            // delay BLOCK_WRITE address size route_id id priority pc coh       num_xfers last_xfer  first_xfer b0 b1 ... bN
            u32          route_id      = get_u32(4, "route_id");
            u8           id            = static_cast<u8>(get_u32(5, "id"));
            u8           priority      = static_cast<u8>(get_u32(6, "priority"));
            xtsc_address pc            = get_u32(7, "pc");
            u32          offset        = (m_format > 1) ? 1 : 0;
            u32          num_xfers     = get_u32(8+offset, "num_xfers");
            bool         last          = (get_u32(9+offset, "last_xfer") != 0);
            bool         first         = (get_u32(10+offset, "first_xfer") != 0);
            if (first) {
              request.initialize(xtsc_request::BLOCK_WRITE, address8, size, 0, num_xfers, 0, last, route_id, id, priority, pc);
              m_block_write_tag = request.get_tag();
            }
            else {
              request.initialize(m_block_write_tag, address8, size, num_xfers, last, route_id, id, priority, pc);
            }
            if (m_set_byte_enables) {
              request.set_byte_enables(m_byte_enables);
            }
            set_buffer(request, 11+offset, size);
            check_for_too_many_parameters(11+offset+size);
            check_for_too_few_parameters(11+offset+size);
            is_req = true;
          }
          else if ((m_words[1] == "block_write") && (m_format >= 3)) {
            // 0     1           2       3    4        5  6        7  8         9         10           11           12         13
            // delay BLOCK_WRITE address size route_id id priority pc coh       num_xfers xfer_num     byte_enables hw_address b0 b1...bN
            u32                 route_id      = get_u32(4, "route_id");
            u8                  id            = static_cast<u8>(get_u32(5, "id"));
            u8                  priority      = static_cast<u8>(get_u32(6, "priority"));
            xtsc_address        pc            = get_u32(7, "pc");
            u32                 num_xfers     = get_u32(9, "num_xfers");
            u32                 xfer_num      = get_u32(10, "xfer_num");
            xtsc_byte_enables   b             = get_u64(11, "byte_enables");
            xtsc_address        hw_address    = get_u64(12, "hw_address");
            bool                last          = (xfer_num == num_xfers);
            bool                first         = (xfer_num == 1);
            if (first) {
              request.initialize(xtsc_request::BLOCK_WRITE, address8, size, 0, num_xfers, 0, last, route_id, id, priority, pc);
              m_block_write_tag = request.get_tag();
            }
            else {
              request.initialize(m_block_write_tag, address8, size, num_xfers, last, route_id, id, priority, pc);
            }
            request.set_byte_enables(b);
            request.adjust_block_write(hw_address, xfer_num);
            set_buffer(request, 13, size);
            check_for_too_many_parameters(13+size);
            check_for_too_few_parameters(13+size);
            is_req = true;
          }
          else if (m_words[1] == "burst_write") {
            // 0     1           2       3    4        5  6        7  8         9         10           11           12         13
            // delay BURST_WRITE address size route_id id priority pc num_xfers xfer_num  byte_enables hw_address   b0         b1 ... bN
            // delay BURST_WRITE address size route_id id priority pc coh       num_xfers xfer_num     byte_enables hw_address b0 b1...bN
            u32                 route_id        = get_u32(4, "route_id");
            u8                  id              = static_cast<u8>(get_u32(5, "id"));
            u8                  priority        = static_cast<u8>(get_u32(6, "priority"));
            xtsc_address        pc              = get_u32(7, "pc");
            u32                 offset          = (m_format > 1) ? 1 : 0;
            u32                 num_xfers       = get_u32(8+offset, "num_xfers");
            u32                 xfer_num        = get_u32(9+offset, "xfer_num");
            xtsc_byte_enables   b               = get_u64(10+offset, "byte_enables");
            xtsc_address        hw_address      = get_u64(11+offset, "hw_address");
            bool                axi             = (m_burst != xtsc_request::NON_AXI);
            if (!xfer_num || (xfer_num > num_xfers)) {
              ostringstream oss;
              oss << "Invalid xfer_num found in BURST_WRITE (transfers are numbered between 1 and num_xfers): " << endl;
              oss << m_line;
              oss << m_script_file_stream.info_for_exception();
              throw xtsc_exception(oss.str());
            }
            if (xfer_num == 1) {
              bool last = (num_xfers == 1) || (axi && m_do_snoop && (m_snoop == xtsc_request::MUEvict));
              request.initialize(xtsc_request::BURST_WRITE, address8, size, 0, num_xfers, b, last, route_id, id, priority, pc, m_burst);
              if (axi) {
                request.adjust_burst_write(hw_address);
              }
              m_burst_write_tag = request.get_tag();
            }
            else {
              request.initialize(hw_address, m_burst_write_tag, address8, size, num_xfers, xfer_num, b, route_id, id, priority, pc, m_burst);
            }
            set_buffer(request, 12+offset, size);
            check_for_too_many_parameters(12+offset+size);
            check_for_too_few_parameters(12+offset+size);
            is_req = true;
          }
          else if (m_words[1] == "rcw") {
            // 0     1   2       3    4        5  6        7  8         9         10        11           12
            // delay RCW address size route_id id priority pc num_xfers last_xfer b0        b1           b2 b3
            // delay RCW address size route_id id priority pc coh       num_xfers last_xfer byte_enables b0 b1 b2 b3
            u32                 route_id      = get_u32(4, "route_id");
            u8                  id            = static_cast<u8>(get_u32(5, "id"));
            u8                  priority      = static_cast<u8>(get_u32(6, "priority"));
            xtsc_address        pc            = get_u32(7, "pc");
            u32                 offset        = (m_format > 1) ? 1 : 0;
            u32                 num_xfers     = get_u32(8+offset, "num_xfers");
            bool                last          = (get_u32(9+offset, "last_xfer") != 0);
            xtsc_byte_enables   b             = ((m_format > 1) ? get_u64(11, "byte_enables") : (0xFFFFFFFFFFFFFFFFull >> (64 - size)));
            if (last) {
              request.initialize(m_rcw_tag, address8, route_id, id, priority, pc);
              request.set_byte_enables(b);
              request.set_byte_size(size);
            }
            else {
              request.initialize(xtsc_request::RCW, address8, size, 0, num_xfers, b, false, route_id, id, priority, pc);
              m_rcw_tag = request.get_tag();
            }
            set_buffer(request, 10+offset*2, size);
            check_for_too_many_parameters(10+offset*2+size);
            check_for_too_few_parameters(10+offset*2+size);
            is_req = true;
          }
          else if (m_words[1] == "snoop") {
            // 0     1     2       3    4        5  6        7  8             9
            // delay SNOOP address size route_id id priority pc num_transfers 
            // delay SNOOP address size route_id id priority pc coh           num_transfers 
            // delay SNOOP address size route_id id priority pc snoop_cmd     num_transfers  
            u32          route_id      = get_u32(4, "route_id");
            u8           id            = static_cast<u8>(get_u32(5, "id"));
            u8           priority      = static_cast<u8>(get_u32(6, "priority"));
            xtsc_address pc            = get_u32(7, "pc");
            u32          offset        = (m_format > 1) ? 1 : 0;
            u32          num_transfers = get_u32(8+offset, "num_transfers");
            request.initialize(xtsc_request::SNOOP, address8, size, 0, num_transfers, 0, true,
                               route_id, id, priority, pc);

            if (m_format == 3) {
                   if (m_words[8] == "readnosnoop") m_snoop = xtsc_request::READ_NO_SNOOP;
              else if (m_words[8] == "writenosnoop") m_snoop = xtsc_request::WRITE_NO_SNOOP;
              else if (m_words[8] == "readonce") m_snoop = xtsc_request::READ_ONCE;
              else if (m_words[8] == "writeunique") m_snoop = xtsc_request::WRITE_UNIQUE;
              else if (m_words[8] == "readshared") m_snoop = xtsc_request::READ_SHARED;
              else if (m_words[8] == "writelineunique") m_snoop = xtsc_request::WRITE_LINE_UNIQUE;
              else if (m_words[8] == "readclean") m_snoop = xtsc_request::READ_CLEAN;
              else if (m_words[8] == "writeclean") m_snoop = xtsc_request::WRITE_CLEAN;
              else if (m_words[8] == "readnotshareddirty") m_snoop = xtsc_request::READ_NOT_SHARED_DIRTY;
              else if (m_words[8] == "writeback") m_snoop = xtsc_request::WRITE_BACK;
              else if (m_words[8] == "evict") m_snoop = xtsc_request::EVICT;
              else if (m_words[8] == "writeevict") m_snoop = xtsc_request::WRITE_EVICT;
              else if (m_words[8] == "readunique") m_snoop = xtsc_request::READ_UNIQUE;
              else if (m_words[8] == "cleanshared") m_snoop = xtsc_request::CLEAN_SHARED;
              else if (m_words[8] == "cleaninvalid") m_snoop = xtsc_request::CLEAN_INVALID;
              else if (m_words[8] == "cleanunique") m_snoop = xtsc_request::CLEAN_UNIQUE;
              else if (m_words[8] == "makeunique") m_snoop = xtsc_request::MAKE_UNIQUE;
              else if (m_words[8] == "makeinvalid") m_snoop = xtsc_request::MAKE_INVALID;
              else if (m_words[8] == "dvmcomplete") m_snoop = xtsc_request::DVM_COMPLETE;
              else if (m_words[8] == "dvpmessage") m_snoop = xtsc_request::DVP_MESSAGE;
              else                              m_snoop = (u8) get_u32(8, "snoop_command");

              request.set_snoop(m_snoop);
            }

            check_for_too_many_parameters(9 + offset);
            is_req = true;
          }
          else {
            ostringstream oss;
            oss << "Unrecognized request type '" << m_words[1] << "':" << endl;
            oss << m_line;
            oss << m_script_file_stream.info_for_exception();
            throw xtsc_exception(oss.str());
          }
        }
        if (is_req) {
          if (m_set_xfer_en) {
            request.set_xfer_en(true);
          }
          if (m_set_user_data) {
            request.set_user_data(m_user_data);
          }
          if (m_exclusive) {
            request.set_exclusive(true);
          }
        }
      }


      if (do_delay) {
          delay = get_double(0, "delay");
          XTSC_DEBUG(m_text, "waiting for " << (delay * m_clock_period));
          wait(delay * m_clock_period);
      }
      if (send) {
        if (m_do_dram_attribute) {
          request.set_dram_attribute(m_dram_attribute);
        }
        if (m_do_pif_attribute) {
          request.set_pif_attribute(m_pif_attribute);
        }
        if (m_do_domain) {
          if (request.is_axi_protocol()) {
            request.set_domain(m_domain);
          }
          else {
            request.set_pif_req_domain(m_domain);
          }
        }
        if (request.is_apb_protocol()) {
          if (m_do_prot) {
            request.set_prot(m_prot);
          }
        }
        if (request.is_axi_protocol()) {
          if (m_do_cache) {
            request.set_cache(m_cache);
          }
          if (m_do_prot) {
            request.set_prot(m_prot);
          }
          if (m_do_snoop) {
            request.set_snoop(m_snoop);
          }
        }
        if (m_set_byte_enables) {
          request.set_byte_enables(m_byte_enables);
        }
        if (m_set_last_transfer) {
          request.set_last_transfer(m_last_transfer);
        }
        ostringstream oss;
        request.dump(oss, true);
        XTSC_INFO(m_text, oss.str());
        m_last_request_tag = request.get_tag();
        m_last_request_got_response = false;
        m_last_request_got_nacc = false;
        request_time = sc_time_stamp();
        m_request_port->nb_request(request);
      }
      if (poke) {
        ostringstream oss;
        oss << "poke 0x" << hex << setfill('0') << setw(n) << address8 << setfill(' ') << ": ";
        xtsc_hex_dump(true, size, m_buffer, oss);
        if (m_use_coherent_peek_poke) {
          oss << " virtual=0x" << hex << setfill('0') << setw(n) << (address8+m_virtual_address_delta);
        }
        XTSC_INFO(m_text, oss.str());
        if (m_use_coherent_peek_poke) {
          m_request_port->nb_poke_coherent(address8+m_virtual_address_delta, address8, size, m_buffer);
        }
        else {
          m_request_port->nb_poke(address8, size, m_buffer);
        }
      }
      if (peek) {
        if (size > m_buffer_size) {
          ostringstream oss;
          oss << "Peek size of " << size << " exceeds maximum peek size of " << m_buffer_size << ":" << endl;
          oss << m_line;
          oss << m_script_file_stream.info_for_exception();
          throw xtsc_exception(oss.str());
        }
        if (m_use_coherent_peek_poke) {
          m_request_port->nb_peek_coherent(address8+m_virtual_address_delta, address8, size, m_buffer);
        }
        else {
          m_request_port->nb_peek(address8, size, m_buffer);
        }
        ostringstream oss;
        oss << "peek 0x" << hex << setfill('0') << setw(n) << address8 << setfill(' ') << ": ";
        xtsc_hex_dump(true, size, m_buffer, oss);
        if (m_use_coherent_peek_poke) {
          oss << " virtual=0x" << hex << setfill('0') << setw(n) << (address8+m_virtual_address_delta);
        }
        XTSC_INFO(m_text, oss.str());
      }
      if (stop) {
        sc_stop();
        wait();
      }
      if (lock) {
        XTSC_INFO(m_text, "nb_lock(" << boolalpha << lock_arg << ")");
        m_request_port->nb_lock(lock_arg);
      }
      if (retire) {
        XTSC_INFO(m_text, "nb_load_retired(0x" << hex << setfill('0') << setw(n) << address8 << ")");
        m_request_port->nb_load_retired(address8);
      }
      if (flush) {
        XTSC_INFO(m_text, "nb_retire_flush()");
        m_request_port->nb_retire_flush();
      }

      if (is_req) {
        prev_req_delay    = delay;
        prev_req_do_delay = do_delay;
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



int xtsc_component::xtsc_master::get_words() {
  m_line_count = m_script_file_stream.get_words(m_words, m_line, true);
  return m_words.size();
}




u32 xtsc_component::xtsc_master::get_u32(u32 index, const string& argument_name) {
  u32 value = 0;
  if (index >= m_words.size()) {
    ostringstream oss;
    oss << argument_name << " argument (#" << index+1 << ") missing in command:" << endl;
    oss << m_line;
    oss << m_script_file_stream.info_for_exception();
    throw xtsc_exception(oss.str());
  }
  try {
    value = xtsc_strtou32(m_words[index]);
  }
  catch (const xtsc_exception&) {
    ostringstream oss;
    oss << "Cannot convert " << argument_name << " argument (#" << index+1 << ") '" << m_words[index] << "' to number in:" << endl;
    oss << m_line;
    oss << m_script_file_stream.info_for_exception();
    throw xtsc_exception(oss.str());
  }
  return value;
}



u64 xtsc_component::xtsc_master::get_u64(u32 index, const string& argument_name) {
  u64 value = 0;
  if (index >= m_words.size()) {
    ostringstream oss;
    oss << argument_name << " argument (#" << index+1 << ") missing in command:" << endl;
    oss << m_line;
    oss << m_script_file_stream.info_for_exception();
    throw xtsc_exception(oss.str());
  }
  try {
    value = xtsc_strtou64(m_words[index]);
  }
  catch (const xtsc_exception&) {
    ostringstream oss;
    oss << "Cannot convert " << argument_name << " argument (#" << index+1 << ") '" << m_words[index] << "' to number in:" << endl;
    oss << m_line;
    oss << m_script_file_stream.info_for_exception();
    throw xtsc_exception(oss.str());
  }
  return value;
}



double xtsc_component::xtsc_master::get_double(u32 index, const string& argument_name) {
  double value = 0;
  if (index >= m_words.size()) {
    ostringstream oss;
    oss << argument_name << " argument (#" << index+1 << ") missing in command:" << endl;
    oss << m_line;
    oss << m_script_file_stream.info_for_exception();
    throw xtsc_exception(oss.str());
  }
  try {
    value = xtsc_strtod(m_words[index]);
  }
  catch (const xtsc_exception&) {
    ostringstream oss;
    oss << "Cannot convert " << argument_name << " argument (#" << index+1 << ") '" << m_words[index] << "' to number:" << endl;
    oss << m_line;
    oss << m_script_file_stream.info_for_exception();
    throw xtsc_exception(oss.str());
  }
  return value;
}



void xtsc_component::xtsc_master::check_for_too_many_parameters(u32 number_expected) {
  if (m_words.size() > number_expected) {
    ostringstream oss;
    oss << "xtsc_master '" << name() << "' found too many words: exp=" << number_expected << " has=" << m_words.size() << ": ";
    m_script_file_stream.dump_last_line_info(oss);
    throw xtsc_exception(oss.str());
  }
}



void xtsc_component::xtsc_master::check_for_too_few_parameters(u32 number_expected) {
  if (m_words.size() < number_expected) {
    ostringstream oss;
    oss << "xtsc_master '" << name() << "' found too few words: exp=" << number_expected << " has=" << m_words.size() << ": ";
    m_script_file_stream.dump_last_line_info(oss);
    throw xtsc_exception(oss.str());
  }
}



void xtsc_component::xtsc_master::set_buffer(u32 index, u32 size8, bool is_poke) {
  XTSC_DEBUG(m_text, "set_buffer: index=" << index << " size8=" << size8 << " m_words.size()=" << m_words.size());
  if (size8 > m_buffer_size) {
    ostringstream oss;
    oss << "xtsc_master '" << name() << "' line " << m_line_count << " requires buffer size of " << size8
        << " which exceeds maximum buffer size of " << m_buffer_size;
    oss << m_script_file_stream.info_for_exception();
    throw xtsc_exception(oss.str());
  }
  memset(m_buffer, 0, xtsc_max_bus_width8);
  if (size8 > xtsc_max_bus_width8) {
    if (!is_poke) {
      ostringstream oss;
      oss << "Size (" << size8 << ") exceeds maximum bus width (" << xtsc_max_bus_width8 << "):" << endl;
      oss << m_line;
      oss << m_script_file_stream.info_for_exception();
      throw xtsc_exception(oss.str());
    }
    memset(m_buffer, 0, size8);
  }
  u32 size = m_words.size();
  for (u32 i=0; i<size8; ++i) {
    if (index+i >= size) break;
    u32 value = get_u32(index+i, "data");
    m_buffer[i] = static_cast<u8>(value);
  }
}



void xtsc_component::xtsc_master::set_buffer(xtsc_request& request, u32 index, u32 size8) {
  XTSC_DEBUG(m_text, "set_buffer: index=" << index << " size8=" << size8 << " m_words.size()=" << m_words.size());
  u8 buffer[xtsc_max_bus_width8];
  memset(buffer, 0, xtsc_max_bus_width8);
  if (size8 > xtsc_max_bus_width8) {
    ostringstream oss;
    oss << "Size (" << size8 << ") exceeds maximum bus width (" << xtsc_max_bus_width8 << "):" << endl;
    oss << m_line;
    oss << m_script_file_stream.info_for_exception();
    throw xtsc_exception(oss.str());
  }
  u32 size = m_words.size();
  for (u32 i=0; i<size8; ++i) {
    if (index+i >= size) break;
    u32 value = get_u32(index+i, "data");
    buffer[i] = static_cast<u8>(value);
  }
  request.set_buffer(size8, buffer);
}



bool xtsc_component::xtsc_master::get_return_value() {
  bool result = true;
  if (m_return_value_file != "") {
    if (m_return_value_index >= m_return_values.size()) {
      m_return_value_line_count = m_p_return_value_file->get_words(m_return_values, m_return_value_line, true);
      m_return_value_index = 0;
    }
    string value = m_return_values[m_return_value_index];
    if ((value == "0") || (value == "false") || (value == "f")) {
      result = false;
    }
    else if ((value == "1") || (value == "true") || (value == "t")) {
      result = true;
    }
    else {
      ostringstream oss;
      oss << "Invalid return value '" << value << "' at word #" << (m_return_value_index + 1) << ":" << endl;
      oss << m_return_value_line;
      oss << m_p_return_value_file->info_for_exception();
      throw xtsc_exception(oss.str());
    }
    m_return_value_index += 1;
  }
  return result;
}



void xtsc_component::xtsc_master::register_port(sc_port_base& port, const char *if_typename) {
  if (m_p_port) {
    ostringstream oss;
    oss << "Illegal multiple binding detected to xtsc_master '" << name() << "' m_respond_export: " << endl;
    oss << "  " << port.name() << endl;
    oss << "  " << m_p_port->name();
    throw xtsc_exception(oss.str());
  }
  XTSC_INFO(m_text, "Binding '" << port.name() << "' to xtsc_master::m_respond_export");
  m_p_port = &port;
}



void xtsc_component::xtsc_master::xtsc_wire_write_if_impl::nb_write(const sc_unsigned& value) {
  if (static_cast<u32>(value.length()) != m_bit_width) {
    ostringstream oss;
    oss << "ERROR: Value of width=" << value.length() << " bits written to sc_export \"" << m_master.m_p_control->name()
        << "\" of width=" << m_bit_width << " in xtsc_master '" << m_master.name() << "'";
    throw xtsc_exception(oss.str());
  }
  XTSC_INFO(m_master.m_text, m_master.m_p_control->name() << " <= " << value.to_string(SC_HEX));
  m_master.m_control_write_count += 1;
  if (value != m_master.m_control_value) {
    m_master.m_control_value = value;
    m_master.m_control_change_count += 1;
  }
  m_master.m_control_write_event.notify(SC_ZERO_TIME);
}



void xtsc_component::xtsc_master::xtsc_wire_write_if_impl::register_port(sc_port_base& port, const char *if_typename) {
  if (m_p_port) {
    ostringstream oss;
    oss << "Illegal multiple binding detected to sc_export<xtsc_wire_write_if> \"" << m_master.m_p_control->name()
        << "\" of xtsc_master '" << m_master.name() << "'" << endl;
    oss << "  " << port.name() << endl;
    oss << "  " << m_p_port->name();
    throw xtsc_exception(oss.str());
  }
  XTSC_INFO(m_master.m_text, "Binding '" << port.name() << "' to sc_export<xtsc_wire_write_if> \"" << m_master.m_p_control->name() <<
                             "\" of xtsc_master '" << m_master.name() << "'");
  m_p_port = &port;
  m_master.m_control_bound = true;
}



