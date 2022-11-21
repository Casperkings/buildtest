#ifndef _XTSC_MEMORY_VP_H_
#define _XTSC_MEMORY_VP_H_

// Copyright (c) 2006-2018 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Cadence Design Systems, Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Cadence Design Systems, Inc.

/**
 * @file 
 */

#include <scml.h>
#include <xtsc/xtsc_memory.h>
#include <xtsc_vp/xtsc_vp.h>



namespace xtsc_vp {


/// Wrapper for xtsc_memory in Synopsys Virtual Prototyping Environment
template <unsigned int NUM_PORTS = 1, unsigned int DATA_WIDTH = 32> 
class xtsc_memory_vp : public sc_core::sc_module {
public:

  sc_export<xtsc::xtsc_request_if>     *m_request_exports[NUM_PORTS];
  sc_port  <xtsc::xtsc_respond_if>     *m_respond_ports  [NUM_PORTS];

      
  SC_HAS_PROCESS(xtsc_memory_vp);

 
  xtsc_memory_vp(const sc_core::sc_module_name &module_name) :
    sc_module                     (module_name),
    m_request_fifo_depth          ("/FIFO/request_fifo_depth",          2),
    m_check_alignment             ("/Misc/check_alignment",             true),
    m_initial_value_file          ("/Misc/initial_value_file",          ""),
    m_host_mutex                  ("/Misc/host_mutex",                  false),
    m_host_shared_memory          ("/Misc/host_shared_memory",          false),
    m_shared_memory_name          ("/Misc/shared_memory_name",          ""),
    m_memory_byte_size            ("/Misc/memory_byte_size",            0),
    m_memory_fill_byte            ("/Misc/memory_fill_byte",            0x00),
    m_page_byte_size              ("/Misc/page_byte_size",              0x4000),
    m_read_only                   ("/Misc/read_only",                   false),
    m_start_byte_address          ("/Misc/start_byte_address",          0x00000000),
    m_support_exclusive           ("/Misc/support_exclusive",           true),
    m_write_responses             ("/Misc/write_responses",             true),
    m_fail_percentage             ("/Testbench/fail_percentage",        100),
    m_fail_request_mask           ("/Testbench/fail_request_mask",      0x00000000),
    m_fail_seed                   ("/Testbench/fail_seed",              1),
    m_fail_status                 ("/Testbench/fail_status",            4),
    m_script_file                 ("/Testbench/script_file",            ""),
    m_wraparound                  ("/Testbench/wraparound",             false),
    m_block_read_delay            ("/Timing/block_read_delay",          1),
    m_block_read_repeat           ("/Timing/block_read_repeat",         1),
    m_block_write_delay           ("/Timing/block_write_delay",         1),
    m_block_write_repeat          ("/Timing/block_write_repeat",        1),
    m_block_write_response        ("/Timing/block_write_response",      1),
    m_burst_read_delay            ("/Timing/burst_read_delay",          1),
    m_burst_read_repeat           ("/Timing/burst_read_repeat",         1),
    m_burst_write_delay           ("/Timing/burst_write_delay",         1),
    m_burst_write_repeat          ("/Timing/burst_write_repeat",        1),
    m_burst_write_response        ("/Timing/burst_write_response",      1),
    m_clock_period                ("/Timing/clock_period",              0xFFFFFFFF),
    m_delay_from_receipt          ("/Timing/delay_from_receipt",        true),
    m_immediate_timing            ("/Timing/immediate_timing",          false),
    m_rcw_repeat                  ("/Timing/rcw_repeat",                1),
    m_rcw_response                ("/Timing/rcw_response",              1),
    m_read_delay                  ("/Timing/read_delay",                1),
    m_recovery_time               ("/Timing/recovery_time",             1),
    m_response_repeat             ("/Timing/response_repeat",           1),
    m_write_delay                 ("/Timing/write_delay",               1),
    m_deny_fast_access            ("/Turbo/deny_fast_access",           ""),
    m_use_fast_access             ("/Turbo/use_fast_access",            true),
    m_use_interface_access        ("/Turbo/use_interface_access",       false),
    m_use_raw_access              ("/Turbo/use_raw_access",             true),
    m_p_memory                    (0)
  {
    xtsc_vp_initialize();

    m_parms.set("block_read_delay",        m_block_read_delay);
    m_parms.set("block_read_repeat",       m_block_read_repeat);
    m_parms.set("block_write_delay",       m_block_write_delay);
    m_parms.set("block_write_repeat",      m_block_write_repeat);
    m_parms.set("block_write_response",    m_block_write_response);
    m_parms.set("burst_read_delay",        m_burst_read_delay);
    m_parms.set("burst_read_repeat",       m_burst_read_repeat);
    m_parms.set("burst_write_delay",       m_burst_write_delay);
    m_parms.set("burst_write_repeat",      m_burst_write_repeat);
    m_parms.set("burst_write_response",    m_burst_write_response);
    m_parms.set("check_alignment",         m_check_alignment);
    m_parms.set("clock_period",            m_clock_period);
    m_parms.set("delay_from_receipt",      m_delay_from_receipt);
    std::vector<xtsc::u32> deny_fast_access;
    bool deny_fast_access_bad_value = false;
    try { xtsc::xtsc_strtou32vector(m_deny_fast_access, deny_fast_access); } catch (...) { deny_fast_access_bad_value = true; }
    if (deny_fast_access_bad_value) {
      std::ostringstream oss;
      oss << "ERROR: Cannot convert value #" << (deny_fast_access.size()+1) << " of parameter \"deny_fast_access\" to u32.";
      throw xtsc::xtsc_exception(oss.str());
    }
    m_parms.set("deny_fast_access",        deny_fast_access); 
    m_parms.set("fail_percentage",         m_fail_percentage);
    m_parms.set("fail_request_mask",       m_fail_request_mask);
    m_parms.set("fail_seed",               m_fail_seed);
    m_parms.set("fail_status",             m_fail_status);
    m_parms.set("immediate_timing",        m_immediate_timing);
    m_parms.set("initial_value_file",      m_initial_value_file.getValue().c_str());
    m_parms.set("host_mutex",              m_host_mutex);
    m_parms.set("host_shared_memory",      m_host_shared_memory);
    m_parms.set("shared_memory_name",      m_shared_memory_name.getValue().c_str());
    m_parms.set("memory_byte_size",        m_memory_byte_size);
    m_parms.set("memory_fill_byte",        m_memory_fill_byte);
    m_parms.set("page_byte_size",          m_page_byte_size);
    m_parms.set("rcw_repeat",              m_rcw_repeat);
    m_parms.set("rcw_response",            m_rcw_response);
    m_parms.set("read_delay",              m_read_delay);
    m_parms.set("read_only",               m_read_only);
    m_parms.set("recovery_time",           m_recovery_time);
    m_parms.set("request_fifo_depth",      m_request_fifo_depth);
    m_parms.set("response_repeat",         m_response_repeat);
    m_parms.set("script_file",             m_script_file.getValue().c_str());
    m_parms.set("start_byte_address",      m_start_byte_address);
    m_parms.set("support_exclusive",       m_support_exclusive);
    m_parms.set("write_responses",         m_write_responses);
    m_parms.set("use_fast_access",         m_use_fast_access);
    m_parms.set("use_interface_access",    m_use_interface_access);
    m_parms.set("use_raw_access",          m_use_raw_access);
    m_parms.set("wraparound",              m_wraparound);
    m_parms.set("write_delay",             m_write_delay);
    m_parms.set("byte_width",              DATA_WIDTH/8);

    m_parms.extract_parms(sc_argc(), sc_argv(), name());
    m_parms.set("num_ports",               NUM_PORTS);
    m_parms.set("byte_width",              DATA_WIDTH/8);
    m_p_memory = new xtsc_component::xtsc_memory("_", m_parms);

    for (xtsc::u32 i=0; i<NUM_PORTS; ++i) {
      std::ostringstream oss1, oss2;
      oss1 << "m_request_exports[" << i << "]";
      oss2 << "m_respond_ports["   << i << "]";
      m_request_exports[i] = new sc_export<xtsc::xtsc_request_if>(oss1.str().c_str());
      m_respond_ports  [i] = new sc_port  <xtsc::xtsc_respond_if>(oss2.str().c_str());
      (*m_p_memory->m_respond_ports[i])(*m_respond_ports[i]);
      (*m_request_exports[i])(*m_p_memory->m_request_exports[i]);
    }

    SCML_COMMAND_PROCESSOR(handle_scml_commands);
    SCML_ADD_COMMAND("dump_parms",                        0, 0, "dump_parms", 
                     "Calls xtsc_parms::dump() to list the xtsc_memory construction parameters.");
    SCML_ADD_COMMAND("byte_dump",                         2, 2, "byte_dump <StartAddress> <NumBytes>", 
                     "Calls xtsc_memory::byte_dump() to dump memory contents.");

  }



