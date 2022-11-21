// Copyright (c) 2007-2018 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of
// Cadence Design Systems, Inc.  They may be adapted and modified by bona fide
// purchasers for internal use, but neither the original nor any adapted
// or modified version may be disclosed or distributed to third parties
// in any manner, medium, or form, in whole or in part, without the prior
// written consent of Cadence Design Systems, Inc.

#include <iostream>
#include <algorithm>
#include <xtsc/xtsc_response.h>
#include <xtsc/xtsc_fast_access.h>
#include <xtsc/xtsc_mmio.h>
#include <xtsc/xtsc_arbiter.h>
#include <xtsc/xtsc_master.h>
#include <xtsc/xtsc_memory_trace.h>
#include <xtsc/xtsc_pin2tlm_memory_transactor.h>
#include <xtsc/xtsc_router.h>
#include <xtsc/xtsc_wire_logic.h>
#include <xtsc/xtsc_wire_source.h>
#include <xtsc/xtsc_core.h>
#include <xtsc/xtsc_interrupt_distributor.h>


using namespace std;
#if SYSTEMC_VERSION >= 20050601
using namespace sc_core;
#endif
using namespace sc_dt;
using namespace log4xtensa;
using namespace xtsc;



xtsc_component::xtsc_mmio::xtsc_mmio(sc_module_name module_name, const xtsc_mmio_parms& mmio_parms) :
  sc_module             (module_name),
  xtsc_module           (*(sc_module*)this),
  m_request_export      ("m_request_export"),
  m_respond_port        ("m_respond_port"),
  m_mmio_parms          (mmio_parms),
  m_request_impl        ("m_request_impl", *this),
  m_text                (TextLogger::getInstance(name())),
  m_use_fast_access     (mmio_parms.get_bool("use_fast_access")),
#if IEEE_1666_SYSTEMC >= 201101L
  m_request_event       ("m_request_event"),
  m_write_event         ("m_write_event"),
  m_poke_event          ("m_poke_event"),
