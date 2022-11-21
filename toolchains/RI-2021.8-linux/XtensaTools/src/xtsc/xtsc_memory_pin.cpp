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
#include <xtsc/xtsc_fast_access.h>
#include <xtsc/xtsc_memory_pin.h>
#include <xtsc/xtsc_core.h>
#include <xtsc/xtsc_tlm2pin_memory_transactor.h>

// TODO: AXI: What makes sure m_axi_wr_rsp_fifo is not "full"?  Shouldn't its event be notified?
// TODO: AXI: Add "axi_wr_rsp_fifo_depth" vector<u32>  (or just u32) and busy the Write Addr channels based on it
// TODO: AXI: Add "axi_rd_rsp_fifo_depth" vector<u32>  (or just u32) and busy the Read  Addr channels based on it

/**
 * Theory of operation:
 *
 * This model supports any of the Xtensa memory interfaces (except the caches themselves) and
 * it supports operating as a multi-ported memory.  It also supports generating a random busy/
 * not-ready signal for testing purposes.
 *
 * The "sample_phase" parameter defines the clock phase at which signals are sampled and
 * the "drive_phase" parameter defines the clock phase when outputs are driven.  Typically
 * you would sample at the clock boundary and drive a fraction of a clock cycle later.
 *
 * When this model is operating with a PIF|IDMA0 interface, it uses the following threads:
 *    pif_request_thread:         To sample signals except PORespRdy
 *    pif_drive_req_rdy_thread:   To drive PIReqRdy
 *    pif_respond_thread:         To drive response outputs and sample PORespRdy
 *
 * When this model is operating with an AXI|IDMA interface, it uses the following threads:
 *    axi_req_addr_thread:        To sample request addr channel signals (AxID, AxADDR, AxLEN, AxSIZE, etc)
 *    axi_req_data_thread:        To sample request data channel signals (WDATA, WSTRB, and WLAST)
 *    axi_drive_addr_rdy_thread:  To drive AxREADY for request addr channel (ARREADY|AWREADY).
 *    axi_drive_data_rdy_thread:  To drive  WREADY for request data channel
 *    axi_rd_rsp_thread:          To drive read  response outputs and sample RREADY for the read  response channel
 *    axi_wr_data_thread:         To process each beat of the request data channel (calls do_burst_write() for each beat)
 *    axi_wr_rsp_thread:          To drive write response outputs and sample BREADY for the write response channel
 *
 * When this model is operating with an APB interface, it uses the following threads:
 *    apb_request_thread:         To sample handshake signals
 *    apb_drive_outputs_thread:   To drive outputs
 *
 *
 * When this model is operating with a local memory interface, it uses the following threads:
 *    lcl_request_thread:         To sample signals                     Non-split Rd/Wr only
 *    lcl_rd_request_thread:      To sample signals                     Rd of split Rd/Wr only
 *    lcl_wr_request_thread:      To sample signals                     Wr of split Rd/Wr only
 *    lcl_drive_read_data_thread: To drive the read data output         Not used for Wr of split Rd/Wr
 *    lcl_drive_busy_thread:      To drive the busy output              If "has_busy" is  true
 *
 * Note: If "has_lock" is true this model will have the lock signals present but will ignore them.  For
 *       subbanked Data RAM (DRAM0BS|DRAM1BS), the lock signal will only be present on the 0th subbank
 *       of each bank: ((port % m_num_subbanks) == 0)
 *
 * Note: For subbanked Data RAM (DRAM0BS|DRAM1BS) with "has_busy" true, the busy signal will only be
 *       present on the 0th subbank of each bank: ((port % m_num_subbanks) == 0)
 *
 */



using namespace std;
#if SYSTEMC_VERSION >= 20050601
using namespace sc_core;
#endif
using namespace sc_dt;
using namespace log4xtensa;
using namespace xtsc;



// Shorthand aliases
typedef sc_core::sc_signal<bool>                bool_signal;
typedef sc_core::sc_signal<sc_dt::sc_uint_base> uint_signal;
typedef sc_core::sc_signal<sc_dt::sc_bv_base>   wide_signal;



// ctor helper
static u32 get_num_ports(const xtsc_component::xtsc_memory_pin_parms& memory_parms) {
  return memory_parms.get_non_zero_u32("num_ports");
}



// ctor helper
static sc_trace_file *get_trace_file(const xtsc_component::xtsc_memory_pin_parms& memory_parms) {
  return static_cast<sc_trace_file*>(const_cast<void*>(memory_parms.get_void_pointer("vcd_handle")));
}



// ctor helper
static const char *get_suffix(const xtsc_component::xtsc_memory_pin_parms& memory_parms) {
  const char *port_name_suffix = memory_parms.get_c_str("port_name_suffix");
  static const char *blank = "";
  return port_name_suffix ? port_name_suffix : blank;
}



// ctor helper
static bool get_banked(const xtsc_component::xtsc_memory_pin_parms& memory_parms) {
  return memory_parms.get_bool("banked");
}



// ctor helper
static bool get_split_rw(const xtsc_component::xtsc_memory_pin_parms& memory_parms) {
  using namespace xtsc_component;
  string interface_uc = xtsc_module_pin_base::get_interface_uc(memory_parms.get_c_str("memory_interface"));
  xtsc_module_pin_base::memory_interface_type interface_type = xtsc_module_pin_base::get_interface_type(interface_uc);
  return ((interface_type == xtsc_module_pin_base::DRAM0RW) || (interface_type == xtsc_module_pin_base::DRAM1RW));
}



// ctor helper
static bool get_dma(const xtsc_component::xtsc_memory_pin_parms& memory_parms) {
  return memory_parms.get_bool("has_dma");
}



// ctor helper
static u32 get_subbanks(const xtsc_component::xtsc_memory_pin_parms& memory_parms) {
  return memory_parms.get_u32("num_subbanks");
}



xtsc_component::xtsc_memory_pin_parms::xtsc_memory_pin_parms(const xtsc_core&   core,
                                                             const char        *memory_interface,
                                                             u32                delay,
                                                             u32                num_ports)
{
  xtsc_core::memory_port mem_port = xtsc_core::get_memory_port(memory_interface, &core);
  if (!core.has_memory_port(mem_port)) {
    ostringstream oss;
    oss << core.kind() << " '" << core.name() << "' doesn't have a \"" << memory_interface << "\" memory port.";
    throw xtsc_exception(oss.str());
  }
  xtsc_module_pin_base::memory_interface_type interface_type = xtsc_module_pin_base::get_interface_type(mem_port);

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

  xtsc_address  start_address8  = 0;
  u32           width8          = core.get_memory_byte_width(mem_port);
  u32           address_bits    = 32;   // PIF|IDMA0|AXI|XLMI0
  u32           size8           = 0;    // 4GB for PIF|IDMA0|AXI
  u32           banks           = 1;
  u32           subbanks        = 0;

  if (xtsc_module_pin_base::is_system_memory(interface_type)) {
    if (delay == 0xFFFFFFFF) {
      delay = core_parms.get_u32("LocalMemoryLatency") - 1;
    }
  }
  else {
    delay = core_parms.get_u32("LocalMemoryLatency") - 1;
    if ((interface_type == xtsc_module_pin_base::XLMI0) || core_parms.get_bool("SimFullLocalMemAddress")) {
      core.get_local_memory_starting_byte_address(mem_port, start_address8);
    }
    core.get_local_memory_byte_size(mem_port, size8);
    if (interface_type != xtsc_module_pin_base::XLMI0) {
      // Compute address_bits  (size8 must be a power of 2)
      address_bits = 0;
      u32 shift_value = size8 / width8;
      for (u32 i=0; i<32; ++i) {
        if (shift_value & 0x1) address_bits = i;
        shift_value >>= 1;
      }
      if ((interface_type == xtsc_module_pin_base::DRAM0  ) || (interface_type == xtsc_module_pin_base::DRAM1  ) ||
          (interface_type == xtsc_module_pin_base::DRAM0BS) || (interface_type == xtsc_module_pin_base::DRAM1BS))
      {
        banks = core_parms.get_u32("DataRAMBanks");
      }
      else if (interface_type == xtsc_module_pin_base::DROM0) {
        banks = core_parms.get_u32("DataROMBanks");
      }
      if (banks > 1) {
        if (banks == 2) {
          address_bits -= 1;
        }
        else if (banks == 4) {
          address_bits -= 2;
        }
        else {
          ostringstream oss;
          oss << kind() << " ctor: banks (" << banks << ") is neither 2 nor 4.";
          throw xtsc_exception(oss.str());
        }
        if ((interface_type == xtsc_module_pin_base::DRAM0BS) || (interface_type == xtsc_module_pin_base::DRAM1BS)) {
          subbanks = core_parms.get_u32("DataRAMSubBanks");
          if (banks*subbanks != num_ports) {
            ostringstream oss;
            oss << kind() << " ctor: banks*subbanks (" << banks << "*" << subbanks << "=" << (banks*subbanks) << ") does not equal num_ports ("
                << num_ports << ")";
            throw xtsc_exception(oss.str());
          }
          address_bits -= xtsc_ceiling_log2(subbanks);
        }
        else if (banks != num_ports) {
          ostringstream oss;
          oss << kind() << " ctor: banks (" << banks << ") does not equal num_ports (" << num_ports << ")";
          throw xtsc_exception(oss.str());
        }
      }
    }
  }

  init(xtsc_module_pin_base::get_interface_name(interface_type), width8, address_bits, delay, num_ports);

  set("clock_period", core_parms.get_u32("SimClockFactor")*xtsc_get_system_clock_factor());
  set("start_byte_address", (u32)start_address8);
  set("memory_byte_size", size8);
  set("big_endian", core.is_big_endian());
  set("has_busy", core.has_busy(mem_port));
  set("has_dma", false);

  if (((interface_type == xtsc_module_pin_base::DRAM0RW) && core_parms.get_bool("DataRAM0HasInbound")) ||
      ((interface_type == xtsc_module_pin_base::DRAM1RW) && core_parms.get_bool("DataRAM1HasInbound")))
  {
    set("has_dma", true);
  }

  if ((interface_type == xtsc_module_pin_base::DRAM0RW) || (interface_type == xtsc_module_pin_base::DRAM1RW)) {
    set("dram_attribute_width", core_parms.get_u32("DataRAMAttributeWidth"));
  }

  if (((interface_type == xtsc_module_pin_base::DRAM0  ) && core_parms.get_bool("DataRAM0HasRCW")) ||
      ((interface_type == xtsc_module_pin_base::DRAM0BS) && core_parms.get_bool("DataRAM0HasRCW")) ||
      ((interface_type == xtsc_module_pin_base::DRAM0RW) && core_parms.get_bool("DataRAM0HasRCW")) ||
      ((interface_type == xtsc_module_pin_base::DRAM1  ) && core_parms.get_bool("DataRAM1HasRCW")) ||
      ((interface_type == xtsc_module_pin_base::DRAM1BS) && core_parms.get_bool("DataRAM1HasRCW")) ||
      ((interface_type == xtsc_module_pin_base::DRAM1RW) && core_parms.get_bool("DataRAM1HasRCW")))
  {
    set("has_lock", true);
  }

  if ((interface_type == xtsc_module_pin_base::DRAM0)   ||
      (interface_type == xtsc_module_pin_base::DRAM0BS) ||
      (interface_type == xtsc_module_pin_base::DRAM1)   ||
      (interface_type == xtsc_module_pin_base::DRAM1BS) ||
      (interface_type == xtsc_module_pin_base::DROM0))
  {
    if (num_ports == 1) {
      set("cbox", core_parms.get_bool("HasCBox"));
    }
    if (banks > 1) {
      set("banked", true);
      set("num_subbanks", subbanks);
    }
  }

  if (((interface_type == xtsc_module_pin_base::IRAM0) && core_parms.get_bool("InstRAM0HasParity")) ||
      ((interface_type == xtsc_module_pin_base::IRAM1) && core_parms.get_bool("InstRAM1HasParity")))
  {
    set("check_bits", width8/4);
  }
  else if (((interface_type == xtsc_module_pin_base::DRAM0  ) && core_parms.get_bool("DataRAM0HasParity")) ||
           ((interface_type == xtsc_module_pin_base::DRAM0BS) && core_parms.get_bool("DataRAM0HasParity")) ||
           ((interface_type == xtsc_module_pin_base::DRAM0RW) && core_parms.get_bool("DataRAM0HasParity")) ||
           ((interface_type == xtsc_module_pin_base::DRAM1  ) && core_parms.get_bool("DataRAM1HasParity")) ||
           ((interface_type == xtsc_module_pin_base::DRAM1BS) && core_parms.get_bool("DataRAM1HasParity")) ||
           ((interface_type == xtsc_module_pin_base::DRAM1RW) && core_parms.get_bool("DataRAM1HasParity")))
  {
    set("check_bits", width8/core_parms.get_u32("DataErrorWordWidth"));
  }
  else if (((interface_type == xtsc_module_pin_base::IRAM0) && core_parms.get_bool("InstRAM0HasECC")) ||
           ((interface_type == xtsc_module_pin_base::IRAM1) && core_parms.get_bool("InstRAM1HasECC")))
  {
    set("check_bits", width8*7/4);
  }
  else if (((interface_type == xtsc_module_pin_base::DRAM0  ) && core_parms.get_bool("DataRAM0HasECC")) ||
           ((interface_type == xtsc_module_pin_base::DRAM0BS) && core_parms.get_bool("DataRAM0HasECC")) ||
           ((interface_type == xtsc_module_pin_base::DRAM0RW) && core_parms.get_bool("DataRAM0HasECC")) ||
           ((interface_type == xtsc_module_pin_base::DRAM1  ) && core_parms.get_bool("DataRAM1HasECC")) ||
           ((interface_type == xtsc_module_pin_base::DRAM1BS) && core_parms.get_bool("DataRAM1HasECC")) ||
           ((interface_type == xtsc_module_pin_base::DRAM1RW) && core_parms.get_bool("DataRAM1HasECC")))
  {
    if (core_parms.get_u32("DataErrorWordWidth") == 1) {
      set("check_bits", width8*5);
    }
    else {
      set("check_bits", width8*7/4);
    }
  }

  if (xtsc_module_pin_base::is_pif_or_idma(interface_type)) {
    set("has_pif_attribute",  core_parms.get_bool("HasPIFReqAttribute"));
    set("has_pif_req_domain", core_parms.get_bool("HasPIFReqDomain"));
  }

  if (xtsc_module_pin_base::is_axi(interface_type)) {
    bool combine_master_axi_ports = core_parms.get_bool("CombineMasterAXIPorts");
    if (num_ports == 3) {
      vector<u32> vec;
      u32 d = core_parms.get_u32("DataMasterType");
      u32 i = core_parms.get_u32("InstMasterType");
      vec.clear(); vec.push_back(8); vec.push_back(8); vec.push_back(4); set("axi_tran_id_bits", vec);
      vec.clear(); vec.push_back(0); vec.push_back(1); vec.push_back(0); set("axi_read_write",   vec);
      vec.clear(); vec.push_back(d); vec.push_back(d); vec.push_back(i); set("axi_port_type",  vec);
      set("axi_name_prefix", "DataMaster,DataMaster,InstMaster");
    } else if (combine_master_axi_ports && (num_ports == 2)) { 
      vector<u32> vec;
      u32 d = core_parms.get_u32("DataMasterType");
      //Note read channel carries both instruction and data, hence the extra axi-id-bit.
      vec.clear(); vec.push_back(8+1); vec.push_back(8); set("axi_tran_id_bits", vec);
      vec.clear(); vec.push_back(0);   vec.push_back(1); set("axi_read_write",   vec);
      vec.clear(); vec.push_back(d);   vec.push_back(d); set("axi_port_type",    vec);
      set("axi_name_prefix", "AXIMaster,AXIMaster");
    }
  }

  if (interface_type == xtsc_module_pin_base::IDMA) {
    if (num_ports == 2) {
      vector<u32> vec;
      u32 d = 2; // ACE-Lite
      vec.clear(); vec.push_back(8); vec.push_back(8); set("axi_tran_id_bits",    vec);
      vec.clear(); vec.push_back(0); vec.push_back(1); set("axi_read_write", vec);
      vec.clear(); vec.push_back(d); vec.push_back(d); set("axi_port_type",  vec);
      set("axi_name_prefix", "IDMA,IDMA");
    }
  }

  if (xtsc_module_pin_base::is_apb(interface_type)) {
    if (num_ports == 1) {
      set("axi_name_prefix", "APBMaster");
    }
  }

}



xtsc_component::xtsc_memory_pin::xtsc_memory_pin(sc_module_name module_name, const xtsc_memory_pin_parms& memory_parms) :
  sc_module             (module_name),
  xtsc_module           (*(sc_module*)this),
  xtsc_module_pin_base  (*this, ::get_num_ports (memory_parms),
                                ::get_trace_file(memory_parms),
                                ::get_suffix    (memory_parms),
                                ::get_banked    (memory_parms),
                                ::get_split_rw  (memory_parms),
                                ::get_dma       (memory_parms),
                                ::get_subbanks  (memory_parms)),
  m_num_ports           (memory_parms.get_non_zero_u32("num_ports")),
  m_num_axi_rd_ports    (0),
  m_num_axi_wr_ports    (0),
  m_interface_uc        (get_interface_uc(memory_parms.get_c_str("memory_interface"))),
  m_interface_lc        (get_interface_lc(memory_parms.get_c_str("memory_interface"))),
  m_interface_type      (get_interface_type(m_interface_uc)),
  m_dram_attribute_width(memory_parms.get_u32("dram_attribute_width")),
  m_dram_attribute_bv   (160),
  m_read_delay_value    (memory_parms.get_u32("read_delay")),
  m_inbound_pif         (false),
  m_has_coherence       (false),
  m_has_pif_attribute   (false),
  m_has_pif_req_domain  (false),
  m_use_fast_access     (memory_parms.get_bool("use_fast_access")),
  m_req_user_data       (""),
  m_req_user_data_name  (""),
  m_req_user_data_width1(0),
  m_rsp_user_data       (""),
  m_rsp_user_data_name  (""),
  m_rsp_user_data_width1(0),
  m_address_bits        (is_system_memory(m_interface_type) ? 32 : memory_parms.get_non_zero_u32("address_bits")),
  m_check_bits          (memory_parms.get_u32("check_bits")),
  m_setw                ((m_address_bits+3)/4),
  m_route_id_bits       (memory_parms.get_u32("route_id_bits")),
  m_address             (m_address_bits),
  m_id_zero             (m_id_bits),
  m_priority_zero       (2),
  m_route_id_zero       (m_route_id_bits ? m_route_id_bits : 1),
  m_data_zero           ((int)memory_parms.get_non_zero_u32("byte_width")),
  m_resp_cntl_zero      (memory_parms.get_u32("void_resp_cntl")),
  m_coh_cntl            (2),
  m_req_cntl            (0),
  m_resp_cntl           (0),
  m_xID                 (NULL),
  m_xRESP               (2),
  m_text                (log4xtensa::TextLogger::getInstance(name())),
  m_binary              (log4xtensa::BinaryLogger::getInstance(name()))
{

  const char *axi_name_prefix = memory_parms.get_c_str("axi_name_prefix");

  u32           byte_width              = memory_parms.get_u32("byte_width");
  u32           start_byte_address      = memory_parms.get_u32("start_byte_address");
  u32           memory_byte_size        = memory_parms.get_u32("memory_byte_size");
  u32           page_byte_size          = memory_parms.get_u32("page_byte_size");
  const char   *initial_value_file      = memory_parms.get_c_str("initial_value_file");
  u8            memory_fill_byte        = (u8) memory_parms.get_u32("memory_fill_byte");

  m_p_memory = new xtsc_memory_b(name(), kind(), byte_width, start_byte_address, memory_byte_size, page_byte_size,
                                 initial_value_file, memory_fill_byte);

  m_start_address8      = m_p_memory->m_start_address8;
  m_size8               = m_p_memory->m_size8;
  m_width8              = m_p_memory->m_width8;
  m_end_address8        = m_p_memory->m_end_address8;
  m_enable_bits         = m_width8;
  if ((m_interface_type == IRAM0) || (m_interface_type == IRAM1) || (m_interface_type == IROM0)) {
    m_enable_bits       = m_width8 / 4;
  }

  m_id_zero                     = 0;
  m_priority_zero               = 0;
  m_route_id_zero               = 0;
  m_data_zero                   = 0;
  m_coh_cntl                    = 0;
  m_request_fifo_depth          = 0;
  m_rsp_user_data_val_echo      = false;
  m_pif_req_event               = 0;
  m_pif_req_rdy_event           = 0;
  m_respond_event               = 0;
  m_axi_rd_rsp_event            = 0;
  m_axi_wr_data_event           = 0;
  m_axi_wr_rsp_event            = 0;
  m_axi_addr_rdy_event          = 0;
  m_axi_data_rdy_event          = 0;
  m_apb_req_event               = 0;
  m_read_event_queue            = 0;
  m_busy_event_queue            = 0;

  if (m_interface_type == PIF) {
    m_inbound_pif       = memory_parms.get_bool("inbound_pif");
    if (!m_inbound_pif) {
      m_has_coherence   = memory_parms.get_bool("has_coherence");
      m_has_pif_req_domain = memory_parms.get_bool("has_pif_req_domain");
    }
    m_has_pif_attribute = memory_parms.get_bool("has_pif_attribute");
  }
  else if (m_interface_type == IDMA0) {
    m_has_pif_attribute = memory_parms.get_bool("has_pif_attribute");
    m_has_pif_req_domain= memory_parms.get_bool("has_pif_req_domain");
  }
  else if (is_axi_or_idma(m_interface_type)) {
    m_axi_tran_id_bits    = memory_parms.get_u32_vector("axi_tran_id_bits");
    m_axi_read_write      = memory_parms.get_u32_vector("axi_read_write");
    m_axi_port_type       = memory_parms.get_u32_vector("axi_port_type");
    check_and_adjust_vu32(kind(), name(), "axi_tran_id_bits",  m_axi_tran_id_bits,  "num_ports", m_num_ports);
    check_and_adjust_vu32(kind(), name(), "axi_read_write",    m_axi_read_write,    "num_ports", m_num_ports);
    check_and_adjust_vu32(kind(), name(), "axi_port_type",     m_axi_port_type,     "num_ports", m_num_ports);
  }

  if (is_amba(m_interface_type)) {
    if (!axi_name_prefix) { axi_name_prefix = ""; }
    xtsc_strtostrvector(axi_name_prefix, m_axi_name_prefix);
    if (m_axi_name_prefix.size() != m_num_ports) {
      if (m_axi_name_prefix.size() == 1) {
        string val = m_axi_name_prefix[0];
        for (u32 i=1; i < m_num_ports; ++i) {
          m_axi_name_prefix.push_back(val);
        }
      }
      else {
        ostringstream oss;
        oss << kind() << " '" << name() << "': \"axi_name_prefix\"" << " contains " << m_axi_name_prefix.size() << " entries but should contain "
            << m_num_ports << " (from \"num_ports\").";
        throw xtsc_exception(oss.str());
      }
    }
  }

  m_big_endian          = memory_parms.get_bool("big_endian");
  m_has_request_id      = memory_parms.get_bool("has_request_id");
  m_write_responses     = memory_parms.get_bool("write_responses");
  m_has_busy            = memory_parms.get_bool("has_busy");
  m_has_lock            = memory_parms.get_bool("has_lock");
  m_has_xfer_en         = memory_parms.get_bool("has_xfer_en");
  m_busy_percentage     = (i32) memory_parms.get_u32 ("busy_percentage");
  m_data_busy_percentage= (i32) memory_parms.get_u32 ("data_busy_percentage");
  m_cbox                = memory_parms.get_bool("cbox");
  m_split_rw            = ::get_split_rw(memory_parms);
  m_has_dma             = memory_parms.get_bool("has_dma");

  if (m_banked && (m_num_subbanks < 2) && (m_num_ports != 2) && (m_num_ports != 4)) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': banked memory without subbanks has " << m_num_ports << " ports (must be 2 or 4)";
    throw xtsc_exception(oss.str());
  }

  if (m_split_rw) {
    if (m_has_xfer_en) {
      ostringstream oss;
      oss << kind() << " '" << name() << "': \"has_xfer_en\" cannot be true when \"memory_interface\" is DRAM0RW or DRAM1RW.";
      throw xtsc_exception(oss.str());
    }
  }

  m_append_id = true;
  if (m_num_ports == 1) {
    if (is_system_memory(m_interface_type) || is_inst_mem(m_interface_type)) {
      m_append_id = false;
    }
    else if (m_cbox) {
      if ((m_interface_type == DRAM0) || (m_interface_type == DRAM1) || (m_interface_type == DROM0)) {
        m_append_id = false;
      }
    }
  }

  // Warn if "memory_byte_size" is non-zero AND "address_bits" is not 32 (i.e. both explicitly set by user) AND they are inconsistent.
  u32 address_unit = ((is_system_memory(m_interface_type) || (m_interface_type == XLMI0)) ?  1 : m_width8);
  if ((m_size8 != 0) && (m_address_bits != 32)) {
    if (m_size8 != (address_unit << (m_address_bits + (m_banked ? xtsc_ceiling_log2(m_num_ports) : 0)))) {
      XTSC_WARN(m_text, "\"memory_byte_size\"=0x" << hex << m_size8 << " is inconsistent with" <<
                        " \"byte_width\"=" << dec << m_width8 <<
                        ", \"address_bits\"=" << m_address_bits <<
                        ", \"banked\"=" << boolalpha << m_banked <<
                        ", and \"num_ports\"=" << m_num_ports);
    }
  }

  if (is_pif_or_idma(m_interface_type) || is_inst_mem(m_interface_type)) {
    if ((m_width8 != 4) && (m_width8 != 8) && (m_width8 != 16) && (m_width8 != 32)) {
      ostringstream oss;
      oss << kind() << " '" << name() << "': Invalid \"byte_width\"= " << m_width8
          << " (legal values for IRAM0|IRAM1|IROM0|PIF|IDMA0 are 4, 8, and 16)";
      throw xtsc_exception(oss.str());
    }
  }
  else if (is_axi_or_idma(m_interface_type)) {
    if ((m_width8 != 4) && (m_width8 != 8) && (m_width8 != 16) && (m_width8 != 32) && (m_width8 != 64)) {
      ostringstream oss;
      oss << kind() << " '" << name() << "': Invalid \"byte_width\"=" << m_width8 << " (legal values for a AXI port are 4, 8, 16, 32, and 64)";
      throw xtsc_exception(oss.str());
    }
    for (u32 i=0; i<m_num_ports; ++i) {
      if (m_axi_read_write[i] > 1) {
        ostringstream oss;
        oss << kind() << " '" << name() << "': Invalid \"axi_read_write[" << i << "]\"=" << m_axi_read_write[i] << " (legal values are 0 and 1)";
        throw xtsc_exception(oss.str());
      }
      m_axi_signal_indx.push_back(m_axi_read_write[i] ? m_num_axi_wr_ports++ : m_num_axi_rd_ports++);
      u32 tid_bits = m_axi_tran_id_bits[i];
      if ((tid_bits < 1) || (tid_bits > 32)) {
        ostringstream oss;
        oss << kind() << " '" << name() << "': Invalid transaction ID bit width of " << tid_bits
            << " (from \"axi_tran_id_bits[" << i << "]\", legal transaction ID bit width range is 1-32)";
        throw xtsc_exception(oss.str());
      }
      if ((m_axi_port_type[i] < 1) || (m_axi_port_type[i] > 3)) {
        ostringstream oss;
        oss << kind() << " '" << name() << "': Invalid \"axi_port_type[" << i << "]\"=" << m_axi_port_type[i] << " (legal values are 1, 2, and 3)";
        throw xtsc_exception(oss.str());
      }
    }
  }
  else if (is_apb(m_interface_type)) {
    if (m_width8 != 4) {
      ostringstream oss;
      oss << kind() << " '" << name() << "': Invalid \"byte_width\"=" << m_width8 << " (the only legal value for the APB port is 4)";
      throw xtsc_exception(oss.str());
    }
  }
  else if ((m_interface_type == DRAM0BS) || (m_interface_type == DRAM1BS)) {
    if ((m_width8 != 4) && (m_width8 != 8) && (m_width8 != 16) && (m_width8 != 32)) {
      ostringstream oss;
      oss << kind() << " '" << name() << "': Invalid \"byte_width\"= " << m_width8 << " (legal values are 4|8|16|32)";
      throw xtsc_exception(oss.str());
    }
  }
  else if ((m_width8 != 4) && (m_width8 != 8) && (m_width8 != 16) && (m_width8 != 32) && (m_width8 != 64)) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': Invalid \"byte_width\"= " << m_width8 << " (legal values are 4|8|16|32|64)";
    throw xtsc_exception(oss.str());
  }

  if (m_route_id_bits > 32) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': Invalid \"route_id_bits\"= " << m_route_id_bits << " (max is 32)";
    throw xtsc_exception(oss.str());
  }

  if (is_system_memory(m_interface_type)) {
    m_address_shift     = 0;
    m_address_mask      = ((m_width8 ==  4) ? 0xFFFFFFFC :
                           (m_width8 ==  8) ? 0xFFFFFFF8 :
                           (m_width8 == 16) ? 0xFFFFFFF0 :
                           (m_width8 == 32) ? 0xFFFFFFE0 :
                                              0xFFFFFFC0);
    if (!is_apb(m_interface_type)) {
      m_request_fifo_depth= memory_parms.get_non_zero_u32("request_fifo_depth");
    }
  }
  else if (m_interface_type == XLMI0) {
    m_address_shift     = 0;
    u32 phy_addr_bits   = 32;
    u32 shift_value     = m_size8;
    for (u32 i=0; i<32; ++i) {
      if (shift_value & 0x1) phy_addr_bits = i;
      shift_value >>= 1;
    }
    m_address_mask      = (phy_addr_bits >= 32) ? 0xFFFFFFFF : (1 << phy_addr_bits) - 1;
    m_address_mask     &= ((m_width8 ==  4) ? 0xFFFFFFFC :
                           (m_width8 ==  8) ? 0xFFFFFFF8 :
                           (m_width8 == 16) ? 0xFFFFFFF0 :
                           (m_width8 == 32) ? 0xFFFFFFE0 :
                                              0xFFFFFFC0);
  }
  else {
    m_address_shift     = ((m_width8 ==  4) ? 2 :
                           (m_width8 ==  8) ? 3 :
                           (m_width8 == 16) ? 4 :
                           (m_width8 == 32) ? 5 :
                                              6);
    if (m_banked) {
      m_address_shift  += xtsc_ceiling_log2(m_num_ports);
    }
    m_address_mask      = (m_address_bits >= 32) ? 0xFFFFFFFF : (1 << m_address_bits) - 1;
  }

  string rsp_user_data_val;
