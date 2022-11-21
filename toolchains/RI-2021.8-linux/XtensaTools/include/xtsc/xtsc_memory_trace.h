#ifndef _XTSC_MEMORY_TRACE_H_
#define _XTSC_MEMORY_TRACE_H_

// Copyright (c) 2005-2018 by Cadence Design Systems Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Cadence Design Systems Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Cadence Design Systems Inc.

/**
 * @file 
 */


#include <xtsc/xtsc.h>
#include <xtsc/xtsc_parms.h>
#include <xtsc/xtsc_request_if.h>
#include <xtsc/xtsc_respond_if.h>
#include <xtsc/xtsc_request.h>
#include <xtsc/xtsc_response.h>
#include <xtsc/xtsc_address_range_entry.h>
#include <xtsc/xtsc_fast_access.h>
#include <vector>
#include <map>
#include <cstring>



namespace xtsc {
class xtsc_core;
}


namespace xtsc_component {

class xtsc_arbiter;
class xtsc_dma_engine;
class xtsc_master;
class xtsc_pin2tlm_memory_transactor;
class xtsc_router;


/**
 * Constructor parameters for a xtsc_memory_trace object.
 *
 *  \verbatim
   Name                  Type   Description
   ------------------    ----   -------------------------------------------------------
  
   "byte_width"          u32    Memory data interface width in bytes.  Valid values are
                                4, 8, 16, 32, and 64.  
                                Default = 64.
  
   "big_endian"          bool   True if the master is big endian.
                                Default = false.

   "vcd_handle"          void*  Pointer to a SystemC VCD object (sc_trace_file *) or 0
                                if the memory trace device should create its own trace 
                                file (which will be called "waveforms.vcd").
                                Default = 0.

   "num_ports"           u32    The number of memory ports attached to the memory trace
                                device.  This number of master devices and this number
                                of slave devices must be connected to the memory trace
                                device.
                                Default = 1.

   "allow_tracing"       bool   True if VCD trace is allowed.  False if VCD trace is not
                                allowed.  Setting "allow_tracing" to false allows
                                leaving the model in place in sc_main but with tracing
                                disabled so there is near zero impact on simulation
                                time.  This might also be done if latency tracking is
                                desired (see "track_latency") but VCD tracing is not.
                                Default = true.

   "enable_tracing"      bool   True if VCD tracing is initially enabled.  False if VCD
                                tracing is initially disabled.  See the enable_tracing
                                command and method.
                                Default = true.

   "track_latency"       bool   True if lifetime, latency, and counter tracking is to be
                                done.  False if not to be done.  When "track_latency" is
                                set to true, the xtsc_memory_trace class maintains
                                lifetime and latency histograms and counters of
                                transactions, request beats, request busys, response
                                beats, and response busys.  The histograms and counters
                                are maintained per xtsc_request type and per port.
                                Transaction lifetime is the number of clock periods
                                between when the first request of the transaction is
                                first seen and when the last response of the transaction
                                is accepted.  Transaction latency is the number of clock
                                periods between when the last request is accepted and
                                when the first response returns.  Results are logged at
                                INFO level at the end of the simulation and may also be
                                obtained at any time during simulation using the
                                dump_statistic_info, dump_latency_histogram,
                                dump_lifetime_histogram, and get_counter commands and
                                methods.
                                Default = false.

   "num_transfers"       u32    If non-zero then only transactions with this number of
                                transfers are tracked (that is, transactions whose
                                xtsc_request::get_num_transfers() method returns a value
                                equal to the value set by this parameter).  This can be
                                used to get statistics on block/burst transactions of a
                                single size rather than mixing unequal transaction sizes
                                together.  This parameter is ignored if "track_latency"
                                is false and enable_latency_tracking(true) is never
                                called.
                                Default = 0.

    \endverbatim
 *
 * @see xtsc_memory_trace
 * @see xtsc::xtsc_parms
 */
class XTSC_COMP_API xtsc_memory_trace_parms : public xtsc::xtsc_parms {
public:


  /**
   * Constructor for an xtsc_memory_trace_parms object.
   *
   * @param     width8          Memory data interface width in bytes.
   *                            Default = 64.
   *
   * @param     big_endian      True if master is big_endian.
   *                            Default = false.
   *
   * @param     p_trace_file    Pointer to SystemC VCD object or 0 if the memory trace
   *                            device should create its own VCD object (which will be
   *                            called "waveforms.vcd").
   *                            Default = 0.
   *
   * @param     num_ports       The number of memory ports the memory trace device has.
   *                            Default = 1.
   *
   */
  xtsc_memory_trace_parms(xtsc::u32               width8        = 64,
                          bool                    big_endian    = false,
                          sc_core::sc_trace_file *p_trace_file  = 0,
                          xtsc::u32               num_ports     = 1)
  {
    init(width8, big_endian, p_trace_file, num_ports);
  }


