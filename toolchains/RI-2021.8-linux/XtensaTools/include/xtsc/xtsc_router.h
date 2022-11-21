#ifndef _XTSC_ROUTER_H_
#define _XTSC_ROUTER_H_

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
#include <xtsc/xtsc_parms.h>
#include <xtsc/xtsc_request_if.h>
#include <xtsc/xtsc_respond_if.h>
#include <xtsc/xtsc_request.h>
#include <xtsc/xtsc_response.h>
#include <xtsc/xtsc_address_range_entry.h>
#include <xtsc/xtsc_fast_access.h>
#include <vector>
#include <cstring>
#include <map>



namespace xtsc {
class xtsc_core;
}



namespace xtsc_component {

class xtsc_arbiter;
class xtsc_dma_engine;
class xtsc_master;
class xtsc_memory_trace;
class xtsc_pin2tlm_memory_transactor;
class xtsc_tlm22xttlm_transactor;


/**
 * Constructor parameters for a xtsc_router object.
 *
 *  \verbatim
   Name                  Type   Description
   ------------------    ----   -------------------------------------------------------
  
   "num_slaves"          u32    The number of memory interface slaves attached to the
                                router.  The router will have this number of memory
                                interface master port pairs (one to connect with each
                                slave).  A value 0 means this is a null router which
                                discards or rejects requests.  In this case, the only
                                valid numbers in the routing table and for the
                                "default_port_num" parameter are DISCARD_REQUEST
                                0xFFFFFFFF) and ADDRESS_ERROR (0xFFFFFFFE).
                                Default = 1.

   "slave_byte_widths" vector<u32>  The byte width of the data interface of each PIF/AXI
                                slave.  Typically, this and the "master_byte_width"
                                parameters are left unset and xtsc_router does not
                                concern itself with the byte width of the data interface
                                (it just forwards requests and responses and leaves it
                                to the upstream master and downstream slaves to have
                                matching data interface byte widths).  If desired when
                                modeling a PIF/AXI interface, this parameter can be set to
                                indicate the byte widths of each PIF/AXI slave (in this case
                                the "master_byte_width" parameter must also be set to
                                indicate the byte width of the upstream PIF/AXI master) and
                                the xtsc_router will act as a PIF/AXI width convertor (PWC)
                                to ensure that each request sent out on a request port
                                has the byte width to match the downstream slave and
                                that each response sent out on the response port has a
                                byte width to match the upstream master.  If this
                                parameter is set then "immediate_timing" must be false.
                                If this parameter is set then it must contain exactly
                                "num_slaves" entries.
                                Valid entry values are 4|8|16|32.
                                Default = <empty>

   "master_byte_width"   u32    The data interface byte width of the upstream master.
                                Typically, this parameter should be left at its default
                                value of 0; however, if either the "slave_byte_widths"
                                or "address_routing_bits" parameter is set then this
                                parameter must be set to a non-zero value to indicate
                                the byte width of the upstream master.
                                Valid non-default values are 4|8|16|32 when acting as a
                                PIF/AXI width converter (when "slave_byte_widths" is set)
                                and 4|8|16|32|64 when "address_routing_bits" is set.
                                Default = 0.

   "use_block_requests"  bool   This parameter is only used when acting as a PIF width
                                converter (i.e. when the "slave_byte_widths" parameter
                                is set).  By default, the downstream request type is the
                                same as the upstream request type.  If this parameter is
                                set to true when acting as a PIF width converter, then
                                an upstream WRITE|READ request which has all byte lanes
                                enabled and which is larger then the downstream PIF
                                width will be converted into BLOCK_WRITE|BLOCK_READ
                                request(s).
                                Default = false.

   "default_routing"     bool   If true, the xtsc_router base class determines the
                                sematics of "routing_table".  If false, an xtsc_router 
                                sub-class will determine semantics of "routing_table".
                                Default = true.

   "routing_table"       char*  Information used to create the routing table.  The 
                                xtsc_router class or sub-class (depending upon the
                                "default_routing" parameter) defines the semantics of
                                this information.

                                If "default_routing" is true and "routing_table" is
                                neither NULL nor empty, the xtsc_router class interprets
                                it as the name of a routing table text file.  Other than
                                a lua_function line described below, this file may only
                                contain lines in the following format:
                                  
                                <PortNum> <LowAddr> <HighAddr> [<NewBaseAddr>]
                                
                                1.  Each line of the text file contains three or four
                                    numbers in decimal or hexadecimal (with '0x'
                                    prefix) format.  For example,

                                    //<PortNum>  <LowAddr>   <HighAddr>  <NewBaseAddr>
                                       0          0x40000000  0x4FFFFFFF
                                       0          0x70000000  0x7FFFFFFF
                                       1          0xF0000000  0xFFFFFFFF  0x00000000
                             
                                2.  The first number is the port number.
                                3.  The second number is the low address of an address
                                    range that is to be routed to <PortNum>.
                                4.  The third number is the high address of an address
                                    range that is to be routed to <PortNum>.
                                5.  The optional fourth number specifies a new base
                                    address for address translation using the formula:
        
                                       NewAddr = OldAddr + <NewBaseAddr> - <LowAddr>

                                    If <NewBaseAddr> is not present, then the value 
                                    specified by the "default_delta" parameter is added 
                                    to each address that is in the range specfied by 
                                    <LowAddr> and <HighAddr>.  If <NewBaseAddr> is the
                                    same as <LowAddr> or if <NewBaseAddr> is missing 
                                    and "default_delta" is 0, then NewAddr will be the
                                    same as OldAddr (i.e. the address is unchanged
                                    after routing).
                                6.  <PortNum> can appear more than once (so a single
                                    port can service multiple address ranges). 
                                7.  <PortNum> can be 0xFFFFFFFF (-1) to mean the same
                                    as DISCARD_REQUEST (see "default_port_num").
                                8.  <PortNum> can be 0xFFFFFFFE (-2) to mean the same
                                    as ADDRESS_ERROR (see "default_port_num").
                                9.  Address ranges cannot overlap.
                                10. Comments, extra whitespace, and blank lines are
                                    ignored.   See xtsc_script_file.

                                The "routing_table" file may also contain a lua_function
                                line to specify a Lua function that is to be called by
                                the model to get the port for any address that is not
                                specified and, optionally, a second Lua function to be
                                called to get the translated address.  If lua_function
                                is specified then the "routing_table" file should
                                have a #lua_beg/#lua_end block containing the function
                                definitions.  The format of the lua_function line is:

                                lua_function <LuaPortFunction> [<LuaAddrFunction>]

                                Both Lua functions take a single Lua number argument
                                giving the address.  The first function should return a
                                Lua number giving the port as a whole number.  The
                                second function should return a Lua number giving the
                                new translated address as a whole number.  The values
                                returned by the Lua functions are not cached by the
                                model, instead, the functions are called each time a
                                routing is performed.  This provides a dynamic routing
                                capability.  If lua_function is specified then the
                                "default_port_num" parameter is not used.  

                                Here is an example "routing_table" file using both line
                                formats and including the required Lua snippet:

                                  #lua_beg
                                    function get_port(addr)
                                      -- print("addr=" .. string.format("0x%08x", addr))
                                      return math.floor(addr/0x01000000) % 16
                                    end
                                  #lua_end
                                  lua_function get_port
                                  //<PortNum>  <LowAddr>   <HighAddr>  <NewBaseAddr>
                                     0          0x00000000  0xBFFFFFFF
                                     0          0xD0000000  0xFFFFFFFF

                                If "default_routing" is true and "routing_table" is
                                either NULL or empty and "num_slaves" is not 0, the
                                xtsc_router class constructs a routing table by
                                calculating a memory aperture size equal to the 4GB
                                address space divided by the number of slave modules
                                ("num_slaves" must be a power of 2).  Each incoming
                                request transaction is sent out on the port whose port
                                number is equal to the address of the request
                                transaction divided by the memory aperture size (using
                                integer division).  Address translation is done using
                                "default_delta".

                                If "default_routing" is true and "routing_table" is
                                either NULL or empty and "num_slaves" is 0, the
                                xtsc_router class will discard or reject all requests
                                depending upon the "default_port_num" parameter value
                                which must be either DISCARD_REQUEST (0xFFFFFFFF) or
                                ADDRESS_ERROR (0xFFFFFFFE).

                                Default = NULL.

   "default_delta"       u64    The amount to be added to the address of each 
                                xtsc::xtsc_request that is sent out that does not
                                have <NewBaseAddr> specified for it in the routing
                                table. 
                                Default = 0.

   "default_port_num"    u32    The port number to send requests to when the request's 
                                address is not in the routing table.  Two values have
                                special meaning.  If "default_port_num" is
                                DISCARD_REQUEST (0xFFFFFFFF) then the request is logged
                                and discarded.  If "default_port_num" is ADDRESS_ERROR
                                (0xFFFFFFFE) then the request is logged and for a PIF
                                (non-AXI) request a single RSP_ADDRESS_ERROR response is
                                sent to the upstream master and for an AXI request the
                                required number of DECERR responses are sent to the
                                upstream master.  To cause an exception to be thrown for
                                a request address that is not in the routing table, set
                                this parameter to any number other than the two special
                                values that is greater than of equal to "num_slaves"
                                (e.g. 0xbad1bad1).  Peek/poke requests to an address
                                marked as ADDRESS_ERROR cause an exception to be thrown.
                                Default = ADDRESS_ERROR.

   "address_routing_bits"  vector<u32>  This parameter allows specifying the routing
                                based upon bit field(s) in the address in lieu of
                                specifying a routing table in a file via the
                                "routing_table" parameter.  This can be useful to
                                support interleaved memory banks for either PIF or
                                local memories.
                                If used, this parameter must contain one or more pairs
                                of numbers with each pair defining a bit field in the
                                address.  The first number of the pair defines the
                                high-order bit of the bit field and the second number of
                                the pair defines the low-order bit of the bit field.
                                When a request is received, the designated bit fields
                                from the request address are concatenated in order to
                                form a single number which is used as the port number to
                                send the request out on.  
                                If this parameter is defined then:
                                - It must contain an even number of integers.
                                - Each integer must be less than 32 and greater than or
                                  equal to log2("master_byte_width").
                                - The "num_slaves" parameter must be a power of 2 and
                                  must be greater than 1.
                                - The number of address routing bits specified by this
                                  parameter must be exactly equal to log2("num_slaves").
                                - The "master_byte_width" parameter must be explicitly
                                  set to one of the valid non-zero values.
                                - The following parameters must be left at their default
                                  value:
                                        "slave_byte_widths"
                                        "use_block_requests"
                                        "default_routing"
                                        "routing_table"
                                        "default_delta"
                                        "default_port_num"
                                - The system designer must ensure that the address range
                                  spanned by any block request sequence (BLOCK_READ and
                                  BLOCK_WRITE) targets the same slave (i.e. the same
                                  master port).
                                - If TurboXim is going to be used, then special care
                                  must be taken to ensure that the largest minimum
                                  granularity for TurboXim fast access is accounted for
                                  (the minimum granularity for TurboXim fast access from
                                  a particular processor is the load/store width of that
                                  processor).
                                As an example, consider constructing a router for a
                                memory system with eight banks and a 4-byte data bus.
                                In this case we need three bits for routing (log2(8)=3).
                                Let us say we want to group the eight banks into two
                                groups of four banks each and we want the most
                                significant bit of the byte address to select one of the
                                two groups and we want the least significant bits of the
                                byte address (not counting the byte lane bits) to select
                                one of the four banks in each group.  In the case of a
                                4-byte data bus, the low two bits of the byte address
                                (i.e. bits 1 and 0) select byte lanes on the data bus so
                                the least significant bit that can be used for routing
                                is bit 2.  If we assume a 32-bit address, then the three
                                address bits we want for routing are 31, 3, and 2. 
                                Because the "address_routing_bits" parameter expects bit
                                fields specified as pairs this corresponds to the two
                                pairs (31,31) and (3,2).  With C++ (e.g. in sc_main) the
                                router parameters can be set like this:
                                    xtsc_router_parms router_parms;
                                    router_parms.set("num_slaves", 8);
                                    router_parms.set("master_byte_width", 4);
                                    vector<u32> bits;
                                    bits.push_back(31);
                                    bits.push_back(31);
                                    bits.push_back(3);
                                    bits.push_back(2);
                                    router_parms.set("address_routing_bits", bits);
                                If using xtsc-run and a script file, the router
                                parameters can be set like this:
                                    --set_router_parm=num_slaves=8
                                    --set_router_parm=master_byte_width=4
                                    --set_router_parm=address_routing_bits=31,31,3,2
                                Default = <empty>

   "route_by_priority" vector<u32> This parameter can be used to specify that routing
                                should occur based on the transaction priority rather
                                than on the address.  This can be useful to send
                                transactions with different priority field values out
                                separate ports so that a downstream xtsc_arbiter can
                                assign different arbitration priorities to them (see
                                "arbitration_policy" in xtsc_arbiter_parms).  The
                                transaction priority is obtained by calling
                                xtsc::xtsc_request::get_priority() and represents the 
                                PIF priority (or AxQOS if AXI is being modelled).
                                If this parameter is specified, then it must contain a
                                multiple of 3 entries with each triplet being:
                                   BeginPriorityValue,EndPriorityValue,PortNumber
                                Where BeginPriorityValue must be less than or equal to
                                EndPriorityValue which must be less than or equal to 15.
                                For example, to send all transactions with priority
                                value of 0 or 1 out port 0, and all transactions with
                                priority value of 2 out port 1, and all transactions
                                with any other priority value out the default port of 2,
                                specify:
                                        num_ports=3
                                        route_by_priority=0,1,0,2,2,1
                                        default_port_num=2
                                As a more typical example, to send all transactions out
                                the port number equal to their transaction priority and
                                cause an exception if transaction priority exceeds 3:
                                        num_ports=4
                                        route_by_priority=0,0,0,1,1,1,2,2,2,3,3,3
                                        default_port_num=0xbad1bad1
                                If this parameter is specified, then the following
                                parameters must be left at their default value:
                                        "slave_byte_widths"
                                        "master_byte_width"
                                        "use_block_requests"
                                        "default_routing"
                                        "route_by_type"
                                        "routing_table"
                                        "default_delta"
                                        "address_routing_bits"
                                If desired, the "immediate_timing" parameter can be set
                                to true to prevent the xtsc_router from consuming
                                simulation cycles.
                                Default = <empty>

   "route_by_type"       char*  This parameter can be used to specify that routing
                                should occur based on the transaction type rather than
                                on the address.  This can be useful to send read and
                                write transactions out separate ports so that a
                                downstream arbiter can assign different priorities to
                                them (see "arbitration_policy" in xtsc_arbiter_parms) or
                                so that a single upstream master can deliver BURST_READ
                                and BURST_WRITE transactions to separate AXI4 memory
                                ports.
              Syntax:
                <RouteByType>  := <Port0Types>[;<Port1Types>]...
              Where:
                <PortNTypes>   := [<Type>[,<Type>]...]
                <Type>         := READ|WRITE|BLOCK_READ|BLOCK_WRITE|BURST_READ|
                                  BURST_WRITE|RCW|RCW1|RCW2|SNOOP|PEEK|POKE|FAST_ACCESS|
                                  LOAD_RETIRED
                                - <Type> is case-insensitive
                                - PEEK, POKE, FAST_ACCESS, and LOAD_RETIRED refer to
                                  nb_peek(), nb_poke(), nb_fast_access(), and
                                  nb_load_retired() methods of xtsc::xtsc_request_if.
                                  The others refer to the request type of the
                                  xtsc_request payload in the nb_request() method.
                                - Only READ and WRITE must be specified.  BLOCK_READ,
                                  BURST_READ, RCW1, SNOOP, PEEK, FAST_ACCESS, and
                                  LOAD_RETIRED default to the READ port.  BLOCK_WRITE,
                                  BURST_WRITE, RCW2, and POKE default to the WRITE port.
                                - RCW1 and RCW2 mean the 1st and 2nd beat, respectively,
                                  of RCW.  If RCW is specified then RCW1 and RCW2 should
                                  not be.
                                - The following parameters must be left at their default
                                  value:
                                        "slave_byte_widths"
                                        "master_byte_width"
                                        "use_block_requests"
                                        "default_routing"
                                        "route_by_priority"
                                        "routing_table"
                                        "default_delta"
                                        "default_port_num"
                                        "address_routing_bits"
                                If desired, the "immediate_timing" parameter can be set
                                to true to prevent the xtsc_router from consuming
                                simulation cycles.
                                Default = NULL.
                                Examples:
        <RouteByType>           Description
        --------------------    -------------------------------------------------------
        read;write              All nb_requests() of type READ, BLOCK_READ, BURST_READ,
                                RCW1, and SNOOP as well as nb_fast_access() and 
                                nb_load_retired() go out port 0.  All nb_requests() of
                                type WRITE, BLOCK_WRITE, BURST_WRITE, and RCW2 go out
                                port 1.
        read,write;rcw          Everything goes out port 0 except RCW which goes out
                                port 1.
                                Note: When specifying "route_by_type" on the Linux
                                command line, any semi-colon will require escaping or
                                quoting.  For example:
                ./xtsc_router -route_by_type="read;write"

   "read_only"           bool   By default, this router supports all transaction types.
                                Set this parameter to true to model a modified PIF
                                interconnect that does not have ReqData pins.  If this
                                parameter is true an exception will be thrown if any of
                                the following types of requests are received (nb_poke
                                calls will still be supported):
                                    WRITE, BLOCK_WRITE, RCW, BURST_WRITE
                                Default:  false

   "write_only"          bool   By default, this router supports all transaction types.
                                Set this parameter to true to model a modified PIF
                                interconnect that does not have RespData pins.  If this
                                parameter is true an exception will be thrown if any of
                                the following types of requests are received (nb_peek
                                calls will still be supported):
                                    READ, BLOCK_READ, RCW, BURST_READ
                                Default:  false

   "wait_on_outstanding_write" bool  If set to true, the xtsc_router guarantees that
                                read requests will not be issued until after all 
                                previous writes to the same address have completed, 
                                where completion is defined as the reception of write 
                                response by the xtsc_router. This is only supported 
                                when xtsc_router is used to route requests on the basis
                                of type, ie parameter 'route_by_type' is used. For other
                                routing modes, it is always assumed to be false and 
                                changing its value has no impact. Also, when this 
                                parameter is used with 'route_by_type', 'immediate_timing' 
                                should be left to its default value of false.
                                Default = true                                  

   "interleave_responses" bool  By default, xtsc_router supports PIF 3.x which requires
                                a sequence of BLOCK_READ or BURST_READ responses to be
                                integral.  PIF 4.0 removes the integral requirement.
                                Set this parameter to true to cause xtsc_router to allow
                                other responses to be interleaved in the middle of a
                                sequence of BLOCK_READ or BURST_READ responses.
                                Default = true.

   "flexible_request_id" bool   If this parameter is true, xtsc_router will RSP_NACC any
                                request that has the same transaction ID as any
                                outstanding request on any other output port (where
                                transaction ID is the concatenation of route ID and the
                                8-bit request ID).  The purpose of this is to
                                ensure the PIF 4 response ordering requirement called
                                "Flexible Request ID Usage" is met.  In AXI4 a similar
                                requirement is called read ordering (see A5.3.1 in 
                                ARM IHI 0022E).  If this parameter is left at its
                                default value of false, xtsc_router will not enforce
                                ordering under the assumption that interconnect should
                                not be burdened with responsibilites that are better
                                handled by the terminals (master and/or slave), the
                                system designer, and the programmer.  
                                Default = false.

   "log_peek_poke"       bool   By default, xtsc_router does not log calls to its
                                peek/poke methods.  Set this parameter to true to cause
                                xtsc_router to log them at VERBOSE_LOG_LEVEL.
                                Default = false.

   "clock_period"        u32    This is the length of this router's clock period
                                expressed in terms of the SystemC time resolution (from
                                sc_get_time_resolution()).  A value of 0xFFFFFFFF means
                                to use the XTSC system clock period (from
                                xtsc_get_system_clock_period()).  A value of 0 means one
                                delta cycle.
                                Default = 0xFFFFFFFF (i.e. use the system clock period).

   "delay_from_receipt"  bool   If false, the following delay parameters apply from 
                                the start of processing of the request or response
                                (i.e.  after all previous requests or all previous
                                responses, as appropriate, have been forwarded).  This
                                models a router that can only service one request at a
                                time and one response at a time.  If true, the following
                                delay parameters apply from the time of receipt of the
                                request or response.  This models a router with
                                pipelining.
                                Default = true.

   "recovery_time"       u32    If "delay_from_receipt" is true, this specifies two
                                things.  First, the minimum number of clock periods 
                                after a request is forwarded before the next request 
                                will be forwarded.  Second, the minimum number of 
                                clock periods after a response is forwarded before the
                                next response will be forwarded.
                                If "delay_from_receipt" is false, this parameter is
                                ignored.  
                                Default = 1.

   "request_delay"       u32    The minimum number of clock periods it takes to forward
                                a request.  If "delay_from_receipt" is true, timing 
                                starts when the request is received by the router.  If 
                                "delay_from_receipt" is false, timing starts at the 
                                later of when the request is received and when the
                                previous request was forwarded.  A value of 0 means one
                                delta cycle.  This parameter can be overridden for
                                read transactions or write transactions by using the
                                "read_delay" or "write_delay" parameters, respectively.
                                Default = 1.

   "read_delay"           u32   The minimum number of clock periods it takes to forward
                                a read request.  If "delay_from_receipt" is true, timing 
                                starts when the request is received by the router.  If 
                                "delay_from_receipt" is false, timing starts at the 
                                later of when the request is received and when the
                                previous request was forwarded.  A value of 0 means one
                                delta cycle.  A value of 0xFFFFFFFF, the default, means
                                to use the value from "request_delay".  This parameter
                                applies to READ, BLOCK_READ, BURST_READ, and SNOOP
                                transactions.
                                Default = 0xFFFFFFFF (use "request_delay" value).

   "write_delay"          u32   The minimum number of clock periods it takes to forward
                                a write request.  If "delay_from_receipt" is true, timing 
                                starts when the request is received by the router.  If 
                                "delay_from_receipt" is false, timing starts at the 
                                later of when the request is received and when the
                                previous request was forwarded.  A value of 0 means one
                                delta cycle.  A value of 0xFFFFFFFF, the default, means
                                to use the value from "request_delay".  This parameter
                                applies to WRITE, BLOCK_WRITE, BURST_WRITE, and RCW
                                transactions.
                                Default = 0xFFFFFFFF (use "request_delay" value).

   "nacc_wait_time"      u32    This parameter, expressed in terms of the SystemC time
                                resolution, specifies how long to wait after sending a
                                request downstream to see if it was rejected by
                                RSP_NACC.  This value must not exceed this router's
                                clock period.  A value of 0 means one delta cycle.  A
                                value of 0xFFFFFFFF means to wait for a period equal to
                                this router's clock period.  CAUTION:  A value of 0 can
                                cause an infinite loop in the simulation if the
                                downstream module requires a non-zero time to become
                                available.
                                Default = 0xFFFFFFFF (router's clock period).

   "response_delay"      u32    The minimum number of clock periods it takes to forward
                                a response.  If "delay_from_receipt" is true, timing 
                                starts when the response is received by the router.  If 
                                "delay_from_receipt" is false, timing starts at the 
                                later of when the response is received and when the
                                previous response was forwarded.  A value of 0 means
                                one delta cycle.  
                                Default = 1.

   "response_repeat"     u32    The number of clock periods after a response is sent
                                and rejected before the response will be resent.  A
                                value of 0 means one delta cycle.
                                Default = 1.

   "immediate_timing"    bool   If true, the above timing parameters are ignored and
                                the router model forwards all requests and responses
                                immediately (without any delay). Requests are forwarded
                                not even with a delta cycle. Resposes are forwarded
                                with one delta cycle. If multiple response are recieved
                                in a cycle, they will be sent back in round-roubin order
                                with one cycle delay. If false, the above delay
                                parameters are used to determine router timing.
                                This parameter must be false when the router is being
                                used as a PIF width converter.
                                Default = false.

   "request_fifo_depth"  u32    The depth of the single request fifo.  
                                Default = 2.

   "response_fifo_depth" u32    The depth of the response fifos (each memory interface
                                slave has its own response fifo).
                                Default = 2.

   "response_fifo_depths" vector<u32> The depth of each response fifo.  Each memory
                                interface slave has its own response fifo.  If this
                                parameter is set it must contain "num_slaves" number
                                of values (all non-zero) which will be used to define 
                                the individual response fifo depths in port number order.
                                If this parameter is not set then "response_fifo_depth"
                                (without the trailing s) will define the depth of all
                                the response fifos.
                                Default = <empty>
                                
   "profile_buffers"     bool   If true, the xtsc_router class keeps the track of used 
                                buffers for its internal request and response fifos. At 
                                the end of the simulation, the maximum used buffer and 
                                the first time that the max buffer has been reached are 
                                printed in the output log file at  NOTE  level.  This 
                                information is printed for each request and response fifo
                                separately. 
                                Default = false.                                

    \endverbatim
 *
 * @see xtsc_router
 * @see xtsc::xtsc_parms
 */
class XTSC_COMP_API xtsc_router_parms : public xtsc::xtsc_parms {
public:

  /**
   * Constructor for an xtsc_router_parms object.
   *
   * @param     num_slaves         The number of memory interface slaves controlled by
   *                               the memory interface master.  A value of 1 (the
   *                               default) can be used to cause the router to act like
   *                               a simple pass-through delay and/or
   *                               address-translation device.  A value of 0 can be used
   *                               to cause the the router to be a terminal device that
   *                               either discards or rejects all requests (depending
   *                               upon default_port_num).
   *
   * @param     default_routing    If true, the xtsc_router base class determines the
   *                               sematics of routing_table.  If false, an xtsc_router
   *                               sub-class will determine semantics of routing_table.
   *
   * @param     routing_table      Information used to create routing table.  The
   *                               xtsc_router class or sub-class (depending upon
   *                               default_routing) defines the semantics of this
   *                               information.
   *
   * @param     default_port_num   The port number to send requests to when the
   *                               request's address is not in the routing table.
   *                               Default is to reject the request by responding with
   *                               RSP_ADDRESS_ERROR.
   */
  xtsc_router_parms(xtsc::u32   num_slaves       = 1,
                    bool        default_routing  = true,
                    const char *routing_table    = NULL,
                    xtsc::u32   default_port_num = 0xFFFFFFFE)
  {
    std::vector<xtsc::u32> empty;
    add("num_slaves",                    num_slaves);
    add("slave_byte_widths",             empty);
    add("master_byte_width",             0);
    add("use_block_requests",            false);
    add("default_routing",               default_routing);
    add("routing_table",                 routing_table);
    add("default_port_num",              default_port_num);
    add("default_delta",                 0ULL);
    add("address_routing_bits",          empty);
    add("route_by_priority",             empty);
    add("route_by_type",                 (char*) NULL);
    add("read_only",                     false);
    add("write_only",                    false);
    add("wait_on_outstanding_write",     true);
    add("log_peek_poke",                 false);
    add("interleave_responses",          true);
    add("flexible_request_id",           false);
    add("clock_period",                  0xFFFFFFFF);
    add("delay_from_receipt",            true);
    add("request_delay",                 1);
    add("read_delay",                    0xFFFFFFFF);
    add("write_delay",                   0xFFFFFFFF);
    add("nacc_wait_time",                0xFFFFFFFF);
    add("response_delay",                1);
    add("response_repeat",               1);
    add("recovery_time",                 1);
    add("immediate_timing",              false);
    add("request_fifo_depth",            2);
    add("response_fifo_depth",           2);
    add("response_fifo_depths",          empty);
    add("profile_buffers",               false);
  }


