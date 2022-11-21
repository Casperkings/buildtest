#ifndef _XTSC_DMA_ENGINE_H_
#define _XTSC_DMA_ENGINE_H_

// Copyright (c) 2005-2017 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of
// Cadence Design Systems, Inc.  They may be adapted and modified by bona fide
// purchasers for internal use, but neither the original nor any adapted
// or modified version may be disclosed or distributed to third parties
// in any manner, medium, or form, in whole or in part, without the prior
// written consent of Cadence Design Systems, Inc.

/**
 * @file 
 */


#include <xtsc/xtsc_memory.h>
#include "xtsc/xtsc_dma_request.h"
#include <deque>




namespace xtsc_component {


/**
 * Constructor parameters for a xtsc_dma_engine object.
 *
 * This class has the same contructor parameters as an xtsc_memory_parms object plus the
 * following additional ones.
 *  \verbatim
   Name                 Type    Description
   ------------------   ----    --------------------------------------------------------

   "reg_base_address"   u32     The base address of the DMA programming registers. The
                                BYTE_OFFSET specified in xtsc_dma_request and
                                xtsc_dma_descriptor documentation is refering to this
                                base address.

   "read_priority"      u32     Priority for READ|BLOCK_READ DMA requests.
                                Valid values are 0|1|2|3.
                                Default = 2.
  
   "write_priority"     u32     Priority for WRITE|BLOCK_WRITE DMA requests.
                                Valid values are 0|1|2|3.
                                Default = 2.
  
   "clear_notify_value" bool    If true, then when the DMA request completes, the
                                DMA request notify_address8 will be written twice in a
                                row.  The first write will contain the value specified by
                                DMA request notify_value, and the second write will
                                contain the value 0.  If false, then only the
                                notify_value will be written.  The purpose of writing
                                the notify_address8 twice is to support edge-triggered
                                interrupts.  If you are using a level-triggered
                                interrupt or if you are going to poll the
                                notify_address8 for the notify_value, then leave this
                                parameter at its default setting of false.  If you are
                                using an edge-triggered interrupt, then change this
                                parameter to true.
                                Note: To support a "dma done" interrupt, an xtsc_mmio
                                      device can be connected at the notify_address8 to
                                      convert the memory-mapped write into an interrupt.  
                                Caution: Use care when using interrupts to avoid a race
                                         condition that can occur if the processor
                                         interrupt level is not properly handled and the
                                         target has code to check if an interrupt has
                                         occurred, and if not, waits on it.  If the
                                         interrupt handler sets a dma_done flag, then
                                         target code to adjust the interrupt level
                                         before the check for dma_done might look like:
                                              _xtos_set_intlevel(15);
                                              if (!dma_done) { XT_WAITI(0); }
                                Default = false.
                                

   "max_reads"          u32     If non-zero, then this specifies the maximum number of
                                DMA read transactions that may be inflight (either
                                queued or downstream) at one time.  If zero, then
                                "max_writes" must also be zero and all DMA transactions
                                are done non-overlapping (so that at most 1 request,
                                whether read or write, is downstream at a time).
                                Note: At most one read request is queued per cycle.  
                                Note: Read requests all have the same ID and read
                                      responses must come back in the same order as
                                      the read requests were sent out.
                                Default = 0.

   "max_writes"         u32     If non-zero, then this specifies the maximum number of
                                DMA write transactions that may be inflight at one time
                                (a BLOCK_WRITE sequence of transfers counts as 1 write
                                transaction).  If zero, then "max_reads" must also be
                                zero and all DMA transactions are done non-overlapping
                                (so that at most 1 request, whether read or write, is
                                downstream at a time).
                                Default = 0.

   "overlap_descriptors" bool   If false, then the DMA engine will wait for all data
                                movement to complete on one descriptor before starting
                                the next descriptor.  If true, then the DMA engine will
                                move on to the next descriptor as soon as all reads for
                                the previous descriptor have been queued.  This
                                parameter must be left at its default value of false if
                                "max_reads" and "max_writes" are 0.
                                Default = false.

   "descriptor_delay"   u32     If non-zero, then a delay for this many clock periods is
                                done at the start of each descriptor to model the time
                                it takes to retrieve the descriptor from memory.  This 
                                parameter is ignored when "turbo" is in effect.
                                Default = 0.

   "reuse_tag"          bool    If true, each DMA write request will use the same tag as
                                its corresponding DMA read request.  If false, a
                                different tag will be used.
                                Default = true.

   "allow_size_zero"    bool    If false, the xtsc_dma_engine will throw an exception if
                                a descriptor with a total transfer byte size of 0 is
                                encountered (that is, if the xtsc_dma_descriptor size8
                                register contains 0).  If true, a size8 value of 0 will
                                be allowed.
                                Default = false.

   "start_at_index_1"   bool    If this parameter is left at its default value of true,
                                then the index of the first descriptor of a DMA request
                                to be processed will be 1.  This parameter can be set to
                                false to cause xtsc_dma_request::done_descriptor field
                                to be read and incremented (wrapping to 1 after 255) when
                                a DMA request is first started to determine the index of
                                the first descriptor to be processed.  This allows DMA
                                descriptors outside the range of the current DMA to be
                                programmed while the current DMA is in progress.  If
                                this parameter is true, then the done_descriptor field
                                must contain a value between 0 and 255 (inclusive) when
                                the "go" byte is written.  Note:  DMA descriptors may be
                                reprogrammed as soon as their index is written by the
                                DMA engine to the done_descriptor field.
                                Default = true.

   "turbo"              bool    If true, then nb_peek/nb_poke will be used to perform
                                the DMA data movement.  If false, then READ|WRITE or
                                BLOCK_READ|BLOCK_WRITE PIF requests will be used (as
                                determined by the xtsc_dma_descriptor num_transfers
                                register).  The value set by this parameter can be
                                overridden using the xtsc_dma_engine::set_turbo()
                                method.
                                Default = As specified by the "turbo" parameter of
                                xtsc_initialize_parms.
                                Note:  The xtsc_dma_engine module does not currently
                                       implement true TurboXim functionality which
                                       allows the downstream module to specify what
                                       memory address ranges support fast access and
                                       which fast access method to use (raw access,
                                       peek/poke access, etc).
                                Note:  If "turbo_min_sync" in xtsc_initialize_parms is
                                       set and the xtsc_core programming the DMA
                                       engine uses an edge-triggered interrupt for
                                       DMA done notification, then under TurboXim it is
                                       possible for the core to attempt to program
                                       the next DMA before the current DMA is complete
                                       resulting in an exception being thrown.


   "posedge_offset"     u32     This specifies the time at which the first posedge of
                                this device's clock conceptually occurs.  It is
                                expressed in units of the SystemC time resolution and
                                the value implied by it must be strictly less than the
                                value implied by the "clock_period" parameter.  A value
                                of 0xFFFFFFFF means to use the same posedge offset as
                                the system clock (from
                                xtsc_get_system_clock_posedge_offset()).
                                Default = 0xFFFFFFFF.

   "nacc_wait_time"     u32     This parameter, expressed in terms of the SystemC time
                                resolution, specifies how long to wait after sending a
                                request downstream to see if it was rejected by
                                RSP_NACC.  This value must not exceed this device's
                                clock period.  A value of 0 means one delta cycle.  A
                                value of 0xFFFFFFFF means to wait for a period equal to
                                this device's clock period.  CAUTION:  A value of 0 can
                                cause an infinite loop in the simulation if the
                                downstream module requires a non-zero time to become
                                available.
                                Default = 0xFFFFFFFF (device's clock period).

    \endverbatim
 *
 * @see xtsc_memory_parms
 * @see xtsc_dma_request
 * @see xtsc_dma_request::notify_address8
 * @see xtsc_dma_descriptor
 * @see xtsc_dma_engine
 * @see xtsc_mmio
 */
class XTSC_COMP_API xtsc_dma_engine_parms : public xtsc_memory_parms {
public:

  /// Constructor for an xtsc_dma_engine_parms object. 
  xtsc_dma_engine_parms(xtsc::u32   reg_base_address    = 0x00000000,
                        xtsc::u32   width8              = 4,
                        xtsc::u32   delay               = 0,
                        xtsc::u32   start_address8      = 0,
                        xtsc::u32   size8               = 0) :
    xtsc_memory_parms(width8, delay, start_address8, size8)
  {
    add("reg_base_address",     reg_base_address);
    add("read_priority",        2);
    add("write_priority",       2);
    add("clear_notify_value",   false);
    add("max_reads",            0);
    add("max_writes",           0);
    add("overlap_descriptors",  false);
    add("descriptor_delay",     0);
    add("reuse_tag",            true);
    add("allow_size_zero",      false);
    add("start_at_index_1",     true);
    add("turbo",                xtsc::xtsc_get_xtsc_initialize_parms().get_bool("turbo"));
    add("posedge_offset",       0xFFFFFFFF);
    add("nacc_wait_time",       0xFFFFFFFF);
  }

  /// Return what kind of xtsc_parms this is (our C++ type)
  const char *kind() const { return "xtsc_dma_engine_parms"; }
};



/**
 * An example DMA engine implementation.
 *
 * This implementation comprises a normal memory (xtsc_memory) that has been enhanced as
 * a DMA engine.  A block of addresses in the xtsc_memory device's address range is
 * reserved as DMA control registers.  The size of this block is (Nmax+1)*256 bytes,
 * where Nmax is the largest value that will appear in the num_descriptors register 
 * throughout the entire simulation (see xtsc_dma_request).  Nmax may not exceed 255.
 *
 * This implementation is not meant to model any particular real RTL; however, it has
 * several features that enable it to approximate many modern DMA engines especially
 * when used in conjunction with other XTSC components:
 *
 * - A single DMA request is comprised of 1 or more DMA descriptors.  Multiple DMA
 *   descriptors can be used to support scatter-gather operation.  Each descriptor
 *   can be programmed to use READ|WRITE requests or BLOCK_READ|BLOCK_WRITE requests
 *   to perform the data movement.  This module can also be configured to use turbo-
 *   style peek/poke operations to allow the data movement to be completed in 0
 *   SystemC simulation time.  See the "turbo" parameter and the set_turbo() method.
 *
 * - When a DMA request completes, a memory mapped write of a user-specified value to a
 *   user-specified address occurs to signal the end of the DMA.  If desired, an
 *   xtsc_mmio device can be used to converted this memory-mapped write into an
 *   interrupt signal.
 *
 * - This DMA engine module implements a single DMA channel.  If multiple DMA channels
 *   are desired, then instantiate this module multiple times and use xtsc_router and
 *   xtsc_arbiter objects to connect everything together.
 *
 * - By default the DMA engine module does non-overlapped data transfers; however, the
 *   "max_reads" and "max_writes" parameters may be set to allow multiple, overlapped
 *   reads and writes.
 *
 * - By default the DMA engine module completes one descriptor before starting the
 *   next; however, when overlapped reads and writes are enabled then the
 *   "overlapped_descriptors" parameter may be set to true to allow the next descriptor
 *   to be started as soon as all the reads are queued for the previous descriptor.
 *
 * Note: The throughput of the xtsc_dma_engine model is determined not only by its own
 * parameters such as "max_reads", "max_writes", and "overlapped_descriptors", but also
 * by the rest of the system.  Some of the parameters of other XTSC models that can
 * affect throughput include the "*_delay" and "request_fifo_depth" parameters of
 * xtsc_memory and "flexible_request_id" of xtsc_router.
 *
 * Only the DMA register space is internally accessed by the xtsc_dma_engine module.
 * If you wish to use an xtsc_dma_engine instance to perform DMA data movements either
 * from or to itself, then the master port pair (m_request_port and m_respond_export)
 * must be externally connected to one of the slave port pairs (m_request_ports[i] and
 * m_respond_exports[i], defined in the xtsc_memory base class).  This can be done, for
 * example, using an xtsc_router at the output and an xtsc_arbiter on the input.
 *
 * See xtsc_dma_request and xtsc_dma_descriptor for information on programming a DMA
 * request.
 *
 * Here is a block diagram of an xtsc_dma_engine as it is used in the xtsc_dma_engine
 * example:
 * @image html  Example_xtsc_dma_engine.jpg
 * @image latex Example_xtsc_dma_engine.eps "xtsc_dma_engine Example" width=10cm
 *
 * @see xtsc_dma_engine_parms
 * @see xtsc_memory
 * @see xtsc_dma_request
 * @see xtsc_dma_descriptor
 * @see xtsc_arbiter
 * @see xtsc_mmio
 * @see xtsc_router
 */
class XTSC_COMP_API xtsc_dma_engine : public xtsc_memory {
public:

  sc_core::sc_port  <xtsc::xtsc_request_if>     m_request_port;         ///< Bind to single slave
  sc_core::sc_export<xtsc::xtsc_respond_if>     m_respond_export;       ///< Single slave binds to this


  typedef xtsc::xtsc_request xtsc_request;

  // SystemC needs this.
  SC_HAS_PROCESS(xtsc_dma_engine);


  /// Our C++ type (SystemC uses this)
  virtual const char* kind() const { return "xtsc_dma_engine"; }


  /**
   * Constructor.
   *
   * @param   module_name     The SystemC module name.
   * @param   dma_parms       The xtsc_dma_engine and xtsc_memory contruction parameters.
   *
   * @see xtsc_dma_engine_parms
   * @see xtsc_memory_parms
   */
  xtsc_dma_engine(sc_core::sc_module_name module_name, const xtsc_dma_engine_parms& dma_parms);


  /// Destructor.
  virtual ~xtsc_dma_engine(void);


  /// For xtsc_connection_interface
  virtual xtsc::u32 get_bit_width(const std::string& port_name, xtsc::u32 interface_num = 0) const;


  /// For xtsc_connection_interface
  virtual sc_core::sc_object *get_port(const std::string& port_name);


  /// For xtsc_connection_interface
  virtual std::string get_default_port_name() const { return "master_port"; }


