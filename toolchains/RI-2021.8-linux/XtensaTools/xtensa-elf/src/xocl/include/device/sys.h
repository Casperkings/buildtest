/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED. 
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of 
 * Cadence Design Systems Inc. Any rights to use, modify, and create 
 * derivative works of this file are set forth under the terms of your 
 * license agreement with Cadence Design Systems, Inc.
 */

#ifndef __SYS_H__
#define __SYS_H__

// System routines

#include <stdint.h>
#include "xdyn_lib_callback_if.h"

// Enable preemption
extern uint32_t xocl_enable_preemption();

// Disable preemption
extern uint32_t xocl_disable_preemption(); 

// Initialize the rtos callback interface
extern void xocl_set_rtos_callback_if(xdyn_lib_rtos_callback_if *cb_if);

#endif // __SYS_H__
