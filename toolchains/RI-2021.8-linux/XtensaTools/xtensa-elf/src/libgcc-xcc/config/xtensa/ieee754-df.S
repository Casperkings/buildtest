/* IEEE-754 double-precision functions for Xtensa
   Copyright (C) 2006, 2007 Free Software Foundation, Inc.
   Copyright 2009 Tensilica, Inc.
   Contributed by Bob Wilson (bwilson@tensilica.com) at Tensilica.

   This file is part of GCC.

   GCC is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   In addition to the permissions in the GNU General Public License,
   the Free Software Foundation gives you unlimited permission to link
   the compiled version of this file into combinations with other
   programs, and to distribute those combinations without any
   restriction coming from the use of this file.  (The General Public
   License restrictions do apply in other respects; for example, they
   cover modification of the file, and distribution when not linked
   into a combine executable.)

   GCC is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with GCC; see the file COPYING.  If not, write to the Free
   Software Foundation, 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.  */

#ifdef __XTENSA_EB__
#define xh a2
#define xl a3
#define yh a4
#define yl a5
#else
#define xh a3
#define xl a2
#define yh a5
#define yl a4
#endif

#if XCHAL_HAVE_DFP_accel
#include "dfpemu_fp.h"
#include "xtensa/config/core.h"
#endif // XCHAL_HAVE_DFP_accel

#include "xtensa/config/system.h"

#if XCHAL_HAVE_FP && !XCHAL_HAVE_VECTORFPU2005
#include <xtensa/tie/xt_FP.h>
#endif

#ifdef  _FP_REGISTER
#define F0  _FP_REGISTER(0)
#elif XCHAL_HAVE_VECTORFPU2005
#define F0 v0
#else
#ifndef F0
#define F0 f0
#endif
#endif


/*  Warning!  The branch displacements for some Xtensa branch instructions
    are quite small, and this code has been carefully laid out to keep
    branch targets in range.  If you change anything, be sure to check that
    the assembler is not relaxing anything to branch over a jump.  */

#ifdef L_negdf2

	.align	4
	.global	__negdf2
	.type	__negdf2, @function
__negdf2:
	leaf_entry sp, 16
	movi	a4, 0x80000000
	xor	xh, xh, a4
	leaf_return
	.size	__negdf2, . - __negdf2
#endif /* L_negdf2 */

#if XCHAL_HAVE_DFP_accel

#ifdef L_sqrt

// Implemented here with the funny name so that all of libm's internal
// functions that call it get the benefit.  Without the dfp acceleration
// tie this gets implemented inside libm.
//
// extern double __ieee754_sqrt(double);
        .text
        .align 4
        .global __ieee754_sqrt
        .type __ieee754_sqrt,@function

// input params (for little endian):
// a2 low part of the double
// a3 high part of the double 
// return value in same regs, same order

__ieee754_sqrt:
        leaf_entry   a1, 16
        f64cmpl a4, xl, xl
        f64cmph a4, xh, xh, SQRT_OP
        bbsi.l a4, Simplex, .Lsq_exit  // early exit, result known already
        bbsi.l a4, Adenorm, .Lsq_denorm 
.Lsq_Rerun:
// accu = mantissa
        f64sig  a5, xh
// if even(exp) accu <<= 1 (f64cmph sets SAR to 32 or 31)
        src     a7, a5, xl      // high part
        sll     a6, xl          // low part
// init F64R with radicant (mantissa)   
        wf64r   a7, a6, 0
// init F64S with 0x00200000 
        movi     a5, 1           
        slli     a5, a5, 21
        wur.f64s a5
// init result regs to to 0
        movi    a3, 0           
        movi    a2, 0
// high part, 22 iterations, first one separately
        f64iter a3, a3, a2, SQRI_HIGH, NO_SHIFT
// second too separately, keep remaining iteration count even	
        f64iter a3, a3, a2, SQRI_HIGH, DO_SHIFT
        movi    a7, 10
#if XCHAL_HAVE_LOOPS	
// execute 2 iterations per loop round, 1 pipeline bubble less  
        loopgtz a7, .Lsq1 // 18+21
        f64iter a3, a3, a2, SQRI_HIGH, DO_SHIFT
        f64iter a3, a3, a2, SQRI_HIGH, DO_SHIFT
.Lsq1:
#else
.Lsq1:
// without hardware loop, execute 4 iterations per loop, reduce loop overhead
        f64iter a3, a3, a2, SQRI_HIGH, DO_SHIFT
        f64iter a3, a3, a2, SQRI_HIGH, DO_SHIFT
        f64iter a3, a3, a2, SQRI_HIGH, DO_SHIFT
        f64iter a3, a3, a2, SQRI_HIGH, DO_SHIFT
	addi	a7, a7, -2
	bnez	a7, .Lsq1
#endif
		
// low part next, 32 iterations
        movi    a7, 16
#if XCHAL_HAVE_LOOPS	
        loopgtz a7, .Lsq2
        f64iter a2, a3, a2, SQRI_LOW, DO_SHIFT
        f64iter a2, a3, a2, SQRI_LOW, DO_SHIFT
.Lsq2:
#else	
.Lsq2:
        f64iter a2, a3, a2, SQRI_LOW, DO_SHIFT
        f64iter a2, a3, a2, SQRI_LOW, DO_SHIFT
        f64iter a2, a3, a2, SQRI_LOW, DO_SHIFT
        f64iter a2, a3, a2, SQRI_LOW, DO_SHIFT
	addi	a7, a7, -2
	bnez	a7, .Lsq2
#endif	
		
// restore status register from AR copy 
        wur.f64s a4
// restore exponent, do rounding if needed
.Lsq_exit:              
        f64rnd  xh, a3, a2, RND_SH_DOWN
        rf64r   xl, 0
        leaf_return
// cycle estimates:     
//   early exit:         11 cycles (NaN/Negative/Inf/0 input)
//   main exit:          80 cycles (with hw loop)
//   main exit:         124 cycles (without hw loop)
//   denormal input:     97 / 141 cycles (with / wo hw loop)

// special case: Denormal input
// 13 instruction, 17 cycles of overhead for denormal input     
.Lsq_denorm:
// Modify denormal to look like regular and jump back to main loop
// lets shift the first 1 to same place where hidden bit should be
// use f64norm instead of NSA since existance of NSA not guaranteed
        f64norm a4, xh, xl, 1   // get shift count to a4, update SAR
        bltui   a4, 32, .Lsq_smallshift
        mov     xh, xl  // larger shift than 32
        movi    xl, 0
.Lsq_smallshift:
        src     xh, xh, xl  // high part
        sll     xl, xl      // low part
// shifting done, calculate the new exponent, a4 has shift count
// new exp = ((1-shift)+1023+LSB) >> 1, LSB is LSB of exp
        neg     a4, a4         // convert shift amount to negative exp
        addi    a4, a4, 1024   // exp = 1-shift
        extui   a5, a4, 0, 1   // extract the LSB of EXP to a5
        sub     a4, a4, a5     // exp - LSB
        srli    a4, a4, 1      // >> 1
        ssl     a5              // shift by 0 or 1
// the exponent/status (a4) reg looks like it should after f64cmph 
// also input in xh:xl is like normal number, sar fixed too.
// the normal routine can do the rest
// Note: the f64s state not updated with new exponent, not needed since 
// it will used as one-hot in main loop, and restored from a4 anyway
        j       .Lsq_Rerun
	.size	__ieee754_sqrt, . - __ieee754_sqrt
#endif // L_sqrt

#ifdef L_addsubdf3
        .text
        .align 4

        // operand A is contained in a3/a2
        // operand B is contained in a5/a4
        // result  will be placed in a3/a2
        .global __adddf3
        .type __adddf3,@function
        .align 4

__adddf3:
        leaf_entry   a1,  16
        // compare and analyze operands, branch out to handler
        f64cmpl a6, xl,  yl
        f64cmph a6, xh,  yh, ADD_OP
        bltz   a6, .Las_Other  // Special cases (bit-31 set)
        bbci.l a6, AgtB, .Las_BgtA

// Preparation for case is where A>B
// place larger to xh/xl and aligned smaller to yh/a8/a9 (a9 guard/sticky)
.Las_AgtB:
        f64sig  xh, xh
        f64sig  yh, yh
        // align B (shift right, outed bits into a9)
        sll     a9, yl        // set sticky portion in a9
        src     a8, yh, yl    // place lower portion
        srl     yh, yh        // smaller in yh/a8 (a9 stky)
// common preparations done, jump to the core routine
        bbsi.l a6, NearSub, .Las_sub01
        bbsi.l a6, LongPre, .Las_shf // shift more if needed
        bbsi.l a6, EffSub,  .Las_EffSub // change branch if effective add

// entry point, from above or from BgtA
// now the larger in xh/xl and smaller in yh/a8/a9
.Las_EffAdd:
        f64addc xl, a8, 1, 1    // lower part
        f64addc xh, yh, 0, 1    // upper part
        wf64r   a9, a9, 1       // guard+sticky to accu
        f64rnd  xh, xh, xl, RND_SEEK
        rf64r   xl, 0
        leaf_return

// Long pre normalization
// For effective add there is jump to different branch.
// For effective sub the operand B is shifted right by 32, and subtracted from A
//
.Las_shf:
        bbci.l a6, EffSub, .Las_EffAdd2   // handle effective-add elsewhere
// F64R = 0 - a8/a9,  also update carry flag for low part subtraction.
        f64subc a9, a9, 3, 1   //
        f64subc a8, a8, 2, 1   //
        wf64r   a8, a9, 0      //
        movi    a8,  0         // zero top part
        f64subc xl, yh, 0, 1   // lower portion = x_low - y_high since y >> 32
        f64subc xh, a8, 0, 1   // higher portion = x_low - 0 - carry
        f64rnd  xh, xh, xl, RND_SEEK  // round and setup exponent
        rf64r   xl, 0
        leaf_return

///////////////////////////////////////////////////////////////////////
// Sub function inverleaved with add function, uses same code anyway.
// Saves code space, but makes profiling more difficult.
        .global __subdf3
        .type __subdf3,@function
        .align 4

__subdf3:
        leaf_entry   a1,  16
        // compare and analyze operands, branch out to handler
        f64cmpl a6, xl,  yl
        f64cmph a6, xh,  yh, SUB_OP
        bltz    a6, .Las_Other  // Special cases (bit-31 set)
        bbsi.l a6, AgtB, .Las_AgtB

// This case is where B>A and the operation is an effective add
// OR where B>A by 2 or more exponent units.
// place larger to xh/xl and aligned smaller to yh/a8 and guard/sticky in a9
// The Sub instruction falls through to this case and branches
// to AgtB while the Add instruction does the reverse
.Las_BgtA:
        f64sig  a7, xh
        f64sig  xh, yh
        sll     a9, xl        // set sticky portion in a9
        src     a8, a7, xl    // place lower portion
        srl     yh, a7        // smaller in yh/a8 (a9 stky)
        mov     xl, yl        // complete B to A copy
        bbsi.l a6, NearSub,.Las_sub01
        bbsi.l a6, LongPre,.Las_shf     // shift more if needed
        bbci.l a6, EffSub, .Las_EffAdd  // change branch if effective subtract

// entry point for effective subtraction, from above or from AgtB
.Las_EffSub:
        f64subc a9, a9, 3, 1   // prepare carry and guard/sticky for sub
        f64subc xl, a8, 0, 1   // subtract lower portion
        f64subc xh, yh, 0, 1   // subtract upper portion
        wf64r   a9, a9, 1      // guard+sticky to accu high part
        f64rnd  xh, xh, xl, RND_SEEK  // round and setup exponent
        rf64r   xl, 0
        leaf_return


// This segment of covers the case of an effective subtract with
// identical exponents (or exponent difference is 1).
// This case has a short pre-shift but potentially a long post normalize.
// pre-shift and operand swapping done in AgtB/BgtA branches.
.Las_sub01:
        f64subc a7, a9, 3, 1  // set C1 if a9 zero (no sticky)
        f64subc xl, a8, 0, 1  // lower portion complete
        f64subc xh, yh, 0, 1  // upper portion complete
        f64subc a7, a9, 3, 1  // Repeat to put sticky bit in C1
        f64norm a6, xh, xl,0  // count number of leading zeros
        src     xh, xh, xl    // move upwards
        src     xl, xl, a7    // move lower portion
        // required shift can be larger than 32, do it in separate branch
        bbsi.l a6, LongPost, .Las_A01sf  // need to shift farther
        f64sexp xh, xh, a6     // setup sign/exp
        f64addc xl, xl, 2, 0   // round lower
        f64addc xh, xh, 2, 0   // round upper
        leaf_return

// long pre-alignment with effective add.
// Almost duplicate of effective add, except operand B is shifter right by 32
.Las_EffAdd2:
        wf64r   a8, a9, 0       // guard+sticky to accu
        f64addc xl, yh, 1, 1    // lower part = x_low + (y >> 32)
        f64addc xh, xh, 2, 1    // upper part = x_high + carry
        f64rnd  xh, xh, xl, RND_SEEK
        rf64r   xl, 0
        leaf_return

// This code does an additional 32 bits worth of post normalization
// for sub01 case and then exits
.Las_A01sf:
        f64sexp xh, xl, a6
        sll     xl, a7
// no rounding required, since LSBs are known to be 0
// the sticky has at most 1 bit since pre-aligment is by one
        leaf_return


// This portion handles when there is a NaN or Inf, or 0 result.
// Or both inputs are denorm, the main routines can't handle
// cases where results hidden bit is not where it is supposed to be.
.Las_Other:
        bbsi.l a6, Simplex, .Las_exit
        bbsi.l a6, RetB, .Las_RetB
// fall-through, both are denorm, own code for this
// since in other cases the first bit won't be in expected place
.Las_Denorm:
// No aligment needed, both have same exponent
        bbsi.l a6, EffSub, .Las_dsub  // if effective sub, go to it's handler
// remove sign bit from operand B (and exp, but it was 0 anyway),
// needed if both are negative, don't matter if both positive
        f64sig  yh, yh
        f64addc xl, yl, 1, 0
        f64addc xh, yh, 0, 0
// the carry will propage in add, and set exponent correctly if growing into nor mal,
// no rounding needed, there is no guard/round bits for denormals (no pre-alignm ent)
        leaf_return

// denormal - denormal handling
.Las_dsub:
        bbsi.l  a6,  AgtB, .Las_dAgtB
.Las_dBgtA:
        f64subc yl, xl, 1, 0
        f64subc yh, xh, 0, 0
        f64sexp xh, yh, a6
        mov     xl, yl
        leaf_return
.Las_dAgtB:
        f64subc xl, yl, 1, 0
        f64subc xh, yh, 0, 0
// no need to check exponent, denorm - denorm is always denorm (or zero)
// but sign could have changed
        f64sexp xh, xh, a6
        leaf_return

// make Nan/Inf/Zero/Return A
.Las_exit:
        f64rnd    xh, xh, xl, RND_ONLY
        rf64r     xl, 0
        leaf_return
.Las_RetB:
        mov     xl,  yl
        f64sexp xh, yh, a6      // take sign from status reg (0-B for example)
        leaf_return
// No size for the symbols here because the add and subtract interleave.

#endif /* L_addsubdf3 */


#ifdef L_muldf3

// extern double __muldf3 (double, double); 
        .global __muldf3
        .type __muldf3,@function
        .text
        .align 4
        
        // prepare to multiply
        // operand A is contained in a3/a2
        // operand B is contained in a5/a4
        // result  will be placed in a3/a2
__muldf3:       
	leaf_entry sp, 32
#if __XTENSA_CALL0_ABI__
	addi	sp, sp, -32
	s32i	a12, sp, 16
	s32i	a13, sp, 20
	s32i	a14, sp, 24
	s32i	a15, sp, 28
#endif
        f64cmpl a6, xl, yl
        f64cmph a6, xh, yh, MUL_OP
        bltz   a6, .Lmul_Simplex // result known already
        f64sig  a10, xh         // Remove sgn/exp & insert hidden bit
        f64sig  a11, yh         // Remove sgn/exp & insert hidden bit
// typically 6 instructions in preparation
        
