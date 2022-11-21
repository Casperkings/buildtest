/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of
 * Cadence Design Systems Inc. Any rights to use, modify, and create
 * derivative works of this file are set forth under the terms of your
 * license agreement with Cadence Design Systems, Inc.
 */

#ifndef __XOCL_BUILDIN_MATH_H__
#define __XOCL_BUILDIN_MATH_H__

#define cospi(x)        _impl_cospi(x)
#define sinpi(x)        _impl_sinpi(x)
#define tanpi(x)        _impl_tanpi(x)
#define acosh(x)        _impl_acosh(x)
#define asinh(x)        _impl_asinh(x)
#define atanh(x)        _impl_atanh(x)
#define sincos(x, y)    _impl_sincos(x, y)
#define exp10(x)        _impl_exp10(x)
#define acospi(x)       _impl_acospi(x)
#define asinpi(x)       _impl_asinpi(x)
#define atanpi(x)       _impl_atanpi(x)
#define expm1(x)        _impl_expm1(x)
#define fdim(x, y)      _impl_fdim(x, y)
#define maxmag(x, y)    _impl_maxmag(x, y)
#define minmag(x, y)    _impl_minmag(x, y)
#define cos(x)          _impl_cos(x)
#define sin(x)          _impl_sin(x)
#define tan(x)          _impl_tan(x)
#define acos(x)         _impl_acos(x)
#define cosh(x)         _impl_cosh(x)
#define sinh(x)         _impl_sinh(x)
#define tanh(x)         _impl_tanh(x)
#define asin(x)         _impl_asin(x)
#define atan(x)         _impl_atan(x)
#define exp(x)          _impl_exp(x)
#define exp2(x)         _impl_exp2(x)
#define expm(x)         _impl_expm(x)
#define log(x)          _impl_log(x)
#define logb(x)         _impl_logb(x)
#define log1p(x)        _impl_log1p(x)
#define ldexp(x, y)     _impl_ldexp(x, y)
#define log2(x)         _impl_log2(x)
#define cbrt(x)         _impl_cbrt(x)
#define log10(x)        _impl_log10(x)
#define sqrt(x)         _impl_sqrt(x)
#define rint(x)         _impl_rint(x)
#define floor(x)        _impl_floor(x)
#define fabs(x)         _impl_fabs(x)
#define trunc(x)        _impl_trunc(x)
#define ceil(x)         _impl_ceil(x)
#define fmin(x, y)      _impl_fmin(x, y)
#define fmod(x, y)      _impl_fmod(x, y)
#define hypot(x, y)     _impl_hypot(x, y)
#define fmax(x, y)      _impl_fmax(x, y)
#define pow(x, y)       _impl_pow((x), (y))
#define powr(x, y)      _impl_powr((x), (y))
#define pown(x, y)      _impl_pown((x), (y))
#define half_powr(x, y) _impl_half_powr((x), (y))
#define rootn(x, y)     _impl_rootn((x), (y))
#define ilogb(x)        _impl_ilogb(x)
#define frexp(x, y)     _impl_frexp(x, y)
#define modf(x, y)      _impl_modf(x, y)
#define remainder(x, y) _impl_remainder(x, y)
#define fract(x, y)     _impl_fract(x, y)
#define nextafter(x, y) _impl_nextafter(x, y)
#define atan2(x, y)     _impl_atan2(x, y)
#define atan2pi(x, y)   _impl_atan2pi(x, y)
#define copysign(x, y)  _impl_copysign(x, y)
#define half_sqrt(x)    _impl_half_sqrt(x)
#define half_exp(x)     _impl_half_exp(x)
#define half_exp2(x)    _impl_half_exp2(x)
#define half_exp10(x)   _impl_half_exp10(x)
#define half_log(x)     _impl_half_log(x)
#define half_log2(x)    _impl_half_log2(x)
#define half_log10(x)   _impl_half_log10(x)
#define half_sin(x)     _impl_half_sin(x)
#define half_cos(x)     _impl_half_cos(x)

#define fma(a, b, c)      _impl_fma(a, b, c)
#define mad(a, b, c)      _impl_mad(a, b, c)
#define remquo(a, b, c)   _impl_remquo(a, b, c)
#define nan(x)            _impl_nan(x)
#define lgamma_r(x, y)    _impl_lgamma_r(x, y)
#define lgamma(x)         _impl_lgamma(x)
#define tgamma(x)         _impl_tgamma(x)
#define isunordered(x, y) _impl_isunordered(x, y)

#define fabsf(x) _impl_fabsf(x)

#define round(x)    _impl_round(x)
#define signbit(x)  _impl_signbit(x)
#define isinf(x)    _impl_isinf(x)
#define isnan(x)    _impl_isnan(x)
#define isnormal(x) _impl_isnormal(x)
#define isfinite(x) _impl_isfinite(x)

#define rsqrt(x)            _impl_rsqrt(x)
#define half_divide(x, y)   _impl_half_divide(x, y)
#define half_recip(x)       _impl_half_recip(x)
#define half_rsqrt(x)       _impl_half_rsqrt(x)
#define half_tan(x)         _impl_half_tan(x)
#define native_cos(x)       _impl_native_cos(x)
#define native_exp(x)       _impl_native_exp(x)
#define native_exp2(x)      _impl_native_exp2(x)
#define native_exp10(x)     _impl_native_exp10(x)
#define native_log(x)       _impl_native_log(x)
#define native_log2(x)      _impl_native_log2(x)
#define native_log10(x)     _impl_native_log10(x)
#define native_recip(x)     _impl_native_recip(x)
#define native_rsqrt(x)     _impl_native_rsqrt(x)
#define native_sin(x)       _impl_native_sin(x)
#define native_sqrt(x)      _impl_native_sqrt(x)
#define native_tan(x)       _impl_native_tan(x)
#define native_divide(x, y) _impl_native_divide(x, y)

#ifndef NAN
#define NAN MCW_NAN()
#endif

#ifndef INFINITY
#define INFINITY MCW_INFINITY()
#endif

#ifndef HUGE_VALF
#define HUGE_VALF MCW_HUGE_VALF()
#endif

#endif // __XOCL_BUILDIN_MATH_H__
