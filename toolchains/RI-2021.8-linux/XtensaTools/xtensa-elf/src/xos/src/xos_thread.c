
// xos_thread.c - XOS Thread management.

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


#include <xtensa/tie/xt_core.h>

#define XOS_INCLUDE_INTERNAL
#include "xos.h"
#if XOS_OPT_SECMON
#include "xos_sm.h"
#endif

#if XCHAL_HAVE_NSA
#include <xtensa/tie/xt_misc.h>
#endif

#if XCHAL_HAVE_ISL || XCHAL_HAVE_KSL || XCHAL_HAVE_PSL
#include <xtensa/tie/xt_exception_dispatch.h>
#endif

#if XOS_OPT_UM
#include <xtensa/tie/xt_mmu.h>
#endif


//-----------------------------------------------------------------------------
// Defines and macros.
//-----------------------------------------------------------------------------
#define XOS_THREAD_CALL_MASK        ((uint32_t)0xC0000000UL)


//-----------------------------------------------------------------------------
// Externs. See xos_handlers.S for assembly functions/labels.
//-----------------------------------------------------------------------------

// parasoft-begin-suppress MISRAC2012-RULE_8_6-a-2 "Defined in assembly code"

extern void
xos_start_thread(XosThread * next_thread);

extern void
xos_resume_cooperative_thread(void);

extern void
xos_resume_preempted_thread(void);

extern void
xos_resume_idle_thread(void);

extern void
xos_resume_by_restart(void);

extern void
xos_thread_abort_stub(void);

extern char
xos_dispatch_from_thread;

extern int32_t
xos_switch_thread(XosThread * next_thread);

// parasoft-end-suppress MISRAC2012-RULE_8_6-a-2


//-----------------------------------------------------------------------------
// Block causes.
//-----------------------------------------------------------------------------
const char * const xos_blkon_idle       = "idle";       // Idle thread only
const char * const xos_blkon_suspend    = "suspend";
const char * const xos_blkon_delay      = "delay";
const char * const xos_blkon_exited     = "exited";
const char * const xos_blkon_join       = "join";
const char * const xos_blkon_event      = "event";
const char * const xos_blkon_condition  = "cond";
const char * const xos_blkon_mutex      = "mutex";
const char * const xos_blkon_sem        = "sem";
const char * const xos_blkon_msgq       = "msgq";


//-----------------------------------------------------------------------------
// Threads related state.
//-----------------------------------------------------------------------------
static bool     xos_init_flag  = false;  // Init done flag
bool            xos_start_flag = false;  // Started flag

XosThread       xos_idle_thread;        // Idle thread (lowest priority)
XosThread *     xos_all_threads;        // List of all threads

static XosThread **    xos_all_threads_tail;
static uint32_t        xos_ready_mask;          // Supports only 32 priority levels
static XosThreadQueue  xos_ready_threads[XOS_MAX_PRIORITY];

#if XOS_USE_SCHED_INT
static uint32_t xos_sched_intr = 0xFFFFFFFFU;   // SW interrupt used for scheduling
#endif


//-----------------------------------------------------------------------------
// Number of context switches performed. Mainly useful for performance and
// debug. Could eventually roll over.
//-----------------------------------------------------------------------------
static uint32_t xos_num_ctx_switches;


//-----------------------------------------------------------------------------
// Scheduler lock counter.
//-----------------------------------------------------------------------------
static uint32_t xos_scheduler_locked;


//-----------------------------------------------------------------------------
//  Thread Queue handling. ALL these functions assume that interrupts have been
//  disabled when necessary by the caller.
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//  Return index of most significant bit set in 'n', or -1 if 0.
//  Used for finding highest priority ready thread (looks at only as many bits
//  as needed per XOS_MAX_PRIORITY).
//-----------------------------------------------------------------------------
static inline int32_t
find_msbit_pri(uint32_t n)
{
#if XCHAL_HAVE_NSA
    return (int32_t)(31U - XT_NSAU(n));
#else
    int32_t i = 0;

    if (n == 0)
        return -1;
# if XOS_MAX_PRIORITY > 16
    if (n >= 0x10000) {  n >>= 16; i += 16;  }
# endif
# if XOS_MAX_PRIORITY > 8
    if (n >= 0x100)   {  n >>= 8;  i += 8;  }
# endif
# if XOS_MAX_PRIORITY > 4
    if (n >= 0x10)    {  n >>= 4;  i += 4;  }
# endif
# if XOS_MAX_PRIORITY > 2
    if (n >= 4)       {  n >>= 2;  i += 2;  }
# endif
# if XOS_MAX_PRIORITY > 1
    if (n >= 2)       {  i++;  }
# endif
    return i;
#endif // XCHAL_HAVE_NSA
}


//-----------------------------------------------------------------------------
//  Remove thread from queue. Assumes thread is in this queue.
//-----------------------------------------------------------------------------
void
xos_q_remove(XosThreadQueue * q, XosThread * thread)
{
    XosThread *  next;
    XosThread ** pprev;

    XOS_ASSERT((q != XOS_NULL) && (thread != XOS_NULL));

    next  = thread->r_next;
    pprev = thread->r_pprev;

    XOS_ASSERT((pprev != XOS_NULL) && (*pprev == thread));

    *pprev = next;
    if (*pprev != XOS_NULL) {
        next->r_pprev = pprev;
    }
    else {
        q->tail = pprev;
    }

    thread->r_next  = XOS_NULL;
    thread->r_pprev = XOS_NULL;
    thread->wq_ptr  = XOS_NULL;
}


//-----------------------------------------------------------------------------
//  Pop thread from head of thread queue. Returns pointer to thread, or NULL
//  if none in queue.
//-----------------------------------------------------------------------------
XosThread *
xos_q_pop(XosThreadQueue * q)
{
    XosThread * thread;

    XOS_ASSERT(q != XOS_NULL);

    thread = q->head;

    if (thread != XOS_NULL) {
        xos_q_remove(q, thread);
    }

    return thread;
}


//-----------------------------------------------------------------------------
//  Insert thread at tail of queue. Assumes thread is not already in a queue
//  (thread->r_next == 0 && thread->r_pprev == 0).
//-----------------------------------------------------------------------------
void
xos_q_push(XosThreadQueue * q, XosThread * thread)
{
    XOS_ASSERT((q != XOS_NULL) && (thread != XOS_NULL));
    XOS_ASSERT((thread->r_next == XOS_NULL) && (thread->r_pprev == XOS_NULL));

    thread->r_pprev = q->tail;
    *thread->r_pprev = thread;
    q->tail = &thread->r_next;
    thread->wq_ptr = q;

    // Queue has at least one element in it now.
    XOS_ASSERT((q->head != XOS_NULL) && (q->tail != &(q->head)));
}


//-----------------------------------------------------------------------------
//  Insert thread at head of queue. Assumes thread is not already in a queue
//  (thread->r_next == 0 && thread->r_pprev == 0).
//-----------------------------------------------------------------------------
void
xos_q_insert(XosThreadQueue * q, XosThread * thread)
{
    XOS_ASSERT((q != XOS_NULL) && (thread != XOS_NULL));
    XOS_ASSERT((thread->r_next == XOS_NULL) && (thread->r_pprev == XOS_NULL));

    if (q->head == XOS_NULL) {
        q->tail = &thread->r_next;
    }
    else {
        q->head->r_pprev = &thread->r_next;
    }

    thread->r_pprev = &q->head;
    thread->r_next  = q->head;

    q->head = thread;
    thread->wq_ptr = q;

    // Queue has at least one element in it now.
    XOS_ASSERT((q->head != XOS_NULL) && (q->tail != &(q->head)));
}


//-----------------------------------------------------------------------------
//  Insert thread in queue in priority order. That is, insert into queue such
//  that this thread is behind all threads that are at a higher or at equal
//  priority. This is useful in maintaining priority order queues for objects
//  such as mutexes and semaphores, where maintaining an array of queues by
//  priority is not practical, and queue lengths are expected to be small.
//  Assumes thread is not already in a queue (thread->r_next == 0 and
//  thread->r_pprev == 0).
//-----------------------------------------------------------------------------
void
xos_q_pri_insert(XosThreadQueue * q, XosThread * thread)
{
    XosThread *  p;
    XosThread ** pp;

    XOS_ASSERT((q != XOS_NULL) && (thread != XOS_NULL));
    XOS_ASSERT((thread->r_next == XOS_NULL) && (thread->r_pprev == XOS_NULL));

    // Find insertion point *pp => p (before any lower-pri threads)
    pp = &(q->head);
    p  = *pp;

    while (p != XOS_NULL) {
        if (p->curr_priority < thread->curr_priority) {
            break;
        }
        pp = &(p->r_next);
        p  = *pp;
    }

    // If we got to the end of the queue, update tail
    if (p == XOS_NULL) {
        q->tail = &thread->r_next;
    }

    // Insert between pp and p
    thread->r_pprev = pp;
    *pp = thread;
    thread->r_next = p;
    if (p != XOS_NULL) {
        p->r_pprev = &thread->r_next;
    }
    thread->wq_ptr = q;

    // Queue has at least one element in it now.
    XOS_ASSERT((q->head != XOS_NULL) && (q->tail != &(q->head)));
}


