#ifndef _XTSC_MEMORY_TLM2_H_
#define _XTSC_MEMORY_TLM2_H_

// Copyright (c) 2006-2018 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
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

#include <tlm.h>
#if defined(MTI_SYSTEMC)
#include <peq_with_get.h>
#else
#include <tlm_utils/peq_with_get.h>
#endif
#include <xtsc/xtsc_parms.h>
#include <xtsc/xtsc_memory_b.h>
#include <cstring>
#include <vector>



namespace xtsc {
// Forward references
class xtsc_core;
};



namespace xtsc_component {


// Forward references
class xtsc_master_tlm2;
class xtsc_xttlm2tlm2_transactor;


/**
 * Constructor parameters for a xtsc_memory_tlm2 object.
 *
 * This class contains the constructor parameters for an xtsc_memory_tlm2 object.
 *
 * Note: AXI is used to indicate the AXI4 system memory interfaces of Xtensa for Load,
 *       Store, and iFetch (does not include iDMA).
 *
 *  \verbatim
   Name                   Type  Description
   ------------------     ----  -------------------------------------------------------

   "num_ports"            u32   The number of slave port pairs this memory has.  This
                                defines how many memory interface master devices will be
                                connected with this memory.
                                Default = 1.
                                Minimum = 1.

   "byte_width"           u32   Memory data interface width in bytes.  Valid values are
                                4, 8, 16, 32, and 64.
  
   "start_byte_address"   u64   The starting byte address of this memory in the full
                                address space.
                                Default = 0x00000000.
  
   "memory_byte_size"     u64   The byte size of this memory.  0 means the memory
                                occupies all of the address space at and above
                                "start_byte_address" to the next 4GB boundary.
                                Default = 0.
  
   "clock_period"         u32   This is the length of this memory's clock period
                                expressed in terms of the SystemC time resolution
                                (from sc_get_time_resolution()).  A value of 
                                0xFFFFFFFF means to use the XTSC system clock 
                                period (from xtsc_get_system_clock_period()).
                                A value of 0 means one delta cycle.
                                Default = 0xFFFFFFFF (i.e. use the system clock 
                                period).

   "check_alignment"      bool  If true, requests will get tlm_response_status of
                                TLM_ADDRESS_ERROR_RESPONSE if their size (data length)
                                is less then the bus width and either the size is not a
                                power of 2 or the address is not size-aligned.
                                In addition, requests whose size is greater then the bus
                                width but is not a multiple of the bus width will get
                                tlm_response_status of TLM_BURST_ERROR_RESPONSE.
                                If false, these checks are not performed.  
                                Note: This parameter should be set to false when not
                                modelling a PIF or AXI bus.
                                Default = true (check alignment).

  "check_page_boundary"   bool  If true, requests will get tlm_response_status of
                                TLM_BURST_ERROR_RESPONSE if their size (data length)
                                is crossing the page boundary. If false, these checks 
                                are not performed. The addresses beyond page boundary
                                will not be updated for a write transaction and in case 
                                of read transaction the data for full transfer size is 
                                returned but will not be correct for the addresses that 
                                are beyond the page boundary.
                                Default = true (ie check page_boundary).

   Note: A TLM2 transaction is considered a burst transfer transaction if the generic
         payload data length attribute is greater then the BUSWIDTH template parameter
         of the socket.  The number of nominal beats in a burst transfer is equal to:
                N = ceil(Payload_Data_Length_Attribute / BUSWIDTH)

   "read_delay"           u32   Specifies the delay for a non-burst read transaction.
                                If "annotate_delay" is true, then this delay is added to
                                the time argument of the b_transport call.
                                If "annotate_delay" is false, a SystemC wait is done
                                for this delay.
                                Default = As specified by delay argument to constructor.

   "burst_read_delay"     u32   Specifies the delay for the nominal first beat of a
                                burst read command.  For a burst read command comprised
                                of N nominal beats, the total delay is calculated as:
                  total_delay = burst_read_delay + burst_read_repeat * (N-1)
                                If "annotate_delay" is true, then total_delay is added
                                to the time argument of the b_transport call.
                                If "annotate_delay" is false, a SystemC wait is done
                                for total_delay.
                                Default = As specified by delay argument to constructor.

   "burst_read_repeat"    u32   Specifies the delay for each of the nominal second
                                through last beat of a burst read command.  See the
                                "burst_read_delay" parameter.
                                Default = 1.

   "write_delay"          u32   Specifies the delay for a non-burst write transaction.
                                If "annotate_delay" is true, then this delay is added to
                                the time argument of the b_transport call.
                                If "annotate_delay" is false, a SystemC wait is done
                                for this delay.
                                Default = As specified by delay argument to constructor.

   "burst_write_delay"    u32   Specifies the delay for the nominal first beat of a
                                burst write command.  For a burst write command comprised
                                of N nominal beats, the total delay is calculated as:
                  total_delay = burst_write_delay + burst_write_repeat * (N-1)
                                If "annotate_delay" is true, then total_delay is added
                                to the time argument of the b_transport call.
                                If "annotate_delay" is false, a SystemC wait is done
                                for total_delay.
                                Default = As specified by delay argument to constructor.

   "burst_write_repeat"   u32   Specifies the delay for each of the nominal second
                                through last beat of a burst write command.  See the
                                "burst_write_delay" parameter.
                                Default = As specified by delay argument to constructor.

   "annotate_delay"       bool  If true, the appropriate delay (as specified by the
                                above delay parameters) is added to the sc_time argument
                                of the b_transport() call and the b_transport()
                                implementation does not call wait().  
                                If false, wait() is called with the appropriate delay
                                (as specified by the above delay parameters) and the
                                sc_time argument of the b_transport() call is not
                                modified.
                                Default = true.

   "nb_transport_delay"   u32   Specifies the delay in terms of this device's clock
                                period to add to the time value of the nb_transport_fw()
                                call.  
                                This parameter is for testing purposes and is ignored if
                                "immediate_timing" is true.
                                Default = 0.

   "test_tlm_accepted"    bool  If true, a separate thread will be used to transition
                                from BEGIN_REQ to END_REQ or BEGIN_RESP (depending on
                                "test_end_req_phase") when nb_transport_fw() is called.
                                This parameter is for testing purposes and is ignored if
                                "immediate_timing" is true.
                                Default = false.

   "test_end_req_phase"   bool  If true, the tlm_accepted_thread will cause an explicit
                                END_REQ phase by calling nb_transport_bw().
                                This parameter is for testing purposes and is ignored if
                                "immediate_timing" is true or "test_tlm_accepted" is
                                false.
                                Default = false.

   "immediate_timing"     bool  If true, the above delays parameters are ignored and
                                the memory model responds to all requests immediately
                                (without any delay--not even a delta cycle).  If false,
                                the above delay parameters are used to determine
                                response timing.
                                Default = false.

   "page_byte_size"       u32   The byte size of a page of memory.  A page of memory in
                                the model is not allocated until it is accessed.  This
                                parameter specifies the page allocation size.
                                Default is 16 Kilobytes (1024*16=16384=0x4000).
                                Minimum page size is 16*byte_width (or 256 if 
                                "byte_width" is 0).
                                Note: "page_byte_size" must be a power of 2.
                                Note: "page_byte_size" should evenly divide
                                "start_byte_address".

   "initial_value_file"   char* If not NULL or empty, this names a text file from which
                                to read the initial memory contents as byte values.
                                Note: "initial_value_file" should not be set when
                                "host_shared_memory" is true.
                                Default = NULL.
                                The text file format is:

                                ([@<Offset>] <Value>*)*

                                1.  Any number (<Offset> or <Value>) can be in decimal
                                    or hexadecimal (using '0x' prefix) format.
                                2.  @<Offset> is added to "start_byte_address".
                                3.  <Value> cannot exceed 255 (0xFF).
                                4.  If a <Value> entry is not immediately preceeded in
                                    the file by an @<Offset> entry, then its offset is
                                    one greater than the preceeding <Value> entry.
                                5.  If the first <Value> entry in the file is not 
                                    preceeded by an @<Offset> entry, then its offset
                                    is zero.
                                6.  Comments, extra whitespace, and blank lines are
                                    ignored.  See xtsc::xtsc_script_file.

                                Example text file contents:

                                   0x01 0x02 0x3    // First three bytes of the memory,
                                                    // 0x01 is at "start_byte_address"
                                   @0x1000 50       // The byte at offset 0x1000 is 50
                                   51 52            // The byte at offset 0x1001 is 51
                                                    // The byte at offset 0x1002 is 52
                             

   "memory_fill_byte"     u32   The low byte specifies the value used to initialize 
                                memory contents at address locations not initialize
                                from "initial_value_file".
                                Note: "memory_fill_byte" does not apply when
                                "host_shared_memory" is true.
                                Default = 0.

   "host_shared_memory"   bool  If true the backing store for this memory will be host
                                OS shared memory (created using shm_open() on Linux and
                                CreateFileMapping() on MS Windows).  If this parameter
                                is true then "memory_fill_byte" and "initial_value_file"
                                do not apply and must be left at their default value.
                                If desired, a Lua script can be used to initialize
                                memory contents.  See "lua_script_file_eoe" in
                                xtsc:xtsc_initialize_parms.
                                Default = false.

   "shared_memory_name"   char* The shared memory name to use when creating host OS
                                shared memory.  If this parameter is left at its default
                                setting of NULL, then the default name will be formed by
                                concatenating the user name, a period, and the module
                                instance hierarchical name.  For example:
                                  MS Windows:     joeuser.myshmem
                                  Linux: /dev/shm/joeuser.myshmem
                                This parameter is ignored if "host_shared_memory" is
                                false.
                                Default = NULL (use default shared memory name)

   "read_only"            bool  If true this memory model represents a ROM which
                                does not support WRITE transactions.  Use
                                "initial_value_file" to initialize the ROM.  The
                                transport_dbg() method may also be used to initialize or
                                change the ROM contents.
                                Default = false.

   "allow_dmi"            bool  If true, this memory will support DMI except to address
                                ranges as specified by the "deny_fast_access" parameter.
                                If false, all DMI requests will be denied.
                                Default = true.

   "deny_fast_access"  vector<u64>   A std::vector containing an even number of
                                addresses.  Each pair of addresses specifies a range of
                                addresses that will be denied fast access.  The first
                                address in each pair is the start address and the second
                                address in each pair is the end address.

    \endverbatim
 *
 * @see xtsc_memory_tlm2
 * @see xtsc::xtsc_parms
 */
class XTSC_COMP_API xtsc_memory_tlm2_parms : public xtsc::xtsc_parms {
public:

  /**
   * Constructor for an xtsc_memory_tlm2_parms object. After the object is constructed, the
   * data members can be directly written using the appropriate xtsc_parms::set() method
   * in cases where non-default values are desired.
   *
   * @param width8              Memory data interface width in bytes.
   *
   * @param delay               Default delay for read and write in terms of this
   *                            memory's clock period (see "clock_period").  Local
   *                            memory devices should use a delay of 0 for a 5-stage
   *                            pipeline and a delay of 1 for a 7-stage pipeline.  PIF
   *                            memory devices should use a delay of 1 or more.
   *
   * @param start_address8      The starting byte address of this memory.
   *
   * @param size8               The byte size of this memory.  0 means the memory
   *                            occupies all of the 4GB address space at and above
   *                            start_address8.
   *
   * @param num_ports           The number of ports this memory has.
   *
   */
  xtsc_memory_tlm2_parms(xtsc::u32   width8            = 4,
                         xtsc::u32   delay             = 0,
                         xtsc::u64   start_address8    = 0,
                         xtsc::u64   size8             = 0,
                         xtsc::u32   num_ports         = 1)
  {
    init(width8, delay, start_address8, size8, num_ports);
  }


  /**
   * Constructor for an xtsc_memory_tlm2_parms object based upon an xtsc_core object and
   * a named memory interface. 
   *
   * This constructor will determine width8, delay, start_address8, size8, and,
   * optionally, num_ports by querying the core object and then pass the values to the
   * init() method.  If memory_interface is a ROM interface, then "read_only" will be be
   * set to true.  In addition, the "clock_period" parameter will be set to match the
   * core's clock period.  For PIF/AXI memories, start_address8 and size8 will both be 0
   * indicating a memory which spans the entire 4 gigabyte address space.  For non-
   * PIF/AXI memories, "check_alignment" will be set to false.  If desired, after the
   * xtsc_memory_tlm2_parms object is constructed, its data members can be changed using
   * the appropriate xtsc_parms::set() method before passing it to the xtsc_memory_tlm2
   * constructor.
   *
   * @param     core               A reference to the xtsc_core object upon which to
   *                               base the xtsc_memory_tlm2_parms.
   *
   * @param     memory_interface  The memory interface name (port name).
   *                              Note:  The core configuration must have the named
   *                              memory interface.
   *
   * @param     delay             Default delay for PIF read and write in terms of this
   *                              memory's clock period (see "clock_period").  PIF
   *                              memory devices should use a delay of 1 or more.  A
   *                              value of 0xFFFFFFFF (the default) means to use a delay
   *                              of 1 if the core has a 5-stage pipeline and a delay of
   *                              2 if the core has a 7-stage pipeline.  This parameter
   *                              is ignored except for PIF memory interfaces.
   *
   * @param     num_ports         The number of ports this memory has.  If 0, the
   *                              default, the number of ports (1 or 2) will be inferred
   *                              thusly: If memory_interface is a LD/ST unit 0 port of
   *                              a dual-ported core interface, and the core is
   *                              dual-ported and has no CBox, and if the 2nd port of
   *                              the core has not been bound, then "num_ports" will be
   *                              2; otherwise, "num_ports" will be 1.
   *
   * @see xtsc::xtsc_core::How_to_do_memory_port_binding for a list of legal
   *      memory_interface values.
   */
  xtsc_memory_tlm2_parms(const xtsc::xtsc_core&  core,
                         const char             *memory_interface,
                         xtsc::u32               delay = 0xFFFFFFFF,
                         xtsc::u32               num_ports = 0);


