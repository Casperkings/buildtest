
/* Copyright (c) 2013 by Tensilica Inc.
   This file has been modified for use in the libgcc library for Xtensa
   processors.  These modifications are distributed under the same terms
   as the original SoftFloat code.  */

/*============================================================================

This C header file is part of the SoftFloat IEC/IEEE Floating-point Arithmetic
Package, Release 2b.

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

#include <xtensa/config/core.h>
#include <xtensa/config/system.h>
#include <xtensa/tie/xt_core.h>
#if XCHAL_HAVE_NSA
#include <xtensa/tie/xt_misc.h>
#endif
#if XCHAL_HAVE_MUL32_HIGH
#include <xtensa/tie/xt_MUL32.h>
#endif

/*----------------------------------------------------------------------------
| One of the macros `BIGENDIAN' or `LITTLEENDIAN' must be defined.
*----------------------------------------------------------------------------*/
#ifdef __XTENSA_EB__
#define BIGENDIAN
#else
#define LITTLEENDIAN
#endif

/*----------------------------------------------------------------------------
| The macro `BITS64' can be defined to indicate that 64-bit integer types are
| supported by the compiler.
*----------------------------------------------------------------------------*/
#define BITS64

/*----------------------------------------------------------------------------
| Each of the following `typedef's defines the most convenient type that holds
| integers of at least as many bits as specified.  For example, `uint8' should
| be the most convenient type that can hold unsigned integers of as many as
| 8 bits.  The `flag' type must be able to hold either a 0 or 1.  For most
| implementations of C, `flag', `uint8', and `int8' should all be `typedef'ed
| to the same as `int'.
*----------------------------------------------------------------------------*/
typedef char flag;
typedef unsigned char uint8;
typedef signed char int8;
typedef int uint16;
typedef int int16;
typedef unsigned int uint32;
typedef signed int int32;
#ifdef BITS64
typedef unsigned long long int uint64;
typedef signed long long int int64;
#endif

/*----------------------------------------------------------------------------
| Each of the following `typedef's defines a type that holds integers
| of _exactly_ the number of bits specified.  For instance, for most
| implementation of C, `bits16' and `sbits16' should be `typedef'ed to
| `unsigned short int' and `signed short int' (or `short int'), respectively.
*----------------------------------------------------------------------------*/
typedef unsigned char bits8;
typedef signed char sbits8;
typedef unsigned short int bits16;
typedef signed short int sbits16;
typedef unsigned int bits32;
typedef signed int sbits32;
#ifdef BITS64
typedef unsigned long long int bits64;
typedef signed long long int sbits64;
#endif

#ifdef BITS64
/*----------------------------------------------------------------------------
| The `LIT64' macro takes as its argument a textual integer literal and
| if necessary ``marks'' the literal as having a 64-bit integer type.
| For example, the GNU C Compiler (`gcc') requires that 64-bit literals be
| appended with the letters `LL' standing for `long long', which is `gcc's
| name for the 64-bit integer type.  Some compilers may allow `LIT64' to be
| defined as the identity macro:  `#define LIT64( a ) a'.
*----------------------------------------------------------------------------*/
#define LIT64( a ) a##LL
#endif

/*----------------------------------------------------------------------------
| Symbolic Boolean literals.
*----------------------------------------------------------------------------*/
enum {
    FALSE = 0,
    TRUE  = 1
};


/* STATIC - "static" keyword if it's not compiled for libgcc,
 for libgcc some which were static have to be exported. */

#ifdef IN_LIBGCC2
#define INLINE static __attribute__((always_inline, unused))
#define STATIC
#else /* !IN_LIBGCC2 */
#define INLINE
#define STATIC static
#endif /* !IN_LIBGCC2 */

#ifndef IN_LIBGCC2

#define LIBGCC2_INCLUDE_COMMON
#define LIBGCC2_INCLUDE_COMMON_F32
#define LIBGCC2_INCLUDE_COMMON_F64
#define LIBGCC2_INCLUDE_COMMON_DIV
#define LIBGCC2_INCLUDE_COMMON_SQRT

#else

