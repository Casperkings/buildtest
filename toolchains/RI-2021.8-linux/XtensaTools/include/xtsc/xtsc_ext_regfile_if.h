#ifndef _XTSC_EXT_REGFILE_IF_H_
#define _XTSC_EXT_REGFILE_IF_H_

// Copyright (c) 2006-2020 by Tensilica Inc.  ALL RIGHTS RESERVED.
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
 * Interface for connecting a TIE external regfile client to an implementation.
 *
 * This interface is for connecting between a TIE external regfile client 
 * (typically an xtsc_core) and a TIE external regfile implementation provided
 * by the user. The TIE external regfile client has sc_port<xtsc_ext_regfile_if>
 * used to connect to the TIE external regfile implementation which inherits from
 * this interface.
 *
 * Note: The methods of xtsc_ext_regfile_if are all non-blocking in the 
 *       OSCI TLM sense.
 *       That is, they must NEVER call wait() either directly or indirectly.
 *       The "nb_" method prefix stands for Non-Blocking.
 *
 * @see xtsc_core::How_to_do_port_binding
 * @see xtsc_core::get_ext_regfile
 * @see xtsc_component::xtsc_ext_regfile
 */
class XTSC_API xtsc_ext_regfile_if : virtual public sc_core::sc_interface {
public:


  /**
   * This method is called to send the TIE external regfile write address and
   * data. A write operation is intended to copy operands and start the engine.
   *
   * For the typical case of an xtsc_core TIE external regfile client, this 
   * method is called on each clock cycle in which the Xtensa core is driving 
   * the TIE_xxx_Write signal.
   * (where xxx is the external regfile name in the user TIE code).
   *
   * @param     address         External regfile write address.
   *                            For an xtsc_core TIE external regfile client, 
   *                            this parameter corresponds to the 
   *                            TIE_xxx_WriteAddr and  TIE_xxx_WriteData 
   *                            signals, where xxx is the external regfile name
   *                            in the user TIE code.
   *
   * This non-blocking method must never call the SystemC wait() method (either
   * directly or indirectly).
   *
   */
  virtual void nb_send_write_address_and_data(xtsc::u32 address,
                                            const sc_dt::sc_unsigned& data) = 0;


  /**
   * This method is called by the TIE external regfile client to send read check
   * address.
   *
   * For the typical case of an xtsc_core TIE external regfile client, this 
   * method is called on each clock cycle in which the Xtensa core is driving 
   * the TIE_xxx_ReadCheck signal.
   * (where xxx is the external regfile name in the user TIE code).  If the 
   * signal TIE_xxx_ReadStall is high, then this method will be re-called by 
   * xtsc_core on each clock cycle till after TIE_xxx_ReadStall goes low.
   *
   * @param     address         External regfile read-check address.
   *                            For an xtsc_core TIE external regfile client, 
   *                            this parameter corresponds to the 
   *                            TIE_xxx_ReadCheckAddr signal, where xxx is the 
   *                            external regfile name in the user TIE code.
   *
   * This non-blocking method must never call the SystemC wait() method (either
   * directly or indirectly).
   *
   */
  virtual void nb_send_read_check_address(xtsc::u32 address) = 0;



  /**
   * This method is called by the TIE external regfile client when it needs to 
   * determine if the read data is ready to be read. If the data is not ready, 
   * the read stall is asserted and is sampled in the same cycle as 
   * nb_send_read_check_address(). 
   *
   * @return true if the TIE external regfile's read data is ready.
   *
   * This non-blocking method must never call the SystemC wait() method (either
   * directly or indirectly).
   *
   */
  virtual bool nb_is_read_data_ready() = 0;


  /**
   * This method is called by the TIE external regfile client to request that a
   * read address be sent. This method is called if nb_send_read_check_address()
   * has been sent and nb_is_read_data_ready() has returned true for the address.
   *
   * For the typical case of an xtsc_core TIE external regfile client, this 
   * method is called on each clock cycle in which the Xtensa core is driving 
   * the TIE_xxx_Read signal.
   * (where xxx is the external regfile name in the user TIE code).
   *
   * @param     address         External regfile read address.
   *                            For an xtsc_core TIE external regfile client, 
   *                            this parameter corresponds to the 
   *                            TIE_xxx_ReadAddr signal, where xxx is the 
   *                            external regfile name in the user TIE code.
   *
   * This non-blocking method must never call the SystemC wait() method (either
   * directly or indirectly).
   *
   */
  virtual void nb_send_read_address(xtsc::u32 address) = 0;


  /**
   * This method is called to get the TIE external regfile read data. It must be
   * called in the same cycle nb_send_read_address() is called.
   *
   * For the typical case of an xtsc_core TIE external regfile client, this 
   * method is called on each clock cycle in which the Xtensa core is driving 
   * the TIE_xxx_Read signal.
   * (where xxx is the external regfile name in the user TIE code).
   *
   * @return  The sc_unsigned object containing the response read data. For an 
   *          xtsc_core TIE external regfile client, this data corresponds to 
   *          the TIE_xxx_ReadData signal, where xxx is the external regfile 
   *          name in the user TIE code.
   *
   * This non-blocking method must never call the SystemC wait() method (either
   * directly or indirectly).
   *
   */
  virtual sc_dt::sc_unsigned nb_get_read_data() = 0;


  /**
   * Get address bit width that the external regfile implementation expects.
   *
   * This method allows the TIE external regfile client to confirm that the 
   * implementation is using the correct size for the external regfile address.
   * For an xtsc_core TIE external regfile client, the address should be the 
   * width of the TIE_xxx_ReadAddr signal.
   */
  virtual u32 nb_get_address_bit_width() = 0;


  /**
   * Get data bit width that the external regfile implementation will return.
   *
   * This method allows the TIE external regfile client to confirm that the 
   * implementation is using the correct size for the response data.  For an 
   * xtsc_core TIE external regfile client, the data returned should be the 
   * width of the TIE_xxx_ReadData signal.
   */
  virtual u32 nb_get_data_bit_width() = 0;


  /**
   * Return the external regfile-ready event.
   *
   * Clients can call this method to get a reference to an event that will be 
   * notified when the external regfile transitions from not-ready to ready.
   *
   * @Note
   * Sub-classes must override this method and return their external regfile
   * ready event.
   */
  virtual const sc_core::sc_event& default_event() const { return sc_interface::default_event(); }


};

} // namespace xtsc

#endif  // _XTSC_EXT_REGFILE_IF_H_
