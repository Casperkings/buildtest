// xtos_save_system_state.S

// Provides routine for saving important registers and stack data for
// post mortem recovery and inspection.  This routine is intended to be
// used inside of a fatal error exception handler however it may be called
// at any time.

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
#include <xtensa/xtruntime.h>
#include "xtos-internal.h"

#if XCHAL_HAVE_XEA1
# error "xtos_save_system_state is not available for XEA1"
#endif

// Register structure
STRUCT_BEGIN
STRUCT_FIELD (long,4,RMCD_,STACK_START)	// stack start address
STRUCT_FIELD (long,4,RMCD_,NAR)		// save num of AR registers
STRUCT_FIELD (long,4,RMCD_,PC)
STRUCT_FIELD (long,4,RMCD_,PS)
#if XCHAL_HAVE_XEA3
STRUCT_FIELD (long,4,RMCD_,WB)
STRUCT_FIELD (long,4,RMCD_,EPC)
#elif XCHAL_HAVE_XEA2 
STRUCT_FIELD (long,4,RMCD_,WINDOWSTART)
STRUCT_FIELD (long,4,RMCD_,WINDOWBASE)
STRUCT_FIELD (long,4,RMCD_,EPC1)
#endif
STRUCT_FIELD (long,4,RMCD_,EXCCAUSE)
STRUCT_FIELD (long,4,RMCD_,EXCVADDR)
#if XCHAL_HAVE_XEA2 
STRUCT_FIELD (long,4,RMCD_,EXCSAVE1)
#endif
STRUCT_AFIELD(long,4,RMCD_,AREG, XCHAL_NUM_AREGS)	// address registers ar0..ar63
STRUCT_AFIELD(long,4,RMCD_,AROLL, 12)	// rollover space for odd aligned windows on XEA2
#define RMCD_AR(n) RMCD_AREG+((n)*4)
#define RMCD_ROLL(n) RMCD_AROLL+((n)*4)
STRUCT_END(__aRMCD_Frame)

	// get_ar_index - returns the index of the ar register corresponding to
	// a0 for the current windowbase.
	.macro get_ar_index ar_out ar_scratch
#if XCHAL_HAVE_XEA3
	rsr	\ar_out, WB
	movi	\ar_scratch, 0x7
	and	\ar_out, \ar_out, \ar_scratch
	slli	\ar_out, \ar_out, 3
#elif XCHAL_HAVE_XEA2
	rsr	\ar_out, WINDOWBASE
	slli	\ar_out, \ar_out, 2
#endif
	.endm

	// rot_window - ABI ambiguous window rotate
	.macro rot_window
#if XCHAL_HAVE_XEA3
	rotw	1
#elif XCHAL_HAVE_XEA2
	rotw	2
#endif
	.endm

	// copy_int32 - memory to memory copy
	.macro copy_int32 ar_src ar_dst ar_temp offset
	l32i	\ar_temp, \ar_src, \offset*4
	s32i	\ar_temp, \ar_dst, \offset*4
	.endm


	.global xtos_save_system_state
xtos_save_system_state:
// void xtos_save_system_state(void) - Save registers and stack
//
//
#if (defined __XTENSA_CALL0_ABI__)
	// Save a0, and a2-a5 for internal use when using the call0 ABI
	addi	a1, a1, -32
	s32i	a0, a1, 0
	s32i	a2, a1, 4
	s32i	a3, a1, 8
	s32i	a4, a1, 12
	s32i	a5, a1, 16
	s32i	a6, a1, 20
	j	1f
1:
#elif (defined __XTENSA_WINDOWED_ABI__)
	abi_entry
#else
#error	"Error: unknown ABI configuration"
#endif
	// Save special registers
	//   a3 - pointer to storage area
	//   a4 - storage for special registers
	//   a5 - scratch use
	movi	a3, xtos_save_reg_mem

	// Save PC, use call0 jump ahead one instruction to force the current PC into
	// a0.  The current PC needs to be saved so that on restoration it will
	// align with the stack.
	mov	a4, a0
	mov	a5, a1
	call0	get_pc
	.align 4
get_pc:
	s32i	a0, a3, RMCD_PC
	mov	a0, a4
	mov	a1, a5

	// Save PS then disable interrupts
	rsr.ps	a4
	s32i	a4, a3, RMCD_PS
#if XCHAL_HAVE_XEA3
        movi	a5, 0x8
        or	a4,a4,a5
#elif XCHAL_HAVE_XEA2
        movi	a5, 1
	movi	a3, -1
        slli	a5, a5, 18
        xor	a5, a5, a3
        and	a4,a4,a5
	movi    a3, xtos_save_reg_mem
#endif
	wsr.ps	a4

	// Save EPC/EPC1
