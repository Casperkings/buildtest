/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of
 * Cadence Design Systems Inc. Any rights to use, modify, and create
 * derivative works of this file are set forth under the terms of your
 * license agreement with Cadence Design Systems, Inc.
 */

#ifndef __ASYNC_COPIES_FUN_H__
#define __ASYNC_COPIES_FUN_H__

#define __OVERLOADABLE__ __attribute__((overloadable))

#define event_t int

void __OVERLOADABLE__ wait_group_events(int num_events, event_t *event_list);

event_t __OVERLOADABLE__ async_work_group_copy(__local char *dst,
                                               const __global char *src,
                                               size_t num_gentypes,
                                               event_t event);
event_t __OVERLOADABLE__ async_work_group_copy(__global char *dst,
                                               const __local char *src,
                                               size_t num_gentypes,
                                               event_t event);

event_t __OVERLOADABLE__ async_work_group_strided_copy(__local char *dst,
                                                       const __global char *src,
                                                       size_t num_gentypes,
                                                       size_t src_stride,
                                                       event_t event);
event_t __OVERLOADABLE__ async_work_group_strided_copy(__global char *dst,
                                                       const __local char *src,
                                                       size_t num_gentypes,
                                                       size_t dst_stride,
                                                       event_t event);

void __OVERLOADABLE__ prefetch(const __global char *p, size_t num_gentypes);

event_t __OVERLOADABLE__ async_work_group_copy(__local uchar *dst,
                                               const __global uchar *src,
                                               size_t num_gentypes,
                                               event_t event);
event_t __OVERLOADABLE__ async_work_group_copy(__global uchar *dst,
                                               const __local uchar *src,
                                               size_t num_gentypes,
                                               event_t event);

event_t __OVERLOADABLE__ async_work_group_strided_copy(
    __local uchar *dst, const __global uchar *src, size_t num_gentypes,
    size_t src_stride, event_t event);
event_t __OVERLOADABLE__ async_work_group_strided_copy(__global uchar *dst,
                                                       const __local uchar *src,
                                                       size_t num_gentypes,
                                                       size_t dst_stride,
                                                       event_t event);

void __OVERLOADABLE__ prefetch(const __global uchar *p, size_t num_gentypes);

event_t __OVERLOADABLE__ async_work_group_copy(__local short *dst,
                                               const __global short *src,
                                               size_t num_gentypes,
                                               event_t event);
event_t __OVERLOADABLE__ async_work_group_copy(__global short *dst,
                                               const __local short *src,
                                               size_t num_gentypes,
                                               event_t event);

event_t __OVERLOADABLE__ async_work_group_strided_copy(
    __local short *dst, const __global short *src, size_t num_gentypes,
    size_t src_stride, event_t event);
event_t __OVERLOADABLE__ async_work_group_strided_copy(__global short *dst,
                                                       const __local short *src,
                                                       size_t num_gentypes,
                                                       size_t dst_stride,
                                                       event_t event);

void __OVERLOADABLE__ prefetch(const __global short *p, size_t num_gentypes);

event_t __OVERLOADABLE__ async_work_group_copy(__local ushort *dst,
                                               const __global ushort *src,
                                               size_t num_gentypes,
                                               event_t event);
event_t __OVERLOADABLE__ async_work_group_copy(__global ushort *dst,
                                               const __local ushort *src,
                                               size_t num_gentypes,
                                               event_t event);

event_t __OVERLOADABLE__ async_work_group_strided_copy(
    __local ushort *dst, const __global ushort *src, size_t num_gentypes,
    size_t src_stride, event_t event);
event_t __OVERLOADABLE__ async_work_group_strided_copy(
    __global ushort *dst, const __local ushort *src, size_t num_gentypes,
    size_t dst_stride, event_t event);

void __OVERLOADABLE__ prefetch(const __global ushort *p, size_t num_gentypes);

event_t __OVERLOADABLE__ async_work_group_copy(__local int *dst,
                                               const __global int *src,
                                               size_t num_gentypes,
                                               event_t event);
event_t __OVERLOADABLE__ async_work_group_copy(__global int *dst,
                                               const __local int *src,
                                               size_t num_gentypes,
                                               event_t event);

event_t __OVERLOADABLE__ async_work_group_strided_copy(__local int *dst,
                                                       const __global int *src,
                                                       size_t num_gentypes,
                                                       size_t src_stride,
                                                       event_t event);
