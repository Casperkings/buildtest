#ifndef _XTSC_PIF2AXI_TRANSACTOR_H_
#define _XTSC_PIF2AXI_TRANSACTOR_H_

// Copyright (c) 2006-2020 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
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
#include <deque>
#include <queue>
#include <map>


namespace xtsc_component {

/**
 * Constructor parameters for a xtsc_pif2axi_transactor object.
 *
 *  \verbatim
   Name                        Type   Description
   ------------------          ----   ---------------------------------------------------

   "allow_axi_wrap"            bool   Allow AXI WRAP transactions to be generated for 
                                      BLOCK_WRITE and BLOCK_READ requests for a core PIF
                                      request ie when 'memory_interface' is PIF. If 
                                      set to false, AXI INCR requests will be generated
                                      for BLOCK_WRITE and BLOCK_READ. For idma0 requests 
                                      ie when 'memory_interface' is iDMA0, AXI INCR 
                                      requests are generated always.
                                      Default = true.

   "axi_byte_width"            char*  Master AXI bus width in bytes. It is required to 
                                      be set only if width conversion is also required.
                                      Otherwise it assumes the value of 'pif_byte_width'.
                                      Valid values if set, are  4, 8, 16, 32.               

   "clock_period"              u32    This is the length of this transactor's clock 
                                      period expressed in terms of the SystemC time 
                                      resolution (from sc_get_time_resolution()).  A 
                                      value of 0xFFFFFFFF means to use the XTSC system 
                                      clock period (from xtsc_get_system_clock_period()).
                                      A value of 0 means one delta cycle.
                                      Default = 0xFFFFFFFF (use the system clock period).
  
   "interleave_responses"      bool   If set to true, the transactor allows interleaving
                                      of read and write responses. If this feature is 
                                      supported by the master generating PIF requests, 
                                      master can use the response ID to identify and 
                                      differentate responses.  In PIF 4.0, cycles from 
                                      different PIF responses can be interleaved and the 
                                      PIF ID must be used to associate each PIF cycle with
                                      a specific response. In PIF versions 3.0, 3.1, and 
                                      3.2, multi-cycle PIF responses cannot be interleaved 
                                      with each other. So, this parameter should be 
                                      disabled in case the master is not PIF 4.0 
                                      compliant.
                                      Default = true.

   "maintain_order"            bool   If set to true, the transactor will block all read 
                                      requests from being issued on AXI until all earlier  
                                      write requests have completed, where completion is  
                                      defined by an AXI write response. This attempts to 
                                      maintain the  PIF request ordering across the 
                                      different AXI channels. It is theoretically possible 
                                      that a write may be issued to the same address as an 
                                      outstanding read.  But, Xtensa LX[2] processor loads 
                                      are blocking , and no stores will be issued while a 
                                      valid read is outstanding.
                                      Default = false.

   "max_outstanding"           u32    The maximum number of outstanding AXI requests that      
                                      can be issued on each read and write channel
                                      seperately. May need to be adjusted if transactor is
                                      also doing width conversion with 'pif_byte_width' >
                                      'axi_byte_width'. 
                                      Default = 16. 

   "memory_interface"          char*  The memory interface type.  Valid values are "PIF" 
                                      and "IDMA0" (LX) (case-insensitive).
                                      Default = "PIF".

   "nacc_wait_time"            u32    This parameter, expressed in terms of the SystemC 
                                      time resolution, specifies how long to wait after
                                      sending a request downstream to see if it was 
                                      rejected by RSP_NACC.  This value must not exceed
                                      this transactor's clock period.  A value of 0 means
                                      one delta cycle. A value of 0xFFFFFFFF means to wait 
                                      for a period equal to this transactor's clock period. 
                                      CAUTION:  A value of 0 can cause an infinite loop 
                                      in the simulation if the downstream module requires 
                                      a non-zero time to become available.
                                      Default = 0xFFFFFFFF (transactor's clock period).
                                      
   "num_ports"                 u32    The number of Xtensa PIF port pairs this transactor
                                      has. For each enabled pif port a pair of read(data 
                                      and instruction read) and write ports are exposed 
                                      on the master interface. Valid values are values 
                                      greater than zero.   
                                      Default = 1.

   "pif_byte_width"            u32    Slave PIF Bus width in bytes. Valid values are 4, 8, 
                                      16. Additionally 32 byte_width is supported if the 
                                      transactor is connected to iDMA interface ie 
                                      'memory_interface' is set to iDMA0.
                                      Default = 4.

   "read_response_delay"       u32    The delay inserted by the transactor in the read 
                                      response path. So, effectively the pif master sees 
                                      the response after the delay of 
                                      "read_response_delay" cycles from the time it was 
                                      issued by the slave.  If there is a previous PIF 
                                      response pending acceptance from the master, this 
                                      delay will be invisible/consumed by the time its 
                                      turn comes for the dispatch. Read responses are 
                                      combinationally issued from AXI to the PIF. In 
                                      the case of width conversion between PIF and AXI, 
                                      because of the extra translation of responses 
                                      required, there can be additional cycles of latency 
                                      to read responses. 
                                      Default = 0.

   "read_request_delay"        u32    The delay inserted by the the transactor in issuing 
                                      input read request to the axi slave. So, efectively 
                                      the axi slave sees this request after these delay
                                      cycles. If there is a previous AXI request pending 
                                      acceptance from the slave port, this delay will be 
                                      invisible/ consumed by the time its turn comes for 
                                      the dispatch.
                                      Default = 1. 

   "read_request_fifo_depth"   u32    The read request fifo depth. The depth will 
                                      determine the read requests the transactor can 
                                      buffer if downstream AXI slave is not accepting 
                                      futher read requests. Valid values are values 
                                      greater than zero. A value of 1 means ony one 
                                      request can be present in the fifo at a time and no
                                      further requests will be accepted until that request
                                      is sent out to AXI.
                                      Default = 2. 

   "wait_on_outstanding_write" bool   If set to true, the transactor guarantees that read
                                      requests will not be issued on AXI until after all 
                                      previous writes to the same address have completed 
                                      on AXI, where completion is defined by an AXI write 
                                      response.  While in some ways 'maintain_order' holds 
                                      back read requests if older writes are outstanding, 
                                      this paramameter checks for only older writes with
                                      the same issued address.
                                      Default = true.

   "write_response_delay"      bool   The delay inserted by the transactor in the write 
                                      response path. So, efectively the pif master sees the
                                      response after the delay of "write_response_delay"
                                      cycles from the time it was received by the transactor.
                                      Write responses are registered and buffered in the 
                                      transactor.
                                      Default = 1.

   "write_request_delay"       u32    The delay inserted by the the transactor in issuing 
                                      input write request to the axi slave. So, effectively
                                      the axi slave sees this request after the these
                                      delay cycles. If there is a previous AXI request 
                                      pending acceptance from the slave port, this delay 
                                      will be invisible/consumed by the time its turn comes 
                                      for the dispatch.
                                      Default = 2. 

   "write_request_fifo_depth"  u32    The write request fifo depth. The depth will 
                                      determine the write requests the transactor can 
                                      buffer if downstream AXI slave is not accepting 
                                      futher write requests. Valid values are values 
                                      greater than zero. A value of 1 means ony one 
                                      request can be present in the fifo at a time and no
                                      further requests will be accepted until that request
                                      is sent out to AXI.
                                      Default = 3.


    \endverbatim
 *
 * @see xtsc_pif2axi_transactor
 * @see xtsc::xtsc_parms
 */
class XTSC_COMP_API xtsc_pif2axi_transactor_parms : public xtsc::xtsc_parms {
public:

  /**
   * Constructor for an xtsc_pif2axi_transactor_parms object.
   *
   * @param memory_interface    The memory interface name (port name).
   *
   * @param pif_byte_width      PIF data bus width in bytes.
   *
   * @param axi_byte_width      AXI data bus width in bytes.
   *
   * @param num_ports           The number of ports this transactor has.
   *
   * @see xtsc::xtsc_core::How_to_do_memory_port_binding for a list of legal
   *      memory_interface values.
   */
  xtsc_pif2axi_transactor_parms(const char *memory_interface = "PIF", xtsc::u32 pif_byte_width = 4, 
                                xtsc::u32 num_ports = 1) {
    init(memory_interface, pif_byte_width, num_ports);
  }


  /**
   * Constructor for an xtsc_pif2axi_transactor_parms object based upon an xtsc_core
   * object and a named memory interface. 
   *
   * This constructor will determine pif_width8 and, optionally, num_ports by querying
   * the core object and then pass the values to the init() method.  In addition, the
   * "clock_period" parameter will be set to match the core's clock period.  If desired,
   * after the xtsc_pif2axi_transactor_parms object is constructed, its data members
   * can be changed using the appropriate xtsc_parms::set() method before passing it to
   * the xtsc_pif2axi_transactor constructor.
   *
   * @param core                A reference to the xtsc_core object upon which to base
   *                            the xtsc_pif2axi_transactor_parms.
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
  xtsc_pif2axi_transactor_parms(const xtsc::xtsc_core& core, const char *memory_interface, xtsc::u32 num_ports = 0);


  /**
   * Do initialization common to both constructors.
   */
  void init(const char *memory_interface = "PIF", xtsc::u32 pif_byte_width = 4, xtsc::u32 num_ports = 1) {
    add("allow_axi_wrap",            true);
    add("axi_byte_width",            ""); 
    add("clock_period",              0xFFFFFFFF);
    add("interleave_responses",      true);
    add("maintain_order",            false);
    add("max_outstanding",           16);
    add("memory_interface",          memory_interface);
    add("nacc_wait_time",            0xFFFFFFFF);
    add("num_ports",                 num_ports);
    add("pif_byte_width",            pif_byte_width);
    add("read_response_delay",       0);
    add("read_request_delay",        1);
    add("read_request_fifo_depth",   2);
    add("wait_on_outstanding_write", true);
    add("write_response_delay",      1);
    add("write_request_delay",       2);
    add("write_request_fifo_depth",  3);
  }


