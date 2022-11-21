// Copyright (c) 2005-2013 by Tensilica Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Tensilica Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Tensilica Inc.


#include <cctype>
#include <algorithm>
#include <xtsc/xtsc_slave.h>
#include <xtsc/xtsc_arbiter.h>
#include <xtsc/xtsc_core.h>
#include <xtsc/xtsc_master.h>
#include <xtsc/xtsc_memory_trace.h>
#include <xtsc/xtsc_pin2tlm_memory_transactor.h>
#include <xtsc/xtsc_router.h>

using namespace std;
#if SYSTEMC_VERSION >= 20050601
using namespace sc_core;
#endif
using namespace xtsc;
using log4xtensa::INFO_LOG_LEVEL;
using log4xtensa::VERBOSE_LOG_LEVEL;



xtsc_component::xtsc_slave::xtsc_slave(sc_module_name module_name, const xtsc_slave_parms& slave_parms) :
  sc_module             (module_name),
  xtsc_module           (*(sc_module*)this),
  m_request_export      ("m_request_export"),
  m_respond_port        ("m_respond_port"),
  m_script_file         (""),
  m_format              (slave_parms.get_non_zero_u32("format")),
  m_ignore_count        (0),
  m_transfer_num        (0),
  m_text                (log4xtensa::TextLogger::getInstance(name()))
{

  m_wraparound          = slave_parms.get_bool("wraparound");
  m_response_stream     = 0;

  // Get clock period 
  u32 clock_period = slave_parms.get_u32("clock_period");
  if (clock_period == 0xFFFFFFFF) {
    m_clock_period = xtsc_get_system_clock_period();
  }
  else {
    m_clock_period = sc_get_time_resolution() * clock_period;
  }

  m_repeat_count        = slave_parms.get_u32("repeat_count");
  u32 repeat_delay      = slave_parms.get_u32("repeat_delay");
  if (repeat_delay == 0xFFFFFFFF) {
    m_repeat_delay_time = m_clock_period;
  }
  else {
    m_repeat_delay_time = sc_get_time_resolution() * repeat_delay;
  }

  const char *file_name = slave_parms.get_c_str("script_file");
  if (file_name && (file_name[0] != 0)) {
    m_response_stream = new xtsc_script_file(file_name, "script_file",  name(), kind(), m_wraparound);
    m_script_file = m_response_stream->name();
  }

  SC_THREAD(delayed_response_thread);   m_process_handles.push_back(sc_get_current_process_handle());

  m_request_export(*this);

  m_port_types["m_request_export"] = REQUEST_EXPORT;
  m_port_types["m_respond_port"  ] = RESPOND_PORT;
  m_port_types["slave_port"      ] = PORT_TABLE;

  log4xtensa::LogLevel ll = xtsc_get_constructor_log_level();
  XTSC_LOG(m_text, ll,        "Constructed xtsc_slave '" << name() << "':");
  XTSC_LOG(m_text, ll,        " script_file             = "   << m_script_file);
  XTSC_LOG(m_text, ll,        " wraparound              = "   << (m_wraparound ? "true" : "false"));
  XTSC_LOG(m_text, ll,        " repeat_count            = "   << m_repeat_count);
  if (repeat_delay == 0xFFFFFFFF) {
  XTSC_LOG(m_text, ll, hex << " repeat_delay            = 0x" << repeat_delay << " (" << m_repeat_delay_time << ")");
  } else {
  XTSC_LOG(m_text, ll,        " repeat_delay            = "   << repeat_delay << " (" << m_repeat_delay_time << ")");
  }
  if (clock_period == 0xFFFFFFFF) {
  XTSC_LOG(m_text, ll, hex << " clock_period            = 0x" << clock_period << " (" << m_clock_period << ")");
  } else {
  XTSC_LOG(m_text, ll,        " clock_period            = "   << clock_period << " (" << m_clock_period << ")");
  XTSC_LOG(m_text, ll,        " format                  = "   << m_format);
  }

  reset();

}



xtsc_component::xtsc_slave::~xtsc_slave(void) {
}



