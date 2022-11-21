/* Assembly functions for the Xtensa version of libgcc1.
   Copyright (C) 2005-2016 Tensilica, Inc.
   Copyright (C) 2001, 2002, 2003, 2005, 2006, 2007
   Free Software Foundation, Inc.
   Contributed by Bob Wilson (bwilson@tensilica.com) and others at Tensilica.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

In addition to the permissions in the GNU General Public License, the
Free Software Foundation gives you unlimited permission to link the
compiled version of this file into combinations with other programs,
and to distribute those combinations without any restriction coming
from the use of this file.  (The General Public License restrictions
do apply in other respects; for example, they cover modification of
the file, and distribution when not linked into a combine
executable.)

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING.  If not, write to the Free
Software Foundation, 51 Franklin Street, Fifth Floor, Boston, MA
02110-1301, USA.  */

#include <xtensa/coreasm.h>

#if XCHAL_HAVE_XEA5
	// FIXME
	.align	4
	.global	__umulsidi3
	.type	__umulsidi3, @function
__umulsidi3:
	mul  a0, a1, a0
	srai a1, a0, 31
	ret

#else

/* Define macros for the ABS and ADDX* instructions to handle cases
   where they are not included in the Xtensa processor configuration.  */

	.macro	do_abs dst, src, tmp
#if XCHAL_HAVE_ABS
	abs	\dst, \src
#else
	neg	\tmp, \src
	movgez	\tmp, \src, \src
	mov	\dst, \tmp
#endif
	.endm

	.macro	do_addx2 dst, as, at, tmp
#if XCHAL_HAVE_ADDX
	addx2	\dst, \as, \at
#else
	slli	\tmp, \as, 1
	add	\dst, \tmp, \at
#endif
	.endm

	.macro	do_addx4 dst, as, at, tmp
#if XCHAL_HAVE_ADDX
	addx4	\dst, \as, \at
#else
	slli	\tmp, \as, 2
	add	\dst, \tmp, \at
#endif
	.endm

	.macro	do_addx8 dst, as, at, tmp
#if XCHAL_HAVE_ADDX
	addx8	\dst, \as, \at
#else
	slli	\tmp, \as, 3
	add	\dst, \tmp, \at
#endif
	.endm

/* Define macros for leaf function entry and return, supporting either the
   standard register windowed ABI or the non-windowed call0 ABI.  These
   macros do not allocate any extra stack space, so they only work for
   leaf functions that do not need to spill anything to the stack.  */

	.macro leaf_entry reg, size
#if XCHAL_HAVE_WINDOWED && !__XTENSA_CALL0_ABI__
# if XCHAL_HAVE_XEA3
	entry \reg, \size + 16
# else
	entry \reg, \size
# endif
#else
	/* do nothing */
#endif
	.endm

	.macro leaf_return
#if XCHAL_HAVE_WINDOWED && !__XTENSA_CALL0_ABI__
	retw
#else
	ret
#endif
	.endm


#ifdef L_mulsi3
	.align	4
	.global	__mulsi3
	.type	__mulsi3, @function
__mulsi3:
	leaf_entry sp, 16

#if XCHAL_HAVE_MUL32
	mull	a2, a2, a3

#elif XCHAL_HAVE_MUL16
	or	a4, a2, a3
	srai	a4, a4, 16
	bnez	a4, .LMUL16
	mul16u	a2, a2, a3
	leaf_return
.LMUL16:
	srai	a4, a2, 16
	srai	a5, a3, 16
	mul16u	a7, a4, a3
	mul16u	a6, a5, a2
	mul16u	a4, a2, a3
	add	a7, a7, a6
	slli	a7, a7, 16
	add	a2, a7, a4

#elif XCHAL_HAVE_MAC16
	mul.aa.hl a2, a3
	mula.aa.lh a2, a3
	rsr	a5, ACCLO
	umul.aa.ll a2, a3
	rsr	a4, ACCLO
	slli	a5, a5, 16
	add	a2, a4, a5

#else /* !MUL32 && !MUL16 && !MAC16 */

	/* Multiply one bit at a time, but unroll the loop 4x to better
	   exploit the addx instructions and avoid overhead.
	   Peel the first iteration to save a cycle on init.  */

	/* Avoid negative numbers.  */
	xor	a5, a2, a3	/* Top bit is 1 if one input is negative.  */
	do_abs	a3, a3, a6
	do_abs	a2, a2, a6

	/* Swap so the second argument is smaller.  */
	sub	a7, a2, a3
	mov	a4, a3
	movgez	a4, a2, a7	/* a4 = max (a2, a3) */
	movltz	a3, a2, a7	/* a3 = min (a2, a3) */

	movi	a2, 0
	extui	a6, a3, 0, 1
	movnez	a2, a4, a6

	do_addx2 a7, a4, a2, a7
	extui	a6, a3, 1, 1
	movnez	a2, a7, a6

	do_addx4 a7, a4, a2, a7
	extui	a6, a3, 2, 1
	movnez	a2, a7, a6

	do_addx8 a7, a4, a2, a7
	extui	a6, a3, 3, 1
	movnez	a2, a7, a6

	bgeui	a3, 16, .Lmult_main_loop
	neg	a3, a2
	movltz	a2, a3, a5
	leaf_return

	.align	4
