// Copyright (c) 2006-2017 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of
// Cadence Design Systems, Inc.  They may be adapted and modified by bona fide
// purchasers for internal use, but neither the original nor any adapted
// or modified version may be disclosed or distributed to third parties
// in any manner, medium, or form, in whole or in part, without the prior
// written consent of Cadence Design Systems, Inc.


#include <cstdlib>
#include <ostream>
#include <string>
#include <xtsc/xtsc_memory_tlm2.h>
#include <xtsc/xtsc_tlm2.h>
#include <xtsc/xtsc_core.h>
#include <xtsc/xtsc_master_tlm2.h>
#include <xtsc/xtsc_xttlm2tlm2_transactor.h>


using namespace std;
using namespace sc_core;
using namespace tlm;
using namespace tlm_utils;
using namespace xtsc;
using namespace log4xtensa;



xtsc_component::xtsc_memory_tlm2_parms::xtsc_memory_tlm2_parms(const xtsc_core&  core,
                                                               const char       *memory_interface,
                                                               u32               delay,
                                                               u32               num_ports)
{
  xtsc_core::memory_port mem_port = xtsc_core::get_memory_port(memory_interface, &core);
  if (!core.has_memory_port(mem_port)) {
    ostringstream oss;
    oss << "xtsc_memory_tlm2_parms: core '" << core.name() << "' has no " << memory_interface << " memory interface.";
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

  u32           width8          = core.get_memory_byte_width(mem_port);
  xtsc_address  start_address8  = 0;
  u32           size8           = 0;  // 4GB

  if (xtsc_core::is_system_port(mem_port)) {
    if (delay == 0xFFFFFFFF) {
      delay = core_parms.get_u32("LocalMemoryLatency");
    }
  }
  else {
    if ((mem_port == xtsc_core::MEM_XLMI0LS0) ||
        (mem_port == xtsc_core::MEM_XLMI0LS1) ||
        core_parms.get_bool("SimFullLocalMemAddress"))
    {
      core.get_local_memory_starting_byte_address(mem_port, start_address8);
    }
    core.get_local_memory_byte_size(mem_port, size8);
    delay = core_parms.get_u32("LocalMemoryLatency") - 1;
  }

  init(width8, delay, start_address8, size8, num_ports);

  set("clock_period", core_parms.get_u32("SimClockFactor")*xtsc_get_system_clock_factor());
  if (!xtsc_core::is_system_port(mem_port)) {
    set("check_alignment", false);
  }
  if ((mem_port == xtsc_core::MEM_IROM0) || (mem_port == xtsc_core::MEM_DROM0LS0) || (mem_port == xtsc_core::MEM_DROM0LS1)) {
    set("read_only", true);
  }
}



xtsc_component::xtsc_memory_tlm2::xtsc_memory_tlm2(sc_module_name module_name, const xtsc_memory_tlm2_parms& memory_parms) :
  sc_module                     (module_name),
  xtsc_module                   (*(sc_module*)this),
  m_target_sockets_4            (NULL),
  m_target_sockets_8            (NULL),
  m_target_sockets_16           (NULL),
  m_target_sockets_32           (NULL),
  m_target_sockets_64           (NULL),
  m_memory_parms                (memory_parms),
  m_num_ports                   (memory_parms.get_non_zero_u32  ("num_ports")),
  m_tlm_fw_transport_if_impl    (NULL),
  m_p_memory                    (NULL),
  m_nb2b_thread_peq             (NULL),
  m_port_nb2b_thread            (0),
  m_tlm_accepted_thread_peq     (NULL),
  m_port_tlm_accepted_thread    (0),
  m_immediate_timing            (memory_parms.get_bool          ("immediate_timing")),
  m_check_alignment             (memory_parms.get_bool          ("check_alignment")),
  m_check_page_boundary         (memory_parms.get_bool          ("check_page_boundary")),
  m_annotate_delay              (memory_parms.get_bool          ("annotate_delay")),
  m_nb_transport_delay          (SC_ZERO_TIME),
  m_test_tlm_accepted           (memory_parms.get_bool          ("test_tlm_accepted")),
  m_test_end_req_phase          (memory_parms.get_bool          ("test_end_req_phase")),
  m_read_only                   (memory_parms.get_bool          ("read_only")),
  m_host_shared_memory          (memory_parms.get_bool          ("host_shared_memory")),
  m_allow_dmi                   (memory_parms.get_bool          ("allow_dmi")),
  m_deny_fast_access            (memory_parms.get_u64_vector    ("deny_fast_access")),
  m_tlm_response_status         (TLM_OK_RESPONSE),
  m_text                        (log4xtensa::TextLogger::getInstance(name()))
{
  u32                 byte_width              =      memory_parms.get_u32         ("byte_width");
  xtsc_address        start_byte_address      =      memory_parms.get_u64         ("start_byte_address");
  xtsc_address        memory_byte_size        =      memory_parms.get_u64         ("memory_byte_size");
  u32                 page_byte_size          =      memory_parms.get_u32         ("page_byte_size");
  const char         *initial_value_file      =      memory_parms.get_c_str       ("initial_value_file");
  u8                  memory_fill_byte        = (u8) memory_parms.get_u32         ("memory_fill_byte");
  const char         *shared_memory_name      =      memory_parms.get_c_str       ("shared_memory_name");

  m_p_memory = new xtsc_memory_b(name(), kind(), byte_width, start_byte_address, memory_byte_size, page_byte_size, initial_value_file,
                                 memory_fill_byte, m_host_shared_memory, false, shared_memory_name);

  m_start_address8      = m_p_memory->m_start_address8;
  m_size8               = m_p_memory->m_size8;
  m_width8              = m_p_memory->m_width8;
  m_end_address8        = m_p_memory->m_end_address8;

  u32 n = xtsc_address_nibbles();

  // Create all the "per-port" stuff

       if (m_width8 ==  4) { m_target_sockets_4  = new target_socket_4 *[m_num_ports]; }
  else if (m_width8 ==  8) { m_target_sockets_8  = new target_socket_8 *[m_num_ports]; }
  else if (m_width8 == 16) { m_target_sockets_16 = new target_socket_16*[m_num_ports]; }
  else if (m_width8 == 32) { m_target_sockets_32 = new target_socket_32*[m_num_ports]; }
  else if (m_width8 == 64) { m_target_sockets_64 = new target_socket_64*[m_num_ports]; }
  else {
    ostringstream oss;
    oss << kind() << " '" << name() << "': Invalid \"byte_width\" parameter value (" << m_width8 << ").  Expected 4|8|16|32|64.";
    throw xtsc_exception(oss.str());
  }

  m_tlm_fw_transport_if_impl    = new tlm_fw_transport_if_impl         *[m_num_ports];
  m_nb2b_thread_peq             = new peq                              *[m_num_ports];
  m_tlm_accepted_thread_peq     = new peq                              *[m_num_ports];

  for (u32 i=0; i<m_num_ports; ++i) {

    ostringstream oss1;
    oss1 << "m_target_sockets_" << m_width8 << "[" << i << "]";
         if (m_width8 ==  4) { m_target_sockets_4 [i] = new target_socket_4 (oss1.str().c_str()); }
    else if (m_width8 ==  8) { m_target_sockets_8 [i] = new target_socket_8 (oss1.str().c_str()); }
    else if (m_width8 == 16) { m_target_sockets_16[i] = new target_socket_16(oss1.str().c_str()); }
    else if (m_width8 == 32) { m_target_sockets_32[i] = new target_socket_32(oss1.str().c_str()); }
    else if (m_width8 == 64) { m_target_sockets_64[i] = new target_socket_64(oss1.str().c_str()); }

    ostringstream oss2;
    oss2 << "m_tlm_fw_transport_if_impl[" << i << "]";
    m_tlm_fw_transport_if_impl[i] = new tlm_fw_transport_if_impl(oss2.str().c_str(), *this, i);
         if (m_width8 ==  4) { (*m_target_sockets_4 [i])(*m_tlm_fw_transport_if_impl[i]); }
    else if (m_width8 ==  8) { (*m_target_sockets_8 [i])(*m_tlm_fw_transport_if_impl[i]); }
    else if (m_width8 == 16) { (*m_target_sockets_16[i])(*m_tlm_fw_transport_if_impl[i]); }
    else if (m_width8 == 32) { (*m_target_sockets_32[i])(*m_tlm_fw_transport_if_impl[i]); }
    else if (m_width8 == 64) { (*m_target_sockets_64[i])(*m_tlm_fw_transport_if_impl[i]); }

    ostringstream oss3;
    oss3 << "nb2b_thread_" << i;
    declare_thread_process(nb2b_thread_handle, oss3.str().c_str(), SC_CURRENT_USER_MODULE, nb2b_thread);
    m_process_handles.push_back(sc_get_current_process_handle());

    ostringstream oss4;
    oss4 << "m_nb2b_thread_peq_" << i;
    m_nb2b_thread_peq[i] = new peq(oss4.str().c_str());

    ostringstream oss5;
    oss5 << "tlm_accepted_thread_" << i;
    declare_thread_process(tlm_accepted_thread_handle, oss5.str().c_str(), SC_CURRENT_USER_MODULE, tlm_accepted_thread);
    m_process_handles.push_back(sc_get_current_process_handle());

    ostringstream oss6;
    oss6 << "m_tlm_accepted_thread_peq_" << i;
    m_tlm_accepted_thread_peq[i] = new peq(oss6.str().c_str());

  }

  // Get clock period 
  u32 clock_period = memory_parms.get_u32("clock_period");
  if (clock_period == 0xFFFFFFFF) {
    m_clock_period = xtsc_get_system_clock_period();
  }
  else {
    m_clock_period = sc_get_time_resolution() * clock_period;
  }

  compute_delays();

  if (m_deny_fast_access.size()) {
    if (m_deny_fast_access.size() & 0x1) {
      ostringstream oss;
      oss << kind() << " '" << name() << "': " << "\"deny_fast_access\" parameter has " << m_deny_fast_access.size()
          << " elements which is not an even number as it must be.";
      throw xtsc_exception(oss.str());
    }
    for (u32 i=0; i<m_deny_fast_access.size(); i+=2) {
      xtsc_address begin = m_deny_fast_access[i];
      xtsc_address end   = m_deny_fast_access[i+1];
      if ((begin < m_start_address8) || (begin > m_end_address8) ||
          (end   < m_start_address8) || (end   > m_end_address8) ||
          (end   < begin)) {
        ostringstream oss;
        oss << kind() << " '" << name() << "': " << "\"deny_fast_access\" range [0x" << hex << setfill('0') << setw(n) << begin
            << "-0x" << setw(n) << end << "] is invalid." << endl;
        oss << "Valid ranges must fit within [0x" << setw(n) << m_start_address8 << "-0x" << setw(n) << m_end_address8
            << "].";
        throw xtsc_exception(oss.str());
      }
    }
  }

  for (u32 i=0; i<m_num_ports; ++i) {
    ostringstream oss;
    oss.str(""); oss << "m_target_sockets_" << m_width8 << "[" << i << "]";
    switch (m_width8) {
      case  4:  m_port_types[oss.str()] = TARGET_SOCKET_4;  break;
      case  8:  m_port_types[oss.str()] = TARGET_SOCKET_8;  break;
      case 16:  m_port_types[oss.str()] = TARGET_SOCKET_16; break;
      case 32:  m_port_types[oss.str()] = TARGET_SOCKET_32; break;
      case 64:  m_port_types[oss.str()] = TARGET_SOCKET_64; break;
    }
    oss.str(""); oss << "target_socket[" << i << "]"; m_port_types[oss.str()] = PORT_TABLE;
  }

  m_port_types["target_sockets"] = PORT_TABLE;

  if (m_num_ports == 1) {
    m_port_types["target_socket"] = PORT_TABLE;
  }

  xtsc_register_command(*this, *this, "allow_dmi", 1, 1,
      "allow_dmi 0|1", 
      "Set allow_dmi flag to true or false.  Return previous value."
  );

  xtsc_register_command(*this, *this, "change_clock_period", 1, 1,
      "change_clock_period <ClockPeriodFactor>", 
      "Call xtsc_memory_tlm2::change_clock_period(<ClockPeriodFactor>).  Return previous <ClockPeriodFactor> for this device."
  );

  xtsc_register_command(*this, *this, "invalidate_direct_mem_ptr", 2, 3,
      "invalidate_direct_mem_ptr <StartAddress> <EndAddress> [<Port>]", 
      "Call xtsc_memory_tlm2::invalidate_direct_mem_ptr(<StartAddress>, <EndAddress>, <Port>).  Default <Port> is 0."
  );

  if (!m_immediate_timing)
  xtsc_register_command(*this, *this, "nb_transport_delay", 1, 1,
      "nb_transport_delay <NumClockPeriods>", 
      "Set m_nb_transport_delay to (m_clock_period * <NumClockPeriods>).  Return previous <NumClockPeriods>."
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
      "Call xtsc_memory_tlm2::reset(<Hard>).  Where <Hard> is 0|1 (default 0)."
  );

  if (!m_immediate_timing)
  xtsc_register_command(*this, *this, "test_end_req_phase", 1, 1,
      "test_end_req_phase 0|1", 
      "Set m_test_end_req_phase to 0|1.  Return previous value of m_test_end_req_phase."
  );

  if (!m_immediate_timing)
  xtsc_register_command(*this, *this, "test_tlm_accepted", 1, 1,
      "test_tlm_accepted 0|1", 
      "Set m_test_tlm_accepted to 0|1.  Return previous value of m_test_tlm_accepted."
  );

  xtsc_register_command(*this, *this, "tlm_response_status", 1, 1,
      "tlm_response_status <Status>", 
      "Call tlm_generic_payload::set_response_status(<Status>)."
  );

  log4xtensa::LogLevel ll = xtsc_get_constructor_log_level();
  XTSC_LOG(m_text, ll,        "Constructed " << kind() << " '" << name() << "':");
  XTSC_LOG(m_text, ll,        " num_ports               = "   << m_num_ports);
  XTSC_LOG(m_text, ll, hex << " start_byte_address      = 0x" << m_start_address8);
  XTSC_LOG(m_text, ll, hex << " memory_byte_size        = 0x" << m_size8 << (m_size8 ? " " : " (4GB boundary)"));
  XTSC_LOG(m_text, ll, hex << " End byte address        = 0x" << m_end_address8);
  if (m_width8) {
  XTSC_LOG(m_text, ll,        " byte_width              = "   << m_width8);
  } else {
  XTSC_LOG(m_text, ll,        " byte_width              = 0 => 4|8|16|32|64");
  }
  XTSC_LOG(m_text, ll,        " check_alignment         = "   << boolalpha << m_check_alignment);
  XTSC_LOG(m_text, ll,        " check_page_boundary     = "   << boolalpha << m_check_page_boundary);
  XTSC_LOG(m_text, ll, hex << " page_byte_size          = 0x" << m_p_memory->m_page_size8);
  XTSC_LOG(m_text, ll, hex << " initial_value_file      = "   << m_p_memory->m_initial_value_file);
  XTSC_LOG(m_text, ll, hex << " memory_fill_byte        = 0x" << (u32) m_p_memory->m_memory_fill_byte);
  XTSC_LOG(m_text, ll,        " host_shared_memory      = "   << boolalpha << m_host_shared_memory);
  XTSC_LOG(m_text, ll,        " shared_memory_name      = "   << m_p_memory->get_shared_memory_name());
  XTSC_LOG(m_text, ll,        " immediate_timing        = "   << boolalpha << m_immediate_timing);
  XTSC_LOG(m_text, ll,        " read_only               = "   << boolalpha << m_read_only);
  if (!m_immediate_timing) {
  if (clock_period == 0xFFFFFFFF) {
  XTSC_LOG(m_text, ll, hex << " clock_period            = 0x" << clock_period << " (" << m_clock_period << ")");
  } else {
  XTSC_LOG(m_text, ll,        " clock_period            = "   << clock_period << " (" << m_clock_period << ")");
  }
  XTSC_LOG(m_text, ll,        " read_delay              = "   << memory_parms.get_u32("read_delay"));
  XTSC_LOG(m_text, ll,        " write_delay             = "   << memory_parms.get_u32("write_delay"));
  XTSC_LOG(m_text, ll,        " burst_read_delay        = "   << memory_parms.get_u32("burst_read_delay"));
  XTSC_LOG(m_text, ll,        " burst_read_repeat       = "   << memory_parms.get_u32("burst_read_repeat"));
  XTSC_LOG(m_text, ll,        " burst_write_delay       = "   << memory_parms.get_u32("burst_write_delay"));
  XTSC_LOG(m_text, ll,        " burst_write_repeat      = "   << memory_parms.get_u32("burst_write_repeat"));
  XTSC_LOG(m_text, ll,        " annotate_delay          = "   << boolalpha << m_annotate_delay);
  XTSC_LOG(m_text, ll,        " nb_transport_delay      = "   << memory_parms.get_u32("nb_transport_delay"));
  XTSC_LOG(m_text, ll,        " test_tlm_accepted       = "   << boolalpha << m_test_tlm_accepted);
  XTSC_LOG(m_text, ll,        " test_end_req_phase      = "   << boolalpha << m_test_end_req_phase);
  }
  XTSC_LOG(m_text, ll,        " allow_dmi               = "   << boolalpha << m_allow_dmi);
  XTSC_LOG(m_text, ll,        " deny_fast_access       :");
  for (u32 i=0; i<m_deny_fast_access.size(); i+=2) {
  XTSC_LOG(m_text, ll, hex << "  [0x" << setfill('0') << setw(n) << m_deny_fast_access[i] << "-0x"
                                                      << setw(n) << m_deny_fast_access[i+1] << "]");
  }

  reset(true);

}



xtsc_component::xtsc_memory_tlm2::~xtsc_memory_tlm2(void) {
  XTSC_DEBUG(m_text, "In ~xtsc_memory_tlm2()");
  if (m_p_memory) {
    delete m_p_memory;
    m_p_memory = 0;
  }
}



u32 xtsc_component::xtsc_memory_tlm2::get_bit_width(const string& port_name, u32 interface_num) const {
  return m_width8*8;
}



sc_object *xtsc_component::xtsc_memory_tlm2::get_port(const string& port_name) {
  string name_portion;
  u32    index;
  bool   indexed = xtsc_parse_port_name(port_name, name_portion, index);

  if (((m_num_ports > 1) && !indexed) ||
      (((name_portion != "m_target_sockets_4" ) || (m_width8 !=  4)) &&
       ((name_portion != "m_target_sockets_8" ) || (m_width8 !=  8)) &&
       ((name_portion != "m_target_sockets_16") || (m_width8 != 16)) &&
       ((name_portion != "m_target_sockets_32") || (m_width8 != 32)) &&
       ((name_portion != "m_target_sockets_64") || (m_width8 != 64))))
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

       if (name_portion == "m_target_sockets_4" )  return m_target_sockets_4 [index];
  else if (name_portion == "m_target_sockets_8" )  return m_target_sockets_8 [index];
  else if (name_portion == "m_target_sockets_16")  return m_target_sockets_16[index];
  else if (name_portion == "m_target_sockets_32")  return m_target_sockets_32[index];
  else if (name_portion == "m_target_sockets_64")  return m_target_sockets_64[index];
  else {
    throw xtsc_exception("Program Bug in xtsc_memory_tlm2.cpp");
  }
}



xtsc_port_table xtsc_component::xtsc_memory_tlm2::get_port_table(const string& port_table_name) const {
  xtsc_port_table table;

  if (port_table_name == "target_sockets") {
    for (u32 i=0; i<m_num_ports; ++i) {
      ostringstream oss;
      oss << "target_socket[" << i << "]";
      table.push_back(oss.str());
    }
    return table;
  }
  
  string name_portion;
  u32    index;
  bool   indexed = xtsc_parse_port_name(port_table_name, name_portion, index);

  if (((m_num_ports > 1) && !indexed) || (name_portion != "target_socket")) {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\" has no port table named \"" << port_table_name << "\"" << endl;
    throw xtsc_exception(oss.str());
  }

  if (index > m_num_ports - 1) {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\": Invalid index: \"" << port_table_name << "\".  Valid range: 0-" << (m_num_ports-1) << endl;
    throw xtsc_exception(oss.str());
  }

  ostringstream oss;

  oss.str(""); oss << "m_target_sockets_" << m_width8 << "[" << index<< "]"; table.push_back(oss.str());

  return table;
}



xtsc_component::xtsc_memory_tlm2::target_socket_4& xtsc_component::xtsc_memory_tlm2::get_target_socket_4(xtsc::u32 port_num) {
  if (m_width8 != 4) {
    ostringstream oss;
    oss << " '" << name() << "': xtsc_memory_tlm2::get_target_socket_4() called but bus width is " << m_width8
        << " so you may only call get_target_socket_" << m_width8 << "()";
    throw xtsc_exception(oss.str());
  }
  if (port_num >= m_num_ports) {
    ostringstream oss;
    oss << " '" << name() << "': xtsc_memory_tlm2::get_target_socket_4() called with port_num=" << port_num
        << " out-of-range.  Max port_num is " << (m_num_ports - 1);
    throw xtsc_exception(oss.str());
  }
  return *m_target_sockets_4[port_num];
}



xtsc_component::xtsc_memory_tlm2::target_socket_8& xtsc_component::xtsc_memory_tlm2::get_target_socket_8(xtsc::u32 port_num) {
  if (m_width8 != 8) {
    ostringstream oss;
    oss << " '" << name() << "': xtsc_memory_tlm2::get_target_socket_8() called but bus width is " << m_width8
        << " so you may only call get_target_socket_" << m_width8 << "()";
    throw xtsc_exception(oss.str());
  }
  if (port_num >= m_num_ports) {
    ostringstream oss;
    oss << " '" << name() << "': xtsc_memory_tlm2::get_target_socket_8() called with port_num=" << port_num
        << " out-of-range.  Max port_num is " << (m_num_ports - 1);
    throw xtsc_exception(oss.str());
  }
  return *m_target_sockets_8[port_num];
}



xtsc_component::xtsc_memory_tlm2::target_socket_16& xtsc_component::xtsc_memory_tlm2::get_target_socket_16(xtsc::u32 port_num) {
  if (m_width8 != 16) {
    ostringstream oss;
    oss << " '" << name() << "': xtsc_memory_tlm2::get_target_socket_16() called but bus width is " << m_width8
        << " so you may only call get_target_socket_" << m_width8 << "()";
    throw xtsc_exception(oss.str());
  }
  if (port_num >= m_num_ports) {
    ostringstream oss;
    oss << " '" << name() << "': xtsc_memory_tlm2::get_target_socket_16() called with port_num=" << port_num
        << " out-of-range.  Max port_num is " << (m_num_ports - 1);
    throw xtsc_exception(oss.str());
  }
  return *m_target_sockets_16[port_num];
}



xtsc_component::xtsc_memory_tlm2::target_socket_32& xtsc_component::xtsc_memory_tlm2::get_target_socket_32(xtsc::u32 port_num) {
  if (m_width8 != 32) {
    ostringstream oss;
    oss << " '" << name() << "': xtsc_memory_tlm2::get_target_socket_32() called but bus width is " << m_width8
        << " so you may only call get_target_socket_" << m_width8 << "()";
    throw xtsc_exception(oss.str());
  }
  if (port_num >= m_num_ports) {
    ostringstream oss;
    oss << " '" << name() << "': xtsc_memory_tlm2::get_target_socket_32() called with port_num=" << port_num
        << " out-of-range.  Max port_num is " << (m_num_ports - 1);
    throw xtsc_exception(oss.str());
  }
  return *m_target_sockets_32[port_num];
}



xtsc_component::xtsc_memory_tlm2::target_socket_64& xtsc_component::xtsc_memory_tlm2::get_target_socket_64(xtsc::u32 port_num) {
  if (m_width8 != 64) {
    ostringstream oss;
    oss << " '" << name() << "': xtsc_memory_tlm2::get_target_socket_64() called but bus width is " << m_width8
        << " so you may only call get_target_socket_" << m_width8 << "()";
    throw xtsc_exception(oss.str());
  }
  if (port_num >= m_num_ports) {
    ostringstream oss;
    oss << " '" << name() << "': xtsc_memory_tlm2::get_target_socket_64() called with port_num=" << port_num
        << " out-of-range.  Max port_num is " << (m_num_ports - 1);
    throw xtsc_exception(oss.str());
  }
  return *m_target_sockets_64[port_num];
}



void xtsc_component::xtsc_memory_tlm2::change_clock_period(u32 clock_period_factor) {
  m_clock_period = sc_get_time_resolution() * clock_period_factor;
  XTSC_INFO(m_text, "change_clock_period(" << clock_period_factor << ") => " << m_clock_period);
  compute_delays();
}



void xtsc_component::xtsc_memory_tlm2::compute_delays() {
  // Convert delays from an integer number of periods to sc_time values
  m_read_delay          = m_clock_period * m_memory_parms.get_u32("read_delay");
  m_burst_read_delay    = m_clock_period * m_memory_parms.get_u32("burst_read_delay");
  m_burst_read_repeat   = m_clock_period * m_memory_parms.get_u32("burst_read_repeat");
  m_write_delay         = m_clock_period * m_memory_parms.get_u32("write_delay");
  m_burst_write_delay   = m_clock_period * m_memory_parms.get_u32("burst_write_delay");
  m_burst_write_repeat  = m_clock_period * m_memory_parms.get_u32("burst_write_repeat");
  m_nb_transport_delay  = m_clock_period * m_memory_parms.get_u32("nb_transport_delay");
}



void xtsc_component::xtsc_memory_tlm2::execute(const string&                 cmd_line, 
                                               const vector<string>&         words,
                                               const vector<string>&         words_lc,
                                               ostream&                      result)
{
  ostringstream res;

  if (false) {
  }
  else if (words[0] == "allow_dmi") {
    res << m_allow_dmi;
    m_allow_dmi = xtsc_command_argtobool(cmd_line, words, 1);
  }
  else if (words[0] == "change_clock_period") {
    u32 clock_period_factor = xtsc_command_argtou32(cmd_line, words, 1);
    res << m_clock_period.value();
    change_clock_period(clock_period_factor);
  }
  else if (words[0] == "invalidate_direct_mem_ptr") {
    u64 start_address = xtsc_command_argtou64(cmd_line, words, 1);
    u64 end_address   = xtsc_command_argtou64(cmd_line, words, 2);
    u32 port          = ((words.size() == 3) ? 0 : xtsc_command_argtou32(cmd_line, words, 3));
    invalidate_direct_mem_ptr(port, start_address, end_address);
  }
  else if (words[0] == "nb_transport_delay") {
    u32 num_clock_periods = xtsc_command_argtou32(cmd_line, words, 1);
    res << m_nb_transport_delay.value() / m_clock_period.value();
    m_nb_transport_delay = m_clock_period * num_clock_periods;
  }
  else if (words[0] == "peek") {
    u64 start_address = xtsc_command_argtou64(cmd_line, words, 1);
    u32 num_bytes     = xtsc_command_argtou32(cmd_line, words, 2);
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
      poke(start_address, num_bytes, buffer);
      delete [] buffer;
    }
  }
  else if (words[0] == "reset") {
    reset((words.size() == 1) ? false : xtsc_command_argtobool(cmd_line, words, 1));
  }
  else if (words[0] == "test_end_req_phase") {
    bool new_value = xtsc_command_argtobool(cmd_line, words, 1);
    res << (m_test_end_req_phase ? 1 : 0);
    m_test_end_req_phase = new_value;
  }
  else if (words[0] == "test_tlm_accepted") {
    bool new_value = xtsc_command_argtobool(cmd_line, words, 1);
    res << (m_test_tlm_accepted ? 1 : 0);
    m_test_tlm_accepted = new_value;
  }
  else if (words[0] == "tlm_response_status") {
    tlm_response_status new_status = m_tlm_response_status;
    if (false) {}
    else if (words_lc[1] == "tlm_ok_response"               ) { new_status = TLM_OK_RESPONSE;                }
    else if (words_lc[1] == "tlm_incomplete_response"       ) { new_status = TLM_INCOMPLETE_RESPONSE;        }
    else if (words_lc[1] == "tlm_generic_error_response"    ) { new_status = TLM_GENERIC_ERROR_RESPONSE;     }
    else if (words_lc[1] == "tlm_address_error_response"    ) { new_status = TLM_ADDRESS_ERROR_RESPONSE;     }
    else if (words_lc[1] == "tlm_command_error_response"    ) { new_status = TLM_COMMAND_ERROR_RESPONSE;     }
    else if (words_lc[1] == "tlm_burst_error_response"      ) { new_status = TLM_BURST_ERROR_RESPONSE;       }
    else if (words_lc[1] == "tlm_byte_enable_error_response") { new_status = TLM_BYTE_ENABLE_ERROR_RESPONSE; }
    else { new_status = (tlm_response_status)xtsc_command_argtoi32(cmd_line, words, 1); }
    res << m_tlm_response_status;
    m_tlm_response_status = new_status;
  }
  else {
    ostringstream oss;
    oss << name() << "::" << __FUNCTION__ << "() called for unknown command '" << cmd_line << "'.";
    throw xtsc_exception(oss.str());
  }

  result << res.str();
}



