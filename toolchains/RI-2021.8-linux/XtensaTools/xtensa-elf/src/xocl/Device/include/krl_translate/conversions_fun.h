/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of
 * Cadence Design Systems Inc. Any rights to use, modify, and create
 * derivative works of this file are set forth under the terms of your
 * license agreement with Cadence Design Systems, Inc.
 */

#ifndef __CONVERSIONS_FUN_H__
#define __CONVERSIONS_FUN_H__

#define __OVERLOADABLE__ __attribute__((overloadable))

#define EXPAND_FDECL_FOR_ALL_VECTOR_LENGTHS(fname, srcType)                    \
  FDECL(fname, srcType, 2)                                                     \
  FDECL(fname, srcType, 3)                                                     \
  FDECL(fname, srcType, 4)                                                     \
  FDECL(fname, srcType, 8)                                                     \
  FDECL(fname, srcType, 16)                                                    \
  FDECL(fname, srcType, 32)                                                    \
  FDECL(fname, srcType, 64)                                                    \
  FDECL(fname, srcType, 128)

#define EXPAND_FDECL_FOR_ALL_VECTOR_REVERSE(fname, srcType)                    \
  FDECL(fname, srcType, 2)                                                     \
  FDECL(fname, srcType, 3)                                                     \
  FDECL(fname, srcType, 4)                                                     \
  FDECL(fname, srcType, 8)                                                     \
  FDECL(fname, srcType, 16)                                                    \
  FDECL(fname, srcType, 32)                                                    \
  FDECL(fname, srcType, 64)                                                    \
  FDECL(fname, srcType, 128)                                                   \
  FDECL(srcType, fname, 2)                                                     \
  FDECL(srcType, fname, 3)                                                     \
  FDECL(srcType, fname, 4)                                                     \
  FDECL(srcType, fname, 8)                                                     \
  FDECL(srcType, fname, 16)                                                    \
  FDECL(srcType, fname, 32)                                                    \
  FDECL(srcType, fname, 64)                                                    \
  FDECL(srcType, fname, 128)

#define VALCAST_FDECL(typeA, typeB)                                            \
  typeA __OVERLOADABLE__ convert_##typeA(typeB in);

#define convert_long(x)   convert_int64_t(x)
#define convert_long2(x)  convert_int64_t2(x)
#define convert_long3(x)  convert_int64_t3(x)
#define convert_long4(x)  convert_int64_t4(x)
#define convert_long8(x)  convert_int64_t8(x)
#define convert_long16(x) convert_int64_t16(x)

#define convert_long_sat(x)   convert_int64_t_sat(x)
#define convert_long2_sat(x)  convert_int64_t2_sat(x)
#define convert_long3_sat(x)  convert_int64_t3_sat(x)
#define convert_long4_sat(x)  convert_int64_t4_sat(x)
#define convert_long8_sat(x)  convert_int64_t8_sat(x)
#define convert_long16_sat(x) convert_int64_t16_sat(x)

#define convert_long_rte(x)   convert_int64_t_rte(x)
#define convert_long2_rte(x)  convert_int64_t2_rte(x)
#define convert_long3_rte(x)  convert_int64_t3_rte(x)
#define convert_long4_rte(x)  convert_int64_t4_rte(x)
#define convert_long8_rte(x)  convert_int64_t8_rte(x)
#define convert_long16_rte(x) convert_int64_t16_rte(x)

#define convert_long_rtz(x)   convert_int64_t_rtz(x)
#define convert_long2_rtz(x)  convert_int64_t2_rtz(x)
#define convert_long3_rtz(x)  convert_int64_t3_rtz(x)
#define convert_long4_rtz(x)  convert_int64_t4_rtz(x)
#define convert_long8_rtz(x)  convert_int64_t8_rtz(x)
#define convert_long16_rtz(x) convert_int64_t16_rtz(x)

#define convert_long_rtp(x)   convert_int64_t_rtp(x)
#define convert_long2_rtp(x)  convert_int64_t2_rtp(x)
#define convert_long3_rtp(x)  convert_int64_t3_rtp(x)
#define convert_long4_rtp(x)  convert_int64_t4_rtp(x)
#define convert_long8_rtp(x)  convert_int64_t8_rtp(x)
#define convert_long16_rtp(x) convert_int64_t16_rtp(x)

