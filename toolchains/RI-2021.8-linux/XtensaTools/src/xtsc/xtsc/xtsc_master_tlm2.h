#ifndef _XTSC_MASTER_TLM2_H_
#define _XTSC_MASTER_TLM2_H_

// Copyright (c) 2005-2018 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Cadence Design Systems, Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Cadence Design Systems, Inc.

/**
 * @file 
 */


#include <tlm.h>
#include <xtsc/xtsc.h>
#include <xtsc/xtsc_parms.h>
#include <string>
#include <vector>
#include <map>
#include <fstream>




namespace xtsc_component {


/**
 * Constructor parameters for a xtsc_master_tlm2 object.
 *
 * This class contains the constructor parameters for a xtsc_master_tlm2 object.
 *  \verbatim
   Name                 Type    Description
   ------------------   ----    --------------------------------------------------------
  
   "byte_width"          u32    Bus width in bytes.  Valid values are 4, 8, 16, 32, and
                                64.
  
   "script_file"        char*   The file to get transaction, peek, and/or poke requests
                                from.  Each request takes one line in the file.  The
                                supported line formats are:

    WAIT  duration
    SYNC  time
    NOTE  message
    INFO  message

    ENABLES size [byte_enables]
    STREAM streaming_width

    delay STOP

    delay POKE  address size b0 b1 ... bN
    delay WRITE address size b0 b1 ... bN
    delay PEEK  address size 
    delay READ  address size 

                                
                                1.  In the POKE, WRITE, PEEK, and READ lines, the
                                    address and size fields are used to set the address
                                    and data length attributes, respectively, in the
                                    generic payload.
                                2.  A POKE line causes a call to the transport_dbg()
                                    method with a command of TLM_WRITE_COMMAND.  The
                                    fields labelled "b0 b1 ... BN" specify the contents
                                    of the data array in the generic payload.
                                3.  A WRITE line causes a call to the b_transport()
                                    method with a command of TLM_WRITE_COMMAND.  The
                                    fields labelled "b0 b1 ... BN" specify the contents
                                    of the data array in the generic payload.
                                4.  A PEEK line causes a call to the transport_dbg()
                                    method with a command of TLM_READ_COMMAND.
                                5.  A READ line causes a call to the b_transport()
                                    method with a command of TLM_READ_COMMAND.
                                6.  In the POKE and WRITE lines, N = size - 1.
                                7.  delay can be 0 (to mean 1 delta cycle), or "now" to
                                    mean no delta cycle delay, or a positive integer or
                                    floating point number to mean that many clock
                                    periods.
                                8.  The "ENABLES" command sets the byte enables array
                                    that will be used for subsequent READ and WRITE
                                    transactions.  If byte_enables is provided then it
                                    must be a string of exactly size characters each of
                                    which is either 0 or 1.  Each 0 in byte_enables will
                                    cause a 0 byte to be added to the TLM2 generic
                                    payload byte enables array and each 1 in
                                    byte_enables will cause a 0xff byte to be added.  If
                                    byte_enables is not provided then it will default to
                                    size 1's (all bytes enabled).
                                    For example:
                                      enables 4
                                      enables 4 1111    // same as above
                                      enables 7 0111001
                                9.  The "STREAM" command can be used to change the 
                                    streaming width attribute of subsequent reads and
                                    writes.  The special value of 0xFFFFFFFF means to
                                    make the streaming width the same as the data length
                                    attribute (from the size argument of the READ or
                                    WRITE command).  The initial streaming width is as
                                    set by the "streaming_width" parameter.
                               10.  The "delay STOP" command causes simulation to stop
                                    via a call to the sc_stop() method after the
                                    specified delay.
                               11.  The "WAIT duration" command can be used to cause a
                                    wait of the specified duration.  duration can be 0
                                    (to mean 1 delta cycle) or a positive integer or
                                    floating point number to mean that many clock
                                    periods.
                               12.  The "SYNC <time>" command with <time> less than 1.0
                                    can be used to cause a wait to the clock phase
                                    (relative to posedge clock) specified by <time>.
                                    Posedge clock is as specified by "posedge_offset".
                                    For example, "sync 0.5" will cause the minimum wait
                                    necessary to sync to negedge clock.
                                    The "SYNC <time>" command with <time> greater than
                                    or equal to 1.0 can be used to cause a wait until
                                    the specified absolute simulation time.
                               13.  The NOTE and INFO commands can be used to cause
                                    the entire line to be logged at NOTE_LOG_LEVEL
                                    or INFO_LOG_LEVEL, respectively.
                               14.  Integers can appear in decimal or hexadecimal (using
                                    '0x' prefix) format.
                               15.  Words are case insensitive.
                               16.  Comments, extra whitespace, blank lines, and lines
                                    between "#if 0" and "#endif" are ignored.  
                                    See xtsc_script_file for a complete list of
                                    pseudo-preprocessor commands.

   "wraparound"         bool    Specifies what should happen when the end of file
                                (EOF) is reached on "script_file".  When EOF is reached
                                and "wraparound" is true, "script_file" will be reset
                                to the beginning of file and the script will be processed
                                again.  When EOF is reached and "wraparound" is false, 
                                the xtsc_master_tlm2 object will cease issuing requests.
                                Default = false.

   "streaming_width"    u32     Specifies the streaming width attribute or read and
                                write transactions.  The special value of 0xFFFFFFFF
                                means to make the streaming width equal to the data
                                length attribute (from the READ/WRITE command).
                                Default = 0xFFFFFFFF.

   "data_fill_byte"     u32     The low byte specifies the value used to initialize 
                                the data pointer array in each new transaction.
                                Default = 0.

   "clock_period"         u32   This is the length of this master's clock period
                                expressed in terms of the SystemC time resolution (from
                                sc_get_time_resolution()).  A value of 0xFFFFFFFF means
                                to use the XTSC system clock period (from
                                xtsc_get_system_clock_period()).  A value of 0 means one
                                delta cycle.
                                Default = 0xFFFFFFFF (i.e. use the system clock 
                                period).

   "posedge_offset"     u32     This specifies the time at which the first posedge of
                                this device's clock conceptually occurs.  It is
                                expressed in units of the SystemC time resolution and
                                the value implied by it must be strictly less than the
                                value implied by the "clock_period" parameter.  A value
                                of 0xFFFFFFFF means to use the same posedge offset as
                                the system clock (from
                                xtsc_get_system_clock_posedge_offset()).
                                Default = 0xFFFFFFFF.

    \endverbatim
 *
 * @see xtsc_master_tlm2
 * @see xtsc::xtsc_parms
 * @see xtsc::xtsc_request
 * @see xtsc::xtsc_script_file
 */
class XTSC_COMP_API xtsc_master_tlm2_parms : public xtsc::xtsc_parms {
public:

  /**
   * Constructor for an xtsc_master_tlm2_parms object.
   *
   * @param script_file         The file name to read the xtsc::xtsc_request test vectors
   *                            from.
   *
   * @param byte_width          The byte width of the memory interface (BUSWIDTH)
   *
   * @param wraparound          Indicates if script_file should wraparound to the
   *                            beginning of the file after the end of file is
   *                            reached.
   *
   */
  xtsc_master_tlm2_parms(const char *script_file = "", xtsc::u32 width8 = 4, bool wraparound = false) {
    add("script_file",          script_file);
    add("byte_width",           width8);
    add("wraparound",           wraparound);
    add("streaming_width",      0xFFFFFFFF);
    add("data_fill_byte",       0);
    add("clock_period",         0xFFFFFFFF);
    add("posedge_offset",       0xFFFFFFFF);
  }


  /// Return what kind of xtsc_parms this is (our C++ type)
  virtual const char* kind() const { return "xtsc_master_tlm2_parms"; }

};





/**
 * A scripted OSCI TLM2 memory interface master.
 *
 * This XTSC module implements an OSCI TLM2 memory interface master that reads an input
 * file ("script_file") to determine when and what transactions to send to an OSCI TLM2
 * memory interface slave module.
 *
 * This module provides a simple means to deliver OSCI TLM2 generic payload test
 * transactions to an xtsc_memory_tlm2, to an xtsc_tlm22xttlm_transactor, or to any
 * other memory-interface slave component that supports the OSCI TLM2 base protocol.
 *
 * Here is a block diagram of an xtsc_master_tlm2 as it is used in the
 * xtsc_tlm22xttlm_transactor example:
 * @image html  Example_xtsc_tlm22xttlm_transactor.jpg
 * @image latex Example_xtsc_tlm22xttlm_transactor.eps "xtsc_tlm22xttlm_transactor Example" width=10cm
 *
 * @see xtsc_master_tlm2_parms
 * @see xtsc_memory_tlm2
 * @see xtsc_tlm22xttlm_transactor
 */
class XTSC_COMP_API xtsc_master_tlm2 : public sc_core::sc_module, 
                                       public xtsc::xtsc_module, 
                                       public xtsc::xtsc_command_handler_interface, 
                                       public tlm::tlm_mm_interface
{
public:

  // Shorthand aliases
  typedef tlm::tlm_initiator_socket< 32> initiator_socket_4;    ///< initiator socket with BUSWIDTH =  32 bits ( 4 bytes)
  typedef tlm::tlm_initiator_socket< 64> initiator_socket_8;    ///< initiator socket with BUSWIDTH =  64 bits ( 8 bytes)
  typedef tlm::tlm_initiator_socket<128> initiator_socket_16;   ///< initiator socket with BUSWIDTH = 128 bits (16 bytes)
  typedef tlm::tlm_initiator_socket<256> initiator_socket_32;   ///< initiator socket with BUSWIDTH = 256 bits (32 bytes)
  typedef tlm::tlm_initiator_socket<512> initiator_socket_64;   ///< initiator socket with BUSWIDTH = 512 bits (64 bytes)

  // The system builder (e.g. sc_main) uses one of the following methods to get an initiator socket for binding

  /// Get a reference to the initiator socket with a 4-byte data interface
  initiator_socket_4& get_initiator_socket_4();

  /// Get a reference to the initiator socket with a 8-byte data interface
  initiator_socket_8& get_initiator_socket_8();

  /// Get a reference to the initiator socket with a 16-byte data interface
  initiator_socket_16& get_initiator_socket_16();

  /// Get a reference to the initiator socket with a 32-byte data interface
  initiator_socket_32& get_initiator_socket_32();

  /// Get a reference to the initiator socket with a 64-byte data interface
  initiator_socket_64& get_initiator_socket_64();



  // For SystemC
  SC_HAS_PROCESS(xtsc_master_tlm2);


  /// Our C++ type (SystemC uses this)
  virtual const char* kind() const { return "xtsc_master_tlm2"; }


  /**
   * Constructor for an xtsc_master_tlm2.
   * @param     module_name     Name of the xtsc_master_tlm2 sc_module.
   * @param     master_parms    The remaining parameters for construction.
   * @see xtsc_master_tlm2_parms
   */
  xtsc_master_tlm2(sc_core::sc_module_name module_name, const xtsc_master_tlm2_parms& master_parms);


  // Destructor.
  ~xtsc_master_tlm2(void);


  /// For xtsc_connection_interface
  virtual xtsc::u32 get_bit_width(const std::string& port_name, xtsc::u32 interface_num = 0) const;


  /// For xtsc_connection_interface
  virtual sc_core::sc_object *get_port(const std::string& port_name);


  /// For xtsc_connection_interface
  virtual std::string get_default_port_name() const { return "initiator_socket"; }


  /// For xtsc_connection_interface
  virtual xtsc::xtsc_port_table get_port_table(const std::string& port_table_name) const;


  /// Return byte width of data interface (from "byte_width" parameter)
  xtsc::u32 get_byte_width() const { return m_width8; }


  /**
   * Implementation of the xtsc::xtsc_command_handler_interface.
   *
   * This implementation supports the following commands:
   *  \verbatim
        dump_last_read_write 
          Dump the log line from the most recent previous read or write command.

        peek <StartAddress> <NumBytes>
          Use transport_dbg() to peek <NumBytes> of memory starting at <StartAddress>.

        poke <StartAddress> <NumBytes> <Byte1> <Byte2> . . . <ByteN>
          Use transport_dbg() to poke <NumBytes> (=N) of memory starting at <StartAddress>.

        reset [<Hard>]
          Call xtsc_memory_tlm2::reset(<Hard>).  Where <Hard> is 0|1 (default 0).
      \endverbatim
   */
  void execute(const std::string&               cmd_line,
               const std::vector<std::string>&  words,
               const std::vector<std::string>&  words_lc,
               std::ostream&                    result);


  /**
   * Reset the xtsc_master_tlm2.
   */
  void reset(bool hard_reset = false);


  /**
   * Dump log line of last read or write transaction.
   *
   * @param       os            The ostream object to which the log line of the most
   *                            recent previous read or write is to be dumped.
   */
  void dump_last_read_write(std::ostream& os);


  /// Get size8 bytes from buffer and use transport_dbg() to poke them starting at address8
  void do_poke(xtsc::xtsc_address address8, xtsc::u32 size8, xtsc::u8 *buffer);


  /// Use transport_dbg() to peek size8 bytes starting at address8 and put them in buffer
  void do_peek(xtsc::xtsc_address address8, xtsc::u32 size8, xtsc::u8 *buffer);


  /// Common routine to throw an exception for missing/extra arguments
  void throw_missing_or_extra_arguments(bool missing);


  /// Send OSCI TLM2 transactions (from m_script_file) to the downstream memory interface slave
  virtual void script_thread(void);


  /// Get the TextLogger for this component (e.g. to adjust its log level)
  log4xtensa::TextLogger& get_text_logger() { return m_text; }


protected:


  /// Get a u8 array from the pool or create a new one
  xtsc::u8 *new_u8_array(xtsc::u32 size8);


  /// Return a u8 array to the pool
  void delete_u8_array(xtsc::u32 size8, xtsc::u8*& p_array);


  /// Duplicate a u8 array using new_u8_array
  xtsc::u8 *duplicate_u8_array(xtsc::u32 size8, const xtsc::u8 *p_src);


  /// Get a new transaction object from the pool
  tlm::tlm_generic_payload *new_transaction();


  /// For tlm_mm_interface
  virtual void free(tlm::tlm_generic_payload *p_trans);


  /// Perform validation for the get_initiator_socket_BW() methods
  void validate_port_width(xtsc::u32 width8);


  /// Implementation of tlm_bw_transport_if.
  class tlm_bw_transport_if_impl : public tlm::tlm_bw_transport_if<>, public sc_core::sc_object {
  public:

    /// Constructor
    tlm_bw_transport_if_impl(const char *object_name, xtsc_master_tlm2& master) :
      sc_object         (object_name),
      m_master          (master),
      m_p_port          (0)
    {}

    /// Send responses
    virtual tlm::tlm_sync_enum nb_transport_bw(tlm::tlm_generic_payload& trans, tlm::tlm_phase& phase, sc_core::sc_time& time);

    /// Invalidate DMI
    virtual void invalidate_direct_mem_ptr(sc_dt::uint64 start_range, sc_dt::uint64 end_range);


  protected:

    /// SystemC callback when something binds to us
    virtual void register_port(sc_core::sc_port_base& port, const char *if_typename);

    xtsc_master_tlm2&           m_master;       ///<  Our xtsc_master_tlm2 object
    sc_core::sc_port_base      *m_p_port;       ///<  Port that is bound to us
  };



  // m_initiator_socket_BW (BW = Byte width of data interface = BUSWIDTH/8)
  initiator_socket_4                   *m_initiator_socket_4;                   ///<  Initiator socket for  4-byte interface
  initiator_socket_8                   *m_initiator_socket_8;                   ///<  Initiator socket for  8-byte interface
  initiator_socket_16                  *m_initiator_socket_16;                  ///<  Initiator socket for 16-byte interface
  initiator_socket_32                  *m_initiator_socket_32;                  ///<  Initiator socket for 32-byte interface
  initiator_socket_64                  *m_initiator_socket_64;                  ///<  Initiator socket for 64-byte interface

  tlm_bw_transport_if_impl              m_tlm_bw_transport_if_impl;             ///<  m_initiator_socket_BW binds to this

  log4xtensa::TextLogger&               m_text;                                 ///<  TextLogger
  xtsc::u32                             m_width8;                               ///<  The bus width in bytes.  See "byte_width"
  xtsc::u32                             m_streaming_width;                      ///<  From the "streaming_width" parameter or 
                                                                                ///<  STREAM script file cmd
  xtsc::u8                              m_data_fill_byte;                       ///<  From the "data_fill_byte" parameter 
  bool                                  m_wraparound;                           ///<  Should script file wraparound at EOF
  xtsc::xtsc_script_file                m_script_file_stream;                   ///<  The script file object
  std::string                           m_script_file;                          ///<  The name of the script file
  std::string                           m_line;                                 ///<  The current script file line
  xtsc::u32                             m_line_count;                           ///<  The current script file line number
  std::vector<std::string>              m_words;                                ///<  Tokenized words from m_line
  xtsc::u64                             m_clock_period_value;                   ///<  This device's clock period as u64
  sc_core::sc_time                      m_clock_period;                         ///<  This device's clock period
  sc_core::sc_time                      m_time_resolution;                      ///<  The SystemC time resolution
  bool                                  m_has_posedge_offset;                   ///<  True if m_posedge_offset is non-zero
//bool                                  m_use_b_transport;                      ///<  True to use b_transport, false to use nb_transport
  std::string                           m_last_read_write;                      ///<  Log line from last read or write command
  sc_core::sc_time                      m_posedge_offset;                       ///<  From "posedge_offset" parameter
  xtsc::u64                             m_posedge_offset_value;                 ///<  m_posedge_offset as u64
  xtsc::u32                             m_p_byte_enable_length;                 ///<  Current byte enable length.  Initially, 0.
  xtsc::u8                             *m_p_byte_enables;                       ///<  Current byte enables.  From "enables" command.
  std::multimap<xtsc::u32,xtsc::u8*>    m_u8_array_pool;                        ///<  Maintain a pool of u8 arrays: <size,ptr>
  std::vector<tlm::tlm_generic_payload*>
                                        m_transaction_pool;                     ///<  Maintain a pool to improve performance
  sc_core::sc_port_base                *m_p_port;                               ///<  Port that is bound to m_respond_export

  std::vector<sc_core::sc_process_handle>
                                        m_process_handles;                      ///<  For reset 


  /// Common method for calling b_transport() 
  void do_b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay, bool use_tlm2_busy);


