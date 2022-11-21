/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of
 * Cadence Design Systems Inc. Any rights to use, modify, and create
 * derivative works of this file are set forth under the terms of your
 * license agreement with Cadence Design Systems, Inc.
 */

#ifndef __INTEGER_FUN_H__
#define __INTEGER_FUN_H__

#define __OVERLOADABLE__ __attribute__((overloadable))

#define EXPAND_ALL_VECTOR()                                                    \
  FDECL(2)                                                                     \
  FDECL(3)                                                                     \
  FDECL(4)                                                                     \
  FDECL(8)                                                                     \
  FDECL(16)                                                                    \
  FDECL(32)                                                                    \
  FDECL(64)                                                                    \
  FDECL(128)

uchar __OVERLOADABLE__ abs(char x);
uchar __OVERLOADABLE__ abs(uchar x);
ushort __OVERLOADABLE__ abs(short x);
ushort __OVERLOADABLE__ abs(ushort x);
uint __OVERLOADABLE__ abs(int x);
uint __OVERLOADABLE__ abs(uint x);
ulong __OVERLOADABLE__ abs(int64_t x);
ulong __OVERLOADABLE__ abs(ulong x);

uchar __OVERLOADABLE__ abs_diff(char x, char y);
uchar __OVERLOADABLE__ abs_diff(uchar x, uchar y);
ushort __OVERLOADABLE__ abs_diff(short x, short y);
ushort __OVERLOADABLE__ abs_diff(ushort x, ushort y);
uint __OVERLOADABLE__ abs_diff(int x, int y);
uint __OVERLOADABLE__ abs_diff(uint x, uint y);
ulong __OVERLOADABLE__ abs_diff(int64_t x, int64_t y);
ulong __OVERLOADABLE__ abs_diff(ulong x, ulong y);

char __OVERLOADABLE__ add_sat(char x, char y);
uchar __OVERLOADABLE__ add_sat(uchar x, uchar y);
short __OVERLOADABLE__ add_sat(short x, short y);
ushort __OVERLOADABLE__ add_sat(ushort x, ushort y);
int __OVERLOADABLE__ add_sat(int x, int y);
uint __OVERLOADABLE__ add_sat(uint x, uint y);
int64_t __OVERLOADABLE__ add_sat(int64_t x, int64_t y);
ulong __OVERLOADABLE__ add_sat(ulong x, ulong y);

char __OVERLOADABLE__ rhadd(char x, char y);
uchar __OVERLOADABLE__ rhadd(uchar x, uchar y);
short __OVERLOADABLE__ rhadd(short x, short y);
ushort __OVERLOADABLE__ rhadd(ushort x, ushort y);
int __OVERLOADABLE__ rhadd(int x, int y);
uint __OVERLOADABLE__ rhadd(uint x, uint y);
int64_t __OVERLOADABLE__ rhadd(int64_t x, int64_t y);
ulong __OVERLOADABLE__ rhadd(ulong x, ulong y);

char __OVERLOADABLE__ hadd(char x, char y);
uchar __OVERLOADABLE__ hadd(uchar x, uchar y);
short __OVERLOADABLE__ hadd(short x, short y);
ushort __OVERLOADABLE__ hadd(ushort x, ushort y);
int __OVERLOADABLE__ hadd(int x, int y);
uint __OVERLOADABLE__ hadd(uint x, uint y);
int64_t __OVERLOADABLE__ hadd(int64_t x, int64_t y);
ulong __OVERLOADABLE__ hadd(ulong x, ulong y);

char __OVERLOADABLE__ clamp(char x, char y, char maxval);
uchar __OVERLOADABLE__ clamp(uchar x, uchar y, uchar maxval);
short __OVERLOADABLE__ clamp(short x, short y, short maxval);
ushort __OVERLOADABLE__ clamp(ushort x, ushort y, ushort maxval);
int __OVERLOADABLE__ clamp(int x, int y, int maxval);
uint __OVERLOADABLE__ clamp(uint x, uint y, uint maxval);
int64_t __OVERLOADABLE__ clamp(int64_t x, int64_t y, int64_t maxval);
ulong __OVERLOADABLE__ clamp(ulong x, ulong y, ulong maxval);

