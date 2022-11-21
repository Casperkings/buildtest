#include "systemc.h"
#include "xtsc_vp/xtsc_if_vp_stubs.h"
#include "xtsc_vp/xtsc_lookup_if_vp_mon.h"
#include "cassert"
#include "cwr_dynamic_loader.h"
#include "cwr_sc_dynamic_stubs.h"
#include "cwr_sc_hierarch_module.h"
#include "cwr_sc_object_creator.h"
#include "scmlinc/scml_abstraction_level_switch.h"
#include "scmlinc/scml_property_registry.h"

namespace xtsc_lookup_if_vp_mon_xtsc_vp__xtsc_lookup_if_vp_mon_FastBuild {

using namespace conf;
using namespace std;

template<int ADDR_WIDTH, int DATA_WIDTH>
class xtsc_vp__xtsc_lookup_if_vp_mon0Creator : public ScObjectCreatorBase
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
      if (creationVerboseMode() >= 6) { std::cout << "xtsc_lookup_if_vp_mon/xtsc_vp::xtsc_lookup_if_vp_mon: STUB for " << hierach_name << " created." << std::endl; }
      conf::stub *result = new conf::stub(n);
       cwr_sc_object_registry::inst().addExport(new xtsc::xtsc_lookup_if_vp_stub(std::string("m_lookup_export" ).c_str()), string(static_cast<sc_object*>(result)->name()) + ".m_lookup_export" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_lookup_if_vp_mon<ADDR_WIDTH, DATA_WIDTH>::m_lookup_port, "m_lookup_port" , string(static_cast<sc_object*>(result)->name()) + ".m_lookup_port" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_lookup_if_vp_mon<ADDR_WIDTH, DATA_WIDTH>::m_num_nb_send_address, "m_num_nb_send_address" , string(static_cast<sc_object*>(result)->name()) + ".m_num_nb_send_address" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_lookup_if_vp_mon<ADDR_WIDTH, DATA_WIDTH>::m_num_nb_is_ready_true, "m_num_nb_is_ready_true" , string(static_cast<sc_object*>(result)->name()) + ".m_num_nb_is_ready_true" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_lookup_if_vp_mon<ADDR_WIDTH, DATA_WIDTH>::m_num_nb_is_ready_false, "m_num_nb_is_ready_false" , string(static_cast<sc_object*>(result)->name()) + ".m_num_nb_is_ready_false" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_lookup_if_vp_mon<ADDR_WIDTH, DATA_WIDTH>::m_num_nb_get_data, "m_num_nb_get_data" , string(static_cast<sc_object*>(result)->name()) + ".m_num_nb_get_data" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_lookup_if_vp_mon<ADDR_WIDTH, DATA_WIDTH>::m_num_ready_events, "m_num_ready_events" , string(static_cast<sc_object*>(result)->name()) + ".m_num_ready_events" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_lookup_if_vp_mon<ADDR_WIDTH, DATA_WIDTH>::m_address, "m_address" , string(static_cast<sc_object*>(result)->name()) + ".m_address" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_lookup_if_vp_mon<ADDR_WIDTH, DATA_WIDTH>::m_data, "m_data" , string(static_cast<sc_object*>(result)->name()) + ".m_data" );
      return result;
    } else {
      if (creationVerboseMode() >= 6) { std::cout << "xtsc_lookup_if_vp_mon/xtsc_vp::xtsc_lookup_if_vp_mon: " << hierach_name << " created." << std::endl; }
      xtsc_vp::xtsc_lookup_if_vp_mon<ADDR_WIDTH, DATA_WIDTH>* result = new xtsc_vp::xtsc_lookup_if_vp_mon<ADDR_WIDTH, DATA_WIDTH>(name.c_str());
       cwr_sc_object_registry::inst().addExport(&result->m_lookup_export, string(static_cast<sc_object*>(result)->name()) + ".m_lookup_export" );
       cwr_sc_object_registry::inst().addPort(&result->m_lookup_port, string(static_cast<sc_object*>(result)->name()) + ".m_lookup_port" );
       cwr_sc_object_registry::inst().addPort(&result->m_num_nb_send_address, string(static_cast<sc_object*>(result)->name()) + ".m_num_nb_send_address" );
       cwr_sc_object_registry::inst().addPort(&result->m_num_nb_is_ready_true, string(static_cast<sc_object*>(result)->name()) + ".m_num_nb_is_ready_true" );
       cwr_sc_object_registry::inst().addPort(&result->m_num_nb_is_ready_false, string(static_cast<sc_object*>(result)->name()) + ".m_num_nb_is_ready_false" );
       cwr_sc_object_registry::inst().addPort(&result->m_num_nb_get_data, string(static_cast<sc_object*>(result)->name()) + ".m_num_nb_get_data" );
       cwr_sc_object_registry::inst().addPort(&result->m_num_ready_events, string(static_cast<sc_object*>(result)->name()) + ".m_num_ready_events" );
       cwr_sc_object_registry::inst().addPort(&result->m_address, string(static_cast<sc_object*>(result)->name()) + ".m_address" );
       cwr_sc_object_registry::inst().addPort(&result->m_data, string(static_cast<sc_object*>(result)->name()) + ".m_data" );
      return result;
    }
  }
};