//-----------------------------------------------------------------------------
//  Move a thread from one priority queue to another, updating its priority
//  in the process. Also update the ready queues mask.
//  ASSUMES: critical section locked by caller, thread currently exists in
//  old_pri queue.
//-----------------------------------------------------------------------------
static void
xos_q_move(XosThread * thread, int8_t old_pri, int8_t new_pri)
{
    XosThreadQueue * queue;

    XOS_ASSERT(thread != XOS_NULL);
    DPRINTF("%s pri %d -> %d\n", thread->name, old_pri, new_pri);

    queue = &(xos_ready_threads[old_pri]);
    xos_q_remove(queue, thread);
    if (queue->head == XOS_NULL) {
        xos_ready_mask &= ~((uint32_t) 1U << (uint8_t) old_pri);
    }

    // Update the current priority.
    thread->curr_priority = new_pri;

    // Insert at head of new queue.
    queue = &(xos_ready_threads[new_pri]);
    xos_q_insert(queue, thread);
    xos_ready_mask |= ((uint32_t) 1U << (uint8_t) new_pri);
}


//-----------------------------------------------------------------------------
//  Thread creation/deletion/startup.
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//  Set up thread's save area for coprocessor and non-coprocessor TIE state.
//  This space is assumed to be allocated on the stack if "tie_save" points
//  to the base of the thread stack. If not, it is assumed to be allocated
//  separately. The "alloc_cp" flag determines whether space will be reserved
//  for saving coprocessor state. See xos_thread_create() for more.
//-----------------------------------------------------------------------------
// This function is required to do some messing about with pointers to set things up correctly.
// parasoft-begin-suppress MISRAC2012-RULE_10_1-e-2 MISRAC2012-RULE_10_4-a-2 "Constants from external files, usage checked"
// parasoft-begin-suppress MISRAC2012-RULE_11_5-a-4 MISRAC2012-RULE_11_6-a-2 "Pointer conversions necessary, checked"
void
xos_thread_setup_tie_save(XosThread * thread, void * tie_save, bool alloc_cp)
{
    uint32_t * p;
    uint32_t   ts;
    uint32_t   sa_align = (uint32_t) XCHAL_TOTAL_SA_ALIGN - 1U;

    // Reserve stack space if needed
    if (thread->stack_base == tie_save) {
        // TIE space needs to be reserved on the thread stack, 4 byte aligned
        ts = (xos_voidp_to_uint32(thread->stack_end) - (alloc_cp ? XT_CP_SIZE : XT_NCP_SIZE)) & ~0x3U;

        // Adjust the stack top
        thread->stack_end = xos_uint32_to_voidp((ts - 4U) & ~0xFU);
    }
    else {
        ts = ((uint32_t)tie_save + 3U) & ~0x3U;
    }

    thread->tie_save = (void *) ts;

    // Build the header. Keep this in sync with xos_common.h offsets.
    p = (uint32_t *) thread->tie_save;
    p[0] = 0;
    p[1] = 0;
    p[2] = ((ts + 16U + sa_align) & ~(sa_align));
    p[3] = alloc_cp ? p[2] : 0U;

    // This memory will never be read before it is written. So no need to
    // initialize it to known state.
}
// parasoft-end-suppress MISRAC2012-RULE_10_1-e-2 MISRAC2012-RULE_10_4-a-2
// parasoft-end-suppress MISRAC2012-RULE_11_5-a-4 MISRAC2012-RULE_11_6-a-2


//-----------------------------------------------------------------------------
//  Create a new thread.
//-----------------------------------------------------------------------------
int32_t
xos_thread_create(XosThread *     thread,
                  XosThread *     reserved,
                  XosThreadFunc * entry,
                  void *          arg,
                  const char *    name,
                  void *          stack,
                  uint32_t        stack_size,
                  int8_t          priority,
                  XosThreadParm * parms,
                  uint32_t        flags )
{
    uint32_t * p;
    uint32_t   i;
    uint32_t   ps;

    // Check parameters.

    UNUSED(reserved);

    if (thread == XOS_NULL) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    // Make sure system has been initialized.
    xos_init();

    // Skip some checks for fake threads (which are OS internal).
    if ((flags & XOS_THREAD_FAKE) == 0U) {
        if ((name == XOS_NULL) || (entry == XOS_NULL)) {
            return XOS_ERR_INVALID_PARAMETER;
        }

        if ((priority < 0) || (priority >= XOS_MAX_PRIORITY)) {
            return XOS_ERR_INVALID_PARAMETER;
        }
    }

    // parasoft-begin-suppress MISRAC2012-RULE_10_1-e-2 MISRAC2012-RULE_10_4-a-2 "Constants from external files, usage checked"

    // Min stack size checks
    if ((flags & XOS_THREAD_NO_CP) != 0U) {
        // Min stack size should hold nonCP state, one exc frame and a bit
        if ((stack != XOS_NULL) && (stack_size < (XOS_STACK_EXTRA_NO_CP + 32U))) {
            return XOS_ERR_STACK_TOO_SMALL;
        }
    }
    else {
        // Min stack size should hold CP state, one exc frame and a bit
        if ((stack != XOS_NULL) && (stack_size < (XOS_STACK_EXTRA + 32U))) {
            return XOS_ERR_STACK_TOO_SMALL;
        }
    }

    // parasoft-end-suppress MISRAC2012-RULE_10_1-e-2 MISRAC2012-RULE_10_4-a-2

    // Init the TCB structure.
    XOS_ASSERT((sizeof(XosThread) % 4U) == 0U);
    p = (uint32_t *) thread; // parasoft-suppress MISRAC2012-RULE_11_3-a-2 "Type conversion checked"
    for (i = 0; i < (sizeof(XosThread)/4U); i++) {
        p[i] = 0;
    }

    thread->sig   = XOS_THREAD_SIG;
    thread->entry = entry;
    thread->arg   = arg;

    // Save stack bounds for thread restart, bounds checking, debug display, etc.
    // Stack grows from the end. Align the end to 16-byte boundary per ABI.

    thread->stack_base = xos_uint32_to_voidp((xos_voidp_to_uint32(stack) + 3U) & ~0x3U);
    thread->stack_end  = xos_uint32_to_voidp((xos_voidp_to_uint32(stack) + stack_size) & ~0xFU);

    if ((flags & XOS_THREAD_FAKE) != 0U) {
        // Main or idle thread, no stack to be set up
#if XCHAL_HAVE_XEA2
        thread->resume_fn = &xos_resume_preempted_thread;  // for main thread; see xos_start() for idle thread
#endif
        thread->switch_fn = &xos_switch_thread;            // for main thread; n/a for idle thread
    }
    else {
        // Normal thread
        thread->resume_fn = &xos_resume_by_restart;
        thread->switch_fn = &xos_switch_thread;

        if (stack == XOS_NULL) {
            return XOS_ERR_INVALID_PARAMETER;
        }

        // Reserve CP/TIE save area out of the stack, if needed
        if ((flags & XOS_THREAD_NO_CP) != 0U) {
            xos_thread_setup_tie_save(thread, stack, false);
        }
        else {
            xos_thread_setup_tie_save(thread, stack, true);
        }

#if XOS_OPT_STACK_CHECK
        // Fill stack with a pattern to keep track of how much gets used
        {
            uint32_t * p1 = xos_voidp_to_uint32p(thread->stack_base);
            uint32_t * p2 = xos_voidp_to_uint32p(thread->stack_end);

            while (p1 < p2) {
                *p1 = XOS_STACK_CHECKVAL;
                p1++;
            }
        }
#endif

#if XCHAL_HAVE_XEA3
        // Create a fake frame on the stack to "return from" on start
        thread->esf = (XosFrame *) (xos_voidp_to_uint32(thread->stack_end) - sizeof(XosFrame)); // parasoft-suppress MISRAC2012-RULE_11_4-a-4 "Type conversion necessary"

        thread->esf->e.pc = (uint32_t) &xos_resume_by_restart; // parasoft-suppress MISRAC2012-RULE_11_1-a-2 "Type conversion necessary"
        thread->esf->e.ps = PS_STACK_FIRSTKER;
#endif
    }

    // switch_fn sets resume_fn as it needs to, then puts it back to the
    // generic preemptive version in case of pre-emption

    thread->base_priority = priority;
    thread->curr_priority = priority;
    thread->name          = name;
    thread->flags         = flags;

#if XOS_OPT_THREAD_SAFE_CLIB
    xos_clib_thread_init(thread);
#endif

    // Process optional parameters.
    if (parms != XOS_NULL) {
        if ((parms->parms_mask & XOS_TP_PREEMPT_PRI) != 0U) {
            // Currently unused
        }

        if ((flags & XOS_THREAD_NO_CP) != 0U) {
            thread->cp_mask = 0;
        }
        else {
            if ((parms->parms_mask & XOS_TP_COPROC_MASK) != 0U) {
                thread->cp_mask = parms->cp_mask;
            }
        }

        if ((parms->parms_mask & XOS_TP_EXIT_HANDLER) != 0U) {
            thread->exit_func = parms->handler;
        }
    }

    thread->exit_waiters.tail = &(thread->exit_waiters.head);

#if XOS_OPT_MUTEX_PRIORITY
    thread->owned_mtx_list = XOS_NULL;
#endif

    // Atomically insert thread at tail of list of all threads.
    ps = xos_critical_enter();
    *xos_all_threads_tail = thread;
    xos_all_threads_tail  = &thread->all_next;
    xos_critical_exit(ps);

    xos_log_sysevent(XOS_SE_THREAD_CREATE,
                     xos_threadp_to_uint32(thread),
                     (uint32_t) thread->base_priority);

    if ((flags & XOS_THREAD_SUSPEND) != 0U) {
        thread->block_cause = xos_blkon_suspend;
    }
    else {
        xos_thread_wake(thread, XOS_NULL, 0);
    }

    return XOS_OK;
}