  /// Common method for calling transport_dbg() and handling the return count
  void do_transport_dbg(tlm::tlm_generic_payload& trans);


  /// Get the next vector of words from the script_file
  int get_words();


  /// Extract a u32 value (named argument_name) from the word at m_words[index]
  xtsc::u32 get_u32(xtsc::u32 index, const std::string& argument_name);


  /// Extract a u64 value (named argument_name) from the word at m_words[index]
  xtsc::u64 get_u64(xtsc::u32 index, const std::string& argument_name);


  /// Extract a double value (named argument_name) from the word at m_words[index]
  double get_double(xtsc::u32 index, const std::string& argument_name);


  /**
   * Throw an exception if the current command has more parameters than num_expected.
   *
   * @param     num_expected            The number of parameters expected.
   */
  void check_for_too_many_parameters(xtsc::u32 num_expected);


  /**
   * Throw an exception if the current command has fewer parameters than num_expected.
   *
   * @param     num_expected            The number of parameters expected.
   */
  void check_for_too_few_parameters(xtsc::u32 num_expected);


  /// Set p_dst using size8 words starting at m_words[index]
  void set_buffer(xtsc::u32 index, xtsc::u32 size8, xtsc::u8 *p_dst);


};



}  // namespace xtsc_component



#endif  // _XTSC_MASTER_TLM2_H_