#define convert_long_rtn(x)   convert_int64_t_rtn(x)
#define convert_long2_rtn(x)  convert_int64_t2_rtn(x)
#define convert_long3_rtn(x)  convert_int64_t3_rtn(x)
#define convert_long4_rtn(x)  convert_int64_t4_rtn(x)
#define convert_long8_rtn(x)  convert_int64_t8_rtn(x)
#define convert_long16_rtn(x) convert_int64_t16_rtn(x)

#define convert_long_sat_rte(x)   convert_int64_t_sat_rte(x)
#define convert_long2_sat_rte(x)  convert_int64_t2_sat_rte(x)
#define convert_long3_sat_rte(x)  convert_int64_t3_sat_rte(x)
#define convert_long4_sat_rte(x)  convert_int64_t4_sat_rte(x)
#define convert_long8_sat_rte(x)  convert_int64_t8_sat_rte(x)
#define convert_long16_sat_rte(x) convert_int64_t16_sat_rte(x)

#define convert_long_sat_rtz(x)   convert_int64_t_sat_rtz(x)
#define convert_long2_sat_rtz(x)  convert_int64_t2_sat_rtz(x)
#define convert_long3_sat_rtz(x)  convert_int64_t3_sat_rtz(x)
#define convert_long4_sat_rtz(x)  convert_int64_t4_sat_rtz(x)
#define convert_long8_sat_rtz(x)  convert_int64_t8_sat_rtz(x)
#define convert_long16_sat_rtz(x) convert_int64_t16_sat_rtz(x)

#define convert_long_sat_rtp(x)   convert_int64_t_sat_rtp(x)
#define convert_long2_sat_rtp(x)  convert_int64_t2_sat_rtp(x)
#define convert_long3_sat_rtp(x)  convert_int64_t3_sat_rtp(x)
#define convert_long4_sat_rtp(x)  convert_int64_t4_sat_rtp(x)
#define convert_long8_sat_rtp(x)  convert_int64_t8_sat_rtp(x)
#define convert_long16_sat_rtp(x) convert_int64_t16_sat_rtp(x)

#define convert_long_sat_rtn(x)   convert_int64_t_sat_rtn(x)
#define convert_long2_sat_rtn(x)  convert_int64_t2_sat_rtn(x)
#define convert_long3_sat_rtn(x)  convert_int64_t3_sat_rtn(x)
#define convert_long4_sat_rtn(x)  convert_int64_t4_sat_rtn(x)
#define convert_long8_sat_rtn(x)  convert_int64_t8_sat_rtn(x)
#define convert_long16_sat_rtn(x) convert_int64_t16_sat_rtn(x)

#define MACRO_CONVERT_TYPEA_TYPEB_SCALAR_ONE(typeA, typeB)                     \
  typeA __OVERLOADABLE__ convert_##typeA(typeB in);                            \
  typeA __OVERLOADABLE__ convert_##typeA##_sat(typeB in);

#define MACRO_CONVERT_TYPEA_TYPEB_SCALAR_REVERSE(typeA, typeB)                 \
  typeA __OVERLOADABLE__ convert_##typeA(typeB in);                            \
  typeB __OVERLOADABLE__ convert_##typeB(typeA in);                            \
  typeA __OVERLOADABLE__ convert_##typeA##_sat(typeB in);                      \
  typeB __OVERLOADABLE__ convert_##typeB##_sat(typeA in);

MACRO_CONVERT_TYPEA_TYPEB_SCALAR_ONE(uchar, uchar);
MACRO_CONVERT_TYPEA_TYPEB_SCALAR_ONE(char, char);
MACRO_CONVERT_TYPEA_TYPEB_SCALAR_ONE(ushort, ushort);
MACRO_CONVERT_TYPEA_TYPEB_SCALAR_ONE(short, short);
MACRO_CONVERT_TYPEA_TYPEB_SCALAR_ONE(uint, uint);
MACRO_CONVERT_TYPEA_TYPEB_SCALAR_ONE(int, int);
MACRO_CONVERT_TYPEA_TYPEB_SCALAR_ONE(float, float);
MACRO_CONVERT_TYPEA_TYPEB_SCALAR_ONE(double, double);
MACRO_CONVERT_TYPEA_TYPEB_SCALAR_ONE(ulong, ulong);
MACRO_CONVERT_TYPEA_TYPEB_SCALAR_ONE(int64_t, int64_t);

