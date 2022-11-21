#include "systemc.h"
#include "xtsc_vp/xtsc_if_vp_stubs.h"
#include "xtsc_vp/xtsc_clock_vp.h"
#include "cassert"
#include "cwr_dynamic_loader.h"
#include "cwr_sc_dynamic_stubs.h"
#include "cwr_sc_hierarch_module.h"
#include "cwr_sc_object_creator.h"
#include "scmlinc/scml_abstraction_level_switch.h"
#include "scmlinc/scml_property_registry.h"

namespace xtsc_clock_vp_xtsc_vp__xtsc_clock_vp_FastBuild {

using namespace conf;
using namespace std;


class xtsc_vp__xtsc_clock_vp0Creator : public ScObjectCreatorBase
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
      if (creationVerboseMode() >= 6) { std::cout << "xtsc_clock_vp/xtsc_vp::xtsc_clock_vp: STUB for " << hierach_name << " created." << std::endl; }
      conf::stub *result = new conf::stub(n);
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_clock_vp::m_clk, "m_clk" , string(static_cast<sc_object*>(result)->name()) + ".m_clk" );
      return result;
    } else {
      if (creationVerboseMode() >= 6) { std::cout << "xtsc_clock_vp/xtsc_vp::xtsc_clock_vp: " << hierach_name << " created." << std::endl; }
      xtsc_vp::xtsc_clock_vp* result = new xtsc_vp::xtsc_clock_vp(name.c_str());
       cwr_sc_object_registry::inst().addPort(&result->m_clk, string(static_cast<sc_object*>(result)->name()) + ".m_clk" );
      return result;
    }
  }
};



struct DllAdapter {
  DllAdapter() {
    ScObjectFactory::inst().addCreator ("sc_signal<bool>", new ScPrimChannelCreator<sc_signal<bool> >());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_clock_vp0", new xtsc_vp__xtsc_clock_vp0Creator());
    ScPortFactory::inst().addCreator ("sc_in<bool>", new ScPortCreator<sc_in<bool> >());
    ScPortFactory::inst().addCreator ("sc_inout<bool>", new ScPortCreator<sc_inout<bool> >());
    ScPortFactory::inst().addCreator ("sc_out<bool>", new ScPortCreator<sc_out<bool> >());
    if (::getenv("SNPS_SLS_DYNAMIC_LOADER_VERBOSE")) { std::cout << "xtsc_clock_vp/xtsc_vp::xtsc_clock_vp loaded." << std::endl; }
  }
  ~DllAdapter() {
  }
  static DllAdapter sInstance;
};

DllAdapter DllAdapter::sInstance;

}
