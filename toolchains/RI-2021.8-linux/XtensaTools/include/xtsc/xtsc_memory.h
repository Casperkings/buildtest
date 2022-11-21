#ifndef _XTSC_MEMORY_H_
#define _XTSC_MEMORY_H_

// Copyright (c) 2005-2018 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Cadence Design Systems, Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Cadence Design Systems, Inc.

/**
 * @file 
 */

#include <xtsc/xtsc_request_if.h>
#include <xtsc/xtsc_respond_if.h>
#include <xtsc/xtsc_request.h>
#include <xtsc/xtsc_response.h>
#include <xtsc/xtsc_parms.h>
#include <xtsc/xtsc_memory_b.h>
#include <cstring>
#include <vector>
#include <list>


namespace xtsc {
class xtsc_core;
class xtsc_fast_access_if;
}

namespace xtsc_component {

class xtsc_arbiter;
class xtsc_dma_engine;
class xtsc_master;
class xtsc_memory_trace;
class xtsc_pin2tlm_memory_transactor;
class xtsc_router;


/**
 * Constructor parameters for a xtsc_memory object.
 *
 * This class contains the constructor parameters for an xtsc_memory object.
 *
 * Note: AXI is used to indicate the AXI4 system memory interfaces of Xtensa for Load,
 *       Store, and iFetch (does not include iDMA).
 *
 *  \verbatim
   Name                   Type  Description
   ------------------     ----  -------------------------------------------------------

   "num_ports"            u32   The number of slave port pairs this memory has.  This
                                defines how many memory interface master devices will be
                                connected with this memory.  It is allowed to set this
                                parameter to 0 if no "hardware" memory interfaces are
                                desired (for example, if the xtsc_memory is only going
                                to be accessed using XTSC command facility commands or
                                direct calls to xtsc_memory API's that do not go through
                                an sc_port).
                                Default = 1.
                                Minimum = 0.

   "byte_width"           u32   Memory data interface width in bytes.  Valid values are
                                0, 4, 8, 16, 32, and 64.  A value of 0 indicates that this 
                                memory supports all of the valid data interface widths.
                                This can be useful, for example, to model a memory that 
                                is connected to multiple Xtensa cores that do not all
                                have the same PIF/AXI data interface width.
                                Default = 4.
  
   "start_byte_address"   u64   The starting byte address of this memory in the full
                                address space.
                                Default = 0x00000000.
             
   "memory_byte_size"     u64   The byte size of this memory.  0 means the memory
                                occupies all of the address space at and above
                                "start_byte_address" to the next 4GB boundary.
                                Default = 0.
 
   "secure_address_range" vector<u64>   A std::vector containing an even number of
                                addresses.  Each pair of addresses specifies a range of
                                addresses that will constitute the secure addresses within 
                                this memory. The first address in each pair is the start
                                address and the second address in each pair is the end 
                                address of that range.
                                Default = <empty>                  

   "clock_period"         u32   This is the length of this memory's clock period
                                expressed in terms of the SystemC time resolution
                                (from sc_get_time_resolution()).  A value of 
                                0xFFFFFFFF means to use the XTSC system clock 
                                period (from xtsc_get_system_clock_period()).
                                A value of 0 means one delta cycle.
                                Default = 0xFFFFFFFF (i.e. use the system clock 
                                period).

   "request_fifo_depth"   u32   The request FIFO depth (i.e. how many entries it has).  
                                Note: A request transfer (a beat) takes one entry in the
                                FIFO.  Single WRITE, single READ, BLOCK_READ, and
                                BURST_READ transactions each have a single request
                                transfer and take up only 1 entry in the FIFO.  A
                                BLOCK_WRITE transaction has N=2|4|8|16 request transfers
                                and takes up to N entries in the FIFO (how many entries
                                in the FIFO a given BLOCK_WRITE transaction sequence
                                actually takes depends upon the memory timing parameters
                                and the arrival times of the transfers).  BURST_WRITE
                                transactions work similar to BLOCK_WRITE transactions.  
                                Note: A request transfer that arrives while the request
                                FIFO is full will receive an RSP_NACC.
                                Default = 2.
                                Minimum = 1.

   "write_responses"      bool  If true, an RSP_OK write response will be sent after a
                                WRITE, BLOCK_WRITE, or BURST_WRITE request is received
                                with its last transfer flag set.  If false, write
                                responses of RSP_OK will not be sent.
                                Default = true.

   "check_alignment"      bool  If true, requests whose address is not size-aligned or
                                whose size is not a power of 2 will get an
                                RSP_ADDRESS_ERROR response.  If false, this check is not
                                performed.  
                                Default = false (don't check alignment).

   "delay_from_receipt"   bool  If false, the following delay parameters apply from 
                                the start of processing of the request (i.e. after 
                                all previous requests have been responded to).  This
                                models a memory that can only service one request
                                at a time.  If true, the following delay parameters 
                                apply from the time of receipt of the request.  This
                                more closely models a memory with pipelining; however,
                                requests are always completed (i.e. the response(s) 
                                is/are sent) in the order that their corresponding 
                                requests were received.
                                Default = true.

   "recovery_time"        u32   If "delay_from_receipt" is true, this specifies the 
                                minimum number of clock periods after a response is 
                                sent (except for non-last BLOCK_READ responses) or a 
                                first RCW is handled or a non-last BLOCK_WRITE is 
                                handled before the next response will be sent or the 
                                next BLOCK_WRITE will be handled ("block_read_repeat"
                                is used in lieu of "recovery_time" to specify the
                                delay between consecutive BLOCK_READ responses to
                                the same BLOCK_READ request).  If "delay_from_receipt"
                                is false, this parameter is ignored.  
                                Default = 1.

   "read_delay"           u32   If "delay_from_receipt" is false, the number of clock 
                                periods between starting to process a READ request and 
                                sending xtsc::xtsc_response.  If "delay_from_receipt" is
                                true, the minimum number of clock periods between receipt
                                of a READ request and sending xtsc::xtsc_response.
                                Default = As specified by delay argument to constructor.

   "block_read_delay"     u32   If "delay_from_receipt" is false, the number of clock 
                                periods between starting to process a BLOCK_READ request 
                                and sending the first xtsc::xtsc_response.  If 
                                "delay_from_receipt" is true, the minimum number of
                                clock periods between receipt of a BLOCK_READ request
                                and sending the first xtsc::xtsc_response.
                                Default = As specified by delay argument to constructor.

   "block_read_repeat"    u32   Number of clock periods between each BLOCK_READ response.
                                Default = 1.

   "burst_read_delay"     u32   If "delay_from_receipt" is false, the number of clock 
                                periods between starting to process a BURST_READ request 
                                and sending the first response.  If "delay_from_receipt"
                                is true, the minimum number of clock periods between
                                receipt of a BURST_READ request and sending the first
                                response.
                                Default = As specified by delay argument to constructor.

   "burst_read_repeat"    u32   Number of clock periods between each BURST_READ response.
                                Default = 1.

   "rcw_repeat"           u32   If "delay_from_receipt" is false, the minimum number of 
                                clock periods between starting to process the first RCW 
                                request and starting to process the second RCW request.
                                If "delay_from_receipt" is true, the minimum number of 
                                clock periods between receipt of the first RCW request 
                                and starting to process the second RCW request.
                                Default = 1.

   "rcw_response"         u32   If "delay_from_receipt" is false, the number of clock 
                                periods between starting to process the second RCW 
                                request and sending the xtsc::xtsc_response.  If
                                "delay_from_receipt" is true, the minimum number of 
                                clock periods between receipt of the second RCW 
                                request and sending the xtsc::xtsc_response.
                                Default = As specified by delay argument to constructor.

   "write_delay"          u32   If "delay_from_receipt" is false, the number of clock 
                                periods between starting to process a WRITE request 
                                and sending xtsc::xtsc_response.  If "delay_from_receipt"
                                is true, the minimum number of clock periods between
                                receipt of a WRITE request and sending xtsc::xtsc_response.
                                Default = As specified by delay argument to constructor.

   "block_write_delay"    u32   If "delay_from_receipt" is false, the number of clock
                                periods to process the first BLOCK_WRITE request of a
                                sequence.  If "delay_from_receipt" is true, the minimum
                                number of clock periods between receipt of the first
                                BLOCK_WRITE request and processing it.
                                Default = As specified by delay argument to constructor.

   "block_write_repeat"   u32   If "delay_from_receipt" is false, the minimum number 
                                of clock periods between starting to process a 
                                BLOCK_WRITE request (except the first or last one) 
                                and starting to process the following BLOCK_WRITE 
                                request.  If "delay_from_receipt" is true, the 
                                minimum number of clock periods between receipt of 
                                a BLOCK_WRITE request (except the first or last one) 
                                and starting to process the following BLOCK_WRITE 
                                request.
                                Default = As specified by delay argument to constructor.

   "block_write_response" u32   If "delay_from_receipt" is false, the number of clock 
                                periods between starting to process the last 
                                BLOCK_WRITE request and sending the xtsc::xtsc_response.
                                If "delay_from_receipt" is true, the minimum number
                                of clock periods between receipt of the last 
                                BLOCK_WRITE request and sending the xtsc::xtsc_response.
                                Default = As specified by delay argument to constructor.

   "burst_write_delay"    u32   If "delay_from_receipt" is false, the minimum number 
                                of clock periods between starting to process the first
                                BURST_WRITE request and starting to process the second 
                                BURST_WRITE request.  If "delay_from_receipt" is true,
                                the minimum number of clock periods between receipt
                                of  the first BURST_WRITE request and starting to 
                                process the second BURST_WRITE request.
                                Default = As specified by delay argument to constructor.

   "burst_write_repeat"   u32   If "delay_from_receipt" is false, the minimum number 
                                of clock periods between starting to process a 
                                BURST_WRITE request (except the first or last one) 
                                and starting to process the following BURST_WRITE 
                                request.  If "delay_from_receipt" is true, the 
                                minimum number of clock periods between receipt of 
                                a BURST_WRITE request (except the first or last one) 
                                and starting to process the following BURST_WRITE 
                                request.
                                Default = As specified by delay argument to constructor.

   "burst_write_response" u32   If "delay_from_receipt" is false, the number of clock 
                                periods between starting to process the last BURST_WRITE
                                request and sending the response.  If
                                "delay_from_receipt" is true, the minimum number of
                                clock periods between receipt of the last BURST_WRITE
                                request and sending the response.
                                Default = As specified by delay argument to constructor.

   "response_repeat"      u32   The number of clock periods after a response is sent 
                                and rejected before the response will be resent.  A 
                                value of 0 means one delta cycle.  
                                Default = 1.

   "immediate_timing"     bool  If true, the above delays parameters are ignored and
                                the memory model responds to all requests immediately
                                (without any delay--not even a delta cycle).  If false,
                                the above delay parameters are used to determine
                                response timing.
                                Note: Setting this parameter to true typically will not
                                work on configs that issue BLOCK_READ requests because
                                the ISS does not support getting multiple responses in
                                the same clock period.
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
                                Note: "page_byte_size" does not apply when
                                "host_shared_memory" is true.
                                Default = 16384 (16KB).

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
                                is true then "page_byte_size", "memory_fill_byte", and
                                "initial_value_file" do not apply and should be left at
                                their default value.  If desired, a Lua script can be
                                used to initialize memory contents.  See
                                "lua_script_file_eoe" in xtsc:xtsc_initialize_parms.
                                Default = false.

   "interval_size"        u64   The number of cycles of this instance's clock period
                                that are used to determine the duration of an interval
                                while computing the statistical summary for this 
                                xtsc_memory. The default value of 1000000 when used with 
                                a clock period of 1 ns(default system clock) results in 
                                a default interval duration of 1 ms. 
                                Default = 1000000.

   "shared_memory_name"   char* The shared memory name to use when creating host OS
                                shared memory.  If this parameter is left at its default
                                setting of NULL, then the default name will be formed by
                                concatenating the user name, a period, and the module 
                                instance hierarchical name. For example:
                                  MS Windows:     joeuser.myshmem
                                  Linux: /dev/shm/joeuser.myshmem
                                This parameter is ignored if "host_shared_memory" is
                                false.
                                Default = NULL (use default shared memory name)

   "host_mutex"           bool  If true then the host OS shared memory will
                                have a host OS level mutex which will be locked
                                during PIF RCW and PIF/AXI exclusive sequences and
                                during each poke and write call.  This parameter is
                                ignored if "host_shared_memory" is false.
                                Default = false.

    Caution:  To avoid potential deadlock situations, Cadence recommends that each OS
    process have at most one host OS level mutex.  This means at most one xtsc_memory
    instance with this parameter set true.
    
    Note:  For performance reasons, it is best to have the smallest memory possible
    protected by a host OS level mutex.  If a larger shared memory is required, this can
    be accomplished by modelling the single large shared memory in XTSC as two separate
    xtsc_memory instances, both with shared memory enabled but only one of them with
    "host_mutex" set to true.  All used memory addresses protected by hardware
    synchronization (PIF RCW or PIF/AXI exclusive) should be located within the single
    small mutex-enabled memory.  An xtsc_router with its "immediate_timing" parameter
    set to true can be used to combine the two xtsc_memory instances.

    Note:  Cadence recommends that "allow_fast_access" be set to false whenever
    "host_mutex" is true.
    
    Note:  Host mutex support in XTSC is an experimental feature.

   "log_user_data_bytes"  u32   Number of bytes of user data to log using the
                                xtsc::xtsc_request::get_user_data_for_logging() method.
                                Default = 0.
   
   "read_only"            bool  If true this memory model represents a ROM which
                                does not support WRITE, BLOCK_WRITE, and RCW 
                                transactions.  Use "initial_value_file" to initialize
                                the ROM.  The nb_poke() method may also be used to
                                initialize or change the ROM contents.
                                Default = false.

   "summary"              bool  If true this memory model at the end of simulation
                                displays statistics prominently like throughput, 
                                number of read and write bytes tranferred along with
                                other profiling results. It uses the parameter
                                "interval_size" to compute statistics for user
                                defined intervals within the simulation.
                                Default = false.

   "support_exclusive"    bool  If true this memory model supports exclusive access 
                                requests.  If false it does not support them (it
                                ignores the exclusve access flag in the request).
                                This parameter is effectively ignored if 
                                "exclusive_script_file" is defined.
                                Default = true.

   "use_fast_access"      bool  If true, this memory will support fast access
                                for the turboxim simulation engine.
                                Default = true.

   "deny_fast_access"  vector<u64>   A std::vector containing an even number of
                                addresses.  Each pair of addresses specifies a range of
                                addresses that will be denied fast access.  The first
                                address in each pair is the start address which should
                                always correspond to the start of a bus width and the
                                second address in each pair is the end address which
                                should always correspond to the end of a bus width.  
                                Default = <empty>

   "fast_access_size"  vector<u64>   This parameter is a std::vector containing a
                                multiple of 3 numbers.  It is used to limit fast access
                                granularity of granted blocks for testing purposes.  
                                The first number in each triplet is the start address.
                                The second number in each triplet is the end address.
                                The third number in each triplet is the fast access
                                granularity.  The granularity must be an integer
                                multiple of the bus width as defined by "byte_width".
                                The start address must be an integer multiple of the
                                granularity.  The end address plus 1 must be an integer
                                multiple of the granularity.
                                Default = <empty>

   "use_raw_access"       bool  If "use_fast_access" is true, then when this parameter
                                is true the memory will give direct raw pointer access
                                to the turboxim simulation engine and when this
                                parameter is false, one of callback access, custom
                                callback access, interface access, or peek/poke access
                                will be used depending on the "use_callback_access",
                                "custom_callback_access", and "use_interface_access"
                                parameters.
                                Default = true.

   Note:  To use a fast access method other then raw access, set "use_raw_access" to
   false, and at most one of "use_callback_access", "use_custom_access", and
   "use_interface_access" to true.  If all 4 are false, then normal nb_peek/nb_poke
   through all upstream devices will be used for fast access.  Setting "use_raw_access"
   to false is primarily for testing purposes.

   "use_callback_access"  bool  This parameter is only used if "use_fast_access" is true
                                and "use_raw_access" is false.  In that case, when this
                                parameter is true then callback access will be used
                                (the callback methods internally call the xtsc_memory
                                peek/poke methods but any upstream devices between the
                                originator and xtsc_memory are bypassed) and when this
                                parameter is false, one of custom callback access,
                                interface access, or peek/poke access will be used
                                depending on the "custom_callback_access" and
                                "use_interface_access" parameters.
                                Default = false.
                                Note: Callback access is not supported for use with
                                big endian cores.

   "use_custom_access"    bool  This parameter is only used if "use_fast_access" is true
                                and both "use_raw_access" and "use_callback_access" are
                                false.  In that case, when this parameter is true then
                                custom callback access will be used (the custom callback
                                methods internally call the xtsc_memory peek/poke
                                methods but any upstream devices between the originator
                                and xtsc_memory are bypassed) and when this parameter is
                                false, one of interface access or peek/poke access will
                                be used depending on the "use_interface_access"
                                parameter.
                                Default = false.

   "use_interface_access" bool  This parameter is only used if "use_fast_access" is true
                                and "use_raw_access", "use_callback_access", and
                                "use_custom_access" are all three false.  In that case,
                                if this parameter is false then normal nb_peek/nb_poke
                                through all upstream devices will be used for fast
                                access and if this parameter is true then interface
                                access will be used (which internally calls the
                                peek/poke methods of xtsc_memory but any upstream
                                devices between the originator and xtsc_memory are
                                bypassed).
                                Default = false.

   "exclusive_script_file" char* If not NULL or empty, this names a text file from which
                                exclusive write responses will be read.  Each non-empty, 
                                non-comment line must have the following format (where
                                <value> can be 0 or 1):
                                      <value> [<Address>]
                                Each time an exclusive WRITE request is received a line
                                from the file will be read and used in a call to
                                xtsc_response::set_exclusive(<value>).  If <Address> is
                                specified then it will be checked against the request
                                address and an exception will be thrown if there is a
                                mismatch.  If the end of the file is reached, processing
                                will wrap around to the beginning of the file.  When the
                                first transfer of an exclusive BLOCK_WRITE sequence is
                                received, one line will be read from the file and its
                                content will control all beats of the BLOCK_WRITE
                                sequence.  Each time an exclusive read request is
                                received xtsc_response::set_exclusive(1) will be called
                                and no line will be read from the file.  The file is
                                opened as an xtsc_script_file, so comments, extra
                                whitespace, and blank lines are allowed and ignored.
                                Default = NULL
                                Note: xtsc_memory::support_exclusive() essentially has
                                      no effect when this parameter is defined.

   "script_file"          char* An optional file to read scripted responses from.
                                Normally xtsc_memory, barring an error in a request and
                                barring requests coming in too fast, performs the
                                requested action (i.e. it reads or writes the memory)
                                and responds with an RSP_OK.  This script file allows
                                you to specify other responses.  This can be useful for
                                test scenarios or for causing an RSP_ADDRESS_ERROR
                                response to be sent to an out-of-bound request instead
                                of having an exception thrown.  You cause non-normal
                                responses to be given by specifying either a single
                                address or an address range and the response that goes
                                with that address or address range and optional port
                                number and request type.  You can also specify the time
                                frame in which that response applies and how many times
                                that response is to be given.  Multiple single addresses
                                and multiple address ranges are supported; however, no
                                overlap is allowed.  This script also lets you override
                                the default user data in the response.  Active lines in
                                this file (that is lines that aren't whitespace or
                                comments) fall into one of three categories.
                                  1. Response definition lines
                                  2. Debug/logging lines
                                  3. Timing control lines
                                When a script file is specified, xtsc_memory spawns a
                                SystemC thread to process it.  At the beginning of
                                simulation this thread starts processing each line in
                                the script file in sequence up to the first timing
                                control line (category 3).  When the first timing
                                control line is hit, the thread stops processing the
                                script file and waits for the time or event implied by
                                the timing control line.  When the time or event implied
                                by the first timing control line occurs, the thread
                                resumes processing the script file until the next timing
                                control line, and so on.  A few examples are shown
                                below.
                                The supported line formats organized by category are:

    Category 1: Response Definition Line Formats:
        lowAddr port type status [limit]
        lowAddr-highAddr port type status [limit]
        CLEAR lowAddr 
        CLEAR lowAddr-highAddr 
        CLEAR
        IS_SHARED true|false
        PASS_DIRTY true|false
        USER_DATA length value
        USER_DATA * byte ...
        USER_DATA
        RESPONSE_LIST 0|2 ...
        LAST true|false
        LUA_FUNCTION luaFunction

    Category 2: Debug/Logging Line Formats:
        DUMP  log_level
        NOTE  message
        INFO  message
                                
    Category 3: Timing Control Line Formats:
        SYNC  time
        WAIT  duration
        WAIT  port type match [count]
        delay STOP

                                1.  Integers can appear in decimal or hexadecimal (using
                                    '0x' prefix) format.
                                2.  port can be any non-negative integer less then
                                    "num_ports" or it can be an asterisk to indicate any
                                    port (i.e. port number is a don't care).
                                3.  type can be read|block_read|rcw|write|block_write|
                                    burst_read|burst_write or it can be an asterisk to
                                    indicate any request type (i.e. request type is a
                                    don't care).
                                4.  status can be okay|nacc|address_error|data_error|
                                    address_data_error|response_list.  Normally, when
                                    a request is received that matches a response
                                    definition line the specified response status is
                                    returned for that request; however, a request that
                                    would have gotten RSP_NACC even without a match
                                    occurring will always get an RSP_NACC (regardless of
                                    the response status specified in the response
                                    definition line).  A request that would have gotten
                                    one of the three possible error responses can be
                                    overridden by the script file to get one of the
                                    other two possible error responses or an RSP_NACC
                                    but it cannot be overridden by the script file to
                                    get an RSP_OK.  If status is data_error than the
                                    last transfer flag will be set on the response.  If
                                    status is response_list, than the status will be
                                    be obtained from RESPONSE_LIST (responses for 
                                    BLOCK_READ and BURST_READ requests will be obtained
                                    by cycling through RESPONSE_LIST, the response for
                                    other request types will come from the first entry
                                    in RESPONSE_LIST).  Note: A status of okay is used
                                    to count requests (LAST true) or count beats (LAST
                                    false), it cannot be used to force an RSP_OK
                                    response to a non-last transfer of a BLOCK_WRITE or
                                    RCW request (See Example 3 below).
                                5.  limit specifies how many times the specified special
                                    response status is to be given.  A limit of 0, the
                                    default, means there is no limit.
                                6.  match can be *|hit|miss.  hit means to count the
                                    request if its address is found in the list of
                                    single addresses or in the list of address ranges.
                                    miss means to count the request if its address is 
                                    not in either list.  An asterisk means to count the
                                    request regardless of whether or not its address is
                                    found in one of the lists (i.e. address matching is
                                    a don't care).
                                7.  count must be greater than 0 and defaults to 1.
                                8.  time, duration, and delay can be any non-negative
                                    integer or floating point number to mean that many
                                    clock cycles.
                                9.  You can use an asterisk in lieu of lowAddr-highAddr 
                                    to indicate all addresses in this memory's address
                                    space.
                               10.  The CLEAR commands cause the specified single address
                                    or address range to be removed from the list of
                                    single addresses or address ranges.  CLEAR by itself
                                    clears all single addresses and all address ranges.
                                    Although you can have multiple response definitions
                                    in effect at one time, given an address and a point
                                    in time in the simulation, at most one response
                                    definition can be in effect for that address.
                                    Because of this, the CLEAR command is often used
                                    after one of the timing control lines in order to
                                    remove a response definition for an address before
                                    defining a new response for that address.
                               11.  The "SYNC time" command can be used to cause a wait
                                    until the specified absolute simulation time.
                               12.  The "WAIT duration" command can be used to cause a 
                                    wait for duration clock cycles.
                               13.  The "WAIT port request match [count]" command can be
                                    used to cause a wait until count number of requests
                                    of the specified request type and with the specified
                                    match criteria have been received on the specified
                                    port.
                               14.  The "delay STOP" command will call sc_stop() after
                                    delaying delay cycles.
                               15.  The DUMP command will cause a list of all current
                                    single addresses and address ranges that have a
                                    response status defined to be dumped using the
                                    logging facility.  log_level can be INFO|NOTE.
                               16.  The IS_SHARED command is used to set IsShared bit
                                    (RRESP[3]/CRRESP[3]) in the AXI read or snoop
                                    response channel. The command takes a bool argument
                                    true or false.
                               17.  The PASS_DIRTY command is used to set PassDirty bit
                                    (RRESP[2]/CRRESP[2]) in the AXI read or snoop
                                    response channel. The command takes a bool argument
                                    true or false.
                               18.  The USER_DATA commands can be used to override the
                                    default user data in the response.  The
                                    "USER_DATA length value" command passes value to
                                    method xtsc_response::set_user_data(value).  The
                                    length argument specifies how many bytes are in
                                    value and it must be greater than 0 and not exceed 4
                                    (32-bit simulator) or 8 (64-bit simulator).  The
                                    "USER_DATA * byte ..." command, allocates memory for
                                    the number of bytes in "byte ...", populates that
                                    memory with those bytes, and calls set_user_data()
                                    with a pointer to the allocated memory.  The
                                    "USER_DATA" command with no arguments, returns to
                                    using the default response user data (that is, the
                                    same user data as came in on the request, if any).
                               19.  The RESPONSE_LIST command can be used to specify a
                                    sequence of responses (0=RSP_OK, 2=RSP_DATA_ERROR).
                                    This list is only used for a response definition
                                    line with a status of response_list and a type of
                                    block_read or burst_read.  The initial list contains
                                    16 RSP_DATA_ERROR entries, but these are overwritten
                                    if a RESPONSE_LIST command is encountered.  If there
                                    are fewer entries in the list then required for a
                                    particular request, then the missing entries are
                                    assumed to be RSP_OK.
                               20.  The LUA_FUNCTION command can be used to name a Lua
                                    function to be called when a matching request is
                                    received.  The Lua function must be defined in a
                                    #lua_beg/#lua_end block that appears earlier in the
                                    script file.  The Lua function should have 5
                                    parameters and return a result (which will be logged
                                    with the function call at INFO log level).  The 5
                                    parameters passed to lua_function are:
                                     port - The port number the request was received on.
                                     addr - The address of the request.
                                     type - The string type of the request from
                                            xtsc::xtsc_request::get_type_name().
                                     last - true if last beat of request, else false.
                                     dump - The string output of xtsc_request::dump().
                                    See Example 6 and got_read() below for an example.
                               21.  By default, script_file handling follows the PIF
                                    protocol requirement that a non-RSP_NACC response 
                                    is only sent after a request with the last transfer
                                    flag set.  This default behaviour can be overriden
                                    by defining the environment variable
                                    XTSC_MEMORY_SCRIPT_FILE_LAST_FALSE or by using the
                                    "LAST false" command.  Either of these will allow a
                                    special response to be sent to a request whose last
                                    transfer flag is false (that is, the first request
                                    of an RCW pair of requests or a non-last BLOCK_WRITE
                                    request).
                               22.  The NOTE and INFO commands can be used to cause
                                    the entire line to be logged at NOTE_LOG_LEVEL
                                    or INFO_LOG_LEVEL, respectively.
                               23.  Words are case insensitive.
                               24.  Comments, extra whitespace, blank lines, and lines
                                    between "#if 0" and "#endif" are ignored.  
                                    See xtsc_script_file.

                                Here are some example script file contents for specific
                                purposes (each example is assumed to be in its own
                                script file):

                // Example 1: RSP_NACC the first 3 READ requests to 0x60001708.
                0x60001708 *  read nacc 3

                // Example 2: Accept the first 2 BLOCK_READ requests in the range of
                //            0x60001a00-0x60001aff, then RSP_NACC the next 3 WRITE
                //            requests to any address.
                0x60001a00-0x60001aff * block_read okay
                wait * * hit 2
                clear
                * * write nacc 3

                // Example 3: RSP_NACC the second transfer of the first BLOCK_WRITE 2
                //            times.  Note the use of last and okay to count the beats
                //            of the BLOCK_WRITE.
                last false
                * *  block_write okay 
                wait * * hit 1
                clear
                * *  block_write nacc 2

                // Example 4: Wait for 1000 requests then RSP_NACC the next 2
                wait * * * 1000
                note 1000 requests received; the next two will get RSP_NACC
                * * * nacc 2

                // Example 5: Return RSP_DATA_ERROR for 2nd & 3rd BLOCK_READ response.
                //            The 1st and 4th response will be RSP_OK.  If more than 4
                //            responses are required then they will also be RSP_OK.
                response_list 0 2 2 0
                * *  BLOCK_READ response_list
                wait * * hit 
                clear

                // Example 6: Call a Lua function called got_read() when a READ to
                //            address 0xC0000000 is received.  The Lua function changes
                //            the contents stored at that address to be the number of
                //            times a read has occurred.  When the READ is handled
                //            (by worker_thread) it will see that value in memory and
                //            return it in the xtsc_response.  In this example, the
                //            got_read() Lua function assumes the xtsc_memory instance
                //            name is "sysmem" and ignores the arguments passed to it.
                #lua_beg
                  cnt = 0
                  function got_read(port, addr, type, last, dump)
                    cnt = cnt + 1
                    poke_bytes = xtsc.num_to_poke_bytes(cnt, 4)
                    xtsc.cmd("sysmem poke 0xC0000000 4 " .. poke_bytes)
                    return cnt
                  end
                #lua_end
                lua_function got_read
                0xC0000000 * read okay

   Note:  The implementation of the "script_file" facility assumes that at most one
   request can occur in a single delta cycle.  If this is not the case (e.g. when
   "num_ports" is greater then 1) the behavior of the "script_file" facility may be
   different then what you desire.

   "wraparound"           bool  Specifies what will happen when the end of file (EOF) is
                                reached on "script_file".  When EOF is reached and
                                "wraparound" is true, "script_file" will be reset to the
                                beginning of file and the script will be processed
                                again.  When EOF is reached and "wraparound" is false,
                                the xtsc_memory object will cease processing the script
                                file itself but response definitions may still remain in
                                effect.
                                Default = false.

   Note:  The following 4 parameters provide another method (besides "script_file") to
          cause the memory to generate special responses (RSP_NACC, RSP_ADDRESS_ERROR,
          RSP_DATA_ERROR, and RSP_ADDRESS_DATA_ERROR) in order to test the memory
          interface master module's handling of error and nacc responses.
          
   Note:  Combining the two methods for generating special responses is not supported.
          That is, it is not legal for both "script_file" and "fail_request_mask" to be 
          non-zero.

   "fail_status"          u32   This specifies which false error response status
                                (xtsc::xtsc_response::status_t) will be generated
                                (RSP_NACC, RSP_ADDRESS_ERROR, RSP_DATA_ERROR, or 
                                RSP_ADDRESS_DATA_ERROR).
                                Default = RSP_NACC.
                                
   "fail_request_mask"    u32   Each bit of this mask determines whether a false
                                error response may be generated for a particular
                                request type (see xtsc_memory::request_type_t).
                                If the bit is set (i.e. 1), then the corresponding
                                request type is a candidate for a false error
                                response.  If the bit is clear (i.e. 0), then
                                the corresponding request type will not be given
                                any false error responses.  If all bits are 0
                                (REQ_NONE) then no false error responses will
                                be generated.  If all bits are 1 (REQ_ALL), then
                                all requests are candidates for a false error
                                response.
                                Default = 0x00000000 (i.e. REQ_NONE).

   "fail_percentage"      u32   This parameter specifies the probability of
                                a false error response being generated when
                                a request is received whose corresponding
                                bit in "fail_request_mask" is set.  A value 
                                of 100 causes all requests whose corresponding
                                bit in "fail_request_mask" is set to receive
                                a false error response (i.e. 100 percent
                                receive a false error response).  A value of 
                                1 will result in a false error response being
                                sent approximately 1% of the time.  Valid 
                                values are 1 to 100.
                                Note: This mechanism should not be used on memories with
                                      subbanks that share a single busy signal.
                                Default = 100.

   "fail_seed"            u32   This parameter is used to seed the psuedo-random number
                                generator.  Each xtsc_memory instance has its own random
                                sequence based on its "fail_seed" value.  If this
                                parameter is set to 0 then the "random" numbers
                                generated by the sequence will all be 0.
                                Default = 1.

    \endverbatim
 *
 * @see xtsc_memory
 * @see xtsc_memory::request_type_t
 * @see xtsc::xtsc_parms
 * @see xtsc::xtsc_response
 * @see xtsc::xtsc_response::status_t
 * @see xtsc::xtsc_script_file
 */
class XTSC_COMP_API xtsc_memory_parms : public xtsc::xtsc_parms {
public:

  /**
   * Constructor for an xtsc_memory_parms object. After the object is constructed, the
   * data members can be directly written using the appropriate xtsc_parms::set() method
   * in cases where non-default values are desired.
   *
   * @param width8              Memory data interface width in bytes.
   *
   * @param delay               Default delay for read and write in terms of this
   *                            memory's clock period (see "clock_period").  Local
   *                            memory devices should use a delay of 0 for a 5-stage
   *                            pipeline and a delay of 1 for a 7-stage pipeline.  
   *                            PIF/AXI memory devices should use a delay of 1 or more.
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
  xtsc_memory_parms(xtsc::u32   width8            = 4,
                    xtsc::u32   delay             = 0,
                    xtsc::u32   start_address8    = 0,
                    xtsc::u32   size8             = 0,
                    xtsc::u32   num_ports         = 1)
  {
    init(width8, delay, start_address8, size8, num_ports);
  }


  /**
   * Constructor for an xtsc_memory_parms object based upon an xtsc_core object and a
   * named memory interface. 
   *
   * This constructor will determine width8, delay, start_address8, size8, and,
   * optionally, num_ports by querying the core object and then pass the values to the
   * init() method.  If port_name is a ROM interface, then "read_only" will be be set
   * to true.  In addition, the "clock_period" parameter will be set to match the
   * core's clock period.  For PIF/AXI memories, start_address8 and size8 will both be 0
   * indicating a memory which spans the entire 4 gigabyte address space, 
   * "check_alignment" will be set to true, and, for PIF only, "write_responses" will be
   * set opposite to the core's "SimPIFFakeWriteResponses" setting.  If required,
   * "page_byte_size" will be adjusted so that it devides "start_byte_address".  If
   * desired, after the xtsc_memory_parms object is constructed, its data members can be
   * changed using the appropriate xtsc_parms::set() method before passing it to the
   * xtsc_memory constructor.
   *
   * @param     core            A reference to the xtsc_core object upon which to base
   *                            the xtsc_memory_parms.
   *
   * @param     port_name       The memory port name (the name of the memory interface).
   *                            Note:  The core configuration must have the named memory
   *                            interface.
   *
   * @param     delay           Default delay for PIF/AXI read and write in terms of
   *                            this memory's clock period (see "clock_period").  
   *                            Devices should use a delay of 1 or more.  A value of
   *                            0xFFFFFFFF (the default) means to use a delay of 1 if
   *                            the core has a 5-stage pipeline and a delay of 2 if the
   *                            core has a 7-stage pipeline.  For local interfaces,
   *                            the delay is 0 or 1 (5 or 7 stage, respectively)
   *                            regardless of the value passed in.
   *
   * @param     num_ports       The number of ports this memory has.  If 0, the default,
   *                            the number of ports will be inferred based on the number
   *                            of multi-ports in the port_name core interface (assuming
   *                            they are unbound).
   *
   * @see xtsc::xtsc_core::How_to_do_memory_port_binding for a list of legal port_name
   *      values.
   */
  xtsc_memory_parms(const xtsc::xtsc_core& core, const char *port_name, xtsc::u32 delay = 0xFFFFFFFF, xtsc::u32 num_ports = 0);


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
    std::vector<xtsc::u64> fast_access_size;
    std::vector<xtsc::u64> secure_address_range;
    add("byte_width",           width8);
    add("start_byte_address",   start_address8);
    add("memory_byte_size",     size8);
    add("secure_address_range", secure_address_range);
    add("page_byte_size",       1024*16);
    add("initial_value_file",   (char*)NULL);
    add("memory_fill_byte",     0);
    add("host_shared_memory",   false);
    add("interval_size",        (xtsc::u64)1000000);
    add("host_mutex",           false);
    add("shared_memory_name",   (char*)NULL);
    add("num_ports",            num_ports);
    add("delay_from_receipt",   true);
    add("read_delay",           delay);
    add("block_read_delay",     delay);
    add("block_read_repeat",    1);
    add("burst_read_delay",     delay);
    add("burst_read_repeat",    1);
    add("rcw_repeat",           1);
    add("rcw_response",         delay);
    add("write_delay",          delay);
    add("block_write_delay",    delay);
    add("block_write_repeat",   delay);
    add("block_write_response", delay);
    add("burst_write_delay",    delay);
    add("burst_write_repeat",   delay);
    add("burst_write_response", delay);
    add("request_fifo_depth",   2);
    add("write_responses",      true);
    add("check_alignment",      false);
    add("log_user_data_bytes",  0);
    add("read_only",            false);
    add("summary",              false);
    add("support_exclusive",    true);
    add("immediate_timing",     false);
    add("response_repeat",      1);
    add("recovery_time",        1);
    add("clock_period",         0xFFFFFFFF);
    add("use_fast_access",      true);
    add("deny_fast_access",     deny_fast_access);
    add("fast_access_size",     fast_access_size);
    add("use_raw_access",       true);
    add("use_callback_access",  false);
    add("use_custom_access",    false);
    add("use_interface_access", false);
    add("script_file",          (char*)NULL);
    add("exclusive_script_file",(char*)NULL);
    add("wraparound",           false);
    add("fail_status",          xtsc::xtsc_response::RSP_NACC);
    add("fail_request_mask",    0x00000000);
    add("fail_percentage",      100);
    add("fail_seed",            1);
  }

