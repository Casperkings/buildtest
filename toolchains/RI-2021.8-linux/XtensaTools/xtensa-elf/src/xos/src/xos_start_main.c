
// xos_start_main.c - XOS startup option to start main() as a thread.

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


#include <alloca.h>
#include <xtensa/tie/xt_core.h>

#define XOS_INCLUDE_INTERNAL
#include "xos.h"


//-----------------------------------------------------------------------------
// Defines and macros.
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Externs. See xos_handlers.S for assembly functions.
//-----------------------------------------------------------------------------

// parasoft-begin-suppress MISRAC2012-RULE_8_6-a-2  "Defined in assembly code / linker script"
// parasoft-begin-suppress MISRAC2012-RULE_21_2-a-2 "Must use system defined names here"

extern void
xos_resume_idle_thread(void); 

extern char _stack_sentry;  // min boundary of stack (linker generated)
extern char __stack;        // initial stack ptr (linker generated)

// parasoft-end-suppress MISRAC2012-RULE_8_6-a-2
// parasoft-end-suppress MISRAC2012-RULE_21_2-a-2


//-----------------------------------------------------------------------------
// Threads related state.
//-----------------------------------------------------------------------------

// These are used only if main() is to be a thread.
static XosThread xos_init_thread;           // Initial thread (main)
static uint8_t   xos_main_tie[XT_CP_SIZE];  // parasoft-suppress MISRAC2012-RULE_10_1-e-2 "Operands verified OK"


//-----------------------------------------------------------------------------
//  Start XOS and convert the caller (presumed main) into the main/init thread
//  of the system. The idle thread is also created. Control returns to the 
//  caller at the end of thread creation.
//-----------------------------------------------------------------------------
static int32_t
xos_start_main_int(const char * name, int8_t priority, uint32_t stack_size)
{
    uint32_t tmp = xos_voidp_to_uint32(&__stack);
    int32_t  ret;

    // Can't call this from interrupt context.
    if (INTERRUPT_CONTEXT) {
        xos_fatal_error(XOS_ERR_INTERRUPT_CONTEXT, XOS_NULL);
    }

    if ((name == XOS_NULL) || ((priority < 0) || (priority >= XOS_MAX_PRIORITY))) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    // Can't start more than once.
    if (xos_started() == true) {
        return XOS_ERR_ILLEGAL_OPERATION;
    }

    // Make sure init is done before creating any threads.
    xos_init();

    // xos_start_main does not allow specifying the stack size, so the size
    // is set to the max available space. This can collide with the heap if
    // the application uses dynamic memory allocation.
    // xos_start_main_ex does specify the stack size, so we can both set the
    // size properly and initialize the remaining stack region.

    ret = xos_thread_create(&xos_init_thread,
                            XOS_NULL,
                            XOS_NULL,
                            XOS_NULL,
                            name,
                            (stack_size > 0U) ? xos_uint32_to_voidp(tmp - stack_size) : (void *)&_stack_sentry,
                            (stack_size > 0U) ? stack_size : (uint32_t)(&__stack - &_stack_sentry),
                            priority,
                            XOS_NULL,
                            XOS_THREAD_FAKE);

    if (ret != XOS_OK) {
        return ret;
    }

#if XOS_OPT_STACK_CHECK
    // Fill remaining stack with a pattern to keep track of how much gets used.
    // We can't fill the entire stack region because we are actually executing
    // on this stack. ONLY do this if stack size was explicitly specified.
    if (stack_size > 0U) {
        uint32_t * p1 = xos_voidp_to_uint32p(xos_init_thread.stack_base);
        uint32_t * p2 = alloca(16);

        while (p1 < (p2 + 4)) {
            *p1 = XOS_STACK_CHECKVAL;
            p1++;
        }
    }

    // Force spill all register windows to ensure all spill regions above the
    // current function are written to.
    xthal_window_spill();
#endif

    // The state save area is not part of the stack in this case.
    xos_thread_setup_tie_save(&xos_init_thread, xos_main_tie, true);

    // Make this the current thread.
    xos_curr_threadptr = &xos_init_thread;
    xos_next_threadptr = xos_curr_threadptr;

#if XOS_OPT_THREAD_SAFE_CLIB
    // Set the global C lib context ptr.
    GLOBAL_CLIB_PTR = xos_init_thread.clib_ptr;
#endif

    // Set up the idle thread. Note that this is never in any ready queue.
    ret = xos_thread_create(&xos_idle_thread,
                            XOS_NULL,
                            XOS_NULL,
                            XOS_NULL,
                            "idle",
                            XOS_NULL,
                            0,
                            0 /*-1*/,
                            XOS_NULL,
                            (XOS_THREAD_FAKE | XOS_THREAD_SUSPEND));

    if (ret != XOS_OK) {
        return ret;
    }

    xos_idle_thread.resume_fn     = &xos_resume_idle_thread;
    xos_idle_thread.block_cause   = xos_blkon_idle;
    xos_idle_thread.curr_priority = -1;

    // Create timer thread as needed.
    ret = xos_create_timer_thread();
    if (ret != XOS_OK) {
        return ret;
    }

    // Set the started flag
    xos_start_flag = true;

#if XOS_OPT_STATS
    // Start cycle counting for the main thread. This is somewhat approximate.
    xos_init_thread.resume_ccount = xos_get_ccount();
#endif

#if XCHAL_HAVE_INTERRUPTS
#if XCHAL_HAVE_XEA2
    // Explicitly apply any pending interrupt enables now, else that will
    // happen only on the next context switch.
    XT_WSR_INTENABLE(xos_intenable_mask & xos_intlevel_mask);
#else
    // Enable interrupts at the core now. Individual interrupts may have
    // been enabled already at the interrupt controller.
    (void) xthal_enable_interrupts();
#endif
#endif

    // Enable preemption and return to caller (we are now the init thread).
    // Remember that if a higher priority thread has already been created
    // then we will switch to that thread now.
    (void) xos_preemption_enable();

    return XOS_OK;
}


//-----------------------------------------------------------------------------
//  Start XOS and convert the caller (presumed main) into the main/init thread
//  of the system. The idle thread is also created. Control returns to the 
//  caller at the end of thread creation.
//-----------------------------------------------------------------------------
void
xos_start_main(const char * name, int8_t priority, uint32_t flags)
{
    UNUSED(flags);

    (void) xos_start_main_int(name, priority, 0U);
}


//-----------------------------------------------------------------------------
//  Start XOS and convert the caller (presumed main) into the main/init thread
//  of the system. The idle thread is also created. Control returns to the 
//  caller at the end of thread creation. The stack size for the main thread
//  is explicitly specified by the caller.
//-----------------------------------------------------------------------------
int32_t
xos_start_main_ex(const char * name, int8_t priority, uint32_t stack_size)
{
    // Stack size will get checked during thread creation.
    return xos_start_main_int(name, priority, stack_size);
}

