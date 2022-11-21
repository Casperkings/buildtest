#include "systemc.h"
#include "xtsc_vp/xtsc_if_vp_stubs.h"
#include "xtsc_vp/xtsc_lookup_vp.h"
#include "cassert"
#include "cwr_dynamic_loader.h"
#include "cwr_sc_dynamic_stubs.h"
#include "cwr_sc_hierarch_module.h"
#include "cwr_sc_object_creator.h"
#include "scmlinc/scml_abstraction_level_switch.h"
#include "scmlinc/scml_property_registry.h"

namespace xtsc_lookup_vp_xtsc_vp__xtsc_lookup_vp_FastBuild {

using namespace conf;
using namespace std;

template<int ADDR_WIDTH, int DATA_WIDTH>
class xtsc_vp__xtsc_lookup_vp0Creator : public ScObjectCreatorBase
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
      if (creationVerboseMode() >= 6) { std::cout << "xtsc_lookup_vp/xtsc_vp::xtsc_lookup_vp: STUB for " << hierach_name << " created." << std::endl; }
      conf::stub *result = new conf::stub(n);
       cwr_sc_object_registry::inst().addExport(new xtsc::xtsc_lookup_if_vp_stub(std::string("m_lookup" ).c_str()), string(static_cast<sc_object*>(result)->name()) + ".m_lookup" );
      return result;
    } else {
      if (creationVerboseMode() >= 6) { std::cout << "xtsc_lookup_vp/xtsc_vp::xtsc_lookup_vp: " << hierach_name << " created." << std::endl; }
      xtsc_vp::xtsc_lookup_vp<ADDR_WIDTH, DATA_WIDTH>* result = new xtsc_vp::xtsc_lookup_vp<ADDR_WIDTH, DATA_WIDTH>(name.c_str());
       cwr_sc_object_registry::inst().addExport(&result->m_lookup, string(static_cast<sc_object*>(result)->name()) + ".m_lookup" );
      return result;
    }
  }
};



struct DllAdapter {
  DllAdapter() {
    ScExportFactory::inst().addCreator ("sc_core::sc_export<xtsc::xtsc_lookup_if>", new ScExportCreator<sc_core::sc_export<xtsc::xtsc_lookup_if> >());
    ScExportFactory::inst().addCreator ("sc_export<xtsc::xtsc_lookup_if>", new ScExportCreator<sc_export<xtsc::xtsc_lookup_if> >());
    ScExportFactory::inst().addCreator ("xtsc::xtsc_lookup_if_vp_stub", new ScExportCreator<xtsc::xtsc_lookup_if_vp_stub>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_lookup_vp0<8, 32>", new xtsc_vp__xtsc_lookup_vp0Creator<8, 32>());
    ScPortFactory::inst().addCreator ("sc_core::sc_port<xtsc::xtsc_lookup_if>", new ScPortCreator<sc_core::sc_port<xtsc::xtsc_lookup_if> >());
    ScPortFactory::inst().addCreator ("sc_port<xtsc::xtsc_lookup_if>", new ScPortCreator<sc_port<xtsc::xtsc_lookup_if> >());
    if (::getenv("SNPS_SLS_DYNAMIC_LOADER_VERBOSE")) { std::cout << "xtsc_lookup_vp/xtsc_vp::xtsc_lookup_vp loaded." << std::endl; }
  }
  ~DllAdapter() {
  }
  static DllAdapter sInstance;
};

DllAdapter DllAdapter::sInstance;

}