//-----------------------------------------------------------------------------
//  Remove thread and free up all resources. Thread must have exited already.
//  Assume that thread exit / abort processing has dealt with all resources
//  owned by this thread.
//
//  A thread cannot call this on itself.
//-----------------------------------------------------------------------------
int32_t
xos_thread_delete(XosThread * thread)
{
    XosThread ** last;
    XosThread *  curr;
    uint32_t     ps;

    if ((thread == XOS_NULL) || (thread->sig != XOS_THREAD_SIG)) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    // Can't delete current thread
    if (thread == xos_curr_threadptr) {
        return XOS_ERR_ILLEGAL_OPERATION;
    }

    // This also covers the "thread is not blocked" case
    if (thread->block_cause != xos_blkon_exited) {
        return XOS_ERR_ILLEGAL_OPERATION;
    }

    ps = xos_critical_enter();

    // Remove from list of all threads
    last = &xos_all_threads;
    curr = *last;

    while (curr != XOS_NULL) {
        if (curr == thread) {
            *last = thread->all_next;
            if (&(thread->all_next) == xos_all_threads_tail) {
                xos_all_threads_tail = last;
            }
            break;
        }

        last = &(curr->all_next);
        curr = *last;
    }

#if XOS_OPT_THREAD_SAFE_CLIB
    xos_clib_thread_cleanup(thread);
#endif

    thread->sig = 0;

    xos_log_sysevent(XOS_SE_THREAD_DELETE,
                     xos_threadp_to_uint32(thread),
                     0U);

    xos_critical_exit(ps);
    return XOS_OK;
}


//-----------------------------------------------------------------------------
//  Initialize the state of the system. This is called internally as required.
//  Not part of user API.
//  Assumption: called with interrupts disabled.
//-----------------------------------------------------------------------------
void
xos_init(void)
{
    XosThreadQueue * q;
    int32_t          i;

    if (xos_init_flag == true) {
        return;
    }

    // Make sure preemption is disabled.
    (void) xos_preemption_disable();

#if XCHAL_HAVE_KSL
    // Disable kernel stack check. This will later be set per-thread.
    XT_WSR_KSL(0);
#endif
#if XCHAL_HAVE_ISL
    // Set the interrupt stack limit to be 128 bytes above the lower end of
    // the interrupt stack area. Before doing so, make sure we don't look
    // like we are running on the interrupt stack.
    extern char xos_int_stack;    // parasoft-suppress MISRAC2012-RULE_8_6-a-2 "Defined in assembly code"
    uint32_t r1 = PS_STACK_FIRSTKER;
    uint32_t r2 = PS_STACK_MASK;

    XT_XPS(r1, r2);
    XT_WSR_ISL((uint32_t)(&xos_int_stack + 128 + 15) & ~0xFU);
#endif

#if XCHAL_HAVE_XEA2
    // Setup XHREG for user exception handler.
    XOS_WSR_XHREG(xos_voidp_to_uint32((void *)&xos_dispatch_from_thread));
#endif

#if XOS_USE_SCHED_INT
    // Set up scheduler interrupt and handler.
    extern void xos_sched_handler(void * arg); // parasoft-suppress MISRAC2012-RULE_8_6-a-2 "Defined in assembly code"

    // Auto-select interrupt number if not already set.
    if (xos_sched_intr == 0xFFFFFFFFU) {
        for (i = 0; i < XCHAL_NUM_INTERRUPTS; i++) {
            if ((Xthal_inttype[i] == (uint8_t) XTHAL_INTTYPE_SOFTWARE) &&
                (!xos_interrupt_in_use((uint32_t)i))) {
                xos_sched_intr = (uint32_t) i;
                break;
            }
        }
    }

    if ((Xthal_inttype[xos_sched_intr] == (uint8_t) XTHAL_INTTYPE_SOFTWARE) &&
        (!xos_interrupt_in_use(xos_sched_intr))) {
        i = xos_register_interrupt_handler(xos_sched_intr, xos_sched_handler, XOS_NULL);
    }
    else {
        i = XOS_ERR_INVALID_PARAMETER;
    }

    if (i == XOS_OK) {
        xthal_interrupt_pri_set(xos_sched_intr, 1);
        (void) xos_interrupt_enable(xos_sched_intr);
    }
    else {
        xos_fatal_error(i, "Error setting up scheduler intr");
    }
#endif

#if XCHAL_HAVE_XEA3
#if (XCHAL_CP_NUM > 0)
    // Install coprocessor exception handler.
    extern XosExcHandlerFunc xos_coproc_exc_handler; // parasoft-suppress MISRAC2012-RULE_8_6-a-2 "Defined in assembly code"
    (void) xos_register_exception_handler(EXCCAUSE_CP_DISABLED, &xos_coproc_exc_handler);
#endif
    // Install debug exception handler.
    extern XosExcHandlerFunc xos_debug_exc_handler; // parasoft-suppress MISRAC2012-RULE_8_6-a-2 "Defined in assembly code"
    (void) xos_register_exception_handler(EXCCAUSE_DEBUG, &xos_debug_exc_handler);
#endif

#if XOS_OPT_SECMON
    // At this time SECMON support is XEA2 only.
    // Register alloca and coprocessor handlers with secure monitor.
#ifdef __XTENSA_WINDOWED_ABI__
    extern XosExcHandlerFunc _xos_alloca_handler;
    if (xos_register_exception_handler(EXCCAUSE_ALLOCA, _xos_alloca_handler) == XOS_NULL) {
        xos_fatal_error(XOS_ERR_SECMON_REJECT, "Secmon error setting up alloca handler");
    }
#endif
#if (XCHAL_CP_NUM > 0)
    extern XosExcHandlerFunc _xos_coproc_handler;
    for (i = 0; i < 8; i++) {
        if (((1U << i) & XCHAL_CP_MASK) != 0U) {
            if (xos_register_exception_handler(EXCCAUSE_CP0_DISABLED + i, _xos_coproc_handler) == XOS_NULL) {
                xos_fatal_error(XOS_ERR_SECMON_REJECT, "Secmon error setting up coproc handler");
            }
        }
    }
#endif
    // Pick up any interrupts already enabled by secure monitor.
    xos_intenable_mask |= XT_RSR_INTENABLE();
#endif

    // Preserve the state of any interrupts already enabled.
    // These are reflected in xos_intenable_mask, which will
    // not be initialized here. Note that xos_intlevel_mask
    // is already initialized to 0xFFFFFFFF.
#if XCHAL_HAVE_INTERRUPTS && XCHAL_HAVE_XEA2
    XOS_ASSERT(xos_intlevel_mask == 0xFFFFFFFFU);
#endif

    xos_num_ctx_switches = 0;

    // Setup all thread queues as empty.
    xos_all_threads      = XOS_NULL;
    xos_all_threads_tail = &xos_all_threads;
    xos_ready_mask       = 0;

    q = xos_ready_threads;
    for (i = 0; i < XOS_MAX_PRIORITY; i++) {
        q->head  = XOS_NULL;
        q->tail  = &q->head;
        q->flags = XOS_Q_WAIT_FIFO;
        q++;
    }

#if XOS_OPT_THREAD_SAFE_CLIB
    xos_clib_init();
#endif

    xos_once_init();
    xos_timer_module_init();

    xos_init_flag = true;
}


//-----------------------------------------------------------------------------
//  Return true if init completed.
//-----------------------------------------------------------------------------
bool
xos_init_done(void)
{
    return xos_init_flag;
}


