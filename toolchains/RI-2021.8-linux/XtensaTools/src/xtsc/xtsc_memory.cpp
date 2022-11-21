// Copyright (c) 2005-2018 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Cadence Design Systems, Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Cadence Design Systems, Inc.

// \w*clock_period\w*\|m_read_delay\|m_block_read_delay\|m_block_read_repeat\|m_burst_read_delay\|m_burst_read_repeat\|m_rcw_repeat\|m_rcw_response\|m_write_delay\|m_block_write_delay\|m_block_write_repeat\|m_block_write_response\|m_burst_write_delay\|m_burst_write_repeat\|m_burst_write_response\|m_recovery_time\|m_response_repeat\|\<wait\>

#include <cstdlib>
#include <ostream>
#include <string>
#include <xtsc/xtsc_memory.h>
#include <xtsc/xtsc_arbiter.h>
#include <xtsc/xtsc_core.h>
#include <xtsc/xtsc_dma_engine.h>
#include <xtsc/xtsc_master.h>
#include <xtsc/xtsc_memory_trace.h>
#include <xtsc/xtsc_router.h>
#include <xtsc/xtsc_pin2tlm_memory_transactor.h>
#include <xtsc/xtsc_logging.h>
#include <xtsc/xtsc_fast_access.h>

using namespace std;
#if SYSTEMC_VERSION >= 20050601
using namespace sc_core;
#endif
using namespace xtsc;
using log4xtensa::INFO_LOG_LEVEL;
using log4xtensa::VERBOSE_LOG_LEVEL;
using log4xtensa::DEBUG_LOG_LEVEL;
//using xtsc::xtsc_fast_access;
using xtsc::xtsc_raw_block;


#define BIT_1  0x2
#define BIT_11 0x800


namespace xtsc_component {

// Read callback for testing ACCESS_CALLBACKS fast access
void read_callback(void *callback_arg, u32 *dst, xtsc_address address8, u32 size8) {
  xtsc_memory *p_mem = (xtsc_memory *) callback_arg;
  p_mem->peek(address8, size8, (u8*)dst);
}



// Write callback for testing ACCESS_CALLBACKS fast access
void write_callback(void *callback_arg, xtsc_address address8, u32 size8, const u32 *src) {
  xtsc_memory *p_mem = (xtsc_memory *) callback_arg;
  p_mem->poke(address8, size8, (u8*)src);
}



// Read callback for testing ACCESS_CUSTOM_CALLBACKS fast access
void custom_read_callback(void *callback_arg, u32 *dst, xtsc_address address8, u32 size8, const u32 *custom_data) {
  xtsc_memory *p_mem = (xtsc_memory *) callback_arg;
  p_mem->peek(address8, size8, (u8*)dst);
  if (xtsc_is_text_logging_enabled() && p_mem->get_text_logger().isEnabledFor(VERBOSE_LOG_LEVEL)) {
    if (custom_data) {
      ostringstream oss;
      oss << hex << setfill('0');
      u8 *p_cd = ((u8 *) custom_data);
      // 160 bits divided by 8 gives 20 bytes which multiplied by 2 gives 40 hex nibbles
      for (u32 i=160/8; i>0; --i) {
        oss << setw(2) << (u32) p_cd[i-1];
      }
      XTSC_VERBOSE(p_mem->get_text_logger(), "custom_read_callback:  addr=0x" << hex << address8 << " custom_data=" << custom_data <<
                                             "=>0x" << oss.str());
    }
    else {
      XTSC_VERBOSE(p_mem->get_text_logger(), "custom_read_callback:  addr=0x" << hex << address8 << " custom_data=" << custom_data);
    }
  }
}



// Write callback for testing ACCESS_CUSTOM_CALLBACKS fast access
void custom_write_callback(void *callback_arg, xtsc_address address8, u32 size8, const u32 *src, const u32 *custom_data) {
  xtsc_memory *p_mem = (xtsc_memory *) callback_arg;
  p_mem->poke(address8, size8, (u8*)src);
  if (xtsc_is_text_logging_enabled() && p_mem->get_text_logger().isEnabledFor(VERBOSE_LOG_LEVEL)) {
    if (custom_data) {
      ostringstream oss;
      oss << hex << setfill('0');
      u8 *p_cd = ((u8 *) custom_data);
      // 160 bits divided by 8 gives 20 bytes which multiplied by 2 gives 40 hex nibbles
      for (u32 i=160/8; i>0; --i) {
        oss << setw(2) << (u32) p_cd[i-1];
      }
      XTSC_VERBOSE(p_mem->get_text_logger(), "custom_write_callback: addr=0x" << hex << address8 << " custom_data=" << custom_data <<
                                             "=>0x" << oss.str());
    }
    else {
      XTSC_VERBOSE(p_mem->get_text_logger(), "custom_write_callback: addr=0x" << hex << address8 << " custom_data=" << custom_data);
    }
  }
}




/* Simple class that just redispatches to the peek poke function */
class xtsc_memory_fast_access : public xtsc_fast_access_if {
public:
  
  xtsc_memory_fast_access(xtsc_memory& mem) : m_mem(mem), m_text(m_mem.get_text_logger())
  {
  }

  void nb_fast_access_read(xtsc_address address8, u32 size8, u8 *dst) {
    m_mem.peek(address8, size8, dst);
    if (xtsc_is_text_logging_enabled() && m_text.isEnabledFor(DEBUG_LOG_LEVEL)) {
      u32 buf_offset = 0;
      ostringstream oss;
      oss << hex << setfill('0');
      for (u32 i = 0; i<size8; ++i) {
        oss << setw(2) << (u32) dst[buf_offset] << " ";
        buf_offset += 1;
      }
      XTSC_DEBUG(m_text, "nb_fast_access_read:  [0x" << hex << address8 << "/" << dec << size8 << "] = " << oss.str());
    }
  }
  
  void nb_fast_access_write(xtsc_address address8, u32 size8, const u8 *src) {
    if (xtsc_is_text_logging_enabled() && m_text.isEnabledFor(DEBUG_LOG_LEVEL)) {
      u32 buf_offset = 0;
      ostringstream oss;
      oss << hex << setfill('0');
      for (u32 i = 0; i<size8; ++i) {
        oss << setw(2) << (u32) src[buf_offset] << " ";
        buf_offset += 1;
      }
      XTSC_DEBUG(m_text, "nb_fast_access_write: [0x" << hex << address8 << "/" << dec << size8 << "] = " << oss.str());
    }
    m_mem.poke(address8, size8, src);
  }

private:

  xtsc_memory&                  m_mem;
  log4xtensa::TextLogger&       m_text;
};

} // namespace xtsc_component



xtsc_component::xtsc_memory_parms::xtsc_memory_parms(const xtsc_core& core, const char *port_name, u32 delay, u32 num_ports) {
  xtsc_core::memory_port mem_port = xtsc_core::get_memory_port(port_name, &core); 
  if (!core.has_memory_port(mem_port)) {
    ostringstream oss;
    oss << "xtsc_memory_parms: core '" << core.name() << "' has no " << port_name << " memory interface.";
    throw xtsc_exception(oss.str());
  }

  // Infer number of ports if num_ports is 0
  if (!num_ports) {
    u32 mpc = core.get_multi_port_count(mem_port);
    // Normally be single-ported ...
    num_ports = 1;
    // ; however, if this core interface is multi-ported (e.g. has banks or has multiple LD/ST units without CBox) AND ...
    if (mpc > 1) {
      // this is the first port (port 0) of the interface AND ...
      if (xtsc_core::is_multi_port_zero(mem_port)) {
        // if all the other ports of the interface appear to not be connected (this is not reliable--they might be delay bound), then ...
        bool found_a_connected_port = false;
        for (u32 n=1; n<mpc; ++n) {
          xtsc_core::memory_port mem_port_n = core.get_nth_multi_port(mem_port, n);
          if (core.get_request_port(mem_port_n).get_interface()) {
            found_a_connected_port = true;
            break;
          }
        }
        if (!found_a_connected_port) {
          // infer multi-ported
          num_ports = mpc;
        }
      }
    }
  }

  const xtsc_core_parms& core_parms = core.get_parms();

  u32   width8            = core.get_memory_byte_width(mem_port);
  u32   start_address8    = 0;
  u32   size8             = 0;          // 4GB
  u32   page_byte_size    = 0x4000;     // 16KB

  if (xtsc_core::is_system_port(mem_port)) {
    if (delay == 0xFFFFFFFF) {
      delay = core_parms.get_u32("LocalMemoryLatency");
    }
  }
  else {
    xtsc_address sa8 = 0;
    core.get_local_memory_starting_byte_address(mem_port, sa8);
    if ((mem_port == xtsc_core::MEM_XLMI0LS0) ||
        (mem_port == xtsc_core::MEM_XLMI0LS1) ||
        core_parms.get_bool("SimFullLocalMemAddress"))
    {
      start_address8 = sa8;
    }
    // Ensure page_byte_size divides start address except don't go below minimum of width8*16
    if (sa8) {
      xtsc_address m = 1;
      while (m) {
        if (sa8 & m) break;
        m <<= 1;
      }
      if (m < page_byte_size) {
        if (m < width8*16) {
          page_byte_size = width8*16;
        }
        else {
          page_byte_size = m;
        }
      }
    }
    core.get_local_memory_byte_size(mem_port, size8);
    delay = core_parms.get_u32("LocalMemoryLatency") - 1;
  }

  init(width8, delay, start_address8, size8, num_ports);

  set("page_byte_size", page_byte_size);
  set("clock_period", core_parms.get_u32("SimClockFactor")*xtsc_get_system_clock_factor());
  if (xtsc_core::is_system_port(mem_port)) {
    set("check_alignment", true);
    if ((mem_port == xtsc_core::MEM_PIF) || (mem_port == xtsc_core::MEM_IDMA0)) {
      if (core_parms.get_bool("SimPIFFakeWriteResponses")) {
        set("write_responses", false);
      }
    }
  }
  if ((mem_port == xtsc_core::MEM_IROM0) || (mem_port == xtsc_core::MEM_DROM0LS0) || (mem_port == xtsc_core::MEM_DROM0LS1)) {
    set("read_only", true);
  }
}



xtsc_component::xtsc_memory::xtsc_memory(sc_module_name module_name, const xtsc_memory_parms& memory_parms) :
  sc_module               (module_name),
  xtsc_module             (*(sc_module*)this),
  m_memory_parms          (memory_parms),
  m_write_responses       (memory_parms.get_bool      ("write_responses")),
#if IEEE_1666_SYSTEMC >= 201101L
  m_script_thread_event   ("m_script_thread_event"),
