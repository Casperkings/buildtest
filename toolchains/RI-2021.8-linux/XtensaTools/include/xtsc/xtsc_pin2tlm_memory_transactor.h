#ifndef _XTSC_PIN2TLM_MEMORY_TRANSACTOR_H_
#define _XTSC_PIN2TLM_MEMORY_TRANSACTOR_H_

// Copyright (c) 2007-2017 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of
// Cadence Design Systems, Inc.  They may be adapted and modified by bona fide
// purchasers for internal use, but neither the original nor any adapted
// or modified version may be disclosed or distributed to third parties
// in any manner, medium, or form, in whole or in part, without the prior
// written consent of Cadence Design Systems, Inc.

/**
 * @file 
 */


#include <deque>
#include <vector>
#include <string>
#include <xtsc/xtsc_parms.h>
#include <xtsc/xtsc_request.h>
#include <xtsc/xtsc_request_if.h>
#include <xtsc/xtsc_respond_if.h>
#include <xtsc/xtsc_module_pin_base.h>




namespace xtsc {
// Forward references
class xtsc_core;
};



namespace xtsc_component {


// Forward references
class xtsc_tlm2pin_memory_transactor;


/**
 * Constructor parameters for a xtsc_pin2tlm_memory_transactor transactor object.
 *
 *  \verbatim
   Name                 Type    Description
   ------------------   ----    --------------------------------------------------------

   "memory_interface"   char*   The memory interface type.  Valid values are "DRAM0",
                                "DRAM0BS", "DRAM0RW", "DRAM1", "DRAM1BS", "DRAM1RW",
                                "DROM0", "IRAM0", "IRAM1", "IROM0", "URAM0", "XLMI0",
                                "PIF", "IDMA0", "AXI", "IDMA", and "APB"
                                (case-insensitive).
                                Note: For inbound PIF, set this parameter to "PIF" and
                                set the "inbound_pif" parameter to true.
                                Note: For snoop port, set this parameter to "PIF" and
                                set the "snoop" parameter to true (future use).
                                Note: "AXI" is used to indicate the AXI4 system memory
                                interfaces of Xtensa for Load, Store, and iFetch and
                                does not include NX iDMA.  "PIF" is used to indicate
                                the system memory interfaces of Xtensa for Load, Store,
                                and iFetch and does not include LX iDMA.  "IDMA0"
                                indicates LX PIF-based iDMA and "IDMA" indicates NX
                                AXI4-based iDMA.
    
   "num_ports"          u32     The number of memory ports this transactor has.  Here
                                "port" means an Xtensa TLM request-response port pair.
                                A value of 1 means this transactor is single-ported, a
                                value of 2 means this transactor is dual-ported, etc.  
                                If "memory_interface" is "DRAM0RW" or "DRAM1RW", then a
                                read port counts as one port and its corresponding write
                                port counts as another port.  If "memory_interface" is
                                "AXI" or "IDMA", then reads and writes must go out on
                                separate ports (separate Xtensa TLM port pairs) and a
                                port supporting reads is referred to below as a read
                                port and a port supporting writes is referred to as a
                                write port.
                                Default = 1.
                                Minimum = 1.
         Note: If you want AXI4 read and write requests to go out on the same Xtensa TLM
               path, then a downstream xtsc_component::xtsc_arbiter can be used to
               combine the reads and writes.

   "axi_id_bits"    vector<u32> A vector containing "num_ports" number of entries.
                                Each entry indicates the number of low-order bits from
                                the AXI4 transaction ID (AxID|xID) to use as low-order
                                bits of the request ID.  This parameter must be
                                specified if "memory_interface" is "AXI"/"IDMA" and is
                                ignored otherwise.  Valid entry value range is 0-8.
                                If "num_ports" is greater than 1 but only one entry is
                                provided for this parameter, then that entry will be
                                duplicated to provide "num_ports" entries.
                                Default = <empty>

   "axi_route_id_bits" vector<u32> A vector containing "num_ports" number of entries.
                                Each entry indicates the number of high-order bits of
                                the AXI4 transaction ID (AxID|xID) to use as low-order
                                bits in the request's route ID.  This parameter is
                                ignored if "memory_interface" is other than "AXI"/"IDMA"
                                If "memory_interface" is "AXI"/"IDMA" and this parameter
                                is empty, then a value of 0 will be used for all ports
                                (no route ID bits will be used in the transaction ID).
                                If "num_ports" is greater than 1 but only one entry is
                                provided for this parameter, then that entry will be
                                duplicated to provide "num_ports" entries.
                                Default = <empty>
         Note: The total transaction ID bit width determined from "axi_route_id_bits"
               and "axi_id_bits" must be in the range of 1-32.

   "axi_read_write" vector<u32> A vector containing "num_ports" number of entries.  An
                                entry value of 0 indicates a read port and an entry
                                value of 1 indicates a write port.  This parameter must
                                be specified if "memory_interface" is "AXI"/"IDMA" and
                                is ignored otherwise.
                                If "num_ports" is greater than 1 but only one entry is
                                provided for this parameter, then that entry will be
                                duplicated to provide "num_ports" entries.
                                Default = <empty>
         Note: A read port is associated with 2 AXI4 channels (Read Address channel and
               Read Data channel) and a write port is associated with 3 AXI4 channels
               (Write Address channel, Write Data channel, and Write Response channel).
               The I/O pins of both AXI4 channels comprising a read port are grouped
               together in their own I/O pin set in xtsc_component::xtsc_module_pin_base.
               Similarly, the I/O pins of all 3 AXI4 channels comprising a write port
               are grouped together in their own I/O pin set.

   "axi_port_type" vector<u32>  A vector containing "num_ports" number of entries.  An
                                entry value of 1 indicates the port is a plain AXI4
                                port, an entry value of 2 indicates an ACE-Lite port,
                                and an entry value of 3 indicates a full ACE port.  This
                                parameter must be specified if "memory_interface" is
                                "AXI"/"IDMA" and is ignored otherwise.
                                If "num_ports" is greater than 1 but only one entry is
                                provided for this parameter, then that entry will be
                                duplicated to provide "num_ports" entries.
                                Default = <empty>

   "axi_name_prefix"    char*   A comma-separated list of prefixes to use when naming
                                AXI4/APB pin-level ports.  This parameter must be
                                specified and have "num_ports" number of comma-separated
                                prefixes if "memory_interface" is "AXI", "IDMA", or
                                "APB" and is ignored otherwise.
                                Default = "".

   "port_name_suffix"   char*   Optional constant suffix to be appended to every input
                                and output port name.
                                Default = "".

   "byte_width"         u32     Memory data interface width in bytes.  Valid values for
                                "AXI", "IDMA", "DRAM0", "DRAM0RW", "DRAM1", "DRAM1RW",
                                "DROM0", "URAM0", and "XLMI0" are 4, 8, 16, 32, and 64.
                                Valid values for "DRAM0BS", "DRAM1BS", and "PIF" are 4,
                                8, 16,and 32.  Valid values for "IRAM0", "IRAM1",
                                "IROM0", and "IDMA0" are 4, 8, and 16.  The only valid
                                value for "APB" is 4.

   "start_byte_address" u32     The number to be added to the pin-level address to form
                                the TLM request address.  For non-XLMI local memories,
                                this corresponds to the starting byte address of the
                                memory in the 4GB address space.  For PIF, IDMA0, AXI,
                                IDMA, APB, and XLMI this should be 0x00000000.
                                Default = 0x00000000.
  
   "big_endian"         bool    True if the memory interface master is big endian.
                                Not used if "memory_interface" is "AXI"/"IDMA".
                                Default = false.

   "clock_period"       u32     This is the length of this device's clock period
                                expressed in terms of the SystemC time resolution
                                (from sc_get_time_resolution()).  A value of 
                                0xFFFFFFFF means to use the XTSC system clock 
                                period (from xtsc_get_system_clock_period()).  A value
                                of 0 means one delta cycle.
                                Default = 0xFFFFFFFF (i.e. use the system clock period).

   "posedge_offset"     u32     This specifies the time at which the first posedge of
                                this device's clock conceptually occurs.  It is
                                expressed in units of the SystemC time resolution and
                                the value implied by it must be strictly less than the
                                value implied by the "clock_period" parameter.  A value
                                of 0xFFFFFFFF means to use the same posedge offset as
                                the system clock (from
                                xtsc_get_system_clock_posedge_offset()).
                                Default = 0xFFFFFFFF.

   "sample_phase"       u32     This specifies the phase (i.e. the point) in a clock
                                period at which input pins are sampled.  Output pins
                                which are used for handshaking (PIReqRdy, PIRespValid,
                                ARREADY, AWREADY, WREADY, RVALID, BVALID, IRamBusy,
                                DRamBusy, PREADY, etc.) are also sampled at this time.
                                This value is expressed in terms of the SystemC time
                                resolution (from sc_get_time_resolution()) and must be
                                strictly less than the clock period as specified by the
                                "clock_period" parameter.  A value of 0 means pins are
                                sampled on posedge clock as specified by the 
                                "posedge_offset" parameter.
                                Default = 0 (sample at posedge clock).

   "output_delay"       u32     This specifies how long to delay after the nb_respond()
                                call before starting to drive the output pins.  The
                                output pins will remain driven for one clock period
                                (see the "clock_period" parameter).  This value is
                                expressed in terms of the SystemC time resolution (from
                                sc_get_time_resolution()) and must be strictly less than
                                the clock period.  A value of 0 means one delta cycle.
                                Default = 1 (i.e. 1 time resolution).

   "vcd_handle"         void*   Pointer to SystemC VCD object (sc_trace_file *) or 0 if
                                tracing is not desired.


   Parameters which apply to PIF|IDMA0 only (Note The snoop port is reserved for future use):

   "inbound_pif"        bool    Set to true for inbound PIF.  Set to false for outbound
                                PIF.  This parameter is ignored if "memory_interface"
                                is other then "PIF".
                                Default = false (outbound PIF or snoop).

   "snoop"              bool    Set to true for snoop port.  Set to false for outbound
                                or inbound PIF.  This parameter is ignored if
                                "memory_interface" is other then "PIF".
                                Default = false (outbound or inbound PIF).

   "has_coherence"      bool    True if the "POReqCohCntl", "POReqCohVAdrsIndex", and
                                "PIRespCohCntl" ports should be present.  This parameter
                                is ignored unless "memory_interface" is "PIF" and
                                "inbound_pif" and "snoop" are both false.
                                Default = false.

   "has_pif_attribute"  bool    True if the "POReqAttribute" or "PIReqAttribute" port
                                should be present.  This parameter is ignored unless
                                "memory_interface" is "PIF" or "IDMA0" and "snoop" is
                                false.
                                Default = false.

   "has_pif_req_domain" bool    True if the "POReqDomain" port should be present.  This
                                parameter is ignored unless "memory_interface" is "PIF"
                                or "IDMA0" and both "inbound_pif" and "snoop" are false.
                                Default = false.

   "has_request_id"     bool    True if the "POReqId" and "PIRespId" ports should be
                                present.  This parameter is ignored unless
                                "memory_interface" is "PIF" or "IDMA0".

   "write_responses"    bool    True if TLM write responses should be dropped on the
                                floor by the xtsc_pin2tlm_memory_transactor.  False if
                                write responses should be passed along to the downstream
                                pin-level slave.  This parameter is ignored unless
                                "memory_interface" is "PIF" or "IDMA0".
                                Default = false.
                                Note for xtsc-run users:  When the --cosim command is
                                present, xtsc-run will initialize "write_responses" with
                                a value appropriate for the current config.

   "route_id_bits"      u32     Number of bits in the route ID.  Valid values are 0-32.
                                If "route_id_bits" is 0, then the "POReqRouteId" and
                                "PIRespRouteId" ports ("PIReqRouteId"/"PORespRouteId"
                                when "inbound_pif" is true) will not be present.  This
                                parameter is ignored unless "memory_interface" is "PIF"
                                or "IDMA0".
                                Default = 0.

   "axi_exclusive"      bool    True if ReqCntl and RespCntl should have AXI4 exclusive
                                encodings.  This parameter is ignored unless
                                "memory_interface" is "PIF" or "IDMA0".
                                Default = false.

   "axi_exclusive_id"   bool    True if the inbound PIF request route ID input, normally
                                called "PIReqRouteId", should be called "AXIExclID", and
                                the "PORespRouteId" output should not be present.  This
                                parameter is ignored unless "memory_interface" is "PIF",
                                "inbound_pif" is true, and "axi_exclusive" is true.
                                Default = false.

   "req_user_data"      char*   If not NULL or empty, this specifies the optional pin-
                                level port that should be used for xtsc_request user
                                data.  The string must give the port name and bit width
                                using the format: PortName,BitWidth
                                Note: The values read from PortName will written to
                                      the low order BitWidth bits of the void*
                                      pointer set by xtsc_request::set_user_data().  
                                      BitWidth may not exceed 32 or 64 depending on
                                      whether you are using a 32 or 64 bit simulator.
                                This parameter is ignored unless "memory_interface" is
                                "PIF" or "IDMA0".
                                Default = "" (xtsc_request user data is ignored).

   "rsp_user_data"      char*   If not NULL or empty, this specifies the optional pin-
                                level port that should be used for xtsc_response user
                                data.  The string must give the port name and bit width
                                using the format: PortName,BitWidth
                                Note: The values driven on PortName will be obtained
                                      from the low order BitWidth bits of the void*
                                      pointer returned by xtsc_response::get_user_data().  
                                      BitWidth may not exceed 32 or 64 depending on
                                      whether you are using a 32 or 64 bit simulator.
                                This parameter is ignored unless "memory_interface" is
                                "PIF" or "IDMA0".
                                Default = "" (xtsc_response user data is ignored).


   Parameters which apply to PIF|IDMA0|AXI|IDMA only:

   "one_at_a_time"      bool    The PIReqRdy signal is always deasserted for one clock
                                period each time the nb_respond() call is RSP_NACC.  In
                                addition, if this parameter is true then PIReqRdy will
                                also be deasserted as soon as a last-transfer request is
                                accepted until the last transfer response is received so
                                that only one complete request is accepted at a time.
                                When this parameter is true, a minimum of one clock
                                period must elapse between when a last-transfer request
                                is sent downstream and when a last-transfer response is
                                received (does not apply to RSP_NACC response).  This
                                parameter is ignored unless "memory_interface" is "PIF".
                                Default = false.

   "prereject_responses" bool   If true, calls to nb_respond() will return false if
                                PORespRdy is low when the call to nb_respond() is made.
                                From the pin-level perspective, a response is not 
                                rejected unless PORespRdy is low at the sample time;
                                however, the XTSC TLM model of the response channel only
                                allows a response to be rejected at the time of the
                                nb_respond() call (by returning false).  The default
                                behavior of this model when PIRespRdy is low at the time
                                of the nb_respond() call is to simply repeat the
                                response the next cycle (if any other responses arrive
                                in the mean time, they are saved and handled in order).
                                This parameter is ignored unless "memory_interface" is
                                "PIF" or "IDMA0".
                                Default = false (don't return false to nb_respond).


   Parameters which apply to local memories only (that is, non-PIF|IDMA0|AXI|IDMA|APB
   memories):

   "memory_byte_size"   u32     The byte size of this memory.  This parameter is
                                ignored unless "memory_interface" is "XLMI0".
  
   "address_bits"       u32     Number of bits in the address.  This parameter is
                                ignored if "memory_interface" is not a local memory.

   "check_bits"         u32     Number of bits in the parity/ecc signals.  This
                                parameter is ignored unless "memory_interface" is
                                "IRAM0", "IRAM1", "DRAM0", "DRAM0BS", "DRAM0RW",
                                "DRAM1", "DRAM1BS", or "DRAM1RW".
                                Default = 0.
  
   "has_busy"           bool    True if the memory interface has a busy pin.  This
                                parameter is ignored if "memory_interface" is not a
                                local memory.
                                Default = true.
  
   "has_lock"           bool    True if the memory interface has a lock pin.  This
                                parameter is ignored unless "memory_interface" is
                                "DRAM0", "DRAM0BS", "DRAM0RW", "DRAM1", "DRAM1BS", or
                                "DRAM1RW".
                                Default = false.
  
   "has_xfer_en"        bool    True if the memory interface has an xfer enable pin.
                                This parameter is ignored if "memory_interface" is
                                "DROM0", "XLMI0", or not a local memory.
                                Default = false.
  
   "cbox"               bool    True if this memory interface is driven from an Xtensa
                                CBOX.  This parameter is ignored if "memory_interface"
                                is other then "DRAM0"|"DRAM1"|"DROM0".
  
   "banked"             bool    True if this is a banked "DRAM0"|"DRAM1"|"DROM0"
                                interface or if this is a "DRAM0BS"|"DRAM1BS" interface.
                                Default = false.
  
   "has_dma"            bool    True if the memory interface has split Rd/Wr ports
                                ("DRAM0RW"|"DRAM1RW") accessible from inbound PIF or
                                "IDMA0".
                                Default = false.
  
   "num_subbanks"       u32     The number of subbanks that each bank has.

   "max_cycle_entries"  u32     Should be at least 1 greater than the memory latency.  
                                Used only when subbanked.  
                                Default = 4.

   "dram_attribute_width" u32   160 if the "DRamNAttr" and "DRamNWrAttr" ports
                                should be present.  This parameter is ignored unless
                                "memory_interface" is "DRAM0RW" or "DRAM1RW".
                                Default = 0.
    \endverbatim
 *
 * @see xtsc_pin2tlm_memory_transactor
 * @see xtsc::xtsc_parms
 */
class XTSC_COMP_API xtsc_pin2tlm_memory_transactor_parms : public xtsc::xtsc_parms {
public:


