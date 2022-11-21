
/* Copyright (c) 2013 by Tensilica Inc.
   This file has been modified for use in the libgcc library for Xtensa
   processors.  These modifications are distributed under the same terms
   as the original SoftFloat code.  */

/*============================================================================

This C source fragment is part of the SoftFloat IEC/IEEE Floating-point
Arithmetic Package, Release 2b.

Written by John R. Hauser.  This work was made possible in part by the
International Computer Science Institute, located at Suite 600, 1947 Center
Street, Berkeley, California 94704.  Funding was partially provided by the
National Science Foundation under grant MIP-9311980.  The original version
of this code was written as part of a project to build a fixed-point vector
processor in collaboration with the University of California at Berkeley,
overseen by Profs. Nelson Morgan and John Wawrzynek.  More information
is available through the Web page `http://www.cs.berkeley.edu/~jhauser/
arithmetic/SoftFloat.html'.

THIS SOFTWARE IS DISTRIBUTED AS IS, FOR FREE.  Although reasonable effort has
been made to avoid it, THIS SOFTWARE MAY CONTAIN FAULTS THAT WILL AT TIMES
RESULT IN INCORRECT BEHAVIOR.  USE OF THIS SOFTWARE IS RESTRICTED TO PERSONS
AND ORGANIZATIONS WHO CAN AND WILL TAKE FULL RESPONSIBILITY FOR ALL LOSSES,
COSTS, OR OTHER PROBLEMS THEY INCUR DUE TO THE SOFTWARE, AND WHO FURTHERMORE
EFFECTIVELY INDEMNIFY JOHN HAUSER AND THE INTERNATIONAL COMPUTER SCIENCE
INSTITUTE (possibly via similar legal warning) AGAINST ALL LOSSES, COSTS, OR
OTHER PROBLEMS INCURRED BY THEIR CUSTOMERS AND CLIENTS DUE TO THE SOFTWARE.

Derivative works are acceptable, even for commercial purposes, so long as
(1) the source code for the derivative work includes prominent notice that
the work is derivative, and (2) the source code includes prominent notice with
these four paragraphs for those parts of this code that are retained.

=============================================================================*/

/*----------------------------------------------------------------------------
| Shifts `a' right by the number of bits given in `count'.  If any nonzero
| bits are shifted off, they are ``jammed'' into the least significant bit of
| the result by setting the least significant bit to 1.  The value of `count'
| can be arbitrarily large; in particular, if `count' is greater than 32, the
| result will be either 0 or 1, depending on whether `a' is zero or nonzero.
| The result is stored in the location pointed to by `zPtr'.
*----------------------------------------------------------------------------*/

INLINE void shift32RightJamming( bits32 a, int16 count, bits32 *zPtr )
{
    bits32 z;

#if defined(__XTENSA__) && !defined(__riscv)
    int lostBits;
    if ( count < 32 ) {
	XT_SSR (count);
	z = XT_SRL (a);
	lostBits = XT_SLL (a);
	z |= ( lostBits != 0 ? 1 : 0 );
    }
#else /* !__XTENSA__ */
    if ( count == 0 ) {
        z = a;
    }
    else if ( count < 32 ) {
        z = ( a>>count ) | ( ( a<<( ( - count ) & 31 ) ) != 0 ? 1 : 0 );
    }
#endif /* !__XTENSA__ */
    else {
        z = ( a != 0 ? 1 : 0 );
    }
    *zPtr = z;

}

/*----------------------------------------------------------------------------
| Shifts the 64-bit value formed by concatenating `a0' and `a1' right by the
| number of bits given in `count'.  Any bits shifted off are lost.  The value
| of `count' can be arbitrarily large; in particular, if `count' is greater
| than 64, the result will be 0.  The result is broken into two 32-bit pieces
| which are stored at the locations pointed to by `z0Ptr' and `z1Ptr'.
*----------------------------------------------------------------------------*/