char __OVERLOADABLE__ clz(char x);
uchar __OVERLOADABLE__ clz(uchar x);
short __OVERLOADABLE__ clz(short x);
ushort __OVERLOADABLE__ clz(ushort x);
int __OVERLOADABLE__ clz(int x);
uint __OVERLOADABLE__ clz(uint x);
int64_t __OVERLOADABLE__ clz(int64_t x);
ulong __OVERLOADABLE__ clz(ulong x);

char __OVERLOADABLE__ mad_hi(char a, char b, char c);
uchar __OVERLOADABLE__ mad_hi(uchar a, uchar b, uchar c);
short __OVERLOADABLE__ mad_hi(short a, short b, short c);
ushort __OVERLOADABLE__ mad_hi(ushort a, ushort b, ushort c);
int __OVERLOADABLE__ mad_hi(int a, int b, int c);
uint __OVERLOADABLE__ mad_hi(uint a, uint b, uint c);
int64_t __OVERLOADABLE__ mad_hi(int64_t a, int64_t b, int64_t c);
ulong __OVERLOADABLE__ mad_hi(ulong a, ulong b, ulong c);

short32 __OVERLOADABLE__ mul_hi(short32 x, short32 y);
ushort32 __OVERLOADABLE__ mul_hi(ushort32 x, ushort32 y);
short64 __OVERLOADABLE__ mul_hi(short64 x, short64 y);
ushort64 __OVERLOADABLE__ mul_hi(ushort64 x, ushort64 y);
int16 __OVERLOADABLE__ mul_hi(int16 x, int16 y);
int32 __OVERLOADABLE__ mul_hi(int32 x, int32 y);

uint32 __OVERLOADABLE__ xt_mul32(ushort32 a, ushort32 b);
int32 __OVERLOADABLE__ xt_mul32(short32 a, short32 b);
uint64 __OVERLOADABLE__ xt_mul32(ushort64 a, ushort64 b);
int64 __OVERLOADABLE__ xt_mul32(short64 a, short64 b);
uint128 __OVERLOADABLE__ xt_mul32(ushort128 a, ushort128 b);
int128 __OVERLOADABLE__ xt_mul32(short128 a, short128 b);
uint32 __OVERLOADABLE__ xt_mad32(ushort32 a, ushort32 b, uint32 c);
int32 __OVERLOADABLE__ xt_mad32(short32 a, short32 b, int32 c);
uint32 __OVERLOADABLE__ xt_mad32(ushort32 a, short32 b, uint32 c);
uint64 __OVERLOADABLE__ xt_mad32(ushort64 a, ushort64 b, uint64 c);
int64 __OVERLOADABLE__ xt_mad32(short64 a, short64 b, int64 c);
uint64 __OVERLOADABLE__ xt_mad32(ushort64 a, short64 b, uint64 c);
uint128 __OVERLOADABLE__ xt_mad32(ushort128 a, ushort128 b, uint128 c);
int128 __OVERLOADABLE__ xt_mad32(short128 a, short128 b, int128 c);
uint128 __OVERLOADABLE__ xt_mad32(ushort128 a, short128 b, uint128 c);

int64 __OVERLOADABLE__ xt_mul24(uchar64 a, uchar64 b);
int64 __OVERLOADABLE__ xt_mad24(uchar64 a, uchar64 b, int64 c);
int64 __OVERLOADABLE__ xt_mul24(char64 a, char64 b);
int64 __OVERLOADABLE__ xt_mad24(char64 a, char64 b, int64 c);
int64 __OVERLOADABLE__ xt_mul24(uchar64 a, char64 b);
int64 __OVERLOADABLE__ xt_mad24(uchar64 a, char64 b, int64 c);

int64 __OVERLOADABLE__ xt_mul24(short64 a, char64 b);
int64 __OVERLOADABLE__ xt_mad24(short64 a, char64 b, int64 c);
int64 __OVERLOADABLE__ xt_mul24(short64 a, uchar64 b);
int64 __OVERLOADABLE__ xt_mad24(short64 a, uchar64 b, int64 c);

