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
#include "xipc_sem_internal.h"

/* Initialize semaphore. Required to be called before the use of the semaphore.
 *
 * sem       : pointer to semaphore object
 * count     : initial count of semaphore
 * wait_kind : spin wait or sleep when waiting on the semaphore.
 *
 * Returns XIPC_OK if successful, else returns XIPC_ERROR_INTERNAL or 
 * XIPC_ERROR_INVALID_ARG.
 */
xipc_status_t
xipc_sem_init(xipc_sem_t      *sem,
              uint32_t         count,
              xipc_wait_kind_t wait_kind)
{
#if !XCHAL_HAVE_S32C1I && !XCHAL_HAVE_EXCLUSIVE
  XIPC_LOG("xipc_sem_init: Conditional store or load/store exclusive not supported\n");
  return XIPC_ERROR_SUBSYSTEM_UNSUPPORTED;
#endif

  if (sizeof(xipc_sem_t) != XIPC_SEM_STRUCT_SIZE) {
    XIPC_LOG("xipc_sem_init: Internal Error!. Expecting sizeof(xipc_sem_t) to be %d, but got size %d", XIPC_SEM_STRUCT_SIZE, sizeof(xipc_sem_t));
    return XIPC_ERROR_INTERNAL;
  }

  if ((XIPC_SEM_MAX_WAITERS & (XIPC_SEM_MAX_WAITERS-1)) != 0) {
    XIPC_LOG("xipc_sem_init: Internal Error!. Expecting XIPC_SEM_MAX_WAITERS %d to be a power of 2\n", XIPC_SEM_MAX_WAITERS);
    return XIPC_ERROR_INTERNAL;
  }

  if (sem == NULL) {
    XIPC_LOG("xipc_sem_init: sem is NULL\n");
    return XIPC_ERROR_INVALID_ARG;
  }

  sem->_lock = 0; 
  sem->_wait_kind = wait_kind;
  sem->_count = count;
  sem->_wq_head = 0;
  sem->_wq_tail = 0;
  uint32_t i;
  for (i = 0; i < XIPC_SEM_MAX_WAITERS; i++) {
    sem->_wait_queue[i] = 0;
  }
#if XCHAL_DCACHE_SIZE>0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_region_writeback((void *)sem, sizeof(xipc_sem_t));
#endif
  XIPC_LOG("Initialized sem @ %p\n", sem);
  return XIPC_OK;
}

/* Checks if semaphore wait is satisfied. Can be called from within 
 * interrupt context when the XosCond object gets signalled.
 *
 * arg    : pointer to the semaphore object
 * val    : ignored
 * thread : pointer to thread structure
 *
 * Returns 0 if the condition is not satisfied; else returns 1.
 */
static int32_t
xipc_cond_sem(void *arg, int32_t val, void *thread)
{
  xipc_sem_t *sem = (xipc_sem_t *)arg;
  uint32_t id = xipc_uniq_id(xipc_get_proc_id(), xipc_get_thread_id(thread));
  uint32_t wait = 0;
#if XCHAL_DCACHE_SIZE>0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_region_writeback_inv((void *)sem, sizeof(xipc_sem_t));
#endif
  /* Check if the wait is still set */
  uint32_t i = sem->_wq_head;
  while (i != sem->_wq_tail) {
    if (sem->_wait_queue[i] == id) {
      wait = 1;
      break;
    }
    i = (i+1) & (XIPC_SEM_MAX_WAITERS-1);
  }
  return !wait;
}

/* Check if wait is still set.
 *
 * sem : pointer to semaphore
 *
 * Returns 1 if wait is set, else 0.
 */
static XIPC_INLINE uint32_t 
xipc_sem_check_wait(xipc_sem_t *sem, uint32_t id)
{
  uint32_t wait = 0;
#if XCHAL_DCACHE_SIZE>0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_region_invalidate((void *)sem, sizeof(xipc_sem_t));
#endif
  uint32_t i = sem->_wq_head;
  while (i != sem->_wq_tail) {
    if (sem->_wait_queue[i] == id) {
      wait = 1;
      break;
    }
    i = (i+1) & (XIPC_SEM_MAX_WAITERS-1);
  }
  return wait;
}

/* Wait on semaphore. If count is 0, sleep or spin wait.  If sleep waiting,
 * an external event like an interrupt is triggered to notify this thread.
 * when the semaphore is available.
 * 
 * sem : semaphore to be acquired
 * 
 * Returns XIPC_OK if successful, else returns XIPC_ERROR_SEM_MAX_WAITERS.
 */
