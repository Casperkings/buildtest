/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of
 * Cadence Design Systems Inc. Any rights to use, modify, and create
 * derivative works of this file are set forth under the terms of your
 * license agreement with Cadence Design Systems, Inc.
 */

#ifndef __RELATIONAL_FUN_H__
#define __RELATIONAL_FUN_H__

#define __OVERLOADABLE__ __attribute__((overloadable))

int __OVERLOADABLE__ isequal(float x, float y);
int __OVERLOADABLE__ isnotequal(float x, float y);
int __OVERLOADABLE__ isgreater(float x, float y);
int __OVERLOADABLE__ isgreaterequal(float x, float y);
int __OVERLOADABLE__ isless(float x, float y);
int __OVERLOADABLE__ islessequal(float x, float y);
int __OVERLOADABLE__ islessgreater(float x, float y);
int __OVERLOADABLE__ isfinite(float);
int __OVERLOADABLE__ isinf(float);
int __OVERLOADABLE__ isnan(float);
int __OVERLOADABLE__ isnormal(float);
int __OVERLOADABLE__ isordered(float x, float y);
int __OVERLOADABLE__ isunordered(float x, float y);
int __OVERLOADABLE__ signbit(float);

int __OVERLOADABLE__ isequal(double x, double y);
int __OVERLOADABLE__ isnotequal(double x, double y);
int __OVERLOADABLE__ isgreater(double x, double y);
int __OVERLOADABLE__ isgreaterequal(double x, double y);
int __OVERLOADABLE__ isless(double x, double y);
int __OVERLOADABLE__ islessequal(double x, double y);
int __OVERLOADABLE__ islessgreater(double x, double y);
int __OVERLOADABLE__ isfinite(double);
int __OVERLOADABLE__ isinf(double);
int __OVERLOADABLE__ isnan(double);
int __OVERLOADABLE__ isnormal(double);
int __OVERLOADABLE__ isordered(double x, double y);
int __OVERLOADABLE__ isunordered(double x, double y);
int __OVERLOADABLE__ signbit(double);

int __OVERLOADABLE__ any(char x);
int __OVERLOADABLE__ any(char2 x);
int __OVERLOADABLE__ any(char3 x);
int __OVERLOADABLE__ any(char4 x);
int __OVERLOADABLE__ any(char8 x);
int __OVERLOADABLE__ any(char16 x);
int __OVERLOADABLE__ any(short x);
int __OVERLOADABLE__ any(short2 x);
int __OVERLOADABLE__ any(short3 x);
int __OVERLOADABLE__ any(short4 x);
int __OVERLOADABLE__ any(short8 x);
int __OVERLOADABLE__ any(short16 x);
int __OVERLOADABLE__ any(int x);
int __OVERLOADABLE__ any(int2 x);
int __OVERLOADABLE__ any(int3 x);
int __OVERLOADABLE__ any(int4 x);
int __OVERLOADABLE__ any(int8 x);
int __OVERLOADABLE__ any(int16 x);
int __OVERLOADABLE__ any(long x);
int __OVERLOADABLE__ any(long2 x);
int __OVERLOADABLE__ any(long3 x);
int __OVERLOADABLE__ any(long4 x);
int __OVERLOADABLE__ any(long8 x);
int __OVERLOADABLE__ any(long16 x);

int __OVERLOADABLE__ all(char x);
int __OVERLOADABLE__ all(char2 x);
int __OVERLOADABLE__ all(char3 x);
int __OVERLOADABLE__ all(char4 x);
int __OVERLOADABLE__ all(char8 x);
int __OVERLOADABLE__ all(char16 x);
int __OVERLOADABLE__ all(short x);
int __OVERLOADABLE__ all(short2 x);
int __OVERLOADABLE__ all(short3 x);
int __OVERLOADABLE__ all(short4 x);
int __OVERLOADABLE__ all(short8 x);
int __OVERLOADABLE__ all(short16 x);
int __OVERLOADABLE__ all(int x);
int __OVERLOADABLE__ all(int2 x);
int __OVERLOADABLE__ all(int3 x);
int __OVERLOADABLE__ all(int4 x);
int __OVERLOADABLE__ all(int8 x);
int __OVERLOADABLE__ all(int16 x);
int __OVERLOADABLE__ all(long x);
int __OVERLOADABLE__ all(long2 x);
int __OVERLOADABLE__ all(long3 x);
int __OVERLOADABLE__ all(long4 x);
int __OVERLOADABLE__ all(long8 x);
int __OVERLOADABLE__ all(long16 x);