#endif
  m_exclusive_monitors    (0),
  m_lua_function          (""),
  m_last                  (getenv("XTSC_MEMORY_SCRIPT_FILE_LAST_FALSE") == NULL),
  m_log_user_data_bytes   (memory_parms.get_u32       ("log_user_data_bytes")),
  m_user_data_type        (0),
  m_user_data_length      (0),
  m_p_user_data           (NULL),
  m_is_shared             (false),
  m_pass_dirty            (false),
  m_host_shared_memory    (memory_parms.get_bool      ("host_shared_memory")),
  m_interval_size         (memory_parms.get_u64       ("interval_size")),
  m_host_mutex            (memory_parms.get_bool      ("host_mutex")),
  m_use_fast_access       (memory_parms.get_bool      ("use_fast_access")),
  m_deny_fast_access      (memory_parms.get_u64_vector("deny_fast_access")),
  m_fast_access_size      (memory_parms.get_u64_vector("fast_access_size")),
  m_use_raw_access        (memory_parms.get_bool      ("use_raw_access")),
  m_use_callback_access   (memory_parms.get_bool      ("use_callback_access")),
  m_use_custom_access     (memory_parms.get_bool      ("use_custom_access")),
  m_use_interface_access  (memory_parms.get_bool      ("use_interface_access")),
  m_filter_peeks          (false),
  m_filter_pokes          (false),
  m_filter_requests       (false),
  m_filter_responses      (false),
  m_filtered_request      (),
  m_filtered_response     (m_filtered_request),
  m_secure_address_range  (memory_parms.get_u64_vector("secure_address_range")),
  m_text                  (log4xtensa::TextLogger::getInstance(name())),
  m_binary                (log4xtensa::BinaryLogger::getInstance(name()))
{

  u32 n = xtsc_address_nibbles();

  u32                 byte_width              =      memory_parms.get_u32         ("byte_width");
  u64                 start_byte_address      =      memory_parms.get_u64         ("start_byte_address");
  u64                 memory_byte_size        =      memory_parms.get_u64         ("memory_byte_size");
  u32                 page_byte_size          =      memory_parms.get_non_zero_u32("page_byte_size");
  const char         *initial_value_file      =      memory_parms.get_c_str       ("initial_value_file");
  u8                  memory_fill_byte        = (u8) memory_parms.get_u32         ("memory_fill_byte");
  const char         *shared_memory_name      =      memory_parms.get_c_str       ("shared_memory_name");

  m_p_memory = new xtsc_memory_b(name(), kind(), byte_width, start_byte_address, memory_byte_size, page_byte_size, initial_value_file,
                                 memory_fill_byte, m_host_shared_memory, m_host_mutex, shared_memory_name);

  m_start_address8      = m_p_memory->m_start_address8;
  m_size8               = m_p_memory->m_size8;
  m_width8              = m_p_memory->m_width8;
  m_end_address8        = m_p_memory->m_end_address8;

  memory_parms.get("read_only",          m_read_only);
  memory_parms.get("support_exclusive",  m_support_exclusive);
  memory_parms.get("summary",            m_summary);
  memory_parms.get("immediate_timing",   m_immediate_timing);
  memory_parms.get("delay_from_receipt", m_delay_from_receipt);
  memory_parms.get("wraparound",         m_wraparound);
  memory_parms.get("fail_request_mask",  m_fail_request_mask);
  memory_parms.get("fail_percentage",    m_fail_percentage);
  compute_let_through();
  memory_parms.get("fail_seed",          m_fail_seed);
  m_z = m_fail_seed;
  m_w = m_fail_seed;
  u32 status;
  memory_parms.get("fail_status",        status);
  m_fail_status = static_cast<xtsc_response::status_t>(status);

  m_num_ports                   = memory_parms.get_u32("num_ports");
  m_check_alignment             = memory_parms.get_bool("check_alignment");

  const char *script_file       = memory_parms.get_c_str("script_file");
  if (m_fail_request_mask && script_file) {
    ostringstream oss;
    oss << "Error in " << kind() << " '" << name()
        << "': It is not legal for both \"fail_request_mask\" and \"script_file\" to be defined (i.e. to be non-zero)";
    throw xtsc_exception(oss.str());
  }
  if ((m_num_ports == 0) && script_file) {
    ostringstream oss;
    oss << "Error in " << kind() << " '" << name() << "': A \"script_file\" should not be specified when \"num_ports\" is 0.";
    throw xtsc_exception(oss.str());
  }
  m_script_file                 = (script_file ? script_file : "");
  m_p_script_stream             = NULL;

  if (m_script_file != "") {
    m_p_script_stream  = new xtsc_script_file(m_script_file.c_str(), "script_file", name(), kind(), m_wraparound);
    m_script_file = m_p_script_stream->name();
  }

  for (u32 i=0; i<16; ++i) {
    m_response_list.push_back(xtsc_response::RSP_DATA_ERROR);
  }

  script_file                   = memory_parms.get_c_str("exclusive_script_file");
  m_exclusive_script_file       = (script_file ? script_file : "");
  m_p_exclusive_script_stream   = NULL;

  if (m_exclusive_script_file != "") {
    m_p_exclusive_script_stream  = new xtsc_script_file(m_exclusive_script_file.c_str(), "exclusive_script_file", name(), kind(), true);
  }


  // Create all the "per-port" stuff

  m_request_exports = new sc_export<xtsc_request_if>*[m_num_ports];
  m_request_impl    = new xtsc_request_if_impl*      [m_num_ports];
  for (u32 i=0; i<m_num_ports; ++i) {
    ostringstream oss1;
    oss1 << "m_request_exports[" << i << "]";
    m_request_exports[i] = new sc_export<xtsc_request_if>(oss1.str().c_str());
    m_port_types[oss1.str()] = REQUEST_EXPORT;
    ostringstream oss2;
    oss2 << "m_request_impl[" << i << "]";
    m_request_impl   [i] = new xtsc_request_if_impl(oss2.str().c_str(), *this, i);
    (*m_request_exports[i])(*m_request_impl[i]);
  }

  m_request_fifo = new sc_fifo<request_info*>*[m_num_ports];
  for (u32 i=0; i<m_num_ports; ++i) {
    ostringstream oss;
    oss << "m_request_fifo[" << i << "]";
    m_request_fifo[i] = new sc_fifo<request_info*>(oss.str().c_str(), memory_parms.get_non_zero_u32("request_fifo_depth"));
  }

  m_respond_ports = new sc_port<xtsc_respond_if>*[m_num_ports];
  for (u32 i=0; i<m_num_ports; ++i) {
    ostringstream oss;
    oss << "m_respond_ports[" << i << "]";
    m_respond_ports[i] = new sc_port<xtsc_respond_if>(oss.str().c_str());
    m_port_types[oss.str()] = RESPOND_PORT;
    oss.str("");
    oss << "slave_port[" << i << "]";
    m_port_types[oss.str()] = PORT_TABLE;
  }

  if (m_num_ports > 0) {
    m_port_types["slave_ports"] = PORT_TABLE;
  }

  if (m_num_ports == 1) {
    m_port_types["slave_port"] = PORT_TABLE;
  }

  m_rcw_compare_data            = new u8[m_num_ports*xtsc_max_bus_width8];
  m_rcw_have_first_transfer     = new bool[m_num_ports];
  m_p_active_request_info       = new request_info*[m_num_ports];
  m_p_active_response           = new xtsc_response*[m_num_ports];
  m_block_write_transfer_count  = new u32[m_num_ports];
  m_burst_write_transfer_count  = new u32[m_num_ports];
  m_first_block_write           = new bool[m_num_ports];
  m_first_burst_write           = new bool[m_num_ports];
  m_first_rcw                   = new bool[m_num_ports];
  m_multi_write_doit            = new bool[m_num_ports];
  m_last_action_time_stamp      = new sc_time[m_num_ports];
  m_worker_thread_event         = new sc_event*[m_num_ports];
  m_statistics                  = new statistics*[m_num_ports];

  for (u32 i=0; i<m_num_ports; ++i) {
    m_p_active_request_info[i]          = NULL;
    m_p_active_response[i]              = NULL;
    ostringstream oss;
    oss << "worker_thread_" << i;
#if IEEE_1666_SYSTEMC >= 201101L
  m_worker_thread_event[i] = new sc_event((oss.str() + "_event").c_str());
#else
  m_worker_thread_event[i] = new sc_event;
  xtsc_event_register(*m_worker_thread_event[i], oss.str() + "_event", this);
#endif
    declare_thread_process(worker_thread_handle, oss.str().c_str(), SC_CURRENT_USER_MODULE, worker_thread);
    m_process_handles.push_back(sc_get_current_process_handle());
  }

  for (u32 i=0; i<m_num_ports; ++i) {
    m_statistics[i] = new statistics();
  }

  if (m_p_script_stream != NULL) {
    SC_THREAD(script_thread);                   m_process_handles.push_back(sc_get_current_process_handle());
  }

  // Get clock period 
  u32 clock_period = memory_parms.get_u32("clock_period");
  if (clock_period == 0xFFFFFFFF) {
    m_clock_period = xtsc_get_system_clock_period();
  }
  else {
    m_clock_period = sc_get_time_resolution() * clock_period;
  }

  m_statistics_interval_duration = m_clock_period * m_interval_size;

  xtsc_memory::compute_delays();

  m_fast_access_object  = new xtsc_memory_fast_access(*this);

  if (m_deny_fast_access.size()) {
    if (m_deny_fast_access.size() & 0x1) {
      ostringstream oss;
      oss << kind() << " '" << name() << "': " << "\"deny_fast_access\" parameter has " << m_deny_fast_access.size()
          << " elements which is not an even number as it must be.";
      throw xtsc_exception(oss.str());
    }
    u32 bus_width = (m_width8 ? m_width8 : 4);
    for (u32 i=0; i<m_deny_fast_access.size(); i+=2) {
      xtsc_address begin = m_deny_fast_access[i+0];
      xtsc_address end   = m_deny_fast_access[i+1];
      if (((begin % bus_width) != 0) || (((end + 1) % bus_width) != 0)) {
        ostringstream oss;
        oss << "\"deny_fast_access\" parameter has begin/end pair (0x" << hex << setfill('0') << setw(n) << begin << "/0x" << setw(n) << end
            << ") which do not correspond to a begin/end bus width (respectively).  "
            << "This can result in TurboXim failing to deny fast access when it should.";
        XTSC_WARN(m_text, oss.str());
      }
      if ((begin < m_start_address8) || (begin > m_end_address8) ||
          (end   < m_start_address8) || (end   > m_end_address8) ||
          (end   < begin))
      {
        ostringstream oss;
        oss << kind() << " '" << name() << "': " << "\"deny_fast_access\" range [0x" << hex << setfill('0') << setw(n) << begin
            << "-0x" << setw(n) << end << "] is invalid." << endl;
        oss << "Valid ranges must fit within [0x" << setw(n) << m_start_address8 << "-0x" << m_end_address8 << "].";
        throw xtsc_exception(oss.str());
      }
    }
  }

  if (m_fast_access_size.size()) {
    if ((m_fast_access_size.size()/3)*3 != m_fast_access_size.size()) {
      ostringstream oss;
      oss << kind() << " '" << name() << "': " << "\"fast_access_size\" parameter has " << m_fast_access_size.size()
          << " elements which is not a multiple of 3 as it must be.";
      throw xtsc_exception(oss.str());
    }
    for (u32 i=0; i<m_fast_access_size.size(); i+=3) {
      xtsc_address begin = m_fast_access_size[i+0];
      xtsc_address end   = m_fast_access_size[i+1];
      xtsc_address gran  = m_fast_access_size[i+2];
      ostringstream oss;
      oss << kind() << " '" << name() << "': " << "\"fast_access_size\" triplet [0x" << hex << setfill('0') << setw(n) << begin
          << "-0x" << setw(n) << end << ": 0x" << gran << "] is invalid." << endl;
      if ((begin < m_start_address8) || (begin > m_end_address8) ||
          (end   < m_start_address8) || (end   > m_end_address8) ||
          (end   < begin))
      {
        oss << "Valid ranges must fit within [0x" << setw(n) << m_start_address8 << "-0x" << setw(n) << m_end_address8 << "].";
        throw xtsc_exception(oss.str());
      }
      if (m_width8 && ((gran/m_width8)*m_width8 != gran)) {
        oss << "Granularity (0x" << hex << gran << ") must be an integer multiple of bus width (0x" << m_width8 << ").";
        throw xtsc_exception(oss.str());
      }
      if ((gran/4)*4 != gran) {
        oss << "Granularity (0x" << hex << gran << ") must be an integer multiple of minimum bus width (0x4).";
        throw xtsc_exception(oss.str());
      }
      if ((begin/gran)*gran != begin) {
        oss << "Start address (0x" << hex << setw(n) << begin << ") must be an integer multiple of granularity (0x" << gran << ").";
        throw xtsc_exception(oss.str());
      }
      if (((end+1)/gran)*gran != (end+1)) {
        oss << "End address plus 1 (0x" << hex << setw(n) << (end+1) << ") must be an integer multiple of granularity (0x" << gran << ").";
        throw xtsc_exception(oss.str());
      }
    }
  }

  if (m_secure_address_range.size()) {
    if (m_secure_address_range.size() & 0x1) {
      ostringstream oss;
      oss << kind() << " '" << name() << "': " << "\"secure_address_range\" parameter has " << m_secure_address_range.size()
          << " elements which is not an even number as it must be.";
      throw xtsc_exception(oss.str());
    }
    for (u32 i=0; i<m_secure_address_range.size(); i+=2) {
      xtsc_address begin = m_secure_address_range[i+0];
      xtsc_address end   = m_secure_address_range[i+1];
      if ((begin < m_start_address8) || (begin > m_end_address8) ||
          (end   < m_start_address8) || (end   > m_end_address8) ||
          (end   < begin))
      {
        ostringstream oss;
        oss << kind() << " '" << name() << "': " << "\"secure_address_range\" range [0x" << hex << setfill('0') << setw(n) << begin
            << "-0x" << setw(n) << end << "] is invalid." << endl;
        oss << "Valid ranges must fit within [0x" << setw(n) << m_start_address8 << "-0x" << m_end_address8 << "].";
        throw xtsc_exception(oss.str());
      }
    }
  }

  xtsc_register_command(*this, *this, "change_clock_period", 1, 1,
      "change_clock_period <ClockPeriodFactor>", 
      "Call xtsc_memory::change_clock_period(<ClockPeriodFactor>).  Return previous <ClockPeriodFactor> for this device."
  );

  xtsc_register_command(*this, *this, "dump", 2, 2,
      "dump <StartAddress> <NumBytes>", 
      "Dump <NumBytes> of memory starting at <StartAddress> (includes header and printable ASCII column)."
  );

  xtsc_register_command(*this, *this, "dump_exclusive_monitors", 0, 0,
      "dump_exclusive_monitors", 
      "Dump all exclusive monitors, one per line, with format: <TranID>:<AddressBeg>-<AddressEnd>"
  );

  xtsc_register_command(*this, *this, "dump_filtered_request", 0, 0,
      "dump_filtered_request", 
      "Dump the most recent previous xtsc_request that passed a xtsc_request watchfilter."
  );

  xtsc_register_command(*this, *this, "dump_filtered_response", 0, 0,
      "dump_filtered_response", 
      "Dump the most recent previous xtsc_response that passed a xtsc_response watchfilter."
  );

  xtsc_register_command(*this, *this, "get_exclusive_monitors_count", 0, 0,
      "get_exclusive_monitors_count", 
      "Return the number of exclusive monitors currently active."
  );

  xtsc_register_command(*this, *this, "get_total_exclusive_monitors_created", 0, 0,
      "get_total_exclusive_monitors_created", 
      "Return the total number of exclusive monitors created so far in the simulation."
  );

  xtsc_register_command(*this, *this, "peek", 2, 2,
      "peek <StartAddress> <NumBytes>", 
      "Peek <NumBytes> of memory starting at <StartAddress>."
  );

  xtsc_register_command(*this, *this, "poke", 2, -1,
      "poke <StartAddress> <NumBytes> <Byte1> <Byte2> . . . <ByteN>", 
      "Poke <NumBytes> (=N) of memory starting at <StartAddress>."
  );

  xtsc_register_command(*this, *this, "reset", 0, 1,
      "reset [<Hard>]", 
      "Call xtsc_memory::reset(<Hard>).  Where <Hard> is 0|1 (default 0)."
  );

  xtsc_register_command(*this, *this, "summary", 0, 0,
      "summary", 
      "Call xtsc_memory::summary() for this memory instance."
  );

  xtsc_register_command(*this, *this, "support_exclusive", 0, 1,
      "support_exclusive [<Exclusive>]", 
      "Call xtsc_memory::support_exclusive(<Exclusive>), where <Exclusive> is 0|1, or return xtsc_memory::m_support_exclusive."
  );

  xtsc_register_command(*this, *this, "watchfilter_add", 2, 2,
      "watchfilter_add <FilterName> <EventName>", 
      "Calls xtsc_memory::watchfilter_add(<FilterName>, <Event>) and returns the watchfilter number."
      "  <EventName> can be a hyphen (-) to mean the last event created by the xtsc_event_create command."
  );

  xtsc_register_command(*this, *this, "watchfilter_dump", 0, 0,
      "watchfilter_dump", 
      "Return xtsc_memory::watchfilter_dump()."
  );

  xtsc_register_command(*this, *this, "watchfilter_remove", 1, 1,
      "watchfilter_remove <Watchfilter>", 
      "Return xtsc_memory::watchfilter_remove(<Watchfilter>).  An * removes all watchfilters."
  );

#if IEEE_1666_SYSTEMC < 201101L
  xtsc_event_register(m_script_thread_event, "m_script_thread_event", this);
#endif

  log4xtensa::LogLevel ll = xtsc_get_constructor_log_level();
  XTSC_LOG(m_text, ll,        "Constructed " << kind() << " '" << name() << "':");
  XTSC_LOG(m_text, ll,        " num_ports               = "   << m_num_ports);
  XTSC_LOG(m_text, ll, hex << " start_byte_address      = 0x" << m_start_address8);
  XTSC_LOG(m_text, ll, hex << " memory_byte_size        = 0x" << m_size8 << (m_size8 ? " " : " (4GB boundary)"));
  XTSC_LOG(m_text, ll, hex << " End byte address        = 0x" << m_end_address8);
  for (u32 i=0; i<m_secure_address_range.size(); i+=2) {
  XTSC_LOG(m_text, ll, hex << "  [0x" << setfill('0') << setw(n) << m_secure_address_range[i+0] << "-0x"
                                                      << setw(n) << m_secure_address_range[i+1] << "]");
  }
  if (m_width8) {
  XTSC_LOG(m_text, ll,        " byte_width              = "   << m_width8);
  } else {
  XTSC_LOG(m_text, ll,        " byte_width              = 0 => 4|8|16|32|64");
  }
  XTSC_LOG(m_text, ll,        " write_responses         = "   << boolalpha << m_write_responses);
  XTSC_LOG(m_text, ll,        " check_alignment         = "   << boolalpha << m_check_alignment);
  XTSC_LOG(m_text, ll, hex << " page_byte_size          = 0x" << m_p_memory->m_page_size8);
  XTSC_LOG(m_text, ll, hex << " initial_value_file      = "   << m_p_memory->m_initial_value_file);
  XTSC_LOG(m_text, ll, hex << " memory_fill_byte        = 0x" << (u32) m_p_memory->m_memory_fill_byte);
  XTSC_LOG(m_text, ll,        " host_shared_memory      = "   << boolalpha << m_host_shared_memory);
  XTSC_LOG(m_text, ll,        " interval_size           = "   << m_interval_size);
  XTSC_LOG(m_text, ll,        " host_mutex              = "   << boolalpha << m_host_mutex);
  if (!m_host_shared_memory | (shared_memory_name && shared_memory_name[0])) {
  XTSC_LOG(m_text, ll,        " shared_memory_name      = "   << m_p_memory->get_shared_memory_name());
  } else {
  XTSC_LOG(m_text, ll,        " shared_memory_name      = \"\" => " << m_p_memory->get_shared_memory_name());
  }
  XTSC_LOG(m_text, ll,        " summary                 = "   << boolalpha << m_summary);
  XTSC_LOG(m_text, ll,        " support_exclusive       = "   << boolalpha << m_support_exclusive);
  XTSC_LOG(m_text, ll,        " immediate_timing        = "   << (m_immediate_timing ? "true" : "false"));
  XTSC_LOG(m_text, ll,        " read_only               = "   << boolalpha << m_read_only);
  if (m_log_user_data_bytes >= 0xFFFFFFF8) {
  XTSC_LOG(m_text, ll, hex << " log_user_data_bytes     = 0x" << m_log_user_data_bytes << "/" << dec << ((i32)m_log_user_data_bytes));
  } else {
  XTSC_LOG(m_text, ll,        " log_user_data_bytes     = "   << m_log_user_data_bytes);
  }
  if (!m_immediate_timing) {
  if (clock_period == 0xFFFFFFFF) {
  XTSC_LOG(m_text, ll, hex << " clock_period            = 0x" << clock_period << " (" << m_clock_period << ")");
  } else {
  XTSC_LOG(m_text, ll,        " clock_period            = "   << clock_period << " (" << m_clock_period << ")");
  }
  XTSC_LOG(m_text, ll,        " request_fifo_depth      = "   << memory_parms.get_u32("request_fifo_depth"));
  XTSC_LOG(m_text, ll,        " delay_from_receipt      = "   << (m_delay_from_receipt ? "true" : "false"));
  XTSC_LOG(m_text, ll,        " read_delay              = "   << memory_parms.get_u32("read_delay"));
  XTSC_LOG(m_text, ll,        " write_delay             = "   << memory_parms.get_u32("write_delay"));
  XTSC_LOG(m_text, ll,        " block_read_delay        = "   << memory_parms.get_u32("block_read_delay"));
  XTSC_LOG(m_text, ll,        " block_read_repeat       = "   << memory_parms.get_u32("block_read_repeat"));
  XTSC_LOG(m_text, ll,        " burst_read_delay        = "   << memory_parms.get_u32("burst_read_delay"));
  XTSC_LOG(m_text, ll,        " burst_read_repeat       = "   << memory_parms.get_u32("burst_read_repeat"));
  XTSC_LOG(m_text, ll,        " block_write_delay       = "   << memory_parms.get_u32("block_write_delay"));
  XTSC_LOG(m_text, ll,        " block_write_repeat      = "   << memory_parms.get_u32("block_write_repeat"));
  XTSC_LOG(m_text, ll,        " block_write_response    = "   << memory_parms.get_u32("block_write_response"));
  XTSC_LOG(m_text, ll,        " burst_write_delay       = "   << memory_parms.get_u32("burst_write_delay"));
  XTSC_LOG(m_text, ll,        " burst_write_repeat      = "   << memory_parms.get_u32("burst_write_repeat"));
  XTSC_LOG(m_text, ll,        " burst_write_response    = "   << memory_parms.get_u32("burst_write_response"));
  XTSC_LOG(m_text, ll,        " rcw_repeat              = "   << memory_parms.get_u32("rcw_repeat"));
  XTSC_LOG(m_text, ll,        " rcw_response            = "   << memory_parms.get_u32("rcw_response"));
  XTSC_LOG(m_text, ll,        " recovery_time           = "   << memory_parms.get_u32("recovery_time"));
  XTSC_LOG(m_text, ll,        " response_repeat         = "   << memory_parms.get_u32("response_repeat"));
  }
  XTSC_LOG(m_text, ll,        " use_fast_access         = "   << boolalpha << m_use_fast_access);
  XTSC_LOG(m_text, ll,        " deny_fast_access       :");
  for (u32 i=0; i<m_deny_fast_access.size(); i+=2) {
  XTSC_LOG(m_text, ll, hex << "  [0x" << setfill('0') << setw(n) << m_deny_fast_access[i+0] << "-0x"
                                                      << setw(n) << m_deny_fast_access[i+1] << "]");
  }
  XTSC_LOG(m_text, ll,        " fast_access_size       :");
  for (u32 i=0; i<m_fast_access_size.size(); i+=3) {
  XTSC_LOG(m_text, ll, hex << "  [0x" << setfill('0') << setw(n) << m_fast_access_size[i+0] << "-0x"
                                                      << setw(n) << m_fast_access_size[i+1] << ": 0x"
                                                                 << m_fast_access_size[i+2] << "]");
  }
  XTSC_LOG(m_text, ll,        " use_raw_access          = "   << boolalpha << m_use_raw_access);
  XTSC_LOG(m_text, ll,        " use_callback_access     = "   << boolalpha << m_use_callback_access);
  XTSC_LOG(m_text, ll,        " use_custom_access       = "   << boolalpha << m_use_custom_access);
  XTSC_LOG(m_text, ll,        " use_interface_access    = "   << boolalpha << m_use_interface_access);
  XTSC_LOG(m_text, ll,        " script_file             = "   << m_script_file);
  XTSC_LOG(m_text, ll,        " exclusive_script_file   = "   << m_exclusive_script_file);
  XTSC_LOG(m_text, ll,        " wraparound              = "   << boolalpha << m_wraparound);
  XTSC_LOG(m_text, ll, hex << " fail_request_mask       = 0x" << m_fail_request_mask);
  if (m_fail_request_mask) {
  XTSC_LOG(m_text, ll,        " fail_percentage         = "   << m_fail_percentage);
  XTSC_LOG(m_text, ll,        " fail_seed               = "   << m_fail_seed);
  XTSC_LOG(m_text, ll,        " fail_status             = "   << xtsc_response::get_status_name(m_fail_status));
  XTSC_LOG(m_text, ll, hex << " Maximum random value    = 0xFFFFFFFF");
  }

  reset(true);

}



xtsc_component::xtsc_memory::~xtsc_memory(void) {
  XTSC_DEBUG(m_text, "In ~xtsc_memory()");
  if (m_p_memory) {
    delete m_p_memory;
    m_p_memory = 0;
  }

  for (u32 i=0; i<m_num_ports; i++) {
    std::list<statistics::statistics_per_interval*> *stats_interval_list = m_statistics[i]->m_stats_interval_list;
    std::list<statistics::statistics_per_interval*>::iterator it = stats_interval_list->begin();
    while (it != stats_interval_list->end())
    {
      delete *it;
      stats_interval_list->erase(it++); 
    }
  }

  for (u32 i=0; i<m_num_ports; ++i) {
    delete m_statistics[i];
  }

  delete [] m_statistics;
}



void xtsc_component::xtsc_memory::end_of_simulation() {
  if (m_exclusive_monitors) {
    XTSC_INFO(m_text, "Total exclusive monitors created: " << m_exclusive_monitors);
  }
  if (m_exclusive_monitor_map.size()) {
    ostringstream oss;
    oss << "Exclusive monitors still active:" << endl;
    dump_exclusive_monitors(oss);
    xtsc_log_multiline(m_text, log4xtensa::WARN_LOG_LEVEL, oss.str(), 2);
  }

  //Push active interval to the list
  for (u32 port_num = 0; port_num < m_num_ports ; port_num++) {
    statistics::statistics_per_interval *active_stats_interval = m_statistics[port_num]->m_active_stats_interval;
    if (active_stats_interval != NULL) {
      //Close active interval and push to the list
      active_stats_interval->m_end_time = sc_time_stamp();
      close_active_stats_interval(active_stats_interval, port_num);
      XTSC_TRACE(m_text, "Last interval " << active_stats_interval->m_start_time << " - "
                         << active_stats_interval->m_end_time);
      XTSC_TRACE(m_text, "m_active_stats_interval set to NULL.  Port#" << port_num);
      m_statistics[port_num]->m_active_stats_interval = NULL;
    }
  }

  if (m_summary) {
    ostringstream oss;
    summary(oss);
    xtsc_log_multiline(m_text, log4xtensa::NOTE_LOG_LEVEL, oss.str());
  }
}


void xtsc_component::xtsc_memory::update_stats_active_interval() {
  sc_time current_time  = sc_time_stamp();

  for(u32 port_num=0; port_num < m_num_ports; port_num++) {
    statistics::statistics_per_interval *active_stats_interval = m_statistics[port_num]->m_active_stats_interval;

    if (active_stats_interval == NULL) {
        //Should be the first Interval
        active_stats_interval               = new statistics::statistics_per_interval();
        active_stats_interval->m_start_time = SC_ZERO_TIME;
    }

    //Loop to keep adding intervals till the interval for current timestamp becomes active
    while (current_time > active_stats_interval->m_start_time + m_statistics_interval_duration) {
      //Close active interval and push to the list
      active_stats_interval->m_end_time = active_stats_interval->m_start_time + m_statistics_interval_duration;
      close_active_stats_interval(active_stats_interval,port_num);
      XTSC_TRACE(m_text, "Closed interval " << active_stats_interval->m_start_time << " - "
                         << active_stats_interval->m_end_time);
      //Create next interval
      statistics::statistics_per_interval *next_stats_interval = new statistics::statistics_per_interval();
      next_stats_interval->m_start_time = active_stats_interval->m_end_time;
      //Now make next interval active
      active_stats_interval = next_stats_interval;
    }
    
    m_statistics[port_num]->m_active_stats_interval = active_stats_interval;
  }
}



void xtsc_component::xtsc_memory::close_active_stats_interval(statistics::statistics_per_interval 
                                                                          *active_stats_interval, u32 port_num) {
  u64 total_bytes_per_interval            =  active_stats_interval->m_num_read_bytes_xfered + 
                                             active_stats_interval->m_num_write_bytes_xfered;
  double interval_duration_in_sec         = (active_stats_interval->m_end_time -
                                             active_stats_interval->m_start_time).to_seconds();
  active_stats_interval->m_throughput     = (double)(total_bytes_per_interval * 8 * 1e-6) /
                                                     interval_duration_in_sec;
  ostringstream oss;
  oss << "Interval(" << active_stats_interval->m_start_time << " - " << active_stats_interval->m_end_time << ")";
  oss << "  Read Requests : "  << active_stats_interval->m_num_read_requests 
      << "  Read Bytes : "     << active_stats_interval->m_num_read_bytes_xfered 
      << "  Write Requests : " << active_stats_interval->m_num_write_requests
      << "  Write Bytes : "    << active_stats_interval->m_num_write_bytes_xfered 
      << "  Throughput : "     << active_stats_interval->m_throughput 
      << "   Port #"           << port_num;
  XTSC_TRACE(m_text, oss.str()); 
  m_statistics[port_num]->m_stats_interval_list->push_back(active_stats_interval);

}


void xtsc_component::xtsc_memory::summary(std::ostream& os) {
  os << endl;
  os << std::left << std::setw(18) << "XTSC Memory"       << " : \"" << name() << "\"" << endl;
  os << std::left << std::setw(18) << "Interval duration" << " : "   << m_statistics_interval_duration << endl;
  for (u32 port_num = 0; port_num < m_num_ports ; port_num++) {
    print_summary_per_port(os, port_num);
  }
}


