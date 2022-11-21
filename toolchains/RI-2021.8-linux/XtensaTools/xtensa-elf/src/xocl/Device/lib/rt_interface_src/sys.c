/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED. 
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of 
 * Cadence Design Systems Inc. Any rights to use, modify, and create 
 * derivative works of this file are set forth under the terms of your 
 * license agreement with Cadence Design Systems, Inc.
 */

#include "sys.h"
#include "misc.h"

// Pointers to table of external callback functions for rtos utilities
static xdyn_lib_rtos_callback_if *rtos_cb_if;

// Enable preemption
uint32_t xocl_enable_preemption()
{
  return rtos_cb_if->rtos_enable_preemption();
}

// Disable preemption
uint32_t xocl_disable_preemption()
{
  return rtos_cb_if->rtos_disable_preemption();
}

// Initialize the rtos callback interface
void xocl_set_rtos_callback_if(xdyn_lib_rtos_callback_if *cb_if)
{
  XOCL_DPRINTF("xocl_set_rtos_callback_if...\n");
  rtos_cb_if = cb_if;
}
