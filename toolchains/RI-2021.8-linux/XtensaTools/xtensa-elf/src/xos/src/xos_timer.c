
// xos_timer.c - Timer handling and timing services.

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


#include <xtensa/config/core.h>

#define XOS_INCLUDE_INTERNAL
#include "xos.h"
#include "xos_regaccess.h"


#define XOS_TIMER_SIG     0x74696d72U // Signature of valid object


//-----------------------------------------------------------------------------
// Maximum value reachable by CCOUNT register.
//-----------------------------------------------------------------------------
#define XOS_CCOUNT_MAX    (0xFFFFFFFFU)

//-----------------------------------------------------------------------------
// Timer thread TCB and stack.
//-----------------------------------------------------------------------------
#if XOS_OPT_TIMER_THREAD
static XosThread xos_timer_thread;
static uint8_t   xos_timer_stack[XOS_TIMER_THREAD_STACK_SIZE];
#endif

//-----------------------------------------------------------------------------
// System clock frequency (cycles per second). Can't run faster than ~4GHz for
// now. Set to the default clock frequency at start. This can be set by calling
// xos_set_clock_freq() at run time or defining XOS_CLOCK_FREQ in xos_params.h
// at build time.
//-----------------------------------------------------------------------------
uint32_t xos_clock_freq = XOS_DEFAULT_CLOCK_FREQ;

//-----------------------------------------------------------------------------
// The system tick period in clock cycles. This is valid only if the system
// timer tick is enabled.
//-----------------------------------------------------------------------------
static uint32_t xos_tick_period;

//-----------------------------------------------------------------------------
// Number of system timer ticks per time slice. Valid only if time slicing is
// enabled.
//-----------------------------------------------------------------------------
#if XOS_OPT_TIME_SLICE
static uint8_t xos_ticks_per_slice = XOS_TIME_SLICE_TICKS;
#endif

//-----------------------------------------------------------------------------
// Timer number being used by the system.
//-----------------------------------------------------------------------------
static int32_t xos_timer_num = -1;

//-----------------------------------------------------------------------------
// Flag to keep track of dynamic tick mode.
//-----------------------------------------------------------------------------
static bool xos_use_dynamic_ticks;

//-----------------------------------------------------------------------------
// Minimum delta used for scheduling timer ticks.
//-----------------------------------------------------------------------------
static uint32_t xos_min_delta = 1000U;

//-----------------------------------------------------------------------------
// Track system tick count. This is valid only if the system timer tick is
// enabled.
//-----------------------------------------------------------------------------
static uint64_t xos_system_ticks;

//-----------------------------------------------------------------------------
// Track system cycle count. This is valid if timer services are enabled.
// We need to remember the last read value of ccount to be able to do this.
// Note this starts tracking only from the time that xos_start_system_timer()
// is called, so is not an exact reflection of ccount.
//-----------------------------------------------------------------------------
uint64_t xos_system_cycles;


//-----------------------------------------------------------------------------
// We need to remember the last read value of ccount to be able to track the
// system cycle count across CCOUNT rollovers.
// Note this starts tracking only from the time that xos_start_system_timer()
// is called, so is not an exact reflection of ccount.
//-----------------------------------------------------------------------------
uint32_t xos_last_ccount;


//-----------------------------------------------------------------------------
// The value of CCOUNT at which the next timer tick will occur. This is kept
// up to date whether fixed or dynamic ticks are used.
//-----------------------------------------------------------------------------
static uint32_t xos_next_ccount;


//-----------------------------------------------------------------------------
// List of pending timer events.
//-----------------------------------------------------------------------------
static XosTimer * xos_timer_list;


