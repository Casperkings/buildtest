#ifndef _XTSC_LOOKUP_PIN_H_
#define _XTSC_LOOKUP_PIN_H_

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
#include <string>
#include <vector>
#include <map>




namespace xtsc_component {



/**
 * Constructor parameters for a xtsc_lookup_pin object.
 *
 * This class contains the constructor parameters for a xtsc_lookup_pin object.
 *  \verbatim
   Name                 Type    Description
   ------------------   ----    -------------------------------------------------------
  
   "address_bit_width"  u32     The width of the request address in bits.  Maximum is
                                1024.

   "data_bit_width"     u32     The width of the response data in bits.  Maximum is
                                1024.

   "has_ready"          bool    If true, the xtsc_lookup_pin device will drive the
                                ready output port (xtsc_lookup_pin::m_ready) for 
                                connecting to the "TIE_xxx_Rdy" TIE input port.  If
                                false, the xtsc_lookup_pin device will cap the m_ready
                                port and the user must not connect to it.

   "pipeline_depth"     u32     This parameter specifies the depth of the pipeline.
                                When the pipeline is full and "has_ready" is true,
                                the m_ready signal will be de-asserted.  If "has_ready"
                                is false and a lookup is requested when the pipeline
                                is full, an xtsc_exception will be thrown.  A value of
                                0 means to be fully pipelined (i.e. have a pipeline
                                depth equal to the latency).
                                Default = 0 (i.e. fully pipelined).

   "latency"            u32     The latency is defined by the <input_stage> and
                                <output_stage> values in the user TIE code defining the
                                TIE lookup port according to the following formula:
                                  latency = <input_stage> - <output_stage>
                                If "has_ready" is true, "latency" specfies how many 
                                clock cycles after m_ready (TIE_xxx_Rdy) is asserted
                                that m_data (TIE_xxx_In) is valid.
                                If "has_ready" is false, "latency" specfies how many 
                                clock cycles after m_req (TIE_xxx_Out_Req) is sampled
                                and found to be asserted that m_data (TIE_xxx_In) is
                                valid.
                                "latency" must be greater than 0.
                                Default = 1.

   "delay"              u32     If "has_ready" is true, "delay" gives the default value
                                for how many clock cycles after a lookup request is
                                accepted before another lookup request will be accepted
                                (provided the pipeline is not full).  A value of 0 means
                                the default is to not delay.  The default delay may be
                                overridden on a per lookup address basis by specifiying
                                a delay value on the "lookup_table" line defining the
                                lookup value for the lookup address in question.  The
                                purpose of this parameter is to allow a limited ability
                                to model the lookup device being busy (for example,
                                because it is shared with another Xtensa or because it
                                is having its values reloaded).  If "has_ready" is
                                false, "delay" is ignored.
                                Default = 0.

   Note:  The interpretation of the "delay" parameter changed after (not including) RC-2010.1
          to better model proper TIE lookup interface protocol.

   "lookup_table"       char*   The name of the file containing the address-data pairs
                                and/or just the data (with an implied address).  Each
                                address-data pair can also have an optional delay
                                specified.  If this parameter is NULL, then all lookups
                                will return the "default_data".  This file must contain
                                lines in the following format:

                                [<Address>] <Value> [@<Delay>]
                                
                                1.  Each line of the text file contains one or two
                                    numbers in decimal or hexadecimal (using '0x'
                                    prefix) format followed by an optional delay
                                    value which must start with an @.
                                    For example,

                                       0x12345678        // Implied address of 0x0
                                       0xbabeface        // Implied address of 0x1
                                       0x99  0x11111111  // Explicit address of 0x99
                                       16    0x11111111  // Explicit address of 0x10
                                       0x22222222        // Implied address of 0x11
                                       18 0x33333333 @3  // Next m_ready is delayed by 3
                             
                                2.  If a line contains two numbers (excluding the
                                    optional delay), the first number is interpreted as
                                    an address and the second number is interpreted as
                                    the data corresponding to the address.
                                3.  If a line only contains one number (excluding the
                                    optional delay),then it is interpreted as the data
                                    corresponding to an address which is one greater
                                    than the address of the previous line.  If the first
                                    line of the file contains one number, then the
                                    implied address is 0.
                                4.  Numbers can contain up to 1024 bits (256 hex 
                                    nibbles).
                                5.  An address can occur at most once in the file
                                    (either explicitly or implied).  A data value can
                                    occur as often as desired.
                                6.  An optional integer delay value can be specified to
                                    override the "delay" parameter for this lookup
                                    address only.  A delay specified here means that
                                    following any lookup to this address, the lookup
                                    device will not be available for a subsequent lookup
                                    for the specified number of clock cycles.  The
                                    @<Delay> entries only take effect if "has_ready" is
                                    true.
                                7.  Comments, extra white space, and blank lines are
                                    ignored.  See xtsc::xtsc_script_file.

   "default_data"       char*   C-string containing the default data to be returned for
                                any address not specified (explicitly or implicitly) in 
                                "lookup_table".
                                Default = "0x0".

   "clock_period"       u32     This is the length of this lookup's clock period
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

   "sample_phase"       u32     This specifies the phase (i.e. the point) in each clock
                                period at which the m_req signal is sampled.  It is
                                expressed in terms of the SystemC time resolution (from
                                sc_get_time_resolution()) and must be strictly less than
                                the clock period as specified by the "clock_period"
                                parameter.  A value of 0 means the m_req signal is
                                sampled on posedge clock as specified by 
                                "posedge_offset".
                                Default = 0.

   "vcd_handle"         void*   Pointer to SystemC VCD object (sc_trace_file *) or
                                0 if tracing is not desired.

    \endverbatim
 *
 * @see xtsc_lookup_pin
 * @see xtsc::xtsc_parms
 * @see xtsc::xtsc_initialize_parms
 */
class XTSC_COMP_API xtsc_lookup_pin_parms : public xtsc::xtsc_parms {
public:

  /**
   * Constructor for an xtsc_lookup_pin_parms object.
   *
   * @param     address_bit_width       The width of the request address in bits.
   *
   * @param     data_bit_width          The width of the response data in bits.
   *
   * @param     has_ready               If true, xtsc_lookup_pin will drive the m_ready
   *                                    signal.  If false, xtsc_lookup_pin will 
   *                                    internally cap the m_ready signal and the user
   *                                    must not connect to it.
   *
   * @param     lookup_table            The name of the file containing the address-data
   *                                    pairs and/or just the data (with an implied
   *                                    address).
   *
   * @param     default_data            The data to use for addresses which aren't in
   *                                    lookup_table.
   *
   * @param     p_trace_file            Pointer to SystemC VCD object or 0 if tracing
   *                                    is not desired.
   *
   */
  xtsc_lookup_pin_parms(xtsc::u32               address_bit_width= 0,
                        xtsc::u32               data_bit_width   = 0,
                        bool                    has_ready        = false,
                        const char             *lookup_table     = NULL,
                        const char             *default_data     = "0x0",
                        sc_core::sc_trace_file *p_trace_file     = 0)
  {
    add("address_bit_width",    address_bit_width);
    add("data_bit_width",       data_bit_width);
    add("has_ready",            has_ready);
    add("pipeline_depth",       0);
    add("latency",              1);
    add("delay",                0);
    add("lookup_table",         lookup_table);
    add("default_data",         default_data);
    add("clock_period",         0xFFFFFFFF);
    add("posedge_offset",       0xFFFFFFFF);
    add("sample_phase",         0);
    add("vcd_handle",           (void*) p_trace_file); 
  }