  virtual ~xtsc_memory_vp() {
    xtsc_vp_finalize();
  }



  string handle_scml_commands(const std::vector<string>& cmd) {
    std::ostringstream oss;
    if (cmd[0] == "dump_parms") {
      m_parms.dump(oss);
    }
    else if (cmd[0] == "byte_dump") {
      try {
        xtsc::xtsc_address address8 = xtsc::xtsc_strtou32(cmd[1]);
        xtsc::u32          size8    = xtsc::xtsc_strtou32(cmd[2]);
        m_p_memory->byte_dump(address8, size8, oss);
      } catch (...) { oss << "<StartAddress> and/or <NumBytes> is not a valid number or is out-of-range"; }
    }
    return oss.str();
  }



  void before_end_of_elaboration() {
    for (xtsc::u32 i=0; i<NUM_PORTS; ++i) {
      XTSC_INFO(m_p_memory->get_text_logger(), "  port name #" << i << " is: " << m_respond_ports  [i]->name());
      XTSC_INFO(m_p_memory->get_text_logger(), "export name #" << i << " is: " << m_request_exports[i]->name());
    }
  }




private:

  scml_property<unsigned int>           m_request_fifo_depth;

  scml_property<bool>                   m_check_alignment;
  scml_property<string>                 m_initial_value_file;
  scml_property<bool>                   m_host_mutex;
  scml_property<bool>                   m_host_shared_memory;
  scml_property<string>                 m_shared_memory_name;
  scml_property<unsigned int>           m_memory_byte_size;
  scml_property<unsigned int>           m_memory_fill_byte;
  scml_property<unsigned int>           m_page_byte_size;
  scml_property<bool>                   m_read_only;
  scml_property<unsigned int>           m_start_byte_address;
  scml_property<bool>                   m_support_exclusive;
  scml_property<bool>                   m_write_responses;

