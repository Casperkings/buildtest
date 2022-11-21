// Copyright (c) 2005-2017 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of
// Cadence Design Systems, Inc.  They may be adapted and modified by bona fide
// purchasers for internal use, but neither the original nor any adapted
// or modified version may be disclosed or distributed to third parties
// in any manner, medium, or form, in whole or in part, without the prior
// written consent of Cadence Design Systems, Inc.


#include "xtsc/xtsc_dma_engine.h"
#include <string>
#include <sstream>

using namespace std;
using namespace sc_core;
using namespace log4xtensa;
using namespace xtsc;




xtsc_component::xtsc_dma_engine::xtsc_dma_engine(sc_module_name module_name, const xtsc_dma_engine_parms& dma_parms) :
  xtsc_memory                           (module_name, dma_parms),
  m_request_port                        ("m_request_port"),
  m_respond_export                      ("m_respond_export"),
  m_respond_impl                        ("m_respond_impl", *this),
#if IEEE_1666_SYSTEMC >= 201101L
  m_dma_thread_event                    ("m_dma_thread_event"),
  m_request_thread_event                ("m_request_thread_event"),
  m_write_thread_event                  ("m_write_thread_event"),
  m_got_read_response_event             ("m_got_read_response_event"),
  m_no_reads_or_writes_event            ("m_no_reads_or_writes_event"),
  m_single_response_available_event     ("m_single_response_available_event"),
  m_block_read_response_available_event ("m_block_read_response_available_event"),
  m_block_write_response_available_event("m_block_write_response_available_event"),
#endif
  m_request_got_nacc                    (false)
{

  m_p_single_response      = 0;
  m_p_block_write_response = 0;

  m_reg_base_address    = dma_parms.get_u32("reg_base_address");
  m_max_reads           = dma_parms.get_u32("max_reads");
  m_max_writes          = dma_parms.get_u32("max_writes");
  m_descriptor_delay    = dma_parms.get_u32("descriptor_delay");
  m_read_priority       = dma_parms.get_u32("read_priority");
  m_write_priority      = dma_parms.get_u32("write_priority");
  m_overlap_descriptors = dma_parms.get_bool("overlap_descriptors");
  m_clear_notify_value  = dma_parms.get_bool("clear_notify_value");
  m_reuse_tag           = dma_parms.get_bool("reuse_tag");
  m_allow_size_zero     = dma_parms.get_bool("allow_size_zero");
  m_start_at_index_1    = dma_parms.get_bool("start_at_index_1");
  m_turbo               = dma_parms.get_bool("turbo");
  m_big_endian          = false;
  m_overlap_read_write  = (m_max_reads > 0) && (m_max_writes > 0);

  if ((m_max_reads == 0) != (m_max_writes == 0)) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': \"max_reads\" (" << m_max_reads << ") and \"max_writes\" (" << m_max_writes
        << ") must both be zero or both be non-zero.";
    throw xtsc_exception(oss.str());
  }

  if (m_overlap_descriptors && (m_max_reads == 0)) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': \"max_reads\" and \"max_writes\" must be non-zero if \"overlap_descriptors\" is true.";
    throw xtsc_exception(oss.str());
  }

  m_port_types["m_request_port"]   = REQUEST_PORT;
  m_port_types["m_respond_export"] = RESPOND_EXPORT;
  m_port_types["master_port"]      = PORT_TABLE;

  xtsc_address reg_end_address8 = m_reg_base_address + 256 + 256 - 1;
  if ((m_reg_base_address < m_start_address8) || (reg_end_address8 > m_end_address8)) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': Minimum DMA register space (0x" << hex << setfill('0') << setw(8) << m_reg_base_address 
        << "-0x" << setw(8) << reg_end_address8 << ") is not completely contained with the xtsc_memory address range (0x"
        << setw(8) << m_start_address8 << "-0x" << m_end_address8 << ")";
    throw xtsc_exception(oss.str());
  }

  u32 clock_period = dma_parms.get_u32("clock_period");
  m_time_resolution = sc_get_time_resolution();
  u32 nacc_wait_time    = dma_parms.get_u32("nacc_wait_time");
  if (nacc_wait_time == 0xFFFFFFFF) {
    m_nacc_wait_time = m_clock_period;
  }
  else {
    m_nacc_wait_time = m_time_resolution * nacc_wait_time;
    if (m_nacc_wait_time > m_clock_period) {
      ostringstream oss;
      oss << kind() << " '" << name() << "': \"nacc_wait_time\"=" << m_nacc_wait_time << " exceeds clock period of " << m_clock_period;
      throw xtsc_exception(oss.str());
    }
  }

  m_clock_period_value = m_clock_period.value();
  u32 posedge_offset = dma_parms.get_u32("posedge_offset");
  if (posedge_offset == 0xFFFFFFFF) {
    m_posedge_offset = xtsc_get_system_clock_posedge_offset();
  }
  else {
    m_posedge_offset = posedge_offset * m_time_resolution;
  }
  if (m_posedge_offset >= m_clock_period) {
    ostringstream oss;
    oss << kind() << " '" << name() << "' parameter error:" << endl;
    oss << "\"posedge_offset\" (0x" << hex << posedge_offset << "=" << dec << posedge_offset << "=>" << m_posedge_offset
        << ") must be strictly less than \"clock_period\" (0x" << hex << clock_period << "=" << dec << clock_period << "=>"
        << m_clock_period << ")";
    throw xtsc_exception(oss.str());
  }
  m_has_posedge_offset = (m_posedge_offset != SC_ZERO_TIME);
  m_posedge_offset_plus_one = m_posedge_offset + m_clock_period;

  for (u32 i=0; i<16; ++i) {
    m_p_block_read_response[i] = 0;
  }

  SC_THREAD(dma_thread);
  m_process_handles.push_back(sc_get_current_process_handle());

  if (m_overlap_read_write) {
    SC_THREAD(request_thread);
    m_process_handles.push_back(sc_get_current_process_handle());

    SC_THREAD(write_thread);
    m_process_handles.push_back(sc_get_current_process_handle());
  }

  m_respond_export(m_respond_impl);

  xtsc_register_command(*this, *this, "dump_descriptor", 1, 1,
      "dump_descriptor <Index>", 
      "Call xtsc_dma_engine::dump_descriptor(<Index>)."
  );

  xtsc_register_command(*this, *this, "dump_descriptors", 0, 2,
      "dump_descriptors [<Start> [<Count>]]", 
      "Call xtsc_dma_engine::dump_descriptors(<Start>, <Count>)."
  );

  xtsc_event_register(m_dma_thread_event,                     "m_dma_thread_event",                     this);
  xtsc_event_register(m_request_thread_event,                 "m_request_thread_event",                 this);
  xtsc_event_register(m_write_thread_event,                   "m_write_thread_event",                   this);
  xtsc_event_register(m_got_read_response_event,              "m_got_read_response_event",              this);
  xtsc_event_register(m_no_reads_or_writes_event,             "m_no_reads_or_writes_event",             this);
  xtsc_event_register(m_single_response_available_event,      "m_single_response_available_event",      this);
  xtsc_event_register(m_block_read_response_available_event,  "m_block_read_response_available_event",  this);
  xtsc_event_register(m_block_write_response_available_event, "m_block_write_response_available_event", this);

  log4xtensa::LogLevel ll = xtsc_get_constructor_log_level();
  XTSC_LOG(m_text, ll, hex << " reg_base_address    = 0x" << setfill('0') << setw(8) << m_reg_base_address);
  XTSC_LOG(m_text, ll,        " read_priority       = "   << (u32) m_read_priority);
  XTSC_LOG(m_text, ll,        " write_priority      = "   << (u32) m_write_priority);
  XTSC_LOG(m_text, ll,        " overlap_descriptors = "   << boolalpha << m_overlap_descriptors);
  XTSC_LOG(m_text, ll,        " clear_notify_value  = "   << boolalpha << m_clear_notify_value);
  XTSC_LOG(m_text, ll,        " max_reads           = "   << m_max_reads);
  XTSC_LOG(m_text, ll,        " max_writes          = "   << m_max_writes);
  XTSC_LOG(m_text, ll,        " reuse_tag           = "   << boolalpha << m_reuse_tag);
  XTSC_LOG(m_text, ll,        " allow_size_zero     = "   << boolalpha << m_allow_size_zero);
  XTSC_LOG(m_text, ll,        " start_at_index_1    = "   << boolalpha << m_start_at_index_1);
  XTSC_LOG(m_text, ll,        " turbo               = "   << boolalpha << m_turbo);
  XTSC_LOG(m_text, ll,        " descriptor_delay    = "   << m_descriptor_delay);
  if (posedge_offset == 0xFFFFFFFF) {
  XTSC_LOG(m_text, ll,        " posedge_offset      = 0xFFFFFFFF => " << m_posedge_offset.value() << " (" << m_posedge_offset << ")");
  } else {
  XTSC_LOG(m_text, ll,        " posedge_offset      = "   << posedge_offset << " (" << m_posedge_offset << ")");
  }
  if (nacc_wait_time == 0xFFFFFFFF) {
  XTSC_LOG(m_text, ll,        " nacc_wait_time      = 0xFFFFFFFF => " << m_nacc_wait_time.value() << " (" << m_nacc_wait_time << ")");
  } else {
  XTSC_LOG(m_text, ll,        " nacc_wait_time      = "   << nacc_wait_time << " (" << m_nacc_wait_time << ")");
  }

  m_deny_fast_access.push_back(m_reg_base_address);
  m_deny_fast_access.push_back(m_reg_base_address+3);
  XTSC_INFO(m_text, "No fast access to xtsc_dma_request num_descriptors register: 0x" << hex << m_reg_base_address <<
                    "-0x" << m_reg_base_address+3);

  reset();
}



