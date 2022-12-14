/* ANSI C standard library function strcmp.

   Copyright (c) 2001-20012 Tensilica Inc.

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
   CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.  */

#include "xtensa-asm.h"
#include <xtensa/tie/xt_virtualops.h>

#ifdef XT_ADDI_A
#define addi_a addi.a
#else
#define addi_a addi
#endif
#ifdef XT_ADDI_A_N
#define addi_a_n addi.a.n
#else
#define addi_a_n addi.n
#endif

#ifdef XT_ADD_A
#define add_a add.a
#else
#define add_a add
#endif


#define MASK4 0x40404040


	.section .text.strcmp
	.begin	schedule
	.align	4
	.literal_position

	.global	strcmp
	.type	strcmp,@function
	.align	4

#if defined(XT_CMP4) && defined(XT_BSWP)

strcmp:
.LBB1_strcmp:
	leaf_entry sp,32
	movi	a4,3
	or	a5,a2,a3
	movi	a8,0
	bany	a4,a5,.LBB3_strcmp
	l32i.n	a7,a3,0
	l32i	a6,a2,0
	j	.Lt_0_6402

.Lt_0_9986:
	l32i	a7,a3,0
	l32i	a6,a2,4
	addi_a	a2,a2,4

.Lt_0_6402:
	cmp4	a9,a6,a8
	bnez.n	a9,.Lt_0_1282
	addi_a	a3,a3,4
	beq	a6,a7,.Lt_0_9986
	bswp	a4,a7
	bswp	a2,a6
	saltu	a6,a4,a2
	saltu	a7,a2,a4
	sub	a2,a6,a7
	leaf_return

.Lt_0_1282:
	l8ui	a8,a2,0
	l8ui	a6,a3,0
	movi	a7,0
	beqz	a8,.LBB32_strcmp
	bne	a8,a6,.LBB32_strcmp
	mov.n	a6,a3
	addi	a7,a7,1
	addi	a6,a6,1
	addi_a	a2,a2,1

.L_0_8450:
	l8ui	a8,a2,0
	addi_a	a2,a2,1
	beqz	a8,.Lt_0_11778
	l8ui	a9,a6,0
	addi	a7,a7,1
	addi	a6,a6,1
	beq	a8,a9,.L_0_8450
	sub	a2,a8,a9
	leaf_return

.LBB32_strcmp:
	sub	a2,a8,a6
	leaf_return

.Lt_0_11778:
	add_a	a2,a7,a3
	l8ui	a2,a2,0
	sub	a2,a8,a2
	leaf_return

.LBB3_strcmp:
	l8ui	a8,a2,0
	l8ui	a6,a3,0
	movi	a7,0
	beqz	a8,.LBB31_strcmp
	bne	a8,a6,.LBB31_strcmp
	mov.n	a6,a3

.L_0_7426:
	addi_a	a2,a2,1
	l8ui	a8,a2,0
	addi	a7,a7,1
	addi	a6,a6,1
	beqz	a8,.Lt_0_11010
	l8ui	a9,a6,0
	beq	a8,a9,.L_0_7426
	sub	a2,a8,a9
	leaf_return

.LBB31_strcmp:
	sub	a2,a8,a6
	leaf_return

.Lt_0_11010:
	add_a	a2,a7,a3
	l8ui	a2,a2,0
	sub	a2,a8,a2
	leaf_return

.L_0_15362:

#else

strcmp:

#if XCHAL_HAVE_LOOPS && XCHAL_HAVE_DENSITY && __XTENSA_EL__ && XCHAL_HAVE_FLIX3
/*  Fast version for FLIX3 Little Endian */


	leaf_entry sp, 16
	/* a2 = s1, a3 = s2 */

	l8ui	a8, a2, 0	// byte 0 from s1
	l8ui	a9, a3, 0	// byte 0 from s2
	movi	a10, 3		// mask
	movi	a5, 0xfffffffc
	or	a11, a2, a3
	movi	a4, MASK0	// mask for byte 0
	movi	a7, MASK4
	addi_a    a3, a3, -8
	addi_a    a2, a2, -8
	and	a5, a5, a2
	bne.w18	a8, a9, .Lretdiff
	l32i	a8, a5, 8	// get word from aligned variant of s1

	bany.w18	a11, a10, .Lnot_aligned

