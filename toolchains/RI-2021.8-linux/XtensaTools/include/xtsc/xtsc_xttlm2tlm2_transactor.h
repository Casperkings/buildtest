#ifndef _XTSC_XTTLM2TLM2_TRANSACTOR_H_
#define _XTSC_XTTLM2TLM2_TRANSACTOR_H_

// Copyright (c) 2006-2018 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
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


#include <tlm.h>
#include <tlm_utils/peq_with_get.h>
#include <xtsc/xtsc.h>
#include <xtsc/xtsc_parms.h>
#include <xtsc/xtsc_request_if.h>
#include <xtsc/xtsc_respond_if.h>
#include <xtsc/xtsc_request.h>
#include <xtsc/xtsc_response.h>
#include <xtsc/xtsc_fast_access.h>
#include <xtsc/xtsc_core.h>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <cstring>




namespace xtsc_component {


// Forward references
class xtsc_arbiter;
class xtsc_master;
class xtsc_memory_trace;
class xtsc_router;


/**
 * Constructor parameters for a xtsc_xttlm2tlm2_transactor object.
 *
 *  \verbatim
   Name                  Type   Description
   ------------------    ----   --------------------------------------------------------
  
   "memory_interface"    char*  The memory interface type.  Valid values are "DRAM0",
                                "DRAM1", "DROM0", "IRAM0", "IRAM1", "IROM0", "URAM0",
                                "XLMI0", "PIF", "AXI", "IDMA0" (LX), "IDMA" (NX), and
                                "APB" (case-insensitive).

   "num_ports"           u32    The number of Xtensa TLM (xttlm) port pairs this
                                transactor has as well as the number of TLM2 sockets
                                this transactor has.
                                Default = 1.
                                Minimum = 1.

   "byte_width"          u32    Bus width in bytes.  Valid values are 4, 8, 16, 32, and
                                64.  If byte_width is 64 then only READ/WRITE request
                                and AXI BURST_READ/BURST_WRITE requests are allowed.
                                The exclusive-to-PIF transactions (BLOCK_READ, RCW,
                                and BLOCK_WRITE, and PIF BURST_READ/BURST_WRITE)
                                are only allowed if byte_width is 4|8|16|32.
  
   "request_fifo_depth"   u32   The request fifo depth.  A value of 0 means to not use
                                a request fifo which causes the incoming request channel
                                to be blocked as soon as one request is accepted until
                                that request is completed.
                                If "use_nb_transport" is true, then "request_fifo_depth"
                                must be non-zero.
                                Default = 1.

   "max_burst_beats"     u32    This parameter specifies the maximum number of transfers
                                (beats) that will be allowed on PIF BURST_READ or
                                BURST_WRITE transactions.  The original PIF Protocol 3.1
                                allowed up to 8 transfers.  The enhanced PIF burst of
                                Xtensa iDMA hardware in RG-2017.8 and later allowed up
                                to 16 transfers.
                                Default = 8.

   "use_nb_transport"    bool   If false, then b_transport() will be used which means
                                each request on a given port is handled one-at-a-time
                                to completion.  If true, then nb_transport_fw() will be
                                used to send requests downstream which allows for
                                multiple requests per port to be downstream at the same
                                time.  
                                If "use_nb_transport" is true, then "immediate_timing"
                                must be false and "request_fifo_depth" must be non-zero.
                                Default = false.

   "turbo_switch"        bool   When "use_nb_transport" is true, then this parameter can
                                be set to true to indicate that this transactor will use
                                nb_transport when the cycle-accurate ISS is active but
                                will use b_transport when TurboXim is active.  When
                                "use_nb_transport" is false, then this parameter must
                                also be false.
                                Default = false.

   "revoke_on_dmi_hint"  bool   If both "turbo_switch" and "revoke_on_dmi_hint" are
                                true, then TurboXim fast access will be revoked for the
                                address range of any transaction that returns from
                                b_transport() with a DMI hint, that is if
                                tlm_generic_payload::is_dmi_allowed() returns true.
                                When "turbo_switch" is false, then this parameter must
                                also be false.
                                Default = false.
                                Note: The "revoke_on_dmi_hint" parameter is an
                                      experimental feature.

   Note: A TLM2 transaction is considered a burst transfer transaction if the generic
         payload data length attribute is greater then the BUSWIDTH template parameter
         of the socket.

   "use_tlm2_burst"      bool   If true, then a single TLM2 burst transfer transaction
                                will be used for BLOCK_READ, BURST_READ, BLOCK_WRITE,
                                and BURST_WRITE PIF transactions and for non-FIXED AXI
                                (WRAP/INCR BURST_READ/BURST_WRITE) transactions.  If
                                false, then multiple non-burst TLM2 single transactions
                                will be used for these transactions.
                                Caution: PIF protocol requires that the multiple
                                         requests of BLOCK_WRITE and BURST_WRITE
                                         transactions be integral (not be interspersed
                                         with other requests).  PIF 3 protocol requires
                                         that the multiple responses of BLOCK_READ and
                                         BURST_READ transactions be integral (not be
                                         interspersed with other responses).  If this
                                         parameter is set to false then the user should
                                         ensure that the downstream TLM2 subsystem can
                                         preserve the integrity of the multiple request
                                         sequence used for BLOCK_WRITE|BURST_WRITE and,
                                         if PIF 3 is being used, the multiple response
                                         sequence used for BLOCK_READ|BURST_READ.
                                Default = true.
  
   Note: A TLM2 transaction is considered a burst transfer transaction if the generic
         payload data length attribute is greater then the BUSWIDTH template parameter
         of the socket.  When performing non-FIXED (that is, WRAP or INCR) AXI burst
         transactions, the setting of "use_tlm2_burst" will also be considered when
         deciding whether or not to use multiple calls to the downstream TLM2 subsystem
         even if the total length of the transaction does not exceed BUSWIDTH.

   "use_tlm2_busy"       bool   If true, then a TLM_INCOMPLETE_RESPONSE will be treated
                                as a busy signal (similar to RSP_NACC) and the TLM2
                                transport call will be repeated after a delay of 
                                one clock period.  If false, then an exception will
                                be thrown if a TLM_INCOMPLETE_RESPONSE is received.
                                This parameter only applies when using b_transport(),
                                if "use_nb_transport" is true, then "use_tlm2_busy"
                                is ignored.
                                Default = true.
  
   "rcw_support"         u32    This parameter specifies how the model should support
                                the RCW transaction.  The legal values and their meaning
                                are:
                                 0 = Use the transport_dbg() interface.  
                                     Note: If this option is selected then RCW requests
                                           may not have any disabled byte lanes.
                                 1 = Use the b_transport() interface and throw an
                                     exception if the read and write beats do not
                                     occur at the same time and in the same delta cycle.
                                 2 = Use the b_transport() interface and log a warning
                                     if the read and write beats do not occur at the
                                     same time and in the same delta cycle.
                                 3 = Use the b_transport() interface and log at
                                     INFO_LOG_LEVEL if the read and write beats do not
                                     occur at the same time and in the same delta cycle.
                                Note: This model will make the read and write calls to
                                      b_transport() back-to-back (without an intervening
                                      wait() call) however, the downstream TLM2
                                      sub-system might insert calls to wait() inside the
                                      b_transport() call and this violates the atomicity
                                      of the RCW sequence.
                                Caution: Options 2 and 3 are not recommended.
                                Caution: Option 0 is not recommended if the memory has
                                         read/write side effects that transport_dbg()
                                         won't trigger.
                                Default = 1.
  
   "turbo_support"       u32    This parameter specifies how the model should support
                                TurboXim fast access.  The legal values and their
                                meaning are:
                                 0 = Do not allow TurboXim fast access for load/store.
                                 1 = Give TurboXim peek/poke fast access to all
                                     addresses except those specified by
                                     "deny_fast_access".
                                 2 = Give TurboXim raw access to all addresses allowed
                                     by the get_direct_mem_ptr() method except those
                                     specified by "deny_fast_access".
                                Caution: Options 1 and 2 are not recommended if the
                                         target memory has side effects.
                                Note: If TurboXim is denied fast access then it will use
                                      non-fast access (XTSC:nb_request to
                                      TLM2:b_transport) for loads and stores; however,
                                      for IFetch it will use XTSC:nb_peek to
                                      TLM2:transport_dbg.
                                Default = 2.

   "deny_fast_access"  vector<u64>   A std::vector containing an even number of
                                addresses.  Each pair of addresses specifies a range of
                                addresses that will be denied fast access.  The first
                                address in each pair is the start address and the second
                                address in each pair is the end address.  This parameter
                                is ignored if "turbo_support" is 0.

   "enable_extensions"    bool  If true, this transactor adds additional transaction 
                                attributes from the inbound PIF/AXI as ignorable TLM2
                                extensions to the outbound TLM2 payload. The attributes
                                that are currently supported to be added are :
                                xtsc_request's transaction_id.
                                Default = false. 

   "exclusive_support"    u32   This parameter specifies how the model should support
                                exclusive requests. The legal values and their meaning
                                are:
                                0 = Not supported, throw exception.
                                1 = Not supported, ignore request.
                                2 = Supported with TLM2 ignorabled extensions. This 
                                    option requires that the downstream model has the 
                                    support for these same TLM2 extensions as used by
                                    this transactor. Otherwise the downstream model 
                                    would just interpret the exclusive transaction as 
                                    a normal transaction.
                                Default = 0.

   "ignore_exclusive"     bool  If true, the exclusive setting of requests will be
                                ignored.  If false, the default, an exclusive request
                                will cause an exception to be thrown.
                                NOTE: This parameter is deprecated and will be removed.
                                Use parameter 'exclusive_support' to achieve the same 
                                functionality.
                                Default = false.

   "immediate_timing"     bool  If true, the transactor attempts to interact with the
                                rest of the system without consuming any SystemC time or
                                delta cycles except as necessary to ensure responses are
                                never sent to the upstream Xtensa TLM subsystem with
                                less then one clock period in between them and that
                                requests that get busied by the downstream OSCI TLM2
                                subsystem are only repeated after a wait of one clock
                                period.  Also, the time argument returned in the
                                b_transport called is ignored.  If false, the transactor
                                does a SystemC wait for a duration equal to the time
                                argument returned by each b_transport call.  In
                                addition, each nb_request call takes one delta cycle to
                                notify worker_thread.
                                If "use_nb_transport" is true, then "immediate_timing"
                                must be false.
                                Default = false.

   "clock_period"        u32    This is the length of this transactor's clock period
                                expressed in terms of the SystemC time resolution
                                (from sc_get_time_resolution()).  A value of 
                                0xFFFFFFFF means to use the XTSC system clock period
                                (from xtsc_get_system_clock_period()).  A value of 0
                                means one delta cycle.
                                Default = 0xFFFFFFFF (i.e. use the system clock period).

   "split_tlm2_phases"   bool   If set to true, the transactor allows sending and receiving
                                request's data transfers individually, as soon as they are 
                                available, using additional phases. If enabled, transactor   
                                makes use of TLM2 ignorable phases but they are not really 
                                ignorable, because they expect the target to update the 
                                phase to next stage, ie return TLM_UPDATED rather than 
                                just TLM_ACCEPTED. This handshake between the transactor
                                and target tlm2 model is required to control the flow of 
                                data, similar to RSP_NACC in PIF or AxREADY in AXI. This 
                                feature provides more timing accuracy but must ONLY BE 
                                ENABLED IF the downstream TLM2 target is able to comprehend
                                and respond to these additional phases. Advancing transaction
                                with TLM_COMPLETED when the request is flowing through 
                                different stages is forbidded. For different transfers, the 
                                same TLM2 generic payload is used and data is placed at its
                                respective index in the payload, upon arrival. The data 
                                length in payload at any time is the total length of bytes 
                                transferred.
                                The additional phases added/used are (in pair) :
                                BEGIN_WDATA, END_WDATA : 
                                Phases to indicate the start and end of each 
                                individual non-last write data transfer. BEGIN_REQ, END_REQ 
                                in this case are merely used to indicate the start and end 
                                of address phase. BEGIN_WDATA for the first data beat will 
                                mostly have similar payload and timing as what was sent with 
                                BEGIN_REQ. It can however be only sent after receiving END_REQ.
                                BEGIN_WDATA_LAST, END_WDATA_LAST :
                                Phases to send and accept the last write data transfer in a 
                                write  request.
                                BEGIN_RDATA, END_RDATA :
                                Phases to receive the indvidual non-last read response 
                                transfers from the target TLM2 model. This transactor responds 
                                with END_RDATA, to each BEGIN_RDATA received from the target. 
                                The last read response beat is sent through BEGIN_RESP, 
                                END_RESP pair.
                                
                                This parameter can only be used in the case of non-blocking 
                                transactions ie when "use_nb_transport" is true. Additionally 
                                "use_tlm2_burst" should be left to its default value.
                                Default = false.

    \endverbatim
 *
 * @see xtsc_xttlm2tlm2_transactor
 * @see xtsc::xtsc_parms
 */
class XTSC_COMP_API xtsc_xttlm2tlm2_transactor_parms : public xtsc::xtsc_parms {
public:

  /**
   * Constructor for an xtsc_xttlm2tlm2_transactor_parms object.
   *
   * @param memory_interface    The memory interface name (port name).
   *
   * @param width8              Memory data bus width in bytes.
   *
   * @param num_ports           The number of ports this transactor has.
   *
   * @see xtsc::xtsc_core::How_to_do_memory_port_binding for a list of legal
   *      memory_interface values.
   */
  xtsc_xttlm2tlm2_transactor_parms(const char *memory_interface = "PIF", xtsc::u32 width8 = 4, xtsc::u32 num_ports = 1) {
    init(memory_interface, width8, num_ports);
  }


  /**
   * Constructor for an xtsc_xttlm2tlm2_transactor_parms object based upon an xtsc_core
   * object and a named memory interface. 
   *
   * This constructor will determine width8 and, optionally, num_ports by querying the
   * core object and then pass the values to the init() method.  In addition, the
   * "clock_period" parameter will be set to match the core's clock period.  If desired,
   * after the xtsc_xttlm2tlm2_transactor_parms object is constructed, its data members
   * can be changed using the appropriate xtsc_parms::set() method before passing it to
   * the xtsc_xttlm2tlm2_transactor constructor.
   *
   * @param core                A reference to the xtsc_core object upon which to base
   *                            the xtsc_xttlm2tlm2_transactor_parms.
   *
   * @param memory_interface    The memory interface name (port name).
   *                            Note:  The core configuration must have the named memory
   *                            interface.
   *
   * @param num_ports           The number of ports this transactor has.  If 0, the
   *                            default, the number of ports (1 or 2) will be inferred
   *                            thusly: If memory_interface is a LD/ST unit 0 port of a
   *                            dual-ported core interface, and the core is dual-ported
   *                            and has no CBox, and if the 2nd port of the core has not
   *                            been bound, then "num_ports" will be 2; otherwise,
   *                            "num_ports" will be 1.
   *
   * @see xtsc::xtsc_core::How_to_do_memory_port_binding for a list of legal
   *      memory_interface values.
   */
  xtsc_xttlm2tlm2_transactor_parms(const xtsc::xtsc_core& core, const char *memory_interface, xtsc::u32 num_ports = 0);


