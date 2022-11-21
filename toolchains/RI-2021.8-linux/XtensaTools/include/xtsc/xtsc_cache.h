#ifndef _XTSC_CACHE_H_
#define _XTSC_CACHE_H_

// Copyright (c) 2015 by Tensilica Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of
// Tensilica Inc.  They may be adapted and modified by bona fide
// purchasers for internal use, but neither the original nor any adapted
// or modified version may be disclosed or distributed to third parties
// in any manner, medium, or form, in whole or in part, without the prior
// written consent of Tensilica Inc.

/**
 * @file
 */


#include <xtsc/xtsc_memory.h>


namespace xtsc_component {


/**
 * Constructor parameters for an xtsc_cache object.
 *
 * This class has the same contructor parameters as an xtsc_memory_parms object plus the
 * following additional ones.
 *
  \verbatim
   Name                 Type    Description
   ------------------   ----    ------------------------------------------------------
   "cache_byte_size"     u32    The total byte size of this cache. The cache byte size
                                should meet the following restrictions: (a) number of
                                sets(= "cache_byte_size" / "line_byte_width"x"num_ways")
                                must be a power of 2,   (b) cache byte size must be an
                                integer multiple of the set byte size, and (c) minimum
                                value of cache size is "line_byte_width"x"num_ways".
                                Default = 0x1000 (4K Bytes).

   "line_byte_width"     u32    The byte size of cache lines. Note: this parameter
                                must be an integer multiple of "byte_width".
                                Valid values for the ratio of "line_byte_width" to
                                "byte_width" are 1|2|4|8|16.
                                Default = 8.

   "num_ways"            u32    The set associativity size. If 0, this class models
                                a fully-associative cache. If 1, this class models
                                a direct-mapped cache. Otherwise, it models a typical
                                "num_ways"-way set associative cache.
                                Default = 2 (i.e. a 2-way set associative cache).

   "replacement_policy"  char*  This parameter defines the strategy for selecting a
                                line to replace. Valid values are RANDOM for random,
                                RR for round-robin, and LRU for least-recently used
                                replacement policies. Note: RANDOM policy is generally
                                used for internal tests only.
                                Default = RR.

   "read_allocate"       bool   If true, the cache line is allocated on a read miss.
                                Default = true.

   "write_allocate"      bool   If true, the cache line is allocated on a write miss,
                                i.e. write misses act like read misses. If false, the
                                data is only modified in the lower-level memory, i.e.
                                write miss do not modify the cache.
                                Default = false.

   "write_back"          bool   This parameter defines cache write policy. If true,
                                data is written only to the cache. The modified
                                line is written to the lower-level memory only
                                when it is replaced. If false, the data is written
                                to both cache and lower-level memory.
                                Default = true.

   "read_priority"       u32    Priority for the lower-level memory READ/BLOCK_READ
                                requests. Valid values are 0|1|2|3.
                                Default = 3 (i.e. the highest priority).

   "write_priority"      u32    Priority for the lower-level memory WRITE/BLOCK_WRITE
                                requests. Valid values are 0|1|2|3.
                                Default = 3 (i.e. the highest priority).

   "profile_cache"       bool   If true, the xtsc_cache class keeps the track of read/
                                write misses. At the end of the simulation, miss rates
                                on read, write, and all accesses are printed in the
                                output log file at  INFO  level.
                                Default = false.

   "use_pif_attribute"   bool   If true, the xtsc_cache class uses the PIF attribute
                                of input requests to determine cache behavior.  This
                                specifies bufferable, cacheable, allocate, and write
                                policy attributes.  If false,  the xtsc_cache class
                                ignores PIF attributes and works based on the
                                xtsc_cache's parameters.
                                Default = false (i.e. ignore the PIF attribute).

   "bypass_delay"        u32    Number of clock periods takes to bypass the xtsc_cache
                                model for non-cacheable PIF requests.
                                Default = 1.

  \endverbatim
 *
 * The following parameters of the xtsc_memory_parms class are initialized by the xtsc_cache class
 * internally and changing their values will cause an exception.
 *
  \verbatim
   Name                   Value
   ------------------     -----------
   "num_ports"             1
   "start_byte_address"    0x00000000
   "memory_byte_size"      0
   "use_raw_access"        false

   Note: The xtsc_cache class does not support raw access. By default, normal
         nb_peek/nb_poke through all upstream devices is used for fast access.
         For other fast access methods, set at most one of "use_callback_access",
         "use_custom_access", and "use_interface_access" to true.

  \endverbatim
 *
 * The following parameters of the xtsc_memory_parms class are not supported by the xtsc_cache module.
 * Note that changing their default values may cause an exception.
 *
  \verbatim
   Name
   --------------------
   "burst_read_delay"
   "burst_read_repeat"
   "burst_write_delay"
   "burst_write_repeat"
   "burst_write_response"
   "initial_value_file"
   "memory_fill_byte"
   "read_only"
   "script_file"
   "wraparound"
   "fail_status"
   "fail_request_mask"
   "fail_percentage"
   "fail_seed"

  \endverbatim
 *
 * @see xtsc_memory_parms
 * @see xtsc_cache
 */
class XTSC_COMP_API xtsc_cache_parms : public xtsc_memory_parms {
public:

  /// Constructor for an xtsc_cache_parms object.
  xtsc_cache_parms(xtsc::u32   byte_width          = 4,
                   xtsc::u32   line_byte_width     = 8,
                   xtsc::u32   hit_delay           = 1,
                   xtsc::u32   byte_size           = 0x1000) :
    xtsc_memory_parms(byte_width, hit_delay, 0x0, 0x0)
  {
    add("cache_byte_size",     byte_size);
    add("line_byte_width",     line_byte_width);
    add("num_ways",            2);
    add("replacement_policy",  "RR");
    add("read_allocate",       true);
    add("write_allocate",      false);
    add("write_back",          true);
    add("read_priority",       0x3);
    add("write_priority",      0x3);
    add("profile_cache",       false);
    add("use_pif_attribute",   false);
    add("bypass_delay",        1);

    // Modify xtsc_memory_parm
    set("use_raw_access",      false);

    // Do not dump the following parameters from the xtsc_memory_parms class
    set_dumpable("num_ports",            false);
    set_dumpable("start_byte_address",   false);
    set_dumpable("memory_byte_size",     false);
    set_dumpable("initial_value_file",   false);
    set_dumpable("memory_fill_byte",     false);
    set_dumpable("burst_read_delay",     false);
    set_dumpable("burst_read_repeat",    false);
    set_dumpable("burst_write_delay",    false);
    set_dumpable("burst_write_repeat",   false);
    set_dumpable("burst_write_response", false);
    set_dumpable("read_only",            false);
    set_dumpable("script_file",          false);
    set_dumpable("wraparound",           false);
    set_dumpable("fail_status",          false);
    set_dumpable("fail_request_mask",    false);
    set_dumpable("fail_percentage",      false);
    set_dumpable("fail_seed",            false);

  }

  /// Return what kind of xtsc_parms this is (our C++ type)
  const char *kind() const { return "xtsc_cache_parms"; }

};


/**
 * This class implements an XTSC model of a typical cache module. The xtsc_cache class can be used as
 * an L2 cache, which sits between a core's PIF port and the system memory.
 *
 * @see xtsc_cache_parms
 * @see xtsc_memory
 */
class XTSC_COMP_API xtsc_cache : public xtsc_memory
{
public:

  sc_core::sc_port  <xtsc::xtsc_request_if>     m_request_port;         ///< Binds to the lower-level memory.
  sc_core::sc_export<xtsc::xtsc_respond_if>     m_respond_export;       ///< The lower-level memory binds to this.

  typedef enum replacement_policy_t {
    REPL_RANDOM   = 0,
    REPL_RR       = 1,
    REPL_LRU      = 2
  } replacement_policy_t;

  // SystemC needs this.
  SC_HAS_PROCESS(xtsc_cache);


  /// Our C++ type (SystemC uses this)
  virtual const char* kind() const { return "xtsc_cache"; }


