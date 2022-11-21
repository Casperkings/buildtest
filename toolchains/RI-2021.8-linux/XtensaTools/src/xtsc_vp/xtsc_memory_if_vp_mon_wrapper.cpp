#include "systemc.h"
#include "xtsc_vp/xtsc_if_vp_stubs.h"
#include "xtsc_vp/xtsc_memory_if_vp_mon.h"
#include "cassert"
#include "cwr_dynamic_loader.h"
#include "cwr_sc_dynamic_stubs.h"
#include "cwr_sc_hierarch_module.h"
#include "cwr_sc_object_creator.h"
#include "scmlinc/scml_abstraction_level_switch.h"
#include "scmlinc/scml_property_registry.h"

namespace xtsc_memory_if_vp_mon_xtsc_vp__xtsc_memory_if_vp_mon_FastBuild {

using namespace conf;
using namespace std;

template<int DATA_WIDTH>
class xtsc_vp__xtsc_memory_if_vp_mon0Creator : public ScObjectCreatorBase
{
public:
  static unsigned int creationVerboseMode() {
    const char * const env_var_val = ::getenv("SNPS_SLS_DYNAMIC_CREATION_VERBOSE");
    return env_var_val ? (::atoi(env_var_val)) : 3;
  }
  sc_object* create ( const string& name ) {
    string hierach_name = getHierarchicalName(name);
    if (scml_property_registry::inst().hasProperty(scml_property_registry::MODULE, scml_property_registry::BOOL, hierach_name, "runtime_disabled") && 
        scml_property_registry::inst().getBoolProperty(scml_property_registry::MODULE, hierach_name, "runtime_disabled")) {
      sc_module_name n(name.c_str());
      if (creationVerboseMode() >= 6) { std::cout << "xtsc_memory_if_vp_mon/xtsc_vp::xtsc_memory_if_vp_mon: STUB for " << hierach_name << " created." << std::endl; }
      conf::stub *result = new conf::stub(n);
       cwr_sc_object_registry::inst().addExport(new xtsc::xtsc_request_if_vp_stub(std::string("m_request_export" ).c_str()), string(static_cast<sc_object*>(result)->name()) + ".m_request_export" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_request_port, "m_request_port" , string(static_cast<sc_object*>(result)->name()) + ".m_request_port" );
       cwr_sc_object_registry::inst().addExport(new xtsc::xtsc_respond_if_vp_stub(std::string("m_respond_export" ).c_str()), string(static_cast<sc_object*>(result)->name()) + ".m_respond_export" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_respond_port, "m_respond_port" , string(static_cast<sc_object*>(result)->name()) + ".m_respond_port" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_req_count, "m_req_count" , string(static_cast<sc_object*>(result)->name()) + ".m_req_count" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_req_tag, "m_req_tag" , string(static_cast<sc_object*>(result)->name()) + ".m_req_tag" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_req_address8, "m_req_address8" , string(static_cast<sc_object*>(result)->name()) + ".m_req_address8" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_req_data, "m_req_data" , string(static_cast<sc_object*>(result)->name()) + ".m_req_data" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_req_size8, "m_req_size8" , string(static_cast<sc_object*>(result)->name()) + ".m_req_size8" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_req_route_id, "m_req_route_id" , string(static_cast<sc_object*>(result)->name()) + ".m_req_route_id" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_req_type, "m_req_type" , string(static_cast<sc_object*>(result)->name()) + ".m_req_type" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_req_num_transfers, "m_req_num_transfers" , string(static_cast<sc_object*>(result)->name()) + ".m_req_num_transfers" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_req_byte_enables, "m_req_byte_enables" , string(static_cast<sc_object*>(result)->name()) + ".m_req_byte_enables" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_req_id, "m_req_id" , string(static_cast<sc_object*>(result)->name()) + ".m_req_id" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_req_priority, "m_req_priority" , string(static_cast<sc_object*>(result)->name()) + ".m_req_priority" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_req_last_transfer, "m_req_last_transfer" , string(static_cast<sc_object*>(result)->name()) + ".m_req_last_transfer" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_req_pc, "m_req_pc" , string(static_cast<sc_object*>(result)->name()) + ".m_req_pc" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_rsp_count, "m_rsp_count" , string(static_cast<sc_object*>(result)->name()) + ".m_rsp_count" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_rsp_ok_count, "m_rsp_ok_count" , string(static_cast<sc_object*>(result)->name()) + ".m_rsp_ok_count" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_rsp_nacc_count, "m_rsp_nacc_count" , string(static_cast<sc_object*>(result)->name()) + ".m_rsp_nacc_count" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_rsp_tag, "m_rsp_tag" , string(static_cast<sc_object*>(result)->name()) + ".m_rsp_tag" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_rsp_address8, "m_rsp_address8" , string(static_cast<sc_object*>(result)->name()) + ".m_rsp_address8" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_rsp_data, "m_rsp_data" , string(static_cast<sc_object*>(result)->name()) + ".m_rsp_data" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_rsp_size8, "m_rsp_size8" , string(static_cast<sc_object*>(result)->name()) + ".m_rsp_size8" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_rsp_route_id, "m_rsp_route_id" , string(static_cast<sc_object*>(result)->name()) + ".m_rsp_route_id" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_rsp_status, "m_rsp_status" , string(static_cast<sc_object*>(result)->name()) + ".m_rsp_status" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_rsp_id, "m_rsp_id" , string(static_cast<sc_object*>(result)->name()) + ".m_rsp_id" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_rsp_priority, "m_rsp_priority" , string(static_cast<sc_object*>(result)->name()) + ".m_rsp_priority" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_rsp_last_transfer, "m_rsp_last_transfer" , string(static_cast<sc_object*>(result)->name()) + ".m_rsp_last_transfer" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_rsp_pc, "m_rsp_pc" , string(static_cast<sc_object*>(result)->name()) + ".m_rsp_pc" );
      return result;
    } else {
      if (creationVerboseMode() >= 6) { std::cout << "xtsc_memory_if_vp_mon/xtsc_vp::xtsc_memory_if_vp_mon: " << hierach_name << " created." << std::endl; }
      xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>* result = new xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>(name.c_str());
       cwr_sc_object_registry::inst().addExport(&result->m_request_export, string(static_cast<sc_object*>(result)->name()) + ".m_request_export" );
       cwr_sc_object_registry::inst().addPort(&result->m_request_port, string(static_cast<sc_object*>(result)->name()) + ".m_request_port" );
       cwr_sc_object_registry::inst().addExport(&result->m_respond_export, string(static_cast<sc_object*>(result)->name()) + ".m_respond_export" );
       cwr_sc_object_registry::inst().addPort(&result->m_respond_port, string(static_cast<sc_object*>(result)->name()) + ".m_respond_port" );
       cwr_sc_object_registry::inst().addPort(&result->m_req_count, string(static_cast<sc_object*>(result)->name()) + ".m_req_count" );
       cwr_sc_object_registry::inst().addPort(&result->m_req_tag, string(static_cast<sc_object*>(result)->name()) + ".m_req_tag" );
       cwr_sc_object_registry::inst().addPort(&result->m_req_address8, string(static_cast<sc_object*>(result)->name()) + ".m_req_address8" );
       cwr_sc_object_registry::inst().addPort(&result->m_req_data, string(static_cast<sc_object*>(result)->name()) + ".m_req_data" );
       cwr_sc_object_registry::inst().addPort(&result->m_req_size8, string(static_cast<sc_object*>(result)->name()) + ".m_req_size8" );
       cwr_sc_object_registry::inst().addPort(&result->m_req_route_id, string(static_cast<sc_object*>(result)->name()) + ".m_req_route_id" );
       cwr_sc_object_registry::inst().addPort(&result->m_req_type, string(static_cast<sc_object*>(result)->name()) + ".m_req_type" );
       cwr_sc_object_registry::inst().addPort(&result->m_req_num_transfers, string(static_cast<sc_object*>(result)->name()) + ".m_req_num_transfers" );
       cwr_sc_object_registry::inst().addPort(&result->m_req_byte_enables, string(static_cast<sc_object*>(result)->name()) + ".m_req_byte_enables" );
       cwr_sc_object_registry::inst().addPort(&result->m_req_id, string(static_cast<sc_object*>(result)->name()) + ".m_req_id" );
       cwr_sc_object_registry::inst().addPort(&result->m_req_priority, string(static_cast<sc_object*>(result)->name()) + ".m_req_priority" );
       cwr_sc_object_registry::inst().addPort(&result->m_req_last_transfer, string(static_cast<sc_object*>(result)->name()) + ".m_req_last_transfer" );
       cwr_sc_object_registry::inst().addPort(&result->m_req_pc, string(static_cast<sc_object*>(result)->name()) + ".m_req_pc" );
       cwr_sc_object_registry::inst().addPort(&result->m_rsp_count, string(static_cast<sc_object*>(result)->name()) + ".m_rsp_count" );
       cwr_sc_object_registry::inst().addPort(&result->m_rsp_ok_count, string(static_cast<sc_object*>(result)->name()) + ".m_rsp_ok_count" );
       cwr_sc_object_registry::inst().addPort(&result->m_rsp_nacc_count, string(static_cast<sc_object*>(result)->name()) + ".m_rsp_nacc_count" );
       cwr_sc_object_registry::inst().addPort(&result->m_rsp_tag, string(static_cast<sc_object*>(result)->name()) + ".m_rsp_tag" );
       cwr_sc_object_registry::inst().addPort(&result->m_rsp_address8, string(static_cast<sc_object*>(result)->name()) + ".m_rsp_address8" );
       cwr_sc_object_registry::inst().addPort(&result->m_rsp_data, string(static_cast<sc_object*>(result)->name()) + ".m_rsp_data" );
       cwr_sc_object_registry::inst().addPort(&result->m_rsp_size8, string(static_cast<sc_object*>(result)->name()) + ".m_rsp_size8" );
       cwr_sc_object_registry::inst().addPort(&result->m_rsp_route_id, string(static_cast<sc_object*>(result)->name()) + ".m_rsp_route_id" );
       cwr_sc_object_registry::inst().addPort(&result->m_rsp_status, string(static_cast<sc_object*>(result)->name()) + ".m_rsp_status" );
       cwr_sc_object_registry::inst().addPort(&result->m_rsp_id, string(static_cast<sc_object*>(result)->name()) + ".m_rsp_id" );
       cwr_sc_object_registry::inst().addPort(&result->m_rsp_priority, string(static_cast<sc_object*>(result)->name()) + ".m_rsp_priority" );
       cwr_sc_object_registry::inst().addPort(&result->m_rsp_last_transfer, string(static_cast<sc_object*>(result)->name()) + ".m_rsp_last_transfer" );
       cwr_sc_object_registry::inst().addPort(&result->m_rsp_pc, string(static_cast<sc_object*>(result)->name()) + ".m_rsp_pc" );
      return result;
    }
  }
};

template<int DATA_WIDTH>
class xtsc_vp__xtsc_memory_if_vp_mon1Creator : public ScObjectCreatorBase
{
public:
  static unsigned int creationVerboseMode() {
    const char * const env_var_val = ::getenv("SNPS_SLS_DYNAMIC_CREATION_VERBOSE");
    return env_var_val ? (::atoi(env_var_val)) : 3;
  }
  sc_object* create ( const string& name ) {
    string hierach_name = getHierarchicalName(name);
    bool big_endian = scml_property_registry::inst().getBoolProperty(
	      scml_property_registry::CONSTRUCTOR, hierach_name, "big_endian");

    string vcd_name = scml_property_registry::inst().getStringProperty(
	      scml_property_registry::CONSTRUCTOR, hierach_name, "vcd_name");

    bool connected = scml_property_registry::inst().getBoolProperty(
	      scml_property_registry::CONSTRUCTOR, hierach_name, "connected");

    if (scml_property_registry::inst().hasProperty(scml_property_registry::MODULE, scml_property_registry::BOOL, hierach_name, "runtime_disabled") && 
        scml_property_registry::inst().getBoolProperty(scml_property_registry::MODULE, hierach_name, "runtime_disabled")) {
      sc_module_name n(name.c_str());
      if (creationVerboseMode() >= 6) { std::cout << "xtsc_memory_if_vp_mon/xtsc_vp::xtsc_memory_if_vp_mon: STUB for " << hierach_name << " created." << std::endl; }
      conf::stub *result = new conf::stub(n);
       cwr_sc_object_registry::inst().addExport(new xtsc::xtsc_request_if_vp_stub(std::string("m_request_export" ).c_str()), string(static_cast<sc_object*>(result)->name()) + ".m_request_export" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_request_port, "m_request_port" , string(static_cast<sc_object*>(result)->name()) + ".m_request_port" );
       cwr_sc_object_registry::inst().addExport(new xtsc::xtsc_respond_if_vp_stub(std::string("m_respond_export" ).c_str()), string(static_cast<sc_object*>(result)->name()) + ".m_respond_export" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_respond_port, "m_respond_port" , string(static_cast<sc_object*>(result)->name()) + ".m_respond_port" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_req_count, "m_req_count" , string(static_cast<sc_object*>(result)->name()) + ".m_req_count" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_req_tag, "m_req_tag" , string(static_cast<sc_object*>(result)->name()) + ".m_req_tag" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_req_address8, "m_req_address8" , string(static_cast<sc_object*>(result)->name()) + ".m_req_address8" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_req_data, "m_req_data" , string(static_cast<sc_object*>(result)->name()) + ".m_req_data" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_req_size8, "m_req_size8" , string(static_cast<sc_object*>(result)->name()) + ".m_req_size8" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_req_route_id, "m_req_route_id" , string(static_cast<sc_object*>(result)->name()) + ".m_req_route_id" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_req_type, "m_req_type" , string(static_cast<sc_object*>(result)->name()) + ".m_req_type" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_req_num_transfers, "m_req_num_transfers" , string(static_cast<sc_object*>(result)->name()) + ".m_req_num_transfers" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_req_byte_enables, "m_req_byte_enables" , string(static_cast<sc_object*>(result)->name()) + ".m_req_byte_enables" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_req_id, "m_req_id" , string(static_cast<sc_object*>(result)->name()) + ".m_req_id" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_req_priority, "m_req_priority" , string(static_cast<sc_object*>(result)->name()) + ".m_req_priority" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_req_last_transfer, "m_req_last_transfer" , string(static_cast<sc_object*>(result)->name()) + ".m_req_last_transfer" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_req_pc, "m_req_pc" , string(static_cast<sc_object*>(result)->name()) + ".m_req_pc" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_rsp_count, "m_rsp_count" , string(static_cast<sc_object*>(result)->name()) + ".m_rsp_count" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_rsp_ok_count, "m_rsp_ok_count" , string(static_cast<sc_object*>(result)->name()) + ".m_rsp_ok_count" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_rsp_nacc_count, "m_rsp_nacc_count" , string(static_cast<sc_object*>(result)->name()) + ".m_rsp_nacc_count" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_rsp_tag, "m_rsp_tag" , string(static_cast<sc_object*>(result)->name()) + ".m_rsp_tag" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_rsp_address8, "m_rsp_address8" , string(static_cast<sc_object*>(result)->name()) + ".m_rsp_address8" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_rsp_data, "m_rsp_data" , string(static_cast<sc_object*>(result)->name()) + ".m_rsp_data" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_rsp_size8, "m_rsp_size8" , string(static_cast<sc_object*>(result)->name()) + ".m_rsp_size8" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_rsp_route_id, "m_rsp_route_id" , string(static_cast<sc_object*>(result)->name()) + ".m_rsp_route_id" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_rsp_status, "m_rsp_status" , string(static_cast<sc_object*>(result)->name()) + ".m_rsp_status" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_rsp_id, "m_rsp_id" , string(static_cast<sc_object*>(result)->name()) + ".m_rsp_id" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_rsp_priority, "m_rsp_priority" , string(static_cast<sc_object*>(result)->name()) + ".m_rsp_priority" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_rsp_last_transfer, "m_rsp_last_transfer" , string(static_cast<sc_object*>(result)->name()) + ".m_rsp_last_transfer" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>::m_rsp_pc, "m_rsp_pc" , string(static_cast<sc_object*>(result)->name()) + ".m_rsp_pc" );
      return result;
    } else {
      if (creationVerboseMode() >= 6) { std::cout << "xtsc_memory_if_vp_mon/xtsc_vp::xtsc_memory_if_vp_mon: " << hierach_name << " created." << std::endl; }
      xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>* result = new xtsc_vp::xtsc_memory_if_vp_mon<DATA_WIDTH>(name.c_str(), big_endian, vcd_name.c_str(), connected);
       cwr_sc_object_registry::inst().addExport(&result->m_request_export, string(static_cast<sc_object*>(result)->name()) + ".m_request_export" );
       cwr_sc_object_registry::inst().addPort(&result->m_request_port, string(static_cast<sc_object*>(result)->name()) + ".m_request_port" );
       cwr_sc_object_registry::inst().addExport(&result->m_respond_export, string(static_cast<sc_object*>(result)->name()) + ".m_respond_export" );
       cwr_sc_object_registry::inst().addPort(&result->m_respond_port, string(static_cast<sc_object*>(result)->name()) + ".m_respond_port" );
       cwr_sc_object_registry::inst().addPort(&result->m_req_count, string(static_cast<sc_object*>(result)->name()) + ".m_req_count" );
       cwr_sc_object_registry::inst().addPort(&result->m_req_tag, string(static_cast<sc_object*>(result)->name()) + ".m_req_tag" );
       cwr_sc_object_registry::inst().addPort(&result->m_req_address8, string(static_cast<sc_object*>(result)->name()) + ".m_req_address8" );
       cwr_sc_object_registry::inst().addPort(&result->m_req_data, string(static_cast<sc_object*>(result)->name()) + ".m_req_data" );
       cwr_sc_object_registry::inst().addPort(&result->m_req_size8, string(static_cast<sc_object*>(result)->name()) + ".m_req_size8" );
       cwr_sc_object_registry::inst().addPort(&result->m_req_route_id, string(static_cast<sc_object*>(result)->name()) + ".m_req_route_id" );
       cwr_sc_object_registry::inst().addPort(&result->m_req_type, string(static_cast<sc_object*>(result)->name()) + ".m_req_type" );
       cwr_sc_object_registry::inst().addPort(&result->m_req_num_transfers, string(static_cast<sc_object*>(result)->name()) + ".m_req_num_transfers" );
       cwr_sc_object_registry::inst().addPort(&result->m_req_byte_enables, string(static_cast<sc_object*>(result)->name()) + ".m_req_byte_enables" );
       cwr_sc_object_registry::inst().addPort(&result->m_req_id, string(static_cast<sc_object*>(result)->name()) + ".m_req_id" );
       cwr_sc_object_registry::inst().addPort(&result->m_req_priority, string(static_cast<sc_object*>(result)->name()) + ".m_req_priority" );
       cwr_sc_object_registry::inst().addPort(&result->m_req_last_transfer, string(static_cast<sc_object*>(result)->name()) + ".m_req_last_transfer" );
       cwr_sc_object_registry::inst().addPort(&result->m_req_pc, string(static_cast<sc_object*>(result)->name()) + ".m_req_pc" );
       cwr_sc_object_registry::inst().addPort(&result->m_rsp_count, string(static_cast<sc_object*>(result)->name()) + ".m_rsp_count" );
       cwr_sc_object_registry::inst().addPort(&result->m_rsp_ok_count, string(static_cast<sc_object*>(result)->name()) + ".m_rsp_ok_count" );
       cwr_sc_object_registry::inst().addPort(&result->m_rsp_nacc_count, string(static_cast<sc_object*>(result)->name()) + ".m_rsp_nacc_count" );
       cwr_sc_object_registry::inst().addPort(&result->m_rsp_tag, string(static_cast<sc_object*>(result)->name()) + ".m_rsp_tag" );
       cwr_sc_object_registry::inst().addPort(&result->m_rsp_address8, string(static_cast<sc_object*>(result)->name()) + ".m_rsp_address8" );
       cwr_sc_object_registry::inst().addPort(&result->m_rsp_data, string(static_cast<sc_object*>(result)->name()) + ".m_rsp_data" );
       cwr_sc_object_registry::inst().addPort(&result->m_rsp_size8, string(static_cast<sc_object*>(result)->name()) + ".m_rsp_size8" );
       cwr_sc_object_registry::inst().addPort(&result->m_rsp_route_id, string(static_cast<sc_object*>(result)->name()) + ".m_rsp_route_id" );
       cwr_sc_object_registry::inst().addPort(&result->m_rsp_status, string(static_cast<sc_object*>(result)->name()) + ".m_rsp_status" );
       cwr_sc_object_registry::inst().addPort(&result->m_rsp_id, string(static_cast<sc_object*>(result)->name()) + ".m_rsp_id" );
       cwr_sc_object_registry::inst().addPort(&result->m_rsp_priority, string(static_cast<sc_object*>(result)->name()) + ".m_rsp_priority" );
       cwr_sc_object_registry::inst().addPort(&result->m_rsp_last_transfer, string(static_cast<sc_object*>(result)->name()) + ".m_rsp_last_transfer" );
       cwr_sc_object_registry::inst().addPort(&result->m_rsp_pc, string(static_cast<sc_object*>(result)->name()) + ".m_rsp_pc" );
      return result;
    }
  }
};



struct DllAdapter {
  DllAdapter() {
    ScExportFactory::inst().addCreator ("sc_core::sc_export<xtsc::xtsc_request_if>", new ScExportCreator<sc_core::sc_export<xtsc::xtsc_request_if> >());
    ScExportFactory::inst().addCreator ("sc_core::sc_export<xtsc::xtsc_respond_if>", new ScExportCreator<sc_core::sc_export<xtsc::xtsc_respond_if> >());
    ScExportFactory::inst().addCreator ("sc_export<xtsc::xtsc_request_if>", new ScExportCreator<sc_export<xtsc::xtsc_request_if> >());
    ScExportFactory::inst().addCreator ("sc_export<xtsc::xtsc_respond_if>", new ScExportCreator<sc_export<xtsc::xtsc_respond_if> >());
    ScExportFactory::inst().addCreator ("xtsc::xtsc_request_if_vp_stub", new ScExportCreator<xtsc::xtsc_request_if_vp_stub>());
    ScExportFactory::inst().addCreator ("xtsc::xtsc_respond_if_vp_stub", new ScExportCreator<xtsc::xtsc_respond_if_vp_stub>());
    ScObjectFactory::inst().addCreator ("sc_signal<bool>", new ScPrimChannelCreator<sc_signal<bool> >());
    ScObjectFactory::inst().addCreator ("sc_signal<sc_biguint<128> >", new ScPrimChannelCreator<sc_signal<sc_biguint<128> > >());
    ScObjectFactory::inst().addCreator ("sc_signal<sc_biguint<256> >", new ScPrimChannelCreator<sc_signal<sc_biguint<256> > >());
    ScObjectFactory::inst().addCreator ("sc_signal<sc_biguint<32> >", new ScPrimChannelCreator<sc_signal<sc_biguint<32> > >());
    ScObjectFactory::inst().addCreator ("sc_signal<sc_biguint<512> >", new ScPrimChannelCreator<sc_signal<sc_biguint<512> > >());
    ScObjectFactory::inst().addCreator ("sc_signal<sc_biguint<64> >", new ScPrimChannelCreator<sc_signal<sc_biguint<64> > >());
    ScObjectFactory::inst().addCreator ("sc_signal<unsigned char>", new ScPrimChannelCreator<sc_signal<unsigned char> >());
    ScObjectFactory::inst().addCreator ("sc_signal<unsigned int>", new ScPrimChannelCreator<sc_signal<unsigned int> >());
    ScObjectFactory::inst().addCreator ("sc_signal<unsigned long long>", new ScPrimChannelCreator<sc_signal<unsigned long long> >());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_memory_if_vp_mon0<128>", new xtsc_vp__xtsc_memory_if_vp_mon0Creator<128>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_memory_if_vp_mon0<256>", new xtsc_vp__xtsc_memory_if_vp_mon0Creator<256>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_memory_if_vp_mon0<32>", new xtsc_vp__xtsc_memory_if_vp_mon0Creator<32>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_memory_if_vp_mon0<512>", new xtsc_vp__xtsc_memory_if_vp_mon0Creator<512>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_memory_if_vp_mon0<64>", new xtsc_vp__xtsc_memory_if_vp_mon0Creator<64>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_memory_if_vp_mon1<128>", new xtsc_vp__xtsc_memory_if_vp_mon1Creator<128>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_memory_if_vp_mon1<256>", new xtsc_vp__xtsc_memory_if_vp_mon1Creator<256>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_memory_if_vp_mon1<32>", new xtsc_vp__xtsc_memory_if_vp_mon1Creator<32>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_memory_if_vp_mon1<512>", new xtsc_vp__xtsc_memory_if_vp_mon1Creator<512>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_memory_if_vp_mon1<64>", new xtsc_vp__xtsc_memory_if_vp_mon1Creator<64>());
    ScPortFactory::inst().addCreator ("sc_core::sc_port<xtsc::xtsc_request_if>", new ScPortCreator<sc_core::sc_port<xtsc::xtsc_request_if> >());
    ScPortFactory::inst().addCreator ("sc_core::sc_port<xtsc::xtsc_respond_if>", new ScPortCreator<sc_core::sc_port<xtsc::xtsc_respond_if> >());
    ScPortFactory::inst().addCreator ("sc_inout<bool>", new ScPortCreator<sc_inout<bool> >());
    ScPortFactory::inst().addCreator ("sc_inout<sc_biguint<128> >", new ScPortCreator<sc_inout<sc_biguint<128> > >());
    ScPortFactory::inst().addCreator ("sc_inout<sc_biguint<256> >", new ScPortCreator<sc_inout<sc_biguint<256> > >());
    ScPortFactory::inst().addCreator ("sc_inout<sc_biguint<32> >", new ScPortCreator<sc_inout<sc_biguint<32> > >());
    ScPortFactory::inst().addCreator ("sc_inout<sc_biguint<512> >", new ScPortCreator<sc_inout<sc_biguint<512> > >());
    ScPortFactory::inst().addCreator ("sc_inout<sc_biguint<64> >", new ScPortCreator<sc_inout<sc_biguint<64> > >());
    ScPortFactory::inst().addCreator ("sc_inout<unsigned char>", new ScPortCreator<sc_inout<unsigned char> >());
    ScPortFactory::inst().addCreator ("sc_inout<unsigned int>", new ScPortCreator<sc_inout<unsigned int> >());
    ScPortFactory::inst().addCreator ("sc_inout<unsigned long long>", new ScPortCreator<sc_inout<unsigned long long> >());
    ScPortFactory::inst().addCreator ("sc_port<xtsc::xtsc_request_if>", new ScPortCreator<sc_port<xtsc::xtsc_request_if> >());
    ScPortFactory::inst().addCreator ("sc_port<xtsc::xtsc_respond_if>", new ScPortCreator<sc_port<xtsc::xtsc_respond_if> >());
    if (::getenv("SNPS_SLS_DYNAMIC_LOADER_VERBOSE")) { std::cout << "xtsc_memory_if_vp_mon/xtsc_vp::xtsc_memory_if_vp_mon loaded." << std::endl; }
  }
  ~DllAdapter() {
  }
  static DllAdapter sInstance;
};

DllAdapter DllAdapter::sInstance;

}