#if defined(__x86_64__) || defined(_M_X64)
  u32 max_bits = 64;
  u32 rsp_user_data_value = 0;
#else
  u32 max_bits = 32;
  u64 rsp_user_data_value = 0;
#endif
  if (is_pif_or_idma(m_interface_type)) {
    parse_port_name_and_bit_width(memory_parms, "req_user_data", m_req_user_data, m_req_user_data_name, m_req_user_data_width1, 1, max_bits);
    parse_port_name_and_bit_width(memory_parms, "rsp_user_data", m_rsp_user_data, m_rsp_user_data_name, m_rsp_user_data_width1, 1, max_bits);
    if (m_rsp_user_data_width1) {
      if (memory_parms.get_c_str("rsp_user_data_val")) {
        rsp_user_data_val = memory_parms.get_c_str("rsp_user_data_val");
        if (rsp_user_data_val == "echo") {
          m_rsp_user_data_val_echo = true;
        }
        else {
          try {
#if defined(__x86_64__) || defined(_M_X64)
            rsp_user_data_value = xtsc_strtou64(rsp_user_data_val);
#else
            rsp_user_data_value = xtsc_strtou32(rsp_user_data_val);
#endif
          } catch (...) {
            ostringstream oss;
            oss << kind() << " '" << name() << "' parameter error: Cannot convert " << "\"rsp_user_data_val\" = \"" << rsp_user_data_val
                << "\" to a number";
            throw xtsc_exception(oss.str());
          }
        }
      }
    }
  }

  // Get clock period 
  m_time_resolution = sc_get_time_resolution();
  u32 clock_period = memory_parms.get_u32("clock_period");
  if (clock_period == 0xFFFFFFFF) {
    m_clock_period = xtsc_get_system_clock_period();
  }
  else {
    m_clock_period = sc_get_time_resolution() * clock_period;
  }
  m_clock_period_value = m_clock_period.value();
  u32 posedge_offset = memory_parms.get_u32("posedge_offset");
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

  // Convert delays from an integer number of periods to sc_time values
  m_read_delay          = m_clock_period * memory_parms.get_u32("read_delay");
  m_block_read_delay    = m_clock_period * memory_parms.get_u32("block_read_delay");
  m_block_read_repeat   = m_clock_period * memory_parms.get_u32("block_read_repeat");
  m_burst_read_delay    = m_clock_period * memory_parms.get_u32("burst_read_delay");
  m_burst_read_repeat   = m_clock_period * memory_parms.get_u32("burst_read_repeat");
  m_rcw_repeat          = m_clock_period * memory_parms.get_u32("rcw_repeat");
  m_rcw_response        = m_clock_period * memory_parms.get_u32("rcw_response");
  m_write_delay         = m_clock_period * memory_parms.get_u32("write_delay");
  m_block_write_delay   = m_clock_period * memory_parms.get_u32("block_write_delay");
  m_block_write_repeat  = m_clock_period * memory_parms.get_u32("block_write_repeat");
  m_block_write_response= m_clock_period * memory_parms.get_u32("block_write_response");
  m_burst_write_delay   = m_clock_period * memory_parms.get_u32("burst_write_delay");
  m_burst_write_repeat  = m_clock_period * memory_parms.get_u32("burst_write_repeat");
  m_burst_write_response= m_clock_period * memory_parms.get_u32("burst_write_response");

  // Get phase at which inputs are to be sampled
  u32 sample_phase = memory_parms.get_u32("sample_phase");
  m_sample_phase = sample_phase * m_time_resolution;
  m_sample_phase_plus_one = m_sample_phase + m_clock_period;

  // Get phase at which outputs are to be driven
  u32 drive_phase = memory_parms.get_u32("drive_phase");
  m_drive_phase = drive_phase * m_time_resolution;
  m_drive_phase_plus_one = m_drive_phase + m_clock_period;
  m_drive_to_sample_time = ((m_drive_phase < m_sample_phase) ? m_sample_phase : m_sample_phase_plus_one) - m_drive_phase;
  m_sample_to_drive_time = m_clock_period - m_drive_to_sample_time;
  m_sample_to_drive_busy_delay  = ((m_drive_phase >= m_sample_phase) ? m_drive_phase : m_drive_phase_plus_one) - m_sample_phase;
  m_sample_to_drive_data_delay  = m_sample_to_drive_busy_delay + m_read_delay;


  // Create all the "per mem port" stuff

  m_last_action_time_stamp      = new sc_time                   [m_num_ports];
  m_data                        = new sc_bv_base*               [m_num_ports];
  m_data_to_be_written          = new sc_bv_base*               [m_num_ports];

  m_debug_exports               = new sc_export<xtsc_debug_if>* [m_num_ports];
  m_debug_impl                  = new xtsc_debug_if_impl*       [m_num_ports];

  if (is_pif_or_idma(m_interface_type)) {
    m_pif_req_fifo              = new deque<pif_req_info*>      [m_num_ports];
    m_first_block_write         = new bool                      [m_num_ports];
    m_first_burst_write         = new bool                      [m_num_ports];
    m_block_write_xfers         = new u32                       [m_num_ports];
    m_block_write_address       = new xtsc_address              [m_num_ports];
    m_block_write_offset        = new xtsc_address              [m_num_ports];
    m_burst_write_address       = new xtsc_address              [m_num_ports];
    m_rcw_compare_data          = new sc_bv_base*               [m_num_ports];
    m_rsp_user_data_val         = new sc_bv_base*               [m_num_ports];
    m_pif_req_event             = new sc_event                  [m_num_ports];
    m_pif_req_rdy_event         = new sc_event                  [m_num_ports];
    m_respond_event             = new sc_event                  [m_num_ports];
    m_testing_busy              = new bool                      [m_num_ports];
  }
  else if (is_axi_or_idma(m_interface_type)) {
    // TODO: Why don't events have names???
    m_axi_addr_fifo             = new deque<axi_addr_info*>     [m_num_ports];
    m_axi_wr_rsp_fifo           = new deque<axi_addr_info*>     [m_num_ports];
    m_axi_data_fifo             = new deque<axi_data_info*>     [m_num_ports];
    m_testing_busy              = new bool                      [m_num_ports];
    m_testing_busy_data         = new bool                      [m_num_ports];
    m_axi_rd_rsp_event          = new sc_event                  [m_num_ports];
    m_axi_wr_data_event         = new sc_event                  [m_num_ports];
    m_axi_wr_rsp_event          = new sc_event                  [m_num_ports];
    m_axi_addr_rdy_event        = new sc_event                  [m_num_ports];
    m_axi_data_rdy_event        = new sc_event                  [m_num_ports];
    m_respond_event             = new sc_event                  [m_num_ports];
    m_xID                       = new sc_uint_base*             [m_num_ports];
  }
  else if (is_apb(m_interface_type)) {
    m_apb_req_event             = new sc_event                  [m_num_ports];
  }
  else {
    m_read_data_fifo            = new wide_fifo*                [m_num_ports];
    m_busy_fifo                 = new bool_fifo*                [m_num_ports];
    m_read_event_queue          = new sc_event_queue*           [m_num_ports];
    m_busy_event_queue          = new sc_event_queue*           [m_num_ports];
  }

  m_p_req_valid         = NULL;
  m_p_req_cntl          = NULL;
  m_p_req_adrs          = NULL;
  m_p_req_data          = NULL;
  m_p_req_data_be       = NULL;
  m_p_req_id            = NULL;
  m_p_req_priority      = NULL;
  m_p_req_route_id      = NULL;
  m_p_req_attribute     = NULL;
  m_p_req_domain        = NULL;
  m_p_req_coh_vadrs     = NULL;
  m_p_req_coh_cntl      = NULL;
  m_p_req_user_data     = NULL;
  m_p_req_rdy           = NULL;
  m_p_resp_valid        = NULL;
  m_p_resp_cntl         = NULL;
  m_p_resp_data         = NULL;
  m_p_resp_id           = NULL;
  m_p_resp_priority     = NULL;
  m_p_resp_route_id     = NULL;
  m_p_resp_coh_cntl     = NULL;
  m_p_resp_user_data    = NULL;
  m_p_resp_rdy          = NULL;
  m_p_arid              = NULL;
  m_p_araddr            = NULL;
  m_p_arlen             = NULL;
  m_p_arsize            = NULL;
  m_p_arburst           = NULL;
  m_p_arlock            = NULL;
  m_p_arcache           = NULL;
  m_p_arprot            = NULL;
  m_p_arqos             = NULL;
  m_p_arbar             = NULL;
  m_p_ardomain          = NULL;
  m_p_arsnoop           = NULL;
  m_p_arvalid           = NULL;
  m_p_arready           = NULL;
  m_p_rid               = NULL;
  m_p_rdata             = NULL;
  m_p_rresp             = NULL;
  m_p_rlast             = NULL;
  m_p_rvalid            = NULL;
  m_p_rready            = NULL;
  m_p_awid              = NULL;
  m_p_awaddr            = NULL;
  m_p_awlen             = NULL;
  m_p_awsize            = NULL;
  m_p_awburst           = NULL;
  m_p_awlock            = NULL;
  m_p_awcache           = NULL;
  m_p_awprot            = NULL;
  m_p_awqos             = NULL;
  m_p_awbar             = NULL;
  m_p_awdomain          = NULL;
  m_p_awsnoop           = NULL;
  m_p_awvalid           = NULL;
  m_p_awready           = NULL;
  m_p_wdata             = NULL;
  m_p_wstrb             = NULL;
  m_p_wlast             = NULL;
  m_p_wvalid            = NULL;
  m_p_wready            = NULL;
  m_p_bid               = NULL;
  m_p_bresp             = NULL;
  m_p_bvalid            = NULL;
  m_p_bready            = NULL;
  m_p_psel              = NULL;
  m_p_penable           = NULL;
  m_p_pwrite            = NULL;
  m_p_paddr             = NULL;
  m_p_pprot             = NULL;
  m_p_pstrb             = NULL;
  m_p_pwdata            = NULL;
  m_p_pready            = NULL;
  m_p_pslverr           = NULL;
  m_p_prdata            = NULL;
  m_p_en                = NULL;
  m_p_addr              = NULL;
  m_p_lane              = NULL;
  m_p_wrdata            = NULL;
  m_p_wr                = NULL;
  m_p_load              = NULL;
  m_p_retire            = NULL;
  m_p_flush             = NULL;
  m_p_lock              = NULL;
  m_p_attr              = NULL;
  m_p_check_wr          = NULL;
  m_p_check             = NULL;
  m_p_xfer_en           = NULL;
  m_p_busy              = NULL;
  m_p_data              = NULL;


  if (is_pif_or_idma(m_interface_type)) {
    m_p_req_valid               = new bool_input*               [m_num_ports];
    m_p_req_cntl                = new uint_input*               [m_num_ports];
    m_p_req_adrs                = new uint_input*               [m_num_ports];
    m_p_req_data                = new wide_input*               [m_num_ports];
    m_p_req_data_be             = new uint_input*               [m_num_ports];
    m_p_req_id                  = new uint_input*               [m_num_ports];
    m_p_req_priority            = new uint_input*               [m_num_ports];
    m_p_req_route_id            = new uint_input*               [m_num_ports];
    m_p_req_attribute           = new uint_input*               [m_num_ports];
    m_p_req_domain              = new uint_input*               [m_num_ports];
    m_p_req_coh_vadrs           = new uint_input*               [m_num_ports];
    m_p_req_coh_cntl            = new uint_input*               [m_num_ports];
    m_p_req_user_data           = new wide_input*               [m_num_ports];
    m_p_req_rdy                 = new bool_output*              [m_num_ports];
    m_p_resp_valid              = new bool_output*              [m_num_ports];
    m_p_resp_cntl               = new uint_output*              [m_num_ports];
    m_p_resp_data               = new wide_output*              [m_num_ports];
    m_p_resp_id                 = new uint_output*              [m_num_ports];
    m_p_resp_priority           = new uint_output*              [m_num_ports];
    m_p_resp_route_id           = new uint_output*              [m_num_ports];
    m_p_resp_coh_cntl           = new uint_output*              [m_num_ports];
    m_p_resp_user_data          = new wide_output*              [m_num_ports];
    m_p_resp_rdy                = new bool_input*               [m_num_ports];
  }
  else if (is_axi_or_idma(m_interface_type)) {
    u32 num_rd_ports = m_num_axi_rd_ports;
    u32 num_wr_ports = m_num_axi_wr_ports;
    if (num_rd_ports) {
      m_p_arid                  = new uint_input*               [num_rd_ports];
      m_p_araddr                = new uint_input*               [num_rd_ports];
      m_p_arlen                 = new uint_input*               [num_rd_ports];
      m_p_arsize                = new uint_input*               [num_rd_ports];
      m_p_arburst               = new uint_input*               [num_rd_ports];
      m_p_arlock                = new bool_input*               [num_rd_ports];
      m_p_arcache               = new uint_input*               [num_rd_ports];
      m_p_arprot                = new uint_input*               [num_rd_ports];
      m_p_arqos                 = new uint_input*               [num_rd_ports];
      m_p_arbar                 = new uint_input*               [num_rd_ports];
      m_p_ardomain              = new uint_input*               [num_rd_ports];
      m_p_arsnoop               = new uint_input*               [num_rd_ports];
      m_p_arvalid               = new bool_input*               [num_rd_ports];
      m_p_arready               = new bool_output*              [num_rd_ports];
      m_p_rid                   = new uint_output*              [num_rd_ports];
      m_p_rdata                 = new wide_output*              [num_rd_ports];
      m_p_rresp                 = new uint_output*              [num_rd_ports];
      m_p_rlast                 = new bool_output*              [num_rd_ports];
      m_p_rvalid                = new bool_output*              [num_rd_ports];
      m_p_rready                = new bool_input*               [num_rd_ports];
    }
    if (num_wr_ports) {
      m_p_awid                  = new uint_input*               [num_wr_ports];
      m_p_awaddr                = new uint_input*               [num_wr_ports];
      m_p_awlen                 = new uint_input*               [num_wr_ports];
      m_p_awsize                = new uint_input*               [num_wr_ports];
      m_p_awburst               = new uint_input*               [num_wr_ports];
      m_p_awlock                = new bool_input*               [num_wr_ports];
      m_p_awcache               = new uint_input*               [num_wr_ports];
      m_p_awprot                = new uint_input*               [num_wr_ports];
      m_p_awqos                 = new uint_input*               [num_wr_ports];
      m_p_awbar                 = new uint_input*               [num_wr_ports];
      m_p_awdomain              = new uint_input*               [num_wr_ports];
      m_p_awsnoop               = new uint_input*               [num_wr_ports];
      m_p_awvalid               = new bool_input*               [num_wr_ports];
      m_p_awready               = new bool_output*              [num_wr_ports];
      m_p_wdata                 = new wide_input*               [num_wr_ports];
      m_p_wstrb                 = new uint_input*               [num_wr_ports];
      m_p_wlast                 = new bool_input*               [num_wr_ports];
      m_p_wvalid                = new bool_input*               [num_wr_ports];
      m_p_wready                = new bool_output*              [num_wr_ports];
      m_p_bid                   = new uint_output*              [num_wr_ports];
      m_p_bresp                 = new uint_output*              [num_wr_ports];
      m_p_bvalid                = new bool_output*              [num_wr_ports];
      m_p_bready                = new bool_input*               [num_wr_ports];
    }
  }
  else if (is_apb(m_interface_type)) {
    m_p_psel                    = new bool_input*               [m_num_ports];
    m_p_penable                 = new bool_input*               [m_num_ports];
    m_p_pwrite                  = new bool_input*               [m_num_ports];
    m_p_paddr                   = new uint_input*               [m_num_ports];
    m_p_pprot                   = new uint_input*               [m_num_ports];
    m_p_pstrb                   = new uint_input*               [m_num_ports];
    m_p_pwdata                  = new wide_input*               [m_num_ports];
    m_p_pready                  = new bool_output*              [m_num_ports];
    m_p_pslverr                 = new bool_output*              [m_num_ports];
    m_p_prdata                  = new wide_output*              [m_num_ports];
  }
  else {
    m_p_en                      = new bool_input*               [m_num_ports];
    m_p_addr                    = new uint_input*               [m_num_ports];
    m_p_lane                    = new uint_input*               [m_num_ports];
    m_p_wrdata                  = new wide_input*               [m_num_ports];
    m_p_wr                      = new bool_input*               [m_num_ports];
    m_p_load                    = new bool_input*               [m_num_ports];
    m_p_retire                  = new bool_input*               [m_num_ports];
    m_p_flush                   = new bool_input*               [m_num_ports];
    m_p_lock                    = new bool_input*               [m_num_ports];
    m_p_attr                    = new wide_input*               [m_num_ports];
    m_p_check_wr                = new wide_input*               [m_num_ports];
    m_p_check                   = new wide_output*              [m_num_ports];
    m_p_xfer_en                 = new bool_input*               [m_num_ports];
    m_p_busy                    = new bool_output*              [m_num_ports];
    m_p_data                    = new wide_output*              [m_num_ports];
  }

  for (u32 port=0; port<m_num_ports; ++port) {

    ostringstream oss1;
    oss1 << "m_debug_exports[" << port << "]";
    m_debug_exports[port]       = new sc_export<xtsc_debug_if>(oss1.str().c_str());

    ostringstream oss2;
    oss2 << "m_debug_impl[" << port << "]";
    m_debug_impl[port]          = new xtsc_debug_if_impl(oss2.str().c_str(), *this, port);

    (*m_debug_exports[port])(*m_debug_impl[port]);

    m_data[port] = new sc_bv_base((int)m_width8*8);
    *m_data[port] = 0;

    m_data_to_be_written[port] = new sc_bv_base((int)m_width8*8);
    *m_data_to_be_written[port] = 0;

    if (is_pif_or_idma(m_interface_type)) {
      m_rcw_compare_data[port]  = new sc_bv_base((int)m_width8*8);
      *m_rcw_compare_data[port] = 0;
      if (m_rsp_user_data_width1) {
        m_rsp_user_data_val[port] = new sc_bv_base((int)m_rsp_user_data_width1);
        if (!m_rsp_user_data_val_echo) {
          *m_rsp_user_data_val[port] = rsp_user_data_value;
        }
      }
      else {
        m_rsp_user_data_val[port] = NULL;
      }
    }
    else if (is_axi_or_idma(m_interface_type)) {
      m_xID[port]               = new sc_uint_base(m_axi_tran_id_bits[port]);
    }
    else if (is_apb(m_interface_type)) {
      ; // Do nothing
    }
    else {
      sc_length_context length(m_width8*8);
      ostringstream oss1;
      oss1 << "m_read_data_fifo[" << port << "]";
      m_read_data_fifo[port]    = new wide_fifo(oss1.str().c_str(), (m_read_delay_value + 1) * 2);
      ostringstream oss2;
      oss2 << "m_busy_fifo[" << port << "]";
      m_busy_fifo[port]         = new bool_fifo(oss2.str().c_str(), (m_read_delay_value + 1) * 2);
      ostringstream oss3;
      oss3 << "m_read_event_queue[" << port << "]";
      m_read_event_queue[port]  = new sc_event_queue(oss3.str().c_str());
      if (m_has_busy) {
        if (((m_num_subbanks < 2) || ((port % m_num_subbanks) == 0))) {
          ostringstream oss4;
          oss4 << "m_busy_event_queue[" << port << "]";
          m_busy_event_queue[port]  = new sc_event_queue(oss4.str().c_str());
        }
        else {
          m_busy_event_queue[port]  = NULL;
        }
      }
    }

    if (is_pif_or_idma(m_interface_type)) {
      m_p_req_valid         [port] = NULL;
      m_p_req_cntl          [port] = NULL;
      m_p_req_adrs          [port] = NULL;
      m_p_req_data          [port] = NULL;
      m_p_req_data_be       [port] = NULL;
      m_p_req_id            [port] = NULL;
      m_p_req_priority      [port] = NULL;
      m_p_req_route_id      [port] = NULL;
      m_p_req_attribute     [port] = NULL;
      m_p_req_domain        [port] = NULL;
      m_p_req_coh_vadrs     [port] = NULL;
      m_p_req_coh_cntl      [port] = NULL;
      m_p_req_user_data     [port] = NULL;
      m_p_req_rdy           [port] = NULL;
      m_p_resp_valid        [port] = NULL;
      m_p_resp_cntl         [port] = NULL;
      m_p_resp_data         [port] = NULL;
      m_p_resp_id           [port] = NULL;
      m_p_resp_priority     [port] = NULL;
      m_p_resp_route_id     [port] = NULL;
      m_p_resp_coh_cntl     [port] = NULL;
      m_p_resp_user_data    [port] = NULL;
      m_p_resp_rdy          [port] = NULL;
    }
    else if (is_axi_or_idma(m_interface_type)) {
      u32 indx = m_axi_signal_indx[port];
      if (m_axi_read_write[port] == 0) {
        m_p_arid            [indx] = NULL;
        m_p_araddr          [indx] = NULL;
        m_p_arlen           [indx] = NULL;
        m_p_arsize          [indx] = NULL;
        m_p_arburst         [indx] = NULL;
        m_p_arlock          [indx] = NULL;
        m_p_arcache         [indx] = NULL;
        m_p_arprot          [indx] = NULL;
        m_p_arqos           [indx] = NULL;
        m_p_arbar           [indx] = NULL;
        m_p_ardomain        [indx] = NULL;
        m_p_arsnoop         [indx] = NULL;
        m_p_arvalid         [indx] = NULL;
        m_p_arready         [indx] = NULL;
        m_p_rid             [indx] = NULL;
        m_p_rdata           [indx] = NULL;
        m_p_rresp           [indx] = NULL;
        m_p_rlast           [indx] = NULL;
        m_p_rvalid          [indx] = NULL;
        m_p_rready          [indx] = NULL;
      }
      else {
        m_p_awid            [indx] = NULL;
        m_p_awaddr          [indx] = NULL;
        m_p_awlen           [indx] = NULL;
        m_p_awsize          [indx] = NULL;
        m_p_awburst         [indx] = NULL;
        m_p_awlock          [indx] = NULL;
        m_p_awcache         [indx] = NULL;
        m_p_awprot          [indx] = NULL;
        m_p_awqos           [indx] = NULL;
        m_p_awbar           [indx] = NULL;
        m_p_awdomain        [indx] = NULL;
        m_p_awsnoop         [indx] = NULL;
        m_p_awvalid         [indx] = NULL;
        m_p_awready         [indx] = NULL;
        m_p_wdata           [indx] = NULL;
        m_p_wstrb           [indx] = NULL;
        m_p_wlast           [indx] = NULL;
        m_p_wvalid          [indx] = NULL;
        m_p_wready          [indx] = NULL;
        m_p_bid             [indx] = NULL;
        m_p_bresp           [indx] = NULL;
        m_p_bvalid          [indx] = NULL;
        m_p_bready          [indx] = NULL;
      }
    }
    else if (is_apb(m_interface_type)) {
      m_p_psel              [port] = NULL;
      m_p_penable           [port] = NULL;
      m_p_pwrite            [port] = NULL;
      m_p_paddr             [port] = NULL;
      m_p_pprot             [port] = NULL;
      m_p_pstrb             [port] = NULL;
      m_p_pwdata            [port] = NULL;
      m_p_pready            [port] = NULL;
      m_p_pslverr           [port] = NULL;
      m_p_prdata            [port] = NULL;
    }
    else {
      m_p_en                [port] = NULL;
      m_p_addr              [port] = NULL;
      m_p_lane              [port] = NULL;
      m_p_wrdata            [port] = NULL;
      m_p_wr                [port] = NULL;
      m_p_load              [port] = NULL;
      m_p_retire            [port] = NULL;
      m_p_flush             [port] = NULL;
      m_p_lock              [port] = NULL;
      m_p_attr              [port] = NULL;
      m_p_check_wr          [port] = NULL;
      m_p_check             [port] = NULL;
      m_p_xfer_en           [port] = NULL;
      m_p_busy              [port] = NULL;
      m_p_data              [port] = NULL;
    }

    switch (m_interface_type) {
      case DRAM0BS:
      case DRAM0: {
        m_p_addr            [port] = &add_uint_input ("DRam0Addr",              m_address_bits,  m_append_id, port);
        m_p_lane            [port] = &add_uint_input ("DRam0ByteEn",            m_width8,        m_append_id, port);
        m_p_data            [port] = &add_wide_output("DRam0Data",              m_width8*8,      m_append_id, port);
        m_p_en              [port] = &add_bool_input ("DRam0En",                                 m_append_id, port);
        m_p_wr              [port] = &add_bool_input ("DRam0Wr",                                 m_append_id, port);
        m_p_wrdata          [port] = &add_wide_input ("DRam0WrData",            m_width8*8,      m_append_id, port);
        if (m_has_lock && ((m_num_subbanks < 2) || ((port % m_num_subbanks) == 0))) {
        m_p_lock            [port] = &add_bool_input ("DRam0Lock",                               m_append_id, port, true);
        }
        if (m_has_xfer_en) {
        m_p_xfer_en         [port] = &add_bool_input ("XferDRam0En",                             m_append_id, port);
        }
        if (m_has_busy && ((m_num_subbanks < 2) || ((port % m_num_subbanks) == 0))) {
          m_p_busy          [port] = &add_bool_output("DRam0Busy",                               m_append_id, port, true);
        }
        if (m_check_bits) {
        m_p_check_wr        [port] = &add_wide_input ("DRam0CheckWrData",       m_check_bits,    m_append_id, port);
        m_p_check           [port] = &add_wide_output("DRam0CheckData",         m_check_bits,    m_append_id, port);
        }
        break;
      }
      case DRAM1BS:
      case DRAM1: {
        m_p_addr            [port] = &add_uint_input ("DRam1Addr",              m_address_bits,  m_append_id, port);
        m_p_lane            [port] = &add_uint_input ("DRam1ByteEn",            m_width8,        m_append_id, port);
        m_p_data            [port] = &add_wide_output("DRam1Data",              m_width8*8,      m_append_id, port);
        m_p_en              [port] = &add_bool_input ("DRam1En",                                 m_append_id, port);
        m_p_wr              [port] = &add_bool_input ("DRam1Wr",                                 m_append_id, port);
        m_p_wrdata          [port] = &add_wide_input ("DRam1WrData",            m_width8*8,      m_append_id, port);
        if (m_has_lock && ((m_num_subbanks < 2) || ((port % m_num_subbanks) == 0))) {
        m_p_lock            [port] = &add_bool_input ("DRam1Lock",                               m_append_id, port, true);
        }
        if (m_has_xfer_en) {
        m_p_xfer_en         [port] = &add_bool_input ("XferDRam1En",                             m_append_id, port);
        }
        if (m_has_busy && ((m_num_subbanks < 2) || ((port % m_num_subbanks) == 0))) {
          m_p_busy          [port] = &add_bool_output("DRam1Busy",                               m_append_id, port, true);
        }
        if (m_check_bits) {
        m_p_check_wr        [port] = &add_wide_input ("DRam1CheckWrData",       m_check_bits,    m_append_id, port);
        m_p_check           [port] = &add_wide_output("DRam1CheckData",         m_check_bits,    m_append_id, port);
        }
        break;
      }
      case DRAM0RW: {
      if ((port & 0x1) == 0) {
        // Rd ports.  Does not use m_p_wr, m_p_wrdata, and m_p_check_wr
        m_p_addr            [port] = &add_uint_input ("DRam0Addr",              m_address_bits,  m_append_id, port);
        m_p_lane            [port] = &add_uint_input ("DRam0ByteEn",            m_width8,        m_append_id, port);
        m_p_data            [port] = &add_wide_output("DRam0Data",              m_width8*8,      m_append_id, port);
        m_p_en              [port] = &add_bool_input ("DRam0En",                                 m_append_id, port);
        if (m_has_lock) {
        m_p_lock            [port] = &add_bool_input ("DRam0Lock",                               m_append_id, port);
        }
        if (m_has_busy) {
          m_p_busy          [port] = &add_bool_output("DRam0Busy",                               m_append_id, port);
        }
        if (m_dram_attribute_width) {
        m_p_attr            [port] = &add_wide_input ("DRam0Attr",      m_dram_attribute_width,  m_append_id, port);
        }
        if (m_check_bits) {
        m_p_check           [port] = &add_wide_output("DRam0CheckData",         m_check_bits,    m_append_id, port);
        }
      }
      else {
        // Wr ports.  Does not use m_p_en, m_p_data, m_p_lock, and m_p_check
        m_p_addr            [port] = &add_uint_input ("DRam0WrAddr",            m_address_bits,  m_append_id, port);
        m_p_lane            [port] = &add_uint_input ("DRam0WrByteEn",          m_width8,        m_append_id, port);
        m_p_wr              [port] = &add_bool_input ("DRam0Wr",                                 m_append_id, port);
        m_p_wrdata          [port] = &add_wide_input ("DRam0WrData",            m_width8*8,      m_append_id, port);
        if (m_has_busy) {
          m_p_busy          [port] = &add_bool_output("DRam0WrBusy",                             m_append_id, port);
        }
        if (m_dram_attribute_width) {
        m_p_attr            [port] = &add_wide_input ("DRam0WrAttr",    m_dram_attribute_width,  m_append_id, port);
        }
        if (m_check_bits) {
        m_p_check_wr        [port] = &add_wide_input ("DRam0CheckWrData",       m_check_bits,    m_append_id, port);
        }
      }
        break;
      }
      case DRAM1RW: {
      if ((port & 0x1) == 0) {
        // Rd ports.  Does not use m_p_wr, m_p_wrdata, m_p_check_wr, 
        m_p_addr            [port] = &add_uint_input ("DRam1Addr",              m_address_bits,  m_append_id, port);
        m_p_lane            [port] = &add_uint_input ("DRam1ByteEn",            m_width8,        m_append_id, port);
        m_p_data            [port] = &add_wide_output("DRam1Data",              m_width8*8,      m_append_id, port);
        m_p_en              [port] = &add_bool_input ("DRam1En",                                 m_append_id, port);
        if (m_has_lock) {
        m_p_lock            [port] = &add_bool_input ("DRam1Lock",                               m_append_id, port);
        }
        if (m_has_busy) {
          m_p_busy          [port] = &add_bool_output("DRam1Busy",                               m_append_id, port);
        }
        if (m_dram_attribute_width) {
        m_p_attr            [port] = &add_wide_input ("DRam1Attr",      m_dram_attribute_width,  m_append_id, port);
        }
        if (m_check_bits) {
        m_p_check           [port] = &add_wide_output("DRam1CheckData",         m_check_bits,    m_append_id, port);
        }
      }
      else {
        // Wr ports.  Does not use m_p_en, m_p_data, m_p_lock, m_p_check
        m_p_addr            [port] = &add_uint_input ("DRam1WrAddr",            m_address_bits,  m_append_id, port);
        m_p_lane            [port] = &add_uint_input ("DRam1WrByteEn",          m_width8,        m_append_id, port);
        m_p_wr              [port] = &add_bool_input ("DRam1Wr",                                 m_append_id, port);
        m_p_wrdata          [port] = &add_wide_input ("DRam1WrData",            m_width8*8,      m_append_id, port);
        if (m_has_busy) {
          m_p_busy          [port] = &add_bool_output("DRam1WrBusy",                             m_append_id, port);
        }
        if (m_dram_attribute_width) {
        m_p_attr            [port] = &add_wide_input ("DRam1WrAttr",    m_dram_attribute_width,  m_append_id, port);
        }
        if (m_check_bits) {
        m_p_check_wr        [port] = &add_wide_input ("DRam1CheckWrData",       m_check_bits,    m_append_id, port);
        }
      }
        break;
      }
      case DROM0: {
        m_p_addr            [port] = &add_uint_input ("DRom0Addr",              m_address_bits,  m_append_id, port);
        m_p_lane            [port] = &add_uint_input ("DRom0ByteEn",            m_width8,        m_append_id, port);
        m_p_data            [port] = &add_wide_output("DRom0Data",              m_width8*8,      m_append_id, port);
        m_p_en              [port] = &add_bool_input ("DRom0En",                                 m_append_id, port);
        if (m_has_busy) {
          m_p_busy          [port] = &add_bool_output("DRom0Busy",                               m_append_id, port);
        }
        break;
      }
      case IRAM0: {
        m_p_addr            [port] = &add_uint_input ("IRam0Addr",              m_address_bits,  m_append_id, port);
        m_p_data            [port] = &add_wide_output("IRam0Data",              m_width8*8,      m_append_id, port);
        m_p_en              [port] = &add_bool_input ("IRam0En",                                 m_append_id, port);
        if (m_width8 >= 8) {
          m_p_lane          [port] = &add_uint_input ("IRam0WordEn",            m_enable_bits,   m_append_id, port);
        }
        m_p_load            [port] = &add_bool_input ("IRam0LoadStore",                          m_append_id, port);
        m_p_wr              [port] = &add_bool_input ("IRam0Wr",                                 m_append_id, port);
        m_p_wrdata          [port] = &add_wide_input ("IRam0WrData",            m_width8*8,      m_append_id, port);
        if (m_has_xfer_en) {
        m_p_xfer_en         [port] = &add_bool_input ("XferIRam0En",                             m_append_id, port);
        }
        if (m_has_busy) {
          m_p_busy          [port] = &add_bool_output("IRam0Busy",                               m_append_id, port);
        }
        if (m_check_bits) {
        m_p_check_wr        [port] = &add_wide_input ("IRam0CheckWrData",       m_check_bits,    m_append_id, port);
        m_p_check           [port] = &add_wide_output("IRam0CheckData",         m_check_bits,    m_append_id, port);
        }
        break;
      }
      case IRAM1: {
        m_p_addr            [port] = &add_uint_input ("IRam1Addr",              m_address_bits,  m_append_id, port);
        m_p_data            [port] = &add_wide_output("IRam1Data",              m_width8*8,      m_append_id, port);
        m_p_en              [port] = &add_bool_input ("IRam1En",                                 m_append_id, port);
        if (m_width8 >= 8) {
          m_p_lane          [port] = &add_uint_input ("IRam1WordEn",            m_enable_bits,   m_append_id, port);
        }
        m_p_load            [port] = &add_bool_input ("IRam1LoadStore",                          m_append_id, port);
        m_p_wr              [port] = &add_bool_input ("IRam1Wr",                                 m_append_id, port);
        m_p_wrdata          [port] = &add_wide_input ("IRam1WrData",            m_width8*8,      m_append_id, port);
        if (m_has_xfer_en) {
        m_p_xfer_en         [port] = &add_bool_input ("XferIRam1En",                             m_append_id, port);
        }
        if (m_has_busy) {
          m_p_busy          [port] = &add_bool_output("IRam1Busy",                               m_append_id, port);
        }
        if (m_check_bits) {
        m_p_check_wr        [port] = &add_wide_input ("IRam1CheckWrData",       m_check_bits,    m_append_id, port);
        m_p_check           [port] = &add_wide_output("IRam1CheckData",         m_check_bits,    m_append_id, port);
        }
        break;
      }
      case IROM0: {
        m_p_addr            [port] = &add_uint_input ("IRom0Addr",              m_address_bits,  m_append_id, port);
        m_p_data            [port] = &add_wide_output("IRom0Data",              m_width8*8,      m_append_id, port);
        m_p_en              [port] = &add_bool_input ("IRom0En",                                 m_append_id, port);
        if (m_width8 >= 8) {
          m_p_lane          [port] = &add_uint_input ("IRom0WordEn",            m_enable_bits,   m_append_id, port);
        }
        m_p_load            [port] = &add_bool_input ("IRom0Load",                               m_append_id, port);
        if (m_has_xfer_en) {
        m_p_xfer_en         [port] = &add_bool_input ("XferIRom0En",                             m_append_id, port);
        }
        if (m_has_busy) {
          m_p_busy          [port] = &add_bool_output("IRom0Busy",                               m_append_id, port);
        }
        break;
      }
      case URAM0: {
        m_p_addr            [port] = &add_uint_input ("URam0Addr",              m_address_bits,  m_append_id, port);
        m_p_lane            [port] = &add_uint_input ("URam0ByteEn",            m_width8,        m_append_id, port);
        m_p_data            [port] = &add_wide_output("URam0Data",              m_width8*8,      m_append_id, port);
        m_p_en              [port] = &add_bool_input ("URam0En",                                 m_append_id, port);
        m_p_load            [port] = &add_bool_input ("URam0LoadStore",                          m_append_id, port);
        m_p_wr              [port] = &add_bool_input ("URam0Wr",                                 m_append_id, port);
        m_p_wrdata          [port] = &add_wide_input ("URam0WrData",            m_width8*8,      m_append_id, port);
        if (m_has_xfer_en) {
        m_p_xfer_en         [port] = &add_bool_input ("XferURam0En",                             m_append_id, port);
        }
        if (m_has_busy) {
          m_p_busy          [port] = &add_bool_output("URam0Busy",                               m_append_id, port);
        }
        break;
      }
      case XLMI0: {
        m_p_en              [port] = &add_bool_input ("DPort0En",                                m_append_id, port);
        m_p_addr            [port] = &add_uint_input ("DPort0Addr",             m_address_bits,  m_append_id, port);
        m_p_lane            [port] = &add_uint_input ("DPort0ByteEn",           m_width8,        m_append_id, port);
        m_p_wr              [port] = &add_bool_input ("DPort0Wr",                                m_append_id, port);
        m_p_wrdata          [port] = &add_wide_input ("DPort0WrData",           m_width8*8,      m_append_id, port);
        m_p_load            [port] = &add_bool_input ("DPort0Load",                              m_append_id, port);
        m_p_data            [port] = &add_wide_output("DPort0Data",             m_width8*8,      m_append_id, port);
        m_p_retire          [port] = &add_bool_input ("DPort0LoadRetired",                       m_append_id, port);
        m_p_flush           [port] = &add_bool_input ("DPort0RetireFlush",                       m_append_id, port);
        if (m_has_busy) {
          m_p_busy          [port] = &add_bool_output("DPort0Busy",                              m_append_id, port);
        }
        break;
      }
      case IDMA0:
      case PIF: {
        if (m_inbound_pif) {
        m_p_req_valid       [port] = &add_bool_input ("PIReqValid",                              m_append_id, port);
        m_p_req_cntl        [port] = &add_uint_input ("PIReqCntl",              8,               m_append_id, port);
        m_p_req_adrs        [port] = &add_uint_input ("PIReqAdrs",              m_address_bits,  m_append_id, port);
        m_p_req_data        [port] = &add_wide_input ("PIReqData",              m_width8*8,      m_append_id, port);
        m_p_req_data_be     [port] = &add_uint_input ("PIReqDataBE",            m_width8,        m_append_id, port);
        m_p_req_priority    [port] = &add_uint_input ("PIReqPriority",          2,               m_append_id, port);
        m_p_req_rdy         [port] = &add_bool_output("POReqRdy",                                m_append_id, port);
        m_p_resp_valid      [port] = &add_bool_output("PORespValid",                             m_append_id, port);
        m_p_resp_cntl       [port] = &add_uint_output("PORespCntl",             8,               m_append_id, port);
        m_p_resp_data       [port] = &add_wide_output("PORespData",             m_width8*8,      m_append_id, port);
        m_p_resp_priority   [port] = &add_uint_output("PORespPriority",                 2,       m_append_id, port);
        m_p_resp_rdy        [port] = &add_bool_input ("PIRespRdy",                               m_append_id, port);
        if (m_has_request_id) {
          m_p_req_id        [port] = &add_uint_input ("PIReqId",                m_id_bits,       m_append_id, port);
          m_p_resp_id       [port] = &add_uint_output("PORespId",               m_id_bits,       m_append_id, port);
        }
        if (m_route_id_bits) {
          m_p_req_route_id  [port] = &add_uint_input ("PIReqRouteId",           m_route_id_bits, m_append_id, port);
          m_p_resp_route_id [port] = &add_uint_output("PORespRouteId",          m_route_id_bits, m_append_id, port);
        }
        if (m_has_pif_attribute) {
          m_p_req_attribute [port] = &add_uint_input ("PIReqAttribute",         12,              m_append_id, port);
        }
        }
        else {
        // outbound PIF/IDMA0
        string sx((m_interface_type == IDMA0) ? "_iDMA" : "");
        m_p_req_valid       [port] = &add_bool_input ("POReqValid"        +sx,                   m_append_id, port);
        m_p_req_cntl        [port] = &add_uint_input ("POReqCntl"         +sx,  8,               m_append_id, port);
        m_p_req_adrs        [port] = &add_uint_input ("POReqAdrs"         +sx,  m_address_bits,  m_append_id, port);
        m_p_req_data        [port] = &add_wide_input ("POReqData"         +sx,  m_width8*8,      m_append_id, port);
        m_p_req_data_be     [port] = &add_uint_input ("POReqDataBE"       +sx,  m_width8,        m_append_id, port);
        m_p_req_priority    [port] = &add_uint_input ("POReqPriority"     +sx,  2,               m_append_id, port);
        m_p_req_rdy         [port] = &add_bool_output("PIReqRdy"          +sx,                   m_append_id, port);
        m_p_resp_valid      [port] = &add_bool_output("PIRespValid"       +sx,                   m_append_id, port);
        m_p_resp_cntl       [port] = &add_uint_output("PIRespCntl"        +sx,  8,               m_append_id, port);
        m_p_resp_data       [port] = &add_wide_output("PIRespData"        +sx,  m_width8*8,      m_append_id, port);
        m_p_resp_priority   [port] = &add_uint_output("PIRespPriority"    +sx,  2,               m_append_id, port);
        m_p_resp_rdy        [port] = &add_bool_input ("PORespRdy"         +sx,                   m_append_id, port);
        if (m_has_request_id) {
          m_p_req_id        [port] = &add_uint_input ("POReqId"           +sx,  m_id_bits,       m_append_id, port);
          m_p_resp_id       [port] = &add_uint_output("PIRespId"          +sx,  m_id_bits,       m_append_id, port);
        }
        if (m_route_id_bits) {
          m_p_req_route_id  [port] = &add_uint_input ("POReqRouteId"      +sx,  m_route_id_bits, m_append_id, port);
          m_p_resp_route_id [port] = &add_uint_output("PIRespRouteId"     +sx,  m_route_id_bits, m_append_id, port);
        }
        if (m_has_coherence) {
          m_p_req_coh_vadrs [port] = &add_uint_input ("POReqCohVAdrsIndex"+sx,  6,               m_append_id, port);
          m_p_req_coh_cntl  [port] = &add_uint_input ("POReqCohCntl"      +sx,  2,               m_append_id, port);
          m_p_resp_coh_cntl [port] = &add_uint_output("PIRespCohCntl"     +sx,  2,               m_append_id, port);
        }
        if (m_has_pif_attribute) {
          m_p_req_attribute [port] = &add_uint_input ("POReqAttribute"    +sx,  12,              m_append_id, port);
        }
        if (m_has_pif_req_domain) {
          string sy(sx);  // Duplication hack to workaround GCC-4.1 64-bit bug causing Gnu assembler to run forever.
          m_p_req_domain    [port] = &add_uint_input ("POReqDomain"       +sy,  2,               m_append_id, port);
        }
        }
        if (m_req_user_data_width1) {
        m_p_req_user_data   [port] = &add_wide_input (m_req_user_data_name, m_req_user_data_width1, m_append_id, port);
        }
        if (m_rsp_user_data_width1) {
        m_p_resp_user_data  [port] = &add_wide_output(m_rsp_user_data_name, m_rsp_user_data_width1, m_append_id, port);
        }
        break;
      }
      case IDMA:
      case AXI: {
        string px(m_axi_name_prefix[port]);
        u32  indx       = m_axi_signal_indx[port];
        u32  data_bits  = m_width8 * 8;
        u32  strb_bits  = m_width8;
        u32  tid_bits   = m_axi_tran_id_bits[port];
        u32  resp_bits  = ((m_axi_port_type[port] == ACE) ? 4 : 2);
        if (m_axi_read_write[port] == 0) {
          m_p_arid           [indx] = &add_uint_input (px+"ARID",     tid_bits,  false, port);
          m_p_araddr         [indx] = &add_uint_input (px+"ARADDR",   32,        false, port);
          m_p_arlen          [indx] = &add_uint_input (px+"ARLEN",    8,         false, port);
          m_p_arsize         [indx] = &add_uint_input (px+"ARSIZE",   3,         false, port);
          m_p_arburst        [indx] = &add_uint_input (px+"ARBURST",  2,         false, port);
          m_p_arlock         [indx] = &add_bool_input (px+"ARLOCK",              false, port);
          m_p_arcache        [indx] = &add_uint_input (px+"ARCACHE",  4,         false, port);
          m_p_arprot         [indx] = &add_uint_input (px+"ARPROT",   3,         false, port);
          m_p_arqos          [indx] = &add_uint_input (px+"ARQOS",    4,         false, port);
          if (m_axi_port_type[port] != AXI4) {
          m_p_arbar          [indx] = &add_uint_input (px+"ARBAR",    2,         false, port);
          m_p_ardomain       [indx] = &add_uint_input (px+"ARDOMAIN", 2,         false, port);
          m_p_arsnoop        [indx] = &add_uint_input (px+"ARSNOOP",  4,         false, port);
          }
          m_p_arvalid        [indx] = &add_bool_input (px+"ARVALID",             false, port);
          m_p_arready        [indx] = &add_bool_output(px+"ARREADY",             false, port);
          m_p_rid            [indx] = &add_uint_output(px+"RID",      tid_bits,  false, port);
          m_p_rdata          [indx] = &add_wide_output(px+"RDATA",    data_bits, false, port);
          m_p_rresp          [indx] = &add_uint_output(px+"RRESP",    resp_bits, false, port);
          m_p_rlast          [indx] = &add_bool_output(px+"RLAST",               false, port);
          m_p_rvalid         [indx] = &add_bool_output(px+"RVALID",              false, port);
          m_p_rready         [indx] = &add_bool_input (px+"RREADY",              false, port);
        }
        else {
          m_p_awid           [indx] = &add_uint_input (px+"AWID",     tid_bits,  false, port);
          m_p_awaddr         [indx] = &add_uint_input (px+"AWADDR",   32,        false, port);
          m_p_awlen          [indx] = &add_uint_input (px+"AWLEN",    8,         false, port);
          m_p_awsize         [indx] = &add_uint_input (px+"AWSIZE",   3,         false, port);
          m_p_awburst        [indx] = &add_uint_input (px+"AWBURST",  2,         false, port);
          m_p_awlock         [indx] = &add_bool_input (px+"AWLOCK",              false, port);
          m_p_awcache        [indx] = &add_uint_input (px+"AWCACHE",  4,         false, port);
          m_p_awprot         [indx] = &add_uint_input (px+"AWPROT",   3,         false, port);
          m_p_awqos          [indx] = &add_uint_input (px+"AWQOS",    4,         false, port);
          if (m_axi_port_type[port] != AXI4) {
          m_p_awbar          [indx] = &add_uint_input (px+"AWBAR",    2,         false, port);
          m_p_awdomain       [indx] = &add_uint_input (px+"AWDOMAIN", 2,         false, port);
          m_p_awsnoop        [indx] = &add_uint_input (px+"AWSNOOP",  3,         false, port);
          }
          m_p_awvalid        [indx] = &add_bool_input (px+"AWVALID",             false, port);
          m_p_awready        [indx] = &add_bool_output(px+"AWREADY",             false, port);
          m_p_wdata          [indx] = &add_wide_input (px+"WDATA",    data_bits, false, port);
          m_p_wstrb          [indx] = &add_uint_input (px+"WSTRB",    strb_bits, false, port);
          m_p_wlast          [indx] = &add_bool_input (px+"WLAST",               false, port);
          m_p_wvalid         [indx] = &add_bool_input (px+"WVALID",              false, port);
          m_p_wready         [indx] = &add_bool_output(px+"WREADY",              false, port);
          m_p_bid            [indx] = &add_uint_output(px+"BID",      tid_bits,  false, port);
          m_p_bresp          [indx] = &add_uint_output(px+"BRESP",    2,         false, port);
          m_p_bvalid         [indx] = &add_bool_output(px+"BVALID",              false, port);
          m_p_bready         [indx] = &add_bool_input (px+"BREADY",              false, port);
        }
        break;
      }
      case APB: {
        string px(m_axi_name_prefix[port]);
        m_p_psel            [port] = &add_bool_input (px+"PSEL",               m_append_id, port);
        m_p_penable         [port] = &add_bool_input (px+"PENABLE",            m_append_id, port);
        m_p_pwrite          [port] = &add_bool_input (px+"PWRITE",             m_append_id, port);
        m_p_paddr           [port] = &add_uint_input (px+"PADDR",      32,     m_append_id, port);
        m_p_pprot           [port] = &add_uint_input (px+"PPROT",      3,      m_append_id, port);
        m_p_pstrb           [port] = &add_uint_input (px+"PSTRB",      4,      m_append_id, port);
        m_p_pwdata          [port] = &add_wide_input (px+"PWDATA",     32,     m_append_id, port);
        m_p_pready          [port] = &add_bool_output(px+"PREADY",             m_append_id, port);
        m_p_pslverr         [port] = &add_bool_output(px+"PSLVERR",            m_append_id, port);
        m_p_prdata          [port] = &add_wide_output(px+"PRDATA",     32,     m_append_id, port);
        break;
      }
    }

  }

  // Squelch SystemC's complaint about multiple thread "objects"
  sc_actions original_action = sc_report_handler::set_actions("object already exists", SC_WARNING, SC_DO_NOTHING);
  for (u32 port=0; port<m_num_ports; ++port) {
    if (is_pif_or_idma(m_interface_type)) {
      ostringstream oss1;
      oss1 << "pif_request_thread_" << port;
      declare_thread_process(pif_request_thread_handle, oss1.str().c_str(), SC_CURRENT_USER_MODULE, pif_request_thread);
      m_process_handles.push_back(sc_get_current_process_handle());
      sensitive << m_p_req_valid[port]->pos() << m_p_req_rdy[port]->pos();
      ostringstream oss2;
      oss2 << "pif_drive_req_rdy_thread_" << port;
      declare_thread_process(pif_drive_req_rdy_thread_handle, oss2.str().c_str(), SC_CURRENT_USER_MODULE, pif_drive_req_rdy_thread);
      m_process_handles.push_back(sc_get_current_process_handle());
      ostringstream oss3;
      oss3 << "pif_respond_thread_" << port;
      declare_thread_process(pif_respond_thread_handle, oss3.str().c_str(), SC_CURRENT_USER_MODULE, pif_respond_thread);
      m_process_handles.push_back(sc_get_current_process_handle());
    }
    else if (is_axi_or_idma(m_interface_type)) {
      bool read_channel = (m_axi_read_write[port] == 0);
      u32  indx         = m_axi_signal_indx[port];
      ostringstream oss1;
      oss1 << "axi_req_addr_thread_" << port;
      declare_thread_process(axi_req_addr_thread_handle, oss1.str().c_str(), SC_CURRENT_USER_MODULE, axi_req_addr_thread);
      m_process_handles.push_back(sc_get_current_process_handle());
      if (read_channel) { sensitive << m_p_arvalid[indx]->pos() << m_p_arready[indx]->pos(); }
      else              { sensitive << m_p_awvalid[indx]->pos() << m_p_awready[indx]->pos(); }
      ostringstream oss2;
      oss2 << "axi_req_data_thread_" << port;
      declare_thread_process(axi_req_data_thread_handle, oss2.str().c_str(), SC_CURRENT_USER_MODULE, axi_req_data_thread);
      m_process_handles.push_back(sc_get_current_process_handle());
      if (!read_channel) { sensitive << m_p_wvalid[indx]->pos() << m_p_wready[indx]->pos(); }
      ostringstream oss3;
      oss3 << "axi_drive_addr_rdy_thread_" << port;
      declare_thread_process(axi_drive_addr_rdy_thread_handle, oss3.str().c_str(), SC_CURRENT_USER_MODULE, axi_drive_addr_rdy_thread);
      m_process_handles.push_back(sc_get_current_process_handle());
      ostringstream oss4;
      oss4 << "axi_drive_data_rdy_thread_" << port;
      declare_thread_process(axi_drive_data_rdy_thread_handle, oss4.str().c_str(), SC_CURRENT_USER_MODULE, axi_drive_data_rdy_thread);
      m_process_handles.push_back(sc_get_current_process_handle());
      ostringstream oss5;
      oss5 << "axi_rd_rsp_thread_" << port;
      declare_thread_process(axi_rd_rsp_thread_handle, oss5.str().c_str(), SC_CURRENT_USER_MODULE, axi_rd_rsp_thread);
      m_process_handles.push_back(sc_get_current_process_handle());
      ostringstream oss6;
      oss6 << "axi_wr_data_thread_" << port;
      declare_thread_process(axi_wr_data_thread_handle, oss6.str().c_str(), SC_CURRENT_USER_MODULE, axi_wr_data_thread);
      m_process_handles.push_back(sc_get_current_process_handle());
      ostringstream oss7;
      oss7 << "axi_wr_rsp_thread_" << port;
      declare_thread_process(axi_wr_rsp_thread_handle, oss7.str().c_str(), SC_CURRENT_USER_MODULE, axi_wr_rsp_thread);
      m_process_handles.push_back(sc_get_current_process_handle());
    }
    else if (is_apb(m_interface_type)) {
      ostringstream oss1;
      oss1 << "apb_request_thread_" << port;
      declare_thread_process(axi_request_thread_handle, oss1.str().c_str(), SC_CURRENT_USER_MODULE, apb_request_thread);
      m_process_handles.push_back(sc_get_current_process_handle());
      sensitive << m_p_psel[port]->pos();
      ostringstream oss2;
      oss2 << "apb_drive_outputs_thread_" << port;
      declare_thread_process(axi_req_data_thread_handle, oss2.str().c_str(), SC_CURRENT_USER_MODULE, apb_drive_outputs_thread);
      m_process_handles.push_back(sc_get_current_process_handle());
    }
    else if (!m_split_rw) {
      ostringstream oss1;
      oss1 << "lcl_request_thread_" << port;
      declare_thread_process(lcl_request_thread_handle, oss1.str().c_str(), SC_CURRENT_USER_MODULE, lcl_request_thread);
      m_process_handles.push_back(sc_get_current_process_handle());
      sensitive << m_p_en[port]->pos();
      ostringstream oss2;
      oss2 << "lcl_drive_read_data_thread_" << port;
      declare_thread_process(lcl_drive_read_data_thread_handle, oss2.str().c_str(), SC_CURRENT_USER_MODULE, lcl_drive_read_data_thread);
      m_process_handles.push_back(sc_get_current_process_handle());
      sensitive << *m_read_event_queue[port];
      if (m_has_busy && ((m_num_subbanks < 2) || ((port % m_num_subbanks) == 0))) {
        ostringstream oss3;
        oss3 << "lcl_drive_busy_thread_" << port;
        declare_thread_process(lcl_drive_busy_thread_handle, oss3.str().c_str(), SC_CURRENT_USER_MODULE, lcl_drive_busy_thread);
        m_process_handles.push_back(sc_get_current_process_handle());
        sensitive << *m_busy_event_queue[port];
      }
    }
    else if ((port & 0x1) == 0) {
      // Rd port of split Rd/Wr interface
      ostringstream oss1;
      oss1 << "lcl_rd_request_thread_" << port;
      declare_thread_process(lcl_rd_request_thread_handle, oss1.str().c_str(), SC_CURRENT_USER_MODULE, lcl_rd_request_thread);
      m_process_handles.push_back(sc_get_current_process_handle());
      sensitive << m_p_en[port]->pos();
      ostringstream oss2;
      oss2 << "lcl_drive_read_data_thread_" << port;
      declare_thread_process(lcl_drive_read_data_thread_handle, oss2.str().c_str(), SC_CURRENT_USER_MODULE, lcl_drive_read_data_thread);
      m_process_handles.push_back(sc_get_current_process_handle());
      sensitive << *m_read_event_queue[port];
      if (m_has_busy) {
        ostringstream oss3;
        oss3 << "lcl_drive_busy_thread_" << port;
        declare_thread_process(lcl_drive_busy_thread_handle, oss3.str().c_str(), SC_CURRENT_USER_MODULE, lcl_drive_busy_thread);
        m_process_handles.push_back(sc_get_current_process_handle());
        sensitive << *m_busy_event_queue[port];
      }
    }
    else {
      // Wr port of split Rd/Wr interface
      ostringstream oss1;
      oss1 << "lcl_wr_request_thread_" << port;
      declare_thread_process(lcl_wr_request_thread_handle, oss1.str().c_str(), SC_CURRENT_USER_MODULE, lcl_wr_request_thread);
      m_process_handles.push_back(sc_get_current_process_handle());
      sensitive << m_p_wr[port]->pos();
      if (m_has_busy) {
        ostringstream oss3;
        oss3 << "lcl_drive_busy_thread_" << port;
        declare_thread_process(lcl_drive_busy_thread_handle, oss3.str().c_str(), SC_CURRENT_USER_MODULE, lcl_drive_busy_thread);
        m_process_handles.push_back(sc_get_current_process_handle());
        sensitive << *m_busy_event_queue[port];
      }
    }
  }
  // Restore SystemC
  sc_report_handler::set_actions("object already exists", SC_WARNING, original_action);

  m_p_memory->load_initial_values();

  for (u32 i=0; i<m_num_ports; ++i) {
    ostringstream oss;
    oss.str(""); oss << "m_debug_exports"   << "[" << i << "]"; m_port_types[oss.str()] = DEBUG_EXPORT;
    oss.str(""); oss << "debug_export"      << "[" << i << "]"; m_port_types[oss.str()] = PORT_TABLE;
    oss.str(""); oss << m_interface_lc      << "[" << i << "]"; m_port_types[oss.str()] = PORT_TABLE;

    set<string> p;
    p = get_bool_input_set (i); for (set<string>::const_iterator j = p.begin(); j != p.end(); ++j) { m_port_types[*j] = BOOL_INPUT;  }
    p = get_uint_input_set (i); for (set<string>::const_iterator j = p.begin(); j != p.end(); ++j) { m_port_types[*j] = UINT_INPUT;  }
    p = get_wide_input_set (i); for (set<string>::const_iterator j = p.begin(); j != p.end(); ++j) { m_port_types[*j] = WIDE_INPUT;  }
    p = get_bool_output_set(i); for (set<string>::const_iterator j = p.begin(); j != p.end(); ++j) { m_port_types[*j] = BOOL_OUTPUT; }
    p = get_uint_output_set(i); for (set<string>::const_iterator j = p.begin(); j != p.end(); ++j) { m_port_types[*j] = UINT_OUTPUT; }
    p = get_wide_output_set(i); for (set<string>::const_iterator j = p.begin(); j != p.end(); ++j) { m_port_types[*j] = WIDE_OUTPUT; }
  }

  m_port_types["debug_exports"] = PORT_TABLE;
  m_port_types[m_interface_lc] = PORT_TABLE;

  if (m_num_ports == 1) {
    m_port_types["debug_export"] = PORT_TABLE;
  }

  xtsc_register_command(*this, *this, "dump", 2, 2,
      "dump <StartAddress> <NumBytes>", 
      "Dump <NumBytes> of memory starting at <StartAddress> (includes header and printable ASCII column)."
  );

  xtsc_register_command(*this, *this, "dump_debug_info", 0, 0,
      "dump_debug_info", 
      "Dump misc debug information such as the number of entries in various FIFO's."
  );

  xtsc_register_command(*this, *this, "peek", 2, 2,
      "peek <StartAddress> <NumBytes>", 
      "Peek <NumBytes> of memory starting at <StartAddress>."
  );

  xtsc_register_command(*this, *this, "poke", 2, -1,
      "poke <StartAddress> <NumBytes> <Byte1> <Byte2> . . . <ByteN>", 
      "Poke <NumBytes> (=N) of memory starting at <StartAddress>."
  );

  // Log our construction
  log4xtensa::LogLevel ll = xtsc_get_constructor_log_level();
  XTSC_LOG(m_text, ll, "Constructed " << kind() << " '" << name() << "':");
  XTSC_LOG(m_text, ll, " memory_interface       = "                 << m_interface_uc);
  if (m_interface_type == PIF) {
  XTSC_LOG(m_text, ll, " inbound_pif            = "   << boolalpha  << m_inbound_pif);
  if (!m_inbound_pif) {
  XTSC_LOG(m_text, ll, " has_coherence          = "   << boolalpha  << m_has_coherence);
  }
  XTSC_LOG(m_text, ll, " req_user_data          = "                 << m_req_user_data);
  XTSC_LOG(m_text, ll, " rsp_user_data          = "                 << m_rsp_user_data);
  XTSC_LOG(m_text, ll, " rsp_user_data_val      = "                 << rsp_user_data_val);
  }
  if (is_pif_or_idma(m_interface_type)) {
  XTSC_LOG(m_text, ll, " has_pif_attribute      = "   << boolalpha  << m_has_pif_attribute);
  if (!m_inbound_pif) {
  XTSC_LOG(m_text, ll, " has_pif_req_domain     = "   << boolalpha  << m_has_pif_req_domain);
  }
  }
  XTSC_LOG(m_text, ll, " num_ports              = "   << dec        << m_num_ports);
  XTSC_LOG(m_text, ll, " banked                 = "   << boolalpha  << m_banked);
  XTSC_LOG(m_text, ll, " num_subbanks           = "   << dec        << m_num_subbanks);
  XTSC_LOG(m_text, ll, " port_name_suffix       = "                 << m_suffix);
  XTSC_LOG(m_text, ll, " byte_width             = "   << dec        << m_width8);
  if (is_amba(m_interface_type)) {
  XTSC_LOG(m_text, ll, " axi_name_prefix        = "                 << axi_name_prefix);
  }
  if (is_axi_or_idma(m_interface_type)) {
  log_vu32(m_text, ll, " axi_tran_id_bits",                            m_axi_tran_id_bits, 24);
  log_vu32(m_text, ll, " axi_read_write",                              m_axi_read_write,   24);
  log_vu32(m_text, ll, " axi_port_type",                               m_axi_port_type,    24);
  }
  XTSC_LOG(m_text, ll, " start_byte_address     = 0x" << hex        << m_start_address8);
  XTSC_LOG(m_text, ll, " memory_byte_size       = 0x" << hex        << m_size8 << (m_size8 ? " " : " (4GB)"));
  XTSC_LOG(m_text, ll, " End byte address       = 0x" << hex        << m_end_address8);
  XTSC_LOG(m_text, ll, " use_fast_access        = "   << boolalpha  << m_use_fast_access);
  XTSC_LOG(m_text, ll, " big_endian             = "   << boolalpha  << m_big_endian);
  XTSC_LOG(m_text, ll, " page_byte_size         = 0x" << hex        << m_p_memory->m_page_size8);
  XTSC_LOG(m_text, ll, " initial_value_file     = "   << hex        << m_p_memory->m_initial_value_file);
  XTSC_LOG(m_text, ll, " memory_fill_byte       = 0x" << hex        << (u32) m_p_memory->m_memory_fill_byte);
  XTSC_LOG(m_text, ll, " vcd_handle             = "                 << m_p_trace_file);
  if (clock_period == 0xFFFFFFFF) {
  XTSC_LOG(m_text, ll, " clock_period           = 0xFFFFFFFF => "   << m_clock_period.value() << " (" << m_clock_period << ")");
  } else {
  XTSC_LOG(m_text, ll, " clock_period           = "                 << clock_period << " (" << m_clock_period << ")");
  }
  if (posedge_offset == 0xFFFFFFFF) {
  XTSC_LOG(m_text, ll, " posedge_offset         = 0xFFFFFFFF => "   << m_posedge_offset.value() << " (" << m_posedge_offset << ")");
  } else {
  XTSC_LOG(m_text, ll, " posedge_offset         = "                 << posedge_offset << " (" << m_posedge_offset << ")");
  }
  XTSC_LOG(m_text, ll, " sample_phase           = "   << dec        << sample_phase << " (" << m_sample_phase << ")");
  XTSC_LOG(m_text, ll, " drive_phase            = "   << dec        << drive_phase << " (" << m_drive_phase << ")");
  if (is_system_memory(m_interface_type) && !is_apb(m_interface_type)) {
  XTSC_LOG(m_text, ll, " request_fifo_depth     = "   << dec        << m_request_fifo_depth);
  }
  if (is_pif_or_idma(m_interface_type)) {
  XTSC_LOG(m_text, ll, " has_request_id         = "   << boolalpha  << m_has_request_id);
  XTSC_LOG(m_text, ll, " write_responses        = "   << boolalpha  << m_write_responses);
  XTSC_LOG(m_text, ll, " route_id_bits          = "   << dec        << m_route_id_bits);
  XTSC_LOG(m_text, ll, " void_resp_cntl         = 0x" << hex        << m_resp_cntl_zero.get_value());
  XTSC_LOG(m_text, ll, " read_delay             = "   << dec        << memory_parms.get_u32("read_delay"));
  XTSC_LOG(m_text, ll, " write_delay            = "   << dec        << memory_parms.get_u32("write_delay"));
  XTSC_LOG(m_text, ll, " block_read_delay       = "   << dec        << memory_parms.get_u32("block_read_delay"));
  XTSC_LOG(m_text, ll, " block_read_repeat      = "   << dec        << memory_parms.get_u32("block_read_repeat"));
  XTSC_LOG(m_text, ll, " block_write_delay      = "   << dec        << memory_parms.get_u32("block_write_delay"));
  XTSC_LOG(m_text, ll, " block_write_repeat     = "   << dec        << memory_parms.get_u32("block_write_repeat"));
  XTSC_LOG(m_text, ll, " block_write_response   = "   << dec        << memory_parms.get_u32("block_write_response"));
  XTSC_LOG(m_text, ll, " rcw_repeat             = "   << dec        << memory_parms.get_u32("rcw_repeat"));
  XTSC_LOG(m_text, ll, " rcw_response           = "   << dec        << memory_parms.get_u32("rcw_response"));
  } else if (is_apb(m_interface_type)) {
  XTSC_LOG(m_text, ll, " read_delay             = "   << dec        << memory_parms.get_u32("read_delay"));
  XTSC_LOG(m_text, ll, " write_delay            = "   << dec        << memory_parms.get_u32("write_delay"));
  } else {
  XTSC_LOG(m_text, ll, " read_delay             = "   << dec        << memory_parms.get_u32("read_delay"));
  XTSC_LOG(m_text, ll, " address_bits           = "   << dec        << m_address_bits);
  XTSC_LOG(m_text, ll, " has_busy               = "   << boolalpha  << m_has_busy);
  }
  if (is_pif_or_idma(m_interface_type) || is_axi_or_idma(m_interface_type)) {
  XTSC_LOG(m_text, ll, " burst_read_delay       = "   << dec        << memory_parms.get_u32("burst_read_delay"));
  XTSC_LOG(m_text, ll, " burst_read_repeat      = "   << dec        << memory_parms.get_u32("burst_read_repeat"));
  XTSC_LOG(m_text, ll, " burst_write_delay      = "   << dec        << memory_parms.get_u32("burst_write_delay"));
  XTSC_LOG(m_text, ll, " burst_write_repeat     = "   << dec        << memory_parms.get_u32("burst_write_repeat"));
  XTSC_LOG(m_text, ll, " burst_write_response   = "   << dec        << memory_parms.get_u32("burst_write_response"));
  }
  XTSC_LOG(m_text, ll, " busy_percentage        = "   << dec        << m_busy_percentage << "%");
  if (is_axi_or_idma(m_interface_type)) {
  XTSC_LOG(m_text, ll, " data_busy_percentage   = "   << dec        << m_data_busy_percentage << "%");
  }
  if ((m_interface_type == DRAM0) || (m_interface_type == DRAM0BS) || (m_interface_type == DRAM0RW) ||
      (m_interface_type == DRAM1) || (m_interface_type == DRAM1BS) || (m_interface_type == DRAM1RW))
  {
  XTSC_LOG(m_text, ll, " has_lock               = "   << boolalpha  << m_has_lock);
  }
  if ((m_interface_type == DRAM0) || (m_interface_type == DRAM0BS) || (m_interface_type == DRAM0RW) ||
      (m_interface_type == DRAM1) || (m_interface_type == DRAM1BS) || (m_interface_type == DRAM1RW) ||
      (m_interface_type == IRAM0) || (m_interface_type == IRAM1  ))
  {
  XTSC_LOG(m_text, ll, " check_bits             = "   << dec        << m_check_bits);
  }
  if ((m_interface_type != DROM0) && (m_interface_type != XLMI0) && !is_pif_or_idma(m_interface_type)) {
  XTSC_LOG(m_text, ll, " has_xfer_en            = "   << boolalpha  << m_has_xfer_en);
  }
  XTSC_LOG(m_text, ll, " cbox                   = "   << boolalpha  << m_cbox);
  if (m_split_rw) {
  XTSC_LOG(m_text, ll, " has_dma                = "   << boolalpha  << m_has_dma);
  XTSC_LOG(m_text, ll, " dram_attribute_width   = "                 << m_dram_attribute_width);
  }
  XTSC_LOG(m_text, ll, " m_address_mask         = 0x" << hex        << m_address_mask);
  XTSC_LOG(m_text, ll, " m_address_shift        = "   << dec        << m_address_shift);
  ostringstream oss;
  oss << "Port List:" << endl;
  dump_ports(oss);
  xtsc_log_multiline(m_text, ll, oss.str(), 2);

}



