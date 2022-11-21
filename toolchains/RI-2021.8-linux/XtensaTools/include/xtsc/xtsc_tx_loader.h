#ifndef _XTSC_TX_LOADER_H_
#define _XTSC_TX_LOADER_H_

// Copyright (c) 2005-2012 by Tensilica Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Tensilica Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Tensilica Inc.

/**
 * @file 
 */


#include <xtsc/xtsc.h>
#include <xtsc/xtsc_parms.h>
#include <xtsc/xtsc_tx_xfer_if.h>
#include <xtsc/xtsc_tx_xfer.h>
#include <xtsc/xtsc_queue_push_if.h>
#include <xtsc/xtsc_queue_pop_if.h>
#include <vector>
#include <cstring>
#include <fstream>




namespace xtsc {


/**
 * Constructor parameters for a xtsc_tx_loader object.
 *
 *  \verbatim
   Name                 Type    Description
   ------------------   ----    --------------------------------------------------------

   "image_file"         char*   Name of the optional image file to load.  This is the
                                file produced by the xt-load program.

   "hex_format"         bool    By default, the "image_file" is assumed to be in 
                                readmemh format.  Set this parameter to true if it was
                                generated with the xt-load --hex option.
                                Default = false (readmemh format).

   "binary_format"      bool    By default, the "image_file" is assumed to be in 
                                readmemh format.  Set this parameter to true if it was
                                generated with the xt-load --binary option.
                                Default = false (readmemh format).

   "turbo"              bool    If true, then the optional image file will be processed
                                in turbo mode (in 0 simulation time).
                                Default = As specified by the "turbo" parameter of
                                xtsc_initialize_parms.
  
   "squelch_loading"    bool    This parameter controls the logging of data while in the
                                Loading state.  If true, then XTSC_VERBOSE() is used.
                                If false, then XTSC_INFO() is used.
                                Default = true.

   "read_fifo_depth"    u32     This is the number of read elements that the
                                xtsc_tx_loader can hold.
                                Default = 1.

   "allow_overflow"     bool    If true, then read fifo data that is not popped will be
                                overwritten after the fifo becomes full.  If false, then
                                an exception will be thrown.
                                Default = true.

   "clock_period"       u32     This is the length of this loader's clock period
                                expressed in terms of the SystemC time resolution (from
                                sc_get_time_resolution()).  A value of 0xFFFFFFFF means
                                to use the XTSC system clock period (from
                                xtsc_get_system_clock_period()).  A value of 0 means one
                                delta cycle.
                                Default = 0xFFFFFFFF (i.e. use the system clock 
                                period).

   "vcd_handle"         void*   Pointer to SystemC VCD object (sc_trace_file *) or 0 if
                                tracing is not desired.  Tracing is supported regardless
                                of the "pin_level" setting.
                                Default = 0 (NULL).

   "pin_level"          bool    If true, pin-level connections are used for the queue
                                interfaces.
                                Default = false (TLM connections are used).

   Note:  The following parameters only apply if "pin_level" is true:

   "posedge_offset"     u32     This specifies the time at which the first posedge of
                                this device's clock conceptually occurs.  It is
                                expressed in units of the SystemC time resolution and
                                the value implied by it must be strictly less than the
                                value implied by the "clock_period" parameter.  A value
                                of 0xFFFFFFFF means to use the same posedge offset as
                                the system clock (from
                                xtsc_get_system_clock_posedge_offset()).
                                Default = 0xFFFFFFFF.

   "sample_phase"       u32     This specifies the phase (i.e. the point) in each clock
                                period at which the signals are sampled.  It is
                                expressed in terms of the SystemC time resolution (from
                                sc_get_time_resolution()) and must be strictly less than
                                the clock period as specified by the "clock_period"
                                parameter.  A value of 0 means sampling occurs at
                                posedge clock as specified by "posedge_offset".
                                Default = 0.

   "output_delay"       u32     This specifies how long to delay before queue output
                                pins are driven.  The delay is expressed in terms of the
                                SystemC time resolution (from sc_get_time_resolution())
                                and must be less than the clock period.  A value of 0
                                means one delta cycle.
                                Default = 1 (i.e. 1 time resolution).

    \endverbatim
 *
 * @see xtsc_tx_loader
 * @see xtsc_parms
 */
class XTSC_API xtsc_tx_loader_parms : public xtsc_parms {
public:


