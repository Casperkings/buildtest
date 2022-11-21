// mtvec_dispatch.S  - Machine-mode trap dispatcher for XEA5.

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


#include <xtensa/coreasm.h>
#include <xtensa/corebits-xea5.h>
#include <xtensa/xtruntime-frames.h>


	.section	.text

//-----------------------------------------------------------------------------
// XTOS interrupt/exception dispatch. The MTVEC trap vector must be aligned to
// at least 64 bytes.
// XTOS handles interrupts/exceptions on the application stack.
//-----------------------------------------------------------------------------

	.global		_xtos_dispatch
	.type		_xtos_dispatch,@function
	.p2align	6

//-----------------------------------------------------------------------------
// Dispatch entry point.
//-----------------------------------------------------------------------------

_xtos_dispatch:
	addi	sp, sp, -ESF_TOTALSIZE		// Make room for exc frame
	sw	a0, sp, EXC_a0			// Save a0-a7, t0-t6
	sw	a1, sp, EXC_a1
	sw	a2, sp, EXC_a2
	sw	a3, sp, EXC_a3
	sw	a4, sp, EXC_a4
	sw	a5, sp, EXC_a5
	sw	a6, sp, EXC_a6
	sw	a7, sp, EXC_a7
	sw	t0, sp, EXC_t0
	sw	t1, sp, EXC_t1
	sw	t2, sp, EXC_t2
	sw	t3, sp, EXC_t3
	sw	t4, sp, EXC_t4
	sw	t5, sp, EXC_t5
	sw	t6, sp, EXC_t6
	sw	ra, sp, EXC_ra			// Save return address
	csrr	a0, mcause			// Read mcause
	csrr	a1, mepc			// Read mepc
	sw	a0, sp, EXC_cause		// Save cause
	sw	a1, sp, EXC_pc			// Save pc
	addi	a2, sp, ESF_TOTALSIZE
	sw	a2, sp, EXC_sp			// Save stack ptr
	bgez	a0, .Lexc			// Branch if exception

//-----------------------------------------------------------------------------
// There is a little tweak to handler lookup here. The RV32 vector table allows
// only 4 bytes per entry, for a handler address. However we want 8 bytes per
// entry, 4 for address and 4 for the argument. So we do not use the address
// provided by "csrrsi mnxti" as is. Instead we double the offset and then read
// 8 bytes from that address.
//-----------------------------------------------------------------------------

handle_int:
	csrrsi	a1, mnxti, MSTATUS_MIE		// Check intr, enable ints
	beqz	a1, restore			// Spurious intr
	csrr	a2, mtvt			// Get vector table base address
	sub	a1, a1, a2			// Compute offset
	slli	a1, a1, 1			// Double the offset
	add	a1, a1, a2			// Add back base
	lw	a0, a1, 4			// Load handler argument
	lw	a1, a1, 0			// Load handler address
	jalr	ra, a1				// Call handler
	csrrsi	a1, mnxti, MSTATUS_MIE		// Check intr, enable ints
	bnez	a1, handle_int			// Branch if pending intr

//-----------------------------------------------------------------------------
// Restore state and return.
//-----------------------------------------------------------------------------

restore:
	lw	a1, sp, EXC_a1			// Restore a1-a7, t0-t6
	lw	a2, sp, EXC_a2
	lw	a3, sp, EXC_a3
	lw	a4, sp, EXC_a4
	lw	a5, sp, EXC_a5
	lw	a6, sp, EXC_a6
	lw	a7, sp, EXC_a7
	lw	t0, sp, EXC_t0
	lw	t1, sp, EXC_t1
	lw	t2, sp, EXC_t2
	lw	t3, sp, EXC_t3
	lw	t4, sp, EXC_t4
	lw	t5, sp, EXC_t5
	lw	t6, sp, EXC_t6
	lw	ra, sp, EXC_ra			// Restore return address

//-----------------------------------------------------------------------------
// Check for pending interrupts. If there is one, branch back and handle it.
// Note that all saved state in the exception frame is still valid.
// If none, disable interrupts, clean up the stack frame and return.
//-----------------------------------------------------------------------------

	csrrci	a0, mnxti, MSTATUS_MIE
	bnez	a0, handle_int
	lw	a0, sp, EXC_pc
	csrw	mepc, a0			// Restore epc
	lw	a0, sp, EXC_a0			// Restore a0
	lw	sp, sp, EXC_sp			// Restore stack pointer
	mret

//-----------------------------------------------------------------------------
// Call exception dispatcher, then return.
//-----------------------------------------------------------------------------

.Lexc:
	jal	ra, _xtos_exc_dispatch		// Call exc dispatcher
	j	restore

	.size	_xtos_dispatch, . - _xtos_dispatch