INLINE void
 shift64Right(
     bits32 a0, bits32 a1, int16 count, bits32 *z0Ptr, bits32 *z1Ptr )
{
    bits32 z0, z1;

#if defined(__XTENSA__) && !defined(__riscv)
    XT_SSR (count);
    if ( count < 32 ) {
	z1 = XT_SRC (a0, a1);
	z0 = XT_SRL (a0);
    }
    else {
	z1 = 0;
        if ( count < 64 ) {
	    z1 = XT_SRL (a0);
	}
        z0 = 0;
    }
#else /* !XTENSA */
    int8 negCount = ( - count ) & 31;

    if ( count == 0 ) {
        z1 = a1;
        z0 = a0;
    }
    else if ( count < 32 ) {
        z1 = ( a0<<negCount ) | ( a1>>count );
        z0 = a0>>count;
    }
    else {
        z1 = ( count < 64 ) ? ( a0>>( count & 31 ) ) : 0;
        z0 = 0;
    }
#endif /* !XTENSA */
    *z1Ptr = z1;
    *z0Ptr = z0;

}

/*----------------------------------------------------------------------------
| Shifts the 64-bit value formed by concatenating `a0' and `a1' right by the
| number of bits given in `count'.  If any nonzero bits are shifted off, they
| are ``jammed'' into the least significant bit of the result by setting the
| least significant bit to 1.  The value of `count' can be arbitrarily large;
| in particular, if `count' is greater than 64, the result will be either 0
| or 1, depending on whether the concatenation of `a0' and `a1' is zero or
| nonzero.  The result is broken into two 32-bit pieces which are stored at
| the locations pointed to by `z0Ptr' and `z1Ptr'.
*----------------------------------------------------------------------------*/

INLINE void
 shift64RightJamming(
     bits32 a0, bits32 a1, int16 count, bits32 *z0Ptr, bits32 *z1Ptr )
{
    bits32 z0, z1;

#if defined(__XTENSA__) && !defined(__riscv)
    int lostBits;
    XT_SSR (count);
    if ( count < 32 ) {
	z1 = XT_SRC (a0, a1);
	lostBits = XT_SLL (a1);
	z1 |= ( lostBits != 0 ? 1 : 0 );
	z0 = XT_SRL (a0);
    }
    else {
	z1 = 0;
	lostBits = a0;
        if ( count < 64 ) {
	    z1 = XT_SRL (a0);
	    lostBits = XT_SLL (a0);
	}
	z1 |= ( (a1 | lostBits) != 0 ? 1 : 0 );
        z0 = 0;
    }
#else /* !__XTENSA__ */
    int8 negCount = ( - count ) & 31;

    if ( count == 0 ) {
        z1 = a1;
        z0 = a0;
    }
    else if ( count < 32 ) {
        z1 = ( ( a0<<negCount ) |
             ( a1>>count ) |
             ( ( a1<<negCount ) != 0 ? 1 : 0 ) );
        z0 = a0>>count;
    }
    else {
        if ( count == 32 ) {
            z1 = a0 | ( a1 != 0 ? 1 : 0 );
        }
        else if ( count < 64 ) {
            z1 = ( a0>>( count & 31 ) ) | ( ( ( a0<<negCount ) | a1 ) != 0 );
        }
        else {
            z1 = ( ( a0 | a1 ) != 0 );
        }
        z0 = 0;
    }
#endif /* !__XTENSA__ */
    *z1Ptr = z1;
    *z0Ptr = z0;

}

/*----------------------------------------------------------------------------
| Shifts the 96-bit value formed by concatenating `a0', `a1', and `a2' right
| by 32 _plus_ the number of bits given in `count'.  The shifted result is
| at most 64 nonzero bits; these are broken into two 32-bit pieces which are
| stored at the locations pointed to by `z0Ptr' and `z1Ptr'.  The bits shifted
| off form a third 32-bit result as follows:  The _last_ bit shifted off is
| the most-significant bit of the extra result, and the other 31 bits of the
| extra result are all zero if and only if _all_but_the_last_ bits shifted off
| were all zero.  This extra result is stored in the location pointed to by
| `z2Ptr'.  The value of `count' can be arbitrarily large.
|     (This routine makes more sense if `a0', `a1', and `a2' are considered
| to form a fixed-point value with binary point between `a1' and `a2'.  This
| fixed-point value is shifted right by the number of bits given in `count',
| and the integer part of the result is returned at the locations pointed to
| by `z0Ptr' and `z1Ptr'.  The fractional part of the result may be slightly
| corrupted as described above, and is returned at the location pointed to by
| `z2Ptr'.)
*----------------------------------------------------------------------------*/

