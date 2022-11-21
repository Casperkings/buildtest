// Copyright (c) 2018 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of
// Cadence Design Systems, Inc.  They may be adapted and modified by bona fide
// purchasers for internal use, but neither the original nor any adapted
// or modified version may be disclosed or distributed to third parties
// in any manner, medium, or form, in whole or in part, without the prior
// written consent of Cadence Design Systems, Inc.


#include <iostream>
#include <ctype.h>
#include <xtsc/xtsc_cache.h>

using namespace std;
#if SYSTEMC_VERSION >= 20050601
using namespace sc_core;
#endif
using namespace xtsc;


xtsc_component::xtsc_cache::xtsc_cache(const sc_module_name module_name, const xtsc_cache_parms& cache_parms) :
  xtsc_memory                             (module_name, cache_parms),
  m_request_port                          ("m_request_port"),
  m_respond_export                        ("m_respond_export"),
  m_respond_impl                          ("m_respond_impl", *this),
#if IEEE_1666_SYSTEMC >= 201101L
  m_single_response_available_event       ("m_single_response_available_event"),
  m_block_read_response_available_event   ("m_block_read_response_available_event"),
  m_block_write_response_available_event  ("m_block_write_response_available_event"),
#endif
  m_cache_parms                           (cache_parms),
  m_cache_byte_size                       (cache_parms.get_non_zero_u32("cache_byte_size")),
  m_line_byte_width                       (cache_parms.get_non_zero_u32("line_byte_width")),
  m_num_ways                              (cache_parms.get_u32("num_ways")),
  m_read_allocate                         (cache_parms.get_bool("read_allocate")),
  m_write_allocate                        (cache_parms.get_bool("write_allocate")),
  m_write_back                            (cache_parms.get_bool("write_back")),
  m_read_priority                         (cache_parms.get_u32("read_priority")),
  m_write_priority                        (cache_parms.get_u32("write_priority")),
  m_profile_cache                         (cache_parms.get_bool("profile_cache")),
  m_use_pif_attribute                     (cache_parms.get_bool("use_pif_attribute"))
{

  // Check memory parameters ...
  check_memory_parms(cache_parms);

  const char *policy              = cache_parms.get_c_str("replacement_policy");
  string repl_policy              = "";
  int i = 0;
  while (policy[i]) {
    repl_policy += toupper(policy[i++]);
  }
       if (repl_policy == "RR"    )  m_replacement_policy  = REPL_RR;
  else if (repl_policy == "LRU"   )  m_replacement_policy  = REPL_LRU;
  else if (repl_policy == "RANDOM")  m_replacement_policy  = REPL_RANDOM;
  else {
    ostringstream oss;
    oss << kind() << " '" << name() << "': Invalid or unsupported replacement policy (" << policy  << ").";
    throw xtsc_exception(oss.str());
  }
  m_access_byte_width              = m_width8;
  m_bypass_delay                   = m_clock_period * cache_parms.get_u32("bypass_delay");

  // Check cache parameters ...
  if (m_line_byte_width % m_access_byte_width) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': Line width (" << m_line_byte_width << "B) "
        << "must be an integer multiple of access width ("<< m_access_byte_width << "B).";
    throw xtsc_exception(oss.str());
  }
  m_line_access_ratio              = m_line_byte_width / m_access_byte_width;
  if ((m_line_access_ratio != 1) &&
      (m_line_access_ratio != 2) &&
      (m_line_access_ratio != 4) &&
      (m_line_access_ratio != 8) &&
      (m_line_access_ratio != 16)) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': Invalid \"line_byte_width\" parameter value (" << m_line_byte_width
        <<"). The ratio of line width to access width must be 1|2|4|8|16.";
    throw xtsc_exception(oss.str());
  }

  m_lru_selected                   = (m_replacement_policy == REPL_LRU) ? true : false;

  // Configure cache structure ...
  configure();

  // Initialize cache lines ...
  initialize();

  // Initialize remaining members
  m_line_buffer                    = new u8[m_line_byte_width];
  m_line_buffer_index              = 0;
  m_cacheable                      = true;
  m_bufferable                     = true;

  m_read_count                     = 0;
  m_read_miss_count                = 0;
  m_write_count                    = 0;
  m_write_miss_count               = 0;
  m_block_read_count               = 0;
  m_block_read_miss_count          = 0;
  m_block_write_count              = 0;
  m_block_write_miss_count         = 0;

  // Bind the export to its implementation
  m_respond_export(m_respond_impl);

  m_port_types["m_request_port"  ] = REQUEST_PORT;
  m_port_types["m_respond_export"] = RESPOND_EXPORT;
  m_port_types["master_port"     ] = PORT_TABLE;

#if IEEE_1666_SYSTEMC < 201101L
  xtsc_event_register(m_single_response_available_event,       "m_single_response_available_event",       this);
  xtsc_event_register(m_block_read_response_available_event,   "m_block_read_response_available_event",   this);
  xtsc_event_register(m_block_write_response_available_event,  "m_block_write_response_avialable_event",  this);
#endif

  xtsc_register_command(*this, *this, "clear_profile_results", 0 , 0,
                        "clear_profile_results",
                        "Clear cache accesses profile results."
                        );

  xtsc_register_command(*this, *this, "dump_config", 0 , 0,
                        "dump_config",
                        "Dump cache configuration."
                        );

  xtsc_register_command(*this, *this, "dump_profile_results", 0 , 0,
                        "dump_profile_results",
                        "Dump cache accesses profile results."
                        );

  xtsc_register_command(*this, *this, "flush", 0 , 0,
                        "flush",
                        "Flush the entire cache."
                        );

  xtsc_register_command(*this, *this, "flush_dirty_lines", 0 , 0,
                        "flush_dirty_lines",
                        "Flush dirty lines only."
                        );

  xtsc_unregister_command(*this, "reset");

  xtsc_register_command(*this, *this, "reset", 0, 1,
                        "reset [<Hard>]",
                        "Call xtsc_cache::reset(<Hard>).  Where <Hard> is 0|1 (default 0)."
                        );

  xtsc_register_command(*this, *this, "set_profile_cache", 0 , 1,
                        "set_profile_cache <Enable>",
                        "Call xtsc_cache::set_profile_cache(<Enable>). Where <Enable> is 0|1."
                        );

  log4xtensa::LogLevel ll = xtsc_get_constructor_log_level();
  XTSC_LOG(m_text, ll, "Constructed xtsc_cache '" << name() << "':");
  XTSC_LOG(m_text, ll, " cache_byte_size         =   " << m_cache_byte_size << " = 0x" << hex << m_cache_byte_size);
  XTSC_LOG(m_text, ll, " line_byte_width         =   " << m_line_byte_width);
  XTSC_LOG(m_text, ll, " num_ways                =   " << m_num_ways);
  XTSC_LOG(m_text, ll, " read_allocate           =   " << boolalpha << m_read_allocate);
  XTSC_LOG(m_text, ll, " write_allocate          =   " << boolalpha << m_write_allocate);
  XTSC_LOG(m_text, ll, " write_back              =   " << boolalpha << m_write_back);
  XTSC_LOG(m_text, ll, " replacement_policy      =   " << policy);
  XTSC_LOG(m_text, ll, " read_priority           =   " << (u32)m_read_priority);
  XTSC_LOG(m_text, ll, " write_priority          =   " << (u32)m_write_priority);
  XTSC_LOG(m_text, ll, " profile_cache           =   " << boolalpha << m_profile_cache);
  XTSC_LOG(m_text, ll, " use_pif_attribute       =   " << boolalpha << m_use_pif_attribute);
  XTSC_LOG(m_text, ll, " bypass_delay            =   " << cache_parms.get_u32("bypass_delay"));

  ostringstream oss;
  dump_config(oss);
  XTSC_INFO(m_text, oss.str());

  reset();
}



xtsc_component::xtsc_cache::~xtsc_cache(void) {
  XTSC_DEBUG(m_text, "In ~xtsc_cache()");
  for (u32 i = 0; i < m_num_sets; i++) {
    delete [] m_lines[i];
  }
  delete [] m_lines;
  delete [] m_line_buffer;
  if (m_lru_selected) {
    for (u32 i = 0; i < m_num_sets; i++)
      delete [] m_lru[i];
    delete [] m_lru;
  }
}



void xtsc_component::xtsc_cache::end_of_simulation() {
  if (xtsc_is_text_logging_enabled() && m_text.isEnabledFor(log4xtensa::INFO_LOG_LEVEL)) {
    ostringstream oss;
    dump_profile_results(oss);
    if (oss.str() != "") {
      xtsc_log_multiline(m_text, log4xtensa::INFO_LOG_LEVEL, oss.str());
    }
  }
}