  /// Return what kind of xtsc_parms this is (our C++ type)
  virtual const char* kind() const { return "xtsc_router_parms"; }

};



/**
 * Example XTSC module implementing a router on a PIF network or local memory
 * interconnect.
 *
 * This module allows a memory interface master (for example, an xtsc_core) to be
 * connected with multiple memory interface slaves (for example, multiple xtsc_memory,
 * xtsc_router, and xtsc_arbiter objects).
 *
 * It can be configured to have only one memory interface slave.  This can be useful to
 * filter out unwanted address ranges so the slave never sees them or to introduce a
 * simple delay device.
 *
 * It can also be configured to have no memory interface slaves.  In this "null router"
 * capacity, it either discards all requests or responds with RSP_ADDRESS_ERROR to all
 * requests (depending upon the "default_port_num" parameter).
 *
 * Typically, the routing that this device performs is configured by specifying a
 * routing table file using the "routing_table" parameter.  The "routing_table" allows
 * specifying a fixed routing table, or a dynamic routing table (using Lua), or a
 * combination of both.  Other means of defining the routing include:
 *  - Using the "route_by_priority" parameter to route based on transaction priority.
 *  - Using the "route_by_type" parameter to route based on transaction type.
 *  - Using the "address_routing_bits" parameter to define the routing based on bit
 *    field(s) of the address.
 *  - Using only the high order bits of the address by NOT specifying "route_by_type",
 *    "address_routing_bits", and "routing_table" and leaving "default_routing" true.
 *  - Using a sub-class and setting "default_routing" to false.
 *
 * If desired this router can be used as a PIF width converter (PWC) by setting the
 * "master_byte_width" parameter to indicated the byte width of the upstream PIF master
 * and by setting the "slave_byte_widths" parameter to indicate the byte width of each
 * downstream PIF slave.  
 *
 * Limitations of PIF Width Convertor:
 *
 * - When going from a wide master to a narrow slave if an incoming BLOCK_READ request
 *   requires multiple outgoing BLOCK_READ requests then the downstream system must
 *   return all the BLOCK_READ responses in the order the requests were sent out and
 *   without any intervening responses to other requests.
 *
 * Typically, this module is used as a clocked router on a PIF network (so that routing
 * transactons consumes 1 or more clock periods), but it is also possible to use it as
 * an unclocked (asynchronous) multiplexor on a PIF, local memory, or XLMI interconnect
 * by setting "immediate_timing" to true.
 * 
 * Here is a block diagram of the system used in the xtsc_router example:
 * @image html  Example_xtsc_router.jpg
 * @image latex Example_xtsc_router.eps "xtsc_router Example" width=10cm
 *
 * Here is the code to connect the system using the xtsc::xtsc_connect() method:
 * \verbatim
    xtsc_connect(core0,  "pif",            "slave_port", router);
    xtsc_connect(router, "master_port[0]", "slave_port", sysrom);
    xtsc_connect(router, "master_port[1]", "slave_port", sysram);
   \endverbatim
 *
 * And here is the code to connect the system using manual SystemC port binding:
 * \verbatim
    core0.get_request_port("pif")(router.m_request_export);
    router.m_respond_port(core0.get_respond_export("pif"));
    (*router.m_request_ports[0])(*sysrom.m_request_exports[0]);
    (*sysrom.m_respond_ports[0])(*router.m_respond_exports[0]);
    (*router.m_request_ports[1])(*sysram.m_request_exports[0]);
    (*sysram.m_respond_ports[0])(*router.m_respond_exports[1]);
   \endverbatim
 *
 * @see xtsc_router_parms
 * @see xtsc::xtsc_request_if
 * @see xtsc::xtsc_respond_if
 * @see xtsc::xtsc_core::How_to_do_port_binding
 */
class XTSC_COMP_API xtsc_router : public sc_core::sc_module, public xtsc::xtsc_module, public xtsc::xtsc_command_handler_interface {
public:

  sc_core::sc_export<xtsc::xtsc_request_if>     m_request_export;       ///<  From single master to us
  sc_core::sc_port  <xtsc::xtsc_request_if>   **m_request_ports;        ///<  From us to multiple slaves
  sc_core::sc_export<xtsc::xtsc_respond_if>   **m_respond_exports;      ///<  From multiple slaves to us
  sc_core::sc_port  <xtsc::xtsc_respond_if>     m_respond_port;         ///<  From us to single master


  // For SystemC
  SC_HAS_PROCESS(xtsc_router);


  /// Our C++ type (SystemC uses this)
  virtual const char* kind() const { return "xtsc_router"; }


  /**
   * Constructor for an xtsc_router.
   * @param     module_name     Name of the xtsc_router sc_module.
   * @param     router_parms    The remaining parameters for construction.
   * @see xtsc_router_parms
   */
  xtsc_router(sc_core::sc_module_name module_name, const xtsc_router_parms& router_parms);


  // Destructor.
  ~xtsc_router(void);


  /// For xtsc_connection_interface
  virtual xtsc::u32 get_bit_width(const std::string& port_name, xtsc::u32 interface_num = 0) const;


  /// For xtsc_connection_interface
  virtual sc_core::sc_object *get_port(const std::string& port_name);