/* s1 is word-aligned; s2 is word-aligned.

   If the zero-overhead loop option is available, use an (almost)
   infinite zero-overhead loop with conditional exits so we only pay
   for taken branches when exiting the loop.  */

/* New algorithm, relying on the fact that all normal ASCII is between
   32 and 127.

   Rather than check all bytes for zero:
   Take one word (4 bytes).  Call it w1.
   Shift w1 left by one into w1'.
   Or w1 and w1'.  For all normal ASCII bit 6 will be 1; for zero it won't.
   Check that all 4 bit 6's (one for each byte) are one:
   If they are, we are definitely not done.
   If they are not, we are probably done, but need to check for zero.  */

.Laligned:
	/* Loop forever */
1:	
	loop	a0, .Laligned_done

	/* First unrolled loop body.  */
	l32i	a9, a3, 8	// get word from s2
	addi_a	a3, a3, 8	// advance s2 pointer
	slli	a5, a8, 1
	or	a10, a8, a5
	{l32i	a11, a2, 12	// get word from s1+4
	bne.w18	a8, a9, .Lwne2}
	l32i	a9, a3, 4	// get word from s2+4
	bnall.w18	a10, a7, .Lprobeq

	/* Second unrolled loop body.  */
	slli	a5, a11, 1
	or	a10, a11, a5
	addi_a	a2, a2, 8	// advance s1 pointer
        mov	a8, a11
	bne.w18	a11, a9, .Lwne2
	l32i	a8, a2, 8	// get word from s1
	bnall.w18	a10, a7, .Lprobeq2

.Laligned_done:
	l32i	a8, a2, 8	// get word from s1
	j     	1b

.Lnot_aligned:
	xor	a11, a2, a3	// compare low two bits of s1 and s2
	bany	a11, a10, .Lunaligned	// if they have different alignment

	/* s1/s2 are not word-aligned.  */
	movi	a5, 0xfffffffc
	addi_a	a2, a2, 1	// advance s1
	beqz	a9, .Leq	// bytes equal, if zero, strings are equal
	addi_a	a3, a3, 1	// advance s2
	and     a6, a2, a5
	l32i	a8, a6, 8	// get word from s1
	bnone	a2, a10, .Laligned // if s1/s2 now aligned
	l8ui	a8, a2, 8	// byte 1 from s1
	l8ui	a9, a3, 8	// byte 1 from s2
	addi_a	a2, a2, 1	// advance s1
	bne	a8, a9, .Lretdiff // if different, return difference
	beqz	a8, .Leq	// bytes equal, if zero, strings are equal
	addi_a	a3, a3, 1	// advance s2
	and     a6, a2, a5
	l32i	a8, a6, 8	// get word from s1
	bnone	a2, a10, .Laligned // if s1/s2 now aligned
	l8ui	a8, a2, 8	// byte 2 from s1
	l8ui	a9, a3, 8	// byte 2 from s2
	addi_a	a2, a2, 1	// advance s1
	bne	a8, a9, .Lretdiff // if different, return difference
	beqz	a8, .Leq	// bytes equal, if zero, strings are equal
	addi_a	a3, a3, 1	// advance s2
	l32i	a8, a2, 8	// get word from s1
	j	.Laligned

/* s1 and s2 have different alignment.

   If the zero-overhead loop option is available, use an (almost)
   infinite zero-overhead loop with conditional exits so we only pay
   for taken branches when exiting the loop.

   Note: It is important for this unaligned case to come before the
   code for aligned strings, because otherwise some of the branches
   above cannot reach and have to be transformed to branches around
   jumps.  The unaligned code is smaller and the branches can reach
   over it.  */

