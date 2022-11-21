#include "systemc.h"
#include "xtsc_vp/xtsc_if_vp_stubs.h"
#include "xtsc_vp/xtsc_wire_vp.h"
#include "cassert"
#include "cwr_dynamic_loader.h"
#include "cwr_sc_dynamic_stubs.h"
#include "cwr_sc_hierarch_module.h"
#include "cwr_sc_object_creator.h"
#include "scmlinc/scml_abstraction_level_switch.h"
#include "scmlinc/scml_property_registry.h"

namespace xtsc_wire_vp_xtsc_vp__xtsc_wire_vp_FastBuild {

using namespace conf;
using namespace std;

template<int DATA_WIDTH>
class xtsc_vp__xtsc_wire_vp0Creator : public ScObjectCreatorBase
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
      if (creationVerboseMode() >= 6) { std::cout << "xtsc_wire_vp/xtsc_vp::xtsc_wire_vp: STUB for " << hierach_name << " created." << std::endl; }
      conf::stub *result = new conf::stub(n);
       cwr_sc_object_registry::inst().addExport(new xtsc::xtsc_wire_write_if_vp_stub(std::string("m_write" ).c_str()), string(static_cast<sc_object*>(result)->name()) + ".m_write" );
       cwr_sc_object_registry::inst().addExport(new xtsc::xtsc_wire_read_if_vp_stub(std::string("m_read" ).c_str()), string(static_cast<sc_object*>(result)->name()) + ".m_read" );
      return result;
    } else {
      if (creationVerboseMode() >= 6) { std::cout << "xtsc_wire_vp/xtsc_vp::xtsc_wire_vp: " << hierach_name << " created." << std::endl; }
      xtsc_vp::xtsc_wire_vp<DATA_WIDTH>* result = new xtsc_vp::xtsc_wire_vp<DATA_WIDTH>(name.c_str());
       cwr_sc_object_registry::inst().addExport(&result->m_write, string(static_cast<sc_object*>(result)->name()) + ".m_write" );
       cwr_sc_object_registry::inst().addExport(&result->m_read, string(static_cast<sc_object*>(result)->name()) + ".m_read" );
      return result;
    }
  }
};



