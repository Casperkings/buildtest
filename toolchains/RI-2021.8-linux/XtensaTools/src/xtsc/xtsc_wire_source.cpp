// Copyright (c) 2006-2012 by Tensilica Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Tensilica Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Tensilica Inc.


#include <cctype>
#include <algorithm>
#include <xtsc/xtsc_wire_source.h>
#include <xtsc/xtsc_core.h>
#include <xtsc/xtsc_interrupt_distributor.h>
#include <xtsc/xtsc_mmio.h>
#include <xtsc/xtsc_wire_logic.h>


using namespace std;
#if SYSTEMC_VERSION >= 20050601
using namespace sc_core;
#endif
using namespace sc_dt;
using namespace xtsc;
using log4xtensa::INFO_LOG_LEVEL;
using log4xtensa::VERBOSE_LOG_LEVEL;


namespace xtsc_component {

  static string get_first_port_name(const string& name, const xtsc_wire_source_parms& source_parms, bool for_m_pin) {
    bool pin_level = source_parms.get_bool("pin_level");
    const char *definition_file = source_parms.get_c_str("definition_file");
    if (definition_file && definition_file[0] && (pin_level == for_m_pin)) {
      xtsc_script_file  file(definition_file, "definition_file", name.c_str(), "xtsc_wire_source", false, NULL, false);
      string            line;
      vector<string>    words;
      if ((file.get_words(words, line, false))) {
        if (words.size() >= 3) {
          return words[1];
        }
      }
    }
    return (for_m_pin ? "m_pin" : "m_write");
  }



  static u32 get_first_port_bit_width(const string& name, const xtsc_wire_source_parms& source_parms) {
    const char *definition_file = source_parms.get_c_str("definition_file");
    if (definition_file && definition_file[0]) {
      xtsc_script_file  file(definition_file, "definition_file", name.c_str(), "xtsc_wire_source", false, NULL, false);
      string            line;
      vector<string>    words;
      if ((file.get_words(words, line, false))) {
        if (words.size() >= 3) {
          u32 bit_width = 1;
          try {
            bit_width = xtsc_strtou32(words[2]);
          }
          catch (...) {}  // Let the ctor redetect this problem and report the error
          return (bit_width ? bit_width : 1);  // ctor will detect this later
        }
      }
    }
    return source_parms.get_non_zero_u32("bit_width");
  }




}   // namespace xtsc_component