  /// Return what kind of xtsc_parms this is (our C++ type)
  virtual const char* kind() const { return "xtsc_lookup_pin_parms"; }

};





/**
 * A TIE lookup implementation using the pin-level interface.
 *
 * Example XTSC lookup implementation that connects at the pin-level and that uses a ROM
 * like lookup table which is initialized from a file.
 *
 * This module can also be used to model a non-ROM lookup (for example, a computed
 * lookup) as long as all possible lookups can be enumerated individually without
 * exceeding the host memory system.   When modeling a lookup device with a ready
 * signal, each possible lookup address can have a custom delay associated with it that
 * specifies how long the TIE_xxx_Rdy signal is deasserted after a lookup to that
 * particular address (so the delay potentially affects the next lookup, not the current
 * lookup).  Note:  This is a change from the RC-2010.1 and earlier behavior.
 *
 * Here is a block diagram of an xtsc_lookup_pin as it is used in the xtsc_lookup_pin
 * example:
 * @image html  Example_xtsc_lookup_pin.jpg
 * @image latex Example_xtsc_lookup_pin.eps "xtsc_lookup_pin Example" width=10cm
 *
 * @see xtsc_lookup_pin_parms
 */
class XTSC_COMP_API xtsc_lookup_pin : public sc_core::sc_module, public xtsc::xtsc_module  {
public:


  sc_core::sc_in<sc_dt::sc_bv_base>     m_address;              ///<  Address from client (TIE_xxx_Out)
  sc_core::sc_in<sc_dt::sc_bv_base>     m_req;                  ///<  Lookup request  (TIE_xxx_Out_Req)

  sc_core::sc_out<sc_dt::sc_bv_base>    m_data;                 ///<  Value to client (TIE_xxx_In)
  sc_core::sc_out<sc_dt::sc_bv_base>    m_ready;                ///<  Ready to client (TIE_xxx_Rdy).  Optional.

  xtsc::xtsc_signal_sc_bv_base_floating m_ready_floating;       ///<  Bind to m_ready when there is no TIE_xxx_Rdy


  // For SystemC
  SC_HAS_PROCESS(xtsc_lookup_pin);


  /// Our C++ type (SystemC uses this)
  virtual const char* kind() const { return "xtsc_lookup_pin"; }


  /**
   * Constructor for an xtsc_lookup_pin.
   *
   * @param     module_name     Name of the xtsc_lookup_pin sc_module.
   *
   * @param     lookup_parms    The remaining parameters for construction.
   *
   * @see xtsc_lookup_pin_parms
   */
  xtsc_lookup_pin(sc_core::sc_module_name module_name, const xtsc_lookup_pin_parms& lookup_parms);


  // Destructor.
  ~xtsc_lookup_pin(void);


  /// For xtsc_connection_interface
  virtual xtsc::u32 get_bit_width(const std::string& port_name, xtsc::u32 interface_num = 0) const;


  /// For xtsc_connection_interface
  virtual sc_core::sc_object *get_port(const std::string& port_name);


  /// For xtsc_connection_interface
  virtual std::string get_default_port_name() const { return "lookup"; }


  /// For xtsc_connection_interface
  virtual xtsc::xtsc_port_table get_port_table(const std::string& port_table_name) const;


  /**
   * Reset the xtsc_lookup_pin.
   */
  void reset(bool hard_reset = false);


  /// Get the TextLogger for this component (e.g. to adjust its log level)
  log4xtensa::TextLogger& get_text_logger() { return m_text; }


  /// Get the BinaryLogger for this component (e.g. to adjust its log level)
  log4xtensa::BinaryLogger& get_binary_logger() { return m_binary; }


protected:

  void before_end_of_elaboration();
  void request_thread();
  void ready_thread();
  void data_thread();

