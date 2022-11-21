/*
 * Copyright (c) 2006 by Tensilica Inc.  ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of Tensilica Inc.
 * They may not be modified, copied, reproduced, distributed, or disclosed to
 * third parties in any manner, medium, or form, in whole or in part, without
 * the prior written consent of Tensilica Inc.
 */

#include <xtensa/coreasm.h>
	
	.global		_gdbio_syscall
	.p2align	2

_gdbio_syscall:
	abi_entry
#if XCHAL_HAVE_XEA2 || XCHAL_HAVE_XEA3
#if XCHAL_HAVE_DEBUG
	break	1, 14
#else
	movi	a2, -1
#endif
#endif
#if XCHAL_HAVE_XEA5
	ebreak		// TODO xt_break
#endif
	abi_return

	.size _gdbio_syscall, . - _gdbio_syscall
