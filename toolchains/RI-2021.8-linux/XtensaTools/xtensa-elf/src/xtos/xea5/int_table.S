// int_table.S - Interrupt Handler table and default handler for XEA5.

// Copyright (c) 2003-2021 Cadence Design Systems, Inc.
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
#include "../xtos-internal.h"


#if XCHAL_HAVE_INTERRUPTS

	// Interrupt Handler Table

	.data

	// This name is used by the reset handler to set up the MTVT.
	// Non-XTOS applications must use the same name.
	.global		_xt_interrupt_table
	.weak		_xt_interrupt_table

	// This name is used by XTOS internally.
	.global		xtos_interrupt_table

	// The table alignment required is a function of the max number
	// of interrupts configured. We don't want to deal with that so
	// set the alignment to 4KB for now.

	.p2align	12

_xt_interrupt_table:
xtos_interrupt_table:

	.set	i, 0
	.rept	XCHAL_NUM_INTERRUPTS
		.word	xtos_unhandled_interrupt	// default handler
	 	.word	i				// default arg - intr number
		.set	i, i+1
	.endr

	// Default Interrupt Handler

	.text
	.p2align	2
	.global		xtos_unhandled_interrupt
	.type		xtos_unhandled_interrupt,@function

xtos_unhandled_interrupt:
	abi_entry
#if XCHAL_HAVE_DEBUG
	//TODO break	1, 15		// break into debugger if present
#else
1:
	j	1b		// loop forever
# endif
	abi_return

	.size	xtos_unhandled_interrupt, . - xtos_unhandled_interrupt

#endif /* XCHAL_HAVE_INTERRUPTS */