.Lmult_main_loop:
	srli	a3, a3, 4
	slli	a4, a4, 4

	add	a7, a4, a2
	extui	a6, a3, 0, 1
	movnez	a2, a7, a6

	do_addx2 a7, a4, a2, a7
	extui	a6, a3, 1, 1
	movnez	a2, a7, a6

	do_addx4 a7, a4, a2, a7
	extui	a6, a3, 2, 1
	movnez	a2, a7, a6

	do_addx8 a7, a4, a2, a7
	extui	a6, a3, 3, 1
	movnez	a2, a7, a6

	bgeui	a3, 16, .Lmult_main_loop

	neg	a3, a2
	movltz	a2, a3, a5

#endif /* !MUL32 && !MUL16 && !MAC16 */

	leaf_return
	.size	__mulsi3, . - __mulsi3

#endif /* L_mulsi3 */


#ifdef L_mulsi3hifi2
	/* This routine should not be invoked (and linked into the
	   final executable) if HiFi2 is not configured.  */
#if XCHAL_HAVE_HIFI2
	.align	4
	.global	__mulsi3hifi2
	.type	__mulsi3hifi2, @function
__mulsi3hifi2:
	leaf_entry sp, 16

#if XCHAL_HAVE_MUL32
	mull	a2, a2, a3
#else
	ae_cvtp24a16x2.hl	aep7, a3, a3
	ae_cvtq48a32s		aeq2, a2
	ae_mulq32sp16s.h	aeq3, aeq2, aep7
	ae_mulq32sp16u.l	aeq2, aeq2, aep7
	ae_slliq56		aeq3, aeq3, 16
	ae_addq56		aeq3, aeq3, aeq2
	ae_slliq56		aeq3, aeq3, 16
	ae_trunca32q48		a2, aeq3
#endif

	leaf_return
	.size	__mulsi3hifi2, . - __mulsi3hifi2

#endif /* XCHAL_HAVE_HIFI2 */
#endif /* L_mulsi3hifi2 */


#ifdef L_umulsidi3

#if !XCHAL_HAVE_MUL16 && !XCHAL_HAVE_MUL32 && !XCHAL_HAVE_MAC16
#define XCHAL_NO_MUL 1
#endif

	.align	4
	.global	__umulsidi3
	.type	__umulsidi3, @function
__umulsidi3:
#if __XTENSA_CALL0_ABI__
	leaf_entry sp, 32
	addi	sp, sp, -32
	s32i	a12, sp, 16
	s32i	a13, sp, 20
	s32i	a14, sp, 24
	s32i	a15, sp, 28
#elif XCHAL_NO_MUL
	/* This is not really a leaf function; allocate enough stack space
	   to allow CALL12s to a helper function.  */
	leaf_entry sp, 48
#else
	leaf_entry sp, 16
#endif

#ifdef __XTENSA_EB__
#define wh a2
#define wl a3
#else
#define wh a3
#define wl a2
#endif /* __XTENSA_EB__ */

	/* This code is taken from the mulsf3 routine in ieee754-sf.S.
	   See more comments there.  */

#if XCHAL_HAVE_MUL32_HIGH
	mull	a6, a2, a3
	muluh	wh, a2, a3
	mov	wl, a6

#else /* ! MUL32_HIGH */

#if __XTENSA_CALL0_ABI__ && XCHAL_NO_MUL
	/* a0 and a8 will be clobbered by calling the multiply function
	   but a8 is not used here and need not be saved.  */
	s32i	a0, sp, 0
#endif

#if XCHAL_HAVE_MUL16 || XCHAL_HAVE_MUL32

#define a2h a4
#define a3h a5

	/* Get the high halves of the inputs into registers.  */
	srli	a2h, a2, 16
	srli	a3h, a3, 16

#define a2l a2
#define a3l a3

#if XCHAL_HAVE_MUL32 && !XCHAL_HAVE_MUL16
	/* Clear the high halves of the inputs.  This does not matter
	   for MUL16 because the high bits are ignored.  */
	extui	a2, a2, 0, 16
	extui	a3, a3, 0, 16
#endif
#endif /* MUL16 || MUL32 */


#if XCHAL_HAVE_MUL16

#define do_mul(dst, xreg, xhalf, yreg, yhalf) \
	mul16u	dst, xreg ## xhalf, yreg ## yhalf

#elif XCHAL_HAVE_MUL32

#define do_mul(dst, xreg, xhalf, yreg, yhalf) \
	mull	dst, xreg ## xhalf, yreg ## yhalf

#elif XCHAL_HAVE_MAC16