template<int ADDR_WIDTH, int DATA_WIDTH>
class xtsc_vp__xtsc_lookup_if_vp_mon1Creator : public ScObjectCreatorBase
{
public:
  static unsigned int creationVerboseMode() {
    const char * const env_var_val = ::getenv("SNPS_SLS_DYNAMIC_CREATION_VERBOSE");
    return env_var_val ? (::atoi(env_var_val)) : 3;
  }
  sc_object* create ( const string& name ) {
    string hierach_name = getHierarchicalName(name);
    string vcd_name = scml_property_registry::inst().getStringProperty(
	      scml_property_registry::CONSTRUCTOR, hierach_name, "vcd_name");

    bool connected = scml_property_registry::inst().getBoolProperty(
	      scml_property_registry::CONSTRUCTOR, hierach_name, "connected");

    if (scml_property_registry::inst().hasProperty(scml_property_registry::MODULE, scml_property_registry::BOOL, hierach_name, "runtime_disabled") && 
        scml_property_registry::inst().getBoolProperty(scml_property_registry::MODULE, hierach_name, "runtime_disabled")) {
      sc_module_name n(name.c_str());
      if (creationVerboseMode() >= 6) { std::cout << "xtsc_lookup_if_vp_mon/xtsc_vp::xtsc_lookup_if_vp_mon: STUB for " << hierach_name << " created." << std::endl; }
      conf::stub *result = new conf::stub(n);
       cwr_sc_object_registry::inst().addExport(new xtsc::xtsc_lookup_if_vp_stub(std::string("m_lookup_export" ).c_str()), string(static_cast<sc_object*>(result)->name()) + ".m_lookup_export" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_lookup_if_vp_mon<ADDR_WIDTH, DATA_WIDTH>::m_lookup_port, "m_lookup_port" , string(static_cast<sc_object*>(result)->name()) + ".m_lookup_port" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_lookup_if_vp_mon<ADDR_WIDTH, DATA_WIDTH>::m_num_nb_send_address, "m_num_nb_send_address" , string(static_cast<sc_object*>(result)->name()) + ".m_num_nb_send_address" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_lookup_if_vp_mon<ADDR_WIDTH, DATA_WIDTH>::m_num_nb_is_ready_true, "m_num_nb_is_ready_true" , string(static_cast<sc_object*>(result)->name()) + ".m_num_nb_is_ready_true" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_lookup_if_vp_mon<ADDR_WIDTH, DATA_WIDTH>::m_num_nb_is_ready_false, "m_num_nb_is_ready_false" , string(static_cast<sc_object*>(result)->name()) + ".m_num_nb_is_ready_false" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_lookup_if_vp_mon<ADDR_WIDTH, DATA_WIDTH>::m_num_nb_get_data, "m_num_nb_get_data" , string(static_cast<sc_object*>(result)->name()) + ".m_num_nb_get_data" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_lookup_if_vp_mon<ADDR_WIDTH, DATA_WIDTH>::m_num_ready_events, "m_num_ready_events" , string(static_cast<sc_object*>(result)->name()) + ".m_num_ready_events" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_lookup_if_vp_mon<ADDR_WIDTH, DATA_WIDTH>::m_address, "m_address" , string(static_cast<sc_object*>(result)->name()) + ".m_address" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_lookup_if_vp_mon<ADDR_WIDTH, DATA_WIDTH>::m_data, "m_data" , string(static_cast<sc_object*>(result)->name()) + ".m_data" );
      return result;
    } else {
      if (creationVerboseMode() >= 6) { std::cout << "xtsc_lookup_if_vp_mon/xtsc_vp::xtsc_lookup_if_vp_mon: " << hierach_name << " created." << std::endl; }
      xtsc_vp::xtsc_lookup_if_vp_mon<ADDR_WIDTH, DATA_WIDTH>* result = new xtsc_vp::xtsc_lookup_if_vp_mon<ADDR_WIDTH, DATA_WIDTH>(name.c_str(), vcd_name.c_str(), connected);
       cwr_sc_object_registry::inst().addExport(&result->m_lookup_export, string(static_cast<sc_object*>(result)->name()) + ".m_lookup_export" );
       cwr_sc_object_registry::inst().addPort(&result->m_lookup_port, string(static_cast<sc_object*>(result)->name()) + ".m_lookup_port" );
       cwr_sc_object_registry::inst().addPort(&result->m_num_nb_send_address, string(static_cast<sc_object*>(result)->name()) + ".m_num_nb_send_address" );
       cwr_sc_object_registry::inst().addPort(&result->m_num_nb_is_ready_true, string(static_cast<sc_object*>(result)->name()) + ".m_num_nb_is_ready_true" );
       cwr_sc_object_registry::inst().addPort(&result->m_num_nb_is_ready_false, string(static_cast<sc_object*>(result)->name()) + ".m_num_nb_is_ready_false" );
       cwr_sc_object_registry::inst().addPort(&result->m_num_nb_get_data, string(static_cast<sc_object*>(result)->name()) + ".m_num_nb_get_data" );
       cwr_sc_object_registry::inst().addPort(&result->m_num_ready_events, string(static_cast<sc_object*>(result)->name()) + ".m_num_ready_events" );
       cwr_sc_object_registry::inst().addPort(&result->m_address, string(static_cast<sc_object*>(result)->name()) + ".m_address" );
       cwr_sc_object_registry::inst().addPort(&result->m_data, string(static_cast<sc_object*>(result)->name()) + ".m_data" );
      return result;
    }
  }
};



