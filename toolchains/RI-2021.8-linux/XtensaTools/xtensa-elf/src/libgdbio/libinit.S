/*
 * Copyright (c) 2006-2021 by Tensilica Inc.  ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of Tensilica Inc.
 * They may not be modified, copied, reproduced, distributed, or disclosed to
 * third parties in any manner, medium, or form, in whole or in part, without
 * the prior written consent of Tensilica Inc.
 */

#include <xtensa/coreasm.h>
#include <xtensa/xtensa-versions.h>
#include <xtensa/simcall.h>

	.data
	.p2align 	2

	// This flag controls the IO mode.
	.global	libio_mode

libio_mode:	.word	0

	// Overrides for weak definitions in crt1.
	.global		_start_argc
	.global		_start_envp

_start_argc:
	.word	1		// default one arg (progname)
_start_envp:
	.word	0		// no environment

	.p2align	2
	.global		_start_argv

_start_argv:			// space for arg list (default all zeros)
	.space	256

	// This code goes into _init and is run before main()
	// is called.

	.section .init, "ax"

#if XCHAL_HAVE_XEA2 || XCHAL_HAVE_XEA3
#if XCHAL_HW_MIN_VERSION < XTENSA_HWVERSION_RE_2013_2
	// Cannot use SIMCALL for detection on older hardware.
	// Support GDBIO mode only.
	movi	a3, libio_mode
	movi	a2, 2			// LIBIO_GDB
	s32i	a2, a3, 0
#else
	movi	a2, SYS_cycle_count
	movi	a3, 0x55
	mov	a4, a2
	mov	a5, a3
	simcall				// ISS presence check
	bne	a2, a4, .Liss
	bne	a3, a5, .Liss
.Lnoiss:
	movi	a2, 2			// LIBIO_GDB
	j	.Lend
.Liss:
	movi	a2, SYS_iss_argv_size
	simcall				// returns size of argv[] + its strings in a2
	bgei	a2, 256, .L1		// buffer too small
	movi	a3, _start_argv		// address of buffer
	movi	a2, SYS_iss_set_argv	// write argv[] at a3
	simcall
	movi	a2, SYS_iss_argc	// put argc in a2
	simcall
	movi	a3, _start_argc
	s32i	a2, a3, 0		// save argc
.L1:
	movi	a2, 1			// LIBIO_SIM
.Lend:
	movi	a3, libio_mode
	s32i	a2, a3, 0
#endif
#endif

#if XCHAL_HAVE_XEA5
	li	a0, SYS_cycle_count
	li	a1, 0x55
	mv	a2, a0
	mv	a3, a1
	xt_simcall			// ISS presence check
	bne	a2, a0, .Liss
	bne	a3, a1, .Liss
.Lnoiss:
	li	a0, 2			// LIBIO_GDB
	j	.Lend
.Liss:
	li	a0, SYS_iss_argv_size
	xt_simcall			// returns size of argv[] + its strings in a0
	li	a1, 256
	bge	a2, a1, .L1		// buffer too small
	la	a1, _start_argv		// address of buffer
	li	a0, SYS_iss_set_argv	// write argv[] at a1
	xt_simcall
	li	a0, SYS_iss_argc	// put argc in a0
	xt_simcall
	la	a2, _start_argc
	sw	a0, a2, 0		// save argc
.L1:
	li	a0, 1			// LIBIO_SIM
.Lend:
	la	a2, libio_mode
	sw	a0, a2, 0
#endif

