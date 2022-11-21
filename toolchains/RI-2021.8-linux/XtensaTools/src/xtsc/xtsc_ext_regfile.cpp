// Copyright (c) 2006-2020 by Tensilica Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Tensilica Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Tensilica Inc.

// m_time_resolution\|\w*clock_period\w*\|\w*posedge_offset\w*\|

#include <cerrno>
#include <algorithm>
#include <ostream>
#include <string>
#include <xtsc/xtsc_ext_regfile.h>
#include <xtsc/xtsc_core.h>
#include <xtsc/xtsc_logging.h>


// xtsc_ext_regfile does binary logging of ext_regfile events at verbose log level and xtsc_core does it
// at info log level.  This is the reverse of the normal arrangement because xtsc_core knows the
// program counter (PC) and the port number, but xtsc_ext_regfile knows neither.



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
//using log4xtensa::EXT_REGFILE_KEY;
using log4xtensa::READY;
using log4xtensa::RESPONSE_VALUE;


static sc_unsigned sc_unsigned_zero(1);
static sc_unsigned sc_unsigned_one (1);


xtsc_component::xtsc_ext_regfile_parms::xtsc_ext_regfile_parms(const xtsc_core&      core, 
                                                               const char           *ext_regfile_name)
{
  u32   address_bit_width       = core.get_ext_regfile_address_bit_width(ext_regfile_name);
  u32   data_bit_width          = core.get_ext_regfile_data_bit_width(ext_regfile_name);
  init(address_bit_width, data_bit_width);
  set("clock_period", core.get_parms().get_u32("SimClockFactor")*xtsc_get_system_clock_factor());
}



xtsc_component::xtsc_ext_regfile::xtsc_ext_regfile(sc_module_name module_name, const xtsc_ext_regfile_parms& ext_regfile_parms) :
  sc_module             (module_name),
  xtsc_module           (*(sc_module*)this),
  m_ext_regfile         ("m_ext_regfile"),
  m_ext_regfile_impl    ("m_ext_regfile_impl", *this),
  m_ext_regfile_parms   (ext_regfile_parms),
  m_address_bit_width   (ext_regfile_parms.get_non_zero_u32("address_bit_width")),
  m_data_bit_width      (ext_regfile_parms.get_non_zero_u32("data_bit_width")),
  m_lua_data_function   (""),
  m_lua_function        (false),
  m_wr_data             (m_data_bit_width),
  m_data                (m_data_bit_width),
  m_data_temp           (m_data_bit_width),
  m_old_data            (m_data_bit_width),
  m_poke_data           (m_data_bit_width),
#if IEEE_1666_SYSTEMC >= 201101L
  m_ext_regfile_ready_event  ("m_ext_regfile_ready_event"),
