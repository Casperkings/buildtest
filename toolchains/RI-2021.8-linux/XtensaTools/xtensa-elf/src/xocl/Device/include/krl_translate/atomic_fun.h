/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of
 * Cadence Design Systems Inc. Any rights to use, modify, and create
 * derivative works of this file are set forth under the terms of your
 * license agreement with Cadence Design Systems, Inc.
 */

#ifndef __ATOMIC_FUN_H__
#define __ATOMIC_FUN_H__

#define __OVERLOADABLE__ __attribute__((overloadable))

int __OVERLOADABLE__ atomic_add(volatile __global int *p, int val);
unsigned int __OVERLOADABLE__ atomic_add(volatile __global unsigned int *p,
                                         unsigned int val);
int __OVERLOADABLE__ atomic_add(volatile __local int *p, int val);
unsigned int __OVERLOADABLE__ atomic_add(volatile __local unsigned int *p,
                                         unsigned int val);

int __OVERLOADABLE__ atomic_sub(volatile __global int *p, int val);
unsigned int __OVERLOADABLE__ atomic_sub(volatile __global unsigned int *p,
                                         unsigned int val);
int __OVERLOADABLE__ atomic_sub(volatile __local int *p, int val);
unsigned int __OVERLOADABLE__ atomic_sub(volatile __local unsigned int *p,
                                         unsigned int val);

int __OVERLOADABLE__ atomic_xchg(volatile __global int *p, int val);
unsigned int __OVERLOADABLE__ atomic_xchg(volatile __global unsigned int *p,
                                          unsigned int val);
float __OVERLOADABLE__ atomic_xchg(volatile __global float *p, float val);
int __OVERLOADABLE__ atomic_xchg(volatile __local int *p, int val);
unsigned int __OVERLOADABLE__ atomic_xchg(volatile __local unsigned int *p,
                                          unsigned int val);
float __OVERLOADABLE__ atomic_xchg(volatile __local float *p, float val);

int __OVERLOADABLE__ atomic_inc(volatile __global int *p);
unsigned int __OVERLOADABLE__ atomic_inc(volatile __global unsigned int *p);
int __OVERLOADABLE__ atomic_inc(volatile __local int *p);
unsigned int __OVERLOADABLE__ atomic_inc(volatile __local unsigned int *p);

int __OVERLOADABLE__ atomic_dec(volatile __global int *p);
unsigned int __OVERLOADABLE__ atomic_dec(volatile __global unsigned int *p);
int __OVERLOADABLE__ atomic_dec(volatile __local int *p);
unsigned int __OVERLOADABLE__ atomic_dec(volatile __local unsigned int *p);

int __OVERLOADABLE__ atomic_cmpxchg(volatile __global int *p, int cmp, int val);
unsigned int __OVERLOADABLE__ atomic_cmpxchg(volatile __global unsigned int *p,
                                             unsigned int cmp,
                                             unsigned int val);
int __OVERLOADABLE__ atomic_cmpxchg(volatile __local int *p, int cmp, int val);
unsigned int __OVERLOADABLE__ atomic_cmpxchg(volatile __local unsigned int *p,
                                             unsigned int cmp,
                                             unsigned int val);

int __OVERLOADABLE__ atomic_min(volatile __global int *p, int val);
unsigned int __OVERLOADABLE__ atomic_min(volatile __global unsigned int *p,
                                         unsigned int val);
int __OVERLOADABLE__ atomic_min(volatile __local int *p, int val);
unsigned int __OVERLOADABLE__ atomic_min(volatile __local unsigned int *p,
                                         unsigned int val);

int __OVERLOADABLE__ atomic_max(volatile __global int *p, int val);
unsigned int __OVERLOADABLE__ atomic_max(volatile __global unsigned int *p,
                                         unsigned int val);
int __OVERLOADABLE__ atomic_max(volatile __local int *p, int val);
unsigned int __OVERLOADABLE__ atomic_max(volatile __local unsigned int *p,
                                         unsigned int val);

int __OVERLOADABLE__ atomic_and(volatile __global int *p, int val);
unsigned int __OVERLOADABLE__ atomic_and(volatile __global unsigned int *p,
                                         unsigned int val);
int __OVERLOADABLE__ atomic_and(volatile __local int *p, int val);
unsigned int __OVERLOADABLE__ atomic_and(volatile __local unsigned int *p,
                                         unsigned int val);

int __OVERLOADABLE__ atomic_or(volatile __global int *p, int val);
unsigned int __OVERLOADABLE__ atomic_or(volatile __global unsigned int *p,
                                        unsigned int val);