//-----------------------------------------------------------------------------
//  Internal function to insert timer in event list. Insert at time "delta"
//  ticks from now. Must be called inside an OS critical section.
//-----------------------------------------------------------------------------
static inline void
xos_insert_timer(XosTimer * timer, uint64_t delta)
{
    XosTimer ** last = &xos_timer_list;
    XosTimer *  e    = xos_timer_list;

    XOS_ASSERT(timer != XOS_NULL);

    // Insert into timer list based on absolute expiry time.
    while ((e != XOS_NULL) && (e->when < timer->when)) {
        last = &(e->next);
        e = e->next;
    }

    timer->next = e;
    *last = timer;

    // Sanity check.
    if ((timer->next != XOS_NULL) && (timer->next->when < timer->when)) {
        xos_fatal_error(XOS_ERR_INTERNAL_ERROR, XOS_NULL);
    }

    // If dynamic ticks are enabled then check to see if the next tick needs
    // to be rescheduled. If the tick is too far out, it must be pulled in.
    // Allow some slack so as not to schedule two ticks close together.

    if (xos_use_dynamic_ticks) {
        uint32_t next = xos_next_ccount - xos_get_ccount();

        if (next <= (delta + xos_min_delta)) {
            // Next tick will happen before timer is scheduled to expire
            // so we're good here.
        }
        else if (xos_system_timer_pending() == true) {
            // If the timer is pending then we don't want to update the next
            // tick because the update might clear the pending interrupt and
            // cause the pending event to disappear.
        }
        else {
            // Next tick is too far out, bring it back in for this timer.
            // Since 'next' can't be more than a 32-bit value, and delta
            // is smaller, 'delta' must fit in 32 bits.
            xos_next_ccount = xos_system_timer_set((uint32_t) delta, true);
        }
    }
}


//-----------------------------------------------------------------------------
//  Internal function to process timer list. Called by tick handler or by timer
//  thread if enabled.
//-----------------------------------------------------------------------------
static void
xos_timer_process(void)
{
    XosTimer * e;

    // Walk the list of active events. This is sorted by time of expiry
    // so we'll stop as soon as we find one with time > current time.
    e = xos_timer_list;
    while (e != XOS_NULL) {
        uint32_t ps;

        XOS_ASSERT(e->sig == XOS_TIMER_SIG);

        // Use the most current value for system cycles on every pass
        // through this loop. If a timer expires while we are in this
        // loop we do want to process it right away.
        if (e->when > xos_get_system_cycles()) {
            break;
        }

        ps = xos_critical_enter();

        // Remove from active list
        xos_timer_list = e->next;
        e->next = XOS_NULL;

        // Reschedule if periodic
        if (e->delta > 0ULL) {
            e->when += e->delta;
            xos_insert_timer(e, e->delta);
        }
        else {
            e->active = 0;
        }

        xos_critical_exit(ps);

        // Invoke event handler callback
        if (e->fn != XOS_NULL) {
            (*e->fn)(e->arg);
        }

#if XOS_OPT_TIMER_WAIT
        // Wake any waiting threads. The wake function will return with
        // preemption disabled.
        if (e->waitq.head != XOS_NULL) {
            (void) xos_wake_queue(&(e->waitq), xos_blkon_delay, XOS_OK);
            (void) xos_preemption_enable();
        }
#endif
        e = xos_timer_list;
    }
}


//-----------------------------------------------------------------------------
//  Timer thread main function. Simply calls the event processor every time it
//  wakes and then goes back to sleep.
//-----------------------------------------------------------------------------
#if XOS_OPT_TIMER_THREAD
static int32_t
xos_timer_main(void * arg, int32_t unused)
{
    int32_t ret = XOS_OK;

    UNUSED(arg);
    UNUSED(unused);

    while (ret == XOS_OK) {
        xos_preemption_disable();
        xos_timer_process();
        xos_preemption_enable();

        ret = xos_thread_suspend(XOS_THREAD_SELF);
    }

    // If we get here something went wrong.
    xos_fatal_error(XOS_ERR_INTERNAL_ERROR, "Timer thread");

    return ret;
}
#endif


//-----------------------------------------------------------------------------
//  Create the timer thread, if specified. The timer thread is created at the
//  highest available priority, and is created initially suspended. It will be
//  resumed by the timer tick handler and will suspend itself again when done.
//-----------------------------------------------------------------------------
int32_t
xos_create_timer_thread(void)
{
#if XOS_OPT_TIMER_THREAD
    return xos_thread_create(&xos_timer_thread,
                             XOS_NULL,
                             &xos_timer_main,
                             XOS_NULL,
                             "timer",
                             xos_timer_stack,
                             XOS_TIMER_THREAD_STACK_SIZE,
                             XOS_MAX_PRIORITY - 1,
                             XOS_NULL,
                             XOS_THREAD_SUSPEND);
#else
    return XOS_OK;
#endif
}


