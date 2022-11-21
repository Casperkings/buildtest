/** @file */

// xos_thread.h - XOS Thread API interface and data structures.

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

// NOTE: Do not include this file directly in your application. Including
// xos.h will automatically include this file.


#ifndef XOS_THREAD_H
#define XOS_THREAD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xos_types.h"
#include "xos_params.h"


//-----------------------------------------------------------------------------
//  External references.
//-----------------------------------------------------------------------------
struct XosMutex;


//-----------------------------------------------------------------------------
//  Number of thread priority levels.
//-----------------------------------------------------------------------------
#ifndef XOS_NUM_PRIORITY
#error "XOS_NUM_PRIORITY must be defined (in xos_params.h)."
#endif
#if XOS_NUM_PRIORITY > 32
#error "The number of thread priority levels (XOS_NUM_PRIORITY) must be <= 32."
#endif
#define XOS_MAX_PRIORITY        (XOS_NUM_PRIORITY)


//-----------------------------------------------------------------------------
//  Macro for thread self pointer.
//-----------------------------------------------------------------------------
#define XOS_THREAD_SELF         (xos_thread_id())


//-----------------------------------------------------------------------------
///
///  Thread entry function pointer type.
///
//-----------------------------------------------------------------------------
typedef int32_t (XosThreadFunc)(void * arg, int32_t wake_value);


//-----------------------------------------------------------------------------
//  Thread switcher function signature.
//-----------------------------------------------------------------------------
typedef struct XosThread XosThread;
typedef struct XosUThread XosUThread;
typedef int32_t (XosSwitchFunc)(XosThread * thread);


//-----------------------------------------------------------------------------
///
///  Condition evaluation callback function pointer type.
///
//-----------------------------------------------------------------------------
typedef int32_t (XosCondFunc)(void * arg, int32_t sig_value, XosThread * thread);


//-----------------------------------------------------------------------------
///
///  Thread exit handler function pointer type.
///
//-----------------------------------------------------------------------------
typedef int32_t (XosThdExitFunc)(int32_t exitcode);


//-----------------------------------------------------------------------------
//  Thread queue structure. Used to implement the ready queues as well
//  as the wait queues.
//-----------------------------------------------------------------------------
typedef struct XosThreadQueue {
  XosThread *  head;    // Pointer to first thread in queue, or 0 if none.
  XosThread ** tail;    // Pointer to last thread's r_next pointer, or
                        // to "head" if none.
  uint16_t     flags;   // Queue properties.
  uint16_t     reserved;
} XosThreadQueue;


//-----------------------------------------------------------------------------
//  Thread queue flags. Some flags may be combined, others are exclusive.
//-----------------------------------------------------------------------------
#define XOS_Q_WAIT_PRIO     0x0001U
#define XOS_Q_WAIT_FIFO     0x0002U


//-----------------------------------------------------------------------------
//  Stack frame for a thread that is not running. That is, it has either
//  been preempted or has yielded.
//-----------------------------------------------------------------------------
#if XCHAL_HAVE_XEA2
typedef union XosFrame {
  XosExcFrame   e;      // resume_fn == &xos_resume_preempted_thread
  XosCoopFrame  c;      // resume_fn == &xos_resume_cooperative_thread
  // nothing for resume_fn == &xos_resume_idle_thread
  // nothing for resume_fn == &xos_resume_by_restart
} XosFrame;
#else
typedef union XosFrame {
  XosExcFrame   e;
} XosFrame;
#endif


//-----------------------------------------------------------------------------
//  Thread Control Block. Tracks the state and control information associated
//  with a thread.
//
//  IMPORTANT:  keep this in sync with TCB_*** offsets in xos_common.h .
//-----------------------------------------------------------------------------
struct XosThread {
  XosThread *   r_next;         // 00 Next thread in queue (eg. ready queue of
                                // its priority, or some queue of blocked threads)
                                // Should be NULL if not in any queue.

  XosThread **  r_pprev;        // 04 Points to previous queue entry's r_next
                                // pointer (i.e. to itself), or to queue head
                                // if first in queue. NULL if not in any queue.

  XosThread *   all_next;       // 08 Next in list of all threads.
  
  void (*resume_fn)(void);      // 12 Pointer to the routine to be called to
                                // resume this thread. On entry to such code:
                                //   a2 == xos_curr_threadptr (thread being resumed)
                                //   a3 == &xos_globals

  XosFrame *    esf;            // 16 Pointer to saved exception stack frame,
                                // just below thread's current stack pointer.

  void *        tie_save;       // 20 TIE state save area. May be NULL if there
                                // is not TIE state saved for this thread.
  
  int32_t       wake_value;     // 24 Value returned from block call (by wake call)
  
  XosSwitchFunc * switch_fn;    // 28 Pointer to a function that
                                // can be called from within this thread, to save
                                // this thread's state and switch to a specified
                                // other thread. Returns wake value.

  void *        stack_base;     // 32 Base of stack as specified by thread creator.
  
  void *        stack_end;      // 36 End of stack (adjusted for TIE state save area
                                // if any).

  XosThreadFunc * entry;        // 40 Pointer to thread entry function.
  
  void *        arg;            // 44 Argument value passed to entry function.

  bool          ready;          // 48 Set when thread is ready to run, and is in
                                // its priority queue (i.e. r_pprev is set when
                                // this flag is set).

  bool          in_exit;        // Exit flag, nonzero when in exit processing.

  int8_t        base_priority;  // Thread base priority, 0 .. (XOS_MAX_PRIORITY - 1).
                                // Higher numbers have higher priority. Must change
                                // only by calling xos_thread_set_priority().

  int8_t        curr_priority;  // Thread's current priority (curr_priority >= base_priority).
                                // A thread's current priority must be higher than another's
                                // current priority to be able to preempt it. The current
                                // priority can change during runtime e.g. due to priority
                                // inheritance.

  uint32_t      flags;          // 52 Thread creation flags.

