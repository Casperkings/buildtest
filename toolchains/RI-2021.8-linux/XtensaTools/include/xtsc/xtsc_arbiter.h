#ifndef _XTSC_ARBITER_H_
#define _XTSC_ARBITER_H_

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
#include <xtsc/xtsc_fast_access.h>
#include <xtsc/xtsc_request.h>
#include <xtsc/xtsc_response.h>
#include <xtsc/xtsc_parms.h>
#include <xtsc/xtsc_address_range_entry.h>
#include <vector>
#include <deque>



namespace xtsc {
class xtsc_core;
}



/**
 * All XTSC component library objects are in the xtsc_component namespace.
 *
 * Note: this does not include xtsc_core which is in the xtsc namespace.
 */
namespace xtsc_component {

class xtsc_dma_engine;
class xtsc_master;
class xtsc_memory_trace;
class xtsc_router;
class xtsc_pin2tlm_memory_transactor;

/**
 * Constructor parameters for a xtsc_arbiter object.
 *
 * This class contains the constructor parameters for a xtsc_arbiter object.
 *  \verbatim
   Name                  Type   Description
   ------------------    ----   --------------------------------------------------------
  
   "apb"                 bool   If true, then the arbiter will operate using APB
                                protocol which means all requests must have their m_apb
                                flag set true, the arbiter expects at most one request
                                from a given master at a time, and the arbiter will only
                                allow one request downstream at a time.  When false, all
                                requests must have their m_apb flag set false.  
                                When "apb" is true:
                                - The "one_at_a_time" parameter is forced to false.
                                - The "route_id_lsb", "num_route_ids", "nacc_wait_time",
                                  "response_repeat", and "check_route_id_bits" 
                                  parameters are basically not used but must be left at
                                  their default value.
                                - The following parameters must be left at their default
                                  values: "master_byte_widths", "immediate_timing", 
                                  "dram_lock", "external_cbox", "xfer_en_port", and
                                  "fail_port_mask".
                                Default: false

   "num_masters"         u32    The number of memory interface masters competing
                                for the memory interface slave.  The arbiter will
                                have this number of memory interface slave port pairs
                                (one for each master to connect with).

   "master_byte_widths" vector<u32>  The byte width of the data interface of each PIF/AXI
                                master.  Typically, this and the "slave_byte_width"
                                parameters are left unset and xtsc_arbiter does not
                                concern itself with the byte width of the data interface
                                (it just forwards requests and responses and leaves it
                                to the upstream masters and downstream slave to have
                                matching data interface byte widths).  If desired when
                                modeling a PIF/AXI interface, this parameter can be set to
                                indicate the byte widths of each PIF/AXI master (in this
                                case the "slave_byte_width" parameter must also be set
                                to indicate the byte width of the downstream PIF/AXI slave)
                                and the xtsc_arbiter will act as a PIF/AXI width convertor
                                (PWC) to ensure that each request sent out on the
                                request port has the byte width to match the downstream
                                slave and that each response sent out on a response port
                                has a byte width to match the upstream master.  If this
                                parameter is set then "immediate_timing",
                                "arbitration_policy", "external_cbox", and
                                "xfer_en_port" must be left at their default values.
                                If this parameter is set then it must contain exactly
                                "num_masters" entries.
                                Valid entry values are 4|8|16|32.
                                Default (unset).

   "slave_byte_width"    u32    The PIF/AXI data interface byte width of the downstream
                                slave.  Typically, this parameter should be left at
                                its default value of 0; however, if the 
                                "master_byte_widths" parameter is set then this
                                parameter must be set to a non-zero value to indicate
                                the byte width of the downstream PIF/AXI slave.
                                Value non-default values are 4|8|16|32.
                                Default = 0.

   "use_block_requests"  bool   This parameter is only used when acting as a PIF width
                                converter (i.e. when the "master_byte_widths" parameter
                                is set).  By default, the downstream request type is the
                                same as the upstream request type.  If this parameter is
                                set to true when acting as a PIF width converter, then
                                an upstream WRITE|READ request which has all byte lanes
                                enabled and which is larger then the downstream PIF
                                width will be converted into BLOCK_WRITE|BLOCK_READ
                                request(s).
                                Default = false.

   "route_id_lsb"        u32    Each xtsc_request and xtsc_response object contains a
                                route ID data member (accessed using the get_route_id()
                                and set_route_id() methods) that can be used by arbiters
                                (and arbiter-like devices) in the system to enable them
                                to route responses back to the originating master.  The
                                xtsc_arbiter model supports two different modes or
                                methods for using the route ID.  The system designer
                                should ensure all arbiters in the system use the same
                                method.
                                Method #1 - System Designer Specified Routing:
                                Each arbiter in a communication path is assigned a bit
                                field in route ID by the system designer. This bit field
                                is assigned by specifying its least significat bit
                                position in the route ID using this parameter
                                ("route_id_lsb").  When a request is received, the
                                arbiter fills in its assigned bit field in route ID with
                                the port number that the request arrived on.  When a
                                response comes back, the arbiter uses its bit field in
                                route ID to determine which port to forward the reply
                                out on.
                                Note:  The arbiter will use ceil(log2(num_masters)) 
                                       bits in route ID.
                                Warning:  The system designer must ensure that all
                                          arbiter-like devices in a communication path
                                          (such as xtsc_arbiter) use non-overlapping bit
                                          fields in route ID.  The simulator is not able
                                          to reliably detect overlapping bit fields in
                                          route ID and they will probably result in
                                          communication failure.
                                Method #2 - Autonomous Routing:
                                The second method is to set this parameter to 0xFFFFFFFF
                                to indicate that the xtsc_arbiter instance "owns" the
                                whole route ID field but is responsible for ensuring
                                that the route ID of each response sent back upstream
                                matches the route ID contained in the original incoming
                                request.  This means the xtsc_arbiter instance must
                                maintain a table of original route ID and incoming port
                                number which is indexed by the outgoing route ID.  The
                                size of this table is specified by the "num_route_ids"
                                parameter.
                                Note: AXI transactions are not supported when autonomous
                                routing is enabled.

   "check_route_id_bits" bool   If true and "route_id_lsb" is not 0xFFFFFFFF (Method #1
                                above), then a check will be performed on each incoming
                                request to ensure the route ID has no bits set in this
                                arbiters route ID bit field (this check is not 
                                fool-proof).  If "route_id_lsb" is 0xFFFFFFFF (Method #2
                                above), then this parameter is ignored.
                                Default = true.

   "num_route_ids"       u32    If "route_id_lsb" is 0xFFFFFFFF, then this parameter
                                specifies how big this arbiters routing table is. 
                                Note: While the routing table is full, all incoming
                                requests will be rejected with RSP_NACC.
                                Default = 16.

   "translation_file"    char*  The name of a script file providing an address
                                translation table for each of the memory interface slave
                                port pairs.  If "translation_file" is NULL or empty,
                                then no address translation is performed.
                                Default = NULL.

                                If "translation_file" is neither NULL nor empty, then
                                it must name a script file containing lines in the
                                following format:

                                <PortNum> <LowAddr> [<HighAddr>] <NewBaseAddr>

                                1.  The numbers can be in decimal or hexadecimal (with
                                    '0x' prefix) format.
                                2.  <PortNum> is the memory interface slave port pair
                                    number.
                                3.  <LowAddr> is the low address of an address
                                    range that is to be translated.
                                4.  The optional <HighAddr> is the high address of 
                                    the address range that is to be translated.  If
                                    <HighAddr> is not present, it defaults to
                                    XTSC_MAX_ADDRESS (0xFFFFFFFFFFFFFFFFull).
                                5.  <NewBaseAddr> specifies a new base address for
                                    address translation using the formula:
        
                                       NewAddr = OldAddr + <NewBaseAddr> - <LowAddr>

                                6.  The same <PortNum> can appear more than once;
                                    however, address ranges for a given <PortNum>
                                    cannot overlap.
                                7.  Comments, extra whitespace, and blank lines are
                                    ignored.   See xtsc_script_file for a complete list
                                    of pseudo-preprocessor commands.

   "dram_lock"           bool   This parameter can be used to support DRamNLock
                                functionality.  If this parameter is left at its default
                                value of false, then the last transfer flag in
                                xtsc::xtsc_request is used to control locking.  This
                                models arbitration on PIF interconnet and nb_lock()
                                calls would typically not be expected to occur (if they
                                do they will simply be passed downstream).  This
                                parameter can be set to true to model DataRAM interconnect
                                and xtsc::xtsc_request_if::nb_lock() in conjunction with
                                xtsc::xtsc_request_if::nb_request() will be used to lock
                                the arbiter to the port getting the grant (see
                                "lock_port_groups" for a way to lock the arbiter to
                                multiple ports).  If "dram_lock" is true then
                                "immediate_timing" and "master_byte_widths" must be left
                                at their default values.
                                Note: "dram_lock" should be false for system-level
                                interconnect such as PIF, AXI, and APB.
                                Default = false.

   "lock_port_groups"    char*  When "dram_lock" is true, this parameter allows grouping
                                ports together such that if the arbiter is locked to a
                                port then requests from that port or any other port in
                                that port's group will be allowed through.  This
                                parameter can be used to support the S32C1I instruction
                                targeting a split Rd/Wr DataRAM interface or an inbound
                                PIF RCW transaction targeting a split Rd/Wr DataRAM
                                interface.  
              Syntax:
                <LockPortGroups> := <LockPortGroup>[;<LockPortGroup>]...
              Where:
                <LockPortGroup>  := <Port>[,<Port>]...
                                Rules:
                                - <Port> must be in the range from 0 to "num_masters"
                                  minus 1.
                                - A given port number (<Port>) can appear at most one
                                  time (in at most one <LockPortGroup>).
                                - Any port number not specified in a <LockPortGroup> is
                                  in its own lock port group containing itself as the
                                  group's only member.
                                - If "lock_port_groups" is specified then "dram_lock"
                                  must be true and "arbitration_policy" must be set.
                                Default = NULL.
                                Note: When specifying "lock_port_groups" on the Linux
                                command line, any semi-colon will require escaping or
                                quoting.  For example:
                ./xtsc_arbiter -dram_lock=true -lock_port_groups="0,1;2,3;4,5"

   "arbitration_policy"  char*  By default, xtsc_arbiter uses a fair, round-robin
                                arbitration policy.  This parameter can be used to
                                instead specify a custom arbitration policy.
              Syntax:
                <ArbitrationPolicy>  := <Port0Policy>[;<Port1Policy>]...
              Where:
                <PortNPolicy>        := <StartPriority>,<EndPriority>,<Decrement>
                                The full arbitration policy is specified by specifying a
                                port policy for each port.  Each port policy is
                                separated from its predecessor with a semi-colon.  If
                                there are fewer port policies specified then the number
                                of ports (from the "num_masters" parameter) then the
                                last port policy specified applies to all remaining
                                ports.  Each port policy is specified using three comma-
                                separated numbers which specify the start priority, the
                                end priority, and the decrement.  For priorities,
                                smaller numbers mean higher priority, so 0 is the
                                highest priority and <EndPriority> must be less than or
                                equal to <StartPriority>.  If <StartPriority> and
                                <EndPriority> are equal then <Decrement> must be 0.  At
                                the beginning of simulation and after reset(), the
                                current priority of each port is set to its
                                <StartPriority>.  After winning a round of arbitration
                                (each cycle on which a port has a request which is
                                passed downstream), the current priority of that port is
                                reset to its <StartPriority>.  After losing a round of
                                arbitration due to priority (but not due to simple round
                                robin), the current priority of a port is decremented by
                                its <Decrement>, but never below its <EndPriority>.  
                                For a given round of arbitration, the current priority
                                of all ports with an active request is checked to find
                                the highest priority (smallest current priority value).
                                Only ports with an active request whose current priority
                                is equal to the smallest current priority value are
                                consider for that round.  In this set of candidate
                                ports, the grant is given to the next port in round
                                robin sequence to the port which most recently received
                                a grant.  If "arbitration_policy" is set then
                                "immediate_timing", "external_cbox", "xfer_en_port",
                                and "master_byte_widths" must be left at their default
                                values.
                                Default = NULL
                                Examples:
        <ArbitrationPolicy>     Description
        --------------------    -----------------------------------------------------
        0,0,0                   Fair round robin (FRR).
        0,0,0;1,1,0             Port 0 has permanent high priority, all others FRR.
        0,0,0;3,0,1             Port 0 has a priority over other ports until they have
                                been RSP_NACC'd 3 times.
                                Note: When specifying "arbitration_policy" on the Linux
                                command line, any semi-colon will require escaping or
                                quoting.  For example:
                ./xtsc_arbiter -arbitration_policy="0,0,0;3,0,1"
                                Note: If it is desired to assign different priorities to
                                different request types (e.g. to prioritize reads over
                                writes), an upstream xtsc_router can be used to first
                                send the different request types to different ports (see
                                "route_by_type" in xtsc_router_parms).
                                Note: If it is desired to assign arbitration priorities
                                based on the transaction priority value obtained from
                                xtsc::xtsc_request::get_priority(), then an upstream
                                xtsc_router can be used to first send transactions with
                                different priority values to different ports (see
                                "route_by_priority" in xtsc_router_parms).

   "external_cbox"       bool   By default, xtsc_arbiter uses a fair, round-robin
                                arbitration policy.  If you want to use xtsc_arbiter as
                                a type of external CBox to connect dual load/store units
                                of an Xtensa to a single local memory, then this
                                parameter can be set to true to cause xtsc_arbiter to
                                modify the arbitration policy such that if there is a
                                pending READ request on one port and a pending WRITE
                                request to the same address on the other port then the
                                READ will always get priority (this is to support the
                                dual load/store unit requirement that a simultaneous
                                read and write to the same address return old data for
                                the read).  If "external_cbox" is true then
                                "immediate_timing", "master_byte_widths", and
                                "arbitration_policy" must be left at their default
                                values, and "num_masters" must be 2.
                                Default = false.

   "xfer_en_port"        u32    By default, xtsc_arbiter uses a fair, round-robin
                                arbitration policy.  If you want to use xtsc_arbiter on
                                a local memory interface with busy of a TX Xtensa core
                                which also has a boot loader interface, then set this
                                parameter to the slave port the TX is connected to to
                                cause xtsc_arbiter to modify the arbitration policy such
                                that a request received on this port whose get_xfer_en()
                                method returns true will get priority over all other
                                requests.  If "xfer_en_port" is set then 
                                "external_cbox", "immediate_timing",
                                "master_byte_widths", and "arbitration_policy" must
                                be left at their default values.
                                Default = 0xFFFFFFFF (no special xfer_en handling).

   "immediate_timing"    bool   If true, the following timing parameters are ignored and
                                the arbiter module forwards all requests and responses
                                immediately (without any delay--not even a delta cycle).
                                In this case, there is no arbitration, because the 
                                arbiter forwards all requests immediately.  If false, 
                                the following parameters are used to determine arbiter
                                timing.  This parameter must be false when the arbiter
                                is being used as a PIF/AXI width converter or is being
                                used for APB.
                                If "immediate_timing" is true then "apb", "dram_lock",
                                "external_cbox", "xfer_en_port", "master_byte_widths",
                                and "arbitration_policy" must be left at their default
                                values.
                                Default = false.

   "request_fifo_depth"  u32    The depth of the request fifos (each memory interface
                                master has its own request fifo).  
                                Default = 2.

   "request_fifo_depths" vector<u32> The depth of each request fifo.  Each memory
                                interface master has its own request fifo.  If this
                                parameter is set it must contain "num_masters" number
                                of values (all non-zero) which will be used to define 
                                the individual request fifo depths in port number order.
                                If this parameter is not set then "request_fifo_depth"
                                (without the trailing s) will define the depth of all
                                the request fifos.  If this parameter is set then
                                "one_at_a_time" should typically be changed to false.
                                Default:  no entries.

   "response_fifo_depth" u32    The depth of the single response fifo.  
                                Default = 2.

   "read_only"           bool   By default, this arbiter supports all transaction types.
                                Set this parameter to true to model a modified PIF
                                interconnect that does not have ReqData pins.  If this
                                parameter is true an exception will be thrown if any of
                                the following types of requests are received (nb_poke
                                calls will still be supported):
                                    WRITE, BLOCK_WRITE, RCW, BURST_WRITE
                                Default:  false

   "write_only"          bool   By default, this arbiter supports all transaction types.
                                Set this parameter to true to model a modified PIF
                                interconnect that does not have RespData pins.  If this
                                parameter is true an exception will be thrown if any of
                                the following types of requests are received (nb_peek
                                calls will still be supported):
                                    READ, BLOCK_READ, RCW, BURST_READ
                                Default:  false

   "log_peek_poke"       bool   By default, xtsc_arbiter does not log calls to its
                                peek/poke methods.  Set this parameter to true to cause
                                xtsc_arbiter to log them at VERBOSE_LOG_LEVEL.
                                Default = false.

   "clock_period"        u32    This is the length of this arbiter's clock period
                                expressed in terms of the SystemC time resolution
                                (from sc_get_time_resolution()).  A value of 
                                0xFFFFFFFF means to use the XTSC system clock 
                                period (from xtsc_get_system_clock_period()).
                                A value of 0 means one delta cycle.
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

   "arbitration_phase"   u32    The phase of the clock at which arbitration is performed
                                expressed in terms of the SystemC time resolution (from
                                sc_get_time_resolution()).  A value of 0 means to
                                arbitrate at posedge clock as specified by
                                "posedge_offset".  A value of 0xFFFFFFFF means to use a
                                phase of one-half of this arbiter's clock period which
                                corresponds to arbitrating at negedge clock.  The
                                arbitration phase must be strictly less than the
                                arbiter's clock period.
                                Warning:  When there are multiple arbiters on a single
                                communications path between an initiator and a target,
                                they should NOT use the same arbitration phase because
                                this will result in communication failure if the
                                downstream arbiter rejects a request (sends an RSP_NACC
                                to the upstream arbiter) and the SystemC kernel has
                                already run the upstream arbiter for that simulation
                                cycle (in this case the upstream arbiter will throw an
                                exception because the RSP_NACC is received after the
                                deadline).  To represent multiple contiguous arbiters in
                                a communications path as combinatorial logic, assign
                                strictly increasing arbitration phases to them as you
                                move along the communications path from initiator to
                                target.  As an example for three arbiters in the default
                                situation of a 1 ps SystemC time resolution and a 1 ns
                                clock period (1000 ps), one could assign the first
                                arbiter an "arbitration_phase" of 400 (i.e. 400 ps), the
                                second arbiter 450, and the third arbiter 500 (which
                                corresponds to negedge clock).  In this case, each
                                arbiter performs its arbitration and forwards the
                                winning request downstream in time for that request to
                                be considered by the downstream arbiter when it performs
                                its arbitration for that clock period.  To represent
                                multiple contiguous arbiters in a communications path as
                                synchronous logic such that each request spends a clock
                                period in each arbiter in sequence, assign strictly
                                decreasing arbitration phases to each arbiter as you
                                move along the communications path from initiator to
                                target.  For the three arbiter case as above, one could
                                assign the first arbiter an "arbitration_phase" of 500,
                                the second arbiter 450, and the third arbiter 400.  In
                                this case, when the first arbiter performs its
                                arbitration and forwards the winning request downstream
                                it arrives after the second arbiter has performed its
                                arbitration for that clock period and so is not
                                considered until the next clock period.  In interconnect
                                situations involving multiple arbiters and routers, it
                                is possible to have a given arbiter that is upstream
                                from a second arbiter on one communications path but
                                downstream from that same arbiter on another
                                communications path.  In this situation it is not
                                possible to model the whole interconnect as
                                combinatorial logic because this would imply a
                                combinational loop in the hardware.
                                Default = 0xFFFFFFFF (arbitrate at negedge clock).

   "nacc_wait_time"      u32    This parameter, expressed in terms of the SystemC time
                                resolution, specifies how long to wait after sending a
                                request downstream to see if it was rejected by
                                RSP_NACC.  This value must not exceed this arbiter's
                                clock period.  A value of 0 means one delta cycle.  A
                                value of 0xFFFFFFFF means to wait for a period equal to
                                this arbiter's clock period.  CAUTION:  A value of 0 can
                                cause an infinite loop in the simulation if the
                                downstream module requires a non-zero time to become
                                available.
                                Default = 0xFFFFFFFF (arbiter's clock period).

   "one_at_a_time"       bool   If true only one request will be accepted by the arbiter
                                at a time (i.e. one for all memory interface masters put
                                together).  If false, each master can have one or more
                                requests pending at one time with the limit on the
                                number of pending requests from each master being
                                determined by the "request_fifo_depth" or
                                "request_fifo_depths" parameters.  If this parameter is
                                true, then "request_delay" and "recovery_time" as it
                                applies to requests are ignored.  If "apb" is true, then
                                the value of this parameter is considered to be false.
                                Default = true.

   "delay_from_receipt"  bool   If false, the following delay parameters apply from 
                                the start of processing of the request or response (i.e.
                                after all previous requests or all previous responses,
                                as appropriate, have been forwarded).  This models a 
                                arbiter that can only service one request at a time 
                                and one response at a time.  If true, the following 
                                delay parameters apply from the time of receipt of 
                                the request or response.  This models an arbiter with
                                pipelining.
                                Default = true.

   "request_delay"       u32    When "one_at_a_time" is false, this parameter specifies
                                the minimum number of clock periods it takes to forward
                                a request.  If "delay_from_receipt" is true, timing
                                starts when the request is received by the arbiter.  If
                                "delay_from_receipt" is false, timing starts at the
                                later of when the request is received and when the
                                previous request was forwarded.  A value of 0 means one
                                delta cycle.  If "one_at_a_time" is true, then this
                                parameter is ignored.
                                Default = 1.

   "response_delay"      u32    The minimum number of clock periods it takes to forward
                                a response.  If "delay_from_receipt" is true, timing 
                                starts when the response is received by the arbiter.  If 
                                "delay_from_receipt" is false, timing starts at the 
                                later of when the response is received and when the
                                previous response was forwarded.  A value of 0 means one
                                delta cycle.  
                                Default = 1.

   "response_repeat"     u32    The number of clock periods after a response is sent and
                                rejected before the response will be resent.  A value of 
                                0 means one delta cycle.
                                Default = 1.

   "recovery_time"       u32    If "delay_from_receipt" is true, this specifies two
                                things.  First, the minimum number of clock periods 
                                after a request is forwarded before the next request 
                                will be forwarded (this doesn't apply if "one_at_a_time"
                                is true).  Second, the minimum number of clock periods
                                after a response is forwarded before the next response
                                will be forwarded. If "delay_from_receipt" is false,
                                this parameter is ignored.  
                                Default = 1.

   "align_request_phase" bool   If true a delay will be introduced as required to cause
                                the downstream request to go out on the same phase of
                                the clock that the request originally came in on.  If
                                false, then requests go out on the clock phase specified
                                by "arbitration_phase" (plus, when "one_at_a_time" is
                                false, any delay implied by "delay_from_receipt,
                                "request_delay", and "recovery_time").
                                Default = false.

   Note:  The following 3 parameters can be used to test an upstream memory interface
          master's ability to handle RSP_NACC responses early in a simulation run.
          Also see setup_random_rsp_nacc_responses().

   "fail_port_mask"       u32   Each bit of this mask determines whether a random
                                RSP_NACC response may be generated for the corresponding
                                port.  If a bit is set (i.e. 1), then the corresponding
                                port is a candidate for randomly-generated RSP_NACC
                                responses.  If the bit is clear (i.e. 0), then the
                                corresponding port will not be given any
                                randomly-generated RSP_NACC responses.  If all bits are
                                0 then no RSP_NACC responses will be randomly generated.
                                If all bits are 1, then all ports are candidates for a
                                random error response.
                                Default = 0x00000000 (i.e. no random RSP_NACC).

   "fail_percentage"      u32   This parameter specifies the probability of a random
                                RSP_NACC response being generated when a request is
                                received on a port whose corresponding bit in
                                "fail_port_mask" is set.  A value of 100 causes all
                                requests received on ports whose corresponding bit in
                                "fail_port_mask" is set to receive a RSP_NACC response
                                (i.e. all requests on those ports receive a RSP_NACC
                                responses).  A value of 1 will result in a random
                                RSP_NACC response being sent approximately 1% of the
                                time.  Valid values are 1 to 100.
                                Default = 100.

   "fail_seed"            u32   This parameter is used to seed the psuedo-random number
                                generator.  Each xtsc_arbiter instance has its own
                                random sequence based on its "fail_seed" value.  If this
                                parameter is set to 0 then the "random" numbers
                                generated by the sequence will all be 0.
                                Default = 1.

   "profile_buffers"     bool   If true, the xtsc_arbiter class keeps the track of used 
                                buffers for its internal request and response fifos. At 
                                the end of the simulation, the maximum used buffer and 
                                the first time that the max buffer has been reached are 
                                printed in the output log file at  NOTE  level.  This 
                                information is printed for each request and response fifo
                                separately. 
                                Default = false.
                                
    \endverbatim
 *
 * @see xtsc_arbiter
 * @see xtsc::xtsc_parms
 * @see xtsc::xtsc_script_file
 */
class XTSC_COMP_API xtsc_arbiter_parms : public xtsc::xtsc_parms {
public:

  /**
   * Constructor for an xtsc_arbiter_parms object.
   *
   * @param     num_masters     The number of memory interface masters competing for the
   *                            memory interface slave.  A value of 1 (the default) can
   *                            be used to cause the arbiter to act like a simple
   *                            pass-through delay and/or address-translation device.
   *                            The arbiter will have this number of memory interface
   *                            slave port pairs (one for each master to connect with).
   *
   * @param     route_id_lsb    The least significant bit position of this arbiters
   *                            route_id bit field.
   *
   * @param     one_at_a_time   If true, the default, only one request will be accepted
   *                            by the arbiter at a time (i.e. one for all memory
   *                            interface masters put together).  If false, each master
   *                            can have one or more requests pending at one time with
   *                            the number of pending requests for each master being
   *                            determined by the "request_fifo_depth" parameter.
   *
   * Note:  The arbiter will use ceil(log2(num_masters)) bits in the route_id.
   */
  xtsc_arbiter_parms(xtsc::u32 num_masters = 1, xtsc::u32 route_id_lsb = 0, bool one_at_a_time = true) {
    std::vector<xtsc::u32> widths;
    std::vector<xtsc::u32> depths;
    add("apb",                  false);
    add("num_masters",          num_masters);
    add("master_byte_widths",   widths);
    add("slave_byte_width",     0);
    add("use_block_requests",   false);
    add("route_id_lsb",         route_id_lsb);
    add("check_route_id_bits",  true);
    add("num_route_ids",        16);
    add("translation_file",     (char*) NULL);
    add("dram_lock",            false);
    add("lock_port_groups",     (char*) NULL);
    add("arbitration_policy",   (char*) NULL);
    add("external_cbox",        false);
    add("xfer_en_port",         0xFFFFFFFF);
    add("immediate_timing",     false);
    add("request_fifo_depth",   2);
    add("request_fifo_depths",  depths);
    add("response_fifo_depth",  2);
    add("read_only",            false);
    add("write_only",           false);
    add("log_peek_poke",        false);
    add("clock_period",         0xFFFFFFFF);
    add("posedge_offset",       0xFFFFFFFF);
    add("arbitration_phase",    0xFFFFFFFF);
    add("nacc_wait_time",       0xFFFFFFFF);
    add("one_at_a_time",        one_at_a_time);
    add("delay_from_receipt",   true);
    add("request_delay",        1);
    add("response_delay",       1);
    add("response_repeat",      1);
    add("recovery_time",        1);
    add("align_request_phase",  false);
    add("fail_port_mask",       0x00000000);
    add("fail_percentage",      100);
    add("fail_seed",            1);
    add("profile_buffers",      false);
  }

