#ifndef _XTSC_LOOKUP_IF_H_
#define _XTSC_LOOKUP_IF_H_

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
 * Interface for connecting a TIE lookup client to an implementation.
 *
 * This interface is for connecting between a TIE lookup client (typically an xtsc_core)
 * and a TIE lookup implementation provided by the user.  The TIE lookup client has an
 * sc_port<xtsc_lookup_if> used to connect to the TIE lookup implementation which
 * inherits from this interface.
 *
 * Note: The methods of xtsc_lookup_if are all non-blocking in the OSCI TLM sense.
 *       That is, they must NEVER call wait() either directly or indirectly.
 *       The "nb_" method prefix stands for Non-Blocking.
 *
 * @see xtsc_core::How_to_do_port_binding
 * @see xtsc_core::get_lookup
 * @see xtsc_component::xtsc_lookup
 */
class XTSC_API xtsc_lookup_if : virtual public sc_core::sc_interface {
public:


  /**
   * This method is called by the TIE lookup client to request that a lookup be 
   * performed using the specified address.
   *
   * For the typical case of an xtsc_core TIE lookup client, this method is called on
   * each clock cycle in which the Xtensa core is driving the TIE_xxx_Out_Req signal
   * (where xxx is the lookup name in the user TIE code).  If the "rdy" keyword was
   * specified in the user TIE code, then this method will be re-called by xtsc_core on
   * each clock cycle after nb_is_ready() returns false (unless the instruction
   * performing the lookup is killed).
   *
   * @param     address         The sc_unsigned object containing the lookup address.
   *                            For an xtsc_core TIE lookup client, this parameter 
   *                            corresponds to the TIE_xxx_Out signal, where xxx is the 
   *                            lookup name in the user TIE code.
   *
   * This non-blocking method must never call the SystemC wait() method (either
   * directly or indirectly).
   *
   */
  virtual void nb_send_address(const sc_dt::sc_unsigned& address) = 0;



  /**
   * This method is called by the TIE lookup client when it needs to determine if the
   * TIE lookup request (sent using the nb_send_address() method) was accepted.
   *
   * For the typical case of an xtsc_core TIE lookup client, this method will not be
   * called unless the lookup section of the user TIE code specified the "rdy" keyword.
   * If the "rdy" keyword was specified in the user TIE code, then the TIE lookup
   * implementation must override this virtual method.
   *
   * @return true if the TIE lookup request was accepted.
   *
   * This non-blocking method must never call the SystemC wait() method (either
   * directly or indirectly).
   *
   */
  virtual bool nb_is_ready() { return false; }


  /**
   * This method is called to get the TIE lookup response data.
   *
   * This method must be called exactly one time for each nb_is_ready() call that
   * returns true.
   *
   * For hardware configurations where the nb_is_ready() call is not used (e.g. an
   * Xtensa TIE port defined without the "rdy" keyword), this method must be called
   * exactly one time for each nb_send_address() call.
   *
   * For the typical case of an xtsc_core TIE lookup client, this method is called after
   * a latency of <use_stage> minus <def_stage> clock cycles, where <use_stage>
   * and <def_stage> are the values specified in the lookup section of the user TIE 
   * code.  If the "rdy" keyword was specified in the user TIE code, then the latency
   * starts on the clock cycle in which nb_is_ready() returns true.  If the "rdy" 
   * keyword was not specified, then the latency starts on the clock cycle when 
   * nb_send_address() is called.
   *
   * @return  The sc_unsigned object containing the response data.  For an xtsc_core 
   *          TIE lookup client, this data corresponds to the TIE_xxx_In signal, where
   *          xxx is the lookup name in the user TIE code.
   *
   * This non-blocking method must never call the SystemC wait() method (either
   * directly or indirectly).
   *
   */
  virtual sc_dt::sc_unsigned nb_get_data() = 0;


  /**
   * Get the address bit width that the lookup implementation expects.
   *
   * This method allows the TIE lookup client to confirm that the implementation is
   * using the correct size for the lookup address.  For an xtsc_core TIE lookup client,
   * the data returned should be the width of the TIE_xxx_Out signal as specified by
   * <output_width> in the "lookup xxx" section of the user TIE code.
   */
  virtual u32 nb_get_address_bit_width() = 0;


  /**
   * Get the data bit width that the lookup implementation will return.
   *
   * This method allows the TIE lookup client to confirm that the implementation is
   * using the correct size for the response data.  For an xtsc_core TIE lookup client,
   * the data returned should be the width of the TIE_xxx_In signal as specified by
   * <input_width> in the the "lookup xxx" section of the user TIE code.
   */
  virtual u32 nb_get_data_bit_width() = 0;


  /**
   * Return the lookup-ready event.
   *
   * Clients can call this method to get a reference to an event that will be notified
   * when the lookup transitions from not-ready to ready.
   *
   * @Note
   * Sub-classes must override this method and return their lookup-ready event.  If the lookup
   * doesn't have a ready then the sub-class should return an event that is never notified.
   */
  virtual const sc_core::sc_event& default_event() const { return sc_interface::default_event(); }


};





} // namespace xtsc

#endif  // _XTSC_LOOKUP_IF_H_
