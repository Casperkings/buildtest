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
#ifndef __XIPC_PRIMITIVES_H__
#define __XIPC_PRIMITIVES_H__

/*! @file */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <xtensa/hal.h>
#include <xtensa/config/core.h>
#if (XCHAL_HAVE_RELEASE_SYNC || XCHAL_HAVE_S32C1I)
#include <xtensa/tie/xt_sync.h>
#endif

#if !XCHAL_DCACHE_IS_COHERENT
/*! Write back modified cache lines where data is present to main memory  */
#define XIPC_WRITE_BACK_ELEMENT(x) xthal_dcache_region_writeback((void *)x, sizeof(*x))

/*! Invalidate cache lines where data is present to main memory */
#define XIPC_INVALIDATE_ELEMENT(x) xthal_dcache_region_invalidate((void *)x, sizeof(*x))

/*! Write back and invalidate cache lines where data is present to
  * main memory  */
#define XIPC_WRITE_BACK_INVALIDATE_ELEMENT(x) xthal_dcache_region_writeback_inv((void *)x, sizeof(*x))

/*! Write back modified cache lines where an array \a x is present to main 
 *  memory */
#define XIPC_WRITE_BACK_ARRAY(x) xthal_dcache_region_writeback((void *)x, sizeof(x))

/*! Invalidate cache lines where an array \a x is present to main 
 *  memory */
#define XIPC_INVALIDATE_ARRAY(x) xthal_dcache_region_invalidate((void *)x, sizeof(x))

/*! Write back and invalidate cache lines where an array \a x is present to 
 *  main memory */
#define XIPC_WRITE_BACK_INVALIDATE_ARRAY(x) xthal_dcache_region_writeback_inv((void *)x, sizeof(x))

/*! Write back modified cache lines where an \a num_elements of array \a x is 
 *  present to main memory */
#define XIPC_WRITE_BACK_ARRAY_ELEMENTS(x, num_elements) xthal_dcache_region_writeback((void *)x, sizeof(*x) * num_elements)

/*! Invalidate cache lines where an \a num_elements of array \a x is 
 *  present to main memory */
#define XIPC_INVALIDATE_ARRAY_ELEMENTS(x, num_elements) xthal_dcache_region_invalidate((void *)x, sizeof(*x) * num_elements)

/*! Write back and invalidate cache lines where an \a num_elements of 
 *  array \a x is present to main memory */
#define XIPC_WRITE_BACK_INVALIDATE_ARRAY_ELEMENTS(x, num_elements) xthal_dcache_region_writeback_inv((void *)x, sizeof(*x) * num_elements)
#else
#define XIPC_WRITE_BACK_ELEMENT(x)
#define XIPC_INVALIDATE_ELEMENT(x)
#define XIPC_WRITE_BACK_INVALIDATE_ELEMENT(x)
#define XIPC_WRITE_BACK_ARRAY(x)
#define XIPC_INVALIDATE_ARRAY(x)
#define XIPC_WRITE_BACK_INVALIDATE_ARRAY(x)
#define XIPC_WRITE_BACK_ARRAY_ELEMENTS(x, num_elements)
#define XIPC_INVALIDATE_ARRAY_ELEMENTS(x, num_elements)
#define XIPC_WRITE_BACK_INVALIDATE_ARRAY_ELEMENTS(x, num_elements)
#endif

/*! Integer type to perform atomic operations */
typedef unsigned int xipc_atomic_int_t;

/*!
 * Atomically increment the integer.
 *
 * \param integer Points to an initialized integer.
 * \param amount  How much to increment.
 * \return        Returns incremented value.
 */
static int
xipc_atomic_int_increment(xipc_atomic_int_t *integer, int amount);

/*!
 * Attempt to atomically set the value of integer. Return \a from if successful,
 * else a value that is not \a from.
 *
 * \param integer Points to an initialized integer.
 * \param from    Believed value of the integer.
 * \param to      New value.
 * \return        \a from if successful, else returns value that is not \a from.
 */
static int
xipc_atomic_int_conditional_set(xipc_atomic_int_t *integer, int from, int to);

/*!
 * Attempt to atomically increment the integer. Return \a prev if successful,
 * else returns a value that is not \a prev.
 *
 * \param integer Points to an initialized integer.
 * \param amount  How much to increment.
 * \param prev    Believed value of the integer.
 * \return        \a prev if successful, else returns value that is not \a prev.
 */
static int
xipc_atomic_int_conditional_increment(xipc_atomic_int_t *integer, 
                                      int amount, 
                                      int prev);
/*!
 * Attempt to atomically sets the value of integer. Returns a boolean to denote
 * success/failure.
 *
 * \param integer Points to an initialized integer.
 * \param from    Believed value of the integer.
 * \param to      New value.
 * \return        1 if successful, else returns 0.
 */
