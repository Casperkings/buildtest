#ifndef _XTSC_WIRE_VP_H_
#define _XTSC_WIRE_VP_H_

// Copyright (c) 2006-2018 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Cadence Design Systems, Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Cadence Design Systems, Inc.

/**
 * @file 
 */

#include <scml.h>
#include <xtsc/xtsc_wire.h>
#include <xtsc_vp/xtsc_vp.h>

#define LOG(msg) { cout << sc_time_stamp() << ": " << msg << endl; }


namespace xtsc_vp {


template <unsigned int DATA_WIDTH = 1> 
class xtsc_wire_vp : public sc_core::sc_module {
public:
  sc_core::sc_export<xtsc::xtsc_wire_write_if>  m_write;       ///<  Source binds to this
  sc_core::sc_export<xtsc::xtsc_wire_read_if>   m_read;        ///<  Sink binds to this
      
  // constructor / destructor
  xtsc_wire_vp(const sc_core::sc_module_name &module_name) :
    sc_module             (module_name),
    m_write               ("m_write"),
    m_read                ("m_read"),
    m_host_shared_memory  ("/Misc/host_shared_memory",  false),
    m_shared_memory_name  ("/Misc/shared_memory_name",  ""),
    m_initial_value       ("/Misc/initial_value",       "0x0"),
    m_read_file           ("/Misc/read_file",           ""),
    m_timestamp           ("/Misc/timestamp",           true),
    m_wraparound          ("/Misc/wraparound",          false),
    m_write_file          ("/Misc/write_file",          ""),
    m_vcd_name            ("/Trace/vcd_name",           ""),
    m_p_wire              (0)
  {
    xtsc_vp_initialize();

    xtsc_component::xtsc_wire_parms wire_parms;

    wire_parms.set("bit_width",               DATA_WIDTH);
    wire_parms.set("initial_value",           m_initial_value.getValue().c_str());
    wire_parms.set("host_shared_memory",      m_host_shared_memory);
    wire_parms.set("shared_memory_name",      m_shared_memory_name.getValue().c_str());
    wire_parms.set("read_file",               m_read_file.getValue().c_str());
    wire_parms.set("timestamp",               m_timestamp);
    if (m_vcd_name.getValue() != "") {
    wire_parms.set("vcd_handle",              xtsc_vp_get_trace_file(m_vcd_name.getValue()));
    }
    wire_parms.set("wraparound",              m_wraparound);
    wire_parms.set("write_file",              m_write_file.getValue().c_str());

    wire_parms.extract_parms(sc_argc(), sc_argv(), name());
    wire_parms.set("bit_width",               DATA_WIDTH);
    m_p_wire = new xtsc_component::xtsc_wire("_", wire_parms);

    m_write(*m_p_wire);
    m_read(*m_p_wire);
  }



  virtual ~xtsc_wire_vp() {
    xtsc_vp_finalize();
  }




private:

  scml_property<bool>                   m_host_shared_memory;
  scml_property<string>                 m_shared_memory_name;
  scml_property<string>                 m_initial_value;
  scml_property<string>                 m_read_file;
  scml_property<bool>                   m_timestamp;
  scml_property<bool>                   m_wraparound;
  scml_property<string>                 m_write_file;
  scml_property<string>                 m_vcd_name;

  xtsc_component::xtsc_wire            *m_p_wire;

};



}  // namespace xtsc_vp


#endif  // _XTSC_WIRE_VP_H_