#endif
  m_zero                (1),
  m_one                 (1),
  m_nb_send_address_cnt (0),
  m_nb_is_ready_cnt     (0),
  m_nb_get_data_cnt     (0),
  m_address_trace       (m_address_bit_width),
  m_data_trace          (m_data_bit_width),
  m_text                (log4xtensa::TextLogger::getInstance(name())),
  m_binary              (log4xtensa::BinaryLogger::getInstance(name()))
{

  m_ext_regfile(m_ext_regfile_impl);

  sc_unsigned_zero      = 0;
  sc_unsigned_one       = 1;

  m_zero                = 0;
  m_one                 = 1;
  m_file_logged         = false;
  m_file                = 0;
  m_p_trace_file        = static_cast<sc_trace_file*>(const_cast<void*>(ext_regfile_parms.get_void_pointer("vcd_handle")));

  // Get clock period 
  m_time_resolution     = sc_get_time_resolution();
  u32 clock_period = ext_regfile_parms.get_non_zero_u32("clock_period");
  if (clock_period == 0xFFFFFFFF) {
    m_clock_period = xtsc_get_system_clock_period();
  }
  else {
    m_clock_period = sc_get_time_resolution() * clock_period;
  }

  u32 posedge_offset = ext_regfile_parms.get_u32("posedge_offset");
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

  compute_delays();

  if (m_p_trace_file) {
    sc_trace(m_p_trace_file, m_nb_send_address_cnt, *new string(name()) + ".nb_send_address_cnt");
    sc_trace(m_p_trace_file, m_address_trace,       *new string(name()) + ".address");
    sc_trace(m_p_trace_file, m_nb_get_data_cnt,     *new string(name()) + ".nb_get_data_cnt");
    sc_trace(m_p_trace_file, m_data_trace,          *new string(name()) + ".data");
  }

  m_port_types["m_ext_regfile"] = EXT_REGFILE_EXPORT;
  m_port_types["ext_regfile"  ] = PORT_TABLE;

  xtsc_register_command(*this, *this, "change_clock_period", 1, 1,
      "change_clock_period <ClockPeriodFactor>", 
      "Call xtsc_ext_regfile::change_clock_period(<ClockPeriodFactor>).  Return previous <ClockPeriodFactor> for this ext_regfile."
  );

  xtsc_register_command(*this, *this, "dump", 0, 0,
      "dump", 
      "Return the os buffer from calling xtsc_ext_regfile::dump(os)."
  );

  xtsc_register_command(*this, *this, "peek", 1, 1,
      "peek <Address>", 
      "Call xtsc_ext_regfile::peek(<Address>)."
  );

  xtsc_register_command(*this, *this, "poke", 2, 2,
      "poke <Address> <Value>", 
      "Return xtsc_ext_regfile::poke(<Address>, <Value>)."
  );

  xtsc_register_command(*this, *this, "reset", 0, 1,
      "reset [<Hard>]", 
      "Call xtsc_ext_regfile::reset(<Hard>).  Where <Hard> is 0|1 (default 0)."
  );

#if IEEE_1666_SYSTEMC < 201101L
  xtsc_event_register(m_ext_regfile_ready_event, "m_ext_regfile_ready_event", this);
#endif

  log4xtensa::LogLevel ll = xtsc_get_constructor_log_level();
  XTSC_LOG(m_text, ll, "Constructed xtsc_ext_regfile '" << name() << "':");
  XTSC_LOG(m_text, ll, " address_bit_width       = "   << m_address_bit_width);
  XTSC_LOG(m_text, ll, " data_bit_width          = "   << m_data_bit_width);
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
  XTSC_LOG(m_text, ll, " vcd_handle              = "   << m_p_trace_file);

  reset(true);

  if (m_lua_function) {
  XTSC_LOG(m_text, ll, " <LuaDataFunction>       = "   << m_lua_data_function);
  XTSC_LOG(m_text, ll, " <LuaDelayFunction>      = "   << m_lua_delay_function);
  }
}



xtsc_component::xtsc_ext_regfile::~xtsc_ext_regfile(void) {
}



u32 xtsc_component::xtsc_ext_regfile::get_bit_width(const string& port_name, u32 interface_num) const {
  return ((interface_num == 0) ? m_data_bit_width : m_address_bit_width);
}



sc_object *xtsc_component::xtsc_ext_regfile::get_port(const string& port_name) {
  if (port_name != "m_ext_regfile") {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\" has no port named \"" << port_name << "\"" << endl;
    throw xtsc_exception(oss.str());
  }

  return (sc_object*) &m_ext_regfile;
}



xtsc_port_table xtsc_component::xtsc_ext_regfile::get_port_table(const string& port_table_name) const {
  if (port_table_name != "ext_regfile") {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\" has no port table named \"" << port_table_name << "\"" << endl;
    throw xtsc_exception(oss.str());
  }

  xtsc_port_table table;
  table.push_back("m_ext_regfile");

  return table;
}



