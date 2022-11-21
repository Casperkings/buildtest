#ifndef _XTSC_WIRE_WRITE_IF_H_
#define _XTSC_WIRE_WRITE_IF_H_

// Copyright (c) 2006-2010 by Tensilica Inc.  ALL RIGHTS RESERVED.
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



/**
 * Interface for writing (driving/sourcing) a wire.
 *
 * This TLM-level interface is for connecting between a wire driver (e.g. an export
 * state or system-level output of xtsc_core) and a wire sink (e.g. an
 * xtsc_component::xtsc_wire or system-level input of xtsc_core).  The wire driver has
 * an sc_port<xtsc_wire_write_if> used to connect to the wire sink which either inherits
 * from this interface or has an sc_export<xtsc_wire_write_if>.
 *
 * Note: The methods of xtsc_wire_write_if are all non-blocking in the OSCI 
 *       TLM sense.  That is, they must NEVER call wait() either directly or 
 *       indirectly.  The "nb_" method prefix stands for Non-Blocking.
 *
 * @see xtsc_wire_read_if
 * @see xtsc_core::How_to_do_port_binding
 * @see xtsc_component::xtsc_wire
 * @see xtsc_component::xtsc_wire_logic
 * @see xtsc_component::xtsc_wire_source
 */
class XTSC_API xtsc_wire_write_if : virtual public sc_core::sc_interface {
public:


  /**
   * This method is called by the wire driver to write a new value to the wire.
   *
   * For the typical case of an xtsc_core TIE export state, this method is called
   * on each clock cycle in which the Xtensa core writes a value to the TIE_xxx
   * interface (where xxx is the state name in the user TIE code).
   *
   * @param     value   The sc_unsigned object containing the value to be written.  
   *
   * This non-blocking method must never call the SystemC wait() method (either
   * directly or indirectly).
   *
   */
  virtual void nb_write(const sc_dt::sc_unsigned& value) = 0;



  /**
   * Get the wire width in bits.
   *
   * This method allows the wire driver to confirm that the implementation
   * is using the correct size for the wire.  
   *
   * This non-blocking method must never call the SystemC wait() method (either
   * directly or indirectly).
   *
   */
  virtual u32 nb_get_bit_width() = 0;


};





} // namespace xtsc

#endif  // _XTSC_WIRE_WRITE_IF_H_
