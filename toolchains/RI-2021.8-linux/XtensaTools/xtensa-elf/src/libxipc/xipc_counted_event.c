/* Copyright (c) 2003-2015 Cadence Design Systems, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "xipc_misc.h"
#include "xipc_common.h"
#include "xipc_counted_event_internal.h"

/* Initialize xipc_counted_event object. Required to be called before using 
 * the same 
 *
 * e         : pointer to event object
 * wait_kind : spin wait or sleep when waiting on the counted event.
 *
 * Returns XIPC_OK, if successful, else returns XIPC_ERROR_INTERNAL, or
 * XIPC_ERROR_SUBSYSTEM_UNSUPPORTED.
 */
xipc_status_t 
xipc_counted_event_init(xipc_counted_event_t *e,
                        xipc_wait_kind_t      wait_kind)
{
#if !XCHAL_HAVE_S32C1I && !XCHAL_HAVE_EXCLUSIVE
  XIPC_LOG("xipc_counted_event_init: Conditional store or load/store exclusive not supported\n");
  return XIPC_ERROR_SUBSYSTEM_UNSUPPORTED;
#endif

  if (sizeof(xipc_counted_event_t) != XIPC_COUNTED_EVENT_STRUCT_SIZE) {
    XIPC_LOG("xipc_counted_event_init:  Internal Error!. Expecting sizeof(xipc_counted_event_t) to be %d, but got size %d", XIPC_COUNTED_EVENT_STRUCT_SIZE, sizeof(xipc_counted_event_t));
    return XIPC_ERROR_INTERNAL;
  }

  if ((xipc_get_master_pid()+xipc_get_num_procs()) > XIPC_MAX_NUM_PROCS) {
    XIPC_LOG("xipc_counted_event_init: Subsystem has %d processors, but the max supported is %d\n", (int)xipc_get_master_pid()+xipc_get_num_procs(), XIPC_MAX_NUM_PROCS);
    return XIPC_ERROR_SUBSYSTEM_UNSUPPORTED;
  }

  if (e == NULL) {
    XIPC_LOG("xipc_counted_event_init: counted event is NULL\n");
    return XIPC_ERROR_INVALID_ARG;
  }

  e->_lock = 0;
  e->_count = 0;
  e->_wait_kind = wait_kind;
  int i;
  for (i = 0; i < XIPC_MAX_NUM_PROCS; i++) { 
    e->_wait_pids[i] = 0;
    e->_wait_count[i] = 0;
  }

#if XCHAL_DCACHE_SIZE>0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_region_writeback((void *)e, sizeof(xipc_counted_event_t));
#endif
  XIPC_LOG("Initialized event @ %p\n", e);
  return XIPC_OK;
}

/* Set an event by incrementing the count. Notify all procs waiting on the 
 * event if the count is >= the counts on which they are waiting.
 *
 * e : event to be set
 *
 * Returns XIPC_OK, if successful, else returns XIPC_ERROR_EVENT_SET if event
 * count overflows
 */
xipc_status_t 
xipc_set_counted_event(xipc_counted_event_t *e)
{
  /* Acquire a spin-lock on the event to protect the event state */
  xipc_spin_lock_acquire(&e->_lock);

#if XCHAL_DCACHE_SIZE>0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_region_invalidate((void *)e, sizeof(xipc_counted_event_t));
#endif

  uint32_t next_count = e->_count+1;
  if (next_count == 0) {
    XIPC_LOG("xipc_set_counted_event: count overflow!\n");
    xipc_spin_lock_release(&e->_lock);
    return XIPC_ERROR_EVENT_SET;
  }
  e->_count = next_count;

  XIPC_LOG("Event @ %p set\n", e);

  XIPC_PROFILE_EVENT(XIPC_PROFILE_COUNTED_EVENT_SET, (uintptr_t)e);

  /* Notify the other processors that are waiting for the event */
  uint32_t pid;
  uint32_t my_pid = xipc_get_proc_id();
  uint32_t master_pid = xipc_get_master_pid();
  uint32_t num_procs = xipc_get_num_procs();
  if (e->_wait_kind == XIPC_SLEEP_WAIT) {
#pragma loop_count min=1
    for (pid = master_pid; pid < master_pid+num_procs; pid++) {
      if (e->_wait_pids[pid] != 0 && e->_wait_count[pid] <= next_count) {
        e->_wait_pids[pid] = 0;
        e->_wait_count[pid] = 0;
        if (pid == my_pid) {
          xipc_self_notify();
        } else {
#if XCHAL_DCACHE_SIZE>0 && !XCHAL_DCACHE_IS_COHERENT
          xthal_dcache_line_writeback((void *)&e->_wait_count[pid]); 
#endif
#pragma flush_memory
          XIPC_LOG("Waking up proc %s on event @ %p\n", 
                   xipc_get_proc_name(pid), e);
          xipc_proc_notify(pid);
        }
      }
    }
  } else {
#pragma loop_count min=1
    for (pid = master_pid; pid < master_pid+num_procs; pid++) {
      if (e->_wait_pids[pid] != 0 && e->_wait_count[pid] <= next_count) {
        e->_wait_pids[pid] = 0;
        e->_wait_count[pid] = 0;
      }
    }
  }

#if XCHAL_DCACHE_SIZE>0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_region_writeback((void *)e, sizeof(xipc_counted_event_t));
#endif

  /* Release the event spin-lock */
  xipc_spin_lock_release(&e->_lock);

  XIPC_PROFILE_EVENT(XIPC_PROFILE_COUNTED_EVENT_RESET, (uintptr_t)e);

  return XIPC_OK;
}