u32 xtsc_component::xtsc_cache::get_bit_width(const string& port_name, u32 interface_num) const {
  return m_access_byte_width*8;
}



sc_object *xtsc_component::xtsc_cache::get_port(const string& port_name) {
       if (port_name == "m_request_port")   return &m_request_port;
  else if (port_name == "m_respond_export") return &m_respond_export;
  return xtsc_memory::get_port(port_name);
}



xtsc_port_table xtsc_component::xtsc_cache::get_port_table(const string& port_table_name) const {
  if (port_table_name == "master_port") {
    xtsc_port_table table;
    table.push_back("m_request_port");
    table.push_back("m_respond_export");
    return table;
  }
  return xtsc_memory::get_port_table(port_table_name);
}



void xtsc_component::xtsc_cache::check_memory_parms(const xtsc_cache_parms &cache_parms) {
  u32 num_ports                       = cache_parms.get_u32("num_ports");
  if (num_ports != 1) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': Multi-port cache is not supported (number of slave ports = " << num_ports << ").";
    oss << " The default value of 'num_ports' should not be changed.";
    throw xtsc_exception(oss.str());
  }
  u32 byte_width                      = cache_parms.get_u32("byte_width");
  if (byte_width == 0) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': The value of 'byte_width' cannot be zero (" << byte_width << ").";
    throw xtsc_exception(oss.str());
  }
  u64 start_byte_address              = cache_parms.get_u64("start_byte_address");
  if (start_byte_address) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': The value of 'start_byte_address' must be zero (0x" << hex << start_byte_address << ").";
    oss << " The default value of 'start_byte_address' should not be changed.";
    throw xtsc_exception(oss.str());
  }
  u64 memory_byte_size                = cache_parms.get_u64("memory_byte_size");
  if (memory_byte_size) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': The value of 'memory_byte_size' must be zero (0x" << hex << memory_byte_size << "B).";
    oss << " The default value of 'memory_byte_size' should not be changed.";
    throw xtsc_exception(oss.str());
  }
  const char   *initial_value_file    = cache_parms.get_c_str("initial_value_file");
  if (initial_value_file) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': The value of 'initial_value_file' parameter must be NULL.";
    oss << " The 'initial_value_file' parameter is not supported for an xtsc_cache object.";
    throw xtsc_exception(oss.str());
  }
  u32 memory_fill_byte                = cache_parms.get_u32("memory_fill_byte");
  if (memory_fill_byte) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': The value of 'memory_fill_byte' must be zero (0x" << hex << memory_fill_byte << ").";
    oss << " The 'memory_fill_byte' parameter is not supported for an xtsc_cache object.";
    throw xtsc_exception(oss.str());
  }
  bool read_only                      = cache_parms.get_bool("read_only");
  if (read_only) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': The value of 'read_only' must be false.";
    oss << " The 'read_only' parameter is not supported for an xtsc_cache object.";
    throw xtsc_exception(oss.str());
  }
  bool use_raw_access                 = cache_parms.get_bool("use_raw_access");
  if (use_raw_access) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': The value of 'use_raw_access' must be false.";
    oss << " Raw access is not supported for an xtsc_cache object.";
    throw xtsc_exception(oss.str());
  }
  const char *script_file             = cache_parms.get_c_str("script_file");
  if (script_file) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': The value of 'script_file' must be NULL.";
    oss << " The 'script_file' parameter is not supported for an xtsc_cache object.";
    throw xtsc_exception(oss.str());
  }
  u32 fail_request_mask               = cache_parms.get_u32("fail_request_mask");
  if (fail_request_mask) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': The value of 'fail_request_mask' parameter must be zero (0x" << hex << fail_request_mask << ").";
    oss << " The 'fail_request_mask' parameter is not supported for an xtsc_cache object.";
    throw xtsc_exception(oss.str());
  }
}



void xtsc_component::xtsc_cache::clear_profile_results() {
  m_read_count               = 0;
  m_read_miss_count          = 0;
  m_write_count              = 0;
  m_write_miss_count         = 0;
  m_block_read_count         = 0;
  m_block_read_miss_count    = 0;
  m_block_write_count        = 0;
  m_block_write_miss_count   = 0;
}



void xtsc_component::xtsc_cache::compute_delays() {
  m_bypass_delay             = m_clock_period * m_cache_parms.get_u32("bypass_delay");
  xtsc_memory::compute_delays();
}



void xtsc_component::xtsc_cache::configure() {
  m_num_lines             = m_cache_byte_size / m_line_byte_width;
  if (m_num_ways == 0) /* Direct-mapped cache model */
    m_num_ways            = m_num_lines;
  if ((m_cache_byte_size % m_line_byte_width) || (m_num_lines == 0) || (m_num_lines % m_num_ways)) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': Cache byte size (" <<  m_cache_byte_size << "B) must be an interger multiple of";
    oss << " set byte size (" << (m_num_ways * m_line_byte_width) << "B).";
    throw xtsc_exception(oss.str());
  }
  m_num_sets              = m_num_lines / m_num_ways;
  m_line_byte_width_log2  = 0;
  u32 shift_value         = m_line_byte_width;
  for (u32 i = 0; i < 32; ++i) {
    if (shift_value & 0x1) m_line_byte_width_log2 = i;
    shift_value >>= 1;
  }
  m_num_sets_log2         = 0;
  shift_value             = m_num_sets;
  for (u32 i = 0; i < 32; ++i) {
    if (shift_value & 0x1) m_num_sets_log2 = i;
    shift_value >>= 1;
  }
  if ((1U << m_num_sets_log2) != m_num_sets) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': Total number of sets (" << dec << m_num_sets << ") is not a power of 2.";
    throw xtsc_exception(oss.str());
  }

  m_set_byte_size         = m_line_byte_width * m_num_ways;
  m_set_shift             = m_line_byte_width_log2;
  m_set_mask              = m_num_sets - 1;
  m_tag_shift             = m_num_sets_log2 + m_set_shift;

  // Memory allocate cache lines ...
  m_lines                 = new struct line_info* [m_num_sets];
  for (u32 i = 0; i < m_num_sets; i++) {
    m_lines[i]            = new struct line_info  [m_num_ways];
  }
  if (m_lru_selected) {
    m_lru                  = new u32* [m_num_sets];
    for (u32 i = 0; i < m_num_sets; i++) {
      m_lru[i]             = new u32  [m_num_ways];
    }
  }
  XTSC_DEBUG(m_text, "tag_shift=" << m_tag_shift << ", set_mask=0x" << hex << m_set_mask << ", set_shift=" << dec << m_set_shift << ", num_ways =" << m_num_ways << ", num_sets=" << m_num_sets);
}



void xtsc_component::xtsc_cache::dump_config(ostream& os) const {
  os  << m_cache_byte_size << " bytes (" << ( m_cache_byte_size/1024 ) << "KB), ";
  if (m_num_ways == 1) {
    os << "direct-mapped, ";
  }
  else if (m_num_ways == m_num_lines) {
    os << "fully-associative, ";
  }
  else {
    os << m_num_ways << "-way set associative, ";
  }
  os << m_line_byte_width << "-byte line, ";
  if (m_write_back) {
    os << "write-back";
  }
  else {
    os << "write-through";
  }
  if (m_write_allocate) {
    os << ", write-allocate";
  }
}



void xtsc_component::xtsc_cache::dump_profile_results(ostream &os) const {
  double miss_rate;
  if (m_read_count) {
    miss_rate = ((double)m_read_miss_count / m_read_count)*100;
    os << "READ        miss rate=" << setw(6) << right << setprecision(2) << fixed << miss_rate << "% - # READ        miss="
       << setw(6) << left << m_read_miss_count << " - # READ=" << m_read_count << endl;
  }
  if (m_block_read_count) {
    miss_rate = ((double)m_block_read_miss_count / m_block_read_count)*100;
    os << "BLOCK_READ  miss rate=" << setw(6) << right << setprecision(2) << fixed << miss_rate << "% - # BLOCK_READ  miss="
       << setw(6) << left << m_block_read_miss_count << " - # BLOCK_READ=" << m_block_read_count << endl;
  }
  if (m_write_count) {
    miss_rate = ((double)m_write_miss_count / m_write_count)*100;
    os << "WRITE       miss rate=" << setw(6) << right << setprecision(2) << fixed << miss_rate << "% - # WRITE       miss="
       << setw(6) << left << m_write_miss_count << " - # WRITE=" << m_write_count << endl;
  }
  if (m_block_write_count) {
    miss_rate = ((double)m_block_write_miss_count / m_block_write_count)*100;
    os << "BLOCK_WRITE miss rate=" << setw(6) << right << setprecision(2) << fixed << miss_rate << "% - # BLOCK_WRITE miss="
       << setw(6) << left << m_block_write_miss_count << " - # BLOCK_WRITE=" << m_block_write_count << endl;
  }
  u64 total = m_read_count + m_write_count + m_block_read_count + m_block_write_count;
  if (total) {
    u64 misses =  m_read_miss_count + m_write_miss_count + m_block_read_miss_count + m_block_write_miss_count;
    miss_rate = ((double)misses / total)*100;
    os << "Total       miss rate=" << setw(6) << right << setprecision(2) << fixed << miss_rate << "% - # Total       miss="
     << setw(6) << left << misses << " - # Accesses=" << total;
  }
}