  const char *  name;           // 56 Thread name (mainly for debug).

  const char *  block_cause;    // 60 Reason for blocking. Valid only when thread
                                // not ready (r_pprev == 0).

  XosUThread *  uthread;        // 64 Back pointer to enclosing XosUThread object
                                // (if any)

  XosThdExitFunc * exit_func;   // 68 Thread exit handler function pointer.

  XosThreadQueue * wq_ptr;      // 72 If this thread is in a wait queue, this
                                // points to the queue. Must be NULL when
                                // thread not in a queue.

  XosCondFunc * cond_fn;        // 76 Condition function. Valid only while thread
                                // is blocked on condition.

  void *        cond_arg;       // 80 Argument to be passed to condition function.

  uint16_t      cp_mask;        // 84 Mask of coprocessors used.
  uint16_t      cp_saved;       // 86 Mask of coprocessors saved.

  uint32_t      event_bits;     // 88 event bits
  uint32_t      event_mask;     // 92 event bit mask
  uint32_t      event_flags;    // 96 event flags

#if XOS_OPT_THREAD_SAFE_CLIB
  CLIB_TS_PTR   clib_ptr;       // 100 Pointer to C lib context struct.
#else
  void *        reserved1;      // 100 Reserved for consistent offsets
#endif

  uint32_t      sig;            // 104 Signature of valid TCB

  uint32_t      resume_ccount;  // 108 cycle count at resume
  uint64_t      cycle_count;    // 112 number of cycles consumed (approx).
                                // NOTE: must be 8-byte aligned
  uint32_t      normal_resumes; // 120 Number of non-preemptive resumptions.
  uint32_t      preempt_resumes;// 124 Number of preemptive resumptions.

  XosThreadQueue  exit_waiters; // 128 Queue of threads waiting for this one to exit.

#if XOS_OPT_MUTEX_PRIORITY
  struct XosMutex * owned_mtx_list; // 132 List of owned mutexes.
#endif

#if XOS_OPT_THREAD_LOCAL_STORAGE
  void *        tls[XOS_TLS_NUM_KEYS];  // Thread local storage
#endif

#if XOS_OPT_THREAD_SAFE_CLIB
  CLIB_THREAD_STRUCT;           // C library context area.
#endif
};


//-----------------------------------------------------------------------------
//  User-Mode Thread Control Block. Contains a standard TCB plus various other
//  items needed to manage user-mode threads.
//-----------------------------------------------------------------------------
struct XosUThread {
  XosThread       tcb;                              // Thread control block
#if XOS_OPT_UM
  char            name[XOS_UM_OBJ_NAME_LEN + 1];    // Thread name
  xthal_MPU_entry mpu[XOS_UM_NUM_MPU];              // List of MPU entries
#endif
};


//-----------------------------------------------------------------------------
//  User-visible flags for xos_thread_create().
//-----------------------------------------------------------------------------
#define XOS_THREAD_SUSPEND      0x0001U ///< Create suspended instead of ready
#define XOS_THREAD_NO_CP        0x0004U ///< Thread does not use coprocessors


//-----------------------------------------------------------------------------
//  Flags used by thread creation extra parameters.
//-----------------------------------------------------------------------------
#define XOS_TP_COPROC_MASK      0x0001U
#define XOS_TP_PREEMPT_PRI      0x0002U
#define XOS_TP_EXIT_HANDLER     0x0004U


//-----------------------------------------------------------------------------
//  Thread creation extra parameters.
//-----------------------------------------------------------------------------
typedef struct XosThreadParm {
  uint32_t      parms_mask;     // Combination of XOS_TP_xxx flags indicating
                                // which parameters are valid.

  uint16_t      cp_mask;        // Mask of coprocessors the thread can access.

  int8_t        preempt_pri;    // Initial preemption blocking priority. Can be
                                // changed later via xos_thread_set_priority().
  uint8_t       reserved1;

  XosThdExitFunc * handler;     // Exit handler function.

} XosThreadParm;


//-----------------------------------------------------------------------------
//  External variables.
//-----------------------------------------------------------------------------
// parasoft-begin-suppress MISRAC2012-RULE_8_6-a-2 "Defined in assembly code"
extern XosThread *      xos_curr_threadptr;     // Current active thread
extern XosThread *      xos_next_threadptr;     // Next ready thread
extern XosThread *      xos_all_threads;        // List of all threads
// parasoft-end-suppress MISRAC2012-RULE_8_6-a-2

extern bool             xos_start_flag;
extern XosThread        xos_idle_thread;


//-----------------------------------------------------------------------------
///
///  Set thread creation parameter: the group of coprocessors that this thread
///  will use. This must be set during thread creation, and cannot be changed
///  after the thread has been created. Defining this allows reduction of
///  memory usage (for CP state saving) in some circumstances, and can also
///  speed up the context switch time.
///
///  NOTE: Support for this is not currently implemented. If a thread uses
///  any coprocessor, space for all coprocessors must be reserved.
///
///  \param     parms           Thread creation parameter structure. Must be
///                             allocated by the caller and initialized with
///                             zeros prior to use.
///
///  \param     cp_mask         Bitmask of coprocessors thread is allowed to
///                             use. Bit 0 for coprocessor 0, etc.
///
///  \return    Returns nothing.
///
//-----------------------------------------------------------------------------
static inline void
xos_threadp_set_cp_mask(XosThreadParm * parms, uint16_t cp_mask)
{   
    if (parms != XOS_NULL) {
        parms->parms_mask |= XOS_TP_COPROC_MASK;
        parms->cp_mask = cp_mask;
    }
}


