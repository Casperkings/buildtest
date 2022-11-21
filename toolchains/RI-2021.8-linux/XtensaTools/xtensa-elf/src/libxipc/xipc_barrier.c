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
#include "xipc_barrier_internal.h"

/* Initialize barrier. Required to be called prior to use of the barrier.
 *
 * barrier     : pointer to the barrier object
 * wait_kind   : spin wait or sleep while waiting for other procs at the barrier
 * num_procs   : number of procs participating in this barrier
 * proc_ids    : proc ids of these participating procs 
 * num_waiters : number of waiters at this barrier. Has to be >= num_procs.
 *
 * Returns XIPC_OK if successful, 
 * else returns one of XIPC_ERROR_INVALID_ARG, XIPC_ERROR_INTERNAL, or
 * XIPC_ERROR_SUBSYSTEM_UNSUPPORTED
 */
xipc_status_t
xipc_barrier_init(xipc_barrier_t  *barrier,
                  xipc_wait_kind_t wait_kind,
                  uint8_t          num_procs,
                  xipc_pid_t      *proc_ids,
                  uint8_t          num_waiters)
{
#if !XCHAL_HAVE_S32C1I && !XCHAL_HAVE_EXCLUSIVE
  XIPC_LOG("xipc_barrier_init: Conditional store or load/store exclusive not supported\n");
  return XIPC_ERROR_SUBSYSTEM_UNSUPPORTED;
#endif

  if (sizeof(xipc_barrier_t) != XIPC_BARRIER_STRUCT_SIZE) {
    XIPC_LOG("xipc_barrier_init:  Internal Error!. Expecting sizeof(xipc_barrier_t) to be %d, but got size %d", XIPC_BARRIER_STRUCT_SIZE, sizeof(xipc_barrier_t));
    return XIPC_ERROR_INTERNAL;
  }

  if ((xipc_get_master_pid()+xipc_get_num_procs()) > XIPC_MAX_NUM_PROCS) {
    XIPC_LOG("xipc_barrier_init: Subsystem has %d processors, but the max supported is %d\n", (int)xipc_get_master_pid()+xipc_get_num_procs(), XIPC_MAX_NUM_PROCS);
    return XIPC_ERROR_SUBSYSTEM_UNSUPPORTED;
  }

  if (barrier == NULL) {
    XIPC_LOG("xipc_barrier_init: barrier is NULL\n");
    return XIPC_ERROR_INVALID_ARG;
  }

  if (proc_ids == NULL) {
    XIPC_LOG("xipc_barrier_init: proc_ids is NULL\n");
    return XIPC_ERROR_INVALID_ARG;
  }

  if (num_procs == 0 || num_procs > xipc_get_num_procs()) {
    XIPC_LOG("xipc_barrier_init: Expecting num_procs to be > 0 and <= %d\n", 
             xipc_get_num_procs());
    return XIPC_ERROR_INVALID_ARG;
  }

  if (num_waiters < num_procs) {
    XIPC_LOG("xipc_barrier_init: Expecting num_waiters %d to be >= num_procs %d\n", num_waiters, num_procs);
    return XIPC_ERROR_INVALID_ARG;
  }

  barrier->_wait_kind = wait_kind;
  barrier->_num_waiters = num_waiters;
  barrier->_count = 0;
  barrier->_state = XIPC_BARRIER_ENTER;
  int i;
  for (i = 0; i < XIPC_MAX_NUM_PROCS; i++) {
    barrier->_wait_pids[i] = 0;
  }

  for (i = 0; i < num_procs; i++) {
    uint32_t pid = proc_ids[i];
    if (pid < xipc_get_master_pid() ||
        pid >= (xipc_get_master_pid() + xipc_get_num_procs())) {
      XIPC_LOG("xipc_barrier_init: Invalid pid %d in proc_ids. Expecting pid "
               "to be >= %d and < %d\n", (int)pid, xipc_get_master_pid(),
               xipc_get_master_pid()+xipc_get_num_procs());
      return XIPC_ERROR_INVALID_ARG;
    }
    barrier->_wait_pids[pid] = 1;
  }
#if XCHAL_DCACHE_SIZE>0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_region_writeback((void *)barrier, sizeof(xipc_barrier_t));
#endif
  XIPC_LOG("Initialized barrier @ %p\n", barrier);
  return XIPC_OK;
}