  /**
   * Do initialization common to both constructors.
   */
  void init(const char *memory_interface, xtsc::u32 width8 = 4, xtsc::u32 num_ports = 1) {
    std::vector<xtsc::u64> deny_fast_access;
    add("memory_interface",     memory_interface);
    add("num_ports",            num_ports);
    add("byte_width",           width8);
    add("request_fifo_depth",   1);
    add("max_burst_beats",      8);
    add("use_nb_transport",     false);
    add("turbo_switch",         false);
    add("revoke_on_dmi_hint",   false);
    add("use_tlm2_burst",       true);
    add("use_tlm2_busy",        true);
    add("rcw_support",          1);
    add("turbo_support",        2);
    add("deny_fast_access",     deny_fast_access);
    add("enable_extensions",    false);
    add("exclusive_support",    0);
    add("ignore_exclusive",     false);
    add("immediate_timing",     false);
    add("clock_period",         0xFFFFFFFF);
    add("split_tlm2_phases",    false);
  }


  /// Return what kind of xtsc_parms this is (our C++ type)
  virtual const char* kind() const { return "xtsc_xttlm2tlm2_transactor_parms"; }

};




/**
 * Example module implementing an Xtensa TLM (xttlm) to OSCI TLM2 transactor.
 *
 * This module can be used to connect an Xtensa TLM memory interface master (for example,
 * xtsc::xtsc_core, xtsc_component::xtsc_arbiter, etc.) to an OSCI TLM2 memory interface
 * slave (for example, xtsc_memory_tlm2).
 *
 * By default, b_transport is used on the OSCI TLM2 side.  The "use_nb_transport"
 * parameter can be set to true to cause the module to use nb_transport.  In addition,
 * the "turbo_switch" parameter can be set to true to cause the module to use b_transport
 * when the cycle-accurate ISS is active and to use nb_transport when TurboXim is active.
 *
 * For protocol and timing information specific to xtsc::xtsc_core and the Xtensa ISS,
 * see xtsc::xtsc_core::Information_on_memory_interface_protocols.
 *
 * A TLM2 response status of TLM_ADDRESS_ERROR_RESPONSE is translated to an Xtensa TLM
 * RSP_ADDRESS_ERROR response status.  A TLM2 response status of
 * TLM_GENERIC_ERROR_RESPONSE is translated to an Xtensa TLM RSP_ADDRESS_DATA_ERROR
 * response status.  A TLM2 response status of TLM_INCOMPLETE_RESPONSE is treated as an
 * Xtensa TLM RSP_NACC if "use_tlm2_busy" is true, otherewise it causes an exception to
 * be thrown (as does any other TLM2 error response status not mentioned here).
 *
 * Note: Nothing in the TLM2 base protocol requires the target to respond within the
 *       deadline that xtsc::xtsc_core requires for Xtensa local memories (1 cycle for a
 *       5-stage pipeline and 2 cycles for 7-stage).  This transactor should not be used
 *       for an Xtensa local memory interface unless you can be sure the downstream TLM2
 *       subsystem will always respond within the required deadline.
 *
 * Note: This model ignores the xtsc_request_if::nb_lock() call which is used if the
 *       Xtensa processor executes an S32C1I instruction targeting DataRAM.  This
 *       transactor should not be used for an Xtensa DataRAM interface if S32C1I
 *       instructions might target that interface unless there is an upstream model
 *       (e.g. xtsc_arbiter) that is handling nb_lock() calls or unless you do not care
 *       if nb_lock() calls are ignored.
 *
 * Note: If this transactor is used to connect to a TLM2 memory subsystem containing
 *       Xtensa code (memory that will be accessed via Xtensa IFetch), then TurboXim
 *       should not be used unless that memory subsystem can support either unrestricted
 *       DMI access (that is, DMI pointers will always be given and will never be
 *       invalidated) or transport_dbg access.  See the "turbo_support" parameter.
 *
 * Here is a block diagram of xtsc_xttlm2tlm2_transactor objects being used in the
 * xtsc_xttlm2tlm2_transactor example:
 * @image html  Example_xtsc_xttlm2tlm2_transactor.jpg
 * @image latex Example_xtsc_xttlm2tlm2_transactor.eps "xtsc_xttlm2tlm2_transactor Example" width=10cm
 *
 * @see xtsc_xttlm2tlm2_transactor_parms
 * @see xtsc::xtsc_request
 * @see xtsc::xtsc_response
 * @see xtsc::xtsc_request_if
 * @see xtsc::xtsc_respond_if
 * @see xtsc::xtsc_core
 * @see xtsc::xtsc_core::Information_on_memory_interface_protocols.
 * @see xtsc_memory_tlm2
 * @see xtsc_tlm22xttlm_transactor
 */
class XTSC_COMP_API xtsc_xttlm2tlm2_transactor : public sc_core::sc_module,
                                                 public xtsc::xtsc_module,
                                                 public xtsc::xtsc_mode_switch_if,
                                                 public xtsc::xtsc_command_handler_interface,
                                                 public tlm::tlm_mm_interface
{
public:

  sc_core::sc_export<xtsc::xtsc_request_if>   **m_request_exports;      ///<  From upstream master(s) to us
  sc_core::sc_port  <xtsc::xtsc_respond_if>   **m_respond_ports;        ///<  From us to upstream master(s)

  // Shorthand aliases
  typedef tlm::tlm_initiator_socket< 32> initiator_socket_4;    ///< initiator socket with BUSWIDTH =  32 bits ( 4 bytes)
  typedef tlm::tlm_initiator_socket< 64> initiator_socket_8;    ///< initiator socket with BUSWIDTH =  64 bits ( 8 bytes)
  typedef tlm::tlm_initiator_socket<128> initiator_socket_16;   ///< initiator socket with BUSWIDTH = 128 bits (16 bytes)
  typedef tlm::tlm_initiator_socket<256> initiator_socket_32;   ///< initiator socket with BUSWIDTH = 256 bits (32 bytes)
  typedef tlm::tlm_initiator_socket<512> initiator_socket_64;   ///< initiator socket with BUSWIDTH = 512 bits (64 bytes)

  // The system builder (e.g. sc_main) uses one of the following methods to get an initiator socket for binding

  /// Get a reference to the initiator socket with a 4-byte data interface
  initiator_socket_4& get_initiator_socket_4(xtsc::u32 port_num = 0);

  /// Get a reference to the initiator socket with a 8-byte data interface
  initiator_socket_8& get_initiator_socket_8(xtsc::u32 port_num = 0);

  /// Get a reference to the initiator socket with a 16-byte data interface
  initiator_socket_16& get_initiator_socket_16(xtsc::u32 port_num = 0);

  /// Get a reference to the initiator socket with a 32-byte data interface
  initiator_socket_32& get_initiator_socket_32(xtsc::u32 port_num = 0);

  /// Get a reference to the initiator socket with a 64-byte data interface
  initiator_socket_64& get_initiator_socket_64(xtsc::u32 port_num = 0);



  // For SystemC
  SC_HAS_PROCESS(xtsc_xttlm2tlm2_transactor);

  /// Our C++ type (SystemC uses this)
  virtual const char* kind() const { return "xtsc_xttlm2tlm2_transactor"; }


  /**
   * Constructor for an xtsc_xttlm2tlm2_transactor.
   * @param     module_name             Name of the xtsc_xttlm2tlm2_transactor sc_module.
   * @param     transactor_parms        The remaining parameters for construction.
   * @see xtsc_xttlm2tlm2_transactor_parms
   */
  xtsc_xttlm2tlm2_transactor(sc_core::sc_module_name module_name, const xtsc_xttlm2tlm2_transactor_parms& transactor_parms);


  // Destructor.
  ~xtsc_xttlm2tlm2_transactor(void);


  /// Print info on pools for debugging
  void end_of_simulation(void);


  /// For xtsc_connection_interface
  virtual xtsc::u32 get_bit_width(const std::string& port_name, xtsc::u32 interface_num = 0) const;


  /// For xtsc_connection_interface
  virtual sc_core::sc_object *get_port(const std::string& port_name);


  /// For xtsc_connection_interface
  virtual xtsc::xtsc_port_table get_port_table(const std::string& port_table_name) const;


  /// Return byte width of data interface (from "byte_width" parameter)
  xtsc::u32 get_byte_width() const { return m_width8; }


  /// Return the number of memory ports this transactor has (from "num_ports" parameter)
  xtsc::u32 get_num_ports() const { return m_num_ports; }


  /// for xtsc_mode_switch_if
  virtual bool prepare_to_switch_sim_mode(xtsc::xtsc_sim_mode mode);


  /// For xtsc_mode_switch_if
  virtual bool switch_sim_mode(xtsc::xtsc_sim_mode mode);


  /// For xtsc_mode_switch_if
  virtual sc_core::sc_event &get_sim_mode_switch_ready_event() { return m_ready_to_switch_event; }


  /// For xtsc_mode_switch_if
  virtual xtsc::xtsc_sim_mode get_sim_mode() const { return ((m_using_nb_transport) ? xtsc::XTSC_CYCLE_ACCURATE : xtsc::XTSC_FUNCTIONAL);};


  /// For xtsc_mode_switch_if
  virtual bool is_mode_switch_pending() const { return m_mode_switch_pending; }


  /**
   * Reset the transactor.
   */
  void reset(bool hard_reset = false);


  /**
   * Method to change the clock period.
   *
   * @param     clock_period_factor     Specifies the new length of this device's clock
   *                                    period expressed in terms of the SystemC time
   *                                    resolution (from sc_get_time_resolution()).
   */
  void change_clock_period(xtsc::u32 clock_period_factor);


  /**
   * Implementation of the xtsc::xtsc_command_handler_interface.
   *
   * This implementation supports the following commands:
   *  \verbatim
        change_clock_period <ClockPeriodFactor>
          Call xtsc_xttlm2tlm2_transactor::change_clock_period(<ClockPeriodFactor>).
          Return previous <ClockPeriodFactor> for this device.

        peek <StartAddress> <NumBytes>
          Peek <NumBytes> of memory starting at <StartAddress>.

        poke <StartAddress> <NumBytes> <Byte1> <Byte2> . . . <ByteN>
          Poke <NumBytes> (=N) of memory starting at <StartAddress>.

        reset [<Hard>]
          Call xtsc_xttlm2tlm2_transactor::reset().
      \endverbatim
   */
  void execute(const std::string&               cmd_line,
               const std::vector<std::string>&  words,
               const std::vector<std::string>&  words_lc,
               std::ostream&                    result);


  /**
   * Connect an xtsc_arbiter with this xtsc_xttlm2tlm2_transactor.
   *
   * This method connects the master port pair of the xtsc_arbiter with the specified
   * Xtensa TLM slave port pair of this xtsc_xttlm2tlm2_transactor.
   *
   * @param     arbiter         The xtsc_arbiter to connect with this
   *                            xtsc_xttlm2tlm2_transactor.
   *
   * @param     port_num        The Xtensa TLM slave port pair of this
   *                            xtsc_xttlm2tlm2_transactor to connect with the
   *                            xtsc_arbiter.
   */
  void connect(xtsc_arbiter& arbiter, xtsc::u32 port_num = 0);


  /**
   * Connect an xtsc_core with this xtsc_xttlm2tlm2_transactor.
   *
   * This method connects the specified memory interface master port pair of the
   * specified xtsc_core with the specified Xtensa TLM slave port pair of this
   * xtsc_xttlm2tlm2_transactor.
   *
   * @param     core                    The xtsc_core to connect with this
   *                                    xtsc_xttlm2tlm2_transactor.
   *
   * @param     memory_port_name        The name of the memory interface master port
   *                                    pair of the xtsc_core to connect with this
   *                                    xtsc_xttlm2tlm2_transactor. 
   *                                    Case-insensitive.
   *
   * @param     port_num                The slave port pair of this
   *                                    xtsc_xttlm2tlm2_transactor to connect the
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
   *                                    port pairs of this xtsc_xttlm2tlm2_transactor.
   *
   * @see xtsc::xtsc_core::How_to_do_memory_port_binding for a list of valid
   *      memory_port_name values.
   *
   * @returns number of ports that were connected by this call (1 or 2)
   */
  xtsc::u32 connect(xtsc::xtsc_core& core, const char *memory_port_name, xtsc::u32 port_num = 0, bool single_connect = false);


  /**
   * Connect an xtsc_master with this xtsc_xttlm2tlm2_transactor.
   *
   * This method connects the master port pair of the xtsc_master with the Xtensa TLM
   * slave port pair of this xtsc_xttlm2tlm2_transactor.
   *
   * @param     master          The xtsc_master to connect with this
   *                            xtsc_xttlm2tlm2_transactor.
   *
   * @param     port_num        The slave port pair of this
   *                            xtsc_xttlm2tlm2_transactor to connect the
   *                            xtsc_master with.
   */
  void connect(xtsc_master& master, xtsc::u32 port_num = 0);


  /**
   * Connect an xtsc_memory_trace with this xtsc_xttlm2tlm2_transactor.
   *
   * This method connects the specified master port pair of the specified upstream
   * xtsc_memory_trace with the specified Xtensa TLM slave port pair of this
   * xtsc_xttlm2tlm2_transactor.  
   *
   * @param     memory_trace    The xtsc_memory_trace to connect with this
   *                            xtsc_xttlm2tlm2_transactor.
   *
   * @param     trace_port      The master port pair of the xtsc_memory_trace to connect
   *                            with this xtsc_xttlm2tlm2_transactor.
   *
   * @param     port_num        The Xtensa TLM slave port pair of this
   *                            xtsc_xttlm2tlm2_transactor to connect the
   *                            xtsc_memory_trace with.
   *
   * @param     single_connect  If true only one Xtensa TLM slave port pair of this
   *                            xtsc_xttlm2tlm2_transactor will be connected.  If
   *                            false, the default, then all contiguous, unconnected
   *                            slave port pairs of this xtsc_xttlm2tlm2_transactor
   *                            starting at port_num that have a corresponding existing
   *                            master port pair in memory_trace (starting at
   *                            trace_port) will be connected with that corresponding
   *                            memory_trace master port pair.
   *
   * @returns number of ports that were connected by this call (1 or more)
   */
  xtsc::u32 connect(xtsc_memory_trace& memory_trace, xtsc::u32 trace_port = 0, xtsc::u32 port_num = 0, bool single_connect = false);


  /**
   * Connect an xtsc_router with this xtsc_xttlm2tlm2_transactor.
   *
   * This method connects the specified master port pair of the specified xtsc_router
   * with the specified Xtensa TLM slave port pair of this xtsc_xttlm2tlm2_transactor.
   *
   * @param     router          The xtsc_router to connect with this
   *                            xtsc_xttlm2tlm2_transactor.
   *
   * @param     router_port     The master port pair of the xtsc_router to connect with
   *                            this xtsc_xttlm2tlm2_transactor.  router_port must
   *                            be in the range of 0 to the xtsc_router's "num_slaves"
   *                            parameter minus 1.
   *
   * @param     port_num        The Xtensa TLM slave port pair of this
   *                            xtsc_xttlm2tlm2_transactor to connect the
   *                            xtsc_router with.
   */
  void connect(xtsc_router& router, xtsc::u32 router_port, xtsc::u32 port_num = 0);


  /// Get the TextLogger for this component (e.g. to adjust its log level)
  log4xtensa::TextLogger& get_text_logger() { return m_text; }


private:

  /// Implementation of xtsc_request_if.
  class xtsc_request_if_impl : public xtsc::xtsc_request_if, public sc_core::sc_object {
  public:

    /// Constructor
    xtsc_request_if_impl(const char *object_name, xtsc_xttlm2tlm2_transactor& transactor, xtsc::u32 port_num) :
      sc_object         (object_name),
      m_transactor      (transactor),
      m_port_num        (port_num),
      m_p_port          (0)
    {}

    /// From upstream master
    /// @see xtsc::xtsc_request_if
    virtual void nb_peek(xtsc::xtsc_address address8, xtsc::u32 size8, xtsc::u8 *buffer);


    /// From upstream master
    /// @see xtsc::xtsc_request_if
    virtual void nb_poke(xtsc::xtsc_address address8, xtsc::u32 size8, const xtsc::u8 *buffer);


    /// From upstream master. Use "turbo_support" to specify fast access support.
    /// @see xtsc::xtsc_request_if
    virtual bool nb_fast_access(xtsc::xtsc_fast_access_request &request);
    

    /// From upstream master
    /// @see xtsc::xtsc_request_if
    void nb_request(const xtsc::xtsc_request& request);
    

    /// From upstream master.  Not supported by this model.
    /// @see xtsc::xtsc_request_if
    void nb_lock(bool lock);


    /// Return true if a port has been bound to this implementation
    bool is_connected() { return (m_p_port != 0); }



  protected:

    /// SystemC callback when something binds to us
    virtual void register_port(sc_core::sc_port_base& port, const char *if_typename);

    xtsc_xttlm2tlm2_transactor&  m_transactor;  ///<  Our xtsc_xttlm2tlm2_transactor object
    xtsc::u32                   m_port_num;     ///<  Our port number
    sc_core::sc_port_base      *m_p_port;       ///<  Port that is bound to us
  };


  /// Implementation of tlm_bw_transport_if.
  class tlm_bw_transport_if_impl : public tlm::tlm_bw_transport_if<>, public sc_core::sc_object {
  public:

    /// Constructor
    tlm_bw_transport_if_impl(const char *object_name, xtsc_xttlm2tlm2_transactor& transactor, xtsc::u32 port_num) :
      sc_object         (object_name),
      m_transactor      (transactor),
      m_port_num        (port_num),
      m_p_port          (0)
    {}

    /// Send responses
    virtual tlm::tlm_sync_enum nb_transport_bw(tlm::tlm_generic_payload& trans, tlm::tlm_phase& phase, sc_core::sc_time& time);

    /// Invalidate DMI
    virtual void invalidate_direct_mem_ptr(sc_dt::uint64 start_range, sc_dt::uint64 end_range);


  protected:

    /// SystemC callback when something binds to us
    virtual void register_port(sc_core::sc_port_base& port, const char *if_typename);

    xtsc_xttlm2tlm2_transactor&  m_transactor;  ///<  Our xtsc_xttlm2tlm2_transactor object
    xtsc::u32                    m_port_num;    ///<  Our port number
    sc_core::sc_port_base       *m_p_port;      ///<  Port that is bound to us
  };


  /// Class to keep track of address ranges and what DMI access has been granted/invalidated
  class address_range {
  public:
    address_range(xtsc::xtsc_address beg_range, xtsc::xtsc_address end_range) :
      m_beg_range       (beg_range),
      m_end_range       (end_range)
    {}

    xtsc::xtsc_address  m_beg_range;            ///<  Beginning address of the range
    xtsc::xtsc_address  m_end_range;            ///<  End address of the range
  };


  /// Class for tlm_mm_interface for nb_transport
  class nb_mm : public tlm::tlm_mm_interface {
  public:
    nb_mm(xtsc_xttlm2tlm2_transactor& transactor) : m_transactor(transactor) {}
    virtual void free(tlm::tlm_generic_payload *p_trans);
    xtsc_xttlm2tlm2_transactor&  m_transactor;  ///<  Our xtsc_xttlm2tlm2_transactor object
  };


  /// Class to keep track of xtsc_request, tlm_generic_payload, and xtsc_response when using nb_transport
  class transaction_info {
  public:
    xtsc::xtsc_request         *m_p_request;            ///< xtsc_request for this transaction
    tlm::tlm_phase              m_phase;                ///< TLM2 phase this transaction is in
    tlm::tlm_generic_payload   *m_p_trans;              ///< tlm_generic_payload for this transaction
    xtsc::xtsc_response        *m_p_response;           ///< xtsc_response for this transaction
    bool                        m_last_tlm2_beat;       ///< True if last TLM2 transaction for a given request (needed for BLOCK_READ|BURST_READ)
    xtsc::u32                   m_responses_received;   ///< received responses count for a given request (needed for BLOCK_READ|BURST_READ mostly for nb case)
  };



  /// Perform validation for the get_initiator_socket_BW() methods
  void validate_port_and_width(xtsc::u32 port_num, xtsc::u32 width8);


  /// Get a new transaction object (from the pool)
  tlm::tlm_generic_payload *new_transaction();


  /// For tlm_mm_interface for b_transport
  virtual void free(tlm::tlm_generic_payload *p_trans);


  /// Get a new xtsc_request (from the pool)
  xtsc::xtsc_request *new_request(const xtsc::xtsc_request& request);


  /// Delete an xtsc_request (return it to the pool)
  void delete_request(xtsc::xtsc_request*& p_request);


  /// Get a new transaction_info object (from the pool)
  transaction_info *new_transaction_info(xtsc::xtsc_request *p_request);


  /// Get a new u8 buffer object (from the pool).  Size is m_width8*m_max_transfers
  xtsc::u8 *new_buffer();


  /// Delete a u8 buffer object (return it to the pool)
  void delete_buffer(xtsc::u8*& p_buffer);


  /// Handle incoming requests from each master when using b_transport
  void worker_thread(void);


  /// Handle incoming requests from each master when using nb_transport
  void request_thread(void);


  /// Handle incoming responses from each slave when using nb_transport
  void respond_thread(void);


  /// Common method for sending a response
  void send_response(xtsc::xtsc_response& response, xtsc::u32 port_num);


  /// Throw when m_width8 is invalid for PIF-only (block/burst/rcw) transaction
  void throw_invalid_pif_bus_width(xtsc::u32 port_num);


  /// Common method for calling transport_dbg() and handling the return count
  void do_transport_dbg(tlm::tlm_generic_payload& trans, xtsc::u32 port_num);


  /// Common method for calling get_direct_mem_ptr()
  bool do_get_direct_mem_ptr(tlm::tlm_generic_payload& trans, tlm::tlm_dmi &dmi_data, xtsc::u32 port_num);


  /// Common method for calling nb_transport_fw()
  tlm::tlm_sync_enum do_nb_transport_fw(tlm::tlm_generic_payload& trans, tlm::tlm_phase& phase, xtsc::u32 port_num);


  /// Common method for calling b_transport() and handling the response status
  void do_b_transport(xtsc::xtsc_response& response, tlm::tlm_generic_payload& trans, sc_core::sc_time& delay, xtsc::u32 port_num);


  /// Common method to check and set TLM2 extensions for an exclusive request
  bool check_and_set_exclusive(xtsc::xtsc_request *p_request, tlm::tlm_generic_payload *p_trans);


  /// Common method to add supported TLM2 ignorable extensions
  void add_extensions(xtsc::xtsc_request *p_request, tlm::tlm_generic_payload *p_trans);


  /// Common method to remove supported TLM2 ignorable extensions
  void remove_extensions(tlm::tlm_generic_payload *p_trans, bool excl);


  void do_read       (xtsc::xtsc_response& response, xtsc::u32 port_num);       ///<  Handle READ        using b_transport
  void do_block_read (xtsc::xtsc_response& response, xtsc::u32 port_num);       ///<  Handle BLOCK_READ  using b_transport
  void do_burst_read (xtsc::xtsc_response& response, xtsc::u32 port_num);       ///<  Handle BURST_READ  using b_transport
  void do_rcw        (xtsc::xtsc_response& response, xtsc::u32 port_num);       ///<  Handle RCW         using b_transport
  void do_write      (xtsc::xtsc_response& response, xtsc::u32 port_num);       ///<  Handle WRITE       using b_transport
  void do_block_write(xtsc::xtsc_response& response, xtsc::u32 port_num);       ///<  Handle BLOCK_WRITE using b_transport
  void do_burst_write(xtsc::xtsc_response& response, xtsc::u32 port_num);       ///<  Handle BURST_WRITE using b_transport

  bool do_nb_read       (transaction_info *p_info, xtsc::u32 port_num);         ///<  Handle READ        using nb_transport, return true if AErr
  bool do_nb_block_read (transaction_info *p_info, xtsc::u32 port_num);         ///<  Handle BLOCK_READ  using nb_transport, return true if AErr
  bool do_nb_burst_read (transaction_info *p_info, xtsc::u32 port_num);         ///<  Handle BURST_READ  using nb_transport, return true if AErr
  bool do_nb_rcw        (transaction_info *p_info, xtsc::u32 port_num);         ///<  Handle RCW         using nb_transport, return true if AErr
  bool do_nb_write      (transaction_info *p_info, xtsc::u32 port_num);         ///<  Handle WRITE       using nb_transport, return true if AErr
  bool do_nb_block_write(transaction_info *p_info, xtsc::u32 port_num);         ///<  Handle BLOCK_WRITE using nb_transport, return true if AErr
  bool do_nb_burst_write(transaction_info *p_info, xtsc::u32 port_num);         ///<  Handle BURST_WRITE using nb_transport, return true if AErr

  typedef std::map<tlm::tlm_generic_payload*, transaction_info*>  transaction_info_map;
  typedef tlm_utils::peq_with_get<tlm::tlm_generic_payload>       peq;

  // m_initiator_sockets_BW (BW = Byte width of data interface = BUSWIDTH/8)
  initiator_socket_4                          **m_initiator_sockets_4;          ///<  Initiator socket(s) for  4-byte interface
  initiator_socket_8                          **m_initiator_sockets_8;          ///<  Initiator socket(s) for  8-byte interface
  initiator_socket_16                         **m_initiator_sockets_16;         ///<  Initiator socket(s) for 16-byte interface
  initiator_socket_32                         **m_initiator_sockets_32;         ///<  Initiator socket(s) for 32-byte interface
  initiator_socket_64                         **m_initiator_sockets_64;         ///<  Initiator socket(s) for 64-byte interface

  xtsc_request_if_impl                        **m_request_impl;                 ///<  m_request_exports bind to these
  tlm_bw_transport_if_impl                    **m_tlm_bw_transport_if_impl;     ///<  m_initiator_sockets_BW binds to these
  nb_mm                                         m_nb_mm;                        ///<  tlm_mm_interface for nb_transport

  xtsc::u32                                     m_next_worker_port_num;         ///<  Used by worker_thread to get its port number
  xtsc::u32                                     m_next_request_port_num;        ///<  Used by request_thread to get its port number
  xtsc::u32                                     m_next_respond_port_num;        ///<  Used by respond_thread to get its port number
  xtsc::u32                                     m_num_ports;                    ///<  See the "num_ports" parameter
  const char                                   *m_memory_interface_str;         ///<  See the "memory_interface" parameter
  xtsc::xtsc_core::memory_port                  m_memory_port;                  ///<  See the "memory_interface" parameter
  xtsc::u32                                     m_width8;                       ///<  The bus width in bytes.  See "byte_width"
  xtsc::u32                                     m_request_fifo_depth;           ///<  See the "request_fifo_depth" parameter.
  xtsc::u32                                     m_max_burst_beats;              ///<  See the "max_burst_beats" parameter.
  bool                                          m_use_nb_transport;             ///<  See the "use_nb_transport" parameter
  bool                                          m_turbo_switch;                 ///<  See the "turbo_switch" parameter
  bool                                          m_revoke_on_dmi_hint;           ///<  See the "revoke_on_dmi_hint" parameter
  bool                                          m_using_nb_transport;           ///<  True if currently using nb_transport, false if using b_transport
  bool                                          m_use_tlm2_burst;               ///<  See the "use_tlm2_burst" parameter
  bool                                          m_use_tlm2_busy;                ///<  See the "use_tlm2_busy" parameter
  bool                                          m_enable_extensions;            ///<  See "enable_extensions" parameter
  xtsc::u32                                     m_exclusive_support;            ///<  See "exclusive_support" parameter
  bool                                          m_ignore_exclusive;             ///<  See "ignore_exclusive" parameter
  bool                                          m_immediate_timing;             ///<  See "immediate_timing" parameter
  bool                                          m_split_tlm2_phases;            ///<  See "split_tlm2_phases" parameter
  bool                                          m_has_pif_width;                ///<  True if m_width8 is 4|8|16.
  xtsc::u32                                     m_rcw_support;                  ///<  See the "rcw_support" parameter
  xtsc::u32                                     m_turbo_support;                ///<  See the "turbo_support" parameter
  std::vector<xtsc::u64>                        m_deny_fast_access;             ///<  See the "deny_fast_access" parameter
  sc_core::sc_time                              m_clock_period;                 ///<  This transactor's clock period
  sc_core::sc_event                           **m_worker_thread_event;          ///<  To notify worker_thread when a request is accepted
  sc_core::sc_event                           **m_request_thread_event;         ///<  To notify request_thread when a request is accepted
  sc_core::sc_event                           **m_request_phase_ended_event;    ///<  To notify request_thread when the request phase ends 
  sc_core::sc_event                             m_ready_to_switch_event;        ///<  Signal ready to switch simulation mode
  peq                                         **m_respond_thread_peq;           ///<  For respond_thread
  xtsc::xtsc_request                          **m_request;                      ///<  Our copy of current request
  sc_core::sc_fifo<xtsc::xtsc_request*>       **m_worker_fifo;                  ///<  The fifos for incoming requests on each port for  b_transport
  sc_core::sc_fifo<xtsc::xtsc_request*>       **m_request_fifo;                 ///<  The fifos for incoming requests on each port for nb_transport
  bool                                         *m_busy;                         ///<  True when we have a current request (fifo not used)
  bool                                          m_mode_switch_pending;          ///<  Indicate simulation mode switch is in progress
  bool                                          m_dmi_invalidated;              ///<  To ensure no DMI invalidate occurred
  bool                                          m_can_revoke_fast_access;       ///<  All fast access requests (if any) allowed revocation
  bool                                         *m_rcw_have_first_transfer;      ///<  True when we've rec'd 1st xfer of RCW but not 2nd
  bool                                         *m_first_block_write;            ///<  Next BLOCK_WRITE is a first transfer
                                                                                ///<  Only used when "use_tlm2_burst" is true
  xtsc::u8                                    **m_rcw_compare_data;             ///<  Comparison data from RCW request
  xtsc::u8                                    **m_burst_data;                   ///<  Accumulate data for BLOCK_READ|BURST_READ|BLOCK_WRITE|BURST_WRITE
                                                                                ///<   (used when m_use_tlm2_burst is true)
  xtsc::u8                                    **m_byte_enables;                 ///<  For byte enable pointer in gp
  xtsc::u32                                    *m_burst_index;                  ///<  Index into m_burst_data for BLOCK_WRITE|BURST_WRITE
  sc_core::sc_time                             *m_burst_start_time;             ///<  Time when 1st RCW|BLOCK_WRITE|BURST_WRITE arrives
  sc_core::sc_time                             *m_prev_response_time;           ///<  When previous response was sent
  transaction_info_map                          m_transaction_info_map;         ///<  Map tlm_generic_payload to transaction_info
  std::map<xtsc::u64,xtsc::u32>                 m_responses_received_map;       ///<  Map tag to number of received responses for a request(mostly used for nb case)
  std::vector<tlm::tlm_generic_payload*>        m_transaction_pool;             ///<  Maintain a pool to improve performance: tlm_generic_payload
  std::vector<transaction_info*>                m_transaction_info_pool;        ///<  Maintain a pool to improve performance: transaction_info
  std::vector<xtsc::xtsc_request*>              m_request_pool;                 ///<  Maintain a pool to improve performance: xtsc_request
  std::vector<xtsc::u8*>                        m_buffer_pool;                  ///<  Maintain a pool of u8 buffers of size m_width8*m_max_transfers

  xtsc::u32                                     m_transaction_count;            ///<  Count each newly created transaction in new_transaction
  xtsc::u32                                     m_transaction_info_count;       ///<  Count each newly created transaction_info
  xtsc::u32                                     m_request_count;                ///<  Count each newly created xtsc_request
  xtsc::u32                                     m_buffer_count;                 ///<  Count each newly created u8 buffer
  xtsc::u32                                     m_max_transfers;                ///<  Max value allowed from xtsc::xtsc_request::get_num_transfers()
  xtsc::u32                                     m_max_payload_bytes;            ///<  Max payload bytes: max_byte_width*max_num_xfers

  std::list<address_range*>                   **m_allowed_ranges;               ///<  Ranges TurboXim has been granted raw access to
  std::set<xtsc::xtsc_fast_access_revocation_if*>
                                                m_revocation_set;               ///<  Set of fast access revocation interface objects
  std::vector<sc_core::sc_process_handle>       m_process_handles;              ///<  For reset 

  log4xtensa::TextLogger&                       m_text;                         ///<  Text logger

};


}  // namespace xtsc_component

#endif  // _XTSC_XTTLM2TLM2_TRANSACTOR_H_
