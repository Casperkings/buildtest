#include "systemc.h"
#include "xtsc_vp/xtsc_if_vp_stubs.h"
#include "xtsc_vp/xtsc_tlm2pin_wire_transactor_vp_CLOCK.h"
#include "cassert"
#include "cwr_dynamic_loader.h"
#include "cwr_sc_dynamic_stubs.h"
#include "cwr_sc_hierarch_module.h"
#include "cwr_sc_object_creator.h"
#include "scmlinc/scml_abstraction_level_switch.h"
#include "scmlinc/scml_property_registry.h"

namespace xtsc_tlm2pin_wire_transactor_vp_CLOCK_xtsc_vp__xtsc_tlm2pin_wire_transactor_vp_CLOCK_FastBuild {

using namespace conf;
using namespace std;


class xtsc_vp__xtsc_tlm2pin_wire_transactor_vp_CLOCK0Creator : public ScObjectCreatorBase
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
      if (creationVerboseMode() >= 6) { std::cout << "xtsc_tlm2pin_wire_transactor_vp_CLOCK/xtsc_vp::xtsc_tlm2pin_wire_transactor_vp_CLOCK: STUB for " << hierach_name << " created." << std::endl; }
      conf::stub *result = new conf::stub(n);
       cwr_sc_object_registry::inst().addExport(new xtsc::xtsc_wire_write_if_vp_stub(std::string("m_sc_export" ).c_str()), string(static_cast<sc_object*>(result)->name()) + ".m_sc_export" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_tlm2pin_wire_transactor_vp_CLOCK::m_sc_out, "m_sc_out" , string(static_cast<sc_object*>(result)->name()) + ".m_sc_out" );
       std::cerr << "WARNING: Could not generate stub interface for disabled component interface "
                 << string(static_cast<sc_object*>(result)->name()) + ".xtsc_wire_write_if" << std::endl;
      return result;
    } else {
      if (creationVerboseMode() >= 6) { std::cout << "xtsc_tlm2pin_wire_transactor_vp_CLOCK/xtsc_vp::xtsc_tlm2pin_wire_transactor_vp_CLOCK: " << hierach_name << " created." << std::endl; }
      xtsc_vp::xtsc_tlm2pin_wire_transactor_vp_CLOCK* result = new xtsc_vp::xtsc_tlm2pin_wire_transactor_vp_CLOCK(name.c_str());
       cwr_sc_object_registry::inst().addExport(&result->m_sc_export, string(static_cast<sc_object*>(result)->name()) + ".m_sc_export" );
       cwr_sc_object_registry::inst().addPort(&result->m_sc_out, string(static_cast<sc_object*>(result)->name()) + ".m_sc_out" );
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
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_tlm2pin_wire_transactor_vp_CLOCK0", new xtsc_vp__xtsc_tlm2pin_wire_transactor_vp_CLOCK0Creator());
    ScPortFactory::inst().addCreator ("sc_core::sc_port<xtsc::xtsc_wire_write_if>", new ScPortCreator<sc_core::sc_port<xtsc::xtsc_wire_write_if> >());
    ScPortFactory::inst().addCreator ("sc_in<bool>", new ScPortCreator<sc_in<bool> >());
    ScPortFactory::inst().addCreator ("sc_inout<bool>", new ScPortCreator<sc_inout<bool> >());
    ScPortFactory::inst().addCreator ("sc_out<bool>", new ScPortCreator<sc_out<bool> >());
    ScPortFactory::inst().addCreator ("sc_port<xtsc::xtsc_wire_write_if>", new ScPortCreator<sc_port<xtsc::xtsc_wire_write_if> >());
    if (::getenv("SNPS_SLS_DYNAMIC_LOADER_VERBOSE")) { std::cout << "xtsc_tlm2pin_wire_transactor_vp_CLOCK/xtsc_vp::xtsc_tlm2pin_wire_transactor_vp_CLOCK loaded." << std::endl; }
  }
  ~DllAdapter() {
  }
  static DllAdapter sInstance;
};

DllAdapter DllAdapter::sInstance;

}
