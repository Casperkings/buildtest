// crt1-tiny.S for XEA5.
//
// This is a reduced version of the code in crt1.S .
// For most hardware / boards, this code sets up the C calling context
// and calls main().
// It has some limitations (see LSP Ref Manual for details) such as:
//	- does not setup the C library (...)
//	- does not call C++ static constructors and destructors
//	- only clears .bss , not other *.bss sections
//
// Control arrives here at _start from the reset vector or from crt0-app.S.

// Copyright (c) 1998-2021 Cadence Design Systems, Inc.
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
#include <xtensa/xtensa-versions.h>
#include <xtensa/simcall.h>
#include "xtos-internal.h"


// Imports
//	__stack			from linker script (see LSP Ref Manual)
//	_bss_start		from linker script (see LSP Ref Manual)
//	_bss_end		from linker script (see LSP Ref Manual)
//	main			from user application


/**************************************************************************/

	.text
	.p2align	2
	.global		_start
_start:
	//  _start is typically NOT at the beginning of the text segment --
	//  it is always called from either the reset vector or other code
	//  that does equivalent initialization (such as crt0-app.S).
	//  See crt1.S for assumptions on entry to _start ,
	//  and for comments on what is being done in this file.

	la	sp, __stack	// setup the stack

	//  Enable interrupts

	csrrsi	x0, mstatus, 0x8	// FIXME

	// Clear the BSS (uninitialized data) segment.
	//
	// This code only supports .bss, not multiple *.bss sections.
	// Corresponding code in crt1.S does, and is faster but bigger.

	la	t0, _bss_start
	la	t1, _bss_end
	bgeu	t0, t1, 2f
1:
	sw	x0, t0, 0
	addi	t0, t0, 4
	bltu	t0, t1, 1b
2:

	//  We can now call C code, the C calling environment is initialized.
	//  This tiny C runtime assumes main is declared as "void main(void)"
	//  rather than with the usual argc,argv.  So there are no arguments.

	call	main

	//  In this tiny C runtime, main() is not expected to return.
	//  If it does, just loop forever.

.L0:
	li	a0, SYS_exit
	xt_simcall		// exit if in simulator, else NOP
	j	.L0

	.size	_start, . - _start