  /// For xtsc_connection_interface
  virtual xtsc::xtsc_port_table get_port_table(const std::string& port_table_name) const;


  /// Reset the device
  void reset(bool hard_reset = false);


  /// Dump descriptor @idx to the specified ostream object
  void dump_descriptor(xtsc::u32 idx, std::ostream& os = std::cout);


  /// Dump count (max 255) descriptors starting at start_idx (wrap after 255 to 1) to the specified ostream object
  void dump_descriptors(xtsc::u32 start_idx = 1, xtsc::u32 count = 1, std::ostream& os = std::cout);


  /**
   * Implementation of the xtsc::xtsc_command_handler_interface.
   *
   * This implementation supports the following commands:
   *  \verbatim
        dump_descriptor <Index>
          Call xtsc_dma_engine::dump_descriptor(<Index>).

        dump_descriptors [<Start> [<Count>]]
          Call xtsc_dma_engine::dump_descriptors(<Start>, <Count>).  Wrap after 255 to 1.

      \endverbatim
   */
  void execute(const std::string&               cmd_line,
               const std::vector<std::string>&  words,
               const std::vector<std::string>&  words_lc,
               std::ostream&                    result);


  /**
   * Set whether or not nb_peek/nb_poke are used to perform the DMA data movement.
   *
   * @param     turbo   If true, then nb_peek/nb_poke will be used to perform the DMA
   *                    data movement.  If false, then READ|WRITE or BLOCK_READ|
   *                    BLOCK_WRITE PIF requests will be used (depending on the
   *                    xtsc_dma_descriptor::num_transfers register);
   */
  void set_turbo(bool turbo) { m_turbo = turbo; }


  /// Return value of m_turbo
  bool get_turbo() const { return m_turbo; }


protected:


  /// We override this method so we can detect writes to the "go" byte (the num_descriptors register)
  virtual void do_write(xtsc::u32 port_num);


  /// We override this method so we can detect writes to the "go" byte (the num_descriptors register)
  virtual void do_block_write(xtsc::u32 port_num);


  /// DMA engine thread
  void dma_thread();


  /// Send read/write requests
  void request_thread();


  /// Move write requests from m_ready_write_deque to m_request_deque with delays to honor "max_writes"
  void write_thread();


  /// Synchronize to this device's clock edge
  void sync_to_posedge(bool always_wait);


  /// Check if the "go" byte was written (LSB of num_descriptors register located at "reg_base_address")
  void check_for_go_byte(xtsc::xtsc_address address8, xtsc::u32 size8, const xtsc::u8 *buffer);


  /// Do descriptor using turbo (nb_peek/nb_poke)
  void use_turbo(xtsc_dma_descriptor &dsc);