  /// For xtsc_connection_interface
  virtual std::string get_default_port_name() const { return "slave_port"; }


  /// For xtsc_connection_interface
  virtual xtsc::xtsc_port_table get_port_table(const std::string& port_table_name) const;


  /**
   * Get the number of memory interface slaves that can be connected with this
   * xtsc_router (this is the number of memory interface master port pairs that this
   * xtsc_router has).
   */
  xtsc::u32 get_num_slaves() { return m_num_slaves; }


  /**
   * Reset the xtsc_router.
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


  /// Implementation of the xtsc::xtsc_command_handler_interface.
  virtual void man(std::ostream& os);


  /**
   * Implementation of the xtsc::xtsc_command_handler_interface.
   *
   * This implementation supports the following commands:
   *  \verbatim
        change_clock_period <ClockPeriodFactor>
          Call xtsc_router::change_clock_period(<ClockPeriodFactor>).  Return previous
          <ClockPeriodFactor> for this router.

        dump_filtered_request
          Dump the most recent previous xtsc_request that passed a xtsc_request
          watchfilter.

        dump_filtered_response
          Dump the most recent previous xtsc_response that passed a xtsc_response
          watchfilter.

        dump_profile_results
          Dump max used buffers for request and response fifos.
          
        dump_transaction_id_counts [<Verbose>]
          Return the buffer from calling dump_transaction_id_counts(<Verbose>).
          <Verbose> may be 0|1 (default is 0).
          
        peek <StartAddress> <NumBytes>
          Peek <NumBytes> of memory starting at <StartAddress>.

        poke <StartAddress> <NumBytes> <Byte1> <Byte2> . . . <ByteN>
          Poke <NumBytes> (=N) of memory starting at <StartAddress>.

        reset
          Call xtsc_router::reset().

        watchfilter_add <FilterName> <EventName>
          Calls xtsc_router::watchfilter_add(<FilterName>, <Event>) and returns the
          watchfilter number.  <EventName> can be a hyphen (-) to mean the last event
          created by the xtsc_event_create command.

        watchfilter_dump
          Return xtsc_router::watchfilter_dump().

        watchfilter_remove <Watchfilter> | *
          Return xtsc_router::watchfilter_remove(<Watchfilter>).  An * removes all
          watchfilters.
      \endverbatim
   */
  virtual void execute(const std::string&               cmd_line,
                       const std::vector<std::string>&  words,
                       const std::vector<std::string>&  words_lc,
                       std::ostream&                    result);


  /**
   * Connect with an upstream xtsc_arbiter.
   *
   * This method connects the master port pair of the specified xtsc_arbiter to the
   * slave port pair of this xtsc_router.
   *
   * @param     arbiter          The upstream xtsc_arbiter to connect with.
   *
   */
  void connect(xtsc_arbiter& arbiter);


  /**
   * Connect with an upstream or downstream (inbound pif) xtsc_core.
   *
   * This method connects this xtsc_router with the memory interface specified by
   * memory_port_name of the specified xtsc_core.  If memory_port_name is "inbound_pif"
   * or "snoop" then the master port pair of this xtsc_router specified by port_num is
   * connected with the inbound pif or snoop slave port pair of core.  If
   * memory_port_name is neither "inbound_pif" nor "snoop" then the master port pair of
   * the memory interface of core specified by memory_port_name is connected with the
   * slave port pair of this xtsc_router.
   *
   * @param     core                    The xtsc_core to connect with.
   *
   * @param     memory_port_name        The name of the memory interface of the
   *                                    xtsc_core to connect with.  Case-insensitive.
   *
   * @param     port_num                This specifies which master port pair of this
   *                                    xtsc_router to connect with the inbound pif or
   *                                    snoop slave port pair of core.  This parameter
   *                                    is ignored unless memory_port_name is
   *                                    "inbound_pif" or "snoop".  If memory_port_name
   *                                    is "inbound_pif" or "snoop", then this parameter
   *                                    must be explicitly set and must be in the range
   *                                    of 0 to this xtsc_router's "num_slaves"
   *                                    parameter minus 1.
   *
   * @see xtsc::xtsc_core::How_to_do_memory_port_binding for a list of valid
   *      memory_port_name values.
   */
  void connect(xtsc::xtsc_core& core, const char *memory_port_name, xtsc::u32 port_num = 0xFFFFFFFF);


  /**
   * Connect with an upstream xtsc_dma_engine.
   *
   * This method connects the master port pair of the specified xtsc_dma_engine with the
   * slave port pair of this xtsc_router.
   *
   * @param     dma_engine      The xtsc_dma_engine to connect with this xtsc_router.
   */
  void connect(xtsc_dma_engine& dma_engine);


  /**
   * Connect with an xtsc_master.
   *
   * This method connects the master port pair of the specified xtsc_master with the
   * slave port pair of this xtsc_router.
   *
   * @param     master          The xtsc_master to connect with this xtsc_router.
   */
  void connect(xtsc_master& master);


  /**
   * Connect with an upstream xtsc_memory_trace.
   *
   * This method connects the specified master port pair of the specified upstream
   * xtsc_memory_trace with the slave port pair of this xtsc_router.
   *
   * @param     memory_trace    The upstream xtsc_memory_trace to connect with this
   *                            xtsc_router.
   *
   * @param     port_num        The master port pair of the upstream
   *                            xtsc_memory_trace to connect with this xtsc_router.
   *                            port_num must be in the range of 0 to the upstream
   *                            xtsc_memory_trace's "num_ports" parameter
   *                            minus 1.
   */
  void connect(xtsc_memory_trace& memory_trace, xtsc::u32 port_num);


  /**
   * Connect to an upstream xtsc_router.
   *
   * This method connects the specified master port of the specified upstream xtsc_router
   * to the slave port of this xtsc_router.
   *
   * @param     router          The upstream xtsc_router to connect to.
   *
   * @param     port_num        The master port of the upstream xtsc_router to connect
   *                            to.  port_num must be in the range of 0 to the upstream
   *                            xtsc_router's "num_slaves" parameter minus 1.
   */
  void connect(xtsc_router& router, xtsc::u32 port_num);


  /**
   * Connect with an upstream xtsc_pin2tlm_memory_transactor.
   *
   * This method connects the specified TLM master port pair of the specified upstream
   * xtsc_pin2tlm_memory_transactor with the slave port pair of this xtsc_router.
   *
   * @param     pin2tlm         The upstream xtsc_pin2tlm_memory_transactor to connect
   *                            with this xtsc_router.
   *
   * @param     port_num        The TLM master port pair of the upstream
   *                            xtsc_pin2tlm_memory_transactor to connect with this
   *                            xtsc_router.  port_num must be in the range of 0 to the
   *                            upstream xtsc_pin2tlm_memory_transactor's "num_ports"
   *                            parameter minus 1.
   */
  void connect(xtsc_pin2tlm_memory_transactor& pin2tlm, xtsc::u32 port_num);


