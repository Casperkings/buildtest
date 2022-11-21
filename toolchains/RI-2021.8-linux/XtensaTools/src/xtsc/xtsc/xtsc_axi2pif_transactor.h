#ifndef _XTSC_AXI2PIF_TRANSACTOR_H_
#define _XTSC_AXI2PIF_TRANSACTOR_H_

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
 * Constructor parameters for a xtsc_axi2pif_transactor object.
 *
 *  \verbatim
   Name                           Type   Description
   ---------------------------    ----   ---------------------------------------------------

   "axi_byte_width"               u32    Slave AXI bus width in bytes. It applies to both
                                         READ/WRITE port width. Exclusive transactions are
                                         treated as normal transactions. Valid values are 4,
                                         8, 16, 32 and 64.
                                         Default = 4.

   "clock_period"                 u32    This is the length of this transactor's clock 
                                         period expressed in terms of the SystemC time 
                                         resolution (from sc_get_time_resolution()).  A 
                                         value of 0xFFFFFFFF means to use the XTSC system 
                                         clock period (from xtsc_get_system_clock_period()).
                                         A value of 0 means one delta cycle.
                                         Default = 0xFFFFFFFF (use the system clock period).
 
   "critical_word_first"          bool   This parameter indicates if critical word first is
                                         enabled. If clear, AXI wrap read is converted into
                                         single READs. If set, critical_word_first is
                                         enabled. Thus, AXI wrap read is sent as READs or
                                         BLOCK_READ(s) depending on other parameters.
                                         Default = false.

   "outstanding_pif_request_ids"  u32    This is the maximum number of pif request ids which
                                         can be pending per pif port. Thus, at any given 
                                         moment, at maximum, "outstanding_pif_request_ids" read 
                                         request ids and "outstanding_pif_request_ids" write 
                                         request ids can be kept pending. All pif requests 
                                         for a particular pif write transaction use the same 
                                         request id. A request is considered completed once
                                         all its responses arrive us. Read request ids start
                                         from 32 and write request ids start from 0. 
                                         Appropriate request id is set before sending it to 
                                         forward block/core. If set to 0, no change in id is
                                         done and request is sent forward with the same id as
                                         send to us.
                                         Default = 8.
 
   "nacc_wait_time"               u32    This parameter, expressed in terms of the SystemC 
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
                                         
   "num_ports"                    u32    The number of Xtensa PIF ports this transactor has.
                                         For each enabled pif port, a pair of read(data and
                                         instruction read) and write ports are exposed on the
                                         slave interface. Minimum valid value is 1.   
                                         Default = 1.

   "read_request_delay"           u32    The delay inserted by the transactor in issuing 
                                         input read request to the pif slave. So, effectively 
                                         the pif slave sees this request atleast after these 
                                         delay cycles. If there is a previous PIF request 
                                         pending acceptance from the slave port, this delay 
                                         will be invisible/ consumed by the time its turn 
                                         comes for the dispatch.
                                         Default = 1.

   "read_request_fifo_depth"      u32    The read request fifo depth. The depth will 
                                         determine the read requests the transactor can 
                                         buffer if downstream pif slave is not accepting 
                                         futher read requests. Valid values are values 
                                         greater than zero. A value of 1 means ony one 
                                         request can be present in the fifo at a time and no
                                         further requests will be accepted until that request
                                         is sent out to AXI. Minimum valid value is 1.
                                         Default = 1.

   "read_response_delay"          u32    The delay inserted by the transactor in the read 
                                         response path. So, effectively the axi master sees 
                                         the response after the delay of 
                                         "read_response_delay" cycles from the time it was 
                                         issued by the slave.  If there is a previous AXI 
                                         response pending acceptance from the master, this 
                                         delay will be invisible/consumed by the time its 
                                         turn comes for the dispatch. In the case of width 
                                         conversion between PIF and AXI, because of the extra 
                                         translation of responses required, the AXI read 
                                         response is registered at the AXI interface, adding 
                                         additional cycles of latency to read responses. 
                                         In that case, this parameter needs to be set 
                                         accordingly.
                                         Default = 0.

   "read_response_fifo_depth"     u32    The read response fifo depth.  The depth will 
                                         determine the AXI read responses the transactor can 
                                         buffer if upstream AXI master is not accepting futher 
                                         responses. Values greater than zero are valid. A 
                                         value of 1 means only one response can be present in
                                         the fifo at a time and no further responses will be
                                         accepted until that response is sent out to AXI.
                                         Minimum valid value is 1.
                                         Default = 1.
 
   "secure_address_range" vector<u64>    A std::vector containing an even number of
                                         addresses.  Each pair of addresses specifies a range of
                                         addresses that will constitute the secure addresses within 
                                         downstream local memory connected to master pif interface. 
                                         The first address in each pair is the start
                                         address and the second address in each pair is the end 
                                         address of that range.
                                         Default = <empty>                  

   "write_request_delay"          u32    The delay inserted by the transactor in issuing 
                                         input write request to the pif slave. So, effectively 
                                         the pif slave sees this request atleast after these 
                                         delay cycles. If there is a previous PIF request 
                                         pending acceptance from the slave port, this delay 
                                         will be invisible/ consumed by the time its turn 
                                         comes for the dispatch.
                                         Default = 1.

   "write_request_fifo_depth"     u32    The write request fifo depth. The depth will 
                                         determine the write requests the transactor can 
                                         buffer if downstream pif slave is not accepting 
                                         futher write requests. Valid values are values 
                                         greater than zero. A value of 1 means ony one 
                                         request can be present in the fifo at a time and no
                                         further requests will be accepted until that request
                                         is sent out to AXI. Minimum valid value is 1.
                                         Default = 1.

   "write_response_delay"         u32    The delay inserted by the transactor in the write 
                                         response path. So, effectively the axi master sees 
                                         the response after the delay of 
                                         "write_response_delay" cycles from the time it was 
                                         issued by the slave.  If there is a previous AXI 
                                         response pending acceptance from the master, this 
                                         delay will be invisible/consumed by the time its 
                                         turn comes for the dispatch. In the case of width 
                                         conversion between PIF and AXI, because of the extra 
                                         translation of responses required, the AXI write 
                                         response is registered at the AXI interface, adding 
                                         additional cycles of latency to write responses. 
                                         In that case, this parameter needs to be set 
                                         accordingly.
                                         Default = 0.

   "write_response_fifo_depth"    u32    The write response fifo depth. The depth will 
                                         determine the AXI write responses the transactor can 
                                         buffer if upstream AXI master is not accepting futher 
                                         responses. Values greater than zero are valid. A 
                                         value of 1 means only one response can be present in
                                         the fifo at a time and no further responses will be
                                         accepted until that response is sent out to AXI. 
                                         Minimum valid value is 1.
                                         Default = 1.


    \endverbatim
 *
 * @see xtsc_axi2pif_transactor
 * @see xtsc::xtsc_parms
 */
