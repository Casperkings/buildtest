/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED. 
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of 
 * Cadence Design Systems Inc. Any rights to use, modify, and create 
 * derivative works of this file are set forth under the terms of your 
 * license agreement with Cadence Design Systems, Inc.
 */

#ifndef __MISC_H__
#define __MISC_H__

// Misc. helper macros

#include <stdint.h>
#include <stdlib.h>
#include <xtensa/config/core-isa.h>
#include <xtensa/tie/xt_core.h>
#include "xdyn_lib_callback_if.h"

// From CL/cl.h 
// Return value to host for any errors encountered in device
#define XOCL_OUT_OF_RESOURCES (-5)

#define XOCL_LMEM_ATTR __attribute__((section(".dram0.data")))

#if XCHAL_HAVE_XEA5
// FIXME: hack to get it to compile on RNX, needs the proper code here
#define GET_PROC_ID 0
#elif XCHAL_HAVE_PRID
#define GET_PROC_ID XT_RSR_PRID()
#else
#define GET_PROC_ID 0 
#endif // XCHAL_HAVE_PRID

// External utilities
extern xdyn_lib_utils_callback_if *utils_cb_if;

__attribute__((unused)) static inline 
void xocl_zap_printf(const char *fmt, ...) {
  (void)fmt;
}

#if defined(XOCL_DEBUG)
#define XOCL_DPRINTF(format, args...) {                   \
  utils_cb_if->printf("XOCL_LOG: PROC%d: ", GET_PROC_ID); \
  utils_cb_if->printf(format, ## args);                   \
}
#define XOCL_ASSERT(C, ...)  { if (!(C)) { XOCL_DPRINTF(__VA_ARGS__); } }
#define XOCL_DEBUG_ASSERT(C) { if (!(C)) { utils_cb_if->abort(); } }
#else
#define XOCL_DPRINTF         xocl_zap_printf
#define XOCL_ASSERT(C, ...)  { if (!(C)) utils_cb_if->abort(); }
#define XOCL_DEBUG_ASSERT(C)
#endif // XOCL_DEBUG

#if defined(XOCL_TIME_KERNEL) || defined(XOCL_TIME_KERNEL_ONLY) || \
    defined(XOCL_TIME_WG_LOOP)                                     
#define XOCL_PPRINTF(format, args...) {                            \
  utils_cb_if->printf("XOCL_PROFILE: PROC%d: ", GET_PROC_ID);      \
  utils_cb_if->printf(format, ## args);                            \
}
#else
#define XOCL_PPRINTF xocl_zap_printf
#endif // XOCL_TIME_KERNEL || XOCL_TIME_KERNEL_ONLY || XOCL_TIME_WG_LOOP

#if defined(XOCL_TRACE_EXEC)                         
#define XOCL_TPRINTF(format, args...) {                     \
  utils_cb_if->printf("XOCL_TRACE: PROC%d: ", GET_PROC_ID); \
  utils_cb_if->printf(format, ## args);                     \
}
#else
#define XOCL_TPRINTF xocl_zap_printf
#endif // XOCL_TRACE_KERNEL_EXEC 

#define XOCL_TRUE           1
#define XOCL_FALSE          0
#define XOCL_INVALID_VALUE -1

#define XOCL_SWAP(x, y, Type) \
  do { Type t = (x); (x) = (y); (y) = t; } while(0)

#define XOCL_MIN(a, b) (((a) < (b)) ? (a) : (b))
#define XOCL_MAX(a, b) (((a) > (b)) ? (a) : (b))

#if defined(XOCL_TIME_KERNEL) || defined(XOCL_TIME_KERNEL_ONLY) || \
    defined(XOCL_TIME_WG_LOOP)
static inline uint32_t _xocl_time_stamp() {
  uint32_t cycle;
  __asm__ __volatile__ ("rsr.ccount %0"
                        : "=a" (cycle) :: "memory"
                       );
  return cycle;
}
#else
static inline uint32_t _xocl_time_stamp() { return 0; }
#endif // XOCL_TIME_KERNEL  || XOCL_TIME_KERNEL_ONLY || XOCL_TIME_WG_LOOP

#endif // __MISC_H__
