// Copyright (c) 2005-2018 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Cadence Design Systems, Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Cadence Design Systems, Inc.


/*
 * TODO:
 *
 * - Before do_b_transport decide how to set the use_tlm2_busy argument
 * - After do_b_transport decide what to do with time value t and use_tlm2_busy
 * - Support nb_transport and implement nb_transport_bw
 *   - Add a script file command to set m_use_b_transport
 * - Support optionally calling b_transport from a separate thread
 *   - Support multiple extra threads so that several b_transport calls can be outstanding
 *     at the same time
 * - Support waiting on the nb_transport transaction completion
 * - Support waiting on the separate-thread version of b_transport transaction completion
 * - Have a parameter to say what should happen if the b_transport call returns an error code
 * - Add support for calling get_direct_mem_ptr and logging the result.
 */

#include <cctype>
#include <algorithm>
#include <xtsc/xtsc_master_tlm2.h>
#include <xtsc/xtsc_tlm2.h>

using namespace std;
using namespace sc_core;
using namespace sc_dt;
using namespace tlm;
using namespace xtsc;
using namespace log4xtensa;



xtsc_component::xtsc_master_tlm2::xtsc_master_tlm2(sc_module_name module_name, const xtsc_master_tlm2_parms& master_parms) :
  sc_module                     (module_name),
  xtsc_module                   (*(sc_module*)this),
  m_initiator_socket_4          (NULL),
  m_initiator_socket_8          (NULL),
  m_initiator_socket_16         (NULL),
  m_initiator_socket_32         (NULL),
  m_initiator_socket_64         (NULL),
  m_tlm_bw_transport_if_impl    ("m_tlm_bw_transport_if_impl", *this),
  m_text                        (log4xtensa::TextLogger::getInstance(name())),
  m_width8                      (master_parms.get_u32   ("byte_width")),
  m_streaming_width             (master_parms.get_u32   ("streaming_width")),
  m_wraparound                  (master_parms.get_bool  ("wraparound")),
  m_script_file_stream          (master_parms.get_c_str ("script_file"),  "script_file",  name(), kind(), m_wraparound),
  m_script_file                 (m_script_file_stream.name()),
