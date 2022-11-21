#include "systemc.h"
#include "xtsc_vp/xtsc_if_vp_stubs.h"
#include "xtsc_vp/xtsc_pin2tlm_wire_transactor_vp.h"
#include "cassert"
#include "cwr_dynamic_loader.h"
#include "cwr_sc_dynamic_stubs.h"
#include "cwr_sc_hierarch_module.h"
#include "cwr_sc_object_creator.h"
#include "scmlinc/scml_abstraction_level_switch.h"
#include "scmlinc/scml_property_registry.h"

namespace xtsc_pin2tlm_wire_transactor_vp_xtsc_vp__xtsc_pin2tlm_wire_transactor_vp_FastBuild {

using namespace conf;
using namespace std;

template<int W, typename T>
class xtsc_vp__xtsc_pin2tlm_wire_transactor_vp0Creator : public ScObjectCreatorBase
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
      if (creationVerboseMode() >= 6) { std::cout << "xtsc_pin2tlm_wire_transactor_vp/xtsc_vp::xtsc_pin2tlm_wire_transactor_vp: STUB for " << hierach_name << " created." << std::endl; }
      conf::stub *result = new conf::stub(n);
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_pin2tlm_wire_transactor_vp<W, T>::m_sc_port, "m_sc_port" , string(static_cast<sc_object*>(result)->name()) + ".m_sc_port" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_pin2tlm_wire_transactor_vp<W, T>::m_sc_in, "m_sc_in" , string(static_cast<sc_object*>(result)->name()) + ".m_sc_in" );
      return result;
    } else {
      if (creationVerboseMode() >= 6) { std::cout << "xtsc_pin2tlm_wire_transactor_vp/xtsc_vp::xtsc_pin2tlm_wire_transactor_vp: " << hierach_name << " created." << std::endl; }
      xtsc_vp::xtsc_pin2tlm_wire_transactor_vp<W, T>* result = new xtsc_vp::xtsc_pin2tlm_wire_transactor_vp<W, T>(name.c_str());
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
    ScObjectFactory::inst().addCreator ("sc_signal<sc_biguint<128> >", new ScPrimChannelCreator<sc_signal<sc_biguint<128> > >());
    ScObjectFactory::inst().addCreator ("sc_signal<sc_biguint<1> >", new ScPrimChannelCreator<sc_signal<sc_biguint<1> > >());
    ScObjectFactory::inst().addCreator ("sc_signal<sc_biguint<32> >", new ScPrimChannelCreator<sc_signal<sc_biguint<32> > >());
    ScObjectFactory::inst().addCreator ("sc_signal<sc_biguint<50> >", new ScPrimChannelCreator<sc_signal<sc_biguint<50> > >());
    ScObjectFactory::inst().addCreator ("sc_signal<sc_biguint<64> >", new ScPrimChannelCreator<sc_signal<sc_biguint<64> > >());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_pin2tlm_wire_transactor_vp0<1, bool>", new xtsc_vp__xtsc_pin2tlm_wire_transactor_vp0Creator<1, bool>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_pin2tlm_wire_transactor_vp0<1, sc_biguint<1> >", new xtsc_vp__xtsc_pin2tlm_wire_transactor_vp0Creator<1, sc_biguint<1> >());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_pin2tlm_wire_transactor_vp0<128, sc_biguint<128> >", new xtsc_vp__xtsc_pin2tlm_wire_transactor_vp0Creator<128, sc_biguint<128> >());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_pin2tlm_wire_transactor_vp0<32, sc_biguint<32> >", new xtsc_vp__xtsc_pin2tlm_wire_transactor_vp0Creator<32, sc_biguint<32> >());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_pin2tlm_wire_transactor_vp0<50, sc_biguint<50> >", new xtsc_vp__xtsc_pin2tlm_wire_transactor_vp0Creator<50, sc_biguint<50> >());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_pin2tlm_wire_transactor_vp0<64, sc_biguint<64> >", new xtsc_vp__xtsc_pin2tlm_wire_transactor_vp0Creator<64, sc_biguint<64> >());
    ScPortFactory::inst().addCreator ("sc_core::sc_port<xtsc::xtsc_wire_write_if>", new ScPortCreator<sc_core::sc_port<xtsc::xtsc_wire_write_if> >());
    ScPortFactory::inst().addCreator ("sc_inout<bool>", new ScPortCreator<sc_inout<bool> >());
    ScPortFactory::inst().addCreator ("sc_inout<sc_biguint<128> >", new ScPortCreator<sc_inout<sc_biguint<128> > >());
    ScPortFactory::inst().addCreator ("sc_inout<sc_biguint<1> >", new ScPortCreator<sc_inout<sc_biguint<1> > >());
    ScPortFactory::inst().addCreator ("sc_inout<sc_biguint<32> >", new ScPortCreator<sc_inout<sc_biguint<32> > >());
    ScPortFactory::inst().addCreator ("sc_inout<sc_biguint<50> >", new ScPortCreator<sc_inout<sc_biguint<50> > >());
    ScPortFactory::inst().addCreator ("sc_inout<sc_biguint<64> >", new ScPortCreator<sc_inout<sc_biguint<64> > >());
    ScPortFactory::inst().addCreator ("sc_port<xtsc::xtsc_wire_write_if>", new ScPortCreator<sc_port<xtsc::xtsc_wire_write_if> >());
    if (::getenv("SNPS_SLS_DYNAMIC_LOADER_VERBOSE")) { std::cout << "xtsc_pin2tlm_wire_transactor_vp/xtsc_vp::xtsc_pin2tlm_wire_transactor_vp loaded." << std::endl; }
  }
  ~DllAdapter() {
  }
  static DllAdapter sInstance;
};

DllAdapter DllAdapter::sInstance;

}
