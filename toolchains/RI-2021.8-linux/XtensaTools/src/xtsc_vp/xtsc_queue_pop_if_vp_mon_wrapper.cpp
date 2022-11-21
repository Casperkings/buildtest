#include "systemc.h"
#include "xtsc_vp/xtsc_if_vp_stubs.h"
#include "xtsc_vp/xtsc_queue_pop_if_vp_mon.h"
#include "cassert"
#include "cwr_dynamic_loader.h"
#include "cwr_sc_dynamic_stubs.h"
#include "cwr_sc_hierarch_module.h"
#include "cwr_sc_object_creator.h"
#include "scmlinc/scml_abstraction_level_switch.h"
#include "scmlinc/scml_property_registry.h"

namespace xtsc_queue_pop_if_vp_mon_xtsc_vp__xtsc_queue_pop_if_vp_mon_FastBuild {

using namespace conf;
using namespace std;

template<int DATA_WIDTH>
class xtsc_vp__xtsc_queue_pop_if_vp_mon0Creator : public ScObjectCreatorBase
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
      if (creationVerboseMode() >= 6) { std::cout << "xtsc_queue_pop_if_vp_mon/xtsc_vp::xtsc_queue_pop_if_vp_mon: STUB for " << hierach_name << " created." << std::endl; }
      conf::stub *result = new conf::stub(n);
       cwr_sc_object_registry::inst().addExport(new xtsc::xtsc_queue_pop_if_vp_stub(std::string("m_queue_pop_export" ).c_str()), string(static_cast<sc_object*>(result)->name()) + ".m_queue_pop_export" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_queue_pop_if_vp_mon<DATA_WIDTH>::m_queue_pop_port, "m_queue_pop_port" , string(static_cast<sc_object*>(result)->name()) + ".m_queue_pop_port" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_queue_pop_if_vp_mon<DATA_WIDTH>::m_num_nb_can_pop_true, "m_num_nb_can_pop_true" , string(static_cast<sc_object*>(result)->name()) + ".m_num_nb_can_pop_true" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_queue_pop_if_vp_mon<DATA_WIDTH>::m_num_nb_can_pop_false, "m_num_nb_can_pop_false" , string(static_cast<sc_object*>(result)->name()) + ".m_num_nb_can_pop_false" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_queue_pop_if_vp_mon<DATA_WIDTH>::m_num_nb_pop_true, "m_num_nb_pop_true" , string(static_cast<sc_object*>(result)->name()) + ".m_num_nb_pop_true" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_queue_pop_if_vp_mon<DATA_WIDTH>::m_num_nb_pop_false, "m_num_nb_pop_false" , string(static_cast<sc_object*>(result)->name()) + ".m_num_nb_pop_false" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_queue_pop_if_vp_mon<DATA_WIDTH>::m_num_nonempty_events, "m_num_nonempty_events" , string(static_cast<sc_object*>(result)->name()) + ".m_num_nonempty_events" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_queue_pop_if_vp_mon<DATA_WIDTH>::m_ticket, "m_ticket" , string(static_cast<sc_object*>(result)->name()) + ".m_ticket" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_queue_pop_if_vp_mon<DATA_WIDTH>::m_element, "m_element" , string(static_cast<sc_object*>(result)->name()) + ".m_element" );
      return result;
    } else {
      if (creationVerboseMode() >= 6) { std::cout << "xtsc_queue_pop_if_vp_mon/xtsc_vp::xtsc_queue_pop_if_vp_mon: " << hierach_name << " created." << std::endl; }
      xtsc_vp::xtsc_queue_pop_if_vp_mon<DATA_WIDTH>* result = new xtsc_vp::xtsc_queue_pop_if_vp_mon<DATA_WIDTH>(name.c_str());
       cwr_sc_object_registry::inst().addExport(&result->m_queue_pop_export, string(static_cast<sc_object*>(result)->name()) + ".m_queue_pop_export" );
       cwr_sc_object_registry::inst().addPort(&result->m_queue_pop_port, string(static_cast<sc_object*>(result)->name()) + ".m_queue_pop_port" );
       cwr_sc_object_registry::inst().addPort(&result->m_num_nb_can_pop_true, string(static_cast<sc_object*>(result)->name()) + ".m_num_nb_can_pop_true" );
       cwr_sc_object_registry::inst().addPort(&result->m_num_nb_can_pop_false, string(static_cast<sc_object*>(result)->name()) + ".m_num_nb_can_pop_false" );
       cwr_sc_object_registry::inst().addPort(&result->m_num_nb_pop_true, string(static_cast<sc_object*>(result)->name()) + ".m_num_nb_pop_true" );
       cwr_sc_object_registry::inst().addPort(&result->m_num_nb_pop_false, string(static_cast<sc_object*>(result)->name()) + ".m_num_nb_pop_false" );
       cwr_sc_object_registry::inst().addPort(&result->m_num_nonempty_events, string(static_cast<sc_object*>(result)->name()) + ".m_num_nonempty_events" );
       cwr_sc_object_registry::inst().addPort(&result->m_ticket, string(static_cast<sc_object*>(result)->name()) + ".m_ticket" );
       cwr_sc_object_registry::inst().addPort(&result->m_element, string(static_cast<sc_object*>(result)->name()) + ".m_element" );
      return result;
    }
  }
};

