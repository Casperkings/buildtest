/** @file */

// xos_timer.h - XOS Timer API interface and data structures.

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


#ifndef XOS_TIMER_H
#define XOS_TIMER_H

#include "xos_types.h"

#ifdef __cplusplus
extern "C" {
#endif


//-----------------------------------------------------------------------------
///
/// Function pointer type for timer callbacks.
///
//-----------------------------------------------------------------------------
typedef void (XosTimerFunc)(void * arg);


//-----------------------------------------------------------------------------
///
/// Timer event structure. Used to track pending timer events.
///
//-----------------------------------------------------------------------------
typedef struct XosTimer {
    struct XosTimer *   next;   ///< Pointer to next event in list.
    XosTimerFunc *      fn;     ///< Function to call when timer expires.
    uint64_t            when;   ///< Time (clock cycles) at which to trigger.
    uint64_t            delta;  ///< Delta for next re-trigger, 0 if none.
    void *              arg;    ///< Argument to pass to called function.
    bool                active; ///< Set if active (in some list of events).
#if XOS_OPT_TIMER_WAIT
    XosThreadQueue      waitq;  ///< Queue of threads waiting on this timer.
#endif
    uint32_t            sig;
} XosTimer;


//-----------------------------------------------------------------------------
// Extern declarations.
//-----------------------------------------------------------------------------

// System clock frequency in cycles per second.
extern uint32_t xos_clock_freq;


///@{
//-----------------------------------------------------------------------------
// Functions to convert from clock cycles to time units and vice versa.
//
// Note that these are integer conversions so for example a cycle count of less
// than one second will convert to zero seconds.
//-----------------------------------------------------------------------------

///  Converts CPU cycles to time in seconds.
///
///  \param     cycles          Number of CPU cycles.
///
///  \return    Equivalent number of seconds (truncated to integer).
__attribute__((always_inline))
static inline uint64_t
xos_cycles_to_secs(uint64_t cycles)
{
    return cycles / xos_clock_freq;
}

/// Converts CPU cycles to time in milliseconds.
///
///  \param     cycles          Number of CPU cycles.
///
///  \return    Equivalent number of milliseconds (truncated to integer).
__attribute__((always_inline))
static inline uint64_t
xos_cycles_to_msecs(uint64_t cycles)
{
    return (cycles * 1000UL) / xos_clock_freq;
}

/// Converts CPU cycles to time in microseconds.
///
///  \param     cycles          Number of CPU cycles.
///
///  \return    Equivalent number of microseconds (truncated to integer).
__attribute__((always_inline))
static inline uint64_t
xos_cycles_to_usecs(uint64_t cycles)
{
    return (cycles * 1000000UL) / xos_clock_freq;
}

/// Converts time in seconds to CPU cycle count.
///
///  \param     secs            Number of seconds.
///
///  \return    Equivalent number of CPU cycles.
__attribute__((always_inline))
static inline uint64_t
xos_secs_to_cycles(uint64_t secs)
{
    return secs * xos_clock_freq;
}

/// Converts time in milliseconds to CPU cycle count.
///
///  \param     msecs           Number of milliseconds.
///
///  \return    Equivalent number of CPU cycles.
__attribute__((always_inline))
static inline uint64_t
xos_msecs_to_cycles(uint64_t msecs)
{
    return (msecs * xos_clock_freq) / 1000UL;
}

/// Converts time in microseconds to CPU cycle count.
///
///  \param     usecs           Number of microseconds.
///
///  \return    Equivalent number of CPU cycles.
__attribute__((always_inline))
static inline uint64_t
xos_usecs_to_cycles(uint64_t usecs)
{
    return (usecs * xos_clock_freq) / 1000000UL;
}
///@}


//-----------------------------------------------------------------------------
///
///  Set system clock frequency. This is expected to be set only once, and only
///  if the clock frequency is not known at compile time. If this function is
///  called, it must be called before selecting and starting the system timer.
///
///  \param     freq            Frequency in cycles per second.
///
///  \return    Returns nothing.
///
//-----------------------------------------------------------------------------
void
xos_set_clock_freq(uint32_t freq);


//-----------------------------------------------------------------------------
///
///  Get current system clock frequency.
///
///  \return    Returns current system clock frequency in cycles per second.
///
//-----------------------------------------------------------------------------
__attribute__((always_inline))
static inline uint32_t
xos_get_clock_freq(void)
{
    return xos_clock_freq;
}


//-----------------------------------------------------------------------------
///
///  Initialize timer support and start the system timer.
///  This function must be called before calling any other timer function.
///  The system clock frequency must have been specified before this function
///  is called.
///
///  NOTE: The smaller the tick period, the more precisely delays can be
///  specified using timers.  However, we also need to make the tick period
///  large enough to allow time both to execute the tick timer interrupt handler
///  and for the application to make reasonable forward progress.  If tick_period
///  is too small, the timer interrupt may re-trigger before the timer interrupt
///  handler has returned to the application, thus keeping the processor busy in
///  constantly executing the timer interrupt handler without leaving any cycles
///  for the application. Or, the application might get some cycles but only a 
///  fraction of what is spent in the timer interrupt handler, thus severely
///  impacting application performance.
///
///  The exact number of cycles needed to execute the timer interrupt handler
///  is not specified here.  It depends on many factors (e.g. use of caches, 
///  various processor configuration options, etc) and can vary by orders of
///  magnitude.  Also note that the time to execute this handler is variable:
///  when timers expire upon a given tick timer interrupt, their respective
///  timer handler functions are called from within the interrupt handler.
///
///  \param     timer_num       Which Xtensa timer to use (0..2). This timer
///                             must exist and be configured at level 1 or at a
///                             medium-priority interrupt level (<=EXCM_LEVEL).
///                             If 'timer_num' is -1, then this function will
///                             automatically choose the highest priority timer
///                             that is suitable for use. This value will be
///                             passed to xos_system_timer_select().
///
///  \param     tick_period     Number of clock (CCOUNT) cycles between ticks.
///                             Must range between 0 and UINT32_MAX.
///                             Zero is used to specify dynamic tick (tickless)
///                             mode.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_start_system_timer(int32_t timer_num, uint32_t tick_period);


//-----------------------------------------------------------------------------
///
///  Initialize timer support and start the system timer, extended version.
///  This function must be called before calling any other timer function.
///  The system clock frequency must have been specified before this function
///  is called.
///
///  The behavior of this function is the same as xos_start_system_timer(), 
///  with the addition that this function allows the tick_delta to be set
///  directly. The tick_delta value is used as follows: when scheduling the
///  next timer tick, the timer module always ensures that the next tick is
///  at least tick_delta cycles in the future. For dynamic tick mode this is
///  a simple adjustment to the next tick time. For fixed-duration tick mode
///  if the next tick is due in less than tick_delta cycles, the tick is 
///  skipped and the next one following is scheduled.
///
///  \param     timer_num       Which Xtensa timer to use (0..2). This timer
///                             must exist and be configured at level 1 or at a
///                             medium-priority interrupt level (<=EXCM_LEVEL).
///                             If 'timer_num' is -1, then this function will
///                             automatically choose the highest priority timer
///                             that is suitable for use. This value will be
///                             passed to xos_system_timer_select().
///
///  \param     tick_period     Number of clock (CCOUNT) cycles between ticks.
///                             Must range between 0 and UINT32_MAX.
///                             Zero is used to specify dynamic tick (tickless)
///                             mode.
///
///  \param     tick_delta      Number of clock (CCOUNT) cycles to use as the
///                             tick delta. If tick_period is nonzero, then this
///                             must be less than half the tick period. A value
///                             of zero is used to indicate use of the default
///                             value. The default is the clock frequency / 1000
///                             or 1000 cycles, whichever is larger.
///  
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_start_system_timer_ex(int32_t  timer_num,
                          uint32_t tick_period,
                          uint32_t tick_delta);


//-----------------------------------------------------------------------------
///
///  Get the timer number of the system timer. Useful mainly when XOS has been
///  allowed to choose its own timer via xos_start_system_timer(). Not valid if
///  called before the system timer has been started.
///
///  \return    Returns one of XOS_SYS_TIMER_0, XOS_SYS_TIMER_1, XOS_SYS_TIMER_2
///             or XOS_SYS_TIMER_EXTERNAL, or XOS_SYS_TIMER_NONE.
///
//-----------------------------------------------------------------------------
int32_t
xos_get_system_timer_num(void);


//-----------------------------------------------------------------------------
///
///  Initialize timer object.
///
///  \param     timer           Pointer to timer event structure.
///
///  \return    Returns XOS_OK on success, else error code.
///
///  NOTE: This function should not be called on a timer object once it has 
///  been activated.
///
//-----------------------------------------------------------------------------
int32_t
xos_timer_init(XosTimer * timer);


//-----------------------------------------------------------------------------
//  Flags for xos_timer_start().
//-----------------------------------------------------------------------------
#define XOS_TIMER_DELTA         0x0000U
#define XOS_TIMER_PERIODIC      0x0001U
#define XOS_TIMER_ABSOLUTE      0x0002U
#define XOS_TIMER_FROM_NOW      0x0000U
#define XOS_TIMER_FROM_LAST     0x0010U


//-----------------------------------------------------------------------------
///
///  Start the timer, and when the timer expires, call the specified function
///  (invoke (*fn)(arg)). If the timer is periodic, it will be automatically
///  restarted when it expires.
///
///  The specified timer event structure must have been initialized before
///  first use by calling xos_timer_init().
///
///  The callback function will be called in an interrupt context. Hence it is
///  NOT safe to use any coprocessors in the function, including the FPU. If a
///  coprocessor must be used, then its state must be saved and restored across
///  its use.
///
///  NOTE: If you are using the timer only to wait on (via xos_timer_wait())
///  then it is not necessary to specify a callback function. You should pass
///  NULL for the callback function and zero for the callback argument.
///
///  \param     timer           Pointer to timer event structure. Must have been
///                             initialized. May be active or not.
///
///  \param     when            When to call the function (see flags).
///
///  \param     flags           Set of option flags XOS_TIMER_* \n
///                             The following flags are mutually exclusive:
///                             - XOS_TIMER_DELTA -- when is number of cycles from
///                               [see below] (default)
///                             - XOS_TIMER_PERIODIC -- when is number of cycles
///                               from [see below], and timer continually
///                               re-triggers at that interval
///                             - XOS_TIMER_ABSOLUTE -- when is absolute value of
///                               cycle count \n
///  \n
///                             The following flags are mutually exclusive:
///                             - XOS_TIMER_FROM_NOW -- *DELTA and *PERIODIC are
///                               relative to now (default)
///                             - XOS_TIMER_FROM_LAST -- *DELTA and *PERIODIC are
///                               relative to the timer event's last specified expiry
///                               time (usually in the future if active, in the past
///                               if not, absolute 0 if was never activated).
///
///  \param     func            Function to call (called in timer interrupt context).
///                             This argument is optional. Specify NULL if no function
///                             is to be called.
///
///  \param     arg             Argument passed to callback function. Only relevant if
///                             'func' is not NULL.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_timer_start(XosTimer *     timer,
                uint64_t       when,
                uint32_t       flags,
                XosTimerFunc * func,
                void *         arg);