  /**
   * Constructor for an xtsc_cache class.
   *
   * @param   module_name     The SystemC module name.
   *
   * @param   cache_parms     The xtsc_cache and xtsc_memory construction parameters.
   *
   * @see xtsc_cache_parms
   * @see xtsc_memory_parms
   */
  xtsc_cache(sc_core::sc_module_name module_name, const xtsc_cache_parms& cache_parms);


  /// Destructor.
  virtual ~xtsc_cache(void);


  /// SystemC calls this method at the end of simulation
  void end_of_simulation();


  /// For xtsc_connection_interface
  virtual xtsc::u32 get_bit_width(const std::string& port_name, xtsc::u32 interface_num = 0) const;


  /// For xtsc_connection_interface
  virtual sc_core::sc_object *get_port(const std::string& port_name);


  /// For xtsc_connection_interface
  virtual std::string get_default_port_name() const { return "master_port"; }


  /// For xtsc_connection_interface
  virtual xtsc::xtsc_port_table get_port_table(const std::string& port_table_name) const;


  /// Clear cache profile results
  void clear_profile_results();


  /// Dump the cache configuration
  void dump_config(std::ostream &os = std::cout) const;


  /// Dump profile results
  void dump_profile_results(std::ostream &os = std::cout) const;


  /// Flush dirty lines to the lower-level memory, invalid the entire cache
  void flush();


  /// Flush dirty lines to the lower-level memory, return the number of flushed lines
  xtsc::u32 flush_dirty_lines();


  /// Reset the device
  virtual void reset(bool hard_reset = false);


  /// Set the value of "m_profile_cache" parameter
  void set_profile_cache(bool enable = true) { m_profile_cache = enable; }


  /**
   * Implementation of the xtsc::xtsc_command_handler_interface.
   *
   * This implementation supports the following commands in addition to the xtsc_memory commands:
   * \verbatim
       clear_profile_results
         Call xtsc_cache::clear_profile_results().

       dump_config
         Call xtsc_cache::dump_config(). Dump cache configuration.

       dump_profile_results
         Call xtsc_cache::dump_profile_results(). Dump cache accesses profile results.

       flush
         Call xtsc_cache::flush(). Flush the entire cache.

       flush_dirty_lines
         Call xtsc_cache::flush_dirty_lines(). Flush dirty lines only.

       reset [<Hard>]
         Call xtsc_cache::reset(<Hard>).  Where <Hard> is 0|1 (default 0).

       set_profile_cache <Enable>
         Call xtsc_cache::set_profile_cache(<Enable>). Where <Enable> is true|false.
         Set the value of "m_profile_cache" parameter.

   \endverbatim
  */
  virtual void execute(const std::string&               cmd_line,
                       const std::vector<std::string>&  words,
                       const std::vector<std::string>&  words_lc,
                       std::ostream&                    result);

protected:

  /// Cache line data structure.
  struct line_info {
    xtsc::xtsc_address tag;       ///< Address tag
    bool valid;                   ///< Valid bit
    bool dirty;                   ///< Dirty bit
    bool lrf;                     ///< LRF (least recently filled) bit
  };

  /// PIF attribute bits
  enum pif_attribute_bits {
    BUFFERABLE       = 0x10,
    CACHEABLE        = 0x20,
    WRITE_BACK       = 0x40,
    READ_ALLOCATE    = 0x80,
    WRITE_ALLOCATE   = 0x100
  };

  /// Implementation of xtsc_respond_if.
  class xtsc_respond_if_impl : public xtsc::xtsc_respond_if, public sc_core::sc_object  {
  public:

    /// Constructor
    xtsc_respond_if_impl(const char *object_name, xtsc_cache& cache) :
      sc_object (object_name),
      m_cache   (cache),
      m_p_port  (0)
    {}

    /// Used by downstream slaves to respond to our PIF requests
    /// @see xtsc::xtsc_respond_if
    bool nb_respond(const xtsc::xtsc_response& response);


  protected:

    /// SystemC callback when something binds to us
    virtual void register_port(sc_core::sc_port_base& port, const char *if_typename);

    xtsc_cache&                 m_cache;        ///< Our xtsc_cache object
    sc_core::sc_port_base      *m_p_port;       ///< Port that is bound to us
  };

  /// Check xtsc_memory_parms values
  void check_memory_parms(const xtsc_cache_parms &cache_parms);

