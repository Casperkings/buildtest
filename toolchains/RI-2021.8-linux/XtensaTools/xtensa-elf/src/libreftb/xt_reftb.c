/* 
 * Copyright (c) 2009-2011 by Tensilica Inc.  ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of
 * Tensilica Inc.  They may be adapted and modified by bona fide
 * purchasers for internal use, but neither the original nor any
 * adapted or modified version may be disclosed or distributed to
 * third parties in any manner, medium, or form, in whole or in part,
 * without the prior written consent of Tensilica Inc.
 *
 * This software and its derivatives are to be executed solely on
 * products incorporating a Tensilica processor.
 */

// Utility routines for running on reference testbench

#ifdef __XTENSA__

#include <xtensa/hal.h>
#include <xtensa/config/core.h>
#include <xtensa/config/system.h>
#include <xtensa/tie/xt_core.h>
#include <xtensa/xt_reftb.h>
#include <math.h>

#if XCHAL_HAVE_HALT
#include <xtensa/tie/xt_halt.h>
#endif

#endif // __XTENSA__

extern volatile unsigned int *_reftb_exit_location;
extern volatile unsigned int *_reftb_power_toggle;
extern volatile unsigned int *_reftb_int_deassert;
extern volatile unsigned int *_reftb_xtmem_scratch;
extern volatile unsigned int *_reftb_stl_info_location;

// Change exit status location
// You must also change the plusarg "+DVMagicExit" sent to the HW simulator
// or change the argument "--exit_location" sent to the ISS
//
unsigned int* 
set_testbench_exit_location(unsigned int* ex_loc)
{
  _reftb_exit_location = ex_loc;
  return (ex_loc);
}

// Change power toggle location
// You must also change the plusarg "+DVPowerLoc" sent to the HW simulator
//
unsigned int* 
set_testbench_power_toggle_location(unsigned int* ex_loc)
{
  _reftb_power_toggle = ex_loc;
  setup_power_toggle();
  return (ex_loc);
}

unsigned int* 
set_stl_info_location(unsigned int* stlInfo_loc)
{
      _reftb_stl_info_location = stlInfo_loc;
        return (stlInfo_loc);
}


// Return exit status location
unsigned int* 
testbench_exit_location()
{
  return (unsigned int*) _reftb_exit_location;
}
// Return power toggle location
unsigned int* 
testbench_power_toggle_location()
{
  return (unsigned int*) _reftb_power_toggle;
}

// Return STL info location
unsigned int* 
testbench_stl_info_location()
{
  return (unsigned int*) _reftb_stl_info_location;
}

// Setup for user power toggling
int setup_power_toggle() 
{
#ifdef __XTENSA__
#if XCHAL_HAVE_PIF
  xthal_set_region_attribute((void *)_reftb_power_toggle, 4, XCHAL_CA_BYPASS, 0);
#endif
#endif
  return 1;
}
#if XCHAL_HAVE_PSO && XCHAL_HAVE_PSO_CDM && XCHAL_HAVE_PSO_FULL_RETENTION
// Change wakeup int deassert location
// You must also change the plusarg "+DVWakeupIntClrLoc" sent to the HW simulator
//
unsigned int* 
set_testbench_int_deassert_location(unsigned int* ex_loc)
{
  _reftb_int_deassert = ex_loc;
  setup_int_deassert();
  return (ex_loc);
}

// Return wakeup int deassert location
unsigned int* 
testbench_int_deassert_location()
{
  return (unsigned int*) _reftb_int_deassert;
}

int setup_int_deassert()
{
#ifdef __XTENSA__
#if XCHAL_HAVE_PIF
  xthal_set_region_attribute((void *)_reftb_int_deassert, 4, XCHAL_CA_BYPASS, 0);
#endif
#endif
}
#endif

#if XCHAL_HAVE_PSO && ! XCHAL_HAVE_PSO_CDM
// Change wakeup int deassert location
// You must also change the plusarg "+DVWakeupIntClrLoc" sent to the HW simulator
//
unsigned int* 
set_testbench_xtmem_scratch_location(unsigned int* ex_loc)
{
  _reftb_xtmem_scratch = ex_loc;
  setup_xtmem_scratch();
  return (ex_loc);
}

// Return wakeup int deassert location
unsigned int* 
testbench_xtmem_scratch_location()
{
  return (unsigned int*) _reftb_xtmem_scratch;
}

int setup_xtmem_scratch()
{
#ifdef __XTENSA__
#if XCHAL_HAVE_PIF
  xthal_set_region_attribute((void *)_reftb_xtmem_scratch, 4, XCHAL_CA_BYPASS, 0);
#endif
#endif
}
#endif