  /**
   * Constructor for an xtsc_tx_loader_parms object.
   *
   * @param     image_file      Name of the optional image file to load.
   *
   */
  xtsc_tx_loader_parms(const char *image_file = NULL) {
    add("image_file",           image_file);
    add("hex_format",           true);
    add("binary_format",        false);
    add("turbo",                xtsc_get_xtsc_initialize_parms().get_bool("turbo"));
    add("squelch_loading",      true);
    add("read_fifo_depth",      1);
    add("allow_overflow",       true);
    add("clock_period",         0xFFFFFFFF);
    add("vcd_handle",           (void*) NULL); 
    add("pin_level",            false);
    add("posedge_offset",       0xFFFFFFFF);
    add("sample_phase",         0);
    add("output_delay",         1);
  }


  /// Return what kind of xtsc_parms this is (our C++ type)
  virtual const char* kind() const { return "xtsc_tx_loader_parms"; }

};





/**
 * XTSC module to model a boot loader for a TX Xtensa chain.
 *
 * This module is designed to model the boot loader provided by Tensilica.  It can be
 * provided an image file to load or it can be driven by its input queue interface
 * (m_producer).  The input queue interface can be driven by an xtsc_core with a 32-bit
 * TIE output queue interface or by an xtsc_queue_producer.  Read data can be obtained
 * from the output queue interface (m_consumer).  The output queue interface can be
 * read by an xtsc_core with a 32-bit TIE input queue interface or by an
 * xtsc_queue_consumer.
 *
 * If desired, an image file can be provided and after the image file is loaded the
 * input queue interface can be used.
 *
 * If desired, this module can be driven by a Verilog module when doing SystemC-Verilog
 * cosimulation.  To do this, set the "pin_level" parameter to true to cause the 
 * command input queue and the read data output queue interfaces to be pin-level.  The
 * Done and Mode wire outputs will remain as TLM interfaces, but transactors can be used
 * to convert them to pin-level.  The xtsc-run program will automatically take care of
 * this for you.  For more information, see the --connect_proxy_loader and/or
 * --connect_wrapper_loader commands in the xtsc-run reference manual (available using
 * the "xtsc-run --manual" command).
 *
 * Note:  The xtsc_tx_xfer_if has no pin-level counterpart.  Because of this, the TX
 *        chain itself cannot cross the SystemC-Verilog boundary.  That is, when doing
 *        SystemC-Verilog cosimulation with TX cores, the boot loader and the TX cores
 *        it is controlling must all be on the same side of the boundary (all in SystemC
 *        or all in Verilog).  
 *
 * Here is a block diagram of an xtsc_tx_loader as it is used in the xtsc_tx_loader
 * example:
 * @image html  Example_xtsc_tx_loader.jpg
 * @image latex Example_xtsc_tx_loader.eps "xtsc_tx_loader Example" width=10cm
 *
 * @see xtsc_tx_loader_parms
 * @see xtsc_tx_xfer_if
 * @see xtsc_core
 * @see xtsc_component::xtsc_queue_producer
 * @see xtsc_component::xtsc_queue_consumer
 *
 */
class XTSC_API xtsc_tx_loader : public sc_core::sc_module, public xtsc_module {
public:

  sc_core::sc_port  <xtsc_tx_xfer_if, NSPP>     m_tx_xfer_port;         ///<  From us to first TX in the chain
  sc_core::sc_export<xtsc_tx_xfer_if>           m_tx_xfer_export;       ///<  From last TX in the chain to us