int64 __OVERLOADABLE__ xt_mul24(char64 a, char b);
int64 __OVERLOADABLE__ xt_mul24(uchar64 a, char b);
int64 __OVERLOADABLE__ xt_mul24(uchar64 a, uchar b);
int64 __OVERLOADABLE__ xt_mad24(char64 a, char b, int64 c);
int64 __OVERLOADABLE__ xt_mad24(uchar64 a, char b, int64 c);
int64 __OVERLOADABLE__ xt_mad24(uchar64 a, uchar b, int64 c);

int64 __OVERLOADABLE__ xt_add24(uchar64 a, uchar64 b);
int64 __OVERLOADABLE__ xt_add24(uchar64 a, uchar64 b, int64 c);
int64 __OVERLOADABLE__ xt_add24(char64 a, char64 b);
int64 __OVERLOADABLE__ xt_add24(char64 a, char64 b, int64);

int64 __OVERLOADABLE__ xt_sub24(uchar64 a, uchar64 b);
int64 __OVERLOADABLE__ xt_sub24(char64 a, char64 b);
int64 __OVERLOADABLE__ xt_sub24(uchar64 a, uchar64 b, int64 c);
int64 __OVERLOADABLE__ xt_sub24(char64 a, char64 b, int64 c);

int128 __OVERLOADABLE__ xt_mul32(uchar128 a, char128 b);
int128 __OVERLOADABLE__ xt_mad32(uchar128 a, char128 b, int128);
int128 __OVERLOADABLE__ xt_mul32(uchar128 a, uchar128 b);
int128 __OVERLOADABLE__ xt_mad32(uchar128 a, uchar128 b, int128);
int128 __OVERLOADABLE__ xt_mul32(char128 a, char128 b);
int128 __OVERLOADABLE__ xt_mad32(char128 a, char128 b, int128);

int128 __OVERLOADABLE__ xt_mul32(short128 a, char128 b);
int128 __OVERLOADABLE__ xt_mad32(short128 a, char128 b, int128 c);
int128 __OVERLOADABLE__ xt_mul32(short128 a, uchar128 b);
int128 __OVERLOADABLE__ xt_mad32(short128 a, uchar128 b, int128 c);

int128 __OVERLOADABLE__ xt_mul32(uchar128 a, char b);
int128 __OVERLOADABLE__ xt_mad32(uchar128 a, char b, int128 c);
int128 __OVERLOADABLE__ xt_mul32(char128 a, char b);
int128 __OVERLOADABLE__ xt_mad32(char128 a, char b, int128 c);
int128 __OVERLOADABLE__ xt_mul32(uchar128 a, uchar b);
int128 __OVERLOADABLE__ xt_mad32(uchar128 a, uchar b, int128 c);

int32 __OVERLOADABLE__ xt_mul32(short32 a, char b);
int32 __OVERLOADABLE__ xt_mad32(short32 a, char b, int32 c);
int64 __OVERLOADABLE__ xt_mul32(short64 a, char b);
int64 __OVERLOADABLE__ xt_mad32(short64 a, char b, int64 c);
int128 __OVERLOADABLE__ xt_mul32(short128 a, char b);
int128 __OVERLOADABLE__ xt_mad32(short128 a, char b, int128 c);

int32 __OVERLOADABLE__ xt_mul32(short32 a, short b);
int32 __OVERLOADABLE__ xt_mad32(short32 a, short b, int32 c);
int64 __OVERLOADABLE__ xt_mul32(short64 a, short b);
int64 __OVERLOADABLE__ xt_mad32(short64 a, short b, int64 c);
int128 __OVERLOADABLE__ xt_mul32(short128 a, short b);
int128 __OVERLOADABLE__ xt_mad32(short128 a, short b, int128 c);

int128 __OVERLOADABLE__ xt_add32(char128 a, char128 b);
int128 __OVERLOADABLE__ xt_add32(char128 a, char128 b, int128 c);
int128 __OVERLOADABLE__ xt_add32(uchar128 a, uchar128 b);
int128 __OVERLOADABLE__ xt_add32(uchar128 a, uchar128 b, int128 c);

int128 __OVERLOADABLE__ xt_sub32(uchar128 a, uchar128 b);
int128 __OVERLOADABLE__ xt_sub32(uchar128 a, uchar128 b, int128 c);
int128 __OVERLOADABLE__ xt_sub32(char128 a, char128 b);
int128 __OVERLOADABLE__ xt_sub32(char128 a, char128 b, int128 c);

