/* Constants for assembly fields */
#ifndef __DFPEMU_FP_H__
#define __DFPEMU_FP_H__

/*  Copyright (c) 2007-2009 Tensilica Inc.  ALL RIGHTS RESERVED.
 *  This program is the copyrighted work of Tensilica Inc.  You may use
 *  it under the terms of version 2 of the GNU General Public License as
 *  published by the Free Software Foundation.  Other use is prohibited
 *  without the prior written consent of Tensilica Inc.
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.
 */


// operations for CMPH
#define SUB_OP  0
#define MUL_OP  1
#define SQRT_OP 2
#define DIV_OP  3
#define ADD_OP  4
#define LT_OP   5
#define LE_OP   6
#define EQ_OP   7
#define GT_OP   8
#define GE_OP   9
#define NE_OP  10
// analyze A & B inputs
#define TST_OP 11

// modes for F64RND
#define RND_SH_DOWN 0   // shift down by one, don't update exponent
#define RND_SEEK    1   // seek MSB in +-1 range, adjust exponent
#define RND_ONLY    2   // don't shift, don't update exponent, just round


// Bits in status register, some have same value since mutually exclude in different operations
#define Complex  31    // result already known to be NaN/Inf/Zero, or further pre processing required
#define Simplex  30    // result already known to be NaN/Inf/Zero, round can handle this
#define mk_NaN   29    // Result must be NaN
#define mk_Inf   28    // Result must be Inf
#define mk_Zero  27    // Result must be Zero
// 26,25,24,23,22 reserved
#define LongPost 22    // Long post normalization required (mul/sub)
#define PUF      22    // potential underflow (div)
#define DorNOut  21    // denormal or normal output (mul)
#define LongPre  21    // longer than 32bit pre-shift required, add/sub
#define NearSub  20    // effective sub, where smaller is only by 0 or 1 exp units smaller (potential large post-shift)

#define Carry1   19
#define Carry0   18
#define EffSub   17    // Effective subtraction (add/sub)
#define AgtB     16    // add,sub, A > B
#define RetB     15    // add,sub, return B (maybe with sign inversion)
#define RetA     14    // add,sub, return A as is, B 0 or so small it don't affect result
#define Bdenorm  13    // A is denormal
#define Adenorm  12    // B is denormal
#define rSign    11    // sign of result
// Bits 10:0 exponent OR:
// TST operation uses some of these bits 

//#define BgtA     6    // abs(B) > abs(A)
#define NanB     5    // B is NaN
#define NanA     4    // A is NaN
#define InfB     3    // B is infinity
#define InfA     2    // A is infinity
#define ZeroB    1    // B is zero
#define ZeroA    0    // A is zero

// parameters for F64ITER
#define DIVI      2
#define SQRI_LOW  0
#define SQRI_HIGH 1
#define NO_SHIFT  1
#define DO_SHIFT  0

#endif /* __DFPEMU_FP_H__ */