  /// Return what kind of xtsc_parms this is (our C++ type)
  virtual const char* kind() const { return "xtsc_memory_parms"; }
};




/**
 * A PIF, AXI, APB, XLMI, or local memory.
 *
 * Example XTSC module implementing a configurable memory.  
 *
 * On a given port, this memory model always processes transactions in the order they
 * were received.
 *
 * You may use this memory directly or just use the code as a starting place for
 * developing your own memory models.  In some cases, this class can be sub-classed for
 * special functionality.
 *
 * Note: The xtsc_memory module does not ensure RCW transactions are atomic.  Ensuring
 * RCW transactions are atomic is the responsibility of upstream modules.
 *
 * Here is a block diagram of an xtsc_memory as it is used in the hello_world example:
 * @image html  Example_hello_world.jpg
 * @image latex Example_hello_world.eps "hello_world Example" width=10cm
 *
 * Here is the code to connect the system using the xtsc::xtsc_connect() method:
 * \verbatim
    xtsc_connect(core0, "pif", "", core0_pif);
   \endverbatim
 *
 * And here is the code to connect the system using manual SystemC port binding:
 * \verbatim
    core0.get_request_port("pif")(*core0_pif.m_request_exports[0]);
    (*core0_pif.m_respond_ports[0])(core0.get_respond_export("pif"));
   \endverbatim
 *
 * @see xtsc_memory_parms
 * @see xtsc::xtsc_memory_b
 * @see xtsc::xtsc_request_if
 * @see xtsc::xtsc_respond_if
 * @see xtsc::xtsc_core::How_to_do_port_binding
 * @see xtsc_arbiter
 * @see xtsc_dma_engine
 * @see xtsc_router
 * @see xtsc_master
 */