//-----------------------------------------------------------------------------
//  Process events on timer tick. This is called by xos_tick_handler() on each
//  timer tick.
//-----------------------------------------------------------------------------
static void
xos_tick_process(void)
{
#if XOS_OPT_TIME_SLICE
    static uint8_t slice_tick_cnt = 0;
#endif
    uint32_t   now   = xos_get_ccount();
    uint32_t   delta = now - xos_last_ccount;

    if (now < xos_last_ccount) {
        DPRINTF("CCOUNT rollover\n");
    }

    // Update tick counts. Multiple tick intervals may have gone by.
    if (!xos_use_dynamic_ticks) {
        while (delta >= xos_tick_period) {
            xos_system_ticks++;
#if XOS_OPT_TIME_SLICE
            slice_tick_cnt++;
#endif
            delta -= xos_tick_period;
        }
    }

    // Update system cycle count.
    xos_system_cycles += xos_next_ccount - xos_last_ccount;
    xos_last_ccount = xos_next_ccount;

#if XOS_OPT_TIME_SLICE
    // Check if the time slice is over and a thread switch is necessary.
    // We call this *before* processing the event list because the event
    // handler functions might wake up other threads which we don't want
    // to time slice.
    // If dynamic ticks enabled, there is no tick period, so this
    // is not really useful.
    if (slice_tick_cnt >= xos_ticks_per_slice) {
        slice_tick_cnt = 0;
        DPUTCHAR('#');
        xos_thread_yield();
    }
#endif
}


//-----------------------------------------------------------------------------
//  System timer interrupt callback. This function must be called on every
//  system timer tick.
//-----------------------------------------------------------------------------
void
xos_tick_handler(void)
{
    uint32_t check;
    uint32_t set;
    uint32_t ps;
    bool     fn;

    xos_log_sysevent(XOS_SE_TIMER_INTR_START,
                     0U,
                     0U);
    DPUTCHAR('<');

    // Set up the next timer tick. This section must be protected from
    // interrupts otherwise higher-priority handlers might manipulate
    // timers and interfere with the process.
    ps = xos_critical_enter();

    // Update tick counts etc.
    xos_tick_process();

    if (xos_use_dynamic_ticks) {
        // Find first timer with expiry time in the future.
        XosTimer * timer     = xos_timer_list;
        uint64_t   now       = xos_system_cycles;
        uint64_t   delta;

        while (timer != XOS_NULL) {
            if (timer->when > now) {
                break;
            }
            timer = timer->next;
        }

        if (timer == XOS_NULL) {
            // No timer found, set next interrupt as far out as we can
            set = XOS_CCOUNT_MAX;
        }
        else {
            // Found a timer, set up for it. However, we don't want to
            // trigger another interrupt too soon, so we'll impose a
            // minimum delta. If the delta is too large for CCOUNT to
            // handle then we'll cap at the max that can be handled.
            if ((timer->when - now) < xos_min_delta) {
                delta = xos_min_delta;
            }
            else {
                delta = timer->when - now;
            }

            if (delta > XOS_CCOUNT_MAX) {
                delta = XOS_CCOUNT_MAX;
            }

            set = (uint32_t) delta;
        }

        fn = true;
    }
    else {
        // Handle fixed-tick operation, and keep track of tick count
        uint32_t period = xos_tick_period;
        uint32_t diff;

        // Figure out how many ticks have gone by
        set = 0U;
        do {
            diff = xos_get_ccount() - xos_next_ccount;
            xos_next_ccount += period;
            set += period;
        } while ( diff > period );

        // If the next tick deadline is too close, skip it and schedule
        // the next one, avoid a race between the set and the deadline.
        if ((period - diff) < xos_min_delta) {
            xos_next_ccount += period;
            set += period;
        }

        fn = false;
    }

    // Set timer for next interrupt. Check that setting the timer does not
    // take more than xos_min_delta cycles.
    check = xos_get_ccount();
    xos_next_ccount = xos_system_timer_set(set, fn);
    check = xos_get_ccount() - check;
    xos_last_ccount = xos_next_ccount - set;

    if (check > xos_min_delta) {
        xos_fatal_error(XOS_ERR_LIMIT, "Timer update");
    }

    // Re-enable interrupts.
    xos_critical_exit(ps);

    // If a timer thread is available then resume it to process timer events.
    // If not, process events directly.
#if XOS_OPT_TIMER_THREAD
    ps = xos_critical_enter();

    // Wake timer thread only if there is work to be done, i.e. at least one
    // event is ready to be processed.
    if ((xos_timer_list != XOS_NULL) && (xos_timer_list->when <= xos_system_cycles)) {
        xos_thread_wake(&xos_timer_thread, XOS_NULL, 0);
    }

    xos_critical_exit(ps);
#else
    xos_timer_process();
#endif

    DPUTCHAR('>');
    xos_log_sysevent(XOS_SE_TIMER_INTR_END,
                     0U,
                     0U);
}


