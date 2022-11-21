// Copyright (c) 2005-2018 by Cadence Design Systems Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Cadence Design Systems Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Cadence Design Systems Inc.


#include <cerrno>
#include <algorithm>
#include <ostream>
#include <string>
#include <xtsc/xtsc_memory_trace.h>
#include <xtsc/xtsc_arbiter.h>
#include <xtsc/xtsc_core.h>
#include <xtsc/xtsc_dma_engine.h>
#include <xtsc/xtsc_master.h>
#include <xtsc/xtsc_pin2tlm_memory_transactor.h>
#include <xtsc/xtsc_router.h>
#include <xtsc/xtsc_logging.h>

using namespace std;
#if SYSTEMC_VERSION >= 20050601
using namespace sc_core;
#endif
using namespace xtsc;
using log4xtensa::INFO_LOG_LEVEL;
using log4xtensa::VERBOSE_LOG_LEVEL;



xtsc_component::xtsc_memory_trace_parms::xtsc_memory_trace_parms(const xtsc_core&       core,
                                                                 const char            *memory_interface,
                                                                 sc_trace_file         *p_trace_file,
                                                                 u32                    num_ports)
{
  string lc(memory_interface ? memory_interface : "");
  transform(lc.begin(), lc.end(), lc.begin(), ::tolower);
  if (lc == "inbound_pif" || lc == "snoop") {
    lc = "pif";
  }
  xtsc_core::memory_port mem_port = xtsc_core::get_memory_port(lc.c_str(), &core);
  if (!core.has_memory_port(mem_port)) {
    ostringstream oss;
    oss << "xtsc_memory_trace_parms: core '" << core.name() << "' has no " << memory_interface << " memory interface.";
    throw xtsc_exception(oss.str());
  }

  // Infer number of ports if num_ports is 0
  if (!num_ports) {
    u32 mpc = core.get_multi_port_count(mem_port);
    // Normally be single-ported ...
    num_ports = 1;
    // ; however, if this core interface is multi-ported (e.g. has banks or has multiple LD/ST units without CBox) AND ...
    if (mpc > 1) {
      // this is the first port (port 0) of the interface AND ...
      if (xtsc_core::is_multi_port_zero(mem_port)) {
        // if all the other ports of the interface appear to not be connected (this is not reliable--they might be delay bound), then ...
        bool found_a_connected_port = false;
        for (u32 n=1; n<mpc; ++n) {
          xtsc_core::memory_port mem_port_n = core.get_nth_multi_port(mem_port, n);
          if (core.get_request_port(mem_port_n).get_interface()) {
            found_a_connected_port = true;
            break;
          }
        }
        if (!found_a_connected_port) {
          // infer multi-ported
          num_ports = mpc;
        }
      }
    }
  }

  u32   width8          = core.get_memory_byte_width(mem_port);
  bool  big_endian      = core.is_big_endian();

  init(width8, big_endian, p_trace_file, num_ports);
}