//m_use_b_transport             (true),
  m_last_read_write             (""),
  m_p_byte_enable_length        (0),
  m_p_byte_enables              (NULL),
  m_p_port                      (0)
{

  if ((m_width8 != 4) && (m_width8 != 8) && (m_width8 != 16) && (m_width8 != 32) && (m_width8 != 64)) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': Invalid \"byte_width\" parameter value (" << m_width8 << ").  Expected 4|8|16|32|64.";
    throw xtsc_exception(oss.str());
  }

  ostringstream oss1;
  oss1 << "m_initiator_socket_" << m_width8;
       if (m_width8 ==  4) { m_initiator_socket_4  = new initiator_socket_4 (oss1.str().c_str()); }
  else if (m_width8 ==  8) { m_initiator_socket_8  = new initiator_socket_8 (oss1.str().c_str()); }
  else if (m_width8 == 16) { m_initiator_socket_16 = new initiator_socket_16(oss1.str().c_str()); }
  else if (m_width8 == 32) { m_initiator_socket_32 = new initiator_socket_32(oss1.str().c_str()); }
  else if (m_width8 == 64) { m_initiator_socket_64 = new initiator_socket_64(oss1.str().c_str()); }
  ;
       if (m_width8 ==  4) { (*m_initiator_socket_4 )(m_tlm_bw_transport_if_impl); }
  else if (m_width8 ==  8) { (*m_initiator_socket_8 )(m_tlm_bw_transport_if_impl); }
  else if (m_width8 == 16) { (*m_initiator_socket_16)(m_tlm_bw_transport_if_impl); }
  else if (m_width8 == 32) { (*m_initiator_socket_32)(m_tlm_bw_transport_if_impl); }
  else if (m_width8 == 64) { (*m_initiator_socket_64)(m_tlm_bw_transport_if_impl); }

  m_data_fill_byte = (u8) master_parms.get_u32("data_fill_byte");

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

  SC_THREAD(script_thread);     m_process_handles.push_back(sc_get_current_process_handle());

  switch (m_width8) {
    case  4:  m_port_types["m_initiator_socket_4" ] = INITIATOR_SOCKET_4;  break;
    case  8:  m_port_types["m_initiator_socket_8" ] = INITIATOR_SOCKET_8;  break;
    case 16:  m_port_types["m_initiator_socket_16"] = INITIATOR_SOCKET_16; break;
    case 32:  m_port_types["m_initiator_socket_32"] = INITIATOR_SOCKET_32; break;
    case 64:  m_port_types["m_initiator_socket_64"] = INITIATOR_SOCKET_64; break;
  }
  m_port_types["initiator_socket"] = PORT_TABLE;

  xtsc_register_command(*this, *this, "dump_last_read_write", 0, 0,
      "dump_last_read_write", 
      "Dump the log line from the most recent previous read or write command."
  );

  xtsc_register_command(*this, *this, "peek", 2, 2,
      "peek <StartAddress> <NumBytes>", 
      "Use transport_dbg() to peek <NumBytes> of memory starting at <StartAddress>."
  );

  xtsc_register_command(*this, *this, "poke", 2, -1,
      "poke <StartAddress> <NumBytes> <Byte1> <Byte2> . . . <ByteN>", 
      "Use transport_dbg() to poke <NumBytes> (=N) of memory starting at <StartAddress>."
  );

  xtsc_register_command(*this, *this, "reset", 0, 1,
      "reset [<Hard>]", 
      "Call xtsc_master_tlm2::reset(<Hard>).  Where <Hard> is 0|1 (default 0)."
  );

  log4xtensa::LogLevel ll = xtsc_get_constructor_log_level();
  XTSC_LOG(m_text, ll, "Constructed xtsc_master_tlm2 '" << name() << "':");
  XTSC_LOG(m_text, ll, " byte_width              = "   << m_width8);
  XTSC_LOG(m_text, ll, " script_file             = "   << m_script_file);
  XTSC_LOG(m_text, ll, " wraparound              = "   << boolalpha << m_wraparound);
  if (m_streaming_width == 0xFFFFFFFF) {
  XTSC_LOG(m_text, ll, " streaming_width         = 0xFFFFFFFF => Match data length attribute (size)");
  } else {
  XTSC_LOG(m_text, ll, " streaming_width         = "   << m_streaming_width << " = 0x" << hex << m_streaming_width);
  }
  XTSC_LOG(m_text, ll, " data_fill_byte          = 0x" << hex << (u32) m_data_fill_byte);
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

  reset();

}



xtsc_component::xtsc_master_tlm2::~xtsc_master_tlm2(void) {
}



u32 xtsc_component::xtsc_master_tlm2::get_bit_width(const string& port_name, u32 interface_num) const {
  return m_width8*8;
}



sc_object *xtsc_component::xtsc_master_tlm2::get_port(const string& port_name) {
       if ((port_name == "m_initiator_socket_4" ) && (m_width8 ==  4))  return m_initiator_socket_4;
  else if ((port_name == "m_initiator_socket_8" ) && (m_width8 ==  8))  return m_initiator_socket_8;
  else if ((port_name == "m_initiator_socket_16") && (m_width8 == 16))  return m_initiator_socket_16;
  else if ((port_name == "m_initiator_socket_32") && (m_width8 == 32))  return m_initiator_socket_32;
  else if ((port_name == "m_initiator_socket_64") && (m_width8 == 64))  return m_initiator_socket_64;
  else {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\" has no port named \"" << port_name << "\"" << endl;
    throw xtsc_exception(oss.str());
  }
}



xtsc_port_table xtsc_component::xtsc_master_tlm2::get_port_table(const string& port_table_name) const {
  if (port_table_name != "initiator_socket") {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\" has no port table named \"" << port_table_name << "\"" << endl;
    throw xtsc_exception(oss.str());
  }

  xtsc_port_table table;
  ostringstream oss;

  oss.str(""); oss << "m_initiator_socket_" << m_width8; table.push_back(oss.str());

  return table;
}