void xtsc_component::xtsc_memory_tlm2::connect(xtsc_master_tlm2& master_tlm2, u32 target_socket) {
  if (target_socket >= m_num_ports) {
    ostringstream oss;
    oss << "Invalid target_socket=" << target_socket << " in connect(): " << endl;
    oss << kind() << " '" << name() << "' has " << m_num_ports << " sockets numbered from 0 to " << m_num_ports-1 << endl;
    throw xtsc_exception(oss.str());
  }

  if (master_tlm2.get_byte_width() != m_width8) {
    ostringstream oss;
    oss << "Mismatched byte widths in connect(): " << endl;
    oss << kind() << " '" << name() << "' has \"byte_width\"=" << m_width8 << endl;
    oss << master_tlm2.kind() << " '" << master_tlm2.name() << "' has \"byte_width\"=" << master_tlm2.get_byte_width() << endl;
    throw xtsc_exception(oss.str());
  }

  switch (m_width8) {
    case  4: master_tlm2.get_initiator_socket_4 ()(get_target_socket_4 (target_socket)); break;
    case  8: master_tlm2.get_initiator_socket_8 ()(get_target_socket_8 (target_socket)); break;
    case 16: master_tlm2.get_initiator_socket_16()(get_target_socket_16(target_socket)); break;
    case 32: master_tlm2.get_initiator_socket_32()(get_target_socket_32(target_socket)); break;
    case 64: master_tlm2.get_initiator_socket_64()(get_target_socket_64(target_socket)); break;
    default: throw xtsc_exception("Unhandled m_width8 in xtsc_memory_tlm2.cpp");
  }

}