  /// Return what kind of xtsc_parms this is (our C++ type)
  virtual const char* kind() const { return "xtsc_arbiter_parms"; }
};




/**
 * A memory interface arbiter and/or address translator.
 *
 * Example XTSC module implementing an arbiter that allows a memory interface slave
 * module (e.g. xtsc_memory or xtsc_mmio) to be accessed by multiple memory interface
 * master modules (e.g. xtsc_core and/or xtsc_master).  All modules involved communicate
 * via the xtsc::xtsc_request_if and xtsc::xtsc_respond_if interfaces.
 *
 * By default, this module supports PIF and AXI interconnect but can be configured for
 * APB by setting the "apb" parameter true.  It can also be configured for some types
 * of local memory interconnect.  See "dram_lock", "external_cbox", and "xfer_en_port".
 *
 * This module can also be used to provided address translations.  Each memory interface
 * master can have a different set of address translations applied.  See
 * "translation_file" in xtsc_arbiter_parms.
 *
 * By default, this module does arbitration on a fair, round-robin basis and ignores the
 * priority field in the xtsc::xtsc_request and xtsc::xtsc_response objects (other than
 * to forward them on).  If a different arbitration policy is desired, there are several
 * options available:
 * - Use the "arbitration_policy" parameter to specify a custom arbitration policy.
 * - Use the "external_cbox" parameter to prioritize reads over writes. 
 * - Use the "xfer_en_port" parameter (TX Xtensa only).
 * - Subclass xtsc_arbiter and override the virtual arbitrate() method.
 * - Create a modified arbiter starting with the xtsc_arbiter.h and xtsc_arbiter.cpp
 *   source code.
 * - Write a custom arbiter from scratch.
 *
 * If desired this arbiter can be used as a PIF width converter (PWC) by setting the
 * "master_byte_widths" parameter to indicate the byte width of each upstream PIF
 * master and by setting the "slave_byte_width" parameter to indicate the byte width of
 * the downstream PIF slave.  
 *
 * Limitations of PIF Width Convertor:
 *
 * - When going from a wide master to a narrow slave if an incoming BLOCK_READ request
 *   requires multiple outgoing BLOCK_READ requests then the downstream system must
 *   return all the BLOCK_READ responses in the order the requests were sent out and
 *   without any intervening responses to other requests.
 *
 * When not configured as a PWC, this module supports all memory interface data bus
 * widths and so does not need to be configured for any particular data bus width.
 *
 * Warning: Special care must be taken by the system builder with respect to routing
 * and timing when multiple arbiters are used on a single communications path between an
 * initiator and a target.  For routing considerations, see the documentation for the
 * "route_id_lsb" parameter in xtsc_arbiter_parms.  For timing considerations, see the
 * documentation for the "arbitration_phase" parameter in xtsc_arbiter_parms.
 *
 * Note: The "fail_port_mask" parameter can be used to test the ability of upstream
 * memory interface masters to handle RSP_NACC from this arbiter model rather then
 * depending on a chance conflict to cause RSP_NACC which may not happen until long into
 * the simulation or not at all.
 *
 * Here is a block diagram of the system used in the xtsc_arbiter example:
 * @image html  Example_xtsc_arbiter.jpg
 * @image latex Example_xtsc_arbiter.eps "xtsc_arbiter Example" width=10cm
 *
 * Here is the code to connect the system using the xtsc::xtsc_connect() method:
 * \verbatim
    xtsc_connect(arbiter, "master_port", "", shr_mem);
    xtsc_connect(core0, "pif", "slave_port[0]", arbiter);
    xtsc_connect(core1, "pif", "slave_port[1]", arbiter);
   \endverbatim
 *
 * And here is the code to connect the system using manual SystemC port binding:
 * \verbatim
    arbiter.m_request_port(*shr_mem.m_request_exports[0]);
    (*shr_mem.m_respond_ports[0])(arbiter.m_respond_export);
    core0.get_request_port("pif")(*arbiter.m_request_exports[0]);
    (*arbiter.m_respond_ports[0])(core0.get_respond_export("pif"));
    core1.get_request_port("pif")(*arbiter.m_request_exports[1]);
    (*arbiter.m_respond_ports[1])(core1.get_respond_export("pif"));
   \endverbatim
 *
 * @see xtsc::xtsc_request_if
 * @see xtsc::xtsc_respond_if
 * @see xtsc_arbiter_parms
 * @see xtsc::xtsc_core::How_to_do_port_binding
 */
class XTSC_COMP_API xtsc_arbiter : public sc_core::sc_module, public xtsc::xtsc_module, public xtsc::xtsc_command_handler_interface {
public:

  sc_core::sc_export<xtsc::xtsc_request_if>   **m_request_exports;      ///< Masters bind to these
  sc_core::sc_port  <xtsc::xtsc_request_if>     m_request_port;         ///< Bind to single slave
  sc_core::sc_export<xtsc::xtsc_respond_if>     m_respond_export;       ///< Single slave binds to this
  sc_core::sc_port  <xtsc::xtsc_respond_if>   **m_respond_ports;        ///< Bind to masters


  // For SystemC
  SC_HAS_PROCESS(xtsc_arbiter);


  /// Our C++ type (SystemC uses this)
  virtual const char* kind() const { return "xtsc_arbiter"; }


  /**
   * Constructor for an xtsc_arbiter.
   * @param     module_name     Name of the xtsc_arbiter sc_module.
   * @param     arbiter_parms   The remaining parameters for construction.
   * @see xtsc_arbiter_parms
   */
  xtsc_arbiter(sc_core::sc_module_name module_name, const xtsc_arbiter_parms& arbiter_parms);

  // Destructor.
  virtual ~xtsc_arbiter(void);


  /// For xtsc_connection_interface
  virtual xtsc::u32 get_bit_width(const std::string& port_name, xtsc::u32 interface_num = 0) const;


  /// For xtsc_connection_interface
  virtual sc_core::sc_object *get_port(const std::string& port_name);