  /**
   * Constructor for an xtsc_memory_trace_parms object based upon an xtsc_core object
   * and a named memory interface. 
   *
   * This constructor will determine width8, big_endian, and, optionally, num_ports by
   * querying the core object and then pass the values to the init() method.  
   *
   * If desired, after the xtsc_memory_trace_parms object is constructed, its data
   * members can be changed using the appropriate xtsc_parms::set() method before
   * passing it to the xtsc_memory constructor.
   *
   * @param core                A reference to the xtsc_core object upon which to base
   *                            the xtsc_memory_trace_parms.
   *
   * @param memory_interface    The memory interface name (port name).
   *                            Note:  The core configuration must have the named memory
   *                            interface.
   *
   * @param p_trace_file        Pointer to SystemC VCD object or 0 if the memory trace
   *                            device should create its own trace file (which will be
   *                            called "waveforms.vcd").
   *                            Default = 0.
   *
   * @param num_ports           The number of ports this memory has.  If 0, the default,
   *                            the number of ports will be inferred based on the number
   *                            of multi-ports in the port_name core interface (assuming
   *                            they are unbound).
   *
   * @see xtsc::xtsc_core::How_to_do_memory_port_binding for a list of legal
   *      memory_interface values.
   */
  xtsc_memory_trace_parms(const xtsc::xtsc_core&        core,
                          const char                   *memory_interface, 
                          sc_core::sc_trace_file       *p_trace_file    = 0,
                          xtsc::u32                     num_ports       = 0);


  /// Return what kind of xtsc_parms this is (our C++ type)
  virtual const char* kind() const { return "xtsc_memory_trace_parms"; }

protected:

  /// Do initialization common to all constructors
  void init(xtsc::u32 width8, bool big_endian, sc_core::sc_trace_file *p_trace_file, xtsc::u32 num_ports) {
    add("byte_width",           width8);
    add("big_endian",           big_endian);
    add("vcd_handle",           (void*) p_trace_file);
    add("num_ports",            num_ports);
    add("allow_tracing",        true);
    add("enable_tracing",       true);
    add("track_latency",        false);
    add("num_transfers",        0);
  }
};





/**
 * Example XTSC model which generates a value-change dump (VCD) file of the data members
 * of each xtsc::xtsc_request and xtsc::xtsc_response that passes through it
 * ("allow_tracing" true) and/or which tracks the lifetime, latency, and counters of
 * each transaction by request type and by port number ("track_latency" true).  
 *
 * This module is designed to be inserted between a memory interface master (for
 * example, an xtsc_core) and a memory interface slave (for example, an xtsc_memory) to 
 * generate a VCD file trace of the data members of each xtsc::xtsc_request that the
 * master sends the slave and each xtsc::xtsc_response that the slave sends the master
 * and/or to track the lifetime, latency, and counters of each transaction.
 *
 * When doing VCD tracing, each request and each response is counted and the
 * counts are traced.  In addition, each RSP_OK response and each RSP_NACC response are
 * separately counted and traced to make it easier to detect when an RSP_NACC response
 * happens in the same cycle as an RSP_OK response.  VCD tracing can be turned off and
 * on during simulation using the enable_tracing method and/or command.
 *
 * When the "track_latency" parameter is true, lifetime and latency histograms and
 * transaction, beat, and busy counters are maintained.  This can be turned off and on
 * during simulation using the enable_latency_tracking method and/or command.  
 * Informally, transaction lifetime is the number of clock periods between when the
 * first request of the transaction is first received from the upstream device and when
 * the last response of the transaction is accepted by the upstream device, while
 * transaction latency is the number of clock periods between when the last request is
 * accepted by the downstream device and when the first response returns from the
 * downstream device.  See transaction_info for the formal definition.
 *
 * Here is a block diagram of an xtsc_memory_trace as it is used in the
 * xtsc_memory_trace example:
 * @image html  Example_xtsc_memory_trace.jpg
 * @image latex Example_xtsc_memory_trace.eps "xtsc_memory_trace Example" width=10cm
 *
 * @see xtsc_memory_trace_parms
 * @see xtsc::xtsc_request_if
 * @see xtsc::xtsc_respond_if
 * @see transaction_info
 * @see statistic_info
 * @see cntr_type
 * @see get_counter
 * @see dump_statistic_info
 * @see dump_latency_histogram
 * @see dump_lifetime_histogram
 *
 */
class XTSC_COMP_API xtsc_memory_trace : public sc_core::sc_module, public xtsc::xtsc_module, public xtsc::xtsc_command_handler_interface {
public:

  sc_core::sc_export<xtsc::xtsc_request_if>   **m_request_exports;      ///<  From multi-ported master or multiple masters to us
  sc_core::sc_port  <xtsc::xtsc_request_if>   **m_request_ports;        ///<  From us to multi-ported slave or multiple slaves
  sc_core::sc_export<xtsc::xtsc_respond_if>   **m_respond_exports;      ///<  From multi-port slave or multiple slaves to us
  sc_core::sc_port  <xtsc::xtsc_respond_if>   **m_respond_ports;        ///<  From us to multi-ported master or multiple masters