INLINE void
 shift64ExtraRightJamming(
     bits32 a0,
     bits32 a1,
     bits32 a2,
     int16 count,
     bits32 *z0Ptr,
     bits32 *z1Ptr,
     bits32 *z2Ptr
 )
{
    bits32 z0, z1, z2;

#if defined(__XTENSA__) && !defined(__riscv)
    int lostBits;
    XT_SSR (count);
    if ( count < 32 ) {
	lostBits = XT_SLL (a2);
	z2 = XT_SRC (a1, a2);
	z1 = XT_SRC (a0, a1);
	z0 = XT_SRL (a0);
	z2 |= ( lostBits != 0 ? 1 : 0 );
    }
    else {
        if ( count < 64 ) {
	    lostBits = XT_SLL (a1);
	    z2 = XT_SRC (a0, a1);
	    z1 = XT_SRL (a0);
	    z2 |= ( ( lostBits | a2 ) != 0 ? 1 : 0 );
	}
	else {
	    if ( count < 96 ) {
		lostBits = XT_SLL (a0);
		z2 = XT_SRL (a0);
		z2 |= ( ( lostBits | a1 | a2 ) != 0 ? 1 : 0 );
	    }
	    else {
		z2 = ( ( a0 | a1 | a2 ) != 0 ? 1 : 0 );
	    }
	    z1 = 0;
	}
        z0 = 0;
    }
#else /* !__XTENSA__ */
    int8 negCount = ( - count ) & 31;

    if ( count == 0 ) {
        z2 = a2;
        z1 = a1;
        z0 = a0;
    }
    else {
        if ( count < 32 ) {
            z2 = a1<<negCount;
            z1 = ( a0<<negCount ) | ( a1>>count );
            z0 = a0>>count;
        }
        else {
            if ( count == 32 ) {
                z2 = a1;
                z1 = a0;
            }
            else {
                a2 |= a1;
                if ( count < 64 ) {
                    z2 = a0<<negCount;
                    z1 = a0>>( count & 31 );
                }
                else {
                    z2 = ( count == 64 ) ? a0 : ( a0 != 0 );
                    z1 = 0;
                }
            }
            z0 = 0;
        }
        z2 |= ( a2 != 0 );
    }
#endif /* !__XTENSA__ */
    *z2Ptr = z2;
    *z1Ptr = z1;
    *z0Ptr = z0;

}

/*----------------------------------------------------------------------------
| Shifts the 64-bit value formed by concatenating `a0' and `a1' left by the
| number of bits given in `count'.  Any bits shifted off are lost.  The value
| of `count' must be less than 32.  The result is broken into two 32-bit
| pieces which are stored at the locations pointed to by `z0Ptr' and `z1Ptr'.
*----------------------------------------------------------------------------*/

INLINE void
 shortShift64Left(
     bits32 a0, bits32 a1, int16 count, bits32 *z0Ptr, bits32 *z1Ptr )
{

#if defined(__XTENSA__) && !defined(__riscv)
    bits32 z0, z1;
    XT_SSL (count);
    z1 = XT_SLL (a1);
    z0 = XT_SRC (a0, a1);
    *z1Ptr = z1;
    *z0Ptr = z0;
#else /* !XTENSA */
    *z1Ptr = a1<<count;
    *z0Ptr =
        ( count == 0 ) ? a0 : ( a0<<count ) | ( a1>>( ( - count ) & 31 ) );
#endif /* !XTENSA */

}

/*----------------------------------------------------------------------------
| Shifts the 96-bit value formed by concatenating `a0', `a1', and `a2' left
| by the number of bits given in `count'.  Any bits shifted off are lost.
| The value of `count' must be less than 32.  The result is broken into three
| 32-bit pieces which are stored at the locations pointed to by `z0Ptr',
| `z1Ptr', and `z2Ptr'.
*----------------------------------------------------------------------------*/

