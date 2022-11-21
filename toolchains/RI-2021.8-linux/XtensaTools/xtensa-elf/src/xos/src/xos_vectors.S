
// xos_vectors.S - Exception and interrupt vectors.

// Copyright (c) 2015-2020 Cadence Design Systems, Inc.
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
#include <xtensa/simcall.h>

#include "xos_common.h"


// Check for both XEA2 and the XSR instruction. For now just check for
// XEA2, if XSR not present there will be build errors.

#if XCHAL_HAVE_XEA2 == 0 && XCHAL_HAVE_EXCEPTIONS
  #error "XOS requires XEA2 (and the XSR instruction)"
#endif

#if (XSHAL_USER_VECTOR_SIZE < 28)
  #error "XOS requires the user exception vector to be at least 28 bytes."
#endif

// Disable assembler scheduling for this file. Using FLIX instructions can
// cause code to overflow vector areas.

	.begin		no-schedule


//-----------------------------------------------------------------------------
// UserExceptionVector
//
// This implements the user exception vector and transfers control to
// the handler specified in XHREG.
//-----------------------------------------------------------------------------

#if XOS_OPT_SECMON

	.section	.text, "ax"
	.global		_UserExceptionVector
	.align		4

_UserExceptionVector:

	// At entry, secure monitor has saved EPC1 in EXCSAVE1, PS.EXCM=0,
	// PS.INTLEVEL=EXCM_LEVEL (bits 0-2), RING (bit 3).

	addi	a1, a1, -XOS_FRAME_SIZE	// allocate exception stack frame
	s32i	a2, a1, FRAME_A2	// save a2
	s32i	a3, a1, FRAME_A3	// save a3
	rsr.excsave1	a3
	wsr.epc1	a3		// restore epc1 from excsave1
	movi	a3, ~PS_INTLEVEL_MASK
	rsr.ps	a2
	bbci.l	a2, 3, 1f		// Bit 3 indicates PS.RING value
	and	a2, a2, a3		// ring 1
	movi	a3, PS_EXCM | PS_INTLEVEL(0) | PS_RING(1)
	or	a2, a2, a3
	j	2f
1:
	and	a2, a2, a3		// ring 0
	movi	a3, PS_EXCM | PS_INTLEVEL(0)
	or	a2, a2, a3
2:
	wsr.ps	a2			// PS.INTLEVEL = 0, PS.EXCM = 1
	xsr_xhreg(a0)			// a0 <=> XHREG
	rsr.epc1	a3		// a2 = PS, a3 = EPC1
	addi	a2, a2, -PS_EXCM	// match return PS of medium-pri interrupts
	jx	a0			// jump to XHREG specified handler

#else

	.section	.UserExceptionVector.text, "ax"
	.global		_UserExceptionVector
	.align		4

_UserExceptionVector:

	addi	a1, a1, - XOS_FRAME_SIZE    // allocate exception stack frame
	s32i	a2, a1, FRAME_A2	// save a2
	s32i	a3, a1, FRAME_A3	// save a3
	xsr_xhreg(a0)			// a0 <=> XHREG
	rsr.ps		a2		// PS for level-one interrupts / exceptions
	rsr.epc1	a3		// PC for level-one interrupts / exceptions
	addi	a2, a2, -PS_EXCM	// match return PS of medium-pri interrupts
	jx	a0			// jump to XHREG specified handler

	// IMPORTANT: cannot exceed 9 instructions (9*3 = 27 bytes), as vector
	// size is only guaranteed 28 bytes across all Xtensa configurations.

#endif // XOS_OPT_SECMON


//-----------------------------------------------------------------------------
// KernelExceptionVector
//
// At this time the only valid reason for a kernel exception would be if
// we take a nested interrupt. Redirect to the user exception handler for
// processing.
//-----------------------------------------------------------------------------

#if XOS_OPT_SECMON

	.section	.text, "ax"
	.global		_KernelExceptionVector
	.align		4

#else

	.section	.KernelExceptionVector.text, "ax"
	.global		_KernelExceptionVector
	.align		4

#endif // XOS_OPT_SECMON

_KernelExceptionVector:

	j	_UserExceptionVector
.Lkevloop:
#if XCHAL_HAVE_DEBUG
	break	1, 15			// Should not get here
#endif
	j	.Lkevloop