void xtsc_component::xtsc_memory::print_summary_per_port(ostream& os, u32 port_num) {

  std::list<statistics::statistics_per_interval*> *stats_interval_list  = m_statistics[port_num]->m_stats_interval_list;
  statistics::statistics_per_interval *active_stats_interval            = m_statistics[port_num]->m_active_stats_interval;

  if (active_stats_interval != NULL) {
    //Close active interval and push to the list
    active_stats_interval->m_end_time = sc_time_stamp();
    close_active_stats_interval(active_stats_interval, port_num);
    XTSC_TRACE(m_text, "Closed active interval temporarily for summary report.  Port#" << port_num);
  }

  bool empty = false;
  std::list<statistics::statistics_per_interval*>::iterator it = stats_interval_list->begin();
  if (it == stats_interval_list->end()) {
    //eq if active_stats_interval == NULL 
    //ie summary() called before any inbound transaction to memory
    empty = true;
  } 

  double        max_throughput                          = empty ? 0             : (*it)->m_throughput;
  sc_time       max_throughput_start_time               = empty ? SC_ZERO_TIME  : (*it)->m_start_time;
  sc_time       max_throughput_end_time                 = empty ? SC_ZERO_TIME  : (*it)->m_end_time;
  u64           max_num_read_requests                   = empty ? 0             : (*it)->m_num_read_requests;
  sc_time       max_num_read_requests_start_time        = empty ? SC_ZERO_TIME  : (*it)->m_start_time;
  sc_time       max_num_read_requests_end_time          = empty ? SC_ZERO_TIME  : (*it)->m_end_time;
  u64           max_num_read_bytes_xfered               = empty ? 0             : (*it)->m_num_read_bytes_xfered;
  sc_time       max_num_read_bytes_xfered_start_time    = empty ? SC_ZERO_TIME  : (*it)->m_start_time;
  sc_time       max_num_read_bytes_xfered_end_time      = empty ? SC_ZERO_TIME  : (*it)->m_end_time;
  u64           max_num_write_requests                  = empty ? 0             : (*it)->m_num_write_requests;
  sc_time       max_num_write_requests_start_time       = empty ? SC_ZERO_TIME  : (*it)->m_start_time;
  sc_time       max_num_write_requests_end_time         = empty ? SC_ZERO_TIME  : (*it)->m_end_time;
  u64           max_num_write_bytes_xfered              = empty ? 0             : (*it)->m_num_write_bytes_xfered;
  sc_time       max_num_write_bytes_xfered_start_time   = empty ? SC_ZERO_TIME  : (*it)->m_start_time;
  sc_time       max_num_write_bytes_xfered_end_time     = empty ? SC_ZERO_TIME  : (*it)->m_end_time;

  double        min_throughput                          = empty ? 0             : (*it)->m_throughput;
  sc_time       min_throughput_start_time               = empty ? SC_ZERO_TIME  : (*it)->m_start_time;
  sc_time       min_throughput_end_time                 = empty ? SC_ZERO_TIME  : (*it)->m_end_time;
  u64           min_num_read_requests                   = empty ? 0             : (*it)->m_num_read_requests;
  sc_time       min_num_read_requests_start_time        = empty ? SC_ZERO_TIME  : (*it)->m_start_time;
  sc_time       min_num_read_requests_end_time          = empty ? SC_ZERO_TIME  : (*it)->m_end_time;
  u64           min_num_read_bytes_xfered               = empty ? 0             : (*it)->m_num_read_bytes_xfered;
  sc_time       min_num_read_bytes_xfered_start_time    = empty ? SC_ZERO_TIME  : (*it)->m_start_time;
  sc_time       min_num_read_bytes_xfered_end_time      = empty ? SC_ZERO_TIME  : (*it)->m_end_time;
  u64           min_num_write_requests                  = empty ? 0             : (*it)->m_num_write_requests;
  sc_time       min_num_write_requests_start_time       = empty ? SC_ZERO_TIME  : (*it)->m_start_time;
  sc_time       min_num_write_requests_end_time         = empty ? SC_ZERO_TIME  : (*it)->m_end_time;
  u64           min_num_write_bytes_xfered              = empty ? 0             : (*it)->m_num_write_bytes_xfered;
  sc_time       min_num_write_bytes_xfered_start_time   = empty ? SC_ZERO_TIME  : (*it)->m_start_time;
  sc_time       min_num_write_bytes_xfered_end_time     = empty ? SC_ZERO_TIME  : (*it)->m_end_time;

  u64           total_num_read_requests                 = empty ? 0             : (*it)->m_num_read_requests;          
  u64           total_num_read_bytes_xfered             = empty ? 0             : (*it)->m_num_read_bytes_xfered;  
  u64           total_num_write_requests                = empty ? 0             : (*it)->m_num_write_requests;        
  u64           total_num_write_bytes_xfered            = empty ? 0             : (*it)->m_num_write_bytes_xfered;

  if (!empty) {
    //Iterate from second stats_interval_list
    it++;
    for (; it != stats_interval_list->end(); ++it) {

      if ((*it)->m_throughput > max_throughput ) {
        max_throughput                            = (*it)->m_throughput;
        max_throughput_start_time                 = (*it)->m_start_time;
        max_throughput_end_time                   = (*it)->m_end_time;
      }
      else if ((*it)->m_throughput < min_throughput) {
        min_throughput                            = (*it)->m_throughput;
        min_throughput_start_time                 = (*it)->m_start_time;
        min_throughput_end_time                   = (*it)->m_end_time;
      }

      if ((*it)->m_num_read_requests > max_num_read_requests ) {
        max_num_read_requests                     = (*it)->m_num_read_requests;
        max_num_read_requests_start_time          = (*it)->m_start_time;
        max_num_read_requests_end_time            = (*it)->m_end_time;
      }
      else if ((*it)->m_num_read_requests < min_num_read_requests) {
        min_num_read_requests                     = (*it)->m_num_read_requests;
        min_num_read_requests_start_time          = (*it)->m_start_time;
        min_num_read_requests_end_time            = (*it)->m_end_time;
      }

      if ((*it)->m_num_read_bytes_xfered > max_num_read_bytes_xfered ) {
        max_num_read_bytes_xfered                 = (*it)->m_num_read_bytes_xfered;
        max_num_read_bytes_xfered_start_time      = (*it)->m_start_time;
        max_num_read_bytes_xfered_end_time        = (*it)->m_end_time;
      }
      else if ((*it)->m_num_read_bytes_xfered < min_num_read_bytes_xfered) {
        min_num_read_bytes_xfered                 = (*it)->m_num_read_bytes_xfered;
        min_num_read_bytes_xfered_start_time      = (*it)->m_start_time;
        min_num_read_bytes_xfered_end_time        = (*it)->m_end_time;
      }

      if ((*it)->m_num_write_requests > max_num_write_requests ) {
        max_num_write_requests                    = (*it)->m_num_write_requests;
        max_num_write_requests_start_time         = (*it)->m_start_time;
        max_num_write_requests_end_time           = (*it)->m_end_time;
      }
      else if ((*it)->m_num_write_requests < min_num_write_requests) {
        min_num_write_requests                    = (*it)->m_num_write_requests;
        min_num_write_requests_start_time         = (*it)->m_start_time;
        min_num_write_requests_end_time           = (*it)->m_end_time;
      }

      if ((*it)->m_num_write_bytes_xfered > max_num_write_bytes_xfered ) {
        max_num_write_bytes_xfered                = (*it)->m_num_write_bytes_xfered;
        max_num_write_bytes_xfered_start_time     = (*it)->m_start_time;
        max_num_write_bytes_xfered_end_time       = (*it)->m_end_time;
      }
      else if ((*it)->m_num_write_bytes_xfered < min_num_write_bytes_xfered) {
        min_num_write_bytes_xfered                = (*it)->m_num_write_bytes_xfered;
        min_num_write_bytes_xfered_start_time     = (*it)->m_start_time;
        min_num_write_bytes_xfered_end_time       = (*it)->m_end_time;
      }

      //Update cumulated values
      total_num_read_requests                    += (*it)->m_num_read_requests; 
      total_num_read_bytes_xfered                += (*it)->m_num_read_bytes_xfered; 
      total_num_write_requests                   += (*it)->m_num_write_requests; 
      total_num_write_bytes_xfered               += (*it)->m_num_write_bytes_xfered; 
    }

    //Pop back the active interval
    stats_interval_list->pop_back();
  } //empty
  
  u64 total_bytes_xfered        = total_num_read_bytes_xfered + total_num_write_bytes_xfered;
  double simulation_throughput  = (total_bytes_xfered)* 8 * 1e-6 /sc_time_stamp().to_seconds();

  size_t value_max_size = get_max_size( 
    to_string(max_throughput).size(),
    to_string(max_num_read_requests).size(),
    to_string(max_num_read_bytes_xfered).size(),
    to_string(max_num_write_requests).size(),
    to_string(max_num_write_bytes_xfered).size(),
    to_string(min_throughput).size(),
    to_string(min_num_read_requests).size(),
    to_string(min_num_read_bytes_xfered).size(),
    to_string(min_num_write_requests).size(),
    to_string(min_num_write_bytes_xfered).size()
  );

  value_max_size = get_max_size(
    value_max_size,
    to_string(simulation_throughput).size(),
    to_string(total_num_read_requests).size(),
    to_string(total_num_read_bytes_xfered).size(),
    to_string(total_num_write_requests).size(),
    to_string(total_num_write_bytes_xfered).size(),
    to_string(m_statistics[port_num]->m_num_nacced_requests).size()
  );

  size_t start_interval_max_size = get_max_size(
    max_throughput_start_time.to_string().size(),
    max_num_read_requests_start_time.to_string().size(),
    max_num_read_bytes_xfered_start_time.to_string().size(),
    max_num_write_requests_start_time.to_string().size(),
    max_num_write_bytes_xfered_start_time.to_string().size(),
    min_throughput_start_time.to_string().size(),
    min_num_read_requests_start_time.to_string().size(),
    min_num_read_bytes_xfered_start_time.to_string().size(),
    min_num_write_requests_start_time.to_string().size(),
    min_num_write_bytes_xfered_start_time.to_string().size()
  );

  size_t end_interval_max_size = get_max_size(
    max_throughput_end_time.to_string().size(),
    max_num_read_requests_end_time.to_string().size(),
    max_num_read_bytes_xfered_end_time.to_string().size(),
    max_num_write_requests_end_time.to_string().size(),
    max_num_write_bytes_xfered_end_time.to_string().size(),
    min_throughput_end_time.to_string().size(),
    min_num_read_requests_end_time.to_string().size(),
    min_num_read_bytes_xfered_end_time.to_string().size(),
    min_num_write_requests_end_time.to_string().size(),
    min_num_write_bytes_xfered_end_time.to_string().size()
  );

  size_t value_setw_size            = (value_max_size          >= 11) ? value_max_size         +2 : 12;
  size_t start_interval_setw_size   = (start_interval_max_size >= 11) ? start_interval_max_size+2 : 12;
  size_t end_interval_setw_size     = (end_interval_max_size   >= 11) ? end_interval_max_size  +2 : 12;

  os << std::left << std::setw(18) << "Port index" << " : "  << port_num << endl; 
  os << endl; 

  os << std::left  << std::setw(26) << "Matrix" 
     << std::right << std::setw(value_setw_size) << "Value"  
     << std::right << std::setw(start_interval_setw_size+6) << "Interval" << endl;

  os << std::left << "Maximum" << endl; 
  print_summary_line(os, "- Throughput(Mbps)",     ":",  max_throughput, max_throughput_start_time, max_throughput_end_time, 
                                                         value_setw_size, start_interval_setw_size, end_interval_setw_size); 
  os << std::left << "- Reads" << endl; 
  print_summary_line(os, "  - Number of Requests", ":", (double)max_num_read_requests, max_num_read_requests_start_time, 
                                                         max_num_read_requests_end_time, value_setw_size, 
                                                         start_interval_setw_size, end_interval_setw_size); 
  print_summary_line(os, "  - Bytes Transferred",  ":", (double)max_num_read_bytes_xfered, max_num_read_bytes_xfered_start_time, 
                                                         max_num_read_bytes_xfered_end_time, value_setw_size, 
                                                         start_interval_setw_size, end_interval_setw_size); 
  os << std::left << "- Writes" << endl; 
  print_summary_line(os, "  - Number of Requests", ":", (double)max_num_write_requests, max_num_write_requests_start_time, 
                                                         max_num_write_requests_end_time, value_setw_size, 
                                                         start_interval_setw_size, end_interval_setw_size); 
  print_summary_line(os, "  - Bytes Transferred",  ":", (double)max_num_write_bytes_xfered, max_num_write_bytes_xfered_start_time, 
                                                         max_num_write_bytes_xfered_end_time, value_setw_size, 
                                                         start_interval_setw_size, end_interval_setw_size);

  os << std::left << "Minimum" << endl; 
  print_summary_line(os, "- Throughput(Mbps)",     ":",  min_throughput, min_throughput_start_time, min_throughput_end_time, 
                                                         value_setw_size, start_interval_setw_size, end_interval_setw_size); 
  os << std::left << "- Reads" << endl; 
  print_summary_line(os, "  - Number of Requests", ":", (double)min_num_read_requests, min_num_read_requests_start_time, 
                                                         min_num_read_requests_end_time, value_setw_size, 
                                                         start_interval_setw_size, end_interval_setw_size); 
  print_summary_line(os, "  - Bytes Transferred",  ":", (double)min_num_read_bytes_xfered, min_num_read_bytes_xfered_start_time, 
                                                         min_num_read_bytes_xfered_end_time, value_setw_size, 
                                                         start_interval_setw_size, end_interval_setw_size); 
  os << std::left << "- Writes" << endl; 
  print_summary_line(os, "  - Number of Requests", ":", (double)min_num_write_requests, min_num_write_requests_start_time, 
                                                         min_num_write_requests_end_time, value_setw_size, 
                                                         start_interval_setw_size, end_interval_setw_size); 
  print_summary_line(os, "  - Bytes Transferred",  ":", (double)min_num_write_bytes_xfered, min_num_write_bytes_xfered_start_time, 
                                                         min_num_write_bytes_xfered_end_time, value_setw_size, 
                                                         start_interval_setw_size, end_interval_setw_size);
  os << endl;

  os << std::left << "Statistics per simulation" << endl;
  print_summary_line(os, "- Throughput(Mbps)",     ":",  simulation_throughput, value_setw_size);

  os << std::left << "- Reads " << endl;
  print_summary_line(os, "  - Number of Requests", ":", (double)total_num_read_requests, value_setw_size); 
  print_summary_line(os, "  - Bytes Transferred",  ":", (double)total_num_read_bytes_xfered, value_setw_size); 
  
  os << std::left << "- Writes " << endl;
  print_summary_line(os, "  - Number of Requests", ":", (double)total_num_write_requests, value_setw_size); 
  print_summary_line(os, "  - Bytes Transferred",  ":", (double)total_num_write_bytes_xfered, value_setw_size);

  print_summary_line(os, "- Nacced Requests",      ":", (double)m_statistics[port_num]->m_num_nacced_requests, value_setw_size);
  os << "" << endl;

}



void xtsc_component::xtsc_memory::print_summary_line(ostream& os, const char* stats, const char* seperator, double value,
                                                     size_t value_setw_size) {
  os << std::left  << std::setw(24) << stats 
     <<               std::setw(2)  << seperator 
     << std::right << std::setw(value_setw_size) << value << endl;
}



void xtsc_component::xtsc_memory::print_summary_line(ostream& os, const char* stats, const char* seperator, double value, 
                                                     sc_time &start, sc_time &end, size_t value_setw_size,
                                                     size_t start_interval_setw_size, size_t end_interval_setw_size) { 
  os << std::left  << std::setw(24) << stats 
     <<               std::setw(2)  << seperator 
     << std::right << std::setw(value_setw_size) << value
     <<               std::setw(start_interval_setw_size) << start
     <<               std::setw(3)  << " - "
     << std::left  << std::setw(end_interval_setw_size) << end << endl;
}

size_t xtsc_component::xtsc_memory::get_max_size(
  size_t arg0,
  size_t arg1,
  size_t arg2,
  size_t arg3,
  size_t arg4,
  size_t arg5,
  size_t arg6,
  size_t arg7,
  size_t arg8,
  size_t arg9
) {
  size_t res = arg0;
  res = max(res, arg1);
  res = max(res, arg2);
  res = max(res, arg3);
  res = max(res, arg4);
  res = max(res, arg5);
  res = max(res, arg6);
  res = max(res, arg7);
  res = max(res, arg8);
  res = max(res, arg9);
  return res;
}

u32 xtsc_component::xtsc_memory::get_bit_width(const string& port_name, u32 interface_num) const {
  return m_width8*8;
}



sc_object *xtsc_component::xtsc_memory::get_port(const string& port_name) {
  string name_portion;
  u32    index;
  bool   indexed = xtsc_parse_port_name(port_name, name_portion, index);

  if (((m_num_ports > 1) && !indexed) ||
      (m_num_ports == 0) ||
      ((name_portion != "m_respond_ports") && (name_portion != "m_request_exports")))
  {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\" has no port named \"" << port_name << "\"" << endl;
    throw xtsc_exception(oss.str());
  }

  if (index > m_num_ports - 1) {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\": Invalid index: \"" << port_name << "\".  Valid range: 0-" << (m_num_ports-1) << endl;
    throw xtsc_exception(oss.str());
  }

  return ((name_portion == "m_respond_ports") ? (sc_object*) m_respond_ports[index] : (sc_object*) m_request_exports[index]);
}



xtsc_port_table xtsc_component::xtsc_memory::get_port_table(const string& port_table_name) const {
  if ((m_num_ports > 0) && (port_table_name == "slave_ports")) {
    xtsc_port_table table;
    for (u32 i=0; i<m_num_ports; ++i) {
      ostringstream oss;
      oss << "slave_port[" << i << "]";
      table.push_back(oss.str());
    }
    return table;
  }
  
  string name_portion;
  u32    index;
  bool   indexed = xtsc_parse_port_name(port_table_name, name_portion, index);

  if ((name_portion != "slave_port") || (m_num_ports == 0) || ((m_num_ports > 1) && !indexed)) {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\" has no port table named \"" << port_table_name << "\"" << endl;
    throw xtsc_exception(oss.str());
  }

  if (index > m_num_ports - 1) {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\": Invalid index: \"" << port_table_name << "\".  Valid range: 0-" << (m_num_ports-1) << endl;
    throw xtsc_exception(oss.str());
  }

  xtsc_port_table table;
  ostringstream oss;
  oss.str(""); oss << "m_request_exports" << "[" << index << "]"; table.push_back(oss.str());
  oss.str(""); oss << "m_respond_ports"   << "[" << index << "]"; table.push_back(oss.str());

  return table;
}



void xtsc_component::xtsc_memory::reset(bool hard_reset) {
  XTSC_INFO(m_text, kind() << "::reset()");

  m_next_port_num               = 0;
  m_line                        = "";
  m_line_count                  = 0;
  m_prev_type                   = 0xFFFFFFFF;
  m_prev_hit                    = false;
  m_prev_port                   = 0xFFFFFFFF;

  for (u32 i=0; i<m_num_ports; ++i) {
    m_rcw_have_first_transfer[i]        = false;
    m_block_write_transfer_count[i]     = 0;
    m_burst_write_transfer_count[i]     = 0;
    m_first_block_write[i]              = true;
    m_first_burst_write[i]              = true;
    m_first_rcw[i]                      = true;
    m_multi_write_doit[i]               = true;
    m_last_action_time_stamp[i]         = SC_ZERO_TIME - m_recovery_time;
    if (m_p_active_request_info[i]) {
      delete_request_info(m_p_active_request_info[i]);
    }
    if (m_p_active_response[i]) {
      delete m_p_active_response[i];
      m_p_active_response[i] = NULL;
    }
  }

  for (u32 i=0; i<m_num_ports; ++i) {
    while (m_request_fifo[i]->num_available()) {
      request_info *p_info = m_request_fifo[i]->read();
      delete_request_info(p_info);
    }
  }

  if (m_p_script_stream != NULL) {
    m_p_script_stream->reset();
  }

  if (hard_reset) {
    load_initial_values();
  }

  // Cancel notified events
  for (u32 i = 0; i < m_num_ports; i++) {
    m_worker_thread_event[i]->cancel();
  }
  m_script_thread_event.cancel();

  xtsc_reset_processes(m_process_handles);
}



void xtsc_component::xtsc_memory::support_exclusive(bool exclusive) {
  if (m_p_exclusive_script_stream) {
    XTSC_WARN(m_text, "Ignoring call to xtsc_memory::support_exclusive() because \"exclusive_script_file\" is defined.");
  }
  else {
    m_support_exclusive = exclusive;
  }
}



void xtsc_component::xtsc_memory::change_clock_period(u32 clock_period_factor) {
  m_clock_period = sc_get_time_resolution() * clock_period_factor;
  XTSC_INFO(m_text, "change_clock_period(" << clock_period_factor << ") => " << m_clock_period);
  m_statistics_interval_duration = m_clock_period * m_interval_size;
  compute_delays();
}



void xtsc_component::xtsc_memory::handle_response_filters(u32 port, const xtsc_response &response) {
  for (set<u32>::const_iterator i = m_response_watchfilters.begin(); i != m_response_watchfilters.end(); ++i) {
    u32 watchfilter = *i;
    map<u32, watchfilter_info*>::const_iterator j = m_watchfilters.find(watchfilter);
    if (j == m_watchfilters.end()) {
      ostringstream oss;
      oss << "Error: watchfilter #" << watchfilter << " missing from m_watchfilters at line " << __LINE__ << " of " << __FILE__;
      throw xtsc_exception(oss.str());
    }
    watchfilter_info *p_watchfilter_info = j->second;
    if (xtsc_filter_apply_xtsc_response(p_watchfilter_info->m_filter_name, port, response)) {
      m_filtered_response = response;
      p_watchfilter_info->m_event.notify(SC_ZERO_TIME);
      XTSC_INFO(m_text, response << " Watchfilter #" << p_watchfilter_info->m_watchfilter);
    }
  }
}



void xtsc_component::xtsc_memory::compute_delays() {
  // Convert delays from an integer number of periods to sc_time values
  m_read_delay          = m_clock_period * m_memory_parms.get_u32("read_delay");
  m_block_read_delay    = m_clock_period * m_memory_parms.get_u32("block_read_delay");
  m_block_read_repeat   = m_clock_period * m_memory_parms.get_u32("block_read_repeat");
  m_burst_read_delay    = m_clock_period * m_memory_parms.get_u32("burst_read_delay");
  m_burst_read_repeat   = m_clock_period * m_memory_parms.get_u32("burst_read_repeat");
  m_rcw_repeat          = m_clock_period * m_memory_parms.get_u32("rcw_repeat");
  m_rcw_response        = m_clock_period * m_memory_parms.get_u32("rcw_response");
  m_write_delay         = m_clock_period * m_memory_parms.get_u32("write_delay");
  m_block_write_delay   = m_clock_period * m_memory_parms.get_u32("block_write_delay");
  m_block_write_repeat  = m_clock_period * m_memory_parms.get_u32("block_write_repeat");
  m_block_write_response= m_clock_period * m_memory_parms.get_u32("block_write_response");
  m_burst_write_delay   = m_clock_period * m_memory_parms.get_u32("burst_write_delay");
  m_burst_write_repeat  = m_clock_period * m_memory_parms.get_u32("burst_write_repeat");
  m_burst_write_response= m_clock_period * m_memory_parms.get_u32("burst_write_response");
  m_recovery_time       = m_clock_period * m_memory_parms.get_u32("recovery_time");
  m_response_repeat     = m_clock_period * m_memory_parms.get_u32("response_repeat");
}



