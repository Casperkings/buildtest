// Copyright (c) 2006-2012 by Tensilica Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Tensilica Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Tensilica Inc.

// m_time_resolution\|\w*clock_period\w*\|\w*posedge_offset\w*\|m_delay\|m_ready

#include <cerrno>
#include <algorithm>
#include <ostream>
#include <string>
#include <xtsc/xtsc_lookup.h>
#include <xtsc/xtsc_core.h>
#include <xtsc/xtsc_lookup_driver.h>
#include <xtsc/xtsc_logging.h>


// xtsc_lookup does binary logging of lookup events at verbose log level and xtsc_core does it
// at info log level.  This is the reverse of the normal arrangement because xtsc_core knows the
// program counter (PC) and the port number, but xtsc_lookup knows neither.



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
using log4xtensa::READY;
using log4xtensa::RESPONSE_VALUE;


static sc_unsigned sc_unsigned_zero(1);
static sc_unsigned sc_unsigned_one (1);


xtsc_component::xtsc_lookup_parms::xtsc_lookup_parms(const xtsc_core&      core, 
                                                     const char           *lookup_name,
                                                     const char           *lookup_table,
                                                     const char           *default_data,
                                                     bool                  ram)
{
  u32   address_bit_width       = core.get_lookup_address_bit_width(lookup_name);
  u32   data_bit_width          = core.get_lookup_data_bit_width(lookup_name);
  u32   latency                 = core.get_lookup_latency(lookup_name);
  bool  has_ready               = core.has_lookup_ready(lookup_name);
  init(address_bit_width, data_bit_width, has_ready, lookup_table, default_data, ram);
  set("latency", latency);
  set("clock_period", core.get_parms().get_u32("SimClockFactor")*xtsc_get_system_clock_factor());
}



xtsc_component::xtsc_lookup::xtsc_lookup(sc_module_name module_name, const xtsc_lookup_parms& lookup_parms) :
  sc_module             (module_name),
  xtsc_module           (*(sc_module*)this),
  m_lookup              ("m_lookup"),
  m_lookup_impl         ("m_lookup_impl", *this),
  m_lookup_parms        (lookup_parms),
  m_address_bit_width   (lookup_parms.get_non_zero_u32("address_bit_width")),
  m_ram_address_bits    (0),
  m_data_bit_width      (lookup_parms.get_non_zero_u32("data_bit_width")),
  m_latency             (lookup_parms.get_non_zero_u32("latency")),
  m_ready_enable        (true),
  m_default_data        (m_data_bit_width),
  m_lua_data_function   (""),
  m_lua_delay_function  (""),
  m_lua_function        (false),
  m_data                (m_data_bit_width),
  m_data_temp           (m_data_bit_width),
  m_old_data            (m_data_bit_width),
  m_poke_data           (m_data_bit_width),
  m_address             (m_address_bit_width),
#if IEEE_1666_SYSTEMC >= 201101L
  m_lookup_ready_event  ("m_lookup_ready_event"),
