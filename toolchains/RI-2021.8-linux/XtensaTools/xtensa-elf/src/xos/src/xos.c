
// xos.c - XOS Misc functions.

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


#define XOS_INCLUDE_INTERNAL
#include "xos.h"
#if XOS_OPT_SECMON
#include "xos_sm.h"
#endif

#if XCHAL_HAVE_DEBUG
#include <xtensa/tie/xt_debug.h>
#endif


//-----------------------------------------------------------------------------
// Externs and globals.
//-----------------------------------------------------------------------------

XosLog *    xos_log;

#if XOS_DEBUG
bool        xos_debug_print = (XOS_DEBUG_VERBOSE != 0);
#endif


//-----------------------------------------------------------------------------
// Interrupt manipulation.
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//  Register an interrupt handler for low and medium priority interrupts.
//-----------------------------------------------------------------------------
int32_t
xos_register_interrupt_handler(uint32_t num, XosIntFunc * handler, void * arg)
{
#if XCHAL_HAVE_INTERRUPTS

    XosIntEntry * entry;
    uint32_t      ps;

    if (num >= (uint32_t) XCHAL_NUM_INTERRUPTS) {
        return XOS_ERR_INVALID_PARAMETER;
    }
    if (handler == XOS_NULL) {
        return XOS_ERR_INVALID_PARAMETER;
    }

#if XCHAL_HAVE_XEA2
    if (Xthal_intlevel[num] > (uint8_t) XCHAL_EXCM_LEVEL) {
        return XOS_ERR_INVALID_PARAMETER;
    }
#endif

#if XOS_OPT_SECMON
    // Request the secure monitor to forward this interrupt.
    if (xos_xtsm_set_int_handler(num, (void *) handler) != 0) {
        return XOS_ERR_SECMON_REJECT;
    }
#endif

    ps = xos_critical_enter();

#if XOS_USE_INT_WRAPPER
    entry = &xos_interrupt_table[num + 1];
#else
    entry = &xos_interrupt_table[num];
#endif
    entry->handler = handler;
    entry->arg = arg;

    xos_critical_exit(ps);
    return XOS_OK;

#else

    UNUSED(num);
    UNUSED(handler);
    UNUSED(arg);

    return XOS_ERR_FEATURE_NOT_PRESENT;

#endif
}


//-----------------------------------------------------------------------------
//  Default interrupt handler. Called if an interrupt is taken for which no
//  handler was registered.
//-----------------------------------------------------------------------------
void
xos_int_unhandled(void * arg)
{
    UNUSED(arg);
    xos_fatal_error(XOS_ERR_UNHANDLED_INTERRUPT, XOS_NULL);
}


//-----------------------------------------------------------------------------
//  Unregister interrupt handler for low and medium priority interrupts.
//-----------------------------------------------------------------------------
int32_t
xos_unregister_interrupt_handler(uint32_t num) // parasoft-suppress MISRAC2012-RULE_1_1-b-2 "Identifier length retained for API compatibility"
{
    return xos_register_interrupt_handler(num, xos_int_unhandled, XOS_NULL);
}


//-----------------------------------------------------------------------------
//  Register a handler for a high priority interrupt level.
//-----------------------------------------------------------------------------
int32_t
xos_register_hp_interrupt_handler(uint32_t level, void * handler) // parasoft-suppress MISRAC2012-RULE_1_1-b-2 "Identifier length retained for API compatibility"
{
#if XCHAL_HAVE_XEA2 && XCHAL_HAVE_INTERRUPTS

#if XOS_OPT_SECMON
    // High priority interrupts will not be forwarded from secure mode.
    UNUSED(level);
    UNUSED(handler);
    return XOS_ERR_SECMON_REJECT;
#else
    extern void * xos_hp_dispatch_table[5]; // parasoft-suppress MISRAC2012-RULE_8_6-a-2 "Defined in assembly code"
    uint32_t save;

    if ((level <= (uint32_t) XCHAL_EXCM_LEVEL) || (level > (uint32_t) XCHAL_NUM_INTLEVELS)) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    // Must block all interrupts including high-priority ones.
    save = xos_set_int_pri_level(XCHAL_NUM_INTLEVELS);
    xos_hp_dispatch_table[level - 2U] = handler;
    xos_restore_int_pri_level(save);

    return XOS_OK;
#endif

#else

    UNUSED(level);
    UNUSED(handler);

    return XOS_ERR_FEATURE_NOT_PRESENT;

#endif
}