xtsc_component::xtsc_memory_trace::xtsc_memory_trace(sc_module_name module_name, const xtsc_memory_trace_parms& trace_parms) :
  sc_module             (module_name),
  xtsc_module           (*(sc_module*)this),
  m_width8              (trace_parms.get_u32("byte_width")),
  m_big_endian          (trace_parms.get_bool("big_endian")),
  m_p_trace_file        ((sc_trace_file*)trace_parms.get_void_pointer("vcd_handle")),
  m_num_ports           (trace_parms.get_non_zero_u32("num_ports")),
  m_num_transfers       (trace_parms.get_u32("num_transfers")),
  m_allow_tracing       (trace_parms.get_bool("allow_tracing")),
  m_enable_tracing      (trace_parms.get_bool("enable_tracing") && m_allow_tracing),
  m_track_latency       (trace_parms.get_bool("track_latency")),
  m_did_track           (m_track_latency),
  m_system_clock_period (xtsc_get_system_clock_period()),
  m_text                (log4xtensa::TextLogger::getInstance(name()))
{

  m_system_clock_period_half = m_system_clock_period / 2;

  if ((m_width8 != 4) && (m_width8 != 8) && (m_width8 != 16) && (m_width8 != 32) && (m_width8 != 64)) {
    ostringstream oss;
    oss << kind() << " '" << name() << "' has illegal \"byte_width\"=" << m_width8 << " (legal values are 4|8|16|32|64)";
    throw xtsc_exception(oss.str());
  }

  sc_trace_file *p_tf = m_p_trace_file;

  if (m_allow_tracing && !m_p_trace_file) {
    m_p_trace_file = sc_create_vcd_trace_file("waveforms");
    if (!m_p_trace_file) {
      throw xtsc_exception("xtsc_memory_trace: creation of VCD file \"waveforms.vcd\" failed");
    }
  }

  m_request_ports       = new sc_port<xtsc_request_if>*  [m_num_ports];
  m_respond_exports     = new sc_export<xtsc_respond_if>*[m_num_ports];
  m_respond_impl        = new xtsc_respond_if_impl*      [m_num_ports];

  m_respond_ports       = new sc_port<xtsc_respond_if>*  [m_num_ports];
  m_request_exports     = new sc_export<xtsc_request_if>*[m_num_ports];
  m_request_impl        = new xtsc_request_if_impl*      [m_num_ports];

  for (u32 i=0; i<m_num_ports; i++) {
    ostringstream oss1, oss2, oss3, oss4, oss5, oss6;

    oss1 << "m_request_ports["   << i << "]";
    oss2 << "m_respond_exports[" << i << "]";
    oss3 << "rsp";

    oss4 << "m_respond_ports["   << i << "]";
    oss5 << "m_request_exports[" << i << "]";
    oss6 << "req";

    if (m_num_ports > 1) {
      oss3 << "(" << i << ")";
      oss6 << "(" << i << ")";
    }

    m_request_ports  [i] = new sc_port<xtsc_request_if>  (oss1.str().c_str());
    m_respond_exports[i] = new sc_export<xtsc_respond_if>(oss2.str().c_str());
    m_respond_impl   [i] = new xtsc_respond_if_impl      (oss3.str().c_str(), *this, i);
    m_respond_ports  [i] = new sc_port<xtsc_respond_if>  (oss4.str().c_str());
    m_request_exports[i] = new sc_export<xtsc_request_if>(oss5.str().c_str());
    m_request_impl   [i] = new xtsc_request_if_impl      (oss6.str().c_str(), *this, i);

    (*m_respond_exports[i])(*m_respond_impl[i]);
    (*m_request_exports[i])(*m_request_impl[i]);

  }

  for (u32 i=0; i<m_num_ports; ++i) {
    ostringstream oss;
    oss.str(""); oss << "m_request_exports" << "[" << i << "]"; m_port_types[oss.str()] = REQUEST_EXPORT;
    oss.str(""); oss << "m_respond_ports"   << "[" << i << "]"; m_port_types[oss.str()] = RESPOND_PORT;
    oss.str(""); oss << "slave_port"        << "[" << i << "]"; m_port_types[oss.str()] = PORT_TABLE;
    oss.str(""); oss << "m_request_ports"   << "[" << i << "]"; m_port_types[oss.str()] = REQUEST_PORT;
    oss.str(""); oss << "m_respond_exports" << "[" << i << "]"; m_port_types[oss.str()] = RESPOND_EXPORT;
    oss.str(""); oss << "master_port"       << "[" << i << "]"; m_port_types[oss.str()] = PORT_TABLE;
  }

  m_port_types["slave_ports" ] = PORT_TABLE;
  m_port_types["master_ports"] = PORT_TABLE;

  if (m_num_ports == 1) {
    m_port_types["slave_port" ] = PORT_TABLE;
    m_port_types["master_port"] = PORT_TABLE;
  }

  m_statistics_maps = new map<type_t, statistic_info*>[m_num_ports];
  for (u32 i=0; i<m_num_ports; ++i) {
    m_statistics_maps[i].insert(pair<type_t, statistic_info*> (xtsc_request::SNOOP,        new statistic_info));
    m_statistics_maps[i].insert(pair<type_t, statistic_info*> (xtsc_request::BURST_WRITE,  new statistic_info));
    m_statistics_maps[i].insert(pair<type_t, statistic_info*> (xtsc_request::BLOCK_WRITE,  new statistic_info));
    m_statistics_maps[i].insert(pair<type_t, statistic_info*> (xtsc_request::WRITE,        new statistic_info));
    m_statistics_maps[i].insert(pair<type_t, statistic_info*> (xtsc_request::RCW,          new statistic_info));
    m_statistics_maps[i].insert(pair<type_t, statistic_info*> (xtsc_request::BURST_READ,   new statistic_info));
    m_statistics_maps[i].insert(pair<type_t, statistic_info*> (xtsc_request::BLOCK_READ,   new statistic_info));
    m_statistics_maps[i].insert(pair<type_t, statistic_info*> (xtsc_request::READ,         new statistic_info));
  }

  xtsc_register_command(*this, *this, "dump_counter_names", 0, 0,
      "dump_counter_names", 
      "Dump a list of valid counter names by calling dump_counter_names()."
  );

  xtsc_register_command(*this, *this, "dump_latencies", 0, 0,
      "dump_latencies", 
      "Dump maximum transaction lifetime and latency.  Deprecated in favor of dump_statistic_info."
  );

  xtsc_register_command(*this, *this, "dump_latency_histogram", 0, 2,
      "dump_latency_histogram [<Types> [<Ports>]]", 
      "Call dump_latency_histogram() for the specified request <Types> and <Ports> (default all)."
  );

  xtsc_register_command(*this, *this, "dump_lifetime_histogram", 0, 2,
      "dump_lifetime_histogram [<Types> [<Ports>]]", 
      "Call dump_lifetime_histogram() for the specified request <Types> and <Ports> (default all)."
  );

  xtsc_register_command(*this, *this, "dump_statistic_info", 0, 2,
      "dump_statistic_info [<Types> [<Ports>]]", 
      "Call statistic_info::dump() for each of the specified request <Types> and <Ports> (default all)."
  );

  xtsc_register_command(*this, *this, "enable_latency_tracking", 1, 1,
      "enable_latency_tracking <Enable>", 
      "Set whether or not latency tracking is enabled."
  );
  
  xtsc_register_command(*this, *this, "enable_tracing", 1, 1,
      "enable_tracing <Enable>", 
      "Set whether or not tracing is enabled."
  );
   
  xtsc_register_command(*this, *this, "get_counter", 1, 3,
      "get_counter <CntrName> [<Types> [<Ports>]]", 
      "Return value from calling xtsc_memory_trace::get_counter(<CntrName>, <Types>, <Ports>)."
  );

  xtsc_register_command(*this, *this, "get_num_ports", 0, 0,
      "get_num_ports", 
      "Return value from calling xtsc_memory_trace::get_num_ports()."
  );

  xtsc_register_command(*this, *this, "reset", 0, 1,
      "reset", 
      "Call xtsc_memory_trace:reset()."
  );
      
  log4xtensa::LogLevel ll = xtsc_get_constructor_log_level();
  XTSC_LOG(m_text, ll,        "Constructed xtsc_memory_trace '" << name() << "':");
  XTSC_LOG(m_text, ll,        " byte_width      = "   << m_width8);
  XTSC_LOG(m_text, ll,        " big_endian      = "   << boolalpha << m_big_endian);
  if (m_allow_tracing) {
  XTSC_LOG(m_text, ll,        " vcd_handle      = "   << p_tf);
  }
  XTSC_LOG(m_text, ll,        " num_ports       = "   << m_num_ports);
  XTSC_LOG(m_text, ll,        " allow_tracing   = "   << boolalpha << m_allow_tracing);
  if (m_allow_tracing) {
  XTSC_LOG(m_text, ll,        " enable_tracing  = "   << boolalpha << m_enable_tracing);
  }
  XTSC_LOG(m_text, ll,        " track_latency   = "   << boolalpha << m_track_latency);
  XTSC_LOG(m_text, ll,        " num_transfers   = "   << m_num_transfers);

  reset(true);

}



xtsc_component::xtsc_memory_trace::~xtsc_memory_trace(void) {

  for (u32 i=0; i<m_num_ports; i++) {
    delete m_request_ports[i];
    delete m_respond_exports[i];
    delete m_respond_ports[i];
    delete m_request_exports[i];
  }

  delete [] m_request_ports;
  delete [] m_respond_exports;
  delete [] m_respond_ports;
  delete [] m_request_exports;

  for (u32 i=0; i<m_num_ports; i++) {
    for (map<type_t, statistic_info*>::iterator it=m_statistics_maps[i].begin(); it!=m_statistics_maps[i].end(); ++it) {
      delete it->second;
    }
    m_statistics_maps[i].clear();
  }
  delete [] m_statistics_maps;
  
  clear_transaction_list();
}



u32 xtsc_component::xtsc_memory_trace::get_bit_width(const string& port_name, u32 interface_num) const {
  return m_width8*8;
}



sc_object *xtsc_component::xtsc_memory_trace::get_port(const string& port_name) {
  string name_portion;
  u32    index;
  bool indexed = xtsc_parse_port_name(port_name, name_portion, index);

  if (((m_num_ports > 1) && !indexed) ||
      ((name_portion != "m_request_exports") &&
       (name_portion != "m_respond_ports")   &&
       (name_portion != "m_request_ports")   &&
       (name_portion != "m_respond_exports")))
  {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\" has no port named \"" << port_name << "\"" << endl;
    throw xtsc_exception(oss.str());
  }

  if (index > m_num_ports - 1) {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\": Invalid index: \"" << port_name << "\".  Valid range: 0-" << (m_num_ports-1) << endl;
    throw xtsc_exception(oss.str());
  }

  if (name_portion == "m_request_exports") return m_request_exports[index];
  if (name_portion == "m_respond_ports")   return m_respond_ports  [index];
  if (name_portion == "m_request_ports")   return m_request_ports  [index];
  if (name_portion == "m_respond_exports") return m_respond_exports[index];

  ostringstream oss;
  oss << kind() << " \"" << name() << "\" has no port named \"" << port_name << "\"" << endl;
  throw xtsc_exception(oss.str());
}



