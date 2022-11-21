// Copyright (c) 2014-2018 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Cadence Design Systems, Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Cadence Design Systems, Inc.

// This file is used to add plugin support for all new models added to the XTSC core or component library.
// These models are called builtins.
// The reason for adding plugin support is so that no changes need to be made to xtsc-run in order to support them.

// Instructions:
//  - Using a consistent alphabetical ordering based on plugin_name, add an entry for each builtin to the 5 "Add entry here" locations below.


#if defined(TEN4_INTERNAL)
// Comment out the following macro before checking in
// #define XTSC_TEST_NATIVE_MODELS_AS_BUILTINS
#endif

#include <xtsc/xtsc_comp.h>

// Add entry here #1/5
#include <xtsc/xtsc_axi2pif_transactor.h>
#include <xtsc/xtsc_cache.h>
#include <xtsc/xtsc_ext_regfile.h>
#include <xtsc/xtsc_ext_regfile_pin.h>
#include <xtsc/xtsc_l2cc.h>
#include <xtsc/xtsc_pin2tlm_lookup_transactor.h>
#include <xtsc/xtsc_udma.h>
#include <xtsc/xtsc_txnne.h>
#include <xtsc/xtsc_xnne.h>
#include <xtsc/xtsc_pif2axi_transactor.h>
#include <xtsc/xtsc_reorder_buffer.h>
#include <xtsc/xtsc_mask_payload.h>

#if defined(XTSC_TEST_NATIVE_MODELS_AS_BUILTINS)
#include <xtsc/xtsc_queue_consumer.h>
#include <xtsc/xtsc_wire_logic.h>
#include <xtsc/xtsc_queue_producer.h>
#include <xtsc/xtsc_queue_pin.h>
#include <xtsc/xtsc_wire_source.h>
#include <xtsc/xtsc_wire.h>
#endif


using namespace std;
using namespace xtsc;
using namespace xtsc_component;


#ifndef xtsc_comp
#define xtsc_comp
#undef  xtsc_comp
#endif



namespace xtsc_component {


class xtsc_builtin_plugin_interface : public xtsc_plugin_interface {
public:

  virtual set<string> get_plugin_names() {
    set<string> plugin_names;
    // Add entry here #2/5
    plugin_names.insert("xtsc_axi2pif_transactor");
    plugin_names.insert("xtsc_cache");
    plugin_names.insert("xtsc_ext_regfile");
    plugin_names.insert("xtsc_ext_regfile_pin");
    plugin_names.insert("xtsc_l2cc");
    plugin_names.insert("xtsc_pif2axi_transactor");
    plugin_names.insert("xtsc_pin2tlm_lookup_transactor");
    plugin_names.insert("xtsc_reorder_buffer");
    plugin_names.insert("xtsc_udma");
    plugin_names.insert("xtsc_txnne");
    plugin_names.insert("xtsc_xnne");
    plugin_names.insert("xtsc_mask_payload_comp");
    plugin_names.insert("xtsc_mask_payload_decomp");

#if defined(XTSC_TEST_NATIVE_MODELS_AS_BUILTINS)
    cout << endl << endl
         << "WARNING: xtsc_comp.cpp is NOT ready for check-in because macro XTSC_TEST_NATIVE_MODELS_AS_BUILTINS is defined"
         << endl << endl;
    plugin_names.insert("consumer_plugin");
    plugin_names.insert("logic_plugin");
    plugin_names.insert("producer_plugin");
    plugin_names.insert("queue_pin_plugin");
    plugin_names.insert("source_plugin");
    plugin_names.insert("wire_plugin");
#endif

    return plugin_names;
  }


