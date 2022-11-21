// Copyright (c) 2006-2020 by Tensilica Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Tensilica Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Tensilica Inc.


#include <cerrno>
#include <algorithm>
#include <ostream>
#include <string>
#include <xtsc/xtsc_ext_regfile_pin.h>
#include <xtsc/xtsc_logging.h>


// xtsc_ext_regfile_pin does binary logging of ext_regfile events at verbose log level and xtsc_core does it
// at info log level.  This is the reverse of the normal arrangement because xtsc_core knows the
// PC and the port number, but xtsc_ext_regfile_pin knows neither.

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
using log4xtensa::RESPONSE_VALUE;


xtsc_component::xtsc_ext_regfile_pin::xtsc_ext_regfile_pin(sc_module_name module_name, const xtsc_ext_regfile_pin_parms& ext_regfile_parms) :
  sc_module             (module_name),
  xtsc_module           (*(sc_module*)this),
  m_wr                  ("m_wr"),
  m_wr_address          ("m_wr_address"),
  m_wr_data             ("m_wr_data"),
  m_rd_check            ("m_rd_check"),
  m_rd_check_addr       ("m_rd_check_addr"),
  m_rd_stall            ("m_rd_stall"),
  m_rd                  ("m_rd"),
  m_rd_address          ("m_rd_address"),
  m_rd_data             ("m_rd_data"),
  m_zero                (1),
  m_one                 (1),
#if IEEE_1666_SYSTEMC >= 201101L
  m_ready_event         ("m_ready_event"),
  m_data_event          ("m_data_event"),
  m_timeout_event       ("m_timeout_event"),
