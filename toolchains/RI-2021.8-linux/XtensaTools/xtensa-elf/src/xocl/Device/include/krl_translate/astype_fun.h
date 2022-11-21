/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of
 * Cadence Design Systems Inc. Any rights to use, modify, and create
 * derivative works of this file are set forth under the terms of your
 * license agreement with Cadence Design Systems, Inc.
 */

#ifndef __ASTYPE_FUN_H__
#define __ASTYPE_FUN_H__

#include "math_impl.h"

#define __OVERLOADABLE__ __attribute__((overloadable))

#define as_long(x)   as_int64_t(x)
#define as_long2(x)  as_int64_t2(x)
#define as_long3(x)  as_int64_t3(x)
#define as_long4(x)  as_int64_t4(x)
#define as_long8(x)  as_int64_t8(x)
#define as_long16(x) as_int64_t16(x)

#define BITCAST_FDECL(typeA, typeB) typeA __OVERLOADABLE__ as_##typeA(typeB x);

// Any type can be cast to another type of the same vector length
#define EXPAND_FDECL_EACH_WAY(typeA, typeB)                                    \
  FDECL(typeA, typeB)                                                          \
  FDECL(typeB, typeA)

#define EXPAND_FDECL_FOR_ALL_VEC_LENGTHS_EACH_WAY(typeA, typeB)                \
  EXPAND_FDECL_EACH_WAY(typeA, typeB)                                          \
  EXPAND_FDECL_EACH_WAY(typeA##2, typeB##2)                                    \
  EXPAND_FDECL_EACH_WAY(typeA##3, typeB##3)                                    \
  EXPAND_FDECL_EACH_WAY(typeA##4, typeB##4)                                    \
  EXPAND_FDECL_EACH_WAY(typeA##8, typeB##8)                                    \
  EXPAND_FDECL_EACH_WAY(typeA##16, typeB##16)                                  \
  EXPAND_FDECL_EACH_WAY(typeA##32, typeB##32)                                  \
  EXPAND_FDECL_EACH_WAY(typeA##64, typeB##64)                                  \
  EXPAND_FDECL_EACH_WAY(typeA##128, typeB##128)

#define EXPAND_FDECL_FOR_EACH_BASIC_TYPE_PAIR_EXCLUSIVE                        \
  EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(char, uchar);                               \
  EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(char, ushort);                              \
  EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(char, short);                               \
  EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(char, uint);                                \
  EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(char, int);                                 \
  EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(char, float);                               \
  EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(char, ulong);                               \
  EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(char, int64_t);                             \
  EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(short, ushort);                             \
  EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(short, uint);                               \
  EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(short, int);                                \
  EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(short, ulong);                              \
  EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(short, int64_t);                            \
  EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(short, float);                              \
  EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(int, uint);                                 \
  EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(int, ulong);                                \
  EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(int, int64_t);                              \
  EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(float, uint);                               \
  EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(float, int);                                \
  EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(float, ulong);                              \
  EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(float, int64_t);                            \
  EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(int64_t, ulong);

#define EXPAND_FDECL_FOR_EACH_BASIC_TYPE_PAIR_INCLUSIVE                        \
  EXPAND_FDECL_FOR_EACH_BASIC_TYPE_PAIR_EXCLUSIVE                              \
  EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(char, char);                                \
  EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(short, short);                              \
  EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(int, int);                                  \
  EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(int64_t, int64_t);                          \
  EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(uchar, uchar);                              \
  EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(ushort, ushort);                            \
  EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(uint, uint);                                \
  EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(ulong, ulong);                              \
  EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(float, float);

#define EXPAND_FDECL_FOR_SIGNED_AND_UNSIGNED_PAIRS(typeA, typeB)               \
  EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(typeA, typeB)                               \
  EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(u##typeA, typeB)                            \
  EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(typeA, u##typeB)                            \
  EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(u##typeA, u##typeB)

// Bitcasts can happen between any two types of the same size
#define FDECL(typeA, typeB) BITCAST_FDECL(typeA, typeB)

// Case 1: casts between signed and unsigned type versions, and ints/floats
// keep in mind that vec3 and vec4 types have the same size
#define EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(typeA, typeB)                         \
  EXPAND_FDECL_FOR_ALL_VEC_LENGTHS_EACH_WAY(typeA, typeB)                      \
  EXPAND_FDECL_EACH_WAY(typeA##3, typeB##4)                                    \
  EXPAND_FDECL_EACH_WAY(typeA##4, typeB##3)

// 8 bits
EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(char, uchar)

// 16 bits
EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(short, ushort)

// 32 bits
EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(int, uint)
EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(int, float)
EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(uint, float)

// 64 bits
EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(int64_t, ulong)
EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(ulong, double)
EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(int64_t, double)
#undef EXPAND_FDECL_FOR_BASIC_TYPE_PAIR

// Case 2: casts between vector types that are of equal size but different
// base types (recall that vec3 types have the same size as vec4 types)

// Scale factor 2: sizeof(typeB) / sizeof(typeA) == 2
#define EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(typeA, typeB)                         \
  EXPAND_FDECL_EACH_WAY(typeA##2, typeB)                                       \
  EXPAND_FDECL_EACH_WAY(typeA##3, typeB##2)                                    \
  EXPAND_FDECL_EACH_WAY(typeA##4, typeB##2)                                    \
  EXPAND_FDECL_EACH_WAY(typeA##8, typeB##3)                                    \
  EXPAND_FDECL_EACH_WAY(typeA##8, typeB##4)                                    \
  EXPAND_FDECL_EACH_WAY(typeA##16, typeB##8)                                   \
  EXPAND_FDECL_EACH_WAY(typeA##32, typeB##16)                                  \
  EXPAND_FDECL_EACH_WAY(typeA##64, typeB##32)                                  \
  EXPAND_FDECL_EACH_WAY(typeA##128, typeB##64)

EXPAND_FDECL_FOR_SIGNED_AND_UNSIGNED_PAIRS(char, short)
EXPAND_FDECL_FOR_SIGNED_AND_UNSIGNED_PAIRS(short, int)
EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(short, float)
EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(ushort, float)
EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(int, int64_t)
EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(uint, int64_t)
EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(int, ulong)
EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(uint, ulong)
EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(float, int64_t)
EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(float, ulong)
EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(int, double)
EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(uint, double)
EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(float, double)
#undef EXPAND_FDECL_FOR_BASIC_TYPE_PAIR

// Scale factor 4: sizeof(typeB) / sizeof(typeA) == 4
#define EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(typeA, typeB)                         \
  EXPAND_FDECL_EACH_WAY(typeA##3, typeB)                                       \
  EXPAND_FDECL_EACH_WAY(typeA##4, typeB)                                       \
  EXPAND_FDECL_EACH_WAY(typeA##8, typeB##2)                                    \
  EXPAND_FDECL_EACH_WAY(typeA##16, typeB##3)                                   \
  EXPAND_FDECL_EACH_WAY(typeA##16, typeB##4)

EXPAND_FDECL_FOR_SIGNED_AND_UNSIGNED_PAIRS(char, int)
EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(char, float)
EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(uchar, float)
EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(short, int64_t)
EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(ushort, int64_t)
EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(short, ulong)
EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(ushort, ulong)
EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(short, double)
EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(ushort, double)
#undef EXPAND_FDECL_FOR_BASIC_TYPE_PAIR

// Scale factor 8: sizeof(typeB) / sizeof(typeA) == 8
#define EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(typeA, typeB)                         \
  EXPAND_FDECL_EACH_WAY(typeA##8, typeB)                                       \
  EXPAND_FDECL_EACH_WAY(typeA##16, typeB##2)

EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(char, int64_t)
EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(uchar, int64_t)
EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(char, ulong)
EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(uchar, ulong)
EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(char, double)
EXPAND_FDECL_FOR_BASIC_TYPE_PAIR(uchar, double)

#undef EXPAND_FDECL_FOR_BASIC_TYPE_PAIR
#undef EXPAND_FDECL_FOR_ALL_VEC_LENGTHS_EACH_WAY
#undef BITCAST_FDECL
#undef FDECL

#endif // __ASTYPE_FUN_H__
