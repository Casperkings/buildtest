/** @file */

// xos_params.h - user-settable compile time parameters for XOS.

// Copyright (c) 2015-2020 Cadence Design Systems, Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


#ifndef XOS_PARAMS_H
#define XOS_PARAMS_H

#include <xtensa/config/core.h>

#include "xos_types.h"

#ifdef __cplusplus
extern "C" {
#endif


//-----------------------------------------------------------------------------
///
/// Number of thread priority levels. At this time XOS supports a maximum of
/// 32 priority levels (0 - 31).
///
//-----------------------------------------------------------------------------
#ifndef XOS_NUM_PRIORITY
#define XOS_NUM_PRIORITY                16       // Default is 16
#endif


//-----------------------------------------------------------------------------
///
/// Debug flags:
///
/// - XOS_DEBUG_ALL       -- Enable debug mode
/// - XOS_DEBUG_VERBOSE   -- Enable verbose debug prints. This is NOT enabled
///                          by XOS_DEBUG_ALL. Turning this on generates lots
///                          of debug output and also drastically changes the
///                          system's timing.
///
/// NOTE: Enabling one or more of these flags will affect system performance
/// and timing.
///
//-----------------------------------------------------------------------------
#ifndef XOS_DEBUG_VERBOSE
#define XOS_DEBUG_VERBOSE               UINT32_C(0)
#endif

#ifndef XOS_DEBUG_ALL
#define XOS_DEBUG_ALL                   UINT32_C(0)
#endif

#if XOS_DEBUG_ALL

#define XOS_DEBUG                       UINT32_C(1)

#else

#ifndef XOS_DEBUG
#define XOS_DEBUG                       UINT32_C(0)
#endif

#endif


//-----------------------------------------------------------------------------
///
/// Set this option to 1 to enable runtime statistics collection for XOS.
/// NOTE: Enabling this option does have some impact on runtime performance
/// and OS footprint.
///
//-----------------------------------------------------------------------------
#ifndef XOS_OPT_STATS
#define XOS_OPT_STATS                   UINT32_C(1)
#endif


//-----------------------------------------------------------------------------
///
/// Size of interrupt stack in bytes. Shared by all interrupt handlers. Must be
/// sized to handle worst case nested interrupts. This is also used by the idle
/// thread so must exist even if interrupts are not configured.
///
//-----------------------------------------------------------------------------
#ifndef XOS_INT_STACK_SIZE
#if XCHAL_HAVE_INTERRUPTS
#define XOS_INT_STACK_SIZE              UINT32_C(8192)
#else
#define XOS_INT_STACK_SIZE              UINT32_C(32)
#endif
#endif


//-----------------------------------------------------------------------------
///
/// Default maximum interrupt level at which XOS primitives may be called.
/// It is the level at which interrupts are disabled by default.
/// See also description of xos_set_int_pri_level().
///
//-----------------------------------------------------------------------------
#ifndef XOS_MAX_OS_INTLEVEL
#if defined XCHAL_EXCM_LEVEL
#define XOS_MAX_OS_INTLEVEL             XCHAL_EXCM_LEVEL
#else
#define XOS_MAX_OS_INTLEVEL             XCHAL_NUM_INTLEVELS
#endif
#endif


//-----------------------------------------------------------------------------
///
/// Set this to 1 to enable stack checking. The stack is filled with a pattern
/// on thread creation, and the stack is checked at certain times during system
/// operation.
///
//-----------------------------------------------------------------------------
#ifndef XOS_OPT_STACK_CHECK
#if XOS_DEBUG_ALL
#define XOS_OPT_STACK_CHECK             UINT32_C(1)
#else
#define XOS_OPT_STACK_CHECK             UINT32_C(0)
#endif
#endif


//-----------------------------------------------------------------------------
///
/// Set XOS_CLOCK_FREQ to the system clock frequency if this is known ahead of
/// time. Otherwise, call xos_set_clock_freq() to set it at run time.
///
//-----------------------------------------------------------------------------
#ifndef XOS_CLOCK_FREQ
#define XOS_CLOCK_FREQ                  UINT32_C(1000000)
#endif
#define XOS_DEFAULT_CLOCK_FREQ          XOS_CLOCK_FREQ


//-----------------------------------------------------------------------------
///
/// Set this option to 1 to enable software prioritization of interrupts. The
/// priority scheme applied is that a higher interrupt number at the same level
/// will have higher priority.
///
//-----------------------------------------------------------------------------
#ifndef XOS_OPT_INTERRUPT_SWPRI
#define XOS_OPT_INTERRUPT_SWPRI         UINT32_C(1)
#endif


//-----------------------------------------------------------------------------
///
/// Set this option to 1 to use the thread-safe version of the C runtime library.
/// You may need to enable this if you call C library functions from multiple
/// threads -- see the documentation for the relevant C library to determine if
/// this is necessary. This option increases the size of the TCB.
/// NOTE: At this time only the newlib and xclib libraries are supported for
/// thread safety.
///
//-----------------------------------------------------------------------------
#include <xtensa/config/system.h>

#ifndef XOS_OPT_THREAD_SAFE_CLIB

#if XSHAL_CLIB == XTHAL_CLIB_XCLIB
#define XOS_OPT_THREAD_SAFE_CLIB        UINT32_C(1)
#elif XSHAL_CLIB == XTHAL_CLIB_NEWLIB
#define XOS_OPT_THREAD_SAFE_CLIB        UINT32_C(1)
#else
#define XOS_OPT_THREAD_SAFE_CLIB        UINT32_C(0)
#endif

#endif


//-----------------------------------------------------------------------------
///
/// Set this option to 1 to enable the wait timeout feature. This allows waits
/// on waitable objects to expire after a specified timeout.
///
//-----------------------------------------------------------------------------
#ifndef XOS_OPT_WAIT_TIMEOUT
#define XOS_OPT_WAIT_TIMEOUT            UINT32_C(1)
#endif


//-----------------------------------------------------------------------------
///
/// Set this option to 1 to enable threads waiting on timer objects. If this
/// feature is not used, turning it off will make timer objects smaller, and
/// reduce the time taken by timer expiry processing (by a small amount).
///
//-----------------------------------------------------------------------------
#ifndef XOS_OPT_TIMER_WAIT
#define XOS_OPT_TIMER_WAIT              UINT32_C(1)
#endif


//-----------------------------------------------------------------------------
///
/// Set this option to 1 to enable timer event processing in a separate thread
/// instead of in the timer interrupt handler. Processing in a thread allows
/// the interrupt handler to finish faster and allows potentially blocking OS
/// calls, but does require more memory for the timer thread TCB and stack.
///
//-----------------------------------------------------------------------------
#ifndef XOS_OPT_TIMER_THREAD
#define XOS_OPT_TIMER_THREAD            UINT32_C(0)
#endif


//-----------------------------------------------------------------------------
///
/// Use this option to specify the stack size in bytes for the timer thread.
/// Make sure the size is large enough to handle user-supplied timer callbacks.
/// This value is ignored if XOS_OPT_TIMER_THREAD is zero.
///
//-----------------------------------------------------------------------------
#ifndef XOS_TIMER_THREAD_STACK_SIZE
#define XOS_TIMER_THREAD_STACK_SIZE     (XOS_STACK_MIN_SIZE + 0x100)
#endif


//-----------------------------------------------------------------------------
///
/// Set this option to 1 to enable time-slicing between multiple threads at
/// the same priority. If this option is enabled then after every slice period
/// the timer handler will switch out the current thread if there is another
/// ready thread at the same priority, and allow the latter thread to run.
/// Execution will be round robin switched among all threads at the same
/// priority.
///
/// The time slice interval is a multiple of the timer tick period, and is
/// determined by the value of XOS_TIME_SLICE_TICKS.
///
/// This feature is only useful if fixed duration timer ticks are used.
/// If dynamic ticks are used then time slicing will work unpredictably
/// because the interval between ticks will vary. To avoid this, time-slicing
/// is disabled if dynamic ticks are used, and this parameter has no effect.
///
//-----------------------------------------------------------------------------
#ifndef XOS_OPT_TIME_SLICE
#define XOS_OPT_TIME_SLICE              UINT32_C(1)
#endif


//-----------------------------------------------------------------------------
///
/// Number of timer ticks per time slice interval. This setting is used only if
/// the option XOS_OPT_TIME_SLICE is enabled. Setting this to a higher number
/// will increase the time slice interval. The valid range for this setting is
/// 1 to 50. Specifying a number outside the range will cause it to be capped
/// within the range limit.
///
//-----------------------------------------------------------------------------
#ifndef XOS_TIME_SLICE_TICKS
#define XOS_TIME_SLICE_TICKS            UINT32_C(1)
#endif


//-----------------------------------------------------------------------------
///
/// Set this option to 1 to enable priority inversion protection for mutexes.
/// This option must be enabled to use mutex priority inheritance and priority
/// protection. 
///
/// Enabling this option will add to both code and data size and runtime.
///
//-----------------------------------------------------------------------------
#ifndef XOS_OPT_MUTEX_PRIORITY
#define XOS_OPT_MUTEX_PRIORITY          UINT32_C(1)
#endif


//-----------------------------------------------------------------------------
///
/// Setting this option to 1 causes all interrupts to go to a wrapper function
/// which then dispatches them to the appropriate handlers.
/// NOTE: This is an experimental feature.
///
//-----------------------------------------------------------------------------
#define XOS_USE_INT_WRAPPER             UINT32_C(0)


//-----------------------------------------------------------------------------
///
/// Setting this option to 1 enables system event logging. The event log must
/// be configured and enabled for event logging to work. If the event log is
/// not enabled, this option will do nothing.
///
//-----------------------------------------------------------------------------
#ifndef XOS_OPT_LOG_SYSEVENT
#define XOS_OPT_LOG_SYSEVENT            UINT32_C(0)
#endif


//-----------------------------------------------------------------------------
///
/// Setting this option to 1 enables the use of C11 atomics. Enabling this will
/// require compiling to the C11 standard (-std=c11 compiler option). Note that
/// C11 atomics are supported only by the Xtensa C Library (xclib).
///
//-----------------------------------------------------------------------------
#ifndef XOS_OPT_C11_ATOMICS
#define XOS_OPT_C11_ATOMICS             UINT32_C(0)
#endif


//-----------------------------------------------------------------------------
///
/// Setting this option to 1 enables thread local storage similar to pthreads
/// thread-specific keys. If this option is enabled, the number of keys is
/// specified by XOS_TLS_NUM_KEYS. The thread-specific values are stored in
/// the TCB, so specifying a larger value for XOS_TLS_NUM_KEYS could increase
/// the size of the TCB by a considerable amount.
///
//-----------------------------------------------------------------------------
#ifndef XOS_OPT_THREAD_LOCAL_STORAGE
#define XOS_OPT_THREAD_LOCAL_STORAGE    UINT32_C(1)
#endif

#ifndef XOS_TLS_NUM_KEYS
#define XOS_TLS_NUM_KEYS                UINT32_C(4)
#endif


//-----------------------------------------------------------------------------
///
/// Setting this option to 1 enables (LX) Secure Mode support. If this option
/// is enabled XOS will request interrupt and exception handler registration
/// from the Xtensa Secure Monitor. See XOS and Secure Monitor documentation
/// for details.
/// NOTE: This option requires LX Secure Mode and the MISC1 special register.
///
//-----------------------------------------------------------------------------
#ifndef XOS_OPT_SECMON
#define XOS_OPT_SECMON                  UINT32_C(0)
#endif


//-----------------------------------------------------------------------------
///
/// Setting this option to 1 enables user-mode support. This allows you to run
/// specified threads at user privilege, and assign and control memory access
/// for these threads. This option is off by default.
///
//-----------------------------------------------------------------------------
#ifndef XOS_OPT_UM
#define XOS_OPT_UM                      UINT32_C(0)
#endif


//-----------------------------------------------------------------------------
///
/// If user-mode support is enabled, this option lets you specify the size of
/// the (optional) heap for libc. This is not required for XOS itself but some
/// libc functions such as file I/O do request allocations from the heap, and
/// will fail if there is no heap. Set this to zero if you do not need the heap.
/// The default size is 16KB. If user-mode thread support is not enabled then
/// this parameter will be ignored.
///
//-----------------------------------------------------------------------------
#if XOS_OPT_UM
#ifndef XOS_UM_HEAP_SIZE
#define XOS_UM_HEAP_SIZE                UINT32_C(16*1024)
#endif
#endif


//-----------------------------------------------------------------------------
// RESERVED: do not modify this setting.
//-----------------------------------------------------------------------------
#if XOS_OPT_UM
#ifndef XOS_UM_NUM_MPU
#define XOS_UM_NUM_MPU                  UINT32_C(2)
#endif
#endif


//-----------------------------------------------------------------------------
///
/// Specify the maximum length (in characters) of user-mode object names.
/// Note that only single-byte characters are supported. the default is 16.
///
//-----------------------------------------------------------------------------
#if XOS_OPT_UM
#ifndef XOS_UM_OBJ_NAME_LEN
#define XOS_UM_OBJ_NAME_LEN             UINT32_C(16)
#endif
#endif


//-----------------------------------------------------------------------------
///
/// Specify the number of user-mode threads to allocate space for. The thread
/// control blocks are statically allocated. This sets the upper bound on the
/// number of user-mode threads that can be created at one time. The default
/// is 16. If user-mode support is not enabled then this parameter will be
/// ignored.
///
//-----------------------------------------------------------------------------
#if XOS_OPT_UM
#ifndef XOS_UM_NUM_THREAD
#define XOS_UM_NUM_THREAD               UINT32_C(16)
#endif
#endif


//-----------------------------------------------------------------------------
///
/// Specify the number of user-mode semaphores to allocate. The semaphores are
/// statically allocated. This sets the upper bound on the number of user-mode
/// semaphores that can be created at one time. The default number is 8.
/// If user-mode support is not enabled then this parameter will be ignored.
///
//-----------------------------------------------------------------------------
#if XOS_OPT_UM
#ifndef XOS_UM_NUM_SEM
#define XOS_UM_NUM_SEM                  UINT32_C(8)
#endif
#endif


//-----------------------------------------------------------------------------
///
/// Specify the number of user-mode mutexes to allocate. The mutexes are
/// statically allocated. This sets the upper bound on the number of user-mode
/// mutexes that can be created at one time. The default number is 8.
/// If user-mode support is not enabled then this parameter will be ignored.
///
//-----------------------------------------------------------------------------
#if XOS_OPT_UM
#ifndef XOS_UM_NUM_MTX
#define XOS_UM_NUM_MTX                  UINT32_C(8)
#endif
#endif


//-----------------------------------------------------------------------------
///
/// Specify the number of user-mode conditions to allocate. The conditions are
/// statically allocated. This sets the upper bound on the number of user-mode
/// conditions that can be created at one time. The default number is 8.
/// If user-mode support is not enabled then this parameter will be ignored.
///
//-----------------------------------------------------------------------------
#if XOS_OPT_UM
#ifndef XOS_UM_NUM_COND
#define XOS_UM_NUM_COND                 UINT32_C(8)
#endif
#endif


//-----------------------------------------------------------------------------
///
/// Specify the maximum number of blocks that can be allocated at one time from
/// the user-mode private memory pool. This number determines the size of the
/// pool header (which resides in privileged memory). The default is 128.
///
//-----------------------------------------------------------------------------
#if XOS_OPT_UM
#ifndef XOS_UM_NUM_PBLKS
#define XOS_UM_NUM_PBLKS                128
#endif
#endif


#ifdef __cplusplus
}
#endif

#endif  // XOS_PARAMS_H

