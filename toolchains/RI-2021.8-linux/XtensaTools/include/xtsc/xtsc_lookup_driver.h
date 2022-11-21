#ifndef _XTSC_LOOKUP_DRIVER_H_
#define _XTSC_LOOKUP_DRIVER_H_

// Copyright (c) 2006-2012 by Tensilica Inc.  ALL RIGHTS RESERVED.
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
#include <xtsc/xtsc_lookup_if.h>
#include <string>
#include <vector>
#include <fstream>



namespace xtsc {
class xtsc_interrupt_distributor;
}



namespace xtsc_component {


/**
 * Constructor parameters for a xtsc_lookup_driver object.
 *
 * This class contains the constructor parameters for a xtsc_lookup_driver object.
 *  \verbatim
   Name                 Type    Description
   ------------------   ----    --------------------------------------------------------
  
   "address_bit_width"  u32     Width of request address in bits.

   "data_bit_width"     u32     Width of respone data in bits.

   "has_ready"          bool    Specifies whether the lookup device has a ready signal.
                                This corresponds to the rdy keyword in the user's TIE
                                code for the lookup.

   "latency"            u32     The latency is defined by the <use_stage> and
                                <def_stage> values in the user TIE code defining the
                                TIE lookup port according to the following formula:
                                  latency = <use_stage> - <def_stage>
                                If "has_ready" is true, "latency" specfies how many 
                                clock cycles after m_ready (TIE_xxx_Rdy) is asserted
                                that m_data (TIE_xxx_In) is valid.
                                If "has_ready" is false, "latency" specfies how many 
                                clock cycles after m_req (TIE_xxx_Out_Req) is asserted
                                that m_data (TIE_xxx_In) is valid.  "latency" must be
                                greater than 0.
                                Default = 1.

   "pin_level"          bool    If true, pin-level connections are used.
                                Default = false (TLM connections are used).

   "vcd_handle"         void*   Pointer to SystemC VCD object (sc_trace_file *) or 0 if
                                tracing is not desired.  This parameter is ignored if
                                "pin_level" is false.
                                Default = 0 (NULL).

   "script_file"        char*   The file to read the test vector commands from.  Each
                                command occupies one line in the file.  Valid command
                                formats are shown below (the first format shows a lookup
                                transaction command):

                                  <delay> <address> [<timeout>]
                                  <delay> STOP
                                  WAIT  <duration>
                                  SYNC  <time>
                                  NOTE  message
                                  INFO  message
                                
                                1.  Integers can appear in decimal or hexadecimal (using
                                    '0x' prefix) format.
                                2.  <delay> specifies how long to wait before sending
                                    the <address> request to the lookup or stopping 
                                    simulation.  <delay> can be 0 (to mean 1 delta
                                    cycle), or "now" to mean no delta cycle delay, or a
                                    positive integer or floating point number to mean
                                    that many clock periods (see "clock_period").
                                3.  <address> specifies the address to send into the lookup.
                                    The <address> format is as specified by Table 3 of
                                    Section 7.3 "String Literals" of the SystemC LRM.
                                    Some example <address> values are:
                                      0x1234            // Hex
                                      0b01101110        // Binary
                                      0d256             // Decimal
                                      256               // Decimal
                                4.  The <timeout> value is only used if "has_ready" is 
                                    true.  It specifies how long to continue sending
                                    the <address> request to the lookup.  For a TLM-level
                                    connection (e.g. to xtsc_lookup), <timeout>
                                    specifies a non-zero integer number of clock
                                    periods and the lookup request is repeated each
                                    clock period for <timeout> number of clock periods
                                    or until the lookup request is granted.  For a
                                    pin-level connection (e.g. to xtsc_lookup_pin),
                                    <timeout> is multiplied by the device's clock
                                    period (see "clock_period") to determine the
                                    maximum amount of time the m_req signal will be
                                    asserted before being abandoned if it has not yet
                                    been not granted.  For a pin-level connection,
                                    <timeout> can be an integer or a floating point
                                    number.  For either a TLM-level or a pin-level
                                    connection, a negative <timeout> value means to
                                    keep trying the request until it is granted.
                                    Default = -1 (keep requesting until granted).
                                5.  The "<delay> STOP" command causes simulation to stop
                                    via a call to the sc_stop() method after the
                                    specified delay.
                                6.  The "WAIT <duration>" command can be used to cause a 
                                    wait of the specified duration.  <duration> can be 0
                                    (to mean 1 delta cycle) or a positive integer or
                                    floating point number to mean that many clock
                                    periods.
                                7.  The "SYNC <time>" command with <time> less than 1.0
                                    can be used to cause a wait to the clock phase
                                    (relative to posedge clock) specified by <time>.
                                    Posedge clock is as specified by "posedge_offset".
                                    For example, "sync 0.5" will cause the minimum wait
                                    necessary to sync to negedge clock.
                                    The "SYNC <time>" command with <time> greater than
                                    or equal to 1.0 can be used to cause a wait until
                                    the specified absolute simulation time.
                                8.  The NOTE and INFO commands can be used to cause
                                    the entire line to be logged at NOTE_LOG_LEVEL
                                    or INFO_LOG_LEVEL, respectively.
                                9.  Words are case insensitive.
                               10.  Comments, extra whitespace, blank lines, and lines
                                    between "#if 0" and "#endif" are ignored.  
                                    See xtsc_script_file for a complete list of
                                    pseudo-preprocessor commands.

   "clock_period"       u32     This is the length of this device's clock period
                                expressed in terms of the SystemC time resolution
                                (from sc_get_time_resolution()).  A value of 
                                0xFFFFFFFF means to use the XTSC system clock 
                                period (from xtsc_get_system_clock_period()).
                                Default = 0xFFFFFFFF (i.e. use the system clock 
                                period).

   "poll_ready_delay"   u32     This parameter is ignored if "pin_level" is true or if
                                "has_ready" is false.  This parameter specifies how long
                                the driver should delay after calling nb_send_address()
                                before calling nb_is_ready().  It is expressed in units
                                of the SystemC time resolution and the value implied by
                                it must be strickly less than the value implied by the
                                "clock_period" parameter.  A value of 0xFFFFFFFF means
                                to delay for one-half of a clock period.  After the call
                                to nb_is_ready(), there is another delay equal to the
                                clock period minus this delay before repeating the call
                                to nb_is_ready() or continuing with the script file.
                                Default = 0xFFFFFFFF (i.e. one-half a clock period).

   Note:  The "poll_ready_delay" parameter was introduced after (not including) RC-2010.1
          to allow the driver to better model xtsc_core behavior.  To get the behavior of
          RC-2010.1 and earlier, set "poll_ready_delay" to 0.

   "posedge_offset"     u32     This specifies the time at which the first posedge of
                                this device's clock conceptually occurs.  It is
                                expressed in units of the SystemC time resolution and
                                the value implied by it must be strictly less than the
                                value implied by the "clock_period" parameter.  A value
                                of 0xFFFFFFFF means to use the same posedge offset as
                                the system clock (from
                                xtsc_get_system_clock_posedge_offset()).
                                Default = 0xFFFFFFFF.

   "sample_phase"       u32     This parameter applies to pin-level connections only.
                                If "has_ready" is true, this parameter specifies the
                                point in each clock period at which the m_ready signal
                                is sampled.  If "has_ready" is false, this parameter
                                specifies the point in each request clock cycle (i.e.
                                a clock cycle in which m_req is asserted) that the
                                "deassert_delay" timing starts.  This parameter is
                                expressed in terms of the SystemC time resolution (from
                                sc_get_time_resolution()) and must be strictly less
                                than the clock period as specified by the
                                "clock_period" parameter.  A value of 0 means sampling
                                occurs (or "deassert_delay" timing starts) at posedge
                                clock as specfied by "posedge_offset".
                                Default = 0.

   "deassert_delay"     u32     This parameter applies to pin-level connections only.
                                If "has_ready" is true, this parameter specifies how
                                long after the m_ready signal is sampled and found to
                                be true, that the m_req signal should be deasserted.
                                If "has_ready" is false, this parameter specifies
                                how long after the "sample_phase" time that the m_req
                                signal should be deasserted.  This parameter is
                                expressed in terms of the SystemC time resolution
                                (from sc_get_time_resolution()) and must be strictly
                                less than the clock period as specified by the
                                "clock_period" parameter.  A value of 0 means the m_req
                                signal will be deasserted 1 delta cycle after the 
                                "sample_phase" time (if m_ready is true or if
                                "has_ready" is false).
                                Default = 0.

    \endverbatim
 *
 * @see xtsc_lookup_driver
 * @see xtsc::xtsc_parms
 * @see xtsc::xtsc_script_file
 */
class XTSC_COMP_API xtsc_lookup_driver_parms : public xtsc::xtsc_parms {
public:

  /**
   * Constructor for an xtsc_lookup_driver_parms object.
   *
   * @param     address_bit_width       Width of request address in bits.
   *
   * @param     data_bit_width          Width of response data in bits.
   *
   * @param     has_ready               Specifies whether or not the lookup device has
   *                                    a ready signal (corresponds to the rdy keyword
   *                                    in the user's TIE code for the lookup).
   *
   * @param     script_file             The file name to read the xtsc::xtsc_request test 
   *                                    vectors from.
   *
   */
  xtsc_lookup_driver_parms(xtsc::u32 address_bit_width = 0, xtsc::u32 data_bit_width = 0, bool has_ready = false, const char *script_file = 0) {
    add("address_bit_width",    address_bit_width);
    add("data_bit_width",       data_bit_width);
    add("has_ready",            has_ready);
    add("latency",              1);
    add("pin_level",            false);
    add("vcd_handle",           (void*) NULL); 
    add("script_file",          script_file);
    add("clock_period",         0xFFFFFFFF);
    add("poll_ready_delay",     0xFFFFFFFF);
    add("posedge_offset",       0xFFFFFFFF);
    add("sample_phase",         0);
    add("deassert_delay",       0);
  }


  /// Return what kind of xtsc_parms this is (our C++ type)
  virtual const char* kind() const { return "xtsc_lookup_driver_parms"; }

};





/**
 * A scripted driver for a lookup.
 *
 * This XTSC device implements a lookup driver that reads an input file ("script_file")
 * to determine when and what lookup requests to send to a lookup device.  
 *
 * This device provides a simple means to deliver test transactions to a lookup at the
 * TLM-level (such as xtsc_lookup) or at the pin-level (such as xtsc_lookup_pin).  To use
 * pin-level connections, you must set the "pin_level" parameter to true.
 *
 * Here is a block diagram of an xtsc_lookup_driver as it is used in the driver example:
 * @image html  xtsc_lookup_driver.jpg
 * @image latex xtsc_lookup_driver.eps "xtsc_lookup_driver Example" width=10cm
 *
 * @see xtsc_lookup_driver_parms
 * @see xtsc::xtsc_lookup_if
 */
class XTSC_COMP_API xtsc_lookup_driver : public sc_core::sc_module, public xtsc::xtsc_module, virtual public xtsc::xtsc_lookup_if {
public:

  sc_core::sc_out<sc_dt::sc_bv_base>            m_address;      ///<  pin-level address to lookup
  sc_core::sc_out<sc_dt::sc_bv_base>            m_req;          ///<  pin-level request to lookup
  sc_core::sc_in<sc_dt::sc_bv_base>             m_data;         ///<  pin-level data signal from lookup
  sc_core::sc_in<sc_dt::sc_bv_base>             m_ready;        ///<  pin-level ready signal from lookup

  sc_core::sc_port<xtsc::xtsc_lookup_if>        m_lookup;       ///<  TLM port to the lookup

  // For SystemC
  SC_HAS_PROCESS(xtsc_lookup_driver);


  /// Our C++ type (SystemC uses this)
  virtual const char* kind() const { return "xtsc_lookup_driver"; }


  /**
   * Constructor for an xtsc_lookup_driver.
   * @param     module_name     Name of the xtsc_lookup_driver sc_module.
   * @param     driver_parms    The remaining parameters for construction.
   * @see xtsc_lookup_driver_parms
   */
  xtsc_lookup_driver(sc_core::sc_module_name module_name, const xtsc_lookup_driver_parms& driver_parms);


  // Destructor.
  ~xtsc_lookup_driver(void);


  /// For xtsc_connection_interface
  virtual xtsc::u32 get_bit_width(const std::string& port_name, xtsc::u32 interface_num = 0) const;


  /// For xtsc_connection_interface
  virtual sc_core::sc_object *get_port(const std::string& port_name);


  /// For xtsc_connection_interface
  virtual std::string get_default_port_name() const { return "lookup"; }


  /// For xtsc_connection_interface
  virtual xtsc::xtsc_port_table get_port_table(const std::string& port_table_name) const;


#ifndef DOXYGEN_SKIP
  /**
   * Connect this xtsc_lookup_driver to a lookup of an xtsc_interrupt_distributor.
   *
   * This method connects the TLM port of this xtsc_lookup_driver to the specified
   * lookup export of the specified xtsc_interrupt_distributor.  This method should not
   * be used when "pin_level" is true.
   *
   * @param     distributor     The xtsc_interrupt_distributor that this
   *                            xtsc_lookup_driver is to be connected to.
   *
   * @param     lookup_name     The lookup sc_export of the xtsc_interrupt_distributor
   *                            that this xtsc_lookup_driver is to be connected to.
   */
  void connect(xtsc::xtsc_interrupt_distributor& distributor, const char *lookup_name);
#endif // DOXYGEN_SKIP


  /**
   * Reset the xtsc_lookup_driver.
   */
  void reset(bool hard_reset = false);


  /// Thread to process commands from m_script_file
  void script_thread(void);


  /// Thread to sample the m_ready signal to see if lookup succeeded
  void sample_phase_thread(void);


  /// Thread to sample the m_data signal 
  void sample_data_thread(void);


  /// Thread to drive the lookup request 
  void request_thread(void);


  /// Get the TextLogger for this component (e.g. to adjust its log level)
  log4xtensa::TextLogger& get_text_logger() { return m_text; }


protected:

  log4xtensa::TextLogger&               m_text;                         ///<  For logging
  bool                                  m_has_ready;                    ///<  From "has_ready" paramter
  xtsc::xtsc_script_file                m_test_vector_stream;           ///<  Script file      from "script_file" parameter
  std::string                           m_script_file;                  ///<  Script file name from "script_file" parameter
  std::string                           m_line;                         ///<  Current line        from "script_file"
  xtsc::u32                             m_line_count;                   ///<  Current line number from "script_file"
  std::vector<std::string>              m_words;                        ///<  Current line        from "script_file" tokenized into words
  sc_core::sc_time                      m_latency;                      ///<  From "latency" parameter
  xtsc::u64                             m_clock_period_value;           ///<  This device's clock period as u64
  sc_core::sc_time                      m_clock_period;                 ///<  From "clock_period" parameter
  sc_core::sc_time                      m_poll_ready_delay;             ///<  From "poll_ready_delay" parameter
  sc_core::sc_time                      m_delay_after_ready;            ///<  m_clock_period - m_poll_ready_delay
  sc_core::sc_time                      m_notify_delay;                 ///<  m_latency - m_poll_ready_delay
  sc_core::sc_time                      m_time_resolution;              ///<  The SystemC time resolution
  bool                                  m_has_posedge_offset;           ///<  True if m_posedge_offset is non-zero
  sc_core::sc_time                      m_posedge_offset;               ///<  From "posedge_offset" parameter
  xtsc::u64                             m_posedge_offset_value;         ///<  m_posedge_offset as u64
  sc_core::sc_time                      m_sample_phase;                 ///<  From "sample_phase" parameter
  sc_core::sc_time                      m_deassert_delay;               ///<  From "deassert_delay" parameter
  sc_core::sc_time                      m_timeout;                      ///<  From <timeout> in current line from "script_file"
  bool                                  m_no_timeout;                   ///<  True if <timeout> was not specified in "script_file" for this lookup
  xtsc::u32                             m_address_width1;               ///<  From "address_bit_width" parameter
  xtsc::u32                             m_data_width1;                  ///<  From "data_bit_width" parameter
  sc_dt::sc_unsigned                    m_lookup_address;               ///<  From <address> in current line from "script_file"
  sc_dt::sc_bv_base                     m_lookup_address_bv;            ///<  From <address> in current line from "script_file"
  bool                                  m_pin_level;                    ///<  From "pin_level" parameter
  sc_core::sc_trace_file               *m_p_trace_file;                 ///<  From "vcd_handle" parameter
  sc_dt::sc_unsigned                    m_zero;                         ///<  Constant 0
  sc_dt::sc_unsigned                    m_one;                          ///<  Constant 1
  sc_dt::sc_bv_base                     m_zero_bv;                      ///<  Constant 0
  sc_dt::sc_bv_base                     m_one_bv;                       ///<  Constant 1
  sc_core::sc_event                     m_next_request;                 ///<  To notify script_thread to continue
  sc_core::sc_event                     m_assert;                       ///<  To notify request_thread to   assert lookup request
  sc_core::sc_event                     m_deassert;                     ///<  To notify request_thread to deassert lookup request
  sc_core::sc_event_queue               m_sample_data;                  ///<  To notify sample_data_thread to sample the data

  std::vector<sc_core::sc_process_handle>
                                        m_process_handles;              ///<  For reset 


  /// Extract a u32 value (named argument_name) from the word at m_words[index]
  xtsc::u32 get_u32(xtsc::u32 index, const std::string& argument_name);

  /// Extract a double value (named argument_name) from the word at m_words[index]
  double get_double(xtsc::u32 index, const std::string& argument_name);

  /// Method to check interface width 
  void end_of_elaboration();

  // These 5 methods are to cap off the TLM port (m_lookup) when this driver is 
  // connected at the pin-level to, for example, xtsc_lookup_pin.  These
  // methods will never be called
  void nb_send_address(const sc_dt::sc_unsigned& address)       { return; }
  bool nb_is_ready()                                            { return false; }
  sc_dt::sc_unsigned nb_get_data()                              { return m_zero; }
  xtsc::u32 nb_get_address_bit_width()                          { return m_address_width1; }
  xtsc::u32 nb_get_data_bit_width()                             { return m_data_width1; }



  // This 4 signals are to cap off the pin-level ports when this driver is 
  // connected at the TLM level to, for example, xtsc_lookup.
  xtsc::xtsc_signal_sc_bv_base_floating m_address_floating;
  xtsc::xtsc_signal_sc_bv_base_floating m_req_floating;
  xtsc::xtsc_signal_sc_bv_base_floating m_data_floating;
  xtsc::xtsc_signal_sc_bv_base_floating m_ready_floating;

};



}  // namespace xtsc_component



#endif  // _XTSC_LOOKUP_DRIVER_H_