u32 xtsc_component::xtsc_memory_tlm2::connect(xtsc_xttlm2tlm2_transactor&       xttlm2tlm2,
                                              u32                               initiator_socket,
                                              u32                               target_socket,
                                              bool                              single_connect)
{
  u32 initiator_sockets = xttlm2tlm2.get_num_ports();

  if (initiator_socket >= initiator_sockets) {
    ostringstream oss;
    oss << "Invalid initiator_socket=" << initiator_socket << " in connect(): " << endl;
    oss << kind() << " '" << name() << "' has " << initiator_sockets << " sockets numbered from 0 to " << initiator_sockets-1 << endl;
    throw xtsc_exception(oss.str());
  }

  if (target_socket >= m_num_ports) {
    ostringstream oss;
    oss << "Invalid target_socket=" << target_socket << " in connect(): " << endl;
    oss << kind() << " '" << name() << "' has " << m_num_ports << " sockets numbered from 0 to " << m_num_ports-1 << endl;
    throw xtsc_exception(oss.str());
  }

  if (xttlm2tlm2.get_byte_width() != m_width8) {
    ostringstream oss;
    oss << "Mismatched byte widths in connect(): " << endl;
    oss << kind() << " '" << name() << "' has \"byte_width\"=" << m_width8 << endl;
    oss << xttlm2tlm2.kind() << " '" << xttlm2tlm2.name() << "' has \"byte_width\"=" << xttlm2tlm2.get_byte_width() << endl;
    throw xtsc_exception(oss.str());
  }

  u32 num_connected = 0;

  while ((initiator_socket < initiator_sockets) && (target_socket < m_num_ports)) {

    switch (m_width8) {
      case  4: xttlm2tlm2.get_initiator_socket_4 (initiator_socket)(get_target_socket_4 (target_socket)); break;
      case  8: xttlm2tlm2.get_initiator_socket_8 (initiator_socket)(get_target_socket_8 (target_socket)); break;
      case 16: xttlm2tlm2.get_initiator_socket_16(initiator_socket)(get_target_socket_16(target_socket)); break;
      case 32: xttlm2tlm2.get_initiator_socket_32(initiator_socket)(get_target_socket_32(target_socket)); break;
      case 64: xttlm2tlm2.get_initiator_socket_64(initiator_socket)(get_target_socket_64(target_socket)); break;
      default: throw xtsc_exception("Unhandled m_width8 in xtsc_memory_tlm2.cpp");
    }

    target_socket    += 1;
    initiator_socket += 1;
    num_connected    += 1;

    if (single_connect) break;
    if (initiator_socket >= initiator_sockets) break;
    if (target_socket >= m_num_ports) break;
  
    if (((m_width8== 4) && get_target_socket_4 (target_socket).get_base_port().get_interface()) ||
        ((m_width8== 8) && get_target_socket_8 (target_socket).get_base_port().get_interface()) ||
        ((m_width8==16) && get_target_socket_16(target_socket).get_base_port().get_interface()) ||
        ((m_width8==32) && get_target_socket_32(target_socket).get_base_port().get_interface()) ||
        ((m_width8==64) && get_target_socket_64(target_socket).get_base_port().get_interface())) break;

    if (((m_width8== 4) && xttlm2tlm2.get_initiator_socket_4 (initiator_socket).get_base_port().get_interface()) ||
        ((m_width8== 8) && xttlm2tlm2.get_initiator_socket_8 (initiator_socket).get_base_port().get_interface()) ||
        ((m_width8==16) && xttlm2tlm2.get_initiator_socket_16(initiator_socket).get_base_port().get_interface()) ||
        ((m_width8==32) && xttlm2tlm2.get_initiator_socket_32(initiator_socket).get_base_port().get_interface()) ||
        ((m_width8==64) && xttlm2tlm2.get_initiator_socket_64(initiator_socket).get_base_port().get_interface())) break;

  }

  return num_connected;
}