/* The preprocessor insists on inserting a space when concatenating after
   a period in the definition of do_mul below.  These macros are a workaround
   using underscores instead of periods when doing the concatenation.  */
#define umul_aa_ll umul.aa.ll
#define umul_aa_lh umul.aa.lh
#define umul_aa_hl umul.aa.hl
#define umul_aa_hh umul.aa.hh

#define do_mul(dst, xreg, xhalf, yreg, yhalf) \
	umul_aa_ ## xhalf ## yhalf	xreg, yreg; \
	rsr	dst, ACCLO

#else /* no multiply hardware */

#define set_arg_l(dst, src) \
	extui	dst, src, 0, 16
#define set_arg_h(dst, src) \
	srli	dst, src, 16

#if __XTENSA_CALL0_ABI__
#define do_mul(dst, xreg, xhalf, yreg, yhalf) \
	set_arg_ ## xhalf (a13, xreg); \
	set_arg_ ## yhalf (a14, yreg); \
	call0	__mul_mulsi3; \
	mov	dst, a12
#else
#if XCHAL_HAVE_XEA3
#define do_mul(dst, xreg, xhalf, yreg, yhalf) \
	set_arg_ ## xhalf (a10, xreg); \
	set_arg_ ## yhalf (a11, yreg); \
	call8	__mul_mulsi3; \
	mov	dst, a10
#else
#define do_mul(dst, xreg, xhalf, yreg, yhalf) \
	set_arg_ ## xhalf (a14, xreg); \
	set_arg_ ## yhalf (a15, yreg); \
	call12	__mul_mulsi3; \
	mov	dst, a14
#endif /* XCHAL_HAVE_XEA3 */
#endif /* __XTENSA_CALL0_ABI__ */

#endif /* no multiply hardware */

	/* Add pp1 and pp2 into a6 with carry-out in a9.  */
	do_mul(a6, a2, l, a3, h)	/* pp 1 */
	do_mul(a11, a2, h, a3, l)	/* pp 2 */
	movi	a9, 0
	add	a6, a6, a11
	bgeu	a6, a11, 1f
	addi	a9, a9, 1
1:
	/* Shift the high half of a9/a6 into position in a9.  Note that
	   this value can be safely incremented without any carry-outs.  */
	ssai	16
	src	a9, a9, a6

	/* Compute the low word into a6.  */
	do_mul(a11, a2, l, a3, l)	/* pp 0 */
	sll	a6, a6
	add	a6, a6, a11
	bgeu	a6, a11, 1f
	addi	a9, a9, 1
1:
	/* Compute the high word into wh.  */
	do_mul(wh, a2, h, a3, h)	/* pp 3 */
	add	wh, wh, a9
	mov	wl, a6

#endif /* !MUL32_HIGH */

#if __XTENSA_CALL0_ABI__ && XCHAL_NO_MUL
	/* Restore the original return address.  */
	l32i	a0, sp, 0
#endif
#if __XTENSA_CALL0_ABI__
	l32i	a12, sp, 16
	l32i	a13, sp, 20
	l32i	a14, sp, 24
	l32i	a15, sp, 28
	addi	sp, sp, 32
#endif
	leaf_return

#if XCHAL_NO_MUL

	/* For Xtensa processors with no multiply hardware, this simplified
	   version of _mulsi3 is used for multiplying 16-bit chunks of
	   the floating-point mantissas.  When using CALL0, this function
	   uses a custom ABI: the inputs are passed in a13 and a14, the
	   result is returned in a12, and a8 and a15 are clobbered.  */
	.align	4
	.local __mul_mulsi3
__mul_mulsi3:
	leaf_entry sp, 16
	.macro mul_mulsi3_body dst, src1, src2, tmp1, tmp2
	movi	\dst, 0
1:	add	\tmp1, \src2, \dst
	extui	\tmp2, \src1, 0, 1
	movnez	\dst, \tmp1, \tmp2

	do_addx2 \tmp1, \src2, \dst, \tmp1
	extui	\tmp2, \src1, 1, 1
	movnez	\dst, \tmp1, \tmp2

	do_addx4 \tmp1, \src2, \dst, \tmp1
	extui	\tmp2, \src1, 2, 1
	movnez	\dst, \tmp1, \tmp2

	do_addx8 \tmp1, \src2, \dst, \tmp1
	extui	\tmp2, \src1, 3, 1
	movnez	\dst, \tmp1, \tmp2

	srli	\src1, \src1, 4
	slli	\src2, \src2, 4
	bnez	\src1, 1b
	.endm
#if __XTENSA_CALL0_ABI__
	mul_mulsi3_body a12, a13, a14, a15, a8
#else
	/* The result will be written into a2, so save that argument in a4.  */
	mov	a4, a2
	mul_mulsi3_body a2, a4, a3, a5, a6
#endif
	leaf_return
#endif /* XCHAL_NO_MUL */

	.size	__umulsidi3, . - __umulsidi3

#endif /* L_umulsidi3 */