struct DllAdapter {
  DllAdapter() {
    ScExportFactory::inst().addCreator ("sc_core::sc_export<xtsc::xtsc_lookup_if>", new ScExportCreator<sc_core::sc_export<xtsc::xtsc_lookup_if> >());
    ScExportFactory::inst().addCreator ("sc_export<xtsc::xtsc_lookup_if>", new ScExportCreator<sc_export<xtsc::xtsc_lookup_if> >());
    ScExportFactory::inst().addCreator ("xtsc::xtsc_lookup_if_vp_stub", new ScExportCreator<xtsc::xtsc_lookup_if_vp_stub>());
    ScObjectFactory::inst().addCreator ("sc_signal<sc_biguint<32> >", new ScPrimChannelCreator<sc_signal<sc_biguint<32> > >());
    ScObjectFactory::inst().addCreator ("sc_signal<sc_biguint<8> >", new ScPrimChannelCreator<sc_signal<sc_biguint<8> > >());
    ScObjectFactory::inst().addCreator ("sc_signal<unsigned long long>", new ScPrimChannelCreator<sc_signal<unsigned long long> >());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_lookup_if_vp_mon0<8, 32>", new xtsc_vp__xtsc_lookup_if_vp_mon0Creator<8, 32>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_lookup_if_vp_mon1<8, 32>", new xtsc_vp__xtsc_lookup_if_vp_mon1Creator<8, 32>());
    ScPortFactory::inst().addCreator ("sc_core::sc_port<xtsc::xtsc_lookup_if>", new ScPortCreator<sc_core::sc_port<xtsc::xtsc_lookup_if> >());
    ScPortFactory::inst().addCreator ("sc_inout<sc_biguint<32> >", new ScPortCreator<sc_inout<sc_biguint<32> > >());
    ScPortFactory::inst().addCreator ("sc_inout<sc_biguint<8> >", new ScPortCreator<sc_inout<sc_biguint<8> > >());
    ScPortFactory::inst().addCreator ("sc_inout<unsigned long long>", new ScPortCreator<sc_inout<unsigned long long> >());
    ScPortFactory::inst().addCreator ("sc_port<xtsc::xtsc_lookup_if>", new ScPortCreator<sc_port<xtsc::xtsc_lookup_if> >());
    if (::getenv("SNPS_SLS_DYNAMIC_LOADER_VERBOSE")) { std::cout << "xtsc_lookup_if_vp_mon/xtsc_vp::xtsc_lookup_if_vp_mon loaded." << std::endl; }
  }
  ~DllAdapter() {
  }
  static DllAdapter sInstance;
};

DllAdapter DllAdapter::sInstance;

}