void xtsc_component::xtsc_memory_tlm2::reset(bool hard_reset) {
  XTSC_INFO(m_text, kind() << "::reset()");

  m_port_nb2b_thread = 0;
  m_port_tlm_accepted_thread = 0;

  if (hard_reset) {
    load_initial_values();
  }

  xtsc_reset_processes(m_process_handles);
}



void xtsc_component::xtsc_memory_tlm2::invalidate_direct_mem_ptr(u32 port, u64 start_address, u64 end_address) {
  u64 beg_range = (u64) start_address;
  u64 end_range = (u64) end_address;

  assert(port < m_num_ports);

       if (m_width8 ==  4) { (*m_target_sockets_4 [port])->invalidate_direct_mem_ptr(beg_range, end_range); }
  else if (m_width8 ==  8) { (*m_target_sockets_8 [port])->invalidate_direct_mem_ptr(beg_range, end_range); }
  else if (m_width8 == 16) { (*m_target_sockets_16[port])->invalidate_direct_mem_ptr(beg_range, end_range); }
  else if (m_width8 == 32) { (*m_target_sockets_32[port])->invalidate_direct_mem_ptr(beg_range, end_range); }
  else if (m_width8 == 64) { (*m_target_sockets_64[port])->invalidate_direct_mem_ptr(beg_range, end_range); }
  else { throw xtsc_exception("Program Bug in xtsc_memory_tlm2.cpp - unsupported bus width"); }
}