#endif
  m_zero                (1),
  m_one                 (1),
  m_nb_send_address_cnt (0),
  m_nb_is_ready_cnt     (0),
  m_nb_get_data_cnt     (0),
  m_address_trace       (m_address_bit_width),
  m_data_trace          (m_data_bit_width),
  m_p_ram_addr          (NULL),
  m_p_effective_ram_addr(NULL),
  m_text                (log4xtensa::TextLogger::getInstance(name())),
  m_binary              (log4xtensa::BinaryLogger::getInstance(name()))
{

  m_lookup(m_lookup_impl);

  sc_unsigned_zero      = 0;
  sc_unsigned_one       = 1;

  m_ram                 = lookup_parms.get_bool("ram");
  m_override_lookup     = lookup_parms.get_bool("override_lookup");
  m_write_strobe_bit    = 0;
  m_active_high_strobe  = lookup_parms.get_bool("active_high_strobe");
  m_ram_address_lsb     = 0;
  m_ram_address_msb     = 0;
  m_write_data_lsb      = 0;
  m_write_data_msb      = 0;
  m_ram_write_enables   = lookup_parms.get_u32_vector("ram_write_enables");
  m_has_ready           = lookup_parms.get_bool("has_ready");
  m_default_data        = lookup_parms.get_c_str("default_data");
  m_zero                = 0;
  m_one                 = 1;
  m_file_logged         = false;
  m_file                = 0;
  m_p_trace_file        = static_cast<sc_trace_file*>(const_cast<void*>(lookup_parms.get_void_pointer("vcd_handle")));
  m_dump_after_sim      = lookup_parms.get_bool("dump_after_sim"); 

  if (m_ram && m_override_lookup) {
    ostringstream oss;
    oss << "xtsc_lookup '" << name() << "' parameter error: \"ram\" and \"override_lookup\" cannot both be true";
    throw xtsc_exception(oss.str());
  }

  m_has_ram_write_enables = (m_ram_write_enables.size() != 0);
  if (m_has_ram_write_enables) {
    if (!m_ram) {
      ostringstream oss;
      oss << "xtsc_lookup '" << name() << "': \"ram\" must be true if \"ram_write_enables\" is set";
      throw xtsc_exception(oss.str());
    }
    if (m_ram_write_enables.size() % 4) {
      ostringstream oss;
      oss << "xtsc_lookup '" << name() << "': size of \"ram_write_enables\" (" << m_ram_write_enables.size()
          << ") must be a multiple of 4";
      throw xtsc_exception(oss.str());
    }
  }

  // Get pipeline depth
  u32 pipeline_depth    = lookup_parms.get_u32("pipeline_depth");
  m_pipeline_depth      = (pipeline_depth ? pipeline_depth : m_latency + 1);

  // Get RAM parameters
  u32 write_strobe_bit  = lookup_parms.get_u32("write_strobe_bit");
  if (m_ram) {
    m_write_strobe_bit  = write_strobe_bit;
    m_write_data_lsb    = lookup_parms.get_u32("write_data_lsb");
    if (m_data_bit_width + 1 >= m_address_bit_width) {
      ostringstream oss;
      oss << "xtsc_lookup '" << name() << "' parameter error with \"ram\"=true:" << endl;
      oss << "  \"address_bit_width\" (=" << m_address_bit_width << ") must be greater than \"data_bit_width\" (="
          << m_data_bit_width << ") + 1. ";
      throw xtsc_exception(oss.str());
    }
    u32 inconsistent = 0;
    m_ram_address_bits = m_address_bit_width - m_data_bit_width - 1;
    if (m_write_data_lsb == 0) {
      if (write_strobe_bit == 0xFFFFFFFF) {
        m_write_strobe_bit = m_address_bit_width - 1;
        m_ram_address_lsb  = m_data_bit_width;
      }
      else {
        if ((write_strobe_bit != m_data_bit_width) && (write_strobe_bit != m_address_bit_width - 1)) {
          inconsistent = __LINE__;
        }
        m_ram_address_lsb  = ((write_strobe_bit == m_data_bit_width) ? (m_data_bit_width + 1) : m_data_bit_width);
      }
    }
    else if (m_write_data_lsb == 1) {
      if (write_strobe_bit == 0xFFFFFFFF) {
        m_write_strobe_bit = (m_ram_address_bits == 1) ? (m_address_bit_width - 1) : 0;
        m_ram_address_lsb  = (m_ram_address_bits == 1) ? 0 : (m_address_bit_width - 1);
      }
      else {
        if ((write_strobe_bit != 0) && ((write_strobe_bit != m_address_bit_width - 1) || (m_ram_address_bits != 1))) {
          inconsistent = __LINE__;
        }
        m_ram_address_lsb  = ((write_strobe_bit == 0) ? (m_data_bit_width + 1) : 0);
      }
    }
    else if (m_write_data_lsb == m_ram_address_bits) {
      if (write_strobe_bit == 0xFFFFFFFF) {
        m_write_strobe_bit = m_address_bit_width - 1;
        m_ram_address_lsb  = 0;
      }
      else {
        if ((write_strobe_bit != m_address_bit_width - 1) && ((write_strobe_bit != 0) || (m_ram_address_bits != 1))) {
          inconsistent = __LINE__;
        }
        m_ram_address_lsb  = ((write_strobe_bit == 0) ? (m_address_bit_width - 1) : 0);
      }
    }
    else if (m_write_data_lsb == m_ram_address_bits + 1) {
      if (write_strobe_bit == 0xFFFFFFFF) {
        m_write_strobe_bit = m_ram_address_bits;
        m_ram_address_lsb  = 0;
      }
      else {
        if ((write_strobe_bit != 0) && (write_strobe_bit != m_write_data_lsb - 1)) {
          inconsistent = __LINE__;
        }
        m_ram_address_lsb  = ((write_strobe_bit == 0) ? 1 : 0);
      }
    }
    else {
      ostringstream oss;
      oss << "xtsc_lookup '" << name() << "' parameter error with \"ram\"=true:" << endl;
      oss << "  \"write_data_lsb\" (" << m_write_data_lsb << ") does not equal one of the four legal values: " << endl;
      oss << "    0, 1, " << m_ram_address_bits << "=\"address_bit_width\"-\"data_bit_width\"-1, or "
          << (m_ram_address_bits+1) << "=\"address_bit_width\"-\"data_bit_width\"" << endl;
      oss << "    Note: \"address_bit_width\"=" << m_address_bit_width << " and \"data_bit_width\'=" << m_data_bit_width;
      throw xtsc_exception(oss.str());
    }
    if (inconsistent) {
      ostringstream oss;
      oss << "xtsc_lookup '" << name() << "' parameter error with \"ram\"=true:" << endl;
      oss << "  The following parameters are inconsistent such that the RAM address, write data, and write strobe" << endl;
      oss << "  do not exactly fill the lookup address field:" << endl;
      oss << "    \"address_bit_width\" = " << m_address_bit_width << endl;
      oss << "    \"data_bit_width\'    = " << m_data_bit_width << endl;
      oss << "    \"write_data_lsb\"    = " << m_write_data_lsb << endl;
      oss << "    \"write_strobe_bit\"  = " << m_write_strobe_bit << endl;
      oss << "  Note: The inconsistency was detected in line " << inconsistent << " of " << __FILE__ << endl;
      throw xtsc_exception(oss.str());
    }
    m_write_data_msb    = m_write_data_lsb  + m_data_bit_width      - 1;
    m_ram_address_msb   = m_ram_address_lsb + m_ram_address_bits - 1;
    if ((m_write_strobe_bit >= m_address_bit_width) ||
        (m_ram_address_msb  >= m_address_bit_width) ||
        (m_write_data_msb   >= m_address_bit_width) ||
        ((m_ram_address_msb - m_ram_address_lsb + 1) + (m_write_data_msb - m_write_data_lsb + 1) + 1 != m_address_bit_width))
    {
      ostringstream oss;
      oss << "xtsc_lookup '" << name() << "' PROGRAM BUG: calculating RAM address, write data, and write strobe bit fields." << endl;
      oss << "PROGRAM BUG: In line " << __LINE__ << " of file " << __FILE__ << endl;
      throw xtsc_exception(oss.str());
    }
    m_p_ram_addr           = new sc_unsigned(m_ram_address_bits);
    m_p_effective_ram_addr = new sc_unsigned(m_ram_address_bits);
    if (m_has_ram_write_enables) {
      m_data_temp = 0;
      for (u32 i=0; i<m_ram_write_enables.size(); i+=4) {
        if (m_ram_write_enables[i+0] >= m_ram_address_bits) {
          ostringstream oss;
          oss << "xtsc_lookup '" << name() << "': ram_write_enables[" << i << "]=" << m_ram_write_enables[i]
              << " is outside the full RAM address sub-field range of 0-" << (m_ram_address_bits-1);
          throw xtsc_exception(oss.str());
        }
        if (m_ram_write_enables[i+1] >= m_data_bit_width) {
          ostringstream oss;
          oss << "xtsc_lookup '" << name() << "': ram_write_enables[" << (i+1) << "]=" << m_ram_write_enables[i+1]
              << " is outside the write data sub-field range of 0-" << (m_data_bit_width-1);
          throw xtsc_exception(oss.str());
        }
        if (m_ram_write_enables[i+2] >= m_data_bit_width) {
          ostringstream oss;
          oss << "xtsc_lookup '" << name() << "': ram_write_enables[" << (i+2) << "]=" << m_ram_write_enables[i+1]
              << " is outside the write data sub-field range of 0-" << (m_data_bit_width-1);
          throw xtsc_exception(oss.str());
        }
        if (m_ram_write_enables[i+1] > m_ram_write_enables[i+2]) {
          ostringstream oss;
          oss << "xtsc_lookup '" << name() << "': ram_write_enables[" << (i+1) << "]=" << m_ram_write_enables[i+1]
              << " may not exceed ram_write_enables[" << (i+2) << "]=" << m_ram_write_enables[i+2];
          throw xtsc_exception(oss.str());
        }
        if (m_ram_write_enables[i+3] > 1) {
          ostringstream oss;
          oss << "xtsc_lookup '" << name() << "': ram_write_enables[" << (i+3) << "]=" << m_ram_write_enables[i+3]
              << " must be either 0 (active low enable) or 1 (active high enable)";
          throw xtsc_exception(oss.str());
        }
        for (u32 j=m_ram_write_enables[i+1]; j<=m_ram_write_enables[i+2]; ++j) {
          if (m_data_temp[j]) {
            ostringstream oss;
            oss << "xtsc_lookup '" << name() << "': quartet #" << (i/4+1) << " defines an enable for write data bits "
                << m_ram_write_enables[i+1] << "-" << m_ram_write_enables[i+2]
                << "; but at least some of those bits are controlled by an enable defined by an earlier quartet.";
            throw xtsc_exception(oss.str());
          }
          m_data_temp[j] = true;
        }
      }
    }
  }

  // Get clock period 
  m_time_resolution     = sc_get_time_resolution();
  u32 clock_period = lookup_parms.get_non_zero_u32("clock_period");
  if (clock_period == 0xFFFFFFFF) {
    m_clock_period = xtsc_get_system_clock_period();
  }
  else {
    m_clock_period = sc_get_time_resolution() * clock_period;
  }

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

  compute_delays();

  m_enforce_latency     = lookup_parms.get_bool("enforce_latency");
  m_delay               = lookup_parms.get_u32("delay");
  m_ready               = (m_delay ? false : true);

  if (m_delay && !m_has_ready) {
    ostringstream oss;
    oss << "xtsc_lookup '" << name() << "' parameter error:" << endl;
    oss << "  \"delay\" must be 0 if \"has_ready\" is false";
    throw xtsc_exception(oss.str());
  }

  if (m_p_trace_file) {
    sc_trace(m_p_trace_file, m_nb_send_address_cnt, *new string(name()) + ".nb_send_address_cnt");
    sc_trace(m_p_trace_file, m_address_trace,       *new string(name()) + ".address");
    sc_trace(m_p_trace_file, m_nb_get_data_cnt,     *new string(name()) + ".nb_get_data_cnt");
    sc_trace(m_p_trace_file, m_data_trace,          *new string(name()) + ".data");
    if (m_has_ready) {
    sc_trace(m_p_trace_file, m_nb_is_ready_cnt,     *new string(name()) + ".nb_is_ready_cnt");
    sc_trace(m_p_trace_file, m_ready,               *new string(name()) + ".rdy");
    }
    if (m_ram) {
    sc_trace(m_p_trace_file, m_write,               *new string(name()) + ".ram_write");
    sc_trace(m_p_trace_file, m_data,                *new string(name()) + ".ram_write_data");
    sc_trace(m_p_trace_file, *m_p_ram_addr,         *new string(name()) + ".ram_address");
    }
  }

  const char *lookup_table = lookup_parms.get_c_str("lookup_table");

  if (!lookup_table || !lookup_table[0]) {
    // No lookup_table specified, all addresses will return m_default_data
    m_lookup_table = "";
  }
  else {
    m_file = new xtsc_script_file(lookup_table, "lookup_table",  name(), kind(), false);
    m_lookup_table = m_file->name();
  }

  m_port_types["m_lookup"] = LOOKUP_EXPORT;
  m_port_types["lookup"  ] = PORT_TABLE;

  xtsc_register_command(*this, *this, "change_clock_period", 1, 1,
      "change_clock_period <ClockPeriodFactor>", 
      "Call xtsc_lookup::change_clock_period(<ClockPeriodFactor>).  Return previous <ClockPeriodFactor> for this lookup."
  );

  xtsc_register_command(*this, *this, "dump", 0, 0,
      "dump", 
      "Return the os buffer from calling xtsc_lookup::dump(os)."
  );

  xtsc_register_command(*this, *this, "get_ready_enable", 0, 0,
      "get_ready_enable", 
      "Return the value from calling xtsc_lookup::get_ready_enable()."
  );

  xtsc_register_command(*this, *this, "peek", 1, 1,
      "peek <Address>", 
      "Call xtsc_lookup::peek(<Address>)."
  );

  xtsc_register_command(*this, *this, "poke", 2, 2,
      "poke <Address> <Value>", 
      "Return xtsc_lookup::poke(<Address>, <Value>)."
  );

  xtsc_register_command(*this, *this, "reset", 0, 1,
      "reset [<Hard>]", 
      "Call xtsc_lookup::reset(<Hard>).  Where <Hard> is 0|1 (default 0)."
  );

  xtsc_register_command(*this, *this, "set_ready_enable", 1, 1,
      "set_ready_enable <ReadyEnable>", 
      "Call xtsc_lookup::set_ready_enable(<ReadyEnable>)."
  );

#if IEEE_1666_SYSTEMC < 201101L
  xtsc_event_register(m_lookup_ready_event, "m_lookup_ready_event", this);
#endif

  log4xtensa::LogLevel ll = xtsc_get_constructor_log_level();
  XTSC_LOG(m_text, ll, "Constructed xtsc_lookup '" << name() << "':");
  XTSC_LOG(m_text, ll, " ram                     = "   << boolalpha << m_ram);
  XTSC_LOG(m_text, ll, " address_bit_width       = "   << m_address_bit_width);
  XTSC_LOG(m_text, ll, " data_bit_width          = "   << m_data_bit_width);
  if (m_ram) {
  XTSC_LOG(m_text, ll, " write_data_lsb          = "   << m_write_data_lsb);
  XTSC_LOG(m_text, ll, " Write data bitfield     = ["  << m_write_data_msb << ":" << m_write_data_lsb << "]");
  XTSC_LOG(m_text, ll, " RAM address bitfield    = ["  << m_ram_address_msb << ":" << m_ram_address_lsb << "]");
  if (write_strobe_bit == 0xFFFFFFFF) {
  XTSC_LOG(m_text, ll, " write_strobe_bit        = 0xFFFFFFFF => " << m_write_strobe_bit);
  } else {
  XTSC_LOG(m_text, ll, " write_strobe_bit        = "   << m_write_strobe_bit);
  }
  XTSC_LOG(m_text, ll, " active_high_strobe      = "   << boolalpha << m_active_high_strobe);
  ostringstream oss; for (u32 i=0; i<m_ram_write_enables.size(); ++i) { if (i) oss << ","; oss << m_ram_write_enables[i]; }
  XTSC_LOG(m_text, ll, " ram_write_enables       = "   << oss.str());
  }
  XTSC_LOG(m_text, ll, " has_ready               = "   << boolalpha << m_has_ready);
  if (pipeline_depth) {
  XTSC_LOG(m_text, ll, " pipeline_depth          = "   << pipeline_depth);
  } else {
  XTSC_LOG(m_text, ll, " pipeline_depth          = "   << pipeline_depth << " => " << m_pipeline_depth);
  }
  XTSC_LOG(m_text, ll, " enforce_latency         = "   << boolalpha << m_enforce_latency);
  XTSC_LOG(m_text, ll, " latency                 = "   << m_latency);
  XTSC_LOG(m_text, ll, " delay                   = "   << m_delay);
  XTSC_LOG(m_text, ll, " lookup_table            = "   << m_lookup_table);
  XTSC_LOG(m_text, ll, " default_data            = 0x" << m_default_data.to_string(SC_HEX).substr(m_data_bit_width%4 ? 2 : 3));
  XTSC_LOG(m_text, ll, " override_lookup         = "   << boolalpha << m_override_lookup);
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
  XTSC_LOG(m_text, ll, " dump_after_sim          = "   << boolalpha << m_dump_after_sim);

  reset(true);

  if (m_lua_function) {
  XTSC_LOG(m_text, ll, " <LuaDataFunction>       = "   << m_lua_data_function);
  XTSC_LOG(m_text, ll, " <LuaDelayFunction>      = "   << m_lua_delay_function);
  }
}



