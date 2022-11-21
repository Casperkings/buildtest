#ifndef _XTSC_LOOKUP_DRIVER_VP_H_
#define _XTSC_LOOKUP_DRIVER_VP_H_

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
#include <xtsc/xtsc_lookup_driver.h>
#include <xtsc_vp/xtsc_vp.h>



namespace xtsc_vp {


/// Wrapper for xtsc_lookup_driver in Synopsys Virtual Prototyping Environment 
template <unsigned int ADDR_WIDTH = 8, unsigned int DATA_WIDTH = 32> 
class xtsc_lookup_driver_vp : public sc_core::sc_module {
public:

  sc_port<xtsc::xtsc_lookup_if>        m_lookup;       ///<  TLM port to the lookup


  xtsc_lookup_driver_vp(const sc_core::sc_module_name &module_name) :
    sc_module                     (module_name),
    m_lookup                      ("m_lookup"),
    m_has_ready                   ("/Misc/has_ready",           false),
    m_script_file                 ("/Misc/script_file",         ""),
    m_clock_period                ("/Timing/clock_period",      0xFFFFFFFF),
    m_latency                     ("/Timing/latency",           1),
    m_poll_ready_delay            ("/Timing/poll_ready_delay",  0xFFFFFFFF),
    m_posedge_offset              ("/Timing/posedge_offset",    0xFFFFFFFF),
    m_p_lookup_driver             (0)
  {
    xtsc_vp_initialize();

    xtsc_component::xtsc_lookup_driver_parms lookup_driver_parms(0, 0, false);

    lookup_driver_parms.set("address_bit_width",       ADDR_WIDTH);
    lookup_driver_parms.set("data_bit_width",          DATA_WIDTH);
    lookup_driver_parms.set("clock_period",            m_clock_period);
    lookup_driver_parms.set("has_ready",               m_has_ready);
    lookup_driver_parms.set("latency",                 m_latency);
    lookup_driver_parms.set("poll_ready_delay",        m_poll_ready_delay);
    lookup_driver_parms.set("posedge_offset",          m_posedge_offset);
    lookup_driver_parms.set("script_file",             m_script_file.getValue().c_str());

    lookup_driver_parms.extract_parms(sc_argc(), sc_argv(), name());
    lookup_driver_parms.set("address_bit_width",       ADDR_WIDTH);
    lookup_driver_parms.set("data_bit_width",          DATA_WIDTH);
    m_p_lookup_driver = new xtsc_component::xtsc_lookup_driver("_", lookup_driver_parms);

    m_p_lookup_driver->m_lookup(m_lookup);
  }



  virtual ~xtsc_lookup_driver_vp() {
    xtsc_vp_finalize();
  }




private:

  scml_property<bool>                   m_has_ready;
  scml_property<string>                 m_script_file;
  scml_property<unsigned int>           m_clock_period;
  scml_property<unsigned int>           m_latency;
  scml_property<unsigned int>           m_poll_ready_delay;
  scml_property<unsigned int>           m_posedge_offset;

  xtsc_component::xtsc_lookup_driver   *m_p_lookup_driver;

};



}  // namespace xtsc_vp


#endif  // _XTSC_LOOKUP_DRIVER_VP_H_