xtsc_component::xtsc_wire_source::xtsc_wire_source(sc_module_name module_name, const xtsc_wire_source_parms& source_parms) :
  sc_module             (module_name),
  xtsc_module           (*(sc_module*)this),
  m_pin                 (get_first_port_name(name(), source_parms, true).c_str()),
  m_write               (get_first_port_name(name(), source_parms, false).c_str()),
  m_text                (log4xtensa::TextLogger::getInstance(name())),
  m_width1              (get_first_port_bit_width(name(), source_parms)),
  m_control             (source_parms.get_bool("control")),
  m_control_bound       (false),
  m_p_control           (NULL),
  m_p_write_impl        (NULL),
  m_control_value       (1),
  m_wraparound          (source_parms.get_bool("wraparound")),
  m_pin_level           (source_parms.get_bool("pin_level")),
  m_p_trace_file        (static_cast<sc_trace_file*>(const_cast<void*>(source_parms.get_void_pointer("vcd_handle")))),
  m_script_file         (""),
  m_p_test_vector_stream(0),
  m_pin_floating        ("m_pin_floating", m_width1, m_text)
{

  if (m_control) {
    m_p_control = new wire_write_export("control");
    m_p_write_impl = new xtsc_wire_write_if_impl("control__impl", *this);
    (*m_p_control)(*m_p_write_impl);
  }
  m_control_value = 0;

  // Get clock period 
  m_time_resolution = sc_get_time_resolution();
  u32 clock_period = source_parms.get_non_zero_u32("clock_period");
  if (clock_period == 0xFFFFFFFF) {
    m_clock_period = xtsc_get_system_clock_period();
  }
  else {
    m_clock_period = sc_get_time_resolution() * clock_period;
  }
  m_clock_period_value = m_clock_period.value();
  u32 posedge_offset = source_parms.get_u32("posedge_offset");
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

  // Open "script_file" if present
  const char *script_file = source_parms.get_c_str("script_file");
  if (script_file && script_file[0]) {
    m_p_test_vector_stream = new xtsc_script_file(script_file, "script_file",  name(), kind(), m_wraparound);
    m_script_file = m_p_test_vector_stream->name();
    SC_THREAD(write_thread);    m_process_handles.push_back(sc_get_current_process_handle());
  }

  // Parse "definition_file" if present
  const char *definition_file = source_parms.get_c_str("definition_file");
  log4xtensa::LogLevel ll = xtsc_get_constructor_log_level();

  if (!definition_file || !definition_file[0]) {
    if (m_script_file == "") {
      ostringstream oss;
      oss << kind() << " '" << name() << "':  Has no \"definition_file\" defined and no \"script_file\" defined.  "
          << "What value is it supposed to drive on its output?";
      throw xtsc_exception(oss.str());
    }
    string output_name(m_pin_level ? "m_pin" : "m_write");
    output_definition *p_output = new output_definition(*this, output_name, m_width1, "0", &m_write, &m_pin);
    XTSC_LOG(m_text, ll, " output (sc_port):  " << p_output->m_name << " " << p_output->m_bit_width <<
                         " " << p_output->m_initial_value);
  }
  else {
    // Process script file to get output port definitions
    xtsc_script_file *p_script_file = new xtsc_script_file(definition_file, "definition_file",  name(), kind(), false);
    m_definition_file = p_script_file->name();

    XTSC_LOG(m_text, ll, "Reading xtsc_wire_source port definitions from file '" << m_definition_file << "'.");
    while ((m_line_count = p_script_file->get_words(m_words, m_line, false))) {
      m_words_lc.clear();
      for (u32 i=0; i<m_words.size(); ++i) {
        string word = m_words[i];
        transform(word.begin(), word.end(), word.begin(), ::tolower);
        m_words_lc.push_back(word);
      }
      if (m_words_lc[0] == "output") {
        string output_name  = validate_identifier(1, "<OutputName>", p_script_file);
        if (m_output_set.find(output_name) != m_output_set.end()) {
          ostringstream oss;
          oss << "Duplicate output name ('" << output_name << "') found:" << endl;
          oss << m_line;
          oss << p_script_file->info_for_exception();
          throw xtsc_exception(oss.str());
        }
        u32 bit_width = get_u32(2, "<BitWidth>", p_script_file);
        if (bit_width == 0) {
          ostringstream oss;
          oss << "<BitWidth> argument (#3) cannot be 0:" << endl;
          oss << m_line;
          oss << p_script_file->info_for_exception();
          throw xtsc_exception(oss.str());
        }
        string initial_value("0");
        if (m_words.size() >= 4) {
          initial_value = m_words[3];
          sc_unsigned test_convert(bit_width);
          try {
            test_convert = initial_value.c_str();
          }
          catch (...) {
            ostringstream oss;
            oss << "Cannot convert <InitialValue> argument (#4) '" << initial_value << "' to number:" << endl;
            oss << m_line;
            oss << p_script_file->info_for_exception();
            throw xtsc_exception(oss.str());
          }
        }
        else if (m_script_file == "") {
          ostringstream oss;
          oss << "Output \"" << output_name << "\" has no initial valued specified " << endl;
          oss << "(when no \"script_file\" is defined all outputs must have initial values specified in the \"definition_file\"):"
              << endl;
          oss << m_line;
          oss << p_script_file->info_for_exception();
          throw xtsc_exception(oss.str());
        }
        // Prevent sign extension (we want 0 fill, not sign extension)
        if ((initial_value.length() > 2) && (initial_value[0] == '0') && ((initial_value[1] == 'x') || (initial_value[1] == 'X'))) {
          initial_value.insert(2, "0");
        }
        u32       index  = m_outputs.size();
        tlm_port *t_port = (index ? NULL : &m_write);
        pin_port *p_port = (index ? NULL : &m_pin);
        output_definition *p_output = new output_definition(*this, output_name, bit_width, initial_value, t_port, p_port);
        XTSC_LOG(m_text, ll, " output (sc_port):  " << p_output->m_name << " " << p_output->m_bit_width <<
                             " " << p_output->m_initial_value);
      }
      else {
        ostringstream oss;
        oss << "Syntax error (first word must be output):" << endl;
        oss << m_line;
        oss << p_script_file->info_for_exception();
        throw xtsc_exception(oss.str());
      }
    }
    delete p_script_file;
    if (!m_outputs.size()) {
      ostringstream oss;
      oss << kind() << " '" << name() << "': No outputs defined in \"definition_file\": " << m_definition_file;
      throw xtsc_exception(oss.str());
    }
  }

  // Cap unused port
  if (m_pin_level) {
    m_write(*this);
  }
  else {
    m_pin(m_pin_floating);
  }

  for (set<string>::const_iterator i = m_output_set.begin(); i != m_output_set.end(); ++i) {
    m_port_types[*i] = (m_pin_level ? WIDE_OUTPUT : WIRE_WRITE_PORT); 
    m_port_table_all_outputs.push_back(*i);
  }

  m_port_table_all.push_back("all_outputs");

  if (m_control) {
    m_port_types["control"] = WIRE_WRITE_EXPORT; 
    m_port_table_all        .push_back("control");
  }


  if (m_port_types.find("all"        ) == m_port_types.end()) { m_port_types["all"        ] = PORT_TABLE; }
  if (m_port_types.find("all_outputs") == m_port_types.end()) { m_port_types["all_outputs"] = PORT_TABLE; }

  XTSC_LOG(m_text, ll, "Constructed xtsc_wire_source '" << name() << "':");
  XTSC_LOG(m_text, ll, " control                 = "   << boolalpha << m_control);
  XTSC_LOG(m_text, ll, " pin_level               = "   << boolalpha << m_pin_level);
  XTSC_LOG(m_text, ll, " definition_file         = "   << m_definition_file);
  XTSC_LOG(m_text, ll, " vcd_handle              = "   << m_p_trace_file);
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
  XTSC_LOG(m_text, ll, " wraparound              = "   << boolalpha << m_wraparound);
}