// Diags expect status 1 or 2
#define DIAG_PASS_STATUS 1
#define DIAG_FAIL_STATUS 2
#define MAGIC_EXIT_FLAG 13

//
// Set exit status for HW simulation
// Monitors.v will detect the halt and exit the simulation. 
//
int set_diag_status(int stat) 
{

#ifdef __XTENSA__

  unsigned int prid_shift = 0;
#if XCHAL_NUM_CORES_IN_CLUSTER > 1
  unsigned int mask_shift = (32 - XCHAL_L2CC_NUM_CORES_LOG2);
  unsigned int mask_prid  = (0xffffffff >> mask_shift);
#endif


#if XCHAL_HAVE_PRID
#if XCHAL_HAVE_NX
#if XCHAL_NUM_CORES_IN_CLUSTER > 1
  prid_shift = (XT_RSR_PRID() & mask_prid) << 4;;
#else
  prid_shift = (XT_RSR_PRID() & 0xF) << 4;
#endif
#else
  prid_shift = 0;
#endif
#endif

#if XCHAL_HAVE_PIF
  xthal_set_region_attribute((void *)_reftb_exit_location, 4, XCHAL_CA_BYPASS, 0);
#endif

#if XCHAL_HAVE_HALT
// 1) Write status (PASS or FAIL) to magic address      
// 2) Do a memw to make sure write has completed
// 3) Issue halt instruction
//
  *_reftb_exit_location = stat | prid_shift;
  XT_HALT();

#else  // XCHAL_HAVE_HALT

// 1) Write MAGIC_EXIT_FLAG to magic address
// 2) Write status (PASS or FAIL) to magic address      
//
  *_reftb_exit_location = MAGIC_EXIT_FLAG | prid_shift;
  *_reftb_exit_location = stat | prid_shift;

#endif  // XCHAL_HAVE_HALT

#if !XCHAL_HAVE_RNX
    __asm__ ("movi    a2, 1\n"
             "simcall\n");
#endif

//add waiti after status update so that the core which has finished stays here
//otherwise the core which finishes first can reach the last instruction and cause illegal instruction exceptions before the simulation finishes
#if XCHAL_HAVE_NX
    XT_WAITI(0);
#endif

#endif // __XTENSA__

  return stat;
}

//
// Exit routines
// Set status then exit
// 

int diag_pass()
{
  return set_diag_status(DIAG_PASS_STATUS);
}

int diag_fail()
{
  return set_diag_status(DIAG_FAIL_STATUS);
}

//
// Set STL status for HW simulation
// Monitors.v will detect fault simulation status. 
//

int set_stl_status(int stat) 
{

#ifdef __XTENSA__
//  Write status (STL_INFO) to magic address      
 *_reftb_stl_info_location = stat;  
#endif // __XTENSA__
  return 0;
}

//
// STL routines
// Set STL status 
// 

int stl_start(int stat)
{
  return set_stl_status(stat);
}


//
// Set exit status for HW simulation
// Monitors.v will detect the halt and exit the simulation. 
//
//  This "fast" version does not call XTOS before setting status.
//  It requires that the user has assured the _reftb_exit_location
//  is in bypass memory prior to calling set_diag_status_fast().
int set_diag_status_fast(int stat) 
{

#ifdef __XTENSA__

  unsigned int prid_shift = 0;

#if XCHAL_HAVE_PRID
#if XCHAL_HAVE_NX
  prid_shift = (XT_RSR_PRID() & 0xF) << 4;
#else
  prid_shift = 0;
#endif
#endif

#if XCHAL_HAVE_HALT
// 1) Write status (PASS or FAIL) to magic address      
// 2) Do a memw to make sure write has completed
// 3) Issue halt instruction
//
  *_reftb_exit_location = stat | prid_shift;  
  XT_HALT();

#else  // XCHAL_HAVE_HALT

// 1) Write MAGIC_EXIT_FLAG to magic address
// 2) Write status (PASS or FAIL) to magic address      
//
  *_reftb_exit_location = MAGIC_EXIT_FLAG | prid_shift;
  *_reftb_exit_location = stat | prid_shift;

#endif  // XCHAL_HAVE_HALT
#endif // __XTENSA__

  return stat;
}

// Stubbed out outbyte and inbyte
void outbyte( int c )
{
    UNUSED(c);
}

int inbyte( void )
{
    return -1;          // always EOF
}

// Standard exit routine that forwards status to diag status 
void _exit( int status )
{
    // diag status is 0 for fail, non zero for pass
    set_diag_status(status == 0);
    while(1);
}