.Lunaligned:
	movi.n	a8, 0		// set up for the maximum loop count
	loop	a8, .Lretdiff	// loop forever (almost anyway)
	l8ui	a8, a2, 8
	l8ui	a9, a3, 8
	addi_a	a2, a2, 1
	bne	a8, a9, .Lretdiff
	addi_a	a3, a3, 1
	beqz	a8, .Lretdiff
.Lretdiff:
	sub	a2, a8, a9
	leaf_return


.Lprobeq2:
	/* Adjust pointers to account for the loop unrolling.  */
        mov	a8, a11
	addi_a	a2, a2, -4
	addi_a	a3, a3, 4

	/* align (0 mod 4) */
.Lprobeq:
	/* Words are probably equal, but check for sure.
	   If not, loop over the rest of string using normal algorithm.  */

	bnone	a8, a4, .Leq	// if byte 0 is zero
	movi	a5, MASK1	// mask for byte 1
	movi	a6, MASK2	// mask for byte 2
	bnone	a8, a5, .Leq	// if byte 1 is zero
	movi	a7, MASK3	// mask for byte 3
	bnone	a8, a6, .Leq	// if byte 2 is zero
	bnone	a8, a7, .Leq	// if byte 3 is zero
	/* align (1 mod 4) */
	addi.n	a2, a2, 12	// advance s1 pointer
	addi.n	a3, a3, 4	// advance s2 pointer
	/* align (1 mod 4) or (2 mod 4) */
1:	
	loop	a0, .Lend	// loop forever (a4 is bigger than max iters)

	l32i	a8, a2, 0	// get word from s1
	l32i	a9, a3, 0	// get word from s2
	addi_a	a2, a2, 4	// advance s1 pointer
	bne	a8, a9, .Lwne
	bnone	a8, a4, .Leq	// if byte 0 is zero
	bnone	a8, a5, .Leq	// if byte 1 is zero
	bnone	a8, a6, .Leq	// if byte 2 is zero
	bnone	a8, a7, .Leq	// if byte 3 is zero
	addi_a	a3, a3, 4	// advance s2 pointer
.Lend:
	j	1b

	/* Words are equal; some byte is zero.  */
.Leq:	movi	a2, 0		// return equal
	leaf_return

.Lwne2:	/* Words are not equal.  On big-endian processors, if none of the
	   bytes are zero, the return value can be determined by a simple
	   comparison.  */
.Lwne:	/* Words are not equal.  */
	xor	a2, a8, a9	// get word with nonzero in byte that differs
	extui	a10, a8, 0, 8
	extui	a11, a9, 0, 8
	movi	a5, MASK1	// mask for byte 1
	bany.w18	a2, a4, .Ldiff0	// if byte 0 differs

	bnone.w18	a8, a4, .Leq	// if byte 0 is zero
	movi	a6, MASK2	// mask for byte 2
	bany.w18	a2, a5, .Ldiff1	// if byte 1 differs
	extui	a10, a8, 24, 8
	bnone.w18	a8, a5, .Leq	// if byte 1 is zero
	extui	a11, a9, 24, 8
	bany.w18	a2, a6, .Ldiff2	// if byte 2 differs
	sub	a2, a10, a11
	bnone.w18	a8, a6, .Leq	// if byte 2 is zero
	/* Little-endian is a little more difficult because can't subtract
	   whole words.  */
.Ldiff3:
	/* Bytes 0-2 are equal; byte 3 is different.
	   For little-endian need to have a sign bit for the difference.  */
	leaf_return
.Ldiff0:
	/* Byte 0 is different.  */
	sub	a2, a10, a11
	leaf_return

.Ldiff1:
	/* Byte 0 is equal; byte 1 is different.  */
	extui	a10, a8, 8, 8
	extui	a11, a9, 8, 8
	sub	a2, a10, a11
	leaf_return

.Ldiff2:
	/* Bytes 0-1 are equal; byte 2 is different.  */
	extui	a10, a8, 16, 8
	extui	a11, a9, 16, 8
	sub	a2, a10, a11
	leaf_return

