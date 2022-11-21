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
#ifndef __XIPC_MISC_H__
#define __XIPC_MISC_H__

#include <stdlib.h>
#include <stdint.h>
#include <xtensa/hal.h>
#include <xtensa/xtutil.h>
#include <xtensa/xtruntime.h>
#include <xtensa/core-macros.h>
#include <xtensa/config/core-isa.h>
#include <xtensa/simcall.h>
#if XCHAL_HAVE_S32C1I
#include <xtensa/tie/xt_sync.h>
#endif
#include <xtensa/tie/xt_core.h>
#if XCHAL_HAVE_CCOUNT
#include <xtensa/tie/xt_timer.h>
#endif
#if XCHAL_HAVE_INTERRUPTS
#include <xtensa/tie/xt_interrupt.h>
#endif
#include "xipc_common.h"
#ifdef XIPC_USE_XOS
#include "xipc_xos.h"
#elif XIPC_USE_XTOS
#include "xipc_xtos.h"
#else
#include "xipc_noos.h"
#endif
#ifdef XIPC_CUSTOM_SUBSYSTEM
#include "xipc_custom_sys.h"
#else
#include "xipc_sys.h"
#endif

#ifdef XIPC_DEBUG
#define XIPC_INLINE
#else
#define XIPC_INLINE  __attribute__((always_inline))
#endif

#if XCHAL_NUM_INSTRAM > 0
#define XIPC_IRAM0 __attribute__ ((section(".iram0.text")))
#else
#define XIPC_IRAM0
#endif

#if XCHAL_NUM_DATARAM > 0
#  if XCHAL_NUM_DATARAM > 1 && XCHAL_DATARAM1_PADDR < XCHAL_DATARAM0_PADDR
#    define XIPC_DRAM __attribute__((section(".dram1.data")))
#  else
#    define XIPC_DRAM __attribute__((section(".dram0.data")))
#  endif
#else
#define XIPC_DRAM
#endif

#if XCHAL_HAVE_CCOUNT
__attribute__((unused)) static XIPC_INLINE uint32_t
xipc_get_clock()
{
  return XT_RSR_CCOUNT();
}
#else
__attribute__((unused)) static XIPC_INLINE uint32_t
xipc_get_clock()
{
  return 0;
}
#endif

__attribute__((unused)) static XIPC_INLINE uint32_t
xipc_get_proc_id()
{
  return xipc_get_my_pid();
}

/* Returns a unique id consisting of the proc and thread id.
 * Unique id == 0 is unused. Only the lower 26-bits of the 
 * underlying OS's thread id is used.
 *
 * Returns 32b unique id.
 */
__attribute__((unused)) static XIPC_INLINE uint32_t
xipc_uniq_id(uint32_t pid, uint32_t tid)
{
#ifdef XIPC_USE_XTOS
  return pid+1;
#else
  /* Store proc id + 1 within lower 6-bits */
  uint32_t proc_id = (pid+1) & 0x3f;
  /* Upper 26-bits are for the thread id */
  uint32_t thread_id = tid << 6;
  return thread_id | proc_id;
#endif
}

/* Returns a unique id consisting of current proc and current thread id.
 *
 * Returns 32b unique id.
 */
__attribute__((unused)) static XIPC_INLINE uint32_t
xipc_get_uniq_id()
{
  return xipc_uniq_id(xipc_get_proc_id(), xipc_get_my_thread_id());
}

/* Returns the proc id from the unique id. The proc id is stored in the lower
 * 6-bits of the unique id.
 *
 * uniq_id : unique id 
 * 
 * Returns the proc id
 */
__attribute__((unused)) static XIPC_INLINE uint32_t
xipc_get_uniq_id_proc_id(uint32_t uniq_id)
{
#ifdef XIPC_USE_XTOS
  return uniq_id - 1;
#else
  return (uniq_id & 0x1f) - 1;
#endif
}

/* Returns the thread id from the unique id. The thread id is stored in the 
 * upper 26-bits of the unique id. Note, only the lower 26-bits
 * of the underlying OS's actual thread id is stored in the unique id and is 
 * returned.
 *
 * uniq_id : unique id 
 * 
 * Returns the thread id
 */
__attribute__((unused)) static XIPC_INLINE uint32_t
xipc_get_uniq_id_thread_id(uint32_t uniq_id)
{
  return uniq_id >> 6;
}

__attribute__((unused)) static void 
xipc_log(const char *fmt, ...)
{
  int l = 0; 
  char _lbuf[1024]; 

  l += xt_sprintf(&_lbuf[l], "(%10d) ", (int)xipc_get_clock());

  char _tbuf[256]; 
  int i;
  for (i = 0; i < xipc_get_proc_id()*10; i++) 
    _tbuf[i] = ' '; 
  _tbuf[i] = '\0'; 
  l += xt_sprintf(&_lbuf[l], "%s", _tbuf);

  l += xt_sprintf(&_lbuf[l], "XIPC_LOG: %s:tid:%d ", 
                  xipc_get_proc_name(xipc_get_proc_id()), 
                  xipc_get_my_thread_id());

  va_list argp;
  va_start(argp, fmt);
  xt_vsprintf(&_lbuf[l], fmt, argp); 
  va_end(argp);
  xt_printf("%s", _lbuf);
}