  virtual void help(const string& plugin_name, bool verbose, ostream& os) {
    // Add entry here #3/5
    if (false) { ; }
    else if (plugin_name == "xtsc_axi2pif_transactor") {
      os << "This transactor converts Xtensa AXI requests to Xtensa PIF requests and the corresponding Xtensa PIF responses"
         << "back to AXI responses." << endl;
    }
    else if (plugin_name == "xtsc_cache") {
      os << "xtsc_cache is a generic cache model." << endl;
    }
    else if (plugin_name == "xtsc_ext_regfile") {
      os << "xtsc_ext_regfile models an external register file with TLM interface." << endl;
    }
    else if (plugin_name == "xtsc_ext_regfile_pin") {
      os << "xtsc_ext_regfile_pin models an external register file with pin interface." << endl;
    }
    else if (plugin_name == "xtsc_l2cc") {
      os << "xtsc_l2cc is a level 2 cache and coherency controller." << endl;
    }
    else if (plugin_name == "xtsc_pif2axi_transactor") {
      os << "This transactor converts Xtensa PIF requests to Xtensa AXI requests and the corresponding Xtensa AXI responses"
         << "back to PIF responses." << endl;
    }
    else if (plugin_name == "xtsc_pin2tlm_lookup_transactor") {
      os << "xtsc_pin2tlm_lookup_transactor is a transactor to convert from a pin-level TIE lookup interface to the corresponding"
         << " Xtensa TLM interface (xtsc_lookup_if)." << endl;
    }
    else if (plugin_name == "xtsc_reorder_buffer") {
      os << "xtsc_reorder_buffer reorders the out of order responses before sending them back to the iDMA AXI bus master." << endl;
    }
    else if (plugin_name == "xtsc_udma") {
      os << "xtsc_udma is a controller that moves data between a core's local data RAMs and memories external to the core."
         << endl;
    }
    else if (plugin_name == "xtsc_txnne") {
      os << "xtsc_txnne is a model of the Tiny Xtensa Neural Network Engine (TXNNE)."
         << endl;
    }    
    else if (plugin_name == "xtsc_xnne") {
      os << "xtsc_xnne is a model of the Xtensa Neural Network Engine (XNNE)."
         << endl;
    }
    else if (plugin_name == "xtsc_mask_payload_comp") {
      os << "xtsc_mask_payload_comp is mask-payload compressor."
         << endl;
    }
    else if (plugin_name == "xtsc_mask_payload_decomp") {
      os << "xtsc_mask_payload_decomp is mask-payload decompressor."
         << endl;
    }
#if defined(XTSC_TEST_NATIVE_MODELS_AS_BUILTINS)
    else if (plugin_name == "consumer_plugin") {
      os << "consumer_plugin is the same as xtsc_queue_consumer documented in xtsc_ug.pdf and xtsc_rm.pdf." << endl;
    }
    else if (plugin_name == "logic_plugin") {
      os << "logic_plugin is the same as xtsc_wire_logic documented in xtsc_ug.pdf and xtsc_rm.pdf." << endl;
    }
    else if (plugin_name == "producer_plugin") {
      os << "producer_plugin is the same as xtsc_queue_producer documented in xtsc_ug.pdf and xtsc_rm.pdf." << endl;
    }
    else if (plugin_name == "queue_pin_plugin") {
      os << "queue_pin_plugin is the same as xtsc_queue_pin documented in xtsc_ug.pdf and xtsc_rm.pdf." << endl;
    }
    else if (plugin_name == "source_plugin") {
      os << "source_plugin is the same as xtsc_wire_source documented in xtsc_ug.pdf and xtsc_rm.pdf." << endl;
    }
    else if (plugin_name == "wire_plugin") {
      os << "wire_plugin is the same as xtsc_wire documented in xtsc_ug.pdf and xtsc_rm.pdf." << endl;
    }
#endif

    else {
      ostringstream oss;
      oss << "xtsc_builtin_plugin_interface::help() called with unknown plugin_name of \"" << plugin_name << "\"";
      throw xtsc_exception(oss.str());
    }
  }


