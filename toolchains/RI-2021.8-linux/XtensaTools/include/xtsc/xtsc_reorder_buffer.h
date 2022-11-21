#ifndef _XTSC_REORDER_BUFFER_H_
#define _XTSC_REORDER_BUFFER_H_

// Copyright (c) 2005-2020 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Cadence Design Systems, Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Cadence Design Systems, Inc.

/**
 * @file 
 */

#include <xtsc/xtsc.h>
#include <xtsc/xtsc_parms.h>
#include <xtsc/xtsc_request_if.h>
#include <xtsc/xtsc_respond_if.h>
#include <xtsc/xtsc_request.h>
#include <xtsc/xtsc_response.h>
#include <vector>
#include <map>

namespace xtsc {



/**
 * Constructor parameters for a xtsc_reorder_buffer object.
 *
 *  \verbatim
   Name                  Type   Description
   ------------------    ----   -------------------------------------------------------

   "axi_byte_width"      u32    AXI bus width in bytes.  Valid values are 16 and 32.
                                Default = 16.

   "clock_period"        u32    This is the length of this buffer's clock period 
                                expressed in terms of the SystemC time resolution 
                                (from sc_get_time_resolution()). A value of 0xFFFFFFFF 
                                means to use the XTSC system clock period (from 
                                xtsc_get_system_clock_period()). A value of 0 means 
                                one delta cycle.
                                Default = 0xFFFFFFFF (use the system clock period).  

   "nacc_wait_time"      u32    This parameter, expressed in terms of the SystemC 
                                time resolution, specifies how long to wait after
                                sending a request downstream to see if it was 
                                rejected by RSP_NACC.  This value must not exceed
                                this model's clock period.  A value of 0 means
                                one delta cycle. A value of 0xFFFFFFFF means to wait 
                                for a period equal to this model's clock period. 
                                CAUTION:  A value of 0 can cause an infinite loop 
                                in the simulation if the downstream module requires 
                                a non-zero time to become available.
                                Default = 0xFFFFFFFF (model's clock period).

   "num_entries"         u32    Number of supported outstanding requests/buffer entries. 
                                Valid values are 32, 48, 64, 128.
                                Default = 48.

   "use_ECC_timing"      bool   If set, the model mimics the timing of the corresponding
                                hardware model configured with ECC mode.
                                Default = false.

    \endverbatim
 *
 * @see xtsc_reorder_buffer
 * @see xtsc::xtsc_parms
 */
class XTSC_API xtsc_reorder_buffer_parms : public xtsc::xtsc_parms {
public:

 /**
   * Constructor for an xtsc_reorder_buffer_parms object. In addition, the
   * "clock_period" parameter will be set to match the core's clock period.
   *
   * @param axi_byte_width      AXI data bus width in bytes.
   *
   * @param num_entries         number of buffer entries.
   *
   * @see xtsc::xtsc_core::How_to_do_memory_port_binding for a list of legal
     memory_interface values.
    */
  xtsc_reorder_buffer_parms(xtsc::u32 axi_byte_width = 16,
                            xtsc::u32 num_entries    = 48)
  {
    add("axi_byte_width",       axi_byte_width);
    add("clock_period",         0xFFFFFFFF);    
    add("nacc_wait_time",       0xFFFFFFFF);
    add("num_entries",          num_entries);
    add("use_ECC_timing",       false);
  }


