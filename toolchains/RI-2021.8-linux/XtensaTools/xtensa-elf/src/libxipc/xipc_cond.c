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
#include "xipc_mutex_internal.h"
#include "xipc_cond_internal.h"

/* Initialize the conditional object. Needs to be called prior to using the same
 *
 * cond : condition to be initialized
 *
 * Returns XIPC_OK if successful, else returns XIPC_ERROR_INTERNAL, or
 * XIPC_ERROR_SUBSYSTEM_UNSUPPORTED
 */
xipc_status_t 
xipc_cond_init(xipc_cond_t *cond, xipc_wait_kind_t wait_kind)
{
#if !XCHAL_HAVE_S32C1I && !XCHAL_HAVE_EXCLUSIVE
  XIPC_LOG("xipc_barrier_init: Conditional store or load/store exclusive not supported\n");
  return XIPC_ERROR_SUBSYSTEM_UNSUPPORTED;
#endif

  if (sizeof(xipc_cond_t) != XIPC_COND_STRUCT_SIZE) {
    XIPC_LOG("xipc_cond_init:  Internal Error!. Expecting sizeof(xipc_cond_t) to be %d, but got size %d", XIPC_COND_STRUCT_SIZE, sizeof(xipc_cond_t));
    return XIPC_ERROR_INTERNAL;
  }

  if ((XIPC_COND_MAX_WAITERS & (XIPC_COND_MAX_WAITERS-1)) != 0) {
    XIPC_LOG("xipc_cond_init: Internal Error!. Expecting XIPC_COND_MAX_WAITERS %d to be a power of 2\n", XIPC_COND_MAX_WAITERS);
    return XIPC_ERROR_INTERNAL;
  }

  if (cond == NULL) {
    XIPC_LOG("xipc_cond_init: cond is NULL\n");
    return XIPC_ERROR_INVALID_ARG;
  }

  cond->_wait_kind = wait_kind;
  cond->_wq_head = 0;
  cond->_wq_tail = 0;
  int32_t i;
  for (i = 0; i < XIPC_COND_MAX_WAITERS; i++) {
    cond->_wait_queue[i] = 0;
  }
#if XCHAL_DCACHE_SIZE>0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_region_writeback((void *)cond, sizeof(xipc_cond_t));
#endif
  XIPC_LOG("Initialized cond @ %p\n", cond);
  return XIPC_OK;
}

/* Checks if condition is satisfied. Can be called from within 
 * interrupt context when the XosCond object gets signalled.
 *
 * arg    : pointer to the conditional object
 * val    : ignored
 * thread : pointer to thread structure
 *
 * Returns 0 if the condition is not satisfied; else returns 1.
 */
static int32_t
xipc_check_cond(void *arg, int32_t val, void *thread)
{
  xipc_cond_t *cond = (xipc_cond_t *)arg;
  uint32_t id = xipc_uniq_id(xipc_get_proc_id(), xipc_get_thread_id(thread));
  uint32_t wait = 0;
#if XCHAL_DCACHE_SIZE>0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_region_writeback_inv((void *)cond, sizeof(xipc_cond_t));
#endif
  /* Check if the wait is still set */
  uint32_t i = cond->_wq_head;
  while (i != cond->_wq_tail) {
    if (cond->_wait_queue[i] == id) {
      wait = 1;
      break;
    }
    i = (i+1) & (XIPC_COND_MAX_WAITERS-1);
  }
  return !wait;
}

/* Wait on condition. Expects the mutex to be locked. Unlocks the mutex and
 * waits; when awoken, reacquire the mutex
 *
 * cond  : condition to wait on
 * mutex : mutex that is expected to be locked
 *
 * Returns XIPC_OK if successful, else returns XIPC_ERROR_MUTEX_NOT_OWNER if
 * the mutex is not owned by this processor.
 */