void xtsc_component::xtsc_cache::execute(const string&                 cmd_line,
                                         const vector<string>&         words,
                                         const vector<string>&         words_lc,
                                         ostream&                      result)
{
  ostringstream res;

  if (false) {
  }
  else if (words[0] == "change_clock_period") {
    u32 clock_period_factor = xtsc_command_argtou32(cmd_line, words, 1);
    res << m_clock_period.value();
    change_clock_period(clock_period_factor);
  }
  else if (words[0] == "clear_profile_results") {
    clear_profile_results();
  }
  else if (words[0] == "dump") {
    u64 start_address = xtsc_command_argtou64(cmd_line, words, 1);
    u32 num_bytes     = xtsc_command_argtou32(cmd_line, words, 2);
    byte_dump(start_address, num_bytes, res);
  }
  else if (words[0] == "dump_config") {
    dump_config(res);
  }
  else if (words[0] == "dump_profile_results") {
    dump_profile_results(res);
  }
  else if (words[0] == "flush") {
    flush();
  }
  else if (words[0] == "flush_dirty_lines") {
    u32 lines = flush_dirty_lines();
    res << " " << lines << " dirty line(s) flushed.";
  }
  else if (words[0] == "peek") {
    u64 start_address = xtsc_command_argtou64(cmd_line, words, 1);
    u32 num_bytes     = xtsc_command_argtou32(cmd_line, words, 2);
    u8 *buffer = new u8[num_bytes];
    peek(start_address, num_bytes, buffer);
    ostringstream oss;
    oss << setfill('0') << hex;
    for (u32 i=0; i<num_bytes; ++i) {
      oss << "0x" << setw(2) << (u32) buffer[i];
      if (i < num_bytes-1) {
        oss << " ";
      }
    }
    res << oss.str();
    delete [] buffer;
  }
  else if (words[0] == "poke") {
    u64 start_address = xtsc_command_argtou64(cmd_line, words, 1);
    u32 num_bytes     = xtsc_command_argtou32(cmd_line, words, 2);
    if (words.size() != num_bytes+3) {
      ostringstream oss;
      oss << "Command '" << cmd_line << "' specifies num_bytes=" << num_bytes << " but contains " << (words.size()-3)
          << " byte arguments.";
      throw xtsc_exception(oss.str());
    }
    if (num_bytes) {
      u8 *buffer = new u8[num_bytes];
      for (u32 i=0; i<num_bytes; ++i) {
        buffer[i] = (u8) xtsc_command_argtou32(cmd_line, words, 3+i);
      }
      poke(start_address, num_bytes, buffer);
      delete [] buffer;
    }
  }
  else if (words[0] == "reset") {
    reset((words.size() == 1) ? false : xtsc_command_argtobool(cmd_line, words, 1));
  }
  else if (words[0] == "set_profile_cache") {
    bool enable = xtsc_command_argtobool(cmd_line, words, 1);
    set_profile_cache(enable);
    res << " profile_cache = " << boolalpha << m_profile_cache;
  }
  else {
    ostringstream oss;
    oss << name() << "::" << __FUNCTION__ << "() called for unknown command '" << cmd_line << "'.";
    throw xtsc_exception(oss.str());
  }
  result << res.str();
}



void xtsc_component::xtsc_cache::flush() {
  flush_dirty_lines();
  initialize();
}



u32 xtsc_component::xtsc_cache::flush_dirty_lines() {
  u32 flushed_lines = 0;
  for (u32 set_num = 0; set_num < m_num_sets; set_num++) {
    for (u32 way_num = 0; way_num < m_num_ways; way_num++) {
      if (is_valid(set_num, way_num) && is_dirty(set_num, way_num)) {
        xtsc_address cache_line_address    = set_num * m_set_byte_size + way_num * m_line_byte_width;
        xtsc_memory::peek(cache_line_address, m_line_byte_width, m_line_buffer);
        xtsc_address mem_line_address      = (get_tag(set_num, way_num) << m_tag_shift) | (set_num  << m_set_shift);
        m_request_port->nb_poke(mem_line_address, m_line_byte_width, m_line_buffer);
        m_lines[set_num][way_num].valid    = false;
        m_lines[set_num][way_num].dirty    = false;
        m_lines[set_num][way_num].lrf      = m_lines[set_num][0].lrf;
        if (m_lru_selected)
          m_lru[set_num][way_num]          = m_num_ways - 1;
        flushed_lines++;
      }
    }
  }
  XTSC_DEBUG(m_text, "Total number of flushed dirty lines = " << flushed_lines);
  return flushed_lines;
}



void xtsc_component::xtsc_cache::initialize() {
  for (u32 set_num = 0; set_num < m_num_sets; set_num++) {
    for (u32 way_num = 0; way_num < m_num_ways; way_num++) {
      m_lines[set_num][way_num].tag      = 0;
      m_lines[set_num][way_num].valid    = false;
      m_lines[set_num][way_num].dirty    = false;
      m_lines[set_num][way_num].lrf      = false;
    }
  }
  if (m_lru_selected) {
    for (u32 set_num = 0; set_num < m_num_sets; set_num++)
      for (u32 way_num = 0; way_num < m_num_ways; way_num++)
        m_lru[set_num][way_num]          = (m_num_ways - way_num - 1);
  }
}



void xtsc_component::xtsc_cache::do_block_read(u32 port_num) {
  if (m_profile_cache) m_block_read_count++;
  xtsc_request  *p_request              = &m_p_active_request_info[port_num]->m_request;
  if (p_request->get_num_transfers() > m_line_access_ratio) {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\": Unable to block read address=0x" << hex << p_request->get_byte_address()
        << ". The block transfer size is larger than the cache line size.";
    throw xtsc_exception(oss.str());
  }
  xtsc_address   memory_address       = p_request->get_byte_address();
  xtsc_address   address_tag          = get_tag(memory_address);
  u32            set_num              = get_set(memory_address);
  u32            way                  = find_hit(address_tag, set_num);

  if (way < m_num_ways) { /* Cache hit */
    u32          line_offset          = memory_address & (m_line_byte_width - 1);
    xtsc_address cached_data_address  = set_num * m_set_byte_size + way * m_line_byte_width + line_offset;
    p_request->set_byte_address(cached_data_address);
    xtsc_memory::do_block_read(port_num);
  }
  else {
    XTSC_DEBUG(m_text, "Cache BLOCK_READ miss on address = 0x" << hex << memory_address);
    if (m_profile_cache) m_block_read_miss_count++;
    if (m_read_allocate) {
      // Allocate on read miss
      way = find_replace_line(set_num);
      // Update the line - critical word only
      update_line(memory_address, set_num, way, true);
      // Update line state
      set_tag(address_tag, set_num, way);
      update_lrf(set_num, way);
      if (!is_valid(set_num, way))
        set_valid(set_num, way);

      // do_block_read() w/ critical word first
      u32          transfered_blocks    = 1;
      u32          num_transfers        = p_request->get_num_transfers();
      u32          transfer_bytes       = num_transfers * m_access_byte_width;
      u32          line_offset          = memory_address & (m_line_byte_width - 1);
      xtsc_address block_start_address  = memory_address & ~(u64)(transfer_bytes - 1);
      xtsc_address block_start_offset   = block_start_address & (m_line_byte_width - 1);
      xtsc_address line_base_addr       = set_num * m_set_byte_size + way * m_line_byte_width;
      // Send the critical word first
      m_p_active_response[port_num]->set_last_transfer(false);
      m_p_active_response[port_num]->set_buffer(m_p_block_read_response[0]->get_buffer());
      send_response(port_num, m_p_memory->m_log_data_binary);
      // Transfer the rest of the line
      for (u32 transfer_count = 1; transfer_count < m_line_access_ratio; transfer_count++) {
        // Wait for the next response from the lower-level memory
        while (m_block_read_response_count <= transfer_count) {
          wait(m_block_read_response_available_event);
        }
        xtsc_response::status_t status = m_p_block_read_response[transfer_count]->get_status();
        if (status != xtsc_response::RSP_OK) {
          ostringstream oss;
          oss << kind() << " \"" << name() << "\": Unable to block read source address=0x" << hex << memory_address
              << ".  status=" << xtsc_response::get_status_name(status);
          throw xtsc_exception(oss.str());
        }
        line_offset = (line_offset + m_access_byte_width) % m_line_byte_width;
        xtsc_memory::poke((line_base_addr+line_offset), m_access_byte_width, m_p_block_read_response[transfer_count]->get_buffer());

        if ((line_offset >= block_start_offset) && (line_offset < (block_start_offset + transfer_bytes))) {
          // Send the BLOCK_READ response to the upper-level memory
          if (!m_immediate_timing) {
            wait(m_block_read_repeat);
          }
          bool        last_transfer     = ((transfered_blocks + 1) == num_transfers);
          m_p_active_response[port_num]->set_last_transfer(last_transfer);
          m_p_active_response[port_num]->set_buffer(m_p_block_read_response[transfer_count]->get_buffer());
          send_response(port_num, m_p_memory->m_log_data_binary);
          transfered_blocks++;
        }
      }
    }
    else {
      // Do not allocate on read miss, bypass cache
      ext_mem_read(port_num);
    }
  }
}