//-----------------------------------------------------------------------------
///
///  Set thread creation parameter: thread pre-emption priority.
///
///  \param     parms           Thread creation parameter structure. Must be
///                             allocated by the caller and initialized with
///                             zeros prior to use.
///
///  \param     preempt_pri     Thread pre-emption blocking priority.
///                             From 0 .. XOS_NUM_PRIORITY - 1.
///                             Must be greater or equal to the thread priority
///                             (if not, is automatically set to thread priority).
///
///  \return    Returns nothing.
///
///  NOTE: This parameter is currently unused.
///
//-----------------------------------------------------------------------------
static inline void
xos_threadp_set_preemption_priority(XosThreadParm * parms, int8_t preempt_pri)
{
    if (parms != XOS_NULL) {
        parms->parms_mask |= XOS_TP_PREEMPT_PRI;
        parms->preempt_pri = preempt_pri;
    }
}


//-----------------------------------------------------------------------------
///
///  Set thread creation parameter: thread exit handler.
///
///  \param     parms           Thread creation parameter structure. Must be
///                             allocated by the caller and initialized with
///                             zeros prior to use.
///
///  \param     handler         Exit handler function.
///
///  \return    Returns nothing.
//-----------------------------------------------------------------------------
static inline void
xos_threadp_set_exit_handler(XosThreadParm * parms, XosThdExitFunc * handler)
{
    if (parms != XOS_NULL) {
        parms->parms_mask |= XOS_TP_EXIT_HANDLER;
        parms->handler     = handler;
    }
}


//-----------------------------------------------------------------------------
///
///  Create a new thread. If the thread is not created suspended, then it will
///  be made ready as soon as it is created, and will immediately run if it is
///  the highest priority non-blocked thread in the system.
///
///  \param     thread          Pointer to the thread descriptor (an otherwise
///                             unused XosThread structure, usually allocated
///                             by the caller for the lifetime of the thread,
///                             for example as a global variable).
///
///  \param     reserved        Reserved at this time, pass NULL.
///
///  \param     entry           Thread entry function, takes one argument.
///
///  \param     arg             Argument "void *" that is passed to the thread function.
///
///  \param     name            Unique name of the thread, for debug/display purposes.
///                             This string must be valid for the lifetime of the thread
///                             (only a pointer to it is stored in the thread control block).
///                             Typically consists of identifier chars with no spaces.
///
///  \param     stack           Base of initial stack for the thread, allocated by the 
///                             caller. Need not be aligned (initial stack pointer will be
///                             computed and aligned from given stack base and size).
///
///  \param     stack_size      Size of the stack, in bytes.
///                             NOTE: stack should be at least XOS_STACK_EXTRA bytes plus
///                             whatever the thread actually needs if the thread will use
///                             coprocessors/TIE state. If the thread will not touch the
///                             coprocessors, then it should be XOS_STACK_EXTRA_NO_CP
///                             plus whatever the thread actually needs.
///                             Recommended minimum stack sizes are defined by the constants
///                             XOS_STACK_MIN_SIZE and XOS_STACK_MIN_SIZE_NO_CP.
///
///  \param     priority        Initial thread priority. From 0 .. XOS_MAX_PRIORITY - 1.
///                             Higher numbers are higher priority.
///
///  \param     parms           Pointer to extra parameters structure, or 0 if none given.
///                             Use xos_thread_p_***() functions to set parameters in the
///                             structure.
///
///  \param     flags           Option flags:
///                             - XOS_THREAD_SUSPEND -- Leave thread suspended instead of
///                               making it ready. The thread can be made ready to run later
///                               by calling xos_thread_resume().
///                             - XOS_THREAD_NO_CP -- Thread does not use coprocessors.
///                               No coprocessor state will be saved for this thread.
///                               Threads that have this flag set will not allocate any
///                               storage for saving coprocessor state and so can have
///                               smaller stacks.
/// 
///  NOTE: xos_start_main() calls xos_thread_create() to convert main() into the 'main'
///  thread.
///
///  \return    Returns XOS_OK if successful, error code otherwise.
///
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
                  uint32_t        flags );


//-----------------------------------------------------------------------------
///
///  Remove thread and free up all resources. Thread must have exited already.
///  After this call returns, all resources allocated to the thread (e.g. TCB,
///  stack space, etc.) can be reused.
///
///  \param     thread          Handle of thread to be deleted.
///
///  \return    Returns XOS_OK on success, else error code.
///
///  NOTE: A thread cannot call this on itself.
///
//-----------------------------------------------------------------------------
int32_t     
xos_thread_delete(XosThread * thread);


//-----------------------------------------------------------------------------
///
///  Force the thread to terminate. The thread execution is aborted, but exit
///  processing will still happen, i.e. the exit handler (if any) will be run.
///  After termination, any other threads waiting on this thread are notified.
///  This function cannot be called on the current thread.
///
///  \param     thread          Handle of thread to be aborted.
///
///  \param     exitcode        Exit code returned to any waiting threads.
///
///  \return    Returns XOS_OK on success, else error code.
///
///  NOTE: If the thread is blocked waiting for something, the wait is aborted
///  and the thread is made ready.
///  NOTE: The thread is not guaranteed to have exited when this call returns.
///  It will be made ready and set up for exit processing, but when the exit
///  processing will actually happen depends on the state of the system and
///  the priority of the thread being aborted.
///
//-----------------------------------------------------------------------------
int32_t
xos_thread_abort(XosThread * thread, int32_t exitcode);


//-----------------------------------------------------------------------------
///
///  Exit the current thread. The exit handler (if any) will be run before the
///  thread terminates.
///
///  \param     exitcode                Exit code to be returned to any waiting threads.
///
///  \return    This function does not return.
///
///  NOTE: This is automatically called if the thread returns from its entry
///  function. The entry function's return value will be passed as the exit
///  code.
///
//-----------------------------------------------------------------------------
void
xos_thread_exit(int32_t exitcode);


//-----------------------------------------------------------------------------
///
///  Wait until the specified thread exits and get its exit code. If the thread
///  has exited already, an error will be returned.
///
///  \param     thread          The thread to wait for. Cannot be "self", i.e.
///                             one cannot wait on one's own exit.
///
///  \param     p_exitcode      If not null, the exit code will be returned here.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_thread_join(XosThread * thread, int32_t * p_exitcode);