xtsc_component::xtsc_lookup::~xtsc_lookup(void) {
  map<string, sc_unsigned*>::iterator imap = m_data_map.begin();
  for (; imap != m_data_map.end(); ++imap) {
    sc_unsigned *p_data = (*imap).second;
    delete p_data;
  }
}



u32 xtsc_component::xtsc_lookup::get_bit_width(const string& port_name, u32 interface_num) const {
  return ((interface_num == 0) ? m_data_bit_width : m_address_bit_width);
}



sc_object *xtsc_component::xtsc_lookup::get_port(const string& port_name) {
  if (port_name != "m_lookup") {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\" has no port named \"" << port_name << "\"" << endl;
    throw xtsc_exception(oss.str());
  }

  return (sc_object*) &m_lookup;
}



xtsc_port_table xtsc_component::xtsc_lookup::get_port_table(const string& port_table_name) const {
  if (port_table_name != "lookup") {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\" has no port table named \"" << port_table_name << "\"" << endl;
    throw xtsc_exception(oss.str());
  }

  xtsc_port_table table;
  table.push_back("m_lookup");

  return table;
}



void xtsc_component::xtsc_lookup::reset(bool hard_reset) {
  XTSC_INFO(m_text, "xtsc_lookup::reset()");

  m_ready_enable        = true;
  m_ready_net_time      = SC_ZERO_TIME;
  m_line_count          = 0;
  m_address             = 0;
  m_write               = false;
  m_delay_next          = m_delay;

  m_lookup_ready_event.cancel();

  m_data_fifo.clear();
  m_cycle_fifo.clear();

  for (deque<sc_unsigned*>::iterator i = m_recycle_fifo.begin(); i != m_recycle_fifo.end(); ++i) {
    sc_unsigned *p_data = *i;
    delete_sc_unsigned(p_data);
  }
  m_recycle_fifo.clear();

  if (m_file && hard_reset) {
    m_file->reset();
    for (map<string, sc_unsigned*>::iterator imap = m_data_map.begin(); imap != m_data_map.end(); ++imap) {
      sc_unsigned *p_data = (*imap).second;
      delete_sc_unsigned(p_data);
    }
    m_data_map.clear();

    if (!m_file_logged) {
      XTSC_LOG(m_text, xtsc_get_constructor_log_level(), "Loading lookup table from file '" << m_lookup_table << "'.");
    }

    u32 address_bits = (m_ram ? m_ram_address_bits : m_address_bit_width);
    sc_unsigned next_address(address_bits);
    next_address = 0;

    while ((m_line_count = m_file->get_words(m_words, m_line, false)) != 0) {

      u32 num_words = m_words.size();

      if ((num_words < 1) || (num_words > 3)) {
        ostringstream oss;
        oss << "Invalid number of words (expected 1, 2, or 3):" << endl;
        oss << m_line;
        oss << m_file->info_for_exception();
        throw xtsc_exception(oss.str());
      }

      if (m_words[0] == "lua_function") {
        if (num_words == 1) {
          ostringstream oss;
          oss << "Empty lua_function line.  Format is: lua_function <LuaDataFunction> [<LuaDelayFunction>]" << endl;
          oss << m_line;
          oss << m_file->info_for_exception();
          throw xtsc_exception(oss.str());
        }
        m_lua_data_function = m_words[1];
        if (num_words == 3) {
          if (!m_has_ready) {
            ostringstream oss;
            oss << "<LuaDelayFunction> not allowed on lua_function line when \"has_ready\" is false." << endl;
            oss << m_line;
            oss << m_file->info_for_exception();
            throw xtsc_exception(oss.str());
          }
          m_lua_delay_function = m_words[2];
        }
        m_lua_function = true;
        continue;
      }

      sc_unsigned address(address_bits);
      sc_unsigned *p_data = new_sc_unsigned(m_zero);
      u32 delay = m_delay;
      if (num_words == 1) {
        address = next_address;
        get_sc_unsigned(0, *p_data);
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
          get_sc_unsigned(num_words-2, *p_data);
          if (num_words == 2) {
            address = next_address;
          }
          else {
            get_sc_unsigned(0, address);
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
          get_sc_unsigned(0, address);
          get_sc_unsigned(1, *p_data);
        }
      }

      ostringstream oss;
      oss << "0x" << address.to_string(SC_HEX).substr(address_bits%4 ? 2 : 3);
      map<string, sc_unsigned*>::iterator imap = m_data_map.find(oss.str());
      if (imap == m_data_map.end()) {
        m_data_map[oss.str()] = p_data;
        m_delay_map[oss.str()] = delay;
      }
      else {
        ostringstream oss;
        oss << "Found duplicate address=0x" << address.to_string(SC_HEX).substr(address_bits%4 ? 2 : 3) << ":" << endl;
        oss << m_line;
        oss << m_file->info_for_exception();
        throw xtsc_exception(oss.str());
      }

      next_address = address + 1;
      if (!m_file_logged) {
        XTSC_VERBOSE(m_text, "Line " << dec << m_line_count << ": 0x" << address.to_string(SC_HEX).substr(address_bits%4 ? 2 : 3) <<
                             " => 0x" << p_data->to_string(SC_HEX).substr(m_data_bit_width%4 ? 2 : 3));
      }
    }

    m_file_logged = true;

  }

  m_lookup_ready_event.cancel();
}