// multiply mantissa portions
//|  bits[127:96] |  bits[95:64]  |  bits[63:32]  |   bits[31:0]  |
//+---------------+---------------+---------------+---------------+
//|               |               |muluh -, xl, yl|mull  -, xl, yl|
//+---------------+---------------+---------------+---------------+
//|               |muluh -,a10, yl|mull  -,a10, yl|               |
//+---------------+---------------+---------------+---------------+
//|               |muluh -,a11, xl|mull  -,a11, xl|               |
//+---------------+---------------+---------------+---------------+
//|muluh -,a10,a11|mull  -,a10,a11|               |               |
//+---------------+---------------+---------------+---------------+
//      a12              a13              a14            a15
// 
// 
#if XCHAL_HAVE_MUL32 &&  XCHAL_HAVE_MUL32_HIGH
        mull    a14, a10, yl    // bits[63:32] from lo of A-hi * B-lo
        mull    a15, a11, xl    // bits[63:32] from lo of B-hi * A-lo
        muluh   a9,  xl,  yl    // bits[63:32] from hi of A-lo * B-lo
        f64addc a14, a15, 1, 1  // 
        f64addc a14, a9,  1, 0  // a14 = bits[63:32] with C0, C1 in bit[64]
        muluh   a13, a10, yl    // bits[95:64] from hi of A-hi * B-lo
        muluh   a15, a11, xl    // bits[95:64] from hi of B-hi * A-lo
        mull    a9,  a10, a11   // bits[95:64] from lo of A-hi * B-hi
        f64addc a13, a15, 0, 1  // C0 output must = 0
        f64addc a13, a9,  0, 0  // a13 = bits[95:64] with C1 in bit[96]
        muluh   a12, a10, a11   // bits[127:96] from hi of A-hi * B-hi
        mull    a15, xl,  yl    // bits[31:0]   from lo of A-lo * B-lo
        f64addc a12, a12, 2, 0  // ignore dest value, use C0
                                // result in a12[9:0], a13, a14, a15
// 13 instructions in MUL32_HIGH
#else
#if XCHAL_HAVE_MUL16 || XCHAL_HAVE_MUL32
// doing 53x53bit multilication in 16x16bit chunks,
// note: since the MSBs are 0 in some additions the carry output is 0,
// and there is no need to propage it (meaning this is not complete 64x64 mul).         
// pp0..pp15 are partial products from 16x16 bit multiplications
// extract upper 16 bits
        extui   a12, yl, 16, 16  // a12 = B[31:16]
        extui    a8, xl, 16, 16  //  a8 = A[31:16]
        extui    a9, a11, 16, 16 //  a9 = B[63:48]
        extui    xh, a10, 16, 16 //  xh = A[63:48]
#if !XCHAL_HAVE_MUL16
#define do_mul mull
// with 32-bit multiplication the 16-msbs need to be cleared    
        extui    xl,  xl, 0, 16
        extui    yl,  yl, 0, 16
        extui   a11, a11, 0, 16
        extui   a10, a10, 0, 16
#else
// 16-bit multiplication ignores the 16MSBs, no need to clear them      
#define do_mul  mul16u  
#endif          
        do_mul   a7, xl, a12    // pp1  = B[31:16] * A[15:0]
        do_mul   yh, a8, yl     // pp4 = A[31:16] * B[15:0]
        do_mul  a15, xl, yl     // pp0 = A[15:0] * B[15:0]
        f64addc  yh, a7, 1, 0   // pp1+pp4, carry will need alignment
        slli     a7, yh, 16     // re-using a7, only pp1+pp4 needed
        f64addc a15, a7, 1, 1   // += (pp1+pp4)<<16, a15 now ready        
// a14 calculation (some parts of a13 too)
        do_mul  a14,  xl, a11    // pp2 = A[15:0] * B[47:32]
        extui    a7,  yh, 16, 16 // (pp1+pp4)>>16
        f64addc a14,  a7, 0, 1   // a14 += (pp1+pp4)>>16+carry1
        movi     yh,   0         // keep yh 0 during rest of calculation
        do_mul   a7,  yl, a10    // pp8 = A[47:32] * B[15:0]
        f64addc a13,  yh, 2, 1   // propage carry to a13
        f64addc a14,  a7, 1, 1   // += pp8
        do_mul   a7,  a8, a12    // pp5 = A[31:16] * A[31:16]
        f64addc a13, a13, 2, 1   // propage carry to a13
        f64addc a14,  a7, 1, 1   // +=pp5
        f64addc a13, a13, 2, 1   // propage carry to a13
        f64addc  a7, yh,  2, 0   // take carry0
        slli     a7, a7, 16      // and align it correctly
        f64addc a14, a7,  1, 0
        f64addc a13, a13, 2, 0   // propage carry to a13
// (pp3+pp6+pp9+pp12)
        do_mul   a7,  a9, xl     // pp3 = A[15:0] * B[63:48],  xl available
        do_mul   yl,  yl, xh     // pp12 = A[63:48] * B[15:0], reuse yl reg
        do_mul   xl, a11, a8     // pp6 = A[31:16] * B[47:32]
        add      a7,  a7, yl     // pp3+pp12 can't create carry out
        do_mul   yl, a10, a12    // pp9 = A[47:32] * B[31:16]
        f64addc  a7,  xl,  1, 0  // pp3+pp6+pp12
        f64addc  a7,  yl, 1, 1   // pp3+pp6+pp9+pp12
        f64addc  xl,  yh, 2, 0
        f64addc  xl,  xl, 2, 1   // sum up carries
        slli     xl,  xl, 16     // shift carries to correct alignment
        slli     yl,  a7, 16     // (pp3+pp6+pp9+pp12)<<16
        f64addc a14,  yl, 1, 0   // a14 += (pp3+pp6+pp9+pp12)<<16, a14 now complete
// a13 calculation continues    
        extui   yl,  a7, 16, 16 // (pp3+pp6+pp9+pp12)>>16
        f64addc a13,  yl, 0, 0  // a13 += (pp3+pp6+pp9+pp12)>>16 + carry0
        add     a13, a13, xl
        do_mul   a8,  a8,  a9   // pp7 = A[31:15] * B[63:48], a8 now reused (b)
        do_mul   xl,  xh, a12   // pp13 = A[63:48] * B[31:16], a12 now free (f)
        add     a13, a13, a8
        add     a13, a13, xl
// note: so far the carry output from a13 additions has always been 0
// some a12 calculations interleaved, start summing up carries to it     
        do_mul   a7, a10, a11   // pp10 = A[47:32] * B[47:32], a10 now free
        do_mul  a12,  xh,  a9   // a12 = pp15 = A[63:48] * B[63:48]
        f64addc a13,  a7,  1, 0 // a13 += p10, first possible carry output from a13 add
        do_mul   xl, a10,  a9   // pp11 = A[47:32] * B[63:48]
        do_mul  a10,  xh, a11   // pp14 = A[63:48] * B[47:32]
        f64addc a12, a12,  2, 0 // a12 += carry0
        add      xl,  xl, a10   // pp11+pp14  (can't create carry out)
        slli    a10,  xl, 16    // (pp11+pp14) << 16
        f64addc a13, a10,  1, 1 // a13 += (pp11+pp14) << 16
// a13 done, finish a12
        extui    xl,  xl, 16, 16 // (pp11+pp14) >> 16
        f64addc a12,  xl,  0, 1  // a12 += (pp11+pp14) >> 16 + carry1
// 55 instructions in MUL16 config      
// 59 instructions in MUL32 config      
// 67 instructions in MAC16 calculation
#else 
// MAC16 config, or no multiplier HW at all
// The main code is mostly the same for both cases,
// hide the differences with macros.
#if XCHAL_HAVE_MAC16
#define do_mul_ll(z,x,y) umul.aa.ll     x,y; \
                         rsr.acclo      z
#define do_mul_lh(z,x,y) umul.aa.lh     x,y; \
                         rsr.acclo      z
#define do_mul_hl(z,x,y) umul.aa.hl     x,y; \
                         rsr.acclo      z
#define do_mul_hh(z,x,y) umul.aa.hh     x,y; \
                         rsr.acclo      z
#else
// There software config is not enough registers to store high/low parts 
// in 16-bit parts separate required part when calling sub-routine
#define do_mul_ll(z,x,y) extui  a8, x, 0, 16; \
                         extui  a9, y, 0, 16; \
                         call0  __mul_sw; \
                         mov    z, a12
#define do_mul_lh(z,x,y) extui  a8, x, 0, 16; \
                         extui  a9, y, 16, 16; \
                         call0  __mul_sw; \
                         mov    z, a12
#define do_mul_hl(z,x,y) extui  a8, x, 16, 16; \
                         extui  a9, y,  0, 16; \
                         call0  __mul_sw; \
                         mov    z, a12
#define do_mul_hh(z,x,y) extui  a8, x, 16, 16; \
                         extui  a9, y, 16, 16; \
                         call0  __mul_sw; \
                         mov    z, a12
// save return address since SW routine called within   
        s32i    a0, a1, 0       
#endif 
        
// note: rsr operations in MAC16 scheduled such way that pipeline stalls minimized
        do_mul_hl(a7, yl,xl)        // pp1 = B[31:16] * A[15:0]
        do_mul_hl(a13, xl, yl)      // pp4 = A[31:16] * B[15:0]
        do_mul_ll(a15, xl, yl)      // pp0 = A[15:0] * B[15:0]
        f64addc    a13, a7, 1, 0    // pp1+pp4, carry will need alignment
        slli        a7,a13, 16      // re-using a7, only pp1+pp4 needed
        f64addc    a15, a7, 1, 1    // += (pp1+pp4)<<16, a15 now ready        
// a14 calculation (some parts of a13 too)
        do_mul_ll(a14, xl, a11)     // pp2 = A[15:0] * B[47:32]
        extui       a7, a13, 16, 16 // (pp1+pp4)>>16
        f64addc    a14,  a7, 0, 1   // a14 += (pp1+pp4)>>16, yh now free
        do_mul_ll(a7, a10, yl)      // pp8 = A[47:32] * B[15:0]
        do_mul_hh(a8, xl, yl)       // pp5 = A[31:16] * A[31:16]
        movi        yh,   0     
        f64addc    a13,  yh, 2, 1   // propage carry to a13
        f64addc    a14,  a7, 1, 1   // += pp8
        f64addc    a13, a13, 2, 1   // += carry1
        f64addc    a14,  a8, 1, 1   // +=pp5
        f64addc    a13, a13, 2, 1   // a13 += carry1
        f64addc     a7,  yh, 2, 0   // take carry0
        slli        a7,  a7,16      // and align it correctly
        f64addc    a14,  a7, 1, 0
        f64addc    a13, a13, 2, 0   // a13 += carry0
// (pp3+pp6+pp9+pp12)
#if XCHAL_HAVE_MAC16
// same thing as in else branch, in different order to optimize pipelining
        do_mul_lh(a7, xl, a11)      // pp3  = A[15:0] * B[63:48]
        do_mul_hl(a8, a10, yl)      // pp12 = A[63:48] * B[15:0]
        do_mul_hl(a9, xl, a11)      // pp6  = A[31:16] * B[47:32]
        add         a7,  a7, a8     // pp3+pp12 can't create carry out
        do_mul_lh(a8, a10,  yl)     // pp9 = A[47:32] * B[31:16]
        f64addc     a7,  a9, 1, 0   // pp3+pp6+pp12
        f64addc     a7,  a8, 1, 1   // pp3+pp6+pp9+pp12
#else   
        do_mul_hl(a7, a11, xl)      // pp3  = A[15:0] * B[63:48]
        do_mul_hl(a8, a10, yl)      // pp12 = A[63:48] * B[15:0]
        add         a7,  a7, a8     // pp3+pp12 can't create carry out
        do_mul_hl(a8, xl, a11)      // pp6  = A[31:16] * B[47:32]
        f64addc     a7,  a8, 1, 0   // pp3+pp6+pp12
        do_mul_lh(a8, a10,  yl)     // pp9 = A[47:32] * B[31:16]
        f64addc     a7,  a8, 1, 1   // pp3+pp6+pp9+pp12
        movi        yh, 0
#endif          
        f64addc     a9,  yh, 2, 0
        f64addc     a9,  a9, 2, 1     // sum up carries
        slli        a9,  a9, 16       // shift carries to correct alignment
        slli        a8,  a7, 16       // (pp3+pp6+pp9+pp12)<<16
        f64addc    a14,  a8, 1, 0     // a14 += (pp3+pp6+pp9+pp12)<<16, a14 now complete
// a13 calculation continues    
        extui      a8,  a7, 16, 16    // (pp3+pp6+pp9+pp12)>>16
        f64addc   a13,  a8, 0, 0      // a13 += (pp3+pp6+pp9+pp12)>>16 + carry0
        add       a13, a13, a9
        do_mul_hh(xl, a11, xl)        // pp7 = A[31:15] * B[63:48]
        do_mul_hh(yl, a10,  yl)       // pp13 = A[63:48] * B[31:16]
        add        a13, a13, xl
// note: so far the carry output from a13 additions has always been 0
// some a12 calculations interleaved, start summing up carries to it     
        do_mul_ll(a7, a10, a11)       // pp10 = A[47:32] * B[47:32]
        add        a13, a13, yl
        do_mul_hh(yl, a10, a11)       // a12 = pp15 = A[63:48] * B[63:48]
        f64addc    a13,  a7,  1, 0    // a13 += p10, first possible carry out from a13 add
        do_mul_hl(xl, a11, a10)       // pp11 = A[47:32] * B[63:48]
        do_mul_hl(a10, a10, a11)      // pp14 = A[63:48] * B[47:32]
        f64addc    a12,  yl,  2, 0    // a12 = pp15 + carry0
        add         a9,  xl, a10      // pp11+pp14  (can't create carry out)
        slli       a10,  a9, 16       // (pp11+pp14) << 16
        f64addc    a13, a10,  1, 1    // a13 += (pp11+pp14) << 16
// a13 done, finish a12
        extui       a9,  a9, 16, 16   // (pp11+pp14) >> 16
        f64addc    a12,  a9,  0, 1    // a12 += (pp11+pp14) >> 16 + carry1
#if !XCHAL_HAVE_MAC16
        // restore return address
        l32i    a0, a1, 0
#endif  
#endif // no MUL32/MUL16
#endif // no MUL32HIGH  
        bbsi.l a6, LongPost, .Lmul_lpost
        // align result
        src      a3, a12, a13   // position upper significand
        src      a2, a13, a14   // position lower significand
        src     a14, a14, a15   // position guard bit
        wf64r   a14, a15, 0     // copy guard/sticky to f64r
        f64rnd   xh, a3, a2, RND_SEEK  // check exp, round, etc
        rf64r    xl, 0          // restore low part of result from F64R
#if __XTENSA_CALL0_ABI__
	l32i	a12, sp, 16
	l32i	a13, sp, 20
	l32i	a14, sp, 24
	l32i	a15, sp, 28
	addi	sp, sp, 32
#endif
        leaf_return                    

// The logical location for .Lmul_Simplex
	
.Lmul_DorN:
// denormal or normal output, 
// exp is 0, but might increase to 1, shift in both cases same
// (can't use RND_SH_SEEK rounding since shift don't change)
// No dernomal input allowed/possible since in those cases exp can't 
// increase to 1 because denormal mantissa has MSB in lower position.
// Default SAR value of 20 is ok for us
        
// exp++ if grown to normal number (bit-9 in a12 set)
        extui    a3, a12, 9, 1
        add      a6, a6, a3
        wur.f64s a6
        src       a3, a12, a13
        src      a13, a13, a14
        sll      a14, a14 
        wf64r    a14, a15, 0
        f64rnd    xh, a3, a13, RND_SH_DOWN
        rf64r     xl, 0
#if __XTENSA_CALL0_ABI__
	l32i	a12, sp, 16
	l32i	a13, sp, 20
	l32i	a14, sp, 24
	l32i	a15, sp, 28
	addi	sp, sp, 32
#endif
        leaf_return

// This code logically belongs below (in the location marked, ".Lmul_ee"
// but branches fit better when it is here.
	
// 16 cycles from rr
.Lmul_ee:               
        src      a3, a12, a13 
        src      a2, a13, a14
        src      a8, a14, a15   // outed bits to sticky
        wf64r    a8, a15,  0    // guard, sticky bits to accu
        f64rnd   xh, a3,  a2, RND_SH_DOWN
        rf64r    xl,   0
#if __XTENSA_CALL0_ABI__
	l32i	a12, sp, 16
	l32i	a13, sp, 20
	l32i	a14, sp, 24
	l32i	a15, sp, 28
	addi	sp, sp, 32
#endif
        leaf_return

// 15 extra cycles, 8 if compared to normal case        

// input or result is denormal, longer post normalization required
// 3 cases here:        
//   1:  Exp candidate is 0, result denormal or normal (if mul res high)
//   2:  Result is guaranteed to be denormal (negative exp cand).
//       Shift as much is required to get exp to 0.
//   3:  Result is normal, but one of inputs was denormal. Need
//       to find 1st bit, and adjust exponent based on that.
//       Note: result can become dernomal if shifting large and exp small
.Lmul_lpost:
        bbsi.l  a6, DorNOut, .Lmul_DorN // jump to case 1 handler
// first abs(extract exp candidate) to a4, tells how much to shift
// exp cand always negative in this branch, convert to positive
#if XCHAL_HAVE_SEXT
        sext     a4, a6, 10