xtsc_port_table xtsc_component::xtsc_memory_trace::get_port_table(const string& port_table_name) const {
  xtsc_port_table table;

  if (port_table_name == "slave_ports") {
    for (u32 i=0; i<m_num_ports; ++i) {
      ostringstream oss;
      oss << "slave_port[" << i << "]";
      table.push_back(oss.str());
    }
    return table;
  }
  
  if (port_table_name == "master_ports") {
    for (u32 i=0; i<m_num_ports; ++i) {
      ostringstream oss;
      oss << "master_port[" << i << "]";
      table.push_back(oss.str());
    }
    return table;
  }

  string name_portion;
  u32    index;
  bool   indexed = xtsc_parse_port_name(port_table_name, name_portion, index);

  if (((m_num_ports > 1) && !indexed) || ((name_portion != "slave_port") && (name_portion != "master_port"))) {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\" has no port table named \"" << port_table_name << "\"" << endl;
    throw xtsc_exception(oss.str());
  }

  if (index > m_num_ports - 1) {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\": Invalid index: \"" << port_table_name << "\".  Valid range: 0-" << (m_num_ports-1)
        << endl;
    throw xtsc_exception(oss.str());
  }

  ostringstream oss;
  if (name_portion == "slave_port") {
    oss.str(""); oss << "m_request_exports" << "[" << index << "]"; table.push_back(oss.str());
    oss.str(""); oss << "m_respond_ports"   << "[" << index << "]"; table.push_back(oss.str());
  }
  else {
    oss.str(""); oss << "m_request_ports"   << "[" << index << "]"; table.push_back(oss.str());
    oss.str(""); oss << "m_respond_exports" << "[" << index << "]"; table.push_back(oss.str());
  }

  return table;
}



void xtsc_component::xtsc_memory_trace::clear_transaction_list() {
  if (!m_pending_transactions.empty()) {
    for (map<u64, transaction_info*>::iterator it=m_pending_transactions.begin(); it!=m_pending_transactions.end(); ++it) {
      delete it->second;
    } 
    m_pending_transactions.clear();
  }
}



void xtsc_component::xtsc_memory_trace::dump_latencies(ostream& os) {
  for (u32 i=0; i<m_num_ports; i++) {
    for (map<type_t, statistic_info*>::iterator it=m_statistics_maps[i].begin(); it!=m_statistics_maps[i].end(); ++it) {
      statistic_info *p_info = it->second;
      if (p_info->m_max_latency_tag > 0 ) {
        os << "Max latency: " << setw(4) << right << setprecision(1) << fixed << p_info->m_max_latency << " - tag="
           << setw(6) << left << p_info->m_max_latency_tag << " - " << setw(11) << xtsc_request::get_type_name(it->first) << " - Port #" << i << endl;
      }
      if (p_info->m_max_lifetime_tag > 0 ) {
        os << "Max lifetime: " << setw(4) << right << setprecision(1) << fixed << p_info->m_max_lifetime << " - tag="
           << setw(6) << left << p_info->m_max_lifetime_tag << " - " << setw(11) << xtsc_request::get_type_name(it->first) << " - Port #" << i << endl;
      }
    }
  } 
}



bool xtsc_component::xtsc_memory_trace::get_types(const string& types, set<type_t>& types_set) {
  bool all_types = ((types == "") || (types == "*"));
  if (!all_types) {
    vector<string> vec;
    xtsc_strtostrvector(types, vec);
    for (vector<string>::iterator i = vec.begin(); i != vec.end(); ++i) {
      types_set.insert(xtsc_request::get_type(*i));
    }
  }
  return all_types;
}



bool xtsc_component::xtsc_memory_trace::get_ports(const string& ports, set<u32>& ports_set) {
  bool all_ports = ((ports == "") || (ports == "*"));
  if (!all_ports) {
    vector<u32> vec;
    xtsc_strtou32vector(ports, vec);
    for (vector<u32>::iterator i = vec.begin() ; i != vec.end(); ++i) {
      ports_set.insert(*i);
    }
  }
  return all_ports;
}



void xtsc_component::xtsc_memory_trace::dump_histogram(ostream& os, const string& types, const string& ports, bool latency) {
  set<type_t> types_set; bool all_types = get_types(types, types_set);
  set<u32>    ports_set; bool all_ports = get_ports(ports, ports_set);
  u64 maximum = 0;
  for (u32 i=0; i<m_num_ports; i++) {
    if (!all_ports && (ports_set.find(i) == ports_set.end())) continue;
    for (map<type_t, statistic_info*>::iterator it=m_statistics_maps[i].begin(); it!=m_statistics_maps[i].end(); ++it) {
      if (!all_types && (types_set.find(it->first) == types_set.end())) continue;
      u64 value = latency ? it->second->m_max_latency : it->second->m_max_lifetime;
      if (value > maximum ) {
        maximum = value;
      }
    }
  }
  for (u64 l=0; l<=maximum; l++) {
    u64 tot = 0;
    for (u32 i=0; i<m_num_ports; i++) {
      if (!all_ports && (ports_set.find(i) == ports_set.end())) continue;
      for (map<type_t, statistic_info*>::iterator it=m_statistics_maps[i].begin(); it!=m_statistics_maps[i].end(); ++it) {
        if (!all_types && (types_set.find(it->first) == types_set.end())) continue;
        map<u64, u64>  histogram = latency ? it->second->m_latency_histogram : it->second->m_lifetime_histogram;
        map<u64, u64>::iterator il = histogram.find(l); 
        if (il != histogram.end()) {
          tot += il->second;
        }
      }
    }
    if (tot) {
      os << l << "," << tot << endl;
    }
  }
}



xtsc::u64 xtsc_component::xtsc_memory_trace::get_counter(const string& cntr_name, const string& types, const string& ports) {
  cntr_type cntr = get_cntr_type(cntr_name);
  set<type_t> types_set; bool all_types = get_types(types, types_set);
  set<u32>    ports_set; bool all_ports = get_ports(ports, ports_set);
  u64 total = 0;
  for (u32 i=0; i<m_num_ports; i++) {
    if (!all_ports && (ports_set.find(i) == ports_set.end())) continue;
    for (map<type_t, statistic_info*>::iterator it=m_statistics_maps[i].begin(); it!=m_statistics_maps[i].end(); ++it) {
      if (!all_types && (types_set.find(it->first) == types_set.end())) continue;
      total += it->second->m_cntrs[cntr];
    }
  }
  return total;
}



void xtsc_component::xtsc_memory_trace::dump_statistic_info(ostream& os, const string& types, const string& ports) {
  set<type_t> types_set; bool all_types = get_types(types, types_set);
  set<u32>    ports_set; bool all_ports = get_ports(ports, ports_set);
  for (u32 i=0; i<m_num_ports; i++) {
    if (!all_ports && (ports_set.find(i) == ports_set.end())) continue;
    for (map<type_t, statistic_info*>::iterator it=m_statistics_maps[i].begin(); it!=m_statistics_maps[i].end(); ++it) {
      if (!all_types && (types_set.find(it->first) == types_set.end())) continue;
      ostringstream oss;
      if (it->second->dump(oss, "  ")) {
        os << "Port #" << i << " " << xtsc_request::get_type_name(it->first) << ": " << endl;
        os << oss.str() << endl;
      }
    }
  }
}