class XTSC_COMP_API xtsc_memory : public sc_core::sc_module, public xtsc::xtsc_module, public xtsc::xtsc_command_handler_interface {
public:


  /// From the memory interface masters (e.g xtsc_core, xtsc_router, etc) to us 
  sc_core::sc_export<xtsc::xtsc_request_if>   **m_request_exports;

  /// From us to the memory interface masters (e.g xtsc_core, xtsc_router, etc)
  sc_core::sc_port<xtsc::xtsc_respond_if>     **m_respond_ports;


  // SystemC needs this
  SC_HAS_PROCESS(xtsc_memory);


  /// Our C++ type (SystemC uses this)
  virtual const char* kind() const { return "xtsc_memory"; }


  /**
   * Constructor for an xtsc_memory.
   *
   * @param     module_name     Name of the xtsc_memory sc_module.
   *
   * @param     memory_parms    The remaining parameters for construction.
   *
   * @see xtsc_memory_parms
   */
  xtsc_memory(sc_core::sc_module_name module_name, const xtsc_memory_parms& memory_parms);


  /// Destructor.
  virtual ~xtsc_memory(void);


  // SystemC calls this method at the end of simulation
  void end_of_simulation();


  /// For xtsc_connection_interface
  virtual xtsc::u32 get_bit_width(const std::string& port_name, xtsc::u32 interface_num = 0) const;


