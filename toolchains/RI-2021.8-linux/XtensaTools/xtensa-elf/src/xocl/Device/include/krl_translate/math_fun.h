/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of
 * Cadence Design Systems Inc. Any rights to use, modify, and create
 * derivative works of this file are set forth under the terms of your
 * license agreement with Cadence Design Systems, Inc.
 */

#include "clang_ocl_declaration_macros.h"

#ifndef __MATH_FUN_H__
#define __MATH_FUN_H__

#define __OVERLOADABLE__ __attribute__((overloadable))

float __OVERLOADABLE__ _impl_acos(float x);
float __OVERLOADABLE__ _impl_acosh(float x);
float __OVERLOADABLE__ _impl_acospi(float x);
float __OVERLOADABLE__ _impl_asin(float x);
float __OVERLOADABLE__ _impl_asinh(float x);
float __OVERLOADABLE__ _impl_asinpi(float x);
float __OVERLOADABLE__ _impl_atan(float x);
float __OVERLOADABLE__ _impl_atan2(float y, float x);
float __OVERLOADABLE__ _impl_atanh(float x);
float __OVERLOADABLE__ _impl_atanpi(float x);
float __OVERLOADABLE__ _impl_atan2pi(float y, float x);
float __OVERLOADABLE__ _impl_cbrt(float);
float __OVERLOADABLE__ _impl_ceil(float);
float __OVERLOADABLE__ _impl_copysign(float x, float y);
float __OVERLOADABLE__ _impl_cos(float);
float __OVERLOADABLE__ _impl_cosh(float);
float __OVERLOADABLE__ _impl_cospi(float);
float __OVERLOADABLE__ _impl_erfc(float);
float __OVERLOADABLE__ _impl_erf(float);
float __OVERLOADABLE__ _impl_exp(float x);
float __OVERLOADABLE__ _impl_exp2(float);
float __OVERLOADABLE__ _impl_exp10(float);
float __OVERLOADABLE__ _impl_expm1(float x);
float __OVERLOADABLE__ _impl_fabs(float);
float __OVERLOADABLE__ _impl_fdim(float x, float y);
float __OVERLOADABLE__ _impl_floor(float);
float __OVERLOADABLE__ _impl_fma(float a, float b, float c);
float __OVERLOADABLE__ _impl_fmax(float x, float y);
float __OVERLOADABLE__ _impl_fmin(float x, float y);
float __OVERLOADABLE__ _impl_fmod(float x, float y);
float __OVERLOADABLE__ _impl_fract(float x, __global float *iptr);
float __OVERLOADABLE__ _impl_fract(float x, __local float *iptr);
float __OVERLOADABLE__ _impl_fract(float x, __private float *iptr);
float __OVERLOADABLE__ _impl_hypot(float x, float y);
float __OVERLOADABLE__ _impl_lgamma(float x);
float __OVERLOADABLE__ _impl_log(float);
float __OVERLOADABLE__ _impl_log2(float);
float __OVERLOADABLE__ _impl_log10(float);
float __OVERLOADABLE__ _impl_log1p(float);
float __OVERLOADABLE__ _impl_logb(float);
float __OVERLOADABLE__ _impl_mad(float a, float b, float c);
float __OVERLOADABLE__ _impl_maxmag(float x, float y);
float __OVERLOADABLE__ _impl_minmag(float x, float y);
float __OVERLOADABLE__ _impl_modf(float x, __global float *iptr);
float __OVERLOADABLE__ _impl_modf(float x, __local float *iptr);
float __OVERLOADABLE__ _impl_modf(float x, __private float *iptr);
float __OVERLOADABLE__ _impl_nextafter(float x, float y);
float __OVERLOADABLE__ _impl_pow(float x, float y);
float __OVERLOADABLE__ _impl_powr(float x, float y);
float __OVERLOADABLE__ _impl_remainder(float x, float y);
float __OVERLOADABLE__ _impl_round(float);
float __OVERLOADABLE__ _impl_rint(float);
float __OVERLOADABLE__ _impl_rsqrt(float);
float __OVERLOADABLE__ _impl_rootn(float x, int y);
float __OVERLOADABLE__ _impl_sin(float);
float __OVERLOADABLE__ _impl_sinh(float);
float __OVERLOADABLE__ _impl_sinpi(float);
float __OVERLOADABLE__ _impl_sqrt(float);
float __OVERLOADABLE__ _impl_tan(float);
float __OVERLOADABLE__ _impl_tanh(float);
float __OVERLOADABLE__ _impl_tanpi(float);
float __OVERLOADABLE__ _impl_tgamma(float);
float __OVERLOADABLE__ _impl_trunc(float);
float __OVERLOADABLE__ _impl_sincos(float x, __global float *cosval);
float __OVERLOADABLE__ _impl_sincos(float x, __local float *cosval);
float __OVERLOADABLE__ _impl_sincos(float x, __private float *cosval);
float __OVERLOADABLE__ _impl_half_cos(float x);
float __OVERLOADABLE__ _impl_half_exp(float x);
float __OVERLOADABLE__ _impl_half_exp2(float x);
float __OVERLOADABLE__ _impl_half_exp10(float x);
float __OVERLOADABLE__ _impl_half_log(float x);
float __OVERLOADABLE__ _impl_half_log2(float x);
float __OVERLOADABLE__ _impl_half_log10(float x);
float __OVERLOADABLE__ _impl_half_powr(float x, float y);
float __OVERLOADABLE__ _impl_half_divide(float x, float y);
float __OVERLOADABLE__ _impl_native_divide(float x, float y);
float __OVERLOADABLE__ _impl_half_recip(float x);
float __OVERLOADABLE__ _impl_half_rsqrt(float x);
float __OVERLOADABLE__ _impl_half_sin(float x);
float __OVERLOADABLE__ _impl_half_sqrt(float x);
float __OVERLOADABLE__ _impl_half_tan(float x);
float __OVERLOADABLE__ _impl_half_cos(float x);
float __OVERLOADABLE__ _impl_native_cos(float x);
float __OVERLOADABLE__ _impl_native_exp(float x);
float __OVERLOADABLE__ _impl_native_exp2(float x);
float __OVERLOADABLE__ _impl_native_exp10(float x);
float __OVERLOADABLE__ _impl_native_log(float x);
float __OVERLOADABLE__ _impl_native_log2(float x);
float __OVERLOADABLE__ _impl_native_log10(float x);
float __OVERLOADABLE__ _impl_native_recip(float x);
float __OVERLOADABLE__ _impl_native_rsqrt(float x);
float __OVERLOADABLE__ _impl_native_sin(float x);
float __OVERLOADABLE__ _impl_native_sqrt(float x);
float __OVERLOADABLE__ _impl_native_tan(float x);
float __OVERLOADABLE__ _impl_lgamma_r(float x, __global int *signp);
float __OVERLOADABLE__ _impl_lgamma_r(float x, __local int *signp);
float __OVERLOADABLE__ _impl_lgamma_r(float x, __private int *signp);
float __OVERLOADABLE__ _impl_ldexp(float x, int k);
float __OVERLOADABLE__ _impl_ldexp(float x, int k);
int __OVERLOADABLE__ _impl_ilogb(float x);
float __OVERLOADABLE__ _impl_frexp(float x, __private int *exp);
float __OVERLOADABLE__ _impl_frexp(float x, __global int *exp);
float __OVERLOADABLE__ _impl_frexp(float x, __local int *exp);
float __OVERLOADABLE__ _impl_nan(uint nancode);
float __OVERLOADABLE__ _impl_pown(float x, int y);
float __OVERLOADABLE__ _impl_remquo(float x, float y, __global int *quo);
float __OVERLOADABLE__ _impl_remquo(float x, float y, __local int *quo);
float __OVERLOADABLE__ _impl_remquo(float x, float y, __private int *quo);