  /**
   * Constructor for an xtsc_pin2tlm_memory_transactor_parms transactor object.
   *
   * @param     memory_interface        The memory interface type.  Valid values are
   *                                    "DRAM0", "DRAM0BS", "DRAM0RW", "DRAM1",
   *                                    "DRAM1BS", "DRAM1RW", "DROM0", "IRAM0", "IRAM1",
   *                                    "IROM0", "URAM0", "XLMI0", "PIF", "IDMA0", 
   *                                    "AXI", "IDMA", and "APB" (case-insensitive).
   *
   * @param     byte_width              Memory data interface width in bytes.
   *
   * @param     address_bits            Number of bits in address.  Ignored for "PIF",
   *                                    "IDMA0", "AXI", "IDMA", and "APB".
   *
   * @param     num_ports               The number of memory ports this transactor has.
   */
  xtsc_pin2tlm_memory_transactor_parms(const char      *memory_interface        = "PIF",
                                       xtsc::u32        byte_width              = 4,
                                       xtsc::u32        address_bits            = 32,
                                       xtsc::u32        num_ports               = 1)
  {
    init(memory_interface, byte_width, address_bits, num_ports);
  }


  /**
   * Constructor for an xtsc_pin2tlm_memory_transactor_parms transactor object based
   * upon an xtsc_core object and a named memory interface. 
   *
   * This constructor will determine "clock_period", "byte_width", "big_endian", "cbox",
   * "address_bits", "start_byte_address", "has_busy", "has_lock", "num_subbanks",
   * "banked", "has_dma", "dram_attribute_width", "check_bits", "has_pif_attribute", 
   * "has_pif_req_domain", "axi_id_bits", "axi_read_write", "axi_port_type", and
   * "axi_name_prefix" by querying the core object.  If memory_interface is XLMI0
   * then it will also determine "memory_byte_size".
   *
   * If desired, after the xtsc_pin2tlm_memory_transactor_parms object is constructed,
   * its data members can be changed using the appropriate xtsc_parms::set() method
   * before passing it to the xtsc_pin2tlm_memory_transactor constructor.
   *
   * @param     core                    A reference to the xtsc_core object upon which
   *                                    to base the xtsc_pin2tlm_memory_transactor_parms.
   *
   * @param     memory_interface        The memory interface type.  Valid values are
   *                                    "DRAM0", "DRAM0BS", "DRAM0RW", "DRAM1",
   *                                    "DRAM1BS", "DRAM1RW", "DROM0", "IRAM0", "IRAM1",
   *                                    "IROM0", "URAM0", "XLMI0", "PIF", "IDMA0", 
   *                                    "AXI", "IDMA", and "APB" (case-insensitive).
   *
   * @param     num_ports               The number of ports this transactor has.  If 0,
   *                                    the number of ports will be determined by
   *                                    calling xtsc_core::get_multi_port_count().  If
   *                                    xtsc_core::is_multi_port_zero() returns false or
   *                                    if any of the ports are already connected then
   *                                    num_ports will be inferred to be 1.
   *
   * Note: If memory_interface is "PIF", "IDMA0", "AXI", "IDMA", or "APB", then
   *       "start_byte_address" will be set to 0.
   */
  xtsc_pin2tlm_memory_transactor_parms(const xtsc::xtsc_core& core, const char *memory_interface, xtsc::u32 num_ports = 0);


