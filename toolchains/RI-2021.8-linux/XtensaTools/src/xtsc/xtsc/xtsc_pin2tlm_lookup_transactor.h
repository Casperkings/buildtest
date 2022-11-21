#ifndef _XTSC_PIN2TLM_LOOKUP_TRANSACTOR_H_
#define _XTSC_PIN2TLM_LOOKUP_TRANSACTOR_H_

// Copyright (c) 2006-2014 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
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
#include <xtsc/xtsc_lookup_if.h>
#include <string>
#include <vector>
#include <fstream>


namespace xtsc_component {


/**
 * Constructor parameters for a xtsc_pin2tlm_lookup_transactor object.
 *
 * This class contains the constructor parameters for a xtsc_pin2tlm_lookup_transactor object.
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

   "vcd_handle"         void*   Pointer to SystemC VCD object (sc_trace_file *) or 0 if
                                tracing is not desired.
                                Default = 0 (NULL).

   "clock_period"       u32     This is the length of this device's clock period
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

   "sample_phase"       u32     This parameter specifies the point in each clock period
                                at which the m_req and m_address signals are sampled.
                                This parameter is expressed in terms of the SystemC time
                                resolution (from sc_get_time_resolution()) and must be
                                strictly less than the clock period as specified by the
                                "clock_period" parameter.  A value of 0 means sampling
                                occurs at posedge clock as specfied by "posedge_offset".
                                The special value of 0xFFFFFFFF means to sample at 1
                                SystemC time resolution prior to posedge clock.
                                Default = 0xFFFFFFFF.

    \endverbatim
 *
 * @see xtsc_pin2tlm_lookup_transactor
 * @see xtsc::xtsc_parms
 */
class XTSC_COMP_API xtsc_pin2tlm_lookup_transactor_parms : public xtsc::xtsc_parms {
public:

  /**
   * Constructor for an xtsc_pin2tlm_lookup_transactor_parms object.
   *
   * @param     address_bit_width       Width of request address in bits.
   *
   * @param     data_bit_width          Width of response data in bits.
   *
   * @param     has_ready               Specifies whether or not the lookup device has
   *                                    a ready signal (corresponds to the rdy keyword
   *                                    in the user's TIE code for the lookup).
   *
   */
  xtsc_pin2tlm_lookup_transactor_parms(xtsc::u32 address_bit_width = 1, xtsc::u32 data_bit_width = 1, bool has_ready = false) {
    add("address_bit_width",    address_bit_width);
    add("data_bit_width",       data_bit_width);
    add("has_ready",            has_ready);
    add("latency",              1);
    add("vcd_handle",           (void*) NULL); 
    add("clock_period",         0xFFFFFFFF);
    add("posedge_offset",       0xFFFFFFFF);
    add("sample_phase",         0xFFFFFFFF);
  }


  /// Return what kind of xtsc_parms this is (our C++ type)
  virtual const char* kind() const { return "xtsc_pin2tlm_lookup_transactor_parms"; }

};





/**
 * A transactor to convert a pin-level TIE lookup interface to Xtensa TLM.
 *
 * This transactor converts a pin-level TIE lookup interface to xtsc_lookup_if, the
 * equivalent Xtensa TLM interface.
 *
 * This transactor supports a TIE lookup interface that has a ready signal; however, the
 * downstream Xtensa TLM lookup must always return true to the nb_is_ready() call.
 *
 * @see xtsc_pin2tlm_lookup_transactor_parms
 * @see xtsc::xtsc_lookup_if
 */
class XTSC_COMP_API xtsc_pin2tlm_lookup_transactor : public sc_core::sc_module, public xtsc::xtsc_module {
public:

  sc_core::sc_in <sc_dt::sc_bv_base>            m_address;      ///<  pin-level address to lookup
  sc_core::sc_in <sc_dt::sc_bv_base>            m_req;          ///<  pin-level request to lookup
  sc_core::sc_out<sc_dt::sc_bv_base>            m_data;         ///<  pin-level data signal from lookup
  sc_core::sc_out<sc_dt::sc_bv_base>            m_ready;        ///<  pin-level ready signal from lookup

