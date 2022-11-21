#ifndef _XTSC_SIM_MODE_SWITCH_IF_H_
#define _XTSC_SIM_MODE_SWITCH_IF_H_

// Copyright (c) 2006-2010 by Tensilica Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Tensilica Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Tensilica Inc.

/**
 * @file 
 */

#include <xtsc/xtsc.h>

namespace xtsc {

/**
 *  Type used to identify simulation modes
 */
typedef enum xtsc_sim_mode {
  XTSC_CYCLE_ACCURATE = 0,
  XTSC_FUNCTIONAL
} xtsc_sim_mode;



/**
 * Interface for dynamic simulation mode switching between fast-functional
 * and cycle-accurate modes
 *
 * Any device that needs to participate in simulation mode switching 
 * should implement this interface, create an xtsc_switch_registration
 * object, and pass that object to xtsc_register_mode_switch_interface.
 *
 * For example, have the sc_module sub-class of the device inherit from
 * and implement the xtsc_mode_switch_if and then have code like this
 * in the constructor:
 * \verbatim
   xtsc_register_mode_switch_interface(
     xtsc_switch_registration(*this, *this, "")
   );
   \endverbatim
 */
class XTSC_API xtsc_mode_switch_if {
 public:

  /**
   * Request a simulation mode switch between instruction-accurate (functional/TurboXim
   * mode) and cycle-accurate (non-TurboXim mode) simulation.  
   *
   * If a mode switch is desired after simulation has started (i.e. after the first call
   * to sc_start), the prepare_to_switch_sim_mode() method should be called on every clock
   * cycle until it returns true before this method is called.
   *
   * @param     mode            If mode is XTSC_CYCLE_ACCURATE, then switch to
   *                            cycle-accurate (non-TurboXim) mode.  If mode is
   *                            XTSC_FUNCTIONAL, then switch to functional mode
   *                            (TurboXim).
   *
   * @returns true if switch was successful.
   *
   * @see prepare_to_switch_sim_mode()
   */
  virtual bool switch_sim_mode(xtsc_sim_mode mode) = 0;


  /**
   * Test to see if a switch between simulation modes (instruction-accurate/functional/
   * TurboXim and cycle-accurate/non-TurboXim) can be made this cycle.
   * During the dynamic simulation mode switching protocol, this method also informs
   * a device that not issue any new requests until it has switched.  If the device is not
   * ready to switch it should fire its ready event (accessible with the
   * get_sim_mode_switch_ready_event()) when it becomes ready.
   * In XTSC_FUNCTIONAL mode a device can simulate a relaxed_cycle_limit number
   * of cycles out-of-order with respect to the rest of the system.  The global
   * function xtsc::xtsc_get_relaxed_simulation_cycle_limit() can be called
   * to determine the limit.
   * 
   * @param     mode            If mode is XTSC_CYCLE_ACCURATE, then switch to
   *                            cycle-accurate (non-TurboXim) mode.  If mode is
   *                            XTSC_FUNCTIONAL, then switch to functional mode
   *                            (TurboXim).
   *
   * @return true if this device is ready to switch, false if it is not
   *              yet ready.
   */
  virtual bool prepare_to_switch_sim_mode(xtsc_sim_mode mode) = 0;


  /**
   * During the dynamic simulation mode switching protocol, when a device returns not
   * ready from prepare_to_switch_sim_mode(), it should fire this event
   * when it is ready.
   *
   * @return event fired when a device is ready to switch after it
   *               reported that it was not ready.
   */
  virtual sc_core::sc_event &get_sim_mode_switch_ready_event() = 0;


  /**
   * Query function to determine the current simulation mode of the device
   * @return the current mode
   */
  virtual xtsc_sim_mode get_sim_mode() const = 0;


  /**
   * Query function to determine whether the device is stalled for
   * a simulation mode switch
   * @return true if the device has a mode switch pending
   */
  virtual bool is_mode_switch_pending() const = 0;
  

  /**
   * virtual destructor
   */
  virtual ~xtsc_mode_switch_if() {};

};

}

#endif /* _XTSC_SIM_MODE_SWITCH_IF_H_ */
