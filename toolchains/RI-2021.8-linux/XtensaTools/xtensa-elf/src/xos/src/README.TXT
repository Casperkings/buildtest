-------------------------------------------------------------------------
Copyright (c) 2013-2020 Cadence Design Systems, Inc.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
-------------------------------------------------------------------------


Notes for Version 3.00
----------------------

- XOS now supports user-mode threads. This feature requires the use of
  dynamic loadable libraries, and requires the MPU to be configured.
  This feature is not enabled by default. Refer to the user manual and
  the examples for details.

- A new function xos_event_clear_and_wait() has been added to complement
  xos_event_set_and_wait().

- Coprocessor exception handling for XEA3 has been modified to disable
  interrupts during coprocessor state update. Interrupts during this
  process can cause coprocessor information to be left in an inconsistent
  state potentially leading to data corruption.

- A bug in xos_get_system_cycles() could cause the return value to roll
  over on the 32-bit boundary if the CCOUNT register rolled over between
  XOS system timer start and the first tick interrupt. This has been
  fixed. This would be a very unusual occurrence in normal operation
  since XOS is usually started shortly after reset, long before CCOUNT
  rolls over for the first time after reset.

- xos_thread_get_stats() was not accounting for cycles consumed by the
  current thread since last resumption. This has been fixed.

- The experimental designation has been removed from Secure Mode support.


Notes for Version 2.10
----------------------

- A new option XOS_OPT_SECMON has been added to enable building applications
  that run in nonsecure mode on Xtensa cores that support Secure Mode. See 
  the user manual and related documentation for details. This feature should
  be considered experimental in this release and not used in production code.

- Reference code for setting up the Memory Protection Unit (MPU) is available
  in the file mpu_init.c. This file is not included in the XOS library, so it
  has to be compiled as part of the application if needed. See the notes in
  the file for details.

- Two minor improvements have been made in scheduling. xos_thread_suspend()
  now runs the scheduler if the current or the next thread is being suspended.
  xos_thread_wake() now always runs the scheduler, not just if the woken thread
  is of higher priority.

- The diagostic functions xos_system_check() / xos_thread_check() are no longer
  classified as experimental.


Notes for Version 2.09
----------------------

- A new function xos_start_main_ex() is introduced as an improvement over
  the xos_start_main() function. See documentation for details.
  While xos_start_main() is retained for compatibility, its use is now
  deprecated.

- The fatal error handler now calls _exit(), but only if the user-defined
  handler returns, and only if _exit() is already being called elsewhere.

- Two new diagnostic functions have been added - xos_system_check() and
  xos_thread_check(). These should be considered experimental and must
  not be used in production code.


Notes for Version 2.08
----------------------

- Some XOS configuration options have been removed and the debug options 
  have been consolidated. See the user manual for details.

- More input parameter error checking has been added at various places in
  the API, and more functions now return a result code. Most API functions
  now check XOS object signatures regardless of debug options selected.

- Timer tick handling has been improved to reduce variation in tick interval
  when operated in fixed-interval mode. This required a change in the timer
  support API which is not backward compatible. The xos_system_timer_set()
  function now takes an extra parameter. The timing margin for scheduling the
  next tick has also been improved. See documentation for details.

- For configurations with exclusive load/store support, the ATOMCTL register
  state was not being correctly saved/restored on interrupt. This has been
  fixed.

- Calling xos_thread_abort() on a thread that had never been run could cause
  an exception leading to a fatal error. A race condition inside the function
  could also cause a fatal error. Both problems have been fixed.

- A bug was introduced in v2.04 that could allow the following scenario:
  a thread calls xos_mutex_lock_timeout() with a short timeout and attempts
  to block on the mutex. After the timeout is started but before the block
  is completed, the timer interrupt could fire and the timeout could be seen
  as expired. The wakeup does nothing in this case as the thread has not 
  blocked yet. After the failed wakeup, the thread blocks and never wakes up
  again because the timeout has expired. This has been fixed.