#endif
  m_address_bit_width   (ext_regfile_parms.get_non_zero_u32("address_bit_width")),
  m_data_bit_width      (ext_regfile_parms.get_non_zero_u32("data_bit_width")),
  m_data_registered     ((int)m_data_bit_width),
  m_text                (log4xtensa::TextLogger::getInstance(name())),
  m_binary              (log4xtensa::BinaryLogger::getInstance(name()))
{

  m_zero                = 0;
  m_one                 = 1;
  m_line_count          = 0;
  m_p_trace_file        = static_cast<sc_trace_file*>(const_cast<void*>(ext_regfile_parms.get_void_pointer("vcd_handle")));
  m_file                = 0;
  m_delay_timeout       = SC_ZERO_TIME;

  m_time_resolution     = sc_get_time_resolution();

  // Get clock period 
  u32 clock_period = ext_regfile_parms.get_non_zero_u32("clock_period");
  if (clock_period == 0xFFFFFFFF) {
    m_clock_period = xtsc_get_system_clock_period();
  }
  else {
    m_clock_period = m_time_resolution * clock_period;
  }
  m_clock_period_value = m_clock_period.value();
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

  // Get clock phase when the req signal is to be sampled
  u32 sample_phase = ext_regfile_parms.get_u32("sample_phase");
  if (sample_phase >= m_clock_period_value) {
    ostringstream oss;
    oss << "xtsc_ext_regfile_pin '" << name() << "' parameter error:" << endl;
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

  SC_THREAD(write_thread);      m_process_handles.push_back(sc_get_current_process_handle());
  SC_THREAD(read_check_thread); m_process_handles.push_back(sc_get_current_process_handle());
  SC_THREAD(read_thread);       m_process_handles.push_back(sc_get_current_process_handle());

  m_port_types["m_wr"]            = WIDE_INPUT;
  m_port_types["m_wr_address"]    = WIDE_INPUT;
  m_port_types["m_wr_data"]       = WIDE_INPUT;
  m_port_types["m_rd_check"]      = WIDE_INPUT;
  m_port_types["m_rd_check_addr"] = WIDE_INPUT;
  m_port_types["m_rd_stall"]      = WIDE_OUTPUT;
  m_port_types["m_rd"]            = WIDE_INPUT;
  m_port_types["m_rd_address"]    = WIDE_INPUT;
  m_port_types["m_rd_data"]       = WIDE_OUTPUT;
  m_port_types["ext_regfile"]     = PORT_TABLE;

  log4xtensa::LogLevel ll = xtsc_get_constructor_log_level();
  XTSC_LOG(m_text, ll, "Constructed xtsc_ext_regfile_pin '" << name() << "':");
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
  XTSC_LOG(m_text, ll, " sample_phase            = "   << sample_phase << " (" << m_sample_phase << ")");
  XTSC_LOG(m_text, ll, " vcd_handle              = "   << m_p_trace_file);

  reset();
}



xtsc_component::xtsc_ext_regfile_pin::~xtsc_ext_regfile_pin(void) {
  if (m_file) {
    delete m_file;
  }
}



xtsc::u32 xtsc_component::xtsc_ext_regfile_pin::get_bit_width(const string& port_name, u32 interface_num) const {
  if (port_name == "m_wr")              return 1;
  if (port_name == "m_wr_address")      return m_address_bit_width;
  if (port_name == "m_wr_data")         return m_data_bit_width;
  if (port_name == "m_rd_check")        return 1;
  if (port_name == "m_rd_check_addr")   return m_address_bit_width;
  if (port_name == "m_rd_stall")        return 1;
  if (port_name == "m_rd")              return 1;
  if (port_name == "m_rd_address")      return m_address_bit_width;
  if (port_name == "m_rd_data")         return m_data_bit_width;

  ostringstream oss;
  oss << kind() << " \"" << name() << "\" has no port named \"" << port_name << "\"" << endl;
  throw xtsc_exception(oss.str());
}



sc_object *xtsc_component::xtsc_ext_regfile_pin::get_port(const string& port_name) {
  if (port_name == "m_wr")              return &m_wr;
  if (port_name == "m_wr_address")      return &m_wr_address;
  if (port_name == "m_wr_data")         return &m_wr_data;
  if (port_name == "m_rd_check")        return &m_rd_check;
  if (port_name == "m_rd_check_addr")   return &m_rd_check_addr;
  if (port_name == "m_rd_stall")        return &m_rd_stall;
  if (port_name == "m_rd")              return &m_rd;
  if (port_name == "m_rd_address")      return &m_rd_address;
  if (port_name == "m_rd_data")         return &m_rd_data;

  ostringstream oss;
  oss << kind() << " \"" << name() << "\" has no port named \"" << port_name << "\"" << endl;
  throw xtsc_exception(oss.str());
}



xtsc_port_table xtsc_component::xtsc_ext_regfile_pin::get_port_table(const string& port_table_name) const {
  xtsc_port_table table;

  if (port_table_name == "ext_regfile") {
    table.push_back("m_wr");
    table.push_back("m_wr_address");
    table.push_back("m_wr_data");
    table.push_back("m_rd_check");
    table.push_back("m_rd_check_addr");
    table.push_back("m_rd_stall");
    table.push_back("m_rd");
    table.push_back("m_rd_address");
    table.push_back("m_rd_data");
  }
  else {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\" has no port table named \"" << port_table_name << "\"" << endl;
    throw xtsc_exception(oss.str());
  }

  return table;
}



void xtsc_component::xtsc_ext_regfile_pin::before_end_of_elaboration() {
  if (m_p_trace_file) {
    sc_trace(m_p_trace_file, m_wr,            m_wr            .name());
    sc_trace(m_p_trace_file, m_wr_address,    m_wr_address    .name());
    sc_trace(m_p_trace_file, m_wr_data,       m_wr_data       .name());
    sc_trace(m_p_trace_file, m_rd_check,      m_rd_check      .name());
    sc_trace(m_p_trace_file, m_rd_check_addr, m_rd_check_addr .name());
    sc_trace(m_p_trace_file, m_rd_stall,      m_rd_stall      .name());
    sc_trace(m_p_trace_file, m_rd,            m_rd            .name());
    sc_trace(m_p_trace_file, m_rd_address,    m_rd_address    .name());
    sc_trace(m_p_trace_file, m_rd_data,       m_rd_data       .name());
  }
}



void xtsc_component::xtsc_ext_regfile_pin::reset(bool /* hard_reset */) {
  XTSC_INFO(m_text, kind() << "::reset()");

  // Cancel notified events (XXXSS: TODO)
  m_ready_event  .cancel();
  m_data_event   .cancel();
  m_timeout_event.cancel();

  xtsc_reset_processes(m_process_handles);
}



void xtsc_component::xtsc_ext_regfile_pin::write_thread() {
  try {

    while (true) {

      if (m_wr.read() == m_zero) {
        // (1) Wait for m_wr to change 
        wait(m_wr.value_changed_event());
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

      // (3) Get write address and data
      xtsc::u32 wr_addr = m_wr_address.read().to_uint();
      sc_bv_base wr_data      = m_wr_data.read();

      // (4) Write data to external regfile and start the operation.
      //For external register file, write request implies start of operation.
      //TODO: Right now doing nothing, except writing to the register.
      m_data_map[wr_addr]       = wr_data;
      //Calculate operation finish time
      u64     delay             = wr_addr % 10; //Delay between 0-9
      sc_time now               = sc_time_stamp();
      u64     cycle             = now.value() / m_clock_period_value; // current cycle
      sc_time posedge_clock     = cycle * m_clock_period;             // posedge clock of current cycle
      sc_time delay_time        = m_clock_period * (delay + 1);       // delay as sc_time
      sc_time op_fini_time      = posedge_clock + delay_time;         // time at which operation finishes
      m_op_fini_time[wr_addr]   = op_fini_time;

      // (5) Wait one clock period before doing it again
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


void xtsc_component::xtsc_ext_regfile_pin::read_check_thread() {
  try {

    m_rd_stall.write(m_one);

    while (true) {
      RD_CHECK_ZERO_WAIT:
      if (m_rd_check.read() == m_zero) {
        // (1) Wait for m_rd_check to change 
        wait(m_rd_check.value_changed_event());
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

      // (3) Get read-check address
      xtsc::u32 rd_check_addr = m_rd_check_addr.read().to_uint();

      // (4) Wait for the operation to finish or for rd_check to change
      while (1) {
        if (sc_time_stamp() < m_op_fini_time[rd_check_addr]) {
          m_rd_stall.write(m_one);
          XTSC_DEBUG(m_text, "read_check_thread: rd_stall asserted for rd_check_addr=0x" << hex << rd_check_addr << dec);
        } else {
          break;
        }

        if (m_rd_check.read() == m_zero) { //Return to step (1)
          goto RD_CHECK_ZERO_WAIT;
        }

        if (m_rd_check_addr.read().to_uint() != rd_check_addr) //read-check adr changed
          rd_check_addr = m_rd_check_addr.read().to_uint();

        wait(m_clock_period);
      }

      XTSC_DEBUG(m_text, "read_check_thread: rd_stall deasserted for rd_check_addr=0x" << hex << rd_check_addr << dec);
      m_rd_stall.write(m_zero);
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



void xtsc_component::xtsc_ext_regfile_pin::read_thread() {
  try {

    m_rd_data.write(m_zero);

    while (true) {
      if (m_rd.read() == m_zero) {
        // (1) Wait for m_rd to change 
        wait(m_rd.value_changed_event());
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

      // (3) Get read address
      const xtsc::u32 rd_addr = m_rd_address.read().to_uint();

      // (4) Drive read-data
      m_rd_data.write(m_data_map[rd_addr]);
      XTSC_DEBUG(m_text, "read_thread: rd_data=0x" << hex << m_data_map[rd_addr] << dec);

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



void xtsc_component::xtsc_ext_regfile_pin::get_sc_bv_base(u32 index, sc_bv_base& value) {
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