MACRO_CONVERT_TYPEA_TYPEB_SCALAR_REVERSE(char, uchar);
MACRO_CONVERT_TYPEA_TYPEB_SCALAR_REVERSE(char, ushort);
MACRO_CONVERT_TYPEA_TYPEB_SCALAR_REVERSE(char, short);
MACRO_CONVERT_TYPEA_TYPEB_SCALAR_REVERSE(char, uint);
MACRO_CONVERT_TYPEA_TYPEB_SCALAR_REVERSE(char, int);
MACRO_CONVERT_TYPEA_TYPEB_SCALAR_REVERSE(char, float);
MACRO_CONVERT_TYPEA_TYPEB_SCALAR_REVERSE(char, double);
MACRO_CONVERT_TYPEA_TYPEB_SCALAR_REVERSE(char, ulong);
MACRO_CONVERT_TYPEA_TYPEB_SCALAR_REVERSE(char, int64_t);
MACRO_CONVERT_TYPEA_TYPEB_SCALAR_REVERSE(short, uchar);
MACRO_CONVERT_TYPEA_TYPEB_SCALAR_REVERSE(short, ushort);
MACRO_CONVERT_TYPEA_TYPEB_SCALAR_REVERSE(short, uint);
MACRO_CONVERT_TYPEA_TYPEB_SCALAR_REVERSE(short, int);
MACRO_CONVERT_TYPEA_TYPEB_SCALAR_REVERSE(short, ulong);
MACRO_CONVERT_TYPEA_TYPEB_SCALAR_REVERSE(short, int64_t);
MACRO_CONVERT_TYPEA_TYPEB_SCALAR_REVERSE(short, float);
MACRO_CONVERT_TYPEA_TYPEB_SCALAR_REVERSE(short, double);
MACRO_CONVERT_TYPEA_TYPEB_SCALAR_REVERSE(int, uchar);
MACRO_CONVERT_TYPEA_TYPEB_SCALAR_REVERSE(int, ushort);
MACRO_CONVERT_TYPEA_TYPEB_SCALAR_REVERSE(int, uint);
MACRO_CONVERT_TYPEA_TYPEB_SCALAR_REVERSE(int, ulong);
MACRO_CONVERT_TYPEA_TYPEB_SCALAR_REVERSE(int, int64_t);
MACRO_CONVERT_TYPEA_TYPEB_SCALAR_REVERSE(int, float);
MACRO_CONVERT_TYPEA_TYPEB_SCALAR_REVERSE(int, double);
MACRO_CONVERT_TYPEA_TYPEB_SCALAR_REVERSE(int64_t, uchar);
MACRO_CONVERT_TYPEA_TYPEB_SCALAR_REVERSE(int64_t, ushort);
MACRO_CONVERT_TYPEA_TYPEB_SCALAR_REVERSE(int64_t, uint);
MACRO_CONVERT_TYPEA_TYPEB_SCALAR_REVERSE(int64_t, ulong);
MACRO_CONVERT_TYPEA_TYPEB_SCALAR_REVERSE(int64_t, float);
MACRO_CONVERT_TYPEA_TYPEB_SCALAR_REVERSE(int64_t, double);
MACRO_CONVERT_TYPEA_TYPEB_SCALAR_REVERSE(float, uchar);
MACRO_CONVERT_TYPEA_TYPEB_SCALAR_REVERSE(float, ushort);
MACRO_CONVERT_TYPEA_TYPEB_SCALAR_REVERSE(float, uint);
MACRO_CONVERT_TYPEA_TYPEB_SCALAR_REVERSE(float, ulong);
MACRO_CONVERT_TYPEA_TYPEB_SCALAR_REVERSE(float, double);
MACRO_CONVERT_TYPEA_TYPEB_SCALAR_REVERSE(double, uchar);
MACRO_CONVERT_TYPEA_TYPEB_SCALAR_REVERSE(double, ushort);
MACRO_CONVERT_TYPEA_TYPEB_SCALAR_REVERSE(double, uint);
MACRO_CONVERT_TYPEA_TYPEB_SCALAR_REVERSE(double, ulong);
MACRO_CONVERT_TYPEA_TYPEB_SCALAR_REVERSE(uchar, ushort);
MACRO_CONVERT_TYPEA_TYPEB_SCALAR_REVERSE(uchar, uint);
MACRO_CONVERT_TYPEA_TYPEB_SCALAR_REVERSE(uchar, ulong);
MACRO_CONVERT_TYPEA_TYPEB_SCALAR_REVERSE(ushort, uint);
MACRO_CONVERT_TYPEA_TYPEB_SCALAR_REVERSE(ushort, ulong);
MACRO_CONVERT_TYPEA_TYPEB_SCALAR_REVERSE(uint, ulong);