uint32 __OVERLOADABLE__ xt_add32(ushort32 a, ushort32 b);
uint32 __OVERLOADABLE__ xt_add32(ushort32 a, ushort32 b, uint32 c);
int32 __OVERLOADABLE__ xt_add32(short32 a, short32 b);
int32 __OVERLOADABLE__ xt_add32(short32 a, short32 b, int32 c);
int64 __OVERLOADABLE__ xt_add32(short64 a, short64 b);
int64 __OVERLOADABLE__ xt_add32(short64 a, short64 b, int64 c);
uint64 __OVERLOADABLE__ xt_add32(ushort64 a, ushort64 b);
uint64 __OVERLOADABLE__ xt_add32(ushort64 a, ushort64 b, uint64 c);
int128 __OVERLOADABLE__ xt_add32(short128 a, short128 b);
int128 __OVERLOADABLE__ xt_add32(short128 a, short128 b, int128 c);
uint128 __OVERLOADABLE__ xt_add32(ushort128 a, ushort128 b);
uint128 __OVERLOADABLE__ xt_add32(ushort128 a, ushort128 b, uint128 c);

int16 __OVERLOADABLE__ xt_mul(int16 a, int16 b);
uint16 __OVERLOADABLE__ xt_mul(uint16 a, uint16 b);
int32 __OVERLOADABLE__ xt_mul(int32 a, int32 b);
uint32 __OVERLOADABLE__ xt_mul(uint32 a, uint32 b);
int64 __OVERLOADABLE__ xt_mul(int64 a, int64 b);
uint64 __OVERLOADABLE__ xt_mul(uint64 a, uint64 b);
int128 __OVERLOADABLE__ xt_mul(int128 a, int128 b);
uint128 __OVERLOADABLE__ xt_mul(uint128 a, uint128 b);

int16 __OVERLOADABLE__ xt_div(int16 x, int16 y);
uint16 __OVERLOADABLE__ xt_div(uint16 x, uint16 y);
int32 __OVERLOADABLE__ xt_div(int32 x, int32 y);
uint32 __OVERLOADABLE__ xt_div(uint32 x, uint32 y);
int64 __OVERLOADABLE__ xt_div(int64 x, int64 y);
uint64 __OVERLOADABLE__ xt_div(uint64 x, uint64 y);
int128 __OVERLOADABLE__ xt_div(int128 x, int128 y);
uint128 __OVERLOADABLE__ xt_div(uint128 x, uint128 y);

int32 __OVERLOADABLE__ xt_div(int32 x, short32 y);
int64 __OVERLOADABLE__ xt_div(int64 x, short64 y);
int128 __OVERLOADABLE__ xt_div(int128 x, short128 y);
uint32 __OVERLOADABLE__ xt_div(uint32 x, ushort32 y);
uint64 __OVERLOADABLE__ xt_div(uint64 x, ushort64 y);
uint128 __OVERLOADABLE__ xt_div(uint128 x, ushort128 y);

long16 __OVERLOADABLE__ xt_mul64(int16 a, int16 b);
long32 __OVERLOADABLE__ xt_mul64(int32 a, int32 b);
long64 __OVERLOADABLE__ xt_mul64(int64 a, int64 b);

long32 __OVERLOADABLE__ xt_mul64(int32 a, short32 b);
long64 __OVERLOADABLE__ xt_mul64(int64 a, short64 b);
long32 __OVERLOADABLE__ xt_msub64(int32 a, short32 b, long32 c);
long32 __OVERLOADABLE__ xt_mad64(int32 a, short32 b, long32 c);
long64 __OVERLOADABLE__ xt_msub64(int64 a, short64 b, long64 c);
long64 __OVERLOADABLE__ xt_mad64(int64 a, short64 b, long64 c);