void xtsc_component::xtsc_cache::do_block_write(u32 port_num) {
  xtsc_request  *p_request           = &m_p_active_request_info[port_num]->m_request;
  xtsc_address   memory_address      = p_request->get_byte_address();
  u32            line_offset         = memory_address & (m_line_byte_width - 1);
  u32            transfer_bytes      = p_request->get_num_transfers() * p_request->get_byte_size();
  if (m_block_write_transfer_count[port_num] == 0) {
    if (m_profile_cache) m_block_write_count++;
    // Chech this condition only for the first transfer
    if ((line_offset + transfer_bytes) > m_line_byte_width) {
      ostringstream oss;
      oss << kind() << " \"" << name() << "\": Unable to block write address=0x" << hex << memory_address
          << ". The block transfer passes cache line boundary.";
      throw xtsc_exception(oss.str());
    }
  }
  xtsc_address   address_tag         = get_tag(memory_address);
  u32            set_num             = get_set(memory_address);
  u32            way                 = find_hit(address_tag, set_num);

  if (way == m_num_ways) { /* Cache miss */
    XTSC_DEBUG(m_text, "Cache BLOCK_WRITE miss on address = 0x" << hex << memory_address);
    if (m_profile_cache && (m_block_write_transfer_count[port_num] == 0)) m_block_write_miss_count++;
    if (m_write_allocate) {
      // Allocate on write miss
      way = find_replace_line(set_num);
      // Update line data
      update_line(memory_address, set_num, way);
      // Update line status
      set_tag(address_tag, set_num, way);
      update_lrf(set_num, way);
      if (!is_valid(set_num, way))
        set_valid(set_num, way);
    }
    else {
      // Only write to the lower-level memory, do not allocate
      ext_mem_block_write(port_num);
      return;
    }
  }

  if (!m_write_back || (m_use_pif_attribute && !m_bufferable)) {
    // Write through or !bufferable .... also write to the lower-level memory
    ext_mem_block_write(port_num, false);
  }
  else {
    set_dirty(set_num, way);
  }

  // Write cache data
  xtsc_address cached_data_address  = set_num * m_set_byte_size + way * m_line_byte_width + line_offset;
  p_request->set_byte_address(cached_data_address);
  xtsc_memory::do_block_write(port_num);
}



void xtsc_component::xtsc_cache::do_burst_read(xtsc::u32 port_num) {
  ostringstream oss;
  oss << kind() << " \"" << name() << "\": Does not support BURST_READ.";
  throw xtsc_exception(oss.str());
}



void xtsc_component::xtsc_cache::do_burst_write(xtsc::u32 port_num) {
  ostringstream oss;
  oss << kind() << " \"" << name() << "\": Does not support BURST_WRITE.";
  throw xtsc_exception(oss.str());
}



void xtsc_component::xtsc_cache::do_rcw(xtsc::u32 port_num) {
  xtsc_request  *p_request             = &m_p_active_request_info[port_num]->m_request;
  if (!m_rcw_have_first_transfer[port_num]) {
    // If the data is in cache, invalidate the corresponding cache line
    xtsc_address memory_address        = p_request->get_byte_address();
    xtsc_address address_tag           = get_tag(memory_address);
    u32          set_num               = get_set(memory_address);
    u32          way                   = find_hit(address_tag, set_num);

    if ((way < m_num_ways) && m_write_back && is_dirty(set_num, way)) {
      // Write this line to the lower-level memory
      xtsc_address cache_line_address  = set_num * m_set_byte_size + way * m_line_byte_width;
      xtsc_memory::peek(cache_line_address, m_line_byte_width, m_line_buffer);
      xtsc_address mem_line_address    = (address_tag << m_tag_shift) | (set_num  << m_set_shift);
      if (m_line_access_ratio  == 1) {
        xtsc_byte_enables byte_enables = (1U << m_line_byte_width) - 1;
        ext_mem_write(mem_line_address, m_line_byte_width, byte_enables, m_line_buffer);
      }
      else
        ext_mem_block_write(mem_line_address, m_line_byte_width, m_line_buffer);
      // Clear status bits
      clear_dirty(set_num, way);
      clear_valid(set_num, way);
      m_lines[set_num][way].lrf        = m_lines[set_num][0].lrf;
      if (m_lru_selected)
        m_lru[set_num][way]            = m_num_ways - 1;
    }
  }
  // Forward the RCW request to the lower-level memory
  ext_mem_rcw(port_num);
}



void xtsc_component::xtsc_cache::do_read(u32 port_num) {
  if (m_profile_cache) m_read_count++;
  xtsc_request  *p_request             = &m_p_active_request_info[port_num]->m_request;
  xtsc_address   memory_address        = p_request->get_byte_address();
  xtsc_address   address_tag           = get_tag(memory_address);
  u32            set_num               = get_set(memory_address);
  u32            way                   = find_hit(address_tag, set_num);

  if (way < m_num_ways) { /* Cache hit */
    // Read cached data
    u32          line_offset           = memory_address & (m_line_byte_width - 1);
    xtsc_address line_base_address     = set_num * m_set_byte_size + way * m_line_byte_width;
    p_request->set_byte_address((line_base_address+line_offset));
    xtsc_memory::do_read(port_num);
  }
  else { /* Cache miss */
    XTSC_DEBUG(m_text, "Cache READ miss on address = 0x" << hex << memory_address);
    if (m_profile_cache) m_read_miss_count++;
    if (m_read_allocate) {
      // Allocate on read miss
      way = find_replace_line(set_num);
      // Read address must be aligned to the m_access_byte_width
      xtsc_address aligned_mem_addr    = memory_address & ~(u64)(m_access_byte_width - 1);
      // Supports critical word first in missed READ accesses
      update_line(aligned_mem_addr, set_num, way, true);
      // Update line state
      set_tag(address_tag, set_num, way);
      update_lrf(set_num, way);
      if (!is_valid(set_num, way))
        set_valid(set_num, way);

      // Send READ response to the upper-level memory
      u32          line_offset         = memory_address & (m_line_byte_width - 1);
      xtsc_address line_base_address   = set_num * m_set_byte_size + way * m_line_byte_width;
      p_request->set_byte_address((line_base_address+line_offset));
      xtsc_memory::do_read(port_num);

      // Update the rest of the cache line with the data recieved from the lower-level memory
      line_offset = aligned_mem_addr & (m_line_byte_width - 1);
      for (u32 transfer_count = 1; transfer_count < m_line_access_ratio; transfer_count++) {
        // Wait for BLOCK_READ response
        while (m_block_read_response_count <= transfer_count) {
          wait(m_block_read_response_available_event);
        }
        // Copy the read data into the cache line
        xtsc_response::status_t status = m_p_block_read_response[transfer_count]->get_status();
        if (status != xtsc_response::RSP_OK) {
          ostringstream oss;
          oss << kind() << " \"" << name() << "\": Unable to block read source address=0x" << hex << memory_address
              << ".  status=" << xtsc_response::get_status_name(status);
          throw xtsc_exception(oss.str());
        }
        // Update line offset
        line_offset = (line_offset + m_access_byte_width) % m_line_byte_width;
        xtsc_memory::poke((line_base_address+line_offset), m_access_byte_width, m_p_block_read_response[transfer_count]->get_buffer());
      }
    }
    else {
      // Do not allocate on read miss, bypass cache
      ext_mem_read(port_num);
    }
  }
}