#else 
#if XCHAL_HAVE_LOOPS && XCHAL_HAVE_DENSITY && __XTENSA_EL__ && XCHAL_HAVE_PDX4
/*  Fast version for FLIX3 Little Endian */


	leaf_entry sp, 16
	/* a2 = s1, a3 = s2 */

	l8ui	a8, a2, 0	// byte 0 from s1
	l8ui	a9, a3, 0	// byte 0 from s2
	movi	a10, 3		// mask
	movi	a5, 0xfffffffc
	or	a11, a2, a3
	movi	a4, MASK0	// mask for byte 0
	movi	a7, MASK4
	addi_a    a3, a3, -8
	addi_a    a2, a2, -8
	and	a5, a5, a2
	bne.w15	a8, a9, .Lretdiff
	l32i	a8, a5, 8	// get word from aligned variant of s1

	bany.w15	a11, a10, .Lnot_aligned

/* s1 is word-aligned; s2 is word-aligned.

   If the zero-overhead loop option is available, use an (almost)
   infinite zero-overhead loop with conditional exits so we only pay
   for taken branches when exiting the loop.  */

/* New algorithm, relying on the fact that all normal ASCII is between
   32 and 127.

   Rather than check all bytes for zero:
   Take one word (4 bytes).  Call it w1.
   Shift w1 left by one into w1'.
   Or w1 and w1'.  For all normal ASCII bit 6 will be 1; for zero it won't.
   Check that all 4 bit 6's (one for each byte) are one:
   If they are, we are definitely not done.
   If they are not, we are probably done, but need to check for zero.  */

.Laligned:
	/* Loop forever */
1:	
	loop	a0, .Laligned_done

	/* First unrolled loop body.  */
	l32i	a9, a3, 8	// get word from s2
	addi_a	a3, a3, 8	// advance s2 pointer
	slli	a5, a8, 1
	or	a10, a8, a5
	{
	bne.w15	a8, a9, .Lwne2
	l32i	a11, a2, 12	// get word from s1+4
	nop
	nop
	}
	l32i	a9, a3, 4	// get word from s2+4
	bnall.w15	a10, a7, .Lprobeq

	/* Second unrolled loop body.  */
	slli	a5, a11, 1
	or	a10, a11, a5
	addi_a	a2, a2, 8	// advance s1 pointer
        mov	a8, a11
	bne.w15	a11, a9, .Lwne2
	l32i	a8, a2, 8	// get word from s1
	bnall.w15	a10, a7, .Lprobeq2

.Laligned_done:
	l32i	a8, a2, 8	// get word from s1
	j     	1b

.Lnot_aligned:
	xor	a11, a2, a3	// compare low two bits of s1 and s2
	bany	a11, a10, .Lunaligned	// if they have different alignment

	/* s1/s2 are not word-aligned.  */
	movi	a5, 0xfffffffc
	addi_a	a2, a2, 1	// advance s1
	beqz	a9, .Leq	// bytes equal, if zero, strings are equal
	addi_a	a3, a3, 1	// advance s2
	and     a6, a2, a5
	l32i	a8, a6, 8	// get word from s1
	bnone	a2, a10, .Laligned // if s1/s2 now aligned
	l8ui	a8, a2, 8	// byte 1 from s1
	l8ui	a9, a3, 8	// byte 1 from s2
	addi_a	a2, a2, 1	// advance s1
	bne	a8, a9, .Lretdiff // if different, return difference
	beqz	a8, .Leq	// bytes equal, if zero, strings are equal
	addi_a	a3, a3, 1	// advance s2
	and     a6, a2, a5
	l32i	a8, a6, 8	// get word from s1
	bnone	a2, a10, .Laligned // if s1/s2 now aligned
	l8ui	a8, a2, 8	// byte 2 from s1
	l8ui	a9, a3, 8	// byte 2 from s2
	addi_a	a2, a2, 1	// advance s1
	bne	a8, a9, .Lretdiff // if different, return difference
	beqz	a8, .Leq	// bytes equal, if zero, strings are equal
	addi_a	a3, a3, 1	// advance s2
	l32i	a8, a2, 8	// get word from s1
	j	.Laligned

