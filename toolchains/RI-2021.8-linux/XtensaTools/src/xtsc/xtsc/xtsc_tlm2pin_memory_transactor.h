#ifndef _XTSC_TLM2PIN_MEMORY_TRANSACTOR_H_
#define _XTSC_TLM2PIN_MEMORY_TRANSACTOR_H_

// Copyright (c) 2007-2017 by Cadence Design Systems Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of
// Cadence Design Systems Inc.  They may be adapted and modified by bona fide
// purchasers for internal use, but neither the original nor any adapted
// or modified version may be disclosed or distributed to third parties
// in any manner, medium, or form, in whole or in part, without the prior
// written consent of Cadence Design Systems Inc.

/**
 * @file 
 */


#include <deque>
#include <map>
#include <vector>
#include <string>
#include <xtsc/xtsc_parms.h>
#include <xtsc/xtsc_request.h>
#include <xtsc/xtsc_request_if.h>
#include <xtsc/xtsc_respond_if.h>
#include <xtsc/xtsc_module_pin_base.h>
#include <xtsc/xtsc_memory_b.h>
#include <xtsc/xtsc_core.h>




namespace xtsc_component {


// Forward references
class xtsc_arbiter;
class xtsc_master;
class xtsc_memory_trace;
class xtsc_router;


/**
 * Constructor parameters for a xtsc_tlm2pin_memory_transactor transactor object.
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
                                set the "snoop" parameter to true.
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
                                "AXI" or "IDMA", then reads and writes must come in on
                                separate ports (separate Xtensa TLM port pairs) and a
                                port supporting reads is referred to below as a read
                                port and a port supporting writes is referred to as a
                                write port.
                                Default = 1.
                                Minimum = 1.
         Note: If you have AXI4 read and write requests on the same Xtensa TLM path,
               then an upstream xtsc_component::xtsc_router can be used to separate the
               reads and writes (see xtsc_component::xtsc_router_parms "route_by_type").

   "axi_id_bits"    vector<u32> A vector containing "num_ports" number of entries.
                                Each entry indicates the number of low-order bits from
                                the request ID to use as low-order bits of the AXI
                                transaction ID (AxID|xID).  This parameter must be
                                specified if "memory_interface" is "AXI" or "IDMA" and
                                is ignored otherwise.  Valid entry value range is 0-8.
                                If "num_ports" is greater than 1 but only one entry is
                                provided for this parameter, then that entry will be
                                duplicated to provide "num_ports" entries.
                                Default = <empty>

   "axi_route_id_bits" vector<u32> A vector containing "num_ports" number of entries.
                                Each entry indicates the number of low-order bits from
                                the request's route ID to use as high-order bits of the
                                AXI4 transaction ID (AxID|xID).  If "memory_interface"
                                is not "AXI"/"IDMA", this parameter is ignored.  If
                                "memory_interface" is "AXI"/"IDMA" and this parameter
                                is empty, then a value of 0 will be used for all ports
                                (no route ID bits will be used in the transaction ID).
                                If "num_ports" is greater than 1 but only one entry is
                                provided for this parameter, then that entry will be
                                duplicated to provide "num_ports" entries.
                                Default = <empty>
         Note: The total transaction ID bit width determined from "axi_route_id_bits"
               and "axi_id_bits" must be in the range of 1-32.

   "axi_read_write" vector<u32> A vector containing "num_ports" number of entries.  An
                                entry value of 0 indicates a read port (and only
                                xtsc::xtsc_request::BURST_READ requests are allowed on
                                that port). An entry value of 1 indicates a write
                                port (and only xtsc::xtsc_request::BURST_WRITE requests
                                are allowed on that port). An entry value of 2 indicates
                                a snoop port (and only xtsc::xtsc_request::SNOOP 
                                requests are allowed on that port).  This parameter must
                                be specified if "memory_interface" is "AXI" or "IDMA" 
                                and is ignored otherwise.
                                If "num_ports" is greater than 1 but only one entry is
                                provided for this parameter, then that entry will be
                                duplicated to provide "num_ports" entries.
                                Default = <empty>
         Note: A read port is associated with 2 AXI4 channels (Read Address channel and
               Read Data channel), a write port is associated with 3 AXI4 channels
               (Write Address channel, Write Data channel, and Write Response channel)
               and a snoop port is associated with 3 ACE channels (Snoop Address 
               channel, Snoop Data channel, and Snoop Response channel).
               The I/O pins of both AXI4 channels comprising a read port are grouped
               together in their own I/O pin set in xtsc_component::xtsc_module_pin_base.
               The I/O pins of all 3 AXI4 channels comprising a write port are grouped
               together in their own I/O pin set.
               The I/O pins of all 3 ACE channels comprising a snoop port are grouped
               together in their own I/O pin set.

   "axi_port_type" vector<u32>  A vector containing "num_ports" number of entries.  An
                                entry value of 1 indicates the port is a plain AXI4
                                port, an entry value of 2 indicates an ACE-Lite port,
                                and an entry value of 3 indicates a full ACE port.  This
                                parameter must be specified if "memory_interface" is
                                "AXI" or "IDMA" and is ignored otherwise.
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

   "port_name_suffix"   char*   Optional constant suffix to be appended to every
                                pin-level input and output port name (every I/O pin
                                name).
                                Default = "".

   "byte_width"         u32     Memory data interface width in bytes.  Valid values for
                                "AXI", "IDMA", "DRAM0", "DRAM0RW", "DRAM1", "DRAM1RW",
                                "DROM0", "URAM0", and "XLMI0" are 4, 8, 16, 32, and 64.
                                Valid values for "DRAM0BS", "DRAM1BS", "IRAM0", "IRAM1",
                                "IROM0", "IDMA0" and "PIF" are 4, 8, 16, and 32. The 
                                only valid value for "APB" is 4.

   "start_byte_address" u32     The number to be subtracted from the address received in
                                the TLM request.  For non-XLMI local memories, this
                                corresponds to the starting byte address of the memory
                                in the 4GB address space.  For PIF, IDMA0, AXI, IDMA,
                                APB, and XLMI this should be 0x00000000.
                                Default = 0x00000000.
  
   "big_endian"         bool    True if the memory interface master is big endian.
                                Default = false.

   "deny_fast_access" vector<u32> A vector containing an even number of addresses.
                                Each pair of addresses specifies a range of addresses
                                that will be denied fast access.  The first address in
                                each pair is the start address and the second address in
                                each pair is the end address.
                                Default = <empty>

   "dso_name"           char*   Optional name of a DSO (Linux shared object or MS Windows
                                DLL) that contains peek and poke functions declared with
                                the signature and with extern "C" (and __declspec if 
                                MS Windows) as shown here:
#ifdef _WIN32
#define DSO_EXPORT extern "C" __declspec(dllexport) 
#else
#define DSO_EXPORT extern "C" 
#endif
typedef unsigned char u8;
typedef unsigned int  u32;
DSO_EXPORT void peek(u32 address8, u32 size8,       u8 *buffer, const char *dso_cookie, u32 port);
DSO_EXPORT void poke(u32 address8, u32 size8, const u8 *buffer, const char *dso_cookie, u32 port);
                                Typically this parameter is only needed when connecting
                                this module to a Verilog/SystemVerilog module for
                                cosimulation when peek/poke access is desired to the
                                Verilog memory (for example, for loading a target
                                program using xtsc_core, for TurboXim or xt-gdb access,
                                or for semi-hosting support for argv[], clock(),
                                fopen(), fread(), fwrite(), printf(), scanf(), etc).
                                The idea is that the named DSO, provided by the user, is
                                able to use some non-blocking mechanism (i.e. a
                                mechanism that does not require any simulation time to
                                elapse) to communicate the peeks/pokes to the Verilog/
                                SystemVerilog module (for example, using a SystemVerilog
                                export "DPI-C" function).  The dso_cookie argument will
                                come from the "dso_cookie" parameter.  
                                Caution: When a DSO is used during SystemC-Verilog
                                         cosimulation, be sure to use the xtsc_core_parm
                                         parameter SimTargetProgram to name the core
                                         program (if any) that will be loaded using the
                                         DSO.  The xtsc-run command --core_program=
                                         should not be used in this situation because it
                                         can result in the core program being loaded
                                         prior to the Verilog memory being constructed.
                                         This is vendor specific but results in a
                                         failure that is hard to decipher.
                                Default = NULL.

   "dso_cookie"         char*   Optional C-string to pass to the peek and poke methods
                                of the DSO named by the "dso_name" parameter.  This
                                model does not use this parameter in any way other then
                                to pass it to the DSO methods.
                                Default = NULL.

   "cosim"              bool    This parameter is for when the model is connected to a
                                Verilog model and "dso_name" is NULL.  Because Verilog
                                is not automatically compatible with the xtsc_debug_if
                                methods (nb_peek, nb_poke, and nb_fast_access) a way is
                                needed to prevent a run-time exception if these methods
                                are called.  If "cosim" is false and the user does not
                                connect the m_debug_ports (either directly or by passing
                                a DSO name in using the "dso_name" parameter) then a
                                call to one of these methods coming in on
                                m_request_exports will result in an exception being
                                thrown.  If "cosim" is true and the user does not
                                connect the m_debug_ports then a call to one of these
                                methods will be handled according to the "shadow_memory"
                                parameter.
                                Default = false.

   "shadow_memory"      bool    If "cosim" is false or if the user connects the
                                m_debug_ports (either directly or by passing a DSO name
                                in using the "dso_name" parameter) then this parameter
                                is ignored.  Otherwise, if "shadow_memory" is false,
                                calls to nb_peek() and nb_poke() will be ignored and
                                calls to nb_fast_access() will return false.  If
                                "shadow_memory" is true, then all writes (calls to
                                nb_request for WRITE, BLOCK_WRITE, BURST_WRITE, and the
                                write beat of RCW) and all nb_poke() calls will update a
                                locally-maintained shadow memory (all writes will also
                                be driven out the pin-level interface).  All nb_peek()
                                calls will return data from the shadow memory.  Reads
                                (calls to nb_request for READ, BLOCK_READ, BURST_READ,
                                and the read beat of RCW) will never use the shadow
                                memory.  Calls to nb_fast_access will return false.
                                Note: The shadow memory mechanism is far from perfect
                                and should NOT be relied upon for accurate simulation
                                and debugging.  Here are some ways in which the shadow
                                memory could be inaccurate:
                                1. Write requests always update the shadow memory even 
                                   if the pin-level write request is not accepted or is
                                   accepted and then rejected due to, for example, an
                                   address or data error.
                                2. The write beat of RCW always updates the shadow memory
                                   regardless of the result of the conditional read part.
                                3. The Verilog memory, as opposed to the shadow memory,
                                   might be modified by other means then through this
                                   module.
                                4. If the debugger (xt-gdb/Xplorer) is used to change
                                   memory contents, only the shadow memory will be
                                   changed.  The Verilog memory, which is what the
                                   Xtensa processor will see when it does read requests,
                                   will not be changed.
                                5. During execution of the SIMCALL pseudo instruction by
                                   the ISS in support of certain semi-hosting features
                                   such as argc, argv[], clock(), fread(), and scanf(),
                                   the host (i.e. the simulator) uses pokes to put data
                                   in memory which is then retrieved using read requests
                                   (load instructions that follow the SIMCALL pseudo
                                   instruction) by the Xtensa processor.  The poke will
                                   go only to the shadow memory; however, the Xtensa
                                   will read the Verilog memory and so will not get the
                                   correct data.
                                Despite the above caveat and issues, in cases where a
                                peek/poke DSO is not available, a shadow memory can
                                sometimes prove useful (for example, to support printf
                                in target code and/or partial debugger visibility).
                                Note: fwrite/printf (unlike fread/scanf) do work with a
                                shadow memory because in these cases Xtensa code first
                                does writes (store instructions) that go to both the
                                shadow memory and to the Verilog memory and then the
                                host does peeks (which go to the shadow memory) to
                                retrieve the data for writing to the host file system or
                                display.
                                Default = false.

   "initial_value_file" char*   If not NULL or empty, this names a text file from which
                                to read the initial memory contents as byte values.
                                This parameter is ignored unless a shadow memory is
                                used.
                                Default = NULL.
                                The text file format is:

                                ([@<Offset>] <Value>*)*

                                1.  Any number (<Offset> or <Value>) can be in decimal
                                    or hexadecimal (using '0x' prefix) format.
                                2.  @<Offset> is added to "start_byte_address".
                                3.  <Value> cannot exceed 255 (0xFF).
                                4.  If a <Value> entry is not immediately preceeded in
                                    the file by an @<Offset> entry, then its offset is
                                    one greater than the preceeding <Value> entry.
                                5.  If the first <Value> entry in the file is not 
                                    preceeded by an @<Offset> entry, then its offset
                                    is zero.
                                6.  Comments, extra whitespace, and blank lines are
                                    ignored.  See xtsc::xtsc_script_file.

                                Example text file contents:

                                   0x01 0x02 0x3    // First three bytes of the memory,
                                                    // 0x01 is at "start_byte_address"
                                   @0x1000 50       // The byte at offset 0x1000 is 50
                                   51 52            // The byte at offset 0x1001 is 51
                                                    // The byte at offset 0x1002 is 52

   "memory_fill_byte"   u32     The low byte specifies the value used to initialize 
                                memory contents at address locations not initialize
                                from "initial_value_file".
                                This parameter is ignored unless a shadow memory is
                                used.
                                Default = 0.

   "clock_period"       u32     This is the length of this device's clock period
                                expressed in terms of the SystemC time resolution
                                (from sc_get_time_resolution()).  A value of 
                                0xFFFFFFFF means to use the XTSC system clock 
                                period (from xtsc_get_system_clock_period()).  A value
                                of 0 means one delta cycle.
                                Default = 0xFFFFFFFF (i.e. use the system clock 
                                period).

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
                                which are used for handshaking (POReqValid, PORespRdy,
                                ARVALID, AWVALID, WVALID, RREADY, BREADY, IRamBusy,
                                DRamBusy, etc) are also sampled at this time.  This
                                value is expressed in terms of the SystemC time
                                resolution (from sc_get_time_resolution()) and must be
                                less than the clock period as specified by parameter
                                "clock_period".  A value of 0 means pins are sampled on
                                posedge clock as specified by "posedge_offset".  A value
                                of 0xFFFFFFFF, the default, means if "memory_interface"
                                is PIF|IDMA0|AXI|IDMA|APB, then pins will be sampled at
                                posedge clock, and if "memory_interface" is something
                                else (i.e. a local memory), then pins will be sampled at
                                1 SystemC time resolution prior to phase A.  This latter
                                value is used to enable meeting the TLM timing
                                requirements of the local memory interfaces of
                                xtsc_core.  See the discussion under
                                xtsc_core::set_clock_phase_delta_factors().
                                Default = 0xFFFFFFFF

   "output_delay"       u32     This specifies how long to delay before output pins are
                                driven.  The output pins will remain driven for one
                                clock period (see the "clock_period" parameter).  For
                                request output pins, the delay timing starts when the
                                nb_request() call is received.  For DPortLoadRetired,
                                DPortRetireFlush, and DRamLock, the delay timing starts
                                when the nb_load_retire(), nb_retire_flush(), or
                                nb_lock() call (respectively) is received.  For
                                PORespRdy, the delay timing starts at the sample phase
                                (see "sample_phase") when the nb_respond() call
                                returns false.  This value is expressed in terms of the
                                SystemC time resolution (from sc_get_time_resolution())
                                and must be less than the clock period.  A value of 0
                                means one delta cycle.
                                Default = 1 (i.e. 1 time resolution).

   "vcd_handle"         void*   Pointer to SystemC VCD object (sc_trace_file *) or 0 if
                                tracing is not desired.

   "request_fifo_depth" u32     The request fifo depth.  
                                Default = 1.
                                Minimum = 1.


   Parameters which apply to PIF|IDMA0 only:

   "inbound_pif"        bool    Set to true for inbound PIF.  Set to false for outbound
                                PIF or snoop.  This parameter is ignored if
                                "memory_interface" is other then "PIF".
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

   "write_responses"    bool    True if write responses should be generated by the
                                xtsc_tlm2pin_memory_transactor.  False if write
                                responses will not be generated by the transactor
                                because they are either generated by the downstream
                                slave or are not required by any upstream master.  This
                                parameter is ignored unless "memory_interface" is "PIF"
                                or "IDMA0".
                                Default = false.
                                Note for xtsc-run users:  When the --cosim command is
                                present, xtsc-run will initialize this parameter with
                                a value appropriate for the current config.

   "track_write_requests" bool  True if write requests should be tracked in anticipation
                                of receiving write responses from the downstream slave.
                                A situation in which you may need to set this parameter
                                to false is when xtsc_core_parms parameter
                                "SimPIFFakeWriteResponses" is true and the downstream
                                slave (at least in some cases) does not send write
                                responses.  This parameter is ignored unless
                                "memory_interface" is "PIF" and "write_responses" is
                                false.
                                Default = true.

   "discard_unknown_responses" bool   If false an exception will be thrown if an unknown
                                response is received from the downsteam slave.  If true,
                                unknown responses from the downstream slave will be
                                discarded.  An "unknown response" is a response which
                                cannot be matched to a preceeding request (this can
                                happen because there was no matching preceeding request
                                or because "write_responses" is true or because
                                "track_write_requests" is false).  One situation in
                                which you may need to set this parameter to true, is
                                when the "write_responses" is true and the downstream
                                slave subsystem (at least sometimes) sends write
                                responses.  Another situation is when the downstream
                                slave subsystem sometimes sends write responses and
                                sometimes does not send them.  This parameter is ignored
                                unless "memory_interface" is "PIF".
                                Default = false.

   "route_id_bits"      u32     Number of bits in the route ID.  Valid values are 0-32.
                                If "route_id_bits" is 0, then the "POReqRouteId" and
                                "PIRespRouteId" output ports will not be present.  This
                                parameter is ignored unless "memory_interface" is "PIF"
                                or "IDMA0".

   "req_user_data"      char*   If not NULL or empty, this specifies the optional pin-
                                level port that should be used for xtsc_request user
                                data.  The string must give the port name and bit width
                                using the format: PortName,BitWidth
                                Note: The values driven on PortName will be obtained
                                      from the low order BitWidth bits of the void*
                                      pointer returned by xtsc_request::get_user_data().  
                                      BitWidth may not exceed 32 or 64 depending on
                                      whether you are using a 32 or 64 bit simulator.
                                This parameter is ignored unless "memory_interface" is
                                "PIF" or "IDMA0".
                                Default = "" (xtsc_request user data is ignored).

   "rsp_user_data"      char*   If not NULL or empty, this specifies the optional pin-
                                level port that should be used for xtsc_response user
                                data.  The string must give the port name and bit width
                                using the format: PortName,BitWidth
                                Note: The values read from PortName will be written to
                                      the low order BitWidth bits of the void* pointer
                                      passed to xtsc_response::set_user_data().  
                                      BitWidth may not exceed 32 or 64 depending on
                                      whether you are using a 32 or 64 bit simulator.
                                This parameter is ignored unless "memory_interface" is
                                "PIF" or "IDMA0".
                                Default = "" (xtsc_response user data is ignored).


   Parameters which apply to local memories only (that is, non-PIF|IDMA0|AXI|IDMA|APB
   memories):

   "address_bits"       u32     Number of bits in the address.  This parameter is
                                ignored if "memory_interface" is not a local memory.

   "check_bits"         u32     Number of bits in the parity/ecc signals.  This
                                parameter is ignored unless "memory_interface" is
                                "IRAM0", "IRAM1", "DRAM0", "DRAM0BS", "DRAM0RW",
                                "DRAM1", "DRAM1BS", or "DRAM1RW".
                                Default = 0.
  
   "has_busy"           bool    True if the memory interface has a busy pin (or ready
                                pin if "external_udma" is true).  This parameter is
                                ignored if "memory_interface" is not a local memory.
                                Default = true.
  
   "has_lock"           bool    True if the memory interface has a lock pin.  This
                                parameter is ignored unless "memory_interface" is
                                "DRAM0", "DRAM0BS", "DRAM0RW", "DRAM1", "DRAM1BS", or
                                "DRAM1RW".
                                Default = false.
  
   "has_xfer_en"        bool    True if the memory interface has an xfer enable pin.
                                This parameter is ignored if "memory_interface" is
                                "DROM0", "XLMI0", or is not a local memory.
                                Default = false.
  
   "read_delay"         u32     Number of clock periods to delay before read data is
                                sampled.  This should be 0 for a 5-stage pipeline and 1
                                for a 7-stage pipeline.  This parameter is ignored if
                                "memory_interface" is not a local memory.
                                Default = 0.
  
   "cbox"               bool    True if this memory interface is driven from an Xtensa
                                CBOX.  This parameter is ignored if "memory_interface"
                                is other then "DRAM0"|"DRAM1"|"DROM0".
                                Default = false.
  
   "banked"             bool    True if this is a banked "DRAM0"|"DRAM1"|"DROM0"
                                interface or if this is a "DRAM0BS"|"DRAM1BS" interface.
                                Default = false.
  
   "num_subbanks"       u32     The number of subbanks that each bank has.

   "has_dma"            bool    True if the memory interface has split Rd/Wr ports
                                ("DRAM0RW"|"DRAM1RW") accessible from inbound PIF.
                                Default = false.
  
   "dram_attribute_width" u32   160 if the "DRamNAttr" and "DRamNWrAttr" ports
                                should be present.  This parameter is ignored unless
                                "memory_interface" is "DRAM0RW" or "DRAM1RW".
                                Default = 0.
  
   "external_udma"      bool    True if this is the DataRam interface of the external
                                micro-DMA engine (xtsc_udma).  When this parameter is
                                set to true, then "memory_interface" must be "DRAM0";
                                "num_ports" must be 1; "address_bits" must be 32;
                                "check_bits" and "start_byte_address" must be 0; and
                                "banked", "has_lock", and "has_xfer_en" must be false.
                                Although "memory_interface" must be "DRAM0", "num_ports"
                                must be 1, and "banked" must be false, the actual
                                interface presented by the transactor is intended to be
                                connected through a banked memory interface mux to 4
                                memory banks (2 banks of DataRam0 and 2 banks of
                                DataRam1).
                                Default = false.
  
   "ram_select_bit"     u32     This parameter specifies which bit in the 32-bit address
                                selects DataRam0 and DataRam1 when "external_udma" is
                                true.  This parameter is ignored when "external_udma" is
                                false.
                                Default = 0xFFFFFFFF (must be explicitly set).
  
   "ram_select_normal"  bool    This parameter specifies the value that the address bit
                                specified by "ram_select_bit" should have to select 
                                DataRam0 or DataRam1.  True means a value of 0 selects
                                DataRam0 and a value of 1 selects DataRam1.  False
                                selects the opposite way.  This parameter is ignored
                                when "external_udma" is false.
                                Default = true.
    \endverbatim
 *
 * @see xtsc_tlm2pin_memory_transactor
 * @see xtsc::xtsc_parms
 * @see xtsc::xtsc_initialize_parms
 */
class XTSC_COMP_API xtsc_tlm2pin_memory_transactor_parms : public xtsc::xtsc_parms {
public:


  /**
   * Constructor for an xtsc_tlm2pin_memory_transactor_parms transactor object.
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
  xtsc_tlm2pin_memory_transactor_parms(const char      *memory_interface        = "PIF",
                                       xtsc::u32        byte_width              = 4,
                                       xtsc::u32        address_bits            = 32,
                                       xtsc::u32        num_ports               = 1)
  {
    init(memory_interface, byte_width, address_bits, num_ports);
  }


  /**
   * Constructor for an xtsc_tlm2pin_memory_transactor_parms transactor object based
   * upon an xtsc_core object and a named memory interface. 
   *
   * This constructor will determine "clock_period", "byte_width", "big_endian",
   * "read_delay", "address_bits", "start_byte_address", "has_busy", "has_lock", "cbox",
   * "banked", "num_subbanks", "has_dma", "dram_attribute_width", "check_bits",
   * "has_pif_attribute", "has_pif_req_domain", "axi_id_bits", "axi_read_write",
   * "axi_port_type", and "axi_name_prefix" by querying the core object.  
   *
   * If desired, after the xtsc_tlm2pin_memory_transactor_parms object is constructed,
   * its data members can be changed using the appropriate xtsc_parms::set() method
   * before passing it to the xtsc_tlm2pin_memory_transactor constructor.
   *
   * @param     core                    A reference to the xtsc_core object upon which
   *                                    to base the xtsc_tlm2pin_memory_transactor_parms.
   *
   * @param     memory_interface        The memory interface type.  Valid values are
   *                                    "DRAM0", "DRAM0BS", "DRAM0RW", "DRAM1",
   *                                    "DRAM1BS", "DRAM1RW", "DROM0", "IRAM0", "IRAM1",
   *                                    "IROM0", "URAM0", "XLMI0", "PIF", "IDMA0", "AXI",
   *                                    "IDMA", and "APB" (case-insensitive).
   *
   * @param     num_ports               The number of ports this transactor has.  If 0,
   *                                    the number of ports will be determined by
   *                                    calling xtsc_core::get_multi_port_count().  If
   *                                    xtsc_core::is_multi_port_zero() returns false or
   *                                    if any of the ports are already connected then
   *                                    num_ports will be inferred to be 1.
   */
  xtsc_tlm2pin_memory_transactor_parms(const xtsc::xtsc_core& core, const char *memory_interface, xtsc::u32 num_ports = 0);


