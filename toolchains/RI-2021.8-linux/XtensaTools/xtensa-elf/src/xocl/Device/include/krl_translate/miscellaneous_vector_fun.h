/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of
 * Cadence Design Systems Inc. Any rights to use, modify, and create
 * derivative works of this file are set forth under the terms of your
 * license agreement with Cadence Design Systems, Inc.
 */

#ifndef __MISC_VECTOR_FUN_H__
#define __MISC_VECTOR_FUN_H__

#define __OVERLOADABLE__ __attribute__((overloadable))

#define SHUFFLE_MACRO(gentype, ugentype, n, m)                                 \
  gentype##n __OVERLOADABLE__ shuffle(gentype##m x, ugentype##n mask);         \
  gentype##n __OVERLOADABLE__ shuffle2(gentype##m x, gentype##m y,             \
                                       ugentype##n mask);

#define SHUFFLE_FUNCTIONS_SOURCE_T(ugentype, n, m)                             \
  SHUFFLE_MACRO(char, ugentype, n, m)                                          \
  SHUFFLE_MACRO(uchar, ugentype, n, m)                                         \
  SHUFFLE_MACRO(short, ugentype, n, m)                                         \
  SHUFFLE_MACRO(ushort, ugentype, n, m)                                        \
  SHUFFLE_MACRO(int, ugentype, n, m)                                           \
  SHUFFLE_MACRO(uint, ugentype, n, m)                                          \
  SHUFFLE_MACRO(long, ugentype, n, m)                                          \
  SHUFFLE_MACRO(ulong, ugentype, n, m)                                         \
  SHUFFLE_MACRO(float, ugentype, n, m)                                         \
  SHUFFLE_MACRO(double, ugentype, n, m)

#define SHUFFLE_FUNCTIONS_MASK(n, m)                                           \
  SHUFFLE_FUNCTIONS_SOURCE_T(uchar, n, m)                                      \
  SHUFFLE_FUNCTIONS_SOURCE_T(uint, n, m)                                       \
  SHUFFLE_FUNCTIONS_SOURCE_T(ushort, n, m)                                     \
  SHUFFLE_FUNCTIONS_SOURCE_T(ulong, n, m)

#define SHUFFLE_FUNCTIONS_M(n)                                                 \
  SHUFFLE_FUNCTIONS_MASK(n, 2)                                                 \
  SHUFFLE_FUNCTIONS_MASK(n, 4)                                                 \
  SHUFFLE_FUNCTIONS_MASK(n, 8)                                                 \
  SHUFFLE_FUNCTIONS_MASK(n, 16)                                                \
  SHUFFLE_FUNCTIONS_MASK(n, 32)                                                \
  SHUFFLE_FUNCTIONS_MASK(n, 64)                                                \
  SHUFFLE_FUNCTIONS_MASK(n, 128)

#define SHUFFLE_FUNCTIONS                                                      \
  SHUFFLE_FUNCTIONS_M(2)                                                       \
  SHUFFLE_FUNCTIONS_M(4)                                                       \
  SHUFFLE_FUNCTIONS_M(8)                                                       \
  SHUFFLE_FUNCTIONS_M(16)

SHUFFLE_FUNCTIONS
SHUFFLE_MACRO(uchar, uchar, 128, 128)
SHUFFLE_MACRO(char, uchar, 128, 128)
SHUFFLE_MACRO(uchar, uchar, 64, 64)
SHUFFLE_MACRO(char, uchar, 64, 64)
SHUFFLE_MACRO(ushort, ushort, 64, 64)
SHUFFLE_MACRO(short, ushort, 64, 64)
SHUFFLE_MACRO(ushort, ushort, 32, 32)
SHUFFLE_MACRO(short, ushort, 32, 32)
SHUFFLE_MACRO(uint, uint, 32, 32)
SHUFFLE_MACRO(int, uint, 32, 32)

#undef SHUFFLE_MACRO

#define GATHER_MACRO(gentype, ugentype, n)                                     \
  gentype##n __OVERLOADABLE__ xt_gather##n(const __local gentype *x,           \
                                           __global ugentype *indices);        \
  gentype##n __OVERLOADABLE__ xt_gather##n(const __local gentype *x,           \
                                           __local ugentype *indices);         \
  gentype##n __OVERLOADABLE__ xt_gather##n(const __local gentype *x,           \
                                           __constant ugentype *indices);      \
  gentype##n __OVERLOADABLE__ xt_gather##n(const __local gentype *x,           \
                                           ugentype *indices);                 \
  gentype##n __OVERLOADABLE__ xt_gather##n(const __local gentype *x,           \
                                           ugentype const *indices);