  sc_core::sc_port  <xtsc_wire_write_if, NSPP>  m_done;                 ///<  Optional Done output
  sc_core::sc_port  <xtsc_wire_write_if, NSPP>  m_mode;                 ///<  Optional Mode output


  // TLM queue interfaces (if "pin_level" is false):

  sc_core::sc_export<xtsc_queue_push_if>       *m_producer;             ///<  TLM: Optional queue producer binds to this
  sc_core::sc_export<xtsc_queue_pop_if>        *m_consumer;             ///<  TLM: Optional queue consumer binds to this


  // Pin-level queue interfaces (if "pin_level" is true):

  sc_core::sc_in <sc_dt::sc_bv_base>           *m_push;                 ///<  Pin: Push request from producer
  sc_core::sc_in <sc_dt::sc_bv_base>           *m_data_in;              ///<  Pin: Input data from producer
  sc_core::sc_out<sc_dt::sc_bv_base>           *m_full;                 ///<  Pin: Signal producer that queue is full

  sc_core::sc_in <sc_dt::sc_bv_base>           *m_pop;                  ///<  Pin: Pop request from consumer
  sc_core::sc_out<sc_dt::sc_bv_base>           *m_empty;                ///<  Pin: Signal consumer that queue is empty
  sc_core::sc_out<sc_dt::sc_bv_base>           *m_data_out;             ///<  Pin: Output data to consumer


  /// Our C++ type (SystemC uses this)
  virtual const char* kind() const { return "xtsc_tx_loader"; }


  /// For SystemC modules with processes
  SC_HAS_PROCESS(xtsc_tx_loader);

  /**
   * Constructor for an xtsc_tx_loader.
   * @param     module_name     Name of the xtsc_tx_loader sc_module.
   * @param     loader_parms    The remaining parameters for construction.
   * @see xtsc_tx_loader_parms
   */
  xtsc_tx_loader(sc_core::sc_module_name module_name, const xtsc_tx_loader_parms& loader_parms);


  // Destructor.
  ~xtsc_tx_loader(void);


  /// For xtsc_connection_interface
  virtual xtsc::u32 get_bit_width(const std::string& port_name, xtsc::u32 interface_num = 0) const;


  /// For xtsc_connection_interface
  virtual sc_core::sc_object *get_port(const std::string& port_name);


  /// For xtsc_connection_interface
  virtual xtsc_port_table get_port_table(const std::string& port_table_name) const;


  /// SystemC callback
  void before_end_of_elaboration();


  /// SystemC callback
  void end_of_elaboration();


  /// SystemC callback
  void start_of_simulation();


  /**
   * Reset the xtsc_tx_loader.
   */
  void reset(bool hard_reset = false);


  /// Thread to process the image file
  void process_image_file_thread();


  /// Thread to handle TLM queue input
  void handle_tlm_queue_command_input_thread();


  /// Thread to handle pin-level queue input (incoming commands)
  void handle_pin_queue_command_input_thread();


  /// Thread to handle pin-level queue output (read data)
  void handle_pin_queue_read_thread();


  /// Synchronize to the sample phase
  void sync_to_sample_phase(const std::string& thread_name);


  /// SystemC method to drive the read queue's outputs (empty and data signals)
  void drive_read_queue_output_method();


  /// SystemC method to drive the read queue's outputs (empty and data signals)
  void drive_full_queue_output_method();


  /// SystemC method to detect read fifo overflow
  void detect_overflow_method();


  /// Return whether or not the queue interfaces are pin-level or TLM
  bool pin_level() const { return m_pin_level; }