  /// For xtsc_connection_interface
  virtual sc_core::sc_object *get_port(const std::string& port_name);


  /// For xtsc_connection_interface
  virtual std::string get_default_port_name() const { return "slave_ports"; }


  /// For xtsc_connection_interface
  virtual xtsc::xtsc_port_table get_port_table(const std::string& port_table_name) const;


  /// Get the TextLogger for this component (e.g. to adjust its log level)
  log4xtensa::TextLogger& get_text_logger() { return m_text; }


  /// Get the BinaryLogger for this component (e.g. to adjust its log level)
  log4xtensa::BinaryLogger& get_binary_logger() { return m_binary; }


  /**
   * Non-hardware reads (for example, reads by the debugger).
   * @see xtsc::xtsc_request_if::nb_peek
   */
  virtual void peek(xtsc::xtsc_address address8, xtsc::u32 size8, xtsc::u8 *buffer) { m_p_memory->peek(address8, size8, buffer); }


  /**
   * Non-hardware writes (for example, writes from the debugger).
   * @see xtsc::xtsc_request_if::nb_poke
   */
  virtual void poke(xtsc::xtsc_address address8, xtsc::u32 size8, const xtsc::u8 *buffer) { m_p_memory->poke(address8, size8, buffer); }


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
   * Reset the memory.
   */
  void reset(bool hard_reset = false);


  /**
   * Set whether or not exlusive access requests are supported.
   *
   * Note: Calling this method when xtsc_memory_parms "exclusive_script_file" is defined
   * has no useful effect and results in a warning.
   */
  void support_exclusive(bool exclusive);


  /**
   * Top level Function to display statistical summary for a simulation
   */
  void summary(std::ostream& os);


  /**
   * Function to display statistical summary of a port
   */
  void print_summary_per_port(std::ostream& os, xtsc::u32 port_num);


  /**
   * Display statistical summary line
   */
  void print_summary_line(std::ostream& os, const char* stats, const char* seperator, double value,
                          size_t value_setw_size);


  /**
   * Display statistical summary line with start and end time
   */
  void print_summary_line(std::ostream& os, const char* stats, const char* seperator, double value, 
                          sc_core::sc_time &start, sc_core::sc_time &end, size_t value_setw_size,
                          size_t start_interval_setw_size, size_t end_interval_setw_size); 

  /// Get max size - utility function
  size_t get_max_size(
    size_t arg0,
    size_t arg1,
    size_t arg2=0,
    size_t arg3=0,
    size_t arg4=0,
    size_t arg5=0,
    size_t arg6=0,
    size_t arg7=0,
    size_t arg8=0,
    size_t arg9=0
  );

  /**
   * Method to change the clock period.
   *
   * @param     clock_period_factor     Specifies the new length of this device's clock
   *                                    period expressed in terms of the SystemC time
   *                                    resolution (from sc_get_time_resolution()).
   */
  void change_clock_period(xtsc::u32 clock_period_factor);


  /// Implementation of the xtsc::xtsc_command_handler_interface.
  virtual void man(std::ostream& os);


  /**
   * Implementation of the xtsc::xtsc_command_handler_interface.
   *
   * This implementation supports the following commands:
   *  \verbatim
        change_clock_period <ClockPeriodFactor>
          Call xtsc_memory::change_clock_period(<ClockPeriodFactor>).  Return previous
          <ClockPeriodFactor> for this device.

        dump <StartAddress> <NumBytes>
          Dump <NumBytes> of memory starting at <StartAddress> (includes header and
          printable ASCII column).

        dump_exclusive_monitors
        Dump all exclusive monitors, one per line, with format:
        <TranID>:<AddressBeg>-<AddressEnd>

        dump_filtered_request
          Dump the most recent previous xtsc_request that passed a xtsc_request
          watchfilter.

        dump_filtered_response
          Dump the most recent previous xtsc_response that passed a xtsc_response
          watchfilter.

        get_exclusive_monitors_count
          Return the number of exclusive monitors currently active.

        get_total_exclusive_monitors_created
          Return the total number of exclusive monitors created so far in the simulation.

        peek <StartAddress> <NumBytes>
          Peek <NumBytes> of memory starting at <StartAddress>.

        poke <StartAddress> <NumBytes> <Byte1> <Byte2> . . . <ByteN>
          Poke <NumBytes> (=N) of memory starting at <StartAddress>.

        reset [<Hard>]
          Call xtsc_memory::reset(<Hard>).  Where <Hard> is 0|1 (default 0).

        summary 
          Print statistical summary for each port of this instance.
          Call xtsc_memory::summary().

        support_exclusive [<Exclusive>]
          Call xtsc_memory::support_exclusive(<Exclusive>), where <Exclusive> is 0|1, or
          return xtsc_memory::m_support_exclusive.

        watchfilter_add <FilterName> <EventName>
          Calls xtsc_memory::watchfilter_add(<FilterName>, <Event>) and returns the
          watchfilter number.  <EventName> can be a hyphen (-) to mean the last event
          created by the xtsc_event_create command.

        watchfilter_dump
          Return xtsc_memory::watchfilter_dump().

        watchfilter_remove <Watchfilter> | *
          Return xtsc_memory::watchfilter_remove(<Watchfilter>).  An * removes all
          watchfilters.
      \endverbatim
   */
  void execute(const std::string&               cmd_line,
               const std::vector<std::string>&  words,
               const std::vector<std::string>&  words_lc,
               std::ostream&                    result);


  /**
   * Connect an xtsc_arbiter with this xtsc_memory.
   *
   * This method connects the master port pair of the specified xtsc_arbiter with the
   * specified slave port pair of this xtsc_memory.
   *
   * @param     arbiter         The xtsc_arbiter to connect with this xtsc_memory.
   *
   * @param     port_num        The slave port pair of this memory to connect the
   *                            xtsc_arbiter with.
   */
  void connect(xtsc_arbiter& arbiter, xtsc::u32 port_num = 0);


  /**
   * Connect an xtsc_core with this xtsc_memory.
   *
   * This method connects the specified memory interface master port pair of the
   * specified xtsc_core with the specified slave port pair of this xtsc_memory.
   *
   * @param     core                    The xtsc::xtsc_core to connect with this
   *                                    xtsc_memory.
   *
   * @param     memory_port_name        The name of the memory interface master port
   *                                    pair of the xtsc_core to connect with this
   *                                    xtsc_memory.
   *
   * @param     port_num                The slave port pair of this xtsc_memory to
   *                                    connect the xtsc_core with.
   *
   * @param     single_connect          If true only one slave port pair of this memory
   *                                    will be connected.  If false, the default, and
   *                                    if memory_port_name names the first port of an
   *                                    unconnected multi-ported interface of core and
   *                                    if port_num is 0 and if the number of ports this
   *                                    memory has matches the number of multi-ports in
   *                                    the core interface, then all master port pairs
   *                                    of the core interface specified by
   *                                    memory_port_name will be connected to the slave
   *                                    port pairs of this xtsc_memory.
   *
   * @returns number of ports that were connected by this call (1 or 2)
   */
  xtsc::u32 connect(xtsc::xtsc_core& core, const char *memory_port_name, xtsc::u32 port_num = 0, bool single_connect = false);


  /**
   * Connect an xtsc_dma_engine with this xtsc_memory.
   *
   * This method connects the master port pair of the specified xtsc_dma_engine with the
   * specified slave port pair of this xtsc_memory.
   *
   * @param     dma             The xtsc_dma_engine to connect with this xtsc_memory.
   *
   * @param     port_num        The slave port pair of this memory to connect the
   *                            xtsc_dma_engine with.
   */
  void connect(xtsc_dma_engine& dma, xtsc::u32 port_num = 0);


  /**
   * Connect an xtsc_master with this xtsc_memory.
   *
   * This method connects the master port pair of the specified xtsc_master with the
   * specified slave port pair of this xtsc_memory.
   *
   * @param     master          The xtsc_master to connect with this xtsc_memory.
   *
   * @param     port_num        The slave port pair of this memory to connect the
   *                            xtsc_master with.
   */
  void connect(xtsc_master& master, xtsc::u32 port_num = 0);


  /**
   * Connect an xtsc_memory_trace with this xtsc_memory.
   *
   * This method connects the specified master port pair of the upstream
   * xtsc_memory_trace with the specified slave port pair of this xtsc_memory.
   *
   * @param     memory_trace    The xtsc_memory_trace to connect with this xtsc_memory.
   *
   * @param     trace_port      The master port pair of the xtsc_memory_trace to connect
   *                            with this xtsc_memory.
   *
   * @param     port_num        The slave port pair of this memory to connect the
   *                            xtsc_memory_trace with.
   *
   * @param     single_connect  If true only one slave port pair of this memory will be
   *                            connected.  If false, the default, then all contiguous,
   *                            unconnected slave port pairs of this memory starting at
   *                            port_num that have a corresponding existing master port
   *                            pair in memory_trace (starting at trace_port) will be
   *                            connected with that corresponding memory_trace master
   *                            port pair.
   *
   * @returns number of ports that were connected by this call (1 or more)
   */
  xtsc::u32 connect(xtsc_memory_trace& memory_trace, xtsc::u32 trace_port = 0, xtsc::u32 port_num = 0, bool single_connect = false);


  /**
   * Connect an xtsc_pin2tlm_memory_transactor with this xtsc_memory.
   *
   * This method connects the specified TLM master port pair of the specified
   * xtsc_pin2tlm_memory_transactor with the specified slave port pair of this
   * xtsc_memory.
   *
   * @param     pin2tlm         The xtsc_pin2tlm_memory_transactor to connect with this
   *                            xtsc_memory.
   *
   * @param     tran_port       The xtsc_pin2tlm_memory_transactor TLM master port pair
   *                            to connect with this xtsc_memory.
   *
   * @param     port_num        The slave port pair of this xtsc_memory to connect the
   *                            xtsc_pin2tlm_memory_transactor with.
   *
   * @param     single_connect  If true only one slave port pair of this xtsc_memory
   *                            will be connected.  If false, the default, then all
   *                            contiguous, unconnected slave port pairs of this
   *                            xtsc_memory starting at port_num that have a
   *                            corresponding existing TLM master port pair in pin2tlm
   *                            (starting at tran_port) will be connected with that
   *                            corresponding pin2tlm master port pair.
   */
  xtsc::u32 connect(xtsc_pin2tlm_memory_transactor&     pin2tlm,
                    xtsc::u32                           tran_port       = 0,
                    xtsc::u32                           port_num        = 0,
                    bool                                single_connect  = false);