/* Checks if barrier has reached the exit state. Can be called from within 
 * interrupt context when the XosCond object gets signalled.
 *
 * arg    : pointer to the barrier
 * val    : ignored
 * thread : pointer to thread structure
 *
 * Returns 0 if the barrier has not reached exit state, i.e the condition is 
 * not satisfied; else returns 1.
 */
static int32_t
xipc_cond_barrier_exit(void *arg, int32_t val, void *thread)
{ 
  xipc_barrier_t *barrier = (xipc_barrier_t *)arg;
#if XCHAL_DCACHE_SIZE>0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_line_writeback_inv((void *)&barrier->_state);
#endif
  return barrier->_state != XIPC_BARRIER_EXIT ? 0 : 1;
}

/* Wait at barrier by either sleep or spin waiting. If sleep waiting,
 * an external event like an interrupt is triggered to notify this thread.
 * 
 * barrier : barrier to perform the wait
 */
void xipc_barrier_wait(xipc_barrier_t *barrier)
{
  /* Track the processors that are exiting the barrier */
  int num_left;

  XIPC_PROFILE_EVENT(XIPC_PROFILE_BARRIER_ENTER, (uintptr_t)barrier);

  /* Wait until everone from the previous invocation of the barrier has exited
   * the barrier and its state is changed to XIPC_BARRIER_ENTER. Since the
   * others from the previous invocation are already in the process of exiting
   * just spin since we expect the state to change quickly. */
  while (xipc_load((volatile uint32_t *)(void *)&barrier->_state) == 
         XIPC_BARRIER_EXIT)
    xipc_delay(16);

  XIPC_LOG("Arriving at barrier @ %p\n", barrier);

  uint32_t curr_count = 
           xipc_atomic_int_increment((volatile int32_t *)&barrier->_count, 1);

  /* The last one to reach the barrier changes the state to XIPC_BARRIER_EXIT 
   * else wait for the barrier to change state to XIPC_BARRIER_EXIT. */
  if (curr_count == barrier->_num_waiters) {
    xipc_store(XIPC_BARRIER_EXIT, 
               (volatile uint32_t *)(void *)&barrier->_state);
    num_left = 
        xipc_atomic_int_increment((volatile int32_t *)&barrier->_count, -1);
    if (barrier->_wait_kind == XIPC_SLEEP_WAIT) {
      /* Notify the other processors that are waiting at the barrier */
      uint32_t pid;
      uint32_t n = xipc_get_master_pid() + xipc_get_num_procs();
      uint32_t my_pid = xipc_get_proc_id();
#pragma loop_count min=1
      for (pid = xipc_get_master_pid(); pid < n; pid++) {
        if (pid != my_pid) {
          if (barrier->_wait_pids[pid] != 0) {
            xipc_proc_notify(pid);
          }   
        } else {
          xipc_self_notify();
        }
      }
    }
  } else {
    /* While the last processor to enter the barrier has not changed the state 
     * to XIPC_BARRIER_EXIT, sleep/spin wait at the barrier. */
    if (barrier->_wait_kind == XIPC_SLEEP_WAIT) {
      while (xipc_load((volatile uint32_t *)(void *)&barrier->_state) != 
             XIPC_BARRIER_EXIT) {
        uint32_t ps = xipc_disable_interrupts();
        if (xipc_load((volatile uint32_t *)(void *)&barrier->_state) != 
              XIPC_BARRIER_EXIT)  {
          XIPC_LOG("Sleep waiting on barrier @ %p\n", barrier);
          xipc_cond_thread_block(xipc_cond_barrier_exit, barrier);
        }
        xipc_enable_interrupts(ps);
      }
    } else {
      while (xipc_load((volatile uint32_t *)(void *)&barrier->_state) != 
             XIPC_BARRIER_EXIT) {
        xipc_delay(16);
      }
    }
    num_left = 
        xipc_atomic_int_increment((volatile int32_t *)&barrier->_count, -1);
  }
  
  /* The last one to leave resets the state to XIPC_BARRIER_ENTER */
  if (num_left == 0) {
    XIPC_LOG("Resetting barrier @ %p\n", barrier);
    xipc_store(XIPC_BARRIER_ENTER, 
               (volatile uint32_t *)(void *)&barrier->_state);
  }

  XIPC_PROFILE_EVENT(XIPC_PROFILE_BARRIER_LEAVE, (uintptr_t)barrier);

  XIPC_LOG("Leaving barrier @ %p\n", barrier);
}