void xtsc_component::xtsc_cache::do_write(u32 port_num) {
  if (m_profile_cache) m_write_count++;
  xtsc_request  *p_request           = &m_p_active_request_info[port_num]->m_request;
  xtsc_address   memory_address      = p_request->get_byte_address();
  xtsc_address   address_tag         = get_tag(memory_address);
  u32            set_num             = get_set(memory_address);
  u32            way                 = find_hit(address_tag, set_num);

  if (way == m_num_ways) { /* Cache miss */
    XTSC_DEBUG(m_text, "Cache WRITE miss on address = 0x" << hex << memory_address);
    if (m_profile_cache) m_write_miss_count++;
    if (m_write_allocate) {
      // Allocate on write miss
      way = find_replace_line(set_num);
      // Update line data
      update_line(memory_address, set_num, way);
      // Update line status
      set_tag(address_tag, set_num, way);
      update_lrf(set_num, way);
      if (!is_valid(set_num, way))
        set_valid(set_num, way);
    }
    else {
      // Only update the lower-level memory, do not allocate
      ext_mem_write(port_num);
      return;
    }
  }

  if (!m_write_back || (m_use_pif_attribute && !m_bufferable)) {
    // Write through or !bufferable .... write to the lower-level memory
    ext_mem_write(memory_address, p_request->get_byte_size(), p_request->get_byte_enables(), p_request->get_buffer());
  }
  else {
    set_dirty(set_num, way);
  }

  // Write cache data
  u32           line_offset         = memory_address & (m_line_byte_width - 1);
  xtsc_address  cached_data_address = set_num * m_set_byte_size + way * m_line_byte_width + line_offset;
  p_request->set_byte_address(cached_data_address);
  xtsc_memory::do_write(port_num);
}



void xtsc_component::xtsc_cache::do_bypass(u32 port_num) {
  xtsc_request  *p_request      = &m_p_active_request_info[port_num]->m_request;
  m_p_active_response[port_num] = new xtsc_response(*p_request);
  switch (p_request->get_type()) {
  case xtsc_request::READ:         ext_mem_read(port_num);        break;
  case xtsc_request::BLOCK_READ:   ext_mem_block_read(port_num);  break;
  case xtsc_request::RCW:          ext_mem_rcw(port_num);         break;
  case xtsc_request::WRITE:        ext_mem_write(port_num);       break;
  case xtsc_request::BLOCK_WRITE:  ext_mem_block_write(port_num); break;
  default:
    ostringstream oss;
    oss << kind() << " '" << name() << "': ";
    oss << "Invalid xtsc_request::type_t in nb_request().";
    throw xtsc_exception(oss.str());
  }
}



void xtsc_component::xtsc_cache::ext_mem_block_read(xtsc_address           src_mem_addr,
                                                    u32                    size,
                                                    u8                    *dst_data,
                                                    bool                   first_transfer_only,
                                                    u32                    port_num)
{
  xtsc_request  *p_request    = &m_p_active_request_info[port_num]->m_request;
  m_num_block_transfers       = size / m_access_byte_width;
  m_request.initialize(xtsc_request::BLOCK_READ,  // type
                       src_mem_addr,              // address
                       m_access_byte_width,       // size
                       0,                         // tag  (0 => XTSC assigns)
                       m_num_block_transfers,     // num_transfers
                       0xFFFF,                    // byte_enables (ignored)
                       true,                      // last_transfer
                       0,                         // route_id
                       m_block_read_id,           // id
                       m_read_priority            // priority
                       );
  // Copy other information
  m_request.set_pc(p_request->get_pc());
  m_request.set_instruction_fetch(p_request->get_instruction_fetch());
  m_request.set_pif_attribute(p_request->get_pif_attribute());
  m_request.set_user_data(p_request->get_user_data());

  u32 tries = 0;
  do {
    m_block_read_response_count = 0;
    if (m_p_block_read_response[0]) {
      delete m_p_block_read_response[0];
      m_p_block_read_response[0] = 0;
    }
    tries += 1;
    XTSC_INFO(m_text, m_request << " Try #" << tries);
    m_request_port->nb_request(m_request);
    wait(m_clock_period);
  } while (m_p_block_read_response[0] && (m_p_block_read_response[0]->get_status() == xtsc_response::RSP_NACC));

  for (u32 transfer_count = 0; transfer_count < m_num_block_transfers; transfer_count++) {
    // Wait for BLOCK_READ response
    while (m_block_read_response_count <= transfer_count) {
      wait(m_block_read_response_available_event);
    }
    // Copy the read data into the cache line buffer.
    xtsc_response::status_t status = m_p_block_read_response[transfer_count]->get_status();
    if (status != xtsc_response::RSP_OK) {
      ostringstream oss;
      oss << kind() << " \"" << name() << "\": Unable to block read source address=0x" << hex << src_mem_addr
          << ".  status=" << xtsc_response::get_status_name(status);
      throw xtsc_exception(oss.str());
    }
    memcpy(&dst_data[transfer_count*m_access_byte_width], m_p_block_read_response[transfer_count]->get_buffer(), m_access_byte_width);
    if (first_transfer_only && !transfer_count)
      return;
  }
}



void xtsc_component::xtsc_cache::ext_mem_block_read(u32 port_num) {
  xtsc_request  *p_request    = &m_p_active_request_info[port_num]->m_request;
  m_block_read_response_count = 0;
  m_num_block_transfers       = p_request->get_num_transfers();
  p_request->set_id(m_block_read_id);
  u32 tries = 0;
  do {
    if (m_p_block_read_response[0]) {
      delete m_p_block_read_response[0];
      m_p_block_read_response[0] = 0;
    }
    tries += 1;
    XTSC_INFO(m_text, *p_request << " Try #" << tries);
    m_request_port->nb_request(*p_request);
    wait(m_clock_period);
  } while (m_p_block_read_response[0] && (m_p_block_read_response[0]->get_status() == xtsc_response::RSP_NACC));

  for (u32 transfer_count = 0; transfer_count < m_num_block_transfers; transfer_count++) {
    // Wait for BLOCK_READ response
    while (m_block_read_response_count <= transfer_count) {
      wait(m_block_read_response_available_event);
    }
    // Copy the read data into the cache line buffer.
    m_p_active_response[port_num]->set_status(m_p_block_read_response[transfer_count]->get_status());
    m_p_active_response[port_num]->set_buffer(m_p_block_read_response[transfer_count]->get_buffer());
    m_p_active_response[port_num]->set_user_data(m_p_block_read_response[transfer_count]->get_user_data());
    m_p_active_response[port_num]->set_last_transfer(m_p_block_read_response[transfer_count]->get_last_transfer());
    send_response(port_num, m_p_memory->m_log_data_binary);
    if (m_p_block_read_response[transfer_count]->get_last_transfer())
      break;
  }
}