  // Perform initialization common to both constructors
  void init(const char *memory_interface, xtsc::u32 byte_width, xtsc::u32 address_bits, xtsc::u32 num_ports) {
    std::vector<xtsc::u32> empty_vector_u32;
    add("memory_interface",     memory_interface);
    add("inbound_pif",          false);
    add("snoop",                false);
    add("has_coherence",        false);
    add("has_pif_attribute",    false);
    add("has_pif_req_domain",   false);
    add("dram_attribute_width", 0);
    add("num_ports",            num_ports);
    add("axi_id_bits",          empty_vector_u32);
    add("axi_route_id_bits",    empty_vector_u32);
    add("axi_read_write",       empty_vector_u32);
    add("axi_port_type",        empty_vector_u32);
    add("axi_name_prefix",      "");
    add("port_name_suffix",     "");
    add("byte_width",           byte_width);
    add("start_byte_address",   0x00000000);
    add("memory_byte_size",     0);
    add("clock_period",         0xFFFFFFFF);
    add("posedge_offset",       0xFFFFFFFF);
    add("sample_phase",         0);
    add("output_delay",         1);
    add("big_endian",           false);
    add("has_request_id",       true);
    add("write_responses",      false);
    add("address_bits",         address_bits);
    add("route_id_bits",        0);
    add("req_user_data",        "");
    add("rsp_user_data",        "");
    add("axi_exclusive",        false);
    add("axi_exclusive_id",     false);
    add("one_at_a_time",        false);
    add("prereject_responses",  false);
    add("has_busy",             true);
    add("has_lock",             false);
    add("has_xfer_en",          false);
    add("vcd_handle",           (void*)NULL);
    add("cbox",                 false);
    add("banked",               false);
    add("num_subbanks",         0);
    add("max_cycle_entries",    4);
    add("has_dma",              false);
    add("check_bits",           0);
  }


  /// Our C++ type (the xtsc_parms base class uses this for error messages)
  virtual const char* kind() const { return "xtsc_pin2tlm_memory_transactor_parms"; }

};




/**
 * This device converts memory transactions from pin level to transaction level.
 *
 * This device converts pin-level memory requests to TLM memory requests
 * (xtsc_request_if) and it converts the corresponding TLM responses (xtsc_respond_if)
 * to pin-level responses.
 *
 * When configured for the PIF, IDMA0, AXI, or IDMA, this module introduces some timing
 * artifacts that might not be present in a pure pin-level system.  This is because of
 * the PIF and AXI4 protocols and the way they are modeled in XTSC TLM.  Specifically,
 * this module does not know that there is a pin-level request until it is too late to
 * reject the request.  Also, the only way to reject a TLM response is to return false
 * to the nb_response() call; however, during the nb_response() call (which is
 * non-blocking) this module does not yet know that the upstream pin-level master will
 * eventually reject the response (but see the "prereject_responses" parameter).  To
 * overcome these issues, the PIReqRdy signal (PIF) or the ARREADY|AWREADY|WREADY signal
 * (AXI4) is deasserted for one clock period each time the nb_respond() call is
 * RSP_NACC.  For the case of back-to-back requests, the effect of this is to reject the
 * next request after the request that would have been rejected in a pure pin-level
 * simulation (but see the "one_at_a_time" parameter).  By default, this model does not
 * reject TLM PIF|AXI4 responses which come from the memory interface slave and, if the
 * memory interface master rejects a pin-level response, then this module will simply
 * repeat the response next cycle.
 *
 * When configured for APB or a local memory, these timing artifacts don't exist because,
 * for a request, the busy is not due until the cycle after the request and, for a
 * response, there is no concept of rejecting it.
 *
 * Note: The parity/ECC signals (DRamNCheckDataM, DRamNCheckWrDataM, IRamNCheckData, and
 * IRamNCheckWrData) are present for IRAM and DRAM interfaces when "check_bits" is
 * non-zero; however, the input signal is ignored and the output signal is driven with
 * constant 0.
 *
 * Here is a block diagram of an xtsc_pin2tlm_memory_transactor as it is used in the
 * xtsc_pin2tlm_memory_transactor example:
 * @image html  Example_xtsc_pin2tlm_memory_transactor.jpg
 * @image latex Example_xtsc_pin2tlm_memory_transactor.eps "xtsc_pin2tlm_memory_transactor Example" width=13cm
 *
 * @see xtsc_pin2tlm_memory_transactor_parms
 * @see xtsc::xtsc_core
 * @see xtsc_tlm2pin_memory_transactor
 *
 */
class XTSC_COMP_API xtsc_pin2tlm_memory_transactor : public sc_core::sc_module, public xtsc::xtsc_module, public xtsc_module_pin_base {
protected:
  class request_info;
  class axi_addr_info;
  class axi_data_info;
  class axi_req_info;
  friend class request_info;
  friend class axi_addr_info;
  friend class axi_data_info;
  friend class axi_req_info;
public:

  sc_core::sc_port<xtsc::xtsc_request_if>     **m_request_ports;        /// From us to slave  (per mem port)

  sc_core::sc_export<xtsc::xtsc_respond_if>   **m_respond_exports;      /// From slave to us  (per mem port)

  sc_core::sc_export<xtsc::xtsc_debug_if>     **m_debug_exports;        /// From master to us (per mem port)

  /**
   * @see xtsc_module_pin_base::get_bool_input()
   * @see xtsc_module_pin_base::get_uint_input()
   * @see xtsc_module_pin_base::get_wide_input()
   * @see xtsc_module_pin_base::get_bool_output()
   * @see xtsc_module_pin_base::get_uint_output()
   * @see xtsc_module_pin_base::get_wide_output()
   */
  xtsc::Readme How_to_get_input_and_output_ports;


  // Shorthand aliases
  typedef xtsc::xtsc_request                            xtsc_request;
  typedef xtsc::xtsc_response                           xtsc_response;
  typedef sc_core::sc_fifo<sc_dt::sc_bv_base>           wide_fifo;
  typedef sc_core::sc_fifo<bool>                        bool_fifo;
  typedef sc_core::sc_signal<bool>                      bool_signal;
  typedef sc_core::sc_signal<sc_dt::sc_uint_base>       uint_signal;
  typedef sc_core::sc_signal<sc_dt::sc_bv_base>         wide_signal;
  typedef std::map<std::string, bool_signal*>           map_bool_signal;
  typedef std::map<std::string, uint_signal*>           map_uint_signal;
  typedef std::map<std::string, wide_signal*>           map_wide_signal;
  typedef std::deque<xtsc::xtsc_address>                address_deque;


