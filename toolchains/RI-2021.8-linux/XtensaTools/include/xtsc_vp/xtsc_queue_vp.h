#ifndef _XTSC_QUEUE_VP_H_
#define _XTSC_QUEUE_VP_H_

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
#include <xtsc/xtsc_queue.h>
#include <xtsc_vp/xtsc_vp.h>



namespace xtsc_vp {


/// Wrapper for xtsc_queue in Synopsys Virtual Prototyping Environment 
template <unsigned int DATA_WIDTH = 32> 
class xtsc_queue_vp : public sc_core::sc_module {
public:

  sc_core::sc_export<xtsc::xtsc_queue_push_if>  m_producer;     ///<  Single producer binds to this
  sc_core::sc_export<xtsc::xtsc_queue_pop_if>   m_consumer;     ///<  Single consumer binds to this

  xtsc_queue_vp(const sc_core::sc_module_name &module_name) :
    sc_module                     (module_name),
    m_producer                    ("m_producer"),
    m_consumer                    ("m_consumer"),
    m_depth                       ("/Misc/depth",               16),
    m_host_shared_memory          ("/Misc/host_shared_memory",  false),
    m_shared_memory_name          ("/Misc/shared_memory_name",  ""),
    m_pop_file                    ("/Misc/pop_file",            ""),
    m_push_file                   ("/Misc/push_file",           ""),
    m_timestamp                   ("/Misc/timestamp",           true),
    m_wraparound                  ("/Misc/wraparound",          false),
    m_p_queue                     (0)
  {
    xtsc_vp_initialize();

    xtsc_component::xtsc_queue_parms queue_parms;

    queue_parms.set("bit_width",               DATA_WIDTH);
    queue_parms.set("depth",                   m_depth);
    queue_parms.set("host_shared_memory",      m_host_shared_memory);
    queue_parms.set("shared_memory_name",      m_shared_memory_name.getValue().c_str());
    queue_parms.set("pop_file",                m_pop_file.getValue().c_str());
    queue_parms.set("push_file",               m_push_file.getValue().c_str());
    queue_parms.set("timestamp",               m_timestamp);
    queue_parms.set("wraparound",              m_wraparound);

    queue_parms.extract_parms(sc_argc(), sc_argv(), name());
    queue_parms.set("bit_width",               DATA_WIDTH);
    queue_parms.set("num_consumers",           1);
    queue_parms.set("num_producers",           1);
    m_p_queue = new xtsc_component::xtsc_queue("_", queue_parms);

    m_producer(m_p_queue->m_producer);
    m_consumer(m_p_queue->m_consumer);
  }



  virtual ~xtsc_queue_vp() {
    xtsc_vp_finalize();
  }




private:

  scml_property<unsigned int>           m_depth;
  scml_property<bool>                   m_host_shared_memory;
  scml_property<string>                 m_shared_memory_name;
  scml_property<string>                 m_pop_file;
  scml_property<string>                 m_push_file;
  scml_property<bool>                   m_timestamp;
  scml_property<bool>                   m_wraparound;

  xtsc_component::xtsc_queue           *m_p_queue;

};



}  // namespace xtsc_vp


#endif  // _XTSC_QUEUE_VP_H_