  /**
   * Connect with an upstream xtsc_tlm22xttlm_transactor.
   *
   * This method connects the specified TLM master port pair of the specified upstream
   * xtsc_tlm22xttlm_transactor with the slave port pair of this xtsc_router.
   *
   * @param     tlm22xttlm      The upstream xtsc_tlm22xttlm_transactor to connect
   *                            with this xtsc_router.
   *
   * @param     port_num        The TLM master port pair of the upstream
   *                            xtsc_tlm22xttlm_transactor to connect with this
   *                            xtsc_router.  port_num must be in the range of 0 to the
   *                            upstream xtsc_tlm22xttlm_transactor's "num_ports"
   *                            parameter minus 1.
   */
  void connect(xtsc_tlm22xttlm_transactor& tlm22xttlm, xtsc::u32 port_num);


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
   * Dump a list of all watchfilters applied to this xtsc_router instance.
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
   *                            watchfilters on this xtsc_router instance.
   *
   * @return the number (count) of watchfilters removed.
   *
   * @see watchfilter_add
   * @see xtsc::xtsc_filter
   */
  xtsc::u32 watchfilter_remove(xtsc::u32 watchfilter);


  /**
   * Dump maximum used buffers for request and response fifos.
   *
   * @param     os              The ostream object to which the profile results should
   *                            be dumped.
   *
   */
  void dump_profile_results(std::ostream& os = std::cout);


  // SystemC calls this method at the end of simulation
  void end_of_simulation();  

  
  /// Get the TextLogger for this component (e.g. to adjust its log level)
  log4xtensa::TextLogger& get_text_logger() { return m_text; }


  /// Get the BinaryLogger for this component (e.g. to adjust its log level)
  log4xtensa::BinaryLogger& get_binary_logger() { return m_binary; }


protected:

  typedef struct bit_field_info {
    bit_field_info(xtsc::u32 pre_shift, xtsc::u32 mask, xtsc::u32 post_shift) :
      m_pre_shift       (pre_shift),
      m_mask            (mask),
      m_post_shift      (post_shift)
    {}
    xtsc::u32   m_pre_shift;    ///< Amount to right shift the address by before masking
    xtsc::u32   m_mask;         ///< Mask to extract bits to be used in routing
    xtsc::u32   m_post_shift;   ///< Amount to left shift the masked value by
  } bit_field_info;

  class req_rsp_info;
  class response_info;
  class request_info;

  /// PWC: Convert a request as required for a narrow/wider downstream PIF
  void convert_request(request_info*& p_request_info, xtsc::u32 slave_byte_width);


  /// PWC: Convert a response as required for a narrow/wider upstream PIF.  Return lock.
  bool convert_response(response_info*& p_response_info, xtsc::u32 slave_byte_width, req_rsp_info*& p_req_rsp_info, bool last_xfer, bool wrap_active);


  /// PWC: Throw an exception if unaligned BLOCK_READ request is received (CWF)
  void confirm_alignment(xtsc::u32 total_bytes, xtsc::xtsc_address address8, xtsc::xtsc_request &request);

  /// Each memory request might be split into multiple requests when (master_byte_width > slave_byte_width)
  struct split_request {
    xtsc::xtsc_address address;
    unsigned int pre_wrap_cnt;
    unsigned int post_wrap_cnt;
  };

  /// Implementation of xtsc_request_if.
  class xtsc_request_if_impl : public xtsc::xtsc_request_if, public sc_core::sc_object  {
  public:

    /// Constructor
    xtsc_request_if_impl(const char *object_name, xtsc_router& router) :
      sc_object (object_name),
      m_router  (router),
      m_p_port  (0)
    {}

    /// @see xtsc::xtsc_debug_if
    virtual void nb_peek(xtsc::xtsc_address address8, xtsc::u32 size8, xtsc::u8 *buffer);

    /// @see xtsc::xtsc_debug_if
    virtual void nb_poke(xtsc::xtsc_address address8, xtsc::u32 size8, const xtsc::u8 *buffer);

    /// @see xtsc::xtsc_debug_if
    virtual bool nb_peek_coherent(xtsc::xtsc_address    virtual_address8,
                                  xtsc::xtsc_address    physical_address8,
                                  xtsc::u32             size8,
                                  xtsc::u8             *buffer);

    /// @see xtsc::xtsc_debug_if
    virtual bool nb_poke_coherent(xtsc::xtsc_address    virtual_address8,
                                  xtsc::xtsc_address    physical_address8,
                                  xtsc::u32             size8,
                                  const xtsc::u8       *buffer);

    /// @see xtsc::xtsc_debug_if
    virtual bool nb_fast_access(xtsc::xtsc_fast_access_request &request);

    /// @see xtsc::xtsc_request_if
    void nb_request(const xtsc::xtsc_request& request);

    /// @see xtsc::xtsc_request_if
    void nb_load_retired(xtsc::xtsc_address address8);

    /// @see xtsc::xtsc_request_if
    void nb_retire_flush();

    /// @see xtsc::xtsc_request_if
    void nb_lock(bool lock);

  protected:

    /// SystemC callback when something binds to us
    virtual void register_port(sc_core::sc_port_base& port, const char *if_typename);

    xtsc_router&                m_router;       ///< Our xtsc_router object
    sc_core::sc_port_base      *m_p_port;       ///< Port that is bound to us

  };


  /// Implementation of xtsc_respond_if.
  class xtsc_respond_if_impl : virtual public xtsc::xtsc_respond_if, public sc_core::sc_object  {
  public:

    /**
     * Constructor.
     * @param   router      A reference to the owning xtsc_router object.
     * @param   port_num    The port number that this object represents.
     */
    xtsc_respond_if_impl(const char *object_name, xtsc_router& router, xtsc::u32 port_num) :
      sc_object         (object_name),
      m_router          (router),
      m_p_port          (0),
      m_port_num        (port_num)
    {}

    /// @see xtsc::xtsc_respond_if
    bool nb_respond(const xtsc::xtsc_response& response);

  protected:

    /// SystemC callback when something binds to us
    virtual void register_port(sc_core::sc_port_base& port, const char *if_typename);

    xtsc_router&                m_router;       ///< Our xtsc_router object
    sc_core::sc_port_base      *m_p_port;       ///< Port that is bound to us
    xtsc::u32                   m_port_num;     ///< Our port number

  };


  /// Information about each request
  class request_info {
  public:
    /// Constructor
    request_info(const xtsc::xtsc_request& request) :
      m_request         (request),
      m_time_stamp      (sc_core::sc_time_stamp())
    {}
    xtsc::xtsc_request  m_request;              ///< Our copy of the request
    sc_core::sc_time    m_time_stamp;           ///< Timestamp when received
  };


  /// Information about each response
  class response_info {
  public:
    /// Constructor
    response_info(const xtsc::xtsc_response& response) :
      m_response        (response),
      m_time_stamp      (sc_core::sc_time_stamp())
    {}
    /// Constructor for PWC
    response_info(const xtsc::xtsc_request& request) :
      m_response        (request),
      m_time_stamp      (sc_core::sc_time_stamp())
    {}
    xtsc::xtsc_response m_response;             ///< Our copy of the response
    sc_core::sc_time    m_time_stamp;           ///< Timestamp when received
  };


  /// Information for PIF width converter (PWC) mode 
  class req_rsp_info {
  public:
    req_rsp_info() { memset(this, 0, sizeof(req_rsp_info)); }
    request_info       *m_p_first_request_info;         ///< To create the responses for sending upstream
    xtsc::u32           m_num_rsp_received;             ///< To place the data in the upstream response buffer
    xtsc::u32           m_num_last_xfer_rsp_expected;   ///< To determine when the final downstream response has been received
    xtsc::u32           m_num_last_xfer_rsp_received;   ///< To determine when the final downstream response has been received
    xtsc::u32           m_num_block_write_requests;     ///< To determine last_transfer flag of multi-sets and also data offset
    xtsc::xtsc_address  m_block_write_address;          ///< To determine address for multi-sets (slave width < master width)
    bool                m_responses_sent;               ///< To detect conflicting error responses from multi-sets
    bool                m_single_rsp_error_received;    ///< True if RSP_ADDRESS_ERROR|RSP_ADDRESS_DATA_ERROR received
    request_info       *m_p_nascent_request;            ///< Hold the downstream request being built from multiple BLOCK_WRITE
    response_info      *m_p_nascent_response;           ///< Hold the upstrearm response being built
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