template<int DATA_WIDTH>
class xtsc_vp__xtsc_queue_pop_if_vp_mon1Creator : public ScObjectCreatorBase
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
      if (creationVerboseMode() >= 6) { std::cout << "xtsc_queue_pop_if_vp_mon/xtsc_vp::xtsc_queue_pop_if_vp_mon: STUB for " << hierach_name << " created." << std::endl; }
      conf::stub *result = new conf::stub(n);
       cwr_sc_object_registry::inst().addExport(new xtsc::xtsc_queue_pop_if_vp_stub(std::string("m_queue_pop_export" ).c_str()), string(static_cast<sc_object*>(result)->name()) + ".m_queue_pop_export" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_queue_pop_if_vp_mon<DATA_WIDTH>::m_queue_pop_port, "m_queue_pop_port" , string(static_cast<sc_object*>(result)->name()) + ".m_queue_pop_port" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_queue_pop_if_vp_mon<DATA_WIDTH>::m_num_nb_can_pop_true, "m_num_nb_can_pop_true" , string(static_cast<sc_object*>(result)->name()) + ".m_num_nb_can_pop_true" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_queue_pop_if_vp_mon<DATA_WIDTH>::m_num_nb_can_pop_false, "m_num_nb_can_pop_false" , string(static_cast<sc_object*>(result)->name()) + ".m_num_nb_can_pop_false" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_queue_pop_if_vp_mon<DATA_WIDTH>::m_num_nb_pop_true, "m_num_nb_pop_true" , string(static_cast<sc_object*>(result)->name()) + ".m_num_nb_pop_true" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_queue_pop_if_vp_mon<DATA_WIDTH>::m_num_nb_pop_false, "m_num_nb_pop_false" , string(static_cast<sc_object*>(result)->name()) + ".m_num_nb_pop_false" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_queue_pop_if_vp_mon<DATA_WIDTH>::m_num_nonempty_events, "m_num_nonempty_events" , string(static_cast<sc_object*>(result)->name()) + ".m_num_nonempty_events" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_queue_pop_if_vp_mon<DATA_WIDTH>::m_ticket, "m_ticket" , string(static_cast<sc_object*>(result)->name()) + ".m_ticket" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_queue_pop_if_vp_mon<DATA_WIDTH>::m_element, "m_element" , string(static_cast<sc_object*>(result)->name()) + ".m_element" );
      return result;
    } else {
      if (creationVerboseMode() >= 6) { std::cout << "xtsc_queue_pop_if_vp_mon/xtsc_vp::xtsc_queue_pop_if_vp_mon: " << hierach_name << " created." << std::endl; }
      xtsc_vp::xtsc_queue_pop_if_vp_mon<DATA_WIDTH>* result = new xtsc_vp::xtsc_queue_pop_if_vp_mon<DATA_WIDTH>(name.c_str(), vcd_name.c_str(), connected);
       cwr_sc_object_registry::inst().addExport(&result->m_queue_pop_export, string(static_cast<sc_object*>(result)->name()) + ".m_queue_pop_export" );
       cwr_sc_object_registry::inst().addPort(&result->m_queue_pop_port, string(static_cast<sc_object*>(result)->name()) + ".m_queue_pop_port" );
       cwr_sc_object_registry::inst().addPort(&result->m_num_nb_can_pop_true, string(static_cast<sc_object*>(result)->name()) + ".m_num_nb_can_pop_true" );
       cwr_sc_object_registry::inst().addPort(&result->m_num_nb_can_pop_false, string(static_cast<sc_object*>(result)->name()) + ".m_num_nb_can_pop_false" );
       cwr_sc_object_registry::inst().addPort(&result->m_num_nb_pop_true, string(static_cast<sc_object*>(result)->name()) + ".m_num_nb_pop_true" );
       cwr_sc_object_registry::inst().addPort(&result->m_num_nb_pop_false, string(static_cast<sc_object*>(result)->name()) + ".m_num_nb_pop_false" );
       cwr_sc_object_registry::inst().addPort(&result->m_num_nonempty_events, string(static_cast<sc_object*>(result)->name()) + ".m_num_nonempty_events" );
       cwr_sc_object_registry::inst().addPort(&result->m_ticket, string(static_cast<sc_object*>(result)->name()) + ".m_ticket" );
       cwr_sc_object_registry::inst().addPort(&result->m_element, string(static_cast<sc_object*>(result)->name()) + ".m_element" );
      return result;
    }
  }
};



