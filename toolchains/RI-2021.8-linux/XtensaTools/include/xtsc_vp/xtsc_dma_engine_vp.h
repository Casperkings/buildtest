#ifndef _XTSC_DMA_ENGINE_VP_H_
#define _XTSC_DMA_ENGINE_VP_H_

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
#include <xtsc/xtsc_dma_engine.h>
#include <xtsc_vp/xtsc_vp.h>



namespace xtsc_vp {


/// Wrapper for xtsc_dma_engine in Synopsys Virtual Prototyping Environment
template <unsigned int DATA_WIDTH = 32> 
class xtsc_dma_engine_vp : public sc_core::sc_module {
public:

  sc_export<xtsc::xtsc_request_if>     m_request_export;       ///< Single master binds to this
  sc_port  <xtsc::xtsc_respond_if>     m_respond_port;         ///< Bind to single master
  sc_port  <xtsc::xtsc_request_if>     m_request_port;         ///< Bind to single slave
  sc_export<xtsc::xtsc_respond_if>     m_respond_export;       ///< Single slave binds to this

  xtsc_dma_engine_vp(const sc_core::sc_module_name &module_name) :
    sc_module                     (module_name),
    m_request_export              ("m_request_export"),
    m_respond_port                ("m_respond_port"),
    m_request_port                ("m_request_port"),
    m_respond_export              ("m_respond_export"),
    m_clear_notify_value          ("/DMA/clear_notify_value",           false),
    m_descriptor_delay            ("/DMA/descriptor_delay",             0),
    m_max_reads                   ("/DMA/max_reads",                    0),
    m_max_writes                  ("/DMA/max_writes",                   0),
    m_overlap_descriptors         ("/DMA/overlap_descriptors",          false),
    m_read_priority               ("/DMA/read_priority",                2),
    m_reg_base_address            ("/DMA/reg_base_address",             0),
    m_start_at_index_1            ("/DMA/start_at_index_1",             true),
    m_write_priority              ("/DMA/write_priority",               2),
    m_request_fifo_depth          ("/FIFO/request_fifo_depth",          2),
    m_check_alignment             ("/Misc/check_alignment",             false),
    m_initial_value_file          ("/Misc/initial_value_file",          ""),
    m_memory_byte_size            ("/Misc/memory_byte_size",            0),
    m_memory_fill_byte            ("/Misc/memory_fill_byte",            0),
    m_page_byte_size              ("/Misc/page_byte_size",              0x4000),
    m_start_byte_address          ("/Misc/start_byte_address",          0),
    m_clock_period                ("/Timing/clock_period",              0xFFFFFFFF),
    m_delay_from_receipt          ("/Timing/delay_from_receipt",        true),
    m_immediate_timing            ("/Timing/immediate_timing",          false),
    m_nacc_wait_time              ("/Timing/nacc_wait_time",            0xFFFFFFFF),
    m_posedge_offset              ("/Timing/posedge_offset",            0xFFFFFFFF),
    m_read_delay                  ("/Timing/read_delay",                0),
    m_recovery_time               ("/Timing/recovery_time",             1),
    m_response_repeat             ("/Timing/response_repeat",           1),
    m_write_delay                 ("/Timing/write_delay",               0),
    m_deny_fast_access            ("/Turbo/deny_fast_access",           ""),
    m_turbo                       ("/Turbo/turbo",                      false),
    m_use_fast_access             ("/Turbo/use_fast_access",            true),
    m_use_raw_access              ("/Turbo/use_raw_access",             true),
    m_p_dma_engine                (0)
  {
    xtsc_vp_initialize();

    xtsc_component::xtsc_dma_engine_parms dma_engine_parms(0);

    dma_engine_parms.set("check_alignment",         m_check_alignment);
    dma_engine_parms.set("clear_notify_value",      m_clear_notify_value);
    dma_engine_parms.set("descriptor_delay",        m_descriptor_delay);
    dma_engine_parms.set("max_reads",               m_max_reads);
    dma_engine_parms.set("max_writes",              m_max_writes);
    dma_engine_parms.set("overlap_descriptors",     m_overlap_descriptors);
    dma_engine_parms.set("clock_period",            m_clock_period);
    dma_engine_parms.set("delay_from_receipt",      m_delay_from_receipt);
    std::vector<xtsc::u32> deny_fast_access;
    bool deny_fast_access_bad_value = false;
    try { xtsc::xtsc_strtou32vector(m_deny_fast_access, deny_fast_access); } catch (...) { deny_fast_access_bad_value = true; }
    if (deny_fast_access_bad_value) {
      std::ostringstream oss;
      oss << "ERROR: Cannot convert value #" << (deny_fast_access.size()+1) << " of parameter \"deny_fast_access\" to u32.";
      throw xtsc::xtsc_exception(oss.str());
    }
    dma_engine_parms.set("deny_fast_access",        deny_fast_access); 
    dma_engine_parms.set("immediate_timing",        m_immediate_timing);
    dma_engine_parms.set("initial_value_file",      m_initial_value_file.getValue().c_str());
    dma_engine_parms.set("memory_byte_size",        m_memory_byte_size);
    dma_engine_parms.set("memory_fill_byte",        m_memory_fill_byte);
    dma_engine_parms.set("nacc_wait_time",          m_nacc_wait_time);
    dma_engine_parms.set("page_byte_size",          m_page_byte_size);
    dma_engine_parms.set("posedge_offset",          m_posedge_offset);
    dma_engine_parms.set("read_delay",              m_read_delay);
    dma_engine_parms.set("read_priority",           m_read_priority);
    dma_engine_parms.set("recovery_time",           m_recovery_time);
    dma_engine_parms.set("reg_base_address",        m_reg_base_address);
    dma_engine_parms.set("request_fifo_depth",      m_request_fifo_depth);
    dma_engine_parms.set("response_repeat",         m_response_repeat);
    dma_engine_parms.set("start_byte_address",      m_start_byte_address);
    dma_engine_parms.set("turbo",                   m_turbo);
    dma_engine_parms.set("use_fast_access",         m_use_fast_access);
    dma_engine_parms.set("use_raw_access",          m_use_raw_access);
    dma_engine_parms.set("write_delay",             m_write_delay);
    dma_engine_parms.set("start_at_index_1",        m_start_at_index_1);
    dma_engine_parms.set("write_priority",          m_write_priority);
    dma_engine_parms.set("byte_width",              DATA_WIDTH/8);

    dma_engine_parms.extract_parms(sc_argc(), sc_argv(), name());
    dma_engine_parms.set("num_ports",               1);
    dma_engine_parms.set("byte_width",              DATA_WIDTH/8);
    m_p_dma_engine = new xtsc_component::xtsc_dma_engine("_", dma_engine_parms);

    (*m_p_dma_engine->m_respond_ports[0])(m_respond_port);
    m_request_export(*m_p_dma_engine->m_request_exports[0]);

    m_p_dma_engine->m_request_port(m_request_port);
    m_respond_export(m_p_dma_engine->m_respond_export);
  }



