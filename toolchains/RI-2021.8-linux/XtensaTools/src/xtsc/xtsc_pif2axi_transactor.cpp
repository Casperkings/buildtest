// Copyright (c) 2006-2018 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of
// Cadence Design Systems, Inc.  They may be adapted and modified by bona fide
// purchasers for internal use, but neither the original nor any adapted
// or modified version may be disclosed or distributed to third parties
// in any manner, medium, or form, in whole or in part, without the prior
// written consent of Cadence Design Systems, Inc.


/*
 *                                   Theory of operation
 *
 *
 * This transactor converts Xtensa PIF transactions to Xtensa Amba AXI transactions : AXI/APB. 
 * Basically this means it converts the xtsc_request_if API's NON_AXI READ,BLOCK_READ,BURST_READ 
 * and WRITE,WRITE_BLOCK,WRITE_BURST to AXI BURST_READ and BURST_WRITE transactions. If the 
 * param 'memory_interface' is configured as iDMA0, AXI INCR transactions are generated respecting
 * 4K boundary. While doing the conversion its sends out AXI BURST_READ requests on read_request_ports 
 * and AXI BURST_WRITEs on write_request_ports. On response path, it converts the  xtsc_respond_if API's 
 * on the respond path back to PIF responses. While sending the response back to PIF interface its 
 * arbitrates between the READ and WRITE responses assigning read response priority in the beginning. 
 * The WRITE responses can be held back to avoid interleaved responses by using interleave_responses 
 * parameter. The number of outstanding AXI requests can be controlled with 'max_outstanding' param.
 *
 * The transactor also supports conversion of PIF transactions having different pif interface 
 * width/transaction byte size into AXI transactions of different interface width and transaction byte 
 * size(AxSIZE).
 *
 * The transactor adds latencies to the read and write request paths, which can be configured through
 * read_request_delay, write_request_delay, response_delay parameters
 *
 * Both read and write PIF requests have respective fifo buffers which can be configured for depth 
 * using read_request_fifo_depth and write_request_fifo_depth parameters.
 * 
 * The xtsc_request_if API's are : 
 * nb_request
 * nb_fast_access
 * nb_peek
 * nb_poke
 *
 * The xtsc_respond_if API's are:
 * nb_respond
 * 
 * On the request path the transaction handling sequence is:
 *       nb_request, [nacc_nb_TYPE_request_thread], TYPE_request_thread, do_FUNCTION, 
 *       convert_to_narrow_burst_TYPE, burst_TYPE_worker--> 
 *    -->nb_respond, nb_respond, arbitrate_response_thread, send_response
 * 
 *    where, TYPE is either read or write
 *           do_FUNCTION is do_burst_read, do_burst_write and do_exclusive
 *
 * * nb_request
 * In nb_request, a copy of the xtsc_request is made and put into m_TYPE_request_fifo if the fifo is 
 * not already full. If the Fifos are full respective nacc_nb_TYPE_request_thread is notified to check 
 * for the possibility of issuing RSP_NACC in the same timestamp (but in different delta) or 
 * subsequently push the request into FIFO if fifo became available in one of the previous delta cycles.
 *
 * * TYPE_request_thread
 * When request_thread wakes up (or finishes the previous request), it gets the next request out of
 * m_TYPE_request_fifo and calls do_FUNCTION depending upon the request type.
 *
 * * do_FUNCTION
 * The do_TYPE methods are: 
 *       do_burst_read
 *       do_exclusive              (convert PIF RCW to AXI Exclusive accesses)
 *       do_burst_write
 *
 * The do_FUNCTION is a time consuming blocking function, which returns after PIF transaction has been 
 * issues succefully to the AXI slave interface. It does some checks and set xtsc_request fields as 
 * appropriate for the AXI request. If pif_byte_width is more than axi_byte_width, it also calls 
 * convert_to_narrow_burst_TYPE function to generate AXI BURST_TYPE transactions of size axi_byte_width. 
 * It then calls burst_TYPE_worker which calls nb_request to send the each AXI transaction to the Xtensa 
 * AXI target through relevant downstream AXI port. 
 *
 * * do_exclusive
 * RCW is a conditional write synchronization mechanism for inter-processor or inter-process 
 * communication by providing mutually-exclusive access to data. So unlike AXI locked accesses, where 
 * the slave is locked by interconnect for that master and written unconditionally, AXI exclusive 
 * accesses are more related to PIF RCW transactions. In exclusive lock access, the lock is established
 * in the first phase and write in the second phase happens only if the memory address has not been 
 * updated during the meantime. Hence RCW is implemented as AXI exlusive access by the transactor. This 
 * requires the slave to support exclusive access by for example with the presence of an exclusive 
 * monitor.
 * Theory of operation of RCW : 
 *     First phase of RCW : save the rcw compare data and send out AXI Exclusive READ Access
 *     Second Phase of RCW: 1)If Exclusive READ response was EXOKAY, following are possibilities :. 
 *                           1.1) The read data does not match rcw compare data: the transactor 
 *                                returns the value read from memory back to the processor and the 
 *                                transactions ends. The subsequent exclusive store does not happen 
 *                                in this case.
 *                           1.1) The read data matches rcw compare data: Excl store is issued.
 *                                If response of Excl WRITE is EXOKAY, send back the RCW response to 
 *                                PIF master with same rcw compare data, that was sent in the first 
 *                                phase (A succesful RCW has write happening and response containing 
 *                                the same data as initial rcw compare data) 
 *                           1.3) If response of Excl WRITE is OKAY, return the response of RCW to 
 *                                the PIF master with complement of the rcw compare value data as 
 *                                part of response.
 *                          2)If Exclusive READ response was OKAY, it indicates that the slave does not 
 *                            support exclusive operations. In this case, an address error is sent back
 *                            to the processor. 
 *                            (Obv, In this case no write was initaited to the slave)
 *
 *
 * * nb_respond
 * nb_respond is issued by the downstream AXI target in response to the requests issued. It does some 
 * checks and identifies the PIF request to which this response belongs to. After the expected number of 
 * AXI responses have been received it creates PIF respons and adds it to the m_rd_response_peq and notifies
 * arbitrate_response_thread.
 *
 * * arbitrate_response_thread
 * When arbitrate_response_thread wakes up (or finishes the previous response), it gets the next transaction out
 * of m_rd_response_peq and calls send_response() the appropriate number of times.
 *
 * * send_response
 * send response back to the PIF Initiator. Keeps trying to send the response on next clock edge if 
 * RSP_NACC is received from the Initiator.
 *
 * * nb_fast_access:
 * If the upstream Xtensa TLM subsystem includes an xtsc_core which wants fast access (TurboXim) to
 * the downstream OSCI TLM2 subsystem, it calls this transactor's nb_fast_access method. which 
 * subsequently calls the downstream Xtensa AXI subsystem using nb_fast_access to attempt to give 
 * TurboXim raw fast access.
 *
 * * nb_peek, nb_poke:
 * If the upstream Xtensa TLM subsystem wants to peek/poke the downstream Xtensa AXI subsystem, it
 * calls this transactor's nb_peek or nb_poke method.  These methods both call the nb_peek or nb_poke 
 * method in the downstream Xtensa AXI subsystem 
 *
 *
 * Key Data structures involved :
 *
 * req_rsp_info : Structure to keep a PIF request associated with multiple aspects of its response.
 * Not all of the fields are relevant for each request. 
 *
 * m_tag_to_req_rsp_info_map : std::map of request tag to req_rsp_info
 *
 * m_axi_tag_from_pif_tag_map : std::map of tags of axi request that are created from inbound pif requests.
 *
 * m_tag_to_axi_request_map  : std::map to keep track of all AXI requests associated with a PIF request tag.
 *
 * m_pif_request_order_dq    : std::deque to keep track of the order of incoming PIF requests
 *
 */

#include <cerrno>
#include <algorithm>
#include <cmath>
#include <ostream>
#include <string>
#include <xtsc/xtsc_pif2axi_transactor.h>

using namespace std;
using namespace sc_core;
using namespace log4xtensa;
using namespace xtsc;

#define BIT_11            0x800
#define AXI_ADDR_BOUNDARY 4096         ///< AXI 4K burst address boundary


xtsc_component::xtsc_pif2axi_transactor_parms:: xtsc_pif2axi_transactor_parms(const xtsc_core&  core,
                                                                              const char *memory_interface, 
                                                                              u32 num_ports) 
{
  const xtsc_core_parms &core_parms = core.get_parms();
  if (!core_parms.get_bool("IsNX")) 
  {
    xtsc_core::memory_port mem_port = xtsc_core::get_memory_port(memory_interface, &core); 
 
    if (!core.has_memory_port(mem_port)) {
      ostringstream oss;
      oss << __FUNCTION__ << ": core '" << core.name() << "' has no " << memory_interface << " memory interface.";
      throw xtsc_exception(oss.str());
    }   

    if (!num_ports) {
      // Normally be single-ported ...
      num_ports = 1;
      // ; however, if this core interface is multi-ported (e.g. has banks or has multiple LD/ST units without CBox) AND ...
      u32 mpc = core.get_multi_port_count(mem_port);

      // Not clear about the need to find a connected port
      if (mpc > 1) {  
        // this is the first port (port 0) of the interface AND ...
        if (xtsc_core::is_multi_port_zero(mem_port)) {
          // if all the other ports of the interface appear to not be connected 
          // (this is not reliable--they might be delay bound), then ...
          bool found_a_connected_port = false;
          for (u32 n=1; n<mpc; ++n) {
            xtsc_core::memory_port mem_port_n = core.get_nth_multi_port(mem_port, n);
            if (core.get_request_port(mem_port_n).get_interface()) {
              found_a_connected_port = true;
              break;
            }
          }
          if (!found_a_connected_port) {
            // infer multi-ported
            num_ports = mpc;
          }
        }
      }
    }
 
    const xtsc_core_parms& core_parms = core.get_parms();
    u32 pif_byte_width = core.get_memory_byte_width(mem_port);

    init(memory_interface, pif_byte_width, num_ports);

    //Overwrite clock period from core's clock period
    set("clock_period", core_parms.get_u32("SimClockFactor")*xtsc_get_system_clock_factor());

  } 
  else {
    //Allow xtsc_pif2axi_transactor_parms to be created for NX core configuration;
    //But it still can't be used with NX core. For using with xtsc_master, for instance, "memory_interface" needs 
    //to be set correctly
    init();
  }
 
}