//-----------------------------------------------------------------------------
///
///  Yield the CPU to the next thread in line. The calling thread remains ready
///  and is placed at the tail of the ready queue at its current priority level.
///  If there are no threads at the same priority level that are ready to run,
///  then this call will return immediately.
///
///  \return    Returns nothing.
///
//-----------------------------------------------------------------------------
void
xos_thread_yield(void);


//-----------------------------------------------------------------------------
///
///  Suspend the specified thread. The thread will remain suspended until 
///  xos_thread_resume() has been called on it. If the thread is already blocked
///  on some other condition, then this function will return an error.
///
///  \param     thread          Handle of thread being suspended. A thread can
///                             use the special handle XOS_THREAD_SELF to suspend
///                             itself.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_thread_suspend(XosThread * thread);


//-----------------------------------------------------------------------------
///
///  Resume a suspended thread. If the thread is not suspended or is blocked on
///  some other condition then this function will do nothing. Otherwise, it will
///  be made ready, and this can cause an immediate context switch if the thread
///  is at a higher priority than the calling thread.
///
///  \param     thread          Handle of thread being resumed.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_thread_resume(XosThread * thread);


//-----------------------------------------------------------------------------
///
///  Get the priority of the specified thread. This returns the priority of the
///  queried thread at this instant, however this can change at any time due to
///  other activity in the system.
///
///  \param     thread          Handle of thread being queried. A thread can use
///                             the special handle XOS_THREAD_SELF to query itself.
///
///  \return    Returns the thread's current priority, or -1 if the thread handle
///             is not valid.
///
//-----------------------------------------------------------------------------
int8_t
xos_thread_get_priority(const XosThread * thread);


//-----------------------------------------------------------------------------
///
///  Set the priority of the specified thread. The thread must exist. 
///
///  \param     thread          Handle of thread being affected. A thread can
///                             use the special handle XOS_THREAD_SELF to specify
///                             itself.
///
///  \param     priority        The new priority level to be set.
///
///  \return    Returns XOS_OK on success, else error code.
///
///  NOTE: Calling this function can result in a scheduler activation, and the
///  caller may be suspended as a result.
///
//-----------------------------------------------------------------------------
int32_t
xos_thread_set_priority(XosThread * thread, int8_t priority);


//-----------------------------------------------------------------------------
///
///  Return the name of the specified thread.
///
///  \param     thread          Handle of thread being queried. A thread can use
///                             the special handle XOS_THREAD_SELF to specify
///                             itself.
///
///  \return    Returns a pointer to the name string if available, else NULL.
///
//-----------------------------------------------------------------------------
const char *
xos_thread_get_name(const XosThread * thread);


//-----------------------------------------------------------------------------
///
///  Set the name of the specified thread.
///
///  \param     thread          Handle of thread whose name is to be set. A thread
///                             can use the special handle XOS_THREAD_SELF to specify
///                             itself.
///
///  \param     name            Pointer to the new name string. The string is not
///                             copied, only the pointer is saved. So the string
///                             must be persistent for the life of the thread.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_thread_set_name(XosThread * thread, const char * name);


//-----------------------------------------------------------------------------
///
///  Set an exit handler for the specified thread. The exit handler is run when
///  the thread terminates, either by calling xos_thread_exit() or by returning
///  from its entry function. It will also be called if the thread is being
///  terminated due to e.g. an unhandled exception.
///
///  The handler must be a function defined as e.g.:
///
///      int32_t exit_handler(int32_t exitcode);
///
///  The exit handler runs in the context of the exiting thread, and can call
///  system services. It is provided with a single parameter which is the
///  thread's exit code (the exit code may be set to an error code if the
///  thread is being terminated due to an error or exception). The handler
///  must return a value which will be set as the thread's exit code.
///
///  \param     thread          Handle of the thread for which the handler is
///                             to be installed. A thread can use the special
///                             handle XOS_THREAD_SELF to specify itself.
///
///  \param     func            Pointer to exit handler function. To clear an
///                             existing handler, pass NULL as the pointer.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_thread_set_exit_handler(XosThread * thread, XosThdExitFunc * func);


//-----------------------------------------------------------------------------
///
///  Return the ID (handle) of the current thread.
///
///  \return    Returns the handle of the current thread. This handle can be
///             used in all XOS system calls.
///
///  NOTE: If called from interrupt context, returns the handle of the thread
///  that was preempted.
///
//-----------------------------------------------------------------------------
XosThread *
xos_thread_id(void);


//-----------------------------------------------------------------------------
///
///  Return the coprocessor mask for the specified thread.
///
///  \param     thread          Handle of thread being queried.
///
///  \return    Returns the mask for the specified thread if available, else 0.
///
//-----------------------------------------------------------------------------
uint16_t
xos_thread_cp_mask(const XosThread * thread);


//-----------------------------------------------------------------------------
///
///  Return the wake value for the specified thread.
///
///  \param     thread          Handle of thread being queried.
///
///  \return    Returns The last set wake value. There is no way to detect what
///             action set the wake value and when.
///
//-----------------------------------------------------------------------------
int32_t
xos_thread_get_wake_value(const XosThread * thread);


//-----------------------------------------------------------------------------
///
///  Return the current value of the event bits for the current thread.
///  This function takes no parameters.
///
///  \return    Returns the current value of the event bits. The event bits
///             are set when the thread is woken from an event wait. They will
///             not change while the thread is running. There is no way to
///             determine when the event bits were last updated.
///
//-----------------------------------------------------------------------------
uint32_t
xos_thread_get_event_bits(void);