struct DllAdapter {
  DllAdapter() {
    ScExportFactory::inst().addCreator ("sc_core::sc_export<xtsc::xtsc_queue_pop_if>", new ScExportCreator<sc_core::sc_export<xtsc::xtsc_queue_pop_if> >());
    ScExportFactory::inst().addCreator ("sc_export<xtsc::xtsc_queue_pop_if>", new ScExportCreator<sc_export<xtsc::xtsc_queue_pop_if> >());
    ScExportFactory::inst().addCreator ("xtsc::xtsc_queue_pop_if_vp_stub", new ScExportCreator<xtsc::xtsc_queue_pop_if_vp_stub>());
    ScObjectFactory::inst().addCreator ("sc_signal<sc_biguint<1024> >", new ScPrimChannelCreator<sc_signal<sc_biguint<1024> > >());
    ScObjectFactory::inst().addCreator ("sc_signal<sc_biguint<104> >", new ScPrimChannelCreator<sc_signal<sc_biguint<104> > >());
    ScObjectFactory::inst().addCreator ("sc_signal<sc_biguint<10> >", new ScPrimChannelCreator<sc_signal<sc_biguint<10> > >());
    ScObjectFactory::inst().addCreator ("sc_signal<sc_biguint<112> >", new ScPrimChannelCreator<sc_signal<sc_biguint<112> > >());
    ScObjectFactory::inst().addCreator ("sc_signal<sc_biguint<11> >", new ScPrimChannelCreator<sc_signal<sc_biguint<11> > >());
    ScObjectFactory::inst().addCreator ("sc_signal<sc_biguint<120> >", new ScPrimChannelCreator<sc_signal<sc_biguint<120> > >());
    ScObjectFactory::inst().addCreator ("sc_signal<sc_biguint<128> >", new ScPrimChannelCreator<sc_signal<sc_biguint<128> > >());
    ScObjectFactory::inst().addCreator ("sc_signal<sc_biguint<12> >", new ScPrimChannelCreator<sc_signal<sc_biguint<12> > >());
    ScObjectFactory::inst().addCreator ("sc_signal<sc_biguint<13> >", new ScPrimChannelCreator<sc_signal<sc_biguint<13> > >());
    ScObjectFactory::inst().addCreator ("sc_signal<sc_biguint<14> >", new ScPrimChannelCreator<sc_signal<sc_biguint<14> > >());
    ScObjectFactory::inst().addCreator ("sc_signal<sc_biguint<15> >", new ScPrimChannelCreator<sc_signal<sc_biguint<15> > >());
    ScObjectFactory::inst().addCreator ("sc_signal<sc_biguint<16> >", new ScPrimChannelCreator<sc_signal<sc_biguint<16> > >());
    ScObjectFactory::inst().addCreator ("sc_signal<sc_biguint<1> >", new ScPrimChannelCreator<sc_signal<sc_biguint<1> > >());
    ScObjectFactory::inst().addCreator ("sc_signal<sc_biguint<20> >", new ScPrimChannelCreator<sc_signal<sc_biguint<20> > >());
    ScObjectFactory::inst().addCreator ("sc_signal<sc_biguint<24> >", new ScPrimChannelCreator<sc_signal<sc_biguint<24> > >());
    ScObjectFactory::inst().addCreator ("sc_signal<sc_biguint<256> >", new ScPrimChannelCreator<sc_signal<sc_biguint<256> > >());
    ScObjectFactory::inst().addCreator ("sc_signal<sc_biguint<28> >", new ScPrimChannelCreator<sc_signal<sc_biguint<28> > >());
    ScObjectFactory::inst().addCreator ("sc_signal<sc_biguint<2> >", new ScPrimChannelCreator<sc_signal<sc_biguint<2> > >());
    ScObjectFactory::inst().addCreator ("sc_signal<sc_biguint<32> >", new ScPrimChannelCreator<sc_signal<sc_biguint<32> > >());
    ScObjectFactory::inst().addCreator ("sc_signal<sc_biguint<3> >", new ScPrimChannelCreator<sc_signal<sc_biguint<3> > >());
    ScObjectFactory::inst().addCreator ("sc_signal<sc_biguint<40> >", new ScPrimChannelCreator<sc_signal<sc_biguint<40> > >());
    ScObjectFactory::inst().addCreator ("sc_signal<sc_biguint<48> >", new ScPrimChannelCreator<sc_signal<sc_biguint<48> > >());
    ScObjectFactory::inst().addCreator ("sc_signal<sc_biguint<4> >", new ScPrimChannelCreator<sc_signal<sc_biguint<4> > >());
    ScObjectFactory::inst().addCreator ("sc_signal<sc_biguint<512> >", new ScPrimChannelCreator<sc_signal<sc_biguint<512> > >());
    ScObjectFactory::inst().addCreator ("sc_signal<sc_biguint<56> >", new ScPrimChannelCreator<sc_signal<sc_biguint<56> > >());
    ScObjectFactory::inst().addCreator ("sc_signal<sc_biguint<5> >", new ScPrimChannelCreator<sc_signal<sc_biguint<5> > >());
    ScObjectFactory::inst().addCreator ("sc_signal<sc_biguint<64> >", new ScPrimChannelCreator<sc_signal<sc_biguint<64> > >());
    ScObjectFactory::inst().addCreator ("sc_signal<sc_biguint<6> >", new ScPrimChannelCreator<sc_signal<sc_biguint<6> > >());
    ScObjectFactory::inst().addCreator ("sc_signal<sc_biguint<72> >", new ScPrimChannelCreator<sc_signal<sc_biguint<72> > >());
    ScObjectFactory::inst().addCreator ("sc_signal<sc_biguint<7> >", new ScPrimChannelCreator<sc_signal<sc_biguint<7> > >());
    ScObjectFactory::inst().addCreator ("sc_signal<sc_biguint<80> >", new ScPrimChannelCreator<sc_signal<sc_biguint<80> > >());
    ScObjectFactory::inst().addCreator ("sc_signal<sc_biguint<88> >", new ScPrimChannelCreator<sc_signal<sc_biguint<88> > >());
    ScObjectFactory::inst().addCreator ("sc_signal<sc_biguint<8> >", new ScPrimChannelCreator<sc_signal<sc_biguint<8> > >());
    ScObjectFactory::inst().addCreator ("sc_signal<sc_biguint<96> >", new ScPrimChannelCreator<sc_signal<sc_biguint<96> > >());
    ScObjectFactory::inst().addCreator ("sc_signal<sc_biguint<9> >", new ScPrimChannelCreator<sc_signal<sc_biguint<9> > >());
    ScObjectFactory::inst().addCreator ("sc_signal<unsigned long long>", new ScPrimChannelCreator<sc_signal<unsigned long long> >());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon0<1024>", new xtsc_vp__xtsc_queue_pop_if_vp_mon0Creator<1024>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon0<104>", new xtsc_vp__xtsc_queue_pop_if_vp_mon0Creator<104>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon0<10>", new xtsc_vp__xtsc_queue_pop_if_vp_mon0Creator<10>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon0<112>", new xtsc_vp__xtsc_queue_pop_if_vp_mon0Creator<112>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon0<11>", new xtsc_vp__xtsc_queue_pop_if_vp_mon0Creator<11>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon0<120>", new xtsc_vp__xtsc_queue_pop_if_vp_mon0Creator<120>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon0<128>", new xtsc_vp__xtsc_queue_pop_if_vp_mon0Creator<128>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon0<12>", new xtsc_vp__xtsc_queue_pop_if_vp_mon0Creator<12>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon0<13>", new xtsc_vp__xtsc_queue_pop_if_vp_mon0Creator<13>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon0<14>", new xtsc_vp__xtsc_queue_pop_if_vp_mon0Creator<14>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon0<15>", new xtsc_vp__xtsc_queue_pop_if_vp_mon0Creator<15>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon0<16>", new xtsc_vp__xtsc_queue_pop_if_vp_mon0Creator<16>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon0<1>", new xtsc_vp__xtsc_queue_pop_if_vp_mon0Creator<1>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon0<20>", new xtsc_vp__xtsc_queue_pop_if_vp_mon0Creator<20>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon0<24>", new xtsc_vp__xtsc_queue_pop_if_vp_mon0Creator<24>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon0<256>", new xtsc_vp__xtsc_queue_pop_if_vp_mon0Creator<256>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon0<28>", new xtsc_vp__xtsc_queue_pop_if_vp_mon0Creator<28>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon0<2>", new xtsc_vp__xtsc_queue_pop_if_vp_mon0Creator<2>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon0<32>", new xtsc_vp__xtsc_queue_pop_if_vp_mon0Creator<32>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon0<3>", new xtsc_vp__xtsc_queue_pop_if_vp_mon0Creator<3>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon0<40>", new xtsc_vp__xtsc_queue_pop_if_vp_mon0Creator<40>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon0<48>", new xtsc_vp__xtsc_queue_pop_if_vp_mon0Creator<48>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon0<4>", new xtsc_vp__xtsc_queue_pop_if_vp_mon0Creator<4>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon0<512>", new xtsc_vp__xtsc_queue_pop_if_vp_mon0Creator<512>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon0<56>", new xtsc_vp__xtsc_queue_pop_if_vp_mon0Creator<56>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon0<5>", new xtsc_vp__xtsc_queue_pop_if_vp_mon0Creator<5>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon0<64>", new xtsc_vp__xtsc_queue_pop_if_vp_mon0Creator<64>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon0<6>", new xtsc_vp__xtsc_queue_pop_if_vp_mon0Creator<6>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon0<72>", new xtsc_vp__xtsc_queue_pop_if_vp_mon0Creator<72>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon0<7>", new xtsc_vp__xtsc_queue_pop_if_vp_mon0Creator<7>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon0<80>", new xtsc_vp__xtsc_queue_pop_if_vp_mon0Creator<80>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon0<88>", new xtsc_vp__xtsc_queue_pop_if_vp_mon0Creator<88>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon0<8>", new xtsc_vp__xtsc_queue_pop_if_vp_mon0Creator<8>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon0<96>", new xtsc_vp__xtsc_queue_pop_if_vp_mon0Creator<96>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon0<9>", new xtsc_vp__xtsc_queue_pop_if_vp_mon0Creator<9>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon1<1024>", new xtsc_vp__xtsc_queue_pop_if_vp_mon1Creator<1024>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon1<104>", new xtsc_vp__xtsc_queue_pop_if_vp_mon1Creator<104>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon1<10>", new xtsc_vp__xtsc_queue_pop_if_vp_mon1Creator<10>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon1<112>", new xtsc_vp__xtsc_queue_pop_if_vp_mon1Creator<112>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon1<11>", new xtsc_vp__xtsc_queue_pop_if_vp_mon1Creator<11>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon1<120>", new xtsc_vp__xtsc_queue_pop_if_vp_mon1Creator<120>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon1<128>", new xtsc_vp__xtsc_queue_pop_if_vp_mon1Creator<128>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon1<12>", new xtsc_vp__xtsc_queue_pop_if_vp_mon1Creator<12>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon1<13>", new xtsc_vp__xtsc_queue_pop_if_vp_mon1Creator<13>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon1<14>", new xtsc_vp__xtsc_queue_pop_if_vp_mon1Creator<14>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon1<15>", new xtsc_vp__xtsc_queue_pop_if_vp_mon1Creator<15>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon1<16>", new xtsc_vp__xtsc_queue_pop_if_vp_mon1Creator<16>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon1<1>", new xtsc_vp__xtsc_queue_pop_if_vp_mon1Creator<1>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon1<20>", new xtsc_vp__xtsc_queue_pop_if_vp_mon1Creator<20>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon1<24>", new xtsc_vp__xtsc_queue_pop_if_vp_mon1Creator<24>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon1<256>", new xtsc_vp__xtsc_queue_pop_if_vp_mon1Creator<256>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon1<28>", new xtsc_vp__xtsc_queue_pop_if_vp_mon1Creator<28>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon1<2>", new xtsc_vp__xtsc_queue_pop_if_vp_mon1Creator<2>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon1<32>", new xtsc_vp__xtsc_queue_pop_if_vp_mon1Creator<32>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon1<3>", new xtsc_vp__xtsc_queue_pop_if_vp_mon1Creator<3>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon1<40>", new xtsc_vp__xtsc_queue_pop_if_vp_mon1Creator<40>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon1<48>", new xtsc_vp__xtsc_queue_pop_if_vp_mon1Creator<48>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon1<4>", new xtsc_vp__xtsc_queue_pop_if_vp_mon1Creator<4>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon1<512>", new xtsc_vp__xtsc_queue_pop_if_vp_mon1Creator<512>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon1<56>", new xtsc_vp__xtsc_queue_pop_if_vp_mon1Creator<56>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon1<5>", new xtsc_vp__xtsc_queue_pop_if_vp_mon1Creator<5>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon1<64>", new xtsc_vp__xtsc_queue_pop_if_vp_mon1Creator<64>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon1<6>", new xtsc_vp__xtsc_queue_pop_if_vp_mon1Creator<6>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon1<72>", new xtsc_vp__xtsc_queue_pop_if_vp_mon1Creator<72>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon1<7>", new xtsc_vp__xtsc_queue_pop_if_vp_mon1Creator<7>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon1<80>", new xtsc_vp__xtsc_queue_pop_if_vp_mon1Creator<80>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon1<88>", new xtsc_vp__xtsc_queue_pop_if_vp_mon1Creator<88>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon1<8>", new xtsc_vp__xtsc_queue_pop_if_vp_mon1Creator<8>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon1<96>", new xtsc_vp__xtsc_queue_pop_if_vp_mon1Creator<96>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pop_if_vp_mon1<9>", new xtsc_vp__xtsc_queue_pop_if_vp_mon1Creator<9>());
    ScPortFactory::inst().addCreator ("sc_core::sc_port<xtsc::xtsc_queue_pop_if>", new ScPortCreator<sc_core::sc_port<xtsc::xtsc_queue_pop_if> >());
    ScPortFactory::inst().addCreator ("sc_inout<sc_biguint<1024> >", new ScPortCreator<sc_inout<sc_biguint<1024> > >());
    ScPortFactory::inst().addCreator ("sc_inout<sc_biguint<104> >", new ScPortCreator<sc_inout<sc_biguint<104> > >());
    ScPortFactory::inst().addCreator ("sc_inout<sc_biguint<10> >", new ScPortCreator<sc_inout<sc_biguint<10> > >());
    ScPortFactory::inst().addCreator ("sc_inout<sc_biguint<112> >", new ScPortCreator<sc_inout<sc_biguint<112> > >());
    ScPortFactory::inst().addCreator ("sc_inout<sc_biguint<11> >", new ScPortCreator<sc_inout<sc_biguint<11> > >());
    ScPortFactory::inst().addCreator ("sc_inout<sc_biguint<120> >", new ScPortCreator<sc_inout<sc_biguint<120> > >());
    ScPortFactory::inst().addCreator ("sc_inout<sc_biguint<128> >", new ScPortCreator<sc_inout<sc_biguint<128> > >());
    ScPortFactory::inst().addCreator ("sc_inout<sc_biguint<12> >", new ScPortCreator<sc_inout<sc_biguint<12> > >());
    ScPortFactory::inst().addCreator ("sc_inout<sc_biguint<13> >", new ScPortCreator<sc_inout<sc_biguint<13> > >());
    ScPortFactory::inst().addCreator ("sc_inout<sc_biguint<14> >", new ScPortCreator<sc_inout<sc_biguint<14> > >());
    ScPortFactory::inst().addCreator ("sc_inout<sc_biguint<15> >", new ScPortCreator<sc_inout<sc_biguint<15> > >());
    ScPortFactory::inst().addCreator ("sc_inout<sc_biguint<16> >", new ScPortCreator<sc_inout<sc_biguint<16> > >());
    ScPortFactory::inst().addCreator ("sc_inout<sc_biguint<1> >", new ScPortCreator<sc_inout<sc_biguint<1> > >());
    ScPortFactory::inst().addCreator ("sc_inout<sc_biguint<20> >", new ScPortCreator<sc_inout<sc_biguint<20> > >());
    ScPortFactory::inst().addCreator ("sc_inout<sc_biguint<24> >", new ScPortCreator<sc_inout<sc_biguint<24> > >());
    ScPortFactory::inst().addCreator ("sc_inout<sc_biguint<256> >", new ScPortCreator<sc_inout<sc_biguint<256> > >());
    ScPortFactory::inst().addCreator ("sc_inout<sc_biguint<28> >", new ScPortCreator<sc_inout<sc_biguint<28> > >());
    ScPortFactory::inst().addCreator ("sc_inout<sc_biguint<2> >", new ScPortCreator<sc_inout<sc_biguint<2> > >());
    ScPortFactory::inst().addCreator ("sc_inout<sc_biguint<32> >", new ScPortCreator<sc_inout<sc_biguint<32> > >());
    ScPortFactory::inst().addCreator ("sc_inout<sc_biguint<3> >", new ScPortCreator<sc_inout<sc_biguint<3> > >());
    ScPortFactory::inst().addCreator ("sc_inout<sc_biguint<40> >", new ScPortCreator<sc_inout<sc_biguint<40> > >());
    ScPortFactory::inst().addCreator ("sc_inout<sc_biguint<48> >", new ScPortCreator<sc_inout<sc_biguint<48> > >());
    ScPortFactory::inst().addCreator ("sc_inout<sc_biguint<4> >", new ScPortCreator<sc_inout<sc_biguint<4> > >());
    ScPortFactory::inst().addCreator ("sc_inout<sc_biguint<512> >", new ScPortCreator<sc_inout<sc_biguint<512> > >());
    ScPortFactory::inst().addCreator ("sc_inout<sc_biguint<56> >", new ScPortCreator<sc_inout<sc_biguint<56> > >());
    ScPortFactory::inst().addCreator ("sc_inout<sc_biguint<5> >", new ScPortCreator<sc_inout<sc_biguint<5> > >());
    ScPortFactory::inst().addCreator ("sc_inout<sc_biguint<64> >", new ScPortCreator<sc_inout<sc_biguint<64> > >());
    ScPortFactory::inst().addCreator ("sc_inout<sc_biguint<6> >", new ScPortCreator<sc_inout<sc_biguint<6> > >());
    ScPortFactory::inst().addCreator ("sc_inout<sc_biguint<72> >", new ScPortCreator<sc_inout<sc_biguint<72> > >());
    ScPortFactory::inst().addCreator ("sc_inout<sc_biguint<7> >", new ScPortCreator<sc_inout<sc_biguint<7> > >());
    ScPortFactory::inst().addCreator ("sc_inout<sc_biguint<80> >", new ScPortCreator<sc_inout<sc_biguint<80> > >());
    ScPortFactory::inst().addCreator ("sc_inout<sc_biguint<88> >", new ScPortCreator<sc_inout<sc_biguint<88> > >());
    ScPortFactory::inst().addCreator ("sc_inout<sc_biguint<8> >", new ScPortCreator<sc_inout<sc_biguint<8> > >());
    ScPortFactory::inst().addCreator ("sc_inout<sc_biguint<96> >", new ScPortCreator<sc_inout<sc_biguint<96> > >());
    ScPortFactory::inst().addCreator ("sc_inout<sc_biguint<9> >", new ScPortCreator<sc_inout<sc_biguint<9> > >());
    ScPortFactory::inst().addCreator ("sc_inout<unsigned long long>", new ScPortCreator<sc_inout<unsigned long long> >());
    ScPortFactory::inst().addCreator ("sc_port<xtsc::xtsc_queue_pop_if>", new ScPortCreator<sc_port<xtsc::xtsc_queue_pop_if> >());
    if (::getenv("SNPS_SLS_DYNAMIC_LOADER_VERBOSE")) { std::cout << "xtsc_queue_pop_if_vp_mon/xtsc_vp::xtsc_queue_pop_if_vp_mon loaded." << std::endl; }
  }
  ~DllAdapter() {
  }
  static DllAdapter sInstance;
};

DllAdapter DllAdapter::sInstance;

}