INLINE void
 shortShift96Left(
     bits32 a0,
     bits32 a1,
     bits32 a2,
     int16 count,
     bits32 *z0Ptr,
     bits32 *z1Ptr,
     bits32 *z2Ptr
 )
{
    bits32 z0, z1, z2;

#if defined(__XTENSA__) && !defined(__riscv)
    XT_SSL (count);
    z2 = XT_SLL (a2);
    z1 = XT_SRC (a1, a2);
    z0 = XT_SRC (a0, a1);
#else /* !XTENSA */
    int8 negCount;

    z2 = a2<<count;
    z1 = a1<<count;
    z0 = a0<<count;
    if ( 0 < count ) {
        negCount = ( ( - count ) & 31 );
        z1 |= a2>>negCount;
        z0 |= a1>>negCount;
    }
#endif /* !XTENSA */
    *z2Ptr = z2;
    *z1Ptr = z1;
    *z0Ptr = z0;

}

/*----------------------------------------------------------------------------
| Adds the 64-bit value formed by concatenating `a0' and `a1' to the 64-bit
| value formed by concatenating `b0' and `b1'.  Addition is modulo 2^64, so
| any carry out is lost.  The result is broken into two 32-bit pieces which
| are stored at the locations pointed to by `z0Ptr' and `z1Ptr'.
*----------------------------------------------------------------------------*/

INLINE void
 add64(
     bits32 a0, bits32 a1, bits32 b0, bits32 b1, bits32 *z0Ptr, bits32 *z1Ptr )
{
    bits32 z1;

    z1 = a1 + b1;
    *z1Ptr = z1;
    *z0Ptr = a0 + b0 + ( z1 < a1 );

}

/*----------------------------------------------------------------------------
| Adds the 96-bit value formed by concatenating `a0', `a1', and `a2' to the
| 96-bit value formed by concatenating `b0', `b1', and `b2'.  Addition is
| modulo 2^96, so any carry out is lost.  The result is broken into three
| 32-bit pieces which are stored at the locations pointed to by `z0Ptr',
| `z1Ptr', and `z2Ptr'.
*----------------------------------------------------------------------------*/

INLINE void
 add96(
     bits32 a0,
     bits32 a1,
     bits32 a2,
     bits32 b0,
     bits32 b1,
     bits32 b2,
     bits32 *z0Ptr,
     bits32 *z1Ptr,
     bits32 *z2Ptr
 )
{
    bits32 z0, z1, z2;
    bits32 carry0, carry1;

    z2 = a2 + b2;
    carry1 = ( z2 < a2 );
    z1 = a1 + b1;
    carry0 = ( z1 < a1 );
    z0 = a0 + b0;
    z1 += carry1;
    z0 += ( z1 < carry1 );
    z0 += carry0;
    *z2Ptr = z2;
    *z1Ptr = z1;
    *z0Ptr = z0;

}

/*----------------------------------------------------------------------------
| Subtracts the 64-bit value formed by concatenating `b0' and `b1' from the
| 64-bit value formed by concatenating `a0' and `a1'.  Subtraction is modulo
| 2^64, so any borrow out (carry out) is lost.  The result is broken into two
| 32-bit pieces which are stored at the locations pointed to by `z0Ptr' and
| `z1Ptr'.
*----------------------------------------------------------------------------*/

INLINE void
 sub64(
     bits32 a0, bits32 a1, bits32 b0, bits32 b1, bits32 *z0Ptr, bits32 *z1Ptr )
{

    *z1Ptr = a1 - b1;
    *z0Ptr = a0 - b0 - ( a1 < b1 );

}

/*----------------------------------------------------------------------------
| Subtracts the 96-bit value formed by concatenating `b0', `b1', and `b2' from
| the 96-bit value formed by concatenating `a0', `a1', and `a2'.  Subtraction
| is modulo 2^96, so any borrow out (carry out) is lost.  The result is broken
| into three 32-bit pieces which are stored at the locations pointed to by
| `z0Ptr', `z1Ptr', and `z2Ptr'.
*----------------------------------------------------------------------------*/

INLINE void
 sub96(
     bits32 a0,
     bits32 a1,
     bits32 a2,
     bits32 b0,
     bits32 b1,
     bits32 b2,
     bits32 *z0Ptr,
     bits32 *z1Ptr,
     bits32 *z2Ptr
 )
{
    bits32 z0, z1, z2;
    bits32 borrow0, borrow1;

    z2 = a2 - b2;
    borrow1 = ( a2 < b2 );
    z1 = a1 - b1;
    borrow0 = ( a1 < b1 );
    z0 = a0 - b0;
    z0 -= ( z1 < borrow1 );
    z1 -= borrow1;
    z0 -= borrow0;
    *z2Ptr = z2;
    *z1Ptr = z1;
    *z0Ptr = z0;

}

