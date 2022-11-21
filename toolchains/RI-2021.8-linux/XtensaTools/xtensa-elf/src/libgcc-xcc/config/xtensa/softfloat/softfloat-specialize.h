
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
| Underflow tininess-detection mode, statically initialized to default value.
| (The declaration in `softfloat.h' must match the `int8' type here.)
*----------------------------------------------------------------------------*/

#ifndef IN_LIBGCC2
/* for now in libgcc this parameter is fixed to float_tininess_after_rounding */
int8 float_detect_tininess = float_tininess_after_rounding;
#endif

/*----------------------------------------------------------------------------
| Floating-point exception flags.
| Only xclib uses this flags.
*----------------------------------------------------------------------------*/

#if defined(LIBGCC2_INCLUDE_COMMON) && (XSHAL_CLIB==XTHAL_CLIB_XCLIB)
int8 float_exception_flags = 0;
#endif

/*----------------------------------------------------------------------------
| Raises the exceptions specified by `flags'.  Floating-point traps can be
| defined here if desired.  It is currently not possible for such a trap
| to substitute a result value.  If traps are not implemented, this routine
| should be simply `float_exception_flags |= flags;'.
*----------------------------------------------------------------------------*/

INLINE void float_raise( int8 flags )
{
#if XSHAL_CLIB==XTHAL_CLIB_XCLIB
    float_exception_flags |= flags;
#endif
}

/*----------------------------------------------------------------------------
| Internal canonical NaN format.
*----------------------------------------------------------------------------*/
typedef struct {
    flag sign;
    bits32 high, low;
} commonNaNT;

/*----------------------------------------------------------------------------
| The pattern for a default generated single-precision NaN.
*----------------------------------------------------------------------------*/
enum {
    float32_default_nan = 0x7FC00000
};

/*----------------------------------------------------------------------------
| Returns 1 if the single-precision floating-point value `a' is a NaN;
| otherwise returns 0.
*----------------------------------------------------------------------------*/

INLINE flag float32_is_nan( float32 a )
{

    return ( 0xFF000000 < (bits32) ( a<<1 ) );

}

/*----------------------------------------------------------------------------
| Returns 1 if the single-precision floating-point value `a' is a signaling
| NaN; otherwise returns 0.
*----------------------------------------------------------------------------*/

INLINE flag float32_is_signaling_nan( float32 a )
{

    return ( ( ( a>>22 ) & 0x1FF ) == 0x1FE ) && ( a & 0x003FFFFF );

}

#ifndef IN_LIBGCC2
/*----------------------------------------------------------------------------
| Returns the result of converting the single-precision floating-point NaN
| `a' to the canonical NaN format.  If `a' is a signaling NaN, the invalid
| exception is raised.
*----------------------------------------------------------------------------*/

static commonNaNT float32ToCommonNaN( float32 a )
{
    commonNaNT z;

    if ( float32_is_signaling_nan( a ) ) float_raise( float_flag_invalid );
    z.sign = a>>31;
    z.low = 0;
    z.high = a<<9;
    return z;

}

/*----------------------------------------------------------------------------
| Returns the result of converting the canonical NaN `a' to the single-
| precision floating-point format.
*----------------------------------------------------------------------------*/

static float32 commonNaNToFloat32( commonNaNT a )
{

    return ( ( (bits32) a.sign )<<31 ) | 0x7FC00000 | ( a.high>>9 );

}

#endif /* ifndef IN_LIBGCC2 */

/*----------------------------------------------------------------------------
| Takes two single-precision floating-point values `a' and `b', one of which
| is a NaN, and returns the appropriate NaN result.  If either `a' or `b' is a
| signaling NaN, the invalid exception is raised.
*----------------------------------------------------------------------------*/

#ifndef LIBGCC2_INCLUDE_COMMON_F32

extern float32 propagateFloat32NaN( float32 a, float32 b );

#else

STATIC float32 propagateFloat32NaN( float32 a, float32 b );

STATIC float32 propagateFloat32NaN( float32 a, float32 b )
{
    flag aIsNaN, aIsSignalingNaN, bIsNaN, bIsSignalingNaN;

    aIsNaN = float32_is_nan( a );
    aIsSignalingNaN = float32_is_signaling_nan( a );
    bIsNaN = float32_is_nan( b );
    bIsSignalingNaN = float32_is_signaling_nan( b );
    a |= 0x00400000;
    b |= 0x00400000;
    if ( aIsSignalingNaN | bIsSignalingNaN ) float_raise( float_flag_invalid );
    if ( aIsSignalingNaN ) {
        if ( bIsSignalingNaN ) goto returnLargerSignificand;
        return bIsNaN ? b : a;
    }
    else if ( aIsNaN ) {
        if ( bIsSignalingNaN | ! bIsNaN ) return a;
 returnLargerSignificand:
        if ( (bits32) ( a<<1 ) < (bits32) ( b<<1 ) ) return b;
        if ( (bits32) ( b<<1 ) < (bits32) ( a<<1 ) ) return a;
        return ( a < b ) ? a : b;
    }
    else {
        return b;
    }

}

#endif

/*----------------------------------------------------------------------------
| The pattern for a default generated double-precision NaN.  The `high' and
| `low' values hold the most- and least-significant bits, respectively.
*----------------------------------------------------------------------------*/
enum {
    float64_default_nan_high = 0x7FF80000,
    float64_default_nan_low  = 0x00000000
};

/*----------------------------------------------------------------------------
| Returns 1 if the double-precision floating-point value `a' is a NaN;
| otherwise returns 0.
*----------------------------------------------------------------------------*/

INLINE flag float64_is_nan( float64 a )
{

    return
           ( 0xFFE00000 <= (bits32) ( a.high<<1 ) )
        && ( a.low || ( a.high & 0x000FFFFF ) );

}

/*----------------------------------------------------------------------------
| Returns 1 if the double-precision floating-point value `a' is a signaling
| NaN; otherwise returns 0.
*----------------------------------------------------------------------------*/

INLINE flag float64_is_signaling_nan( float64 a )
{

    return
           ( ( ( a.high>>19 ) & 0xFFF ) == 0xFFE )
        && ( a.low || ( a.high & 0x0007FFFF ) );

}

#ifndef IN_LIBGCC2

/*----------------------------------------------------------------------------
| Returns the result of converting the double-precision floating-point NaN
| `a' to the canonical NaN format.  If `a' is a signaling NaN, the invalid
| exception is raised.
*----------------------------------------------------------------------------*/

static commonNaNT float64ToCommonNaN( float64 a )
{
    commonNaNT z;

    if ( float64_is_signaling_nan( a ) ) float_raise( float_flag_invalid );
    z.sign = a.high>>31;
    shortShift64Left( a.high, a.low, 12, &z.high, &z.low );
    return z;

}

/*----------------------------------------------------------------------------
| Returns the result of converting the canonical NaN `a' to the double-
| precision floating-point format.
*----------------------------------------------------------------------------*/

static float64 commonNaNToFloat64( commonNaNT a )
{
    float64 z;

    shift64Right( a.high, a.low, 12, &z.high, &z.low );
    z.high |= ( ( (bits32) a.sign )<<31 ) | 0x7FF80000;
    return z;

}

#endif /* ifndef IN_LIBGCC2 */

/*----------------------------------------------------------------------------
| Takes two double-precision floating-point values `a' and `b', one of which
| is a NaN, and returns the appropriate NaN result.  If either `a' or `b' is a
| signaling NaN, the invalid exception is raised.
*----------------------------------------------------------------------------*/

#ifndef LIBGCC2_INCLUDE_COMMON_F64

extern float64 propagateFloat64NaN( float64 a, float64 b );

#else

STATIC float64 propagateFloat64NaN( float64 a, float64 b );

STATIC float64 propagateFloat64NaN( float64 a, float64 b )
{
    flag aIsNaN, aIsSignalingNaN, bIsNaN, bIsSignalingNaN;

    aIsNaN = float64_is_nan( a );
    aIsSignalingNaN = float64_is_signaling_nan( a );
    bIsNaN = float64_is_nan( b );
    bIsSignalingNaN = float64_is_signaling_nan( b );
    a.high |= 0x00080000;
    b.high |= 0x00080000;
    if ( aIsSignalingNaN | bIsSignalingNaN ) float_raise( float_flag_invalid );
    if ( aIsSignalingNaN ) {
        if ( bIsSignalingNaN ) goto returnLargerSignificand;
        return bIsNaN ? b : a;
    }
    else if ( aIsNaN ) {
        if ( bIsSignalingNaN | ! bIsNaN ) return a;
 returnLargerSignificand:
        if ( lt64( a.high<<1, a.low, b.high<<1, b.low ) ) return b;
        if ( lt64( b.high<<1, b.low, a.high<<1, a.low ) ) return a;
        return ( a.high < b.high ) ? a : b;
    }
    else {
        return b;
    }

}

#endif