/* Define a macro for the NSAU (unsigned normalize shift amount)
   instruction, which computes the number of leading zero bits,
   to handle cases where it is not included in the Xtensa processor
   configuration.  */

	.macro	do_nsau cnt, val, tmp, a
#if XCHAL_HAVE_NSA
	nsau	\cnt, \val
#else
	mov	\a, \val
	movi	\cnt, 0
	extui	\tmp, \a, 16, 16
	bnez	\tmp, 0f
	movi	\cnt, 16
	slli	\a, \a, 16
0:
	extui	\tmp, \a, 24, 8
	bnez	\tmp, 1f
	addi	\cnt, \cnt, 8
	slli	\a, \a, 8
1:
	movi	\tmp, __nsau_data
	extui	\a, \a, 24, 8
	add	\tmp, \tmp, \a
	l8ui	\tmp, \tmp, 0
	add	\cnt, \cnt, \tmp
#endif /* !XCHAL_HAVE_NSA */
	.endm

#ifdef L_clz
	.section .rodata
	.align	4
	.global	__nsau_data
	.type	__nsau_data, @object
__nsau_data:
#if !XCHAL_HAVE_NSA
	.byte	8, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4
	.byte	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3
	.byte	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2
	.byte	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2
	.byte	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
	.byte	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
	.byte	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
	.byte	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
	.byte	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	.byte	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	.byte	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	.byte	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	.byte	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	.byte	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	.byte	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	.byte	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
#endif /* !XCHAL_HAVE_NSA */
	.size	__nsau_data, . - __nsau_data
	.hidden	__nsau_data
#endif /* L_clz */


#ifdef L_clzsi2
	.align	4
	.global	__clzsi2
	.type	__clzsi2, @function
__clzsi2:
	leaf_entry sp, 16
	do_nsau	a2, a2, a3, a4
	leaf_return
	.size	__clzsi2, . - __clzsi2

#endif /* L_clzsi2 */


#ifdef L_ctzsi2
	.align	4
	.global	__ctzsi2
	.type	__ctzsi2, @function
__ctzsi2:
	leaf_entry sp, 16
	neg	a3, a2
	and	a3, a3, a2
	do_nsau	a2, a3, a4, a5
	neg	a2, a2
	addi	a2, a2, 31
	leaf_return
	.size	__ctzsi2, . - __ctzsi2

#endif /* L_ctzsi2 */


#ifdef L_ffssi2
	.align	4
	.global	__ffssi2
	.type	__ffssi2, @function
__ffssi2:
	leaf_entry sp, 16
	neg	a3, a2
	and	a3, a3, a2
	do_nsau	a2, a3, a4, a5
	neg	a2, a2
	addi	a2, a2, 32
	leaf_return
	.size	__ffssi2, . - __ffssi2

#endif /* L_ffssi2 */


#ifdef L_udivsi3
	.align	4
	.global	__udivsi3
	.type	__udivsi3, @function
__udivsi3:
	leaf_entry sp, 16
#if XCHAL_HAVE_DIV32
	quou	a2, a2, a3
#else
	bltui	a3, 2, .Lle_one	/* check if the divisor <= 1 */

	mov	a6, a2		/* keep dividend in a6 */
	do_nsau	a5, a6, a2, a7	/* dividend_shift = nsau (dividend) */
	do_nsau	a4, a3, a2, a7	/* divisor_shift = nsau (divisor) */
	bgeu	a5, a4, .Lspecial

	sub	a4, a4, a5	/* count = divisor_shift - dividend_shift */
	ssl	a4
	sll	a3, a3		/* divisor <<= count */
	movi	a2, 0		/* quotient = 0 */

	/* test-subtract-and-shift loop; one quotient bit on each iteration */
#if XCHAL_HAVE_LOOPS
	loopnez	a4, .Lloopend
#endif /* XCHAL_HAVE_LOOPS */
.Lloop:
	bltu	a6, a3, .Lzerobit
	sub	a6, a6, a3
	addi	a2, a2, 1
.Lzerobit:
	slli	a2, a2, 1
	srli	a3, a3, 1
#if !XCHAL_HAVE_LOOPS
	addi	a4, a4, -1
	bnez	a4, .Lloop
#endif /* !XCHAL_HAVE_LOOPS */
.Lloopend:

	bltu	a6, a3, .Lreturn
	addi	a2, a2, 1	/* increment quotient if dividend >= divisor */
.Lreturn:
	leaf_return

.Lle_one:
	beqz	a3, .Lerror	/* if divisor == 1, return the dividend */
	leaf_return

.Lspecial:
	/* return dividend >= divisor */
	bltu	a6, a3, .Lreturn0
	movi	a2, 1
	leaf_return

.Lerror:
	/* Divide by zero: Use an illegal instruction to force an exception.
	   The subsequent "DIV0" string can be recognized by the exception
	   handler to identify the real cause of the exception.  */
	ill
	.ascii	"DIV0"

.Lreturn0:
	movi	a2, 0
