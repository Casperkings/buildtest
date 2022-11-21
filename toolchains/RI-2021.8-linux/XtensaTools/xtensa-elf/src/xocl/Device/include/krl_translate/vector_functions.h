/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of
 * Cadence Design Systems Inc. Any rights to use, modify, and create
 * derivative works of this file are set forth under the terms of your
 * license agreement with Cadence Design Systems, Inc.
 */

#include "clang_ocl_declaration_macros.h"

#ifndef __VECTOR_FUNCTIONS_H__
#define __VECTOR_FUNCTIONS_H__

#define __OVERLOADABLE__ __attribute__((overloadable))

#define VLOADSTORE(basetype, n)                                                \
  basetype##n __OVERLOADABLE__ vload##n(size_t offset, const basetype *p);     \
  basetype##n __OVERLOADABLE__ vload##n(size_t offset,                         \
                                        __global const basetype *p);           \
  basetype##n __OVERLOADABLE__ vload##n(size_t offset,                         \
                                        __constant const basetype *p);         \
  void __OVERLOADABLE__ vstore##n(basetype##n data, size_t offset,             \
                                  basetype *p);                                \
  void __OVERLOADABLE__ vstore##n(basetype##n data, size_t offset,             \
                                  __global basetype *p)

#define CLANG_OCL_MACRO(basetype, n) VLOADSTORE(basetype, n)
EXPAND_MACRO_FOR_ALL_BUILTIN_TYPES;
#undef CLANG_OCL_MACRO

#undef __OVERLOADABLE__

#endif // __VECTOR_FUNCTIONS_H__
