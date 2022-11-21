#ifndef _XTSC_CLOCK_VP_H_
#define _XTSC_CLOCK_VP_H_

// Copyright (c) 2006-2011 by Tensilica Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Tensilica Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Tensilica Inc.

/**
 * @file 
 */

#include <xtsc_vp/xtsc_vp.h>



namespace xtsc_vp {


/**
 * Parameters for an xtsc_clock_vp.
 *
 * We use xtsc_parms to allow overriding at run time using the extract_parms mechanism.
 *
   "clock_period"       u32     This is the length of this clock's period expressed in
                                terms of the SystemC time resolution (from
                                sc_get_time_resolution()).  A value of 0xFFFFFFFF means
                                to use the XTSC system clock period (from
                                xtsc_get_system_clock_period()).
                                Default = 0xFFFFFFFF (i.e. use the system clock period).

   "enable_tracing"     bool    True if the clock signal is trace.  False if it is not
                                traced.
                                Default = true.

   "generator"          bool    By default, this module does not drive the m_clk output
                                signal.  If this parameter is set to true then this
                                module will operate as a clock generator and drive the
                                m_clk output signal.
                                Default = false.

   "high_duty_cycle"    double  The fraction of the period when the signal is high.
                                Default = 0.5 (50% duty cycle).

   "posedge_first"      bool    True if clock signal starts low and then goes high.
                                False if clock signal starts high and then goes low.
                                Default = true.

   "start_time"         u32     This is the time when the first clock edge occurs
                                expressed in terms of the SystemC time resolution (from
                                sc_get_time_resolution()).  
                                Default = 0.

   "vcd_name"           char*   The name of the VCD file to trace to.
 */
class XTSC_VP_API xtsc_clock_vp_parms : public xtsc::xtsc_parms {
public:

  xtsc_clock_vp_parms() {
    add("clock_period",         0xFFFFFFFF);
    add("enable_tracing",       true);
    add("generator",            false);
    add("high_duty_cycle",      0.5);
    add("posedge_first",        true);
    add("start_time",           0);
    add("vcd_name",             "");
  }

  virtual const char* kind() const { return "xtsc_clock_vp_parms"; }

};




/**
 * This module allows adding a clock signal to a VCD trace file and or generating a
 * clock signal.
 *
 * This module allows adding a clock signal to a VCD trace file but this can be turned
 * off by setting the "enable_tracing" parameter to false.  
 *
 * This module also has a m_clk signal output that can optionally be driven by setting
 * the "generator" parameter to true.
 *
 */
class XTSC_VP_API xtsc_clock_vp : public sc_module {
public:

  typedef std::ostringstream                    ostringstream;
  typedef xtsc::xtsc_exception                  xtsc_exception;
  typedef xtsc::u64                             u64;
  typedef xtsc::u32                             u32;
  typedef xtsc::u16                             u16;
  typedef xtsc::u8                              u8;

  sc_out<bool>                                  m_clk;

  SC_HAS_PROCESS(xtsc_clock_vp);