#if XCHAL_HAVE_XEA3
        rsr.epc	a4
        s32i	a4, a3, RMCD_EPC
#elif XCHAL_HAVE_XEA2
        rsr.epc1 a4
        s32i	a4, a3, RMCD_EPC1
#endif

	// Save EXCCAUSE and EXCVADDR
	rsr.exccause a4
        s32i	a4, a3, RMCD_EXCCAUSE
        rsr	a4, EXCVADDR
        s32i	a4, a3, RMCD_EXCVADDR

	// Save EXCSAVE, why?
#if XCHAL_HAVE_XEA2
        rsr.excsave1 a4
        s32i	a4, a3, RMCD_EXCSAVE1
#endif

	// Save number of address registers
        movi	a4, XCHAL_NUM_AREGS
        s32i	a4, a3, RMCD_NAR

	// Save windowing control registers
#if (defined __XTENSA_WINDOWED_ABI__) && XCHAL_HAVE_XEA3
        rsr.wb	a4
        s32i	a4, a3, RMCD_WB
        movi	a5, 0xFFFF
        and	a4, a4, a5
        wsr.wb 	a4
#elif (defined __XTENSA_WINDOWED_ABI__) && XCHAL_HAVE_XEA2
        rsr.windowstart a4
        s32i	a4, a3, RMCD_WINDOWSTART
        rsr.windowbase a4
        s32i	a4, a3, RMCD_WINDOWBASE
#endif

	// Save address registers. Windowed ABI requires using the rotate to
	// walk through the full register file.  Call0 grabs a snapshot of the
	// current registers.
	//   a3 - pointer to storage area, moves through window
	//   a4 - windowed offset
	//   a5 - temporary variable space pointer
#if (defined __XTENSA_WINDOWED_ABI__)
reg_save:
	get_ar_index a4, a5		// get the ar index for the current a0
	movi	a3, xtos_save_reg_mem
	addx4	a3, a4, a3		// get corrected base address for window
	s32i	a0, a3, RMCD_AR(0)	// store the first bank of registers
	s32i	a1, a3, RMCD_AR(1)	// ...
	s32i	a2, a3, RMCD_AR(2)
	s32i	a3, a3, RMCD_AR(3)
	s32i	a4, a3, RMCD_AR(4)
	s32i	a5, a3, RMCD_AR(5)
	s32i	a6, a3, RMCD_AR(6)
	s32i	a7, a3, RMCD_AR(7)
	// Enter the main register save loop.  Registers a0-a7 have already been
	// saved so only a8-a15 need to be saved.  Since the register window is
	// always moved by 8 then the precondition that a0-a7 have been save is
	// always true.  Every time through the loop the control variables need
	// to be reinitialized since the rotate moved them to an inaccessible
	// location.
reg_save_loop:
	get_ar_index a4, a5		// rot wiped out the address offset
	slli	a4, a4, 2		// convert index to address offset
	movi	a3, xtos_save_reg_mem
#if XCHAL_HAVE_XEA3
	l32i	a5, a3, RMCD_WB
	movi	a3, 0x7
	and	a5, a5, a3		// mask WB to get window base
	movi	a3, xtos_save_reg_mem
#elif XCHAL_HAVE_XEA2
	l32i	a5, a3, RMCD_WINDOWBASE
#endif
	add	a3, a3, a4		// correct base address for window
	s32i	a8, a3, RMCD_AR(8)	// store upper bank of registers
	s32i	a9, a3, RMCD_AR(9)	// ...
	s32i	a10, a3, RMCD_AR(10)
	s32i	a11, a3, RMCD_AR(11)
	s32i	a12, a3, RMCD_AR(12)
	s32i	a13, a3, RMCD_AR(13)
	s32i	a14, a3, RMCD_AR(14)
	s32i	a15, a3, RMCD_AR(15)
	mov	a11, a3			// move a3 to a3 of next window
	mov	a12, a4			// move a4 to a4 of next window, maybe not needed
	mov	a13, a5			// move a5 to a5 of next window, maybe not needed
	l32i	a3, a11, RMCD_AR(3)
        l32i	a4, a11, RMCD_AR(4)
        l32i	a5, a11, RMCD_AR(5)
	rot_window			// rotate window by 8
#if XCHAL_HAVE_XEA3
	rsr.wb	a3
	movi	a4, 0x7
	and	a3, a3, a4
#elif XCHAL_HAVE_XEA2
	rsr.windowbase a3
#endif
	beq	a3, a5, reg_save_end	// have we rotated back to the start
	j	reg_save_loop		// no, well keep going