/* s1 and s2 have different alignment.

   If the zero-overhead loop option is available, use an (almost)
   infinite zero-overhead loop with conditional exits so we only pay
   for taken branches when exiting the loop.

   Note: It is important for this unaligned case to come before the
   code for aligned strings, because otherwise some of the branches
   above cannot reach and have to be transformed to branches around
   jumps.  The unaligned code is smaller and the branches can reach
   over it.  */

.Lunaligned:
	movi.n	a8, 0		// set up for the maximum loop count
	loop	a8, .Lretdiff	// loop forever (almost anyway)
	l8ui	a8, a2, 8
	l8ui	a9, a3, 8
	addi_a	a2, a2, 1
	bne	a8, a9, .Lretdiff
	addi_a	a3, a3, 1
	beqz	a8, .Lretdiff
.Lretdiff:
	sub	a2, a8, a9
	leaf_return


.Lprobeq2:
	/* Adjust pointers to account for the loop unrolling.  */
        mov	a8, a11
	addi_a	a2, a2, -4
	addi_a	a3, a3, 4

	/* align (0 mod 4) */
.Lprobeq:
	/* Words are probably equal, but check for sure.
	   If not, loop over the rest of string using normal algorithm.  */

	bnone	a8, a4, .Leq	// if byte 0 is zero
	movi	a5, MASK1	// mask for byte 1
	movi	a6, MASK2	// mask for byte 2
	bnone	a8, a5, .Leq	// if byte 1 is zero
	movi	a7, MASK3	// mask for byte 3
	bnone	a8, a6, .Leq	// if byte 2 is zero
	bnone	a8, a7, .Leq	// if byte 3 is zero
	/* align (1 mod 4) */
	addi_a_n	a2, a2, 12	// advance s1 pointer
	addi_a_n	a3, a3, 4	// advance s2 pointer
	/* align (1 mod 4) or (2 mod 4) */
1:	
	loop	a0, .Lend	// loop forever (a4 is bigger than max iters)

	l32i	a8, a2, 0	// get word from s1
	l32i	a9, a3, 0	// get word from s2
	addi_a	a2, a2, 4	// advance s1 pointer
	bne	a8, a9, .Lwne
	bnone	a8, a4, .Leq	// if byte 0 is zero
	bnone	a8, a5, .Leq	// if byte 1 is zero
	bnone	a8, a6, .Leq	// if byte 2 is zero
	bnone	a8, a7, .Leq	// if byte 3 is zero
	addi_a	a3, a3, 4	// advance s2 pointer
.Lend:
	j	1b

	/* Words are equal; some byte is zero.  */
.Leq:	movi	a2, 0		// return equal
	leaf_return

.Lwne2:	/* Words are not equal.  On big-endian processors, if none of the
	   bytes are zero, the return value can be determined by a simple
	   comparison.  */
.Lwne:	/* Words are not equal.  */
	xor	a2, a8, a9	// get word with nonzero in byte that differs
	extui	a10, a8, 0, 8
	extui	a11, a9, 0, 8
	movi	a5, MASK1	// mask for byte 1
	bany.w15	a2, a4, .Ldiff0	// if byte 0 differs

	bnone.w15	a8, a4, .Leq	// if byte 0 is zero
	movi	a6, MASK2	// mask for byte 2
	bany.w15	a2, a5, .Ldiff1	// if byte 1 differs
	extui	a10, a8, 24, 8
	bnone.w15	a8, a5, .Leq	// if byte 1 is zero
	extui	a11, a9, 24, 8
	bany.w15	a2, a6, .Ldiff2	// if byte 2 differs
	sub	a2, a10, a11
	bnone.w15	a8, a6, .Leq	// if byte 2 is zero
	/* Little-endian is a little more difficult because can't subtract
	   whole words.  */
.Ldiff3:
	/* Bytes 0-2 are equal; byte 3 is different.
	   For little-endian need to have a sign bit for the difference.  */
	leaf_return
