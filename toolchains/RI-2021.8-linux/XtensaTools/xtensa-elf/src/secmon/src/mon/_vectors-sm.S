// Copyright (c) 2004-2021 Cadence Design Systems, Inc.
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

// _vectors-sm.S -- Reference symbols to pull in all required vectors


#include <xtensa/coreasm.h>
#include <xtensa/config/system.h>	/* for XSHAL_VECTORS_PACKED */

#include "xtos-internal.h"

.global	_ResetVector

# ifdef XCHAL_DOUBLEEXC_VECTOR_VADDR
.global	_DoubleExceptionVector
# endif

# if XCHAL_HAVE_DEBUG
.global	_DebugExceptionVector
# endif

.global	_KernelExceptionVector
.global	_UserExceptionVector

# if XCHAL_HAVE_NMI
.global	_NMIExceptionVector
# endif

# if XCHAL_HAVE_MEM_ECC_PARITY
.global	_MemErrorVector
# endif


#if (XCHAL_NUM_INTLEVELS >= 2) && (!IS_DEBUGLEVEL(2))
.global	_Level2Vector
#endif
#if (XCHAL_NUM_INTLEVELS >= 3) && (!IS_DEBUGLEVEL(3))
.global	_Level3Vector
#endif
#if (XCHAL_NUM_INTLEVELS >= 4) && (!IS_DEBUGLEVEL(4))
.global	_Level4Vector
#endif
#if (XCHAL_NUM_INTLEVELS >= 5) && (!IS_DEBUGLEVEL(5))
.global	_Level5Vector
#endif
#if (XCHAL_NUM_INTLEVELS >= 6) && (!IS_DEBUGLEVEL(6))
.global	_Level6Vector
#endif

/*  These don't take up space:  */
.global	xthals_hw_configid0
.global	xthals_hw_configid1
.global	xthals_release_major
.global	xthals_release_minor

/*  The following allows this object file to be pulled in by reference:  */
.text
.global	_xtos_vectors_ref_
.set	_xtos_vectors_ref_, 0