#define MACRO_CONVERT_SAME_TYPE_DEC(gentypeA, gentypeB)                        \
  gentypeA __OVERLOADABLE__ convert_##gentypeA##_sat(gentypeB x);

MACRO_CONVERT_SAME_TYPE_DEC(uchar, uchar);
MACRO_CONVERT_SAME_TYPE_DEC(char, char);
MACRO_CONVERT_SAME_TYPE_DEC(ushort, ushort);
MACRO_CONVERT_SAME_TYPE_DEC(short, short);
MACRO_CONVERT_SAME_TYPE_DEC(uint, uint);
MACRO_CONVERT_SAME_TYPE_DEC(int, int);
MACRO_CONVERT_SAME_TYPE_DEC(ulong, ulong);
MACRO_CONVERT_SAME_TYPE_DEC(int64_t, int64_t);
MACRO_CONVERT_SAME_TYPE_DEC(float, float);
MACRO_CONVERT_SAME_TYPE_DEC(double, double);

#define MACRO_CONVERT_TYPE(gentypeA, gentypeB)                                 \
  gentypeA __OVERLOADABLE__ convert_##gentypeA##_rte(gentypeB x);              \
  gentypeA __OVERLOADABLE__ convert_##gentypeA##_rtz(gentypeB x);              \
  gentypeA __OVERLOADABLE__ convert_##gentypeA##_rtp(gentypeB x);              \
  gentypeA __OVERLOADABLE__ convert_##gentypeA##_rtn(gentypeB x);              \
  gentypeA __OVERLOADABLE__ convert_##gentypeA##_sat_rte(gentypeB x);          \
  gentypeA __OVERLOADABLE__ convert_##gentypeA##_sat_rtz(gentypeB x);          \
  gentypeA __OVERLOADABLE__ convert_##gentypeA##_sat_rtp(gentypeB x);          \
  gentypeA __OVERLOADABLE__ convert_##gentypeA##_sat_rtn(gentypeB x);

#define MACRO_CONVERT_TYPE_REVERSE(gentypeA, gentypeB)                         \
  MACRO_CONVERT_TYPE(gentypeA, gentypeB);                                      \
  MACRO_CONVERT_TYPE(gentypeB, gentypeA);

