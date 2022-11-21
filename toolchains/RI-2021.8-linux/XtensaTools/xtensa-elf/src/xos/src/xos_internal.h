
// xos_internal.h - XOS internal declarations/definitions.

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


#ifndef XOS_INTERNAL_H
#define XOS_INTERNAL_H

#if !defined(XOS_H) || !defined(XOS_INCLUDE_INTERNAL)
  #error "xos_internal.h must be included by defining XOS_INCLUDE_INTERNAL before including xos.h"
#endif

#include <xtensa/config/core.h>
#include "xos_common.h"

#ifdef __cplusplus
extern "C" {
#endif


// Use this macro to suppress compiler warnings for unused variables.
#ifndef UNUSED
#define UNUSED(x)       ((void)(x))    // parasoft-suppress MISRAC2012-DIR_4_9-a-4 "To suppress compiler warnings about unused variables"
#endif


// Only for debug support, therefore the following MISRA rules are suppressed:
// parasoft-begin-suppress MISRAC2012-RULE_17_1-a-2 "OK to use stdarg.h for debug code"
// parasoft-begin-suppress MISRAC2012-RULE_17_1-b-2 "OK to use stdarg.h for debug code"
// parasoft-begin-suppress MISRAC2012-DIR_4_9-a-4   "OK to use function-like macros for debug code"

#if XOS_DEBUG

#include <stdio.h>
#include <xtensa/xtutil.h>

// Controls debug output.
extern bool xos_debug_print;

// Debug printf function.
__attribute__((always_inline))
static inline void
DPRINTF(const char * fmt, ...)
{
    if (xos_debug_print != 0) {
        va_list ap;

        va_start(ap, fmt);
        (void) xt_vprintf(fmt, ap);
        va_end(ap);
    }
}

// Debug char output function.
__attribute__((always_inline))
static inline void
DPUTCHAR(char ch)
{
    if (xos_debug_print != 0) {
        xt_putchar(ch);
    }
}

#else

#define DPRINTF(...)    do {} while(0)
#define DPUTCHAR(ch)    do {} while(0)

#endif

// parasoft-end-suppress MISRAC2012-RULE_17_1-a-2
// parasoft-end-suppress MISRAC2012-RULE_17_1-b-2
// parasoft-end-suppress MISRAC2012-DIR_4_9-a-4


//-----------------------------------------------------------------------------
// Type conversion helpers.
//-----------------------------------------------------------------------------

__attribute__((always_inline))
static inline uint32_t *
xos_voidp_to_uint32p(void * arg)
{
    return (uint32_t *) arg; // parasoft-suppress MISRAC2012-RULE_11_5-a-4 "Type conversion checked"
}

__attribute__((always_inline))
static inline void *
xos_uint32p_to_voidp(uint32_t * arg)
{
    return (void *) arg; // parasoft-suppress MISRAC2012-RULE_11_5-a-4 "Type conversion checked"
}

__attribute__((always_inline))
static inline uint32_t
xos_voidp_to_uint32(void * arg)
{
    return (uint32_t) arg; // parasoft-suppress MISRAC2012-RULE_11_6-a-2 "Type conversion checked"
}

__attribute__((always_inline))
static inline void *
xos_uint32_to_voidp(uint32_t arg)
{
    return (void *) arg; // parasoft-suppress MISRAC2012-RULE_11_6-a-2 "Type conversion checked"
}

__attribute__((always_inline))
static inline uint32_t *
xos_uint32_to_uint32p(uint32_t arg)
{
    return (uint32_t *) arg; // parasoft-suppress MISRAC2012-RULE_11_4-a-4 "Type conversion checked"
}

__attribute__((always_inline))
static inline uint32_t
xos_uint32p_to_uint32(uint32_t * arg)
{
    return (uint32_t) arg; // parasoft-suppress MISRAC2012-RULE_11_4-a-4 "Type conversion checked"
}


//-----------------------------------------------------------------------------
// Internal flags for thread creation.
//-----------------------------------------------------------------------------
#define XOS_THREAD_FAKE         0x8000U // Don't allocate stack (init and idle threads).
#define XOS_THREAD_USERMODE     0x4000U // Thread will run in user mode.


//-----------------------------------------------------------------------------
// Defines for user-mode object keys.
//-----------------------------------------------------------------------------
#define XOS_KEY_ZERO            0ULL


//-----------------------------------------------------------------------------
// Interrupt handler table entry. This structure defines one entry in the XOS
// interrupt handler table.
//-----------------------------------------------------------------------------
typedef struct XosIntEntry {
  XosIntFunc *    handler;      // Pointer to handler function.
  void *          arg;          // Argument passed to handler function.
#if XCHAL_HAVE_XEA2
#if XOS_OPT_INTERRUPT_SWPRI
  uint8_t         level;        // Interrupt level.
  uint8_t         priority;     // Interrupt priority.
  uint16_t        reserved;     // Reserved.
  uint32_t        primask;      // Mask of interrupts at higher priority.
#else
  uint32_t        ps;           // Value of PS when running the handler.
#endif
#endif
} XosIntEntry;


//-----------------------------------------------------------------------------
// Extern variables.
//-----------------------------------------------------------------------------
// parasoft-begin-suppress MISRAC2012-RULE_8_6-a-2 "Defined in assembly code"
#if XCHAL_HAVE_INTERRUPTS
#if XCHAL_HAVE_XEA2
extern uint32_t         xos_intlevel_mask;
extern uint32_t         xos_intenable_mask;
#endif
#if XOS_USE_INT_WRAPPER && XCHAL_HAVE_XEA2
extern XosIntEntry      xos_interrupt_table[XCHAL_NUM_INTERRUPTS + 1];
#else
extern XosIntEntry      xos_interrupt_table[XCHAL_NUM_INTERRUPTS];
#endif
#endif
// parasoft-end-suppress MISRAC2012-RULE_8_6-a-2


//-----------------------------------------------------------------------------
// Extern functions and functions declared to avoid compiler warnings.
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//  Default interrupt handler.
//-----------------------------------------------------------------------------
void
xos_int_unhandled(void * arg);


//-----------------------------------------------------------------------------
//  Default exception handler.
//-----------------------------------------------------------------------------
void
xos_default_exception_handler(XosExcFrame * arg);


//-----------------------------------------------------------------------------
//  Returns true if the interrupt is in use (a handler has been installed).
//-----------------------------------------------------------------------------
bool
xos_interrupt_in_use(uint32_t intnum);


//-----------------------------------------------------------------------------
//  Init the once module.
//-----------------------------------------------------------------------------
void
xos_once_init(void);


//-----------------------------------------------------------------------------
//  Init the timer module.
//-----------------------------------------------------------------------------
void
xos_timer_module_init(void);


//-----------------------------------------------------------------------------
// Functions used internally by XOS.
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//  Enter an OS critical section, i.e. get exclusive access to OS critical 
//  state and data structures. Code that manipulates the state of OS objects
//  or modifies internal OS state must call this function first, to ensure
//  that it has exclusive access. On a single-core system, this is equivalent
//  to blocking all interrupts that can interact directly with the OS, i.e.
//  all interrupts at or below XOS_MAX_OS_INTLEVEL. In a multi-core system
//  this is likely to be implemented differently to achieve the same effect.
//
//  Returns: A value that is to be used to restore the state of the CPU when
//  exiting the critical section. This must be treated as opaque and passed
//  unmodified to xos_critical_exit().
//
//  NOTE: This function is meant for use in OS code, not in applications.
//-----------------------------------------------------------------------------
__attribute__((always_inline))
static inline uint32_t
xos_critical_enter(void)
{
#if XCHAL_HAVE_XEA2
    return xthal_intlevel_set_min(XOS_MAX_OS_INTLEVEL);
#else
    return xthal_disable_interrupts();
#endif
}


//-----------------------------------------------------------------------------
//  Exit an OS critical section and restore CPU state. See the documentation
//  for xos_critical_enter().
//
//  cflags              Return value from xos_critical_enter().
//                      Must be treated as an opaque value.
//
//  Returns: Nothing.
//
//  NOTE: This function is meant for use in OS code, not in applications.
//-----------------------------------------------------------------------------
__attribute__((always_inline))
static inline void
xos_critical_exit(uint32_t cflags)
{
#if XCHAL_HAVE_XEA2
    (void) xthal_intlevel_set(cflags);
#else
    xthal_restore_interrupts(cflags);
#endif
}


//-----------------------------------------------------------------------------
//  Return true if in OS critical section, false if not.
//-----------------------------------------------------------------------------
__attribute__((always_inline))
static inline bool
xos_in_critical_section(void)
{
#if XCHAL_HAVE_XEA2
    return (xthal_intlevel_get() >= (uint32_t)XOS_MAX_OS_INTLEVEL);
#elif XCHAL_HAVE_XEA3
    return ((XT_RSR_PS() & PS_DI) == PS_DI);
#endif
}


//-----------------------------------------------------------------------------
//  Return true if in interrupt context, false if not.
//-----------------------------------------------------------------------------
__attribute__((always_inline))
static inline bool
xos_interrupt_context(void)
{
#if XCHAL_HAVE_INTERRUPTS && XCHAL_HAVE_XEA2
    return ((XT_RSR_PS() & (uint32_t)PS_UM) == 0U);
#elif XCHAL_HAVE_INTERRUPTS && XCHAL_HAVE_XEA3
    uint32_t pss = (XT_RSR_PS() >> PS_STACK_SHIFT) & (PS_STACK_MASK >> PS_STACK_SHIFT);
    // Using the PS_ constants produces worse output from the compiler.
    return (pss <= 1U);
#else
    return 0;
#endif
}


//-----------------------------------------------------------------------------
//  Return true if in nested interrupt context, false if not.
//-----------------------------------------------------------------------------
__attribute__((always_inline))
static inline bool
xos_interrupt_nested(void)
{
#if XCHAL_HAVE_INTERRUPTS && XCHAL_HAVE_XEA2
    extern char xos_dispatch_from_nest; // parasoft-suppress MISRAC2012-RULE_8_6-a-2 "Label defined in assembly code"
    return ((xos_interrupt_context()) &&
            (XOS_RSR_XHREG() == xos_voidp_to_uint32((void *)&xos_dispatch_from_nest)));
#elif XCHAL_HAVE_INTERRUPTS && XCHAL_HAVE_XEA3
    return ((XT_RSR_PS() & PS_STACK_MASK) == PS_STACK_INTERRUPT);
#else
    return 0;
#endif
}


//-----------------------------------------------------------------------------
//  Macro equivalents for the above inline functions.
//-----------------------------------------------------------------------------
#define INTERRUPT_CONTEXT       xos_interrupt_context()
#define INTERRUPT_NESTED        xos_interrupt_nested()
#define IN_CRITICAL_SECTION     xos_in_critical_section()


#ifdef __cplusplus
}
#endif

#endif  /* XOS_INTERNAL_H */

