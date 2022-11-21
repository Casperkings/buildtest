
// crt1.S for XEA5

// Sets up the C calling environment (sets up stack, clears BSS sections,
// calls __clibrary_init) and then calls main(). Control arrives here from
// the reset handler.

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


#if defined (SIMULATOR)
#include <xtensa/simboard.h>
#include <xtensa/simcall.h>
#endif

#include <xtensa/coreasm.h>
#include <xtensa/corebits-xea5.h>
#include "xtos-internal.h"

	// Exports
	.global	_start
	.type	_start,@function

	// External Functions
	//
	// __clibrary_init		from C library
	// _init			from C library
	// _fini			from C library
	// _init_reent_bss		from C library
	// main				from user code
	// exit				from C library
	// board_init			from BSP/user code
	// xthal_dcache_all_writeback	from HAL
	// __memmap_init		(optional) user code
	// __bss_init			(optional) user code
	// __stack_init			(optional) user code

	// External Symbols/Variables
	//
	// __stack			from linker script or user code
	// _bss_table_start		from linker script
	// _bss_table_end		from linker script
	// __xtos_simc			from linker script or user code


	.data

#if !defined (SIMULATOR)
	.weak	_start_argc
	.weak	_start_argv

	.p2align	2
_start_argc:
	.word	1		// one argument (program name)
_start_argv:
	.word	_start_null	// empty program name
#endif

	.weak	_start_envp

	.p2align	2
_start_envp:
_start_null:
	.word	0		// end of argv array, empty string


	.text
	.p2align	2

_start:
	//  _start is typically NOT at the beginning of the text segment --
	//  it is always called from either the reset vector or other code
	//  that does equivalent initialization (such as crt0-app.S).
	//
	//  Assumptions on entry to _start:
	//	- Interrupts are disabled via mstatus.mie/sie. Enable before
	//	  calling main.
	//	- C calling context not initialized
	//	- SP not initialized
	//	- The following are initialized:
	//	  - CPENABLE, loop regs, FCR/FSR (if present)
	//	- mtvec

	// Set up the stack pointer. It is possible to override the default
	// stack location by providing a user hook __stack_init. This hook
	// must not rely on stack and BSS availability.

	.weak	__stack_init
	la	t0, __stack_init	// optional user hook
	la	sp, __stack		// default SP
	beq	t0, x0, 1f
	jalr	t0
1:

#if defined (SIMULATOR)
	// Reserve stack space for
	//    - argv array
	//    - argument strings

	li	a0, SYS_iss_argv_size
	xt_simcall			// returns size of argv[] + its strings in a0

	// The stack only needs 16-byte alignment.
	// However, here we round up the argv size further to 128 byte multiples
	// so that in most cases, variations in argv[0]'s path do not result in
	// different stack allocation.  Otherwise, such variations can impact
	// execution timing (eg. due to cache effects etc) for the same code and data.
	// If we have an outbound bus, it's more likely there's enough memory.

# if XCHAL_HAVE_PIF
	addi	a0, a0, 127
	srli	a0, a0, 7
	slli	a0, a0, 7
# else
	// Keep stack 16-byte aligned.
	addi	a0, a0, 15
	srli	a0, a0, 4
	slli	a0, a0, 4
# endif

	sub	sp, sp, a0		// Adjust SP
#endif	// SIMULATOR

	// Enable interrupts/exceptions.
	csrrsi	x0, mstatus, MSTATUS_MIE

	// Do any initialization that affects the memory map, such as setting
	// up TLB entries, that needs to be done before we can successfully
	// clear BSS (e.g. if some BSS segments are in remapped areas).
	//
	// NOTE: This hook works where the reset vector does not unpack segments
	// (see "ROM packing" in the LSP manual), or where unpacking of segments
	// is not affected by memory remapping. If ROM unpacking is affected,
	// TLB setup must be done in assembler from the reset handler.
	//
	// The __memmap_init() routine can be a C function, however it does not
	// have BSS initialized! In particular, __memmap_init() cannot set BSS
	// variables, i.e. uninitialized global variables (they'll be wiped out
	// by the following BSS clear), nor can it assume they are yet initialized
	// to zero.

	.weak	__memmap_init
	la	t0, __memmap_init	// optional memory map init hook
	beq	t0, x0, 1f
	jalr	t0
