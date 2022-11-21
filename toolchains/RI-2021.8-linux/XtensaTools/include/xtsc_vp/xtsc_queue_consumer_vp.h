#ifndef _XTSC_QUEUE_CONSUMER_VP_H_
#define _XTSC_QUEUE_CONSUMER_VP_H_

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
#include <xtsc/xtsc_queue_consumer.h>
#include <xtsc_vp/xtsc_vp.h>



namespace xtsc_vp {


/// Wrapper for xtsc_queue_consumer in Synopsys Virtual Prototyping Environment 
template <unsigned int DATA_WIDTH = 32> 
class xtsc_queue_consumer_vp : public sc_core::sc_module {
public:

  sc_port<xtsc::xtsc_queue_pop_if>      m_queue;        ///<  TLM port to queue

  xtsc_queue_consumer_vp(const sc_core::sc_module_name &module_name) :
    sc_module                     (module_name),
    m_queue                       ("m_queue"),
    m_script_file                 ("/Misc/script_file",         ""),
    m_wraparound                  ("/Misc/wraparound",          false),
    m_clock_period                ("/Timing/clock_period",      0xFFFFFFFF),
    m_posedge_offset              ("/Timing/posedge_offset",    0xFFFFFFFF),
    m_p_queue_consumer            (0)
  {
    xtsc_vp_initialize();

    xtsc_component::xtsc_queue_consumer_parms queue_consumer_parms;

    queue_consumer_parms.set("bit_width",               DATA_WIDTH);
    queue_consumer_parms.set("clock_period",            m_clock_period);
    queue_consumer_parms.set("posedge_offset",          m_posedge_offset);
    queue_consumer_parms.set("script_file",             m_script_file.getValue().c_str());
    queue_consumer_parms.set("wraparound",              m_wraparound);

    queue_consumer_parms.extract_parms(sc_argc(), sc_argv(), name());
    queue_consumer_parms.set("bit_width",               DATA_WIDTH);
    m_p_queue_consumer = new xtsc_component::xtsc_queue_consumer("_", queue_consumer_parms);

    m_p_queue_consumer->m_queue(m_queue);
  }



  virtual ~xtsc_queue_consumer_vp() {
    xtsc_vp_finalize();
  }


private:

  scml_property<string>                 m_script_file;
  scml_property<bool>                   m_wraparound;
  scml_property<unsigned int>           m_clock_period;
  scml_property<unsigned int>           m_posedge_offset;

  xtsc_component::xtsc_queue_consumer  *m_p_queue_consumer;

};



}  // namespace xtsc_vp


#endif  // _XTSC_QUEUE_CONSUMER_VP_H_