void xtsc_component::xtsc_memory::reset_fifos() {
  wait(SC_ZERO_TIME);
  for (u32 i=0; i<m_num_ports; ++i) {
    while (m_request_fifo[i]->num_available()) {
      request_info *p_info = m_request_fifo[i]->read();
      delete_request_info(p_info);
    }
  }
}



void xtsc_component::xtsc_memory::man(ostream& os) {
  os << " xtsc_memory supports watchfilters using xtsc_filter kinds of 'xtsc_request', 'xtsc_response', 'xtsc_peek', and 'xtsc_poke'."
     << endl;
}


void xtsc_component::xtsc_memory::execute(const string&                 cmd_line, 
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
    xtsc_address start_address = xtsc_command_argtou64(cmd_line, words, 1);
    u32          num_bytes     = xtsc_command_argtou32(cmd_line, words, 2);
    byte_dump(start_address, num_bytes, res);
  }
  else if (words[0] == "dump_exclusive_monitors") {
    dump_exclusive_monitors(res);
  }
  else if (words[0] == "dump_filtered_request") {
    res << m_filtered_request;
  }
  else if (words[0] == "dump_filtered_response") {
    res << m_filtered_response;
  }
  else if (words[0] == "get_exclusive_monitors_count") {
    res << m_exclusive_monitor_map.size();
  }
  else if (words[0] == "get_total_exclusive_monitors_created") {
    res << m_exclusive_monitors;
  }
  else if (words[0] == "peek") {
    xtsc_address start_address = xtsc_command_argtou64(cmd_line, words, 1);
    u32          num_bytes     = xtsc_command_argtou32(cmd_line, words, 2);
    u8 *buffer = new u8[num_bytes];
    peek(start_address, num_bytes, buffer);
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
    xtsc_address start_address = xtsc_command_argtou64(cmd_line, words, 1);
    u32          num_bytes     = xtsc_command_argtou32(cmd_line, words, 2);
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
      poke(start_address, num_bytes, buffer);
      delete [] buffer;
    }
  }
  else if (words[0] == "reset") {
    reset((words.size() == 1) ? false : xtsc_command_argtobool(cmd_line, words, 1));
  }
  else if (words[0] == "summary") {
    summary(res);
  }
  else if (words[0] == "support_exclusive") {
    if (words.size() == 1) {
      res << m_support_exclusive;
    }
    else {
      support_exclusive(xtsc_command_argtobool(cmd_line, words, 1));
    }
  }
  else if (words[0] == "watchfilter_add") {
    sc_event& event = xtsc_event_get(words[2]);
    res << watchfilter_add(words[1], event);
  }
  else if (words[0] == "watchfilter_dump") {
    watchfilter_dump(res);
  }
  else if (words[0] == "watchfilter_remove") {
    u32 watchfilter = ((words[1] == "*") ? 0xFFFFFFFF : xtsc_command_argtou32(cmd_line, words, 1));
    res << watchfilter_remove(watchfilter);
  }
  else {
    ostringstream oss;
    oss << name() << "::" << __FUNCTION__ << "() called for unknown command '" << cmd_line << "'.";
    throw xtsc_exception(oss.str());
  }

  result << res.str();
}



void xtsc_component::xtsc_memory::connect(xtsc_arbiter& arbiter, u32 port_num) {
  if (port_num >= m_num_ports) {
    ostringstream oss;
    oss << "Invalid port_num=" << port_num << " in connect(): " << endl;
    oss << kind() << " '" << name() << "' has " << m_num_ports << " ports numbered from 0 to " << m_num_ports-1 << endl;
    throw xtsc_exception(oss.str());
  }
  arbiter.m_request_port(*m_request_exports[port_num]);
  (*m_respond_ports[port_num])(arbiter.m_respond_export);
}



u32 xtsc_component::xtsc_memory::connect(xtsc_core& core, const char *memory_port_name, u32 port_num, bool single_connect) {
  if (port_num >= m_num_ports) {
    ostringstream oss;
    oss << "Invalid port_num=" << port_num << " in connect(): " << endl;
    oss << kind() << " '" << name() << "' has " << m_num_ports << " ports numbered from 0 to " << m_num_ports-1 << endl;
    throw xtsc_exception(oss.str());
  }
  xtsc_core::memory_port mem_port = xtsc_core::get_memory_port(memory_port_name, &core);
  u32 width8 = core.get_memory_byte_width(mem_port);
  if (m_width8 && (width8 != m_width8)) {
    ostringstream oss;
    oss << "Memory interface data width mismatch: " << endl;
    oss << kind() << " '" << name() << "' is " << m_width8 << " bytes wide, but '" << memory_port_name << "' interface of xtsc_core '"
        << core.name() << "' is " << width8 << " bytes wide.";
    throw xtsc_exception(oss.str());
  }
  core.get_request_port(memory_port_name)(*m_request_exports[port_num]);
  (*m_respond_ports[port_num])(core.get_respond_export(memory_port_name));
  u32 num_connected = 1;
  // Should we connect other ports of multi-port interface?
  u32 mpc = core.get_multi_port_count(mem_port);
  if (!single_connect && (port_num+mpc <= m_num_ports) && (mpc > 1) && xtsc_core::is_multi_port_zero(mem_port)) {
    bool found_a_connected_port = false;
    for (u32 n=1; n<mpc && !found_a_connected_port; ++n) {
      xtsc_core::memory_port mem_port_n = core.get_nth_multi_port(mem_port, n);
      // Don't connect if our multi-port or the core's multi-port has already been connected.
      // The get_interface() test is not reliable so we'll catch any errors below and carry on (which
      // works for the OSCI simulator--but maybe not for others).
      if (m_request_impl[port_num+n]->is_connected() || core.get_request_port(mem_port_n).get_interface()) {
        found_a_connected_port = true;
        break;
      }
    }
    if (!found_a_connected_port) {
      for (u32 n=1; n<mpc; ++n) {
        xtsc_core::memory_port mem_port_n = core.get_nth_multi_port(mem_port, n);
        sc_port<xtsc_request_if, NSPP>& multi_port = core.get_request_port(mem_port_n);
        try {
          multi_port(*m_request_exports[port_num+n]);
          (*m_respond_ports[port_num+n])(core.get_respond_export(mem_port_n));
          num_connected += 1;
        } catch (...) {
          XTSC_INFO(m_text, "Core '" << core.name() << "' memory multi-port '" << xtsc_core::get_memory_port_name(mem_port_n) <<
                            "' is already bond.");
          if (n > 1) throw;  // Don't ignore failure after 2nd port is connected.
          break;
        }
      }
    }
  }
  return num_connected;
}



void xtsc_component::xtsc_memory::connect(xtsc_dma_engine& dma, u32 port_num) {
  if (port_num >= m_num_ports) {
    ostringstream oss;
    oss << "Invalid port_num=" << port_num << " in connect(): " << endl;
    oss << kind() << " '" << name() << "' has " << m_num_ports << " ports numbered from 0 to " << m_num_ports-1 << endl;
    throw xtsc_exception(oss.str());
  }
  dma.m_request_port(*m_request_exports[port_num]);
  (*m_respond_ports[port_num])(dma.m_respond_export);
}



void xtsc_component::xtsc_memory::connect(xtsc_master& master, u32 port_num) {
  if (port_num >= m_num_ports) {
    ostringstream oss;
    oss << "Invalid port_num=" << port_num << " in connect(): " << endl;
    oss << kind() << " '" << name() << "' has " << m_num_ports << " ports numbered from 0 to " << m_num_ports-1 << endl;
    throw xtsc_exception(oss.str());
  }
  master.m_request_port(*m_request_exports[port_num]);
  (*m_respond_ports[port_num])(master.m_respond_export);
}



u32 xtsc_component::xtsc_memory::connect(xtsc_memory_trace& memory_trace, u32 trace_port, u32 port_num, bool single_connect) {
  u32 trace_ports = memory_trace.get_num_ports();
  if (trace_port >= trace_ports) {
    ostringstream oss;
    oss << "Invalid trace_port=" << trace_port << " in connect(): " << endl;
    oss << memory_trace.kind() << " '" << memory_trace.name() << "' has " << trace_ports << " ports numbered from 0 to "
        << trace_ports-1 << endl;
    throw xtsc_exception(oss.str());
  }
  if (port_num >= m_num_ports) {
    ostringstream oss;
    oss << "Invalid port_num=" << port_num << " in connect(): " << endl;
    oss << kind() << " '" << name() << "' has " << m_num_ports << " ports numbered from 0 to " << m_num_ports-1 << endl;
    throw xtsc_exception(oss.str());
  }

  u32 num_connected = 0;

  while ((trace_port < trace_ports) && (port_num < m_num_ports)) {

    (*memory_trace.m_request_ports[trace_port])(*m_request_exports[port_num]);
    (*m_respond_ports[port_num])(*memory_trace.m_respond_exports[trace_port]);

    port_num += 1;
    trace_port += 1;
    num_connected += 1;

    if (single_connect) break;
    if (trace_port >= trace_ports) break;
    if (port_num >= m_num_ports) break;
  }

  return num_connected;
}



void xtsc_component::xtsc_memory::connect(xtsc_router& router, u32 router_port, u32 port_num) {
  u32 num_slaves = router.get_num_slaves();
  if (router_port >= num_slaves) {
    ostringstream oss;
    oss << "Invalid router_port=" << router_port << " in connect(): " << endl;
    oss << router.kind() << " '" << router.name() << "' has " << num_slaves << " ports numbered from 0 to " << num_slaves-1 << endl;
    throw xtsc_exception(oss.str());
  }
  if (port_num >= m_num_ports) {
    ostringstream oss;
    oss << "Invalid port_num=" << port_num << " in connect(): " << endl;
    oss << kind() << " '" << name() << "' has " << m_num_ports << " ports numbered from 0 to " << m_num_ports-1 << endl;
    throw xtsc_exception(oss.str());
  }
  (*router.m_request_ports[router_port])(*m_request_exports[port_num]);
  (*m_respond_ports[port_num])(*router.m_respond_exports[router_port]);
}



u32 xtsc_component::xtsc_memory::connect(xtsc_pin2tlm_memory_transactor& pin2tlm, u32 tran_port, u32 port_num, bool single_connect) {
  u32 tran_ports = pin2tlm.get_num_ports();
  if (tran_port >= tran_ports) {
    ostringstream oss;
    oss << "Invalid tran_port=" << tran_port << " in connect(): " << endl;
    oss << pin2tlm.kind() << " '" << pin2tlm.name() << "' has " << tran_ports << " ports numbered from 0 to " << tran_ports-1 << endl;
    throw xtsc_exception(oss.str());
  }
  if (port_num >= m_num_ports) {
    ostringstream oss;
    oss << "Invalid port_num=" << port_num << " in connect(): " << endl;
    oss << kind() << " '" << name() << "' has " << m_num_ports << " ports numbered from 0 to " << m_num_ports-1 << endl;
    throw xtsc_exception(oss.str());
  }

  u32 num_connected = 0;

  while ((tran_port < tran_ports) && (port_num < m_num_ports)) {

    (*pin2tlm.m_request_ports[tran_port])(*m_request_exports[port_num]);
    (*m_respond_ports[port_num])(*pin2tlm.m_respond_exports[tran_port]);

    port_num += 1;
    tran_port += 1;
    num_connected += 1;

    if (single_connect) break;
    if (tran_port >= tran_ports) break;
    if (port_num >= m_num_ports) break;
  }

  return num_connected;

}



u32 xtsc_component::xtsc_memory::watchfilter_add(const string& filter_name, sc_event& event) {
  const xtsc_filter& filter = xtsc_filter_get(filter_name);
  const string& filter_kind = filter.get_kind();
  if ((filter_kind != "xtsc_peek") && (filter_kind != "xtsc_poke") && (filter_kind != "xtsc_request") && (filter_kind != "xtsc_response")) {
    ostringstream oss;
    oss << kind() << "::" << __FUNCTION__ << "() does not support filter kind of '" << filter_kind << "'";
    throw xtsc_exception(oss.str());
  }
  watchfilter_info *p_watchfilter_info = new watchfilter_info(filter_kind, filter_name, event);
  u32 watchfilter = xtsc_get_next_watchfilter_number();
  p_watchfilter_info->m_watchfilter = watchfilter;
  m_watchfilters[watchfilter] = p_watchfilter_info;
  if (filter_kind == "xtsc_peek") {
    m_peek_watchfilters.insert(watchfilter);
    m_filter_peeks = true;
  }
  else if (filter_kind == "xtsc_poke") {
    m_poke_watchfilters.insert(watchfilter);
    m_filter_pokes = true;
  }
  else if (filter_kind == "xtsc_request") {
    m_request_watchfilters.insert(watchfilter);
    m_filter_requests = true;
  }
  else if (filter_kind == "xtsc_response") {
    m_response_watchfilters.insert(watchfilter);
    m_filter_responses = true;
  }
  else {
    ostringstream oss;
    oss << "Program Bug: " << kind() << "::" << __FUNCTION__ << "(): Unhandled case of xtsc_filter kind of '" << filter_kind << "'";
    throw xtsc_exception(oss.str());
  }
  return watchfilter;
}



void xtsc_component::xtsc_memory::watchfilter_dump(ostream& os) {
  for (map<u32, watchfilter_info*>::const_iterator i = m_watchfilters.begin(); i != m_watchfilters.end(); ++i) {
    os << i->first << " " << i->second->m_filter_kind << " " << i->second->m_filter_name << endl;
  }
}



u32 xtsc_component::xtsc_memory::watchfilter_remove(u32 watchfilter) {
  if (watchfilter == 0xFFFFFFFF) {
    u32 cnt = m_peek_watchfilters    .size() +
              m_poke_watchfilters    .size() + 
              m_request_watchfilters .size() + 
              m_response_watchfilters.size();

    m_peek_watchfilters    .clear();
    m_poke_watchfilters    .clear();
    m_request_watchfilters .clear();
    m_response_watchfilters.clear();

    m_filter_peeks     = false;
    m_filter_pokes     = false;
    m_filter_requests  = false;
    m_filter_responses = false;

    return cnt;
  }
  map<u32, watchfilter_info*>::iterator i = m_watchfilters.find(watchfilter);
  if (i == m_watchfilters.end()) {
    ostringstream oss;
    oss << this->kind() << "::" << __FUNCTION__ << "(): '" << name() << "' does not have a watchfilter of " << watchfilter << "."
        << "  Here are the existing watchfilters:" << endl;
    watchfilter_dump(oss);
    throw xtsc_exception(oss.str());
  }
  if (i->second->m_filter_kind == "xtsc_peek") {
    m_peek_watchfilters.erase(watchfilter);
    m_filter_peeks = (m_peek_watchfilters.size() != 0);
  }
  else if (i->second->m_filter_kind == "xtsc_poke") {
    m_poke_watchfilters.erase(watchfilter);
    m_filter_pokes = (m_poke_watchfilters.size() != 0);
  }
  else if (i->second->m_filter_kind == "xtsc_request") {
    m_request_watchfilters.erase(watchfilter);
    m_filter_requests = (m_request_watchfilters.size() != 0);
  }
  else if (i->second->m_filter_kind == "xtsc_response") {
    m_response_watchfilters.erase(watchfilter);
    m_filter_responses = (m_response_watchfilters.size() != 0);
  }
  else {
    ostringstream oss;
    oss << "Program Bug: " << this->kind() << "::" << __FUNCTION__ << "(): Unhandled case of filter kind of '" << i->second->m_filter_kind
        << "'";
    throw xtsc_exception(oss.str());
  }
  delete i->second;
  m_watchfilters.erase(i);
  return 1;
}



void xtsc_component::xtsc_memory::setup_false_error_responses(xtsc_response::status_t status, u32 request_mask, u32 fail_percentage) {
  m_fail_status         = status;
  m_fail_request_mask   = request_mask;
  m_fail_percentage     = fail_percentage;
  compute_let_through();
}



void xtsc_component::xtsc_memory::compute_let_through() {
  if (m_fail_percentage > 100) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': ";
    oss << "fail_percentage of " << m_fail_percentage << " is out of range (1-100).";
    throw xtsc_exception(oss.str());
  }
  u64 max = 0xffffffff;
  m_let_through = static_cast<u32>(max - ((max * m_fail_percentage) / 100));
  XTSC_DEBUG(m_text, "m_fail_percentage=" << m_fail_percentage << " => m_let_through=0x" << hex << m_let_through);
}



bool xtsc_component::xtsc_memory::set_exclusive_from_script(xtsc_address address8, xtsc_response& response) {
  // get_words is guaranteed to return a line because wraparound is true
  m_exclusive_line_num = m_p_exclusive_script_stream->get_words(m_exclusive_words, m_exclusive_line, true, &m_current_exclusive_file);
  bool okay      = true;
  bool bad_addr  = false;
  bool exclusive = true;
  const string &word = m_exclusive_words[0];
  if (word == "0") {
    exclusive = false;
  }
  else if (word == "1") {
    exclusive = true;
  }
  else {
    okay = false;
  }
  if (m_exclusive_words.size() >= 2) {
    xtsc_address address = address8;
    try { address = xtsc_strtou64(m_exclusive_words[1]); }
    catch (...) {
      okay     = false;
      bad_addr = true;
    }
    if (okay && (address != address8)) {
      ostringstream oss;
      oss << kind() << " '" << name() << "': ";
      oss << "Address mismatch at " << m_current_exclusive_file << ":" << m_exclusive_line_num
          << " defined by xtsc_memory_parms \"exclusive_script_file\"." << endl;
      oss << "Request address is:  0x" << hex << address8 << endl;
      oss << "But script file has: " << m_exclusive_words[1];
      throw xtsc_exception(oss.str());
    }
  }
  if (!okay) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': ";
    oss << "Bad content found at " << m_current_exclusive_file << ":" << m_exclusive_line_num
        << " defined by xtsc_memory_parms \"exclusive_script_file\"." << endl;
    oss << "Expected line format is:  0|1 [<Address>]" << endl;
    oss << "But found:  " << m_exclusive_line;
    if (bad_addr) {
      oss << endl << "(Cannot convert '" << m_exclusive_words[1] << "' to a number)";
    }
    throw xtsc_exception(oss.str());
  }
  response.set_exclusive_ok(exclusive);
  XTSC_INFO(m_text, response << " (" << m_exclusive_line_num << ": " << m_exclusive_line << ")");
  return exclusive;
}



void xtsc_component::xtsc_memory::create_exclusive_monitor(xtsc_request &request, xtsc_address address8, u32 size8) {
  if (size8 > 128) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': " << " Exclusive read of " << size8 << " bytes exceeds the 128-byte limit: " << request;
    throw xtsc_exception(oss.str());
  }
  u64 tran_id = request.get_transaction_id();
  map<u64, address_range>::iterator i = m_exclusive_monitor_map.find(tran_id);
  if (i != m_exclusive_monitor_map.end()) {
    address_range& range = i->second;
    XTSC_INFO(m_text, request << ": Delete monitor 0x" << hex << tran_id << ":0x" << range.first << "-0x" << range.second);
    m_exclusive_monitor_map.erase(i);
  }
  address_range range = make_pair(address8, address8 + size8 - 1);
  m_exclusive_monitor_map[tran_id] = range;
  XTSC_INFO(m_text, request << ": Add monitor 0x" << hex << tran_id << ":0x" << range.first << "-0x" << range.second);
  m_exclusive_monitors += 1;
}



// Note: This method does not consider which memory port the requests came in on.  If user wants that they can get the equivalent effect
// by attaching dummy arbiters to each memory port.  Each arbiter has as many masters ports as this memory model has ports and a different
// master port of each arbiter is used for the real master (other master ports have a dummy xtsc_master driving them).
bool xtsc_component::xtsc_memory::check_exclusive_write(xtsc_request &request, xtsc_address address8, u32 size8, xtsc_response& response) {
  u64 tran_id = request.get_transaction_id();
  map<u64, address_range>::iterator i = m_exclusive_monitor_map.find(tran_id);
  bool doit = (i != m_exclusive_monitor_map.end());
  
  if (doit) {
    address_range& range = i->second;
    xtsc_address wrt_beg = address8;
    xtsc_address wrt_end = address8 + size8 - 1;
    xtsc_address mon_beg = range.first;
    xtsc_address mon_end = range.second;
    if ((wrt_beg < mon_beg) || (wrt_end > mon_end)) {
      ostringstream oss;
      oss << "Exclusive monitor 0x" << hex << tran_id << ":0x" << mon_beg << "-0x" << mon_end
          << " has address range which contradicts exclusive write tran_id=0x" << tran_id << ":0x" << wrt_beg << "-0x" << wrt_end << ": " << request
          << ": Typically this happens when a master (Xtensa) does a L32EX to one address followed by a S32EX to a different address.";
      static bool warned = false;
      if (warned) { XTSC_INFO(m_text, oss.str()); }
      else        { XTSC_WARN(m_text, oss.str()); }
      warned = true;
      doit   = false;
    }
  }
  response.set_exclusive_ok(doit);
  return doit;
}



void xtsc_component::xtsc_memory::check_exclusive_monitors_against_write(xtsc_request &request, xtsc_address address8, u32 size8) {
  XTSC_DEBUG(m_text, "check_exclusive_monitors_against_write: monitor count = " << m_exclusive_monitor_map.size());
  xtsc_address wrt_beg = address8;
  xtsc_address wrt_end = address8 + size8 - 1;
  for (map<u64, address_range>::iterator i = m_exclusive_monitor_map.begin(); i != m_exclusive_monitor_map.end();) {
    address_range& range = i->second;
    xtsc_address mon_beg = range.first;
    xtsc_address mon_end = range.second;
    XTSC_DEBUG(m_text, request << ": Check monitor 0x" << hex << i->first << ":0x" << range.first << "-0x" << range.second);
    if ((wrt_end >= mon_beg) && (wrt_beg <= mon_end)) {
      XTSC_INFO(m_text, request << ": Delete monitor 0x" << hex << i->first << ":0x" << range.first << "-0x" << range.second);
      m_exclusive_monitor_map.erase(i++);
    }
    else {
      ++i;
    }
  }
}