  /**
   * This method determines the port number to associate with the
   * specified address.  It also modifies the specified address
   * if address translation is in effect.  Sub-classes can override
   * this method to use their own routing and address translation
   * algorithms.  
   *
   * @param     address8        The address to look up in the
   *                            routing table or routing algorithm.
   *                            This method should change the
   *                            value of address8 as required
   *                            by any address translation 
   *                            in effect.
   * @return the port number or DISCARD_REQUEST or ADDRESS_ERROR.
   */
  virtual xtsc::u32 get_port_and_apply_address_translation(xtsc::xtsc_address& address8);

  /**
   * Get the port based on the transaction type.
   *
   * This method is used in lieu of get_port_and_apply_address_translation() when
   * "route_by_type" is specified.
   */
  xtsc::u32 get_port_by_type(xtsc::u32 type);

  /// Handle xtsc_response filters
  void handle_response_filters(xtsc::u32 port, const xtsc::xtsc_response &response);

  /// Handle request
  void handle_request(request_info *p_request_info);

  /// Handle incoming requests from single master at the correct time
  virtual void router_thread(void);

  /// PWC: Handle incoming requests from single master at the correct time
  virtual void router_pwc_thread(void);

  /// Handle wait of read requests waiting for outstanding write requests
  void waiting_reads_thread(void);

  /// Handle responses from multiple slaves at the correct time
  void response_arbiter_thread(void);

  /// PWC: Handle responses from multiple slaves at the correct time
  void response_arbiter_pwc_thread(void);

  /// Get a new request_info (from the pool)
  request_info *new_request_info(const xtsc::xtsc_request& request);

  /// Copy a new request_info (using the pool)
  request_info *new_request_info(const request_info& info);

  /// Delete an request_info (return it to the pool)
  void delete_request_info(request_info*& p_request_info);

  /// Get a new response_info (from the pool)
  response_info *new_response_info(const xtsc::xtsc_response& response);

  /// Get a new response_info (from the pool)
  response_info *new_response_info(const xtsc::xtsc_request& request);

  /// Delete an response_info (return it to the pool)
  void delete_response_info(response_info*& p_response_info);

  /// Get a new req_rsp_info (from the pool)
  req_rsp_info *new_req_rsp_info(request_info *first_request_info);

  /// Delete an req_rsp_info (return it to the pool)
  void delete_req_rsp_info(req_rsp_info*& p_req_rsp_info);

  /// Common method to update count in m_transaction_id_counts when "flexible_request_id" is true
  void update_transaction_id_counts(xtsc::u32 port_num, const xtsc::xtsc_response& response);

  /// Check if there is any outstanding write request with the address as used by p_request_info
  bool check_outstanding_write(request_info* p_request_info);

  /**
   * Dump transaction ID's and outstanding counts for each port.
   *
   * @param     os      The ostream object to which the dump should be done.  Format is:
   *                        <Port> <Transaction ID> <OutstandingCount>
   *
   * @param     verbose If true, transaction ID's are shown even if their outstanding
   *                    count is 0.
   *
   * @return total of all outstanding counts (should be 0 at end_of_simulation)
   */
  xtsc::u32 dump_transaction_id_counts(std::ostream& os, bool verbose = false);

  /// Common method to compute/re-compute time delays
  void compute_delays();

  /// Reset internal fifos
  void reset_fifos();

  xtsc_request_if_impl                    m_request_impl;               ///<  m_request_export binds to this
  xtsc_respond_if_impl                  **m_respond_impl;               ///<  m_respond_exports bind to these

  xtsc::u32                               m_request_fifo_depth;         ///<  From "request_fifo_depth" parameter
  sc_core::sc_fifo<request_info*>         m_request_fifo;               ///<  Buffer requests from single master
  sc_core::sc_fifo<request_info*>         m_waiting_reads_fifo;         ///<  Buffer waiting read requests. 
                                                                        ///<  Used with "wait_on_outstanding_write"
  sc_core::sc_fifo<request_info*>         m_ready_reads_fifo;           ///<  Buffer ready read requests. 
                                                                        ///<  Used with "wait_on_outstanding_write"
  sc_core::sc_fifo<response_info*>      **m_response_fifos;             ///<  Buffer responses from multiple slaves

  xtsc_router_parms                       m_router_parms;               ///<  Copy of xtsc_router_parms
  bool                                    m_is_pwc;                     ///<  True if acting as a PIF width converter
  bool                                    m_use_block_requests;         ///<  PWC: From "use_block_requests" parameter
  std::vector<xtsc::u32>                  m_slave_byte_widths;          ///<  PWC: From "slave_byte_widths" parameter
  xtsc::u32                               m_master_byte_width;          ///<  PWC: From "master_byte_width" parameter
  static const xtsc::u8                   m_num_slots = 64;             ///<  PWC: Number of Request ID's (2^6=64)
  xtsc::u8                                m_next_slot;                  ///<  PWC: Next slot to test for availability
  xtsc::u64                               m_pending_request_tag;        ///<  PWC: New ID of pending multi-request (when != m_num_slots)
  xtsc::u64                               m_active_block_read_tag;      ///<  PWC: ID of BLOCK_READ request while responses are active
  request_info                           *m_requests[8];                ///<  PWC: List of requests to be sent downstream
  response_info                          *m_responses[8];               ///<  PWC: List of responses to be sent back upstream
  std::map<xtsc::u64, split_request>      m_issued_split_requests;      ///<  PWC: List of split requests sent to the memory 
  std::map<xtsc::u64, req_rsp_info*>      m_req_rsp_table;              ///<  PWC: A map of outstanding requests indexed by request tag!

  xtsc::u32                               m_num_slaves;                 ///<  The number of slaves (master port pairs)
  bool                                    m_waiting_for_nacc;           ///<  True if waiting for RSP_NACC from slave
  bool                                    m_request_got_nacc;           ///<  True if active request got RSP_NACC from slave
  bool                                    m_first_transfer;             ///<  First transfer flag of the current request
  bool                                    m_lock;                       ///<  Lock response arbiter for multiple response sequences
  xtsc::u32                               m_token;                      ///<  For arbitrating responses
  xtsc::u32                               m_default_port_num;           ///<  If address is not in routing table, send req here
  xtsc::xtsc_address                      m_default_delta;              ///<  Default address translation delta 

  bool                                    m_read_only;                  ///<  From "read_only" parameter
  bool                                    m_write_only;                 ///<  From "write_only" parameter
  bool                                    m_wait_on_outstanding_write;  ///<  From "wait_on_outstanding_write" parameter
  bool                                    m_use_wait_on_outstanding_write; ///<  True if both "wait_on_outstanding_write"
                                                                           ///<  and "route_by_type" are used
  bool                                    m_log_peek_poke;              ///<  From "log_peek_poke" parameter
  bool                                    m_interleave_responses;       ///<  From "interleave_responses" parameter
  bool                                    m_flexible_request_id;        ///<  From "flexible_request_id" parameter

  sc_core::sc_time                        m_clock_period;               ///<  This router's clock period

  bool                                    m_delay_from_receipt;         ///<  True if delay starts upon request receipt
  bool                                    m_immediate_timing;           ///<  True if no delay (not even a delta cycle)
  sc_core::sc_time                        m_last_request_time_stamp;    ///<  Time last request was sent out
  sc_core::sc_time                        m_last_response_time_stamp;   ///<  Time last response was sent out