  virtual xtsc_parms *create_parms(const string& plugin_name) {
    // Add entry here #4/5
    if (false) { ; }
    else if (plugin_name == "xtsc_axi2pif_transactor") {
      return new xtsc_axi2pif_transactor_parms;
    }
    else if (plugin_name == "xtsc_cache") {
      return new xtsc_cache_parms;
    }
    else if (plugin_name == "xtsc_ext_regfile") {
      return new xtsc_ext_regfile_parms;
    }
    else if (plugin_name == "xtsc_ext_regfile_pin") {
      return new xtsc_ext_regfile_pin_parms;
    }
    else if (plugin_name == "xtsc_l2cc") {
      return new xtsc_l2cc_parms;
    }
    else if (plugin_name == "xtsc_pif2axi_transactor") {
      return new xtsc_pif2axi_transactor_parms;
    }
    else if (plugin_name == "xtsc_pin2tlm_lookup_transactor") {
      return new xtsc_pin2tlm_lookup_transactor_parms;
    }
    else if (plugin_name == "xtsc_reorder_buffer") {
      return new xtsc_reorder_buffer_parms;
    }
    else if (plugin_name == "xtsc_udma") {
      return new xtsc_udma_parms;
    }
    else if (plugin_name == "xtsc_txnne") {
      return new xtsc_txnne_parms;
    }
    else if (plugin_name == "xtsc_xnne") {
      return new xtsc_xnne_parms;
    }
    else if (plugin_name == "xtsc_mask_payload_comp") {
      return new xtsc_mp_comp_parms;
    }
    else if (plugin_name == "xtsc_mask_payload_decomp") {
      return new xtsc_mp_decomp_parms;
    }
#if defined(XTSC_TEST_NATIVE_MODELS_AS_BUILTINS)
    else if (plugin_name == "consumer_plugin") {
      return new xtsc_queue_consumer_parms;
    }
    else if (plugin_name == "logic_plugin") {
      return new xtsc_wire_logic_parms("");
    }
    else if (plugin_name == "producer_plugin") {
      return new xtsc_queue_producer_parms;
    }
    else if (plugin_name == "queue_pin_plugin") {
      return new xtsc_queue_pin_parms;
    }
    else if (plugin_name == "source_plugin") {
      return new xtsc_wire_source_parms(1, "");
    }
    else if (plugin_name == "wire_plugin") {
      return new xtsc_wire_parms;
    }
#endif

    else {
      ostringstream oss;
      oss << "xtsc_builtin_plugin_interface::create_parms() called with unknown plugin_name of \"" << plugin_name << "\"";
      throw xtsc_exception(oss.str());
    }
  }


