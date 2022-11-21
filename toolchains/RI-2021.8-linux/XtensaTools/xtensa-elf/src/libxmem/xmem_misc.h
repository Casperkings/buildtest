/* Copyright (c) 2019 Cadence Design Systems, Inc.
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

#ifndef __XMEM_MISC_H__
#define __XMEM_MISC_H__

#include <stdint.h>
#include <stdlib.h>
#include <xtensa/xtutil.h>
#include <xtensa/config/core-isa.h>
#if XCHAL_HAVE_CCOUNT
#include <xtensa/tie/xt_timer.h>
#endif
#if XCHAL_HAVE_NSA
#include <xtensa/tie/xt_misc.h>
#endif
#include "xmem_sys.h"

#if XCHAL_NUM_DATARAM > 0
#  if XCHAL_NUM_DATARAM > 1 && XCHAL_DATARAM1_PADDR < XCHAL_DATARAM0_PADDR
#    define XMEM_BANK_PREFERRED_BANK __attribute__((section(".dram1.data")))
#  else
#    define XMEM_BANK_PREFERRED_BANK __attribute__((section(".dram0.data")))
#  endif
#else
#define XMEM_BANK_PREFERRED_BANK
#endif

#ifdef XMEM_DEBUG
#define XMEM_INLINE
#else
#define XMEM_INLINE  __attribute__((always_inline))
#endif

__attribute__((unused)) static void 
xmem_delay(int delay_count) 
{
  int i;
  for (i = 0; i < delay_count; i++)
    asm volatile ("_nop");
}

#if XCHAL_HAVE_CCOUNT
__attribute__((unused)) static XMEM_INLINE uint32_t
xmem_get_clock()
{
  return XT_RSR_CCOUNT();
}
#else
__attribute__((unused)) static XMEM_INLINE uint32_t
xmem_get_clock()
{
  return 0;
}
#endif

#if XCHAL_HAVE_PRID
__attribute__((unused)) static XMEM_INLINE uint32_t
xmem_get_proc_id()
{
  return XT_RSR_PRID();
}
#else
__attribute__((unused)) static XMEM_INLINE uint32_t
xmem_get_proc_id()
{
  return 0;
}
#endif // XCHAL_HAVE_PRID

#ifdef XMEM_DEBUG
static void
xmem_log(const char *fmt, ...)
{
  int l = 0;
  char _lbuf[1024];
  l += xt_sprintf(&_lbuf[l], "XMEM_LOG: PROC%d: tid:%d ", 
                  xmem_get_proc_id(), xmem_get_thread_id());
  va_list argp;
  va_start(argp, fmt);
  xt_vsprintf(&_lbuf[l], fmt, argp);
  va_end(argp);
  xt_printf("%s", _lbuf);
}

#define XMEM_LOG(...)                   xmem_log(__VA_ARGS__);
#define XMEM_ABORT(...)                 { xmem_log(__VA_ARGS__); exit(-1); }
#define XMEM_ASSERT(C, ...)             { if (!(C)) XMEM_ABORT(__VA_ARGS__); }
#define XMEM_CHECK(C, R, ...)           { if (!(C)) {                         \
                                            XMEM_LOG(__VA_ARGS__);            \
                                            return R;                         \
                                          }                                   \
                                        }
#define XMEM_CHECK_STATUS(C, S, R, label, ...) { if (!(C)) {                  \
                                                   XMEM_LOG(__VA_ARGS__);     \
                                                   S = R;                     \
                                                   goto label;                \
                                                 }                            \
                                               }
#else
#define XMEM_LOG(...)
#define XMEM_ABORT(...)                 { exit(-1); }
#define XMEM_ASSERT(C, ...)             { if (!(C)) {                         \
                                            _Pragma("frequency_hint NEVER")   \
                                            XMEM_ABORT(__VA_ARGS__);          \
                                          }                                   \
                                        }
#define XMEM_CHECK(C, R, ...)           { if (!(C)) {                         \
                                            _Pragma("frequency_hint NEVER")   \
                                            return R;                         \
                                          }                                   \
                                        }
#define XMEM_CHECK_STATUS(C, S, R, label, ...) {                              \
                                          if (!(C)) {                         \
                                            _Pragma("frequency_hint NEVER")   \
                                            S = R;                            \
                                            goto label;                       \
                                          }                                   \
                                        }
#endif // XMEM_DEBUG

#ifdef XMEM_VERIFY
#define XMEM_VERIFY_ASSERT(C, ...) { if (!(C)) XMEM_ABORT(__VA_ARGS__); }
#else
#define XMEM_VERIFY_ASSERT(C, ...)
#endif

/* Return index of msb bit set in 'n', assuming n is a power of 2 */
__attribute__((unused)) static XMEM_INLINE uint32_t
xmem_find_msbit(uint32_t n) {
#ifdef XMEM_VERIFY
  if (n & (n-1)) {
    XMEM_ABORT("xmem_find_msbit: n (%d) is not a power of 2\n", n);
  }
#endif
#if XCHAL_HAVE_NSA
  return 31 - XT_NSAU(n);
#else
  uint32_t nbits = 0;
  while (n && (n & 1) == 0) {
    n >>= 1;
    nbits++;
  }
  return nbits;
#endif
}