  /**
   * Connect with an upstream xtsc_core.
   *
   * This method connects this loader with the specified upstream xtsc_core.  Which
   * interfaces are connected depends on the iface argument.
   *
   * @param     core            The xtsc_core to connect with.
   *
   * @param     iface           If iface is "tx_xfer_out", then the output XFER interface
   *                            of core is connected to input XFER interface of this
   *                            loader (so core is the last TX in the chain).  If iface
   *                            is not "tx_xfer_out", then \"pin_level\" must be false
   *                            and iface must name a 32-bit TLM TIE output queue
   *                            interface of core, which will be connected to the TLM
   *                            command queue interface (m_producer) of this loader.
   */
  void connect(xtsc_core& core, const char *iface);


  /// Get the TextLogger for this component (e.g. to adjust its log level)
  log4xtensa::TextLogger& get_text_logger() { return m_text; }


  /// Boot loader state
  typedef enum state {
    PowerSave           = 0,
    Configuration       = 1,
    Loading             = 2,
    Reading             = 3,
    ZeroWrites          = 4
  } state;

  /// Return the state as a c-string
  static const char *get_state_string(state s);


  // Internal address
  typedef enum internal_address {
    IA_address          = 0x0,
    IA_length           = 0x2,
    IA_config_read      = 0x3,
    IA_config_write     = 0x4,
    IA_boot_loader_mode = 0x5,
    IA_done_pin_control = 0x6
  } internal_address;


protected:


  /// Implementation of xtsc_tx_xfer_if
  class xtsc_tx_xfer_if_impl : virtual public xtsc_tx_xfer_if, public sc_core::sc_object  {
  public:

    /**
     * Constructor.
     * @param   object_name     The name of this SystemC channel (aka implementation)
     * @param   loader          A reference to the owning xtsc_tx_loader object.
     */
    xtsc_tx_xfer_if_impl(const char *object_name, xtsc_tx_loader& loader);

    /// @see xtsc_tx_xfer_if
    void nb_tx_xfer(xtsc_tx_xfer& tx_xfer);

    /// Return true if a port has bound to this implementation
    bool is_connected() { return (m_p_port != 0); }

    /// Return true if the loader's tx_xfer port is connected to its own export
    bool is_self_connected() { return (m_p_port == &m_loader.m_tx_xfer_port); }

  protected:

    /// SystemC callback when something binds to us
    virtual void register_port(sc_core::sc_port_base& port, const char *if_typename);

    xtsc_tx_loader&             m_loader;               ///<  Our xtsc_tx_loader object
    sc_core::sc_port_base      *m_p_port;               ///<  Port that is bound to us
  };


  /// Implementation of xtsc_queue_push_if.
  class xtsc_queue_push_if_impl : public xtsc_queue_push_if, public sc_core::sc_object  {
  public:

    /// Constructor
    xtsc_queue_push_if_impl(const char *object_name, xtsc_tx_loader& loader) :
      sc_object (object_name),
      m_loader  (loader),
      m_p_port  (0)
    {}

    /// @see xtsc_queue_push_if
    bool nb_can_push();

    /// @see xtsc_queue_push_if
    bool nb_push(const sc_dt::sc_unsigned& element, u64& ticket = push_ticket);

    /// @see xtsc_queue_push_if
    u32 nb_get_bit_width() { return 32; }

    /**
     * Get the event that will be notified when the queue transitions from full to not
     * full.
     */
    virtual const sc_core::sc_event& default_event() const { return m_loader.m_can_accept_push_event; }

  private:

    /// SystemC callback when something binds to us
    virtual void register_port(sc_core::sc_port_base& port, const char *if_typename);

    xtsc_tx_loader&             m_loader;               ///< Our xtsc_tx_loader object
    sc_core::sc_port_base      *m_p_port;               ///< Port that is bound to us
  };


  /// Implementation of xtsc_queue_pop_if.
  class xtsc_queue_pop_if_impl : public xtsc_queue_pop_if, public sc_core::sc_object  {
  public:

    /// Constructor
    xtsc_queue_pop_if_impl(const char *object_name, xtsc_tx_loader& loader) :
      sc_object (object_name),
      m_loader  (loader),
      m_p_port  (0)
    {}