/*----------------------------------------------------------------------------
| Multiplies `a' by `b' to obtain a 64-bit product.  The product is broken
| into two 32-bit pieces which are stored at the locations pointed to by
| `z0Ptr' and `z1Ptr'.
*----------------------------------------------------------------------------*/

INLINE void mul32To64( bits32 a, bits32 b, bits32 *z0Ptr, bits32 *z1Ptr )
{
#if XCHAL_HAVE_MUL32_HIGH
    bits32 z0, z1;

#if defined(__riscv)
    z0 = XT_MULHU (a, b);
    z1 = XT_MUL (a, b);
#else
    z0 = XT_MULUH (a, b);
    z1 = XT_MULL (a, b);
#endif /* defined(__riscv) */
#else /* !XCHAL_HAVE_MUL32_HIGH */

    bits16 aHigh, aLow, bHigh, bLow;
    bits32 z0, zMiddleA, zMiddleB, z1;

    aLow = a;
    aHigh = a>>16;
#if XCHAL_HAVE_MUL16 || XCHAL_HAVE_MUL32
    bLow = b;
#else /* !(XCHAL_HAVE_MUL16 || XCHAL_HAVE_MUL32) */
    /* clear the high bits of bLow to speed up the multiply routine */
    bLow = b & 0xffff;
#endif /* !(XCHAL_HAVE_MUL16 || XCHAL_HAVE_MUL32) */
    bHigh = b>>16;
    z1 = ( (bits32) aLow ) * bLow;
    zMiddleA = ( (bits32) aLow ) * bHigh;
    zMiddleB = ( (bits32) aHigh ) * bLow;
    z0 = ( (bits32) aHigh ) * bHigh;
    zMiddleA += zMiddleB;
    z0 += ( ( (bits32) ( zMiddleA < zMiddleB ) )<<16 ) + ( zMiddleA>>16 );
    zMiddleA <<= 16;
    z1 += zMiddleA;
    z0 += ( z1 < zMiddleA );
#endif /* !XCHAL_HAVE_MUL32_HIGH */
    *z1Ptr = z1;
    *z0Ptr = z0;

}

/*----------------------------------------------------------------------------
| Multiplies the 64-bit value formed by concatenating `a0' and `a1' by `b'
| to obtain a 96-bit product.  The product is broken into three 32-bit pieces
| which are stored at the locations pointed to by `z0Ptr', `z1Ptr', and
| `z2Ptr'.
*----------------------------------------------------------------------------*/

INLINE void
 mul64By32To96(
     bits32 a0,
     bits32 a1,
     bits32 b,
     bits32 *z0Ptr,
     bits32 *z1Ptr,
     bits32 *z2Ptr
 )
{
    bits32 z0, z1, z2, more1;

    mul32To64( a1, b, &z1, &z2 );
    mul32To64( a0, b, &z0, &more1 );
    add64( z0, more1, 0, z1, &z0, &z1 );
    *z2Ptr = z2;
    *z1Ptr = z1;
    *z0Ptr = z0;

}

/*----------------------------------------------------------------------------
| Multiplies the 64-bit value formed by concatenating `a0' and `a1' to the
| 64-bit value formed by concatenating `b0' and `b1' to obtain a 128-bit
| product.  The product is broken into four 32-bit pieces which are stored at
| the locations pointed to by `z0Ptr', `z1Ptr', `z2Ptr', and `z3Ptr'.
*----------------------------------------------------------------------------*/

INLINE void
 mul64To128(
     bits32 a0,
     bits32 a1,
     bits32 b0,
     bits32 b1,
     bits32 *z0Ptr,
     bits32 *z1Ptr,
     bits32 *z2Ptr,
     bits32 *z3Ptr
 )
{
    bits32 z0, z1, z2, z3;
    bits32 more1, more2;

    mul32To64( a1, b1, &z2, &z3 );
    mul32To64( a1, b0, &z1, &more2 );
    add64( z1, more2, 0, z2, &z1, &z2 );
    mul32To64( a0, b0, &z0, &more1 );
    add64( z0, more1, 0, z1, &z0, &z1 );
    mul32To64( a0, b1, &more1, &more2 );
    add64( more1, more2, 0, z2, &more1, &z2 );
    add64( z0, z1, 0, more1, &z0, &z1 );
    *z3Ptr = z3;
    *z2Ptr = z2;
    *z1Ptr = z1;
    *z0Ptr = z0;

}

