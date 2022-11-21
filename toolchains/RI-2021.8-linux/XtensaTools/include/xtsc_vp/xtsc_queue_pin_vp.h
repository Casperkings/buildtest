#ifndef _XTSC_QUEUE_PIN_VP_H_
#define _XTSC_QUEUE_PIN_VP_H_

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
#include <xtsc/xtsc_queue_pin.h>
#include <xtsc_vp/xtsc_vp.h>



namespace xtsc_vp {


/// Wrapper for xtsc_queue_pin in Synopsys Virtual Prototyping Environment 
template <unsigned int DATA_WIDTH = 32> 
class xtsc_queue_pin_vp : public sc_core::sc_module {
public:

  sc_core::sc_in <bool>                            m_push;      ///<  Push request(LX)/command(NX) from producer
  sc_core::sc_in <sc_dt::sc_biguint<DATA_WIDTH> >  m_data_in;   ///<  Input data from producer
  sc_core::sc_out<bool>                            m_full;      ///<  Signal producer that queue is full         (LX only)
  sc_core::sc_out<sc_dt::sc_biguint<8> >           m_pushable;  ///<  Number of pushable empty slots in queue    (NX only)

  sc_core::sc_in <bool>                            m_pop;       ///<  Pop request(LX)/command(NX) from consumer
  sc_core::sc_out<sc_dt::sc_biguint<DATA_WIDTH> >  m_data_out;  ///<  Output data to consumer
  sc_core::sc_out<bool>                            m_empty;     ///<  Signal consumer that queue is empty        (LX only)
  sc_core::sc_out<sc_dt::sc_biguint<8> >           m_poppable;  ///<  Number of poppable occupied slots in queue (NX only)

  xtsc_queue_pin_vp(const sc_core::sc_module_name &module_name) :
    sc_module                     (module_name),
    m_push                        ("m_push"),
    m_data_in                     ("m_data_in"),
    m_full                        ("m_full"),
    m_pushable                    ("m_pushable"),
    m_pop                         ("m_pop"),
    m_data_out                    ("m_data_out"),
    m_empty                       ("m_empty"),
    m_poppable                    ("m_poppable"),
    m_push_adapter                ("m_push_adapter"),
    m_data_in_adapter             ("m_data_in_adapter"),
    m_full_adapter                ("m_full_adapter"),
    m_pushable_adapter            ("m_pushable_adapter"),
    m_pop_adapter                 ("m_pop_adapter"),
    m_data_out_adapter            ("m_data_out_adapter"),
    m_empty_adapter               ("m_empty_adapter"),
    m_poppable_adapter            ("m_poppable_adapter"),
    m_depth                       ("/Misc/depth",               16),
    m_pop_file                    ("/Misc/pop_file",            ""),
    m_push_file                   ("/Misc/push_file",           ""),
    m_nx                          ("/Misc/nx",                  false),
    m_nx_producer                 ("/Misc/nx_producer",         false),
    m_nx_consumer                 ("/Misc/nx_consumer",         false),
    m_timestamp                   ("/Misc/timestamp",           true),
    m_wraparound                  ("/Misc/wraparound",          false),
    m_clock_period                ("/Timing/clock_period",      0xFFFFFFFF),
    m_posedge_offset              ("/Timing/posedge_offset",    0xFFFFFFFF),
    m_vcd_name                    ("/Trace/vcd_name",           ""),
    m_p_queue                     (0)
  {
    xtsc_vp_initialize();

    xtsc_component::xtsc_queue_pin_parms queue_parms;

    queue_parms.set("bit_width",               DATA_WIDTH);
    queue_parms.set("depth",                   m_depth);
    queue_parms.set("pop_file",                m_pop_file.getValue().c_str());
    queue_parms.set("push_file",               m_push_file.getValue().c_str());
    queue_parms.set("nx",                      m_nx);
    queue_parms.set("nx_producer",             m_nx_producer);
    queue_parms.set("nx_consumer",             m_nx_consumer);
    queue_parms.set("timestamp",               m_timestamp);
    queue_parms.set("wraparound",              m_wraparound);
    queue_parms.set("clock_period",            m_clock_period);
    queue_parms.set("posedge_offset",          m_posedge_offset);
    if (m_vcd_name.getValue() != "") {
    queue_parms.set("vcd_handle",              xtsc_vp_get_trace_file(m_vcd_name.getValue()));
    }

    queue_parms.extract_parms(sc_argc(), sc_argv(), name());

    queue_parms.set("bit_width",               DATA_WIDTH);

    m_p_queue = new xtsc_component::xtsc_queue_pin("_", queue_parms);

    // producer side adapters
    m_p_queue->m_push    (m_push_adapter    .m_sc_export);  m_push_adapter    .m_sc_in (m_push);
    m_p_queue->m_data_in (m_data_in_adapter .m_sc_export);  m_data_in_adapter .m_sc_in (m_data_in);
                                                            m_full_adapter    .m_sc_out(m_full);
                                                            m_pushable_adapter.m_sc_out(m_pushable);
    if (m_nx || m_nx_producer) {
    m_p_queue->m_pushable(m_pushable_adapter.m_sc_export);
    } else {
    m_p_queue->m_full    (m_full_adapter    .m_sc_export);
    }

    // consumer side adapters
    m_p_queue->m_pop     (m_pop_adapter     .m_sc_export);  m_pop_adapter     .m_sc_in (m_pop);
    m_p_queue->m_data_out(m_data_out_adapter.m_sc_export);  m_data_out_adapter.m_sc_out(m_data_out);
                                                            m_empty_adapter   .m_sc_out(m_empty);
                                                            m_poppable_adapter.m_sc_out(m_poppable);
    if (m_nx || m_nx_consumer) {
    m_p_queue->m_poppable(m_poppable_adapter.m_sc_export);
    } else {
    m_p_queue->m_empty   (m_empty_adapter   .m_sc_export);
    }

  }



