/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of
 * Cadence Design Systems Inc. Any rights to use, modify, and create
 * derivative works of this file are set forth under the terms of your
 * license agreement with Cadence Design Systems, Inc.
 */

#ifndef __VECTOR_TYPES_H__
#define __VECTOR_TYPES_H__

#include <xtensa/config/core-isa.h>
#if XCHAL_HAVE_VISION_HP_VFPU == 0
#define half unsigned short
#endif

#if XCHAL_HAVE_CONNX_B20
#include <xtensa/tie/xt_bben.h>
#endif

/* scalar types  */
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
// This header file will be inclued as spir target.
// long type is 64 bit in spir target.
typedef long int64_t;
typedef unsigned long ulong;

#ifdef XTENSA
typedef __typeof__(sizeof(int)) size_t;
typedef int ptrdiff_t;
typedef int intptr_t;
typedef int uintptr_t;
#else
typedef unsigned long size_t;
typedef long ptrdiff_t;
typedef long intptr_t;
typedef long uintptr_t;
#endif

#ifndef image2d_t
#define image2d_t long
#endif
#ifndef image3d_t
#define image3d_t long
#endif
#ifndef sampler_t
#define sampler_t long
#endif

#ifndef UINT32_MAX
#define UINT32_MAX (4294967295U)
#endif

#ifndef CHAR_BIT
#define CHAR_BIT 8
#endif
#ifndef INT_MAX
#define INT_MAX 2147483647
#endif
#ifndef INT_MIN
#define INT_MIN (-2147483647 - 1)
#endif
#ifndef LONG_MAX
#define LONG_MAX 0x7fffffffffffffffL
#endif
#ifndef LONG_MIN
#define LONG_MIN (-0x7fffffffffffffffL - 1)
#endif
#ifndef SCHAR_MAX
#define SCHAR_MAX 127
#endif
#ifndef SCHAR_MIN
#define SCHAR_MIN (-127 - 1)
#endif
#ifndef CHAR_MAX
#define CHAR_MAX SCHAR_MAX
#endif
#ifndef CHAR_MIN
#define CHAR_MIN SCHAR_MIN
#endif
#ifndef SHRT_MAX
#define SHRT_MAX 32767
#endif
#ifndef SHRT_MIN
#define SHRT_MIN (-32767 - 1)
#endif
#ifndef UCHAR_MAX
#define UCHAR_MAX 255
#endif
#ifndef USHRT_MAX
#define USHRT_MAX 65535
#endif
#ifndef UINT_MAX
#define UINT_MAX 0xffffffff
#endif
#ifndef ULONG_MAX
#define ULONG_MAX 0xffffffffffffffffUL
#endif

#ifndef half
// #pragma OPENCL EXTENSION cl_khr_fp16 : enable
#endif
#ifndef bool
#define bool int
#endif
#ifndef true
#define true 1
#endif
#ifndef false
#define false 0
#endif

#ifndef BOOL
#define BOOL bool
#endif
#ifndef TRUE
#define TRUE true
#endif
#ifndef FALSE
#define FALSE false
#endif

#ifndef event_t
#define event_t int
#endif

#ifndef FLT_DIG
#define FLT_DIG 6
#endif
#ifndef FLT_MANT_DIG
#define FLT_MANT_DIG 24
#endif
#ifndef FLT_MAX_10_EXP
#define FLT_MAX_10_EXP +38
#endif
#ifndef FLT_MAX_EXP
#define FLT_MAX_EXP +128
#endif
#ifndef FLT_MIN_10_EXP
#define FLT_MIN_10_EXP -37
#endif
#ifndef FLT_MIN_EXP
#define FLT_MIN_EXP -125
#endif
#ifndef FLT_RADIX
#define FLT_RADIX 2
#endif
#ifndef FLT_MAX
#define FLT_MAX 0x1.fffffep127f
#endif
#ifndef FLT_MIN
#define FLT_MIN 0x1.0p-126f
#endif
#ifndef FLT_EPSILON
#define FLT_EPSILON 0x1.0p-23f
#endif
#ifndef M_E_F
#define M_E_F 2.7182818284590452353602874713526625 /* e */
#endif
#ifndef M_LOG2E_F
#define M_LOG2E_F 1.4426950408889634073599246810018921 /* log_2 e */
#endif
#ifndef M_LOG10E_F
#define M_LOG10E_F 0.4342944819032518276511289189166051 /* log_10 e */
#endif
#ifndef M_LN2_F
#define M_LN2_F 0.6931471805599453094172321214581766 /* log_e 2 */
#endif
#ifndef M_LN10_F
#define M_LN10_F 2.3025850929940456840179914546843642 /* log_e 10 */
#endif
#ifndef M_PI_F
#define M_PI_F 3.1415926535897932384626433832795029 /* pi */
#endif
#ifndef M_PI_2_F
#define M_PI_2_F 1.5707963267948966192313216916397514 /* pi/2 */
#endif
#ifndef M_PI_4_F
#define M_PI_4_F 0.7853981633974483096156608458198757 /* pi/4 */
#endif
#ifndef M_1_PI_F
#define M_1_PI_F 0.3183098861837906715377675267450287 /* 1/pi */
#endif
#ifndef M_2_PI_F
#define M_2_PI_F 0.6366197723675813430755350534900574 /* 2/pi */
#endif
#ifndef M_2_SQRTPI_F
#define M_2_SQRTPI_F 1.1283791670955125738961589031215452 /* 2/sqrt(pi) */
#endif
#ifndef M_SQRT2_F
#define M_SQRT2_F 1.4142135623730950488016887242096981 /* sqrt(2) */
#endif
#ifndef M_SQRT1_2_F
#define M_SQRT1_2_F 0.7071067811865475244008443621048490 /* 1/sqrt(2) */
#endif
#ifndef MAXFLOAT
#define MAXFLOAT 0x1.fffffep127f
#endif
#ifndef FP_ILOGB0
#define FP_ILOGB0 (-2147483647 - 1)
#endif
#ifndef FP_ILOGBNAN
#define FP_ILOGBNAN (-2147483647 - 1)
#endif