  /// We override this method from the xtsc_memory class
  virtual void compute_delays();

  /// Configure the cache
  void configure();

  /// Initialize the cache
  void initialize();

  /// Bypass the cache model
  virtual void do_bypass(xtsc::u32 port_num);

  /// We override this method from the xtsc_memory class
  virtual void do_block_read(xtsc::u32 port_num);

  /// We override this method from the xtsc_memory class
  virtual void do_block_write(xtsc::u32 port_num);

  /// We override this method from the xtsc_memory class
  virtual void do_burst_read(xtsc::u32 port_num);

  /// We override this method from the xtsc_memory class
  virtual void do_burst_write(xtsc::u32 port_num);

  /// We override this method from the xtsc_memory class
  virtual void do_rcw(xtsc::u32 port_num);

  /// We override this method from the xtsc_memory class
  virtual void do_read(xtsc::u32 port_num);

  /// We override this method from the xtsc_memory class
  virtual void do_write(xtsc::u32 port_num);

  /// We override this method from the xtsc_memory class
  virtual void worker_thread(void);

  /// Send a BLOCK_READ request to the lower-level memory
  void ext_mem_block_read(xtsc::xtsc_address      src_mem_addr,
                          xtsc::u32               size,
                          xtsc::u8               *dst_data,
                          bool                    first_transfer_only = false,
                          xtsc::u32               port_num = 0);

  /// Bypass the cache on BLOCK_READ requests
  void ext_mem_block_read(xtsc::u32 port_num);

  /// Send a BLOCK_WRITE request to the lower-level memory
  void ext_mem_block_write(xtsc::xtsc_address     dst_mem_addr,
                           xtsc::u32              size,
                           const xtsc::u8        *src_data,
                           xtsc::u32              port_num = 0);

  /// Bypass the cache model on BLOCK_WRITE requests if cache_bypass is true, otherwise send a copy of input request to the lower-level memory
  void ext_mem_block_write(xtsc::u32 port_num, bool cache_bypass = true);

  /// Send a READ request to the lower-level memory
  void ext_mem_read(xtsc::xtsc_address            src_mem_addr,
                    xtsc::u32                     size,
                    xtsc::xtsc_byte_enables       byte_enables,
                    xtsc::u8                     *dst_data,
                    xtsc::u32                     port_num = 0);

  /// Bypass the cache model on READ requests
  void ext_mem_read(xtsc::u32 port_num);

  /// Bypass the cache model on RCW requests
  void ext_mem_rcw(xtsc::u32 port_num);

  /// Send a WRITE request to the lower-level memory
  void ext_mem_write(xtsc::xtsc_address           dst_mem_addr,
                     xtsc::u32                    size,
                     xtsc::xtsc_byte_enables      byte_enables,
                     const xtsc::u8              *src_data,
                     xtsc::u32                    port_num = 0);

  /// Bypass the cache model on WRITE requests
  void ext_mem_write(xtsc::u32 port_num);

  /// Return the matched way number if it is a hit, otherwise return "m_num_ways"
  xtsc::u32 find_hit(xtsc::xtsc_address address_tag, xtsc::u32 set_num);

  /// Return the replaced way number on a conflict cache miss
  xtsc::u32 find_replace_line(xtsc::u32 set_num);

  /// Return the replaced way number based on the LRU policy
  xtsc::u32 find_replace_lru(xtsc::u32 set_num);

  /// Return the replaced way number based on the Random policy
  xtsc::u32 find_replace_random(xtsc::u32 set_num);

  /// Return the replaced way number based on the RR policy
  xtsc::u32 find_replace_rr(xtsc::u32 set_num);

  /// We override this method from the xtsc_memory class
  virtual void peek(xtsc::xtsc_address address8, xtsc::u32 size8, xtsc::u8 *buffer);

  /// We override this method from the xtsc_memory class
  virtual void poke(xtsc::xtsc_address address8, xtsc::u32 size8, const xtsc::u8 *buffer);

  /// Update a cache line's data from the lower-level memory
  void update_line(xtsc::xtsc_address memory_address, xtsc::u32 set_num, xtsc::u32 way_num, bool critical_word_only = false);

