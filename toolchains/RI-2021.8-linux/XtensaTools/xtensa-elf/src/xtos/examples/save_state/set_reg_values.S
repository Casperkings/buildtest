/*
 * Copyright (c) 2021 by Cadence Design Systems. ALL RIGHTS RESERVED.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <xtensa/coreasm.h>


    .text
    .align 1

    .type _set_reg_values, @function
    .global _set_reg_values
    .type _set_reg_values2, @function
    .global _set_reg_values2
    .align 4

reg_id_vals1:
    .word   0x00ffff00,0x10ffff01,0x20ffff02,0x30ffff03
    .word   0x40ffff04,0x50ffff05,0x60ffff06,0x70ffff07
    .word   0x80ffff08,0x90ffff09,0xa0ffff0a,0xb0ffff0b
    .word   0xc0ffff0c,0xd0ffff0d,0xe0ffff0e,0xf0ffff0f

reg_id_vals2:
    .word   0xff0000ff,0xff1001ff,0xff2002ff,0xff3003ff
    .word   0xff4004ff,0xff5005ff,0xff6006ff,0xff700fff
    .word   0xff8008ff,0xff9009ff,0xffa00aff,0xffb00bff
    .word   0xffc00cff,0xffd00dff,0xfff00eff,0xfff00fff

    .align 4
_set_reg_values2:
#if defined(__XTENSA_WINDOWED_ABI__)
    entry a1, 48
#elif defined(__XTENSA_CALL0_ABI__)
    addi a1, a1, -64
    s32i a0, a1, 0
    s32i a2, a1, 4
    s32i a8, a1, 8
    s32i a9, a1, 12
    s32i a10, a1, 16
    s32i a11, a1, 20
    s32i a12, a1, 24
    s32i a13, a1, 28
    s32i a14, a1, 32
    s32i a15, a1, 36
#endif
    j 1f
1:
    movi    a2, reg_id_vals2
    l32i    a8, a2, 32
    l32i    a9, a2, 36
    l32i    a10, a2, 40
    l32i    a11, a2, 44
    l32i    a12, a2, 48
    l32i    a13, a2, 52
    l32i    a14, a2, 56
    l32i    a15, a2, 60
    l32i    a2, a2, 8
    j 1f
1:
    ill     // trigger illegal instruction
#if defined(__XTENSA_WINDOWED_ABI__)
    retw
#elif defined(__XTENSA_CALL0_ABI__)
    l32i a0, a1, 0
    l32i a2, a1, 4
    l32i a8, a1, 8
    l32i a9, a1, 12
    l32i a10, a1, 16
    l32i a11, a1, 20
    l32i a12, a1, 24
    l32i a13, a1, 28
    l32i a14, a1, 32
    l32i a15, a1, 36
    addi a1, a1, 64
    ret
#endif


    .align 4
_set_reg_values:
#if defined(__XTENSA_WINDOWED_ABI__)
    entry a1, 48
#elif defined(__XTENSA_CALL0_ABI__)
    addi a1, a1, -64
    s32i a0, a1, 0
    s32i a2, a1, 4
    s32i a3, a1, 8
    s32i a4, a1, 12
    s32i a5, a1, 16
    s32i a6, a1, 20
    s32i a7, a1, 24
    s32i a8, a1, 28
    s32i a9, a1, 32
    s32i a10, a1, 36
    s32i a11, a1, 40
    s32i a12, a1, 44
    s32i a13, a1, 48
    s32i a14, a1, 52
    s32i a15, a1, 56
#endif
    j 1f
1:
    movi    a2, reg_id_vals1
    l32i    a3, a2, 12
    l32i    a4, a2, 16
    l32i    a5, a2, 20
    l32i    a6, a2, 24
    l32i    a7, a2, 28
    l32i    a8, a2, 32
    l32i    a9, a2, 36
    l32i    a10, a2, 40
    l32i    a11, a2, 44
    l32i    a12, a2, 48
    l32i    a13, a2, 52
    l32i    a14, a2, 56
    l32i    a15, a2, 60
    l32i    a2, a2, 8
#ifdef __XTENSA_WINDOWED_ABI__
    call8   _set_reg_values2
#else
    call0   _set_reg_values2
#endif

#if defined(__XTENSA_WINDOWED_ABI__)
    retw
#elif defined(__XTENSA_CALL0_ABI__)
    l32i a0, a1, 0
    l32i a2, a1, 4
    l32i a3, a1, 8
    l32i a4, a1, 12
    l32i a5, a1, 16
    l32i a6, a1, 20
    l32i a7, a1, 24
    l32i a8, a1, 28
    l32i a9, a1, 32
    l32i a10, a1, 36
    l32i a11, a1, 40
    l32i a12, a1, 44
    l32i a13, a1, 48
    l32i a14, a1, 52
    l32i a15, a1, 56
    addi a1, a1, 64
    ret
#endif