u32 xtsc_component::xtsc_slave::get_bit_width(const string& port_name, u32 interface_num) const {
  return 0;
}



sc_object *xtsc_component::xtsc_slave::get_port(const string& port_name) {
  if ((port_name != "m_request_export") && (port_name != "m_respond_port")) {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\" has no port named \"" << port_name << "\"" << endl;
    throw xtsc_exception(oss.str());
  }

  return ((port_name == "m_request_export") ?  (sc_object*) &m_request_export : (sc_object*) &m_respond_port);
}



xtsc_port_table xtsc_component::xtsc_slave::get_port_table(const string& port_table_name) const {
  if (port_table_name != "slave_port") {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\" has no port table named \"" << port_table_name << "\"" << endl;
    throw xtsc_exception(oss.str());
  }

  xtsc_port_table table;
  table.push_back("m_request_export");
  table.push_back("m_respond_port"  );

  return table;
}



void xtsc_component::xtsc_slave::reset(bool /*hard_reset*/) {
  XTSC_INFO(m_text, "xtsc_slave::reset()");

  m_line                = "";
  m_line_count          = 0;
  m_ignore_count        = 0;
  m_transfer_num        = 0;

  m_words.clear();

  if (m_response_stream) {
    m_response_stream->reset();
  }

  // Empty deque of response
  while (m_responses.size()) {
    response_info *p_response_info = m_responses[0];
    m_responses.pop_front();
    delete_response_info(p_response_info);
  }

  // Cancel notified events
  m_delayed_response_event.cancel();

  xtsc_reset_processes(m_process_handles);
}



void xtsc_component::xtsc_slave::connect(xtsc_arbiter& arbiter) {
  arbiter.m_request_port(m_request_export);
  m_respond_port(arbiter.m_respond_export);
}



void xtsc_component::xtsc_slave::connect(xtsc_core& core, const char *memory_name) {
  core.get_request_port(memory_name)(m_request_export);
  m_respond_port(core.get_respond_export(memory_name));
}



void xtsc_component::xtsc_slave::connect(xtsc_master& master) {
  master.m_request_port(m_request_export);
  m_respond_port(master.m_respond_export);
}



void xtsc_component::xtsc_slave::connect(xtsc_memory_trace& memory_trace, u32 port_num) {
  u32 num_ports = memory_trace.get_num_ports();
  if (port_num >= num_ports) {
    ostringstream oss;
    oss << "Invalid port_num=" << port_num << " in connect(): " << endl;
    oss << memory_trace.kind() << " '" << memory_trace.name() << "' has " << num_ports << " ports numbered from 0 to " << num_ports-1
        << endl;
    throw xtsc_exception(oss.str());
  }
  (*memory_trace.m_request_ports[port_num])(m_request_export);
  m_respond_port(*memory_trace.m_respond_exports[port_num]);
}



void xtsc_component::xtsc_slave::connect(xtsc_pin2tlm_memory_transactor& pin2tlm, u32 port_num) {
  u32 num_slaves = pin2tlm.get_num_ports();
  if (port_num >= num_slaves) {
    ostringstream oss;
    oss << "Invalid port_num=" << port_num << " in connect(): " << endl;
    oss << pin2tlm.kind() << " '" << pin2tlm.name() << "' has " << num_slaves << " ports numbered from 0 to " << num_slaves-1 << endl;
    throw xtsc_exception(oss.str());
  }
  (*pin2tlm.m_request_ports[port_num])(m_request_export);
  m_respond_port(*pin2tlm.m_respond_exports[port_num]);
}



void xtsc_component::xtsc_slave::connect(xtsc_router& router, u32 port_num) {
  u32 num_slaves = router.get_num_slaves();
  if (port_num >= num_slaves) {
    ostringstream oss;
    oss << "Invalid port_num=" << port_num << " in connect(): " << endl;
    oss << router.kind() << " '" << router.name() << "' has " << num_slaves << " ports numbered from 0 to " << num_slaves-1 << endl;
    throw xtsc_exception(oss.str());
  }
  (*router.m_request_ports[port_num])(m_request_export);
  m_respond_port(*router.m_respond_exports[port_num]);
}