.Ldiff0:
	/* Byte 0 is different.  */
	sub	a2, a10, a11
	leaf_return

.Ldiff1:
	/* Byte 0 is equal; byte 1 is different.  */
	extui	a10, a8, 8, 8
	extui	a11, a9, 8, 8
	sub	a2, a10, a11
	leaf_return

.Ldiff2:
	/* Bytes 0-1 are equal; byte 2 is different.  */
	extui	a10, a8, 16, 8
	extui	a11, a9, 16, 8
	sub	a2, a10, a11
	leaf_return


#else /* Not FLIX3 */
	leaf_entry sp, 16
	/* a2 = s1, a3 = s2 */

	l8ui	a8, a2, 0	// byte 0 from s1
	l8ui	a9, a3, 0	// byte 0 from s2
	movi	a10, 3		// mask
	bne	a8, a9, .Lretdiff

	or	a11, a2, a3
	bnone	a11, a10, .Laligned

	xor	a11, a2, a3	// compare low two bits of s1 and s2
	bany	a11, a10, .Lunaligned	// if they have different alignment

	/* s1/s2 are not word-aligned.  */
	addi_a	a2, a2, 1	// advance s1
	beqz	a8, .Leq	// bytes equal, if zero, strings are equal
	addi_a	a3, a3, 1	// advance s2
	bnone	a2, a10, .Laligned // if s1/s2 now aligned
	l8ui	a8, a2, 0	// byte 1 from s1
	l8ui	a9, a3, 0	// byte 1 from s2
	addi_a	a2, a2, 1	// advance s1
	bne	a8, a9, .Lretdiff // if different, return difference
	beqz	a8, .Leq	// bytes equal, if zero, strings are equal
	addi_a	a3, a3, 1	// advance s2
	bnone	a2, a10, .Laligned // if s1/s2 now aligned
	l8ui	a8, a2, 0	// byte 2 from s1
	l8ui	a9, a3, 0	// byte 2 from s2
	addi_a	a2, a2, 1	// advance s1
	bne	a8, a9, .Lretdiff // if different, return difference
	beqz	a8, .Leq	// bytes equal, if zero, strings are equal
	addi_a	a3, a3, 1	// advance s2
	j	.Laligned

/* s1 and s2 have different alignment.

   If the zero-overhead loop option is available, use an (almost)
   infinite zero-overhead loop with conditional exits so we only pay
   for taken branches when exiting the loop.

   Note: It is important for this unaligned case to come before the
   code for aligned strings, because otherwise some of the branches
   above cannot reach and have to be transformed to branches around
   jumps.  The unaligned code is smaller and the branches can reach
   over it.  */

	.align	4
#if XCHAL_HAVE_LOOPS
#if XCHAL_HAVE_DENSITY
	/* (2 mod 4) alignment for loop instruction */
#else
	/* (1 mod 4) alignment for loop instruction */
	.byte	0
	.byte	0
#endif
#endif
.Lunaligned:
#if XCHAL_HAVE_LOOPS
#if XCHAL_HAVE_DENSITY
	_movi.n	a8, 0		// set up for the maximum loop count
#else
	_movi	a8, 0		// set up for the maximum loop count
#endif
	loop	a8, .Lretdiff	// loop forever (almost anyway)
#endif
.Lnextbyte:
	l8ui	a8, a2, 0
	l8ui	a9, a3, 0
	addi_a	a2, a2, 1
	bne	a8, a9, .Lretdiff
	addi_a	a3, a3, 1
#if XCHAL_HAVE_LOOPS
	beqz	a8, .Lretdiff
#else
	bnez	a8, .Lnextbyte
#endif
.Lretdiff:
	sub	a2, a8, a9
	leaf_return

/* s1 is word-aligned; s2 is word-aligned.

   If the zero-overhead loop option is available, use an (almost)
   infinite zero-overhead loop with conditional exits so we only pay
   for taken branches when exiting the loop.  */