void xtsc_component::xtsc_memory_trace::dump_latency_histogram(ostream& os, const string& types, const string& ports) {
  dump_histogram(os, types, ports, true);
}



void xtsc_component::xtsc_memory_trace::dump_lifetime_histogram(ostream& os, const string& types, const string& ports) {
  dump_histogram(os, types, ports, false);
}



void xtsc_component::xtsc_memory_trace::reset(bool /*hard_reset*/) {
  XTSC_INFO(m_text, "xtsc_memory_trace::reset()");
}



void xtsc_component::xtsc_memory_trace::enable_tracing(bool enable) {
  if (!m_allow_tracing && enable) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': enable_tracing(true) called, but the \"allow_tracing\" parameter was false.";
    throw xtsc_exception(oss.str());
  }
  m_enable_tracing = enable;
}



void xtsc_component::xtsc_memory_trace::enable_latency_tracking(bool enable) { 
  m_track_latency = enable; 
  if (m_track_latency) {
    m_did_track = true;
  }
  else {
    clear_transaction_list();
  }
}  



void xtsc_component::xtsc_memory_trace::execute(const string&           cmd_line, 
                                          const vector<string>&         words,
                                          const vector<string>&         words_lc,
                                          ostream&                      result)
{
  ostringstream res;

  if (false) {
  }
  else if (words[0] == "dump_counter_names") {
    dump_counter_names(res);
  }
  else if (words[0] == "dump_latencies") {
    dump_latencies(res);
  }
  else if (words[0] == "dump_latency_histogram") {
    string types(""); if (words.size() > 1) { types = words[1]; }
    string ports(""); if (words.size() > 2) { ports = words[2]; }
    dump_latency_histogram(res, types, ports);
  }
  else if (words[0] == "dump_lifetime_histogram") {
    string types(""); if (words.size() > 1) { types = words[1]; }
    string ports(""); if (words.size() > 2) { ports = words[2]; }
    dump_lifetime_histogram(res, types, ports);
  }
  else if (words[0] == "dump_statistic_info") {
    string types(""); if (words.size() > 1) { types = words[1]; }
    string ports(""); if (words.size() > 2) { ports = words[2]; }
    dump_statistic_info(res, types, ports);
  }
  else if (words[0] == "enable_latency_tracking") {
    if (words.size() > 1) {    
      bool enable = xtsc_command_argtobool(cmd_line, words, 1);
      enable_latency_tracking(enable);
    }       
  }
  else if (words[0] == "enable_tracing") {
    if (words.size() > 1) {
      bool enable = xtsc_command_argtobool(cmd_line, words, 1);
      enable_tracing(enable);
    }
  }
  else if (words[0] == "get_counter") {
    string types(""); if (words.size() > 2) { types = words[2]; }
    string ports(""); if (words.size() > 3) { ports = words[3]; }
    res << get_counter(words[1], types, ports);
  }
  else if (words[0] == "get_num_ports") {
    res << m_num_ports;
  }
  else if (words[0] == "reset") {
    reset();
  }
  else {
    ostringstream oss;
    oss << name() << "::" << __FUNCTION__ << "() called for unknown command '" << cmd_line << "'.";
    throw xtsc_exception(oss.str());
  }

  result << res.str();
}



void xtsc_component::xtsc_memory_trace::connect(xtsc_arbiter& arbiter, u32 trace_port) {
  if (trace_port >= m_num_ports) {
    ostringstream oss;
    oss << "Invalid trace_port=" << trace_port << " in connect(): " << endl;
    oss << kind() << " '" << name() << "' has " << m_num_ports << " ports numbered from 0 to " << m_num_ports-1 << endl;
    throw xtsc_exception(oss.str());
  }
  arbiter.m_request_port(*m_request_exports[trace_port]);
  (*m_respond_ports[trace_port])(arbiter.m_respond_export);
}



u32 xtsc_component::xtsc_memory_trace::connect(xtsc_core& core, const char *memory_port_name, u32 port_num, bool single_connect) {
  u32 num_connected = 1;
  if (port_num >= m_num_ports) {
    ostringstream oss;
    oss << "Invalid port_num=" << port_num << " in connect(): " << endl;
    oss << kind() << " '" << name() << "' has " << m_num_ports << " ports numbered from 0 to " << m_num_ports-1 << endl;
    throw xtsc_exception(oss.str());
  }
  string lc(memory_port_name ? memory_port_name : "");
  transform(lc.begin(), lc.end(), lc.begin(), ::tolower);
  if (lc == "inbound_pif" || lc == "snoop") {
    (*m_request_ports[port_num])(core.get_request_export(memory_port_name));
    core.get_respond_port(memory_port_name)(*m_respond_exports[port_num]);
  }
  else {
    xtsc_core::memory_port mem_port = xtsc_core::get_memory_port(memory_port_name, &core);
    u32 width8 = core.get_memory_byte_width(mem_port);
    if (m_width8 && (width8 != m_width8)) {
      ostringstream oss;
      oss << "Memory interface data width mismatch: " << endl;
      oss << kind() << " '" << name() << "' is " << m_width8 << " bytes wide, but '" << memory_port_name << "' interface of" << endl;
      oss << "xtsc_core '" << core.name() << "' is " << width8 << " bytes wide.";
      throw xtsc_exception(oss.str());
    }
    core.get_request_port(memory_port_name)(*m_request_exports[port_num]);
    (*m_respond_ports[port_num])(core.get_respond_export(memory_port_name));
    // Should we connect other ports of multi-port interface?
    u32 mpc = core.get_multi_port_count(mem_port);
    if (!single_connect && (port_num+mpc <= m_num_ports) && (mpc > 1) && xtsc_core::is_multi_port_zero(mem_port)) {
      bool found_a_connected_port = false;
      for (u32 n=1; n<mpc && !found_a_connected_port; ++n) {
        xtsc_core::memory_port mem_port_n = core.get_nth_multi_port(mem_port, n);
        // Don't connect if our multi-port or the core's multi-port has already been connected.
        // The get_interface() test is not reliable so we'll catch any errors below and carry on (which
        // works for the OSCI simulator--but maybe not for others).
        if (m_request_impl[port_num+n]->is_connected() || core.get_request_port(mem_port_n).get_interface()) {
          found_a_connected_port = true;
          break;
        }
      }
      if (!found_a_connected_port) {
        for (u32 n=1; n<mpc; ++n) {
          xtsc_core::memory_port mem_port_n = core.get_nth_multi_port(mem_port, n);
          sc_port<xtsc_request_if, NSPP>& multi_port = core.get_request_port(mem_port_n);
          try {
            multi_port(*m_request_exports[port_num+n]);
            (*m_respond_ports[port_num+n])(core.get_respond_export(mem_port_n));
            num_connected += 1;
          } catch (...) {
            XTSC_INFO(m_text, "Core '" << core.name() << "' memory multi-port '" << xtsc_core::get_memory_port_name(mem_port_n) <<
                              "' is already bond.");
            if (n > 1) throw;  // Don't ignore failure after 2nd port (port 1) is connected.
            break;
          }
        }
      }
    }
  }
  return num_connected;
}



