#ifndef _XTSC_EXT_REGFILE_PIN_H_
#define _XTSC_EXT_REGFILE_PIN_H_

// Copyright (c) 2006-2020 by Tensilica Inc.  ALL RIGHTS RESERVED.
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
 * Constructor parameters for a xtsc_ext_regfile_pin object.
 *
 * This class contains the constructor parameters for a xtsc_ext_regfile_pin object.
 *  \verbatim
   Name                 Type    Description
   ------------------   ----    -------------------------------------------------------
  
   "address_bit_width"  u32     The width of the request address in bits.

   "data_bit_width"     u32     The width of the response data in bits.

   "clock_period"       u32     This is the length of this ext_regfile's clock period
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
 * @see xtsc_ext_regfile_pin
 * @see xtsc::xtsc_parms
 * @see xtsc::xtsc_initialize_parms
 */
class XTSC_COMP_API xtsc_ext_regfile_pin_parms : public xtsc::xtsc_parms {
public:

  /**
   * Constructor for an xtsc_ext_regfile_pin_parms object.
   *
   * @param     address_bit_width       The width of the request address in bits.
   *
   * @param     data_bit_width          The width of the response data in bits.
   *
   * @param     p_trace_file            Pointer to SystemC VCD object or 0 if tracing
   *                                    is not desired.
   *
   */
  xtsc_ext_regfile_pin_parms(xtsc::u32               address_bit_width= 0,
                        xtsc::u32               data_bit_width   = 0,
                        sc_core::sc_trace_file *p_trace_file     = 0)
  {
    add("address_bit_width",    address_bit_width);
    add("data_bit_width",       data_bit_width);
    add("clock_period",         0xFFFFFFFF);
    add("posedge_offset",       0xFFFFFFFF);
    add("sample_phase",         0);
    add("vcd_handle",           (void*) p_trace_file); 
  }


  /// Return what kind of xtsc_parms this is (our C++ type)
  virtual const char* kind() const { return "xtsc_ext_regfile_pin_parms"; }

};





/**
 * A TIE ext_regfile implementation using the pin-level interface.
 *
 * Example XTSC ext_regfile implementation that connects at the pin-level.
 *
 * Here is a block diagram of an xtsc_ext_regfile_pin as it is used in the xtsc_ext_regfile_pin
 * example:
 * @image html  Example_xtsc_ext_regfile_pin.jpg
 * @image latex Example_xtsc_ext_regfile_pin.eps "xtsc_ext_regfile_pin Example" width=10cm
 *
 * @see xtsc_ext_regfile_pin_parms
 */
class XTSC_COMP_API xtsc_ext_regfile_pin : public sc_core::sc_module, public xtsc::xtsc_module  {
public:


  sc_core::sc_in<sc_dt::sc_bv_base>     m_wr;                   ///<  TIE_xxx_Write
  sc_core::sc_in<sc_dt::sc_bv_base>     m_wr_address;           ///<  TIE_xxx_WriteAddr
  sc_core::sc_in<sc_dt::sc_bv_base>     m_wr_data;              ///<  TIE_xxx_WriteData

  sc_core::sc_in<sc_dt::sc_bv_base>     m_rd_check;             ///<  TIE_xxx_ReadCheck
  sc_core::sc_in<sc_dt::sc_bv_base>     m_rd_check_addr;        ///<  TIE_xxx_ReadCheckAddr
  sc_core::sc_out<sc_dt::sc_bv_base>    m_rd_stall;             ///<  TIE_xxx_ReadStall

  sc_core::sc_in<sc_dt::sc_bv_base>     m_rd;                   ///<  TIE_xxx_Read 
  sc_core::sc_in<sc_dt::sc_bv_base>     m_rd_address;           ///<  TIE_xxx_ReadAddr
  sc_core::sc_out<sc_dt::sc_bv_base>    m_rd_data;              ///<  TIE_xxx_ReadData

/*
  sc_core::sc_in<sc_dt::sc_bv_base>     m_address;              ///<  Address from client (TIE_xxx_Out)
  sc_core::sc_in<sc_dt::sc_bv_base>     m_req;                  ///<  Lookup request  (TIE_xxx_Out_Req)

  sc_core::sc_out<sc_dt::sc_bv_base>    m_data;                 ///<  Value to client (TIE_xxx_In)
  sc_core::sc_out<sc_dt::sc_bv_base>    m_ready;                ///<  Ready to client (TIE_xxx_Rdy).  Optional.
*/

  // For SystemC
  SC_HAS_PROCESS(xtsc_ext_regfile_pin);


  /// Our C++ type (SystemC uses this)
  virtual const char* kind() const { return "xtsc_ext_regfile_pin"; }