//-----------------------------------------------------------------------------
///
///  Stop the timer and remove it from the list of active timers. Has no effect
///  if the timer is not active. Any waiting threads are woken up.
/// 
///  The timer structure must have been initialized at least once, else its
///  contents are undefined and can lead to unpredictable results.
///
///  \param     timer           Pointer to timer object.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_timer_stop(XosTimer * timer);


//-----------------------------------------------------------------------------
///
///  Reset and restart the timer.
///
///  The timer is reset to go off at time "when" from now. If the timer was not
///  active, it will be activated. If the timer was active, it will be restarted.
///  If the timer is periodic, the period will be set to "when".
///  The timer object must have been initialized at some point before this call.
///
///  \param     timer           Pointer to timer object.
///
///  \param     when            Number of cycles from now that the timer will expire.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_timer_restart(XosTimer * timer, uint64_t when);


//-----------------------------------------------------------------------------
///
///  Check if the timer is active. The timer is active if it has been started
///  and not yet expired or canceled.
///
///  \param     timer           Pointer to timer object.
///
///  \return    Returns non-zero if the timer is active, else zero.
///
//-----------------------------------------------------------------------------
__attribute__((always_inline))
static inline int32_t
xos_timer_is_active(const XosTimer * timer)
{
    return (timer != XOS_NULL) ? timer->active : 0;
}