  /// For xtsc_connection_interface
  virtual std::string get_default_port_name() const { return "master_port"; }


  /// For xtsc_connection_interface
  virtual xtsc::xtsc_port_table get_port_table(const std::string& port_table_name) const;


  /**
   * Get the number of memory interface masters that can be connected with this
   * xtsc_arbiter (this is the number of memory interface slave port pairs that this
   * xtsc_arbiter has).
   */
  xtsc::u32 get_num_masters() { return m_num_masters; }


  /**
   * Reset the xtsc_arbiter.
   */
  virtual void reset(bool hard_reset = false);


  /**
   * In most configurations, this method does the actual arbitration but may be
   * overridden by a sub-class (see Exceptions below).
   *
   * @param     port_num        On entry this specifies the port which most recently
   *                            received a granted.  If this method returns true, then
   *                            it must set port_num to the port receiving the grant.
   *
   * @returns true if a grant is given this round, otherwise returns false.
   *
   * Note: If a sub-class overrides this method then the override must consider, but
   *       must not adjust, m_lock.  See this class's implementation of arbitrate() for
   *       an example of how to do arbitration.
   *
   * Note: Exceptions: This method is not used if "external_cbox", "xfer_en_port",
   *       "arbitration_policy", or "immediate_timing" are set (not left at their
   *       default value).
   */
  virtual bool arbitrate(xtsc::u32& port_num);


  /**
   * This method does the actual arbitration when "arbitration_policy" is specified.
   *
   * @param     port_num        On entry this specifies the port which most recently
   *                            received a granted.  If this method returns true, then
   *                            it must set port_num to the port receiving the grant.
   *
   * @returns true if a grant is given this round, otherwise returns false.
   */
  virtual bool arbitrate_policy(xtsc::u32& port_num);


  /**
   * This method can be used to control the sending of randomly generated RSP_NACC
   * responses (for example, to test the upstream memory interface master device's
   * handling of them).  This method is not supported when "apb" is true.
   *
   * @param     port_mask       See "fail_port_mask" in xtsc_arbiter_parms.
   *
   * @param     fail_percentage See "fail_percentage" in xtsc_arbiter_parms.
   */
  void setup_random_rsp_nacc_responses(xtsc::u32 port_mask, xtsc::u32 fail_percentage);


  /**
   * Method to change the clock period.
   *
   * @param     clock_period_factor      Specifies the new length of this device's clock
   *                                     period expressed in terms of the SystemC time
   *                                     resolution (from sc_get_time_resolution()).
   *
   * @param     arbitration_phase_factor Specifies the phase of the clock at which
   *                                     arbitration is performed expressed in terms of
   *                                     the SystemC time resolution (from
   *                                     sc_get_time_resolution()).  This value must be
   *                                     strictly less then clock_period_factor.
   */
  void change_clock_period(xtsc::u32 clock_period_factor, xtsc::u32 arbitration_phase_factor);


  /**
   * Implementation of the xtsc::xtsc_command_handler_interface.
   *
   * This implementation supports the following commands:
   *  \verbatim
        change_clock_period <ClockPeriodFactor>
          Call xtsc_arbiter::change_clock_period(<ClockPeriodFactor>).  Return previous
          <ClockPeriodFactor> for this device.

        dump_profile_results
          Dump max used buffers for request and response fifos.          

        reset
          Call xtsc_arbiter::reset().
      \endverbatim
   */
  virtual void execute(const std::string&               cmd_line,
                       const std::vector<std::string>&  words,
                       const std::vector<std::string>&  words_lc,
                       std::ostream&                    result);


  /**
   * Dump maximmum used buffers for request and response fifos.
   *
   * @param     os              The ostream object to which the profile results should
   *                            be dumped.
   *
   */
  void dump_profile_results(std::ostream& os = std::cout);


  // SystemC calls this method at the end of simulation
  void end_of_simulation();
  
   
  /**
   * Connect an upstream xtsc_arbiter with this xtsc_arbiter.
   *
   * This method connects the single master port pair of the specified upstream
   * xtsc_arbiter with the specified slave port pair of this xtsc_arbiter.
   *
   * @param     arbiter         The upstream xtsc_arbiter to be connected with this
   *                            xtsc_arbiter.
   *
   * @param     port_num        This specifies the slave port pair of this xtsc_arbiter
   *                            that the single master port pair of the upstream
   *                            xtsc_arbiter is to be connected with.  port_num must
   *                            be in the range of 0 to this xtsc_arbiter's
   *                            "num_masters" parameter minus 1.
   */
  void connect(xtsc_arbiter& arbiter, xtsc::u32 port_num);