class XTSC_COMP_API xtsc_axi2pif_transactor_parms : public xtsc::xtsc_parms {
public:

  /**
   * Constructor for an xtsc_axi2pif_transactor_parms object.
   *
   * @param axi_byte_width      AXI data bus width in bytes.
   *
   * @param num_ports           The number of ports this transactor has.
   *
   * @see xtsc::xtsc_core::How_to_do_memory_port_binding for a list of legal
   *      memory_interface values.
   */
  xtsc_axi2pif_transactor_parms(xtsc::u32 axi_byte_width = 4, 
                                xtsc::u32 num_ports = 1) {
    init(axi_byte_width, num_ports);
  }


  /**
   * Do initialization common to both constructors.
   */
  void init(xtsc::u32 axi_byte_width = 4, xtsc::u32 num_ports = 1) {
    std::vector<xtsc::u64> secure_address_range;
    add("axi_byte_width",               axi_byte_width); 
    add("clock_period",                 0xFFFFFFFF);
    add("critical_word_first",          false);
    add("outstanding_pif_request_ids",  8);
    add("nacc_wait_time",               0xFFFFFFFF);
    add("num_ports",                    num_ports);
    add("read_request_delay",           1);
    add("read_request_fifo_depth",      1);
    add("read_response_delay",          0);
    add("read_response_fifo_depth",     1);
    add("secure_address_range",         secure_address_range);
    add("write_request_delay",          1);
    add("write_request_fifo_depth",     1);
    add("write_response_delay",         0);
    add("write_response_fifo_depth",    1);
  }


  /// Return what kind of xtsc_parms this is (our C++ type)
  virtual const char* kind() const { return "xtsc_axi2pif_transactor_parms"; }

};

