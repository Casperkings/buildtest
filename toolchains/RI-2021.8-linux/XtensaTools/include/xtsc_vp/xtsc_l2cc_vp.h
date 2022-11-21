#ifndef _XTSC_L2CC_VP_H_
#define _XTSC_L2CC_VP_H_

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
#include <xtsc/xtsc_l2cc.h>
#include <xtsc_vp/xtsc_vp.h>



namespace xtsc_vp {

/// Wrapper for xtsc_l2cc in Synopsys Virtual Prototyping Environment 
class xtsc_l2cc_vp : public sc_core::sc_module {
public:

  sc_export<xtsc::xtsc_request_if>            coredata_rd_req;
  sc_port  <xtsc::xtsc_respond_if>            coredata_rd_rsp;

  sc_export<xtsc::xtsc_request_if>            coredata_wr_req;
  sc_port  <xtsc::xtsc_respond_if>            coredata_wr_rsp;

  sc_export<xtsc::xtsc_request_if>            coreinst_rd_req;
  sc_port  <xtsc::xtsc_respond_if>            coreinst_rd_rsp;

  sc_export<xtsc::xtsc_request_if>            slave_rd_req;
  sc_port  <xtsc::xtsc_respond_if>            slave_rd_rsp;

  sc_export<xtsc::xtsc_request_if>            slave_wr_req;
  sc_port  <xtsc::xtsc_respond_if>            slave_wr_rsp;

  sc_port  <xtsc::xtsc_request_if>            master_rd_req;
  sc_export<xtsc::xtsc_respond_if>            master_rd_rsp;

  sc_port  <xtsc::xtsc_request_if>            master_wr_req;
  sc_export<xtsc::xtsc_respond_if>            master_wr_rsp;

  sc_port  <xtsc::xtsc_wire_write_if, NSPP>   l2_err_port;
  sc_port  <xtsc::xtsc_wire_write_if, NSPP>   l2_status_port;

