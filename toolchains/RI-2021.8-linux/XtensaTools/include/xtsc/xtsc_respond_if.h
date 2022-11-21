#ifndef _XTSC_RESPOND_IF_H_
#define _XTSC_RESPOND_IF_H_

// Copyright (c) 2005-2010 by Tensilica Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Tensilica Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Tensilica Inc.

/**
 * @file 
 */


#include <xtsc/xtsc_types.h>


namespace xtsc {

class xtsc_response;


/**
 * Interface for sending responses from a memory interface slave back to the requesting
 * memory interface master.
 *
 * This interface is for communication from a memory interface slave module (for
 * example, xtsc_component::xtsc_memory) back to the memory interface master module (for
 * example, xtsc_core) 
 *
 * Every memory interface master must have two ports.  These two ports are referred to
 * thoughtout the XTSC documentation as a "memory interface master port pair" or
 * simply a "master port pair".  They are:
 *  \verbatim
    1. sc_port  <xtsc_request_if> (for sending requests)
    2. sc_export<xtsc_respond_if> (for receiving responses)
    \endverbatim
 *
 * Correspondingly, every memory interface slave must have two ports.  These two ports
 * are referred to thoughtout the XTSC documentation as a "memory interface slave port
 * pair" or simply a "slave port pair".  They are:
 *  \verbatim
    1. sc_export<xtsc_request_if> (for receiving requests)
    2. sc_port  <xtsc_respond_if> (for sending responses)
    \endverbatim
 *
 * For more information, please see xtsc_request_if.
 *
 * Note: The methods of xtsc_respond_if are all non-blocking in the OSCI TLM sense.
 *       That is, they must NEVER call wait() either directly or indirectly.
 *       The "nb_" method prefix stands for Non-Blocking.
 *
 * @see xtsc_response
 * @see xtsc_request_if
 * @see xtsc_request
 * @see xtsc_core::How_to_do_port_binding
 * @see xtsc_core::Information_on_memory_interface_protocols.
 * @see xtsc_component::xtsc_arbiter
 * @see xtsc_component::xtsc_dma_engine
 * @see xtsc_component::xtsc_master
 * @see xtsc_component::xtsc_memory
 * @see xtsc_component::xtsc_router
 * @see xtsc_component::xtsc_slave
 */
class XTSC_API xtsc_respond_if : virtual public sc_core::sc_interface {
public:

  /**
   * This is method is used by a slave module to either reject the master module's
   * request (xtsc_response::get_status() == xtsc_response::RSP_NACC) or to respond to
   * that request (xtsc_response::get_status() != xtsc_response::RSP_NACC).
   *
   * In the latter case, if the master module is busy and cannot accept the response
   * during the current clock cycle, the master module should return false to the 
   * nb_respond() call.
   *
   * @param     response                The xtsc_response object.
   *
   * Note: If nb_respond() is called with status == xtsc_response::RSP_NACC (which
   *       indicates the slave module is busy and cannot accept the request), then 
   *       nb_respond() must return true.  This allows nb_respond() with a status of
   *       xtsc_response::RSP_NACC to be called from xtsc_request_if::nb_request().
   *       Typically, the nb_respond() method should not be called from 
   *       xtsc_request_if::nb_request() with a status of other than 
   *       xtsc_response::RSP_NACC.  An exception to this rule is when the module
   *       is being operated in untimed mode (e.g. see parameter "immediate_timing"
   *       in xtsc_component::xtsc_memory_parms); however, in this case the module
   *       should throw an exception if the nb_respond() call returns false.
   *
   * Note: The caller module owns the response object.  If the callee module needs
   *       access to the response after returning from the nb_respond() call, then the
   *       callee module must make its own copy of the response object.
   * 
   * This non-blocking method must never call the SystemC wait() method (either
   * directly or indirectly).
   *
   * @see xtsc_response
   * @see xtsc_core::Information_on_memory_interface_protocols for protocol and timing
   *      issues specific to xtsc_core.
   */
  virtual bool nb_respond(const xtsc_response& response) = 0;

};



} // namespace xtsc


#endif  // _XTSC_RESPOND_IF_H_
