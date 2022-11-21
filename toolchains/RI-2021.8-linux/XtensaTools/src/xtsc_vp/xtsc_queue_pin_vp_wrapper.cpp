#include "systemc.h"
#include "xtsc_vp/xtsc_if_vp_stubs.h"
#include "xtsc_vp/xtsc_queue_pin_vp.h"
#include "cassert"
#include "cwr_dynamic_loader.h"
#include "cwr_sc_dynamic_stubs.h"
#include "cwr_sc_hierarch_module.h"
#include "cwr_sc_object_creator.h"
#include "scmlinc/scml_abstraction_level_switch.h"
#include "scmlinc/scml_property_registry.h"

namespace xtsc_queue_pin_vp_xtsc_vp__xtsc_queue_pin_vp_FastBuild {

using namespace conf;
using namespace std;

template<int DATA_WIDTH>
class xtsc_vp__xtsc_queue_pin_vp0Creator : public ScObjectCreatorBase
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
      if (creationVerboseMode() >= 6) { std::cout << "xtsc_queue_pin_vp/xtsc_vp::xtsc_queue_pin_vp: STUB for " << hierach_name << " created." << std::endl; }
      conf::stub *result = new conf::stub(n);
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_queue_pin_vp<DATA_WIDTH>::m_push, "m_push" , string(static_cast<sc_object*>(result)->name()) + ".m_push" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_queue_pin_vp<DATA_WIDTH>::m_data_in, "m_data_in" , string(static_cast<sc_object*>(result)->name()) + ".m_data_in" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_queue_pin_vp<DATA_WIDTH>::m_full, "m_full" , string(static_cast<sc_object*>(result)->name()) + ".m_full" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_queue_pin_vp<DATA_WIDTH>::m_pushable, "m_pushable" , string(static_cast<sc_object*>(result)->name()) + ".m_pushable" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_queue_pin_vp<DATA_WIDTH>::m_pop, "m_pop" , string(static_cast<sc_object*>(result)->name()) + ".m_pop" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_queue_pin_vp<DATA_WIDTH>::m_data_out, "m_data_out" , string(static_cast<sc_object*>(result)->name()) + ".m_data_out" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_queue_pin_vp<DATA_WIDTH>::m_empty, "m_empty" , string(static_cast<sc_object*>(result)->name()) + ".m_empty" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_queue_pin_vp<DATA_WIDTH>::m_poppable, "m_poppable" , string(static_cast<sc_object*>(result)->name()) + ".m_poppable" );
      return result;
    } else {
      if (creationVerboseMode() >= 6) { std::cout << "xtsc_queue_pin_vp/xtsc_vp::xtsc_queue_pin_vp: " << hierach_name << " created." << std::endl; }
      xtsc_vp::xtsc_queue_pin_vp<DATA_WIDTH>* result = new xtsc_vp::xtsc_queue_pin_vp<DATA_WIDTH>(name.c_str());
       cwr_sc_object_registry::inst().addPort(&result->m_push, string(static_cast<sc_object*>(result)->name()) + ".m_push" );
       cwr_sc_object_registry::inst().addPort(&result->m_data_in, string(static_cast<sc_object*>(result)->name()) + ".m_data_in" );
       cwr_sc_object_registry::inst().addPort(&result->m_full, string(static_cast<sc_object*>(result)->name()) + ".m_full" );
       cwr_sc_object_registry::inst().addPort(&result->m_pushable, string(static_cast<sc_object*>(result)->name()) + ".m_pushable" );
       cwr_sc_object_registry::inst().addPort(&result->m_pop, string(static_cast<sc_object*>(result)->name()) + ".m_pop" );
       cwr_sc_object_registry::inst().addPort(&result->m_data_out, string(static_cast<sc_object*>(result)->name()) + ".m_data_out" );
       cwr_sc_object_registry::inst().addPort(&result->m_empty, string(static_cast<sc_object*>(result)->name()) + ".m_empty" );
       cwr_sc_object_registry::inst().addPort(&result->m_poppable, string(static_cast<sc_object*>(result)->name()) + ".m_poppable" );
      return result;
    }
  }
};