  virtual ~xtsc_dma_engine_vp() {
    xtsc_vp_finalize();
  }




private:

  scml_property<bool>                   m_clear_notify_value;
  scml_property<unsigned int>           m_descriptor_delay;
  scml_property<unsigned int>           m_max_reads;
  scml_property<unsigned int>           m_max_writes;
  scml_property<bool>                   m_overlap_descriptors;
  scml_property<unsigned int>           m_read_priority;
  scml_property<unsigned int>           m_reg_base_address;
  scml_property<bool>                   m_start_at_index_1;
  scml_property<unsigned int>           m_write_priority;
  scml_property<unsigned int>           m_request_fifo_depth;
  scml_property<bool>                   m_check_alignment;
  scml_property<string>                 m_initial_value_file;
  scml_property<unsigned int>           m_memory_byte_size;
  scml_property<unsigned int>           m_memory_fill_byte;
  scml_property<unsigned int>           m_page_byte_size;
  scml_property<unsigned int>           m_start_byte_address;
  scml_property<unsigned int>           m_clock_period;
  scml_property<bool>                   m_delay_from_receipt;
  scml_property<bool>                   m_immediate_timing;
  scml_property<unsigned int>           m_nacc_wait_time;
  scml_property<unsigned int>           m_posedge_offset;
  scml_property<unsigned int>           m_read_delay;
  scml_property<unsigned int>           m_recovery_time;
  scml_property<unsigned int>           m_response_repeat;
  scml_property<unsigned int>           m_write_delay;
  scml_property<string>                 m_deny_fast_access;
  scml_property<bool>                   m_turbo;
  scml_property<bool>                   m_use_fast_access;
  scml_property<bool>                   m_use_raw_access;

  xtsc_component::xtsc_dma_engine      *m_p_dma_engine;

};

}  // namespace xtsc_vp


#endif  // _XTSC_DMA_ENGINE_VP_H_