void xtsc_component::xtsc_slave::nb_peek(xtsc_address address8, u32 size8, u8 *buffer) {
  XTSC_INFO(m_text, "nb_peek: address=0x" << hex << address8 << " size=" << dec << size8);
}



void xtsc_component::xtsc_slave::nb_poke(xtsc_address address8, u32 size8, const u8 *buffer) {
  XTSC_INFO(m_text, "nb_poke: address=0x" << hex << address8 << " size=" << dec << size8);
}



bool xtsc_component::xtsc_slave::nb_fast_access(xtsc_fast_access_request &request) {
  xtsc_address address8 = request.get_translated_request_address();
  XTSC_INFO(m_text, "nb_fast_access: address=0x" << hex << address8);
  request.deny_access();
  return true;
}



bool xtsc_component::xtsc_slave::nb_peek_coherent(xtsc_address  virtual_address8,
                                                  xtsc_address  physical_address8,
                                                  u32           size8,
                                                  u8           *buffer)
{
  XTSC_INFO(m_text, "nb_peek_coherent: virtual_address8=0x" << hex << virtual_address8 << " physical_address8=0x" << hex <<
                    physical_address8 << " size=" << dec << size8);
  return true;
}



bool xtsc_component::xtsc_slave::nb_poke_coherent(xtsc_address  virtual_address8,
                                                  xtsc_address  physical_address8,
                                                  u32           size8,
                                                  const u8     *buffer)
{
  XTSC_INFO(m_text, "nb_poke_coherent: virtual_address8=0x" << hex << virtual_address8 << " physical_address8=0x" << hex <<
                    physical_address8 << " size=" << dec << size8);
  return true;
}



void xtsc_component::xtsc_slave::nb_request(const xtsc_request& request) {
  ostringstream oss;
  oss << request;
  XTSC_INFO(m_text, oss.str());
  response_info *p_response_info = new_response_info(request);
  bool more = (p_response_info != NULL);
  while (more) {
    more = p_response_info->m_cont;
    if (p_response_info->m_respond_now) {
      send_response(*p_response_info->m_p_response, true);
      delete_response_info(p_response_info);
    }
    else {
      m_responses.push_back(p_response_info);
      m_delayed_response_event.notify(SC_ZERO_TIME);
    }
    if (more) {
      p_response_info = new_response_info(request);
    }
  }
}