/* in libgcc: turning on individual functions */

#ifdef LIBGCC2_INCLUDE_DIVF64
#if !XCHAL_HAVE_DFP_accel && !XCHAL_HAVE_DFP_DIV && XCHAL_HAVE_DIV32 && XSHAL_CLIB!=XTHAL_CLIB_UCLIBC
#define L_divdf3
#endif
#endif

#ifdef LIBGCC2_INCLUDE_MODF64
#define L_moddf3
#endif

#ifdef LIBGCC2_INCLUDE_SQRTF64
#if !XCHAL_HAVE_DFP_accel && !XCHAL_HAVE_DFP_SQRT
#define L_sqrtdf2
#endif
#endif

#ifdef LIBGCC2_INCLUDE_MODF32
#define L_modsf3
#endif

#ifdef LIBGCC2_INCLUDE_SQRTF32
#if !XCHAL_HAVE_FP_SQRT
#define L_sqrtsf2
#endif
#endif


#endif

/*
-------------------------------------------------------------------------------
Translate names to those used by GCC.
-------------------------------------------------------------------------------
*/

#ifdef IN_LIBGCC2

#define float_detect_tininess           float_round_nearest_even
#define float_rounding_mode             float_tininess_after_rounding

/* global variables */
#define float_exception_flags           __float_exception_flags

/* global functions */
#define float_raise                     __float_raise
#define estimateDiv64To32               __estimateDiv64To32
#define estimateSqrt32                  __estimateSqrt32
#define propagateFloat32NaN             __propagateFloat32NaN
#define propagateFloat64NaN             __propagateFloat64NaN
#define roundAndPackFloat32             __roundAndPackFloat32
#define roundAndPackFloat64             __roundAndPackFloat64

/* 32-bit floating-point functions */
#define float32_add                     __addsf3
#define float32_sub                     __subsf3
#define float32_mul                     __mulsf3
#define float32_div                     __divsf3
#define float32_neg                     __negsf2
#define float32_abs                     __abssf2
/* math library will use square root and reminder provided here
#define float32_sqrt                    __sqrtsf2
#define float32_rem                     __modsf3
*/
#define float32_sqrt                    __ieee754_sqrtf
#define float32_rem                     __ieee754_remainderf
#define float32_eq                      __eqsf2
#define float32_le                      __lesf2
#define float32_lt                      __ltsf2
#define float32_ne                      __nesf2
#define float32_ge                      __gesf2
#define float32_gt                      __gtsf2
#define float32_unord                   __unordsf2
#define int32_to_float32                __floatsisf
#define int64_to_float32                __floatdisf
#define float32_to_int32_round_to_zero  __fixsfsi
#define float32_to_int64_round_to_zero  __fixsfdi
#define float32_to_uint32_round_to_zero __fixunssfsi
#define float32_to_uint64_round_to_zero __fixunssfdi
#define float32_to_float64              __extendsfdf2

/* 64-bit floating-point functions */
#define float64_add                     __adddf3
#define float64_sub                     __subdf3
#define float64_mul                     __muldf3
#define float64_div                     __divdf3
#define float64_neg                     __negdf2
#define float64_abs                     __absdf2
/*
#define float64_sqrt                    __sqrtdf2
#define float64_rem                     __moddf3
*/
#define float64_sqrt                    __ieee754_sqrt
#define float64_rem                     __ieee754_remainder
#define float64_eq                      __eqdf2
#define float64_le                      __ledf2
#define float64_lt                      __ltdf2
#define float64_ne                      __nedf2
#define float64_ge                      __gedf2
#define float64_gt                      __gtdf2
#define float64_unord                   __unorddf2
#define int32_to_float64                __floatsidf
#define int64_to_float64                __floatdidf
#define float64_to_int32_round_to_zero  __fixdfsi
#define float64_to_int64_round_to_zero  __fixdfdi
#define float64_to_uint32_round_to_zero __fixunsdfsi
#define float64_to_uint64_round_to_zero __fixunsdfdi
#define float64_to_float32              __truncdfsf2

#endif /* IN_LIBGCC2 */