  /**
   * Connect an xtsc_router with this xtsc_memory.
   *
   * This method connects the specified master port pair of the specified xtsc_router
   * with the specified slave port pair of this xtsc_memory.
   *
   * @param     router          The xtsc_router to connect with this xtsc_memory.
   *
   * @param     router_port     The xtsc_router master port pair to connect with this
   *                            xtsc_memory.
   *
   * @param     port_num        The slave port pair of this xtsc_memory to connect the
   *                            xtsc_router with.
   */
  void connect(xtsc_router& router, xtsc::u32 router_port, xtsc::u32 port_num = 0);


  /**
   * Add a watchfilter on peeks, pokes, requests, or responses.
   *
   * @param     filter_name     The filter instance name.  The actual xtsc::xtsc_filter
   *                            object will be obtained via a call to
   *                            xtsc::xtsc_filter_get.  Its kind must be one of
   *                            "xtsc_peek", "xtsc_poke", "xtsc_request", or
   *                            "xtsc_response".
   *
   * @param     event           The sc_event to notify when a nb_peek, nb_poke,
   *                            nb_request, or nb_response (as appropriate) occurs whose
   *                            payload and port passes the filter.
   *
   * @return the watchfilter number (use to remove the watchfilter).
   *
   * @see watchfilter_remove
   * @see xtsc::xtsc_filter
   * @see xtsc::xtsc_filter_get
   */
  xtsc::u32 watchfilter_add(const std::string& filter_name, sc_core::sc_event& event);


  /**
   * Dump a list of all watchfilters applied to this xtsc_memory instance.
   *
   * @param     os              The ostream object to which the watchfilters should be
   *                            dumped.
   *
   * @see watchfilter_add
   * @see xtsc::xtsc_filter
   */
  void watchfilter_dump(std::ostream& os = std::cout);


  /**
   * Remove the specified watchfilter or all watchfilters.
   *
   * @param     watchfilter     The number returned from a previous call to
   *                            watchfilter_add.  A -1 (0xFFFFFFFF) means to remove all
   *                            watchfilters on this xtsc_memory instance.
   *
   * @return the number (count) of watchfilters removed.
   *
   * @see watchfilter_add
   * @see xtsc::xtsc_filter
   */
  xtsc::u32 watchfilter_remove(xtsc::u32 watchfilter);


  // This enum gives the exact request type (especially to distinguish the different
  // block write requests and to distinguish the different rcw requests).
  typedef enum request_type_t {
    REQ_READ            = 1 << 0,         ///< 0x00000001 = Single read
    REQ_WRITE           = 1 << 1,         ///< 0x00000002 = Write
    REQ_BLOCK_READ      = 1 << 2,         ///< 0x00000004 = Block read
    REQ_RCW_1           = 1 << 3,         ///< 0x00000008 = Read-conditional-write request #1
    REQ_RCW_2           = 1 << 4,         ///< 0x00000010 = Read-conditional-write request #2
    REQ_BURST_READ      = 1 << 5,         ///< 0x00000020 = Burst read
    REQ_BURST_WRITE_1   = 1 << 8,         ///< 0x00000100 = Burst write request #1
    REQ_BURST_WRITE_2   = 1 << 9,         ///< 0x00000200 = Burst write request #2
    REQ_BURST_WRITE_3   = 1 << 10,        ///< 0x00000400 = Burst write request #3
    REQ_BURST_WRITE_4   = 1 << 11,        ///< 0x00000800 = Burst write request #4
    REQ_BURST_WRITE_5   = 1 << 12,        ///< 0x00001000 = Burst write request #5
    REQ_BURST_WRITE_6   = 1 << 13,        ///< 0x00002000 = Burst write request #6
    REQ_BURST_WRITE_7   = 1 << 14,        ///< 0x00004000 = Burst write request #7
    REQ_BURST_WRITE_8   = 1 << 15,        ///< 0x00008000 = Burst write request #8
    REQ_BLOCK_WRITE_1   = 1 << 16,        ///< 0x00010000 = Block write request #1
    REQ_BLOCK_WRITE_2   = 1 << 17,        ///< 0x00020000 = Block write request #2
    REQ_BLOCK_WRITE_3   = 1 << 18,        ///< 0x00040000 = Block write request #3
    REQ_BLOCK_WRITE_4   = 1 << 19,        ///< 0x00080000 = Block write request #4
    REQ_BLOCK_WRITE_5   = 1 << 20,        ///< 0x00100000 = Block write request #5
    REQ_BLOCK_WRITE_6   = 1 << 21,        ///< 0x00200000 = Block write request #6
    REQ_BLOCK_WRITE_7   = 1 << 22,        ///< 0x00400000 = Block write request #7
    REQ_BLOCK_WRITE_8   = 1 << 23,        ///< 0x00800000 = Block write request #8
    REQ_BLOCK_WRITE_9   = 1 << 24,        ///< 0x01000000 = Block write request #9
    REQ_BLOCK_WRITE_10  = 1 << 25,        ///< 0x02000000 = Block write request #10
    REQ_BLOCK_WRITE_11  = 1 << 26,        ///< 0x04000000 = Block write request #11
    REQ_BLOCK_WRITE_12  = 1 << 27,        ///< 0x08000000 = Block write request #12
    REQ_BLOCK_WRITE_13  = 1 << 28,        ///< 0x10000000 = Block write request #13
    REQ_BLOCK_WRITE_14  = 1 << 29,        ///< 0x20000000 = Block write request #14
    REQ_BLOCK_WRITE_15  = 1 << 30,        ///< 0x40000000 = Block write request #15
    REQ_BLOCK_WRITE_16  = 1 << 31,        ///< 0x80000000 = Block write request #16
    REQ_ALL                 = 0xFFFFFFFF, ///< 0xFFFFFFFF = All request types
    REQ_NONE                = 0           ///< 0x00000000 = No request types
  } request_type_t;


  /**
   * This method can be used to control the sending of false error responses (for example, to
   * test the upstream memory interface master device's handling of them).
   *
   * @param     status          see "fail_status" in xtsc_memory_parms.
   *
   * @param     request_mask    see "fail_request_mask" in xtsc_memory_parms.
   *
   * @param     fail_percentage see "fail_percentage" in xtsc_memory_parms.
   */
  void setup_false_error_responses(xtsc::xtsc_response::status_t status, xtsc::u32 request_mask, xtsc::u32 fail_percentage);


  /// Clear all addresses and address ranges that are to receive special responses. 
  void clear_addresses();


  /// Dump all addresses and address ranges that are to receive special responses. 
  void dump_addresses(std::ostream& os);


  /// Dump all exclusive monitors in format <TranID>:<AddressBeg>-<AddressEnd>
  void dump_exclusive_monitors(std::ostream& os);


  /**
   * Determine if the specified request should get a special response and also compute type.
   *
   * @param     request         The xtsc_request object.
   *
   * @param     port_num        The slave port number the request came in on.
   *
   * @param     status          Reference in which to return the response status based
   *                            on the "script_file".  Even if this is RSP_OK, the
   *                            request might still get an error response due to a real
   *                            error.
   *
   * @param     list            Reference in which to return list flag to indicate
   *                            BLOCK_READ and BURST_READ responses should set their
   *                            status according to response list.
   *
   * @param     type            Reference in which to return the request type:  
   *                            0=READ, 1=BLOCK_READ, 2=RCW, 3=WRITE, 4=BLOCK_WRITE
   *
   * @return true if this request matches up with one of the address lists.
   */
  bool compute_special_response(const xtsc::xtsc_request&       request,
                                xtsc::u32                       port_num,
                                xtsc::xtsc_response::status_t&  status,
                                bool&                           list,
                                xtsc::u32&                      type);


  /// Extract and return the request type code from the word at m_words[index]
  xtsc::u32 get_request_type_code(xtsc::u32 index);


  /// Compute a pseudo-random sequence based on George Marsaglia's multiply-with-carry method.
  xtsc::u32 random();


  /// POD class to help keep track of information related to a special address or address range.
  class address_info {
  public:

    typedef xtsc::xtsc_response::status_t       status_t;
    typedef xtsc::xtsc_address                  xtsc_address;
    typedef xtsc::u32                           u32;

    address_info(xtsc_address   low_address,
                 xtsc_address   high_address,
                 bool           is_range,
                 u32            port_num,
                 u32            num_ports,
                 u32            type,
                 status_t       status,
                 bool           list,
                 u32            limit) :
      m_low_address     (low_address),
      m_high_address    (high_address),
      m_is_range        (is_range),
      m_port_num        (port_num),
      m_num_ports       (num_ports),
      m_type            (type),
      m_status          (status),
      m_list            (list),
      m_limit           (limit),
      m_count           (0),
      m_finished        (false)
    { }

    void dump(std::ostream& os = std::cout) const;

    /// Increments m_count, adjusts m_finished if required, returns m_finished
    bool used();

    xtsc_address        m_low_address;  ///<  Single address or low address of address range.
    xtsc_address        m_high_address; ///<  High address of address range.
    bool                m_is_range;     ///<  True if this is an address range.
    u32                 m_port_num;     ///<  0-m_num_ports:  If m_port_num==m_num_ports it means port number is don't care.
    u32                 m_num_ports;    ///<  Number of ports that the memory has
    u32                 m_type;         ///<  0-7:  where 0=READ, 1=BLOCK_READ, 2=RCW, 3=WRITE, 4=BLOCK_WRITE, 5=don't care
                                        ///<                      6=BURST_READ                  7=BURST_WRITE
    status_t            m_status;       ///<  The response status to be given (when m_list is false).
    bool                m_list;         ///<  True if the response status should be taken from the response status list.
    u32                 m_limit;        ///<  How many times this address/range should get the specified response (0=no limit).
    u32                 m_count;        ///<  How many times this address/range has gotten the specified response.
    bool                m_finished;     ///<  True when m_limit has been reached.
  };