struct DllAdapter {
  DllAdapter() {
    ScExportFactory::inst().addCreator ("sc_core::sc_export<xtsc::xtsc_wire_read_if>", new ScExportCreator<sc_core::sc_export<xtsc::xtsc_wire_read_if> >());
    ScExportFactory::inst().addCreator ("sc_core::sc_export<xtsc::xtsc_wire_write_if>", new ScExportCreator<sc_core::sc_export<xtsc::xtsc_wire_write_if> >());
    ScExportFactory::inst().addCreator ("sc_export<xtsc::xtsc_wire_read_if>", new ScExportCreator<sc_export<xtsc::xtsc_wire_read_if> >());
    ScExportFactory::inst().addCreator ("sc_export<xtsc::xtsc_wire_write_if>", new ScExportCreator<sc_export<xtsc::xtsc_wire_write_if> >());
    ScExportFactory::inst().addCreator ("xtsc::xtsc_wire_read_if_vp_stub", new ScExportCreator<xtsc::xtsc_wire_read_if_vp_stub>());
    ScExportFactory::inst().addCreator ("xtsc::xtsc_wire_write_if_vp_stub", new ScExportCreator<xtsc::xtsc_wire_write_if_vp_stub>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_wire_vp0<1024>", new xtsc_vp__xtsc_wire_vp0Creator<1024>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_wire_vp0<104>", new xtsc_vp__xtsc_wire_vp0Creator<104>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_wire_vp0<10>", new xtsc_vp__xtsc_wire_vp0Creator<10>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_wire_vp0<112>", new xtsc_vp__xtsc_wire_vp0Creator<112>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_wire_vp0<11>", new xtsc_vp__xtsc_wire_vp0Creator<11>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_wire_vp0<120>", new xtsc_vp__xtsc_wire_vp0Creator<120>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_wire_vp0<128>", new xtsc_vp__xtsc_wire_vp0Creator<128>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_wire_vp0<12>", new xtsc_vp__xtsc_wire_vp0Creator<12>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_wire_vp0<13>", new xtsc_vp__xtsc_wire_vp0Creator<13>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_wire_vp0<14>", new xtsc_vp__xtsc_wire_vp0Creator<14>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_wire_vp0<15>", new xtsc_vp__xtsc_wire_vp0Creator<15>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_wire_vp0<16>", new xtsc_vp__xtsc_wire_vp0Creator<16>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_wire_vp0<17>", new xtsc_vp__xtsc_wire_vp0Creator<17>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_wire_vp0<18>", new xtsc_vp__xtsc_wire_vp0Creator<18>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_wire_vp0<19>", new xtsc_vp__xtsc_wire_vp0Creator<19>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_wire_vp0<1>", new xtsc_vp__xtsc_wire_vp0Creator<1>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_wire_vp0<20>", new xtsc_vp__xtsc_wire_vp0Creator<20>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_wire_vp0<21>", new xtsc_vp__xtsc_wire_vp0Creator<21>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_wire_vp0<22>", new xtsc_vp__xtsc_wire_vp0Creator<22>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_wire_vp0<23>", new xtsc_vp__xtsc_wire_vp0Creator<23>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_wire_vp0<24>", new xtsc_vp__xtsc_wire_vp0Creator<24>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_wire_vp0<256>", new xtsc_vp__xtsc_wire_vp0Creator<256>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_wire_vp0<25>", new xtsc_vp__xtsc_wire_vp0Creator<25>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_wire_vp0<26>", new xtsc_vp__xtsc_wire_vp0Creator<26>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_wire_vp0<27>", new xtsc_vp__xtsc_wire_vp0Creator<27>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_wire_vp0<28>", new xtsc_vp__xtsc_wire_vp0Creator<28>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_wire_vp0<29>", new xtsc_vp__xtsc_wire_vp0Creator<29>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_wire_vp0<2>", new xtsc_vp__xtsc_wire_vp0Creator<2>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_wire_vp0<30>", new xtsc_vp__xtsc_wire_vp0Creator<30>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_wire_vp0<31>", new xtsc_vp__xtsc_wire_vp0Creator<31>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_wire_vp0<32>", new xtsc_vp__xtsc_wire_vp0Creator<32>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_wire_vp0<3>", new xtsc_vp__xtsc_wire_vp0Creator<3>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_wire_vp0<40>", new xtsc_vp__xtsc_wire_vp0Creator<40>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_wire_vp0<48>", new xtsc_vp__xtsc_wire_vp0Creator<48>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_wire_vp0<4>", new xtsc_vp__xtsc_wire_vp0Creator<4>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_wire_vp0<50>", new xtsc_vp__xtsc_wire_vp0Creator<50>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_wire_vp0<512>", new xtsc_vp__xtsc_wire_vp0Creator<512>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_wire_vp0<56>", new xtsc_vp__xtsc_wire_vp0Creator<56>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_wire_vp0<5>", new xtsc_vp__xtsc_wire_vp0Creator<5>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_wire_vp0<64>", new xtsc_vp__xtsc_wire_vp0Creator<64>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_wire_vp0<6>", new xtsc_vp__xtsc_wire_vp0Creator<6>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_wire_vp0<72>", new xtsc_vp__xtsc_wire_vp0Creator<72>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_wire_vp0<7>", new xtsc_vp__xtsc_wire_vp0Creator<7>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_wire_vp0<80>", new xtsc_vp__xtsc_wire_vp0Creator<80>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_wire_vp0<88>", new xtsc_vp__xtsc_wire_vp0Creator<88>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_wire_vp0<8>", new xtsc_vp__xtsc_wire_vp0Creator<8>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_wire_vp0<96>", new xtsc_vp__xtsc_wire_vp0Creator<96>());
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_wire_vp0<9>", new xtsc_vp__xtsc_wire_vp0Creator<9>());
    ScPortFactory::inst().addCreator ("sc_core::sc_port<xtsc::xtsc_wire_read_if>", new ScPortCreator<sc_core::sc_port<xtsc::xtsc_wire_read_if> >());
    ScPortFactory::inst().addCreator ("sc_core::sc_port<xtsc::xtsc_wire_write_if>", new ScPortCreator<sc_core::sc_port<xtsc::xtsc_wire_write_if> >());
    ScPortFactory::inst().addCreator ("sc_port<xtsc::xtsc_wire_read_if>", new ScPortCreator<sc_port<xtsc::xtsc_wire_read_if> >());
    ScPortFactory::inst().addCreator ("sc_port<xtsc::xtsc_wire_write_if>", new ScPortCreator<sc_port<xtsc::xtsc_wire_write_if> >());
    if (::getenv("SNPS_SLS_DYNAMIC_LOADER_VERBOSE")) { std::cout << "xtsc_wire_vp/xtsc_vp::xtsc_wire_vp loaded." << std::endl; }
  }
  ~DllAdapter() {
  }
  static DllAdapter sInstance;
};

DllAdapter DllAdapter::sInstance;

}