xtsc_component::xtsc_memory::request_type_t xtsc_component::xtsc_memory::get_request_type(const xtsc_request& request, u32 port_num) {
  switch (request.get_type()) {
    case xtsc_request::READ:        return REQ_READ;
    case xtsc_request::BLOCK_READ:  return REQ_BLOCK_READ;
    case xtsc_request::BURST_READ:  return REQ_BURST_READ;
    case xtsc_request::WRITE:       return REQ_WRITE;
    case xtsc_request::BLOCK_WRITE: return static_cast<request_type_t>(REQ_BLOCK_WRITE_1 << m_block_write_transfer_count[port_num]);
    case xtsc_request::BURST_WRITE: return static_cast<request_type_t>(REQ_BURST_WRITE_1 << m_burst_write_transfer_count[port_num]);
    case xtsc_request::RCW:         return static_cast<request_type_t>(REQ_RCW_1 << (request.get_last_transfer() ? 1 : 0));
    default: {
      ostringstream oss;
      oss << kind() << " '" << name() << "': ";
      oss << "Invalid xtsc_request::type_t in xtsc_memory::get_request_type().";
      throw xtsc_exception(oss.str());
    }
  }
}



bool xtsc_component::xtsc_memory::do_nacc_failure(const xtsc_request& request, u32 port_num) {
  return ((m_fail_request_mask)                                         &&
          (get_request_type(request, port_num) & m_fail_request_mask)   &&
          (m_fail_status == xtsc_response::RSP_NACC)                    &&
          (random() >= m_let_through));
}



xtsc_response::status_t xtsc_component::xtsc_memory::get_status_for_testing_failures(request_info* p_request_info, u32 port_num, bool& list) {
  if (p_request_info->m_status != xtsc_response::RSP_OK) {
    if (m_last && !p_request_info->m_request.get_last_transfer()) return xtsc_response::RSP_OK;
    list = false;
    xtsc_request::type_t type = p_request_info->m_request.get_type();
    if ((type == xtsc_request::BLOCK_READ) || (type == xtsc_request::BURST_READ)) {
      list = p_request_info->m_list;
    }
    return p_request_info->m_status;
  }
  list = false;
  return ((m_fail_request_mask)                                                         &&
          (get_request_type(p_request_info->m_request, port_num) & m_fail_request_mask) &&
          (m_fail_status != xtsc_response::RSP_NACC)                                    &&
          (random() >= m_let_through)
         ) ? m_fail_status : xtsc_response::RSP_OK;
}


bool xtsc_component::xtsc_memory::check_non_secure_access(xtsc_request* p_request) {
  XTSC_TRACE(m_text, "check_non_secure_access : " << *p_request);
  // Determine whether or not xtsc request is inside a "secure_address_range" block:
  // For a non-secure access, flag it
  xtsc_address  address8                = p_request->get_byte_address();
  u32           size8                   = p_request->get_byte_size();
  u32           num_transfers           = p_request->get_num_transfers();
  bool          axi                     = p_request->is_axi_protocol();
  u32           total_bytes_to_transfer = num_transfers * size8;
  xtsc_address  start_address8          = (address8 / total_bytes_to_transfer) * 
                                          total_bytes_to_transfer;
  xtsc_address  end_address8            = start_address8 + total_bytes_to_transfer - 1;

  for (u32 i=0; i<m_secure_address_range.size(); i+=2) {
    xtsc_address begin   = m_secure_address_range[i+0];
    xtsc_address end     = m_secure_address_range[i+1];
    XTSC_DEBUG(m_text, "start_address8 : " << std::hex << start_address8 << "end_address8 : " << end_address8 
                       << "begin : " << begin << "end : " << end << "axi : " << axi << endl);
    if (((start_address8 <= begin) && (end_address8 >= begin)) || 
        ((start_address8 >= begin) && (end_address8 <= end  )) || 
        ((start_address8 <= end  ) && (end_address8 >= end  ))   ){
      if ((!axi && (p_request->get_pif_attribute() & BIT_11)) || 
          ( axi && (p_request->get_prot()          & BIT_1 )) ){ // NSM Access
        u32 n = xtsc_address_nibbles();
        XTSC_WARN(m_text, "Attempted non secure access: addr=0x" << hex << setfill('0') << setw(n) << address8 <<
                                    " secure=[0x" << setw(n) << begin << ",0x" << setw(n) << end << "]");
        return true;
      }
    }
  }
  return false;
}


// When multi-ported delay writes of all kinds to ensure simultaneous reads of all kinds return old data
void xtsc_component::xtsc_memory::worker_thread(void) {

  // Get the port number for this "instance" of worker_thread
  u32 port_num = m_next_port_num++;

  try {
    // Reset internal fifos
    if (port_num == 0)
      reset_fifos();

    while (true) {
      wait(*m_worker_thread_event[port_num]);
      while (m_request_fifo[port_num]->num_available()) {
        bool delta_delay = false;
        // Accept this one as our current transaction
        m_request_fifo[port_num]->nb_read(m_p_active_request_info[port_num]);
        XTSC_DEBUG(m_text, "worker_thread() got: " << m_p_active_request_info[port_num]->m_request);
        // Determine delay
        sc_time delay = SC_ZERO_TIME;
        switch (m_p_active_request_info[port_num]->m_request.get_type()) {
          case xtsc_request::READ:         
            delay  = m_read_delay;
            m_statistics[port_num]->m_active_stats_interval->m_num_read_requests++;
            break;
          case xtsc_request::BLOCK_READ:   
            delay  = m_block_read_delay;
            m_statistics[port_num]->m_active_stats_interval->m_num_read_requests++;
            break;
          case xtsc_request::BURST_READ:   
            delay  = m_burst_read_delay;
            m_statistics[port_num]->m_active_stats_interval->m_num_read_requests++;
            break;
          case xtsc_request::RCW:          
            if (m_read_only) {
              ostringstream oss;
              oss << "Read-only xtsc_memory '" << name() << "' doesn't support transaction: "
                  << m_p_active_request_info[port_num]->m_request;
              throw xtsc_exception(oss.str());
            }
            delay  = (m_first_rcw[port_num] ?  m_rcw_repeat : m_rcw_response);
            m_statistics[port_num]->m_active_stats_interval->m_num_write_requests += (m_first_rcw[port_num] ? 0 : 1);
            m_first_rcw[port_num] = m_p_active_request_info[port_num]->m_request.get_last_transfer();
            break;
          case xtsc_request::WRITE:        
            if (m_read_only) {
              ostringstream oss;
              oss << "Read-only xtsc_memory '" << name() << "' doesn't support transaction: "
                  << m_p_active_request_info[port_num]->m_request;
              throw xtsc_exception(oss.str());
            }
            // To model the dual load/store unit requirement that a simultaneous read and write to
            // the same address must return the old data for the read (NOT the new data being written
            // by the write), we delay WRITE transactions for a delta cycle when we are multi-ported.
            delta_delay = (m_num_ports > 1);
            delay  = m_write_delay;
            m_statistics[port_num]->m_active_stats_interval->m_num_write_requests++;
            break;
          case xtsc_request::BLOCK_WRITE:  {
            if (m_read_only) {
              ostringstream oss;
              oss << "Read-only xtsc_memory '" << name() << "' doesn't support transaction: "
                  << m_p_active_request_info[port_num]->m_request;
              throw xtsc_exception(oss.str());
            }
            bool last = m_p_active_request_info[port_num]->m_request.get_last_transfer();
            delta_delay = (m_num_ports > 1);
            delay  = (m_first_block_write[port_num] ?  m_block_write_delay : (last ? m_block_write_response : m_block_write_repeat));
            m_statistics[port_num]->m_active_stats_interval->m_num_write_requests += (m_first_block_write[port_num] ? 1 : 0);
            m_first_block_write[port_num] = last;
            break;
          }
          case xtsc_request::BURST_WRITE:  {
            // TODO: Ignore AXI MUEvict???
            if (m_read_only) {
              ostringstream oss;
              oss << "Read-only xtsc_memory '" << name() << "' doesn't support transaction: "
                  << m_p_active_request_info[port_num]->m_request;
              throw xtsc_exception(oss.str());
            }
            bool last = m_p_active_request_info[port_num]->m_request.get_last_transfer();
            delta_delay = (m_num_ports > 1);
            delay  = (m_first_burst_write[port_num] ?  m_burst_write_delay : (last ? m_burst_write_response : m_burst_write_repeat));
            m_statistics[port_num]->m_active_stats_interval->m_num_write_requests += (m_first_burst_write[port_num] ? 1 : 0);
            m_first_burst_write[port_num] = last;
            break;
          }
          default: {
            ostringstream oss;
            oss << kind() << " '" << name() << "': ";
            oss << "Invalid xtsc_request::type_t in nb_request().";
            throw xtsc_exception(oss.str());
          }
        }
        if (m_delay_from_receipt) {
          // net => No Earlier Than time
          sc_time receipt_net     = m_p_active_request_info[port_num]->m_time_stamp + delay;
          sc_time last_action_net = m_last_action_time_stamp[port_num] + m_recovery_time;
          sc_time latest_net      = (receipt_net > last_action_net) ? receipt_net : last_action_net;
          sc_time now             = sc_time_stamp();
          delay = (latest_net <= now) ? SC_ZERO_TIME : (latest_net - now);
        }
        XTSC_DEBUG(m_text, "worker_thread() doing wait for " << delay);
        wait(delay);
        if (delta_delay) {
          wait(SC_ZERO_TIME);
        }
        do_active_request(port_num);
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



void xtsc_component::xtsc_memory::do_active_request(u32 port_num) {
  bool drop_on_floor = false;
  xtsc_request *p_request       = &m_p_active_request_info[port_num]->m_request;
  m_p_active_response[port_num] = new xtsc_response(m_p_active_request_info[port_num]->m_request);
  bool list = false;
  xtsc_response::status_t drop_status;
  xtsc_response::status_t status = get_status_for_testing_failures(m_p_active_request_info[port_num], port_num, list);
  if (m_check_alignment) {
    u32 size8 = p_request->get_byte_size();
    bool aligned = ((p_request->get_byte_address() % size8) == 0);
    bool powerof2 = (size8==1 || size8==2 || size8==4 || size8==8 || size8==16 || size8==32 || size8==64);
    if (!aligned || !powerof2) {
      if (p_request->get_last_transfer()) {
        status = xtsc_response::RSP_ADDRESS_ERROR;
        XTSC_DEBUG(m_text, *p_request << " (RSP_ADDRESS_ERROR: size8=" << size8 << " powerof2=" << boolalpha <<
                           powerof2 << " aligned=" << aligned << ")");
      }
      else {
        drop_on_floor = true;
        drop_status  = xtsc_response::RSP_ADDRESS_ERROR;
      }
    }
  }
  if (m_filter_requests) {
    for (set<u32>::const_iterator i = m_request_watchfilters.begin(); i != m_request_watchfilters.end(); ++i) {
      u32 watchfilter = *i;
      map<u32, watchfilter_info*>::const_iterator j = m_watchfilters.find(watchfilter);
      if (j == m_watchfilters.end()) {
        ostringstream oss;
        oss << "Error: watchfilter #" << watchfilter << " missing from m_watchfilters at line " << __LINE__ << " of " << __FILE__;
        throw xtsc_exception(oss.str());
      }
      watchfilter_info *p_watchfilter_info = j->second;
      if (xtsc_filter_apply_xtsc_request(p_watchfilter_info->m_filter_name, port_num, m_p_active_request_info[port_num]->m_request)) {
        m_filtered_request = m_p_active_request_info[port_num]->m_request;
        p_watchfilter_info->m_event.notify(SC_ZERO_TIME);
        XTSC_INFO(m_text, m_p_active_request_info[port_num]->m_request << " Watchfilter #" << p_watchfilter_info->m_watchfilter);
      }
    }
  }
  if (check_non_secure_access(p_request)) {
    if (p_request->get_last_transfer()) {
      status = xtsc_response::RSP_DATA_ERROR;
      XTSC_WARN(m_text, *p_request << " RSP_DATA_ERROR: Non-Secure Access to a secured address range");
    } else {
      drop_on_floor = true;
      drop_status   = xtsc_response::RSP_DATA_ERROR;
    }
  }

  if (drop_on_floor) {
    XTSC_INFO(m_text, "Dropping " << xtsc_response::get_status_name(drop_status) << ": " << m_p_active_request_info[port_num]->m_request);
    do_block_write_transfer_count(p_request, port_num, p_request->get_last_transfer());
    do_burst_write_transfer_count(p_request, port_num, p_request->get_last_transfer());
  }
  else if (!list && (status != xtsc_response::RSP_OK)) {
    m_p_active_response[port_num]->set_status(status);
    // Why this check only here? This condition will fail for requests with dropping transfers.
    // We can put it inside if condtion also. Check for BURST_WRITE also missing
    do_block_write_transfer_count(p_request, port_num, p_request->get_last_transfer());
    do_burst_write_transfer_count(p_request, port_num, p_request->get_last_transfer());
    u32 num_responses = (p_request->is_axi_protocol() && p_request->is_read()) ? p_request->get_num_transfers() : 1;
    for (u32 transfer_num = 1; transfer_num <= num_responses; ++transfer_num) {
      m_p_active_response[port_num]->set_transfer_number(transfer_num);
      m_p_active_response[port_num]->set_last_transfer(transfer_num == num_responses);
      send_response(port_num, false);
      if (!m_immediate_timing && (transfer_num < num_responses)) {
        wait(m_burst_read_repeat);
      }
    }
  }
  else {
    switch (m_p_active_request_info[port_num]->m_request.get_type()) {
      case xtsc_request::READ: {
        do_read(port_num);
        break;
      }
      case xtsc_request::BLOCK_READ: {
        do_block_read(port_num);
        break;
      }
      case xtsc_request::BURST_READ: {
        do_burst_read(port_num);
        break;
      }
      case xtsc_request::RCW: {
        do_rcw(port_num);
        break;
      }
      case xtsc_request::WRITE: {
        do_write(port_num);
        break;
      }
      case xtsc_request::BLOCK_WRITE: {
        do_block_write(port_num);
        break;
      }
      case xtsc_request::BURST_WRITE: {
        do_burst_write(port_num);
        break;
      }
      default: {
        ostringstream oss;
        oss << kind() << " '" << name() << "': ";
        oss << "Program Bug: Unrecognized xtsc_request::type_t in xtsc_memory::worker_thread.";
        throw xtsc_exception(oss.str());
      }
    }
  }
  m_last_action_time_stamp[port_num] = sc_time_stamp();
  delete_request_info(m_p_active_request_info[port_num]);
  delete m_p_active_response[port_num];
  m_p_active_response[port_num] = NULL;
}



void xtsc_component::xtsc_memory::do_read(u32 port_num) {
  xtsc_request       *p_request     = &m_p_active_request_info[port_num]->m_request;
  xtsc_address        address8      = p_request->get_byte_address();
  u32                 size8         = p_request->get_byte_size();
  u8                 *buffer        = m_p_active_response[port_num]->get_buffer();
  if (m_width8 && (size8 > m_width8)) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': ";
    oss << "Received READ tag=" << p_request->get_tag() << " with byte size of " << size8 << " (exceeds \"byte_width\" of "
        << m_width8 << ")";
    throw xtsc_exception(oss.str());
  }
  if (size8 > xtsc_max_bus_width8) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': ";
    oss << "Received READ tag=" << p_request->get_tag() << " with byte size of " << size8 << " (exceeds xtsc_max_bus_width8 of "
        << xtsc_max_bus_width8 << ")";
    throw xtsc_exception(oss.str());
  }
  if (p_request->get_exclusive()) {
    if (m_p_exclusive_script_stream) {
      m_p_active_response[port_num]->set_exclusive_ok(true);
    }
    else if (m_support_exclusive) {
      create_exclusive_monitor(*p_request, address8, size8);
      m_p_active_response[port_num]->set_exclusive_ok(true);
      if (m_host_mutex) { m_p_memory->lock(address8); }
    }
  }
  u32 page = get_page(address8);
  u32 mem_offset = get_page_offset(address8);
  memcpy(buffer, m_p_memory->m_page_table[page]+mem_offset, size8);
  m_statistics[port_num]->m_active_stats_interval->m_num_read_bytes_xfered += size8;
  if (xtsc_is_text_logging_enabled() && m_text.isEnabledFor(DEBUG_LOG_LEVEL)) {
    u32 mem_offset  = get_page_offset(address8);
    ostringstream oss;
    xtsc_hex_dump(size8, (m_p_memory->m_page_table[page]+mem_offset), oss);
    XTSC_DEBUG(m_text, m_p_active_request_info[port_num]->m_request << "= " << oss.str());
  }
  send_response(port_num, m_p_memory->m_log_data_binary);
}



void xtsc_component::xtsc_memory::do_block_read(u32 port_num) {
  xtsc_request *p_request       = &m_p_active_request_info[port_num]->m_request;
  u8           *buffer          = m_p_active_response[port_num]->get_buffer();
  u32           num_transfers   = p_request->get_num_transfers();
  u32           size8           = p_request->get_byte_size();
  u32           total_size8     = size8 * num_transfers;
  bool          list            = m_p_active_request_info[port_num]->m_list;
  if (m_width8 && (size8 != m_width8)) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': ";
    oss << "Received BLOCK_READ tag=" << p_request->get_tag() << " with byte size of " << size8
        << " (contradicts \"byte_width\" of " << m_width8 << ")";
    throw xtsc_exception(oss.str());
  }
  if (size8 > xtsc_max_bus_width8) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': ";
    oss << "Received BLOCK_READ tag=" << p_request->get_tag() << " with byte size of " << size8 << " (exceeds xtsc_max_bus_width8 of "
        << xtsc_max_bus_width8 << ")";
    throw xtsc_exception(oss.str());
  }
  if (p_request->get_exclusive()) {
    if (m_p_exclusive_script_stream) {
      m_p_active_response[port_num]->set_exclusive_ok(true);
    }
    else if (m_support_exclusive) {
      create_exclusive_monitor(*p_request, p_request->get_byte_address(), total_size8);
      m_p_active_response[port_num]->set_exclusive_ok(true);
    }
  }
  for (u32 transfer_count = 0; transfer_count < num_transfers; ++transfer_count) {
    if (!m_immediate_timing && transfer_count) {
      wait(m_block_read_repeat);
    }
    m_p_active_response[port_num]->set_transfer_number(transfer_count+1);
    bool last_transfer   = ((transfer_count+1) == num_transfers);
    m_p_active_response[port_num]->set_last_transfer(last_transfer);
    if (list) {
      xtsc_response::status_t status = xtsc_response::RSP_OK;
      if (transfer_count < m_response_list.size()) {
        status = m_response_list[transfer_count];
      }
      m_p_active_response[port_num]->set_status(status);
    }
    // Adjust address for this transfer.  Include wrap-around for Critical Word First support.
    xtsc_address address8 = p_request->get_byte_address();
    xtsc_address mask = total_size8-1;
    address8 = (address8 & ~mask) | (((address8 & mask) + transfer_count * size8) % total_size8);
    XTSC_DEBUG(m_text, "in do_block_read for tag=" << p_request->get_tag() << " xfer #" << 
                       (transfer_count+1) << "/" << num_transfers);
    u32 page = get_page(address8);
    u32 mem_offset = get_page_offset(address8);
    memcpy(buffer, m_p_memory->m_page_table[page]+mem_offset, size8);
    m_statistics[port_num]->m_active_stats_interval->m_num_read_bytes_xfered += size8;
    if (xtsc_is_text_logging_enabled() && m_text.isEnabledFor(DEBUG_LOG_LEVEL)) {
      u32 mem_offset  = get_page_offset(address8);
      ostringstream oss;
      xtsc_hex_dump(size8, (m_p_memory->m_page_table[page]+mem_offset), oss);
      XTSC_DEBUG(m_text, "BLOCK_READ" << (last_transfer ? "*" : " ") << " tag=" << m_p_active_response[port_num]->get_tag() <<
                         " [0x" << hex << setfill('0') << setw(xtsc_address_nibbles()) << address8 << setfill(' ') << "]= " << oss.str());
    }
    send_response(port_num, m_p_memory->m_log_data_binary);
  }
}