  sc_core::sc_time                        m_recovery_time;              ///<  See "recovery_time" in xtsc_router_parms
  sc_core::sc_time                        m_request_delay;              ///<  See "request_delay" in xtsc_router_parms
  sc_core::sc_time                        m_read_delay;                 ///<  See "read_delay" in xtsc_router_parms
  sc_core::sc_time                        m_write_delay;                ///<  See "write_delay" in xtsc_router_parms
  sc_core::sc_time                        m_nacc_wait_time;             ///<  See "nacc_wait_time" in xtsc_router_parms
  sc_core::sc_time                        m_response_delay;             ///<  See "response_delay" in xtsc_router_parms
  sc_core::sc_time                        m_response_repeat;            ///<  See "response_repeat" in xtsc_router_parms

  std::vector<xtsc::u32>                  m_address_routing_bits;       ///<  From "address_routing_bits" parameter
  bool                                    m_address_routing;            ///<  True if "address_routing_bits" is non-empty
  std::vector<bit_field_info*>            m_address_routing_info;       ///<  Address bit field information for routing
  xtsc::u32                               m_num_bit_fields;             ///<  Number of address routing bit field pairs
  xtsc::xtsc_address                      m_address_routing_turbo_mask; ///<  Address mask from routing bits lsb for TurboXim nb_fast_access
  xtsc::u32                               m_address_routing_turbo_size; ///<  2^lsb for TurboXim nb_fast_access

  std::vector<xtsc::xtsc_address_range_entry>
                                          m_routing_table;              ///<  The routing table

  std::string                             m_lua_port_function;          ///<  From <LuaPortFunction> in lua_function line of "routing_table" file
  std::string                             m_lua_addr_function;          ///<  From <LuaAddrFunction> in lua_function line of "routing_table" file
  bool                                    m_lua_function;               ///<  True if there was a lua_function line in "routing_table" file

  std::vector<xtsc::u32>                  m_route_by_priority;          ///<  See "route_by_priority" in xtsc_arbiter_parms
  xtsc::u32                               m_priority_port_map[16];      ///<  Map up to 4-bits of priority to port number
  bool                                    m_use_route_by_priority;      ///<  True if "route_by_priority" is non-empty

  std::string                             m_route_by_type;              ///<  See "route_by_type" in xtsc_arbiter_parms
  bool                                    m_use_route_by_type;          ///<  True if "route_by_type" was specified
  std::map<xtsc::u32, xtsc::u32>          m_type_port_map;              ///<  Map transaction type to its port

  std::map<xtsc::u64, xtsc::u32>         *m_transaction_id_counts;      ///<  For when "flexible_request_id" parameter is true.
  std::map<xtsc::u64, xtsc::u64>         *m_transaction_id_timestamps;  ///<  For when "flexible_request_id" parameter is true.

  sc_core::sc_event                       m_router_thread_event;        ///<  To notify router_thread when a request is accepted
  sc_core::sc_event                       m_response_arbiter_thread_event; ///<  To notify response_arbiter_thread 
  sc_core::sc_event                       m_waiting_reads_thread_event; ///<  To notify waiting_reads_thread when a read is waiting
  sc_core::sc_event                       m_outstanding_write_event;    ///<  Notified on the completion of an outstanding write

  std::vector<req_rsp_info*>              m_req_rsp_info_pool;          ///<  Maintain a pool of req_rsp_info to improve performance
  std::vector<request_info*>              m_request_pool;               ///<  Maintain a pool of requests to improve performance
  std::vector<response_info*>             m_response_pool;              ///<  Maintain a pool of responses to improve performance

  std::string                             m_file_name;                  ///<  Routing table file name from "routing_table"
  xtsc::xtsc_script_file                 *m_file;                       ///<  Pointer to routing table file
  std::string                             m_line;                       ///<  Current line of routing table file
  xtsc::u32                               m_line_count;                 ///<  Current line number in routing table file
  std::vector<std::string>                m_words;                      ///<  Current line tokenized into words

  std::map<xtsc::u32, watchfilter_info*>  m_watchfilters;               ///<  The currently active watchfilters.

  std::set<xtsc::u32>                     m_peek_watchfilters;          ///<  The currently active peek          watchfilters.
  std::set<xtsc::u32>                     m_poke_watchfilters;          ///<  The currently active poke          watchfilters.
  std::set<xtsc::u32>                     m_request_watchfilters;       ///<  The currently active xtsc_request  watchfilters.
  std::set<xtsc::u32>                     m_response_watchfilters;      ///<  The currently active xtsc_response watchfilters.

  bool                                    m_filter_peeks;               ///<  True if m_peek_watchfilters     is non-empty
  bool                                    m_filter_pokes;               ///<  True if m_poke_watchfilters     is non-empty
  bool                                    m_filter_requests;            ///<  True if m_request_watchfilters  is non-empty
  bool                                    m_filter_responses;           ///<  True if m_response_watchfilters is non-empty

  xtsc::xtsc_request                      m_filtered_request;           ///<  Copy of most recent previous filtered xtsc_request
  xtsc::xtsc_response                     m_filtered_response;          ///<  Copy of most recent previous filtered xtsc_response

  bool                                    m_profile_buffers;            ///<  See "profile_buffers" in xtsc_router_parms 
  xtsc::u32                               m_max_num_requests;           ///<  The maximum available items in request_fifo
  sc_core::sc_time                        m_max_num_requests_timestamp; ///<  Time when the max request buffer happened
  xtsc::u64                               m_max_num_requests_tag;       ///<  Tag of max buffered item in request_fifo
  xtsc::u32                              *m_max_num_responses;          ///<  The maximum available items in response_fifos
  sc_core::sc_time                       *m_max_num_responses_timestamp;///<  Time when the max response buffers happened
  xtsc::u64                              *m_max_num_responses_tag;      ///<  Tag of max buffered items in response_fifos
  std::map<xtsc::u64, request_info*>      m_outstanding_writes_map;     ///<  Map to keep a track of outstanding write requests. 
                                                                        ///<  Used with "wait_on_outstanding_write"
  std::map<xtsc::xtsc_address, bool>      m_waiting_reads_address_map;  ///<  Map to keep a track of addresses of waiting read requests.  
                                                                        ///<  Used with "wait_on_outstanding_write"

  std::vector<sc_core::sc_process_handle> m_process_handles;            ///<  For reset 

  log4xtensa::TextLogger&                 m_text;                       ///<  Text logger
  log4xtensa::BinaryLogger&               m_binary;                     ///<  Binary logger
  bool                                    m_log_data_binary;            ///<  True if transaction data should be logged by m_binary

  /// Port number to indicate the xtsc::xtsc_request should be discarded
  static const xtsc::u32                  DISCARD_REQUEST = 0xFFFFFFFF;

  /**
   * Port number to indicate that xtsc::xtsc_response::RSP_ADDRESS_ERROR should be sent upstream
   * (instead of sending the request downstream).
   */
  static const xtsc::u32                  ADDRESS_ERROR   = 0xFFFFFFFE;

  // Convert and return the word at m_words[index] as a u32
  xtsc::u32 get_u32(xtsc::u32 index);

  // Convert and return the word at m_words[index] as a u64
  xtsc::u64 get_u64(xtsc::u32 index);

  // Transaction types for "route_by_type" and m_type_port_map
  static const xtsc::u32 READ           = 0x00;
  static const xtsc::u32 WRITE          = 0x80;
  static const xtsc::u32 BLOCK_READ     = 0x10;
  static const xtsc::u32 BLOCK_WRITE    = 0x90;
  static const xtsc::u32 BURST_READ     = 0x30;
  static const xtsc::u32 BURST_WRITE    = 0xB0;
  static const xtsc::u32 RCW            = 0x50;
  static const xtsc::u32 RCW1           = 0x51;
  static const xtsc::u32 RCW2           = 0x52;
  static const xtsc::u32 SNOOP          = 0x60;
  static const xtsc::u32 PEEK           = 0xF0;
  static const xtsc::u32 POKE           = 0xF1;
  static const xtsc::u32 FAST_ACCESS    = 0xF2;
  static const xtsc::u32 LOAD_RETIRED   = 0xF3;

};



}  // namespace xtsc_component




#endif  // _XTSC_ROUTER_H_