double __OVERLOADABLE__ _impl_acos(double x);
double __OVERLOADABLE__ _impl_acosh(double x);
double __OVERLOADABLE__ _impl_acospi(double x);
double __OVERLOADABLE__ _impl_asin(double x);
double __OVERLOADABLE__ _impl_asinh(double x);
double __OVERLOADABLE__ _impl_asinpi(double x);
double __OVERLOADABLE__ _impl_atan(double x);
double __OVERLOADABLE__ _impl_atan2(double y, double x);
double __OVERLOADABLE__ _impl_atanh(double x);
double __OVERLOADABLE__ _impl_atanpi(double x);
double __OVERLOADABLE__ _impl_atan2pi(double y, double x);
double __OVERLOADABLE__ _impl_cbrt(double);
double __OVERLOADABLE__ _impl_ceil(double);
double __OVERLOADABLE__ _impl_copysign(double x, double y);
double __OVERLOADABLE__ _impl_cos(double);
double __OVERLOADABLE__ _impl_cosh(double);
double __OVERLOADABLE__ _impl_cospi(double);
double __OVERLOADABLE__ _impl_erfc(double);
double __OVERLOADABLE__ _impl_erf(double);
double __OVERLOADABLE__ _impl_exp(double x);
double __OVERLOADABLE__ _impl_exp2(double);
double __OVERLOADABLE__ _impl_exp10(double);
double __OVERLOADABLE__ _impl_expm1(double x);
double __OVERLOADABLE__ _impl_fabs(double);
double __OVERLOADABLE__ _impl_fdim(double x, double y);
double __OVERLOADABLE__ _impl_floor(double);
double __OVERLOADABLE__ _impl_fma(double a, double b, double c);
double __OVERLOADABLE__ _impl_fmax(double x, double y);
double __OVERLOADABLE__ _impl_fmin(double x, double y);
double __OVERLOADABLE__ _impl_fmod(double x, double y);
double __OVERLOADABLE__ _impl_fract(double x, __global double *iptr);
double __OVERLOADABLE__ _impl_fract(double x, __local double *iptr);
double __OVERLOADABLE__ _impl_fract(double x, __private double *iptr);
double __OVERLOADABLE__ _impl_hypot(double x, double y);
double __OVERLOADABLE__ _impl_lgamma(double x);
double __OVERLOADABLE__ _impl_log(double);
double __OVERLOADABLE__ _impl_log2(double);
double __OVERLOADABLE__ _impl_log10(double);
double __OVERLOADABLE__ _impl_log1p(double);
double __OVERLOADABLE__ _impl_logb(double);
double __OVERLOADABLE__ _impl_mad(double a, double b, double c);
double __OVERLOADABLE__ _impl_maxmag(double x, double y);
double __OVERLOADABLE__ _impl_minmag(double x, double y);
double __OVERLOADABLE__ _impl_modf(double x, __global double *iptr);
double __OVERLOADABLE__ _impl_modf(double x, __local double *iptr);
double __OVERLOADABLE__ _impl_modf(double x, __private double *iptr);
double __OVERLOADABLE__ _impl_nextafter(double x, double y);
double __OVERLOADABLE__ _impl_pow(double x, double y);
double __OVERLOADABLE__ _impl_powr(double x, double y);
double __OVERLOADABLE__ _impl_remainder(double x, double y);
double __OVERLOADABLE__ _impl_round(double);
double __OVERLOADABLE__ _impl_rint(double);
double __OVERLOADABLE__ _impl_rsqrt(double);
double __OVERLOADABLE__ _impl_rootn(double x, int y);
double __OVERLOADABLE__ _impl_sin(double);
double __OVERLOADABLE__ _impl_sinh(double);
double __OVERLOADABLE__ _impl_sinpi(double);
double __OVERLOADABLE__ _impl_sqrt(double);
double __OVERLOADABLE__ _impl_tan(double);
double __OVERLOADABLE__ _impl_tanh(double);
double __OVERLOADABLE__ _impl_tanpi(double);
double __OVERLOADABLE__ _impl_tgamma(double);
double __OVERLOADABLE__ _impl_trunc(double);
double __OVERLOADABLE__ _impl_sincos(double x, __global double *cosval);
double __OVERLOADABLE__ _impl_sincos(double x, __local double *cosval);
double __OVERLOADABLE__ _impl_sincos(double x, __private double *cosval);
double __OVERLOADABLE__ _impl_lgamma_r(double x, __global int *signp);
double __OVERLOADABLE__ _impl_lgamma_r(double x, __local int *signp);
double __OVERLOADABLE__ _impl_lgamma_r(double x, __private int *signp);
double __OVERLOADABLE__ _impl_ldexp(double x, int k);
double __OVERLOADABLE__ _impl_ldexp(double x, int k);
int __OVERLOADABLE__ _impl_ilogb(double x);
double __OVERLOADABLE__ _impl_frexp(double x, __private int *exp);
double __OVERLOADABLE__ _impl_frexp(double x, __global int *exp);
double __OVERLOADABLE__ _impl_frexp(double x, __local int *exp);
double __OVERLOADABLE__ _impl_nan(ulong nancode);
double __OVERLOADABLE__ _impl_pown(double x, int y);
double __OVERLOADABLE__ _impl_remquo(double x, double y, __global int *quo);
double __OVERLOADABLE__ _impl_remquo(double x, double y, __local int *quo);
double __OVERLOADABLE__ _impl_remquo(double x, double y, __private int *quo);

