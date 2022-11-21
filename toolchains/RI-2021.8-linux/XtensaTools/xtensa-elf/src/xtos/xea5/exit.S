// exit.S
//
// For hardware / boards, this is the default _exit routine called by the
// C library exit() function.  If the program ever exits, we eventually
// end up here after all C library cleanup (such as closing open files,
// calling exit callbacks and C++ destructors, etc) is complete.

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
#include <xtensa/config/core-isa.h>
#include <xtensa/simcall.h>
#include "xtos-internal.h"


	.text
	.p2align	2
	.global		_exit
	.type		_exit, @function

_exit:
	addi	sp, sp, -16
	sw	a0, sp, 0	// save exit code

	// sync dirty data to memory before terminating
#if XCHAL_DCACHE_IS_WRITEBACK
#if XCHAL_HAVE_L2_CACHE
	call	xthal_async_dcache_all_writeback
	call	xthal_async_dcache_wait
#else
	call	xthal_dcache_all_writeback
#endif
#endif

.L1:
	li	a0, SYS_exit
	lw	a1, sp, 0	// exit code
	xt_simcall

	mv	a0, a1		// exit code for debugger
	ebreak
	j	.L1	

	.size	_exit, . - _exit