MACRO_CONVERT_TYPE_REVERSE(char, uchar);
MACRO_CONVERT_TYPE(char, char);
MACRO_CONVERT_TYPE_REVERSE(char, ushort);
MACRO_CONVERT_TYPE_REVERSE(char, short);
MACRO_CONVERT_TYPE_REVERSE(char, uint);
MACRO_CONVERT_TYPE_REVERSE(char, int);
MACRO_CONVERT_TYPE_REVERSE(char, float);
MACRO_CONVERT_TYPE_REVERSE(char, double);
MACRO_CONVERT_TYPE_REVERSE(char, ulong);
MACRO_CONVERT_TYPE_REVERSE(char, int64_t);
MACRO_CONVERT_TYPE_REVERSE(short, uchar);
MACRO_CONVERT_TYPE_REVERSE(short, ushort);
MACRO_CONVERT_TYPE(short, short);
MACRO_CONVERT_TYPE_REVERSE(short, uint);
MACRO_CONVERT_TYPE_REVERSE(short, int);
MACRO_CONVERT_TYPE_REVERSE(short, ulong);
MACRO_CONVERT_TYPE_REVERSE(short, int64_t);
MACRO_CONVERT_TYPE_REVERSE(short, float);
MACRO_CONVERT_TYPE_REVERSE(short, double);
MACRO_CONVERT_TYPE_REVERSE(int, uchar);
MACRO_CONVERT_TYPE_REVERSE(int, ushort);
MACRO_CONVERT_TYPE_REVERSE(int, uint);
MACRO_CONVERT_TYPE(int, int);
MACRO_CONVERT_TYPE_REVERSE(int, ulong);
MACRO_CONVERT_TYPE_REVERSE(int, int64_t);
MACRO_CONVERT_TYPE_REVERSE(int, float);
MACRO_CONVERT_TYPE_REVERSE(int, double);
MACRO_CONVERT_TYPE_REVERSE(int64_t, uchar);
MACRO_CONVERT_TYPE_REVERSE(int64_t, ushort);
MACRO_CONVERT_TYPE_REVERSE(int64_t, uint);
MACRO_CONVERT_TYPE_REVERSE(int64_t, ulong);
MACRO_CONVERT_TYPE(int64_t, int64_t);
MACRO_CONVERT_TYPE_REVERSE(int64_t, float);
MACRO_CONVERT_TYPE_REVERSE(int64_t, double);
MACRO_CONVERT_TYPE_REVERSE(float, uchar);
MACRO_CONVERT_TYPE_REVERSE(float, ushort);
MACRO_CONVERT_TYPE_REVERSE(float, uint);
MACRO_CONVERT_TYPE_REVERSE(float, ulong);
MACRO_CONVERT_TYPE_REVERSE(float, double);
MACRO_CONVERT_TYPE(float, float);
MACRO_CONVERT_TYPE_REVERSE(double, uchar);
MACRO_CONVERT_TYPE_REVERSE(double, ushort);
MACRO_CONVERT_TYPE_REVERSE(double, uint);
MACRO_CONVERT_TYPE_REVERSE(double, ulong);
MACRO_CONVERT_TYPE(double, double);
MACRO_CONVERT_TYPE(uchar, uchar);
MACRO_CONVERT_TYPE_REVERSE(uchar, ushort);
MACRO_CONVERT_TYPE_REVERSE(uchar, uint);
MACRO_CONVERT_TYPE_REVERSE(uchar, ulong);
MACRO_CONVERT_TYPE(ushort, ushort);
MACRO_CONVERT_TYPE_REVERSE(ushort, uint);
MACRO_CONVERT_TYPE_REVERSE(ushort, ulong);
MACRO_CONVERT_TYPE_REVERSE(ushort, ulong);
MACRO_CONVERT_TYPE(uint, uint);
MACRO_CONVERT_TYPE_REVERSE(uint, ulong);
MACRO_CONVERT_TYPE(ulong, ulong);

#define MACRO_CONVERT_TYPEN(gentypeA, gentypeB, n)                             \
  gentypeA##n __OVERLOADABLE__ convert_##gentypeA##n(gentypeB##n x);           \
  gentypeA##n __OVERLOADABLE__ convert_##gentypeA##n##_rte(gentypeB##n x);     \
  gentypeA##n __OVERLOADABLE__ convert_##gentypeA##n##_rtz(gentypeB##n x);     \
  gentypeA##n __OVERLOADABLE__ convert_##gentypeA##n##_rtp(gentypeB##n x);     \
  gentypeA##n __OVERLOADABLE__ convert_##gentypeA##n##_rtn(gentypeB##n x);     \
  gentypeA##n __OVERLOADABLE__ convert_##gentypeA##n##_sat(gentypeB##n x);     \
  gentypeA##n __OVERLOADABLE__ convert_##gentypeA##n##_sat_rte(gentypeB##n x); \
  gentypeA##n __OVERLOADABLE__ convert_##gentypeA##n##_sat_rtz(gentypeB##n x); \
  gentypeA##n __OVERLOADABLE__ convert_##gentypeA##n##_sat_rtp(gentypeB##n x); \
  gentypeA##n __OVERLOADABLE__ convert_##gentypeA##n##_sat_rtn(gentypeB##n x);

#define FDECL(fname, srcType, n) MACRO_CONVERT_TYPEN(fname, srcType, n)