xtsc_component::xtsc_pif2axi_transactor::xtsc_pif2axi_transactor(
  sc_module_name                       module_name,
  const xtsc_pif2axi_transactor_parms& parms
  ) :
  sc_module                            (module_name),
  xtsc_module                          (*(sc_module*)this),
  m_request_exports                    (NULL),
  m_respond_ports                      (NULL),
  m_read_request_ports                 (NULL),
  m_read_respond_exports               (NULL),
  m_write_request_ports                (NULL),
  m_write_respond_exports              (NULL),
  m_xtsc_request_if_impl               (NULL),
  m_xtsc_read_respond_if_impl          (NULL),
  m_xtsc_write_respond_if_impl         (NULL),
  m_memory_interface_str               (parms.get_c_str        ("memory_interface")),
  m_pif_width8                         (parms.get_non_zero_u32 ("pif_byte_width")),
  m_num_ports                          (parms.get_non_zero_u32 ("num_ports")),
  m_read_request_fifo_depth            (parms.get_non_zero_u32 ("read_request_fifo_depth")),
  m_write_request_fifo_depth           (parms.get_non_zero_u32 ("write_request_fifo_depth")),
  m_read_request_delay                 (parms.get_u32          ("read_request_delay")),
  m_write_request_delay                (parms.get_u32          ("write_request_delay")),
  m_read_response_delay                (parms.get_u32          ("read_response_delay")),
  m_write_response_delay               (parms.get_u32          ("write_response_delay")),
  m_max_outstanding                    (parms.get_u32          ("max_outstanding")),
  m_allow_axi_wrap                     (parms.get_bool         ("allow_axi_wrap")),
  m_maintain_order                     (parms.get_bool         ("maintain_order")),
  m_wait_on_outstanding_write          (parms.get_bool         ("wait_on_outstanding_write")),
  m_interleave_responses               (parms.get_bool         ("interleave_responses")),
  m_outstanding_reads                  (NULL),
  m_outstanding_writes                 (NULL),
  m_tag_axi_write                      (NULL),
  m_read_request_thread_event          (NULL),
  m_write_request_thread_event         (NULL),
  m_arbitrate_response_thread_event    (NULL),
  m_nacc_nb_write_request_thread_event (NULL),
  m_nacc_nb_read_request_thread_event  (NULL),
  m_outstanding_write_event            (NULL),
  m_read_request_fifo                  (NULL),
  m_write_request_fifo                 (NULL),
  m_rd_response_peq                    (NULL),
  m_wr_response_peq                    (NULL),
  m_nacc_nb_write_request              (NULL),
  m_nacc_nb_read_request               (NULL),
  m_tag_to_axi_request_map             (NULL),
  m_tag_to_req_rsp_info_map            (NULL),
  m_axi_tag_from_pif_tag_map           (NULL),
  m_pif_request_order_dq               (NULL),
  m_request_count                      (0),
  m_req_rsp_info_count                 (0),
  m_time_resolution                    (sc_get_time_resolution()),
  m_text                               (TextLogger::getInstance(name()))
{
  string axi_width8       = parms.get_c_str ("axi_byte_width");
  m_axi_width8            = (axi_width8 == "") ? m_pif_width8 : xtsc_strtou32(axi_width8);

  string mem_if_str       = m_memory_interface_str;
  std::transform(mem_if_str.begin(), mem_if_str.end(), mem_if_str.begin(), ::toupper);
  m_interface_is_idma0 = (mem_if_str == "IDMA0") ? true : false;

  if ( (mem_if_str != "PIF") && (mem_if_str != "IDMA0") ) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': Invalid \"memory_interface\" parameter value (" << mem_if_str << ")." <<  
                                       " Expected PIF|IDMA0 (case-insensitive).";
    throw xtsc_exception(oss.str());
  }

  if ((m_pif_width8 !=  4) && (m_pif_width8 !=  8) && (m_pif_width8 != 16) && (m_pif_width8 != 32) ) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': Invalid \"pif_byte_width\" parameter value (" << m_pif_width8 << "). Expected 4|8|16|32.";
    throw xtsc_exception(oss.str());
  }

  if ((m_pif_width8 == 32) && (mem_if_str != "IDMA0")) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': Invalid \"pif_byte_width\" parameter value (" << m_pif_width8 << ") when connected to a non IDMA0 port." <<
                                       " Expected for a non IDMA0 PIF port : 4|8|16.  Did you forget to set 'memory_interface' to IDMA0 ?" ;
    //throw xtsc_exception(oss.str());
    XTSC_WARN(m_text, oss.str());
  }

  if ((m_axi_width8 !=  4) && (m_axi_width8 !=  8) && (m_axi_width8 != 16) && (m_axi_width8 != 32)) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': Invalid \"axi_byte_width\" parameter value (" << m_axi_width8 << ").  Expected 4|8|16|32.";
    throw xtsc_exception(oss.str());
  }

  if ((m_axi_width8 == 32) && (mem_if_str != "IDMA0")) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': Invalid \"axi_byte_width\" parameter value (" << m_axi_width8 << ") when connected to a non IDMA0 port." <<
                                       " Expected for a PIF port : 4|8|16.  Did you forget to set 'memory_interface' to IDMA0 ?";
    //throw xtsc_exception(oss.str());
    XTSC_WARN(m_text, oss.str());
  }

  if (m_num_ports == 0) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': Invalid \"num_ports\" parameter value (" << m_num_ports << ").  Expected non zero value.";
    throw xtsc_exception(oss.str());
  }

  // Create all the "per-port" stuff
  m_request_exports                     = new sc_export<xtsc_request_if>              *[m_num_ports];
  m_respond_ports                       = new sc_port<xtsc_respond_if>                *[m_num_ports];
  m_read_request_ports                  = new sc_port<xtsc_request_if>                *[m_num_ports];
  m_read_respond_exports                = new sc_export<xtsc_respond_if>              *[m_num_ports];
  m_write_request_ports                 = new sc_port<xtsc_request_if>                *[m_num_ports];
  m_write_respond_exports               = new sc_export<xtsc_respond_if>              *[m_num_ports];

  m_xtsc_request_if_impl                = new xtsc_request_if_impl                    *[m_num_ports];
  m_xtsc_read_respond_if_impl           = new xtsc_respond_if_impl                    *[m_num_ports];
  m_xtsc_write_respond_if_impl          = new xtsc_respond_if_impl                    *[m_num_ports];

  m_outstanding_reads                   = new xtsc::u32                                [m_num_ports];
  m_outstanding_writes                  = new xtsc::u32                                [m_num_ports];
  m_tag_axi_write                       = new xtsc::u64                                [m_num_ports];
  m_read_request_thread_event           = new sc_event                                *[m_num_ports];
  m_write_request_thread_event          = new sc_event                                *[m_num_ports];
  m_arbitrate_response_thread_event     = new sc_event                                *[m_num_ports];
  m_nacc_nb_write_request_thread_event  = new sc_event                                *[m_num_ports];
  m_nacc_nb_read_request_thread_event   = new sc_event                                *[m_num_ports];
  m_outstanding_write_event             = new sc_event                                *[m_num_ports];
  m_read_waiting_for_nacc               = new bool                                     [m_num_ports];
  m_write_waiting_for_nacc              = new bool                                     [m_num_ports];
  m_read_request_got_nacc               = new bool                                     [m_num_ports];
  m_write_request_got_nacc              = new bool                                     [m_num_ports];
  m_response_locked                     = new bool                                     [m_num_ports];
  m_excl_read_resp_rcvd                 = new bool                                     [m_num_ports];
  m_read_request_fifo                   = new std::deque<req_sched_info*>             *[m_num_ports];
  m_write_request_fifo                  = new std::deque<req_sched_info*>             *[m_num_ports];
  m_rd_response_peq                     = new peq                                     *[m_num_ports];
  m_wr_response_peq                     = new peq                                     *[m_num_ports];
  m_nacc_nb_write_request               = new xtsc_request                            *[m_num_ports];
  m_nacc_nb_read_request                = new xtsc_request                            *[m_num_ports];
  m_tag_to_axi_request_map              = new std::map<xtsc::u64, 
                                                 std::vector<xtsc::xtsc_request*> >   *[m_num_ports];
  m_tag_to_req_rsp_info_map             = new std::map<xtsc::u64, req_rsp_info*>      *[m_num_ports];
  m_axi_tag_from_pif_tag_map            = new std::map<xtsc::u64, xtsc::u64>          *[m_num_ports];
  m_pif_request_order_dq                = new std::deque<xtsc::u64>                   *[m_num_ports];

  for (u32 i=0; i<m_num_ports; ++i) {

    ostringstream oss1;
    oss1 << "m_request_exports[" << i << "]";
    m_request_exports[i] = new sc_export<xtsc_request_if>(oss1.str().c_str());
    m_bit_width_map[oss1.str()] = m_pif_width8*8;

    ostringstream oss2;
    oss2 << "m_respond_ports[" << i << "]";
    m_respond_ports[i] = new sc_port<xtsc_respond_if>(oss2.str().c_str());
    m_bit_width_map[oss2.str()] = m_pif_width8*8;

    ostringstream oss3;
    oss3 << "m_read_request_ports[" << i << "]";
    m_read_request_ports[i] = new sc_port<xtsc_request_if>(oss3.str().c_str());
    m_bit_width_map[oss3.str()] = m_axi_width8*8;

    ostringstream oss4;
    oss4 << "m_read_respond_exports[" << i << "]";
    m_read_respond_exports[i] = new sc_export<xtsc_respond_if>(oss4.str().c_str());
    m_bit_width_map[oss4.str()] = m_axi_width8*8;

    ostringstream oss5;
    oss5 << "m_write_request_ports[" << i << "]";
    m_write_request_ports[i] = new sc_port<xtsc_request_if>(oss5.str().c_str());
    m_bit_width_map[oss5.str()] = m_axi_width8*8;

    ostringstream oss6;
    oss6 << "m_write_respond_exports[" << i << "]";
    m_write_respond_exports[i] = new sc_export<xtsc_respond_if>(oss6.str().c_str());
    m_bit_width_map[oss6.str()] = m_axi_width8*8;

    ostringstream oss7;
    oss7 << "m_xtsc_read_respond_if_impl[" << i << "]";
    m_xtsc_read_respond_if_impl[i] = new xtsc_respond_if_impl(oss7.str().c_str(), *this, i, "READ");
    m_bit_width_map[oss7.str()] = m_axi_width8*8;

    ostringstream oss8;
    oss8 << "m_xtsc_write_respond_if_impl[" << i << "]";
    m_xtsc_write_respond_if_impl[i] = new xtsc_respond_if_impl(oss8.str().c_str(), *this, i, "WRITE");

    ostringstream oss9;
    oss9 << "m_xtsc_request_if_impl[" << i << "]";
    m_xtsc_request_if_impl[i] = new xtsc_request_if_impl(oss9.str().c_str(), *this, i);

    (*m_read_respond_exports[i])(*m_xtsc_read_respond_if_impl[i]);
    (*m_write_respond_exports[i])(*m_xtsc_write_respond_if_impl[i]);
    (*m_request_exports[i])(*m_xtsc_request_if_impl[i]);

    if (m_read_request_fifo_depth) {
      m_read_request_fifo[i] = new std::deque<req_sched_info*>();
    } else {
      ostringstream oss;
      oss << kind() << " '" << name() << "' Invalid \"read_request_fifo_depth\" parameter value (" << 
             m_read_request_fifo_depth << ").  Expected greather than 0.";
      throw xtsc_exception(oss.str());
    }

    if (m_write_request_fifo_depth) {
      m_write_request_fifo[i] = new std::deque<req_sched_info*>();
    } else {
      ostringstream oss;
      oss << kind() << " '" << name() << "' Invalid \"write_request_fifo_depth\" parameter value (" << 
             m_write_request_fifo_depth << ").  Expected greather than 0.";
      throw xtsc_exception(oss.str());
    }

    m_rd_response_peq[i] = new peq("rd_response_peq");
    m_wr_response_peq[i] = new peq("wr_response_peq");

    ostringstream oss10;
    oss10 << "read_request_thread_" << i;
    ostringstream oss11;
    oss11 << "write_request_thread_" << i;
    ostringstream oss12;
    oss12 << "arbitrate_response_thread_" << i;
    ostringstream oss13;
    oss13 << "nacc_nb_write_request_thread_" << i;
    ostringstream oss14;
    oss14 << "nacc_nb_read_request_thread_" << i;
    ostringstream oss15;
    oss15 << "outstanding_write_" << i;
#if IEEE_1666_SYSTEMC >= 201101L
    m_read_request_thread_event[i]          = new sc_event((oss10.str() + "event").c_str());
    m_write_request_thread_event[i]         = new sc_event((oss11.str() + "event").c_str());
    m_arbitrate_response_thread_event[i]    = new sc_event((oss12.str() + "event").c_str());
    m_nacc_nb_write_request_thread_event[i] = new sc_event((oss13.str() + "event").c_str());
    m_nacc_nb_read_request_thread_event[i]  = new sc_event((oss14.str() + "event").c_str());
    m_outstanding_write_event[i]            = new sc_event((oss15.str() + "event").c_str());
#else
    m_read_request_thread_event[i]          = new sc_event;
    xtsc_event_register(*m_read_request_thread_event[i],          oss10.str() + "_event", this);
    m_write_request_thread_event[i]         = new sc_event;
    xtsc_event_register(*m_write_request_thread_event[i],         oss11.str() + "_event", this);
    m_arbitrate_response_thread_event[i]    = new sc_event;
    xtsc_event_register(*m_arbitrate_response_thread_event[i],    oss12.str() + "_event", this);
    m_nacc_nb_write_request_thread_event[i] = new sc_event;
    xtsc_event_register(*m_nacc_nb_write_request_thread_event[i], oss13.str() + "_event", this);
    m_nacc_nb_read_request_thread_event[i]  = new sc_event;
    xtsc_event_register(*m_nacc_nb_read_request_thread_event[i],  oss14.str() + "_event", this);
    m_outstanding_write_event[i]            = new sc_event;
    xtsc_event_register(*m_outstanding_write_event[i],            oss15.str() + "_event", this);
#endif
    declare_thread_process(read_request_thread_handle,          oss10.str().c_str(), SC_CURRENT_USER_MODULE, read_request_thread);
    m_process_handles.push_back(sc_get_current_process_handle());
    declare_thread_process(write_request_thread_handle,         oss11.str().c_str(), SC_CURRENT_USER_MODULE, write_request_thread);
    m_process_handles.push_back(sc_get_current_process_handle());
    declare_thread_process(arbitrate_response_thread_handle,    oss12.str().c_str(), SC_CURRENT_USER_MODULE, arbitrate_response_thread);
    m_process_handles.push_back(sc_get_current_process_handle());
    declare_thread_process(nacc_nb_write_request_thread_handle, oss13.str().c_str(), SC_CURRENT_USER_MODULE, nacc_nb_write_request_thread);
    declare_thread_process(nacc_nb_read_request_thread_handle,  oss14.str().c_str(), SC_CURRENT_USER_MODULE, nacc_nb_read_request_thread);

    m_tag_to_axi_request_map[i]    = new std::map<xtsc::u64, std::vector<xtsc::xtsc_request*> >();
    m_tag_to_req_rsp_info_map[i]   = new std::map<xtsc::u64, req_rsp_info*>();
    m_axi_tag_from_pif_tag_map[i]  = new std::map<xtsc::u64, xtsc::u64>();
    m_pif_request_order_dq[i]      = new std::deque<xtsc::u64>();

    m_response_locked[i]           = false;
    m_excl_read_resp_rcvd[i]       = false;
    m_nacc_nb_write_request[i]     = NULL;
    m_nacc_nb_read_request[i]      = NULL;
    m_outstanding_reads[i]         = 0;         
    m_outstanding_writes[i]        = 0;         
  }


  // Get clock period 
  u32 clock_period = parms.get_u32("clock_period");
  if (clock_period == 0xFFFFFFFF) {
    m_clock_period = xtsc_get_system_clock_period();
  }
  else {
    m_clock_period = m_time_resolution * clock_period;
  }
  m_clock_period_value = m_clock_period.value();

  u32 nacc_wait_time = parms.get_u32("nacc_wait_time");
  if (nacc_wait_time == 0xFFFFFFFF) {
    m_nacc_wait_time = m_clock_period;
  }
  else {
    m_nacc_wait_time = m_time_resolution * nacc_wait_time;
    if (m_nacc_wait_time > m_clock_period) {
      ostringstream oss;
      oss << kind() << " '" << name() << "': \"nacc_wait_time\"=" << m_nacc_wait_time << " exceeds clock period of " << m_clock_period;
      throw xtsc_exception(oss.str());
    }
  }

  for (u32 i=0; i<m_num_ports; ++i) {
    ostringstream oss;
    oss.str(""); oss << "m_request_exports"         << "[" << i << "]"; m_port_types[oss.str()]   = REQUEST_EXPORT;
    oss.str(""); oss << "m_respond_ports"           << "[" << i << "]"; m_port_types[oss.str()]   = RESPOND_PORT;
    oss.str(""); oss << "m_read_request_ports"      << "[" << i << "]"; m_port_types[oss.str()]   = REQUEST_PORT;
    oss.str(""); oss << "m_read_respond_exports"    << "[" << i << "]"; m_port_types[oss.str()]   = RESPOND_EXPORT;
    oss.str(""); oss << "m_write_request_ports"     << "[" << i << "]"; m_port_types[oss.str()]   = REQUEST_PORT;
    oss.str(""); oss << "m_write_respond_exports"   << "[" << i << "]"; m_port_types[oss.str()]   = RESPOND_EXPORT;
    oss.str(""); oss << "slave_port"                << "[" << i << "]"; m_port_types[oss.str()]   = PORT_TABLE;
    oss.str(""); oss << "master_port"               << "[" << i << "]"; m_port_types[oss.str()]   = PORT_TABLE;
    oss.str(""); oss << "aximaster_rd"              << "[" << i << "]"; m_port_types[oss.str()]   = PORT_TABLE;
    oss.str(""); oss << "aximaster_wr"              << "[" << i << "]"; m_port_types[oss.str()]   = PORT_TABLE;
  }

  m_port_types["slave_ports"]      = PORT_TABLE;
  m_port_types["master_ports"]     = PORT_TABLE;
  if (m_num_ports == 1) {
    m_port_types["slave_port"]     = PORT_TABLE;
    m_port_types["master_port"]    = PORT_TABLE;
    m_port_types["aximaster_rd"]   = PORT_TABLE;
    m_port_types["aximaster_wr"]   = PORT_TABLE;
  }


  xtsc_register_command(*this, *this, "change_clock_period", 1, 1,
      "change_clock_period <ClockPeriodFactor>", 
      "Call xtsc_pif2axi_transactor::change_clock_period(<ClockPeriodFactor>)."
      "  Return previous <ClockPeriodFactor> for this device."
  );

  xtsc_register_command(*this, *this, "peek", 3, 3,
      "peek <PortNumber> <StartAddress> <NumBytes>", 
      "Peek <NumBytes> of memory starting at <StartAddress>."
  );

  xtsc_register_command(*this, *this, "poke", 3, -1,
      "poke <PortNumber> <StartAddress> <NumBytes> <Byte1> <Byte2> . . . <ByteN>", 
      "Poke <NumBytes> (=N) of memory starting at <StartAddress>."
  );

  xtsc_register_command(*this, *this, "reset", 0, 0,
      "reset", 
      "Call xtsc_pif2axi_transactor::reset()."
  );

  LogLevel ll = xtsc_get_constructor_log_level();
  XTSC_LOG(m_text, ll,        " Constructed xtsc_pif2axi_transactor_parms '" << name() << "':");
  XTSC_LOG(m_text, ll,        " allow_axi_wrap                  = "   << m_allow_axi_wrap);
  XTSC_LOG(m_text, ll,        " axi_byte_width                  = "   << m_axi_width8);
  if (clock_period == 0xFFFFFFFF) {
  XTSC_LOG(m_text, ll, hex << " clock_period                    = 0x" << clock_period << " (" << m_clock_period << ")");
  } else {
  XTSC_LOG(m_text, ll,        " clock_period                    = "   << clock_period << " (" << m_clock_period << ")");
  }
  XTSC_LOG(m_text, ll,        " interleave_responses            = "   << m_interleave_responses);
  XTSC_LOG(m_text, ll,        " maintain_order                  = "   << m_maintain_order);
  XTSC_LOG(m_text, ll,        " max_outstanding                 = "   << m_max_outstanding);
  XTSC_LOG(m_text, ll,        " memory_interface                = "   << m_memory_interface_str);
  if (nacc_wait_time == 0xFFFFFFFF) {
  XTSC_LOG(m_text, ll,        " nacc_wait_time                  = 0xFFFFFFFF => " << m_nacc_wait_time.value() << " (" << m_nacc_wait_time << ")");
  } else {
  XTSC_LOG(m_text, ll,        " nacc_wait_time                  = "   << nacc_wait_time << " (" << m_nacc_wait_time << ")");
  }  
  XTSC_LOG(m_text, ll,        " num_ports                       = "   << m_num_ports);
  XTSC_LOG(m_text, ll,        " pif_byte_width                  = "   << m_pif_width8);
  XTSC_LOG(m_text, ll,        " read_response_delay             = "   << m_read_response_delay);
  XTSC_LOG(m_text, ll,        " read_request_delay              = "   << m_read_request_delay);
  XTSC_LOG(m_text, ll,        " read_request_fifo_depth         = "   << m_read_request_fifo_depth);
  XTSC_LOG(m_text, ll,        " wait_on_outstanding_write       = "   << m_wait_on_outstanding_write);
  XTSC_LOG(m_text, ll,        " write_response_delay            = "   << m_write_response_delay);
  XTSC_LOG(m_text, ll,        " write_request_delay             = "   << m_write_request_delay);
  XTSC_LOG(m_text, ll,        " write_request_fifo_depth        = "   << m_write_request_fifo_depth);

  reset();
}