reg_save_end:
	// When the end of the ar register file is reached the hardware has the
	// ability to split the addressing to use some ar registers from the end
	// of the register file and some from the start.  For example a0-a7 will
	// map to ar56-ar63 and a8-a15 will map from ar0-ar7.  The save loop
	// does not take this into acount and will over run the end of the save
	// area slightly.  Memory has been reserved for this purpose so that no
	// memory corruption occurs however now those values need to be moved
	// from the reserved space to the correct location in the save region.
	// On XEA3 the maximum overrun is 8 due to the fixed rotate window size
	// however on XEA2 it could be as much as 12.
	mov	a5, a3
	movi	a3, xtos_save_reg_mem+RMCD_AR(0)
	movi	a4, xtos_save_reg_mem+RMCD_AR(XCHAL_NUM_AREGS)
	copy_int32 a4, a3, a5, 0
	copy_int32 a4, a3, a5, 1
	copy_int32 a4, a3, a5, 2
	copy_int32 a4, a3, a5, 3
	copy_int32 a4, a3, a5, 4
	copy_int32 a4, a3, a5, 5
	copy_int32 a4, a3, a5, 6
	copy_int32 a4, a3, a5, 7
#if XCHAL_HAVE_XEA2
	rsr.windowbase a5
	bbci	a5, 0, restore_state
	copy_int32 a4, a3, a5, 8
	copy_int32 a4, a3, a5, 9
	copy_int32 a4, a3, a5, 10
	copy_int32 a4, a3, a5, 11
#endif
#elif (defined __XTENSA_CALL0_ABI__)
	movi	a3, xtos_save_reg_mem
	s32i	a0, a3, RMCD_AR(0)
	s32i	a1, a3, RMCD_AR(1)
	s32i	a2, a3, RMCD_AR(2)
	s32i	a3, a3, RMCD_AR(3)
	s32i	a4, a3, RMCD_AR(4)
	s32i	a5, a3, RMCD_AR(5)
	s32i	a6, a3, RMCD_AR(6)
	s32i	a7, a3, RMCD_AR(7)
	s32i	a8, a3, RMCD_AR(8)
	s32i	a9, a3, RMCD_AR(9)
	s32i	a10, a3, RMCD_AR(10)
	s32i	a11, a3, RMCD_AR(11)
	s32i	a12, a3, RMCD_AR(12)
	s32i	a13, a3, RMCD_AR(13)
	s32i	a14, a3, RMCD_AR(14)
	s32i	a15, a3, RMCD_AR(15)
#endif // __XTENSA_WINDOWED_ABI__

	// Save the stack area
	//   a3 - pointer to stack memory
	//   a4 - pointer to storage
	//   a5 - copy counter (XTOS_SAVE_STACK_MEM_SIZE -> 0)
	//   a6 - intermediate storage
	movi	a4, xtos_save_stack_size
	l32i	a4, a4, 0			// a4 gets the max stack capacity
	movi	a3, __stack
	sub	a3, a3, a1			// a3 gets the actual size
	bge	a4, a3, stack_oversized		// max greater than actual
	mov	a3, a1
	j	start_copy_stack
stack_oversized:
	movi	a3, __stack - XTOS_SAVE_STACK_MEM_SIZE
	j	start_copy_stack
start_copy_stack:
	movi	a6, xtos_save_reg_mem
	s32i	a3, a6, RMCD_STACK_START
	movi	a5, XTOS_SAVE_STACK_MEM_SIZE >> 2
	movi	a4, xtos_save_stack_mem
copy_stack_loop:
	copy_int32 a3, a4, a6, 0
	addi	a5, a5, -1
	addi	a3, a3, 4
	addi	a4, a4, 4
	bnez	a5, copy_stack_loop

	// Stage four, restore system state
	//   a3 - pointer to storage area
	//   a4 - windowed offset
	//   a5 - temporary variable space pointer
restore_state:
	movi	a3, xtos_save_reg_mem
	l32i	a4, a3, RMCD_PS		// restore key state to return
	wsr.ps	a4

#if (defined __XTENSA_WINDOWED_ABI__)
#if XCHAL_HAVE_XEA3
	l32i	a4, a3, RMCD_WB
	wsr.wb	a4
#elif XCHAL_HAVE_XEA2
	l32i	a4, a3, RMCD_WINDOWSTART
	wsr.windowstart a4
#endif
#endif // __XTENSA_WINDOWED_ABI__

#if (defined __XTENSA_CALL0_ABI__)
	l32i	a0, a1, 0
	l32i	a2, a1, 4
	l32i	a3, a1, 8
	l32i	a4, a1, 12
	l32i	a5, a1, 16
	l32i	a6, a1, 20
	addi	a1, a1, 32
	ret
#elif (defined __XTENSA_WINDOWED_ABI__)
	abi_return
#endif