void xtsc_component::xtsc_slave::delayed_response_thread(void) {

  try {

    while (true) {
      wait(m_delayed_response_event);
      while (m_responses.size()) {
        response_info *p_response_info = m_responses[0];
        m_responses.pop_front();
        if (!p_response_info->m_respond_now) {
          wait(p_response_info->m_delay);
        }
        send_response(*p_response_info->m_p_response, false);
        delete_response_info(p_response_info);
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




void xtsc_component::xtsc_slave::send_response(xtsc_response &response, bool non_blocking) {
  bool repeat = false;
  u32 repeat_count = m_repeat_count;
  m_transfer_num += 1;
  response.set_transfer_number(m_transfer_num);
  if (response.get_last_transfer()) {
    m_transfer_num = 0;
  }
  do {
    XTSC_INFO(m_text, response);
    repeat = false;
    if (!m_respond_port->nb_respond(response)) {
      XTSC_INFO(m_text, response << " <-- REJECTED");
      if (!non_blocking && (repeat_count != 0)) {
        if (repeat_count != 0xFFFFFFFF) {
          repeat_count -= 1;
        }
        repeat = true;
        wait(m_repeat_delay_time);
      }
    }
  } while (repeat);
}



xtsc_component::xtsc_slave::response_info *xtsc_component::xtsc_slave::new_response_info(const xtsc_request& request) {
  if (m_ignore_count) {
    m_ignore_count -= 1;
    return NULL;
  }
  while (true) {
    get_words();
    if (m_words[0] == "info") {
      XTSC_INFO(m_text, m_line);
    }
    else if (m_words[0] == "note") {
      XTSC_NOTE(m_text, m_line);
    }
    else if (m_words[0] == "repeat") {
      if ((m_words.size() != 2) && (m_words.size() != 3)) {
        ostringstream oss;
        oss << "Syntax error (expected REPEAT rcount [rdelay]):" << endl;
        oss << m_line;
        oss << m_response_stream->info_for_exception();
        throw xtsc_exception(oss.str());
      }
      if (m_words[1] == "forever") {
        m_repeat_count = 0xFFFFFFFF;
      }
      else {
        m_repeat_count = get_u32(1, "rcount");
      }
      if (m_words.size() > 2) {
        m_repeat_delay_time = m_clock_period*get_double(2, "rdelay");
      }
    }
    else if (m_words[0] == "ignore") {
      if (m_words.size() != 2) {
        ostringstream oss;
        oss << "Syntax error (expected: IGNORE icount):" << endl;
        oss << m_line;
        oss << m_response_stream->info_for_exception();
        throw xtsc_exception(oss.str());
      }
      m_ignore_count = get_u32(1, "icount");
      if (m_ignore_count == 0) {
        ostringstream oss;
        oss << "icount cannot be 0:" << endl;
        oss << m_line;
        oss << m_response_stream->info_for_exception();
        throw xtsc_exception(oss.str());
      }
      m_ignore_count -= 1;
      return NULL;
    }
    else if (m_words[0] == "format") {
      if ((m_words.size() != 2) || ((m_words[1] != "1") && (m_words[1] != "2"))) {
        ostringstream oss;
        oss << "Syntax error (expected FORMAT 1|2):" << endl;
        oss << m_line;
        oss << m_response_stream->info_for_exception();
        throw xtsc_exception(oss.str());
      }
      m_format = ((m_words[1] == "1") ? 1 : 2);
    }
    else break;
  }
  xtsc_response::status_t status = xtsc_response::RSP_OK;
       if ((m_words[1] == "okay") || (m_words[1] == "ok"))      status = xtsc_response::RSP_OK;
  else if ((m_words[1] == "addr") || (m_words[1] == "address")) status = xtsc_response::RSP_ADDRESS_ERROR;
  else if ((m_words[1] == "data"))                              status = xtsc_response::RSP_DATA_ERROR;
  else if ((m_words[1] == "both"))                              status = xtsc_response::RSP_ADDRESS_DATA_ERROR;
  else if ((m_words[1] == "nacc"))                              status = xtsc_response::RSP_NACC;
  else {
    ostringstream oss;
    oss << "Unrecognized response status '" << m_words[1] << "' at argument #2:" << endl;
    oss << m_line;
    oss << m_response_stream->info_for_exception();
    throw xtsc_exception(oss.str());
  }
  bool cont = (m_words[m_words.size()-1] == "cont");
  bool last = (get_u32(7, "last_transfer") != 0);

  xtsc_response *p_response = new xtsc_response(request, status, last);

  // Format 1
  // 0     1      2    3        4  5        6  7    8  9  ... 8+N
  // delay status size route_id id priority pc last b0 b1 ... bN [cont]

  // Format 2
  // 0     1      2    3        4  5        6  7    8   9  10 ... 9+N
  // delay status size route_id id priority pc last coh b0 b1 ... bN [cont]

  if (m_words[3] != "*") {
    p_response->set_route_id(get_u32(3, "route_id"));
  }
  if (m_words[4] != "*") {
    p_response->set_id(static_cast<u8>(get_u32(4, "id")));
  }
  if (m_words[5] != "*") {
    p_response->set_priority(static_cast<u8>(get_u32(5, "priority")));
  }
  if (m_words[6] != "*") {
    p_response->set_pc(get_u32(6, "pc"));
  }

  u32           size    = get_u32(2, "size");
  u32           offset  = (m_format > 1) ? 1 : 0;
  bool          snoop   = p_response->is_snoop();
  p_response->set_snoop_data(snoop && size);

  set_buffer(*p_response, 8+offset, size);

  bool respond_now = (m_words[0] == "now");
  sc_time delay = respond_now ? SC_ZERO_TIME : m_clock_period*get_double(0, "delay");
  return new response_info(p_response, respond_now, cont, delay);
}



void xtsc_component::xtsc_slave::delete_response_info(response_info*& p_response_info) {
  if (p_response_info) {
    delete p_response_info;
    p_response_info = 0;
  }
}



int xtsc_component::xtsc_slave::get_words() {

  m_line_count = 0;

  if (m_script_file != "") {
    m_line_count = m_response_stream->get_words(m_words, m_line, true);
    XTSC_DEBUG(m_text, "\"script_file\" line #" << m_line_count << ": " << m_line);
  }

  if (!m_line_count) {
    // Standard response
    m_words.clear();
    m_words.push_back("1");     // 0 delay
    m_words.push_back("okay");  // 1 status
    m_words.push_back("0");     // 2 size
    m_words.push_back("*");     // 3 route_id
    m_words.push_back("*");     // 4 id
    m_words.push_back("*");     // 5 priority
    m_words.push_back("*");     // 6 pc
    m_words.push_back("1");     // 7 last
  }

  if (m_words.size() == 1) {
    ostringstream oss;
    oss << "Line has too few words:" << endl;
    oss << m_line;
    oss << m_response_stream->info_for_exception();
    throw xtsc_exception(oss.str());
  }

  return m_words.size();
}



u32 xtsc_component::xtsc_slave::get_u32(u32 index, const string& argument_name) {
  u32 value = 0;
  if (index >= m_words.size()) {
    ostringstream oss;
    oss << argument_name << " argument (#" << index+1 << ") missing:" << endl;
    oss << m_line;
    oss << m_response_stream->info_for_exception();
    throw xtsc_exception(oss.str());
  }
  try {
    value = xtsc_strtou32(m_words[index]);
  }
  catch (const xtsc_exception&) {
    ostringstream oss;
    oss << "Cannot convert argument #" << index+1 << " '" << m_words[index] << "' to number:" << endl;
    oss << m_line;
    oss << m_response_stream->info_for_exception();
    throw xtsc_exception(oss.str());
  }
  return value;
}



double xtsc_component::xtsc_slave::get_double(u32 index, const string& argument_name) {
  double value = 0;
  if (index >= m_words.size()) {
    ostringstream oss;
    oss << argument_name << " argument (#" << index+1 << ") missing:" << endl;
    oss << m_line;
    oss << m_response_stream->info_for_exception();
    throw xtsc_exception(oss.str());
  }
  try {
    value = xtsc_strtod(m_words[index]);
  }
  catch (const xtsc_exception&) {
    ostringstream oss;
    oss << "Cannot convert argument #" << index+1 << " '" << m_words[index] << "' to number:" << endl;
    oss << m_line;
    oss << m_response_stream->info_for_exception();
    throw xtsc_exception(oss.str());
  }
  return value;
}



void xtsc_component::xtsc_slave::set_buffer(xtsc_response& response, u32 index, u32 size8) {
  XTSC_DEBUG(m_text, "set_buffer: index=" << index << " size8=" << size8 << " m_words.size()=" << m_words.size());
  u8 buffer[xtsc_max_bus_width8];
  memset(buffer, 0, xtsc_max_bus_width8);
  if (size8 > xtsc_max_bus_width8) {
    ostringstream oss;
    oss << "size argument (" << size8 << ") exceeds maximum bus width (" << xtsc_max_bus_width8 << "):" << endl;
    oss << m_line;
    oss << m_response_stream->info_for_exception();
    throw xtsc_exception(oss.str());
  }
  u32 size = m_words.size();
  for (u32 i=0; i<size8; ++i) {
    if (index+i >= size) break;
    u32 value = get_u32(index+i, "data");
    buffer[i] = static_cast<u8>(value);
  }
  response.set_buffer(buffer);
}



