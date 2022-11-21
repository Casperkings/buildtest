#ifndef _XTSC_ROUTER_VP_H_
#define _XTSC_ROUTER_VP_H_

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
#include <xtsc/xtsc_router.h>
#include <xtsc_vp/xtsc_vp.h>



namespace xtsc_vp {


/// Wrapper for xtsc_router in Synopsys Virtual Prototyping Environment 
template <unsigned int NUM_SLAVES = 2, unsigned int DATA_WIDTH = 32> 
class xtsc_router_vp : public sc_core::sc_module {
public:

  sc_export<xtsc::xtsc_request_if>      m_request_export;               ///<  From single master to us
  sc_port  <xtsc::xtsc_request_if>     *m_request_ports[NUM_SLAVES];    ///<  From us to multiple slaves
  sc_export<xtsc::xtsc_respond_if>     *m_respond_exports[NUM_SLAVES];  ///<  From multiple slaves to us
  sc_port  <xtsc::xtsc_respond_if>      m_respond_port;                 ///<  From us to single master

  xtsc_router_vp(const sc_core::sc_module_name &module_name) :
    sc_module                     (module_name),
    m_request_export              ("m_request_export"),
    m_respond_port                ("m_respond_port"),
    m_request_fifo_depth          ("/FIFO/request_fifo_depth",          2),
    m_response_fifo_depth         ("/FIFO/response_fifo_depth",         2),
    m_response_fifo_depths        ("/FIFO/response_fifo_depths",        "" ),
    m_address_routing_bits        ("/Misc/address_routing_bits",        "" ),
    m_default_delta               ("/Misc/default_delta",               0),
    m_default_port_num            ("/Misc/default_port_num",            0xFFFFFFFE),
    m_default_routing             ("/Misc/default_routing",             true),
    m_flexible_request_id         ("/Misc/flexible_request_id",         false),
    m_interleave_responses        ("/Misc/interleave_responses",        false),
    m_log_peek_poke               ("/Misc/log_peek_poke",               false),
    m_master_byte_width           ("/Misc/master_byte_width",           0),
    m_profile_buffers             ("/Misc/profile_buffers",             false),
    m_read_only                   ("/Misc/read_only",                   false),
    m_route_by_priority           ("/Misc/route_by_priority",           ""),
    m_route_by_type               ("/Misc/route_by_type",               ""),
    m_routing_table               ("/Misc/routing_table",               ""),
    m_slave_byte_widths           ("/Misc/slave_byte_widths",           "" ),
    m_use_block_requests          ("/Misc/use_block_requests",          false),
    m_write_only                  ("/Misc/write_only",                  false),
    m_clock_period                ("/Timing/clock_period",              0xFFFFFFFF),
    m_delay_from_receipt          ("/Timing/delay_from_receipt",        true),
    m_immediate_timing            ("/Timing/immediate_timing",          false),
    m_nacc_wait_time              ("/Timing/nacc_wait_time",            0xFFFFFFFF),
    m_read_delay                  ("/Timing/read_delay",                0xFFFFFFFF),
    m_recovery_time               ("/Timing/recovery_time",             1),
    m_request_delay               ("/Timing/request_delay",             1),
    m_response_delay              ("/Timing/response_delay",            1),
    m_response_repeat             ("/Timing/response_repeat",           1),
    m_write_delay                 ("/Timing/write_delay",               0xFFFFFFFF),
    m_p_router                    (0)
  {
    xtsc_vp_initialize();

    xtsc_component::xtsc_router_parms router_parms;

    std::vector<xtsc::u32> address_routing_bits;
    bool address_routing_bits_bad_value = false;
    try { xtsc::xtsc_strtou32vector(m_address_routing_bits, address_routing_bits); }
    catch (...) { address_routing_bits_bad_value = true; }
    if (address_routing_bits_bad_value) {
      std::ostringstream oss;
      oss << "ERROR: Cannot convert value #" << (address_routing_bits.size()+1) << " of parameter \"address_routing_bits\" to u32.";
      throw xtsc::xtsc_exception(oss.str());
    }
    router_parms.set("address_routing_bits",    address_routing_bits); 
    router_parms.set("clock_period",            m_clock_period);
    router_parms.set("default_delta",           m_default_delta);
    router_parms.set("default_port_num",        m_default_port_num);
    router_parms.set("default_routing",         m_default_routing);
    router_parms.set("delay_from_receipt",      m_delay_from_receipt);
    router_parms.set("flexible_request_id",     m_flexible_request_id);
    router_parms.set("immediate_timing",        m_immediate_timing);
    router_parms.set("interleave_responses",    m_interleave_responses);
    router_parms.set("log_peek_poke",           m_log_peek_poke);
    router_parms.set("master_byte_width",       m_master_byte_width);
    router_parms.set("nacc_wait_time",          m_nacc_wait_time);
    router_parms.set("profile_buffers",         m_profile_buffers);
    router_parms.set("read_delay",              m_read_delay);
    router_parms.set("read_only",               m_read_only);
    router_parms.set("recovery_time",           m_recovery_time);
    router_parms.set("request_delay",           m_request_delay);
    router_parms.set("request_fifo_depth",      m_request_fifo_depth);
    router_parms.set("response_delay",          m_response_delay);
    router_parms.set("response_fifo_depth",     m_response_fifo_depth);
    std::vector<xtsc::u32> response_fifo_depths;
    bool response_fifo_depths_bad_value = false;
    try { xtsc::xtsc_strtou32vector(m_response_fifo_depths, response_fifo_depths); }
    catch (...) { response_fifo_depths_bad_value = true; }
    if (response_fifo_depths_bad_value) {
      std::ostringstream oss;
      oss << "ERROR: Cannot convert value #" << (response_fifo_depths.size()+1) << " of parameter \"response_fifo_depths\" to u32.";
      throw xtsc::xtsc_exception(oss.str());
    }
    router_parms.set("response_fifo_depths",    response_fifo_depths); 
    router_parms.set("response_repeat",         m_response_repeat);
    std::vector<xtsc::u32> route_by_priority;
    bool route_by_priority_bad_value = false;
    try { xtsc::xtsc_strtou32vector(m_route_by_priority, route_by_priority); } catch (...) { route_by_priority_bad_value = true; }
    if (route_by_priority_bad_value) {
      ostringstream oss;
      oss << "ERROR: Cannot convert value #" << (route_by_priority.size()+1) << " of parameter \"route_by_priority\" to u32.";
      throw xtsc::xtsc_exception(oss.str());
    }
    router_parms.set("route_by_priority",       route_by_priority); 
    router_parms.set("route_by_type",           m_route_by_type.getValue().c_str());
    router_parms.set("routing_table",           m_routing_table.getValue().c_str());
    std::vector<xtsc::u32> slave_byte_widths;
    bool slave_byte_widths_bad_value = false;
    try { xtsc::xtsc_strtou32vector(m_slave_byte_widths, slave_byte_widths); } catch (...) { slave_byte_widths_bad_value = true; }
    if (slave_byte_widths_bad_value) {
      std::ostringstream oss;
      oss << "ERROR: Cannot convert value #" << (slave_byte_widths.size()+1) << " of parameter \"slave_byte_widths\" to u32.";
      throw xtsc::xtsc_exception(oss.str());
    }
    router_parms.set("slave_byte_widths",       slave_byte_widths); 
    router_parms.set("use_block_requests",      m_use_block_requests);
    router_parms.set("write_delay",             m_write_delay);
    router_parms.set("write_only",              m_write_only);

    router_parms.extract_parms(sc_argc(), sc_argv(), name());
    router_parms.set("num_slaves", NUM_SLAVES);
    m_p_router = new xtsc_component::xtsc_router("_", router_parms);

    m_p_router->m_respond_port(m_respond_port);
    m_request_export(m_p_router->m_request_export);

    for (xtsc::u32 i=0; i<NUM_SLAVES; ++i) {
      std::ostringstream oss1, oss2;
      oss1 << "m_respond_exports[" << i << "]";
      oss2 << "m_request_ports["   << i << "]";
      m_respond_exports[i] = new sc_export<xtsc::xtsc_respond_if>(oss1.str().c_str());
      m_request_ports  [i] = new sc_port  <xtsc::xtsc_request_if>(oss2.str().c_str());
      (*m_p_router->m_request_ports[i])(*m_request_ports[i]);
      (*m_respond_exports[i])(*m_p_router->m_respond_exports[i]);
    }
  }



