// Copyright (c) 2007-2018 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of
// Cadence Design Systems, Inc.  They may be adapted and modified by bona fide
// purchasers for internal use, but neither the original nor any adapted
// or modified version may be disclosed or distributed to third parties
// in any manner, medium, or form, in whole or in part, without the prior
// written consent of Cadence Design Systems, Inc.

#include <iostream>
#include <xtsc/xtsc_response.h>
#include <xtsc/xtsc_fast_access.h>
#include <xtsc/xtsc_pin2tlm_memory_transactor.h>
#include <xtsc/xtsc_core.h>
#include <xtsc/xtsc_arbiter.h>
#include <xtsc/xtsc_slave.h>
#include <xtsc/xtsc_mmio.h>
#include <xtsc/xtsc_router.h>
#include <xtsc/xtsc_tlm2pin_memory_transactor.h>


/**
   Theory of operation:
  
   This module converts a pin-level PIF, IDMA0, inbound PIF/AXI, snoop, AXI, IDMA, APB, or local memory interface
   into a XTSC TLM memory interface.  The upstream module is a pin-level memory interface master module and the
   downstream module is a TLM memory interface slave module.  Requests come in on the pins and get converted to
   xtsc_request objects that are passed downstream via a call to nb_request().  Responses come in by means of
   the downstream TLM module calling this modules' nb_respond() method.  This module then converts the
   xtsc_response to the appropriate pin-level response signals (except that PIF RSP_OK write responses when
   "write_responses" is true are simply dropped on the floor).
  
   For local memory interface pins that share a common function, the same data member is used, but it is
   given a SystemC name that exactly matches the Xtensa RTL.  For example, the m_p_en data member is given
   the SystemC name of "DPort0En0" when it is used for XLMI0 load/store unit 0 and it is given the SystemC
   name of "DRam0En0" when it is used for DRAM0 load/store unit 0.  An example data member for a PIF pin is
   m_p_req_adrs which is given the SystemC name of "POReqAdrs".  The comments in this file and the
   corresponding header file generally omit the digits in the signal names (so "DRam0En0", "DRam0En1",
   "DRam1En0", and "DRam1En1" are all referred to as DRamEn)
  
   Multi-ported memories are supported for all memory interfaces.  This is accomplished by having each port
   data member be an array with one entry per memory port.  In addition, one instance of each applicable
   SystemC thread process is created per memory port.  Each thread process knows which memory port it is
   associated with by the local port variable which is created on the stack when the thread process is
   first entered.  This technique does not work for SystemC method processes because they always run to
   completion, so there is only one instance of the dram_lock_method spawned (DRAM only) and it is made
   sensitive to the necessary events of all the memory ports.

   Note that a bi-directional AXI4 interface (i.e. one that supports both read and write) requires 2 separate
   memory ports (1 for read and 1 for write).
  
   When configured for the PIF, IDMA0, inbound PIF, or snoop, the following SystemC threads are used:
     - pif_sample_pin_request_thread
           This thread waits for m_p_req_valid or m_p_req_rdy (representing the POReqValid and PIReqRdy
           signals, respectively) to go high, synchronizes to the sample phase (specified by the
           "sample_phase" parameter), then once each clock period while both m_p_req_valid and m_p_req_rdy
           remain high it reads the pin values, converts them to an xtsc_request, puts the request in a queue
           (m_request_fifo), and notifies pif_send_tlm_request_thread (with a delay of m_output_delay).
     - pif_drive_req_rdy_thread
           This thread drives m_p_req_rdy (representing the PIReqRdy signal) low for one clock period each
           time an nb_respond() call is received with RSP_NACC.  The nb_respond() method notifies this thread
           using m_drive_req_rdy_event.notify(m_output_delay).  To ensure all RSP_NACC calls are accounted for,
           the nb_respond() method adds a false value to m_req_rdy_fifo.  In addition, when the "one_at_a_time"
           parameter is true, then this thread will also deassert PIReqRdy from the time a last-transfer
           request is accepted until the last-transfer response is received.
     - pif_send_tlm_request_thread
           This thread waits to be notified by pif_sample_pin_request_thread that a request has been
           received and then repeatedly sends the TLM request downstream until it is accepted.  Acceptance
           is defined as no RSP_NACC being received for one clock period after the nb_request() call.  When
           "one_at_a_time" is true, this thread is also responsible to notifing pif_drive_req_rdy_thread that a
           last-transfer request has been accepted.  It does this by adding a true value to m_req_rdy_fifo and
           calling m_drive_req_rdy_event.notify(m_output_delay).
     - pif_drive_pin_response_thread
           This thread waits to be notified by the nb_respond() method that a TLM response has been received
           and then repeatedly drives the pin-level response until it has been accepted.  The nb_respond()
           method notifies this thread using m_response_event.notify(m_output_delay).  The response
           information is stored in m_response_fifo.  See the "prereject_responses" parameter.

   When configured with a AXI or IDMA interface, the following SystemC threads are used:
     - axi_req_addr_thread
           This thread is used to sample the addr channel signals (AxID, AxADDR, AxLEN, AxSIZE, etc) and store their
           values in a axi_addr_info object.  For the case of a read memory port, a axi_req_info object is also made at
           this time and added to m_axi_req_fifo and m_axi_send_tlm_req_event is notified to alert axi_send_tlm_req_thread.
           For the case of a write memory port, the axi_addr_info object is stored in m_axi_addr_fifo and 
           m_axi_wr_req_event is notified to alert axi_wr_req_thread in case write data has already been received on the
           write data channel by axi_req_data_thread.
     - axi_req_data_thread
           This thread is used to sample the write data channel signals (WDATA, WSTRB, and WLAST) and store their values
           in a axi_data_info object which is then added to m_axi_data_fifo and m_axi_wr_req_event notified to alert
           axi_wr_req_thread in case the write address has already been received on the write address channel by
           axi_req_addr_thread.
           Note: Thread is spawned for both read and write memory ports, but exits immediately in case of read port.
     - axi_wr_req_thread
           This thread is used to match up each beat from the write data channel (retrieved from m_axi_data_fifo) with
           the corresponding single beat on the write address channel (retrieved from m_axi_addr_fifo).  The beat from
           the write address channel applies to all write data channel beats in the burst.  Once a axi_data_info object
           is paired with a axi_addr_info object, they are used to create a axi_req_info object which is added to
           m_axi_req_fifo and m_axi_send_tlm_req_event is notified to alert axi_send_tlm_req_thread.
           Note: Thread is spawned for both read and write memory ports, but exits immediately in case of read port.
     - axi_drive_req_rdy_thread
           This thread is used to drive the xREADY signals.  ARREADY is used to flow control a read memory port and
           WREADY is used to flow control a write memory port.  For a write memory port, the AWREADY signal is always
           asserted.  This thread is notified by m_drive_req_rdy_event.  When it finds a false value in m_req_rdy_fifo
           if drives xREADY low for one clock period.  When it finds a true value in m_req_rdy_fifo it drives xREADY low
           until another value (dummy) appears in m_req_rdy_fifo.
     - axi_send_tlm_req_thread
           This thread waits for m_axi_send_tlm_req_event and then retrieves a axi_req_info object from m_axi_req_fifo.
           The xtsc_request object (m_request) in it is then repeatedly sent downstream until it is accepted.
     - axi_drive_rsp_thread
           This thread is used to sample xREADY and drive xVALID and xRESP (also RDATA, RLAST if rd port).  It waits on
           the m_response_event (notified from nb_respond) and then retrieves an xtsc_response object from m_response_fifo.
           The xtsc_response object is used to drive the pin-level response channel until the pin-level response is
           accepted.

   When configured with a APB interface, the following SystemC threads are used:
     - apb_request_thread
           This thread is used to sample the pin-level inputs, form the xtsc_request, and send it downstream.
     - apb_drive_outputs_thread
           This thread is notified by nb_request() to drive the output pins for one clock period.

   When configured for a local memory, the following SystemC processes (threads or methods) are used:
     - lcl_request_thread:              Non-split Rd/Wr
       lcl_split_rw_request_thread:     Split Rd/Wr
           This thread waits for m_p_en (representing the DPortEn, DRamEn, DRomEn, IRamEn, or IRomEn signal)
           to go high, synchronizes to the sample phase (specified by the "sample_phase" parameter), then
           once each clock period while m_p_en (or m_p_wr if Wr port of split Rd/Wr interface) remains high
           it reads the pin values, converts them to an xtsc_request, and sends it to the downstream TLM model
           by calling nb_request().
     - lcl_drive_read_data_thread
           This thread drives m_p_data (representing the DPortData, DRamData, DRomData, IRamData, or IRomData 
           signal) for one clock period each time this module's nb_respond() method is called with an
           xtsc_response of RSP_OK.  This thread is activated by the nb_respond() method notifying 
           m_drive_read_data_event.  The m_p_data signal takes its new value at the time after the nb_respond()
           call specified by the "output_delay" parameter.
     - lcl_drive_busy_thread (only if the interface has a busy signal)
           This thread drives m_p_busy (representing the DPortBusy, DRamBusy, DRomBusy, IRamBusy, or IRomBusy
           signal) high for one clock period each time this module's nb_respond() method is called with an
           xtsc_response of RSP_NACC.  This thread is activated by the nb_respond() method notifying 
           m_drive_busy_event. The m_p_busy signal goes high at the time after the nb_respond() call defined by
           the "output_delay" parameter.  For subbanked Data RAM, all nb_respond calls in the same cycle for
           multiple subbanks of the same bank must be consistent - either all RSP_OK or all RSP_NACC.  This
           is tracked and enforced using m_subbank_activity[bank][cycle_index] and related variables.
     - xlmi_load_retired_thread  (xlmi only)
           This thread waits for m_p_retire (representing the DPortLoadRetired signal) to go high, synchronizes
           to the sample phase (specified by the "sample_phase" parameter), then once each clock period while
           m_p_retire remains high it calls the nb_load_retired() method in the downstream TLM module.
     - xlmi_retire_flush_thread  (xlmi only)
           This thread waits for m_p_flush (representing the DPortRetireFlush signal) to go high, synchronizes
           to the sample phase (specified by the "sample_phase" parameter), then once each clock period while
           m_p_flush remains high it calls the nb_retire_flush() method in the downstream TLM module.
     - dram_lock_method  (dram only)
           Each time the m_p_lock signal changes (representing the DRamLock signal) this method calls the
           nb_lock() method in the downstream TLM module.  There is no attempt to synchronize with the sample
           phase.  For subbanked Data RAM, only the 0th subbank of each bank gets the nb_lock calls.
  
 */


using namespace std;
#if SYSTEMC_VERSION >= 20050601
using namespace sc_core;
#endif
using namespace sc_dt;
using namespace log4xtensa;
using namespace xtsc;




// Shorthand aliases
typedef xtsc::xtsc_request::type_t              type_t;
typedef xtsc::xtsc_request::burst_t             burst_t;
typedef sc_core::sc_signal<bool>                bool_signal;
typedef sc_core::sc_signal<sc_dt::sc_uint_base> uint_signal;
typedef sc_core::sc_signal<sc_dt::sc_bv_base>   wide_signal;








// ctor helper
static u32 get_num_ports(const xtsc_component::xtsc_pin2tlm_memory_transactor_parms& pin2tlm_parms) {
  return pin2tlm_parms.get_non_zero_u32("num_ports");
}



// ctor helper
static sc_trace_file *get_trace_file(const xtsc_component::xtsc_pin2tlm_memory_transactor_parms& pin2tlm_parms) {
  return static_cast<sc_trace_file*>(const_cast<void*>(pin2tlm_parms.get_void_pointer("vcd_handle")));
}



// ctor helper
static const char *get_suffix(const xtsc_component::xtsc_pin2tlm_memory_transactor_parms& pin2tlm_parms) {
  const char *port_name_suffix = pin2tlm_parms.get_c_str("port_name_suffix");
  static const char *blank = "";
  return port_name_suffix ? port_name_suffix : blank;
}



// ctor helper
static bool get_banked(const xtsc_component::xtsc_pin2tlm_memory_transactor_parms& pin2tlm_parms) {
  return pin2tlm_parms.get_bool("banked");
}



// ctor helper
static bool get_split_rw(const xtsc_component::xtsc_pin2tlm_memory_transactor_parms& pin2tlm_parms) {
  using namespace xtsc_component;
  string interface_uc = xtsc_module_pin_base::get_interface_uc(pin2tlm_parms.get_c_str("memory_interface"));
  xtsc_module_pin_base::memory_interface_type interface_type = xtsc_module_pin_base::get_interface_type(interface_uc);
  return ((interface_type == xtsc_module_pin_base::DRAM0RW) || (interface_type == xtsc_module_pin_base::DRAM1RW));
}



// ctor helper
static bool get_dma(const xtsc_component::xtsc_pin2tlm_memory_transactor_parms& pin2tlm_parms) {
  return pin2tlm_parms.get_bool("has_dma");
}



// ctor helper
static u32 get_subbanks(const xtsc_component::xtsc_pin2tlm_memory_transactor_parms& pin2tlm_parms) {
  return pin2tlm_parms.get_u32("num_subbanks");
}



