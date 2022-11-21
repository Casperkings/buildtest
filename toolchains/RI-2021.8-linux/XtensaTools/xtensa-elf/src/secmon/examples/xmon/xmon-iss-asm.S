// Copyright (c) 2002-2021 Cadence Design Systems, Inc.
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

// xmon-iss-asm.S -- Instruction Set Simulator (ISS) specific XMON assembler functions


#include <xtensa/coreasm.h>
#include <xtensa/simcall.h>
#include <xtensa/config/specreg.h>

#if (defined __XTENSA_WINDOWED_ABI__) && __XTENSA_WINDOWED_ABI__
# define _RET		retw
# define _ENTRY(sp,N)	entry	sp, N
#else
# define _RET		ret
# define _ENTRY(r,N)

#endif

	.global	_xmon_iss_out
	.global	_xmon_iss_in
	.global	_xmon_iss_in_rdy
	.global	_xmon_iss_flush
	.global	_xmon_iss_init
	.global	_xmon_closesim

	.text

	.align	4
_xmon_iss_out:
	_ENTRY(sp,64)
	mov	a3, a2  // pass the 2nd arg as the first arg.
	movi	a2, SYS_xmon_out
	simcall
	_RET

	.align	4
_xmon_iss_in_rdy:
	_ENTRY(sp,64)
	movi	a2, SYS_xmon_in_rdy
	simcall
	_RET

	.align	4
_xmon_iss_in:
	_ENTRY(sp,64)
	movi	a2, SYS_xmon_in
	simcall
	_RET

	.align	4
_xmon_iss_flush:
	_ENTRY(sp,64)
	movi	a2, SYS_xmon_flush
	simcall
	_RET

	.align	4
_xmon_iss_init:
	_ENTRY(sp,64)
	mov	a5, a4  // pass the 4th arg as the 3rd arg.
	mov	a4, a3  // pass the 3rd arg as the 2nd arg.
	mov	a3, a2  // pass the 2nd arg as the 1st arg.
	movi	a2, SYS_xmon_init
	simcall
	_RET

	.align	4
_xmon_closesim:
	_ENTRY(sp,64)
	movi	a2, SYS_xmon_close
	simcall
	_RET

	.align 4
.global _xmon_iss_crash
_xmon_crash:
	_ENTRY(sp,64)
	.byte   0,0,0,0,0,0
	_RET
