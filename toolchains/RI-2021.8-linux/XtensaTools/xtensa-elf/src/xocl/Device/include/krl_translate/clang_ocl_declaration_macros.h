/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of
 * Cadence Design Systems Inc. Any rights to use, modify, and create
 * derivative works of this file are set forth under the terms of your
 * license agreement with Cadence Design Systems, Inc.
 */

// THIS FILE IN AN EXCEPTION TO THE INCLUDE-ONCE PROGRAMMING PRACTICE
// This file is meant to be included repeatedly with different active
// definitions of the macro CLANG_OCL_MACRO

#define FOR_ALL_VECTOR_LENGTHS(basetype)                                       \
  CLANG_OCL_MACRO(basetype, 2);                                                \
  CLANG_OCL_MACRO(basetype, 3);                                                \
  CLANG_OCL_MACRO(basetype, 4);                                                \
  CLANG_OCL_MACRO(basetype, 8);                                                \
  CLANG_OCL_MACRO(basetype, 16);                                               \
  CLANG_OCL_MACRO(basetype, 32);                                               \
  CLANG_OCL_MACRO(basetype, 64);                                               \
  CLANG_OCL_MACRO(basetype, 128)

#define FOR_ALL_VECTOR_LENGTHS_SIGNED_AND_UNSIGNED(basetype)                   \
  FOR_ALL_VECTOR_LENGTHS(basetype);                                            \
  FOR_ALL_VECTOR_LENGTHS(u##basetype)

#define EXPAND_MACRO_FOR_ALL_BUILTIN_TYPES                                     \
  FOR_ALL_VECTOR_LENGTHS_SIGNED_AND_UNSIGNED(char);                            \
  FOR_ALL_VECTOR_LENGTHS_SIGNED_AND_UNSIGNED(short);                           \
  FOR_ALL_VECTOR_LENGTHS_SIGNED_AND_UNSIGNED(int);                             \
  FOR_ALL_VECTOR_LENGTHS(int64_t);                                             \
  FOR_ALL_VECTOR_LENGTHS(ulong);                                               \
  FOR_ALL_VECTOR_LENGTHS(float);                                               \
  FOR_ALL_VECTOR_LENGTHS(double);