#endif
  m_definition_file     (mmio_parms.get_non_empty_c_str("definition_file"))
{

  m_busy                = false;
  m_byte_width          = mmio_parms.get_u32("byte_width");
  m_swizzle_bytes       = mmio_parms.get_bool("swizzle_bytes");
  m_always_write        = mmio_parms.get_bool("always_write");

  if ((m_byte_width != 4) && (m_byte_width != 8) && (m_byte_width != 16) && (m_byte_width != 32) && (m_byte_width != 64)) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': Invalid \"byte_width\"= " << m_byte_width << " (legal values are 4|8|16|32|64)";
    throw xtsc_exception(oss.str());
  }

  // Get clock period 
  u32 clock_period = mmio_parms.get_u32("clock_period");
  if (clock_period == 0xFFFFFFFF) {
    m_clock_period = xtsc_get_system_clock_period();
  }
  else {
    m_clock_period = sc_get_time_resolution() * clock_period;
  }

  // Get the response time as a u32 and save it as an sc_time
  u32 response_time = mmio_parms.get_u32("response_time");

  compute_delays();

  // Tell SystemC to run this function in a SystemC thread process
  SC_THREAD(request_thread);  m_process_handles.push_back(sc_get_current_process_handle());

  // Bind the export to the implementation
  m_request_export(m_request_impl);

  // Process script file to get register, sc_port, and sc_export definitions
  m_p_definition_file = new xtsc_script_file(m_definition_file.c_str(), "definition_file",  name(), kind(), false);
  m_definition_file = m_p_definition_file->name();
  log4xtensa::LogLevel ll = xtsc_get_constructor_log_level();

  // First Pass - Ensure first word is valid and process register definitions (leave input and output parsing to second pass)
  //   register <RegisterName> <BitWidth> <Address> {<InitialValue>}
  XTSC_LOG(m_text, ll, "Reading memory-mapped register definitions from file '" << m_definition_file << "'.");
  while ((m_line_count = m_p_definition_file->get_words(m_words, m_line, false))) {
    transform(m_words[0].begin(), m_words[0].end(), m_words[0].begin(), ::tolower);
    if (m_words[0] == "register") {
      register_definition *p_reg = new register_definition(*this);
      p_reg->m_name = validate_identifier(1, "<RegisterName>");
      p_reg->m_bit_width = get_u32(2, "<BitWidth>");
      u32 byte_width = (p_reg->m_bit_width + 7) / 8;
      if ((byte_width == 0) || (byte_width > m_byte_width)) {
        ostringstream oss;
        oss << "<BitWidth>=" << p_reg->m_bit_width << " must be between 1 and \"byte_width\"*8=" << (m_byte_width*8) << ":" << endl;
        oss << m_line;
        oss << m_p_definition_file->info_for_exception();
        throw xtsc_exception(oss.str());
      }
      p_reg->m_address = get_u64(3, "<Address>");
      if (p_reg->m_address % byte_width != 0) {
        ostringstream oss;
        oss << "<Address>=0x" << hex << setfill('0') << setw(xtsc_address_nibbles()) << p_reg->m_address << dec
            << " is not aligned to register size in bytes=" << byte_width << " (from <BitWidth>=" << p_reg->m_bit_width << "):" << endl;
        oss << m_line;
        oss << m_p_definition_file->info_for_exception();
        throw xtsc_exception(oss.str());
      }
      p_reg->m_p_initial_value  = new sc_unsigned(p_reg->m_bit_width);
      p_reg->m_p_previous_value = new sc_unsigned(p_reg->m_bit_width);
      p_reg->m_p_current_value  = new sc_unsigned(p_reg->m_bit_width);
      // Handle 4 cases
      // a) No read_only or InitialValue (i.e only 4 words are specified)
      // b) read_only specified, but no InitialValue
      // c) InitialValue specified, but no read_only
      // d) read_only followed by InitialValue
      if (m_words.size() == 4) {
        *p_reg->m_p_initial_value = 0;
        p_reg->m_read_only = false;
      } 
      else if (m_words[4] == "read_only" && m_words.size() == 5) {
        *p_reg->m_p_initial_value = 0;
        p_reg->m_read_only = true;
      }
      else if ((m_words[4] == "read_only" && m_words.size() == 6) || m_words[4] != "read_only") {
        int ival_idx;
        if (m_words[4] == "read_only") {
          p_reg->m_read_only = true;
          ival_idx = 5;
        }
        else {
          p_reg->m_read_only = false;
          ival_idx = 4;
        }
        // Prevent sign extension (we want 0 fill, not sign extension)
        string initial_value(m_words[ival_idx]);
        if ((initial_value.length() > 2) && (initial_value[0] == '0') && ((initial_value[1] == 'x') || (initial_value[1] == 'X'))) {
          initial_value.insert(2, "0");
        }
        try {
          *p_reg->m_p_initial_value = initial_value.c_str();
        }
        catch (...) {
          ostringstream oss;
          oss << "Cannot convert <InitialValue> argument (#5) '" << m_words[4] << "' to number:" << endl;
          oss << m_line;
          oss << m_p_definition_file->info_for_exception();
          throw xtsc_exception(oss.str());
        }
      }
      *p_reg->m_p_previous_value = 0;
      *p_reg->m_p_current_value  = *p_reg->m_p_initial_value;
      m_register_definition_map.insert(register_definition_map::value_type(p_reg->m_name, p_reg));
      for (u32 i=0; i<byte_width; ++i) {
        xtsc_address address = p_reg->m_address + i;
        address_register_map::iterator ia = m_address_register_map.find(address);
        if (ia != m_address_register_map.end()) {
          ostringstream oss;
          oss << "Address=0x" << hex << setfill('0') << setw(xtsc_address_nibbles()) << address << dec << " already maps to register '"
              << ia->second->m_name << "':" << endl;
          oss << m_line;
          oss << m_p_definition_file->info_for_exception();
          throw xtsc_exception(oss.str());
        }
        m_address_register_map.insert(address_register_map::value_type(address, p_reg));
      }
      XTSC_LOG(m_text, ll, " Register: " << *p_reg);
    }
    else if ((m_words[0] != "input") && (m_words[0] != "output")) {
      ostringstream oss;
      oss << "Syntax error (first word must be register|input|output):" << endl;
      oss << m_line;
      oss << m_p_definition_file->info_for_exception();
      throw xtsc_exception(oss.str());
    }
  }

  // Second Pass - Process output and input definitions
  //   output <PortName> <RegisterName> {<HighBit> {<LowBit>}}
  //   input <ExportName> <RegisterName> {<HighBit> {<LowBit>}}
  m_p_definition_file->reset();
  XTSC_LOG(m_text, ll, "Reading output and input definitions from file '" << m_definition_file << "'.");
  while ((m_line_count = m_p_definition_file->get_words(m_words, m_line, false))) {
    transform(m_words[0].begin(), m_words[0].end(), m_words[0].begin(), ::tolower);
    if ((m_words[0] != "output") && (m_words[0] != "input")) continue;
    bool output = (m_words[0] == "output");
    string io_name  = validate_identifier(1, (output ? "<PortName>" : "<ExportName>"));
    string reg_name = validate_identifier(2, "<RegisterName>");
    map<string, register_definition*>::iterator ir = m_register_definition_map.find(reg_name);
    if (ir == m_register_definition_map.end()) {
      ostringstream oss;
      oss << "Undefined <RegisterName> ('" << reg_name << "'):" << endl;
      oss << m_line;
      oss << m_p_definition_file->info_for_exception();
      throw xtsc_exception(oss.str());
    }
    register_definition& reg = *ir->second;
    u32 high_bit = reg.m_bit_width - 1;
    u32 low_bit = 0;
    switch (m_words.size()) {
      case 3: break;
      case 4: high_bit = low_bit = get_u32(3, "<HighBit>"); break;
      case 5: high_bit = get_u32(3, "<HighBit>"); low_bit = get_u32(4, "<LowBit>"); break;
      default: {
        ostringstream oss;
        oss << "Syntax error (too many words):" << endl;
        oss << m_line;
        oss << m_p_definition_file->info_for_exception();
        throw xtsc_exception(oss.str());
        break;
      }
    }
    if (low_bit >= reg.m_bit_width) {
      ostringstream oss;
      oss << "Invalid <LowBit> (valid range = [0:" << (reg.m_bit_width-1) << "]):" << endl;
      oss << m_line;
      oss << m_p_definition_file->info_for_exception();
      throw xtsc_exception(oss.str());
    }
    if ((high_bit < low_bit) || (high_bit >= reg.m_bit_width)) {
      ostringstream oss;
      oss << "Invalid <HighBit> (valid range = [" << low_bit << ":" << (reg.m_bit_width-1) << "]):" << endl;
      oss << m_line;
      oss << m_p_definition_file->info_for_exception();
      throw xtsc_exception(oss.str());
    }
    if (m_words[0] == "output") {
      if (m_io_set.find(io_name) != m_io_set.end()) {
        ostringstream oss;
        oss << "Duplicate input/output name ('" << io_name << "'):" << endl;
        oss << m_line;
        oss << m_p_definition_file->info_for_exception();
        throw xtsc_exception(oss.str());
      }
      output_definition *p_output       = new output_definition(*this);
      p_output->m_name                  = io_name;
      p_output->m_reg_name              = reg_name;
      p_output->m_high_bit              = high_bit;
      p_output->m_low_bit               = low_bit;
      p_output->m_p_wire_write_port     = new wire_write_port(io_name.c_str());
      m_output_definition_map.insert(map<string, output_definition*>::value_type(p_output->m_name, p_output));
      m_output_set.insert(p_output->m_name);
      m_io_set.insert(p_output->m_name);
      reg.m_output_set.insert(p_output);
      m_port_types[p_output->m_name] = WIRE_WRITE_PORT;
      m_port_table_all_outputs.push_back(p_output->m_name);
      m_port_table_all.push_back(p_output->m_name);
      XTSC_LOG(m_text, ll, " output (sc_port):  " << *p_output);
    }
    else if (m_words[0] == "input") {
      if (m_io_set.find(io_name) != m_io_set.end()) {
        ostringstream oss;
        oss << "Duplicate input/output name ('" << io_name << "'):" << endl;
        oss << m_line;
        oss << m_p_definition_file->info_for_exception();
        throw xtsc_exception(oss.str());
      }
      if (reg.m_read_only) {
        ostringstream oss;
        oss << "Read only register ('" << reg.m_name << "') cannot have an input definition ('" << io_name << "'):" << endl;
        oss << m_line;
        oss << m_p_definition_file->info_for_exception();
        throw xtsc_exception(oss.str());
      }
      u32 bit_width                     = high_bit - low_bit + 1;
      input_definition *p_input         = new input_definition(*this);
      p_input->m_name                   = io_name;
      p_input->m_p_register_definition  = &reg;
      p_input->m_high_bit               = high_bit;
      p_input->m_low_bit                = low_bit;
      p_input->m_p_wire_write_export    = new wire_write_export(io_name.c_str());
      p_input->m_p_wire_write_impl      = new input_definition::xtsc_wire_write_if_impl(io_name + "__impl", *p_input, bit_width);
      (*p_input->m_p_wire_write_export)(*p_input->m_p_wire_write_impl);  // Bind the sc_export to the implementation
      m_input_definition_map.insert(map<string, input_definition*>::value_type(p_input->m_name, p_input));
      m_input_set.insert(p_input->m_name);
      m_io_set.insert(p_input->m_name);
      reg.m_input_set.insert(p_input);
      m_port_types[p_input->m_name] = WIRE_WRITE_EXPORT;
      m_port_table_all_inputs.push_back(p_input->m_name);
      m_port_table_all.push_back(p_input->m_name);
      XTSC_LOG(m_text, ll, " input (sc_export): " << *p_input);
    }
  }

  m_port_types["m_request_export"] = REQUEST_EXPORT;
  m_port_types["m_respond_port"  ] = RESPOND_PORT;
  m_port_types["slave_port"      ] = PORT_TABLE;

  if (m_port_types.find("all"        ) == m_port_types.end()) { m_port_types["all"        ] = PORT_TABLE; }
  if (m_port_types.find("all_inputs" ) == m_port_types.end()) { m_port_types["all_inputs" ] = PORT_TABLE; }
  if (m_port_types.find("all_outputs") == m_port_types.end()) { m_port_types["all_outputs"] = PORT_TABLE; }

  xtsc_register_command(*this, *this, "change_clock_period", 1, 1,
      "change_clock_period <ClockPeriodFactor>", 
      "Call xtsc_mmio::change_clock_period(<ClockPeriodFactor>).  Return previous <ClockPeriodFactor> for this device."
  );

  xtsc_register_command(*this, *this, "peek", 2, 2,
      "peek <StartAddress> <NumBytes>", 
      "Peek <NumBytes> starting at <StartAddress>."
  );

  xtsc_register_command(*this, *this, "poke", 2, -1,
      "poke <StartAddress> <NumBytes> <Byte1> <Byte2> . . . <ByteN>", 
      "Poke <NumBytes> (=N) starting at <StartAddress>."
  );

  xtsc_register_command(*this, *this, "reset", 0, 0,
      "reset", 
      "Call xtsc_mmio::reset()."
  );

  xtsc_event_register(m_request_event, "m_request_event", this);
  xtsc_event_register(m_write_event,   "m_write_event",   this);
  xtsc_event_register(m_poke_event,    "m_poke_event",    this);

  // Log our construction
  XTSC_LOG(m_text, ll, "Constructed " << kind() << " '" << name() << "':");
  XTSC_LOG(m_text, ll, " definition_file    = "                 << m_definition_file);
  XTSC_LOG(m_text, ll, " byte_width         = "                 << m_byte_width);
  XTSC_LOG(m_text, ll, " always_write       = " << boolalpha    << m_always_write);
  XTSC_LOG(m_text, ll, " swizzle_bytes      = " << boolalpha    << m_swizzle_bytes);
  XTSC_LOG(m_text, ll, " use_fast_access    = " << boolalpha    << m_use_fast_access);
  if (clock_period == 0xFFFFFFFF) {
  XTSC_LOG(m_text, ll, " clock_period       = 0x" << hex        << clock_period << " (" << m_clock_period << ")");
  } else {
  XTSC_LOG(m_text, ll, " clock_period       = "                 << clock_period << " (" << m_clock_period << ")");
  }
  XTSC_LOG(m_text, ll, " response_time      = "                 << response_time << " (" << m_response_time << ")");
}