  /// Class to represent the statistics per port
  class statistics {
  public:

    /// Constructor
    statistics() :
      m_num_nacced_requests     (0),
      m_active_stats_interval   (NULL),
      m_stats_interval_list     (NULL)
    {
      m_stats_interval_list = new std::list<statistics_per_interval*> ();
    }

    ~statistics() {
      delete m_stats_interval_list;
    }

    class statistics_per_interval {
    public:

      statistics_per_interval() :
        m_start_time            (sc_core::SC_ZERO_TIME),
        m_end_time              (sc_core::SC_ZERO_TIME),
        m_num_read_requests     (0),
        m_num_write_requests    (0),
        m_num_read_bytes_xfered (0),
        m_num_write_bytes_xfered(0),
        m_throughput            (0)
      {}

      sc_core::sc_time                     m_start_time;
      sc_core::sc_time                     m_end_time;
      xtsc::u64                            m_num_read_requests;
      xtsc::u64                            m_num_write_requests;
      xtsc::u64                            m_num_read_bytes_xfered;
      xtsc::u64                            m_num_write_bytes_xfered;
      double                               m_throughput;
    };

    xtsc::u64                              m_num_nacced_requests;
    statistics_per_interval               *m_active_stats_interval;
    std::list<statistics_per_interval*>   *m_stats_interval_list;

  };


protected:


  /// Implementation of xtsc_request_if.
  class xtsc_request_if_impl : public xtsc::xtsc_request_if, public sc_core::sc_object {
  public:

    /**
     * Constructor.
     * @param   memory      A reference to the owning xtsc_memory object.
     * @param   port_num    The slave port number that this object serves.
     */
    xtsc_request_if_impl(const char *object_name, xtsc_memory& memory, xtsc::u32 port_num) :
      sc_object         (object_name),
      m_memory          (memory),
      m_p_port          (0),
      m_port_num        (port_num)
    {}

    /// @see xtsc::xtsc_request_if
    virtual void nb_peek(xtsc::xtsc_address address8, xtsc::u32 size8, xtsc::u8 *buffer);

    /// @see xtsc::xtsc_request_if
    virtual void nb_poke(xtsc::xtsc_address address8, xtsc::u32 size8, const xtsc::u8 *buffer);

    /// @see xtsc::xtsc_request_if
    virtual bool nb_fast_access(xtsc::xtsc_fast_access_request  &request);

    /// @see xtsc::xtsc_request_if
    void nb_request(const xtsc::xtsc_request& request);

    /// @see xtsc::xtsc_request_if
    void nb_load_retired(xtsc::xtsc_address address8);

    /// @see xtsc::xtsc_request_if
    void nb_retire_flush();

    /// @see xtsc::xtsc_request_if
    void nb_lock(bool lock);

    /// Return true if a port has bound to this implementation
    bool is_connected() { return (m_p_port != 0); }


  protected:

    /// SystemC callback when something binds to us
    virtual void register_port(sc_core::sc_port_base& port, const char *if_typename);

    xtsc_memory&                m_memory;       ///< Our xtsc_memory object
    sc_core::sc_port_base      *m_p_port;       ///< Port that is bound to us
    xtsc::u32                   m_port_num;     ///< Our slave port pair number
  };



  /// Information about each request
  class request_info {
  public:
    /// Constructor
    request_info(const xtsc::xtsc_request& request, xtsc::xtsc_response::status_t status, bool list) {
      init(request, status, list);
    }
    void init(const xtsc::xtsc_request& request, xtsc::xtsc_response::status_t status, bool list) {
      m_request         = request;
      m_time_stamp      = sc_core::sc_time_stamp();
      m_status          = status;
      m_list            = list;
    }
    xtsc::xtsc_request                  m_request;      ///< Our copy of the request
    sc_core::sc_time                    m_time_stamp;   ///< Timestamp when received
    xtsc::xtsc_response::status_t       m_status;       ///< Response status from "script_file" when m_list is false.
    bool                                m_list;         ///< True if the response status should be taken from the response status list.
  };


  /// Information about each watchfilter
  class watchfilter_info {
  public:
    /// Constructor
    watchfilter_info(const std::string& filter_kind, const std::string& filter_name, sc_core::sc_event& event) :
      m_filter_kind     (filter_kind),
      m_filter_name     (filter_name),
      m_event           (event),
      m_watchfilter     (0)
    {}
    std::string                 m_filter_kind;          ///< The xtsc_filter kind
    std::string                 m_filter_name;          ///< The xtsc_filter name
    sc_core::sc_event&          m_event;                ///< The sc_event to notify when the filter specification passes
    xtsc::u32                   m_watchfilter;          ///< The assigned watchfilter number
  };


  /// Handle xtsc_response filters
  void handle_response_filters(xtsc::u32 port, const xtsc::xtsc_response &response);


  /// Common method to compute/re-compute time delays
  virtual void compute_delays();


  /// Reset internal fifos
  void reset_fifos();


  /// Process optional "script_file"
  void script_thread();


  /// Translate fail_percentage into terms of maximum random value
  void compute_let_through();


  /// Call set_exclusive based on value in "exclusive_script_file".  Return exclusive setting.
  bool set_exclusive_from_script(xtsc::xtsc_address address8, xtsc::xtsc_response& response);


  /// Create an exclusive monitor of the specified address and size and transaction ID (from request)
  void create_exclusive_monitor(xtsc::xtsc_request &request, xtsc::xtsc_address address8, xtsc::u32 size8);


  /// Return whether or not there is a monitor for the given exclusive write and, if so, remove it
  bool check_exclusive_write(xtsc::xtsc_request &request, xtsc::xtsc_address address8, xtsc::u32 size8, xtsc::xtsc_response& response);


  /// Remove all exclusive monitors that overlap a write of the specified address and size.
  void check_exclusive_monitors_against_write(xtsc::xtsc_request &request, xtsc::xtsc_address address8, xtsc::u32 size8);


  /// Return the exact request type of this request
  request_type_t get_request_type(const xtsc::xtsc_request& request, xtsc::u32 port_num);


  /// Return true if an RSP_NACC should be sent for testing purposes
  bool do_nacc_failure(const xtsc::xtsc_request& request, xtsc::u32 port_num);


  /// Update the active statistcs interval, which records various stats for ongoing request
  void update_stats_active_interval();


  /// Compute throughput and close the active statistcs interval
  void close_active_stats_interval(statistics::statistics_per_interval *active_stats_interval, xtsc::u32 port_num);


  /**
   * Return response status for testing purposes.
   *
   * @param list        Use response list for status instead of the return value.
   */
  xtsc::xtsc_response::status_t get_status_for_testing_failures(request_info *p_request_info, xtsc::u32 port_num, bool& list);


  /// Check if a request access secure region
  bool check_non_secure_access(xtsc::xtsc_request* p_request);


  /// Thread to handle transactions at the correct time.
  virtual void worker_thread(void);


  /// Method to handle the current (active) request
  virtual void do_active_request(xtsc::u32 port_num);


  /// Helper method to handle xtsc::xtsc_request::READ.
  virtual void do_read(xtsc::u32 port_num);


  /// Helper method to handle xtsc::xtsc_request::BLOCK_READ.
  virtual void do_block_read(xtsc::u32 port_num);


  /// Helper method to handle BURST_READ.
  virtual void do_burst_read(xtsc::u32 port_num);


  /// Helper method to handle xtsc::xtsc_request::RCW.
  virtual void do_rcw(xtsc::u32 port_num);


  /// Helper method to handle xtsc::xtsc_request::WRITE.
  virtual void do_write(xtsc::u32 port_num);


  /// Helper method to handle xtsc::xtsc_request::BLOCK_WRITE.
  virtual void do_block_write(xtsc::u32 port_num);


  /// Update m_block_write_transfer_count and perform checks on it
  virtual void do_block_write_transfer_count(xtsc::xtsc_request *p_request, xtsc::u32 port_num, bool last_transfer);


  /// Update m_burst_write_transfer_count and perform checks on it
  virtual void do_burst_write_transfer_count(xtsc::xtsc_request *p_request, xtsc::u32 port_num, bool last_transfer);


  /// Helper method to handle BURST_WRITE.
  virtual void do_burst_write(xtsc::u32 port_num);


  /// Helper method to binary log response and then send the response until it is accepted
  void send_response(xtsc::u32 port_num, bool log_data_binary);


  /// Get a new request_info (from the pool)
  request_info *new_request_info(const xtsc::xtsc_request& request, xtsc::xtsc_response::status_t status, bool list);


  /// Delete an request_info (return it to the pool)
  void delete_request_info(request_info*& p_request_info);


  /// Get the object to use for fast access implemented through CALLBACKS
  xtsc::xtsc_fast_access_if *get_fast_access_object() const;


  /// Get the next vector of words from the script file
  int get_words();


  /// Extract a u32 value (named argument_name) from the word at m_words[index]
  xtsc::u32 get_u32(xtsc::u32 index, const std::string& argument_name);


  /// Extract a u64 value (named argument_name) from the word at m_words[index]
  xtsc::u64 get_u64(xtsc::u32 index, const std::string& argument_name);


  /// Extract a double value (named argument_name) from the word at m_words[index]
  double get_double(xtsc::u32 index, const std::string& argument_name);


  /**
   * Method to convert an address or address range string into a pair of numeric
   * addresses.  An address range must be specified without any spaces.  For example,
   * 0x80000000-0x8FFFFFFF.
   *
   * @param index               The index of the string in m_words[].
   *
   * @param argument_name       The name of the argument being converted.  This name is
   *                            from the "script_file" line format documentation.
   *
   * @param low_address         The converted low address.
   *
   * @param high_address        If the string is an address range then this is the
   *                            converted high address.  Otherwise, this is equal to
   *                            low_address.
   *
   * @returns true if the string is an address range.
   */
  bool get_addresses(xtsc::u32           index,
                     const std::string&  argument_name,
                     xtsc::xtsc_address& low_address,
                     xtsc::xtsc_address& high_address);


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


  typedef std::pair<xtsc::xtsc_address, xtsc::xtsc_address> address_range;



  xtsc_memory_parms                     m_memory_parms;                 ///< Copy of xtsc_memory_parms
  xtsc::u32                             m_num_ports;                    ///< The number of ports this memory has
  xtsc::u32                             m_next_port_num;                ///< Used by worker_thread entry to get its slave port number
  xtsc_request_if_impl                **m_request_impl;                 ///< The m_request_exports objects bind to these
  xtsc::xtsc_memory_b                  *m_p_memory;                     ///< The memory itself