  /// Return what kind of xtsc_parms this is (our C++ type)
  virtual const char* kind() const { return "xtsc_pif2axi_transactor_parms"; }

};



/**
 * Module implementing an Xtensa PIF (xttlm) to Xtensa AXI transactor.
 *
 * This module can be used to connect an Xtensa PIF memory interface master (for example,
 * xtsc::xtsc_core(Xtensa LX), xtsc_component::xtsc_arbiter, etc.) to an Xtensa AXI memory 
 * interface slave having seperate read and write xtensa ports (for example, xtsc_memory).
 *
 * For protocol and timing information specific to xtsc::xtsc_core and the Xtensa ISS,
 * see xtsc::xtsc_core::Information_on_memory_interface_protocols.
 *
 * Here is a block diagram of xtsc_pif2axi_transactor objects being used in the
 * xtsc_pif2axi_transactor example:
 * @image html  Example_xtsc_pif2axi_transactor.jpg
 * @image latex Example_xtsc_pif2axi_transactor.eps "xtsc_pif2axi_transactor Example" width=10cm
 *
 * @see xtsc_pif2axi_transactor_parms
 * @see xtsc::xtsc_request
 * @see xtsc::xtsc_response
 * @see xtsc::xtsc_request_if
 * @see xtsc::xtsc_respond_if
 * @see xtsc::xtsc_core
 * @see xtsc::xtsc_core::Information_on_memory_interface_protocols.
 */
class XTSC_COMP_API xtsc_pif2axi_transactor : public sc_core::sc_module,
                                              public xtsc::xtsc_module,
                                              public xtsc::xtsc_mode_switch_if,
                                              public xtsc::xtsc_command_handler_interface
{
public:

  sc_core::sc_export<xtsc::xtsc_request_if>   **m_request_exports;       ///<  From upstream master(s) to us
  sc_core::sc_port  <xtsc::xtsc_respond_if>   **m_respond_ports;         ///<  From us to upstream master(s)

  sc_core::sc_port  <xtsc::xtsc_request_if>   **m_read_request_ports;    ///<  From us to downstream slave(s) 
  sc_core::sc_export<xtsc::xtsc_respond_if>   **m_read_respond_exports;  ///<  From downstream slave(s) to us
  sc_core::sc_port  <xtsc::xtsc_request_if>   **m_write_request_ports;   ///<  From us to downstream slave(s) 
  sc_core::sc_export<xtsc::xtsc_respond_if>   **m_write_respond_exports; ///<  From downstream slave(s) to us


  // For SystemC
  SC_HAS_PROCESS(xtsc_pif2axi_transactor);

  /// Our C++ type (SystemC uses this)
  virtual const char* kind() const { return "xtsc_pif2axi_transactor"; }


  /**
   * Constructor for an xtsc_pif2axi_transactor.
   * @param     module_name     Name of the xtsc_pif2axi_transactor sc_module.
   * @param     parms           The remaining parameters for construction.
   * @see xtsc_pif2axi_transactor_parms
   */
  xtsc_pif2axi_transactor(sc_core::sc_module_name module_name, const xtsc_pif2axi_transactor_parms& parms);


  // Destructor.
  ~xtsc_pif2axi_transactor(void);


  /// For sc_core::sc_moduleinterface. Simulation callback.
  virtual void end_of_simulation(void);


  /// For xtsc_connection_interface
  virtual xtsc::u32 get_bit_width(const std::string& port_name, xtsc::u32 interface_num = 0) const;


  /// For xtsc_connection_interface
  virtual sc_core::sc_object *get_port(const std::string& port_name);


  /// For xtsc_connection_interface
  virtual xtsc::xtsc_port_table get_port_table(const std::string& port_table_name) const;


  /// for xtsc_mode_switch_if
  virtual bool prepare_to_switch_sim_mode(xtsc::xtsc_sim_mode mode);


  /// For xtsc_mode_switch_if
  virtual bool switch_sim_mode(xtsc::xtsc_sim_mode mode);


  /// For xtsc_mode_switch_if
  virtual sc_core::sc_event &get_sim_mode_switch_ready_event() { return m_ready_to_switch_event; }


  /// For xtsc_mode_switch_if
  virtual xtsc::xtsc_sim_mode get_sim_mode() const { return m_sim_mode;};


  /// For xtsc_mode_switch_if
  virtual bool is_mode_switch_pending() const { return m_mode_switch_pending; }

  
  /**
   * Implementation of the xtsc::xtsc_command_handler_interface.
   *
   * This implementation supports the following commands:
   *  \verbatim
        change_clock_period <ClockPeriodFactor>
          Call xtsc_pif2axi_transactor::change_clock_period(<ClockPeriodFactor>).
          Return previous <ClockPeriodFactor> for this device.

        peek <PortNumber> <StartAddress> <NumBytes>
          Peek <NumBytes> of memory starting at <StartAddress>.

        poke <PortNumber> <StartAddress> <NumBytes> <Byte1> <Byte2> . . . <ByteN>
          Poke <NumBytes> (=N) of memory starting at <StartAddress>.

        reset [<Hard>]
          Call xtsc_pif2axi_transactor::reset().
      \endverbatim
   */
  virtual void execute(const std::string&       cmd_line,
               const std::vector<std::string>&  words,
               const std::vector<std::string>&  words_lc,
               std::ostream&                    result);


  /**
   * Implementation of the xtsc::xtsc_module
   *
   * Reset the transactor.
   */
  virtual void reset(bool hard_reset = false);


  /**
   * Method to change the clock period.
   *
   * @param     clock_period_factor     Specifies the new length of this device's clock
   *                                    period expressed in terms of the SystemC time
   *                                    resolution (from sc_get_time_resolution()).
   */
  void change_clock_period(xtsc::u32 clock_period_factor);


  /// Return pif byte width of data interface (from "pif_byte_width" parameter)
  xtsc::u32 get_pif_byte_width() const { return m_pif_width8; }


  /// Return axi byte width of data interface (from "axi_byte_width" parameter)
  xtsc::u32 get_axi_byte_width() const { return m_axi_width8; }


  /// Return the number of memory ports this transactor has (from "num_ports" parameter)
  xtsc::u32 get_num_ports() const { return m_num_ports; }


  /// Dispatch the axi request to read port
  void burst_read_worker(xtsc::xtsc_request*, xtsc::u32 port_num);


  /// Dispatch the axi request to write port
  void burst_write_worker(xtsc::xtsc_request*, xtsc::u32 port_num);


  /// Return true if this is a READ|BLOCK_READ|BURST_READ|RCW1
  bool is_read_access(const xtsc::xtsc_request* p_request) {
    return (p_request->is_read());
  }


  /// Check if there is any outstanding write request with the address as used by p_axi_requests 
  bool check_outstanding_write(xtsc::xtsc_request* p_axi_request, xtsc::u32 port_num);


  /// Convert PIF read requests if size8 of PIF request > axi data width
  void convert_to_narrow_burst_reads(xtsc::xtsc_request*, xtsc::u32, std::vector<xtsc::xtsc_request*>&);


  /// Convert PIF write requests of size8 of PIF request > axi data width
  void convert_to_narrow_burst_writes(xtsc::xtsc_request*, xtsc::u32, std::vector<xtsc::xtsc_request*>&);


  /// Get the TextLogger for this component (e.g. to adjust its log level)
  log4xtensa::TextLogger& get_text_logger() { return m_text; }


private:

  /// Implementation of xtsc_request_if.
  class xtsc_request_if_impl : public xtsc::xtsc_request_if, public sc_core::sc_object {
  public:

    /// Constructor
    xtsc_request_if_impl(const char *object_name, xtsc_pif2axi_transactor& transactor, xtsc::u32 port_num) :
      sc_object         (object_name),
      m_transactor      (transactor),
      m_port_num        (port_num),
      m_p_port          (0)
    {}

    /// The kind of sc_object we are
    const char* kind() const { return "xtsc_pif2axi_transactor::xtsc_request_if_impl"; }


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
    virtual void nb_request(const xtsc::xtsc_request& request);
    

    /// Worker function for nb_request. Pushes requests into queues
    void nb_request_worker(xtsc::xtsc_request* request);


    /// Return true if a port has been bound to this implementation
    bool is_connected() { return (m_p_port != 0); }


  protected:

    /// SystemC callback when something binds to us
    virtual void register_port(sc_core::sc_port_base& port, const char *if_typename);

    xtsc_pif2axi_transactor    &m_transactor;  ///<  Our xtsc_pif2axi_transactor object
    xtsc::u32                   m_port_num;    ///<  Our port number
    sc_core::sc_port_base      *m_p_port;      ///<  Port that is bound to us
  };


  /// Implementation of xtsc_respond_if.
  class xtsc_respond_if_impl : public xtsc::xtsc_respond_if, public sc_core::sc_object {
  public:

    /// Constructor
    xtsc_respond_if_impl(const char *object_name, xtsc_pif2axi_transactor& transactor, xtsc::u32 port_num, 
                         std::string type) :
      sc_object         (object_name),
      m_transactor      (transactor),
      m_port_num        (port_num),
      m_p_port          (0),
      m_type            (type)
    {}

    /// The kind of sc_object we are
    const char* kind() const { return "xtsc_pif2axi_transactor::xtsc_respond_if_impl"; }

    /// From downstream slave
    /// @see xtsc::xtsc_respond_if
    bool nb_respond(const xtsc::xtsc_response& response);

    /// Return true if a port has been already bound to this implementation
    bool is_connected() { return (m_p_port != 0); }


  protected:

    /// SystemC callback when something binds to us
    virtual void register_port(sc_core::sc_port_base& port, const char *if_typename);

    xtsc_pif2axi_transactor  &m_transactor;  ///<  Our xtsc_pif2axi_transactor object
    xtsc::u32                 m_port_num;    ///<  Our port number
    sc_core::sc_port_base    *m_p_port;      ///<  Port that is bound to us
    std::string               m_type;        ///<  Type of response : READ/WRITE
  };


  // Class to keep a track of a PIF request and its scheduled time after accounting request_delay
 class req_sched_info {
 public:
   xtsc::xtsc_request*  m_pif_request;
   sc_core::sc_time  m_sched_time;
 };


  // Class to keep a track of a PIF request(s) associated witha tag and its response related metadata
 class req_rsp_info {
 public:
   // Constructor
   req_rsp_info() {
     init();
   }

   void init() {
     m_pif_request_vec.clear();
     m_active_read_response    = NULL;    
     m_pif_resp_data_offset    = 0;
     m_num_pif_resp_sent       = 0;
     m_num_expected_write_resp = 0;
     m_rcw_first_transfer_rcvd = false;
     m_excl_lock_supported     = false;
     m_excl_read_match         = false;
   }

   std::vector<xtsc::xtsc_request*>  m_pif_request_vec;              ///< vector of PIF SINGLE/BURST/BLOCK requests
   xtsc::xtsc_response              *m_active_read_response;         ///< Active PIF response, being populated with axi responses 
   xtsc::u32                         m_pif_resp_data_offset;         ///< Offset while creating PIF READ response from AXI READ responses
   xtsc::u32                         m_num_pif_resp_sent;            ///< Number of READ/BURST_READ/BLOCK_READ responses sent 
   xtsc::u32                         m_num_expected_write_resp;      ///< Number of AXI BURST_WRITEs converted from this request
   xtsc::u32                         m_rcw_compare_data;             ///< RCW compare data
   bool                              m_rcw_first_transfer_rcvd;      ///< Flag for RCW first transfer
   bool                              m_excl_lock_supported;          ///< Check if slave supports exclusive accesses
   bool                              m_excl_read_match;              ///< Flag to check if the Excl AXI READ data matches m_rcw_compare_data
 };


  /// Get a new copy of an xtsc_request (from the pool)
  xtsc::xtsc_request *copy_request(const xtsc::xtsc_request& request);


  /// Get a new xtsc_request (from the pool)
  xtsc::xtsc_request *new_request();


  /// Delete an xtsc_request (return it to the pool)
  void delete_request(xtsc::xtsc_request*& p_request);


  /// Get a new req_rsp_info (from the pool)
  req_rsp_info *new_req_rsp_info();


  /// Delete a req_rsp_info (return it to the pool)
  void delete_req_rsp_info(req_rsp_info*& p_req_rsp_info);


  /// SystemC thread to handle read requests to each target
  void read_request_thread(void);


  /// SystemC thread to handle write requests to each target
  void write_request_thread(void);


  /// SystemC thread to handle incoming responses from each slave
  void arbitrate_response_thread(void);


  /// SystemC thread to handle write requests RSP_NACC handling
  void nacc_nb_write_request_thread(void);


  /// SystemC thread to handle read requests RSP_NACC handling
  void nacc_nb_read_request_thread(void);


  /// Common method for sending a response
  void send_response(xtsc::xtsc_response& response, xtsc::u32 port_num);
  
 
  /// Handle PIF requests of type READ/BLOCK_READ/BURST_READ and convert into AXI BURST_READ 
  void do_burst_read (xtsc::xtsc_request *p_pif_request, xtsc::u32 port_num); 


  /// Handle PIF requests of type RCW and convert into AXI exclusive accesses 
  void do_exclusive  (xtsc::xtsc_request *p_pif_request, xtsc::u32 port_num);      


  /// Handle PIF requests of type WRITE/BLOCK_WRITE/BURST_WRITE and convert into AXI BURST_WRIT 
  void do_burst_write(xtsc::xtsc_request *p_pif_request, xtsc::u32 port_num);      


  typedef tlm_utils::peq_with_get
                     <xtsc::xtsc_response>   peq;

  xtsc_request_if_impl                     **m_xtsc_request_if_impl;                ///<  m_request_exports bind to these
  xtsc_respond_if_impl                     **m_xtsc_read_respond_if_impl;           ///<  m_read_respond_exports bind to these
  xtsc_respond_if_impl                     **m_xtsc_write_respond_if_impl;          ///<  m_write_respond_exports bind to these
  const char                                *m_memory_interface_str;                ///<  See the "memory_interface" parameter
  xtsc::u32                                  m_pif_width8;                          ///<  See the "pif_byte_width"
  xtsc::u32                                  m_axi_width8;                          ///<  See the "axi_byte_width"
  sc_core::sc_time                           m_nacc_wait_time;                      ///<  See "nacc_wait_time" parameter
  xtsc::u32                                  m_num_ports;                           ///<  See the "num_ports" parameter
  xtsc::u32                                  m_read_request_fifo_depth;             ///<  See the "read_request_fifo_depth" parameter.
  xtsc::u32                                  m_write_request_fifo_depth;            ///<  See the "write_request_fifo_depth" parameter.
  xtsc::u32                                  m_read_request_delay;                  ///<  See the "read_request_delay" parameter.
  xtsc::u32                                  m_write_request_delay;                 ///<  See the "write_request_delay" parameter.
  xtsc::u32                                  m_read_response_delay;                 ///<  See the "read_response_delay" parameter.
  xtsc::u32                                  m_write_response_delay;                ///<  See the "write_response_delay" parameter.
  xtsc::u32                                  m_max_outstanding;                     ///<  See the "max_outstanding" parameter.
  bool                                       m_allow_axi_wrap;                      ///<  See the "allow_axi_wrap" parameter.
  bool                                       m_maintain_order;                      ///<  See the "maintain_order" parameter.
  bool                                       m_wait_on_outstanding_write;           ///<  See the "wait_on_outstanding_write" parameter.
  bool                                       m_interleave_responses;                ///<  See the "interleave_responses" parameter.
  bool                                       m_interface_is_idma0;                  ///<  Is this transactor converting idma0 transactions.
  xtsc::u32                                 *m_outstanding_reads;                   ///<  Number of outstanding read AXI requests.
  xtsc::u32                                 *m_outstanding_writes;                  ///<  Number of outstanding write AXI requests.
  xtsc::u32                                  m_next_read_request_port_num;          ///<  Used by request_thread to get its port number
  xtsc::u32                                  m_next_write_request_port_num;         ///<  Used by request_thread to get its port number
  xtsc::u32                                  m_next_response_port_num;              ///<  Used by respond_thread to get its port number
  xtsc::u32                                  m_next_nacc_nb_write_request_port_num; ///<  Used by nacc_nb_write_request_thread to get its port num
  xtsc::u32                                  m_next_nacc_nb_read_request_port_num;  ///<  Used by nacc_nb_read_request_thread to get its port num
  sc_core::sc_time                           m_clock_period;                        ///<  This transactor's clock period
  xtsc::u64                                  m_clock_period_value;                  ///<  Clock period as u64
  xtsc::u64                                 *m_tag_axi_write;                       ///<  Tag field used by axi write transfers following 1st transfer
  sc_core::sc_event                        **m_read_request_thread_event;           ///<  To notify read_request_thread when a request is accepted
  sc_core::sc_event                        **m_write_request_thread_event;          ///<  To notify write_request_thread when a request is accepted
  sc_core::sc_event                        **m_arbitrate_response_thread_event;     ///<  To notify arbitrate response_thread when a response is accepted from axi port
  sc_core::sc_event                        **m_nacc_nb_write_request_thread_event;  ///<  To notify nacc_nb_request_thread to check for potential RSP_NACC
  sc_core::sc_event                        **m_nacc_nb_read_request_thread_event;   ///<  To notify nacc_nb_request_thread to check for potential RSP_NACC
  sc_core::sc_event                        **m_outstanding_write_event;             ///<  Event to check outstanding write requests 
  sc_core::sc_event                          m_ready_to_switch_event;               ///<  Signal ready to switch simulation mode
  std::deque<req_sched_info*>              **m_read_request_fifo;                   ///<  The fifo for incoming read requests 
  std::deque<req_sched_info*>              **m_write_request_fifo;                  ///<  The fifo for incoming write requests 
  peq                                      **m_rd_response_peq;                     ///<  For arbitrate_response_thread
  peq                                      **m_wr_response_peq;                     ///<  For arbitrate_response_thread
  xtsc::xtsc_request                       **m_nacc_nb_write_request;               ///<  The write request to be used by nacc_nb_wrte_request_thread
  xtsc::xtsc_request                       **m_nacc_nb_read_request;                ///<  The read request to be used by nacc_nb_read_request_thread
  bool                                       m_mode_switch_pending;                 ///<  Indicate simulation mode switch is in progress
  bool                                      *m_read_waiting_for_nacc;               ///<  State where read port is waiting on the potential NACC
  bool                                      *m_write_waiting_for_nacc;              ///<  State where write port is waiting on the potential NACC
  bool                                      *m_read_request_got_nacc;               ///<  Indicate that read port has received NACC
  bool                                      *m_write_request_got_nacc;              ///<  Indicate that write port has received NACC
  bool                                      *m_response_locked;                     ///<  Semaphore for atomicity of READ* responses 
  bool                                      *m_excl_read_resp_rcvd;                 ///<  Indicate the receipt of exclusive read response
  std::map<xtsc::u64, std::vector
   <xtsc::xtsc_request*> >                 **m_tag_to_axi_request_map;              ///<  Map generated AXI requests for a tag
  std::map<xtsc::u64, req_rsp_info*>       **m_tag_to_req_rsp_info_map;             ///<  req_rsp_info lookup table, with tag as the key
  std::map<xtsc::u64, xtsc::u64>           **m_axi_tag_from_pif_tag_map;            ///<  Map for tags of axi requests created from pif requests 
  std::deque<xtsc::u64>                    **m_pif_request_order_dq;                ///<  PIF request ordered queue
  std::map<std::string, xtsc::u32>           m_bit_width_map;                       ///<  For xtsc_connection_interface
  std::vector<xtsc::xtsc_request*>           m_request_pool;                        ///<  Maintain a pool of xtsc_request to improve performance
  xtsc::u32                                  m_request_count;                       ///<  Count each newly created xtsc_request
  std::vector<req_rsp_info*>                 m_req_rsp_info_pool;                   ///<  Maintain a pool of req_rsp_info to improve performance
  xtsc::u32                                  m_req_rsp_info_count;                  ///<  Count each newly created req_rsp_info
  std::vector<sc_core::sc_process_handle>    m_process_handles;                     ///<  For reset 
  xtsc::xtsc_sim_mode                        m_sim_mode;                            ///<  Current Simulation mode of the transactor
  sc_core::sc_time                           m_time_resolution;                     ///<  SystemC time resolution
  log4xtensa::TextLogger&                    m_text;                                ///<  Text logger

};


}  // namespace xtsc_component

#endif  // _XTSC_PIF2AXI_TRANSACTOR_H_