char __OVERLOADABLE__ mad_sat(char a, char b, char c);
uchar __OVERLOADABLE__ mad_sat(uchar a, uchar b, uchar c);
short __OVERLOADABLE__ mad_sat(short a, short b, short c);
ushort __OVERLOADABLE__ mad_sat(ushort a, ushort b, ushort c);
int __OVERLOADABLE__ mad_sat(int a, int b, int c);
uint __OVERLOADABLE__ mad_sat(uint a, uint b, uint c);
int64_t __OVERLOADABLE__ mad_sat(int64_t a, int64_t b, int64_t c);
ulong __OVERLOADABLE__ mad_sat(ulong a, ulong b, ulong c);

char __OVERLOADABLE__ max(char x, char y);
uchar __OVERLOADABLE__ max(uchar x, uchar y);
short __OVERLOADABLE__ max(short x, short y);
ushort __OVERLOADABLE__ max(ushort x, ushort y);
int __OVERLOADABLE__ max(int x, int y);
uint __OVERLOADABLE__ max(uint x, uint y);
int64_t __OVERLOADABLE__ max(int64_t x, int64_t y);
ulong __OVERLOADABLE__ max(ulong x, ulong y);

char __OVERLOADABLE__ min(char x, char y);
uchar __OVERLOADABLE__ min(uchar x, uchar y);
short __OVERLOADABLE__ min(short x, short y);
ushort __OVERLOADABLE__ min(ushort x, ushort y);
int __OVERLOADABLE__ min(int x, int y);
uint __OVERLOADABLE__ min(uint x, uint y);
int64_t __OVERLOADABLE__ min(int64_t x, int64_t y);
ulong __OVERLOADABLE__ min(ulong x, ulong y);

char __OVERLOADABLE__ mul_hi(char x, char y);
uchar __OVERLOADABLE__ mul_hi(uchar x, uchar y);
short __OVERLOADABLE__ mul_hi(short x, short y);
ushort __OVERLOADABLE__ mul_hi(ushort x, ushort y);
int __OVERLOADABLE__ mul_hi(int x, int y);
uint __OVERLOADABLE__ mul_hi(uint x, uint y);
int64_t __OVERLOADABLE__ mul_hi(int64_t x, int64_t y);
ulong __OVERLOADABLE__ mul_hi(ulong x, ulong y);

char __OVERLOADABLE__ rotate(char v, char i);
uchar __OVERLOADABLE__ rotate(uchar v, uchar i);
short __OVERLOADABLE__ rotate(short v, short i);
ushort __OVERLOADABLE__ rotate(ushort v, ushort i);
int __OVERLOADABLE__ rotate(int v, int i);
uint __OVERLOADABLE__ rotate(uint v, uint i);
int64_t __OVERLOADABLE__ rotate(int64_t v, int64_t i);
ulong __OVERLOADABLE__ rotate(ulong v, ulong i);

char __OVERLOADABLE__ sub_sat(char x, char y);
uchar __OVERLOADABLE__ sub_sat(uchar x, uchar y);
short __OVERLOADABLE__ sub_sat(short x, short y);
ushort __OVERLOADABLE__ sub_sat(ushort x, ushort y);
int __OVERLOADABLE__ sub_sat(int x, int y);
uint __OVERLOADABLE__ sub_sat(uint x, uint y);
int64_t __OVERLOADABLE__ sub_sat(int64_t x, int64_t y);
ulong __OVERLOADABLE__ sub_sat(ulong x, ulong y);

short __OVERLOADABLE__ upsample(char hi, uchar lo);
ushort __OVERLOADABLE__ upsample(uchar hi, uchar lo);
int __OVERLOADABLE__ upsample(short hi, ushort lo);
uint __OVERLOADABLE__ upsample(ushort hi, ushort lo);
int64_t __OVERLOADABLE__ upsample(int hi, uint lo);
ulong __OVERLOADABLE__ upsample(uint hi, uint lo);