int __OVERLOADABLE__ atomic_or(volatile __local int *p, int val);
unsigned int __OVERLOADABLE__ atomic_or(volatile __local unsigned int *p,
                                        unsigned int val);

int __OVERLOADABLE__ atomic_xor(volatile __global int *p, int val);
unsigned int __OVERLOADABLE__ atomic_xor(volatile __global unsigned int *p,
                                         unsigned int val);
int __OVERLOADABLE__ atomic_xor(volatile __local int *p, int val);
unsigned int __OVERLOADABLE__ atomic_xor(volatile __local unsigned int *p,
                                         unsigned int val);

int __OVERLOADABLE__ atom_add(volatile __global int *p, int val);
unsigned int __OVERLOADABLE__ atom_add(volatile __global unsigned int *p,
                                       unsigned int val);
int __OVERLOADABLE__ atom_add(volatile __local int *p, int val);
unsigned int __OVERLOADABLE__ atom_add(volatile __local unsigned int *p,
                                       unsigned int val);

int __OVERLOADABLE__ atom_sub(volatile __global int *p, int val);
unsigned int __OVERLOADABLE__ atom_sub(volatile __global unsigned int *p,
                                       unsigned int val);
int __OVERLOADABLE__ atom_sub(volatile __local int *p, int val);
unsigned int __OVERLOADABLE__ atom_sub(volatile __local unsigned int *p,
                                       unsigned int val);

int __OVERLOADABLE__ atom_xchg(volatile __global int *p, int val);
unsigned int __OVERLOADABLE__ atom_xchg(volatile __global unsigned int *p,
                                        unsigned int val);
float __OVERLOADABLE__ atom_xchg(volatile __global float *p, float val);
int __OVERLOADABLE__ atom_xchg(volatile __local int *p, int val);
unsigned int __OVERLOADABLE__ atom_xchg(volatile __local unsigned int *p,
                                        unsigned int val);
float __OVERLOADABLE__ atom_xchg(volatile __local float *p, float val);

int __OVERLOADABLE__ atom_inc(volatile __global int *p);
unsigned int __OVERLOADABLE__ atom_inc(volatile __global unsigned int *p);
int __OVERLOADABLE__ atom_inc(volatile __local int *p);
unsigned int __OVERLOADABLE__ atom_inc(volatile __local unsigned int *p);

int __OVERLOADABLE__ atom_dec(volatile __global int *p);
unsigned int __OVERLOADABLE__ atom_dec(volatile __global unsigned int *p);
int __OVERLOADABLE__ atom_dec(volatile __local int *p);
unsigned int __OVERLOADABLE__ atom_dec(volatile __local unsigned int *p);

int __OVERLOADABLE__ atom_cmpxchg(volatile __global int *p, int cmp, int val);
unsigned int __OVERLOADABLE__ atom_cmpxchg(volatile __global unsigned int *p,
                                           unsigned int cmp, unsigned int val);
int __OVERLOADABLE__ atom_cmpxchg(volatile __local int *p, int cmp, int val);
unsigned int __OVERLOADABLE__ atom_cmpxchg(volatile __local unsigned int *p,
                                           unsigned int cmp, unsigned int val);

int __OVERLOADABLE__ atom_min(volatile __global int *p, int val);
unsigned int __OVERLOADABLE__ atom_min(volatile __global unsigned int *p,
                                       unsigned int val);
int __OVERLOADABLE__ atom_min(volatile __local int *p, int val);
unsigned int __OVERLOADABLE__ atom_min(volatile __local unsigned int *p,
                                       unsigned int val);

int __OVERLOADABLE__ atom_max(volatile __global int *p, int val);
unsigned int __OVERLOADABLE__ atom_max(volatile __global unsigned int *p,
                                       unsigned int val);
int __OVERLOADABLE__ atom_max(volatile __local int *p, int val);
unsigned int __OVERLOADABLE__ atom_max(volatile __local unsigned int *p,
                                       unsigned int val);

int __OVERLOADABLE__ atom_and(volatile __global int *p, int val);
unsigned int __OVERLOADABLE__ atom_and(volatile __global unsigned int *p,
                                       unsigned int val);
int __OVERLOADABLE__ atom_and(volatile __local int *p, int val);
unsigned int __OVERLOADABLE__ atom_and(volatile __local unsigned int *p,
                                       unsigned int val);

int __OVERLOADABLE__ atom_or(volatile __global int *p, int val);
unsigned int __OVERLOADABLE__ atom_or(volatile __global unsigned int *p,
                                      unsigned int val);