//-----------------------------------------------------------------------------
//  Start XOS. The idle thread is created here. After that multitasking is 
//  enabled and control never returns to the caller.
//-----------------------------------------------------------------------------
void
xos_start(uint32_t flags)
{
    int32_t ret;

    UNUSED(flags);

    // Can't call this from interrupt context.
    if (INTERRUPT_CONTEXT) {
        xos_fatal_error(XOS_ERR_INTERRUPT_CONTEXT, XOS_NULL);
    }

    // Can't start more than once.
    if (xos_started() == true) {
        xos_fatal_error(XOS_ERR_ILLEGAL_OPERATION, XOS_NULL);
    }

    // Make sure init is done before creating any threads.
    xos_init();

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

    XOS_ASSERT(ret == XOS_OK);

    xos_idle_thread.resume_fn     = &xos_resume_idle_thread;
    xos_idle_thread.block_cause   = xos_blkon_idle;
    xos_idle_thread.curr_priority = -1;

    // Create timer thread as needed.
    ret = xos_create_timer_thread();
    XOS_ASSERT(ret == XOS_OK);

    // No current or next thread yet.
    xos_curr_threadptr = XOS_NULL;
    xos_next_threadptr = XOS_NULL;

    // Set the started flag
    xos_start_flag = true;

    // Don't call xos_preemption_enable() because we don't want the scheduler
    // to be called. Scheduler will try to save state for outgoing thread.
    // Instead we will explicitly start the first thread. This will also set
    // up the INTENABLE register properly with any pending enables.

    xos_scheduler_locked = 0U;

    if (xos_ready_mask != 0U) {
        // At least one thread is ready, find highest ready priority.
        int32_t          pri = find_msbit_pri(xos_ready_mask);
        XosThreadQueue * q   = &xos_ready_threads[pri];

        DPRINTF("mask=0x%08x pri=%d head=%p\n",
                (uint32_t)xos_ready_mask, (int32_t)pri, q->head);

        XOS_ASSERT(q->head != XOS_NULL);
        xos_start_thread(q->head);
    }

    // No ready thread found, run idle thread.
    xos_start_thread(&xos_idle_thread);

    // Must never return here.
    xos_fatal_error(XOS_ERR_INTERNAL_ERROR, XOS_NULL);
}


//-----------------------------------------------------------------------------
//  Return true if XOS started.
//-----------------------------------------------------------------------------
bool
xos_started(void)
{
    return xos_start_flag;
}


//-----------------------------------------------------------------------------
//  Switch to specified thread (from curr_thread to next_thread). To go idle,
//  next_thread must point to the idle thread's thread structure. Can be called
//  in thread context or in interrupt context. Note that no context switch will
//  happen if the scheduler is locked.
//
//  This is a low-level function.  Application threads and handlers should not
//  call it directly. OS primitives normally call this (e.g. after looking at
//  the thread ready queues to decide what thread to run next, etc).
//
//  NOTE: If called from thread context, must be called with interrupts
//  disabled. Also, must not be called if curr_thread == next_thread.
//-----------------------------------------------------------------------------
static inline int32_t
xos_switch_to(XosThread * curr_thread, XosThread * next_thread)
{
    int32_t ret = XOS_OK;

    XOS_ASSERT(curr_thread != next_thread);

    // Incoming thread must be ready to run - sanity check.
    if ((next_thread != XOS_NULL) && (next_thread != &xos_idle_thread)) {
        if (!next_thread->ready) {
            xos_fatal_error(XOS_ERR_INTERNAL_ERROR, "scheduler internal error");
        }
    }

    if (xos_scheduler_locked > 0U) {
#pragma frequency_hint NEVER
        return XOS_OK;
    }

    DPRINTF(" switch_to %s -> %s (%u)\n",
            (curr_thread != XOS_NULL) ? curr_thread->name : "<idle>",
            (next_thread != XOS_NULL) ? next_thread->name : "<idle>",
            xos_get_ccount());

    xos_log_sysevent(XOS_SE_THREAD_SWITCH,
                     xos_threadp_to_uint32(curr_thread),
                     xos_threadp_to_uint32(next_thread));

    // If we have the kernel stack limit check in hw, don't do sw check.
#if XOS_OPT_STACK_CHECK && (!XCHAL_HAVE_KSL || (defined __XTENSA_CALL0_ABI__))
    if ((!(INTERRUPT_CONTEXT)) && (curr_thread != XOS_NULL)) {
        void * check = (void *) &ret;

        if ((check < curr_thread->stack_base) || (check >= curr_thread->stack_end)) {
            DPRINTF("STACK OVERFLOW %s: %p outside %p -- %p\n",
                    curr_thread->name, check,
                    curr_thread->stack_base, curr_thread->stack_end);
            xos_fatal_error(XOS_ERR_STACK_TOO_SMALL, XOS_NULL);
        }
    }

    if ((next_thread != XOS_NULL) && (next_thread->esf != XOS_NULL)) {
        void * check = (void *) next_thread->esf;

        if ((check < next_thread->stack_base) || (check >= next_thread->stack_end)) {
            DPRINTF("STACK OVERFLOW %s: %p outside %p -- %p\n",
                    next_thread->name, next_thread->esf,
                    next_thread->stack_base, next_thread->stack_end);
            xos_fatal_error(XOS_ERR_STACK_TOO_SMALL, XOS_NULL);
        }
    }
#endif

#if XOS_OPT_UM
    if (next_thread->uthread != XOS_NULL) {
        XosUThread * uthread = next_thread->uthread;

        XT_WPTLB(uthread->mpu[0].at, uthread->mpu[0].as);
        XT_WPTLB(uthread->mpu[1].at, uthread->mpu[1].as);

        DPRINTF("  MPU -> 0x%08x,0x%08x\n", uthread->mpu[0].at, uthread->mpu[0].as);
        DPRINTF("  MPU -> 0x%08x,0x%08x\n", uthread->mpu[1].at, uthread->mpu[1].as);
    }
#endif

    // If in interrupt context then just remember where to switch to.
    // Actual switch done during interrupt return. Else switch now.
    if (INTERRUPT_CONTEXT) {
        xos_next_threadptr = next_thread;
#if XOS_USE_SCHED_INT
        if ((curr_thread != XOS_NULL) && (curr_thread != &xos_idle_thread)) {
            xthal_interrupt_trigger(xos_sched_intr);
        }
#endif
    }
    else {
        if (curr_thread != XOS_NULL) {
            ret = (*curr_thread->switch_fn)(next_thread);
        }
    }

    xos_num_ctx_switches++;
    return ret;
}


//-----------------------------------------------------------------------------
//  Run the highest priority ready thread, or go idle if no thread is ready.
//  Can be called both in thread context and in interrupt context.
//
//  curr_thread is a copy of xos_curr_threadptr (provided as parameter to allow
//  better optimizations by avoiding having to re-read the global variable)
//
//  MUST BE CALLED WITH INTERRUPTS DISABLED.
//-----------------------------------------------------------------------------
static int32_t
xos_schedule(XosThread * curr_thread)
{
    XOS_ASSERT(IN_CRITICAL_SECTION != 0);

    if (xos_ready_mask != 0U) {
        // At least one thread is ready, find highest ready priority
        int32_t          pri = find_msbit_pri(xos_ready_mask);
        XosThreadQueue * q   = &xos_ready_threads[pri];

        DPRINTF("mask=0x%08x pri=%d head=%p\n",
                (uint32_t)xos_ready_mask, (int32_t)pri, q->head);

        // If queue is not empty then queue head cannot be NULL
        if (q->head == XOS_NULL) {
            xos_fatal_error(XOS_ERR_INTERNAL_ERROR, XOS_NULL);
        }

        // If we are already running the highest priority thread then skip
        if (q->head == curr_thread) {
            return XOS_OK;
        }

        return xos_switch_to(curr_thread, q->head);
    }

    // No ready thread found, go idle
    return xos_switch_to(curr_thread, &xos_idle_thread);
}


//-----------------------------------------------------------------------------
//  If there are other threads ready at the same priority as the current thread
//  then put the current thread at the end of the queue and switch to the next
//  ready thread. Otherwise, do nothing.
//
//  Can be called in interrupt context to implement time-slicing.
//-----------------------------------------------------------------------------
void
xos_thread_yield(void)
{
    XosThread * next;
    XosThread * curr = xos_curr_threadptr;
    uint32_t    ps;

    // If the scheduler is locked, return immediately.
    if (xos_scheduler_locked > 0U) {
#pragma frequency_hint NEVER
        return;
    }

    ps = xos_critical_enter();

    // The current active thread (xos_curr_threadptr) is assumed to be at the
    // head of its ready queue. The following also works for the idle thread,
    // where r_next is always 0.

    next = curr->r_next;
    if (next != XOS_NULL) {
        XosThreadQueue * q = &xos_ready_threads[curr->curr_priority];

        // Move the current thread from the head of its queue to the tail.
        // Since this thread was running, this queue must be the highest
        // priority queue with ready threads in it.
        xos_q_remove(q, curr);
        xos_q_push(q, curr);

        DPRINTF((INTERRUPT_CONTEXT) ? " round-robin " : " yield ");
        xos_log_sysevent(XOS_SE_THREAD_YIELD,
                         xos_threadp_to_uint32(curr),
                         0U);

        (void) xos_schedule(curr);
    }

    // The current thread is the only one ready at its priority level, which
    // is the highest active priority. Resume it immediately.
    xos_critical_exit(ps);
}