xtsc_component::xtsc_pif2axi_transactor::~xtsc_pif2axi_transactor(void) {
//Delete all requests of m_request_pool
  for(unsigned int i = 0; i < m_num_ports; i++) {
    delete m_request_exports[i];
    delete m_respond_ports[i];
    delete m_read_request_ports[i];
    delete m_read_respond_exports[i];
    delete m_write_request_ports[i];
    delete m_write_respond_exports[i];
    delete m_xtsc_read_respond_if_impl[i];
    delete m_xtsc_write_respond_if_impl[i];
    delete m_xtsc_request_if_impl[i];

    delete m_read_request_fifo[i]; 
    delete m_write_request_fifo[i]; 
    delete m_rd_response_peq[i]; 
    delete m_wr_response_peq[i]; 
    delete m_read_request_thread_event[i];
    delete m_write_request_thread_event[i];
    delete m_arbitrate_response_thread_event[i];
    delete m_outstanding_write_event[i];
    delete m_tag_to_axi_request_map[i];
    delete m_tag_to_req_rsp_info_map[i];
    delete m_axi_tag_from_pif_tag_map[i];
    delete m_pif_request_order_dq[i];
  }

  // Delete all the "m_num_ports array of pointers" stuff
  delete[] m_request_exports;
  delete[] m_respond_ports;
  delete[] m_read_request_ports;
  delete[] m_read_respond_exports;
  delete[] m_write_request_ports;
  delete[] m_write_respond_exports;
  delete[] m_xtsc_request_if_impl;
  delete[] m_xtsc_read_respond_if_impl;
  delete[] m_xtsc_write_respond_if_impl;
  delete[] m_read_request_thread_event; 
  delete[] m_outstanding_reads; 
  delete[] m_outstanding_writes; 
  delete[] m_tag_axi_write; 
  delete[] m_write_request_thread_event;
  delete[] m_arbitrate_response_thread_event;
  delete[] m_nacc_nb_write_request_thread_event;
  delete[] m_nacc_nb_read_request_thread_event;
  delete[] m_outstanding_write_event;
  delete[] m_read_waiting_for_nacc;
  delete[] m_write_waiting_for_nacc;
  delete[] m_read_request_got_nacc;
  delete[] m_write_request_got_nacc;
  delete[] m_response_locked;
  delete[] m_excl_read_resp_rcvd;
  delete[] m_read_request_fifo;     
  delete[] m_write_request_fifo;  
  delete[] m_nacc_nb_write_request; 
  delete[] m_nacc_nb_read_request; 
  delete[] m_tag_to_axi_request_map;  
  delete[] m_tag_to_req_rsp_info_map;  
  delete[] m_axi_tag_from_pif_tag_map;  
  delete[] m_pif_request_order_dq;  
  delete[] m_rd_response_peq;
  delete[] m_wr_response_peq;

  while (m_request_pool.size()) {
    delete m_request_pool.back();
    m_request_pool.pop_back();
  }
  while (m_req_rsp_info_pool.size()) {
    delete m_req_rsp_info_pool.back();
    m_req_rsp_info_pool.pop_back();
  }
}


void xtsc_component::xtsc_pif2axi_transactor::end_of_simulation(void) 
{
  LogLevel ll = log4xtensa::INFO_LOG_LEVEL;
  bool mismatch = ( m_request_pool.size() != m_request_count );
  if (mismatch) {
    ll = log4xtensa::ERROR_LOG_LEVEL;
    XTSC_LOG(m_text, ll, "Error: 1 or more mismatches between pool sizes and counts which indicates a leak: ");
  }
  XTSC_LOG(m_text, ll, m_request_pool.size() << "/" << m_request_count << " (m_request_pool/m_request_count)");

  bool mismatch_2 = ( m_req_rsp_info_pool.size() != m_req_rsp_info_count );
  if (mismatch_2) {
    ll = log4xtensa::ERROR_LOG_LEVEL;
    XTSC_LOG(m_text, ll, "Error: 1 or more mismatches between pool sizes and counts which indicates a leak: ");
  }
  XTSC_LOG(m_text, ll, m_req_rsp_info_pool.size() << "/" << m_req_rsp_info_count << 
                       " (m_req_rsp_info_pool/m_req_rsp_info_count)");

}



u32 xtsc_component::xtsc_pif2axi_transactor::get_bit_width(const string& port_name, u32 interface_num) const {
  if (interface_num == 0) {
    map<string,u32>::const_iterator i = m_bit_width_map.find(port_name);
    if (i == m_bit_width_map.end()) {
      ostringstream oss;
      oss << kind() << " \"" << name() << "\" has no port named \"" << port_name << "\"" << endl;
      throw xtsc_exception(oss.str());
    }
    return i->second;
  }
  else {
    return 0;
  }
}


sc_object *xtsc_component::xtsc_pif2axi_transactor::get_port(const string& port_name)
{
  string name_portion;
  u32    index;
  bool   indexed = xtsc_parse_port_name(port_name, name_portion, index);

  if (((m_num_ports > 1) && !indexed)              ||
      ( (name_portion != "m_request_exports")      &&
        (name_portion != "m_respond_ports")        &&
        (name_portion != "m_read_request_ports")   &&
        (name_portion != "m_read_respond_exports") &&
        (name_portion != "m_write_request_ports")  &&
        (name_portion != "m_write_respond_exports") ))
  {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\" has no port named \"" << port_name << "\"" << endl;
    throw xtsc_exception(oss.str());
  }

  if (index > m_num_ports - 1) {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\": Invalid index: \"" << port_name << "\".  Valid range: 0-" << (m_num_ports-1) << endl;
    throw xtsc_exception(oss.str());
  }

       if (name_portion == "m_request_exports")         return m_request_exports        [index];
  else if (name_portion == "m_respond_ports")           return m_respond_ports          [index];
  else if (name_portion == "m_read_request_ports")      return m_read_request_ports     [index];
  else if (name_portion == "m_read_respond_exports")    return m_read_respond_exports   [index];
  else if (name_portion == "m_write_request_ports")     return m_write_request_ports    [index];
  else if (name_portion == "m_write_respond_exports")   return m_write_respond_exports  [index];
  else {
    ostringstream oss;
    oss << __FUNCTION__ << "Program Bug in xtsc_pif2axi_transactor.cpp" << endl;
    throw xtsc_exception(oss.str());
  }
}



xtsc_port_table xtsc_component::xtsc_pif2axi_transactor::get_port_table(const string& port_table_name) const 
{
  xtsc_port_table table;
  if (port_table_name == "slave_ports") {
    if (m_num_ports == 1) {
      ostringstream oss;
      oss << "slave_port";
      table.push_back(oss.str());
    } 
    else {    
      for (u32 i=0; i<m_num_ports; ++i) {
        ostringstream oss;
        oss << "slave_port[" << i << "]";
        table.push_back(oss.str());
      }
    } 
    return table;
  }

  if (port_table_name == "master_ports") {
    if (m_num_ports == 1) {
      ostringstream oss;
      oss << "master_port";
      table.push_back(oss.str());
    } 
    else {  
      for (u32 i=0; i<m_num_ports; ++i) {
        ostringstream oss;
        oss << "master_port[" << i << "]";
        table.push_back(oss.str());
      }
    }
    return table;
  }  

  string name_portion;
  u32    index;
  bool   indexed = xtsc_parse_port_name(port_table_name, name_portion, index);

  if (((m_num_ports > 1) && !indexed) || 
      ((name_portion != "aximaster_rd") && 
       (name_portion != "aximaster_wr") && 
       (name_portion != "master_port")  && 
       (name_portion != "slave_port"))) 
  {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\" has no port table named \"" << port_table_name << "\"" << endl;
    throw xtsc_exception(oss.str());
  }
  if (index > m_num_ports - 1) {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\": Invalid index: \"" << port_table_name << "\".  Valid range: 0-" << (m_num_ports-1) << endl;
    throw xtsc_exception(oss.str());
  }

  ostringstream oss;
  if (name_portion == "master_port") {
    oss.str(""); oss << "aximaster_rd"            << "[" << index << "]"; table.push_back(oss.str());
    oss.str(""); oss << "aximaster_wr"            << "[" << index << "]"; table.push_back(oss.str());
  } 
  if (name_portion == "aximaster_rd") {
    oss.str(""); oss << "m_read_request_ports"    << "[" << index << "]"; table.push_back(oss.str());
    oss.str(""); oss << "m_read_respond_exports"  << "[" << index << "]"; table.push_back(oss.str());
  } 
  else if (name_portion == "aximaster_wr") {
    oss.str(""); oss << "m_write_request_ports"   << "[" << index << "]"; table.push_back(oss.str());
    oss.str(""); oss << "m_write_respond_exports" << "[" << index << "]"; table.push_back(oss.str());
  }
  else if (name_portion == "slave_port") {
    oss.str(""); oss << "m_request_exports"       << "[" << index << "]"; table.push_back(oss.str());
    oss.str(""); oss << "m_respond_ports"         << "[" << index << "]"; table.push_back(oss.str());
  }

  return table;
}


bool xtsc_component::xtsc_pif2axi_transactor::prepare_to_switch_sim_mode(xtsc_sim_mode mode) {

  string str = (mode==xtsc::XTSC_CYCLE_ACCURATE) ? "XTSC_CYCLE_ACCURATE" : "XTSC_FUNCTIONAL";
  XTSC_DEBUG(m_text, "prepare_to_switch_sim_mode with mode=" << str << ")");

  m_mode_switch_pending = true;

  for (u32 i=0; i < m_num_ports; ++i) {
    if (!m_read_request_fifo[i]->empty()) {
      XTSC_INFO(m_text, "prepare_to_switch_sim_mode() returning false because m_read_request_fifo[" << i << "] not empty");
      return false;
    }
    if (!m_write_request_fifo[i]->empty()) {
      XTSC_INFO(m_text, "prepare_to_switch_sim_mode() returning false because m_write_request_fifo[" << i << "] not empty");
      return false;
    }
  }

  XTSC_INFO(m_text, "prepare_to_switch_sim_mode() successful");
  return true;
}



bool xtsc_component::xtsc_pif2axi_transactor::switch_sim_mode(xtsc_sim_mode mode)
{
  string str = (mode==xtsc::XTSC_CYCLE_ACCURATE) ? "XTSC_CYCLE_ACCURATE" : "XTSC_FUNCTIONAL";
  XTSC_DEBUG(m_text, "switch_sim_mode with mode=" << str << ")");

  m_sim_mode = mode;
  m_mode_switch_pending = false;

  XTSC_INFO(m_text, "switch_sim_mode() successful");
  return true;
}


void xtsc_component::xtsc_pif2axi_transactor::reset(bool /* hard_reset */) 
{
  XTSC_INFO(m_text, "reset() called");

  m_mode_switch_pending                 = false;
  m_next_read_request_port_num          = 0;
  m_next_write_request_port_num         = 0;
  m_next_response_port_num              = 0;
  m_next_nacc_nb_read_request_port_num  = 0;
  m_next_nacc_nb_write_request_port_num = 0;

  for (u32 port_num=0; port_num<m_num_ports; ++port_num) {
    m_read_request_thread_event          [port_num]->cancel();
    m_write_request_thread_event         [port_num]->cancel();
    m_arbitrate_response_thread_event    [port_num]->cancel();
    m_nacc_nb_write_request_thread_event [port_num]->cancel();
    m_nacc_nb_read_request_thread_event  [port_num]->cancel();
    m_outstanding_write_event            [port_num]->cancel();
#if ((defined(SC_API_VERSION_STRING) && (SC_API_VERSION_STRING != sc_api_version_2_2_0)) || IEEE_1666_SYSTEMC >= 201101L)    
    m_rd_response_peq                    [port_num]->cancel_all();
    m_wr_response_peq                    [port_num]->cancel_all();
#endif
  }

  // TODO: m_transaction_info_map
  xtsc_reset_processes(m_process_handles);
}



void xtsc_component::xtsc_pif2axi_transactor::change_clock_period(u32 clock_period_factor) 
{
  m_clock_period = sc_get_time_resolution() * clock_period_factor;
  XTSC_INFO(m_text, "Clock period changed by factor: " << clock_period_factor << " to " << m_clock_period);
}



