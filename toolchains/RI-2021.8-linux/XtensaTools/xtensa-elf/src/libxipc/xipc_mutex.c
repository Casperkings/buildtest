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
#include "xipc_mutex_internal.h"

/* Initialize mutex. Required to be called before the use of the mutex 
 *
 * mutex     : pointer to mutex object
 * wait_kind : spin wait or sleep when waiting on the mutex.
 *
 * Returns XIPC_OK if successful, else returns XIPC_ERROR_INTERNAL or 
 * XIPC_ERROR_INVALID_ARG.
 */
xipc_status_t 
xipc_mutex_init(xipc_mutex_t     *mutex, 
                xipc_wait_kind_t wait_kind)
{
#if !XCHAL_HAVE_S32C1I && !XCHAL_HAVE_EXCLUSIVE
  XIPC_LOG("xipc_mutex_init: Conditional store or load/store exclusive not supported\n");
  return XIPC_ERROR_SUBSYSTEM_UNSUPPORTED;
#endif

  if (sizeof(xipc_mutex_t) != XIPC_MUTEX_STRUCT_SIZE) {
    XIPC_LOG("xipc_mutex_init:  Internal Error!. Expecting sizeof(xipc_mutex_t) to be %d, but got size %d", XIPC_MUTEX_STRUCT_SIZE, sizeof(xipc_mutex_t));
    return XIPC_ERROR_INTERNAL;
  }

  if ((XIPC_MUTEX_MAX_WAITERS & (XIPC_MUTEX_MAX_WAITERS-1)) != 0) {
    XIPC_LOG("xipc_mutex_init: Internal Error!. Expecting XIPC_MUTEX_MAX_WAITERS %d to be a power of 2\n", XIPC_MUTEX_MAX_WAITERS);
    return XIPC_ERROR_INTERNAL;
  }

  if (mutex == NULL) {
    XIPC_LOG("xipc_mutex_init: mutex is NULL\n");
    return XIPC_ERROR_INVALID_ARG;
  }

  mutex->_lock = 0;
  mutex->_owner = XIPC_MUTEX_NO_OWNER;
  mutex->_wait_kind = wait_kind;
  mutex->_wq_head = 0;
  mutex->_wq_tail = 0;
  uint32_t i;
  for (i = 0; i < XIPC_MUTEX_MAX_WAITERS; i++) {
    mutex->_wait_queue[i] = XIPC_MUTEX_NO_OWNER;
  }
#if XCHAL_DCACHE_SIZE>0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_region_writeback((void *)mutex, sizeof(xipc_mutex_t));
#endif
  XIPC_LOG("Initialized mutex @ %p\n", mutex);
  return XIPC_OK;
}

/* Checks if mutex is owned by this processor. Can be called from within 
 * interrupt context when the XosCond object gets signalled.
 *
 * arg    : pointer to the mutex
 * val    : ignored
 * thread : pointer to thread structure
 *
 * Returns 0 if the mutex is not owned, i.e the condition is not satisfied; else
 * returns 1.
 */
static int32_t
xipc_cond_mutex_owner(void *arg, int32_t val, void *thread)
{
  xipc_mutex_t *mutex = (xipc_mutex_t *)arg;
  uint32_t id = xipc_uniq_id(xipc_get_proc_id(), xipc_get_thread_id(thread));
  /* Perform a writeback+invalidate since this could be called from an interrupt
   * that can happen between the spin-locks during mutex acquire/release.
   * During that spin-lock, the mutex state is being modified and is flushed
   * right before the spin-lock release. If an interrupt happens in that 
   * interval, (spin locks only disable preemption, not interrupts), 
   * this function could get executed and so an invalidate alone would 
   * wipe out any changes that are being made. */
#if XCHAL_DCACHE_SIZE>0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_line_writeback_inv(&mutex->_owner);
#endif
  return mutex->_owner != id ? 0 : 1;
}

/* Acquires mutex. If already acquired, sleep or spin wait.  If sleep waiting,
 * an external event like an interrupt is triggered to notify this thread.
 * 
 * mutex : mutex to be acquired
 * Returns XIPC_OK if successful, else returns XIPC_ERROR_MUTEX_MAX_WAITERS.
 */