event_t __OVERLOADABLE__ async_work_group_strided_copy(__global int *dst,
                                                       const __local int *src,
                                                       size_t num_gentypes,
                                                       size_t dst_stride,
                                                       event_t event);

void __OVERLOADABLE__ prefetch(const __global int *p, size_t num_gentypes);

event_t __OVERLOADABLE__ async_work_group_copy(__local uint *dst,
                                               const __global uint *src,
                                               size_t num_gentypes,
                                               event_t event);
event_t __OVERLOADABLE__ async_work_group_copy(__global uint *dst,
                                               const __local uint *src,
                                               size_t num_gentypes,
                                               event_t event);

event_t __OVERLOADABLE__ async_work_group_strided_copy(__local uint *dst,
                                                       const __global uint *src,
                                                       size_t num_gentypes,
                                                       size_t src_stride,
                                                       event_t event);
event_t __OVERLOADABLE__ async_work_group_strided_copy(__global uint *dst,
                                                       const __local uint *src,
                                                       size_t num_gentypes,
                                                       size_t dst_stride,
                                                       event_t event);

void __OVERLOADABLE__ prefetch(const __global uint *p, size_t num_gentypes);

event_t __OVERLOADABLE__ async_work_group_copy(__local long *dst,
                                               const __global long *src,
                                               size_t num_gentypes,
                                               event_t event);
event_t __OVERLOADABLE__ async_work_group_copy(__global long *dst,
                                               const __local long *src,
                                               size_t num_gentypes,
                                               event_t event);

event_t __OVERLOADABLE__ async_work_group_strided_copy(__local long *dst,
                                                       const __global long *src,
                                                       size_t num_gentypes,
                                                       size_t src_stride,
                                                       event_t event);
event_t __OVERLOADABLE__ async_work_group_strided_copy(__global long *dst,
                                                       const __local long *src,
                                                       size_t num_gentypes,
                                                       size_t dst_stride,
                                                       event_t event);

void __OVERLOADABLE__ prefetch(const __global long *p, size_t num_gentypes);

event_t __OVERLOADABLE__ async_work_group_copy(__local ulong *dst,
                                               const __global ulong *src,
                                               size_t num_gentypes,
                                               event_t event);
event_t __OVERLOADABLE__ async_work_group_copy(__global ulong *dst,
                                               const __local ulong *src,
                                               size_t num_gentypes,
                                               event_t event);

event_t __OVERLOADABLE__ async_work_group_strided_copy(
    __local ulong *dst, const __global ulong *src, size_t num_gentypes,
    size_t src_stride, event_t event);
event_t __OVERLOADABLE__ async_work_group_strided_copy(__global ulong *dst,
                                                       const __local ulong *src,
                                                       size_t num_gentypes,
                                                       size_t dst_stride,
                                                       event_t event);

void __OVERLOADABLE__ prefetch(const __global ulong *p, size_t num_gentypes);

event_t __OVERLOADABLE__ async_work_group_copy(__local float *dst,
                                               const __global float *src,
                                               size_t num_gentypes,
                                               event_t event);
event_t __OVERLOADABLE__ async_work_group_copy(__global float *dst,
                                               const __local float *src,
                                               size_t num_gentypes,
                                               event_t event);

event_t __OVERLOADABLE__ async_work_group_strided_copy(
    __local float *dst, const __global float *src, size_t num_gentypes,
    size_t src_stride, event_t event);
event_t __OVERLOADABLE__ async_work_group_strided_copy(__global float *dst,
                                                       const __local float *src,
                                                       size_t num_gentypes,
                                                       size_t dst_stride,
                                                       event_t event);

void __OVERLOADABLE__ prefetch(const __global float *p, size_t num_gentypes);

event_t __OVERLOADABLE__ async_work_group_copy(__local double *dst,
                                               const __global double *src,
                                               size_t num_gentypes,
                                               event_t event);
event_t __OVERLOADABLE__ async_work_group_copy(__global double *dst,
                                               const __local double *src,
                                               size_t num_gentypes,
                                               event_t event);

event_t __OVERLOADABLE__ async_work_group_strided_copy(
    __local double *dst, const __global double *src, size_t num_gentypes,
    size_t src_stride, event_t event);
event_t __OVERLOADABLE__ async_work_group_strided_copy(
    __global double *dst, const __local double *src, size_t num_gentypes,
    size_t dst_stride, event_t event);