//-----------------------------------------------------------------------------
///
///  Get the repeat period for a periodic timer. For a one-shot timer this will
///  return zero. The period is reported in system clock cycles.
///
///  \param     timer           Pointer to timer object.
///
///  \return    Returns period in cycles, or zero for non-periodic timers.
///
//-----------------------------------------------------------------------------
__attribute__((always_inline))
static inline uint64_t
xos_timer_get_period(const XosTimer * timer)
{
    return (timer != XOS_NULL) ? timer->delta : 0UL;
}


//-----------------------------------------------------------------------------
///
///  Set the repeat period for a periodic timer. The period must be specified
///  in system clock cycles.
///
///  If the timer is active, the change in period does not take effect until 
///  the timer expires at least once after this call.
///  Note that setting a period of zero will effectively turn a periodic timer
///  into a one-shot timer. Similarly, a one-shot timer can be turned into a
///  periodic timer.
///
///  \param     timer           Pointer to timer object.
///
///  \param     period          Repeat period in system clock cycles.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_timer_set_period(XosTimer * timer, uint64_t period);


//-----------------------------------------------------------------------------
///
///  Get the current system cycle count. This accounts for the periodic rollover
///  of the 32-bit CCOUNT cycle counter and returns a 64-bit value.
///
///  \return    Returns the current system cycle count.
///
//-----------------------------------------------------------------------------
__attribute__((always_inline))
static inline uint64_t
xos_get_system_cycles(void)
{
    extern uint64_t xos_system_cycles;
    extern uint32_t xos_last_ccount;

    uint32_t ps;
    uint32_t lastcc;
    uint64_t cycles;

    // Prevent the timer interrupt handler from changing things while we read.
#pragma no_reorder
    ps = xos_disable_interrupts();
    cycles = xos_system_cycles;
    lastcc = xos_last_ccount;
    xos_restore_interrupts(ps);

    // xos_last_ccount was updated when xos_system_cycles was last updated.
    // We need to add in the number of cycles elapsed since then.
    return cycles + (uint64_t)(xos_get_ccount() - lastcc); // parasoft-suppress MISRAC2012-RULE_10_8-a-2 "Type conversion necessary here"
}