//-----------------------------------------------------------------------------
//  Enable a specified interrupt if XOS started, else mark as enable pending.
//-----------------------------------------------------------------------------
int32_t
xos_interrupt_enable(uint32_t intnum)
{
#if XCHAL_HAVE_INTERRUPTS

    if (intnum >= (uint32_t) XCHAL_NUM_INTERRUPTS) {
        return XOS_ERR_INVALID_PARAMETER;
    }

#if XCHAL_HAVE_XEA2

    // Disable all interrupts. We can't rule out the possibility of high-level
    // interrupts messing with interrupt enable states.
    uint32_t ps   = xos_disable_interrupts();
    uint32_t mask = ((uint32_t)1U) << (intnum & 0x1FU);

#pragma no_reorder
    xos_intenable_mask |= mask;

    // Update the INTENABLE register only if XOS is started.
    if (xos_started() == true) {
        mask = xos_intenable_mask & xos_intlevel_mask;
        XT_WSR_INTENABLE(mask);
    }

    // Restore old interrupt state.
    xos_restore_interrupts(ps);

#endif
#if XCHAL_HAVE_XEA3

    if (xos_started() == false) {
        // Disable all interrupts before enabling any. We don't want to take
        // any interrupts before XOS has started. Will be enabled on start.
        (void) xos_disable_interrupts();
    }
    xthal_interrupt_enable(intnum);

#endif

    return XOS_OK;

#else    // No interrupts

    UNUSED(intnum);

    return XOS_ERR_FEATURE_NOT_PRESENT;

#endif
}


//-----------------------------------------------------------------------------
//  Return enable state of specified interrupt.
//-----------------------------------------------------------------------------
bool
xos_interrupt_enabled(uint32_t intnum)
{
#if XCHAL_HAVE_INTERRUPTS

    if (intnum >= (uint32_t) XCHAL_NUM_INTERRUPTS) {
        return false;
    }

#if XCHAL_HAVE_XEA2

    return (xos_intenable_mask & ((uint32_t)1U << (intnum & 0x1FU))) != 0U;

#else

    return (xthal_interrupt_enabled(intnum) != 0U);

#endif

#else

    UNUSED(intnum);
    return false;

#endif
}


//-----------------------------------------------------------------------------
//  Disable a specified interrupt if XOS started, else mark as disable pending.
//-----------------------------------------------------------------------------
int32_t
xos_interrupt_disable(uint32_t intnum)
{
#if XCHAL_HAVE_INTERRUPTS

    if (intnum >= (uint32_t) XCHAL_NUM_INTERRUPTS) {
        return XOS_ERR_INVALID_PARAMETER;
    }

#if XCHAL_HAVE_XEA2

    // Disable all interrupts. We can't rule out the possibility of high-level
    // interrupts messing with interrupt enable states.
    uint32_t ps   = xos_disable_interrupts();
    uint32_t mask = ((uint32_t)1U) << (intnum & 0x1FU);

#pragma no_reorder
    xos_intenable_mask &= ~mask;

    // Update the INTENABLE register only if XOS is started.
    if (xos_started() == true) {
        mask = xos_intenable_mask & xos_intlevel_mask;
        XT_WSR_INTENABLE(mask);
    }

    // Restore old interrupt state.
    xos_restore_interrupts(ps);

#endif
#if XCHAL_HAVE_XEA3

    xthal_interrupt_disable(intnum);

#endif

    return XOS_OK;

#else    // No Interrupts

    UNUSED(intnum);

    return XOS_ERR_FEATURE_NOT_PRESENT;

#endif
}