EXPAND_FDECL_FOR_ALL_VECTOR_REVERSE(char, uchar);
EXPAND_FDECL_FOR_ALL_VECTOR_LENGTHS(char, char);
EXPAND_FDECL_FOR_ALL_VECTOR_REVERSE(char, ushort);
EXPAND_FDECL_FOR_ALL_VECTOR_REVERSE(char, short);
EXPAND_FDECL_FOR_ALL_VECTOR_REVERSE(char, uint);
EXPAND_FDECL_FOR_ALL_VECTOR_REVERSE(char, int);
EXPAND_FDECL_FOR_ALL_VECTOR_REVERSE(char, float);
EXPAND_FDECL_FOR_ALL_VECTOR_REVERSE(char, double);
EXPAND_FDECL_FOR_ALL_VECTOR_REVERSE(char, ulong);
EXPAND_FDECL_FOR_ALL_VECTOR_REVERSE(char, int64_t);
EXPAND_FDECL_FOR_ALL_VECTOR_REVERSE(short, uchar);
EXPAND_FDECL_FOR_ALL_VECTOR_REVERSE(short, ushort);
EXPAND_FDECL_FOR_ALL_VECTOR_LENGTHS(short, short);
EXPAND_FDECL_FOR_ALL_VECTOR_REVERSE(short, uint);
EXPAND_FDECL_FOR_ALL_VECTOR_REVERSE(short, int);
EXPAND_FDECL_FOR_ALL_VECTOR_REVERSE(short, ulong);
EXPAND_FDECL_FOR_ALL_VECTOR_REVERSE(short, int64_t);
EXPAND_FDECL_FOR_ALL_VECTOR_REVERSE(short, float);
EXPAND_FDECL_FOR_ALL_VECTOR_REVERSE(short, double);
EXPAND_FDECL_FOR_ALL_VECTOR_REVERSE(int, uchar);
EXPAND_FDECL_FOR_ALL_VECTOR_REVERSE(int, ushort);
EXPAND_FDECL_FOR_ALL_VECTOR_REVERSE(int, uint);
EXPAND_FDECL_FOR_ALL_VECTOR_LENGTHS(int, int);
EXPAND_FDECL_FOR_ALL_VECTOR_REVERSE(int, ulong);
EXPAND_FDECL_FOR_ALL_VECTOR_REVERSE(int, int64_t);
EXPAND_FDECL_FOR_ALL_VECTOR_REVERSE(int, float);
EXPAND_FDECL_FOR_ALL_VECTOR_REVERSE(int, double);
EXPAND_FDECL_FOR_ALL_VECTOR_REVERSE(int64_t, uchar);
EXPAND_FDECL_FOR_ALL_VECTOR_REVERSE(int64_t, ushort);
EXPAND_FDECL_FOR_ALL_VECTOR_REVERSE(int64_t, uint);
EXPAND_FDECL_FOR_ALL_VECTOR_REVERSE(int64_t, ulong);
EXPAND_FDECL_FOR_ALL_VECTOR_LENGTHS(int64_t, int64_t);
EXPAND_FDECL_FOR_ALL_VECTOR_REVERSE(int64_t, float);
EXPAND_FDECL_FOR_ALL_VECTOR_REVERSE(int64_t, double);
EXPAND_FDECL_FOR_ALL_VECTOR_REVERSE(float, uchar);
EXPAND_FDECL_FOR_ALL_VECTOR_REVERSE(float, ushort);
EXPAND_FDECL_FOR_ALL_VECTOR_REVERSE(float, uint);
EXPAND_FDECL_FOR_ALL_VECTOR_REVERSE(float, ulong);
EXPAND_FDECL_FOR_ALL_VECTOR_LENGTHS(float, float);
EXPAND_FDECL_FOR_ALL_VECTOR_REVERSE(float, double);
EXPAND_FDECL_FOR_ALL_VECTOR_REVERSE(double, uchar);
EXPAND_FDECL_FOR_ALL_VECTOR_REVERSE(double, ushort);
EXPAND_FDECL_FOR_ALL_VECTOR_REVERSE(double, uint);
EXPAND_FDECL_FOR_ALL_VECTOR_REVERSE(double, ulong);
EXPAND_FDECL_FOR_ALL_VECTOR_LENGTHS(double, double);
EXPAND_FDECL_FOR_ALL_VECTOR_LENGTHS(uchar, uchar);
EXPAND_FDECL_FOR_ALL_VECTOR_REVERSE(uchar, ushort);
EXPAND_FDECL_FOR_ALL_VECTOR_REVERSE(uchar, uint);
EXPAND_FDECL_FOR_ALL_VECTOR_REVERSE(uchar, ulong);
EXPAND_FDECL_FOR_ALL_VECTOR_LENGTHS(ushort, ushort);
EXPAND_FDECL_FOR_ALL_VECTOR_REVERSE(ushort, uint);
EXPAND_FDECL_FOR_ALL_VECTOR_REVERSE(ushort, ulong);
EXPAND_FDECL_FOR_ALL_VECTOR_LENGTHS(uint, uint);
EXPAND_FDECL_FOR_ALL_VECTOR_REVERSE(uint, ulong);
EXPAND_FDECL_FOR_ALL_VECTOR_LENGTHS(ulong, ulong);