//-----------------------------------------------------------------------------
//  Set up periodic or dynamic timer tick for system.
//
//  "timer_num"     - Timer ID to use for system timer.
//  "tick_period"   - Timer tick period in clock cycles, 0 if dynamic.
//-----------------------------------------------------------------------------
int32_t
xos_start_system_timer(int32_t timer_num, uint32_t tick_period)
{
    uint32_t set_val;
    int32_t  sel;
    int32_t  ret;

    // Can't call this from interrupt context.
    if (INTERRUPT_CONTEXT) {
        xos_fatal_error(XOS_ERR_INTERRUPT_CONTEXT, XOS_NULL);
    }

    // Initialize if needed, in case this is called very early.
    xos_init();

    // Select the timer to use, internal or external.
    ret = xos_system_timer_select(timer_num, &sel);
    if (ret != XOS_OK) {
        return ret;
    }

    xos_tick_period = tick_period;
    xos_timer_num   = sel;

    // Select tick period or dynamic ticking.
    if (tick_period == 0U) {
        xos_use_dynamic_ticks = 1;
        set_val = XOS_CCOUNT_MAX;
    }
    else {
        xos_use_dynamic_ticks = 0;
        set_val = tick_period;
    }

#if XOS_OPT_TIME_SLICE
    // Check bounds for time slice interval.
    if (xos_ticks_per_slice < 1U) {
        xos_ticks_per_slice = 1U;
    }
    if (xos_ticks_per_slice > 50U) {
        xos_ticks_per_slice = 50U;
    }
#endif

    // Start the timer and init the next ccount value.
    xos_next_ccount = xos_system_timer_init(set_val);

    // Init the last ccount value for system cycle count tracking.
    xos_last_ccount = xos_next_ccount - set_val;

    DPRINTF("System timer is timer %d\n", xos_timer_num);
    return XOS_OK;
}


//-----------------------------------------------------------------------------
//  Enhanced version of system timer start, also specifying tick delta.
//
//  "timer_num"     - Timer ID to use for system timer.
//  "tick_period"   - Timer tick period in clock cycles, 0 if dynamic.
//  "tick_delta"    - Timer tick delta in clock cycles, 0 if default.
//-----------------------------------------------------------------------------
int32_t
xos_start_system_timer_ex(int32_t timer_num, uint32_t tick_period, uint32_t tick_delta)
{
    int32_t  ret;
    uint32_t delta;

    // Ensure sane value for delta.
    if (tick_delta == 0U) {
        delta = tick_period / 10U;
    }
    else if ((tick_period > 0U) && (tick_delta >= (tick_period/2U))) {
        return XOS_ERR_INVALID_PARAMETER;
    }
    else {
        delta = tick_delta;
    }

    ret = xos_start_system_timer(timer_num, tick_period);

    if (ret == XOS_OK) {
        xos_min_delta = ((delta == 0U) ? (xos_clock_freq / 1000U) : delta);
    }

    return ret;
}


//-----------------------------------------------------------------------------
//  Return system timer number, if available.
//-----------------------------------------------------------------------------
int32_t
xos_get_system_timer_num(void)
{
    return xos_timer_num;
}