  /**
   * Connect with an upstream or downstream (inbound pif) xtsc_core.
   *
   * This method connects this xtsc_arbiter with the memory interface specified by
   * memory_port_name of the xtsc_core specified by core.  If memory_port_name is
   * "inbound_pif" or "snoop" then the master port pair of this xtsc_arbiter is
   * connected with the inbound pif or snoop slave port pair of core.  If
   * memory_port_name is neither "inbound_pif" nor "snoop" then the memory interface
   * master port pair specified by memory_port_name of core is connected with the slave
   * port pair specified by port_num of this xtsc_arbiter.
   *
   * @param     core                    The xtsc_core to connect with.
   *
   * @param     memory_port_name        The memory interface name to connect with.
   *                                    Case-insensitive.
   *
   * @param     port_num                If memory_port_name is neither "inbound_pif" nor
   *                                    "snoop", then the memory interface of core
   *                                    specified by memory_port_name will be connected
   *                                    with the slave port pair of this xtsc_arbiter
   *                                    specified by this parameter.  In this case, this
   *                                    parameter must be explicitly set and must be in
   *                                    the range of 0 to this xtsc_arbiter's
   *                                    "num_masters" parameter minus 1.
   *                                    This parameter is ignored if memory_port_name is
   *                                    "inbound_pif" or "snoop".
   *
   * @see xtsc::xtsc_core::How_to_do_memory_port_binding for a list of valid
   *      memory_port_name values.
   *
   * Note: The snoop port is reserved for future use.
   */
  void connect(xtsc::xtsc_core& core, const char *memory_port_name, xtsc::u32 port_num = 0xFFFFFFFF);


  /**
   * Connect an upstream xtsc_dma_engine with this xtsc_arbiter.
   *
   * This method connects the master port pair of the specified xtsc_dma_engine with the
   * specified slave port pair of this xtsc_arbiter.
   *
   * @param     dma_engine      The xtsc_dma_engine to connect with this xtsc_arbiter.
   *
   * @param     port_num        This specifies the slave port pair of this xtsc_arbiter
   *                            that the specified xtsc_dma_engine will be connected with. 
   *                            port_num must be in the range of 0 to this
   *                            xtsc_arbiter's "num_masters" parameter minus 1.
   */
  void connect(xtsc_dma_engine& dma_engine, xtsc::u32 port_num);


  /**
   * Connect an upstream xtsc_master with this xtsc_arbiter.
   *
   * This method connects the master port pair of the specified xtsc_master with the
   * specified slave port pair of this xtsc_arbiter.
   *
   * @param     master          The xtsc_master to connect with this xtsc_arbiter.
   *
   * @param     port_num        This specifies the slave port pair of this xtsc_arbiter
   *                            that the specified xtsc_master will be connected with. 
   *                            port_num must be in the range of 0 to this
   *                            xtsc_arbiter's "num_masters" parameter minus 1.
   */
  void connect(xtsc_master& master, xtsc::u32 port_num);


  /**
   * Connect an upstream xtsc_memory_trace with this xtsc_arbiter.
   *
   * This method connects the specified master port pair of the specified upstream
   * xtsc_memory_trace with the specified slave port pair of this xtsc_arbiter.
   *
   * @param     memory_trace    The upstream xtsc_memory_trace to connect with.
   *
   * @param     trace_port      The master port pair of the upstream xtsc_memory_trace
   *                            to connect with this xtsc_arbiter.  trace_port must be
   *                            in the range of 0 to the upstream xtsc_memory_trace's
   *                            "num_ports" parameter minus 1.
   *
   * @param     arbiter_port    The slave port pair of this xtsc_arbiter to connect the
   *                            xtsc_memory_trace with.  arbiter_port must be in the
   *                            range of 0 to this xtsc_arbiter's "num_masters"
   *                            parameter minus 1.
   */
  void connect(xtsc_memory_trace& memory_trace, xtsc::u32 trace_port, xtsc::u32 arbiter_port);


  /**
   * Connect an upstream xtsc_pin2tlm_memory_transactor with this xtsc_arbiter.
   *
   * This method connects the specified master port pair of the specified upstream 
   * xtsc_pin2tlm_memory_transactor with the specified slave port pair of this
   * xtsc_arbiter.
   *
   * @param     pin2tlm         The upstream xtsc_pin2tlm_memory_transactor to connect
   *                            with this xtsc_arbiter.
   *
   * @param     tran_port       The xtsc_pin2tlm_memory_transactor master port pair to
   *                            connect with this xtsc_arbiter.  tran_port must be in
   *                            the range of 0 to the xtsc_pin2tlm_memory_transactor's
   *                            "num_ports" parameter minus 1.
   *
   * @param     arbiter_port    The slave port pair of this xtsc_arbiter to connect with
   *                            the xtsc_pin2tlm_memory_transactor.  arbiter_port must be
   *                            in the range of 0 to this xtsc_arbiter's "num_masters"
   *                            parameter minus 1.
   */
  void connect(xtsc_pin2tlm_memory_transactor& pin2tlm, xtsc::u32 tran_port, xtsc::u32 arbiter_port);


  /**
   * Connect an upstream xtsc_router with this xtsc_arbiter.
   *
   * This method connects the specified master port pair of the specified upstream
   * xtsc_router with the specified slave port pair of this xtsc_arbiter.
   *
   * @param     router          The upstream xtsc_router to connect with this
   *                            xtsc_arbiter.
   *
   * @param     router_port     The master port pair of the upstream xtsc_router to
   *                            connect with this xtsc_arbiter.  router_port must be in
   *                            the range of 0 to the upstream xtsc_router's
   *                            "num_slaves" parameter minus 1.
   *
   * @param     arbiter_port    The slave port pair of this xtsc_arbiter to connect the
   *                            xtsc_router with.  arbiter_port must be in the range of
   *                            0 to this xtsc_arbiter's "num_masters" parameter minus 1.
   */
  void connect(xtsc_router& router, xtsc::u32 router_port, xtsc::u32 arbiter_port);



  /// Get the TextLogger for this component (e.g. to adjust its log level)
  log4xtensa::TextLogger& get_text_logger() { return m_text; }


  /// Get the BinaryLogger for this component (e.g. to adjust its log level)
  log4xtensa::BinaryLogger& get_binary_logger() { return m_binary; }


protected:

  class req_rsp_info;
  class response_info;
  class request_info;


  /// Translate fail_percentage into terms of maximum random value
  void compute_let_through();


  /// Compute a pseudo-random sequence based on George Marsaglia's multiply-with-carry method.
  xtsc::u32 random();


  /// Get the next available route ID
  xtsc::u32 get_available_route_id();


  /// PWC: Convert a request as required for a narrow/wider downstream PIF
  void convert_request(request_info*& p_request_info, xtsc::u32 master_byte_width, xtsc::u32 port_num);


  /**
   * PWC: Convert a response as required for a narrow/wider upstream PIF.
   * @return true if this was final last transfer.
   */
  bool convert_response(response_info*& p_response_info, xtsc::u32 master_byte_width, req_rsp_info*& p_req_rsp_info, bool last_xfer, bool wrap_active);


  /// PWC: Throw an exception if unaligned BLOCK_READ request is received (CWF)
  void confirm_alignment(xtsc::u32 total_bytes, xtsc::xtsc_address address8, xtsc::xtsc_request &request);

  /// Each memory request might be split into multiple requests when (master_byte_width > slave_byte_width)
  struct split_request {
    xtsc::xtsc_address address;
    unsigned int pre_wrap_cnt;
    unsigned int post_wrap_cnt;
  };

  /// Implementation of xtsc_request_if.
  class xtsc_request_if_impl : virtual public xtsc::xtsc_request_if, public sc_core::sc_object {
  public:

    /**
     * Constructor.
     * @param   arbiter     A reference to the owning xtsc_arbiter object.
     * @param   port_num    The port number that this object serves.
     */
    xtsc_request_if_impl(const char *object_name, xtsc_arbiter& arbiter, xtsc::u32 port_num) :
      sc_object         (object_name),
      m_arbiter         (arbiter),
      m_p_port          (0),
      m_port_num        (port_num)
    {}