xipc_status_t 
xipc_cond_wait(xipc_cond_t *cond, xipc_mutex_t *mutex)
{
  XIPC_PROFILE_EVENT(XIPC_PROFILE_COND_WAIT, (uintptr_t)cond);

  uint32_t id = xipc_get_uniq_id();

#if XCHAL_DCACHE_SIZE>0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_line_invalidate(&mutex->_owner);
#endif

#if XCHAL_DCACHE_SIZE>0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_region_invalidate((void *)cond, sizeof(xipc_cond_t));
#endif

  /* The mutex is assumed to be locked by this proc */
  if (mutex->_owner != id) {
    return XIPC_ERROR_MUTEX_NOT_OWNER;
  }

  if (((cond->_wq_tail+1) & (XIPC_COND_MAX_WAITERS-1)) == cond->_wq_head) {
    return XIPC_ERROR_COND_MAX_WAITERS;
  }

  uint32_t wq_tail = cond->_wq_tail;
  cond->_wait_queue[wq_tail] = id;
  cond->_wq_tail = (wq_tail+1) & (XIPC_COND_MAX_WAITERS-1);

#if XCHAL_DCACHE_SIZE>0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_region_writeback((void *)cond, sizeof(xipc_cond_t));
#endif
  
  /* Unlock the mutex before going to sleep */
  xipc_mutex_release(mutex);

  uint32_t wait = 1;
  while (wait) {
    uint32_t ps = xipc_disable_interrupts();
#if XCHAL_DCACHE_SIZE>0 && !XCHAL_DCACHE_IS_COHERENT
    xthal_dcache_region_invalidate((void *)cond, sizeof(xipc_cond_t));
#endif
    wait = 0;
    /* Check if the wait is still set */
    uint32_t i = cond->_wq_head;
    while (i != cond->_wq_tail) {
      if (cond->_wait_queue[i] == id) {
        wait = 1;
        break;
      }
      i = (i+1) & (XIPC_COND_MAX_WAITERS-1);
    }
    if (wait) {
      if (cond->_wait_kind == XIPC_SLEEP_WAIT) {
        XIPC_LOG("Sleep on condition @ %p\n", cond);
        xipc_cond_thread_block(xipc_check_cond, cond);
      }
    }
    xipc_enable_interrupts(ps);
  }

  /* Reacquire the mutex */
  xipc_mutex_acquire(mutex);

  XIPC_PROFILE_EVENT(XIPC_PROFILE_COND_WAKEUP, (uintptr_t)cond);

  return XIPC_OK;
}

/* Wakeup the next proc waiting on the condition
 *
 * cond : condition to wakeup
 *
 * Returns void
 */
void xipc_cond_signal(xipc_cond_t *cond)
{
  XIPC_PROFILE_EVENT(XIPC_PROFILE_COND_SIGNAL, (uintptr_t)cond);

#if XCHAL_DCACHE_SIZE>0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_region_invalidate((void *)cond, sizeof(xipc_cond_t));
#endif

  uint32_t wake_pid = 0xffffffff;
  uint32_t wq_head = cond->_wq_head;
  uint32_t next_waiter = cond->_wait_queue[wq_head];
  if (next_waiter != 0) {
    cond->_wait_queue[wq_head] = 0;
    cond->_wq_head = (wq_head+1) & (XIPC_COND_MAX_WAITERS-1);
    wake_pid = xipc_get_uniq_id_proc_id(next_waiter);
  }

#if XCHAL_DCACHE_SIZE>0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_region_writeback((void *)cond, sizeof(xipc_cond_t));
#endif

  if (wake_pid != 0xffffffff && cond->_wait_kind == XIPC_SLEEP_WAIT) {
    XIPC_LOG("Signaling pid %d on condition @ %p\n", (int)wake_pid, cond);
#pragma flush_memory
    if (wake_pid == xipc_get_proc_id())
      xipc_self_notify();
    else
      xipc_proc_notify(wake_pid);
  }

  XIPC_PROFILE_EVENT(XIPC_PROFILE_COND_SIGNAL_DONE, (uintptr_t)cond);
}

/* Wakeup all procs that are waiting on the condition. 
 *
 * cond : condition to wakeup
 *
 * Returns void
 */
void 
xipc_cond_broadcast(xipc_cond_t *cond)
{
  XIPC_PROFILE_EVENT(XIPC_PROFILE_COND_SIGNAL, (uintptr_t)cond);

#if XCHAL_DCACHE_SIZE>0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_region_invalidate((void *)cond, sizeof(xipc_cond_t));
#endif

  uint32_t i = cond->_wq_head;
  while (i != cond->_wq_tail) {
    uint32_t waiter = cond->_wait_queue[i];
    cond->_wait_queue[i] = 0;
    i = (i+1) & (XIPC_COND_MAX_WAITERS-1);
    cond->_wq_head = i;
#if XCHAL_DCACHE_SIZE>0 && !XCHAL_DCACHE_IS_COHERENT
    xthal_dcache_region_writeback((void *)cond, sizeof(xipc_cond_t));
#endif
#pragma flush_memory
    if (cond->_wait_kind == XIPC_SLEEP_WAIT) {
      uint32_t pid = xipc_get_uniq_id_proc_id(waiter);
      XIPC_LOG("Signaling pid %d on condition @ %p\n", (int)pid, cond);
      if (pid == xipc_get_proc_id())
        xipc_self_notify();
      else
        xipc_proc_notify(pid);
    }
  }

  XIPC_PROFILE_EVENT(XIPC_PROFILE_COND_SIGNAL_DONE, (uintptr_t)cond);
}
