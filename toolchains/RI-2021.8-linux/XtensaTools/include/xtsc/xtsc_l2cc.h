#ifndef _XTSC_L2CC_H_
#define _XTSC_L2CC_H_

// Copyright (c) 2020 by Tensilica Inc.  ALL RIGHTS RESERVED.
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
#include <xtsc/xtsc.h>
#include <xtsc/xtsc_core.h>
#include <xtsc/xtsc_parms.h>
#include <xtsc/xtsc_respond_if.h>
#include <xtsc/xtsc_request_if.h>
#include <xtsc/xtsc_request.h>
#include <xtsc/xtsc_response.h>
#include <xtsc/xtsc_wire_write_if.h>
#include <xtsc/xtsc_fast_access.h>
#include <xtsc/xtsc_memory_b.h>

#include <string>
#include <set>
#include <vector>
#include <cstring>

using namespace std;

namespace xtsc {

class xtsc_l2cc_parts;

/**
 * Constructor parameters for an xtsc_l2cc object.
 *
 * This class contains the constructor parameters for an xtsc_l2cc object.
 *
 * Most of the parameters which define the xtsc_l2cc internal structure are from the
 * core's configuration database in the Xtensa registry. In other words, these parameters
 * must match the core that is driving the xtsc_l2cc module. Here is a list of such
 * parameters:
 *
 \verbatim
 Name                        Type    Description
 -------------------------   ----    ---------------------------------------------------
 "has_axi_slave_interface"   bool    This parameter defines if the xtsc_l2cc module has
                                     an AXI slave interface. The slave interface allows
                                     external masters to access the local RAM (L2RAM) of
                                     the xtsc_l2cc module.
                                     Default = true.

 "control_reg_address"       u32     The starting byte address of xtsc_l2cc registers in
                                     the 4GB address space. It is aligned to 4K Bytes
                                     boundary.
                                     Default = 0xE3A02000.

 "reset_tcm_base_address"    u32     The starting byte address of xtsc_l2cc L2RAM in the
                                     4GB address space. This parameter defines the initial
                                     address at reset time. Software can change the L2RAM
                                     start address at run time by programming xtsc_l2cc
                                     control registers.
                                     Default = 0xE3600000.

 "core_data_byte_width"      u32     Core data interface width in bytes.
                                     Valid values are 16 or 32.
                                     Default = 16 (128 bits).

 "core_inst_byte_width"      u32     Core instruction interface width in bytes.
                                     Valid values are 16 or 32.
                                     Default = 16 (128 bits).

 "master_byte_width"         u32     Master data interface width in bytes.
                                     Valid values are 8 or 16.
                                     Default = 16 (128 bits).

 "slave_byte_width"          u32     Slave data interface width in bytes.
                                     Valid values are 16 or 32.
                                     Default = 8 (64 bits).

 "cache_byte_size"           u32     Total byte size of the xtsc_l2cc internal memory.
                                     Valid values are 1M, 2M, and 4M Bytes. The internal
                                     memory of the xtsc_l2cc module can be organized as
                                     partial L2Cache and partial L2RAM. At reset time, it
                                     is all L2RAM. Software can program the xtsc_l2cc to
                                     all L2Cache, or partial L2Cache and partial L2RAM.
                                     Default = 0x200000 (2M Bytes).

 "line_byte_size"            u32     The byte size of cache lines.
                                     Default = 64.

 "num_ways"                  u32     Cache set-associativity. The xtsc_l2cc module only
                                     supports 8-way and 16-way set associative caches.
                                     Default = 8 (i.e. an 8-way set associative cache).

 "cache_memory_latency"      u32     The latency of the xtsc_l2cc internal memory,
                                     expressed in terms of clock periods.
                                     Default = 2.

 "num_perf_counters"         u32     This parameter defines the number of availavle
                                     performance counters that can be programmed by
                                     software to count L2CC different events.
                                     Default = 8.

 "has_index_lock"            bool    This parameter defines if the xtsc_l2cc module can
                                     support 'Prefetch and Lock" CacheOP.
                                     Default = true.

 "has_l2_error"              bool    This parameter defines if the xtsc_l2cc module uses
                                     L2Err interrupt port to notify L2 errors. If it is
                                     set to false, all interrupts are reported on L2Status
                                     interrupt port.
                                     Default = true.

 "support_multicore"         bool    This parameter defines if the xtsc_l2cc module can
                                     support multicore configs. This parameter is only
                                     set when the cofig HW version is NX1.1.0 or later.
                                     Default = false.

 "has_coherency_controller"  bool    This parameter defines if the xtsc_l2cc module
                                     supports ACE coherency controller. This feature is
                                     available when "support_multicore" is true.
                                     Default = false.

 "l2ram_coherent"            bool    If true, L2RAM address space is coherent. This feature
                                     is available when "has_coherency_controller" is true.
                                     Default = false.

 "l2ram_only"                bool    If true, the xtsc_l2cc module is configured to operate
                                     as exclusively L2RAM controller. The L2RAM-only feature
                                     doesn’t support any of the coherence-related features,
                                     which means "has_coherency_controller" must be false.
                                     Default = false.

 "num_exclusives"            u32     Enables non-coherent L2RAM exclusive transactions.  If
                                     the number of exclusive masters is zero, L2 proceeds with
                                     L2 RAM access and returns OKAY responses.
                                     Default = 1;

 "core_port_muxing"          u32     When this parameter is chosen cores instruction and data
                                     channels are muxed inside L2. Valid values are 0|1|2|4.
                                     0: No core port muxing.
                                     1: Inst and Data Read channels of each core muxed.
                                     2: Inst and Data Read and Write channels from 2 cores
                                     muxed. For 4 cores, Core0 with Core1 and Core2 with Core3
                                     are muxed.
                                     4: Inst and Data channels from 4 cores muxed.
                                     Default = 0.

 "default_xtensa_core"       char *  The Xtensa core which defines the default value
                                     of L2CC configuration parameters.
                                     Default = Atlanta_NX.
 \endverbatim

 *
 * The following parameters are not obtained from the core's configuration database. These
 * parameters are basically used to define a sub-system structure or to control some timing
 * aspect of the simulation.
 *
 \verbatim

 Name                        Type    Description
 -------------------------   ----    ---------------------------------------------------
 "num_cores"                 u32     Number of xtsc_cores that are connected to this
                                     xtsc_l2cc module. Valid values are 1|2|4.
                                     Default = 1.

 "num_slaves"                u32     Number of slave interfaces that this xtsc_l2cc
                                     module has.
                                     Default = 1.

 "connect_slave_interface"  bool     This parameter defines if the slave interface is
                                     bound to an external AXI master port. If slave
                                     interface is left unbounded, the xtsc_l2cc module
                                     does not model slave interface components.
                                     Default = false (i.e the slave port is not modeled).

 "connect_init_done_port"   bool     This parameter defines whether the init_done port
                                     is bound to an external module in a subsystem. If
                                     this port is left unbounded, the xtsc_l2cc module
                                     should not drive the init_done port.
                                     default = false (i.e. the port is not connected).

 "page_byte_size"            u32     The internal memory page byte size. In the internal
                                     memory model, memory is not allocated until it is
                                     accessed. This parameter specifies the allocation
                                     size.
                                     Default = 1024x16 (16K Bytes)

 "clock_period"              u32     This is the length of this xtsc_l2cc's clock period
                                     expressed in terms of the SystemC time resolution
                                     (from sc_get_time_resolution()).
                                     A value of 0xFFFFFFFF means to use the XTSC system
                                     clock period (from xtsc_get_system_clock_period()).
                                     A value of 0 means one delta cycle.
                                     Default = 0xFFFFFFFF (i.e. use the system clock
                                     period).

 "nacc_wait_time"            u32     This parameter, expressed in terms of the SystemC
                                     time resolution, specifies how long to wait after
                                     sending a request downstream to see if it was
                                     rejected by RSP_NACC. This value must not exceed
                                     this device's clock period. A value of 0 means one
                                     delta cycle. A value of 0xFFFFFFFF means to wait
                                     for a period equal to this device's clock period.
                                     CAUTION:  A value of 0 can cause an infinite loop
                                     in the simulation if the downstream module requires
                                     a non-zero time to become available.
                                     Default = 0xFFFFFFFF (device's clock period).

 "posedge_offset"            u32     This specifies the time at which the first posedge
                                     of this device's clock conceptually occurs. It is
                                     expressed in units of the SystemC time resolution.
                                     The value implied by it must be strictly less than
                                     the value implied by the "clock_period" parameter.
                                     A value of 0xFFFFFFFF means to use the same posedge
                                     offset as the system clock (from
                                     xtsc_get_system_clock_posedge_offset()).
                                     Default = 0xFFFFFFFF.

 "arbitration_phase"         u32     The phase of the clock at which arbitration is
                                     performed expressed in terms of the SystemC time
                                     resolution (from sc_get_time_resolution()).  A
                                     value of 0 means to arbitrate at posedge clock as
                                     specified by "posedge_offset".  A value of
                                     0xFFFFFFFF means to use a phase of one-half of this
                                     device's clock period which corresponds to arbitrating
                                     at negedge clock.  The arbitration phase must be
                                     strictly less than this device's clock period.
                                     Default = 0xFFFFFFFF.

 "turbo"                     bool    If true, turboXim fast access mode will be used to
                                     model L2. If false, then AXI BURST_READ|BURST_WRITE
                                     requests will be used to model a cycle-accurate
                                     implementation of xtsc_l2cc. The value set by this
                                     parameter can be changed by
                                     xtsc_switch_sim_mode(xtsc_sim_mode mode) method at
                                     run-time. xtsc_l2cc switches simulation mode when
                                     there are no pending AXI transactions.
                                     Default = As specified by the "turbo" parameter
                                     of xtsc_initialize_parms.

 "l2ram_load_file"           char*   File name to initialize L2RAM during the
                                     start_of_simulation() callback.
                                     Default = NULL.

 "host_shared_memory"        bool    If true the backing store for L2RAM will be host
                                     OS shared memory (created using shm_open() on Linux
                                     and CreateFileMapping() on MS Windows).  If this
                                     parameter is true then "page_byte_size" does not apply
                                     and should be left at its default value. This parameter
                                     can be true only if "l2ram_only" is true and "l2ram_coherent"
                                     is false.
                                     Default = false.

 "shared_memory_name"        char*   The shared memory name to use when creating host OS
                                     shared memory.  If this parameter is left at its default
                                     setting of NULL, then the default name will be formed by
                                     concatenating the user name, a period, and the module
                                     instance hierarchical name. For example:
                                     MS Windows:     joeuser.myshmem
                                     Linux: /dev/shm/joeuser.myshmem
                                     This parameter is ignored if "host_shared_memory" is
                                     false.
                                     Default = NULL (use default shared memory name)

 "host_mutex"                bool    If true then the host OS shared memory will
                                     have a host OS level mutex which will be locked
                                     during AXI exclusive sequences and
                                     during each poke and write call.  This parameter is
                                     ignored if "host_shared_memory" is false.
                                     Default = false.

 \endverbatim
 *
**/

class XTSC_API xtsc_l2cc_parms : public xtsc_parms {
public:

  /// Constructor for an xtsc_l2cc_parms object.
  xtsc_l2cc_parms(u32 num_ways         = 0,
                  u32 line_byte_size   = 0,
                  u32 cache_byte_size  = 0);

  /// Return what kind of xtsc_parms this is (our C++ type)
  const char *kind() const { return "xtsc_l2cc_parms"; }

};


/**
 *
 * The xtsc_l2cc class provides an XTSC model of Cadence/Tensilica's L2 Cache Controller
 * (L2CC). The L2CC is designed to support Level 2 cache (L2Cache) and additional local
 * memories (L2RAM) for Xtensa NX processors. The L2CC sits between an Xtensa core's AXI
 * interface and a system memory. More information about L2CC architecture can be found
 * in the Xtensa NX Microprocessor Data Book, Chapter L2 Memory Subsystem.
 *
 * @see xtsc_l2cc_parms
 * @see xtsc::xtsc_memory_b
 * @see xtsc::xtsc_request_if
 * @see xtsc::xtsc_respond_if
 * @see xtsc::xtsc_core::How_to_do_port_binding
 */
class XTSC_API xtsc_l2cc : public sc_core::sc_module
                         , public xtsc_module
                         , public xtsc_mode_switch_if
                         , public xtsc_command_handler_interface
{
public:

  /// Our C++ type (SystemC uses this)
  virtual const char* kind() const { return "xtsc_l2cc"; }


  /**
   * Constructor for an xtsc_l2cc class.
   *
   * @param   module_name     The SystemC module name.
   *
   * @param   l2cc_parms      The xtsc_l2cc parameters.
   *
   * @see xtsc_l2cc_parms
   */
  xtsc_l2cc(sc_core::sc_module_name module_name, const xtsc_l2cc_parms& l2cc_parms);


  /// Destructor.
  virtual ~xtsc_l2cc(void);


  /// For xtsc_connection_interface
  virtual u32 get_bit_width(const std::string& port_name, xtsc::u32 interface_num = 0) const;


  /// For xtsc_connection_interface
  virtual sc_core::sc_object *get_port(const std::string& port_name);


  /// For xtsc_connection_interface
  virtual std::string get_default_port_name() const { return "core0data_rd"; }


  /// For xtsc_connection_interface
  virtual xtsc_port_table get_port_table(const std::string& port_table_name) const;


  /// For xtsc_mode_switch_if
  virtual bool prepare_to_switch_sim_mode(xtsc_sim_mode mode);


  /// For xtsc_mode_switch_if
  virtual bool switch_sim_mode(xtsc_sim_mode mode);


  /// For xtsc_mode_switch_if
  virtual sc_core::sc_event &get_sim_mode_switch_ready_event();


  /// For xtsc_mode_switch_if
  virtual xtsc::xtsc_sim_mode get_sim_mode() const;


  /// For xtsc_mode_switch_if
  virtual bool is_mode_switch_pending() const;


  /**
   * Implementation of the xtsc::xtsc_command_handler_interface.
   *
   * This implementation supports the following commands:
   * \verbatim
       change_clock_period <ClockPeriodFactor>
         Call xtsc_l2cc::change_clock_period(<ClockPeriodFactor>).  Return previous
         <ClockPeriodFactor> for this L2CC.

       reset [<Hard>]
         Call xtsc_cache::reset(<Hard>).  Where <Hard> is 0|1 (default 0).

     \endverbatim
  */
  void execute(const std::string&               cmd_line,
               const std::vector<std::string>&  words,
               const std::vector<std::string>&  words_lc,
               std::ostream&                    result);


  void start_of_simulation();

private:

  log4xtensa::TextLogger&              m_text;                                 ///< For logging text messages

  // Note: Keep m_p last in class
  xtsc_l2cc_parts&                     m_p;                                    ///< Internal parts

  friend class xtsc_l2cc_parts;
};
}  // namespace xtsc


#endif  // _XTSC_L2CC_H_