    /// @see xtsc_queue_pop_if
    bool nb_can_pop();

    /// @see xtsc_queue_pop_if
    bool nb_pop(sc_dt::sc_unsigned& element, u64& ticket = pop_ticket);

    /// @see xtsc_queue_pop_if
    u32 nb_get_bit_width() { return 32; }

    /**
     * Get the event that will be notified when the queue transitions from empty
     * to not empty.
     */
    virtual const sc_core::sc_event& default_event() const { return m_loader.m_have_read_data_event; }


  private:

    /// SystemC callback when something binds to us
    virtual void register_port(sc_core::sc_port_base& port, const char *if_typename);

    xtsc_tx_loader&             m_loader;               ///< Our xtsc_tx_loader object
    sc_core::sc_port_base      *m_p_port;               ///< Port that is bound to us
  };


  /// Set the state, log it, and drive m_mode (if bound)
  void set_state(state s);


  /// Process the command word
  void do_command(u32 word, bool turbo);


  /// Get next word from image file.  Returns false at end-of-file
  bool get_next_word_from_image_file(u32& word);


  /// Log and send out an xtsc_tx_xfer
  void send_xfer(xtsc_tx_xfer& tx_xfer);


  /// Get an xtsc_tx_xfer from the pool
  xtsc_tx_xfer *new_xfer(bool done, u32 address, u32 data, bool write, bool turbo);


  /// Return an xtsc_tx_xfer to the pool
  void delete_tx_xfer(xtsc_tx_xfer*& p_tx_xfer);


  xtsc_signal_sc_bv_base_floating      *m_push_floating;                ///<  Pin: To cap unused push interface
  xtsc_signal_sc_bv_base_floating      *m_data_in_floating;             ///<  Pin: To cap unused push interface
  xtsc_signal_sc_bv_base_floating      *m_full_floating;                ///<  Pin: To cap unused push interface

  xtsc_signal_sc_bv_base_floating      *m_pop_floating;                 ///<  Pin: To cap unused pop interface
  xtsc_signal_sc_bv_base_floating      *m_empty_floating;               ///<  Pin: To cap unused pop interface
  xtsc_signal_sc_bv_base_floating      *m_data_out_floating;            ///<  Pin: To cap unused pop interface

  xtsc_tx_xfer_if_impl                  m_tx_xfer_impl;                 ///<  m_tx_xfer_export binds to this
  xtsc_queue_push_if_impl              *m_queue_push_impl;              ///<  m_producer binds to this
  xtsc_queue_pop_if_impl               *m_queue_pop_impl;               ///<  m_consumer binds to this

  bool                                  m_pin_level;                    ///<  See the "pin_level" parameter

  sc_core::sc_trace_file               *m_p_trace_file;                 ///<  From the "vcd_handle" parameter
  xtsc_tx_xfer                          m_outgoing_tx_xfer;             ///<  For VCD tracing
  xtsc_tx_xfer                          m_incoming_tx_xfer;             ///<  For VCD tracing
  u32                                   m_outgoing_count;               ///<  For VCD tracing
  u32                                   m_incoming_count;               ///<  For VCD tracing
  u32                                   m_push_word;                    ///<  For VCD tracing
  u32                                   m_read_word;                    ///<  For VCD tracing
  u32                                   m_push_count;                   ///<  For VCD tracing
  u32                                   m_read_count;                   ///<  For VCD tracing