  // Buffer requests from the memory interface master
  sc_core::sc_fifo<request_info*>     **m_request_fifo;                 ///< The fifos for incoming requests on each port

  request_info                        **m_p_active_request_info;        ///< The active (current) request on each port
  xtsc::xtsc_response                 **m_p_active_response;            ///< The active (current) response on each port
  xtsc::u32                            *m_block_write_transfer_count;   ///< Keep track of block writes on each port
  xtsc::u32                            *m_burst_write_transfer_count;   ///< Keep track of burst writes on each port
  bool                                 *m_first_block_write;            ///< True if first block write request on each port
  bool                                 *m_first_burst_write;            ///< True if first burst write request on each port
  bool                                 *m_first_rcw;                    ///< True if first RCW request on each port
  sc_core::sc_time                     *m_last_action_time_stamp;       ///< Time of last action on each port: recovery time starts from here

  sc_core::sc_event                   **m_worker_thread_event;          ///< To notify worker_thread of a request on each port

  bool                                 *m_rcw_have_first_transfer;      ///< True if first RCW has been received but not second
  xtsc::u8                             *m_rcw_compare_data;             ///< Comparison data from RCW request

  sc_core::sc_time                      m_clock_period;                 ///< The clock period of this memory
  sc_core::sc_time                      m_statistics_interval_duration; ///< The interval duration for calculating statistics
  bool                                  m_immediate_timing;             ///< True if requests should be handled without any delay
  bool                                  m_delay_from_receipt;           ///< True if delay timing starts from receipt of request
  bool                                  m_write_responses;              ///< See "write_responses" parameter
  bool                                  m_check_alignment;              ///< If true, check that address is size aligned

  sc_core::sc_time                      m_recovery_time;                ///< See "recovery_time" parameter
  sc_core::sc_time                      m_read_delay;                   ///< See "read_delay" parameter
  sc_core::sc_time                      m_block_read_delay;             ///< See "block_read_delay" parameter
  sc_core::sc_time                      m_block_read_repeat;            ///< See "block_read_repeat" parameter
  sc_core::sc_time                      m_burst_read_delay;             ///< See "burst_read_delay" parameter
  sc_core::sc_time                      m_burst_read_repeat;            ///< See "burst_read_repeat" parameter
  sc_core::sc_time                      m_rcw_repeat;                   ///< See "rcw_repeat" parameter
  sc_core::sc_time                      m_rcw_response;                 ///< See "rcw_response" parameter
  sc_core::sc_time                      m_write_delay;                  ///< See "write_delay" parameter
  sc_core::sc_time                      m_block_write_delay;            ///< See "block_write_delay" parameter
  sc_core::sc_time                      m_block_write_repeat;           ///< See "block_write_repeat" parameter
  sc_core::sc_time                      m_block_write_response;         ///< See "block_write_response" parameter
  sc_core::sc_time                      m_burst_write_delay;            ///< See "burst_write_delay" parameter
  sc_core::sc_time                      m_burst_write_repeat;           ///< See "burst_write_repeat" parameter
  sc_core::sc_time                      m_burst_write_response;         ///< See "burst_write_response" parameter
  sc_core::sc_time                      m_response_repeat;              ///< See "response_repeat" parameter

  std::string                           m_script_file;                  ///< The name of the optional script file
  bool                                  m_wraparound;                   ///< Should script file wraparound at EOF

  sc_core::sc_event                     m_script_thread_event;          ///< To notify script_thread of a request 
  xtsc::xtsc_script_file               *m_p_script_stream;              ///< Pointer to the optional script file object
  std::string                           m_line;                         ///< The current script file line
  xtsc::u32                             m_line_count;                   ///< The current script file line number
  std::vector<std::string>              m_words;                        ///< Tokenized words from m_line

  std::string                           m_exclusive_script_file;        ///< See "exclusive_script_file" parameter
  xtsc::xtsc_script_file               *m_p_exclusive_script_stream;    ///< Pointer to the optional exclusive script file object
  std::string                           m_current_exclusive_file;       ///< "exclusive_script_file": Current file name (may be from #include)
  xtsc::u32                             m_exclusive_line_num;           ///< "exclusive_script_file": Current line number
  std::string                           m_exclusive_line;               ///< "exclusive_script_file": Current line
  std::vector<std::string>              m_exclusive_words;              ///< "exclusive_script_file": words from current line 
  std::map<xtsc::u64, address_range>    m_exclusive_monitor_map;        ///< Map transaction ID to exclusive monitor address range
  statistics                          **m_statistics;                   ///< Capture profiling statistics on each port
  bool                                  m_summary;                      ///< See "summary" parameter, method, and command
  bool                                  m_support_exclusive;            ///< See "support_exclusive" parameter, method, and command
  bool                                 *m_multi_write_doit;             ///< Do the writes of an exclusive BLOCK_WRITE or BURST_WRITE (per port)
  xtsc::u32                             m_exclusive_monitors;           ///< Total exclusive monitors created - logged at end of simulation

  xtsc::u32                             m_prev_type;                    ///< type code of most recent previous request
  bool                                  m_prev_hit;                     ///< true if request address was in one of the address lists
  xtsc::u32                             m_prev_port;                    ///< port number of most recent previous request

  xtsc::xtsc_response::status_t         m_fail_status;                  ///< See "fail_status" parameter
  xtsc::u32                             m_fail_request_mask;            ///< See "fail_request_mask" parameter
  xtsc::u32                             m_fail_percentage;              ///< See "fail_percentage" parameter
  xtsc::u32                             m_fail_seed;                    ///< See "fail_seed" parameter.
  xtsc::u32                             m_z;                            ///< For Marsaglia's multipy-with-carry PRNG.
  xtsc::u32                             m_w;                            ///< For Marsaglia's multipy-with-carry PRNG.
  xtsc::u32                             m_let_through;                  ///< Gate when "fail_request_mask" is non-zero
  std::string                           m_lua_function;                 ///< See LUA_FUNCTION under "script_file" parameter
  bool                                  m_last;                         ///< See LAST under "script_file" parameter
  bool                                  m_read_only;                    ///< See "read_only" parameter
  xtsc::u32                             m_log_user_data_bytes;          ///< See "log_user_data_bytes" parameter
  xtsc::u32                             m_user_data_type;               ///< 0=none (use default), 1=value, 2=pointer
  xtsc::u32                             m_user_data_length;             ///< Number of bytes in or pointed to by m_p_user_data
  xtsc::u8                             *m_p_user_data;                  ///< To override default response user data
  bool                                  m_is_shared;                    ///< See IS_SHARED under "script_file" parameter
  bool                                  m_pass_dirty;                   ///< See PASS_DIRTY under "script_file" parameter

  std::vector<request_info*>            m_request_pool;                 ///< Pool of requests

  bool                                  m_host_shared_memory;           ///< See "host_shared_memory" parameter
  xtsc::u64                             m_interval_size;                ///< See "interval_size" parameter
  bool                                  m_host_mutex;                   ///< See "host_mutex" parameter

  bool                                  m_use_fast_access;              ///< For turboxim.  See "use_fast_access".
  std::vector<xtsc::u64>                m_deny_fast_access;             ///< For turboxim.  See "deny_fast_access".
  std::vector<xtsc::u64>                m_fast_access_size;             ///< For turboxim.  See "fast_access_size".
  bool                                  m_use_raw_access;               ///< For turboxim.  See "use_raw_access".
  bool                                  m_use_callback_access;          ///< For turboxim.  See "use_callback_access".
  bool                                  m_use_custom_access;            ///< For turboxim.  See "use_custom_access".
  bool                                  m_use_interface_access;         ///< For turboxim.  See "use_interface_access".
  xtsc::xtsc_fast_access_if            *m_fast_access_object;           ///< Object for fast access through CALLBACKS

  std::vector<xtsc::xtsc_response::status_t>  m_response_list;          ///< See RESPONSE_LIST under "script_file".
  std::map<xtsc::xtsc_address, address_info*> m_address_map;            ///< Map of single addresses
  std::map<xtsc::xtsc_address, address_info*> m_address_range_map;      ///< Map of address ranges

  std::vector<sc_core::sc_process_handle>     m_process_handles;        ///< For reset 

  std::map<xtsc::u32, watchfilter_info*>      m_watchfilters;           ///< The currently active watchfilters.

  std::set<xtsc::u32>                   m_peek_watchfilters;            ///< The currently active peek          watchfilters.
  std::set<xtsc::u32>                   m_poke_watchfilters;            ///< The currently active poke          watchfilters.
  std::set<xtsc::u32>                   m_request_watchfilters;         ///< The currently active xtsc_request  watchfilters.
  std::set<xtsc::u32>                   m_response_watchfilters;        ///< The currently active xtsc_response watchfilters.

  bool                                  m_filter_peeks;                 ///< True if m_peek_watchfilters     is non-empty
  bool                                  m_filter_pokes;                 ///< True if m_poke_watchfilters     is non-empty
  bool                                  m_filter_requests;              ///< True if m_request_watchfilters  is non-empty
  bool                                  m_filter_responses;             ///< True if m_response_watchfilters is non-empty

  xtsc::xtsc_request                    m_filtered_request;             ///< Copy of most recent previous filtered xtsc_request
  xtsc::xtsc_response                   m_filtered_response;            ///< Copy of most recent previous filtered xtsc_response

  xtsc::xtsc_address                    m_start_address8;               ///< The starting byte address of this memory
  xtsc::xtsc_address                    m_size8;                        ///< The byte size of this memory
  std::vector<xtsc::u64>                m_secure_address_range;         ///< For Non secure access.  See "secure_address_range".
  xtsc::u32                             m_width8;                       ///< The byte width of this memories data interface
  xtsc::xtsc_address                    m_end_address8;                 ///< The ending byte address of this memory

  log4xtensa::TextLogger&               m_text;                         ///< Text logger
  log4xtensa::BinaryLogger&             m_binary;                       ///< Binary logger

};



XTSC_COMP_API std::ostream& operator<<(std::ostream& os, const xtsc_memory::address_info& info);



}  // namespace xtsc_component


#endif  // _XTSC_MEMORY_H_