void xtsc_component::xtsc_memory_tlm2::nb2b_thread(void) {

  // Get the port number for this "instance" of nb2b_thread
  u32 port_num = m_port_nb2b_thread++;

#if ((defined(SC_API_VERSION_STRING) && (SC_API_VERSION_STRING != sc_api_version_2_2_0)) || IEEE_1666_SYSTEMC >= 201101L)  
  m_nb2b_thread_peq[port_num]->cancel_all();
#endif

  try {

    sc_event& our_peq_event     = m_nb2b_thread_peq[port_num]->get_event();
    sc_time   time              = SC_ZERO_TIME;
    tlm_phase phase             = BEGIN_RESP;

    while (true) {
      XTSC_TRACE(m_text, "nb2b_thread[" << port_num << "] going to sleep.");
      wait(our_peq_event);
      XTSC_TRACE(m_text, "nb2b_thread[" << port_num << "] woke up.");

      // Drain all transactions from m_nb2b_thread_peq that are due now
      while (tlm_generic_payload *p_trans = m_nb2b_thread_peq[port_num]->get_next_transaction()) { 

        // Forward each transaction to b_transport() for handling 
        time = SC_ZERO_TIME;
        m_tlm_fw_transport_if_impl[port_num]->b_transport(*p_trans, time);

        // Send each transaction back to the TLM2 initiator.
        // We don't care about the tlm_sync_enum return value nor the gp response status.
        time    = SC_ZERO_TIME;
        phase   = BEGIN_RESP;
             if (m_width8 ==  4) { (*m_target_sockets_4 [port_num])->nb_transport_bw(*p_trans, phase, time); }
        else if (m_width8 ==  8) { (*m_target_sockets_8 [port_num])->nb_transport_bw(*p_trans, phase, time); }
        else if (m_width8 == 16) { (*m_target_sockets_16[port_num])->nb_transport_bw(*p_trans, phase, time); }
        else if (m_width8 == 32) { (*m_target_sockets_32[port_num])->nb_transport_bw(*p_trans, phase, time); }
        else if (m_width8 == 64) { (*m_target_sockets_64[port_num])->nb_transport_bw(*p_trans, phase, time); }
        else { throw xtsc_exception("Unsupported m_width8 in nb2b_thread()"); }

      }
    }

  }
  catch (const sc_unwind_exception& error) { throw; }
  catch (const exception& error) {
    ostringstream oss;
    oss << "std::exception caught in nb2b_thread[" << port_num << "] of " << kind() << " '" << name() << "'." << endl;
    oss << "what(): " << error.what() << endl;
    xtsc_log_multiline(m_text, FATAL_LOG_LEVEL, oss.str(), 2);
    throw;
  }

}