void xtsc_component::xtsc_pif2axi_transactor::execute(const string&         cmd_line, 
                                                      const vector<string>& words,
                                                      const vector<string>& words_lc,
                                                      ostream&              result) 
{
  ostringstream res;

  if (words[0] == "change_clock_period") {
    u32 clock_period_factor = xtsc_command_argtou32(cmd_line, words, 1);
    res << m_clock_period.value();
    change_clock_period(clock_period_factor);
  }

  else if (words[0] == "peek") {
    u32 port_num      = xtsc_command_argtou32(cmd_line, words, 1);
    u64 start_address = xtsc_command_argtou64(cmd_line, words, 2);
    u32 num_bytes     = xtsc_command_argtou32(cmd_line, words, 3);
    u8 *buffer = new u8[num_bytes];
    m_xtsc_request_if_impl[port_num]->nb_peek(start_address, num_bytes, buffer);
    ostringstream oss;
    oss << setfill('0') << hex;
    for (u32 i=0; i<num_bytes; ++i) {
      oss << "0x" << setw(2) << (u32) buffer[i];
      if (i < num_bytes-1) {
        oss << " ";
      }
    }
    res << oss.str();
    delete [] buffer;
  }

  else if (words[0] == "poke") {
    u32 port_num      = xtsc_command_argtou32(cmd_line, words, 1);
    u64 start_address = xtsc_command_argtou64(cmd_line, words, 2);
    u32 num_bytes     = xtsc_command_argtou32(cmd_line, words, 3);
    if (words.size() != num_bytes+4) {
      ostringstream oss;
      oss << "Command '" << cmd_line << "' specifies num_bytes=" << num_bytes << " but contains " << (words.size()-4)
          << " byte arguments.";
      throw xtsc_exception(oss.str());
    }
    if (num_bytes) {
      u8 *buffer = new u8[num_bytes];
      for (u32 i=0; i<num_bytes; ++i) {
        buffer[i] = (u8) xtsc_command_argtou32(cmd_line, words, 4+i);
      }
      m_xtsc_request_if_impl[port_num]->nb_poke(start_address, num_bytes, buffer);
      delete [] buffer;
    }
  }

  else if (words[0] == "reset") {
    reset();
  }

  else {
    ostringstream oss;
    oss << name() << "::" << __FUNCTION__ << "() called for unknown command '" << cmd_line << "'.";
    throw xtsc_exception(oss.str());
  }

  result << res.str();
}


xtsc_request *xtsc_component::xtsc_pif2axi_transactor::copy_request(const xtsc_request& request) {
  xtsc_request *p_request = NULL;
  if (m_request_pool.empty()) {
    m_request_count += 1;
    XTSC_TRACE(m_text, "Created new xtsc_request, count #" << m_request_count);
    p_request = new xtsc_request();
  }
  else {
    p_request = m_request_pool.back();
    m_request_pool.pop_back();
    XTSC_TRACE(m_text, "Used xtsc_request from pool, size #" << m_request_pool.size());
  }
  *p_request = request; //Copy contents
  return p_request;
}


xtsc_request *xtsc_component::xtsc_pif2axi_transactor::new_request() {
  xtsc_request *p_request = NULL;
  if (m_request_pool.empty()) {
    m_request_count += 1;
    p_request = new xtsc_request();
    XTSC_TRACE(m_text, "Created new xtsc_request, count #" << m_request_count);
  }
  else {
    p_request = m_request_pool.back();
    m_request_pool.pop_back();
    XTSC_TRACE(m_text, "Used xtsc_request from pool, size #" << m_request_pool.size());
  }
  return p_request;
}


void xtsc_component::xtsc_pif2axi_transactor::delete_request(xtsc_request*& p_request) {
  m_request_pool.push_back(p_request);
  XTSC_TRACE(m_text, "Returned xtsc_request to pool, size #" << m_request_pool.size());
  p_request = 0;
}


xtsc_component::xtsc_pif2axi_transactor::req_rsp_info* 
xtsc_component::xtsc_pif2axi_transactor::new_req_rsp_info() {
  req_rsp_info *p_req_rsp_info;
  if (m_req_rsp_info_pool.empty()) {
    m_req_rsp_info_count += 1;
    XTSC_TRACE(m_text, "Created a new req_rsp_info, count #" << m_req_rsp_info_count);
    return new req_rsp_info();
  }
  else {
    req_rsp_info *p_req_rsp_info = m_req_rsp_info_pool.back();
    m_req_rsp_info_pool.pop_back();
    XTSC_TRACE(m_text, "Used a req_rsp_info from pool, size #" << m_req_rsp_info_pool.size());
    return p_req_rsp_info;
  }
}


void xtsc_component::xtsc_pif2axi_transactor::delete_req_rsp_info(req_rsp_info*& p_req_rsp_info) {
  p_req_rsp_info->init();
  m_req_rsp_info_pool.push_back(p_req_rsp_info);
  XTSC_TRACE(m_text, "Returned req_rsp_info to pool, size #" << m_req_rsp_info_pool.size());
  p_req_rsp_info = 0;
}


void xtsc_component::xtsc_pif2axi_transactor::read_request_thread(void) {
  // Get the port number for this "instance" of request_thread
  u32 port_num = m_next_read_request_port_num++;

  try {
    // Handle re-entry after reset (TODO : Check, clearing of fifo may be done in reset() itself)
    if (sc_time_stamp() != SC_ZERO_TIME) {
      //wait(SC_ZERO_TIME);  // Allow one delta cycle to ensure we can completely empty m_request_fifo (an sc_fifo)
      while (!m_read_request_fifo[port_num]->empty()) {
        XTSC_DEBUG(m_text, "m_read_request_fifo[" << port_num << "]: Emptying on reset.");
        req_sched_info *p_req_sched_info = m_read_request_fifo[port_num]->front();
        xtsc_request *p_pif_request = p_req_sched_info->m_pif_request;
        delete_request(p_pif_request);
        m_read_request_fifo[port_num]->pop_front();
        delete p_req_sched_info;
      }
    }

    while (true) {

      while (!m_read_request_fifo[port_num]->empty()) {

        XTSC_TRACE(m_text, "Reenter read_request_thread. Read read_request_fifo[" << port_num << "]");
        req_sched_info *p_req_sched_info = m_read_request_fifo[port_num]->front();

        if (p_req_sched_info->m_sched_time > sc_time_stamp()) {
          XTSC_DEBUG(m_text, "Wait " << p_req_sched_info->m_sched_time - sc_time_stamp() << " for scheduled event");
          wait(p_req_sched_info->m_sched_time - sc_time_stamp());
        }
        
        xtsc_request *p_pif_request   = p_req_sched_info->m_pif_request;
        if (p_pif_request->get_type() == xtsc_request::RCW) {
          do_exclusive(p_pif_request,port_num);
        }
        else {
          do_burst_read(p_pif_request, port_num);
        }

        m_read_request_fifo[port_num]->pop_front();
        delete p_req_sched_info; 
        
      }

      XTSC_TRACE(m_text, "read_request_thread[" << port_num << "] going to sleep.");
      wait(*m_read_request_thread_event[port_num]);
      XTSC_TRACE(m_text, "read_request_thread[" << port_num << "] woke up.");

    } 
  }
  catch (const sc_unwind_exception& error) { throw; }
  catch (const exception& error) {
    ostringstream oss;
    oss << "std::exception caught in read_request_thread[" << port_num << "] of " << kind() << " '" <<
            name() << "'." << endl;
    oss << "what(): " << error.what() << endl;
    xtsc_log_multiline(m_text, FATAL_LOG_LEVEL, oss.str(), 2);
    throw;
  }
}


void xtsc_component::xtsc_pif2axi_transactor::burst_read_worker(xtsc_request *p_axi_request, u32 port_num) 
{
  //Maintain order
  if (m_maintain_order) {
    while(p_axi_request->get_tag() != m_pif_request_order_dq[port_num]->front()) {
      XTSC_VERBOSE(m_text, "Older Write Outstanding with tag: " << m_pif_request_order_dq[port_num]->front() <<
                           " ..Retry request with tag=" << p_axi_request->get_tag() << " after 1 cycle");
      wait(m_clock_period);
    }
  } 
  XTSC_DEBUG(m_text, "No Older Write outstanding");
  
  //Check write request outstanding with the same address
  if (m_wait_on_outstanding_write) {
    while(check_outstanding_write(p_axi_request, port_num)) {
      XTSC_VERBOSE(m_text, "Write Outstanding with address: 0x" << std::hex << p_axi_request->get_byte_address() 
                           << " ..Retry request with tag=" << p_axi_request->get_tag() << " after 1 cycle");
      wait(m_clock_period , *m_outstanding_write_event[port_num]);
    }
  }

  u32 wait_count = 0;
  while (m_outstanding_reads[port_num] == m_max_outstanding) {
    wait_count++;
    XTSC_VERBOSE(m_text, "Read Outstanding limit reached! Waiting. Port #" << port_num << " Wait #" << wait_count);
    wait (m_clock_period);
  }

  //Send the request and wait 1 cycle for any potential RSP_NACC
  m_read_waiting_for_nacc[port_num] = true;
  u32 try_count = 0;
  do {
    m_read_request_got_nacc[port_num] = false;
    try_count++;
    XTSC_DEBUG(m_text, "AXI request:  " << *p_axi_request << " Read Port #" << port_num << "  Try #" << try_count);
    XTSC_INFO(m_text, *p_axi_request << " Read Port #" << port_num << "  Try #" << try_count);
    (*m_read_request_ports[port_num])->nb_request(*p_axi_request);
    wait(m_nacc_wait_time);
  } while (m_read_request_got_nacc[port_num]);
  m_outstanding_reads[port_num]++;
  m_read_waiting_for_nacc[port_num] = false;
} 


void xtsc_component::xtsc_pif2axi_transactor::write_request_thread(void) 
{
  // Get the port number for this "instance" of request_thread
  u32 port_num = m_next_write_request_port_num++;

  try {
    // Handle re-entry after reset
    if (sc_time_stamp() != SC_ZERO_TIME) {
      //wait(SC_ZERO_TIME);  // Allow one delta cycle to ensure we can completely empty m_request_fifo (an sc_fifo)
      while (!m_write_request_fifo[port_num]->empty()) {
        XTSC_DEBUG(m_text, "m_write_request_fifo[" << port_num << "]: Emptying on reset.");
        req_sched_info *p_req_sched_info = m_write_request_fifo[port_num]->front();
        xtsc_request *p_pif_request = p_req_sched_info->m_pif_request;
        delete_request(p_pif_request);
        m_write_request_fifo[port_num]->pop_front();
        delete p_req_sched_info;
      }
    }

    while (true) {

      while (!m_write_request_fifo[port_num]->empty()) {

        XTSC_TRACE(m_text, "Reenter write_request_thread. Read write_request_fifo[" << port_num << "]");
        req_sched_info *p_req_sched_info = m_write_request_fifo[port_num]->front();

        if (p_req_sched_info->m_sched_time > sc_time_stamp()) {
          XTSC_DEBUG(m_text, "Wait " << p_req_sched_info->m_sched_time - sc_time_stamp() << " for scheduled event");
          wait(p_req_sched_info->m_sched_time - sc_time_stamp());
        }
        
        xtsc_request *p_pif_request   = p_req_sched_info->m_pif_request;
        if (p_pif_request->get_type() == xtsc_request::RCW) {
          do_exclusive(p_pif_request, port_num);
        }
        else {
          do_burst_write(p_pif_request, port_num); 
        }

        m_write_request_fifo[port_num]->pop_front();
        delete p_req_sched_info; 
      }

      XTSC_TRACE(m_text, "write_request_thread[" << port_num << "] going to sleep.");
      wait(*m_write_request_thread_event[port_num]);
      XTSC_TRACE(m_text, "write_request_thread[" << port_num << "] woke up.");

    } 
  }
  catch (const sc_unwind_exception& error) { throw; }
  catch (const exception& error) {
    ostringstream oss;
    oss << "std::exception caught in write_request_thread[" << port_num << "] of " << kind() << " '" << name() << "'." << endl;
    oss << "what(): " << error.what() << endl;
    xtsc_log_multiline(m_text, FATAL_LOG_LEVEL, oss.str(), 2);
    throw;
  }
}


void xtsc_component::xtsc_pif2axi_transactor::burst_write_worker(xtsc_request *p_axi_request, u32 port_num) 
{
  u32 wait_count = 0;
  while (m_outstanding_writes[port_num] == m_max_outstanding) {
    wait_count++;
    XTSC_VERBOSE(m_text, "Write Outstanding limit reached! Waiting. Port #" << port_num << " Wait #" << wait_count);
    wait (m_clock_period);
  }

  //Send the request and wait 1 cycle for any potential RSP_NACC
  m_write_waiting_for_nacc[port_num] = true;
  u32 try_count = 0;
  do {
    m_write_request_got_nacc[port_num] = false;
    try_count++;
    XTSC_DEBUG(m_text, "AXI request:  " << *p_axi_request << " Write Port #" << port_num << "  Try #" << try_count);
    XTSC_INFO(m_text, *p_axi_request << " Write Port #" << port_num << "  Try #" << try_count);
    (*m_write_request_ports[port_num])->nb_request(*p_axi_request);
    wait(m_nacc_wait_time);
  } while (m_write_request_got_nacc[port_num]);
  if (p_axi_request->get_last_transfer()) {
    m_outstanding_writes[port_num]++;
  }
  m_write_waiting_for_nacc[port_num] = false;

} 


