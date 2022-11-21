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
#include <string.h>
#include "xipc_addr.h"
#include "xipc_cqueue_internal.h"
#include "xipc_misc.h"

/* Checks if queue is full 
 *
 * cq : pointer to the queue
 *
 * Returns 0, if full, else returns a non-zero value
 */
static XIPC_INLINE int32_t
_xipc_cqueue_full(xipc_cqueue_t *cq)
{
  /* Queue is full if the free packet queue is empty */
  uint32_t head = cq->_free_head;
  uint32_t tail = cq->_free_tail;
  return head - tail;
}

/* Checks if queue is full 
 *
 * cq : pointer to the queue
 *
 * Returns 0, if full, else returns a non-zero value
 */
int32_t 
xipc_cqueue_full(xipc_cqueue_t *cq)
{
  /* Queue is full if the free packet queue is empty */
#if XCHAL_DCACHE_SIZE > 0 && !XCHAL_DCACHE_IS_COHERENT
  if (cq->_in_non_coherent_cached_mem) {
    xthal_dcache_line_invalidate((void *)&cq->_free_head);
    xthal_dcache_line_invalidate((void *)&cq->_free_tail);
  }
#endif
  return _xipc_cqueue_full(cq);
}

/* Checks if queue is empty 
 *
 * cq : pointer to the queue
 *
 * Returns 0, if empty, else returns a non-zero value
 */
static XIPC_INLINE int32_t 
_xipc_cqueue_empty(xipc_cqueue_t *cq)
{
  /* Queue is empty if the ready packet queue is empty */
  uint32_t head = cq->_ready_head;
  uint32_t tail = cq->_ready_tail;
  return head - tail;
}

/* Checks if queue is empty 
 *
 * cq : pointer to the queue
 *
 * Returns 0, if empty, else returns a non-zero value
 */
int32_t xipc_cqueue_empty(xipc_cqueue_t *cq)
{
  /* Queue is empty if the ready packet queue is empty */
#if XCHAL_DCACHE_SIZE > 0 && !XCHAL_DCACHE_IS_COHERENT
  if (cq->_in_non_coherent_cached_mem) {
    xthal_dcache_line_invalidate((void *)&cq->_ready_head);
    xthal_dcache_line_invalidate((void *)&cq->_ready_tail);
  } 
#endif
  return _xipc_cqueue_empty(cq);
}

/* Check if the queue is empty. Can be called from within 
 * interrupt context when the XosCond object gets signalled.
 *
 * cq     : pointer to queue
 * val    : ignored
 * thread : pointer to thread structure
 *
 * Returns 0 if the queue is empty, i.e the condition is not satisfied; else
 * returns non-zero value.
 */
static int32_t 
xipc_cond_cqueue_empty(void *cqueue, int32_t val, void *thread)
{
  xipc_cqueue_t *cq = (xipc_cqueue_t *)cqueue;
#if XCHAL_DCACHE_SIZE > 0 && !XCHAL_DCACHE_IS_COHERENT
  if (cq->_in_non_coherent_cached_mem) {
    xthal_dcache_line_writeback_inv((void *)&cq->_ready_head);
    xthal_dcache_line_writeback_inv((void *)&cq->_ready_tail);
  } 
#endif
  return _xipc_cqueue_empty(cq);
}

/* Checks if queue is full. Can be called from within 
 * interrupt context when the XosCond object gets signalled.
 *
 * cq     : pointer to queue
 * val    : ignored
 * thread : pointer to thread structure
 *
 * Returns 0 if the queue is full, i.e the condition is not satisfied; else
 * returns non-zero value.
 */
static int32_t 
xipc_cond_cqueue_full(void *cqueue, int32_t val, void *thread)
{
  xipc_cqueue_t *cq = (xipc_cqueue_t *)cqueue;
#if XCHAL_DCACHE_SIZE > 0 && !XCHAL_DCACHE_IS_COHERENT
  if (cq->_in_non_coherent_cached_mem) {
    xthal_dcache_line_writeback_inv((void *)&cq->_free_head);
    xthal_dcache_line_writeback_inv((void *)&cq->_free_tail);
  }
#endif
  return _xipc_cqueue_full(cq);
}

/* Returns the number of free packets in the queue
 *
 * cq : pointer to the queue
 *
 * Returns free packet count
 */
uint32_t
xipc_cqueue_free_count(xipc_cqueue_t *cq)
{
  uint32_t head, tail;
#if XCHAL_DCACHE_SIZE > 0 && !XCHAL_DCACHE_IS_COHERENT
  if (cq->_in_non_coherent_cached_mem) {
    xthal_dcache_line_invalidate((void *)&cq->_free_head);
    xthal_dcache_line_invalidate((void *)&cq->_free_tail);
  }
#endif
  head = cq->_free_head;
  tail = cq->_free_tail;
  return xipc_channel_count(head, tail, UINT32_MAX);
}