  /// Our C++ type (SystemC uses this)
  virtual const char* kind() const { return "xtsc_memory_trace"; }


  /**
   * Constructor for an xtsc_memory_trace.
   * @param     module_name     Name of the xtsc_memory_trace sc_module.
   * @param     trace_parms     The remaining parameters for construction.
   * @see xtsc_memory_trace_parms
   */
  xtsc_memory_trace(sc_core::sc_module_name module_name, const xtsc_memory_trace_parms& trace_parms);


  // Destructor.
  ~xtsc_memory_trace(void);


  /// For xtsc_connection_interface
  virtual xtsc::u32 get_bit_width(const std::string& port_name, xtsc::u32 interface_num = 0) const;


  /// For xtsc_connection_interface
  virtual sc_core::sc_object *get_port(const std::string& port_name);


  /// For xtsc_connection_interface
  virtual xtsc::xtsc_port_table get_port_table(const std::string& port_table_name) const;


  /// Get the number of memory ports this memory trace device has
  xtsc::u32 get_num_ports() { return m_num_ports; }


  /// Get a pointer to the VCD object that this memory trace device is using
  sc_core::sc_trace_file *get_trace_file() { return m_p_trace_file; }


  /**
   * Reset the xtsc_memory_trace.
   */
  void reset(bool hard_reset = false);


  /**
   * Return whether or not tracing is enabled.
   *
   * @see xtsc_memory_trace_parms "enable_tracing".
   */
  bool is_tracing_enabled() const { return m_enable_tracing; }


  /**
   * Set whether or not tracing is enabled.
   *
   * @see xtsc_memory_trace_parms "enable_tracing".
   */
  void enable_tracing(bool enable);


  /**
   * Return whether or not latency tracking is enabled.
   *
   * @see xtsc_memory_trace_parms "track_latency".
   */
  bool is_latency_tracking_enabled() const { return m_track_latency; }


  /**
   * Set whether or not latency tracking is enabled.
   *
   * @see xtsc_memory_trace_parms "track_latency".
   */
  void enable_latency_tracking(bool enable);


  /// SystemC calls this method at the end of simulation 
  void end_of_simulation();
  
  
  /**
   * Dump maximum latencies and lifetimes observed for each xtsc::xtsc_request::type_t
   * type.
   *
   * This method is deprecated in favor of the dump_statistic_info() method which
   * provides more useful information.
   *
   * @param     os              The ostream object to which the results should be dumped.
   *
   * @see "track_latency" in xtsc_memory_trace_parms.
   */
  void dump_latencies(std::ostream& os = std::cout);


  /**
   * Return the value of the specified counter for the specified xtsc::xtsc_request::type_t
   * type and specified port.
   *
   * This method returns the value of the specified counter for the specified request
   * type and specified port.  Multiple request types and multiple ports can be
   * specified, in which case the method returns the sum of the values of the specified
   * counter for all matching request types and ports.
   *
   * @param     cntr_name       The desired counter name.  See cntr_type.
   *
   * @param     types           A comma separated list of the xtsc::xtsc_request::type_t
   *                            types desired (for example, "READ,BLOCK_READ").  The
   *                            empty string ("") or asterisk ("*") both mean all
   *                            tracked request types.
   *
   * @param     ports           A comma separated list of the desired port numbers (for
   *                            examples, "1,3,5").  The empty string ("") or asterisk
   *                            ("*") both mean all ports.
   *
   * @see dump_counter_names
   * @see cntr_type
   */
  xtsc::u64 get_counter(const std::string& cntr_name, const std::string& types = "", const std::string& ports = "");


  /**
   * Dump statistic info for the specified xtsc::xtsc_request::type_t types and ports to
   * the specified ostream object.
   *
   * Method dumps a block of statistic info for each specified xtsc::xtsc_request::type_t
   * type of each specified port.  For example, here is a sample output block when this
   * method was called with types of "BLOCK_READ" and ports of "1" (the Counters and
   * Average lines are shown on multiple lines due to printing limitations in some media):
   *  \verbatim
      Port #1 BLOCK_READ: 
        Counters: transactions=4 latency=111 lifetime=200 req_beats=18 req_busys=14 
                                                          rsp_beats=79 rsp_busys=15
        Average per transaction: latency=27.75 lifetime=50 req_beats=4.5 req_busys=3.5
                                                          rsp_beats=19.75 rsp_busys=3.75
        Histograms (Format: NumCycles=TranCount):
        latency:  2=1,16=1,46=1,47=1 (tag=5)
        lifetime: 17=1,46=1,61=1,76=1 (tag=5)
      \endverbatim
   * The value in parenthesis at the end of the histograms is the tag of the first
   * transaction during simulation with the maximum (last) NumCycles value.
   *
   * @param     types           A comma separated list of xtsc::xtsc_request::type_t
   *                            types desired (for example, "READ,BLOCK_READ").  The
   *                            empty string ("") or asterisk ("*") both mean all
   *                            tracked request types.
   *
   * @param     ports           A comma separated list of the desired port numbers (for
   *                            examples, "1,3,5").  The empty string ("") or asterisk
   *                            ("*") both mean all ports.
   *
   * @see xtsc_memory_trace for the definition of transaction latency and lifetime
   * @see statistic_info
   * @see cntr_type
   */
  void dump_statistic_info(std::ostream& os = std::cout, const std::string& types = "", const std::string& ports = "");