#define MATH_MACRO_TYPE(n)                                                     \
  float##n __OVERLOADABLE__ _impl_acos(float##n x);                            \
  float##n __OVERLOADABLE__ _impl_acosh(float##n x);                           \
  float##n __OVERLOADABLE__ _impl_acospi(float##n x);                          \
  float##n __OVERLOADABLE__ _impl_asin(float##n x);                            \
  float##n __OVERLOADABLE__ _impl_asinh(float##n x);                           \
  float##n __OVERLOADABLE__ _impl_asinpi(float##n x);                          \
  float##n __OVERLOADABLE__ _impl_atan(float##n x);                            \
  float##n __OVERLOADABLE__ _impl_atan2(float##n y, float##n x);               \
  float##n __OVERLOADABLE__ _impl_atanh(float##n x);                           \
  float##n __OVERLOADABLE__ _impl_atanpi(float##n x);                          \
  float##n __OVERLOADABLE__ _impl_atan2pi(float##n y, float##n x);             \
  float##n __OVERLOADABLE__ _impl_cbrt(float##n);                              \
  float##n __OVERLOADABLE__ _impl_ceil(float##n);                              \
  float##n __OVERLOADABLE__ _impl_copysign(float##n x, float##n y);            \
  float##n __OVERLOADABLE__ _impl_cos(float##n);                               \
  float##n __OVERLOADABLE__ _impl_cosh(float##n);                              \
  float##n __OVERLOADABLE__ _impl_cospi(float##n);                             \
  float##n __OVERLOADABLE__ _impl_erfc(float##n);                              \
  float##n __OVERLOADABLE__ _impl_erf(float##n);                               \
  float##n __OVERLOADABLE__ _impl_exp(float##n x);                             \
  float##n __OVERLOADABLE__ _impl_exp2(float##n);                              \
  float##n __OVERLOADABLE__ _impl_exp10(float##n);                             \
  float##n __OVERLOADABLE__ _impl_expm1(float##n x);                           \
  float##n __OVERLOADABLE__ _impl_fabs(float##n);                              \
  float##n __OVERLOADABLE__ _impl_fdim(float##n x, float##n y);                \
  float##n __OVERLOADABLE__ _impl_floor(float##n);                             \
  float##n __OVERLOADABLE__ _impl_fma(float##n a, float##n b, float##n c);     \
  float##n __OVERLOADABLE__ _impl_fmax(float##n x, float##n y);                \
  float##n __OVERLOADABLE__ _impl_fmin(float##n x, float##n y);                \
  float##n __OVERLOADABLE__ _impl_fmod(float##n x, float##n y);                \
  float##n __OVERLOADABLE__ _impl_fract(float##n x, __global float##n *iptr);  \
  float##n __OVERLOADABLE__ _impl_fract(float##n x, __local float##n *iptr);   \
  float##n __OVERLOADABLE__ _impl_fract(float##n x, __private float##n *iptr); \
  float##n __OVERLOADABLE__ _impl_hypot(float##n x, float##n y);               \
  float##n __OVERLOADABLE__ _impl_lgamma(float##n x);                          \
  float##n __OVERLOADABLE__ _impl_log(float##n);                               \
  float##n __OVERLOADABLE__ _impl_log2(float##n);                              \
  float##n __OVERLOADABLE__ _impl_log10(float##n);                             \
  float##n __OVERLOADABLE__ _impl_log1p(float##n);                             \
  float##n __OVERLOADABLE__ _impl_logb(float##n);                              \
  float##n __OVERLOADABLE__ _impl_mad(float##n a, float##n b, float##n c);     \
  float##n __OVERLOADABLE__ _impl_maxmag(float##n x, float##n y);              \
  float##n __OVERLOADABLE__ _impl_minmag(float##n x, float##n y);              \
  float##n __OVERLOADABLE__ _impl_modf(float##n x, __global float##n *iptr);   \
  float##n __OVERLOADABLE__ _impl_modf(float##n x, __local float##n *iptr);    \
  float##n __OVERLOADABLE__ _impl_modf(float##n x, __private float##n *iptr);  \
  float##n __OVERLOADABLE__ _impl_nextafter(float##n x, float##n y);           \
  float##n __OVERLOADABLE__ _impl_pow(float##n x, float##n y);                 \
  float##n __OVERLOADABLE__ _impl_powr(float##n x, float##n y);                \
  float##n __OVERLOADABLE__ _impl_remainder(float##n x, float##n y);           \
  float##n __OVERLOADABLE__ _impl_round(float##n);                             \
  float##n __OVERLOADABLE__ _impl_rint(float##n);                              \
  float##n __OVERLOADABLE__ _impl_rsqrt(float##n);                             \
  float##n __OVERLOADABLE__ _impl_rootn(float##n x, int##n y);                 \
  float##n __OVERLOADABLE__ _impl_sin(float##n);                               \
  float##n __OVERLOADABLE__ _impl_sinh(float##n);                              \
  float##n __OVERLOADABLE__ _impl_sinpi(float##n);                             \
  float##n __OVERLOADABLE__ _impl_sqrt(float##n);                              \
  float##n __OVERLOADABLE__ _impl_tan(float##n);                               \
  float##n __OVERLOADABLE__ _impl_tanh(float##n);                              \
  float##n __OVERLOADABLE__ _impl_tanpi(float##n);                             \
  float##n __OVERLOADABLE__ _impl_tgamma(float##n);                            \
  float##n __OVERLOADABLE__ _impl_trunc(float##n);                             \
  float##n __OVERLOADABLE__ _impl_sincos(float##n x,                           \
                                         __global float##n *cosval);           \
  float##n __OVERLOADABLE__ _impl_sincos(float##n x,                           \
                                         __local float##n *cosval);            \
  float##n __OVERLOADABLE__ _impl_sincos(float##n x,                           \
                                         __private float##n *cosval);          \
  float##n __OVERLOADABLE__ _impl_half_cos(float##n x);                        \
  float##n __OVERLOADABLE__ _impl_half_exp(float##n x);                        \
  float##n __OVERLOADABLE__ _impl_half_exp2(float##n x);                       \
  float##n __OVERLOADABLE__ _impl_half_exp10(float##n x);                      \
  float##n __OVERLOADABLE__ _impl_half_log(float##n x);                        \
  float##n __OVERLOADABLE__ _impl_half_log2(float##n x);                       \
  float##n __OVERLOADABLE__ _impl_half_log10(float##n x);                      \
  float##n __OVERLOADABLE__ _impl_half_powr(float##n x, float##n y);           \
  float##n __OVERLOADABLE__ _impl_half_divide(float##n x, float##n y);         \
  float##n __OVERLOADABLE__ _impl_native_divide(float##n x, float##n y);       \
  float##n __OVERLOADABLE__ _impl_half_recip(float##n x);                      \
  float##n __OVERLOADABLE__ _impl_half_rsqrt(float##n x);                      \
  float##n __OVERLOADABLE__ _impl_half_sin(float##n x);                        \
  float##n __OVERLOADABLE__ _impl_half_sqrt(float##n x);                       \
  float##n __OVERLOADABLE__ _impl_half_tan(float##n x);                        \
  float##n __OVERLOADABLE__ _impl_half_cos(float##n x);                        \
  float##n __OVERLOADABLE__ _impl_native_cos(float##n x);                      \
  float##n __OVERLOADABLE__ _impl_native_exp(float##n x);                      \
  float##n __OVERLOADABLE__ _impl_native_exp2(float##n x);                     \
  float##n __OVERLOADABLE__ _impl_native_exp10(float##n x);                    \
  float##n __OVERLOADABLE__ _impl_native_log(float##n x);                      \
  float##n __OVERLOADABLE__ _impl_native_log2(float##n x);                     \
  float##n __OVERLOADABLE__ _impl_native_log10(float##n x);                    \
  float##n __OVERLOADABLE__ _impl_native_recip(float##n x);                    \
  float##n __OVERLOADABLE__ _impl_native_rsqrt(float##n x);                    \
  float##n __OVERLOADABLE__ _impl_native_sin(float##n x);                      \
  float##n __OVERLOADABLE__ _impl_native_sqrt(float##n x);                     \
  float##n __OVERLOADABLE__ _impl_native_tan(float##n x);                      \
  float##n __OVERLOADABLE__ _impl_lgamma_r(float##n x,                         \
                                           __global int##n *signp);            \
  float##n __OVERLOADABLE__ _impl_lgamma_r(float##n x, __local int##n *signp); \
  float##n __OVERLOADABLE__ _impl_lgamma_r(float##n x,                         \
                                           __private int##n *signp);           \
  float##n __OVERLOADABLE__ _impl_ldexp(float##n x, int##n k);                 \
  float##n __OVERLOADABLE__ _impl_ldexp(float##n x, int k);                    \
  int##n __OVERLOADABLE__ _impl_ilogb(float##n x);                             \
  float##n __OVERLOADABLE__ _impl_frexp(float##n x, __private int##n *exp);    \
  float##n __OVERLOADABLE__ _impl_frexp(float##n x, __global int##n *exp);     \
  float##n __OVERLOADABLE__ _impl_frexp(float##n x, __local int##n *exp);      \
  float##n __OVERLOADABLE__ _impl_nan(uint##n nancode);                        \
  float##n __OVERLOADABLE__ _impl_pown(float##n x, int##n y);                  \
  float##n __OVERLOADABLE__ _impl_remquo(float##n x, float##n y,               \
                                         __global int##n *quo);                \
  float##n __OVERLOADABLE__ _impl_remquo(float##n x, float##n y,               \
                                         __local int##n *quo);                 \
  float##n __OVERLOADABLE__ _impl_remquo(float##n x, float##n y,               \
                                         __private int##n *quo);

