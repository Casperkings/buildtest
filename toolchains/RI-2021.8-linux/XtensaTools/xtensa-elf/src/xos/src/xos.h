/** @file */

// xos.h - XOS API interface and data structures visible to user code.

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


#ifndef XOS_H
#define XOS_H

#ifdef __cplusplus
extern "C" {
#endif


#include "xos_types.h"

#include <xtensa/hal.h>
#include <xtensa/config/core.h>
#include <xtensa/core-macros.h>
#include <xtensa/tie/xt_core.h>
#if XCHAL_HAVE_INTERRUPTS
#include <xtensa/tie/xt_interrupt.h>
#endif

#include "xos_common.h"
#include "xos_errors.h"
#include "xos_regaccess.h"


//-----------------------------------------------------------------------------
// XOS version.
//-----------------------------------------------------------------------------
#define XOS_VERSION_MAJOR       3
#define XOS_VERSION_MINOR       0
#define XOS_VERSION_PATCH       0
#define XOS_VERSION_STRING      "3.00"      ///< XOS version string.


//-----------------------------------------------------------------------------
// Runtime error handling.
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
///
///  Reports a fatal error and halts XOS operation, i.e. halts the system. This
///  function will call a user-registered error handler (if one has been set) 
///  and then halt the system. The user handler may do system-specific things
///  such as record the error reason in nonvolatile memory etc.
///
///  \param     errcode         Error code. May be any user defined value.
///                             Values -65536 through -1 are used by XOS.
///
///  \param     errmsg          Optional text string describing the error.
///
///  \return    This function does not return.
///
//-----------------------------------------------------------------------------
void
xos_fatal_error(int32_t errcode, const char * errmsg);


#if XOS_DEBUG

//-----------------------------------------------------------------------------
///
///  Flag controlling debug output. Set to zero to disable XOS debug output.
///
//-----------------------------------------------------------------------------
extern bool xos_debug_print;

// Do not call directly.
void
xos_assert(const char * file, int32_t line);

//-----------------------------------------------------------------------------
///
///  Check condition and fail if condition expression is false.
///  In debug builds, an assertion failure will cause a fatal error to be
///  reported. In non-debug builds, assertions are compiled out.
///
///  NOTE: Remember that any code in XOS_ASSERT() statements gets compiled out
///  for non-debug builds.
///
//-----------------------------------------------------------------------------
__attribute__((always_inline))
static inline void
XOS_ASSERT(bool expr)
{
    if ((expr) == false) {
        xos_assert(__FILE__, __LINE__);
    }
}

#else

// parasoft-begin-suppress MISRAC2012-DIR_4_9-a-4 "Function-like macro for debug efficiency"
#define XOS_ASSERT(expr)
// parasoft-end-suppress MISRAC2012-DIR_4_9-a-4

#endif


//-----------------------------------------------------------------------------
///
/// Interrupt handler function pointer type.
///
//-----------------------------------------------------------------------------
typedef void    (XosIntFunc)(void * arg);

//-----------------------------------------------------------------------------
///
/// Print handler function pointer type.
///
//-----------------------------------------------------------------------------
typedef int32_t (XosPrintFunc)(void * arg, const char * fmt, ...);

//-----------------------------------------------------------------------------
///
/// Fatal error handler function pointer type.
///
//-----------------------------------------------------------------------------
typedef void    (XosFatalErrFunc)(int32_t errcode, const char * errmsg);

//-----------------------------------------------------------------------------
///
/// Exception handler function pointer type.
///
//-----------------------------------------------------------------------------
/*  This header gets included by packages supporting other exception arches:  */
#if !XCHAL_HAVE_XEA2 && !XCHAL_HAVE_XEA3
#define XosExcFrame int
#endif
typedef void    (XosExcHandlerFunc)(XosExcFrame * frame);


//-----------------------------------------------------------------------------
///
///  Install a user defined exception handler for the specified exception type.
///  This will override the default XOS exception handler. The handler is a C
///  function that is passed one parameter -- a pointer to the exception frame.
///  The exception frame is allocated on the stack of the thread that caused the
///  exception, and contains saved state and exception information. For details
///  of the exception frame see the structure XosExcFrame.
///
///  \param     exc             Exception type (number) to override. The exception
///                             numbers are enumerated in <xtensa/corebits.h>.
///
///  \param     handler         Pointer to handler function to be installed.
///                             To revert to the default handler, pass NULL.
///
///  \return    Returns a pointer to previous handler installed, if any.
///
//-----------------------------------------------------------------------------
XosExcHandlerFunc *
xos_register_exception_handler(uint32_t exc, XosExcHandlerFunc * handler);


//-----------------------------------------------------------------------------
///
///  Install a user defined fatal error handler. This function will be called if
///  a fatal error is reported either by user code or by XOS itself. It will be
///  passed the same arguments that are passed to xos_fatal_error().
///
///  The handler need not return. It should make minimal assumptions about the
///  state of the system. In particular, it should not assume that further XOS
///  system calls will succeed.
///
///  \param     handler         Pointer to handler function to be installed.
///
///  \return    Returns a pointer to previous handler installed, if any.
///
//-----------------------------------------------------------------------------
XosFatalErrFunc *
xos_register_fatal_error_handler(XosFatalErrFunc * handler); // parasoft-suppress MISRAC2012-RULE_1_1-b-2 "Identifier length retained for API compatibility"


#ifdef XOS_INCLUDE_INTERNAL
# include "xos_internal.h"
#endif


#include "xos_thread.h"
#include "xos_cond.h"
#include "xos_event.h"
#include "xos_mutex.h"
#include "xos_msgq.h"
#include "xos_semaphore.h"
#include "xos_blockmem.h"
#include "xos_barrier.h"


//-----------------------------------------------------------------------------
///
///  Register a handler function to call when interrupt "num" occurs.
///
///  For level-triggered and timer interrupts, the handler function will have
///  to clear the source of the interrupt before returning, to avoid infinitely
///  retaking the interrupt. Edge-triggered and software interrupts are 
///  automatically cleared by the OS interrupt dispatcher (see xos_handlers.S).
///
///  \param     num             Xtensa interrupt number (0..XCHAL_NUM_INTERRUPTS-1).
///                             Use HAL macro XCHAL_EXTINTx_NUM to refer to a
///                             specific external interrupt number (BInterrupt pin),
///                             where 'x' is the external number.
///
///  \param     handler         Pointer to handler function.
///
///  \param     arg             Argument passed to handler.
///
///  \return    Returns XOS_OK if successful, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_register_interrupt_handler(uint32_t num, XosIntFunc * handler, void * arg);


//-----------------------------------------------------------------------------
///
///  Unregister a handler function for interrupt "num". If no handler was 
///  installed, this function will have no effect.
///
///  \param     num             Xtensa interrupt number (0..XCHAL_NUM_INTERRUPTS-1).
///                             Use HAL macro XCHAL_EXTINTx_NUM to refer to a
///                             specific external interrupt number (BInterrupt pin),
///                             where 'x' is the external number.
///
///  \return    Returns XOS_OK if successful, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_unregister_interrupt_handler(uint32_t num); // parasoft-suppress MISRAC2012-RULE_1_1-b-2 "Identifier length retained for API compatibility"


//-----------------------------------------------------------------------------
///
///  Register a high priority interrupt handler for interrupt level "level".
///  This function is available only for configurations using XEA2.
///
///  Unlike low and medium priority interrupt handlers, high priority handlers
///  are not installed for a specific interrupt number, but for an interrupt
///  level. The level must be above XCHAL_EXCM_LEVEL. The handler function must
///  be written in assembly since C handlers are not supported for levels above
///  XCHAL_EXCM_LEVEL. The handler function must preserve all registers except
///  a0, and must return to the dispatcher via a "ret" instruction, not "rfi".
///
///  NOTE: This method of dispatch takes a few cycles of overhead. If you wish
///  to save even these cycles, then you can define your own dispatch function
///  to override the built-in dispatcher. See xos_handlers.S for more details.
///
///  \param     level           The interrupt level to be handled.
///
///  \param     handler         Pointer to handler function.
///
///  \return    Returns XOS_OK if successful, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_register_hp_interrupt_handler(uint32_t level, void * handler); // parasoft-suppress MISRAC2012-RULE_1_1-b-2 "Identifier length retained for API compatibility"


//-----------------------------------------------------------------------------
///
///  Enable a specific interrupt, by interrupt number.
///  The state (enabled vs. disabled) of individual interrupts is global, i.e.
///  not associated with any specific thread. Depending on system options and
///  implementation, this state may not be exactly reflected in the hardware
///  at all times, e.g. when software prioritization is in effect for finer
///  grained prioritization within an interrupt level. Thus it is best not to
///  access the interrupt control/status hardware directly.
///
///  To modify thread-specific interrupt priority level, use one of:
///    - xos_set_int_pri_level()
///    - xos_restore_int_pri_level()
///    - xos_disable_interrupts()
///    - xos_restore_interrupts()
///
///  NOTE: Use the HAL macro XCHAL_EXTINTx_NUM to refer to a specific external
///  interrupt number (BInterrupt pin), where 'x' is the external interrupt
///  number. For example, to enable external interrupt 3 (BInterrupt[3]),
///  you can use:
///
///      xos_interrupt_enable( XCHAL_EXTINT3_NUM );
///
///  \param     intnum          Interrupt number to enable. Must range between
///                             0 and XCHAL_NUM_INTERRUPTS - 1.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_interrupt_enable(uint32_t intnum);


//-----------------------------------------------------------------------------
///
///  Check whether the specified interrupt is enabled. See the description of
///  xos_interrupt_enable() for further comments and notes.
///
///  \param     intnum          Interrupt number to query. Must range between
///                             0 and XCHAL_NUM_INTERRUPTS - 1.
///
///  \return    Returns true if the interrupt is enabled, else returns false.
///
//-----------------------------------------------------------------------------
bool
xos_interrupt_enabled(uint32_t intnum);


//-----------------------------------------------------------------------------
///
///  Disable a specific individual interrupt, by interrupt number.
///
///  This is the counterpart to xos_interrupt_enable(). See the description
///  of xos_interrupt_enable() for further comments and notes.
///
///  \param     intnum          Interrupt number to disable. Must range between
///                             0 and XCHAL_NUM_INTERRUPTS - 1.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_interrupt_disable(uint32_t intnum);


//-----------------------------------------------------------------------------
///
///  Check whether the specified interrupt is pending. See the description of
///  xos_interrupt_enable() for further comments and notes.
///
///  \param     intnum          Interrupt number to check. Must range between
///                             0 and XCHAL_NUM_INTERRUPTS - 1.
///
///  \return    Returns true if the interrupt is pending, else returns false.
///
//-----------------------------------------------------------------------------
__attribute__((always_inline))
static inline bool
xos_interrupt_pending(uint32_t intnum)
{
    return (xthal_interrupt_pending(intnum) != 0U);
}


//-----------------------------------------------------------------------------
///
///  Get the CPU's current interrupt priority level. Interrupts at or below this
///  priority level are blocked.
///
///  \return    Returns the current IPL, ranging from 0 to XCHAL_NUM_INTLEVELS.
///
//-----------------------------------------------------------------------------
__attribute__((always_inline))
static inline uint32_t
xos_get_int_pri_level(void)
{
    return xthal_intlevel_get();
}


//-----------------------------------------------------------------------------
///
///  Set the CPU's interrupt priority level to the specified level, but only if
///  the current IPL is below the one requested. This function will never cause
///  the interrupt priority level to be lowered from the current level.
///  Call this function to block interrupts at or below the specified priority
///  level.
///
///  When setting the IPL temporarily (such as in a critical section), call
///  xos_set_int_pri_level(), execute the critical code section, and then call
///  xos_restore_int_pri_level().
///
///  The interrupt priority level is part of the thread context, so it is saved
///  and restored across context switches. To enable and disable individual
///  interrupts globally, use the functions xos_interrupt_enable() and
///  xos_interrupt_disable() instead.
///
///  NOTE: It is usually not required to disable interrupts at a level higher
///  than that of the highest priority interrupt that interacts with the OS
///  (i.e. calls into XOS such that threads may be woken / blocked / 
///  reprioritized / switched, or otherwise access XOS data structures).
///  In XOS, that maximum level is XOS_MAX_OS_INTLEVEL, which defaults to
///  XCHAL_EXCM_LEVEL. This may be modified by editing xos_params.h and
///  rebuilding XOS.
///
///  \param     level           The new interrupt priority level (IPL).
///
///  \return    Returns a value that can be used to restore the previous
///             priority level by calling xos_restore_int_pri_level(). This
///             value should be treated as opaque by application code, and
///             should be passed unchanged to the restore function.
///
//-----------------------------------------------------------------------------
__attribute__((always_inline))
static inline uint32_t
xos_set_int_pri_level(uint32_t level)
{
    return xthal_intlevel_set_min(level);
}


//-----------------------------------------------------------------------------
///
///  Restores the CPU to a previously saved interrupt priority level. This level
///  must have been obtained by calling xos_set_int_pri_level().
///
///  \param     oldval          Return value from xos_set_int_pri_level().
///
///  \return    Returns nothing.
///
//-----------------------------------------------------------------------------
__attribute__((always_inline))
static inline void
xos_restore_int_pri_level(const uint32_t oldval)
{
    (void) xthal_intlevel_set(oldval);
}


//-----------------------------------------------------------------------------
///
///  Disable all interrupts.
///
///  Returns: A value that can be used to restore the previous interrupt state
///  by calling xos_restore_interrupts(). This value should be treated as
///  opaque by application code, and should be passed unchanged to the restore
///  function.
///
//-----------------------------------------------------------------------------
__attribute__((always_inline))
static inline uint32_t
xos_disable_interrupts(void)
{
    return xthal_disable_interrupts();
}


//-----------------------------------------------------------------------------
///
///  Restore the CPU's previously saved interrupt status.
///
///  \param     rval            Return value from xos_disable_interrupts().
///
///  \return    Returns nothing.
///
//-----------------------------------------------------------------------------
__attribute__((always_inline))
static inline void
xos_restore_interrupts(uint32_t rval)
{
    (void) xthal_restore_interrupts(rval);
}


// These files use things defined above so have to be included late.
// parasoft-begin-suppress MISRAC2012-RULE_20_1-a-4 "Included here due to dependencies."
#include "xos_log.h"
#include "xos_timer.h"
#include "xos_stopwatch.h"
// parasoft-end-suppress MISRAC2012-RULE_20_1-a-4 "Included here due to dependencies."


// Misc

//-----------------------------------------------------------------------------
///
///  Run internal system integrity checks and report result.
///
///  \param     ppthread        Pointer to storage for a thread pointer. If a
///                             problematic thread is detected then a pointer
///                             to it will be returned here.
///
///  \param     disable_p       If true, disable preemption while running checks.
///
///  \return    Returns XOS_OK if no problems detected, else error code.
///             Also returns a pointer to problematic thread if relevant.
///
//-----------------------------------------------------------------------------
int32_t
xos_system_check(XosThread ** ppthread, bool disable_p);


//-----------------------------------------------------------------------------
///
///  Check the state of the specified thread and report any problems.
///
///  \param     thread          Thread to be checked.
///
///  \return    XOS_OK if no problems detected, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_thread_check(XosThread * thread);


//-----------------------------------------------------------------------------
//  Helper function to list all threads in system. Useful for debug.
//-----------------------------------------------------------------------------
void
xos_display_threads(void * arg, XosPrintFunc * print_fn);


//-----------------------------------------------------------------------------
//  Returns the XOS library's build config signature.
//-----------------------------------------------------------------------------
uint32_t
xos_get_config_sig_lib(void);


//-----------------------------------------------------------------------------
///
///  Check build configuration mismatch between XOS library and application.
///  This function checks a build configuration signature embedded in the XOS
///  library against a similar signature that is generated while building the
///  application. A signature mismatch indicates that the XOS library and the
///  application are built with different configuration options. This can cause
///  incorrect behavior and system crashes.
///
///  Each bit of the signature indicates a specific option being enabled (1) or
///  disabled (0).
///
///  \latexonly \begin{table}[h] \caption{XOS Signature Fields} \end{table} \endlatexonly
///  | Bit Position | Option |
///  | ------------ | ------ |
///  | Bit 0  | XOS_DEBUG_ALL                |
///  | Bit 1  | XOS_DEBUG                    |
///  | Bit 2  | XOS_OPT_STATS                |
///  | Bit 3  | XOS_OPT_STACK_CHECK          |
///  | Bit 4  | XOS_OPT_INTERRUPT_SWPRI      |
///  | Bit 5  | XOS_OPT_THREAD_SAFE_CLIB     |
///  | Bit 6  | XOS_OPT_WAIT_TIMEOUT         |
///  | Bit 7  | XOS_OPT_TIMER_WAIT           |
///  | Bit 8  | XOS_OPT_TIMER_THREAD         |
///  | Bit 9  | XOS_OPT_TIME_SLICE           |
///  | Bit 10 | XOS_OPT_MUTEX_PRIORITY       |
///  | Bit 11 | XOS_OPT_LOG_SYSEVENT         |
///  | Bit 12 | XOS_OPT_C11_ATOMICS          |
///  | Bit 13 | XOS_OPT_THREAD_LOCAL_STORAGE |
///
///  \param     app_cfgp        Pointer to storage for the application config
///                             signature. The signature is returned as an
///                             unsigned 32-bit integer.
///
///  \param     lib_cfgp        Pointer to storage for the library config
///                             signature. The signature is returned as an
///                             unsigned 32-bit integer.
///
///  \return    Returns true if the signatures match, false if they do not.
///  
///  NOTE: If you only want to check whether there is a mismatch, you can
///  pass NULL for both "app_cfgp" and "lib_cfgp".
//-----------------------------------------------------------------------------
__attribute__((always_inline))
static inline bool
xos_check_build_config(uint32_t * app_cfgp, uint32_t * lib_cfgp)
{
    if (lib_cfgp != XOS_NULL) {
        *lib_cfgp = xos_get_config_sig_lib();
    }
    if (app_cfgp != XOS_NULL) {
        *app_cfgp = (uint32_t) XOS_CONFIG_SIG;
    }
    return ((uint32_t)XOS_CONFIG_SIG == xos_get_config_sig_lib());
}


//-----------------------------------------------------------------------------
///
///  Allowed states for once_control variable.
///
//-----------------------------------------------------------------------------
#define XOS_ONCE_INIT           0U
#define XOS_ONCE_INPROGRESS     1U
#define XOS_ONCE_DONE           2U


//-----------------------------------------------------------------------------
///
///  This function supports one-time initialization. The first call to this
///  function from any thread with a specific "once_control" will cause the
///  "init_routine" function to be called. Subsequent calls will not invoke
///  init_routine. Any call to this function (from any thread) will return
///  only after init_routine has completed.
///  
///  "once_control" must be initialized with the value XOS_ONCE_INIT, else the
///  result of calling xos_once() is undefined. Each control variable must be
///  initialized before any thread calls xos_once() for that variable. The
///  safest way is to declare it as a static initialized variable.
///
///      uint32_t my_once = XOS_ONCE_INIT;
///      
///      ret = xos_once(&my_once, init_routine);
///
///  \param     once_control    Pointer to control variable for one-time init.
///                             Must be initialized to XOS_ONCE_INIT before
///                             first use.
///
///  \param     init_routine    Pointer to init function. Cannot be NULL.
///
///  \return    Returns XOS_OK if successful, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_once(uint32_t * once_control, void (*init_routine)(void));


// User-mode support

//-----------------------------------------------------------------------------
///
///  Initialize XOS user-mode support. This function must be called before any
///  user-mode threads can be created or user-mode memory can be allocated.
///
///  \param     umem            Pointer to the contiguous memory region that
///                             will be used as the user-mode memory pool for
///                             dynamic allocation.
///
///  \param     umem_size       Size of the memory region.
///
///  \return    Returns XOS_OK if successful, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_um_init(void * umem, uint32_t umem_size);


//-----------------------------------------------------------------------------
///
///  Allocate a user-mode TCB from the TCB pool. This TCB is meant to be used
///  to create a user-mode thread.
///
///  \return    Returns a pointer to the allocated TCB, or NULL if no free
///             TCB is found.
///
//-----------------------------------------------------------------------------
XosUThread *
xos_um_tcb_alloc(void);


//-----------------------------------------------------------------------------
///
///  Free a previously allocated user-mode TCB back to the TCB pool.
///
///  \param     tcb             Pointer to the TCB to be freed.
///
///  \return    Returns XOS_OK if successful, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_um_tcb_free(XosUThread * tcb);


//-----------------------------------------------------------------------------
///
///  Validate a user-mode TCB pointer.
///
///  \param     tcb             The TCB pointer to validate.
///
///  \return    Returns true if the pointer is valid, else false.
///
//-----------------------------------------------------------------------------
bool
xos_um_tcb_valid(XosUThread * tcb);


//-----------------------------------------------------------------------------
///
///  Allocate a block of memory from the user-mode pool. These blocks are meant
///  to be used by user-mode threads.
///
///  \param     nbytes          Number of bytes to allocate. Must not be zero.
///
///  \param     align           Desired alignment. This will be rounded up to
///                             the MPU minimum alignment if a smaller value
///                             is specified.
///
///  \param     pp              Address of variable where the allocated pointer
///                             will be returned. Must not be NULL.
///
///  \return    Returns XOS_OK if successful, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_um_private_mem_alloc(uint32_t nbytes, uint32_t align, void ** pp);


//-----------------------------------------------------------------------------
///
///  Free a previously allocated memory block back to the user-mode pool.
///
///  \param     p               Pointer to block to be freed. Can be NULL.
///
///  \return    Returns XOS_OK if successful, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_um_private_mem_free(void * p);


#ifdef __cplusplus
}
#endif

#endif  // XOS_H