//-----------------------------------------------------------------------------
//  Blocks the current active thread.
//
//  Currently, this can be called from an interrupt handler to block the thread
//  that was interrupted. Note that in interrupt context the current thread can
//  already be in the blocked state, due to a previous call to this function.
//  Can be called with interrupts enabled.
//
//  NOTE: This function will re-enable scheduling if the scheduler was locked.
//-----------------------------------------------------------------------------
int32_t
xos_block(const char *     block_cause,
          XosThreadQueue * block_queue,
          int32_t          must_schedule,
          int32_t          use_priority)
{
    XosThreadQueue * queue;
    XosThread *      curr = xos_curr_threadptr;
    int8_t           pri;
    int32_t          ret;
    uint32_t         ps;

    UNUSED(must_schedule);

    XOS_ASSERT((curr != XOS_NULL) && (curr->sig == XOS_THREAD_SIG));

    DPRINTF(" %s block for %s\n", curr->name, block_cause);

#if XCHAL_HAVE_INTERRUPTS
    // Can't block if already blocked - this is a fatal error
    if ((INTERRUPT_CONTEXT) && (!curr->ready)) {
        xos_fatal_error(XOS_ERR_THREAD_BLOCKED, XOS_NULL);
        return XOS_ERR_THREAD_BLOCKED;
    }
#endif

    ps = xos_critical_enter();

    // Check for valid priority before use
    if ((curr->curr_priority < 0) || (curr->curr_priority >= XOS_MAX_PRIORITY)) {
        xos_fatal_error(XOS_ERR_INTERNAL_ERROR, XOS_NULL);
        return XOS_ERR_INTERNAL_ERROR;
    }

    pri   = curr->curr_priority;
    queue = &(xos_ready_threads[pri]);
    xos_q_remove(queue, curr);
    curr->block_cause = block_cause;
    curr->ready = false;

    // If queue is now empty, clear this priority level in priority mask.
    if (queue->head == XOS_NULL) {
        xos_ready_mask &= ~(((uint32_t) 1U) << (uint8_t) pri);
    }

    // If a wait queue was provided then insert into it, either by priority
    // or in fifo order
    if (block_queue != XOS_NULL) {
        if (use_priority != 0) {
            xos_q_pri_insert(block_queue, curr);
        }
        else {
            xos_q_push(block_queue, curr);
        }
    }

    xos_log_sysevent(XOS_SE_THREAD_BLOCK,
                     xos_threadp_to_uint32(curr),
                     (uint32_t) block_cause); // parasoft-suppress MISRAC2012-RULE_11_4-a-4 "Type conversion for logging only"

    // Should be safe to do this as interrupts are disabled.
    if (xos_scheduler_locked > 0U) {
#pragma frequency_hint NEVER
        xos_scheduler_locked = 0U;
        DPRINTF("   scheduler enabled on block\n");
    }

    // Run scheduler to select next thread.
    ret = xos_schedule(curr);
    xos_critical_exit(ps);
    return ret;
}


//-----------------------------------------------------------------------------
//  Unblocks the specified blocked thread and puts it at the tail end of its
//  ready queue. Schedules it if it is higher priority than the current thread.
//  No effect if the thread is not blocked with the specified cause.
//  Can be called with interrupts enabled. Can be called in interrupt context.
//-----------------------------------------------------------------------------
void
xos_thread_wake(XosThread *  thread,
                const char * expected_cause,
                int32_t      wake_value)
{
    XosThreadQueue * queue;
    uint32_t         ps;

    XOS_ASSERT((thread != XOS_NULL) && (thread->sig == XOS_THREAD_SIG));
    XOS_ASSERT(thread != &xos_idle_thread);

    DPRINTF(" wake %s from %s\n",
            thread->name, (expected_cause != XOS_NULL) ? expected_cause : "(any cause)");

    // Must have interrupts disabled from here
    ps = xos_critical_enter();

    if (thread->ready) {
#pragma frequency_hint NEVER
        xos_critical_exit(ps);
        DPRINTF(" waking thread %s ALREADY READY\n", thread->name);
        return;
    }

    if ((expected_cause != XOS_NULL) && (thread->block_cause != expected_cause)) {
#pragma frequency_hint NEVER
        xos_critical_exit(ps);
        DPRINTF(" thread %s BLOCKED ON %s NOT %s\n",
                thread->name, thread->block_cause, expected_cause);
        return;
    }

    // Check for valid priority before use
    if ((thread->curr_priority < 0) || (thread->curr_priority >= XOS_MAX_PRIORITY)) {
        xos_fatal_error(XOS_ERR_INTERNAL_ERROR, XOS_NULL);
        return;
    }

    // Check if thread is on a wait queue, remove it if so.
    if (thread->wq_ptr != XOS_NULL) {
        xos_q_remove(thread->wq_ptr, thread);
    }

    // Insert thread at tail of its priority queue
    queue = &(xos_ready_threads[thread->curr_priority]);
    xos_q_push(queue, thread);

    // Update thread state
    thread->ready       = true;
    thread->block_cause = XOS_NULL;
    thread->wake_value  = wake_value;

    // Update ready mask for this thread's priority
    xos_ready_mask |= ((uint32_t) 1U << (uint8_t) thread->curr_priority);

    xos_log_sysevent(XOS_SE_THREAD_WAKE,
                     xos_threadp_to_uint32(thread),
                     (uint32_t) wake_value);

    // If scheduling is not disabled then run the scheduler. This woken
    // thread may be higher than the current, but is not necessarily the
    // highest priority ready thread.
    if (xos_scheduler_locked == 0U) {
        XosThread * curr = xos_curr_threadptr;

        XOS_ASSERT((curr != XOS_NULL) && (curr->sig == XOS_THREAD_SIG));
        (void) xos_schedule(curr);
    }

    xos_critical_exit(ps);
}


//-----------------------------------------------------------------------------
//  Change the priority of the specified thread. If thread is ready/running,
//  dynamically reschedule as needed.
//
//  Can be called with interrupts enabled. Can be called in interrupt context.
//-----------------------------------------------------------------------------
int32_t
xos_thread_set_priority(XosThread * thread, int8_t priority)
{
    XosThread * curr;
    int8_t      old_pri;
    uint32_t    ps;

    if ((thread == XOS_NULL) || (thread->sig != XOS_THREAD_SIG)) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    if ((priority < 0) || (priority >= XOS_MAX_PRIORITY)) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    ps = xos_critical_enter();

    if (thread->base_priority == priority) {
        xos_critical_exit(ps);
        return XOS_OK;
    }

    // Save thread current priority, we'll need it below.
    old_pri = thread->curr_priority;

    // Sanity check.
    XOS_ASSERT(thread->curr_priority >= thread->base_priority);

    // If curr_priority > base_priority, some kind of override is in effect,
    // so we'll update only the base_priority. Eventually, curr_priority will
    // be reset back to base_priority. However, if the new priority is higher
    // than both curr and base, then both need to be updated.

    if ((thread->curr_priority == thread->base_priority) ||
        (thread->curr_priority < priority)) {
        thread->curr_priority = priority;
    }
    thread->base_priority = priority;

    // If the current priority did not change, there's nothing more to do.
    if (old_pri == thread->curr_priority) {
        xos_critical_exit(ps);
        return XOS_OK;
    }

    xos_log_sysevent(XOS_SE_THREAD_PRI_CHANGE,
                     xos_threadp_to_uint32(thread),
                     (uint32_t) priority);

    // If thread is ready, move it to the proper ready queue and reschedule.
    if (thread->ready) {
        // Move to new priority queue (also updates priority mask).
        xos_q_move(thread, old_pri, thread->curr_priority);

        // Figure out who to run next.
        curr = xos_curr_threadptr;
        if (thread == curr) {
            // Thread is running, so if the priority has been raised we don't
            // have to do anything - it was already the highest priority ready
            // thread. If the priority has been lowered then run scheduler.
            if (thread->curr_priority < old_pri) {
                (void) xos_schedule(curr);
            }
        }
        else {
            // Not running, so if priority was lowered then no action. If the
            // priority was raised, then switch if higher than current.
            if (thread->curr_priority > curr->curr_priority) {
                (void) xos_schedule(curr);
            }
        }
    }
    else {
        if (thread->r_pprev != XOS_NULL) {
            XOS_ASSERT(thread->wq_ptr != XOS_NULL);

            if ((thread->wq_ptr->flags & XOS_Q_WAIT_PRIO) != 0U) {
                // Queue is prioritized not fifo, update queue. Make temporary
                // copy of queue ptr because the remove will clear the wq_ptr
                // field.
                XosThreadQueue * q = thread->wq_ptr;

                xos_q_remove(q, thread);
                xos_q_pri_insert(q, thread);
            }
        }
    }

    xos_critical_exit(ps);
    return XOS_OK;
}