//-----------------------------------------------------------------------------
//  Set system clock frequency. Set min_delta to a reasonable matching value.
//-----------------------------------------------------------------------------
void
xos_set_clock_freq(uint32_t freq)
{
    xos_clock_freq = (freq == 0U) ? XOS_DEFAULT_CLOCK_FREQ : freq;

    xos_min_delta = freq / 1000U;
    if (xos_min_delta < 1000U) {
        xos_min_delta = 1000U;
    }
}


//-----------------------------------------------------------------------------
//  Init the timer module.
//-----------------------------------------------------------------------------
void
xos_timer_module_init(void)
{
    if (xos_clock_freq == 0U) {
        xos_clock_freq = XOS_DEFAULT_CLOCK_FREQ;
    }
    xos_tick_period   = 0;
    xos_system_ticks  = 0;
    xos_system_cycles = 0;
}


//-----------------------------------------------------------------------------
//  Timer API functions.
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//  Initialize timer object.
//-----------------------------------------------------------------------------
int32_t
xos_timer_init(XosTimer * timer)
{
    if (timer != XOS_NULL) {
        timer->when   = 0;
        timer->delta  = 0;
        timer->fn     = XOS_NULL;
        timer->arg    = XOS_NULL;
        timer->active = 0;
        timer->next   = XOS_NULL;
        timer->sig    = XOS_TIMER_SIG;

#if XOS_OPT_TIMER_WAIT
        timer->waitq.head = XOS_NULL;
        timer->waitq.tail = &(timer->waitq.head);
#endif
        return XOS_OK;
    }

    return XOS_ERR_INVALID_PARAMETER;
}


//-----------------------------------------------------------------------------
//  Set up the timer object and activate it.
//-----------------------------------------------------------------------------
int32_t
xos_timer_start(XosTimer *     timer,
                uint64_t       when,
                uint32_t       flags,
                XosTimerFunc * func,
                void *         arg)
{
    uint32_t ps;
    uint64_t syswhen;
    uint64_t event_when;

    if ((timer == XOS_NULL) || (timer->sig != XOS_TIMER_SIG)) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    // If the timer is active, disable it and remove from timer list
    if (timer->active) {
        // Nothing useful we can do with the return code here
        (void) xos_timer_stop(timer);
    }

    // Insert atomically
    ps = xos_critical_enter();

    syswhen = xos_get_system_cycles();

    if ((flags & XOS_TIMER_ABSOLUTE) != 0U) {
        // Absolute cycle count specified
        event_when = when;
    }
    else {
        if ((flags & XOS_TIMER_FROM_LAST) != 0U) {
            // From last requested trigger time (no drift)
            // Last req time will be zero if timer never started
            event_when = timer->when + when;
        }
        else {
            // From now
            event_when = syswhen + when;
        }
    }

    timer->when  = event_when;
    timer->delta = ((flags & XOS_TIMER_PERIODIC) != 0U) ? when : 0ULL;
    timer->fn    = func;
    timer->arg   = arg;

    if (event_when < syswhen) {
        xos_critical_exit(ps);
        return XOS_ERR_INVALID_PARAMETER;
    }

    // Insert in timer list
    xos_insert_timer(timer, event_when - syswhen);
    timer->active = 1;

    xos_critical_exit(ps);
    return XOS_OK;
}


//-----------------------------------------------------------------------------
//  Deactivate the timer and remove it from the list of pending timers.
//-----------------------------------------------------------------------------
int32_t
xos_timer_stop(XosTimer * timer)
{
    XosTimer **  last;
    XosTimer *   e;
    uint32_t     ps;
    int32_t      ret = XOS_OK;

    if ((timer == XOS_NULL) || (timer->sig != XOS_TIMER_SIG)) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    ps = xos_critical_enter();

    if (timer->active) {
        last = &xos_timer_list;
        e    = xos_timer_list;

        while (e != XOS_NULL) {
            if (e == timer) {
                *last = e->next;
                timer->active = 0;
                timer->next   = XOS_NULL;
                break;
            }
            last = &(e->next);
            e = e->next;
        }
        if (e != timer) {
            ret = XOS_ERR_NOT_FOUND;
        }
    }

    xos_critical_exit(ps);

#if XOS_OPT_TIMER_WAIT
        // We can do this with interrupts enabled. Since we cleared the active
        // flag already, no other entries will be put into the queue.
        if ((ret == XOS_OK) && (timer->waitq.head != XOS_NULL)) {
            (void) xos_wake_queue(&(timer->waitq), xos_blkon_delay, XOS_ERR_TIMER_DELETE);
            (void) xos_preemption_enable();
        }
#endif

    return ret;
}


