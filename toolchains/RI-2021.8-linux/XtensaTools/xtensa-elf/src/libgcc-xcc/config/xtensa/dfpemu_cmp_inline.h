/* Functions for inlining double precision floating point comparison */

/*
 * Copyright (c) 2007-2015 Cadence Design Systems, Inc.
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


#ifndef __DFPEMU_CMP_INLINE_H__
#define __DFPEMU_CMP_INLINE_H__
#include "dfpemu_fp.h"
#include <xtensa/tie/dfpemu_lib.h>

union dfpemu_dblll {
   unsigned long long ll;
   double dbl;
};

inline unsigned long long dfpemu_dbl2ll(double a)
{
  union dfpemu_dblll tmp_a;
  tmp_a.dbl = a;
  return tmp_a.ll;
}
  
inline int dfpemu_eq(double a, double b)
{
  unsigned long long tmp_a = dfpemu_dbl2ll(a);
  unsigned long long tmp_b = dfpemu_dbl2ll(b);
  F64CMPL((unsigned)tmp_a, (unsigned)tmp_b);
  return F64CMPH(tmp_a >> 32, tmp_b >> 32, EQ_OP);
}

inline int dfpemu_ne(double a, double b)
{
  unsigned long long tmp_a = dfpemu_dbl2ll(a);
  unsigned long long tmp_b = dfpemu_dbl2ll(b);
  F64CMPL((unsigned)tmp_a, (unsigned)tmp_b);
  return F64CMPH(tmp_a >> 32, tmp_b >> 32, NE_OP);
}

inline int dfpemu_lt(double a, double b)
{
  unsigned long long tmp_a = dfpemu_dbl2ll(a);
  unsigned long long tmp_b = dfpemu_dbl2ll(b);
  F64CMPL((unsigned)tmp_a, (unsigned)tmp_b);
  return F64CMPH(tmp_a >> 32, tmp_b >> 32, LT_OP);
}

inline int dfpemu_le(double a, double b)
{
  unsigned long long tmp_a = dfpemu_dbl2ll(a);
  unsigned long long tmp_b = dfpemu_dbl2ll(b);
  F64CMPL((unsigned)tmp_a, (unsigned)tmp_b);
  return F64CMPH(tmp_a >> 32, tmp_b >> 32, LE_OP);
}

inline int dfpemu_gt(double a, double b)
{
  unsigned long long tmp_a = dfpemu_dbl2ll(a);
  unsigned long long tmp_b = dfpemu_dbl2ll(b);
  F64CMPL((unsigned)tmp_a, (unsigned)tmp_b);
  return F64CMPH(tmp_a >> 32, tmp_b >> 32, GT_OP);
}

inline int dfpemu_ge(double a, double b)
{
  unsigned long long tmp_a = dfpemu_dbl2ll(a);
  unsigned long long tmp_b = dfpemu_dbl2ll(b);
  F64CMPL((unsigned)tmp_a, (unsigned)tmp_b);
  return F64CMPH(tmp_a >> 32, tmp_b >> 32, GE_OP);
}

#endif /* __DFPEMU_CMP_INLINE_H__ */