static int
xipc_atomic_int_conditional_set_bool(xipc_atomic_int_t *integer, 
                                     int from,
                                     int to);

/*!
 * Attempt to atomically increment the integer. Returns a boolean to denote
 * success/failure.
 *
 * \param integer Points to an initialized integer.
 * \param amount  How much to increment.
 * \param prev    Believed value of the integer.
 * \return        1 if successful, else returns 0.
 */
static int
xipc_atomic_int_conditional_increment_bool(xipc_atomic_int_t *integer, 
                                           int amount, 
                                           int prev);

/*!
 * Acquire a spin lock.
 *
 * \param lock Object to be locked. Needs to be initialized to zero first.
 */
static void xipc_simple_spinlock_acquire(xipc_atomic_int_t *lock);

/*!
 * Release a spin lock.
 *
 * \param lock Object to be released. 
 */
static void xipc_simple_spinlock_release(xipc_atomic_int_t *lock);

static inline uint32_t xipc_prid()
{
#if XCHAL_HAVE_PRID
  return XT_RSR_PRID();
#else
  return 0;
#endif
}

static inline void xipc_spin()
{
  int i;
  for (i = 0; i < 16; i++)
    __asm__ volatile ("_nop");
}

static inline xipc_atomic_int_t
xipc_coherent_l32ai(xipc_atomic_int_t *address)
{ 
  xthal_dcache_line_invalidate(address);
#if XCHAL_HAVE_RELEASE_SYNC
  return XT_L32AI(address, 0);
#else
#warning "Synchronize option not supported. Cannot perform load acquires."
  return *address;
#endif
}

static inline void
xipc_coherent_s32ri(xipc_atomic_int_t value, xipc_atomic_int_t *address)
{ 
#if XCHAL_HAVE_RELEASE_SYNC
  XT_S32RI(value, address, 0);
#else
#warning "Synchronize option not supported. Cannot perform release stores."
  *address = value;
#endif
  xthal_dcache_line_writeback(address);
}

/*! 
 * Initialize atomic integer.
 * 
 * \param integer Object to be initialized.
 * \param value   Value to be initialized with.
 */
static inline void
xipc_atomic_int_init(xipc_atomic_int_t *integer, int value)
{
  xipc_coherent_s32ri(value, integer);
}

/*! 
 * Get value of atomic integer.
 * 
 * \param integer Pointer to atomic integer object.
 * \return        The value of the atomic integer.
 */
static inline int
xipc_atomic_int_value(xipc_atomic_int_t *integer)
{
  return xipc_coherent_l32ai(integer);
}

#if XCHAL_HAVE_S32C1I
static inline int
xipc_atomic_int_increment(xipc_atomic_int_t *integer, int amount)
{
  int val;
  int saved;
  val = xipc_coherent_l32ai(integer);
  do {
    XT_WSR_SCOMPARE1(val);
    saved = val;
    val = val + amount;
    XT_S32C1I(val, integer, 0);
  } while (val != saved);
  return val + amount;
}

static inline int
xipc_atomic_int_conditional_set(xipc_atomic_int_t *integer, int from, int to)
{
  int val;
  val = xipc_coherent_l32ai(integer);
  if (val == from) {
    XT_WSR_SCOMPARE1(from);
    val = to;
    XT_S32C1I(val, integer, 0);
  }
  return val;
} 

static inline int
xipc_atomic_int_conditional_increment(xipc_atomic_int_t *integer, 
                                      int amount, 
                                      int prev)
{ 
  int val;
  int saved;
  XT_WSR_SCOMPARE1(prev);
  val = prev + amount;
  saved = val;
  XT_S32C1I(val, integer, 0);
  return val;
}

static inline int
xipc_atomic_int_conditional_increment_bool(xipc_atomic_int_t *integer, 
                                           int amount, 
                                           int prev)
{ 
  int val;
  int saved;
  XT_WSR_SCOMPARE1(prev);
  val = prev + amount;
  saved = val;
  XT_S32C1I(val, integer, 0);
  return val == saved;
}

static inline int
xipc_atomic_int_conditional_set_bool(xipc_atomic_int_t *integer, 
                                     int from, 
                                     int to)
{
  int val;
  val = xipc_coherent_l32ai(integer);
  if (val == from) {
    XT_WSR_SCOMPARE1(from);
    val = to;
    XT_S32C1I(val, integer, 0);
    return val == to;
  }
  return 0;
} 

#elif XCHAL_HAVE_EXCLUSIVE
static inline int
xipc_atomic_int_increment(xipc_atomic_int_t *integer, int amount)
{
  int val;
  int tmp = 0;
  while (!tmp) {
    val = XT_L32EX((int *)integer);
    val = val + amount;
    tmp = val;
    XT_S32EX(tmp, (int *)integer);
    XT_GETEX(tmp);
  }
  return val;
}