1:

	// Clear the BSS segments. This code supports multiple BSS areas.

	la	a0, _bss_table_start
	la	a1, _bss_table_end
	bgeu	a0, a1, .Lnobss		// No BSS areas if a0 >= a1

	// If there is an optional BSS init routine provided, we will call
	// it instead of clearing BSS here. This routine can be a C function,
	// but it must be written to be able to work with BSS not initialized.

	.weak	__bss_init
	la	t0, __bss_init
	beq	t0, x0, 2f
	jalr	t0			// a0 = start, a1 = end
	j	.Lnobss			// skip default BSS init code
2:

	// If running on simulator use simcall for memset.
	// Else clear memory the hard way.

#if defined (SIMULATOR)
	mv	t0, a0
	mv	t1, a1
.Lbssloop:
	li	a0, SYS_memset
	lw	a1, t0, 0		// arg1 = fill start address
	li	a2, 0			// arg2 = fill pattern
	lw	a3, t0, 4		// get end address
	addi	t0, t0, 8		// next bss table entry
	sub	a3, a3, a1		// arg3 = fill size in bytes
	xt_simcall			// memset(a1,a2,a3)
	bltu	t0, t1, .Lbssloop	// loop until end of bss table
#else
.L0zte:
	lw	t0, a0, 0		// get start address, assumed multiple of 4
	lw	t1, a0, 4		// get end address, assumed multiple of 4
	addi	a0, a0, 8		// next entry
	sub	t2, t1, t0		// t2 = length, assumed a multiple of 4
	li	t3, 16
.L1zte:
	blt	t2, t3, .L2zte
	sw	x0, t0, 0
	sw	x0, t0, 4
	sw	x0, t0, 8
	sw	x0, t0, 12
	addi	t0, t0, 16
	addi	t2, t2, -16
	bne	t2, x0, .L1zte
.L2zte:
	beq	t2, x0, .L3zte
	sw	x0, t0, 0
	addi	t0, t0, 4
	addi	t2, t2, -4
	j	.L2zte
.L3zte:
	bltu	a0, a1, .L0zte
#endif	// SIMULATOR

.Lnobss:

	// Call __clibrary_init to initialize the C library:
	//
	// void __clibrary_init(int argc, char ** argv, char ** environ,
	//	void(*init_func)(void), void(*fini_func)(void));

#if defined (SIMULATOR)
	// Get argv with the arguments from the ISS
	li	a0, SYS_iss_set_argv
	mv	a1, sp			// tell simcall where to write argv[]
	xt_simcall			// write argv[] array at a1
	li	a0, SYS_iss_argc
	xt_simcall			// put argc in a0
	mv	s1, a0
#else
	// Call board-specific BSP or user provided init function
	call	board_init
	la	a0, _start_argc
	la	a1, _start_argv		// argv = ["", 0]
	lw	a0, a0, 0		// argc = 1
#endif	// SIMULATOR

#if XCHAL_HAVE_PRID && (XSHAL_CLIB == XTHAL_CLIB_XCLIB)
	.weak	__xtos_simc

	la	t0, __xtos_simc		// externally defined symbol
	beq	t0, x0, 1f
	mv	t1, x0	//FIXME xt_core_id t1
	beq	t1, x0, 1f		// core 0, skip ahead
	call	_init_reent_bss		// limited init for core != core0
	j	.Lmain
1:
#endif

	la 	a2, _start_envp		// envp
	la 	a3, _init		// _init
	la	a4, _fini		// _fini
	call	__clibrary_init

.Lmain:
	// Call: int main(int argc, char ** argv, char ** environ);

#if defined (SIMULATOR)
	mv	a0, s1			// argc
	mv	a1, sp			// argv
#else
	la	a0, _start_argc
	la	a1, _start_argv		// argv = ["", 0]
	lw	a0, a0, 0		// argc = 1
#endif	// SIMULATOR

	la	a2, _start_envp		// envp
	call	main

#if XCHAL_HAVE_PRID && (XSHAL_CLIB == XTHAL_CLIB_XCLIB)
	la	t0, __xtos_simc		// externally defined symbol
	beq	t0, x0, 1f
	mv	t1, x0	//FIXME xt_core_id t1
	beq	t1, x0, 1f		// core 0, skip ahead
	call	_exit			// skip exit processing for core != core0
1:
#endif
	// The return value is the same register as the first outgoing argument.

	call	exit			// exit with main's return value

	// Does not return here.

	.size	_start, . - _start