  /// Do descriptor @idx using overlapped requests
  void do_overlapped_requests(xtsc_dma_descriptor &dsc, xtsc::u32 idx);


  /// Do descriptor using non-overlapped, non-block READ/WRITE requests
  void use_single_transfers(xtsc_dma_descriptor &dsc);


  /// Do descriptor using non-overlapped BLOCK_READ/BLOCK_WRITE requests
  void use_block_transfers(xtsc_dma_descriptor &dsc);


  /// Send out a READ request
  xtsc::xtsc_response::status_t remote_read(xtsc::xtsc_address          address8,
                                            xtsc::u32                   size8,
                                            xtsc::xtsc_byte_enables     byte_enables,
                                            xtsc::u8                   *buffer);


  /// Send out a WRITE request
  xtsc::xtsc_response::status_t remote_write(xtsc::xtsc_address         address8,
                                             xtsc::u32                  size8,
                                             xtsc::xtsc_byte_enables    byte_enables,
                                             xtsc::u8                  *buffer,
                                             xtsc::u64                  tag = 0);


  /// Send out a WRITE request of 4 bytes representing a 32-bit value (consider endianess)
  xtsc::xtsc_response::status_t remote_write_u32(xtsc::xtsc_address address8, xtsc::u32 data);


  /// Common method to compute/re-compute time delays
  virtual void compute_delays();


  /// Get a new xtsc_request (from the pool)
  xtsc_request *new_request();


  /// Delete an xtsc_request (return it to the pool)
  void delete_request(xtsc_request*& p_request);


  /// Implementation of xtsc_respond_if.
  class xtsc_respond_if_impl : public xtsc::xtsc_respond_if, public sc_core::sc_object  {
  public:

    /// Constructor
    xtsc_respond_if_impl(const char *object_name, xtsc_dma_engine& dma) :
      sc_object (object_name),
      m_dma     (dma),
      m_p_port  (0)
    {}

    /// Used by downstream slaves to respond to our PIF requests
    /// @see xtsc::xtsc_respond_if
    bool nb_respond(const xtsc::xtsc_response& response);


  protected:

    /// SystemC callback when something binds to us
    virtual void register_port(sc_core::sc_port_base& port, const char *if_typename);

    xtsc_dma_engine&            m_dma;          ///< Our xtsc_dma_engine object
    sc_core::sc_port_base      *m_p_port;       ///< Port that is bound to us
  };



  xtsc_respond_if_impl          m_respond_impl;                         ///< m_respond_export binds to this
  xtsc_request                  m_request;                              ///< For sending non-overlapped DMA requests
  std::vector<xtsc_request*>    m_request_pool;                         ///< Maintain a pool of requests to improve performance
  xtsc::u32                     m_reg_base_address;                     ///< DMA registers base address ("reg_base_address" parameter)
  xtsc::u32                     m_max_reads;                            ///< See parameter "max_reads"
  xtsc::u32                     m_max_writes;                           ///< See parameter "max_writes"
  xtsc::u32                     m_descriptor_delay;                     ///< See parameter "descriptor_delay"
  xtsc::u8                      m_read_priority;                        ///< See parameter "read_priority"
  xtsc::u8                      m_write_priority;                       ///< See parameter "write_priority"
  bool                          m_overlap_descriptors;                  ///< See parameter "overlap_descriptors"
  bool                          m_clear_notify_value;                   ///< See parameter "clear_notify_value"
  bool                          m_reuse_tag;                            ///< See parameter "reuse_tag"
  bool                          m_allow_size_zero;                      ///< See parameter "allow_size_zero"
  bool                          m_start_at_index_1;                     ///< See parameter "start_at_index_1"
  bool                          m_turbo;                                ///< See parameter "turbo" and method set_turbo()
  bool                          m_big_endian;                           ///< Determined by write to "go" byte

  xtsc::u64                     m_clock_period_value;                   ///< This device's clock period as u64
  sc_core::sc_time              m_time_resolution;                      ///< The SystemC time resolution
  bool                          m_has_posedge_offset;                   ///< True if m_posedge_offset is non-zero
  sc_core::sc_time              m_posedge_offset;                       ///< From "posedge_offset" parameter
  sc_core::sc_time              m_posedge_offset_plus_one;              ///< m_posedge_offset plus m_clock_period