  xtsc_clock_vp(const sc_module_name &module_name) :
    sc_module                     (module_name),
    m_clk                         ("m_clk"),
    m_generator                   ("/misc/generator",           false),
    m_clock_period                ("/Timing/clock_period",      0xFFFFFFFF),
    m_high_duty_cycle             ("/Timing/high_duty_cycle",   0.5),
    m_start_time                  ("/Timing/start_time",        0),
    m_posedge_first               ("/Timing/posedge_first",     true),
    m_enable_tracing              ("/Trace/enable_tracing",     false),
    m_vcd_name                    ("/Trace/vcd_name",           ""),
    m_p_trace_file                (0),
    m_p_clock                     (0),
    m_text                        (log4xtensa::TextLogger::getInstance(name()))
  {
    xtsc_vp_initialize();

    xtsc_clock_vp_parms parms;
    parms.set("generator",       m_generator);
    parms.set("clock_period",    m_clock_period);
    parms.set("high_duty_cycle", m_high_duty_cycle);
    parms.set("start_time",      m_start_time);
    parms.set("posedge_first",   m_posedge_first);
    parms.set("enable_tracing",  m_enable_tracing);
    parms.set("vcd_name",        ((string)m_vcd_name).c_str());

    parms.extract_parms(sc_argc(), sc_argv(), name());

    m_generator         = parms.get_bool  ("generator");
    m_clock_period      = parms.get_u32   ("clock_period");
    m_high_duty_cycle   = parms.get_double("high_duty_cycle");
    m_start_time        = parms.get_u32   ("start_time");
    m_posedge_first     = parms.get_bool  ("posedge_first");
    m_enable_tracing    = parms.get_bool  ("enable_tracing");
    
    const char *vcd_name = parms.get_c_str("vcd_name");
    m_vcd_name = (vcd_name ? vcd_name : "");

    bool default_period = false;
    if (m_clock_period == 0xFFFFFFFF) {
      default_period = true;
      m_clock_period = (u32) (xtsc::xtsc_get_system_clock_period().value() & 0x0FFFFFFFFull);
    }

    sc_time period(m_clock_period*sc_get_time_resolution());

    if (m_enable_tracing || m_generator) {
      m_p_clock = new sc_clock("_", period, m_high_duty_cycle, m_start_time*sc_get_time_resolution(), m_posedge_first);

      if (m_enable_tracing) {
        m_p_trace_file = xtsc_vp_get_trace_file(m_vcd_name);
        sc_trace(m_p_trace_file, *m_p_clock, m_p_clock->name());
      }

      if (m_generator) {
        SC_METHOD(clock_method);
        sensitive << m_p_clock->value_changed_event();
      }
    }

    if (!m_generator) {
      XTSC_INFO(m_text, "\"generator\" is false, m_clk output will NOT be driven");
    }

    log4xtensa::LogLevel ll = xtsc::xtsc_get_constructor_log_level();
    XTSC_LOG(m_text, ll,        "Constructed xtsc_clock_vp '" << name() << "'" << (m_p_clock ? "" : " (Disabled)") << ":");
    if (default_period) {
    XTSC_LOG(m_text, ll,        " clock_period            = 0xFFFFFFFF (" << period << ")");
    } else {
    XTSC_LOG(m_text, ll,        " clock_period            = "   << m_clock_period << " (" << period << ")");
    }
    XTSC_LOG(m_text, ll,        " enable_tracing          = "   << std::boolalpha << m_enable_tracing);
    XTSC_LOG(m_text, ll,        " generator               = "   << std::boolalpha << m_generator);
    XTSC_LOG(m_text, ll,        " high_duty_cycle         = "   << m_high_duty_cycle);
    XTSC_LOG(m_text, ll,        " posedge_first           = "   << std::boolalpha << m_posedge_first);
    XTSC_LOG(m_text, ll,        " start_time              = "   << m_start_time << " (" << m_start_time*sc_get_time_resolution() << ")");
    XTSC_LOG(m_text, ll,        " vcd_name                = "   << m_vcd_name);
  }



  virtual ~xtsc_clock_vp() {
    xtsc_vp_finalize();
  }



private:

  void clock_method() {
    m_clk.write(m_p_clock->read());
  }



  scml_property<bool>                   m_generator;                    ///< See parameter "generator"
  scml_property<u32>                    m_clock_period;                 ///< See parameter "clock_period"
  scml_property<double>                 m_high_duty_cycle;              ///< See parameter "high_duty_cycle"
  scml_property<u32>                    m_start_time;                   ///< See parameter "start_time"
  scml_property<bool>                   m_posedge_first;                ///< See parameter "posedge_first"
  scml_property<bool>                   m_enable_tracing;               ///< See parameter "enable_tracing"
  scml_property<string>                 m_vcd_name;                     ///< See parameter "vcd_name"

  sc_trace_file                        *m_p_trace_file;                 ///< The VCD file

  sc_clock                             *m_p_clock;                      ///< The sc_clock object
  log4xtensa::TextLogger&               m_text;                         ///< Text logger
};


};  // namespace xtsc_vp 


#endif  // _XTSC_CLOCK_VP_H_