/* New algorithm, relying on the fact that all normal ASCII is between
   32 and 127.

   Rather than check all bytes for zero:
   Take one word (4 bytes).  Call it w1.
   Shift w1 left by one into w1'.
   Or w1 and w1'.  For all normal ASCII bit 6 will be 1; for zero it won't.
   Check that all 4 bit 6's (one for each byte) are one:
   If they are, we are definitely not done.
   If they are not, we are probably done, but need to check for zero.  */

	.align	4
#if XCHAL_HAVE_LOOPS
#if XCHAL_HAVE_CONST16
	/* (2 mod 4) alignment for loop instruction */
	.byte	0
#endif
.Laligned:
	movi	a4, MASK0	// mask for byte 0
	movi	a7, MASK4

	/* Loop forever */
1:	
	loop	a0, .Laligned_done

	/* First unrolled loop body.  */
	l32i	a8, a2, 0	// get word from s1
	l32i	a9, a3, 0	// get word from s2
	slli	a5, a8, 1
	bne	a8, a9, .Lwne2
	or	a9, a8, a5
	bnall	a9, a7, .Lprobeq

	/* Second unrolled loop body.  */
	l32i	a8, a2, 4	// get word from s1+4
	l32i	a9, a3, 4	// get word from s2+4
	slli	a5, a8, 1
	bne	a8, a9, .Lwne2
	or	a9, a8, a5
	bnall	a9, a7, .Lprobeq2

	addi_a	a2, a2, 8	// advance s1 pointer
	addi_a	a3, a3, 8	// advance s2 pointer
.Laligned_done:
	j     	1b

.Lprobeq2:
	/* Adjust pointers to account for the loop unrolling.  */
	addi_a	a2, a2, 4
	addi_a	a3, a3, 4

#else /* !XCHAL_HAVE_LOOPS */

.Laligned:
	movi	a4, MASK0	// mask for byte 0
	movi	a7, MASK4
	j	.Lfirstword
.Lnextword:
	addi_a	a2, a2, 4	// advance s1 pointer
	addi_a	a3, a3, 4	// advance s2 pointer
.Lfirstword:
	l32i	a8, a2, 0	// get word from s1
	l32i	a9, a3, 0	// get word from s2
	slli	a5, a8, 1
	bne	a8, a9, .Lwne2
	or	a9, a8, a5
	ball	a9, a7, .Lnextword
#endif /* !XCHAL_HAVE_LOOPS */

	/* align (0 mod 4) */
.Lprobeq:
	/* Words are probably equal, but check for sure.
	   If not, loop over the rest of string using normal algorithm.  */

	bnone	a8, a4, .Leq	// if byte 0 is zero
	movi	a5, MASK1	// mask for byte 1
	movi	a6, MASK2	// mask for byte 2
	bnone	a8, a5, .Leq	// if byte 1 is zero
	movi	a7, MASK3	// mask for byte 3
	bnone	a8, a6, .Leq	// if byte 2 is zero
	bnone	a8, a7, .Leq	// if byte 3 is zero
	/* align (1 mod 4) */
#if XCHAL_HAVE_DENSITY
	addi_a_n	a2, a2, 4	// advance s1 pointer
	addi_a_n	a3, a3, 4	// advance s2 pointer
	/* align (1 mod 4) or (2 mod 4) */
#else
	addi_a	a2, a2, 4	// advance s1 pointer
	addi_a	a3, a3, 4	// advance s2 pointer
	or	a1, a1, a1	// nop
#if XCHAL_HAVE_CONST16
	or	a1, a1, a1	// nop
#endif
	/* align (2 mod 4) */
#endif /* XCHAL_HAVE_DENSITY */
#if XCHAL_HAVE_LOOPS
1:	
	loop	a0, .Leq	// loop forever (a4 is bigger than max iters)
	l32i	a8, a2, 0	// get word from s1
	l32i	a9, a3, 0	// get word from s2
	addi_a	a2, a2, 4	// advance s1 pointer
	bne	a8, a9, .Lwne
	bnone	a8, a4, .Leq	// if byte 0 is zero
	bnone	a8, a5, .Leq	// if byte 1 is zero
	bnone	a8, a6, .Leq	// if byte 2 is zero
	bnone	a8, a7, .Leq	// if byte 3 is zero
	addi_a	a3, a3, 4	// advance s2 pointer
	j	1b