//-----------------------------------------------------------------------------
//  Restart the timer to trigger at the new time specified.
//-----------------------------------------------------------------------------
int32_t
xos_timer_restart(XosTimer * timer, uint64_t when)
{
    uint32_t ps;

    if ((timer == XOS_NULL) || (timer->sig != XOS_TIMER_SIG)) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    // If active, disable and take off the active list
    if (timer->active) {
        // Nothing useful we can do with the return code here
        (void) xos_timer_stop(timer);
    }

    // Trigger time is "when" cycles from current time. If periodic, then update
    // the period as well.
    timer->when  = xos_get_system_cycles() + when;
    timer->delta = (timer->delta != 0ULL) ? when : 0ULL;

    ps = xos_critical_enter();
    xos_insert_timer(timer, when);
    timer->active = 1;
    xos_critical_exit(ps);

    return XOS_OK;
}


//-----------------------------------------------------------------------------
//  Update the timer period. Can convert a periodic timer to one shot and vice
//  versa.
//-----------------------------------------------------------------------------
int32_t
xos_timer_set_period(XosTimer * timer, uint64_t period)
{
    if ((timer != XOS_NULL) && (timer->sig == XOS_TIMER_SIG)) {
        uint32_t ps = xos_critical_enter();

        timer->delta = period;
        xos_critical_exit(ps);
        return XOS_OK;
    }

    return XOS_ERR_INVALID_PARAMETER;
}


//-----------------------------------------------------------------------------
//  Thread delay functions.
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//  Thread wakeup callback.
//-----------------------------------------------------------------------------
static void
thread_wakeup_cb(void * arg)
{
    XosThread * thread = xos_voidp_to_threadp(arg);

    XOS_ASSERT(thread != XOS_NULL);

    xos_thread_wake(thread, xos_blkon_delay, 0);
}


//-----------------------------------------------------------------------------
//  Put thread to sleep for at least the specified number of cycles.
//-----------------------------------------------------------------------------
int32_t
xos_thread_sleep(uint64_t cycles)
{
    XosTimer timer;
    uint32_t ps;
    int32_t  ret;

    // Can't call this from interrupt context.
    if (INTERRUPT_CONTEXT) {
        xos_fatal_error(XOS_ERR_INTERRUPT_CONTEXT, XOS_NULL);
    }

    if (cycles == 0ULL) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    (void) xos_timer_init(&timer);

    ps = xos_critical_enter();

    ret = xos_timer_start(&timer,
                          cycles,
                          XOS_TIMER_DELTA | XOS_TIMER_FROM_NOW,
                          &thread_wakeup_cb,
                          xos_curr_threadptr);

    if (ret == XOS_OK) {
        ret = xos_block(xos_blkon_delay, XOS_NULL, 0, 0);
    }

    xos_critical_exit(ps);
    return ret;
}


//-----------------------------------------------------------------------------
//  Block the calling thread on the specified timer. The thread will be woken
//  when the timer expires or is cancelled.
//-----------------------------------------------------------------------------
int32_t
xos_timer_wait(XosTimer * timer)
{
#if XOS_OPT_TIMER_WAIT
    uint32_t ps;
    int32_t  ret;

    // Can't call this from interrupt context.
    if (INTERRUPT_CONTEXT) {
        xos_fatal_error(XOS_ERR_INTERRUPT_CONTEXT, XOS_NULL);
    }

    ps = xos_critical_enter();

    // Timer must be present and active. Don't want to have the active flag
    // change out from under us so do this inside a critical section.
    if ((timer == XOS_NULL) || (timer->sig != XOS_TIMER_SIG) || (!timer->active)) {
        ret = XOS_ERR_INVALID_PARAMETER;
    } else {
        ret = xos_block(xos_blkon_delay, &(timer->waitq), 0, 0);
    }
    xos_critical_exit(ps);
    return ret;

#else

    UNUSED(timer);
    return XOS_ERR_FEATURE_NOT_PRESENT;

#endif
}

