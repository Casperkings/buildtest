// exc_default_handler.S  -  Default exception handler for XEA5.

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


//-----------------------------------------------------------------------------
// Storage space needed by default exception handler.
// NOTE: These saved registers will be restored by the ISS. The layout must
// match that assumed by the ISS. A pointer to this area will be in a1 (x11)
// when the simcall is executed.
//-----------------------------------------------------------------------------

	.data
	.p2align	2
	.local		xtos_exc_savearea

xtos_exc_savearea:
	.word		0	// PC
	.word		0	// cause
	.word		0	// tval
	.word		0	// a0/x10
	.word		0	// a1/x11

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

	.text

	.global		xtos_default_exc_handler
	.type		xtos_default_exc_handler,@function
	.p2align	2

xtos_default_exc_handler:

	mv	a1, a0				// Exc frame pointer
	lw	a0, a1, EXC_cause
	csrw	mcause, a0			// Restore mcause

#if defined (SIMULATOR)

	la	a2, xtos_exc_savearea
	sw	a0, a2, 4			// Save cause
	lw	a0, a1, EXC_tval
	csrw	mtval, a0			// Restore mtval
	sw	a0, a2, 8			// Save tval
	lw	a0, a1, EXC_pc
	sw	a0, a2, 0			// Save PC
	lw	a0, a1, EXC_a0
	sw	a0, a2, 12			// Save a0
	lw	a0, a1, EXC_a1
	sw	a0, a2, 16			// Save a1
	lw	a2, a1, EXC_a2
	la	a1, xtos_exc_savearea
	li	a0, SYS_gdb_enter_sktloop
	simcall

#else // SIMULATOR

#endif

	.size	xtos_default_exc_handler, . - xtos_default_exc_handler