//-----------------------------------------------------------------------------
///
///  Enum values for thread state.
///
//-----------------------------------------------------------------------------
typedef enum xos_thread_state_t {
    XOS_THREAD_STATE_INVALID = 0,       ///< Invalid thread
    XOS_THREAD_STATE_BLOCKED,           ///< Thread is blocked
    XOS_THREAD_STATE_READY,             ///< Thread is ready to run
    XOS_THREAD_STATE_RUNNING,           ///< Thread is running
    XOS_THREAD_STATE_EXITED,            ///< Thread has exited
} xos_thread_state_t;


//-----------------------------------------------------------------------------
///
///  Return the state of the specified thread.
///
///  \param     thread          Handle of thread being queried.
///
///  \return    Returns one of the following values:
///             - XOS_THREAD_STATE_RUNNING -- The thread is currently running.
///             - XOS_THREAD_STATE_READY   -- The thread is ready to run.
///             - XOS_THREAD_STATE_BLOCKED -- The thread is blocked on something.
///             - XOS_THREAD_STATE_INVALID -- The thread handle is invalid.
///             - XOS_THREAD_STATE_EXITED  -- The thread has exited.
///
//-----------------------------------------------------------------------------
xos_thread_state_t
xos_thread_get_state(const XosThread * thread);


//-----------------------------------------------------------------------------
///
///  Disable thread preemption. Prevents context switching to another thread.
///  However, interrupt handlers will still continue to be run. Multiple calls
///  will nest, and the same number of calls to xos_preemption_enable() will be
///  required to re-enable preemption. If the calling thread yields the CPU or
///  exits without enabling preemption, it will cause a system halt.
///  If the calling thread encounters a fatal error, preemption will be enabled
///  during fatal error handling.
///
///  \return    Returns the new value of preemption disable flag after this call.
///
///  NOTE: Cannot be called from interrupt context.
///
//-----------------------------------------------------------------------------
uint32_t
xos_preemption_disable(void);


//-----------------------------------------------------------------------------
///
///  Enable thread preemption. Has no effect if preemption was already enabled.
///  Otherwise, it decrements the value of the preemption disable flag and if
///  the value goes to zero, enables preemption.
///
///  \return    Returns the new value of preemption disable flag after this call.
///
///  NOTE: If scheduling gets enabled, it may cause an immediate context switch
///  if higher priority threads are ready.
///
//-----------------------------------------------------------------------------
uint32_t
xos_preemption_enable(void);


//-----------------------------------------------------------------------------
///
///  Initialize XOS thread support and start scheduler.
///
///  Called from application code to start multitasking. This function initializes
///  thread support, creates the idle thread and starts the scheduler. It does
///  not return to its caller. This means that at least one user thread
///  must be created before calling xos_start(). Otherwise, the scheduler will
///  run the idle thread since it will be the only thread in the system, and no
///  other thread can be created. Typically this function is called from main()
///  after some initialization has been completed and one or more threads have
///  been created.
///
///  NOTE: This function does not initialize timer/tick support. For timer
///  services to be available xos_start_system_timer() must be called.
///
///  NOTE: xos_start() and xos_start_main() are exclusive, both cannot be
///  called within the same application.
///
///  \param     flags           Currently unused (pass 0).
///
///  \return    Does not return.
///
//-----------------------------------------------------------------------------
void
xos_start(uint32_t flags);


//-----------------------------------------------------------------------------
///
///  Initialize XOS thread support and create init (main) thread.
///
///  Called from application code to start multitasking. This function initializes
///  thread support, creates the idle thread, converts the caller into the 'main'
///  or 'init' thread, and returns to the caller. Typically this function is called
///  from main() after some initialization has been completed. Because it returns,
///  it is not required to create any user threads before calling this function.
///
///  NOTE: This function does not initialize timer/tick support. For timer
///  services to be available xos_start_system_timer() must be called.
///
///  NOTE: xos_start_main() and xos_start() are exclusive, both cannot be
///  called within the same application.
///
///  \param     name            Name of main thread (see xos_thread_create()).
///
///  \param     priority        Initial priority of main thread.
///
///  \param     flags           Currently unused (pass 0).
///
///  \return    Returns nothing.
///
//-----------------------------------------------------------------------------
void
xos_start_main(const char * name, int8_t priority, uint32_t flags);


//-----------------------------------------------------------------------------
///
///  Initialize XOS thread support and create init (main) thread with specified
///  stack size.
///
///  Called from application code to start multitasking. This function initializes
///  thread support, creates the idle thread, converts the caller into the 'main'
///  or 'init' thread, and returns to the caller. Typically this function is called
///  from main() after some initialization has been completed. Because it returns,
///  it is not required to create any user threads before calling this function.
///
///  NOTE: This function does not initialize timer/tick support. For timer
///  services to be available xos_start_system_timer() must be called.
///
///  NOTE: xos_start_main_ex() and xos_start() are exclusive, both cannot be
///  called within the same application.
///
///  \param     name            Name of main thread (see xos_thread_create()).
///
///  \param     priority        Initial priority of main thread.
///
///  \param     stack_size      Required stack size. The stack currently in use
///                             will continue to be used, the size is required
///                             to set the stack limit for bounds checking.
///
///  \return    XOS_OK on success, return code on error.
///
//-----------------------------------------------------------------------------
int32_t
xos_start_main_ex(const char * name, int8_t priority, uint32_t stack_size);


//-----------------------------------------------------------------------------
///
///  Per-thread stats structure.
///  Note that the CPU use % is approximate, both because of cycle counting
///  and because of integer division. So all the threads' CPU % will not add
///  up to exactly 100%.
///
//-----------------------------------------------------------------------------
typedef struct XosThreadStats {
  XosThread *   thread;            ///< Thread handle (or pseudo-handle)
  uint32_t      cpu_pct;           ///< CPU use % for this thread
  uint32_t      normal_switches;   ///< Number of non-preemptive switches.
  uint32_t      preempt_switches;  ///< Number of preemptive switches.
  uint64_t      cycle_count;       ///< Number of cycles consumed.
} XosThreadStats;