  virtual ~xtsc_router_vp() {
    xtsc_vp_finalize();
  }




private:

  scml_property<unsigned int>           m_request_fifo_depth;
  scml_property<unsigned int>           m_response_fifo_depth;
  scml_property<string>                 m_response_fifo_depths;
  scml_property<string>                 m_address_routing_bits;
  scml_property<unsigned int>           m_default_delta;
  scml_property<unsigned int>           m_default_port_num;
  scml_property<bool>                   m_default_routing;
  scml_property<bool>                   m_flexible_request_id;
  scml_property<bool>                   m_interleave_responses;
  scml_property<bool>                   m_log_peek_poke;
  scml_property<unsigned int>           m_master_byte_width;
  scml_property<bool>                   m_profile_buffers;
  scml_property<bool>                   m_read_only;
  scml_property<string>                 m_route_by_priority;
  scml_property<string>                 m_route_by_type;
  scml_property<string>                 m_routing_table;
  scml_property<string>                 m_slave_byte_widths;
  scml_property<bool>                   m_use_block_requests;
  scml_property<bool>                   m_write_only;
  scml_property<unsigned int>           m_clock_period;
  scml_property<bool>                   m_delay_from_receipt;
  scml_property<bool>                   m_immediate_timing;
  scml_property<unsigned int>           m_nacc_wait_time;
  scml_property<unsigned int>           m_read_delay;
  scml_property<unsigned int>           m_recovery_time;
  scml_property<unsigned int>           m_request_delay;
  scml_property<unsigned int>           m_response_delay;
  scml_property<unsigned int>           m_response_repeat;
  scml_property<unsigned int>           m_write_delay;

  xtsc_component::xtsc_router          *m_p_router;

};



class XTSC_VP_API xtsc_router_vp_DIM0026_workaround_do_not_instantiate_me {
  sc_port  <xtsc::xtsc_request_if>      m_request_ports;
  sc_export<xtsc::xtsc_respond_if>      m_respond_exports;
};



}  // namespace xtsc_vp


#endif  // _XTSC_ROUTER_VP_H_
