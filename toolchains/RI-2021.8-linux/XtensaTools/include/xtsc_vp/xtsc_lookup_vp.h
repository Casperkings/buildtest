#ifndef _XTSC_LOOKUP_VP_H_
#define _XTSC_LOOKUP_VP_H_

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
#include <xtsc/xtsc_lookup.h>
#include <xtsc_vp/xtsc_vp.h>



namespace xtsc_vp {


/// Wrapper for xtsc_lookup in Synopsys Virtual Prototyping Environment 
template <unsigned int ADDR_WIDTH = 8, unsigned int DATA_WIDTH = 32> 
class xtsc_lookup_vp : public sc_core::sc_module {
public:

  sc_export<xtsc::xtsc_lookup_if>       m_lookup;       ///<  Driver binds to this


  xtsc_lookup_vp(const sc_core::sc_module_name &module_name) :
    sc_module                     (module_name),
    m_lookup                      ("m_lookup"),
    m_default_data                ("/Misc/default_data",        "0x0"),
    m_dump_after_sim              ("/Misc/dump_after_sim",      false),
    m_has_ready                   ("/Misc/has_ready",           false),
    m_lookup_table                ("/Misc/lookup_table",        ""),
    m_override_lookup             ("/Misc/override_lookup",     false),
    m_pipeline_depth              ("/Misc/pipeline_depth",      0),
    m_active_high_strobe          ("/RAM/active_high_strobe",   true),
    m_ram                         ("/RAM/ram",                  false),
    m_ram_write_enables           ("/RAM/ram_write_enables",    ""),
    m_write_data_lsb              ("/RAM/write_data_lsb",       0),
    m_write_strobe_bit            ("/RAM/write_strobe_bit",     0xFFFFFFFF),
    m_clock_period                ("/Timing/clock_period",      0xFFFFFFFF),
    m_delay                       ("/Timing/delay",             0),
    m_enforce_latency             ("/Timing/enforce_latency",   true),
    m_latency                     ("/Timing/latency",           1),
    m_posedge_offset              ("/Timing/posedge_offset",    0xFFFFFFFF),
    m_vcd_name                    ("/Trace/vcd_name",           ""),
    m_p_lookup                    (0)
  {
    xtsc_vp_initialize();

    xtsc_component::xtsc_lookup_parms lookup_parms(0, 0, false);

    lookup_parms.set("address_bit_width",       ADDR_WIDTH);
    lookup_parms.set("data_bit_width",          DATA_WIDTH);
    lookup_parms.set("active_high_strobe",      m_active_high_strobe);
    lookup_parms.set("clock_period",            m_clock_period);
    lookup_parms.set("default_data",            m_default_data.getValue().c_str());
    lookup_parms.set("delay",                   m_delay);
    lookup_parms.set("enforce_latency",         m_enforce_latency);
    lookup_parms.set("dump_after_sim",          m_dump_after_sim);
    lookup_parms.set("has_ready",               m_has_ready);
    lookup_parms.set("latency",                 m_latency);
    lookup_parms.set("lookup_table",            m_lookup_table.getValue().c_str());
    lookup_parms.set("override_lookup",         m_override_lookup);
    lookup_parms.set("pipeline_depth",          m_pipeline_depth);
    lookup_parms.set("posedge_offset",          m_posedge_offset);
    lookup_parms.set("ram",                     m_ram);
    std::vector<xtsc::u32> ram_write_enables;
    bool ram_write_enables_bad_value = false;
    try { xtsc::xtsc_strtou32vector(m_ram_write_enables, ram_write_enables); } catch (...) { ram_write_enables_bad_value = true; }
    if (ram_write_enables_bad_value) {
      std::ostringstream oss;
      oss << "ERROR: Cannot convert value #" << (ram_write_enables.size()+1) << " of parameter \"ram_write_enables\" to u32.";
      throw xtsc::xtsc_exception(oss.str());
    }
    lookup_parms.set("ram_write_enables",       ram_write_enables); 
    if (m_vcd_name.getValue() != "") {
    lookup_parms.set("vcd_handle",              xtsc_vp_get_trace_file(m_vcd_name.getValue()));
    }
    lookup_parms.set("write_data_lsb",          m_write_data_lsb);
    lookup_parms.set("write_strobe_bit",        m_write_strobe_bit);

    lookup_parms.extract_parms(sc_argc(), sc_argv(), name());
    lookup_parms.set("address_bit_width",       ADDR_WIDTH);
    lookup_parms.set("data_bit_width",          DATA_WIDTH);
    m_p_lookup = new xtsc_component::xtsc_lookup("_", lookup_parms);

    m_lookup(m_p_lookup->m_lookup);
  }



  virtual ~xtsc_lookup_vp() {
    xtsc_vp_finalize();
  }



private:

  scml_property<string>                 m_default_data;
  scml_property<bool>                   m_dump_after_sim;
  scml_property<bool>                   m_has_ready;
  scml_property<string>                 m_lookup_table;
  scml_property<bool>                   m_override_lookup;
  scml_property<unsigned int>           m_pipeline_depth;
  scml_property<bool>                   m_active_high_strobe;
  scml_property<bool>                   m_ram;
  scml_property<string>                 m_ram_write_enables;
  scml_property<unsigned int>           m_write_data_lsb;
  scml_property<unsigned int>           m_write_strobe_bit;
  scml_property<unsigned int>           m_clock_period;
  scml_property<unsigned int>           m_delay;
  scml_property<bool>                   m_enforce_latency;
  scml_property<unsigned int>           m_latency;
  scml_property<unsigned int>           m_posedge_offset;
  scml_property<string>                 m_vcd_name;

  xtsc_component::xtsc_lookup          *m_p_lookup;

};



}  // namespace xtsc_vp


#endif  // _XTSC_LOOKUP_VP_H_



