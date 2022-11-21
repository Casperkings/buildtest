#ifndef _XTSC_PIN2TLM_WIRE_TRANSACTOR_VP_H_
#define _XTSC_PIN2TLM_WIRE_TRANSACTOR_VP_H_

// Copyright (c) 2006-2011 by Tensilica Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Tensilica Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Tensilica Inc.

/**
 * @file 
 */

#include <xtsc/xtsc.h>




namespace xtsc_vp {



template <int W, typename T>
class xtsc_pin2tlm_wire_transactor_vp : public xtsc::xtsc_pin2tlm_wire_transactor<W, T> {
public:
  xtsc_pin2tlm_wire_transactor_vp(sc_core::sc_module_name module_name) :
    xtsc::xtsc_pin2tlm_wire_transactor<W, T>(module_name) {}
};



class XTSC_VP_API xtsc_pin2tlm_wire_transactor_vp_RESET : public xtsc::xtsc_pin2tlm_wire_transactor<1, bool> {
public:
  xtsc_pin2tlm_wire_transactor_vp_RESET(sc_core::sc_module_name module_name) :
    xtsc::xtsc_pin2tlm_wire_transactor<1, bool>(module_name) {}
};



class XTSC_VP_API xtsc_pin2tlm_wire_transactor_vp_CLOCK : public xtsc::xtsc_pin2tlm_wire_transactor<1, bool> {
public:
  xtsc_pin2tlm_wire_transactor_vp_CLOCK(sc_core::sc_module_name module_name) :
    xtsc::xtsc_pin2tlm_wire_transactor<1, bool>(module_name) {}
};



}  // namespace xtsc_vp


#endif  // _XTSC_PIN2TLM_WIRE_TRANSACTOR_VP_H_