void xtsc_component::xtsc_memory_trace::connect(xtsc_dma_engine& dma_engine, u32 trace_port) {
  if (trace_port >= m_num_ports) {
    ostringstream oss;
    oss << "Invalid trace_port=" << trace_port << " in connect(): " << endl;
    oss << kind() << " '" << name() << "' has " << m_num_ports << " ports numbered from 0 to " << m_num_ports-1 << endl;
    throw xtsc_exception(oss.str());
  }
  dma_engine.m_request_port(*m_request_exports[trace_port]);
  (*m_respond_ports[trace_port])(dma_engine.m_respond_export);
}



void xtsc_component::xtsc_memory_trace::connect(xtsc_master& master, u32 trace_port) {
  if (trace_port >= m_num_ports) {
    ostringstream oss;
    oss << "Invalid trace_port=" << trace_port << " in connect(): " << endl;
    oss << kind() << " '" << name() << "' has " << m_num_ports << " ports numbered from 0 to " << m_num_ports-1 << endl;
    throw xtsc_exception(oss.str());
  }
  master.m_request_port(*m_request_exports[trace_port]);
  (*m_respond_ports[trace_port])(master.m_respond_export);
}



u32 xtsc_component::xtsc_memory_trace::connect(xtsc_pin2tlm_memory_transactor&  pin2tlm,
                                               u32                              tran_port,
                                               u32                              trace_port,
                                               bool                             single_connect)
{
  u32 tran_ports = pin2tlm.get_num_ports();
  if (tran_port >= tran_ports) {
    ostringstream oss;
    oss << "Invalid tran_port=" << tran_port << " in connect(): " << endl;
    oss << pin2tlm.kind() << " '" << pin2tlm.name() << "' has " << tran_ports << " ports numbered from 0 to " << tran_ports-1 << endl;
    throw xtsc_exception(oss.str());
  }
  if (trace_port >= m_num_ports) {
    ostringstream oss;
    oss << "Invalid trace_port=" << trace_port << " in connect(): " << endl;
    oss << kind() << " '" << name() << "' has " << m_num_ports << " ports numbered from 0 to " << m_num_ports-1 << endl;
    throw xtsc_exception(oss.str());
  }

  u32 num_connected = 0;

  while ((tran_port < tran_ports) && (trace_port < m_num_ports)) {

    (*pin2tlm.m_request_ports[tran_port])(*m_request_exports[trace_port]);
    (*m_respond_ports[trace_port])(*pin2tlm.m_respond_exports[tran_port]);

    trace_port += 1;
    tran_port += 1;
    num_connected += 1;

    if (single_connect) break;
    if (tran_port >= tran_ports) break;
    if (trace_port >= m_num_ports) break;
  }

  return num_connected;

}



void xtsc_component::xtsc_memory_trace::connect(xtsc_router& router, u32 router_port, u32 trace_port) {
  u32 num_slaves = router.get_num_slaves();
  if (router_port >= num_slaves) {
    ostringstream oss;
    oss << "Invalid router_port=" << router_port << " in connect(): " << endl;
    oss << router.kind() << " '" << router.name() << "' has " << num_slaves << " ports numbered from 0 to " << num_slaves-1 << endl;
    throw xtsc_exception(oss.str());
  }
  if (trace_port >= m_num_ports) {
    ostringstream oss;
    oss << kind() << " '" << name() << "':  Invalid trace_port specified to xtsc_memory_trace:connect():  trace_port=" << trace_port;
    oss << ".  Valid range is 0 to " << (m_num_ports - 1) << ".";
    throw xtsc_exception(oss.str());
  }
  (*router.m_request_ports[router_port])(*m_request_exports[trace_port]);
  (*m_respond_ports[trace_port])(*router.m_respond_exports[router_port]);
}



void xtsc_component::xtsc_memory_trace::end_of_simulation() {
  if (m_did_track) {
    ostringstream oss;
    oss << "statistic_info dump begin {" << endl;
    dump_statistic_info(oss);
    oss << "statistic_info dump end   }";
    xtsc_log_multiline(m_text, INFO_LOG_LEVEL, oss.str());
  }
}



// static
void xtsc_component::xtsc_memory_trace::dump_counter_names(ostream& os) {
  for (xtsc::u32 i = 0; i < cntr_count; ++i) {
    os << get_cntr_type_name((cntr_type)i) << endl;
  }
}



// static
string xtsc_component::xtsc_memory_trace::get_cntr_type_name(cntr_type type) {
  if (type == cntr_transactions) return "transactions";
  if (type == cntr_latency)      return "latency";
  if (type == cntr_lifetime)     return "lifetime";
  if (type == cntr_req_beats)    return "req_beats";
  if (type == cntr_req_busys)    return "req_busys";
  if (type == cntr_rsp_beats)    return "rsp_beats";
  if (type == cntr_rsp_busys)    return "rsp_busys";
  ostringstream oss;
  oss << "Unrecognized cntr_type=" << type;
  throw xtsc_exception(oss.str());
}



// static
xtsc_component::xtsc_memory_trace::cntr_type xtsc_component::xtsc_memory_trace::get_cntr_type(string name) {
  if (name == "transactions")   return cntr_transactions;
  if (name == "latency")        return cntr_latency;
  if (name == "lifetime")       return cntr_lifetime;
  if (name == "req_beats")      return cntr_req_beats;
  if (name == "req_busys")      return cntr_req_busys;
  if (name == "rsp_beats")      return cntr_rsp_beats;
  if (name == "rsp_busys")      return cntr_rsp_busys;
  ostringstream oss;
  oss << "Unrecognized cntr_type name = \"" << name << "\" (try dump_counter_names)";
  throw xtsc_exception(oss.str());
}



xtsc_component::xtsc_memory_trace::transaction_info *xtsc_component::xtsc_memory_trace::new_transaction_info(type_t type) {
  if (m_transaction_pool.empty()) {
    transaction_info *p_transaction_info = new transaction_info(sc_time_stamp(), type);
    return p_transaction_info;
  }
  else {
    transaction_info *p_transaction_info = m_transaction_pool.back();
    m_transaction_pool.pop_back();
    p_transaction_info->m_time_req_beg    = sc_time_stamp();
    p_transaction_info->m_time_req_end    = p_transaction_info->m_time_req_beg;
    p_transaction_info->m_time_rsp_beg    = sc_core::SC_ZERO_TIME;
    p_transaction_info->m_time_rsp_end    = sc_core::SC_ZERO_TIME;
    p_transaction_info->m_type            = type;
    return p_transaction_info;
  }
}



