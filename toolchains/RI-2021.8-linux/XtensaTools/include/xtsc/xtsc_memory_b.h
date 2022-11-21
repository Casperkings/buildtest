#ifndef _XTSC_MEMORY_B_H_
#define _XTSC_MEMORY_B_H_

// Copyright (c) 2005-2018 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Cadence Design Systems, Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Cadence Design Systems, Inc.

/**
 * @file 
 */

#include <xtsc/xtsc.h>
#include <cstring>



namespace xtsc {


/**
 * Class for a memory model.
 *
 * This class contains a lot of common code that can be used by either a TLM or
 * pin-level memory model.
 *
 * @see xtsc_memory
 * @see xtsc_memory_pin
 */
class XTSC_API xtsc_memory_b {

typedef log4xtensa::LogLevel   LogLevel;

public:

  /**
   * Constructor for an xtsc_memory_b.
   *
   * @param     name                    The hierarchical name of the sc_module using this
   *                                    memory model.
   *
   * @param     kind                    The kind the sc_module using this memory model.
   *
   * @param     byte_width              See "byte_width" in xtsc_memory_parms
   *
   * @param     start_byte_address      See "start_byte_address" in xtsc_memory_parms.
   *
   * @param     memory_byte_size        See "memory_byte_size" in xtsc_memory_parms.
   *
   * @param     page_byte_size          See "page_byte_size" in xtsc_memory_parms. 
   *                                    Ignored when "host_shared_memory" is true.
   *
   * @param     initial_value_file      See "initial_value_file" in xtsc_memory_parms.
   *                                    Must be NULL when "host_shared_memory" is true.
   *
   * @param     memory_fill_byte        See "memory_fill_byte" in xtsc_memory_parms.
   *                                    Ignored when "host_shared_memory" is true.
   *
   * @param     host_shared_memory      True if host OS shared memory should be used for
   *                                    the backing store of this memory instance.
   *
   * @param     host_mutex              True if a host OS level mutex (xtsc_host_mutex)
   *                                    should be made for the shared memory.  The
   *                                    xtsc_memory_b class will take care of lock and
   *                                    unlock during calls to its poke, write_u8, and
   *                                    write_u32 methods.  The xtsc_memory_b client is
   *                                    responsible for calling lock() and unlock() if
   *                                    it modifies memory directly and for PIF RCW and
   *                                    PIF/AXI exclusive activity.  If this parameter
   *                                    is true, then atomic access addresses should be
   *                                    in Xtensa's uncached space and atomic accesses
   *                                    must always occur in pairs (RCW1/RCW2 pair or
   *                                    exclusive read/write pair).
   * Note:  Host mutex support in XTSC is an experimental feature.
   *
   * @param     shared_memory_name      Name to used for the host shared memory.  If
   *                                    NULL or empty, then the default name will be
   *                                    used.
   *                                      
   * @param     log_level               Specifies the log level to be used for dumping
   *                                    read, write, peek and poke calls.
   *
   */
  xtsc_memory_b(const char          *name,
                const char          *kind,
                xtsc::u32            byte_width,
                xtsc::xtsc_address   start_byte_address,
                xtsc::xtsc_address   memory_byte_size,
                xtsc::u32            page_byte_size,
                const char          *initial_value_file,
                xtsc::u8             memory_fill_byte,
                bool                 host_shared_memory = false,
                bool                 host_mutex         = false,
                const char          *shared_memory_name = NULL,
                LogLevel             log_level          = 10000   /*DEBUG_LOG_LEVEL*/
                );


  /// The destructor.
  virtual ~xtsc_memory_b(void);


  /**
   * Non-hardware reads (for example, reads by the debugger).
   * @see xtsc::xtsc_request_if::nb_peek
   */
  void peek(xtsc::xtsc_address address8, xtsc::u32 size8, xtsc::u8 *buffer);


  /**
   * Non-hardware writes (for example, writes from the debugger).
   * @see xtsc::xtsc_request_if::nb_poke
   */
  void poke(xtsc::xtsc_address address8, xtsc::u32 size8, const xtsc::u8 *buffer);


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
                 bool                   adjust_address          = true);


  /**
   * Method to initialize memory contents.
   *
   * If host_shared_memory was false this method sets the contents of all currently 
   * allocated memory pages (if any) to the value defined by "memory_fill_byte" and then
   * loads the values from "initial_value_file".  If host_shared_memory was true this
   * method returns without doing anything.
   */
  void load_initial_values();


  /// Get the size in bytes of this memory
  xtsc::xtsc_address get_byte_size() { return m_size8; }


  /// Get the page of memory containing address8 (allocate as needed). 
  xtsc::u32 get_page(xtsc::xtsc_address address8);