#else
        slli     a4, a6, 21     
        srai     a4, a4, 21
#endif  
        abs      a4, a4
// make new status reg to a5, the result exp is 0 (denormal)
        _movi    a7, 0xFFFFF800
        and      a5, a6, a7    // clear exp, keep sign bit
        wur.f64s a5            // update f64s to contain new exp cand
                
// if the input was denormal and result has positive exp candidate
        bbci.l a6, 10, .Lmul_denin  // jump to case 2 handler

// Case where result is guaranteed to be denormal (negative exp)
// Shift right until exp is 0, no need to look for 1st bit.   
// 27 cycles worst case (starting from mul_lpost)
.Lmul_denout:  
        
// offset from res[8] to res[20], shift at least this much
        addi     a7, a4, 19
        bbsi.l  a6, Adenorm, .Lmul_rr
        bbsi.l  a6, Bdenorm, .Lmul_rr
// ++ shift if both inputs regular
        addi     a7, a7, 1
.Lmul_rr:               
        ssr      a7
        bltui   a7, 32, .Lmul_ee
        bgeui   a7, 64, .Lmul_dd
// if shift > 32, a12:a13 has mantissa
        srl       a3, a12
        src       a2, a12, a13
        src       a8, a13, a14
        or       a15, a15, a14  // sum sticky bits to a15
        wf64r     a8, a15,  0   // guard,sticky bits to accu
        f64rnd    xh,  a3,  a2, RND_SH_DOWN
        rf64r     xl,   0
#if __XTENSA_CALL0_ABI__
	l32i	a12, sp, 16
	l32i	a13, sp, 20
	l32i	a14, sp, 24
	l32i	a15, sp, 28
	addi	sp, sp, 32
#endif
        leaf_return

// 13 cycles from rr
.Lmul_dd:
        // shift larger than 64, a12 has the bits for mantissa (if any)
        // a13:a14:a15 are guard/sticky
        movi     a3, 0
        srl      a2, a12
        src      a8, a12, a13
        or      a15, a15, a14   // sum all sticky bits to a15
        or      a15, a15, a13
        wf64r    a8, a15,  0    // guard,sticky bits to accu
        f64rnd   xh, a3,  a2, RND_SH_DOWN
        rf64r    xl,  0
#if __XTENSA_CALL0_ABI__
	l32i	a12, sp, 16
	l32i	a13, sp, 20
	l32i	a14, sp, 24
	l32i	a15, sp, 28
	addi	sp, sp, 32
#endif
        leaf_return

// Logical location for .Lmul_ee

// 13 cycles from rr
        
.Lmul_denin:            
// One of inputs was denormal, seek the first bit
// then check how much exp needs adjustement, result might be normal or denormal
        f64norm a8, a12, a13, 1 // is the bit in a12:13 ?
        bgeui  a8, 32, .Lmul_1
// shift result candidate in-place, (a12:a13 will be mantissa a14:15 guard/sticky)
        src      a12, a12, a13
        src      a13, a13, a14
        src      a14, a14, a15
        sll      a15, a15 
// normal msb position in mul is different than f64norm assumes, fix difference
        addi     a8, a8, -12
        j       .Lmul_aa        // we are done shifting
// first bit not in the first 32 bits, check next 32
.Lmul_1:        
        f64norm  a8, a13, a14, 1 // is it in a13:14
        bgeui   a8,  32, .Lmul_2
        src     a12, a13, a14
        src     a13, a14, a15
        sll     a14, a15
        movi    a15, 0
        addi     a8, a8, 20
        j       .Lmul_aa        // we are done now
// last bits to check
.Lmul_2:                
        f64norm  a8, a14, a15, 1
        src     a12, a14, a15
        sll     a13, a15
        movi    a14, 0
        movi    a15, 0
        addi     a8, a8, 52
.Lmul_aa:

// check is exponent going below 0, and reduce shifting if so

// new exp = old_exp - shift    
        sub     a11, a4, a8
// exp > 0, regular result
        bgez   a11, .Lmul_xx
// exp < -54, underflow, Can't happen
// exp 0, or negative, denormal result, shift right until exp is 0
        abs     a9, a11         // shift until exp = 0
        addi    a9, a9, -1      // -- since f64rnd does >> 1
        ssr     a9
// shift right to get mantissa to agree with exponent   
        bltui    a9, 32, .Lmul_smallNorm
        or      a15, a15, a13   // add to sticky bits before thrown away
        mov     a13, a12
        movi    a12, 0
.Lmul_smallNorm:        
        src     a2, a12, a13
        srl     a3, a12
// add shifted out bits to sticky bits
        src      a7, a13, a14
        or      a15, a15, a7

        wf64r   a14, a15, 0   // put sticky bits to accu for f64rnd to use
        f64rnd   xh,  a3, a2, RND_SH_DOWN
        rf64r    xl, 0
#if __XTENSA_CALL0_ABI__
	l32i	a12, sp, 16
	l32i	a13, sp, 20
	l32i	a14, sp, 24
	l32i	a15, sp, 28
	addi	sp, sp, 32
#endif
        leaf_return
        
// denormal * regular = regular, shifting already done, adjust only exponent
.Lmul_xx:
// new exp = old_exp - shift
// take sign from old status, exp from a11
        addi     a11, a11,  1  // 1 for denorm input
        or        a5,  a5, a11  // combine sign and exponent
        wf64r    a14, a15, 0
        wur.f64s a5
        f64rnd    xh, a12, a13, RND_ONLY
        rf64r     xl, 0
#if __XTENSA_CALL0_ABI__
	l32i	a12, sp, 16
	l32i	a13, sp, 20
	l32i	a14, sp, 24
	l32i	a15, sp, 28
	addi	sp, sp, 32
#endif
        leaf_return

#if !XCHAL_HAVE_MUL16 && !XCHAL_HAVE_MUL32 && !XCHAL_HAVE_MAC16
        /* For Xtensa processors with no multiply hardware, this simplified
           version of mulsi3 is used for multiplying 16-bit chunks of
           the floating-point mantissas.  It uses a custom ABI: the inputs
           are passed in a8 and a9, the result is returned in a12, and
           xh and yh are clobbered.  */
// note: If you known that MSBs (4 or more) of one operand are 0, then place it to a8,
//       this activates early exit.
	
        .align  4
	.local __mul_sw
__mul_sw:
        movi    a12, 0
.Lmul_lp:
        add      yh, a9, a12
        extui    xh, a8, 0, 1
        movnez  a12, yh, xh

        addx2    yh, a9, a12
        extui    xh, a8, 1, 1
        movnez  a12, yh, xh

        addx4    yh, a9, a12
        extui    xh, a8,  2, 1
        movnez  a12, yh,  xh

        addx8    yh, a9, a12
        extui    xh, a8,  3, 1
        movnez  a12, yh,  xh

        srli    a8, a8, 4
        slli    a9, a9, 4
        bnez    a8, .Lmul_lp
        ret
#endif  

	.align 4

// This code more logically goes in the place marked with the comment
// "The logical place for .Lmul_Simplex", but branches fit better here

// 8 instructions in typical post normalization

// typical case: 29 cycles for mul32high, 72 cycles mul16, 76 cycles mul32, 
//               83 cycles mac16, ~1200 cycles for non-multiplier config
// best case:    14 (NaN/Zero/Inf/Immediate overflow) cycles
// worst case:   xx cycles (rare, denormal input/output)
        
.Lmul_Simplex:
// Zero/Nan/Inf/guaranteed overflow (or return B)
        bbci.l  a6, RetB, .Lmul_exit
// or A is power of 2, then return B with adjusted exponent
        mov      xl, yl
        mov      xh, yh
.Lmul_exit:
        f64rnd   xh, xh, xl, RND_ONLY
        rf64r    xl, 0       
#if __XTENSA_CALL0_ABI__
	l32i	a12, sp, 16
	l32i	a13, sp, 20
	l32i	a14, sp, 24
	l32i	a15, sp, 28
	addi	sp, sp, 32
#endif
        leaf_return
        

	.size __muldf3, . - __muldf3
#endif  

#ifdef L_divdf3



// extern double __divdf3(double x, double y);

        .text
        .align 4
        .global __divdf3
        .type __divdf3,@function
        
// other registers used:
// a6 status
// a7 iter count
// a8-a11 in denormal input handling    

__divdf3:
        leaf_entry    a1, 16
        f64cmpl  a6, xl, yl
        f64cmph  a6, xh, yh, DIV_OP
        bltz    a6, .Ldiv_Others
        rsr.sar  a7             // get iteration count from SAR
.Ldiv_rerun:
        f64sig   xh, xh         // clear exp, set hidden bit
        f64sig   yh, yh
        wf64r    xh, xl,0       // accum = x
        movi     xh, 0          // 
        wur.f64s xh             // clear result collection reg
// loop in 32-bit mode, 1-cycle per iteration
// calculate low and high part separately
        
// Results to upper part first, typically 20 or something iterations
// but can be other too for denormals, when second loop is not executed
// reusing xl as tmp-regs

// calculate iteration count for first loop
// if (iter_count > 32) 
//   first_loop_cnt = iter_count-32
// else
//   first_loop_cnt = iter_count        
        addi    a7, a7, -1   // reduce iter count since 1st iter not in loop
        addi    xl, a7, -32  // 
        movltz  xl, a7,  xl 
// first iteration separately, without shifting 
        f64iter yh, yh, yl, DIVI, NO_SHIFT
#if XCHAL_HAVE_LOOPS   
        loopgtz xl, .Ldiv_ita
        f64iter yh, yh, yl, DIVI, DO_SHIFT
.Ldiv_ita:
#else
        blti   xl, 1, .Ldiv_bp    // bypass loop if no iterations to do
.Ldiv_ita:
        f64iter yh, yh, yl, DIVI, DO_SHIFT
        addi    xl, xl, -1
        bnez    xl, .Ldiv_ita
//note:  can't do simple loop unrolling to speed up since number of iterations change  
.Ldiv_bp:       
#endif                  
// if 2nd loop skipped result goes to lower part        
        blti  a7, 32, .Ldiv_bp_loop2
        rur.f64s xh         // fetch result high part from collection reg

// second iteration, results to lower part, always 32 iterations
// but not always executed
// execute loop with 2 iterations per loop round, 1 pipeline bubble less
#if XCHAL_HAVE_LOOPS   
        movi     xl, 16
        loopgtz  xl, .Ldiv_itb
        f64iter  yh, yh, yl, DIVI, DO_SHIFT
        f64iter  yh, yh, yl, DIVI, DO_SHIFT
.Ldiv_itb:
#else
        movi     xl, 7
.Ldiv_itb:
// do 4 bits per loop round, reduce loop overhead (4 cycles per loop)   
        f64iter  yh, yh, yl, DIVI, DO_SHIFT
        f64iter  yh, yh, yl, DIVI, DO_SHIFT
        f64iter  yh, yh, yl, DIVI, DO_SHIFT
        f64iter  yh, yh, yl, DIVI, DO_SHIFT
        addi     xl, xl, -1
        bgez   xl, .Ldiv_itb
#endif
.Ldiv_bp_loop2: 
        rur.f64s xl       // fetch result lower part
        wur.f64s a6       // restore status bits for round/exp setting
// division done, fix exponent and sign, round and exit
        f64rnd  xh, xh, xl, RND_SH_DOWN
        rf64r   xl,0
        leaf_return
// typical case something like 78 cycles with 55 iterations
        
///////////////////// SPECIAL CASE HANDLING ////////////////////// 
// potential underflow, exp <= 0 (result denormal or zero)
// adjust iteration count
.Ldiv_puf:
// extract exponent from status[10:0] (as signed since 0 or negative)
#if XCHAL_HAVE_SEXT
        sext    a8, a6, 10
#else
        slli    a8, a6, 21
        srai    a8, a8, 21
#endif          
// speed optimization, if exp < -53 exit immediatly.
// No point normalizing X and calculating new status since result is 0
        addi    a7, a8, 53
        bltz   a7, .Ldiv_mkZero // guaranteed underflow
// oops, X is denormal, fix it first, come back later
// Note: do this after quaranteed underflow checking    
        bbsi.l a6, Adenorm, .Ldiv_denorm
.Ldiv_puf_back:         
// adjust iteration count, when result is denormal do enough iterations
// to bring first bit to location indicated by result exponent  
        rsr.sar a7               // iter-count to a7
// iter_count += (exp_new-1)
        add     a7, a7, a8
        addi    a7, a7, -1
// recreate status (set exp=0, keep sign in status[11])
        srli    a6, a6, 11 
        slli    a6, a6, 11
// a7 has updated iteration count, a6 updated status, main handler will do the rest 
        bgei a7, 1, .Ldiv_rerun
// assuming of course that there is some iterations to do 
// if not fall-through to mkZero

// create Zero with correct result sign
.Ldiv_mkZero:
        _movi   xl,0
        _movi   xh, 0xFFFFF800
        and     a6, xh, a6      // clear exp, keep sign
        f64sexp xh, xl, a6
        leaf_return

.Ldiv_Others:
        bbci.l a6, Simplex, .Ldiv_complex
// simple case, quick exit, return Nan/Inf/0 (or X with exp change)
        f64rnd  xh, xh, xl, RND_ONLY
        rf64r   xl,0
        leaf_return
.Ldiv_complex:          
        bbsi.l a6, PUF, .Ldiv_puf
        
// fall-through to denormal input handler

// denormal handling, slow and painful
.Ldiv_denorm:
// first separate exponents to a8, a9
        extui   a8, xh, 20, 11
        extui   a9, yh, 20, 11
        movi    a10, 1                  // for future use
// if X is regular, skip the mangling
        bbci.l a6, Adenorm, .Ldiv_fixY       
        
// X is denorm, move first set bit to hidden bit's locations, and adjust exponent
.Ldiv_fixX:   
        f64norm a7, xh, xl, 1
        bltui   a7, 32, .Ldiv_xSmallShift
// shift > 32, optional step
        mov     xh, xl      
        movi    xl, 0
.Ldiv_xSmallShift:
        src     xh, xh, xl      // shift low part, always executed
        sll     xl, xl
// adjust exponent of X based on how much X was shifted
// since X was denormal (a8 = 0), the new_exp = 1-shift
        sub     a8, a10, a7
        
// now the xh:xl looks like normal number, exp looks like 1, sign bit is lost
// then let's does check Y need adjustment

// Y regular, skip      
        bbci.l a6, Bdenorm, .Ldiv_uf_of
                        
.Ldiv_fixY:
// Y is denorm, move first set bit to hidden bit's locations, and adjust exponent
        f64norm a7, yh, yl, 1
        bltui   a7, 32, .Ldiv_ySmallShift
// shift > 32, optional step
        mov     yh, yl      
        movi    yl, 0
.Ldiv_ySmallShift:
        src     yh, yh, yl      // shift low part, always executed
        sll     yl, yl
// adjust exponent of Y based on how much Y was shifted
// since Y was denormal, the new_exp = 1-shift
        sub     a9, a10, a7
// now the yh:yl looks like normal number, exp looks like 1, sign bit is lost
        
// time to make the final exponent and check for overflow or underflow
.Ldiv_uf_of:
// calculate new exponent
// exp_new = exp(x) - exp(y) + 1023
// note using 1024 (+1) since it helps to make code smaller/faster
        sub     a10, a8, a9
        addmi   a10, a10, 1024
// update sar to contain iter-count, a9 don't care
// Note: mess up F64S, but it will be restored from a6
        f64cmpl a9, xl, yl
        f64cmph a9, xh, yh, DIV_OP
        rsr.sar a7      
// a7 is 55 or 54, if 55 then exp-- needed
        addi    a11, a7, -54
        sub     a10, a10, a11   // sub 1 or 0
        
// exp > 2046, overflow 
        addmi   a9, a10, -2048
        bgez    a9, .Ldiv_mkInf

// correct bias from 1024 to 1023, a8 exp candidate (signed form)
        addi    a8, a10, -1
// recreate status reg, take sign from status[11]
        extui   a9, a6, rSign, 1   // extract sign-bit
        slli    a9, a9, rSign      // move to correct position
// clear MSBs in exponent
        extui   a11, a8, 0, 11
// combine sign+exp     
        or      a6, a9, a11
// exp and sign now ok, note: other status bits not valid, but not used in rerun/puf
// go to potential underflow, or main routine
        addi    a9, a8, -1   // -- since bgez has longer range than blti x,1
        bgez   a9, .Ldiv_rerun
// exp <= 0, back to potential underflow routine
        j       .Ldiv_puf_back
        
.Ldiv_mkInf:
// create Inf with correct result sign
        _movi   xl, 0
        _movi   xh, 0x7ff
        or      a6, xh, a6      // keep sign, set exp for inf
        f64sexp xh, xl, a6
        leaf_return
	.size __divdf3, . - __divdf3
        
#endif // L_divdf3