void xtsc_component::xtsc_lookup::change_clock_period(u32 clock_period_factor) {
  m_clock_period = sc_get_time_resolution() * clock_period_factor;
  XTSC_INFO(m_text, "change_clock_period(" << clock_period_factor << ") => " << m_clock_period);
  compute_delays();
}



void xtsc_component::xtsc_lookup::set_ready_enable(bool ready_enable) {
  if (!m_has_ready) {
    ostringstream oss;
    oss << "xtsc_lookup '" << m_lookup.name() << "': Illegal call to set_ready_enable() (\"has_ready\" is false)";
    throw xtsc_exception(oss.str());
  }
  XTSC_INFO(m_text, "set_ready_enable(" << ready_enable << ")");
  m_ready_enable = ready_enable;
}



bool xtsc_component::xtsc_lookup::get_ready_enable() const {
  if (!m_has_ready) {
    ostringstream oss;
    oss << "xtsc_lookup '" << m_lookup.name() << "': Illegal call to get_ready_enable() (\"has_ready\" is false)";
    throw xtsc_exception(oss.str());
  }
  return m_ready_enable;
}



void xtsc_component::xtsc_lookup::compute_delays() {
  m_clock_period_value  = m_clock_period.value();
  u32 posedge_offset = m_lookup_parms.get_u32("posedge_offset");
  if (m_posedge_offset >= m_clock_period) {
    ostringstream oss;
    oss << kind() << " '" << name() << "' parameter error:" << endl;
    oss << "\"posedge_offset\" (0x" << hex << posedge_offset << "=>" << m_posedge_offset
        << ") must be strictly less than clock period => " << m_clock_period << ")";
    throw xtsc_exception(oss.str());
  }
}