int __OVERLOADABLE__ mad24(int x, int y, int z);
int2 __OVERLOADABLE__ mad24(int2 x, int2 y, int2 z);
int3 __OVERLOADABLE__ mad24(int3 x, int3 y, int3 z);
int4 __OVERLOADABLE__ mad24(int4 x, int4 y, int4 z);
int8 __OVERLOADABLE__ mad24(int8 x, int8 y, int8 z);
int16 __OVERLOADABLE__ mad24(int16 x, int16 y, int16 z);
uint __OVERLOADABLE__ mad24(uint x, uint y, uint z);
uint2 __OVERLOADABLE__ mad24(uint2 x, uint2 y, uint2 z);
uint3 __OVERLOADABLE__ mad24(uint3 x, uint3 y, uint3 z);
uint4 __OVERLOADABLE__ mad24(uint4 x, uint4 y, uint4 z);
uint8 __OVERLOADABLE__ mad24(uint8 x, uint8 y, uint8 z);
uint16 __OVERLOADABLE__ mad24(uint16 x, uint16 y, uint16 z);

int __OVERLOADABLE__ mul24(int x, int y);
int2 __OVERLOADABLE__ mul24(int2 x, int2 y);
int3 __OVERLOADABLE__ mul24(int3 x, int3 y);
int4 __OVERLOADABLE__ mul24(int4 x, int4 y);
int8 __OVERLOADABLE__ mul24(int8 x, int8 y);
int16 __OVERLOADABLE__ mul24(int16 x, int16 y);
uint __OVERLOADABLE__ mul24(uint x, uint y);
uint2 __OVERLOADABLE__ mul24(uint2 x, uint2 y);
uint3 __OVERLOADABLE__ mul24(uint3 x, uint3 y);
uint4 __OVERLOADABLE__ mul24(uint4 x, uint4 y);
uint8 __OVERLOADABLE__ mul24(uint8 x, uint8 y);
uint16 __OVERLOADABLE__ mul24(uint16 x, uint16 y);

uint __OVERLOADABLE__ xt_reduce_add(ushort32 x);
int __OVERLOADABLE__ xt_reduce_add(short32 x);
uint __OVERLOADABLE__ xt_reduce_add_masked(ushort32 x, int32 mask);
int __OVERLOADABLE__ xt_reduce_add_masked(short32 x, int32 mask);
uint __OVERLOADABLE__ xt_reduce_add(ushort64 x);
int __OVERLOADABLE__ xt_reduce_add(short64 x);
uint __OVERLOADABLE__ xt_reduce_add_masked(ushort64 x, int64 mask);
int __OVERLOADABLE__ xt_reduce_add_masked(short64 x, int64 mask);

#if XCHAL_HAVE_CONNX_B20
long32 __OVERLOADABLE__ xt_mul40(short32 a, short32 b);
long32 __OVERLOADABLE__ xt_mad40(short32 a, short32 b, long32 c);
cfloat16 __OVERLOADABLE__ xt_mad(cfloat16 a, cfloat16 b, cfloat16 c);
#endif

#define INTEGER_MACRO(gentype, n)                                              \
  gentype##n __OVERLOADABLE__ clz(gentype##n x);                               \
  gentype##n __OVERLOADABLE__ mad_hi(gentype##n x, gentype##n y,               \
                                     gentype##n z);                            \
  gentype##n __OVERLOADABLE__ mad_sat(gentype##n x, gentype##n y,              \
                                      gentype##n z);                           \
  gentype##n __OVERLOADABLE__ rotate(gentype##n v, gentype##n u);              \
  gentype##n __OVERLOADABLE__ sub_sat(gentype##n x, gentype##n y);             \
  gentype##n __OVERLOADABLE__ add_sat(gentype##n x, gentype##n y);             \
  gentype##n __OVERLOADABLE__ hadd(gentype##n x, gentype##n y);                \
  gentype##n __OVERLOADABLE__ rhadd(gentype##n x, gentype##n y);               \
  gentype##n __OVERLOADABLE__ clamp(gentype##n x, gentype##n minval,           \
                                    gentype##n maxval);                        \
  gentype##n __OVERLOADABLE__ max(gentype##n x, gentype##n y);                 \
  gentype##n __OVERLOADABLE__ min(gentype##n x, gentype##n y);                 \
  gentype##n __OVERLOADABLE__ mul_hi(gentype##n x, gentype##n y);

#define INTEGER_MACRO_TYPE(n)                                                  \
  INTEGER_MACRO(char, n)                                                       \
  INTEGER_MACRO(uchar, n)                                                      \
  INTEGER_MACRO(short, n)                                                      \
  INTEGER_MACRO(ushort, n)                                                     \
  INTEGER_MACRO(int, n)                                                        \
  INTEGER_MACRO(uint, n)                                                       \
  INTEGER_MACRO(int64_t, n)                                                    \
  INTEGER_MACRO(ulong, n)