  // Perform initialization common to both constructors
  void init(const char *memory_interface, xtsc::u32 byte_width, xtsc::u32 address_bits, xtsc::u32 num_ports) {
    std::vector<xtsc::u32> empty_vector_u32;
    add("memory_interface",          memory_interface);
    add("inbound_pif",               false);
    add("snoop",                     false);
    add("has_coherence",             false);
    add("has_pif_attribute",         false);
    add("has_pif_req_domain",        false);
    add("dram_attribute_width",      0);
    add("num_ports",                 num_ports);
    add("axi_id_bits",               empty_vector_u32);
    add("axi_route_id_bits",         empty_vector_u32);
    add("axi_read_write",            empty_vector_u32);
    add("axi_port_type",             empty_vector_u32);
    add("axi_name_prefix",           "");
    add("port_name_suffix",          "");
    add("byte_width",                byte_width);
    add("start_byte_address",        0x00000000);
    add("clock_period",              0xFFFFFFFF);
    add("posedge_offset",            0xFFFFFFFF);
    add("sample_phase",              0xFFFFFFFF);
    add("output_delay",              1);
    add("big_endian",                false);
    add("deny_fast_access",          empty_vector_u32);
    add("dso_name",                  (char*)NULL);
    add("dso_cookie",                (char*)NULL);
    add("cosim",                     false);
    add("shadow_memory",             false);
    add("test_shadow_memory",        false);         // Do not use - for test purposes only
    add("initial_value_file",        (char*)NULL);
    add("memory_fill_byte",          0);
    add("has_request_id",            true);
    add("write_responses",           false);
    add("track_write_requests",      true);
    add("discard_unknown_responses", false);
    add("address_bits",              address_bits);
    add("route_id_bits",             0);
    add("req_user_data",             "");
    add("rsp_user_data",             "");
    add("has_busy",                  true);
    add("has_lock",                  false);
    add("has_xfer_en",               false);
    add("read_delay",                0);
    add("vcd_handle",                (void*)NULL);
    add("request_fifo_depth",        1);
    add("cbox",                      false);
    add("banked",                    false);
    add("num_subbanks",              0);
    add("has_dma",                   false);
    add("external_udma",             false);
    add("ram_select_bit",            0xFFFFFFFF);
    add("ram_select_normal",         true);
    add("check_bits",                0);
  }


