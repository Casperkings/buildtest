#ifndef _XTSC_VP_H_
#define _XTSC_VP_H_

// Copyright (c) 2006-2013 by Tensilica Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Tensilica Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Tensilica Inc.

/**
 * @file 
 */

#include <scml.h>
#include <xtsc/xtsc.h>
#include <xtsc/xtsc_core.h>
#include <xtsc/xtsc_mode_switch_if.h>



namespace xtsc_vp {



///  Method to initialize XTSC_VP
XTSC_VP_API void xtsc_vp_initialize();



///  Method to finalize XTSC_VP
XTSC_VP_API void xtsc_vp_finalize();



/**
 * Get or create the named VCD sc_trace_file.
 *
 * The first time this method is called for the named sc_trace_file, it will be created.
 * Subsequent calls with the same name will return the already created sc_trace_file.
 */
XTSC_VP_API sc_core::sc_trace_file *xtsc_vp_get_trace_file(const std::string& name);



/// Allow integration layer to hook into the ISS thread
XTSC_VP_API void xtsc_vp_register_iss_thread_callbacks(xtsc::xtsc_core &core, 
                                                       void *arg,
                                                       void (*before_step_cpu)(void*),
                                                       bool (*after_step_cpu)(void*));



/// Print the debug cheat sheet but only print Xplorer info the first time
XTSC_VP_API void xtsc_vp_print_debug_cheat_sheet(log4xtensa::TextLogger& logger, xtsc::u32 debugger_port, const char *target_program);



/**
 * For putting registers in a modified alphabetical order:
 *   1)  Case-insensitive
 *   2)  Numbered registers appear in numeric order.  For example
 *       registers AR10, AR11, ..., AR19 all appear after register
 *       AR9 (instead of after register AR1 and before register
 *       AR2--which would be strictly alphabetical).
 */
XTSC_VP_API bool xtsc_vp_is_first_less_then_second(const std::string& first, const std::string& second);



/**
 * Return true if name consists of a non-zero number of uppercase letters
 * followed by a non-zero number of decimal digits.
 * When returning true set alpha to the number of uppercase letters and
 * set numeric to the number of decimal digits
 */
XTSC_VP_API bool xtsc_vp_is_numbered_register(const std::string& name, xtsc::u32& alpha, xtsc::u32& numeric);



/// Convert std::string to a c-str array by tokenizing on commas
XTSC_VP_API char **xtsc_vp_create_c_str_array(const std::string &csv);



/**
 * Interface class so all xtsc_core_vp instances can be notified when the lead (first 
 * instantiated) xtsc_core_vp instance is getting ready to complete and completes a
 * switch between TurboXim modes (fast-functional and cycle-accurate).
 */
class XTSC_VP_API xtsc_vp_turboxim_switch_listener_interface {
public:
  /**
   * Method to be called just before and just after a sim mode switch completes.
   *
   * @param     before          If true then the call is occuring just before the switch.
   *                            If false then the call is occuring just after the switch.
   *
   * @param     new_mode        The mode after the switch completes.
   */
  virtual void sim_mode_switch_completion(bool before, xtsc::xtsc_sim_mode new_mode) = 0;
};



/**
 *  Called by all xtsc_core_vp instances at construction time.
 *
 *  The first instance to call this method will get a return value of 1 to indicate it
 *  is responsible for actually doing the TurboXim mode switch protocol and then calling
 *  xtsc_vp_notify_turboxim_switch_listeners() with the new mode.  Listeners whose
 *  registration number is not 1 do not perform the mode switch protocol.  Instead, they
 *  rely on the lead xtsc_core_vp instance (the one whose registration number is 1) to
 *  perform the switch protocol and then to notify them by calling 
 *  xtsc_vp_notify_turboxim_switch_listeners() twice; the first time just before the
 *  switch completes and the second time just after the switch completes (no SystemC
 *  time may elapse in between the two calls).
 *
 *  @returns registration/listener number (starting with 1)
 */
XTSC_VP_API xtsc::u32 xtsc_vp_register_turboxim_switch_listener(xtsc_vp_turboxim_switch_listener_interface& listener);



/**
 *  Called by lead (first instantiated) xtsc_core_vp instance when mode switch completes.
 *
 *  The implementation of this function will call all registered listeners with
 *     sim_mode_switch_completion(before, new_mode);
 *
 *  @param      before          If true then the call is occuring just before the switch.
 *                              If false then the call is occuring just after the switch.
 *
 *  @param      new_mode        The mode after the switch completes.
 */
XTSC_VP_API void xtsc_vp_notify_turboxim_switch_listeners(bool before, xtsc::xtsc_sim_mode new_mode);



}  // namespace xtsc_vp




#endif  // _XTSC_VP_H_
