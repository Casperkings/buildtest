#ifndef _XTSC_ARBITER_VP_H_
#define _XTSC_ARBITER_VP_H_

// Copyright (c) 2006-2017 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Cadence Design Systems, Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Cadence Design Systems, Inc.

/**
 * @file 
 */

#include <scml.h>
#include <xtsc/xtsc_arbiter.h>
#include <xtsc_vp/xtsc_vp.h>



namespace xtsc_vp {


/// Wrapper for xtsc_arbiter in Synopsys Virtual Prototyping Environment
template <unsigned int NUM_MASTERS = 2, unsigned int DATA_WIDTH = 32> 
class xtsc_arbiter_vp : public sc_core::sc_module {
public:

  sc_export<xtsc::xtsc_request_if>     *m_request_exports[NUM_MASTERS]; ///< Masters bind to these
  sc_port  <xtsc::xtsc_request_if>      m_request_port;                 ///< Bind to single slave
  sc_export<xtsc::xtsc_respond_if>      m_respond_export;               ///< Single slave binds to this
  sc_port  <xtsc::xtsc_respond_if>     *m_respond_ports  [NUM_MASTERS]; ///< Bind to masters

  xtsc_arbiter_vp(const sc_core::sc_module_name &module_name) :
    sc_module                     (module_name),
    m_request_port                ("m_request_port"),
    m_respond_export              ("m_respond_export"),
    m_request_fifo_depth          ("/FIFO/request_fifo_depth",          2),
    m_request_fifo_depths         ("/FIFO/request_fifo_depths",         ""),
    m_response_fifo_depth         ("/FIFO/response_fifo_depth",         2),
    m_apb                         ("/Misc/apb",                         false),
    m_arbitration_policy          ("/Misc/arbitration_policy",          ""),
    m_check_route_id_bits         ("/Misc/check_route_id_bits",         true),
    m_dram_lock                   ("/Misc/dram_lock",                   false),
    m_external_cbox               ("/Misc/external_cbox",               false),
    m_lock_port_groups            ("/Misc/lock_port_groups",            ""),
    m_log_peek_poke               ("/Misc/log_peek_poke",               false),
    m_master_byte_widths          ("/Misc/master_byte_widths",          ""),
    m_num_route_ids               ("/Misc/num_route_ids",               16),
    m_profile_buffers             ("/Misc/profile_buffers",             false),
    m_read_only                   ("/Misc/read_only",                   false),
    m_route_id_lsb                ("/Misc/route_id_lsb",                0),
    m_slave_byte_width            ("/Misc/slave_byte_width",            0),
    m_translation_file            ("/Misc/translation_file",            ""),
    m_use_block_requests          ("/Misc/use_block_requests",          false),
    m_write_only                  ("/Misc/write_only",                  false),
    m_xfer_en_port                ("/Misc/xfer_en_port",                0xFFFFFFFF),
    m_align_request_phase         ("/Timing/align_request_phase",       false),
    m_arbitration_phase           ("/Timing/arbitration_phase",         0xFFFFFFFF),
    m_clock_period                ("/Timing/clock_period",              0xFFFFFFFF),
    m_delay_from_receipt          ("/Timing/delay_from_receipt",        true),
    m_immediate_timing            ("/Timing/immediate_timing",          false),
    m_nacc_wait_time              ("/Timing/nacc_wait_time",            0xFFFFFFFF),
    m_one_at_a_time               ("/Timing/one_at_a_time",             true),
    m_posedge_offset              ("/Timing/posedge_offset",            0xFFFFFFFF),
    m_recovery_time               ("/Timing/recovery_time",             1),
    m_request_delay               ("/Timing/request_delay",             1),
    m_response_delay              ("/Timing/response_delay",            1),
    m_response_repeat             ("/Timing/response_repeat",           1),
    m_p_arbiter                   (0)
  {
    xtsc_vp_initialize();

    xtsc_component::xtsc_arbiter_parms arbiter_parms;

    arbiter_parms.set("align_request_phase",     m_align_request_phase);
    arbiter_parms.set("arbitration_phase",       m_arbitration_phase);
    arbiter_parms.set("clock_period",            m_clock_period);
    arbiter_parms.set("delay_from_receipt",      m_delay_from_receipt);
    arbiter_parms.set("apb",                     m_apb);
    arbiter_parms.set("arbitration_policy",      m_arbitration_policy.getValue().c_str());
    arbiter_parms.set("check_route_id_bits",     m_check_route_id_bits);
    arbiter_parms.set("dram_lock",               m_dram_lock);
    arbiter_parms.set("external_cbox",           m_external_cbox);
    arbiter_parms.set("immediate_timing",        m_immediate_timing);
    vector<xtsc::u32> master_byte_widths;
    bool master_byte_widths_bad_value = false;
    try { xtsc::xtsc_strtou32vector(m_master_byte_widths, master_byte_widths); } catch (...) { master_byte_widths_bad_value = true; }
    if (master_byte_widths_bad_value) {
      std::ostringstream oss;
      oss << "ERROR: Cannot convert value #" << (master_byte_widths.size()+1) << " of parameter \"master_byte_widths\" to u32.";
      throw xtsc::xtsc_exception(oss.str());
    }
    arbiter_parms.set("lock_port_groups",        m_lock_port_groups.getValue().c_str());
    arbiter_parms.set("log_peek_poke",           m_log_peek_poke);
    arbiter_parms.set("master_byte_widths",      master_byte_widths); 
    arbiter_parms.set("num_route_ids",           m_num_route_ids);
    arbiter_parms.set("nacc_wait_time",          m_nacc_wait_time);
    arbiter_parms.set("one_at_a_time",           m_one_at_a_time);
    arbiter_parms.set("posedge_offset",          m_posedge_offset);
    arbiter_parms.set("profile_buffers",         m_profile_buffers);
    arbiter_parms.set("read_only",               m_read_only);
    arbiter_parms.set("recovery_time",           m_recovery_time);
    arbiter_parms.set("request_delay",           m_request_delay);
    arbiter_parms.set("request_fifo_depth",      m_request_fifo_depth);
    vector<xtsc::u32> request_fifo_depths;
    bool request_fifo_depths_bad_value = false;
    try { xtsc::xtsc_strtou32vector(m_request_fifo_depths, request_fifo_depths); } catch (...) { request_fifo_depths_bad_value = true; }
    if (request_fifo_depths_bad_value) {
      std::ostringstream oss;
      oss << "ERROR: Cannot convert value #" << (request_fifo_depths.size()+1) << " of parameter \"request_fifo_depths\" to u32.";
      throw xtsc::xtsc_exception(oss.str());
    }
    arbiter_parms.set("request_fifo_depths",     request_fifo_depths); 
    arbiter_parms.set("response_delay",          m_response_delay);
    arbiter_parms.set("response_fifo_depth",     m_response_fifo_depth);
    arbiter_parms.set("response_repeat",         m_response_repeat);
    arbiter_parms.set("route_id_lsb",            m_route_id_lsb);
    arbiter_parms.set("slave_byte_width",        m_slave_byte_width);
    arbiter_parms.set("translation_file",        m_translation_file.getValue().c_str());
    arbiter_parms.set("use_block_requests",      m_use_block_requests);
    arbiter_parms.set("write_only",              m_write_only);
    arbiter_parms.set("xfer_en_port",            m_xfer_en_port);

    arbiter_parms.extract_parms(sc_argc(), sc_argv(), name());
    arbiter_parms.set("num_masters", NUM_MASTERS);
    m_p_arbiter = new xtsc_component::xtsc_arbiter("_", arbiter_parms);

    m_p_arbiter->m_request_port(m_request_port);
    m_respond_export(m_p_arbiter->m_respond_export);

    for (xtsc::u32 i=0; i<NUM_MASTERS; ++i) {
      std::ostringstream oss1, oss2;
      oss1 << "m_request_exports[" << i << "]";
      oss2 << "m_respond_ports["   << i << "]";
      m_request_exports[i] = new sc_export<xtsc::xtsc_request_if>(oss1.str().c_str());
      m_respond_ports  [i] = new sc_port  <xtsc::xtsc_respond_if>(oss2.str().c_str());
      (*m_p_arbiter->m_respond_ports[i])(*m_respond_ports[i]);
      (*m_request_exports[i])(*m_p_arbiter->m_request_exports[i]);
    }
  }