  /**
   * Dump an aggregate histogram of the latency values for the specified
   * xtsc::xtsc_request::type_t types and ports to the specified ostream object.
   *
   * This method dumps a histogram of the latency values for the specified request
   * type and port.  For example, here is a sample output block when this method was
   * called with types of "BLOCK_READ" and ports of "1" using the same simulation as
   * shown for dump_statistic_info:
   *  \verbatim
      2,1
      16,1
      46,1
      47,1
      \endverbatim
   *
   * If this method is called for multiple types or ports then an aggregate histogram is
   * formed by combining the individual histograms.  For example, if the same simulation
   * had 2 BLOCK_WRITE transactions on Port #1, one with a latency of 2 cycles and the
   * other with a latency of 8 cycles, and this method were called with types of
   * "BLOCK_READ,BLOCK_WRITE" and ports of "1", then the output would be:
   *  \verbatim
      2,2
      8,1
      16,1
      46,1
      47,1
      \endverbatim
   *
   * @param     types           A comma separated list of the xtsc::xtsc_request::type_t
   *                            types desired (for example, "READ,BLOCK_READ").  The
   *                            empty string ("") or asterisk ("*") both mean all
   *                            tracked request types.
   *
   * @param     ports           A comma separated list of the desired port numbers (for
   *                            examples, "1,3,5").  The empty string ("") or asterisk
   *                            ("*") both mean all ports.
   *
   * @see statistic_info
   * @see cntr_type
   */
  void dump_latency_histogram(std::ostream& os = std::cout, const std::string& types = "", const std::string& ports = "");


  /**
   * Dump an aggregate histogram of the lifetime values for the specified
   * xtsc::xtsc_request::type_t types and ports to the specified ostream object.
   *
   * This method dumps a histogram of the lifetime values for the specified request
   * type and port.  For example, here is a sample output block when this method was
   * called with types of "BLOCK_READ" and ports of "1" using the same simulation as
   * shown for dump_statistic_info:
   *  \verbatim
      17,1
      46,1
      61,1
      76,1
      \endverbatim
   *
   * If this method is called for multiple types or ports then an aggregate histogram is
   * formed by combining the individual histograms.
   *
   * @param     types           A comma separated list of the xtsc::xtsc_request::type_t
   *                            types desired (for example, "READ,BLOCK_READ").  The
   *                            empty string ("") or asterisk ("*") both mean all
   *                            tracked request types.
   *
   * @param     ports           A comma separated list of the desired port numbers (for
   *                            examples, "1,3,5").  The empty string ("") or asterisk
   *                            ("*") both mean all ports.
   *
   * @see statistic_info
   * @see cntr_type
   */
  void dump_lifetime_histogram(std::ostream& os = std::cout, const std::string& types = "", const std::string& ports = "");
  
  
  /**
   * Implementation of the xtsc::xtsc_command_handler_interface.
   *
   * This implementation supports the following commands:
   *  \verbatim
        dump_counter_names
          Dump a list of valid counter names by calling dump_counter_names().

        dump_latency_histogram [<Types> [<Ports>]]
          Call dump_latency_histogram() for the specified request <Types> and <Ports>.
          Default all <Types> and <Ports>.

        dump_lifetime_histogram [<Types> [<Ports>]]
          Call dump_lifetime_histogram() for the specified request <Types> and <Ports>.
          Default all <Types> and <Ports>.

        dump_statistic_info [<Types> [<Ports>]]
          Call statistic_info::dump() for each of the specified request <Types> and <Ports>.
          Default all <Types> and <Ports>.

        enable_latency_tracking <Enable>
          Call xtsc_memory_trace::enable_latency_tracking(<Enable>). 
          Set whether or not latency tracking is enabled. 
          
        enable_tracing <Enable>
          Call xtsc_memory_trace::enable_tracing(<Enable>).
          Set whether or not tracing is enabled.  
          
        get_counter <CntrName> [<Types> [<Ports>]]
          Return value from calling xtsc_memory_trace::get_counter(<CntrName>, <Types>, <Ports>).
          Default all <Types> and <Ports>.

        get_num_ports
          Return value from calling xtsc_memory_trace::get_num_ports().

        reset
          Call xtsc_memory_trace::reset().  

      \endverbatim
   */
  virtual void execute(const std::string&               cmd_line,
                       const std::vector<std::string>&  words,
                       const std::vector<std::string>&  words_lc,
                       std::ostream&                    result);