bool xtsc_component::xtsc_pif2axi_transactor::check_outstanding_write(xtsc_request *p_axi_request, xtsc::u32 port_num) 
{
  xtsc_address     address8                     = p_axi_request->get_byte_address();
  u32              num_transfers                = p_axi_request->get_num_transfers();
  u32              size8                        = p_axi_request->get_byte_size();
  u32              total_bytes_to_transfer      = num_transfers*size8;
  xtsc_address     read_start_address8          = (address8 / total_bytes_to_transfer) * 
                                                  total_bytes_to_transfer;  //lower wrap boundary
  xtsc_address     read_end_address8            = read_start_address8 + total_bytes_to_transfer - 1;

  for (auto it = m_tag_to_axi_request_map[port_num]->begin(); it != m_tag_to_axi_request_map[port_num]->end(); it++) {
    for (unsigned int i = 0; i < it->second.size(); i++) { 
      if (!is_read_access(it->second[i])) {
        u32              write_num_transfers        = it->second[i]->get_num_transfers();
        u32              write_size8                = it->second[i]->get_byte_size();
        xtsc_address     write_start_address8       = it->second[i]->get_byte_address();
        xtsc_address     write_end_address8         = write_start_address8 + write_num_transfers*write_size8 - 1;
        //Look for overlapping overlapping read and write blocks
        if ((read_start_address8 >= write_start_address8 && read_start_address8 <= write_end_address8) ||
            (read_end_address8   >= write_start_address8 && read_end_address8   <= write_end_address8) || 
            (read_start_address8 <= write_start_address8 && read_end_address8   >= write_end_address8)) {
          XTSC_DEBUG(m_text, "Found Outstanding Write request: " << *(it->second[i]));
          return true;
        }
      }
    }
  }

  //check if there is an "earlier" write in write request fifo, with matching address
  for (auto it = m_write_request_fifo[port_num]->begin(); it != m_write_request_fifo[port_num]->end(); it++) {
    u32              write_num_transfers        = (*it)->m_pif_request->get_num_transfers();
    u32              write_size8                = (*it)->m_pif_request->get_byte_size();
    xtsc_address     write_start_address8       = (*it)->m_pif_request->get_byte_address();
    xtsc_address     write_end_address8         = write_start_address8 + write_num_transfers*write_size8 - 1;
    if ((*it)->m_sched_time <= sc_time_stamp()) { //write was scheduled for time before this read
      //Look for overlapping overlapping read and write blocks
      if ((read_start_address8 >= write_start_address8 && read_start_address8 <= write_end_address8) ||
          (read_end_address8   >= write_start_address8 && read_end_address8   <= write_end_address8) || 
          (read_start_address8 <= write_start_address8 && read_end_address8   >= write_end_address8)) {
        XTSC_DEBUG(m_text, "Found Outstanding Write request: " << *((*it)->m_pif_request));
        return true;
      }
    }
  }

  XTSC_DEBUG(m_text, "No Outstanding Write with address: 0x" << std::hex << address8);
  return false;
}


void xtsc_component::xtsc_pif2axi_transactor::arbitrate_response_thread(void) 
{
  u32 port_num  = m_next_response_port_num++;

  try {

    sc_event &rd_peq_event = m_rd_response_peq[port_num]->get_event();
    sc_event &wr_peq_event = m_wr_response_peq[port_num]->get_event();

    // Loop forever
    while (true) {
     
      XTSC_TRACE(m_text, "arbitrate_respond_thread[" << port_num << "] going to sleep.");
      wait(rd_peq_event | wr_peq_event);
      XTSC_TRACE(m_text, "arbitrate_respond_thread[" << port_num << "] woke up.");

      xtsc_response *p_rd_pif_response      = m_rd_response_peq[port_num]->get_next_transaction();
      xtsc_response *p_wr_pif_response      = m_wr_response_peq[port_num]->get_next_transaction();
      bool prioritize_write                 = false;
      xtsc_response *p_arbitrated_response  = NULL;

      do {
        XTSC_DEBUG(m_text, "Arbitrate responses." << " prioritize_write=" << prioritize_write);
        if (p_rd_pif_response != NULL) 
          XTSC_DEBUG(m_text, "p_rd_pif_response=" << *p_rd_pif_response);
        if (p_wr_pif_response != NULL) 
          XTSC_DEBUG(m_text, "p_wr_pif_response=" << *p_wr_pif_response); 

        if (prioritize_write) {
          p_arbitrated_response = p_wr_pif_response;
          prioritize_write      = false;
          p_wr_pif_response     = NULL;
        } else if (p_rd_pif_response) {
          p_arbitrated_response = p_rd_pif_response;
          p_rd_pif_response     = NULL;
          if (p_wr_pif_response) {
            prioritize_write    = true;
          }
        } else if (p_wr_pif_response) {
          p_arbitrated_response = p_wr_pif_response;
          p_wr_pif_response     = NULL;
        }

        send_response(*p_arbitrated_response, port_num);
        delete p_arbitrated_response;
        wait(m_clock_period);

        // Check for the availability of new responses.
        if (p_rd_pif_response == NULL)
          p_rd_pif_response = m_rd_response_peq[port_num]->get_next_transaction();
        if (p_wr_pif_response == NULL)
          p_wr_pif_response = m_wr_response_peq[port_num]->get_next_transaction();

      } while (p_rd_pif_response != NULL || p_wr_pif_response != NULL);
    } 
  }
  catch (const sc_unwind_exception& error) { throw; }
  catch (const exception& error) {
    ostringstream oss;
    oss << "std::exception caught in response_thread[" << port_num << "] of " << kind() << " '" 
        << name() << "'." << endl;
    oss << "what(): " << error.what() << endl;
    xtsc_log_multiline(m_text, FATAL_LOG_LEVEL, oss.str(), 2);
    throw;
  }
}



void xtsc_component::xtsc_pif2axi_transactor::send_response(xtsc_response& response, u32 port_num) 
{
  u32 try_count = 1;
  XTSC_DEBUG(m_text, "PIF response: " <<  response << " Port #" << port_num << "  Try #" << try_count);
  XTSC_INFO (m_text, response << " Port #" << port_num << "  Try #" << try_count);
  // Need to check return value because master might be busy
  while (!(*m_respond_ports[port_num])->nb_respond(response)) {
    XTSC_VERBOSE(m_text, "nb_respond() returned false; waiting one clock period to try again.  Port #" << port_num);
    wait(m_clock_period);
    try_count++;
    XTSC_INFO(m_text, response << " Port #" << port_num << "  Try #" << try_count);
  }
}



// do_TYPE
void xtsc_component::xtsc_pif2axi_transactor::do_burst_read(xtsc_request *p_pif_request, u32 port_num) 
{
  xtsc_address          address8        = p_pif_request->get_byte_address();
  u32                   size8           = p_pif_request->get_byte_size();
  u64                   pif_tag         = p_pif_request->get_tag();
  u32                   num_transfers   = p_pif_request->get_num_transfers();
  xtsc_byte_enables     byte_enables    = p_pif_request->get_byte_enables();
  u32                   route_id        = p_pif_request->get_route_id();
  u8                    id              = p_pif_request->get_id();
  u8                    priority        = p_pif_request->get_priority();
  xtsc_address          pc              = p_pif_request->get_pc();
  u32                   total_bytes     = num_transfers * size8;
  bool                  wrap            = false;
  
  string type_name = (string)p_pif_request->get_type_name();
  if ( ((type_name == "READ") && num_transfers != 1)  ||
       ((type_name == "BLOCK_READ") && num_transfers != 2 && num_transfers !=4 && 
         num_transfers != 8 && num_transfers != 16 )  ||
       ((type_name == "BURST_READ") && (num_transfers < 2 || num_transfers >= 16)) ) 
  {
    ostringstream oss;
    oss << kind() << " '" << name() << "' Unsupported number of transfers : " << num_transfers 
        << " for request type : " << type_name << endl;
    throw xtsc_exception(oss.str());
  }

  if ((type_name == "BLOCK_READ" || type_name == "BURST_READ") && (address8 % size8)) {
    ostringstream oss;
    oss << kind() << " '" << name() << "' Unaligned transfer for request type : " << type_name << endl;
    throw xtsc_exception(oss.str());
  }

  if ( ((type_name == "BLOCK_READ")    && (size8 != m_pif_width8)) || 
       ((type_name == "BURST_READ")    && (size8 != m_pif_width8)) || 
       ((type_name == "READ")          && (((size8 !=  1) && (size8 !=  2) && (size8 !=  4) && 
        (size8 !=  8) && (size8 != 16) && (size8 != 32))  || (size8 > m_pif_width8))) 
     ) {
    ostringstream oss;
    oss << kind() << " '" << name() << "' Unsupported size of transfer : " << size8 << 
                                       " for request type : " << type_name << endl;
    throw xtsc_exception(oss.str());
  }

  //Book keep pif and axi requests for the response path
  req_rsp_info *p_req_rsp_info = new_req_rsp_info();
  p_req_rsp_info->m_pif_request_vec.push_back(p_pif_request);
  (*m_tag_to_req_rsp_info_map[port_num])[pif_tag] = p_req_rsp_info;

  //Vector to populate multiple axi requests for this pif request
  std::vector<xtsc_request*> conv_axi_request_vec;

  if (size8 > m_axi_width8) {   //m_pif_width8 > m_axi_width8
    convert_to_narrow_burst_reads(p_pif_request, port_num, conv_axi_request_vec);
  } 
  else {                        //m_pif_width8 <= m_axi_width8
    if (!m_interface_is_idma0   &&  (m_allow_axi_wrap && (type_name == "BLOCK_READ" || 
                                    (type_name == "BURST_READ" && num_transfers > 1)))) { 
      wrap = true;
    }
    xtsc_request::burst_t burst   = (wrap==true) ? xtsc_request::WRAP : xtsc_request::INCR;
    xtsc_byte_enables be          = 0xFFFFFFFFFFFFFFFFull >> (64-size8);
    u32  bytes_to_send            = total_bytes;  
    xtsc_address offset           = 0;
    do {       
      u32 bytes_to_4k_boundary    = AXI_ADDR_BOUNDARY - ((address8+offset) % AXI_ADDR_BOUNDARY);
      u32 bytes_in_axi            = (m_interface_is_idma0 & (bytes_to_send > bytes_to_4k_boundary)) 
                                     ? bytes_to_4k_boundary : bytes_to_send;       //Multiple transfers for iDMA0 if crossing 4K boundary
      u32 axi_num_transfers       = bytes_in_axi / size8;
      xtsc_request *p_axi_request = new_request();
      p_axi_request->initialize(xtsc_request::BURST_READ, address8+offset, size8, (offset == 0 ? pif_tag : 0),
                                axi_num_transfers, be, true, route_id, id, priority, pc, burst);
      p_axi_request->set_instruction_fetch(p_pif_request->get_instruction_fetch());
      p_axi_request->set_prot((u8)((p_pif_request->get_pif_attribute() & BIT_11) >> 10));
      p_axi_request->set_exclusive(p_pif_request->get_exclusive());
      XTSC_DEBUG(m_text, "Initialized AXI request: " << *p_axi_request); 

      conv_axi_request_vec.push_back(p_axi_request);
      bytes_to_send              -= bytes_in_axi;
      offset                     += bytes_in_axi;
    } while (bytes_to_send);   
  }

  //fill & notify read_request_fifo
  for (u32 i=0; i < conv_axi_request_vec.size(); i++) {
    //Bookkeep converted axi requests 
    (*m_tag_to_axi_request_map[port_num])[pif_tag].push_back(conv_axi_request_vec[i]);
    u64 axi_tag = conv_axi_request_vec[i]->get_tag();
    (*m_axi_tag_from_pif_tag_map[port_num])[axi_tag] = pif_tag; 
    XTSC_DEBUG(m_text, "PIF tag : " << pif_tag << "  ->  AXI tag : " << axi_tag); 
    burst_read_worker(conv_axi_request_vec[i], port_num);
  }

} 


//
// do_TYPE
void xtsc_component::xtsc_pif2axi_transactor::do_exclusive(xtsc_request *p_pif_request, u32 port_num) 
{
  if (p_pif_request->get_exclusive()) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': Received illegal exclusive request: " << *p_pif_request;
    throw xtsc_exception(oss.str());
  }

  xtsc_address          address8                        = p_pif_request->get_byte_address();
  xtsc_address          hw_address8                     = p_pif_request->get_hardware_address();
  u32                   size8                           = p_pif_request->get_byte_size();
  u64                   tag                             = p_pif_request->get_tag();
  u32                   num_transfers                   = p_pif_request->get_num_transfers();
  xtsc_byte_enables     byte_enables                    = p_pif_request->get_byte_enables();
  bool                  last                            = p_pif_request->get_last_transfer();
  u32                   route_id                        = p_pif_request->get_route_id();
  u8                    id                              = p_pif_request->get_id();
  u8                    priority                        = p_pif_request->get_priority();
  xtsc_address          pc                              = p_pif_request->get_pc();
  xtsc_byte_enables     be_mask                         = 0xFFFFFFFFFFFFFFFFull >> (64-m_axi_width8);
  u32                   num_axi_transfers               = 1;

  if ( num_transfers != 2)   
  {
    ostringstream oss;
    oss << kind() << " '" << name() << "' Unsupported number of transfers : " << num_transfers << 
           " for request type : " << p_pif_request->get_type_name() << endl;
    throw xtsc_exception(oss.str());
  }

  if ((size8 != 4) || (address8 % size8 != 0)) {
    ostringstream oss;
    oss << kind() << " '" << name() << "' Unsupported size of transfer : " << size8 << " for request type : " 
                  << p_pif_request->get_type_name() << ". Supported size : 4 bytes" << endl;
    throw xtsc_exception(oss.str());
  }

  //Book keep pif requests for the response path
  req_rsp_info *p_req_rsp_info = NULL;
  if (!last) {
    p_req_rsp_info = new_req_rsp_info();
    (*m_tag_to_req_rsp_info_map[port_num])[tag] = p_req_rsp_info;
  } 
  else {
    p_req_rsp_info = (*m_tag_to_req_rsp_info_map[port_num])[tag];
  }
  p_req_rsp_info->m_pif_request_vec.push_back(p_pif_request);


  if (!last) {  //1st transfer of RCW
    memcpy(&p_req_rsp_info->m_rcw_compare_data, p_pif_request->get_buffer(), size8);
    p_req_rsp_info->m_rcw_first_transfer_rcvd   = true;
    XTSC_DEBUG(m_text, "Read Compare Data=0x" << hex << p_req_rsp_info->m_rcw_compare_data);
  } 
  else {        //2nd transfer of RCW
    if (!p_req_rsp_info->m_rcw_first_transfer_rcvd) {
      ostringstream oss;
      oss << kind() << " '" << name() << "' Received a last-transfer RCW without receiving first-transfer RCW" << endl;
      throw xtsc_exception(oss.str());
    }

    xtsc_request *p_axi_request = new_request();
    p_axi_request->initialize(xtsc_request::BURST_READ, address8, size8, tag, num_axi_transfers,
                              byte_enables&be_mask, true, route_id, id, priority, pc, xtsc_request::INCR);
    p_axi_request->set_exclusive(true);
    p_axi_request->set_prot((u8)((p_pif_request->get_pif_attribute() & BIT_11) >> 10));
    XTSC_DEBUG(m_text, "Exclusive AXI xtsc_request: " << *p_axi_request);

    (*m_axi_tag_from_pif_tag_map[port_num])[tag] = tag; 
    (*m_tag_to_axi_request_map[port_num])[tag].push_back(p_axi_request);
    burst_read_worker(p_axi_request, port_num);
    while(!m_excl_read_resp_rcvd[port_num]) {
      //Allow the response of Excl BURST_READ to come first     
      wait(m_clock_period); 
    }
     
    if (p_req_rsp_info && p_req_rsp_info->m_excl_lock_supported && p_req_rsp_info->m_excl_read_match) { //Read EXOKAY with data match
      xtsc_request *p_axi_request = new_request();
      p_axi_request->initialize(xtsc_request::BURST_WRITE, address8, size8, tag, num_axi_transfers,
                                byte_enables&be_mask, true, route_id, id, priority, pc, xtsc_request::INCR);
      p_axi_request->adjust_burst_write(hw_address8);
      p_axi_request->set_exclusive(true);
      p_axi_request->set_prot((u8)((p_pif_request->get_pif_attribute() & BIT_11) >> 10));
      memcpy(p_axi_request->get_buffer(), p_pif_request->get_buffer(), m_axi_width8);
      XTSC_DEBUG(m_text, "Exclusive AXI xtsc_request: " << *p_axi_request);

      p_req_rsp_info->m_num_expected_write_resp    = 1;
      (*m_axi_tag_from_pif_tag_map[port_num])[tag] = tag; 
      (*m_tag_to_axi_request_map[port_num])[tag].push_back(p_axi_request);
      burst_write_worker(p_axi_request, port_num);

    }
    m_excl_read_resp_rcvd[port_num] = false;
  }
}



