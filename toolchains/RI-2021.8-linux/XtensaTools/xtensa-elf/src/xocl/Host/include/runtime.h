/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED. 
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of 
 * Cadence Design Systems Inc. Any rights to use, modify, and create 
 * derivative works of this file are set forth under the terms of your 
 * license agreement with Cadence Design Systems, Inc.
 */

#ifndef __RUNTIME_H__
#define __RUNTIME_H__

#include <stdint.h>
#ifndef __private
#define __private
#endif
#ifndef private
#define private
#endif
#ifndef __global
#define __global
#endif
#ifndef global
#define global
#endif
#ifndef __local
#define __local
#endif
#ifndef local
#define local
#endif
#ifndef __constant
#define __constant
#endif
#ifndef constant
#define constant
#endif
#ifndef __kernel
#define __kernel
#endif
#ifndef kernel
#define kernel
#endif
#ifndef __read_only
#define __read_only
#endif
#ifndef read_only
#define read_only
#endif
#ifndef __write_only
#define __write_only
#endif
#ifndef write_only
#define write_only
#endif
#ifndef __read_write
#define __read_write
#endif
#ifndef read_write
#define read_write
#endif

typedef __typeof__(sizeof(int)) size_t;

#ifndef UINT32_MAX
#define UINT32_MAX (4294967295U)
#endif

#ifndef CHAR_BIT
#define CHAR_BIT   8
#endif
#ifndef INT_MAX
#define INT_MAX   2147483647
#endif
#ifndef INT_MIN
#define INT_MIN   (-2147483647 - 1)
#endif

#ifndef LONG_MAX
#define LONG_MAX   0x7fffffffffffffffL
#endif
#ifndef LONG_MIN
#define LONG_MIN   (-0x7fffffffffffffffL - 1)
#endif

#ifndef SCHAR_MAX
#define SCHAR_MAX   127
#endif
#ifndef SCHAR_MIN
#define SCHAR_MIN   (-127 - 1)
#endif
#ifndef CHAR_MAX
#define CHAR_MAX   SCHAR_MAX
#endif
#ifndef CHAR_MIN
#define CHAR_MIN   SCHAR_MIN
#endif
#ifndef SHRT_MAX
#define SHRT_MAX   32767
#endif
#ifndef SHRT_MIN
#define SHRT_MIN   (-32767 - 1)
#endif
#ifndef UCHAR_MAX
#define UCHAR_MAX   255
#endif
#ifndef USHRT_MAX
#define USHRT_MAX   65535
#endif
#ifndef UINT_MAX
#define UINT_MAX   0xffffffff
#endif
#ifndef ULONG_MAX
#define ULONG_MAX   0xffffffffffffffffUL
#endif
#ifndef half
#define half unsigned short
#endif
#ifndef FLT_DIG
#define FLT_DIG     6
#endif
#ifndef FLT_MANT_DIG
#define FLT_MANT_DIG  24
#endif
#ifndef FLT_MAX_10_EXP
#define FLT_MAX_10_EXP   +38
#endif
#ifndef FLT_MAX_EXP
#define FLT_MAX_EXP   +128
#endif
#ifndef FLT_MIN_10_EXP
#define FLT_MIN_10_EXP   -37
#endif
#ifndef FLT_MIN_EXP
#define FLT_MIN_EXP   -125
#endif
#ifndef FLT_RADIX
#define FLT_RADIX     2
#endif
#ifndef FLT_MAX
#define FLT_MAX    0x1.fffffep127f
#endif
#ifndef FLT_MIN
#define FLT_MIN    0x1.0p-126f
#endif
#ifndef FLT_EPSILON
#define FLT_EPSILON   0x1.0p-23f
#endif
#ifndef M_E_F
#define M_E_F       2.7182818284590452353602874713526625L   /* e */
#endif
#ifndef M_LOG2E_F
#define M_LOG2E_F   1.4426950408889634073599246810018921L   /* log_2 e */
#endif
#ifndef M_LOG10E_F
#define M_LOG10E_F  0.4342944819032518276511289189166051L   /* log_10 e */
#endif
#ifndef M_LN2_F
#define M_LN2_F     0.6931471805599453094172321214581766L   /* log_e 2 */
#endif
#ifndef M_LN10_F
#define M_LN10_F    2.3025850929940456840179914546843642L   /* log_e 10 */
#endif
#ifndef M_PI_F
#define M_PI_F      3.1415926535897932384626433832795029L   /* pi */
#endif
#ifndef M_PI_2_F
#define M_PI_2_F    1.5707963267948966192313216916397514L   /* pi/2 */
#endif
#ifndef M_PI_4_F
#define M_PI_4_F    0.7853981633974483096156608458198757L   /* pi/4 */
#endif
#ifndef M_1_PI_F
#define M_1_PI_F    0.3183098861837906715377675267450287L   /* 1/pi */
#endif
#ifndef M_2_PI_F
#define M_2_PI_F    0.6366197723675813430755350534900574L   /* 2/pi */
#endif
#ifndef M_2_SQRTPI_F
#define M_2_SQRTPI_F    1.1283791670955125738961589031215452L   /* 2/sqrt(pi) */
#endif
#ifndef M_SQRT2_F
#define M_SQRT2_F   1.4142135623730950488016887242096981L   /* sqrt(2) */
#endif
#ifndef M_SQRT1_2_F
#define M_SQRT1_2_F 0.7071067811865475244008443621048490L   /* 1/sqrt(2) */
#endif
#ifndef MAXFLOAT
#define MAXFLOAT   0x1.fffffep127f
#endif

#ifndef FP_ILOGB0
#define FP_ILOGB0   (-2147483647 - 1)
#endif
#ifndef FP_ILOGBNAN
#define FP_ILOGBNAN    (-2147483647 - 1)
#endif

#ifndef __OPENCL_VERSION__
#define __OPENCL_VERSION__ 120
#endif

#ifndef __OPENCL_C_VERSION__
#define __OPENCL_C_VERSION__  120
#endif

#ifndef __kernel_exec
#define __kernel_exec
#endif

#ifndef __ROUNDING_MODE__
#define __ROUNDING_MODE__ rte
#endif

#ifndef M_PI
#define M_PI		3.14159265358979323846264338327950288
#endif

#include "../include/xocl_kernel_info.h"
#define __CONSTRUCTOR__ __attribute__((constructor))
typedef int bool;
enum {false, true};

extern void _xt_ocl_reg_krldes_func(int, const xocl_opencl_kernel_descriptor*);

#endif // __RUNTIME_H__