  /// This SystemC macro inserts some code required for SC_THREAD's to work
  SC_HAS_PROCESS(xtsc_pin2tlm_memory_transactor);


  /// Our C++ type (SystemC uses this)
  virtual const char* kind() const { return "xtsc_pin2tlm_memory_transactor"; }


  /**
   * Constructor for a xtsc_pin2tlm_memory_transactor.
   *
   * @param     module_name             Name of the xtsc_pin2tlm_memory_transactor
   *                                    sc_module.
   * @param     pin2tlm_parms           The remaining parameters for construction.
   *
   * @see xtsc_pin2tlm_memory_transactor_parms
   */
  xtsc_pin2tlm_memory_transactor(sc_core::sc_module_name module_name, const xtsc_pin2tlm_memory_transactor_parms& pin2tlm_parms);


  /// Destructor.
  ~xtsc_pin2tlm_memory_transactor(void);


  /// For xtsc_connection_interface
  virtual xtsc::u32 get_bit_width(const std::string& port_name, xtsc::u32 interface_num = 0) const;


  /// For xtsc_connection_interface
  virtual sc_core::sc_object *get_port(const std::string& port_name);


  /// For xtsc_connection_interface
  virtual xtsc::xtsc_port_table get_port_table(const std::string& port_table_name) const;


  /**
   * Connect this xtsc_pin2tlm_memory_transactor to the inbound PIF or snoop port of an
   * xtsc_core.
   *
   * Depending upon the "snoop" parameter value, this method connects the specified TLM
   * master port pair of this xtsc_pin2tlm_memory_transactor to either the inbound PIF
   * ("snoop" is false) or the snoop ("snoop" is true) slave port pair of the specified
   * xtsc_core.
   *
   * @param     core                    The xtsc_core to connect with this
   *                                    xtsc_pin2tlm_memory_transactor.
   *
   * @param     pin2tlm_port            The master port pair of this
   *                                    xtsc_pin2tlm_memory_transactor to connect with
   *                                    the inbound PIF or snoop interface of core.
   *
   * Note: The snoop port is reserved for future use.
   *
   */
  void connect(xtsc::xtsc_core& core, xtsc::u32 pin2tlm_port = 0);


  /**
   * Connect an xtsc_tlm2pin_memory_transactor transactor to this
   * xtsc_pin2tlm_memory_transactor transactor.
   *
   * This method connects the pin-level ports of an upstream
   * xtsc_tlm2pin_memory_transactor to the pin-level ports of this
   * xtsc_pin2tlm_memory_transactor.  In the process, it creates the necessary signals
   * of type sc_signal<bool>, sc_signal<sc_uint_base>, and sc_signal<sc_bv_base>.  The
   * name of each signal is formed by the concatenation of the SystemC name of the
   * xtsc_pin2tlm_memory_transactor object, the 2 characters "__", and the SystemC name
   * of the xtsc_pin2tlm_memory_transactor port (for example, "pin2tlm__POReqValid").
   *
   * @param     tlm2pin         The xtsc_tlm2pin_memory_transactor to connect to this
   *                            xtsc_pin2tlm_memory_transactor.
   *
   * @param     tlm2pin_port    The tlm2pin port to connect to.
   *
   * @param     pin2tlm_port    The port of this transactor to connect the tlm2pin to.
   *
   * @param     single_connect  If true only one port of this transactor will be connected.
   *                            If false, the default, then all contiguous, unconnected
   *                            port numbers of this transactor starting at pin2tlm_port
   *                            that have a corresponding existing port in tlm2pin
   *                            (starting at tlm2pin_port) will be connected to that
   *                            corresponding port in tlm2pin.
   *
   * NOTE:  This method is just for special testing purposes.  In general, connecting a
   *        xtsc_tlm2pin_memory_transactor to a xtsc_pin2tlm_memory_transactor is not
   *        guarranteed to meet timing requirements.
   *
   * @returns number of ports that were connected by this call (1 or more)
   */
  xtsc::u32 connect(xtsc_tlm2pin_memory_transactor&     tlm2pin,
                    xtsc::u32                           tlm2pin_port = 0,
                    xtsc::u32                           pin2tlm_port = 0,
                    bool                                single_connect = false);


  virtual void reset(bool hard_reset = false);


  /// Return the interface type
  memory_interface_type get_interface_type() const { return m_interface_type; }


  /// Return the interface name string
  const char *get_interface_name() const { return xtsc_module_pin_base::get_interface_name(m_interface_type); }


  /// Return the number of memory ports this transactor has
  xtsc::u32 get_num_ports() const { return m_num_ports; }


  /// Return true if pin port names include the set_id as a suffix
  bool get_append_id() const { return m_append_id; }


protected:


  /// Helper method to get the tlm2pin port name and confirm pin sizes match
  std::string adjust_name_and_check_size(const std::string&                     port_name,
                                         const xtsc_tlm2pin_memory_transactor&  tlm2pin,
                                         xtsc::u32                              tlm2pin_port,
                                         const set_string&                      transactor_set) const;


  /// Dump a set of strings one string per line with the specified indent
  void dump_set_string(std::ostringstream& oss, const set_string& strings, const std::string& indent);


  /// SystemC callback
  virtual void end_of_elaboration(void);


  /// SystemC callback
  virtual void start_of_simulation(void);


  /// Move response buffer data to m_data
  void get_read_data_from_response(const xtsc::xtsc_response& response);


  /// Sync to sample phase (m_sample_phase).  If already at sample phase, sync to next one.
  void sync_to_sample_phase();


  /// Handle local memory-interface requests on non-split Rd/Wr interfaces
  void lcl_request_thread(void);


  /// Handle local memory-interface requests on split Rd/Wr interfaces
  void lcl_split_rw_request_thread(void);


  /// Handle local memory-interface requests
  void lcl_drive_read_data_thread(void);


  /// Handle local memory-interface busy
  void lcl_drive_busy_thread(void);


  /// DPort0LoadRetiredm
  void xlmi_load_retired_thread();


  /// DPort0RetireFlushm
  void xlmi_retire_flush_thread();


  /// DRamnLockm
  void dram_lock_method();


  /// PIF|IDMA0: Capture a pin request from upstream
  void pif_sample_pin_request_thread(void);


  /// PIF|IDMA0: Drive PIReqRdy
  void pif_drive_req_rdy_thread(void);


  /// PIF|IDMA0: Send a TLM request downstream
  void pif_send_tlm_request_thread(void);


  /// PIF|IDMA0: Send a pin response upstream
  void pif_drive_pin_response_thread(void);


  // To sample AXI4 request addr channel signals (AxID, AxADDR, AxLEN, AxSIZE, etc)
  void axi_req_addr_thread(void);


  // To sample AXI4 request data channel signals (WDATA, WSTRB, and WLAST)
  void axi_req_data_thread(void);


  // To match up write address channel and write data channel signals to form a Xtensa TLM request
  void axi_wr_req_thread(void);


  // To drive ARREADY of read memory port or WREADY of write memory port
  void axi_drive_req_rdy_thread(void);


  // To call nb_request for AXI4 requests
  void axi_send_tlm_req_thread(void);


  // To sample xREADY and drive xVALID and xRESP (also RDATA and RLAST if read) to send pin-level response upstream
  void axi_drive_rsp_thread(void);


  /// Handle APB requests
  void apb_request_thread(void);


  /// Drive APB outputs after "output_delay"
  void apb_drive_outputs_thread(void);


  /// Create an sc_signal<bool> with the specified name
  bool_signal& create_bool_signal(const std::string& signal_name);


  /// Create an sc_signal<sc_uint_base> with the specified name and size
  uint_signal& create_uint_signal(const std::string& signal_name, xtsc::u32 num_bits);


  /// Create an sc_signal<sc_bv_base> with the specified name and size
  wide_signal& create_wide_signal(const std::string& signal_name, xtsc::u32 num_bits);


  /// Swizzle byte enables 
  void swizzle_byte_enables(xtsc::xtsc_byte_enables& byte_enables) const;


  /// Implementation of xtsc_respond_if.
  class xtsc_respond_if_impl : public xtsc::xtsc_respond_if, public sc_core::sc_object {
  public:

    /// Constructor
    xtsc_respond_if_impl(const char *object_name, xtsc_pin2tlm_memory_transactor& pin2tlm, xtsc::u32 port_num);

    /// The kind of sc_object we are
    const char* kind() const { return "xtsc_pin2tlm_memory_transactor::xtsc_respond_if_impl"; }

    /// @see xtsc::xtsc_respond_if
    bool nb_respond(const xtsc::xtsc_response& response);

    /// Return true if a port has been bound to this implementation
    bool is_connected() { return (m_p_port != 0); }

  protected:

    /// SystemC callback when something binds to us
    virtual void register_port(sc_core::sc_port_base& port, const char *if_typename);