void xtsc_component::xtsc_memory::do_burst_read(u32 port_num) {
  xtsc_request *p_request       = &m_p_active_request_info[port_num]->m_request;
  u8           *buffer          = m_p_active_response[port_num]->get_buffer();
  u32           num_transfers   = p_request->get_actual_transfers();
  u32           size8           = p_request->get_byte_size();
  u32           total_size8     = size8 * num_transfers; 
  bool          list            = m_p_active_request_info[port_num]->m_list;
  bool          axi             = p_request->is_axi_protocol();
  bool          cache_maint     = false;
  bool          fixed           = false;
  bool          wrap            = false;
  xtsc_address  address8        = p_request->get_byte_address();
  xtsc_address  lower_wrap_addr = 0;
  xtsc_address  upper_wrap_addr = 0;
  if (axi) {
    xtsc_request::burst_t burst = p_request->get_burst_type();
    cache_maint     = p_request->is_cache_maintenance();
    fixed           = (burst == xtsc_request::FIXED);
    wrap            = (burst == xtsc_request::WRAP);
    lower_wrap_addr = (address8 / total_size8) * total_size8;
    upper_wrap_addr = lower_wrap_addr + total_size8;
  }
  if (!axi && m_width8 && (size8 != m_width8)) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': ";
    oss << "Received BURST_READ tag=" << p_request->get_tag() << " with byte size of " << size8
        << " (contradicts \"byte_width\" of " << m_width8 << ")";
    throw xtsc_exception(oss.str());
  }
  if (axi && m_width8 && (size8 > m_width8)) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': ";
    oss << "Received BURST_READ tag=" << p_request->get_tag() << " with byte size of " << size8
        << " (exceeds \"byte_width\" of " << m_width8 << ")";
    throw xtsc_exception(oss.str());
  }
  if (size8 > xtsc_max_bus_width8) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': ";
    oss << "Received BURST_READ tag=" << p_request->get_tag() << " with byte size of " << size8 << " (exceeds xtsc_max_bus_width8 of "
        << xtsc_max_bus_width8 << ")";
    throw xtsc_exception(oss.str());
  }
  if (p_request->get_exclusive()) {
    if (axi) {
      if (m_p_exclusive_script_stream) {
        m_p_active_response[port_num]->set_exclusive_ok(true);
      }
      else if (m_support_exclusive) {
        create_exclusive_monitor(*p_request, address8, total_size8);
        m_p_active_response[port_num]->set_exclusive_ok(true);
      }
    }
    else {
      ostringstream oss;
      oss << kind() << " '" << name() << "': Received illegal exclusive request: " << *p_request;
      throw xtsc_exception(oss.str());
    }
  }
  for (u32 transfer_count = 0; transfer_count < num_transfers; ++transfer_count) {
    if (!m_immediate_timing && transfer_count) {
      wait(m_burst_read_repeat);
    }
    m_p_active_response[port_num]->set_transfer_number(transfer_count+1);
    bool last_transfer   = ((transfer_count+1) == num_transfers);
    m_p_active_response[port_num]->set_last_transfer(last_transfer);
    if (axi) {
      m_p_active_response[port_num]->set_is_shared(m_is_shared);
      m_p_active_response[port_num]->set_pass_dirty(m_pass_dirty);
    }
    if (list) {
      xtsc_response::status_t status = xtsc_response::RSP_OK;
      if (transfer_count < m_response_list.size()) {
        status = m_response_list[transfer_count];
      }
      m_p_active_response[port_num]->set_status(status);
    }
    XTSC_DEBUG(m_text, "in do_burst_read for tag=" << p_request->get_tag() << " xfer #" << 
                       (transfer_count+1) << "/" << num_transfers);
    u32 page = get_page(address8);
    u32 mem_offset = get_page_offset(address8);
    if (!cache_maint) {
      memcpy(buffer, m_p_memory->m_page_table[page]+mem_offset, size8);
      m_statistics[port_num]->m_active_stats_interval->m_num_read_bytes_xfered += size8;
    }
    if (xtsc_is_text_logging_enabled() && m_text.isEnabledFor(DEBUG_LOG_LEVEL)) {
      ostringstream oss;
      if (!cache_maint) {
        xtsc_hex_dump(size8, (m_p_memory->m_page_table[page]+mem_offset), oss);
      }
      XTSC_DEBUG(m_text, "BURST_READ" << (last_transfer ? "*" : " ") << " tag=" << m_p_active_response[port_num]->get_tag() <<
                         " [0x" << hex << setfill('0') << setw(xtsc_address_nibbles()) << address8 << setfill(' ') << "]= " << oss.str());
    }
    send_response(port_num, m_p_memory->m_log_data_binary);
    // Adjust address for next transfer.
    if (!fixed) {
      address8 += size8;
      if (wrap) {
        if (address8 >= upper_wrap_addr ) {
          address8 -= total_size8;
        }
      }
    }
  }
}



void xtsc_component::xtsc_memory::do_rcw(u32 port_num) {
  xtsc_request *p_request = &m_p_active_request_info[port_num]->m_request;
  xtsc_address  address8  = p_request->get_byte_address();
  u8           *buffer    = p_request->get_buffer();
  u32           page      = get_page(address8);
  u32           size8     = p_request->get_byte_size();
  if (p_request->get_exclusive()) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': Received illegal exclusive request: " << *p_request;
    throw xtsc_exception(oss.str());
  }
  if (!m_rcw_have_first_transfer[port_num]) {
    if (m_host_mutex) { m_p_memory->lock(address8); }
    m_rcw_have_first_transfer[port_num] = true;
    memcpy(&m_rcw_compare_data[port_num*xtsc_max_bus_width8], buffer, size8);
    if (xtsc_is_text_logging_enabled() && m_text.isEnabledFor(DEBUG_LOG_LEVEL)) {
      ostringstream oss;
      xtsc_hex_dump(size8, buffer, oss);
      XTSC_DEBUG(m_text, "RCW  tag=" << p_request->get_tag() << " Compare address 0x" << hex << 
                         setfill('0') << setw(xtsc_address_nibbles()) << address8 << setfill(' ') << " for bytes: " << oss.str());
    }
  }
  else {
    xtsc_byte_enables byte_enables = p_request->get_byte_enables();
    u32 mem_offset  = get_page_offset(address8);
    u8 *response_buffer = m_p_active_response[port_num]->get_buffer();
    memcpy(response_buffer, m_p_memory->m_page_table[page]+mem_offset, size8);
    m_statistics[port_num]->m_active_stats_interval->m_num_read_bytes_xfered+=size8;
    bool compare_data_matches = true;
    xtsc_byte_enables bytes = byte_enables;
    for (u32 i = 0; i<size8; ++i) {
      if (bytes & 0x1) {
        if (m_rcw_compare_data[port_num*xtsc_max_bus_width8+i] != *(m_p_memory->m_page_table[page]+mem_offset)) {
          compare_data_matches = false;
          break;
        }
      }
      bytes >>= 1;
      mem_offset += 1;
    }
    if (xtsc_is_text_logging_enabled() && m_text.isEnabledFor(DEBUG_LOG_LEVEL)) {
      mem_offset  = get_page_offset(address8);
      ostringstream oss1;
      xtsc_hex_dump(size8, m_p_memory->m_page_table[page]+mem_offset, oss1);
      if (compare_data_matches) {
        ostringstream oss2;
        xtsc_hex_dump(size8, buffer, oss2);
        XTSC_DEBUG(m_text, "RCW* tag=" << p_request->get_tag() << " bytes " << oss1.str() <<
                           " replaced with " << oss2.str() << " using byte_enables=0x" << hex << byte_enables);
      }
      else {
        XTSC_DEBUG(m_text, "RCW* tag=" << p_request->get_tag() << " bytes " << oss1.str() <<
                           " not replaced using byte_enables=0x" << hex << byte_enables);
      }
    }
    bytes = byte_enables;
    if (compare_data_matches) {
      mem_offset  = get_page_offset(address8);
      xtsc_byte_enables all_be = ((size8 == 64) ? 0xFFFFFFFFFFFFFFFFull : ((1ull << size8) - 1));
      if ((bytes & all_be) == all_be) {
        memcpy(m_p_memory->m_page_table[page]+mem_offset, buffer, size8);
        m_statistics[port_num]->m_active_stats_interval->m_num_write_bytes_xfered += size8;
      }
      else {
        for (u32 i = 0; i<size8; ++i) {
          if (bytes & 0x1) {
            *(m_p_memory->m_page_table[page]+mem_offset) = buffer[i];
            m_statistics[port_num]->m_active_stats_interval->m_num_write_bytes_xfered += 1;
          }
          bytes >>= 1;
          mem_offset += 1;
        }
      }
      if (m_support_exclusive) {
        check_exclusive_monitors_against_write(*p_request, address8, size8);
      }
    }
    if (m_host_mutex) { m_p_memory->unlock(address8); }
    m_rcw_have_first_transfer[port_num] = false;
    send_response(port_num, m_p_memory->m_log_data_binary);
  }
}



void xtsc_component::xtsc_memory::do_write(u32 port_num) {
  xtsc_request       *p_request     = &m_p_active_request_info[port_num]->m_request;
  xtsc_address        address8      = p_request->get_byte_address();
  u32                 size8         = p_request->get_byte_size();
  const u8           *buffer        = p_request->get_buffer();
  xtsc_byte_enables   byte_enables  = p_request->get_byte_enables();
  u32                 page          = get_page(address8);
  xtsc_address        addr8         = address8 & (XTSC_MAX_ADDRESS ^ (size8-1));
  if (m_width8 && (size8 > m_width8)) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': ";
    oss << "Received WRITE tag=" << p_request->get_tag() << " with byte size of " << size8 << " (exceeds \"byte_width\" of "
        << m_width8 << ")";
    throw xtsc_exception(oss.str());
  }
  if (size8 > xtsc_max_bus_width8) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': ";
    oss << "Received WRITE tag=" << p_request->get_tag() << " with byte size of " << size8 << " (exceeds xtsc_max_bus_width8 of "
        << xtsc_max_bus_width8 << ")";
    throw xtsc_exception(oss.str());
  }
  bool doit = true;
  if (p_request->get_exclusive()) {
    if (m_p_exclusive_script_stream) {
      doit = set_exclusive_from_script(address8, *m_p_active_response[port_num]);
    }
    else if (m_support_exclusive) {
      doit = check_exclusive_write(*p_request, address8, size8, *m_p_active_response[port_num]);
    }
  }
  XTSC_DEBUG(m_text, "do_write address8=0x" << hex << address8 << " byte_enables=0x" << byte_enables <<
                     " addr8=0x" << addr8 << " size8=" << dec << size8 << " doit=" << doit);
  if (doit) {
    u32 mem_offset  = get_page_offset(addr8);
    u32 buf_offset  = 0;
    xtsc_byte_enables bytes = byte_enables;
    xtsc_byte_enables all_be = ((size8 == 64) ? 0xFFFFFFFFFFFFFFFFull : ((1ull << size8) - 1));
    if (m_host_mutex) { m_p_memory->lock(address8); }
    if ((bytes & all_be) == all_be) {
      memcpy(m_p_memory->m_page_table[page]+mem_offset, buffer, size8);
      m_statistics[port_num]->m_active_stats_interval->m_num_write_bytes_xfered += size8;
    }
    else {
      for (u32 i = 0; i<size8; ++i) {
        if (bytes & 0x1) {
          *(m_p_memory->m_page_table[page]+mem_offset) = buffer[buf_offset];
          m_statistics[port_num]->m_active_stats_interval->m_num_write_bytes_xfered += 1;
        }
        bytes >>= 1;
        mem_offset += 1;
        buf_offset += 1;
      }
    }
    if (m_host_mutex) { m_p_memory->unlock(address8); }
    XTSC_DEBUG(m_text, m_p_active_request_info[port_num]->m_request);
    if (m_support_exclusive) {
      check_exclusive_monitors_against_write(*p_request, address8, size8);
    }
  }
  else {
    XTSC_INFO(m_text, *p_request << " (Exclusive write not done)");
  }
  if (m_support_exclusive && p_request->get_exclusive()) {
    if (m_host_mutex) { m_p_memory->unlock(address8); }
  }

  if (m_write_responses) {
    send_response(port_num, false);
  }
}



void xtsc_component::xtsc_memory::do_block_write(u32 port_num) {
  xtsc_request       *p_request     = &m_p_active_request_info[port_num]->m_request;
  xtsc_address        address8      = p_request->get_byte_address();
  const u8           *buffer        = p_request->get_buffer();
  xtsc_byte_enables   byte_enables  = p_request->get_byte_enables();
  bool                last_transfer = p_request->get_last_transfer();
  u32                 page          = get_page(address8);
  u32                 size8         = p_request->get_byte_size();
  xtsc_address        hw_addr       = p_request->get_hardware_address();
  u32                 mem_offset    = get_page_offset(address8);
  u32                 buf_offset    = 0;
  XTSC_DEBUG(m_text, "in do_block_write for tag=" << p_request->get_tag() << " xfer #" << 
                     (m_block_write_transfer_count[port_num]+1) << "/" << p_request->get_num_transfers());
  XTSC_DEBUG(m_text, "do_block_write address8=0x" << hex << address8 << " byte_enables=0x" << byte_enables << " hw_addr=0x" << hw_addr);
  if (m_width8 && (size8 != m_width8)) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': ";
    oss << "Received BLOCK_WRITE tag=" << p_request->get_tag() << " with byte size of " << size8
        << " (contradicts \"byte_width\" of " << m_width8 << ")";
    throw xtsc_exception(oss.str());
  }
  if (size8 > xtsc_max_bus_width8) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': ";
    oss << "Received BLOCK_WRITE tag=" << p_request->get_tag() << " with byte size of " << size8 << " (exceeds xtsc_max_bus_width8 of "
        << xtsc_max_bus_width8 << ")";
    throw xtsc_exception(oss.str());
  }
  if (m_block_write_transfer_count[port_num] == 0) {
    // First transfer
    if (p_request->get_exclusive()) {
      if (m_p_exclusive_script_stream) {
        m_multi_write_doit[port_num] = set_exclusive_from_script(address8, *m_p_active_response[port_num]);
      }
      else if (m_support_exclusive) {
        u32 total_size8 = size8 * p_request->get_num_transfers();
        m_multi_write_doit[port_num] = check_exclusive_write(*p_request, address8, total_size8, *m_p_active_response[port_num]);
      }
    }
  }
  if (m_multi_write_doit[port_num]) {
    xtsc_byte_enables bytes = byte_enables;
    xtsc_byte_enables all_be = ((size8 == 64) ? 0xFFFFFFFFFFFFFFFFull : ((1ull << size8) - 1));
    if (m_host_mutex) { m_p_memory->lock(address8); }
    if ((bytes & all_be) == all_be) {
      memcpy(m_p_memory->m_page_table[page]+mem_offset, buffer, size8);
      m_statistics[port_num]->m_active_stats_interval->m_num_write_bytes_xfered += size8;
    }
    else {
      for (u32 i = 0; i<size8; ++i) {
        if (bytes & 0x1) {
          *(m_p_memory->m_page_table[page]+mem_offset) = buffer[buf_offset];
          m_statistics[port_num]->m_active_stats_interval->m_num_write_bytes_xfered += 1;
        }
        bytes >>= 1;
        mem_offset += 1;
        buf_offset += 1;
      }
    }
    if (m_host_mutex) { m_p_memory->unlock(address8); }
    if (m_support_exclusive) {
      check_exclusive_monitors_against_write(*p_request, address8, size8);
    }
  }
  else {
    XTSC_INFO(m_text, *p_request << " (Exclusive write not done)");
  }
  if (xtsc_is_text_logging_enabled() && m_text.isEnabledFor(DEBUG_LOG_LEVEL)) {
    u32 mem_offset  = get_page_offset(address8);
    ostringstream oss;
    xtsc_hex_dump(size8, (m_p_memory->m_page_table[page]+mem_offset), oss);
    XTSC_DEBUG(m_text, "BLOCK_WRITE" << (last_transfer ? "*" : " ") << " tag=" << p_request->get_tag() << " doit=" << m_multi_write_doit[port_num] <<
                      " [0x" << hex << setfill('0') << setw(xtsc_address_nibbles()) << address8 << setfill(' ') << "/" << size8 << "]= " << oss.str());
  }
  do_block_write_transfer_count(p_request, port_num, last_transfer);
  if (last_transfer) {
    m_multi_write_doit[port_num] = true;
    if (m_write_responses) {
      send_response(port_num, false);
    }
  }
}



void xtsc_component::xtsc_memory::do_block_write_transfer_count(xtsc_request *p_request, u32 port_num, bool last_transfer) {
  if (p_request->get_type() == xtsc_request::BLOCK_WRITE) {
    m_block_write_transfer_count[port_num] += 1;
    if (last_transfer) {
      if (m_block_write_transfer_count[port_num] != p_request->get_num_transfers()) {
        ostringstream oss;
        oss << kind() << " '" << name() << "': ";
        oss << "Received BLOCK_WRITE tag=" << p_request->get_tag() << " with last_transfer flag set but only "
            << m_block_write_transfer_count[port_num] << " transfers out of an expected total of " << p_request->get_num_transfers()
            << " have occurred.";
        throw xtsc_exception(oss.str());
      }
      m_block_write_transfer_count[port_num] = 0;
    }
    else {
      if (m_block_write_transfer_count[port_num] == p_request->get_num_transfers()) {
        ostringstream oss;
        oss << kind() << " '" << name() << "': ";
        oss << "Received BLOCK_WRITE tag=" << p_request->get_tag() << " with last_transfer flag NOT set but all "
            << m_block_write_transfer_count[port_num] << " transfers have occurred.";
        throw xtsc_exception(oss.str());
      }
    }
  }
}


void xtsc_component::xtsc_memory::do_burst_write_transfer_count(xtsc_request *p_request, u32 port_num, bool last_transfer) {
  if (p_request->get_type() == xtsc_request::BURST_WRITE) {
    m_burst_write_transfer_count[port_num] += 1;
    if (last_transfer) {
      if (!p_request->is_evict() && !p_request->is_cache_maintenance() && 
          (m_burst_write_transfer_count[port_num] != p_request->get_num_transfers())) {
        ostringstream oss;
        oss << kind() << " '" << name() << "': ";
        oss << "Received BURST_WRITE tag=" << p_request->get_tag() << " with last_transfer flag set but only "
            << m_burst_write_transfer_count[port_num] << " transfers out of an expected total of " << p_request->get_num_transfers()
            << " have occurred.";
        throw xtsc_exception(oss.str());
      }
      m_burst_write_transfer_count[port_num] = 0;
    }
    else {
      if (m_burst_write_transfer_count[port_num] == p_request->get_num_transfers()) {
        ostringstream oss;
        oss << kind() << " '" << name() << "': ";
        oss << "Received BURST_WRITE tag=" << p_request->get_tag() << " with last_transfer flag NOT set but all "
            << m_burst_write_transfer_count[port_num] << " transfers have occurred.";
        throw xtsc_exception(oss.str());
      }
    }
  }
}



void xtsc_component::xtsc_memory::do_burst_write(u32 port_num) {
  xtsc_request         *p_request       = &m_p_active_request_info[port_num]->m_request;
  xtsc_address          address8        = p_request->get_byte_address();
  const u8             *buffer          = p_request->get_buffer();
  xtsc_byte_enables     byte_enables    = p_request->get_byte_enables();
  bool                  first_transfer  = (p_request->get_transfer_number() == 1);
  bool                  last_transfer   = p_request->get_last_transfer();
  bool                  evict           = p_request->is_evict();
  bool                  cache_maint     = p_request->is_cache_maintenance();
  u32                   page            = get_page(address8);
  u32                   size8           = p_request->get_byte_size();
  u32                   mem_offset      = get_page_offset(address8);
  u32                   buf_offset      = 0;
  bool                  axi             = p_request->is_axi_protocol();
  XTSC_DEBUG(m_text, "in do_burst_write for tag=" << p_request->get_tag() << " xfer #" << 
                     (m_burst_write_transfer_count[port_num]+1) << "/" << p_request->get_num_transfers());
  XTSC_DEBUG(m_text, "do_burst_write address8=0x" << hex << address8 << " byte_enables=0x" << byte_enables);
  if (!axi && m_width8 && (size8 != m_width8)) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': ";
    oss << "Received BURST_WRITE tag=" << p_request->get_tag() << " with byte size of " << size8
        << " (contradicts \"byte_width\" of " << m_width8 << ")";
    throw xtsc_exception(oss.str());
  }
  if (axi && m_width8 && (size8 > m_width8)) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': ";
    oss << "Received BURST_WRITE tag=" << p_request->get_tag() << " with byte size of " << size8
        << " (exceeds \"byte_width\" of " << m_width8 << ")";
    throw xtsc_exception(oss.str());
  }
  if (size8 > xtsc_max_bus_width8) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': ";
    oss << "Received BURST_WRITE tag=" << p_request->get_tag() << " with byte size of " << size8 << " (exceeds xtsc_max_bus_width8 of "
        << xtsc_max_bus_width8 << ")";
    throw xtsc_exception(oss.str());
  }

  if (first_transfer) {
    if (p_request->get_exclusive()) {
      if (axi) {
        if (m_p_exclusive_script_stream) {
          m_multi_write_doit[port_num] = set_exclusive_from_script(address8, *m_p_active_response[port_num]);
        }
        else if (m_support_exclusive) {
          u32 total_size8 = size8 * p_request->get_num_transfers();
          m_multi_write_doit[port_num] = check_exclusive_write(*p_request, address8, total_size8, *m_p_active_response[port_num]);
        }
      }
      else {
        ostringstream oss;
        oss << kind() << " '" << name() << "': Received illegal exclusive request: " << *p_request;
        throw xtsc_exception(oss.str());
      }
    }
  }
  u32  first_bit = byte_enables & 0x1;
  bool other_bit_found = false;
  bool pif_illegal_byte_enables = false;
  if (m_multi_write_doit[port_num]) {
    if (!evict && !cache_maint) {
      xtsc_byte_enables bytes = byte_enables;
      xtsc_byte_enables all_be = ((size8 == 64) ? 0xFFFFFFFFFFFFFFFFull : ((1ull << size8) - 1));
      if (m_host_mutex) { m_p_memory->lock(address8); }
      if ((bytes & all_be) == all_be) {
        memcpy(m_p_memory->m_page_table[page]+mem_offset, buffer, size8);
        m_statistics[port_num]->m_active_stats_interval->m_num_write_bytes_xfered += size8;
      }
      else {
        for (u32 i = 0; i<size8; ++i) {
          if (bytes & 0x1) {
            *(m_p_memory->m_page_table[page]+mem_offset) = buffer[buf_offset];
            m_statistics[port_num]->m_active_stats_interval->m_num_write_bytes_xfered += 1;
          }
          if (((bytes & 0x1) != first_bit)) {
            other_bit_found = true;
          }
          else if (other_bit_found) {
            pif_illegal_byte_enables = true;
          }
          bytes >>= 1;
          mem_offset += 1;
          buf_offset += 1;
        }
      }
      if (m_host_mutex) { m_p_memory->unlock(address8); }
    }
    if (m_support_exclusive) {
      check_exclusive_monitors_against_write(*p_request, address8, size8);
    }
  }
  if ((byte_enables == 0) || (last_transfer && (first_bit == 0)) || (first_transfer && (first_bit == 1) && other_bit_found)) {
    pif_illegal_byte_enables = true;
  }
  if (pif_illegal_byte_enables && !axi) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': " << "Illegal PIF byte enables: " << *p_request;
    throw xtsc_exception(oss.str());
  }
  if (xtsc_is_text_logging_enabled() && m_text.isEnabledFor(DEBUG_LOG_LEVEL)) {
    u32 mem_offset  = get_page_offset(address8);
    ostringstream oss;
    if (!evict && !cache_maint) {
      xtsc_hex_dump(size8, (m_p_memory->m_page_table[page]+mem_offset), oss);
    }
    XTSC_DEBUG(m_text, "BURST_WRITE" << (last_transfer ? "*" : " ") << " tag=" << p_request->get_tag() << " doit=" << m_multi_write_doit[port_num] <<
                      " [0x" << hex << setfill('0') << setw(xtsc_address_nibbles()) << address8 << setfill(' ') << "/" << size8 << "]= " << oss.str());
  }
  do_burst_write_transfer_count(p_request, port_num, last_transfer);
  if (last_transfer) {
    m_multi_write_doit[port_num] = true;
    if (m_write_responses) {
      send_response(port_num, false);
    }
  }
}