  scml_property<unsigned int>           m_fail_percentage;
  scml_property<unsigned int>           m_fail_request_mask;
  scml_property<unsigned int>           m_fail_seed;
  scml_property<unsigned int>           m_fail_status;
  scml_property<string>                 m_script_file;
  scml_property<bool>                   m_wraparound;

  scml_property<unsigned int>           m_block_read_delay;
  scml_property<unsigned int>           m_block_read_repeat;
  scml_property<unsigned int>           m_block_write_delay;
  scml_property<unsigned int>           m_block_write_repeat;
  scml_property<unsigned int>           m_block_write_response;
  scml_property<unsigned int>           m_burst_read_delay;
  scml_property<unsigned int>           m_burst_read_repeat;
  scml_property<unsigned int>           m_burst_write_delay;
  scml_property<unsigned int>           m_burst_write_repeat;
  scml_property<unsigned int>           m_burst_write_response;
  scml_property<unsigned int>           m_clock_period;
  scml_property<bool>                   m_delay_from_receipt;
  scml_property<bool>                   m_immediate_timing;
  scml_property<unsigned int>           m_rcw_repeat;
  scml_property<unsigned int>           m_rcw_response;
  scml_property<unsigned int>           m_read_delay;
  scml_property<unsigned int>           m_recovery_time;
  scml_property<unsigned int>           m_response_repeat;
  scml_property<unsigned int>           m_write_delay;

  scml_property<string>                 m_deny_fast_access;
  scml_property<bool>                   m_use_fast_access;
  scml_property<bool>                   m_use_interface_access;
  scml_property<bool>                   m_use_raw_access;

  xtsc_component::xtsc_memory_parms     m_parms;
  xtsc_component::xtsc_memory          *m_p_memory;

};



class XTSC_VP_API xtsc_memory_vp_DIM0026_workaround_do_not_instantiate_me {
  sc_export<xtsc::xtsc_request_if>      m_request_exports;
  sc_port  <xtsc::xtsc_respond_if>      m_respond_ports;
};



}  // namespace xtsc_vp


#endif  // _XTSC_MEMORY_VP_H_