xtsc_component::xtsc_dma_engine::~xtsc_dma_engine(void) {
  XTSC_DEBUG(m_text, "In ~xtsc_dma_engine()");
}



u32 xtsc_component::xtsc_dma_engine::get_bit_width(const string& port_name, u32 interface_num) const {
  return m_width8*8;
}



sc_object *xtsc_component::xtsc_dma_engine::get_port(const string& port_name) {
       if (port_name == "m_request_port")   return &m_request_port;
  else if (port_name == "m_respond_export") return &m_respond_export;

  return xtsc_memory::get_port(port_name);
}



xtsc_port_table xtsc_component::xtsc_dma_engine::get_port_table(const string& port_table_name) const {
  if (port_table_name == "master_port") {
    xtsc_port_table table;
    table.push_back("m_request_port");
    table.push_back("m_respond_export");
    return table;
  }
  return xtsc_memory::get_port_table(port_table_name);
}



void xtsc_component::xtsc_dma_engine::check_for_go_byte(xtsc_address address8, u32 size8, const u8 *buffer) {
  u32 offset = m_reg_base_address - address8;
  if (offset < size8) {
    if (m_busy) {
      ostringstream oss;
      oss << kind() << " \"" << name() << "\": num_descriptors register written while xtsc_dma_engine is already busy.";
      throw xtsc_exception(oss.str());
    }
    if (((size8 > 0) && (*(buffer+offset+0) != 0)) ||
        ((size8 > 1) && (*(buffer+offset+1) != 0)) ||
        ((size8 > 2) && (*(buffer+offset+2) != 0)) ||
        ((size8 > 3) && (*(buffer+offset+3) != 0)))
    {
      XTSC_DEBUG(m_text, "num_descriptors register written.  Notifying dma_thread.");
      m_dma_thread_event.notify(SC_ZERO_TIME);
    }
  }
}