void __OVERLOADABLE__ prefetch(const __global double *p, size_t num_gentypes);

// xtensa specific async copies
event_t __OVERLOADABLE__ xt_async_work_group_2d_copy(
    __local char *dst, const __global char *src, size_t row_size,
    size_t num_rows, size_t dst_pitch, size_t src_pitch, event_t event);
event_t __OVERLOADABLE__ xt_async_work_group_2d_copy(
    __global char *dst, const __local char *src, size_t row_size,
    size_t num_rows, size_t dst_pitch, size_t src_pitch, event_t event);

event_t __OVERLOADABLE__ xt_async_work_group_2d_copy(
    __local uchar *dst, const __global uchar *src, size_t row_size,
    size_t num_rows, size_t dst_pitch, size_t src_pitch, event_t event);
event_t __OVERLOADABLE__ xt_async_work_group_2d_copy(
    __global uchar *dst, const __local uchar *src, size_t row_size,
    size_t num_rows, size_t dst_pitch, size_t src_pitch, event_t event);

event_t __OVERLOADABLE__ xt_async_work_group_2d_copy(
    __local short *dst, const __global short *src, size_t row_size,
    size_t num_rows, size_t dst_pitch, size_t src_pitch, event_t event);
event_t __OVERLOADABLE__ xt_async_work_group_2d_copy(
    __global short *dst, const __local short *src, size_t row_size,
    size_t num_rows, size_t dst_pitch, size_t src_pitch, event_t event);

event_t __OVERLOADABLE__ xt_async_work_group_2d_copy(
    __local ushort *dst, const __global ushort *src, size_t row_size,
    size_t num_rows, size_t dst_pitch, size_t src_pitch, event_t event);
event_t __OVERLOADABLE__ xt_async_work_group_2d_copy(
    __global ushort *dst, const __local ushort *src, size_t row_size,
    size_t num_rows, size_t dst_pitch, size_t src_pitch, event_t event);

event_t __OVERLOADABLE__ xt_async_work_group_2d_copy(
    __local int *dst, const __global int *src, size_t row_size, size_t num_rows,
    size_t dst_pitch, size_t src_pitch, event_t event);
event_t __OVERLOADABLE__ xt_async_work_group_2d_copy(
    __global int *dst, const __local int *src, size_t row_size, size_t num_rows,
    size_t dst_pitch, size_t src_pitch, event_t event);

event_t __OVERLOADABLE__ xt_async_work_group_2d_copy(
    __local uint *dst, const __global uint *src, size_t row_size,
    size_t num_rows, size_t dst_pitch, size_t src_pitch, event_t event);
event_t __OVERLOADABLE__ xt_async_work_group_2d_copy(
    __global uint *dst, const __local uint *src, size_t row_size,
    size_t num_rows, size_t dst_pitch, size_t src_pitch, event_t event);

event_t __OVERLOADABLE__ xt_async_work_group_2d_copy(
    __local long *dst, const __global long *src, size_t row_size,
    size_t num_rows, size_t dst_pitch, size_t src_pitch, event_t event);
event_t __OVERLOADABLE__ xt_async_work_group_2d_copy(
    __global long *dst, const __local long *src, size_t row_size,
    size_t num_rows, size_t dst_pitch, size_t src_pitch, event_t event);

event_t __OVERLOADABLE__ xt_async_work_group_2d_copy(
    __local ulong *dst, const __global ulong *src, size_t row_size,
    size_t num_rows, size_t dst_pitch, size_t src_pitch, event_t event);
event_t __OVERLOADABLE__ xt_async_work_group_2d_copy(
    __global ulong *dst, const __local ulong *src, size_t row_size,
    size_t num_rows, size_t dst_pitch, size_t src_pitch, event_t event);

event_t __OVERLOADABLE__ xt_async_work_group_2d_copy(
    __local float *dst, const __global float *src, size_t row_size,
    size_t num_rows, size_t dst_pitch, size_t src_pitch, event_t event);
event_t __OVERLOADABLE__ xt_async_work_group_2d_copy(
    __global float *dst, const __local float *src, size_t row_size,
    size_t num_rows, size_t dst_pitch, size_t src_pitch, event_t event);

