#include "systemc.h"
#include "xtsc_vp/xtsc_if_vp_stubs.h"
#include "xtsc_vp/xtsc_pin2tlm_wire_transactor_vp_RESET.h"
#include "cassert"
#include "cwr_dynamic_loader.h"
#include "cwr_sc_dynamic_stubs.h"
#include "cwr_sc_hierarch_module.h"
#include "cwr_sc_object_creator.h"
#include "scmlinc/scml_abstraction_level_switch.h"
#include "scmlinc/scml_property_registry.h"

namespace xtsc_pin2tlm_wire_transactor_vp_RESET_xtsc_vp__xtsc_pin2tlm_wire_transactor_vp_RESET_FastBuild {

using namespace conf;
using namespace std;


class xtsc_vp__xtsc_pin2tlm_wire_transactor_vp_RESET0Creator : public ScObjectCreatorBase
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
      if (creationVerboseMode() >= 6) { std::cout << "xtsc_pin2tlm_wire_transactor_vp_RESET/xtsc_vp::xtsc_pin2tlm_wire_transactor_vp_RESET: STUB for " << hierach_name << " created." << std::endl; }
      conf::stub *result = new conf::stub(n);
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_pin2tlm_wire_transactor_vp_RESET::m_sc_port, "m_sc_port" , string(static_cast<sc_object*>(result)->name()) + ".m_sc_port" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_pin2tlm_wire_transactor_vp_RESET::m_sc_in, "m_sc_in" , string(static_cast<sc_object*>(result)->name()) + ".m_sc_in" );
      return result;
    } else {
      if (creationVerboseMode() >= 6) { std::cout << "xtsc_pin2tlm_wire_transactor_vp_RESET/xtsc_vp::xtsc_pin2tlm_wire_transactor_vp_RESET: " << hierach_name << " created." << std::endl; }
      xtsc_vp::xtsc_pin2tlm_wire_transactor_vp_RESET* result = new xtsc_vp::xtsc_pin2tlm_wire_transactor_vp_RESET(name.c_str());
       cwr_sc_object_registry::inst().addPort(&result->m_sc_port, string(static_cast<sc_object*>(result)->name()) + ".m_sc_port" );
       cwr_sc_object_registry::inst().addPort(&result->m_sc_in, string(static_cast<sc_object*>(result)->name()) + ".m_sc_in" );
      return result;
    }
  }
};



struct DllAdapter {
  DllAdapter() {
    ScExportFactory::inst().addCreator ("sc_core::sc_export<xtsc::xtsc_wire_write_if>", new ScExportCreator<sc_core::sc_export<xtsc::xtsc_wire_write_if> >());
    ScExportFactory::inst().addCreator ("sc_export<xtsc::xtsc_wire_write_if>", new ScExportCreator<sc_export<xtsc::xtsc_wire_write_if> >());
    ScExportFactory::inst().addCreator ("xtsc::xtsc_wire_write_if_vp_stub", new ScExportCreator<xtsc::xtsc_wire_write_if_vp_stub>());
    ScObjectFactory::inst().addCreator ("sc_signal<bool>", new ScPrimChannelCreator<sc_signal<bool> >());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_pin2tlm_wire_transactor_vp_RESET0", new xtsc_vp__xtsc_pin2tlm_wire_transactor_vp_RESET0Creator());
    ScPortFactory::inst().addCreator ("sc_core::sc_port<xtsc::xtsc_wire_write_if>", new ScPortCreator<sc_core::sc_port<xtsc::xtsc_wire_write_if> >());
    ScPortFactory::inst().addCreator ("sc_in<bool>", new ScPortCreator<sc_in<bool> >());
    ScPortFactory::inst().addCreator ("sc_inout<bool>", new ScPortCreator<sc_inout<bool> >());
    ScPortFactory::inst().addCreator ("sc_out<bool>", new ScPortCreator<sc_out<bool> >());
    ScPortFactory::inst().addCreator ("sc_port<xtsc::xtsc_wire_write_if>", new ScPortCreator<sc_port<xtsc::xtsc_wire_write_if> >());
    if (::getenv("SNPS_SLS_DYNAMIC_LOADER_VERBOSE")) { std::cout << "xtsc_pin2tlm_wire_transactor_vp_RESET/xtsc_vp::xtsc_pin2tlm_wire_transactor_vp_RESET loaded." << std::endl; }
  }
  ~DllAdapter() {
  }
  static DllAdapter sInstance;
};

DllAdapter DllAdapter::sInstance;

}