xtsc_component::xtsc_wire_source::~xtsc_wire_source(void) {
}



u32 xtsc_component::xtsc_wire_source::get_bit_width(const string& port_name, u32 interface_num) const {
  return get_bit_width(port_name.c_str());
}



sc_object *xtsc_component::xtsc_wire_source::get_port(const string& port_name) {

  if (has_output(port_name.c_str())) {
    if (m_pin_level) return &get_output_pin(port_name.c_str());
    else             return &get_tlm_output(port_name.c_str());
  }

  if (m_control && (port_name == "control")) {
    return &get_control_input();
  }

  ostringstream oss;
  oss << kind() << " \"" << name() << "\" has no port named \"" << port_name << "\"" << endl;
  throw xtsc_exception(oss.str());
}



string xtsc_component::xtsc_wire_source::get_default_port_name() const {
  return ((m_output_set.size() != 1) ? "" : (m_pin_level ? m_pin.basename() : m_write.basename()));
}



xtsc_port_table xtsc_component::xtsc_wire_source::get_port_table(const string& port_table_name) const {
  if (port_table_name == "all"        ) return m_port_table_all;
  if (port_table_name == "all_outputs") return m_port_table_all_outputs;

  ostringstream oss;
  oss << kind() << " \"" << name() << "\" has no port table named \"" << port_table_name << "\"" << endl;
  throw xtsc_exception(oss.str());
}



