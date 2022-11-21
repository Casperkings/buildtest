#ifndef _XTSC_VSP_H_
#define _XTSC_VSP_H_

// Copyright (c) 2006-2013 by Tensilica Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Tensilica Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Tensilica Inc.

/**
 * @file 
 */

#include <xtsc/xtsc.h>
#include <xtsc/xtsc_core.h>
#include <xtsc/xtsc_mode_switch_if.h>



namespace xtsc_vsp {


/**
 *  Same as xtsc::xtsc_initialize_parms except:
 *   - Default value of "turbo" is true
 *
 * @see xtsc::xtsc_initialize_parms
 */
class xtsc_vsp_initialize_parms : public xtsc::xtsc_initialize_parms {
public:

  /**
   * Constructor for an xtsc_vsp_initialize_parms object.
   *
   * @param text_logging_config_file    The value for the "text_logging_config_file"
   *                                    parameter.
   */
  xtsc_vsp_initialize_parms(const char *text_logging_config_file = NULL);

  /// Return what kind of xtsc_parms this is (our C++ type)
  const char *kind() const { return "xtsc_vsp_initialize_parms"; }
};



/**
 *  Method to initialize XTSC_VP.
 *
 *  @note This method internally calls extract_parms using "xtsc" for the id argument.
 */
void xtsc_vsp_initialize(xtsc_vsp_initialize_parms& init_parms);



///  Method to finalize XTSC_VP
void xtsc_vsp_finalize();



/**
 * Get or create the named VCD sc_trace_file.
 *
 * The first time this method is called for the named sc_trace_file, it will be created.
 * Subsequent calls with the same name will return the already created sc_trace_file.
 */
sc_core::sc_trace_file *xtsc_vsp_get_trace_file(const std::string& name);



/// Allow integration layer to hook into the ISS thread
void xtsc_vsp_register_iss_thread_callbacks(xtsc::xtsc_core &core, 
                                            void *arg,
                                            void (*before_step_cpu)(void*),
                                            bool (*after_step_cpu)(void*));



/**
 * For putting registers in a modified alphabetical order:
 *   1)  Case-insensitive
 *   2)  Numbered registers appear in numeric order.  For example
 *       registers AR10, AR11, ..., AR19 all appear after register
 *       AR9 (instead of after register AR1 and before register
 *       AR2--which would be strictly alphabetical).
 */
bool xtsc_vsp_is_first_less_then_second(const std::string& first, const std::string& second);



/**
 * Return true if name consists of a non-zero number of uppercase letters
 * followed by a non-zero number of decimal digits.
 * When returning true set alpha to the number of uppercase letters and
 * set numeric to the number of decimal digits
 */
bool xtsc_vsp_is_numbered_register(const std::string& name, xtsc::u32& alpha, xtsc::u32& numeric);



/// Convert std::string to a c-str array by tokenizing on commas
char **xtsc_vsp_create_c_str_array(const std::string &csv);



/**
 * Interface class so all xtsc_core_vsp instances can be notified when the lead (first 
 * instantiated) xtsc_core_vsp instance is getting ready to complete and completes a
 * switch between TurboXim modes (fast-functional and cycle-accurate).
 */
class xtsc_vsp_turboxim_switch_listener_interface {
public:
  virtual ~xtsc_vsp_turboxim_switch_listener_interface() {};

  /**
   * Method to be called just before and just after a sim mode switch completes.
   *
   * @param     before          If true then the call is occuring just before the switch.
   *                            If false then the call is occuring just after the switch.
   *
   * @param     new_mode        The mode after the switch completes.
   */
  virtual void sim_mode_switch_completion(bool before, xtsc::xtsc_sim_mode new_mode) = 0;

  /**
   * Method to return the event to be notified when a TurboXim sim mode switch has
   * been requested (either by the simulator user or by an Xtensa target program).  This
   * method will only be called on the lead (first instantiated) xtsc_core_vsp instance.
   */
  virtual sc_core::sc_event& get_switch_sim_mode_event() = 0;
};



/**
 *  Called by all xtsc_core_vsp instances at construction time.
 *
 *  The first instance to call this method will get a return value of 1 to indicate it
 *  is responsible for actually doing the TurboXim mode switch protocol and then calling
 *  xtsc_vsp_notify_turboxim_switch_listeners() with the new mode.  Listeners whose
 *  registration number is not 1 do not perform the mode switch protocol.  Instead, they
 *  rely on the lead xtsc_core_vsp instance (the one whose registration number is 1) to
 *  perform the switch protocol and then to notify them by calling 
 *  xtsc_vsp_notify_turboxim_switch_listeners() twice; the first time just before the
 *  switch completes and the second time just after the switch completes (no SystemC
 *  time may elapse in between the two calls).
 *
 *  @returns registration/listener number (starting with 1)
 */
xtsc::u32 xtsc_vsp_register_turboxim_switch_listener(xtsc_vsp_turboxim_switch_listener_interface& listener);



/**
 *  Called by lead (first instantiated) xtsc_core_vsp instance when mode switch completes.
 *
 *  The implementation of this function will call all registered listeners with
 *     sim_mode_switch_completion(before, new_mode);
 *
 *  @param      before          If true  then the call is occuring just before the switch.
 *                              If false then the call is occuring just after  the switch.
 *
 *  @param      new_mode        The mode after the switch completes.
 */
void xtsc_vsp_notify_turboxim_switch_listeners(bool before, xtsc::xtsc_sim_mode new_mode);



/**
 * Method to request a TurboXim sim mode switch.  The request can originate from the
 * simulator user or from an Xtensa target program.  The implementation of this method
 * will notify the lead (first instantiated) xtsc_core_vsp instance to actually perform
 * the mode switch.
 */
void xtsc_vsp_request_sim_mode_switch();



}  // namespace xtsc_vsp




#endif  // _XTSC_VSP_H_