void xtsc_component::xtsc_memory_trace::delete_transaction_info(transaction_info*& p_transaction_info) {
  m_transaction_pool.push_back(p_transaction_info);
  p_transaction_info = 0;
}



xtsc_component::xtsc_memory_trace::transaction_info::transaction_info(sc_time timestamp, type_t type) :
  m_time_req_beg (timestamp),
  m_time_req_end (timestamp),
  m_time_rsp_beg (sc_core::SC_ZERO_TIME),
  m_time_rsp_end (sc_core::SC_ZERO_TIME),
  m_type         (type)
{}  



xtsc_component::xtsc_memory_trace::statistic_info::statistic_info() :
  m_max_latency      (0),
  m_max_lifetime     (0),
  m_max_latency_tag  (0),
  m_max_lifetime_tag (0)
{
  for (u32 i = 0; i < cntr_count; ++i) { m_cntrs[i] = 0; }
}



bool xtsc_component::xtsc_memory_trace::statistic_info::dump(ostream& os, const string& prefix) {
  if (m_cntrs[cntr_transactions] == 0) return false;
  os << prefix << "Counters:";
  for (xtsc::u32 i = 0; i < cntr_count; ++i) {
    cntr_type type = (cntr_type) i;
    os << " " << get_cntr_type_name(type) << "=" << m_cntrs[type];
  }
  float trans = m_cntrs[cntr_transactions];
  os << endl << prefix << "Average per transaction:";
  for (xtsc::u32 i = 0; i < cntr_count; ++i) {
    cntr_type type = (cntr_type) i;
    if (type == cntr_transactions) continue;
    os << " " << get_cntr_type_name(type) << "=" << m_cntrs[type]/trans;
  }
  os << endl << prefix << "Histograms (Format: NumCycles=TranCount):" << endl;
  os << prefix << "latency:  ";
  for (map<u64, u64>::iterator i = m_latency_histogram.begin(); i != m_latency_histogram.end(); ++i) {
    if (i != m_latency_histogram.begin()) { os << ","; }
    os << i->first << "=" << i->second;
  }
  os << " (tag=" << m_max_latency_tag << ")" << endl;
  os << prefix << "lifetime: ";
  for (map<u64, u64>::iterator i = m_lifetime_histogram.begin(); i != m_lifetime_histogram.end(); ++i) {
    if (i != m_lifetime_histogram.begin()) { os << ","; }
    os << i->first << "=" << i->second;
  }
  os << " (tag=" << m_max_lifetime_tag << ")";
  return true;
}



xtsc_component::xtsc_memory_trace::xtsc_request_if_impl::xtsc_request_if_impl(const char               *object_name,
                                                                              xtsc_memory_trace&        trace,
                                                                              u32                       port_num) :
  sc_object     (object_name),
  m_trace       (trace),
  m_p_port      (0),
  m_port_num    (port_num),
  m_data        (m_trace.m_width8*8)
{

  m_nb_request_count            = 0;
  m_address8                    = 0;
  m_data                        = 0;
  m_size8                       = 0;
  m_pif_attribute               = 0xFFFFFFFF;
  m_route_id                    = 0;
  m_type                        = 0;
  m_burst                       = 0;
  m_cache                       = 0;
  m_domain                      = 0;
  m_prot                        = 0;
  m_snoop                       = 0;
  m_num_transfers               = 0;
  m_byte_enables                = 0;
  m_id                          = 0;
  m_priority                    = 0;
  m_last_transfer               = 0;
  m_pc                          = 0;
  m_tag                         = 0;
  m_instruction_fetch           = 0;
  m_hw_address8                 = 0;
  m_transfer_num                = 0;
  m_transaction_id              = 0;

  string prefix(name());

  if (m_trace.m_allow_tracing) {
    u32 b = xtsc_address_bits();
    sc_trace(m_trace.m_p_trace_file, m_nb_request_count,          (prefix+".count"         ).c_str());
    sc_trace(m_trace.m_p_trace_file, m_address8,                  (prefix+".address"       ).c_str(), b);
    sc_trace(m_trace.m_p_trace_file, m_data,                      (prefix+".data"          ).c_str());
    sc_trace(m_trace.m_p_trace_file, m_size8,                     (prefix+".size"          ).c_str());
    sc_trace(m_trace.m_p_trace_file, m_pif_attribute,             (prefix+".pif_attribute" ).c_str());
    sc_trace(m_trace.m_p_trace_file, m_route_id,                  (prefix+".route_id"      ).c_str());
    sc_trace(m_trace.m_p_trace_file, m_type,                      (prefix+".type"          ).c_str());
    sc_trace(m_trace.m_p_trace_file, m_burst,                     (prefix+".burst"         ).c_str());
    sc_trace(m_trace.m_p_trace_file, m_cache,                     (prefix+".cache"         ).c_str());
    sc_trace(m_trace.m_p_trace_file, m_domain,                    (prefix+".domain"        ).c_str());
    sc_trace(m_trace.m_p_trace_file, m_prot,                      (prefix+".prot"          ).c_str());
    sc_trace(m_trace.m_p_trace_file, m_snoop,                     (prefix+".snoop"         ).c_str());
    sc_trace(m_trace.m_p_trace_file, m_num_transfers,             (prefix+".num_transfers" ).c_str());
    sc_trace(m_trace.m_p_trace_file, m_byte_enables,              (prefix+".byte_enables"  ).c_str());
    sc_trace(m_trace.m_p_trace_file, m_id,                        (prefix+".id"            ).c_str());
    sc_trace(m_trace.m_p_trace_file, m_priority,                  (prefix+".priority"      ).c_str());
    sc_trace(m_trace.m_p_trace_file, m_last_transfer,             (prefix+".last_transfer" ).c_str());
    sc_trace(m_trace.m_p_trace_file, m_pc,                        (prefix+".pc"            ).c_str());
    sc_trace(m_trace.m_p_trace_file, m_tag,                       (prefix+".tag"           ).c_str());
    sc_trace(m_trace.m_p_trace_file, m_instruction_fetch,         (prefix+".ifetch"        ).c_str());
    sc_trace(m_trace.m_p_trace_file, m_hw_address8,               (prefix+".hw_address"    ).c_str(), b);
    sc_trace(m_trace.m_p_trace_file, m_transfer_num,              (prefix+".transfer_num"  ).c_str());
    sc_trace(m_trace.m_p_trace_file, m_transaction_id,            (prefix+".transaction_id").c_str());
  }
}



void xtsc_component::xtsc_memory_trace::xtsc_request_if_impl::nb_peek(xtsc_address address8, u32 size8, u8 *buffer) {
  (*m_trace.m_request_ports[m_port_num])->nb_peek(address8, size8, buffer);
}



void xtsc_component::xtsc_memory_trace::xtsc_request_if_impl::nb_poke(xtsc_address address8, u32 size8, const u8 *buffer) {
  (*m_trace.m_request_ports[m_port_num])->nb_poke(address8, size8, buffer);
}