    /// @see xtsc::xtsc_request_if
    virtual void nb_request(const xtsc::xtsc_request& request);

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
    virtual void nb_load_retired(xtsc::xtsc_address address8);

    /// @see xtsc::xtsc_request_if
    virtual void nb_retire_flush();

    /// @see xtsc::xtsc_request_if
    virtual void nb_lock(bool lock);


  protected:

    /// SystemC callback when something binds to us
    virtual void register_port(sc_core::sc_port_base& port, const char *if_typename);

    xtsc_arbiter&               m_arbiter;      ///< Our xtsc_arbiter object
    sc_core::sc_port_base      *m_p_port;       ///< Port that is bound to us
    xtsc::u32                   m_port_num;     ///< Our port number
  };



  /// Implementation of xtsc_respond_if.
  class xtsc_respond_if_impl : public xtsc::xtsc_respond_if, public sc_core::sc_object {
  public:

    /// Constructor
    xtsc_respond_if_impl(const char *object_name, xtsc_arbiter& arbiter) :
      sc_object (object_name),
      m_arbiter (arbiter),
      m_p_port  (0)
    {}

    /// @see xtsc::xtsc_respond_if
    bool nb_respond(const xtsc::xtsc_response& response);

  protected:

    /// SystemC callback when something binds to us
    virtual void register_port(sc_core::sc_port_base& port, const char *if_typename);

    xtsc_arbiter&               m_arbiter;      ///< Our xtsc_arbiter object
    sc_core::sc_port_base      *m_p_port;       ///< Port that is bound to us
  };


  /// Information about each request
  class request_info {
  public:
    /// Constructor
    request_info(const xtsc::xtsc_request& request, xtsc::u32 port_num) :
      m_request         (request),
      m_port_num        (port_num),
      m_time_stamp      (sc_core::sc_time_stamp())
    {}
    xtsc::xtsc_request  m_request;              ///< Our copy of the request
    xtsc::u32           m_port_num;             ///< Port request came in on
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
    xtsc::xtsc_address  m_block_write_address;          ///< Keep track of next address to be used for downstream requests
    bool                m_responses_sent;               ///< To detect conflicting error responses from multi-sets
    bool                m_single_rsp_error_received;    ///< True if RSP_ADDRESS_ERROR|RSP_ADDRESS_DATA_ERROR received
    request_info       *m_p_nascent_request;            ///< Hold the downstream request being built from multiple BLOCK_WRITE
    response_info      *m_p_nascent_response;           ///< Hold the upstrearm response being built
  };


  /// Information from "arbitration_policy" for arbitrate_policy()
  class port_policy_info {
  public:
    port_policy_info() { memset(this, 0, sizeof(port_policy_info)); }
    xtsc::u32           m_start_priority;               ///< From <StartPriority> in <PortNPolicy> in "arbitration_policy"
    xtsc::u32           m_end_priority;                 ///< From <EndPriority>   in <PortNPolicy> in "arbitration_policy"
    xtsc::u32           m_decrement;                    ///< From <Decrement>     in <PortNPolicy> in "arbitration_policy"
    xtsc::u32           m_current_priority;             ///< The current priority of this port
  };


  /// nb_request helper when m_immediate_timing is true
  void do_request_immediate_timing(xtsc::u32 port_num, const xtsc::xtsc_request& request);

  /// nb_request helper when m_immediate_timing is false
  void do_request(xtsc::u32 port_num, const xtsc::xtsc_request& request);

  /// Handle request
  void handle_request(request_info*& p_active_request_info, xtsc::u32 port_num, const char *caller);

  /// Common routine to nacc all remaining requests when operating with m_one_at_a_time true
  void nacc_remaining_requests();

  /// Handle incoming requests from multiple masters at the correct time
  void arbiter_thread(void);

  /// Handle incoming requests from multiple masters at the correct time when "apb" is true
  void arbiter_apb_thread(void);

  /// PWC: Handle incoming requests from multiple masters at the correct time
  virtual void arbiter_pwc_thread(void);

  /*
   * Handle incoming requests from multiple masters at the correct time for the special
   * cases of "xfer_en_port" or "external_cbox" being set to other than their default
   * values.
   */
  void arbiter_special_thread(void);

  /// Align clock phase that the request is sent downstream to the clock phase when it was received
  void align_request_phase_thread(void);

  /// Handle responses from single slave at the correct time
  void response_thread(void);

  /// PWC: Handle responses from single slave at the correct time
  void response_pwc_thread(void);

  /**
   * Given a response transaction, this method determines which port to use to
   * forward the response back to the upstream module that sent the original request.
   */
  xtsc::u32 get_port_from_response(xtsc::xtsc_response& response);

  /**
   * This method updates the route ID in the request with the bits of the 
   * specified port number so the the response derived from the request
   * will be able to get back to the upstream module that sent xtsc_arbiter
   * this request.
   */
  void add_route_id_bits(xtsc::xtsc_request& request, xtsc::u32 port_num);

  /// Get a new request_info (from the pool)
  request_info *new_request_info(xtsc::u32 port_num, const xtsc::xtsc_request& request);

  /// Copy a new request_info (using the pool)
  request_info *copy_request_info(const request_info& info);

  /// Delete an request_info (return it to the pool)
  void delete_request_info(request_info*& p_request_info);

  /// Get a new response_info (from the pool)
  response_info *new_response_info(const xtsc::xtsc_response& response);

  /// Get a new response_info (from the pool)
  response_info *new_response_info(const xtsc::xtsc_request& request);

  /// Delete an response_info (return it to the pool)
  void delete_response_info(response_info*& p_response_info);

  /// Apply address translation if applicable
  xtsc::xtsc_address translate(xtsc::u32 port_num, xtsc::xtsc_address address8);

  /// Get a new req_rsp_info (from the pool)
  req_rsp_info *new_req_rsp_info(request_info *first_request_info);

  /// Delete an req_rsp_info (return it to the pool)
  void delete_req_rsp_info(req_rsp_info*& p_req_rsp_info);

  /// Common method to compute/re-compute time delays
  void compute_delays();


  /// Reset internal fifos
  void reset_fifos();

  
  xtsc_request_if_impl                  **m_request_impl;               ///<  m_request_exports bind to these
  xtsc_respond_if_impl                    m_respond_impl;               ///<  m_respond_export binds to this

  std::deque<request_info*>             **m_request_deques;             ///<  Buffer requests from multiple masters in peekable deque's
  sc_core::sc_fifo<int>                 **m_request_fifos;              ///<  Use sc_fifo to ensure determinancy
  sc_core::sc_fifo<response_info*>        m_response_fifo;              ///<  Buffer responses from single slave
  sc_core::sc_fifo<request_info*>        *m_phase_delay_fifo;           ///<  Buffer requests being delayed to align their phase
  request_info                           *m_p_apb_request_info;         ///<  Active APB request

  xtsc_arbiter_parms                      m_arbiter_parms;              ///<  Copy of xtsc_arbiter_parms
  bool                                    m_apb;                        ///<  See "apb" parameter.
  bool                                    m_is_pwc;                     ///<  True if acting as a PIF width converter
  bool                                    m_use_block_requests;         ///<  PWC: From "use_block_requests" parameter
  std::vector<xtsc::u32>                  m_master_byte_widths;         ///<  PWC: From "master_byte_widths" parameter
  xtsc::u32                               m_slave_byte_width;           ///<  PWC: From "slave_byte_width" parameter
  static const xtsc::u8                   m_num_slots = 64;             ///<  PWC: Number of Request ID's (2^6=64)
  xtsc::u8                                m_next_slot;                  ///<  PWC: Next slot to test for availability
  xtsc::u64                               m_pending_request_tag;        ///<  PWC: New ID of pending multi-request (when != m_num_slots)
  request_info                           *m_requests[8];                ///<  PWC: List of requests to be sent downstream
  response_info                          *m_responses[8];               ///<  PWC: List of responses to be sent back upstream
  std::map<xtsc::u64, split_request>      m_issued_split_requests;	///<  PWC: List of split requests sent to the memory 
  std::map<xtsc::u64, req_rsp_info*>      m_req_rsp_table;              ///<  PWC: A map of outstanding requests indexed by request tag!

