// 
// miscellaneous.S - miscellaneous assembly language routines
//
// $Id: //depot/rel/Homewood/ib.8/Xtensa/OS/hal/miscellaneous.S#1 $

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
#include <xtensa/cacheasm.h>
#include <xtensa/cacheattrasm.h>
#include <xtensa/xtensa-versions.h>


//----------------------------------------------------------------------
// These functions are defined as inline C functions in core-macros.h.
// The assembly language versions defined in this file are for assembly language
// callers or link-time HAL.

	.text

//----------------------------------------------------------------------
// Clear any remnant register state pointing to (or containing) code.
// Specifically, clear loop registers (LCOUNT) to avoid hardware loopback
// from LEND to LBEGIN when new code is loaded where code containing a
// zero-overhead loop was located.  See the HAL chapter of the Tensilica
// System Software Reference Manual for details on the use of this function.
//----------------------------------------------------------------------

#if defined(__SPLIT__clear_regcached_code) || \
    defined(__SPLIT__clear_regcached_code_nw)

DECLFUNC(xthal_clear_regcached_code)
	abi_entry
#if XCHAL_HAVE_XEA5
// TODO
#else
# if XCHAL_HAVE_LOOPS
	movi		a2, 0
	wsr.lcount	a2
# endif
	isync_retw_nop
	abi_return
	endfunc
#endif
#endif

#if XCHAL_HAVE_XEA2

#if defined(__SPLIT__get_intenable) || \
    defined(__SPLIT__get_intenable_nw)

//----------------------------------------------------------------------
// Access INTENABLE register from C
//----------------------------------------------------------------------

// unsigned xthal_get_intenable(void)
//
DECLFUNC(xthal_get_intenable)
	abi_entry
# if XCHAL_HAVE_INTERRUPTS
	rsr.intenable	a2
# else
	movi	a2, 0	// if no INTENABLE (no interrupts), tell caller nothing is enabled
# endif
	abi_return
	endfunc


#elif defined(__SPLIT__set_intenable) || \
      defined(__SPLIT__set_intenable_nw)

// void xthal_set_intenable(unsigned)
//
DECLFUNC(xthal_set_intenable)
	abi_entry
# if XCHAL_HAVE_INTERRUPTS
	wsr.intenable	a2
# endif
	abi_return
	endfunc


//----------------------------------------------------------------------
// Access INTERRUPT, INTSET, INTCLEAR register from C
//----------------------------------------------------------------------

#elif defined(__SPLIT__get_interrupt) || \
      defined (__SPLIT__get_interrupt_nw)

// unsigned xthal_get_interrupt(void)
//
DECLFUNC (xthal_get_interrupt)
	abi_entry
# if XCHAL_HAVE_INTERRUPTS
	rsr.interrupt	a2
# else
	movi	a2, 0	// if no INTERRUPT (no interrupts), tell caller nothing is pending
# endif
	abi_return
	endfunc

#elif defined(__SPLIT__get_intread) || \
      defined(__SPLIT__get_intread_nw)

DECLFUNC (xthal_get_intread)
	abi_entry
# if XCHAL_HAVE_INTERRUPTS
	rsr.interrupt	a2
# else
	movi	a2, 0	// if no INTERRUPT (no interrupts), tell caller nothing is pending
# endif
	abi_return
	endfunc
#endif

#endif // XCHAL_HAVE_XEA2