/* Returns the number of allocated packets in the queue
 *
 * cq : pointer to the queue
 *
 * Returns allocated packet count
 */
uint32_t
xipc_cqueue_alloc_count(xipc_cqueue_t *cq)
{
  uint32_t head, tail;
#if XCHAL_DCACHE_SIZE > 0 && !XCHAL_DCACHE_IS_COHERENT
  if (cq->_in_non_coherent_cached_mem) {
    xthal_dcache_line_invalidate((void *)&cq->_ready_head);
    xthal_dcache_line_invalidate((void *)&cq->_ready_tail);
  }
#endif
  head = cq->_ready_head;
  tail = cq->_ready_tail;
  return xipc_channel_count(head, tail, UINT32_MAX);
}

/* Allocate a packet from queue. Blocks if the queue is full.  
 *
 * cq  : pointer to the queue
 * pkt : pointer to packet that is allocated
 *
 * Returns void
 */
void
xipc_cqueue_allocate(xipc_cqueue_t *cq, xipc_pkt_t *pkt)
{
  XIPC_PROFILE_EVENT(XIPC_PROFILE_CQUEUE_ALLOCATING, cq->_cqueue_id);

  uint32_t head;
#if XCHAL_DCACHE_SIZE > 0 && !XCHAL_DCACHE_IS_COHERENT
  if (!cq->_in_non_coherent_cached_mem) {
#endif
    /* Use lock-free method */
    while (1) {
      head = cq->_free_head;
      /* Block while the queue is full */
      while (_xipc_cqueue_full(cq) == 0) {
        uint32_t ps = xipc_disable_interrupts();
        if (_xipc_cqueue_full(cq) == 0) {
          xipc_cond_thread_block(xipc_cond_cqueue_full, cq);
        }
        xipc_enable_interrupts(ps);
      }
      /* Do an atomic update. If it fails, retry */
#if XCHAL_HAVE_S32C1I
      if (xipc_atomic_int_conditional_set((volatile int32_t *)&cq->_free_head, 
                                          head, 
                                          head+1) == head) {
#else
      if (xipc_atomic_int_conditional_set_bool((volatile int32_t *)
                                               &cq->_free_head, 
                                               head, 
                                               head+1)) {
#endif
        break;
      }
    }
    /* Get the head packet */
    xipc_qpkt_slot_t *pkt_slot = 
                      &cq->_free_packets[head & (XIPC_CQUEUE_NUM_SLOTS-1)];
    /* Initialize the pkt structure */
    pkt->_pkt_ptr = cq->_buffer_proc_addrs[xipc_get_proc_id()] + 
                    pkt_slot->_pkt_ptr;
    pkt->_size = cq->_packet_size;
#if XCHAL_DCACHE_SIZE > 0 && !XCHAL_DCACHE_IS_COHERENT
  } else {
    /* xipc_cqueue_t object located in non-coherent cached memory. 
     * Acquire the lock to protect from concurrent access. */
    xipc_spin_lock_acquire(&cq->_lock);
    xthal_dcache_region_invalidate((void *)cq, sizeof(xipc_cqueue_t));
    head = cq->_free_head;
    /* Block while the queue is full */
    while (_xipc_cqueue_full(cq) == 0) {
      uint32_t ps = xipc_disable_interrupts();
      if (_xipc_cqueue_full(cq) == 0) {
        xipc_spin_lock_release(&cq->_lock);
        xipc_cond_thread_block(xipc_cond_cqueue_full, cq);
        /* Re-acquire lock */
        xipc_spin_lock_acquire(&cq->_lock);
        xthal_dcache_region_invalidate((void *)cq, sizeof(xipc_cqueue_t));
        head = cq->_free_head;
      }
      xipc_enable_interrupts(ps);
    }
    /* Get the head packet */
    xipc_qpkt_slot_t *pkt_slot = 
                      &cq->_free_packets[head & (XIPC_CQUEUE_NUM_SLOTS-1)];
    /* Initialize the pkt structure */
    pkt->_pkt_ptr = cq->_buffer_proc_addrs[xipc_get_proc_id()] + 
                    pkt_slot->_pkt_ptr;
    pkt->_size = cq->_packet_size;
    /* Write back modified data and release lock */
    xipc_store(head + 1, &cq->_free_head);
    xipc_spin_lock_release(&cq->_lock);
  }
#endif
  XIPC_LOG("Allocating on cq pkt_ptr: %p size: %d at idx: %d\n", (void *)pkt->_pkt_ptr, pkt->_size, head&(XIPC_CQUEUE_NUM_SLOTS-1));

  XIPC_PROFILE_EVENT(XIPC_PROFILE_CQUEUE_ALLOCATED, cq->_cqueue_id);
}

/* Send an allocated packet to the queue
 *
 * cq  : pointer to the queue
 * pkt : packet to be send
 *
 * Returns void
 */
void
xipc_cqueue_send(xipc_cqueue_t *cq, 
                 xipc_pkt_t *pkt, 
                 xipc_wait_kind_t wait_kind)
{
  if (pkt->_size > cq->_packet_size) {
    XIPC_ABORT("xipc_cqueue_send: packet size %d cannot be > queue packet size %d\n", (int)pkt->_size, (int)cq->_packet_size);
  }
  uint32_t tail;
  int my_pid = xipc_get_proc_id();
#if XCHAL_DCACHE_SIZE > 0 && !XCHAL_DCACHE_IS_COHERENT
  if (!cq->_in_non_coherent_cached_mem) {
#endif
    /* Use lock-free method */
    /* Update the tail end of the ready queue with the packet offset and size 
     * of the packet being sent. */
    /* Atomically update the ready_tail_copy */
    tail = xipc_atomic_int_increment((volatile int32_t *)
                                     &cq->_ready_tail_copy, 1) - 1;
    xipc_qpkt_slot_t *s = &cq->_ready_packets[tail & (XIPC_CQUEUE_NUM_SLOTS-1)];
    s->_pkt_ptr = pkt->_pkt_ptr - cq->_buffer_proc_addrs[my_pid];
    s->_size = pkt->_size;
    /* Wait for the ready queue tail to be same as the tail copy */
    while (cq->_ready_tail != tail)
      ;
    cq->_ready_tail = tail + 1;
#pragma flush_memory
#if XCHAL_DCACHE_SIZE > 0 && !XCHAL_DCACHE_IS_COHERENT
  } else {
    /* xipc_cqueue_t object located in non-coherent cached memory. 
     * Acquire the lock to protect from concurrent access. */
    xipc_spin_lock_acquire(&cq->_lock);
    xthal_dcache_region_invalidate((void *)cq, sizeof(xipc_cqueue_t));
    tail = cq->_ready_tail;
    xipc_qpkt_slot_t *s = &cq->_ready_packets[tail & (XIPC_CQUEUE_NUM_SLOTS-1)];
    s->_pkt_ptr = pkt->_pkt_ptr - cq->_buffer_proc_addrs[my_pid];
    s->_size = pkt->_size;
    cq->_ready_tail = tail + 1;
    /* Write back modified data and release lock */
    xthal_dcache_region_writeback((void *)cq, sizeof(xipc_cqueue_t));
    xipc_spin_lock_release(&cq->_lock);
  }
#endif
  if (wait_kind == XIPC_SLEEP_WAIT) {
    /* Notify all consumers */
    int i;
    int n = cq->_num_cons_procs;
#pragma loop_count min=1
    for (i = 0; i < n; i++) {
      int pid = cq->_cons_wait_pids[i];
      if (pid != my_pid) {
        xipc_proc_notify(pid);
      } else {
        xipc_self_notify();
      }
    }
  }
  XIPC_LOG("Sending on cq pkt_ptr: %p size: %d at idx: %d\n", (void *)pkt->_pkt_ptr, pkt->_size, tail&(XIPC_CQUEUE_NUM_SLOTS-1));

  XIPC_PROFILE_EVENT(XIPC_PROFILE_CQUEUE_SENT, cq->_cqueue_id);
}

/* Receive a packet from the queue. Block if the queue is empty.
 *
 * cq  : pointer to the queue
 * pkt : packet that is being received
 *
 * Returns void
 */
void
xipc_cqueue_recv(xipc_cqueue_t *cq, xipc_pkt_t *pkt)
{
  XIPC_PROFILE_EVENT(XIPC_PROFILE_CQUEUE_RECEIVING, cq->_cqueue_id);

  uint32_t head;
#if XCHAL_DCACHE_SIZE > 0 && !XCHAL_DCACHE_IS_COHERENT
  if (!cq->_in_non_coherent_cached_mem) {
#endif
    /* Use lock-free method */
    while (1) {
      head = cq->_ready_head;
      /* Block while the queue is empty */
      while (_xipc_cqueue_empty(cq) == 0) {
        uint32_t ps = xipc_disable_interrupts();
        if (_xipc_cqueue_empty(cq) == 0) {
          xipc_cond_thread_block(xipc_cond_cqueue_empty, cq);
        }
        xipc_enable_interrupts(ps);
      }
      /* Do an atomic update. If it fails, retry */
#if XCHAL_HAVE_S32C1I
      if (xipc_atomic_int_conditional_set((volatile int32_t *)&cq->_ready_head, 
                                          head, 
                                          head+1) == head) {
#else
      if (xipc_atomic_int_conditional_set_bool((volatile int32_t *)
                                               &cq->_ready_head, 
                                               head, 
                                               head+1)) {
#endif
        break;
      }
    }
    /* Get the head packet */ 
    xipc_qpkt_slot_t *pkt_slot = 
                      &cq->_ready_packets[head & (XIPC_CQUEUE_NUM_SLOTS-1)];
    /* Initialize the pkt structure */
    pkt->_pkt_ptr = cq->_buffer_proc_addrs[xipc_get_proc_id()] + 
                    pkt_slot->_pkt_ptr;
    pkt->_size = pkt_slot->_size;
#if XCHAL_DCACHE_SIZE > 0 && !XCHAL_DCACHE_IS_COHERENT
  } else {
    /* xipc_cqueue_t object located in non-coherent cached memory. 
     * Acquire the lock to protect from concurrent access. */
    xipc_spin_lock_acquire(&cq->_lock);
    xthal_dcache_region_invalidate((void *)cq, sizeof(xipc_cqueue_t));
    head = cq->_ready_head;
    /* Block while the queue is empty */
    while (_xipc_cqueue_empty(cq) == 0) {
      uint32_t ps = xipc_disable_interrupts();
      if (_xipc_cqueue_empty(cq) == 0) {
        xipc_spin_lock_release(&cq->_lock);
        xipc_cond_thread_block(xipc_cond_cqueue_empty, cq);
        /* Re-acquire lock */
        xipc_spin_lock_acquire(&cq->_lock);
        xthal_dcache_region_invalidate((void *)cq, sizeof(xipc_cqueue_t));
        head = cq->_ready_head;
      }
      xipc_enable_interrupts(ps);
    }
    /* Get the head packet */ 
    xipc_qpkt_slot_t *pkt_slot = 
                      &cq->_ready_packets[head & (XIPC_CQUEUE_NUM_SLOTS-1)];
    /* Initialize the pkt structure */
    pkt->_pkt_ptr = cq->_buffer_proc_addrs[xipc_get_proc_id()] + 
                    pkt_slot->_pkt_ptr;
    pkt->_size = pkt_slot->_size;
    /* Write back modified data and release lock */
    xipc_store(head + 1, &cq->_ready_head);
    xipc_spin_lock_release(&cq->_lock);
  }
#endif
  XIPC_LOG("Receiving on cq pkt_ptr: %p size: %d from idx %d\n", (void *)pkt->_pkt_ptr, pkt->_size, head&(XIPC_CQUEUE_NUM_SLOTS-1));

  XIPC_PROFILE_EVENT(XIPC_PROFILE_CQUEUE_RECEIVED, cq->_cqueue_id);
}

/* Release the packet back to the queue
 *
 * cq : pointer to the channel
 * pkt : packet to be released
 *
 * Returns void
 */
void
xipc_cqueue_release(xipc_cqueue_t *cq, 
                    xipc_pkt_t *pkt, 
                    xipc_wait_kind_t wait_kind)
{
  uint32_t tail;
  int my_pid = xipc_get_proc_id();
#if XCHAL_DCACHE_SIZE > 0 && !XCHAL_DCACHE_IS_COHERENT
  if (!cq->_in_non_coherent_cached_mem) {
#endif
    /* Update the tail end of the free queue with the packet offset of the 
     * packet being released. */
    /* Atomically update the free_tail_copy */
    tail = 
      xipc_atomic_int_increment((volatile int32_t *)
                                &cq->_free_tail_copy, 1) - 1;
    xipc_qpkt_slot_t *s = &cq->_free_packets[tail & (XIPC_CQUEUE_NUM_SLOTS-1)];
    s->_pkt_ptr = pkt->_pkt_ptr - cq->_buffer_proc_addrs[my_pid];
    /* Wait for the free queue tail to be same as the tail copy */
    while (cq->_free_tail != tail)
      ;
    cq->_free_tail = tail + 1;
#pragma flush_memory
#if XCHAL_DCACHE_SIZE > 0 && !XCHAL_DCACHE_IS_COHERENT
  } else {
    /* xipc_cqueue_t object located in non-coherent cached memory. 
     * Acquire the lock to protect from concurrent access. */
    xipc_spin_lock_acquire(&cq->_lock);
    xthal_dcache_region_invalidate((void *)cq, sizeof(xipc_cqueue_t));
    tail = cq->_free_tail;
    xipc_qpkt_slot_t *s = &cq->_free_packets[tail & (XIPC_CQUEUE_NUM_SLOTS-1)];
    s->_pkt_ptr = pkt->_pkt_ptr - cq->_buffer_proc_addrs[my_pid];
    cq->_free_tail = tail + 1;
    /* Write back modified data and release lock */
    xthal_dcache_region_writeback((void *)cq, sizeof(xipc_cqueue_t));
    xipc_spin_lock_release(&cq->_lock);
  }
#endif
  if (wait_kind == XIPC_SLEEP_WAIT) {
    /* Notify all producers */
    int i;
    int n = cq->_num_prod_procs;
#pragma loop_count min=1
    for (i = 0; i < n; i++) {
      int pid = cq->_prod_wait_pids[i];
      if (pid != my_pid) {
        xipc_proc_notify(pid);
      } else {
        xipc_self_notify();
      }
    }
  }
  XIPC_LOG("Releasing on cq pkt_ptr: %p size: %d at idx %d\n", (void *)pkt->_pkt_ptr, pkt->_size, tail&(XIPC_CQUEUE_NUM_SLOTS-1));

  XIPC_PROFILE_EVENT(XIPC_PROFILE_CQUEUE_RELEASED, cq->_cqueue_id);
}

/* Try to allocate a packet from the queue. Returns error if a new packet could
 * not be atomically allocated. Does not block.
 *
 * cq  : pointer to the queue
 * pkt : pointer to packet that is allocated
 *
 * Returns XIPC_ERROR_CQUEUE_ALLOCATE if a new packet could not be atomically 
 * allocated. If return value is XIPC_OK, the allocated packet is returned in
 * the parameter pkt.
 */
xipc_status_t
xipc_cqueue_try_allocate(xipc_cqueue_t *cq, xipc_pkt_t *pkt)
{
  uint32_t head;
#if XCHAL_DCACHE_SIZE > 0 && !XCHAL_DCACHE_IS_COHERENT
  if (!cq->_in_non_coherent_cached_mem) {
#endif
    head = cq->_free_head;
    if (_xipc_cqueue_full(cq) == 0) 
      return XIPC_ERROR_CQUEUE_ALLOCATE;
    /* Do an atomic update. If it fails, return error */
#if XCHAL_HAVE_S32C1I
    if (xipc_atomic_int_conditional_set((volatile int32_t *)&cq->_free_head, 
                                        head, 
                                        head+1) != head) {
#else
    if (xipc_atomic_int_conditional_set_bool((volatile int32_t *)
                                             &cq->_free_head, 
                                             head, 
                                             head+1) == 0) {
#endif
      return XIPC_ERROR_CQUEUE_ALLOCATE;
    }
    /* Get the head packet */
    xipc_qpkt_slot_t *pkt_slot = 
                      &cq->_free_packets[head & (XIPC_CQUEUE_NUM_SLOTS-1)];
    /* Initialize the pkt structure */
    pkt->_pkt_ptr = cq->_buffer_proc_addrs[xipc_get_proc_id()] + 
                    pkt_slot->_pkt_ptr;
    pkt->_size = cq->_packet_size;
#if XCHAL_DCACHE_SIZE > 0 && !XCHAL_DCACHE_IS_COHERENT
  } else {
    /* xipc_cqueue_t object located in non-coherent cached memory. 
     * Acquire the lock to protect from concurrent access. */
    xipc_spin_lock_acquire(&cq->_lock);
    xthal_dcache_region_invalidate((void *)cq, sizeof(xipc_cqueue_t));
    head = cq->_free_head;
    if (_xipc_cqueue_full(cq) == 0) {
      xipc_spin_lock_release(&cq->_lock);
      return XIPC_ERROR_CQUEUE_ALLOCATE;
    }
    /* Get the head packet */
    xipc_qpkt_slot_t *pkt_slot = 
                      &cq->_free_packets[head & (XIPC_CQUEUE_NUM_SLOTS-1)];
    /* Initialize the pkt structure */
    pkt->_pkt_ptr = cq->_buffer_proc_addrs[xipc_get_proc_id()] + 
                    pkt_slot->_pkt_ptr;
    pkt->_size = cq->_packet_size;
    /* Write back modified data and release lock */
    xipc_store(head + 1, &cq->_free_head);
    xipc_spin_lock_release(&cq->_lock);
  }
#endif
  XIPC_LOG("Allocating on cq pkt_ptr: %p size: %d at idx: %d\n", (void *)pkt->_pkt_ptr, pkt->_size, head&(XIPC_CQUEUE_NUM_SLOTS-1));
  return XIPC_OK;
}

/* Try to receive a packet from the queue. Returns error if a new packet could
 * not be atomically received. Does not block.
 *
 * cq  : pointer to the queue
 * pkt : packet that is being received
 *
 * Returns XIPC_ERROR_CQUEUE_RECV if a new packet could not be atomically 
 * received. If return value is XIPC_OK, the received packet is returned in
 * the parameter pkt.
 */
xipc_status_t 
xipc_cqueue_try_recv(xipc_cqueue_t *cq, xipc_pkt_t *pkt)
{
  uint32_t head;
#if XCHAL_DCACHE_SIZE > 0 && !XCHAL_DCACHE_IS_COHERENT
  if (!cq->_in_non_coherent_cached_mem) {
#endif
    head = cq->_ready_head;
    if (_xipc_cqueue_empty(cq) == 0)
      return XIPC_ERROR_CQUEUE_RECV;
    /* Do an atomic update. If it fails, return error */
#if XCHAL_HAVE_S32C1I
    if (xipc_atomic_int_conditional_set((volatile int32_t *)&cq->_ready_head, 
                                        head, 
                                        head+1) != head) {
#else
    if (xipc_atomic_int_conditional_set_bool((volatile int32_t *)
                                             &cq->_ready_head, 
                                             head, 
                                             head+1) == 0) {
#endif
      return XIPC_ERROR_CQUEUE_RECV;
    }
    /* Get the head packet */ 
    xipc_qpkt_slot_t *pkt_slot = 
                      &cq->_ready_packets[head & (XIPC_CQUEUE_NUM_SLOTS-1)];
    /* Initialize the pkt structure */
    pkt->_pkt_ptr = cq->_buffer_proc_addrs[xipc_get_proc_id()] + 
                    pkt_slot->_pkt_ptr;
    pkt->_size = pkt_slot->_size;
#if XCHAL_DCACHE_SIZE > 0 && !XCHAL_DCACHE_IS_COHERENT
  } else {
    /* xipc_cqueue_t object located in non-coherent cached memory. 
     * Acquire the lock to protect from concurrent access. */
    xipc_spin_lock_acquire(&cq->_lock);
    xthal_dcache_region_invalidate((void *)cq, sizeof(xipc_cqueue_t));
    head = cq->_ready_head;
    if (_xipc_cqueue_empty(cq) == 0) {
      xipc_spin_lock_release(&cq->_lock);
      return XIPC_ERROR_CQUEUE_RECV;
    }
    /* Get the head packet */ 
    xipc_qpkt_slot_t *pkt_slot = 
                      &cq->_ready_packets[head & (XIPC_CQUEUE_NUM_SLOTS-1)];
    /* Initialize the pkt structure */
    pkt->_pkt_ptr = cq->_buffer_proc_addrs[xipc_get_proc_id()] + 
                    pkt_slot->_pkt_ptr;
    pkt->_size = pkt_slot->_size;
    /* Write back modified data and release lock */
    xipc_store(head + 1, &cq->_ready_head);
    xipc_spin_lock_release(&cq->_lock);
  }
#endif
  XIPC_LOG("Receiving on cq pkt_ptr: %p size: %d from idx %d\n", (void *)pkt->_pkt_ptr, pkt->_size, head&(XIPC_CQUEUE_NUM_SLOTS-1));
  return XIPC_OK;
}

/* Initializes the concurrent queue 
 *
 * cq             : cqueue to be initialized
 * cqueue_id      : unique id for the queue
 * packet_size    : size of each packet in the queue
 * num_packets    : number of packets in the queue.
 * buffer         : buffer to hold the packets. Should be of size 
 *                  packet_size*num_packets 
 * buffer_pid     : if the buffer is in local memory of a processor, 
 *                  its proc_id. If the buffer is in shared memory, this has 
 *                  to be set to -1.
 * prod_proc_ids  : array of proc ids of producer processors of this 
 *                  concurrent queue
 * prod_num_procs : number of producer processors in the prod_proc_ids array
 * cons_proc_ids  : array of proc ids of consumer processors of this concurrent 
 *                  queue
 * cons_num_procs : number of consumer processors in the cons_proc_ids array
 * in_non_coherent_cached_mem : Is the cqueue structure in non-coherent
 *                              cached memory
 *
 * Returns XIPC_OK if successful.
 * else returns one of XIPC_ERROR_INVALID_ARG, XIPC_ERROR_INTERNAL, or
 * XIPC_ERROR_SUBSYSTEM_UNSUPPORTED
 */
xipc_status_t 
xipc_cqueue_initialize(xipc_cqueue_t *cq,
                       uint16_t cqueue_id,
                       uint16_t packet_size,
                       uint16_t num_packets,
                       void *buffer,
                       int32_t buffer_pid,
                       xipc_pid_t *prod_proc_pids,
                       uint32_t prod_num_procs,
                       xipc_pid_t *cons_proc_pids,
                       uint32_t cons_num_procs,
                       uint32_t in_non_coherent_cached_mem)
{
#if !XCHAL_HAVE_S32C1I && !XCHAL_HAVE_EXCLUSIVE
  XIPC_LOG("xipc_cqueue_initialize: Conditional store or load/store exclusive not supported\n");
  return XIPC_ERROR_SUBSYSTEM_UNSUPPORTED;
#endif

  if (sizeof(xipc_cqueue_t) != XIPC_CQUEUE_STRUCT_SIZE) {
    XIPC_LOG("xipc_cqueue_initialize:  Internal Error!. Expecting sizeof(xipc_cqueue_t) to be %d, but got size %d", XIPC_CQUEUE_STRUCT_SIZE, sizeof(xipc_cqueue_t));
    return XIPC_ERROR_INTERNAL;
  }

  if (xipc_get_master_pid()+xipc_get_num_procs() > XIPC_MAX_NUM_PROCS) {
    XIPC_LOG("xipc_cqueue_initialize: Subsystem has %d processors, but the max supported is %d\n", (int)xipc_get_master_pid()+xipc_get_num_procs(), XIPC_MAX_NUM_PROCS);
    return XIPC_ERROR_SUBSYSTEM_UNSUPPORTED;
  }

  if (prod_num_procs == 0 || prod_num_procs > xipc_get_num_procs()) {
    XIPC_LOG("xipc_cqueue_initialize: Expecting prod_num_procs %d to be > 0 and <= %d\n", prod_num_procs, xipc_get_num_procs());
    return XIPC_ERROR_INVALID_ARG;
  }

  if (cons_num_procs == 0 || cons_num_procs > xipc_get_num_procs()) {
    XIPC_LOG("xipc_cqueue_initialize: Expecting cons_num_procs %d to be > 0 and <= %d\n", cons_num_procs, xipc_get_num_procs());
    return XIPC_ERROR_INVALID_ARG;
  }

  if (cq == NULL) {
    XIPC_LOG("xipc_cqueue_initialize: cq is NULL\n");
    return XIPC_ERROR_INVALID_ARG;
  }

  if (buffer == NULL) {
    XIPC_LOG("xipc_cqueue_initialize: buffer is NULL\n");
    return XIPC_ERROR_INVALID_ARG;
  }

  if (prod_proc_pids == NULL) {
    XIPC_LOG("xipc_cqueue_initialize: prod_proc_pids is NULL\n");
    return XIPC_ERROR_INVALID_ARG;
  }

  if (cons_proc_pids == NULL) {
    XIPC_LOG("xipc_cqueue_initialize: cons_proc_pids is NULL\n");
    return XIPC_ERROR_INVALID_ARG;
  }

  if (num_packets == 0 || num_packets > XIPC_CQUEUE_NUM_SLOTS) {
    XIPC_LOG("xipc_cqueue_initialize: num_packets %d should be >=1 and <= %d\n", num_packets, XIPC_CQUEUE_NUM_SLOTS);
    return XIPC_ERROR_INVALID_ARG;
  }

  /* If buffer_pid is negative, it is in the global address space */
  if (buffer_pid >= 0) {
    if (buffer_pid < xipc_get_master_pid() ||
        buffer_pid >= (xipc_get_master_pid()+xipc_get_num_procs())) {
      XIPC_LOG("xipc_cqueue_initialize: Expecting buffer_pid to be >= %d and "
               "< %d\n", xipc_get_master_pid(), 
               xipc_get_master_pid()+xipc_get_num_procs());
      return XIPC_ERROR_INVALID_ARG;
    }
  }

  cq->_lock = 0;
  cq->_cqueue_id = cqueue_id;
  cq->_ready_head = 0;
  cq->_ready_tail = 0;
  cq->_ready_tail_copy = 0;
  int16_t i;
  for (i = 0; i < XIPC_CQUEUE_NUM_SLOTS; i++) {
    cq->_ready_packets[i]._pkt_ptr = 0;
  }
  cq->_free_head = 0;
  cq->_free_tail = num_packets;
  cq->_free_tail_copy = num_packets;
  for (i = 0; i < num_packets; i++) {
    cq->_free_packets[i]._pkt_ptr = i * packet_size;
  }
  cq->_packet_size = packet_size;
  cq->_num_packets = num_packets;

  for (i = 0; i < XIPC_MAX_NUM_PROCS; i++) {
    cq->_prod_wait_pids[i] = 0;
    cq->_cons_wait_pids[i] = 0;
  }

  for (i = 0; i < prod_num_procs; i++) {
    xipc_pid_t pid = prod_proc_pids[i];
    if (pid < xipc_get_master_pid() ||
        pid >= (xipc_get_master_pid()+xipc_get_num_procs())) {
      XIPC_LOG("xipc_cqueue_initialize: Invalid pid %d in prod_proc_ids. Expecting pid to be >= %d and < %d\n", (int)pid, xipc_get_master_pid(), xipc_get_master_pid()+xipc_get_num_procs());
      return XIPC_ERROR_INVALID_ARG;
    }
    cq->_prod_wait_pids[i] = pid;
  }
  cq->_num_prod_procs = prod_num_procs;

  for (i = 0; i < cons_num_procs; i++) {
    xipc_pid_t pid = cons_proc_pids[i];
    if (pid < xipc_get_master_pid() ||
        pid >= (xipc_get_master_pid()+xipc_get_num_procs())) {
      XIPC_LOG("xipc_cqueue_initialize: Invalid pid %d in cons_proc_ids. Expecting pid to be >= %d and < %d\n", (int)pid, xipc_get_master_pid(), xipc_get_master_pid()+xipc_get_num_procs());
      return XIPC_ERROR_INVALID_ARG;
    }
    cq->_cons_wait_pids[i] = pid;
  }
  cq->_num_cons_procs = cons_num_procs;

  xipc_pid_t pid;
  for (pid = xipc_get_master_pid(); 
       pid < xipc_get_master_pid()+xipc_get_num_procs(); ++pid) {
    /* If the buffer is local to the processor use the buffer address else 
     * translate it to the global space */
    if (buffer_pid >= 0) {
      if (pid == buffer_pid)
        cq->_buffer_proc_addrs[pid] = (uintptr_t)buffer;
      else
        cq->_buffer_proc_addrs[pid] = (uintptr_t)xipc_get_sys_addr(buffer, 
                                                                   buffer_pid);
    } else {
      cq->_buffer_proc_addrs[pid] = (uintptr_t)buffer;
    }
  }
  cq->_in_non_coherent_cached_mem = in_non_coherent_cached_mem;

#if XCHAL_DCACHE_SIZE > 0 && !XCHAL_DCACHE_IS_COHERENT
  if (in_non_coherent_cached_mem) {
    xthal_dcache_region_writeback((void *)cq, sizeof(xipc_cqueue_t));
  }
#endif

  XIPC_LOG("Initialized cq @ %p, id: %d, ready_head: %d, ready_tail: %d, free_head: %d, free_tail: %d, packet_size: %d, num_packets: %d, buffer %p, buffer_pid: %d\n", cq, cq->_cqueue_id, cq->_ready_head, cq->_ready_tail, cq->_free_head, cq->_free_tail, cq->_packet_size, cq->_num_packets, buffer, buffer_pid);

  return XIPC_OK;
}

/* Block until a packet is available in the queue
 *
 * cq : pointer to the queue
 *
 * Returns void
 */
void
xipc_cqueue_wait_recv(xipc_cqueue_t *cq)
{
#if XCHAL_DCACHE_SIZE > 0 && !XCHAL_DCACHE_IS_COHERENT
  if (!cq->_in_non_coherent_cached_mem) {
#endif
    /* Use lock-free method */
    /* Block while the queue is empty */
    while (_xipc_cqueue_empty(cq) == 0) {
      uint32_t ps = xipc_disable_interrupts();
      if (_xipc_cqueue_empty(cq) == 0) {
        xipc_cond_thread_block(xipc_cond_cqueue_empty, cq);
      }
      xipc_enable_interrupts(ps);
    }
#if XCHAL_DCACHE_SIZE > 0 && !XCHAL_DCACHE_IS_COHERENT
  } else {
    /* xipc_cqueue_t object located in non-coherent cached memory. 
     * Acquire the lock to protect from concurrent access. */
    xipc_spin_lock_acquire(&cq->_lock);
    xthal_dcache_region_invalidate((void *)cq, sizeof(xipc_cqueue_t));
    /* Block while the queue is empty */
    while (_xipc_cqueue_empty(cq) == 0) {
      uint32_t ps = xipc_disable_interrupts();
      if (_xipc_cqueue_empty(cq) == 0) {
        xipc_spin_lock_release(&cq->_lock);
        xipc_cond_thread_block(xipc_cond_cqueue_empty, cq);
        /* Re-acquire lock */
        xipc_spin_lock_acquire(&cq->_lock);
        xthal_dcache_region_invalidate((void *)cq, sizeof(xipc_cqueue_t));
      }
      xipc_enable_interrupts(ps);
    }
  }
#endif
}