event_t __OVERLOADABLE__ xt_async_work_group_2d_copy(
    __local half *dst, const __global half *src, size_t row_size,
    size_t num_rows, size_t dst_pitch, size_t src_pitch, event_t event);
event_t __OVERLOADABLE__ xt_async_work_group_2d_copy(
    __global half *dst, const __local half *src, size_t row_size,
    size_t num_rows, size_t dst_pitch, size_t src_pitch, event_t event);

event_t __OVERLOADABLE__ xt_async_work_group_2d_copy(
    __local double *dst, const __global double *src, size_t row_size,
    size_t num_rows, size_t dst_pitch, size_t src_pitch, event_t event);
event_t __OVERLOADABLE__ xt_async_work_group_2d_copy(
    __global double *dst, const __local double *src, size_t row_size,
    size_t num_rows, size_t dst_pitch, size_t src_pitch, event_t event);

#if XCHAL_HAVE_CONNX_B20
event_t __OVERLOADABLE__ xt_async_work_group_2d_copy(
    __local cfloat *dst, const __global cfloat *src, size_t row_size,
    size_t num_rows, size_t dst_pitch, size_t src_pitch, event_t event);
event_t __OVERLOADABLE__ xt_async_work_group_2d_copy(
    __global cfloat *dst, const __local cfloat *src, size_t row_size,
    size_t num_rows, size_t dst_pitch, size_t src_pitch, event_t event);
#endif

#define MACRO_WORK_GROUP(gentype, n)                                           \
  event_t __OVERLOADABLE__ async_work_group_copy(                              \
      __local gentype##n *dst, const __global gentype##n *src,                 \
      size_t num_gentypes, event_t event);                                     \
  event_t __OVERLOADABLE__ async_work_group_copy(                              \
      __global gentype##n *dst, const __local gentype##n *src,                 \
      size_t num_gentypes, event_t event);                                     \
  event_t __OVERLOADABLE__ async_work_group_strided_copy(                      \
      __local gentype##n *dst, const __global gentype##n *src,                 \
      size_t num_gentypes, size_t src_stride, event_t event);                  \
  event_t __OVERLOADABLE__ async_work_group_strided_copy(                      \
      __global gentype##n *dst, const __local gentype##n *src,                 \
      size_t num_gentypes, size_t dst_stride, event_t event);                  \
  event_t __OVERLOADABLE__ xt_async_work_group_2d_copy(                        \
      __local gentype##n *dst, const __global gentype##n *src,                 \
      size_t row_size, size_t num_rows, size_t dst_pitch, size_t src_pitch,    \
      event_t event);                                                          \
  event_t __OVERLOADABLE__ xt_async_work_group_2d_copy(                        \
      __global gentype##n *dst, const __local gentype##n *src,                 \
      size_t row_size, size_t num_rows, size_t dst_pitch, size_t src_pitch,    \
      event_t event);                                                          \
  void __OVERLOADABLE__ prefetch(const __global gentype##n *p,                 \
                                 size_t num_gentypes);

#define MACRO_WORK_GROUP_TYPE(n)                                               \
  MACRO_WORK_GROUP(char, n)                                                    \
  MACRO_WORK_GROUP(uchar, n)                                                   \
  MACRO_WORK_GROUP(short, n)                                                   \
  MACRO_WORK_GROUP(ushort, n)                                                  \
  MACRO_WORK_GROUP(int, n)                                                     \
  MACRO_WORK_GROUP(uint, n)                                                    \
  MACRO_WORK_GROUP(long, n)                                                    \
  MACRO_WORK_GROUP(ulong, n)                                                   \
  MACRO_WORK_GROUP(float, n)                                                   \
  MACRO_WORK_GROUP(double, n)

#define MACRO_WORK_GROUP_DECLARE                                               \
  MACRO_WORK_GROUP_TYPE(2)                                                     \
  MACRO_WORK_GROUP_TYPE(3)                                                     \
  MACRO_WORK_GROUP_TYPE(4)                                                     \
  MACRO_WORK_GROUP_TYPE(8)                                                     \
  MACRO_WORK_GROUP_TYPE(16)                                                    \
  MACRO_WORK_GROUP_TYPE(32)

MACRO_WORK_GROUP_DECLARE

#if XCHAL_HAVE_CONNX_B20
MACRO_WORK_GROUP(cfloat, 16)
#endif

#undef MACRO_WORK_GROUP

#endif // __ASYNC_COPIES_FUN_H__