#ifdef L_cmpdf2

// less than, less than or equal to, equal,
// greater than, greater than or equal, not equal
//
// The compiler expects the routines to return
//   values that are less than zero, zero, or greater than zero.
//   This requires a little extra work after the 2-instruction sequence
//  __ltdf2 :	 (a < b)  ? -1 :	 0
//  __ledf2 :	 (a <= b) ? -1 :	 1  (return 0x80000001 or 1)
//  __eqdf2 :	 (a == b) ?  0 :	 1
//  __gtdf2 :	 (a > b)  ?  1 :	 0
//  __gedf2 :	 (a >= b) ?  1 :	-1
//  __nedf2 :	 (a != b) ?  1 :	 0
//   Since the function __nedf2 returns the same value as
//    __eqdf2, we use the faster one by default.
//   For testing, the F64CMPH EQ_OP op, compile with -DTEST_DFPEMU_EQ=1
//
// For performance-critical compares, the 2-instruction sequence
//        f64cmpl xl,  xl,  yl
//        f64cmph a2,  xh,  yh, LT_OP
// can be manually inlined at its use.

// LESS_THAN        
        // operand A is contained in xh/xl 
        // operand B is contained in yh/yl 
        // result  will be placed in a2 
        .global __ltdf2
        .type __ltdf2,@function
        .align 4
        
__ltdf2:
	leaf_entry   a1, 16
        f64cmpl xl,  xl,  yl
        f64cmph a2,  xh,  yh, LT_OP
	// return -1 when a2 == 1 and 0 when a2 == 0
	neg a2,a2
        leaf_return
	.size	__ltdf2, . - __ltdf2

// LESS_OR_EQUAL	
        // operand A is contained in xh/xl 
        // operand B is contained in yh/yl 
        // result  will be placed in a2 
        .global __ledf2
        .type __ledf2,@function
        .align 4

__ledf2:  
        leaf_entry   a1,  16
        f64cmpl xl,  xl,  yl
        f64cmph a2,  xh,  yh, LE_OP
	// return negative (0x80000000) when a2 == 1 and 1 when a2 == 0
	slli    a2,  a2,  31
	addi    a2,  a2,  1
        leaf_return
	.size	__ledf2, . - __ledf2

#if TEST_DFPEMU_EQ
// EQUAL	
        // operand A is contained in xh/xl 
        // operand B is contained in yh/yl 
        // result  will be placed in a2 
        .global __eqdf2
        .type __eqdf2,@function
        .align 4
__eqdf2:  
        leaf_entry   a1,  16
        f64cmpl xl,  xl,  yl
        f64cmph a2,  xh,  yh, EQ_OP
	// return 0 when a2 == 1, 1 when a2 == 0
        movi a3,1
	xor a2,a2,a3
        leaf_return
	.size	__eqdf2, . - __eqdf2
#endif
	
// GREATER THAN	
        // operand A is contained in xh/xl 
        // operand B is contained in yh/yl 
        // result  will be placed in a2 
        .global __gtdf2
        .type __gtdf2,@function
        .align 4
        
__gtdf2:  
        leaf_entry   a1,  16
        f64cmpl xl,  xl,  yl
        f64cmph a2,  xh,  yh, GT_OP
        leaf_return
	.size	__gtdf2, . - __gtdf2

// GREATER OR EQUAL	
        // operand A is contained in xh/xl 
        // operand B is contained in yh/yl 
        // result  will be placed in a2 
        .global __gedf2
        .type __gedf2,@function
        .align 4

__gedf2:  
        leaf_entry   a1,  16
        f64cmpl xl,  xl,  yl
        f64cmph a2,  xh,  yh, GE_OP
	// return 1 when a2==1 and -1 when a2 == 0
	slli    a2,  a2,  1
	addi    a2,  a2, -1
        leaf_return
	.size	__gedf2, . - __gedf2

        // operand A is contained in xh/xl 
        // operand B is contained in yh/yl 
        // result  will be placed in a2 
        .global __nedf2
        .type __nedf2,@function
        .align 4

// NOT EQUAL
#if !TEST_DFPEMU_EQ
        .global __eqdf2
        .type __eqdf2,@function
__eqdf2:
#endif
__nedf2:  
        leaf_entry   a1,  16
        f64cmpl xl,  xl,  yl
        f64cmph a2,  xh,  yh, NE_OP
        leaf_return
	.size	__nedf2, . - __nedf2
#if !TEST_DFPEMU_EQ
	.size	__eqdf2, . - __eqdf2
#endif
#endif /* L_compdf2 */

#else /* !XCHAL_HAVE_DFP_accel */

#ifdef L_addsubdf3
	/* Addition */
__adddf3_aux:
	
	/* Handle NaNs and Infinities.  (This code is placed before the
	   start of the function just to keep it in range of the limited
	   branch displacements.)  */

.Ladd_xnan_or_inf:
	/* If y is neither Infinity nor NaN, return x.  */
	bnall	yh, a6, .Ladd_return_nan_or_inf
	/* If x is a NaN, return it.  Otherwise, return y.  */
	slli	a7, xh, 12
	or	a7, a7, xl
	bnez	a7, .Ladd_return_nan

.Ladd_ynan_or_inf:
	/* Return y.  */
	mov	xh, yh
	mov	xl, yl

.Ladd_return_nan_or_inf:
	slli	a7, xh, 12
	or	a7, a7, xl
	bnez	a7, .Ladd_return_nan
	leaf_return

.Ladd_return_nan:
	movi	a4, 0x80000	/* make it a quiet NaN */
	or	xh, xh, a4
	leaf_return

.Ladd_opposite_signs:
	/* Operand signs differ.  Do a subtraction.  */
	slli	a7, a6, 11
	xor	yh, yh, a7
	j	.Lsub_same_sign

	.align	4
	.global	__adddf3
	.type	__adddf3, @function
__adddf3:
	leaf_entry sp, 16
	movi	a6, 0x7ff00000

	/* Check if the two operands have the same sign.  */
	xor	a7, xh, yh
	bltz	a7, .Ladd_opposite_signs

.Ladd_same_sign:	
	/* Check if either exponent == 0x7ff (i.e., NaN or Infinity).  */
	ball	xh, a6, .Ladd_xnan_or_inf
	ball	yh, a6, .Ladd_ynan_or_inf

	/* Compare the exponents.  The smaller operand will be shifted
	   right by the exponent difference and added to the larger
	   one.  */
	extui	a7, xh, 20, 12
	extui	a8, yh, 20, 12
	bltu	a7, a8, .Ladd_shiftx

.Ladd_shifty:
	/* Check if the smaller (or equal) exponent is zero.  */
	bnone	yh, a6, .Ladd_yexpzero

	/* Replace yh sign/exponent with 0x001.  */
	or	yh, yh, a6
	slli	yh, yh, 11
	srli	yh, yh, 11

.Ladd_yexpdiff:
	/* Compute the exponent difference.  Optimize for difference < 32.  */
	sub	a10, a7, a8
	bgeui	a10, 32, .Ladd_bigshifty
	
	/* Shift yh/yl right by the exponent difference.  Any bits that are
	   shifted out of yl are saved in a9 for rounding the result.  */
	ssr	a10
	movi	a9, 0
	src	a9, yl, a9
	src	yl, yh, yl
	srl	yh, yh

.Ladd_addy:
	/* Do the 64-bit addition.  */
	add	xl, xl, yl
	add	xh, xh, yh
	bgeu	xl, yl, 1f
	addi	xh, xh, 1
1:
	/* Check if the add overflowed into the exponent.  */
	extui	a10, xh, 20, 12
	beq	a10, a7, .Ladd_round
	mov	a8, a7
	j	.Ladd_carry

.Ladd_yexpzero:
	/* y is a subnormal value.  Replace its sign/exponent with zero,
	   i.e., no implicit "1.0", and increment the apparent exponent
	   because subnormals behave as if they had the minimum (nonzero)
	   exponent.  Test for the case when both exponents are zero.  */
	slli	yh, yh, 12
	srli	yh, yh, 12
	bnone	xh, a6, .Ladd_bothexpzero
	addi	a8, a8, 1
	j	.Ladd_yexpdiff

.Ladd_bothexpzero:
	/* Both exponents are zero.  Handle this as a special case.  There
	   is no need to shift or round, and the normal code for handling
	   a carry into the exponent field will not work because it
	   assumes there is an implicit "1.0" that needs to be added.  */
	add	xl, xl, yl
	add	xh, xh, yh
	bgeu	xl, yl, 1f
	addi	xh, xh, 1
1:	leaf_return

.Ladd_bigshifty:
	/* Exponent difference > 64 -- just return the bigger value.  */
	bgeui	a10, 64, 1b

	/* Shift yh/yl right by the exponent difference.  Any bits that are
	   shifted out are saved in a9 for rounding the result.  */
	ssr	a10
	sll	a11, yl		/* lost bits shifted out of yl */
	src	a9, yh, yl
	srl	yl, yh
	movi	yh, 0
	beqz	a11, .Ladd_addy
	or	a9, a9, a10	/* any positive, nonzero value will work */
	j	.Ladd_addy

.Ladd_xexpzero:
	/* Same as "yexpzero" except skip handling the case when both
	   exponents are zero.  */
	slli	xh, xh, 12
	srli	xh, xh, 12
	addi	a7, a7, 1
	j	.Ladd_xexpdiff

.Ladd_shiftx:
	/* Same thing as the "shifty" code, but with x and y swapped.  Also,
	   because the exponent difference is always nonzero in this version,
	   the shift sequence can use SLL and skip loading a constant zero.  */
	bnone	xh, a6, .Ladd_xexpzero

	or	xh, xh, a6
	slli	xh, xh, 11
	srli	xh, xh, 11

.Ladd_xexpdiff:
	sub	a10, a8, a7
	bgeui	a10, 32, .Ladd_bigshiftx
	
	ssr	a10
	sll	a9, xl
	src	xl, xh, xl
	srl	xh, xh

.Ladd_addx:
	add	xl, xl, yl
	add	xh, xh, yh
	bgeu	xl, yl, 1f
	addi	xh, xh, 1
1:
	/* Check if the add overflowed into the exponent.  */
	extui	a10, xh, 20, 12
	bne	a10, a8, .Ladd_carry

.Ladd_round:
	/* Round up if the leftover fraction is >= 1/2.  */
	bgez	a9, 1f
	addi	xl, xl, 1
	beqz	xl, .Ladd_roundcarry

	/* Check if the leftover fraction is exactly 1/2.  */
	slli	a9, a9, 1
	beqz	a9, .Ladd_exactlyhalf
1:	leaf_return

.Ladd_bigshiftx:
	/* Mostly the same thing as "bigshifty"....  */
	bgeui	a10, 64, .Ladd_returny

	ssr	a10
	sll	a11, xl
	src	a9, xh, xl
	srl	xl, xh
	movi	xh, 0
	beqz	a11, .Ladd_addx
	or	a9, a9, a10
	j	.Ladd_addx

.Ladd_returny:
	mov	xh, yh
	mov	xl, yl
	leaf_return

.Ladd_carry:	
	/* The addition has overflowed into the exponent field, so the
	   value needs to be renormalized.  The mantissa of the result
	   can be recovered by subtracting the original exponent and
	   adding 0x100000 (which is the explicit "1.0" for the
	   mantissa of the non-shifted operand -- the "1.0" for the
	   shifted operand was already added).  The mantissa can then
	   be shifted right by one bit.  The explicit "1.0" of the
	   shifted mantissa then needs to be replaced by the exponent,
	   incremented by one to account for the normalizing shift.
	   It is faster to combine these operations: do the shift first
	   and combine the additions and subtractions.  If x is the
	   original exponent, the result is:
	       shifted mantissa - (x << 19) + (1 << 19) + (x << 20)
	   or:
	       shifted mantissa + ((x + 1) << 19)
	   Note that the exponent is incremented here by leaving the
	   explicit "1.0" of the mantissa in the exponent field.  */

	/* Shift xh/xl right by one bit.  Save the lsb of xl.  */
	mov	a10, xl
	ssai	1
	src	xl, xh, xl
	srl	xh, xh

	/* See explanation above.  The original exponent is in a8.  */
	addi	a8, a8, 1
	slli	a8, a8, 19
	add	xh, xh, a8

	/* Return an Infinity if the exponent overflowed.  */
	ball	xh, a6, .Ladd_infinity
	
	/* Same thing as the "round" code except the msb of the leftover
	   fraction is bit 0 of a10, with the rest of the fraction in a9.  */
	bbci.l	a10, 0, 1f
	addi	xl, xl, 1
	beqz	xl, .Ladd_roundcarry
	beqz	a9, .Ladd_exactlyhalf
1:	leaf_return

.Ladd_infinity:
	/* Clear the mantissa.  */
	movi	xl, 0
	srli	xh, xh, 20
	slli	xh, xh, 20

	/* The sign bit may have been lost in a carry-out.  Put it back.  */
	slli	a8, a8, 1
	or	xh, xh, a8
	leaf_return

.Ladd_exactlyhalf:
	/* Round down to the nearest even value.  */
	srli	xl, xl, 1
	slli	xl, xl, 1
	leaf_return

.Ladd_roundcarry:
	/* xl is always zero when the rounding increment overflows, so
	   there's no need to round it to an even value.  */
	addi	xh, xh, 1
	/* Overflow to the exponent is OK.  */
	leaf_return


	/* Subtraction */
__subdf3_aux:
	
	/* Handle NaNs and Infinities.  (This code is placed before the
	   start of the function just to keep it in range of the limited
	   branch displacements.)  */

.Lsub_xnan_or_inf:
	/* If y is neither Infinity nor NaN, return x.  */
	bnall	yh, a6, .Lsub_return_nan_or_inf

.Lsub_return_nan:
	/* Both x and y are either NaN or Inf, so the result is NaN.  */
	movi	a4, 0x80000	/* make it a quiet NaN */
	or	xh, xh, a4
	leaf_return

.Lsub_ynan_or_inf:
	/* Negate y and return it.  */
	slli	a7, a6, 11
	xor	xh, yh, a7
	mov	xl, yl

.Lsub_return_nan_or_inf:
	slli	a7, xh, 12
	or	a7, a7, xl
	bnez	a7, .Lsub_return_nan
	leaf_return

.Lsub_opposite_signs:
	/* Operand signs differ.  Do an addition.  */
	slli	a7, a6, 11
	xor	yh, yh, a7
	j	.Ladd_same_sign

	.align	4
	.global	__subdf3
	.type	__subdf3, @function
__subdf3:
	leaf_entry sp, 16
	movi	a6, 0x7ff00000

	/* Check if the two operands have the same sign.  */
	xor	a7, xh, yh
	bltz	a7, .Lsub_opposite_signs

.Lsub_same_sign:	
	/* Check if either exponent == 0x7ff (i.e., NaN or Infinity).  */
	ball	xh, a6, .Lsub_xnan_or_inf
	ball	yh, a6, .Lsub_ynan_or_inf

	/* Compare the operands.  In contrast to addition, the entire
	   value matters here.  */
	extui	a7, xh, 20, 11
	extui	a8, yh, 20, 11
	bltu	xh, yh, .Lsub_xsmaller
	beq	xh, yh, .Lsub_compare_low

.Lsub_ysmaller:
	/* Check if the smaller (or equal) exponent is zero.  */
	bnone	yh, a6, .Lsub_yexpzero

	/* Replace yh sign/exponent with 0x001.  */
	or	yh, yh, a6
	slli	yh, yh, 11
	srli	yh, yh, 11

.Lsub_yexpdiff:
	/* Compute the exponent difference.  Optimize for difference < 32.  */
	sub	a10, a7, a8
	bgeui	a10, 32, .Lsub_bigshifty
	
	/* Shift yh/yl right by the exponent difference.  Any bits that are
	   shifted out of yl are saved in a9 for rounding the result.  */
	ssr	a10
	movi	a9, 0
	src	a9, yl, a9
	src	yl, yh, yl
	srl	yh, yh

.Lsub_suby:
	/* Do the 64-bit subtraction.  */
	sub	xh, xh, yh
	bgeu	xl, yl, 1f
	addi	xh, xh, -1
1:	sub	xl, xl, yl

	/* Subtract the leftover bits in a9 from zero and propagate any
	   borrow from xh/xl.  */
	neg	a9, a9
	beqz	a9, 1f
	addi	a5, xh, -1
	moveqz	xh, a5, xl
	addi	xl, xl, -1
1:
	/* Check if the subtract underflowed into the exponent.  */
	extui	a10, xh, 20, 11
	beq	a10, a7, .Lsub_round
	j	.Lsub_borrow

.Lsub_compare_low:
	/* The high words are equal.  Compare the low words.  */
	bltu	xl, yl, .Lsub_xsmaller
	bltu	yl, xl, .Lsub_ysmaller
	/* The operands are equal.  Return 0.0.  */
	movi	xh, 0
	movi	xl, 0