/**
 * Module implementing an Xtensa AXI to Xtensa PIF transactor.
 *
 * This module can be used to connect an Xtensa AXI memory interface master having 
 * separate read and write Xtensa ports(for example, xtsc_component::xtsc_router
 * master ports) to an Xtensa PIF memory interface slave (for example, 
 * xtsc::xtsc_core inbound port (Xtensa LX), xtsc_component::xtsc_arbiter slave ports).
 *
 * For protocol and timing information specific to xtsc::xtsc_core and the Xtensa ISS,
 * see xtsc::xtsc_core::Information_on_memory_interface_protocols.
 *
 * Here is a block diagram of xtsc_axi2pif_transactor objects being used in the
 * xtsc_axi2pif_transactor example:
 * @image html  Example_xtsc_axi2pif_transactor.jpg
 * @image latex Example_xtsc_axi2pif_transactor.eps "xtsc_axi2pif_transactor Example" width=10cm
 *
 * @see xtsc_axi2pif_transactor_parms
 * @see xtsc::xtsc_request
 * @see xtsc::xtsc_response
 * @see xtsc::xtsc_request_if
 * @see xtsc::xtsc_respond_if
 * @see xtsc::xtsc_core
 * @see xtsc::xtsc_core::Information_on_memory_interface_protocols.
 */