void xtsc_component::xtsc_cache::ext_mem_block_write(xtsc_address          dst_mem_addr,
                                                     u32                   size,
                                                     const u8             *src_data,
                                                     u32                   port_num)
{
  xtsc_request  *p_request  = &m_p_active_request_info[port_num]->m_request;
  xtsc_address mem_address  = dst_mem_addr;
  u32 request_sent_count    = 0;
  u64 tag                   = 0;
  m_num_block_transfers     = size / m_access_byte_width;
  while (request_sent_count < m_num_block_transfers) {
    if (request_sent_count == 0) {
      // First request of the BLOCK_WRITE
      m_request.initialize(xtsc_request::BLOCK_WRITE,     // type
                           mem_address,                   // address
                           m_access_byte_width,           // size
                           0,                             // tag  (0 => XTSC assigns)
                           m_num_block_transfers,         // num_transfers
                           0xFFFF,                        // byte_enables (ignored)
                           false,                         // last_transfer
                           0,                             // route_id
                           m_block_write_id,              // id
                           m_write_priority               // priority
                           );
      tag = m_request.get_tag();
    }
    else {
      bool last = ((request_sent_count + 1) == m_num_block_transfers);
      m_request.initialize(tag,                           // tag
                           mem_address,                   // address
                           m_access_byte_width,           // size
                           m_num_block_transfers,         // num_transfers
                           last,                          // last_transfer
                           0,                             // route_id
                           m_block_write_id,              // id
                           m_write_priority               // priority
                           );
    }

    // Copy other information from current active request
    m_request.set_pc(p_request->get_pc());
    m_request.set_instruction_fetch(p_request->get_instruction_fetch());
    m_request.set_pif_attribute(p_request->get_pif_attribute());
    m_request.set_user_data(p_request->get_user_data());

    memcpy(m_request.get_buffer(), &src_data[request_sent_count*m_access_byte_width], m_access_byte_width);

    u32 tries = 0;
    do {
      if (m_p_block_write_response) {
        delete m_p_block_write_response;
        m_p_block_write_response = 0;
      }
      tries += 1;
      XTSC_INFO(m_text, m_request << "Try #" << tries);
      m_request_port->nb_request(m_request);
      wait(m_clock_period);
    } while (m_p_block_write_response && (m_p_block_write_response->get_status() == xtsc_response::RSP_NACC));

    request_sent_count     += 1;
    mem_address            += m_access_byte_width;
  }

  if (!m_p_block_write_response) {
    wait(m_block_write_response_available_event);
  }

  xtsc_response::status_t status = m_p_block_write_response->get_status();
  if (m_use_pif_attribute && !m_bufferable) {
    m_p_active_response[port_num]->set_status(status);
    m_p_active_response[port_num]->set_user_data(m_p_block_write_response->get_user_data());
  }
  else {
    if (status != xtsc_response::RSP_OK) {
      ostringstream oss;
      oss << kind() << " \"" << name() << "\": Unable to block write destination address=0x" << hex << dst_mem_addr
          << ".  status=" << xtsc_response::get_status_name(status);
      throw xtsc_exception(oss.str());
    }
  }
}



void xtsc_component::xtsc_cache::ext_mem_block_write(u32 port_num, bool cache_bypass) {
  xtsc_request  *p_request    = &m_p_active_request_info[port_num]->m_request;
  p_request->set_id(m_block_write_id);
  u32 tries = 0;
  do {
    if (m_p_block_write_response) {
      delete m_p_block_write_response;
      m_p_block_write_response = 0;
    }
    tries += 1;
    XTSC_INFO(m_text, *p_request << "Try #" << tries);
    m_request_port->nb_request(*p_request);
    wait(m_clock_period);
  } while (m_p_block_write_response && (m_p_block_write_response->get_status() == xtsc_response::RSP_NACC));

  bool last_transfer = p_request->get_last_transfer();
  if (cache_bypass) {
    do_block_write_transfer_count(p_request, port_num, last_transfer);
  }
  if (last_transfer) {
    if (!m_p_block_write_response) {
      wait(m_block_write_response_available_event);
    }
    m_p_active_response[port_num]->set_status(m_p_block_write_response->get_status());
    m_p_active_response[port_num]->set_user_data(m_p_block_write_response->get_user_data());
    if (cache_bypass) {
      send_response(port_num, false);
    }
  }
}



void xtsc_component::xtsc_cache::ext_mem_read(xtsc_address                 src_mem_addr,
                                              u32                          size,
                                              xtsc_byte_enables            byte_enables,
                                              u8                          *dst_data,
                                              u32                          port_num)
{
  xtsc_request  *p_request    = &m_p_active_request_info[port_num]->m_request;
  m_request.initialize(xtsc_request::READ,        // type
                       src_mem_addr,              // address
                       size,                      // size
                       0,                         // tag  (0 => XTSC assigns)
                       1,                         // num_transfers
                       byte_enables,              // byte_enables
                       true,                      // last_transfer
                       0,                         // route_id
                       m_read_id,                 // id
                       m_read_priority            // priority
                       );
  // Copy other information
  m_request.set_pc(p_request->get_pc());
  m_request.set_instruction_fetch(p_request->get_instruction_fetch());
  m_request.set_pif_attribute(p_request->get_pif_attribute());
  m_request.set_user_data(p_request->get_user_data());

  u32 tries = 0;
  do {
    tries += 1;
    XTSC_INFO(m_text, m_request << "Try #" << tries);
    m_request_port->nb_request(m_request);
    wait(m_single_response_available_event);
  } while (m_p_single_response->get_status() == xtsc_response::RSP_NACC);

  xtsc_response::status_t status = m_p_single_response->get_status();
  if (status == xtsc_response::RSP_OK) {
    memcpy(dst_data, m_p_single_response->get_buffer(), size);
  }
  else {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\": Unable to read source address=0x" << hex << src_mem_addr
        << ".  status=" << xtsc_response::get_status_name(status);
    throw xtsc_exception(oss.str());
  }
}



void xtsc_component::xtsc_cache::ext_mem_read(u32 port_num) {
  xtsc_request  *p_request    = &m_p_active_request_info[port_num]->m_request;
  p_request->set_id(m_read_id);
  u32 tries = 0;
  do {
    tries += 1;
    XTSC_INFO(m_text, *p_request << "Try #" << tries);
    m_request_port->nb_request(*p_request);
    wait(m_single_response_available_event);
  } while (m_p_single_response->get_status() == xtsc_response::RSP_NACC);

  m_p_active_response[port_num]->set_status(m_p_single_response->get_status());
  m_p_active_response[port_num]->set_buffer(m_p_single_response->get_buffer());
  m_p_active_response[port_num]->set_user_data(m_p_single_response->get_user_data());
  send_response(port_num, m_p_memory->m_log_data_binary);
}



void xtsc_component::xtsc_cache::ext_mem_rcw(u32 port_num) {
  xtsc_request  *p_request    = &m_p_active_request_info[port_num]->m_request;
  p_request->set_id(m_rcw_id);
  // Send RCW request to the lower-level memory
  u32 tries = 0;
  do {
    if (m_p_single_response) {
      delete m_p_single_response;
      m_p_single_response = 0;
    }
    tries += 1;
    XTSC_INFO(m_text, *p_request << "Try #" << tries);
    m_request_port->nb_request(*p_request);
    wait(m_clock_period);
  } while (m_p_single_response && (m_p_single_response->get_status() == xtsc_response::RSP_NACC));

  if (!m_rcw_have_first_transfer[port_num]) {
    m_rcw_have_first_transfer[port_num] = true;
  }
  else {
    m_rcw_have_first_transfer[port_num] = false;
    if (!m_p_single_response) {
      wait(m_single_response_available_event);
    }
    // Copy the data from the lower-level memory
    u8 *response_buffer = m_p_active_response[port_num]->get_buffer();
    memcpy(response_buffer, m_p_single_response->get_buffer(), p_request->get_byte_size());
    // Copy the response status
    m_p_active_response[port_num]->set_status(m_p_single_response->get_status());
    m_p_active_response[port_num]->set_user_data(m_p_single_response->get_user_data());
    send_response(port_num, m_p_memory->m_log_data_binary);
  }
}



void xtsc_component::xtsc_cache::ext_mem_write(xtsc_address                dst_mem_addr,
                                               u32                         size,
                                               xtsc_byte_enables           byte_enables,
                                               const u8                   *src_data,
                                               u32                         port_num)
{
  xtsc_request  *p_request    = &m_p_active_request_info[port_num]->m_request;
  m_request.initialize(xtsc_request::WRITE,       // type
                       dst_mem_addr,              // address
                       size,                      // size
                       0,                         // tag  (0 => XTSC assigns)
                       1,                         // num_transfers
                       byte_enables,              // byte_enables
                       true,                      // last_transfer
                       0,                         // route_id
                       m_write_id,                // id
                       m_write_priority           // priority
                       );

  // Copy other information
  m_request.set_pc(p_request->get_pc());
  m_request.set_instruction_fetch(p_request->get_instruction_fetch());
  m_request.set_pif_attribute(p_request->get_pif_attribute());
  m_request.set_user_data(p_request->get_user_data());

  memcpy(m_request.get_buffer(), src_data, size);

  u32 tries = 0;
  do {
    tries += 1;
    XTSC_INFO(m_text, m_request << "Try #" << tries);
    m_request_port->nb_request(m_request);
    wait(m_single_response_available_event);
  } while (m_p_single_response->get_status() == xtsc_response::RSP_NACC);

  xtsc_response::status_t status = m_p_single_response->get_status();
  if (m_use_pif_attribute && !m_bufferable) {
    m_p_active_response[port_num]->set_status(status);
    m_p_active_response[port_num]->set_user_data(m_p_single_response->get_user_data());
  }
  else {
    if (status != xtsc_response::RSP_OK) {
      ostringstream oss;
      oss << kind() << " \"" << name() << "\": Unable to write destination address=0x" << hex << dst_mem_addr
          << ".  status=" << xtsc_response::get_status_name(status);
      throw xtsc_exception(oss.str());
    }
  }
}



