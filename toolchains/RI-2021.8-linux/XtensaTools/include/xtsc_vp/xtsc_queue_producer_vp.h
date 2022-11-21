#ifndef _XTSC_QUEUE_PRODUCER_VP_H_
#define _XTSC_QUEUE_PRODUCER_VP_H_

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
#include <xtsc/xtsc_queue_producer.h>
#include <xtsc_vp/xtsc_vp.h>



namespace xtsc_vp {


/// Wrapper for xtsc_queue_producer in Synopsys Virtual Prototyping Environment 
template <unsigned int DATA_WIDTH = 32> 
class xtsc_queue_producer_vp : public sc_core::sc_module {
public:

  sc_port  <xtsc::xtsc_queue_push_if>   m_queue;        ///< TLM port to the queue
  sc_export<xtsc::xtsc_wire_write_if>   m_control;      ///< Optional 1-bit control input

  xtsc_queue_producer_vp(const sc_core::sc_module_name &module_name) :
    sc_module                     (module_name),
    m_queue                       ("m_queue"),
    m_control                     ("m_control"),
    m_script_file                 ("/Misc/script_file",         ""),
    m_clock_period                ("/Timing/clock_period",      0xFFFFFFFF),
    m_posedge_offset              ("/Timing/posedge_offset",    0xFFFFFFFF),
    m_p_queue_producer            (0)
  {
    xtsc_vp_initialize();

    xtsc_component::xtsc_queue_producer_parms queue_producer_parms;

    queue_producer_parms.set("bit_width",               DATA_WIDTH);
    queue_producer_parms.set("clock_period",            m_clock_period);
    queue_producer_parms.set("posedge_offset",          m_posedge_offset);
    queue_producer_parms.set("script_file",             m_script_file.getValue().c_str());

    queue_producer_parms.extract_parms(sc_argc(), sc_argv(), name());
    queue_producer_parms.set("control",                 true);
    queue_producer_parms.set("bit_width",               DATA_WIDTH);
    m_p_queue_producer = new xtsc_component::xtsc_queue_producer("_", queue_producer_parms);

    m_p_queue_producer->m_queue(m_queue);
    m_control(m_p_queue_producer->get_control_input());
  }



  virtual ~xtsc_queue_producer_vp() {
    xtsc_vp_finalize();
  }




private:

  scml_property<string>                 m_script_file;
  scml_property<unsigned int>           m_clock_period;
  scml_property<unsigned int>           m_posedge_offset;

  xtsc_component::xtsc_queue_producer  *m_p_queue_producer;

};



}  // namespace xtsc_vp


#endif  // _XTSC_QUEUE_PRODUCER_VP_H_