- Exception dispatch for XEA2 could potentially cause stack corruption when
  registers were spilled to stack. This has been fixed.


Notes for Version 2.07
----------------------

- A bug in the function xos_tls_delete() that caused it to return an error
  code even on success, has been fixed.

- An interface layer to the iDMA library has been added. This code is self
  contained and none of it is linked in if the application does not use the
  iDMA feature. See the user manual for details.

- A new define XOS_INT_STACK_MIN_SIZE has been added to provide an indication
  of the minimum interrupt stack size required for the target configuration.
  This number is meant as a guide only, please read the user manual carefully
  to understand its implications.

- Some unused symbols in assembly code have been removed. Some dummy symbols
  have been added so as to cause an error if XTOS interrupt/exception API
  functions are called from an XOS application. XOS and XTOS cannot be mixed
  together in the same application.


Notes for Version 2.06
----------------------

- A bug in XOS timer tick processing has been fixed in this release. This
  bug was infrequent but when certain timing conditions matched up it would
  cause XOS internal state to become inconsistent with timer state and lead
  to missed interrupts and missed timer events. This is now fixed. However
  the API functions xos_system_timer_init() and xos_system_timer_set() did
  need to have their signature modified. Both these functions now return the
  cycle count value of the timer that they set (i.e. the value written to the
  CCOMPARE register in case of internal timer). See user guide for details.

- A bug in xos_timer_wait() could cause a thread to block on a timer that had
  already expired. This has been fixed.

- Run-to-completion (RTC) threads are no longer supported. This feature
  was never fully tested and supported. The associated code has been 
  removed to reduce complexity and make testing easier.

- There is now an option to handle timer processing in a thread rather than
  in the timer interrupt handler. This is controlled by XOS_OPT_TIMER_THREAD
  and is disabled by default. Processing in a thread allows the interrupt
  handler to finish faster and allows potentially blocking OS calls, but
  does require more memory for the timer thread TCB and stack. See the user
  manual for details.

- The XOS_DEBUG option no longer prints debug output by default. A new option
  XOS_DEBUG_VERBOSE controls printing in debug mode. Printing may also be enabled
  with XOS_DEBUG at runtime by setting "xos_debug_print" to 1. This makes debug
  mode (XOS_DEBUG) run faster and still include all assertion checks.
  More assertion checks have been added in the debug build.

- The user manual has been revised and updated. A new chapter on performance
  has been added, with an explanation of the XOS performance test.


Notes for Version 2.05
----------------------

- Thread-local storage support is available in this version. This option is
  controlled by XOS_OPT_THREAD_LOCAL_STORAGE and is enabled by default. See
  the user manual for details.

- A bug in the XOS message queue implementation could cause a race condition
  between a thread blocking to wait for a message and a message being posted
  to the queue, such that the blocked thread would never be woken up. This
  has been fixed.

- Timer interrupt handling has been improved to eliminate cases where a higher
  priority interrupt's handler could manipulate timers and interfere with the
  timer handler's processing.


Notes for Version 2.04
----------------------

- A potential problem in xos_timer_restart() that could inadvertently
  clear a pending timer interrupt without processing it, has been fixed.
  This would have affected only processors using the XEA2 exception
  architecture and using one of the internal timers as the system timer.
  NOTE: A new function xos_system_timer_pending() is defined and must be
  supported by the system timer driver if you implement your own driver.

- xos_start_main() was not setting the C library thread context pointer
  correctly for the initial thread. It would only get set up correctly on
  the first context switch. This has been fixed.

- A build configuration signature check is now available to aid in debugging
  configuration mismatches between the XOS library and the application.
  See documentation for xos_check_build_config() for instructions on usage.

- A new API function xos_once() is now available for implementing one-time
  initialization routines. See documentation for details.

- A new barrier API is now available for synchronizing multiple threads.
  See documentation for details.

- Mutex lock/unlock functions can no longer be called from interrupt handlers.
  Allowing this led to confusion on usage and potentially dangerous behavior.