xipc_status_t
xipc_mutex_acquire(xipc_mutex_t *mutex)
{
  XIPC_PROFILE_EVENT(XIPC_PROFILE_MUTEX_ACQUIRING, (uintptr_t)mutex);

  uint32_t id = xipc_get_uniq_id();

  XIPC_LOG("Attempting to acquire mutex @ %p\n", mutex);

  /* Acquire a spin-lock on the mutex to prevent others from updating
   * the mutex state */
  xipc_spin_lock_acquire(&mutex->_lock);

#if XCHAL_DCACHE_SIZE>0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_region_invalidate((void *)mutex, sizeof(xipc_mutex_t));
#endif

  if (((mutex->_wq_tail+1) & (XIPC_MUTEX_MAX_WAITERS-1)) == mutex->_wq_head) {
    xipc_spin_lock_release(&mutex->_lock);
    return XIPC_ERROR_MUTEX_MAX_WAITERS;
  }

  if (mutex->_owner != XIPC_MUTEX_NO_OWNER) {
    uint32_t wq_tail = mutex->_wq_tail;
    mutex->_wait_queue[wq_tail] = id;
    mutex->_wq_tail = (wq_tail+1) & (XIPC_MUTEX_MAX_WAITERS-1);
  } else {
    mutex->_owner = id;
#if XCHAL_DCACHE_SIZE>0 && !XCHAL_DCACHE_IS_COHERENT
    xthal_dcache_region_writeback((void *)mutex, sizeof(xipc_mutex_t));
#endif
    /* Release the mutex spin-lock */
    xipc_spin_lock_release(&mutex->_lock);
    XIPC_LOG("Acquired mutex @ %p\n", mutex);

    XIPC_PROFILE_EVENT(XIPC_PROFILE_MUTEX_ACQUIRED, (uintptr_t)mutex);

    return XIPC_OK;
  }

#if XCHAL_DCACHE_SIZE>0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_region_writeback((void *)mutex, sizeof(xipc_mutex_t));
#endif

  /* Release the mutex spin-lock */
  xipc_spin_lock_release(&mutex->_lock);

  if (mutex->_wait_kind == XIPC_SLEEP_WAIT) {
    do {
      uint32_t ps = xipc_disable_interrupts();
      if (xipc_load(&mutex->_owner) != id) {
        XIPC_LOG("Sleep waiting on mutex @ %p\n", mutex);
        xipc_cond_thread_block(xipc_cond_mutex_owner, mutex);
      }
      xipc_enable_interrupts(ps);
    } while (xipc_load(&mutex->_owner) != id);
  } else {
    do {
      int32_t d = xipc_load(&mutex->_wq_tail) - xipc_load(&mutex->_wq_head);
      XT_MOVLTZ(d, d+XIPC_MUTEX_MAX_WAITERS, d);
      xipc_delay(32*d);
    } while (xipc_load(&mutex->_owner) != id);
  }

  XIPC_LOG("Acquired mutex @ %p\n", mutex);

  XIPC_PROFILE_EVENT(XIPC_PROFILE_MUTEX_ACQUIRED, (uintptr_t)mutex);

  return XIPC_OK;
}

/* Release the mutex. Notifies all procs waiting for the mutex 
 *
 * mutex: mutex to be released. 
 *
 * Returns XIPC_OK, if successful, else returns XIPC_ERROR_MUTEX_NOT_OWNER if
 * attempting to release the mutex that was not acquire by this proc.
 */
xipc_status_t xipc_mutex_release(xipc_mutex_t *mutex)
{
  if (xipc_load(&mutex->_owner) != xipc_get_uniq_id())
    return XIPC_ERROR_MUTEX_NOT_OWNER;

  /* Acquire a spin-lock on the mutex to prevent others from updating
   * the mutex state */
  xipc_spin_lock_acquire(&mutex->_lock);

#if XCHAL_DCACHE_SIZE>0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_region_invalidate((void *)mutex, sizeof(xipc_mutex_t));
#endif

  mutex->_owner = XIPC_MUTEX_NO_OWNER;
  uint32_t wq_head = mutex->_wq_head;
  uint32_t wake_pid = 0xffffffff;
  if (mutex->_wait_queue[wq_head] != XIPC_MUTEX_NO_OWNER) {
    uint32_t owner = mutex->_wait_queue[wq_head];
    mutex->_wait_queue[wq_head] = XIPC_MUTEX_NO_OWNER;
    mutex->_wq_head = (wq_head+1) & (XIPC_MUTEX_MAX_WAITERS-1);
    mutex->_owner = owner;
    wake_pid = xipc_get_uniq_id_proc_id(owner);
  }

#if XCHAL_DCACHE_SIZE>0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_region_writeback((void *)mutex, sizeof(xipc_mutex_t));
#endif

  /* Release the mutex spin-lock */
  xipc_spin_lock_release(&mutex->_lock);

  XIPC_LOG("Releasing mutex @ %p\n", mutex);

  XIPC_PROFILE_EVENT(XIPC_PROFILE_MUTEX_RELEASE, (uintptr_t)mutex);

  if (wake_pid != 0xffffffff && mutex->_wait_kind == XIPC_SLEEP_WAIT) {
    if (wake_pid == xipc_get_proc_id())
      xipc_self_notify();
    else
      xipc_proc_notify(wake_pid);
  }

  return XIPC_OK;
}

/* Attempts to acquire the mutex
 * 
 * mutex : mutex to be acquire
 *
 * Returns XIPC_OK if successful, else returns XIPC_ERROR_MUTEX_ACQUIRED if
 * mutex is alread acqired.
 */
xipc_status_t xipc_mutex_try_acquire(xipc_mutex_t *mutex)
{
  uint32_t id = xipc_get_uniq_id();

  XIPC_LOG("Attempting to acquire mutex @ %p\n", mutex);

  /* Check if mutex is already owned. If yes, abort */
  if (xipc_load(&mutex->_owner) != XIPC_MUTEX_NO_OWNER) {
    return XIPC_ERROR_MUTEX_ACQUIRED;
  }

  /* Acquire a spin-lock on the mutex to prevent others from updating
   * the mutex state */
  xipc_spin_lock_acquire(&mutex->_lock);

#if XCHAL_DCACHE_SIZE>0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_region_invalidate((void *)mutex, sizeof(xipc_mutex_t));
#endif

  if (mutex->_owner == XIPC_MUTEX_NO_OWNER) {
    mutex->_owner = id;
#if XCHAL_DCACHE_SIZE>0 && !XCHAL_DCACHE_IS_COHERENT
    xthal_dcache_region_writeback((void *)mutex, sizeof(xipc_mutex_t));
#endif
  }

  /* Release the mutex spin-lock */
  xipc_spin_lock_release(&mutex->_lock);

  if (xipc_load(&mutex->_owner) != id) {
    XIPC_LOG("Failed to acquire mutex @ %p\n", mutex);
    return XIPC_ERROR_MUTEX_ACQUIRED;
  }

  XIPC_LOG("Acquired mutex @ %p\n", mutex);

  return XIPC_OK;
}