int __OVERLOADABLE__ atom_or(volatile __local int *p, int val);
unsigned int __OVERLOADABLE__ atom_or(volatile __local unsigned int *p,
                                      unsigned int val);

int __OVERLOADABLE__ atom_xor(volatile __global int *p, int val);
unsigned int __OVERLOADABLE__ atom_xor(volatile __global unsigned int *p,
                                       unsigned int val);
int __OVERLOADABLE__ atom_xor(volatile __local int *p, int val);
unsigned int __OVERLOADABLE__ atom_xor(volatile __local unsigned int *p,
                                       unsigned int val);

long __OVERLOADABLE__ atom_add(volatile __global long *p, long val);
long __OVERLOADABLE__ atom_add(volatile __local long *p, long val);
ulong __OVERLOADABLE__ atom_add(volatile __global ulong *p, ulong val);
ulong __OVERLOADABLE__ atom_add(volatile __local ulong *p, ulong val);

long __OVERLOADABLE__ atom_sub(volatile __global long *p, long val);
long __OVERLOADABLE__ atom_sub(volatile __local long *p, long val);
ulong __OVERLOADABLE__ atom_sub(volatile __global ulong *p, ulong val);
ulong __OVERLOADABLE__ atom_sub(volatile __local ulong *p, ulong val);

long __OVERLOADABLE__ atom_xchg(volatile __global long *p, long val);
long __OVERLOADABLE__ atom_xchg(volatile __local long *p, long val);
ulong __OVERLOADABLE__ atom_xchg(volatile __global ulong *p, ulong val);
ulong __OVERLOADABLE__ atom_xchg(volatile __local ulong *p, ulong val);

long __OVERLOADABLE__ atom_dec(volatile __global long *p);
long __OVERLOADABLE__ atom_dec(volatile __local long *p);
ulong __OVERLOADABLE__ atom_dec(volatile __global ulong *p);
ulong __OVERLOADABLE__ atom_dec(volatile __local ulong *p);

long __OVERLOADABLE__ atom_inc(volatile __global long *p);
long __OVERLOADABLE__ atom_inc(volatile __local long *p);
ulong __OVERLOADABLE__ atom_inc(volatile __global ulong *p);
ulong __OVERLOADABLE__ atom_inc(volatile __local ulong *p);

long __OVERLOADABLE__ atom_cmpxchg(volatile __global long *p, long cmp,
                                   long val);
long __OVERLOADABLE__ atom_cmpxchg(volatile __local long *p, long cmp,
                                   long val);
ulong __OVERLOADABLE__ atom_cmpxchg(volatile __global ulong *p, ulong cmp,
                                    ulong val);
ulong __OVERLOADABLE__ atom_cmpxchg(volatile __local ulong *p, ulong cmp,
                                    ulong val);

long __OVERLOADABLE__ atom_min(volatile __global long *p, long val);
long __OVERLOADABLE__ atom_min(volatile __local long *p, long val);
ulong __OVERLOADABLE__ atom_min(volatile __global ulong *p, ulong val);
ulong __OVERLOADABLE__ atom_min(volatile __local ulong *p, ulong val);

long __OVERLOADABLE__ atom_max(volatile __global long *p, long val);
long __OVERLOADABLE__ atom_max(volatile __local long *p, long val);
ulong __OVERLOADABLE__ atom_max(volatile __global ulong *p, ulong val);
ulong __OVERLOADABLE__ atom_max(volatile __local ulong *p, ulong val);

long __OVERLOADABLE__ atom_and(volatile __global long *p, long val);
long __OVERLOADABLE__ atom_and(volatile __local long *p, long val);
ulong __OVERLOADABLE__ atom_and(volatile __global ulong *p, ulong val);
ulong __OVERLOADABLE__ atom_and(volatile __local ulong *p, ulong val);

long __OVERLOADABLE__ atom_or(volatile __global long *p, long val);
long __OVERLOADABLE__ atom_or(volatile __local long *p, long val);
ulong __OVERLOADABLE__ atom_or(volatile __global ulong *p, ulong val);
ulong __OVERLOADABLE__ atom_or(volatile __local ulong *p, ulong val);

long __OVERLOADABLE__ atom_xor(volatile __global long *p, long val);
long __OVERLOADABLE__ atom_xor(volatile __local long *p, long val);
ulong __OVERLOADABLE__ atom_xor(volatile __global ulong *p, ulong val);
ulong __OVERLOADABLE__ atom_xor(volatile __local ulong *p, ulong val);

#endif // __ATOMIC_FUN_H__