xipc_status_t
xipc_sem_wait(xipc_sem_t *sem)
{
  XIPC_PROFILE_EVENT(XIPC_PROFILE_SEM_ACQUIRING, (uintptr_t)sem);

  uint32_t id = xipc_get_uniq_id();
  
  XIPC_LOG("Wait on semaphore @ %p\n", sem);
  
  /* Acquire a spin-lock on the semaphore to prevent others from updating
   * the semaphore state */
  xipc_spin_lock_acquire(&sem->_lock);
  
  while (xipc_load(&sem->_count) == 0) {
#if XCHAL_DCACHE_SIZE>0 && !XCHAL_DCACHE_IS_COHERENT
    xthal_dcache_region_invalidate((void *)sem, sizeof(xipc_sem_t));
#endif
    if (((sem->_wq_tail+1) & (XIPC_SEM_MAX_WAITERS-1)) == sem->_wq_head) {
      xipc_spin_lock_release(&sem->_lock);
      return XIPC_ERROR_SEM_MAX_WAITERS;
    }
    uint32_t wq_tail = sem->_wq_tail;
    sem->_wait_queue[wq_tail] = id;
    sem->_wq_tail = (wq_tail+1) & (XIPC_SEM_MAX_WAITERS-1);
#if XCHAL_DCACHE_SIZE>0 && !XCHAL_DCACHE_IS_COHERENT
    xthal_dcache_region_writeback((void *)sem, sizeof(xipc_sem_t));
#endif
    /* Release the semaphore spin-lock */
    xipc_spin_lock_release(&sem->_lock);

    uint32_t wait = 1;
    if (sem->_wait_kind == XIPC_SLEEP_WAIT) {
      while (wait) {
        uint32_t ps = xipc_disable_interrupts();
        if ((wait = xipc_sem_check_wait(sem, id))) {
          XIPC_LOG("Sleep on semaphore @ %p\n", sem);
          xipc_cond_thread_block(xipc_cond_sem, sem);
        }
        xipc_enable_interrupts(ps);
      }
    } else {
      while (wait) {
        if ((wait = xipc_sem_check_wait(sem, id))) {
          xipc_delay(16);
        }
      }
    }

    /* Acquire a spin-lock on the semaphore to prevent others from updating
     * the semaphore state */
    xipc_spin_lock_acquire(&sem->_lock);
  } 

  sem->_count--;

  XIPC_LOG("Acquire semaphore @ %p, count %d\n", sem, sem->_count);

#if XCHAL_DCACHE_SIZE>0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_line_writeback(&sem->_count);
#endif

  /* Release the semaphore spin-lock */
  xipc_spin_lock_release(&sem->_lock);

  XIPC_PROFILE_EVENT(XIPC_PROFILE_SEM_ACQUIRED, (uintptr_t)sem);

  return XIPC_OK;
}

/* Increments the semaphore and wakesup next proc/thread waiting on semaphore.
 *
 * sem : pointer to semaphore
 *
 * Return nothing
 */
void 
xipc_sem_post(xipc_sem_t *sem)
{
  /* Acquire a spin-lock on the semaphore to prevent others from updating
   * the semaphore state */
  xipc_spin_lock_acquire(&sem->_lock);

#if XCHAL_DCACHE_SIZE>0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_region_invalidate((void *)sem, sizeof(xipc_sem_t));
#endif

  sem->_count++;
  uint32_t wake_pid = 0xffffffff;
  uint32_t wq_head = sem->_wq_head;
  uint32_t next_waiter = sem->_wait_queue[wq_head];
  if (next_waiter != 0) {
    sem->_wait_queue[wq_head] = 0;
    sem->_wq_head = (wq_head+1) & (XIPC_SEM_MAX_WAITERS-1);
    wake_pid = xipc_get_uniq_id_proc_id(next_waiter);
  } 
  
#if XCHAL_DCACHE_SIZE>0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_region_writeback((void *)sem, sizeof(xipc_sem_t));
#endif

  XIPC_LOG("Releasing semaphore @ %p, count %d\n", sem, sem->_count);

  /* Release the semaphore spin-lock */
  xipc_spin_lock_release(&sem->_lock);

  XIPC_PROFILE_EVENT(XIPC_PROFILE_SEM_RELEASE, (uintptr_t)sem);

  if (wake_pid != 0xffffffff && sem->_wait_kind == XIPC_SLEEP_WAIT) {
    if (wake_pid == xipc_get_proc_id())
      xipc_self_notify();
    else
      xipc_proc_notify(wake_pid);
  }
}

/* Attempt locking semaphore
 * 
 * sem : semaphore to be acquired
 * 
 * Returns XIPC_OK if successful, else returns XIPC_ERROR_MUTEX_LOCKED if
 * semaphore count is 0.
 */
xipc_status_t
xipc_sem_try_wait(xipc_sem_t *sem)
{
  XIPC_LOG("Wait on semaphore @ %p\n", sem);
  
  /* Acquire a spin-lock on the semaphore to prevent others from updating
   * the semaphore state */
  xipc_spin_lock_acquire(&sem->_lock);
  
  if (xipc_load(&sem->_count) == 0) {
    /* Release the semaphore spin-lock */
    xipc_spin_lock_release(&sem->_lock);
    return XIPC_ERROR_SEM_LOCKED;
  }

  sem->_count--;

#if XCHAL_DCACHE_SIZE>0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_line_writeback(&sem->_count);
#endif

  /* Release the semaphore spin-lock */
  xipc_spin_lock_release(&sem->_lock);

  return XIPC_OK;
}
