#include "systemc.h"
#include "xtsc_vp/xtsc_if_vp_stubs.h"
#include "xtsc_vp/xtsc_arbiter_vp.h"
#include "cassert"
#include "cwr_dynamic_loader.h"
#include "cwr_sc_dynamic_stubs.h"
#include "cwr_sc_hierarch_module.h"
#include "cwr_sc_object_creator.h"
#include "scmlinc/scml_abstraction_level_switch.h"
#include "scmlinc/scml_property_registry.h"

namespace xtsc_arbiter_vp_xtsc_vp__xtsc_arbiter_vp_FastBuild {

using namespace conf;
using namespace std;

template<int NUM_MASTERS, int DATA_WIDTH>
class xtsc_vp__xtsc_arbiter_vp0Creator : public ScObjectCreatorBase
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
      if (creationVerboseMode() >= 6) { std::cout << "xtsc_arbiter_vp/xtsc_vp::xtsc_arbiter_vp: STUB for " << hierach_name << " created." << std::endl; }
      conf::stub *result = new conf::stub(n);
      {
       unsigned pctDynamicPortArraySize = 0;
       {
        pctDynamicPortArraySize = NUM_MASTERS;
       }
       // coverity[dead_error_condition]
       for (unsigned port_array_index = 0; port_array_index != pctDynamicPortArraySize; ++port_array_index) {
        std::ostringstream port_array_index_tmp;
        port_array_index_tmp << "[" << port_array_index << "]"; 
        cwr_sc_object_registry::inst().addExport(new xtsc::xtsc_request_if_vp_stub(std::string("m_request_exports" + port_array_index_tmp.str()).c_str()), string(static_cast<sc_object*>(result)->name()) + ".m_request_exports" + port_array_index_tmp.str());
      }
     }
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_arbiter_vp<NUM_MASTERS, DATA_WIDTH>::m_request_port, "m_request_port" , string(static_cast<sc_object*>(result)->name()) + ".m_request_port" );
       cwr_sc_object_registry::inst().addExport(new xtsc::xtsc_respond_if_vp_stub(std::string("m_respond_export" ).c_str()), string(static_cast<sc_object*>(result)->name()) + ".m_respond_export" );
      {
       unsigned pctDynamicPortArraySize = 0;
       {
        pctDynamicPortArraySize = NUM_MASTERS;
       }
       // coverity[dead_error_condition]
       for (unsigned port_array_index = 0; port_array_index != pctDynamicPortArraySize; ++port_array_index) {
        std::ostringstream port_array_index_tmp;
        port_array_index_tmp << "[" << port_array_index << "]"; 
        conf::stub_port_registrator<NUM_MASTERS>::register_stub_port(&xtsc_vp::xtsc_arbiter_vp<NUM_MASTERS, DATA_WIDTH>::m_respond_ports, "m_respond_ports" + port_array_index_tmp.str(), string(static_cast<sc_object*>(result)->name()) + ".m_respond_ports" + port_array_index_tmp.str());
      }
     }
      return result;
    } else {
      if (creationVerboseMode() >= 6) { std::cout << "xtsc_arbiter_vp/xtsc_vp::xtsc_arbiter_vp: " << hierach_name << " created." << std::endl; }
      xtsc_vp::xtsc_arbiter_vp<NUM_MASTERS, DATA_WIDTH>* result = new xtsc_vp::xtsc_arbiter_vp<NUM_MASTERS, DATA_WIDTH>(name.c_str());
      {
       unsigned pctDynamicPortArraySize = 0;
       {
        pctDynamicPortArraySize = NUM_MASTERS;
       }
       // coverity[dead_error_condition]
       for (unsigned port_array_index = 0; port_array_index != pctDynamicPortArraySize; ++port_array_index) {
        std::ostringstream port_array_index_tmp;
        port_array_index_tmp << "[" << port_array_index << "]"; 
        cwr_sc_object_registry::inst().addExport(result->m_request_exports[port_array_index], string(static_cast<sc_object*>(result)->name()) + ".m_request_exports" + port_array_index_tmp.str());
      }
     }
       cwr_sc_object_registry::inst().addPort(&result->m_request_port, string(static_cast<sc_object*>(result)->name()) + ".m_request_port" );
       cwr_sc_object_registry::inst().addExport(&result->m_respond_export, string(static_cast<sc_object*>(result)->name()) + ".m_respond_export" );
      {
       unsigned pctDynamicPortArraySize = 0;
       {
        pctDynamicPortArraySize = NUM_MASTERS;
       }
       // coverity[dead_error_condition]
       for (unsigned port_array_index = 0; port_array_index != pctDynamicPortArraySize; ++port_array_index) {
        std::ostringstream port_array_index_tmp;
        port_array_index_tmp << "[" << port_array_index << "]"; 
        cwr_sc_object_registry::inst().addPort(result->m_respond_ports[port_array_index], string(static_cast<sc_object*>(result)->name()) + ".m_respond_ports" + port_array_index_tmp.str());
      }
     }
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
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_arbiter_vp0<1, 128>", new xtsc_vp__xtsc_arbiter_vp0Creator<1, 128>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_arbiter_vp0<1, 256>", new xtsc_vp__xtsc_arbiter_vp0Creator<1, 256>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_arbiter_vp0<1, 32>", new xtsc_vp__xtsc_arbiter_vp0Creator<1, 32>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_arbiter_vp0<1, 512>", new xtsc_vp__xtsc_arbiter_vp0Creator<1, 512>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_arbiter_vp0<1, 64>", new xtsc_vp__xtsc_arbiter_vp0Creator<1, 64>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_arbiter_vp0<2, 128>", new xtsc_vp__xtsc_arbiter_vp0Creator<2, 128>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_arbiter_vp0<2, 256>", new xtsc_vp__xtsc_arbiter_vp0Creator<2, 256>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_arbiter_vp0<2, 32>", new xtsc_vp__xtsc_arbiter_vp0Creator<2, 32>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_arbiter_vp0<2, 512>", new xtsc_vp__xtsc_arbiter_vp0Creator<2, 512>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_arbiter_vp0<2, 64>", new xtsc_vp__xtsc_arbiter_vp0Creator<2, 64>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_arbiter_vp0<3, 128>", new xtsc_vp__xtsc_arbiter_vp0Creator<3, 128>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_arbiter_vp0<3, 256>", new xtsc_vp__xtsc_arbiter_vp0Creator<3, 256>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_arbiter_vp0<3, 32>", new xtsc_vp__xtsc_arbiter_vp0Creator<3, 32>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_arbiter_vp0<3, 512>", new xtsc_vp__xtsc_arbiter_vp0Creator<3, 512>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_arbiter_vp0<3, 64>", new xtsc_vp__xtsc_arbiter_vp0Creator<3, 64>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_arbiter_vp0<4, 128>", new xtsc_vp__xtsc_arbiter_vp0Creator<4, 128>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_arbiter_vp0<4, 256>", new xtsc_vp__xtsc_arbiter_vp0Creator<4, 256>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_arbiter_vp0<4, 32>", new xtsc_vp__xtsc_arbiter_vp0Creator<4, 32>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_arbiter_vp0<4, 512>", new xtsc_vp__xtsc_arbiter_vp0Creator<4, 512>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_arbiter_vp0<4, 64>", new xtsc_vp__xtsc_arbiter_vp0Creator<4, 64>());
    ScPortFactory::inst().addCreator ("sc_core::sc_port<xtsc::xtsc_request_if>", new ScPortCreator<sc_core::sc_port<xtsc::xtsc_request_if> >());
    ScPortFactory::inst().addCreator ("sc_core::sc_port<xtsc::xtsc_respond_if>", new ScPortCreator<sc_core::sc_port<xtsc::xtsc_respond_if> >());
    ScPortFactory::inst().addCreator ("sc_port<xtsc::xtsc_request_if>", new ScPortCreator<sc_port<xtsc::xtsc_request_if> >());
    ScPortFactory::inst().addCreator ("sc_port<xtsc::xtsc_respond_if>", new ScPortCreator<sc_port<xtsc::xtsc_respond_if> >());
    if (::getenv("SNPS_SLS_DYNAMIC_LOADER_VERBOSE")) { std::cout << "xtsc_arbiter_vp/xtsc_vp::xtsc_arbiter_vp loaded." << std::endl; }
  }
  ~DllAdapter() {
  }
  static DllAdapter sInstance;
};

DllAdapter DllAdapter::sInstance;

}