//-----------------------------------------------------------------------------
// MEDIUM and HIGH PRIORITY INTERRUPT VECTORS (priority level greater than 1)
// One vector per level, multiple interrupts may be mapped to the same level.
//
// NOTE: We could use an XTOS compatible vector, that allows placing the vector
// in ROM and the handler/dispatcher in RAM, but requires initializing EXCSAVE_n
// registers on reset:
//
//    xsr.excsaveN  a2
//    jx  a2
//
// What is in use now is an alternate (simpler; and faster, especially if the 
// handler/dispatcher is in reach) which lets call0 expand to callx0 with the
// -mlongcalls option, and shrink back to call0 by linker relaxation. However
// the handler/dispatcher must be linked together with vector for this to work.
//
//    wsr.excsaveN  a0
//    call0      xos_level<n>_interrupt_dispatch
//    <DOES NOT RETURN HERE>
//-----------------------------------------------------------------------------

	.macro	xos_int_vector level vecsize

#if XOS_OPT_SECMON

	.section	.text, "ax"
	.global		_Level&level&Vector
	.weak		_Level&level&Vector
	.align		4

	// At entry, secure monitor has saved EPC1 in EXCSAVEN, PS.EXCM=0,
	// PS.INTLEVEL=EXCM_LEVEL. EPCN/EPSN are accurate.

_Level&level&Vector:
.ifeq (XCHAL_EXCM_LEVEL - \level) & 0xF00       // if \level <= XCHAL_EXCM_LEVEL
	addi	a1, a1, -XOS_FRAME_SIZE		// allocate exception stack frame
	s32i	a2, a1, FRAME_A2		// save a2
	s32i	a3, a1, FRAME_A3		// save a3
	readsr excsave \level a3
	wsr.epc1	a3			// restore epc1 from excsaveN
	movi	a3, ~PS_INTLEVEL_MASK
	rsr.ps	a2
	and	a2, a2, a3
	movi	a3, PS_EXCM | PS_INTLEVEL(\level)
	or	a2, a2, a3
	wsr.ps	a2				// PS.INTLEVEL = level, PS.EXCM = 1
	movi	a2, EXCCAUSE_LEVEL1_INTERRUPT	// set EXCCAUSE as for level-1 interrupts...
	wsr.exccause	a2			// ...
	xsr_xhreg(a0)				// a0 <=> XHREG
	readsr	eps \level a2			// get return PS for medium-pri interrupt
	readsr	epc \level a3			// get return PC for medium-pri interrupt
	jx	a0				// jump to XHREG specified handler
.endif

#else

	.section	.Level&level&InterruptVector.text, "ax"
	.global		_Level&level&Vector
	.weak		_Level&level&Vector
	.global		xos_level&level&_interrupt_dispatch
	.align		4

_Level&level&Vector:
.ifeq (XCHAL_EXCM_LEVEL - \level) & 0xF00	// if \level <= XCHAL_EXCM_LEVEL
    .ifeq (27 - \vecsize) & 0xF00		// Vector size < 28 bytes, split in two parts.
	writesr	excsave \level a0		// preserve a0
	call0	xos_level&level&_interrupt_dispatch	// may expand to callx0

	.text
	.align		4

xos_level&level&_interrupt_dispatch:
	readsr	excsave \level a0		// restore a0
    .endif
	addi	a1, a1, - XOS_FRAME_SIZE	// allocate exception stack frame
	s32i	a2, a1, FRAME_A2		// save a2
	s32i	a3, a1, FRAME_A3		// save a3
	movi	a2, EXCCAUSE_LEVEL1_INTERRUPT	// set EXCCAUSE as for level-1 interrupts...
	wsr.exccause	a2			// ...
	xsr_xhreg(a0)				// a0 <=> XHREG
	readsr	eps \level a2			// get return PS for medium-pri interrupt
	readsr	epc \level a3			// get return PC for medium-pri interrupt
	//addi	a2, a2, PS_EXCM			// (can't fit in 9 instructions)
	jx	a0				// jump to XHREG specified handler
.else
	// This bit will always fit even in the smallest vector.
	writesr	excsave \level a0			// preserve a0
	call0	xos_level&level&_interrupt_dispatch	// may expand to callx0
	// Does not return here (see xos_handlers.S)
.endif

#endif // XOS_OPT_SECMON

	.size	_Level&level&Vector, \vecsize
	.endm


// Instantiate vectors for levels that exist and are not the debug level

//-----------------------------------------------------------------------------
// Level 2
//-----------------------------------------------------------------------------

#if XCHAL_NUM_INTLEVELS >= 2 && XCHAL_DEBUGLEVEL != 2
	xos_int_vector	2  XSHAL_INTLEVEL2_VECTOR_SIZE
#endif

//-----------------------------------------------------------------------------
// Level 3
//-----------------------------------------------------------------------------