#endif /* XCHAL_HAVE_DIV32 */
	leaf_return
	.size	__udivsi3, . - __udivsi3

#endif /* L_udivsi3 */


#ifdef L_divsi3
	.align	4
	.global	__divsi3
	.type	__divsi3, @function
__divsi3:
	leaf_entry sp, 16
#if XCHAL_HAVE_DIV32
	quos	a2, a2, a3
#else
	xor	a7, a2, a3	/* sign = dividend ^ divisor */
	do_abs	a6, a2, a4	/* udividend = abs (dividend) */
	do_abs	a3, a3, a4	/* udivisor = abs (divisor) */
	bltui	a3, 2, .Lle_one	/* check if udivisor <= 1 */
	/* Tensilica-only change: A result value of zero or +/-1 is handled
	   as a special case.  The standard version of this code detects the
	   special cases by checking if udividend_shift >= udivisor_shift,
	   but that misses some of them.  We can catch more cases by checking
	   (udividend >> 1) < udivisor.  This improves the performance for
	   those extra special cases, especially if NSAU is not available,
	   but lengthens the normal path by one cycle.  In general, it's a
	   toss up which is better, but this special case occurs frequently
	   for signed divisions in an important benchmark.  */
	srli	a5, a6, 1
	bltu	a5, a3, .Lspecial /* unsigned compare to handle abs(MININT) */
	do_nsau	a5, a6, a2, a8	/* udividend_shift = nsau (udividend) */
	do_nsau	a4, a3, a2, a8	/* udivisor_shift = nsau (udivisor) */
	/* bgeu    a5, a4, .Lspecial */

	sub	a4, a4, a5	/* count = udivisor_shift - udividend_shift */
	ssl	a4
	sll	a3, a3		/* udivisor <<= count */
	movi	a2, 0		/* quotient = 0 */

	/* test-subtract-and-shift loop; one quotient bit on each iteration */
#if XCHAL_HAVE_LOOPS
	loopnez	a4, .Lloopend
#endif /* XCHAL_HAVE_LOOPS */
.Lloop:
	bltu	a6, a3, .Lzerobit
	sub	a6, a6, a3
	addi	a2, a2, 1
.Lzerobit:
	slli	a2, a2, 1
	srli	a3, a3, 1
#if !XCHAL_HAVE_LOOPS
	addi	a4, a4, -1
	bnez	a4, .Lloop
#endif /* !XCHAL_HAVE_LOOPS */
.Lloopend:

	bltu	a6, a3, .Lreturn
	addi	a2, a2, 1	/* increment if udividend >= udivisor */
.Lreturn:
	neg	a5, a2
	movltz	a2, a5, a7	/* return (sign < 0) ? -quotient : quotient */
	leaf_return

.Lle_one:
	beqz	a3, .Lerror
	neg	a2, a6		/* if udivisor == 1, then return... */
	movgez	a2, a6, a7	/* (sign < 0) ? -udividend : udividend */
	leaf_return

.Lspecial:
	bltu	a6, a3, .Lreturn0 /* if dividend < divisor, return 0 */
	movi	a2, 1
	movi	a4, -1
	movltz	a2, a4, a7	/* else return (sign < 0) ? -1 : 1 */
	leaf_return

.Lerror:
	/* Divide by zero: Use an illegal instruction to force an exception.
	   The subsequent "DIV0" string can be recognized by the exception
	   handler to identify the real cause of the exception.  */
	ill
	.ascii	"DIV0"

.Lreturn0:
	movi	a2, 0
#endif /* XCHAL_HAVE_DIV32 */
	leaf_return
	.size	__divsi3, . - __divsi3

#endif /* L_divsi3 */


#ifdef L_umodsi3
	.align	4
	.global	__umodsi3
	.type	__umodsi3, @function
__umodsi3:
	leaf_entry sp, 16
#if XCHAL_HAVE_DIV32
	remu	a2, a2, a3
#else
	bltui	a3, 2, .Lle_one	/* check if the divisor is <= 1 */

	do_nsau	a5, a2, a6, a7	/* dividend_shift = nsau (dividend) */
	do_nsau	a4, a3, a6, a7	/* divisor_shift = nsau (divisor) */
	bgeu	a5, a4, .Lspecial

	sub	a4, a4, a5	/* count = divisor_shift - dividend_shift */
	ssl	a4
	sll	a3, a3		/* divisor <<= count */

	/* test-subtract-and-shift loop */
#if XCHAL_HAVE_LOOPS
	loopnez	a4, .Lloopend
#endif /* XCHAL_HAVE_LOOPS */
.Lloop:
	bltu	a2, a3, .Lzerobit
	sub	a2, a2, a3
.Lzerobit:
	srli	a3, a3, 1
#if !XCHAL_HAVE_LOOPS
	addi	a4, a4, -1
	bnez	a4, .Lloop
#endif /* !XCHAL_HAVE_LOOPS */
.Lloopend:

.Lspecial:
	bltu	a2, a3, .Lreturn
	sub	a2, a2, a3	/* subtract once more if dividend >= divisor */
.Lreturn:
	leaf_return

.Lle_one:
	bnez	a3, .Lreturn0

	/* Divide by zero: Use an illegal instruction to force an exception.
	   The subsequent "DIV0" string can be recognized by the exception
	   handler to identify the real cause of the exception.  */
	ill
	.ascii	"DIV0"

.Lreturn0:
	movi	a2, 0
#endif /* XCHAL_HAVE_DIV32 */
	leaf_return
	.size	__umodsi3, . - __umodsi3

#endif /* L_umodsi3 */


#ifdef L_modsi3
	.align	4
	.global	__modsi3
	.type	__modsi3, @function
__modsi3:
	leaf_entry sp, 16
#if XCHAL_HAVE_DIV32
	rems	a2, a2, a3
#else
	mov	a7, a2		/* save original (signed) dividend */
	do_abs	a2, a2, a4	/* udividend = abs (dividend) */
	do_abs	a3, a3, a4	/* udivisor = abs (divisor) */
	bltui	a3, 2, .Lle_one	/* check if udivisor <= 1 */
	do_nsau	a5, a2, a6, a8	/* udividend_shift = nsau (udividend) */
	do_nsau	a4, a3, a6, a8	/* udivisor_shift = nsau (udivisor) */
	bgeu	a5, a4, .Lspecial

	sub	a4, a4, a5	/* count = udivisor_shift - udividend_shift */
	ssl	a4
	sll	a3, a3		/* udivisor <<= count */

	/* test-subtract-and-shift loop */
#if XCHAL_HAVE_LOOPS
	loopnez	a4, .Lloopend
#endif /* XCHAL_HAVE_LOOPS */
.Lloop:
	bltu	a2, a3, .Lzerobit
	sub	a2, a2, a3
.Lzerobit:
	srli	a3, a3, 1
#if !XCHAL_HAVE_LOOPS
	addi	a4, a4, -1
	bnez	a4, .Lloop
#endif /* !XCHAL_HAVE_LOOPS */
.Lloopend:

.Lspecial:
	bltu	a2, a3, .Lreturn
	sub	a2, a2, a3	/* subtract again if udividend >= udivisor */
.Lreturn:
	bgez	a7, .Lpositive
	neg	a2, a2		/* if (dividend < 0), return -udividend */
.Lpositive:
	leaf_return

.Lle_one:
	bnez	a3, .Lreturn0

	/* Divide by zero: Use an illegal instruction to force an exception.
	   The subsequent "DIV0" string can be recognized by the exception
	   handler to identify the real cause of the exception.  */
	ill
	.ascii	"DIV0"

.Lreturn0:
	movi	a2, 0
#endif /* XCHAL_HAVE_DIV32 */
	leaf_return
	.size	__modsi3, . - __modsi3

#endif /* L_modsi3 */


#ifdef __XTENSA_EB__
#define uh a2
#define ul a3
#else
#define uh a3
#define ul a2
#endif /* __XTENSA_EB__ */


#ifdef L_ashldi3
	.align	4
	.global	__ashldi3
	.type	__ashldi3, @function
__ashldi3:
	leaf_entry sp, 16
	ssl	a4
	bgei	a4, 32, .Llow_only
	src	uh, uh, ul
	sll	ul, ul
	leaf_return

.Llow_only:
	sll	uh, ul
	movi	ul, 0
	leaf_return
	.size	__ashldi3, . - __ashldi3

#endif /* L_ashldi3 */


#ifdef L_ashrdi3
	.align	4
	.global	__ashrdi3
	.type	__ashrdi3, @function
__ashrdi3:
	leaf_entry sp, 16
	ssr	a4
	bgei	a4, 32, .Lhigh_only
	src	ul, uh, ul
	sra	uh, uh
	leaf_return

.Lhigh_only:
	sra	ul, uh
	srai	uh, uh, 31
	leaf_return
	.size	__ashrdi3, . - __ashrdi3

#endif /* L_ashrdi3 */


#ifdef L_lshrdi3
	.align	4
	.global	__lshrdi3
	.type	__lshrdi3, @function
__lshrdi3:
	leaf_entry sp, 16
	ssr	a4
	bgei	a4, 32, .Lhigh_only1
	src	ul, uh, ul
	srl	uh, uh
	leaf_return

.Lhigh_only1:
	srl	ul, uh
	movi	uh, 0
	leaf_return
	.size	__lshrdi3, . - __lshrdi3

#endif /* L_lshrdi3 */


#include "ieee754-df.S"
#include "ieee754-sf.S"


#if XCHAL_HAVE_DFP_accel

// Signed 32 bit divide emulation routine
// The compiler will generate a call to this routine for
//    (int)a / (int)b
// 
// Because this routine modifies the F64S and F64R states,
// it should never be invoked from an interrupt service routine
// unless the routine properly saves and restores these states

// examples how to calculate integer division/moduo using F64ITER instruction