- Atomic operations such as mutex and semaphore operations can now use the
  atomic functions defined by C11. This is enabled by the XOS_OPT_C11_ATOMICS
  option. This option requires the Xtensa C Library (xclib) and the code must
  be compiled to the C11 standard. See documentation for details.
  NOTE: This feature is new and still under test.


Notes for Version 2.03
----------------------

- A bug in thread wakeup logic that allowed a lower priority thread to
  run while a higher priority thread was ready has been fixed.

- A bug in mutex priority handling that could cause a crash has been fixed.


Notes for Version 2.02
----------------------

- A new system logging module is introduced in this version. It is more
  flexible than the previous version, which is now deprecated but still
  supported. XOS itself will use the new module to log system events when
  the option XOS_OPT_LOG_SYSEVENT is enabled.

- A bug in mutex priority inversion handling that could cause inconsistent
  internal state has been fixed.

- A bug in coprocessor lazy context switch handling that could cause data
  corruption has been fixed.

- Some compiler warnings have been fixed and some unused code has been
  removed.


Notes for Version 2.01
----------------------

- Xtensa NX and XEA3 are fully supported.

- All bugfixes and improvements from XOS version 1.15 are incorporated
  in this release.


Notes for Version 2.00
----------------------

- First release with Xtensa NX and XEA3 support. This should be treated
  as a beta release for NX/XEA3.


Notes for Version 1.15
----------------------

- XOS now supports priority inversion protection for mutexes. The option
  XOS_OPT_MUTEX_PRIORITY controls this feature. See the user manual for
  details.

- New API calls have been added for condition objects: xos_cond_signal_one(),
  xos_cond_wait_mutex(), xos_cond_wait_mutex_timeout(). See the user
  manual for details.

- Threads now wait on condition objects in priority order.

- A bug in timer handling that could cause timer events to be processed
  out of order has been fixed.

- The use of XOS_DEBUG and XOS_DEBUG_ALL has been made consistent. Now 
  both are always defined, with a zero value indicating disabled and a
  nonzero value indicating enabled.


Notes for Version 1.14
----------------------

- The XOS_EVENT_AUTO_CLEAR option for event objects is now supported.
  See user manual for details.

- The time period for round robin time slicing can now be configured in
  terms of the number of system timer ticks by specifying the value of
  XOS_TIME_SLICE_TICKS. This takes effect only if XOS_OPT_TIME_SLICE is
  enabled. See user manual for details.

- In version 1.13, a bug was introduced into the coprocessor context
  switching logic. This bug could corrupt one or more of registers 
  a8-a15 during lazy coprocessor context switching. This has been fixed.

- For C++ thread-safe operation with the Xtensa C/C++ Library (xclib) the
  default number of locks has been increased by 8. If C++ will not be used,
  some memory can be saved by reducing this number. See XOS_NUM_CLIB_LOCKS
  in file xos_clib.c.

- Various compiler warnings have been cleaned up. This should have no
  effect on functionality or API compatibility.


Notes for Version 1.13
----------------------

- A new API call xos_sem_put_max() has been added. This increments the
  semaphore count only if the specified max count will not be exceeded.

- XOS vector code has been reorganized to support Xtensa configurations
  with small vector sizes (< 28 bytes).

- Assembler instruction scheduling has been enabled in the default build
  options, for assembly source files. This can potentially improve XOS
  performance depending on the hardware characteristics of the target
  Xtensa configuration. Note that assembler scheduling is not enabled
  for XOS vector code (this can lead to code size increase and overflow
  of the vector code segments).

- XOS is now compatible with the hardware-based profiling library.

- XOS headers only define "bool" if the language is not C++ and stdbool.h
  is not available.


Notes for Version 1.12
----------------------

- A bug in xos_thread_get_stats() that would cause a crash if called with
  the special handle XOS_THD_STATS_IDLE has been fixed.

- A bug in xos_get_system_cycles() could cause the wrong number of cycles
  to be returned if a timer interrupt occurred at just the right point in
  the function. This has been fixed.

- There is now a way to suppress XOS debug print output when using the
  debug version of XOS. See the user guide for details.