void xtsc_component::xtsc_memory::send_response(u32 port_num, bool log_data_binary) {
  u32 tries = 0;
  if (m_user_data_type) {
    m_p_active_response[port_num]->set_user_data(m_p_user_data);
  }
  if (m_filter_responses) {
    handle_response_filters(port_num, *m_p_active_response[port_num]);
  }
  while (true) {
    tries += 1;
    XTSC_INFO(m_text, *m_p_active_response[port_num] << m_p_active_response[port_num]->get_user_data_for_logging(m_user_data_length) <<
                      " Port #" << port_num << " Try #" << tries);
    xtsc_log_memory_response_event(m_binary, INFO_LOG_LEVEL, false, 0, log_data_binary, *m_p_active_response[port_num]);
    if ((*m_respond_ports[port_num])->nb_respond(*m_p_active_response[port_num])) {
      break;
    }
    if (m_immediate_timing) {
      ostringstream oss;
      oss << kind() << " '" << name() << "': ";
      oss << "nb_respond() returned false which is not allowed when \"immediate_timing\" is true: " << *m_p_active_response[port_num];
      throw xtsc_exception(oss.str());
    }
    wait(m_response_repeat);
  }
}



xtsc_component::xtsc_memory::request_info *xtsc_component::xtsc_memory::new_request_info(const xtsc_request&     request,
                                                                                         xtsc_response::status_t status,
                                                                                         bool                    list)
{
  if (m_request_pool.empty()) {
    XTSC_DEBUG(m_text, "Creating a new request_info");
    return new request_info(request, status, list);
  }
  else {
    request_info *p_request_info = m_request_pool.back();
    m_request_pool.pop_back();
    p_request_info->init(request, status, list);
    return p_request_info;
  }
}



void xtsc_component::xtsc_memory::delete_request_info(request_info*& p_request_info) {
  m_request_pool.push_back(p_request_info);
  p_request_info = 0;
}



void xtsc_component::xtsc_memory::xtsc_request_if_impl::nb_peek(xtsc_address address8, u32 size8, u8 *buffer) {
  m_memory.peek(address8, size8, buffer);
  if (m_memory.m_filter_peeks) {
    for (set<u32>::const_iterator i = m_memory.m_peek_watchfilters.begin(); i != m_memory.m_peek_watchfilters.end(); ++i) {
      u32 watchfilter = *i;
      map<u32, watchfilter_info*>::const_iterator j = m_memory.m_watchfilters.find(watchfilter);
      if (j == m_memory.m_watchfilters.end()) {
        ostringstream oss;
        oss << "Error: watchfilter #" << watchfilter << " missing from m_watchfilters at line " << __LINE__ << " of " << __FILE__;
        throw xtsc_exception(oss.str());
      }
      watchfilter_info *p_watchfilter_info = j->second;
      if (xtsc_filter_apply_xtsc_peek(p_watchfilter_info->m_filter_name, m_port_num, address8, size8, buffer)) {
        p_watchfilter_info->m_event.notify(SC_ZERO_TIME);
        XTSC_INFO(m_memory.m_text, "nb_peek(0x" << hex << address8 << ", " << dec << size8 << ") Watchfilter #" <<
                                   p_watchfilter_info->m_watchfilter);
      }
    }
  }
}



void xtsc_component::xtsc_memory::xtsc_request_if_impl::nb_poke(xtsc_address address8, u32 size8, const u8 *buffer) {
  m_memory.poke(address8, size8, buffer);
  if (m_memory.m_filter_pokes) {
    for (set<u32>::const_iterator i = m_memory.m_poke_watchfilters.begin(); i != m_memory.m_poke_watchfilters.end(); ++i) {
      u32 watchfilter = *i;
      map<u32, watchfilter_info*>::const_iterator j = m_memory.m_watchfilters.find(watchfilter);
      if (j == m_memory.m_watchfilters.end()) {
        ostringstream oss;
        oss << "Error: watchfilter #" << watchfilter << " missing from m_watchfilters at line " << __LINE__ << " of " << __FILE__;
        throw xtsc_exception(oss.str());
      }
      watchfilter_info *p_watchfilter_info = j->second;
      if (xtsc_filter_apply_xtsc_poke(p_watchfilter_info->m_filter_name, m_port_num, address8, size8, buffer)) {
        p_watchfilter_info->m_event.notify(SC_ZERO_TIME);
        XTSC_INFO(m_memory.m_text, "nb_poke(0x" << hex << address8 << ", " << dec << size8 << ") Watchfilter #" <<
                                   p_watchfilter_info->m_watchfilter);
      }
    }
  }
}



bool xtsc_component::xtsc_memory::xtsc_request_if_impl::nb_fast_access(xtsc_fast_access_request &request) {
  xtsc_address address8    = request.get_translated_request_address();
  xtsc_address page_start8 = address8 & ~(m_memory.m_p_memory->m_page_size8 - 1);
  xtsc_address page_end8   = page_start8 + m_memory.m_p_memory->m_page_size8 - 1;

  if ((address8 < m_memory.m_start_address8) || (address8 > m_memory.m_end_address8)) {
    ostringstream oss;
    oss << "Memory access out-of-range (address=0x" << hex << address8 << ") in nb_fast_access() of memory '"
        << m_memory.name() << "' (Valid range: 0x" << m_memory.m_start_address8 << "-0x"
        << m_memory.m_end_address8 << ").";
    throw xtsc_exception(oss.str());
  }

  // Allow any fast access?
  if (!m_memory.m_use_fast_access) {
    request.deny_access();
    XTSC_INFO(m_memory.m_text, hex << setfill('0') << "nb_fast_access: address8=0x" << address8 <<
                               " page=[0x" << page_start8 << "-0x" << page_end8 << "] deny_access");
    return true;
  }
  
  string access("raw access");
  if (m_memory.m_use_raw_access) {
    u32 page = m_memory.get_page(address8);
    u8 *raw_bytes = m_memory.m_p_memory->m_page_table[page];
    request.allow_raw_access(page_start8, (u32*)raw_bytes, m_memory.m_p_memory->m_page_size8, 0);
    xtsc_fast_access_block page_block(address8, page_start8, page_end8);
    request.restrict_to_block(page_block);
  }
  else if (m_memory.m_use_callback_access) {
    request.allow_callbacks_access(&m_memory, read_callback, write_callback);
    access = "callback access";
  }
  else if (m_memory.m_use_custom_access) {
    request.allow_custom_callbacks_access(&m_memory, custom_read_callback, custom_write_callback);
    access = "custom callback access";
  }
  else if (m_memory.m_use_interface_access) {
    request.allow_interface_access(m_memory.get_fast_access_object());
    access = "interface access";
  }
  else {
    request.allow_peek_poke_access();
    access = "peek/poke access";
  }

  bool granted = true;
  u32  n       = xtsc_address_nibbles();
  
  // Determine whether or not address8 is inside a "deny_fast_access" block:
  // - If it is, then deny fast access to the "deny_fast_access" block
  // - If it is not, then ensure grant does not include any "deny_fast_access" addresses.
  for (u32 i=0; i<m_memory.m_deny_fast_access.size(); i+=2) {
    xtsc_address begin = m_memory.m_deny_fast_access[i+0];
    xtsc_address end   = m_memory.m_deny_fast_access[i+1];
    if ((address8 >= begin) && (address8 <= end)) {
      // Deny fast access
      granted = false;
      request.deny_access();
      xtsc_fast_access_block deny_block(address8, begin, end);
      request.restrict_to_block(deny_block);
      access = "deny access";
      XTSC_DEBUG(m_memory.m_text, "nb_fast_access: addr=0x" << hex << setfill('0') << setw(n) << address8 <<
                                  " deny=[0x" << setw(n) << begin << ",0x" << setw(n) << end << "]" << " Port #" << m_port_num);
      break;
    }
    request.remove_address_range(address8, begin, end);
    XTSC_DEBUG(m_memory.m_text, hex << setfill('0') << "nb_fast_access: address8=0x" << address8 <<
                                " page=[0x" << page_start8 << "-0x" << page_end8 << "]" <<
                                " remove=[0x" << begin << "-0x" << end << "]");
  }


  // Determine whether or not address8 is inside any of "secure_address_range" block:
  // - If it is, then deny fast access to the "secure_address_range" block
  // - If it is not, then ensure grant does not include any "secure_address_range" addresses.
  for (u32 i=0; i<m_memory.m_secure_address_range.size(); i+=2) {
    xtsc_address begin = m_memory.m_secure_address_range[i+0];
    xtsc_address end   = m_memory.m_secure_address_range[i+1];
    if ((address8 >= begin) && (address8 <= end)) {
      // Deny fast access to secure range
      granted = false;
      request.deny_access();
      xtsc_fast_access_block secure_block(address8, begin, end);
      request.restrict_to_block(secure_block);
      access = "secure access";
      XTSC_DEBUG(m_memory.m_text, "nb_fast_access: addr=0x" << hex << setfill('0') << setw(n) << address8 <<
                                  " deny=[0x" << setw(n) << begin << ",0x" << setw(n) << end << "]" << " Port #" << m_port_num);
    }
    request.remove_address_range(address8, begin, end);
    XTSC_DEBUG(m_memory.m_text, hex << setfill('0') << "nb_fast_access: address8=0x" << address8 <<
                                " page=[0x" << page_start8 << "-0x" << page_end8 << "]" <<
                                " remove=[0x" << begin << "-0x" << end << "]");
  }


  // Limit granted access
  if (granted) {

    // Ensure grant is limited to the page
    if (page_start8 > 0) {
      request.remove_address_range(address8, 0, page_start8-1);
    }
    if (page_end8 < XTSC_MAX_ADDRESS) {
      request.remove_address_range(address8, page_end8+1, XTSC_MAX_ADDRESS);
    }

    // Determine whether or not address8 is inside a "fast_access_size" block:
    // - If it is, then apply that granularity
    // - If it is not, then ensure grant does not include any "fast_access_size" addresses.
    for (u32 i=0; i<m_memory.m_fast_access_size.size(); i+=3) {
      xtsc_address begin = m_memory.m_fast_access_size[i+0];
      xtsc_address end   = m_memory.m_fast_access_size[i+1];
      xtsc_address gran  = m_memory.m_fast_access_size[i+2];
      if ((address8 >= begin) && (address8 <= end)) {
        xtsc_address gran_beg = (address8 / gran) * gran;
        xtsc_address gran_end = gran_beg + gran - 1;
        if (gran_beg > 0) {
          request.remove_address_range(address8, 0, gran_beg-1);
        }
        if (gran_end < XTSC_MAX_ADDRESS) {
          request.remove_address_range(address8, gran_end+1, XTSC_MAX_ADDRESS);
        }
      }
      else {
        request.remove_address_range(address8, begin, end);
      }
    }

  }

  if (xtsc_is_text_logging_enabled() && m_memory.m_text.isEnabledFor(INFO_LOG_LEVEL)) {
    xtsc_fast_access_block block = request.get_local_block(address8);
    XTSC_INFO(m_memory.m_text, hex << setfill('0') << "nb_fast_access: addr=0x" << hex << setfill('0') << setw(n) <<
                               request.get_request_address() << " trans=0x" << setw(n) << address8 << " [0x" << setw(n) <<
                               block.get_block_beg_address() << "-0x" << setw(n) << block.get_block_end_address() << "] " << access);
  }

  return true;
}



void xtsc_component::xtsc_memory::xtsc_request_if_impl::nb_request(const xtsc_request& request) {
  XTSC_INFO(m_memory.m_text, request << request.get_user_data_for_logging(m_memory.m_log_user_data_bytes) << " Port #" << m_port_num);
  xtsc_log_memory_request_event(m_memory.m_binary, INFO_LOG_LEVEL, true, 0, m_memory.m_p_memory->m_log_data_binary, request);
  // Check if we should NACC this request for testing purposes (non-script based)
  if (m_memory.do_nacc_failure(request, m_port_num)) {
    xtsc_response response(request, xtsc_response::RSP_NACC);
    XTSC_INFO(m_memory.m_text, response << " (Test RSP_NACC Port #" << m_port_num << ")");
    xtsc_log_memory_response_event(m_memory.m_binary, INFO_LOG_LEVEL, false, 0, false, response);
    if (m_memory.m_filter_responses) {
      m_memory.handle_response_filters(m_port_num, response);
    }
    m_memory.m_statistics[m_port_num]->m_num_nacced_requests++;
    (*m_memory.m_respond_ports[m_port_num])->nb_respond(response);
    return;
  }
  // Check if we should NACC this request because we don't have room for it
  if (!m_memory.m_immediate_timing && (m_memory.m_request_fifo[m_port_num]->num_free() == 0)) {
    xtsc_response response(request, xtsc_response::RSP_NACC);
    XTSC_INFO(m_memory.m_text, response << " (Request FIFO full Port #" << m_port_num << ")");
    xtsc_log_memory_response_event(m_memory.m_binary, INFO_LOG_LEVEL, false, 0, false, response);
    if (m_memory.m_filter_responses) {
      m_memory.handle_response_filters(m_port_num, response);
    }
    m_memory.m_statistics[m_port_num]->m_num_nacced_requests++;
    (*m_memory.m_respond_ports[m_port_num])->nb_respond(response);
    return;
  }
  // Check and Update active_stats_interval
  m_memory.update_stats_active_interval();
  // Do special handling based on "script_file"
  xtsc_response::status_t status = xtsc_response::RSP_OK;
  bool list = false;
  if (m_memory.m_p_script_stream != NULL) {
    m_memory.m_prev_port = m_port_num;
    m_memory.m_prev_hit  = m_memory.compute_special_response(request, m_port_num, status, list, m_memory.m_prev_type);
    m_memory.m_script_thread_event.notify();
    if (m_memory.m_prev_hit && (status == xtsc_response::RSP_NACC)) {
      xtsc_response response(request, xtsc_response::RSP_NACC);
      XTSC_INFO(m_memory.m_text, response << " (script_file directed)");
      xtsc_log_memory_response_event(m_memory.m_binary, INFO_LOG_LEVEL, false, 0, false, response);
      if (m_memory.m_filter_responses) {
        m_memory.handle_response_filters(m_port_num, response);
      }
      m_memory.m_statistics[m_port_num]->m_num_nacced_requests++;
      (*m_memory.m_respond_ports[m_port_num])->nb_respond(response);
      return;
    }
  }
  request_info *p_request_info = m_memory.new_request_info(request, status, list);
  if (m_memory.m_immediate_timing) {
    // Accept this one as our current transaction
    m_memory.m_p_active_request_info[m_port_num] = p_request_info;
    m_memory.do_active_request(m_port_num);
  }
  else {
    m_memory.m_request_fifo[m_port_num]->nb_write(p_request_info);
    m_memory.m_worker_thread_event[m_port_num]->notify(SC_ZERO_TIME);
    XTSC_DEBUG(m_memory.m_text, "nb_request() called m_worker_thread_event[" << m_port_num << "]->notify(SC_ZERO_TIME): " <<
                                p_request_info->m_request);
  }
}



void xtsc_component::xtsc_memory::xtsc_request_if_impl::nb_load_retired(xtsc_address address8) {
  XTSC_INFO(m_memory.m_text, "nb_load_retired(0x" << setfill('0') << hex << setw(xtsc_address_nibbles()) << address8 << ") called");
}



void xtsc_component::xtsc_memory::xtsc_request_if_impl::nb_retire_flush() {
  XTSC_INFO(m_memory.m_text, "nb_retire_flush() called");
}



void xtsc_component::xtsc_memory::xtsc_request_if_impl::nb_lock(bool lock) {
  XTSC_INFO(m_memory.m_text, "nb_lock(" << boolalpha << lock << ") called on Port #" << m_port_num);
  if (m_memory.m_host_mutex) {
    if (lock) {
      m_memory.m_p_memory->lock(m_memory.m_end_address8);
    }
    else {
      m_memory.m_p_memory->unlock(m_memory.m_end_address8);
    }
  }
}



void xtsc_component::xtsc_memory::xtsc_request_if_impl::register_port(sc_port_base& port, const char *if_typename) {
  if (m_p_port) {
    ostringstream oss;
    oss << "Illegal multiple binding detected to xtsc_memory '" << m_memory.name() << "' m_request_exports[" << m_port_num << "]: "
        << endl;
    oss << "  " << port.name() << endl;
    oss << "  " << m_p_port->name();
    throw xtsc_exception(oss.str());
  }
  XTSC_INFO(m_memory.m_text, "Binding '" << port.name() << "' to xtsc_memory::m_request_exports[" << m_port_num << "]");
  m_p_port = &port;
}



xtsc::xtsc_fast_access_if *xtsc_component::xtsc_memory::get_fast_access_object() const {
  return m_fast_access_object;
}