  xtsc::u8                      m_data[xtsc::xtsc_max_bus_width8*16];   ///< Buffer read rsp data before sending out write req
  bool                          m_busy;                                 ///< True if a DMA is in progress
  sc_core::sc_event             m_dma_thread_event;                     ///< Used to notify dma_thread that "go" byte was written
  sc_core::sc_event             m_request_thread_event;                 ///< Used to notify request_thread that an xtsc_request is ready to send out
  sc_core::sc_event             m_write_thread_event;                   ///< Used to notify write_thread
  sc_core::sc_event             m_got_read_response_event;              ///< Notified when a read response is received
  sc_core::sc_event             m_no_reads_or_writes_event;             ///< Notified when total reads and writes changes to 0
  sc_core::sc_event             m_single_response_available_event;      ///< Notified when READ or WRITE rsp is received (id=0x2|0x3)
  sc_core::sc_event             m_block_read_response_available_event;  ///< Notified when a BLOCK_READ rsp is received (id=0x4)
  sc_core::sc_event             m_block_write_response_available_event; ///< Notified when a BLOCK_WRITE rsp is received (id=0x5)

  bool                          m_overlap_read_write;                   ///< True if "max_reads" and "max_writes" are non-zero, else false.
  xtsc::u32                     m_num_reads;                            ///< Number of outstanding read transactions
  xtsc::u32                     m_num_writes;                           ///< Number of outstanding write transactions
  std::deque<xtsc_request*>     m_request_deque;                        ///< deque of overlapped read/write requests for request_thread
  std::deque<xtsc_request*>     m_blank_write_deque;                    ///< deque of pre-formed write request without data
  std::deque<xtsc_request*>     m_ready_write_deque;                    ///< deque of write requests with data, ready to be moved to m_request_deque
  std::deque<xtsc::u32>         m_last_write_deque;                     ///< deque of dummy tokens - one for each last transfer write request
  std::deque<xtsc::u32>         m_done_descriptor_deque;                ///< deque to track when overlapped descriptor complete.  There is one zero
                                                                        ///< entry for each non-last read request of the descriptor.  The entry for the
                                                                        ///< last read request of the descriptor is the descriptor index to be written
                                                                        ///< to the done_descriptor field.
  bool                          m_waiting_for_nacc;                     ///< True if request_thread is waiting for RSP_NACC from downstream
  bool                          m_request_got_nacc;                     ///< True if request_thread request got RSP_NACC 

  const xtsc::xtsc_response    *m_p_single_response;                    ///< Current rsp to READ or WRITE request

  xtsc::u32                     m_num_block_transfers;                  ///< Number of BLOCK_READ responses in currect descriptor
  xtsc::u32                     m_block_read_response_count;            ///< Number of BLOCK_READ responses received so far
  sc_core::sc_time              m_p_block_read_response_time[16];       ///< Time when each BLOCK_READ rsp was received
  const xtsc::xtsc_response    *m_p_block_read_response[16];            ///< Maintain our copy of BLOCK_READ responses

  xtsc::u32                     m_block_write_sent_count;               ///< Number of BLOCK_WRITE requests sent so far
  const xtsc::xtsc_response    *m_p_block_write_response;               ///< Current BLOCK_WRITE response

  sc_core::sc_time              m_nacc_wait_time;                       ///< See "nacc_wait_time" in xtsc_dma_engine_parms

  static const xtsc::u8         m_read_id               = 0x2;          ///< xtsc_request::m_id for READ
  static const xtsc::u8         m_write_id              = 0x3;          ///< xtsc_request::m_id for WRITE
  static const xtsc::u8         m_block_read_id         = 0x4;          ///< xtsc_request::m_id for BLOCK_READ
  static const xtsc::u8         m_block_write_id        = 0x5;          ///< xtsc_request::m_id for BLOCK_WRITE
  static const xtsc::u8         m_overlapped_read_id    = 0x6;          ///< xtsc_request::m_id for overlapped READ/BLOCK_READ
  static const xtsc::u8         m_overlapped_write_id   = 0x7;          ///< xtsc_request::m_id for overlapped WRITE/BLOCK_WRITE

};



}  // namespace xtsc_component



#endif  // _XTSC_DMA_ENGINE_H_
