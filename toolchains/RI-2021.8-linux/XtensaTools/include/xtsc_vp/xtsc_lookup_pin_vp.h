#ifndef _XTSC_LOOKUP_PIN_VP_H_
#define _XTSC_LOOKUP_PIN_VP_H_

// Copyright (c) 2006-2013 by Tensilica Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Tensilica Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Tensilica Inc.

/**
 * @file 
 */

#include <scml.h>
#include <xtsc/xtsc_lookup_pin.h>
#include <xtsc_vp/xtsc_vp.h>



namespace xtsc_vp {


/// Wrapper for xtsc_lookup_pin in Synopsys Virtual Prototyping Environment 
template <unsigned int ADDR_WIDTH = 8, unsigned int DATA_WIDTH = 32> 
class xtsc_lookup_pin_vp : public sc_core::sc_module {
public:

  sc_core::sc_in <sc_dt::sc_biguint<ADDR_WIDTH> >  m_address;   ///<  Address from client (TIE_xxx_Out)
  sc_core::sc_in <bool>                            m_req;       ///<  Lookup request      (TIE_xxx_Out_Req)
  sc_core::sc_out<sc_dt::sc_biguint<DATA_WIDTH> >  m_data;      ///<  Value to client     (TIE_xxx_In)
  sc_core::sc_out<bool>                            m_ready;     ///<  Ready to client     (TIE_xxx_Rdy).  Optional.


  xtsc_lookup_pin_vp(const sc_core::sc_module_name &module_name) :
    sc_module                     (module_name),
    m_address                     ("m_address"),
    m_req                         ("m_req"),
    m_data                        ("m_data"),
    m_ready                       ("m_ready"),
    m_address_adapter             ("m_address_adapter"),
    m_req_adapter                 ("m_req_adapter"),
    m_data_adapter                ("m_data_adapter"),
    m_default_data                ("/Misc/default_data",        "0x0"),
    m_has_ready                   ("/Misc/has_ready",           false),
    m_lookup_table                ("/Misc/lookup_table",        ""),
    m_pipeline_depth              ("/Misc/pipeline_depth",      0),
    m_clock_period                ("/Timing/clock_period",      0xFFFFFFFF),
    m_delay                       ("/Timing/delay",             0),
    m_latency                     ("/Timing/latency",           1),
    m_posedge_offset              ("/Timing/posedge_offset",    0xFFFFFFFF),
    m_vcd_name                    ("/Trace/vcd_name",           ""),
    m_p_lookup                    (0)
  {
    xtsc_vp_initialize();

    xtsc_component::xtsc_lookup_pin_parms lookup_parms(0, 0, false);

    lookup_parms.set("address_bit_width",       ADDR_WIDTH);
    lookup_parms.set("data_bit_width",          DATA_WIDTH);
    lookup_parms.set("clock_period",            m_clock_period);
    lookup_parms.set("default_data",            m_default_data.getValue().c_str());
    lookup_parms.set("delay",                   m_delay);
    lookup_parms.set("has_ready",               m_has_ready);
    lookup_parms.set("latency",                 m_latency);
    lookup_parms.set("lookup_table",            m_lookup_table.getValue().c_str());
    lookup_parms.set("pipeline_depth",          m_pipeline_depth);
    lookup_parms.set("posedge_offset",          m_posedge_offset);
    if (m_vcd_name.getValue() != "") {
    lookup_parms.set("vcd_handle",              xtsc_vp_get_trace_file(m_vcd_name.getValue()));
    }

    lookup_parms.extract_parms(sc_argc(), sc_argv(), name());
    lookup_parms.set("address_bit_width",       ADDR_WIDTH);
    lookup_parms.set("data_bit_width",          DATA_WIDTH);
    m_p_lookup = new xtsc_component::xtsc_lookup_pin("_", lookup_parms);

    m_p_lookup->m_address(m_address_adapter.m_sc_export);  m_address_adapter.m_sc_in (m_address);
    m_p_lookup->m_req    (m_req_adapter    .m_sc_export);  m_req_adapter    .m_sc_in (m_req);
    m_p_lookup->m_data   (m_data_adapter   .m_sc_export);  m_data_adapter   .m_sc_out(m_data);
    if (lookup_parms.get_bool("has_ready")) {
    xtsc::xtsc_sc_out_sc_bv_base_adapter<1, bool> *m_ready_adapter = new
    xtsc::xtsc_sc_out_sc_bv_base_adapter<1, bool>("m_ready_adapter");
    m_p_lookup->m_ready  (m_ready_adapter ->m_sc_export);  m_ready_adapter ->m_sc_out(m_ready);
    }

  }



  virtual ~xtsc_lookup_pin_vp() {
    xtsc_vp_finalize();
  }



private:

  xtsc::xtsc_sc_in_sc_bv_base_adapter <ADDR_WIDTH, sc_dt::sc_biguint<ADDR_WIDTH> >  m_address_adapter;
  xtsc::xtsc_sc_in_sc_bv_base_adapter <1, bool>                                     m_req_adapter;
  xtsc::xtsc_sc_out_sc_bv_base_adapter<DATA_WIDTH, sc_dt::sc_biguint<DATA_WIDTH> >  m_data_adapter;

  scml_property<string>                 m_default_data;
  scml_property<bool>                   m_has_ready;
  scml_property<string>                 m_lookup_table;
  scml_property<unsigned int>           m_pipeline_depth;
  scml_property<unsigned int>           m_clock_period;
  scml_property<unsigned int>           m_delay;
  scml_property<unsigned int>           m_latency;
  scml_property<unsigned int>           m_posedge_offset;
  scml_property<string>                 m_vcd_name;

  xtsc_component::xtsc_lookup_pin      *m_p_lookup;

};



}  // namespace xtsc_vp


#endif  // _XTSC_LOOKUP_PIN_VP_H_



