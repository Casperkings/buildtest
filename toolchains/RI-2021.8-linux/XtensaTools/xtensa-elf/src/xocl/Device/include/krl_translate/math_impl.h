/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of
 * Cadence Design Systems Inc. Any rights to use, modify, and create
 * derivative works of this file are set forth under the terms of your
 * license agreement with Cadence Design Systems, Inc.
 */

#ifndef __MATH_IMPL_H__
#define __MATH_IMPL_H__

#ifndef __OVERLOADABLE__
#define __OVERLOADABLE__ __attribute__((overloadable))
#endif

#define EXPAND_FOR_ALL_VECTOR_LENGTHS(typeA, typeB)                            \
  CLANG_OCL_MACRO(typeA##2, typeB##2);                                         \
  CLANG_OCL_MACRO(typeA##3, typeB##3);                                         \
  CLANG_OCL_MACRO(typeA##4, typeB##4);                                         \
  CLANG_OCL_MACRO(typeA##8, typeB##8);                                         \
  CLANG_OCL_MACRO(typeA##16, typeB##16)

#define EXPAND_FOR_VECTOR_AND_SCALAR(typeA, typeB)                             \
  CLANG_OCL_MACRO(typeA, typeB);                                               \
  EXPAND_FOR_ALL_VECTOR_LENGTHS(typeA, typeB)

#define CLANG_OCL_MACRO(typeA, typeB)                                          \
  typeA __OVERLOADABLE__ _impl_signbit(typeB x);                               \
  typeA __OVERLOADABLE__ _impl_isinf(typeB x);                                 \
  typeA __OVERLOADABLE__ _impl_isnan(typeB x);                                 \
  typeA __OVERLOADABLE__ _impl_isnormal(typeB x);                              \
  typeA __OVERLOADABLE__ _impl_isunordered(typeB x, typeB y);                  \
  typeA __OVERLOADABLE__ _impl_isfinite(typeB x);
EXPAND_FOR_VECTOR_AND_SCALAR(int, float);
CLANG_OCL_MACRO(int, double);
EXPAND_FOR_ALL_VECTOR_LENGTHS(int64_t, double);
#undef CLANG_OCL_MACRO

#define CLANG_OCL_MACRO(typeA, typeB)                                          \
  typeA __OVERLOADABLE__ _impl_ilogb(typeB x)
EXPAND_FOR_VECTOR_AND_SCALAR(int, float);
EXPAND_FOR_VECTOR_AND_SCALAR(int, double);
#undef CLANG_OCL_MACRO

#define CLANG_OCL_MACRO(typeA, typeB)                                          \
  typeA __OVERLOADABLE__ _impl_round(typeB x);                                 \
  typeA __OVERLOADABLE__ _impl_cos(typeB x);                                   \
  typeA __OVERLOADABLE__ _impl_sin(typeB x);                                   \
  typeA __OVERLOADABLE__ _impl_exp(typeB x);                                   \
  typeA __OVERLOADABLE__ _impl_exp2(typeB x);                                  \
  typeA __OVERLOADABLE__ _impl_log(typeB x);                                   \
  typeA __OVERLOADABLE__ _impl_log2(typeB x);                                  \
  typeA __OVERLOADABLE__ _impl_log10(typeB x);                                 \
  typeA __OVERLOADABLE__ _impl_sqrt(typeB x);                                  \
  typeA __OVERLOADABLE__ _impl_rint(typeB x);                                  \
  typeA __OVERLOADABLE__ _impl_tan(typeB x);                                   \
  typeA __OVERLOADABLE__ _impl_acos(typeB x);                                  \
  typeA __OVERLOADABLE__ _impl_asin(typeB x);                                  \
  typeA __OVERLOADABLE__ _impl_atan(typeB x);                                  \
  typeA __OVERLOADABLE__ _impl_acosh(typeB x);                                 \
  typeA __OVERLOADABLE__ _impl_asinh(typeB x);                                 \
  typeA __OVERLOADABLE__ _impl_atanh(typeB x);                                 \
  typeA __OVERLOADABLE__ _impl_floor(typeB x);                                 \
  typeA __OVERLOADABLE__ _impl_fabs(typeB x);                                  \
  typeA __OVERLOADABLE__ _impl_trunc(typeB x);                                 \
  typeA __OVERLOADABLE__ _impl_ceil(typeB x)
EXPAND_FOR_VECTOR_AND_SCALAR(float, float);
EXPAND_FOR_VECTOR_AND_SCALAR(double, double);
#undef CLANG_OCL_MACRO

#define CLANG_OCL_MACRO(typeA, typeB)                                          \
  typeA __OVERLOADABLE__ _impl_cospi(typeA x);                                 \
  typeA __OVERLOADABLE__ _impl_fdim(typeA x, typeA y);                         \
  typeA __OVERLOADABLE__ _impl_fmod(typeA x, typeA y);                         \
  typeA __OVERLOADABLE__ _impl_hypot(typeA x, typeA y);                        \
  typeA __OVERLOADABLE__ _impl_expm1(typeA x);                                 \
  typeA __OVERLOADABLE__ _impl_sinpi(typeA x);                                 \
  typeA __OVERLOADABLE__ _impl_tanpi(typeA x);                                 \
  typeA __OVERLOADABLE__ _impl_fabsf(typeA x);                                 \
  typeA __OVERLOADABLE__ _impl_logb(typeA x);                                  \
  typeA __OVERLOADABLE__ _impl_cosh(typeA x);                                  \
  typeA __OVERLOADABLE__ _impl_sinh(typeA x);                                  \
  typeA __OVERLOADABLE__ _impl_tanh(typeA x);                                  \
  typeA __OVERLOADABLE__ _impl_rtypeB(typeA x);                                \
  typeA __OVERLOADABLE__ _impl_log1p(typeA x);                                 \
  typeA __OVERLOADABLE__ _impl_cbrt(typeA x);                                  \
  typeA __OVERLOADABLE__ _impl_frexp(typeA x, typeB *exp);                     \
  typeA __OVERLOADABLE__ _impl_modf(typeA x, typeA *iptr);                     \
  typeA __OVERLOADABLE__ _impl_fract(typeA x, typeA *iptr);                    \
  typeA __OVERLOADABLE__ _impl_remquo(typeA x, typeA y, typeB *quo);           \
  typeA __OVERLOADABLE__ _impl_lgamma_r(typeA x, typeB *signp);                \
  typeA __OVERLOADABLE__ _impl_lgamma(typeA x);                                \
  typeA __OVERLOADABLE__ _impl_rootn(typeA x, typeB y);                        \
  typeA __OVERLOADABLE__ _impl_ldexp(typeA x, typeB y);                        \
  typeA __OVERLOADABLE__ _impl_remainder(typeA x, typeA y);                    \
  typeA __OVERLOADABLE__ _impl_pown(typeA x, typeB y);                         \
  typeA __OVERLOADABLE__ _impl_fma(typeA x, typeA y, typeA z);                 \
  typeA __OVERLOADABLE__ _impl_half_powr(typeA x, typeA y);
EXPAND_FOR_VECTOR_AND_SCALAR(float, int);
EXPAND_FOR_VECTOR_AND_SCALAR(double, int);
#undef CLANG_OCL_MACRO

#define CLANG_OCL_MACRO(typeA, typeB)                                          \
  typeA __OVERLOADABLE__ _impl_nan(typeB nancode);
EXPAND_FOR_ALL_VECTOR_LENGTHS(float, uint);
EXPAND_FOR_VECTOR_AND_SCALAR(double, ulong);
#undef CLANG_OCL_MACRO

#define CLANG_OCL_MACRO(typeA, typeB)                                          \
  typeA __OVERLOADABLE__ _impl_ldexp(typeA x, int y)
EXPAND_FOR_ALL_VECTOR_LENGTHS(float, float);
EXPAND_FOR_ALL_VECTOR_LENGTHS(double, double);
#undef CLANG_OCL_MACRO

#define CLANG_OCL_MACRO(typeA, typeB)                                          \
  typeA __OVERLOADABLE__ _impl_pow(typeB x, typeB y);                          \
  typeA __OVERLOADABLE__ _impl_powr(typeB x, typeB y);                         \
  typeA __OVERLOADABLE__ _impl_nextafter(typeB x, typeB y);                    \
  typeA __OVERLOADABLE__ _impl_fmin(typeB x, typeB y);                         \
  typeA __OVERLOADABLE__ _impl_fmax(typeB x, typeB y);                         \
  typeA __OVERLOADABLE__ _impl_atan2(typeB x, typeB y);                        \
  typeA __OVERLOADABLE__ _impl_copysign(typeB x, typeB y)
EXPAND_FOR_VECTOR_AND_SCALAR(float, float);
EXPAND_FOR_VECTOR_AND_SCALAR(double, double);
#undef CLANG_OCL_MACRO

#undef EXPAND_FOR_ALL_VECTOR_LENGTHS
#undef EXPAND_FOR_VECTOR_AND_SCALAR

float __OVERLOADABLE__ MCW_HUGE_VALF(void);
float __OVERLOADABLE__ MCW_NAN(void);
float __OVERLOADABLE__ MCW_INFINITY(void);

#define printf __impl_printf
int __OVERLOADABLE__ __impl_printf(const __constant char *format, ...);

#endif // __MATH_IMPL_H__