void xtsc_component::xtsc_memory_tlm2::tlm_accepted_thread(void) {

  // Get the port number for this "instance" of tlm_accepted_thread
  u32 port_num = m_port_tlm_accepted_thread++;

#if ((defined(SC_API_VERSION_STRING) && (SC_API_VERSION_STRING != sc_api_version_2_2_0)) || IEEE_1666_SYSTEMC >= 201101L)  
  m_tlm_accepted_thread_peq[port_num]->cancel_all();
#endif

  try {

    sc_event& our_peq_event     = m_tlm_accepted_thread_peq[port_num]->get_event();
    sc_time   time              = SC_ZERO_TIME;
    tlm_phase phase             = END_REQ;

    while (true) {
      XTSC_TRACE(m_text, "tlm_accepted_thread[" << port_num << "] going to sleep.");
      wait(our_peq_event);
      XTSC_TRACE(m_text, "tlm_accepted_thread[" << port_num << "] woke up.");

      // Drain all transactions from m_tlm_accepted_thread_peq that are due now
      while (tlm_generic_payload *p_trans = m_tlm_accepted_thread_peq[port_num]->get_next_transaction()) { 
        if (m_test_end_req_phase) {
          // Send each transaction back to the TLM2 initiator.
          // We don't care about the tlm_sync_enum return value nor the gp response status.
          time    = SC_ZERO_TIME;
          phase   = END_REQ;
               if (m_width8 ==  4) { (*m_target_sockets_4 [port_num])->nb_transport_bw(*p_trans, phase, time); }
          else if (m_width8 ==  8) { (*m_target_sockets_8 [port_num])->nb_transport_bw(*p_trans, phase, time); }
          else if (m_width8 == 16) { (*m_target_sockets_16[port_num])->nb_transport_bw(*p_trans, phase, time); }
          else if (m_width8 == 32) { (*m_target_sockets_32[port_num])->nb_transport_bw(*p_trans, phase, time); }
          else if (m_width8 == 64) { (*m_target_sockets_64[port_num])->nb_transport_bw(*p_trans, phase, time); }
          else { throw xtsc_exception("Unsupported m_width8 in tlm_accepted_thread()"); }
          m_nb2b_thread_peq[port_num]->notify(*p_trans, m_clock_period);
        }
        else {
          m_nb2b_thread_peq[port_num]->notify(*p_trans);
        }
      }
    }

  }
  catch (const sc_unwind_exception& error) { throw; }
  catch (const exception& error) {
    ostringstream oss;
    oss << "std::exception caught in tlm_accepted_thread[" << port_num << "] of " << kind() << " '" << name() << "'." << endl;
    oss << "what(): " << error.what() << endl;
    xtsc_log_multiline(m_text, FATAL_LOG_LEVEL, oss.str(), 2);
    throw;
  }

}