  /**
   * Connect an upstream xtsc_arbiter with this xtsc_memory_trace.
   *
   * This method connects the master port pair of the specified xtsc_arbiter to the
   * specified slave port pair of this xtsc_memory_trace.
   *
   * @param     arbiter         The xtsc_arbiter to connect with.
   *
   * @param     trace_port      The slave port pair of this xtsc_memory_trace to connect
   *                            the xtsc_arbiter with.  trace_port must be in the
   *                            range of 0 to this xtsc_memory_trace's "num_ports"
   *                            parameter minus 1.
   */
  void connect(xtsc_arbiter& arbiter, xtsc::u32 trace_port = 0);


  /**
   * Connect with an upstream or downstream (inbound pif) xtsc_core.
   *
   * This method connects this xtsc_memory_trace with the memory interface specified by
   * memory_port_name of the xtsc_core specified by core.  If memory_port_name is
   * "inbound_pif" or "snoop", then a master port pair of this xtsc_memory_trace is
   * connected with the inbound pif or snoop slave port pair (respectively) of core.  If
   * memory_port_name is neither "inbound_pif" nor "snoop", then the master port pair of
   * core specified by memory_port_name is connected with a slave port pair of this
   * xtsc_memory_trace
   *
   * @param     core                    The xtsc_core to connect with.
   *
   * @param     memory_port_name        The name of the xtsc_core memory interface to
   *                                    connect with.  Case-insensitive.
   *
   * @param     port_num                If memory_port_name is "inbound_pif" or "snoop"
   *                                    then this specifies the master port pair of this
   *                                    xtsc_memory_trace to connect with core.  If
   *                                    memory_port_name is neither "inbound_pif" nor
   *                                    "snoop" then this specifies the slave port pair
   *                                    of this xtsc_memory_trace to connect with core.
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
   *                                    port pairs of this xtsc_memory_trace.
   *                                    This parameter is ignored if memory_port_name is
   *                                    "inbound_pif" or "snoop".
   *
   * @returns number of ports that were connected by this call (1 or 2)
   *
   * @see xtsc::xtsc_core::How_to_do_memory_port_binding for a list of valid
   *      memory_port_name values.
   *
   * Note: The snoop port is reserved for future use.
   */
  xtsc::u32 connect(xtsc::xtsc_core& core, const char *memory_port_name, xtsc::u32 port_num = 0, bool single_connect = false);


  /**
   * Connect with an xtsc_dma_engine.
   *
   * This method connects the master port pair of the specified xtsc_dma_engine with the
   * specified slave port pair of this xtsc_memory_trace.
   *
   * @param     dma_engine      The xtsc_dma_engine to connect with.
   *
   * @param     trace_port      The slave port pair of this xtsc_memory_trace to connect
   *                            the xtsc_dma_engine with.
   */
  void connect(xtsc_dma_engine& dma_engine, xtsc::u32 trace_port = 0);


  /**
   * Connect with an xtsc_master.
   *
   * This method connects the master port pair of the specified xtsc_master with the
   * specified slave port pair of this xtsc_memory_trace.
   *
   * @param     master          The xtsc_master to connect with.
   *
   * @param     trace_port      The slave port pair of this xtsc_memory_trace to connect
   *                            the xtsc_master with.
   */
  void connect(xtsc_master& master, xtsc::u32 trace_port = 0);


  /**
   * Connect with an xtsc_pin2tlm_memory_transactor.
   *
   * This method connects this xtsc_memory_trace with the specified TLM master port pair
   * of the specified xtsc_pin2tlm_memory_transactor.
   *
   * @param     pin2tlm         The xtsc_pin2tlm_memory_transactor to connect with this
   *                            xtsc_memory_trace.
   *
   * @param     tran_port       The xtsc_pin2tlm_memory_transactor TLM master port pair
   *                            to connect with this xtsc_memory_trace.
   *
   * @param     trace_port      The slave port pair of this xtsc_memory_trace to connect
   *                            the xtsc_pin2tlm_memory_transactor with.
   *
   * @param     single_connect  If true only one slave port pair of this
   *                            xtsc_memory_trace will be connected.  If false, the
   *                            default, then all contiguous, unconnected slave port
   *                            pairs of this xtsc_memory_trace starting at trace_port
   *                            that have a corresponding existing master port pair in
   *                            pin2tlm (starting at tran_port) will be connected with
   *                            that corresponding pin2tlm master port pair.
   */
  xtsc::u32 connect(xtsc_pin2tlm_memory_transactor&     pin2tlm,
                    xtsc::u32                           tran_port       = 0,
                    xtsc::u32                           trace_port      = 0,
                    bool                                single_connect  = false);


  /**
   * Connect with an upstream xtsc_router.
   *
   * This method connects the specified master port pair of the specified upstream xtsc_router
   * with the specified slave port pair of this xtsc_memory_trace.
   *
   * @param     router          The upstream xtsc_router to connect with.
   *
   * @param     router_port     The master port pair of the upstream xtsc_router to connect
   *                            with.  router_port must be in the range of 0 to the upstream
   *                            xtsc_router's "num_slaves" parameter minus 1.
   *
   * @param     trace_port      The slave port pair of this xtsc_memory_trace to connect
   *                            with.  trace_port must be in the range of 0 to this
   *                            xtsc_memory_trace's "num_ports" parameter minus 1.
   */
  void connect(xtsc_router& router, xtsc::u32 router_port, xtsc::u32 trace_port);