  sc_core::sc_port<xtsc::xtsc_lookup_if>        m_lookup;       ///<  TLM port to the lookup

  // For SystemC
  SC_HAS_PROCESS(xtsc_pin2tlm_lookup_transactor);


  /// Our C++ type (SystemC uses this)
  virtual const char* kind() const { return "xtsc_pin2tlm_lookup_transactor"; }


  /**
   * Constructor for an xtsc_pin2tlm_lookup_transactor.
   * @param     module_name     Name of the xtsc_pin2tlm_lookup_transactor sc_module.
   * @param     parms           The remaining parameters for construction.
   * @see xtsc_pin2tlm_lookup_transactor_parms
   */
  xtsc_pin2tlm_lookup_transactor(sc_core::sc_module_name module_name, const xtsc_pin2tlm_lookup_transactor_parms& parms);


  // Destructor.
  ~xtsc_pin2tlm_lookup_transactor(void);


  /// For xtsc_connection_interface
  virtual xtsc::u32 get_bit_width(const std::string& port_name, xtsc::u32 interface_num = 0) const;


  /// For xtsc_connection_interface
  virtual sc_core::sc_object *get_port(const std::string& port_name);


  /// For xtsc_connection_interface
  virtual std::string get_default_port_name() const { return "lookup"; }


  /// For xtsc_connection_interface
  virtual xtsc::xtsc_port_table get_port_table(const std::string& port_table_name) const;


  /**
   * Reset the xtsc_pin2tlm_lookup_transactor.
   */
  void reset(bool hard_reset = false);


  /**
   * Request thread: samples m_req and m_address then calls nb_send_address() and nb_is_ready()
   * and then notifies drive_data_thread.
   */
  void request_thread(void);


  /**
   * Thread to wait the appropriate delay for timing and latency then call nb_get_data()
   * then drive m_data then wait 1 clock cycle and then deassert m_data.
   */
  void drive_data_thread(void);


  /// Get the TextLogger for this component (e.g. to adjust its log level)
  log4xtensa::TextLogger& get_text_logger() { return m_text; }


protected:

  log4xtensa::TextLogger&               m_text;                         ///<  For logging
  bool                                  m_has_ready;                    ///<  From "has_ready" parameter
  sc_core::sc_time                      m_latency;                      ///<  From "latency" parameter
  xtsc::u64                             m_clock_period_value;           ///<  This device's clock period as u64
  sc_core::sc_time                      m_clock_period;                 ///<  This device's clock period as sc_time
  sc_core::sc_time                      m_time_resolution;              ///<  The SystemC time resolution
  bool                                  m_has_posedge_offset;           ///<  True if m_posedge_offset is non-zero
  sc_core::sc_time                      m_posedge_offset;               ///<  From "posedge_offset" parameter
  xtsc::u64                             m_posedge_offset_value;         ///<  m_posedge_offset as u64
  sc_core::sc_time                      m_sample_phase;                 ///<  From "sample_phase" parameter
  xtsc::u32                             m_address_width1;               ///<  From "address_bit_width" parameter
  xtsc::u32                             m_data_width1;                  ///<  From "data_bit_width" parameter
  sc_core::sc_trace_file               *m_p_trace_file;                 ///<  If "vcd_handle" specified
  sc_dt::sc_bv_base                     m_zero_bv;                      ///<  To detect low signal
  sc_dt::sc_bv_base                     m_one_bv;                       ///<  To detect high signal
  sc_core::sc_event_queue               m_drive_data;                   ///<  For drive_data_thread

  std::vector<sc_core::sc_process_handle>
                                        m_process_handles;              ///<  For reset 


  /// Method to check interface width 
  void end_of_elaboration();

  xtsc::xtsc_signal_sc_bv_base_floating m_ready_floating;

};



}  // namespace xtsc_component



#endif  // _XTSC_PIN2TLM_LOOKUP_TRANSACTOR_H_