  virtual ~xtsc_queue_pin_vp() {
    xtsc_vp_finalize();
  }




private:

  xtsc::xtsc_sc_in_sc_bv_base_adapter <1, bool>                                     m_push_adapter;
  xtsc::xtsc_sc_in_sc_bv_base_adapter <DATA_WIDTH, sc_dt::sc_biguint<DATA_WIDTH> >  m_data_in_adapter;
  xtsc::xtsc_sc_out_sc_bv_base_adapter<1, bool>                                     m_full_adapter;
  xtsc::xtsc_sc_out_sc_bv_base_adapter<8, sc_dt::sc_biguint<8> >                    m_pushable_adapter;

  xtsc::xtsc_sc_in_sc_bv_base_adapter <1, bool>                                     m_pop_adapter;
  xtsc::xtsc_sc_out_sc_bv_base_adapter<DATA_WIDTH, sc_dt::sc_biguint<DATA_WIDTH> >  m_data_out_adapter;
  xtsc::xtsc_sc_out_sc_bv_base_adapter<1, bool>                                     m_empty_adapter;
  xtsc::xtsc_sc_out_sc_bv_base_adapter<8, sc_dt::sc_biguint<8> >                    m_poppable_adapter;

  scml_property<unsigned int>           m_depth;
  scml_property<string>                 m_pop_file;
  scml_property<string>                 m_push_file;
  scml_property<bool>                   m_nx;
  scml_property<bool>                   m_nx_producer;
  scml_property<bool>                   m_nx_consumer;
  scml_property<bool>                   m_timestamp;
  scml_property<bool>                   m_wraparound;
  scml_property<unsigned int>           m_clock_period;
  scml_property<unsigned int>           m_posedge_offset;
  scml_property<string>                 m_vcd_name;

  xtsc_component::xtsc_queue_pin       *m_p_queue;

};



}  // namespace xtsc_vp


#endif  // _XTSC_QUEUE_PIN_VP_H_