  /// Return what kind of xtsc_parms this is (our C++ type)
  virtual const char* kind() const override { return "xtsc_reorder_buffer_parms"; }

};



/**
 * Module implementing the functionality of a Reorder Buffer.
 *
 * The Reorder Buffer allows an Xtensa core and MP subsystems to be used in systems 
 * that can respond to read and write requests out of order. The Reorder Buffers issue 
 * sequential AXI IDs for the requests they receive from the iDMA AXI bus master so that
 * they can track the response ID of the in-order response.  They buffer responses that 
 * are received out of order so that they can be returned to the iDMA AXI master when 
 * their ID becomes in-order.  
 *
 * Here is a block diagram of the system used in the xtsc_reorder_buffer example:
 * @image html  Example_xtsc_reorder_buffer.jpg
 * @image latex Example_xtsc_reorder_buffer.eps "xtsc_reorder_buffer Example" width=10cm
 *
 * @see xtsc::xtsc_request_if
 * @see xtsc::xtsc_respond_if
 * @see xtsc_reorder_buffer_parms
 * @see xtsc::xtsc_core::How_to_do_port_binding
 */
class XTSC_API xtsc_reorder_buffer : public sc_core::sc_module, 
                                     public xtsc::xtsc_module, 
                                     public xtsc::xtsc_command_handler_interface {
public:

  // Port-Export Pairs for Rd & Wr on both sides
  sc_core::sc_port  <xtsc::xtsc_request_if>     m_rd_request_port;         ///< ROB->Slave
  sc_core::sc_export<xtsc::xtsc_request_if>     m_rd_request_export;       ///< Master->ROB
  sc_core::sc_port  <xtsc::xtsc_respond_if>     m_rd_respond_port;         ///< Slave->ROB
  sc_core::sc_export<xtsc::xtsc_respond_if>     m_rd_respond_export;       ///< ROB->Slave
  sc_core::sc_port  <xtsc::xtsc_request_if>     m_wr_request_port;         ///< ROB->Slave  
  sc_core::sc_export<xtsc::xtsc_request_if>     m_wr_request_export;       ///< Master->ROB 
  sc_core::sc_port  <xtsc::xtsc_respond_if>     m_wr_respond_port;         ///< Slave->ROB  
  sc_core::sc_export<xtsc::xtsc_respond_if>     m_wr_respond_export;       ///< ROB->Slave  


  // For SystemC
  SC_HAS_PROCESS(xtsc_reorder_buffer);

  
  /// Our C++ type (SystemC uses this)
  virtual const char* kind() const override { return "xtsc_reorder_buffer"; }


  /**
   * Constructor for an xtsc_reorder_buffer.
   * @param     module_name             Name of the xtsc_reorder_buffer sc_module.
   * @param     reorder_buffer_parms    The remaining parameters for construction.
   * @see xtsc_reorder_buffer_parms
   */
  xtsc_reorder_buffer(sc_core::sc_module_name module_name, 
                      const xtsc_reorder_buffer_parms& reorder_buffer_parms);


  // Destructor.
  virtual ~xtsc_reorder_buffer(void);


  /// Print info on pools for debugging
  virtual void end_of_simulation(void) override;


  /// For xtsc_connection_interface
  virtual xtsc::u32 get_bit_width(const std::string& port_name, xtsc::u32 interface_num = 0) const override;


  /// For xtsc_connection_interface
  virtual sc_core::sc_object *get_port(const std::string& port_name) override;


  /// For xtsc_connection_interface
  virtual xtsc::xtsc_port_table get_port_table(const std::string& port_table_name) const override;


  /// Reset the xtsc_reorder_buffer.
  virtual void reset(bool hard_reset = false) override;


 /**
   * Implementation of the xtsc::xtsc_command_handler_interface.
   *
   * This implementation supports the following commands:
   *  \verbatim
        change_clock_period <ClockPeriodFactor>
          Call xtsc_reorder_buffer::change_clock_period(<ClockPeriodFactor>).
          Return previous <ClockPeriodFactor> for this device.

        peek <PortNumber> <StartAddress> <NumBytes>
          Peek <NumBytes> of memory starting at <StartAddress>.
          <PortNumber>=0 is for read port and other value for write port.

        poke <PortNumber> <StartAddress> <NumBytes> <Byte1> <Byte2> . . . <ByteN>
          Poke <NumBytes> (=N) of memory starting at <StartAddress>.
          <PortNumber>=0 is for read port and other value for write port.

        reset [<Hard>]
          Call xtsc_reorder_buffer::reset().
      \endverbatim
   */
  virtual void execute(const std::string&               cmd_line,
                       const std::vector<std::string>&  words,
                       const std::vector<std::string>&  words_lc,
                       std::ostream&                    result) override;



   /// Get the TextLogger for this component (e.g. to adjust its log level)
  log4xtensa::TextLogger& get_text_logger() { return m_text; }


private:

  /// SystemC thread to handle read requests
  void read_request_thread(void);


  /// SystemC thread to handle write requests
  void write_request_thread(void);


  /// SystemC thread to handle read responses
  void read_response_thread(void);


  /// SystemC thread to handle write responses
  void write_response_thread(void);


  /// Common method for sending a response
  void send_response(xtsc::xtsc_response& response, bool);


  /// Increment counter value and wraparounds to 0 as it becomes equal to m_num_entries
  void increment_counter(xtsc::u32 &counter);


  /// Get a new copy of an xtsc_request (from the pool)
  xtsc::xtsc_request *copy_request(const xtsc::xtsc_request& request);


  /// Delete an xtsc_request (return it to the pool)
  void delete_request(xtsc::xtsc_request*& p_request);


   /**
   * Method to change the clock period.
   *
   * @param     clock_period_factor     Specifies the new length of this device's clock
   *                                    period expressed in terms of the SystemC time
   *                                    resolution (from sc_get_time_resolution()).
   */
  void change_clock_period(xtsc::u32 clock_period_factor);



  /// Implementation of xtsc_request_if.
  class xtsc_request_if_impl : public xtsc::xtsc_request_if, public sc_core::sc_object {
  public:

    /// Constructor
    xtsc_request_if_impl(const char *object_name, xtsc_reorder_buffer& rob, std::string type) :
      sc_object         (object_name),
      m_rob             (rob),
      m_p_port          (0),
      m_type            (type)
    {}

    /// The kind of sc_object we are
    virtual const char* kind() const override { return "xtsc_reorder_buffer::xtsc_request_if_impl"; }


    /// From upstream master
    /// @see xtsc::xtsc_request_if
    virtual void nb_peek(xtsc::xtsc_address address8, xtsc::u32 size8, xtsc::u8 *buffer) override;


    /// From upstream master
    /// @see xtsc::xtsc_request_if
    virtual void nb_poke(xtsc::xtsc_address address8, xtsc::u32 size8, const xtsc::u8 *buffer) override;


    /// From upstream master. Use "turbo_support" to specify fast access support.
    /// @see xtsc::xtsc_request_if
    virtual bool nb_fast_access(xtsc::xtsc_fast_access_request &request) override;
    

    /// From upstream master
    /// @see xtsc::xtsc_request_if
    virtual void nb_request(const xtsc::xtsc_request& request) override;
    

    /// Return true if a port has been bound to this implementation
    bool is_connected() { return (m_p_port != 0); }


  protected:

    /// SystemC callback when something binds to us
    virtual void register_port(sc_core::sc_port_base& port, const char *if_typename) override;

    xtsc_reorder_buffer        &m_rob;         ///<  Our xtsc_reorder_buffer object
    sc_core::sc_port_base      *m_p_port;      ///<  Port that is bound to us
    std::string                 m_type;        ///<  Type of request port 
  };


  /// Implementation of xtsc_respond_if.
  class xtsc_respond_if_impl : public xtsc::xtsc_respond_if, public sc_core::sc_object {
  public:

    /// Constructor
    xtsc_respond_if_impl(const char *object_name, xtsc_reorder_buffer& rob, std::string type) :
      sc_object         (object_name),
      m_rob             (rob),
      m_p_port          (0),
      m_type            (type)
    {}

    /// The kind of sc_object we are
    virtual const char* kind() const override { return "xtsc_reorder_buffer::xtsc_respond_if_impl"; }

    /// From downstream slave
    /// @see xtsc::xtsc_respond_if
    bool nb_respond(const xtsc::xtsc_response& response) override;

    /// Return true if a port has been already bound to this implementation
    bool is_connected() { return (m_p_port != 0); }


  protected:

    /// SystemC callback when something binds to us
    virtual void register_port(sc_core::sc_port_base& port, const char *if_typename) override;

    xtsc_reorder_buffer      &m_rob;         ///<  Our xtsc_reorder_buffer object
    sc_core::sc_port_base    *m_p_port;      ///<  Port that is bound to us
    std::string               m_type;        ///<  Type of response : READ/WRITE
  };


  // Hold the entries into the buffers
  class buffer_entry {
  public:

    //Constructor
    buffer_entry()
    {
      reset();
    }

    void reset() {
      m_idma_channel_id         = 0;
      m_num_responses_expected  = 0;
      m_responses_received      = 0;
      m_responses.clear();
    }

    xtsc::u8                           m_idma_channel_id;          ///< Effectively the AxID of the incoming request
    xtsc::u32                          m_num_responses_expected;   ///< 1 for *WRITE and num_of_transfers for *READ
    xtsc::u32                          m_responses_received;       ///< Counter to track received responses
    std::vector<xtsc::xtsc_response*>  m_responses;                ///< Vector to hold xtsc_responses meant for iDMA

  };


  /// Get a new buffer_entry (from the pool)
  buffer_entry *new_buffer_entry();


  /// Delete a buffer_entry (return it to the pool)
  void delete_buffer_entry(buffer_entry*& p_buffer_entry);


  // Class constant
  static const  xtsc::u32  m_max_supported_block_size = 8;           ///<  Maximum block size supported.


  // Member Variables
  xtsc::u32                            m_axi_width8;                 ///<  See the "axi_byte_width".
  sc_core::sc_time                     m_nacc_wait_time;             ///<  See "nacc_wait_time" parameter.
  xtsc::u32                            m_num_entries;                ///<  See the "num_entries" parameter.
  bool                                 m_use_ECC_timing;             ///<  See the "use_ECC_timing" parameter.
  std::map< xtsc::u32, 
            buffer_entry* >            m_read_buffer_map;            ///<  Container to hold the read entries.
  std::map< xtsc::u32,
            buffer_entry* >            m_write_buffer_map;           ///<  Container to hold the write entries.
  xtsc::u32                            m_read_request_counter;       ///<  ARID of the next read request sent by
                                                                     ///<  the buffer.  
  xtsc::u32                            m_write_request_counter;      ///<  ARID of the next write request sent by
                                                                     ///<  the buffer. 
  xtsc::u32                            m_read_response_counter;      ///<  The next in-order read response.  
  xtsc::u32                            m_write_response_counter;     ///<  The next in-order write response.
  sc_core::sc_time                     m_clock_period;               ///<  Buffer's clock period
  xtsc::u64                            m_clock_period_value;         ///<  Clock period as u64 
  xtsc::u32                            m_request_delay;              ///<  Cycles delay inserted by buffer in the 
                                                                     ///<  request path  
  xtsc::u32                            m_response_ecc_delay;         ///<  Cycles delay inserted by buffer in the 
                                                                     ///<  request path  
  xtsc::u32                            m_response_nonecc_delay;      ///<  Cycles delay inserted by buffer in the 
                                                                     ///<  request path 
  std::vector<sc_core::
              sc_process_handle>       m_process_handles;            ///<  SystemC process handles for reset 
  sc_core::sc_time                     m_time_resolution;            ///<  SystemC time resolution
  log4xtensa::TextLogger&              m_text;                       ///<  Text logger 
  xtsc_request_if_impl                *m_rd_request_if_impl;         ///<  m_rd_request_port binds to it.
  xtsc_request_if_impl                *m_wr_request_if_impl;         ///<  m_wr_request_port binds to it.
  xtsc_respond_if_impl                *m_rd_respond_if_impl;         ///<  m_rd_respond_port binds to it.
  xtsc_respond_if_impl                *m_wr_respond_if_impl;         ///<  m_wr_respond_port binds to it.
  std::map<xtsc::u64,
           xtsc::xtsc_request*>        m_request_map;                ///<  Map to keep a track of incoming requests 
  std::vector<xtsc::xtsc_request*>     m_request_pool;               ///<  Maintain a pool of xtsc_request 
  xtsc::u32                            m_request_count;              ///<  Count each newly created xtsc_request
  std::vector<buffer_entry*>           m_buffer_entry_pool;          ///<  Maintain a pool of read_buffer_entry 
  xtsc::u32                            m_buffer_entry_count;         ///<  Count each newly created read_buffer_entry  
  bool                                *m_read_responses_received;    ///<  An array for equests with respones received
  bool                                *m_write_responses_received;   ///<  An array for equests with respones received
  bool                                 m_read_waiting_for_nacc;      ///<  Used by read port to wait on the potential NACC
  bool                                 m_write_waiting_for_nacc;     ///<  Used by write port to wait on the potential NACC
  bool                                 m_read_request_got_nacc;      ///<  Indicate that read port has received NACC
  bool                                 m_write_request_got_nacc;     ///<  Indicate that write port has received NACC
  sc_core::sc_fifo<xtsc::
                   xtsc_request*>      m_read_request_fifo;          ///<  Fifo to hold read in xtsc request
  sc_core::sc_fifo<xtsc::
                   xtsc_request*>      m_write_request_fifo;         ///<  Fifo to hold read in xtsc request
  sc_core::sc_event                   *m_read_request_thread_event;  ///<  Event to trigger read_request_thread
  sc_core::sc_event                   *m_write_request_thread_event; ///<  Event to trigger write_request_thread
  sc_core::sc_event                   *m_read_response_thread_event; ///<  Event to trigger read_response_thread
  sc_core::sc_event                   *m_write_response_thread_event;///<  Event to trigger write_response_thread

};

} //namespace xtsc

#endif //_XTSC_REORDER_BUFFER_H_