  /**
   * Constructor for an xtsc_ext_regfile_pin.
   *
   * @param     module_name     Name of the xtsc_ext_regfile_pin sc_module.
   *
   * @param     ext_regfile_parms    The remaining parameters for construction.
   *
   * @see xtsc_ext_regfile_pin_parms
   */
  xtsc_ext_regfile_pin(sc_core::sc_module_name module_name, const xtsc_ext_regfile_pin_parms& ext_regfile_parms);


  // Destructor.
  ~xtsc_ext_regfile_pin(void);


  /// For xtsc_connection_interface
  virtual xtsc::u32 get_bit_width(const std::string& port_name, xtsc::u32 interface_num = 0) const;


  /// For xtsc_connection_interface
  virtual sc_core::sc_object *get_port(const std::string& port_name);


  /// For xtsc_connection_interface
  virtual std::string get_default_port_name() const { return "ext_regfile"; }


  /// For xtsc_connection_interface
  virtual xtsc::xtsc_port_table get_port_table(const std::string& port_table_name) const;


  /**
   * Reset the xtsc_ext_regfile_pin.
   */
  void reset(bool hard_reset = false);


  /// Get the TextLogger for this component (e.g. to adjust its log level)
  log4xtensa::TextLogger& get_text_logger() { return m_text; }


  /// Get the BinaryLogger for this component (e.g. to adjust its log level)
  log4xtensa::BinaryLogger& get_binary_logger() { return m_binary; }


protected:

  void before_end_of_elaboration();
  void write_thread();
  void read_check_thread();
  void read_thread();

  sc_core::sc_time                      m_time_resolution;      ///<  The SystemC time resolution
  sc_core::sc_time                      m_clock_period;         ///<  The ext_regfile's clock period (from "clock_period" parameter)
  xtsc::u64                             m_clock_period_value;   ///<  The ext_regfile's clock period expressed as u64 value
  bool                                  m_has_posedge_offset;   ///<  True if m_posedge_offset is non-zero
  sc_core::sc_time                      m_posedge_offset;       ///<  From "posedge_offset" parameter
  xtsc::u64                             m_posedge_offset_value; ///<  m_posedge_offset as u64
  sc_core::sc_time                      m_sample_phase;         ///<  Phase of clock when m_req is sampled (0 => posedge clock) (from "sample_phase")
  xtsc::u64                             m_sample_phase_value;   ///<  Phase of clock when m_req is sampled expressed as u64 value
  sc_core::sc_time                      m_latency;              ///<  From xtsc_ext_regfile_pin_parm "latency"
  xtsc::u32                             m_delay;                ///<  Default delay from xtsc_ext_regfile_pin_parm "delay"
  sc_dt::sc_bv_base                     m_zero;                 ///<  Constant 0
  sc_dt::sc_bv_base                     m_one;                  ///<  Constant 1
  sc_core::sc_time                      m_delay_timeout;        ///<  Time for the ready delay to expire

  sc_core::sc_event                     m_ready_event;          ///<  Pipeline full/not-full, or delay 
  sc_core::sc_event                     m_data_event;           ///<  When to drive the next data
  sc_core::sc_event                     m_timeout_event;        ///<  Internal state machine state timeouts


  xtsc::u32                             m_address_bit_width;    ///<  From "address_bit_width" parameter
  xtsc::u32                             m_data_bit_width;       ///<  From "data_bit_width" parameter
  sc_dt::sc_unsigned                    m_next_address;         ///<  Keep track of implied next address to use if not explicit in ext_regfile table file
  sc_dt::sc_bv_base                     m_default_data;         ///<  From "default_data" parameter
  sc_dt::sc_bv_base                     m_data_registered;      ///<  The registered ext_regfile data being driven out
  std::string                           m_ext_regfile_table;         ///<  The ext_regfile table file name (from "ext_regfile_table" parameter)
  xtsc::xtsc_script_file               *m_file;                 ///<  The ext_regfile table file
  std::string                           m_line;                 ///<  Current line in the ext_regfile table
  xtsc::u32                             m_line_count;           ///<  Current line number in the ext_regfile table as it is being parsed
  std::vector<std::string>              m_words;                ///<  Current line in ext_regfile table tokenized into words

  std::map<xtsc::u32, sc_dt::sc_bv_base>
                                        m_data_map;             ///<  The ext_regfile table
  std::map<xtsc::u32, sc_core::sc_time> 
                                        m_op_fini_time;         ///<  Time at which operation finishes

  log4xtensa::TextLogger&               m_text;                 ///<  TextLogger
  log4xtensa::BinaryLogger&             m_binary;               ///<  BinaryLogger

  sc_core::sc_trace_file               *m_p_trace_file;         ///<  VCD trace file (see "vcd_handle" parameter)

  std::vector<sc_core::sc_process_handle>
                                        m_process_handles;      ///<  For reset 

  /// Convert m_words[index] to sc_bv_base value
  void get_sc_bv_base(xtsc::u32 index, sc_dt::sc_bv_base& value);
};



}  // namespace xtsc_component




#endif  // _XTSC_EXT_REGFILE_PIN_H_