#ifdef L_divsi3_dfp_accel

#if !XCHAL_HAVE_DIV32

	.align	4
	.global	__divsi3_dfp_accel
	.type	__divsi3_dfp_accel,@function
__divsi3_dfp_accel:	
	leaf_entry   a1, 16
	xor	a7, a2, a3	// sign = dividend ^ divisor
	abs	a6, a2		// udividend = abs(dividend)
	abs	a3, a3		// udivisor = abs(divisor)
	bltui	a3, 2, .Lsdiv_le_one	// check if udivisor <= 1
	srli	a5, a6, 1
	bltu	a5, a3, .Lsdiv_special
	movi	a2, 0		// quotient = 0
	f64norm a5, a2, a6, 1	// divident shift
	f64norm a4, a2, a3, 1	// divisor shift

	sub	a4, a4, a5	// count = udivisor_shift - udividend_shift
	ssl	a4
	sll	a3, a3		// udivisor <<= count
	
        wur.f64s a2        // clear result collection reg
        wf64r    a6,a2,0   // write actual divident to high part, low part 0
	f64iter  a3, a3,a2, DIVI, NO_SHIFT  // prepare first iteration differently
	
	// test-subtract-and-shift loop; one quotient bit on each iteration
#if XCHAL_HAVE_LOOPS
	loopnez	a4, .Lsdiv_loopend
.Lsdiv_loop:
	f64iter a3, a3, a2, DIVI, DO_SHIFT
#else
	beqz a4, .Lsdiv_loopend			
.Lsdiv_loopbeg:
	f64iter	a3, a3, a2, DIVI, DO_SHIFT	// note: a2 must be 0
	addi a4,a4,-1
	bnez a4, .Lsdiv_loopbeg			
#endif
.Lsdiv_loopend:
	
	rur.f64s a2
.Lsdiv_return:
	neg	a5, a2
	movltz	a2, a5, a7	// return (sign < 0) ? -quotient : quotient
	leaf_return

.Lsdiv_special:
	bltu	a6, a3, .Lsdiv_return2 //  if dividend < divisor, return 0
	movi	a2, 1
	movi	a4, -1
	movltz	a2, a4, a7	// else return (sign < 0) ? -1 : 1 
	leaf_return
.Lsdiv_return2:
	movi	a2, 0
	leaf_return

.Lsdiv_le_one:
	beqz	a3, .Lsdiv_error
	neg	a2, a6		// if udivisor == 1, then return...
	movgez	a2, a6, a7	// (sign < 0) ? -udividend : udividend
	leaf_return
.Lsdiv_error:
	movi	a2, 0		// just return 0; could throw an exception
	leaf_return
	.size	__divsi3_dfp_accel, . - __divsi3_dfp_accel


#endif

#endif // L_divsi3_dfp_accel

#ifdef L_udivsi3_dfp_accel

#if !XCHAL_HAVE_DIV32
	.align	4
	.global	__udivsi3_dfp_accel
	.type	__udivsi3_dfp_accel,@function
__udivsi3_dfp_accel:
	leaf_entry   a1, 16
	bltui	a3, 2, .Ludiv_le_one	// check if the divisor <= 1
	bgeu	a3, a2, .Ludiv_special  // divisor >= divider, return 1 or 0
	mov	a6, a2			// keep dividend in a6
	movi	a2, 0	// quotient = 0
	
// using f64norm since it we are interested about relative difference of shift
// and f64norm is known to exist, while nsau is not certain
// note: keep the high part 0 (a2), since bits 31-21 not seeked in the high part
	f64norm a5, a2, a6, 1
	f64norm a4, a2, a3, 1

	sub	a4, a4, a5 // count = divisor_shift - dividend_shift
	ssl	a4
	sll	a3, a3	   // divisor <<= count, align divisor with divident

        wur.f64s a2        // clear result collection reg
        wf64r    a6,a2,0   // write actual divident to high part, low part 0
	f64iter  a3, a3,a2, DIVI, NO_SHIFT  // first iteration without shifting

	// test-subtract-and-shift loop; one quotient bit on each iteration
#if XCHAL_HAVE_LOOPS
	loopnez	a4, .Ludiv_loopend
	f64iter	a3, a3, a2, DIVI, DO_SHIFT	// note: a2 must be 0
#else
	beqz a4, .Ludiv_loopend			
.Ludiv_loopbeg:
	f64iter	a3, a3, a2, DIVI, DO_SHIFT	// note: a2 must be 0
	addi a4,a4,-1
	bnez a4, .Ludiv_loopbeg			
#endif
.Ludiv_loopend:
	rur.f64s a2
	leaf_return

.Ludiv_special:
// return 0 if dividend < divisor, return 1 if equal
	sub	a4, a2, a3
	movi	a2, 1
	movi	a3, 0
	movnez	a2, a3, a4
	leaf_return

.Ludiv_le_one:
	beqz	a3, .Ludiv_error	// if divisor == 1, return the dividend
	leaf_return
