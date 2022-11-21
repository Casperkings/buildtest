#ifndef _XTSC_REQUEST_IF_H_
#define _XTSC_REQUEST_IF_H_

// Copyright (c) 2005-2018 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Cadence Design Systems, Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Cadence Design Systems, Inc.

/**
 * @file 
 */


#include <xtsc/xtsc_types.h>


namespace xtsc {


class xtsc_request;
class xtsc_fast_access_request;


/**
 * Interface for non-hardware communication from a memory interface master to a memory
 * interface slave.
 *
 * This interface is for non-hardware communication from a master module (for example,
 * xtsc_core) to a slave module (for example, xtsc_component::xtsc_memory or
 * xtsc_component::xtsc_router).  It also includes the nb_fast_access() method.
 *
 * The nb_peek() and nb_poke() methods are for non-hardware communication from a master
 * module to a slave module.  For example, they are used by a non-intrusive target
 * debugger to read and write memory without the read/write transaction itself
 * disturbing the hardware state of the target processor or the memory controller.  Of
 * course, if the program executing on the target processor depends upon memory data
 * that is over-written by the nb_poke() method, then processor state will change.
 *
 * The nb_fast_access() method is used by TurboXim to find out what methods are
 * available for accessing memories in fast-functional mode.
 *
 * The xtsc_request_if interface expands the xtsc_debug_if by adding the nb_request()
 * method which is used to model hardware communication .
 *
 * Note: The methods of xtsc_debug_if are all non-blocking in the OSCI 
 *       TLM sense.  That is, they must NEVER call wait() either directly or 
 *       indirectly.  The "nb_" method prefix stands for Non-Blocking.
 *
 * @see xtsc_request_if
 */
class XTSC_API xtsc_debug_if : virtual public sc_core::sc_interface {
public:


  /**
   * This method is used to read memory without disturbing the memory
   * hardware, the processor hardware, or the bus hardware.  
   *
   * @param     address8        The byte address of the first byte
   *                            to be peeked.
   *
   * @param     size8           The number of bytes to peek.
   *
   * @param     buffer          The byte array in which to return the
   *                            peeked data.  The byte at address8 is
   *                            returned in buffer[0], the byte at 
   *                            address8+1 is returned in buffer[1], and
   *                            so on up to the byte at address8+size8-1 
   *                            is returned in buffer[size8-1]. This format 
   *                            applies regardless of host and memory client 
   *                            endianess.
   *
   * Note: The implementation should support arbitrary values for address8
   *       and size8 as long as all locations map to the address space of the
   *       module.  If an attempt is made to access a location outside the
   *       address space of the module, an xtsc_exception should be thrown.
   *
   * This non-blocking method must never call the SystemC wait() method (either
   * directly or indirectly).
   */
  virtual void nb_peek(xtsc_address address8, u32 size8, u8 *buffer) = 0;


  /**
   * This method is used to write memory without disturbing the memory
   * controller hardware, the processor hardware, or the bus hardware.  
   *
   * @param     address8        The byte address of the first byte
   *                            to be poked.
   *
   * @param     size8           The number of bytes to poke.
   *
   * @param     buffer          The byte array in which to obtain the
   *                            poked data.  The byte in buffer[0]
   *                            is poked into memory at address8, the
   *                            byte in buffer[1] is poked into memory
   *                            at address8+1, and so on up to the byte
   *                            in buffer[size8-1] is poked into memory
   *                            at address8+size8-1.  This format applies 
   *                            regardless of host and memory client 
   *                            endianess.
   *
   * Note: The implementation should support arbitrary values for address8
   *       and size8 as long as all locations map to the address space of the
   *       module.  If an attempt is made to access a location outside the
   *       address space of the module, an xtsc_exception should be thrown.
   *
   * This non-blocking method must never call the SystemC wait() method (either
   * directly or indirectly).
   */
  virtual void nb_poke(xtsc_address address8, u32 size8, const u8 *buffer) = 0;


  /**
   * Get information about fast access to a given address.
   *
   * Calls to this method are used to get information about what fast access method, if
   * any, is available for the address specified in the xtsc_fast_access_request object.
   *
   * @return false to indicate that fast access is not available at this point in the
   * simulation, but may be re-requested at some later time in the simulation.
   *
   * @see xtsc_fast_access_request
   */
  virtual bool nb_fast_access(xtsc_fast_access_request &request) = 0;