  /**
   * Do initialization common to both constructors.
   */
  void init(xtsc::u32   width8            = 4,
            xtsc::u32   delay             = 0,
            xtsc::u64   start_address8    = 0,
            xtsc::u64   size8             = 0,
            xtsc::u32   num_ports         = 1)
  {
    std::vector<xtsc::u64> deny_fast_access;
    add("byte_width",           width8);
    add("start_byte_address",   start_address8);
    add("memory_byte_size",     size8);
    add("page_byte_size",       1024*16);
    add("initial_value_file",   (char*)NULL);
    add("memory_fill_byte",     0);
    add("host_shared_memory",   false);
    add("shared_memory_name",   (char*)NULL);
    add("num_ports",            num_ports);
    add("read_delay",           delay);
    add("burst_read_delay",     delay);
    add("burst_read_repeat",    1);
    add("write_delay",          delay);
    add("burst_write_delay",    delay);
    add("burst_write_repeat",   delay);
    add("check_alignment",      true);
    add("check_page_boundary",  true);
    add("read_only",            false);
    add("nb_transport_delay",   0);
    add("test_tlm_accepted",    false);
    add("test_end_req_phase",   false);
    add("immediate_timing",     false);
    add("annotate_delay",       true);
    add("clock_period",         0xFFFFFFFF);
    add("allow_dmi",            true);
    add("deny_fast_access",     deny_fast_access);
  }