1:	leaf_return

.Lsub_yexpzero:
	/* y is a subnormal value.  Replace its sign/exponent with zero,
	   i.e., no implicit "1.0".  Unless x is also a subnormal, increment
	   y's apparent exponent because subnormals behave as if they had
	   the minimum (nonzero) exponent.  */
	slli	yh, yh, 12
	srli	yh, yh, 12
	bnone	xh, a6, .Lsub_yexpdiff
	addi	a8, a8, 1
	j	.Lsub_yexpdiff

.Lsub_bigshifty:
	/* Exponent difference > 64 -- just return the bigger value.  */
	bgeui	a10, 64, 1b

	/* Shift yh/yl right by the exponent difference.  Any bits that are
	   shifted out are saved in a9 for rounding the result.  */
	ssr	a10
	sll	a11, yl		/* lost bits shifted out of yl */
	src	a9, yh, yl
	srl	yl, yh
	movi	yh, 0
	beqz	a11, .Lsub_suby
	or	a9, a9, a10	/* any positive, nonzero value will work */
	j	.Lsub_suby

.Lsub_xsmaller:
	/* Same thing as the "ysmaller" code, but with x and y swapped and
	   with y negated.  */
	bnone	xh, a6, .Lsub_xexpzero

	or	xh, xh, a6
	slli	xh, xh, 11
	srli	xh, xh, 11

.Lsub_xexpdiff:
	sub	a10, a8, a7
	bgeui	a10, 32, .Lsub_bigshiftx
	
	ssr	a10
	movi	a9, 0
	src	a9, xl, a9
	src	xl, xh, xl
	srl	xh, xh

	/* Negate y.  */
	slli	a11, a6, 11
	xor	yh, yh, a11

.Lsub_subx:
	sub	xl, yl, xl
	sub	xh, yh, xh
	bgeu	yl, xl, 1f
	addi	xh, xh, -1
1:
	/* Subtract the leftover bits in a9 from zero and propagate any
	   borrow from xh/xl.  */
	neg	a9, a9
	beqz	a9, 1f
	addi	a5, xh, -1
	moveqz	xh, a5, xl
	addi	xl, xl, -1
1:
	/* Check if the subtract underflowed into the exponent.  */
	extui	a10, xh, 20, 11
	bne	a10, a8, .Lsub_borrow

.Lsub_round:
	/* Round up if the leftover fraction is >= 1/2.  */
	bgez	a9, 1f
	addi	xl, xl, 1
	beqz	xl, .Lsub_roundcarry

	/* Check if the leftover fraction is exactly 1/2.  */
	slli	a9, a9, 1
	beqz	a9, .Lsub_exactlyhalf
1:	leaf_return

.Lsub_xexpzero:
	/* Same as "yexpzero".  */
	slli	xh, xh, 12
	srli	xh, xh, 12
	bnone	yh, a6, .Lsub_xexpdiff
	addi	a7, a7, 1
	j	.Lsub_xexpdiff

.Lsub_bigshiftx:
	/* Mostly the same thing as "bigshifty", but with the sign bit of the
	   shifted value set so that the subsequent subtraction flips the
	   sign of y.  */
	bgeui	a10, 64, .Lsub_returny

	ssr	a10
	sll	a11, xl
	src	a9, xh, xl
	srl	xl, xh
	slli	xh, a6, 11	/* set sign bit of xh */
	beqz	a11, .Lsub_subx
	or	a9, a9, a10
	j	.Lsub_subx

.Lsub_returny:
	/* Negate and return y.  */
	slli	a7, a6, 11
	xor	xh, yh, a7
	mov	xl, yl
	leaf_return

.Lsub_borrow:	
	/* The subtraction has underflowed into the exponent field, so the
	   value needs to be renormalized.  Shift the mantissa left as
	   needed to remove any leading zeros and adjust the exponent
	   accordingly.  If the exponent is not large enough to remove
	   all the leading zeros, the result will be a subnormal value.  */

	slli	a8, xh, 12
	beqz	a8, .Lsub_xhzero
	do_nsau	a6, a8, a7, a11
	srli	a8, a8, 12
	bge	a6, a10, .Lsub_subnormal
	addi	a6, a6, 1

.Lsub_shift_lt32:
	/* Shift the mantissa (a8/xl/a9) left by a6.  */
	ssl	a6
	src	a8, a8, xl
	src	xl, xl, a9
	sll	a9, a9

	/* Combine the shifted mantissa with the sign and exponent,
	   decrementing the exponent by a6.  (The exponent has already
	   been decremented by one due to the borrow from the subtraction,
	   but adding the mantissa will increment the exponent by one.)  */
	srli	xh, xh, 20
	sub	xh, xh, a6
	slli	xh, xh, 20
	add	xh, xh, a8
	j	.Lsub_round

.Lsub_exactlyhalf:
	/* Round down to the nearest even value.  */
	srli	xl, xl, 1
	slli	xl, xl, 1
	leaf_return

.Lsub_roundcarry:
	/* xl is always zero when the rounding increment overflows, so
	   there's no need to round it to an even value.  */
	addi	xh, xh, 1
	/* Overflow to the exponent is OK.  */
	leaf_return

.Lsub_xhzero:
	/* When normalizing the result, all the mantissa bits in the high
	   word are zero.  Shift by "20 + (leading zero count of xl) + 1".  */
	do_nsau	a6, xl, a7, a11
	addi	a6, a6, 21
	blt	a10, a6, .Lsub_subnormal

.Lsub_normalize_shift:
	bltui	a6, 32, .Lsub_shift_lt32

	ssl	a6
	src	a8, xl, a9
	sll	xl, a9
	movi	a9, 0

	srli	xh, xh, 20
	sub	xh, xh, a6
	slli	xh, xh, 20
	add	xh, xh, a8
	j	.Lsub_round

.Lsub_subnormal:
	/* The exponent is too small to shift away all the leading zeros.
	   Set a6 to the current exponent (which has already been
	   decremented by the borrow) so that the exponent of the result
	   will be zero.  Do not add 1 to a6 in this case, because: (1)
	   adding the mantissa will not increment the exponent, so there is
	   no need to subtract anything extra from the exponent to
	   compensate, and (2) the effective exponent of a subnormal is 1
	   not 0 so the shift amount must be 1 smaller than normal. */
	mov	a6, a10
	j	.Lsub_normalize_shift

#endif /* L_addsubdf3 */


#ifdef L_muldf3

	/* Multiplication */
#if !XCHAL_HAVE_MUL16 && !XCHAL_HAVE_MUL32 && !XCHAL_HAVE_MAC16
#define XCHAL_NO_MUL 1
#endif

__muldf3_aux:

	/* Handle unusual cases (zeros, subnormals, NaNs and Infinities).
	   (This code is placed before the start of the function just to
	   keep it in range of the limited branch displacements.)  */

.Lmul_xexpzero:
	/* Clear the sign bit of x.  */
	slli	xh, xh, 1
	srli	xh, xh, 1

	/* If x is zero, return zero.  */
	or	a10, xh, xl
	beqz	a10, .Lmul_return_zero

	/* Normalize x.  Adjust the exponent in a8.  */
	beqz	xh, .Lmul_xh_zero
	do_nsau	a10, xh, a11, a12
	addi	a10, a10, -11
	ssl	a10
	src	xh, xh, xl
	sll	xl, xl
	movi	a8, 1
	sub	a8, a8, a10
	j	.Lmul_xnormalized	
.Lmul_xh_zero:
	do_nsau	a10, xl, a11, a12
	addi	a10, a10, -11
	movi	a8, -31
	sub	a8, a8, a10
	ssl	a10
	bltz	a10, .Lmul_xl_srl
	sll	xh, xl
	movi	xl, 0
	j	.Lmul_xnormalized
.Lmul_xl_srl:
	srl	xh, xl
	sll	xl, xl
	j	.Lmul_xnormalized
	
.Lmul_yexpzero:
	/* Clear the sign bit of y.  */
	slli	yh, yh, 1
	srli	yh, yh, 1

	/* If y is zero, return zero.  */
	or	a10, yh, yl
	beqz	a10, .Lmul_return_zero

	/* Normalize y.  Adjust the exponent in a9.  */
	beqz	yh, .Lmul_yh_zero
	do_nsau	a10, yh, a11, a12
	addi	a10, a10, -11
	ssl	a10
	src	yh, yh, yl
	sll	yl, yl
	movi	a9, 1
	sub	a9, a9, a10
	j	.Lmul_ynormalized	
.Lmul_yh_zero:
	do_nsau	a10, yl, a11, a12
	addi	a10, a10, -11
	movi	a9, -31
	sub	a9, a9, a10
	ssl	a10
	bltz	a10, .Lmul_yl_srl
	sll	yh, yl
	movi	yl, 0
	j	.Lmul_ynormalized
.Lmul_yl_srl:
	srl	yh, yl
	sll	yl, yl
	j	.Lmul_ynormalized	

.Lmul_return_zero:
	/* Return zero with the appropriate sign bit.  */
	srli	xh, a7, 31
	slli	xh, xh, 31
	movi	xl, 0
	j	.Lmul_done

.Lmul_xnan_or_inf:
	/* If y is zero, return NaN.  */
	bnez	yl, 1f
	slli	a8, yh, 1
	beqz	a8, .Lmul_return_nan
1:
	/* If y is NaN, return y.  */
	bnall	yh, a6, .Lmul_returnx
	slli	a8, yh, 12
	or	a8, a8, yl
	beqz	a8, .Lmul_returnx

.Lmul_returny:
	mov	xh, yh
	mov	xl, yl

.Lmul_returnx:
	slli	a8, xh, 12
	or	a8, a8, xl
	bnez	a8, .Lmul_return_nan
	/* Set the sign bit and return.  */
	extui	a7, a7, 31, 1
	slli	xh, xh, 1
	ssai	1
	src	xh, a7, xh
	j	.Lmul_done

.Lmul_ynan_or_inf:
	/* If x is zero, return NaN.  */
	bnez	xl, .Lmul_returny
	slli	a8, xh, 1
	bnez	a8, .Lmul_returny
	mov	xh, yh

.Lmul_return_nan:
	movi	a4, 0x80000	/* make it a quiet NaN */
	or	xh, xh, a4
	j	.Lmul_done

	.align	4
	.global	__muldf3
	.type	__muldf3, @function
__muldf3:
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
	leaf_entry sp, 64
#else
	leaf_entry sp, 32
#endif
	movi	a6, 0x7ff00000

	/* Get the sign of the result.  */
	xor	a7, xh, yh

	/* Check for NaN and infinity.  */
	ball	xh, a6, .Lmul_xnan_or_inf
	ball	yh, a6, .Lmul_ynan_or_inf

	/* Extract the exponents.  */
	extui	a8, xh, 20, 11
	extui	a9, yh, 20, 11

	beqz	a8, .Lmul_xexpzero
.Lmul_xnormalized:	
	beqz	a9, .Lmul_yexpzero
.Lmul_ynormalized:	

	/* Add the exponents.  */
	add	a8, a8, a9

	/* Replace sign/exponent fields with explicit "1.0".  */
	movi	a10, 0x1fffff
	or	xh, xh, a6
	and	xh, xh, a10
	or	yh, yh, a6
	and	yh, yh, a10

	/* Multiply 64x64 to 128 bits.  The result ends up in xh/xl/a6.
	   The least-significant word of the result is thrown away except
	   that if it is nonzero, the lsb of a6 is set to 1.  */
#if XCHAL_HAVE_MUL32_HIGH

	/* Compute a6 with any carry-outs in a10.  */
	movi	a10, 0
	mull	a6, xl, yh
	mull	a11, xh, yl
	add	a6, a6, a11
	bgeu	a6, a11, 1f
	addi	a10, a10, 1
1:
	muluh	a11, xl, yl
	add	a6, a6, a11
	bgeu	a6, a11, 1f
	addi	a10, a10, 1
1:	
	/* If the low word of the result is nonzero, set the lsb of a6.  */
	mull	a11, xl, yl
	beqz	a11, 1f
	movi	a9, 1
	or	a6, a6, a9
1:
	/* Compute xl with any carry-outs in a9.  */
	movi	a9, 0
	mull	a11, xh, yh
	add	a10, a10, a11
	bgeu	a10, a11, 1f
	addi	a9, a9, 1
1:	
	muluh	a11, xh, yl
	add	a10, a10, a11
	bgeu	a10, a11, 1f
	addi	a9, a9, 1
1:	
	muluh	xl, xl, yh
	add	xl, xl, a10
	bgeu	xl, a10, 1f
	addi	a9, a9, 1
1:
	/* Compute xh.  */
	muluh	xh, xh, yh
	add	xh, xh, a9

#else /* ! XCHAL_HAVE_MUL32_HIGH */

	/* Break the inputs into 16-bit chunks and compute 16 32-bit partial
	   products.  These partial products are:

		0 xll * yll

		1 xll * ylh
		2 xlh * yll

		3 xll * yhl
		4 xlh * ylh
		5 xhl * yll

		6 xll * yhh
		7 xlh * yhl
		8 xhl * ylh
		9 xhh * yll

		10 xlh * yhh
		11 xhl * yhl
		12 xhh * ylh

		13 xhl * yhh
		14 xhh * yhl

		15 xhh * yhh

	   where the input chunks are (hh, hl, lh, ll).  If using the Mul16
	   or Mul32 multiplier options, these input chunks must be stored in
	   separate registers.  For Mac16, the UMUL.AA.* opcodes can specify
	   that the inputs come from either half of the registers, so there
	   is no need to shift them out ahead of time.  If there is no
	   multiply hardware, the 16-bit chunks can be extracted when setting
	   up the arguments to the separate multiply function.  */

	/* Save a7 since it is needed to hold a temporary value.  */
	s32i	a7, sp, 4
#if __XTENSA_CALL0_ABI__ && XCHAL_NO_MUL
	/* Calling a separate multiply function will clobber a0 and requires
	   use of a8 as a temporary, so save those values now.  (The function
	   uses a custom ABI so nothing else needs to be saved.)  */
	s32i	a0, sp, 0
	s32i	a8, sp, 8
#endif

#if XCHAL_HAVE_MUL16 || XCHAL_HAVE_MUL32

#define xlh a12
#define ylh a13
#define xhh a14
#define yhh a15

	/* Get the high halves of the inputs into registers.  */
	srli	xlh, xl, 16
	srli	ylh, yl, 16
	srli	xhh, xh, 16
	srli	yhh, yh, 16

#define xll xl
#define yll yl
#define xhl xh
#define yhl yh

#if XCHAL_HAVE_MUL32 && !XCHAL_HAVE_MUL16
	/* Clear the high halves of the inputs.  This does not matter
	   for MUL16 because the high bits are ignored.  */
	extui	xl, xl, 0, 16
	extui	xh, xh, 0, 16
	extui	yl, yl, 0, 16
	extui	yh, yh, 0, 16
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

	/* Add pp1 and pp2 into a10 with carry-out in a9.  */
	do_mul(a10, xl, l, yl, h)	/* pp 1 */
	do_mul(a11, xl, h, yl, l)	/* pp 2 */
	movi	a9, 0
	add	a10, a10, a11
	bgeu	a10, a11, 1f
	addi	a9, a9, 1
1:
	/* Initialize a6 with a9/a10 shifted into position.  Note that
	   this value can be safely incremented without any carry-outs.  */
	ssai	16
	src	a6, a9, a10

	/* Compute the low word into a10.  */
	do_mul(a11, xl, l, yl, l)	/* pp 0 */
	sll	a10, a10
	add	a10, a10, a11
	bgeu	a10, a11, 1f
	addi	a6, a6, 1
1:
	/* Compute the contributions of pp0-5 to a6, with carry-outs in a9.
	   This is good enough to determine the low half of a6, so that any
	   nonzero bits from the low word of the result can be collapsed
	   into a6, freeing up a register.  */
	movi	a9, 0
	do_mul(a11, xl, l, yh, l)	/* pp 3 */
	add	a6, a6, a11
	bgeu	a6, a11, 1f
	addi	a9, a9, 1
1:
	do_mul(a11, xl, h, yl, h)	/* pp 4 */
	add	a6, a6, a11
	bgeu	a6, a11, 1f
	addi	a9, a9, 1
1:
	do_mul(a11, xh, l, yl, l)	/* pp 5 */
	add	a6, a6, a11
	bgeu	a6, a11, 1f
	addi	a9, a9, 1
1:
	/* Collapse any nonzero bits from the low word into a6.  */
	beqz	a10, 1f
	movi	a11, 1
	or	a6, a6, a11