static inline int
xipc_atomic_int_conditional_set(xipc_atomic_int_t *integer, int from, int to)
{
  int val = XT_L32EX((int *)integer);
  int tmp = to;
  if (val == from) {
    XT_S32EX(tmp, (int *)integer);
    XT_GETEX(tmp);
    if (!tmp)
      val++;
  }
  return val;
}

static inline int
xipc_atomic_int_conditional_increment(xipc_atomic_int_t *integer, 
                                      int amount, 
                                      int prev)
{ 
  int val = XT_L32EX((int *)integer);
  if (val == prev) {
    int tmp = val + amount;
    XT_S32EX(tmp, (int *)integer);
    XT_GETEX(tmp);
    if (!tmp)
      val++;
  }
  return val;
}

static inline int
xipc_atomic_int_conditional_set_bool(xipc_atomic_int_t *integer, 
                                     int from, 
                                     int to)
{
  int val = XT_L32EX((int *)integer);
  int tmp = to;
  if (val == from) {
    XT_S32EX(tmp, (int *)integer);
    XT_GETEX(tmp);
    return tmp;
  }
  return 0;
}

static inline int
xipc_atomic_int_conditional_increment_bool(xipc_atomic_int_t *integer, 
                                           int amount, 
                                           int prev)
{ 
  int val = XT_L32EX((int *)integer);
  if (val == prev) {
    int tmp = val + amount;
    XT_S32EX(tmp, (int *)integer);
    XT_GETEX(tmp);
    return tmp;
  }
  return 0;
}
#else
#warning "Neither conditional store nor exclusive load/store option supported. Atomics not implemented."
static inline int
xipc_atomic_int_increment(xipc_atomic_int_t *integer, int amount)
{
  return *(int *)integer + amount;
}

static inline int
xipc_atomic_int_conditional_set(xipc_atomic_int_t *integer, int from, int to)
{
  int val = *(int *)integer;
  if (val == from)
    *(int *)integer = to;
  return val;
}

static inline int
xipc_atomic_int_conditional_increment(xipc_atomic_int_t *integer, 
                                      int amount, 
                                      int prev)
{ 
  int val = *(int *)integer;
  if (val == prev)
    *(int *)integer = val + amount;
  return val;
}

static inline int
xipc_atomic_int_conditional_set_bool(xipc_atomic_int_t *integer, 
                                     int from, 
                                     int to)
{
  int val = *(int *)integer;
  if (val == from) {
    *(int *)integer = to;
    return 1;
  }
  return 0;
}

static inline int
xipc_atomic_int_conditional_increment_bool(xipc_atomic_int_t *integer, 
                                           int amount, 
                                           int prev)
{ 
  int val = *(int *)integer;
  if (val == prev) {
    *(int *)integer = val + amount;
    return 1;
  }
  return 0;
}
#endif

#if XCHAL_HAVE_S32C1I
static inline void 
xipc_simple_spinlock_acquire(xipc_atomic_int_t *lock) 
{ 
  while (xipc_atomic_int_conditional_set(lock, 0, xipc_prid()+1) != 0) 
    xipc_spin();
}

static inline void 
xipc_simple_spinlock_release(xipc_atomic_int_t *lock) 
{ 
  while (xipc_atomic_int_conditional_set(lock, xipc_prid()+1, 0) != 
         xipc_prid()+1)
    xipc_spin();
}
#elif XCHAL_HAVE_EXCLUSIVE
static inline void 
xipc_simple_spinlock_acquire(xipc_atomic_int_t *lock) 
{ 
  while (xipc_atomic_int_conditional_set_bool(lock, 0, xipc_prid()+1) == 0) 
    xipc_spin();
}

static inline void 
xipc_simple_spinlock_release(xipc_atomic_int_t *lock) 
{ 
/* Flush all previous writes before releasing lock with L32EX/S32EX */
#pragma flush_memory
  while (xipc_atomic_int_conditional_set_bool(lock, xipc_prid()+1, 0) == 0)
    xipc_spin();
}
#else
#warning "Neither conditional store nor exclusive load/store option supported. Atomics not implemented."
static inline void 
xipc_simple_spinlock_acquire(xipc_atomic_int_t *lock) 
{ 
}

static inline void 
xipc_simple_spinlock_release(xipc_atomic_int_t *lock) 
{ 
}
#endif

static inline uint32_t 
xipc_simple_spinlock_owner(xipc_atomic_int_t *lock) 
{
  return xipc_atomic_int_value(lock) - 1;
}

#ifdef __cplusplus
}
#endif

#endif /* __XIPC_PRIMITIVES_H__ */