  /// Get the TextLogger for this component (e.g. to adjust its log level)
  log4xtensa::TextLogger& get_text_logger() { return m_text; }


protected:


  typedef xtsc::xtsc_request::type_t type_t;
  typedef xtsc::u64                  u64;


  /**
   * Common helper method to determine the all_types flag and populate the types_set
   * container.
   *
   * This method returns true if types is the empty string or "*" (both indicate all
   * types).  Otherwise it returns false and populates the types_set container with each
   * xtsc::xtsc_request::type_t type whose string name is listed in types.
   *
   * @param     types           A comma separated list of the xtsc::xtsc_request::type_t
   *                            types desired (for example, "READ,BLOCK_READ").  Both
   *                            the empty string or asterisk ("" or "*") mean all types.
   *
   * @param     types_set       If types is neither "" nor "*", then this container will
   *                            be populated with the xtsc::xtsc_request::type_t values
   *                            found in types.
   */
  bool get_types(const std::string& types, std::set<type_t>& types_set);


  /**
   * Common helper method to determine the all_ports flag and populate the ports_set
   * container.
   *
   * This method returns true if ports is the empty string or "*" (both indicate all
   * ports).  Otherwise it returns false and populates the ports_set container with each
   * port listed in ports.
   *
   * @param     ports           A comma separated list of the desired ports (for example,
   *                            "1,3").  Both the empty string or asterisk ("" or "*")
   *                            mean all ports.
   *
   * @param     ports_set       If ports is neither "" nor "*", then this container will
   *                            be populated with the port values found in ports.
   */
  bool get_ports(const std::string& ports, std::set<xtsc::u32>& ports_set);


  /// Common helper method used by dump_latency_histogram and dump_lifetime_histogram
  void dump_histogram(std::ostream& os = std::cout, const std::string& types = "", const std::string& ports = "", bool latency = true);


  // Clear the transaction list when "enable_latency_tracking" is set to false
  void clear_transaction_list();
    
    
  /**
   * This class is used to keep track of 4 key times during each transaction's lifecycle
   * in order to compute transaciton lifetime and latency.
   *
   * When the transaction is complete, these 4 times and the XTSC System Clock Period
   * (SCP) are used to calculate the transaction's lifetime and latency like this:
   *  \verbatim
     lifetime = (m_time_rsp_end - m_time_req_beg + SCP/2) / SCP
     latency  = (m_time_rsp_beg - m_time_req_end + SCP/2) / SCP
      \endverbatim
   */
  class transaction_info {
  public:
    transaction_info(sc_core::sc_time timestamp, type_t type);  ///< Constructor
    sc_core::sc_time    m_time_req_beg;                         ///< Transaction request  begin time
    sc_core::sc_time    m_time_req_end;                         ///< Transaction request  end   time
    sc_core::sc_time    m_time_rsp_beg;                         ///< Transaction response begin time
    sc_core::sc_time    m_time_rsp_end;                         ///< Transaction response end   time
    type_t              m_type;                                 ///< Transaction request type
  };
  

  /// Get a new transaction_info (from the pool)
  transaction_info *new_transaction_info(type_t type);
  

  /// Delete an transaction_info (return it to the pool)
  void delete_transaction_info(transaction_info*& p_transaction_info);


  /// enum for the various counters in the statistic_info class
  enum cntr_type {
    cntr_transactions = 0,      ///< "transactions": Total transactions
    cntr_latency,               ///< "latency":      Total latency of all transactions
    cntr_lifetime,              ///< "lifetime":     Total lifetime of all transactions
    cntr_req_beats,             ///< "req_beats":    Total request beats
    cntr_req_busys,             ///< "req_busys":    Total request beats that get RSP_NACC
    cntr_rsp_beats,             ///< "rsp_beats":    Total response beats (does NOT include RSP_NACC)
    cntr_rsp_busys,             ///< "rsp_busys":    Total response beats that get rejected
    cntr_count,
  };


  /// Dump the string version of all the counter names
  static void dump_counter_names(std::ostream& os);


  /// Return the string version of the specified counter type
  static std::string get_cntr_type_name(cntr_type type);


  /// Return the specified counter type given its string name
  static cntr_type get_cntr_type(std::string name);