/* Checks if event count is reached. Can be called from within 
 * interrupt context when the XosCond object gets signalled.
 *
 * arg    : pointer to the event
 * val    : ignored
 * thread : pointer to thread structure
 *
 * Returns 0 if the event count has not reached, i.e the condition is not 
 * satisfied; else returns 1.
 */
static int32_t
xipc_cond_counted_event(void *arg, int32_t val, void *thread)
{
  xipc_counted_event_t *e = (xipc_counted_event_t *)arg;
  uint32_t pid = xipc_get_proc_id();
  /* Note, xipc_set_counted_event resets the wait_count to 0, if the count
   * is reached */
#if XCHAL_DCACHE_SIZE>0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_line_writeback_inv((void *)&e->_wait_count[pid]);
#endif
  return e->_wait_count[pid] != 0 ? 0 : 1;
}

/* Checks if event count of any event in an event set is reached. 
 * Can be called from within interrupt context when the XosCond object 
 * gets signalled.
 *
 * arg    : pointer to the event structure that holds the event set
 * val    : ignored
 * thread : pointer to thread structure
 *
 * Returns 0 if non of the event count has not reached, i.e the condition 
 * is not satisfied; else returns 1.
 */
static int32_t
xipc_cond_counted_event_any_set(void *arg, int32_t val, void *thread)
{
  int i;
  struct xipc_cond_counted_event_struct *eset = 
                           (struct xipc_cond_counted_event_struct *)arg;
  uint32_t pid = xipc_get_proc_id();
  for (i = 0; i < eset->_num_events; i++) {
    /* Note, xipc_set_counted_event resets the wait_count to 0, if the count
     * is reached */
#if XCHAL_DCACHE_SIZE>0 && !XCHAL_DCACHE_IS_COHERENT
    xthal_dcache_line_writeback_inv((void *)
                                    &eset->_events[i]->_wait_count[pid]);
#endif
    if (eset->_events[i]->_wait_count[pid] == 0)
      return 1;
  }
  return 0;
}

/* Wait until the event count is reached by either
 * sleep or spin waiting. If sleep waiting, an external event like an 
 * interrupt is triggered to notify this thread. If multiple threads from
 * the same core waits for the counted event, the wait count for the core 
 * is set to the maximum across all such waiting threads.
 * 
 * e     : event to wait for
 * count : count to wait for
 * 
 * Returns void
 */
void 
xipc_wait_counted_event(xipc_counted_event_t *e, uint32_t count)
{
  XIPC_PROFILE_EVENT(XIPC_PROFILE_COUNTED_EVENT_WAIT, (uintptr_t)e);

  uint32_t pid = xipc_get_proc_id();

#if XCHAL_DCACHE_SIZE>0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_region_invalidate((void *)e, sizeof(xipc_counted_event_t));
#endif

  /* Return right away if the threshold has reached */
  if (count <= e->_count) {
    XIPC_PROFILE_EVENT(XIPC_PROFILE_COUNTED_EVENT_RECEIVED, (uintptr_t)e);
    return;
  }

  /* Acquire a spin-lock on the event to protect the event state */
  xipc_spin_lock_acquire(&e->_lock);

#if XCHAL_DCACHE_SIZE>0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_region_invalidate((void *)e, sizeof(xipc_counted_event_t));
#endif

  /* Return right away if the threshold has reached */
  if (count <= e->_count) {
    /* Release the event spin-lock */
    xipc_spin_lock_release(&e->_lock);
    XIPC_PROFILE_EVENT(XIPC_PROFILE_COUNTED_EVENT_RECEIVED, (uintptr_t)e);
    return;
  }

  e->_wait_pids[pid] = 1;
  uint32_t c = e->_wait_count[pid]; 
  e->_wait_count[pid] = count > c ? count : c;

#if XCHAL_DCACHE_SIZE>0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_region_writeback((void *)e, sizeof(xipc_counted_event_t));
#endif

  /* Release the event spin-lock */
  xipc_spin_lock_release(&e->_lock);
  
  if (e->_wait_kind == XIPC_SLEEP_WAIT) {
    while (xipc_load(&e->_wait_count[pid]) != 0) {
      XIPC_LOG("Sleep waiting on counted event @ %p with count %d\n", 
               e, (int)count);
      uint32_t ps = xipc_disable_interrupts();
      if (xipc_load(&e->_wait_count[pid]) != 0)  {
        xipc_cond_thread_block(xipc_cond_counted_event, e);
      }
      xipc_enable_interrupts(ps);
    }
  } else {
    while (xipc_load(&e->_wait_count[pid]) != 0) {
      xipc_delay(16);
    }
  }

  XIPC_LOG("Received event @ %p\n", e);

  XIPC_PROFILE_EVENT(XIPC_PROFILE_COUNTED_EVENT_RECEIVED, (uintptr_t)e);
}

