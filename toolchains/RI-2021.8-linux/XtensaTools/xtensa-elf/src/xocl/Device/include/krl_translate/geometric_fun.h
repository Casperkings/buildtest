/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of
 * Cadence Design Systems Inc. Any rights to use, modify, and create
 * derivative works of this file are set forth under the terms of your
 * license agreement with Cadence Design Systems, Inc.
 */

#ifndef __GEOMETRIC_FUN_H__
#define __GEOMETRIC_FUN_H__

#define __OVERLOADABLE__ __attribute__((overloadable))

float4 __OVERLOADABLE__ cross(float4 p0, float4 p1);
float3 __OVERLOADABLE__ cross(float3 p0, float3 p1);
double4 __OVERLOADABLE__ cross(double4 p0, double4 p1);
double3 __OVERLOADABLE__ cross(double3 p0, double3 p1);

#define GEOMETRIC_MACRO(gentypen)                                              \
  float __OVERLOADABLE__ dot(gentypen p0, gentypen p1);                        \
  float __OVERLOADABLE__ distance(gentypen p0, gentypen p1);                   \
  float __OVERLOADABLE__ length(gentypen p);                                   \
  gentypen __OVERLOADABLE__ normalize(gentypen p);                             \
  float __OVERLOADABLE__ fast_distance(gentypen p0, gentypen p1);              \
  float __OVERLOADABLE__ fast_length(gentypen p);                              \
  gentypen __OVERLOADABLE__ fast_normalize(gentypen p);

GEOMETRIC_MACRO(float)
GEOMETRIC_MACRO(float2)
GEOMETRIC_MACRO(float3)
GEOMETRIC_MACRO(float4)
#undef GEOMETRIC_MACRO

#define GEOMETRIC_MACRO(gentypen)                                              \
  double __OVERLOADABLE__ dot(gentypen p0, gentypen p1);                       \
  double __OVERLOADABLE__ distance(gentypen p0, gentypen p1);                  \
  double __OVERLOADABLE__ length(gentypen p);                                  \
  gentypen __OVERLOADABLE__ normalize(gentypen p);                             \
  double __OVERLOADABLE__ fast_distance(gentypen p0, gentypen p1);             \
  double __OVERLOADABLE__ fast_length(gentypen p);                             \
  gentypen __OVERLOADABLE__ fast_normalize(gentypen p);

GEOMETRIC_MACRO(double)
GEOMETRIC_MACRO(double2)
GEOMETRIC_MACRO(double3)
GEOMETRIC_MACRO(double4)
#undef GEOMETRIC_MACRO

#endif // __GEOMETRIC_FUN_H__