  xtsc::u32                               m_num_masters;                ///<  The number of slave port pairs for masters to connect to
  bool                                    m_one_at_a_time;              ///<  True if arbiter will only accept one request at a time
  bool                                    m_align_request_phase;        ///<  See "align_request_phase" parameter.
  bool                                    m_check_route_id_bits;        ///<  See "check_route_id_bits" parameter.
  xtsc::u32                               m_route_id_bits_mask;         ///<  Our bit-field in the request route ID
  xtsc::u32                               m_route_id_bits_clear;        ///<  All bits 1 except our bit-field in the request route ID
  xtsc::u32                               m_route_id_bits_shift;        ///<  Offset to our bit-field in request route ID
  xtsc::u32                               m_num_route_ids;              ///<  See "num_route_ids" in xtsc_arbiter_parms
  xtsc::u32                               m_route_ids_used;             ///<  Keep track of how many route ID's are in use
  xtsc::u32                               m_next_route_id;              ///<  Next free entry in m_routing_table (or 0xFFFFFFFF if none)
  xtsc::u32                             (*m_routing_table)[2];          ///<  [i][0]=>port, [i][1]=>Upstream route ID
  xtsc::u32                              *m_downstream_route_id;        ///<  Keep track for multi-xfer reqs: RCW|BLOCK_WRITE|BURST_WRITE
  bool                                    m_do_translation;             ///<  Indicates address translations may apply
  bool                                    m_waiting_for_nacc;           ///<  True if waiting for RSP_NACC from slave
  bool                                    m_request_got_nacc;           ///<  True if active request got RSP_NACC from slave
  xtsc::u32                               m_token;                      ///<  The port number which has the token
  bool                                    m_lock;                       ///<  Lock if non-last_transfer 

  bool                                    m_read_only;                  ///<  From "read_only" parameter
  bool                                    m_write_only;                 ///<  From "write_only" parameter
  bool                                    m_log_peek_poke;              ///<  From "log_peek_poke" parameter

  sc_core::sc_time                        m_clock_period;               ///<  This arbiter's clock period

  sc_core::sc_time                        m_arbitration_phase;          ///<  Clock phase arbitration occurs
  sc_core::sc_time                        m_arbitration_phase_plus_one; ///<  Clock phase arbitration occurs plus one clock period
  sc_core::sc_time                        m_time_resolution;            ///<  SystemC time resolution
  xtsc::u64                               m_clock_period_value;         ///<  Clock period as u64
  bool                                    m_has_posedge_offset;         ///<  True if m_posedge_offset is non-zero
  sc_core::sc_time                        m_posedge_offset;             ///<  From "posedge_offset" parameter
  xtsc::u64                               m_posedge_offset_value;       ///<  m_posedge_offset as u64

  bool                                    m_dram_lock;                  ///<  See "dram_lock" in xtsc_arbiter_parms
  std::vector<bool>                       m_dram_locks;                 ///<  Current value from nb_lock().  Reset to false.
  bool                                    m_external_cbox;              ///<  See "external_cbox" in xtsc_arbiter_parms
  xtsc::u32                               m_xfer_en_port;               ///<  See "xfer_en_port" in xtsc_arbiter_parms

  bool                                    m_delay_from_receipt;         ///<  True if delay starts upon request receipt
  bool                                    m_immediate_timing;           ///<  True if no delay (not even a delta cycle)
  sc_core::sc_time                        m_last_request_time_stamp;    ///<  Time last request was sent out
  sc_core::sc_time                        m_last_response_time_stamp;   ///<  Time last response was sent out

  sc_core::sc_time                        m_recovery_time;              ///<  See "recovery_time" in xtsc_arbiter_parms
  sc_core::sc_time                        m_request_delay;              ///<  See "request_delay" in xtsc_arbiter_parms
  sc_core::sc_time                        m_nacc_wait_time;             ///<  See "nacc_wait_time" in xtsc_arbiter_parms
  sc_core::sc_time                        m_response_delay;             ///<  See "response_delay" in xtsc_arbiter_parms
  sc_core::sc_time                        m_response_repeat;            ///<  See "response_repeat" in xtsc_arbiter_parms

  xtsc::u32                               m_fail_port_mask;             ///<  See "fail_port_mask" in xtsc_arbiter_parms
  xtsc::u32                               m_fail_percentage;            ///<  See "fail_percentage" in xtsc_arbiter_parms
  xtsc::u32                               m_fail_seed;                  ///<  See "fail_seed" in xtsc_arbiter_parms.
  xtsc::u32                               m_z;                          ///<  For Marsaglia's multipy-with-carry PRNG.
  xtsc::u32                               m_w;                          ///<  For Marsaglia's multipy-with-carry PRNG.
  xtsc::u32                               m_let_through;                ///<  Random number gating threshold based on fail percentage.

  std::string                             m_arbitration_policy;         ///<  See "arbitration_policy" in xtsc_arbiter_parms
  std::vector<port_policy_info*>          m_port_policy_table;          ///<  From parsing "arbitration_policy"
  xtsc::u32                              *m_ports_with_requests;        ///<  For arbitrate_policy() method

  std::string                             m_lock_port_groups;           ///<  See "lock_port_groups" in xtsc_arbiter_parms
  bool                                    m_use_lock_port_groups;       ///<  True if "lock_port_groups" was specified
  std::vector<std::vector<xtsc::u32>*>    m_lock_port_group_table;      ///<  Lock port group each port belongs to

  sc_core::sc_event                       m_arbiter_thread_event;       ///<  To notify arbiter_thread
  sc_core::sc_event                       m_response_thread_event;      ///<  To notify response_thread
  sc_core::sc_event                       m_apb_response_done_event;    ///<  To notify arbiter_apb_thread that request/response is completed
  sc_core::sc_event                       m_align_request_phase_thread_event;
                                                                        ///<  To notify align_request_phase_thread

  std::vector<req_rsp_info*>              m_req_rsp_info_pool;          ///<  Maintain a pool of req_rsp_info to improve performance
  std::vector<request_info*>              m_request_pool;               ///<  Maintain a pool of requests to improve performance
  std::vector<response_info*>             m_response_pool;              ///<  Maintain a pool of responses to improve performance

  std::vector<std::vector<xtsc::xtsc_address_range_entry*>*>
                                          m_translation_tables;         ///<  One table of address translations for each master

  std::vector<sc_core::sc_process_handle> m_process_handles;            ///<  For reset 

  bool                                    m_profile_buffers;            ///<  See "profile_buffers" in xtsc_arbiter_parms 
  xtsc::u32                              *m_max_num_requests;           ///<  The maximum available items in request_fifos
  sc_core::sc_time                       *m_max_num_requests_timestamp; ///<  Time when the max request buffer happened 
  xtsc::u64                              *m_max_num_requests_tag;       ///<  Tag of max buffered items in request_fifos 
  xtsc::u32                               m_max_num_responses;          ///<  The maximum available items in response_fifo
  sc_core::sc_time                        m_max_num_responses_timestamp;///<  Time when the max response buffer happened
  xtsc::u64                               m_max_num_responses_tag;      ///<  Tag of max buffered items in response_fifo

  log4xtensa::TextLogger&                 m_text;                       ///<  Text logger
  log4xtensa::BinaryLogger&               m_binary;                     ///<  Binary logger
  bool                                    m_log_data_binary;            ///<  True if transaction data should be logged by m_binary

};



}  // namespace xtsc_component 



#endif  // _XTSC_ARBITER_H_