    xtsc_pin2tlm_memory_transactor&     m_pin2tlm;      ///< Our xtsc_pin2tlm_memory_transactor object
    sc_core::sc_port_base              *m_p_port;       ///< Port that is bound to us
    xtsc::u32                           m_port_num;     ///< Our port number
    xtsc::u32                           m_busy_port;    ///< For DRAM0BS|DRAM1BS, port num of the 0th subbank, otherwise same as m_port_num
    xtsc::u32                           m_bank;         ///< Which bank
  };



  /// Implementation of xtsc_debug_if.
  class xtsc_debug_if_impl : public xtsc::xtsc_debug_if, public sc_core::sc_object {
  public:

    /// Constructor
    xtsc_debug_if_impl(const char *object_name, xtsc_pin2tlm_memory_transactor& pin2tlm, xtsc::u32 port_num) :
      sc_object         (object_name),
      m_pin2tlm         (pin2tlm),
      m_p_port          (0),
      m_port_num        (port_num)
    {}

    /// The kind of sc_object we are
    const char* kind() const { return "xtsc_pin2tlm_memory_transactor::xtsc_debug_if_impl"; }

    /**
     *  Receive peeks from the memory interface master
     *  @see xtsc::xtsc_debug_if
     */
    virtual void nb_peek(xtsc::xtsc_address address8, xtsc::u32 size8, xtsc::u8 *buffer);

    /**
     *  Receive pokes from the memory interface master
     *  @see xtsc::xtsc_debug_if
     */
    virtual void nb_poke(xtsc::xtsc_address address8, xtsc::u32 size8, const xtsc::u8 *buffer);

    /**
     *  Receive coherent peeks from the memory interface master
     *  @see xtsc::xtsc_debug_if
     */
    virtual bool nb_peek_coherent(xtsc::xtsc_address    virtual_address8,
                                  xtsc::xtsc_address    physical_address8,
                                  xtsc::u32             size8,
                                  xtsc::u8             *buffer);

    /**
     *  Receive coherent pokes from the memory interface master
     *  @see xtsc::xtsc_debug_if
     */
    virtual bool nb_poke_coherent(xtsc::xtsc_address    virtual_address8,
                                  xtsc::xtsc_address    physical_address8,
                                  xtsc::u32             size8,
                                  const xtsc::u8       *buffer);

    /**
     *  Receive requests for fast access information from the memory interface master
     *  @see xtsc::xtsc_debug_if
     */
    virtual bool nb_fast_access(xtsc::xtsc_fast_access_request &request);

    /// Return true if a port has been bound to this implementation
    bool is_connected() { return (m_p_port != 0); }


  protected:

    /// SystemC callback when something binds to us
    void register_port(sc_core::sc_port_base& port, const char *if_typename);

    xtsc_pin2tlm_memory_transactor&     m_pin2tlm;      ///< Our xtsc_pin2tlm_memory_transactor object
    sc_core::sc_port_base              *m_p_port;       ///< Port that is bound to us
    xtsc::u32                           m_port_num;     ///< Our port number
  };



  /**
   * Information about each non-AXI4/APB request.
   * Constructor and init() populate data members by reading the input pin values.
   */
  class request_info {
  public:

    /// Constructor for a new request_info
    request_info(const xtsc_pin2tlm_memory_transactor& pin2tlm, xtsc::u32 port);

    /// Initialize an already existing request_info object
    void init(xtsc::u32 port);

    /// Dump a request_info object
    void dump(std::ostream& os) const;

    const xtsc_pin2tlm_memory_transactor&       m_pin2tlm;              ///< A reference to the owning xtsc_pin2tlm_memory_transactor
    req_cntl                                    m_req_cntl;             ///< POReqCntl/SnoopReqCntl
    xtsc::xtsc_address                          m_address;              ///< POReqAdrs
    sc_dt::sc_bv_base                           m_data;                 ///< POReqData
    xtsc::xtsc_byte_enables                     m_byte_enables;         ///< POReqDataBE
    sc_dt::sc_uint_base                         m_id;                   ///< POReqId
    sc_dt::sc_uint_base                         m_priority;             ///< POReqPriority
    sc_dt::sc_uint_base                         m_route_id;             ///< POReqRouteId
    sc_dt::sc_uint_base                         m_req_attribute;        ///< POReqAttribute
    sc_dt::sc_uint_base                         m_req_domain;           ///< POReqDomain
    sc_dt::sc_uint_base                         m_vadrs;                ///< POReqCohVAdrsIndex/SnoopReqCohVAdrsIndex
    sc_dt::sc_uint_base                         m_coherence;            ///< POReqCohCntl/SnoopReqCohCntl
    xtsc::xtsc_byte_enables                     m_fixed_byte_enables;   ///< POReqDataBE swizzled if m_big_endian
    xtsc::xtsc_address                          m_fixed_address;        ///< POReqAdrs fixed for xtsc_request
    xtsc::xtsc_request                          m_request;              ///< The TLM request
    xtsc::u64                                   m_cycle_num;            ///< Only if subbanked: Cycle number of request
    xtsc::u32                                   m_cycle_index;          ///< Only if subbanked: Cycle index of request
  };
  friend std::ostream& operator<<(std::ostream& os, const request_info& info);


  /// Get a new request_info (from the pool)
  request_info *new_request_info(xtsc::u32 port);


  /// Delete an request_info (return it to the pool)
  void delete_request_info(request_info*& p_request_info);


  /**
   * Information about each AXI4 address request.
   * Constructor and init() populate data members by reading the input pin values.
   */
  class axi_addr_info {
  public:

    /// Constructor for a new axi_addr_info
    axi_addr_info(const xtsc_pin2tlm_memory_transactor& pin2tlm, xtsc::u32 port);

    /// Initialize an already existing axi_addr_info object
    void init(xtsc::u32 port);

    /// Dump a axi_addr_info object
    void dump(std::ostream& os) const;

    const xtsc_pin2tlm_memory_transactor& m_pin2tlm;            ///< A reference to the owning xtsc_pin2tlm_memory_transactor
    xtsc::u32                             m_port;               ///< The memory port this request was received on
    xtsc::u32                             m_indx;               ///< Signal indx for this memory port
    bool                                  m_read_channel;       ///< True if this is a read channel
    sc_core::sc_time                      m_time_stamp;         ///< Time when request was received
    xtsc::u32                             m_AxID;               ///< ARID     | AWID
    xtsc::xtsc_address                    m_AxADDR;             ///< ARADDR   | AWADDR
    xtsc::u32                             m_AxLEN;              ///< ARLEN    | AWLEN
    xtsc::u32                             m_AxSIZE;             ///< ARSIZE   | AWSIZE
    xtsc::u32                             m_AxBURST;            ///< ARBURST  | AWBURST
    bool                                  m_AxLOCK;             ///< ARLOCK   | AWLOCK
    xtsc::u32                             m_AxCACHE;            ///< ARCACHE  | AWCACHE
    xtsc::u32                             m_AxPROT;             ///< ARPROT   | AWPROT
    xtsc::u32                             m_AxQOS;              ///< ARQOS    | AWQOS
    xtsc::u32                             m_AxBAR;              ///< ARBAR    | AWBAR
    xtsc::u32                             m_AxDOMAIN;           ///< ARDOMAIN | AWDOMAIN
    xtsc::u32                             m_AxSNOOP;            ///< ARSNOOP  | AWSNOOP
  };
  friend std::ostream& operator<<(std::ostream& os, const axi_addr_info& info);


  /// Get a new axi_addr_info (from the pool)
  axi_addr_info *new_axi_addr_info(xtsc::u32 port);


  /// Delete an axi_addr_info (return it to the pool)
  void delete_axi_addr_info(axi_addr_info*& p_axi_addr_info);


  /**
   * Information about each AXI4 data request.
   * Constructor and init() populate data members by reading the input pin values.
   */
  class axi_data_info {
  public:

    /// Constructor for a new axi_data_info
    axi_data_info(const xtsc_pin2tlm_memory_transactor& pin2tlm, xtsc::u32 port);

    /// Initialize an already existing axi_data_info object
    void init(xtsc::u32 port);

    /// Dump a axi_data_info object
    void dump(std::ostream& os) const;

    const xtsc_pin2tlm_memory_transactor& m_pin2tlm;            ///< A reference to the owning xtsc_pin2tlm_memory_transactor
    xtsc::u32                             m_port;               ///< The memory port this request was received on
    xtsc::u32                             m_indx;               ///< Signal indx for this memory port
    sc_core::sc_time                      m_time_stamp;         ///< Time when request was received
    sc_dt::sc_bv_base                     m_WDATA;              ///< WDATA
    xtsc::xtsc_byte_enables               m_WSTRB;              ///< WSTRB
    bool                                  m_WLAST;              ///< WLAST
  };
  friend std::ostream& operator<<(std::ostream& os, const axi_data_info& info);


  /// Get a new axi_data_info (from the pool)
  axi_data_info *new_axi_data_info(xtsc::u32 port);


  /// Delete an axi_data_info (return it to the pool)
  void delete_axi_data_info(axi_data_info*& p_axi_data_info);