//-----------------------------------------------------------------------------
//  Wake all threads on the specified queue, with the specified wake_value.
//  Can be called with interrupts enabled (see below).
//
//  NOTE: This function locks the scheduler, and does not unlock it when
//  it returns. The caller must unlock the scheduler and cause a scheduling
//  operation to happen so that higher priority woken threads get to run.
//  This is better than forcing this function to be called with interrupts
//  disabled. This way, we get to have interrupts disabled for less time.
//-----------------------------------------------------------------------------
int32_t
xos_wake_queue(XosThreadQueue * queue,
               const char *     expected_cause,
               int32_t          wake_value)
{
    XosThread * waiter;
    int32_t     num_woken = 0;
    uint32_t    ps;

    XOS_ASSERT(queue != XOS_NULL);

    // The queue we are processing may not be prioritized, so we might wake
    // a thread that is higher priority than the caller which takes over
    // right away, leaving an even higher priority thread on the queue.
    // To prevent this, we disable the scheduler while processing the queue.
    // This also allows us to process the whole queue without being kicked
    // off the cpu, and the caller can do any cleanup it needs to (e.g. if
    // processing thread exit) before yielding the cpu.

    (void) xos_preemption_disable();

    while (1) {
        ps = xos_critical_enter();
        waiter = xos_q_pop(queue);
        xos_critical_exit(ps);
        if (waiter == XOS_NULL) {
            break;
        }
        xos_thread_wake(waiter, expected_cause, wake_value);
        num_woken++;
    }

    // Note we are returning with the scheduler disabled.
    return num_woken;
}


//-----------------------------------------------------------------------------
//  Install exit handler for specified thread.
//-----------------------------------------------------------------------------
int32_t
xos_thread_set_exit_handler(XosThread * thread, XosThdExitFunc * func)
{
    if ((thread == XOS_NULL) || (thread->sig != XOS_THREAD_SIG)) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    // Make sure thread hasn't exited already.
    if (thread->block_cause == xos_blkon_exited) {
        return XOS_ERR_THREAD_EXITED;
    }

    thread->exit_func = func;
    return XOS_OK;
}


//-----------------------------------------------------------------------------
//  Exit the current thread. The exit code is passed to waiters that called
//  xos_thread_join(). This function also gets called by xos_thread_exit_trap
//  asm code if a thread exits by returning from its entry function.
//  Can only be called in a thread context, not from interrupt code.
//-----------------------------------------------------------------------------
void
xos_thread_exit(int32_t exitcode)
{
    XosThread * thread = xos_curr_threadptr;
    int32_t     xcode  = exitcode;

    // Can't call this from interrupt context.
    if (INTERRUPT_CONTEXT) {
        xos_fatal_error(XOS_ERR_INTERRUPT_CONTEXT, XOS_NULL);
    }

    // Set flag to indicate we are in exit processing.
    thread->in_exit = 1;

    // Run exit handler if any. This needs to happen before any waiting
    // threads are woken.
    if (thread->exit_func != XOS_NULL) {
        xcode = (*thread->exit_func)(xcode);
    }

#if XOS_OPT_MUTEX_PRIORITY
    // At this point the thread should not own any mutexes.
    if (thread->owned_mtx_list != XOS_NULL) {
        xos_fatal_error(XOS_ERR_THREAD_EXIT_WITH_LOCK, XOS_NULL);
    }
#endif

#if XOS_OPT_THREAD_LOCAL_STORAGE
    // Run TLS cleanup.
    xos_tls_cleanup();
#endif

#if XCHAL_CP_NUM > 0
    // Release any owned coprocessors.
    if (thread->tie_save != XOS_NULL) {
        extern void * xos_coproc_owner_sa[XCHAL_CP_MAX];  // parasoft-suppress MISRAC2012-RULE_8_6-a-2 "Defined in assembly code"
        int32_t i;

        for (i = 0; i < XCHAL_CP_MAX; i++) {
            if (xos_coproc_owner_sa[i] == thread->tie_save) {
                xos_coproc_owner_sa[i] = XOS_NULL;
            }
        }
    }
#endif

    // Joiners may need this so save it away.
    thread->wake_value = xcode;

    // Wake all threads waiting for this one to exit.
    (void) xos_wake_queue(&(thread->exit_waiters), xos_blkon_join, xcode);

    // xos_wake_queue() returned with scheduling blocked, but now this call
    // will enable it.
    (void) xos_block(xos_blkon_exited, XOS_NULL, 1, 0);

    // Should not return here, so assert.
    xos_fatal_error(XOS_ERR_INTERNAL_ERROR, XOS_NULL);
}


//-----------------------------------------------------------------------------
//  Wait until the specified thread exits if it hasn't already exited.
//  If p_exitcode is not NULL, return the thread's exit code in *p_exitcode.
//  (To cancel the join, call xos_thread_wake(thread, xos_blkon_join, <errcode>)
//  from a separate thread or timer event routine.)
//  Cannot be called from interrupt context.
//-----------------------------------------------------------------------------
int32_t
xos_thread_join(XosThread * thread, int32_t * p_exitcode)
{
    int32_t  exitcode;
    uint32_t ps;

    // Can't call this from interrupt context.
    if (INTERRUPT_CONTEXT) {
        xos_fatal_error(XOS_ERR_INTERRUPT_CONTEXT, XOS_NULL);
    }

    if ((thread == XOS_NULL) || (thread->sig != XOS_THREAD_SIG)) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    if (thread == xos_curr_threadptr) {
        return XOS_ERR_ILLEGAL_OPERATION;
    }

    ps = xos_critical_enter();

    if (thread->ready || (thread->block_cause != xos_blkon_exited)) {
        // Not yet exited.  Queue ourselves on its exit_waiters queue.
        exitcode = xos_block(xos_blkon_join, &(thread->exit_waiters), 0, 1);
    }
    else {
        // Already exited, just grab the exit code.
        exitcode = thread->wake_value;
    }

    xos_critical_exit(ps);

    if (p_exitcode != XOS_NULL) {
        *p_exitcode = exitcode;
    }

    return XOS_OK;
}


//-----------------------------------------------------------------------------
// Suspend thread.
// Can't call xos_block() because that only works on the current thread.
//-----------------------------------------------------------------------------
int32_t
xos_thread_suspend(XosThread * thread)
{
    XosThreadQueue * queue;
    uint32_t         ps;
    int32_t          ret;

    if ((thread == XOS_NULL) || (thread->sig != XOS_THREAD_SIG)) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    DPRINTF("Suspending thread %s\n", thread->name);
    ps = xos_critical_enter();

    // Can't suspend if already blocked
    if (!thread->ready) {
        xos_critical_exit(ps);
        return XOS_ERR_THREAD_BLOCKED;
    }

    // Check for valid priority before use
    if ((thread->curr_priority < 0) || (thread->curr_priority >= XOS_MAX_PRIORITY)) {
        xos_fatal_error(XOS_ERR_INTERNAL_ERROR, XOS_NULL);
        return XOS_ERR_INTERNAL_ERROR;
    }

    // Remove from ready queue and mark as suspended
    queue = &(xos_ready_threads[thread->curr_priority]);
    xos_q_remove(queue, thread);
    thread->block_cause = xos_blkon_suspend;
    thread->ready = false;

    // Update priority mask if no other ready threads at this priority.
    if (queue->head == XOS_NULL) {
        xos_ready_mask &= ~(((uint32_t) 1U) << (uint8_t) thread->curr_priority);
    }

    xos_log_sysevent(XOS_SE_THREAD_SUSPEND,
                     xos_threadp_to_uint32(thread),
                     0U);

    // If we just suspended the current thread or the incoming thread
    // then run the scheduler.
    if ((thread == xos_curr_threadptr) || (thread == xos_next_threadptr)) {
        ret = xos_schedule(thread);
    }
    else {
        ret = XOS_OK;
    }

    xos_critical_exit(ps);
    return ret;
}


//-----------------------------------------------------------------------------
// Resume thread.
//-----------------------------------------------------------------------------
int32_t
xos_thread_resume(XosThread * thread)
{
    if ((thread == XOS_NULL) || (thread->sig != XOS_THREAD_SIG)) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    // If thread not suspended do nothing
    if (thread->block_cause != xos_blkon_suspend) {
        return XOS_OK;
    }

    DPRINTF("Resuming thread %s\n", thread->name);

    // Wake thread and put in ready queue, reschedule if needed
    xos_thread_wake(thread, xos_blkon_suspend, 0);
    return XOS_OK;
}


