#ifndef _XTSC_MASTER_VP_H_
#define _XTSC_MASTER_VP_H_

// Copyright (c) 2006-2012 by Tensilica Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Tensilica Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Tensilica Inc.

/**
 * @file 
 */

#include <scml.h>
#include <xtsc/xtsc_master.h>
#include <xtsc_vp/xtsc_vp.h>



namespace xtsc_vp {


/// Wrapper for xtsc_master in Synopsys Virtual Prototyping Environment 
template <unsigned int DATA_WIDTH = 32> 
class xtsc_master_vp : public sc_core::sc_module {
public:

  sc_port  <xtsc::xtsc_request_if>      m_request_port;    ///< From us to slave
  sc_export<xtsc::xtsc_respond_if>      m_respond_export;  ///< From slave to us
  sc_export<xtsc::xtsc_wire_write_if>   m_control;         ///< Optional 1-bit control input

  xtsc_master_vp(const sc_core::sc_module_name &module_name) :
    sc_module                     (module_name),
    m_request_port                ("m_request_port"),
    m_respond_export              ("m_respond_export"),
    m_control                     ("m_control"),
    m_format                      ("/Misc/format",              1),
    m_return_value_file           ("/Misc/return_value_file",   ""),
    m_script_file                 ("/Misc/script_file",         ""),
    m_wraparound                  ("/Misc/wraparound",          false),
    m_zero_delay_repeat           ("/Misc/zero_delay_repeat",   false),
    m_clock_period                ("/Timing/clock_period",      0xFFFFFFFF),
    m_posedge_offset              ("/Timing/posedge_offset",    0xFFFFFFFF),
    m_p_master                    (0)
  {
    xtsc_vp_initialize();

    xtsc_component::xtsc_master_parms master_parms(0);

    master_parms.set("clock_period",            m_clock_period);
    master_parms.set("format",                  m_format);
    master_parms.set("posedge_offset",          m_posedge_offset);
    master_parms.set("return_value_file",       m_return_value_file.getValue().c_str());
    master_parms.set("script_file",             m_script_file.getValue().c_str());
    master_parms.set("wraparound",              m_wraparound);
    master_parms.set("zero_delay_repeat",       m_zero_delay_repeat);

    master_parms.extract_parms(sc_argc(), sc_argv(), name());
    master_parms.set("control",                 true);
    m_p_master = new xtsc_component::xtsc_master("_", master_parms);

    m_p_master->m_request_port(m_request_port);
    m_respond_export(m_p_master->m_respond_export);
    m_control(m_p_master->get_control_input());
  }



  virtual ~xtsc_master_vp() {
    xtsc_vp_finalize();
  }


private:

  scml_property<unsigned int>           m_format;
  scml_property<string>                 m_return_value_file;
  scml_property<string>                 m_script_file;
  scml_property<bool>                   m_wraparound;
  scml_property<bool>                   m_zero_delay_repeat;
  scml_property<unsigned int>           m_clock_period;
  scml_property<unsigned int>           m_posedge_offset;

  xtsc_component::xtsc_master          *m_p_master;

};



}  // namespace xtsc_vp


#endif  // _XTSC_MASTER_VP_H_