  /// Get the page of storage corresponding to the specified address
  xtsc::u32 get_page_id(xtsc::xtsc_address address8) const {
    return (u32)(m_host_shared_memory ? 0 : (address8 - m_start_address8) >> m_page_size8_log2);
  }


  /// Get the offset into the page of storage corresponding to the specified address
  xtsc::u32 get_page_offset(xtsc::xtsc_address address8) const {
    return (u32)((address8 - m_start_address8) & m_page_offset_mask);
  }


  /// Return the host OS shared memory name (will be empty unless host_shared_memory was true)
  const std::string& get_shared_memory_name() const { return m_shared_memory_name; }


  /**
   * Lock the memory.  An exception is thrown if either host_shared_memory or host_mutex
   * was false.
   *
   * Note: When host_mutex is true, lock() and unlock() methods are used by
   * xtsc_memory_b clients to lock the shared memory.  For normal writes and pokes, the
   * shared memory should be locked just long enough to perform the modification.  For
   * atomic accesses, the memory has to be locked for the extended period of time of the
   * atomic access.  Specifically, for PIF RCW access the memory should be locked upon
   * receipt of the 1st RCW beat and unlocked upon completion of the 2nd RCW beat.  For
   * a PIF/AXI exclusive access sequence, the memory should be locked upon receipt of
   * the exclusive read and unlocked upon completion of the exclusive write.
   *
   * Warning: System deadlock will probably occur if an atomic access pair is not
   * completed.  For example, if RCW1 is issued without RCW2 (forbidden by PIF protocol)
   * or if an exclusive read is not followed by an exclusive write (allowed by AXI but
   * not supported when using host_mutex).
   */
  void lock(xtsc::xtsc_address address8);


  /**
   * Unlock the memory.  An exception is thrown if either host_shared_memory or
   * host_mutex was false.
   *
   * @see lock()
   */
  void unlock(xtsc::xtsc_address address8);


  /// Helper method to read a u8 value (allocate as needed).
  virtual xtsc::u8 read_u8(xtsc::xtsc_address address8);


  /// Helper method to write a u8 value (allocate as needed).
  virtual void write_u8(xtsc::xtsc_address address8, xtsc::u8 value);


  /// Helper method to read a u16 value (allocate as needed).
  virtual xtsc::u16 read_u16(xtsc::xtsc_address address8, bool big_endian = false);


  /// Helper method to write a u16 value (allocate as needed).
  virtual void write_u16(xtsc::xtsc_address address8, xtsc::u16 value, bool big_endian = false);


  /// Helper method to read a u32 value (allocate as needed).
  virtual xtsc::u32 read_u32(xtsc::xtsc_address address8, bool big_endian = false);


  /// Helper method to write a u32 value (allocate as needed).
  virtual void write_u32(xtsc::xtsc_address address8, xtsc::u32 value, bool big_endian = false);



  std::string                           m_name;                         ///< The hierarchical name of the module using this memory
  std::string                           m_kind;                         ///< The kind of module using this memory
  xtsc::xtsc_address                    m_start_address8;               ///< The starting byte address of this memory
  xtsc::xtsc_address                    m_size8;                        ///< The byte size of this memory
  xtsc::u32                             m_width8;                       ///< The byte width of this memories data interface

  xtsc::xtsc_address                    m_end_address8;                 ///< The ending byte address of this memory
  xtsc::xtsc_address                    m_page_offset_mask;             ///< Mask for getting the page offset mask
  xtsc::u32                             m_num_pages;                    ///< The number of pages in this memory
  xtsc::u8                            **m_page_table;                   ///< The page table for this memory

  std::string                           m_initial_value_file;           ///< The name of the optional file containing initial values
  xtsc::xtsc_script_file               *m_p_initial_value_file;         ///< Pointer to the optional initial value file object

  xtsc::u8                              m_memory_fill_byte;             ///< Uninitialized memory has this value

  log4xtensa::TextLogger&               m_text;                         ///< Text logger
  log4xtensa::BinaryLogger&             m_binary;                       ///< Binary logger
  LogLevel                              m_log_level;                    ///< Log level to dump read/write/peek/poke calls
  bool                                  m_log_data_binary;              ///< True if transaction data should be logged by m_binary

  xtsc::u64                             m_page_size8;                   ///< Memory page size for allocation - must be a power of 2
  xtsc::u32                             m_page_size8_log2;              ///< Log base 2 of memory page size 

  bool                                  m_host_shared_memory;           ///< Create host OS shared memory for each page
  bool                                  m_host_mutex;                   ///< Create and use a xtsc_host_mutex object for this memory
  std::string                           m_shared_memory_name;           ///< Host OS shared memory name
  xtsc::xtsc_host_mutex                *m_p_host_mutex;                 ///< Mutex object for the shared memory
};



}  // namespace xtsc


#endif  // _XTSC_MEMORY_B_H_