// division by 0	
.Ludiv_error:
	movi	a2, 0	// just return 0; could throw an exception
	leaf_return
	.size	__udivsi3_dfp_accel, . - __udivsi3_dfp_accel

#endif/* Unsigned 32 bit divide emulation routine */
#endif // L_udivsi3_dfp_accel

#ifdef L_modsi3_dfp_accel  

#if !XCHAL_HAVE_DIV32
// perform the function:	 
//  (a2 < 0) ? -__umodsi3_dfp_accel(abs(a2), abs(a3)) : __umodsi(abs(a2), abs(a3)) ;
// 
	.align	4
	.global	__modsi3_dfp_accel
	.type	__modsi3_dfp_accel,@function
__modsi3_dfp_accel:	
	leaf_entry   a1, 16
	mov	a7, a2
	abs	a2, a2
	abs	a3, a3
	bltui	a3, 2, .Lsimod_le_one	// check if the divisor is <= 1

	movi	 a6, 0
	f64norm  a5, a6, a2, 1
	f64norm  a4, a6, a3, 1
	
	bgeu	a5, a4, .Lsimod_special

	sub	a4, a4, a5	// count = divisor_shift - dividend_shift
	ssl	a4
	sll	a3, a3		// divisor <<= count

        wf64r    a2, a6, 0 // write actual divident to high part, low part 0
	f64iter  a3, a3, a6, DIVI, NO_SHIFT  // prepare first iteration differently

	// test-subtract-and-shift loop
#if XCHAL_HAVE_LOOPS
	loopnez	a4, .Lsimod_loopend
.Lsimod_loop:
	f64iter a3, a3, a6, DIVI, DO_SHIFT
#else
	mov a5,a4
	beqz a5, .Lsimod_loopend
.Lsimod_loopbeg:
	f64iter a3, a3, a6, DIVI, DO_SHIFT
	addi a5, a5, -1
	bnez a5, .Lsimod_loopbeg	
#endif
.Lsimod_loopend:
// shift remainder right as much divisor was shifted left
	ssr	a4
	rf64r	a2, 1
	srl	a2, a2
	
.Lsimod_return:
	neg	a3,a2
	movltz	a2,a3,a7
.Lsimod_end:	
	leaf_return

.Lsimod_special:
	bltu	a2, a3, .Lsimod_return
	sub	a2, a2, a3	// subtract once if dividend >= divisor
	j	.Lsimod_return

.Lsimod_le_one:
	// the divisor is either 0 or 1, so just return 0.
	// someday we may want to throw an exception if the divisor is 0.
	movi	a2, 0
	leaf_return
	.size	__modsi3_dfp_accel, . - __modsi3_dfp_accel



#endif

#endif  // L_modsi3_dfp_accel  
#ifdef L_umodsi3_dfp_accel  

#if !XCHAL_HAVE_DIV32

	.align	4
	.global	__umodsi3_dfp_accel
	.type	__umodsi3_dfp_accel,@function
__umodsi3_dfp_accel:	
	leaf_entry   a1, 16
	bltui	a3, 2, .Lumod_le_one	// check if the divisor is <= 1

	movi	 a6, 0
	f64norm  a5, a6, a2, 1
	f64norm  a4, a6, a3, 1
	
	bgeu	a5, a4, .Lumod_special

	sub	a4, a4, a5	// count = divisor_shift - dividend_shift
	ssl	a4
	sll	a3, a3		// divisor <<= count

        wf64r    a2, a6, 0 // write actual divident to high part, low part 0
	f64iter  a3, a3, a6, DIVI, NO_SHIFT  // prepare first iteration differently

	// test-subtract-and-shift loop
#if XCHAL_HAVE_LOOPS
	loopnez	a4, .Lumod_loopend
.Lumod_loop:
	f64iter a3, a3, a6, DIVI, DO_SHIFT
#else
	mov a5,a4
	beqz a5, .Lumod_loopend
.Lumod_loopbeg:
	f64iter a3, a3, a6, DIVI, DO_SHIFT
	addi a5, a5, -1
	bnez a5, .Lumod_loopbeg	
#endif
.Lumod_loopend:
// shift remainder right as much divisor was shifted left
	ssr	a4
	rf64r	a2, 1
	srl	a2, a2
.Lumod_return:
	leaf_return

.Lumod_special:
	bltu	a2, a3, .Lumod_return2
	sub	a2, a2, a3	// subtract once if dividend >= divisor
.Lumod_return2:
	leaf_return

.Lumod_le_one:
	// the divisor is either 0 or 1, so just return 0.
	// someday we may want to throw an exception if the divisor is 0.
	movi	a2, 0
	leaf_return
	.size	__umodsi3_dfp_accel, . - __umodsi3_dfp_accel


#endif
#endif  // L_umodsi3_dfp_accel  
#endif // XCHAL_HAVE_DFP_accel

#endif // XCHAL_HAVE_XEA5

