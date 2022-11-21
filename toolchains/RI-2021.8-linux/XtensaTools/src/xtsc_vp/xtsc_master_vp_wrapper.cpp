#include "systemc.h"
#include "xtsc_vp/xtsc_if_vp_stubs.h"
#include "xtsc_vp/xtsc_master_vp.h"
#include "cassert"
#include "cwr_dynamic_loader.h"
#include "cwr_sc_dynamic_stubs.h"
#include "cwr_sc_hierarch_module.h"
#include "cwr_sc_object_creator.h"
#include "scmlinc/scml_abstraction_level_switch.h"
#include "scmlinc/scml_property_registry.h"

namespace xtsc_master_vp_xtsc_vp__xtsc_master_vp_FastBuild {

using namespace conf;
using namespace std;

template<int DATA_WIDTH>
class xtsc_vp__xtsc_master_vp0Creator : public ScObjectCreatorBase
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
      if (creationVerboseMode() >= 6) { std::cout << "xtsc_master_vp/xtsc_vp::xtsc_master_vp: STUB for " << hierach_name << " created." << std::endl; }
      conf::stub *result = new conf::stub(n);
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_master_vp<DATA_WIDTH>::m_request_port, "m_request_port" , string(static_cast<sc_object*>(result)->name()) + ".m_request_port" );
       cwr_sc_object_registry::inst().addExport(new xtsc::xtsc_respond_if_vp_stub(std::string("m_respond_export" ).c_str()), string(static_cast<sc_object*>(result)->name()) + ".m_respond_export" );
       cwr_sc_object_registry::inst().addExport(new xtsc::xtsc_wire_write_if_vp_stub(std::string("m_control" ).c_str()), string(static_cast<sc_object*>(result)->name()) + ".m_control" );
      return result;
    } else {
      if (creationVerboseMode() >= 6) { std::cout << "xtsc_master_vp/xtsc_vp::xtsc_master_vp: " << hierach_name << " created." << std::endl; }
      xtsc_vp::xtsc_master_vp<DATA_WIDTH>* result = new xtsc_vp::xtsc_master_vp<DATA_WIDTH>(name.c_str());
       cwr_sc_object_registry::inst().addPort(&result->m_request_port, string(static_cast<sc_object*>(result)->name()) + ".m_request_port" );
       cwr_sc_object_registry::inst().addExport(&result->m_respond_export, string(static_cast<sc_object*>(result)->name()) + ".m_respond_export" );
       cwr_sc_object_registry::inst().addExport(&result->m_control, string(static_cast<sc_object*>(result)->name()) + ".m_control" );
      return result;
    }
  }
};



struct DllAdapter {
  DllAdapter() {
    ScExportFactory::inst().addCreator ("sc_core::sc_export<xtsc::xtsc_request_if>", new ScExportCreator<sc_core::sc_export<xtsc::xtsc_request_if> >());
    ScExportFactory::inst().addCreator ("sc_core::sc_export<xtsc::xtsc_respond_if>", new ScExportCreator<sc_core::sc_export<xtsc::xtsc_respond_if> >());
    ScExportFactory::inst().addCreator ("sc_core::sc_export<xtsc::xtsc_wire_write_if>", new ScExportCreator<sc_core::sc_export<xtsc::xtsc_wire_write_if> >());
    ScExportFactory::inst().addCreator ("sc_export<xtsc::xtsc_request_if>", new ScExportCreator<sc_export<xtsc::xtsc_request_if> >());
    ScExportFactory::inst().addCreator ("sc_export<xtsc::xtsc_respond_if>", new ScExportCreator<sc_export<xtsc::xtsc_respond_if> >());
    ScExportFactory::inst().addCreator ("sc_export<xtsc::xtsc_wire_write_if>", new ScExportCreator<sc_export<xtsc::xtsc_wire_write_if> >());
    ScExportFactory::inst().addCreator ("xtsc::xtsc_request_if_vp_stub", new ScExportCreator<xtsc::xtsc_request_if_vp_stub>());
    ScExportFactory::inst().addCreator ("xtsc::xtsc_respond_if_vp_stub", new ScExportCreator<xtsc::xtsc_respond_if_vp_stub>());
    ScExportFactory::inst().addCreator ("xtsc::xtsc_wire_write_if_vp_stub", new ScExportCreator<xtsc::xtsc_wire_write_if_vp_stub>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_master_vp0<128>", new xtsc_vp__xtsc_master_vp0Creator<128>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_master_vp0<256>", new xtsc_vp__xtsc_master_vp0Creator<256>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_master_vp0<32>", new xtsc_vp__xtsc_master_vp0Creator<32>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_master_vp0<512>", new xtsc_vp__xtsc_master_vp0Creator<512>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_master_vp0<64>", new xtsc_vp__xtsc_master_vp0Creator<64>());
    ScPortFactory::inst().addCreator ("sc_core::sc_port<xtsc::xtsc_request_if>", new ScPortCreator<sc_core::sc_port<xtsc::xtsc_request_if> >());
    ScPortFactory::inst().addCreator ("sc_core::sc_port<xtsc::xtsc_respond_if>", new ScPortCreator<sc_core::sc_port<xtsc::xtsc_respond_if> >());
    ScPortFactory::inst().addCreator ("sc_core::sc_port<xtsc::xtsc_wire_write_if>", new ScPortCreator<sc_core::sc_port<xtsc::xtsc_wire_write_if> >());
    ScPortFactory::inst().addCreator ("sc_port<xtsc::xtsc_request_if>", new ScPortCreator<sc_port<xtsc::xtsc_request_if> >());
    ScPortFactory::inst().addCreator ("sc_port<xtsc::xtsc_respond_if>", new ScPortCreator<sc_port<xtsc::xtsc_respond_if> >());
    ScPortFactory::inst().addCreator ("sc_port<xtsc::xtsc_wire_write_if>", new ScPortCreator<sc_port<xtsc::xtsc_wire_write_if> >());
    if (::getenv("SNPS_SLS_DYNAMIC_LOADER_VERBOSE")) { std::cout << "xtsc_master_vp/xtsc_vp::xtsc_master_vp loaded." << std::endl; }
  }
  ~DllAdapter() {
  }
  static DllAdapter sInstance;
};

DllAdapter DllAdapter::sInstance;

}