  bool                                  m_has_ready;            ///<  True if lookup has a rdy signal (from "has_ready" parameter)
  sc_core::sc_time                      m_time_resolution;      ///<  The SystemC time resolution
  sc_core::sc_time                      m_clock_period;         ///<  The lookup's clock period (from "clock_period" parameter)
  xtsc::u64                             m_clock_period_value;   ///<  The lookup's clock period expressed as u64 value
  bool                                  m_has_posedge_offset;   ///<  True if m_posedge_offset is non-zero
  sc_core::sc_time                      m_posedge_offset;       ///<  From "posedge_offset" parameter
  xtsc::u64                             m_posedge_offset_value; ///<  m_posedge_offset as u64
  sc_core::sc_time                      m_sample_phase;         ///<  Phase of clock when m_req is sampled (0 => posedge clock) (from "sample_phase")
  xtsc::u64                             m_sample_phase_value;   ///<  Phase of clock when m_req is sampled expressed as u64 value
  sc_core::sc_time                      m_latency;              ///<  From xtsc_lookup_pin_parm "latency"
  xtsc::u32                             m_delay;                ///<  Default delay from xtsc_lookup_pin_parm "delay"
  sc_dt::sc_bv_base                     m_zero;                 ///<  Constant 0
  sc_dt::sc_bv_base                     m_one;                  ///<  Constant 1
  sc_core::sc_time                      m_delay_timeout;        ///<  Time for the ready delay to expire

  sc_core::sc_event                     m_ready_event;          ///<  Pipeline full/not-full, or delay 
  sc_core::sc_event                     m_data_event;           ///<  When to drive the next data
  sc_core::sc_event                     m_timeout_event;        ///<  Internal state machine state timeouts


  xtsc::u32                             m_address_bit_width;    ///<  From "address_bit_width" parameter
  xtsc::u32                             m_data_bit_width;       ///<  From "data_bit_width" parameter
  sc_dt::sc_unsigned                    m_next_address;         ///<  Keep track of implied next address to use if not explicit in lookup table file
  sc_dt::sc_bv_base                     m_default_data;         ///<  From "default_data" parameter
  sc_dt::sc_bv_base                     m_data_registered;      ///<  The registered lookup data being driven out
  std::string                           m_lookup_table;         ///<  The lookup table file name (from "lookup_table" parameter)
  xtsc::xtsc_script_file               *m_file;                 ///<  The lookup table file
  std::string                           m_line;                 ///<  Current line in the lookup table
  xtsc::u32                             m_line_count;           ///<  Current line number in the lookup table as it is being parsed
  std::vector<std::string>              m_words;                ///<  Current line in lookup table tokenized into words
  std::map<std::string, sc_dt::sc_bv_base*>
                                        m_data_map;             ///<  The address-to-data (key-to-value) map of lookup data
  std::map<std::string, xtsc::u32>      m_delay_map;            ///<  The address-to-delay map of lookup data

  log4xtensa::TextLogger&               m_text;                 ///<  TextLogger
  log4xtensa::BinaryLogger&             m_binary;               ///<  BinaryLogger

  sc_core::sc_trace_file               *m_p_trace_file;         ///<  VCD trace file (see "vcd_handle" parameter)

  xtsc::u32                             m_pipeline_depth;       ///<  Number of slots in m_pipeline_data from "pipeline_depth" or "latency"
  xtsc::u32                             m_pipeline_wp;          ///<  Write pointer into m_pipeline_data fifo
  xtsc::u32                             m_pipeline_rp;          ///<  Read  pointer into m_pipeline_data fifo
  sc_dt::sc_bv_base                   **m_pipeline_data;        ///<  The fifo of pipeline data
  sc_core::sc_time                     *m_pipeline_times;       ///<  The fifo of due times (when to drive the data)

  std::vector<sc_core::sc_process_handle>
                                        m_process_handles;      ///<  For reset 

  /// Convert m_words[index] to sc_bv_base value
  void get_sc_bv_base(xtsc::u32 index, sc_dt::sc_bv_base& value);

  /// Return true if pipeline is full
  bool pipeline_full();

  /// Do the lookup and log it
  void do_lookup(const sc_dt::sc_bv_base& address, const sc_core::sc_time& when);

};



}  // namespace xtsc_component




#endif  // _XTSC_LOOKUP_PIN_H_