// xtensa specific conversions

ushort32 __OVERLOADABLE__ xt_convert_ushort32(uint32 x, int s);
short32 __OVERLOADABLE__ xt_convert_short32(int32 x, int s);
ushort64 __OVERLOADABLE__ xt_convert_ushort64(uint64 x, int s);
short64 __OVERLOADABLE__ xt_convert_short64(int64 x, int s);
ushort128 __OVERLOADABLE__ xt_convert_ushort128(uint128 x, int s);
short128 __OVERLOADABLE__ xt_convert_short128(int128 x, int s);

ushort32 __OVERLOADABLE__ xt_convert_ushort32_sat_rnd(uint32 x, int s);
short32 __OVERLOADABLE__ xt_convert_short32_sat_rnd(int32 x, int s);
ushort64 __OVERLOADABLE__ xt_convert_ushort64_sat_rnd(uint64 x, int s);
short64 __OVERLOADABLE__ xt_convert_short64_sat_rnd(int64 x, int s);
ushort128 __OVERLOADABLE__ xt_convert_ushort128_sat_rnd(uint128 x, int s);
short128 __OVERLOADABLE__ xt_convert_short128_sat_rnd(int128 x, int s);

uchar128 __OVERLOADABLE__ xt_convert_uchar128(int128 x, int s);
char128 __OVERLOADABLE__ xt_convert_char128(int128 x, int s);

uchar128 __OVERLOADABLE__ xt_convert_uchar128_sat_rnd(int128 x, int s);
char128 __OVERLOADABLE__ xt_convert_char128_sat_rnd(int128 x, int s);

uchar64 __OVERLOADABLE__ xt_convert24_uchar64(int64 x, int s);
char64 __OVERLOADABLE__ xt_convert24_char64(int64 x, int s);

uchar64 __OVERLOADABLE__ xt_convert24_uchar64_sat_rnd(int64 x, int s);
char64 __OVERLOADABLE__ xt_convert24_char64_sat_rnd(int64 x, int s);

ushort64 __OVERLOADABLE__ xt_convert24_ushort64(int64 x);
short64 __OVERLOADABLE__ xt_convert24_short64(int64 x);

ushort64 __OVERLOADABLE__ xt_convert24_ushort64(uint64 x, int s);
short64 __OVERLOADABLE__ xt_convert24_short64(int64 x, int s);

int16 __OVERLOADABLE__ xt_convert_int16(long16 x, int s);
int16 __OVERLOADABLE__ xt_convert_int16_sat_rnd(long16 x, int s);
int32 __OVERLOADABLE__ xt_convert_int32(long32 x, int s);
int32 __OVERLOADABLE__ xt_convert_int32_sat_rnd(long32 x, int s);
int64 __OVERLOADABLE__ xt_convert_int64(long64 x, int s);
int64 __OVERLOADABLE__ xt_convert_int64_sat_rnd(long64 x, int s);

half32 __OVERLOADABLE__ convert_half32(float32 x);
float32 __OVERLOADABLE__ convert_float32(half32 x);
int32 __OVERLOADABLE__ convert_int32(half32 x);
half32 __OVERLOADABLE__ convert_half32(int32 x);
short32 __OVERLOADABLE__ convert_short32(half32 x);
half32 __OVERLOADABLE__ convert_half32(short32 x);

half64 __OVERLOADABLE__ convert_half64(float64 x);
float64 __OVERLOADABLE__ convert_float64(half64 x);
int64 __OVERLOADABLE__ convert_int64(half64 x);
half64 __OVERLOADABLE__ convert_half64(int64 x);
short64 __OVERLOADABLE__ convert_short64(half64 x);
half64 __OVERLOADABLE__ convert_half64(short64 x);

short32 __OVERLOADABLE__ xt_convert40_char64_sat_rnd(long32 x, int s);
short32 __OVERLOADABLE__ xt_convert40_short32(long32 x);

#undef FDECL

#endif // __CONVERSIONS_FUN_H__