  virtual ~xtsc_arbiter_vp() {
    xtsc_vp_finalize();
  }

private:

  scml_property<unsigned int>           m_request_fifo_depth;
  scml_property<string>                 m_request_fifo_depths;
  scml_property<unsigned int>           m_response_fifo_depth;
  scml_property<bool>                   m_apb;
  scml_property<string>                 m_arbitration_policy;
  scml_property<bool>                   m_check_route_id_bits;
  scml_property<bool>                   m_dram_lock;
  scml_property<bool>                   m_external_cbox;
  scml_property<string>                 m_lock_port_groups;
  scml_property<bool>                   m_log_peek_poke;
  scml_property<string>                 m_master_byte_widths;
  scml_property<unsigned int>           m_num_route_ids;
  scml_property<bool>                   m_profile_buffers;
  scml_property<bool>                   m_read_only;
  scml_property<unsigned int>           m_route_id_lsb;
  scml_property<unsigned int>           m_slave_byte_width;
  scml_property<string>                 m_translation_file;
  scml_property<bool>                   m_use_block_requests;
  scml_property<bool>                   m_write_only;
  scml_property<unsigned int>           m_xfer_en_port;
  scml_property<bool>                   m_align_request_phase;
  scml_property<unsigned int>           m_arbitration_phase;
  scml_property<unsigned int>           m_clock_period;
  scml_property<bool>                   m_delay_from_receipt;
  scml_property<bool>                   m_immediate_timing;
  scml_property<unsigned int>           m_nacc_wait_time;
  scml_property<bool>                   m_one_at_a_time;
  scml_property<unsigned int>           m_posedge_offset;
  scml_property<unsigned int>           m_recovery_time;
  scml_property<unsigned int>           m_request_delay;
  scml_property<unsigned int>           m_response_delay;
  scml_property<unsigned int>           m_response_repeat;

  xtsc_component::xtsc_arbiter         *m_p_arbiter;

};



class XTSC_VP_API xtsc_arbiter_vp_DIM0026_workaround_do_not_instantiate_me {
  sc_export<xtsc::xtsc_request_if>      m_request_exports;
  sc_port  <xtsc::xtsc_respond_if>      m_respond_ports;
};



}  // namespace xtsc_vp


#endif  // _XTSC_ARBITER_VP_H_