void xtsc_component::xtsc_lookup::execute(const string&                 cmd_line, 
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
  else if (words[0] == "get_ready_enable") {
    res << get_ready_enable();
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
  else if (words[0] == "set_ready_enable") {
    set_ready_enable(xtsc_command_argtobool(cmd_line, words, 1));
  }
  else {
    ostringstream oss;
    oss << "xtsc_lookup::execute called for unknown command '" << cmd_line << "'.";
    throw xtsc_exception(oss.str());
  }

  result << res.str();
}



void xtsc_component::xtsc_lookup::connect(xtsc_core& core, const char *lookup_name) {
  core.get_lookup(lookup_name)(m_lookup);
}



void xtsc_component::xtsc_lookup::connect(xtsc_lookup_driver& driver) {
  driver.m_lookup(m_lookup);
}



void xtsc_component::xtsc_lookup::get_sc_unsigned(u32 index, sc_unsigned& value) {
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



// If desired, a sub-class can override this virtual method 
void xtsc_component::xtsc_lookup::get_data_from_address() {
  ostringstream oss;
  oss << "xtsc_lookup '" << m_lookup.name() << "': When \"override_lookup\" is true "
      << "then you must provide a subclass that overrides the xtsc_lookup::get_data_from_address() method." << endl;
  throw xtsc_exception(oss.str());
}



bool xtsc_component::xtsc_lookup::pipeline_full() {
  bool full = (m_data_fifo.size() >= m_pipeline_depth);
  if (full) {
    XTSC_DEBUG(m_text, "pipeline full.");
  }
  return full;
}



void xtsc_component::xtsc_lookup::do_lookup() {
  if (m_override_lookup) {
    // Sub-class override
    get_data_from_address();
    sc_unsigned *p_data = new_sc_unsigned(m_data);
    m_data_fifo.push_back(p_data);
  }
  else if (m_ram) {
    // Operating as lookup ram device
    *m_p_effective_ram_addr = *m_p_ram_addr;
    if (m_has_ram_write_enables) {
      // Zero any enable bits to make the effective address
      for (u32 i=0; i<m_ram_write_enables.size(); i+=4) {
        (*m_p_effective_ram_addr)[m_ram_write_enables[i]] = 0;
      }
    }
    ostringstream oss;
    oss << "0x" << m_p_effective_ram_addr->to_string(SC_HEX).substr(m_ram_address_bits%4 ? 2 : 3);
    map<string, sc_unsigned*>::iterator imap = m_data_map.find(oss.str());
    if (imap == m_data_map.end()) {
      m_data_map[oss.str()] = new_sc_unsigned(m_default_data);
      imap = m_data_map.find(oss.str());
    }
    sc_unsigned *p_data = new_sc_unsigned(*(imap->second));
    m_data_fifo.push_back(p_data);
    if (m_write) {
      if (m_has_ram_write_enables) {
        // Make a copy of the original contents
        m_data_temp = *(imap->second);
        // Write the entire new value
        *(imap->second) = m_data;
        // Restore the original contents of bit fields that are controlled by an inactive enable bit
        for (u32 i=0; i<m_ram_write_enables.size(); i+=4) {
          if (m_ram_write_enables[i+3] != (*m_p_ram_addr)[m_ram_write_enables[i]]) {
            imap->second->range(m_ram_write_enables[i+2], m_ram_write_enables[i+1]) =
                    m_data_temp(m_ram_write_enables[i+2], m_ram_write_enables[i+1]);
          }
        }
      }
      else {
        *(imap->second) = m_data;
      }
    }
  }
  else {
    // Regular lookup
    ostringstream oss;
    oss << "0x" << m_address.to_string(SC_HEX).substr(m_address_bit_width%4 ? 2 : 3);
    map<string, sc_unsigned*>::iterator imap = m_data_map.find(oss.str());
    if (imap != m_data_map.end()) {
      m_data_fifo.push_back(imap->second);
    }
    else if (m_lua_function) {
      ostringstream exp;
      exp << m_lua_data_function << "(\"" << oss.str() << "\")";
      string result = m_file->evaluate_lua_expression(exp.str());
      XTSC_VERBOSE(m_text, exp.str() << " => " << result);
      sc_unsigned *p_sc_unsigned = new_sc_unsigned(result);
      m_data_fifo.push_back(p_sc_unsigned);
      m_recycle_fifo.push_back(p_sc_unsigned);
    }
    else {
      m_data_fifo.push_back(&m_default_data);
    }
  }
  if (m_enforce_latency) {
    u64 cycle = (sc_time_stamp().value() + m_clock_period_value - m_posedge_offset_value) / m_clock_period_value;
    m_cycle_fifo.push_back(cycle);
    XTSC_DEBUG(m_text, "do_lookup() cycle=" << cycle);
  }
}



sc_unsigned *xtsc_component::xtsc_lookup::new_sc_unsigned(const string& value) {
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



sc_unsigned *xtsc_component::xtsc_lookup::new_sc_unsigned(const sc_unsigned& value) {
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



void xtsc_component::xtsc_lookup::delete_sc_unsigned(sc_unsigned*& p_sc_unsigned) {
  XTSC_DEBUG(m_text, "delete_sc_unsigned @" << p_sc_unsigned << " = " << hex << "0x" << *p_sc_unsigned);
  m_sc_unsigned_pool.push_back(p_sc_unsigned);
  p_sc_unsigned = 0;
}



void xtsc_component::xtsc_lookup::xtsc_lookup_if_impl::nb_send_address(const sc_unsigned& address) {
  XTSC_DEBUG(m_lookup.m_text, "nb_send_address(0x" << address.to_string(SC_HEX).substr(m_lookup.m_address_bit_width%4 ? 2 : 3));
  if (m_lookup.m_p_trace_file) {
    m_lookup.m_address_trace = address;
    m_lookup.m_nb_send_address_cnt += 1;
  }
  if (m_lookup.m_ram) {
    m_lookup.m_write      = m_lookup.m_active_high_strobe != (address[m_lookup.m_write_strobe_bit] == sc_unsigned_zero);
    m_lookup.m_data       = address(m_lookup.m_write_data_msb, m_lookup.m_write_data_lsb);
    m_lookup.m_address    = address(m_lookup.m_ram_address_msb, m_lookup.m_ram_address_lsb);
   *m_lookup.m_p_ram_addr = address(m_lookup.m_ram_address_msb, m_lookup.m_ram_address_lsb);
  }
  else {
    m_lookup.m_address = address;
  }
  if (m_lookup.m_has_ready) {
    ostringstream oss;
    oss << "0x" << m_lookup.m_address.to_string(SC_HEX).substr(m_lookup.m_address_bit_width%4 ? 2 : 3);
    map<string, u32>::iterator id = m_lookup.m_delay_map.find(oss.str());
    if (id != m_lookup.m_delay_map.end()) {
      m_lookup.m_delay_next = id->second;
    }
    else if (m_lookup.m_lua_function && (m_lookup.m_lua_delay_function != "")) {
      ostringstream exp;
      exp << m_lookup.m_lua_delay_function << "(\"" << oss.str() << "\")";
      string result = m_lookup.m_file->evaluate_lua_expression(exp.str());
      XTSC_VERBOSE(m_lookup.m_text, exp.str() << " => " << result);
      m_lookup.m_delay_next = xtsc_strtou32(result);
    }
    else {
      m_lookup.m_delay_next = m_lookup.m_delay;
    }
    XTSC_DEBUG(m_lookup.m_text, "address=" << oss.str() << " next delay=" << m_lookup.m_delay_next);
  }
  else {
    if (m_lookup.pipeline_full()) {
      ostringstream oss;
      oss << "xtsc_lookup '" << m_lookup.name() << "': nb_send_address() called to do another lookup when pipeline is already full." << endl;
      oss << "\"pipeline_depth\"=" << m_lookup.m_pipeline_depth << endl;
      oss << "\"latency\"=" << m_lookup.m_latency;
      throw xtsc_exception(oss.str());
    }
    m_lookup.do_lookup();
  }
  xtsc_log_lookup_event(m_lookup.m_binary, VERBOSE_LOG_LEVEL, UNKNOWN, LOOKUP_KEY, UNKNOWN_PC, true, address);
  if (m_lookup.m_ram) {
    XTSC_INFO(m_lookup.m_text, "key=" << address.to_string(SC_BIN));
    XTSC_INFO(m_lookup.m_text, "key=0x" << address.to_string(SC_HEX).substr(m_lookup.m_address_bit_width%4 ? 2 : 3) <<
                               " addr=0x" << m_lookup.m_p_ram_addr->to_string(SC_HEX).substr(m_lookup.m_ram_address_bits%4 ? 2 : 3) <<
                               " data=0x" << m_lookup.m_data.to_string(SC_HEX).substr(m_lookup.m_data_bit_width%4 ? 2 : 3) <<
                               " write=" << (m_lookup.m_write ? 1 : 0));
  }
  else {
    XTSC_INFO(m_lookup.m_text, "address=0x" << address.to_string(SC_HEX).substr(m_lookup.m_address_bit_width%4 ? 2 : 3));
  }
}



bool xtsc_component::xtsc_lookup::xtsc_lookup_if_impl::nb_is_ready() {
  if (m_lookup.m_p_trace_file) {
    m_lookup.m_nb_is_ready_cnt += 1;
  }

  if (!m_lookup.m_has_ready) {
    ostringstream oss;
    oss << "xtsc_lookup '" << m_lookup.name() << "': Illegal call to nb_is_ready() (\"has_ready\" is false)";
    throw xtsc_exception(oss.str());
  }

  XTSC_DEBUG(m_lookup.m_text, "nb_is_ready(): pipeline_full()=" << boolalpha << m_lookup.pipeline_full() <<
                              " m_ready_net_time=" << m_lookup.m_ready_net_time);

  if (m_lookup.pipeline_full()) {
    m_lookup.m_ready = false;
  }
  else if (m_lookup.m_has_ready && (sc_time_stamp() < m_lookup.m_ready_net_time)) {
    m_lookup.m_ready = false;
  }
  else {
    m_lookup.m_ready = true;
  }

  bool ready = (m_lookup.m_ready && m_lookup.m_ready_enable);

  if (ready) {
    m_lookup.do_lookup();
    if (m_lookup.m_has_ready) {
      sc_time now               = sc_time_stamp();
      u64     cycle             = now.value() / m_lookup.m_clock_period_value;          // current cycle
      sc_time posedge_clock     = cycle * m_lookup.m_clock_period;                      // posedge clock of current cycle
      sc_time phase_now         = now - posedge_clock;                                  // clock phase now
      sc_time delay_time        = m_lookup.m_clock_period * (m_lookup.m_delay_next + 1);// delay as sc_time
      sc_time ready_delta       = delay_time - phase_now;                               // time to ready cycle
      m_lookup.m_ready_net_time = posedge_clock + delay_time;                           // time of ready cycle
      m_lookup.m_lookup_ready_event.notify(ready_delta);
      XTSC_DEBUG(m_lookup.m_text, "nb_is_ready(): posedge_clock=" << posedge_clock << 
                                  " m_ready_net_time=" << m_lookup.m_ready_net_time);
    }
  }

  xtsc_log_lookup_event(m_lookup.m_binary, VERBOSE_LOG_LEVEL, UNKNOWN, READY, UNKNOWN_PC, true,
                                           (ready ? m_lookup.m_one : m_lookup.m_zero));

  XTSC_DEBUG(m_lookup.m_text, "nb_is_ready() = " << boolalpha << ready);

  return ready;
}



sc_unsigned xtsc_component::xtsc_lookup::xtsc_lookup_if_impl::nb_get_data() {
  XTSC_DEBUG(m_lookup.m_text, "nb_get_data()");

  if (m_lookup.m_data_fifo.empty()) {
    ostringstream oss;
    oss << "xtsc_lookup '" << m_lookup.name() << "': Illegal call to nb_get_data() to read lookup data " << endl;
    oss << "without a corresponding call to ";
    if (m_lookup.m_has_ready) {
      oss << "nb_is_ready() which returned true.";
    }
    else {
      oss << "nb_send_address().";
    }
    throw xtsc_exception(oss.str());
  }

  // Should we do the latency check?
  if (m_lookup.m_enforce_latency) {
    u64 then = m_lookup.m_cycle_fifo[0];
    m_lookup.m_cycle_fifo.pop_front();
    u64 due  = then + m_lookup.m_latency;
    u64 adjusted_time_value  = (sc_time_stamp().value() + m_lookup.m_clock_period_value - m_lookup.m_posedge_offset_value);
    u64 now  = adjusted_time_value / m_lookup.m_clock_period_value;
    XTSC_DEBUG(m_lookup.m_text, "nb_get_data() now=" << now);
    if (due != now) {
      ostringstream oss;
      oss << "xtsc_lookup '" << m_lookup.name() << "' latency timing violation: nb_get_data() called in " << endl;
      oss << "cycle #" << (now-1) << " for lookup data due in cycle #" << (due-1) << endl;
      oss << "If you don't want lookup timing to be enforced, set the \"enforce_latency\" parameter to false.";
      throw xtsc_exception(oss.str());
    }
  }

  sc_unsigned *p_data = m_lookup.m_data_fifo[0];
  m_lookup.m_data_fifo.pop_front();
  m_lookup.m_lookup_ready_event.notify();  // Immediate notification
  xtsc_log_lookup_event(m_lookup.m_binary, VERBOSE_LOG_LEVEL, UNKNOWN, RESPONSE_VALUE, UNKNOWN_PC, true, *p_data);
  XTSC_INFO(m_lookup.m_text, "data=0x" << p_data->to_string(SC_HEX).substr(m_lookup.m_data_bit_width%4 ? 2 : 3));

  bool recycle = (m_lookup.m_ram || m_lookup.m_override_lookup);
  if (m_lookup.m_lua_function) {
    if (m_lookup.m_recycle_fifo.size() && (m_lookup.m_recycle_fifo[0] == p_data)) {
      recycle = true;
      m_lookup.m_recycle_fifo.pop_front();
    }
  }

  if (recycle) {
    m_lookup.m_data_temp = *p_data;
    m_lookup.delete_sc_unsigned(p_data);
    if (m_lookup.m_p_trace_file) {
      m_lookup.m_data_trace = m_lookup.m_data_temp;
      m_lookup.m_nb_get_data_cnt += 1;
    }
    return m_lookup.m_data_temp;
  }
  else {
    if (m_lookup.m_p_trace_file) {
      m_lookup.m_data_trace = *p_data;
      m_lookup.m_nb_get_data_cnt += 1;
    }
    return *p_data;
  }

}



void xtsc_component::xtsc_lookup::xtsc_lookup_if_impl::register_port(sc_port_base& port, const char *if_typename) {
  if (m_p_port) {
    ostringstream oss;
    oss << "Illegal multiple binding detected to xtsc_lookup '" << m_lookup.name() << "' m_lookup export: " << endl;
    oss << "  " << port.name() << endl;
    oss << "  " << m_p_port->name();
    throw xtsc_exception(oss.str());
  }
  XTSC_INFO(m_lookup.m_text, "Binding '" << port.name() << "' to xtsc_lookup::m_lookup");
  m_p_port = &port;
}



void xtsc_component::xtsc_lookup::dump(ostream &os) {
  map<string, sc_unsigned*>::iterator imap = m_data_map.begin();
  for (; imap != m_data_map.end(); ++imap) {
    string p_address = (*imap).first;
    sc_unsigned *p_data = (*imap).second;
    os << p_address << ": 0x" << p_data->to_string(SC_HEX).substr(3) << endl;
  }
}



void xtsc_component::xtsc_lookup::validate_address(const string& address) {
  bool okay = false;
  u32 nibbles = ((m_ram ?  m_ram_address_bits : m_address_bit_width) + 3) / 4;
  if (address.length() == nibbles + 2) {
    if (address.substr(0, 2) == "0x") {
      if (address.substr(2).find_first_not_of("0123456789abcdef") == string::npos) {
        okay = true;
      }
    }
  }
  if (!okay) {
    ostringstream oss;
    oss << "xtsc_lookup '" << m_lookup.name() << "': Invalid peek/poke address '" << address << "'";
    throw xtsc_exception(oss.str());

  }
}



string xtsc_component::xtsc_lookup::peek(const string& address) {
  if (m_override_lookup) {
    ostringstream oss;
    oss << "xtsc_lookup '" << m_lookup.name() << "': When \"override_lookup\" is true "
        << "then you must provide a subclass that overrides the xtsc_lookup::peek() method." << endl;
    throw xtsc_exception(oss.str());
  }
  validate_address(address);
  map<string, sc_unsigned*>::iterator imap = m_data_map.find(address);
  if (imap == m_data_map.end()) {
    return "";
  }
  else {
    ostringstream oss;
    oss << "0x" << imap->second->to_string(SC_HEX).substr(3);
    return oss.str();
  }
}



void xtsc_component::xtsc_lookup::poke(const string& address, const string& data) {
  if (m_override_lookup) {
    ostringstream oss;
    oss << "xtsc_lookup '" << m_lookup.name() << "': When \"override_lookup\" is true "
        << "then you must provide a subclass that overrides the xtsc_lookup::poke() method." << endl;
    throw xtsc_exception(oss.str());
  }
  validate_address(address);
  map<string, sc_unsigned*>::iterator imap = m_data_map.find(address);
  if (imap == m_data_map.end()) {
    m_poke_data = data.c_str();
    sc_unsigned *p_data = new_sc_unsigned(m_poke_data);
    m_data_map[address] = p_data;
  }
  else {
    *(imap->second) = data.c_str();
  }
}



void xtsc_component::xtsc_lookup::end_of_simulation() {

  // Dump lookup values if we were configured to do so
  if (m_dump_after_sim) {
    XTSC_INFO(m_text, "Dump of xtsc_lookup: " << name() << " {");

    ostringstream oss;
    dump(oss);
    xtsc_log_multiline(m_text, INFO_LOG_LEVEL, oss.str(), 0);

    XTSC_INFO(m_text, "} end of dump of xtsc_lookup: " << name());
  }

}