// do_TYPE
void xtsc_component::xtsc_pif2axi_transactor::do_burst_write(xtsc_request* p_pif_request, u32 port_num) 
{
  xtsc_address          address8        = p_pif_request->get_byte_address();
  xtsc_address          hw_address8     = p_pif_request->get_hardware_address();
  u32                   size8           = p_pif_request->get_byte_size();
  u64                   pif_tag         = p_pif_request->get_tag();
  u32                   num_transfers   = p_pif_request->get_num_transfers();
  xtsc_byte_enables     byte_enables    = p_pif_request->get_byte_enables();
  bool                  last            = p_pif_request->get_last_transfer();
  u32                   transfer_num    = p_pif_request->get_transfer_number();
  u32                   route_id        = p_pif_request->get_route_id();
  u8                    id              = p_pif_request->get_id();
  u8                    priority        = p_pif_request->get_priority();
  xtsc_address          pc              = p_pif_request->get_pc();
  string                type_name       = (string)p_pif_request->get_type_name();
  u32                   total_bytes     = num_transfers * size8;
  bool                  wrap            = false;

  if ((type_name == "WRITE"       && num_transfers != 1)                                                                    ||
      (type_name == "BLOCK_WRITE" && num_transfers != 2  && num_transfers !=4 && num_transfers != 8 && num_transfers != 16) ||
      (type_name == "BURST_WRITE" && (num_transfers < 2 || num_transfers >= 16))) 
  {
    ostringstream oss;
    oss << kind() << " '" << name() << "' Unsupported number of transfers : " << num_transfers << 
           " for request type : " << type_name << endl;
    throw xtsc_exception(oss.str());
  }
 
  if ((transfer_num == 1)                                                    && 
      (type_name    == "BLOCK_WRITE"  && (address8 % (size8*num_transfers))) || 
      (type_name    == "BURST_WRITE") && (address8 % size8)) {
    ostringstream oss;
    oss << kind() << " '" << name() << "' Unaligned transfer for request type : " << type_name << endl;
    throw xtsc_exception(oss.str());
  }

  //Book keep pif requests for the response path
  req_rsp_info *p_req_rsp_info = NULL;
  if (transfer_num == 1) {
    p_req_rsp_info                                  = new_req_rsp_info();
    (*m_tag_to_req_rsp_info_map[port_num])[pif_tag] = p_req_rsp_info;
  } else {
    p_req_rsp_info = (*m_tag_to_req_rsp_info_map[port_num])[pif_tag];
  }
  p_req_rsp_info->m_pif_request_vec.push_back(p_pif_request);

  //Populate multiple axi requests for this pif request
  std::vector<xtsc_request*> conv_axi_request_vec;
  if (size8 > m_axi_width8) {   //m_pif_width8 > m_axi_width8
    convert_to_narrow_burst_writes(p_pif_request, port_num, conv_axi_request_vec);
  } else {                      //m_pif_width8 <= m_axi_width8
    if (!m_interface_is_idma0  &&  (m_allow_axi_wrap && (type_name == "BLOCK_WRITE" || 
                                   (type_name == "BURST_WRITE" && num_transfers > 1)))) { 
      wrap = true;
    }

    xtsc_request::burst_t burst = xtsc_request::INCR;
    xtsc_address axi_addr8      = address8;
    if (wrap) {
      xtsc_address lower_wrap   = (address8 / total_bytes) * total_bytes;
      xtsc_address upper_wrap   = lower_wrap + total_bytes;
      burst                     = xtsc_request::WRAP;
      axi_addr8                 = address8 < upper_wrap ? address8
                                                        : lower_wrap + (address8 - upper_wrap);      
    } 
    u32 bytes_to_4k_boundary    = AXI_ADDR_BOUNDARY - (hw_address8 % AXI_ADDR_BOUNDARY);
    u32 total_bytes_in_axi      = total_bytes;  
    u32 axi_transfer_num        = transfer_num; 
    if (m_interface_is_idma0 && (total_bytes >= bytes_to_4k_boundary)) { //Multiple transfers for iDMA0 if crossing 4K boundary
      total_bytes_in_axi        = (transfer_num*size8 <= bytes_to_4k_boundary) ? bytes_to_4k_boundary 
                                                                               : (total_bytes        - bytes_to_4k_boundary);
      axi_transfer_num          = (transfer_num*size8 <= bytes_to_4k_boundary) ? transfer_num      
                                                                               : (transfer_num*size8 - bytes_to_4k_boundary) / size8;
    }
    u32 axi_num_transfers       = total_bytes_in_axi / size8;

    if (bytes_to_4k_boundary < total_bytes) {
      XTSC_DEBUG(m_text, "PIF Transfer crossing 4K boundary.");
    }
    XTSC_TRACE(m_text, "hw_address8=0x" << hex << hw_address8 << " bytes_to_4k_boundary=0x" << bytes_to_4k_boundary << dec <<
                       " axi_transfer=" << axi_transfer_num   << "/" << axi_num_transfers );
    xtsc_request *p_axi_request = new_request();
    if (axi_transfer_num == 1) {
      p_axi_request->initialize(xtsc_request::BURST_WRITE, axi_addr8, size8, (transfer_num==1 ? pif_tag : 0),
                                axi_num_transfers, byte_enables, (axi_transfer_num==axi_num_transfers), route_id, 
                                id, priority, pc, burst);
      p_axi_request->set_prot((u8)((p_pif_request->get_pif_attribute() & BIT_11) >> 10));
      p_axi_request->adjust_burst_write(hw_address8);
      m_tag_axi_write[port_num] = p_axi_request->get_tag();
      (*m_tag_to_req_rsp_info_map[port_num])[pif_tag]->m_num_expected_write_resp += 1;
    }
    else {
      p_axi_request->initialize(hw_address8, m_tag_axi_write[port_num], axi_addr8, size8, axi_num_transfers, 
                                axi_transfer_num, byte_enables, route_id, id, priority, pc, burst);
      p_axi_request->set_prot((u8)((p_pif_request->get_pif_attribute() & BIT_11) >> 10));
    }
    memcpy(p_axi_request->get_buffer(), p_pif_request->get_buffer(), m_pif_width8);  
    p_axi_request->set_exclusive(p_pif_request->get_exclusive());
    XTSC_DEBUG(m_text, "Initialized AXI request: " << *p_axi_request); 

    conv_axi_request_vec.push_back(p_axi_request);
  } 

  //fill & notify write_request_fifo
  for (u32 i=0; i < conv_axi_request_vec.size(); i++) {
    //Bookkeep axi requests 
    (*m_tag_to_axi_request_map[port_num])[pif_tag].push_back(conv_axi_request_vec[i]);
    u64 axi_tag = conv_axi_request_vec[i]->get_tag();
    (*m_axi_tag_from_pif_tag_map[port_num])[axi_tag] = pif_tag; 
    XTSC_DEBUG(m_text, "PIF tag=" << pif_tag << " -> AXI tag=" << axi_tag); 

    burst_write_worker(conv_axi_request_vec[i], port_num);
  }
}


void xtsc_component::xtsc_pif2axi_transactor::convert_to_narrow_burst_reads(xtsc_request *p_pif_request, 
                                               u32 port_num, std::vector<xtsc_request*>& conv_axi_request_vec) 
{
  xtsc_address          address8                    = p_pif_request->get_byte_address();
  u32                   size8                       = p_pif_request->get_byte_size();
  u32                   num_transfers               = p_pif_request->get_num_transfers();
  u64                   pif_tag                     = p_pif_request->get_tag();
  xtsc_byte_enables     byte_enables                = p_pif_request->get_byte_enables();
  u32                   route_id                    = p_pif_request->get_route_id();
  u8                    id                          = p_pif_request->get_id();
  u8                    priority                    = p_pif_request->get_priority();
  xtsc_address          pc                          = p_pif_request->get_pc();
  string                type_name                   = (string)p_pif_request->get_type_name();
  u32                   total_bytes_to_transfer     = size8 * num_transfers;

  //Generate AXI WRAP for transfer aligned BLOCK_READs. For CWF generate AXI INCR, wrapped by upper_wrap addr boundary
  ////For CWF/AXI WRAP
  xtsc_address          lower_wrap_addr             = (address8 / total_bytes_to_transfer) * total_bytes_to_transfer;
  xtsc_address          upper_wrap_addr             = lower_wrap_addr + total_bytes_to_transfer;
  bool                  is_addr_for_critical_word   = (address8 > lower_wrap_addr) && (address8 <= upper_wrap_addr); 
  bool                  wrap                        = (type_name == "BLOCK_READ" && m_allow_axi_wrap && 
                                                       !is_addr_for_critical_word) ? true : false;
  xtsc_request::burst_t burst                       = (wrap==true) ? xtsc_request::WRAP : xtsc_request::INCR;
  u32                   max_num_xfers               = (wrap==true) ? 16 : 256; //INCR supports 256 transfers
  xtsc_byte_enables     be                          = 0xFFFFFFFFFFFFFFFFull >> (64-m_axi_width8);
  u32                   expected_num_axi_transfers  = total_bytes_to_transfer/m_axi_width8;
  u32                   pending_num_axi_transfers   = expected_num_axi_transfers;
  u32                   axi_burst_count             = 0;
  xtsc_address          axi_burst_address8          = address8;

  do {

    axi_burst_count ++;
    //Calculate number of transfers in this axi burst
    u32 num_axi_transfers = pending_num_axi_transfers < max_num_xfers ? pending_num_axi_transfers
                                                                      : max_num_xfers;
    if (is_addr_for_critical_word) { //dont cross upper boundary
      if (axi_burst_address8 + num_axi_transfers*m_axi_width8 > upper_wrap_addr) {
              num_axi_transfers = (upper_wrap_addr - axi_burst_address8) / m_axi_width8;
      }
    }
    XTSC_TRACE(m_text, "axi_burst_count #" << axi_burst_count << " axi_burst_address8: 0x" << std::hex << 
                        axi_burst_address8 << " number_axi_transfers: " << std::dec << num_axi_transfers);

    xtsc_request *p_conv_axi_request = new_request();
    /*reuse pif_tag for 1st burst*/
    u64 tag = (axi_burst_count == 1) ? pif_tag : 0;
    p_conv_axi_request->initialize(xtsc_request::BURST_READ, axi_burst_address8, m_axi_width8, tag, 
                                   num_axi_transfers, be, true, route_id, id, priority, pc, burst);
    p_conv_axi_request->set_instruction_fetch(p_pif_request->get_instruction_fetch());
    p_conv_axi_request->set_exclusive(p_pif_request->get_exclusive());
    p_conv_axi_request->set_prot((u8)((p_pif_request->get_pif_attribute() & BIT_11) >> 10));

    XTSC_DEBUG(m_text, "converted AXI xtsc_request: " << *p_conv_axi_request << " Burst #" << axi_burst_count);
    conv_axi_request_vec.push_back(p_conv_axi_request);

    //Prepare for next burst, if any
    pending_num_axi_transfers -= num_axi_transfers;
    axi_burst_address8 += num_axi_transfers*m_axi_width8;
    if (is_addr_for_critical_word & axi_burst_address8 == upper_wrap_addr) {
      axi_burst_address8 = lower_wrap_addr;
    }
  } while ( pending_num_axi_transfers > 0);
}


