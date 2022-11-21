// Copyright (c) 2007-2018 by Cadence Design Systems Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of
// Cadence Design Systems Inc.  They may be adapted and modified by bona fide
// purchasers for internal use, but neither the original nor any adapted
// or modified version may be disclosed or distributed to third parties
// in any manner, medium, or form, in whole or in part, without the prior
// written consent of Cadence Design Systems Inc.

#include <iostream>
#include <xtsc/xtsc_response.h>
#include <xtsc/xtsc_fast_access.h>
#include <xtsc/xtsc_tlm2pin_memory_transactor.h>
#include <xtsc/xtsc_arbiter.h>
#include <xtsc/xtsc_master.h>
#include <xtsc/xtsc_memory_trace.h>
#include <xtsc/xtsc_router.h>
#if defined(_WIN32)
#else
#include <dlfcn.h>
#endif


/*
   Theory of operation
  
   This module can be used as a transactor for the PIF, IDMA0, AXI, IDMA, APB, inbound PIF|AXI, snoop,
   or for any of the local memory interfaces.  Which interface it is being used for determines which
   pin-level ports and what SystemC processes get instantiated.  The main difference is between the
   PIF and AXI and APB and the local memory interfaces and between split Rd/Wr and non-split Rd/Wr
   local memory interfaces.
   
   Port Data-Member Naming Scheme:
   
   PIF|IDMA0|Inbound PIF|snoop    AXI|IDMA|APB|Inbound AXI     Local Memory Interface
   ---------------------------    -------------------------    -------------------------------------------
   m_p_req*                       m_p_xxx (xxx is lowercase    m_p_xxx  (xxx does not start with req/resp 
   m_p_resp*                      AXI4/APB signal name)        and is not a lower-case AXI4 signal name)
    

   SystemC Processes:

   PIF|IDMA0|Inbound PIF|snoop    AXI|IDMA|Inbound AXI         Local Memory Interface
   ---------------------------    -------------------------    -------------------------------------------
   pif_request_thread             axi_request_thread           lcl_request_thread
   pif_response_thread            axi_response_thread          lcl_busy_write_rsp_thread
   pif_drive_resp_rdy_thread      axi_drive_resp_rdy_thread    lcl_sample_read_data_thread
                                                               lcl_7stage_write_rsp_thread     (7stage only)
                                                               xlmi_retire_thread              (XLMI only)
                                  APB                          xlmi_flush_thread               (XLMI only)
                                  -------------------------    xlmi_load_thread                (XLMI only)
                                  apb_thread                   xlmi_load_method                (XLMI only)
                                                               dram_lock_thread                (DRAM only)
 
   For local memory interface pins that share a common function, the same data member is
   used, but it is given a SystemC name that exactly matches the Xtensa RTL.  For example,
   the m_p_en data member is given the SystemC name of "DPort0En0" when it is used for
   XLMI0 load/store unit 0 and it is given the SystemC name of "DRam0En0" when it is used
   for DRAM0 load/store unit 0.  An example data member for a PIF pin is m_p_req_adrs which
   is given the SystemC name of "POReqAdrs".  An example data member for an AXI4 pin is
   m_p_arid which corresponds to ARID (its SystemC name will be the concatenation of any
   prefix specified by "axi_name_prefix" and "ARID").  An example data member for an APB
   pin is m_p_paddr which corresponds to PADDR (its SystemC name will be the concatenation
   of any prefix specified by "axi_name_prefix" and "PADDR").  The comments in this file
   and the corresponding header file generally omit the digits in the signal names (so
   "DRam0En0", "DRam0En1", "DRam1En0", and "DRam1En1" are all referred to as DRamEn).
  
   Multi-ported memories are supported for all memory interfaces.  This is accomplished
   by having each pin-level port data member be an array with one entry per memory port.
   In addition, one instance of each applicable SystemC thread process is created per
   memory port.  Each thread process knows which memory port it is associated with by
   the local port variable which is created on the stack when the thread process is
   first entered.  This technique does not work for SystemC method processes because
   they always run to completion, so there is only one instance of the xlmi_load_method
   spawned (XLMI with busy only) and it is made sensitive to the necessary events of all
   the memory ports.

   The "sample_phase" and "output_delay" parameters play key roles in the timing of
   the model.  See their discussion in the xtsc_tlm2pin_memory_transactor_parms
   documentation.

   The request-response life cycle is like this:

   To generated a request, the upstream Xtensa TLM master calls nb_request() in this module.
   The nb_request() implementation in this module makes a copy of the request, adds the
   copy to m_request_fifo, and notifies the appropriate thread (pif_request_thread,
   axi_request_thread, or lcl_request_thread) by notifying m_request_event.  From this point
   on, the behavior is different for the PIF, AXI, APB, and the local memory interfaces:


   PIF, IDMA0, Inbound PIF, snoop:

   When pif_request_thread wakes up because m_request_event has been notified, it
   translates the TLM request to a pin-level request and drives the pin-level signals at
   that time (which is delayed from the TLM nb_request call by m_output_delay).  The
   thread then syncs to the sample phase (as specified by the "sample_phase" parameter)
   and samples the "PIReqRdy" input to determine if the request was accepted by the
   downstream pin-level slave.
   
   If the request was not accepted then an RSP_NACC is sent to the upstream TLM master
   (this occurs at the "sample_phase" clock phase).  If the request is accepted and it
   is a write request with its last-transfer flag set and if this module is responsible
   for generating write responses (the "write_responses" parameter is true) then a TLM
   write response of RSP_OK is added to m_write_response_deque and pif_response_thread
   is notify via m_write_response_event.  If necessary, a second wait is performed at
   this point so that the output signals will be driven for one full clock period.
   After being driven for one full clock period, the output signals are deasserted
   (driven to 0).

   A TLM request may or may not be tracked on the pin-level side (grep tracked).  All
   read requests are tracked (grep is_read).  All write requests are also tracked unless
   m_track_write_requests is false (grep "track_write_requests").

   When a tracked TLM request is first received a TLM response is pre-formed and stored
   in m_tran_id_rsp_info.  If the pin-level request gets rejected or if it is not a
   last-transfer, then the pre-formed response gets removed from m_tran_id_rsp_info and
   discarded.  Otherwise, it stays in m_tran_id_rsp_info until a non-RSP_NACC, last-transfer
   response is received.  When the non-RSP_NACC pin-level response is received it is
   matched up with the pre-formed TLM response in m_tran_id_rsp_info.  When a last-transfer
   TLM response is accepted by the upstream TLM master, the response is removed from
   m_tran_id_rsp_info.

   Pin-level responses are matched back up with their corresponding pre-formed TLM
   response using the transaction ID (tran_id).  A transaction ID is formed from the
   route ID, if any, left-shifted by 8 bits plus the PIF ID (POReqId or PIRespId), if
   any.  If there is no PIF ID (m_has_request_id is false), then the transaction ID is
   always 0.  In the m_tran_id_rsp_info map, each transaction ID that has been used is
   mapped to a deque of response_info objects.  This deque holds all outstanding tracked
   response_info objects in the order they were received.  This allows multiple
   outstanding requests with the same transaction ID.

   If an untracked request receives a response from the pin-level side, an exception
   will be thrown unless m_discard_unknown_responses is true (in which case it will be
   discarded, i.e. it will be ignored other than being logged).

   Each clock period that there is something to do, the pif_response_thread wakes up at
   the "sample_phase" phase and does the following (in order):
   1. Checks to see if a new response is available ("PIRespValid" and "PORespRdy" are
      both high) and, if it is, captures it.  If the transaction ID of the response maps
      to a non-empty deque in m_tran_id_rsp_info, then the oldest entry in the deque is
      removed and stored in m_system_response_deque.  Otherwise the response is ignored or
      an exception is thrown depending on m_discard_unknown_responses.
   2. If the previous response was accepted and if it was a last transfer and if there
      are locally generated write responses in m_write_response_deque (see the
      "write_responses" parameter), then they are moved to the front of
      m_system_response_deque.
   3. If there is a response in m_system_response_deque, it is sent to the upstream TLM
      master.  If the upstream TLM master, rejects the response, then "PORespRdy" is
      driven low for one clock period so that no new response will be accepted by this
      transactor (this is handled in pif_drive_resp_rdy_thread which is notified by
      m_drive_resp_rdy_event).  The original response will be sent to the upstream TLM
      master once per clock period until it is accepted (it is too late for this module
      to reject the response at the pin-level interface).


   AXI, IDMA:

   When axi_request_thread wakes up because m_request_event has been notified, it
   translates the TLM request to a pin-level request and drives the pin-level signals at
   that time (which is delayed from the TLM nb_request call by m_output_delay).  For a
   read port, the outputs on the Read Address channel are driven.  For a write port, the
   outputs on the Write Data channel are driven and, if this is a first_transfer, the
   outputs on the Write Address channel are also driven.
   
   The thread then syncs to the sample phase (as specified by the "sample_phase" parameter)
   and samples the "AxREADY" input (read ports and the first_transfer on write ports) and
   the "WREADY" input (write ports only) to determine if the request was accepted by the
   downstream pin-level slave.  For a read port, the request is considered accepted if
   "ARREADY" is asserted.  For a write port, a non-first_transfer request is considered
   accepted if "WREADY" is asserted and a first_transfer request is considered accepted if
   either "AWREADY" or "WREADY" are asserted.  If one is asserted and one is not, then
   axi_request_thread continues to drive VALID signal on the non-READY channel until it is
   accepted and it also sets m_hard_busy true to prevent any more requests being accepted
   until the current write request is fully accepted (both Addr channel and Data channel).

   If the request was not accepted (all applicable READY signals deasserted) then a NOTRDY
   is sent to the upstream TLM master (this occurs at the "sample_phase" clock phase).  
   
   If necessary, a second wait is performed at this point so that the output signals will
   be driven for one full clock period.  After being driven for one full clock period,
   the output signals are deasserted (driven to 0).

   When a TLM request is first received a TLM response is pre-formed and stored in
   m_tran_id_rsp_info.  If the pin-level request gets rejected or if it is not a
   last_transfer, then the pre-formed response gets removed from m_tran_id_rsp_info and
   discarded.  Otherwise, it stays in m_tran_id_rsp_info until a non-NOTRDY, last_transfer
   response is received.  When the non-NOTRDY pin-level response is received it is
   matched up with the pre-formed TLM response in m_tran_id_rsp_info.  When a last_transfer
   TLM response is accepted by the upstream TLM master, the response is removed from
   m_tran_id_rsp_info.

   Pin-level responses are matched back up with their corresponding pre-formed TLM
   response using the transaction ID (tran_id/xID).  A transaction ID is formed from the
   request ID and request route ID as specified by the "axi_id_bits" and
   "axi_route_id_bits" parameters.  In the m_tran_id_rsp_info map, each transaction ID
   that has been used is mapped to a deque of response_info objects.  This deque holds
   all outstanding response_info objects in the order they were received.  This allows
   multiple outstanding requests with the same transaction ID.

   Each clock period that there is something to do, the axi_response_thread wakes up at
   the "sample_phase" phase and does the following (in order):
   1. Checks to see if a new response is available ("xVALID" and "xREADY" are both high)
      and, if it is, captures it.  If the transaction ID of the response maps to a
      non-empty deque in m_tran_id_rsp_info, then the oldest entry in the deque is
      removed and stored in m_system_response_deque.  Otherwise an exception is thrown.
   2. If there is a response in m_system_response_deque, it is sent to the upstream TLM
      master.  If the upstream TLM master, rejects the response, then "xREADY" is
      driven low for one clock period so that no new response will be accepted by this
      transactor (this is handled in axi_drive_resp_rdy_thread which is notified by
      m_drive_resp_rdy_event).  The original response will be sent to the upstream TLM
      master once per clock period until it is accepted (it is too late for this module
      to reject the response at the pin-level interface).


   APB:

   When apb_thread wakes up because m_request_event has been notified, it translates the
   TLM request to a pin-level request and drives the pin-level signals at that time
   (which is delayed from the TLM nb_request call by m_output_delay).  The "PENABLE" signal
   is driven low at this time.  The thread waits one clock period and drives "PENABLE" high.
   
   The thread then syncs to the sample phase (as specified by the "sample_phase" parameter)
   and samples the "PREADY" input to determine if the request was accepted by the
   downstream pin-level slave.  The thread repeats the check at one clock period intervals
   as long as "PREADY" is low.  When "PREADY" is high at the sample phase, the thread
   reads the "PSLVERR" signal and, if doing a read, the "PRDATA" signal.  The TLM response
   is then formed and sent back upstream.
   
   If necessary, a second wait is performed at this point so that the output signals will
   be driven for an integer number of clock periods.  
   
   The output signals are deasserted (driven to 0) whenever there is no pending request.


   Local Memory Interface:

   When lcl_request_thread() wakes up (which is delayed from the TLM nb_request call by
   m_output_delay, see the "output_delay" parameter) because m_request_event has been
   notified, it pre-forms a TLM response and then translates the TLM request to a
   pin-level request and drives the pin-level signals.  It then schedules various events
   as appropriate, waits one clock period, and then deasserts the outputs by driving
   them with all 0's.  The events that may get scheduled are:

   m_load_event_queue:  (XLMI read only)
   This event queue is notified twice: first with a delay of m_clock_period and second 
   with a delay of 2*m_clock_period.  The first notification is to assert DPortLoad and
   it is notified with a delay of m_clock_period so that DPortLoad is asserted one clock
   cycle later then the other signals.  The second notification is to deassert
   DPortLoad.  The assert/deassert values are stored in m_load_deque.  The
   xlmi_load_thread waits for this event queue.

   m_busy_write_rsp_event_queue:
   This event is notified if the interface has a busy signal or if the request is a
   write.  The event is notified with a delay calculated to aligned with the sample
   phase.  The lcl_busy_write_rsp_thread waits for this event and then executes the
   following algorithm at the sample phase:
     - If interface has busy:
         - If busy is asserted:
             - Send RSP_NACC TLM response
         - If busy is not asserted and request is a read:
             - Schedule read data to be sample now (5-stage) or after 1 clock period
               (7-stage).  This scheduling is done by m_read_data_event_queue.notify().
         - If busy is not asserted and request is a write:
             - (5-stage) Send RSP_OK TLM write response
             - (7-stage) Tell lcl_7stage_write_rsp_thread to send RSP_OK TLM write response
     - If interface has no busy (then this must be a write):
         - Send RSP_OK TLM write response

   m_read_data_event_queue:
   This event only applies to read requests.  This event is notified by lcl_request_thread
   if the interface has no busy signal and it is notified by lcl_busy_write_rsp_thread if
   the interface has a busy signal and busy is not asserted.  The event is notified with a
   delay calculated to aligned with the sample phase of the appropriate cycle for sampling
   the read data.  The lcl_sample_read_data_thread handles this event.

   When the lcl_sample_read_data_thread() thread wakes up at the sample phase, it gets
   the pre-formed response from m_read_data_rsp_deque, copies the read data from the
   pin-level read data bus into the TLM response buffer, and sends the TLM response to
   the upstream TLM master.

   The xlmi_retire_thread drives m_p_retire (DPortLoadRetired) based on m_retire_event
   and m_retire_deassert which are controlled by nb_load_retired().

   The xlmi_flush_thread drives m_p_flush (DPortRetireFlush) based on m_flush_event and
   m_flush_deassert which are controlled by nb_retire_flush().

   The xlmi_load_thread drives m_p_load (DPortLoad) for an XLMI without a busy and it
   drives m_p_preload for an XLMI with a busy.  It is controlled by m_load_event_queue
   which gets notified by lcl_request_thread.

   The xlmi_load_method drives m_p_load (DPortLoad) for an XLMI with a busy.  Its job is
   to qualify the load signal (m_p_preload) with the busy signal (m_p_busy/DPortBusy).

   The dram_lock_thread drives m_p_lock (DRamLock) based upon m_lock_event_queue and
   m_lock_deque which are controlled by nb_lock().  
   
   For DRAM0BS|DRAM1BS, if "has_busy" is true, then m_p_busy[port] is only non-NULL for
   the 0th subbank of each bank: ((port % m_num_subbanks) == 0)

   For DRAM0BS|DRAM1BS, if "has_lock" is true, then m_p_lock[port] is only non-NULL for
   the 0th subbank of each bank: ((port % m_num_subbanks) == 0)


 */



using namespace std;
#if SYSTEMC_VERSION >= 20050601
using namespace sc_core;
#endif
using namespace sc_dt;
using namespace log4xtensa;
using namespace xtsc;



typedef xtsc::xtsc_request::type_t type_t;



#if defined(_WIN32)
static const char *win_error() {
  static char buffer[256];
  DWORD error = GetLastError();
  if (!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, 0, buffer, sizeof(buffer), NULL)) {
    ostringstream oss;
    oss << " GetLastError()=" << error;
    strcpy(buffer, oss.str().c_str());
  }
  return buffer;
}
#endif



// ctor helper
static u32 get_num_ports(const xtsc_component::xtsc_tlm2pin_memory_transactor_parms& tlm2pin_parms) {
  return tlm2pin_parms.get_non_zero_u32("num_ports");
}



// ctor helper
static sc_trace_file *get_trace_file(const xtsc_component::xtsc_tlm2pin_memory_transactor_parms& tlm2pin_parms) {
  return static_cast<sc_trace_file*>(const_cast<void*>(tlm2pin_parms.get_void_pointer("vcd_handle")));
}



// ctor helper
static const char *get_suffix(const xtsc_component::xtsc_tlm2pin_memory_transactor_parms& tlm2pin_parms) {
  const char *port_name_suffix = tlm2pin_parms.get_c_str("port_name_suffix");
  static const char *blank = "";
  return port_name_suffix ? port_name_suffix : blank;
}



// ctor helper
static bool get_banked(const xtsc_component::xtsc_tlm2pin_memory_transactor_parms& tlm2pin_parms) {
  return tlm2pin_parms.get_bool("banked");
}



// ctor helper
static bool get_split_rw(const xtsc_component::xtsc_tlm2pin_memory_transactor_parms& tlm2pin_parms) {
  using namespace xtsc_component;
  string interface_uc = xtsc_module_pin_base::get_interface_uc(tlm2pin_parms.get_c_str("memory_interface"));
  xtsc_module_pin_base::memory_interface_type interface_type = xtsc_module_pin_base::get_interface_type(interface_uc);
  return ((interface_type == xtsc_module_pin_base::DRAM0RW) || (interface_type == xtsc_module_pin_base::DRAM1RW));
}



// ctor helper
static bool get_dma(const xtsc_component::xtsc_tlm2pin_memory_transactor_parms& tlm2pin_parms) {
  return tlm2pin_parms.get_bool("has_dma");
}



// ctor helper
static u32 get_subbanks(const xtsc_component::xtsc_tlm2pin_memory_transactor_parms& tlm2pin_parms) {
  return tlm2pin_parms.get_u32("num_subbanks");
}