MATH_MACRO_TYPE(2)
MATH_MACRO_TYPE(3)
MATH_MACRO_TYPE(4)
MATH_MACRO_TYPE(8)
MATH_MACRO_TYPE(16)
#undef MATH_MACRO_TYPE

#define MATH_MACRO_TYPE(n)                                                     \
  double##n __OVERLOADABLE__ _impl_acos(double##n x);                          \
  double##n __OVERLOADABLE__ _impl_acosh(double##n x);                         \
  double##n __OVERLOADABLE__ _impl_acospi(double##n x);                        \
  double##n __OVERLOADABLE__ _impl_asin(double##n x);                          \
  double##n __OVERLOADABLE__ _impl_asinh(double##n x);                         \
  double##n __OVERLOADABLE__ _impl_asinpi(double##n x);                        \
  double##n __OVERLOADABLE__ _impl_atan(double##n x);                          \
  double##n __OVERLOADABLE__ _impl_atan2(double##n y, double##n x);            \
  double##n __OVERLOADABLE__ _impl_atanh(double##n x);                         \
  double##n __OVERLOADABLE__ _impl_atanpi(double##n x);                        \
  double##n __OVERLOADABLE__ _impl_atan2pi(double##n y, double##n x);          \
  double##n __OVERLOADABLE__ _impl_cbrt(double##n);                            \
  double##n __OVERLOADABLE__ _impl_ceil(double##n);                            \
  double##n __OVERLOADABLE__ _impl_copysign(double##n x, double##n y);         \
  double##n __OVERLOADABLE__ _impl_cos(double##n);                             \
  double##n __OVERLOADABLE__ _impl_cosh(double##n);                            \
  double##n __OVERLOADABLE__ _impl_cospi(double##n);                           \
  double##n __OVERLOADABLE__ _impl_erfc(double##n);                            \
  double##n __OVERLOADABLE__ _impl_erf(double##n);                             \
  double##n __OVERLOADABLE__ _impl_exp(double##n x);                           \
  double##n __OVERLOADABLE__ _impl_exp2(double##n);                            \
  double##n __OVERLOADABLE__ _impl_exp10(double##n);                           \
  double##n __OVERLOADABLE__ _impl_expm1(double##n x);                         \
  double##n __OVERLOADABLE__ _impl_fabs(double##n);                            \
  double##n __OVERLOADABLE__ _impl_fdim(double##n x, double##n y);             \
  double##n __OVERLOADABLE__ _impl_floor(double##n);                           \
  double##n __OVERLOADABLE__ _impl_fma(double##n a, double##n b, double##n c); \
  double##n __OVERLOADABLE__ _impl_fmax(double##n x, double##n y);             \
  double##n __OVERLOADABLE__ _impl_fmin(double##n x, double##n y);             \
  double##n __OVERLOADABLE__ _impl_fmod(double##n x, double##n y);             \
  double##n __OVERLOADABLE__ _impl_fract(double##n x,                          \
                                         __global double##n *iptr);            \
  double##n __OVERLOADABLE__ _impl_fract(double##n x,                          \
                                         __local double##n *iptr);             \
  double##n __OVERLOADABLE__ _impl_fract(double##n x,                          \
                                         __private double##n *iptr);           \
  double##n __OVERLOADABLE__ _impl_hypot(double##n x, double##n y);            \
  double##n __OVERLOADABLE__ _impl_lgamma(double##n x);                        \
  double##n __OVERLOADABLE__ _impl_log(double##n);                             \
  double##n __OVERLOADABLE__ _impl_log2(double##n);                            \
  double##n __OVERLOADABLE__ _impl_log10(double##n);                           \
  double##n __OVERLOADABLE__ _impl_log1p(double##n);                           \
  double##n __OVERLOADABLE__ _impl_logb(double##n);                            \
  double##n __OVERLOADABLE__ _impl_mad(double##n a, double##n b, double##n c); \
  double##n __OVERLOADABLE__ _impl_maxmag(double##n x, double##n y);           \
  double##n __OVERLOADABLE__ _impl_minmag(double##n x, double##n y);           \
  double##n __OVERLOADABLE__ _impl_modf(double##n x,                           \
                                        __global double##n *iptr);             \
  double##n __OVERLOADABLE__ _impl_modf(double##n x, __local double##n *iptr); \
  double##n __OVERLOADABLE__ _impl_modf(double##n x,                           \
                                        __private double##n *iptr);            \
  double##n __OVERLOADABLE__ _impl_nextafter(double##n x, double##n y);        \
  double##n __OVERLOADABLE__ _impl_pow(double##n x, double##n y);              \
  double##n __OVERLOADABLE__ _impl_powr(double##n x, double##n y);             \
  double##n __OVERLOADABLE__ _impl_remainder(double##n x, double##n y);        \
  double##n __OVERLOADABLE__ _impl_round(double##n);                           \
  double##n __OVERLOADABLE__ _impl_rint(double##n);                            \
  double##n __OVERLOADABLE__ _impl_rsqrt(double##n);                           \
  double##n __OVERLOADABLE__ _impl_rootn(double##n x, int##n y);               \
  double##n __OVERLOADABLE__ _impl_sin(double##n);                             \
  double##n __OVERLOADABLE__ _impl_sinh(double##n);                            \
  double##n __OVERLOADABLE__ _impl_sinpi(double##n);                           \
  double##n __OVERLOADABLE__ _impl_sqrt(double##n);                            \
  double##n __OVERLOADABLE__ _impl_tan(double##n);                             \
  double##n __OVERLOADABLE__ _impl_tanh(double##n);                            \
  double##n __OVERLOADABLE__ _impl_tanpi(double##n);                           \
  double##n __OVERLOADABLE__ _impl_tgamma(double##n);                          \
  double##n __OVERLOADABLE__ _impl_trunc(double##n);                           \
  double##n __OVERLOADABLE__ _impl_sincos(double##n x,                         \
                                          __global double##n *cosval);         \
  double##n __OVERLOADABLE__ _impl_sincos(double##n x,                         \
                                          __local double##n *cosval);          \
  double##n __OVERLOADABLE__ _impl_sincos(double##n x,                         \
                                          __private double##n *cosval);        \
  double##n __OVERLOADABLE__ _impl_lgamma_r(double##n x,                       \
                                           __global int##n *signp);            \
  double##n __OVERLOADABLE__ _impl_lgamma_r(double##n x,                       \
                                            __local int##n *signp); \
  double##n __OVERLOADABLE__ _impl_lgamma_r(double##n x,                       \
                                           __private int##n *signp);           \
  double##n __OVERLOADABLE__ _impl_ldexp(double##n x, int##n k);               \
  double##n __OVERLOADABLE__ _impl_ldexp(double##n x, int k);                  \
  int##n __OVERLOADABLE__ _impl_ilogb(double##n x);                            \
  double##n __OVERLOADABLE__ _impl_frexp(double##n x, __private int##n *exp);  \
  double##n __OVERLOADABLE__ _impl_frexp(double##n x, __global int##n *exp);   \
  double##n __OVERLOADABLE__ _impl_frexp(double##n x, __local int##n *exp);    \
  double##n __OVERLOADABLE__ _impl_nan(ulong##n nancode);                      \
  double##n __OVERLOADABLE__ _impl_pown(double##n x, int##n y);                \
  double##n __OVERLOADABLE__ _impl_remquo(double##n x, double##n y,            \
                                         __global int##n *quo);                \
  double##n __OVERLOADABLE__ _impl_remquo(double##n x, double##n y,            \
                                         __local int##n *quo);                 \
  double##n __OVERLOADABLE__ _impl_remquo(double##n x, double##n y,            \
                                         __private int##n *quo);

MATH_MACRO_TYPE(2)
MATH_MACRO_TYPE(3)
MATH_MACRO_TYPE(4)
MATH_MACRO_TYPE(8)
MATH_MACRO_TYPE(16)
#undef MATH_MACRO_TYPE

#undef __OVERLOADABLE__

#endif // __MATH_FUN_H__