#define FDECL(n) INTEGER_MACRO_TYPE(n)
EXPAND_ALL_VECTOR()
#undef FDECL

#undef INTEGER_MACRO

#define INTEGER_NUM_MACRO(n)                                                   \
  short##n __OVERLOADABLE__ upsample(char##n hi, uchar##n lo);                 \
  ushort##n __OVERLOADABLE__ upsample(uchar##n hi, uchar##n lo);               \
  uint##n __OVERLOADABLE__ upsample(ushort##n hi, ushort##n lo);               \
  int##n __OVERLOADABLE__ upsample(short##n hi, ushort##n lo);                 \
  int64_t##n __OVERLOADABLE__ upsample(int##n hi, uint##n lo);                 \
  ulong##n __OVERLOADABLE__ upsample(uint##n hi, uint##n lo);

#define FDECL(n) INTEGER_NUM_MACRO(n)
EXPAND_ALL_VECTOR()
#undef FDECL

#undef INTEGER_NUM_MACRO

#define INTEGER_MACRO_ABS(rgentype, gentype, n)                                \
  rgentype##n __OVERLOADABLE__ abs(gentype##n x);                              \
  rgentype##n __OVERLOADABLE__ abs_diff(gentype##n x, gentype##n y);

#define INTEGER_MACRO_ABS_TYPE(n)                                              \
  INTEGER_MACRO_ABS(uchar, char, n)                                            \
  INTEGER_MACRO_ABS(ushort, short, n)                                          \
  INTEGER_MACRO_ABS(uint, int, n)                                              \
  INTEGER_MACRO_ABS(ulong, int64_t, n)                                         \
  INTEGER_MACRO_ABS(uchar, uchar, n)                                           \
  INTEGER_MACRO_ABS(ushort, ushort, n)                                         \
  INTEGER_MACRO_ABS(uint, uint, n)                                             \
  INTEGER_MACRO_ABS(ulong, ulong, n)

#define FDECL(n) INTEGER_MACRO_ABS_TYPE(n)
EXPAND_ALL_VECTOR()
#undef FDECL
#undef INTEGER_MACRO_ABS
#undef EXPAND_ALL_VECTOR

char __OVERLOADABLE__ popcount(char x);
uchar __OVERLOADABLE__ popcount(uchar x);
short __OVERLOADABLE__ popcount(short x);
ushort __OVERLOADABLE__ popcout(ushort);
int __OVERLOADABLE__ popcount(int x);
uint __OVERLOADABLE__ popcount(uint x);
ulong __OVERLOADABLE__ popcount(ulong x);
int64_t __OVERLOADABLE__ popcount(int64_t x);

#define EXPAND_POPCOUNT_TYPEN_DECL(genTypeA, n)                                \
  genTypeA##n __OVERLOADABLE__ popcount(genTypeA##n x);

#define EXPAND_POPCOUNT_ALLTYPEN(n)                                            \
  EXPAND_POPCOUNT_TYPEN_DECL(uchar, n);                                        \
  EXPAND_POPCOUNT_TYPEN_DECL(char, n);                                         \
  EXPAND_POPCOUNT_TYPEN_DECL(ushort, n);                                       \
  EXPAND_POPCOUNT_TYPEN_DECL(short, n);                                        \
  EXPAND_POPCOUNT_TYPEN_DECL(uint, n);                                         \
  EXPAND_POPCOUNT_TYPEN_DECL(int, n);                                          \
  EXPAND_POPCOUNT_TYPEN_DECL(ulong, n);                                        \
  EXPAND_POPCOUNT_TYPEN_DECL(int64_t, n);

EXPAND_POPCOUNT_ALLTYPEN(2)
EXPAND_POPCOUNT_ALLTYPEN(3)
EXPAND_POPCOUNT_ALLTYPEN(4)
EXPAND_POPCOUNT_ALLTYPEN(8)
EXPAND_POPCOUNT_ALLTYPEN(16)

#endif // __INTEGER_FUN_H__