void xtsc_component::xtsc_dma_engine::dump_descriptor(u32 idx, ostream& os) {

  if ((idx < 1) || (idx > 255)) {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\": dump_descriptor called with idx=" << idx << " which is outside the legal range of [1-255]";
    throw xtsc_exception(oss.str());
  }

  // Save state of stream
  char c = os.fill('0');
  ios::fmtflags old_flags = os.flags();
  streamsize w = os.width();

  xtsc_dma_descriptor dsc;

  dsc.source_address8             = read_u32(m_reg_base_address+(0x100*idx)+0x00, m_big_endian);
  dsc.destination_address8        = read_u32(m_reg_base_address+(0x100*idx)+0x04, m_big_endian);
  dsc.size8                       = read_u32(m_reg_base_address+(0x100*idx)+0x08, m_big_endian);
  dsc.num_transfers               = read_u32(m_reg_base_address+(0x100*idx)+0x0C, m_big_endian);

  os << "Descriptor #" << left << dec << setfill(' ') << setw(3) << idx << " 0x" << setfill('0') << right << hex << setw(8)
     << (m_reg_base_address+(0x100*idx)+0x00) << ":";
  if (dsc.size8 != 0) {
    os << " 0x" << setw(8) << dsc.source_address8
       << " 0x" << setw(8) << dsc.destination_address8
       << " 0x" << setw(4) << dsc.size8 << dec << setfill(' ') 
       << " "   << setw(2) << dsc.num_transfers;
  }
  os << endl;

  // Restore state of stream
  os.fill(c);
  os.flags(old_flags);
  os.width(w);

}



void xtsc_component::xtsc_dma_engine::dump_descriptors(u32 start_idx, u32 count, ostream& os) {
  if (count > 255) {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\": dump_descriptors called with count=" << count << " which exceeds the maximum of 255.";
    throw xtsc_exception(oss.str());
  }
  u32 idx = start_idx;
  for (u32 i = 0; i<count; ++i, ++idx) {
    if (idx == 256) { idx = 1; }
    dump_descriptor(idx, os);
  }
}



void xtsc_component::xtsc_dma_engine::execute(const string&             cmd_line, 
                                              const vector<string>&     words,
                                              const vector<string>&     words_lc,
                                              ostream&                  result)
{
  ostringstream res;

  if (false) {
  }
  else if (words[0] == "dump_descriptor") {
    u32 idx = xtsc_command_argtou32(cmd_line, words, 1);
    dump_descriptor(idx, res);
  }
  else if (words[0] == "dump_descriptors") {
    u32 start_idx = ((words.size() > 1) ? xtsc_command_argtou32(cmd_line, words, 1) :   1);
    u32 end_idx   = ((words.size() > 2) ? xtsc_command_argtou32(cmd_line, words, 2) : 255);
    dump_descriptors(start_idx, end_idx, res);
  }
  else {
    return xtsc_memory::execute(cmd_line, words, words_lc, result);
  }

  result << res.str();
}



void xtsc_component::xtsc_dma_engine::do_write(u32 port_num) {
  xtsc_memory::do_write(port_num);
  xtsc_request       *p_request     = &m_p_active_request_info[port_num]->m_request;
  xtsc_address        address8      = p_request->get_byte_address();
  u32                 size8         = p_request->get_byte_size();
  const u8           *buffer        = p_request->get_buffer();
  check_for_go_byte(address8, size8, buffer);
}



void xtsc_component::xtsc_dma_engine::do_block_write(u32 port_num) {
  xtsc_memory::do_block_write(port_num);
  xtsc_request *p_request = &m_p_active_request_info[port_num]->m_request;
  xtsc_address  address8  = p_request->get_byte_address();
  const u8     *buffer    = p_request->get_buffer();
  check_for_go_byte(address8, m_width8, buffer);
}



void xtsc_component::xtsc_dma_engine::sync_to_posedge(bool always_wait) {
  sc_time now = sc_time_stamp();
  sc_time phase_now = (now.value() % m_clock_period_value) * m_time_resolution;
  if (m_has_posedge_offset) {
    if (phase_now < m_posedge_offset) {
      phase_now += m_clock_period;
    }
    phase_now -= m_posedge_offset;
  }
  if (phase_now < m_posedge_offset) {
    wait(m_posedge_offset - phase_now);
  }
  else if (phase_now > m_posedge_offset) {
    wait(m_posedge_offset_plus_one - phase_now);
  }
  else if (always_wait) {
    wait(m_clock_period);
  }
}



void xtsc_component::xtsc_dma_engine::reset(bool hard_reset) {
  m_num_block_transfers       = 0;
  m_block_read_response_count = 0;
  m_num_reads                 = 0;
  m_num_writes                = 0;

  m_waiting_for_nacc          = false;
  m_request_got_nacc          = false;

  if (m_p_block_write_response) {
    delete m_p_block_write_response;
    m_p_block_write_response = 0;
  }

  if (m_p_single_response) {
    delete m_p_single_response;
    m_p_single_response = 0;
  }

  while (!m_request_deque.empty()) {
    xtsc_request *p_request = m_request_deque.front();
    m_request_deque.pop_front();
    delete_request(p_request);
  }

  while (!m_blank_write_deque.empty()) {
    xtsc_request *p_request = m_blank_write_deque.front();
    m_blank_write_deque.pop_front();
    delete_request(p_request);
  }

  while (!m_ready_write_deque.empty()) {
    xtsc_request *p_request = m_ready_write_deque.front();
    m_ready_write_deque.pop_front();
    delete_request(p_request);
  }

  while (!m_last_write_deque.empty()) {
    m_last_write_deque.pop_front();
  }

  while (!m_done_descriptor_deque.empty()) {
    m_done_descriptor_deque.pop_front();
  }

  // Cancel any event notifications
  m_dma_thread_event                    .cancel();
  m_request_thread_event                .cancel();
  m_write_thread_event                  .cancel();
  m_got_read_response_event             .cancel();
  m_no_reads_or_writes_event            .cancel();
  m_single_response_available_event     .cancel();
  m_block_read_response_available_event .cancel();
  m_block_write_response_available_event.cancel();

  xtsc_memory::reset(hard_reset);
}