//-----------------------------------------------------------------------------
//  Kill a thread. Note that the thread is NOT guaranteed to have terminated
//  when this function returns. Exit processing will have to happen first.
//-----------------------------------------------------------------------------
int32_t
xos_thread_abort(XosThread * thread, int32_t exitcode)
{
    uint32_t ps;

    if ((thread == XOS_NULL) || (thread->sig != XOS_THREAD_SIG)) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    if (thread == xos_curr_threadptr) {
        return XOS_ERR_ILLEGAL_OPERATION;
    }

    ps = xos_critical_enter();
    (void) xos_preemption_disable();

    // Check for exited must be done inside critical section.
    if (((!thread->ready) && (thread->block_cause == XOS_NULL)) ||
        (thread->block_cause == xos_blkon_exited)) {
        // Can't do anything.
        (void) xos_preemption_enable();
        xos_critical_exit(ps);
        return XOS_ERR_THREAD_EXITED;
    }

    // Check if thread is already in exit processing.
    if (thread->in_exit) {
        // Can't do anything.
        (void) xos_preemption_enable();
        xos_critical_exit(ps);
        return XOS_OK;
    }

    if (thread->ready) {
        // Already ready, set wake value
        thread->wake_value = exitcode;
    }
    else if (thread->wq_ptr != XOS_NULL) {
        // Remove thread from wait queue and make ready.
        xos_q_remove(thread->wq_ptr, thread);
        xos_thread_wake(thread, XOS_NULL, exitcode);
    }
    else if (thread->block_cause == xos_blkon_suspend) {
        (void) xos_thread_resume(thread);
        thread->wake_value = exitcode;
    }
    else {
        // Should never happen
        xos_fatal_error(XOS_ERR_INTERNAL_ERROR, XOS_NULL);
    }

    // Set the resumption PC to the abort handler assembly stub.
    // parasoft-begin-suppress MISRAC2012-RULE_11_1-a-2 "Type conversion necessary here"
#if XCHAL_HAVE_XEA2
    if (thread->esf == XOS_NULL) {
        // No esf, most likely thread hasn't started running yet.
    }
    else if (thread->resume_fn == &xos_resume_cooperative_thread) {
#ifdef __XTENSA_WINDOWED_ABI__
        // Copy bits 31:30 from saved a0 in preparation for windowed return.
        uint32_t a0 = thread->esf->c.a0;
        thread->esf->c.a0 = (a0 & XOS_THREAD_CALL_MASK) | 
            ((uint32_t)&xos_thread_abort_stub & ~XOS_THREAD_CALL_MASK);
#else
        thread->esf->c.a0 = (uint32_t) &xos_thread_abort_stub;
#endif
    }
    else {
        thread->esf->e.pc = (uint32_t) &xos_thread_abort_stub;
    }
#elif XCHAL_HAVE_XEA3
    XOS_ASSERT(thread->esf != XOS_NULL);
    thread->esf->e.pc = (uint32_t) &xos_thread_abort_stub;
#endif
    // parasoft-end-suppress MISRAC2012-RULE_11_1-a-2

    // Set the exit flag for the thread startup code, in case
    // the startup code hasn't run yet.
    thread->in_exit = 1;

    xos_log_sysevent(XOS_SE_THREAD_ABORT,
                     xos_threadp_to_uint32(thread),
                     (uint32_t) exitcode);

    (void) xos_preemption_enable();
    xos_critical_exit(ps);

    return XOS_OK;
}


//-----------------------------------------------------------------------------
//  Return thread ID.
//-----------------------------------------------------------------------------
XosThread *
xos_thread_id(void)
{
    return xos_curr_threadptr;
}


//-----------------------------------------------------------------------------
//  Return thread state.
//-----------------------------------------------------------------------------
xos_thread_state_t
xos_thread_get_state(const XosThread * thread)
{
    if ((thread != XOS_NULL) && (thread->sig == XOS_THREAD_SIG)) {
        if (thread->ready) {
            if (thread == xos_curr_threadptr) {
                return XOS_THREAD_STATE_RUNNING;
            }
            return XOS_THREAD_STATE_READY;
        }
        if (thread->block_cause == XOS_NULL) {
            return XOS_THREAD_STATE_INVALID;
        }
        if (thread->block_cause == xos_blkon_exited) {
            return XOS_THREAD_STATE_EXITED;
        }
        return XOS_THREAD_STATE_BLOCKED;
    }

    return XOS_THREAD_STATE_INVALID;
}


//-----------------------------------------------------------------------------
// Return thread priority.
//-----------------------------------------------------------------------------
int8_t
xos_thread_get_priority(const XosThread * thread)
{
    if ((thread != XOS_NULL) && (thread->sig == XOS_THREAD_SIG)) {
        return thread->curr_priority;
    }

    return -1;
}


//-----------------------------------------------------------------------------
// Return thread name.
//-----------------------------------------------------------------------------
const char *
xos_thread_get_name(const XosThread * thread)
{
    if ((thread != XOS_NULL) && (thread->sig == XOS_THREAD_SIG)) {
        return thread->name;
    }

    return XOS_NULL;
}


//-----------------------------------------------------------------------------
// Set thread name.
//-----------------------------------------------------------------------------
int32_t
xos_thread_set_name(XosThread * thread, const char * name)
{
    if ((thread != XOS_NULL) && (thread->sig == XOS_THREAD_SIG)) {
        thread->name = name;
        return XOS_OK;
    }

    return XOS_ERR_INVALID_PARAMETER;
}


//-----------------------------------------------------------------------------
// Return coprocessor mask.
//-----------------------------------------------------------------------------
uint16_t
xos_thread_cp_mask(const XosThread * thread)
{
    if ((thread != XOS_NULL) && (thread->sig == XOS_THREAD_SIG)) {
        return thread->cp_mask;
    }

    return 0U;
}


//-----------------------------------------------------------------------------
// Return wake value. This is also the exit code after thread exits.
//-----------------------------------------------------------------------------
int32_t
xos_thread_get_wake_value(const XosThread * thread)
{
    if ((thread != XOS_NULL) && (thread->sig == XOS_THREAD_SIG)) {
        return thread->wake_value;
    }

    return 0;
}


//-----------------------------------------------------------------------------
// Return event bits.
//-----------------------------------------------------------------------------
uint32_t
xos_thread_get_event_bits(void)
{
    XosThread * thread = xos_thread_id();

    if ((thread != XOS_NULL) && (thread->sig == XOS_THREAD_SIG)) {
        return thread->event_bits;
    }

    return 0U;
}


//-----------------------------------------------------------------------------
// Disable preemption.
//-----------------------------------------------------------------------------
uint32_t
xos_preemption_disable(void)
{
    uint32_t ps;

    ps = xos_critical_enter();

    xos_scheduler_locked++;

    xos_critical_exit(ps);
    return xos_scheduler_locked;
}


//-----------------------------------------------------------------------------
// Enable preemption. Call the scheduler to see if a context switch is needed.
//-----------------------------------------------------------------------------
uint32_t
xos_preemption_enable(void)
{
    uint32_t ps;

    ps = xos_critical_enter();

    if (xos_scheduler_locked > 0U) {
        xos_scheduler_locked--;

        if (xos_scheduler_locked == 0U) {
            if (xos_started() == true) {
                (void) xos_schedule(xos_curr_threadptr);
            }
        }
    }

    xos_critical_exit(ps);
    return xos_scheduler_locked;
}


//-----------------------------------------------------------------------------
// Enable preemption but do NOT invoke the scheduler.
//-----------------------------------------------------------------------------
uint32_t
xos_preemption_enable_ns(void)
{
    uint32_t ps;
    uint32_t ret;

    ps = xos_critical_enter();

    if (xos_scheduler_locked > 0U) {
        xos_scheduler_locked--;
    }
    ret = xos_scheduler_locked;

    xos_critical_exit(ps);
    return ret;
}


#if XOS_OPT_MUTEX_PRIORITY

//-----------------------------------------------------------------------------
// Add mutex to owned list, check and update priority as needed.
//-----------------------------------------------------------------------------
void
xos_thread_mutex_add(XosThread * owner, struct XosMutex * mutex)
{
    XosMutex * pm = XOS_NULL;
    XosMutex * cm;

    XOS_ASSERT(owner != XOS_NULL);
    XOS_ASSERT(mutex != XOS_NULL);
    XOS_ASSERT(mutex->next == XOS_NULL);

    if (owner == XOS_THREAD_SYSTEM) {
        return;
    }

    // Assign the mutex's current priority.
    if ((mutex->flags & XOS_MUTEX_PRIO_PROTECT) != 0U) {
        mutex->curr_pri = mutex->priority;
    }
    else {
        mutex->curr_pri = 0;
    }

    // Insert into thread's owned list in priority order.
    cm = owner->owned_mtx_list;

    while ((cm != XOS_NULL) && (cm->curr_pri > mutex->curr_pri)) {
        pm = cm;
        cm = cm->next;
    }
    if (pm != XOS_NULL) {
        pm->next = mutex;
        mutex->next = cm;
    }
    else {
        owner->owned_mtx_list = mutex;
        mutex->next = cm;
    }

    // Now examine the highest priority mutex (which must be at the head)
    cm = owner->owned_mtx_list;

    if (cm->curr_pri > owner->curr_priority) {
        // We are raising the priority of the current running thread, so no
        // need to reschedule. Just move the thread to its new priority queue.
        xos_q_move(owner, owner->curr_priority, cm->curr_pri);
    }
}