tlm_sync_enum xtsc_component::xtsc_memory_tlm2::tlm_fw_transport_if_impl::nb_transport_fw(tlm_generic_payload& trans, tlm_phase& phase, sc_time& t) {
  XTSC_INFO(m_memory.m_text, trans << " " << xtsc_tlm_phase_text(phase) << " (" << t << ") nb_transport_fw[" << m_port_num << "]");
  if (phase == BEGIN_REQ) {
    if (m_memory.m_immediate_timing) {
      b_transport(trans, t);
      return TLM_COMPLETED;
    }
    else if (m_memory.m_test_tlm_accepted) {
      t = t + m_memory.m_nb_transport_delay;
      XTSC_DEBUG(m_memory.m_text, trans << " " << xtsc_tlm_phase_text(phase) << " " << t << " nb_transport_fw[" << m_port_num <<
                                  "] (adding to m_tlm_accepted_thread_peq)");
      m_memory.m_tlm_accepted_thread_peq[m_port_num]->notify(trans, t);
      return TLM_ACCEPTED;
    }
    else {
      t = t + m_memory.m_nb_transport_delay;
      m_memory.m_nb2b_thread_peq[m_port_num]->notify(trans, t);
      phase = END_REQ;
      return TLM_UPDATED;
    }
  }
  else if (phase == END_RESP) {
    return TLM_COMPLETED;
  }
  else if ((phase == END_REQ) || (phase == BEGIN_RESP)) {
    // TLM-2.0.1 LRM, para 8.2.3.c, page 105; also para 8.2.13.4.c, page 124
    ostringstream oss;
    oss << m_memory.kind() << " '" << m_memory.name() << "': nb_transport_fw received illegal phase=" << xtsc_tlm_phase_text(phase)
        << " for trans: " << trans;
    throw xtsc_exception(oss.str());
  }
  else {
    // TLM-2.0.1 LRM, para 8.2.5.b, page 110; also para 8.2.13.4.c, page 124
    XTSC_INFO(m_memory.m_text, trans << " " << xtsc_tlm_phase_text(phase) << " " << t << " nb_transport_fw[" << m_port_num << "] (Ignored)");
    return TLM_ACCEPTED;
  }
}



void xtsc_component::xtsc_memory_tlm2::tlm_fw_transport_if_impl::b_transport(tlm_generic_payload& trans, sc_time& t) {

  if (m_busy) {
    XTSC_INFO(m_memory.m_text, trans << ": (Busy) b_transport[" << m_port_num << "]");
    return;
  }

  tlm_command   cmd     = trans.get_command();
  u64           adr     = trans.get_address();
  u8           *ptr     = trans.get_data_ptr();
  u32           len     = trans.get_data_length();
  u32           sw      = trans.get_streaming_width();
  u8           *bep     = trans.get_byte_enable_ptr();
  u32           bel     = trans.get_byte_enable_length();
  bool          read    = (cmd == TLM_READ_COMMAND);

  if (read) {
    XTSC_VERBOSE(m_memory.m_text, trans << " (" << t << ") b_transport[" << m_port_num << "]");
  }
  else if (cmd != TLM_IGNORE_COMMAND) {
    XTSC_INFO(m_memory.m_text, trans << " (" << t << ") b_transport[" << m_port_num << "]");
  }

  if (cmd == TLM_IGNORE_COMMAND) {
    XTSC_INFO(m_memory.m_text, trans << ": b_transport[" << m_port_num << "] returning TLM_OK_RESPONSE");
    trans.set_response_status(TLM_OK_RESPONSE);
    return;
  }

  if ((cmd != TLM_READ_COMMAND) && (cmd != TLM_WRITE_COMMAND)) {
    ostringstream oss;
    oss << "Unsupported command received in b_transport() of xtsc_memory_tlm2 '" << m_memory.name() << "': " << trans;
    throw xtsc_exception(oss.str());
  }

  if (m_memory.m_read_only && (cmd == TLM_WRITE_COMMAND)) {
    ostringstream oss;
    oss << "Unsupported WRITE command received in b_transport() of read-only xtsc_memory_tlm2 '" << m_memory.name() << "': " << trans;
    throw xtsc_exception(oss.str());
  }

  if (sw < len) {
    ostringstream oss;
    oss << "Streaming transfers are not supported in b_transport() of xtsc_memory_tlm2 '" << m_memory.name() << "': " << trans;
    throw xtsc_exception(oss.str());
  }

  xtsc_address addr = (xtsc_address) adr;

  if (m_memory.m_check_alignment) {
    if ((len < m_memory.m_width8) && (((len !=   1) &&
                                       (len !=   2) &&
                                       (len !=   4) &&
                                       (len !=   8) &&
                                       (len !=  16) &&
                                       (len !=  32) &&
                                       (len !=  64) &&
                                       (len != 128) &&
                                       (len != 256)) || (addr & (len -1))))
    {
      trans.set_response_status(TLM_ADDRESS_ERROR_RESPONSE);
      XTSC_INFO(m_memory.m_text, trans << " b_transport[" << m_port_num << "]");
      return;
    }
    if ((len > m_memory.m_width8) && (len % m_memory.m_width8 != 0)) {
      trans.set_response_status(TLM_BURST_ERROR_RESPONSE);
      XTSC_INFO(m_memory.m_text, trans << " b_transport[" << m_port_num << "]");
      return;
    }
  }

  if (m_memory.m_check_page_boundary) {
    u32 page_of_start_addr        = m_memory.m_p_memory->get_page(addr);
    u32 page_of_end_addr          = m_memory.m_p_memory->get_page(addr+len-1);

    if (page_of_start_addr != page_of_end_addr) {
      trans.set_response_status(TLM_BURST_ERROR_RESPONSE);
      XTSC_DEBUG(m_memory.m_text, trans << " Crossing Page Boundary!");
      XTSC_INFO(m_memory.m_text, trans << " b_transport[" << m_port_num << "]");
      return;
    }
  }

  bool all_enabled = true;
  u8 enabled = TLM_BYTE_ENABLED;
  if (!bep) {
    bep = &enabled;
    bel = 1;
  }
  else {
    for (u32 i=0; i < len; ++i) {
      if (bep[i % bel] != enabled) {
        all_enabled = false;
        break;
      }
    }
  }

  if (m_memory.m_tlm_response_status == TLM_OK_RESPONSE) {
    if (all_enabled) {
      u32 page       = m_memory.m_p_memory->get_page(addr);
      u32 mem_offset = m_memory.m_p_memory->get_page_offset(addr);
      if (read) {
        memcpy(ptr, m_memory.m_p_memory->m_page_table[page]+mem_offset, len);
      }
      else {
        memcpy(m_memory.m_p_memory->m_page_table[page]+mem_offset, ptr, len);
      }
    }
    else {
      for (u32 i=0; i < len; ++i, ++addr) {
        if (bep[i % bel] == enabled) {
          if (read) {
            ptr[i] = m_memory.m_p_memory->read_u8(addr);
          }
          else {
            m_memory.m_p_memory->write_u8(addr, ptr[i]);
          }
        }
      }
    }
    bool allowed = m_memory.m_allow_dmi;
    if (allowed) {
      u32 beg_adr = adr;
      u32 end_adr = adr + len - 1;
      for (u32 i=0; i<m_memory.m_deny_fast_access.size(); i+=2) {
        xtsc_address beg_rng = m_memory.m_deny_fast_access[i];
        xtsc_address end_rng = m_memory.m_deny_fast_access[i+1];
        if (((beg_rng >= beg_adr) && (beg_rng <= end_adr)) ||
            ((end_rng >= beg_adr) && (end_rng <= end_adr)) ||
            ((beg_rng <  beg_adr) && (end_rng >  end_adr)))
        {
          allowed = false;
          break;
        }
      }
      if (allowed) {
        trans.set_dmi_allowed(true);
      }
    }
  }

  trans.set_response_status(m_memory.m_tlm_response_status);

  sc_time delay = (read ? m_memory.m_read_delay : m_memory.m_write_delay);
  u32 beats  = (len + m_memory.m_width8 - 1) / m_memory.m_width8;
  if (beats > 1) {
    if (read) {
      delay = m_memory.m_burst_read_delay + m_memory.m_burst_read_repeat * (beats - 1);
    }
    else {
      delay = m_memory.m_burst_write_delay + m_memory.m_burst_write_repeat * (beats - 1);
    }
  }
  if (m_memory.m_immediate_timing) {
    ; // Do nothing
  }
  else if (m_memory.m_annotate_delay) {
    t += delay;
  }
  else if (delay != SC_ZERO_TIME) {
    m_busy = true;
    m_memory.wait(delay);
    m_busy = false;
  }

  if (read) {
    XTSC_INFO(m_memory.m_text, trans << " (" << t << ") b_transport[" << m_port_num << "]");
  }
  else {
    XTSC_VERBOSE(m_memory.m_text, trans << " (" << t << ") b_transport[" << m_port_num << "]");
  }
}