  /// Return what kind of xtsc_parms this is (our C++ type)
  virtual const char* kind() const { return "xtsc_memory_tlm2_parms"; }
};




/**
 * A PIF, AXI, XLMI, or local memory which uses OSCI TLM2.
 *
 * Example module implementing a configurable memory which uses OSCI TLM2.
 *
 * On a given port, this memory model always processes transactions in the order they
 * were received.
 *
 * You may use this memory directly or just use the code as a starting place for
 * developing your own memory models.
 *
 * Here is a block diagram of xtsc_memory_tlm2 objects being used in the
 * xtsc_xttlm2tlm2_transactor example:
 * @image html  Example_xtsc_xttlm2tlm2_transactor.jpg
 * @image latex Example_xtsc_xttlm2tlm2_transactor.eps "xtsc_xttlm2tlm2_transactor Example" width=10cm
 *
 * @see xtsc_memory_tlm2_parms
 * @see xtsc::xtsc_memory_b
 * @see xtsc::xtsc_core
 * @see xtsc_xttlm2tlm2_transactor
 * @see xtsc_master_tlm2
 */
class XTSC_COMP_API xtsc_memory_tlm2 : public sc_core::sc_module, public xtsc::xtsc_module, public xtsc::xtsc_command_handler_interface {
public:

  // Shorthand aliases
  typedef tlm::tlm_target_socket< 32> target_socket_4;  ///< target socket with BUSWIDTH =  32 bits ( 4 bytes)
  typedef tlm::tlm_target_socket< 64> target_socket_8;  ///< target socket with BUSWIDTH =  64 bits ( 8 bytes)
  typedef tlm::tlm_target_socket<128> target_socket_16; ///< target socket with BUSWIDTH = 128 bits (16 bytes)
  typedef tlm::tlm_target_socket<256> target_socket_32; ///< target socket with BUSWIDTH = 256 bits (32 bytes)
  typedef tlm::tlm_target_socket<512> target_socket_64; ///< target socket with BUSWIDTH = 512 bits (64 bytes)

  // The system builder (e.g. sc_main) uses one of the following methods to get a target socket for binding

  /// Get a reference to a target socket when configured with a 4-byte data interface
  target_socket_4& get_target_socket_4(xtsc::u32 port_num = 0);

  /// Get a reference to a target socket when configured with a 8-byte data interface
  target_socket_8& get_target_socket_8(xtsc::u32 port_num = 0);