void xtsc_component::xtsc_dma_engine::dma_thread() {

  try {

    xtsc_dma_request req;

    while (true) {

      m_busy = false;
      wait(m_dma_thread_event);
      if (!read_u32(m_reg_base_address+0x00, false /* don't care */)) continue;
      m_busy = true;
      m_big_endian = false;
      req.num_descriptors         = read_u32(m_reg_base_address+0x00, false);
      if (req.num_descriptors > 255) {
        req.num_descriptors       = read_u32(m_reg_base_address+0x00, true);
        if (req.num_descriptors > 255) {
          ostringstream oss;
          oss << kind() << " \"" << name() << "\": num_descriptors cannot exceed 255"
              << " (DMA request address: 0x" << hex << setfill('0') << setw(8) << (m_reg_base_address+0x00) << ")";
          throw xtsc_exception(oss.str());
        }
        m_big_endian = true;
      }
      req.notify_address8       = read_u32(m_reg_base_address+0x04, m_big_endian);
      req.notify_value          = read_u32(m_reg_base_address+0x08, m_big_endian);
      req.turboxim_event_id     = read_u32(m_reg_base_address+0x0C, m_big_endian);
      req.done_descriptor       = read_u32(m_reg_base_address+0x10, m_big_endian);


      if (req.notify_address8 & (m_width8-1)) {
        ostringstream oss;
        oss << kind() << " \"" << name() << "\": notify_address8=0x" << hex << req.notify_address8
            << " is not aligned to PIF width=" << dec << m_width8
            << " (DMA request address: 0x" << hex << setfill('0') << setw(8) << (m_reg_base_address+0x04) << ")";
        throw xtsc_exception(oss.str());
      }

      {
        ostringstream oss;
        oss << "DMA request:"
            << " num_descriptors="   << req.num_descriptors << hex
            << " notify_address8=0x" << req.notify_address8
            << " notify_value=0x"    << req.notify_value;
        if (!m_start_at_index_1) {
          oss << " done_descriptor=" << dec << req.done_descriptor;
        }
        XTSC_INFO(m_text, oss.str());
      }

      xtsc_address reg_end_address8 = m_reg_base_address + 256 + (256 * req.num_descriptors) - 1;
      if ((m_reg_base_address < m_start_address8) || (reg_end_address8 > m_end_address8)) {
        ostringstream oss;
        oss << kind() << " '" << name() << "': Required DMA register space (0x" << hex << setfill('0') << setw(8) << m_reg_base_address 
            << "-0x" << setw(8) << reg_end_address8 << ") is not completely contained with the xtsc_memory address range (0x"
            << setw(8) << m_start_address8 << "-0x" << m_end_address8 << ")";
        throw xtsc_exception(oss.str());
      }

      u32 first_descriptor = 1;
      if (!m_start_at_index_1) {
        first_descriptor = req.done_descriptor;
        if (first_descriptor > 255) {
          ostringstream oss;
          oss << kind() << " '" << name() << "': The done_descriptor value (" << first_descriptor << ") at address 0x" << hex << setfill('0')
              << setw(8) << (m_reg_base_address+0x10) << " is outside the legal range of [0,255]";
          throw xtsc_exception(oss.str());
        }
        first_descriptor += 1;
        if (first_descriptor == 256) {
          first_descriptor = 1;
        }
      }

      for (u32 i=0; i<req.num_descriptors; ++i) {

        u32 idx = first_descriptor + i;
        if (idx > 255) { idx = idx - 255; }

        ostringstream des;
        des << "#" << (i+1) << "/" << req.num_descriptors;
        if (!m_start_at_index_1) { des << "@" << idx; }

        xtsc_dma_descriptor dsc;

        dsc.source_address8             = read_u32(m_reg_base_address+(0x100*idx)+0x00, m_big_endian);
        dsc.destination_address8        = read_u32(m_reg_base_address+(0x100*idx)+0x04, m_big_endian);
        dsc.size8                       = read_u32(m_reg_base_address+(0x100*idx)+0x08, m_big_endian);
        dsc.num_transfers               = read_u32(m_reg_base_address+(0x100*idx)+0x0C, m_big_endian);

        // Sanity checks
        if ((dsc.num_transfers != 1) &&
            (dsc.num_transfers != 2) &&
            (dsc.num_transfers != 4) &&
            (dsc.num_transfers != 8) &&
            (dsc.num_transfers != 16))
        {
          ostringstream oss;
          oss << kind() << " \"" << name() << "\": num_transfers=" << dsc.num_transfers << " is invalid (must be 1|2|4|8|16)"
              << " (descriptor address: 0x" << hex << setfill('0') << setw(8) << (m_reg_base_address+(0x100*idx+0x0C)) << ")";
          throw xtsc_exception(oss.str());
        }
        if (dsc.size8 & (m_width8*dsc.num_transfers-1)) {
          ostringstream oss;
          oss << kind() << " \"" << name() << "\": size8=0x" << hex << dsc.size8
              << " is not aligned to the block transfer size=0x" << (m_width8*dsc.num_transfers)
              << " (descriptor address: 0x" << hex << setfill('0') << setw(8) << (m_reg_base_address+(0x100*idx+0x08)) << ")";
          throw xtsc_exception(oss.str());
        }
        if (dsc.source_address8 & (m_width8*dsc.num_transfers-1)) {
          ostringstream oss;
          oss << kind() << " \"" << name() << "\": source_address8=0x" << hex << dsc.source_address8
              << " is not aligned to the block transfer size=0x" << (m_width8*dsc.num_transfers)
              << " (descriptor address: 0x" << hex << setfill('0') << setw(8) << (m_reg_base_address+(0x100*idx+0x08)) << ")";
          throw xtsc_exception(oss.str());
        }
        if (dsc.destination_address8 & (m_width8*dsc.num_transfers-1)) {
          ostringstream oss;
          oss << kind() << " \"" << name() << "\": destination_address8=0x" << hex << dsc.destination_address8
              << " is not aligned to the block transfer size=0x" << (m_width8*dsc.num_transfers)
              << " (descriptor address: 0x" << hex << setfill('0') << setw(8) << (m_reg_base_address+(0x100*idx+0x08)) << ")";
          throw xtsc_exception(oss.str());
        }
        if (!dsc.size8 && !m_allow_size_zero) {
          ostringstream oss;
          oss << kind() << " \"" << name() << "\": Descriptor size8 register is 0 but \"allow_size_zero\" parameter is false";
          throw xtsc_exception(oss.str());
        }

        if (!m_turbo && m_descriptor_delay) {
          wait(m_descriptor_delay * m_clock_period);
        }

        XTSC_INFO(m_text, "Descriptor " << des.str() << ":" << hex
            << " source_address8=0x"            << dsc.source_address8
            << " destination_address8=0x"       << dsc.destination_address8
            << " size8=0x"                      << dsc.size8
            << " num_transfers=0x"              << dsc.num_transfers
        );

        if (m_turbo) {
          use_turbo(dsc);
        }
        else if (m_overlap_read_write) {
          do_overlapped_requests(dsc, idx);
          if (((i+1)==req.num_descriptors) || !m_overlap_descriptors) {
            bool first = true;
            while (m_num_reads || m_num_writes || m_request_deque.size() || m_ready_write_deque.size() || m_blank_write_deque.size()) {
              if (first) {
                XTSC_INFO(m_text, "Waiting for Descriptor " << des.str() << " to complete");
                first = false;
              }
              wait(m_no_reads_or_writes_event);
            }
            if (!first) {
              XTSC_INFO(m_text, "Descriptor " << des.str() << " completed.");
            }
          }
        }
        else if (dsc.num_transfers == 1) {
          use_single_transfers(dsc);
          write_u32(m_reg_base_address+0x10, idx, m_big_endian);
        }
        else {
          use_block_transfers(dsc);
          write_u32(m_reg_base_address+0x10, idx, m_big_endian);
        }
      }

      xtsc_response::status_t status = remote_write_u32(req.notify_address8, req.notify_value);
      if (status != xtsc_response::RSP_OK) {
        ostringstream oss;
        oss << kind() << " \"" << name() << "\": unable to write notify address=0x" << hex << req.notify_address8
            << ".  status=" << xtsc_response::get_status_name(status);
        throw xtsc_exception(oss.str());
      }
      if (m_clear_notify_value) {
        remote_write_u32(req.notify_address8, 0);
      }

      if (req.turboxim_event_id) {
        xtsc_fire_turboxim_event_id(req.turboxim_event_id);
      }

      m_busy = false;
      XTSC_INFO(m_text, "DMA done");

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



void xtsc_component::xtsc_dma_engine::request_thread() {
  try {
    while (true) {
      wait(m_request_thread_event);
      sync_to_posedge(false);
      while (!m_request_deque.empty()) {
        xtsc_request *p_request = m_request_deque.front();
        m_request_deque.pop_front();
        u32 tries = 0;
        do {
          tries += 1;
          XTSC_INFO(m_text, *p_request << " Try #" << tries);
          m_request_got_nacc = false;
          m_waiting_for_nacc = true;
          m_request_port->nb_request(*p_request);
          wait(m_nacc_wait_time);
        } while (m_request_got_nacc);
        m_waiting_for_nacc = false;
        delete_request(p_request);
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



void xtsc_component::xtsc_dma_engine::write_thread() {
  try {
    while (true) {
      XTSC_DEBUG(m_text, "Waiting for m_write_thread_event");
      wait(m_write_thread_event);
      XTSC_DEBUG(m_text, "Got m_write_thread_event");
      while (!m_last_write_deque.empty() && (m_num_writes < m_max_writes)) {
        xtsc_request *p_request = m_ready_write_deque.front();
        m_ready_write_deque.pop_front();
        m_request_deque.push_back(p_request);
        m_request_thread_event.notify(SC_ZERO_TIME);
        if (p_request->get_last_transfer()) {
          m_last_write_deque.pop_front();
          m_num_writes += 1;
          XTSC_DEBUG(m_text, "Inc m_num_writes to " << m_num_writes);
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



// Use nb_peek/nb_poke
void xtsc_component::xtsc_dma_engine::use_turbo(xtsc_dma_descriptor &dsc) {
  u32 chunk_size = dsc.num_transfers * m_width8;
  while (dsc.size8) {
    m_request_port->nb_peek(dsc.source_address8, chunk_size, m_data);
    m_request_port->nb_poke(dsc.destination_address8, chunk_size, m_data);
    dsc.source_address8 += chunk_size;
    dsc.destination_address8 += chunk_size;
    dsc.size8 -= chunk_size;
  }
}



void xtsc_component::xtsc_dma_engine::do_overlapped_requests(xtsc_dma_descriptor &dsc, u32 idx) {
  xtsc_request::type_t type = xtsc_request::READ;
  xtsc_request *p_request = NULL;
  u32 bytes = m_width8 * dsc.num_transfers;
  while (dsc.size8) {
    while (m_num_reads >= m_max_reads) {
      XTSC_DEBUG(m_text, "Waiting for m_got_read_response_event");
      wait(m_got_read_response_event);
      XTSC_DEBUG(m_text, "Got m_got_read_response_event");
    }
    m_num_reads += 1;
    XTSC_DEBUG(m_text, "Inc m_num_reads to " << m_num_reads);
    wait(m_clock_period);
    // Form the read request and put it in m_request_deque for request_thread to send downstream
    type = (dsc.num_transfers == 1) ? xtsc_request::READ : xtsc_request::BLOCK_READ;
    p_request = new_request();
    p_request->initialize(type,                         // type
                          dsc.source_address8,          // address
                          m_width8,                     // size
                          0,                            // tag  (0 => XTSC assigns tag)
                          dsc.num_transfers,            // num_transfers
                          0xFFFF,                       // byte_enables (ignored)
                          true,                         // last_transfer
                          0,                            // route_id
                          m_overlapped_read_id,         // id
                          m_read_priority               // priority
                          );
    m_request_deque.push_back(p_request);
    bool last_req = (dsc.size8 == bytes);
    m_done_descriptor_deque.push_back(last_req ? idx : 0);
    m_request_thread_event.notify(SC_ZERO_TIME);
    u64 tag = (m_reuse_tag ? p_request->get_tag() : 0);
    // Now pre-form write requests without data and put them in m_blank_write_deque to await read responses
    for (u32 i=1; i <= dsc.num_transfers; ++i) {
      bool last = (i == dsc.num_transfers);
      p_request = new_request();
      if (i == 1) {
        type = (dsc.num_transfers == 1) ? xtsc_request::WRITE : xtsc_request::BLOCK_WRITE;
        p_request->initialize(type,                     // type
                              dsc.destination_address8, // address
                              m_width8,                 // size
                              tag,                      // tag  (0 => XTSC assigns tag)
                              dsc.num_transfers,        // num_transfers
                              0xFFFFFFFFull,            // byte_enables 
                              last,                     // last_transfer
                              0,                        // route_id
                              m_overlapped_write_id,    // id
                              m_write_priority          // priority
                              );
        tag = p_request->get_tag();
      }
      else {
        p_request->initialize(tag,                      // tag
                              dsc.destination_address8, // address
                              m_width8,                 // size
                              dsc.num_transfers,        // num_transfers
                              last,                     // last_transfer
                              0,                        // route_id
                              m_overlapped_write_id,    // id
                              m_write_priority          // priority
                              );
      }
      m_blank_write_deque.push_back(p_request);
      dsc.destination_address8 += m_width8;
    }
    dsc.source_address8 += bytes;
    dsc.size8 -= bytes;
  }
}



// Use non-overlapped, non-block READ/WRITE
void xtsc_component::xtsc_dma_engine::use_single_transfers(xtsc_dma_descriptor &dsc) {
  while (dsc.size8) {
    xtsc_response::status_t status = remote_read(dsc.source_address8, m_width8, 0xFFFF, m_data);
    if (status != xtsc_response::RSP_OK) {
      ostringstream oss;
      oss << kind() << " \"" << name() << "\": unable to read source address=0x" << hex << dsc.source_address8
          << ".  status=" << xtsc_response::get_status_name(status);
      throw xtsc_exception(oss.str());
    }
    wait(m_clock_period);
    u64 tag = (m_reuse_tag ? m_request.get_tag() : 0);
    status = remote_write(dsc.destination_address8, m_width8, 0xFFFF, m_data, tag);
    if (status != xtsc_response::RSP_OK) {
      ostringstream oss;
      oss << kind() << " \"" << name() << "\": unable to write destination address=0x" << hex << dsc.destination_address8
          << ".  status=" << xtsc_response::get_status_name(status);
      throw xtsc_exception(oss.str());
    }
    dsc.source_address8 += m_width8;
    dsc.destination_address8 += m_width8;
    dsc.size8 -= m_width8;
  }
}



// Use non-overlapped BLOCK_READ/BLOCK_WRITE
void xtsc_component::xtsc_dma_engine::use_block_transfers(xtsc_dma_descriptor &dsc) {
  m_num_block_transfers = dsc.num_transfers;
  xtsc_response::status_t status;
  while (dsc.size8) {
    sync_to_posedge(true);
    m_request.initialize(xtsc_request::BLOCK_READ,      // type
                         dsc.source_address8,           // address
                         m_width8,                      // size
                         0,                             // tag  (0 => XTSC assigns tag)
                         dsc.num_transfers,             // num_transfers
                         0xFFFF,                        // byte_enables (ignored)
                         true,                          // last_transfer
                         0,                             // route_id
                         m_block_read_id,               // id
                         m_read_priority                // priority
                         );

    u32 tries = 0;
    do {
      if (m_p_block_read_response[0]) {
        delete m_p_block_read_response[0];
        m_p_block_read_response[0] = 0;
      }
      tries += 1;
      XTSC_INFO(m_text, m_request << " Try #" << tries);
      m_block_read_response_count = 0;
      m_request_port->nb_request(m_request);
      wait(m_clock_period);
    } while (m_p_block_read_response[0] && (m_p_block_read_response[0]->get_status() == xtsc_response::RSP_NACC));

    m_block_write_sent_count = 0;
    u64 tag = (m_reuse_tag ? m_request.get_tag() : 0);
    while (m_block_write_sent_count < m_num_block_transfers) {
      while (m_block_write_sent_count >= m_block_read_response_count) {
        wait(m_block_read_response_available_event);
        XTSC_DEBUG(m_text, "use_block_transfers 0x" << hex << dsc.source_address8 << ": tag=" << dec << m_request.get_tag() <<
                           " got m_block_read_response_available_event");
      }
      sc_time req_net = m_p_block_read_response_time[m_block_write_sent_count] + m_clock_period;
      while (sc_time_stamp() < req_net) {
        sync_to_posedge(true);
      }

      status = m_p_block_read_response[m_block_write_sent_count]->get_status();
      if (status != xtsc_response::RSP_OK) {
        ostringstream oss;
        oss << kind() << " \"" << name() << "\": unable to block read source address=0x" << hex << dsc.source_address8
            << ".  status=" << xtsc_response::get_status_name(status);
        throw xtsc_exception(oss.str());
      }

      if (m_block_write_sent_count == 0) {
        m_request.initialize(xtsc_request::BLOCK_WRITE,     // type
                             dsc.destination_address8,      // address
                             m_width8,                      // size
                             tag,                           // tag  (0 => XTSC assigns tag)
                             dsc.num_transfers,             // num_transfers
                             0xFFFF,                        // byte_enables (ignored)
                             false,                         // last_transfer
                             0,                             // route_id
                             m_block_write_id,              // id
                             m_write_priority               // priority
                             );
        tag = m_request.get_tag();
      }
      else {
        bool last = ((m_block_write_sent_count + 1) == m_num_block_transfers);
        m_request.initialize(tag,                           // tag
                             dsc.destination_address8,      // address
                             m_width8,                      // size
                             dsc.num_transfers,             // num_transfers
                             last,                          // last_transfer
                             0,                             // route_id
                             m_block_write_id,              // id
                             m_write_priority               // priority
                             );
      }

      memcpy(m_request.get_buffer(), m_p_block_read_response[m_block_write_sent_count]->get_buffer(), m_width8);
      u32 tries = 0;
      do {
        if (m_p_block_write_response) {
          delete m_p_block_write_response;
          m_p_block_write_response = 0;
        }
        tries += 1;
        XTSC_INFO(m_text, m_request << "Try #" << tries);
        m_request_port->nb_request(m_request);
        wait(m_clock_period);
      } while (m_p_block_write_response && (m_p_block_write_response->get_status() == xtsc_response::RSP_NACC));

      m_block_write_sent_count += 1;
      dsc.destination_address8 += m_width8;

    }

    if (!m_p_block_write_response) {
      wait(m_block_write_response_available_event);
    }

    status = m_p_block_write_response->get_status();
    if (status != xtsc_response::RSP_OK) {
      ostringstream oss;
      oss << kind() << " \"" << name() << "\": unable to block write destination address=0x" << hex << dsc.destination_address8
          << ".  status=" << xtsc_response::get_status_name(status);
      throw xtsc_exception(oss.str());
    }


    u32 bytes_in_block = m_width8 * m_num_block_transfers;
    dsc.source_address8 += bytes_in_block;
    dsc.size8 -= bytes_in_block;

  }
  m_num_block_transfers = 0;
}



xtsc_response::status_t xtsc_component::xtsc_dma_engine::remote_read(xtsc_address       address8,
                                                                     u32                size8,
                                                                     xtsc_byte_enables  byte_enables,
                                                                     u8                *buffer)
{
  m_request.initialize(xtsc_request::READ,      // type
                       address8,                // address8
                       size8,                   // size8
                       0,                       // tag  (0 => XTSC assigns)
                       1,                       // num_transfers
                       byte_enables,            // byte_enables
                       true,                    // last_transfer
                       0,                       // route_id
                       m_read_id,               // id
                       m_read_priority          // priority
                       );
  u32 tries = 0;
  do {
    tries += 1;
    XTSC_INFO(m_text, m_request << "Try #" << tries);
    m_request_port->nb_request(m_request);
    wait(m_single_response_available_event);
    XTSC_DEBUG(m_text, "remote_read 0x" << hex << address8 << ": tag=" << dec << m_request.get_tag() <<
                       " got m_single_response_available_event");
  } while (m_p_single_response->get_status() == xtsc_response::RSP_NACC);

  if (m_p_single_response->get_status() == xtsc_response::RSP_OK) {
    memcpy(buffer, m_p_single_response->get_buffer(), size8);
  }

  return m_p_single_response->get_status();
}



xtsc_response::status_t xtsc_component::xtsc_dma_engine::remote_write_u32(xtsc_address address8, u32 data) {
  u8 buffer[4];
  u8 *p_data = reinterpret_cast<u8*>(&data);
  xtsc_byte_enables byte_enables = 0x000F;
  if (m_big_endian) {
    for (int i=0; i<4; i++) {
      buffer[i] = p_data[3-i];
    }
    p_data = buffer;
  }
  return remote_write(address8, 4, byte_enables, p_data);
}



xtsc_response::status_t xtsc_component::xtsc_dma_engine::remote_write(xtsc_address      address8,
                                                                      u32               size8,
                                                                      xtsc_byte_enables byte_enables,
                                                                      u8               *buffer,
                                                                      u64               tag)
{
  m_request.initialize(xtsc_request::WRITE,     // type
                       address8,                // address8
                       size8,                   // size8
                       tag,                     // tag (0 => XTSC assigns)
                       1,                       // num_transfers
                       byte_enables,            // byte_enables
                       true,                    // last_transfer
                       0,                       // route_id
                       m_write_id,              // id
                       m_write_priority         // priority
                       );
  memcpy(m_request.get_buffer(), buffer, size8);
  u32 tries = 0;
  do {
    tries += 1;
    XTSC_INFO(m_text, m_request << "Try #" << tries);
    m_request_port->nb_request(m_request);
    wait(m_single_response_available_event);
    XTSC_DEBUG(m_text, "remote_write 0x" << hex << address8 << ": tag=" << dec << m_request.get_tag() << 
                       " got m_single_response_available_event");
  } while (m_p_single_response->get_status() == xtsc_response::RSP_NACC);

  return m_p_single_response->get_status();
}



void xtsc_component::xtsc_dma_engine::compute_delays() {
  xtsc_memory::compute_delays();

  u32 posedge_offset = m_memory_parms.get_u32("posedge_offset");
  if (m_posedge_offset >= m_clock_period) {
    ostringstream oss;
    oss << kind() << " '" << name() << "' parameter error:" << endl;
    oss << "\"posedge_offset\" (0x" << hex << posedge_offset << "=>" << m_posedge_offset
        << ") must be strictly less than clock period => " << m_clock_period << ")";
    throw xtsc_exception(oss.str());
  }

  m_clock_period_value      = m_clock_period.value();
  m_posedge_offset_plus_one = m_posedge_offset + m_clock_period;
}



xtsc_request *xtsc_component::xtsc_dma_engine::new_request() {
  if (m_request_pool.empty()) {
    xtsc_request *p_request = new xtsc_request();
    XTSC_DEBUG(m_text, "Creating a new xtsc_request " << p_request);
    return p_request;
  }
  else {
    xtsc_request *p_request = m_request_pool.back();
    XTSC_DEBUG(m_text, "Reusing xtsc_request " << p_request);
    m_request_pool.pop_back();
    return p_request;
  }
}



void xtsc_component::xtsc_dma_engine::delete_request(xtsc_request*& p_request) {
  m_request_pool.push_back(p_request);
  p_request = 0;
}



bool xtsc_component::xtsc_dma_engine::xtsc_respond_if_impl::nb_respond(const xtsc_response& response) {
  XTSC_INFO(m_dma.m_text, response);
  u8 rsp_id = response.get_id();
  if ((rsp_id == m_dma.m_overlapped_read_id) || (rsp_id == m_dma.m_overlapped_write_id)) {
    xtsc_response::status_t status = response.get_status();
    if (status == xtsc_response::RSP_NACC) {
      if (m_dma.m_waiting_for_nacc) {
        m_dma.m_request_got_nacc = true;
      }
      else {
        ostringstream oss;
        oss << m_dma.kind() << " '" << m_dma.name() << "' received nacc too late: " << response << endl;
        oss << " - Possibly something is wrong with the downstream device" << endl;
        oss << " - Possibly this " << m_dma.kind() << "'s \"nacc_wait_time\" needs to be adjusted";
        throw xtsc_exception(oss.str());
      }
    }
    else if (status == xtsc_response::RSP_OK) {
      if (rsp_id == m_dma.m_overlapped_read_id) {
        if (response.get_byte_size() == 0) {
          ostringstream oss;
          oss << m_dma.kind() << " '" << m_dma.name() << "' received a write response with a read ID: " << response << endl;
          throw xtsc_exception(oss.str());
        }
        if (m_dma.m_blank_write_deque.empty()) {
          ostringstream oss;
          oss << m_dma.kind() << " '" << m_dma.name() << "' received unexpected response: " << response << endl;
          throw xtsc_exception(oss.str());
        }
        xtsc_request *p_request = m_dma.m_blank_write_deque.front();
        m_dma.m_blank_write_deque.pop_front();
        bool last = p_request->get_last_transfer();
        if (response.get_last_transfer() != last) {
          ostringstream oss;
          oss << m_dma.kind() << " '" << m_dma.name() << "' received " << (last ? "non-" : "") << "last_transfer response when "
              << (last ? "" : "non-") << "last_transfer read response was expected: " << response << endl;
          throw xtsc_exception(oss.str());
        }
        if (m_dma.m_reuse_tag) {
          if (response.get_tag() != p_request->get_tag()) {
            if (getenv("XTSC_DMA_ENGINE_DO_NOT_CHECK_TAG") == NULL) {
              ostringstream oss;
              oss << m_dma.kind() << " '" << m_dma.name() << "' was expecting tag=" << p_request->get_tag() << " but received: " << response;
              throw xtsc_exception(oss.str());
            }
          }
        }
        if (last) {
          if (m_dma.m_num_reads == m_dma.m_max_reads) {
            m_dma.m_got_read_response_event.notify(SC_ZERO_TIME);
          }
          m_dma.m_num_reads -= 1;
          XTSC_DEBUG(m_dma.m_text, "Dec m_num_reads to " << m_dma.m_num_reads);
          if ((m_dma.m_num_reads == 0) && (m_dma.m_num_writes == 0)) {
            m_dma.m_no_reads_or_writes_event.notify(SC_ZERO_TIME);
          }
          m_dma.m_last_write_deque.push_back(0);
          m_dma.m_write_thread_event.notify(SC_ZERO_TIME);
        }
        memcpy(p_request->get_buffer(), response.get_buffer(), response.get_byte_size());
        m_dma.m_ready_write_deque.push_back(p_request);
      }
      else {
        if (m_dma.m_done_descriptor_deque.empty()) {
          ostringstream oss;
          oss << "Program Bug: " << m_dma.kind() << " '" << m_dma.name() << "' m_done_descriptor_deque is un-expectedly empty.";
          throw xtsc_exception(oss.str());
        }
        u32 idx = m_dma.m_done_descriptor_deque.front();
        m_dma.m_done_descriptor_deque.pop_front();
        if (idx != 0) {
          m_dma.write_u32(m_dma.m_reg_base_address+0x10, idx, m_dma.m_big_endian);
          XTSC_INFO(m_dma.m_text, "Descriptor @" << idx << " completed");
        }
        m_dma.m_num_writes -= 1;
        XTSC_DEBUG(m_dma.m_text, "Dec m_num_writes to " << m_dma.m_num_writes);
        m_dma.m_write_thread_event.notify(SC_ZERO_TIME);
        if ((m_dma.m_num_reads == 0) && (m_dma.m_num_writes == 0)) {
          m_dma.m_no_reads_or_writes_event.notify(SC_ZERO_TIME);
        }
      }
    }
    else {
      ostringstream oss;
      oss << m_dma.kind() << " \"" << m_dma.name() << "\": received unsupported response: " << response;
      throw xtsc_exception(oss.str());
    }
  }
  else if ((rsp_id == m_dma.m_read_id) || (rsp_id == m_dma.m_write_id)) {
    if (m_dma.m_p_single_response) {
      delete m_dma.m_p_single_response;
      m_dma.m_p_single_response = 0;
    }
    m_dma.m_p_single_response = new xtsc_response(response);
    XTSC_DEBUG(m_dma.m_text, "nb_respond() called for tag=" << response.get_tag() <<
                             " notifying m_dma.m_single_response_available_event");
    m_dma.m_single_response_available_event.notify(SC_ZERO_TIME);
  }
  else if (rsp_id == m_dma.m_block_read_id) {
    if (m_dma.m_block_read_response_count >= m_dma.m_num_block_transfers) {
      ostringstream oss;
      oss << m_dma.kind() << " '" << m_dma.name() << "' nb_respond(): Received " << (m_dma.m_block_read_response_count+1)
          << " BLOCK_READ responses.  " << m_dma.m_num_block_transfers << " were expected.";
      throw xtsc_exception(oss.str());
    }
    if (m_dma.m_p_block_read_response[m_dma.m_block_read_response_count]) {
      delete m_dma.m_p_block_read_response[m_dma.m_block_read_response_count];
      m_dma.m_p_block_read_response[m_dma.m_block_read_response_count] = 0;
    }
    m_dma.m_p_block_read_response[m_dma.m_block_read_response_count] = new xtsc_response(response);
    m_dma.m_p_block_read_response_time[m_dma.m_block_read_response_count] = sc_time_stamp();
    XTSC_DEBUG(m_dma.m_text, "nb_respond() called for tag=" << response.get_tag() <<
                             " notifying m_dma.m_block_read_response_available_event");
    m_dma.m_block_read_response_available_event.notify(SC_ZERO_TIME);
    m_dma.m_block_read_response_count += 1;
  }
  else if (rsp_id == m_dma.m_block_write_id) {
    if (m_dma.m_p_block_write_response) {
      delete m_dma.m_p_block_write_response;
      m_dma.m_p_block_write_response = 0;
    }
    m_dma.m_p_block_write_response = new xtsc_response(response);
    XTSC_DEBUG(m_dma.m_text, "nb_respond() called for tag=" << response.get_tag() <<
                             " notifying m_dma.m_block_write_response_available_event");
    m_dma.m_block_write_response_available_event.notify(SC_ZERO_TIME);
  }
  else {
    ostringstream oss;
    oss << m_dma.kind() << " '" << m_dma.name() << "' nb_respond(): Got response with unsupported id=" << (u32) rsp_id;
    throw xtsc_exception(oss.str());
  }
  return true;
}



void xtsc_component::xtsc_dma_engine::xtsc_respond_if_impl::register_port(sc_port_base& port, const char *if_typename) {
  if (m_p_port) {
    ostringstream oss;
    oss << "Illegal multiple binding detected to xtsc_dma_engine '" << m_dma.name() << "' m_respond_export: " << endl;
    oss << "  " << port.name() << endl;
    oss << "  " << m_p_port->name();
    throw xtsc_exception(oss.str());
  }
  XTSC_INFO(m_dma.m_text, "Binding '" << port.name() << "' to xtsc_dma_engine::m_respond_export");
  m_p_port = &port;
}