  /**
   * Class to hold the Xtensa TLM request (xtsc_request) and pointers to the pin-level
   * request values (axi_addr_info and axi_data_info) of each request.
   */
  class axi_req_info {
  public:

    /// Constructor for a new axi_req_info
    axi_req_info(const xtsc_pin2tlm_memory_transactor& pin2tlm, xtsc::u32 port, axi_addr_info *p_addr_info, axi_data_info *p_data_info);

    /// Initialize an already existing axi_req_info object
    void init(xtsc::u32 port, axi_addr_info *p_addr_info, axi_data_info *p_data_info);

    /// Dump a axi_req_info object
    void dump(std::ostream& os) const;

    const xtsc_pin2tlm_memory_transactor& m_pin2tlm;            ///< A reference to the owning xtsc_pin2tlm_memory_transactor
    xtsc::u32                             m_port;               ///< The memory port this request was received on
    axi_addr_info                        *m_p_addr_info;        ///< Pointer to the axi_addr_info for this request
    axi_data_info                        *m_p_data_info;        ///< Pointer to the axi_data_info for this request (NULL for read path)
    xtsc::xtsc_request                    m_request;            ///< The TLM request
  };
  friend std::ostream& operator<<(std::ostream& os, const axi_req_info& info);


  /// Get a new axi_req_info (from the pool)
  axi_req_info *new_axi_req_info(xtsc::u32 port, axi_addr_info *p_addr_info, axi_data_info *p_data_info);


  /**
   * Delete a axi_req_info and its associated axi_addr_info and axi_data_info (return
   * them to their respective pools).  
   */
  void delete_axi_req_info(axi_req_info*& p_axi_req_info);


  /**
   * Information about each APB request.
   * The init() method populates data members by reading the input pin values.
   */
  class apb_info {
  public:

    /// Constructor for a new apb_info
    apb_info(const xtsc_pin2tlm_memory_transactor& pin2tlm, xtsc::u32 port);

    /// Initialize the apb_info object
    void init();

    /// Dump the apb_info object
    void dump(std::ostream& os) const;

    const xtsc_pin2tlm_memory_transactor&       m_pin2tlm;              ///< A reference to the owning xtsc_pin2tlm_memory_transactor
    xtsc::u32                                   m_port;                 ///< Which port
    xtsc::xtsc_address                          m_address;              ///< PADDR
    xtsc::u32                                   m_prot;                 ///< PPROT
    xtsc::u32                                   m_data;                 ///< PWDATA|PRDATA
    xtsc::u32                                   m_strb;                 ///< PSTRB
    bool                                        m_write;                ///< PWRITE
    bool                                        m_slverr;               ///< PSLVERR
    xtsc::xtsc_request                          m_request;              ///< The TLM request
  };
  friend std::ostream& operator<<(std::ostream& os, const apb_info& info);


  /// Get a new xtsc_response (from the pool)
  xtsc_response *new_response(const xtsc_response& response);


  /// Delete an xtsc_response (return it to the pool)
  void delete_response(xtsc_response*& p_response);



  /**
   * Keep track of subbank activity to a given given bank to ensure all responses are
   * consistent (all RSP_OK or all RSP_NACC).  Only used for DRAM0BS|DRAM1BS and then
   * only if "has_busy" is true.
   */
  class subbank_activity {
  public:
    xtsc::u64                   m_cycle_num;            ///< Cycle number of requests
    xtsc::u32                   m_num_req;              ///< Number of subbank requests to this bank this cycle
    xtsc::u32                   m_num_rsp_ok;           ///< Number of RSP_OK responses received
    xtsc::u32                   m_num_rsp_nacc;         ///< Number of RSP_NACC responses received
  };


  /// Initialize a subbank_activity to all zeroes
  void initialize_subbank_activity(subbank_activity *p_subbank_activity);



  xtsc_respond_if_impl        **m_respond_impl;                 ///< m_respond_exports binds to this                                    (per mem port)
  xtsc_debug_if_impl          **m_debug_impl;                   ///< m_debug_exports binds to this                                      (per mem port)
  xtsc::u32                     m_num_ports;                    ///< The number of memory ports this transactor has
  xtsc::u32                     m_num_axi_rd_ports;             ///< Number of AXI4 read  memory ports
  xtsc::u32                     m_num_axi_wr_ports;             ///< Number of AXI4 write memory ports
  std::vector<xtsc::u32>        m_axi_signal_indx;              ///< Nth entry gives indx into AXI4 signal arrays (m_p_arid, etc) for mem port #N
  std::deque<request_info*>    *m_request_fifo;                 ///< The fifo of incoming requests (non-AXI4)                            (per mem port)
  std::deque<axi_req_info*>    *m_axi_req_fifo;                 ///< The fifo of incoming requests (AXI4)                                (per mem port)
  std::deque<xtsc_response*>   *m_response_fifo;                ///< The fifo of incoming PIF|IDMA0|AXI|IDMA responses                   (per mem port)
  std::string                   m_interface_uc;                 ///< Uppercase version of "memory_interface" parameter
  std::string                   m_interface_lc;                 ///< Lowercase version of "memory_interface" parameter
  memory_interface_type         m_interface_type;               ///< The memory interface type
  xtsc::u32                     m_size8;                        ///< Byte size of the attached XLMI0 from "memory_byte_size" parameter
  xtsc::u32                     m_width8;                       ///< Data width in bytes of the memory interface
  xtsc::u32                     m_dram_attribute_width;         ///< See "dram_attribute_width" parameter
  mutable sc_dt::sc_unsigned    m_dram_attribute;               ///< To set value using xtsc_request::set_dram_attribute()
  mutable sc_dt::sc_bv_base     m_dram_attribute_bv;            ///< To read dram attribute before converting to sc_unsigned 
  xtsc::xtsc_address            m_start_byte_address;           ///< Number to be add to the pin address to form the TLM request address
  xtsc::u64                     m_clock_period_value;           ///< This device's clock period as u64
  sc_core::sc_time              m_clock_period;                 ///< This device's clock period as sc_time
  sc_core::sc_time              m_time_resolution;              ///< The SystemC time resolution
  sc_core::sc_time              m_posedge_offset;               ///< From "posedge_offset" parameter
  sc_core::sc_time              m_sample_phase;                 ///< Clock phase for sampling certain signals (see "sample_phase")
  sc_core::sc_time              m_sample_phase_plus_one;        ///< m_sample_phase plus one clock period
  sc_core::sc_time              m_output_delay;                 ///< See "output_delay" parameter
  sc_core::sc_time             *m_request_time_stamp;           ///< Time stamp of last-transfer request when "one_at_a_time" is true   (per mem port)
  xtsc::u64                     m_posedge_offset_value;         ///< m_posedge_offset as u64
  bool                          m_pif_or_idma;                  ///< True if PIF|IDMA0 interfaces are in use, false if AXI|IDMA  or local memory.
  bool                          m_axi_or_idma;                  ///< True if AXI|IDMA  interfaces are in use, false if PIF|IDMA0 or local memory.
  bool                          m_has_posedge_offset;           ///< True if m_posedge_offset is non-zero
  bool                         *m_waiting_for_nacc;             ///< True if waiting for        RSP_NACC|NOTRDY from PIF|AXI4 slave     (per mem port)
  bool                         *m_request_got_nacc;             ///< True if active request got RSP_NACC|NOTRDY from PIF|AXI4 slave     (per mem port)
  bool                          m_cbox;                         ///< See "cbox" parameter
  bool                          m_split_rw;                     ///< True if m_interface_type is DRAM0RW or DRAM1RW
  bool                          m_has_dma;                      ///< See "has_dma" parameter
  bool                          m_append_id;                    ///< True if pin port names should include the set_id.
  bool                          m_inbound_pif;                  ///< True if interface is inbound PIF
  bool                          m_snoop;                        ///< True if interface is snoop port (future use)
  bool                          m_has_coherence;                ///< See "has_coherence" parameter
  bool                          m_has_pif_attribute;            ///< See "has_pif_attribute" parameter
  bool                          m_has_pif_req_domain;           ///< See "has_pif_req_domain" parameter
  bool                          m_axi_exclusive;                ///< See "axi_exclusive" parameter
  bool                          m_axi_exclusive_id;             ///< See "axi_exclusive_id" parameter
  bool                          m_big_endian;                   ///< True if master is big endian
  bool                          m_write_responses;              ///< True if TLM write responses will be dropped on the floor (PIF|IDMA0 only)
  bool                          m_has_request_id;               ///< True if the "POReqId" and "PIRespId" ports should be present
  bool                          m_one_at_a_time;                ///< See "one_at_a_time" parameter
  bool                          m_prereject_responses;          ///< See "prereject_responses" parameter
  std::vector<xtsc::u32>        m_axi_id_bits;                  ///< See "axi_id_bits" parameter.
  std::vector<xtsc::u32>        m_axi_route_id_bits;            ///< See "axi_route_id_bits" parameter.
  std::vector<xtsc::u32>        m_axi_read_write;               ///< See "axi_read_write" parameter. [N]=0 => Port #N is a read port, 1 => write port
  std::vector<xtsc::u32>        m_axi_port_type;                ///< See "axi_port_type" parameter.
  std::vector<std::string>      m_axi_name_prefix;              ///< See "axi_name_prefix" parameter
  xtsc::u32                    *m_current_id;                   ///< Used when m_has_request_id is false                                (per mem port)
  std::string                   m_req_user_data;                ///< See "req_user_data" parameter
  std::string                   m_req_user_data_name;           ///< Name of request user data port
  xtsc::u32                     m_req_user_data_width1;         ///< Bit width of request user data port
  std::string                   m_rsp_user_data;                ///< See "rsp_user_data" parameter
  std::string                   m_rsp_user_data_name;           ///< Name of response user data port
  xtsc::u32                     m_rsp_user_data_width1;         ///< Bit width of response user data port
  sc_dt::sc_bv_base            *m_user_data_val;                ///< Value for rsp_user_data port
  xtsc::u32                     m_address_bits;                 ///< Number of bits in the address (non-PIF|IDMA0|AXI|IDMA only)
  xtsc::u32                     m_check_bits;                   ///< Number of bits in ECC/parity signals (from "check_bits")
  xtsc::xtsc_address            m_address_mask;                 ///< Address mask
  xtsc::xtsc_address            m_bus_addr_bits_mask;           ///< Address mask to get bits which indicate which byte lane
  xtsc::u32                     m_address_shift;                ///< Number of bits to right-shift the address
  xtsc::u32                     m_route_id_bits;                ///< Number of bits in the route ID (PIF|IDMA0 only)
  xtsc::u32                     m_max_cycle_entries;            ///< See "max_cycle_entries" parameter
  subbank_activity            **m_subbank_activity;             ///< m_subbank_activity[bank][cycle_index]