1:
	/* Add pp6-9 into a11 with carry-outs in a10.  */
	do_mul(a7, xl, l, yh, h)	/* pp 6 */
	do_mul(a11, xh, h, yl, l)	/* pp 9 */
	movi	a10, 0
	add	a11, a11, a7
	bgeu	a11, a7, 1f
	addi	a10, a10, 1
1:	
	do_mul(a7, xl, h, yh, l)	/* pp 7 */
	add	a11, a11, a7
	bgeu	a11, a7, 1f
	addi	a10, a10, 1
1:	
	do_mul(a7, xh, l, yl, h)	/* pp 8 */
	add	a11, a11, a7
	bgeu	a11, a7, 1f
	addi	a10, a10, 1
1:	
	/* Shift a10/a11 into position, and add low half of a11 to a6.  */
	src	a10, a10, a11
	add	a10, a10, a9
	sll	a11, a11
	add	a6, a6, a11
	bgeu	a6, a11, 1f
	addi	a10, a10, 1
1:
	/* Add pp10-12 into xl with carry-outs in a9.  */
	movi	a9, 0
	do_mul(xl, xl, h, yh, h)	/* pp 10 */
	add	xl, xl, a10
	bgeu	xl, a10, 1f
	addi	a9, a9, 1
1:
	do_mul(a10, xh, l, yh, l)	/* pp 11 */
	add	xl, xl, a10
	bgeu	xl, a10, 1f
	addi	a9, a9, 1
1:
	do_mul(a10, xh, h, yl, h)	/* pp 12 */
	add	xl, xl, a10
	bgeu	xl, a10, 1f
	addi	a9, a9, 1
1:
	/* Add pp13-14 into a11 with carry-outs in a10.  */
	do_mul(a11, xh, l, yh, h)	/* pp 13 */
	do_mul(a7, xh, h, yh, l)	/* pp 14 */
	movi	a10, 0
	add	a11, a11, a7
	bgeu	a11, a7, 1f
	addi	a10, a10, 1
1:
	/* Shift a10/a11 into position, and add low half of a11 to a6.  */
	src	a10, a10, a11
	add	a10, a10, a9
	sll	a11, a11
	add	xl, xl, a11
	bgeu	xl, a11, 1f
	addi	a10, a10, 1
1:
	/* Compute xh.  */
	do_mul(xh, xh, h, yh, h)	/* pp 15 */
	add	xh, xh, a10

	/* Restore values saved on the stack during the multiplication.  */
	l32i	a7, sp, 4
#if __XTENSA_CALL0_ABI__ && XCHAL_NO_MUL
	l32i	a0, sp, 0
	l32i	a8, sp, 8
#endif
#endif /* ! XCHAL_HAVE_MUL32_HIGH */

	/* Shift left by 12 bits, unless there was a carry-out from the
	   multiply, in which case, shift by 11 bits and increment the
	   exponent.  Note: It is convenient to use the constant 0x3ff
	   instead of 0x400 when removing the extra exponent bias (so that
	   it is easy to construct 0x7fe for the overflow check).  Reverse
	   the logic here to decrement the exponent sum by one unless there
	   was a carry-out.  */
	movi	a4, 11
	srli	a5, xh, 21 - 12
	bnez	a5, 1f
	addi	a4, a4, 1
	addi	a8, a8, -1
1:	ssl	a4
	src	xh, xh, xl
	src	xl, xl, a6
	sll	a6, a6

	/* Subtract the extra bias from the exponent sum (plus one to account
	   for the explicit "1.0" of the mantissa that will be added to the
	   exponent in the final result).  */
	movi	a4, 0x3ff
	sub	a8, a8, a4
	
	/* Check for over/underflow.  The value in a8 is one less than the
	   final exponent, so values in the range 0..7fd are OK here.  */
	slli	a4, a4, 1	/* 0x7fe */
	bgeu	a8, a4, .Lmul_overflow
	
.Lmul_round:
	/* Round.  */
	bgez	a6, .Lmul_rounded
	addi	xl, xl, 1
	beqz	xl, .Lmul_roundcarry
	slli	a6, a6, 1
	beqz	a6, .Lmul_exactlyhalf

.Lmul_rounded:
	/* Add the exponent to the mantissa.  */
	slli	a8, a8, 20
	add	xh, xh, a8

.Lmul_addsign:
	/* Add the sign bit.  */
	srli	a7, a7, 31
	slli	a7, a7, 31
	or	xh, xh, a7

.Lmul_done:
#if __XTENSA_CALL0_ABI__
	l32i	a12, sp, 16
	l32i	a13, sp, 20
	l32i	a14, sp, 24
	l32i	a15, sp, 28
	addi	sp, sp, 32
#endif
	leaf_return

.Lmul_exactlyhalf:
	/* Round down to the nearest even value.  */
	srli	xl, xl, 1
	slli	xl, xl, 1
	j	.Lmul_rounded

.Lmul_roundcarry:
	/* xl is always zero when the rounding increment overflows, so
	   there's no need to round it to an even value.  */
	addi	xh, xh, 1
	/* Overflow is OK -- it will be added to the exponent.  */
	j	.Lmul_rounded

.Lmul_overflow:
	bltz	a8, .Lmul_underflow
	/* Return +/- Infinity.  */
	addi	a8, a4, 1	/* 0x7ff */
	slli	xh, a8, 20
	movi	xl, 0
	j	.Lmul_addsign

.Lmul_underflow:
	/* Create a subnormal value, where the exponent field contains zero,
	   but the effective exponent is 1.  The value of a8 is one less than
	   the actual exponent, so just negate it to get the shift amount.  */
	neg	a8, a8
	mov	a9, a6
	ssr	a8
	bgeui	a8, 32, .Lmul_bigshift
	
	/* Shift xh/xl right.  Any bits that are shifted out of xl are saved
	   in a6 (combined with the shifted-out bits currently in a6) for
	   rounding the result.  */
	sll	a6, xl
	src	xl, xh, xl
	srl	xh, xh
	j	1f

.Lmul_bigshift:
	bgeui	a8, 64, .Lmul_flush_to_zero
	sll	a10, xl		/* lost bits shifted out of xl */
	src	a6, xh, xl
	srl	xl, xh
	movi	xh, 0
	or	a9, a9, a10

	/* Set the exponent to zero.  */
1:	movi	a8, 0

	/* Pack any nonzero bits shifted out into a6.  */
	beqz	a9, .Lmul_round
	movi	a9, 1
	or	a6, a6, a9
	j	.Lmul_round
	
.Lmul_flush_to_zero:
	/* Return zero with the appropriate sign bit.  */
	srli	xh, a7, 31
	slli	xh, xh, 31
	movi	xl, 0
	j	.Lmul_done

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
#endif /* L_muldf3 */


#ifdef L_divdf3

	/* Division */

#if XCHAL_HAVE_DFP_DIV
#if !XCHAL_HAVE_USER_DPFPU && !XCHAL_HAVE_USER_SPFPU && !XCHAL_HAVE_VECTORFPU2005

        .text
        .align 4
        .global __divdf3
        .type __divdf3,@function

__divdf3:
        leaf_entry    a1, 16

#if XCHAL_HAVE_DFP && XSHAL_FLOATING_POINT_ABI
	mov.d		f2, f1
	mov.d		f1, f0
#else
	wfrd		f1, xh, xl
	wfrd		f2, yh, yl
#endif


	div0.d		f3, f2
	nexp01.d	f4, f2
	const.d		f0, 1
	maddn.d		f0, f4, f3
	const.d		f5, 0
	mov.d		f7, f2
	mkdadj.d	f7, f1
	maddn.d		f3, f0, f3
	maddn.d		f5, f0, f0
	nexp01.d	f1, f1
	div0.d		f2, f2
	maddn.d		f3, f5, f3
	const.d		f5, 1
	const.d		f0, 0
	neg.d		f6, f1
	maddn.d		f5, f4, f3
	maddn.d		f0, f6, f2
	maddn.d		f3, f5, f3
	maddn.d		f6, f4, f0
	const.d		f2, 1
	maddn.d		f2, f4, f3
	maddn.d		f0, f6, f3
	neg.d		f1, f1
	maddn.d		f3, f2, f3
	maddn.d		f1, f4, f0
	addexpm.d	f0, f7
	addexp.d	f3, f7
	divn.d		f0, f1, f3

	rfr		xl, f0
	rfrd		xh, f0

	leaf_return
	.size __divdf3, . - __divdf3
#endif
#else

/* if XCHAL_HAVE_DIV32 is set then __difdf3 is provided by softfloat package,
   uclibc should use this version though */

#if !XCHAL_HAVE_DIV32 || XSHAL_CLIB==XTHAL_CLIB_UCLIBC

__divdf3_aux:

	/* Handle unusual cases (zeros, subnormals, NaNs and Infinities).
	   (This code is placed before the start of the function just to
	   keep it in range of the limited branch displacements.)  */

.Ldiv_yexpzero:
	/* Clear the sign bit of y.  */
	slli	yh, yh, 1
	srli	yh, yh, 1

	/* Check for division by zero.  */
	or	a10, yh, yl
	beqz	a10, .Ldiv_yzero

	/* Normalize y.  Adjust the exponent in a9.  */
	beqz	yh, .Ldiv_yh_zero
	do_nsau	a10, yh, a11, a9
	addi	a10, a10, -11
	ssl	a10
	src	yh, yh, yl
	sll	yl, yl
	movi	a9, 1
	sub	a9, a9, a10
	j	.Ldiv_ynormalized	
.Ldiv_yh_zero:
	do_nsau	a10, yl, a11, a9
	addi	a10, a10, -11
	movi	a9, -31
	sub	a9, a9, a10
	ssl	a10
	bltz	a10, .Ldiv_yl_srl
	sll	yh, yl
	movi	yl, 0
	j	.Ldiv_ynormalized
.Ldiv_yl_srl:
	srl	yh, yl
	sll	yl, yl
	j	.Ldiv_ynormalized	

.Ldiv_yzero:
	/* y is zero.  Return NaN if x is also zero; otherwise, infinity.  */
	slli	xh, xh, 1
	srli	xh, xh, 1
	or	xl, xl, xh
	srli	xh, a7, 31
	slli	xh, xh, 31
	or	xh, xh, a6
	bnez	xl, 1f
	movi	a4, 0x80000	/* make it a quiet NaN */
	or	xh, xh, a4
1:	movi	xl, 0
	leaf_return

.Ldiv_xexpzero:
	/* Clear the sign bit of x.  */
	slli	xh, xh, 1
	srli	xh, xh, 1

	/* If x is zero, return zero.  */
	or	a10, xh, xl
	beqz	a10, .Ldiv_return_zero

	/* Normalize x.  Adjust the exponent in a8.  */
	beqz	xh, .Ldiv_xh_zero
	do_nsau	a10, xh, a11, a8
	addi	a10, a10, -11
	ssl	a10
	src	xh, xh, xl
	sll	xl, xl
	movi	a8, 1
	sub	a8, a8, a10
	j	.Ldiv_xnormalized	
.Ldiv_xh_zero:
	do_nsau	a10, xl, a11, a8
	addi	a10, a10, -11
	movi	a8, -31
	sub	a8, a8, a10
	ssl	a10
	bltz	a10, .Ldiv_xl_srl
	sll	xh, xl
	movi	xl, 0
	j	.Ldiv_xnormalized
.Ldiv_xl_srl:
	srl	xh, xl
	sll	xl, xl
	j	.Ldiv_xnormalized
	
.Ldiv_return_zero:
	/* Return zero with the appropriate sign bit.  */
	srli	xh, a7, 31
	slli	xh, xh, 31
	movi	xl, 0
	leaf_return

.Ldiv_xnan_or_inf:
	/* Set the sign bit of the result.  */
	srli	a7, yh, 31
	slli	a7, a7, 31
	xor	xh, xh, a7
	/* If y is NaN or Inf, return NaN.  */
	ball	yh, a6, .Ldiv_return_nan
	slli	a8, xh, 12
	or	a8, a8, xl
	bnez	a8, .Ldiv_return_nan
	leaf_return

.Ldiv_ynan_or_inf:
	/* If y is Infinity, return zero.  */
	slli	a8, yh, 12
	or	a8, a8, yl
	beqz	a8, .Ldiv_return_zero
	/* y is NaN; return it.  */
	mov	xh, yh
	mov	xl, yl

.Ldiv_return_nan:
	movi	a4, 0x80000	/* make it a quiet NaN */
	or	xh, xh, a4
	leaf_return

.Ldiv_highequal1:
	bltu	xl, yl, 2f
	j	3f

	.align	4
	.global	__divdf3
	.type	__divdf3, @function
__divdf3:
	leaf_entry sp, 16
	movi	a6, 0x7ff00000

	/* Get the sign of the result.  */
	xor	a7, xh, yh

	/* Check for NaN and infinity.  */
	ball	xh, a6, .Ldiv_xnan_or_inf
	ball	yh, a6, .Ldiv_ynan_or_inf

	/* Extract the exponents.  */
	extui	a8, xh, 20, 11
	extui	a9, yh, 20, 11

	beqz	a9, .Ldiv_yexpzero
.Ldiv_ynormalized:	
	beqz	a8, .Ldiv_xexpzero
.Ldiv_xnormalized:	

	/* Subtract the exponents.  */
	sub	a8, a8, a9

	/* Replace sign/exponent fields with explicit "1.0".  */
	movi	a10, 0x1fffff
	or	xh, xh, a6
	and	xh, xh, a10
	or	yh, yh, a6
	and	yh, yh, a10

	/* Set SAR for left shift by one.  */
	ssai	(32 - 1)

	/* The first digit of the mantissa division must be a one.
	   Shift x (and adjust the exponent) as needed to make this true.  */
	bltu	yh, xh, 3f
	beq	yh, xh, .Ldiv_highequal1
2:	src	xh, xh, xl
	sll	xl, xl
	addi	a8, a8, -1
3:
	/* Do the first subtraction and shift.  */
	sub	xh, xh, yh
	bgeu	xl, yl, 1f
	addi	xh, xh, -1
1:	sub	xl, xl, yl
	src	xh, xh, xl
	sll	xl, xl

	/* Put the quotient into a10/a11.  */
	movi	a10, 0
	movi	a11, 1

	/* Divide one bit at a time for 52 bits.  */
	movi	a9, 52
#if XCHAL_HAVE_LOOPS
	loop	a9, .Ldiv_loopend
#endif
.Ldiv_loop:
	/* Shift the quotient << 1.  */
	src	a10, a10, a11
	sll	a11, a11

	/* Is this digit a 0 or 1?  */
	bltu	xh, yh, 3f
	beq	xh, yh, .Ldiv_highequal2

	/* Output a 1 and subtract.  */
2:	addi	a11, a11, 1
	sub	xh, xh, yh
	bgeu	xl, yl, 1f
	addi	xh, xh, -1
1:	sub	xl, xl, yl

	/* Shift the dividend << 1.  */
3:	src	xh, xh, xl
	sll	xl, xl

#if !XCHAL_HAVE_LOOPS
	addi	a9, a9, -1
	bnez	a9, .Ldiv_loop
#endif
.Ldiv_loopend:

	/* Add the exponent bias (less one to account for the explicit "1.0"
	   of the mantissa that will be added to the exponent in the final
	   result).  */
	movi	a9, 0x3fe
	add	a8, a8, a9
	
	/* Check for over/underflow.  The value in a8 is one less than the
	   final exponent, so values in the range 0..7fd are OK here.  */
	addmi	a9, a9, 0x400	/* 0x7fe */
	bgeu	a8, a9, .Ldiv_overflow

.Ldiv_round:
	/* Round.  The remainder (<< 1) is in xh/xl.  */
	bltu	xh, yh, .Ldiv_rounded
	beq	xh, yh, .Ldiv_highequal3
.Ldiv_roundup:
	addi	a11, a11, 1
	beqz	a11, .Ldiv_roundcarry

.Ldiv_rounded:
	mov	xl, a11
	/* Add the exponent to the mantissa.  */
	slli	a8, a8, 20
	add	xh, a10, a8

.Ldiv_addsign:
	/* Add the sign bit.  */
	srli	a7, a7, 31
	slli	a7, a7, 31
	or	xh, xh, a7
	leaf_return

.Ldiv_highequal2:
	bgeu	xl, yl, 2b
	j	3b

.Ldiv_highequal3:
	bltu	xl, yl, .Ldiv_rounded
	bne	xl, yl, .Ldiv_roundup

	/* Remainder is exactly half the divisor.  Round even.  */
	addi	a11, a11, 1
	beqz	a11, .Ldiv_roundcarry
	srli	a11, a11, 1
	slli	a11, a11, 1
	j	.Ldiv_rounded

.Ldiv_overflow:
	bltz	a8, .Ldiv_underflow
	/* Return +/- Infinity.  */
	addi	a8, a9, 1	/* 0x7ff */
	slli	xh, a8, 20
	movi	xl, 0
	j	.Ldiv_addsign