#ifdef XMEM_DEBUG
__attribute__((unused)) static XMEM_INLINE void
xmem_print_bitvec(const char *msg, const uint32_t *bitvec, uint32_t num_bits) {
  int i;
  char b[256];
  sprintf(b, "%s: (%d-bits) ", msg, num_bits);
  int num_words = (num_bits+31)>>5;
  xt_printf("%s", b);
  for (i = num_words-1; i >= 0; i--) {
    xt_printf("0x%x,", bitvec[i]);
  }
  xt_printf("\n");
}
#endif

/* Function that toggles 'num_bits' starting from the bit position 'pos'
 * of the bitvector 'bitvec'. 'bitvec' is an arbitrary length bitvector of
 * 'num_bits_in_bitvec' length that is stored in an array of 32-bit words. 
 *
 * bitvec             : arbitrary length bitvector
 * num_bits_in_bitvec : number of bits in the bitvector
 * pos                : position within the bitvector to start toggling bits
 * num_bits           : number of bits to toggle
 *
 * Returns void
 */
__attribute__((unused)) static XMEM_INLINE void
xmem_toggle_bitvec(uint32_t *bitvec,
                   uint32_t num_bits_in_bitvec,
                   uint32_t pos,
                   uint32_t num_bits)
{
  (void)num_bits_in_bitvec;
#ifdef XMEM_VERIFY
  if (pos >= num_bits_in_bitvec) {
    XMEM_ABORT("xmem_toggle_bitvec: pos %d has to be between 0 and %d\n",
               pos, num_bits_in_bitvec-1);
  }
  if (num_bits > num_bits_in_bitvec) {
    XMEM_ABORT("xmem_toggle_bitvec: num_bits %d has to be <= %d\n",
               num_bits, num_bits_in_bitvec);
  }
#endif
  /* Find the word corresponding to bit position pos */
  uint32_t word_idx = pos >> 5;
  /* Find bit offset within this word */
  uint32_t word_off = pos & 31;
  /* Find remaining bits in word starting from off to end of word */
  uint32_t rem_bits_in_word = 32 - word_off;
  uint32_t nbits = rem_bits_in_word;
  /* If num_bits is greater than the remaining number of bits to end of word
   * toggle the remaining number of bits in word, else only toggle num_bits */
  XT_MOVLTZ(nbits, num_bits, num_bits - rem_bits_in_word);
  /* Form bit mask of the number of bits to toggle */
  uint32_t n = 1 << nbits;
  XT_MOVEQZ(n, 0, nbits - 32);
  /* Align the mask to the word offset */
  uint32_t bmask = (n - 1) << word_off;
  /* Negate the bits in the word */
  *(bitvec + word_idx) ^= bmask;
  num_bits -= nbits;
  word_idx++;
  /* If there are more bits remaining, continue with the next word */
  while (num_bits > 0) {
    nbits = 32;
    /* Form bit mask of either 32 1s or num_bits 1s if num_bits < 32 */
    XT_MOVLTZ(nbits, num_bits, num_bits - 32);
    n = 1 << nbits;
    XT_MOVEQZ(n, 0, nbits - 32);
    bmask = n - 1;
    /* Negate the bits in the word */
    *(bitvec + word_idx) ^= bmask;
    num_bits -= nbits;
    word_idx++;
  }
}

/* Function that finds the leading number of 0s or 1s starting from the bit 
 * position 'pos' of the bitvector 'bitvec'. 'bitvec' is an arbitrary length 
 * bitvector of 'num_bits' length that is stored in an array of 32-bit words. 
 *  
 * bitvec     : arbitrary length bitvector
 * num_bits   : number of bits in the bitvector
 * pos        : position within the bitvector to find leading 1 or 0 count
 * zero_count : If 0, find the 0 count, if 0xffffffff, find 1 count
 *
 * Returns the leading 1 or 0 count
 */