  // The following m_next_port_*_thread members are used to give each thread "instance" a port number
  xtsc::u32                     m_next_port_lcl_request_thread;
  xtsc::u32                     m_next_port_lcl_drive_read_data_thread;
  xtsc::u32                     m_next_port_lcl_drive_busy_thread;
  xtsc::u32                     m_next_port_xlmi_load_retired_thread;
  xtsc::u32                     m_next_port_xlmi_retire_flush_thread;
  xtsc::u32                     m_next_port_pif_sample_pin_request_thread;
  xtsc::u32                     m_next_port_pif_drive_req_rdy_thread;
  xtsc::u32                     m_next_port_pif_send_tlm_request_thread;
  xtsc::u32                     m_next_port_pif_drive_pin_response_thread;
  xtsc::u32                     m_next_port_axi_req_addr_thread;
  xtsc::u32                     m_next_port_axi_req_data_thread;
  xtsc::u32                     m_next_port_axi_wr_req_thread;
  xtsc::u32                     m_next_port_axi_drive_req_rdy_thread;
  xtsc::u32                     m_next_port_axi_send_tlm_req_thread;
  xtsc::u32                     m_next_port_axi_drive_rsp_thread;
  xtsc::u32                     m_next_port_apb_request_thread;
  xtsc::u32                     m_next_port_apb_drive_outputs_thread;

  bool                          m_has_busy;                     ///< True if memory interface has a busy pin (non-PIF|IDMA0|AXI|IDMA|APB only)
  bool                          m_has_lock;                     ///< True if memory interface has a lock pin 
                                                                ///<  (DRAM0|DRAM0BS|DRAM0RW|DRAM1|DRAM1BS|DRAM1RW only)
  bool                          m_has_xfer_en;                  ///< True if memory interface has Xfer enable pin
                                                                ///<  (not applicable for PIF|IDMA0|AXI|IDMA|DROM0|XLMI0|APB)

  sc_core::sc_event            *m_pif_req_event;                ///< Notify pif_send_tlm_request_thread                                 (per mem port)
  sc_core::sc_event            *m_response_event;               ///< Notify pif_drive_pin_response_thread|axi_drive_rsp_thread          (per mem port)
  sc_core::sc_event            *m_axi_wr_req_event;             ///< Event used to notify axi_wr_req_thread                             (per mem port)
  sc_core::sc_event            *m_axi_send_tlm_req_event;       ///< Event used to notify axi_send_tlm_req_thread                       (per mem port)
  sc_core::sc_event            *m_apb_rsp_event;                ///< Event used to notify apb_drive_outputs_thread                      (per mem port)

  bool                         *m_first_block_write;            ///< True if next BLOCK_WRITE will be first in the block                (per mem port)
  xtsc::u32                    *m_burst_write_transfer_num;     ///< Transfer number for BURST_WRITE                                    (per mem port)
  bool                         *m_first_rcw;                    ///< True if next RCW will be first in the block                        (per mem port)
  bool                         *m_dram_lock;                    ///< State of DRAM lock pin
  bool                          m_dram_lock_reset;              ///< State of DRAM lock pin needs to be reset
  xtsc::u64                    *m_tag;                          ///< Tag from first BLOCK_WRITE and RCW
  xtsc::xtsc_address           *m_last_address;                 ///< Keep track of BLOCK_WRITE|BURST_WRITE addresses
  sc_dt::sc_uint_base           m_address;                      ///< The address after any required shifting and masking
  sc_dt::sc_uint_base           m_id;                           ///< POReqId|PIRespId
  sc_dt::sc_uint_base           m_priority;                     ///< POReqPriority|PIRespPriority
  sc_dt::sc_uint_base           m_route_id;                     ///< POReqRouteId|PIRespRouteId
  sc_dt::sc_uint_base           m_coh_cntl;                     ///< PIRespCohCntl|SnoopRespCohCntl
  sc_dt::sc_uint_base           m_len;                          ///< AxLEN
  sc_dt::sc_uint_base           m_size;                         ///< AxSIZE
  sc_dt::sc_uint_base           m_burst;                        ///< AxBURST
  sc_dt::sc_uint_base           m_cache;                        ///< AxCACHE
  sc_dt::sc_uint_base           m_prot;                         ///< AxPROT
  sc_dt::sc_uint_base           m_bar;                          ///< AxBAR
  sc_dt::sc_uint_base           m_domain;                       ///< AxDOMAIN
  sc_dt::sc_bv_base             m_data;                         ///< Read/write data
  req_cntl                      m_req_cntl;                     ///< Value for POReqCntl|SnoopReqCntl
  resp_cntl                     m_resp_cntl;                    ///< Value for PIRespCntl|SnoopRespCntl

  apb_info                    **m_apb_info_table;               ///< Table of apb_info objects                                          (per mem port)
  sc_dt::sc_uint_base         **m_xID;                          ///< For driving RID|BID                                                (per mem port)
  sc_dt::sc_uint_base           m_xRESP;                        ///< For driving RRESP|BRESP

  wide_fifo                   **m_read_data_fifo;               ///< sc_fifo of sc_bv_base read data values                             (per mem port)
  bool_fifo                   **m_busy_fifo;                    ///< sc_fifo to keep track of busy pin                                  (per mem port)
  bool_fifo                   **m_req_rdy_fifo;                 ///< sc_fifo to keep track of PIReqRdy|AxREADY pin                      (per mem port)
                                                                ///< A false entry, means to deassert for one clock period
                                                                ///< A true  entry, means to deassert until m_drive_req_rdy_event is notified again
                                                                ///< at which point the pin will be reasserted and a false entry read from the fifo.
  sc_core::sc_event            *m_drive_read_data_event;        ///< Notify when read data should be driven                             (per mem port)
  sc_core::sc_event            *m_drive_busy_event;             ///< Notify when busy should be driven                                  (per mem port)
  sc_core::sc_event            *m_drive_req_rdy_event;          ///< Notify when PIReqRdy|AxREADY should be driven                      (per mem port)
  std::vector<request_info*>    m_request_pool;                 ///< Maintain a pool of requests to improve performance
  std::vector<xtsc_response*>   m_response_pool;                ///< Maintain a pool of responses to improve performance
  std::deque<axi_addr_info*>   *m_axi_addr_fifo;                ///< The fifo of incoming requests on the AXI4 rd/wr addr channel       (per mem port)
  std::deque<axi_data_info*>   *m_axi_data_fifo;                ///< The fifo of incoming requests on the AXI4    wr data channel       (per mem port)
  std::vector<axi_addr_info*>   m_axi_addr_pool;                ///< Pool of axi_addr_info objects
  std::vector<axi_data_info*>   m_axi_data_pool;                ///< Pool of axi_data_info objects
  std::vector<axi_req_info*>    m_axi_req_pool;                 ///< Pool of axi_req_info objects
  address_deque                *m_load_address_deque;           ///< deque of XLMI load addresses                                       (per mem port)
  bool                         *m_previous_response_last;       ///< true if previous response was a last transfer                      (per mem port)
  map_bool_signal               m_map_bool_signal;              ///< The optional map of all sc_signal<bool> signals
  map_uint_signal               m_map_uint_signal;              ///< The optional map of all sc_signal<sc_uint_base> signals
  map_wide_signal               m_map_wide_signal;              ///< The optional map of all sc_signal<sc_bv_base> signals
  sc_dt::sc_bv_base             m_zero_bv;                      ///< For initialization
  sc_dt::sc_uint_base           m_zero_uint;                    ///< For initialization
  log4xtensa::TextLogger&       m_text;                         ///< Used for logging 

