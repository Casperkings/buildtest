#ifndef _XTSC_UDMA_H_
#define _XTSC_UDMA_H_

// Copyright (c) 2006-2014 by Tensilica Inc.  ALL RIGHTS RESERVED.
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
#include <xtsc/xtsc_lookup_if.h>
#include <xtsc/xtsc_respond_if.h>
#include <xtsc/xtsc_request_if.h>
#include <xtsc/xtsc_request.h>
#include <xtsc/xtsc_response.h>
#include <xtsc/xtsc_wire_write_if.h>
#include <xtsc/xtsc_fast_access.h>


namespace xtsc {


 /**
  * Constructor parameters for an xtsc_udma object.
  *
  * This class contains the constructor parameters for a xtsc_udma object.
  *  \verbatim
    Name                      Type    Description
    ----------------------    ----    --------------------------------------------------

    "ram_byte_width"          u32     The byte width of the data interface of the local
                                      memory slaves. NOTE: this parameter must be an
                                      integral multiple of "pif_byte_width" parameter.
                                      Default = 64.

    "ram_start_address"       u32     The starting byte address of the local memory.
                                      Default = 0x3FF70000 (i.e. a single memory model).

    "ram_start_addresses"     vector<u32> This parameter is a std::vector containing
                                      the starting byte address of all local memories
                                      connected to the xtsc_udma module. This parameter
                                      is used when more than one xtsc_memory component
                                      is connected to xtsc_udma.
                                      Default (unset).

    "ram_byte_size"           u32     The byte size of the local memory.
                                      Default = 0x1000 (i.e. a single memory model).

    "ram_byte_sizes"          vector<u32>  This parameter is a std::vector containing
                                      the byte size of all local memories connected to
                                      the xtsc_udma module.  Note:  the size of this
                                      vector must be equal to the size of
                                      "ram_start_addresses" parameter.
                                      Default (unset).

    "pif_byte_width"          u32     The byte width of the data interface of PIF slave.
                                      Default = 16.

    "pif_out_fifo_depth"      u32     The depth of PIF output fifo.
                                      Default = 4.

    "ram_read_priority"       u32     Priority for local RAM READ requests. xtsc_udma
                                      must have the highest READ priority among all
                                      masters connected to the local memory. Response
                                      type of RSP_NACC, DATA_ERROR, and ADDRESS_ERROR
                                      are not allowed at run-time.
                                      Valid values are 0|1|2|3.
                                      Default = (i.e. the highest priority).

    "ram_write_priority"      u32     Priority for local RAM WRITE requests. xtsc_udma
                                      must have the highest WRITE priority among all
                                      masters connected to the local memory. Response
                                      type of RSP_NACC, DATA_ERROR, and ADDRESS_ERROR
                                      are not allowed at run-time.
                                      Valid values are 0|1|2|3.
                                      Default = 3 (i.e. the highest priority).

    "pif_read_priority"       u32     Priority for PIF READ/BLOCK_READ requests.
                                      Valid values are 0|1|2|3.
                                      Default = 3 (i.e. the highest priority).

    "pif_write_priority"      u32     Priority for PIF WRITE/BLOCK_WRITE requests.
                                      Valid values are 0|1|2|3.
                                      Default = 3 (i.e. the highest priority).

    "ram_read_delay"          u32     The number of clock periods required between
                                      sending a local RAM READ request and receiving
                                      the corresponding response.
                                      Default = 2.

    "ram_write_delay"         u32     The number of clock periods required between
                                      sending a local RAM  WRITE request and receiving
                                      the corresponding response.
                                      Default = 2.

    "rer_address_bit_width"   u32     Width of RER request address in bits.
                                      Default = 32.

    "rer_data_bit_width"      u32     Width of RER response data in bits.
                                      Default = 32.

    "wer_address_bit_width"   u32     Width of WER request address in bits
                                      Default = 64.

    "wer_data_bit_width"      u32     Width of WER response data in bits.
                                      Default = 1.

    "clock_period"            u32     This is the length of this xtsc_udma's clock
                                      period expressed in terms of the SystemC time
                                      resolution (from sc_get_time_resolution()). A
                                      value of 0xFFFFFFFF means to use the XTSC system
                                      clock period (from xtsc_get_system_clock_period()).
                                      A value of 0 means one delta cycle.
                                      Default = 0xFFFFFFFF (i.e. use the system clock
                                      period).

    "nacc_wait_time"          u32     This parameter, expressed in terms of the SystemC
                                      time resolution, specifies how long to wait after
                                      sending a request downstream to see if it was
                                      rejected by RSP_NACC. This value must not exceed
                                      this device's clock period. A value of 0 means
                                      one delta cycle. A value of 0xFFFFFFFF means to
                                      wait for a period equal to this device's clock
                                      period. CAUTION:  A value of 0 can cause an
                                      infinite loop in the simulation if the downstream
                                      module requires a non-zero time to become available.
                                      Default = 0xFFFFFFFF (device's clock period).

    "posedge_offset"          u32     This specifies the time at which the first
                                      posedge of this device's clock  conceptually
                                      occurs. It is expressed in units of the SystemC
                                      time resolution and the value implied by it must
                                      be strictly less than the value implied by the
                                      "clock_period" parameter. A value of 0xFFFFFFFF
                                      means to use the same posedge offset as the system
                                      clock (from
                                      xtsc_get_system_clock_posedge_offset()).
                                      Default = 0xFFFFFFFF.

    "sync_offset"             u32     This specifies the synchronization offset from
                                      clock posedge, which xtsc_udma uses to send both
                                      its local RAM and PIF requests (i.e. the point
                                      that an nb_request can be called). It is expressed
                                      in terms of the SystemC time resolution (from
                                      sc_get_time_resolution()). A value of 0 means to
                                      synchronize at clock posedge as specified by
                                      "posedge_offset". A value of 0xFFFFFFFF means to
                                      use the Phase B of the system clock (from
                                      get_clock_phase_delta_factors()).
                                      This means sync_offset is equal to
                                           "posedge_offset + (pda_a + pda_b)"
                                      (i.e. the point that xtsc_core calls nb_request()
                                      to its memories).
                                      Default = 0xFFFFFFFF.

    "turbo"                   bool    If true, turboXim fast access mode will be used
                                      to implement uDMA data movement. If false, then
                                      READ|WRITE or BLOCK_READ|BLOCK_WRITE requests
                                      will be used to model a cycle-accurate
                                      implementation of xtsc_udma. The value set by
                                      this parameter can be changed by
                                      xtsc_switch_sim_mode(xtsc_sim_mode mode) method
                                      at run-time. xtsc_udma switches from the cycle-
                                      accurate simulation to the fast functional
                                      simulation at completion of current descriptor,
                                      while it switches from the fast mode to cycle-
                                      accurate mode at the end of the execution of
                                      all programmed descriptors.
                                      Default = As specified by the "turbo" parameter
                                      of xtsc_initialize_parms.

    "use_peek_poke"           bool    If true, xtsc_udma will only use nb_peek() and
                                      nb_poke() methods from xtsc_fast_access interface
                                      to implement the turbo simulation.
                                      If false, the downstream module specifies what
                                      memory address ranges support fast access and
                                      which fast access method to use (raw access,
                                      peek/poke access, etc).
                                      Default = true.

    "log_single_line"         bool    If true, the current descriptor's information
                                      is logged in a brief, single-line format.
                                      If false, the current descriptor's information
                                      is logged in detail with a full description of
                                      each field.
                                      Default = true.

  \endverbatim
 *
 * @see xtsc_udma
 * @see xtsc::xtsc_parms
 */

class XTSC_API xtsc_udma_parms : public xtsc_parms {
public:

  /**
   * Constructor for an xtsc_udma object. After the object is constructed, the
   * data members can be directly written using the appropriate xtsc_parms::set() method
   * in cases where non-default values are desired.
   *
   * @param     ram_byte_width       The byte width of the data interface of the local memory.
   *
   * @param     ram_start_address    Starting byte address of the local memory.
   *
   * @param     ram_byte_size        Local memory byte size.
   *
   * @param     pif_byte_width       The byte width of the data interface of PIF slave.
   *
   */
  xtsc_udma_parms(u32   ram_byte_width            = 64,
                  u32   ram_start_address         = 0x3FF70000,
                  u32   ram_byte_size             = 0x1000,
                  u32   pif_byte_width            = 16)
  {
    init(ram_byte_width, ram_start_address, ram_byte_size, pif_byte_width);
  }

  /**
   * Do initialization.
   */
  void init(u32   ram_byte_width            = 64,
            u32   ram_start_address         = 0x3FF70000,
            u32   ram_byte_size             = 0x1000,
            u32   pif_byte_width            = 16)
  {
    std::vector<u32>                ram_start_addresses;
    std::vector<u32>                ram_byte_sizes;
    bool turbo = xtsc_get_xtsc_initialize_parms().get_bool("turbo");

    add("ram_byte_width",           ram_byte_width);
    add("ram_start_address",        ram_start_address);
    add("ram_byte_size",            ram_byte_size);
    add("ram_start_addresses",      ram_start_addresses);
    add("ram_byte_sizes",           ram_byte_sizes);
    add("pif_byte_width",           pif_byte_width);
    add("pif_out_fifo_depth",       0x4);
    add("ram_read_priority",        0x3);
    add("ram_write_priority",       0x3);
    add("pif_read_priority",        0x3);
    add("pif_write_priority",       0x3);
    add("ram_read_delay",           0x2);
    add("ram_write_delay",          0x2);
    add("rer_address_bit_width",    32);
    add("rer_data_bit_width",       32);
    add("wer_address_bit_width",    64);
    add("wer_data_bit_width",       1);
    add("clock_period",             0xFFFFFFFF);
    add("nacc_wait_time",           0xFFFFFFFF);
    add("posedge_offset",           0xFFFFFFFF);
    add("sync_offset",              0xFFFFFFFF);
    add("turbo",                    turbo);
    add("use_peek_poke",            true);
    add("log_single_line",          true);
  }