//-----------------------------------------------------------------------------
// Type conversion helper (external)
//-----------------------------------------------------------------------------
__attribute__((always_inline))
static inline XosThread *
xos_uint32_to_threadp(uint32_t arg)
{
    return (XosThread *) arg; // parasoft-suppress MISRAC2012-RULE_11_4-a-4 "Type conversion checked"
}

//-----------------------------------------------------------------------------
//  Thread pseudo-handles.
//-----------------------------------------------------------------------------
#define XOS_THD_STATS_IDLE      xos_uint32_to_threadp(1U)
#define XOS_THD_STATS_INTR      xos_uint32_to_threadp(2U)
#define XOS_THREAD_SYSTEM       xos_uint32_to_threadp(0xFFFFFFFFU)


//-----------------------------------------------------------------------------
///
///  Get the thread statistics for the specified thread. Statistics are only
///  available if XOS_OPT_STATS has been enabled. Otherwise, the function
///  will return XOS_OK, but the structure contents will be undefined.
///
///  \param     thread          Handle of thread being queried. The following
///                             special pseudo-handles can be used:
///                             - XOS_THD_STATS_IDLE -- stats for idle thread
///                             - XOS_THD_STATS_INTR -- stats for interrupt processing
///
///  \param     stats           Pointer to XosThreadStats struct to be filled in.
///
///  \return    Returns XOS_OK on success, else error code.
///
///  NOTE: Can be called from interrupt context.
///  NOTE: This call will not fill in the "thread" and "cpu_pct" fields in the
///  "stats" structure. The thread handle is already known, and calculating the
///  CPU loading can take quite a bit of time so is not done here.
///
//-----------------------------------------------------------------------------
int32_t
xos_thread_get_stats(const XosThread * thread, XosThreadStats * stats);


//-----------------------------------------------------------------------------
///
///  Get CPU loading statistics for the system. This function computes the CPU
///  percentage use for all threads in the system (including the idle thread and
///  the 'interrupt thread' (interrupt context). It also returns the cycle count
///  and number of context switches for each thread.
///  Statistics are only available if XOS_OPT_STATS has been enabled.
///  Otherwise, the function will return XOS_OK, but the structure contents will
///  be undefined.
///
///  IMPORTANT: The entry for interrupt context does not contain a real thread
///  handle. It uses the pseudo-handle XOS_THD_STATS_INTR to indicate that this
///  entry reports interrupt statistics. This pseudo-handle cannot be used for
///  any other thread operations or queries.
///
///  NOTE: This function disables interrupts while traversing the thread list.
///  It does not leave interrupts disabled during the computations, as that can
///  take a fair amount of time.
///
///  \param     stats           Pointer to an array of XosThreadStats structures.
///                             The array must be large enough to accommodate all
///                             threads in the system.
///
///  \param     size            The number of elements available in the array. If
///                             this is smaller than the number of threads plus one
///                             (for the interrupt context) then XOS_ERR_INVALID_PARAMETER
///                             will be returned and '*size' will be set to the
///                             minimum number of elements required. On a successful
///                             return, '*size' is set to the number of elements
///                             actually filled in.
///
///  \param     reset           If nonzero, then thread stats counters are reset
///                             after reading. This is useful if you want to track
///                             the stats so as to get a better idea of current
///                             system loading. E.g. calling this function once a
///                             second with 'reset' nonzero will provide CPU load
///                             information for the last second on each call.
///
///  \return    Returns XOS_OK on success, else error code. In particular,
///             XOS_ERR_INVALID_PARAMETER will be returned if the output buffer
///             is too small.
///
//-----------------------------------------------------------------------------
int32_t
xos_get_cpu_load(XosThreadStats * stats, int32_t * size, int32_t reset);


//-----------------------------------------------------------------------------
//  NOTE: This function is currently not documented or supported.
//
//  Specify which software interrupt the scheduler should use. This is only
//  required for XEA3-based systems. The scheduler will use the interrupt to
//  trigger context switch as needed. If you do not specify an interrupt 
//  number, then XOS will automatically select the first (i.e. numerically
//  smallest) free software interrupt that it finds during startup.
//
//  \param     intnum          SW interrupt number to use for scheduling.
//
//  \return    Returns XOS_OK on success, XOS_ERR_INVALID_PARAMETER if the
//             specified interrupt is not a software interrupt.
//  
//  IMPORTANT: If this function is called, it must be called before either
//  xos_start() or xos_start_main() has been called. Once XOS is started the
//  interrupt number cannot be changed. Also, once the scheduler interrupt
//  has been selected, it cannot be used for any other purpose.
//
//-----------------------------------------------------------------------------
int32_t
xos_set_sched_interrupt(int32_t intnum);


//-----------------------------------------------------------------------------
///
///  Allocate a thread-specific data key visible to all user threads. The same
///  key value can be used by different threads, but the value bound to the key
///  will be specific to each thread. The numeric value of the key should not
///  be relied upon. A key must be allocated before it can be used in calls to
///  xos_tls_set() and xos_tls_get().
///
///  An optional function may be associated with the key when it is created.
///  When a thread terminates, if the function pointer for a key is not NULL
///  and the thread's value for the key is not NULL then the function will be
///  called with the key's value as its parameter, and the key's value will be
///  set to NULL. If multiple keys have functions associated with them the
///  order in which they are called is unspecified.
///
///  \param     key             Pointer to storage for assigned key. Space for
///                             storing the key must be allocated by caller.
///
///  \param     destructor      Pointer to optional function to be associated
///                             with this key. Pass NULL if no function is to
///                             be registered.
///
///  \return    Returns XOS_OK on success, else an error code. XOS_ERR_LIMIT
///             is returned if there are no more keys available.
///
//-----------------------------------------------------------------------------
int32_t
xos_tls_create(uint32_t * key, void (*destructor)(void * arg));


