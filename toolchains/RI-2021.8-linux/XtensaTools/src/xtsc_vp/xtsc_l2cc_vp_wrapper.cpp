#include "systemc.h"
#include "xtsc_vp/xtsc_if_vp_stubs.h"
#include "xtsc_vp/xtsc_l2cc_vp.h"
#include "cassert"
#include "cwr_dynamic_loader.h"
#include "cwr_sc_dynamic_stubs.h"
#include "cwr_sc_hierarch_module.h"
#include "cwr_sc_object_creator.h"
#include "scmlinc/scml_abstraction_level_switch.h"
#include "scmlinc/scml_property_registry.h"

namespace xtsc_l2cc_vp_xtsc_vp__xtsc_l2cc_vp_FastBuild {

using namespace conf;
using namespace std;


class xtsc_vp__xtsc_l2cc_vp0Creator : public ScObjectCreatorBase
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
      if (creationVerboseMode() >= 6) { std::cout << "xtsc_l2cc_vp/xtsc_vp::xtsc_l2cc_vp: STUB for " << hierach_name << " created." << std::endl; }
      conf::stub *result = new conf::stub(n);
       cwr_sc_object_registry::inst().addExport(new xtsc::xtsc_request_if_vp_stub(std::string("coredata_rd_req" ).c_str()), string(static_cast<sc_object*>(result)->name()) + ".coredata_rd_req" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_l2cc_vp::coredata_rd_rsp, "coredata_rd_rsp" , string(static_cast<sc_object*>(result)->name()) + ".coredata_rd_rsp" );
       cwr_sc_object_registry::inst().addExport(new xtsc::xtsc_request_if_vp_stub(std::string("coredata_wr_req" ).c_str()), string(static_cast<sc_object*>(result)->name()) + ".coredata_wr_req" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_l2cc_vp::coredata_wr_rsp, "coredata_wr_rsp" , string(static_cast<sc_object*>(result)->name()) + ".coredata_wr_rsp" );
       cwr_sc_object_registry::inst().addExport(new xtsc::xtsc_request_if_vp_stub(std::string("coreinst_rd_req" ).c_str()), string(static_cast<sc_object*>(result)->name()) + ".coreinst_rd_req" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_l2cc_vp::coreinst_rd_rsp, "coreinst_rd_rsp" , string(static_cast<sc_object*>(result)->name()) + ".coreinst_rd_rsp" );
       cwr_sc_object_registry::inst().addExport(new xtsc::xtsc_request_if_vp_stub(std::string("slave_rd_req" ).c_str()), string(static_cast<sc_object*>(result)->name()) + ".slave_rd_req" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_l2cc_vp::slave_rd_rsp, "slave_rd_rsp" , string(static_cast<sc_object*>(result)->name()) + ".slave_rd_rsp" );
       cwr_sc_object_registry::inst().addExport(new xtsc::xtsc_request_if_vp_stub(std::string("slave_wr_req" ).c_str()), string(static_cast<sc_object*>(result)->name()) + ".slave_wr_req" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_l2cc_vp::slave_wr_rsp, "slave_wr_rsp" , string(static_cast<sc_object*>(result)->name()) + ".slave_wr_rsp" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_l2cc_vp::master_rd_req, "master_rd_req" , string(static_cast<sc_object*>(result)->name()) + ".master_rd_req" );
       cwr_sc_object_registry::inst().addExport(new xtsc::xtsc_respond_if_vp_stub(std::string("master_rd_rsp" ).c_str()), string(static_cast<sc_object*>(result)->name()) + ".master_rd_rsp" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_l2cc_vp::master_wr_req, "master_wr_req" , string(static_cast<sc_object*>(result)->name()) + ".master_wr_req" );
       cwr_sc_object_registry::inst().addExport(new xtsc::xtsc_respond_if_vp_stub(std::string("master_wr_rsp" ).c_str()), string(static_cast<sc_object*>(result)->name()) + ".master_wr_rsp" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_l2cc_vp::l2_err_port, "l2_err_port" , string(static_cast<sc_object*>(result)->name()) + ".l2_err_port" );
       conf::stub_port_registrator<>::register_stub_port(&xtsc_vp::xtsc_l2cc_vp::l2_status_port, "l2_status_port" , string(static_cast<sc_object*>(result)->name()) + ".l2_status_port" );
      return result;
    } else {
      if (creationVerboseMode() >= 6) { std::cout << "xtsc_l2cc_vp/xtsc_vp::xtsc_l2cc_vp: " << hierach_name << " created." << std::endl; }
      xtsc_vp::xtsc_l2cc_vp* result = new xtsc_vp::xtsc_l2cc_vp(name.c_str());
       cwr_sc_object_registry::inst().addExport(&result->coredata_rd_req, string(static_cast<sc_object*>(result)->name()) + ".coredata_rd_req" );
       cwr_sc_object_registry::inst().addPort(&result->coredata_rd_rsp, string(static_cast<sc_object*>(result)->name()) + ".coredata_rd_rsp" );
       cwr_sc_object_registry::inst().addExport(&result->coredata_wr_req, string(static_cast<sc_object*>(result)->name()) + ".coredata_wr_req" );
       cwr_sc_object_registry::inst().addPort(&result->coredata_wr_rsp, string(static_cast<sc_object*>(result)->name()) + ".coredata_wr_rsp" );
       cwr_sc_object_registry::inst().addExport(&result->coreinst_rd_req, string(static_cast<sc_object*>(result)->name()) + ".coreinst_rd_req" );
       cwr_sc_object_registry::inst().addPort(&result->coreinst_rd_rsp, string(static_cast<sc_object*>(result)->name()) + ".coreinst_rd_rsp" );
       cwr_sc_object_registry::inst().addExport(&result->slave_rd_req, string(static_cast<sc_object*>(result)->name()) + ".slave_rd_req" );
       cwr_sc_object_registry::inst().addPort(&result->slave_rd_rsp, string(static_cast<sc_object*>(result)->name()) + ".slave_rd_rsp" );
       cwr_sc_object_registry::inst().addExport(&result->slave_wr_req, string(static_cast<sc_object*>(result)->name()) + ".slave_wr_req" );
       cwr_sc_object_registry::inst().addPort(&result->slave_wr_rsp, string(static_cast<sc_object*>(result)->name()) + ".slave_wr_rsp" );
       cwr_sc_object_registry::inst().addPort(&result->master_rd_req, string(static_cast<sc_object*>(result)->name()) + ".master_rd_req" );
       cwr_sc_object_registry::inst().addExport(&result->master_rd_rsp, string(static_cast<sc_object*>(result)->name()) + ".master_rd_rsp" );
       cwr_sc_object_registry::inst().addPort(&result->master_wr_req, string(static_cast<sc_object*>(result)->name()) + ".master_wr_req" );
       cwr_sc_object_registry::inst().addExport(&result->master_wr_rsp, string(static_cast<sc_object*>(result)->name()) + ".master_wr_rsp" );
       cwr_sc_object_registry::inst().addPort(&result->l2_err_port, string(static_cast<sc_object*>(result)->name()) + ".l2_err_port" );
       cwr_sc_object_registry::inst().addPort(&result->l2_status_port, string(static_cast<sc_object*>(result)->name()) + ".l2_status_port" );
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
    ScObjectFactory::inst().addCreator ("xtsc_vp::xtsc_l2cc_vp0", new xtsc_vp__xtsc_l2cc_vp0Creator());
    ScPortFactory::inst().addCreator ("sc_core::sc_port<xtsc::xtsc_request_if>", new ScPortCreator<sc_core::sc_port<xtsc::xtsc_request_if> >());
    ScPortFactory::inst().addCreator ("sc_core::sc_port<xtsc::xtsc_respond_if>", new ScPortCreator<sc_core::sc_port<xtsc::xtsc_respond_if> >());
    ScPortFactory::inst().addCreator ("sc_core::sc_port<xtsc::xtsc_wire_write_if>", new ScPortCreator<sc_core::sc_port<xtsc::xtsc_wire_write_if> >());
    ScPortFactory::inst().addCreator ("sc_port<xtsc::xtsc_request_if>", new ScPortCreator<sc_port<xtsc::xtsc_request_if> >());
    ScPortFactory::inst().addCreator ("sc_port<xtsc::xtsc_respond_if>", new ScPortCreator<sc_port<xtsc::xtsc_respond_if> >());
    ScPortFactory::inst().addCreator ("sc_port<xtsc::xtsc_wire_write_if>", new ScPortCreator<sc_port<xtsc::xtsc_wire_write_if> >());
    if (::getenv("SNPS_SLS_DYNAMIC_LOADER_VERBOSE")) { std::cout << "xtsc_l2cc_vp/xtsc_vp::xtsc_l2cc_vp loaded." << std::endl; }
  }
  ~DllAdapter() {
  }
  static DllAdapter sInstance;
};

DllAdapter DllAdapter::sInstance;

}