void xtsc_component::xtsc_wire_source::end_of_elaboration(void) {
  reset();
}



void xtsc_component::xtsc_wire_source::reset(bool /*hard_reset*/) {
  XTSC_INFO(m_text, "xtsc_wire_source::reset()");

  m_words.clear();
  m_line        = "";
  m_line_count  = 0;

  if (m_p_test_vector_stream) {
    m_p_test_vector_stream->reset();
  }

  // Reset each output
  for (output_definition_vector::iterator io = m_outputs.begin(); io != m_outputs.end(); ++io) {
    (*io)->reset();
  }

  // Cancel notified events
  m_control_write_event.cancel();

  xtsc_reset_processes(m_process_handles);
}



string xtsc_component::xtsc_wire_source::get_default_output_name() const {
  return m_outputs[0]->m_name;
}



bool xtsc_component::xtsc_wire_source::has_output(const char *output_name) const {
  output_definition_map::const_iterator io = m_output_definition_map.find(output_name);
  return (io != m_output_definition_map.end());
}



xtsc::u32 xtsc_component::xtsc_wire_source::get_bit_width(const char *output_name) const {
  string output((output_name && output_name[0]) ? output_name : get_default_output_name());
  output_definition_map::const_iterator p_output = m_output_definition_map.find(output);
  if (p_output == m_output_definition_map.end()) {
    ostringstream oss;
    oss << "xtsc_wire_source::get_bit_width(): '" << name() << "' has no output named '" << output << "'";
    throw xtsc_exception(oss.str());
  }
  return p_output->second->m_bit_width;
}



sc_export<xtsc_wire_write_if>& xtsc_component::xtsc_wire_source::get_control_input() const {
  if (!m_control) {
    ostringstream oss;
    oss << "xtsc_wire_source '" << name() << "' has \"control\" false, so get_control_input() should not be called.";
    throw xtsc_exception(oss.str());
  }
  return *m_p_control;
}



sc_port<xtsc_wire_write_if, NSPP>& xtsc_component::xtsc_wire_source::get_tlm_output(const char *output_name) const {
  if (m_pin_level) {
    ostringstream oss;
    oss << "xtsc_wire_source '" << name() << "' has \"pin_level\" true, so get_tlm_output() should not be called.";
    throw xtsc_exception(oss.str());
  }
  string output((output_name && output_name[0]) ? output_name : get_default_output_name());
  output_definition_map::const_iterator io = m_output_definition_map.find(output);
  if (io == m_output_definition_map.end()) {
    ostringstream oss;
    oss << "xtsc_wire_source::get_tlm_output(): '" << name() << "' has no sc_port named '" << output << "'";
    throw xtsc_exception(oss.str());
  }
  return *io->second->m_p_tlm_port;
}



sc_out<sc_bv_base>& xtsc_component::xtsc_wire_source::get_output_pin(const char *output_name) const {
  if (!m_pin_level) {
    ostringstream oss;
    oss << "xtsc_wire_source '" << name() << "' has \"pin_level\" false, so get_output_pin() should not be called.";
    throw xtsc_exception(oss.str());
  }
  string output((output_name && output_name[0]) ? output_name : get_default_output_name());
  output_definition_map::const_iterator io = m_output_definition_map.find(output);
  if (io == m_output_definition_map.end()) {
    ostringstream oss;
    oss << "xtsc_wire_source::get_output_pin(): '" << name() << "' has no sc_out named '" << output << "'";
    throw xtsc_exception(oss.str());
  }
  return *io->second->m_p_pin_port;
}



set<string> xtsc_component::xtsc_wire_source::get_output_set() const {
  return m_output_set;
}