xtsc_component::xtsc_tlm2pin_memory_transactor_parms::xtsc_tlm2pin_memory_transactor_parms(const xtsc_core&     core,
                                                                                           const char          *memory_interface,
                                                                                           u32                  num_ports)
{
  xtsc_core::memory_port mem_port = xtsc_core::get_memory_port(memory_interface, &core);
  if (!core.has_memory_port(mem_port)) {
    ostringstream oss;
    oss << "xtsc_tlm2pin_memory_transactor_parms: xtsc_core '" << core.name() << "' doesn't have a \"" << memory_interface
        << "\" memory port.";
    throw xtsc_exception(oss.str());
  }
  xtsc_module_pin_base::memory_interface_type interface_type = xtsc_module_pin_base::get_interface_type(mem_port);

  // Infer number of ports if num_ports is 0
  if (!num_ports) {
    u32 mpc = core.get_multi_port_count(mem_port);
    // Normally be single-ported ...
    num_ports = 1;
    // ; however, if this core interface is multi-ported (e.g. has banks, split Rd/Wr, or multiple LD/ST units without CBox) AND ...
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

  xtsc_address  start_address8  = 0;    // PIF|AXI|XLMI0
  u32           width8          = core.get_memory_byte_width(mem_port);
  u32           address_bits    = 32;   // PIF|AXI|XLMI0
  u32           banks           = 1;
  u32           subbanks        = 0;

  if (xtsc_module_pin_base::is_system_memory(interface_type)) {
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
  set("read_delay", core_parms.get_u32("LocalMemoryLatency") - 1);
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

  if ((interface_type == xtsc_module_pin_base::DRAM0  ) ||
      (interface_type == xtsc_module_pin_base::DRAM0BS) ||
      (interface_type == xtsc_module_pin_base::DRAM1  ) ||
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
      vec.clear(); vec.push_back(8); vec.push_back(8); vec.push_back(4); set("axi_id_bits",    vec);
      vec.clear(); vec.push_back(0); vec.push_back(1); vec.push_back(0); set("axi_read_write", vec);
      vec.clear(); vec.push_back(d); vec.push_back(d); vec.push_back(i); set("axi_port_type",  vec);
      set("axi_name_prefix", "DataMaster,DataMaster,InstMaster");
    } else if (combine_master_axi_ports && (num_ports == 2)) { 
      vector<u32> vec;
      u32 d = core_parms.get_u32("DataMasterType");
      //Note read channel carries both instruction and data, hence the extra axi-id-bit.
      vec.clear(); vec.push_back(8+1); vec.push_back(8); set("axi_id_bits",    vec);
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

  if (xtsc_module_pin_base::is_apb(interface_type)) {
    if (num_ports == 1) {
      set("axi_name_prefix", "APBMaster");
    }
  }

}



xtsc_component::xtsc_tlm2pin_memory_transactor::xtsc_tlm2pin_memory_transactor(
    sc_module_name                              module_name,
    const xtsc_tlm2pin_memory_transactor_parms& tlm2pin_parms
  ) :
  sc_module             (module_name),
  xtsc_module           (*(sc_module*)this),
  xtsc_module_pin_base  (*this, ::get_num_ports (tlm2pin_parms),
                                ::get_trace_file(tlm2pin_parms),
                                ::get_suffix    (tlm2pin_parms),
                                ::get_banked    (tlm2pin_parms),
                                ::get_split_rw  (tlm2pin_parms),
                                ::get_dma       (tlm2pin_parms),
                                ::get_subbanks  (tlm2pin_parms)),
  m_num_ports           (tlm2pin_parms.get_non_zero_u32("num_ports")),
  m_num_axi_rd_ports    (0),
  m_num_axi_wr_ports    (0),
  m_num_axi_sn_ports    (0),
  m_request_fifo_depth  (tlm2pin_parms.get_non_zero_u32("request_fifo_depth")),
  m_interface_uc        (get_interface_uc(tlm2pin_parms.get_c_str("memory_interface"))),
  m_interface_lc        (get_interface_lc(tlm2pin_parms.get_c_str("memory_interface"))),
  m_interface_type      (xtsc_module_pin_base::get_interface_type(m_interface_uc)),
  m_width8              (tlm2pin_parms.get_non_zero_u32("byte_width")),
  m_dram_attribute_width(tlm2pin_parms.get_u32("dram_attribute_width")),
  m_dram_attribute      (160),
  m_dram_attribute_bv   (160),
  m_start_byte_address  (tlm2pin_parms.get_u32("start_byte_address")),
  m_read_delay          (tlm2pin_parms.get_u32("read_delay")),
  m_is_7_stage          (m_read_delay == 1),
  m_inbound_pif         (false),
  m_snoop               (false),
  m_has_coherence       (false),
  m_has_pif_attribute   (false),
  m_has_pif_req_domain  (false),
  m_deny_fast_access    (tlm2pin_parms.get_u32_vector("deny_fast_access")),
  m_initial_value_file  (""),
  m_req_user_data       (""),
  m_req_user_data_name  (""),
  m_req_user_data_width1(0),
  m_rsp_user_data       (""),
  m_rsp_user_data_name  (""),
  m_rsp_user_data_width1(0),
  m_user_data_val       (0),
  m_address_bits        (is_system_memory(m_interface_type) ? 32 : tlm2pin_parms.get_non_zero_u32("address_bits")),
  m_check_bits          (tlm2pin_parms.get_u32("check_bits")),
  m_ram_select_bit      (tlm2pin_parms.get_u32("ram_select_bit")),
  m_ram_select_normal   (tlm2pin_parms.get_bool("ram_select_normal")),
  m_route_id_bits       (tlm2pin_parms.get_u32("route_id_bits")),
  m_route_id_mask       ((1 << m_route_id_bits) - 1),
  m_address             (m_address_bits),
  m_vadrs               (6),
  m_req_coh_cntl        (2),
  m_lane                (get_number_of_enables(m_interface_type, m_width8)),
  m_id                  (m_id_bits),
  m_priority            (is_axi_or_idma(m_interface_type) ? 4 : 2),
  m_route_id            (m_route_id_bits ? m_route_id_bits : 1),
  m_req_attribute       (12),
  m_req_domain          (2),
  m_len                 (8),
  m_size                (3),
  m_burst               (2),
  m_cache               (4),
  m_prot                (3),
  m_bar                 (2),
  m_domain              (2),
  m_data                ((int)m_width8*8),
  m_req_cntl            (0),
  m_text                (TextLogger::getInstance(name())),
  m_tran_id_rsp_info    (0),
  m_pending_rsp_info_cnt(0)
{

  const char *axi_name_prefix = tlm2pin_parms.get_c_str("axi_name_prefix");

  if (m_interface_type == PIF) {
    m_inbound_pif       = tlm2pin_parms.get_bool("inbound_pif");
    m_snoop             = tlm2pin_parms.get_bool("snoop");
    if (m_inbound_pif && m_snoop) {
      ostringstream oss;
      oss << kind() << " '" << name() << "':  You cannot have both \"inbound_pif\" and \"snoop\" set to true";
      throw xtsc_exception(oss.str());
    }
    if (!m_inbound_pif && !m_snoop) {
      m_has_coherence = tlm2pin_parms.get_bool("has_coherence");
      m_has_pif_req_domain = tlm2pin_parms.get_bool("has_pif_req_domain");
    }
    if (!m_snoop) {
      m_has_pif_attribute = tlm2pin_parms.get_bool("has_pif_attribute");
    }
  }
  else if (m_interface_type == IDMA0) {
    m_has_pif_attribute   = tlm2pin_parms.get_bool("has_pif_attribute");
    m_has_pif_req_domain  = tlm2pin_parms.get_bool("has_pif_req_domain");
  }
  else if (is_axi_or_idma(m_interface_type)) {
    m_axi_id_bits         = tlm2pin_parms.get_u32_vector("axi_id_bits");
    m_axi_route_id_bits   = tlm2pin_parms.get_u32_vector("axi_route_id_bits");
    m_axi_read_write      = tlm2pin_parms.get_u32_vector("axi_read_write");
    m_axi_port_type       = tlm2pin_parms.get_u32_vector("axi_port_type");
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

  m_big_endian                = tlm2pin_parms.get_bool("big_endian");
  m_has_request_id            = tlm2pin_parms.get_bool("has_request_id");
  m_write_responses           = tlm2pin_parms.get_bool("write_responses");
  m_track_write_requests      = tlm2pin_parms.get_bool("track_write_requests");
  m_discard_unknown_responses = tlm2pin_parms.get_bool("discard_unknown_responses");
  m_has_busy                  = tlm2pin_parms.get_bool("has_busy");
  m_has_lock                  = tlm2pin_parms.get_bool("has_lock");
  m_has_xfer_en               = tlm2pin_parms.get_bool("has_xfer_en");
  m_bus_addr_bits_mask        = ((m_width8 ==  4) ? 0x03 :
                                 (m_width8 ==  8) ? 0x07 : 
                                 (m_width8 == 16) ? 0x0F :
                                 (m_width8 == 32) ? 0x1F :
                                                    0x3F);
  m_cbox                      = tlm2pin_parms.get_bool("cbox");
  m_split_rw                  = ::get_split_rw(tlm2pin_parms);
  m_has_dma                   = tlm2pin_parms.get_bool("has_dma");
  m_external_udma             = tlm2pin_parms.get_bool("external_udma");
  m_cosim                     = tlm2pin_parms.get_bool("cosim");
  m_shadow_memory             = tlm2pin_parms.get_bool("shadow_memory");
  m_test_shadow_memory        = tlm2pin_parms.get_bool("test_shadow_memory");
  m_dso_name                  = tlm2pin_parms.get_c_str("dso_name") ? tlm2pin_parms.get_c_str("dso_name") : "";
  m_dso_cookie                = xtsc_copy_c_str(tlm2pin_parms.get_c_str("dso_cookie"));
  m_memory_fill_byte          = (u8) tlm2pin_parms.get_u32("memory_fill_byte");
  m_pif_id_mask               = (m_has_request_id ? m_id_mask : 0);
  m_p_memory                  = NULL;
  m_dso                       = NULL;
  m_peek                      = NULL;
  m_poke                      = NULL;
  m_ram_select_mask           = 0;

  if (m_external_udma) {
    bool okay = false;
    ostringstream oss;
    oss << kind() << " '" << name() << "': Error: When \"external_udma\" is true, then ";
    if (m_interface_type != DRAM0) {
      oss << "\"memory_interface\" must be \"DRAM0\".";
    }
    else if (m_num_ports != 1) {
      oss << "\"num_ports\" must be 1.";
    }
    else if (m_address_bits != 32) {
      oss << "\"address_bits\" must be 32.";
    }
    else if (m_start_byte_address != 0) {
      oss << "\"start_byte_address\" must be 0.";
    }
    else if (m_check_bits != 0) {
      oss << "\"check_bits\" must be 0.";
    }
    else if (m_ram_select_bit >= 32) {
      oss << "\"ram_select_bit\" must be less than 32.";
    }
    else if (m_banked) {
      oss << "\"banked\" must be false.";
    }
    else if (m_has_lock) {
      oss << "\"has_lock\" must be false.";
    }
    else if (m_has_xfer_en) {
      oss << "\"has_xfer_en\" must be false.";
    }
    else if (!m_has_busy) {
      oss << "\"has_busy\" must be true.";
    }
    else {
      okay = true;
    }
    if (!okay) {
      throw xtsc_exception(oss.str());
    }
    m_ram_select_mask = 1 << m_ram_select_bit;
  }
  if (m_write_responses) {
    // The "track_write_response" parameter is ignored when "write_responses" is true.  In this case we set the
    // m_track_write_requests variable to false (regardless of the parameter setting).
    m_track_write_requests = false;
  }

  const char *initial_value_file = tlm2pin_parms.get_c_str("initial_value_file");
  if (initial_value_file) {
    m_initial_value_file  = xtsc_copy_c_str(initial_value_file);
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

  if (is_pif_or_idma(m_interface_type) || is_inst_mem(m_interface_type)) {
    if ((m_width8 != 4) && (m_width8 != 8) && (m_width8 != 16) && (m_width8 != 32)) {
      ostringstream oss;
      oss << kind() << " '" << name() << "': Invalid \"byte_width\"= " << m_width8
          << " (legal values for IRAM0|IRAM1|IROM0|PIF|IDMA0 are 4, 8, 16, and 32)";
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
      if (m_axi_read_write[i] >  2) { // read/write/snoop
        ostringstream oss;
        oss << kind() << " '" << name() << "': Invalid \"axi_read_write[" << i << "]\"=" << m_axi_read_write[i] << " (legal values are 0 and 1)";
        throw xtsc_exception(oss.str());
      }
      m_axi_signal_indx.push_back((m_axi_read_write[i]==2) ? m_num_axi_sn_ports++
                                : (m_axi_read_write[i]==1) ? m_num_axi_wr_ports++ 
                                : m_num_axi_rd_ports++);

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
    m_address_mask      = 0xFFFFFFFF;
  }
  else if (m_interface_type == XLMI0) {
    m_address_shift     = 0;
    m_address_mask      = (m_address_bits >= 32) ? 0xFFFFFFFF : (1 << m_address_bits) - 1;
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
    if (m_external_udma) {
      m_address_shift  = 0;
      m_address_mask   = 0xFFFFFFFF;
    }
  }

  if (is_pif_or_idma(m_interface_type)) {
#if defined(__x86_64__) || defined(_M_X64)
    u32 max_bits = 64;
#else
    u32 max_bits = 32;
#endif
    parse_port_name_and_bit_width(tlm2pin_parms, "req_user_data", m_req_user_data, m_req_user_data_name, m_req_user_data_width1, 1, max_bits);
    parse_port_name_and_bit_width(tlm2pin_parms, "rsp_user_data", m_rsp_user_data, m_rsp_user_data_name, m_rsp_user_data_width1, 1, max_bits);
    if (m_req_user_data_width1) {
      m_user_data_val = new sc_bv_base((int)m_req_user_data_width1);
    }
  }

  // Get clock period 
  m_time_resolution = sc_get_time_resolution();
  u32 clock_period = tlm2pin_parms.get_u32("clock_period");
  if (clock_period == 0xFFFFFFFF) {
    m_clock_period = xtsc_get_system_clock_period();
  }
  else {
    m_clock_period = m_time_resolution * clock_period;
  }
  m_clock_period_value = m_clock_period.value();
  u32 posedge_offset = tlm2pin_parms.get_u32("posedge_offset");
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

  m_read_delay_time = m_clock_period * m_read_delay;

  // Get sample phase
  u32 sample_phase = tlm2pin_parms.get_u32("sample_phase");
  if (sample_phase == 0xFFFFFFFF) {
    if (is_system_memory(m_interface_type)) {
      m_sample_phase = 0 * m_time_resolution;
    }
    else {
      u32 pdf_a, pdf_b, pdf_c;
      xtsc_core::get_clock_phase_delta_factors(pdf_a, pdf_b, pdf_c);
      if (pdf_a == 0) {
        pdf_a = xtsc_get_system_clock_factor();
      }
      m_sample_phase = (pdf_a - 1) * m_time_resolution;
    }
  }
  else {
    m_sample_phase = sample_phase * m_time_resolution;
  }
  m_sample_phase_plus_one = m_sample_phase + m_clock_period;
  if (m_sample_phase >= m_clock_period) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': Sample phase of " << m_sample_phase << " (from \"sample_phase\"= " << sample_phase
        << ") must be less than clock period of " << m_clock_period;
    throw xtsc_exception(oss.str());
  }


  m_output_delay = tlm2pin_parms.get_u32("output_delay") * m_time_resolution;

  // Create all the "per mem port" stuff

  m_request_exports             = new sc_export<xtsc_request_if>*       [m_num_ports];
  m_request_impl                = new xtsc_request_if_impl*             [m_num_ports];
  m_debug_cap                   = new xtsc_debug_if_cap*                [m_num_ports];
  m_request_fifo                = new deque<xtsc_request*>              [m_num_ports];
  m_respond_ports               = new sc_port<xtsc_respond_if>*         [m_num_ports];
  m_debug_ports                 = new sc_port<xtsc_debug_if, NSPP>*     [m_num_ports];
  m_request_event               = new sc_event                          [m_num_ports];


  m_write_response_event        = 0;
  m_drive_resp_rdy_event        = 0;
  m_resp_rdy_fifo               = 0;
  m_system_response_deque       = 0;
  m_write_response_deque        = 0;
  m_previous_response_last      = 0;
  m_transfer_number             = 0;
  m_busy_write_rsp_deque        = 0;
  m_busy_write_rsp_event_queue  = 0;
  m_7stage_write_rsp_event_queue= 0;
  m_read_data_rsp_deque         = 0;
  m_read_data_event_queue       = 0;
  m_retire_event                = 0;
  m_flush_event                 = 0;
  m_retire_deassert             = 0;
  m_flush_deassert              = 0;
  m_lock_event_queue            = 0;
  m_load_event_queue            = 0;
  m_lock_deque                  = 0;
  m_load_deque                  = 0;

  m_hard_busy                   = new bool                              [m_num_ports];

  if (is_pif_or_idma(m_interface_type)) {
    m_tran_id_rsp_info          = new rsp_info_deque_map                [m_num_ports];
    m_write_response_event      = new sc_event                          [m_num_ports];
    m_drive_resp_rdy_event      = new sc_event                          [m_num_ports];
    m_resp_rdy_fifo             = new bool_fifo*                        [m_num_ports];
    m_system_response_deque     = new deque<response_info*>             [m_num_ports];
    m_write_response_deque      = new deque<response_info*>             [m_num_ports];
    m_previous_response_last    = new bool                              [m_num_ports];
    m_transfer_number           = new u32                               [m_num_ports];
  }
  else if (is_axi_or_idma(m_interface_type)) {
    m_tran_id_rsp_info          = new rsp_info_deque_map                [m_num_ports];
    m_drive_resp_rdy_event      = new sc_event                          [m_num_ports];
    m_resp_rdy_fifo             = new bool_fifo*                        [m_num_ports];
    m_system_response_deque     = new deque<response_info*>             [m_num_ports];
    m_transfer_number           = new u32                               [m_num_ports];
  }
  else if (is_apb(m_interface_type)) {
    ; // Nothing to do
  }
  else {
    m_busy_write_rsp_deque      = new deque<response_info*>             [m_num_ports];
    m_7stage_write_rsp_deque    = new deque<response_info*>             [m_num_ports];
    m_busy_write_rsp_event_queue= new sc_event_queue                    [m_num_ports];
    m_7stage_write_rsp_event_queue= new sc_event_queue                  [m_num_ports];
    m_read_data_rsp_deque       = new deque<response_info*>             [m_num_ports];
    m_read_data_event_queue     = new sc_event_queue                    [m_num_ports];
  }

  if (m_interface_type == XLMI0) {
    m_retire_event              = new sc_event                          [m_num_ports];
    m_flush_event               = new sc_event                          [m_num_ports];
    m_retire_deassert           = new sc_time                           [m_num_ports];
    m_flush_deassert            = new sc_time                           [m_num_ports];
    m_load_event_queue          = new sc_event_queue                    [m_num_ports];
    m_load_deque                = new deque<bool>                       [m_num_ports];
  }

  if (((m_interface_type == DRAM0  ) ||
       (m_interface_type == DRAM0BS) ||
       (m_interface_type == DRAM0RW) ||
       (m_interface_type == DRAM1  ) ||
       (m_interface_type == DRAM1BS) ||
       (m_interface_type == DRAM1RW)) && m_has_lock)
  {
    m_lock_event_queue          = new sc_event_queue                    [m_num_ports];
    m_lock_deque                = new deque<bool>                       [m_num_ports];
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
  m_p_acaddr            = NULL;
  m_p_acprot            = NULL;
  m_p_acsnoop           = NULL;
  m_p_acvalid           = NULL;
  m_p_acready           = NULL;
  m_p_cddata            = NULL;
  m_p_cdlast            = NULL;
  m_p_cdvalid           = NULL;
  m_p_cdready           = NULL;
  m_p_crresp            = NULL;
  m_p_crvalid           = NULL;
  m_p_crready           = NULL;
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
  m_p_preload           = NULL;
  m_p_retire            = NULL;
  m_p_flush             = NULL;
  m_p_lock              = NULL;
  m_p_attr              = NULL;
  m_p_check_wr          = NULL;
  m_p_check             = NULL;
  m_p_xfer_en           = NULL;
  m_p_busy              = NULL;
  m_p_data              = NULL;
  m_p_ram0              = NULL;
  m_p_ram1              = NULL;

  if (is_pif_or_idma(m_interface_type)) {
    m_p_req_valid               = new bool_output*                      [m_num_ports];
    m_p_req_cntl                = new uint_output*                      [m_num_ports];
    m_p_req_adrs                = new uint_output*                      [m_num_ports];
    m_p_req_data                = new wide_output*                      [m_num_ports];
    m_p_req_data_be             = new uint_output*                      [m_num_ports];
    m_p_req_id                  = new uint_output*                      [m_num_ports];
    m_p_req_priority            = new uint_output*                      [m_num_ports];
    m_p_req_route_id            = new uint_output*                      [m_num_ports];
    m_p_req_attribute           = new uint_output*                      [m_num_ports];
    m_p_req_domain              = new uint_output*                      [m_num_ports];
    m_p_req_coh_vadrs           = new uint_output*                      [m_num_ports];
    m_p_req_coh_cntl            = new uint_output*                      [m_num_ports];
    m_p_req_user_data           = new wide_output*                      [m_num_ports];
    m_p_req_rdy                 = new bool_input*                       [m_num_ports];
    m_p_resp_valid              = new bool_input*                       [m_num_ports];
    m_p_resp_cntl               = new uint_input*                       [m_num_ports];
    m_p_resp_data               = new wide_input*                       [m_num_ports];
    m_p_resp_id                 = new uint_input*                       [m_num_ports];
    m_p_resp_priority           = new uint_input*                       [m_num_ports];
    m_p_resp_route_id           = new uint_input*                       [m_num_ports];
    m_p_resp_coh_cntl           = new uint_input*                       [m_num_ports];
    m_p_resp_user_data          = new wide_input*                       [m_num_ports];
    m_p_resp_rdy                = new bool_output*                      [m_num_ports];
  }
  else if (is_axi_or_idma(m_interface_type)) {
    u32 num_rd_ports = m_num_axi_rd_ports;
    u32 num_wr_ports = m_num_axi_wr_ports;
    u32 num_sn_ports = m_num_axi_sn_ports;
    if (num_rd_ports) {
      m_p_arid                  = new uint_output*                      [num_rd_ports];
      m_p_araddr                = new uint_output*                      [num_rd_ports];
      m_p_arlen                 = new uint_output*                      [num_rd_ports];
      m_p_arsize                = new uint_output*                      [num_rd_ports];
      m_p_arburst               = new uint_output*                      [num_rd_ports];
      m_p_arlock                = new bool_output*                      [num_rd_ports];
      m_p_arcache               = new uint_output*                      [num_rd_ports];
      m_p_arprot                = new uint_output*                      [num_rd_ports];
      m_p_arqos                 = new uint_output*                      [num_rd_ports];
      m_p_arbar                 = new uint_output*                      [num_rd_ports];
      m_p_ardomain              = new uint_output*                      [num_rd_ports];
      m_p_arsnoop               = new uint_output*                      [num_rd_ports];
      m_p_arvalid               = new bool_output*                      [num_rd_ports];
      m_p_arready               = new bool_input*                       [num_rd_ports];
      m_p_rid                   = new uint_input*                       [num_rd_ports];
      m_p_rdata                 = new wide_input*                       [num_rd_ports];
      m_p_rresp                 = new uint_input*                       [num_rd_ports];
      m_p_rlast                 = new bool_input*                       [num_rd_ports];
      m_p_rvalid                = new bool_input*                       [num_rd_ports];
      m_p_rready                = new bool_output*                      [num_rd_ports];
    }
    if (num_wr_ports) {
      m_p_awid                  = new uint_output*                      [num_wr_ports];
      m_p_awaddr                = new uint_output*                      [num_wr_ports];
      m_p_awlen                 = new uint_output*                      [num_wr_ports];
      m_p_awsize                = new uint_output*                      [num_wr_ports];
      m_p_awburst               = new uint_output*                      [num_wr_ports];
      m_p_awlock                = new bool_output*                      [num_wr_ports];
      m_p_awcache               = new uint_output*                      [num_wr_ports];
      m_p_awprot                = new uint_output*                      [num_wr_ports];
      m_p_awqos                 = new uint_output*                      [num_wr_ports];
      m_p_awbar                 = new uint_output*                      [num_wr_ports];
      m_p_awdomain              = new uint_output*                      [num_wr_ports];
      m_p_awsnoop               = new uint_output*                      [num_wr_ports];
      m_p_awvalid               = new bool_output*                      [num_wr_ports];
      m_p_awready               = new bool_input*                       [num_wr_ports];
      m_p_wdata                 = new wide_output*                      [num_wr_ports];
      m_p_wstrb                 = new uint_output*                      [num_wr_ports];
      m_p_wlast                 = new bool_output*                      [num_wr_ports];
      m_p_wvalid                = new bool_output*                      [num_wr_ports];
      m_p_wready                = new bool_input*                       [num_wr_ports];
      m_p_bid                   = new uint_input*                       [num_wr_ports];
      m_p_bresp                 = new uint_input*                       [num_wr_ports];
      m_p_bvalid                = new bool_input*                       [num_wr_ports];
      m_p_bready                = new bool_output*                      [num_wr_ports];
    }
    if (num_sn_ports) {
      m_p_acaddr                = new uint_output*                      [num_sn_ports];
      m_p_acprot                = new uint_output*                      [num_sn_ports];
      m_p_acsnoop               = new uint_output*                      [num_sn_ports];
      m_p_acvalid               = new bool_output*                      [num_sn_ports];
      m_p_acready               = new bool_input*                       [num_sn_ports];
      m_p_cddata                = new wide_input*                       [num_sn_ports];
      m_p_cdlast                = new bool_input*                       [num_sn_ports];
      m_p_cdvalid               = new bool_input*                       [num_sn_ports];
      m_p_cdready               = new bool_output*                      [num_sn_ports];
      m_p_crresp                = new uint_input*                       [num_sn_ports];
      m_p_crvalid               = new bool_input*                       [num_sn_ports];
      m_p_crready               = new bool_output*                      [num_sn_ports];
    }
  }
  else if (is_apb(m_interface_type)) {
    m_p_psel                    = new bool_output*                      [m_num_ports];
    m_p_penable                 = new bool_output*                      [m_num_ports];
    m_p_pwrite                  = new bool_output*                      [m_num_ports];
    m_p_paddr                   = new uint_output*                      [m_num_ports];
    m_p_pprot                   = new uint_output*                      [m_num_ports];
    m_p_pstrb                   = new uint_output*                      [m_num_ports];
    m_p_pwdata                  = new wide_output*                      [m_num_ports];
    m_p_pready                  = new bool_input*                       [m_num_ports];
    m_p_pslverr                 = new bool_input*                       [m_num_ports];
    m_p_prdata                  = new wide_input*                       [m_num_ports];
  }
  else {
    m_p_en                      = new bool_output*                      [m_num_ports];
    m_p_addr                    = new uint_output*                      [m_num_ports];
    m_p_lane                    = new uint_output*                      [m_num_ports];
    m_p_wrdata                  = new wide_output*                      [m_num_ports];
    m_p_wr                      = new bool_output*                      [m_num_ports];
    m_p_load                    = new bool_output*                      [m_num_ports];
    m_p_preload                 = new bool_signal*                      [m_num_ports];
    m_p_retire                  = new bool_output*                      [m_num_ports];
    m_p_flush                   = new bool_output*                      [m_num_ports];
    m_p_lock                    = new bool_output*                      [m_num_ports];
    m_p_attr                    = new wide_output*                      [m_num_ports];
    m_p_check_wr                = new wide_output*                      [m_num_ports];
    m_p_check                   = new wide_input*                       [m_num_ports];
    m_p_xfer_en                 = new bool_output*                      [m_num_ports];
    m_p_busy                    = new bool_input*                       [m_num_ports];
    m_p_data                    = new wide_input*                       [m_num_ports];
    m_p_ram0                    = new bool_output*                      [m_num_ports];
    m_p_ram1                    = new bool_output*                      [m_num_ports];
  }


  for (u32 port=0; port<m_num_ports; ++port) {

    ostringstream oss1;
    oss1 << "m_request_exports[" << port << "]";
    m_request_exports[port]     = new sc_export<xtsc_request_if>(oss1.str().c_str());

    ostringstream oss2;
    oss2 << "m_request_impl[" << port << "]";
    m_request_impl[port]        = new xtsc_request_if_impl(oss2.str().c_str(), *this, port);

    (*m_request_exports[port])(*m_request_impl[port]);

    ostringstream oss3;
    oss3 << "m_respond_ports[" << port << "]";
    m_respond_ports[port]       = new sc_port<xtsc_respond_if>(oss3.str().c_str());

    ostringstream oss4;
    oss4 << "m_debug_ports[" << port << "]";
    m_debug_ports[port]         = new sc_port<xtsc_debug_if, NSPP>(oss4.str().c_str());

    if (is_pif_or_idma(m_interface_type) || is_axi_or_idma(m_interface_type)) {
      ostringstream oss2;
      oss2 << "m_resp_rdy_fifo[" << port << "]";
      m_resp_rdy_fifo[port]      = new bool_fifo(oss2.str().c_str());
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
      else if (m_axi_read_write[port] == 1) {
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
      } else {
        m_p_acaddr          [indx] = NULL;
        m_p_acprot          [indx] = NULL;
        m_p_acsnoop         [indx] = NULL;
        m_p_acvalid         [indx] = NULL;
        m_p_acready         [indx] = NULL;
        m_p_cddata          [indx] = NULL;
        m_p_cdlast          [indx] = NULL;
        m_p_cdvalid         [indx] = NULL;
        m_p_cdready         [indx] = NULL;
        m_p_crresp          [indx] = NULL;
        m_p_crvalid         [indx] = NULL;
        m_p_crready         [indx] = NULL;
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
      m_p_preload           [port] = NULL;
      m_p_retire            [port] = NULL;
      m_p_flush             [port] = NULL;
      m_p_lock              [port] = NULL;
      m_p_attr              [port] = NULL;
      m_p_check_wr          [port] = NULL;
      m_p_check             [port] = NULL;
      m_p_xfer_en           [port] = NULL;
      m_p_busy              [port] = NULL;
      m_p_data              [port] = NULL;
      m_p_ram0              [port] = NULL;
      m_p_ram1              [port] = NULL;
    }

    switch (m_interface_type) {
      case DRAM0BS:
      case DRAM0: {
        if (m_external_udma) {
        m_p_addr            [port] = &add_uint_output("DmaAddr",                m_address_bits,  m_append_id, port);
        m_p_lane            [port] = &add_uint_output("DmaByteEn",              m_width8,        m_append_id, port);
        m_p_data            [port] = &add_wide_input ("DmaData",                m_width8*8,      m_append_id, port);
        m_p_en              [port] = &add_bool_output("DmaEn",                                   m_append_id, port);
        m_p_wr              [port] = &add_bool_output("DmaWr",                                   m_append_id, port);
        m_p_wrdata          [port] = &add_wide_output("DmaWrData",              m_width8*8,      m_append_id, port);
        m_p_busy            [port] = &add_bool_input ("DmaRdy",                                  m_append_id, port);
        m_p_ram0            [port] = &add_bool_output("DmaDRam0",                                m_append_id, port);
        m_p_ram1            [port] = &add_bool_output("DmaDRam1",                                m_append_id, port);
        }
        else {
        m_p_addr            [port] = &add_uint_output("DRam0Addr",              m_address_bits,  m_append_id, port);
        m_p_lane            [port] = &add_uint_output("DRam0ByteEn",            m_width8,        m_append_id, port);
        m_p_data            [port] = &add_wide_input ("DRam0Data",              m_width8*8,      m_append_id, port);
        m_p_en              [port] = &add_bool_output("DRam0En",                                 m_append_id, port);
        m_p_wr              [port] = &add_bool_output("DRam0Wr",                                 m_append_id, port);
        m_p_wrdata          [port] = &add_wide_output("DRam0WrData",            m_width8*8,      m_append_id, port);
        if (m_has_lock && ((m_num_subbanks < 2) || ((port % m_num_subbanks) == 0))) {
        m_p_lock            [port] = &add_bool_output("DRam0Lock",                               m_append_id, port, true);
        }
        if (m_has_xfer_en) {
        m_p_xfer_en         [port] = &add_bool_output("XferDRam0En",                             m_append_id, port);
        }
        if (m_has_busy && ((m_num_subbanks < 2) || ((port % m_num_subbanks) == 0))) {
          m_p_busy          [port] = &add_bool_input ("DRam0Busy",                               m_append_id, port, true);
        }
        if (m_check_bits) {
        m_p_check_wr        [port] = &add_wide_output("DRam0CheckWrData",       m_check_bits,    m_append_id, port);
        m_p_check           [port] = &add_wide_input ("DRam0CheckData",         m_check_bits,    m_append_id, port);
        }
        }
        break;
      }
      case DRAM1BS:
      case DRAM1: {
        m_p_addr            [port] = &add_uint_output("DRam1Addr",              m_address_bits,  m_append_id, port);
        m_p_lane            [port] = &add_uint_output("DRam1ByteEn",            m_width8,        m_append_id, port);
        m_p_data            [port] = &add_wide_input ("DRam1Data",              m_width8*8,      m_append_id, port);
        m_p_en              [port] = &add_bool_output("DRam1En",                                 m_append_id, port);
        m_p_wr              [port] = &add_bool_output("DRam1Wr",                                 m_append_id, port);
        m_p_wrdata          [port] = &add_wide_output("DRam1WrData",            m_width8*8,      m_append_id, port);
        if (m_has_lock && ((m_num_subbanks < 2) || ((port % m_num_subbanks) == 0))) {
        m_p_lock            [port] = &add_bool_output("DRam1Lock",                               m_append_id, port, true);
        }
        if (m_has_xfer_en) {
        m_p_xfer_en         [port] = &add_bool_output("XferDRam1En",                             m_append_id, port);
        }
        if (m_has_busy && ((m_num_subbanks < 2) || ((port % m_num_subbanks) == 0))) {
          m_p_busy          [port] = &add_bool_input ("DRam1Busy",                               m_append_id, port, true);
        }
        if (m_check_bits) {
        m_p_check_wr        [port] = &add_wide_output("DRam1CheckWrData",       m_check_bits,    m_append_id, port);
        m_p_check           [port] = &add_wide_input ("DRam1CheckData",         m_check_bits,    m_append_id, port);
        }
        break;
      }
      case DRAM0RW: {
      if ((port & 0x1) == 0) {
        // Rd ports.  Does not use m_p_wr, m_p_wrdata, m_p_check_wr, 
        m_p_addr            [port] = &add_uint_output("DRam0Addr",              m_address_bits,  m_append_id, port);
        m_p_lane            [port] = &add_uint_output("DRam0ByteEn",            m_width8,        m_append_id, port);
        m_p_data            [port] = &add_wide_input ("DRam0Data",              m_width8*8,      m_append_id, port);
        m_p_en              [port] = &add_bool_output("DRam0En",                                 m_append_id, port);
        if (m_has_lock) {
        m_p_lock            [port] = &add_bool_output("DRam0Lock",                               m_append_id, port);
        }
        if (m_has_busy) {
          m_p_busy          [port] = &add_bool_input ("DRam0Busy",                               m_append_id, port);
        }
        if (m_dram_attribute_width) {
        m_p_attr            [port] = &add_wide_output("DRam0Attr",      m_dram_attribute_width,  m_append_id, port);
        }
        if (m_check_bits) {
        m_p_check           [port] = &add_wide_input ("DRam0CheckData",         m_check_bits,    m_append_id, port);
        }
      }
      else {
        // Wr ports.  Does not use m_p_en, m_p_data, m_p_lock, m_p_check
        m_p_addr            [port] = &add_uint_output("DRam0WrAddr",            m_address_bits,  m_append_id, port);
        m_p_lane            [port] = &add_uint_output("DRam0WrByteEn",          m_width8,        m_append_id, port);
        m_p_wr              [port] = &add_bool_output("DRam0Wr",                                 m_append_id, port);
        m_p_wrdata          [port] = &add_wide_output("DRam0WrData",            m_width8*8,      m_append_id, port);
        if (m_has_busy) {
          m_p_busy          [port] = &add_bool_input ("DRam0WrBusy",                             m_append_id, port);
        }
        if (m_dram_attribute_width) {
        m_p_attr            [port] = &add_wide_output("DRam0WrAttr",    m_dram_attribute_width,  m_append_id, port);
        }
        if (m_check_bits) {
        m_p_check_wr        [port] = &add_wide_output("DRam0CheckWrData",       m_check_bits,    m_append_id, port);
        }
      }
        break;
      }
      case DRAM1RW: {
      if ((port & 0x1) == 0) {
        // Rd ports.  Does not use m_p_wr, m_p_wrdata, m_p_check_wr, 
        m_p_addr            [port] = &add_uint_output("DRam1Addr",              m_address_bits,  m_append_id, port);
        m_p_lane            [port] = &add_uint_output("DRam1ByteEn",            m_width8,        m_append_id, port);
        m_p_data            [port] = &add_wide_input ("DRam1Data",              m_width8*8,      m_append_id, port);
        m_p_en              [port] = &add_bool_output("DRam1En",                                 m_append_id, port);
        if (m_has_lock) {
        m_p_lock            [port] = &add_bool_output("DRam1Lock",                               m_append_id, port);
        }
        if (m_has_busy) {
          m_p_busy          [port] = &add_bool_input ("DRam1Busy",                               m_append_id, port);
        }
        if (m_dram_attribute_width) {
        m_p_attr            [port] = &add_wide_output("DRam1Attr",      m_dram_attribute_width,  m_append_id, port);
        }
        if (m_check_bits) {
        m_p_check           [port] = &add_wide_input ("DRam1CheckData",         m_check_bits,    m_append_id, port);
        }
      }
      else {
        // Wr ports.  Does not use m_p_en, m_p_data, m_p_lock, m_p_check
        m_p_addr            [port] = &add_uint_output("DRam1WrAddr",            m_address_bits,  m_append_id, port);
        m_p_lane            [port] = &add_uint_output("DRam1WrByteEn",          m_width8,        m_append_id, port);
        m_p_wr              [port] = &add_bool_output("DRam1Wr",                                 m_append_id, port);
        m_p_wrdata          [port] = &add_wide_output("DRam1WrData",            m_width8*8,      m_append_id, port);
        if (m_has_busy) {
          m_p_busy          [port] = &add_bool_input ("DRam1WrBusy",                             m_append_id, port);
        }
        if (m_dram_attribute_width) {
        m_p_attr            [port] = &add_wide_output("DRam1WrAttr",    m_dram_attribute_width,  m_append_id, port);
        }
        if (m_check_bits) {
        m_p_check_wr        [port] = &add_wide_output("DRam1CheckWrData",       m_check_bits,    m_append_id, port);
        }
      }
        break;
      }
      case DROM0: {
        m_p_addr            [port] = &add_uint_output("DRom0Addr",              m_address_bits,  m_append_id, port);
        m_p_lane            [port] = &add_uint_output("DRom0ByteEn",            m_width8,        m_append_id, port);
        m_p_data            [port] = &add_wide_input ("DRom0Data",              m_width8*8,      m_append_id, port);
        m_p_en              [port] = &add_bool_output("DRom0En",                                 m_append_id, port);
        if (m_has_busy) {
          m_p_busy          [port] = &add_bool_input ("DRom0Busy",                               m_append_id, port);
        }
        break;
      }
      case IRAM0: {
        m_p_addr            [port] = &add_uint_output("IRam0Addr",              m_address_bits,  m_append_id, port);
        m_p_data            [port] = &add_wide_input ("IRam0Data",              m_width8*8,      m_append_id, port);
        m_p_en              [port] = &add_bool_output("IRam0En",                                 m_append_id, port);
        if (m_width8 >= 8) {
          m_p_lane          [port] = &add_uint_output("IRam0WordEn",            m_width8/4,      m_append_id, port);
        }
        m_p_load            [port] = &add_bool_output("IRam0LoadStore",                          m_append_id, port);
        m_p_wr              [port] = &add_bool_output("IRam0Wr",                                 m_append_id, port);
        m_p_wrdata          [port] = &add_wide_output("IRam0WrData",            m_width8*8,      m_append_id, port);
        if (m_has_xfer_en) {
        m_p_xfer_en         [port] = &add_bool_output("XferIRam0En",                             m_append_id, port);
        }
        if (m_has_busy) {
          m_p_busy          [port] = &add_bool_input ("IRam0Busy",                               m_append_id, port);
        }
        if (m_check_bits) {
        m_p_check_wr        [port] = &add_wide_output("IRam0CheckWrData",       m_check_bits,    m_append_id, port);
        m_p_check           [port] = &add_wide_input ("IRam0CheckData",         m_check_bits,    m_append_id, port);
        }
        break;
      }
      case IRAM1: {
        m_p_addr            [port] = &add_uint_output("IRam1Addr",              m_address_bits,  m_append_id, port);
        m_p_data            [port] = &add_wide_input ("IRam1Data",              m_width8*8,      m_append_id, port);
        m_p_en              [port] = &add_bool_output("IRam1En",                                 m_append_id, port);
        if (m_width8 >= 8) {
          m_p_lane          [port] = &add_uint_output("IRam1WordEn",            m_width8/4,      m_append_id, port);
        }
        m_p_load            [port] = &add_bool_output("IRam1LoadStore",                          m_append_id, port);
        m_p_wr              [port] = &add_bool_output("IRam1Wr",                                 m_append_id, port);
        m_p_wrdata          [port] = &add_wide_output("IRam1WrData",            m_width8*8,      m_append_id, port);
        if (m_has_xfer_en) {
        m_p_xfer_en         [port] = &add_bool_output("XferIRam1En",                             m_append_id, port);
        }
        if (m_has_busy) {
          m_p_busy          [port] = &add_bool_input ("IRam1Busy",                               m_append_id, port);
        }
        if (m_check_bits) {
        m_p_check_wr        [port] = &add_wide_output("IRam1CheckWrData",       m_check_bits,    m_append_id, port);
        m_p_check           [port] = &add_wide_input ("IRam1CheckData",         m_check_bits,    m_append_id, port);
        }
        break;
      }
      case IROM0: {
        m_p_addr            [port] = &add_uint_output("IRom0Addr",              m_address_bits,  m_append_id, port);
        m_p_data            [port] = &add_wide_input ("IRom0Data",              m_width8*8,      m_append_id, port);
        m_p_en              [port] = &add_bool_output("IRom0En",                                 m_append_id, port);
        if (m_width8 >= 8) {
          m_p_lane          [port] = &add_uint_output("IRom0WordEn",            m_width8/4,      m_append_id, port);
        }
        m_p_load            [port] = &add_bool_output("IRom0Load",                               m_append_id, port);
        if (m_has_xfer_en) {
        m_p_xfer_en         [port] = &add_bool_output("XferIRom0En",                             m_append_id, port);
        }
        if (m_has_busy) {
          m_p_busy          [port] = &add_bool_input ("IRom0Busy",                               m_append_id, port);
        }
        break;
      }
      case URAM0: {
        m_p_addr            [port] = &add_uint_output("URam0Addr",              m_address_bits,  m_append_id, port);
        m_p_lane            [port] = &add_uint_output("URam0ByteEn",            m_width8,        m_append_id, port);
        m_p_data            [port] = &add_wide_input ("URam0Data",              m_width8*8,      m_append_id, port);
        m_p_en              [port] = &add_bool_output("URam0En",                                 m_append_id, port);
        m_p_load            [port] = &add_bool_output("URam0LoadStore",                          m_append_id, port);
        m_p_wr              [port] = &add_bool_output("URam0Wr",                                 m_append_id, port);
        m_p_wrdata          [port] = &add_wide_output("URam0WrData",            m_width8*8,      m_append_id, port);
        if (m_has_xfer_en) {
        m_p_xfer_en         [port] = &add_bool_output("XferURam0En",                             m_append_id, port);
        }
        if (m_has_busy) {
          m_p_busy          [port] = &add_bool_input ("URam0Busy",                               m_append_id, port);
        }
        break;
      }
      case XLMI0: {
        m_p_en              [port] = &add_bool_output("DPort0En",                                m_append_id, port);
        m_p_addr            [port] = &add_uint_output("DPort0Addr",             m_address_bits,  m_append_id, port);
        m_p_lane            [port] = &add_uint_output("DPort0ByteEn",           m_width8,        m_append_id, port);
        m_p_wr              [port] = &add_bool_output("DPort0Wr",                                m_append_id, port);
        m_p_wrdata          [port] = &add_wide_output("DPort0WrData",           m_width8*8,      m_append_id, port);
        m_p_load            [port] = &add_bool_output("DPort0Load",                              m_append_id, port);
        m_p_data            [port] = &add_wide_input ("DPort0Data",             m_width8*8,      m_append_id, port);
        m_p_retire          [port] = &add_bool_output("DPort0LoadRetired",                       m_append_id, port);
        m_p_flush           [port] = &add_bool_output("DPort0RetireFlush",                       m_append_id, port);
        if (m_has_busy) {
          m_p_busy          [port] = &add_bool_input ("DPort0Busy",                              m_append_id, port);
          ostringstream preload;
          preload << "preload" << port;
          m_p_preload       [port] = new bool_signal(preload.str().c_str());
        }
        break;
      }
      case IDMA0:
      case PIF: {
        if (m_inbound_pif) {
        m_p_req_valid       [port] = &add_bool_output("PIReqValid",                              m_append_id, port);
        m_p_req_cntl        [port] = &add_uint_output("PIReqCntl",              8,               m_append_id, port);
        m_p_req_adrs        [port] = &add_uint_output("PIReqAdrs",              m_address_bits,  m_append_id, port);
        m_p_req_data        [port] = &add_wide_output("PIReqData",              m_width8*8,      m_append_id, port);
        m_p_req_data_be     [port] = &add_uint_output("PIReqDataBE",            m_width8,        m_append_id, port);
        m_p_req_priority    [port] = &add_uint_output("PIReqPriority",          2,               m_append_id, port);
        m_p_req_rdy         [port] = &add_bool_input ("POReqRdy",                                m_append_id, port);
        m_p_resp_valid      [port] = &add_bool_input ("PORespValid",                             m_append_id, port);
        m_p_resp_cntl       [port] = &add_uint_input ("PORespCntl",             8,               m_append_id, port);
        m_p_resp_data       [port] = &add_wide_input ("PORespData",             m_width8*8,      m_append_id, port);
        m_p_resp_priority   [port] = &add_uint_input ("PORespPriority",         2,               m_append_id, port);
        m_p_resp_rdy        [port] = &add_bool_output("PIRespRdy",                               m_append_id, port);
        if (m_has_request_id) {
          m_p_req_id        [port] = &add_uint_output("PIReqId",                m_id_bits,       m_append_id, port);
          m_p_resp_id       [port] = &add_uint_input ("PORespId",               m_id_bits,       m_append_id, port);
        }
        if (m_route_id_bits) {
          m_p_req_route_id  [port] = &add_uint_output("PIReqRouteId",           m_route_id_bits, m_append_id, port);
          m_p_resp_route_id [port] = &add_uint_input ("PORespRouteId",          m_route_id_bits, m_append_id, port);
        }
        if (m_has_pif_attribute) {
          m_p_req_attribute [port] = &add_uint_output("PIReqAttribute",         12,              m_append_id, port);
        }
        }
        else if (m_snoop) {
        m_p_req_valid       [port] = &add_bool_output("SnoopReqValid",                           m_append_id, port);
        m_p_req_cntl        [port] = &add_uint_output("SnoopReqCntl",           8,               m_append_id, port);
        m_p_req_coh_cntl    [port] = &add_uint_output("SnoopReqCohCntl",        2,               m_append_id, port);
        m_p_req_adrs        [port] = &add_uint_output("SnoopReqAdrs",           m_address_bits,  m_append_id, port);
        m_p_req_priority    [port] = &add_uint_output("SnoopReqPriority",       2,               m_append_id, port);
        m_p_req_coh_vadrs   [port] = &add_uint_output("SnoopReqCohVAdrsIndex",  6,               m_append_id, port);
        m_p_req_rdy         [port] = &add_bool_input ("SnoopReqRdy",                             m_append_id, port);
        m_p_resp_valid      [port] = &add_bool_input ("SnoopRespValid",                          m_append_id, port);
        m_p_resp_cntl       [port] = &add_uint_input ("SnoopRespCntl",          8,               m_append_id, port);
        m_p_resp_coh_cntl   [port] = &add_uint_input ("SnoopRespCohCntl",       2,               m_append_id, port);
        m_p_resp_data       [port] = &add_wide_input ("SnoopRespData",          m_width8*8,      m_append_id, port);
        m_p_resp_rdy        [port] = &add_bool_output("SnoopRespRdy",                            m_append_id, port);
        if (m_has_request_id) {
          m_p_req_id        [port] = &add_uint_output("SnoopReqId",             m_id_bits,       m_append_id, port);
          m_p_resp_id       [port] = &add_uint_input ("SnoopRespId",            m_id_bits,       m_append_id, port);
        }
        }
        else {
        // outbound PIF/IDMA0
        string sx((m_interface_type == IDMA0) ? "_iDMA" : "");
        string workaround_binutils_2_17_50_assembler_hang_when_compiling_using_gcc_4_1_2_and_using_IUS141_or_IUS142_SystemC_headers("");
        m_p_req_valid       [port] = &add_bool_output("POReqValid"        +sx,                   m_append_id, port);
        m_p_req_cntl        [port] = &add_uint_output("POReqCntl"         +sx,  8,               m_append_id, port);
        m_p_req_adrs        [port] = &add_uint_output("POReqAdrs"         +sx,  m_address_bits,  m_append_id, port);
        m_p_req_data        [port] = &add_wide_output("POReqData"         +sx,  m_width8*8,      m_append_id, port);
        m_p_req_data_be     [port] = &add_uint_output("POReqDataBE"       +sx,  m_width8,        m_append_id, port);
        m_p_req_priority    [port] = &add_uint_output("POReqPriority"     +sx,  2,               m_append_id, port);
        m_p_req_rdy         [port] = &add_bool_input ("PIReqRdy"          +sx,                   m_append_id, port);
        m_p_resp_valid      [port] = &add_bool_input ("PIRespValid"       +sx,                   m_append_id, port);
        m_p_resp_cntl       [port] = &add_uint_input ("PIRespCntl"        +sx,  8,               m_append_id, port);
        m_p_resp_data       [port] = &add_wide_input ("PIRespData"        +sx,  m_width8*8,      m_append_id, port);
        m_p_resp_priority   [port] = &add_uint_input ("PIRespPriority"    +sx,  2,               m_append_id, port);
        m_p_resp_rdy        [port] = &add_bool_output("PORespRdy"         +sx,                   m_append_id, port);
        if (m_has_request_id) {
          m_p_req_id        [port] = &add_uint_output("POReqId"           +sx,  m_id_bits,       m_append_id, port);
          m_p_resp_id       [port] = &add_uint_input ("PIRespId"          +sx,  m_id_bits,       m_append_id, port);
        }
        if (m_route_id_bits) {
          m_p_req_route_id  [port] = &add_uint_output("POReqRouteId"      +sx,  m_route_id_bits, m_append_id, port);
          m_p_resp_route_id [port] = &add_uint_input ("PIRespRouteId"     +sx,  m_route_id_bits, m_append_id, port);
        }
        if (m_has_coherence) {
          m_p_req_coh_vadrs [port] = &add_uint_output("POReqCohVAdrsIndex"+sx,  6,               m_append_id, port);
          m_p_req_coh_cntl  [port] = &add_uint_output("POReqCohCntl"      +sx,  2,               m_append_id, port);
          m_p_resp_coh_cntl [port] = &add_uint_input ("PIRespCohCntl"     +sx,  2,               m_append_id, port);
        }
        if (m_has_pif_attribute) {
          m_p_req_attribute [port] = &add_uint_output("POReqAttribute"    +sx,  12,              m_append_id, port);
        }
        if (m_has_pif_req_domain) {
          m_p_req_domain    [port] = &add_uint_output("POReqDomain"       +sx,  2,               m_append_id, port);
        }
        }
        if (m_req_user_data_width1) {
        m_p_req_user_data   [port] = &add_wide_output(m_req_user_data_name, m_req_user_data_width1, m_append_id, port);
        }
        if (m_rsp_user_data_width1) {
        m_p_resp_user_data  [port] = &add_wide_input (m_rsp_user_data_name, m_rsp_user_data_width1, m_append_id, port);
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
          m_p_arid           [indx] = &add_uint_output(px+"ARID",     tid_bits,  false, port);
          m_p_araddr         [indx] = &add_uint_output(px+"ARADDR",   32,        false, port);
          m_p_arlen          [indx] = &add_uint_output(px+"ARLEN",    8,         false, port);
          m_p_arsize         [indx] = &add_uint_output(px+"ARSIZE",   3,         false, port);
          m_p_arburst        [indx] = &add_uint_output(px+"ARBURST",  2,         false, port);
          m_p_arlock         [indx] = &add_bool_output(px+"ARLOCK",              false, port);
          m_p_arcache        [indx] = &add_uint_output(px+"ARCACHE",  4,         false, port);
          m_p_arprot         [indx] = &add_uint_output(px+"ARPROT",   3,         false, port);
          m_p_arqos          [indx] = &add_uint_output(px+"ARQOS",    4,         false, port);
          if (m_axi_port_type[port] != AXI4) {
          m_p_arbar          [indx] = &add_uint_output(px+"ARBAR",    2,         false, port);
          m_p_ardomain       [indx] = &add_uint_output(px+"ARDOMAIN", 2,         false, port);
          m_p_arsnoop        [indx] = &add_uint_output(px+"ARSNOOP",  4,         false, port);
          }
          m_p_arvalid        [indx] = &add_bool_output(px+"ARVALID",             false, port);
          m_p_arready        [indx] = &add_bool_input (px+"ARREADY",             false, port);
          m_p_rid            [indx] = &add_uint_input (px+"RID",      tid_bits,  false, port);
          m_p_rdata          [indx] = &add_wide_input (px+"RDATA",    data_bits, false, port);
          m_p_rresp          [indx] = &add_uint_input (px+"RRESP",    resp_bits, false, port);
          m_p_rlast          [indx] = &add_bool_input (px+"RLAST",               false, port);
          m_p_rvalid         [indx] = &add_bool_input (px+"RVALID",              false, port);
          m_p_rready         [indx] = &add_bool_output(px+"RREADY",              false, port);
        }
        else if (m_axi_read_write[port] == 1) { // write
          m_p_awid           [indx] = &add_uint_output(px+"AWID",     tid_bits,  false, port);
          m_p_awaddr         [indx] = &add_uint_output(px+"AWADDR",   32,        false, port);
          m_p_awlen          [indx] = &add_uint_output(px+"AWLEN",    8,         false, port);
          m_p_awsize         [indx] = &add_uint_output(px+"AWSIZE",   3,         false, port);
          m_p_awburst        [indx] = &add_uint_output(px+"AWBURST",  2,         false, port);
          m_p_awlock         [indx] = &add_bool_output(px+"AWLOCK",              false, port);
          m_p_awcache        [indx] = &add_uint_output(px+"AWCACHE",  4,         false, port);
          m_p_awprot         [indx] = &add_uint_output(px+"AWPROT",   3,         false, port);
          m_p_awqos          [indx] = &add_uint_output(px+"AWQOS",    4,         false, port);
          if (m_axi_port_type[port] != AXI4) {
          m_p_awbar          [indx] = &add_uint_output(px+"AWBAR",    2,         false, port);
          m_p_awdomain       [indx] = &add_uint_output(px+"AWDOMAIN", 2,         false, port);
          m_p_awsnoop        [indx] = &add_uint_output(px+"AWSNOOP",  3,         false, port);
          }
          m_p_awvalid        [indx] = &add_bool_output(px+"AWVALID",             false, port);
          m_p_awready        [indx] = &add_bool_input (px+"AWREADY",             false, port);
          m_p_wdata          [indx] = &add_wide_output(px+"WDATA",    data_bits, false, port);
          m_p_wstrb          [indx] = &add_uint_output(px+"WSTRB",    strb_bits, false, port);
          m_p_wlast          [indx] = &add_bool_output(px+"WLAST",               false, port);
          m_p_wvalid         [indx] = &add_bool_output(px+"WVALID",              false, port);
          m_p_wready         [indx] = &add_bool_input (px+"WREADY",              false, port);
          m_p_bid            [indx] = &add_uint_input (px+"BID",      tid_bits,  false, port);
          m_p_bresp          [indx] = &add_uint_input (px+"BRESP",    2,         false, port);
          m_p_bvalid         [indx] = &add_bool_input (px+"BVALID",              false, port);
          m_p_bready         [indx] = &add_bool_output(px+"BREADY",              false, port);
        }
        else { // snoop
          m_p_acaddr         [indx] = &add_uint_output(px+"ACADDR",   32,        false, port);
          m_p_acprot         [indx] = &add_uint_output(px+"ACPROT",   3,         false, port);
          m_p_acsnoop        [indx] = &add_uint_output(px+"ACSNOOP",  4,         false, port);
          m_p_acvalid        [indx] = &add_bool_output(px+"ACVALID",             false, port);
          m_p_acready        [indx] = &add_bool_input (px+"ACREADY",             false, port);
          m_p_cddata         [indx] = &add_wide_input (px+"CDDATA",   data_bits, false, port);
          m_p_cdlast         [indx] = &add_bool_input (px+"CDLAST",              false, port);
          m_p_cdvalid        [indx] = &add_bool_input (px+"CDVALID",             false, port);
          m_p_cdready        [indx] = &add_bool_output(px+"CDREADY",             false, port);
          m_p_crresp         [indx] = &add_uint_input (px+"CRRESP",   5,         false, port);
          m_p_crvalid        [indx] = &add_bool_input (px+"CRVALID",             false, port);
          m_p_crready        [indx] = &add_bool_output(px+"CRREADY",             false, port);
        }
        break;
      }
      case APB: {
        string px(m_axi_name_prefix[port]);
        m_p_psel            [port] = &add_bool_output(px+"PSEL",               m_append_id, port);
        m_p_penable         [port] = &add_bool_output(px+"PENABLE",            m_append_id, port);
        m_p_pwrite          [port] = &add_bool_output(px+"PWRITE",             m_append_id, port);
        m_p_paddr           [port] = &add_uint_output(px+"PADDR",      32,     m_append_id, port);
        m_p_pprot           [port] = &add_uint_output(px+"PPROT",      3,      m_append_id, port);
        m_p_pstrb           [port] = &add_uint_output(px+"PSTRB",      4,      m_append_id, port);
        m_p_pwdata          [port] = &add_wide_output(px+"PWDATA",     32,     m_append_id, port);
        m_p_pready          [port] = &add_bool_input (px+"PREADY",             m_append_id, port);
        m_p_pslverr         [port] = &add_bool_input (px+"PSLVERR",            m_append_id, port);
        m_p_prdata          [port] = &add_wide_input (px+"PRDATA",     32,     m_append_id, port);
        break;
      }
    }

  }

  // Squelch SystemC's complaint about multiple thread "objects"
  sc_actions original_action = sc_report_handler::set_actions("object already exists", SC_WARNING, SC_DO_NOTHING);
  for (u32 port=0; port<m_num_ports; ++port) {
    if (is_pif_or_idma(m_interface_type)) {
      ostringstream oss1;
      oss1 << "pif_request_thread[" << port << "]";
      declare_thread_process(pif_request_thread_handle, oss1.str().c_str(), SC_CURRENT_USER_MODULE, pif_request_thread);
      m_process_handles.push_back(sc_get_current_process_handle());
      ostringstream oss2;
      oss2 << "pif_response_thread[" << port << "]";
      declare_thread_process(pif_response_thread_handle, oss2.str().c_str(), SC_CURRENT_USER_MODULE, pif_response_thread);
      m_process_handles.push_back(sc_get_current_process_handle());
      sensitive << m_p_resp_valid[port]->pos() << m_p_resp_rdy[port]->pos() << m_write_response_event[port];
      ostringstream oss3;
      oss3 << "pif_drive_resp_rdy_thread[" << port << "]";
      declare_thread_process(pif_drive_resp_rdy_thread_handle, oss3.str().c_str(), SC_CURRENT_USER_MODULE, pif_drive_resp_rdy_thread);
      m_process_handles.push_back(sc_get_current_process_handle());
    }
    else if (is_axi_or_idma(m_interface_type)) {
      ostringstream oss1;
      oss1 << "axi_request_thread[" << port << "]";
      declare_thread_process(axi_request_thread_handle, oss1.str().c_str(), SC_CURRENT_USER_MODULE, axi_request_thread);
      m_process_handles.push_back(sc_get_current_process_handle());
      ostringstream oss2;
      oss2 << "axi_response_thread[" << port << "]";
      declare_thread_process(axi_response_thread_handle, oss2.str().c_str(), SC_CURRENT_USER_MODULE, axi_response_thread);
      m_process_handles.push_back(sc_get_current_process_handle());
      bool read_channel  = (m_axi_read_write[port] == 0);
      bool write_channel = (m_axi_read_write[port] == 1);
      u32  indx         = m_axi_signal_indx[port];
      if      (read_channel)  { sensitive << m_p_rvalid [indx]->pos() << m_p_rready [indx]->pos(); }
      else if (write_channel) { sensitive << m_p_bvalid [indx]->pos() << m_p_bready [indx]->pos(); }
      else                    { sensitive << m_p_cdvalid[indx]->pos() << m_p_cdready[indx]->pos() 
                                          << m_p_crvalid[indx]->pos() << m_p_crready[indx]->pos(); }
      ostringstream oss3;
      oss3 << "axi_drive_resp_rdy_thread[" << port << "]";
      declare_thread_process(axi_drive_resp_rdy_thread_handle, oss3.str().c_str(), SC_CURRENT_USER_MODULE, axi_drive_resp_rdy_thread);
      m_process_handles.push_back(sc_get_current_process_handle());
    }
    else if (is_apb(m_interface_type)) {
      ostringstream oss;
      oss << "apb_thread[" << port << "]";
      declare_thread_process(axi_request_thread_handle, oss.str().c_str(), SC_CURRENT_USER_MODULE, apb_thread);
      m_process_handles.push_back(sc_get_current_process_handle());
    }
    else {
      ostringstream oss1;
      oss1 << "lcl_request_thread[" << port << "]";
      declare_thread_process(lcl_request_thread_handle, oss1.str().c_str(), SC_CURRENT_USER_MODULE, lcl_request_thread);
      m_process_handles.push_back(sc_get_current_process_handle());
      ostringstream oss2;
      oss2 << "lcl_busy_write_rsp_thread[" << port << "]";
      declare_thread_process(lcl_busy_write_rsp_thread_handle, oss2.str().c_str(),
                             SC_CURRENT_USER_MODULE, lcl_busy_write_rsp_thread);
      m_process_handles.push_back(sc_get_current_process_handle());
      sensitive << m_busy_write_rsp_event_queue[port];
      if (m_is_7_stage) {
        ostringstream oss3;
        oss3 << "lcl_7stage_write_rsp_thread[" << port << "]";
        declare_thread_process(lcl_7stage_write_rsp_thread_handle, oss3.str().c_str(),
                               SC_CURRENT_USER_MODULE, lcl_7stage_write_rsp_thread);
        m_process_handles.push_back(sc_get_current_process_handle());
        sensitive << m_7stage_write_rsp_event_queue[port];
      }
      if (!m_split_rw || ((port & 0x1) == 0)) {
        ostringstream oss3;
        oss3 << "lcl_sample_read_data_thread[" << port << "]";
        declare_thread_process(lcl_sample_read_data_thread_handle, oss3.str().c_str(),
                               SC_CURRENT_USER_MODULE, lcl_sample_read_data_thread);
        m_process_handles.push_back(sc_get_current_process_handle());
        sensitive << m_read_data_event_queue[port];
        if (((m_interface_type == DRAM0  ) ||
             (m_interface_type == DRAM0BS) ||
             (m_interface_type == DRAM0RW) ||
             (m_interface_type == DRAM1  ) ||
             (m_interface_type == DRAM1BS) ||
             (m_interface_type == DRAM1RW)) && m_has_lock && ((m_num_subbanks < 2) || ((port % m_num_subbanks) == 0)))
        {
          ostringstream oss1;
          oss1 << "dram_lock_thread[" << port << "]";
          declare_thread_process(dram_lock_thread_handle, oss1.str().c_str(), SC_CURRENT_USER_MODULE, dram_lock_thread);
          m_process_handles.push_back(sc_get_current_process_handle());
          sensitive << m_lock_event_queue[port];
        }
      }
      if (m_interface_type == XLMI0) {
        ostringstream oss1;
        oss1 << "xlmi_retire_thread[" << port << "]";
        declare_thread_process(xlmi_retire_thread_handle, oss1.str().c_str(), SC_CURRENT_USER_MODULE, xlmi_retire_thread);
        m_process_handles.push_back(sc_get_current_process_handle());
        ostringstream oss2;
        oss2 << "xlmi_flush_thread[" << port << "]";
        declare_thread_process(xlmi_flush_thread_handle, oss2.str().c_str(), SC_CURRENT_USER_MODULE, xlmi_flush_thread);
        m_process_handles.push_back(sc_get_current_process_handle());
        ostringstream oss3;
        oss3 << "xlmi_load_thread[" << port << "]";
        declare_thread_process(xlmi_load_thread_handle, oss3.str().c_str(), SC_CURRENT_USER_MODULE, xlmi_load_thread);
        m_process_handles.push_back(sc_get_current_process_handle());
        sensitive << m_load_event_queue[port];
      }
    }
  }
  if ((m_interface_type == XLMI0) && m_has_busy) {
    SC_METHOD(xlmi_load_method);
    for (u32 port=0; port<m_num_ports; ++port) {
      sensitive << *m_p_preload[port] << *m_p_busy[port];
    }
  }
  // Restore SystemC
  sc_report_handler::set_actions("object already exists", SC_WARNING, original_action);

  for (u32 i=0; i<m_num_ports; ++i) {
    ostringstream oss;
    oss.str(""); oss << "m_request_exports" << "[" << i << "]"; m_port_types[oss.str()] = REQUEST_EXPORT;
    oss.str(""); oss << "m_respond_ports"   << "[" << i << "]"; m_port_types[oss.str()] = RESPOND_PORT;
    oss.str(""); oss << "m_debug_ports"     << "[" << i << "]"; m_port_types[oss.str()] = DEBUG_PORT;
    oss.str(""); oss << "debug_port"        << "[" << i << "]"; m_port_types[oss.str()] = PORT_TABLE;
    oss.str(""); oss << "slave_port"        << "[" << i << "]"; m_port_types[oss.str()] = PORT_TABLE;
    oss.str(""); oss << m_interface_lc      << "[" << i << "]"; m_port_types[oss.str()] = PORT_TABLE;

    set<string> p;
    p = get_bool_output_set(i); for (set<string>::const_iterator j = p.begin(); j != p.end(); ++j) { m_port_types[*j] = BOOL_OUTPUT; }
    p = get_uint_output_set(i); for (set<string>::const_iterator j = p.begin(); j != p.end(); ++j) { m_port_types[*j] = UINT_OUTPUT; }
    p = get_wide_output_set(i); for (set<string>::const_iterator j = p.begin(); j != p.end(); ++j) { m_port_types[*j] = WIDE_OUTPUT; }
    p = get_bool_input_set (i); for (set<string>::const_iterator j = p.begin(); j != p.end(); ++j) { m_port_types[*j] = BOOL_INPUT;  }
    p = get_uint_input_set (i); for (set<string>::const_iterator j = p.begin(); j != p.end(); ++j) { m_port_types[*j] = UINT_INPUT;  }
    p = get_wide_input_set (i); for (set<string>::const_iterator j = p.begin(); j != p.end(); ++j) { m_port_types[*j] = WIDE_INPUT;  }

  }

  m_port_types["debug_ports"]  = PORT_TABLE;
  m_port_types["slave_ports"]  = PORT_TABLE;
  m_port_types[m_interface_lc] = PORT_TABLE;

  if (m_num_ports == 1) {
    m_port_types["debug_port"] = PORT_TABLE;
    m_port_types["slave_port"] = PORT_TABLE;
  }

  if (m_deny_fast_access.size()) {
    if (m_deny_fast_access.size() & 0x1) {
      ostringstream oss;
      oss << kind() << " '" << name() << "': " << "\"deny_fast_access\" parameter has " << m_deny_fast_access.size()
          << " elements which is not an even number as it must be.";
      throw xtsc_exception(oss.str());
    }
    xtsc_address end_address = 0xFFFFFFFF;
    if ((m_interface_type != XLMI0) && !is_pif_or_idma(m_interface_type)) {
      if (m_address_bits < 32) {
        end_address = m_start_byte_address + (1 << (m_address_bits + m_address_shift)) - 1;
      }
    }
    for (u32 i=0; i<m_deny_fast_access.size(); i+=2) {
      xtsc_address begin = m_deny_fast_access[i];
      xtsc_address end   = m_deny_fast_access[i+1];
      if ((begin < m_start_byte_address) || (begin > end_address) ||
          (end   < m_start_byte_address) || (end   > end_address) ||
          (end   < begin)) {
        ostringstream oss;
        oss << kind() << " '" << name() << "': " << "\"deny_fast_access\" range [0x" << hex << setfill('0') << setw(8) << begin
            << "-0x" << setw(8) << end << "] is invalid." << endl;
        oss << "Valid ranges must fit within [0x" << setw(8) << m_start_byte_address << "-0x" << end_address
            << "].";
        throw xtsc_exception(oss.str());
      }
    }
  }

  xtsc_register_command(*this, *this, "dump_pending_rsp_info", 0, 0,
      "dump_pending_rsp_info", 
      "Return the buffer from calling dump_pending_rsp_info()."
  );

  xtsc_register_command(*this, *this, "get_pending_rsp_info_cnt", 0, 0,
      "get_pending_rsp_info_cnt", 
      "Return get_pending_rsp_info_cnt()."
  );

  // Log our construction
  log4xtensa::LogLevel ll = xtsc_get_constructor_log_level();
  XTSC_LOG(m_text, ll, "Constructed " << kind() << " '" << name() << "':");
  XTSC_LOG(m_text, ll, " memory_interface          = "                 << m_interface_uc);
  if (m_interface_type == PIF) {
  XTSC_LOG(m_text, ll, " inbound_pif               = "   << boolalpha  << m_inbound_pif);
  XTSC_LOG(m_text, ll, " snoop                     = "   << boolalpha  << m_snoop);
  if (!m_inbound_pif && !m_snoop) {
  XTSC_LOG(m_text, ll, " has_coherence             = "   << boolalpha  << m_has_coherence);
  }
  XTSC_LOG(m_text, ll, " req_user_data             = "                 << m_req_user_data);
  XTSC_LOG(m_text, ll, " rsp_user_data             = "                 << m_rsp_user_data);
  }
  if (is_pif_or_idma(m_interface_type) && !m_snoop) {
  XTSC_LOG(m_text, ll, " has_pif_attribute         = "   << boolalpha  << m_has_pif_attribute);
  if (!m_inbound_pif) {
  XTSC_LOG(m_text, ll, " has_pif_req_domain        = "   << boolalpha  << m_has_pif_req_domain);
  }
  }
  XTSC_LOG(m_text, ll, " num_ports                 = "   << dec        << m_num_ports);
  XTSC_LOG(m_text, ll, " banked                    = "   << boolalpha  << m_banked);
  XTSC_LOG(m_text, ll, " num_subbanks              = "   << dec        << m_num_subbanks);
  XTSC_LOG(m_text, ll, " port_name_suffix          = "                 << m_suffix);
  XTSC_LOG(m_text, ll, " byte_width                = "                 << m_width8);
  if (is_amba(m_interface_type)) {
  XTSC_LOG(m_text, ll, " axi_name_prefix           = "                 << axi_name_prefix);
  }
  if (is_axi_or_idma(m_interface_type)) {
  log_vu32(m_text, ll, " axi_id_bits",                                    m_axi_id_bits,       27);
  log_vu32(m_text, ll, " axi_route_id_bits",                              m_axi_route_id_bits, 27);
  log_vu32(m_text, ll, " axi_read_write",                                 m_axi_read_write,    27);
  log_vu32(m_text, ll, " axi_port_type",                                  m_axi_port_type,     27);
  }
  XTSC_LOG(m_text, ll, " start_byte_address        = 0x" << hex        << setfill('0') << setw(8) << m_start_byte_address);
  XTSC_LOG(m_text, ll, " big_endian                = "   << boolalpha  << m_big_endian);
  XTSC_LOG(m_text, ll, " vcd_handle                = "                 << m_p_trace_file);
  XTSC_LOG(m_text, ll, " request_fifo_depth        = "                 << m_request_fifo_depth);
  if (clock_period == 0xFFFFFFFF) {
  XTSC_LOG(m_text, ll, " clock_period              = 0xFFFFFFFF => "   << m_clock_period.value() << " (" << m_clock_period << ")");
  } else {
  XTSC_LOG(m_text, ll, " clock_period              = "                 << clock_period << " (" << m_clock_period << ")");
  }
  if (posedge_offset == 0xFFFFFFFF) {
  XTSC_LOG(m_text, ll, " posedge_offset            = 0xFFFFFFFF => "   << m_posedge_offset.value() << " (" << m_posedge_offset << ")");
  } else {
  XTSC_LOG(m_text, ll, " posedge_offset            = "                 << posedge_offset << " (" << m_posedge_offset << ")");
  }
  if (sample_phase == 0xFFFFFFFF) {
  XTSC_LOG(m_text, ll, " sample_phase              = 0x" << hex        << sample_phase << " (" << m_sample_phase << ")");
  } else {
  XTSC_LOG(m_text, ll, " sample_phase              = "                 << sample_phase << " (" << m_sample_phase << ")");
  }
  XTSC_LOG(m_text, ll, " read_delay                = "                 << m_read_delay);
  if (is_pif_or_idma(m_interface_type)) {
  XTSC_LOG(m_text, ll, " has_request_id            = "   << boolalpha  << m_has_request_id);
  XTSC_LOG(m_text, ll, " write_responses           = "   << boolalpha  << m_write_responses);
  if (!m_write_responses) {
  XTSC_LOG(m_text, ll, " track_write_requests      = "   << boolalpha  << m_track_write_requests);
  }
  XTSC_LOG(m_text, ll, " discard_unknown_responses = "   << boolalpha  << m_discard_unknown_responses);
  XTSC_LOG(m_text, ll, " route_id_bits             = "   << dec        << m_route_id_bits);
  } else {
  XTSC_LOG(m_text, ll, " address_bits              = "   << dec        << m_address_bits);
  XTSC_LOG(m_text, ll, " has_busy                  = "   << boolalpha  << m_has_busy);
  }
  XTSC_LOG(m_text, ll, " output_delay              = "                 << m_output_delay);
  if ((m_interface_type == DRAM0) || (m_interface_type == DRAM0BS) || (m_interface_type == DRAM0RW) ||
      (m_interface_type == DRAM1) || (m_interface_type == DRAM1BS) || (m_interface_type == DRAM1RW))
  {
  XTSC_LOG(m_text, ll, " has_lock                  = "   << boolalpha  << m_has_lock);
  }
  if ((m_interface_type == DRAM0) || (m_interface_type == DRAM0BS) || (m_interface_type == DRAM0RW) ||
      (m_interface_type == DRAM1) || (m_interface_type == DRAM1BS) || (m_interface_type == DRAM1RW) ||
      (m_interface_type == IRAM0) || (m_interface_type == IRAM1  ))
  {
  XTSC_LOG(m_text, ll, " check_bits                = "                 << m_check_bits);
  }
  if ((m_interface_type != DROM0) && (m_interface_type != XLMI0) && !is_pif_or_idma(m_interface_type)) {
  XTSC_LOG(m_text, ll, " has_xfer_en               = "   << boolalpha  << m_has_xfer_en);
  }
  XTSC_LOG(m_text, ll, " cbox                      = "   << boolalpha  << m_cbox);
  if (m_split_rw) {
  XTSC_LOG(m_text, ll, " has_dma                   = "   << boolalpha  << m_has_dma);
  XTSC_LOG(m_text, ll, " dram_attribute_width      = "                 << m_dram_attribute_width);
  }
  XTSC_LOG(m_text, ll, " external_udma             = "   << boolalpha  << m_external_udma);
  if (m_external_udma) {
  XTSC_LOG(m_text, ll, " ram_select_bit            = "   << dec        << m_ram_select_bit);
  XTSC_LOG(m_text, ll, " ram_select_normal          = "  << boolalpha  << m_ram_select_normal);
  }
  XTSC_LOG(m_text, ll, " deny_fast_access         :");
  for (u32 i=0; i<m_deny_fast_access.size(); i+=2) {
  XTSC_LOG(m_text, ll, hex << "  [0x" << setfill('0') << setw(8) << m_deny_fast_access[i] << "-0x"
                                                      << setw(8) << m_deny_fast_access[i+1] << "]");
  }
  XTSC_LOG(m_text, ll, " dso_name                  = "                 << m_dso_name);
  XTSC_LOG(m_text, ll, " dso_cookie                = "                 << (m_dso_cookie ? m_dso_cookie : ""));
  XTSC_LOG(m_text, ll, " cosim                     = "   << boolalpha  << m_cosim);
  XTSC_LOG(m_text, ll, " shadow_memory             = "   << boolalpha  << m_shadow_memory);
  XTSC_LOG(m_text, ll, " test_shadow_memory        = "   << boolalpha  << m_test_shadow_memory);
  if (m_test_shadow_memory) {
    XTSC_WARN(m_text, "The \"test_shadow_memory\" flag is set -- Operating in shadow memory test mode.");
  }
  XTSC_LOG(m_text, ll, " initial_value_file        = "                 << m_initial_value_file);
  XTSC_LOG(m_text, ll, " memory_fill_byte          = 0x" << hex        << (u32) m_memory_fill_byte);
  XTSC_LOG(m_text, ll, " m_address_mask            = 0x" << hex        << m_address_mask);
  XTSC_LOG(m_text, ll, " m_address_shift           = "   << dec        << m_address_shift);
  if (m_external_udma) {
  XTSC_LOG(m_text, ll, " m_ram_select_mask         = 0x" << hex        << setfill('0') << setw(8) << m_ram_select_mask);
  }
  ostringstream oss;
  oss << "Port List:" << endl;
  dump_ports(oss);
  xtsc_log_multiline(m_text, ll, oss.str(), 2);

}



xtsc_component::xtsc_tlm2pin_memory_transactor::~xtsc_tlm2pin_memory_transactor(void) {
  // Do any required clean-up here
  XTSC_DEBUG(m_text, "m_request_pool.size()=" << m_request_pool.size());
  XTSC_DEBUG(m_text, "m_response_pool.size()=" << m_response_pool.size());
  if (m_p_memory) {
    delete m_p_memory;
    m_p_memory = 0;
  }
}



u32 xtsc_component::xtsc_tlm2pin_memory_transactor::get_bit_width(const string& port_name, u32 interface_num) const {
  string name_portion;
  u32    index;
  xtsc_parse_port_name(port_name, name_portion, index);

  if ((name_portion == "m_request_exports") || (name_portion == "m_respond_ports") || (name_portion == "m_debug_ports")) {
    return m_width8 * 8;
  }

  return xtsc_module_pin_base::get_bit_width(port_name);
}



sc_object *xtsc_component::xtsc_tlm2pin_memory_transactor::get_port(const string& port_name) {
  if (has_bool_output(port_name)) return &get_bool_output(port_name);
  if (has_uint_output(port_name)) return &get_uint_output(port_name);
  if (has_wide_output(port_name)) return &get_wide_output(port_name);
  if (has_bool_input (port_name)) return &get_bool_input (port_name);
  if (has_uint_input (port_name)) return &get_uint_input (port_name);
  if (has_wide_input (port_name)) return &get_wide_input (port_name);

  string name_portion;
  u32    index;
  xtsc_parse_port_name(port_name, name_portion, index);

  if ((name_portion != "m_request_exports") && (name_portion != "m_respond_ports") && (name_portion != "m_debug_ports")) {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\" has no port named \"" << port_name << "\"" << endl;
    throw xtsc_exception(oss.str());
  }

  if (index > m_num_ports - 1) {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\": Invalid index: \"" << port_name << "\".  Valid range: 0-" << (m_num_ports-1)
        << endl;
    throw xtsc_exception(oss.str());
  }

       if (name_portion == "m_request_exports") return (sc_object*) m_request_exports[index];
  else if (name_portion == "m_respond_ports")   return (sc_object*) m_respond_ports  [index];
  else                                          return (sc_object*) m_debug_ports    [index];
}



xtsc_port_table xtsc_component::xtsc_tlm2pin_memory_transactor::get_port_table(const string& port_table_name) const {
  xtsc_port_table table;

  if (port_table_name == "debug_ports") {
    for (u32 i=0; i<m_num_ports; ++i) {
      ostringstream oss;
      oss << "debug_port[" << i << "]";
      table.push_back(oss.str());
    }
    return table;
  }
  
  if (port_table_name == "slave_ports") {
    for (u32 i=0; i<m_num_ports; ++i) {
      ostringstream oss;
      oss << "slave_port[" << i << "]";
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
      ((name_portion != "debug_port") && (name_portion != "slave_port") && (name_portion != m_interface_lc)))
  {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\" has no port table named \"" << port_table_name << "\"" << endl;
    throw xtsc_exception(oss.str());
  }

  if (index > m_num_ports - 1) {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\": Invalid index: \"" << port_table_name << "\".  Valid range: 0-"
        << (m_num_ports-1) << endl;
    throw xtsc_exception(oss.str());
  }

  ostringstream oss;

  if (name_portion == "slave_port") {
    oss.str(""); oss << "m_request_exports" << "[" << index << "]"; table.push_back(oss.str());
    oss.str(""); oss << "m_respond_ports"   << "[" << index << "]"; table.push_back(oss.str());
  }
  else if (name_portion == "debug_port") {
    oss.str(""); oss << "m_debug_ports"     << "[" << index << "]"; table.push_back(oss.str());
  }
  else {
    set<string> p;
    p = get_bool_output_set(index); for (set<string>::const_iterator j = p.begin(); j != p.end(); ++j) { table.push_back(*j); }
    p = get_uint_output_set(index); for (set<string>::const_iterator j = p.begin(); j != p.end(); ++j) { table.push_back(*j); }
    p = get_wide_output_set(index); for (set<string>::const_iterator j = p.begin(); j != p.end(); ++j) { table.push_back(*j); }
    p = get_bool_input_set (index); for (set<string>::const_iterator j = p.begin(); j != p.end(); ++j) { table.push_back(*j); }
    p = get_uint_input_set (index); for (set<string>::const_iterator j = p.begin(); j != p.end(); ++j) { table.push_back(*j); }
    p = get_wide_input_set (index); for (set<string>::const_iterator j = p.begin(); j != p.end(); ++j) { table.push_back(*j); }
  }

  return table;
}



bool xtsc_component::xtsc_tlm2pin_memory_transactor::has_dso() const {
  return (m_peek != NULL);
}



void xtsc_component::xtsc_tlm2pin_memory_transactor::dump_pending_rsp_info(ostream& os) {
  bool axi = is_axi_or_idma(m_interface_type);
  for (u32 port=0; port<m_num_ports; ++port) {
    for (rsp_info_deque_map::iterator i = m_tran_id_rsp_info[port].begin(); i != m_tran_id_rsp_info[port].end(); ++i) {
      rsp_info_deque& rid = *(i->second);
      for (rsp_info_deque::iterator j = rid.begin(); j != rid.end(); ++j) {
        response_info *p_response_info = *j;
        os << port;
        if (!axi) {
          os << " 0x" << hex << p_response_info->m_route_id;
        }
        os << " " << dec << p_response_info->m_id << " " << (p_response_info->m_is_read ? "rd" : "wr")<< " ";
        if (p_response_info->m_p_response) {
          os << *p_response_info->m_p_response;
        }
        os << endl;
      }
    }
  }
}



xtsc::u64 xtsc_component::xtsc_tlm2pin_memory_transactor::get_pending_rsp_info_cnt(void) {
  return m_pending_rsp_info_cnt;
}



void xtsc_component::xtsc_tlm2pin_memory_transactor::execute(const string&                 cmd_line, 
                                                             const vector<string>&         words,
                                                             const vector<string>&         words_lc,
                                                             ostream&                      result)
{
  ostringstream res;

  if (false) {
  }
  else if (words[0] == "dump_pending_rsp_info") {
    dump_pending_rsp_info(res);
  }
  else if (words[0] == "get_pending_rsp_info_cnt") {
    res << get_pending_rsp_info_cnt();
  }
  else {
    ostringstream oss;
    oss << name() << "::" << __FUNCTION__ << "() called for unknown command '" << cmd_line << "'.";
    throw xtsc_exception(oss.str());
  }

  result << res.str();
}



void xtsc_component::xtsc_tlm2pin_memory_transactor::connect(xtsc_arbiter& arbiter, u32 port_num) {
  if (port_num >= m_num_ports) {
    ostringstream oss;
    oss << "Invalid port_num=" << port_num << " in connect(): " << endl;
    oss << kind() << " '" << name() << "' has " << m_num_ports << " ports numbered from 0 to " << m_num_ports-1 << endl;
    throw xtsc_exception(oss.str());
  }
  arbiter.m_request_port(*m_request_exports[port_num]);
  (*m_respond_ports[port_num])(arbiter.m_respond_export);
}



xtsc::u32 xtsc_component::xtsc_tlm2pin_memory_transactor::connect(xtsc_core&    core,
                                                                  const char   *memory_port_name,
                                                                  u32           port_num,
                                                                  bool          single_connect)
{
  if (port_num >= m_num_ports) {
    ostringstream oss;
    oss << "Invalid port_num=" << port_num << " in connect(): " << endl;
    oss << kind() << " '" << name() << "' has " << m_num_ports << " ports numbered from 0 to " << m_num_ports-1 << endl;
    throw xtsc_exception(oss.str());
  }
  xtsc_core::memory_port mem_port = xtsc_core::get_memory_port(memory_port_name, &core);
  bool consistent = false;
  if (xtsc_core::is_subbanked_dram0(mem_port)) {
    consistent = (m_interface_type == DRAM0BS);
  }
  else if (xtsc_core::is_subbanked_dram1(mem_port)) {
    consistent = (m_interface_type == DRAM1BS);
  }
  else switch (mem_port) {
    case xtsc_core::MEM_DRAM0LS0:
    case xtsc_core::MEM_DRAM0LS1:   consistent = (m_interface_type == DRAM0); break;

    case xtsc_core::MEM_DRAM0LS0RD:
    case xtsc_core::MEM_DRAM0LS1RD:
    case xtsc_core::MEM_DRAM0LS2RD:
    case xtsc_core::MEM_DRAM0DMARD:
    case xtsc_core::MEM_DRAM0LS0WR:
    case xtsc_core::MEM_DRAM0LS1WR:
    case xtsc_core::MEM_DRAM0LS2WR:
    case xtsc_core::MEM_DRAM0DMAWR: consistent = (m_interface_type == DRAM0RW); break;

    case xtsc_core::MEM_DRAM1LS0:
    case xtsc_core::MEM_DRAM1LS1:   consistent = (m_interface_type == DRAM1); break;

    case xtsc_core::MEM_DRAM1LS0RD:
    case xtsc_core::MEM_DRAM1LS1RD:
    case xtsc_core::MEM_DRAM1LS2RD:
    case xtsc_core::MEM_DRAM1DMARD:
    case xtsc_core::MEM_DRAM1LS0WR:
    case xtsc_core::MEM_DRAM1LS1WR:
    case xtsc_core::MEM_DRAM1LS2WR:
    case xtsc_core::MEM_DRAM1DMAWR: consistent = (m_interface_type == DRAM1RW); break;

    case xtsc_core::MEM_DROM0LS0:
    case xtsc_core::MEM_DROM0LS1:   consistent = (m_interface_type == DROM0); break;

    case xtsc_core::MEM_IRAM0:      consistent = (m_interface_type == IRAM0); break;
    case xtsc_core::MEM_IRAM1:      consistent = (m_interface_type == IRAM1); break;

    case xtsc_core::MEM_IROM0:      consistent = (m_interface_type == IROM0); break;

    case xtsc_core::MEM_URAM0:      consistent = (m_interface_type == URAM0); break;

    case xtsc_core::MEM_XLMI0LS0:
    case xtsc_core::MEM_XLMI0LS1:   consistent = (m_interface_type == XLMI0); break;

    case xtsc_core::MEM_PIF:        consistent = (m_interface_type == PIF); break;

    case xtsc_core::MEM_IDMA0:      consistent = (m_interface_type == IDMA0); break;

    default: {
      ostringstream oss;
      oss << "Program bug (unsupported xtsc_core::mem_port) in line " << __LINE__ << " of " << __FILE__;
      throw xtsc_exception(oss.str());
    }
  }
  if (!consistent) {
    ostringstream oss;
    oss << "Cannot connect " << kind() << " '" << name() << "' with interface type of \"" << m_interface_uc
        << "\" to memory port \"" << memory_port_name << "\" of xtsc_core '" << core.name() << "'";
    throw xtsc_exception(oss.str());
  }
  u32 width8 = core.get_memory_byte_width(mem_port);
  if (m_width8 && (width8 != m_width8)) {
    ostringstream oss;
    oss << "Memory interface data width mismatch: " << endl;
    oss << kind() << " '" << name() << "' is " << m_width8 << " bytes wide, but '" << memory_port_name << "' interface of" << endl;
    oss << "xtsc_core '" << core.name() << "' is " << width8 << " bytes wide.";
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



void xtsc_component::xtsc_tlm2pin_memory_transactor::connect(xtsc_master& master, u32 port_num) {
  if (port_num >= m_num_ports) {
    ostringstream oss;
    oss << "Invalid port_num=" << port_num << " in connect(): " << endl;
    oss << kind() << " '" << name() << "' has " << m_num_ports << " ports numbered from 0 to " << m_num_ports-1 << endl;
    throw xtsc_exception(oss.str());
  }
  master.m_request_port(*m_request_exports[port_num]);
  (*m_respond_ports[port_num])(master.m_respond_export);
}



u32 xtsc_component::xtsc_tlm2pin_memory_transactor::connect(xtsc_memory_trace&  memory_trace,
                                                            u32                 trace_port,
                                                            u32                 port_num,
                                                            bool                single_connect)
{
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



void xtsc_component::xtsc_tlm2pin_memory_transactor::connect(xtsc_router& router, u32 router_port, u32 port_num) {
  if (port_num >= m_num_ports) {
    ostringstream oss;
    oss << "Invalid port_num=" << port_num << " in connect(): " << endl;
    oss << kind() << " '" << name() << "' has " << m_num_ports << " ports numbered from 0 to " << m_num_ports-1 << endl;
    throw xtsc_exception(oss.str());
  }
  u32 num_slaves = router.get_num_slaves();
  if (router_port >= num_slaves) {
    ostringstream oss;
    oss << "Invalid router_port=" << router_port << " in xtsc_tlm2pin_memory_transactor::connect(): " << endl;
    oss << router.kind() << " '" << router.name() << "' has " << num_slaves << " ports numbered from 0 to " << num_slaves-1 << endl;
    throw xtsc_exception(oss.str());
  }
  (*router.m_request_ports[router_port])(*m_request_exports[port_num]);
  (*m_respond_ports[port_num])(*router.m_respond_exports[router_port]);
}



void xtsc_component::xtsc_tlm2pin_memory_transactor::before_end_of_elaboration(void) {
  log4xtensa::LogLevel ll = xtsc_get_constructor_log_level();

  if (m_dso_name != "") {
#if defined(_WIN32)
    m_dso = LoadLibrary(m_dso_name.c_str());
    if (!m_dso) {
      ostringstream oss;
      oss << kind() << " '" << name() << "': LoadLibrary() call failed for \"dso_name\" = " << m_dso_name << ": " << win_error();
      throw xtsc_exception(oss.str());
    }

    m_peek = (peek_t) GetProcAddress(m_dso, "peek");
    if (!m_peek) {
      ostringstream oss;
      oss << kind() << " '" << name() << "': GetProcAddress() call failed for symbol \"peek\" in DSO " << m_dso_name << ": "
          << win_error();
      throw xtsc_exception(oss.str());
    }

    m_poke = (poke_t) GetProcAddress(m_dso, "poke");
    if (!m_poke) {
      ostringstream oss;
      oss << kind() << " '" << name() << "': GetProcAddress() call failed for symbol \"poke\" in DSO " << m_dso_name << ": "
          << win_error();
      throw xtsc_exception(oss.str());
    }
#else
    m_dso = dlopen(m_dso_name.c_str(), RTLD_LAZY);
    if (!m_dso) {
      ostringstream oss;
      oss << kind() << " '" << name() << "': dlopen() call failed for \"dso_name\" = " << m_dso_name << ": " << dlerror();
      throw xtsc_exception(oss.str());
    }

    m_peek = (peek_t) dlsym(m_dso, "peek");
    if (!m_peek) {
      ostringstream oss;
      oss << kind() << " '" << name() << "': dlsym() call failed for symbol \"peek\" in DSO " << m_dso_name << ": " << dlerror();
      throw xtsc_exception(oss.str());
    }

    m_poke = (poke_t) dlsym(m_dso, "poke");
    if (!m_poke) {
      ostringstream oss;
      oss << kind() << " '" << name() << "': dlsym() call failed for symbol \"poke\" in DSO " << m_dso_name << ": " << dlerror();
      throw xtsc_exception(oss.str());
    }
#endif
  }
  bool found_one_unconnected = false;
  for (u32 port=0; port<m_num_ports; ++port) {
    if (m_debug_ports[port]->get_interface() == NULL) {
      ostringstream oss;
      oss << ( (m_dso_name != "") ? "m_debug_dso" : "m_debug_cap") << "[" << port << "]";
      m_debug_cap[port] = new xtsc_debug_if_cap(*this, port);
      xtsc_trap_port_binding_failures(true);
      try {
        (*m_debug_ports[port])(*m_debug_cap[port]);
        XTSC_LOG(m_text, ll, "Connected port \"" << m_debug_ports[port]->name() << "\") to " << oss.str());
        found_one_unconnected = true;
      }
      catch (...) {
        XTSC_LOG(m_text, ll, "Detected delay-bound port " << m_debug_ports[port]->name());
      }
      xtsc_trap_port_binding_failures(false);
    }
  }
  if (m_test_shadow_memory || (found_one_unconnected && m_cosim && m_shadow_memory)) {
    XTSC_LOG(m_text, ll, "Creating shadow memory");
    m_p_memory = new xtsc_memory_b(name(), kind(), m_width8, m_start_byte_address, 0, 1024*16, m_initial_value_file,
                                   m_memory_fill_byte);
  }

}



void xtsc_component::xtsc_tlm2pin_memory_transactor::end_of_elaboration(void) {
} 



void xtsc_component::xtsc_tlm2pin_memory_transactor::start_of_simulation(void) {
  reset(true);
} 



void xtsc_component::xtsc_tlm2pin_memory_transactor::end_of_simulation(void) {
  if (m_pending_rsp_info_cnt) {
    XTSC_WARN(m_text, "m_pending_rsp_info_cnt = " << m_pending_rsp_info_cnt << " which indicates there are still pending pin-level responses");
  }
  else {
    XTSC_VERBOSE(m_text, "m_pending_rsp_info_cnt = " << m_pending_rsp_info_cnt);
  }
} 



void xtsc_component::xtsc_tlm2pin_memory_transactor::reset(bool hard_reset) {
  XTSC_INFO(m_text, kind() << "::reset()");

  m_next_port_pif_request_thread                        = 0;
  m_next_port_pif_response_thread                       = 0;
  m_next_port_pif_drive_resp_rdy_thread                 = 0;
  m_next_port_axi_request_thread                        = 0;
  m_next_port_axi_response_thread                       = 0;
  m_next_port_axi_drive_resp_rdy_thread                 = 0;
  m_next_port_apb_request_thread                        = 0;
  m_next_port_lcl_request_thread                        = 0;
  m_next_port_xlmi_retire_thread                        = 0;
  m_next_port_xlmi_flush_thread                         = 0;
  m_next_port_xlmi_load_thread                          = 0;
  m_next_port_dram_lock_thread                          = 0;
  m_next_port_lcl_busy_write_rsp_thread                 = 0;
  m_next_port_lcl_7stage_write_rsp_thread               = 0;
  m_next_port_lcl_sample_read_data_thread               = 0;

  for (u32 port=0; port<m_num_ports; ++port) {

    m_hard_busy[port] = false;

    for (deque<xtsc_request*>::iterator ir = m_request_fifo[port].begin(); ir != m_request_fifo[port].end(); ++ir) {
      xtsc_request *p_request = *ir;
      delete_request(p_request);
    }
    m_request_fifo[port].clear();

    if (is_pif_or_idma(m_interface_type) || is_axi_or_idma(m_interface_type)) {
      deque<response_info*>::iterator ir;

      if (is_pif_or_idma(m_interface_type)) {
        for (ir = m_write_response_deque[port].begin(); ir != m_write_response_deque[port].end(); ++ir) {
          response_info *p_response_info = *ir;
          delete_response_info(p_response_info);
        }
        m_write_response_deque[port].clear();
        m_previous_response_last[port] = true;
      }

      for (ir = m_system_response_deque[port].begin(); ir != m_system_response_deque[port].end(); ++ir) {
        response_info *p_response_info = *ir;
        delete_response_info(p_response_info);
      }
      m_system_response_deque[port].clear();

      m_transfer_number[port] = 0;

      for (rsp_info_deque_map::iterator i = m_tran_id_rsp_info[port].begin(); i != m_tran_id_rsp_info[port].end(); ++i) {
        rsp_info_deque& rid = *(i->second);
        for (rsp_info_deque::iterator j = rid.begin(); j != rid.end(); ++j) {
          response_info *p_response_info = *j;
          delete_response_info(p_response_info);
          m_pending_rsp_info_cnt -= 1;
        }
        rid.clear();
      }

    }

  }

  if (hard_reset && m_p_memory) {
    m_p_memory->load_initial_values();
  }

  // Cancel any pending sc_event and sc_event_queue notifications:
  for (u32 port=0; port<m_num_ports; ++port) {
    if (m_request_event)                m_request_event               [port].cancel();
    if (m_write_response_event)         m_write_response_event        [port].cancel();
    if (m_drive_resp_rdy_event)         m_drive_resp_rdy_event        [port].cancel();
    if (m_retire_event)                 m_retire_event                [port].cancel();
    if (m_flush_event)                  m_flush_event                 [port].cancel();
    if (m_busy_write_rsp_event_queue)   m_busy_write_rsp_event_queue  [port].cancel_all();
    if (m_7stage_write_rsp_event_queue) m_7stage_write_rsp_event_queue[port].cancel_all();
    if (m_read_data_event_queue)        m_read_data_event_queue       [port].cancel_all();
    if (m_lock_event_queue)             m_lock_event_queue            [port].cancel_all();
    if (m_load_event_queue)             m_load_event_queue            [port].cancel_all();
  }

  // Clear some more deque's
  for (u32 port=0; port<m_num_ports; ++port) {
    if (is_system_memory(m_interface_type)) {
      ; // Do Nothing
    }
    else {
      while (!m_busy_write_rsp_deque[port].empty()) {
        response_info *p_info  = m_busy_write_rsp_deque[port].front();
        m_busy_write_rsp_deque[port].pop_front();
        delete_response_info(p_info);
      }
      while (!m_7stage_write_rsp_deque[port].empty()) {
        response_info *p_info  = m_7stage_write_rsp_deque[port].front();
        m_7stage_write_rsp_deque[port].pop_front();
        delete_response_info(p_info);
      }
      while (!m_read_data_rsp_deque[port].empty()) {
        response_info *p_info  = m_read_data_rsp_deque[port].front();
        m_read_data_rsp_deque[port].pop_front();
        delete_response_info(p_info);
      }
    }
    if (m_interface_type == XLMI0) {
      m_load_deque[port].clear();
    }
    if (((m_interface_type == DRAM0  ) ||
         (m_interface_type == DRAM0RW) ||
         (m_interface_type == DRAM1  ) ||
         (m_interface_type == DRAM1RW)) && m_has_lock)
    {
      m_lock_deque[port].clear();
    }
  }

  xtsc_reset_processes(m_process_handles);
}




void xtsc_component::xtsc_tlm2pin_memory_transactor::peek(u32 port_num, xtsc_address address8, u32 size8, u8 *buffer) {
  if (m_test_shadow_memory) {
    m_p_memory->peek(address8, size8, buffer);
  }
  else {
    (*m_debug_ports[port_num])->nb_peek(address8, size8, buffer);
  }
}



void xtsc_component::xtsc_tlm2pin_memory_transactor::poke(u32 port_num, xtsc_address address8, u32 size8, const u8 *buffer) {
  if (m_test_shadow_memory) {
    m_p_memory->poke(address8, size8, buffer);
  }
  else {
    (*m_debug_ports[port_num])->nb_poke(address8, size8, buffer);
  }
}



bool xtsc_component::xtsc_tlm2pin_memory_transactor::fast_access(u32 port_num, xtsc_fast_access_request &request) {
  return (*m_debug_ports[port_num])->nb_fast_access(request);
}



void xtsc_component::xtsc_tlm2pin_memory_transactor::pif_request_thread(void) {

  // Get the port number for this "instance" of the thread
  u32 port = m_next_port_pif_request_thread++;

  // A try/catch block in sc_main will not catch an exception thrown from
  // an SC_THREAD, so we'll catch them here, log them, then rethrow them.
  try {

    XTSC_INFO(m_text, "in pif_request_thread[" << port << "]");

    m_p_req_valid[port]->write(false);

    // Loop forever
    while (true) {

      // Wait for nb_request to tell us there's something to do 
      wait(m_request_event[port]);

      while (!m_request_fifo[port].empty()) {

        xtsc_request *p_request = m_request_fifo[port].front();
        m_request_fifo[port].pop_front();

        // Create response from request
        xtsc_response *p_response = new xtsc_response(*p_request, xtsc_response::RSP_OK);

        // Pick out some useful information about the request
        type_t            type            = p_request->get_type();
        xtsc_address      addr8           = p_request->get_byte_address();
        u32               size            = p_request->get_byte_size();
        xtsc_byte_enables byte_enables    = p_request->get_byte_enables();
        u32               bus_addr_bits   = (addr8 & m_bus_addr_bits_mask);

        // Must be a power of 2 and may not exceed bus width
        if (((size != 1) && (size != 2) && (size != 4) && (size != 8) && (size != 16) && (size != 32)) || (size > m_width8)) {
          ostringstream oss;
          oss << "xtsc_tlm2pin_memory_transactor::pif_request_thread():  Request with invalid (not power of 2) size=" << size
              << " or exceed bus width=" << m_width8 << ": " << *p_request;
          throw xtsc_exception(oss.str());
        }

        xtsc_byte_enables fixed_byte_enables = (byte_enables & (0xFFFFFFFFFFFFFFFFull >> (64 - size))) << bus_addr_bits;
        if (m_big_endian) {
          swizzle_byte_enables(fixed_byte_enables);
        }

        bool is_read  = ((type == xtsc_request::READ)        ||
                         (type == xtsc_request::RCW)         ||
                         (type == xtsc_request::BLOCK_READ)  ||
                         (type == xtsc_request::BURST_READ)  ||
                         (type == xtsc_request::SNOOP));
        bool is_write = ((type == xtsc_request::WRITE)       ||
                         (type == xtsc_request::RCW)         ||
                         (type == xtsc_request::BLOCK_WRITE) ||
                         (type == xtsc_request::BURST_WRITE));
        bool tracked  = is_read || m_track_write_requests;
        m_address       = p_request->get_hardware_address();
        m_vadrs         = 0;
        m_req_coh_cntl  = 0;
        m_lane          = fixed_byte_enables;
        m_id            = p_request->get_id() & m_pif_id_mask;
        m_priority      = p_request->get_priority();
        m_route_id      = p_request->get_route_id() & m_route_id_mask;
        m_req_attribute = p_request->get_pif_attribute();
        m_req_domain    = p_request->get_pif_req_domain();
        u32 tran_id     = (m_route_id << 8) + m_id;
        if (is_write && m_p_req_data[port]) {
          m_data = 0;
          const u8     *buffer          = p_request->get_buffer();
          u32           delta           = (m_big_endian ? -8 : +8);
          u32           bit_offset      = 8 * (m_big_endian ? (m_width8 - bus_addr_bits - 1) : bus_addr_bits);
          u64           bytes           = byte_enables;
          u32           max_offset      = (m_width8 - 1) * 8; // Prevent SystemC "E5 out of bounds" - slave should give an Aerr rsp
          for (u32 i = 0; (i<size) && (bit_offset <= max_offset); ++i) {
            if (bytes & 0x1) {
              m_data.range(bit_offset+7, bit_offset) = buffer[i];
              if (m_p_memory) {
                // Skip unaligned - they should get an Aerr
                if ((addr8 % size) == 0) {
                  m_p_memory->write_u8(addr8+i, buffer[i]);
                }
              }
            }
            bytes >>= 1;
            bit_offset += delta;
          }
          m_p_req_data[port]->write(m_data);
        }
        m_p_req_valid[port]->write(true);
        m_p_req_cntl[port]->write(m_req_cntl.init(*p_request));
        m_p_req_adrs[port]->write(m_address);
        m_p_req_priority[port]->write(m_priority);
        if (m_p_req_data_be  [port]) m_p_req_data_be  [port]->write(m_lane);
        if (m_p_req_id       [port]) m_p_req_id       [port]->write(m_id);
        if (m_p_req_route_id [port]) m_p_req_route_id [port]->write(m_route_id);
        if (m_p_req_coh_vadrs[port]) m_p_req_coh_vadrs[port]->write(m_vadrs);
        if (m_p_req_coh_cntl [port]) m_p_req_coh_cntl [port]->write(m_req_coh_cntl);
        if (m_p_req_attribute[port]) m_p_req_attribute[port]->write(m_req_attribute);
        if (m_p_req_domain   [port]) m_p_req_domain   [port]->write(m_req_domain);
        if (m_p_req_user_data[port]) {
          *m_user_data_val= (u64)p_request->get_user_data();
          m_p_req_user_data[port]->write(*m_user_data_val);
        }
        response_info *p_info = new_response_info(p_response, bus_addr_bits, size, is_read, m_id, m_route_id);
        rsp_info_deque *p_rid = NULL;
        if (tracked) {
          XTSC_DEBUG(m_text, "Adding to m_tran_id_rsp_info[" << port << "] tran_id=" << tran_id << ": " << *p_info->m_p_response);
          rsp_info_deque_map::iterator i = m_tran_id_rsp_info[port].find(tran_id);
          if (i == m_tran_id_rsp_info[port].end()) {
            m_tran_id_rsp_info[port][tran_id] = new rsp_info_deque;
            i = m_tran_id_rsp_info[port].find(tran_id);
          }
          p_rid = i->second;
          p_rid->push_back(p_info);
          m_pending_rsp_info_cnt += 1;
        }

        // Sync to sample phase
        sc_time now = sc_time_stamp();
        sc_time phase_now = (now.value() % m_clock_period_value) * m_time_resolution;
        if (m_has_posedge_offset) {
          if (phase_now < m_posedge_offset) {
            phase_now += m_clock_period;
          }
          phase_now -= m_posedge_offset;
        }
        sc_time time_to_sample_phase = (((phase_now < m_sample_phase) ?  m_sample_phase : m_sample_phase_plus_one) - phase_now);
        wait(time_to_sample_phase);
        // Handle according to whether or not the request was accepted
        if (m_p_req_rdy[port]->read()) {
          // Request was accepted
          // Are we responsible for write responses and is this a last-data write?
          if (m_write_responses && !is_read && p_request->get_last_transfer()) {
            p_info->m_status = (u32) xtsc_response::RSP_OK;
            p_info->m_last = true;
            m_write_response_deque[port].push_back(p_info);
            m_write_response_event[port].notify(SC_ZERO_TIME);
          }
          else if (!p_request->get_last_transfer()) {
            if (tracked) {
              p_rid->pop_back();
              m_pending_rsp_info_cnt -= 1;
              XTSC_DEBUG(m_text, "Removed   m_tran_id_rsp_info[" << port << "] tran_id=" << tran_id << ": because request was not last data");
            }
            delete_response_info(p_info);
          }
        }
        else {
          // Request was not accepted
          if (tracked) {
            p_rid->pop_back();
            m_pending_rsp_info_cnt -= 1;
            XTSC_DEBUG(m_text, "Removed   m_tran_id_rsp_info[" << port << "] tran_id=" << tran_id << ": because request was not accepted");
          }
          delete_response_info(p_info);
          p_response->set_status(xtsc_response::RSP_NACC);
          send_unchecked_response(p_response, port);
        }
        delete_request(p_request);
        // Continue driving the request for the balance of 1 clock period
        if (time_to_sample_phase != m_clock_period) {
          wait(m_clock_period - time_to_sample_phase);
        }
        // Deassert the request and drive all-zero values
        m_address       = 0;
        m_lane          = 0;
        m_id            = 0;
        m_priority      = 0;
        m_route_id      = 0;
        m_req_attribute = 0;
        m_req_domain    = 0;
        m_vadrs         = 0;
        m_req_coh_cntl  = 0;
        m_data          = 0;
        if (m_p_req_data[port]) m_p_req_data[port]->write(m_data);
        m_p_req_valid[port]->write(false);
        m_p_req_cntl[port]->write(m_req_cntl.init(0));
        m_p_req_adrs[port]->write(m_address);
        if (m_p_req_data_be[port]) m_p_req_data_be[port]->write(m_lane);
        if (m_p_req_id[port]) m_p_req_id[port]->write(m_id);
        m_p_req_priority[port]->write(m_priority);
        if (m_p_req_route_id [port]) m_p_req_route_id [port]->write(m_route_id);
        if (m_p_req_coh_vadrs[port]) m_p_req_coh_vadrs[port]->write(m_vadrs);
        if (m_p_req_coh_cntl [port]) m_p_req_coh_cntl [port]->write(m_req_coh_cntl);
        if (m_p_req_attribute[port]) m_p_req_attribute[port]->write(m_req_attribute);
        if (m_p_req_domain   [port]) m_p_req_domain   [port]->write(m_req_domain);
        if (m_p_req_user_data[port]) {
          *m_user_data_val= 0;
          m_p_req_user_data[port]->write(*m_user_data_val);
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



void xtsc_component::xtsc_tlm2pin_memory_transactor::pif_response_thread(void) {

  // Get the port number for this "instance" of the thread
  u32 port = m_next_port_pif_response_thread++;

  try {

    XTSC_INFO(m_text, "in pif_response_thread[" << port << "]");

    // Loop forever
    while (true) {

      do {
        // Wait for either PIRespValid or PORespRdy to go high or for locally-generated write response
        XTSC_DEBUG(m_text, "pif_response_thread[" << port << "]: calling wait()");
        wait(); // m_p_resp_valid[port]->pos()  m_p_resp_rdy[port]->pos()  m_write_response_event[port];
        XTSC_DEBUG(m_text, "pif_response_thread[" << port << "]: awoke from wait()");
        // TODO:  Do we need check for m_system_response_deque[port].empty()?  If so, how does it get non-empty in this loop?
      } while ((!m_p_resp_valid[port]->read() || !m_p_resp_rdy[port]->read()) &&
               m_system_response_deque[port].empty() &&
               m_write_response_deque[port].empty());

      // Sync to sample phase.  If already at the sample phase, wait 1 full clock period to ensure
      // PIRespValid and PORespRdy are high a non-zero amount of time.
      sc_time now = sc_time_stamp();
      sc_time phase_now = (now.value() % m_clock_period_value) * m_time_resolution;
      if (m_has_posedge_offset) {
        if (phase_now < m_posedge_offset) {
          phase_now += m_clock_period;
        }
        phase_now -= m_posedge_offset;
      }
      wait(((phase_now >= m_sample_phase) ? m_sample_phase_plus_one : m_sample_phase) - phase_now);
      XTSC_DEBUG(m_text, "pif_response_thread[" << port << "]: sync'd to sample phase");

      // Once each clock period:
      //  1) Capture one pin-level response from downstream slave if both PIRespValid and PORespRdy are asserted
      //  2) Attempt to send current TLM response to upstream master
      while ((m_p_resp_valid[port]->read() && m_p_resp_rdy[port]->read()) ||
             !m_system_response_deque[port].empty() ||
             !m_write_response_deque[port].empty())
      {

        // Capture and save pin-level response (if there is one this clock cycle)
        if (m_p_resp_valid[port]->read() && m_p_resp_rdy[port]->read()) {
          u32 id      = (m_p_resp_id      [port] ? m_p_resp_id      [port]->read().to_uint() : 0);
          u32 rte_id  = (m_p_resp_route_id[port] ? m_p_resp_route_id[port]->read().to_uint() : 0);
          u32 tran_id = (rte_id << 8) + id;
          u32 cntl    = m_p_resp_cntl[port]->read().to_uint();
          u32 coh     = 0;
          response_info *p_info  = NULL;
          rsp_info_deque_map::iterator i = m_tran_id_rsp_info[port].find(tran_id);
          if (i != m_tran_id_rsp_info[port].end()) {
            rsp_info_deque& rid = *(i->second);
            if (!rid.empty()) {
              p_info = rid.front();
            }
          }
          bool process_response = true;
          if (!p_info) {
            if (m_discard_unknown_responses) {
              XTSC_INFO(m_text, "Discarding unknown response PORespID=" << id << " PORouteID=" << rte_id << " PORespCntl=0x" << hex << cntl);
              process_response = false;
            }
            else {
              ostringstream oss;
              oss << kind() << " '" << name() << "' got response tran_id=" << tran_id << " with no corresponding request.";
              if (m_write_responses) {
                oss << endl
                    << "(A possible cause of this problem is having both this transactor AND the downstream slave "
                    << "configured to send write responses)";
              }
              else if (!m_track_write_requests) {
                oss << endl
                    << "(A possible cause of this problem is having \"track_write_requests\" false when the downstream slave " 
                    << "does send write responses)";
              }
              throw xtsc_exception(oss.str());
            }
          }
          if (process_response) {
            p_info->m_last = ((cntl & resp_cntl::m_last_mask) == 1);
            p_info->m_status = (m_snoop ? 0 : ((cntl & resp_cntl::m_status_mask) >> resp_cntl::m_status_shift));
            if (!p_info->m_last) {
              // This should only happen for a BLOCK_READ|BURST_READ|SNOOP
              p_info = new_response_info(*p_info);
              XTSC_DEBUG(m_text, "Created duplicated response_info " << p_info);
            }
            m_transfer_number[port] += 1;
            p_info->m_p_response->set_transfer_number(m_transfer_number[port]);
            if (p_info->m_last) {
              m_transfer_number[port] = 0;
            }
            if (m_p_resp_priority[port]) {
              p_info->m_p_response->set_priority((u8) m_p_resp_priority[port]->read().to_uint());
            }
            if (m_p_resp_route_id[port]) {
              p_info->m_p_response->set_route_id(m_p_resp_route_id[port]->read().to_uint());
            }
            if (m_p_resp_coh_cntl[port]) {
              coh = m_p_resp_coh_cntl[port]->read().to_uint();
            }
            if (m_p_resp_user_data[port]) {
#if defined(__x86_64__) || defined(_M_X64)
              p_info->m_p_response->set_user_data((void*)m_p_resp_user_data[port]->read().to_uint64());
#else
              p_info->m_p_response->set_user_data((void*)m_p_resp_user_data[port]->read().to_uint  ());
#endif
            }
            p_info->m_p_response->set_exclusive_ok((cntl & resp_cntl::m_exclusive_ok_mask) == resp_cntl::m_exclusive_ok_mask);
            bool has_data = (m_snoop ? (coh & 0x1) : ((p_info->m_status != xtsc_response::RSP_NACC) && p_info->m_is_read));
            if (has_data) {
              p_info->m_copy_data = true;
              m_data = m_p_resp_data[port]->read();
              u32 bus_addr_bits = p_info->m_bus_addr_bits;
              u32 size          = p_info->m_size;
              u8 *buffer        = p_info->m_buffer;
              u32 delta         = (m_big_endian ? -8 : +8);
              u32 bit_offset    = 8 * (m_big_endian ? (m_width8 - bus_addr_bits - 1) : bus_addr_bits);
              u32 max_offset    = (m_width8 - 1) * 8; // Prevent SystemC "E5 out of bounds" - slave should have given an Aerr response
              for (u32 i = 0; (i<size) && (bit_offset <= max_offset); ++i) {
                buffer[i] = (u8) m_data.range(bit_offset+7, bit_offset).to_uint();
                bit_offset += delta;
              }
            }
            if (m_snoop) {
              p_info->m_p_response->set_snoop_data(has_data);
            }
#if 0
            XTSC_NOTE(m_text, "coh=" << coh << " has_data=" << has_data << " m_data=0x" << hex << m_data);
#endif
            m_system_response_deque[port].push_back(p_info);
            XTSC_VERBOSE(m_text, "pif_response_thread[" << port << "]: Got response: tran_id=" << tran_id << " status=" << p_info->m_status <<
                                 " tag=" << p_info->m_p_response->get_tag() << " address=0x" << setfill('0') << hex <<
                                 setw(8) << p_info->m_p_response->get_byte_address());
          }
        }

        // If previous response was accepted and was a last-transfer then push all pending locally-generated
        // write responses to the front of the line (this is to avoid interrupting a BLOCK_READ|BURST_READ sequence).
        // We use a reverse iterator so this set will be in time order; however, it is possible for a write that
        // happens in the near future to get its response inserted in front of this set.
        if (m_previous_response_last[port] && !m_write_response_deque[port].empty()) {
          deque<response_info*>::reverse_iterator ir;
          for (ir = m_write_response_deque[port].rbegin(); ir != m_write_response_deque[port].rend(); ++ir) {
            XTSC_DEBUG(m_text, *(*ir)->m_p_response << ": Moving response_info " << (*ir) <<
                               " from m_write_response_deque to m_system_response_deque");
            m_system_response_deque[port].push_front(*ir);
          }
          m_write_response_deque[port].clear();
        }


        if (!m_system_response_deque[port].empty()) {

          // Now handle current TLM response
          response_info *p_info = m_system_response_deque[port].front();

          if (p_info->m_copy_data) {
            memcpy(p_info->m_p_response->get_buffer(), p_info->m_buffer, p_info->m_size);
            p_info->m_copy_data = false;
          }
          p_info->m_p_response->set_last_transfer(p_info->m_last);
          p_info->m_p_response->set_status((xtsc_response::status_t) p_info->m_status);

          // Log the response
          XTSC_INFO(m_text, *p_info->m_p_response << " Port #" << port);
          
          bool accepted = (*m_respond_ports[port])->nb_respond(*p_info->m_p_response);

          if (accepted) {
            m_system_response_deque[port].pop_front();
            m_previous_response_last[port] = p_info->m_last;
            // Delete it?
            if (p_info->m_last) {
              if (p_info->m_is_read || m_track_write_requests) {
                u32 tran_id = ((p_info->m_route_id & m_route_id_mask) << 8) + (p_info->m_id & m_pif_id_mask);
                rsp_info_deque_map::iterator i = m_tran_id_rsp_info[port].find(tran_id);
                if (i == m_tran_id_rsp_info[port].end()) {
                  ostringstream oss;
                  oss << kind() << " '" << name() << "': tran_id=" << tran_id << " has no deque in m_tran_id_rsp_info." 
                      << "  Is the downstream slave sending unexpected write responses?";
                  throw xtsc_exception(oss.str());
                }
                rsp_info_deque& rid = *(i->second);
                if (rid.empty()) {
                  ostringstream oss;
                  oss << kind() << " '" << name() << "': tran_id=" << tran_id << " has empty deque in m_tran_id_rsp_info."
                      << "  Is the downstream slave sending unexpected write responses?";
                  throw xtsc_exception(oss.str());
                }
                rid.pop_front();
                m_pending_rsp_info_cnt -= 1;
                XTSC_DEBUG(m_text, "Removed   m_tran_id_rsp_info[" << port << "] tran_id=" << tran_id << ": because request was last data");
              }
              delete p_info->m_p_response;
              p_info->m_p_response = 0;
              delete_response_info(p_info);
            }
            else {
              XTSC_DEBUG(m_text, "Deleting duplicated response_info " << p_info);
              delete_response_info(p_info);
            }
          }
          else {
            m_previous_response_last[port] = false;   // Treat as non-last so write responses can't cut in
            m_resp_rdy_fifo[port]->write(true);
            m_drive_resp_rdy_event[port].notify(m_output_delay);
            XTSC_INFO(m_text, *p_info->m_p_response << " Port #" << port << " <-- REJECTED");
          }
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



void xtsc_component::xtsc_tlm2pin_memory_transactor::pif_drive_resp_rdy_thread(void) {

  // Get the port number for this "instance" of the thread
  u32 port = m_next_port_pif_drive_resp_rdy_thread++;
  bool dummy = false;

  try {

    XTSC_INFO(m_text, "in pif_drive_resp_rdy_thread[" << port << "]");

    // Handle re-entry after reset
    if (sc_time_stamp() != SC_ZERO_TIME) {
      wait(SC_ZERO_TIME);  // Allow one delta cycle to ensure we can completely empty an sc_fifo
      while (m_resp_rdy_fifo[port]->nb_read(dummy));
    }

    m_p_resp_rdy[port]->write(true);

    // Loop forever
    while (true) {
      // Wait to be notified
      wait(m_drive_resp_rdy_event[port]);
      while (m_resp_rdy_fifo[port]->num_available()) {
        m_resp_rdy_fifo[port]->nb_read(dummy);
        m_p_resp_rdy[port]->write(false);
        XTSC_DEBUG(m_text, "pif_drive_resp_rdy_thread[" << port << "] deasserting PIRespRdy");
        wait(m_clock_period);
        m_p_resp_rdy[port]->write(true);
        XTSC_DEBUG(m_text, "pif_drive_resp_rdy_thread[" << port << "] asserting   PIRespRdy");
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



void xtsc_component::xtsc_tlm2pin_memory_transactor::axi_request_thread(void) {

  // Get the port number and signal indx for this "instance" of the thread
  u32 port = m_next_port_axi_request_thread++;
  u32 indx = m_axi_signal_indx[port];

  try {

    bool read_channel  = (m_axi_read_write[port] == 0);
    bool write_channel = (m_axi_read_write[port] == 1);
    bool snoop_channel = (m_axi_read_write[port] == 2);
    bool axi4          = (m_axi_port_type[port] == AXI4);
    u32  id_bits       = m_axi_id_bits[port];
    u32  rid_bits      = m_axi_route_id_bits[port];
    u32  id_bits_mask  = ((1 <<  id_bits) - 1);
    u32  rid_bits_mask = ((1 << rid_bits) - 1);

    XTSC_INFO(m_text, "in axi_request_thread[" << port << "]. Using " 
        << (read_channel ? "read" : write_channel ? "write" : "snoop") 
        << " signal indx=" << indx);

    sc_uint_base tran_id(rid_bits + id_bits);
    sc_uint_base snoop  (write_channel ? 3 : 4);
    sc_bv_base   data   ((int)m_width8 * 8);

    uint_output *AxBAR    = NULL;
    uint_output *AxDOMAIN = NULL;
    uint_output *AxSNOOP  = NULL;

    uint_output *AxID     = (read_channel ? m_p_arid    [indx] : write_channel ? m_p_awid    [indx] : NULL);
    uint_output *AxADDR   = (read_channel ? m_p_araddr  [indx] : write_channel ? m_p_awaddr  [indx] : m_p_acaddr[indx]);
    uint_output *AxLEN    = (read_channel ? m_p_arlen   [indx] : write_channel ? m_p_awlen   [indx] : NULL);
    uint_output *AxSIZE   = (read_channel ? m_p_arsize  [indx] : write_channel ? m_p_awsize  [indx] : NULL);
    uint_output *AxBURST  = (read_channel ? m_p_arburst [indx] : write_channel ? m_p_awburst [indx] : NULL);
    bool_output *AxLOCK   = (read_channel ? m_p_arlock  [indx] : write_channel ? m_p_awlock  [indx] : NULL);
    uint_output *AxCACHE  = (read_channel ? m_p_arcache [indx] : write_channel ? m_p_awcache [indx] : NULL);
    uint_output *AxPROT   = (read_channel ? m_p_arprot  [indx] : write_channel ? m_p_awprot  [indx] : m_p_acprot[indx]);
    uint_output *AxQOS    = (read_channel ? m_p_arqos   [indx] : write_channel ? m_p_awqos   [indx] : NULL);
    bool_output *AxVALID  = (read_channel ? m_p_arvalid [indx] : write_channel ? m_p_awvalid [indx] : m_p_acvalid[indx]);
    bool_input  *AxREADY  = (read_channel ? m_p_arready [indx] : write_channel ? m_p_awready [indx] : m_p_acready[indx]);

    if (!axi4) {
      AxBAR    = (read_channel ? m_p_arbar   [indx] : write_channel ? m_p_awbar   [indx] : NULL);
      AxDOMAIN = (read_channel ? m_p_ardomain[indx] : write_channel ? m_p_awdomain[indx] : NULL);
      AxSNOOP  = (read_channel ? m_p_arsnoop [indx] : write_channel ? m_p_awsnoop [indx] : m_p_acsnoop[indx]);
    }

    wide_output *WDATA    = (!write_channel ? NULL             : m_p_wdata [indx]);
    uint_output *WSTRB    = (!write_channel ? NULL             : m_p_wstrb [indx]);
    bool_output *WLAST    = (!write_channel ? NULL             : m_p_wlast [indx]);
    bool_output *WVALID   = (!write_channel ? NULL             : m_p_wvalid[indx]);
    bool_input  *WREADY   = (!write_channel ? NULL             : m_p_wready[indx]);

    AxVALID->write(false);
    if (write_channel) {
      WVALID->write(false);
    }

    if (!axi4 && !snoop_channel) {
      m_bar = 0;
      AxBAR->write(m_bar);
    }

    bool first_transfer = true;

    // Loop forever
    while (true) {

      if (m_hard_busy[port]) {
        m_hard_busy[port] = false;
        XTSC_INFO(m_text, "m_hard_busy[" << port << "] set false");
      }

      // Wait for nb_request to tell us there's something to do 
      wait(m_request_event[port]);

      while (!m_request_fifo[port].empty()) {

        xtsc_request *p_request = m_request_fifo[port].front();
        m_request_fifo[port].pop_front();

        // Create response from request
        xtsc_response *p_response = new xtsc_response(*p_request, xtsc_response::AXI_OK);

        // Pick out some useful information about the request
        type_t            type            = p_request->get_type();
        u32               size            = p_request->get_byte_size();
        xtsc_address      addr8           = p_request->get_byte_address();
        u32               bus_addr_bits   = (addr8 & m_bus_addr_bits_mask);
        u32               tid             = snoop_channel ? 0 : (((p_request->get_route_id() & rid_bits_mask) << id_bits) | (p_request->get_id() & id_bits_mask));
        u32               num_transfers   = p_request->get_num_transfers();
        bool              last_transfer   = p_request->get_last_transfer();

        // Must be a power of 2
             if (size ==  1) { m_size = 0; }
        else if (size ==  2) { m_size = 1; }
        else if (size ==  4) { m_size = 2; }
        else if (size ==  8) { m_size = 3; }
        else if (size == 16) { m_size = 4; }
        else if (size == 32) { m_size = 5; }
        else if (size == 64) { m_size = 6; }
        else {
          ostringstream oss;
          oss << __FUNCTION__ << "[" << port << "]:  Request with invalid size=" << size << ": " << *p_request;
          throw xtsc_exception(oss.str());
        }

        // Must not exceed bus width
        // This is a protocol error that could get a SLVERR response from downstream rather than an exception being thrown here
        if (size > m_width8) {
          ostringstream oss;
          oss << __FUNCTION__ << "[" << port << "]:  Request size=" << size << " exceeds bus width=" << m_width8 << ": " << *p_request;
          throw xtsc_exception(oss.str());
        }

        bool is_read  = (type == xtsc_request::BURST_READ);
        bool is_write = (type == xtsc_request::BURST_WRITE);
        bool is_snoop = (type == xtsc_request::SNOOP);

        if ((!is_read && !is_write && !is_snoop) || !p_request->is_axi_protocol()) {
          ostringstream oss;
          oss << __FUNCTION__ << "[" << port << "]:  Received non-AXI request: " << *p_request;
          throw xtsc_exception(oss.str());
        }

        if ((is_read && !read_channel) || (is_write && !write_channel)) { // consider snoop channel
          ostringstream oss;
          oss << __FUNCTION__ << "[" << port << "]:  Received a "
              << (is_read ? "read"  : "write") << " request on a "
              << (is_read ? "write" : "read" ) << " port: " << *p_request;
          throw xtsc_exception(oss.str());
        }

        if (first_transfer) {
          tran_id    = tid;
          m_address  = p_request->get_hardware_address();
          m_len      = num_transfers - 1;
          m_burst    = p_request->get_burst_bits();
          m_cache    = p_request->get_cache();
          m_prot     = p_request->get_prot();
          m_priority = p_request->get_priority();
          if (!is_snoop) {
            AxID   ->write(tran_id);
            AxLEN  ->write(m_len);
            AxSIZE ->write(m_size);
            AxBURST->write(m_burst);
            AxLOCK ->write(p_request->get_exclusive());
            AxCACHE->write(m_cache);
            AxQOS  ->write(m_priority);
          }
          AxADDR ->write(m_address);
          AxPROT ->write(m_prot);
          AxVALID->write(true);
          if (!axi4) {
            if (!is_snoop) {
              m_domain = p_request->get_domain();
              AxDOMAIN->write(m_domain);
            }
            snoop    = p_request->get_snoop();
            AxSNOOP ->write(snoop);
          }
          XTSC_VERBOSE(m_text, *p_request << " Port #" << port << " AxADDR   asserted");
        }

        if (is_write) {
          const u8 *buffer       = p_request->get_buffer();
          u32       bit_offset   = 8 * bus_addr_bits;
          u32       max_offset   = (m_width8 - 1) * 8; // Prevent SystemC "E5 out of bounds" - slave should give an SLVERR rsp
          u64       byte_enables = p_request->get_byte_enables();
          m_lane = (byte_enables & (0xFFFFFFFFFFFFFFFFull >> (64 - size))) << bus_addr_bits;
          data   = 0;
          for (u32 i = 0; (i<size) && (bit_offset <= max_offset); ++i) {
            data.range(bit_offset+7, bit_offset) = buffer[i];
            if (byte_enables & 0x1) {
              if (m_p_memory) {
                m_p_memory->write_u8(addr8+i, buffer[i]);
              }
            }
            byte_enables >>= 1;
            bit_offset += 8;
          }
          WDATA ->write(data);
          WSTRB ->write(m_lane);
          WLAST ->write(last_transfer);
          WVALID->write(true);
          XTSC_VERBOSE(m_text, *p_request << " Port #" << port << "  WDATA   asserted");
        }

        response_info *p_info = new_response_info(p_response, bus_addr_bits, size, is_read, tid, 0, num_transfers, p_request->get_burst_type());
        rsp_info_deque *p_rid = NULL;
        XTSC_DEBUG(m_text, "Adding to m_tran_id_rsp_info[" << port << "] tran_id=0x" << hex << tid << ": " << *p_info->m_p_response);
        rsp_info_deque_map::iterator i = m_tran_id_rsp_info[port].find(tid);
        if (i == m_tran_id_rsp_info[port].end()) {
          m_tran_id_rsp_info[port][tid] = new rsp_info_deque;
          i = m_tran_id_rsp_info[port].find(tid);
        }
        p_rid = i->second;
        p_rid->push_back(p_info);
        m_pending_rsp_info_cnt += 1;

        // Calculate time to sync to sample phase
        sc_time now = sc_time_stamp();
        sc_time phase_now = (now.value() % m_clock_period_value) * m_time_resolution;
        if (m_has_posedge_offset) {
          if (phase_now < m_posedge_offset) {
            phase_now += m_clock_period;
          }
          phase_now -= m_posedge_offset;
        }
        sc_time time_to_sample_phase = (((phase_now < m_sample_phase) ?  m_sample_phase : m_sample_phase_plus_one) - phase_now);
  
        bool addr_channel_accepted   = !first_transfer;
        bool addr_channel_deasserted = !first_transfer;
        bool data_channel_accepted   = !is_write;
        bool data_channel_deasserted = !is_write;
        bool first_pass              = true;
        bool doit_again              = false;   // For 1st transfer write when 1 channel (Write Addr or Write Data) accepts and other does not
        while (first_pass || doit_again) {
          XTSC_DEBUG(m_text, "axi_request_thread[" << port << "] doing wait for " << time_to_sample_phase << " to sync to sample phase");
          wait(time_to_sample_phase);
          XTSC_DEBUG(m_text, "axi_request_thread[" << port << "] done with wait");
          bool all_channels_rejected = true;
          if (!addr_channel_accepted) {
            addr_channel_accepted = AxREADY->read();
            if (addr_channel_accepted) all_channels_rejected = false;
          }
          if (!data_channel_accepted ) {
            data_channel_accepted = WREADY->read();
            if (data_channel_accepted) all_channels_rejected = false;
          }
          bool all_channels_accepted = addr_channel_accepted && data_channel_accepted;

          if (all_channels_accepted && !last_transfer) {
            p_rid->pop_back();
            m_pending_rsp_info_cnt -= 1;
            XTSC_DEBUG(m_text, "Removed   m_tran_id_rsp_info[" << port << "] tran_id=0x" << hex << tid << ": because request was not last data");
            delete_response_info(p_info);
            delete p_response;
            p_response = 0;
          }

          if (all_channels_rejected && first_pass) {
            // Request was not accepted
            p_rid->pop_back();
            m_pending_rsp_info_cnt -= 1;
            XTSC_DEBUG(m_text, "Removed   m_tran_id_rsp_info[" << port << "] tran_id=0x" << hex << tid << ": because request was not accepted");
            delete_response_info(p_info);
            p_response->set_status(xtsc_response::NOTRDY);
            p_response->set_last_transfer(true);
            send_unchecked_response(p_response, port);
          }

          if (!all_channels_accepted) {
            if (!m_request_fifo[port].empty()) {
              XTSC_INFO(m_text, "Emptying m_request_fifo[" << port << "]");
              while (!m_request_fifo[port].empty()) {
                xtsc_request *p_req = m_request_fifo[port].front();
                m_request_fifo[port].pop_front();
                xtsc_response *p_rsp = new xtsc_response(*p_req, xtsc_response::NOTRDY);
                send_unchecked_response(p_rsp, port);
                delete_request(p_req);
              }
            }
          }

          doit_again = (is_write && first_transfer && (addr_channel_accepted != data_channel_accepted));
          XTSC_DEBUG(m_text, "doit_again=" << doit_again << " addr_channel_accepted=" << addr_channel_accepted <<
                                                            " data_channel_accepted=" << data_channel_accepted);

          if (doit_again && !m_hard_busy[port]) {
            m_hard_busy[port] = true;
            XTSC_INFO(m_text, "m_hard_busy[" << port << "] set true due to half-accepted request" << 
                              ": Wr Addr chan: " << (addr_channel_accepted ? "Not Busy" : "Busy") <<
                              ", Wr Data chan: " << (data_channel_accepted ? "Not Busy" : "Busy"));
          }

          // Continue driving the request channel(s) for the balance of 1 clock period
          if (time_to_sample_phase != m_clock_period) {
            XTSC_DEBUG(m_text, "Port #" << port << " doing wait for " << (m_clock_period - time_to_sample_phase));
            wait(m_clock_period - time_to_sample_phase);
            XTSC_DEBUG(m_text, "Port #" << port << " done waiting");
          }

          if (!addr_channel_deasserted && (addr_channel_accepted || !doit_again)) {
            // Deassert the request address channel and drive all-zero values
            tran_id    = 0;
            m_address  = 0;
            m_len      = 0;
            m_size     = 0;
            m_burst    = 0;
            m_cache    = 0;
            m_prot     = 0;
            m_priority = 0;
            if (!is_snoop) {
              AxID   ->write(tran_id);
              AxLEN  ->write(m_len);
              AxSIZE ->write(m_size);
              AxBURST->write(m_burst);
              AxLOCK ->write(false);
              AxCACHE->write(m_cache);
              AxQOS  ->write(m_priority);
            }
            AxADDR ->write(m_address);
            AxPROT ->write(m_prot);
            AxVALID->write(false);
            if (!axi4) {
              if (!is_snoop) {
                m_domain = 0;
                AxDOMAIN->write(m_domain);
              }
              snoop    = 0;
              AxSNOOP ->write(snoop);
            }
            addr_channel_deasserted = true;
            XTSC_DEBUG(m_text, *p_request << " Port #" << port << " AxADDR deasserted");
          }

          if (!data_channel_deasserted && (data_channel_accepted || !doit_again)) {
            // Deassert the request data channel and drive all-zero values
            data       = 0;
            m_lane     = 0;
            WDATA ->write(data);
            WSTRB ->write(m_lane);
            WLAST ->write(false);
            WVALID->write(false);
            data_channel_deasserted = true;
            XTSC_DEBUG(m_text, *p_request << " Port #" << port << "  WDATA deasserted");
          }

          first_pass = false;
        };
        if (addr_channel_accepted && data_channel_accepted) {
          first_transfer = last_transfer;
          XTSC_DEBUG(m_text, *p_request << " Port #" << port << " Setting first_transfer=" << first_transfer);
        }
        delete_request(p_request);
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



void xtsc_component::xtsc_tlm2pin_memory_transactor::axi_response_thread(void) {

  // Get the port number and signal indx for this "instance" of the thread
  u32 port = m_next_port_axi_response_thread++;
  u32 indx = m_axi_signal_indx[port];

  try {

    bool read_channel  = (m_axi_read_write[port] == 0);
    bool write_channel = (m_axi_read_write[port] == 1);
    bool snoop_channel = (m_axi_read_write[port] == 2);

    XTSC_INFO(m_text, "in axi_response_thread[" << port << "]. Using " << (read_channel ? "read" : "write") << " signal indx=" << indx);

    sc_bv_base   data   ((int)m_width8 * 8);

    uint_input  *xID      = (read_channel ? m_p_rid[indx]   : write_channel ? m_p_bid[indx] : NULL);
    uint_input  *xRESP    = (read_channel ? m_p_rresp       : write_channel ? m_p_bresp  : m_p_crresp  )[indx];
    bool_input  *xVALID   = (read_channel ? m_p_rvalid      : write_channel ? m_p_bvalid : m_p_cdvalid )[indx];
    bool_output *xREADY   = (read_channel ? m_p_rready      : write_channel ? m_p_bready : m_p_cdready )[indx];

    wide_input  *xDATA    = (read_channel ? m_p_rdata[indx] : snoop_channel ? m_p_cddata[indx] : NULL        ); 
    bool_input  *xLAST    = (read_channel ? m_p_rlast[indx] : snoop_channel ? m_p_cdlast[indx] : NULL        ); 

    bool_input  *CRVALID  = (snoop_channel ? m_p_crvalid[indx] : NULL);
    bool_output *CRREADY  = (snoop_channel ? m_p_crready[indx] : NULL);

    // Loop forever
    while (true) {

      do {
        // Wait for either xVALID or xREADY to go high
        XTSC_DEBUG(m_text, "axi_response_thread[" << port << "]: calling wait()");
        wait(); // xVALID->pos()  xREADY->pos()
        XTSC_DEBUG(m_text, "axi_response_thread[" << port << "]: awoke from wait()");
      } while ((!xVALID->read() || !xREADY->read()) &&
               (!snoop_channel  || !CRVALID->read()));

      // Sync to sample phase.  If already at the sample phase, wait 1 full clock period to ensure
      // xVALID and xREADY are high a non-zero amount of time.
      sc_time now = sc_time_stamp();
      sc_time phase_now = (now.value() % m_clock_period_value) * m_time_resolution;
      if (m_has_posedge_offset) {
        if (phase_now < m_posedge_offset) {
          phase_now += m_clock_period;
        }
        phase_now -= m_posedge_offset;
      }
      wait(((phase_now >= m_sample_phase) ? m_sample_phase_plus_one : m_sample_phase) - phase_now);
      XTSC_DEBUG(m_text, "axi_response_thread[" << port << "]: sync'd to sample phase");

      // Once each clock period:
      //  1) Capture one pin-level response from downstream slave if both xVALID and xREADY are asserted
      //  2) Attempt to send current TLM response to upstream master
      while ((xVALID->read() && xREADY->read()) || 
             (snoop_channel && CRREADY->read() && CRVALID->read()) ||
              !m_system_response_deque[port].empty()) {

        // snoop response
        if (snoop_channel && CRVALID->read() && CRREADY->read()) {
          u32 resp    = xRESP->read().to_uint();
          response_info *p_info  = NULL;
          rsp_info_deque_map::iterator i = m_tran_id_rsp_info[port].find(0);
          if (i != m_tran_id_rsp_info[port].end()) {
            rsp_info_deque& rid = *(i->second);
            if (!rid.empty()) {
              p_info = rid.front();
            }
          }
          if (!p_info) {
            ostringstream oss;
            oss << kind() << " '" << name() << "' got snoop response with no corresponding request.";
            throw xtsc_exception(oss.str());
          }
          p_info->m_status = xtsc_response::AXI_OK;
          // pass dirty, is_shared .... TODO: pass Error and WasUnique
          p_info->m_p_response->set_snoop_data(resp & 0x1);
          p_info->m_p_response->set_pass_dirty(resp & 0x4);
          p_info->m_p_response->set_is_shared(resp & 0x8);
          // if no data, skip the next part
          if ((resp & 0x1) == 0) {// no data
            p_info->m_last = true;
            m_system_response_deque[port].push_back(p_info);
          }
        }
      
        // Capture and save pin-level response (if there is one this clock cycle)
        if (xVALID->read() && xREADY->read()) {
          u32 tran_id = snoop_channel ? 0 : xID->read().to_uint();
          response_info *p_info  = NULL;
          rsp_info_deque_map::iterator i = m_tran_id_rsp_info[port].find(tran_id);
          if (i != m_tran_id_rsp_info[port].end()) {
            rsp_info_deque& rid = *(i->second);
            if (!rid.empty()) {
              p_info = rid.front();
            }
          }
          if (!p_info) {
            ostringstream oss;
            oss << kind() << " '" << name() << "' got response tran_id=" << tran_id << " with no corresponding request.";
            throw xtsc_exception(oss.str());
          }
          p_info->m_last = (write_channel ? true : xLAST->read());
          bool exclusive_ok = false;
          if (!snoop_channel) {
            u32 resp    = xRESP->read().to_uint();
            p_info->m_status = get_xttlm_info_from_axi_resp(resp, exclusive_ok);
          }

          if (!p_info->m_last) {
            // This should only happen for a BURST_READ, SNOOP with data
            p_info = new_response_info(*p_info);
            XTSC_DEBUG(m_text, "Created duplicated response_info " << p_info);
          }
          u32 transfer_num = m_transfer_number[port];
          m_transfer_number[port] += 1;
          p_info->m_p_response->set_transfer_number(m_transfer_number[port]);
          if (p_info->m_last) {
            m_transfer_number[port] = 0;
          }
          p_info->m_p_response->set_exclusive_ok(exclusive_ok);
          if (read_channel || (snoop_channel && (xRESP->read().to_uint() & 0x1))) {
            p_info->m_copy_data = true;
            data = xDATA->read();
            bool wrap          = p_info->m_burst == xtsc_request::WRAP;
            bool fixed         = p_info->m_burst == xtsc_request::FIXED;
            u32  bus_addr_bits = p_info->m_bus_addr_bits;
            u32  size          = p_info->m_size;
            u8  *buffer        = p_info->m_buffer;
            u32  byte_offset   = (bus_addr_bits + (fixed ? 0 : size * transfer_num));
            u32  max_offset    = (m_width8 - 1) * 8; // Prevent SystemC "E5 out of bounds" - slave should have given an SLVERR response
            u32  total_size8   = size * p_info->m_num_transfers;
            u32  lower_wrap    = (bus_addr_bits / total_size8) * total_size8;
            u32  upper_wrap    = lower_wrap + total_size8;
            if (wrap) {
              if (byte_offset >= upper_wrap ) {
                byte_offset -= total_size8;
              }
            }
            u32  bit_offset    = (byte_offset % m_width8) * 8;
            for (u32 i = 0; (i < size) && (bit_offset <= max_offset); ++i) {
              buffer[i] = (u8) data.range(bit_offset+7, bit_offset).to_uint();
              bit_offset += 8;
            }
          }
          m_system_response_deque[port].push_back(p_info);
          XTSC_VERBOSE(m_text, "axi_response_thread[" << port << "]: Got response: tran_id=" << tran_id << " status=" << p_info->m_status <<
                               " tag=" << p_info->m_p_response->get_tag() << " address=0x" << setfill('0') << hex <<
                               setw(8) << p_info->m_p_response->get_byte_address());
        }

        if (!m_system_response_deque[port].empty()) {

          // Now handle current TLM response
          response_info *p_info = m_system_response_deque[port].front();

          if (p_info->m_copy_data) {
            memcpy(p_info->m_p_response->get_buffer(), p_info->m_buffer, p_info->m_size);
            p_info->m_copy_data = false;
          }
          p_info->m_p_response->set_last_transfer(p_info->m_last);
          p_info->m_p_response->set_status((xtsc_response::status_t) p_info->m_status);

          // Log the response
          XTSC_INFO(m_text, *p_info->m_p_response << " Port #" << port);
          
          bool accepted = (*m_respond_ports[port])->nb_respond(*p_info->m_p_response);

          if (accepted) {
            m_system_response_deque[port].pop_front();
            // Delete it?
            if (p_info->m_last) {
              u32 tran_id = p_info->m_id;
              rsp_info_deque_map::iterator i = m_tran_id_rsp_info[port].find(tran_id);
              if (i == m_tran_id_rsp_info[port].end()) {
                ostringstream oss;
                oss << kind() << " '" << name() << "': tran_id=" << tran_id << " has no entry in m_tran_id_rsp_info.";
                throw xtsc_exception(oss.str());
              }
              rsp_info_deque& rid = *(i->second);
              if (rid.empty()) {
                ostringstream oss;
                oss << kind() << " '" << name() << "': tran_id=" << tran_id << " has empty deque in m_tran_id_rsp_info.";
                throw xtsc_exception(oss.str());
              }
              rid.pop_front();
              m_pending_rsp_info_cnt -= 1;
              XTSC_DEBUG(m_text, "Removed   m_tran_id_rsp_info[" << port << "] tran_id=" << tran_id << ": because request was last data");
              delete p_info->m_p_response;
              p_info->m_p_response = 0;
              delete_response_info(p_info);
            }
            else {
              XTSC_DEBUG(m_text, "Deleting duplicated response_info " << p_info);
              delete_response_info(p_info);
            }
          }
          else {
            m_resp_rdy_fifo[port]->write(true);
            m_drive_resp_rdy_event[port].notify(m_output_delay);
            XTSC_INFO(m_text, *p_info->m_p_response << " Port #" << port << " <-- REJECTED");
          }
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



void xtsc_component::xtsc_tlm2pin_memory_transactor::axi_drive_resp_rdy_thread(void) {

  // Get the port number and signal indx for this "instance" of the thread
  u32 port = m_next_port_axi_drive_resp_rdy_thread++;
  u32 indx = m_axi_signal_indx[port];
  bool dummy = false;

  try {

    bool read_channel  = (m_axi_read_write[port] == 0);
    bool write_channel = (m_axi_read_write[port] == 1);
    bool snoop_channel = (m_axi_read_write[port] == 2);

    XTSC_INFO(m_text, "in axi_drive_resp_rdy_thread[" << port << "]. Using " << (read_channel ? "read" : "write") << " signal indx=" << indx);

    bool_output *xREADY   = (read_channel ? m_p_rready   : write_channel ? m_p_bready : m_p_cdready  )[indx];

    // Handle re-entry after reset
    if (sc_time_stamp() != SC_ZERO_TIME) {
      wait(SC_ZERO_TIME);  // Allow one delta cycle to ensure we can completely empty an sc_fifo
      while (m_resp_rdy_fifo[port]->nb_read(dummy));
    }

    xREADY->write(true);
    if (snoop_channel) {
      //XXXSS: Is it ok to keep crready always asserted?
      m_p_crready[indx]->write(true);
    }

    // Loop forever
    while (true) {
      // Wait to be notified
      wait(m_drive_resp_rdy_event[port]);
      while (m_resp_rdy_fifo[port]->num_available()) {
        m_resp_rdy_fifo[port]->nb_read(dummy);
        xREADY->write(false);
        XTSC_DEBUG(m_text, "axi_drive_resp_rdy_thread[" << port << "] deasserting " 
            << (read_channel ? "R" : write_channel ? "B" : "CD") << "READY");
        wait(m_clock_period);
        xREADY->write(true);
        XTSC_DEBUG(m_text, "axi_drive_resp_rdy_thread[" << port << "] asserting   "
            << (read_channel ? "R" : write_channel ? "B" : "CD") << "READY");
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



void xtsc_component::xtsc_tlm2pin_memory_transactor::apb_thread(void) {

  // Get the port number for this "instance" of the thread
  u32 port = m_next_port_apb_request_thread++;

  // A try/catch block in sc_main will not catch an exception thrown from
  // an SC_THREAD, so we'll catch them here, log them, then rethrow them.
  try {

    XTSC_INFO(m_text, "in " << __FUNCTION__ << "[" << port << "]");

    apb_drive_zeroes(port);

    // Loop forever
    while (true) {

      // Wait for nb_request to tell us there's something to do 
      wait(m_request_event[port]);

      while (!m_request_fifo[port].empty()) {

        xtsc_request *p_request = m_request_fifo[port].front();

        // Pick out some useful information about the request
        type_t            type            = p_request->get_type();
        xtsc_address      addr8           = p_request->get_byte_address();
        u32               size            = p_request->get_byte_size();
        xtsc_byte_enables byte_enables    = p_request->get_byte_enables();
        u32               bus_addr_bits   = (addr8 & m_bus_addr_bits_mask);

        // Must be a power of 2 and may not exceed bus width
        if (((size != 1) && (size != 2) && (size != 4) && (size != 8) && (size != 16)) || (size > m_width8)) {
          ostringstream oss;
          oss << kind() << "::" << __FUNCTION__ << "():  Request with invalid (not power of 2) size=" << size 
              << " or exceed bus width=" << m_width8 << ": " << *p_request;
          throw xtsc_exception(oss.str());
        }

        xtsc_byte_enables fixed_byte_enables = (byte_enables & (0xFFFFFFFFFFFFFFFFull >> (64 - size))) << bus_addr_bits;

        bool is_read  = (type == xtsc_request::READ);
        bool is_write = (type == xtsc_request::WRITE);

        if (!is_read && !is_write) {
          ostringstream oss;
          oss << kind() << "::" << __FUNCTION__ << "():  Received illegal APB request type: " << *p_request;
          throw xtsc_exception(oss.str());
        }

        m_address       = p_request->get_hardware_address();
        m_lane          = (is_read ? 0 : fixed_byte_enables);
        m_prot          = p_request->get_prot();
        m_data          = 0;

        if (is_write) {
          const u8     *buffer          = p_request->get_buffer();
          u32           delta           = 8;
          u32           bit_offset      = 8 * bus_addr_bits;
          u64           bytes           = byte_enables;
          u32           max_offset      = (m_width8 - 1) * 8; // Prevent SystemC "E5 out of bounds" - slave should assert PSLVERR TODO
          for (u32 i = 0; (i<size) && (bit_offset <= max_offset); ++i) {
            if (bytes & 0x1) {
              m_data.range(bit_offset+7, bit_offset) = buffer[i];
              if (m_p_memory) {
                // Skip unaligned - they should cause PSLVERR to assert   TODO
                if ((addr8 % size) == 0) {
                  m_p_memory->write_u8(addr8+i, buffer[i]);
                }
              }
            }
            bytes >>= 1;
            bit_offset += delta;
          }
        }

        m_p_psel    [port]->write(true);
        m_p_penable [port]->write(false);
        m_p_pwrite  [port]->write(is_write);
        m_p_paddr   [port]->write(m_address);
        m_p_pprot   [port]->write(m_prot);
        m_p_pstrb   [port]->write(m_lane);
        m_p_pwdata  [port]->write(m_data);

        // Wait one clock and drive enable on 2nd cycle of request
        wait(m_clock_period);
        m_p_penable[port]->write(true);

        // Sync to sample phase
        sc_time now = sc_time_stamp();
        sc_time phase_now = (now.value() % m_clock_period_value) * m_time_resolution;
        if (m_has_posedge_offset) {
          if (phase_now < m_posedge_offset) {
            phase_now += m_clock_period;
          }
          phase_now -= m_posedge_offset;
        }
        sc_time time_to_sample_phase = (((phase_now < m_sample_phase) ?  m_sample_phase : m_sample_phase_plus_one) - phase_now);
        wait(time_to_sample_phase);

        // Check for accepted at 1 cycle intervals
        while (!m_p_pready[port]->read()) {
          wait(m_clock_period);
        }

        // Create response from request
        xtsc_response *p_response = new xtsc_response(*p_request, (m_p_pslverr[port]->read() ? xtsc_response::SLVERR : xtsc_response::APB_OK));

        // Extract read data
        if (is_read) {
          m_data = m_p_prdata[port]->read();
          u8 *buffer        = p_response->get_buffer();
          u32 delta         = 8;
          u32 bit_offset    = 8 * bus_addr_bits;
          for (u32 i = 0; i<size; ++i) {
            buffer[i] = (u8) m_data.range(bit_offset+7, bit_offset).to_uint();
            bit_offset += delta;
          }
        }

        // Send response
        send_unchecked_response(p_response, port);

        // Remove request - this allows another request to come in starting now
        m_request_fifo[port].pop_front();
        delete_request(p_request);

        // Continue driving the request for the balance of 1 clock period
        if (time_to_sample_phase != m_clock_period) {
          wait(m_clock_period - time_to_sample_phase);
        }
      }

      // Deassert the request and drive all-zero values
      apb_drive_zeroes(port);
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



void xtsc_component::xtsc_tlm2pin_memory_transactor::apb_drive_zeroes(u32 port) {
    m_address   = 0;
    m_lane      = 0;
    m_data      = 0;
    m_prot      = 0;

    m_p_psel    [port]->write(false);
    m_p_psel    [port]->write(false);
    m_p_penable [port]->write(false);
    m_p_pwrite  [port]->write(false);
    m_p_paddr   [port]->write(m_address);
    m_p_pprot   [port]->write(m_prot);
    m_p_pstrb   [port]->write(m_lane);
    m_p_pwdata  [port]->write(m_data);
}



void xtsc_component::xtsc_tlm2pin_memory_transactor::lcl_request_thread(void) {

  // Get the port number for this "instance" of the thread
  u32 port = m_next_port_lcl_request_thread++;

  try {

    XTSC_INFO(m_text, "in lcl_request_thread[" << port << "]");

    if (m_p_en     [port]) m_p_en     [port]->write(false);
    if (m_p_wr     [port]) m_p_wr     [port]->write(false);
    if (m_p_xfer_en[port]) m_p_xfer_en[port]->write(false);
    if (m_p_ram0   [port]) m_p_ram0   [port]->write(false);
    if (m_p_ram1   [port]) m_p_ram1   [port]->write(false);
    if ((m_interface_type != XLMI0)) {
    if (m_p_load   [port]) m_p_load   [port]->write(false);
    }

    // If present, drive 0 on parity/ECC signal
    if (m_p_check_wr[port]) {
      sc_bv_base dummy((int)m_check_bits);
      dummy = 0;
      m_p_check_wr[port]->write(dummy);
    }

    // Loop forever
    while (true) {

      // Wait for nb_request to tell us there's something to do 
      wait(m_request_event[port]);

      while (!m_request_fifo[port].empty()) {

        xtsc_request *p_request = m_request_fifo[port].front();
        m_request_fifo[port].pop_front();

        // Create response from request
        xtsc_response *p_response = new xtsc_response(*p_request, xtsc_response::RSP_OK);

        // Pick out some useful information about the request
        xtsc_address      addr8           = p_request->get_byte_address() - m_start_byte_address;
        u32               size            = p_request->get_byte_size();
        xtsc_byte_enables byte_enables    = p_request->get_byte_enables();
        u32               bus_addr_bits   = (addr8 & m_bus_addr_bits_mask);
        bool              inst_fetch      = p_request->get_instruction_fetch();

        // Must be a power of 2  and may not exceed bus width
        if (((size != 1) && (size != 2) && (size != 4) && (size != 8) && (size != 16) && (size != 32) && (size != 64)) ||
            (size > m_width8))
        {
          ostringstream oss;
          oss << "xtsc_tlm2pin_memory_transactor::lcl_request_thread():  Request with invalid (not power of 2) size=" << size 
              << " or exceed bus width=" << m_width8 << ": " << *p_request;
          throw xtsc_exception(oss.str());
        }

        m_address = ((addr8 & m_address_mask) >> m_address_shift);

        // Optionally, compute and drive byte/word lanes 
        if (m_p_lane[port]) {
          xtsc_byte_enables fixed_byte_enables = (byte_enables & (0xFFFFFFFFFFFFFFFFull >> (64 - size))) << bus_addr_bits;
#if 0
          XTSC_NOTE(m_text, "addr8=0x" << hex << addr8 << " byte_enables=0x" << byte_enables <<
                            " fixed_byte_enables=0x" << fixed_byte_enables << " size=" << dec << size <<
                            " bus_addr_bits=" << bus_addr_bits);
#endif
          if (m_big_endian) {
            swizzle_byte_enables(fixed_byte_enables);
          }
          if ((m_interface_type == IRAM0) || (m_interface_type == IRAM1) || (m_interface_type == IROM0)) {
            // Convert byte enables to word enables (1 "word" is 32 bits) (256-bit IRAM0|IRAM1|IROM0 only)
            xtsc_byte_enables fixed_word_enables = 0;
            if (fixed_byte_enables & 0x0000000F) fixed_word_enables |= 0x1;
            if (fixed_byte_enables & 0x000000F0) fixed_word_enables |= 0x2;
            if (fixed_byte_enables & 0x00000F00) fixed_word_enables |= 0x4;
            if (fixed_byte_enables & 0x0000F000) fixed_word_enables |= 0x8;
            if (fixed_byte_enables & 0x000F0000) fixed_word_enables |= 0x10;
            if (fixed_byte_enables & 0x00F00000) fixed_word_enables |= 0x20;
            if (fixed_byte_enables & 0x0F000000) fixed_word_enables |= 0x40;
            if (fixed_byte_enables & 0xF0000000) fixed_word_enables |= 0x80;
            m_lane = fixed_word_enables;
          }
          else {
            m_lane = fixed_byte_enables;
          }
          m_p_lane[port]->write(m_lane);
        }

        // Optionally, drive IRamNLoadStore|IRom0Load|URam0LoadStore
        if (m_p_load[port] && (m_interface_type != XLMI0)) {
          m_p_load[port]->write(!inst_fetch);
          if (!inst_fetch) {
            XTSC_VERBOSE(m_text, m_p_load[port]->name() << " asserted");
          }
        }

        // Optionally, drive DRamNXferEn0|IRamNXferEn0|IRom0XferEn0|URam0XferEn0
        if (m_has_xfer_en) {
          m_p_xfer_en[port]->write(p_request->get_xfer_en());
        }

        // Handle request according to its type
        type_t type = p_request->get_type();
        bool is_read = (type == xtsc_request::READ);
        if (is_read && m_p_data[port]) {
          if (m_interface_type == XLMI0) {
            // DPortLoad is asserted 1 cycle after the request
            m_load_event_queue[port].notify(m_clock_period);
            m_load_deque[port].push_back(true);
            // . . . so it needs to be deassert 2 cycles after the request
            m_load_event_queue[port].notify(2*m_clock_period);
            m_load_deque[port].push_back(false);
          }
          m_p_en[port]->write(true);
          if (m_p_wr[port]) m_p_wr[port]->write(false);
          m_p_addr[port]->write(m_address);
          if (m_p_attr[port]) {
            if (p_request->has_dram_attribute()) {
              p_request->get_dram_attribute(m_dram_attribute);
              m_dram_attribute_bv = m_dram_attribute;
            }
            else {
              m_dram_attribute_bv = 0;
            }
            m_p_attr[port]->write(m_dram_attribute_bv);
          }
          if (m_external_udma) {
            bool bit = ((addr8 & m_ram_select_mask) != 0);
            bool ram1 = ((bit && m_ram_select_normal) || (!bit && !m_ram_select_normal));
            m_p_ram0[port]->write(!ram1);
            m_p_ram1[port]->write(ram1);
          }
          XTSC_DEBUG(m_text, "READ:  m_address=0x" << hex << m_address << " lane=0x" << m_lane);
        }
        else if ((type == xtsc_request::WRITE) && m_p_wrdata[port]) {
          m_data = 0;
          const u8 *buffer = p_request->get_buffer();
          u32 delta      = (m_big_endian ? -8 : +8);
          u32 bit_offset = 8 * (m_big_endian ? (m_width8 - bus_addr_bits - 1) : bus_addr_bits);
          u64 bytes      = byte_enables;
          for (u32 i = 0; i<size; ++i) {
            if (bytes & 0x1) {
              m_data.range(bit_offset+7, bit_offset) = buffer[i];
              if (m_p_memory) {
                m_p_memory->write_u8(m_start_byte_address+addr8+i, buffer[i]);
              }
            }
            bytes >>= 1;
            bit_offset += delta;
          }
          if (m_p_en[port]) m_p_en[port]->write(true);
          if (m_p_wr[port]) m_p_wr[port]->write(true);
          m_p_addr[port]->write(m_address);
          m_p_wrdata[port]->write(m_data);
          if (m_p_attr[port]) {
            if (p_request->has_dram_attribute()) {
              p_request->get_dram_attribute(m_dram_attribute);
              m_dram_attribute_bv = m_dram_attribute;
            }
            else {
              m_dram_attribute_bv = 0;
            }
            m_p_attr[port]->write(m_dram_attribute_bv);
          }
          if (m_external_udma) {
            bool bit = ((addr8 & m_ram_select_mask) != 0);
            bool ram1 = ((bit && m_ram_select_normal) || (!bit && !m_ram_select_normal));
            m_p_ram0[port]->write(!ram1);
            m_p_ram1[port]->write(ram1);
          }
          XTSC_DEBUG(m_text, "WRITE: m_address=0x" << hex << m_address << " lane=0x" << m_lane << " data=0x" << m_data);
        }
        else {
          // READ isn't support on the Wr ports of split Rd/Wr DRAM0RW and DRAM1RW
          // BLOCK_READ, BLOCK_WRITE, BURST_READ, BURST_WRITE, and RCW aren't supported on local memory interfaces
          // WRITE isn't support on IROM and DROM nor on the Rd ports of split Rd/Wr DRAM0RW and DRAM1RW
          ostringstream oss;
          oss << "Unsupported request type=" << p_request->get_type_name() << " passed to " << m_interface_uc << " Port #" << port;
          throw xtsc_exception(oss.str());
        }

        sc_time now = sc_time_stamp();
        sc_time phase_now = (now.value() % m_clock_period_value) * m_time_resolution;
        if (m_has_posedge_offset) {
          if (phase_now < m_posedge_offset) {
            phase_now += m_clock_period;
          }
          phase_now -= m_posedge_offset;
        }
        sc_time delay_time = (((phase_now < m_sample_phase) ? m_sample_phase : m_sample_phase_plus_one) - phase_now);
        response_info *p_info = new_response_info(p_response, bus_addr_bits, size, is_read);
        if (m_has_busy || !is_read) {
          m_busy_write_rsp_deque[port].push_back(p_info);
          m_busy_write_rsp_event_queue[port].notify(delay_time);
        }
        else {
          m_read_data_rsp_deque[port].push_back(p_info);
          m_read_data_event_queue[port].notify(delay_time + m_read_delay_time);
        }

        wait(m_clock_period);

        // Deassert the request
        if (m_p_load[port] && (m_interface_type != XLMI0)) {
          m_p_load[port]->write(false);
        }
        if (m_p_lane[port]) {
          m_lane = 0;
          m_p_lane[port]->write(m_lane);
        }
        if (m_has_xfer_en) {
          m_p_xfer_en[port]->write(false);
        }
        m_address = 0;
        if (m_p_en[port]) m_p_en[port]->write(false);
        if (m_p_wr[port]) m_p_wr[port]->write(false);
        m_p_addr[port]->write(m_address);
        if (m_p_wrdata[port]) {
          m_data = 0;
          m_p_wrdata[port]->write(m_data);
        }
        if (m_external_udma) {
          m_p_ram0[port]->write(false);
          m_p_ram1[port]->write(false);
        }
        delete_request(p_request);
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



void xtsc_component::xtsc_tlm2pin_memory_transactor::lcl_busy_write_rsp_thread(void) {

  // Get the port number for this "instance" of the thread
  u32 port = m_next_port_lcl_busy_write_rsp_thread++;

  try {

    XTSC_INFO(m_text, "in lcl_busy_write_rsp_thread[" << port << "]");

    bool_input *p_busy_port   = !m_has_busy ? NULL : m_p_busy[(m_num_subbanks < 2) ? port : (port/m_num_subbanks)*m_num_subbanks];
    bool        busy_encoding = !m_external_udma;

    // Loop forever
    while (true) {
      wait();   // m_busy_write_rsp_event_queue[port]
      response_info *p_info = m_busy_write_rsp_deque[port].front();
      m_busy_write_rsp_deque[port].pop_front();
      bool send_response = true;
      bool busy          = false;
      bool read          = false;
      if (m_has_busy) {
        if (p_busy_port->read() == busy_encoding) {
          // It is busy (or not ready in the case of external micro-DMA) so send RSP_NACC
          p_info->m_p_response->set_status(xtsc_response::RSP_NACC);
          busy = true;
        }
        else if (p_info->m_is_read) {
          // Not busy and is a read so schedule read data
          send_response = false;
          m_read_data_rsp_deque[port].push_back(p_info);
          m_read_data_event_queue[port].notify(m_read_delay_time);
          read = true;
        }
      }
      if (!read && !busy && (m_is_7_stage)) {
        m_7stage_write_rsp_deque[port].push_back(p_info);
        m_7stage_write_rsp_event_queue[port].notify(m_clock_period);
        send_response = false;
      }
      if (send_response) {
        send_unchecked_response(p_info->m_p_response, port);
        delete_response_info(p_info);
      }
    }

  }
  catch (const sc_unwind_exception& error) { throw; }
  catch (const exception& error) {
    ostringstream oss;
    oss << "std::exception caught in " << __FUNCTION__ << "[" << port << "], an SC_THREAD of " << kind() << " '" << name() << "'."
        << endl;
    oss << "what(): " << error.what() << endl;
    oss << __FUNCTION__ << " now rethrowing:";
    xtsc_log_multiline(m_text, FATAL_LOG_LEVEL, oss.str(), 2);
    throw;
  }

}



void xtsc_component::xtsc_tlm2pin_memory_transactor::lcl_7stage_write_rsp_thread(void) {

  // Get the port number for this "instance" of the thread
  u32 port = m_next_port_lcl_7stage_write_rsp_thread++;

  try {

    XTSC_INFO(m_text, "in lcl_7stage_write_rsp_thread[" << port << "]");

    // Loop forever
    while (true) {
      wait();   // m_7stage_write_rsp_event_queue[port]
      response_info *p_info = m_7stage_write_rsp_deque[port].front();
      m_7stage_write_rsp_deque[port].pop_front();
      send_unchecked_response(p_info->m_p_response, port);
      delete_response_info(p_info);
    }

  }
  catch (const sc_unwind_exception& error) { throw; }
  catch (const exception& error) {
    ostringstream oss;
    oss << "std::exception caught in " << __FUNCTION__ << "[" << port << "], an SC_THREAD of " << kind() << " '" << name() << "'."
        << endl;
    oss << "what(): " << error.what() << endl;
    oss << __FUNCTION__ << " now rethrowing:";
    xtsc_log_multiline(m_text, FATAL_LOG_LEVEL, oss.str(), 2);
    throw;
  }

}



void xtsc_component::xtsc_tlm2pin_memory_transactor::lcl_sample_read_data_thread(void) {

  // Get the port number for this "instance" of the thread
  u32 port = m_next_port_lcl_sample_read_data_thread++;
  if (m_split_rw) {
    // Only the even port numbers are Rd ports when using a split Rd/Wr interface
    m_next_port_lcl_sample_read_data_thread += 1;
  }

  try {

    XTSC_INFO(m_text, "in lcl_sample_read_data_thread[" << port << "]");

    // Loop forever
    while (true) {
      wait();   // m_read_data_event_queue[port]
      response_info *p_info = m_read_data_rsp_deque[port].front();
      m_read_data_rsp_deque[port].pop_front();
      m_data = m_p_data[port]->read();
      u32 bus_addr_bits = p_info->m_bus_addr_bits;
      u32 size          = p_info->m_size;
      u8 *buffer        = p_info->m_p_response->get_buffer();
      u32 delta         = (m_big_endian ? -8 : +8);
      u32 bit_offset    = 8 * (m_big_endian ? (m_width8 - bus_addr_bits - 1) : bus_addr_bits);
      for (u32 i = 0; i<size; ++i) {
        buffer[i] = (u8) m_data.range(bit_offset+7, bit_offset).to_uint();
        bit_offset += delta;
      }
      send_unchecked_response(p_info->m_p_response, port);
      delete_response_info(p_info);
    }

  }
  catch (const sc_unwind_exception& error) { throw; }
  catch (const exception& error) {
    ostringstream oss;
    oss << "std::exception caught in " << __FUNCTION__ << "[" << port << "], an SC_THREAD of " << kind() << " '" << name() << "'."
        << endl;
    oss << "what(): " << error.what() << endl;
    oss << __FUNCTION__ << " now rethrowing:";
    xtsc_log_multiline(m_text, FATAL_LOG_LEVEL, oss.str(), 2);
    throw;
  }

}



// Handle DPortLoadRetired
void xtsc_component::xtsc_tlm2pin_memory_transactor::xlmi_retire_thread(void) {

  // Get the port number for this "instance" of the thread
  u32 port = m_next_port_xlmi_retire_thread++;

  try {

    XTSC_INFO(m_text, "in xlmi_retire_thread[" << port << "]");

    // Loop forever
    while (true) {
      m_p_retire[port]->write(false);
      wait(m_retire_event[port]);
      m_p_retire[port]->write(true);
      sc_time now = sc_time_stamp();
      while (m_retire_deassert[port] > now) {
        wait(m_retire_deassert[port] - now);
        now = sc_time_stamp();
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



void xtsc_component::xtsc_tlm2pin_memory_transactor::xlmi_flush_thread(void) {

  // Get the port number for this "instance" of the thread
  u32 port = m_next_port_xlmi_flush_thread++;

  try {

    XTSC_INFO(m_text, "in xlmi_flush_thread[" << port << "]");

    // Loop forever
    while (true) {
      m_p_flush[port]->write(false);
      wait(m_flush_event[port]);
      m_p_flush[port]->write(true);
      sc_time now = sc_time_stamp();
      while (m_flush_deassert[port] > now) {
        wait(m_flush_deassert[port] - now);
        now = sc_time_stamp();
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



// Handle DRamLock
void xtsc_component::xtsc_tlm2pin_memory_transactor::dram_lock_thread(void) {

  // Get the port number for this "instance" of the thread
  u32 port = m_next_port_dram_lock_thread;
  // Split Rd/Wr: Only the Rd ports (even port numbers) have a lock
  // Subbanked:   Only the first subbank of each bank has a lock
  m_next_port_dram_lock_thread += (m_split_rw ? 2 : (m_num_subbanks < 2) ? 1 : m_num_subbanks);

  try {

    XTSC_INFO(m_text, "in dram_lock_thread[" << port << "]");
    m_p_lock[port]->write(false);

    // Loop forever
    while (true) {
      wait();   // m_lock_event_queue[port]
      bool lock = m_lock_deque[port].front();
      m_lock_deque[port].pop_front();
      m_p_lock[port]->write(lock);
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



// Drive m_p_load/DPortLoad (without busy) or m_p_preload (with busy)
void xtsc_component::xtsc_tlm2pin_memory_transactor::xlmi_load_thread(void) {

  // Get the port number for this "instance" of the thread
  u32 port = m_next_port_xlmi_load_thread++;

  try {

    XTSC_INFO(m_text, "in xlmi_load_thread[" << port << "]");

    if (m_has_busy) {
      m_p_preload[port]->write(false);
    }
    else {
      m_p_load[port]->write(false);
    }

    // Loop forever
    while (true) {
      wait();   // m_load_event_queue[port]
      bool load = m_load_deque[port].front();
      m_load_deque[port].pop_front();
      if (m_has_busy) {
        m_p_preload[port]->write(load);
      }
      else {
        m_p_load[port]->write(load);
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



// Drive m_p_load/DPortLoad in case where XLMI has busy (because DPortLoad is qualified by DPortBusy)
void xtsc_component::xtsc_tlm2pin_memory_transactor::xlmi_load_method(void) {
  for (u32 port=0; port<m_num_ports; ++port) {
    m_p_load[port]->write(m_p_preload[port]->read() && !m_p_busy[port]->read());
  }
}



void xtsc_component::xtsc_tlm2pin_memory_transactor::send_unchecked_response(xtsc_response*& p_response, u32 port) {
  // Log the response
  XTSC_INFO(m_text, *p_response << " Port #" << port);
  
  // Send response
  (*m_respond_ports[port])->nb_respond(*p_response);

  // Delete it?
  if (!is_system_memory(m_interface_type) || (p_response->get_last_transfer())) {
    delete p_response;
    p_response = 0;
  }
}



xtsc_component::xtsc_tlm2pin_memory_transactor::
response_info *xtsc_component::xtsc_tlm2pin_memory_transactor::new_response_info(xtsc_response    *p_response,
                                                                                 u32               bus_addr_bits,
                                                                                 u32               size,
                                                                                 bool              is_read,
                                                                                 u32               id,
                                                                                 u32               route_id,
                                                                                 u32               num_transfers,
                                                                                 burst_t           burst)
{
  if (m_response_pool.empty()) {
    response_info *p_response_info = new response_info(p_response, bus_addr_bits, size, is_read, id, route_id, num_transfers, burst);
    XTSC_DEBUG(m_text, "Created new response_info (" << p_response_info << ") from an xtsc_response: " << *p_response);
    return p_response_info;
  }
  else {
    response_info *p_response_info = m_response_pool.back();
    XTSC_DEBUG(m_text, "Recycling response_info (" << p_response_info << ") given an xtsc_response: " << *p_response);
    m_response_pool.pop_back();
    p_response_info->m_p_response    = p_response;
    p_response_info->m_bus_addr_bits = bus_addr_bits;
    p_response_info->m_size          = size;
    p_response_info->m_is_read       = is_read;
    p_response_info->m_id            = id;
    p_response_info->m_route_id      = route_id;
    p_response_info->m_num_transfers = num_transfers;
    p_response_info->m_burst         = burst;
    return p_response_info;
  }
}



xtsc_component::xtsc_tlm2pin_memory_transactor::
response_info *xtsc_component::xtsc_tlm2pin_memory_transactor::new_response_info(const response_info& info) {
  if (m_response_pool.empty()) {
    response_info *p_response_info = new response_info(info);
    XTSC_DEBUG(m_text, "Created new response_info (" << p_response_info << ") from another response_info");
    return p_response_info;
  }
  else {
    response_info *p_response_info = m_response_pool.back();
    XTSC_DEBUG(m_text, "Recycling response_info (" << p_response_info << ") given a response_info");
    m_response_pool.pop_back();
    *p_response_info = info;
    return p_response_info;
  }
}



void xtsc_component::xtsc_tlm2pin_memory_transactor::delete_response_info(response_info*& p_response_info) {
  XTSC_DEBUG(m_text, "Freeing up response_info " << p_response_info);
  assert(p_response_info);
  m_response_pool.push_back(p_response_info);
  p_response_info = 0;
}



xtsc_request *xtsc_component::xtsc_tlm2pin_memory_transactor::new_request(const xtsc_request& request) {
  if (m_request_pool.empty()) {
    XTSC_DEBUG(m_text, "Creating a new xtsc_request");
    return new xtsc_request(request);
  }
  else {
    xtsc_request *p_request = m_request_pool.back();
    m_request_pool.pop_back();
    *p_request = request;
    return p_request;
  }
}



void xtsc_component::xtsc_tlm2pin_memory_transactor::delete_request(xtsc_request*& p_request) {
  m_request_pool.push_back(p_request);
  p_request = 0;
}



void xtsc_component::xtsc_tlm2pin_memory_transactor::swizzle_byte_enables(xtsc_byte_enables& byte_enables) const {
  xtsc_byte_enables swizzled = 0;
  for (u32 i=0; i<m_width8; i++) {
    swizzled <<= 1;
    swizzled |= byte_enables & 1;
    byte_enables >>= 1;
  }
  byte_enables = swizzled;
}



xtsc_component::xtsc_tlm2pin_memory_transactor::xtsc_request_if_impl::xtsc_request_if_impl(const char                          *object_name,
                                                                                           xtsc_tlm2pin_memory_transactor&      transactor,
                                                                                           xtsc::u32                            port_num) :
  sc_object             (object_name),
  m_transactor          (transactor),
  m_p_port              (0),
  m_port_num            (port_num),
  m_lock_port           (port_num),
  m_lock_port_str       ("")
{
  if (m_transactor.m_num_subbanks >= 2) {
    m_lock_port = (m_port_num/m_transactor.m_num_subbanks)*m_transactor.m_num_subbanks;
    ostringstream oss;
    oss << ":" << m_lock_port;
    m_lock_port_str = oss.str();
  }
}



void xtsc_component::xtsc_tlm2pin_memory_transactor::xtsc_request_if_impl::nb_peek(xtsc_address address8, u32 size8, u8 *buffer) {
  m_transactor.peek(m_port_num, address8, size8, buffer);
  if (xtsc_is_text_logging_enabled() && m_transactor.m_text.isEnabledFor(VERBOSE_LOG_LEVEL)) {
    u32 buf_offset = 0;
    ostringstream oss;
    oss << hex << setfill('0');
    for (u32 i = 0; i<size8; ++i) {
      oss << setw(2) << (u32) buffer[buf_offset] << " ";
      buf_offset += 1;
    }
    XTSC_VERBOSE(m_transactor.m_text, "peek: " << " [0x" << hex << address8 << "/" << size8 << "] = " << oss.str());
  }
}



void xtsc_component::xtsc_tlm2pin_memory_transactor::xtsc_request_if_impl::nb_poke(xtsc_address  address8,
                                                                                   u32           size8,
                                                                                   const u8     *buffer)
{
  m_transactor.poke(m_port_num, address8, size8, buffer);
  if (xtsc_is_text_logging_enabled() && m_transactor.m_text.isEnabledFor(VERBOSE_LOG_LEVEL)) {
    u32 buf_offset = 0;
    ostringstream oss;
    oss << hex << setfill('0');
    for (u32 i = 0; i<size8; ++i) {
      oss << setw(2) << (u32) buffer[buf_offset] << " ";
      buf_offset += 1;
    }
    XTSC_VERBOSE(m_transactor.m_text, "poke: " << " [0x" << hex << address8 << "/" << size8 << "] = " << oss.str());
  }
}



bool xtsc_component::xtsc_tlm2pin_memory_transactor::xtsc_request_if_impl::nb_fast_access(xtsc_fast_access_request &request) {
  xtsc_address address8 = request.get_translated_request_address();

  for (u32 i=0; i<m_transactor.m_deny_fast_access.size(); i+=2) {
    xtsc_address begin = m_transactor.m_deny_fast_access[i];
    xtsc_address end   = m_transactor.m_deny_fast_access[i+1];
    if ((address8 >= begin) && (address8 <= end)) {
      // Deny fast access
      request.deny_access();
      xtsc_fast_access_block deny_block(address8, begin, end);
      request.restrict_to_block(deny_block);
      if (xtsc_is_text_logging_enabled() && m_transactor.m_text.isEnabledFor(INFO_LOG_LEVEL)) {
        xtsc_fast_access_block block = request.get_local_block(address8);
        XTSC_INFO(m_transactor.m_text, hex << setfill('0') << "nb_fast_access: addr=0x" << hex << setfill('0') << setw(8) <<
                                   request.get_request_address() << " translated=0x" << setw(8) << address8 << " block=[0x" << setw(8) <<
                                   block.get_block_beg_address() << "-0x" << setw(8) << block.get_block_end_address() << "] deny access");
      }
      return true;
    }
  }
  return m_transactor.fast_access(m_port_num, request);
}



void xtsc_component::xtsc_tlm2pin_memory_transactor::xtsc_request_if_impl::nb_request(const xtsc_request& request) {
  XTSC_INFO(m_transactor.m_text, request << " Port #" << m_port_num);

  if (is_axi_or_idma(m_transactor.m_interface_type) != request.is_axi_protocol()) {
    ostringstream oss;
    oss << m_transactor.kind() << " '" << m_transactor.name() << "': is configured with "
        << xtsc_module_pin_base::get_interface_name(m_transactor.m_interface_type) << " interface but received "
        << (request.is_axi_protocol() ? "AXI" : "non-AXI") << " request: " << request;
    throw xtsc_exception(oss.str());
  }

  if ((m_transactor.m_interface_type == APB) != request.is_apb_protocol()) {
    ostringstream oss;
    oss << m_transactor.kind() << " '" << m_transactor.name() << "': is configured with "
        << xtsc_module_pin_base::get_interface_name(m_transactor.m_interface_type) << " interface but received "
        << (request.is_apb_protocol() ? "APB" : "non-APB") << " request: " << request;
    throw xtsc_exception(oss.str());
  }

  if ((m_transactor.m_interface_type == APB) && (m_transactor.m_request_fifo[m_port_num].size())) {
    ostringstream oss;
    oss << m_transactor.kind() << " '" << m_transactor.name() << "' received 2nd APB request on Port #" << m_port_num
        << " while 1st request is still outstanding:" << endl;
    oss << " 1st request: " << *m_transactor.m_request_fifo[m_port_num].front() << endl;
    oss << " 2nd request: " << request;
    throw xtsc_exception(oss.str());
  }

  // Can we accept the request at this time?
  if (m_transactor.m_hard_busy[m_port_num] || (m_transactor.m_request_fifo[m_port_num].size() >= m_transactor.m_request_fifo_depth)) {
    // No. We're full.  Create an RSP_NACC|NOTRDY response.
    xtsc_response response(request, xtsc_response::RSP_NACC, true);
    // Log the response
    XTSC_INFO(m_transactor.m_text, response << " Port #" << m_port_num);
    // Send the response
    (*m_transactor.m_respond_ports[m_port_num])->nb_respond(response);
  }
  else {
    // Create and queue our copy of the request
    m_transactor.m_request_fifo[m_port_num].push_back(m_transactor.new_request(request));
    // Notify the appropriate request thread
    m_transactor.m_request_event[m_port_num].notify(m_transactor.m_output_delay);
  }
}



void xtsc_component::xtsc_tlm2pin_memory_transactor::xtsc_request_if_impl::nb_load_retired(xtsc_address address8) {
  if (m_transactor.m_interface_type != m_transactor.XLMI0) {
    ostringstream oss;
    oss << "'" << m_transactor.name() << "': nb_load_retired() called for interface type '"
        << xtsc_module_pin_base::get_interface_name(m_transactor.m_interface_type) << "' (only allowed for XLMI0)";
    throw xtsc_exception(oss.str());
  }
  XTSC_VERBOSE(m_transactor.m_text, "nb_load_retired(0x" << setfill('0') << hex << setw(8) << address8 <<
                                     ") Port #" << m_port_num);
  m_transactor.m_retire_event[m_port_num].notify(m_transactor.m_output_delay);
  m_transactor.m_retire_deassert[m_port_num] = sc_time_stamp() + m_transactor.m_output_delay + m_transactor.m_clock_period;
}



void xtsc_component::xtsc_tlm2pin_memory_transactor::xtsc_request_if_impl::nb_retire_flush() {
  if (m_transactor.m_interface_type != m_transactor.XLMI0) {
    ostringstream oss;
    oss << "'" << m_transactor.name() << "': nb_retire_flush() called for interface type '"
        << xtsc_module_pin_base::get_interface_name(m_transactor.m_interface_type) << "' (only allowed for XLMI0)";
    throw xtsc_exception(oss.str());
  }
  XTSC_VERBOSE(m_transactor.m_text, "nb_retire_flush() Port #" << m_port_num);
  m_transactor.m_flush_event[m_port_num].notify(m_transactor.m_output_delay);
  m_transactor.m_flush_deassert[m_port_num] = sc_time_stamp() + m_transactor.m_output_delay + m_transactor.m_clock_period;
}



void xtsc_component::xtsc_tlm2pin_memory_transactor::xtsc_request_if_impl::nb_lock(bool lock) {
  if ((m_transactor.m_interface_type == m_transactor.DRAM0RW) || (m_transactor.m_interface_type == m_transactor.DRAM1RW)) {
    if (m_port_num & 0x1) {
      ostringstream oss;
      oss << "'" << m_transactor.name() << "': nb_lock() called for interface type '"
          << xtsc_module_pin_base::get_interface_name(m_transactor.m_interface_type) << "' Port #" << m_port_num 
          << ", but nb_lock() is only allowed on Rd (even-numbered) ports of DRAM0RW|DRAM1RW.";
      throw xtsc_exception(oss.str());
    }
  }
  else if ((m_transactor.m_interface_type != m_transactor.DRAM0  ) && (m_transactor.m_interface_type != m_transactor.DRAM1  ) &&
           (m_transactor.m_interface_type != m_transactor.DRAM0BS) && (m_transactor.m_interface_type != m_transactor.DRAM1BS))
  {
    ostringstream oss;
    oss << "'" << m_transactor.name() << "': nb_lock() called for interface type '"
        << xtsc_module_pin_base::get_interface_name(m_transactor.m_interface_type)
        << "' (only allowed for DRAM0|DRAM0BS|DRAM0RW|DRAM1|DRAM1BS|DRAM1RW)";
    throw xtsc_exception(oss.str());
  }
  if (!m_transactor.m_has_lock) {
    ostringstream oss;
    oss << "'" << m_transactor.name() << "': nb_lock() called but \"has_lock\" is false";
    throw xtsc_exception(oss.str());
  }
  XTSC_VERBOSE(m_transactor.m_text, "nb_lock(" << boolalpha << lock << ") Port #" << m_port_num << m_lock_port_str);
  m_transactor.m_lock_deque[m_lock_port].push_back(lock);
  m_transactor.m_lock_event_queue[m_lock_port].notify(m_transactor.m_output_delay);
}



void xtsc_component::xtsc_tlm2pin_memory_transactor::xtsc_request_if_impl::register_port(sc_port_base& port, const char *if_typename) {
  if (m_p_port) {
    ostringstream oss;
    oss << "Illegal multiple binding detected to xtsc_tlm2pin_memory_transactor '" << m_transactor.name() << "' m_request_exports["
        << m_port_num << "]: " << endl;
    oss << "  " << port.name() << endl;
    oss << "  " << m_p_port->name();
    throw xtsc_exception(oss.str());
  }
  XTSC_INFO(m_transactor.m_text, "Binding '" << port.name() << "' to '" << m_transactor.name() << ".m_request_exports[" <<
                                 m_port_num << "]'");
  m_p_port = &port;
}



void xtsc_component::xtsc_tlm2pin_memory_transactor::xtsc_debug_if_cap::nb_peek(xtsc_address address8, u32 size8, u8 *buffer) {
  if (m_transactor.m_peek) {
    m_transactor.m_peek(address8, size8, buffer, m_transactor.m_dso_cookie, m_port_num);
  }
  else {
    if (!m_transactor.m_cosim) {
      ostringstream oss;
      oss << m_transactor.name() << ": nb_peek() method called for capped port";
      if (m_p_port) { oss << ": " << m_p_port->name(); }
      oss << " (Try setting \"cosim\" to true)";
      throw xtsc_exception(oss.str());
    }
    if (m_transactor.m_p_memory) {
      m_transactor.m_p_memory->peek(address8, size8, buffer);
    }
  }
}



void xtsc_component::xtsc_tlm2pin_memory_transactor::xtsc_debug_if_cap::nb_poke(xtsc_address address8, u32 size8, const u8 *buffer) {
  if (m_transactor.m_poke) {
    m_transactor.m_poke(address8, size8, buffer, m_transactor.m_dso_cookie, m_port_num);
  }
  else {
    if (!m_transactor.m_cosim) {
      ostringstream oss;
      oss << m_transactor.name() << ": nb_poke() method called for capped port";
      if (m_p_port) { oss << ": " << m_p_port->name(); }
      oss << " (Try setting \"cosim\" to true)";
      throw xtsc_exception(oss.str());
    }
    if (m_transactor.m_p_memory) {
      m_transactor.m_p_memory->poke(address8, size8, buffer);
    }
  }
}



bool xtsc_component::xtsc_tlm2pin_memory_transactor::xtsc_debug_if_cap::nb_fast_access(xtsc_fast_access_request &request) {
  if (m_transactor.m_peek) {
    request.allow_peek_poke_access();
    return true;
  }
  if (!m_transactor.m_cosim) {
    ostringstream oss;
    oss << m_transactor.name() << ": nb_fast_access() method called for capped port";
    if (m_p_port) { oss << ": " << m_p_port->name(); }
    oss << " (Try setting \"cosim\" to true)";
    throw xtsc_exception(oss.str());
  }
  return false;
}



void xtsc_component::xtsc_tlm2pin_memory_transactor::xtsc_debug_if_cap::register_port(sc_port_base& port, const char *if_typename) {
  m_p_port = &port;
}