char __OVERLOADABLE__ bitselect(char a, char b, char c);
uchar __OVERLOADABLE__ bitselect(uchar a, uchar b, uchar c);
short __OVERLOADABLE__ bitselect(short a, short b, short c);
ushort __OVERLOADABLE__ bitselect(ushort a, ushort b, ushort c);
int __OVERLOADABLE__ bitselect(int a, int b, int c);
uint __OVERLOADABLE__ bitselect(uint a, uint b, uint c);
long __OVERLOADABLE__ bitselect(long a, long b, long c);
ulong __OVERLOADABLE__ bitselect(ulong a, ulong b, ulong c);
float __OVERLOADABLE__ bitselect(float a, float b, float c);
double __OVERLOADABLE__ bitselect(double a, double b, double c);

char __OVERLOADABLE__ select(char a, char b, char c);
uchar __OVERLOADABLE__ select(uchar a, uchar b, char c);
short __OVERLOADABLE__ select(short a, short b, short c);
ushort __OVERLOADABLE__ select(ushort a, ushort b, short c);
int __OVERLOADABLE__ select(int a, int b, int c);
uint __OVERLOADABLE__ select(uint a, uint b, int c);
long __OVERLOADABLE__ select(long a, long b, long c);
ulong __OVERLOADABLE__ select(ulong a, ulong b, long c);
float __OVERLOADABLE__ select(float a, float b, int c);
double __OVERLOADABLE__ select(double a, double b, int64_t c);

char __OVERLOADABLE__ select(char a, char b, uchar c);
uchar __OVERLOADABLE__ select(uchar a, uchar b, uchar c);
short __OVERLOADABLE__ select(short a, short b, ushort c);
ushort __OVERLOADABLE__ select(ushort a, ushort b, ushort c);
int __OVERLOADABLE__ select(int a, int b, uint c);
uint __OVERLOADABLE__ select(uint a, uint b, uint c);
long __OVERLOADABLE__ select(long a, long b, ulong c);
ulong __OVERLOADABLE__ select(ulong a, ulong b, ulong c);
float __OVERLOADABLE__ select(float a, float b, uint c);
double __OVERLOADABLE__ select(double a, double b, ulong c);

#define RELATIONAL_MACRO_NUM(n)                                                \
  int##n __OVERLOADABLE__ isequal(float##n x, float##n y);                     \
  int##n __OVERLOADABLE__ isnotequal(float##n x, float##n y);                  \
  int##n __OVERLOADABLE__ isgreater(float##n x, float##n y);                   \
  int##n __OVERLOADABLE__ isgreaterequal(float##n x, float##n y);              \
  int##n __OVERLOADABLE__ isless(float##n x, float##n y);                      \
  int##n __OVERLOADABLE__ islessequal(float##n x, float##n y);                 \
  int##n __OVERLOADABLE__ islessgreater(float##n x, float##n y);               \
  int##n __OVERLOADABLE__ isfinite(float##n);                                  \
  int##n __OVERLOADABLE__ isinf(float##n);                                     \
  int##n __OVERLOADABLE__ isnan(float##n);                                     \
  int##n __OVERLOADABLE__ isnormal(float##n);                                  \
  int##n __OVERLOADABLE__ isordered(float##n x, float##n y);                   \
  int##n __OVERLOADABLE__ isunordered(float##n x, float##n y);                 \
  int##n __OVERLOADABLE__ signbit(float##n);                                   \
  long##n __OVERLOADABLE__ isequal(double##n x, double##n y);                  \
  long##n __OVERLOADABLE__ isnotequal(double##n x, double##n y);               \
  long##n __OVERLOADABLE__ isgreater(double##n x, double##n y);                \
  long##n __OVERLOADABLE__ isgreaterequal(double##n x, double##n y);           \
  long##n __OVERLOADABLE__ isless(double##n x, double##n y);                   \
  long##n __OVERLOADABLE__ islessequal(double##n x, double##n y);              \
  long##n __OVERLOADABLE__ islessgreater(double##n x, double##n y);            \
  long##n __OVERLOADABLE__ isfinite(double##n);                                \
  long##n __OVERLOADABLE__ isinf(double##n);                                   \
  long##n __OVERLOADABLE__ isnan(double##n);                                   \
  long##n __OVERLOADABLE__ isnormal(double##n);                                \
  long##n __OVERLOADABLE__ isordered(double##n x, double##n y);                \
  long##n __OVERLOADABLE__ isunordered(double##n x, double##n y);              \
  long##n __OVERLOADABLE__ signbit(double##n);