  /// Our C++ type (the xtsc_parms base class uses this for error messages)
  virtual const char* kind() const { return "xtsc_udma_parms"; }

};


/**
 * The xtsc_udma class provides an XTSC model of Cadence/Tensilica's micro-DMA engine (uDMA).
 * It is used to transfer data between a core's local data RAMs and memories external to the core.
 *
 * The xtsc_udma operations are defined by uDMA descriptors located in a core's local
 * memory. The xtsc_udma module is then programmed by writing to its software accessible,
 * programming registers to set the pointers to the descriptors and define the number of
 * descriptors to execute. The xtsc_udma completion can be determined by a programmed
 * interrupt or polling the xtsc_udma status register.
 *
 * An xtcs_udma descriptor is comprised of:
 * - Source address
 * - Destination address
 * - Control (syncInterrupt enable, maximum block size, maximum
 *            outstanding PIF requests, number of bytes to transfer)
 * - Number of rows
 * - Source pitch
 * - Destination pitch
 *
 * The xtsc_udma limits the source and destination address alignments:
 * - For partial size transfers (1, 2, 4, 8, 16 Bytes):
 *   (a) both source and destination address must have the same 16B alignment,
 *   (b) both source and destination address must be aligned to the number
 *      of bytes being transferred.
 * - For block size transfers (32, 64, 128, 256 Bytes):
 *   (a) the total number of bytes must be an integer multiple of the block size,
 *   (b) the external memory address must be aligned to the byte size of
 *      the block transaction,
 *   (c) the local memory address must be aligned to the smaller of the
 *      local memory byte width or the block transaction byte size,
 *   (d) the row transfer must not cross the local memory boundaries.
 *
 * The xtsc_udma has a set of software accessible registers, that can be programmed via RER and WER
 * instructions:
 * \verbatim
 --------------------------------------------------------------------------------------------
  Register Name   WER/RER Address     Description
 --------------------------------------------------------------------------------------------
  Config          0xD0000000          Configuration register, which includes the following
                                      fields.
                                         Enable: Index 0     0: Disabled, 1: Enabled
                                         NMI:    Index 1     0: Disabled, 1: Enabled
                                         Version:Index 31:28
  Status          0xD0000001          Status register, which includes the following fields.
                                         Busy:   Index 0     Busy processing uDMA descriptors
                                         Error:  Index 31:16 One-hot error bits:
                                                            0x0001: Bad descriptor
                                                            0x0002: DMA crosses DRam boundary
                                                            0x0004: PIF address bus error
                                                            0x0008: PIF data bus error
  SrcPtr           0xD0000002          Current source address register.
  DestPtr          0xD0000003          Current destination address register.
  DescFirstPtr     0xD0000010          Descriptor first pointer in memory.
  DescLastPtr      0xD0000011          Descriptor last pointer in memory.
  DescCurPtr       0xD0000012          Descriptor current pointer in memory.
  DescNum          0xD0000013          Number of descriptor to execute. Valid values 0 to 63.
  DescNumIncr      0xD0000014          Increment number of descriptors. Write-only.

 \endverbatim
 *
 * see xtsc_udma_parms
 */
class XTSC_API xtsc_udma : public sc_core::sc_module
                         , public xtsc_module
                         , public xtsc_mode_switch_if
                         , public xtsc_command_handler_interface
{

public:

  sc_core::sc_export<xtsc_lookup_if>           m_rer_export;              ///< Tie lookup RER interface
  sc_core::sc_export<xtsc_lookup_if>           m_wer_export;              ///< Tie lookup WER interface

  sc_core::sc_port  <xtsc_request_if>          m_ram_request_port;        ///< From uDMA to local memory
  sc_core::sc_export<xtsc_respond_if>          m_ram_respond_export;      ///< From local memory to uDMA

  sc_core::sc_port  <xtsc_request_if>          m_pif_request_port;        ///< From uDMA to PIF
  sc_core::sc_export<xtsc_respond_if>          m_pif_respond_export;      ///< From PIF to uDMA

  sc_core::sc_port  <xtsc_wire_write_if, NSPP> m_sync_intr_port;          ///< Sync interrupt port
  sc_core::sc_port  <xtsc_wire_write_if, NSPP> m_error_intr_port;         ///< Error interrupt port


  /// This SystemC macro inserts some code required for SC_THREAD's to work
  SC_HAS_PROCESS(xtsc_udma);


  /// Our C++ type (SystemC uses this)
  virtual const char* kind() const { return "xtsc_udma"; }


  /**
   * Constructor for a xtsc_udma.
   *
   * @param     module_name     Name of the xtsc_udma sc_module.
   *
   * @param     udma_parms      The remaining parameters for construction.
   *
   * @see xtsc_udma_parms
   */
  xtsc_udma(const sc_core::sc_module_name module_name, const xtsc_udma_parms& udma_parms);


  /// Destructor
  ~xtsc_udma();


  /// For xtsc_connection_interface
  virtual u32 get_bit_width(const std::string& port_name, u32 interface_num = 0) const;


  /// For xtsc_connection_interface
  virtual sc_core::sc_object *get_port(const std::string& port_name);


  /// For xtsc_connection_interface
  virtual std::string get_default_port_name() const { return "rer_export"; }


  /// For xtsc_connection_interface
  virtual xtsc_port_table get_port_table(const std::string& port_table_name) const;


  /// for xtsc_mode_switch_if
  virtual bool prepare_to_switch_sim_mode(xtsc_sim_mode mode);


  /// For xtsc_mode_switch_if
  virtual bool switch_sim_mode(xtsc_sim_mode mode);


  /// For xtsc_mode_switch_if
  virtual sc_core::sc_event &get_sim_mode_switch_ready_event() { return m_ready_to_switch_event; }


  /// For xtsc_mode_switch_if
  virtual xtsc::xtsc_sim_mode get_sim_mode() const { return ((m_turbo) ? xtsc::XTSC_FUNCTIONAL : xtsc::XTSC_CYCLE_ACCURATE);};


  /// For xtsc_mode_switch_if
  virtual bool is_mode_switch_pending() const { return m_prepare_to_switch; }


  /**
   * Method to change the clock period.
   *
   * @param     clock_period_factor     Specifies the new length of this device's clock
   *                                    period expressed in terms of the SystemC time
   *                                    resolution (from sc_get_time_resolution()).
   */
  void change_clock_period(xtsc::u32 clock_period_factor);


  /**
   * This method dumps the current descriptor info to the specified ostream object.
   *
   * @param     os            The ostream object to which the dump should be done.
   *
   * @param     single_line   If true, a single line format will be used.
   *
   * If the single_line option is selected, the format of the output is:
   * \verbatim
       Descriptor (<address>) [<SA>/<DA>/<NumBytes>/<NumTransfers>/<NumOutstanding>/
                               <NumRows>/<SP>/<DP>/<IntrEnable>]

       Where:
        <address>             is m_desc_cur_ptr in hexadecimal.
        <SA>                  is m_descriptor.source_address in hexadecimal.
        <DA>                  is m_descriptor.destination_address in hexadecimal.
        <NumBytes>            is m_descriptor.num_bytes in decimal.
        <NumTransfers>        is m_descriptor.max_block_transfers in decimal.
        <NumOutstanding>      is m_descriptor.max_num_out in decimal.
        <NumRows>             is m_descriptor.num_rows in decimal.
        <SP>                  is m_descriptor.source_pitch in hexadecimal.
        <DP>                  is m_descriptor.destination_pitch in hexadecimal.
        <IntrEnable>          is is m_descriptor.sync_intr_enabled in boolean.

   \endverbatim
  */
  void dump_descriptor(std::ostream &os, bool single_line = true) const;


  /**
   * This method dumps current descriptor profile results.
   *
   * @param     os            The ostream object to which the dump should be done.
   *
   * The format of output is:
   * \verbatim
       Descriptor (<address>): Cycle count = <CC>, transfer rate = <TR>

       Where:
        <address>      is m_desc_cur_ptr in hexadecimal.
        <CC>           is the total number of cycles in decimal.
        <TR>           is the number of bytes transfered per cycle in double.
                       Calculated as (m_descriptor.num_rows x m_descriptor.num_bytes) / <CC>.
   \endverbatim
  */
  void dump_profile_results(std::ostream &os) const;


  /// Reset the xtsc_udma.
  void reset(bool hard_reset = false);


  /**
   * Implementation of the xtsc::xtsc_command_handler_interface.
   *
   * This implementation supports the following commands:
   * \verbatim
       change_clock_period <ClockPeriodFactor>
        Call xtsc_udma::change_clock_period(<ClockPeriodFactor>).  Return previous
        <ClockPeriodFactor> for this uDMA.

       dump_descriptor
        Call xtsc_udma::dump_descriptor(). Dump the current descriptor details.

       dump_profile_results
        Call xtsc_udma::dump_profile_results(). Dump the current descriptor execution info.

       reset
        Call xtsc_udma::reset().

   \endverbatim
  */
  void execute(const std::string&               cmd_line,
               const std::vector<std::string>&  words,
               const std::vector<std::string>&  words_lc,
               std::ostream&                    result);


protected:
  /**
   * Enumeration used to identify software accessible registers.
   */
  enum register_address_map {
    CONFIG = 0xD0000000,                 ///< Configuration register
    STATUS,                              ///< DMA status
    SRCPTR,                              ///< Current source address pointer
    DESTPTR,                             ///< Current destination address pointer
    DESCFIRSTPTR = 0xD0000010,           ///< Descriptor first pointer in memory
    DESCLASTPTR,                         ///< Descriptor last pointer in memory
    DESCCURPTR,                          ///< Descriptor current pointer in memory
    DESCNUM,                             ///< Number of descriptors to execute
    DESCNUMINCR,                         ///< Increament number of DMA descriptors. Write-ony.
    NUMREGS = 9                          ///< Number of registers
  };

  /**
   * Enumeration used to identify the access direction.
  */
  typedef enum direction_t {
    RAM2PIF   = 0,                       ///< Local RAM to PIF transfers
    PIF2RAM   = 1,                       ///< PIF to local RAM transfers
    RAM2RAM   = 2,                       ///< Local RAM to local RAM transfers
    PIF2PIF   = 3                        ///< PIF to PIF transfers
  } direction_t;

  /**
   * Enumeration used to identify DMA error codes.
   */
  typedef enum error_t {
    BAD_DESCRIPTOR_ERROR     = 0x0001,   ///< Bad descriptor
    CROSS_RAM_BOUNDARY_ERROR = 0x0002,   ///< DMA crosses DRAM boundary
    PIF_ADDRESS_ERROR        = 0x0004,   ///< PIF address bus error
    PIF_DATA_ERROR           = 0x0008,   ///< PIF data bus error
    PIF_ADDRESS_DATA_ERROR   = 0x0010,   ///< PIF address and data bus error
    UDMA_OK                  = 0x0000    ///< DMA OK (no error)
  } error_t;

  /**
   * Data structure used to store a uDMA descriptor.
   */
  typedef struct udma_descriptor {
    u32 source_address;                  ///< Source address
    u32 destination_address;             ///< Destination address
    u32 source_pitch;                    ///< Source pitch
    u32 destination_pitch;               ///< Destination pitch
    u16 num_rows;                        ///< Number of rows to transfer
    u16 num_bytes;                       ///< Number of bytes per row to transfer
    u16 max_block_transfers;             ///< Maximum block size for DMA (valid values are 2|4|8|16 transfers)
    u16 max_num_out;                     ///< Maximum number of outstanding requests with a range of 1-16
    bool sync_intr_enabled;              ///< Enable Sync interrupt on successful completion of descriptor
  } udma_descriptor;

  /// Implementation of xtsc_lookup_if for RER port.
  class xtsc_rer_lookup_if_impl: public xtsc_lookup_if, public sc_core::sc_object  {
  public:

    /// Constructor
    xtsc_rer_lookup_if_impl(const char *object_name, xtsc_udma& udma) :
      sc_object    (object_name),
      m_udma       (udma),
      m_p_port     (0)
    {}

    /// @see xtsc::xtsc_lookup_if
    void nb_send_address(const sc_dt::sc_unsigned& address);

    /// @see xtsc::xtsc_lookup_if
    bool nb_is_ready();

    /// @see xtsc::xtsc_lookup_if
    sc_dt::sc_unsigned nb_get_data();

    /// @see xtsc::xtsc_lookup_if
    u32 nb_get_address_bit_width() { return m_udma.m_rer_address_bit_width; }

    /// @see xtsc::xtsc_lookup_if
    u32 nb_get_data_bit_width() { return m_udma.m_rer_data_bit_width; }

    /**
     * Get the event that will be notified when the lookup data is available.
     */
    virtual const sc_core::sc_event& default_event() const {
      // Only application in turbo mode
      return m_udma.m_turbo_finish_event;
    }

  protected:

    /// SystemC callback when something binds to us
    virtual void register_port(sc_core::sc_port_base& port, const char *if_typename);

    xtsc_udma&                  m_udma;         ///< Our xtsc_udma object
    sc_core::sc_port_base      *m_p_port;       ///< Port that is bound to us
  };

  /// Implementation of xtsc_lookup_if for WER port.
  class xtsc_wer_lookup_if_impl: public xtsc_lookup_if, public sc_core::sc_object  {
  public:

    /// Constructor
    xtsc_wer_lookup_if_impl(const char *object_name, xtsc_udma& udma) :
      sc_object    (object_name),
      m_udma       (udma),
      m_p_port     (0)
    {}

    /// @see xtsc::xtsc_lookup_if
    void nb_send_address(const sc_dt::sc_unsigned& address);

    /// @see xtsc::xtsc_lookup_if
    bool nb_is_ready();

    /// @see xtsc::xtsc_lookup_if
    sc_dt::sc_unsigned nb_get_data();

    /// @see xtsc::xtsc_lookup_if
    u32 nb_get_address_bit_width() { return m_udma.m_wer_address_bit_width; }

    /// @see xtsc::xtsc_lookup_if
    u32 nb_get_data_bit_width() { return m_udma.m_wer_data_bit_width; }

    /**
     * Get the event that will be notified when the lookup data is available.
     */
    virtual const sc_core::sc_event& default_event() const {
      // Only applicable in turbo mode
      return m_udma.m_turbo_finish_event;
    }

  protected:

    /// SystemC callback when something binds to us
    virtual void register_port(sc_core::sc_port_base& port, const char *if_typename);

    xtsc_udma&                  m_udma;         ///< Our xtsc_udma object
    sc_core::sc_port_base      *m_p_port;       ///< Port that is bound to us
  };


  /// Implementation of xtsc_respond_if for local RAM port.
  class xtsc_ram_respond_if_impl : public xtsc_respond_if, public sc_core::sc_object {
  public:

    /// Constructor.
    xtsc_ram_respond_if_impl(const char *object_name, xtsc_udma& udma) :
      sc_object   (object_name),
      m_udma      (udma),
      m_p_port    (0)
    {}

    /// @see xtsc::xtsc_respond_if
    bool nb_respond(const xtsc::xtsc_response& response);

  protected:

    /// SystemC callback when something binds to us
    virtual void register_port(sc_core::sc_port_base& port, const char *if_typename);

    xtsc_udma&                  m_udma;         ///< Our xtsc_udma object
    sc_core::sc_port_base      *m_p_port;       ///< Port that is bound to us
  };


  /// Implementation of xtsc_respond_if for system RAM port.
  class xtsc_pif_respond_if_impl : public xtsc_respond_if, public sc_core::sc_object {
  public:

    /// Constructor
    xtsc_pif_respond_if_impl(const char *object_name, xtsc_udma& udma) :
      sc_object    (object_name),
      m_udma       (udma),
      m_p_port     (0)
    {}

    /// @see xtsc::xtsc_respond_if
    bool nb_respond(const xtsc_response& response);

  protected:

    /// SystemC callback when something binds to us
    virtual void register_port(sc_core::sc_port_base& port, const char *if_typename);

    xtsc_udma&                  m_udma;         ///< Our xtsc_udma object
    sc_core::sc_port_base      *m_p_port;       ///< Port that is bound to us
  };



  /// Check crossing RAM boundaries and direction errors on each row
  error_t check_row_errors(xtsc_address source_address, xtsc_address destination_address);


  /// Compute PIF block transfer size (returns false if BAD_DESCRIPTOR error happens)
  bool compute_block_transfers();


  /// Common method to compute/re-compute time delays
  void compute_delays();


  /// This method implements local RAM read transfers in a cycle-accurate simulation
  error_t do_local_read_transfers();


  /// This method implements local RAM write transfers in a cycle-accurate simulation
  error_t do_local_write_transfers();


  /// This method implements PIF block read transfers in a cycle-accurate simulation
  error_t do_pif_block_read_transfers();


  /// This method implements PIF block write transfers in a cycle-accurate simulation
  error_t do_pif_block_write_transfers();


  /// This method implements PIF partial read transfers in a cycle-accurate simulation
  error_t do_pif_single_read_transfers();


  /// This method implements PIF partial write transfers in a cycle-accurate simulation
  error_t do_pif_single_write_transfers();


  /// This method implements uDMA operation in turbo mode
  void do_turbo_transfers();


  /// Flush PIF output fifo
  void flush_pif_fifo();


  /// Determine uDMA direction
  bool get_udma_direction(xtsc_address source_address, xtsc_address destination_address, direction_t &direction);


  /// Get the error name in string
  const char * get_error_name(error_t error);


  /// Get the xtsc_fast_access_request object pointer associated to the requested address
  xtsc_fast_access_request* get_fast_access_request(xtsc_address address, u32 num_bytes, u8 port_type);


  /// This method implements a cycle-ccurate local RAM read transaction
  void local_read(xtsc_address address, u32 byte_size, u8 *data);


  /// This method implements a cycle-accurate local RAM write transaction
  void local_write(xtsc_address address, u32 byte_size, const u8 *data);


  /// Check crossing local RAM boundary
  bool ram_boundary_crossing(xtsc_address start_address, u32 num_bytes);


  /// uDMA local RAM control thread
  void ram_control_thread(void);


  /// This method reads the descriptor located in address m_desc_cur_ptr from local RAM
  bool read_descriptor(udma_descriptor &descriptor);


  /// Read uDMA user interface register on tie lookup accesses
  u32 read_udma_register(xtsc_address address);


  /// Synchronize the simulated time to the next sync point
  void sync(void);


  /// Event-driven local RAM read transfer, used in turbo mode
  void turbo_local_read(xtsc_address address, u32 byte_size, u8 *data);


  /// Event-driven local RAM  write transfer, used in turbo mode
  void turbo_local_write(xtsc_address address, u32 byte_size, const u8 *data);


  /// Event-driven PIF read transfer, used in turbo mode
  void turbo_pif_read(xtsc_address address, u32 byte_size, u8 *data);


  /// Event-driven PIF write transfer, used in turbo mode
  void turbo_pif_write(xtsc_address address, u32 byte_size, const u8 *data);


  /// uDMA main thread (PIF control thread)
  void udma_control_thread(void);


  /// Write uDMA user interface registers on WER tie lookup accesses
  void write_udma_register(xtsc_address address, u32 write_data);


  /// Reset uDMA internal FIFOs
  void reset_fifos(void);

  /* xtsc_udma parameters */
  xtsc_udma_parms               m_udma_parms;                           ///< Copy of xtsc_udma_parms
  u32                           m_ram_byte_width;                       ///< See "ram_byte_width"
  u32                           m_pif_byte_width;                       ///< See "pif_byte_width"
  u8                            m_ram_read_priority;                    ///< See "ram_read_priority"
  u8                            m_ram_write_priority;                   ///< See "ram_write_priority"
  u8                            m_pif_read_priority;                    ///< See "pif_read_priority"
  u8                            m_pif_write_priority;                   ///< See "pif_write_priority"
  sc_core::sc_time              m_clock_period;                         ///< See "clock_period"
  sc_core::sc_time              m_ram_read_delay;                       ///< See "ram_read_delay"
  sc_core::sc_time              m_ram_write_delay;                      ///< See "ram_write_delay"
  sc_core::sc_time              m_nacc_wait_time;                       ///< See "nacc_wait_time"
  sc_core::sc_time              m_posedge_offset;                       ///< See "posedge_offset"
  sc_core::sc_time              m_sync_offset;                          ///< See "sync_offset"
  std::vector<u32>              m_ram_start_addresses;                  ///< See "ram_start_addresses"
  std::vector<u32>              m_ram_byte_sizes;                       ///< See "ram_byte_sizes"
  bool                          m_turbo;                                ///< See "turbo"
  bool                          m_use_peek_poke;                        ///< See "use_peek_poke"
  bool                          m_log_single_line;                      ///< See "log_single_line"
  u8                            m_udma_version;                         ///< See "udma_version"

  /* User interface internal registers */
  xtsc_address                  m_source_address;                       ///< Current source address
  xtsc_address                  m_destination_address;                  ///< Current destination address
  xtsc_address                  m_desc_first_ptr;                       ///< Descriptor first pointer in memory
  xtsc_address                  m_desc_last_ptr;                        ///< Descriptor last pointer in memory
  xtsc_address                  m_desc_cur_ptr;                         ///< Descriptor current pointer in memory
  u32                           m_desc_num;                             ///< Number of descriptors to execute

  /* uDMA status */
  u16                           m_error_status;                         ///< Keep the error state of uDMA operation
  bool                          m_udma_enable;                          ///< Indicate that uDMA is enabled to start its operation
  bool                          m_busy;                                 ///< If true, uDMA is busy and transferring data
  bool                          m_error_intr_enabled;                   ///< If true, assert an error interrupt at the completion of the current descriptor (if needed)

  /* uDMA descriptor */
  udma_descriptor               m_descriptor;                           ///< Current descriptor
  direction_t                   m_udma_direction;                       ///< uDMA transfer direction (RAM2PIF or PIF2RAM)
  u32                           m_block_transfers;                      ///< Block transfer size (2,4,8, or 16 transfers)

  /* uDMA timing parameters */
  u64                           m_clock_period_value;                   ///< This device's clock period as u64
  sc_core::sc_time              m_time_resolution;                      ///< The SystemC time resolution
  sc_core::sc_time              m_sync_offset_plus_one;                 ///< The sync offset on the next clock cycle

  /* Local RAM interface variables */
  u32                           m_ram_transfer_byte_size;               ///< Local ram transfer byte size, needed when RAM byte width is greater than the transfer byte number
  u32                           m_ram_words_index;                      ///< Index of stored data in m_ram_words buffer
  u8                            m_ram_words[xtsc::xtsc_max_bus_width8]; ///< Buffer used for collecting data from PIF and sending it to the local RAM
  std::vector<u32>              m_ram_end_addresses;                    ///< End address list of local memories, initialized in the uDMA constructor
  sc_core::sc_event             m_dma_thread_start_event;               ///< Signal uDMA to start its operation
  sc_core::sc_event             m_ram_control_start_event;              ///< Signal ram_control_thread to start its operation
  sc_core::sc_event             m_ram_words_available_event;            ///< Notify ram_control_thread when ram write data is collected via PIF port and is ready to send

  /* PIF interface */
  sc_core::sc_fifo<u8*>        *m_pif_output_fifo;                      ///< Used to buffer write data to be sent on PIF port
  u32                           m_pif_response_count;                   ///< PIF response count, used in block transfers
  u32                           m_pif_transfer_byte_size;               ///< Byte size of partial or block PIF transfers
  u16                           m_pif_error;                            ///< PIF bus error code
  bool                          m_terminate_transfers;                  ///< uDMA threads notify transfers terminations when a PIF error happens

  /* XTSC_CORE RER & WER interface */
  xtsc_rer_lookup_if_impl      *m_rer_lookup_impl;                      ///< RER xtsc_lookup_if implementation, m_rer_lookup_export binds to this
  u32                           m_rer_address_bit_width;                ///< RER tie lookup address bit width
  u32                           m_rer_data_bit_width;                   ///< RER tie lookup data bit width
  sc_dt::sc_unsigned            m_rer_data;                             ///< RER tie lookup data
  xtsc_wer_lookup_if_impl      *m_wer_lookup_impl;                      ///< WER xtsc_lookup_if implementation, m_wer_lookup_export binds to this
  u32                           m_wer_address_bit_width;                ///< WER tie lookup address bit width
  u32                           m_wer_data_bit_width;                   ///< WER tie lookup data bit width
  //sc_core::sc_event             m_lookup_ready_event;                   ///< RER/WER tie lookup ready event

  /* Local RAM interface implementation */
  xtsc_ram_respond_if_impl     *m_ram_respond_impl;                     ///< Local RAM xtsc_response_if implementation
  xtsc_response                *m_ram_response;                         ///< Pointer to a local RAM response
  sc_core::sc_event             m_ram_response_available_event;         ///< To notify that uDMA has received a local RAM response

  /* PIF interface implementation */
  xtsc_pif_respond_if_impl     *m_pif_respond_impl;                     ///< PIF xtsc_response_if implementation
  xtsc_response                *m_pif_response;                         ///< Pointer to a PIF response
  sc_core::sc_event             m_pif_response_available_event;         ///< To notify that uDMA has received a PIF response
  u16                           m_num_out_pif_requests;                 ///< Keep the number of outstanding PIF requests

  /* Turbo simulation */
  //#ifdef UDMA_SUPPORT_FAST_ACCESS
  std::vector<xtsc_fast_access_request*> m_fast_access_list;            ///< Keep fast access information for turbo mode simulation
  //#endif
  bool                          m_prepare_to_switch;                    ///< Indicate switch to turbo mode
  sc_core::sc_event             m_ready_to_switch_event;                ///< Signal ready to switch simulation mode
  sc_core::sc_event             m_switch_mode_done_event;               ///< Signal a successful simulation mode switch
  sc_core::sc_event             m_turbo_finish_event;                   ///< Indicate completion of turbo transfers for the programmed uDMA

  /* Profiling info */
  u64                           m_descriptor_cycle_count;               ///< Total number of cycles for teh current descriptor execution
  double                        m_descriptor_transfer_rate;             ///< Transfer rate [bytes/cycle] of the current descriptor

  /* Reset uDMA state */
  std::vector<sc_core::sc_process_handle> m_process_handles;            ///<  For reset

  log4xtensa::TextLogger&       m_text;                                 ///< Used for logging

  /* Constant data members */
  static const u8               m_read_id               = 0x3;          ///< xtsc_request::m_id for READ
  static const u8               m_write_id              = 0x9;          ///< xtsc_request::m_id for WRITE
  static const u8               m_block_read_id         = 0x7;          ///< xtsc_request::m_id for BLOCK_READ
  static const u8               m_block_write_id        = 0x5;          ///< xtsc_request::m_id for BLOCK_WRITE
  static const u32              m_descriptor_byte_size  = 32;           ///< Descriptor byte size

};

}  // namespace xtsc

#endif // _XTSC_UDMA_H_