//-----------------------------------------------------------------------------
///
///  Delete a thread-specific data key. This function does not alter the thread
///  specific data values in any way. It removes the key-specific destructor
///  function associated with this key (if any). Any attempt to use the key
///  after deleting it can result in undefined behavior. This function does not
///  call any key destructor functions.
///
///  \param     key             Key allocated by a call to xos_tls_create().
///
///  \return    Returns XOS_OK on success, else an error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_tls_delete(uint32_t key);


//-----------------------------------------------------------------------------
///
///  Set thread-specific value for a specified key. The key must have been 
///  created first, or the result is undefined. This function must be called
///  in thread context.
///
///  \param     key             Key allocated by a call to xos_tls_create().
///
///  \param     value           Value to associate with this key in the context
///                             of the current thread.
///
///  \return    Returns XOS_OK on success, else an error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_tls_set(uint32_t key, void * value);


//-----------------------------------------------------------------------------
///
///  Retrieve thread-specific value for a specified key. The key must have been 
///  created first, or the result is undefined. This function must be called
///  in thread context.
///
///  \param     key             Key allocated by a call to xos_tls_create().
///
///  \return    Returns the value associated with this key, or NULL if the key
///             is invalid or no value was set.
///
//-----------------------------------------------------------------------------
void *
xos_tls_get(uint32_t key);


//-----------------------------------------------------------------------------
///
///  Create a new user-mode thread. If the thread is not created suspended,
///  then it will be made ready as soon as it is created, and will immediately
///  run if it is the highest priority non-blocked thread in the system.
///
///  The new thread's private memory base and bound must be specified or 
///  inherited from the caller. Typically this region will include the thread's
///  code, data and stack. This region must be set before the thread is started,
///  else there will be no memory for the thread to run in.
///
///  \param     uthread         Pointer to the thread descriptor (an otherwise
///                             unused XosUThread structure, usually allocated
///                             by the caller for the lifetime of the thread).
///
///  \param     entry           Thread entry function, takes one argument.
///
///  \param     arg             Argument "void *" that is passed to the thread function.
///
///  \param     res1            Reserved, pass NULL.
///
///  \param     res2            Reserved, pass NULL.
///
///  \param     name            Unique name of the thread. This string must be valid
///                             for the lifetime of the thread (only a pointer to it
///                             is stored).
///
///  \param     stack           Base of initial stack for the thread, allocated by the 
///                             caller. Need not be aligned (initial stack pointer will be
///                             computed and aligned from given stack base and size).
///
///  \param     stack_size      Size of the stack, in bytes.
///                             NOTE: stack should be at least XOS_STACK_EXTRA bytes plus
///                             whatever the thread actually needs if the thread will use
///                             coprocessors/TIE state. If the thread will not touch the
///                             coprocessors, then it should be XOS_STACK_EXTRA_NO_CP
///                             plus whatever the thread actually needs.
///                             Recommended minimum stack sizes are defined by the constants
///                             XOS_STACK_MIN_SIZE and XOS_STACK_MIN_SIZE_NO_CP.
///
///  \param     priority        Initial thread priority. From 0 .. XOS_MAX_PRIORITY - 1.
///                             Higher numbers are higher priority.
///
///  \param     flags           Option flags:
///                             - XOS_THREAD_SUSPEND -- Leave thread suspended instead of
///                               making it ready. The thread can be made ready to run later
///                               by calling xos_thread_resume().
///                             - XOS_THREAD_NO_CP -- Thread does not use coprocessors.
///                               No coprocessor state will be saved for this thread.
///                               Threads that have this flag set will not allocate any
///                               storage for saving coprocessor state and so can have
///                               smaller stacks.
/// 
///  \param     pmptr           Pointer to thread's private memory region. Must not be
///                             NULL if called from a privileged thread. If called from
///                             user-mode thread and this is NULL, then the private 
///                             region is inherited from the calling thread. Must be 
///                             aligned to MPU minimum region size if not NULL.
///
///  \param     pmsize          Size of private memory region. Must not be zero. Must
///                             be aligned to MPU minimum region size.
///
///  \param     mpu_entry       Index of MPU entry (pair) to be used to define private
///                             memory region. 'entry' and 'entry+1' are used.
///
///  \return    Returns XOS_OK if successful, error code otherwise.
///
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
                     uint32_t        mpu_entry);


//-----------------------------------------------------------------------------
///
///  Remove user-mode thread and free up all resources. The thread must have
///  exited already. After this call returns, all resources allocated to the
///  thread (e.g. TCB, stack space, etc.) can be reused.
///
///  \param     uthread          Handle of thread to be deleted.
///
///  \return    Returns XOS_OK on success, else error code.
///
///  NOTE: A thread cannot call this on itself.
///
//-----------------------------------------------------------------------------
int32_t
xos_uthread_delete_p(XosUThread * uthread);


//-----------------------------------------------------------------------------
///
///  Retrieve the thread handle for the specified user-mode thread. The handle
///  is to be used for all privileged-mode calls.
///
///  \param     uthread         Handle of user-mode thread.
///
///  \return    The thread's privileged-mode handle.
///
//-----------------------------------------------------------------------------
XosThread *
xos_uthread_thread(XosUThread * uthread);


#ifdef XOS_INCLUDE_INTERNAL

// Signature of valid thread object
#define XOS_THREAD_SIG  0x54485244U


// Extern functions
void
xos_init(void);

bool
xos_init_done(void);

bool
xos_started(void);

void
xos_thread_setup_tie_save(XosThread * thread, void * tie_save, bool alloc_cp);

int32_t
xos_create_timer_thread(void);

uint32_t
xos_preemption_enable_ns(void);

void
xos_q_pri_insert(XosThreadQueue * q, XosThread * thread);

void
xos_q_remove(XosThreadQueue * q, XosThread * thread);

void
xos_q_push(XosThreadQueue * q, XosThread * thread);

XosThread *
xos_q_pop(XosThreadQueue * q);