  xtsc_l2cc_vp(const sc_core::sc_module_name &module_name) :
    sc_module                     (module_name),
    coredata_rd_req               ("coredata_rd_req"),
    coredata_rd_rsp               ("coredata_rd_rsp"),
    coredata_wr_req               ("coredata_wr_req"),
    coredata_wr_rsp               ("coredata_wr_rsp"),
    coreinst_rd_req               ("coreinst_rd_req"),
    coreinst_rd_rsp               ("coreinst_rd_rsp"),
    slave_rd_req                  ("slave_rd_req"),
    slave_rd_rsp                  ("slave_rd_rsp"),
    slave_wr_req                  ("slave_wr_req"),
    slave_wr_rsp                  ("slave_wr_rsp"),
    master_rd_req                 ("master_rd_req"),
    master_rd_rsp                 ("master_rd_rsp"),
    master_wr_req                 ("master_wr_req"),
    master_wr_rsp                 ("master_wr_rsp"),
    l2_err_port                   ("l2_err_port"),
    l2_status_port                ("l2_status_port"),
    m_control_reg_address         ("/L2CC/control_reg_address",       0xE3A02000),
    m_reset_tcm_base_address      ("/L2CC/reset_tcm_base_address",    0xE3600000),
    m_cache_byte_size             ("/L2CC/cache_byte_size",           0x200000),
    m_clock_period                ("/Timing/clock_period",            0xFFFFFFFF),
    m_nacc_wait_time              ("/Timing/nacc_wait_time",          0xFFFFFFFF),
    m_posedge_offset              ("/Timing/posedge_offset",          0xFFFFFFFF),
    m_arbitration_phase           ("/Timing/arbitration_phase",       0xFFFFFFFF),
    m_turbo                       ("/Turbo/turbo",                    false),
    m_p_l2cc                      (0)
  {
    xtsc_vp_initialize();

    xtsc::xtsc_l2cc_parms l2cc_parms;

    l2cc_parms.set("has_axi_slave_interface",   true);
    l2cc_parms.set("control_reg_address",       m_control_reg_address);
    l2cc_parms.set("reset_tcm_base_address",    m_reset_tcm_base_address);
    l2cc_parms.set("core_data_byte_width",      16);
    l2cc_parms.set("core_inst_byte_width",      16);
    l2cc_parms.set("master_byte_width",         16);
    l2cc_parms.set("slave_byte_width",          8);
    l2cc_parms.set("cache_byte_size",           m_cache_byte_size);
    l2cc_parms.set("line_byte_size",            64);
    l2cc_parms.set("num_ways",                  8);
    l2cc_parms.set("cache_memory_latency",      2);
    l2cc_parms.set("num_perf_counters",         8);
    l2cc_parms.set("has_index_lock",            true);
    l2cc_parms.set("has_l2_error",              true);
    l2cc_parms.set("num_cores",                 1);
    l2cc_parms.set("connect_slave_interface",   true);
    l2cc_parms.set("connect_init_done_port",    false);
    l2cc_parms.set("clock_period",              m_clock_period);
    l2cc_parms.set("nacc_wait_time",            m_nacc_wait_time);
    l2cc_parms.set("posedge_offset",            m_posedge_offset);
    l2cc_parms.set("arbitration_phase",         m_arbitration_phase);
    l2cc_parms.set("turbo",                     m_turbo);
    l2cc_parms.extract_parms(sc_argc(), sc_argv(), name());

    m_p_l2cc = new xtsc::xtsc_l2cc("_", l2cc_parms);

    // sc_port/sc_export binding
    coredata_rd_req(*(sc_core::sc_export<xtsc::xtsc_request_if>*)m_p_l2cc->get_port("core0data_rd_request_export"));
    (*(sc_core::sc_port<xtsc::xtsc_respond_if>*)m_p_l2cc->get_port("core0data_rd_respond_port"))(coredata_rd_rsp);

    coredata_wr_req(*(sc_core::sc_export<xtsc::xtsc_request_if>*)m_p_l2cc->get_port("core0data_wr_request_export"));
    (*(sc_core::sc_port<xtsc::xtsc_respond_if>*)m_p_l2cc->get_port("core0data_wr_respond_port"))(coredata_wr_rsp);

    coreinst_rd_req(*(sc_core::sc_export<xtsc::xtsc_request_if>*)m_p_l2cc->get_port("core0inst_rd_request_export"));
    (*(sc_core::sc_port<xtsc::xtsc_respond_if>*)m_p_l2cc->get_port("core0inst_rd_respond_port"))(coreinst_rd_rsp);

    slave_rd_req(*(sc_core::sc_export<xtsc::xtsc_request_if>*)m_p_l2cc->get_port("slave0_rd_request_export"));
    (*(sc_core::sc_port<xtsc::xtsc_respond_if>*)m_p_l2cc->get_port("slave0_rd_respond_port"))(slave_rd_rsp);

    slave_wr_req(*(sc_core::sc_export<xtsc::xtsc_request_if>*)m_p_l2cc->get_port("slave0_wr_request_export"));
    (*(sc_core::sc_port<xtsc::xtsc_respond_if>*)m_p_l2cc->get_port("slave0_wr_respond_port"))(slave_wr_rsp);

    (*(sc_core::sc_port<xtsc::xtsc_request_if>*)m_p_l2cc->get_port("master0_rd_request_port"))(master_rd_req);
    master_rd_rsp(*(sc_core::sc_export<xtsc::xtsc_respond_if>*)m_p_l2cc->get_port("master0_rd_respond_export"));

    (*(sc_core::sc_port<xtsc::xtsc_request_if>*)m_p_l2cc->get_port("master0_wr_request_port"))(master_wr_req);
    master_wr_rsp(*(sc_core::sc_export<xtsc::xtsc_respond_if>*)m_p_l2cc->get_port("master0_wr_respond_export"));

    (*(sc_core::sc_port<xtsc::xtsc_wire_write_if, NSPP>*)m_p_l2cc->get_port("l2_err"))(l2_err_port);
    (*(sc_core::sc_port<xtsc::xtsc_wire_write_if, NSPP>*)m_p_l2cc->get_port("l2_status"))(l2_status_port);
  }

  virtual ~xtsc_l2cc_vp() {
    xtsc_vp_finalize();
  }

private:

  scml_property<unsigned int>           m_control_reg_address;
  scml_property<unsigned int>           m_reset_tcm_base_address;
  scml_property<unsigned int>           m_cache_byte_size;
  scml_property<unsigned int>           m_clock_period;
  scml_property<unsigned int>           m_nacc_wait_time;
  scml_property<unsigned int>           m_posedge_offset;
  scml_property<unsigned int>           m_arbitration_phase;
  scml_property<bool>                   m_turbo;

  xtsc::xtsc_l2cc                      *m_p_l2cc;

};


} // namespace xtsc_vp

#endif  // _XTSC_L2CC_VP_H_