/*----------------------------------------------------------------------------
| Returns an approximation to the 32-bit integer quotient obtained by dividing
| `b' into the 64-bit value formed by concatenating `a0' and `a1'.  The
| divisor `b' must be at least 2^31.  If q is the exact quotient truncated
| toward zero, the approximation returned lies between q and q + 2 inclusive.
| If the exact quotient q is larger than 32 bits, the maximum positive 32-bit
| unsigned integer is returned.
*----------------------------------------------------------------------------*/

#ifndef LIBGCC2_INCLUDE_COMMON_DIV

extern bits32 estimateDiv64To32( bits32 a0, bits32 a1, bits32 b );

#else 

STATIC bits32 estimateDiv64To32( bits32 a0, bits32 a1, bits32 b );

STATIC bits32 estimateDiv64To32( bits32 a0, bits32 a1, bits32 b )
{
#if !XCHAL_HAVE_DIV32

    bits32 quotient;
    bits32 bOrZero;
    int count;

    if ( b <= a0 )
	goto special1;

    quotient = 0;

    /* Because we're keeping the remainder in a 32-bit register at each step,
       we need to deal with the case where the high bit of the remainder is
       set (and will be shifted out) but remainder < divisor.  The fastest
       solution I have found is to temporarily force the divisor to zero so
       that the next comparison will always succeed, as it ought to since the
       true remainder at that point is always bigger than the divisor. */

    bOrZero = b;

    XT_SSL (1);
    XT_MOVLTZ (bOrZero, 0, a0);
    a0 = XT_SRC (a0, a1);
    a1 = XT_SLL (a1);

    /* The following loop requires 2.5 instructions (average) more than the
       standard divide loop.  This is necessary to handle the 64-bit dividend.
       If we use the same trick as the standard estimateDiv64To32 code below,
       we really only need this for the first 16 bits of the quotient.
       (Compared to the standard version, we save by computing the first 16
       bits of quotient and the remainder at the same time.)  We can use the
       standard divide loop for the last 16 bits. */

    for (count = 0; count < 15; count++) {
	if (a0 >= bOrZero) {
	    quotient += 1;
	    a0 -= b;
	    bOrZero = b;
	}
	XT_MOVLTZ (bOrZero, 0, a0);
	quotient = XT_SLL (quotient);
	a0 = XT_SRC (a0, a1);
	a1 = XT_SLL (a1);
    }

    if (a0 >= bOrZero) {
	quotient += 1;
	a0 -= b;
    }

    /* Now repeat for the last 16 bits, using the faster divide loop.... */

    b = (b >> 16) << 16;	/* clear the low 16 bits */
    if ( b <= a0 )
	goto special2;

    quotient <<= 1;
    b >>= 1;
    for (count = 0; count < 15; count++) {
	if (a0 >= b) {
	    quotient += 1;
	    a0 -= b;
	}
	quotient <<= 1;
	b >>= 1;
    }
    if (a0 >= b) {
	quotient += 1;
    }

    return quotient;

 special1:
    return 0xFFFFFFFF;

 special2:
    return (quotient << 16) | 0xFFFF;

#else /* XCHAL_HAVE_DIV32 */

    bits32 b0, b1;
    bits32 rem0, rem1, term0, term1;
    bits32 z;

    if ( b <= a0 ) return 0xFFFFFFFF;
    b0 = b>>16;
    z = ( b0<<16 <= a0 ) ? 0xFFFF0000 : ( a0 / b0 )<<16;
    mul32To64( b, z, &term0, &term1 );
    sub64( a0, a1, term0, term1, &rem0, &rem1 );
    while ( ( (sbits32) rem0 ) < 0 ) {
        z -= 0x10000;
        b1 = b<<16;
        add64( rem0, rem1, b0, b1, &rem0, &rem1 );
    }
    rem0 = ( rem0<<16 ) | ( rem1>>16 );
    z |= ( b0<<16 <= rem0 ) ? 0xFFFF : rem0 / b0;
    return z;

#endif /* XCHAL_HAVE_DIV32 */
}