__attribute__((unused)) static uint32_t
xmem_find_leading_zero_one_count(const uint32_t *bitvec,
                                 uint32_t num_bits,
                                 uint32_t pos,
                                 uint32_t zero_count)
{
#ifdef XMEM_VERIFY
  if (pos >= num_bits) {
    XMEM_ABORT("xmem_find_leading_zero_one_count: pos %d has to be between 0 and %d\n", pos, num_bits-1);
  }
#endif
  /* Find the word corresponding to bit position pos */
  uint32_t word_idx = pos >> 5;
  /* Find bit offset within this word */
  uint32_t word_off = pos & 31;
  /* Not all of the word will be in use. If num_bits is < 32, then only
   * num_bits are used else all 32-bits are used */
  uint32_t num_bits_used_in_word = 32;
  XT_MOVLTZ(num_bits_used_in_word, num_bits, (int32_t)(num_bits - 32));
  /* Find remaining bits in word starting from off to end of word */
  uint32_t num_rem_bits_in_word = num_bits_used_in_word - word_off;
  uint32_t num_words_in_bitvec = (num_bits + 31) >> 5;
  uint32_t w = bitvec[word_idx];
  w ^= zero_count;
  uint32_t ws = w >> word_off;
  /* Find position of the least signficant 1b */
  uint32_t ls1b = ws & -ws;
  uint32_t ls1b_pos = xmem_find_msbit(ls1b);
  /* If there are no 1s in the word then the leading count is the remaining
   * bits in the word */
  uint32_t lzc = ls1b_pos;
  XT_MOVEQZ(lzc, num_rem_bits_in_word, ls1b);
  uint32_t num_rem_words = num_words_in_bitvec - (word_idx + 1);
  /* While there is no 1 in the word and there are words remaining, continue */
  while (ls1b == 0 && num_rem_words != 0) {
#pragma frequency_hint NEVER
    num_bits -= 32;
    word_idx++;
    num_rem_words--;
    w = bitvec[word_idx];
    w ^= zero_count;
    ls1b = w & -w;
    ls1b_pos = xmem_find_msbit(ls1b);
    num_bits_used_in_word = 32;
    XT_MOVLTZ(num_bits_used_in_word, num_bits, (int32_t)(num_bits - 32));
    uint32_t l = ls1b_pos;
    XT_MOVEQZ(l, num_bits_used_in_word, ls1b);
    lzc += l;
  }
  return lzc;
}

/* Find number of zero/one bits in bit vector
 *
 * bitvec    : arbitrary length bitvector
 * num_bits  : number of bits in the bitvector
 * one_count : If 0, find the 1 count, if 0xffffffff, find 0 count
 *
 * Returns number of set bits
 */
__attribute__((unused)) static uint32_t
xmem_pop_count(const uint32_t *bitvec,
               uint32_t num_bits,
               uint32_t one_count)
{
  int i = 0;
  unsigned n = num_bits;
  unsigned c = 0;
  while (n > 0) {
    uint32_t v = bitvec[i];
    v ^= one_count;
    v = v - ((v >> 1) & 0x55555555);
    v = (v & 0x33333333) + ((v >> 2) & 0x33333333);
    c += (((v + (v >> 4)) & 0xF0F0F0F) * 0x1010101) >> 24;
    n -= 32;
    i++;
  }
  return c;
}

#if XCHAL_HAVE_EXCLUSIVE
__attribute__((unused))static int32_t 
xmem_atomic_int_conditional_set(volatile int32_t *addr,
                                int32_t from, int32_t to) {
  int32_t val = XT_L32EX(addr);
  int32_t tmp = to;
  if (val == from) {
    #pragma frequency_hint FREQUENT
    XT_S32EX(tmp, (volatile int32_t *)addr);
    XT_GETEX(tmp);
    return tmp;
  }
  return 0;
}

__attribute__((unused)) static void 
xmem_lock(volatile void *lock) {
  if (!lock) {
    #pragma frequency_hint FREQUENT
    return;
  }
  uint32_t pid = xmem_get_proc_id();
  while (xmem_atomic_int_conditional_set((volatile int32_t *)lock, 
                                         0, pid + 1) == 0) {
    #pragma frequency_hint NEVER
    xmem_delay(1000);
  }
}

__attribute__((unused)) static void 
xmem_unlock(volatile void *lock) {
  if (!lock) {
    #pragma frequency_hint FREQUENT
    return;
  }
  uint32_t pid = xmem_get_proc_id();
  /* Flush all previous writes before releasing lock with L32EX/S32EX */
#pragma flush_memory
  while (xmem_atomic_int_conditional_set((volatile int32_t *)lock, 
                                         pid + 1, 0) == 0) {
    #pragma frequency_hint NEVER
    xmem_delay(1000);
  }
}
#else
__attribute__((unused)) static void 
xmem_lock(volatile void *lock)   { (void)lock; }

__attribute__((unused)) static void 
xmem_unlock(volatile void *lock) { (void)lock; }
#endif

#endif // __XMEM_MISC_H__