void xtsc_component::xtsc_ext_regfile::reset(bool hard_reset) {
  XTSC_INFO(m_text, "xtsc_ext_regfile::reset()");

  m_line_count          = 0;
  m_write               = false;

  m_ext_regfile_ready_event.cancel();

  m_data_fifo.clear();
  m_cycle_fifo.clear();

  for (deque<sc_unsigned*>::iterator i = m_recycle_fifo.begin(); i != m_recycle_fifo.end(); ++i) {
    sc_unsigned *p_data = *i;
    delete_sc_unsigned(p_data);
  }
  m_recycle_fifo.clear();

  if (m_file && hard_reset) {
    m_file->reset();
    m_data_map.clear();

    if (!m_file_logged) {
      XTSC_LOG(m_text, xtsc_get_constructor_log_level(), "Loading ext_regfile table from file '" << m_ext_regfile_table << "'.");
    }

    u32 next_address = 0;

    //while ((m_line_count = m_file->get_words(m_words, m_line, false)) != 0) {

    //  u32 num_words = m_words.size();

    //  if ((num_words < 1) || (num_words > 3)) {
    //    ostringstream oss;
    //    oss << "Invalid number of words (expected 1, 2, or 3):" << endl;
    //    oss << m_line;
    //    oss << m_file->info_for_exception();
    //    throw xtsc_exception(oss.str());
    //  }

    //  if (m_words[0] == "lua_function") {
    //    if (num_words == 1) {
    //      ostringstream oss;
    //      oss << "Empty lua_function line.  Format is: lua_function <LuaDataFunction> [<LuaDelayFunction>]" << endl;
    //      oss << m_line;
    //      oss << m_file->info_for_exception();
    //      throw xtsc_exception(oss.str());
    //    }
    //    m_lua_data_function = m_words[1];
    //    if (num_words == 3) {
    //      m_lua_delay_function = m_words[2];
    //    }
    //    m_lua_function = true;
    //    continue;
    //  }

    //  u32 address = 0;
    //  sc_unsigned *p_data = new_sc_unsigned(m_zero);
    //  if (num_words == 1) {
    //    address = next_address;
    //    get_sc_unsigned(0, *p_data);
    //  }
    //  else {
    //    if (m_words[num_words-1][0] == '@') {
    //      try {
    //        //delay = xtsc_strtou32(m_words[num_words-1].substr(1));
    //      }
    //      catch (...) {
    //        ostringstream oss;
    //        oss << "Invalid delay:" << endl;
    //        oss << m_line;
    //        oss << m_file->info_for_exception();
    //        throw xtsc_exception(oss.str());
    //      }
    //      get_sc_unsigned(num_words-2, *p_data);
    //      if (num_words == 2) {
    //        address = next_address;
    //      }
    //      else {
    //        get_sc_unsigned(0, address);
    //      }
    //    }
    //    else {
    //      if (num_words == 3) {
    //        ostringstream oss;
    //        oss << "3rd word is not a delay (it must start with @):" << endl;
    //        oss << m_line;
    //        oss << m_file->info_for_exception();
    //        throw xtsc_exception(oss.str());
    //      }
    //      get_sc_unsigned(0, address);
    //      get_sc_unsigned(1, *p_data);
    //    }
    //  }

    //  ostringstream oss;
    //  oss << "0x" << address.to_string(SC_HEX);
    //  map<string, sc_unsigned*>::iterator imap = m_data_map.find(oss.str());
    //  if (imap == m_data_map.end()) {
    //    m_data_map[oss.str()] = p_data;
    //  }
    //  else {
    //    ostringstream oss;
    //    oss << "Found duplicate address=0x" << address.to_string(SC_HEX).substr(address_bits%4 ? 2 : 3) << ":" << endl;
    //    oss << m_line;
    //    oss << m_file->info_for_exception();
    //    throw xtsc_exception(oss.str());
    //  }

    //  next_address = address + 1;
    //  if (!m_file_logged) {
    //    XTSC_VERBOSE(m_text, "Line " << dec << m_line_count << ": 0x" << address.to_string(SC_HEX).substr(address_bits%4 ? 2 : 3) <<
    //                         " => 0x" << p_data->to_string(SC_HEX).substr(m_data_bit_width%4 ? 2 : 3));
    //  }
    //}

    m_file_logged = true;

  }

  m_ext_regfile_ready_event.cancel();
}

void xtsc_component::xtsc_ext_regfile::change_clock_period(u32 clock_period_factor) {
  m_clock_period = sc_get_time_resolution() * clock_period_factor;
  XTSC_INFO(m_text, "change_clock_period(" << clock_period_factor << ") => " << m_clock_period);
  compute_delays();
}

void xtsc_component::xtsc_ext_regfile::compute_delays() {
  m_clock_period_value  = m_clock_period.value();
  u32 posedge_offset = m_ext_regfile_parms.get_u32("posedge_offset");
  if (m_posedge_offset >= m_clock_period) {
    ostringstream oss;
    oss << kind() << " '" << name() << "' parameter error:" << endl;
    oss << "\"posedge_offset\" (0x" << hex << posedge_offset << "=>" << m_posedge_offset
        << ") must be strictly less than clock period => " << m_clock_period << ")";
    throw xtsc_exception(oss.str());
  }
}

void xtsc_component::xtsc_ext_regfile::execute(const string&            cmd_line, 
                                          const vector<string>&         words,
                                          const vector<string>&         words_lc,
                                          ostream&                      result)
{
  ostringstream res;

  if (false) {
  }
  else if (words[0] == "change_clock_period") {
    u32 clock_period_factor = xtsc_command_argtou32(cmd_line, words, 1);
    res << m_clock_period.value();
    change_clock_period(clock_period_factor);
  }
  else if (words[0] == "dump") {
    dump(res);
  }
  else if (words[0] == "peek") {
    res << peek(words[1]);
  }
  else if (words[0] == "poke") {
    poke(words[1], words[2]);
  }
  else if (words[0] == "reset") {
    reset((words.size() == 1) ? false : xtsc_command_argtobool(cmd_line, words, 1));
  }
  else {
    ostringstream oss;
    oss << "xtsc_ext_regfile::execute called for unknown command '" << cmd_line << "'.";
    throw xtsc_exception(oss.str());
  }

  result << res.str();
}