bool xtsc_component::xtsc_memory_tlm2::tlm_fw_transport_if_impl::get_direct_mem_ptr(tlm_generic_payload& trans, tlm_dmi& dmi_data) {
  xtsc_address address8      = (xtsc_address) trans.get_address();
  xtsc_address page_start    = address8 & ~(u64)(m_memory.m_p_memory->m_page_size8 - 1);
  xtsc_address start_address = page_start;
  xtsc_address end_address   = start_address + m_memory.m_p_memory->m_page_size8 - 1;

  u32 n = xtsc_address_nibbles();

  XTSC_VERBOSE(m_memory.m_text, "get_direct_mem_ptr: address=0x" << hex << setfill('0') << setw(n) << address8 << " " <<
                                (trans.is_read() ? "READ " : "WRITE"));

  bool allow = false;
  bool out_of_range = ((start_address < m_memory.m_p_memory->m_start_address8) ||
                       (end_address   > m_memory.m_p_memory->m_end_address8  ));

  dmi_data.set_start_address(start_address);
  dmi_data.set_end_address(end_address);

  dmi_data.allow_read_write();

  if (!out_of_range && m_memory.m_allow_dmi) {
    bool denied = false;
    for (u32 i=0; i<m_memory.m_deny_fast_access.size(); i+=2) {
      xtsc_address begin = m_memory.m_deny_fast_access[i];
      xtsc_address end   = m_memory.m_deny_fast_access[i+1];
      if (((begin >= start_address) && (begin <= end_address)) ||
          ((end   >= start_address) && (end   <= end_address)) ||
          ((begin <  start_address) && (end   >  end_address)))
      {
        if (((address8 >= begin) && (address8 <= end)) || ((begin <  start_address) && (end   >  end_address))) {
          denied = true;
          dmi_data.set_start_address(begin);
          dmi_data.set_end_address(end);
          break;
        }
        if ((address8 < begin) && (begin < end_address)) {
          end_address = begin - 1;
        }
        else if ((start_address <= end) && (end < address8)) {
          start_address = end + 1;
        }
        dmi_data.set_start_address(start_address);
        dmi_data.set_end_address(end_address);
        XTSC_VERBOSE(m_memory.m_text, "get_direct_mem_ptr: address=0x" << hex << setfill('0') << setw(n) << address8 <<
                                      " reduced range=[0x" << setw(n) << start_address << "-0x" << setw(n) << end_address << "]" <<
                                      " after removing deny=[0x" << setw(n) << begin << "-0x" << setw(n) << end << "]");
      }
    }
    if (!out_of_range && !denied) {
      u32 page = m_memory.get_page(page_start);
      dmi_data.set_dmi_ptr(m_memory.m_p_memory->m_page_table[page] + (start_address - page_start));
      allow = true;
    }
  }

  if (!out_of_range && m_memory.m_read_only) {
    if (trans.is_write()) {
      dmi_data.allow_write();
      allow = false;
    }
    else {
      dmi_data.allow_read();
    }
  }

  XTSC_INFO(m_memory.m_text, "get_direct_mem_ptr: address=0x" << hex << setfill('0') << setw(n) << address8 <<
                             " is_read()=" << boolalpha << trans.is_read() <<
                             " out_of_range=" << out_of_range <<
                             " allow=" << allow <<
                             " get_granted_access()=" << dmi_data.get_granted_access() <<
                             " range=0x" << setw(n) << dmi_data.get_start_address() <<
                             "-0x" << setw(n) << dmi_data.get_end_address() <<
                             " ptr=" << (void*)dmi_data.get_dmi_ptr());

  return allow;
}



u32 xtsc_component::xtsc_memory_tlm2::tlm_fw_transport_if_impl::transport_dbg(tlm_generic_payload& trans) {

  XTSC_VERBOSE(m_memory.m_text, "Port #" << m_port_num << " transport_dbg: " << trans);

  tlm_command   cmd     = trans.get_command();
  u64           adr     = trans.get_address();
  u8           *ptr     = trans.get_data_ptr();
  u32           len     = trans.get_data_length();

  if (cmd == TLM_IGNORE_COMMAND) {
    XTSC_INFO(m_memory.m_text, "Port #" << m_port_num << " transport_dbg: " << trans << ": returning 0");
    return 0;
  }

  if ((cmd != TLM_READ_COMMAND) && (cmd != TLM_WRITE_COMMAND)) {
    ostringstream oss;
    oss << "Port #" << m_port_num << " Unsupported command received in transport_dbg() of xtsc_memory_tlm2 '" << m_memory.name()
        << "': " << trans;
    throw xtsc_exception(oss.str());
  }

  xtsc_address start_address = (xtsc_address) adr;
  xtsc_address end_address   = start_address + len - 1;

  if ((start_address < m_memory.m_p_memory->m_start_address8) || (end_address > m_memory.m_p_memory->m_end_address8)) {
    XTSC_INFO(m_memory.m_text, "Port #" << m_port_num << " address=0x" << hex << adr << "/length=0x" << len <<
                               " is out-of-range in transport_dbg() of xtsc_memory_tlm2 '" << m_memory.name() << "': " << trans);
    return 0;
  }

  // This is NOT required
  trans.set_response_status(TLM_OK_RESPONSE);

  if (len == 0) {
    XTSC_INFO(m_memory.m_text, "Port #" << m_port_num << " transport_dbg: " << trans << ": returning 0");
    return 0;
  }

  if (cmd == TLM_READ_COMMAND) {
    m_memory.m_p_memory->peek((xtsc_address) adr, len, ptr);
  }
  else {
    m_memory.m_p_memory->poke((xtsc_address) adr, len, ptr);
  }

  return len;
}



void xtsc_component::xtsc_memory_tlm2::tlm_fw_transport_if_impl::register_port(sc_port_base& port, const char *if_typename) {
  if (m_p_port) {
    ostringstream oss;
    oss << "Illegal multiple binding detected to xtsc_memory_tlm2 '" << m_memory.name() << "' " << name() << ": "
        << endl;
    oss << "  " << port.name() << endl;
    oss << "  " << m_p_port->name();
    throw xtsc_exception(oss.str());
  }
  XTSC_INFO(m_memory.m_text, "Binding '" << port.name() << "' to xtsc_memory_tlm2::" << name() << "");
  m_p_port = &port;
}



