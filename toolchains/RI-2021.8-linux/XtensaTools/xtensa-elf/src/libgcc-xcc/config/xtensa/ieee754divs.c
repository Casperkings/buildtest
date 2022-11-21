/*
 * Copyright (c) 2014-2015 Cadence Design Systems, Inc.
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

#include <xtensa/config/core.h>
#if XCHAL_HAVE_DFP
#include <xtensa/tie/xt_DFP.h>
#else
#if  XCHAL_HAVE_FP
#include <xtensa/tie/xt_FP.h>
#endif
#endif

#define HAVE_USER_OR_VECTOR_FPU \
	(XCHAL_HAVE_USER_DPFPU || XCHAL_HAVE_USER_SPFPU || XCHAL_HAVE_VECTORFPU2005)

#if !XCHAL_HAVE_FP_RECIP || HAVE_USER_OR_VECTOR_FPU
#if defined(XT_RECIPX1_S)
float __recipsf2(float input)
{
  return XT_RECIPX1_S(input);
}
#elif defined(XT_RECIP_S)
float __recipsf2(float input)
{
  return XT_RECIP_S(input);
}
#endif
#endif



#if !XCHAL_HAVE_FP_DIV || HAVE_USER_OR_VECTOR_FPU
#if defined(XT_DIVX1_S)
float __divsf3(float num, float denom)
{
  return XT_DIVX1_S(num, denom);
}
#elif defined(XT_DIV_S)
float __divsf3(float num, float denom)
{
  return XT_DIV_S(num, denom);
}
#endif
#endif