void xtsc_component::xtsc_ext_regfile::connect(xtsc_core& core, const char *ext_regfile_name) {
  core.get_ext_regfile(ext_regfile_name)(m_ext_regfile);
}

void xtsc_component::xtsc_ext_regfile::get_sc_unsigned(u32 index, sc_unsigned& value) {
  try {
    // Prevent sign extension that sc_unsigned does when assigned a hex string with the high bit set
    string word_no_sign_extend = m_words[index];
    if ((word_no_sign_extend.size() > 2) && (word_no_sign_extend.substr(0,2) == "0x")) {
      word_no_sign_extend.insert(2, "0");
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

void xtsc_component::xtsc_ext_regfile::do_write() {
  //Write data to external regfile and start the operation.
  //TODO: Right now doing nothing, except writing to the register.
  m_data_map[m_wr_addr]   = m_wr_data;

  //For external register file, write request implies start of operation.
  u64     delay             = m_wr_addr % 10; //Delay between 0-9
  sc_time now               = sc_time_stamp();
  u64     cycle             = now.value() / m_clock_period_value; // current cycle
  sc_time posedge_clock     = cycle * m_clock_period;             // posedge clock of current cycle
  //sc_time phase_now         = now - posedge_clock;                // clock phase now
  sc_time delay_time        = m_clock_period * (delay + 1);       // delay as sc_time
  //sc_time ready_delta       = delay_time - phase_now;             // time to finish operation
  sc_time op_fini_time      = posedge_clock + delay_time;         // time at which operation finishes
  m_op_fini_time[m_wr_addr] = op_fini_time;
}

sc_unsigned *xtsc_component::xtsc_ext_regfile::new_sc_unsigned(const string& value) {
  if (m_sc_unsigned_pool.empty()) {
    sc_unsigned *p_sc_unsigned = new sc_unsigned(m_data_bit_width);
    *p_sc_unsigned = value.c_str();
    XTSC_DEBUG(m_text, "new_sc_unsigned @" << p_sc_unsigned << " = " << hex << "0x" << *p_sc_unsigned << " (new)");
    return p_sc_unsigned;
  }
  else {
    sc_unsigned *p_sc_unsigned = m_sc_unsigned_pool.back();
    m_sc_unsigned_pool.pop_back();
    *p_sc_unsigned = value.c_str();
    XTSC_DEBUG(m_text, "new_sc_unsigned @" << p_sc_unsigned << " = " << hex << "0x" << *p_sc_unsigned << " (recycled)");
    return p_sc_unsigned;
  }
}

sc_unsigned *xtsc_component::xtsc_ext_regfile::new_sc_unsigned(const sc_unsigned& value) {
  if (m_sc_unsigned_pool.empty()) {
    sc_unsigned *p_sc_unsigned = new sc_unsigned(m_data_bit_width);
    *p_sc_unsigned = value;
    XTSC_DEBUG(m_text, "new_sc_unsigned @" << p_sc_unsigned << " = " << hex << "0x" << *p_sc_unsigned << " (new)");
    return p_sc_unsigned;
  }
  else {
    sc_unsigned *p_sc_unsigned = m_sc_unsigned_pool.back();
    m_sc_unsigned_pool.pop_back();
    *p_sc_unsigned = value;
    XTSC_DEBUG(m_text, "new_sc_unsigned @" << p_sc_unsigned << " = " << hex << "0x" << *p_sc_unsigned << " (recycled)");
    return p_sc_unsigned;
  }
}

void xtsc_component::xtsc_ext_regfile::delete_sc_unsigned(sc_unsigned*& p_sc_unsigned) {
  XTSC_DEBUG(m_text, "delete_sc_unsigned @" << p_sc_unsigned << " = " << hex << "0x" << *p_sc_unsigned);
  m_sc_unsigned_pool.push_back(p_sc_unsigned);
  p_sc_unsigned = 0;
}

void xtsc_component::xtsc_ext_regfile::xtsc_ext_regfile_if_impl::nb_send_write_address_and_data(
                                                xtsc::u32 address, const sc_dt::sc_unsigned& data)
{
  XTSC_DEBUG(m_ext_regfile.m_text, "nb_send_write_address_and_data: wr_addr=0x" << hex << address << dec
      << " wr_data=" << data.to_string(SC_HEX));
  m_ext_regfile.m_wr_addr = address;
  m_ext_regfile.m_wr_data = data;
  m_ext_regfile.do_write();
}

void xtsc_component::xtsc_ext_regfile::xtsc_ext_regfile_if_impl::nb_send_read_check_address(xtsc::u32 address)
{
  XTSC_DEBUG(m_ext_regfile.m_text, "nb_send_read_check_address: addr=0x" << hex << address << dec);
  m_ext_regfile.m_rd_chk_addr = address;
}

bool xtsc_component::xtsc_ext_regfile::xtsc_ext_regfile_if_impl::nb_is_read_data_ready()
{
  bool rd_data_rdy = (m_ext_regfile.m_op_fini_time[m_ext_regfile.m_rd_chk_addr] <= sc_time_stamp());
  XTSC_DEBUG(m_ext_regfile.m_text, "nb_is_read_data_ready: rd_data_rdy=" << rd_data_rdy);
  return rd_data_rdy;
}

void xtsc_component::xtsc_ext_regfile::xtsc_ext_regfile_if_impl::nb_send_read_address(xtsc::u32 address)
{
  XTSC_DEBUG(m_ext_regfile.m_text, "nb_send_read_address: addr=0x" << hex << address << dec);
  m_ext_regfile.m_rd_addr = address;
}

sc_dt::sc_unsigned xtsc_component::xtsc_ext_regfile::xtsc_ext_regfile_if_impl::nb_get_read_data()
{
  XTSC_DEBUG(m_ext_regfile.m_text, "nb_get_read_data: rd_data=" << m_ext_regfile.m_data_map[m_ext_regfile.m_rd_addr].to_string(SC_HEX));
  return m_ext_regfile.m_data_map[m_ext_regfile.m_rd_addr];
}

void xtsc_component::xtsc_ext_regfile::xtsc_ext_regfile_if_impl::register_port(sc_port_base& port, const char *if_typename) {
  if (m_p_port) {
    ostringstream oss;
    oss << "Illegal multiple binding detected to xtsc_ext_regfile '" << m_ext_regfile.name() << "' m_ext_regfile export: " << endl;
    oss << "  " << port.name() << endl;
    oss << "  " << m_p_port->name();
    throw xtsc_exception(oss.str());
  }
  XTSC_INFO(m_ext_regfile.m_text, "Binding '" << port.name() << "' to xtsc_ext_regfile::m_ext_regfile");
  m_p_port = &port;
}



void xtsc_component::xtsc_ext_regfile::validate_address(const string& address) {
  bool okay = false;
  u32 nibbles = ((m_address_bit_width) + 3) / 4;
  if (address.length() == nibbles + 2) {
    if (address.substr(0, 2) == "0x") {
      if (address.substr(2).find_first_not_of("0123456789abcdef") == string::npos) {
        okay = true;
      }
    }
  }
  if (!okay) {
    ostringstream oss;
    oss << "xtsc_ext_regfile '" << m_ext_regfile.name() << "': Invalid peek/poke address '" << address << "'";
    throw xtsc_exception(oss.str());

  }
}


//TODO: May be implemented later
void xtsc_component::xtsc_ext_regfile::dump(ostream &os) {
  //map<string, sc_unsigned*>::iterator imap = m_data_map.begin();
  //for (; imap != m_data_map.end(); ++imap) {
  //  string p_address = (*imap).first;
  //  sc_unsigned *p_data = (*imap).second;
  //  os << p_address << ": 0x" << p_data->to_string(SC_HEX).substr(3) << endl;
  //}
}



//TODO: May be implemented later
string xtsc_component::xtsc_ext_regfile::peek(const string& address) {
  //validate_address(address);
  //map<string, sc_unsigned*>::iterator imap = m_data_map.find(address);
  //if (imap == m_data_map.end()) {
  //  return "";
  //}
  //else {
  //  ostringstream oss;
  //  oss << "0x" << imap->second->to_string(SC_HEX).substr(3);
  //  return oss.str();
  //}
  return "";
}



//TODO: May be implemented later
void xtsc_component::xtsc_ext_regfile::poke(const string& address, const string& data) {
  //validate_address(address);
  //map<string, sc_unsigned*>::iterator imap = m_data_map.find(address);
  //if (imap == m_data_map.end()) {
  //  m_poke_data = data.c_str();
  //  sc_unsigned *p_data = new_sc_unsigned(m_poke_data);
  //  m_data_map[address] = p_data;
  //}
  //else {
  //  *(imap->second) = data.c_str();
  //}
}



void xtsc_component::xtsc_ext_regfile::end_of_simulation() {
  //EMPTY 
}