/* Barrier synchronization for all procs in the subsystem. Does not
 * require initialization. Typically used during bootup synchronization.
 * Synchronization is performed with proc with id 0 as the master processor.
 *
 * sync : Array of xipc_reset_sync_t structures, one per processor for 
 *        initial synchronization. Note, each xipc_reset_sync_t element in
 *        the array is assumed to be max dache line size across all processors.
 *
 * Returns XIPC_OK, if successful, else return XIPC_ERROR_INTERNAL
 */
xipc_status_t xipc_reset_sync(volatile xipc_reset_sync_t *sync)
{
  uint32_t pid;
  uint32_t my_pid = xipc_get_proc_id();
  uint32_t master_xipc_pid = xipc_get_master_pid();
  uint32_t num_xipc_procs = xipc_get_num_procs();
  volatile xipc_reset_sync_t *p_sync;
  const uint8_t XIPC_RESET_SYNC_POST = 128;
  /* Each element in the reset sync array is max dcache line size across
   * all processors. Convert to units of uint32s */
  uint32_t reset_sync_elem_inc = xipc_get_max_dcache_linesize()/4;

  if (sizeof(xipc_reset_sync_t) != XIPC_RESET_SYNC_STRUCT_SIZE) {
    XIPC_LOG("xipc_reset_sync:  Internal Error!. Expecting sizeof(xipc_reset_sync_t) to be %d, but got size %d", XIPC_RESET_SYNC_STRUCT_SIZE, sizeof(xipc_reset_sync_t));
    return XIPC_ERROR_INTERNAL;
  }

  p_sync = sync;
  
  if (my_pid == master_xipc_pid) {
    /* Initialize each proc's reset_sync location with their respective 
     * proc ids */
    for (pid = master_xipc_pid; pid < master_xipc_pid+num_xipc_procs; ++pid) {
      xipc_store(pid, &p_sync->_loc);
      p_sync += reset_sync_elem_inc;
    }
    /* Wait for other procs to set their respective reset_sync location to 0 */
    p_sync = sync + reset_sync_elem_inc;
    for (pid = master_xipc_pid+1; pid < master_xipc_pid+num_xipc_procs; ++pid) {
      while (xipc_load(&p_sync->_loc) != 0)
        ;
      /* Re-initialize each proc's reset_sync location with its proc id */
      xipc_store(pid, &p_sync->_loc);
      p_sync += reset_sync_elem_inc;
    }
#pragma flush_memory
    /* All procs have reached the barrier. Write a sentinal to master proc's
     * reset_sync location to let all other procs know the same. */
    xipc_store(XIPC_RESET_SYNC_POST, &sync->_loc);
  } else {
    p_sync += ((my_pid - master_xipc_pid)*reset_sync_elem_inc);
    /* Initialize my reset_sync location to 0 */
    xipc_store(0, &p_sync->_loc);
    /* Wait for my reset_sync location to be set to my proc id by proc 0 */
    while (xipc_load(&p_sync->_loc) != my_pid)
      ;
    /* Re-initialize my reset_sync location to 0 */
    xipc_store(0, &p_sync->_loc);
#pragma flush_memory
    /* Wait until all other procs have also reached the barrier by waiting
     * on master proc to write the sentinal. Master proc sets the sentinal in 
     * its reset_sync location when all procs have reached the barrier */
    while (xipc_load(&sync->_loc) != XIPC_RESET_SYNC_POST)
      ;
  }
  return XIPC_OK;
}