#ifndef DBL_DIG
#define DBL_DIG          15
#endif
#ifndef DBL_MANT_DIG
#define DBL_MANT_DIG     53
#endif
#ifndef DBL_MAX_10_EXP
#define DBL_MAX_10_EXP   +308
#endif
#ifndef DBL_MAX_EXP
#define DBL_MAX_EXP      +1024
#endif
#ifndef DBL_MIN_10_EXP
#define DBL_MIN_10_EXP   -307
#endif
#ifndef DBL_MIN_EXP
#define DBL_MIN_EXP      -1021
#endif
#ifndef DBL_RADIX
#define DBL_RADIX        2
#endif
#ifndef DBL_MAX
#define DBL_MAX          179769313486231570814527423731704356798070567525844996598917476803157260780028538760589558632766878171540458953514382464234321326889464182768467546703537516986049910576551282076245490090389328944075868508455133942304583236903222948165808559332123348274797826204144723168738177180919299881250404026184124858368.0
#endif
#ifndef DBL_MIN
#define DBL_MIN          2.225073858507201383090e-308
#endif
#ifndef DBL_EPSILON
#define DBL_EPSILON      2.220446049250313080847e-16
#endif

#ifndef M_E
#define M_E             2.718281828459045090796
#endif
#ifndef M_LOG2E
#define M_LOG2E         1.442695040888963387005
#endif
#ifndef M_LOG10E
#define M_LOG10E        0.434294481903251816668
#endif
#ifndef M_LN2
#define M_LN2           0.693147180559945286227
#endif
#ifndef M_LN10
#define M_LN10          2.302585092994045901094
#endif
#ifndef M_PI
#define M_PI            3.141592653589793115998
#endif
#ifndef M_PI_2
#define M_PI_2          1.570796326794896557999
#endif
#ifndef M_PI_4
#define M_PI_4          0.785398163397448278999
#endif
#ifndef M_1_PI
#define M_1_PI          0.318309886183790691216
#endif
#ifndef M_2_PI
#define M_2_PI          0.636619772367581382433
#endif
#ifndef M_2_SQRTPI
#define M_2_SQRTPI      1.128379167095512558561
#endif
#ifndef M_SQRT2
#define M_SQRT2         1.414213562373095145475
#endif
#ifndef M_SQRT1_2
#define M_SQRT1_2       0.707106781186547572737
#endif

float __attribute__((overloadable)) MCW_HUGE_VALF(void);
float __attribute__((overloadable)) MCW_NAN(void);
float __attribute__((overloadable)) MCW_INFINITY(void);

#ifndef __OPENCL_VERSION__
#define __OPENCL_VERSION__ 120
#endif

#ifndef __OPENCL_C_VERSION__
#define __OPENCL_C_VERSION__ 120
#endif

#ifndef __kernel_exec
#define __kernel_exec
#endif

#ifndef __ROUNDING_MODE__
#define __ROUNDING_MODE__ rte
#endif

#ifndef HUGE_VALF
#define HUGE_VALF MCW_HUGE_VALF()
#endif

#ifndef INFINITY
// std::numeric_limits<int>::max() is not infinity, it's just a very
// large integer, usually 2^31-1 or 2^63-1.
// Integers don't have an infinity value. Furthermore,
// std::numeric_limits<T>::max() is not infinity for any T.
// std::numeric_limits<T>::infinity().  You can do double inf=1.0/0.0;
#define INFINITY (1.0f / 0.0f)
#endif

#ifndef NAN
#define NAN (0.0f / 0.0f)
#endif

#endif // __VECTOR_TYPES_H__