- If a thread is blocked and waiting in a prioritized queue, changing its
  priority now correctly updates its position in the queue.


Notes for Version 1.11
----------------------

- XOS now supports block memory pool objects for fast allocation and freeing
  of fixed-size memory blocks. See the documentation for details.

- A bug in xos_thread_suspend() caused the action to fail if called to
  suspend the current thread. This has been fixed.

- The testsuite has been improved and new tests have been added. The basic
  test now supports Xtensa configurations with a single timer (some tests
  get disabled). The performance test output has been improved and it now
  measures more parameters.


Notes for Version 1.10
----------------------

- This version adds hooks for custom handling of the system timer, which
  includes support for external timers (timers outside the Xtensa core).
  XOS timekeeping still relies on the internal cycle counter. See the
  documentation for details.

- A conditional workaround for HW erratum 487 has been added. This is not
  enabled by default. This erratum is applicable only to a few specific
  HW configurations.

- A bug in coprocessor state save/restore handling has been fixed. This
  bug could cause data corruption in big-endian configurations that have
  one or more coprocessors.


Notes for Version 1.09
----------------------

- The build option XOS_OPT_MAIN_THREAD has been removed. This is replaced
  by a new function xos_start_main(). Calling xos_start_main() converts
  the calling function into the init thread, and returns control to the
  caller. Calling xos_start() now always starts the scheduler and does
  not return. See the user guide for more details.

- The XOS_TIMER_RECURRING option has been renamed to XOS_TIMER_PERIODIC.

- The API function xos_timer_reset() has been renamed to xos_timer_restart().


Notes for Version 1.08
----------------------

- One new parameter has been added to condition callback functions.
  This is the thread ID of the thread for which the evaluation is to
  be performed. This makes it easier to use a single function to check
  multiple threads.

- The default interrupt stack size is reduced to 32 bytes if interrupts
  are not configured. The stack still needs to exist since it is also
  used by the idle thread.

- xos_interrupt_enable() can now be called before xos_start() is called.
  The interrupts are recorded but not actually enabled until xos_start()
  is called.

- xos_preemption_enable() and xos_preemption_disable() can now be called
  before xos_start() has been called.


Notes for Version 1.07
----------------------

- A new option XOS_OPT_MAIN_THREAD has been added, which controls the
  behavior of xos_start(). If this option is enabled, then the call to
  xos_start() converts the calling function into the init thread, and
  causes xos_start() to return to its caller.
  If this option is disabled then xos_start() does not create the init
  thread and does not return to its caller. It calls the scheduler and
  begins multithreading. This is the default setting. See the user guide
  for more details.

- The source tree has been reorganized. The new structure is -

  xos
   |
   --- src (XOS source and header files)
   |
   --- testsuite (test and example files)
   |
   --- bin (object and executable files, generated by build)
   |
   --- include
         |
         --- xtensa (exported XOS header files, generated by build)

- Because of the change in the exported location of the XOS headers,
  application code must now include xos.h as follows -

      #include <xtensa/xos.h>

  instead of using <xtensa/xos/xos.h> for previous versions.

- Some unneeded files have been removed from the testsuite directory.
  A README file has been added for the testsuite directory.


Notes for Version 1.06
----------------------

- xos_get_system_cycles() was not computing the cycle count correctly.
  This has been fixed.

- Some performance degradation in the syslog module (due to recent
  changes) has been fixed.

- The syslog entries now use 32-bit timestamps instead of 64-bit.
  This saves space and time, and rollover can be detected since
  the oldest entry in the log is known at all times.

- xos_params.h has been modified to allow more build options to be
  overridden from command line.


Notes for Version 1.05
----------------------

- The stack layout has been reorganized. The space reserved for saving
  thread TIE state is now allocated at the top of the stack area instead
  of at the bottom. This makes it easier to detect stack overflow bugs.