xtsc_component::xtsc_pin2tlm_memory_transactor_parms::xtsc_pin2tlm_memory_transactor_parms(const xtsc_core&     core,
                                                                                           const char          *memory_name,
                                                                                           u32                  num_ports)
{
  xtsc_core::memory_port mem_port = xtsc_core::get_memory_port(memory_name, &core);
  if (!core.has_memory_port(mem_port)) {
    ostringstream oss;
    oss << "xtsc_pin2tlm_memory_transactor_parms: xtsc_core '" << core.name() << "' doesn't have a \"" << memory_name
        << "\" memory port.";
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

  xtsc_address  start_address8  = 0;    // PIF|IDMA0|XLMI0
  u32           width8          = core.get_memory_byte_width(mem_port);
  u32           address_bits    = 32;  // PIF|IDMA0|XLMI0
  u32           banks           = 1;
  u32           subbanks        = 0;

  if (xtsc_module_pin_base::is_pif_or_idma(interface_type)) {
    ; // Nothing
  }
  else {
    if (interface_type != xtsc_module_pin_base::XLMI0) {
      core.get_local_memory_starting_byte_address(mem_port, start_address8);
      // Compute address_bits  (size8 must be a power of 2)
      u32 size8 = 0;
      core.get_local_memory_byte_size(mem_port, size8);
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

  init(xtsc_module_pin_base::get_interface_name(interface_type), width8, address_bits, num_ports);

  set("clock_period", core_parms.get_u32("SimClockFactor")*xtsc_get_system_clock_factor());
  set("big_endian", core.is_big_endian());
  set("has_busy", core.has_busy(mem_port));
  set("start_byte_address", (u32)start_address8);
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

  if (interface_type == xtsc_module_pin_base::XLMI0) {
    u32 size8 = 0;
    core.get_local_memory_byte_size(mem_port, size8);
    set("memory_byte_size", size8);
  }

  if (xtsc_module_pin_base::is_axi(interface_type)) {
    bool combine_master_axi_ports = core_parms.get_bool("CombineMasterAXIPorts");
    if (num_ports == 3) {
      vector<u32> vec;
      u32 d = core_parms.get_u32("DataMasterType");
      u32 i = core_parms.get_u32("InstMasterType");
      vec.clear(); vec.push_back(8); vec.push_back(8); vec.push_back(4); set("axi_id_bits",    vec);
      vec.clear(); vec.push_back(0); vec.push_back(1); vec.push_back(0); set("axi_read_write", vec);
      vec.clear(); vec.push_back(d); vec.push_back(d); vec.push_back(i); set("axi_port_type",  vec);
      set("axi_name_prefix", "DataMaster,DataMaster,InstMaster");
    } else if (combine_master_axi_ports && (num_ports == 2)) { 
      vector<u32> vec;
      u32 d = core_parms.get_u32("DataMasterType");
      //Note read channel carries both instruction and data, hence the extra axi-id-bit.
      vec.clear(); vec.push_back(8+1); vec.push_back(8); set("axi_id_bit",     vec);
      vec.clear(); vec.push_back(0);   vec.push_back(1); set("axi_read_write", vec);
      vec.clear(); vec.push_back(d);   vec.push_back(d); set("axi_port_type",  vec);
      set("axi_name_prefix", "AXIMaster,AXIMaster");
    }
  }

  if (interface_type == xtsc_module_pin_base::IDMA) {
    if (num_ports == 2) {
      vector<u32> vec;
      u32 d = 2; // ACE-Lite
      vec.clear(); vec.push_back(8); vec.push_back(8); set("axi_id_bits",    vec);
      vec.clear(); vec.push_back(0); vec.push_back(1); set("axi_read_write", vec);
      vec.clear(); vec.push_back(d); vec.push_back(d); set("axi_port_type",  vec);
      set("axi_name_prefix", "IDMA,IDMA");
    }
  }

}



xtsc_component::xtsc_pin2tlm_memory_transactor::xtsc_pin2tlm_memory_transactor(
    sc_module_name                              module_name,
    const xtsc_pin2tlm_memory_transactor_parms& pin2tlm_parms
  ) :
  sc_module             (module_name),
  xtsc_module           (*(sc_module*)this),
  xtsc_module_pin_base  (*this, ::get_num_ports (pin2tlm_parms),
                                ::get_trace_file(pin2tlm_parms),
                                ::get_suffix    (pin2tlm_parms),
                                ::get_banked    (pin2tlm_parms),
                                ::get_split_rw  (pin2tlm_parms),
                                ::get_dma       (pin2tlm_parms),
                                ::get_subbanks  (pin2tlm_parms)),
  m_num_ports           (pin2tlm_parms.get_non_zero_u32("num_ports")),
  m_num_axi_rd_ports    (0),
  m_num_axi_wr_ports    (0),
  m_interface_uc        (get_interface_uc(pin2tlm_parms.get_c_str("memory_interface"))),
  m_interface_lc        (get_interface_lc(pin2tlm_parms.get_c_str("memory_interface"))),
  m_interface_type      (xtsc_module_pin_base::get_interface_type(m_interface_uc)),
  m_size8               (pin2tlm_parms.get_u32("memory_byte_size")),
  m_width8              (pin2tlm_parms.get_non_zero_u32("byte_width")),
  m_dram_attribute_width(pin2tlm_parms.get_u32("dram_attribute_width")),
  m_dram_attribute      (160),
  m_dram_attribute_bv   (160),
  m_start_byte_address  (pin2tlm_parms.get_u32("start_byte_address")),
  m_pif_or_idma         (is_pif_or_idma(m_interface_type)),
  m_axi_or_idma         (is_axi_or_idma(m_interface_type)),
  m_inbound_pif         (false),
  m_snoop               (false),
  m_has_coherence       (false),
  m_has_pif_attribute   (false),
  m_has_pif_req_domain  (false),
  m_axi_exclusive       (pin2tlm_parms.get_bool("axi_exclusive")),
  m_axi_exclusive_id    (pin2tlm_parms.get_bool("axi_exclusive_id")),
  m_req_user_data       (""),
  m_req_user_data_name  (""),
  m_req_user_data_width1(0),
  m_rsp_user_data       (""),
  m_rsp_user_data_name  (""),
  m_rsp_user_data_width1(0),
  m_user_data_val       (0),
  m_address_bits        (is_system_memory(m_interface_type) ? 32 : pin2tlm_parms.get_non_zero_u32("address_bits")),
  m_check_bits          (pin2tlm_parms.get_u32("check_bits")),
  m_route_id_bits       (pin2tlm_parms.get_u32("route_id_bits")),
  m_max_cycle_entries   (pin2tlm_parms.get_non_zero_u32("max_cycle_entries")),
  m_subbank_activity    (0),
  m_address             (m_address_bits),
  m_id                  (m_id_bits),
  m_priority            (m_axi_or_idma ? 4 : 2),
  m_route_id            (m_route_id_bits ? m_route_id_bits : 1),
  m_len                 (8),
  m_size                (3),
  m_burst               (2),
  m_cache               (4),
  m_prot                (3),
  m_bar                 (2),
  m_domain              (2),
  m_data                ((int)m_width8*8),
  m_req_cntl            (0, m_axi_exclusive),
  m_resp_cntl           (0),
  m_xID                 (NULL),
  m_xRESP               (5),
  m_zero_bv             (1),
  m_zero_uint           (1),
  m_text                (TextLogger::getInstance(name()))
{

  const char *axi_name_prefix = pin2tlm_parms.get_c_str("axi_name_prefix");

  m_zero_bv   = 0;
  m_zero_uint = 0;
  if (m_interface_type == PIF) {
    m_inbound_pif       = pin2tlm_parms.get_bool("inbound_pif");
    m_snoop             = pin2tlm_parms.get_bool("snoop");
    if (m_inbound_pif && m_snoop) {
      ostringstream oss;
      oss << kind() << " '" << name() << "':  You cannot have both \"inbound_pif\" and \"snoop\" set to true";
      throw xtsc_exception(oss.str());
    }
    if (!m_inbound_pif && !m_snoop) {
      m_has_coherence = pin2tlm_parms.get_bool("has_coherence");
      m_has_pif_req_domain = pin2tlm_parms.get_bool("has_pif_req_domain");
    }
    if (!m_snoop) {
      m_has_pif_attribute = pin2tlm_parms.get_bool("has_pif_attribute");
    }
  }
  else if (m_interface_type == IDMA0) {
    m_has_pif_attribute = pin2tlm_parms.get_bool("has_pif_attribute");
    m_has_pif_req_domain= pin2tlm_parms.get_bool("has_pif_req_domain");
  }
  else if (m_axi_or_idma) {
    m_axi_id_bits         = pin2tlm_parms.get_u32_vector("axi_id_bits");
    m_axi_route_id_bits   = pin2tlm_parms.get_u32_vector("axi_route_id_bits");
    m_axi_read_write      = pin2tlm_parms.get_u32_vector("axi_read_write");
    m_axi_port_type       = pin2tlm_parms.get_u32_vector("axi_port_type");
    check_and_adjust_vu32(kind(), name(), "axi_id_bits",       m_axi_id_bits,       "num_ports", m_num_ports);
    check_and_adjust_vu32(kind(), name(), "axi_route_id_bits", m_axi_route_id_bits, "num_ports", m_num_ports, true, 0);
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

  m_big_endian          = (is_amba(m_interface_type) ? false : pin2tlm_parms.get_bool("big_endian"));
  m_has_request_id      = pin2tlm_parms.get_bool("has_request_id");
  m_write_responses     = pin2tlm_parms.get_bool("write_responses");
  m_has_busy            = pin2tlm_parms.get_bool("has_busy");
  m_has_lock            = pin2tlm_parms.get_bool("has_lock");
  m_has_xfer_en         = pin2tlm_parms.get_bool("has_xfer_en");
  m_bus_addr_bits_mask  = ((m_width8 ==  4) ? 0x03 :
                           (m_width8 ==  8) ? 0x07 : 
                           (m_width8 == 16) ? 0x0F :
                           (m_width8 == 32) ? 0x1F :
                                              0x3F);
  m_cbox                = pin2tlm_parms.get_bool("cbox");
  m_split_rw            = ::get_split_rw(pin2tlm_parms);
  m_has_dma             = pin2tlm_parms.get_bool("has_dma");
  m_one_at_a_time       = pin2tlm_parms.get_bool("one_at_a_time");
  m_prereject_responses = pin2tlm_parms.get_bool("prereject_responses");
  m_dram_lock_reset     = true;

  if (!m_axi_exclusive) {
    m_axi_exclusive_id = false;
  }

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

  if (m_pif_or_idma || is_inst_mem(m_interface_type)) {
    if ((m_width8 != 4) && (m_width8 != 8) && (m_width8 != 16) && (m_width8 != 32)) {
      ostringstream oss;
      oss << kind() << " '" << name() << "': Invalid \"byte_width\"= " << m_width8
          << " (legal values for IRAM0|IRAM1|IROM0|PIF|IDMA0 are 4, 8, and 16)";
      throw xtsc_exception(oss.str());
    }
  }
  else if (m_axi_or_idma) {
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
      u32 id_bits  = m_axi_id_bits[i];
      u32 rid_bits = m_axi_route_id_bits[i];
      u32 tid_bits = rid_bits + id_bits;
      if (id_bits > 9) {
        ostringstream oss;
        oss << kind() << " '" << name() << "': Invalid \"axi_id_bits[" << i << "]\"=" << id_bits << " (legal value range is 0-8)";
        throw xtsc_exception(oss.str());
      }
      if ((tid_bits < 1) || (tid_bits > 32)) {
        ostringstream oss;
        oss << kind() << " '" << name() << "': Invalid Transaction ID bit width of " << tid_bits << " (from \"axi_id_bits[" << i << "]\"=" << id_bits
            << "plus \"axi_route_id_bits[" << i << "]\"=" << rid_bits << ") (legal Transaction ID bit width range is 1-32)";
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
      oss << kind() << " '" << name() << "': Invalid \"byte_width\"=" << m_width8 << " (the only legal value for a APB port is 4)";
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
    m_has_busy          = false;
    m_address_shift     = 0;
    m_address_mask      = 0xFFFFFFFF;
  }
  else if (m_interface_type == XLMI0) {
    if (m_size8 == 0) {
      ostringstream oss;
      oss << kind() << " '" << name() << "': \"memory_byte_size\" cannot be 0 for XLMI0";
      throw xtsc_exception(oss.str());
    }
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
    m_address_mask      = ((m_address_bits + m_address_shift) >= 32) ? 0xFFFFFFFF : (1 << (m_address_bits + m_address_shift)) - 1;
  }

  if (m_pif_or_idma) {
#if defined(__x86_64__) || defined(_M_X64)
    u32 max_bits = 64;
#else
    u32 max_bits = 32;
#endif
    parse_port_name_and_bit_width(pin2tlm_parms, "req_user_data", m_req_user_data, m_req_user_data_name, m_req_user_data_width1, 1, max_bits);
    parse_port_name_and_bit_width(pin2tlm_parms, "rsp_user_data", m_rsp_user_data, m_rsp_user_data_name, m_rsp_user_data_width1, 1, max_bits);
    if (m_rsp_user_data_width1) {
      m_user_data_val = new sc_bv_base((int)m_rsp_user_data_width1);
    }
  }

  // Get clock period 
  m_time_resolution = sc_get_time_resolution();
  u32 clock_period = pin2tlm_parms.get_u32("clock_period");
  if (clock_period == 0xFFFFFFFF) {
    m_clock_period = xtsc_get_system_clock_period();
  }
  else {
    m_clock_period = m_time_resolution * clock_period;
  }
  m_clock_period_value = m_clock_period.value();
  u32 posedge_offset = pin2tlm_parms.get_u32("posedge_offset");
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

  // Get sample phase
  u32 sample_phase = pin2tlm_parms.get_u32("sample_phase");
  m_sample_phase = sample_phase * m_time_resolution;
  m_sample_phase_plus_one = m_sample_phase + m_clock_period;

  m_output_delay = pin2tlm_parms.get_u32("output_delay") * m_time_resolution;


  // Create all the "per mem port" stuff

  m_request_ports               = new sc_port<xtsc_request_if>*         [m_num_ports];
  m_respond_exports             = new sc_export<xtsc_respond_if>*       [m_num_ports];
  m_respond_impl                = new xtsc_respond_if_impl*             [m_num_ports];
  m_debug_exports               = new sc_export<xtsc_debug_if>*         [m_num_ports];
  m_debug_impl                  = new xtsc_debug_if_impl*               [m_num_ports];

  m_request_fifo                = 0;
  m_response_fifo               = 0;
  m_response_event              = 0;
  m_pif_req_event               = 0;
  m_axi_req_fifo                = 0;
  m_axi_addr_fifo               = 0;
  m_axi_data_fifo               = 0;
  m_axi_wr_req_event            = 0;
  m_axi_send_tlm_req_event      = 0;
  m_apb_rsp_event               = 0;
  m_xID                         = 0;
  m_current_id                  = 0;
  m_drive_busy_event            = 0;
  m_previous_response_last      = 0;
  m_first_block_write           = 0;
  m_burst_write_transfer_num    = 0;
  m_first_rcw                   = 0;
  m_tag                         = 0;
  m_request_time_stamp          = 0;
  m_waiting_for_nacc            = 0;
  m_request_got_nacc            = 0;
  m_last_address                = 0;
  m_read_data_fifo              = 0;
  m_drive_read_data_event       = 0;
  m_busy_fifo                   = 0;
  m_req_rdy_fifo                = 0;
  m_drive_req_rdy_event         = 0;
  m_dram_lock                   = 0;
  m_load_address_deque          = 0;

  if (is_system_memory(m_interface_type)) {
    m_response_fifo             = new deque<xtsc_response*>             [m_num_ports];
    m_response_event            = new sc_event                          [m_num_ports];
    m_drive_req_rdy_event       = new sc_event                          [m_num_ports];
    m_req_rdy_fifo              = new bool_fifo*                        [m_num_ports];
    m_tag                       = new u64                               [m_num_ports];
    m_burst_write_transfer_num  = new u32                               [m_num_ports];
    m_request_time_stamp        = new sc_time                           [m_num_ports];
    m_waiting_for_nacc          = new bool                              [m_num_ports];
    m_request_got_nacc          = new bool                              [m_num_ports];
  }

  if (m_pif_or_idma) {
    m_request_fifo              = new deque<request_info*>              [m_num_ports];
    m_pif_req_event             = new sc_event                          [m_num_ports];
    m_current_id                = new u32                               [m_num_ports];
    m_previous_response_last    = new bool                              [m_num_ports];
    m_first_block_write         = new bool                              [m_num_ports];
    m_first_rcw                 = new bool                              [m_num_ports];
    m_last_address              = new xtsc_address                      [m_num_ports];
  }
  else if (m_axi_or_idma) {
    m_axi_req_fifo              = new deque<axi_req_info*>              [m_num_ports];
    m_axi_addr_fifo             = new deque<axi_addr_info*>             [m_num_ports];
    m_axi_data_fifo             = new deque<axi_data_info*>             [m_num_ports];
    m_axi_wr_req_event          = new sc_event                          [m_num_ports];
    m_axi_send_tlm_req_event    = new sc_event                          [m_num_ports];
    m_xID                       = new sc_uint_base*                     [m_num_ports];
  }
  else if (is_apb(m_interface_type)) {
    m_apb_rsp_event             = new sc_event                          [m_num_ports];
    m_apb_info_table            = new apb_info*                         [m_num_ports];
    for (u32 i=0; i<m_num_ports; ++i) {
      m_apb_info_table[i]       = new apb_info(*this, i);
    }
  }
  else {
    m_request_fifo              = new deque<request_info*>              [m_num_ports];
    m_read_data_fifo            = new wide_fifo*                        [m_num_ports];
    m_drive_read_data_event     = new sc_event                          [m_num_ports];
    if (((m_interface_type == DRAM0  ) ||
         (m_interface_type == DRAM0BS) ||
         (m_interface_type == DRAM0RW) ||
         (m_interface_type == DRAM1  ) ||
         (m_interface_type == DRAM1BS) ||
         (m_interface_type == DRAM1RW)) && m_has_lock)
    {
      m_dram_lock               = new bool                              [m_num_ports];
    }
    if (m_interface_type == XLMI0) {
      m_load_address_deque      = new address_deque                     [m_num_ports];
    }
    if (m_has_busy) {
      m_busy_fifo               = new bool_fifo*                        [m_num_ports];
      m_drive_busy_event        = new sc_event                          [m_num_ports];
      if (m_num_subbanks >= 2) {
        m_subbank_activity      = new subbank_activity*                 [m_num_ports/m_num_subbanks];
        for (u32 i=0; i<m_num_ports/m_num_subbanks; ++i) {
          m_subbank_activity[i] = new subbank_activity                  [m_max_cycle_entries];
        }
      }
    }
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

  if (m_pif_or_idma) {
    m_p_req_valid               = new bool_input*                       [m_num_ports];
    m_p_req_cntl                = new uint_input*                       [m_num_ports];
    m_p_req_adrs                = new uint_input*                       [m_num_ports];
    m_p_req_data                = new wide_input*                       [m_num_ports];
    m_p_req_data_be             = new uint_input*                       [m_num_ports];
    m_p_req_id                  = new uint_input*                       [m_num_ports];
    m_p_req_priority            = new uint_input*                       [m_num_ports];
    m_p_req_route_id            = new uint_input*                       [m_num_ports];
    m_p_req_attribute           = new uint_input*                       [m_num_ports];
    m_p_req_domain              = new uint_input*                       [m_num_ports];
    m_p_req_coh_vadrs           = new uint_input*                       [m_num_ports];
    m_p_req_coh_cntl            = new uint_input*                       [m_num_ports];
    m_p_req_user_data           = new wide_input*                       [m_num_ports];
    m_p_req_rdy                 = new bool_output*                      [m_num_ports];
    m_p_resp_valid              = new bool_output*                      [m_num_ports];
    m_p_resp_cntl               = new uint_output*                      [m_num_ports];
    m_p_resp_data               = new wide_output*                      [m_num_ports];
    m_p_resp_id                 = new uint_output*                      [m_num_ports];
    m_p_resp_priority           = new uint_output*                      [m_num_ports];
    m_p_resp_route_id           = new uint_output*                      [m_num_ports];
    m_p_resp_coh_cntl           = new uint_output*                      [m_num_ports];
    m_p_resp_user_data          = new wide_output*                      [m_num_ports];
    m_p_resp_rdy                = new bool_input*                       [m_num_ports];
  }
  else if (m_axi_or_idma) {
    u32 num_rd_ports = m_num_axi_rd_ports;
    u32 num_wr_ports = m_num_axi_wr_ports;
    if (num_rd_ports) {
      m_p_arid                  = new uint_input*                       [num_rd_ports];
      m_p_araddr                = new uint_input*                       [num_rd_ports];
      m_p_arlen                 = new uint_input*                       [num_rd_ports];
      m_p_arsize                = new uint_input*                       [num_rd_ports];
      m_p_arburst               = new uint_input*                       [num_rd_ports];
      m_p_arlock                = new bool_input*                       [num_rd_ports];
      m_p_arcache               = new uint_input*                       [num_rd_ports];
      m_p_arprot                = new uint_input*                       [num_rd_ports];
      m_p_arqos                 = new uint_input*                       [num_rd_ports];
      m_p_arbar                 = new uint_input*                       [num_rd_ports];
      m_p_ardomain              = new uint_input*                       [num_rd_ports];
      m_p_arsnoop               = new uint_input*                       [num_rd_ports];
      m_p_arvalid               = new bool_input*                       [num_rd_ports];
      m_p_arready               = new bool_output*                      [num_rd_ports];
      m_p_rid                   = new uint_output*                      [num_rd_ports];
      m_p_rdata                 = new wide_output*                      [num_rd_ports];
      m_p_rresp                 = new uint_output*                      [num_rd_ports];
      m_p_rlast                 = new bool_output*                      [num_rd_ports];
      m_p_rvalid                = new bool_output*                      [num_rd_ports];
      m_p_rready                = new bool_input*                       [num_rd_ports];
    }
    if (num_wr_ports) {
      m_p_awid                  = new uint_input*                       [num_wr_ports];
      m_p_awaddr                = new uint_input*                       [num_wr_ports];
      m_p_awlen                 = new uint_input*                       [num_wr_ports];
      m_p_awsize                = new uint_input*                       [num_wr_ports];
      m_p_awburst               = new uint_input*                       [num_wr_ports];
      m_p_awlock                = new bool_input*                       [num_wr_ports];
      m_p_awcache               = new uint_input*                       [num_wr_ports];
      m_p_awprot                = new uint_input*                       [num_wr_ports];
      m_p_awqos                 = new uint_input*                       [num_wr_ports];
      m_p_awbar                 = new uint_input*                       [num_wr_ports];
      m_p_awdomain              = new uint_input*                       [num_wr_ports];
      m_p_awsnoop               = new uint_input*                       [num_wr_ports];
      m_p_awvalid               = new bool_input*                       [num_wr_ports];
      m_p_awready               = new bool_output*                      [num_wr_ports];
      m_p_wdata                 = new wide_input*                       [num_wr_ports];
      m_p_wstrb                 = new uint_input*                       [num_wr_ports];
      m_p_wlast                 = new bool_input*                       [num_wr_ports];
      m_p_wvalid                = new bool_input*                       [num_wr_ports];
      m_p_wready                = new bool_output*                      [num_wr_ports];
      m_p_bid                   = new uint_output*                      [num_wr_ports];
      m_p_bresp                 = new uint_output*                      [num_wr_ports];
      m_p_bvalid                = new bool_output*                      [num_wr_ports];
      m_p_bready                = new bool_input*                       [num_wr_ports];
    }
  }
  else if (is_apb(m_interface_type)) {
    m_p_psel                    = new bool_input*                       [m_num_ports];
    m_p_penable                 = new bool_input*                       [m_num_ports];
    m_p_pwrite                  = new bool_input*                       [m_num_ports];
    m_p_paddr                   = new uint_input*                       [m_num_ports];
    m_p_pprot                   = new uint_input*                       [m_num_ports];
    m_p_pstrb                   = new uint_input*                       [m_num_ports];
    m_p_pwdata                  = new wide_input*                       [m_num_ports];
    m_p_pready                  = new bool_output*                      [m_num_ports];
    m_p_pslverr                 = new bool_output*                      [m_num_ports];
    m_p_prdata                  = new wide_output*                      [m_num_ports];
  }
  else {
    m_p_en                      = new bool_input*                       [m_num_ports];
    m_p_addr                    = new uint_input*                       [m_num_ports];
    m_p_lane                    = new uint_input*                       [m_num_ports];
    m_p_wrdata                  = new wide_input*                       [m_num_ports];
    m_p_wr                      = new bool_input*                       [m_num_ports];
    m_p_load                    = new bool_input*                       [m_num_ports];
    m_p_retire                  = new bool_input*                       [m_num_ports];
    m_p_flush                   = new bool_input*                       [m_num_ports];
    m_p_lock                    = new bool_input*                       [m_num_ports];
    m_p_attr                    = new wide_input*                       [m_num_ports];
    m_p_check_wr                = new wide_input*                       [m_num_ports];
    m_p_check                   = new wide_output*                      [m_num_ports];
    m_p_xfer_en                 = new bool_input*                       [m_num_ports];
    m_p_busy                    = new bool_output*                      [m_num_ports];
    m_p_data                    = new wide_output*                      [m_num_ports];
  }


  for (u32 port=0; port<m_num_ports; ++port) {

    ostringstream oss0;
    oss0 << "m_request_ports[" << port << "]";
    m_request_ports[port]       = new sc_port<xtsc_request_if>(oss0.str().c_str());

    ostringstream oss1;
    oss1 << "m_respond_exports[" << port << "]";
    m_respond_exports[port]     = new sc_export<xtsc_respond_if>(oss1.str().c_str());

    ostringstream oss2;
    oss2 << "m_respond_impl[" << port << "]";
    m_respond_impl[port]        = new xtsc_respond_if_impl(oss2.str().c_str(), *this, port);

    (*m_respond_exports[port])(*m_respond_impl[port]);

    ostringstream oss3;
    oss3 << "m_debug_exports[" << port << "]";
    m_debug_exports[port]       = new sc_export<xtsc_debug_if>(oss3.str().c_str());

    ostringstream oss4;
    oss4 << "m_debug_impl[" << port << "]";
    m_debug_impl[port]          = new xtsc_debug_if_impl(oss4.str().c_str(), *this, port);

    (*m_debug_exports[port])(*m_debug_impl[port]);

    if (is_apb(m_interface_type)) {
      ; // Do nothing
    }
    else if (is_system_memory(m_interface_type)) {
      ostringstream oss2;
      oss2 << "m_req_rdy_fifo[" << port << "]";
      m_req_rdy_fifo[port]      = new bool_fifo(oss2.str().c_str());
      if (m_axi_or_idma) {
        u32  tid_bits           = m_axi_id_bits[port] + m_axi_route_id_bits[port];
        m_xID[port]             = new sc_uint_base(tid_bits);
      }
    }
    else {
      sc_length_context length(m_width8*8);
      ostringstream oss1;
      oss1 << "m_read_data_fifo[" << port << "]";
      m_read_data_fifo[port]    = new wide_fifo(oss1.str().c_str());
      if (m_has_busy) {
        if (((m_num_subbanks < 2) || ((port % m_num_subbanks) == 0))) {
          ostringstream oss2;
          oss2 << "m_busy_fifo[" << port << "]";
          m_busy_fifo[port]     = new bool_fifo(oss2.str().c_str());
        }
        else {
          m_busy_fifo[port]     = NULL;
        }
      }
    }

    if (m_pif_or_idma) {
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
    else if (m_axi_or_idma) {
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
          m_p_lock          [port] = &add_bool_input ("DRam0Lock",                               m_append_id, port, true);
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
          m_p_lock          [port] = &add_bool_input ("DRam1Lock",                               m_append_id, port, true);
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
          m_p_lane          [port] = &add_uint_input ("IRam0WordEn",            m_width8/4,      m_append_id, port);
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
          m_p_lane          [port] = &add_uint_input ("IRam1WordEn",            m_width8/4,      m_append_id, port);
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
          m_p_lane          [port] = &add_uint_input ("IRom0WordEn",            m_width8/4,      m_append_id, port);
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
        string route_id(m_axi_exclusive_id ? "AXIExclID" : "PIReqRouteId");
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
        m_p_resp_priority   [port] = &add_uint_output("PORespPriority",         2,               m_append_id, port);
        m_p_resp_rdy        [port] = &add_bool_input ("PIRespRdy",                               m_append_id, port);
        if (m_has_request_id) {
          m_p_req_id        [port] = &add_uint_input ("PIReqId",                m_id_bits,       m_append_id, port);
          m_p_resp_id       [port] = &add_uint_output("PORespId",               m_id_bits,       m_append_id, port);
        }
        if (m_route_id_bits) {
          m_p_req_route_id  [port] = &add_uint_input (route_id,                 m_route_id_bits, m_append_id, port);
          if (!m_axi_exclusive_id) {
          m_p_resp_route_id [port] = &add_uint_output("PORespRouteId",          m_route_id_bits, m_append_id, port);
          }
        }
        if (m_has_pif_attribute) {
          m_p_req_attribute [port] = &add_uint_input ("PIReqAttribute",         12,              m_append_id, port);
        }
        }
        else if (m_snoop) {
        m_p_req_valid       [port] = &add_bool_input ("SnoopReqValid",                           m_append_id, port);
        m_p_req_cntl        [port] = &add_uint_input ("SnoopReqCntl",           8,               m_append_id, port);
        m_p_req_coh_cntl    [port] = &add_uint_input ("SnoopReqCohCntl",        2,               m_append_id, port);
        m_p_req_adrs        [port] = &add_uint_input ("SnoopReqAdrs",           m_address_bits,  m_append_id, port);
        m_p_req_priority    [port] = &add_uint_input ("SnoopReqPriority",       2,               m_append_id, port);
        m_p_req_coh_vadrs   [port] = &add_uint_input ("SnoopReqCohVAdrsIndex",  6,               m_append_id, port);
        m_p_req_rdy         [port] = &add_bool_output("SnoopReqRdy",                             m_append_id, port);
        m_p_resp_valid      [port] = &add_bool_output("SnoopRespValid",                          m_append_id, port);
        m_p_resp_cntl       [port] = &add_uint_output("SnoopRespCntl",          8,               m_append_id, port);
        m_p_resp_coh_cntl   [port] = &add_uint_output("SnoopRespCohCntl",       2,               m_append_id, port);
        m_p_resp_data       [port] = &add_wide_output("SnoopRespData",          m_width8*8,      m_append_id, port);
        m_p_resp_rdy        [port] = &add_bool_input ("SnoopRespRdy",                            m_append_id, port);
        if (m_has_request_id) {
          m_p_req_id        [port] = &add_uint_input ("SnoopReqId",             m_id_bits,       m_append_id, port);
          m_p_resp_id       [port] = &add_uint_output("SnoopRespId",            m_id_bits,       m_append_id, port);
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
          m_p_req_domain    [port] = &add_uint_input ("POReqDomain"       +sx,  2,               m_append_id, port);
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
        u32  tid_bits   = m_axi_id_bits[port] + m_axi_route_id_bits[port];
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
    if (m_pif_or_idma) {
      ostringstream oss1;
      oss1 << "pif_sample_pin_request_thread[" << port << "]";
      declare_thread_process(requestuest_thread_handle, oss1.str().c_str(), SC_CURRENT_USER_MODULE, pif_sample_pin_request_thread);
      m_process_handles.push_back(sc_get_current_process_handle());
      sensitive << m_p_req_valid[port]->pos() << m_p_req_rdy[port]->pos();
      ostringstream oss2;
      oss2 << "pif_drive_req_rdy_thread[" << port << "]";
      declare_thread_process(pif_respond_thread_handle, oss2.str().c_str(), SC_CURRENT_USER_MODULE, pif_drive_req_rdy_thread);
      m_process_handles.push_back(sc_get_current_process_handle());
      ostringstream oss3;
      oss3 << "pif_send_tlm_request_thread[" << port << "]";
      declare_thread_process(pif_respond_thread_handle, oss3.str().c_str(), SC_CURRENT_USER_MODULE, pif_send_tlm_request_thread);
      m_process_handles.push_back(sc_get_current_process_handle());
      ostringstream oss4;
      oss4 << "pif_drive_pin_response_thread[" << port << "]";
      declare_thread_process(pif_respond_thread_handle, oss4.str().c_str(), SC_CURRENT_USER_MODULE, pif_drive_pin_response_thread);
      m_process_handles.push_back(sc_get_current_process_handle());
    }
    else if (m_axi_or_idma) {
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
      oss3 << "axi_wr_req_thread_" << port;
      declare_thread_process(axi_wr_req_thread_handle, oss3.str().c_str(), SC_CURRENT_USER_MODULE, axi_wr_req_thread);
      m_process_handles.push_back(sc_get_current_process_handle());
      ostringstream oss4;
      oss4 << "axi_drive_req_rdy_thread_" << port;
      declare_thread_process(axi_drive_req_rdy_thread_handle, oss4.str().c_str(), SC_CURRENT_USER_MODULE, axi_drive_req_rdy_thread);
      m_process_handles.push_back(sc_get_current_process_handle());
      ostringstream oss5;
      oss5 << "axi_send_tlm_req_thread_" << port;
      declare_thread_process(axi_send_tlm_req_thread_handle, oss5.str().c_str(), SC_CURRENT_USER_MODULE, axi_send_tlm_req_thread);
      m_process_handles.push_back(sc_get_current_process_handle());
      ostringstream oss6;
      oss6 << "axi_drive_rsp_thread_" << port;
      declare_thread_process(axi_drive_rsp_thread_handle, oss6.str().c_str(), SC_CURRENT_USER_MODULE, axi_drive_rsp_thread);
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
      oss1 << "lcl_request_thread[" << port << "]";
      declare_thread_process(lcl_request_thread_handle, oss1.str().c_str(), SC_CURRENT_USER_MODULE, lcl_request_thread);
      m_process_handles.push_back(sc_get_current_process_handle());
      sensitive << m_p_en[port]->pos();
      ostringstream oss2;
      oss2 << "lcl_drive_read_data_thread[" << port << "]";
      declare_thread_process(lcl_drive_read_data_thread_handle, oss2.str().c_str(), SC_CURRENT_USER_MODULE, lcl_drive_read_data_thread);
      m_process_handles.push_back(sc_get_current_process_handle());
      if (m_interface_type == XLMI0) {
        ostringstream oss3;
        oss3 << "xlmi_load_retired_thread[" << port << "]";
        declare_thread_process(xlmi_load_retired_thread_handle, oss3.str().c_str(), SC_CURRENT_USER_MODULE, xlmi_load_retired_thread);
        m_process_handles.push_back(sc_get_current_process_handle());
        sensitive << m_p_retire[port]->pos();
        ostringstream oss4;
        oss4 << "xlmi_retire_flush_thread[" << port << "]";
        declare_thread_process(xlmi_retire_flush_thread_handle, oss4.str().c_str(), SC_CURRENT_USER_MODULE, xlmi_retire_flush_thread);
        m_process_handles.push_back(sc_get_current_process_handle());
        sensitive << m_p_flush[port]->pos();
      }
      if (m_has_busy && ((m_num_subbanks < 2) || ((port % m_num_subbanks) == 0))) {
        ostringstream oss5;
        oss5 << "lcl_drive_busy_thread[" << port << "]";
        declare_thread_process(lcl_drive_busy_thread_handle, oss5.str().c_str(), SC_CURRENT_USER_MODULE, lcl_drive_busy_thread);
        m_process_handles.push_back(sc_get_current_process_handle());
      }
    }
    else {
      // split Rd/Wr 
      bool rd_port = ((port & 0x1) == 0);
      ostringstream oss1;
      oss1 << "lcl_split_rw_request_thread_" << port;
      declare_thread_process(lcl_split_rw_request_thread_handle,
                             oss1.str().c_str(),
                             SC_CURRENT_USER_MODULE,
                             lcl_split_rw_request_thread);
      m_process_handles.push_back(sc_get_current_process_handle());
      sensitive << (rd_port ? m_p_en[port]->pos() : m_p_wr[port]->pos());
      ostringstream oss2;
      if (rd_port) {
        oss2 << "lcl_drive_read_data_thread_" << port;
        declare_thread_process(lcl_drive_read_data_thread_handle,
                               oss2.str().c_str(),
                               SC_CURRENT_USER_MODULE,
                               lcl_drive_read_data_thread);
        m_process_handles.push_back(sc_get_current_process_handle());
      }
      if (m_has_busy) {
        ostringstream oss3;
        oss3 << "lcl_drive_busy_thread_" << port;
        declare_thread_process(lcl_drive_busy_thread_handle, oss3.str().c_str(), SC_CURRENT_USER_MODULE, lcl_drive_busy_thread);
        m_process_handles.push_back(sc_get_current_process_handle());
      }
    }
  }
  // Restore SystemC
  sc_report_handler::set_actions("object already exists", SC_WARNING, original_action);

  if (((m_interface_type == DRAM0  ) ||
       (m_interface_type == DRAM0BS) ||
       (m_interface_type == DRAM0RW) ||
       (m_interface_type == DRAM1  ) ||
       (m_interface_type == DRAM1BS) ||
       (m_interface_type == DRAM1RW)) && m_has_lock)
  {
    SC_METHOD(dram_lock_method);
    for (u32 port=0; port<m_num_ports;) {
      sensitive << m_p_lock[port]->pos() << m_p_lock[port]->neg();
      // Subbanked:   Only the 0th subbank of each bank has lock
      // Split Rd/Wr: Only the Rd/even-numbered ports have lock
      port += ((m_num_subbanks >= 2) ? m_num_subbanks : m_split_rw ? 2 : 1);
    }
  }

  for (u32 i=0; i<m_num_ports; ++i) {
    ostringstream oss;
    oss.str(""); oss << "m_request_ports"   << "[" << i << "]"; m_port_types[oss.str()] = REQUEST_PORT;
    oss.str(""); oss << "m_respond_exports" << "[" << i << "]"; m_port_types[oss.str()] = RESPOND_EXPORT;
    oss.str(""); oss << "m_debug_exports"   << "[" << i << "]"; m_port_types[oss.str()] = DEBUG_EXPORT;
    oss.str(""); oss << "debug_export"      << "[" << i << "]"; m_port_types[oss.str()] = PORT_TABLE;
    oss.str(""); oss << "master_port"       << "[" << i << "]"; m_port_types[oss.str()] = PORT_TABLE;
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
  m_port_types["master_ports"]  = PORT_TABLE;
  m_port_types[m_interface_lc]  = PORT_TABLE;

  if (m_num_ports == 1) {
    m_port_types["debug_export"] = PORT_TABLE;
    m_port_types["master_port"]  = PORT_TABLE;
  }

  // Log our construction
  log4xtensa::LogLevel ll = xtsc_get_constructor_log_level();
  XTSC_LOG(m_text, ll, "Constructed " << kind() << " '" << name() << "':");
  XTSC_LOG(m_text, ll, " memory_interface       = "                 << m_interface_uc);
  if (m_interface_type == PIF) {
  XTSC_LOG(m_text, ll, " inbound_pif            = "   << boolalpha  << m_inbound_pif);
  XTSC_LOG(m_text, ll, " snoop                  = "   << boolalpha  << m_snoop);
  if (!m_snoop) {
  if (!m_inbound_pif) {
  XTSC_LOG(m_text, ll, " has_coherence          = "   << boolalpha  << m_has_coherence);
  }
  XTSC_LOG(m_text, ll, " req_user_data          = "                 << m_req_user_data);
  XTSC_LOG(m_text, ll, " rsp_user_data          = "                 << m_rsp_user_data);
  }
  }
  if (m_pif_or_idma) {
  XTSC_LOG(m_text, ll, " axi_exclusive          = "   << boolalpha  << m_axi_exclusive);
  if (m_inbound_pif && m_axi_exclusive) {
  XTSC_LOG(m_text, ll, " axi_exclusive_id       = "   << boolalpha  << m_axi_exclusive_id);
  }
  if (!m_snoop) {
  XTSC_LOG(m_text, ll, " has_pif_attribute      = "   << boolalpha  << m_has_pif_attribute);
  if (!m_inbound_pif) {
  XTSC_LOG(m_text, ll, " has_pif_req_domain     = "   << boolalpha  << m_has_pif_req_domain);
  }
  }
  }
  XTSC_LOG(m_text, ll, " num_ports              = "   << dec        << m_num_ports);
  XTSC_LOG(m_text, ll, " banked                 = "   << boolalpha  << m_banked);
  XTSC_LOG(m_text, ll, " num_subbanks           = "   << dec        << m_num_subbanks);
  XTSC_LOG(m_text, ll, " port_name_suffix       = "                 << m_suffix);
  XTSC_LOG(m_text, ll, " start_byte_address     = 0x" << hex        << setfill('0') << setw(8) << m_start_byte_address);
  if (m_interface_type == XLMI0) {
  XTSC_LOG(m_text, ll, " memory_byte_size       = 0x" << hex        << m_size8);
  }
  XTSC_LOG(m_text, ll, " byte_width             = "                 << m_width8);
  if (is_amba(m_interface_type)) {
  XTSC_LOG(m_text, ll, " axi_name_prefix        = "                 << axi_name_prefix);
  }
  if (m_axi_or_idma) {
  log_vu32(m_text, ll, " axi_id_bits",                                 m_axi_id_bits,       24);
  log_vu32(m_text, ll, " axi_route_id_bits",                           m_axi_route_id_bits, 24);
  log_vu32(m_text, ll, " axi_read_write",                              m_axi_read_write,    24);
  log_vu32(m_text, ll, " axi_port_type",                               m_axi_port_type,     24);
  }
  if (!m_axi_or_idma) {
  XTSC_LOG(m_text, ll, " big_endian             = "   << boolalpha  << m_big_endian);
  }
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
  XTSC_LOG(m_text, ll, " sample_phase           = "                 << sample_phase << " (" << m_sample_phase << ")");
  if (m_pif_or_idma) {
  XTSC_LOG(m_text, ll, " has_request_id         = "   << boolalpha  << m_has_request_id);
  XTSC_LOG(m_text, ll, " write_responses        = "   << boolalpha  << m_write_responses);
  XTSC_LOG(m_text, ll, " route_id_bits          = "   << dec        << m_route_id_bits);
  }
  if (is_system_memory(m_interface_type) && !is_apb(m_interface_type)) {
  XTSC_LOG(m_text, ll, " one_at_a_time          = "   << boolalpha  << m_one_at_a_time);
  XTSC_LOG(m_text, ll, " prereject_responses    = "   << boolalpha  << m_prereject_responses);
  }
  if (!is_system_memory(m_interface_type)) {
  XTSC_LOG(m_text, ll, " address_bits           = "   << dec        << m_address_bits);
  XTSC_LOG(m_text, ll, " has_busy               = "   << boolalpha  << m_has_busy);
  }
  XTSC_LOG(m_text, ll, " output_delay           = "                 << m_output_delay);
  if ((m_interface_type == DRAM0) || (m_interface_type == DRAM0BS) || (m_interface_type == DRAM0RW) ||
      (m_interface_type == DRAM1) || (m_interface_type == DRAM1BS) || (m_interface_type == DRAM1RW))
  {
  XTSC_LOG(m_text, ll, " has_lock               = "   << boolalpha  << m_has_lock);
  }
  if ((m_interface_type == DRAM0) || (m_interface_type == DRAM0BS) || (m_interface_type == DRAM0RW) ||
      (m_interface_type == DRAM1) || (m_interface_type == DRAM1BS) || (m_interface_type == DRAM1RW) ||
      (m_interface_type == IRAM0) || (m_interface_type == IRAM1  ))
  {
  XTSC_LOG(m_text, ll, " check_bits             = "                 << m_check_bits);
  }
  if ((m_interface_type != DROM0) && (m_interface_type != XLMI0) && !m_pif_or_idma) {
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



xtsc_component::xtsc_pin2tlm_memory_transactor::~xtsc_pin2tlm_memory_transactor(void) {
  // Do any required clean-up here
}



u32 xtsc_component::xtsc_pin2tlm_memory_transactor::get_bit_width(const string& port_name, u32 interface_num) const {
  string name_portion;
  u32    index;
  xtsc_parse_port_name(port_name, name_portion, index);

  if ((name_portion == "m_request_ports") || (name_portion == "m_respond_exports") || (name_portion == "m_debug_exports")) {
    return m_width8 * 8;
  }

  return xtsc_module_pin_base::get_bit_width(port_name);
}



sc_object *xtsc_component::xtsc_pin2tlm_memory_transactor::get_port(const string& port_name) {
  if (has_bool_input (port_name)) return &get_bool_input (port_name);
  if (has_uint_input (port_name)) return &get_uint_input (port_name);
  if (has_wide_input (port_name)) return &get_wide_input (port_name);
  if (has_bool_output(port_name)) return &get_bool_output(port_name);
  if (has_uint_output(port_name)) return &get_uint_output(port_name);
  if (has_wide_output(port_name)) return &get_wide_output(port_name);

  string name_portion;
  u32    index;
  bool   indexed = xtsc_parse_port_name(port_name, name_portion, index);

  if (((m_num_ports > 1) && !indexed) ||
      ((name_portion != "m_request_ports") && (name_portion != "m_respond_exports") && (name_portion != "m_debug_exports")))
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

       if (name_portion == "m_request_ports"  ) return (sc_object*) m_request_ports  [index];
  else if (name_portion == "m_respond_exports") return (sc_object*) m_respond_exports[index];
  else                                          return (sc_object*) m_debug_exports  [index];
}



xtsc_port_table xtsc_component::xtsc_pin2tlm_memory_transactor::get_port_table(const string& port_table_name) const {
  xtsc_port_table table;

  if (port_table_name == "debug_exports") {
    for (u32 i=0; i<m_num_ports; ++i) {
      ostringstream oss;
      oss << "debug_export[" << i << "]";
      table.push_back(oss.str());
    }
    return table;
  }
  
  if (port_table_name == "master_ports") {
    for (u32 i=0; i<m_num_ports; ++i) {
      ostringstream oss;
      oss << "master_port[" << i << "]";
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

  if (((m_num_ports > 1) && !indexed) ||
      ((name_portion != "master_port") && (name_portion != "debug_export") && (name_portion != m_interface_lc)))
  {
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

  if (name_portion == "master_port") {
    oss.str(""); oss << "m_request_ports"   << "[" << index << "]"; table.push_back(oss.str());
    oss.str(""); oss << "m_respond_exports" << "[" << index << "]"; table.push_back(oss.str());
  }
  else if (name_portion == "debug_export") {
    oss.str(""); oss << "m_debug_exports"   << "[" << index << "]"; table.push_back(oss.str());
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



string xtsc_component::xtsc_pin2tlm_memory_transactor::adjust_name_and_check_size(
 const string&                               port_name,
 const xtsc_tlm2pin_memory_transactor&       tlm2pin,
 u32                                         tlm2pin_port,
 const set_string&                           transactor_set
) const
{
  string tlm2pin_port_name(port_name);
  if (m_append_id) {
    tlm2pin_port_name.erase(tlm2pin_port_name.size()-1);
  }
  if (tlm2pin.get_append_id()) {
    ostringstream oss;
    oss << tlm2pin_port;
    tlm2pin_port_name += oss.str();
  }
  u32 pin2tlm_width = get_bit_width(port_name);
  u32 tlm2pin_width = tlm2pin.get_bit_width(tlm2pin_port_name);
  if (pin2tlm_width != tlm2pin_width) {
    ostringstream oss;
    oss << "Signal mismatch in connect():  " << kind() << " '" << name() << "' has a port named \"" << port_name
        << "\" with a width of " << pin2tlm_width << " bits, but port \"" << tlm2pin_port_name << "\" in " << tlm2pin.kind()
        << " '" << tlm2pin.name() << "' has " << tlm2pin_width << " bits.";
    throw xtsc_exception(oss.str());
  }
  return tlm2pin_port_name;
} 



void xtsc_component::xtsc_pin2tlm_memory_transactor::dump_set_string(ostringstream&     oss,
                                                                     const set_string&  strings,
                                                                     const string&      indent)
{
  for (set_string::const_iterator is = strings.begin(); is != strings.end(); ++is) {
    oss << indent << *is << endl;
  }
}



void xtsc_component::xtsc_pin2tlm_memory_transactor::connect(xtsc_core& core, u32 pin2tlm_port) {
  if (m_interface_type != PIF) {
    ostringstream oss;
    oss << kind() << " '" << name() << "' has a " << get_interface_name()
        << " interface which cannot be connected to the inbound PIF/snoop interface of an xtsc_core";
    throw xtsc_exception(oss.str());
  }
  if (!m_inbound_pif && !m_snoop) {
    ostringstream oss;
    oss << kind() << " '" << name()
        << "' has a outbound PIF interface which cannot be connected to the inbound PIF/snoop interface of an xtsc_core";
    throw xtsc_exception(oss.str());
  }
  if (pin2tlm_port >= m_num_ports) {
    ostringstream oss;
    oss << "Invalid pin2tlm_port=" << pin2tlm_port << " in connect(): " << endl;
    oss << kind() << " '" << name() << "' has " << m_num_ports << " ports numbered from 0 to " << m_num_ports-1 << endl;
    throw xtsc_exception(oss.str());
  }
  u32 width8 = core.get_memory_byte_width(xtsc_core::get_memory_port("PIF", &core));
  if (width8 != m_width8) {
    ostringstream oss;
    oss << "Memory interface data width mismatch: " << endl;
    oss << kind() << " '" << name() << "' is " << m_width8 << " bytes wide, but the inbound PIF/snoop interface of" << endl;
    oss << core.kind() << " '" << core.name() << "' is " << width8 << " bytes wide.";
    throw xtsc_exception(oss.str());
  }
  if (m_snoop) {
    core.get_respond_port("snoop")(*m_respond_exports[pin2tlm_port]);
    (*m_request_ports[pin2tlm_port])(core.get_request_export("snoop"));
  }
  else {
    core.get_respond_port("inbound_pif")(*m_respond_exports[pin2tlm_port]);
    (*m_request_ports[pin2tlm_port])(core.get_request_export("inbound_pif"));
  }
}



u32 xtsc_component::xtsc_pin2tlm_memory_transactor::connect(xtsc_tlm2pin_memory_transactor&     tlm2pin,
                                                            u32                                 tlm2pin_port,
                                                            u32                                 pin2tlm_port,
                                                            bool                                single_connect)
{
  u32 tlm2pin_ports = tlm2pin.get_num_ports();
  if (tlm2pin_port >= tlm2pin_ports) {
    ostringstream oss;
    oss << "Invalid tlm2pin_port=" << tlm2pin_port << " in connect(): " << endl;
    oss << kind() << " '" << name() << "' has " << tlm2pin_ports << " ports numbered from 0 to " << tlm2pin_ports-1 << endl;
    throw xtsc_exception(oss.str());
  }
  if (pin2tlm_port >= m_num_ports) {
    ostringstream oss;
    oss << "Invalid pin2tlm_port=" << pin2tlm_port << " in connect(): " << endl;
    oss << kind() << " '" << name() << "' has " << m_num_ports << " ports numbered from 0 to " << m_num_ports-1 << endl;
    throw xtsc_exception(oss.str());
  }
  if (m_interface_type != tlm2pin.get_interface_type()) {
    ostringstream oss;
    oss << "Interface type mismatch in connect(): " << kind() << " '" << name() << "' has a " << get_interface_name() << endl;
    oss << " interface but the upstream device, " << tlm2pin.kind() << " '" << tlm2pin.name() << "', has a "
        << tlm2pin.get_interface_name() << " interface.";
    throw xtsc_exception(oss.str());
  }

  u32 num_connected = 0;

  while ((tlm2pin_port < tlm2pin_ports) && (pin2tlm_port < m_num_ports)) {

    {
      // Connect sc_in<bool> ports of xtsc_pin2tlm_memory_transactor to sc_out<bool> ports of xtsc_tlm2pin_memory_transactor 
      set_string mem_set = get_bool_input_set(pin2tlm_port);
      set_string tran_set = tlm2pin.get_bool_output_set(tlm2pin_port);
      if (mem_set.size() != tran_set.size()) {
        ostringstream oss;
        oss << "Signal set sizes don't match in xtsc_pin2tlm_memory_transactor::connect():" << endl;
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
      // Connect sc_in<sc_uint_base> ports of xtsc_pin2tlm_memory_transactor to sc_out<sc_uint_base> ports of
      // xtsc_tlm2pin_memory_transactor 
      set_string mem_set = get_uint_input_set(pin2tlm_port);
      set_string tran_set = tlm2pin.get_uint_output_set(tlm2pin_port);
      if (mem_set.size() != tran_set.size()) {
        ostringstream oss;
        oss << "Signal set sizes don't match in xtsc_pin2tlm_memory_transactor::connect():" << endl;
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
      // Connect sc_in<sc_bv_base> ports of xtsc_pin2tlm_memory_transactor to sc_out<sc_bv_base> ports of
      // xtsc_tlm2pin_memory_transactor 
      set_string mem_set = get_wide_input_set(pin2tlm_port);
      set_string tran_set = tlm2pin.get_wide_output_set(tlm2pin_port);
      if (mem_set.size() != tran_set.size()) {
        ostringstream oss;
        oss << "Signal set sizes don't match in xtsc_pin2tlm_memory_transactor::connect():" << endl;
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
      // Connect sc_out<bool> ports of xtsc_pin2tlm_memory_transactor to sc_in<bool> ports of xtsc_tlm2pin_memory_transactor 
      set_string mem_set = get_bool_output_set(pin2tlm_port);
      set_string tran_set = tlm2pin.get_bool_input_set(tlm2pin_port);
      if (mem_set.size() != tran_set.size()) {
        ostringstream oss;
        oss << "Signal set sizes don't match in xtsc_pin2tlm_memory_transactor::connect():" << endl;
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
      // Connect sc_out<sc_uint_base> ports of xtsc_pin2tlm_memory_transactor to sc_in<sc_uint_base> ports of
      // xtsc_tlm2pin_memory_transactor 
      set_string mem_set = get_uint_output_set(pin2tlm_port);
      set_string tran_set = tlm2pin.get_uint_input_set(tlm2pin_port);
      if (mem_set.size() != tran_set.size()) {
        ostringstream oss;
        oss << "Signal set sizes don't match in xtsc_pin2tlm_memory_transactor::connect():" << endl;
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
      // Connect sc_out<sc_bv_base> ports of xtsc_pin2tlm_memory_transactor to sc_in<sc_bv_base> ports of
      // xtsc_tlm2pin_memory_transactor 
      set_string mem_set = get_wide_output_set(pin2tlm_port);
      set_string tran_set = tlm2pin.get_wide_input_set(tlm2pin_port);
      if (mem_set.size() != tran_set.size()) {
        ostringstream oss;
        oss << "Signal set sizes don't match in xtsc_pin2tlm_memory_transactor::connect():" << endl;
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
    (*tlm2pin.m_debug_ports[tlm2pin_port])(*m_debug_exports[pin2tlm_port]);

    pin2tlm_port += 1;
    tlm2pin_port += 1;
    num_connected += 1;

    if (single_connect) break;
    if (tlm2pin_port >= tlm2pin_ports) break;
    if (pin2tlm_port >= m_num_ports) break;
    if (m_debug_impl[pin2tlm_port]->is_connected()) break;
  }

  return num_connected;
}



void xtsc_component::xtsc_pin2tlm_memory_transactor::end_of_elaboration(void) {
} 



void xtsc_component::xtsc_pin2tlm_memory_transactor::start_of_simulation(void) {
  reset();
} 



void xtsc_component::xtsc_pin2tlm_memory_transactor::reset(bool /* hard_reset */) {
  XTSC_INFO(m_text, kind() << "::reset()");

  m_next_port_pif_sample_pin_request_thread     = 0;
  m_next_port_pif_drive_req_rdy_thread          = 0;
  m_next_port_pif_send_tlm_request_thread       = 0;
  m_next_port_pif_drive_pin_response_thread     = 0;
  m_next_port_axi_req_addr_thread               = 0;
  m_next_port_axi_req_data_thread               = 0;
  m_next_port_axi_wr_req_thread                 = 0;
  m_next_port_axi_drive_req_rdy_thread          = 0;
  m_next_port_axi_send_tlm_req_thread           = 0;
  m_next_port_axi_drive_rsp_thread              = 0;
  m_next_port_lcl_request_thread                = 0;
  m_next_port_lcl_drive_read_data_thread        = 0;
  m_next_port_lcl_drive_busy_thread             = 0;
  m_next_port_xlmi_load_retired_thread          = 0;
  m_next_port_xlmi_retire_flush_thread          = 0;

  for (u32 port=0; port<m_num_ports; ++port) {
    if (is_system_memory(m_interface_type)) {
      m_burst_write_transfer_num [port] = 1;
      m_tag                      [port] = 0;
      m_waiting_for_nacc         [port] = false;
      m_request_got_nacc         [port] = false;
    }
    if (m_pif_or_idma) {
      m_first_block_write        [port] = true;
      m_first_rcw                [port] = true;
    }
    else if (((m_interface_type == DRAM0  ) ||
              (m_interface_type == DRAM0BS) ||
              (m_interface_type == DRAM0RW) ||
              (m_interface_type == DRAM1  ) ||
              (m_interface_type == DRAM1BS) ||
              (m_interface_type == DRAM1RW)) && m_has_lock)
    {
      m_dram_lock[port] = false;
    }
  }

  m_dram_lock_reset     = true;

  // Cancel notified events
  for (u32 port=0; port<m_num_ports; ++port) {
    if (is_system_memory(m_interface_type)) {
      m_drive_req_rdy_event   [port].cancel();
      m_response_event        [port].cancel();
    }
    if (m_pif_or_idma) {
      m_pif_req_event         [port].cancel();
    }
    else if (m_axi_or_idma) {
      m_axi_wr_req_event      [port].cancel();
      m_axi_send_tlm_req_event[port].cancel();
    }
    else if (is_apb(m_interface_type)) {
      m_apb_rsp_event         [port].cancel();
    }
    else {
      m_drive_read_data_event [port].cancel();
      if (m_has_busy) {
        m_drive_busy_event    [port].cancel();
      }
    }
  }

  // Clear deque's
  for (u32 port=0; port<m_num_ports; ++port) {
    if (m_pif_or_idma) {
      m_request_fifo[port].clear();
      while (!m_response_fifo[port].empty()) {
        xtsc_response *p_response  = m_response_fifo[port].front();
        m_response_fifo[port].pop_front();
        delete_response(p_response);
      }
    }
    else if (m_axi_or_idma) {
      m_axi_req_fifo [port].clear();
      m_axi_addr_fifo[port].clear();
      m_axi_data_fifo[port].clear();
    }
    else if (is_apb(m_interface_type)) {
      ; // Do nothing
    }
    else {
      m_request_fifo[port].clear();
    }
    if (m_interface_type == XLMI0) {
      m_load_address_deque[port].clear();
    }
  }

  if (m_has_busy && (m_num_subbanks >= 2)) {
    for (u32 i=0; i<m_num_ports/m_num_subbanks; ++i) {
      for (u32 j=0; j<m_max_cycle_entries; ++j) {
        initialize_subbank_activity(&m_subbank_activity[i][j]);
      }
    }
  }

  xtsc_reset_processes(m_process_handles);
}



void xtsc_component::xtsc_pin2tlm_memory_transactor::get_read_data_from_response(const xtsc_response& response) {
  bool big_endian = m_big_endian && !m_axi_or_idma;
  m_data = 0;
  u32 size = response.get_byte_size();
  if (size) {
    u32           transfer_num    = response.get_transfer_number();
    xtsc_address  address8        = response.get_byte_address();
    if (m_axi_or_idma) {
      burst_t     burst           = response.get_burst_type();
      if (burst != xtsc_request::FIXED) {
        u32          adjustment  = ((transfer_num - 1) * size);
        xtsc_address adj_address = address8 + adjustment;
        if (burst == xtsc_request::WRAP) {
          u32          num_transfers  = response.get_num_transfers();
          u32          total_size8    = size * num_transfers;
          u32          initial_offset = address8 % total_size8;
          xtsc_address low_address    = address8 - initial_offset;
          xtsc_address wrap_address   = low_address + total_size8;
          if (adj_address >= wrap_address) {
            adj_address -= total_size8;
          }
          XTSC_DEBUG(m_text, "get_read_data_from_response() address8=0x" << hex << address8 << " low_address=0x" << low_address <<
                     " wrap_address=0x" << wrap_address << " adj_address=0x" << adj_address << dec <<
                     " adjustment=" << adjustment << " total_size8=" << total_size8 << " initial_offset=" << initial_offset);
        }
        address8 = adj_address;
      }
    }
    u32           bus_addr_bytes  = address8 & (m_width8 - 1);
    u32           bit_offset      = (big_endian ? (m_width8 - 1 - bus_addr_bytes) : bus_addr_bytes) * 8;
    u32           delta           = (big_endian ? -8 : 8);
    const u8     *buffer          = response.get_buffer();
    u32           max_offset      = (m_width8 - 1) * 8; // Prevent SystemC "E5 out of bounds" - (should be an Aerr response)
    for (u32 i=0; (i<size) && (bit_offset<=max_offset); ++i) {
      m_data.range(bit_offset+7, bit_offset) = buffer[i];
      bit_offset += delta;
    }
  }
  XTSC_DEBUG(m_text, "get_read_data_from_response: size=" << size << " m_data=0x" << hex << m_data);
}



void xtsc_component::xtsc_pin2tlm_memory_transactor::sync_to_sample_phase(void) {
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
  else  {
    wait(m_sample_phase_plus_one - phase_now);
  }
} 



void xtsc_component::xtsc_pin2tlm_memory_transactor::lcl_request_thread(void) {

  // Get the port number for this "instance" of the thread
  u32  port      = m_next_port_lcl_request_thread++;
  bool subbanked = (m_num_subbanks >= 2);
  u32  bank      = (subbanked ? (port / m_num_subbanks) : port);

  // A try/catch block in sc_main will not catch an exception thrown from
  // an SC_THREAD, so we'll catch them here, log them, then rethrow them.
  try {

    XTSC_INFO(m_text, "in lcl_request_thread[" << port << "]");

    // If present, drive 0 on parity/ECC signal
    if (m_p_check[port]) {
      sc_bv_base dummy((int)m_check_bits);
      dummy = 0;
      m_p_check[port]->write(dummy);
    }

    // Loop forever
    while (true) {

      // Wait for enable to go high: m_p_en[port]->pos()
      wait();

      XTSC_DEBUG(m_text, __FUNCTION__ << "(): awoke from wait()");

      // Sync to sample phase
      sync_to_sample_phase();

      XTSC_DEBUG(m_text, __FUNCTION__ << "(): completed sync_to_sample_phase()");

      // Capture a request once each clock period that enable is high
      while (m_p_en[port]->read()) {

        request_info *p_info = new_request_info(port);

        XTSC_INFO(m_text, p_info->m_request);

        if (subbanked) {
          subbank_activity *p_sa = &m_subbank_activity[bank][p_info->m_cycle_index];
          if (p_info->m_cycle_num == p_sa->m_cycle_num) {
            p_sa->m_num_req += 1;
          }
          else {
            if (p_sa->m_num_rsp_ok + p_sa->m_num_rsp_nacc != p_sa->m_num_req) {
              ostringstream oss;
              oss << kind() << " '" << name() << "': m_subbank_activity[" << bank << "][" << p_info->m_cycle_index << "] had " << p_sa->m_num_req 
                  << " requests at m_cycle_num=" << p_sa->m_cycle_num << " which subsequently received " << (p_sa->m_num_rsp_ok + p_sa->m_num_rsp_nacc)
                  << " responses (" << p_sa->m_num_rsp_ok << " RSP_OK and " << p_sa->m_num_rsp_nacc
                  << " RSP_NACC).  Possibly downstream memory latency is too large so \"max_cycle_entries\" needs to be increased.";
              throw xtsc_exception(oss.str());
            }
            initialize_subbank_activity(p_sa);
            p_sa->m_cycle_num = p_info->m_cycle_num;
            p_sa->m_num_req = 1;
          }
        }

        m_request_fifo[port].push_back(p_info);
        (*m_request_ports[port])->nb_request(p_info->m_request);

        wait(m_clock_period);

      }

      XTSC_DEBUG(m_text, __FUNCTION__ << "(): enable/valid is low");

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



void xtsc_component::xtsc_pin2tlm_memory_transactor::lcl_split_rw_request_thread(void) {

  // Get the port number for this "instance" of the thread
  u32 port = m_next_port_lcl_request_thread++;

  bool rd_port = ((port & 0x1) == 0);

  // A try/catch block in sc_main will not catch an exception thrown from
  // an SC_THREAD, so we'll catch them here, log them, then rethrow them.
  try {

    XTSC_INFO(m_text, "in lcl_split_rw_request_thread[" << port << "]");

    // If present, drive 0 on parity/ECC signal
    if (m_p_check[port]) {
      sc_bv_base dummy((int)m_check_bits);
      dummy = 0;
      m_p_check[port]->write(dummy);
    }

    // Loop forever
    while (true) {

      // Wait for enable to go high: m_p_en[port]->pos() or m_p_wr[port]->pos()
      wait();

      XTSC_DEBUG(m_text, __FUNCTION__ << "(): awoke from wait()");

      // Sync to sample phase
      sync_to_sample_phase();

      XTSC_DEBUG(m_text, __FUNCTION__ << "(): completed sync_to_sample_phase()");

      // Capture a request once each clock period that enable is high
      while (rd_port ?  m_p_en[port]->read() : m_p_wr[port]->read()) {

        request_info *p_info = new_request_info(port);

        XTSC_INFO(m_text, p_info->m_request);

        m_request_fifo[port].push_back(p_info);
        (*m_request_ports[port])->nb_request(p_info->m_request);

        wait(m_clock_period);

      }

      XTSC_DEBUG(m_text, __FUNCTION__ << "(): enable/valid is low");

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



void xtsc_component::xtsc_pin2tlm_memory_transactor::lcl_drive_read_data_thread(void) {

  // Get the port number for this "instance" of the thread
  u32 port = m_next_port_lcl_drive_read_data_thread++;
  if (m_split_rw) {
    // Only the even port numbers are Rd ports when using a split Rd/Wr interface
    m_next_port_lcl_drive_read_data_thread += 1;
  }

  // Empty fifo's
  while (m_read_data_fifo[port]->nb_read(m_data));

  try {

    XTSC_INFO(m_text, "in lcl_drive_read_data_thread[" << port << "]");

    // Loop forever
    while (true) {
      // Wait to be notified
      wait(m_drive_read_data_event[port]);
      while (m_read_data_fifo[port]->num_available()) {
        m_read_data_fifo[port]->nb_read(m_data);
        m_p_data[port]->write(m_data);
        XTSC_DEBUG(m_text, __FUNCTION__ << "[" << port << "] driving read data 0x" << hex << m_data);
        wait(m_clock_period);
        if (!m_read_data_fifo[port]->num_available()) {
          m_data = 0;
          m_p_data[port]->write(m_data);
        }
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



void xtsc_component::xtsc_pin2tlm_memory_transactor::lcl_drive_busy_thread(void) {

  // Get the port number for this "instance" of the thread
  u32 port = m_next_port_lcl_drive_busy_thread;
  m_next_port_lcl_drive_busy_thread += ((m_num_subbanks < 2) ? 1 : m_num_subbanks);

  bool dummy = false;

  // Empty fifo's
  while (m_busy_fifo[port]->nb_read(dummy));

  m_p_busy[port]->write(false);

  try {

    XTSC_INFO(m_text, "in lcl_drive_busy_thread[" << port << "]");

    // Loop forever
    while (true) {
      // Wait to be notified
      wait(m_drive_busy_event[port]);
      while (m_busy_fifo[port]->num_available()) {
        m_busy_fifo[port]->nb_read(dummy);
        m_p_busy[port]->write(true);
        XTSC_DEBUG(m_text, __FUNCTION__ << "[" << port << "] asserting busy");
        wait(m_clock_period);
        if (!m_busy_fifo[port]->num_available()) {
          m_p_busy[port]->write(false);
          XTSC_DEBUG(m_text, __FUNCTION__ << "[" << port << "] deasserting busy");
        }
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



void xtsc_component::xtsc_pin2tlm_memory_transactor::xlmi_load_retired_thread() {

  // Get the port number for this "instance" of the thread
  u32 port = m_next_port_xlmi_load_retired_thread++;

  try {

    XTSC_INFO(m_text, "in xlmi_load_retired_thread[" << port << "]");

    // Loop forever
    while (true) {

      // Wait for DPortLoadRetired to go high: m_p_retire[port]->pos()
      wait();

      // Sync to sample phase
      sync_to_sample_phase();

      // Call nb_load_retired() once each clock period while DPort0LoadRetiredm is high
      while (m_p_retire[port]->read()) {
        if (m_load_address_deque[port].empty()) {
          ostringstream oss;
          oss << "XLMI Port #" << port << ": the interface signaled load retired, but there are no outstanding loads";
          throw xtsc_exception(oss.str());
        }
        xtsc_address address = m_load_address_deque[port].front();
        m_load_address_deque[port].pop_front();
        (*m_request_ports[port])->nb_load_retired(address);
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



void xtsc_component::xtsc_pin2tlm_memory_transactor::xlmi_retire_flush_thread() {

  // Get the port number for this "instance" of the thread
  u32 port = m_next_port_xlmi_retire_flush_thread++;

  try {

    XTSC_INFO(m_text, "in xlmi_retire_flush_thread[" << port << "]");

    // Loop forever
    while (true) {

      // Wait for DPortRetireFlush to go high: m_p_flush[port]->pos()
      wait();

      // Sync to sample phase
      sync_to_sample_phase();

      // Call nb_retire_flush() once each clock period while DPort0RetireFlushm is high
      while (m_p_flush[port]->read()) {
        if (m_load_address_deque[port].empty()) {
          ostringstream oss;
          oss << "XLMI Port #" << port << ": the interface signaled retire flush, but there are no outstanding loads";
          throw xtsc_exception(oss.str());
        }
        m_load_address_deque[port].clear();
        (*m_request_ports[port])->nb_retire_flush();
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



void xtsc_component::xtsc_pin2tlm_memory_transactor::dram_lock_method() {
  // Subbanked:   Only the 0th subbank of each bank has lock
  // Split Rd/Wr: Only the Rd/even-numbered ports have lock
  u32 inc = ((m_num_subbanks >= 2) ? m_num_subbanks : m_split_rw ? 2 : 1);
  try {
    for (u32 port=0; port<m_num_ports; port+=inc) {
      bool lock = (*m_p_lock[port])->read();
      if (m_dram_lock_reset || (lock != m_dram_lock[port])) {
        (*m_request_ports[port])->nb_lock(lock);
        m_dram_lock[port] = lock;
      }
    }
    m_dram_lock_reset = false;
  }
  catch (const sc_unwind_exception& error) { throw; }
  catch (const exception& error) {
    ostringstream oss;
    oss << "std::exception caught in " << __FUNCTION__ << "(), an SC_METHOD of " << kind() << " '" << name() << "'." << endl;
    oss << "what(): " << error.what() << endl;
    oss << __FUNCTION__ << " now rethrowing:";
    xtsc_log_multiline(m_text, FATAL_LOG_LEVEL, oss.str(), 2);
    throw;
  }
}



void xtsc_component::xtsc_pin2tlm_memory_transactor::pif_sample_pin_request_thread(void) {

  // Get the port number for this "instance" of the thread
  u32 port = m_next_port_pif_sample_pin_request_thread++;

  try {

    XTSC_INFO(m_text, "in pif_sample_pin_request_thread[" << port << "]");

    // Loop forever
    while (true) {

      do {
        XTSC_DEBUG(m_text, __FUNCTION__ << "(): Waiting for POReqValid|PIReqRdy");
        wait();  // m_p_req_valid[port]->pos()  /  m_p_req_rdy[port]->pos()
        XTSC_DEBUG(m_text, __FUNCTION__ << "(): Got POReqValid|PIReqRdy");
      } while (!m_p_req_valid[port]->read() || !m_p_req_rdy[port]->read());

      // Sync to sample phase
      sync_to_sample_phase();
      XTSC_DEBUG(m_text, __FUNCTION__ << "(): completed sync_to_sample_phase()");

      // Capture one request per clock period while both POReqValid and PIReqRdy are asserted
      while (m_p_req_valid[port]->read() && m_p_req_rdy[port]->read()) {
        request_info *p_info = new_request_info(port);
        m_request_fifo[port].push_back(p_info);
        m_pif_req_event[port].notify(SC_ZERO_TIME);
        XTSC_INFO(m_text, *p_info);
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



void xtsc_component::xtsc_pin2tlm_memory_transactor::pif_drive_req_rdy_thread(void) {

  // Get the port number for this "instance" of the thread
  u32 port = m_next_port_pif_drive_req_rdy_thread++;

  bool wait_for_response = false;

  // Empty fifo's
  while (m_req_rdy_fifo[port]->nb_read(wait_for_response));

  try {

    XTSC_INFO(m_text, "in pif_drive_req_rdy_thread[" << port << "]");

    m_p_req_rdy[port]->write(true);

    // Loop forever
    while (true) {
      // Wait to be notified
      wait(m_drive_req_rdy_event[port]);
      while (m_req_rdy_fifo[port]->num_available()) {
        m_req_rdy_fifo[port]->nb_read(wait_for_response);
        m_p_req_rdy[port]->write(false);
        XTSC_DEBUG(m_text, __FUNCTION__ << "[" << port << "] deasserting PIReqRdy");
        if (wait_for_response) {
          // true means wait until event is notified again
          wait(m_drive_req_rdy_event[port]);
          bool dummy = false;
          m_req_rdy_fifo[port]->nb_read(dummy); // dummy should be false, but we don't check it
        }
        else {
          // false means wait one clock period
          wait(m_clock_period);
        }
        m_p_req_rdy[port]->write(true);
        XTSC_DEBUG(m_text, __FUNCTION__ << "[" << port << "] asserting PIReqRdy");
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



void xtsc_component::xtsc_pin2tlm_memory_transactor::pif_send_tlm_request_thread(void) {

  // Get the port number for this "instance" of the thread
  u32 port = m_next_port_pif_send_tlm_request_thread++;

  try {

    XTSC_INFO(m_text, "in pif_send_tlm_request_thread[" << port << "]");

    // Loop forever
    while (true) {

      // Wait for a request
      wait(m_pif_req_event[port]);
      XTSC_DEBUG(m_text, __FUNCTION__ << "(): awoke from wait()");

      // Process up to one request at a time while there are requests available.
      while (!m_request_fifo[port].empty()) {
        request_info *p_info = m_request_fifo[port].front();
        m_request_fifo[port].pop_front();
        if (m_one_at_a_time && p_info->m_request.get_last_transfer()) {
          m_req_rdy_fifo[port]->write(true);
          m_drive_req_rdy_event[port].notify(m_output_delay);
          XTSC_INFO(m_text, p_info->m_request << ": scheduled PIReqRdy deassert upon last-transfer request");
          m_request_time_stamp[port] = sc_time_stamp();
        }
        u32 tries = 0;
        do {
          m_request_got_nacc[port] = false;
          tries += 1;
          XTSC_VERBOSE(m_text, p_info->m_request << " Port #" << port << " Try #" << tries);
          m_waiting_for_nacc[port] = true;
          (*m_request_ports[port])->nb_request(p_info->m_request);
          wait(m_clock_period);
          m_waiting_for_nacc[port] = false;
        } while (m_request_got_nacc[port]);
        delete_request_info(p_info);
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



void xtsc_component::xtsc_pin2tlm_memory_transactor::pif_drive_pin_response_thread(void) {

  // Get the port number for this "instance" of the thread
  u32 port = m_next_port_pif_drive_pin_response_thread++;

  m_p_resp_valid[port]->write(false);

  try {

    XTSC_INFO(m_text, "in pif_drive_pin_response_thread[" << port << "]");

    // Loop forever
    while (true) {

      // Wait for a response
      wait(m_response_event[port]);
      XTSC_DEBUG(m_text, __FUNCTION__ << "(): awoke from wait()");

      // Process up to one response per clock period while there are responses available.
      while (!m_response_fifo[port].empty()) {

        xtsc_response *p_response = m_response_fifo[port].front();
        m_response_fifo[port].pop_front();

        m_id       = p_response->get_id();
        m_priority = p_response->get_priority();
        m_route_id = p_response->get_route_id();
        get_read_data_from_response(*p_response);
        if (m_p_resp_user_data[port]) {
          *m_user_data_val = (u64)p_response->get_user_data();
        }
        bool exclusive_req = p_response->get_exclusive_req();
        bool exclusive_ok  = p_response->get_exclusive_ok();

        m_resp_cntl.init((u32)p_response->get_status(), p_response->get_last_transfer(), exclusive_req, exclusive_ok);

        // drive response
        m_p_resp_valid      [port]->write(true);
        m_p_resp_cntl       [port]->write(m_resp_cntl.get_value());
        m_p_resp_data       [port]->write(m_data);
        if (m_p_resp_priority [port]) {
          m_p_resp_priority [port]->write(m_priority);
        }
        if (m_has_request_id) {
          m_p_resp_id       [port]->write(m_id);
        }
        if (m_route_id_bits && !m_axi_exclusive_id) {
          m_p_resp_route_id [port]->write(m_route_id);
        }
        if (m_p_resp_coh_cntl[port]) {
          if (m_snoop) {
            bool data   = p_response->has_snoop_data();
            m_coh_cntl = (data ? 1 : 0);
#if 0
            XTSC_NOTE(m_text, "data=" << data << " m_coh_cntl=" << m_coh_cntl);
#endif
          }
          else {
            m_coh_cntl = 0;
          }
          m_p_resp_coh_cntl [port]->write(m_coh_cntl);
        }
        if (m_p_resp_user_data[port]) {
          m_p_resp_user_data[port]->write(*m_user_data_val);
        }

        // We continue driving this response until it is accepted because at this point it
        // is too late to reject the TLM response because we have already returned true
        // to the nb_respond() call (the only way to reject a TLM response)
        bool ready = false;
        while (!ready) {
          sc_time now = sc_time_stamp();
          sc_time phase_now = (now.value() % m_clock_period_value) * m_time_resolution;
          if (m_has_posedge_offset) {
            if (phase_now < m_posedge_offset) {
              phase_now += m_clock_period;
            }
            phase_now -= m_posedge_offset;
          }
          sc_time wait_before(m_clock_period);
          if (phase_now < m_sample_phase) {
            wait_before = (m_sample_phase - phase_now);
          }
          else if (phase_now > m_sample_phase) {
            wait_before = (m_sample_phase_plus_one - phase_now);
          }
          wait(wait_before);
          // Sample at "sample_phase" time
          ready = m_p_resp_rdy[port]->read();
          sc_time wait_after(m_clock_period - wait_before);
          if (wait_after != SC_ZERO_TIME) {
            wait(wait_after);
          }
        }

        delete_response(p_response);
      }

      m_resp_cntl.init(0);
      m_id       = 0;
      m_priority = 0;
      m_route_id = 0;
      m_coh_cntl = 0;
      m_data     = 0;

      // deassert response
      m_p_resp_valid    [port]->write(false);
      m_p_resp_cntl     [port]->write(m_resp_cntl.get_value());
      m_p_resp_data     [port]->write(m_data);
      if (m_p_resp_priority [port]) {
        m_p_resp_priority [port]->write(m_priority);
      }
      if (m_has_request_id) {
        m_p_resp_id       [port]->write(m_id);
      }
      if (m_route_id_bits && !m_axi_exclusive_id) {
        m_p_resp_route_id [port]->write(m_route_id);
      }
      if (m_p_resp_coh_cntl[port]) {
        m_p_resp_coh_cntl [port]->write(m_coh_cntl);
      }
      if (m_p_resp_user_data[port]) {
        m_p_resp_user_data[port]->write(m_zero_bv);
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



void xtsc_component::xtsc_pin2tlm_memory_transactor::axi_req_addr_thread(void) {

  // Get the port number and signal indx for this "instance" of the thread
  u32 port = m_next_port_axi_req_addr_thread++;
  u32 indx = m_axi_signal_indx[port];

  try {

    bool read_channel = (m_axi_read_write[port] == 0);

    XTSC_INFO(m_text, "in axi_req_addr_thread[" << port << "]. Using " << (read_channel ? "read" : "write") << " signal indx=" << indx);

    string wait_signals(read_channel ?  "ARVALID|ARREADY" : "AWVALID|AWREADY");

    bool_input  *AxVALID  = (read_channel ? m_p_arvalid  : m_p_awvalid)[indx];
    bool_output *AxREADY  = (read_channel ? m_p_arready  : m_p_awready)[indx];
    uint_input  *AxADDR   = (read_channel ? m_p_araddr   : m_p_awaddr )[indx];

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
          axi_addr_info *p_addr_info = new_axi_addr_info(port);
          if (read_channel) {
            axi_req_info *p_req_info = new_axi_req_info(port, p_addr_info, NULL);
            m_axi_req_fifo[port].push_back(p_req_info);
            m_axi_send_tlm_req_event[port].notify(SC_ZERO_TIME);
          }
          else {
            if (p_addr_info->m_AxSNOOP == 0x4) { // Evict... No data
              axi_data_info *p_data_info = new_axi_data_info(port);
              axi_req_info  *p_req_info  = new_axi_req_info(port, p_addr_info, p_data_info);
              m_axi_req_fifo[port].push_back(p_req_info);
              m_axi_send_tlm_req_event[port].notify(SC_ZERO_TIME);
            } else {
              m_axi_addr_fifo[port].push_back(p_addr_info);
              m_axi_wr_req_event[port].notify(SC_ZERO_TIME);
            }
          }
          XTSC_VERBOSE(m_text, *p_addr_info << " (axi_req_addr_thread[" << port << "])");
        }
        else {
          XTSC_INFO(m_text, "Busy AxREADY of Port #" << port << " 0x" << hex << setfill('0') << setw(8) << AxADDR->read());
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



void xtsc_component::xtsc_pin2tlm_memory_transactor::axi_req_data_thread(void) {

  // Get the port number and signal indx for this "instance" of the thread
  u32 port = m_next_port_axi_req_data_thread++;
  u32 indx = m_axi_signal_indx[port];

  try {

    bool read_channel = (m_axi_read_write[port] == 0);
    if (read_channel) return;

    XTSC_INFO(m_text, "in axi_req_data_thread[" << port << "]. Using write signal indx=" << indx);

    string wait_signals("WVALID|WREADY");

    bool_input  *WVALID = m_p_wvalid[indx];
    bool_output *WREADY = m_p_wready[indx];

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
          axi_data_info *p_data_info = new_axi_data_info(port);
          m_axi_data_fifo[port].push_back(p_data_info);
          m_axi_wr_req_event[port].notify(SC_ZERO_TIME);
          XTSC_VERBOSE(m_text, *p_data_info << " (axi_req_data_thread[" << port << "])");
        }
        else {
          XTSC_INFO(m_text, "Busy WREADY of Port #" << port);
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



void xtsc_component::xtsc_pin2tlm_memory_transactor::axi_wr_req_thread(void) {

  // Get the port number for this "instance" of the thread
  u32 port = m_next_port_axi_wr_req_thread++;

  bool read_channel = (m_axi_read_write[port] == 0);
  if (read_channel) return;

  try {

    XTSC_INFO(m_text, "in axi_wr_req_thread[" << port << "]");

    axi_addr_info *p_addr_info  = NULL;

    // Loop forever
    while (true) {

      // Wait for a request
      wait(m_axi_wr_req_event[port]);   // Notified from axi_req_addr_thread and axi_req_data_thread
      XTSC_DEBUG(m_text, __FUNCTION__ << "(): awoke from wait()");

      // Process up to one request at a time while there are requests available.
      while (!m_axi_data_fifo[port].empty()) {
        axi_data_info *p_data_info = m_axi_data_fifo[port].front();
        m_axi_data_fifo[port].pop_front();
        if (!p_addr_info) {
          bool first_time = true;
          while (m_axi_addr_fifo[port].empty()) {
            if (first_time) {
              XTSC_INFO(m_text, "Port #" << port << " received transfer on the Write Data channel (" << *p_data_info <<
                                "), now waiting for Write Address channel.");
              first_time = false;
            }
            wait(m_axi_wr_req_event[port]);   // Notified from axi_req_addr_thread and axi_req_data_thread
          }
          if (!first_time) {
            XTSC_INFO(m_text, "Port #" << port << " done waiting for Write Address channel.");
          }
          p_addr_info = m_axi_addr_fifo[port].front();
          m_axi_addr_fifo[port].pop_front();
          XTSC_DEBUG(m_text, __FUNCTION__ << "(): Use  p_addr_info: " << *p_addr_info);
        }
        axi_req_info *p_req_info = new_axi_req_info(port, p_addr_info, p_data_info);
        m_axi_req_fifo[port].push_back(p_req_info);
        m_axi_send_tlm_req_event[port].notify(SC_ZERO_TIME);
        if (p_data_info->m_WLAST) {
          XTSC_DEBUG(m_text, __FUNCTION__ << "(): Free p_addr_info: " << *p_addr_info);
          p_addr_info  = NULL;
        }
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



void xtsc_component::xtsc_pin2tlm_memory_transactor::axi_drive_req_rdy_thread(void) {

  // Get the port number and signal indx for this "instance" of the thread
  u32 port = m_next_port_axi_drive_req_rdy_thread++;
  u32 indx = m_axi_signal_indx[port];

  bool read_channel = (m_axi_read_write[port] == 0);

  string ready_signal(read_channel ? "ARREADY" : "WREADY");

  bool_output *xREADY = (read_channel ? m_p_arready : m_p_wready)[indx];

  bool wait_for_response = false;

  // Empty fifo's
  while (m_req_rdy_fifo[port]->nb_read(wait_for_response));

  try {

    XTSC_INFO(m_text, "in axi_drive_req_rdy_thread[" << port << "]");

    xREADY->write(true);

    if (!read_channel) {
      // Drive AWREADY high and leave it there
      bool_output *AWREADY = m_p_awready[indx];
      AWREADY->write(true);
    }

    // Loop forever
    while (true) {
      // Wait to be notified
      wait(m_drive_req_rdy_event[port]);
      while (m_req_rdy_fifo[port]->num_available()) {
        m_req_rdy_fifo[port]->nb_read(wait_for_response);
        xREADY->write(false);
        XTSC_DEBUG(m_text, __FUNCTION__ << "[" << port << "] deasserting " << ready_signal);
        if (wait_for_response) {
          // true means wait until event is notified again
          wait(m_drive_req_rdy_event[port]);
          bool dummy = false;
          m_req_rdy_fifo[port]->nb_read(dummy); // dummy should be false, but we don't check it
        }
        else {
          // false means wait one clock period
          wait(m_clock_period);
        }
        xREADY->write(true);
        XTSC_DEBUG(m_text, __FUNCTION__ << "[" << port << "]   asserting " << ready_signal);
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



void xtsc_component::xtsc_pin2tlm_memory_transactor::axi_send_tlm_req_thread(void) {

  // Get the port number for this "instance" of the thread
  u32 port = m_next_port_axi_send_tlm_req_thread++;

  try {

    XTSC_INFO(m_text, "in axi_send_tlm_req_thread[" << port << "]");

    // Loop forever
    while (true) {

      // Wait for a request
      wait(m_axi_send_tlm_req_event[port]);   // Notified from axi_req_addr_thread (rd) and axi_wr_req_thread (wr)
      XTSC_DEBUG(m_text, __FUNCTION__ << "(): awoke from wait()");

      // Process up to one request at a time while there are requests available.
      while (!m_axi_req_fifo[port].empty()) {
        axi_req_info *p_info = m_axi_req_fifo[port].front();
        m_axi_req_fifo[port].pop_front();
        if (m_one_at_a_time && p_info->m_request.get_last_transfer()) {
          m_req_rdy_fifo[port]->write(true);
          m_drive_req_rdy_event[port].notify(m_output_delay);
          XTSC_INFO(m_text, p_info->m_request << ": scheduled AxREADY deassert upon last-transfer request");
          m_request_time_stamp[port] = sc_time_stamp();
        }
        u32 tries = 0;
        do {
          m_request_got_nacc[port] = false;
          tries += 1;
          XTSC_VERBOSE(m_text, p_info->m_request << " Port #" << port << " Try #" << tries);
          m_waiting_for_nacc[port] = true;
          (*m_request_ports[port])->nb_request(p_info->m_request);
          wait(m_clock_period);
          m_waiting_for_nacc[port] = false;
        } while (m_request_got_nacc[port]);
        delete_axi_req_info(p_info);
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



void xtsc_component::xtsc_pin2tlm_memory_transactor::axi_drive_rsp_thread(void) {

  // Get the port number and signal indx for this "instance" of the thread
  u32 port = m_next_port_axi_drive_rsp_thread++;
  u32 indx = m_axi_signal_indx[port];

  try {

    bool read_channel = (m_axi_read_write[port] == 0);

    XTSC_INFO(m_text, "in axi_drive_rsp_thread[" << port << "]. Using signal indx=" << indx);

    bool_input  *xREADY = (read_channel ? m_p_rready : m_p_bready)[indx];
    bool_output *xVALID = (read_channel ? m_p_rvalid : m_p_bvalid)[indx];
    uint_output *xID    = (read_channel ? m_p_rid    : m_p_bid   )[indx];
    uint_output *xRESP  = (read_channel ? m_p_rresp  : m_p_bresp )[indx];

    wide_output *RDATA  = (read_channel ? m_p_rdata[indx] : NULL);
    bool_output *RLAST  = (read_channel ? m_p_rlast[indx] : NULL);

    xVALID->write(false);

    // Loop forever
    while (true) {

      // deassert response
      *m_xID[port] = 0;
      m_xRESP      = 0x3;               // Normally we use 0 for deasserted signals but xRESP of 0 means OKAY
      xVALID ->write(false);
      xID    ->write(*m_xID[port]);
      xRESP  ->write(m_xRESP);
      if (read_channel) {
        m_data = 0;
        RDATA->write(m_data);
        RLAST->write(false);
      }

      // Wait for a response
      wait(m_response_event[port]);
      XTSC_DEBUG(m_text, __FUNCTION__ << "(): awoke from wait()");

      // Process up to one response per clock period while there are responses available.
      while (!m_response_fifo[port].empty()) {

        xtsc_response *p_response = m_response_fifo[port].front();
        m_response_fifo[port].pop_front();
        status_t status = p_response->get_status();

        // drive response
        *m_xID[port] = (p_response->get_route_id() << 8) + p_response->get_id();
        m_xRESP = p_response->get_resp_bits();
        xVALID->write(true);
        xID   ->write(*m_xID[port]);
        xRESP ->write(m_xRESP);
        if (read_channel) {
          get_read_data_from_response(*p_response);
          RDATA->write(m_data);
          RLAST->write(p_response->get_last_transfer());
        }

        // We continue driving this response until it is accepted because at this point it
        // is too late to reject the TLM response because we have already returned true
        // to the nb_respond() call (the only way to reject a TLM response)
        bool ready = false;
        while (!ready) {
          sc_time now = sc_time_stamp();
          sc_time phase_now = (now.value() % m_clock_period_value) * m_time_resolution;
          if (m_has_posedge_offset) {
            if (phase_now < m_posedge_offset) {
              phase_now += m_clock_period;
            }
            phase_now -= m_posedge_offset;
          }
          sc_time wait_before(m_clock_period);
          if (phase_now < m_sample_phase) {
            wait_before = (m_sample_phase - phase_now);
          }
          else if (phase_now > m_sample_phase) {
            wait_before = (m_sample_phase_plus_one - phase_now);
          }
          wait(wait_before);
          // Sample at "sample_phase" time
          ready = xREADY->read();
          sc_time wait_after(m_clock_period - wait_before);
          if (wait_after != SC_ZERO_TIME) {
            wait(wait_after);
          }
        }

        delete_response(p_response);
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



void xtsc_component::xtsc_pin2tlm_memory_transactor::apb_request_thread(void) {

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
        // Sample inputs and form xtsc_request
        m_apb_info_table[port]->init();

        XTSC_INFO(m_text, *m_apb_info_table[port]);

        // Send xtsc_request downstream
        (*m_request_ports[port])->nb_request(m_apb_info_table[port]->m_request);

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



void xtsc_component::xtsc_pin2tlm_memory_transactor::apb_drive_outputs_thread(void) {

  // Get the port number for this "instance" of the thread
  u32 port = m_next_port_apb_drive_outputs_thread++;

  try {

    XTSC_INFO(m_text, "in " << __FUNCTION__ << "[" << port << "]");

    m_data = 0;
    m_p_prdata [port]->write(m_data);
    m_p_pready [port]->write(false);
    m_p_pslverr[port]->write(false);

    // Loop forever
    while (true) {
      wait(m_apb_rsp_event[port]);

      m_data = m_apb_info_table[port]->m_data;
      m_p_prdata[port]->write(m_data);
      m_p_pready [port]->write(true);
      m_p_pslverr[port]->write(m_apb_info_table[port]->m_slverr);
      wait(m_clock_period);
      m_data = 0;
      m_p_prdata[port]->write(m_data);
      m_p_pready [port]->write(false);
      m_p_pslverr[port]->write(false);
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



bool_signal& xtsc_component::xtsc_pin2tlm_memory_transactor::create_bool_signal(const string& signal_name) {
  bool_signal *p_signal = new bool_signal(signal_name.c_str());
  m_map_bool_signal[signal_name] = p_signal;
  return *p_signal;
}



uint_signal& xtsc_component::xtsc_pin2tlm_memory_transactor::create_uint_signal(const string& signal_name, u32 num_bits) {
  sc_length_context length(num_bits);
  uint_signal *p_signal = new uint_signal(signal_name.c_str());
  m_map_uint_signal[signal_name] = p_signal;
  return *p_signal;
}



wide_signal& xtsc_component::xtsc_pin2tlm_memory_transactor::create_wide_signal(const string& signal_name, u32 num_bits) {
  sc_length_context length(num_bits);
  wide_signal *p_signal = new wide_signal(signal_name.c_str());
  m_map_wide_signal[signal_name] = p_signal;
  return *p_signal;
}



void xtsc_component::xtsc_pin2tlm_memory_transactor::swizzle_byte_enables(xtsc_byte_enables& byte_enables) const {
  xtsc_byte_enables swizzled = 0;
  for (u32 i=0; i<m_width8; i++) {
    swizzled <<= 1;
    swizzled |= byte_enables & 1;
    byte_enables >>= 1;
  }
  byte_enables = swizzled;
}



void xtsc_component::xtsc_pin2tlm_memory_transactor::initialize_subbank_activity(subbank_activity *p_subbank_activity) {
  memset(p_subbank_activity, 0, sizeof(subbank_activity));
  p_subbank_activity->m_cycle_num = (u64)-1;
}



xtsc_component::xtsc_pin2tlm_memory_transactor::request_info::request_info(const xtsc_pin2tlm_memory_transactor& pin2tlm, u32 port) :
  m_pin2tlm       (pin2tlm),
  m_req_cntl      (0, m_pin2tlm.m_axi_exclusive),
  m_data          ((int)m_pin2tlm.m_width8*8),
  m_id            (m_id_bits),
  m_priority      (2),
  m_route_id      (m_pin2tlm.m_route_id_bits ? m_pin2tlm.m_route_id_bits : 1),
  m_req_attribute (12),
  m_req_domain    (2),
  m_vadrs         (6),
  m_coherence     (2)
{
  init(port);
}



xtsc_component::xtsc_pin2tlm_memory_transactor::request_info *
xtsc_component::xtsc_pin2tlm_memory_transactor::new_request_info(u32 port) {
  if (m_request_pool.empty()) {
    XTSC_DEBUG(m_text, "Creating a new request_info");
    return new request_info(*this, port);
  }
  else {
    request_info *p_request_info = m_request_pool.back();
    m_request_pool.pop_back();
    p_request_info->init(port);
    return p_request_info;
  }
}



void xtsc_component::xtsc_pin2tlm_memory_transactor::delete_request_info(request_info*& p_request_info) {
  m_request_pool.push_back(p_request_info);
  p_request_info = 0;
}



void xtsc_component::xtsc_pin2tlm_memory_transactor::request_info::init(u32 port) {
  bool   has_data = false;
  u32    size     = m_pin2tlm.m_width8;
  type_t type     = xtsc_request::READ;
  u32    bus_bits = 0;

  if (m_pin2tlm.m_pif_or_idma) {
    m_req_cntl.set_value( m_pin2tlm.m_p_req_cntl     [port]->read().to_uint());
    m_address           = m_pin2tlm.m_p_req_adrs     [port]->read().to_uint();
    m_fixed_address     = m_address;
    m_priority          = m_pin2tlm.m_p_req_priority [port]->read();
    m_byte_enables      = m_pin2tlm.m_p_req_data_be  [port] ? m_pin2tlm.m_p_req_data_be  [port]->read().to_uint() : 0;
    m_data              = m_pin2tlm.m_p_req_data     [port] ? m_pin2tlm.m_p_req_data     [port]->read() : m_pin2tlm.m_zero_bv;
    m_id                = m_pin2tlm.m_p_req_id       [port] ? m_pin2tlm.m_p_req_id       [port]->read() : m_pin2tlm.m_zero_uint;
    m_route_id          = m_pin2tlm.m_p_req_route_id [port] ? m_pin2tlm.m_p_req_route_id [port]->read() : m_pin2tlm.m_zero_uint;
    m_req_attribute     = m_pin2tlm.m_p_req_attribute[port] ? m_pin2tlm.m_p_req_attribute[port]->read() : m_pin2tlm.m_zero_uint;
    m_req_domain        = m_pin2tlm.m_p_req_domain   [port] ? m_pin2tlm.m_p_req_domain   [port]->read() : m_pin2tlm.m_zero_uint;
    m_vadrs             = m_pin2tlm.m_p_req_coh_vadrs[port] ? m_pin2tlm.m_p_req_coh_vadrs[port]->read() : m_pin2tlm.m_zero_uint;
    m_coherence         = m_pin2tlm.m_p_req_coh_cntl [port] ? m_pin2tlm.m_p_req_coh_cntl [port]->read() : m_pin2tlm.m_zero_uint;
    m_fixed_byte_enables = m_byte_enables;
    if (m_pin2tlm.m_big_endian) {
      m_pin2tlm.swizzle_byte_enables(m_fixed_byte_enables);
    }
  }
  else {
    m_address = m_pin2tlm.m_p_addr[port]->read().to_uint();
    m_fixed_address = ((m_address << m_pin2tlm.m_address_shift) & m_pin2tlm.m_address_mask) + m_pin2tlm.m_start_byte_address;
    if (m_pin2tlm.m_banked) {
      m_fixed_address += port * m_pin2tlm.m_width8;
    }
    if (m_pin2tlm.m_p_wr[port] && m_pin2tlm.m_p_wr[port]->read()) {
      m_data = m_pin2tlm.m_p_wrdata[port]->read();
      type = xtsc_request::WRITE;
      has_data = true;
    }
    m_byte_enables = (0xFFFFFFFFFFFFFFFFull >> (64 - (size)));
    m_fixed_byte_enables = m_byte_enables;
    if (m_pin2tlm.m_p_lane[port]) {
      m_byte_enables = m_pin2tlm.m_p_lane[port]->read();
      if ((m_pin2tlm.m_interface_type == IRAM0) || (m_pin2tlm.m_interface_type == IRAM1) || (m_pin2tlm.m_interface_type == IROM0)) {
        // Convert word enables (1 "word" is 32 bits) to byte enables (256-bit IRAM0|IRAM1|IROM0 only)
        xtsc_byte_enables be = 0x00000000;
        if (m_byte_enables & 0x1)  be |= 0x0000000F;
        if (m_byte_enables & 0x2)  be |= 0x000000F0;
        if (m_byte_enables & 0x4)  be |= 0x00000F00;
        if (m_byte_enables & 0x8)  be |= 0x0000F000;
        if (m_byte_enables & 0x10) be |= 0x000F0000;
        if (m_byte_enables & 0x20) be |= 0x00F00000;
        if (m_byte_enables & 0x40) be |= 0x0F000000;
        if (m_byte_enables & 0x80) be |= 0xF0000000;
        m_byte_enables = be;
      }
      m_fixed_byte_enables = m_byte_enables;
      if (m_pin2tlm.m_big_endian) {
        m_pin2tlm.swizzle_byte_enables(m_fixed_byte_enables);
      }
    }
  }

  // Adjust bus_bits/byte enables/size or address for xtsc_request byte enables/address requirments
  if (m_pin2tlm.m_pif_or_idma) {
    type = m_req_cntl.get_request_type();
    if ((type == xtsc_request::WRITE) || (type == xtsc_request::RCW)) {
      bus_bits = (m_fixed_address & m_pin2tlm.m_bus_addr_bits_mask);
      m_fixed_byte_enables >>= bus_bits;
           if (m_fixed_byte_enables <   2) size =  1;
      else if (m_fixed_byte_enables <   4) size =  2;
      else if (m_fixed_byte_enables <  16) size =  4;
      else if (m_fixed_byte_enables < 256) size =  8;
      else                                 size = 16;
    }
    else if ((m_req_cntl.get_request_type() != xtsc_request::BLOCK_WRITE) && !m_pin2tlm.m_snoop) {
      m_fixed_address -= (m_fixed_address & m_pin2tlm.m_bus_addr_bits_mask);
    }
  }

  XTSC_DEBUG(m_pin2tlm.m_text, "In init(): m_address=0x" << hex << setw(8) << m_address << " m_fixed_address=0x" << setw(8) << m_fixed_address <<
                               " m_byte_enables=0x" << m_byte_enables << " m_fixed_byte_enables=0x" << m_fixed_byte_enables <<
                               " size=" << dec << size << " m_pif_or_idma=" << m_pin2tlm.m_pif_or_idma);

  if (m_pin2tlm.m_pif_or_idma) {
    u64 tag = 0;
    u32  num_transfers = m_req_cntl.get_num_transfers();
    bool last_transfer = m_req_cntl.get_last_transfer();
    u8 id = (u8) m_id.to_uint();
    if (type == xtsc_request::BLOCK_WRITE) {
      has_data = true;
      if (m_pin2tlm.m_first_block_write[port]) {
        m_request.initialize(type, m_fixed_address, size, tag, num_transfers, m_fixed_byte_enables,
                             last_transfer, m_route_id, id, m_priority);
      }
      else {
        tag                     = m_pin2tlm.m_tag[port];
        xtsc_address total_size = (m_pin2tlm.m_width8 * num_transfers);
        xtsc_address offset     = m_pin2tlm.m_last_address[port] % total_size;
        xtsc_address base       = m_pin2tlm.m_last_address[port] - offset;
        m_fixed_address         = base + ((offset + m_pin2tlm.m_width8) % total_size);
        m_request.initialize(tag, m_fixed_address, size, num_transfers, last_transfer, m_route_id, id, m_priority);
      }
      m_request.set_byte_enables(m_fixed_byte_enables);
      m_pin2tlm.m_tag[port] = m_request.get_tag();
      m_pin2tlm.m_last_address[port] = m_fixed_address;
      m_pin2tlm.m_first_block_write[port] = last_transfer;
    }
    else if (type == xtsc_request::BURST_WRITE) {
      has_data = true;
      if (m_pin2tlm.m_burst_write_transfer_num[port] == 1) {
        m_request.initialize(type, m_fixed_address, size, tag, num_transfers, m_fixed_byte_enables,
                             last_transfer, m_route_id, id, m_priority);
      }
      else {
        tag = m_pin2tlm.m_tag[port];
        m_fixed_address = m_pin2tlm.m_last_address[port] + m_pin2tlm.m_width8;
        m_request.initialize(m_address, tag, m_fixed_address, size, num_transfers, m_pin2tlm.m_burst_write_transfer_num[port],
                             m_fixed_byte_enables, m_route_id, id, m_priority);
      }
      m_pin2tlm.m_last_address[port] = m_fixed_address;
      m_pin2tlm.m_tag[port] = m_request.get_tag();
      if (last_transfer) {
        m_pin2tlm.m_burst_write_transfer_num[port] = 1;
      }
      else {
        m_pin2tlm.m_burst_write_transfer_num[port] += 1;
      }
    }
    else if (type == xtsc_request::RCW) {
      has_data = true;
      if (m_pin2tlm.m_first_rcw[port]) {
        m_request.initialize(type, m_fixed_address, size, tag, num_transfers, m_fixed_byte_enables,
                             last_transfer, m_route_id, id, m_priority);
      }
      else {
        tag = m_pin2tlm.m_tag[port];
        m_request.initialize(tag, m_fixed_address, m_route_id, id, m_priority);
        m_request.set_byte_size(size);
        m_request.set_byte_enables(m_fixed_byte_enables);
      }
      m_pin2tlm.m_tag[port] = m_request.get_tag();
      m_pin2tlm.m_first_rcw[port] = last_transfer;
    }
    else {
      if (type == xtsc_request::WRITE) {
        has_data = true;
      }
      m_request.initialize(type, m_fixed_address, size, tag, num_transfers, m_fixed_byte_enables,
                           last_transfer, m_route_id, id, m_priority);
    }
    if (m_pin2tlm.m_p_req_attribute[port]) {
      m_request.set_pif_attribute(m_req_attribute.to_uint());
    }
    if (m_pin2tlm.m_p_req_domain[port]) {
      m_request.set_pif_req_domain(m_req_domain.to_uint());
    }
    if (m_pin2tlm.m_p_req_user_data[port]) {
#if defined(__x86_64__) || defined(_M_X64)
      m_request.set_user_data((void*)m_pin2tlm.m_p_req_user_data[port]->read().to_uint64());
#else
      m_request.set_user_data((void*)m_pin2tlm.m_p_req_user_data[port]->read().to_uint());
#endif
    }
    if (m_pin2tlm.m_axi_exclusive) {
      m_request.set_exclusive(m_req_cntl.is_exclusive());
    }
  }
  else {
    m_request.initialize(type, m_fixed_address, size, 0, 1, m_fixed_byte_enables);
    if (m_pin2tlm.m_p_xfer_en && m_pin2tlm.m_p_xfer_en[port]) {
      m_request.set_xfer_en(m_pin2tlm.m_p_xfer_en[port]->read());
    }
    if (!has_data && m_pin2tlm.m_interface_type == xtsc_module_pin_base::XLMI0) {
      // If busy gets asserted, then this will need to be removed from the deque
      m_pin2tlm.m_load_address_deque[port].push_back(m_fixed_address);
    }
    if (m_pin2tlm.m_p_attr[port]) {
      m_pin2tlm.m_dram_attribute_bv = m_pin2tlm.m_p_attr[port]->read();
      m_pin2tlm.m_dram_attribute = m_pin2tlm.m_dram_attribute_bv;
      m_request.set_dram_attribute(m_pin2tlm.m_dram_attribute);
    }
    if (m_pin2tlm.m_num_subbanks >= 2) {
      sc_time now = sc_time_stamp();
      m_cycle_num = ((now.value() - m_pin2tlm.m_posedge_offset_value) / m_pin2tlm.m_clock_period_value);
      m_cycle_index = m_cycle_num % m_pin2tlm.m_max_cycle_entries;
    }
  }

  // We don't check for valid address, size, byte enables here - the terminal device is responsible for enforcing legal combinations
  if (has_data) {
    u8 *buffer = m_request.get_buffer();
    u32 r = (m_pin2tlm.m_big_endian ? (m_pin2tlm.m_width8-1-bus_bits) : bus_bits) * 8;
    u32 delta = (m_pin2tlm.m_big_endian ? -8 : 8);
    for (u32 i=0; i<size; ++i) {
      if ((r+7) < (m_pin2tlm.m_width8*8)) {
        XTSC_DEBUG(m_pin2tlm.m_text, "init() data movement: buffer[" << i << "] = m_data.range(" << (r+7) << ", " << r << ")");
        buffer[i] = m_data.range(r+7, r).to_uint();
      }
      r += delta;
    }
  }

  XTSC_INFO(m_pin2tlm.m_text, m_request << ": In init()");
}



void xtsc_component::xtsc_pin2tlm_memory_transactor::request_info::dump(ostream& os) const {
  // Save state of stream
  char c = os.fill('0');
  ios::fmtflags old_flags = os.flags();
  streamsize w = os.width();
  os << right;

  ostringstream oss;
  oss << m_req_cntl;
  string req_cntl(oss.str());
  u32 type = m_req_cntl.get_type();
  if (type == req_cntl::SNOOP) {
    req_cntl.replace(2, 1, ((m_coherence == 1) ? "S" : (m_coherence == 2) ? "E" : (m_coherence == 3) ? "I" : "X"));
  }
  os << req_cntl << " [0x" << hex << setw(8) << m_address << "/0x" << setw(m_pin2tlm.m_width8/4) << m_byte_enables << "]";

  if (type == req_cntl::WRITE || type == req_cntl::BLOCK_WRITE || type == req_cntl::BURST_WRITE || type == req_cntl::RCW) {
    os << "=0x" << hex << m_data;
  }

  // Restore state of stream
  os.fill(c);
  os.flags(old_flags);
  os.width(w);
}



namespace xtsc_component {
ostream& operator<<(ostream& os, const xtsc_pin2tlm_memory_transactor::request_info& info) {
  info.dump(os);
  return os;
}
}



xtsc_component::xtsc_pin2tlm_memory_transactor::axi_addr_info *xtsc_component::xtsc_pin2tlm_memory_transactor::new_axi_addr_info(u32 port) {
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



void xtsc_component::xtsc_pin2tlm_memory_transactor::delete_axi_addr_info(axi_addr_info*& p_axi_addr_info) {
  m_axi_addr_pool.push_back(p_axi_addr_info);
  p_axi_addr_info = 0;
}



xtsc_component::xtsc_pin2tlm_memory_transactor::axi_addr_info::axi_addr_info(const xtsc_pin2tlm_memory_transactor& pin2tlm, u32 port) :
  m_pin2tlm     (pin2tlm)
{
  init(port);
}



void xtsc_component::xtsc_pin2tlm_memory_transactor::axi_addr_info::init(u32 port) {
  bool axi4             = (m_pin2tlm.m_axi_port_type[port] == AXI4);
  m_port                = port;
  m_read_channel        = (m_pin2tlm.m_axi_read_write[m_port] == 0);
  m_indx                = m_pin2tlm.m_axi_signal_indx[m_port];
  m_time_stamp          = sc_time_stamp();
  m_AxBAR               = 0;
  m_AxDOMAIN            = 0;
  m_AxSNOOP             = 0;
  if (m_read_channel) {
    m_AxID      = m_pin2tlm.m_p_arid   [m_indx]->read().to_uint();
    m_AxADDR    = m_pin2tlm.m_p_araddr [m_indx]->read().to_uint();
    m_AxLEN     = m_pin2tlm.m_p_arlen  [m_indx]->read().to_uint();
    m_AxSIZE    = m_pin2tlm.m_p_arsize [m_indx]->read().to_uint();
    m_AxBURST   = m_pin2tlm.m_p_arburst[m_indx]->read().to_uint();
    m_AxLOCK    = m_pin2tlm.m_p_arlock [m_indx]->read();
    m_AxCACHE   = m_pin2tlm.m_p_arcache[m_indx]->read().to_uint();
    m_AxPROT    = m_pin2tlm.m_p_arprot [m_indx]->read().to_uint();
    m_AxQOS     = m_pin2tlm.m_p_arqos  [m_indx]->read().to_uint();
    if (!axi4) {
      m_AxBAR     = m_pin2tlm.m_p_arbar   [m_indx]->read().to_uint();
      m_AxDOMAIN  = m_pin2tlm.m_p_ardomain[m_indx]->read().to_uint();
      m_AxSNOOP   = m_pin2tlm.m_p_arsnoop [m_indx]->read().to_uint();
    }
  }
  else {
    m_AxID      = m_pin2tlm.m_p_awid   [m_indx]->read().to_uint();
    m_AxADDR    = m_pin2tlm.m_p_awaddr [m_indx]->read().to_uint();
    m_AxLEN     = m_pin2tlm.m_p_awlen  [m_indx]->read().to_uint();
    m_AxSIZE    = m_pin2tlm.m_p_awsize [m_indx]->read().to_uint();
    m_AxBURST   = m_pin2tlm.m_p_awburst[m_indx]->read().to_uint();
    m_AxLOCK    = m_pin2tlm.m_p_awlock [m_indx]->read();
    m_AxCACHE   = m_pin2tlm.m_p_awcache[m_indx]->read().to_uint();
    m_AxPROT    = m_pin2tlm.m_p_awprot [m_indx]->read().to_uint();
    m_AxQOS     = m_pin2tlm.m_p_awqos  [m_indx]->read().to_uint();
    if (!axi4) {
      m_AxBAR    = m_pin2tlm.m_p_awbar   [m_indx]->read().to_uint();
      m_AxDOMAIN = m_pin2tlm.m_p_awdomain[m_indx]->read().to_uint();
      m_AxSNOOP  = m_pin2tlm.m_p_awsnoop [m_indx]->read().to_uint();
    }
  }
  u32  num   = m_AxLEN + 1;
  bool fixed = (m_AxBURST == 0);
  bool wrap  = (m_AxBURST == 2);
  if ((fixed && (num > 16)) || (wrap && (num != 2) && (num != 4) && (num != 8) && (num != 16))) {
    ostringstream oss;
    oss << m_pin2tlm.kind() << " '" << m_pin2tlm.name() << "': Received " << (fixed ? "FIXED" : "WRAP") << " burst "
        << (m_read_channel ? "read" : "write") << " with illegal AxLEN=" << (num-1);
    throw xtsc_exception(oss.str());
  }
}



void xtsc_component::xtsc_pin2tlm_memory_transactor::axi_addr_info::dump(ostream& os) const {
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
ostream& operator<<(ostream& os, const xtsc_pin2tlm_memory_transactor::axi_addr_info& info) {
  info.dump(os);
  return os;
}
}



xtsc_component::xtsc_pin2tlm_memory_transactor::axi_data_info *xtsc_component::xtsc_pin2tlm_memory_transactor::new_axi_data_info(u32 port) {
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



void xtsc_component::xtsc_pin2tlm_memory_transactor::delete_axi_data_info(axi_data_info*& p_axi_data_info) {
  m_axi_data_pool.push_back(p_axi_data_info);
  p_axi_data_info = 0;
}



xtsc_component::xtsc_pin2tlm_memory_transactor::axi_data_info::axi_data_info(const xtsc_pin2tlm_memory_transactor& pin2tlm, u32 port) :
  m_pin2tlm     (pin2tlm),
  m_WDATA       ((int)m_pin2tlm.m_width8*8)
{
  init(port);
}



void xtsc_component::xtsc_pin2tlm_memory_transactor::axi_data_info::init(u32 port) {
  m_port        = port;
  m_indx        = m_pin2tlm.m_axi_signal_indx[m_port];
  m_time_stamp  = sc_time_stamp();
  m_WDATA       = m_pin2tlm.m_p_wdata[m_indx]->read();
  m_WSTRB       = m_pin2tlm.m_p_wstrb[m_indx]->read().to_uint();
  m_WLAST       = m_pin2tlm.m_p_wlast[m_indx]->read();
}



void xtsc_component::xtsc_pin2tlm_memory_transactor::axi_data_info::dump(ostream& os) const {
  // Save state of stream
  char c = os.fill('0');
  ios::fmtflags old_flags = os.flags();
  streamsize w = os.width();

  ostringstream oss;
  oss << hex << m_WDATA;

  os << right << "0x" << oss.str().substr(1) << (m_WLAST ? "*" : " ") << "[0x" << hex << setw(m_pin2tlm.m_width8/4) << m_WSTRB << "]";

  // Restore state of stream
  os.fill(c);
  os.flags(old_flags);
  os.width(w);
}



namespace xtsc_component {
ostream& operator<<(ostream& os, const xtsc_pin2tlm_memory_transactor::axi_data_info& info) {
  info.dump(os);
  return os;
}
}



xtsc_component::xtsc_pin2tlm_memory_transactor::axi_req_info::axi_req_info(const xtsc_pin2tlm_memory_transactor& pin2tlm,
                                                                           u32                                   port,
                                                                           axi_addr_info                        *p_addr_info,
                                                                           axi_data_info                        *p_data_info) :
  m_pin2tlm       (pin2tlm)
{
  init(port, p_addr_info, p_data_info);
}



xtsc_component::xtsc_pin2tlm_memory_transactor::axi_req_info *
xtsc_component::xtsc_pin2tlm_memory_transactor::new_axi_req_info(u32 port, axi_addr_info *p_addr_info, axi_data_info *p_data_info) {
  if (m_axi_req_pool.empty()) {
    XTSC_DEBUG(m_text, "Creating a new axi_req_info");
    return new axi_req_info(*this, port, p_addr_info, p_data_info);
  }
  else {
    axi_req_info *p_axi_req_info = m_axi_req_pool.back();
    m_axi_req_pool.pop_back();
    p_axi_req_info->init(port, p_addr_info, p_data_info);
    return p_axi_req_info;
  }
}



void xtsc_component::xtsc_pin2tlm_memory_transactor::delete_axi_req_info(axi_req_info*& p_axi_req_info) {
  if (p_axi_req_info->m_request.get_last_transfer()) {
    delete_axi_addr_info(p_axi_req_info->m_p_addr_info);
  }
  else {
    p_axi_req_info->m_p_addr_info = 0;
  }
  if (p_axi_req_info->m_p_data_info) {
    delete_axi_data_info(p_axi_req_info->m_p_data_info);
  }
  m_axi_req_pool.push_back(p_axi_req_info);
  p_axi_req_info = 0;
}



void xtsc_component::xtsc_pin2tlm_memory_transactor::axi_req_info::init(u32 port, axi_addr_info *p_addr_info, axi_data_info *p_data_info) {
  m_p_addr_info = p_addr_info;
  m_p_data_info = p_data_info;
  
  bool              is_read        = (p_data_info == NULL);
  type_t            type            = (is_read ? xtsc_request::BURST_READ : xtsc_request::BURST_WRITE);
  u32               size            = (1 << p_addr_info->m_AxSIZE);
  xtsc_address      hw_address      = p_addr_info->m_AxADDR;
  xtsc_address      fix_address     = hw_address & (((xtsc_address)-1) ^ (size - 1)); // "fix" means "corrected" (not AXI4 FIXED)
  u64               tag             = 0;
  u32               num_transfers   = (p_addr_info->m_AxLEN + 1);
  xtsc_byte_enables byte_enables    = (is_read ? 0    : p_data_info->m_WSTRB);
  bool              last_transfer   = (is_read ? true : p_data_info->m_WLAST);
  u32               tran_id         = (p_addr_info->m_AxID);
  u32               route_id        = (tran_id >> m_pin2tlm.m_axi_id_bits[port]);
  u8                id              = (tran_id & ((1 << m_pin2tlm.m_axi_id_bits[port]) - 1));
  u8                priority        = (p_addr_info->m_AxQOS);
  u8                cache           = (p_addr_info->m_AxCACHE);
  u8                domain          = (p_addr_info->m_AxDOMAIN);
  u8                snoop           = (p_addr_info->m_AxSNOOP);
  burst_t           burst           = xtsc_request::get_burst_type_from_bits(p_addr_info->m_AxBURST);
  bool              exclusive       = (p_addr_info->m_AxLOCK != 0);

  if (is_read) {
    m_request.initialize(type, fix_address, size, tag, num_transfers, byte_enables, last_transfer, route_id, id, priority, -1, burst);
  }
  else {
    xtsc_address adj_address = fix_address;
    if (burst != xtsc_request::FIXED) {
      u32 adjustment = ((m_pin2tlm.m_burst_write_transfer_num[port] - 1) * size);
      adj_address += adjustment;
      if (burst == xtsc_request::WRAP) {
        u32          total_size8    = size * num_transfers;
        u32          initial_offset = fix_address % total_size8;
        xtsc_address low_address    = fix_address - initial_offset;
        xtsc_address wrap_address   = low_address + total_size8;
        if (adj_address >= wrap_address) {
          adj_address -= total_size8;
        }
        XTSC_DEBUG(m_pin2tlm.m_text, "init() hw_address=0x" << hex << hw_address << " fix_address=0x" << fix_address <<
                   " low_address=0x" << low_address << " wrap_address=0x" << wrap_address << " adj_address=0x" << adj_address << dec <<
                   " adjustment=" << adjustment << " total_size8=" << total_size8 << " initial_offset=" << initial_offset);
      }
    }
    u32 bus_addr_bytes = adj_address & (m_pin2tlm.m_width8 - 1);
    xtsc_byte_enables adj_enables = ((byte_enables >> bus_addr_bytes) & ((1 << size) - 1));
    if (m_pin2tlm.m_burst_write_transfer_num[port] == 1) {
      m_request.initialize(type, adj_address, size, tag, num_transfers, adj_enables, last_transfer, route_id, id, priority, -1, burst);
    }
    else {
      tag = m_pin2tlm.m_tag[port];
      m_request.initialize(hw_address, tag, adj_address, size, num_transfers, m_pin2tlm.m_burst_write_transfer_num[port],
                           adj_enables, route_id, id, priority, -1, burst);
    }
    m_pin2tlm.m_tag[port] = m_request.get_tag();
    if (last_transfer) {
      m_pin2tlm.m_burst_write_transfer_num[port] = 1;
    }
    else {
      m_pin2tlm.m_burst_write_transfer_num[port] += 1;
    }
    // We don't check agains byte enables here, that is up to the terminal device
    u8 *buffer = m_request.get_buffer();
    u32 r = bus_addr_bytes * 8;
    XTSC_DEBUG(m_pin2tlm.m_text, "init() hw_address=" << hex << hw_address << " fix_address=0x" << fix_address <<
                                 " adj_address=0x" << adj_address << " bus_addr_bytes=" << dec << bus_addr_bytes);
    for (u32 i=0; i<size; ++i) {
      XTSC_DEBUG(m_pin2tlm.m_text, "init() data movement: buffer[" << i << "] = m_WDATA.range(" << (r+7) << ", " << r << ")");
      buffer[i] = p_data_info->m_WDATA.range(r+7, r).to_uint();
      r += 8;
    }
  }
  m_request.set_cache(cache);
  m_request.set_domain(domain);
  m_request.set_exclusive(exclusive);
  m_request.set_snoop(snoop);

  XTSC_INFO(m_pin2tlm.m_text, m_request << ": In axi_req_info::init()");
}



void xtsc_component::xtsc_pin2tlm_memory_transactor::axi_req_info::dump(ostream& os) const {
  // Save state of stream
  char c = os.fill('0');
  ios::fmtflags old_flags = os.flags();
  streamsize w = os.width();
  os << right;

  os << m_request << ": " << *m_p_addr_info;

  if (m_p_data_info != NULL) {
    os << ": " << m_p_data_info;
  }

  // Restore state of stream
  os.fill(c);
  os.flags(old_flags);
  os.width(w);
}



namespace xtsc_component {
ostream& operator<<(ostream& os, const xtsc_pin2tlm_memory_transactor::axi_req_info& info) {
  info.dump(os);
  return os;
}
}



xtsc_component::xtsc_pin2tlm_memory_transactor::apb_info::apb_info(const xtsc_pin2tlm_memory_transactor& pin2tlm, u32 port) :
  m_pin2tlm     (pin2tlm),
  m_port        (port),
  m_address     (0),
  m_prot        (0),
  m_data        (0),
  m_strb        (0),
  m_write       (false),
  m_slverr      (false),
  m_request     ()
{
}



void xtsc_component::xtsc_pin2tlm_memory_transactor::apb_info::init() {
  m_address  = m_pin2tlm.m_p_paddr[m_port]->read();
  m_prot     = m_pin2tlm.m_p_pprot[m_port]->read();
  m_strb     = m_pin2tlm.m_p_pstrb[m_port]->read();
  m_write    = m_pin2tlm.m_p_pwrite[m_port]->read();
  m_slverr   = false;
  xtsc_request::type_t type = (m_write ? xtsc_request::WRITE : xtsc_request::READ);
  m_request.initialize(type, m_address, m_pin2tlm.m_width8, 0, 1, (u64)m_strb, true, 0, 0, 0, 0, xtsc_request::NON_AXI, true);
  m_request.set_prot(m_prot);
  if (m_write) {
    m_data   = m_pin2tlm.m_p_pwdata[m_port]->read().to_uint();
    memcpy(m_request.get_buffer(), &m_data, m_pin2tlm.m_width8);
  }
}



void xtsc_component::xtsc_pin2tlm_memory_transactor::apb_info::dump(ostream& os) const {
  os << (m_write ? "WRITE" : "READ ") << " 0x" << hex << setfill('0') << setw(8) << m_address << "/0x" << m_strb << ": 0x" << setw(8) << m_data
     << (m_slverr ? " PSLVERR" : "") << " Port #" << m_port;
}



namespace xtsc_component {
ostream& operator<<(ostream& os, const xtsc_pin2tlm_memory_transactor::apb_info& info) {
  info.dump(os);
  return os;
}
}



xtsc_response *xtsc_component::xtsc_pin2tlm_memory_transactor::new_response(const xtsc_response& response) {
  if (m_response_pool.empty()) {
    XTSC_DEBUG(m_text, "Creating a new xtsc_response");
    return new xtsc_response(response);
  }
  else {
    xtsc_response *p_response = m_response_pool.back();
    m_response_pool.pop_back();
    *p_response = response;
    return p_response;
  }
}



void xtsc_component::xtsc_pin2tlm_memory_transactor::delete_response(xtsc_response*& p_response) {
  m_response_pool.push_back(p_response);
  p_response = 0;
}



void xtsc_component::xtsc_pin2tlm_memory_transactor::xtsc_debug_if_impl::nb_peek(xtsc_address address8, u32 size8, u8 *buffer) {
  (*m_pin2tlm.m_request_ports[m_port_num])->nb_peek(address8, size8, buffer);
}



void xtsc_component::xtsc_pin2tlm_memory_transactor::xtsc_debug_if_impl::nb_poke(xtsc_address address8, u32 size8, const u8 *buffer) {
  (*m_pin2tlm.m_request_ports[m_port_num])->nb_poke(address8, size8, buffer);
}



bool xtsc_component::xtsc_pin2tlm_memory_transactor::xtsc_debug_if_impl::nb_peek_coherent(xtsc_address  virtual_address8,
                                                                                          xtsc_address  physical_address8,
                                                                                          u32           size8,
                                                                                          u8           *buffer)
{
  return (*m_pin2tlm.m_request_ports[m_port_num])->nb_peek_coherent(virtual_address8, physical_address8, size8, buffer);
}



bool xtsc_component::xtsc_pin2tlm_memory_transactor::xtsc_debug_if_impl::nb_poke_coherent(xtsc_address  virtual_address8,
                                                                                          xtsc_address  physical_address8,
                                                                                          u32           size8,
                                                                                          const u8     *buffer)
{
  return (*m_pin2tlm.m_request_ports[m_port_num])->nb_poke_coherent(virtual_address8, physical_address8, size8, buffer);
}



bool xtsc_component::xtsc_pin2tlm_memory_transactor::xtsc_debug_if_impl::nb_fast_access(xtsc_fast_access_request &request) {
  return (*m_pin2tlm.m_request_ports[m_port_num])->nb_fast_access(request);
}



void xtsc_component::xtsc_pin2tlm_memory_transactor::xtsc_debug_if_impl::register_port(sc_port_base& port, const char *if_typename) {
  if (m_p_port) {
    ostringstream oss;
    oss << "Illegal multiple binding detected to xtsc_pin2tlm_memory_transactor '" << m_pin2tlm.name() << "' m_debug_exports["
        << m_port_num << "]: " << endl;
    oss << "  " << port.name() << endl;
    oss << "  " << m_p_port->name();
    throw xtsc_exception(oss.str());
  }
  XTSC_INFO(m_pin2tlm.m_text, "Binding '" << port.name() << "' to '" << m_pin2tlm.name() << ".m_debug_exports[" << m_port_num <<
                              "]'");
  m_p_port = &port;
}



xtsc_component::xtsc_pin2tlm_memory_transactor::xtsc_respond_if_impl::xtsc_respond_if_impl(const char                          *object_name,
                                                                                           xtsc_pin2tlm_memory_transactor&      pin2tlm,
                                                                                           u32                                  port_num) :
  sc_object         (object_name),
  m_pin2tlm         (pin2tlm),
  m_p_port          (0),
  m_port_num        (port_num),
  m_busy_port       (port_num),
  m_bank            (port_num)
{
  if (m_pin2tlm.m_num_subbanks >= 2) {
    m_bank = (m_port_num / m_pin2tlm.m_num_subbanks);
    m_busy_port = m_bank * m_pin2tlm.m_num_subbanks;
  }
}



bool xtsc_component::xtsc_pin2tlm_memory_transactor::xtsc_respond_if_impl::nb_respond(const xtsc_response& response) {
  XTSC_INFO(m_pin2tlm.m_text, response);
  if (m_pin2tlm.m_pif_or_idma) {
    if (response.get_status() == xtsc_response::RSP_NACC) {
      if (m_pin2tlm.m_waiting_for_nacc[m_port_num]) {
        m_pin2tlm.m_request_got_nacc[m_port_num] = true;
        m_pin2tlm.m_req_rdy_fifo[m_port_num]->write(false);
        m_pin2tlm.m_drive_req_rdy_event[m_port_num].notify(m_pin2tlm.m_output_delay);
        XTSC_DEBUG(m_pin2tlm.m_text, "scheduled PIReqRdy");
        return true;
      }
      else {
        ostringstream oss;
        oss << "xtsc_pin2tlm_memory_transactor '" << m_pin2tlm.name() << "' received nacc too late: " << response << endl;
        oss << " - Possibly something is wrong with the downstream device" << endl;
        throw xtsc_exception(oss.str());
      }
    }
    else {
      if (m_pin2tlm.m_one_at_a_time && response.get_last_transfer()) {
        sc_time now = sc_time_stamp();
        if ((now - m_pin2tlm.m_request_time_stamp[m_port_num]) < m_pin2tlm.m_clock_period) {
          ostringstream oss;
          oss << "xtsc_pin2tlm_memory_transactor '" << m_pin2tlm.name() << "' received response too soon: " << response << endl;
          oss << " When \"one_at_a_time\" is true, non-RSP_NACC responses must come a minimum of 1 clock period after the request." << endl;
          oss << " Request time stamp=" << m_pin2tlm.m_request_time_stamp[m_port_num] << ", clock period=" << m_pin2tlm.m_clock_period << ", now="
              << now;
          throw xtsc_exception(oss.str());
        }
        m_pin2tlm.m_req_rdy_fifo[m_port_num]->write(false);
        m_pin2tlm.m_drive_req_rdy_event[m_port_num].notify(m_pin2tlm.m_output_delay);
        XTSC_INFO(m_pin2tlm.m_text, response << ": scheduled PIReqRdy assert upon last-transfer response");
      }
      if (m_pin2tlm.m_write_responses && (response.get_status() == xtsc_response::RSP_OK) && !response.get_byte_size()) {
        // Drop write response on the floor
        return true;
      }
      if (m_pin2tlm.m_prereject_responses && !m_pin2tlm.m_p_resp_rdy[m_port_num]->read()) {
        XTSC_INFO(m_pin2tlm.m_text, response << " <== Rejected on port #" << m_port_num);
        return false;
      }
      xtsc_response *p_response = m_pin2tlm.new_response(response);
      m_pin2tlm.m_response_fifo[m_port_num].push_back(p_response);
      m_pin2tlm.m_response_event[m_port_num].notify(m_pin2tlm.m_output_delay);
      return true;
    }
  }
  else if (m_pin2tlm.m_axi_or_idma) {
    if (response.get_status() == xtsc_response::RSP_NACC) {
      if (m_pin2tlm.m_waiting_for_nacc[m_port_num]) {
        m_pin2tlm.m_request_got_nacc[m_port_num] = true;
        m_pin2tlm.m_req_rdy_fifo[m_port_num]->write(false);
        m_pin2tlm.m_drive_req_rdy_event[m_port_num].notify(m_pin2tlm.m_output_delay);
        XTSC_DEBUG(m_pin2tlm.m_text, "scheduled AxREADY");
        return true;
      }
      else {
        ostringstream oss;
        oss << "xtsc_pin2tlm_memory_transactor '" << m_pin2tlm.name() << "' received nacc too late: " << response << endl;
        oss << " - Possibly something is wrong with the downstream device" << endl;
        throw xtsc_exception(oss.str());
      }
    }
    else {
      if (m_pin2tlm.m_one_at_a_time && response.get_last_transfer()) {
        sc_time now = sc_time_stamp();
        if ((now - m_pin2tlm.m_request_time_stamp[m_port_num]) < m_pin2tlm.m_clock_period) {
          ostringstream oss;
          oss << "xtsc_pin2tlm_memory_transactor '" << m_pin2tlm.name() << "' received response too soon: " << response << endl;
          oss << " When \"one_at_a_time\" is true, non-RSP_NACC responses must come a minimum of 1 clock period after the request." << endl;
          oss << " Request time stamp=" << m_pin2tlm.m_request_time_stamp[m_port_num] << ", clock period=" << m_pin2tlm.m_clock_period << ", now="
              << now;
          throw xtsc_exception(oss.str());
        }
        m_pin2tlm.m_req_rdy_fifo[m_port_num]->write(false);
        m_pin2tlm.m_drive_req_rdy_event[m_port_num].notify(m_pin2tlm.m_output_delay);
        XTSC_INFO(m_pin2tlm.m_text, response << ": scheduled AxREADY assert upon last-transfer response");
      }
      if (m_pin2tlm.m_prereject_responses && !m_pin2tlm.m_p_resp_rdy[m_port_num]->read()) {
        XTSC_INFO(m_pin2tlm.m_text, response << " <== Rejected on port #" << m_port_num);
        return false;
      }
      xtsc_response *p_response = m_pin2tlm.new_response(response);
      m_pin2tlm.m_response_fifo[m_port_num].push_back(p_response);
      m_pin2tlm.m_response_event[m_port_num].notify(m_pin2tlm.m_output_delay);
      return true;
    }
  }
  else if (xtsc_module_pin_base::is_apb(m_pin2tlm.m_interface_type)) {
    xtsc_response::status_t status = response.get_status();
    if (status == xtsc_response::APB_OK) {
      m_pin2tlm.m_apb_info_table[m_port_num]->m_slverr = false;
      if (m_pin2tlm.m_apb_info_table[m_port_num]->m_write) {
        m_pin2tlm.m_apb_info_table[m_port_num]->m_data = 0;
      }
      else {
        memcpy(&m_pin2tlm.m_apb_info_table[m_port_num]->m_data, response.get_buffer(), m_pin2tlm.m_width8);
      }
    }
    else if (status == xtsc_response::SLVERR) {
      m_pin2tlm.m_apb_info_table[m_port_num]->m_slverr = true;
    }
    else {
      ostringstream oss;
      oss << kind() << " '" << name() << "' received APB response with illegal status: " << response;
      throw xtsc_exception(oss.str());
    }
    m_pin2tlm.m_apb_rsp_event[m_port_num].notify(SC_ZERO_TIME);
    return true;
  }
  else {
    if (m_pin2tlm.m_request_fifo[m_port_num].empty()) {
      ostringstream oss;
      oss << kind() << " '" << name() << "' received a response with no outstanding request";
      throw xtsc_exception(oss.str());
    }
    request_info *p_info = m_pin2tlm.m_request_fifo[m_port_num].front();
    m_pin2tlm.m_request_fifo[m_port_num].pop_front();

    bool is_nacc           = (response.get_status() == xtsc_response::RSP_NACC);
    bool nacc_already_done = false;

    // DRAM0BS|DRAM1BS: Per bank: Check for consistency and arrange to only drive busy once per cycle
    if (m_pin2tlm.m_num_subbanks >= 2) {
      subbank_activity *p_sa = &m_pin2tlm.m_subbank_activity[m_bank][p_info->m_cycle_index];
      bool consistent = false;
      if (is_nacc) {
        nacc_already_done = (p_sa->m_num_rsp_nacc != 0);
        p_sa->m_num_rsp_nacc += 1;
        consistent = (p_sa->m_num_rsp_ok == 0);
      }
      else {
        p_sa->m_num_rsp_ok += 1;
        consistent = (p_sa->m_num_rsp_nacc == 0);
      }
      if (!consistent) {
        ostringstream oss;
        oss << kind() << " '" << name() << "' bank #" << m_bank << " has received inconsistent responses (" << p_sa->m_num_rsp_ok << " RSP_OK and "
            << p_sa->m_num_rsp_nacc << " RSP_NACC) for requests of cycle #" << p_sa->m_cycle_num << ".  Latest response: " << response;
        throw xtsc_exception(oss.str());
      }
    }

    if (is_nacc && !nacc_already_done) {
      if (m_pin2tlm.m_p_busy[m_busy_port]) {
        m_pin2tlm.m_busy_fifo[m_busy_port]->write(true);
        m_pin2tlm.m_drive_busy_event[m_busy_port].notify(m_pin2tlm.m_output_delay);
        XTSC_DEBUG(m_pin2tlm.m_text, "scheduled busy");
      }
      else {
        ostringstream oss;
        oss << kind() << " '" << name() << "' received RSP_NACC but this " << m_pin2tlm.m_interface_uc
            << " interface has no busy signal";
        throw xtsc_exception(oss.str());
      }
      // XLMI0: If the request getting busy (RSP_NACC) was a READ, then the last address added to the deque needs to be removed
      if ((m_pin2tlm.m_interface_type == xtsc_module_pin_base::XLMI0) && (p_info->m_request.get_type() == xtsc_request::READ)) {
        m_pin2tlm.m_load_address_deque[m_port_num].pop_back();
      }
    }
    else if (p_info->m_request.get_type() == xtsc_request::READ) {
      // Schedule read data to be driven
      m_pin2tlm.get_read_data_from_response(response);
      m_pin2tlm.m_read_data_fifo[m_port_num]->write(m_pin2tlm.m_data);
      m_pin2tlm.m_drive_read_data_event[m_port_num].notify(m_pin2tlm.m_output_delay);
      XTSC_DEBUG(m_pin2tlm.m_text, "schedule drive_read_data of 0x" << hex << m_pin2tlm.m_data);
    }
    m_pin2tlm.delete_request_info(p_info);
    return true;
  }
}



void xtsc_component::xtsc_pin2tlm_memory_transactor::xtsc_respond_if_impl::register_port(sc_port_base& port, const char *if_typename) {
  if (m_p_port) {
    ostringstream oss;
    oss << "Illegal multiple binding detected to xtsc_pin2tlm_memory_transactor '" << m_pin2tlm.name() << "' m_respond_exports["
        << m_port_num << "]: " << endl;
    oss << "  " << port.name() << endl;
    oss << "  " << m_p_port->name();
    throw xtsc_exception(oss.str());
  }
  XTSC_INFO(m_pin2tlm.m_text, "Binding '" << port.name() << "' to '" << m_pin2tlm.name() << ".m_respond_exports[" << m_port_num <<
                              "]'");
  m_p_port = &port;
}