  /// Clear a cache line's dirty bit
  void clear_dirty(xtsc::u32 set_num, xtsc::u32 way_num) { m_lines[set_num][way_num].dirty = false; }

  /// Clear a cache line's valid bit
  void clear_valid(xtsc::u32 set_num, xtsc::u32 way_num) { m_lines[set_num][way_num].valid = false; }

  /// Return the tag of the address
  xtsc::xtsc_address get_tag(xtsc::xtsc_address address) const { return address >> m_tag_shift; }

  /// Return the tag of a cache line
  xtsc::xtsc_address get_tag(xtsc::u32 set_num, xtsc::u32 way_num) const { return m_lines[set_num][way_num].tag; }

  /// Return the LRF bit of a cache line
  bool get_lrf(xtsc::u32 set_num, xtsc::u32 way_num) const { return m_lines[set_num][way_num].lrf; }

  /// Return the set number of the address
  xtsc::u32 get_set(xtsc::xtsc_address address) const { return ((address >> m_set_shift) & m_set_mask); }

  /// Return true, if the current request is cacheable based on its PIF attribute
  bool is_bufferable(xtsc::u32 pif_attribute) const { return (pif_attribute &  BUFFERABLE); }

  /// Return true, if the current request is cacheable based on its PIF attribute
  bool is_cacheable(xtsc::u32 pif_attribute) const { return (pif_attribute & CACHEABLE); }

  /// Check if the cache line is dirty
  bool is_dirty(xtsc::u32 set_num, xtsc::u32 way_num) const { return m_lines[set_num][way_num].dirty; }

  /// Return true, if the current cache access is write-allocate based on its PIF attribute
  bool is_read_allocate(xtsc::u32 pif_attribute) const { return (pif_attribute & READ_ALLOCATE); }

  /// Check if the cache line is valid
  bool is_valid(xtsc::u32 set_num, xtsc::u32 way_num) const { return m_lines[set_num][way_num].valid; }

  /// Return true, if the current cache access is write-allocate based on its PIF attribute
  bool is_write_allocate(xtsc::u32 pif_attribute) const { return (pif_attribute & WRITE_ALLOCATE); }

  /// Return true, if the current cache access is write-back based on its PIF attribute
  bool is_write_back(xtsc::u32 pif_attribute) const { return (pif_attribute & WRITE_BACK); }

  /// Set a cache line's tag bits
  void set_tag(xtsc::xtsc_address tag, xtsc::u32 set_num, xtsc::u32 way_num) { m_lines[set_num][way_num].tag = tag; }

  /// Set a cache line's dirty bit
  void set_dirty(xtsc::u32 set_num, xtsc::u32 way_num) { m_lines[set_num][way_num].dirty = true; }

  /// Set a cache line's valid bit
  void set_valid(xtsc::u32 set_num, xtsc::u32 way_num) { m_lines[set_num][way_num].valid = true; }

  /// Update the LRF bit of a cache line
  void update_lrf(xtsc::u32 set_num, xtsc::u32 way_num) { m_lines[set_num][way_num].lrf = !m_lines[set_num][way_num].lrf; }

  /// Update the LRU bits of a cache line
  void update_lru(xtsc::u32 set_num, xtsc::u32 ref);

  /* External memory PIF interface implementation */
  xtsc_respond_if_impl          m_respond_impl;                         ///< m_respond_export binds to this
  xtsc::xtsc_request            m_request;                              ///< For sending system memory requests
  const xtsc::xtsc_response    *m_p_single_response;                    ///< Current rsp to READ, WRITE, or RCW request
  sc_core::sc_event             m_single_response_available_event;      ///< Notified when READ, WRITE, or RCW rsp is received
  const xtsc::xtsc_response    *m_p_block_read_response[16];            ///< Maintain our copy of BLOCK_READ responses
  sc_core::sc_event             m_block_read_response_available_event;  ///< Notified when a BLOCK_READ rsp is received
  const xtsc::xtsc_response    *m_p_block_write_response;               ///< Current BLOCK_WRITE response
  sc_core::sc_event             m_block_write_response_available_event; ///< Notified when a BLOCK_WRITE rsp is received
  xtsc::u32                     m_num_block_transfers;                  ///< Number of BLOCK_READ/BLOCK_WRITE transfers
  xtsc::u32                     m_block_read_response_count;            ///< Number of BLOCK_READ responses received so far