struct DllAdapter {
  DllAdapter() {
    ScObjectFactory::inst().addCreator ("sc_signal<bool>", new ScPrimChannelCreator<sc_signal<bool> >());
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
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pin_vp0<1024>", new xtsc_vp__xtsc_queue_pin_vp0Creator<1024>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pin_vp0<104>", new xtsc_vp__xtsc_queue_pin_vp0Creator<104>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pin_vp0<10>", new xtsc_vp__xtsc_queue_pin_vp0Creator<10>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pin_vp0<112>", new xtsc_vp__xtsc_queue_pin_vp0Creator<112>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pin_vp0<11>", new xtsc_vp__xtsc_queue_pin_vp0Creator<11>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pin_vp0<120>", new xtsc_vp__xtsc_queue_pin_vp0Creator<120>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pin_vp0<128>", new xtsc_vp__xtsc_queue_pin_vp0Creator<128>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pin_vp0<12>", new xtsc_vp__xtsc_queue_pin_vp0Creator<12>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pin_vp0<13>", new xtsc_vp__xtsc_queue_pin_vp0Creator<13>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pin_vp0<14>", new xtsc_vp__xtsc_queue_pin_vp0Creator<14>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pin_vp0<15>", new xtsc_vp__xtsc_queue_pin_vp0Creator<15>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pin_vp0<16>", new xtsc_vp__xtsc_queue_pin_vp0Creator<16>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pin_vp0<1>", new xtsc_vp__xtsc_queue_pin_vp0Creator<1>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pin_vp0<20>", new xtsc_vp__xtsc_queue_pin_vp0Creator<20>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pin_vp0<24>", new xtsc_vp__xtsc_queue_pin_vp0Creator<24>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pin_vp0<256>", new xtsc_vp__xtsc_queue_pin_vp0Creator<256>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pin_vp0<28>", new xtsc_vp__xtsc_queue_pin_vp0Creator<28>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pin_vp0<2>", new xtsc_vp__xtsc_queue_pin_vp0Creator<2>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pin_vp0<32>", new xtsc_vp__xtsc_queue_pin_vp0Creator<32>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pin_vp0<3>", new xtsc_vp__xtsc_queue_pin_vp0Creator<3>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pin_vp0<40>", new xtsc_vp__xtsc_queue_pin_vp0Creator<40>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pin_vp0<48>", new xtsc_vp__xtsc_queue_pin_vp0Creator<48>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pin_vp0<4>", new xtsc_vp__xtsc_queue_pin_vp0Creator<4>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pin_vp0<512>", new xtsc_vp__xtsc_queue_pin_vp0Creator<512>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pin_vp0<56>", new xtsc_vp__xtsc_queue_pin_vp0Creator<56>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pin_vp0<5>", new xtsc_vp__xtsc_queue_pin_vp0Creator<5>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pin_vp0<64>", new xtsc_vp__xtsc_queue_pin_vp0Creator<64>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pin_vp0<6>", new xtsc_vp__xtsc_queue_pin_vp0Creator<6>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pin_vp0<72>", new xtsc_vp__xtsc_queue_pin_vp0Creator<72>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pin_vp0<7>", new xtsc_vp__xtsc_queue_pin_vp0Creator<7>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pin_vp0<80>", new xtsc_vp__xtsc_queue_pin_vp0Creator<80>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pin_vp0<88>", new xtsc_vp__xtsc_queue_pin_vp0Creator<88>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pin_vp0<8>", new xtsc_vp__xtsc_queue_pin_vp0Creator<8>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pin_vp0<96>", new xtsc_vp__xtsc_queue_pin_vp0Creator<96>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_queue_pin_vp0<9>", new xtsc_vp__xtsc_queue_pin_vp0Creator<9>());
    ScPortFactory::inst().addCreator ("sc_inout<bool>", new ScPortCreator<sc_inout<bool> >());
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
    if (::getenv("SNPS_SLS_DYNAMIC_LOADER_VERBOSE")) { std::cout << "xtsc_queue_pin_vp/xtsc_vp::xtsc_queue_pin_vp loaded." << std::endl; }
  }
  ~DllAdapter() {
  }
  static DllAdapter sInstance;
};

DllAdapter DllAdapter::sInstance;

}