void xtsc_component::xtsc_wire_source::connect(xtsc_wire_logic& logic, const char *output_name) {
  if (!m_control) {
    ostringstream oss;
    oss << "'" << name() << "' has \"control\" false, so xtsc_wire_source::connect(xtsc_wire_logic&, ...) should not be called.";
    throw xtsc_exception(oss.str());
  }
  u32 wo = logic.get_bit_width(output_name);
  u32 wi = 1;
  if (wo != wi) {
    ostringstream oss;
    oss << "Width mismatch:  cannot connect " << wo << "-bit output '" << output_name << "' of xtsc_wire_logic '" << logic.name()
        << "' to " << wi << "-bit control input of xtsc_wire_source '" << name() << "'";
    throw xtsc_exception(oss.str());
  }
  logic.get_output(output_name)(*m_p_control);
}



void xtsc_component::xtsc_wire_source::connect(xtsc_mmio& mmio, const char *output_name) {
  if (!m_control) {
    ostringstream oss;
    oss << "'" << name() << "' has \"control\" false, so xtsc_wire_source::connect(xtsc_mmio&, ...) should not be called.";
    throw xtsc_exception(oss.str());
  }
  u32 wo = mmio.get_bit_width(output_name);
  u32 wi = 1;
  if (wo != wi) {
    ostringstream oss;
    oss << "Width mismatch:  cannot connect " << wo << "-bit output '" << output_name << "' of xtsc_mmio '" << mmio.name()
        << "' to " << wi << "-bit control input of xtsc_wire_source '" << name() << "'";
    throw xtsc_exception(oss.str());
  }
  mmio.get_output(output_name)(*m_p_control);
}



void xtsc_component::xtsc_wire_source::connect(xtsc_core& core, const char *input_name, const char *output_name) {
  if (m_pin_level) {
    ostringstream oss;
    oss << "xtsc_wire_source '" << name() << "' has \"pin_level\" true, so the connect() method should not be called.";
    throw xtsc_exception(oss.str());
  }
  string output((output_name && output_name[0]) ? output_name : get_default_output_name());
  u32 wo = get_bit_width(output.c_str());
  u32 wi = core.get_sysio_bit_width(input_name);
  if (wo != wi) {
    ostringstream oss;
    oss << "Width mismatch:  cannot connect " << wo << "-bit output '" << output << "' of xtsc_wire_source '" << name()
        << "' to " << wi << "-bit system-level input '" << input_name << "' of xtsc_core '" << core.name() << "'";
    throw xtsc_exception(oss.str());
  }
  get_tlm_output(output.c_str())(core.get_system_input_wire(input_name));
}



void xtsc_component::xtsc_wire_source::connect(xtsc_interrupt_distributor&      distributor,
                                               const char                      *input_name,
                                               const char                      *output_name)
{
  if (m_pin_level) {
    ostringstream oss;
    oss << "xtsc_wire_source '" << name() << "' has \"pin_level\" true, so the connect() method should not be called.";
    throw xtsc_exception(oss.str());
  }
  string output((output_name && output_name[0]) ? output_name : get_default_output_name());
  u32 wo = get_bit_width(output.c_str());
  u32 wi = distributor.get_bit_width(input_name);
  if (wo != wi) {
    ostringstream oss;
    oss << "Width mismatch:  cannot connect " << wo << "-bit output '" << output << "' of xtsc_wire_source '" << name()
        << "' to " << wi << "-bit input '" << input_name << "' of xtsc_interrupt_distributor '" << distributor.name() << "'";
    throw xtsc_exception(oss.str());
  }
  get_tlm_output(output.c_str())(distributor.get_input(input_name));
}