bool xtsc_component::xtsc_memory_trace::xtsc_request_if_impl::nb_peek_coherent(xtsc_address     virtual_address8,
                                                                               xtsc_address     physical_address8,
                                                                               u32              size8,
                                                                               u8              *buffer)
{
  return (*m_trace.m_request_ports[m_port_num])->nb_peek_coherent(virtual_address8, physical_address8, size8, buffer);
}



bool xtsc_component::xtsc_memory_trace::xtsc_request_if_impl::nb_poke_coherent(xtsc_address     virtual_address8,
                                                                               xtsc_address     physical_address8,
                                                                               u32              size8,
                                                                               const u8        *buffer)
{
  return (*m_trace.m_request_ports[m_port_num])->nb_poke_coherent(virtual_address8, physical_address8, size8, buffer);
}



bool xtsc_component::xtsc_memory_trace::xtsc_request_if_impl::nb_fast_access(xtsc_fast_access_request &request) {
  return (*m_trace.m_request_ports[m_port_num])->nb_fast_access(request);
}



void xtsc_component::xtsc_memory_trace::xtsc_request_if_impl::nb_request(const xtsc_request& request) {
  XTSC_DEBUG(m_trace.m_text, request << " Port #" << m_port_num);

  // Update values for tracing
  if (m_trace.m_enable_tracing) {
    m_nb_request_count += 1;
    m_address8                  = request.get_byte_address();
    m_size8                     = request.get_byte_size();
    m_pif_attribute             = request.get_pif_attribute();
    m_route_id                  = request.get_route_id();
    m_type                      = (u8) request.get_type();
    m_burst                     = (u8) request.get_burst_type();
    m_cache                     = request.get_cache();
    m_domain                    = request.get_domain();
    m_prot                      = request.get_prot();
    m_snoop                     = request.get_snoop();
    m_num_transfers             = request.get_num_transfers();
    m_byte_enables              = request.get_byte_enables();
    m_id                        = request.get_id();
    m_priority                  = request.get_priority();
    m_last_transfer             = request.get_last_transfer();
    m_pc                        = request.get_pc();
    m_tag                       = request.get_tag();
    m_instruction_fetch         = request.get_instruction_fetch();
    m_hw_address8               = request.get_hardware_address();
    m_transfer_num              = request.get_transfer_number();
    m_transaction_id            = request.get_transaction_id();
    const u8 *buf               = request.get_buffer();
    m_data = 0;
    for (u32 i=0; i<m_size8; ++i) {
      u32 index = i;
      if (m_trace.m_big_endian) {
        index = m_size8 - 1 - i;
      }
      m_data.range(i*8+7, i*8) = buf[index];
    }
  }

  // Track latency
  if (m_trace.m_track_latency && ((m_trace.m_num_transfers == 0) || (m_trace.m_num_transfers == request.get_num_transfers()))) {
    type_t type = request.get_type();
    map<type_t, statistic_info*>::iterator ix = m_trace.m_statistics_maps[m_port_num].find(type);
    if (ix != m_trace.m_statistics_maps[m_port_num].end()) {
      ix->second->m_cntrs[cntr_req_beats] += 1;
      u64 tag = request.get_tag();
      map<u64, transaction_info*>::iterator it = m_trace.m_pending_transactions.find(tag);
      if (it == m_trace.m_pending_transactions.end()) {
        ix->second->m_cntrs[cntr_transactions] += 1;
        transaction_info *p_info = m_trace.new_transaction_info(type);
        m_trace.m_pending_transactions.insert(pair<u64, transaction_info*>(tag, p_info));
      }
      else {
        it->second->m_time_req_end = sc_time_stamp();
      }
    }
  }
  
  // Forward the call
  (*m_trace.m_request_ports[m_port_num])->nb_request(request);
}



void xtsc_component::xtsc_memory_trace::xtsc_request_if_impl::nb_load_retired(xtsc_address address8) {
  (*m_trace.m_request_ports[m_port_num])->nb_load_retired(address8);
}



void xtsc_component::xtsc_memory_trace::xtsc_request_if_impl::nb_retire_flush() {
  (*m_trace.m_request_ports[m_port_num])->nb_retire_flush();
}



void xtsc_component::xtsc_memory_trace::xtsc_request_if_impl::nb_lock(bool lock) {
  (*m_trace.m_request_ports[m_port_num])->nb_lock(lock);
}



void xtsc_component::xtsc_memory_trace::xtsc_request_if_impl::register_port(sc_port_base& port, const char *if_typename) {
  if (m_p_port) {
    ostringstream oss;
    oss << "Illegal multiple binding detected to xtsc_memory_trace '" << m_trace.name() << "' " << name() << ": " << endl;
    oss << "  " << port.name() << endl;
    oss << "  " << m_p_port->name();
    throw xtsc_exception(oss.str());
  }
  XTSC_INFO(m_trace.m_text, "Binding '" << port.name() << "' to " << name());
  m_p_port = &port;

}



xtsc_component::xtsc_memory_trace::xtsc_respond_if_impl::xtsc_respond_if_impl(const char               *object_name,
                                                                              xtsc_memory_trace&        trace,
                                                                              u32                       port_num) :
  sc_object     (object_name),
  m_trace       (trace),
  m_p_port      (0),
  m_port_num    (port_num),
  m_data        (m_trace.m_width8*8)
{

  m_nb_respond_count    = 0;
  m_rsp_busy_count      = 0;
  m_rsp_ok_count        = 0;
  m_rsp_nacc_count      = 0;
  m_rsp_data_err_count  = 0;
  m_rsp_addr_err_count  = 0;
  m_rsp_a_d_err_count   = 0;
  m_address8            = 0;
  m_data                = 0;
  m_size8               = 0;
  m_route_id            = 0;
  m_status              = 0;
  m_burst               = 0;
  m_id                  = 0;
  m_priority            = 0;
  m_last_transfer       = 0;
  m_pc                  = 0;
  m_tag                 = 0;
  m_snoop               = 0;
  m_snoop_data          = 0;
  m_transfer_num        = 0;
  m_transaction_id      = 0;
  
  
  string prefix(name());

  if (m_trace.m_allow_tracing) {
    u32 b = xtsc_address_bits();
    sc_trace(m_trace.m_p_trace_file, m_nb_respond_count,  (prefix+".count"         ).c_str());
    sc_trace(m_trace.m_p_trace_file, m_rsp_busy_count,    (prefix+".rsp_busy"      ).c_str());
    sc_trace(m_trace.m_p_trace_file, m_rsp_ok_count,      (prefix+".rsp_ok"        ).c_str());
    sc_trace(m_trace.m_p_trace_file, m_rsp_nacc_count,    (prefix+".rsp_nacc"      ).c_str());
    sc_trace(m_trace.m_p_trace_file, m_rsp_data_err_count,(prefix+".rsp_data_err"  ).c_str());
    sc_trace(m_trace.m_p_trace_file, m_rsp_addr_err_count,(prefix+".rsp_addr_err"  ).c_str());
    sc_trace(m_trace.m_p_trace_file, m_rsp_a_d_err_count, (prefix+".rsp_a_d_err"   ).c_str());
    sc_trace(m_trace.m_p_trace_file, m_address8,          (prefix+".address"       ).c_str(), b);
    sc_trace(m_trace.m_p_trace_file, m_data,              (prefix+".data"          ).c_str());
    sc_trace(m_trace.m_p_trace_file, m_size8,             (prefix+".size"          ).c_str());
    sc_trace(m_trace.m_p_trace_file, m_route_id,          (prefix+".route_id"      ).c_str());
    sc_trace(m_trace.m_p_trace_file, m_status,            (prefix+".status"        ).c_str());
    sc_trace(m_trace.m_p_trace_file, m_burst,             (prefix+".burst"         ).c_str());
    sc_trace(m_trace.m_p_trace_file, m_id,                (prefix+".id"            ).c_str());
    sc_trace(m_trace.m_p_trace_file, m_priority,          (prefix+".priority"      ).c_str());
    sc_trace(m_trace.m_p_trace_file, m_last_transfer,     (prefix+".last_transfer" ).c_str());
    sc_trace(m_trace.m_p_trace_file, m_pc,                (prefix+".pc"            ).c_str());
    sc_trace(m_trace.m_p_trace_file, m_tag,               (prefix+".tag"           ).c_str());
    sc_trace(m_trace.m_p_trace_file, m_snoop,             (prefix+".snoop"         ).c_str());
    sc_trace(m_trace.m_p_trace_file, m_snoop_data,        (prefix+".snoop_data"    ).c_str());
    sc_trace(m_trace.m_p_trace_file, m_transfer_num,      (prefix+".transfer_num"  ).c_str());
    sc_trace(m_trace.m_p_trace_file, m_transaction_id,    (prefix+".transaction_id").c_str());
  }
}