//-----------------------------------------------------------------------------
// Remove mutex from owned list, check and update priority as needed.
//-----------------------------------------------------------------------------
void
xos_thread_mutex_rem(XosThread * owner, struct XosMutex * mutex)
{
    XosMutex * pm = XOS_NULL;
    XosMutex * cm;
    int8_t     old_pri;
    int8_t     new_pri;

    XOS_ASSERT(owner != XOS_NULL);
    XOS_ASSERT(mutex != XOS_NULL);

    if (owner == XOS_THREAD_SYSTEM) {
        return;
    }

    // Remove mutex from owned list.
    cm = owner->owned_mtx_list;

    while ((cm != mutex) && (cm != XOS_NULL)) {
        pm = cm;
        cm = cm->next;
    }

    if (cm != XOS_NULL) {
        XOS_ASSERT(cm == mutex);
        if (pm != XOS_NULL) {
            pm->next = cm->next;
        }
        else {
            owner->owned_mtx_list = cm->next;
        }
        mutex->next = XOS_NULL;
    }
    else {
        xos_fatal_error(XOS_ERR_INTERNAL_ERROR, XOS_NULL);
    }

    // Reset mutex current priority.
    mutex->curr_pri = 0;

    // Now decide if the thread priority needs to be changed.
    old_pri = owner->curr_priority;
    new_pri = owner->base_priority;

    if (owner->owned_mtx_list != XOS_NULL) {
        // The mutex at the head of the list has the highest priority.
        if (owner->owned_mtx_list->curr_pri > new_pri) {
            new_pri = owner->owned_mtx_list->curr_pri;
        }
    }

    // There is no way that the new priority can be higher.
    XOS_ASSERT(new_pri <= old_pri);

    if (new_pri < old_pri) {
        // Move thread to new priority queue (also updates priority mask).
        xos_q_move(owner, old_pri, new_pri);
        // There may be threads waiting on the mutex that need waking up
        // before rescheduling. Do not run the scheduler here.
    }
}


//-----------------------------------------------------------------------------
// Adjust priority of owned mutex and owner thread.
//-----------------------------------------------------------------------------
void
xos_thread_mutex_bump(XosThread * owner, struct XosMutex * mutex, int8_t pri)
{
    XosMutex * pm = XOS_NULL;
    XosMutex * cm;

    XOS_ASSERT(owner != XOS_NULL);
    XOS_ASSERT(mutex != XOS_NULL);
    XOS_ASSERT(owner != XOS_THREAD_SELF);

    // Remove mutex from owned list.
    cm = owner->owned_mtx_list;

    while ((cm != mutex) && (cm != XOS_NULL)) {
        pm = cm;
        cm = cm->next;
    }

    if (cm != XOS_NULL) {
        XOS_ASSERT(cm == mutex);
        if (pm != XOS_NULL) {
            pm->next = cm->next;
        }
        else {
            owner->owned_mtx_list = cm->next;
        }
        mutex->next = XOS_NULL;
    }
    else {
        xos_fatal_error(XOS_ERR_INTERNAL_ERROR, XOS_NULL);
    }

    // Update mutex current priority.
    XOS_ASSERT(mutex->curr_pri < pri);
    mutex->curr_pri = pri;

    // Re-insert into thread's owned list in priority order.
    pm = XOS_NULL;
    cm = owner->owned_mtx_list;

    while ((cm != XOS_NULL) && (cm->curr_pri > mutex->curr_pri)) {
        pm = cm;
        cm = cm->next;
    }
    if (pm != XOS_NULL) {
        pm->next = mutex;
        mutex->next = cm;
    }
    else {
        owner->owned_mtx_list = mutex;
        mutex->next = cm;
    }

    // Now examine the highest priority mutex (which must be the one we
    // just inserted).
    XOS_ASSERT(owner->owned_mtx_list == mutex);
    cm = owner->owned_mtx_list;

    // We must be raising the priority or else we wouldn't be here.
    XOS_ASSERT(cm->curr_pri > owner->curr_priority);

    xos_log_sysevent(XOS_SE_THREAD_PRI_CHANGE,
                     xos_threadp_to_uint32(owner),
                     (uint32_t) cm->curr_pri);

    // If thread is ready, move it to the proper ready queue.
    if (owner->ready) {
        // No need to reschedule here, caller will block and invoke scheduler.
        xos_q_move(owner, owner->curr_priority, cm->curr_pri);
    }
    else {
        owner->curr_priority = cm->curr_pri;

        if (owner->r_pprev != XOS_NULL) {
            XOS_ASSERT(owner->wq_ptr != XOS_NULL);

            if ((owner->wq_ptr->flags & XOS_Q_WAIT_PRIO) != 0U) {
                // Queue is prioritized not fifo, update queue. Make temporary
                // copy of queue ptr because the remove will clear the wq_ptr
                // field.
                XosThreadQueue * q = owner->wq_ptr;

                xos_q_remove(q, owner);
                xos_q_pri_insert(q, owner);
            }
        }
    }
}

#endif // XOS_OPT_MUTEX_PRIORITY


//-----------------------------------------------------------------------------
// Set scheduler interrupt number.
//-----------------------------------------------------------------------------
int32_t
xos_set_sched_interrupt(int32_t intnum)
{
#if XOS_USE_SCHED_INT
    xos_sched_intr = (uint32_t) intnum;
#else
    UNUSED(intnum);
#endif
    return XOS_OK;
}


//-----------------------------------------------------------------------------
// User-mode support functions.
//-----------------------------------------------------------------------------

#if XOS_OPT_UM

//-----------------------------------------------------------------------------
// Create user-mode thread.
//-----------------------------------------------------------------------------
int32_t
xos_uthread_create_p(XosUThread *    uthread,
                     XosThreadFunc * entry,
                     void *          arg,
                     void *          res1,
                     void *          res2,
                     const char *    name,
                     void *          stack,
                     uint32_t        stack_size,
                     int8_t          priority,
                     uint32_t        flags,
                     void *          pmptr,
                     uint32_t        pmsize,
                     uint32_t        mpu_entry)
{
    int32_t  ret;
    uint32_t lflags;

    UNUSED(res1);
    UNUSED(res2);

    if (uthread == XOS_NULL) {
        return XOS_ERR_INVALID_PARAMETER;
    }
    if ((pmptr != XOS_NULL) && (pmsize == 0U)) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    // pmptr and pmsize must be MPU aligned, if a memory region has
    // been specified.
    if (pmptr != XOS_NULL) {
        if ((((uint32_t)pmptr & (XCHAL_MPU_ALIGN - 1)) != 0U) ||
           ((pmsize & (XCHAL_MPU_ALIGN - 1)) != 0U)) {
            return XOS_ERR_INVALID_PARAMETER;
        }
    }

    lflags = flags | XOS_THREAD_USERMODE;

    // We must create the thread and then set some of its properties
    // before it runs for the first time. Disable scheduling to ensure.
    xos_preemption_disable();

    // See if thread is created OK. Return any errors.
    ret = xos_thread_create(&(uthread->tcb),
                            XOS_NULL,
                            entry,
                            arg,
                            name,
                            stack,
                            stack_size,
                            priority,
                            XOS_NULL,
                            lflags);
    if (ret != XOS_OK) {
        xos_preemption_enable();
        return ret;
    }

    // Set back pointer in TCB.
    uthread->tcb.uthread = uthread;

    // Set up MPU entries for bounds. If not specified, and the calling
    // thread is a user thread, then copy settings from it.

    if (pmptr != XOS_NULL) {
        uthread->mpu[0] = (xthal_MPU_entry)
            XTHAL_MPU_ENTRY((uint32_t)pmptr, 1, XTHAL_AR_RWXrwx, XTHAL_MEM_WRITEBACK);
        uthread->mpu[1] = (xthal_MPU_entry)
            XTHAL_MPU_ENTRY((uint32_t)pmptr + pmsize, 1, XTHAL_AR_RWX, XTHAL_MEM_WRITEBACK);

        uthread->mpu[0].at |= mpu_entry;
        uthread->mpu[1].at |= (mpu_entry + 1);
    }
    else {
        XosThread * curr = xos_curr_threadptr;

        if ((curr != XOS_NULL) && (curr->uthread != XOS_NULL)) {
            uthread->mpu[0] = curr->uthread->mpu[0];
            uthread->mpu[1] = curr->uthread->mpu[1];
        }
        else {
            uthread->mpu[0] = (xthal_MPU_entry){0, 0};
            uthread->mpu[1] = (xthal_MPU_entry){0, 0};
        }
    }

    xos_preemption_enable();
    return XOS_OK;
}


//-----------------------------------------------------------------------------
// Delete user-mode thread.
//-----------------------------------------------------------------------------
int32_t
xos_uthread_delete_p(XosUThread * uthread)
{
    int32_t ret;

    if (uthread == XOS_NULL) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    ret = xos_thread_delete(&(uthread->tcb));
    if (ret == XOS_OK) {
        uthread->mpu[0] = (xthal_MPU_entry){0, 0};
        uthread->mpu[1] = (xthal_MPU_entry){0, 0};
    }

    return ret;
}


//-----------------------------------------------------------------------------
// Return thread pointer for user-mode thread.
//-----------------------------------------------------------------------------
XosThread *
xos_uthread_thread(XosUThread * uthread)
{
    return (uthread == XOS_NULL) ? XOS_NULL : &(uthread->tcb);
}


#endif // XOS_OPT_UM