void xtsc_component::xtsc_pif2axi_transactor::convert_to_narrow_burst_writes(xtsc_request *p_pif_request, 
                                               u32 port_num, std::vector<xtsc_request*>& conv_axi_request_vec) 
{
  xtsc_address          address8                        = p_pif_request->get_byte_address();
  xtsc_address          hw_address8                     = p_pif_request->get_hardware_address();
  u32                   size8                           = p_pif_request->get_byte_size();
  xtsc_byte_enables     byte_enables                    = p_pif_request->get_byte_enables();
  u32                   num_transfers                   = p_pif_request->get_num_transfers();
  u32                   transfer_num                    = p_pif_request->get_transfer_number();
  u64                   pif_tag                         = p_pif_request->get_tag();
  u32                   route_id                        = p_pif_request->get_route_id();
  u8                    id                              = p_pif_request->get_id();
  u8                    priority                        = p_pif_request->get_priority();
  xtsc_address          pc                              = p_pif_request->get_pc();
  u32                   total_bytes_to_transfer         = num_transfers*size8;
  string                type_name                       = (string)p_pif_request->get_type_name();

  bool                  wrap                            = (type_name == "BLOCK_WRITE" && m_allow_axi_wrap) ? true : false;
  u32                   max_num_xfers                   = (wrap==true) ? 16 : 256; //INCR supports 256 transfers
  xtsc_request::burst_t burst                           = (wrap==true) ? xtsc_request::WRAP : xtsc_request::INCR;
  xtsc_byte_enables     be_mask                         = 0xFFFFFFFFFFFFFFFFull >> (64-m_axi_width8);
  u32                   expected_num_axi_transfers      = total_bytes_to_transfer/m_axi_width8;
  u32                   num_axi_per_pif_transfer        = size8/m_axi_width8;
  u32                   total_axi_bursts_to_transfer    = (u32)ceil( (double)total_bytes_to_transfer/(max_num_xfers*m_axi_width8) );
  //Split request in to multiple axi bursts if, data for this transfer exceeds max_num_xfers
  u32                   axi_burst_count                 = ((transfer_num-1)*size8/m_axi_width8) / max_num_xfers + 1;
  u32                   num_axi_transfers               = expected_num_axi_transfers > axi_burst_count*max_num_xfers ? max_num_xfers 
                                                          : expected_num_axi_transfers - max_num_xfers*(axi_burst_count-1);
  u32                   axi_transfer_num                = (num_axi_per_pif_transfer*(transfer_num-1)) % (max_num_xfers) + 1 ; 

  XTSC_TRACE(m_text, "axi_burst_count: " << axi_burst_count << " total_axi_bursts_to_transfer: " << total_axi_bursts_to_transfer << 
                     " axi_transfer_num: " << axi_transfer_num << " num_axi_transfers: " << num_axi_transfers << 
                     " (PIF)transfer_num: " << transfer_num << " expected_num_axi_transfers: " << expected_num_axi_transfers);

  if (transfer_num == 1) {
    //Set expected responses
    (*m_tag_to_req_rsp_info_map[port_num])[pif_tag]->m_num_expected_write_resp = total_axi_bursts_to_transfer; 
  }

  /*reuse pif_tag for 1st burst*/
  u64 tag = (axi_burst_count == 1) ? pif_tag : ((axi_transfer_num == 1) ? 0 : m_tag_axi_write[port_num]);

  xtsc_request *p_conv_axi_request = new_request();
  bool last = (axi_transfer_num == num_axi_transfers) ? true : false;
  if (axi_transfer_num == 1) {
    p_conv_axi_request->initialize(xtsc_request::BURST_WRITE, address8, m_axi_width8, tag, num_axi_transfers,
                                   byte_enables&be_mask, last, route_id, id, priority, pc, burst);
    p_conv_axi_request->adjust_burst_write(hw_address8);
    p_conv_axi_request->set_prot((u8)((p_pif_request->get_pif_attribute() & BIT_11) >> 10));
    m_tag_axi_write[port_num] = p_conv_axi_request->get_tag();
  }
  else {
    p_conv_axi_request->initialize(hw_address8, m_tag_axi_write[port_num], address8, m_axi_width8, num_axi_transfers, axi_transfer_num, 
                                   byte_enables & be_mask, route_id, id, priority, pc, burst);
    p_conv_axi_request->set_prot((u8)((p_pif_request->get_pif_attribute() & BIT_11) >> 10));
  }
  memcpy(p_conv_axi_request->get_buffer(), p_pif_request->get_buffer(), m_axi_width8);
  XTSC_DEBUG(m_text, "converted AXI xtsc_request[" << p_conv_axi_request->get_transfer_number() << "]: " << 
                     *p_conv_axi_request << " Burst #" << axi_burst_count);
  conv_axi_request_vec.push_back(p_conv_axi_request);

  for (u32 i=1; i < num_axi_per_pif_transfer; i++) {
    xtsc_request *p_conv_axi_request = new_request();
    p_conv_axi_request->initialize(hw_address8, m_tag_axi_write[port_num], address8 + i*m_axi_width8, m_axi_width8, num_axi_transfers, 
                                   axi_transfer_num + i, (byte_enables >> i*m_axi_width8) & be_mask, route_id, id, priority, 
                                   pc, burst);
    p_conv_axi_request->set_prot((u8)((p_pif_request->get_pif_attribute() & BIT_11) >> 10));
    bool last = (axi_transfer_num + i == num_axi_transfers) ? true : false;
    p_conv_axi_request->set_last_transfer(last);
    memcpy(p_conv_axi_request->get_buffer(), p_pif_request->get_buffer() + i*m_axi_width8, m_axi_width8);
    XTSC_DEBUG(m_text, "converted AXI xtsc_request[" << p_conv_axi_request->get_transfer_number() << "]: " 
                       << *p_conv_axi_request << " Burst #" << axi_burst_count);
    conv_axi_request_vec.push_back(p_conv_axi_request);
  }
}


void xtsc_component::xtsc_pif2axi_transactor::nacc_nb_read_request_thread(){
  // Get the port number for this "instance" of request_thread
  u32 port_num = m_next_nacc_nb_read_request_port_num++;

  try {

    while (true) {

      if (sc_time_stamp() != SC_ZERO_TIME) {

        if (sc_pending_activity_at_current_time()) {
          //Allow this thread to be scheduled in the last delta cycle 
          //(TODO : check if SystemC has in buit mechanism for schedling thread in last delta)
          XTSC_TRACE(m_text, "Wait for other runnable threads to execute first..");
          wait(SC_ZERO_TIME);
        }
        XTSC_TRACE(m_text, "Wait over, now in the last delta of current timestamp.");

        //Now we are in the last delta of current time stamp
        if ( m_read_request_fifo[port_num]->size() == m_read_request_fifo_depth ) {
          //Return as fifo(s) are still full
          xtsc_response response(*m_nacc_nb_read_request[port_num], xtsc_response::RSP_NACC);
          XTSC_DEBUG(m_text, "PIF response: " << response << " Port #" << port_num <<  " (Read Request fifo full)" );
          XTSC_INFO(m_text, response << " Port #" << port_num << " (Read Request fifo full)" );
          (*m_respond_ports[port_num])->nb_respond(response);
          delete_request(m_nacc_nb_read_request[port_num]);
        }
        else {
          //Fifo slot now available, Push into the fifos
          m_xtsc_request_if_impl[port_num]->nb_request_worker(m_nacc_nb_read_request[port_num]);
        }
        m_nacc_nb_read_request[port_num] = NULL;
      }

      XTSC_TRACE(m_text, "nacc_nb_read_request_thread[" << port_num << "] going to sleep.");
      wait(*m_nacc_nb_read_request_thread_event[port_num]);
      XTSC_TRACE(m_text, "nacc_nb_read_request_thread[" << port_num << "] woke up.");
    } 
  }
  catch (const sc_unwind_exception& error) { throw; }
  catch (const exception& error) {
    ostringstream oss;
    oss << "std::exception caught in nacc_nb_read_request_thread[" << port_num << "] of " << kind() << " '" 
        << name() << "'." << endl;
    oss << "what(): " << error.what() << endl;
    xtsc_log_multiline(m_text, FATAL_LOG_LEVEL, oss.str(), 2);
    throw;
  }
}



void xtsc_component::xtsc_pif2axi_transactor::nacc_nb_write_request_thread(){
  // Get the port number for this "instance" of request_thread
  u32 port_num = m_next_nacc_nb_write_request_port_num++;

  try {

    while (true) {

      if (sc_time_stamp() != SC_ZERO_TIME) {
        if (sc_pending_activity_at_current_time()) {
          //Allow this thread to be scheduled in the last delta cycle 
          //(TODO : check if SystemC has in buit mechanism for schedling thread in last delta)
          XTSC_TRACE(m_text, "Wait for other runnable threads to execute first..");
          wait(SC_ZERO_TIME);
        }
        XTSC_TRACE(m_text, "Wait over, now in the last delta of current timestamp.");

        //Now we are in the last delta of current time stamp
        if ( m_write_request_fifo[port_num]->size() == m_write_request_fifo_depth ) {
          //Return as fifo(s) are still full
          xtsc_response response(*m_nacc_nb_write_request[port_num], xtsc_response::RSP_NACC);
          XTSC_DEBUG(m_text, "PIF response: " << response << " Port #" << port_num <<  
                             " (Write Request fifo full)" );
          XTSC_INFO(m_text, response << " Port #" << port_num << " (Write Request fifo full)" );
          (*m_respond_ports[port_num])->nb_respond(response);
          delete_request(m_nacc_nb_write_request[port_num]);
        }
        else {
          //Fifo slot now available, Push into the queues
          m_xtsc_request_if_impl[port_num]->nb_request_worker(m_nacc_nb_write_request[port_num]);
        }
        m_nacc_nb_write_request[port_num] = NULL;
      }

      XTSC_TRACE(m_text, "nacc_nb_write_request_thread[" << port_num << "] going to sleep.");
      wait(*m_nacc_nb_write_request_thread_event[port_num]);
      XTSC_TRACE(m_text, "nacc_nb_write_request_thread[" << port_num << "] woke up.");
    } 
  }
  catch (const sc_unwind_exception& error) { throw; }
  catch (const exception& error) {
    ostringstream oss;
    oss << "std::exception caught in nacc_nb_write_request_thread[" << port_num << "] of " << kind() << " '" 
        << name() << "'." << endl;
    oss << "what(): " << error.what() << endl;
    xtsc_log_multiline(m_text, FATAL_LOG_LEVEL, oss.str(), 2);
    throw;
  }
}



void xtsc_component::xtsc_pif2axi_transactor::xtsc_request_if_impl::nb_request(const xtsc_request& request) 
{
  XTSC_DEBUG(m_transactor.m_text, "PIF request:  " << request << " Port #" << m_port_num );
  XTSC_INFO(m_transactor.m_text, request << " Port #" << m_port_num );

  if (request.get_byte_size() > m_transactor.m_pif_width8) {
      ostringstream oss;
      oss << kind() << " '" << name() << "' Received " << request.get_type_name() << " tag=" << request.get_tag() <<  
             "with byte size of " << request.get_byte_size() << " (can't be more than \"pif_byte_width\" of  " << 
             m_transactor.m_pif_width8 << ")";
      throw xtsc_exception(oss.str());
  }

  if (!m_transactor.is_read_access(&request) && m_transactor.m_nacc_nb_write_request[m_port_num] != NULL) {
      ostringstream oss;
      oss << kind() << " '" << name() << "' Received PIF request: " << request << " before waiting sufficiently long \
             for RSP_NACC for previous request: " << *m_transactor.m_nacc_nb_write_request[m_port_num];
      throw xtsc_exception(oss.str());
  }

  if (m_transactor.is_read_access(&request) && m_transactor.m_nacc_nb_read_request[m_port_num] != NULL) {
      ostringstream oss;
      oss << kind() << " '" << name() << "' Received PIF request: " << request << " before waiting sufficiently long \
             for RSP_NACC for previous request: " << *m_transactor.m_nacc_nb_read_request[m_port_num];
      throw xtsc_exception(oss.str());
  }

  XTSC_DEBUG(m_transactor.m_text, "Create a copy of input PIF request: " << request);
  xtsc_request *p_pif_request = m_transactor.copy_request(request);

  if ( !m_transactor.is_read_access(&request) && (m_transactor.m_write_request_fifo[m_port_num]->size() == m_transactor.m_write_request_fifo_depth) ) 
  {
    //Check for queue fifo size in another delta cycle
    m_transactor.m_nacc_nb_write_request[m_port_num] = p_pif_request;
    m_transactor.m_nacc_nb_write_request_thread_event[m_port_num]->notify(SC_ZERO_TIME);  
    return;
  }

  if ( m_transactor.is_read_access(&request) && (m_transactor.m_read_request_fifo[m_port_num]->size() == m_transactor.m_read_request_fifo_depth) ) 
  {
    //Check for queue fifo size in another delta cycle
    m_transactor.m_nacc_nb_read_request[m_port_num] = p_pif_request;
    m_transactor.m_nacc_nb_read_request_thread_event[m_port_num]->notify(SC_ZERO_TIME);  
    return;
  }

  nb_request_worker(p_pif_request);
  return;
}


void xtsc_component::xtsc_pif2axi_transactor::xtsc_request_if_impl::nb_request_worker(xtsc::xtsc_request* p_pif_request) 
{
  if (m_transactor.is_read_access(p_pif_request)) { //includes RCW also
    req_sched_info *p_req_sched_info = new req_sched_info;
    p_req_sched_info->m_pif_request  = p_pif_request;
    p_req_sched_info->m_sched_time   = sc_time_stamp() + m_transactor.m_read_request_delay * m_transactor.m_clock_period;
    m_transactor.m_read_request_fifo[m_port_num]->push_back(p_req_sched_info);
    m_transactor.m_read_request_thread_event[m_port_num]->notify(m_transactor.m_read_request_delay * m_transactor.m_clock_period);

    XTSC_DEBUG(m_transactor.m_text, "Pushed into m_read_request_fifo[" << m_port_num << "] free slots: " <<
                                     m_transactor.m_read_request_fifo_depth - m_transactor.m_read_request_fifo[m_port_num]->size() );

  } 
  else { 
    req_sched_info *p_req_sched_info = new req_sched_info;
    p_req_sched_info->m_pif_request  = p_pif_request;
    p_req_sched_info->m_sched_time   = sc_time_stamp() + m_transactor.m_write_request_delay * m_transactor.m_clock_period;
    m_transactor.m_write_request_fifo[m_port_num]->push_back(p_req_sched_info);
    m_transactor.m_write_request_thread_event[m_port_num]->notify(m_transactor.m_write_request_delay * m_transactor.m_clock_period);

    XTSC_DEBUG(m_transactor.m_text, "Pushed into m_write_request_fifo[" << m_port_num << "] free slots: "
                                    << m_transactor.m_write_request_fifo_depth - m_transactor.m_write_request_fifo[m_port_num]->size() );
  }

  //queue to maintain order of requests
  m_transactor.m_pif_request_order_dq[m_port_num]->push_back(p_pif_request->get_tag());

  return;  
} 


void xtsc_component::xtsc_pif2axi_transactor::xtsc_request_if_impl::nb_peek(xtsc_address address8, u32 size8, u8 *buffer) 
{
  (*m_transactor.m_read_request_ports[m_port_num])->nb_peek(address8, size8, buffer);

  u32 buf_offset  = 0;
  ostringstream oss;
  oss << hex << setfill('0');
  for (u32 i = 0; i<size8; ++i) {
    oss << setw(2) << (u32) buffer[buf_offset] << " ";
    buf_offset += 1;
  }
  XTSC_VERBOSE(m_transactor.m_text, "nb_peek: " << " [0x" << hex << address8 << "/" << dec << size8 << "] = " 
                                 << oss.str() << " Read Port #" << m_port_num);
} 


void xtsc_component::xtsc_pif2axi_transactor::xtsc_request_if_impl::nb_poke(xtsc_address address8, u32 size8, const u8 *buffer) 
{
  u32 buf_offset  = 0;
  ostringstream oss;
  oss << hex << setfill('0');
  for (u32 i = 0; i<size8; ++i) {
    oss << setw(2) << (u32) buffer[buf_offset] << " ";
    buf_offset += 1;
  }
  XTSC_VERBOSE(m_transactor.m_text, "nb_poke: " << " [0x" << hex << address8 << "/" << dec << size8 << "] = " 
                                 << oss.str() << " Write Port #" << m_port_num);

  (*m_transactor.m_write_request_ports[m_port_num])->nb_poke(address8, size8, buffer);
} 


bool xtsc_component::xtsc_pif2axi_transactor::xtsc_request_if_impl::nb_fast_access(xtsc_fast_access_request &fast_request) 
{
  XTSC_VERBOSE(m_transactor.m_text, "nb_fast_access() called  Port #" << m_port_num);
  return (*m_transactor.m_read_request_ports[m_port_num])->nb_fast_access(fast_request);
}
 

void xtsc_component::xtsc_pif2axi_transactor::xtsc_request_if_impl::register_port(sc_port_base& port, const char *if_typename) 
{
  if (m_p_port) {
    ostringstream oss;
    oss << "Illegal multiple binding detected to xtsc_pif2axi_transactor::m_request_exports[" << m_port_num << "] of '"
        << m_transactor.name() << "': " << endl;
    oss << "  " << port.name() << endl;
    oss << "  " << m_p_port->name();
    throw xtsc_exception(oss.str());
  }
  XTSC_INFO(m_transactor.m_text, "Binding '" << port.name() << "' to xtsc_pif2axi_transactor::m_request_exports[" << 
                                  m_port_num << "]");
  m_p_port = &port;
}



