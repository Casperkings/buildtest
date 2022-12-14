// 
// mp_asm.S - multi-processor synchronization routines


// Copyright (c) 1999-2018 Cadence Design Systems, Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include <xtensa/coreasm.h>

#if !XCHAL_HAVE_XEA5

	.begin	schedule	// turn on assembler scheduling for this file

/*
  int32_t xthal_compare_and_set( int32_t* address, int32_t test_value, int32_t set_value )

  Atomically sets *address to set_value if *address equals test_value.
  Returns the previous value of *address (the one compared with test_value).

  Uses the S32C1I instruction if available.
  S32C1I requires special support from the memory controller for
  memory accessed via the PIF interface.  For this and other reasons,
  S32C1I might not work on the entire 4GB address range.  This function
  does not test address validity.  That is the responsibility of the
  software invoking this function.
*/
	.text
	.align 4
	.global xthal_compare_and_set
	.type	xthal_compare_and_set,@function

xthal_compare_and_set:
	abi_entry
	// a2 == address
	// a3 == test value
	// a4 == set value

#if XCHAL_HAVE_EXCLUSIVE
	mov	a6, a4		// a6 = copy of set_value
1:
	l32ex	a5, a2		// a5 = *address, set monitor
	bne	a5, a3, 2f	// skip write if *address != test_value
	mov	a4, a6		// a4 = set_value
	s32ex	a4, a2		// *address = set_value
	getex	a4		// get result of store
	beqz	a4, 1b
2:
	mov	a2, a5		// a2 = *address, return value
	clrex			// in case we skipped write
#elif XCHAL_HAVE_S32C1I && XCHAL_HW_MIN_VERSION_MAJOR >= 2200
	mov	a6, a4		// a6 = copy of set_value
	movi	a5, -1
	xor	a5, a5, a3	// a5 = ~a3
	wsr.scompare1	a3	// set test_value
1:
	mov	a4, a6		// a4 = set_value
	s32c1i	a4, a2, 0
	bne	a4, a5, 2f	// if a4 != ~SCOMPARE1 then done
	l32i	a4, a2, 0	// a4 = *address
	bne	a4, a5, 1b	// retry if *address != ~SCOMPARE1
2:
	mov	a2, a4
#else
	mov	a7, a2		// a7 == address, a2 is return val
# if XCHAL_HAVE_INTERRUPTS
#  if XCHAL_HAVE_XEA3
	movi	a5, PS_DI
	xps	a5, a5		// disable interrupts, a5 = old PS
#  else
	rsil	a5, 15		// disable interrupts, a5 = old PS
#  endif
# endif
	l32i	a2, a7, 0	// a2 == value to test, return val
	bne	a3, a2, done	// test

	s32i	a4, a7, 0	// write the new value

done:
# if XCHAL_HAVE_INTERRUPTS
	wsr.ps	a5		// restore PS (interrupt state)
	rsync
# endif
#endif
	abi_return

	.size	xthal_compare_and_set, . - xthal_compare_and_set

	.end	schedule

#endif