void xtsc_component::xtsc_memory::script_thread() {

  try {

    while (get_words() != 0) {
      if (m_words[0] == "wait") {
        if (m_words.size() < 2) {
          ostringstream oss;
          oss << "WAIT command is missing arguments:" << endl;
          oss << m_line;
          oss << m_p_script_stream->info_for_exception();
          throw xtsc_exception(oss.str());
        }
        else if (m_words.size() == 2) {
          sc_time duration = m_clock_period * get_double(1, "duration");
          XTSC_DEBUG(m_text, "waiting " << duration);
          wait(duration);
        }
        else if ((m_words.size() == 4) || (m_words.size() == 5)) {
          u32 port = m_num_ports;
          if (m_words[1] != "*") {
            port = get_u32(1, "port");
            if (port >= m_num_ports) {
              ostringstream oss;
              oss << "port=" << port << " is out of range (0-" << (m_num_ports-1) << "):" << endl;
              oss << m_line;
              oss << m_p_script_stream->info_for_exception();
              throw xtsc_exception(oss.str());
            }
          }
          u32 type = get_request_type_code(2);
          u32 match = 0;
               if (m_words[3] == "*"   ) { match = 2; }
          else if (m_words[3] == "hit" ) { match = 1; }
          else if (m_words[3] == "miss") { match = 0; }
          else {
            ostringstream oss;
            oss << "WAIT command match argument (#3) is invalid (expected *|hit|miss):" << endl;
            oss << m_line;
            oss << m_p_script_stream->info_for_exception();
            throw xtsc_exception(oss.str());
          }
          u32 count = ((m_words.size() == 4) ? 1 : get_u32(4, "count"));
          if (count == 0) {
            ostringstream oss;
            oss << "WAIT command count cannot be zero:" << endl;
            oss << m_line;
            oss << m_p_script_stream->info_for_exception();
            throw xtsc_exception(oss.str());
          }
          do {
            bool got_one = false;
            do {
              XTSC_DEBUG(m_text, "Doing wait with current count of " << count << " for command: " << m_line);
              wait(m_script_thread_event);
              if ((port == m_prev_port) || (port == m_num_ports)) {
                if ((type == m_prev_type) || (type == 5)) {
                  if ((!match && !m_prev_hit) || (match && m_prev_hit) || (match == 2)) {
                    got_one = true;
                  }
                }
              }
            } while (!got_one);
            count -= 1;
          } while (count != 0);
        }
        else {
          ostringstream oss;
          oss << "WAIT command has missing/extra arguments:" << endl;
          oss << m_line;
          oss << m_p_script_stream->info_for_exception();
          throw xtsc_exception(oss.str());
        }
      }
      else if ((m_words[0] == "sync") || (m_words[0] == "synchronize")) {
        if (m_words.size() != 2) {
          ostringstream oss;
          oss << "SYNC command has missing/extra arguments:" << endl;
          oss << m_line;
          oss << m_p_script_stream->info_for_exception();
          throw xtsc_exception(oss.str());
        }
        sc_time time = m_clock_period * get_double(1, "time");
        sc_time now = sc_time_stamp();
        sc_time delay = (time <= now) ? SC_ZERO_TIME : (time - now);
        XTSC_DEBUG(m_text, "sync to time " << time << " requires delay of " << delay << " (no wait for 0 delay)");
        if (delay != SC_ZERO_TIME) {
          wait(delay);
        }
      }
      else if (m_words[0] == "info") {
        XTSC_INFO(m_text, m_line);
      }
      else if (m_words[0] == "note") {
        XTSC_NOTE(m_text, m_line);
      }
      else if ((m_words.size() > 1) && (m_words[1] == "stop")) {
        sc_time delay = m_clock_period * get_double(0, "delay");
        XTSC_DEBUG(m_text, "delaying " << delay << " before calling sc_stop()");
        wait(delay);
        XTSC_INFO(m_text, "calling sc_stop()");
        sc_stop();
      }
      else if ((m_words[0] == "dump")) {
        log4xtensa::LogLevel log_level = log4xtensa::INFO_LOG_LEVEL;
        if (m_words.size() > 1) {
          if (m_words[1] == "note") {
            log_level = log4xtensa::NOTE_LOG_LEVEL;
          }
        }
        ostringstream oss;
        oss << "Addresses:" << endl;
        dump_addresses(oss);
        xtsc_log_multiline(m_text, log_level, oss.str(), 2);
      }
      else if (m_words[0] == "clear") {
        if (m_words.size() == 1) {
          clear_addresses();
        }
        else {
          xtsc_address low_address;
          xtsc_address high_address;
          bool is_range = get_addresses(1, "address/address-range", low_address, high_address);
          map<xtsc_address, address_info*> *p_map = (is_range ? &m_address_range_map : &m_address_map);
          map<xtsc_address, address_info*>::iterator im = p_map->find(low_address);
          if ((im == p_map->end()) || (im->second->m_high_address != high_address)) {
            ostringstream oss;
            oss << "No entry for the address/address-range specified:" << endl;
            oss << m_line;
            oss << m_p_script_stream->info_for_exception();
            throw xtsc_exception(oss.str());
          }
          XTSC_INFO(m_text, "Clearing " << *im->second);
          p_map->erase(low_address);
        }
      }
      else if (m_words[0] == "user_data") {
        if (m_user_data_type == 2) {
          delete [] m_p_user_data;
        }
        if (m_words.size() == 1) {
          m_user_data_length = 0;
          m_p_user_data      = 0;
          m_user_data_type   = 0;
        }
        else if (m_words.size() == 2) {
          ostringstream oss;
          oss << "Too few arguments in USER_DATA command:" << endl;
          oss << m_line;
          oss << m_p_script_stream->info_for_exception();
          throw xtsc_exception(oss.str());
        }
        else {
          if (m_words[1] == "*") {
            m_user_data_length = m_words.size() - 2;
            m_p_user_data      = new u8[m_user_data_length];
            m_user_data_type   = 2;
            for (u32 i=0; i < m_user_data_length; ++i) {
              m_p_user_data[i] = (u8) get_u32(i+2, "byte");
            }
          }
          else {
            u32 length         = get_u32(1, "length");
            m_user_data_length = -length;
#if   defined(__x86_64__) || defined(_M_X64)
            u32 limit          = 8;
            m_p_user_data      = (u8 *) get_u64(2, "value");
#else
            u32 limit          = 4;
            m_p_user_data      = (u8 *) get_u32(2, "value");
#endif
            m_user_data_type   = 1;
            if (!length || (length > limit)) {
              ostringstream oss;
              oss << "Invalid length=" << length << ".  Must be between 1 and " << limit << endl;
              oss << m_line;
              oss << m_p_script_stream->info_for_exception();
              throw xtsc_exception(oss.str());
            }
          }
        }
      }
      else if (m_words[0] == "response_list") {
        m_response_list.clear();
        for (u32 i=1; i < m_words.size(); ++i) {
          if (m_words[i] == "0") {
            m_response_list.push_back(xtsc_response::RSP_OK);
          }
          else if (m_words[i] == "1") {
            m_response_list.push_back(xtsc_response::RSP_ADDRESS_ERROR);
          }
          else if (m_words[i] == "2") {
            m_response_list.push_back(xtsc_response::RSP_DATA_ERROR);
          }
          else if (m_words[i] == "3") {
            m_response_list.push_back(xtsc_response::RSP_ADDRESS_DATA_ERROR);
          }
          else if (m_words[i] == "4") {
            m_response_list.push_back(xtsc_response::RSP_NACC);
          }
          else {
            ostringstream oss;
            oss << "Invalid response status (" << m_words[i] << ") at entry #" << i << " of response_list.  Should be 0 (RSP_OK) or 2 (RSP_DATA_ERROR)"
                << endl;
            oss << m_line;
            oss << m_p_script_stream->info_for_exception();
            throw xtsc_exception(oss.str());
          }
        }
      }
      else if (m_words[0] == "IS_SHARED") {
        if ((m_words.size() != 2)
            || !(m_words[1]=="false" || m_words[1]=="true" 
                 || m_words[1]=="0"  || m_words[1]=="1")) {
          ostringstream oss;
          oss << "IS_SHARED command takes 1 mandatory argument: true|false" << endl;
          oss << m_line;
          oss << m_p_script_stream->info_for_exception();
          throw xtsc_exception(oss.str());
        }
        else {
          m_is_shared = ((m_words[1]=="1") || (m_words[1]=="true")) ? true : false;
        }
      }
      else if (m_words[0] == "PASS_DIRTY") {
        if ((m_words.size() != 2)
            || !(m_words[1]=="false" || m_words[1]=="true" 
                 || m_words[1]=="0"  || m_words[1]=="1")) {
          ostringstream oss;
          oss << "PASS_DIRTY command takes 1 mandatory argument: true|false" << endl;
          oss << m_line;
          oss << m_p_script_stream->info_for_exception();
          throw xtsc_exception(oss.str());
        }
        else {
          m_pass_dirty = ((m_words[1]=="1") || (m_words[1]=="true")) ? true : false;
        }
      }
      else if (((m_words.size() == 4) || (m_words.size() == 5)) &&
               ((m_words[3] == "okay" )           ||
                (m_words[3] == "ok")              ||
                (m_words[3] == "nacc")            ||
                (m_words[3] == "address_error")   ||
                (m_words[3] == "data_error" )     ||
                (m_words[3] == "response_list") ||
                (m_words[3] == "address_data_error")))
      {
        xtsc_address low_address;
        xtsc_address high_address;
        bool is_range = get_addresses(0, "address/address-range", low_address, high_address);
        map<xtsc_address, address_info*> *p_map = (is_range ? &m_address_range_map : &m_address_map);
        map<xtsc_address, address_info*>::iterator im = p_map->find(low_address);
        if (im != p_map->end()) {
          ostringstream oss;
          oss << "Duplicate entry for the address/address-range specified:" << endl;
          oss << m_line;
          oss << m_p_script_stream->info_for_exception();
          throw xtsc_exception(oss.str());
        }
        if (is_range) {
          for (im = m_address_range_map.begin(); im != m_address_range_map.end(); ++im) {
            address_info &info = *im->second;
            if (((low_address  >= info.m_low_address) && (low_address  <= info.m_high_address)) || 
                ((high_address >= info.m_low_address) && (high_address <= info.m_high_address)))
            {
              ostringstream oss;
              oss << "Specified address range, 0x" << hex << low_address << "-0x" << high_address << ", overlaps entry: "
                  << info << endl;
              oss << m_line;
              oss << m_p_script_stream->info_for_exception();
              throw xtsc_exception(oss.str());
            }
          }
        }
        if ((low_address < m_start_address8) || (high_address > m_end_address8)) {
          ostringstream oss;
          if (is_range) {
            oss << "Specified address range, 0x" << hex << low_address << "-0x" << high_address;
          }
          else {
            oss << "Specified address, 0x" << hex << low_address;
          }
          oss << ", is not within xtsc_memory '" << name() << "' (0x" << m_start_address8 << "-0x" << m_end_address8 << "):" << endl;
          oss << m_line;
          oss << m_p_script_stream->info_for_exception();
          throw xtsc_exception(oss.str());
        }
        u32 port = m_num_ports;
        if (m_words[1] != "*") {
          port = get_u32(1, "port");
          if (port >= m_num_ports) {
            ostringstream oss;
            oss << "port=" << port << " is out of range (0-" << (m_num_ports-1) << "):" << endl;
            oss << m_line;
            oss << m_p_script_stream->info_for_exception();
            throw xtsc_exception(oss.str());
          }
        }
        u32 type = get_request_type_code(2);
        bool list = (m_words[3] == "response_list");
        xtsc_response::status_t status;
             if (m_words[3] == "okay")               status = xtsc_response::RSP_OK;
        else if (m_words[3] == "ok")                 status = xtsc_response::RSP_OK;
        else if (m_words[3] == "nacc")               status = xtsc_response::RSP_NACC;
        else if (m_words[3] == "address_error")      status = xtsc_response::RSP_ADDRESS_ERROR;
        else if (m_words[3] == "data_error")         status = xtsc_response::RSP_DATA_ERROR;
        else if (m_words[3] == "response_list")      status = m_response_list[0];
        else if (m_words[3] == "address_data_error") status = xtsc_response::RSP_ADDRESS_DATA_ERROR;
        else  {
          ostringstream oss;
          oss << "Program Bug: in line " << __LINE__ << " of file " << __FILE__;
          throw xtsc_exception(oss.str());
        }
        u32 limit = (m_words.size() == 4) ? 0 : get_u32(4, "limit");
        address_info *p_info = new address_info(low_address, high_address, is_range, port, m_num_ports, type, status, list, limit);
        (*p_map)[low_address] = p_info;
      }
      else if (m_words[0] == "last") {
        if (m_words.size() == 2) {
               if ((m_words[1] == "0") || (m_words[1] == "f") || (m_words[1] == "false")) { m_last = false; continue;}
          else if ((m_words[1] == "1") || (m_words[1] == "t") || (m_words[1] == "true" )) { m_last = true;  continue;}
        }
        ostringstream oss;
        oss << "LAST command has missing/extra arguments:" << endl;
        oss << m_line;
        oss << m_p_script_stream->info_for_exception();
        throw xtsc_exception(oss.str());
      }
      else if (m_words[0] == "lua_function") {
        if (m_words.size() == 2) {
          m_lua_function = m_words[1];
          continue;
        }
        ostringstream oss;
        oss << "lua_function command has missing/extra arguments:" << endl;
        oss << m_line;
        oss << m_p_script_stream->info_for_exception();
        throw xtsc_exception(oss.str());
      }
      else {
        ostringstream oss;
        oss << "Syntax error in line:" << endl;
        oss << m_line;
        oss << m_p_script_stream->info_for_exception();
        throw xtsc_exception(oss.str());
      }
    }

  }
  catch (const sc_unwind_exception& error) { throw; }
  catch (const exception& error) {
    ostringstream oss;
    oss << "std::exception caught in SC_THREAD, script_thread, of " << kind() << " '" << name() << "'." << endl;
    oss << "what(): " << error.what() << endl;
    xtsc_log_multiline(m_text, log4xtensa::FATAL_LOG_LEVEL, oss.str(), 2);
    throw;
  }

}



int xtsc_component::xtsc_memory::get_words() {
  m_line_count = m_p_script_stream->get_words(m_words, m_line, true);
  if (m_words.size()) {
    XTSC_DEBUG(m_text, "get_words(): " << m_line);
  }
  return m_words.size();
}



u32 xtsc_component::xtsc_memory::get_u32(u32 index, const string& argument_name) {
  u32 value = 0;
  if (index >= m_words.size()) {
    ostringstream oss;
    oss << argument_name << " argument (#" << index+1 << ") missing:" << endl;
    oss << m_line;
    oss << m_p_script_stream->info_for_exception();
    throw xtsc_exception(oss.str());
  }
  try {
    value = xtsc_strtou32(m_words[index]);
  }
  catch (const xtsc_exception&) {
    ostringstream oss;
    oss << "Cannot convert " << argument_name << " argument (#" << index+1 << ") '" << m_words[index] << "' to number:" << endl;
    oss << m_line;
    oss << m_p_script_stream->info_for_exception();
    throw xtsc_exception(oss.str());
  }
  return value;
}



u64 xtsc_component::xtsc_memory::get_u64(u32 index, const string& argument_name) {
  u64 value = 0;
  if (index >= m_words.size()) {
    ostringstream oss;
    oss << argument_name << " argument (#" << index+1 << ") missing:" << endl;
    oss << m_line;
    oss << m_p_script_stream->info_for_exception();
    throw xtsc_exception(oss.str());
  }
  try {
    value = xtsc_strtou64(m_words[index]);
  }
  catch (const xtsc_exception&) {
    ostringstream oss;
    oss << "Cannot convert " << argument_name << " argument (#" << index+1 << ") '" << m_words[index] << "' to number:" << endl;
    oss << m_line;
    oss << m_p_script_stream->info_for_exception();
    throw xtsc_exception(oss.str());
  }
  return value;
}



double xtsc_component::xtsc_memory::get_double(u32 index, const string& argument_name) {
  double value = 0;
  if (index >= m_words.size()) {
    ostringstream oss;
    oss << argument_name << " argument (#" << index+1 << ") missing:" << endl;
    oss << m_line;
    oss << m_p_script_stream->info_for_exception();
    throw xtsc_exception(oss.str());
  }
  try {
    value = xtsc_strtod(m_words[index]);
  }
  catch (const xtsc_exception&) {
    ostringstream oss;
    oss << "Cannot convert " << argument_name << " argument (#" << index+1 << ") '" << m_words[index] << "' to number:" << endl;
    oss << m_line;
    oss << m_p_script_stream->info_for_exception();
    throw xtsc_exception(oss.str());
  }
  return value;
}



bool xtsc_component::xtsc_memory::get_addresses(u32             index,
                                                const string&   argument_name,
                                                xtsc_address&   low_address,
                                                xtsc_address&   high_address)
{
  bool is_range = false;
  if (index >= m_words.size()) {
    ostringstream oss;
    oss << argument_name << " argument (#" << index+1 << ") missing:" << endl;
    oss << m_line;
    oss << m_p_script_stream->info_for_exception();
    throw xtsc_exception(oss.str());
  }
  if (m_words[index] == "*") {
    low_address  = m_start_address8;
    high_address = m_end_address8;
    return true;
  }
  string low  = m_words[index];
  string high = "";
  string::size_type pos = low.find_first_of("-");
  if ((pos != string::npos) && (pos != 0)) {
    is_range = true;
    high = low.substr(pos+1);
    low  = low.substr(0, pos);
  }
  try {
    low_address = xtsc_strtou64(low);
    if (is_range) {
      high_address = xtsc_strtou64(high);
      if (high_address < low_address) {
        ostringstream oss;
        oss << "highAddr (0x" << hex << high_address << ") cannot be less than low_address (0x" << low_address << "):" << endl;
        oss << m_line;
        oss << m_p_script_stream->info_for_exception();
        throw xtsc_exception(oss.str());
      }
    }
    else {
      high_address = low_address;
    }
  }
  catch (const xtsc_exception&) {
    ostringstream oss;
    oss << "Cannot convert " << argument_name << " argument (#" << index+1 << ") '" << m_words[index] << "' to address(es):" << endl;
    oss << m_line;
    oss << m_p_script_stream->info_for_exception();
    throw xtsc_exception(oss.str());
  }
  return is_range;
}



void xtsc_component::xtsc_memory::clear_addresses() {
  XTSC_INFO(m_text, "Clearing all addresses");
  for (u32 i=0; i<2; ++i) {
    map<xtsc_address, address_info*> *p_map = (i ? &m_address_range_map : &m_address_map);
    map<xtsc_address, address_info*>::iterator im;
    for (im = p_map->begin(); im != p_map->end(); ++im) {
      delete im->second;
    }
    p_map->clear();
  }
}



void xtsc_component::xtsc_memory::dump_addresses(ostream& os) {
  for (u32 i=0; i<2; ++i) {
    map<xtsc_address, address_info*> *p_map = (i ? &m_address_range_map : &m_address_map);
    map<xtsc_address, address_info*>::iterator im;
    for (im = p_map->begin(); im != p_map->end(); ++im) {
      os << *im->second << endl;
    }
  }
}



void xtsc_component::xtsc_memory::dump_exclusive_monitors(ostream& os) {
  for (map<u64, address_range>::iterator i = m_exclusive_monitor_map.begin(); i != m_exclusive_monitor_map.end(); ++i) {
    os << "0x" << hex << i->first << ":0x" << i->second.first << "-0x" << i->second.second << endl;
  }
}



bool xtsc_component::xtsc_memory::compute_special_response(const xtsc::xtsc_request&    request,
                                                           xtsc::u32                    port_num,
                                                           xtsc_response::status_t&     status,
                                                           bool&                        list,
                                                           xtsc::u32&                   type)
{
  bool hit = false;
  xtsc_address address8 = request.get_byte_address();
  switch (request.get_type()) {
      case xtsc_request::READ:          type = 0; break;
      case xtsc_request::BLOCK_READ:    type = 1; break;
      case xtsc_request::RCW:           type = 2; break;
      case xtsc_request::WRITE:         type = 3; break;
      case xtsc_request::BLOCK_WRITE:   type = 4; break;
      case xtsc_request::BURST_READ:    type = 6; break;
      case xtsc_request::BURST_WRITE:   type = 7; break;
      default:  throw xtsc_exception("PROGRAM BUG: Unknown xtsc_request type in xtsc_memory::compute_special_response()");
  }
  address_info *p_info = NULL;
  map<xtsc_address, address_info*>::iterator im = m_address_map.find(address8);
  if (im != m_address_map.end() && (!im->second->m_finished)) {
    p_info = im->second;
  }
  else {
    for (im = m_address_range_map.begin(); im != m_address_range_map.end(); ++im) {
      address_info &info = *im->second;
      if (!info.m_finished && (info.m_low_address <= address8) && (address8 <= info.m_high_address)) {
        p_info = &info;
        break;
      }
    }
  }
  // Did we find this address in one of the address lists?
  if (p_info) {
    // Does the request type match (or is it a don't care)?
    if ((p_info->m_type == type) || (p_info->m_type == 5)) {
      // Does the port number match (or is it a don't care)?
      if ((p_info->m_port_num == port_num) || (p_info->m_port_num == m_num_ports)) {
        bool last = request.get_last_transfer();
        if (last || !m_last) {
          XTSC_VERBOSE(m_text, "0x" << hex << setw(xtsc_address_nibbles()) << setfill('0') << address8 << ": Special response: " << *p_info);
          status = p_info->m_status;
          list   = p_info->m_list;
          p_info->used();
          hit = true;
          if (m_lua_function != "") {
            ostringstream exp;
            exp << m_lua_function << "(" << port_num << ", 0x" << hex << address8 << ", \"" << request.get_type_name() << "\", " << boolalpha << last
                << ", \"" << request << "\")";
            string result = m_p_script_stream->evaluate_lua_expression(exp.str());
            XTSC_INFO(m_text, exp.str() << " => " << result);
          }
        }
      }
    }
  }
  return hit;
}



u32 xtsc_component::xtsc_memory::get_request_type_code(u32 index) {
  u32 type = 0;
       if (m_words[index] == "*"          ) { type = 5; }
  else if (m_words[index] == "read"       ) { type = 0; }
  else if (m_words[index] == "block_read" ) { type = 1; }
  else if (m_words[index] == "rcw"        ) { type = 2; }
  else if (m_words[index] == "write"      ) { type = 3; }
  else if (m_words[index] == "block_write") { type = 4; }
  else if (m_words[index] == "burst_read" ) { type = 6; }
  else if (m_words[index] == "burst_write") { type = 7; }
  else {
    ostringstream oss;
    oss << "type argument (#" << index << ") invalid (expected *|read|block_read|rcw|write|block_write|burst_read|burst_write) in file '"
        << m_script_file << "' on line #" << m_line_count << ": " << endl;
    oss << m_line;
    throw xtsc_exception(oss.str());
  }
  return type;
}



void xtsc_component::xtsc_memory::address_info::dump(ostream& os) const {

  // Save state of stream
  char c = os.fill('0');
  ios::fmtflags old_flags = os.flags();
  streamsize w = os.width();
  os << right;

  u32 n = xtsc_address_nibbles();

  os << hex << "0x" << setfill('0') << setw(n) << m_low_address;
  if (m_is_range) {
    os << "-0x" << setw(n) << m_high_address;
  }
  else {
    os << "           ";
  }
  os << " " << dec;
  if (m_port_num == m_num_ports) {
    os << "*";
  }
  else {
    os << m_port_num;
  }
  os << " ";
  os << ((m_type == 0) ? "read       " :
         (m_type == 1) ? "block_read " :
         (m_type == 2) ? "rcw        " :
         (m_type == 3) ? "write      " :
         (m_type == 4) ? "block_write" :
         (m_type == 6) ? "burst_read " :
         (m_type == 7) ? "burst_write" :
                         "*          ");
  os << " " << xtsc_response::get_status_name(m_status) << " " << m_count << "/";
  if (m_limit) {
    os << m_limit;
  }
  else {
    os << "<NoLimit>";
  }
  if (m_finished) {
    os << " Finished";
  }

  // Restore state of stream
  os.fill(c);
  os.flags(old_flags);
  os.width(w);

}



u32 xtsc_component::xtsc_memory::random() {
  m_z = 0x9069 * (m_z & 0xFFFF) + (m_z >> 16);
  m_w = 0x4650 * (m_w & 0xFFFF) + (m_w >> 16);
  return (m_z << 16) + m_w;
}



bool xtsc_component::xtsc_memory::address_info::used() {
  m_count += 1;
  if (m_limit && m_count >= m_limit) {
    m_finished = true;
  }
  return m_finished;
}



std::ostream& xtsc_component::operator<<(std::ostream& os, const xtsc_component::xtsc_memory::address_info& info) {
  info.dump(os);
  return os;
}