#endif /* LIBGCC2_INCLUDE_COMMON_DIV */

/*----------------------------------------------------------------------------
| Returns an approximation to the square root of the 32-bit significand given
| by `a'.  Considered as an integer, `a' must be at least 2^31.  If bit 0 of
| `aExp' (the least significant bit) is 1, the integer returned approximates
| 2^31*sqrt(`a'/2^31), where `a' is considered an integer.  If bit 0 of `aExp'
| is 0, the integer returned approximates 2^31*sqrt(`a'/2^30).  In either
| case, the approximation returned lies strictly within +/-2 of the exact
| value.
*----------------------------------------------------------------------------*/

#ifndef LIBGCC2_INCLUDE_COMMON_SQRT

extern bits32 estimateSqrt32( int16 aExp, bits32 a );

#else

STATIC bits32 estimateSqrt32( int16 aExp, bits32 a );

STATIC bits32 estimateSqrt32( int16 aExp, bits32 a )
{
#if !XCHAL_HAVE_DIV32

    /* Integer division is very slow without hardware support on
       Xtensa.  It is much faster to compute the sqrt directly, one
       bit at a time.  This code uses the same algorithm as the
       version of sqrt in gnu/newlib/libm/math/e_sqrt.c -- see the
       comments there for an explanation of how it works.  A
       complication here is that estimateSqrt32 is supposed to return
       a full 32-bit result.  (This may not be necessary for a 32-bit
       sqrt but this same code is also used to compute the high bits
       of a 64-bit sqrt, where I suspect all 32-bits are required.)
       At least for the way the newlib code is written, the "error
       term" (`y') requires 2 more bits than the result.  The solution
       used here is to peel off the first 2 iterations (to allow
       shifting the low 2 bits of `a' into `y') and also the last 2
       iterations (to handle the 33 and 34-bit error terms and to
       store the last 2 bits of the result in a separate variable). */
	
    bits32 r, s, q, q0, t, y, y0;
    int count;

    r = 0x20000000L;	/* bit moves to the right, skipping 2 high bits */

    /* load the input `a' into y/y0 */
    if ( aExp & 1 ) {
	/* aExp includes a bias of 0x7f, so if aExp is odd then the
	   real exponent is even... */
	y = a >> 2;
	y0 = a << 30;
    } else {
	/* shift only by one, essentially multiplying the mantissa by 2
	   (relative to the even exponent case) to compensate for the
	   exponent: i.e., sqrt(a*2^(2k+1)) = sqrt(2a)*2^k */
	y = a >> 1;
	y0 = a << 31;
    }

    /* first peeled iteration (test is always true) */
    s = r + r; 
    y -= r; 
    q = r; 
    /* shift y/y0 left by one bit */
    XT_SSL (1);
    y = XT_SRC (y, y0);
    y0 = XT_SLL (y0);
    r >>= 1;

    /* second peeled iteration */
    t = s + r; 
    if (t <= y) { 
	s = t + r; 
	y -= t; 
	q += r; 
    } 
    /* shift y/y0 left by one bit */
    y = XT_SRC (y, y0);
    r >>= 1;

    /* main loop: all but the first 2 and last 2 bits */
    count = 28;
    do {
	t = s + r; 
	if (t <= y) { 
	    s = t + r; 
	    y -= t; 
	    q += r; 
	} 
	y += y;
	r >>= 1;
	count -= 1;
    } while (count > 0);

    /* The last two iterations have to be done separately.  Note that
       we do not really need 33/34 bit additions and comparisons.
       Bits 33/34 of `y' and `s' will always be zero, so "s+r <= y" is
       equivalent to "s < y".  Likewise, we can also simplify many of
       the other operations to avoid multi-word arithmetic.  The last
       two bits of the result go into the low bits of `q0'. */

    q0 = 0;
    y0 = 0;
    if (s < y) {
	y0 = (1 << 31);		/* y0 -= r */
	s += 1;
	y -= s; 
	q0 = 2;
    } 
    y = XT_SRC (y, y0);

    if (s < y) { 
	q0 += 1;
	/* `s', `y' and `r' are dead here; skip updates */
    }

    return (q << 2) | q0;

#else /* XCHAL_HAVE_DIV32 */

    static const bits16 sqrtOddAdjustments[] = {
        0x0004, 0x0022, 0x005D, 0x00B1, 0x011D, 0x019F, 0x0236, 0x02E0,
        0x039C, 0x0468, 0x0545, 0x0631, 0x072B, 0x0832, 0x0946, 0x0A67
    };
    static const bits16 sqrtEvenAdjustments[] = {
        0x0A2D, 0x08AF, 0x075A, 0x0629, 0x051A, 0x0429, 0x0356, 0x029E,
        0x0200, 0x0179, 0x0109, 0x00AF, 0x0068, 0x0034, 0x0012, 0x0002
    };
    int8 index;
    bits32 z;

    index = ( a>>27 ) & 15;
    if ( aExp & 1 ) {
        z = 0x4000 + ( a>>17 ) - sqrtOddAdjustments[ index ];
        z = ( ( a / z )<<14 ) + ( z<<15 );
        a >>= 1;
    }
    else {
        z = 0x8000 + ( a>>17 ) - sqrtEvenAdjustments[ index ];
        z = a / z + z;
        z = ( 0x20000 <= z ) ? 0xFFFF8000 : ( z<<15 );
        if ( z <= a ) return (bits32) ( ( (sbits32) a )>>1 );
    }
    return ( ( estimateDiv64To32( a, 0, z ) )>>1 ) + ( z>>1 );

#endif /* XCHAL_HAVE_DIV32 */
}