xtsc_component::xtsc_mmio::~xtsc_mmio(void) {
  // Do any required clean-up here
}



u32 xtsc_component::xtsc_mmio::get_bit_width(const string& port_name, u32 interface_num) const {
  if ((port_name == "m_request_export") || (port_name == "m_respond_port")) {
    return m_byte_width*8;
  }
  return get_bit_width(port_name.c_str());
}



sc_object *xtsc_component::xtsc_mmio::get_port(const string& port_name) {
  if (has_output(port_name.c_str())) return &get_output(port_name.c_str());
  if (has_input (port_name.c_str())) return &get_input (port_name.c_str());

  if ((port_name != "m_request_export") && (port_name != "m_respond_port")) {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\" has no port named \"" << port_name << "\"" << endl;
    throw xtsc_exception(oss.str());
  }

  return ((port_name == "m_request_export") ?  (sc_object*) &m_request_export : (sc_object*) &m_respond_port);
}



xtsc_port_table xtsc_component::xtsc_mmio::get_port_table(const string& port_table_name) const {
  if (port_table_name == "all"        ) return m_port_table_all;
  if (port_table_name == "all_inputs" ) return m_port_table_all_inputs;
  if (port_table_name == "all_outputs") return m_port_table_all_outputs;

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



bool xtsc_component::xtsc_mmio::has_input(const char *input_name) const {
  input_definition_map::const_iterator ii = m_input_definition_map.find(input_name);
  return (ii != m_input_definition_map.end());
}



bool xtsc_component::xtsc_mmio::has_output(const char *output_name) const {
  output_definition_map::const_iterator ii = m_output_definition_map.find(output_name);
  return (ii != m_output_definition_map.end());
}



xtsc::u32 xtsc_component::xtsc_mmio::get_bit_width(const char *io_name) const {
  input_definition_map::const_iterator ii = m_input_definition_map.find(io_name);
  if (ii != m_input_definition_map.end()) {
    return ii->second->m_high_bit - ii->second->m_low_bit + 1;
  }
  output_definition_map::const_iterator io = m_output_definition_map.find(io_name);
  if (io != m_output_definition_map.end()) {
    return io->second->m_high_bit - io->second->m_low_bit + 1;
  }
  ostringstream oss;
  oss << "xtsc_mmio::get_bit_width(): '" << name() << "' has no input/output named '" << io_name << "'";
  throw xtsc_exception(oss.str());
}



sc_export<xtsc_wire_write_if>& xtsc_component::xtsc_mmio::get_input(const char *input_name) const {
  input_definition_map::const_iterator ii = m_input_definition_map.find(input_name);
  if (ii == m_input_definition_map.end()) {
    ostringstream oss;
    oss << "xtsc_mmio::get_input(): '" << name() << "' has no sc_export input named '" << input_name << "'";
    throw xtsc_exception(oss.str());
  }
  return *ii->second->m_p_wire_write_export;
}



sc_port<xtsc_wire_write_if, NSPP>& xtsc_component::xtsc_mmio::get_output(const char *output_name) const {
  output_definition_map::const_iterator io = m_output_definition_map.find(output_name);
  if (io == m_output_definition_map.end()) {
    ostringstream oss;
    oss << "xtsc_mmio::get_output(): '" << name() << "' has no sc_port output named '" << output_name << "'";
    throw xtsc_exception(oss.str());
  }
  return *io->second->m_p_wire_write_port;
}



set<string> xtsc_component::xtsc_mmio::get_input_set() const {
  return m_input_set;
}



set<string> xtsc_component::xtsc_mmio::get_output_set() const {
  return m_output_set;
}



void xtsc_component::xtsc_mmio::execute(const string&           cmd_line, 
                                        const vector<string>&   words,
                                        const vector<string>&   words_lc,
                                        ostream&                result)
{
  ostringstream res;

  if (false) {
  }
  else if (words[0] == "change_clock_period") {
    u32 clock_period_factor = xtsc_command_argtou32(cmd_line, words, 1);
    res << m_clock_period.value();
    change_clock_period(clock_period_factor);
  }
  else if (words[0] == "peek") {
    u64 start_address = xtsc_command_argtou64(cmd_line, words, 1);
    u32 num_bytes     = xtsc_command_argtou32(cmd_line, words, 2);
    u8 *buffer = new u8[num_bytes];
    m_request_impl.nb_peek(start_address, num_bytes, buffer);
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
    u64 start_address = xtsc_command_argtou64(cmd_line, words, 1);
    u32 num_bytes     = xtsc_command_argtou32(cmd_line, words, 2);
    if (words.size() != num_bytes+3) {
      ostringstream oss;
      oss << "Command '" << cmd_line << "' specifies num_bytes=" << num_bytes << " but contains " << (words.size()-3)
          << " byte arguments.";
      throw xtsc_exception(oss.str());
    }
    if (num_bytes) {
      u8 *buffer = new u8[num_bytes];
      for (u32 i=0; i<num_bytes; ++i) {
        buffer[i] = (u8) xtsc_command_argtou32(cmd_line, words, 3+i);
      }
      m_request_impl.nb_poke(start_address, num_bytes, buffer);
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



void xtsc_component::xtsc_mmio::connect(xtsc_arbiter& arbiter) {
  arbiter.m_request_port(m_request_export);
  m_respond_port(arbiter.m_respond_export);
}



void xtsc_component::xtsc_mmio::connect(xtsc_core& core, const char *memory_port_name) {
  core.get_request_port(memory_port_name)(m_request_export);
  m_respond_port(core.get_respond_export(memory_port_name));
}



void xtsc_component::xtsc_mmio::connect(xtsc_core& core, const char *core_intf_name, const char *io_name) {
  u32 wo = 0;
  bool output = true;
  if (core.has_export_state(core_intf_name)) {
    core.get_export_state(core_intf_name)(get_input(io_name));
    wo = core.get_tie_bit_width(core_intf_name);
  }
  else if (core.has_system_output_wire(core_intf_name)) {
    core.get_system_output_wire(core_intf_name)(get_input(io_name));
    wo = core.get_sysio_bit_width(core_intf_name);
  }
  else if (core.has_system_input_wire(core_intf_name)) {
    output = false;
    get_output(io_name)(core.get_system_input_wire(core_intf_name));
    wo = core.get_sysio_bit_width(core_intf_name);
  }
  else {
    ostringstream oss;
    oss << "xtsc_mmio::connect: core '" << core.name() << "' has no export state or system-level output/input wire named '"
        << core_intf_name << "'.";
    throw xtsc_exception(oss.str());
  }
  u32 wi = get_bit_width(io_name);
  if (wo != wi) {
    ostringstream oss;
    oss << "Width mismatch:  cannot connect " << wo << "-bit " << (output ? "output" : "input") << " '" << core_intf_name
        << "' of xtsc_core '" << core.name() << "' to " << wi << "-bit " << (output ? "input" : "output") << " '" << io_name
        << "' of xtsc_mmio '" << name() << "'";
    throw xtsc_exception(oss.str());
  }
}



void xtsc_component::xtsc_mmio::connect(xtsc_interrupt_distributor&     distributor,
                                        const char                     *distributor_io_name,
                                        const char                     *mmio_io_name)
{
  bool distributor_is_output = distributor.has_output(distributor_io_name);
  u32 wo = distributor.get_bit_width(distributor_io_name);
  u32 wi = get_bit_width(mmio_io_name);
  if (wo != wi) {
    ostringstream oss;
    oss << "Width mismatch:  cannot connect " << wo << "-bit I/O '" << distributor_io_name << "' of xtsc_interrupt_distributor '"
        << distributor.name() << "' to " << wi << "-bit I/O '" << mmio_io_name << "' of xtsc_mmio '" << name() << "'";
    throw xtsc_exception(oss.str());
  }
  if (distributor_is_output) {
    distributor.get_output(distributor_io_name)(get_input(mmio_io_name));
  }
  else {
    get_output(mmio_io_name)(distributor.get_input(distributor_io_name));
  }
}



void xtsc_component::xtsc_mmio::connect(xtsc_master& master) {
  master.m_request_port(m_request_export);
  m_respond_port(master.m_respond_export);
}



void xtsc_component::xtsc_mmio::connect(xtsc_memory_trace& memory_trace, u32 port_num) {
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



void xtsc_component::xtsc_mmio::connect(xtsc_pin2tlm_memory_transactor& pin2tlm, u32 port_num) {
  u32 num_slaves = pin2tlm.get_num_ports();
  if (port_num >= num_slaves) {
    ostringstream oss;
    oss << "Invalid port_num=" << port_num << " in xtsc_mmio::connect(): " << endl;
    oss << pin2tlm.kind() << " '" << pin2tlm.name() << "' has " << num_slaves << " ports numbered from 0 to " << num_slaves-1 << endl;
    throw xtsc_exception(oss.str());
  }
  (*pin2tlm.m_request_ports[port_num])(m_request_export);
  m_respond_port(*pin2tlm.m_respond_exports[port_num]);
}



void xtsc_component::xtsc_mmio::connect(xtsc_router& router, u32 port_num) {
  u32 num_slaves = router.get_num_slaves();
  if (port_num >= num_slaves) {
    ostringstream oss;
    oss << "Invalid port_num=" << port_num << " in xtsc_mmio::connect(): " << endl;
    oss << router.kind() << " '" << router.name() << "' has " << num_slaves << " ports numbered from 0 to " << num_slaves-1 << endl;
    throw xtsc_exception(oss.str());
  }
  (*router.m_request_ports[port_num])(m_request_export);
  m_respond_port(*router.m_respond_exports[port_num]);
}



void xtsc_component::xtsc_mmio::connect(xtsc_mmio& mmio, const char *output_name, const char *input_name) {
  u32 wo = mmio.get_bit_width(output_name);
  u32 wi = get_bit_width(input_name);
  if (wo != wi) {
    ostringstream oss;
    oss << "Width mismatch:  cannot connect " << wo << "-bit output '" << output_name << "' of xtsc_mmio '" << mmio.name()
        << "' to " << wi << "-bit input '" << input_name << "' of xtsc_mmio '" << name() << "'";
    throw xtsc_exception(oss.str());
  }
  mmio.get_output(output_name)(get_input(input_name));
}



void xtsc_component::xtsc_mmio::connect(xtsc_wire_logic& logic, const char *output_name, const char *input_name) {
  u32 wo = logic.get_bit_width(output_name);
  u32 wi = get_bit_width(input_name);
  if (wo != wi) {
    ostringstream oss;
    oss << "Width mismatch:  cannot connect " << wo << "-bit output '" << output_name << "' of xtsc_wire_logic '" << logic.name()
        << "' to " << wi << "-bit input '" << input_name << "' of xtsc_mmio '" << name() << "'";
    throw xtsc_exception(oss.str());
  }
  logic.get_output(output_name)(get_input(input_name));
}



void xtsc_component::xtsc_mmio::connect(xtsc_wire_source& source, const char *output_name, const char *input_name) {
  // For backward compatibility
  if (input_name == NULL) {
    input_name = output_name;
    output_name = NULL;
  }
  string output((output_name && output_name[0]) ? output_name : source.get_default_output_name().c_str());
  u32 wo = source.get_bit_width(output_name);
  u32 wi = get_bit_width(input_name);
  if (wo != wi) {
    ostringstream oss;
    oss << "Width mismatch:  cannot connect " << wo << "-bit output '" << output << "' of xtsc_wire_source '" << source.name()
        << "' to " << wi << "-bit input '" << input_name << "' of xtsc_mmio '" << name() << "'";
    throw xtsc_exception(oss.str());
  }
  source.get_tlm_output(output_name)(get_input(input_name));
}



void xtsc_component::xtsc_mmio::end_of_elaboration(void) {
  reset();
} 



void xtsc_component::xtsc_mmio::reset(bool /* hard_reset */) {
  XTSC_INFO(m_text, "xtsc_mmio::reset()");

  m_busy = false;
  for (register_definition_map::iterator ir = m_register_definition_map.begin(); ir != m_register_definition_map.end(); ++ir) {
    ir->second->reset();
  }

  // Cancel notified events
  m_request_event.cancel();
  m_write_event  .cancel();
  m_poke_event   .cancel();

  xtsc_reset_processes(m_process_handles);
}



void xtsc_component::xtsc_mmio::change_clock_period(u32 clock_period_factor) {
  m_clock_period = sc_get_time_resolution() * clock_period_factor;
  XTSC_INFO(m_text, "change_clock_period(" << clock_period_factor << ") => " << m_clock_period);
  compute_delays();
}



void xtsc_component::xtsc_mmio::compute_delays() {
  u32 response_time = m_mmio_parms.get_u32("response_time");
  m_response_time = response_time * m_clock_period;
}



void xtsc_component::xtsc_mmio::request_thread(void) {

  // A try/catch block in sc_main will not catch an exception thrown from
  // an SC_THREAD, so we'll catch them here, log them, then rethrow them.
  try {

    // Loop forever
    while (true) {

      // Tell nb_request that we're no longer busy
      m_busy = false;

      // Wait for nb_request to tell us there's something to do 
      wait(m_request_event);

      // Create response from request
      xtsc_response response(m_active_request, xtsc_response::RSP_OK);

      // Pick out some useful information about the request
      xtsc_address addr     = m_active_request.get_byte_address();
      u32          size     = m_active_request.get_byte_size();
      bool         axi      = m_active_request.is_axi_protocol();
      bool         one_beat = (m_active_request.get_num_transfers() == 1);

      bool unsupported_request = false;

      // Handle request according to its type
      switch (m_active_request.get_type()) {

        case xtsc_request::READ: {
          read_bytes(addr, size, response.get_buffer());
          send_response(response);
          break;
        }

        case xtsc_request::BURST_READ: {
          if (axi && one_beat) {
            read_bytes(addr, size, response.get_buffer());
            send_response(response);
          }
          else {
            unsupported_request = true;
          }
          break;
        }

        case xtsc_request::WRITE: {
          write_bytes(addr, size, m_active_request.get_buffer());
          m_write_event.notify(SC_ZERO_TIME);
          send_response(response);
          break;
        }


        case xtsc_request::BURST_WRITE: {
          if (axi && one_beat) {
            write_bytes(addr, size, m_active_request.get_buffer());
            m_write_event.notify(SC_ZERO_TIME);
            send_response(response);
          }
          else {
            unsupported_request = true;
          }
          break;
        }

        case xtsc_request::BLOCK_READ:  { unsupported_request = true; break; }
        case xtsc_request::RCW:         { unsupported_request = true; break; }
        case xtsc_request::BLOCK_WRITE: { unsupported_request = true; break; }

        // We covered all the cases, but just in case . . .
        default: {
          ostringstream oss;
          oss << "Unsupported request type=" << m_active_request.get_type_name();
          throw xtsc_exception(oss.str());
        }
      }

      // Unsupported: return address error after last-transfer request
      if (unsupported_request) {
        if (m_active_request.get_last_transfer()) {
          response.set_status(xtsc_response::RSP_ADDRESS_ERROR);
          response.set_last_transfer(true);
          send_response(response);
        }
      }

    }

  }
  catch (const sc_unwind_exception& error) { throw; }
  catch (const exception& error) {
    ostringstream oss;
    oss << "std::exception caught in SC_THREAD of " << kind() << " '" << name() << "'." << endl;
    oss << "what(): " << error.what() << endl;
    xtsc_log_multiline(m_text, FATAL_LOG_LEVEL, oss.str(), 2);
    throw;
  }

}



void xtsc_component::xtsc_mmio::send_response(xtsc_response &response) {
  // Log the response
  XTSC_INFO(m_text, response);
  
  // Send the response until it is accepted
  while (!m_respond_port->nb_respond(response)) {
    XTSC_INFO(m_text, response << " <-- REJECTED");
    wait(m_clock_period);
  }
}



xtsc_component::xtsc_mmio::register_definition *xtsc_component::xtsc_mmio::get_register(xtsc_address    address,
                                                                                        u32&            high_bit,
                                                                                        u32&            low_bit)
{
  address_register_map::iterator ia = m_address_register_map.find(address);
  if (ia == m_address_register_map.end()) return NULL;
  register_definition& reg = *ia->second;
  low_bit = (address - reg.m_address) * 8;
  high_bit = min(low_bit + 7, reg.m_bit_width - 1);
  return &reg;
}



u32 xtsc_component::xtsc_mmio::get_u32(u32 index, const string& argument_name) {
  u32 value = 0;
  if (index >= m_words.size()) {
    ostringstream oss;
    oss << argument_name << " argument (#" << index+1 << ") missing:" << endl;
    oss << m_line;
    oss << m_p_definition_file->info_for_exception();
    throw xtsc_exception(oss.str());
  }
  try {
    value = xtsc_strtou32(m_words[index]);
  }
  catch (const xtsc_exception&) {
    ostringstream oss;
    oss << "Cannot convert " << argument_name << " argument (#" << index+1 << ") '" << m_words[index] << "' to 32-bit number:" << endl;
    oss << m_line;
    oss << m_p_definition_file->info_for_exception();
    throw xtsc_exception(oss.str());
  }
  return value;
}



u64 xtsc_component::xtsc_mmio::get_u64(u32 index, const string& argument_name) {
  u64 value = 0;
  if (index >= m_words.size()) {
    ostringstream oss;
    oss << argument_name << " argument (#" << index+1 << ") missing:" << endl;
    oss << m_line;
    oss << m_p_definition_file->info_for_exception();
    throw xtsc_exception(oss.str());
  }
  try {
    value = xtsc_strtou64(m_words[index]);
  }
  catch (const xtsc_exception&) {
    ostringstream oss;
    oss << "Cannot convert " << argument_name << " argument (#" << index+1 << ") '" << m_words[index] << "' to 64-bit number:" << endl;
    oss << m_line;
    oss << m_p_definition_file->info_for_exception();
    throw xtsc_exception(oss.str());
  }
  return value;
}



const string& xtsc_component::xtsc_mmio::validate_identifier(u32 index, const string& argument_name) {
  if (index >= m_words.size()) {
    ostringstream oss;
    oss << "The " << argument_name << " argument (#" << index+1 << ") is missing:" << endl;
    oss << m_line;
    oss << m_p_definition_file->info_for_exception();
    throw xtsc_exception(oss.str());
  }
  const char *name_c_str = m_words[index].c_str();
  for (const char *pc = name_c_str; *pc; ++pc) {
    char c = *pc;
    if (c>='a' && c<='z') continue;
    if (c>='A' && c<='Z') continue;
    if (c=='_') continue;
    // If not 1st character . . .
    if (pc != name_c_str) {
      // Allow digit
      if (c>='0' && c<='9') continue;
    }
    ostringstream oss;
    oss << "The " << argument_name << " argument ('" << m_words[index] << "') contains invalid characters:" << endl;
    oss << m_line;
    oss << m_p_definition_file->info_for_exception();
    throw xtsc_exception(oss.str());
  }
  return m_words[index];
}



void xtsc_component::xtsc_mmio::swizzle_buffer(xtsc_address address8, u32 size8, u8 *buffer) {
  bool          growing         = true;
  u32           shift           = 1;
  xtsc_address  addr            = address8;
  u32           bytes_remaining = size8;
  for (u32 swizzle_size=1; bytes_remaining; swizzle_size = (growing ? (swizzle_size << shift) : (swizzle_size >> 1))) {
    XTSC_DEBUG(m_text, "swizzle_size=" << swizzle_size << " growing=" << growing << " shift=" << shift <<
                       " bytes_remaining=" << bytes_remaining << " addr=0x" << hex << setfill('0') << setw(xtsc_address_nibbles()) << addr);
    if (swizzle_size <= bytes_remaining) {
      if ((swizzle_size == m_byte_width) || (addr % (swizzle_size*2) != 0) || (!growing)) {
        XTSC_DEBUG(m_text, "Swizzling " << swizzle_size << " bytes at address=0x" << hex << setfill('0') << setw(xtsc_address_nibbles()) << addr);
        for (u32 lo = addr - address8, hi = lo + swizzle_size - 1; lo < hi; ++lo, --hi) {
          u8 b = buffer[lo];
          buffer[lo] = buffer[hi];
          buffer[hi] = b;
        }
        addr += swizzle_size;
        bytes_remaining -= swizzle_size;
      }
    }
    if (swizzle_size == m_byte_width) {
      shift = 0;
      if (bytes_remaining < swizzle_size) {
        growing = false;
      }
    }
  }
}



void xtsc_component::xtsc_mmio::read_bytes(xtsc_address address8, u32 size8, u8 *buffer) {
  XTSC_DEBUG(m_text, "read_bytes 0x" << hex << setfill('0') << setw(xtsc_address_nibbles()) << address8 << "/" << size8);
  for (u32 i=0; i<size8; ++i) {
    xtsc_address addr = address8 + i;
    u32 high_bit = 0;
    u32 low_bit  = 0;
    register_definition *p_reg = get_register(addr, high_bit, low_bit);
    if (p_reg != NULL) {
      buffer[i] = (u8) p_reg->m_p_current_value->range(high_bit, low_bit).to_uint() & 0xFF;
      XTSC_DEBUG(m_text, "read_bytes 0x" << hex << setfill('0') << setw(xtsc_address_nibbles()) << addr << " = " << p_reg->m_name << " = 0x" <<
                         setw(2) << (u32) buffer[i]);
    }
    else {
      buffer[i] = 0;
    }
  }
  if (m_swizzle_bytes) {
    swizzle_buffer(address8, size8, buffer);
  }
}



void xtsc_component::xtsc_mmio::write_bytes(xtsc_address address8, u32 size8, const u8 *buffer) {
  XTSC_DEBUG(m_text, "write_bytes 0x" << hex << setfill('0') << setw(xtsc_address_nibbles()) << address8 << "/" << size8);
  if (m_swizzle_bytes) {
    // Cheat and use buffer for reordering
    u8 *buf = const_cast<u8*>(buffer);
    swizzle_buffer(address8, size8, buf);
  }

  set<register_definition*> touched_registers;
  for (u32 i=0; i<size8; ++i) {
    xtsc_address addr = address8 + i;
    u32 high_bit = 0;
    u32 low_bit  = 0;
    register_definition *p_reg = get_register(addr, high_bit, low_bit);
    if (p_reg != NULL && !p_reg->m_read_only) {
      p_reg->m_p_current_value->range(high_bit, low_bit) = buffer[i];
      touched_registers.insert(p_reg);
      XTSC_DEBUG(m_text, "write_bytes 0x" << hex << setfill('0') << setw(xtsc_address_nibbles()) << addr << " = " << p_reg->m_name << " = 0x" <<
                         setw(2) << (u32) buffer[i]);
    }
  }
  for (set<register_definition*>::iterator ir = touched_registers.begin(); ir != touched_registers.end(); ++ir) {
    (*ir)->write_outputs(m_always_write);
  }

  if (m_swizzle_bytes) {
    // Now put the bytes back just like they were
    u8 *buf = const_cast<u8*>(buffer);
    swizzle_buffer(address8, size8, buf);
  }
}



void xtsc_component::xtsc_mmio::register_definition::dump(ostream& os) const {
  // Save state of stream
  char c = os.fill('0');
  ios::fmtflags old_flags = os.flags();
  streamsize w = os.width();
  os << right;

  os << m_name << " [0x" << hex << setw(xtsc_address_nibbles()) << m_address << dec << "/" << m_bit_width << "] = 0x" << hex << *m_p_current_value;

  // Restore state of stream
  os.fill(c);
  os.flags(old_flags);
  os.width(w);
}



void xtsc_component::xtsc_mmio::output_definition::dump(ostream& os) const {
  // Save state of stream
  char c = os.fill('0');
  ios::fmtflags old_flags = os.flags();

  os << m_name << " <= " <<  m_reg_name << "[" << m_high_bit << ":" << m_low_bit << "]";

  // Restore state of stream
  os.fill(c);
  os.flags(old_flags);
}



void xtsc_component::xtsc_mmio::input_definition::dump(ostream& os) const {
  // Save state of stream
  char c = os.fill('0');
  ios::fmtflags old_flags = os.flags();
  streamsize w = os.width();
  os << right;

  os << m_name << " => " <<  m_p_register_definition->m_name << "[" << m_high_bit << ":" << m_low_bit << "]";

  // Restore state of stream
  os.fill(c);
  os.flags(old_flags);
  os.width(w);
}



void xtsc_component::xtsc_mmio::register_definition::reset() {
  *m_p_current_value = *m_p_initial_value;
   write_outputs(true);
}



void xtsc_component::xtsc_mmio::register_definition::write_outputs(bool always_write) {
  XTSC_DEBUG(m_mmio.m_text, "write_outputs(" << boolalpha << always_write << ") called");
  for (output_set::iterator io = m_output_set.begin(); io != m_output_set.end(); ++io) {
    output_definition& output = **io;
    u32 hi = output.m_high_bit;
    u32 lo = output.m_low_bit;
    XTSC_DEBUG(m_mmio.m_text, "write_outputs: register " << m_name << "[" << hi << ":" << lo <<
                             "] => output \"" << output.m_name << "\"");
    if (always_write || (m_p_current_value->range(hi, lo) != m_p_previous_value->range(hi, lo))) {
      (*output.m_p_wire_write_port)->nb_write(m_p_current_value->range(hi, lo));
      XTSC_INFO(m_mmio.m_text, output.m_name << " => " << m_p_current_value->range(hi, lo).to_string(SC_HEX));
    }
  }
  *m_p_previous_value = *m_p_current_value;
}



void xtsc_component::xtsc_mmio::input_definition::xtsc_wire_write_if_impl::nb_write(const sc_unsigned& value) {
  if (static_cast<u32>(value.length()) != m_bit_width) {
    ostringstream oss;
    oss << "ERROR: Value of width=" << value.length() << " bits written to sc_export \"" << m_input_definition.m_name << "\" of width="
        << m_bit_width << " in xtsc_mmio '" << m_input_definition.m_mmio.name() << "'";
    throw xtsc_exception(oss.str());
  }
  register_definition& reg = *m_input_definition.m_p_register_definition;
  reg.m_p_current_value->range(m_input_definition.m_high_bit, m_input_definition.m_low_bit) = value;
  XTSC_INFO(m_input_definition.m_mmio.m_text, m_input_definition.m_name << " <= " << value.to_string(SC_HEX));
  m_input_definition.m_mmio.m_write_event.notify(SC_ZERO_TIME);
}



u32 xtsc_component::xtsc_mmio::input_definition::xtsc_wire_write_if_impl::nb_get_bit_width() {
  return m_bit_width;
}



void xtsc_component::xtsc_mmio::input_definition::xtsc_wire_write_if_impl::register_port(sc_port_base& port, const char *if_typename) {
  if (m_p_port) {
    ostringstream oss;
    oss << "Illegal multiple binding detected to sc_export<xtsc_wire_write_if> \"" << m_input_definition.m_name << "\" of xtsc_mmio '"
        << m_input_definition.m_mmio.name() << "'" << endl;
    oss << "  " << port.name() << endl;
    oss << "  " << m_p_port->name();
    throw xtsc_exception(oss.str());
  }
  XTSC_INFO(m_input_definition.m_mmio.m_text, "Binding '" << port.name() << "' to sc_export<xtsc_wire_write_if> \"" <<
                                              m_input_definition.m_name << "\" of xtsc_mmio '" << m_input_definition.m_mmio.name() <<
                                              "'");
  m_p_port = &port;
}



void xtsc_component::xtsc_mmio::xtsc_request_if_impl::nb_peek(xtsc_address address8, u32 size8, u8 *buffer) {
  m_mmio.read_bytes(address8, size8, buffer);
  ostringstream oss;
  xtsc_hex_dump(size8, buffer, oss);
  XTSC_INFO(m_mmio.m_text, "nb_peek: address=0x" << hex << address8 << " size=" << dec << size8 << ": " << oss.str());
}



void xtsc_component::xtsc_mmio::xtsc_request_if_impl::nb_poke(xtsc_address address8, u32 size8, const u8 *buffer) {
  ostringstream oss;
  xtsc_hex_dump(size8, buffer, oss);
  XTSC_INFO(m_mmio.m_text, "nb_poke: address=0x" << hex << address8 << " size=" << dec << size8 << ": " << oss.str());
  m_mmio.write_bytes(address8, size8, buffer);
  m_mmio.m_poke_event.notify(SC_ZERO_TIME);
}



bool xtsc_component::xtsc_mmio::xtsc_request_if_impl::nb_fast_access(xtsc_fast_access_request &request) {
  // Allow any fast access?
  if (!m_mmio.m_use_fast_access) {
    request.deny_access();
    XTSC_INFO(m_mmio.m_text, "nb_fast_access: deny_access");
    return true;
  }
  
  XTSC_VERBOSE(m_mmio.m_text, "nb_fast_access: using peek/poke");
  request.allow_peek_poke_access();
  return true;
}



void xtsc_component::xtsc_mmio::xtsc_request_if_impl::nb_request(const xtsc_request& request) {
  XTSC_INFO(m_mmio.m_text, request);

  // Can we accept the request at this time?
  if (m_mmio.m_busy) {
    // No. We're already busy.  Create an RSP_NACC response.
    xtsc_response response(request, xtsc_response::RSP_NACC, true);
    // Log the response
    XTSC_INFO(m_mmio.m_text, response);
    // Send the response
    m_mmio.m_respond_port->nb_respond(response);
  }
  else {
    // Yes.  We accept this request, so now we're busy.
    m_mmio.m_busy = true;
    // Create our copy of the request
    m_mmio.m_active_request = request;
    // Notify request_thread
    m_mmio.m_request_event.notify(m_mmio.m_response_time);
  }
}



void xtsc_component::xtsc_mmio::xtsc_request_if_impl::register_port(sc_port_base& port, const char *if_typename) {
  if (m_p_port) {
    ostringstream oss;
    oss << "Illegal multiple binding detected to xtsc_mmio '" << m_mmio.name() << "' m_request_export: " << endl;
    oss << "  " << port.name() << endl;
    oss << "  " << m_p_port->name();
    throw xtsc_exception(oss.str());
  }
  XTSC_INFO(m_mmio.m_text, "Binding '" << port.name() << "' to '" << m_mmio.name() << ".m_request_export'");
  m_p_port = &port;
}