  /**
   * This class is used to keep track of transaction statistics.
   *
   * The xtsc_memory_trace class uses m_statistics_maps which has one map for each port.
   * Each map maps from an xtsc_request::type_t to a statistic_info object used to keep
   * track of statistics for that request type.
   *
   * @see m_statistics_maps
   * @see transaction_info
   */
  class statistic_info {
  public:
    statistic_info();
    bool dump(std::ostream& os = std::cout, const std::string& prefix = "");
    xtsc::u64           m_max_latency;          ///< Maximum latency
    xtsc::u64           m_max_lifetime;         ///< Maximum lifetime
    xtsc::u64           m_max_latency_tag;      ///< Transaction tag with maximum latency
    xtsc::u64           m_max_lifetime_tag;     ///< Transaction tag with maximum lifetime
    xtsc::u64           m_cntrs[cntr_count];    ///< Array of counters
    std::map<u64, u64>  m_latency_histogram;    ///< Map latency  (first) to count (second)
    std::map<u64, u64>  m_lifetime_histogram;   ///< Map lifetime (first) to count (second)
  };  
  
  
  /// Implementation of xtsc_request_if.
  class xtsc_request_if_impl : public xtsc::xtsc_request_if, public sc_core::sc_object  {
  public:

    /**
     * Constructor.
     * @param   object_name     The name of this SystemC channel (aka implementation)
     * @param   trace           A reference to the owning xtsc_memory_trace object.
     * @param   port_num        The port number that this object represents.
     */
    xtsc_request_if_impl(const char *object_name, xtsc_memory_trace& trace, xtsc::u32 port_num);

    /// @see xtsc::xtsc_debug_if
    virtual void nb_peek(xtsc::xtsc_address address8, xtsc::u32 size8, xtsc::u8 *buffer);

    /// @see xtsc::xtsc_debug_if
    virtual void nb_poke(xtsc::xtsc_address address8, xtsc::u32 size8, const xtsc::u8 *buffer);

    /// @see xtsc::xtsc_debug_if
    virtual bool nb_peek_coherent(xtsc::xtsc_address    virtual_address8,
                                  xtsc::xtsc_address    physical_address8,
                                  xtsc::u32             size8,
                                  xtsc::u8             *buffer);

    /// @see xtsc::xtsc_debug_if
    virtual bool nb_poke_coherent(xtsc::xtsc_address    virtual_address8,
                                  xtsc::xtsc_address    physical_address8,
                                  xtsc::u32             size8,
                                  const xtsc::u8       *buffer);

    /// @see xtsc::xtsc_debug_if
    virtual bool nb_fast_access(xtsc::xtsc_fast_access_request &request);

    /// @see xtsc::xtsc_request_if
    virtual void nb_request(const xtsc::xtsc_request& request);

    /// @see xtsc::xtsc_request_if
    virtual void nb_load_retired(xtsc::xtsc_address address8);

    /// @see xtsc::xtsc_request_if
    virtual void nb_retire_flush();

    /// @see xtsc::xtsc_request_if
    virtual void nb_lock(bool lock);


    /// Return true if a port has bound to this implementation
    bool is_connected() { return (m_p_port != 0); }

  protected:

    /// SystemC callback when something binds to us
    virtual void register_port(sc_core::sc_port_base& port, const char *if_typename);

    xtsc_memory_trace&          m_trace;                ///<  Our xtsc_memory_trace object
    sc_core::sc_port_base      *m_p_port;               ///<  Port that is bound to us
    xtsc::u32                   m_port_num;             ///<  Our port number

    // The following are to allow tracing of each nb_request call
    xtsc::u64                   m_nb_request_count;     ///<  Count the nb_request calls on each port
    xtsc::u64                   m_address8;             ///<  Byte address 
    sc_dt::sc_unsigned          m_data;                 ///<  Data 
    xtsc::u32                   m_size8;                ///<  Byte size of each transfer
    xtsc::u32                   m_pif_attribute;        ///<  PIF request attribute of each transfer
    xtsc::u32                   m_route_id;             ///<  Route ID for arbiters
    xtsc::u8                    m_type;                 ///<  Request type (READ, BLOCK_READ, etc)
    xtsc::u8                    m_burst;                ///<  Request burst type (NON_AXI, FIXED, INCR, WRAP)
    xtsc::u8                    m_cache;                ///<  AXI: AxCACHE.
    xtsc::u8                    m_domain;               ///<  AXI: AxDOMAIN.   non-AXI: PIF request domain (0xFF => not set).  2 bits.
    xtsc::u8                    m_prot;                 ///<  AXI: AxPROT [0]=Priveleged access, [1]=Non-secure access, [2]=Instruction access
    xtsc::u8                    m_snoop;                ///<  AXI: AxSNOOP.
    xtsc::u32                   m_num_transfers;        ///<  Number of transfers
    xtsc::u16                   m_byte_enables;         ///<  Byte enables
    xtsc::u8                    m_id;                   ///<  PIF ID
    xtsc::u8                    m_priority;             ///<  Transaction priority
    bool                        m_last_transfer;        ///<  True if last transfer of request
    xtsc::u32                   m_pc;                   ///<  Program counter associated with request 
    xtsc::u64                   m_tag;                  ///<  Unique tag per request-response set
    bool                        m_instruction_fetch;    ///<  True if request is for an instruction fetch, otherwise false
    xtsc::u64                   m_hw_address8;          ///<  Address that would appear in hardware.
    xtsc::u32                   m_transfer_num;         ///<  Number of this transfer (request beat).
    xtsc::u32                   m_transaction_id;       ///<  Transaction ID
    
  };