  /* xtsc_cache parameters */
  xtsc_cache_parms              m_cache_parms;                          ///< Copy of xtsc_cache_parms
  xtsc::u32                     m_cache_byte_size;                      ///< See "cache_byte_size"
  xtsc::u32                     m_line_byte_width;                      ///< See "line_byte_width"
  xtsc::u32                     m_num_ways;                             ///< See "num_ways"
  xtsc::u32                     m_replacement_policy;                   ///< See "replacement_policy"
  bool                          m_read_allocate;                        ///< See "read_allocate"
  bool                          m_write_allocate;                       ///< See "write_allocate"
  bool                          m_write_back;                           ///< See "write_back"
  xtsc::u8                      m_read_priority;                        ///< See "read_priority"
  xtsc::u8                      m_write_priority;                       ///< See "write_priority"
  bool                          m_profile_cache;                        ///< See "profile_cache"
  bool                          m_use_pif_attribute;                    ///< See "use_pif_attribute"
  sc_core::sc_time              m_bypass_delay;                         ///< See "bypass_delay"

  struct line_info            **m_lines;                                ///< Cache lines, includes the address tag and status bits
  xtsc::u32                   **m_lru;                                  ///< Least-recently used (LRU) counters
  bool                          m_lru_selected;                         ///< Indicates if LRU policy is selected
  xtsc::u32                     m_access_byte_width;                    ///< A copy of "byte_width"
  xtsc::u32                     m_num_lines;                            ///< Total number of cache lines
  xtsc::u32                     m_num_sets;                             ///< Total number of cache sets
  xtsc::u32                     m_line_access_ratio;                    ///< The number of access words in a cache line
  xtsc::u32                     m_set_byte_size;                        ///< Total number of bytes in a set
  xtsc::u32                     m_num_sets_log2;                        ///< Log2 of m_num_sets
  xtsc::u32                     m_line_byte_width_log2;                 ///< Log2 of m_line_byte_width
  xtsc::u32                     m_tag_shift;                            ///< Shift value to calculate address tag
  xtsc::u32                     m_set_shift;                            ///< Shift value to calculate set index
  xtsc::u32                     m_set_mask;                             ///< Set's index mask

  xtsc::u8                     *m_line_buffer;                          ///< Internal buffer for updating cache lines
  xtsc::u32                     m_line_buffer_index;                    ///< Index of cache line buffer

  bool                          m_cacheable;                            ///< Indicate if the current request is cacheable
  bool                          m_bufferable;                           ///< Indicate if the current request is bufferable

  /* Profiling info */
  xtsc::u64                     m_read_count;                           ///< Number of READ accesses
  xtsc::u64                     m_read_miss_count;                      ///< Number of READ misses
  xtsc::u64                     m_block_read_count;                     ///< Number of BLOCK_READ accesses
  xtsc::u64                     m_block_read_miss_count;                ///< Number of BLOCK_READ misses
  xtsc::u64                     m_write_count;                          ///< Number of WRITE accesses
  xtsc::u64                     m_write_miss_count;                     ///< Number of WRITE misses
  xtsc::u64                     m_block_write_count;                    ///< Number of BLOCK_WRITE accesses
  xtsc::u64                     m_block_write_miss_count;               ///< Number of BLOCK_WRITE misses

  /* Constant data members */
  static const xtsc::u8         m_read_id               = 0x3;          ///< xtsc_request::m_id for READ
  static const xtsc::u8         m_write_id              = 0x5;          ///< xtsc_request::m_id for WRITE
  static const xtsc::u8         m_block_read_id         = 0x7;          ///< xtsc_request::m_id for BLOCK_READ
  static const xtsc::u8         m_block_write_id        = 0x9;          ///< xtsc_request::m_id for BLOCK_WRITE
  static const xtsc::u8         m_rcw_id                = 0xb;          ///< xtsc_request::m_id for RCW

};


}  // namespace xtsc_component


#endif  // _XTSC_CACHE_H_