//-----------------------------------------------------------------------------
///
///  Put calling thread to sleep for at least the specified number of cycles.
///  The actual number of cycles spent sleeping may be larger depending upon
///  the granularity of the system timer. Once the specified time has elapsed
///  the thread will be woken and made ready.
///
///  \param     cycles          Number of system clock cycles to sleep.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_thread_sleep(uint64_t cycles);


//-----------------------------------------------------------------------------
///
///  Put calling thread to sleep for at least the specified number of msec.
///  The actual amount of time spent sleeping may be larger depending upon
///  the granularity of the system timer. Once the specified time has elapsed
///  the thread will be woken and made ready.
///
///  \param     msecs           The number of milliseconds to sleep.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
__attribute__((always_inline))
static inline int32_t
xos_thread_sleep_msec(uint64_t msecs)
{
    return xos_thread_sleep(xos_msecs_to_cycles(msecs));
}


//-----------------------------------------------------------------------------
///
///  Put calling thread to sleep for at least the specified number of usec.
///  The actual amount of time spent sleeping may be larger depending upon
///  the granularity of the system timer. Once the specified time has elapsed
///  the thread will be woken and made ready.
///
///  \param     usecs           The number of microseconds to sleep.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
__attribute__((always_inline))
static inline int32_t
xos_thread_sleep_usec(uint64_t usecs)
{
    return xos_thread_sleep(xos_usecs_to_cycles(usecs));
}


