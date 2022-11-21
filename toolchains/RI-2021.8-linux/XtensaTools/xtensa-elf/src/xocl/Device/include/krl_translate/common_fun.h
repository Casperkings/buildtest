/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of
 * Cadence Design Systems Inc. Any rights to use, modify, and create
 * derivative works of this file are set forth under the terms of your
 * license agreement with Cadence Design Systems, Inc.
 */

#ifndef __COMMON_FUN_H__
#define __COMMON_FUN_H__

#define __OVERLOADABLE__ __attribute__((overloadable))

#define COMMON_MACRO(basetype, gentypen)                                       \
  gentypen __OVERLOADABLE__ clamp(gentypen x, gentypen minval,                 \
                                  gentypen maxval);                            \
  gentypen __OVERLOADABLE__ max(gentypen x, gentypen y);                       \
  gentypen __OVERLOADABLE__ max(gentypen x, basetype y);                       \
  gentypen __OVERLOADABLE__ min(gentypen x, gentypen y);                       \
  gentypen __OVERLOADABLE__ min(gentypen x, basetype y);                       \
  gentypen __OVERLOADABLE__ step(gentypen edge, gentypen x);                   \
  gentypen __OVERLOADABLE__ step(basetype edge, gentypen x);                   \
  gentypen __OVERLOADABLE__ smoothstep(gentypen edge0, gentypen edge1,         \
                                       gentypen x);                            \
  gentypen __OVERLOADABLE__ smoothstep(basetype edge0, basetype edge1,         \
                                       gentypen x);                            \
  gentypen __OVERLOADABLE__ degrees(gentypen radians);                         \
  gentypen __OVERLOADABLE__ sign(gentypen x);                                  \
  gentypen __OVERLOADABLE__ mix(gentypen x, gentypen y, gentypen a);           \
  gentypen __OVERLOADABLE__ mix(gentypen x, gentypen y, basetype a);           \
  gentypen __OVERLOADABLE__ radians(gentypen degrees);

#define COMMON_MACRO_DECLARE                                                   \
  COMMON_MACRO(float, float)                                                   \
  COMMON_MACRO(float, float2)                                                  \
  COMMON_MACRO(float, float3)                                                  \
  COMMON_MACRO(float, float4)                                                  \
  COMMON_MACRO(float, float8)                                                  \
  COMMON_MACRO(float, float16)                                                 \
  COMMON_MACRO(double, double)                                                 \
  COMMON_MACRO(double, double2)                                                \
  COMMON_MACRO(double, double3)                                                \
  COMMON_MACRO(double, double4)                                                \
  COMMON_MACRO(double, double8)                                                \
  COMMON_MACRO(double, double16)

COMMON_MACRO_DECLARE
#undef COMMON_MACRO

#endif // __COMMON_FUN_H__