void xtsc_component::xtsc_wire_source::write_thread(void) {

  try {

    while ((m_line_count = m_p_test_vector_stream->get_words(m_words, m_line, false)) != 0) {

      XTSC_DEBUG(m_text, "\"script_file\" line #" << m_line_count << ": " << m_line);

      m_words_lc.clear();
      for (u32 i=0; i<m_words.size(); ++i) {
        string word = m_words[i];
        transform(word.begin(), word.end(), word.begin(), ::tolower);
        m_words_lc.push_back(word);
      }

      if (m_words_lc[0] == "wait") {
        if ((m_words.size() >= 2) && (m_words_lc[1] == "control")) {
          if (!m_control) {
            ostringstream oss;
            oss << "WAIT CONTROL command cannot be used unless the \"control\" parameter is set to true:" << endl;
            oss << m_line;
            oss << m_p_test_vector_stream->info_for_exception();
            throw xtsc_exception(oss.str());
          }
          if (!m_control_bound) {
            ostringstream oss;
            oss << "WAIT CONTROL command cannot be used unless something is connected to the control input:" << endl;
            oss << m_line;
            oss << m_p_test_vector_stream->info_for_exception();
            throw xtsc_exception(oss.str());
          }
          if (m_words.size() <= 4) {
            u32 count = 1;
            if (m_words.size() == 4) {
              count = get_u32(3, "<count>", m_p_test_vector_stream);
            }
            if ((m_words.size() == 2) || (m_words_lc[2] == "write")) {
              u32 target_write_count = m_control_write_count + count;
              while (target_write_count != m_control_write_count) {
                wait(m_control_write_event);
              }
            }
            else if (m_words_lc[2] == "change") {
              u32 target_change_count = m_control_change_count + count;
              while (target_change_count != m_control_change_count) {
                wait(m_control_write_event);
              }
            }
            else {
              u32 value = get_u32(2, "<value>", m_p_test_vector_stream);
              if (value > 1) {
                ostringstream oss;
                oss << "<value> = " << value << " is not allowed (must be 0 or 1) in command:" << endl;
                oss << m_line;
                oss << m_p_test_vector_stream->info_for_exception();
                throw xtsc_exception(oss.str());
              }
              if (m_control_value.to_uint() == value) { count -= 1; }
              while (count) {
                wait(m_control_write_event);
                if (m_control_value.to_uint() == value) { count -= 1; }
              }
            }
            continue;
          }
        }
        if (m_words.size() != 2) {
          ostringstream oss;
          oss << "WAIT command has missing/extra/invalid arguments:" << endl;
          oss << m_line;
          oss << m_p_test_vector_stream->info_for_exception();
          throw xtsc_exception(oss.str());
        }
        double time = get_double(1, "duration", m_p_test_vector_stream);
        sc_time duration = time * m_clock_period;
        XTSC_DEBUG(m_text, "waiting " << duration);
        wait(duration);
        continue;
      }

      if ((m_words_lc[0] == "sync") || (m_words_lc[0] == "synchronize")) {
        if (m_words.size() != 2) {
          ostringstream oss;
          oss << "SYNC command has missing/extra arguments:" << endl;
          oss << m_line;
          oss << m_p_test_vector_stream->info_for_exception();
          throw xtsc_exception(oss.str());
        }
        double time = get_double(1, "time", m_p_test_vector_stream);
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

      if (m_words_lc[0] == "note") {
        XTSC_NOTE(m_text, m_line);
        continue;
      }

      if (m_words_lc[0] == "info") {
        XTSC_INFO(m_text, m_line);
        continue;
      }

      if (m_words.size() < 2) {
        ostringstream oss;
        oss << "Unrecognized/malformed command:" << endl;
        oss << m_line;
        oss << m_p_test_vector_stream->info_for_exception();
        throw xtsc_exception(oss.str());
      }

      if (m_words_lc[0] != "now") {
        double delay = get_double(0, "delay", m_p_test_vector_stream);
        XTSC_DEBUG(m_text, "write_thread is delaying for " << delay << " clock periods");
        wait(delay * m_clock_period);
        XTSC_DEBUG(m_text, "write_thread done with delay");
      }

      if (m_words_lc[1] == "stop") {
        XTSC_INFO(m_text, "write_thread calling sc_stop()");
        sc_stop();
        wait();
      }

      bool by_name = (m_line.find_first_of('=') != string::npos);

      for (u32 i=1; i<m_words.size(); ++i) {
        string value = m_words[i];
        output_definition *p_output = NULL;
        if (by_name) {
          string::size_type pos = value.find_first_of('=');
          if ((pos == string::npos) || (pos == 0) || (pos == value.size() - 1)) {
            ostringstream oss;
            oss << "Malformed <OutputName>=<value> line:" << endl;
            oss << m_line;
            oss << m_p_test_vector_stream->info_for_exception();
            throw xtsc_exception(oss.str());
          }
          string output_name = value.substr(0, pos);
          value = value.substr(pos+1);
          output_definition_map::iterator io = m_output_definition_map.find(output_name);
          if (io == m_output_definition_map.end()) {
            ostringstream oss;
            oss << kind() << " '" << name() << "': Has no output named '" << output_name << "' (word #" << (i+1) << "):" << endl;
            oss << m_line;
            oss << m_p_test_vector_stream->info_for_exception();
            throw xtsc_exception(oss.str());
          }
          p_output = io->second;
        }
        else {
          if (i>m_outputs.size()) {
            ostringstream oss;
            oss << "Too many output values:" << endl;
            oss << m_line;
            oss << m_p_test_vector_stream->info_for_exception();
            throw xtsc_exception(oss.str());
          }
          p_output = m_outputs[i-1];
        }

        // Hyphen is special character meaning to skip this output port
        if (value == "-") continue;

        // Prevent sign extension that sc_unsigned does when assigned a hex string with the high bit set
        if ((value.size() > 2) && (value.substr(0,2) == "0x")) {
          value.insert(2, "0");
        }

        if (value == "~") {
          // Tilde is special character meaning to output the bit-wise complement of previous output
          p_output->complement_output();
        }
        else {
          sc_unsigned test_convert(1);
          try {
            test_convert = value.c_str();
          }
          catch (...) {
            ostringstream oss;
            oss << "Cannot convert value in word #" << (i+1) << " ('" << value << "') to number:" << endl;
            oss << m_line;
            oss << m_p_test_vector_stream->info_for_exception();
            throw xtsc_exception(oss.str());
          }
          p_output->drive(value);
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



u32 xtsc_component::xtsc_wire_source::get_u32(u32 index, const string& argument_name, xtsc_script_file *p_script_file) {
  u32 value = 0;
  if (index >= m_words.size()) {
    ostringstream oss;
    oss << argument_name << " argument (#" << index+1 << ") missing:" << endl;
    oss << m_line;
    oss << p_script_file->info_for_exception();
    throw xtsc_exception(oss.str());
  }
  try {
    value = xtsc_strtou32(m_words[index]);
  }
  catch (const xtsc_exception&) {
    ostringstream oss;
    oss << "Cannot convert " << argument_name << " argument (#" << index+1 << ") '" << m_words[index] << "' to number:" << endl;
    oss << m_line;
    oss << p_script_file->info_for_exception();
    throw xtsc_exception(oss.str());
  }
  return value;
}



double xtsc_component::xtsc_wire_source::get_double(u32 index, const string& argument_name, xtsc_script_file *p_script_file) {
  double value = 0;
  if (index >= m_words.size()) {
    ostringstream oss;
    oss << argument_name << " argument (#" << index+1 << ") missing:" << endl;
    oss << m_line;
    oss << p_script_file->info_for_exception();
    throw xtsc_exception(oss.str());
  }
  try {
    value = xtsc_strtod(m_words[index]);
  }
  catch (const xtsc_exception&) {
    ostringstream oss;
    oss << "Cannot convert " << argument_name << " argument (#" << index+1 << ") '" << m_words[index] << "' to number:" << endl;
    oss << m_line;
    oss << p_script_file->info_for_exception();
    throw xtsc_exception(oss.str());
  }
  return value;
}



const string& xtsc_component::xtsc_wire_source::validate_identifier(u32                 index,
                                                                    const string&       argument_name,
                                                                    xtsc_script_file   *p_script_file)
{
  if (index >= m_words.size()) {
    ostringstream oss;
    oss << "The " << argument_name << " argument (#" << index+1 << ") is missing:" << endl;
    oss << m_line;
    oss << p_script_file->info_for_exception();
    throw xtsc_exception(oss.str());
  }
  if (!is_identifier(m_words[index])) {
    ostringstream oss;
    oss << "The " << argument_name << " argument ('" << m_words[index] << "') contains invalid characters:" << endl;
    oss << m_line;
    oss << p_script_file->info_for_exception();
    throw xtsc_exception(oss.str());
  }
  return m_words[index];
}



bool xtsc_component::xtsc_wire_source::is_identifier(const string& name) {
  const char *name_c_str = name.c_str();
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
    return false;
  }
  return (name != "");
}



/// Constructor
xtsc_component::xtsc_wire_source::output_definition::output_definition(xtsc_wire_source&         source,
                                                                       const std::string&        name,
                                                                       xtsc::u32                 bit_width,
                                                                       const std::string&        initial_value,
                                                                       tlm_port                 *p_tlm_port,
                                                                       pin_port                 *p_pin_port) :
  m_source              (source),
  m_name                (name),
  m_index               (source.m_outputs.size()),
  m_bit_width           (bit_width),
  m_pin_level           (source.m_pin_level),
  m_value               (bit_width),
  m_value_bv            ((int)bit_width),
  m_initial_value       (initial_value),
  m_p_tlm_port          (p_tlm_port),
  m_p_pin_port          (p_pin_port)
{
  if (m_pin_level) {
    if (!m_p_pin_port) {
     m_p_pin_port = new pin_port(m_name.c_str());
    }
  }
  else {
    if (!m_p_tlm_port) {
     m_p_tlm_port = new tlm_port(m_name.c_str());
    }
  }

  m_source.m_output_definition_map.insert(map<string, output_definition*>::value_type(m_name, this));
  m_source.m_output_set.insert(m_name);
  m_source.m_outputs.push_back(this);
  if (m_source.m_p_trace_file) {
    if (m_pin_level) {
      sc_trace(m_source.m_p_trace_file, m_value_bv, m_p_pin_port->name());
    }
    else {
      sc_trace(m_source.m_p_trace_file, m_value,    m_p_tlm_port->name());
    }
  }
}



void xtsc_component::xtsc_wire_source::xtsc_wire_write_if_impl::nb_write(const sc_unsigned& value) {
  if (static_cast<u32>(value.length()) != m_bit_width) {
    ostringstream oss;
    oss << "ERROR: Value of width=" << value.length() << " bits written to sc_export \"" << m_source.m_p_control->name()
        << "\" of width=" << m_bit_width << " in xtsc_wire_source '" << m_source.name() << "'";
    throw xtsc_exception(oss.str());
  }
  // a variable substr is used to get rid of the pesky extra leading bit
  XTSC_INFO(m_source.m_text, m_source.m_p_control->name() << " <= 0x" << value.to_string(SC_HEX).substr(m_bit_width%4 ? 2 : 3));
  m_source.m_control_write_count += 1;
  if (value != m_source.m_control_value) {
    m_source.m_control_value = value;
    m_source.m_control_change_count += 1;
  }
  m_source.m_control_write_event.notify(SC_ZERO_TIME);
}



void xtsc_component::xtsc_wire_source::xtsc_wire_write_if_impl::register_port(sc_port_base& port, const char *if_typename) {
  if (m_p_port) {
    ostringstream oss;
    oss << "Illegal multiple binding detected to sc_export<xtsc_wire_write_if> \"" << m_source.m_p_control->name()
        << "\" of xtsc_wire_source '" << m_source.name() << "'" << endl;
    oss << "  " << port.name() << endl;
    oss << "  " << m_p_port->name();
    throw xtsc_exception(oss.str());
  }
  XTSC_INFO(m_source.m_text, "Binding '" << port.name() << "' to sc_export<xtsc_wire_write_if> \"" << m_source.m_p_control->name() <<
                             "\" of xtsc_wire_source '" << m_source.name() << "'");
  m_p_port = &port;
  m_source.m_control_bound = true;
}