void xtsc_component::xtsc_master_tlm2::execute(const string&                 cmd_line, 
                                               const vector<string>&         words,
                                               const vector<string>&         words_lc,
                                               ostream&                      result)
{
  ostringstream res;

  if (false) {
  }
  else if (words[0] == "dump_last_read_write") {
    dump_last_read_write(res);
  }
  else if (words[0] == "peek") {
    u64 start_address = xtsc_command_argtou64(cmd_line, words, 1);
    u32 num_bytes     = xtsc_command_argtou32(cmd_line, words, 2);
    u8 *buffer = new u8[num_bytes];
    do_peek(start_address, num_bytes, buffer);
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
      do_poke(start_address, num_bytes, buffer);
      delete [] buffer;
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



void xtsc_component::xtsc_master_tlm2::reset(bool /*hard_reset*/) {
  XTSC_INFO(m_text, "xtsc_master_tlm2::reset()");

  m_line                        = "";
  m_line_count                  = 0;

  m_words.clear();

  m_script_file_stream.reset();

  xtsc_reset_processes(m_process_handles);
}



u8 *xtsc_component::xtsc_master_tlm2::new_u8_array(u32 size8) {
  u8 *p_array = NULL;
  multimap<u32,u8*>::iterator i = m_u8_array_pool.find(size8);
  if (i == m_u8_array_pool.end()) {
    p_array = new u8[size8];
    XTSC_DEBUG(m_text, "Created new u8[" << size8 << "] " << (void*) p_array);
  }
  else {
    p_array = i->second;
    m_u8_array_pool.erase(i);
    XTSC_DEBUG(m_text, "Reusing u8[" << size8 << "] " << (void*) p_array);
  }
  memset(p_array, m_data_fill_byte, size8);
  return p_array;
}



void xtsc_component::xtsc_master_tlm2::delete_u8_array(u32 size8, u8*& p_array) {
    XTSC_DEBUG(m_text, "Recycling u8[" << size8 << "] " << (void*) p_array);
    m_u8_array_pool.insert(make_pair(size8, p_array));
    p_array = NULL;
}



u8 *xtsc_component::xtsc_master_tlm2::duplicate_u8_array(u32 size8, const u8 *p_src) {
  if (!p_src) return NULL;
  u8 *p_dst = new_u8_array(size8);
  memcpy(p_dst, p_src, size8);
  return p_dst;
}



tlm_generic_payload *xtsc_component::xtsc_master_tlm2::new_transaction() {
  if (m_transaction_pool.empty()) {
    tlm_generic_payload *p_trans = new tlm_generic_payload(this);
    XTSC_DEBUG(m_text, "Created new tlm_generic_payload " << p_trans);
    p_trans->acquire();
    return p_trans;
  }
  else {
    tlm_generic_payload *p_trans = m_transaction_pool.back();
    m_transaction_pool.pop_back();
    XTSC_DEBUG(m_text, "Reusing tlm_generic_payload " << p_trans);
    p_trans->acquire();
    return p_trans;
  }
}



void xtsc_component::xtsc_master_tlm2::free(tlm_generic_payload *p_trans) {
  XTSC_DEBUG(m_text, "Recycling tlm_generic_payload " << p_trans);
  if (p_trans->get_ref_count()) {
    ostringstream oss;
    oss << name() << " - xtsc_master_tlm2::free() called for a transaction with a non-zero reference count of "
        << p_trans->get_ref_count();
    throw xtsc_exception(oss.str());
  }
  u8 *p_data = p_trans->get_data_ptr();
  if (p_data) {
    delete_u8_array(p_trans->get_data_length(), p_data);
  }
  u8 *p_enables = p_trans->get_byte_enable_ptr();
  if (p_enables) {
    delete_u8_array(p_trans->get_byte_enable_length(), p_enables);
  }
  p_trans->reset();
  m_transaction_pool.push_back(p_trans);
}



void xtsc_component::xtsc_master_tlm2::validate_port_width(u32 width8) {

  if (width8 != m_width8) {
    ostringstream oss;
    oss << " '" << name() << "': xtsc_master_tlm2::get_initiator_socket_" << width8<< "() called but bus width is " << m_width8
        << " so you may only call get_initiator_socket_" << m_width8 << "()";
    throw xtsc_exception(oss.str());
  }

}



xtsc_component::xtsc_master_tlm2::initiator_socket_4& xtsc_component::xtsc_master_tlm2::get_initiator_socket_4() {
  validate_port_width(4);
  return *m_initiator_socket_4;
}



xtsc_component::xtsc_master_tlm2::initiator_socket_8& xtsc_component::xtsc_master_tlm2::get_initiator_socket_8() {
  validate_port_width(8);
  return *m_initiator_socket_8;
}



xtsc_component::xtsc_master_tlm2::initiator_socket_16& xtsc_component::xtsc_master_tlm2::get_initiator_socket_16() {
  validate_port_width(16);
  return *m_initiator_socket_16;
}



xtsc_component::xtsc_master_tlm2::initiator_socket_32& xtsc_component::xtsc_master_tlm2::get_initiator_socket_32() {
  validate_port_width(32);
  return *m_initiator_socket_32;
}



xtsc_component::xtsc_master_tlm2::initiator_socket_64& xtsc_component::xtsc_master_tlm2::get_initiator_socket_64() {
  validate_port_width(64);
  return *m_initiator_socket_64;
}



void xtsc_component::xtsc_master_tlm2::dump_last_read_write(ostream& os) {
  os << m_last_read_write;;
}



void xtsc_component::xtsc_master_tlm2::do_poke(xtsc_address address8, u32 size8, u8 *buffer) {
  tlm_generic_payload *p_trans = new_transaction();
  p_trans->set_command        (TLM_WRITE_COMMAND);
  p_trans->set_address        (address8);
  p_trans->set_data_length    (size8);
  p_trans->set_streaming_width((m_streaming_width == 0xFFFFFFFF) ? size8 : m_streaming_width);
  p_trans->set_response_status(TLM_INCOMPLETE_RESPONSE);
  p_trans->set_data_ptr       (buffer);
  p_trans->set_byte_enable_ptr(0);
  do_transport_dbg(*p_trans);
  p_trans->release();
}



void xtsc_component::xtsc_master_tlm2::do_peek(xtsc_address address8, u32 size8, u8 *buffer) {
  tlm_generic_payload *p_trans = new_transaction();
  p_trans->set_command        (TLM_READ_COMMAND);
  p_trans->set_address        (address8);
  p_trans->set_data_length    (size8);
  p_trans->set_streaming_width((m_streaming_width == 0xFFFFFFFF) ? size8 : m_streaming_width);
  p_trans->set_response_status(TLM_INCOMPLETE_RESPONSE);
  p_trans->set_data_ptr       (buffer);
  p_trans->set_byte_enable_ptr(0);
  do_transport_dbg(*p_trans);
  p_trans->release();
}



void xtsc_component::xtsc_master_tlm2::throw_missing_or_extra_arguments(bool missing) {
  ostringstream oss;
  oss << "'" << m_words[0] << "' command has " << (missing ? "missing" : "extra") << " arguments: " << endl;
  oss << m_line;
  oss << m_script_file_stream.info_for_exception();
  throw xtsc_exception(oss.str());
}



void xtsc_component::xtsc_master_tlm2::script_thread(void) {

  try {

    u32 n = xtsc_address_nibbles();

    while (get_words() != 0) {

      XTSC_DEBUG(m_text, "\"script_file\" line #" << m_line_count << ": " << m_line);

      double       delay    = 0;
      bool         poke     = false;
      bool         peek     = false;
      bool         stop     = false;
      bool         read     = false;
      bool         write    = false;
      bool         do_delay = (m_words[0] != "now");
      u32          size     = 0;
      u64          address8 = 0;
      u8          *p_array  = NULL;

      if (m_words[0] == "wait") {
        do_delay = false;
        if (m_words.size() < 2) {
          throw_missing_or_extra_arguments(true);
        }
        else {
          double time = get_double(1, "duration");
          sc_time duration = time * m_clock_period;
          XTSC_DEBUG(m_text, "waiting " << duration);
          wait(duration);
        }
      }
      else if ((m_words[0] == "sync") || (m_words[0] == "synchronize")) {
        do_delay = false;
        if (m_words.size() != 2) {
          throw_missing_or_extra_arguments(m_words.size() < 2);
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
      else if ((m_words[0] == "byte_enables") || (m_words[0] == "bytes") || (m_words[0] == "be") || (m_words[0] == "enables")) {
        do_delay = false;
        if ((m_words.size() < 2) && (m_words.size() > 3)) {
          throw_missing_or_extra_arguments(m_words.size() < 2);
        }
        size = get_u32(1, "size");
        bool default_enables = true;
        if (m_words.size() == 3) {
          default_enables = false;
          if (m_words[2].find_first_not_of("01") != string::npos) {
            ostringstream oss;
            oss << "byte_enables argument (\"" << m_words[2] << "\") contains characters other then '0' and '1': " << endl;
            oss << m_line;
            oss << m_script_file_stream.info_for_exception();
            throw xtsc_exception(oss.str());
          }
          if (m_words[2].length() != size) {
            ostringstream oss;
            oss << "byte_enables argument length of " << m_words[2].length() << " doesn't match size argument of " << size << ":" << endl;
            oss << m_line;
            oss << m_script_file_stream.info_for_exception();
            throw xtsc_exception(oss.str());
          }
        }
        if (m_p_byte_enables) {
          delete_u8_array(m_p_byte_enable_length, m_p_byte_enables);
        }
        m_p_byte_enables = new_u8_array(size);
        m_p_byte_enable_length = size;
        for (u32 i=0; i<size; ++i) {
          m_p_byte_enables[i] = ((default_enables || (m_words[2][i] == '1')) ?  TLM_BYTE_ENABLED : TLM_BYTE_DISABLED);
        }
      }
      else if (m_words[0] == "stream") {
        do_delay = false;
        if (m_words.size() != 2) {
          throw_missing_or_extra_arguments(m_words.size() < 2);
        }
        m_streaming_width = get_u32(1, "streaming_width");
      }
      else if (m_words[0] == "info") {
        do_delay = false;
        XTSC_INFO(m_text, m_line);
      }
      else if (m_words[0] == "note") {
        do_delay = false;
        XTSC_NOTE(m_text, m_line);
      }
      else {
        if (m_words.size() == 1) {
          ostringstream oss;
          oss << "Line has too few words: " << endl;
          oss << m_line;
          oss << m_script_file_stream.info_for_exception();
          throw xtsc_exception(oss.str());
        }
        if (m_words[1] == "stop") {
          // delay STOP
          stop = true;
        }
        else if (m_words.size() >= 4) {
          if ((m_words[1] != "poke") && (m_words[1] != "peek") && (m_words[1] != "read") && (m_words[1] != "write")) {
            ostringstream oss;
            oss << "Unrecognized request type '" << m_words[1] << "':" << endl;
            oss << m_line;
            oss << m_script_file_stream.info_for_exception();
            throw xtsc_exception(oss.str());
          }
          address8 = get_u64(2, "address");
          size     = get_u32(3, "size");
          if (m_words[1] == "poke") {
            // 0     1     2       3    4  5  ... 3+size
            // delay POKE  address size b0 b1 ... bN
            p_array = new_u8_array(size);
            set_buffer(4, size, p_array);
            poke = true;
          }
          else if (m_words[1] == "peek") {
            // 0     1     2       3   
            // delay PEEK  address size 
            check_for_too_many_parameters(4);
            p_array = new_u8_array(size);
            peek = true;
          }
          else if (m_words[1] == "read") {
            // 0     1     2       3   
            // delay READ  address size 
            check_for_too_many_parameters(4);
            p_array = new_u8_array(size);
            read = true;
          }
          else if (m_words[1] == "write") {
            // 0     1     2       3    4  5  ... 3+size
            // delay WRITE address size b0 b1 ... bN
            p_array = new_u8_array(size);
            set_buffer(4, size, p_array);
            write = true;
          }
          else {
            ostringstream oss;
            oss << "Unrecognized request type '" << m_words[1] << "':" << endl;
            oss << m_line;
            oss << m_script_file_stream.info_for_exception();
            throw xtsc_exception(oss.str());
          }
        }
        else {
          ostringstream oss;
          oss << "Unrecognized request '" << m_words[0] << "':" << endl;
          oss << m_line;
          oss << m_script_file_stream.info_for_exception();
          throw xtsc_exception(oss.str());
        }
      }


      if (do_delay) {
          delay = get_double(0, "delay");
          XTSC_DEBUG(m_text, "waiting for " << (delay * m_clock_period));
          wait(delay * m_clock_period);
      }
      if (poke) {
        ostringstream oss;
        oss << "poke 0x" << hex << setfill('0') << setw(n) << address8 << setfill(' ') << ": ";
        xtsc_hex_dump(true, size, p_array, oss);
        XTSC_INFO(m_text, oss.str());
        do_poke(address8, size, p_array);
      }
      if (peek) {
        do_peek(address8, size, p_array);
        ostringstream oss;
        oss << "peek 0x" << hex << setfill('0') << setw(n) << address8 << setfill(' ') << ": ";
        xtsc_hex_dump(true, size, p_array, oss);
        XTSC_INFO(m_text, oss.str());
      }
      if (read) {
        tlm_generic_payload *p_trans = new_transaction();
        p_trans->set_command            (TLM_READ_COMMAND);
        p_trans->set_address            (address8);
        p_trans->set_data_length        (size);
        p_trans->set_streaming_width    ((m_streaming_width == 0xFFFFFFFF) ? size : m_streaming_width);
        p_trans->set_response_status    (TLM_INCOMPLETE_RESPONSE);
        p_trans->set_data_ptr           (p_array);
        p_trans->set_byte_enable_ptr    (duplicate_u8_array(m_p_byte_enable_length, m_p_byte_enables));
        p_trans->set_byte_enable_length (m_p_byte_enable_length);
        sc_time t = SC_ZERO_TIME;
        do_b_transport(*p_trans, t, true);
        ostringstream oss;
        oss << "read  0x" << hex << setfill('0') << setw(n) << address8 << setfill(' ') << ": ";
        xtsc_hex_dump(true, size, p_array, oss);
        XTSC_INFO(m_text, oss.str());
        p_trans->release();
        m_last_read_write = oss.str();
      }
      if (write) {
        tlm_generic_payload *p_trans = new_transaction();
        p_trans->set_command            (TLM_WRITE_COMMAND);
        p_trans->set_address            (address8);
        p_trans->set_data_length        (size);
        p_trans->set_streaming_width    ((m_streaming_width == 0xFFFFFFFF) ? size : m_streaming_width);
        p_trans->set_response_status    (TLM_INCOMPLETE_RESPONSE);
        p_trans->set_data_ptr           (p_array);
        p_trans->set_byte_enable_ptr    (duplicate_u8_array(m_p_byte_enable_length, m_p_byte_enables));
        p_trans->set_byte_enable_length (m_p_byte_enable_length);
        sc_time t = SC_ZERO_TIME;
        ostringstream oss;
        oss << "write 0x" << hex << setfill('0') << setw(n) << address8 << setfill(' ') << ": ";
        xtsc_hex_dump(true, size, p_array, oss);
        XTSC_INFO(m_text, oss.str());
        do_b_transport(*p_trans, t, true);
        p_trans->release();
        m_last_read_write = oss.str();
      }
      if (stop) {
        sc_stop();
        wait();
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



void xtsc_component::xtsc_master_tlm2::do_b_transport(tlm_generic_payload& trans, sc_time& delay, bool use_tlm2_busy) {
  tlm_response_status status = TLM_INCOMPLETE_RESPONSE;

  do {
         if (m_width8 ==  4) { (*m_initiator_socket_4 )->b_transport(trans, delay); }
    else if (m_width8 ==  8) { (*m_initiator_socket_8 )->b_transport(trans, delay); }
    else if (m_width8 == 16) { (*m_initiator_socket_16)->b_transport(trans, delay); }
    else if (m_width8 == 32) { (*m_initiator_socket_32)->b_transport(trans, delay); }
    else if (m_width8 == 64) { (*m_initiator_socket_64)->b_transport(trans, delay); }
    else { throw xtsc_exception("Program Bug in xtsc_master_tlm2.cpp - unsupported bus width"); }
    status = trans.get_response_status();
    if (use_tlm2_busy && (status == TLM_INCOMPLETE_RESPONSE)) {
      wait(m_clock_period);
    }
  } while (use_tlm2_busy && (status == TLM_INCOMPLETE_RESPONSE));

  if (status != TLM_OK_RESPONSE) {
    if (status == TLM_ADDRESS_ERROR_RESPONSE) {
    }
    else if (status == TLM_GENERIC_ERROR_RESPONSE) {
    }
    else {
      ostringstream oss;
      oss << name() << "(in xtsc_master_tlm2::do_b_transport): b_transport() failed:  " << trans;
      XTSC_INFO(m_text, oss.str());
//    throw xtsc_exception(oss.str());
    }
  }
}



void xtsc_component::xtsc_master_tlm2::do_transport_dbg(tlm_generic_payload& trans) {
  u32 count = 0;

       if (m_width8 ==  4) { count = (*m_initiator_socket_4 )->transport_dbg(trans); }
  else if (m_width8 ==  8) { count = (*m_initiator_socket_8 )->transport_dbg(trans); }
  else if (m_width8 == 16) { count = (*m_initiator_socket_16)->transport_dbg(trans); }
  else if (m_width8 == 32) { count = (*m_initiator_socket_32)->transport_dbg(trans); }
  else if (m_width8 == 64) { count = (*m_initiator_socket_64)->transport_dbg(trans); }
  else { throw xtsc_exception("Program Bug in xtsc_master_tlm2.cpp - unsupported bus width"); }

  if (count != trans.get_data_length()) {
    ostringstream oss;
    oss << name() << " (in xtsc_master_tlm2::do_transport_dbg): transport_dbg() failed: " << trans
        << " count=" << count << " (possibly downstream model doesn't support that address or doesn't support data length of "
        << trans.get_data_length() << ")";
    throw xtsc_exception(oss.str());
  }
}



int xtsc_component::xtsc_master_tlm2::get_words() {
  m_line_count = m_script_file_stream.get_words(m_words, m_line, true);
  return m_words.size();
}




u32 xtsc_component::xtsc_master_tlm2::get_u32(u32 index, const string& argument_name) {
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



u64 xtsc_component::xtsc_master_tlm2::get_u64(u32 index, const string& argument_name) {
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



double xtsc_component::xtsc_master_tlm2::get_double(u32 index, const string& argument_name) {
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



void xtsc_component::xtsc_master_tlm2::check_for_too_many_parameters(u32 num_expected) {
  if (m_words.size() > num_expected) {
    ostringstream oss;
    oss << "xtsc_master_tlm2 '" << name() << "' found too many words on line (expected=" << num_expected << " has=" << m_words.size()
        << "):" << endl;
    oss << m_line;
    oss << m_script_file_stream.info_for_exception();
    throw xtsc_exception(oss.str());
  }
}



void xtsc_component::xtsc_master_tlm2::check_for_too_few_parameters(u32 num_expected) {
  if (m_words.size() < num_expected) {
    ostringstream oss;
    oss << "xtsc_master_tlm2 '" << name() << "' found too few words on line (expected=" << num_expected << " has=" << m_words.size()
        << "):" << endl;
    oss << m_line;
    oss << m_script_file_stream.info_for_exception();
    throw xtsc_exception(oss.str());
  }
}



void xtsc_component::xtsc_master_tlm2::set_buffer(u32 index, u32 size8, u8 *p_dst) {
  XTSC_DEBUG(m_text, "set_buffer: index=" << index << " size8=" << size8 << " m_words.size()=" << m_words.size());
  u32 num_expected = index + size8;
  check_for_too_many_parameters(num_expected);
  check_for_too_few_parameters(num_expected);
  for (u32 i=0; i<size8; ++i) {
    u32 value = get_u32(index+i, "data");
    p_dst[i] = static_cast<u8>(value);
  }
}



tlm_sync_enum xtsc_component::xtsc_master_tlm2::tlm_bw_transport_if_impl::nb_transport_bw(tlm_generic_payload&      trans,
                                                                                          tlm_phase&                phase,
                                                                                          sc_time&                  time)
{
  throw xtsc_exception("xtsc_master_tlm2::tlm_bw_transport_if_impl::nb_transport_bw() called but it is not supported.");
//return TLM_ACCEPTED;
}



void xtsc_component::xtsc_master_tlm2::tlm_bw_transport_if_impl::invalidate_direct_mem_ptr(uint64 start_range, uint64 end_range) {
  XTSC_INFO(m_master.m_text, "invalidate_direct_mem_ptr() range: 0x" << hex << start_range << "-0x" << end_range);
}



void xtsc_component::xtsc_master_tlm2::tlm_bw_transport_if_impl::register_port(sc_port_base& port, const char *if_typename) {
  if (m_p_port) {
    ostringstream oss;
    oss << "Illegal multiple binding detected to xtsc_master_tlm2 '" << m_master.name() << "' " << name() << ": " << endl;
    oss << "  " << port.name() << endl;
    oss << "  " << m_p_port->name();
    throw xtsc_exception(oss.str());
  }
  XTSC_INFO(m_master.m_text, "Binding '" << port.name() << "' to xtsc_master_tlm2::" << name());
  m_p_port = &port;
}