RELATIONAL_MACRO_NUM(2)
RELATIONAL_MACRO_NUM(3)
RELATIONAL_MACRO_NUM(4)
RELATIONAL_MACRO_NUM(8)
RELATIONAL_MACRO_NUM(16)
#undef RELATIONAL_MACRO_NUM

#define MACRO_BITSELECT(gentype, n)                                            \
  gentype##n __OVERLOADABLE__ bitselect(gentype##n a, gentype##n b,            \
                                        gentype##n c);

#define MACRO_BITSELECT_TYPE(n)                                                \
  MACRO_BITSELECT(char, n)                                                     \
  MACRO_BITSELECT(uchar, n)                                                    \
  MACRO_BITSELECT(short, n)                                                    \
  MACRO_BITSELECT(ushort, n)                                                   \
  MACRO_BITSELECT(int, n)                                                      \
  MACRO_BITSELECT(uint, n)                                                     \
  MACRO_BITSELECT(long, n)                                                     \
  MACRO_BITSELECT(ulong, n)                                                    \
  MACRO_BITSELECT(float, n)                                                    \
  MACRO_BITSELECT(double, n)

MACRO_BITSELECT_TYPE(2)
MACRO_BITSELECT_TYPE(3)
MACRO_BITSELECT_TYPE(4)
MACRO_BITSELECT_TYPE(8)
MACRO_BITSELECT_TYPE(16)
#undef MACRO_BITSELECT

#define RELATIONAL_MACRO_SELECT_I(gentype, igentype, n)                        \
  gentype##n __OVERLOADABLE__ select(gentype##n a, gentype##n b, igentype##n c);

#define RELATIONAL_MACRO_SELECT_U(gentype, ugentype, n)                        \
  gentype##n __OVERLOADABLE__ select(gentype##n a, gentype##n b, ugentype##n c);

#define RELATIONAL_MACRO_SELECT_I_TYPE(n)                                      \
  RELATIONAL_MACRO_SELECT_I(char, char, n)                                     \
  RELATIONAL_MACRO_SELECT_I(uchar, char, n)                                    \
  RELATIONAL_MACRO_SELECT_I(short, short, n)                                   \
  RELATIONAL_MACRO_SELECT_I(ushort, short, n)                                  \
  RELATIONAL_MACRO_SELECT_I(int, int, n)                                       \
  RELATIONAL_MACRO_SELECT_I(uint, int, n)                                      \
  RELATIONAL_MACRO_SELECT_I(long, long, n)                                     \
  RELATIONAL_MACRO_SELECT_I(ulong, long, n)                                    \
  RELATIONAL_MACRO_SELECT_I(float, int, n)                                     \
  RELATIONAL_MACRO_SELECT_U(float, uint, n)                                    \
  RELATIONAL_MACRO_SELECT_I(double, int64_t, n)                                \
  RELATIONAL_MACRO_SELECT_U(double, ulong, n)                                  \
  RELATIONAL_MACRO_SELECT_U(char, uchar, n)                                    \
  RELATIONAL_MACRO_SELECT_U(uchar, uchar, n)                                   \
  RELATIONAL_MACRO_SELECT_U(short, ushort, n)                                  \
  RELATIONAL_MACRO_SELECT_U(ushort, ushort, n)                                 \
  RELATIONAL_MACRO_SELECT_U(int, uint, n)                                      \
  RELATIONAL_MACRO_SELECT_U(uint, uint, n)                                     \
  RELATIONAL_MACRO_SELECT_U(long, ulong, n)                                    \
  RELATIONAL_MACRO_SELECT_U(ulong, ulong, n)

#define RELATIONAL_MACRO_SELECT_FUNCTION                                       \
  RELATIONAL_MACRO_SELECT_I_TYPE(2)                                            \
  RELATIONAL_MACRO_SELECT_I_TYPE(3)                                            \
  RELATIONAL_MACRO_SELECT_I_TYPE(4)                                            \
  RELATIONAL_MACRO_SELECT_I_TYPE(8)                                            \
  RELATIONAL_MACRO_SELECT_I_TYPE(16)                                           \
  RELATIONAL_MACRO_SELECT_I_TYPE(32)

RELATIONAL_MACRO_SELECT_FUNCTION

#undef RELATIONAL_MACRO_SELECT_I
#undef RELATIONAL_MACRO_SELECT_U

#endif // __RELATIONAL_FUN_H__