void xtsc_component::xtsc_cache::ext_mem_write(u32 port_num) {
  xtsc_request  *p_request    = &m_p_active_request_info[port_num]->m_request;
  p_request->set_id(m_write_id);
  u32 tries = 0;
  do {
    tries += 1;
    XTSC_INFO(m_text, *p_request << "Try #" << tries);
    m_request_port->nb_request(*p_request);
    wait(m_single_response_available_event);
  } while (m_p_single_response->get_status() == xtsc_response::RSP_NACC);

  m_p_active_response[port_num]->set_status(m_p_single_response->get_status());
  m_p_active_response[port_num]->set_user_data(m_p_single_response->get_user_data());
  send_response(port_num, false);
}



u32 xtsc_component::xtsc_cache::find_hit(xtsc_address address_tag, u32 set_num) {
  u32 way = 0;
  for (way = 0; way < m_num_ways; way++) {
    if (is_valid(set_num, way) && (get_tag(set_num, way) == address_tag)) {
      XTSC_DEBUG(m_text, "Cache HIT on set " << set_num << ", line " << way << ", tag = 0x" << hex << address_tag);
      if (m_lru_selected)
        update_lru(set_num, m_lru[set_num][way]);
      return way;
    }
  }
  return way;
}



u32 xtsc_component::xtsc_cache::find_replace_line(u32 set_num) {
  if (m_num_ways == 0) return 0; // Direct-mapped cache model
  u32 way = 0;
  for (way = 0; way < m_num_ways; way++)
    if (!is_valid(set_num, way)) {
      if (m_lru_selected)
        update_lru(set_num, m_lru[set_num][way]);
      return way;
    }

  switch(m_replacement_policy) {
  case REPL_RANDOM:
    way = find_replace_random(set_num);
    break;
  case REPL_RR:
    way = find_replace_rr(set_num);
    break;
  case REPL_LRU:
    way = find_replace_lru(set_num);
    break;
  default:
    way = find_replace_rr(set_num);
  }
  XTSC_DEBUG(m_text, "Cache conflict MISS: set " << set_num << ", line " << way << " replaced.");
  return way;
}



u32 xtsc_component::xtsc_cache::find_replace_lru(u32 set_num) {
  const u32 maxlru = m_num_ways - 1;
  u32 way = 0;
  for ( ; way < m_num_ways; way++) {
    if ( m_lru[set_num][way] == maxlru ) {
      break;
    }
  }
  if (way == m_num_ways) {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\": Unable to find LRU line.";
    throw xtsc_exception(oss.str());
  }
  update_lru(set_num, maxlru);
  return way;
}



u32 xtsc_component::xtsc_cache::find_replace_random(u32 set_num) {
  u32 way = rand() % m_num_ways;
  return way;
}



u32 xtsc_component::xtsc_cache::find_replace_rr(u32 set_num) {
  bool lrf_bit = get_lrf(set_num, 0);
  for (u32 way = 1; way < m_num_ways; way++) {
    if (get_lrf(set_num, way) ^ lrf_bit)
      return way;
  }
  return 0;
}



void xtsc_component::xtsc_cache::peek(xtsc_address address8, u32 size8, u8 *buffer) {
  u32            line_offset          = address8 & (m_line_byte_width - 1);
  if ((line_offset + size8) > m_line_byte_width) {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\": Unable to peek address=0x" << hex << address8
        << ". The peek passes cache line boundary.";
    throw xtsc_exception(oss.str());
  }
  xtsc_address address_tag            = get_tag(address8);
  u32          set_num                = get_set(address8);
  u32          way                    = find_hit(address_tag, set_num);
  if (way < m_num_ways) { /* Cache hit */
    // Read cached data
    xtsc_address cached_data_address  = set_num * m_set_byte_size + way * m_line_byte_width + line_offset;
    xtsc_memory::peek(cached_data_address, size8, buffer);
  }
  else {
    XTSC_DEBUG(m_text, "Cache PEEK miss on address = 0x" << hex << address8);
    m_request_port->nb_peek(address8, size8, buffer);
  }
}



void xtsc_component::xtsc_cache::poke(xtsc_address address8, u32 size8, const u8 *buffer) {
  u32            line_offset         = address8 & (m_line_byte_width - 1);
  if ((line_offset + size8) > m_line_byte_width) {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\": Unable to poke address=0x" << hex << address8
        << ". The poke passes cache line boundary.";
    throw xtsc_exception(oss.str());
  }
  xtsc_address    address_tag        = get_tag(address8);
  u32             set_num            = get_set(address8);
  u32             way                = find_hit(address_tag, set_num);
  if (way < m_num_ways) {  /* Cache hit */
    // Write cache data
    xtsc_address cached_data_address = set_num * m_set_byte_size + way * m_line_byte_width + line_offset;
    xtsc_memory::poke(cached_data_address, size8, buffer);
  }
  else {
    XTSC_DEBUG(m_text, "Cache POKE miss on address = 0x" << hex << address8);
  }
  // Always Update the lower-level memory
  m_request_port->nb_poke(address8, size8, buffer);
}



void xtsc_component::xtsc_cache::reset(bool hard_reset) {
  m_num_block_transfers        = 0;
  m_block_read_response_count  = 0;
  m_p_single_response          = 0;
  m_p_block_write_response     = 0;
  for (u32 i = 0; i < 16; i++)
    m_p_block_read_response[i] = 0;
  if (hard_reset)
    initialize();

  // Cancel notified events
  m_single_response_available_event.cancel();
  m_block_read_response_available_event.cancel();
  m_block_write_response_available_event.cancel();

  xtsc_memory::reset(hard_reset);
}



void xtsc_component::xtsc_cache::update_line(xtsc_address mem_addr, u32 set_num, u32 way_num, bool critical_word_only) {
  xtsc_address mem_line_address;
  xtsc_address cache_line_address  = set_num * m_set_byte_size + way_num * m_line_byte_width;
  // Transfer this line to the downstream memory
  if (is_valid(set_num, way_num) && is_dirty(set_num, way_num)) {
    xtsc_memory::peek(cache_line_address, m_line_byte_width, m_line_buffer);
    mem_line_address     = (get_tag(set_num, way_num) << m_tag_shift) | (set_num  << m_set_shift);
    if (m_line_access_ratio  == 1) {
      xtsc_byte_enables byte_enables = (1U << m_line_byte_width) - 1;
      ext_mem_write(mem_line_address, m_line_byte_width, byte_enables, m_line_buffer);
    }
    else
      ext_mem_block_write(mem_line_address, m_line_byte_width, m_line_buffer);
    // Clear the dirty bit
    clear_dirty(set_num, way_num);
  }
  // Read a cache line from downstream memories.
  if (critical_word_only) {
    // Read from the requested memory address
    if (m_line_access_ratio == 1) {
      xtsc_byte_enables byte_enables = (1U << m_line_byte_width) - 1;
      ext_mem_read(mem_addr, m_access_byte_width, byte_enables, m_line_buffer);
    }
    else
      ext_mem_block_read(mem_addr, m_line_byte_width, m_line_buffer, true);
    cache_line_address = cache_line_address + (mem_addr & (m_line_byte_width - 1));
    xtsc_memory::poke(cache_line_address, m_access_byte_width, m_line_buffer);
  }
  else {
    // Read from the start of the cache line address
    mem_line_address       = mem_addr & ~(u64)(m_line_byte_width - 1);
    if (m_line_access_ratio == 1) {
      xtsc_byte_enables byte_enables = (1U << m_line_byte_width) - 1;
      ext_mem_read(mem_line_address, m_line_byte_width, byte_enables, m_line_buffer);
    }
    else
      ext_mem_block_read(mem_line_address, m_line_byte_width, m_line_buffer);
    xtsc_memory::poke(cache_line_address, m_line_byte_width, m_line_buffer);
  }
}