  /**
   * Peek method for use by coherence devices.
   *
   * Guidelines:
   * This method is for use by cache coherence devices (specifically xtsc_core and an
   * external coherence controller model).  Terminal devices can just use the default
   * implementation of this method provided here.  Network devices such as routers and
   * arbiters should just forward the call to the appropriate downstream device (with
   * any required address translations).
   *
   * @param     virtual_address8        The virtual byte address of the first byte to be
   *                                    peeked.
   *
   * @param     physical_address8       The physical byte address of the first byte to
   *                                    be peeked.
   *
   * @param     size8                   The number of bytes to peek.  xtsc_core will
   *                                    thrown an exception if an attempt is made to
   *                                    peek more then 4 bytes at a time.
   *
   * @param     buffer                  The byte array in which to return the peeked
   *                                    data.  The byte at address8 is returned in
   *                                    buffer[0], the byte at address8+1 is returned in
   *                                    buffer[1], and so on up to the byte at
   *                                    address8+size8-1 is returned in buffer[size8-1].
   *                                    This format applies regardless of host and
   *                                    memory client endianess.
   *
   * This non-blocking method must never call the SystemC wait() method (either directly
   * or indirectly).
   *
   * Note: This method is reserved for future use.
   */
  virtual bool nb_peek_coherent(xtsc_address virtual_address8, xtsc_address physical_address8, u32 size8, u8 *buffer) {
    nb_peek(physical_address8, size8, buffer);
    return true;
  }


  /**
   * Poke method for use by coherence devices.
   *
   * Guidelines:
   * This method is for use by cache coherence devices (specifically xtsc_core and an
   * external coherence controller model).  Terminal devices can just use the default
   * implementation of this method provided here.  Network devices such as routers and
   * arbiters should just forward the call to the appropriate downstream device (with
   * any required address translations).
   *
   * @param     virtual_address8        The virtual byte address of the first byte to be
   *                                    poked.
   *
   * @param     physical_address8       The physical byte address of the first byte to
   *                                    be poked.
   *
   * @param     size8                   The number of bytes to poke.  xtsc_core will
   *                                    thrown an exception if an attempt is made to
   *                                    poke more then 4 bytes at a time.
   *
   * @param     buffer                  The byte array in which to obtain the poked
   *                                    data.  The byte in buffer[0] is poked into
   *                                    memory at address8, the byte in buffer[1] is
   *                                    poked into memory at address8+1, and so on up to
   *                                    the byte in buffer[size8-1] is poked into memory
   *                                    at address8+size8-1.  This format applies
   *                                    regardless of host and memory client endianess.
   *
   * This non-blocking method must never call the SystemC wait() method (either directly
   * or indirectly).
   *
   * Note: This method is reserved for future use.
   */
  virtual bool nb_poke_coherent(xtsc_address virtual_address8, xtsc_address physical_address8, u32 size8, const u8 *buffer) {
    nb_poke(physical_address8, size8, buffer);
    return true;
  }


};





/**
 * Interface for sending requests from a memory interface master to a memory interface
 * slave.
 *
 * This composite interface is for both debug and normal hardware communication from a
 * memory interface master module to a memory interface slave module.
 *
 * A memory interface master is a module (such as xtsc_core) that is capable of making
 * memory interface requests (for example, read, write, block read, etc.) and a memory
 * interface slave is a module (such as xtsc_component::xtsc_memory) that is capable of
 * servicing memory interface requests.
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
 * To connect a memory interface master with a memory interface slave requires two port
 * binding operations:
 *  \verbatim
    1. The master's sc_port<xtsc_request_if> must be bound to the slave's
       sc_export<xtsc_request_if>
    2. The slave's sc_port<xtsc_respond_if> must be bound to the master's
       sc_export<xtsc_respond_if>
    \endverbatim
 *
 * For port-binding information specific to xtsc_core, see
 * xtsc_core::How_to_do_memory_port_binding.
 *
 * For protocol and timing information specific to xtsc_core, see
 * xtsc_core::Information_on_memory_interface_protocols.
 *
 * Note: The methods of xtsc_request_if are all non-blocking in the OSCI 
 *       TLM sense.  That is, they must NEVER call wait() either directly or 
 *       indirectly.  The "nb_" method prefix stands for Non-Blocking.
 *
 * @see xtsc_request
 * @see xtsc_debug_if
 * @see xtsc_respond_if
 * @see xtsc_response
 * @see xtsc_core::How_to_do_port_binding
 * @see xtsc_core::Information_on_memory_interface_protocols
 * @see xtsc_component::xtsc_arbiter
 * @see xtsc_component::xtsc_dma_engine
 * @see xtsc_component::xtsc_master
 * @see xtsc_component::xtsc_memory
 * @see xtsc_component::xtsc_router
 * @see xtsc_component::xtsc_slave
 */
class XTSC_API xtsc_request_if : virtual public xtsc_debug_if {
public:


  /**
   * Calls to this method represent the request phase of real hardware bus activity.
   *
   * @param     request         The xtsc_request object.
   *
   * The nb_request() method returns void.  If the slave module (the callee) does not 
   * want to accept the request (e.g. it is already busy with another request), it 
   * must call xtsc_respond_if::nb_respond() with xtsc_response::RSP_NACC.
   *
   * Note: The caller module owns the request object.  If the callee module needs access
   *       to the request after returning from the nb_request() call, then the callee
   *       module must make its own copy of the request object.
   * 
   * This non-blocking method must never call the SystemC wait() method (either
   * directly or indirectly).
   *
   * @see xtsc_request
   * @see xtsc_core::Information_on_memory_interface_protocols for protocol and timing
   *      issues specific to xtsc_core.
   */
  virtual void nb_request(const xtsc_request& request) = 0;

  
  /**
   * This method is called to indicate that the oldest outstanding XLMI load has retired.
   *
   * This method models the DPortNLoadRetiredM signal of XLMI.  This method will never
   * be called by xtsc_core except on an XLMI port.
   *
   * @param     address8        The address of the oldest outstanding load.
   *
   * Note: A router should forward this method call out the master port indicated by the
   *       address argument.
   *
   * This non-blocking method must never call the SystemC wait() method (either
   * directly or indirectly).
   */
  virtual void nb_load_retired(xtsc_address address8) {};


  /**
   * This method is called to indicate that none of the outstanding XLMI loads will
   * commit (and therefore they should be flushed).
   *
   * This method models the DPortnRetireFlushm signal of XLMI.  This method will never
   * be called by xtsc_core except on an XLMI port.
   *
   * Note: A router should broadcast this method call out all master ports (i.e. to all
   *       slaves).
   *
   * This non-blocking method must never call the SystemC wait() method (either
   * directly or indirectly).
   */
  virtual void nb_retire_flush() {};


  /**
   * This method is called to indicate that an atomic read-followed-by-write operation
   * is in progress to the connected DataRAM.
   *
   * This method models the DRamnLockm signal of DataRAM.  This method will never
   * be called by xtsc_core except on a DataRAM port.  
   *
   * @param     lock            If true, the DRamnLockm signal is being asserted.  If
   *                            false, the DRamnLockm signal is being deasserted.
   *
   * Note: Protocol for shared DataRAM:
   *    - A router should broadcast this method call out all master ports (i.e. to
   *      all slaves).
   *    - An arbiter should maintain state of the the lock argument for each port.
   *    - An arbiter should actually lock on a port only when a request is received
   *      and granted while the lock argument state is true.
   *    - When an arbiter first locks on a port, it should send a nb_lock(true) call
   *      downstream prior to forwarding the nb_request() call.
   *    - When an arbiter receives an nb_lock(false) call on a port to which it is
   *      locked, then it should unlock the port and foward the nb_lock(false) call
   *      downstream.
   *
   * This non-blocking method must never call the SystemC wait() method (either
   * directly or indirectly).
   */
  virtual void nb_lock(bool lock) {};

};





#ifndef DOXYGEN_SKIP
// Deprecated.
class XTSC_API xtsc_retire_if : virtual public sc_core::sc_interface {
public:
  // Deprecated.  Use xtsc_request_if::nb_load_retired() and xtsc_request_if::nb_retire_flush().
  virtual void nb_retire(u32 ls_unit, bool retired) = 0;
};
#endif // DOXYGEN_SKIP



} // namespace xtsc


#endif  // _XTSC_REQUEST_IF_H_