/* Wait until any event in a set of events has reached its corresponding
 * count is reached by either sleep or spin waiting. If sleep waiting, 
 * an external event like an interrupt is triggered to notify this thread. 
 * If multiple threads from the same core waits for the counted event, 
 * the wait count for the core is set to the maximum across all such waiting 
 * threads.
 * 
 * events     : event set to wait for
 * counts     : count correspinding to each event in the event set
 * num_events : number of events in the event set
 * 
 * Returns index of earliest event that meets count.
 */
uint32_t 
xipc_wait_counted_event_any(xipc_counted_event_t *events[], 
                            uint32_t counts[],
                            uint32_t num_events)
{
  int i, j;
  uint32_t pid = xipc_get_proc_id();

  for (i = 0; i < num_events; i++) {
#if XCHAL_DCACHE_SIZE>0 && !XCHAL_DCACHE_IS_COHERENT
    xthal_dcache_region_invalidate((void *)events[i], 
                                   sizeof(xipc_counted_event_t));
#endif
    /* Return right away if the threshold has reached */
    if (counts[i] <= events[i]->_count) {
      return i;
    }
  }

  for (i = 0; i < num_events; i++) {
    /* Acquire a spin-lock on the event to protect the event state */
    xipc_spin_lock_acquire(&events[i]->_lock);

#if XCHAL_DCACHE_SIZE>0 && !XCHAL_DCACHE_IS_COHERENT
    xthal_dcache_region_invalidate((void *)events[i], 
                                   sizeof(xipc_counted_event_t));
#endif

    /* Return right away if the threshold has reached */
    if (counts[i] <= events[i]->_count) {
      /* Release the event spin-lock for all earlier events that were locked */
      for (j = 0; j <= i; j++) {
        xipc_spin_lock_release(&events[j]->_lock);
      }
      return i;
    }
  }

  for (i = 0; i < num_events; i++) {
    events[i]->_wait_pids[pid] = 1;
    uint32_t c = events[i]->_wait_count[pid]; 
    events[i]->_wait_count[pid] = counts[i] > c ? counts[i] : c;

#if XCHAL_DCACHE_SIZE>0 && !XCHAL_DCACHE_IS_COHERENT
    xthal_dcache_region_writeback((void *)events[i], 
                                  sizeof(xipc_counted_event_t));
#endif

    /* Release the event spin-lock */
    xipc_spin_lock_release(&events[i]->_lock);
  }
  
  xipc_counted_event_t *ev = 0;
  uint32_t idx = 0;
  /* Expects all events to be of the same xipc_wait_kind_t type */
  if (events[0]->_wait_kind == XIPC_SLEEP_WAIT) {
    while (ev == 0) {
      uint32_t ps = xipc_disable_interrupts();
      for (i = 0; i < num_events; i++) {
        if (xipc_load(&events[i]->_wait_count[pid]) == 0) {
          idx = i;
          ev = events[i];
          break;
        }
      }
      if (ev == 0) {
        XIPC_LOG("Sleep waiting on counted events\n");
        struct xipc_cond_counted_event_struct s = {._events = events, 
                                                   ._num_events = num_events};
        xipc_cond_thread_block(xipc_cond_counted_event_any_set, &s);
      }
      xipc_enable_interrupts(ps);
    }
  } else {
    while (ev == 0) {
      xipc_delay(16);
      for (i = 0; i < num_events; i++) {
        if (xipc_load(&events[i]->_wait_count[pid]) == 0) {
          idx = i;
          ev = events[i];
          break;
        }
      }
    }
  }

  XIPC_LOG("Received event @ %p\n", ev);
  return idx;
}

/* Read event count
 *
 * e : event to read count
 *
 * Returns the event count
 */
uint32_t 
xipc_read_counted_event(xipc_counted_event_t *e)
{
  return xipc_load(&e->_count);
}