void xtsc_component::xtsc_cache::update_lru(u32 set_num, u32 ref) {
  for (u32 way = 0; way < m_num_ways; way++) {
    if (m_lru[set_num][way] > ref) {
      continue;
    }
    if (m_lru[set_num][way] == ref) {
      m_lru[set_num][way] = 0;
      continue;
    }
    if (m_lru[set_num][way] < ref) {
      m_lru[set_num][way]++;
      continue;
    }
  }
}



void xtsc_component::xtsc_cache::worker_thread(void) {

  // Get the port number for this "instance" of worker_thread
  u32 port_num = m_next_port_num++;

  try {
    // Reset internal fifos
    reset_fifos();

    while (true) {
      wait(*m_worker_thread_event[port_num]);
      while (m_request_fifo[port_num]->num_available()) {
        bool delta_delay = false;
        // Accept this one as our current transaction
        m_request_fifo[port_num]->nb_read(m_p_active_request_info[port_num]);
        XTSC_DEBUG(m_text, "worker_thread() got: " << m_p_active_request_info[port_num]->m_request);
        if (m_use_pif_attribute) {
          u32 pif_attribute = m_p_active_request_info[port_num]->m_request.get_pif_attribute();
          m_read_allocate   = is_read_allocate(pif_attribute);
          m_write_allocate  = is_write_allocate(pif_attribute);
          m_write_back      = is_write_back(pif_attribute);
          m_cacheable       = is_cacheable(pif_attribute);
          m_bufferable      = is_bufferable(pif_attribute);
        }
        if (m_use_pif_attribute && !m_cacheable) {
          XTSC_DEBUG(m_text, "worker_thread() bypass: " << m_p_active_request_info[port_num]->m_request);
          wait(m_bypass_delay);
          do_bypass(port_num);
        }
        else {
          // Determine delay
          sc_time delay = SC_ZERO_TIME;
          switch (m_p_active_request_info[port_num]->m_request.get_type()) {
          case xtsc_request::READ:
            delay  = m_read_delay;
            break;
          case xtsc_request::BLOCK_READ:
            delay  = m_block_read_delay;
            break;
          case xtsc_request::BURST_READ:
            delay  = m_burst_read_delay;
            break;
          case xtsc_request::RCW:
            if (m_read_only) {
              ostringstream oss;
              oss << "Read-only xtsc_memory '" << name() << "' doesn't support transaction: "
                  << m_p_active_request_info[port_num]->m_request;
              throw xtsc_exception(oss.str());
            }
            delay  = (m_first_rcw[port_num] ?  m_rcw_repeat : m_rcw_response);
            m_first_rcw[port_num] = m_p_active_request_info[port_num]->m_request.get_last_transfer();
            break;
          case xtsc_request::WRITE:
            if (m_read_only) {
              ostringstream oss;
              oss << "Read-only xtsc_memory '" << name() << "' doesn't support transaction: "
                  << m_p_active_request_info[port_num]->m_request;
              throw xtsc_exception(oss.str());
            }
            // To model the dual load/store unit requirement that a simultaneous read and write to
            // the same address must return the old data for the read (NOT the new data being written
            // by the write), we delay WRITE transactions for a delta cycle when we are multi-ported.
            delta_delay = (m_num_ports > 1);
            delay  = m_write_delay;
            break;
          case xtsc_request::BLOCK_WRITE:  {
            if (m_read_only) {
              ostringstream oss;
              oss << "Read-only xtsc_memory '" << name() << "' doesn't support transaction: "
                  << m_p_active_request_info[port_num]->m_request;
              throw xtsc_exception(oss.str());
            }
            bool last = m_p_active_request_info[port_num]->m_request.get_last_transfer();
            delta_delay = (m_num_ports > 1);
            delay  = (m_first_block_write[port_num] ?  m_block_write_delay : (last ? m_block_write_response : m_block_write_repeat));
            m_first_block_write[port_num] = last;
            break;
          }
          case xtsc_request::BURST_WRITE:  {
            if (m_read_only) {
              ostringstream oss;
              oss << "Read-only xtsc_memory '" << name() << "' doesn't support transaction: "
                  << m_p_active_request_info[port_num]->m_request;
              throw xtsc_exception(oss.str());
            }
            bool last = m_p_active_request_info[port_num]->m_request.get_last_transfer();
            delta_delay = (m_num_ports > 1);
            delay  = (m_first_burst_write[port_num] ?  m_burst_write_delay : (last ? m_burst_write_response : m_burst_write_repeat));
            m_first_burst_write[port_num] = last;
            break;
          }
          default: {
            ostringstream oss;
            oss << kind() << " '" << name() << "': ";
            oss << "Invalid xtsc_request::type_t in nb_request().";
            throw xtsc_exception(oss.str());
          }
          }
          if (m_delay_from_receipt) {
            // net => No Earlier Than time
            sc_time receipt_net     = m_p_active_request_info[port_num]->m_time_stamp + delay;
            sc_time last_action_net = m_last_action_time_stamp[port_num] + m_recovery_time;
            sc_time latest_net      = (receipt_net > last_action_net) ? receipt_net : last_action_net;
            sc_time now             = sc_time_stamp();
            delay = (latest_net <= now) ? SC_ZERO_TIME : (latest_net - now);
          }
          XTSC_DEBUG(m_text, "worker_thread() doing wait for " << delay);
          wait(delay);
          if (delta_delay) {
            wait(SC_ZERO_TIME);
          }
          do_active_request(port_num);
        }
      }
    }

  }
  catch (const sc_unwind_exception& error) { throw; }
  catch (const exception& error) {
    ostringstream oss;
    oss << "std::exception caught in SC_THREAD of " << kind() << " '" << name() << "'." << endl;
    oss << "what(): " << error.what() << endl;
    xtsc_log_multiline(m_text, log4xtensa::FATAL_LOG_LEVEL, oss.str(), 2);
    throw;
  }

}



bool xtsc_component::xtsc_cache::xtsc_respond_if_impl::nb_respond(const xtsc_response& response) {
  XTSC_INFO(m_cache.m_text, response);
  u8 rsp_id = response.get_id();
  if ((rsp_id == m_cache.m_read_id) || (rsp_id == m_cache.m_write_id) || (rsp_id == m_cache.m_rcw_id)) {
    if (m_cache.m_p_single_response) {
      delete m_cache.m_p_single_response;
      m_cache.m_p_single_response = 0;
    }
    m_cache.m_p_single_response = new xtsc_response(response);
    m_cache.m_single_response_available_event.notify(SC_ZERO_TIME);
  }
  else if (rsp_id == m_cache.m_block_read_id) {
    if (m_cache.m_block_read_response_count >= m_cache.m_num_block_transfers) {
      ostringstream oss;
      oss << m_cache.kind() << " '" << m_cache.name() << "' nb_respond(): Received " << (m_cache.m_block_read_response_count+1)
          << " BLOCK_READ responses.  " << m_cache.m_num_block_transfers << " were expected.";
      throw xtsc_exception(oss.str());
    }
    if (m_cache.m_p_block_read_response[m_cache.m_block_read_response_count]) {
      delete m_cache.m_p_block_read_response[m_cache.m_block_read_response_count];
      m_cache.m_p_block_read_response[m_cache.m_block_read_response_count] = 0;
    }
    m_cache.m_p_block_read_response[m_cache.m_block_read_response_count] = new xtsc_response(response);
    m_cache.m_block_read_response_available_event.notify(SC_ZERO_TIME);
    m_cache.m_block_read_response_count += 1;
  }
  else if (rsp_id == m_cache.m_block_write_id) {
    if (m_cache.m_p_block_write_response) {
      delete m_cache.m_p_block_write_response;
      m_cache.m_p_block_write_response = 0;
    }
    m_cache.m_p_block_write_response = new xtsc_response(response);
    m_cache.m_block_write_response_available_event.notify(SC_ZERO_TIME);
  }
  else {
    ostringstream oss;
    oss << m_cache.kind() << " '" << m_cache.name() << "' nb_respond(): Got response with unsupported id=" << (u32) rsp_id;
    throw xtsc_exception(oss.str());
  }
  return true;
}



void xtsc_component::xtsc_cache::xtsc_respond_if_impl::register_port(sc_port_base& port, const char *if_typename) {
  if (m_p_port) {
    ostringstream oss;
    oss << "Illegal multiple binding detected to xtsc_cache '" << m_cache.name() << "' m_respond_export: " << endl;
    oss << "  " << port.name() << endl;
    oss << "  " << m_p_port->name();
    throw xtsc_exception(oss.str());
  }
  XTSC_INFO(m_cache.m_text, "Binding '" << port.name() << "' to xtsc_cache::m_respond_export");
  m_p_port = &port;
}