bool xtsc_component::xtsc_pif2axi_transactor::xtsc_respond_if_impl::nb_respond(const xtsc_response& response) 
{
  XTSC_DEBUG(m_transactor.m_text, "AXI response: " << response << ((m_type=="READ") ? " Read" : " Write") << " Export #" << m_port_num);
  XTSC_INFO(m_transactor.m_text, response << ((m_type=="READ") ? " Read" : " Write") << " Export #" << m_port_num );

  const xtsc_response *p_axi_response = &response;
  if (p_axi_response->get_status() == xtsc_response::RSP_NACC) {

    if (m_type == "READ" && m_transactor.m_read_waiting_for_nacc[m_port_num]) {
      m_transactor.m_read_request_got_nacc[m_port_num] = true;
      return true;
    } 
    else if (m_type == "WRITE" && m_transactor.m_write_waiting_for_nacc[m_port_num]) {
      m_transactor.m_write_request_got_nacc[m_port_num] = true;
      return true;
    }
    else {
      ostringstream oss;
      oss << m_transactor.kind() << " '" << m_transactor.name() << "' received nacc too late: " << *p_axi_response << endl;
      oss << " - Possibly something is wrong with the downstream device" << endl;
      oss << " - Possibly this device's \"clock_period\" needs to be adjusted";
      throw xtsc_exception(oss.str());
    }
  }

  //Get corresponding pif request from the axi request
  u64 pif_tag;
  u64 axi_tag                   = p_axi_response->get_tag();
  xtsc_request *p_pif_request   = NULL;
  req_rsp_info *p_req_rsp_info  = NULL;
  bool return_to_pool           = false;

  if ((m_transactor.m_axi_tag_from_pif_tag_map[m_port_num])->find(axi_tag) == (m_transactor.m_axi_tag_from_pif_tag_map[m_port_num])->end()) {
    ostringstream oss;
    oss << kind() << " '" << name() << "': " << "Unknown pif request for axi response with tag: "  << axi_tag << endl;
    throw xtsc_exception(oss.str());
  }
  pif_tag = (*m_transactor.m_axi_tag_from_pif_tag_map[m_port_num])[axi_tag];

  if ((m_transactor.m_tag_to_req_rsp_info_map[m_port_num])->find(pif_tag) != (m_transactor.m_tag_to_req_rsp_info_map[m_port_num])->end()) { 
    p_req_rsp_info              = (*m_transactor.m_tag_to_req_rsp_info_map[m_port_num])[pif_tag];
    p_pif_request               = p_req_rsp_info->m_pif_request_vec.front();
  } else {
    ostringstream oss;
    oss << kind() << " '" << name() << "': " << "Programming error : Unknown pif request with tag: "  << pif_tag << endl;
    throw xtsc_exception(oss.str());
  }

  XTSC_TRACE(m_transactor.m_text, "Check response for PIF request: " << *p_pif_request);
  switch (p_pif_request->get_type()) {

    // READ: validate then send response with data 
    case xtsc_request::READ :
    case xtsc_request::BLOCK_READ :
    case xtsc_request::BURST_READ : 
       {
         u32 pif_resp_data_offset = p_req_rsp_info->m_pif_resp_data_offset;
         u32 axi_size8            = (m_transactor.m_axi_width8 >= p_pif_request->get_byte_size()) ? 
                                     p_pif_request->get_byte_size() : m_transactor.m_axi_width8;
         if (pif_resp_data_offset == 0) {
           //Create Read response for pif
           bool last = (p_req_rsp_info->m_num_pif_resp_sent==p_pif_request->get_num_transfers()-1) ?true:false;
           xtsc_response *p_pif_response = new xtsc_response(*p_pif_request, p_axi_response->get_status(), last);
           p_pif_response->set_transfer_number(p_req_rsp_info->m_num_pif_resp_sent+1);
           p_pif_response->set_exclusive_ok(p_axi_response->get_exclusive_ok());
           memcpy(p_pif_response->get_buffer(), p_axi_response->get_buffer(), axi_size8);
           pif_resp_data_offset += axi_size8;
           p_req_rsp_info->m_active_read_response = p_pif_response;
           XTSC_DEBUG(m_transactor.m_text, "Created PIF response: " << *p_pif_response);
         } 
         else {
           memcpy(p_req_rsp_info->m_active_read_response->get_buffer() + pif_resp_data_offset, 
                  p_axi_response->get_buffer(), axi_size8);
           XTSC_DEBUG(m_transactor.m_text, "Filled data in PIF response: " << *p_req_rsp_info->m_active_read_response
                                           << "  at offset: " << pif_resp_data_offset);
           pif_resp_data_offset += axi_size8;
         }
         p_req_rsp_info->m_pif_resp_data_offset = pif_resp_data_offset;

         if (pif_resp_data_offset % p_pif_request->get_byte_size() == 0) {
           sc_time delay = m_transactor.m_read_response_delay * m_transactor.m_clock_period;
           m_transactor.m_rd_response_peq[m_port_num]->notify(*(p_req_rsp_info->m_active_read_response), delay); 
           XTSC_DEBUG(m_transactor.m_text, "Notify m_rd_response_peq[" << m_port_num << "] after a delay of: (" 
                                           << m_transactor.m_read_response_delay << "*" << m_transactor.m_clock_period << ")");

           //A kind of semaphore, to maintain that Write PIF response is not 
           //interleaved between Read PIF responses
           if (!m_transactor.m_interleave_responses) {
             m_transactor.m_response_locked[m_port_num] = true;
           }

           p_req_rsp_info->m_active_read_response          = NULL;
           p_req_rsp_info->m_pif_resp_data_offset          = 0;
           p_req_rsp_info->m_num_pif_resp_sent            += 1;

           if (p_req_rsp_info->m_num_pif_resp_sent == p_pif_request->get_num_transfers()) {
             return_to_pool                                = true;
             m_transactor.m_outstanding_reads[m_port_num] -= 1;
           } 
         }
         break;
       }

    case xtsc_request::RCW : 
       {
         u32 size8 = p_pif_request->get_byte_size();
        
         if (!m_transactor.m_excl_read_resp_rcvd[m_port_num]) {
           //Resp for AXI BURST_READ 
           m_transactor.m_excl_read_resp_rcvd[m_port_num]  = true;
           p_req_rsp_info->m_excl_lock_supported           = !strcmp(p_axi_response->get_status_name(),"EXOKAY");
           p_req_rsp_info->m_excl_read_match               = !memcmp(p_axi_response->get_buffer(), &p_req_rsp_info->m_rcw_compare_data, size8);

           if (!p_req_rsp_info->m_excl_lock_supported) {  
             //OKAY for read
             //Exclusive not supported by axi target
             //Dont issue write request to axi. Send back address error in response to complete RCW
             XTSC_DEBUG(m_transactor.m_text, "Exclusive BURST_READ Unsupported. Returning address error.");
             xtsc_response *p_pif_response    = new xtsc_response(*p_pif_request, xtsc_response::RSP_ADDRESS_ERROR);
             sc_time delay                    = m_transactor.m_read_response_delay * m_transactor.m_clock_period;
             m_transactor.m_rd_response_peq[m_port_num]->notify(*p_pif_response, delay);
             XTSC_DEBUG(m_transactor.m_text, "Notify m_rd_response_peq[" << m_port_num << "] after a delay of: (" 
                                             << m_transactor.m_read_response_delay << "*" << m_transactor.m_clock_period << ")");
             return_to_pool                   = true;
             m_transactor.m_outstanding_reads[m_port_num] -= 1;
           }
           else if (p_req_rsp_info->m_excl_read_match == 0) {
             //EXOKAY for read
             //Exclusive supported by axi target
             //Excl Lock not established. Dont issue write request to axi. Send back memory data in response to complete RCW 
             XTSC_DEBUG(m_transactor.m_text, "Exclusive BURST_READ Unsuccessful. Returning memory read data.");
             xtsc_response *p_pif_response    = new xtsc_response(*(p_req_rsp_info->m_pif_request_vec[1]));
             memcpy(p_pif_response->get_buffer(), p_axi_response->get_buffer(), size8);
             sc_time delay                    = m_transactor.m_read_response_delay * m_transactor.m_clock_period;
             m_transactor.m_rd_response_peq[m_port_num]->notify(*p_pif_response, delay);
             XTSC_DEBUG(m_transactor.m_text, "Notify m_rd_response_peq[" << m_port_num << "] after a delay of: (" 
                                             << m_transactor.m_read_response_delay << "*" << m_transactor.m_clock_period << ")");
             return_to_pool                   = true;
             m_transactor.m_outstanding_reads[m_port_num] -= 1;
           }
           else
             XTSC_DEBUG(m_transactor.m_text, "Exclusive BURST_READ Successful. Sending exclusive write request.");

         } 
         else {
           //Resp for AXI BURST_WRITE 
           p_req_rsp_info->m_num_expected_write_resp--;
           if (p_req_rsp_info->m_num_expected_write_resp == 0) {
             //Create Write response for RCW 
             xtsc_response *p_pif_response     = new xtsc_response(*(p_req_rsp_info->m_pif_request_vec[1]));
             XTSC_DEBUG(m_transactor.m_text, "Created PIF response: " << *p_pif_response);
        
             //Prepare return data from RCW response
             if (!strcmp(p_axi_response->get_status_name(),"EXOKAY")) {
               XTSC_DEBUG(m_transactor.m_text, "Exclusive BURST_WRITE Successful. Returning rcw compare data");
               memcpy(p_pif_response->get_buffer(), &p_req_rsp_info->m_rcw_compare_data, size8);
             }
             else {
               u32 compl_rcw_compare_data      = ~p_req_rsp_info->m_rcw_compare_data;
               XTSC_DEBUG(m_transactor.m_text, "Exclusive BURST_WRITE Unsuccessful. Returning complement of rcw compare data");
               memcpy(p_pif_response->get_buffer(), &compl_rcw_compare_data, size8);
             }
        
             sc_time delay = m_transactor.m_read_response_delay * m_transactor.m_clock_period;
             m_transactor.m_rd_response_peq[m_port_num]->notify(*p_pif_response, delay);
             XTSC_DEBUG(m_transactor.m_text, "Notify m_rd_response_peq[" << m_port_num << "] after a delay of: (" <<
                                              m_transactor.m_read_response_delay << "*" << m_transactor.m_clock_period << ")");
             return_to_pool                    = true;
             m_transactor.m_outstanding_writes[m_port_num] -= 1;
           }
         }
         break;
       }

    case xtsc_request::WRITE : 
    case xtsc_request::BLOCK_WRITE : 
    case xtsc_request::BURST_WRITE :
      if (m_transactor.m_response_locked[m_port_num]) {
        XTSC_INFO(m_transactor.m_text, *p_axi_response << ((m_type=="READ") ? " Read" : " Write") << " Export #" << 
                                        m_port_num << " (Rejected: BLOCK_READ/BURST_READ response ongoing)");
        return false;
      }
     
      p_req_rsp_info->m_num_expected_write_resp--;
     
      XTSC_TRACE(m_transactor.m_text, "Response for WRITE, pending responses: " << p_req_rsp_info->m_num_expected_write_resp);
     
      if (p_req_rsp_info->m_num_expected_write_resp == 0) {
        xtsc_response *p_pif_response = new xtsc_response(*p_pif_request, p_axi_response->get_status(), true);
        p_pif_response->set_exclusive_ok(p_axi_response->get_exclusive_ok());
        XTSC_DEBUG(m_transactor.m_text, "Created PIF response: " << *p_pif_response);
     
        sc_time delay = m_transactor.m_write_response_delay * m_transactor.m_clock_period;
        m_transactor.m_wr_response_peq[m_port_num]->notify(*p_pif_response, delay);
        XTSC_DEBUG(m_transactor.m_text, "Notify m_wr_response_peq[" << m_port_num << "] after a delay of: (" 
                                        << m_transactor.m_write_response_delay << "*" << m_transactor.m_clock_period << ")");
     
        return_to_pool = true;
        m_transactor.m_outstanding_writes[m_port_num] -= 1;
        m_transactor.m_outstanding_write_event[m_port_num]->notify();
      }
      break;

    // We covered all the cases, but just in case . . .
    default: {
      ostringstream oss;
      oss << kind() << " '" << name() << "' Unsupported request type=" << p_pif_request->get_type_name();
      throw xtsc_exception(oss.str());
    }
  }

  if (return_to_pool) {
    //Release lock for Write responses on this port
    m_transactor.m_response_locked[m_port_num] = false;

    //For write, only first axi_request and pif_request were bookkept and hence required to be deleted
    // std::map<xtsc::u64, vector<xtsc::xtsc_request*> >::iterator it;
    XTSC_TRACE(m_transactor.m_text, "Cleanup..Return requests with tag: " << axi_tag << " to shared pool");
    vector<xtsc_request*> vec = (*m_transactor.m_tag_to_axi_request_map[m_port_num])[pif_tag];
    xtsc_request* p_axi_request;
    for (u32 i = 0; i < vec.size(); i++) {
      p_axi_request = vec[i];
      m_transactor.delete_request(p_axi_request);
    }
    (m_transactor.m_tag_to_axi_request_map[m_port_num])->erase(pif_tag);
    (m_transactor.m_axi_tag_from_pif_tag_map[m_port_num])->erase(axi_tag);

    vec = p_req_rsp_info->m_pif_request_vec;
    xtsc_request *p_pif_request;
    for (u32 i = 0; i < vec.size(); i++) {
      p_pif_request = vec[i];
      m_transactor.delete_request(p_pif_request);
    }
    m_transactor.delete_req_rsp_info(p_req_rsp_info);
    (m_transactor.m_tag_to_req_rsp_info_map[m_port_num])->erase(pif_tag);

    //Iteration required as responses can be un-ordered
    for (std::deque<xtsc::u64>::iterator it = m_transactor.m_pif_request_order_dq[m_port_num]->begin() ; 
         it!= m_transactor.m_pif_request_order_dq[m_port_num]->end();) {
      it = (*it == pif_tag) ? m_transactor.m_pif_request_order_dq[m_port_num]->erase(it) : ++it;
    }
  }
  return true; 
}


void xtsc_component::xtsc_pif2axi_transactor::xtsc_respond_if_impl::register_port(sc_port_base& port, const char *if_typename) 
{
  if (m_p_port) {
    ostringstream oss;
    oss << "Illegal multiple binding detected to xtsc_pif2axi_transactor '" << m_transactor.name() << "' " << name() << ": " << endl;
    oss << "  " << port.name() << endl;
    oss << "  " << m_p_port->name();
    throw xtsc_exception(oss.str());
  }
  XTSC_INFO(m_transactor.m_text, "Binding '" << port.name() << "' to xtsc_pif2axi_transactor::" << name() << " Port #" << m_port_num);
  m_p_port = &port;
}