  virtual sc_core::sc_module& create_module(const string& plugin_name, const string& instance_name, const xtsc_parms *p_parms) {
    // Add entry here #5/5
    if (false) { ; }
    else if (plugin_name == "xtsc_axi2pif_transactor") {
      xtsc_axi2pif_transactor *p_mod = new xtsc_axi2pif_transactor(instance_name.c_str(), *(xtsc_axi2pif_transactor_parms*)p_parms);
      m_instance_map[p_mod->name()] = p_mod;
      return *p_mod;
    }    
    else if (plugin_name == "xtsc_cache") {
      xtsc_cache *p_mod = new xtsc_cache(instance_name.c_str(), *(xtsc_cache_parms*)p_parms);
      m_instance_map[p_mod->name()] = p_mod;
      return *p_mod;
    }
    else if (plugin_name == "xtsc_ext_regfile") {
      xtsc_ext_regfile *p_mod = new xtsc_ext_regfile(instance_name.c_str(), *(xtsc_ext_regfile_parms*)p_parms);
      m_instance_map[p_mod->name()] = p_mod;
      return *p_mod;
    }
    else if (plugin_name == "xtsc_ext_regfile_pin") {
      xtsc_ext_regfile_pin *p_mod = new xtsc_ext_regfile_pin(instance_name.c_str(), *(xtsc_ext_regfile_pin_parms*)p_parms);
      m_instance_map[p_mod->name()] = p_mod;
      return *p_mod;
    }
    else if (plugin_name == "xtsc_l2cc") {
      xtsc_l2cc *p_mod = new xtsc_l2cc(instance_name.c_str(), *(xtsc_l2cc_parms*)p_parms);
      m_instance_map[p_mod->name()] = p_mod;
      return *p_mod;
    }
    else if (plugin_name == "xtsc_pif2axi_transactor") {
      xtsc_pif2axi_transactor *p_mod = new xtsc_pif2axi_transactor(instance_name.c_str(), *(xtsc_pif2axi_transactor_parms*)p_parms);
      m_instance_map[p_mod->name()] = p_mod;
      return *p_mod;
    }    
    else if (plugin_name == "xtsc_pin2tlm_lookup_transactor") {
      xtsc_pin2tlm_lookup_transactor *p_mod = new xtsc_pin2tlm_lookup_transactor(instance_name.c_str(),
                                                                                 *(xtsc_pin2tlm_lookup_transactor_parms*)p_parms);
      m_instance_map[p_mod->name()] = p_mod;
      return *p_mod;
    }
    else if (plugin_name == "xtsc_reorder_buffer") {
      xtsc_reorder_buffer *p_mod = new xtsc_reorder_buffer(instance_name.c_str(), *(xtsc_reorder_buffer_parms*)p_parms);
      m_instance_map[p_mod->name()] = p_mod;
      return *p_mod;
    }    
    else if (plugin_name == "xtsc_udma") {
      xtsc_udma *p_mod = new xtsc_udma(instance_name.c_str(), *(xtsc_udma_parms*)p_parms);
      m_instance_map[p_mod->name()] = p_mod;
      return *p_mod;
    }
    else if (plugin_name == "xtsc_txnne") {
      xtsc_txnne *p_mod = new xtsc_txnne(instance_name.c_str(), *(xtsc_txnne_parms*)p_parms);
      m_instance_map[p_mod->name()] = p_mod;
      return *p_mod;
    }
    else if (plugin_name == "xtsc_xnne") {
      xtsc_xnne *p_mod = new xtsc_xnne(instance_name.c_str(), *(xtsc_xnne_parms*)p_parms);
      m_instance_map[p_mod->name()] = p_mod;
      return *p_mod;
    }
    else if (plugin_name == "xtsc_mask_payload_comp") {
      xtsc_mask_payload_comp *p_mod = new xtsc_mask_payload_comp(instance_name.c_str(), *(xtsc_mp_comp_parms*)p_parms);
      m_instance_map[p_mod->name()] = p_mod;
      return *p_mod;
    }
    else if (plugin_name == "xtsc_mask_payload_decomp") {
      xtsc_mask_payload_decomp *p_mod = new xtsc_mask_payload_decomp(instance_name.c_str(), *(xtsc_mp_decomp_parms*)p_parms);
      m_instance_map[p_mod->name()] = p_mod;
      return *p_mod;
    }
#if defined(XTSC_TEST_NATIVE_MODELS_AS_BUILTINS)
    else if (plugin_name == "consumer_plugin") {
      xtsc_queue_consumer *p_mod = new xtsc_queue_consumer(instance_name.c_str(), *(xtsc_queue_consumer_parms*)p_parms);
      m_instance_map[p_mod->name()] = p_mod;
      return *p_mod;
    }
    else if (plugin_name == "logic_plugin") {
      xtsc_wire_logic *p_mod = new xtsc_wire_logic(instance_name.c_str(), *(xtsc_wire_logic_parms*)p_parms);
      m_instance_map[p_mod->name()] = p_mod;
      return *p_mod;
    }
    else if (plugin_name == "producer_plugin") {
      xtsc_queue_producer *p_mod = new xtsc_queue_producer(instance_name.c_str(), *(xtsc_queue_producer_parms*)p_parms);
      m_instance_map[p_mod->name()] = p_mod;
      return *p_mod;
    }
    else if (plugin_name == "queue_pin_plugin") {
      xtsc_queue_pin *p_mod = new xtsc_queue_pin(instance_name.c_str(), *(xtsc_queue_pin_parms*)p_parms);
      m_instance_map[p_mod->name()] = p_mod;
      return *p_mod;
    }
    else if (plugin_name == "source_plugin") {
      xtsc_wire_source *p_mod = new xtsc_wire_source(instance_name.c_str(), *(xtsc_wire_source_parms*)p_parms);
      m_instance_map[p_mod->name()] = p_mod;
      return *p_mod;
    }
    else if (plugin_name == "wire_plugin") {
      xtsc_wire *p_mod = new xtsc_wire(instance_name.c_str(), *(xtsc_wire_parms*)p_parms);
      m_instance_map[p_mod->name()] = p_mod;
      return *p_mod;
    }
#endif

    else {
      ostringstream oss;
      oss << "xtsc_builtin_plugin_interface::create_module() called with unknown plugin_name of \"" << plugin_name << "\"";
      throw xtsc_exception(oss.str());
    }
  }


  virtual xtsc_connection_interface *get_connection_interface(const string& hierarchical_name) {
    if (m_instance_map.find(hierarchical_name) == m_instance_map.end()) {
      ostringstream oss;
      oss << "xtsc_builtin_plugin_interface::get_connection_interface() called with unknown hierarchical_name of \""
          << hierarchical_name << "\"";
      throw xtsc_exception(oss.str());
    }
    return m_instance_map[hierarchical_name];
  }


  map<string, xtsc_connection_interface*> m_instance_map;  ///< Map hierarchical name to connection interface
};


} // namespace xtsc_component 




extern "C" XTSC_COMP_API xtsc_plugin_interface& xtsc_get_builtin_plugin_interface() {
  static xtsc_builtin_plugin_interface *p_xtsc_builtin_plugin = 0;
  if (!p_xtsc_builtin_plugin) {
    p_xtsc_builtin_plugin = new xtsc_builtin_plugin_interface;
  }
  return *p_xtsc_builtin_plugin;
}