.Ldiv_underflow:
	/* Create a subnormal value, where the exponent field contains zero,
	   but the effective exponent is 1.  The value of a8 is one less than
	   the actual exponent, so just negate it to get the shift amount.  */
	neg	a8, a8
	ssr	a8
	bgeui	a8, 32, .Ldiv_bigshift
	
	/* Shift a10/a11 right.  Any bits that are shifted out of a11 are
	   saved in a6 for rounding the result.  */
	sll	a6, a11
	src	a11, a10, a11
	srl	a10, a10
	j	1f

.Ldiv_bigshift:
	bgeui	a8, 64, .Ldiv_flush_to_zero
	sll	a9, a11		/* lost bits shifted out of a11 */
	src	a6, a10, a11
	srl	a11, a10
	movi	a10, 0
	or	xl, xl, a9

	/* Set the exponent to zero.  */
1:	movi	a8, 0

	/* Pack any nonzero remainder (in xh/xl) into a6.  */
	or	xh, xh, xl
	beqz	xh, 1f
	movi	a9, 1
	or	a6, a6, a9
	
	/* Round a10/a11 based on the bits shifted out into a6.  */
1:	bgez	a6, .Ldiv_rounded
	addi	a11, a11, 1
	beqz	a11, .Ldiv_roundcarry
	slli	a6, a6, 1
	bnez	a6, .Ldiv_rounded
	srli	a11, a11, 1
	slli	a11, a11, 1
	j	.Ldiv_rounded

.Ldiv_roundcarry:
	/* a11 is always zero when the rounding increment overflows, so
	   there's no need to round it to an even value.  */
	addi	a10, a10, 1
	/* Overflow to the exponent field is OK.  */
	j	.Ldiv_rounded

.Ldiv_flush_to_zero:
	/* Return zero with the appropriate sign bit.  */
	srli	xh, a7, 31
	slli	xh, xh, 31
	movi	xl, 0
	leaf_return
	.size __divdf3, . - __divdf3

#endif // !XCHAL_HAVE_DIV32
#endif // XCHAL_HAVE_DFP_DIV

#endif /* L_divdf3 */


#ifdef L_cmpdf2

	/* Equal and Not Equal */

	.align	4
	.global	__eqdf2
	.global	__nedf2
	.set	__nedf2, __eqdf2
	.type	__eqdf2, @function
__eqdf2:
	leaf_entry sp, 16
	bne	xl, yl, 2f
	bne	xh, yh, 4f

	/* The values are equal but NaN != NaN.  Check the exponent.  */
	movi	a6, 0x7ff00000
	ball	xh, a6, 3f

	/* Equal.  */
	movi	a2, 0
	leaf_return

	/* Not equal.  */
2:	movi	a2, 1
	leaf_return

	/* Check if the mantissas are nonzero.  */
3:	slli	a7, xh, 12
	or	a7, a7, xl
	j	5f

	/* Check if x and y are zero with different signs.  */
4:	or	a7, xh, yh
	slli	a7, a7, 1
	or	a7, a7, xl	/* xl == yl here */

	/* Equal if a7 == 0, where a7 is either abs(x | y) or the mantissa
	   or x when exponent(x) = 0x7ff and x == y.  */
5:	movi	a2, 0
	movi	a3, 1
	movnez	a2, a3, a7	
	leaf_return


	/* Greater Than */

	.align	4
	.global	__gtdf2
	.type	__gtdf2, @function
__gtdf2:
	leaf_entry sp, 16
	movi	a6, 0x7ff00000
	ball	xh, a6, 2f
1:	bnall	yh, a6, .Lle_cmp

	/* Check if y is a NaN.  */
	slli	a7, yh, 12
	or	a7, a7, yl
	beqz	a7, .Lle_cmp
	movi	a2, 0
	leaf_return

	/* Check if x is a NaN.  */
2:	slli	a7, xh, 12
	or	a7, a7, xl
	beqz	a7, 1b
	movi	a2, 0
	leaf_return


	/* Less Than or Equal */

	.align	4
	.global	__ledf2
	.type	__ledf2, @function
__ledf2:
	leaf_entry sp, 16
	movi	a6, 0x7ff00000
	ball	xh, a6, 2f
1:	bnall	yh, a6, .Lle_cmp

	/* Check if y is a NaN.  */
	slli	a7, yh, 12
	or	a7, a7, yl
	beqz	a7, .Lle_cmp
	movi	a2, 1
	leaf_return

	/* Check if x is a NaN.  */
2:	slli	a7, xh, 12
	or	a7, a7, xl
	beqz	a7, 1b
	movi	a2, 1
	leaf_return

.Lle_cmp:
	/* Check if x and y have different signs.  */
	xor	a7, xh, yh
	bltz	a7, .Lle_diff_signs

	/* Check if x is negative.  */
	bltz	xh, .Lle_xneg

	/* Check if x <= y.  */
	bltu	xh, yh, 4f
	bne	xh, yh, 5f
	bltu	yl, xl, 5f
4:	movi	a2, 0
	leaf_return

.Lle_xneg:
	/* Check if y <= x.  */
	bltu	yh, xh, 4b
	bne	yh, xh, 5f
	bgeu	xl, yl, 4b
5:	movi	a2, 1
	leaf_return

.Lle_diff_signs:
	bltz	xh, 4b

	/* Check if both x and y are zero.  */
	or	a7, xh, yh
	slli	a7, a7, 1
	or	a7, a7, xl
	or	a7, a7, yl
	movi	a2, 1
	movi	a3, 0
	moveqz	a2, a3, a7
	leaf_return


	/* Greater Than or Equal */

	.align	4
	.global	__gedf2
	.type	__gedf2, @function
__gedf2:
	leaf_entry sp, 16
	movi	a6, 0x7ff00000
	ball	xh, a6, 2f
1:	bnall	yh, a6, .Llt_cmp

	/* Check if y is a NaN.  */
	slli	a7, yh, 12
	or	a7, a7, yl
	beqz	a7, .Llt_cmp
	movi	a2, -1
	leaf_return

	/* Check if x is a NaN.  */
2:	slli	a7, xh, 12
	or	a7, a7, xl
	beqz	a7, 1b
	movi	a2, -1
	leaf_return


	/* Less Than */

	.align	4
	.global	__ltdf2
	.type	__ltdf2, @function
__ltdf2:
	leaf_entry sp, 16
	movi	a6, 0x7ff00000
	ball	xh, a6, 2f
1:	bnall	yh, a6, .Llt_cmp

	/* Check if y is a NaN.  */
	slli	a7, yh, 12
	or	a7, a7, yl
	beqz	a7, .Llt_cmp
	movi	a2, 0
	leaf_return

	/* Check if x is a NaN.  */
2:	slli	a7, xh, 12
	or	a7, a7, xl
	beqz	a7, 1b
	movi	a2, 0
	leaf_return

.Llt_cmp:
	/* Check if x and y have different signs.  */
	xor	a7, xh, yh
	bltz	a7, .Llt_diff_signs

	/* Check if x is negative.  */
	bltz	xh, .Llt_xneg

	/* Check if x < y.  */
	bltu	xh, yh, 4f
	bne	xh, yh, 5f
	bgeu	xl, yl, 5f
4:	movi	a2, -1
	leaf_return

.Llt_xneg:
	/* Check if y < x.  */
	bltu	yh, xh, 4b
	bne	yh, xh, 5f
	bltu	yl, xl, 4b
5:	movi	a2, 0
	leaf_return

.Llt_diff_signs:
	bgez	xh, 5b

	/* Check if both x and y are nonzero.  */
	or	a7, xh, yh
	slli	a7, a7, 1
	or	a7, a7, xl
	or	a7, a7, yl
	movi	a2, 0
	movi	a3, -1
	movnez	a2, a3, a7
	leaf_return



#endif /* L_cmpdf2 */
#endif /*  XCHAL_HAVE_DFP_accel */

#ifdef L_cmpdf2
	/* Unordered for both normal and XCHAL_HAVE_DFP_accel */

	.align	4
	.global	__unorddf2
	.type	__unorddf2, @function
__unorddf2:
	leaf_entry sp, 16
	movi	a6, 0x7ff00000
	ball	xh, a6, 3f
1:	ball	yh, a6, 4f
2:	movi	a2, 0
	leaf_return

3:	slli	a7, xh, 12
	or	a7, a7, xl
	beqz	a7, 1b
	movi	a2, 1
	leaf_return

4:	slli	a7, yh, 12
	or	a7, a7, yl
	beqz	a7, 2b
	movi	a2, 1
	leaf_return
	.size __unorddf2, . - __unorddf2

#endif /* L_cmpdf2 */

#ifdef L_fixdfsi

	.align	4
	.global	__fixdfsi
	.type	__fixdfsi, @function
__fixdfsi:
	leaf_entry sp, 16
#if XCHAL_HAVE_DFP && XSHAL_FLOATING_POINT_ABI
	rfr	xl, F0
	rfrd	xh, F0
#endif

	/* Check for NaN and Infinity.  */
	movi	a6, 0x7ff00000
	ball	xh, a6, .Lfixdfsi_nan_or_inf

	/* Extract the exponent and check if 0 < (exp - 0x3fe) < 32.  */
	extui	a4, xh, 20, 11
	extui	a5, a6, 19, 10	/* 0x3fe */
	sub	a4, a4, a5
	bgei	a4, 32, .Lfixdfsi_maxint
	blti	a4, 1, .Lfixdfsi_zero

	/* Add explicit "1.0" and shift << 11.  */
	or	a7, xh, a6
	ssai	(32 - 11)
	src	a5, a7, xl

	/* Shift back to the right, based on the exponent.  */
	ssl	a4		/* shift by 32 - a4 */
	srl	a5, a5

	/* Negate the result if sign != 0.  */
	neg	a2, a5
	movgez	a2, a5, a7
	leaf_return

.Lfixdfsi_nan_or_inf:
	/* Handle Infinity and NaN.  */
	slli	a4, xh, 12
	or	a4, a4, xl
	beqz	a4, .Lfixdfsi_maxint

	/* Translate NaN to +maxint.  */
	movi	xh, 0

.Lfixdfsi_maxint:
	slli	a4, a6, 11	/* 0x80000000 */
	addi	a5, a4, -1	/* 0x7fffffff */
	movgez	a4, a5, xh
	mov	a2, a4
	leaf_return

.Lfixdfsi_zero:
	movi	a2, 0
	leaf_return
	.size __fixdfsi, . - __fixdfsi
	
#endif /* L_fixdfsi */

#ifdef L_fixdfdi

	.align	4
	.global	__fixdfdi
	.type	__fixdfdi, @function
__fixdfdi:
	leaf_entry sp, 16
#if XCHAL_HAVE_DFP && XSHAL_FLOATING_POINT_ABI
	rfr	xl, F0
	rfrd	xh, F0
#endif

	/* Check for NaN and Infinity.  */
	movi	a6, 0x7ff00000
	ball	xh, a6, .Lfixdfdi_nan_or_inf

	/* Extract the exponent and check if 0 < (exp - 0x3fe) < 64.  */
	extui	a4, xh, 20, 11
	extui	a5, a6, 19, 10	/* 0x3fe */
	sub	a4, a4, a5
	bgei	a4, 64, .Lfixdfdi_maxint
	blti	a4, 1, .Lfixdfdi_zero

	/* Add explicit "1.0" and shift << 11.  */
	or	a7, xh, a6
	ssai	(32 - 11)
	src	xh, a7, xl
	sll	xl, xl

	/* Shift back to the right, based on the exponent.  */
	ssl	a4		/* shift by 64 - a4 */
	bgei	a4, 32, .Lfixdfdi_smallshift
	srl	xl, xh
	movi	xh, 0

.Lfixdfdi_shifted:	
	/* Negate the result if sign != 0.  */
	bgez	a7, 1f
	neg	xl, xl
	neg	xh, xh
	beqz	xl, 1f
	addi	xh, xh, -1
1:	leaf_return

.Lfixdfdi_smallshift:
	src	xl, xh, xl
	srl	xh, xh
	j	.Lfixdfdi_shifted

.Lfixdfdi_nan_or_inf:
	/* Handle Infinity and NaN.  */
	slli	a4, xh, 12
	or	a4, a4, xl
	beqz	a4, .Lfixdfdi_maxint

	/* Translate NaN to +maxint.  */
	movi	xh, 0

.Lfixdfdi_maxint:
	slli	a7, a6, 11	/* 0x80000000 */
	bgez	xh, 1f
	mov	xh, a7
	movi	xl, 0
	leaf_return

1:	addi	xh, a7, -1	/* 0x7fffffff */
	movi	xl, -1
	leaf_return

.Lfixdfdi_zero:
	movi	xh, 0
	movi	xl, 0
	leaf_return

#endif /* L_fixdfdi */

#ifdef L_fixunsdfsi

	.align	4
	.global	__fixunsdfsi
	.type	__fixunsdfsi, @function
__fixunsdfsi:
	leaf_entry sp, 16
#if XCHAL_HAVE_DFP && XSHAL_FLOATING_POINT_ABI
	rfr	xl, F0
	rfrd	xh, F0
#endif

	/* Check for NaN and Infinity.  */
	movi	a6, 0x7ff00000
	ball	xh, a6, .Lfixunsdfsi_nan_or_inf

	/* Extract the exponent and check if 0 <= (exp - 0x3ff) < 32.  */
	extui	a4, xh, 20, 11
	extui	a5, a6, 20, 10	/* 0x3ff */
	sub	a4, a4, a5
	bgei	a4, 32, .Lfixunsdfsi_maxint
	bltz	a4, .Lfixunsdfsi_zero

	/* Add explicit "1.0" and shift << 11.  */
	or	a7, xh, a6
	ssai	(32 - 11)
	src	a5, a7, xl

	/* Shift back to the right, based on the exponent.  */
	addi	a4, a4, 1
	beqi	a4, 32, .Lfixunsdfsi_bigexp
	ssl	a4		/* shift by 32 - a4 */
	srl	a5, a5

	/* Negate the result if sign != 0.  */
	neg	a2, a5
	movgez	a2, a5, a7
	leaf_return

.Lfixunsdfsi_nan_or_inf:
	/* Handle Infinity and NaN.  */
	slli	a4, xh, 12
	or	a4, a4, xl
	beqz	a4, .Lfixunsdfsi_maxint

	/* Translate NaN to 0xffffffff.  */
	movi	a2, -1
	leaf_return

.Lfixunsdfsi_maxint:
	slli	a4, a6, 11	/* 0x80000000 */
	movi	a5, -1		/* 0xffffffff */
	movgez	a4, a5, xh
	mov	a2, a4
	leaf_return

.Lfixunsdfsi_zero:
	movi	a2, 0
	leaf_return

.Lfixunsdfsi_bigexp:
	/* Handle unsigned maximum exponent case.  */
	bltz	xh, 1f
	mov	a2, a5		/* no shift needed */
	leaf_return

	/* Return 0x80000000 if negative.  */
1:	slli	a2, a6, 11
	leaf_return

#endif /* L_fixunsdfsi */

#ifdef L_fixunsdfdi

	.align	4
	.global	__fixunsdfdi
	.type	__fixunsdfdi, @function
__fixunsdfdi:
	leaf_entry sp, 16
#if XCHAL_HAVE_DFP && XSHAL_FLOATING_POINT_ABI
	rfr	xl, F0
	rfrd	xh, F0
#endif

	/* Check for NaN and Infinity.  */
	movi	a6, 0x7ff00000
	ball	xh, a6, .Lfixunsdfdi_nan_or_inf

	/* Extract the exponent and check if 0 <= (exp - 0x3ff) < 64.  */
	extui	a4, xh, 20, 11
	extui	a5, a6, 20, 10	/* 0x3ff */
	sub	a4, a4, a5
	bgei	a4, 64, .Lfixunsdfdi_maxint
	bltz	a4, .Lfixunsdfdi_zero

	/* Add explicit "1.0" and shift << 11.  */
	or	a7, xh, a6
	ssai	(32 - 11)
	src	xh, a7, xl
	sll	xl, xl

	/* Shift back to the right, based on the exponent.  */
	addi	a4, a4, 1
	beqi	a4, 64, .Lfixunsdfdi_bigexp
	ssl	a4		/* shift by 64 - a4 */
	bgei	a4, 32, .Lfixunsdfdi_smallshift
	srl	xl, xh
	movi	xh, 0

.Lfixunsdfdi_shifted:
	/* Negate the result if sign != 0.  */
	bgez	a7, 1f
	neg	xl, xl
	neg	xh, xh
	beqz	xl, 1f
	addi	xh, xh, -1
1:	leaf_return

.Lfixunsdfdi_smallshift:
	src	xl, xh, xl
	srl	xh, xh
	j	.Lfixunsdfdi_shifted