  /// Our C++ type (the xtsc_parms base class uses this for error messages)
  virtual const char* kind() const { return "xtsc_tlm2pin_memory_transactor_parms"; }

};




/**
 * This transactor converts memory transactions from transaction level to pin level.
 *
 * This module is a transactor which converts Xtensa TLM memory requests
 * (xtsc_request_if) to pin-level requests and the corresponding pin-level responses
 * into Xtensa TLM responses (xtsc_respond_if).
 *
 * On the Xtensa TLM side, this module can be connected with any XTSC memory interface
 * master (e.g. xtsc_core, xtsc_arbiter, xtsc_master, xtsc_router, etc).  However, it is
 * always configured for the specified memory interface of xtsc_core (such as DRAM0,
 * DRAM1, IRAM0, PIF, etc) or of the external micro-DMA engine.  See the
 * "memory_interface" and "external_udma" parameters in
 * xtsc_tlm2pin_memory_transactor_parms.
 *
 * Although there is a pin-level XTSC memory model (xtsc_memory_pin) that this
 * transactor can be connected to on the pin-level side, the main use for this
 * transactor is assumed to be for connecting to RTL (Verilog) models of, for instance,
 * XLMI or PIF devices.  When connecting to RTL, there is no guarranteed support for the
 * Xtensa TLM debug interface (see the m_debug_ports data member) because there is no
 * way to guarrantee support for non-blocking access across the SystemC-Verilog boundary
 * for peek and poke to arbitrary Verilog modules.  See the discussion of the "dso_name",
 * "dso_cookie", "cosim", and "shadow_memory" parameters in
 * xtsc_tlm2pin_memory_transactor_parms for more information.
 *
 * This module inherits from the xtsc_module_pin_base class which is responsible for
 * maintaining the pin-level sc_in<> and sc_out<> ports.  The pin-level port names
 * exactly match the Xtensa RTL.  These names, as well as their SystemC type and bit
 * width, are logged at info log-level when the module is constructed.
 *
 * This module supports driving multi-ported memories.  See the "num_ports" parameter of
 * xtsc_tlm2pin_memory_transactor_parms.
 *
 * Note: The parity/ECC signals (DRamNCheckDataM, DRamNCheckWrDataM, IRamNCheckData, and
 * IRamNCheckWrData) are present for IRAM and DRAM interfaces when "check_bits" is
 * non-zero; however, the input signal is ignored and the output signal is driven with
 * constant 0.
 *
 * Here is a block diagram of an xtsc_tlm2pin_memory_transactor as it is used in the
 * xtsc_tlm2pin_memory_transactor example:
 * @image html  Example_xtsc_tlm2pin_memory_transactor.jpg
 * @image latex Example_xtsc_tlm2pin_memory_transactor.eps "xtsc_tlm2pin_memory_transactor Example" width=13cm
 *
 * @see xtsc_module_pin_base
 * @see xtsc_memory_pin
 * @see xtsc_tlm2pin_memory_transactor_parms
 * @see xtsc::xtsc_core
 * @see xtsc_arbiter
 * @see xtsc_master
 * @see xtsc_router
 *
 */
class XTSC_COMP_API xtsc_tlm2pin_memory_transactor : public sc_core::sc_module,
                                                     public xtsc::xtsc_module,
                                                     public xtsc::xtsc_command_handler_interface,
                                                     public xtsc_module_pin_base
{
public:

  sc_core::sc_export<xtsc::xtsc_request_if>           **m_request_exports;      /// From master to us (per mem port)
  sc_core::sc_port  <xtsc::xtsc_respond_if>           **m_respond_ports;        /// From us to master (per mem port)
  sc_core::sc_port  <xtsc::xtsc_debug_if, NSPP>       **m_debug_ports;          /// From us to slave  (per mem port)

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
  typedef xtsc::xtsc_request::burst_t                   burst_t;
  typedef xtsc::xtsc_response                           xtsc_response;
  typedef sc_core::sc_signal<bool>                      bool_signal;
  typedef sc_core::sc_fifo<bool>                        bool_fifo;

  /// This SystemC macro inserts some code required for SC_THREAD's to work
  SC_HAS_PROCESS(xtsc_tlm2pin_memory_transactor);


  /// Our C++ type (SystemC uses this)
  virtual const char* kind() const { return "xtsc_tlm2pin_memory_transactor"; }


  /**
   * Constructor for a xtsc_tlm2pin_memory_transactor.
   *
   * @param     module_name             Name of the xtsc_tlm2pin_memory_transactor
   *                                    sc_module.
   * @param     tlm2pin_parms           The remaining parameters for construction.
   *
   * @see xtsc_tlm2pin_memory_transactor_parms
   */
  xtsc_tlm2pin_memory_transactor(sc_core::sc_module_name module_name, const xtsc_tlm2pin_memory_transactor_parms& tlm2pin_parms);


  /// Destructor.
  ~xtsc_tlm2pin_memory_transactor(void);


  /// For xtsc_connection_interface
  virtual xtsc::u32 get_bit_width(const std::string& port_name, xtsc::u32 interface_num = 0) const;


  /// For xtsc_connection_interface
  virtual sc_core::sc_object *get_port(const std::string& port_name);


  /// For xtsc_connection_interface
  virtual xtsc::xtsc_port_table get_port_table(const std::string& port_table_name) const;


  /// Return true if "dso_name" was provided
  bool has_dso() const;


  /// Dump pending response info
  void dump_pending_rsp_info(std::ostream& os);


  /// Return the number of pending responses (m_pending_rsp_info_cnt)
  xtsc::u64 get_pending_rsp_info_cnt(void);


  /**
   * Implementation of the xtsc::xtsc_command_handler_interface.
   *
   * This implementation supports the following commands:
   *  \verbatim
        dump_pending_rsp_info
          Return the buffer from calling dump_pending_rsp_info().

        get_pending_rsp_info_cnt
          Return get_pending_rsp_info_cnt().
      \endverbatim
   */
  void execute(const std::string&               cmd_line,
               const std::vector<std::string>&  words,
               const std::vector<std::string>&  words_lc,
               std::ostream&                    result);


  /**
   * Connect an xtsc_arbiter with this xtsc_tlm2pin_memory_transactor.
   *
   * This method connects the master port pair of the xtsc_arbiter with the specified
   * TLM slave port pair of this xtsc_tlm2pin_memory_transactor.
   *
   * @param     arbiter         The xtsc_arbiter to connect with this
   *                            xtsc_tlm2pin_memory_transactor.
   *
   * @param     port_num        The TLM slave port pair of this xtsc_tlm2pin_memory_transactor to
   *                            connect with the xtsc_arbiter.
   */
  void connect(xtsc_arbiter& arbiter, xtsc::u32 port_num = 0);


  /**
   * Connect an xtsc_core with this xtsc_tlm2pin_memory_transactor.
   *
   * This method connects the specified memory interface master port pair of the
   * specified xtsc_core with the specified TLM slave port pair of this
   * xtsc_tlm2pin_memory_transactor.
   *
   * @param     core                    The xtsc_core to connect with this
   *                                    xtsc_tlm2pin_memory_transactor.
   *
   * @param     memory_port_name        The name of the memory interface master port
   *                                    pair of the xtsc_core to connect with this
   *                                    xtsc_tlm2pin_memory_transactor. 
   *                                    Case-insensitive.
   *
   * @param     port_num                The slave port pair of this
   *                                    xtsc_tlm2pin_memory_transactor to connect the
   *                                    xtsc_core with.
   *
   * @param     single_connect          If true only one slave port pair of this device
   *                                    will be connected.  If false, the default, and
   *                                    if memory_port_name names the first port of an
   *                                    unconnected multi-ported interface of core and
   *                                    if port_num is 0 and if the number of ports this
   *                                    device has matches the number of multi-ports in
   *                                    the core interface, then all master port pairs
   *                                    of the core interface specified by
   *                                    memory_port_name will be connected to the slave
   *                                    port pairs of this xtsc_tlm2pin_memory_transactor.
   *
   * @see xtsc::xtsc_core::How_to_do_memory_port_binding for a list of valid
   *      memory_port_name values.
   *
   * @returns number of ports that were connected by this call (1 or 2)
   */
  xtsc::u32 connect(xtsc::xtsc_core& core, const char *memory_port_name, xtsc::u32 port_num = 0, bool single_connect = false);


  /**
   * Connect an xtsc_master with this xtsc_tlm2pin_memory_transactor.
   *
   * This method connects the master port pair of the xtsc_master with the TLM slave
   * port pair of this xtsc_tlm2pin_memory_transactor.
   *
   * @param     master          The xtsc_master to connect with this
   *                            xtsc_tlm2pin_memory_transactor.
   *
   * @param     port_num        The slave port pair of this
   *                            xtsc_tlm2pin_memory_transactor to connect the
   *                            xtsc_master with.
   */
  void connect(xtsc_master& master, xtsc::u32 port_num = 0);


  /**
   * Connect an xtsc_memory_trace with this xtsc_tlm2pin_memory_transactor.
   *
   * This method connects the specified master port pair of the specified upstream
   * xtsc_memory_trace with the specified TLM slave port pair of this
   * xtsc_tlm2pin_memory_transactor.  
   *
   * @param     memory_trace    The xtsc_memory_trace to connect with this
   *                            xtsc_tlm2pin_memory_transactor.
   *
   * @param     trace_port      The master port pair of the xtsc_memory_trace to connect
   *                            with this xtsc_tlm2pin_memory_transactor.
   *
   * @param     port_num        The TLM slave port pair of this
   *                            xtsc_tlm2pin_memory_transactor to connect the
   *                            xtsc_memory_trace with.
   *
   * @param     single_connect  If true only one TLM slave port pair of this
   *                            xtsc_tlm2pin_memory_transactor will be connected.  If
   *                            false, the default, then all contiguous, unconnected
   *                            slave port pairs of this xtsc_tlm2pin_memory_transactor
   *                            starting at port_num that have a corresponding existing
   *                            master port pair in memory_trace (starting at
   *                            trace_port) will be connected with that corresponding
   *                            memory_trace master port pair.
   *
   * @returns number of ports that were connected by this call (1 or more)
   */
  xtsc::u32 connect(xtsc_memory_trace& memory_trace, xtsc::u32 trace_port = 0, xtsc::u32 port_num = 0, bool single_connect = false);


  /**
   * Connect an xtsc_router with this xtsc_tlm2pin_memory_transactor.
   *
   * This method connects the specified master port pair of the specified xtsc_router
   * with the specified TLM slave port pair of this xtsc_tlm2pin_memory_transactor.
   *
   * @param     router          The xtsc_router to connect with this
   *                            xtsc_tlm2pin_memory_transactor.
   *
   * @param     router_port     The master port pair of the xtsc_router to connect with
   *                            this xtsc_tlm2pin_memory_transactor.  router_port must
   *                            be in the range of 0 to the xtsc_router's "num_slaves"
   *                            parameter minus 1.
   *
   * @param     port_num        The TLM slave port pair of this
   *                            xtsc_tlm2pin_memory_transactor to connect the
   *                            xtsc_router with.
   */
  void connect(xtsc_router& router, xtsc::u32 router_port, xtsc::u32 port_num = 0);


  virtual void reset(bool hard_reset = false);


  /**
   *  Handle nb_peek() calls.
   *
   *  This method can be overriden by a derived class to provide custom nb_peek handling
   *  when this device is used to connect to a Verilog module.
   */
  virtual void peek(xtsc::u32 port_num, xtsc::xtsc_address address8, xtsc::u32 size8, xtsc::u8 *buffer);


  /**
   *  Handle nb_poke() calls.
   *
   *  This method can be overriden by a derived class to provide custom nb_poke handling
   *  when this device is used to connect to a Verilog module.
   */
  virtual void poke(xtsc::u32 port_num, xtsc::xtsc_address address8, xtsc::u32 size8, const xtsc::u8 *buffer);


  /**
   *  Handle nb_fast_access() calls.
   *
   *  This method can be overriden by a derived class to provide custom nb_fast_access
   *  handling when this device is used to connect to a Verilog module.
   */
  virtual bool fast_access(xtsc::u32 port_num, xtsc::xtsc_fast_access_request &request);


  /// Return the interface type
  memory_interface_type get_interface_type() const { return m_interface_type; }


  /// Return the interface name string
  const char *get_interface_name() const { return xtsc_module_pin_base::get_interface_name(m_interface_type); }


  /// Return the number of memory ports this transactor has
  xtsc::u32 get_num_ports() const { return m_num_ports; }


  /// Return true if pin port names include the set_id as a suffix
  bool get_append_id() const { return m_append_id; }


protected:


  /// Information about each response
  class response_info {
  public:
    /// Constructor
    response_info(xtsc::xtsc_response          *p_response,
                  xtsc::u32                     bus_addr_bits,
                  xtsc::u32                     size,
                  bool                          is_read,
                  xtsc::u32                     id,
                  xtsc::u32                     route_id,
                  xtsc::u32                     num_transfers,
                  burst_t                       burst) :
      m_p_response      (p_response),
      m_bus_addr_bits   (bus_addr_bits),
      m_size            (size),
      m_id              (id),
      m_route_id        (route_id),
      m_num_transfers   (num_transfers),
      m_is_read         (is_read),
      m_copy_data       (false),
      m_status          (0),
      m_last            (true),
      m_burst           (burst)
    {}
    xtsc::xtsc_response        *m_p_response;                           ///< The response
    xtsc::u32                   m_bus_addr_bits;                        ///< Bits of xtsc_request address that identify byte lanes
    xtsc::u32                   m_size;                                 ///< Size from xtsc_request
    xtsc::u32                   m_id;                                   ///< PIF/IDMA0: request ID;       AXI/IDMA: pin-level transaction ID (AxID|xID)
    xtsc::u32                   m_route_id;                             ///< PIF/IDMA0: request Route ID; AXI/IDMA: not used
    xtsc::u32                   m_num_transfers;                        ///< PIF/IDMA0: not used;         AXI/IDMA: number of transfers
    bool                        m_is_read;                              ///< Expect read data in response
    bool                        m_copy_data;                            ///< True if data needs to be copied from m_buffer to response
    xtsc::u32                   m_status;                               ///< Response status
    bool                        m_last;                                 ///< True if actual response has last_transfer bit set
    burst_t                     m_burst;                                ///< AXI4 burst type of request
    xtsc::u8                    m_buffer[xtsc::xtsc_max_bus_width8];    ///< Buffer for non-last BLOCK_READ read data
  };

  // Shorthand aliases
  typedef std::deque<response_info*>                    rsp_info_deque;
  typedef std::map<xtsc::u64, rsp_info_deque*>          rsp_info_deque_map;

  /// SystemC callback
  virtual void before_end_of_elaboration(void);


  /// SystemC callback
  virtual void end_of_elaboration(void);


  /// SystemC callback
  virtual void start_of_simulation(void);


  /// SystemC callback
  virtual void end_of_simulation(void);


  /// Handle PIF|IDMA0 requests
  void pif_request_thread(void);


  /// Handle PIF|IDMA0 responses
  void pif_response_thread(void);


  /// PIF|IDMA0: Drive PORespRdy
  void pif_drive_resp_rdy_thread(void);


  /// Handle AXI4 requests
  void axi_request_thread(void);


  /// Handle AXI4 responses
  void axi_response_thread(void);


  /// AXI4: Drive RREADY|BREADY
  void axi_drive_resp_rdy_thread(void);


  /// Handle APB requests and responses
  void apb_thread(void);


  /// APB: helper method to drive all zeroes on the bus
  void apb_drive_zeroes(xtsc::u32 port);


  /// Handle local memory requests
  void lcl_request_thread(void);


  /// Thread to write response for 7stage configs
  void lcl_7stage_write_rsp_thread(void);


  /// Thread to sample local busy and/or send write response
  void lcl_busy_write_rsp_thread(void);


  /// Thread to sample local memory read data and respond
  void lcl_sample_read_data_thread(void);


  /// XLMI: Handle DPortLoadRetired pin
  void xlmi_retire_thread(void);


  /// XLMI: Handle DPortRetireFlush pin
  void xlmi_flush_thread(void);


  /// XLMI: Handle DPortLoad pin
  void xlmi_load_thread(void);


  /// XLMI: Help handle DPortLoad pin when interface has a busy pin
  void xlmi_load_method(void);


  /// DRAM: Handle DRamLock
  void dram_lock_thread(void);


  /// Log and send a local-memory or RSP_NACC response; also delete it if it is a last transfer
  void send_unchecked_response(xtsc::xtsc_response*& p_response, xtsc::u32 port);


  /// Get a new response_info (from the pool)
  response_info *new_response_info(xtsc::xtsc_response *p_response,
                                   xtsc::u32            bus_addr_bits,
                                   xtsc::u32            size,
                                   bool                 is_read         = false,
                                   xtsc::u32            id              = 0,
                                   xtsc::u32            route_id        = 0,
                                   xtsc::u32            num_transfers   = 1,
                                   burst_t              burst           = xtsc::xtsc_request::NON_AXI);


  /// Get a new response_info (from the pool) and initialize it by copying
  response_info *new_response_info(const response_info& info);


  /// Delete an response_info (return it to the pool)
  void delete_response_info(response_info*& p_response_info);


  /// Get a new xtsc_request (from the pool) that is a copy of the specified request
  xtsc::xtsc_request *new_request(const xtsc::xtsc_request& request);


  /// Delete an xtsc_request (return it to the pool)
  void delete_request(xtsc::xtsc_request*& p_request);


  /// Swizzle byte enables 
  void swizzle_byte_enables(xtsc::xtsc_byte_enables& byte_enables) const;


  /// Implementation of xtsc_request_if.
  class xtsc_request_if_impl : public xtsc::xtsc_request_if, public sc_core::sc_object {
  public:

    /// Constructor
    xtsc_request_if_impl(const char *object_name, xtsc_tlm2pin_memory_transactor& transactor, xtsc::u32 port_num);

    /// The kind of sc_object we are
    const char* kind() const { return "xtsc_tlm2pin_memory_transactor::xtsc_request_if_impl"; }

    /**
     *  Receive peeks from the memory interface master
     *  @see xtsc::xtsc_request_if
     */
    void nb_peek(xtsc::xtsc_address address8, xtsc::u32 size8, xtsc::u8 *buffer);

    /**
     *  Receive pokes from the memory interface master
     *  @see xtsc::xtsc_request_if
     */
    void nb_poke(xtsc::xtsc_address address8, xtsc::u32 size8, const xtsc::u8 *buffer);

    /**
     *  Receive requests for information about how to do fast access from the memory
     *  interface master
     *  @see xtsc::xtsc_request_if
     */
    bool nb_fast_access(xtsc::xtsc_fast_access_request &request);

    /**
     *  Receive requests from the memory interface master
     *  @see xtsc::xtsc_request_if
     */
    void nb_request(const xtsc::xtsc_request& request);

    /**
     *  For XLMI: DPortLoadRetired.
     *  @see xtsc::xtsc_request_if
     */
    void nb_load_retired(xtsc::xtsc_address address8);

    /**
     *  For XLMI: DPortRetireFlush.
     *  @see xtsc::xtsc_request_if
     */
    void nb_retire_flush();

    /**
     *  For DRamnLockm.
     *  @see xtsc::xtsc_request_if
     */
    void nb_lock(bool lock);

    /// Return true if a port has been bound to this implementation
    bool is_connected() { return (m_p_port != 0); }


  protected:

    /// SystemC callback when something binds to us
    void register_port(sc_core::sc_port_base& port, const char *if_typename);

    xtsc_tlm2pin_memory_transactor&     m_transactor;   ///< Our xtsc_tlm2pin_memory_transactor object
    sc_core::sc_port_base              *m_p_port;       ///< Port that is bound to us
    xtsc::u32                           m_port_num;     ///< Our port number
    xtsc::u32                           m_lock_port;    ///< For DRAM0BS|DRAM1BS, port num of the 0th subbank, otherwise same as m_port_num
    std::string                         m_lock_port_str;///< For DRAM0Bs|DRAM1BS: To add actual port to nb_lock logging 
  };



  /**
   * To cap an unconnected m_debug_ports port when the user can't bind anything to it.
   * For example, when connecting to RTL and a DSO cannot be provided (see "dso_name").
   */
  class xtsc_debug_if_cap : public xtsc::xtsc_debug_if {
  public:

    /// Constructor
    xtsc_debug_if_cap(xtsc_tlm2pin_memory_transactor& transactor, xtsc::u32 port_num) :
      m_transactor      (transactor),
      m_p_port          (0),
      m_port_num        (port_num)
    {}

    /**
     *  Receive peeks from the memory interface master
     *  @see xtsc::xtsc_debug_if
     */
    void nb_peek(xtsc::xtsc_address address8, xtsc::u32 size8, xtsc::u8 *buffer);

    /**
     *  Receive pokes from the memory interface master
     *  @see xtsc::xtsc_debug_if
     */
    void nb_poke(xtsc::xtsc_address address8, xtsc::u32 size8, const xtsc::u8 *buffer);

    /**
     *  Receive fast access requests from the memory interface master
     *  @see xtsc::xtsc_debug_if
     */
    bool nb_fast_access(xtsc::xtsc_fast_access_request &request);


  protected:

    /// SystemC callback when something binds to us
    void register_port(sc_core::sc_port_base& port, const char *if_typename);

    xtsc_tlm2pin_memory_transactor&     m_transactor;   ///< Our xtsc_tlm2pin_memory_transactor object
    sc_core::sc_port_base              *m_p_port;       ///< Port that is bound to us
    xtsc::u32                           m_port_num;     ///< Our port number
  };


#if !defined(_WIN32)
  typedef void *HMODULE;
#endif

  typedef void (*peek_t)(xtsc::u32 address8, xtsc::u32 size8,       xtsc::u8 *buffer, const char *dso_cookie, xtsc::u32 port);
  typedef void (*poke_t)(xtsc::u32 address8, xtsc::u32 size8, const xtsc::u8 *buffer, const char *dso_cookie, xtsc::u32 port);

  xtsc_request_if_impl        **m_request_impl;                 ///< m_request_exports binds to this (per mem port)
  xtsc_debug_if_cap           **m_debug_cap;                    ///< m_debug_ports binds to this if user can't (per mem port)
  xtsc::u32                     m_num_ports;                    ///< The number of memory ports this transactor has
  xtsc::u32                     m_num_axi_rd_ports;             ///< Number of AXI4 read  memory ports
  xtsc::u32                     m_num_axi_wr_ports;             ///< Number of AXI4 write memory ports
  xtsc::u32                     m_num_axi_sn_ports;             ///< Number of ACE snoop memory ports
  std::vector<xtsc::u32>        m_axi_signal_indx;              ///< Nth entry gives indx into AXI4 signal arrays (m_p_arid, etc) for mem port #N

  xtsc::xtsc_memory_b          *m_p_memory;                     ///< Optional shadow memory
  HMODULE                       m_dso;                          ///< from dlopen/LoadLibrary of m_dso_name
  peek_t                        m_peek;                         ///< Function pointer to the peek symbol in m_dso
  poke_t                        m_poke;                         ///< Function pointer to the poke symbol in m_dso

  std::deque<xtsc_request*>    *m_request_fifo;                 ///< The fifo of incoming requests (per mem port)
  xtsc::u32                     m_request_fifo_depth;           ///< From "request_fifo_depth" parameter
  std::string                   m_interface_uc;                 ///< Uppercase version of "memory_interface" parameter
  std::string                   m_interface_lc;                 ///< Lowercase version of "memory_interface" parameter
  memory_interface_type         m_interface_type;               ///< The memory interface type
  xtsc::u32                     m_width8;                       ///< Data width in bytes of the memory interface
  xtsc::u32                     m_dram_attribute_width;         ///< See "dram_attribute_width" parameter
  sc_dt::sc_unsigned            m_dram_attribute;               ///< To retrieve value using xtsc_request::get_dram_attribute()
  sc_dt::sc_bv_base             m_dram_attribute_bv;            ///< To convert sc_unsigned m_dram_attribute to sc_bv_base
  xtsc::xtsc_address            m_start_byte_address;           ///< Number to be subtracted from the TLM request address
  xtsc::u64                     m_clock_period_value;           ///< This device's clock period as u64
  sc_core::sc_time              m_clock_period;                 ///< This device's clock period as sc_time
  sc_core::sc_time              m_time_resolution;              ///< The SystemC time resolution
  sc_core::sc_time              m_posedge_offset;               ///< From "posedge_offset" parameter
  sc_core::sc_time              m_sample_phase;                 ///< Clock phase for sampling inputs and deasserting outputs
  sc_core::sc_time              m_sample_phase_plus_one;        ///< m_sample_phase plus one clock period
  sc_core::sc_time              m_output_delay;                 ///< See "output_delay" parameter
  sc_core::sc_time             *m_retire_deassert;              ///< Time at which XLMI retire should be deasserted (per mem port)
  sc_core::sc_time             *m_flush_deassert;               ///< Time at which XLMI flush should be deasserted (per mem port)
  sc_core::sc_time              m_read_delay_time;              ///< See "read_delay" parameter
  xtsc::u32                     m_read_delay;                   ///< See "read_delay" parameter
  xtsc::u64                     m_posedge_offset_value;         ///< m_posedge_offset as u64
  bool                          m_is_7_stage;                   ///< If "read_delay" is 1
  bool                          m_has_posedge_offset;           ///< True if m_posedge_offset is non-zero
  bool                          m_cosim;                        ///< See "cosim" parameter
  bool                          m_shadow_memory;                ///< See "shadow_memory" parameter
  bool                          m_test_shadow_memory;
  bool                          m_cbox;                         ///< See "cbox" parameter
  bool                          m_split_rw;                     ///< True if m_interface_type is DRAM0RW or DRAM1RW
  bool                          m_has_dma;                      ///< See "has_dma" parameter
  bool                          m_external_udma;                ///< See "external_udma" parameter
  bool                          m_append_id;                    ///< True if pin port names should include the set_id.
  bool                          m_inbound_pif;                  ///< True if interface is inbound PIF
  bool                          m_snoop;                        ///< True if interface is snoop port.  See "snoop" parameter.
  bool                          m_has_coherence;                ///< See "has_coherence" parameter
  bool                          m_has_pif_attribute;            ///< See "has_pif_attribute" parameter
  bool                          m_has_pif_req_domain;           ///< See "has_pif_req_domain" parameter
  bool                          m_big_endian;                   ///< True if the memory interface master is big endian
  bool                          m_write_responses;              ///< See "write_responses" parameter (PIF|IDMA0 only)
  bool                          m_track_write_requests;         ///< See "track_write_requests" parameter (PIF only)
  bool                          m_discard_unknown_responses;    ///< See "discard_unknown_responses" parameter (PIF only)
  bool                          m_has_request_id;               ///< True if the "POReqId" and "PIRespId" ports should be present
  std::vector<xtsc::u32>        m_deny_fast_access;             ///< For turboxim.  See "deny_fast_access".
  std::vector<xtsc::u32>        m_axi_id_bits;                  ///< See "axi_id_bits" parameter.
  std::vector<xtsc::u32>        m_axi_route_id_bits;            ///< See "axi_route_id_bits" parameter.
  std::vector<xtsc::u32>        m_axi_read_write;               ///< See "axi_read_write" parameter. [N]=0 => Port #N is a read port, 1 => write port
  std::vector<xtsc::u32>        m_axi_port_type;                ///< See "axi_port_type" parameter.
  std::vector<std::string>      m_axi_name_prefix;              ///< See "axi_name_prefix" parameter
  std::string                   m_dso_name;                     ///< See "dso_name" parameter
  const char                   *m_dso_cookie;                   ///< See "dso_cookie" parameter
  const char                   *m_initial_value_file;           ///< See "initial_value_file" parameter
  std::string                   m_req_user_data;                ///< See "req_user_data" parameter
  std::string                   m_req_user_data_name;           ///< Name of request user data port
  xtsc::u32                     m_req_user_data_width1;         ///< Bit width of request user data port
  std::string                   m_rsp_user_data;                ///< See "rsp_user_data" parameter
  std::string                   m_rsp_user_data_name;           ///< Name of response user data port
  xtsc::u32                     m_rsp_user_data_width1;         ///< Bit width of response user data port
  sc_dt::sc_bv_base            *m_user_data_val;                ///< Value for req_user_data port
  xtsc::u8                      m_memory_fill_byte;             ///< See "memory_fill_byte" parameter
  xtsc::u32                     m_address_bits;                 ///< Number of bits in the address (non-PIF|IDMA0 only)
  xtsc::u32                     m_check_bits;                   ///< Number of bits in ECC/parity signals (from "check_bits")
  xtsc::u32                     m_ram_select_bit;               ///< See "ram_select_bit" parameter
  bool                          m_ram_select_normal;            ///< See "ram_select_normal" parameter
  xtsc::u32                     m_ram_select_mask;              ///< Mask with all bits 0 except m_ram_select_bit
  xtsc::xtsc_address            m_address_mask;                 ///< Address mask
  xtsc::xtsc_address            m_bus_addr_bits_mask;           ///< Address mask to get bits which indicate which byte lane
  xtsc::u32                     m_address_shift;                ///< Number of bits to right-shift the address
  xtsc::u32                     m_route_id_bits;                ///< Number of bits in the route ID (PIF|IDMA0 only)
  xtsc::u32                     m_route_id_mask;                ///< Mask to get bits for m_route_id
  xtsc::u32                     m_pif_id_mask;                  ///< Mask to get bits of PIF request ID
  xtsc::u32                     m_next_port_pif_request_thread;         ///< To give each thread instance a port number
  xtsc::u32                     m_next_port_pif_response_thread;        ///< To give each thread instance a port number
  xtsc::u32                     m_next_port_pif_drive_resp_rdy_thread;  ///< To give each thread instance a port number
  xtsc::u32                     m_next_port_axi_request_thread;         ///< To give each thread instance a port number
  xtsc::u32                     m_next_port_axi_response_thread;        ///< To give each thread instance a port number
  xtsc::u32                     m_next_port_axi_drive_resp_rdy_thread;  ///< To give each thread instance a port number
  xtsc::u32                     m_next_port_apb_request_thread;         ///< To give each thread instance a port number
  xtsc::u32                     m_next_port_lcl_request_thread;         ///< To give each thread instance a port number
  xtsc::u32                     m_next_port_xlmi_retire_thread;         ///< To give each thread instance a port number
  xtsc::u32                     m_next_port_xlmi_flush_thread;          ///< To give each thread instance a port number
  xtsc::u32                     m_next_port_xlmi_load_thread;           ///< To give each thread instance a port number
  xtsc::u32                     m_next_port_dram_lock_thread;           ///< To give each thread instance a port number
  xtsc::u32                     m_next_port_lcl_busy_write_rsp_thread;  ///< To give each thread instance a port number
  xtsc::u32                     m_next_port_lcl_7stage_write_rsp_thread;///< To give each thread instance a port number
  xtsc::u32                     m_next_port_lcl_sample_read_data_thread;///< To give each thread instance a port number
  bool                          m_has_busy;                     ///< True if memory interface has a busy/rdy pin (non-PIF|IDMA0|AXI|IDMA|APB only)
  bool                          m_has_lock;                     ///< True if memory interface has a lock pin 
                                                                ///<  (DRAM0|DRAM0BS|DRAM0RW|DRAM1|DRAM1BS|DRAM1RW only)
  bool                          m_has_xfer_en;                  ///< True if memory interface has Xfer enable pin 
                                                                ///<  (NA PIF|IDMA0|AXI|IDMA|APB|DROM0|XLMI0)
  sc_core::sc_event            *m_write_response_event;         ///< Event used to notify pif_response_thread (per mem port)
  sc_core::sc_event            *m_request_event;                ///< Event used to notify request_thread (per mem port)
  sc_core::sc_event            *m_retire_event;                 ///< Event used to notify xlmi_retire_thread (per mem port)
  sc_core::sc_event            *m_flush_event;                  ///< Event used to notify xlmi_flush_thread (per mem port)
  sc_dt::sc_uint_base           m_address;                      ///< The address after any required shifting and masking
  sc_dt::sc_uint_base           m_vadrs;                        ///< SnoopReqCohVadrsIndex
  sc_dt::sc_uint_base           m_req_coh_cntl;                 ///< POReqCohCntl;
  sc_dt::sc_uint_base           m_lane;                         ///< Byte/Word enables
  sc_dt::sc_uint_base           m_id;                           ///< POReqId|PIRespId
  sc_dt::sc_uint_base           m_priority;                     ///< POReqPriority|PIRespPriority|AxQOS
  sc_dt::sc_uint_base           m_route_id;                     ///< POReqRouteId|PIRespRouteId
  sc_dt::sc_uint_base           m_req_attribute;                ///< POReqAttribute|PIReqAttribute
  sc_dt::sc_uint_base           m_req_domain;                   ///< POReqDomain
  sc_dt::sc_uint_base           m_len;                          ///< AxLEN
  sc_dt::sc_uint_base           m_size;                         ///< AxSIZE
  sc_dt::sc_uint_base           m_burst;                        ///< AxBURST
  sc_dt::sc_uint_base           m_cache;                        ///< AxCACHE
  sc_dt::sc_uint_base           m_prot;                         ///< AxPROT|PPROT
  sc_dt::sc_uint_base           m_bar;                          ///< AxBAR
  sc_dt::sc_uint_base           m_domain;                       ///< AxDOMAIN
  sc_dt::sc_bv_base             m_data;                         ///< Read/write data
  req_cntl                      m_req_cntl;                     ///< Value for POReqCntrl
  bool_fifo                   **m_resp_rdy_fifo;                ///< sc_fifo to keep track of PORespRdy|xREADY pin (per mem port)
  sc_core::sc_event            *m_drive_resp_rdy_event;         ///< Notify when PORespRdy|xREADY should be driven (per mem port)
  std::vector<xtsc_request*>    m_request_pool;                 ///< Maintain a pool of requests to improve performance
  std::vector<response_info*>   m_response_pool;                ///< Maintain a pool of responses to improve performance
  std::deque<response_info*>   *m_busy_write_rsp_deque;         ///< pending responses: check busy and/or send write rsp (per mem port)
  std::deque<response_info*>   *m_7stage_write_rsp_deque;       ///< pending responses: send write rsp (per mem port)  7 stage only
  std::deque<response_info*>   *m_read_data_rsp_deque;          ///< deque of pending read responses (per mem port)
  std::deque<response_info*>   *m_system_response_deque;        ///< deque of pending PIF|IDMA0|AXI|IDMA responses (per mem port)
  std::deque<response_info*>   *m_write_response_deque;         ///< deque of pending generated PIF|IDMA0 write responses (per mem port)
  std::deque<bool>             *m_lock_deque;                   ///< deque of pending DRamLock values (per mem port)
  std::deque<bool>             *m_load_deque;                   ///< deque of pending DPortLoad/m_p_preload values (per mem port)
  bool                         *m_previous_response_last;       ///< true if previous response was a last transfer (per mem port)
  bool                         *m_hard_busy;                    ///< Force AXI4 write request port busy due to half-accepted request (per mem port)
  xtsc::u32                    *m_transfer_number;              ///< For xtsc_response::set_transfer_number()      (per mem port)
  sc_core::sc_event_queue      *m_busy_write_rsp_event_queue;   ///< When busy should be sampled and/or write rsp sent (per mem port)
  sc_core::sc_event_queue      *m_7stage_write_rsp_event_queue; ///< When write rsp should be sent (per mem port)  7 stage only
  sc_core::sc_event_queue      *m_read_data_event_queue;        ///< When read data should be sampled (per mem port)
  sc_core::sc_event_queue      *m_lock_event_queue;             ///< When DRamLock should be driven (per mem port)
  sc_core::sc_event_queue      *m_load_event_queue;             ///< When DPortLoad/m_p_preload should be driven (per mem port)
  log4xtensa::TextLogger&       m_text;                         ///< Used for logging 
  bool_signal                 **m_p_preload;                    ///< DPortLoad prior to qualification with the busy pin

  // PIF|IDMA0|inbound PIF|Snoop request channel pins (per mem port)
  bool_output                 **m_p_req_valid;                  ///< POReqValid
  uint_output                 **m_p_req_cntl;                   ///< POReqCntl
  uint_output                 **m_p_req_adrs;                   ///< POReqAdrs
  wide_output                 **m_p_req_data;                   ///< POReqData
  uint_output                 **m_p_req_data_be;                ///< POReqDataBE
  uint_output                 **m_p_req_id;                     ///< POReqId
  uint_output                 **m_p_req_priority;               ///< POReqPriority
  uint_output                 **m_p_req_route_id;               ///< POReqRouteId
  uint_output                 **m_p_req_attribute;              ///< POReqAttribute
  uint_output                 **m_p_req_domain;                 ///< POReqDomain
  uint_output                 **m_p_req_coh_vadrs;              ///< POReqCohVAdrsIndex/SnoopReqCohVAdrsIndex
  uint_output                 **m_p_req_coh_cntl;               ///< POReqCohCntl/SnoopReqCohCntl
  wide_output                 **m_p_req_user_data;              ///< Request User Data.  See "req_user_data" parameter.
  bool_input                  **m_p_req_rdy;                    ///< PIReqRdy
  
  // PIF|IDMA0|inbound PIF|Snoop response channel pins (per mem port)
  bool_input                  **m_p_resp_valid;                 ///< PIRespValid
  uint_input                  **m_p_resp_cntl;                  ///< PIRespCntl
  wide_input                  **m_p_resp_data;                  ///< PORespData
  uint_input                  **m_p_resp_id;                    ///< PIRespId
  uint_input                  **m_p_resp_priority;              ///< PIRespPriority
  uint_input                  **m_p_resp_route_id;              ///< PIRespRouteId
  uint_input                  **m_p_resp_coh_cntl;              ///< PIRespCohCntl/SnoopRespCohCntl
  wide_input                  **m_p_resp_user_data;             ///< Response User Data.  See "rsp_user_data" parameter.
  bool_output                 **m_p_resp_rdy;                   ///< PORespRdy
  
  // AXI4 read address channel pins (per mem read port)
  uint_output                 **m_p_arid;                       ///< ARID       4|8  "axi_id_bits"   "axi_route_id_bits"
  uint_output                 **m_p_araddr;                     ///< ARADDR     32
  uint_output                 **m_p_arlen;                      ///< ARLEN      8
  uint_output                 **m_p_arsize;                     ///< ARSIZE     3
  uint_output                 **m_p_arburst;                    ///< ARBURST    2
  bool_output                 **m_p_arlock;                     ///< ARLOCK     1
  uint_output                 **m_p_arcache;                    ///< ARCACHE    4
  uint_output                 **m_p_arprot;                     ///< ARPROT     3
  uint_output                 **m_p_arqos;                      ///< ARQOS      4
  uint_output                 **m_p_arbar;                      ///< ARBAR      2
  uint_output                 **m_p_ardomain;                   ///< ARDOMAIN   2
  uint_output                 **m_p_arsnoop;                    ///< ARSNOOP    4
  bool_output                 **m_p_arvalid;                    ///< ARVALID    1
  bool_input                  **m_p_arready;                    ///< ARREADY    1

  // AXI4 read response channel pins (per mem read port)
  uint_input                  **m_p_rid;                        ///< RID        4|8  "axi_id_bits"   "axi_route_id_bits"
  wide_input                  **m_p_rdata;                      ///< RDATA      32|64|128|256|512    "byte_width"
  uint_input                  **m_p_rresp;                      ///< RRESP      2|4                  "axi_port_type"
  bool_input                  **m_p_rlast;                      ///< RLAST      1
  bool_input                  **m_p_rvalid;                     ///< RVALID     1
  bool_output                 **m_p_rready;                     ///< RREADY     1

  // AXI4 write address channel pins (per mem write port)
  uint_output                 **m_p_awid;                       ///< AWID       4|8  "axi_id_bits"   "axi_route_id_bits"
  uint_output                 **m_p_awaddr;                     ///< AWADDR     32
  uint_output                 **m_p_awlen;                      ///< AWLEN      8
  uint_output                 **m_p_awsize;                     ///< AWSIZE     3
  uint_output                 **m_p_awburst;                    ///< AWBURST    2
  bool_output                 **m_p_awlock;                     ///< AWLOCK     1
  uint_output                 **m_p_awcache;                    ///< AWCACHE    4
  uint_output                 **m_p_awprot;                     ///< AWPROT     3
  uint_output                 **m_p_awqos;                      ///< AWQOS      4
  uint_output                 **m_p_awbar;                      ///< AWBAR      2
  uint_output                 **m_p_awdomain;                   ///< AWDOMAIN   2
  uint_output                 **m_p_awsnoop;                    ///< AWSNOOP    3
  bool_output                 **m_p_awvalid;                    ///< AWVALID    1
  bool_input                  **m_p_awready;                    ///< AWREADY    1

  // AXI4 write data channel pins (per mem write port)
  wide_output                 **m_p_wdata;                      ///< WDATA      32|64|128|256|512    "byte_width"
  uint_output                 **m_p_wstrb;                      ///< WSTRB      4|8|16|32|64         "byte_width" / 8
  bool_output                 **m_p_wlast;                      ///< WLAST      1
  bool_output                 **m_p_wvalid;                     ///< WVALID     1
  bool_input                  **m_p_wready;                     ///< WREADY     1

  // AXI4 write response channel pins (per mem write port)
  uint_input                  **m_p_bid;                        ///< BID        4|8  "axi_id_bits"   "axi_route_id_bits"
  uint_input                  **m_p_bresp;                      ///< BRESP      2
  bool_input                  **m_p_bvalid;                     ///< BVALID     1
  bool_output                 **m_p_bready;                     ///< BREADY     1

  // ACE snoop address channel pins (per mem snoop port)
  uint_output                 **m_p_acaddr;                     ///< ACADDR     32
  uint_output                 **m_p_acprot;                     ///< ACPROT     3
  uint_output                 **m_p_acsnoop;                    ///< ACSNOOP    4
  bool_output                 **m_p_acvalid;                    ///< ACVALID    1
  bool_input                  **m_p_acready;                    ///< ACREADY    1

  // ACE snoop data channel pins (per mem snoop port)
  wide_input                  **m_p_cddata;                     ///< CDDATA     32|64|128|256|512    "byte_width"
  bool_input                  **m_p_cdlast;                     ///< CDLAST     1
  bool_input                  **m_p_cdvalid;                    ///< CDVALID    1
  bool_output                 **m_p_cdready;                    ///< CDREADY    1

  // ACE snoop response channel pins (per mem snoop port)
  uint_input                  **m_p_crresp;                     ///< RRESP      2|4                  "axi_port_type"
  bool_input                  **m_p_crvalid;                    ///< CRVALID    1
  bool_output                 **m_p_crready;                    ///< CRREADY    1

  // APB pins (per mem port)
  bool_output                 **m_p_psel;                       ///< PSEL       1
  bool_output                 **m_p_penable;                    ///< PENABLE    1
  bool_output                 **m_p_pwrite;                     ///< PWRITE     1
  uint_output                 **m_p_paddr;                      ///< PADDR      32
  uint_output                 **m_p_pprot;                      ///< PPROT      3
  uint_output                 **m_p_pstrb;                      ///< PSTRB      4
  wide_output                 **m_p_pwdata;                     ///< PWDATA     32
  bool_input                  **m_p_pready;                     ///< PREADY     1
  bool_input                  **m_p_pslverr;                    ///< PSLVERR    1
  wide_input                  **m_p_prdata;                     ///< PRDATA     32

  // Local Memory pins (per mem port)
  bool_output                 **m_p_en;                         ///< DPortEn, DRamEn, DRomEn, IRamEn, IRomEn
  uint_output                 **m_p_addr;                       ///< DPortAddr, DRamAddr, DRomAddr, IRamAddr, IRomAddr
  uint_output                 **m_p_lane;                       ///< DPortByteEn, DRamByteEn, DRomByteEn, IRamWordEn, IRomWordEn
  wide_output                 **m_p_wrdata;                     ///< DPortWrData, DRamWrData, IRamWrData
  bool_output                 **m_p_wr;                         ///< DPortWr, DRamWr, IRamWr
  bool_output                 **m_p_load;                       ///< DPortLoad, IRamLoadStore, IRomLoad
  bool_output                 **m_p_retire;                     ///< DPortLoadRetired
  bool_output                 **m_p_flush;                      ///< DPortRetireFlush
  bool_output                 **m_p_lock;                       ///< DRamLock (per bank if subbanked)
  wide_output                 **m_p_attr;                       ///< DRamAttr, DRamWrAttr
  wide_output                 **m_p_check_wr;                   ///< DRamCheckWrData, IRamCheckWrData
  wide_input                  **m_p_check;                      ///< DRamCheckData, IRamCheckData
  bool_output                 **m_p_xfer_en;                    ///< DRamXferEn, IRamXferEn, IRomXferEn, URamXferEn
  bool_input                  **m_p_busy;                       ///< DPortBusy, DRamBusy, DRomBusy, IRamBusy, IRomBusy (per bank if subbanked)
  wide_input                  **m_p_data;                       ///< DPortData, DRamData, DRomData, IRamData, IRomData
  bool_output                 **m_p_ram0;                       ///< DmaDRam0
  bool_output                 **m_p_ram1;                       ///< DmaDRam1
  
  rsp_info_deque_map           *m_tran_id_rsp_info;             ///< Storage for pending responses (per mem port) (PIF|IDMA0|AXI|IDMA only)
                                                                ///< Map transaction ID to deque of response_info objects
  xtsc::u64                     m_pending_rsp_info_cnt;         ///< Count of entries in m_tran_id_rsp_info

  std::vector<sc_core::sc_process_handle>
                                m_process_handles;              ///< For reset 

  static const xtsc::u32        AXI4     = 1;                   ///< For "axi_port_type" entry values
  static const xtsc::u32        ACE_LITE = 2;                   ///< For "axi_port_type" entry values
  static const xtsc::u32        ACE      = 3;                   ///< For "axi_port_type" entry values

};  // class xtsc_tlm2pin_memory_transactor 






}  // namespace xtsc_component



#endif // _XTSC_TLM2PIN_MEMORY_TRANSACTOR_H_