#else /* !XCHAL_HAVE_LOOPS */

	j	.Lfirstword2
.Lnextword2:
	addi_a	a3, a3, 4	// advance s2 pointer
.Lfirstword2:
	l32i	a8, a2, 0	// get word from s1
	l32i	a9, a3, 0	// get word from s2
	addi_a	a2, a2, 4	// advance s1 pointer
	bne	a8, a9, .Lwne
	bnone	a8, a4, .Leq	// if byte 0 is zero
	bnone	a8, a5, .Leq	// if byte 1 is zero
	bnone	a8, a6, .Leq	// if byte 2 is zero
	bany	a8, a7, .Lnextword2	// if byte 3 is zero
#endif /* !XCHAL_HAVE_LOOPS */

	/* Words are equal; some byte is zero.  */
.Leq:	movi	a2, 0		// return equal
	leaf_return

.Lwne2:	/* Words are not equal.  On big-endian processors, if none of the
	   bytes are zero, the return value can be determined by a simple
	   comparison.  */
#ifdef __XTENSA_EB__
	or	a10, a8, a5
	bnall	a10, a7, .Lsomezero
	bgeu	a8, a9, .Lposreturn
	movi	a2, -1
	leaf_return
.Lposreturn:
	movi	a2, 1
	leaf_return
.Lsomezero:	// There is probably some zero byte.
#endif /* __XTENSA_EB__ */
.Lwne:	/* Words are not equal.  */
	xor	a2, a8, a9	// get word with nonzero in byte that differs
	bany	a2, a4, .Ldiff0	// if byte 0 differs
	movi	a5, MASK1	// mask for byte 1
	bnone	a8, a4, .Leq	// if byte 0 is zero
	bany	a2, a5, .Ldiff1	// if byte 1 differs
	movi	a6, MASK2	// mask for byte 2
	bnone	a8, a5, .Leq	// if byte 1 is zero
	bany	a2, a6, .Ldiff2	// if byte 2 differs
	bnone	a8, a6, .Leq	// if byte 2 is zero
#ifdef __XTENSA_EB__
.Ldiff3:
.Ldiff2:
.Ldiff1:
	/* Byte 0 is equal (at least) and there is a difference before a zero
	   byte.  Just subtract words to get the return value.
	   The high order equal bytes cancel, leaving room for the sign.  */
	sub	a2, a8, a9
	leaf_return

.Ldiff0:
	/* Need to make room for the sign, so can't subtract whole words.  */
	extui	a10, a8, 24, 8
	extui	a11, a9, 24, 8
	sub	a2, a10, a11
	leaf_return

#else /* !__XTENSA_EB__ */
	/* Little-endian is a little more difficult because can't subtract
	   whole words.  */
.Ldiff3:
	/* Bytes 0-2 are equal; byte 3 is different.
	   For little-endian need to have a sign bit for the difference.  */
	extui	a10, a8, 24, 8
	extui	a11, a9, 24, 8
	sub	a2, a10, a11
	leaf_return

.Ldiff0:
	/* Byte 0 is different.  */
	extui	a10, a8, 0, 8
	extui	a11, a9, 0, 8
	sub	a2, a10, a11
	leaf_return

.Ldiff1:
	/* Byte 0 is equal; byte 1 is different.  */
	extui	a10, a8, 8, 8
	extui	a11, a9, 8, 8
	sub	a2, a10, a11
	leaf_return

.Ldiff2:
	/* Bytes 0-1 are equal; byte 2 is different.  */
	extui	a10, a8, 16, 8
	extui	a11, a9, 16, 8
	sub	a2, a10, a11
	leaf_return

#endif /* !__XTENSA_EB */
#endif /* FLIX3 */
#endif /* FLIX3 */
#endif /* XT_CMP4 && XT_BSWP */

	.end	schedule
	.size	strcmp, . - strcmp