//-----------------------------------------------------------------------------
//  Return TRUE if interrupt in use (non-default handler installed).
//-----------------------------------------------------------------------------
bool
xos_interrupt_in_use(uint32_t intnum)
{
#if XCHAL_HAVE_INTERRUPTS

    XosIntEntry * entry;
    
    if (intnum >= (uint32_t) XCHAL_NUM_INTERRUPTS) {
        return false;
    }

#if XOS_USE_INT_WRAPPER
    entry = &xos_interrupt_table[intnum + 1];
#else
    entry = &xos_interrupt_table[intnum];
#endif

    return entry->handler != &xos_int_unhandled;

#else    // No interrupts

    UNUSED(intnum);
    return false;

#endif
}


//-----------------------------------------------------------------------------
//  Error handling.
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//  User settable fatal error handler.
//-----------------------------------------------------------------------------
static XosFatalErrFunc * xos_fatal_handler;


//-----------------------------------------------------------------------------
//  Install a user defined fatal error handler.
//-----------------------------------------------------------------------------
XosFatalErrFunc *
xos_register_fatal_error_handler(XosFatalErrFunc * handler) // parasoft-suppress MISRAC2012-RULE_1_1-b-2 "Identifier length retained for API compatibility"
{
    XosFatalErrFunc * old = xos_fatal_handler;

    xos_fatal_handler = handler;
    return old;
}


//-----------------------------------------------------------------------------
//  Handle fatal errors. Defer to user handler if present, else halt.
//-----------------------------------------------------------------------------
void
xos_fatal_error(int32_t errcode, const char * errmsg)
{
    // Declare _exit as weak so we don't pull it in unless already used.
    extern void _exit(int32_t status) __attribute__((weak)); // parasoft-suppress MISRAC2012-RULE_21_2-a-2 MISRAC2012-RULE_21_2-b-2 MISRAC2012-RULE_21_2-c-2 MISRAC2012-RULE_8_6-a-2 "Defined this way for use only if already used"

    (void) xos_preemption_disable();

    // Call user handler if present. Not expected to return.
    if (xos_fatal_handler != XOS_NULL) {
        (*xos_fatal_handler)(errcode, errmsg);
    }

#if XOS_DEBUG
    // Force enable debug output to ensure this message is seen.
    xos_debug_print = 1;
    DPRINTF(" ** FATAL ERROR (%d) %s\n", errcode, (errmsg != XOS_NULL) ? errmsg : "");
#endif

    // Call _exit if available.
    if (&_exit != XOS_NULL) {
        _exit(errcode);
    }

    // Attempt to trap to debugger if present, else suspend here.
#if XCHAL_HAVE_DEBUG
    XT_BREAK(1, 15);
#endif
    while (1) {}
}


//-----------------------------------------------------------------------------
//  Runtime assertion.
//-----------------------------------------------------------------------------
#if XOS_DEBUG
void
xos_assert(const char * file, int32_t line)
{
    UNUSED(file);
    UNUSED(line);

    xos_fatal_error(XOS_ERR_ASSERT_FAILED, XOS_NULL);
}
#endif


