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

// debug-vector-sm.S -- Debug Exception Vector


#include <xtensa/xtensa-versions.h>
#include <xtensa/coreasm.h>
#include <xtensa/simcall.h>

#if XCHAL_HAVE_DEBUG && XCHAL_HAVE_EXCEPTIONS && (XCHAL_HAVE_XEA1 || XCHAL_HAVE_XEA2)

	//  This code goes at the debug exception vector

	.begin	literal_prefix	.DebugExceptionVector
	.section		.DebugExceptionVector.text, "ax"
	.align	4
	.global	_DebugExceptionVector
_DebugExceptionVector:

	writesr	excsave XCHAL_DEBUGLEVEL a2	// save a2 where simulator (whether present or not) expects it

	// If coming from secure code, skip nonsecure debug vector handling
	rsr.ps	a2
	extui	a2, a2, PS_PNSM_SHIFT, PS_PNSM_BITS
	beqz	a2, .Lsec_debug			// PS.PNSM == 0

	// Continue to nonsecure handling only if nonsecure virtual vector is registered
	movi	a2, secmon_nsm_dbg_handler
	l32i	a2, a2, 0
	beqz	a2, .Lsec_debug

.Lnsm_debug:
	movi	a2, secmon_dispatch_debug_to_nsm
	jx	a2

.Lsec_debug:

# if defined(SIMULATOR) || XCHAL_HW_MIN_VERSION >= XTENSA_HWVERSION_RE_2013_2	/* SIMCALL is NOP in hw? */
	//  In the simulator (ISS), let the debugger (if any is attached)
	//  handle the debug exception, else simply stop the simulation:
	//
	movi	a2, SYS_gdb_enter_sktloop
	simcall					// have ISS handle the debug exception
# endif
# ifndef SIMULATOR
	//  For hardware, this code does not handle debug exceptions.
	//  To implement a target-side debug monitor, replace this
	//  vector with a real one that uses target-specific facilities
	//  to communicate with the debugger.
	//  
1:
#  if XCHAL_HAVE_INTERRUPTS
	//waiti	XCHAL_DEBUGLEVEL // unexpected debug exception, loop in low-power mode
#  endif
	j	1b		// infinite loop - unexpected debug exception
# endif /*!SIMULATOR*/

	.end	literal_prefix
	.size	_DebugExceptionVector, . - _DebugExceptionVector


	// Prepare for return to nonsecure code
	.text
	.align	4
	.global	secmon_dispatch_debug_to_nsm
secmon_dispatch_debug_to_nsm:

	// Preserve original a0, a2
	movi	a2, _secmon_dbg_handler_scratch
	s32i	a0, a2, 0
	readsr	excsave XCHAL_DEBUGLEVEL a0     // excsave[DEBUGLEVEL] contains a2
	s32i	a0, a2, 4

	// Clear PS.EXCM prior to RFI
	rsr.ps	a2
	movi	a0, ~PS_RING_MASK
	and	a2, a0, a2
	wsr.ps	a2

	// Save eps[DEBUGLEVEL] into misc0
	readsr	eps XCHAL_DEBUGLEVEL a2
	wsr.misc0	a2

	// Set nonsecure PS (eps[DEBUGLEVEL): 
	// vector runs at DEBUGLEVEL; set PS.EXCM; clear ring (for nonsecure RFI)
	movi	a0, ~(PS_INTLEVEL_MASK | PS_RING_MASK)
	and	a2, a0, a2
	movi	a0, (PS_INTLEVEL(XCHAL_DEBUGLEVEL) | PS_EXCM)
	or	a2, a0, a2
	writesr	eps XCHAL_DEBUGLEVEL a2

	// Save original epc[DEBUGLEVEL] into excsave[DEBUGLEVEL] prior to updating
	readsr	epc XCHAL_DEBUGLEVEL a2
	writesr	excsave XCHAL_DEBUGLEVEL a2
	movi	a2, secmon_nsm_dbg_handler
	l32i	a2, a2, 0
	writesr	epc XCHAL_DEBUGLEVEL a2

	// Restore a0, a2 prior to nonsecure code transition
	movi	a2, _secmon_dbg_handler_scratch
	l32i	a0, a2, 0
	l32i	a2, a2, 4
	rfi	XCHAL_DEBUGLEVEL

	.size	secmon_dispatch_debug_to_nsm, . - secmon_dispatch_debug_to_nsm



	// Address to nonsecure virtual debug "vector".  Nonzero if installed.
	.data
	.align	4

	.global	secmon_nsm_dbg_handler
	.type	secmon_nsm_dbg_handler,@object
	.size	secmon_nsm_dbg_handler,4
secmon_nsm_dbg_handler:
	.word	0


	// Scratch space for secure debug exception handler
	.align	4
	.global	_secmon_dbg_handler_scratch
_secmon_dbg_handler_scratch:
	.word	0
	.word	0


#endif /* XCHAL_HAVE_DEBUG && XCHAL_HAVE_EXCEPTIONS && (XEA1 or XEA2) */