  /// Get a reference to a target socket when configured with a 16-byte data interface
  target_socket_16& get_target_socket_16(xtsc::u32 port_num = 0);

  /// Get a reference to a target socket when configured with a 32-byte data interface
  target_socket_32& get_target_socket_32(xtsc::u32 port_num = 0);

  /// Get a reference to a target socket when configured with a 64-byte data interface
  target_socket_64& get_target_socket_64(xtsc::u32 port_num = 0);


  // SystemC needs this
  SC_HAS_PROCESS(xtsc_memory_tlm2);


  /// Our C++ type (SystemC uses this)
  virtual const char* kind() const { return "xtsc_memory_tlm2"; }


  /**
   * Constructor for an xtsc_memory_tlm2.
   *
   * @param     module_name     Name of the xtsc_memory_tlm2 sc_module.
   *
   * @param     memory_parms    The remaining parameters for construction.
   *
   * @see xtsc_memory_tlm2_parms
   */
  xtsc_memory_tlm2(sc_core::sc_module_name module_name, const xtsc_memory_tlm2_parms& memory_parms);


  /// The destructor.
  virtual ~xtsc_memory_tlm2(void);


  /// For xtsc_connection_interface
  virtual xtsc::u32 get_bit_width(const std::string& port_name, xtsc::u32 interface_num = 0) const;


  /// For xtsc_connection_interface
  virtual sc_core::sc_object *get_port(const std::string& port_name);


  /// For xtsc_connection_interface
  virtual std::string get_default_port_name() const { return "target_sockets"; }


  /// For xtsc_connection_interface
  virtual xtsc::xtsc_port_table get_port_table(const std::string& port_table_name) const;


  /// Get the TextLogger for this component (e.g. to adjust its log level)
  log4xtensa::TextLogger& get_text_logger() { return m_text; }



  /**
   * Non-hardware reads (for example, reads by the debugger).
   * @see xtsc::xtsc_request_if::nb_peek
   */
  void peek(xtsc::xtsc_address address8, xtsc::u32 size8, xtsc::u8 *buffer) { m_p_memory->peek(address8, size8, buffer); }


  /**
   * Non-hardware writes (for example, writes from the debugger).
   * @see xtsc::xtsc_request_if::nb_poke
   */
  void poke(xtsc::xtsc_address address8, xtsc::u32 size8, const xtsc::u8 *buffer) { m_p_memory->poke(address8, size8, buffer); }


  /**
   * This method dumps the specified number of bytes from the memory.  Each
   * line of output is divided into three columnar sections, each of which is
   * optional.  The first section contains an address.  The second section contains
   * a hex dump of some (possibly all) of the data (two hex nibbles and a space for
   * each byte from the memory).  The third section contains an ASCII dump of the 
   * same data.
   *
   * @param       address8                The starting byte address in memory.
   *                                      
   * @param       size8                   The number of bytes of data to dump.
   *
   * @param       os                      The ostream object to which the data is to be
   *                                      dumped.
   *
   * @param       left_to_right           If true, the data is dumped in the order:
   *                                      memory[0], memory[1], ..., memory[bytes_per_line-1].
   *                                      If false, the data is dumped in the order:
   *                                      memory[bytes_per_line-1], memory[bytes_per_line-2], ..., memory[0].
   *
   * @param       bytes_per_line          The number of bytes to dump on each line of output.
   *                                      If bytes_per_line is 0 then all size8 bytes are dumped 
   *                                      on a single line with no newline at the end.  If 
   *                                      bytes_per_line is non-zero, then all lines of output
   *                                      end in newline.
   *
   * @param       show_address            If true, the first columnar section contains an 
   *                                      address printed as an 8-hex-digit number with a 0x 
   *                                      prefix.  If false, the first columnar section is null
   *                                      and takes no space in the output.
   *
   * @param       show_hex_values         If true, the second (middle) columnar section of 
   *                                      hex data values is printed.  If false, the second
   *                                      columnar section is null and takes no space in the
   *                                      output.
   *
   * @param       do_column_heading       If true, print byte position column headings over 
   *                                      the hex values section.  If false, no column headings
   *                                      are printed.  If show_hex_values is false, then the
   *                                      do_column_heading value is ignored and no column
   *                                      headings are printed.
   *
   * @param       show_ascii_values       If true, the third (last) columnar section of ASCII
   *                                      data values is printed (if an ASCII value is a
   *                                      non-printable character a period is printed).  If 
   *                                      show_ascii_values is false, the third columnar
   *                                      section is null and takes no space in the output.
   *
   * @param       adjust_address          If adjust_address is true and address8 modulo 
   *                                      bytes_per_line is not 0, then offset the
   *                                      printed values on the first line of the hex and 
   *                                      ASCII columnar sections and adjust the printed 
   *                                      address so that the printed address modulo 
   *                                      bytes_per_line is always zero.  Otherwize, do not
   *                                      offset the first printed data values and do not
   *                                      adjust the printed address.
   */
  void byte_dump(xtsc::xtsc_address     address8,
                 xtsc::u32              size8,
                 std::ostream&          os                      = std::cout,
                 bool                   left_to_right           = true,
                 xtsc::u32              bytes_per_line          = 16,
                 bool                   show_address            = true,
                 bool                   show_hex_values         = true,
                 bool                   do_column_heading       = true,
                 bool                   show_ascii_values       = true,
                 bool                   adjust_address          = true)
  {
    m_p_memory->byte_dump(address8, size8, os, left_to_right, bytes_per_line, show_address, show_hex_values, do_column_heading,
                           show_ascii_values, adjust_address);
  }