#ifdef XIPC_DEBUG
#define XIPC_LOG(...)     xipc_log(__VA_ARGS__);
#define XIPC_ABORT(...) { xipc_log(__VA_ARGS__); exit(-1); }
#else
#define XIPC_LOG(...)
#define XIPC_ABORT(...) { exit(-1); }
#endif

__attribute__((unused)) static XIPC_INLINE uint32_t 
xipc_load(volatile uint32_t *address)
{
#if XCHAL_DCACHE_SIZE>0 && !XCHAL_DCACHE_IS_COHERENT
#if XCHAL_HAVE_L2_CACHE
  // Assumes all cores share the non-coherent L2-cache
  xthal_dcache_L1_line_invalidate((void *)address);
#else
  xthal_dcache_line_invalidate((void *)address);
#endif
#endif
  return *address;
}

__attribute__((unused)) static XIPC_INLINE void 
xipc_store(uint32_t value, volatile uint32_t *address)
{
  *address = value;
#if XCHAL_DCACHE_SIZE>0 && !XCHAL_DCACHE_IS_COHERENT
#if XCHAL_HAVE_L2_CACHE
  // Assumes all cores share the non-coherent L2-cache
  xthal_dcache_L1_line_writeback((void *)address);
#else
  xthal_dcache_line_writeback((void *)address);
#endif
#endif
}

__attribute__((unused)) static XIPC_INLINE void
xipc_delay(int delay_count)
{
  int i;
  for (i = 0; i < delay_count; i++)
    asm volatile ("_nop");
}

#if XCHAL_HAVE_S32C1I
/* Increment memory value by amount and return the incremented value */
__attribute__((unused)) static XIPC_INLINE int32_t
xipc_atomic_int_increment(volatile int32_t *addr, int32_t amount)
{
  int32_t saved;
  int32_t val = xipc_load((volatile uint32_t *)addr);
  do {
    XT_WSR_SCOMPARE1(val);
    saved = val;
    val = val + amount;
    XT_S32C1I(val, (unsigned int *)addr, 0);
  } while (val != saved);
  return val + amount;
}

/* If successful, returns 'from' else returns value that is not 'from' */
__attribute__((unused)) static XIPC_INLINE int32_t
xipc_atomic_int_conditional_set(volatile int32_t *addr,
                                int32_t from,
                                int32_t to)
{
  int32_t val = xipc_load((volatile uint32_t *)addr);
  if (val == from) {
    XT_WSR_SCOMPARE1(from);
    val = to;
    XT_S32C1I(val, (unsigned int *)addr, 0);
  }
  return val;
}
#elif XCHAL_HAVE_EXCLUSIVE
/* Increment memory value by amount and return the incremented value */
__attribute__((unused)) static XIPC_INLINE int32_t
xipc_atomic_int_increment(volatile int32_t *addr, int32_t amount)
{
  int32_t val;
  int32_t tmp = 0;
  while (!tmp) {
    val = XT_L32EX(addr);
    val = val + amount;
    tmp = val;
    XT_S32EX(tmp, addr);
    XT_GETEX(tmp);
  }
  return val;
}

/* If successful, returns 'from' else returns value that is not 'from' */
__attribute__((unused)) static XIPC_INLINE int32_t
xipc_atomic_int_conditional_set(volatile int32_t *addr,
                                int32_t from,
                                int32_t to)
{
  int32_t val = XT_L32EX(addr);
  int32_t tmp = to;
  if (val == from) {
    XT_S32EX(tmp, (volatile int32_t *)addr);
    XT_GETEX(tmp);
    if (!tmp)
      val++;
  }
  return val;
}

/* If successful, returns 1 else returns 0 */
__attribute__((unused)) static XIPC_INLINE int32_t
xipc_atomic_int_conditional_set_bool(volatile int32_t *addr,
                                     int32_t from,
                                     int32_t to)
{
  int32_t val = XT_L32EX(addr);
  int32_t tmp = to;
  if (val == from) {
    XT_S32EX(tmp, (volatile int32_t *)addr);
    XT_GETEX(tmp);
    return tmp;
  }
  return 0;
}
#else
__attribute__((unused)) static XIPC_INLINE int32_t
xipc_atomic_int_increment(volatile int32_t *addr, int32_t amount)
{
  XIPC_ABORT("xipc_atomic_int_increment: S32C1I/L32EX unsupported\n");
}

__attribute__((unused)) static XIPC_INLINE int32_t
xipc_atomic_int_conditional_set(volatile int32_t *addr,
                                int32_t from,
                                int32_t to)
{
  XIPC_ABORT("xipc_atomic_int_conditional_set: S32C1I/L32EX unsupported\n");
}

