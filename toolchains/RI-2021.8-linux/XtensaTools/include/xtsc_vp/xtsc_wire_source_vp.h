#ifndef _XTSC_WIRE_SOURCE_VP_H_
#define _XTSC_WIRE_SOURCE_VP_H_

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
#include <xtsc/xtsc_wire_source.h>
#include <xtsc_vp/xtsc_vp.h>

#define LOG(msg) { cout << sc_time_stamp() << ": " << msg << endl; }



namespace xtsc_vp {


template <unsigned int DATA_WIDTH = 1> 
class xtsc_wire_source_vp : public sc_core::sc_module {
public:

  sc_port  <xtsc::xtsc_wire_write_if, NSPP>    m_write;        ///< TLM port to the wire
  sc_export<xtsc::xtsc_wire_write_if>          m_control;      ///< Optional 1-bit control input
      
  // constructor / destructor
  xtsc_wire_source_vp(const sc_core::sc_module_name &module_name) :
    sc_module             (module_name),
    m_write               ("m_write"),
    m_control             ("m_control"),
    m_script_file         ("/Misc/script_file",         ""),
    m_wraparound          ("/Misc/wraparound",          false),
    m_clock_period        ("/Timing/clock_period",      0xFFFFFFFF),
    m_posedge_offset      ("/Timing/posedge_offset",    0xFFFFFFFF),
    m_vcd_name            ("/Trace/vcd_name",           ""),
    m_p_wire_source       (0)
  {
    xtsc_vp_initialize();

    xtsc_component::xtsc_wire_source_parms wire_source_parms(DATA_WIDTH, m_script_file.getStringValue().c_str(), m_wraparound);

    wire_source_parms.set("clock_period",       m_clock_period);
    wire_source_parms.set("posedge_offset",     m_posedge_offset);
    if (m_vcd_name.getValue() != "") {
    wire_source_parms.set("vcd_handle",         xtsc_vp_get_trace_file(m_vcd_name.getValue()));
    }

    wire_source_parms.extract_parms(sc_argc(), sc_argv(), name());
    wire_source_parms.set("control",            true);
    wire_source_parms.set("bit_width",          DATA_WIDTH);
    m_p_wire_source = new xtsc_component::xtsc_wire_source("_", wire_source_parms);

    m_p_wire_source->m_write(m_write);
    m_control(m_p_wire_source->get_control_input());
  }



  virtual ~xtsc_wire_source_vp() {
    xtsc_vp_finalize();
  }


private:

  scml_property<string>                 m_script_file;
  scml_property<bool>                   m_wraparound;
  scml_property<unsigned int>           m_clock_period;
  scml_property<unsigned int>           m_posedge_offset;
  scml_property<string>                 m_vcd_name;

  xtsc_component::xtsc_wire_source     *m_p_wire_source;

};



}  // namespace xtsc_vp


#endif  // _XTSC_WIRE_SOURCE_VP_H_