  /// Implementation of xtsc_respond_if
  class xtsc_respond_if_impl : virtual public xtsc::xtsc_respond_if, public sc_core::sc_object  {
  public:

    /**
     * Constructor.
     * @param   object_name     The name of this SystemC channel (aka implementation)
     * @param   trace           A reference to the owning xtsc_memory_trace object.
     * @param   port_num        The port number that this object represents.
     */
    xtsc_respond_if_impl(const char *object_name, xtsc_memory_trace& trace, xtsc::u32 port_num);

    /// @see xtsc::xtsc_respond_if
    bool nb_respond(const xtsc::xtsc_response& response);

    
  protected:

    /// SystemC callback when something binds to us
    virtual void register_port(sc_core::sc_port_base& port, const char *if_typename);

    xtsc_memory_trace&          m_trace;                ///<  Our xtsc_memory_trace object
    sc_core::sc_port_base      *m_p_port;               ///<  Port that is bound to us
    xtsc::u32                   m_port_num;             ///<  Our port number

    // The following are to allow tracing of each nb_respond call
    xtsc::u64                   m_nb_respond_count;     ///<  Count the nb_respond calls on each port
    xtsc::u64                   m_rsp_busy_count;       ///<  Count the nb_respond calls on each port that are rejected (return false)
    xtsc::u64                   m_rsp_ok_count;         ///<  Count the nb_respond calls on each port that are RSP_OK
    xtsc::u64                   m_rsp_nacc_count;       ///<  Count the nb_respond calls on each port that are RSP_NACC
    xtsc::u64                   m_rsp_data_err_count;   ///<  Count the nb_respond calls on each port that are RSP_DATA_ERROR
    xtsc::u64                   m_rsp_addr_err_count;   ///<  Count the nb_respond calls on each port that are RSP_ADDRESS_ERROR
    xtsc::u64                   m_rsp_a_d_err_count;    ///<  Count the nb_respond calls on each port that are RSP_ADDRESS_DATA_ERROR
    xtsc::u64                   m_address8;             ///<  Starting byte address
    sc_dt::sc_unsigned          m_data;                 ///<  Data
    xtsc::u32                   m_size8;                ///<  Byte size of each transfer
    xtsc::u32                   m_route_id;             ///<  Route ID for arbiters
    xtsc::u8                    m_status;               ///<  Response status
    xtsc::u8                    m_burst;                ///<  Request burst type (NON_AXI, FIXED, INCR, WRAP)
    xtsc::u8                    m_id;                   ///<  PIF ID
    xtsc::u8                    m_priority;             ///<  Transaction priority
    bool                        m_last_transfer;        ///<  True if last transfer of response
    xtsc::u32                   m_pc;                   ///<  Program counter associated with request
    xtsc::u64                   m_tag;                  ///<  Unique tag per request-response set
    bool                        m_snoop;                ///<  True if this is a snoop response, otherwise false (future use)
    bool                        m_snoop_data;           ///<  True if this is a snoop response with data, otherwise false (future use)
    xtsc::u32                   m_transfer_num;         ///<  Number of this transfer (response beat).
    xtsc::u32                   m_transaction_id;       ///<  Transaction ID

  };



  xtsc_request_if_impl                **m_request_impl;                 ///<  m_request_exports bind to these
  xtsc_respond_if_impl                **m_respond_impl;                 ///<  m_respond_exports bind to these

  xtsc::u32                             m_width8;                       ///<  The byte width of the memory data interface
  bool                                  m_big_endian;                   ///<  Swizzle the bytes in the request/response data bufer
  sc_core::sc_trace_file               *m_p_trace_file;                 ///<  The VCD object
  xtsc::u32                             m_num_ports;                    ///<  The number of memory ports
  xtsc::u32                             m_num_transfers;                ///<  See "num_transfers" parameter
  bool                                  m_allow_tracing;                ///<  See "allow_tracing" parameter
  bool                                  m_enable_tracing;               ///<  See "enable_tracing" parameter enable_tracing method/command
  bool                                  m_track_latency;                ///<  See "track_latency" parameter and enable_latency_tracking method/command
  bool                                  m_did_track;                    ///<  True if m_track_latency was ever true.
  std::vector<transaction_info*>        m_transaction_pool;             ///<  Maintain a pool of transaction_info objects to improve performance
  std::map<u64, transaction_info*>      m_pending_transactions;         ///<  Map of all transactions in progress
  std::map<type_t, statistic_info*>    *m_statistics_maps;              ///<  Map request type to statistic_info (one map per port)
  sc_core::sc_time                      m_system_clock_period;          ///<  The XTSC System Clock Period (SCP)
  sc_core::sc_time                      m_system_clock_period_half;     ///<  One-half of the XTSC System Clock Period (SCP/2)
  
  log4xtensa::TextLogger&               m_text;                         ///<  Text logger


};



}  // namespace xtsc_component




#endif  // _XTSC_MEMORY_TRACE_H_
