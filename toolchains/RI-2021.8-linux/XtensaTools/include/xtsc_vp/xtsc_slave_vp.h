#ifndef _XTSC_SLAVE_VP_H_
#define _XTSC_SLAVE_VP_H_

// Copyright (c) 2006-2011 by Tensilica Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Tensilica Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Tensilica Inc.

/**
 * @file 
 */

#include <scml.h>
#include <xtsc/xtsc_slave.h>
#include <xtsc_vp/xtsc_vp.h>



namespace xtsc_vp {


/// Wrapper for xtsc_slave in Synopsys Virtual Prototyping Environment 
template <unsigned int DATA_WIDTH = 32> 
class xtsc_slave_vp : public sc_core::sc_module {
public:

  sc_export<xtsc::xtsc_request_if>      m_request_export; ///< From memory interface master to us
  sc_port  <xtsc::xtsc_respond_if>      m_respond_port;   ///< From us to memory interface master

  xtsc_slave_vp(const sc_core::sc_module_name &module_name) :
    sc_module                     (module_name),
    m_request_export              ("m_request_export"),
    m_respond_port                ("m_respond_port"),
    m_format                      ("/Misc/format",              1),
    m_repeat_count                ("/Misc/repeat_count",        0),
    m_script_file                 ("/Misc/script_file",         ""),
    m_wraparound                  ("/Misc/wraparound",          true),
    m_clock_period                ("/Timing/clock_period",      0xFFFFFFFF),
    m_repeat_delay                ("/Timing/repeat_delay",      0xFFFFFFFF),
    m_p_slave                     (0)
  {
    xtsc_vp_initialize();

    xtsc_component::xtsc_slave_parms slave_parms;

    slave_parms.set("clock_period",            m_clock_period);
    slave_parms.set("format",                  m_format);
    slave_parms.set("repeat_count",            m_repeat_count);
    slave_parms.set("repeat_delay",            m_repeat_delay);
    slave_parms.set("script_file",             m_script_file.getValue().c_str());
    slave_parms.set("wraparound",              m_wraparound);

    slave_parms.extract_parms(sc_argc(), sc_argv(), name());
    m_p_slave = new xtsc_component::xtsc_slave("_", slave_parms);

    m_request_export(m_p_slave->m_request_export);
    m_p_slave->m_respond_port(m_respond_port);
  }



  virtual ~xtsc_slave_vp() {
    xtsc_vp_finalize();
  }

private:

  scml_property<unsigned int>           m_format;
  scml_property<unsigned int>           m_repeat_count;
  scml_property<string>                 m_script_file;
  scml_property<bool>                   m_wraparound;
  scml_property<unsigned int>           m_clock_period;
  scml_property<unsigned int>           m_repeat_delay;

  xtsc_component::xtsc_slave           *m_p_slave;

};



}  // namespace xtsc_vp


#endif  // _XTSC_SLAVE_VP_H_



