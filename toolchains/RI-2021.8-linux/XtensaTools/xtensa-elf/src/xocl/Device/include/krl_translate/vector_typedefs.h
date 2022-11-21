/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of
 * Cadence Design Systems Inc. Any rights to use, modify, and create
 * derivative works of this file are set forth under the terms of your
 * license agreement with Cadence Design Systems, Inc.
 */

#ifndef __VECTOR_TYPEDEFS_H__
#define __VECTOR_TYPEDEFS_H__

#define OCLVEC(x) __attribute__((ext_vector_type(x)))

#include "clang_ocl_declaration_macros.h"

#define complex _Complex

#define CLANG_OCL_MACRO(basetype, n) typedef basetype basetype##n OCLVEC(n)
EXPAND_MACRO_FOR_ALL_BUILTIN_TYPES;

#if XCHAL_HAVE_VISION_HP_VFPU == 0
#define half2  ushort2
#define half3  ushort3
#define half4  ushort4
#define half8  ushort8
#define half16 ushort16
#define half32 ushort32
#define half64 ushort64
#else
FOR_ALL_VECTOR_LENGTHS(half);
#endif

#if XCHAL_HAVE_CONNX_B20
typedef xtcomplexfloat cfloat;
typedef xb_vecN_2xcf32 cfloat16;
#endif

#undef CLANG_OCL_MACRO

#define CLANG_OCL_MACRO(basetype, n) typedef basetype##n long##n
FOR_ALL_VECTOR_LENGTHS(int64_t);
#undef CLANG_OCL_MACRO

#undef OCLVEC

#endif // __VECTOR_TYPEDEFS_H__