  /**
   * Implementation of the xtsc::xtsc_command_handler_interface.
   *
   * This implementation supports the following commands:
   *  \verbatim
        allow_dmi 0|1
          Set allow_dmi flag to true or false.  Return previous value.

        change_clock_period <ClockPeriodFactor>
          Call xtsc_memory_tlm2::change_clock_period(<ClockPeriodFactor>).  Return previous
          <ClockPeriodFactor> for this device.

        invalidate_direct_mem_ptr <StartAddress> <EndAddress> [<Port>]
          Call xtsc_memory_tlm2::invalidate_direct_mem_ptr(<Port, <StartAddress>,
          <EndAddress>).  Default <Port> is 0.

        nb_transport_delay <NumClockPeriods>
          Set m_nb_transport_delay to (m_clock_period * <NumClockPeriods>).  Return
          previous <NumClockPeriods>.

        peek <StartAddress> <NumBytes>
          Peek <NumBytes> of memory starting at <StartAddress>.

        poke <StartAddress> <NumBytes> <Byte1> <Byte2> . . . <ByteN>
          Poke <NumBytes> (=N) of memory starting at <StartAddress>.

        reset [<Hard>]
          Call xtsc_memory_tlm2::reset(<Hard>).  Where <Hard> is 0|1 (default 0).

        test_end_req_phase 0|1
          Set m_test_end_req_phase to 0|1.  Return previous value of m_test_end_req_phase.

        test_tlm_accepted 0|1
          Set m_test_tlm_accepted to 0|1.  Return previous value of m_test_tlm_accepted.

        tlm_response_status <Status>
          Call tlm_generic_payload::set_response_status(<Status>).
      \endverbatim
   *
   * Note: The nb_transport_delay, test_end_req_phase, and test_tlm_accepted commands
   *       are only available when "immediate_timing" is false.
   */
  void execute(const std::string&               cmd_line,
               const std::vector<std::string>&  words,
               const std::vector<std::string>&  words_lc,
               std::ostream&                    result);


  /**
   * Connect an xtsc_master_tlm2 to this xtsc_memory_tlm2.
   *
   * This method connects the specified xtsc_master_tlm2 to the specified target socket
   * of this xtsc_memory_tlm2.
   *
   * @param     master_tlm2             The xtsc_master_tlm2 to connect to this
   *                                    xtsc_memory_tlm2.
   *
   * @param     target_socket           The target socket of this memory to connect the
   *                                    initiator socket of master_tlm2 to.
   */
  void connect(xtsc_master_tlm2& master_tlm2, xtsc::u32 target_socket);


  /**
   * Connect an xtsc_xttlm2tlm2_transactor transactor to this xtsc_memory_tlm2.
   *
   * This method connects the specified initiator socket(s) of the
   * xtsc_xttlm2tlm2_transactor to specified target socket(s) of this xtsc_memory_tlm2.
   *
   * @param     xttlm2tlm2              The xtsc_xttlm2tlm2_transactor to connect to
   *                                    this xtsc_memory_tlm2.
   *
   * @param     initiator_socket        The xttlm2tlm2 initiator socket to connect to.
   *
   * @param     target_socket           The target socket of this memory to connect the
   *                                    initiator socket of xttlm2tlm2 to.
   *
   * @param     single_connect          If true only one socket of this memory will be
   *                                    connected.  If false, the default, then all
   *                                    contiguous, unconnected socket numbers of this
   *                                    memory starting at target_socket that have a
   *                                    corresponding existing socket in xttlm2tlm2
   *                                    (starting at initiator_socket) will be connected
   *                                    to that corresponding socket in xttlm2tlm2.
   *
   * @returns number of sockets that were connected by this call (1 or more)
   */
  xtsc::u32 connect(xtsc_xttlm2tlm2_transactor& xttlm2tlm2,
                    xtsc::u32                   initiator_socket = 0,
                    xtsc::u32                   target_socket    = 0,
                    bool                        single_connect   = false);