- The TIE save area allocation now takes into account the thread creation
  flag XOS_THREAD_NO_CP. If this flag is specified, no save area will be
  allocated to save coprocessor state for that thread. This allows the
  application to allocate smaller stacks for such threads. Such a thread
  must never try to use any of the coprocessors.


Notes for Version 1.04
----------------------

- xos_disable_intlevel() renamed to xos_critical_enter().

- OS use of xos_restore_intlevel() replaced by xos_critical_exit().

- xos_set_intlevel() renamed to xos_set_int_pri_level().

- xos_restore_intlevel() renamed to xos_restore_int_pri_level().

- xos_get_intlevel() renamed to xos_get_int_pri_level().

- New functions xos_disable_interrupts() and xos_restore_interrupts().

- xos_enable_ints() replaced by xos_interrupt_enable(). The new call
  takes an interrupt number instead of an interrupt mask.

- xos_disable_ints() replaced by xos_interrupt_disable(). The new call
  takes an interrupt number instead of an interrupt mask.

- Option XOS_OPT_THREAD_ABORT has been removed.

- The thread exit handler function can now be specified in the thread
  creation parameters structure.

- The xos_syslog_write() function had a bug, which caused it to write
  to the log even if the log had been disabled. This has been fixed.


Notes for Version 1.03
----------------------

- Incorrect defines for XOS headers being included in C++ source files
  have been fixed.

- XOS configuration options have been renamed in the style XOS_OPT_XXX.

- Mutexes now support recursive locking by the same thread.

- Blocking wait functions are now implemented separately in two versions,
  with and without the timeout option. This is done for performance.

- An XOS_NULL value is now defined independent of the C library being
  used (if any). A 'bool' type is also defined.

- Callee-saved coprocessor registers are now supported.


Notes for Version 1.02
----------------------

- A bug in interrupt handling that could cause register corruption has 
  been fixed.

- A bug in interrupt dispatch that could result in interrupts of lower
  priority getting dispatched at higher priority levels has been fixed.

- Wait functions for mutexes, semaphores, message queues and events now
  have a timeout option. New API calls have been added to support the
  timeout. The older wait calls still remain and are supported.
  This feature is controlled by the option XOS_OPT_WAIT_TIMEOUT.

- A bug that would cause threads created in the suspended state to never
  get activated has been fixed.

- A new API call xos_get_cpu_load() has been added. This function computes
  the per-thread CPU use % (approximate). Please see documentation for
  more details.

- A new API call xos_thread_abort() has been added. Please see documentation
  for more details.

- A new API call xos_thread_get_state() has been added. Please see
  documentation for more details.

- A new API call xos_get_system_timer_num() has been added. This is useful
  when XOS has been allowed to select a timer automatically at startup time.
  Please see documentation for more details.

- The XOS error code range has been changed so that it is less likely to
  interfere with user return values. A new macro IS_XOS_ERRCODE() has been
  added to check if a value is a valid XOS return code or not.

- The thread exit handler can now be specified in thread parameters at create
  time.


Notes for Version 1.01
----------------------

- Fixed a bug in xos_thread_suspend(). If suspending the thread left no
  other ready threads at this priority level, then the priority mask was
  not being updated.

- All API calls that are not allowed to be called from interrupt context
  now have extra checking to enforce this restriction. Calling these from
  interrupt context will generate a fatal error.

- xos_start_ticking() has been renamed to xos_start_system_timer().


Notes for Version 1.00
----------------------

- First version circulated. This is a beta version, and the API is subject
  to change.

Supported Features
------------------

- Multithreading
- Supports both call0 and windowed ABIs
- Supports context-switching of coprocessor state
- Support for Timer/Mutex/Semaphore/Event/Message Queue/Event Log/Condition objects
- Support for Level-1, medium and high priority interrupts.
- Some support for run-to-completion threads (that can share a stack, etc); not
  as optimized as dedicated support.
- MIT style open source licensing
- Thread safety support for newlib and xclib

Issues
------

- Interrupt latency not fully optimized
- Coprocessor context switch not fully optimized
- Run-to-completion thread model not well tested
- No support for callee-saved registers

-End-