.Lfixunsdfdi_nan_or_inf:
	/* Handle Infinity and NaN.  */
	slli	a4, xh, 12
	or	a4, a4, xl
	beqz	a4, .Lfixunsdfdi_maxint

	/* Translate NaN to 0xffffffff.... */
1:	movi	xh, -1
	movi	xl, -1
	leaf_return

.Lfixunsdfdi_maxint:
	bgez	xh, 1b
2:	slli	xh, a6, 11	/* 0x80000000 */
	movi	xl, 0
	leaf_return

.Lfixunsdfdi_zero:
	movi	xh, 0
	movi	xl, 0
	leaf_return

.Lfixunsdfdi_bigexp:
	/* Handle unsigned maximum exponent case.  */
	bltz	a7, 2b
	leaf_return		/* no shift needed */

#endif /* L_fixunsdfdi */

#ifdef L_floatsidf

	.align	4
	.global	__floatunsidf
	.type	__floatunsidf, @function
__floatunsidf:
	leaf_entry sp, 16
	beqz	a2, .Lfloatsidf_return_zero

	/* Set the sign to zero and jump to the floatsidf code.  */
	movi	a7, 0
	j	.Lfloatsidf_normalize

	.align	4
	.global	__floatsidf
	.type	__floatsidf, @function
__floatsidf:
	leaf_entry sp, 16

	/* Check for zero.  */
	beqz	a2, .Lfloatsidf_return_zero

	/* Save the sign.  */
	extui	a7, a2, 31, 1

	/* Get the absolute value.  */
#if XCHAL_HAVE_ABS
	abs	a2, a2
#else
	neg	a4, a2
	movltz	a2, a4, a2
#endif

.Lfloatsidf_normalize:
	/* Normalize with the first 1 bit in the msb.  */
	do_nsau	a4, a2, a5, a6
	ssl	a4
	sll	a5, a2

	/* Shift the mantissa into position.  */
	srli	xh, a5, 11
	slli	xl, a5, (32 - 11)

	/* Set the exponent.  */
	movi	a5, 0x41d	/* 0x3fe + 31 */
	sub	a5, a5, a4
	slli	a5, a5, 20
	add	xh, xh, a5

	/* Add the sign and return. */
	slli	a7, a7, 31
	or	xh, xh, a7
#if XCHAL_HAVE_DFP && XSHAL_FLOATING_POINT_ABI
	wfrd	F0, xh, xl
#endif
	leaf_return

.Lfloatsidf_return_zero:
	movi	a3, 0
#if XCHAL_HAVE_DFP && XSHAL_FLOATING_POINT_ABI
	wfrd	F0, xh, xl
#endif
	leaf_return
	.size __floatsidf, . - __floatsidf

#endif /* L_floatsidf */

#ifdef L_floatdidf

	.align	4
	.global	__floatundidf
	.type	__floatundidf, @function
__floatundidf:
	leaf_entry sp, 16

	/* Check for zero.  */
	or	a4, xh, xl
	beqz	a4, 2f

	/* Set the sign to zero and jump to the floatdidf code.  */
	movi	a7, 0
	j	.Lfloatdidf_normalize

	.align	4
	.global	__floatdidf
	.type	__floatdidf, @function
__floatdidf:
	leaf_entry sp, 16

	/* Check for zero.  */
	or	a4, xh, xl
	beqz	a4, 2f

	/* Save the sign.  */
	extui	a7, xh, 31, 1

	/* Get the absolute value.  */
	bgez	xh, .Lfloatdidf_normalize
	neg	xl, xl
	neg	xh, xh
	beqz	xl, .Lfloatdidf_normalize
	addi	xh, xh, -1

.Lfloatdidf_normalize:
	/* Normalize with the first 1 bit in the msb of xh.  */
	beqz	xh, .Lfloatdidf_bigshift
	do_nsau	a4, xh, a5, a6
	ssl	a4
	src	xh, xh, xl
	sll	xl, xl

.Lfloatdidf_shifted:
	/* Shift the mantissa into position, with rounding bits in a6.  */
	ssai	11
	sll	a6, xl
	src	xl, xh, xl
	srl	xh, xh

	/* Set the exponent.  */
	movi	a5, 0x43d	/* 0x3fe + 63 */
	sub	a5, a5, a4
	slli	a5, a5, 20
	add	xh, xh, a5

	/* Add the sign.  */
	slli	a7, a7, 31
	or	xh, xh, a7

	/* Round up if the leftover fraction is >= 1/2.  */
	bgez	a6, 2f
	addi	xl, xl, 1
	beqz	xl, .Lfloatdidf_roundcarry

	/* Check if the leftover fraction is exactly 1/2.  */
	slli	a6, a6, 1
	beqz	a6, .Lfloatdidf_exactlyhalf
2:	nop
#if XCHAL_HAVE_DFP && XSHAL_FLOATING_POINT_ABI
	wfrd	F0, xh, xl
#endif
	leaf_return

.Lfloatdidf_bigshift:
	/* xh is zero.  Normalize with first 1 bit of xl in the msb of xh.  */
	do_nsau	a4, xl, a5, a6
	ssl	a4
	sll	xh, xl
	movi	xl, 0
	addi	a4, a4, 32
	j	.Lfloatdidf_shifted

.Lfloatdidf_exactlyhalf:
	/* Round down to the nearest even value.  */
	srli	xl, xl, 1
	slli	xl, xl, 1
#if XCHAL_HAVE_DFP && XSHAL_FLOATING_POINT_ABI
	wfrd	F0, xh, xl
#endif
	leaf_return

.Lfloatdidf_roundcarry:
	/* xl is always zero when the rounding increment overflows, so
	   there's no need to round it to an even value.  */
	addi	xh, xh, 1
	/* Overflow to the exponent is OK.  */
#if XCHAL_HAVE_DFP && XSHAL_FLOATING_POINT_ABI
	wfrd	F0, xh, xl
#endif
	leaf_return

#endif /* L_floatdidf */

#ifdef L_truncdfsf2

	.align	4
	.global	__truncdfsf2
	.type	__truncdfsf2, @function
__truncdfsf2:
	leaf_entry sp, 16

#if XCHAL_HAVE_DFP && XSHAL_FLOATING_POINT_ABI
	rfr	xl, F0
	rfrd	xh, F0
#endif

	/* Adjust the exponent bias.  */
	movi	a4, (0x3ff - 0x7f) << 20
	sub	a5, xh, a4

	/* Check for underflow.  */
	xor	a6, xh, a5
	bltz	a6, .Ltrunc_underflow
	extui	a6, a5, 20, 11
	beqz	a6, .Ltrunc_underflow

	/* Check for overflow.  */
	movi	a4, 255
	bge	a6, a4, .Ltrunc_overflow

	/* Shift a5/xl << 3 into a5/a4.  */
	ssai	(32 - 3)
	src	a5, a5, xl
	sll	a4, xl

.Ltrunc_addsign:
	/* Add the sign bit.  */
	extui	a6, xh, 31, 1
	slli	a6, a6, 31
	or	a2, a6, a5

	/* Round up if the leftover fraction is >= 1/2.  */
	bgez	a4, 1f
	addi	a2, a2, 1
	/* Overflow to the exponent is OK.  The answer will be correct.  */

	/* Check if the leftover fraction is exactly 1/2.  */
	slli	a4, a4, 1
	beqz	a4, .Ltrunc_exactlyhalf
1:	nop	
#if (XCHAL_HAVE_DFP || XCHAL_HAVE_FP) && XSHAL_FLOATING_POINT_ABI
	wfr	F0, a2
#endif
	leaf_return

.Ltrunc_exactlyhalf:
	/* Round down to the nearest even value.  */
	srli	a2, a2, 1
	slli	a2, a2, 1
#if (XCHAL_HAVE_DFP || XCHAL_HAVE_FP) && XSHAL_FLOATING_POINT_ABI
	wfr	F0, a2
#endif
	leaf_return

.Ltrunc_overflow:
	/* Check if exponent == 0x7ff.  */
	movi	a4, 0x7ff00000
	bnall	xh, a4, 1f

	/* Check if mantissa is nonzero.  */
	slli	a5, xh, 12
	or	a5, a5, xl
	beqz	a5, 1f

	/* Shift a4 to set a bit in the mantissa, making a quiet NaN.  */
	srli	a4, a4, 1

1:	slli	a4, a4, 4	/* 0xff000000 or 0xff800000 */
	/* Add the sign bit.  */
	extui	a6, xh, 31, 1
	ssai	1
	src	a2, a6, a4
#if (XCHAL_HAVE_DFP || XCHAL_HAVE_FP) && XSHAL_FLOATING_POINT_ABI
	wfr	F0, a2
#endif
	leaf_return

.Ltrunc_underflow:
	/* Find shift count for a subnormal.  Flush to zero if >= 32.  */
	extui	a6, xh, 20, 11
	movi	a5, 0x3ff - 0x7f
	sub	a6, a5, a6
	addi	a6, a6, 1
	bgeui	a6, 32, 1f

	/* Replace the exponent with an explicit "1.0".  */
	slli	a5, a5, 13	/* 0x700000 */
	or	a5, a5, xh
	slli	a5, a5, 11
	srli	a5, a5, 11

	/* Shift the mantissa left by 3 bits (into a5/a4).  */
	ssai	(32 - 3)
	src	a5, a5, xl
	sll	a4, xl

	/* Shift right by a6.  */
	ssr	a6
	sll	a7, a4
	src	a4, a5, a4
	srl	a5, a5
	beqz	a7, .Ltrunc_addsign
	or	a4, a4, a6	/* any positive, nonzero value will work */
	j	.Ltrunc_addsign

	/* Return +/- zero.  */
1:	extui	a2, xh, 31, 1
	slli	a2, a2, 31
#if (XCHAL_HAVE_DFP || XCHAL_HAVE_FP) && XSHAL_FLOATING_POINT_ABI
	wfr	F0, a2
#endif
	leaf_return
	.size __truncdfsf2, . - __truncdfsf2

#endif /* L_truncdfsf2 */

#ifdef L_extendsfdf2

	.align	4
	.global	__extendsfdf2
	.type	__extendsfdf2, @function
__extendsfdf2:
	leaf_entry sp, 16

#if (XCHAL_HAVE_DFP || XCHAL_HAVE_FP) && XSHAL_FLOATING_POINT_ABI
	rfr	a2, F0
#endif
	/* Save the sign bit and then shift it off.  */
	extui	a5, a2, 31, 1
	slli	a5, a5, 31
	slli	a4, a2, 1

	/* Extract and check the exponent.  */
	extui	a6, a2, 23, 8
	beqz	a6, .Lextend_expzero
	addi	a6, a6, 1
	beqi	a6, 256, .Lextend_nan_or_inf

	/* Shift >> 3 into a4/xl.  */
	srli	a4, a4, 4
	slli	xl, a2, (32 - 3)

	/* Adjust the exponent bias.  */
	movi	a6, (0x3ff - 0x7f) << 20
	add	a4, a4, a6

	/* Add the sign bit.  */
	or	xh, a4, a5
	leaf_return

.Lextend_nan_or_inf:
	movi	a4, 0x7ff00000

	/* Check for NaN.  */
	slli	a7, a2, 9
	beqz	a7, 1f

	slli	a6, a6, 11	/* 0x80000 */
	or	a4, a4, a6

	/* Add the sign and return.  */
1:	or	xh, a4, a5
	movi	xl, 0
	leaf_return

.Lextend_expzero:
	beqz	a4, 1b

	/* Normalize it to have 8 zero bits before the first 1 bit.  */
	do_nsau	a7, a4, a2, a3
	addi	a7, a7, -8
	ssl	a7
	sll	a4, a4
	
	/* Shift >> 3 into a4/xl.  */
	slli	xl, a4, (32 - 3)
	srli	a4, a4, 3

	/* Set the exponent.  */
	movi	a6, 0x3fe - 0x7f
	sub	a6, a6, a7
	slli	a6, a6, 20
	add	a4, a4, a6

	/* Add the sign and return.  */
	or	xh, a4, a5
#if XCHAL_HAVE_DFP && XSHAL_FLOATING_POINT_ABI
	wfrd	F0, xh, xl
#endif
	leaf_return

	.size __extendsfdf2, . - __extendsfdf2
#endif /* L_extendsfdf2 */


#if !XCHAL_HAVE_USER_DPFPU && !XCHAL_HAVE_USER_SPFPU && !XCHAL_HAVE_VECTORFPU2005
#if XCHAL_HAVE_DFP_SQRT
#ifdef L_sqrt

// Implemented here with the funny name so that all of libm's internal
// functions that call it get the benefit.  Without the dfp acceleration
// tie this gets implemented inside libm.
//
// extern double __ieee754_sqrt(double);
        .text
        .align 4
        .global __ieee754_sqrt
        .type __ieee754_sqrt,@function

// input params (for little endian):
// a2 low part of the double
// a3 high part of the double 
// return value in same regs, same order

__ieee754_sqrt:
        leaf_entry   a1, 16

#if XCHAL_HAVE_DFP && XSHAL_FLOATING_POINT_ABI
	mov.d 		f1, f0
#else
	wfrd		f1, xh, xl
#endif

	sqrt0.d		f2, f1
	const.d		f4, 0
	maddn.d		f4, f2, f2
	nexp01.d	f3, f1
	const.d		f0, 3
	addexp.d	f3, f0
	maddn.d		f0, f4, f3
	nexp01.d	f4, f1
	maddn.d		f2, f0, f2
	const.d		f5, 0
	maddn.d		f5, f2, f3
	const.d		f0, 3
	maddn.d		f0, f5, f2
	neg.d		f6, f4
	maddn.d		f2, f0, f2
	const.d		f0, 0
	const.d		f5, 0
	const.d		f7, 0
	maddn.d		f0, f6, f2
	maddn.d		f5, f2, f3
	const.d		f3, 3
	maddn.d		f7, f3, f2
	maddn.d		f4, f0, f0
	maddn.d		f3, f5, f2
	neg.d		f2, f7
	maddn.d		f0, f4, f2
	maddn.d		f7, f3, f7
	mksadj.d	f2, f1
	nexp01.d	f1, f1
	maddn.d		f1, f0, f0
	neg.d		f3, f7
	addexpm.d	f0, f2
	addexp.d	f3, f2
	divn.d		f0, f1, f3

	rfr		xl, f0
	rfrd		xh, f0

	leaf_return
	.size	__ieee754_sqrt, . - __ieee754_sqrt

#endif /* L_sqrt */
#endif /* XCHAL_HAVE_DFP_SQRT */

#if XCHAL_HAVE_DFP_RECIP
#ifdef L_recipdf2
	/* Reciprocal */

	.align	4
	.global	__recipdf2
	.type	__recipdf2, @function
__recipdf2:
	leaf_entry sp, 16

#if XCHAL_HAVE_DFP && XSHAL_FLOATING_POINT_ABI
	mov.d 		f1, f0
#else
	wfrd		f1, xh, xl
#endif

	recip0.d	f0, f1
	const.d		f2, 2
	msub.d		f2, f1, f0
	mul.d		f3, f1, f0
	const.d		f4, 2 
	mul.d		f5, f0, f2
	msub.d		f4, f3, f2
	const.d		f2, 1
	mul.d		f0, f5, f4
	msub.d		f2, f1, f0
	maddn.d		f0, f0, f2

	rfr		xl, f0
	rfrd		xh, f0

	leaf_return
	.size	__recipdf2, . - __recipdf2

#endif /* L_recipdf2 */
#endif /* XCHAL_HAVE_DFP_RECIP */

#if XCHAL_HAVE_DFP_RSQRT
#ifdef L_rsqrtdf2
	/* Reciprocal square root */

	.align	4
	.global	__rsqrtdf2
	.type	__rsqrtdf2, @function
__rsqrtdf2:
	leaf_entry sp, 16

#if XCHAL_HAVE_DFP && XSHAL_FLOATING_POINT_ABI
	mov.d 		f1, f0
#else
	wfrd		f1, xh, xl
#endif

	rsqrt0.d	f0, f1
	mul.d		f2, f1, f0
	const.d		f3, 3
	mul.d		f4, f3, f0
	const.d		f5, 1
	msub.d		f5, f2, f0
	maddn.d		f0, f4, f5
	const.d		f2, 1
	mul.d		f4, f1, f0
	mul.d		f5, f3, f0
	msub.d		f2, f4, f0
	maddn.d		f0, f5, f2
	const.d		f2, 1
	mul.d		f1, f1, f0
	mul.d		f3, f3, f0
	msub.d		f2, f1, f0
	maddn.d		f0, f3, f2

	rfr		xl, f0
	rfrd		xh, f0

	leaf_return
	.size	__rsqrtdf2, . - __rsqrtdf2
#endif /* L_rsqrtdf2 */
#endif /* XCHAL_HAVE_DFP_RSQRT */
#endif