  /**
   * Reset the memory.
   */
  void reset(bool hard_reset = false);


  /**
   * Method to change the clock period.
   *
   * @param     clock_period_factor     Specifies the new length of this device's clock
   *                                    period expressed in terms of the SystemC time
   *                                    resolution (from sc_get_time_resolution()).
   */
  void change_clock_period(xtsc::u32 clock_period_factor);


  /// Helper method to call invalidate_direct_mem_ptr() through the specified port of the appropriate socket
  void invalidate_direct_mem_ptr(xtsc::u32 port, xtsc::u64 start_address, xtsc::u64 end_address);


  /// Convert nb_transport to b_transport (one per port)
  void nb2b_thread(void);


  /// Thread to transition from BEGIN_REQ to END_REQ or BEGIN_RESP for testing purposes (one per port)
  void tlm_accepted_thread(void);


protected:


  //   tlm_fw_nonblocking_transport_if
  //   tlm_blocking_transport_if
  //   tlm_fw_direct_mem_if
  //   tlm_transport_dbg_if
  /// Implementation of tlm_fw_transport_if<>
  class tlm_fw_transport_if_impl : public tlm::tlm_fw_transport_if<>, public sc_core::sc_object {
  public:

    /**
     * Constructor.
     * @param   memory      A reference to the owning xtsc_memory_tlm2 object.
     * @param   port_num    The port number that this object serves.
     */
    tlm_fw_transport_if_impl(const char *object_name, xtsc_memory_tlm2& memory, xtsc::u32 port_num) :
      sc_object         (object_name),
      m_memory          (memory),
      m_p_port          (0),
      m_port_num        (port_num),
      m_busy            (false)
    {}


    virtual tlm::tlm_sync_enum nb_transport_fw   (tlm::tlm_generic_payload& trans, tlm::tlm_phase& phase, sc_core::sc_time& t);
    virtual void               b_transport       (tlm::tlm_generic_payload& trans, sc_core::sc_time& t);
    virtual bool               get_direct_mem_ptr(tlm::tlm_generic_payload& trans, tlm::tlm_dmi& dmi_data);
    virtual xtsc::u32          transport_dbg     (tlm::tlm_generic_payload& trans);


    /// Return true if a port has bound to this implementation
    bool is_connected() { return (m_p_port != 0); }


  protected:

    /// SystemC callback when something binds to us
    virtual void register_port(sc_core::sc_port_base& port, const char *if_typename);

    xtsc_memory_tlm2&           m_memory;       ///<  Our xtsc_memory_tlm2 object
    sc_core::sc_port_base      *m_p_port;       ///<  Port that is bound to us
    xtsc::u32                   m_port_num;     ///<  Our port number
    bool                        m_busy;         ///<  Only allow one request at a time
  };



  /// Common method to compute/re-compute time delays
  virtual void compute_delays();


  /** 
   * Helper function to initialize memory contents.
   *
   * @see xtsc::xtsc_memory_b::load_initial_values
   */
  void load_initial_values() {
    m_p_memory->load_initial_values();
  }


  /// Get the page of memory containing address8 (allocate as needed). 
  xtsc::u32 get_page(xtsc::xtsc_address address8) {
    return m_p_memory->get_page(address8);
  }


  /// Get the page of storage corresponding to the specified address
  xtsc::u32 get_page_id(xtsc::xtsc_address address8) const {
    return m_p_memory->get_page_id(address8);
  }


  /// Get the offset into the page of storage corresponding to the specified address
  xtsc::u32 get_page_offset(xtsc::xtsc_address address8) const {
    return m_p_memory->get_page_offset(address8);
  }


  /// Helper method to read a u8 value (allocate as needed).
  virtual xtsc::u8 read_u8(xtsc::xtsc_address address8) {
    return m_p_memory->read_u8(address8);
  }


  /// Helper method to write a u8 value (allocate as needed).
  virtual void write_u8(xtsc::xtsc_address address8, xtsc::u8 value) {
    m_p_memory->write_u8(address8, value);
  }


