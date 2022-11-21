#ifndef _XTSC_QUEUE_PRODUCER_H_
#define _XTSC_QUEUE_PRODUCER_H_

// Copyright (c) 2006-2017 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
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
#include <xtsc/xtsc_queue_push_if.h>
#include <string>
#include <vector>
#include <fstream>




namespace xtsc {
class xtsc_tx_loader;
}



namespace xtsc_component {

class xtsc_wire_logic;
class xtsc_mmio;

/**
 * Constructor parameters for a xtsc_queue_producer object.
 *
 * This class contains the constructor parameters for a xtsc_queue_producer object.
 *  \verbatim
   Name                 Type    Description
   ------------------   ----    --------------------------------------------------------
  
   "control"            bool    If true, then a 1-bit TLM control input will be created
                                and the "WAIT CONTROL" commands will be enabled in the
                                script file (see "script_file").  The control input can
                                be used to control the xtsc_queue_producer device with
                                another device.
                                Default = false.
                                Note: The control input is a TLM interface regardless of
                                      the "pin_level" setting.

   "nx"                 bool    When "pin_level" is true, this parameter determines
                                whether the producer has NX or LX TIE output queue
                                interface pins.  When "pin_level" is false, this
                                parameter essentially has no effect.
                                Default = false (have LX TIE output queue interface).

   "pin_level"          bool    If true, pin-level queue connections are used.
                                Default = false (TLM connections are used).

   "vcd_handle"         void*   Pointer to SystemC VCD object (sc_trace_file *) or 0 if
                                tracing is not desired.  This parameter is ignored if
                                "pin_level" is false.
                                Default = 0 (NULL).

   "script_file"        char*   The file to read the test vector commands from.  Each
                                command occupies one line in the file.  Valid command
                                formats are shown below (the first format shows a queue
                                push transaction command):

                                  <value>
                                  <delay> <value> [<timeout>] 
                                  <delay> STOP
                                  WAIT  <duration>
                                  WAIT  CONTROL WRITE|CHANGE|<value> { <count> }
                                  SYNC  <time>
                                  TEST_FULL
                                  NOTE  message
                                  INFO  message
                                
                                1.  Integers can appear in decimal or hexadecimal (using
                                    '0x' prefix) format.
                                2.  <delay> specifies how long to wait after completing
                                    the previous command before attempting to push a
                                    value into the queue or stopping the simulation.
                                    <delay> can be 0 (to mean 1 delta cycle), or "now"
                                    to mean no delta cycle delay, or a positive integer
                                    or floating point number to mean that many clock
                                    periods (see "clock_period").
                                    Note:  For pin-level connections, a push command
                                    is considered to be complete when the push request
                                    is deasserted.  See parameter "deassert_delay".
                                3.  <value> specifies the value to push into the queue.
                                    It can be specified in hex or decimal format.
                                    If <value> is specified by itself (that is, without
                                    a <delay>), then a wait is performed to the clock
                                    phase specified by "request_phase" (if not already
                                    there) and then the value is pushed until it is
                                    accepted.
                                4.  The optional <timeout> specifies how long to
                                    continue attempting to push <value> into the queue.
                                    For a TLM-level connection (e.g. to xtsc_queue) and
                                    for NX pin-level connections (e.g. to xtsc_queue_pin),
                                    <timeout> specifies a non-zero integer number of
                                    clock periods and the push attempt is repeated each
                                    clock period for <timeout> number of clock periods
                                    or until the push attempt succeeds.  For an LX pin-
                                    level connection (e.g. to xtsc_queue_pin), <timeout>
                                    is multiplied by the module's clock period (see
                                    "clock_period") to determine the maximum amount of
                                    time the push request will be asserted before being
                                    abandoned if it has not yet been not granted.  For
                                    an LX pin-level connection, <timeout> can be an
                                    integer or a floating point number.  For either a
                                    TLM-level or a pin-level connection, a negative or
                                    missing <timeout> value means to keep trying to push
                                    until it succeeds.
                                5.  The "<delay> STOP" command causes simulation to stop
                                    via a call to the sc_stop() method after the
                                    specified delay.
                                6.  The "WAIT <duration>" command can be used to cause a 
                                    wait of the specified duration.  <duration> can be 0
                                    (to mean 1 delta cycle) or a positive integer or
                                    floating point number to mean that many clock
                                    periods.
                                7.  If the "control" parameter was set to true then the
                                    "WAIT CONTROL" command can be used to cause a wait
                                    until the specified activity occurs on the control
                                    input.  WRITE means any write even if its the same
                                    value, CHANGE means a write of a new value, and
                                    <value> (which can only be 0 or 1) means a write of
                                    the specified value.  An optional <count> can be
                                    specified to mean the event has to occur <count>
                                    times.  The default <count> is 1.  The default event
                                    is WRITE (so "wait control" is the same thing as
                                    "wait control write 1").
                                8.  The "SYNC <time>" command with <time> less than 1.0
                                    can be used to cause a wait to the clock phase
                                    (relative to posedge clock) specified by <time>.
                                    Posedge clock is as specified by "posedge_offset".
                                    For example, "sync 0.5" will cause the minimum wait
                                    necessary to sync to negedge clock.
                                    The "SYNC <time>" command with <time> greater than
                                    or equal to 1.0 can be used to cause a wait until
                                    the specified absolute simulation time.
                                9.  The "TEST_FULL" command causes the nb_can_push()
                                    method to be called (TLM-level), or the m_pushable
                                    signal (NX pin-level) or the m_full signal (LX
                                    pin-level) to be sampled and the result to be
                                    logged.
                               10.  The NOTE and INFO commands can be used to cause
                                    the entire line to be logged at NOTE_LOG_LEVEL
                                    or INFO_LOG_LEVEL, respectively.
                               11.  Words are case insensitive.
                               12.  Comments, extra whitespace, blank lines, and lines
                                    between "#if 0" and "#endif" are ignored.  
                                    See xtsc_script_file for a complete list of
                                    pseudo-preprocessor commands.

   "bit_width"          u32     Width of each queue element in bits.

   "clock_period"       u32     This is the length of this module's clock period
                                expressed in terms of the SystemC time resolution
                                (from sc_get_time_resolution()).  A value of 
                                0xFFFFFFFF means to use the XTSC system clock 
                                period (from xtsc_get_system_clock_period()).
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

   "request_phase"      u32     This specifies the phase (i.e. the point) in the clock
                                period at which the request occurs ("pin_level" false)
                                or starts ("pin_level" true) when using the plain
                                <value> line format.  That is, this parameter only
                                applies to line formats of:
                                  <value>
                                It does NOT apply to line formats of:
                                  <delay> <value> [<timeout>] 
                                It is expressed in terms of the SystemC time resolution
                                (from sc_get_time_resolution()) and must be strictly
                                less than the clock period as specified by the
                                "clock_period" parameter.  A value of 0 means the
                                request occurs at posedge clock as specified by
                                "posedge_offset".  A value of 0xFFFFFFFF means to use 0
                                if "pin_level" is false and to use Phase B if true (see 
                                xtsc_core::set_clock_phase_delta_factors documentation).
                                Note:  When "pin_level" is true it is recommended that
                                this value NOT be set to 0.
                                Default = 0xFFFFFFFF.

   "sample_phase"       u32     This specifies the phase (i.e. the point) in each clock
                                period at which the m_push and m_pushable (NX) or m_full
                                (LX) signals are sampled.  It is expressed in terms of
                                the SystemC time resolution (from
                                sc_get_time_resolution()) and must be strictly less
                                than the clock period as specified by the
                                "clock_period" parameter.  A value of 0 means sampling
                                occurs at posedge clock as specified by "posedge_offset".
                                This parameter is for pin-level connection only.
                                Default = 0.

   "deassert_delay"     u32     During a push attempt to an LX pin-level interface, this
                                specifies how long after the m_full signal is sampled
                                and found to be false, that the m_push signal should be
                                deasserted.  During a push to an NX pin-level interface,
                                this specifies how long after the m_push signal is
                                sampled that is should be deasserted.  It is expressed
                                in terms of the SystemC time resolution (from
                                sc_get_time_resolution()) and must be strictly less
                                than the clock period as specified by the "clock_period"
                                parameter.  A value of 0 means 1 delta cycle.  This
                                parameter is for pin-level connection only.
                                Default = 0.

    \endverbatim
 *
 * @see xtsc_queue_producer
 * @see xtsc::xtsc_parms
 * @see xtsc::xtsc_script_file
 */
class XTSC_COMP_API xtsc_queue_producer_parms : public xtsc::xtsc_parms {
public:

  /**
   * Constructor for an xtsc_queue_producer_parms object.
   *
   * @param     script_file             The file name to read the test vectors from.
   *
   * @param     width1                  Width of each queue element in bits.
   *
   *
   */
  xtsc_queue_producer_parms(const char *script_file = 0, xtsc::u32 width1 = 32) {
    add("control",              false);
    add("nx",                   false);
    add("pin_level",            false);
    add("vcd_handle",           (void*) NULL); 
    add("script_file",          script_file);
    add("bit_width",            width1);
    add("clock_period",         0xFFFFFFFF);
    add("posedge_offset",       0xFFFFFFFF);
    add("request_phase",        0xFFFFFFFF);
    add("sample_phase",         0);
    add("deassert_delay",       0);
  }


  /// Return what kind of xtsc_parms this is (our C++ type)
  virtual const char* kind() const { return "xtsc_queue_producer_parms"; }

};





/**
 * A scripted producer to supply a queue using an LX or NX TIE output queue interface.
 *
 * This XTSC module implements a queue producer that reads an input file ("script_file")
 * to determine when and what data values to send to a queue module.  
 *
 * This module provides a simple means to deliver test transactions to a queue at the
 * TLM-level (such as xtsc_queue) or at the pin-level (such as xtsc_queue_pin).  To use
 * pin-level queue connections, you must set the "pin_level" parameter to true.
 *
 * To provide a degree of feedback or control of the script, the "control" option can be
 * set to true and a wire writer such as xtsc::xtsc_core, xtsc_mmio, or xtsc_wire_logic
 * can be connected to the control input.  This allows the xtsc_queue_producer device to
 * better model certain SoC components.  To perform port binding of the control input,
 * use the get_control_input() method to obtain a reference to the
 * sc_export<xtsc_wire_write_if> or use the xtsc::xtsc_connect() method.
 *
 * Here is a block diagram of an xtsc_queue_producer as it is used in the queue consumer
 * example:
 * @image html  xtsc_queue_producer.jpg
 * @image latex xtsc_queue_producer.eps "xtsc_queue_consumer Example" width=10cm
 *
 * @see xtsc_queue_producer_parms
 * @see xtsc::xtsc_queue_push_if
 * @see xtsc::xtsc_connect()
 * @see xtsc::xtsc_core::How_to_do_port_binding
 */
//  virtual public xtsc::xtsc_queue_push_if is for capping m_queue when "pin_level" is true
class XTSC_COMP_API xtsc_queue_producer : 
          public sc_core::sc_module, 
          public xtsc::xtsc_module, 
          public xtsc::xtsc_command_handler_interface,
  virtual public xtsc::xtsc_queue_push_if
{
public:
 
  // See get_control_input() if the "control" parameter was set to true

  sc_core::sc_out<sc_dt::sc_bv_base>            m_push;         ///<  pin-level push request (LX) or command (NX) to queue
  sc_core::sc_out<sc_dt::sc_bv_base>            m_data;         ///<  pin-level data to queue
  sc_core::sc_in <sc_dt::sc_bv_base>            m_pushable;     ///<  pin-level number of pushable empty slots from queue   (NX only)
  sc_core::sc_in <sc_dt::sc_bv_base>            m_full;         ///<  pin-level full signal from queue                      (LX only)

  sc_core::sc_port<xtsc::xtsc_queue_push_if>    m_queue;        ///<  TLM port to the queue


  // For SystemC
  SC_HAS_PROCESS(xtsc_queue_producer);


  /// Our C++ type (SystemC uses this)
  virtual const char* kind() const { return "xtsc_queue_producer"; }


  /**
   * Constructor for an xtsc_queue_producer.
   *
   * @param     module_name     Name of the xtsc_queue_producer sc_module.
   *
   * @param     producer_parms  The remaining parameters for construction.
   *
   * @see xtsc_queue_producer_parms
   */
  xtsc_queue_producer(sc_core::sc_module_name module_name, const xtsc_queue_producer_parms& producer_parms);


  // Destructor.
  ~xtsc_queue_producer(void);


  /// For xtsc_connection_interface
  virtual xtsc::u32 get_bit_width(const std::string& port_name, xtsc::u32 interface_num = 0) const;


  /// For xtsc_connection_interface
  virtual sc_core::sc_object *get_port(const std::string& port_name);


  /// For xtsc_connection_interface
  virtual std::string get_default_port_name() const { return "queue_push"; }


  /// For xtsc_connection_interface
  virtual xtsc::xtsc_port_table get_port_table(const std::string& port_table_name) const;


  /**
   * Reset the xtsc_queue_producer.
   */
  void reset(bool hard_reset = false);


  /**
   * Return the sc_export of the optional control input.
   *
   * This method may be used for port binding of the optional control input.
   *
   * For example, to bind the TIE export state named "onebit" of an xtsc_core named core0
   * to the control input of an xtsc_queue_producer named producer:
   * \verbatim
       core0.get_export_state("onebit")(producer.get_control_input());
     \endverbatim
   */
  sc_core::sc_export<xtsc::xtsc_wire_write_if>& get_control_input() const;


  /**
   * Implementation of the xtsc::xtsc_command_handler_interface.
   *
   * This implementation supports the following commands:
   *  \verbatim
        can_push
          Return nb_can_push() (TLM), or m_pushable>0 (NX pin-level),
          or !m_full (LX pin-level).

        get_push_count
          Return number of items pushed since simulation began.

        get_push_ticket
          Return the ticket of most recent previous element pushed.
          Not available if "pin_level" is true.

        num_free_spaces
          Return nb_num_free_spaces() (TLM), or m_pushable (NX pin-level),
          or 0|1 depending on the m_full signal (LX pin-level).

        push <Value>
          Return nb_push(<Value>).
          Not available if "pin_level" is true.

        reset
          Call xtsc_queue_producer::reset().
      \endverbatim
   */
  virtual void execute(const std::string&               cmd_line,
                       const std::vector<std::string>&  words,
                       const std::vector<std::string>&  words_lc,
                       std::ostream&                    result);


  /**
   * Connect to a downstream xtsc_tx_loader.
   *
   * This method connects the m_queue sc_port of this xtsc_queue_producer to the
   * m_producer sc_export of an xtsc_tx_loader.
   *
   * @param     loader          The xtsc_tx_loader to connect to.
   */
  void connect(xtsc::xtsc_tx_loader& loader);


  /**
   * Connect an xtsc_wire_logic output to the control input of this xtsc_queue_producer.
   *
   * This method connects the specified output of the specified xtsc_wire_logic to the
   * optional control input of this xtsc_queue_producer.  This method should not be used
   * unless the "control" parameter was set to true.
   *
   * @param     logic           The xtsc_wire_logic to connect to the control input of
   *                            this xtsc_queue_producer.
   *
   * @param     output_name     The output of the xtsc_wire_logic. 
   */
  void connect(xtsc_wire_logic& logic, const char *output_name);


  /**
   * Connect an xtsc_mmio output to the control input of this xtsc_queue_producer.
   *
   * This method connects the specified output of the specified xtsc_mmio to the
   * optional control input of this xtsc_queue_producer.  This method should not be used
   * unless the "control" parameter was set to true.
   *
   * @param     mmio            The xtsc_mmio to connect to the control input of
   *                            this xtsc_queue_producer.
   *
   * @param     output_name     The output of the xtsc_mmio. 
   */
  void connect(xtsc_mmio& mmio, const char *output_name);


  /// Thread to process commands from m_script_file
  void script_thread(void);


  /// Thread to sample the m_push and m_pushable/m_full signals to see if a push occurred (NX) or succeeded (LX)
  void sample_thread(void);


  /// Thread to drive the push request 
  void request_thread(void);


  /// Get the TextLogger for this component (e.g. to adjust its log level)
  log4xtensa::TextLogger& get_text_logger() { return m_text; }


protected:


  class xtsc_wire_write_if_impl : public xtsc::xtsc_wire_write_if, public sc_core::sc_object {
  public:

    /// Constructor
    xtsc_wire_write_if_impl(const std::string& port_name, xtsc_queue_producer& producer) :
      sc_object         (port_name.c_str()),
      m_producer        (producer),
      m_name            (port_name),
      m_bit_width       (1),
      m_p_port          (0)
    {}

    /// The kind of sc_object we are
    const char* kind() const { return "xtsc_queue_producer::xtsc_wire_write_if_impl"; }

    /**
     *  Receive writes from the wire source
     *  @see xtsc::xtsc_wire_write_if
     */
    virtual void nb_write(const sc_dt::sc_unsigned& value);

    /**
     *  Get the wire width in bits.
     *  @see xtsc::xtsc_wire_write_if
     */
    virtual xtsc::u32 nb_get_bit_width() { return m_bit_width; }


  protected:

    /// SystemC callback when something binds to us
    void register_port(sc_core::sc_port_base& port, const char *if_typename);

    xtsc_queue_producer&      m_producer;       ///< Our xtsc_queue_producer object
    std::string               m_name;           ///< Our name as a std::string
    xtsc::u32                 m_bit_width;      ///< Width in bits
    sc_core::sc_port_base    *m_p_port;         ///< Port that is bound to us
  };



  typedef sc_core::sc_export<xtsc::xtsc_wire_write_if>          wire_write_export;

  log4xtensa::TextLogger&               m_text;
  bool                                  m_control;                      ///<  From "control" parameter
  bool                                  m_control_bound;                ///<  Something is connected to the control input
  wire_write_export                    *m_p_control;                    ///<  Optional control input
  xtsc_wire_write_if_impl              *m_p_write_impl;                 ///<  Implementaion for optional control input
  sc_dt::sc_unsigned                    m_control_value;                ///<  Current value of the control input
  xtsc::u32                             m_control_write_count;          ///<  Number of times control input is written
  xtsc::u32                             m_control_change_count;         ///<  Number of times control input is written with a new value
  xtsc::xtsc_script_file                m_test_vector_stream;           ///<  Script file      from "script_file" parameter
  std::string                           m_script_file;                  ///<  Script file name from "script_file" parameter
  std::string                           m_line;                         ///<  Current line        from "script_file"
  xtsc::u32                             m_line_count;                   ///<  Current line number from "script_file"
  std::vector<std::string>              m_words;                        ///<  Current line        from "script_file" tokenized into words
  xtsc::u64                             m_clock_period_value;           ///<  This device's clock period as u64
  sc_core::sc_time                      m_clock_period;                 ///<  From "clock_period" parameter
  sc_core::sc_time                      m_time_resolution;              ///<  The SystemC time resolution
  bool                                  m_has_posedge_offset;           ///<  True if m_posedge_offset is non-zero
  sc_core::sc_time                      m_posedge_offset;               ///<  From "posedge_offset" parameter
  xtsc::u64                             m_posedge_offset_value;         ///<  m_posedge_offset as u64
  sc_core::sc_time                      m_request_phase;                ///<  From "request_phase" parameter
  sc_core::sc_time                      m_sample_phase;                 ///<  From "sample_phase" parameter
  sc_core::sc_time                      m_previous_push;                ///<  Time of most recent previous push
  sc_core::sc_time                      m_deassert_delay;               ///<  From "deassert_delay" parameter
  xtsc::u64                             m_ticket;                       ///<  Save value of last ticket pushed
  xtsc::u32                             m_width1;                       ///<  Bit width of each element
  bool                                  m_nx;                           ///<  From "nx" parameter
  bool                                  m_pin_level;                    ///<  From "pin_level" parameter
  sc_core::sc_trace_file               *m_p_trace_file;                 ///<  From "vcd_handle" parameter
  xtsc::u64                             m_push_count;                   ///<  Number of items pushed
  sc_dt::sc_unsigned                    m_value;                        ///<  Value from "script_file" when not operating "pin_level"
  sc_dt::sc_bv_base                     m_value_bv;                     ///<  Value from "script_file" when     operating "pin_level"
  sc_dt::sc_bv_base                     m_zero_bv;                      ///<  Constant 0
  sc_dt::sc_bv_base                     m_one_bv;                       ///<  Constant 1
  sc_core::sc_event                     m_control_write_event;          ///<  Notified when control input is written
  sc_core::sc_event                     m_next_request;                 ///<  To notify script_thread to advance when operating "pin_level"
  sc_core::sc_event                     m_assert;                       ///<  To notify request_thread to   assert the request
  sc_core::sc_event                     m_deassert;                     ///<  To notify request_thread to deassert the request
  xtsc::u64                             m_assert_delta_cycle;           ///<  Handle thread scheduling indeterminacy at time 0

  std::vector<sc_core::sc_process_handle>
                                        m_process_handles;              ///<  For reset 


  /// Extract a u32 value (named argument_name) from the word at m_words[index]
  xtsc::u32 get_u32(xtsc::u32 index, const std::string& argument_name);

  /// Extract a double value (named argument_name) from the word at m_words[index]
  double get_double(xtsc::u32 index, const std::string& argument_name);

  /// Method to check interface width 
  void end_of_elaboration();

  // These 3 methods are to cap off the TLM port (m_queue) when this producer is 
  // connected at the pin-level to, for example, xtsc_queue_pin.  These
  // methods will never be called
  bool nb_can_push()                                                            { return false; }
  bool nb_push(const sc_dt::sc_unsigned& /*element*/, xtsc::u64& /*ticket*/)    { return false; }
  xtsc::u32 nb_get_bit_width()                                                  { return m_width1; }

  // These signals are to cap off the pin-level ports when this producer is 
  // connected at the TLM level to, for example, xtsc_queue.
  xtsc::xtsc_signal_sc_bv_base_floating m_push_floating;
  xtsc::xtsc_signal_sc_bv_base_floating m_data_floating;
  xtsc::xtsc_signal_sc_bv_base_floating m_pushable_floating;
  xtsc::xtsc_signal_sc_bv_base_floating m_full_floating;

};



}  // namespace xtsc_component



#endif  // _XTSC_QUEUE_PRODUCER_H_
