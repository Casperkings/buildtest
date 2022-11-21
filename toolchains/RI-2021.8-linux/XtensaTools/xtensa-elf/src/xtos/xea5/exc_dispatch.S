// exc_dispatch.S  -  Exception dispatch for XEA5

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
#include <xtensa/corebits-xea5.h>
#include <xtensa/xtruntime-frames.h>
#if defined (SIMULATOR)
#include <xtensa/simcall.h>
#endif


	.text

//-----------------------------------------------------------------------------
// The MTVEC dispatch vector transfers here if an exception has occurred.
// On entry,
//   a0/a1/a2/cause/pc/sp/ra have been saved into the exception frame.
//   a0 holds cause
//-----------------------------------------------------------------------------

	//.weakref	_exc_table, xtos_exc_handler_table

	.global		_xtos_exc_dispatch
	.type		_xtos_exc_dispatch,@function
	.p2align	2

_xtos_exc_dispatch:
	csrr	a1, mtval
	sw	a1, sp, EXC_tval
	li	a2, MCAUSE_EXCCODE_MASK
	la	a1, xtos_exc_handler_table // _exc_table
	and	a0, a0, a2
	li	a2, XCHAL_EXCCAUSE_NUM
	bgeu	a0, a2, handle_default
	slli	a0, a0, 2
	add	a1, a1, a0
	lw	a1, a1, 0
	mv	a0, sp				// Exc frame pointer
	jalr	x0, a1

handle_default:
	la	a1, xtos_default_exc_handler
	mv	a0, sp
	jalr	x0, a1

	.size	_xtos_exc_dispatch, . - _xtos_exc_dispatch