#endif /* LIBGCC2_INCLUDE_COMMON_SQRT */


/*----------------------------------------------------------------------------
| Returns the number of leading 0 bits before the most-significant 1 bit of
| `a'.  If `a' is zero, 32 is returned.
*----------------------------------------------------------------------------*/

#ifdef IN_LIBGCC2

INLINE int8 countLeadingZeros32( bits32 a )
{
    return (int8) __builtin_clz(a);
}

#else /* !IN_LIBGCC2 */

int8 countLeadingZeros32( bits32 a )
{
    static const int8 countLeadingZerosHigh[] = {
        8, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4,
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    int8 shiftCount;

    shiftCount = 0;
    if ( a < 0x10000 ) {
        shiftCount += 16;
        a <<= 16;
    }
    if ( a < 0x1000000 ) {
        shiftCount += 8;
        a <<= 8;
    }
    shiftCount += countLeadingZerosHigh[ a>>24 ];
    return shiftCount;
}
#endif /* !IN_LIBGCC2 */

/*----------------------------------------------------------------------------
| Returns 1 if the 64-bit value formed by concatenating `a0' and `a1' is
| equal to the 64-bit value formed by concatenating `b0' and `b1'.  Otherwise,
| returns 0.
*----------------------------------------------------------------------------*/

INLINE flag eq64( bits32 a0, bits32 a1, bits32 b0, bits32 b1 )
{

    return ( a0 == b0 ) && ( a1 == b1 );

}

/*----------------------------------------------------------------------------
| Returns 1 if the 64-bit value formed by concatenating `a0' and `a1' is less
| than or equal to the 64-bit value formed by concatenating `b0' and `b1'.
| Otherwise, returns 0.
*----------------------------------------------------------------------------*/

INLINE flag le64( bits32 a0, bits32 a1, bits32 b0, bits32 b1 )
{

    return ( a0 < b0 ) || ( ( a0 == b0 ) && ( a1 <= b1 ) );

}

/*----------------------------------------------------------------------------
| Returns 1 if the 64-bit value formed by concatenating `a0' and `a1' is less
| than the 64-bit value formed by concatenating `b0' and `b1'.  Otherwise,
| returns 0.
*----------------------------------------------------------------------------*/

INLINE flag lt64( bits32 a0, bits32 a1, bits32 b0, bits32 b1 )
{

    return ( a0 < b0 ) || ( ( a0 == b0 ) && ( a1 < b1 ) );

}

/*----------------------------------------------------------------------------
| Returns 1 if the 64-bit value formed by concatenating `a0' and `a1' is not
| equal to the 64-bit value formed by concatenating `b0' and `b1'.  Otherwise,
| returns 0.
*----------------------------------------------------------------------------*/

INLINE flag ne64( bits32 a0, bits32 a1, bits32 b0, bits32 b1 )
{

    return ( a0 != b0 ) || ( a1 != b1 );

}


