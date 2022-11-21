/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of
 * Cadence Design Systems Inc. Any rights to use, modify, and create
 * derivative works of this file are set forth under the terms of your
 * license agreement with Cadence Design Systems, Inc.
 */

#ifndef __VECTOR_DATA_LOAD_STORE_H__
#define __VECTOR_DATA_LOAD_STORE_H__

#ifndef __OVERLOADABLE__
#define __OVERLOADABLE__ __attribute__((overloadable))
#endif

#define EXPAND_ALL_VECTOR(ADDR_SPACE)                                          \
  FDECL(ADDR_SPACE, 2)                                                         \
  FDECL(ADDR_SPACE, 3)                                                         \
  FDECL(ADDR_SPACE, 4)                                                         \
  FDECL(ADDR_SPACE, 8)                                                         \
  FDECL(ADDR_SPACE, 16)                                                        \
  FDECL(ADDR_SPACE, 32)                                                        \
  FDECL(ADDR_SPACE, 64)                                                        \
  FDECL(ADDR_SPACE, 128)

#define S_EXPAND(ADDR_SPACE)                                                   \
  float __OVERLOADABLE__ vload_half(size_t offset, const ADDR_SPACE half *p);  \
  void __OVERLOADABLE__ vstore_half(float data, size_t offset,                 \
                                    ADDR_SPACE half *p);                       \
  void __OVERLOADABLE__ vstore_half_rte(float data, size_t offset,             \
                                        ADDR_SPACE half *p);                   \
  void __OVERLOADABLE__ vstore_half_rtz(float data, size_t offset,             \
                                        ADDR_SPACE half *p);                   \
  void __OVERLOADABLE__ vstore_half_rtp(float data, size_t offset,             \
                                        ADDR_SPACE half *p);                   \
  void __OVERLOADABLE__ vstore_half_rtn(float data, size_t offset,             \
                                        ADDR_SPACE half *p);                   \
  float __OVERLOADABLE__ vloada_half(size_t offset, const ADDR_SPACE half *p); \
  void __OVERLOADABLE__ vstorea_half(float data, size_t offset,                \
                                     ADDR_SPACE half *p);                      \
  void __OVERLOADABLE__ vstorea_half_rte(float data, size_t offset,            \
                                         ADDR_SPACE half *p);                  \
  void __OVERLOADABLE__ vstorea_half_rtz(float data, size_t offset,            \
                                         ADDR_SPACE half *p);                  \
  void __OVERLOADABLE__ vstorea_half_rtp(float data, size_t offset,            \
                                         ADDR_SPACE half *p);                  \
  void __OVERLOADABLE__ vstorea_half_rtn(float data, size_t offset,            \
                                         ADDR_SPACE half *p);

S_EXPAND(__global);
S_EXPAND(__local);
S_EXPAND(__private);
S_EXPAND(__constant);
#undef S_EXPAND

#define VECTOR_MACRO_VLOADN(ADDR_SPACE, gentype, n)                            \
  gentype##n __OVERLOADABLE__ vload##n(size_t offset,                          \
                                       const ADDR_SPACE gentype *p);           \
  void __OVERLOADABLE__ vstore##n(gentype##n data, size_t offset,              \
                                  ADDR_SPACE gentype *p);

#define VECTOR_MACRO_VLOADN_TYPE(ADDR_SPACE, n)                                \
  VECTOR_MACRO_VLOADN(ADDR_SPACE, char, n)                                     \
  VECTOR_MACRO_VLOADN(ADDR_SPACE, uchar, n)                                    \
  VECTOR_MACRO_VLOADN(ADDR_SPACE, short, n)                                    \
  VECTOR_MACRO_VLOADN(ADDR_SPACE, ushort, n)                                   \
  VECTOR_MACRO_VLOADN(ADDR_SPACE, int, n)                                      \
  VECTOR_MACRO_VLOADN(ADDR_SPACE, uint, n)                                     \
  VECTOR_MACRO_VLOADN(ADDR_SPACE, int64_t, n)                                  \
  VECTOR_MACRO_VLOADN(ADDR_SPACE, ulong, n)                                    \
  VECTOR_MACRO_VLOADN(ADDR_SPACE, float, n)                                    \
  VECTOR_MACRO_VLOADN(ADDR_SPACE, double, n)

#define FDECL(ADDR_SPACE, n) VECTOR_MACRO_VLOADN_TYPE(ADDR_SPACE, n)
EXPAND_ALL_VECTOR(__global)
EXPAND_ALL_VECTOR(__local)
EXPAND_ALL_VECTOR(__private)
EXPAND_ALL_VECTOR(__constant)
#undef FDECL

#undef VECTOR_MACRO_VLOADN

// The following functions are half_fun and these functions are
// declared but not implemented
#define VECTOR_MACRO_VLOAD_HALF(ADDR_SPACE, n)                                 \
  float##n __OVERLOADABLE__ vload_half##n(size_t offset,                       \
                                          const ADDR_SPACE half *p);           \
  void __OVERLOADABLE__ vstore_half##n(float##n data, size_t offset,           \
                                       ADDR_SPACE half *p);                    \
  void __OVERLOADABLE__ vstore_half##n##_rte(float##n data, size_t offset,     \
                                             ADDR_SPACE half *p);              \
  void __OVERLOADABLE__ vstore_half##n##_rtz(float##n data, size_t offset,     \
                                             ADDR_SPACE half *p);              \
  void __OVERLOADABLE__ vstore_half##n##_rtp(float##n data, size_t offset,     \
                                             ADDR_SPACE half *p);              \
  void __OVERLOADABLE__ vstore_half##n##_rtn(float##n data, size_t offset,     \
                                             ADDR_SPACE half *p);              \
  float##n __OVERLOADABLE__ vloada_half##n(size_t offset,                      \
                                           const ADDR_SPACE half *p);          \
  void __OVERLOADABLE__ vstorea_half##n(float##n data, size_t offset,          \
                                        ADDR_SPACE half *p);                   \
  void __OVERLOADABLE__ vstorea_half##n##_rte(float##n data, size_t offset,    \
                                              ADDR_SPACE half *p);             \
  void __OVERLOADABLE__ vstorea_half##n##_rtz(float##n data, size_t offset,    \
                                              ADDR_SPACE half *p);             \
  void __OVERLOADABLE__ vstorea_half##n##_rtp(float##n data, size_t offset,    \
                                              ADDR_SPACE half *p);             \
  void __OVERLOADABLE__ vstorea_half##n##_rtn(float##n data, size_t offset,    \
                                              ADDR_SPACE half *p);

#define FDECL(ADDR_SPACE, n) VECTOR_MACRO_VLOAD_HALF(ADDR_SPACE, n)
EXPAND_ALL_VECTOR(__global)
EXPAND_ALL_VECTOR(__local)
EXPAND_ALL_VECTOR(__private)
EXPAND_ALL_VECTOR(__constant)
#undef FDECL
#undef EXPAND_ALL_VECTOR

#endif // __VECTOR_DATA_LOAD_STORE_H__