#if XCHAL_NUM_INTLEVELS >= 3 && XCHAL_DEBUGLEVEL != 3
	xos_int_vector	3  XSHAL_INTLEVEL3_VECTOR_SIZE
#endif

//-----------------------------------------------------------------------------
// Level 4
//-----------------------------------------------------------------------------

#if XCHAL_NUM_INTLEVELS >= 4 && XCHAL_DEBUGLEVEL != 4
	xos_int_vector	4  XSHAL_INTLEVEL4_VECTOR_SIZE
#endif

//-----------------------------------------------------------------------------
// Level 5
//-----------------------------------------------------------------------------

#if XCHAL_NUM_INTLEVELS >= 5 && XCHAL_DEBUGLEVEL != 5
	xos_int_vector	5  XSHAL_INTLEVEL5_VECTOR_SIZE
#endif

//-----------------------------------------------------------------------------
// Level 6
//-----------------------------------------------------------------------------

#if XCHAL_NUM_INTLEVELS >= 6 && XCHAL_DEBUGLEVEL != 6
	xos_int_vector	6  XSHAL_INTLEVEL6_VECTOR_SIZE
#endif


//-----------------------------------------------------------------------------
// DoubleExceptionVector
//
// TODO: Provide some traceability of where this came from.
//-----------------------------------------------------------------------------

	.section	.DoubleExceptionVector.text, "ax"
	.global		_DoubleExceptionVector
	.weak		_DoubleExceptionVector
	.align		4

_DoubleExceptionVector:

#if XCHAL_HAVE_DEBUG
	break	1, 15
#endif
	j	_DoubleExceptionVector


//-----------------------------------------------------------------------------
// DebugExceptionVector
//-----------------------------------------------------------------------------

#if XCHAL_HAVE_DEBUG
	.section	.DebugExceptionVector.text, "ax"
	.global		_DebugExceptionVector
	.weak		_DebugExceptionVector
	.global		xos_debug_dispatch
	.align		4

_DebugExceptionVector:

	writesr	excsave XCHAL_DEBUGLEVEL a0	// save a0
	call0	xos_debug_dispatch		// may expand to callx0

	.size	_DebugExceptionVector, . - _DebugExceptionVector

	.text
	.align		4

xos_debug_dispatch:
	readsr	excsave XCHAL_DEBUGLEVEL a0	// restore a0

	// Attempt to trap to the simulator first. If we are not running on
	// the simulator, the SIMCALL will act like a NOP and we will fall
	// through. If that happens, set the exception cause and emulate the
	// user exception vector and transfer to the handler specified in
	// XHREG. This will call the user-supplied handler (if any) or
	// the default handler.
        // (On older hardware the simcall will cause an illegal instruction
        // exception so we skip it).

	writesr	excsave XCHAL_DEBUGLEVEL a2	// save a2 where simulator expects it
#if XCHAL_HW_MIN_VERSION >= XTENSA_HWVERSION_RE_2013_2
	movi	a2, SYS_gdb_enter_sktloop
	simcall					// have ISS handle the debug exception
#endif
	movi	a2, EXCCAUSE_DEBUG
	wsr.exccause	a2			// set exception cause for dispatch
	readsr  excsave XCHAL_DEBUGLEVEL a2	// restore a2
	addi	a1, a1, - XOS_FRAME_SIZE	// allocate exception stack frame
	s32i	a2, a1, FRAME_A2		// save a2
	s32i	a3, a1, FRAME_A3		// save a3
	xsr_xhreg(a0)				// a0 <=> XHREG
	rsr.ps		a2			// PS for level-one interrupts / exceptions
	rsr.epc1	a3			// PC for level-one interrupts / exceptions
	addi	a2, a2, -PS_EXCM		// match return PS of medium-pri interrupts
	jx	a0				// jump to XHREG specified handler

	.size	xos_debug_dispatch, . - xos_debug_dispatch
#endif // XCHAL_HAVE_DEBUG


//-----------------------------------------------------------------------------
// NMIExceptionVector
//
// NMI vector, jumps to XOS NMI handler.
//-----------------------------------------------------------------------------

#if XCHAL_HAVE_NMI
	.section	.NMIExceptionVector.text, "ax"
	.global		_NMIExceptionVector
	.weak		_NMIExceptionVector
	.global		xos_nmi_dispatch
	.align		4

_NMIExceptionVector:
	writesr	excsave XCHAL_NMILEVEL a0	// Preserve a0
	call0	xos_nmi_dispatch		// jump to handler
	// Does not return here

	.size	_NMIExceptionVector, . - _NMIExceptionVector
#endif // XCHAL_HAVE_NMI


	.end	no-schedule