class XTSC_COMP_API xtsc_axi2pif_transactor : public sc_core::sc_module,
                                              public xtsc::xtsc_module,
                                              public xtsc::xtsc_mode_switch_if,
                                              public xtsc::xtsc_command_handler_interface
{
public:

  /**
   * Constructor for an xtsc_axi2pif_transactor.
   * @param     module_name     Name of the xtsc_axi2pif_transactor sc_module.
   * @param     parms           The remaining parameters for construction.
   * @see xtsc_axi2pif_transactor_parms
   */
  xtsc_axi2pif_transactor(sc_core::sc_module_name module_name, const xtsc_axi2pif_transactor_parms& parms);

  // Destructor.
  ~xtsc_axi2pif_transactor(void);

  /// For sc_core::sc_moduleinterface. Simulation callback.
  virtual void end_of_simulation(void);

  /*********** Class functions *************/

public:
  // SystemC functions
  /// For SystemC
  SC_HAS_PROCESS(xtsc_axi2pif_transactor);

  /// Our C++ type (SystemC uses this)
  virtual const char* kind() const { return "xtsc_axi2pif_transactor"; }

  // xtsc get functions
  /// For xtsc_connection_interface
  virtual xtsc::u32 get_bit_width(const std::string& port_name, xtsc::u32 interface_num = 0) const;


  /// For xtsc_connection_interface
  virtual sc_core::sc_object *get_port(const std::string& port_name);

  /// For xtsc_connection_interface
  virtual xtsc::xtsc_port_table get_port_table(const std::string& port_table_name) const;

  /// Return axi byte width of data interface (from "axi_byte_width" parameter)
  xtsc::u32 get_axi_byte_width() const { return m_axi_width8; }

  /// Return the number of memory ports this transactor has (from "num_ports" parameter)
  xtsc::u32 get_num_ports() const { return m_num_ports; }

  //xtsc functions

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

  /**
   * Implementation of the xtsc::xtsc_command_handler_interface.
   *
   * This implementation supports the following commands:
   *  \verbatim
        change_clock_period <ClockPeriodFactor>
          Call xtsc_axi2pif_transactor::change_clock_period(<ClockPeriodFactor>).
          Return previous <ClockPeriodFactor> for this device.

        peek <PortNumber> <StartAddress> <NumBytes>
          Peek <NumBytes> of memory starting at <StartAddress>.

        poke <PortNumber> <StartAddress> <NumBytes> <Byte1> <Byte2> . . . <ByteN>
          Poke <NumBytes> (=N) of memory starting at <StartAddress>.

        reset [<Hard>]
          Call xtsc_axi2pif_transactor::reset().
      \endverbatim
   */
  virtual void execute(const std::string&       cmd_line,
               const std::vector<std::string>&  words,
               const std::vector<std::string>&  words_lc,
               std::ostream&                    result);


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


  /************ subclasses ************/
private:

  // Class to keep a track of a PIF request and its scheduled time after accounting request_delay
  class axi_req_info {
  public:
    //Constructor
    axi_req_info() {
      reset();
    }
 
    void reset() {
      m_axi_request         = nullptr;
      m_sched_time          = sc_core::sc_time(0, sc_core::SC_NS);
 
      m_pif_conv_pending    = true;
    }
 
    xtsc::xtsc_request*                m_axi_request;                          ///< AXI request from upstream
    sc_core::sc_time                   m_sched_time;                           ///< Sched time
 
    bool                               m_pif_conv_pending;                     ///< Indicate if AXI request if converted into PIF request(s)
  };
 
  // Class to keep track of all the PIF request(s)/response(s) for a particular AXI transaction
  class axi_trans_info {
  public:
    // Constructor
    axi_trans_info() {
      reset();
    }

    ~axi_trans_info() {
    }
 
    void reset() {
      m_axi_request                         = nullptr;
      m_curr_axi_transfer_num               = 0;
      m_is_read                             = true;
 
      m_total_pif_requests                  = 0;
      m_num_pif_trans                       = 0;
 
      m_all_pif_req_sent                    = false;
      m_num_pif_rsps_arrvd                  = 0;
      m_num_rsps_remaining_4_curr_pif_trans = 0;
      m_num_axi_resp_sent                   = 0;
      m_curr_pif_tag                        = 0;
      m_is_block_request                    = false;
      m_pif_reqs_sent                       = 0;
      m_curr_response_status                = xtsc::xtsc_response::AXI_OK;

      m_is_nsm_error                        = false;
 
      while(!m_num_pif_reqs_vec.empty()) {
        m_num_pif_reqs_vec.pop();
      }
      while(!m_pif_requests_tag_vec.empty()) {
        m_pif_requests_tag_vec.pop();
      }
      while(!m_pif_requests_vec.empty()) {
        m_pif_requests_vec.pop();
      }
    }
 
    xtsc::xtsc_request*                m_axi_request;                          ///< Corresponding AXI request (first request in case of write request)
    bool                               m_is_read;                              ///< Indicate if its a read request
 
    xtsc::u32                          m_total_pif_requests;                   ///< Total number of PIF requests
    xtsc::u32                          m_num_pif_trans;                        ///< Number of PIF transactions
 
    std::queue<xtsc::u64>              m_num_pif_reqs_vec;                     ///< Vector of number of PIF requests in each PIF transaction
    std::queue<xtsc::u64>              m_pif_requests_tag_vec;                 ///< Vector of PIF SINGLE/BURST/BLOCK requests tags. Each PIF transaction has its own tag.
 
    // Signals only for request phase
    xtsc::u32                          m_pif_reqs_sent;                        ///< Num of total PIF requests sent
    bool                               m_is_block_request;                     ///< Indicate if pif requests are block requests (only for writes)
    xtsc::u32                          m_curr_axi_transfer_num;                ///< Current expected AXI request transfer number (used only for write request)
    std::queue<xtsc::xtsc_request*>    m_pif_requests_vec;                     ///< vector of PIF SINGLE/BURST/BLOCK requests
 
    // Signals only for response phase
    bool                               m_all_pif_req_sent;                     ///< Indicate if all pif requests have been sent (only for writes)
    xtsc::u32                          m_num_pif_rsps_arrvd;                   ///< Number of PIF responses arrived
    xtsc::u32                          m_num_rsps_remaining_4_curr_pif_trans;  ///< Number of PIF responses remaining from downstream for current PIF transaction
    xtsc::u32                          m_num_axi_resp_sent;                    ///< Number of AXI responses sent
    xtsc::u64                          m_curr_pif_tag;                         ///< Current PIF tag for upcoming response

    xtsc::xtsc_response::status_t      m_curr_response_status;                 ///< AXI response to be sent back after combining PIF responses

    bool                               m_is_nsm_error;                         ///< True if the access is to a secure region
  };

  // Class to keep a track of a AXI response and its scheduled time after accounting response_delay
  class axi_rsp_trans_info {
  public:
    // Constructor
    axi_rsp_trans_info() {
      reset();
    }

    ~axi_rsp_trans_info() {
    }
 
    void reset() {
      m_axi_response      = nullptr;
      m_sched_time        = sc_core::sc_time(0, sc_core::SC_NS);
 
      m_axi_request_tag   = 0;
    }
 
    xtsc::xtsc_response*               m_axi_response;                     ///< AXI response from downstream
    sc_core::sc_time                   m_sched_time;                       ///< Sched time
 
    xtsc::u64                          m_axi_request_tag;                  ///< Corresponding AXI request tag
  };

  /// Implementation of xtsc_request_if.
  class xtsc_request_if_impl : public xtsc::xtsc_request_if, public sc_core::sc_object {
  public:

    /// Constructor
    xtsc_request_if_impl(const char *object_name, xtsc_axi2pif_transactor& transactor, xtsc::u32 port_num,
                         std::string type) :
      sc_object         (object_name),
      m_transactor      (transactor),
      m_port_num        (port_num),
      m_p_port          (0),
      m_type            (type)
    {}

    /// The kind of sc_object we are
    const char* kind() const { return "xtsc_axi2pif_transactor::xtsc_request_if_impl"; }


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

    xtsc_axi2pif_transactor    &m_transactor;           ///< Our xtsc_axi2pif_transactor object
    xtsc::u32                   m_port_num;             ///< Port number for current request interface
    sc_core::sc_port_base      *m_p_port;               ///< Port that is bound to us
    std::string                 m_type;                 ///< Type of response : READ/WRITE
  };


  /// Implementation of xtsc_respond_if.
  class xtsc_respond_if_impl : public xtsc::xtsc_respond_if, public sc_core::sc_object {
  public:

    /// Constructor
    xtsc_respond_if_impl(const char *object_name, xtsc_axi2pif_transactor& transactor, xtsc::u32 port_num) :
      sc_object         (object_name),
      m_transactor      (transactor),
      m_port_num        (port_num),
      m_p_port          (0)
    {}

    /// The kind of sc_object we are
    const char* kind() const { return "xtsc_axi2pif_transactor::xtsc_respond_if_impl"; }

    /// From downstream slave
    /// @see xtsc::xtsc_respond_if
    bool nb_respond(const xtsc::xtsc_response& response);

    /// Worker function for read response interface
    void read_response_worker(const xtsc::xtsc_response *p_pif_response);

    /// Worker function for write response interface
    void write_response_worker(const xtsc::xtsc_response *p_pif_response);

    /// Return true if a port has been already bound to this implementation
    bool is_connected() { return (m_p_port != 0); }

  protected:

    /// SystemC callback when something binds to us
    virtual void register_port(sc_core::sc_port_base& port, const char *if_typename);

    xtsc_axi2pif_transactor  &m_transactor;              ///< Our xtsc_axi2pif_transactor object
    xtsc::u32                 m_port_num;                ///< Port number for current response interface
    sc_core::sc_port_base    *m_p_port;                  ///< Port that is bound to us
  };

 /************ Functions ******************/

private:

  // threads
  // Request thread
  void request_thread(void);

  // Response threads
  void read_response_thread(void);
  void write_response_thread(void);

  // xtsc functions to send pif requests
  void handle_axi_request(axi_req_info *p_axi_req_info, xtsc::u32 port_num);

  void handle_pif_request(xtsc::xtsc_request *p_pif_request, xtsc::u32 port_num);

  void send_pif_req_worker(xtsc::xtsc_request *p_axi_request, xtsc::u32 port_num);

  axi_trans_info * conv_rw_axi2pif_request(xtsc::xtsc_request *p_axi_request, xtsc::u32 port_num);

  axi_trans_info * conv_read_axi2pif_request(xtsc::xtsc_request *p_axi_request, xtsc::u32 port_num);

  axi_trans_info * conv_write_axi2pif_request(xtsc::xtsc_request *p_axi_request, xtsc::u32 port_num);

  axi_req_info * get_next_axi_request(xtsc::u32 port_num);

  void set_next_req_id(xtsc::xtsc_request *p_axi_request, xtsc::u32 port_num);

  // xtsc functions for responses
  void send_read_response(xtsc::xtsc_response& p_axi_response, xtsc::u32);
  void send_write_response(xtsc::xtsc_response& p_axi_response, xtsc::u32);

  /// Get a new copy of an xtsc_request (from the pool)
  xtsc::xtsc_request *copy_request(const xtsc::xtsc_request& request);

  /// Get a new xtsc_request (from the pool)
  xtsc::xtsc_request *new_request();

  /// Delete an xtsc_request (return it to the pool)
  void delete_request(xtsc::xtsc_request*& p_request);

  //xtsc utils functions
  /// Return true if this is a READ|BLOCK_READ|BURST_READ|RCW1
  bool is_read_access(const xtsc::xtsc_request* p_request) {
    return (p_request->is_read()  &&  !(p_request->get_type() == xtsc::xtsc_request::RCW &&
                                        p_request->get_transfer_number() == 2));
  }

  void clear_fifos(void);

  /// Check if a request access secure region
  bool check_non_secure_access(xtsc::xtsc_request* p_request);
  void check_rd_nsm_rsp_id(xtsc::u32 port_num);
  void check_wr_nsm_rsp_id(xtsc::u32 port_num);
  void send_rd_nsm_rsps(xtsc::u32 port_num);
  void send_wr_nsm_rsps(xtsc::u32 port_num);

  void delete_req_info(axi_req_info *i_axi_req_info);
  void delete_axi_rsp(axi_rsp_trans_info *i_axi_rsp_trans_info, xtsc::u32 port_num);
  void delete_trans_info(axi_trans_info *i_axi_trans_info, xtsc::u32 port_num);

  /************ Class members *************/

public:
  // Input data ports
  // Read port/export
  sc_core::sc_export  <xtsc::xtsc_request_if>   **m_read_request_exports;   ///< From upstream master(s) to us
  sc_core::sc_port    <xtsc::xtsc_respond_if>   **m_read_respond_ports;     ///< From us to upstream master(s)
  // Write port/export
  sc_core::sc_export  <xtsc::xtsc_request_if>   **m_write_request_exports;  ///< From upstream master(s) to us
  sc_core::sc_port    <xtsc::xtsc_respond_if>   **m_write_respond_ports;    ///< From us to upstream master(s)

  // Output data ports
  sc_core::sc_port    <xtsc::xtsc_request_if>   **m_request_ports;          ///< From us to downstream slave(s)
  sc_core::sc_export  <xtsc::xtsc_respond_if>   **m_respond_exports;        ///< From downstream slave(s) to us

private:
  // Main params
  xtsc::u32                                  m_axi_width8;                          ///<  See the "axi_byte_width"
  xtsc::u32                                  m_num_ports;                           ///<  See the "num_ports" parameter
  xtsc::u32                                  m_pif_width8;                          ///<  Equal to "m_axi_width8"

  // Transaction delays
  xtsc::u32                                  m_read_request_delay;                  ///<  See the "read_request_delay" parameter.
  xtsc::u32                                  m_write_request_delay;                 ///<  See the "write_request_delay" parameter.
  xtsc::u32                                  m_read_response_delay;                 ///<  See the "read_response_delay" parameter.
  xtsc::u32                                  m_write_response_delay;                ///<  See the "write_response_delay" parameter.
  
  // Clock related timings
  sc_core::sc_time                           m_clock_period;                        ///<  This transactor's clock period
  xtsc::u64                                  m_clock_period_value;                  ///<  Clock period as u64
  sc_core::sc_time                           m_time_resolution;                     ///<  SystemC time resolution

  // XTSC request/response interfaces
  xtsc_request_if_impl                     **m_xtsc_read_request_if_impl;           ///<  m_read_respond_exports bind to these
  xtsc_request_if_impl                     **m_xtsc_write_request_if_impl;          ///<  m_write_respond_exports bind to these
  xtsc_respond_if_impl                     **m_xtsc_respond_if_impl;                ///<  m_request_exports bind to these

  //Events
  sc_core::sc_event                        **m_request_thread_event;                ///<  To notify read_request_thread when a request is accepted
  sc_core::sc_event                        **m_read_response_thread_event;          ///<  To notify read_response_thread when a response is accepted from axi port
  sc_core::sc_event                        **m_write_response_thread_event;         ///<  To notify write_response_thread when a response is accepted from axi port

  // Queues/fifos/vectors
  std::queue<axi_req_info*>                **m_read_request_fifo;                   ///<  The fifo for incoming read requests 
  std::queue<axi_req_info*>                **m_write_request_fifo;                  ///<  The fifo for incoming write requests 
  std::queue<axi_rsp_trans_info*>          **m_read_response_fifo;                  ///<  The fifo for incoming read responses from the axi slave 
  std::queue<axi_rsp_trans_info*>          **m_write_response_fifo;                 ///<  The fifo for incoming write responses from the axi slave 
  std::queue<xtsc::u64>                    **m_axi_request_order_fifo;              ///<  PIF request ordered queue
  std::queue<xtsc::u64>                    **m_axi_rd_response_order_fifo;          ///<  PIF read response ordered queue
  std::queue<xtsc::u64>                    **m_axi_wr_response_order_fifo;          ///<  PIF write response ordered queue
  
  // Fifo depths
  xtsc::u32                                  m_read_request_fifo_depth;             ///<  See the "read_request_fifo_depth" parameter.
  xtsc::u32                                  m_write_request_fifo_depth;            ///<  See the "write_request_fifo_depth" parameter.
  xtsc::u32                                  m_read_response_fifo_depth;            ///<  See the "response_fifo_depth" parameter.
  xtsc::u32                                  m_write_response_fifo_depth;           ///<  See the "response_fifo_depth" parameter.

  // nacc related params
  sc_core::sc_time                           m_nacc_wait_time;                      ///<  See "nacc_wait_time" parameter
  bool                                      *m_pif_waiting_for_nacc;                ///<  State where write port is waiting on the potential NACC
  bool                                      *m_pif_request_got_nacc;                ///<  Indicate that read port has received NACC

  bool                                       m_critical_word_first;                 ///<  Enable support for critical word first

  xtsc::u32                                  m_outstanding_pif_request_ids;         ///< See the "outstanding_pif_request_ids" parameter
  bool                                     **m_read_request_id_pending;             ///<  Indicate if which request ids are pending for reads
  bool                                     **m_write_request_id_pending;            ///<  Indicate if which request ids are pending for writes
  xtsc::u32                                 *m_num_read_pif_requests_pending;       ///<  Count for PIF read requests sent downstream whose response has not arrived yet
  xtsc::u32                                 *m_num_write_pif_requests_pending;      ///<  Count for PIF write requests sent downstream whose response has not arrived yet
  xtsc::u8                                  *m_last_read_req_id_selected;           ///<  Last Request Id used for read request
  xtsc::u8                                  *m_last_write_req_id_selected;          ///<  Last Request Id used for write request

  bool                                      *m_write_request_active;                ///<  Indicate if write requests are being sent for a particular transaction to avoid sending read in between
  xtsc::u64                                 *m_write_request_active_tag;            ///<  Corresponding AXI write request tag
  
  xtsc::u32                                  m_next_request_port_num;               ///<  Used by request_thread to get its port number
  xtsc::u32                                  m_next_read_response_port_num;         ///<  Used by read_respond_thread to get its port number
  xtsc::u32                                  m_next_write_response_port_num;        ///<  Used by write_respond_thread to get its port number

  std::vector<xtsc::u64>                     m_secure_address_range;                ///<  For Non secure access.  See "secure_address_range".
  xtsc::u32                                  m_max_block_pif_byte_size;             ///<  Maximum byte_size allowed for a pif block request. Set to 32
  bool                                       m_non_secure_mode;                     ///<  True if secure address range has a valid entry

  bool                                      *m_rd_nsm_vld;                          ///<  Valid if any nsm ready to be scheduled
  bool                                      *m_wr_nsm_vld;                          ///<  Valid if any nsm ready to be scheduled

  // Mappings
  std::map<xtsc::u64, std::queue<axi_trans_info*> >  m_tag_2_axi_trans_info_map;    ///<  Map AXI/PIF request(s) tag to axi_trans_info
  std::map<std::string, xtsc::u32>           m_bit_width_map;                       ///<  For xtsc_connection_interface

  xtsc::u32                                  m_request_count;                       ///<  Count each newly created xtsc_request
  std::vector<xtsc::xtsc_request*>           m_request_pool;                        ///<  Maintain a pool of xtsc_request to improve performance

  std::vector<sc_core::sc_process_handle>    m_process_handles;                     ///<  For reset 

  xtsc::xtsc_sim_mode                        m_sim_mode;                            ///<  Current Simulation mode of the transactor
  bool                                       m_mode_switch_pending;                 ///<  Indicate simulation mode switch is in progress
  sc_core::sc_event                          m_ready_to_switch_event;               ///<  Signal ready to switch simulation mode

  // Logging
  log4xtensa::TextLogger&                    m_text;                                ///<  Text logger
};

}  // namespace xtsc_component

#endif  // _XTSC_AXI2PIF_TRANSACTOR_H_