//-----------------------------------------------------------------------------
///
///  Wait on a timer until it expires or is cancelled. The calling thread will
///  be blocked. The timer must be active.
///  NOTE: This operation is only available if XOS_OPT_TIMER_WAIT is set
///  to 1 in the configuration options.
///
///  \param     timer           Pointer to timer object.
///
///  \return    Returns XOS_OK on normal timeout, else an error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_timer_wait(XosTimer * timer);


//-----------------------------------------------------------------------------
// System timer control interface.
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Defines for system timer ID.
//-----------------------------------------------------------------------------
#define XOS_SYS_TIMER_0             0    ///< Internal timer 0
#define XOS_SYS_TIMER_1             1    ///< Internal timer 1
#define XOS_SYS_TIMER_2             2    ///< Internal timer 2
#define XOS_SYS_TIMER_EXTERNAL      -2   ///< External timer
#define XOS_SYS_TIMER_NONE          -1   ///< No system timer selected


//-----------------------------------------------------------------------------
///
///  This function handles XOS timer tick processing. It must be called by the
///  timer interrupt handler on every timer interrupt. This function computes
///  the time to the next tick and sets it up by calling xos_system_timer_set().
///
//-----------------------------------------------------------------------------
void
xos_tick_handler(void);


//-----------------------------------------------------------------------------
///
///  Selects the timer to use. The selection may be one of the internal timers
///  or an external timer. The default implementation selects an internal timer.
///  This function can be overridden to provide custom timer processing or to
///  support an external timer.
///
///  \param     timer_num       The internal timer number to select (0-2) or
///                             -1 to auto-select a timer. This parameter can
///                             be ignored by custom implementations that use
///                             an external timer.
///
///  \param     psel            Pointer to a location where the selected timer
///                             ID must be returned. The timer ID must be one
///                             of XOS_SYS_TIMER_0, XOS_SYS_TIMER_1, XOS_SYS_TIMER_2
///                             or XOS_SYS_TIMER_EXTERNAL.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_system_timer_select(int32_t timer_num, int32_t *psel);


//-----------------------------------------------------------------------------
///
///  Starts the system timer and sets up the first interrupt. This function can
///  be overridden to provide custom timer processing or to support an external
///  timer.
///
///  \param     cycles          The number of CPU cycles from now when the
///                             first interrupt must occur.
///
///  \return    Returns cycle number modulo 2^32 when first interrupt will occur.
///
//-----------------------------------------------------------------------------
uint32_t
xos_system_timer_init(uint32_t cycles);


//-----------------------------------------------------------------------------
///
///  Sets the next trigger value of the system timer. The parameter 'cycles' is
///  a cycle count delta, with the base value being specified by the 'from_now'
///  parameter.
///  This function can be overridden to provide custom timer processing or to
///  support an external timer.
///
///  \param     cycles          The number of CPU cycles from the base when the
///                             next interrupt must occur.
///
///  \param     from_now        If true, 'cycles' is the delta from now.
///                             If false, 'cycles' is the delta from the last
///                             set value.
///
///  \return    Returns cycle number modulo 2^32 when next interrupt will occur.
///
//-----------------------------------------------------------------------------
uint32_t
xos_system_timer_set(uint32_t cycles, bool from_now);


//-----------------------------------------------------------------------------
///
///  Reads the current value of the system timer. The value returned is in
///  CPU cycles. This function can be overridden to provide custom timer
///  processing or to support an external timer.
///
///  \return    Returns the current value of the system timer, i.e. the count
///             at which the next timer interrupt will trigger.
///
//-----------------------------------------------------------------------------
uint32_t
xos_system_timer_get(void);


//-----------------------------------------------------------------------------
///
///  Returns true if the system timer has expired and is pending. This function
///  can be overridden to provide custom timer processing or to support an
///  external timer.
///
//-----------------------------------------------------------------------------
bool
xos_system_timer_pending(void);


#ifdef __cplusplus
}
#endif

#endif  // XOS_TIMER_H