  // PIF|IDMA0 request channel pins (per mem port)
  bool_input                  **m_p_req_valid;                  ///< POReqValid/SnoopReqValid
  uint_input                  **m_p_req_cntl;                   ///< POReqCntl/SnoopReqCntl
  uint_input                  **m_p_req_adrs;                   ///< POReqAdrs/SnoopReqAdrs
  wide_input                  **m_p_req_data;                   ///< POReqData
  uint_input                  **m_p_req_data_be;                ///< POReqDataBE
  uint_input                  **m_p_req_id;                     ///< POReqId/SnoopReqId
  uint_input                  **m_p_req_priority;               ///< POReqPriority/SnoopReqPriority
  uint_input                  **m_p_req_route_id;               ///< POReqRouteId
  uint_input                  **m_p_req_attribute;              ///< POReqAttribute
  uint_input                  **m_p_req_domain;                 ///< POReqDomain
  uint_input                  **m_p_req_coh_vadrs;              ///< POReqCohVAdrsIndex/SnoopReqCohVAdrsIndex
  uint_input                  **m_p_req_coh_cntl;               ///< POReqCohCntl/SnoopReqCohCntl
  wide_input                  **m_p_req_user_data;              ///< Request User Data.  See "req_user_data" parameter.
  bool_output                 **m_p_req_rdy;                    ///< PIReqRdy/SnoopReqRdy
  
  // PIF|IDMA0 response channel pins (per mem port)
  bool_output                 **m_p_resp_valid;                 ///< PIRespValid/SnoopRespValid
  uint_output                 **m_p_resp_cntl;                  ///< PIRespCntl/SnoopRespCntl
  wide_output                 **m_p_resp_data;                  ///< PORespData/SnoopRespData
  uint_output                 **m_p_resp_id;                    ///< PIRespId/SnoopRespId
  uint_output                 **m_p_resp_priority;              ///< PIRespPriority
  uint_output                 **m_p_resp_route_id;              ///< PIRespRouteId
  uint_output                 **m_p_resp_coh_cntl;              ///< PIRespCohCntl/SnoopRespCohCntl
  wide_output                 **m_p_resp_user_data;             ///< Response User Data.  See "rsp_user_data" parameter.
  bool_input                  **m_p_resp_rdy;                   ///< PORespRdy/SnoopRespRdy
  
  // AXI4 read address channel pins (per mem read port)
  uint_input                  **m_p_arid;                       ///< ARID       4|8  "axi_id_bits"      "axi_route_id_bits"
  uint_input                  **m_p_araddr;                     ///< ARADDR     32
  uint_input                  **m_p_arlen;                      ///< ARLEN      8
  uint_input                  **m_p_arsize;                     ///< ARSIZE     3
  uint_input                  **m_p_arburst;                    ///< ARBURST    2
  bool_input                  **m_p_arlock;                     ///< ARLOCK     1
  uint_input                  **m_p_arcache;                    ///< ARCACHE    4
  uint_input                  **m_p_arprot;                     ///< ARPROT     3
  uint_input                  **m_p_arqos;                      ///< ARQOS      4
  uint_input                  **m_p_arbar;                      ///< ARBAR      2
  uint_input                  **m_p_ardomain;                   ///< ARDOMAIN   2
  uint_input                  **m_p_arsnoop;                    ///< ARSNOOP    4
  bool_input                  **m_p_arvalid;                    ///< ARVALID    1
  bool_output                 **m_p_arready;                    ///< ARREADY    1

  // AXI4 read response channel pins (per mem read port)
  uint_output                 **m_p_rid;                        ///< RID        4|8  "axi_id_bits"      "axi_route_id_bits"
  wide_output                 **m_p_rdata;                      ///< RDATA      32|64|128|256|512       "byte_width"
  uint_output                 **m_p_rresp;                      ///< RRESP      2|4                     "axi_port_type"
  bool_output                 **m_p_rlast;                      ///< RLAST      1
  bool_output                 **m_p_rvalid;                     ///< RVALID     1
  bool_input                  **m_p_rready;                     ///< RREADY     1

  // AXI4 write address channel pins (per mem write port)
  uint_input                  **m_p_awid;                       ///< AWID       4|8  "axi_id_bits"      "axi_route_id_bits"
  uint_input                  **m_p_awaddr;                     ///< AWADDR     32
  uint_input                  **m_p_awlen;                      ///< AWLEN      8
  uint_input                  **m_p_awsize;                     ///< AWSIZE     3
  uint_input                  **m_p_awburst;                    ///< AWBURST    2
  bool_input                  **m_p_awlock;                     ///< AWLOCK     1
  uint_input                  **m_p_awcache;                    ///< AWCACHE    4
  uint_input                  **m_p_awprot;                     ///< AWPROT     3
  uint_input                  **m_p_awqos;                      ///< AWQOS      4
  uint_input                  **m_p_awbar;                      ///< AWBAR      2
  uint_input                  **m_p_awdomain;                   ///< AWDOMAIN   2
  uint_input                  **m_p_awsnoop;                    ///< AWSNOOP    3
  bool_input                  **m_p_awvalid;                    ///< AWVALID    1
  bool_output                 **m_p_awready;                    ///< AWREADY    1

  // AXI4 write data channel pins (per mem write port)
  wide_input                  **m_p_wdata;                      ///< WDATA      32|64|128|256|512       "byte_width"
  uint_input                  **m_p_wstrb;                      ///< WSTRB      4|8|16|32|64            "byte_width" / 8
  bool_input                  **m_p_wlast;                      ///< WLAST      1
  bool_input                  **m_p_wvalid;                     ///< WVALID     1
  bool_output                 **m_p_wready;                     ///< WREADY     1

  // AXI4 write response channel pins (per mem write port)
  uint_output                 **m_p_bid;                        ///< BID        4|8  "axi_id_bits"      "axi_route_id_bits"
  uint_output                 **m_p_bresp;                      ///< BRESP      2
  bool_output                 **m_p_bvalid;                     ///< BVALID     1
  bool_input                  **m_p_bready;                     ///< BREADY     1
  
  // APB pins (per mem port)
  bool_input                  **m_p_psel;                       ///< PSEL       1
  bool_input                  **m_p_penable;                    ///< PENABLE    1
  bool_input                  **m_p_pwrite;                     ///< PWRITE     1
  uint_input                  **m_p_paddr;                      ///< PADDR      32
  uint_input                  **m_p_pprot;                      ///< PPROT      3
  uint_input                  **m_p_pstrb;                      ///< PSTRB      4
  wide_input                  **m_p_pwdata;                     ///< PWDATA     32
  bool_output                 **m_p_pready;                     ///< PREADY     1
  bool_output                 **m_p_pslverr;                    ///< PSLVERR    1
  wide_output                 **m_p_prdata;                     ///< PRDATA     32

  // Local Memory pins (per mem port)
  bool_input                  **m_p_en;                         ///< DPortEn, DRamEn, DRomEn, IRamEn, IRomEn
  uint_input                  **m_p_addr;                       ///< DPortAddr, DRamAddr, DRomAddr, IRamAddr, IRomAddr
  uint_input                  **m_p_lane;                       ///< DPortByteEn, DRamByteEn, DRomByteEn, IRamWordEn, IRomWordEn
  wide_input                  **m_p_wrdata;                     ///< DPortWrData, DRamWrData, IRamWrData
  bool_input                  **m_p_wr;                         ///< DPortWr, DRamWr, IRamWr
  bool_input                  **m_p_load;                       ///< DPortLoad, IRamLoadStore, IRomLoad
  bool_input                  **m_p_retire;                     ///< DPortLoadRetired
  bool_input                  **m_p_flush;                      ///< DPortRetireFlush
  bool_input                  **m_p_lock;                       ///< DRamLock
  wide_input                  **m_p_attr;                       ///< DRamAttr, DRamWrAttr
  wide_input                  **m_p_check_wr;                   ///< DRamCheckWrData, IRamCheckWrData
  wide_output                 **m_p_check;                      ///< DRamCheckData, IRamCheckData
  bool_input                  **m_p_xfer_en;                    ///< DRamXferEn, IRamXferEn, IRomXferEn, URamXferEn
  bool_output                 **m_p_busy;                       ///< DPortBusy, DRamBusy, DRomBusy, IRamBusy, IRomBusy
  wide_output                 **m_p_data;                       ///< DPortData, DRamData, DRomData, IRamData, IRomData
  
  std::vector<sc_core::sc_process_handle>
                                m_process_handles;              ///< For reset 

  static const xtsc::u32        AXI4     = 1;                   ///< For "axi_port_type" entry values
  static const xtsc::u32        ACE_LITE = 2;                   ///< For "axi_port_type" entry values
  static const xtsc::u32        ACE      = 3;                   ///< For "axi_port_type" entry values

};  // class xtsc_pin2tlm_memory_transactor 



}  // namespace xtsc_component



#endif // _XTSC_PIN2TLM_MEMORY_TRANSACTOR_H_
