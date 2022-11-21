#ifndef _XTSC_MASTER_H_
#define _XTSC_MASTER_H_

// Copyright (c) 2005-2018 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Cadence Design Systems, Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Cadence Design Systems, Inc.

/**
 * @file 
 */


#include <xtsc/xtsc_parms.h>
#include <xtsc/xtsc_request_if.h>
#include <xtsc/xtsc_respond_if.h>
#include <xtsc/xtsc_request.h>
#include <xtsc/xtsc_response.h>
#include <xtsc/xtsc_wire_write_if.h>
#include <string>
#include <vector>
#include <fstream>
#include <deque>



namespace xtsc {
// Forward references
class xtsc_core;
};



namespace xtsc_component {


class xtsc_wire_logic;
class xtsc_mmio;

/**
 * Constructor parameters for a xtsc_master object.
 *
 * This class contains the constructor parameters for a xtsc_master object.
 *  \verbatim
   Name                 Type    Description
   ------------------   ----    --------------------------------------------------------
  
   "control"            bool    If true, then a 1-bit control input will be created and
                                the "WAIT CONTROL" commands will be enabled in the
                                script file (see "script_file").  The control input can
                                be used to control the xtsc_master device with another
                                device.
                                Default = false.

   "format"             u32     Set the initial format expected in the script file.  The
                                line format specified by this parameter can be changed
                                using the "FORMAT 1|2|3" command in the script file
                                itself.
                                Default 1.

   "script_file"        char*   The file to read the requests from.  Each request takes 
                                one line in the file.  The supported line formats are:

Note: Long lines may be truncated in XTSC_RM.pdf (please consult the html or header file).

    // All formats
    delay POKE        address size b0 b1 ... bN
    delay PEEK        address size 
    delay LOCK lock
    delay RETIRE address
    delay FLUSH
    delay STOP
    WAIT  RESPONSE|RSP|TAG [repeat]
    WAIT  NACC [timeout [repeat]]
    WAIT  duration
    WAIT  CONTROL WRITE|CHANGE|value [count]
    FETCH ON|OFF
    SYNC  time
    NOTE  message
    INFO  message
    VIRTUAL addr_delta
    COHERENT ON|OFF
    DRAM_ATTRIBUTE attr|OFF
    PIF_ATTRIBUTE attr|OFF
    DOMAIN domain|OFF
    XFER_EN ON|OFF
    USER_DATA value|OFF
    BE byte_enables|OFF
    LAST_TRANSFER 0|1|OFF
    EXCLUSIVE ON|OFF
    BURST PIF|FIXED|WRAP|INCR
    CACHE cache|OFF
    PROT prot|OFF
    SNOOP snoop|CaMx_CS|CaMx_CI|CaMx_MI|MUEvict|OFF
    APB ON|OFF
    FORMAT 1|2|3

    // Format 1
    delay READ        address size route_id id priority pc 
    delay WRITE       address size route_id id priority pc byte_enables b0 b1 ... bN
    delay BLOCK_READ  address size route_id id priority pc num_xfers 
    delay BURST_READ  address size route_id id priority pc num_xfers 
    delay BLOCK_WRITE address size route_id id priority pc num_xfers last_xfer first_xfer b0 b1 ... bN
    delay BURST_WRITE address size route_id id priority pc num_xfers xfer_num byte_enables hw_address b0 b1 ... bN
    delay RCW         address size route_id id priority pc num_xfers last_xfer b0 b1 b2 b3
    delay SNOOP       address size route_id id priority pc num_xfers 

    // Format 2  (adds coh to all requests and adds byte_enables to RCW)
    delay READ        address size route_id id priority pc coh
    delay WRITE       address size route_id id priority pc coh byte_enables b0 b1 ... bN
    delay BLOCK_READ  address size route_id id priority pc coh num_xfers 
    delay BURST_READ  address size route_id id priority pc coh num_xfers 
    delay BLOCK_WRITE address size route_id id priority pc coh num_xfers last_xfer first_xfer b0 b1 ... bN
    delay BURST_WRITE address size route_id id priority pc coh num_xfers xfer_num byte_enables hw_address b0 b1 ... bN
    delay RCW         address size route_id id priority pc coh num_xfers last_xfer byte_enables b0 b1 b2 b3
    delay SNOOP       address size route_id id priority pc coh num_xfers 

    // Format 3  (same as Format 2 except for BLOCK_WRITE and SNOOP)
    delay BLOCK_WRITE address size route_id id priority pc coh num_xfers xfer_num byte_enables hw_address b0 b1 ... bN
    delay SNOOP       address size route_id id priority pc snp num_xfers 

                                
                                1.  Each field after a request type (READ|WRITE|RCW|
                                    BLOCK_READ|BLOCK_WRITE|BURST_READ|BURST_WRITE|SNOOP)
                                    corresponds to an argument in the
                                    xtsc::xtsc_request constructor used to create a
                                    request of that type.
                                    Exceptions/Clarifications:
                                    - The first_xfer field is a 1 for the first transfer
                                      of a BLOCK_WRITE sequence and is 0 for all others.
                                    - The last_xfer field is a 1 for the last transfer
                                      of a BLOCK_WRITE sequence and is 0 for all others.
                                    - The coh field is no longer used and can be set to 0.
                                    - The xfer_num field is 1 for the first transfer, 2
                                      for the second transfer, etc.
                                    - In format 3, after construction xfer_num and
                                      hw_address are set via explicit call to
                                      xtsc_request::adjust_block_write() and
                                      byte_enables are set via an explicit call to
                                      xtsc_request::set_byte_enables.
                                    - In format 3, the "delay SNOOP" command is extended
                                      with the snp field to specifiy the snoop transaction
                                      type associated with the snoop request. The snoop
                                      transaction type can be numeric or one of the following
                                      command which will be converted to the following value
                                      for ARSNOOP and AWSNOOP, respectively:
                                      * For ARSNOOP, the commands are 
                                            ReadNoSnoop        = 0
                                            ReadOnce           = 0
                                            ReadShared         = 1
                                            ReadClean          = 2
                                            ReadNotSharedDirty = 3
                                            ReadUnique         = 7
                                            CleanShared        = 8
                                            CleanInvalid       = 9
                                            CleanUnique        = 11
                                            MakeUnique         = 12
                                            MakeInvalid        = 13
                                            DVMComplete        = 14
                                            DVPMessage         = 15
                                      * For AWSNOOP, the commands are 
                                            WriteNoSnoop       = 0
                                            WriteUnique        = 0
                                            WriteLineUnique    = 1
                                            WriteClean         = 2
                                            WriteBack          = 3
                                            Evict              = 4
                                            WriteEvict         = 5
                                2.  A POKE line can be used to cause a call to either
                                    the nb_poke() method or the nb_poke_coherent()
                                    method depending on whether COHERENT is OFF or ON,
                                    respectively.  Each field after POKE specifies the
                                    value to pass to the corresponding argument in the
                                    nb_poke() or nb_poke_coherent() method.  The fields
                                    labelled "b0 b1 ... BN" specify the contents of the
                                    buffer argument.
                                3.  A PEEK line can be used to cause a call to either
                                    the nb_peek() method or the nb_peek_coherent()
                                    method depending on whether COHERENT is OFF or ON,
                                    respectively.  Each field after PEEK specifies the
                                    value to pass to the corresponding argument in the
                                    nb_peek() or nb_peek_coherent() method.
                                4.  Integers can appear in decimal or hexadecimal (using
                                    '0x' prefix) format.
                                5.  N = size - 1.
                                6.  delay can be 0 (to mean 1 delta cycle), or "now" to
                                    mean no delta cycle delay, or a positive integer or
                                    floating point number to mean that many clock
                                    periods.
                                7.  A "FORMAT 1|2|3" line can be used to specify what
                                    format the following lines will be in.  The format
                                    stays the same until another "FORMAT 1|2|3" line is
                                    encountered.  The initial format is as specified by
                                    the "format" parameter which, by default, is 1.
                                8.  The "delay LOCK lock" command causes a call to
                                    nb_lock() with an argument of lock.  lock can be
                                    "true"|"1"|"on" for an argument of true or it can
                                    be "false"|"0"|"off" for an argument of false.  This
                                    is for modelling the DRamnLockm signal of DRAM.
                                9.  The "delay RETIRE address" command causes a call to
                                    nb_load_retired() with an argument of address.  This
                                    is for modelling the DPortnLoadRetiredm signal of
                                    the XLMI.
                               10.  The "delay FLUSH" command causes a call to
                                    nb_retire_flush().  This is for modelling the
                                    DPortnRetireFlushm signal of the XLMI.
                               11.  The "delay STOP" command causes simulation to stop
                                    via a call to the sc_stop() method after the
                                    specified delay.
                               12.  The "WAIT RESPONSE|RSP" command causes a wait until
                                    the next response is received without regard to
                                    which request the response is for.  The "WAIT TAG"
                                    command causes a wait until a response is received
                                    to the most recent previous request.  A response is
                                    deemed to be for the most recent previous request if
                                    they have the same tag, i.e. if response.get_tag()
                                    equals request.get_tag().  The repeat option
                                    specifies how many times the preceeding request
                                    should be repeated if the response is RSP_NACC.  If
                                    the repeat option is not present, the preceeding
                                    request will be repeated until a response other than
                                    RSP_NACC is received.  If no repeat is desired,
                                    specify 0 for the repeat option.
                               13.  The "WAIT NACC" command causes a wait for the
                                    specified timeout_period and then a check is made to
                                    see if an RSP_NACC has been received for the
                                    preceeding request.  A response is deemed to be for
                                    the most recent previous request if they have the
                                    same tag, i.e. if response.get_tag() equals
                                    request.get_tag().  The timeout_period is defined
                                    based on the timeout argument using the formula:
                                       timeout_period = timeout * clock_period
                                    The timeout argument can be a positive integer or
                                    floating point number.  clock_period is based upon
                                    the "clock_period" parameter.  If no timeout
                                    argument is specified, it defaults to 1 (which means
                                    the timeout_period is the same as the xtsc_master's
                                    clock period).  At the end of the specified
                                    timeout_period, if no RSP_NACC has been received for
                                    the preceding request, then the xtsc_master
                                    continues processing the script file.  If an
                                    RSP_NACC has been received then the preceding
                                    request will be repeated subject to the repeat
                                    option.  The repeat option specifies how many times
                                    the preceeding request should be repeated if it has
                                    gotten a RSP_NACC response by the end of the
                                    specified timeout_period.  If the repeat option is
                                    not present, the preceeding request will be repeated
                                    indefinitely as long as it always gets an RSP_NACC
                                    within the specified timeout_period.  If no repeat
                                    is desired, specify 0 for the repeat option.  
                               14.  The "WAIT duration" command can be used to cause a
                                    wait of the specified duration.  duration can be 0
                                    (to mean 1 delta cycle) or a positive integer or
                                    floating point number to mean that many clock
                                    periods.
                               15.  If the "control" parameter was set to true then the
                                    "WAIT CONTROL" command can be used to cause a wait
                                    until the specified activity occurs on the control
                                    input.  WRITE means any write even if its the same
                                    value, CHANGE means a write of a new value, and
                                    value (which can only be 0 or 1) means a write of
                                    the specified value.  An optional count can be
                                    specified to mean the event has to occur count
                                    times.  The default count is 1.  The default event
                                    is WRITE (so "wait control" is the same thing as
                                    "wait control write 1").
                               16.  The "FETCH ON" command causes all subsequent READ
                                    and BLOCK_READ requests to have the xtsc_request
                                    method set_instruction_fetch(true) called.  To turn
                                    this off, use the "FETCH OFF" command.  The initial
                                    setting is "FETCH OFF".
                               17.  The "SYNC <time>" command with <time> less than 1.0
                                    can be used to cause a wait to the clock phase
                                    (relative to posedge clock) specified by <time>.
                                    Posedge clock is as specified by "posedge_offset".
                                    For example, "sync 0.5" will cause the minimum wait
                                    necessary to sync to negedge clock.
                                    The "SYNC <time>" command with <time> greater than
                                    or equal to 1.0 can be used to cause a wait until
                                    the specified absolute simulation time.
                               18.  The NOTE and INFO commands can be used to cause
                                    the entire line to be logged at NOTE_LOG_LEVEL
                                    or INFO_LOG_LEVEL, respectively.
                               19.  The VIRTUAL command can be used to specify an
                                    addr_delta value which is used in calculating the
                                    virtual_address8 argument used in nb_poke_coherent()
                                    and nb_peek_coherent() calls (see the COHERENT
                                    command).
                                    The initial value of addr_delta is 0.
                               20.  The "COHERENT ON" command causes subsequent POKE and
                                    PEEK lines to result in calls to nb_poke_coherent()
                                    and nb_peek_coherent(), respectively, instead of
                                    nb_poke() and nb_peek().  The virtual_address8 value
                                    passed into the nb_poke_coherent() and
                                    nb_peek_coherent() methods is computed by adding the
                                    addr_delta value (see the VIRTUAL command) to the
                                    address value on the POKE or PEEK line.  To return
                                    to using nb_poke() and nb_peek(), use a
                                    "COHERENT OFF" command.
                                    The initial setting is "COHERENT OFF".
                               21.  The "DRAM_ATTRIBUTE attr" command causes the DRAM
                                    attributes to be set to attr in subsequent requests
                                    using:
                                      xtsc_request::set_dram_attribute(attr);
                                    Setting of DRAM attributes can be turned off using
                                    the "DRAM_ATTRIBUTE OFF" command.
                                    The initial setting is "DRAM_ATTRIBUTE OFF".
                               22.  The "PIF_ATTRIBUTE attr" command causes the PIF
                                    attributes to be set to attr in subsequent requests
                                    using:
                                      xtsc_request::set_pif_attribute(attr);
                                    Setting of PIF attributes can be turned off using
                                    the "PIF_ATTRIBUTE OFF" command.  If desired,
                                    ATTRIBUTE may be used as an alias for PIF_ATTRIBUTE.
                                    The initial setting is "PIF_ATTRIBUTE OFF".
                               23.  The "DOMAIN domain" command causes the PIF|AXI
                                    domain to be set to domain in subsequent requests
                                    using one of the following methods.
                                    If BURST is PIF:
                                      xtsc_request::set_pif_req_domain(domain);
                                    If BURST is not PIF:
                                      xtsc_request::set_domain(domain);
                                    Setting of PIF|AXI domain can be turned off using
                                    the "DOMAIN OFF" command.  If desired, PIF_DOMAIN
                                    may be used as an alias for DOMAIN.
                                    The initial setting is "DOMAIN OFF".
                               24.  The "XFER_EN ON" command causes subsequent requests to
                                    have their m_xfer_en flag set via a call to
                                    xtsc_request::set_xfer_en(true).  To return to not
                                    having the m_xfer_en flag set, use a "XFER_EN OFF"
                                    command.
                                    The initial setting is "XFER_EN OFF".
                               25.  The "USER_DATA value" command causes subsequent
                                    requests to have their m_user_data flag set via a
                                    call to xtsc_request::set_user_data((void*)value).
                                    To return to not having the m_user_data flag set,
                                    use a "USER_DATA OFF" command.
                                    The initial setting is "USER_DATA OFF".
                               26.  The "BE byte_enables" command causes subsequent
                                    transactions to have their byte enables overridden
                                    to byte_enables (regardless of any byte enables that
                                    appear on the request type line) via an explicit
                                    call to xtsc_request::set_byte_enables().  The
                                    "BE OFF" command terminates the override call to
                                    xtsc_request::set_byte_enables() for subsequent
                                    transactions.
                                    The initial setting is "BE OFF".
                               27.  The "LAST_TRANSFER 0|1" command causes subsequent
                                    requests to have their normal m_last_transfer flag
                                    overridden via a call to
                                    xtsc_request::set_last_transfer(0|1).  To return to
                                    not having the normal setting overridden, use a
                                    "LAST_TRANSFER OFF" command.
                                    The initial setting is "LAST_TRANSFER OFF".
                               28.  The "EXCLUSIVE ON" command causes subsequent
                                    requests to have their m_exclusive flag set via a
                                    call to xtsc_request::set_exclusive(true).
                                    To return to not having the m_exclusive flag set,
                                    use an "EXCLUSIVE OFF" command.
                                    The initial setting is "EXCLUSIVE OFF".
                               29.  The BURST command can be used with an argument of
                                    FIXED|WRAP|INCR to specify that subsequent requests
                                    are AXI requests.  
                                    The initial setting is "BURST PIF".
                               30.  When BURST is not PIF, the "CACHE cache" command
                                    causes the AxCACHE bits to be set to cache in
                                    subsequent requests using:
                                      xtsc_request::set_cache(cache);
                                    Setting of AxCACHE can be turned off using the
                                    "CACHE OFF" command.
                                    The initial setting is "CACHE OFF".
                               31.  When APB is ON or BURST is not PIF, the "PROT prot"
                                    command causes the PPROT or AxPROT bits to be set to
                                    prot in subsequent requests using:
                                      xtsc_request::set_prot(prot);
                                    Setting of PPROT/AxPROT can be turned off using the
                                    "PROT OFF" command.
                                    The initial setting is "PROT OFF".
                               32.  When BURST is not PIF, the "SNOOP snoop" command
                                    causes the AxSNOOP bits to be set to snoop in
                                    subsequent requests using:
                                      xtsc_request::set_snoop(snoop);
                                    The snoop argument can be numeric or it can be one
                                    of the following special codes which will be
                                    converted to the value shown:
                                        MUEvict = 0x4
                                        CaMx_CS = 0x8
                                        CaMx_CI = 0x9
                                        CaMx_MI = 0xD
                                    Setting of AxSNOOP can be turned off using the
                                    "SNOOP OFF" command.
                                    The initial setting is "SNOOP OFF".
                               33.  The "APB ON" command causes subsequent read and
                                    write requests to have their m_apb flag set true.
                                    To return to not having the m_apb flag set true,
                                    use an "APB OFF" command.
                                    The initial setting is "APB OFF".
                               34.  Words are case insensitive.
                               35.  Comments, extra whitespace, blank lines, and lines
                                    between "#if 0" and "#endif" are ignored.  
                                    See xtsc_script_file for a complete list of
                                    pseudo-preprocessor commands.

   "wraparound"         bool    Specifies what should happen when the end of file
                                (EOF) is reached on "script_file".  When EOF is reached
                                and "wraparound" is true, "script_file" will be reset
                                to the beginning of file and the script will be processed
                                again.  When EOF is reached and "wraparound" is false, 
                                the xtsc_master object will cease issuing requests.
                                Default = false.

   "response_history_depth" u32 The maximum number of entries to maintain in the
                                response history table.  This value may be overridden
                                using the set_response_history_depth() method.  You may
                                wish to maintain a response history table in order to
                                examine responses during program execution using the
                                XTSC command facility and a Lua script/snippet or the
                                XTSC cmd prompt.  See the xtsc_master::execute() method.
                                Default = 0 (no response history is maintained).

   "zero_delay_repeat"  bool    A request line in the script file with a delay of either
                                "now" or 0 followed by a "WAIT RESPONSE|RSP|TAG" line
                                will result in a hung simulation if the downstream slave
                                is busy, requires a non-zero amount of SystemC time to
                                become non-busy, and sends its RSP_NACC without any
                                SystemC delay.  To prevent a potential hung simulation,
                                leave this parameter at its default setting of false to
                                cause a one clock cycle delay to be inserted whenever
                                the downstream slave responds with RSP_NACC at the same
                                SystemC time as a request from a request line with a
                                "now" or 0 delay followed by a "WAIT RESPONSE|RSP|TAG"
                                line.  Set this parameter to true to get the behavior of
                                RD-2011.2 and earlier (which allows a potential hung
                                simulation).
                                Default = false.

   "return_value_file"  char*   The file to read the nb_respond return values from.
                                Each time xtsc_master::nb_respond() is called with a
                                non-RSP_NACC status, another word is read from this
                                file.  If the word is "1", nb_respond returns true.  If
                                the word is "0", nb_respond returns false.  When the end
                                of the file is reached, the file pointer is reset to the
                                beginning of the file.  If "return_value_file" is NULL
                                or empty, then nb_respond always returns true.  Comments
                                and blank lines are ignored.
                                See xtsc::xtsc_script_file.

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
 * @see xtsc_master
 * @see xtsc::xtsc_parms
 * @see xtsc::xtsc_request
 * @see xtsc::xtsc_request_if::nb_request
 * @see xtsc::xtsc_debug_if::nb_poke
 * @see xtsc::xtsc_debug_if::nb_peek
 * @see xtsc::xtsc_debug_if::nb_poke_coherent
 * @see xtsc::xtsc_debug_if::nb_peek_coherent
 * @see xtsc::xtsc_script_file
 */
class XTSC_COMP_API xtsc_master_parms : public xtsc::xtsc_parms {
public:

  /**
   * Constructor for an xtsc_master_parms object.
   *
   * @param script_file         The file name to read the xtsc::xtsc_request test vectors
   *                            from.
   *
   * @param wraparound          Indicates if script_file should wraparound to the
   *                            beginning of the file after the end of file is
   *                            reached.
   *
   * @param return_value_file   The optional file name to read the xtsc::nb_respond
   *                            return values from.  If this argument is NULL or empty,
   *                            then xtsc_master::nb_respond() will always return true. 
   *                            When the end of this file is reached, the file pointer
   *                            is reset to the beginning of the file.
   *
   */
  xtsc_master_parms(const char *script_file = "", bool wraparound = false, const char *return_value_file = NULL) {
    add("control",                false);
    add("script_file",            script_file);
    add("wraparound",             wraparound);
    add("response_history_depth", 0);
    add("zero_delay_repeat",      false);
    add("return_value_file",      return_value_file);
    add("clock_period",           0xFFFFFFFF);
    add("posedge_offset",         0xFFFFFFFF);
    add("format",                 1);
  }


  /// Return what kind of xtsc_parms this is (our C++ type)
  virtual const char* kind() const { return "xtsc_master_parms"; }

};





/**
 * A scripted memory interface master.
 *
 * This XTSC module implements a memory interface master that reads an input file
 * ("script_file") to determine when and what requests to send to a memory interface
 * slave module.  An optional return value file can be named (in the "return_value_file"
 * parameter) from which the return value for calls to nb_respond() will be obtained (if
 * no return value file is provided, then nb_respond() always returns true).
 *
 * This module provides a simple means to deliver xtsc::xtsc_request test transactions
 * to an xtsc_memory, to an xtsc_core (inbound PIF), or even to a system comprised of
 * xtsc_arbiter, xtsc_core, xtsc_dma_engine, xtsc_memory, xtsc_memory_pin, xtsc_mmio,
 * xtsc_router, xtsc_slave, and xtsc_tlm2pin_memory_transactor objects.  
 *
 * To provide an additional degree of feedback or control of the script (beyond what the
 * memory interface slave can exert through memory responses), the "control" option can
 * be set to true and a wire writer such as xtsc_core, xtsc_mmio, or xtsc_wire_logic can
 * be connected to the control input.  This allows the xtsc_master device to better
 * model certain SoC components.  For example, if you are using an xtsc_master to
 * program an xtsc_dma_engine, then the memory-mapped write from the xtsc_dma_engine
 * used to signal "DMA done" can be routed through an xtsc_mmio device and thence to the
 * control input of the xtsc_master to cause it to program the next DMA movement.
 *
 * Here is a block diagram of an xtsc_master as it is used in the master example:
 * @image html  xtsc_master.jpg
 * @image latex xtsc_master.eps "xtsc_master Example" width=10cm
 *
 * @see xtsc_master_parms
 * @see xtsc::xtsc_request_if
 * @see xtsc::xtsc_respond_if
 */
class XTSC_COMP_API xtsc_master :
          public sc_core::sc_module,
          public xtsc::xtsc_module,
          public xtsc::xtsc_command_handler_interface,
  virtual public xtsc::xtsc_respond_if
{
public:

  // See get_control_input() if the "control" parameter was set to true

  /// From us to slave
  sc_core::sc_port  <xtsc::xtsc_request_if>     m_request_port;

  /// From slave to us
  sc_core::sc_export<xtsc::xtsc_respond_if>     m_respond_export;


  // For SystemC
  SC_HAS_PROCESS(xtsc_master);


  /// Our C++ type (SystemC uses this)
  virtual const char* kind() const { return "xtsc_master"; }


  /**
   * Constructor for an xtsc_master.
   * @param     module_name     Name of the xtsc_master sc_module.
   * @param     master_parms    The remaining parameters for construction.
   * @see xtsc_master_parms
   */
  xtsc_master(sc_core::sc_module_name module_name, const xtsc_master_parms& master_parms);


  // Destructor.
  ~xtsc_master(void);


  /// For xtsc_connection_interface
  virtual xtsc::u32 get_bit_width(const std::string& port_name, xtsc::u32 interface_num = 0) const;


  /// For xtsc_connection_interface
  virtual sc_core::sc_object *get_port(const std::string& port_name);


  /// For xtsc_connection_interface
  virtual std::string get_default_port_name() const { return "master_port"; }


  /// For xtsc_connection_interface
  virtual xtsc::xtsc_port_table get_port_table(const std::string& port_table_name) const;


  /**
   * Reset the xtsc_master.
   */
  void reset(bool hard_reset = false);


  /**
   * Dump the response history table.
   *
   * This method dumps the contents of the response history table.  Each response is
   * shown on a separate line starting with the 1-based index of that response in the
   * table.
   *
   * Example output using the XTSC cmd prompt:
   *  \verbatim
      cmd: master dump_response_history
      1: tag=17 pc=0x00000000 RSP_OK*[0x60003000/0/00]: 04 05 06 07 
      2: tag=17 pc=0x00000000 RSP_OK [0x60003000/0/00]: 00 01 02 03 
      3: tag=16 pc=0x00000000 RSP_OK*[0x60003040/0/00]
      4: tag=15 pc=0x00000000 RSP_OK*[0x60003020/0/00]
      \endverbatim
   */
  void dump_response_history(std::ostream &os);


  /**
   * Get nth entry from the response history table.
   *
   * @param     n       Get the nth most recent previous response from the response
   *                    history table.  Use n=1 for the most recent previous response,
   *                    n=2 for the next most recent response, etc.  n must be
   *                    inclusively between 1 and the value returned by
   *                    get_response_history_count().
   * @see get_response_history_count()
   */
  const std::string& get_response(xtsc::u32 n);


  /**
   * Get response count.
   *
   * This method returns the number of entries currently in the response history table.
   * The value returned by this method is guaranteed to be inclusively between 0 and the
   * value set by the most recent previous call to set_response_history_depth().
   *
   * @see set_response_history_depth()
   */
  xtsc::u32 get_response_history_count() const;


  /**
   * Return the response history depth.
   *
   * This method returns the value set by the most recent previous call to
   * set_response_history_depth().
   *
   * @see set_response_history_depth()
   */
  xtsc::u32 get_response_history_depth() const;


  /**
   * Set response history depth.
   *
   * This method sets the maximum number of responses that the response history table
   * may hold.  If there are currently more enteries in the table, the oldest entries
   * are discarded.  Setting depth to 0 will empty the response history table and turn
   * off response history capture until such time as this method is again called with a
   * non-zero value for depth.  The initial response history depth is as set by the
   * "response_history_depth" parameter.
   *
   * @param     depth   The maximum number of entries to maintain in the response
   *                    history table.
   *
   * @see get_response_history_depth()
   */
  void set_response_history_depth(xtsc::u32 depth);


  /**
   * Implementation of the xtsc::xtsc_command_handler_interface.
   *
   * This implementation supports the following commands:
   *  \verbatim
        dump_response_history
          Return the os buffer from calling xtsc_master::dump_response_history(os).

        get_response <N>
          Return value from calling xtsc_master::get_response(<N>).

        get_response_history_count
          Return value from calling xtsc_master::get_response_history_count().

        get_response_history_depth
          Return value from calling xtsc_master::get_response_history_depth().

        set_response_history_depth <Depth>
          Call xtsc_master::set_response_history_depth(<Depth>).
      \endverbatim
   */
  void execute(const std::string&               cmd_line,
               const std::vector<std::string>&  words,
               const std::vector<std::string>&  words_lc,
               std::ostream&                    result);


  /**
   * Return the sc_export of the optional control input.
   *
   * This method may be used for port binding of the optional control input.
   *
   * For example, to bind the TIE export state named "onebit" of an xtsc_core name
   * core0 to the control input of an xtsc_master named master:
   * \verbatim
       core0.get_export_state("onebit")(master.get_control_input());
     \endverbatim
   */
  sc_core::sc_export<xtsc::xtsc_wire_write_if>& get_control_input() const;


  /**
   * Connect an xtsc_wire_logic output to the control input of this xtsc_master.
   *
   * This method connects the specified output of the specified xtsc_wire_logic to the
   * optional control input of this xtsc_master.  This method should not be used unless
   * the "control" parameter was set to true.
   *
   * @param     logic           The xtsc_wire_logic to connect to the control input of
   *                            this xtsc_master.
   *
   * @param     output_name     The output of the xtsc_wire_logic. 
   */
  void connect(xtsc_wire_logic& logic, const char *output_name);


  /**
   * Connect an xtsc_mmio output to the control input of this xtsc_master.
   *
   * This method connects the specified output of the specified xtsc_mmio to the
   * optional control input of this xtsc_master.  This method should not be used unless
   * the "control" parameter was set to true.
   *
   * @param     mmio            The xtsc_mmio to connect to the control input of
   *                            this xtsc_master.
   *
   * @param     output_name     The output of the xtsc_mmio. 
   */
  void connect(xtsc_mmio& mmio, const char *output_name);


  /**
   * Connect to the inbound pif or snoop slave port pair of an xtsc_core.
   *
   * @param     core            The xtsc_core to connect to.
   *
   * @param     port            Either "inbound_pif" or "snoop", case-insensitive.
   */
  void connect(xtsc::xtsc_core& core, const char *port = "inbound_pif");


  /// Send requests (from m_script_file) to the memory interface slave
  virtual void request_thread(void);


  /// Receive responses from the memory interface slave 
  bool nb_respond(const xtsc::xtsc_response& response);


  /// Get the TextLogger for this component (e.g. to adjust its log level)
  log4xtensa::TextLogger& get_text_logger() { return m_text; }


protected:


  /// SystemC callback when something binds to m_respond_export
  virtual void register_port(sc_core::sc_port_base& port, const char *if_typename);


  class xtsc_wire_write_if_impl : public xtsc::xtsc_wire_write_if, public sc_core::sc_object {
  public:

    /// Constructor
    xtsc_wire_write_if_impl(const std::string& port_name, xtsc_master& master) :
      sc_object         (port_name.c_str()),
      m_master          (master),
      m_name            (port_name),
      m_bit_width       (1),
      m_p_port          (0)
    {}

    /// The kind of sc_object we are
    const char* kind() const { return "xtsc_master::xtsc_wire_write_if_impl"; }

    /**
     *  Receive writes from the wire source
     *  @see xtsc::xtsc_wire_write_if
     */
    virtual void nb_write(const sc_dt::sc_unsigned& value);

    /**
     *  Get the wire width in bits.
     *  @see xtsc::xtsc_wire_write_if
     */
    virtual xtsc::u32 nb_get_bit_width() { return m_bit_width; }


  protected:

    /// SystemC callback when something binds to us
    void register_port(sc_core::sc_port_base& port, const char *if_typename);

    xtsc_master&              m_master;         ///< Our xtsc_master object
    std::string               m_name;           ///< Our name as a std::string
    xtsc::u32                 m_bit_width;      ///< Width in bits
    sc_core::sc_port_base    *m_p_port;         ///< Port that is bound to us
  };




  typedef sc_core::sc_export<xtsc::xtsc_wire_write_if>          wire_write_export;

  log4xtensa::TextLogger&               m_text;                                 ///<  TextLogger
  bool                                  m_control;                              ///<  From "control" parameter
  bool                                  m_control_bound;                        ///<  Something is connected to the control input
  wire_write_export                    *m_p_control;                            ///<  Optional control input
  xtsc_wire_write_if_impl              *m_p_write_impl;                         ///<  Implementaion for optional control input
  sc_dt::sc_unsigned                    m_control_value;                        ///<  Current value of the control input
  xtsc::u32                             m_control_write_count;                  ///<  Number of times control input is written
  xtsc::u32                             m_control_change_count;                 ///<  Number of times input is written with a new value
  bool                                  m_wraparound;                           ///<  Should script file wraparound at EOF
  bool                                  m_zero_delay_repeat;                    ///<  See the "zero_delay_repeat" parameter
  std::string                           m_script_file;                          ///<  The name of the script file
  xtsc::xtsc_script_file                m_script_file_stream;                   ///<  The script file object
  std::string                           m_return_value_file;                    ///<  The name of the optional return value file
  xtsc::xtsc_script_file               *m_p_return_value_file;                  ///<  Pointer to the return value file object
  xtsc::u32                             m_format;                               ///<  The format in effect.  See "format" parameter.
  std::string                           m_line;                                 ///<  The current script file line
  xtsc::u32                             m_line_count;                           ///<  The current script file line number
  std::vector<std::string>              m_words;                                ///<  Tokenized words from m_line
  std::vector<std::string>              m_return_values;                        ///<  Tokenized words from return value file
  xtsc::u32                             m_return_value_index;                   ///<  Current word of return value file
  xtsc::u32                             m_return_value_line_count;              ///<  The current return value file line number
  std::string                           m_return_value_line;                    ///<  The current return value file line
  xtsc::u64                             m_block_write_tag;                      ///<  Keep track of tag for BLOCK_WRITE
  xtsc::u64                             m_burst_write_tag;                      ///<  Keep track of tag for BURST_WRITE
  xtsc::u64                             m_rcw_tag;                              ///<  Keep track of tag for RCW
  xtsc::u64                             m_last_request_tag;                     ///<  Tag of most recent previous request sent
  xtsc::xtsc_address                    m_virtual_address_delta;                ///<  Amount to add to get snoop/coherent virtual addr
  xtsc::u32                             m_response_history_depth;               ///<  As initially set by the "response_history_depth" parameter or
                                                                                ///<  overridden by the set_response_history_depth() method.
  std::deque<std::string>               m_response_history;                     ///<  The response history table
  bool                                  m_last_request_got_response;            ///<  Response received with tag of m_last_request_tag
  bool                                  m_last_request_got_nacc;                ///<  RSP_NACC received with tag of m_last_request_tag
  bool                                  m_fetch;                                ///<  For xtsc_request::set_instruction_fetch()
  bool                                  m_use_coherent_peek_poke;               ///<  See "COHERENT ON|OFF" command
  bool                                  m_set_xfer_en;                          ///<  See "XFER_EN ON|OFF" command
  bool                                  m_set_user_data;                        ///<  See "USER_DATA value|OFF" command
  void                                 *m_user_data;                            ///<  See "USER_DATA value|OFF" command
  bool                                  m_set_byte_enables;                     ///<  True if a "BE byte_enables" command is in effect
  bool                                  m_set_last_transfer;                    ///<  See "LAST_TRANSFER OFF" command
  bool                                  m_last_transfer;                        ///<  See "LAST_TRANSFER 0|1" command
  bool                                  m_do_dram_attribute;                    ///<  False when "DRAM_ATTRIBUTE OFF" is in effect
  bool                                  m_do_pif_attribute;                     ///<  False when "PIF_ATTRIBUTE OFF" is in effect
  bool                                  m_do_domain;                            ///<  False when "DOMAIN OFF" is in effect
  bool                                  m_do_cache;                             ///<  False when "CACHE OFF" is in effect
  bool                                  m_do_prot;                              ///<  False when "PROT OFF" is in effect
  bool                                  m_do_snoop;                             ///<  False when "SNOOP OFF" is in effect
  bool                                  m_exclusive;                            ///<  False when "EXCLUSIVE OFF" is in effect
  bool                                  m_apb;                                  ///<  False when "APB OFF" is in effect
  xtsc::xtsc_request::burst_t           m_burst;                                ///<  Value from "BURST PIF|FIXED|WRAP|INCR" command
  sc_dt::sc_unsigned                    m_dram_attribute;                       ///<  Value from "DRAM_ATTRIBUTE attr" command
  xtsc::u32                             m_pif_attribute;                        ///<  Value from "PIF_ATTRIBUTE attr" command
  xtsc::u8                              m_domain;                               ///<  Value from "DOMAIN domain" command
  xtsc::u8                              m_cache;                                ///<  Value from "CACHE cache" command
  xtsc::u8                              m_prot;                                 ///<  Value from "PROT prot" command
  xtsc::u8                              m_snoop;                                ///<  Value from "SNOOP snoop" command
  xtsc::xtsc_byte_enables               m_byte_enables;                         ///<  Byte enables from the "BE byte_enables" command
  static const xtsc::u32                m_buffer_size = 4096;                   ///<  Max size for peeks and poke
  xtsc::u8                              m_buffer[m_buffer_size];                ///<  Buffer for xtsc_request and peek/poke
  xtsc::u64                             m_clock_period_value;                   ///<  This device's clock period as u64
  sc_core::sc_time                      m_clock_period;                         ///<  This device's clock period
  sc_core::sc_time                      m_time_resolution;                      ///<  The SystemC time resolution
  bool                                  m_has_posedge_offset;                   ///<  True if m_posedge_offset is non-zero
  sc_core::sc_time                      m_posedge_offset;                       ///<  From "posedge_offset" parameter
  xtsc::u64                             m_posedge_offset_value;                 ///<  m_posedge_offset as u64
  sc_core::sc_event                     m_control_write_event;                  ///<  Notified when control input is written
  sc_core::sc_event                     m_response_event;                       ///<  Event to notify when a response is received
  xtsc::xtsc_response::status_t         m_last_response_status;                 ///<  Status of last response
  sc_core::sc_port_base                *m_p_port;                               ///<  Port that is bound to m_respond_export

  std::vector<sc_core::sc_process_handle>
                                        m_process_handles;                      ///<  For reset 

  /// Get the next vector of words which define an xtsc::xtsc_request test vector
  int get_words();


  /// Extract a u32 value (named argument_name) from the word at m_words[index]
  xtsc::u32 get_u32(xtsc::u32 index, const std::string& argument_name);


  /// Extract a u64 value (named argument_name) from the word at m_words[index]
  xtsc::u64 get_u64(xtsc::u32 index, const std::string& argument_name);


  /// Extract a double value (named argument_name) from the word at m_words[index]
  double get_double(xtsc::u32 index, const std::string& argument_name);


  /**
   * Throw an exception if the current command has more parameters than number_expected.
   *
   * @param     number_expected         The number of parameters expected.
   */
  void check_for_too_many_parameters(xtsc::u32 number_expected);


  /**
   * Throw an exception if the current command has fewer parameters than number_expected.
   *
   * @param     number_expected         The number of parameters expected.
   */
  void check_for_too_few_parameters(xtsc::u32 number_expected);


  /// Set m_buffer using size8 words starting at m_words[index]
  void set_buffer(xtsc::u32 index, xtsc::u32 size8, bool is_poke = false);


  /// Set the buffer of this request using size8 words starting at m_words[index]
  void set_buffer(xtsc::xtsc_request& request, xtsc::u32 index, xtsc::u32 size8);


  /// Get the next return value for a call to nb_respond
  bool get_return_value();

};



}  // namespace xtsc_component



#endif  // _XTSC_MASTER_H_