//-----------------------------------------------------------------------------
//  Exception handling.
//  All user-overridable exceptions are dispatched through the handler table
//  xos_exc_handler_table. To start with, every entry in the table points to
//  xos_default_exception_handler() (see below). The application can set its
//  own handler for an exception by calling xos_register_exception_handler().
//  It is not allowed to not have a handler - if a NULL is passed as the
//  handler address, the default handler will be used.
//
//  The following exceptions are handled by XOS and the handlers cannot be
//  overridden by xos_register_exception_handler():
//
//  - Window overflow and underflow exceptions
//  - Coprocessor exceptions
//  - Alloca exceptions
//  - Level 1 interrupts
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//  Default exception handler. If the exception occurs in a user thread, then
//  the thread is terminated with an error. Any waiting threads will be woken
//  with this error code as the return value.
//  If the exception occurs in OS context, then it is considered to be a fatal
//  error and the fatal error handler is called.
//-----------------------------------------------------------------------------
void
xos_default_exception_handler(XosExcFrame * arg)
{
#if !XOS_DEBUG
    UNUSED(arg);
#endif

#if XCHAL_HAVE_EXCEPTIONS

    XosThread * thread = xos_thread_id();
    int32_t     retval = XOS_ERR_UNHANDLED_EXCEPTION;
#if XOS_DEBUG
    const char * errmsg = "unhandled";
#endif

    if ((INTERRUPT_CONTEXT) || (thread == XOS_NULL) || (thread == &xos_idle_thread)) {
        // If no active thread then exception was in OS code, fatal.
        // Same if exception occurred in interrupt handler.
        xos_fatal_error(XOS_ERR_UNHANDLED_EXCEPTION, XOS_NULL);
    }

    // Otherwise, terminate the user thread and notify any waiters.
#if XOS_OPT_STACK_CHECK

    // This handler is running on the thread's stack. If the stack pointer
    // is out of bounds, report this as a stack overrun.
    if (((void *)(&thread) < thread->stack_base) || // parasoft-suppress MISRAC2012-RULE_11_4-a-4 "Type conversion for bounds check only"
        ((void *)(&thread) >= thread->stack_end)) { // parasoft-suppress MISRAC2012-RULE_11_4-a-4 "Type conversion for bounds check only"
#if XOS_DEBUG
        errmsg = "possible stack overrun";
#endif
        retval = XOS_ERR_STACK_OVERRUN;
    }
#endif

#if XOS_DEBUG
    // Force enable debug output to ensure this message is seen.
    xos_debug_print = 1;
    DPRINTF("** ERROR: exception (%x) in thread '%s' (%s) **\n",
            arg->exccause, thread->name, errmsg);
#endif

    xos_thread_exit(retval);

#else

    // Don't have exceptions, how did we get here ?
    DPRINTF("** ERROR: internal error in thread '%s' **\n", thread->name);
    xos_thread_exit(XOS_ERR_INTERNAL_ERROR);

#endif
}


extern XosExcHandlerFunc * xos_exc_handler_table[XCHAL_EXCCAUSE_NUM]; // parasoft-suppress MISRAC2012-RULE_8_6-a-2 "Defined in assembly code"

//-----------------------------------------------------------------------------
//  Install user-defined exception handler. If user passes NULL, use default
//  handler.
//-----------------------------------------------------------------------------
XosExcHandlerFunc *
xos_register_exception_handler(uint32_t exc, XosExcHandlerFunc * handler)
{
#if XCHAL_HAVE_EXCEPTIONS

    XosExcHandlerFunc * old;
    uint32_t            ps;

    if (exc >= (uint32_t) XCHAL_EXCCAUSE_NUM) {
        return XOS_NULL;
    }

#if XOS_OPT_SECMON
    if (xos_xtsm_set_exc_handler(exc, (void *) handler) != 0) {
        return XOS_NULL;
    }
#endif

    ps = xos_critical_enter();

    old = xos_exc_handler_table[exc];
    xos_exc_handler_table[exc] =
        (handler != XOS_NULL) ? handler : xos_default_exception_handler;

    xos_critical_exit(ps);
    return old;

#else

    return XOS_NULL;

#endif
}


//-----------------------------------------------------------------------------
//  Return the library's build config signature.
//-----------------------------------------------------------------------------
uint32_t
xos_get_config_sig_lib(void)
{
    static uint32_t xos_config_sig_lib = XOS_CONFIG_SIG;

    return xos_config_sig_lib;
}