/* If successful, returns 1 else returns 0 */
__attribute__((unused)) static XIPC_INLINE int32_t
xipc_atomic_int_conditional_set_bool(volatile int32_t *addr,
                                     int32_t from,
                                     int32_t to)
{
  XIPC_ABORT("xipc_atomic_int_conditional_set_bool: S32C1I/L32EX unsupported\n");
}

#endif

#if XCHAL_HAVE_S32C1I
__attribute__((unused)) static XIPC_INLINE void
xipc_spin_lock_acquire(volatile uint32_t *lock)
{
  xipc_disable_preemption();
  uint32_t pid = xipc_get_proc_id();
  while (xipc_atomic_int_conditional_set((volatile int32_t *)lock, 
                                         0, pid + 1) != 0)
    xipc_delay(16);
}

__attribute__((unused)) static XIPC_INLINE void
xipc_spin_lock_release(volatile uint32_t *lock)
{
  uint32_t pid = xipc_get_proc_id();
  while (xipc_atomic_int_conditional_set((volatile int32_t *)lock, 
                                         pid + 1, 0) != pid + 1)
    xipc_delay(16);
  xipc_enable_preemption();
}
#elif XCHAL_HAVE_EXCLUSIVE
__attribute__((unused)) static XIPC_INLINE void
xipc_spin_lock_acquire(volatile uint32_t *lock)
{
  xipc_disable_preemption();
  uint32_t pid = xipc_get_proc_id();
  while (xipc_atomic_int_conditional_set_bool((volatile int32_t *)lock, 
                                              0, pid + 1) == 0)
    xipc_delay(16);
}

__attribute__((unused)) static XIPC_INLINE void
xipc_spin_lock_release(volatile uint32_t *lock)
{
  uint32_t pid = xipc_get_proc_id();
/* Flush all previous writes before releasing lock with L32EX/S32EX */
#pragma flush_memory
  while (xipc_atomic_int_conditional_set_bool((volatile int32_t *)lock, 
                                              pid + 1, 0) == 0)
    xipc_delay(16);
  xipc_enable_preemption();
}
#else
__attribute__((unused)) static XIPC_INLINE void
xipc_spin_lock_acquire(volatile uint32_t *lock)
{
  XIPC_ABORT("xipc_spin_lock_acquire: S32C1I/L32EX unsupported\n");
}

__attribute__((unused)) static XIPC_INLINE void
xipc_spin_lock_release(volatile uint32_t *lock)
{
  XIPC_ABORT("xipc_spin_lock_release: S32C1I/L32EX unsupported\n");
}
#endif

__attribute__((unused)) static XIPC_INLINE uint32_t
xipc_modulo_add(uint32_t val, uint32_t amount, uint32_t size)
{
  uint32_t tmp = val + amount;
  int32_t d = tmp - size;
  XT_MOVLTZ(d, tmp, d);
  return d;
}

/* Returns the number of allocated entries in the circular buffer
 *
 * nslots    : total number of slots in the circular buffer
 * read_ptr  : points to slot index of the first element where the next read
 *             occurs
 * write_ptr : points to slot index of the element after the last element,
 *             where the next write occurs
 */
__attribute__((unused)) static XIPC_INLINE uint32_t
xipc_channel_count(uint32_t read_ptr, uint32_t write_ptr, uint32_t nslots)
{
  // Check if write_ptr is before/after read_ptr
  int32_t c1 = write_ptr - read_ptr;
  // number of allocated elements, if write is before read
  uint32_t wbr_cnt = nslots + (write_ptr - read_ptr);
  // number of allocated elements, if read is before write
  uint32_t rbw_cnt = write_ptr - read_ptr;
  return c1 >= 0 ? rbw_cnt : wbr_cnt;
}

/* Generate tracing. See description under xipc_profile_event_type_t
 * in xipc_common.h.
 */
__attribute__((unused)) static XIPC_INLINE void
xipc_profile_event(xipc_profile_event_type_t event, uint32_t xipc_obj_id)
{
  register int a2 __asm__ ("a2") = SYS_user_simcall;
  // The first of the 6 arguments is passed as the last argument to the simcall.
  register int a8 __asm__ ("a8") = xipc_obj_id;
  register int a3 __asm__ ("a3") = event;
#ifndef XIPC_USE_XTOS
  register int a4 __asm__ ("a4") = xipc_get_my_thread_id();
#endif
    __asm__ volatile ("simcall"
          :
#ifndef XIPC_USE_XTOS
          : "a" (a2), "a" (a3), "a" (a4), "a" (a8));
#else
          : "a" (a2), "a" (a3), "a" (a8));
#endif
}

#ifdef XIPC_PROFILE
#define XIPC_PROFILE_EVENT(xipc_profile_event_type, xipc_obj_id) \
        xipc_profile_event(xipc_profile_event_type, xipc_obj_id)
#else
#define XIPC_PROFILE_EVENT(xipc_profile_event_type, xipc_obj_id)
#endif

#endif /* __XIPC_MISC_H__ */