__attribute__((always_inline))
static inline XosThread *
xos_q_peek(const XosThreadQueue * q)
{
    return (q != XOS_NULL) ? q->head : XOS_NULL;
}

void
xos_q_insert(XosThreadQueue * q, XosThread * thread);

int32_t
xos_wake_queue(XosThreadQueue * queue, const char * expected_cause, int32_t wake_value);

// Well known block causes
extern const char * const xos_blkon_idle;       // (for idle thread only)
extern const char * const xos_blkon_suspend;
extern const char * const xos_blkon_delay;
extern const char * const xos_blkon_exited;
extern const char * const xos_blkon_join;
extern const char * const xos_blkon_event;
extern const char * const xos_blkon_condition;
extern const char * const xos_blkon_mutex;
extern const char * const xos_blkon_sem;
extern const char * const xos_blkon_msgq;


//-----------------------------------------------------------------------------
//  Blocks the current active thread.
//
//  Currently, this can be called from an interrupt handler to block the thread
//  that was interrupted. Note that in interrupt context the current thread can
//  already be in the blocked state, due to a previous call to this function.
//  Can be called with interrupts enabled.
//
//  block_cause         Reason for blocking.
//
//  block_queue         Queue on to which this thread should be pushed once it
//                      is blocked. Can be NULL.
//
//  must_schedule       If nonzero, then forces a scheduling operation to pick
//                      the next thread to run, even if there are ready threads
//                      at the same priority level as the blocked thread.
//                      (Currently unused).
//
//  use_priority        If nonzero, then the blocked thread will be queued in
//                      priority order in the specified block queue. If zero,
//                      the thread is queued in FIFO order. If no queue has
//                      been specified, this parameter is ignored.
//
//  Returns: The value passed by xos_thread_wake().
//-----------------------------------------------------------------------------
int32_t
xos_block(const char *     block_cause,
          XosThreadQueue * block_queue,
          int32_t          must_schedule,
          int32_t          use_priority);


//-----------------------------------------------------------------------------
//  Unblocks the specified blocked thread and puts it at the tail end of its
//  ready queue. Schedules it if it is higher priority than the current thread.
//  No effect if the thread is not blocked with the specified cause.
//
//  thread              The thread to wake up (make ready).
//
//  expected_cause      The expected block cause of the thread. Thread will be
//                      woken only if its block cause matches this cause, or if
//                      expected_cause is zero.
//  
//  wake_value          The value to be passed to the woken thread as a return
//                      value from xos_thread_block().
//
//  Returns: nothing.
//
//  The target thread can be woken at a different priority by changing its
//  priority while the thread is blocked.
//  Can be called with interrupts enabled. Can be called in interrupt context.
//-----------------------------------------------------------------------------
void
xos_thread_wake(XosThread * thread, const char * expected_cause, int32_t wake_value);


#if XOS_OPT_MUTEX_PRIORITY
//-----------------------------------------------------------------------------
//  Adds the mutex to the thread's owned list and reprioritizes the thread if
//  needed based on the mutex's priority.
//
//  owner               The thread that owns the mutex.
//
//  mutex               The newly owned mutex.
//
//  Returns: Nothing.
//
//  IMPORTANT: This function expects to be called within an OS critical 
//  section, where it is safe to manipulate the thread and mutex states
//  directly without further protection.
//-----------------------------------------------------------------------------
void
xos_thread_mutex_add(XosThread * owner, struct XosMutex * mutex);


//-----------------------------------------------------------------------------
//  Removes the mutex from the thread's owned list and reprioritizes the thread
//  if needed.
//
//  owner               The thread that owns the mutex.
//
//  mutex               The mutex being released.
//
//  Returns: Nothing.
//
//  IMPORTANT: This function expects to be called within an OS critical 
//  section, where it is safe to manipulate the thread and mutex states
//  directly without further protection.
//-----------------------------------------------------------------------------
void
xos_thread_mutex_rem(XosThread * owner, struct XosMutex * mutex);
#endif


//-----------------------------------------------------------------------------
//  Raises the priority of the 'owner' thread to the specified priority while
//  it owns 'mutex'. This provides priority inheritance and prevents a higher
//  priority thread from being blocked by a lower priority one.
//
//  owner               The thread that owns the mutex.
//
//  mutex               The mutex which controls the inheritance.
//
//  pri                 The new priority for the owner while it owns 'mutex'.
//
//  Returns: Nothing.
//
//  IMPORTANT: This function expects to be called within an OS critical 
//  section, where it is safe to manipulate the thread and mutex states
//  directly without further protection.
//-----------------------------------------------------------------------------
void
xos_thread_mutex_bump(XosThread * owner, struct XosMutex * mutex, int8_t pri);


//-----------------------------------------------------------------------------
// Run TLS cleanup for exiting thread.
//-----------------------------------------------------------------------------
void
xos_tls_cleanup(void);


//-----------------------------------------------------------------------------
//  Function to init C library per-thread and reentrancy support.
//-----------------------------------------------------------------------------
#if XOS_OPT_THREAD_SAFE_CLIB
void
xos_clib_init(void);

void
xos_clib_thread_init(XosThread * thread);

void
xos_clib_thread_cleanup(XosThread * thread);
#endif


//-----------------------------------------------------------------------------
// Type conversion helpers.
//-----------------------------------------------------------------------------
__attribute__((always_inline))
static inline XosThread *
xos_voidp_to_threadp(void * arg)
{
    return (XosThread *) arg; // parasoft-suppress MISRAC2012-RULE_11_5-a-4 "Type conversion checked"
}

__attribute__((always_inline))
static inline uint32_t
xos_threadp_to_uint32(XosThread * arg)
{
    return (uint32_t) arg; // parasoft-suppress MISRAC2012-RULE_11_4-a-4 "Type conversion checked"
}


#endif // XOS_INCLUDE_INTERNAL


#ifdef __cplusplus
}
#endif

#endif  // XOS_THREAD_H