xtsc_component::xtsc_memory_pin::~xtsc_memory_pin(void) {
  // Do any required clean-up here
  if (m_p_memory) {
    delete m_p_memory;
    m_p_memory = 0;
  }
}



u32 xtsc_component::xtsc_memory_pin::get_bit_width(const string& port_name, u32 interface_num) const {
  string name_portion;
  u32    index;
  xtsc_parse_port_name(port_name, name_portion, index);

  if (get_port_type(port_name) == DEBUG_EXPORT) {
   return m_width8 * 8;
  }
  else {
    return xtsc_module_pin_base::get_bit_width(port_name);
  }
}



sc_object *xtsc_component::xtsc_memory_pin::get_port(const string& port_name) {
  if (has_bool_input (port_name)) return &get_bool_input (port_name);
  if (has_uint_input (port_name)) return &get_uint_input (port_name);
  if (has_wide_input (port_name)) return &get_wide_input (port_name);
  if (has_bool_output(port_name)) return &get_bool_output(port_name);
  if (has_uint_output(port_name)) return &get_uint_output(port_name);
  if (has_wide_output(port_name)) return &get_wide_output(port_name);

  string name_portion;
  u32    index;
  bool   indexed = xtsc_parse_port_name(port_name, name_portion, index);

  if (((m_num_ports > 1) && !indexed) || (name_portion != "m_debug_exports")) {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\" has no port named \"" << port_name << "\"" << endl;
    throw xtsc_exception(oss.str());
  }

  if (index > m_num_ports - 1) {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\": Invalid index: \"" << port_name << "\".  Valid range: 0-" << (m_num_ports-1) << endl;
    throw xtsc_exception(oss.str());
  }

  return (sc_object*) m_debug_exports[index];
}