bool xtsc_component::xtsc_memory_trace::xtsc_respond_if_impl::nb_respond(const xtsc_response& response) {
  XTSC_DEBUG(m_trace.m_text, response << " Port #" << m_port_num);

  // Forward the call
  bool accepted = (*m_trace.m_respond_ports[m_port_num])->nb_respond(response);

  // Update values for tracing
  if (m_trace.m_enable_tracing) {
    m_nb_respond_count += 1;
    if (!accepted)                                                      m_rsp_busy_count     += 1;
    if (response.get_status() == xtsc_response::RSP_OK)                 m_rsp_ok_count       += 1;
    if (response.get_status() == xtsc_response::RSP_NACC)               m_rsp_nacc_count     += 1;
    if (response.get_status() == xtsc_response::RSP_DATA_ERROR)         m_rsp_data_err_count += 1;
    if (response.get_status() == xtsc_response::RSP_ADDRESS_ERROR)      m_rsp_addr_err_count += 1;
    if (response.get_status() == xtsc_response::RSP_ADDRESS_DATA_ERROR) m_rsp_a_d_err_count  += 1;
    m_address8      = response.get_byte_address();
    m_size8         = response.get_byte_size();
    m_route_id      = response.get_route_id();
    m_status        = (u8) response.get_status();
    m_burst         = (u8) response.get_burst_type();
    m_id            = response.get_id();
    m_priority      = response.get_priority();
    m_last_transfer = response.get_last_transfer();
    m_pc            = response.get_pc();
    m_tag           = response.get_tag();
    m_snoop         = response.is_snoop();
    m_snoop_data    = response.has_snoop_data();
    m_transfer_num  = response.get_transfer_number();
    m_transaction_id= response.get_transaction_id();
    const u8 *buf   = response.get_buffer();
    m_data = 0;
    for (u32 i=0; i<m_size8; ++i) {
      u32 index = i;
      if (m_trace.m_big_endian) {
        index = m_size8 - 1 - i;
      }
      m_data.range(i*8+7, i*8) = buf[index];
    }
  }

  // Track latency
  if (m_trace.m_track_latency) {
    u64 tag = response.get_tag();  
    map<u64, transaction_info*>::iterator it = m_trace.m_pending_transactions.find(tag);
    if (it != m_trace.m_pending_transactions.end()) {
      transaction_info *t_info = it->second;
      map<type_t, statistic_info*>::iterator ix = m_trace.m_statistics_maps[m_port_num].find(t_info->m_type);
      if (ix != m_trace.m_statistics_maps[m_port_num].end()) {
        statistic_info *s_info = ix->second;
        if (response.get_status() == xtsc_response::RSP_NACC) {
          s_info->m_cntrs[cntr_req_busys] += 1;
        }
        else {
          s_info->m_cntrs[cntr_rsp_beats] += 1;
          if (!accepted) {
            s_info->m_cntrs[cntr_rsp_busys] += 1;
          }
          sc_time now = sc_time_stamp();
          if (t_info->m_time_rsp_beg == SC_ZERO_TIME) {
            t_info->m_time_rsp_beg = now;
          }
          if (accepted && response.get_last_transfer()) {
            t_info->m_time_rsp_end = now;
            u64 latency  =(u64)((t_info->m_time_rsp_beg - t_info->m_time_req_end + m_trace.m_system_clock_period_half)/m_trace.m_system_clock_period);
            s_info->m_cntrs[cntr_latency]  += latency;
            map<u64, u64>::iterator il = s_info->m_latency_histogram.find(latency);
            if (il == s_info->m_latency_histogram.end()) {
              s_info->m_latency_histogram[latency]  = 1;
            }
            else {
              s_info->m_latency_histogram[latency] += 1;
            }
            u64 lifetime = (u64)((t_info->m_time_rsp_end - t_info->m_time_req_beg + m_trace.m_system_clock_period_half)/m_trace.m_system_clock_period);
            map<u64, u64>::iterator id = s_info->m_lifetime_histogram.find(lifetime);
            s_info->m_cntrs[cntr_lifetime]  += lifetime;
            if (id == s_info->m_lifetime_histogram.end()) {
              s_info->m_lifetime_histogram[lifetime]  = 1;
            }
            else {
              s_info->m_lifetime_histogram[lifetime] += 1;
            }
            if ((latency > s_info->m_max_latency) || (s_info->m_max_latency_tag == 0)) {
              s_info->m_max_latency      = latency;
              s_info->m_max_latency_tag  = tag;
            } 
            if ((lifetime > s_info->m_max_lifetime) || (s_info->m_max_lifetime_tag == 0)) {
              s_info->m_max_lifetime     = lifetime;
              s_info->m_max_lifetime_tag = tag;
            } 
            m_trace.delete_transaction_info(t_info);
            m_trace.m_pending_transactions.erase(tag);
          }   
        }
      }
    }
  }
  
  return accepted;
}



void xtsc_component::xtsc_memory_trace::xtsc_respond_if_impl::register_port(sc_port_base& port, const char *if_typename) {
  if (m_p_port) {
    ostringstream oss;
    oss << "Illegal multiple binding detected to xtsc_memory_trace '" << m_trace.name() << "' " << name() << ": " << endl;
    oss << "  " << port.name() << endl;
    oss << "  " << m_p_port->name();
    throw xtsc_exception(oss.str());
  }
  XTSC_INFO(m_trace.m_text, "Binding '" << port.name() << "' to " << name());
  m_p_port = &port;
}