  /// Helper method to read a u32 value (allocate as needed).
  virtual xtsc::u32 read_u32(xtsc::xtsc_address address8, bool big_endian = false) {
    return m_p_memory->read_u32(address8, big_endian);
  }


  /// Helper method to write a u32 value (allocate as needed).
  virtual void write_u32(xtsc::xtsc_address address8, xtsc::u32 value, bool big_endian = false) {
    m_p_memory->write_u32(address8, value, false);
  }

  typedef tlm_utils::peq_with_get<tlm::tlm_generic_payload> peq;

  // m_target_sockets_BW (BW = Byte width of data interface = BUSWIDTH/8)
  target_socket_4                     **m_target_sockets_4;             ///<  Target socket(s) for  4-byte interface
  target_socket_8                     **m_target_sockets_8;             ///<  Target socket(s) for  8-byte interface
  target_socket_16                    **m_target_sockets_16;            ///<  Target socket(s) for 16-byte interface
  target_socket_32                    **m_target_sockets_32;            ///<  Target socket(s) for 32-byte interface
  target_socket_64                    **m_target_sockets_64;            ///<  Target socket(s) for 64-byte interface

  xtsc_memory_tlm2_parms                m_memory_parms;                 ///<  Copy of xtsc_memory_tlm2_parms
  xtsc::u32                             m_num_ports;                    ///<  The number of ports this memory has

  tlm_fw_transport_if_impl            **m_tlm_fw_transport_if_impl;     ///<  The m_target_sockets_BW objects bind to these
  xtsc::xtsc_memory_b                  *m_p_memory;                     ///<  The memory itself

  peq                                 **m_nb2b_thread_peq;              ///<  For nb_transport/nb2b_thread (per port)
  xtsc::u32                             m_port_nb2b_thread;             ///<  Used by nb2b_thread to get its port number

  peq                                 **m_tlm_accepted_thread_peq;      ///<  For tlm_accepted_thread (per port)
  xtsc::u32                             m_port_tlm_accepted_thread;     ///<  Used by tlm_accepted_thread to get its port number

  sc_core::sc_time                      m_clock_period;                 ///<  The clock period of this memory

  bool                                  m_immediate_timing;             ///<  See "immediate_timing" parameter
  bool                                  m_check_alignment;              ///<  See "check_alignment" parameter
  bool                                  m_check_page_boundary;          ///<  See "check_page_boundary" parameter
  bool                                  m_annotate_delay;               ///<  See "annotate_delay" parameter

  sc_core::sc_time                      m_read_delay;                   ///<  See "read_delay" parameter
  sc_core::sc_time                      m_burst_read_delay;             ///<  See "burst_read_delay" parameter
  sc_core::sc_time                      m_burst_read_repeat;            ///<  See "burst_read_repeat" parameter
  sc_core::sc_time                      m_write_delay;                  ///<  See "write_delay" parameter
  sc_core::sc_time                      m_burst_write_delay;            ///<  See "burst_write_delay" parameter
  sc_core::sc_time                      m_burst_write_repeat;           ///<  See "burst_write_repeat" parameter

  sc_core::sc_time                      m_nb_transport_delay;           ///<  See "nb_transport_delay" parameter and command
  bool                                  m_test_tlm_accepted;            ///<  See "test_tlm_accepted" parameter and command
  bool                                  m_test_end_req_phase;           ///<  See "test_end_req_phase" parameter and command

  bool                                  m_read_only;                    ///<  See "read_only" parameter

  bool                                  m_host_shared_memory;           ///<  See "host_shared_memory" parameter

  bool                                  m_allow_dmi;                    ///<  See "allow_dmi" parameter and command
  std::vector<xtsc::u64>                m_deny_fast_access;             ///<  For DMI. See "deny_fast_access" paramter

  tlm::tlm_response_status              m_tlm_response_status;          ///<  Normally TLM_OK_RESPONSE, can be changed using tlm_response_status command

  xtsc::xtsc_address                    m_start_address8;               ///<  The starting byte address of this memory.        See "start_byte_address".
  xtsc::xtsc_address                    m_size8;                        ///<  The byte size of this memory.                    See "memory_byte_size".
  xtsc::u32                             m_width8;                       ///<  The byte width of this memory's data interface.  See "byte_width".
  xtsc::xtsc_address                    m_end_address8;                 ///<  The ending byte address of this memory

  std::vector<sc_core::sc_process_handle>
                                        m_process_handles;              ///<  For reset 

  log4xtensa::TextLogger&               m_text;                         ///<  Text logger

};



}  // namespace xtsc_component


#endif  // _XTSC_MEMORY_TLM2_H_