  std::string                           m_image_file_name;              ///<  See the "image_file" parameter
  std::ifstream                         m_image_file;                   ///<  The "image_file" stream
  u32                                   m_word_number;                  ///<  word/line number in "image_file"
  bool                                  m_hex_format;                   ///<  See the "hex_format" parameter
  bool                                  m_binary_format;                ///<  See the "binary_format" parameter
  bool                                  m_turbo;                        ///<  See the "turbo" parameter
  bool                                  m_squelch_loading;              ///<  See the "squelch_loading" parameter
  sc_core::sc_time                      m_time_resolution;              ///<  The SystemC time resolution
  sc_core::sc_time                      m_clock_period;                 ///<  This loader's clock period
  u64                                   m_clock_period_value;           ///<  Clock period as u64
  bool                                  m_has_posedge_offset;           ///<  True if m_posedge_offset is non-zero
  sc_core::sc_time                      m_posedge_offset;               ///<  From "posedge_offset" parameter
  u64                                   m_posedge_offset_value;         ///<  m_posedge_offset as u64
  sc_core::sc_time                      m_sample_phase;                 ///<  From "sample_phase" parameter
  u64                                   m_sample_phase_value;           ///<  Sample phase as u64
  sc_core::sc_time                      m_output_delay;                 ///<  From "output_delay" parameter

  u32                                   m_address;                      ///<  Address for loading, reading, zeroing
  u32                                   m_counter;                      ///<  Counter for loading, reading, zeroing
  bool                                  m_processing_image_file;        ///<  Image file processing is in progress
  state                                 m_state;                        ///<  Boot loader state (aka Mode)
  sc_dt::sc_unsigned                    m_mode_unsigned;                ///<  Boot loader state (aka Mode) as sc_unsigned
  sc_dt::sc_unsigned                    m_done_unsigned;                ///<  Done output as sc_unsigned
  sc_dt::sc_bv_base                     m_read_bv_base;                 ///<  Read queue output as sc_bv_base
  sc_dt::sc_bv_base                     m_full_bv_base;                 ///<  Command queue full output as sc_bv_base
  bool                                  m_self_connected;               ///<  m_tx_xfer_port is connected to m_tx_xfer_export
  bool                                  m_done_bound;                   ///<  m_done is connected
  bool                                  m_mode_bound;                   ///<  m_mode is connected
  bool                                  m_push_bound;                   ///<  pin-level input queue interface is connected
  bool                                  m_read_bound;                   ///<  pin-level output queue interface is connected
  u32                                   m_read_fifo_depth;              ///<  See the "read_fifo_depth" parameter
  bool                                  m_allow_overflow;               ///<  See the "allow_overflow" parameter
  sc_core::sc_fifo<u32>                 m_push_fifo;                    ///<  FIFO of write commands from m_producer
  sc_core::sc_fifo<u32>                 m_read_fifo;                    ///<  FIFO of read data tokens available via m_consumer 
  std::deque<u32>                       m_read_deque;                   ///<  Deque of actual read data to allow look ahead
  sc_core::sc_event                     m_have_read_data_event;         ///<  Notified when m_consumer (m_read_fifo) has data
  sc_core::sc_event                     m_can_accept_push_event;        ///<  Notified when another push can be accepted
  sc_core::sc_event                     m_have_queue_input_event;       ///<  Used to notify handle_tlm_queue_command_input_thread
  sc_core::sc_event                     m_drive_read_queue_output_event;///<  Used to notify drive_read_queue_output_method
  sc_core::sc_event                     m_drive_full_queue_output_event;///<  Used to notify drive_full_queue_output_method
  sc_core::sc_event                     m_detect_overflow_event;        ///<  Used to notify detect_overflow_method
  
  u64                                   m_fifo_overflow_delta_cycle;    ///<  Delta cycle in which m_read_fifo overflow occurred
  u32                                   m_fifo_overflow_old_data;       ///<  Data that was overwritten by the m_read_fifo overflow
  u64                                   m_pop_delta_cycle;              ///<  Delta cycle in which last pop of m_read_fifo occurred

  std::vector<xtsc_tx_xfer*>            m_tx_xfer_pool;                 ///<  Maintain a pool of xfers to improve performance

  log4xtensa::TextLogger&               m_text;                         ///<  Text logger


};



}  // namespace xtsc




#endif  // _XTSC_TX_LOADER_H_