#define SCATTER_MACRO(gentype, ugentype, n)                                    \
  void __OVERLOADABLE__ xt_scatter##n(gentype##n, __local gentype *x,          \
                                      __global ugentype *indices);             \
  void __OVERLOADABLE__ xt_scatter##n(gentype##n, __local gentype *x,          \
                                      __local ugentype *indices);              \
  void __OVERLOADABLE__ xt_scatter##n(gentype##n, __local gentype *x,          \
                                      __constant ugentype *indices);           \
  void __OVERLOADABLE__ xt_scatter##n(gentype##n, __local gentype *x,          \
                                      ugentype *indices);                      \
  void __OVERLOADABLE__ xt_scatter##n(gentype##n, __local gentype *x,          \
                                      ugentype const *indices);

#define GATHER_MACRO_FUNCTIONS(n)                                              \
  GATHER_MACRO(char, ushort, n)                                                \
  GATHER_MACRO(uchar, ushort, n)                                               \
  GATHER_MACRO(short, ushort, n)                                               \
  GATHER_MACRO(ushort, ushort, n)                                              \
  GATHER_MACRO(int, uint, n)                                                   \
  GATHER_MACRO(uint, uint, n)                                                  \
  GATHER_MACRO(float, uint, n)                                                 \
  GATHER_MACRO(half, ushort, n)

#define SCATTER_MACRO_FUNCTIONS(n)                                             \
  SCATTER_MACRO(char, ushort, n)                                               \
  SCATTER_MACRO(uchar, ushort, n)                                              \
  SCATTER_MACRO(short, ushort, n)                                              \
  SCATTER_MACRO(ushort, ushort, n)                                             \
  SCATTER_MACRO(int, uint, n)                                                  \
  SCATTER_MACRO(uint, uint, n)                                                 \
  SCATTER_MACRO(float, uint, n)                                                \
  SCATTER_MACRO(half, ushort, n)

#define GATHER_FUNCTIONS                                                       \
  GATHER_MACRO_FUNCTIONS(2)                                                    \
  GATHER_MACRO_FUNCTIONS(4)                                                    \
  GATHER_MACRO_FUNCTIONS(8)                                                    \
  GATHER_MACRO_FUNCTIONS(16)                                                   \
  GATHER_MACRO_FUNCTIONS(32)                                                   \
  GATHER_MACRO_FUNCTIONS(64)

#define SCATTER_FUNCTIONS                                                      \
  SCATTER_MACRO_FUNCTIONS(2)                                                   \
  SCATTER_MACRO_FUNCTIONS(4)                                                   \
  SCATTER_MACRO_FUNCTIONS(8)                                                   \
  SCATTER_MACRO_FUNCTIONS(16)                                                  \
  SCATTER_MACRO_FUNCTIONS(32)                                                  \
  SCATTER_MACRO_FUNCTIONS(64)

GATHER_FUNCTIONS
SCATTER_FUNCTIONS

#undef GATHER_MACRO
#undef SCATTER_MACRO

void __OVERLOADABLE__ xt_deinterleave_x3(uchar64 *rgb_d,
                                         const __local uchar *rgb_i);
void __OVERLOADABLE__ xt_deinterleave_x3(uchar64 *rgb_d,
                                         const __global uchar *rgb_i);
void __OVERLOADABLE__ xt_deinterleave_x3(uchar64 *rgb_d,
                                         const __private uchar *rgb_i);
void __OVERLOADABLE__ xt_deinterleave_x3(uchar64 *rgb_d,
                                         const __constant uchar *rgb_i);
void __OVERLOADABLE__ xt_deinterleave_x3(uchar128 *rgb_d,
                                         const __local uchar *rgb_i);
void __OVERLOADABLE__ xt_deinterleave_x3(uchar128 *rgb_d,
                                         const __global uchar *rgb_i);
void __OVERLOADABLE__ xt_deinterleave_x3(uchar128 *rgb_d,
                                         const __private uchar *rgb_i);
void __OVERLOADABLE__ xt_deinterleave_x3(uchar128 *rgb_d,
                                         const __constant uchar *rgb_i);

void __OVERLOADABLE__ xt_interleave_x3(uchar64 *rgb_d, __local uchar *rgb_i);
void __OVERLOADABLE__ xt_interleave_x3(uchar64 *rgb_d, __global uchar *rgb_i);
void __OVERLOADABLE__ xt_interleave_x3(uchar64 *rgb_d, __private uchar *rgb_i);
void __OVERLOADABLE__ xt_interleave_x3(uchar128 *rgb_d, __local uchar *rgb_i);
void __OVERLOADABLE__ xt_interleave_x3(uchar128 *rgb_d, __global uchar *rgb_i);
void __OVERLOADABLE__ xt_interleave_x3(uchar128 *rgb_d, __private uchar *rgb_i);

#endif // __MISC_VECTOR_FUN_H__