xtsc_port_table xtsc_component::xtsc_memory_pin::get_port_table(const string& port_table_name) const {
  xtsc_port_table table;

  if (port_table_name == "debug_exports") {
    for (u32 i=0; i<m_num_ports; ++i) {
      ostringstream oss;
      oss << "debug_export[" << i << "]";
      table.push_back(oss.str());
    }
    return table;
  }
  
  if (port_table_name == m_interface_lc) {
    for (u32 i=0; i<m_num_ports; ++i) {
      ostringstream oss;
      oss << m_interface_lc << "[" << i << "]";
      table.push_back(oss.str());
    }
    return table;
  }

  string name_portion;
  u32    index;
  bool   indexed = xtsc_parse_port_name(port_table_name, name_portion, index);

  if (((m_num_ports > 1) && !indexed) || ((name_portion != "debug_export") && (name_portion != m_interface_lc))) {
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

  if (name_portion == "debug_export") {
    oss.str(""); oss << "m_debug_exports" << "[" << index << "]"; table.push_back(oss.str());
  }
  else {
    set<string> p;
    p = get_bool_input_set (index); for (set<string>::const_iterator j = p.begin(); j != p.end(); ++j) { table.push_back(*j); }
    p = get_uint_input_set (index); for (set<string>::const_iterator j = p.begin(); j != p.end(); ++j) { table.push_back(*j); }
    p = get_wide_input_set (index); for (set<string>::const_iterator j = p.begin(); j != p.end(); ++j) { table.push_back(*j); }
    p = get_bool_output_set(index); for (set<string>::const_iterator j = p.begin(); j != p.end(); ++j) { table.push_back(*j); }
    p = get_uint_output_set(index); for (set<string>::const_iterator j = p.begin(); j != p.end(); ++j) { table.push_back(*j); }
    p = get_wide_output_set(index); for (set<string>::const_iterator j = p.begin(); j != p.end(); ++j) { table.push_back(*j); }
  }

  return table;
}



string xtsc_component::xtsc_memory_pin::adjust_name_and_check_size(const string&                                port_name,
                                                                   const xtsc_tlm2pin_memory_transactor&        tlm2pin,
                                                                   u32                                          tlm2pin_port,
                                                                   const set_string&                            transactor_set) const
{
  string transactor_port_name(port_name);
  if (m_append_id) {
    transactor_port_name.erase(transactor_port_name.size()-1);
  }
  if (tlm2pin.get_append_id()) {
    ostringstream oss;
    oss << tlm2pin_port;
    transactor_port_name += oss.str();
  }
  u32 memory_width = get_bit_width(port_name);
  u32 transactor_width = tlm2pin.get_bit_width(transactor_port_name);
  if (memory_width != transactor_width) {
    ostringstream oss;
    oss << "Signal mismatch in connect():  " << kind() << " '" << name() << "' has a port named \"" << port_name
        << "\" with a width of " << memory_width << " bits, but port \"" << transactor_port_name << "\" in " << tlm2pin.kind()
        << " '" << tlm2pin.name() << "' has " << transactor_width << " bits.";
    throw xtsc_exception(oss.str());
  }
  return transactor_port_name;
} 



void xtsc_component::xtsc_memory_pin::dump_set_string(ostringstream& oss, const set_string& strings, const string& indent) {
  for (set_string::const_iterator is = strings.begin(); is != strings.end(); ++is) {
    oss << indent << *is << endl;
  }
}



void xtsc_component::xtsc_memory_pin::execute(const string&                 cmd_line, 
                                              const vector<string>&         words,
                                              const vector<string>&         words_lc,
                                              ostream&                      result)
{
  ostringstream res;

  if (false) {
  }
  else if (words[0] == "dump") {
    u32 start_address = xtsc_command_argtou32(cmd_line, words, 1);
    u32 num_bytes     = xtsc_command_argtou32(cmd_line, words, 2);
    m_p_memory->byte_dump(start_address, num_bytes, res);
  }
  else if (words[0] == "dump_debug_info") {
    if (is_pif_or_idma(m_interface_type)) {
      res << "m_pif_req_pool.size()=" << m_pif_req_pool.size() << endl;
    }
    else if (is_axi_or_idma(m_interface_type)) {
      res << "m_axi_addr_pool.size()=" << m_axi_addr_pool.size() << endl;
      res << "m_axi_data_pool.size()=" << m_axi_data_pool.size() << endl;
    }
    else if (is_apb(m_interface_type)) {
      ; // Do nothing
    }
    for (u32 port=0; port<m_num_ports; ++port) {
      if (is_pif_or_idma(m_interface_type)) {
        res << "m_pif_req_fifo[" << port << "].size()=" << m_pif_req_fifo[port].size() << endl;
      }
      else if (is_axi_or_idma(m_interface_type)) {
        res << "m_axi_addr_fifo  [" << port << "].size()=" << m_axi_addr_fifo  [port].size() << endl;
        res << "m_axi_data_fifo  [" << port << "].size()=" << m_axi_data_fifo  [port].size() << endl;
        res << "m_axi_wr_rsp_fifo[" << port << "].size()=" << m_axi_wr_rsp_fifo[port].size() << endl;
      }
      else if (is_apb(m_interface_type)) {
        ; // Do nothing
      }
      else {
        res << "m_read_data_fifo[" << port << "]->num_available()=" << m_read_data_fifo[port]->num_available() << endl;
        res << "m_busy_fifo     [" << port << "]->num_available()=" << m_busy_fifo     [port]->num_available() << endl;
      }
    }
  }
  else if (words[0] == "peek") {
    u32 start_address = xtsc_command_argtou32(cmd_line, words, 1);
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
    u32 start_address = xtsc_command_argtou32(cmd_line, words, 1);
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
  else {
    ostringstream oss;
    oss << name() << "::" << __FUNCTION__ << "() called for unknown command '" << cmd_line << "'.";
    throw xtsc_exception(oss.str());
  }

  result << res.str();
}



xtsc::u32 xtsc_component::xtsc_memory_pin::connect(xtsc_tlm2pin_memory_transactor&      tlm2pin,
                                                   u32                                  tlm2pin_port,
                                                   u32                                  mem_port,
                                                   bool                                 single_connect)
{
  u32 tran_ports = tlm2pin.get_num_ports();
  if (tlm2pin_port >= tran_ports) {
    ostringstream oss;
    oss << "Invalid tlm2pin_port=" << tlm2pin_port << " in connect(): " << endl;
    oss << tlm2pin.kind() << " '" << tlm2pin.name() << "' has " << tran_ports << " ports numbered from 0 to " << tran_ports-1
        << endl;
    throw xtsc_exception(oss.str());
  }
  if (mem_port >= m_num_ports) {
    ostringstream oss;
    oss << "Invalid mem_port=" << mem_port << " in connect(): " << endl;
    oss << kind() << " '" << name() << "' has " << m_num_ports << " ports numbered from 0 to " << m_num_ports-1 << endl;
    throw xtsc_exception(oss.str());
  }

  u32 num_connected = 0;

  while ((tlm2pin_port < tran_ports) && (mem_port < m_num_ports)) {

    {
      // Connect sc_in<bool> ports of xtsc_memory_pin to sc_out<bool> ports of xtsc_tlm2pin_memory_transactor 
      set_string mem_set = get_bool_input_set(mem_port);
      set_string tran_set = tlm2pin.get_bool_output_set(tlm2pin_port);
      if (mem_set.size() != tran_set.size()) {
        ostringstream oss;
        oss << "Signal set sizes don't match in xtsc_memory_pin::connect():" << endl;
        oss << kind() << " '" << name() << "':" << endl;
        dump_set_string(oss, mem_set, " ");
        oss << tlm2pin.kind() << " '" << tlm2pin.name() << "':" << endl;
        dump_set_string(oss, tran_set, " ");
        throw xtsc_exception(oss.str());
      }
      for (set_string::const_iterator is = mem_set.begin(); is != mem_set.end(); ++is) {
        string their_name = adjust_name_and_check_size(*is, tlm2pin, tlm2pin_port, tran_set);
        string signal_name = name() + ("__" + *is);
        bool_signal& signal = create_bool_signal(signal_name);
        get_bool_input(*is)(signal);
        tlm2pin.get_bool_output(their_name)(signal);
      }
    }


    {
      // Connect sc_in<sc_uint_base> ports of xtsc_memory_pin to sc_out<sc_uint_base> ports of xtsc_tlm2pin_memory_transactor 
      set_string mem_set = get_uint_input_set(mem_port);
      set_string tran_set = tlm2pin.get_uint_output_set(tlm2pin_port);
      if (mem_set.size() != tran_set.size()) {
        ostringstream oss;
        oss << "Signal set sizes don't match in xtsc_memory_pin::connect():" << endl;
        oss << kind() << " '" << name() << "':" << endl;
        dump_set_string(oss, mem_set, " ");
        oss << tlm2pin.kind() << " '" << tlm2pin.name() << "':" << endl;
        dump_set_string(oss, tran_set, " ");
        throw xtsc_exception(oss.str());
      }
      for (set_string::const_iterator is = mem_set.begin(); is != mem_set.end(); ++is) {
        string their_name = adjust_name_and_check_size(*is, tlm2pin, tlm2pin_port, tran_set);
        string signal_name = name() + ("__" + *is);
        uint_signal& signal = create_uint_signal(signal_name, get_bit_width(*is));
        get_uint_input(*is)(signal);
        tlm2pin.get_uint_output(their_name)(signal);
      }
    }


    {
      // Connect sc_in<sc_bv_base> ports of xtsc_memory_pin to sc_out<sc_bv_base> ports of xtsc_tlm2pin_memory_transactor 
      set_string mem_set = get_wide_input_set(mem_port);
      set_string tran_set = tlm2pin.get_wide_output_set(tlm2pin_port);
      if (mem_set.size() != tran_set.size()) {
        ostringstream oss;
        oss << "Signal set sizes don't match in xtsc_memory_pin::connect():" << endl;
        oss << kind() << " '" << name() << "':" << endl;
        dump_set_string(oss, mem_set, " ");
        oss << tlm2pin.kind() << " '" << tlm2pin.name() << "':" << endl;
        dump_set_string(oss, tran_set, " ");
        throw xtsc_exception(oss.str());
      }
      for (set_string::const_iterator is = mem_set.begin(); is != mem_set.end(); ++is) {
        string their_name = adjust_name_and_check_size(*is, tlm2pin, tlm2pin_port, tran_set);
        string signal_name = name() + ("__" + *is);
        wide_signal& signal = create_wide_signal(signal_name, get_bit_width(*is));
        get_wide_input(*is)(signal);
        tlm2pin.get_wide_output(their_name)(signal);
      }
    }


    {
      // Connect sc_out<bool> ports of xtsc_memory_pin to sc_in<bool> ports of xtsc_tlm2pin_memory_transactor 
      set_string mem_set = get_bool_output_set(mem_port);
      set_string tran_set = tlm2pin.get_bool_input_set(tlm2pin_port);
      if (mem_set.size() != tran_set.size()) {
        ostringstream oss;
        oss << "Signal set sizes don't match in xtsc_memory_pin::connect():" << endl;
        oss << kind() << " '" << name() << "':" << endl;
        dump_set_string(oss, mem_set, " ");
        oss << tlm2pin.kind() << " '" << tlm2pin.name() << "':" << endl;
        dump_set_string(oss, tran_set, " ");
        throw xtsc_exception(oss.str());
      }
      for (set_string::const_iterator is = mem_set.begin(); is != mem_set.end(); ++is) {
        string their_name = adjust_name_and_check_size(*is, tlm2pin, tlm2pin_port, tran_set);
        string signal_name = name() + ("__" + *is);
        bool_signal& signal = create_bool_signal(signal_name);
        get_bool_output(*is)(signal);
        tlm2pin.get_bool_input(their_name)(signal);
      }
    }


    {
      // Connect sc_out<sc_uint_base> ports of xtsc_memory_pin to sc_in<sc_uint_base> ports of xtsc_tlm2pin_memory_transactor 
      set_string mem_set = get_uint_output_set(mem_port);
      set_string tran_set = tlm2pin.get_uint_input_set(tlm2pin_port);
      if (mem_set.size() != tran_set.size()) {
        ostringstream oss;
        oss << "Signal set sizes don't match in xtsc_memory_pin::connect():" << endl;
        oss << kind() << " '" << name() << "':" << endl;
        dump_set_string(oss, mem_set, " ");
        oss << tlm2pin.kind() << " '" << tlm2pin.name() << "':" << endl;
        dump_set_string(oss, tran_set, " ");
        throw xtsc_exception(oss.str());
      }
      for (set_string::const_iterator is = mem_set.begin(); is != mem_set.end(); ++is) {
        string their_name = adjust_name_and_check_size(*is, tlm2pin, tlm2pin_port, tran_set);
        string signal_name = name() + ("__" + *is);
        uint_signal& signal = create_uint_signal(signal_name, get_bit_width(*is));
        get_uint_output(*is)(signal);
        tlm2pin.get_uint_input(their_name)(signal);
      }
    }


    {
      // Connect sc_out<sc_bv_base> ports of xtsc_memory_pin to sc_in<sc_bv_base> ports of xtsc_tlm2pin_memory_transactor 
      set_string mem_set = get_wide_output_set(mem_port);
      set_string tran_set = tlm2pin.get_wide_input_set(tlm2pin_port);
      if (mem_set.size() != tran_set.size()) {
        ostringstream oss;
        oss << "Signal set sizes don't match in xtsc_memory_pin::connect():" << endl;
        oss << kind() << " '" << name() << "':" << endl;
        dump_set_string(oss, mem_set, " ");
        oss << tlm2pin.kind() << " '" << tlm2pin.name() << "':" << endl;
        dump_set_string(oss, tran_set, " ");
        throw xtsc_exception(oss.str());
      }
      for (set_string::const_iterator is = mem_set.begin(); is != mem_set.end(); ++is) {
        string their_name = adjust_name_and_check_size(*is, tlm2pin, tlm2pin_port, tran_set);
        string signal_name = name() + ("__" + *is);
        wide_signal& signal = create_wide_signal(signal_name, get_bit_width(*is));
        get_wide_output(*is)(signal);
        tlm2pin.get_wide_input(their_name)(signal);
      }
    }

    // Connect the debug interface
    if (!tlm2pin.has_dso()) {
      (*tlm2pin.m_debug_ports[tlm2pin_port])(*m_debug_exports[mem_port]);
    }

    mem_port += 1;
    tlm2pin_port += 1;
    num_connected += 1;

    if (single_connect) break;
    if (tlm2pin_port >= tran_ports) break;
    if (mem_port >= m_num_ports) break;
    if (m_debug_impl[mem_port]->is_connected()) break;
  }

  return num_connected;
}



void xtsc_component::xtsc_memory_pin::end_of_elaboration(void) {
} 



void xtsc_component::xtsc_memory_pin::start_of_simulation(void) {
  reset();
} 



void xtsc_component::xtsc_memory_pin::reset(bool hard_reset) {
  XTSC_INFO(m_text, "xtsc_memory_pin::reset()");

  m_next_port_lcl_request_thread         = 0;
  m_next_port_lcl_drive_read_data_thread = 0;
  m_next_port_lcl_drive_busy_thread      = 0;
  m_next_port_pif_request_thread         = 0;
  m_next_port_pif_drive_req_rdy_thread   = 0;
  m_next_port_pif_respond_thread         = 0;
  m_next_port_axi_req_addr_thread        = 0;
  m_next_port_axi_req_data_thread        = 0;
  m_next_port_axi_drive_addr_rdy_thread  = 0;
  m_next_port_axi_drive_data_rdy_thread  = 0;
  m_next_port_axi_rd_rsp_thread          = 0;
  m_next_port_axi_wr_data_thread         = 0;
  m_next_port_axi_wr_rsp_thread          = 0;
  m_next_port_apb_request_thread         = 0;
  m_next_port_apb_drive_outputs_thread   = 0;

  for (u32 port=0; port<m_num_ports; ++port) {
    if (is_pif_or_idma(m_interface_type)) {
      m_block_write_xfers[port]         = 0;
      m_block_write_address[port]       = 0;
      m_block_write_offset[port]        = 0;
      m_burst_write_address[port]       = 0;
      m_first_block_write[port]         = true;
      m_first_burst_write[port]         = true;
      *m_rcw_compare_data[port]         = 0;
      m_last_action_time_stamp[port]    = sc_time_stamp() - sc_get_time_resolution();
      m_testing_busy[port]              = false;
      if (m_rsp_user_data_width1) {
        if (m_rsp_user_data_val_echo) {
          *m_rsp_user_data_val[port]    = 0;
        }
      }
    }
    else if (is_axi_or_idma(m_interface_type)) {
      m_testing_busy     [port]         = false;
      m_testing_busy_data[port]         = false;
    }
    else if (is_apb(m_interface_type)) {
      ; // Do nothing
    }
  }

  // Clear deque's
  for (u32 port=0; port<m_num_ports; ++port) {
    if (is_pif_or_idma(m_interface_type)) {
      while (!m_pif_req_fifo[port].empty()) {
        pif_req_info *p_info  = m_pif_req_fifo[port].front();
        m_pif_req_fifo[port].pop_front();
        delete_pif_req_info(p_info);
      }
    }
    else if (is_axi_or_idma(m_interface_type)) {
      while (!m_axi_addr_fifo[port].empty()) {
        axi_addr_info *p_info  = m_axi_addr_fifo[port].front();
        m_axi_addr_fifo[port].pop_front();
        delete_axi_addr_info(p_info);
      }
      while (!m_axi_data_fifo[port].empty()) {
        axi_data_info *p_info  = m_axi_data_fifo[port].front();
        m_axi_data_fifo[port].pop_front();
        delete_axi_data_info(p_info);
      }
      while (!m_axi_wr_rsp_fifo[port].empty()) {
        axi_addr_info *p_info  = m_axi_wr_rsp_fifo[port].front();
        m_axi_wr_rsp_fifo[port].pop_front();
        delete_axi_addr_info(p_info);
      }
    }
    else if (is_apb(m_interface_type)) {
      ; // Do nothing
    }
  }

  if (hard_reset) {
    m_p_memory->load_initial_values();
  }

  // Cancel any pending sc_event and sc_event_queue notifications:
  for (u32 port=0; port<m_num_ports; ++port) {
    if (m_pif_req_event)      m_pif_req_event     [port].cancel();
    if (m_pif_req_rdy_event)  m_pif_req_rdy_event [port].cancel();
    if (m_respond_event)      m_respond_event     [port].cancel();
    if (m_axi_rd_rsp_event)   m_axi_rd_rsp_event  [port].cancel();
    if (m_axi_wr_data_event)  m_axi_wr_data_event [port].cancel();
    if (m_axi_wr_rsp_event)   m_axi_wr_rsp_event  [port].cancel();
    if (m_axi_addr_rdy_event) m_axi_addr_rdy_event[port].cancel();
    if (m_axi_data_rdy_event) m_axi_data_rdy_event[port].cancel();
    if (m_apb_req_event)      m_apb_req_event     [port].cancel();

    if (m_read_event_queue)   m_read_event_queue  [port]->cancel_all();
    if (m_busy_event_queue && m_has_busy && m_busy_event_queue[port]) {
                              m_busy_event_queue  [port]->cancel_all();
    }
  }

  xtsc_reset_processes(m_process_handles);
}



void xtsc_component::xtsc_memory_pin::sync_to_sample_phase(void) {
  sc_time now = sc_time_stamp();
  sc_time phase_now = (now.value() % m_clock_period_value) * m_time_resolution;
  if (m_has_posedge_offset) {
    if (phase_now < m_posedge_offset) {
      phase_now += m_clock_period;
    }
    phase_now -= m_posedge_offset;
  }
  if (phase_now < m_sample_phase) {
    wait(m_sample_phase - phase_now);
  }
  else {
    wait(m_sample_phase_plus_one - phase_now);
  }
} 



void xtsc_component::xtsc_memory_pin::sync_to_drive_phase(void) {
  sc_time now = sc_time_stamp();
  sc_time phase_now = (now.value() % m_clock_period_value) * m_time_resolution;
  if (m_has_posedge_offset) {
    if (phase_now < m_posedge_offset) {
      phase_now += m_clock_period;
    }
    phase_now -= m_posedge_offset;
  }
  if (phase_now < m_drive_phase) {
    wait(m_drive_phase - phase_now);
  }
  else if (phase_now > m_drive_phase) {
    wait(m_drive_phase_plus_one - phase_now);
  }
} 



void xtsc_component::xtsc_memory_pin::lcl_request_thread(void) {

  // Get the port number for this "instance" of the thread
  u32 port = m_next_port_lcl_request_thread++;

  // For dual-ported case, get other port
  u32 other = ((port == 0) ? 1 : 0);

  bool busy = false;

  // Empty fifo's
  while (m_read_data_fifo[port]->nb_read(*m_data[port]));
  while (m_busy_fifo[port]->nb_read(busy));

  // A try/catch block in sc_main will not catch an exception thrown from
  // an SC_THREAD, so we'll catch them here, log them, then rethrow them.
  try {

    XTSC_INFO(m_text, "in " << __FUNCTION__ << "[" << port << "]");

    // If present, drive 0 on parity/ECC signal
    if (m_p_check[port]) {
      sc_bv_base dummy((int)m_check_bits);
      dummy = 0;
      m_p_check[port]->write(dummy);
    }

    // Loop forever
    while (true) {

      bool need_to_schedule_read_data_deassert = false;

      // Wait for a request: m_p_en[port]->pos()
      wait();

      XTSC_DEBUG(m_text, __FUNCTION__ << "[" << port << "]: awoke from wait()");

      // Sync to sample phase
      sync_to_sample_phase();

      XTSC_DEBUG(m_text, __FUNCTION__ << "[" << port << "]: completed sync_to_sample_phase()");

      busy = false;

      while (m_p_en[port]->read()) {
        xtsc_address addr8 = ((m_p_addr[port]->read().to_uint() & m_address_mask) << m_address_shift) + m_start_address8;
        if (m_banked) {
          addr8 += port * m_width8;
        }
        xtsc_byte_enables byte_enables = 0xFFFF;
        xtsc_byte_enables lane         = 0xFFFF;
        if (m_p_lane[port]) {
          lane = (xtsc_byte_enables) m_p_lane[port]->read().to_uint64();
          byte_enables = lane;
          if ((m_interface_type == IRAM0) || (m_interface_type == IRAM1) || (m_interface_type == IROM0)) {
            // Convert word enables (1 "word" is 32 bits) to byte enables (64-bit IRAM0|IRAM1|IROM0 only)
            xtsc_byte_enables be = 0x0000;
            if (byte_enables & 0x1) be |= 0x000F;
            if (byte_enables & 0x2) be |= 0x00F0;
            if (byte_enables & 0x4) be |= 0x0F00;
            if (byte_enables & 0x8) be |= 0xF000;
            byte_enables = be;
          }
          if (m_big_endian) {
            swizzle_byte_enables(byte_enables);
          }
        }
        if (m_has_busy && ((m_num_subbanks < 2) || ((port % m_num_subbanks) == 0)) && m_busy_percentage && ((rand() % 100) < m_busy_percentage)) {
          XTSC_INFO(m_text, "Busy 0x" << hex << addr8);
          m_busy_event_queue[port]->notify(m_sample_to_drive_busy_delay);
          m_busy_fifo[port]->write(true);
          busy = true;
        }
        else if (m_p_wr[port] && m_p_wr[port]->read()) {
          // Handle a write request
          if (need_to_schedule_read_data_deassert) {
            *m_data[port] = 0;
            m_read_event_queue[port]->notify(m_sample_to_drive_data_delay);
            m_read_data_fifo[port]->write(*m_data[port]);
            need_to_schedule_read_data_deassert = false;
          }
          *m_data_to_be_written[port] = m_p_wrdata[port]->read();
          // If we're dual-ported and the other port has a read this cycle (or if we have more then 2 ports),
          // then we delay handling of write requests by one delta cycle so that if there is also a read this
          // clock cycle to the same address it will return the original (old) data regardless of SystemC
          // thread scheduling indeterminancy.
          if (((m_num_ports == 2) && m_p_en[other]->read() && !m_p_wr[other]->read()) || (m_num_ports > 2)) {
            wait(SC_ZERO_TIME);
          }
          u32 delta      = (m_big_endian ? -8 : +8);
          u32 bit_offset = (m_big_endian ? (8 * (m_width8 - 1)) : 0);
          u32 mem_offset = m_p_memory->get_page_offset(addr8);
          u32 page       = m_p_memory->get_page(addr8);
          u64 bytes      = byte_enables;
          for (u32 i = 0; i<m_width8; ++i) {
            if (bytes & 0x1) {
              *(m_p_memory->m_page_table[page]+mem_offset) = (u8) m_data_to_be_written[port]->range(bit_offset+7, bit_offset).to_uint();
            }
            bytes >>= 1;
            mem_offset += 1;
            bit_offset += delta;
          }
          if (xtsc_is_text_logging_enabled() && m_text.isEnabledFor(log4xtensa::INFO_LOG_LEVEL)) {
            ostringstream oss;
            oss << hex << *m_data_to_be_written[port];
            XTSC_INFO(m_text, "Write  [0x" << hex << setfill('0') << setw(8) << addr8 << "/0x" << setw(m_enable_bits/4) << lane <<
                              "]: 0x" << oss.str().substr(1));
          }
        }
        else {
          // Handle a read request
          *m_data[port]  = 0;
          u32 delta      = (m_big_endian ? -8 : +8);
          u32 bit_offset = (m_big_endian ? (8 * (m_width8 - 1)) : 0);
          u32 mem_offset = m_p_memory->get_page_offset(addr8);
          u32 page       = m_p_memory->get_page(addr8);
          u64 bytes      = byte_enables;
          for (u32 i = 0; i<m_width8; ++i) {
            if (bytes & 0x1) {
              m_data[port]->range(bit_offset+7, bit_offset) = *(m_p_memory->m_page_table[page]+mem_offset);
            }
            bytes >>= 1;
            mem_offset += 1;
            bit_offset += delta;
          }
          m_read_event_queue[port]->notify(m_sample_to_drive_data_delay);
          m_read_data_fifo[port]->write(*m_data[port]);
          if (xtsc_is_text_logging_enabled() && m_text.isEnabledFor(log4xtensa::INFO_LOG_LEVEL)) {
            ostringstream oss;
            oss << hex << *m_data[port];
            XTSC_INFO(m_text, "Read   [0x" << hex << setfill('0') << setw(8) << addr8 << "/0x" << setw(m_enable_bits/4) << lane <<
                              "]: 0x" << oss.str().substr(1));
          }

          need_to_schedule_read_data_deassert = true;
        }

        // Wait one clock period
        wait(m_clock_period);

        // Schedule busy to be deasserted, if applicable
        if (busy) {
          m_busy_fifo[port]->write(false);
          m_busy_event_queue[port]->notify(m_sample_to_drive_busy_delay);
        }
      }

      if (need_to_schedule_read_data_deassert) {
        *m_data[port] = 0;
        m_read_event_queue[port]->notify(m_sample_to_drive_data_delay);
        m_read_data_fifo[port]->write(*m_data[port]);
        need_to_schedule_read_data_deassert = false;
      }

      XTSC_DEBUG(m_text, __FUNCTION__ << "[" << port << "]: enable/valid is low");

    }

  }
  catch (const sc_unwind_exception& error) { throw; }
  catch (const exception& error) {
    ostringstream oss;
    oss << "std::exception caught in " << __FUNCTION__ << "[" << port << "], an SC_THREAD of " << kind() << " '" << name() << "'." << endl;
    oss << "what(): " << error.what() << endl;
    oss << __FUNCTION__ << " now rethrowing:";
    xtsc_log_multiline(m_text, FATAL_LOG_LEVEL, oss.str(), 2);
    throw;
  }

}



void xtsc_component::xtsc_memory_pin::lcl_rd_request_thread(void) {

  // Get the port number for this "instance" of the thread
  u32 port = m_next_port_lcl_request_thread++;

  bool busy = false;

  // Empty fifo's
  while (m_read_data_fifo[port]->nb_read(*m_data[port]));
  while (m_busy_fifo[port]->nb_read(busy));

  // A try/catch block in sc_main will not catch an exception thrown from
  // an SC_THREAD, so we'll catch them here, log them, then rethrow them.
  try {

    XTSC_INFO(m_text, "in " << __FUNCTION__ << "[" << port << "]");

    // If present, drive 0 on parity/ECC signal
    if (m_p_check[port]) {
      sc_bv_base dummy((int)m_check_bits);
      dummy = 0;
      m_p_check[port]->write(dummy);
    }

    // Loop forever
    while (true) {

      bool need_to_schedule_read_data_deassert = false;

      // Wait for a request: m_p_en[port]->pos()
      wait();

      XTSC_DEBUG(m_text, __FUNCTION__ << "[" << port << "]: awoke from wait()");

      // Sync to sample phase
      sync_to_sample_phase();

      XTSC_DEBUG(m_text, __FUNCTION__ << "[" << port << "]: completed sync_to_sample_phase()");

      busy = false;

      while (m_p_en[port]->read()) {
        xtsc_address addr8 = ((m_p_addr[port]->read().to_uint() & m_address_mask) << m_address_shift) + m_start_address8;
        if (m_banked) {
          addr8 += port * m_width8;
        }
        xtsc_byte_enables byte_enables = 0xFFFF;
        xtsc_byte_enables lane         = 0xFFFF;
        if (m_p_lane[port]) {
          lane = (xtsc_byte_enables) m_p_lane[port]->read().to_uint64();
          byte_enables = lane;
          if ((m_interface_type == IRAM0) || (m_interface_type == IRAM1) || (m_interface_type == IROM0)) {
            // Convert word enables (1 "word" is 32 bits) to byte enables (64-bit IRAM0|IRAM1|IROM0 only)
            xtsc_byte_enables be = 0x0000;
            if (byte_enables & 0x1) be |= 0x000F;
            if (byte_enables & 0x2) be |= 0x00F0;
            if (byte_enables & 0x4) be |= 0x0F00;
            if (byte_enables & 0x8) be |= 0xF000;
            byte_enables = be;
          }
          if (m_big_endian) {
            swizzle_byte_enables(byte_enables);
          }
        }
        if (m_has_busy && m_busy_percentage && ((rand() % 100) < m_busy_percentage)) {
          XTSC_INFO(m_text, "Busy 0x" << hex << addr8);
          m_busy_event_queue[port]->notify(m_sample_to_drive_busy_delay);
          m_busy_fifo[port]->write(true);
          busy = true;
        }
        else {
          // Handle a read request
          *m_data[port]  = 0;
          u32 delta      = (m_big_endian ? -8 : +8);
          u32 bit_offset = (m_big_endian ? (8 * (m_width8 - 1)) : 0);
          u32 mem_offset = m_p_memory->get_page_offset(addr8);
          u32 page       = m_p_memory->get_page(addr8);
          u64 bytes      = byte_enables;
          for (u32 i = 0; i<m_width8; ++i) {
            if (bytes & 0x1) {
              m_data[port]->range(bit_offset+7, bit_offset) = *(m_p_memory->m_page_table[page]+mem_offset);
            }
            bytes >>= 1;
            mem_offset += 1;
            bit_offset += delta;
          }
          m_read_event_queue[port]->notify(m_sample_to_drive_data_delay);
          m_read_data_fifo[port]->write(*m_data[port]);
          if (xtsc_is_text_logging_enabled() && m_text.isEnabledFor(log4xtensa::INFO_LOG_LEVEL)) {
            ostringstream oss;
            oss << hex << *m_data[port];
            if (m_p_attr[port]) {
              ostringstream oss1;
              m_dram_attribute_bv = m_p_attr[port]->read();
              oss1 << hex << m_dram_attribute_bv;
              oss << " 0x" << oss1.str().substr(1);
            }
            XTSC_INFO(m_text, "Read   [0x" << hex << setfill('0') << setw(8) << addr8 << "/0x" << setw(m_enable_bits/4) << lane <<
                              "]: 0x" << oss.str().substr(1));
          }

          need_to_schedule_read_data_deassert = true;
        }

        // Wait one clock period
        wait(m_clock_period);

        // Schedule busy to be deasserted, if applicable
        if (busy) {
          m_busy_fifo[port]->write(false);
          m_busy_event_queue[port]->notify(m_sample_to_drive_busy_delay);
        }
      }

      if (need_to_schedule_read_data_deassert) {
        *m_data[port] = 0;
        m_read_event_queue[port]->notify(m_sample_to_drive_data_delay);
        m_read_data_fifo[port]->write(*m_data[port]);
        need_to_schedule_read_data_deassert = false;
      }

      XTSC_DEBUG(m_text, __FUNCTION__ << "[" << port << "]: enable/valid is low");

    }

  }
  catch (const sc_unwind_exception& error) { throw; }
  catch (const exception& error) {
    ostringstream oss;
    oss << "std::exception caught in " << __FUNCTION__ << "[" << port << "], an SC_THREAD of " << kind() << " '" << name() << "'." << endl;
    oss << "what(): " << error.what() << endl;
    oss << __FUNCTION__ << " now rethrowing:";
    xtsc_log_multiline(m_text, FATAL_LOG_LEVEL, oss.str(), 2);
    throw;
  }

}



void xtsc_component::xtsc_memory_pin::lcl_wr_request_thread(void) {

  // Get the port number for this "instance" of the thread
  u32 port = m_next_port_lcl_request_thread++;

  bool busy = false;

  // Empty fifo's
  while (m_busy_fifo[port]->nb_read(busy));

  // A try/catch block in sc_main will not catch an exception thrown from
  // an SC_THREAD, so we'll catch them here, log them, then rethrow them.
  try {

    XTSC_INFO(m_text, "in " << __FUNCTION__ << "[" << port << "]");

    // Loop forever
    while (true) {

      // Wait for a request: m_p_wr[port]->pos()
      wait();

      XTSC_DEBUG(m_text, __FUNCTION__ << "[" << port << "]: awoke from wait()");

      // Sync to sample phase
      sync_to_sample_phase();

      XTSC_DEBUG(m_text, __FUNCTION__ << "[" << port << "]: completed sync_to_sample_phase()");

      busy = false;

      while (m_p_wr[port]->read()) {
        xtsc_address addr8 = ((m_p_addr[port]->read().to_uint() & m_address_mask) << m_address_shift) + m_start_address8;
        if (m_banked) {
          addr8 += port * m_width8;
        }
        xtsc_byte_enables byte_enables = 0xFFFF;
        xtsc_byte_enables lane         = 0xFFFF;
        if (m_p_lane[port]) {
          lane = (xtsc_byte_enables) m_p_lane[port]->read().to_uint64();
          byte_enables = lane;
          if ((m_interface_type == IRAM0) || (m_interface_type == IRAM1) || (m_interface_type == IROM0)) {
            // Convert word enables (1 "word" is 32 bits) to byte enables (64-bit IRAM0|IRAM1|IROM0 only)
            xtsc_byte_enables be = 0x0000;
            if (byte_enables & 0x1) be |= 0x000F;
            if (byte_enables & 0x2) be |= 0x00F0;
            if (byte_enables & 0x4) be |= 0x0F00;
            if (byte_enables & 0x8) be |= 0xF000;
            byte_enables = be;
          }
          if (m_big_endian) {
            swizzle_byte_enables(byte_enables);
          }
        }
        if (m_has_busy && m_busy_percentage && ((rand() % 100) < m_busy_percentage)) {
          XTSC_INFO(m_text, "Busy 0x" << hex << addr8);
          m_busy_event_queue[port]->notify(m_sample_to_drive_busy_delay);
          m_busy_fifo[port]->write(true);
          busy = true;
        }
        else {
          // Handle a write request
          *m_data_to_be_written[port] = m_p_wrdata[port]->read();
          // We delay handling of write requests by one delta cycle so that if there is also a read this
          // clock cycle to the same address it will return the original (old) data regardless of SystemC
          // thread scheduling indeterminancy.
          wait(SC_ZERO_TIME);
          u32 delta      = (m_big_endian ? -8 : +8);
          u32 bit_offset = (m_big_endian ? (8 * (m_width8 - 1)) : 0);
          u32 mem_offset = m_p_memory->get_page_offset(addr8);
          u32 page       = m_p_memory->get_page(addr8);
          u64 bytes      = byte_enables;
          for (u32 i = 0; i<m_width8; ++i) {
            if (bytes & 0x1) {
              *(m_p_memory->m_page_table[page]+mem_offset) = (u8) m_data_to_be_written[port]->range(bit_offset+7, bit_offset).to_uint();
            }
            bytes >>= 1;
            mem_offset += 1;
            bit_offset += delta;
          }
          if (xtsc_is_text_logging_enabled() && m_text.isEnabledFor(log4xtensa::INFO_LOG_LEVEL)) {
            ostringstream oss;
            oss << hex << *m_data_to_be_written[port];
            if (m_p_attr[port]) {
              ostringstream oss1;
              m_dram_attribute_bv = m_p_attr[port]->read();
              oss1 << hex << m_dram_attribute_bv;
              oss << " 0x" << oss1.str().substr(1);
            }
            XTSC_INFO(m_text, "Write  [0x" << hex << setfill('0') << setw(8) << addr8 << "/0x" << setw(m_enable_bits/4) << lane <<
                              "]: 0x" << oss.str().substr(1));
          }
        }

        // Wait one clock period
        wait(m_clock_period);

        // Schedule busy to be deasserted, if applicable
        if (busy) {
          m_busy_fifo[port]->write(false);
          m_busy_event_queue[port]->notify(m_sample_to_drive_busy_delay);
        }
      }
      XTSC_DEBUG(m_text, __FUNCTION__ << "[" << port << "]: enable/valid is low");
    }

  }
  catch (const sc_unwind_exception& error) { throw; }
  catch (const exception& error) {
    ostringstream oss;
    oss << "std::exception caught in " << __FUNCTION__ << "[" << port << "], an SC_THREAD of " << kind() << " '" << name() << "'." << endl;
    oss << "what(): " << error.what() << endl;
    oss << __FUNCTION__ << " now rethrowing:";
    xtsc_log_multiline(m_text, FATAL_LOG_LEVEL, oss.str(), 2);
    throw;
  }

}



void xtsc_component::xtsc_memory_pin::lcl_drive_read_data_thread(void) {

  // Get the port number for this "instance" of the thread
  u32 port = m_next_port_lcl_drive_read_data_thread++;
  if (m_split_rw) {
    // Only the even port numbers are Rd ports when using a split Rd/Wr interface
    m_next_port_lcl_drive_read_data_thread += 1;
  }

  try {

    XTSC_INFO(m_text, "in " << __FUNCTION__ << "[" << port << "]");

    // Handle re-entry after reset
    if (sc_time_stamp() != SC_ZERO_TIME) {
      wait(SC_ZERO_TIME);  // Allow one delta cycle to ensure we can completely empty an sc_fifo
      while (m_read_data_fifo[port]->nb_read(*m_data[port]));
    }

    // Loop forever
    while (true) {

      // Wait to be notified: m_read_event_queue
      wait();

      if (!m_read_data_fifo[port]->nb_read(*m_data[port])) {
        ostringstream oss;
        oss << "Program bug in xtsc_memory_pin::lcl_drive_read_data_thread: m_read_data_fifo[" << port << "] is empty";
        throw xtsc_exception(oss.str());
      }

      m_p_data[port]->write(*m_data[port]);

      XTSC_VERBOSE(m_text, "lcl_drive_read_data_thread() driving read data 0x" << hex << *m_data[port]);

    }

  }
  catch (const sc_unwind_exception& error) { throw; }
  catch (const exception& error) {
    ostringstream oss;
    oss << "std::exception caught in " << __FUNCTION__ << "[" << port << "], an SC_THREAD of " << kind() << " '" << name() << "'." << endl;
    oss << "what(): " << error.what() << endl;
    oss << __FUNCTION__ << " now rethrowing:";
    xtsc_log_multiline(m_text, FATAL_LOG_LEVEL, oss.str(), 2);
    throw;
  }

}



void xtsc_component::xtsc_memory_pin::lcl_drive_busy_thread(void) {

  // Get the port number for this "instance" of the thread
  u32 port = m_next_port_lcl_drive_busy_thread;
  m_next_port_lcl_drive_busy_thread += ((m_num_subbanks < 2) ? 1 : m_num_subbanks);

  bool busy = false;

  // Empty fifo's
  while (m_busy_fifo[port]->nb_read(busy));

  m_p_busy[port]->write(false);

  try {

    XTSC_INFO(m_text, "in " << __FUNCTION__ << "[" << port << "]");

    // Handle re-entry after reset
    if (sc_time_stamp() != SC_ZERO_TIME) {
      wait(SC_ZERO_TIME);  // Allow one delta cycle to ensure we can completely empty an sc_fifo
      while (m_busy_fifo[port]->nb_read(busy));
    }

    // Loop forever
    while (true) {

      // Wait to be notified: m_busy_event_queue
      wait();

      if (!m_busy_fifo[port]->nb_read(busy)) {
        ostringstream oss;
        oss << "Program bug in xtsc_memory_pin::lcl_drive_busy_thread: m_busy_fifo[" << port << "] is empty";
        throw xtsc_exception(oss.str());
      }

      m_p_busy[port]->write(busy);

      XTSC_VERBOSE(m_text, "lcl_drive_busy_thread() driving busy=" << busy);

    }

  }
  catch (const sc_unwind_exception& error) { throw; }
  catch (const exception& error) {
    ostringstream oss;
    oss << "std::exception caught in " << __FUNCTION__ << "[" << port << "], an SC_THREAD of " << kind() << " '" << name() << "'." << endl;
    oss << "what(): " << error.what() << endl;
    oss << __FUNCTION__ << " now rethrowing:";
    xtsc_log_multiline(m_text, FATAL_LOG_LEVEL, oss.str(), 2);
    throw;
  }

}



void xtsc_component::xtsc_memory_pin::pif_request_thread(void) {

  // Get the port number for this "instance" of the thread
  u32 port = m_next_port_pif_request_thread++;

  try {

    XTSC_INFO(m_text, "in " << __FUNCTION__ << "[" << port << "]");

    // Loop forever
    while (true) {

      // Wait for posedge POReqValid or PIReqRdy
      XTSC_DEBUG(m_text, __FUNCTION__ << "[" << port << "]: waiting for posedge POReqValid/PIReqRdy");
      wait();
      XTSC_DEBUG(m_text, __FUNCTION__ << "[" << port << "]: got posedge POReqValid/PIReqRdy");

      // Sync to sample phase
      sync_to_sample_phase();
      XTSC_DEBUG(m_text, __FUNCTION__ << "[" << port << "]: completed sync_to_sample_phase()");

      // While valid is asserted
      while (m_p_req_valid[port]->read()) {
        // Capture one request per clock period while both POReqValid and PIReqRdy are asserted
        if (m_p_req_rdy[port]->read()) {
          pif_req_info *p_info = new_pif_req_info(port);
          m_pif_req_fifo[port].push_back(p_info);
          m_pif_req_event[port].notify(SC_ZERO_TIME);
          m_pif_req_rdy_event[port].notify(SC_ZERO_TIME);
          if (m_req_user_data_width1 && m_rsp_user_data_width1 && m_rsp_user_data_val_echo) {
            *m_rsp_user_data_val[port] = m_p_req_user_data[port]->read();
          }
          XTSC_VERBOSE(m_text, *p_info << " (pif_request_thread)");
        }
        else {
          XTSC_INFO(m_text, " Busy [" << port << "] 0x" << hex << setfill('0') << setw(8) << m_p_req_adrs[port]->read() <<
                            (m_testing_busy[port] ? " (test)" : ""));
          m_pif_req_rdy_event[port].notify(SC_ZERO_TIME);
          m_testing_busy[port] = false;
        }
        wait(m_clock_period);
      }

    }

  }
  catch (const sc_unwind_exception& error) { throw; }
  catch (const exception& error) {
    ostringstream oss;
    oss << "std::exception caught in " << __FUNCTION__ << "[" << port << "], an SC_THREAD of " << kind() << " '" << name() << "'." << endl;
    oss << "what(): " << error.what() << endl;
    oss << __FUNCTION__ << " now rethrowing:";
    xtsc_log_multiline(m_text, FATAL_LOG_LEVEL, oss.str(), 2);
    throw;
  }

}



void xtsc_component::xtsc_memory_pin::pif_drive_req_rdy_thread(void) {

  // Get the port number for this "instance" of the thread
  u32 port = m_next_port_pif_drive_req_rdy_thread++;

  try {

    XTSC_INFO(m_text, "in " << __FUNCTION__ << "[" << port << "]");

    m_p_req_rdy[port]->write(true);

    // Loop forever
    while (true) {

      XTSC_DEBUG(m_text, __FUNCTION__ << "[" << port << "]: calling wait()");
      wait(m_pif_req_rdy_event[port]);
      XTSC_DEBUG(m_text, __FUNCTION__ << "[" << port << "]: awoke from wait()");
      
      // Sync to m_drive_phase
      sync_to_drive_phase();
      XTSC_DEBUG(m_text, __FUNCTION__ << "[" << port << "]: returned from sync_to_drive_phase()");

      if (m_pif_req_fifo[port].size() >= m_request_fifo_depth) {
        m_p_req_rdy[port]->write(false);
        m_testing_busy[port] = false;
      }
      else if (m_busy_percentage && (m_testing_busy[port] || ((rand() % 100) < m_busy_percentage))) {
        m_p_req_rdy[port]->write(false);
        m_testing_busy[port] = true;
      }
      else {
        m_p_req_rdy[port]->write(true);
      }

    }

  }
  catch (const sc_unwind_exception& error) { throw; }
  catch (const exception& error) {
    ostringstream oss;
    oss << "std::exception caught in " << __FUNCTION__ << "[" << port << "], an SC_THREAD of " << kind() << " '" << name() << "'." << endl;
    oss << "what(): " << error.what() << endl;
    oss << __FUNCTION__ << " now rethrowing:";
    xtsc_log_multiline(m_text, FATAL_LOG_LEVEL, oss.str(), 2);
    throw;
  }

}



void xtsc_component::xtsc_memory_pin::pif_respond_thread(void) {

  // Get the port number for this "instance" of the thread
  u32 port = m_next_port_pif_respond_thread++;

  try {

    XTSC_INFO(m_text, "in " << __FUNCTION__ << "[" << port << "]");

    if (m_p_resp_coh_cntl[port]) {
      m_p_resp_coh_cntl [port]->write(m_coh_cntl);
    }

    // Loop forever
    while (true) {

      // Wait for a request
      wait(m_pif_req_event[port]);
      XTSC_DEBUG(m_text, __FUNCTION__ << "[" << port << "]: awoke from wait(m_pif_req_event)");

      // Sync to m_drive_phase
      sync_to_drive_phase();
      XTSC_DEBUG(m_text, __FUNCTION__ << "[" << port << "]: returned from sync_to_drive_phase()");

      // Process up to one request per clock period while there are requests available.
      while (!m_pif_req_fifo[port].empty()) {
        pif_req_info *p_info = m_pif_req_fifo[port].front();
        m_pif_req_fifo[port].pop_front();
        m_pif_req_rdy_event[port].notify(SC_ZERO_TIME);
        m_respond_event[port].notify(SC_ZERO_TIME);

        // Determine delay
        sc_time delay = SC_ZERO_TIME;
        switch (p_info->m_req_cntl.get_type()) {
          case req_cntl::READ: {
            delay  = m_read_delay;
            break;
          }
          case req_cntl::BLOCK_READ: {
            delay  = m_block_read_delay;
            break;
          }
          case req_cntl::BURST_READ: {
            delay  = m_burst_read_delay;
            break;
          }
          case req_cntl::RCW: {
            delay  = (p_info->m_req_cntl.get_last_transfer() ?  m_rcw_response : m_rcw_repeat);
            break;
          }
          case req_cntl::WRITE: {
            delay  = m_write_delay;
            break;
          }
          case req_cntl::BLOCK_WRITE:  {
            bool last = p_info->m_req_cntl.get_last_transfer();
            delay  = (m_first_block_write[port] ?  m_block_write_delay : (last ? m_block_write_response : m_block_write_repeat));
            break;
          }
          case req_cntl::BURST_WRITE:  {
            bool last = p_info->m_req_cntl.get_last_transfer();
            delay  = (m_first_burst_write[port] ?  m_burst_write_delay : (last ? m_burst_write_response : m_burst_write_repeat));
            break;
          }
          default: {
            // Unrecognized request type: If last transfer, then respond with AErr, else drop on floor
            if (p_info->m_req_cntl.get_last_transfer()) {
              XTSC_INFO(m_text, *p_info << " (Rsp=AErr)");
              m_resp_cntl.init(resp_cntl::AErr, true);
              pif_drive_response(*p_info, m_resp_cntl, *m_data[port]);
            }
            else {
              XTSC_INFO(m_text, *p_info << " (Dropping on floor)");
              wait(m_clock_period);
            }
            continue;
          }
        }
        if (delay != SC_ZERO_TIME) {
          XTSC_DEBUG(m_text, "pif_respond_thread() doing wait for " << delay);
          wait(delay);
        }

        // Initialize some common values
        m_resp_cntl.init(resp_cntl::OK, true);

        switch (p_info->m_req_cntl.get_type()) {
          case req_cntl::READ: {
            do_read(*p_info);
            break;
          }
          case req_cntl::BLOCK_READ: {
            do_block_read(*p_info);
            break;
          }
          case req_cntl::BURST_READ: {
            do_burst_read(*p_info);
            break;
          }
          case req_cntl::RCW: {
            do_rcw(*p_info);
            break;
          }
          case req_cntl::WRITE: {
            do_write(*p_info);
            break;
          }
          case req_cntl::BLOCK_WRITE: {
            do_block_write(*p_info);
            break;
          }
          case req_cntl::BURST_WRITE: {
            do_burst_write(*p_info);
            break;
          }
          default: {
            ostringstream oss;
            oss << kind() << " '" << name() << "': ";
            oss << "Unrecognized request type = 0x" << hex << p_info->m_req_cntl.get_type();
            throw xtsc_exception(oss.str());
          }
        }

        delete_pif_req_info(p_info);

      }

    }

  }
  catch (const sc_unwind_exception& error) { throw; }
  catch (const exception& error) {
    ostringstream oss;
    oss << "std::exception caught in " << __FUNCTION__ << "[" << port << "], an SC_THREAD of " << kind() << " '" << name() << "'." << endl;
    oss << "what(): " << error.what() << endl;
    oss << __FUNCTION__ << " now rethrowing:";
    xtsc_log_multiline(m_text, FATAL_LOG_LEVEL, oss.str(), 2);
    throw;
  }

}



void xtsc_component::xtsc_memory_pin::axi_req_addr_thread(void) {

  // Get the port number and signal indx for this "instance" of the thread
  u32 port = m_next_port_axi_req_addr_thread++;
  u32 indx = m_axi_signal_indx[port];

  try {

    bool read_channel = (m_axi_read_write[port] == 0);

    XTSC_INFO(m_text, "in " << __FUNCTION__ << "[" << port << "]. Using " << (read_channel ? "read" : "write") << " signal indx=" << indx);

    string wait_signals(read_channel ?  "ARVALID|ARREADY" : "AWVALID|AWREADY");

    bool_input  *AxVALID  = (read_channel ? m_p_arvalid  : m_p_awvalid )[indx];
    bool_output *AxREADY  = (read_channel ? m_p_arready  : m_p_awready )[indx];
    uint_input  *AxADDR   = (read_channel ? m_p_araddr   : m_p_awaddr  )[indx];

    // Loop forever
    while (true) {

      // Wait for posedge on wait_signals
      XTSC_DEBUG(m_text, __FUNCTION__ << "[" << port << "]: waiting for posedge " << wait_signals);
      wait();
      XTSC_DEBUG(m_text, __FUNCTION__ << "[" << port << "]: got posedge " << wait_signals);

      // Sync to sample phase
      sync_to_sample_phase();
      XTSC_DEBUG(m_text, __FUNCTION__ << "[" << port << "]: completed sync_to_sample_phase()");

      // While valid is asserted
      while (AxVALID->read()) {
        // Capture one request per clock period while both AxVALID and AxREADY are asserted
        if (AxREADY->read()) {
          axi_addr_info *p_info = new_axi_addr_info(port);
          m_axi_addr_fifo[port].push_back(p_info);
          if (read_channel) {
            m_axi_rd_rsp_event[port].notify(SC_ZERO_TIME); // Notify axi_rd_rsp_thread()
          }
          else {
            ;  // Do nothing:
            //    m_axi_wr_data_event is notified from axi_req_data_thread()
            //    m_axi_wr_rsp_event  is notified from axi_wr_data_thread()
          }
          m_axi_addr_rdy_event[port].notify(SC_ZERO_TIME);      // Notify axi_drive_addr_rdy_thread()
          XTSC_VERBOSE(m_text, *p_info << " (axi_req_addr_thread[" << port << "])");
        }
        else {
          XTSC_INFO(m_text, "Busy AxREADY of Port #" << port << " 0x" << hex << setfill('0') << setw(8) << AxADDR->read() <<
                            (m_testing_busy[port] ? " (test)" : ""));
          m_axi_addr_rdy_event[port].notify(SC_ZERO_TIME);      // Notify axi_drive_addr_rdy_thread()
          m_testing_busy[port] = false;
        }
        wait(m_clock_period);
      }

    }

  }
  catch (const sc_unwind_exception& error) { throw; }
  catch (const exception& error) {
    ostringstream oss;
    oss << "std::exception caught in " << __FUNCTION__ << "[" << port << "], an SC_THREAD of " << kind() << " '" << name() << "'." << endl;
    oss << "what(): " << error.what() << endl;
    oss << __FUNCTION__ << " now rethrowing:";
    xtsc_log_multiline(m_text, FATAL_LOG_LEVEL, oss.str(), 2);
    throw;
  }

}



void xtsc_component::xtsc_memory_pin::axi_req_data_thread(void) {

  // Get the port number and signal indx for this "instance" of the thread
  u32 port = m_next_port_axi_req_data_thread++;
  u32 indx = m_axi_signal_indx[port];

  try {

    bool read_channel = (m_axi_read_write[port] == 0);
    if (read_channel) return;

    XTSC_INFO(m_text, "in " << __FUNCTION__ << "[" << port << "]. Using write signal indx=" << indx);

    string wait_signals("WVALID|WREADY");

    bool_input  *WVALID  = m_p_wvalid[indx];
    bool_output *WREADY  = m_p_wready[indx];

    // Loop forever
    while (true) {

      // Wait for posedge on wait_signals
      XTSC_DEBUG(m_text, __FUNCTION__ << "[" << port << "]: waiting for posedge " << wait_signals);
      wait();
      XTSC_DEBUG(m_text, __FUNCTION__ << "[" << port << "]: got posedge " << wait_signals);

      // Sync to sample phase
      sync_to_sample_phase();
      XTSC_DEBUG(m_text, __FUNCTION__ << "[" << port << "]: completed sync_to_sample_phase()");

      // While valid is asserted
      while (WVALID->read()) {
        // Capture one request per clock period while both WVALID and WREADY are asserted
        if (WREADY->read()) {
          axi_data_info *p_info = new_axi_data_info(port);
          m_axi_data_fifo[port].push_back(p_info);
          m_axi_wr_data_event[port].notify(SC_ZERO_TIME);       // See axi_wr_data_thread()
          m_axi_data_rdy_event[port].notify(SC_ZERO_TIME);      // See axi_drive_data_rdy_thread()
          XTSC_VERBOSE(m_text, *p_info << " (axi_req_data_thread[" << port << "])");
        }
        else {
          XTSC_INFO(m_text, "Busy  WREADY of Port #" << port << (m_testing_busy_data[port] ? " (test)" : ""));
          m_axi_data_rdy_event[port].notify(SC_ZERO_TIME);      // See axi_drive_data_rdy_thread()
          m_testing_busy_data[port] = false;
        }
        wait(m_clock_period);
      }

    }

  }
  catch (const sc_unwind_exception& error) { throw; }
  catch (const exception& error) {
    ostringstream oss;
    oss << "std::exception caught in " << __FUNCTION__ << "[" << port << "], an SC_THREAD of " << kind() << " '" << name() << "'." << endl;
    oss << "what(): " << error.what() << endl;
    oss << __FUNCTION__ << " now rethrowing:";
    xtsc_log_multiline(m_text, FATAL_LOG_LEVEL, oss.str(), 2);
    throw;
  }

}



void xtsc_component::xtsc_memory_pin::axi_drive_addr_rdy_thread(void) {

  // Get the port number and signal indx for this "instance" of the thread
  u32 port = m_next_port_axi_drive_addr_rdy_thread++;
  u32 indx = m_axi_signal_indx[port];

  try {

    bool read_channel = (m_axi_read_write[port] == 0);

    XTSC_INFO(m_text, "in " << __FUNCTION__ << "[" << port << "]. Using " << (read_channel ? "read" : "write") << " signal indx=" << indx);

    bool_output *AxREADY = (read_channel ? m_p_arready : m_p_awready )[indx];

    AxREADY->write(true);

    // Loop forever
    while (true) {

      XTSC_DEBUG(m_text, __FUNCTION__ << "[" << port << "]: calling wait()");
      wait(m_axi_addr_rdy_event[port]);   // See axi_req_addr_thread(), axi_rd_rsp_thread()
      XTSC_DEBUG(m_text, __FUNCTION__ << "[" << port << "]: awoke from wait()");
      
      // Sync to m_drive_phase
      sync_to_drive_phase();
      XTSC_DEBUG(m_text, __FUNCTION__ << "[" << port << "]: returned from sync_to_drive_phase()");

      if (m_axi_addr_fifo[port].size() >= m_request_fifo_depth) {
        AxREADY->write(false);
        m_testing_busy[port] = false;
      }
      else if (m_busy_percentage && (m_testing_busy[port] || ((rand() % 100) < m_busy_percentage))) {
        AxREADY->write(false);
        m_testing_busy[port] = true;
      }
      else {
        AxREADY->write(true);
      }

    }

  }
  catch (const sc_unwind_exception& error) { throw; }
  catch (const exception& error) {
    ostringstream oss;
    oss << "std::exception caught in " << __FUNCTION__ << "[" << port << "], an SC_THREAD of " << kind() << " '" << name() << "'." << endl;
    oss << "what(): " << error.what() << endl;
    oss << __FUNCTION__ << " now rethrowing:";
    xtsc_log_multiline(m_text, FATAL_LOG_LEVEL, oss.str(), 2);
    throw;
  }

}



void xtsc_component::xtsc_memory_pin::axi_drive_data_rdy_thread(void) {

  // Get the port number and signal indx for this "instance" of the thread
  u32 port = m_next_port_axi_drive_data_rdy_thread++;
  u32 indx = m_axi_signal_indx[port];

  try {

    bool read_channel = (m_axi_read_write[port] == 0);
    if (read_channel) return;

    XTSC_INFO(m_text, "in " << __FUNCTION__ << "[" << port << "]. Using write signal indx=" << indx);

    bool_output *WREADY  = m_p_wready[indx];

    WREADY->write(true);

    // Loop forever
    while (true) {

      XTSC_DEBUG(m_text, __FUNCTION__ << "[" << port << "]: calling wait()");
      wait(m_axi_data_rdy_event[port]);         // See axi_req_data_thread()
      XTSC_DEBUG(m_text, __FUNCTION__ << "[" << port << "]: awoke from wait()");
      
      // Sync to m_drive_phase
      sync_to_drive_phase();
      XTSC_DEBUG(m_text, __FUNCTION__ << "[" << port << "]: returned from sync_to_drive_phase()");

      if (m_axi_data_fifo[port].size() >= m_request_fifo_depth) {
        WREADY->write(false);
        m_testing_busy_data[port] = false;
      }
      else if (m_data_busy_percentage && (m_testing_busy_data[port] || ((rand() % 100) < m_data_busy_percentage))) {
        WREADY->write(false);
        m_testing_busy_data[port] = true;
      }
      else {
        WREADY->write(true);
      }

    }

  }
  catch (const sc_unwind_exception& error) { throw; }
  catch (const exception& error) {
    ostringstream oss;
    oss << "std::exception caught in " << __FUNCTION__ << "[" << port << "], an SC_THREAD of " << kind() << " '" << name() << "'." << endl;
    oss << "what(): " << error.what() << endl;
    oss << __FUNCTION__ << " now rethrowing:";
    xtsc_log_multiline(m_text, FATAL_LOG_LEVEL, oss.str(), 2);
    throw;
  }

}



void xtsc_component::xtsc_memory_pin::axi_rd_rsp_thread(void) {

  // Get the port number and signal indx for this "instance" of the thread
  u32 port = m_next_port_axi_rd_rsp_thread++;
  u32 indx = m_axi_signal_indx[port];

  try {

    bool read_channel = (m_axi_read_write[port] == 0);
    if (!read_channel) return;

    XTSC_INFO(m_text, "in " << __FUNCTION__ << "[" << port << "]. Using read signal indx=" << indx);

    sc_time delay = m_burst_read_delay;

    // Loop forever
    while (true) {

      // Wait for a request
      wait(m_axi_rd_rsp_event[port]); //  See axi_req_addr_thread()
      XTSC_DEBUG(m_text, __FUNCTION__ << "[" << port << "]: awoke from wait(m_axi_rd_rsp_event)");

      // Sync to m_drive_phase
      sync_to_drive_phase();
      XTSC_DEBUG(m_text, __FUNCTION__ << "[" << port << "]: returned from sync_to_drive_phase()");

      // Process up to one read request per clock period while there are requests available.
      while (!m_axi_addr_fifo[port].empty()) {
        axi_addr_info *p_info = m_axi_addr_fifo[port].front();
        m_axi_addr_fifo[port].pop_front();
        m_axi_addr_rdy_event[port].notify(SC_ZERO_TIME);  // Notify axi_drive_addr_rdy_thread()
        m_respond_event[port].notify(SC_ZERO_TIME);       // For XTSC command facility
        if (delay != SC_ZERO_TIME) {
          XTSC_DEBUG(m_text, "axi_rd_rsp_thread() doing wait for " << delay);
          wait(delay);
        }
        do_burst_read(*p_info);
        delete_axi_addr_info(p_info);
      }

    }

  }
  catch (const sc_unwind_exception& error) { throw; }
  catch (const exception& error) {
    ostringstream oss;
    oss << "std::exception caught in " << __FUNCTION__ << "[" << port << "], an SC_THREAD of " << kind() << " '" << name() << "'." << endl;
    oss << "what(): " << error.what() << endl;
    oss << __FUNCTION__ << " now rethrowing:";
    xtsc_log_multiline(m_text, FATAL_LOG_LEVEL, oss.str(), 2);
    throw;
  }

}



void xtsc_component::xtsc_memory_pin::axi_wr_data_thread(void) {

  // Get the port number for this "instance" of the thread
  u32 port = m_next_port_axi_wr_data_thread++;

  try {

    bool read_channel = (m_axi_read_write[port] == 0);
    if (read_channel) return;

    XTSC_INFO(m_text, "in " << __FUNCTION__ << "[" << port << "].");

    sc_time        delay        = m_clock_period;
    axi_addr_info *p_addr_info  = NULL;
    u32            transfer_num = 0;

    // Loop forever
    while (true) {

      // Wait for a request
      wait(m_axi_wr_data_event[port]);  // See axi_req_data_thread()
      XTSC_DEBUG(m_text, __FUNCTION__ << "[" << port << "]: awoke from wait(m_axi_wr_data_event)");

      // Sync to m_drive_phase
      sync_to_drive_phase();
      XTSC_DEBUG(m_text, __FUNCTION__ << "[" << port << "]: returned from sync_to_drive_phase()");

      // Process up to one write data beat per clock period while there are write data beats available.
      while (!m_axi_data_fifo[port].empty()) {
        axi_data_info *p_data_info = m_axi_data_fifo[port].front();
        if (!p_addr_info) {
          bool first_time = true;
          while (m_axi_addr_fifo[port].empty()) {
            if (first_time) {
              XTSC_INFO(m_text, "Port #" << port << " received transfer on the Write Data channel (" << *p_data_info <<
                                "), now waiting for Write Address channel.");
              first_time = false;
            }
            wait(m_clock_period);
          }
          if (!first_time) {
            XTSC_INFO(m_text, "Port #" << port << " done waiting for Write Address channel.");
          }
          p_addr_info = m_axi_addr_fifo[port].front();
          m_axi_addr_fifo[port].pop_front();
          XTSC_DEBUG(m_text, __FUNCTION__ << "[" << port << "]: Use  p_addr_info: " << *p_addr_info);
          m_axi_addr_rdy_event[port].notify(SC_ZERO_TIME);  // Notify axi_drive_addr_rdy_thread()
        }
        delay  = ((transfer_num == 0) ?  m_burst_write_delay : (p_data_info->m_WLAST ? m_burst_write_response : m_burst_write_repeat));
        if (delay != SC_ZERO_TIME) {
          XTSC_DEBUG(m_text, "axi_wr_data_thread() doing wait for " << delay);
          wait(delay);
        }
        m_axi_data_fifo[port].pop_front();
        do_burst_write(*p_addr_info, *p_data_info, transfer_num);
        transfer_num += 1;
        // TODO: WRESP/resp of other than OKAY
        u32 num_transfers = p_addr_info->m_AxLEN + 1;
        if (p_data_info->m_WLAST) {
          if (transfer_num != num_transfers) {
            ostringstream oss;
            oss << kind() << " '" << name() << "': ";
            oss << "Received transfer on write data channel with WLAST set " << "high but only " << transfer_num
                << " transfer(s) out of an expected total of " << num_transfers << " have occurred.";
            throw xtsc_exception(oss.str());
          }
          m_axi_wr_rsp_fifo[port].push_back(p_addr_info);       // axi_wr_rsp_thread()
          m_axi_wr_rsp_event[port].notify(SC_ZERO_TIME);        // axi_wr_rsp_thread()
          XTSC_DEBUG(m_text, __FUNCTION__ << "[" << port << "]: Free p_addr_info: " << *p_addr_info);
          p_addr_info  = NULL;
          transfer_num = 0;
        }
        else {
          if (transfer_num >= num_transfers) {
            ostringstream oss;
            oss << kind() << " '" << name() << "': ";
            oss << "Received transfer on write data channel with WLAST set " << "low but " << transfer_num
                << " transfer(s) out of an expected total of " << num_transfers << " have occurred.";
            throw xtsc_exception(oss.str());
          }
        }
        delete_axi_data_info(p_data_info);
      }

    }

  }
  catch (const sc_unwind_exception& error) { throw; }
  catch (const exception& error) {
    ostringstream oss;
    oss << "std::exception caught in " << __FUNCTION__ << "[" << port << "], an SC_THREAD of " << kind() << " '" << name() << "'." << endl;
    oss << "what(): " << error.what() << endl;
    oss << __FUNCTION__ << " now rethrowing:";
    xtsc_log_multiline(m_text, FATAL_LOG_LEVEL, oss.str(), 2);
    throw;
  }

}



void xtsc_component::xtsc_memory_pin::axi_wr_rsp_thread(void) {

  // Get the port number for this "instance" of the thread
  u32 port = m_next_port_axi_wr_rsp_thread++;

  try {

    bool read_channel = (m_axi_read_write[port] == 0);
    if (read_channel) return;

    XTSC_INFO(m_text, "in " << __FUNCTION__ << "[" << port << "].");

    // Loop forever
    while (true) {

      // Wait for a request
      wait(m_axi_wr_rsp_event[port]);   // See axi_wr_data_thread()
      XTSC_DEBUG(m_text, __FUNCTION__ << "[" << port << "]: awoke from wait(m_axi_wr_rsp_event)");

      // Sync to m_drive_phase
      sync_to_drive_phase();
      XTSC_DEBUG(m_text, __FUNCTION__ << "[" << port << "]: returned from sync_to_drive_phase()");

      // Process up to one write responses per clock period while there are write responses available.
      while (!m_axi_wr_rsp_fifo[port].empty()) {
        axi_addr_info *p_addr_info = m_axi_wr_rsp_fifo[port].front();
        m_axi_wr_rsp_fifo[port].pop_front();
        axi_drive_wr_rsp(*p_addr_info, 0);   // TODO: RESP other than 0=OKAY
        delete_axi_addr_info(p_addr_info);
      }

    }

  }
  catch (const sc_unwind_exception& error) { throw; }
  catch (const exception& error) {
    ostringstream oss;
    oss << "std::exception caught in " << __FUNCTION__ << "[" << port << "], an SC_THREAD of " << kind() << " '" << name() << "'." << endl;
    oss << "what(): " << error.what() << endl;
    oss << __FUNCTION__ << " now rethrowing:";
    xtsc_log_multiline(m_text, FATAL_LOG_LEVEL, oss.str(), 2);
    throw;
  }

}



void xtsc_component::xtsc_memory_pin::apb_request_thread(void) {

  // Get the port number for this "instance" of the thread
  u32 port = m_next_port_apb_request_thread++;

  try {

    XTSC_INFO(m_text, "in " << __FUNCTION__ << "[" << port << "]");

    // Loop forever
    while (true) {

      // Wait for a request: posedge m_p_psel (PSELx)
      wait();
      XTSC_DEBUG(m_text, __FUNCTION__ << "[" << port << "]: awoke from wait()");

      // Sync to sample_phase
      sync_to_sample_phase();
      XTSC_DEBUG(m_text, __FUNCTION__ << "[" << port << "]: completed sync_to_sample_phase()");

      if (m_p_penable[port]->read()) {
        throw xtsc_exception("PENABLE is high when APB protocol requires it to be low");
      }

      // Process up to one request every other clock period while PSELx remains asserted
      while (m_p_psel[port]->read()) {
        sc_time delay = m_drive_phase + (m_p_pwrite[port]->read() ? m_write_delay : m_read_delay);
        m_apb_req_event[port].notify(delay);
        do {
          wait(m_clock_period);
          if (!m_p_penable[port]->read()) {
            throw xtsc_exception("PENABLE is low when APB protocol requires it to be high");
          }
        } while (!m_p_pready[port]->read());
        wait(m_clock_period);
      }

    }

  }
  catch (const sc_unwind_exception& error) { throw; }
  catch (const exception& error) {
    ostringstream oss;
    oss << "std::exception caught in " << __FUNCTION__ << "[" << port << "] an SC_THREAD of " << kind() << " '" << name() << "'." << endl;
    oss << "what(): " << error.what() << endl;
    oss << __FUNCTION__ << " now rethrowing:";
    xtsc_log_multiline(m_text, FATAL_LOG_LEVEL, oss.str(), 2);
    throw;
  }
}



void xtsc_component::xtsc_memory_pin::apb_drive_outputs_thread(void) {

  // Get the port number for this "instance" of the thread
  u32 port = m_next_port_apb_drive_outputs_thread++;

  // A try/catch block in sc_main will not catch an exception thrown from
  // an SC_THREAD, so we'll catch them here, log them, then rethrow them.
  try {

    XTSC_INFO(m_text, "in " << __FUNCTION__ << "[" << port << "]");

    *m_data[port] = 0;
    m_p_pready [port]->write(false);
    m_p_pslverr[port]->write(false);
    m_p_prdata [port]->write(*m_data[port]);

    // Loop forever
    while (true) {
      wait(m_apb_req_event[port]);
      bool         is_write = m_p_pwrite[port]->read();
      xtsc_address address8 = m_p_paddr[port]->read();
      xtsc_address addr8    = address8 & m_address_mask;
      bool pslverr = (addr8 != address8) || (addr8 < m_start_address8) || (addr8 > m_end_address8);
      u32  bytes   = m_p_pstrb[port]->read();
      if (is_write) {
        *m_data[port] = m_p_pwdata[port]->read();
      }
      else if (pslverr) {
        *m_data[port] = 0;
      }
      else {
        u32           page            = m_p_memory->get_page(addr8);
        u32           mem_offset      = m_p_memory->get_page_offset(addr8);
        u32           delta           = (m_big_endian ? -8 : +8);
        u32           bit_offset      = 8 * (m_big_endian ? (m_width8 - 1) : 0);
        XTSC_DEBUG(m_text, __FUNCTION__ << ": address8=0x" << hex << setfill('0') << setw(8) << address8 << " addr8=0x" << setw(8) << addr8 <<
                           " page=" << dec << page << " mem_offset=" << mem_offset << " bit_offset=" << bit_offset);
        for (u32 i = 0; i<m_width8; ++i) {
          m_data[port]->range(bit_offset+7, bit_offset) = *(m_p_memory->m_page_table[page]+mem_offset);
          mem_offset += 1;
          bit_offset += delta;
        }
        m_p_prdata[port]->write(*m_data[port]);
      }
      u32 data = m_data[port]->to_uint();
      XTSC_INFO(m_text, (is_write ? "WRITE" : "READ ") << " 0x" << hex << setfill('0') << setw(8) << addr8 << "/0x" << bytes <<
                        ": 0x" << setw(8) << data << (pslverr ? " PSLVERR" : "") << " Port #" << port);
      m_p_pready [port]->write(true);
      m_p_pslverr[port]->write(pslverr);
      wait(m_clock_period);
      m_p_pready [port]->write(false);
      m_p_pslverr[port]->write(false);
      if (pslverr) {
        ;  // Do nothing?
      }
      else if (is_write) {
        u32           page            = m_p_memory->get_page(addr8);
        u32           mem_offset      = m_p_memory->get_page_offset(addr8);
        u32           bit_offset      = 0;
        XTSC_DEBUG(m_text, __FUNCTION__ << ": address8=0x" << hex << setfill('0') << setw(8) << address8 << " addr8=0x" << setw(8) << addr8 <<
                           " page=" << dec << page << " mem_offset=" << mem_offset << " bit_offset=" << bit_offset <<
                           " bytes=0x" << hex << bytes);
        for (u32 i = 0; i<m_width8; ++i) {
          if (bytes & 0x1) {
            *(m_p_memory->m_page_table[page]+mem_offset) = (u8) m_data[port]->range(bit_offset+7, bit_offset).to_uint();
          }
          bytes >>= 1;
          mem_offset += 1;
          bit_offset += 8;
        }
      }
      else {
        *m_data[port] = 0;
        m_p_prdata[port]->write(*m_data[port]);
      }
    }
  }
  catch (const sc_unwind_exception& error) { throw; }
  catch (const exception& error) {
    ostringstream oss;
    oss << "std::exception caught in " << __FUNCTION__ << "[" << port << "] an SC_THREAD of " << kind() << " '" << name() << "'." << endl;
    oss << "what(): " << error.what() << endl;
    oss << __FUNCTION__ << " now rethrowing:";
    xtsc_log_multiline(m_text, FATAL_LOG_LEVEL, oss.str(), 2);
    throw;
  }

}



void xtsc_component::xtsc_memory_pin::do_read(const pif_req_info& info) {
  u32           port            = info.m_port;
  xtsc_address  address8        = info.m_address;
  xtsc_address  addr8           = address8 & m_address_mask;
  u32           page            = m_p_memory->get_page(addr8);
  u32           mem_offset      = m_p_memory->get_page_offset(addr8);
  u32           delta           = (m_big_endian ? -8 : +8);
  u32           bit_offset      = 8 * (m_big_endian ? (m_width8 - 1) : 0);
  u64           bytes           = info.m_fixed_byte_enables;

  XTSC_DEBUG(m_text, "do_read: address8=0x" << hex << setfill('0') << setw(8) << address8 << " addr8=0x" << setw(8) << addr8 <<
                     " page=" << dec << page << " mem_offset=" << mem_offset << " bit_offset=" << bit_offset <<
                     " bytes=0x" << hex << bytes);

  for (u32 i = 0; i<m_width8; ++i) {
    if (bytes & 0x1) {
      m_data[port]->range(bit_offset+7, bit_offset) = *(m_p_memory->m_page_table[page]+mem_offset);
    }
    bytes >>= 1;
    mem_offset += 1;
    bit_offset += delta;
  }

  if (xtsc_is_text_logging_enabled() && m_text.isEnabledFor(log4xtensa::INFO_LOG_LEVEL)) {
    ostringstream oss;
    oss << hex << *m_data[port];
    XTSC_INFO(m_text, info << "=0x" << oss.str().substr(1));
  }

  pif_drive_response(info, m_resp_cntl, *m_data[port]);
}



void xtsc_component::xtsc_memory_pin::do_block_read(const pif_req_info& info) {
  u32           port            = info.m_port;
  xtsc_address  address8        = info.m_address;
  xtsc_address  addr8           = address8 & m_address_mask;
  u32           page            = m_p_memory->get_page(addr8);
  u32           mem_offset      = m_p_memory->get_page_offset(addr8);
  u32           delta           = (m_big_endian ? -8 : +8);
  u32           num_transfers   = info.m_req_cntl.get_num_transfers();
  u32           block_size      = num_transfers * m_width8;
  xtsc_address  addr8_limit     = addr8 | (block_size - 1);

  XTSC_DEBUG(m_text, "do_block_read: address8=0x" << hex << setfill('0') << setw(8) << address8 << " addr8=0x" << setw(8) << addr8 <<
                     " page=" << dec << page << " mem_offset=" << mem_offset << " num_transfers=" << num_transfers << 
                     " block_size=" << block_size << " addr8_limit=0x" << hex << setw(8) << addr8_limit);

  for (u32 j=0; j<num_transfers; ++j) {
    u32         bit_offset      = 8 * (m_big_endian ? (m_width8 - 1) : 0);
    bool        last            = ((j+1)==num_transfers);
    for (u32 i = 0; i<m_width8; ++i) {
      m_data[port]->range(bit_offset+7, bit_offset) = *(m_p_memory->m_page_table[page]+mem_offset);
      mem_offset += 1;
      addr8 += 1;
      bit_offset += delta;
    }
    m_resp_cntl.set_last_transfer(last);

    if (xtsc_is_text_logging_enabled() && m_text.isEnabledFor(log4xtensa::INFO_LOG_LEVEL)) {
      ostringstream oss;
      oss << hex << *m_data[port];
      XTSC_INFO(m_text, "BlR    0x" << hex << (address8 + m_width8*j) << "=0x" << oss.str().substr(1) << " " << m_resp_cntl);
    }

    pif_drive_response(info, m_resp_cntl, *m_data[port]);
    if (!last && (m_block_read_repeat > m_clock_period)) {
      wait(m_block_read_repeat - m_clock_period);
    }
    if (addr8 > addr8_limit) {
      xtsc_address old_addr8 = addr8;
      addr8 = old_addr8 - block_size;
      mem_offset = m_p_memory->get_page_offset(addr8);
      XTSC_DEBUG(m_text, "do_block_read: after transfer #" << (j+1) << ", wrapping address from 0x" << hex << setfill('0') <<
                         setw(8) << old_addr8 << " to 0x" << setw(8) << addr8 << " mem_offset=" << mem_offset);
    }
  }
}



void xtsc_component::xtsc_memory_pin::do_burst_read(const pif_req_info& info) {
  u32           port            = info.m_port;
  xtsc_address  address8        = info.m_address;
  xtsc_address  addr8           = address8 & m_address_mask;
  u32           page            = m_p_memory->get_page(addr8);
  u32           mem_offset      = m_p_memory->get_page_offset(addr8);
  u32           delta           = (m_big_endian ? -8 : +8);
  u32           num_transfers   = info.m_req_cntl.get_num_transfers();

  XTSC_DEBUG(m_text, "do_burst_read: address8=0x" << hex << setfill('0') << setw(8) << address8 << " addr8=0x" << setw(8) << addr8 <<
                     " page=" << dec << page << " mem_offset=" << mem_offset << " num_transfers=" << num_transfers);

  for (u32 j=0; j<num_transfers; ++j) {
    u32         bit_offset      = 8 * (m_big_endian ? (m_width8 - 1) : 0);
    bool        last            = ((j+1)==num_transfers);
    for (u32 i = 0; i<m_width8; ++i) {
      m_data[port]->range(bit_offset+7, bit_offset) = *(m_p_memory->m_page_table[page]+mem_offset);
      mem_offset += 1;
      addr8 += 1;
      bit_offset += delta;
    }
    m_resp_cntl.set_last_transfer(last);

    if (xtsc_is_text_logging_enabled() && m_text.isEnabledFor(log4xtensa::INFO_LOG_LEVEL)) {
      ostringstream oss;
      oss << hex << *m_data[port];
      XTSC_INFO(m_text, "BuR    0x" << hex << (address8 + m_width8*j) << "=0x" << oss.str().substr(1) << " " << m_resp_cntl);
    }

    pif_drive_response(info, m_resp_cntl, *m_data[port]);
    if (!last && (m_burst_read_repeat > m_clock_period)) {
      wait(m_burst_read_repeat - m_clock_period);
    }
  }
}



void xtsc_component::xtsc_memory_pin::do_rcw(const pif_req_info& info) {
  u32           port            = info.m_port;
  if (!info.m_req_cntl.get_last_transfer()) {
    *m_rcw_compare_data[port] = info.m_data;
    XTSC_INFO(m_text, info);
    wait(m_clock_period);
  }
  else {
    xtsc_address  address8        = info.m_address;
    xtsc_address  addr8           = address8 & m_address_mask;
    u32           page            = m_p_memory->get_page(addr8);
    u32           mem_offset      = m_p_memory->get_page_offset(addr8);
    u32           delta           = (m_big_endian ? -8 : +8);
    u32           bit_offset      = 8 * (m_big_endian ? (m_width8 - 1) : 0);
    u64           bytes           = info.m_fixed_byte_enables;
    bool          data_matches    = true;

    XTSC_DEBUG(m_text, "do_rcw: address8=0x" << hex << setfill('0') << setw(8) << address8 << " addr8=0x" << setw(8) << addr8 <<
                       " page=" << dec << page << " mem_offset=" << mem_offset << " bit_offset=" << bit_offset <<
                       " bytes=0x" << hex << bytes);

    // Compare data and capture old memory value in m_data
    for (u32 i = 0; i<m_width8; ++i) {
      if (bytes & 0x1) {
        m_data[port]->range(bit_offset+7, bit_offset) = *(m_p_memory->m_page_table[page]+mem_offset);
        if (m_rcw_compare_data[port]->range(bit_offset+7, bit_offset) != m_data[port]->range(bit_offset+7, bit_offset)) {
          data_matches = false;
        }
      }
      bytes >>= 1;
      mem_offset += 1;
      bit_offset += delta;
    }

    if (data_matches) {
      mem_offset  = m_p_memory->get_page_offset(addr8);
      bit_offset  = 8 * (m_big_endian ? (m_width8 - 1) : 0);
      bytes       = info.m_fixed_byte_enables;
      for (u32 i = 0; i<m_width8; ++i) {
        if (bytes & 0x1) {
          *(m_p_memory->m_page_table[page]+mem_offset) = (u8) info.m_data.range(bit_offset+7, bit_offset).to_uint();
        }
        bytes >>= 1;
        mem_offset += 1;
        bit_offset += delta;
      }
    }

    XTSC_INFO(m_text, info << (data_matches ? " Match" : " NoMatch"));

    pif_drive_response(info, m_resp_cntl, *m_data[port]);
  }
}



void xtsc_component::xtsc_memory_pin::do_write(const pif_req_info& info) {
  u32           port            = info.m_port;
  xtsc_address  address8        = info.m_address;
  xtsc_address  addr8           = address8 & m_address_mask;
  u32           page            = m_p_memory->get_page(addr8);
  u32           mem_offset      = m_p_memory->get_page_offset(addr8);
  u32           delta           = (m_big_endian ? -8 : +8);
  u32           bit_offset      = 8 * (m_big_endian ? (m_width8 - 1) : 0);
  u64           bytes           = info.m_fixed_byte_enables;

  XTSC_DEBUG(m_text, "do_write: address8=0x" << hex << setfill('0') << setw(8) << address8 << " addr8=0x" << setw(8) << addr8 <<
                     " page=" << dec << page << " mem_offset=" << mem_offset << " bit_offset=" << bit_offset <<
                     " bytes=0x" << hex << bytes);

  for (u32 i = 0; i<m_width8; ++i) {
    if (bytes & 0x1) {
      *(m_p_memory->m_page_table[page]+mem_offset) = (u8) info.m_data.range(bit_offset+7, bit_offset).to_uint();
    }
    bytes >>= 1;
    mem_offset += 1;
    bit_offset += delta;
  }

  XTSC_INFO(m_text, info);

  if (m_write_responses) {
    *m_data[port] = 0;
    pif_drive_response(info, m_resp_cntl, *m_data[port]);
  }
  else {
    wait(m_clock_period);
  }
}



void xtsc_component::xtsc_memory_pin::do_block_write(const pif_req_info& info) {
  u32           port            = info.m_port;
  xtsc_address  address8        = info.m_address;                                                       // Address on the input pins
  if (m_first_block_write[port]) {
    m_block_write_xfers[port]   = info.m_req_cntl.get_num_transfers();
    m_block_write_address[port] = address8 & m_address_mask;                                            // Mask out bus byte lanes
    m_block_write_offset[port]  = m_block_write_address[port] % (m_width8 * m_block_write_xfers[port]); // CWF offset
    m_block_write_address[port]-= m_block_write_offset[port];                                           // Subtract off CWF offset
  }
  xtsc_address  addr8           = m_block_write_address[port] + m_block_write_offset[port];             // Address where data should go
  u32           page            = m_p_memory->get_page(addr8);
  u32           mem_offset      = m_p_memory->get_page_offset(addr8);
  u32           delta           = (m_big_endian ? -8 : +8);
  u32           bit_offset      = 8 * (m_big_endian ? (m_width8 - 1) : 0);
  u64           bytes           = info.m_fixed_byte_enables;

  XTSC_DEBUG(m_text, "do_block_write: address8=0x" << hex << setfill('0') << setw(8) << address8 << " addr8=0x" << setw(8) << addr8 <<
                     " page=" << dec << page << " mem_offset=" << mem_offset << " bit_offset=" << bit_offset <<
                     " bytes=0x" << hex << bytes);

  for (u32 i = 0; i<m_width8; ++i) {
    if (bytes & 0x1) {
      *(m_p_memory->m_page_table[page]+mem_offset) = (u8) info.m_data.range(bit_offset+7, bit_offset).to_uint();
    }
    bytes >>= 1;
    mem_offset += 1;
    bit_offset += delta;
  }

  XTSC_INFO(m_text, info);

  if (!info.m_req_cntl.get_last_transfer()) {
    m_first_block_write[port] = false;
    m_block_write_offset[port] = (m_block_write_offset[port] + m_width8) % (m_width8 * m_block_write_xfers[port]);
    wait(m_clock_period);
  }
  else {
    m_first_block_write[port] = true;
    if (m_write_responses) {
      *m_data[port] = 0;
      pif_drive_response(info, m_resp_cntl, *m_data[port]);
    }
    else {
      wait(m_clock_period);
    }
  }
}



void xtsc_component::xtsc_memory_pin::do_burst_write(const pif_req_info& info) {
  u32           port            = info.m_port;
  xtsc_address  address8        = m_first_burst_write[port] ? info.m_address : m_burst_write_address[port];
  xtsc_address  addr8           = address8 & m_address_mask;
  u32           page            = m_p_memory->get_page(addr8);
  u32           mem_offset      = m_p_memory->get_page_offset(addr8);
  u32           delta           = (m_big_endian ? -8 : +8);
  u32           bit_offset      = 8 * (m_big_endian ? (m_width8 - 1) : 0);
  u64           bytes           = info.m_fixed_byte_enables;

  XTSC_DEBUG(m_text, "do_burst_write: address8=0x" << hex << setfill('0') << setw(8) << address8 << " addr8=0x" << setw(8) << addr8 <<
                     " page=" << dec << page << " mem_offset=" << mem_offset << " bit_offset=" << bit_offset <<
                     " bytes=0x" << hex << bytes);

  for (u32 i = 0; i<m_width8; ++i) {
    if (bytes & 0x1) {
      *(m_p_memory->m_page_table[page]+mem_offset) = (u8) info.m_data.range(bit_offset+7, bit_offset).to_uint();
    }
    mem_offset += 1;
    bit_offset += delta;
    bytes >>= 1;
  }

  XTSC_INFO(m_text, info);

  if (!info.m_req_cntl.get_last_transfer()) {
    m_first_burst_write[port] = false;
    m_burst_write_address[port] = addr8 + m_width8;
    wait(m_clock_period);
  }
  else {
    m_first_burst_write[port] = true;
    if (m_write_responses) {
      *m_data[port] = 0;
      pif_drive_response(info, m_resp_cntl, *m_data[port]);
    }
    else {
      wait(m_clock_period);
    }
  }
}



void xtsc_component::xtsc_memory_pin::pif_drive_response(const pif_req_info&  info,
                                                         const resp_cntl&     response,
                                                         const sc_bv_base&    data)
{
  u32 port = info.m_port;

  // Assert signals 
  m_p_resp_valid[port]->write(true);
  m_p_resp_cntl[port]->write(response.get_value());
  m_p_resp_data[port]->write(data);
  XTSC_VERBOSE(m_text, "Drive data=0x" << hex << data);
  if (m_p_resp_id[port]) m_p_resp_id[port]->write(info.m_id);
  m_p_resp_priority[port]->write(info.m_priority);
  if (m_p_resp_route_id [port]) m_p_resp_route_id [port]->write(info.m_route_id);
  if (m_p_resp_user_data[port]) m_p_resp_user_data[port]->write(*m_rsp_user_data_val[port]);

  // Drive in time chunks of one clock cycle until accepted
  bool ready = true;
  do {
    if (m_sample_phase == m_drive_phase) {
      wait(m_clock_period);
      ready = m_p_resp_rdy[port]->read();
    }
    else {
      wait(m_drive_to_sample_time);
      ready = m_p_resp_rdy[port]->read();
      XTSC_DEBUG(m_text, "pif_drive_response() at sample_phase: ready=" << ready);
      wait(m_sample_to_drive_time);
      XTSC_DEBUG(m_text, "pif_drive_response() at drive_phase");
    }
  } while (!ready);

  // Deassert signals
  m_p_resp_valid[port]->write(false);
  m_p_resp_cntl[port]->write(m_resp_cntl_zero.get_value());
  m_p_resp_data[port]->write(m_data_zero);
  if (m_p_resp_id[port]) m_p_resp_id[port]->write(m_id_zero);
  m_p_resp_priority[port]->write(m_priority_zero);
  if (m_p_resp_route_id [port]) m_p_resp_route_id [port]->write(m_route_id_zero);
  if (m_p_resp_user_data[port]) m_p_resp_user_data[port]->write("0");
}



void xtsc_component::xtsc_memory_pin::do_burst_read(const axi_addr_info& info) {
  u32           port            = info.m_port;
  u32           num_transfers   = info.m_AxLEN + 1;
  u32           size            = (1 << info.m_AxSIZE);
  u32           total_size8     = size * num_transfers; 
  bool          fixed           = (info.m_AxBURST == 0);
  bool          wrap            = (info.m_AxBURST == 2);
  xtsc_address  address8        = info.m_AxADDR;
  xtsc_address  addr8           = address8;
  u32           resp            = OKAY;         // TODO: RRESP/resp of other than OKAY, can't that happen here?
  xtsc_address  lower_wrap_addr = 0;
  xtsc_address  upper_wrap_addr = 0;
  if (wrap) {
    lower_wrap_addr = (address8 / total_size8) * total_size8;
    upper_wrap_addr = lower_wrap_addr + total_size8;
  }
  for (u32 j=0; j<num_transfers; ++j) {
    u32         page            = m_p_memory->get_page(addr8);
    u32         mem_offset      = m_p_memory->get_page_offset(addr8);
    u32         bit_offset      = (addr8 % m_width8) * 8;
    bool        last            = ((j+1)==num_transfers);
    XTSC_DEBUG(m_text, "do_burst_read: address8=0x" << hex << setfill('0') << setw(8) << address8 << " addr8=0x" << setw(8) << addr8 << dec <<
                       " size=" << size << " page=" << page << " mem_offset=" << mem_offset << " transfer=" << (j+1) << "/" << num_transfers <<
                       " last=" << last);
    *m_data[port] = 0;
    for (u32 i = 0; i<size; ++i) {
      m_data[port]->range(bit_offset+7, bit_offset) = *(m_p_memory->m_page_table[page]+mem_offset);
      mem_offset += 1;
      bit_offset += 8;
    }
    if (xtsc_is_text_logging_enabled() && m_text.isEnabledFor(log4xtensa::INFO_LOG_LEVEL)) {
      ostringstream oss;
      oss << hex << *m_data[port];
      XTSC_INFO(m_text, info << " 0x" << hex << addr8 << ":" << dec << (j+1) << "=0x" << oss.str().substr(1));
    }
    axi_drive_rd_rsp(info, resp, *m_data[port], last);
    if (!last && (m_burst_read_repeat > m_clock_period)) {
      wait(m_burst_read_repeat - m_clock_period);
    }
    if (!fixed) {
      addr8 += size;
      if (wrap && (addr8 >= upper_wrap_addr)) {
        addr8 -= total_size8;
      }
    }
  }
}



void xtsc_component::xtsc_memory_pin::axi_drive_rd_rsp(const axi_addr_info& info, u32 resp, const sc_bv_base& data, bool last) {
  u32 port = info.m_port;
  u32 indx = info.m_indx;

  // Assert signals 
  *m_xID[port] = info.m_AxID;
  m_xRESP      = resp;
  m_p_rid   [indx]->write(*m_xID[port]);
  m_p_rdata [indx]->write(data);
  m_p_rresp [indx]->write(m_xRESP);
  m_p_rlast [indx]->write(last);
  m_p_rvalid[indx]->write(true);
  XTSC_VERBOSE(m_text, "axi_drive_rd_rsp[" << port << "] drive RDATA[" << indx << "]=0x" << hex << data);

  // Drive in time chunks of one clock cycle until accepted
  bool ready = true;
  do {
    if (m_sample_phase == m_drive_phase) {
      wait(m_clock_period);
      ready = m_p_rready[indx]->read();
    }
    else {
      wait(m_drive_to_sample_time);
      ready = m_p_rready[indx]->read();
      XTSC_DEBUG(m_text, "axi_drive_rd_rsp[" << port << "] at sample_phase: ready=" << ready);
      wait(m_sample_to_drive_time);
      XTSC_DEBUG(m_text, "axi_drive_rd_rsp[" << port << "] at drive_phase");
    }
  } while (!ready);

  // Deassert signals
  *m_xID[port] = 0;
  m_xRESP      = 0;;
  m_p_rid   [indx]->write(*m_xID[port]);
  m_p_rdata [indx]->write(m_data_zero);
  m_p_rresp [indx]->write(m_xRESP);
  m_p_rlast [indx]->write(false);
  m_p_rvalid[indx]->write(false);
}



void xtsc_component::xtsc_memory_pin::do_burst_write(const axi_addr_info& addr_info, const axi_data_info& data_info, u32 transfer_num) {
  u32           port            = addr_info.m_port;
  u32           num_transfers   = addr_info.m_AxLEN + 1;
  u32           size            = (1 << addr_info.m_AxSIZE);
  u32           total_size8     = size * num_transfers; 
  bool          fixed           = (addr_info.m_AxBURST == 0);
  bool          wrap            = (addr_info.m_AxBURST == 2);
  xtsc_address  address8        = addr_info.m_AxADDR;
  xtsc_address  aligned_address = (address8 / size) * size;
  xtsc_address  addr8           = aligned_address + (fixed ? 0 : (size * transfer_num));
  xtsc_address  lower_wrap_addr = 0;
  xtsc_address  upper_wrap_addr = 0;
  if (wrap) {
    lower_wrap_addr = (address8 / total_size8) * total_size8;
    upper_wrap_addr = lower_wrap_addr + total_size8;
    if (addr8 >= upper_wrap_addr ) {
      addr8 -= total_size8;
    }
  }
  u32           page            = m_p_memory->get_page(addr8);
  u32           mem_offset      = m_p_memory->get_page_offset(addr8);
  u32           bus_offset      = (addr8 % m_width8);
  u32           bit_offset      = 8 * bus_offset;
  u64           wstrb           = data_info.m_WSTRB;
  u64           mask            = (1 << bus_offset);
  XTSC_DEBUG(m_text, "do_burst_write: address8=0x" << hex << setfill('0') << setw(8) << address8 << " addr8=0x" << setw(8) << addr8 <<
                     " size=" << dec << size << " page=" << page << " mem_offset=" << mem_offset << " bit_offset=" << bit_offset <<
                     " bus_offset=" << bus_offset << " WSTRB=0x" << hex << wstrb);
  for (u32 i = 0; i<size; ++i) {
    if (wstrb & mask) {
      *(m_p_memory->m_page_table[page]+mem_offset) = (u8) data_info.m_WDATA.range(bit_offset+7, bit_offset).to_uint();
    }
    mem_offset += 1;
    bit_offset += 8;
    mask <<= 1;
  }
  XTSC_INFO(m_text, addr_info << " 0x" << hex << addr8 << ":" << dec << (transfer_num+1) << "=" << data_info);
}



void xtsc_component::xtsc_memory_pin::axi_drive_wr_rsp(const axi_addr_info& info, u32 resp) {
  u32 port = info.m_port;
  u32 indx = info.m_indx;

  // Assert signals 
  *m_xID[port] = info.m_AxID;
  m_xRESP      = resp;
  m_p_bid   [indx]->write(*m_xID[port]);
  m_p_bresp [indx]->write(m_xRESP);
  m_p_bvalid[indx]->write(true);
  XTSC_VERBOSE(m_text, "axi_drive_wr_rsp[" << port << "] drive resp=" << resp);

  // Drive in time chunks of one clock cycle until accepted
  bool ready = true;
  do {
    if (m_sample_phase == m_drive_phase) {
      wait(m_clock_period);
      ready = m_p_bready[indx]->read();
    }
    else {
      wait(m_drive_to_sample_time);
      ready = m_p_bready[indx]->read();
      XTSC_DEBUG(m_text, "axi_drive_wr_rsp[" << port << "] at sample_phase: ready=" << ready);
      wait(m_sample_to_drive_time);
      XTSC_DEBUG(m_text, "axi_drive_wr_rsp[" << port << "] at drive_phase");
    }
  } while (!ready);

  // Deassert signals
  *m_xID[port] = 0;
  m_xRESP      = 0;
  m_p_bid   [indx]->write(*m_xID[port]);
  m_p_bresp [indx]->write(m_xRESP);
  m_p_bvalid[indx]->write(false);
}



bool_signal& xtsc_component::xtsc_memory_pin::create_bool_signal(const string& signal_name) {
  bool_signal *p_signal = new bool_signal(signal_name.c_str());
  m_map_bool_signal[signal_name] = p_signal;
  return *p_signal;
}



uint_signal& xtsc_component::xtsc_memory_pin::create_uint_signal(const string& signal_name, u32 num_bits) {
  sc_length_context length(num_bits);
  uint_signal *p_signal = new uint_signal(signal_name.c_str());
  m_map_uint_signal[signal_name] = p_signal;
  return *p_signal;
}



wide_signal& xtsc_component::xtsc_memory_pin::create_wide_signal(const string& signal_name, u32 num_bits) {
  sc_length_context length(num_bits);
  wide_signal *p_signal = new wide_signal(signal_name.c_str());
  m_map_wide_signal[signal_name] = p_signal;
  return *p_signal;
}



void xtsc_component::xtsc_memory_pin::swizzle_byte_enables(xtsc_byte_enables& byte_enables) const {
  xtsc_byte_enables swizzled = 0;
  for (u32 i=0; i<m_width8; i++) {
    swizzled <<= 1;
    swizzled |= byte_enables & 1;
    byte_enables >>= 1;
  }
  byte_enables = swizzled;
}



xtsc_component::xtsc_memory_pin::pif_req_info *xtsc_component::xtsc_memory_pin::new_pif_req_info(u32 port) {
  if (m_pif_req_pool.empty()) {
    XTSC_DEBUG(m_text, "Creating a new pif_req_info");
    return new pif_req_info(*this, port);
  }
  else {
    pif_req_info *p_pif_req_info = m_pif_req_pool.back();
    m_pif_req_pool.pop_back();
    p_pif_req_info->init(port);
    return p_pif_req_info;
  }
}



void xtsc_component::xtsc_memory_pin::delete_pif_req_info(pif_req_info*& p_pif_req_info) {
  m_pif_req_pool.push_back(p_pif_req_info);
  p_pif_req_info = 0;
}






xtsc_component::xtsc_memory_pin::pif_req_info::pif_req_info(const xtsc_memory_pin& memory, u32 port) :
  m_memory      (memory),
  m_req_cntl    (0),
  m_data        ((int)m_memory.m_width8*8),
  m_id          (m_id_bits),
  m_priority    (2),
  m_route_id    (m_memory.m_route_id_bits ? m_memory.m_route_id_bits : 1)
{
  init(port);
}



void xtsc_component::xtsc_memory_pin::pif_req_info::init(u32 port) {
  m_port                = port;
  m_time_stamp          = sc_time_stamp();
  m_req_cntl            = m_memory.m_p_req_cntl[port]->read().to_uint();
  m_address             = m_memory.m_p_req_adrs[port]->read().to_uint();
  m_data                = m_memory.m_p_req_data[port]->read();
  m_byte_enables        = m_memory.m_p_req_data_be[port]->read().to_uint();
  m_id                  = m_memory.m_p_req_id[port] ? m_memory.m_p_req_id[port]->read() : 0;
  m_priority            = m_memory.m_p_req_priority[port]->read();
  m_route_id            = m_memory.m_p_req_route_id[port] ? m_memory.m_p_req_route_id[port]->read() : 0;
  m_fixed_byte_enables  = m_byte_enables;
  if (m_memory.m_big_endian) {
    m_memory.swizzle_byte_enables(m_fixed_byte_enables);
  }
}



void xtsc_component::xtsc_memory_pin::pif_req_info::dump(ostream& os) const {
  // Save state of stream
  char c = os.fill('0');
  ios::fmtflags old_flags = os.flags();
  streamsize w = os.width();
  os << right;

  os << m_req_cntl << " [0x" << hex << setw(8) << m_address << "/0x" << setw(m_memory.m_width8/4) << m_byte_enables << "]";

  u32 type = m_req_cntl.get_type();
  if (type == req_cntl::WRITE || type == req_cntl::BLOCK_WRITE || type == req_cntl::BURST_WRITE || type == req_cntl::RCW) {
    ostringstream oss;
    oss << hex << m_data;
    os << "=0x" << oss.str().substr(1);
  }

  // Restore state of stream
  os.fill(c);
  os.flags(old_flags);
  os.width(w);
}



namespace xtsc_component {
ostream& operator<<(ostream& os, const xtsc_memory_pin::pif_req_info& info) {
  info.dump(os);
  return os;
}
}



xtsc_component::xtsc_memory_pin::axi_addr_info *xtsc_component::xtsc_memory_pin::new_axi_addr_info(u32 port) {
  if (m_axi_addr_pool.empty()) {
    XTSC_DEBUG(m_text, "Creating a new axi_addr_info");
    return new axi_addr_info(*this, port);
  }
  else {
    axi_addr_info *p_axi_addr_info = m_axi_addr_pool.back();
    m_axi_addr_pool.pop_back();
    p_axi_addr_info->init(port);
    return p_axi_addr_info;
  }
}



void xtsc_component::xtsc_memory_pin::delete_axi_addr_info(axi_addr_info*& p_axi_addr_info) {
  m_axi_addr_pool.push_back(p_axi_addr_info);
  p_axi_addr_info = 0;
}



xtsc_component::xtsc_memory_pin::axi_addr_info::axi_addr_info(const xtsc_memory_pin& memory, u32 port) :
  m_memory      (memory)
{
  init(port);
}



void xtsc_component::xtsc_memory_pin::axi_addr_info::init(u32 port) {
  m_port                = port;
  m_read_channel        = (m_memory.m_axi_read_write[m_port] == 0);
  m_indx                = m_memory.m_axi_signal_indx[m_port];
  m_time_stamp          = sc_time_stamp();
  if (m_read_channel) {
    m_AxID      = m_memory.m_p_arid   [m_indx]->read().to_uint();
    m_AxADDR    = m_memory.m_p_araddr [m_indx]->read().to_uint();
    m_AxLEN     = m_memory.m_p_arlen  [m_indx]->read().to_uint();
    m_AxSIZE    = m_memory.m_p_arsize [m_indx]->read().to_uint();
    m_AxBURST   = m_memory.m_p_arburst[m_indx]->read().to_uint();
    m_AxLOCK    = m_memory.m_p_arlock [m_indx]->read();
    m_AxCACHE   = m_memory.m_p_arcache[m_indx]->read().to_uint();
    m_AxPROT    = m_memory.m_p_arprot [m_indx]->read().to_uint();
    m_AxQOS     = m_memory.m_p_arqos  [m_indx]->read().to_uint();
  }
  else {
    m_AxID      = m_memory.m_p_awid   [m_indx]->read().to_uint();
    m_AxADDR    = m_memory.m_p_awaddr [m_indx]->read().to_uint();
    m_AxLEN     = m_memory.m_p_awlen  [m_indx]->read().to_uint();
    m_AxSIZE    = m_memory.m_p_awsize [m_indx]->read().to_uint();
    m_AxBURST   = m_memory.m_p_awburst[m_indx]->read().to_uint();
    m_AxLOCK    = m_memory.m_p_awlock [m_indx]->read();
    m_AxCACHE   = m_memory.m_p_awcache[m_indx]->read().to_uint();
    m_AxPROT    = m_memory.m_p_awprot [m_indx]->read().to_uint();
    m_AxQOS     = m_memory.m_p_awqos  [m_indx]->read().to_uint();
  }
  u32  num   = m_AxLEN + 1;
  bool fixed = (m_AxBURST == 0);
  bool wrap  = (m_AxBURST == 2);
  if ((fixed && (num > 16)) || (wrap && (num != 2) && (num != 4) && (num != 8) && (num != 16))) {
    ostringstream oss;
    oss << m_memory.kind() << " '" << m_memory.name() << "': Received " << (fixed ? "FIXED" : "WRAP") << " burst "
        << (m_read_channel ? "read" : "write") << " with illegal AxLEN=" << (num-1);
    throw xtsc_exception(oss.str());
  }
}



void xtsc_component::xtsc_memory_pin::axi_addr_info::dump(ostream& os) const {
  // Save state of stream
  char c = os.fill('0');
  ios::fmtflags old_flags = os.flags();
  streamsize w = os.width();
  os << right;

  os << (m_read_channel ? "Rd" : "Wr") << " " << ((m_AxBURST == 0) ? "FIXD" : (m_AxBURST == 1) ? "INCR" : (m_AxBURST == 2) ? "WRAP" : "UNKN") 
     << " [0x" << hex << setw(8) << m_AxADDR << "/0x" << setw(xtsc_get_text_logging_axi_id_width()) << m_AxID << "/" << dec << (m_AxLEN+1) << "/"
     << (1<<m_AxSIZE) << "]";

  // Restore state of stream
  os.fill(c);
  os.flags(old_flags);
  os.width(w);
}



namespace xtsc_component {
ostream& operator<<(ostream& os, const xtsc_memory_pin::axi_addr_info& info) {
  info.dump(os);
  return os;
}
}



xtsc_component::xtsc_memory_pin::axi_data_info *xtsc_component::xtsc_memory_pin::new_axi_data_info(u32 port) {
  if (m_axi_data_pool.empty()) {
    XTSC_DEBUG(m_text, "Creating a new axi_data_info");
    return new axi_data_info(*this, port);
  }
  else {
    axi_data_info *p_axi_data_info = m_axi_data_pool.back();
    m_axi_data_pool.pop_back();
    p_axi_data_info->init(port);
    return p_axi_data_info;
  }
}



void xtsc_component::xtsc_memory_pin::delete_axi_data_info(axi_data_info*& p_axi_data_info) {
  m_axi_data_pool.push_back(p_axi_data_info);
  p_axi_data_info = 0;
}



xtsc_component::xtsc_memory_pin::axi_data_info::axi_data_info(const xtsc_memory_pin& memory, u32 port) :
  m_memory      (memory),
  m_WDATA       ((int)m_memory.m_width8*8)
{
  init(port);
}



void xtsc_component::xtsc_memory_pin::axi_data_info::init(u32 port) {
  m_port        = port;
  m_indx        = m_memory.m_axi_signal_indx[m_port];
  m_time_stamp  = sc_time_stamp();
  m_WDATA       = m_memory.m_p_wdata[m_indx]->read();
  m_WSTRB       = m_memory.m_p_wstrb[m_indx]->read().to_uint();
  m_WLAST       = m_memory.m_p_wlast[m_indx]->read();
}



void xtsc_component::xtsc_memory_pin::axi_data_info::dump(ostream& os) const {
  // Save state of stream
  char c = os.fill('0');
  ios::fmtflags old_flags = os.flags();
  streamsize w = os.width();

  ostringstream oss;
  oss << hex << m_WDATA;

  os << right << "0x" << oss.str().substr(1) << (m_WLAST ? "*" : " ") << "[0x" << hex << setw(m_memory.m_width8/4) << m_WSTRB << "]";

  // Restore state of stream
  os.fill(c);
  os.flags(old_flags);
  os.width(w);
}



namespace xtsc_component {
ostream& operator<<(ostream& os, const xtsc_memory_pin::axi_data_info& info) {
  info.dump(os);
  return os;
}
}



void xtsc_component::xtsc_memory_pin::xtsc_debug_if_impl::nb_peek(xtsc_address address8, u32 size8, u8 *buffer) {
  m_memory_pin.peek(address8, size8, buffer);
}



void xtsc_component::xtsc_memory_pin::xtsc_debug_if_impl::nb_poke(xtsc_address address8, u32 size8, const u8 *buffer) {
  m_memory_pin.poke(address8, size8, buffer);
}



bool xtsc_component::xtsc_memory_pin::xtsc_debug_if_impl::nb_fast_access(xtsc_fast_access_request &request) {
  xtsc_address address8    = request.get_translated_request_address();
  xtsc_address page_start8 = address8 & ~(m_memory_pin.m_p_memory->m_page_size8 - 1);
  xtsc_address page_end8   = page_start8 + m_memory_pin.m_p_memory->m_page_size8 - 1;

  // Allow any fast access?
  if (!m_memory_pin.m_use_fast_access) {
    request.deny_access();
    XTSC_INFO(m_memory_pin.m_text, hex << setfill('0') << "nb_fast_access: address8=0x" << address8 <<
                               " page=[0x" << page_start8 << "-0x" << page_end8 << "] deny_access");
    return true;
  }
  
  XTSC_VERBOSE(m_memory_pin.m_text, "nb_fast_access: using peek/poke");
  request.allow_peek_poke_access();
  return true;
}



void xtsc_component::xtsc_memory_pin::xtsc_debug_if_impl::register_port(sc_port_base& port, const char *if_typename) {
  if (m_p_port) {
    ostringstream oss;
    oss << "Illegal multiple binding detected to xtsc_memory_pin '" << m_memory_pin.name() << "' m_debug_exports[" << m_port_num
        << "]: " << endl;
    oss << "  " << port.name() << endl;
    oss << "  " << m_p_port->name();
    throw xtsc_exception(oss.str());
  }
  XTSC_INFO(m_memory_pin.m_text, "Binding '" << port.name() << "' to '" << m_memory_pin.name() << ".m_debug_exports[" << m_port_num <<
                                 "]");
  m_p_port = &port;
}



