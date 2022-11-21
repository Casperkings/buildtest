
// xos_int_table.S - Interrupt handler and priority info tables.

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
#include <xtensa/corebits.h>

#include "xos_common.h"


#ifdef __XTENSA_CALL0_ABI__
#define CALLX    callx0
#else
#define CALLX    callx8
#endif


// XOS .init section, early stage initialization goes here.

        .section .init, "ax"

#if XOS_OPT_THREAD_SAFE_CLIB && (XSHAL_CLIB == XTHAL_CLIB_XCLIB)
        movi    a2, _init_locks
        CALLX   a2
#endif
#if XCHAL_HAVE_XEA2
        movi    a2, xos_dispatch_from_thread
        wsr_xhreg(a2)
#endif


//-----------------------------------------------------------------------------
// XOS globals. NOTE offsets must match those in XOS_xxx in xos_common.h.
// Ensure that these are all in a single cache line (assumes cache line
// size is a multiple of 16 bytes).
//-----------------------------------------------------------------------------

        .data
        .align  16
        .global xos_globals
        .global xos_intlevel_mask
        .global xos_intenable_mask
        .global xos_curr_threadptr
        .global xos_next_threadptr

xos_globals:
xos_intlevel_mask:      .word   0xFFFFFFFF      // interrupt level mask (level zero)
xos_intenable_mask:     .word   0x00000000      // individual enable mask
xos_curr_threadptr:     .word   0xFFFFFFFF      // ptr to current thread block
xos_next_threadptr:     .word   0               // ptr to next thread block

#if (XCHAL_HAVE_XEA2 && XCHAL_HAVE_INTERRUPTS)

// NOTE:  xos_interrupt_table below follows immediately, at offset 16.

        .macro  inttable_entry  n, level, primask
        .if     ((\n-XCHAL_NUM_INTERRUPTS) & 0xFF00)    // if n < num_interrupts
        .word   xos_int_unhandled                       // default interrupt handler
        .word   \n                                      // handler argument (interrupt number)
#if XOS_OPT_INTERRUPT_SWPRI                             // software-prioritization ?
        .byte   \level                                  // interrupt level
        .byte   \level*32+\n                            // (future) interrupt priority
        .byte   0                                       // (reserved)
        .byte   0                                       // (reserved)
        .word   \primask                                // mask of interrupts of higher priority
#else
        .word   PS_WOE + PS_INTLEVEL(\level)            // handler PS value for interrupt's level
#endif
        .endif
        .endm

#define INTTABLE_ENTRY(n)       \
        inttable_entry  n, XCHAL_INT_LEVEL(n), (~XCHAL_INTLEVEL_ANDBELOW_MASK(XCHAL_INT_LEVEL(n)) | (XCHAL_INTLEVEL_MASK(XCHAL_INT_LEVEL(n)) & (-(1<<n)-(1<<n))))


//-----------------------------------------------------------------------------
// The interrupt table is organized as a structure for each interrupt.
// This is done so that if this table is in cached memory, we only need to
// incur a single dcache miss when dispatching an interrupt. Ensure that
// one entry (struct) resides completely within a single cache line.
//-----------------------------------------------------------------------------

        .align  16
        .global xos_interrupt_table

xos_interrupt_table:
        INTTABLE_ENTRY(0)
        INTTABLE_ENTRY(1)
        INTTABLE_ENTRY(2)
        INTTABLE_ENTRY(3)
        INTTABLE_ENTRY(4)
        INTTABLE_ENTRY(5)
        INTTABLE_ENTRY(6)
        INTTABLE_ENTRY(7)
        INTTABLE_ENTRY(8)
        INTTABLE_ENTRY(9)
        INTTABLE_ENTRY(10)
        INTTABLE_ENTRY(11)
        INTTABLE_ENTRY(12)
        INTTABLE_ENTRY(13)
        INTTABLE_ENTRY(14)
        INTTABLE_ENTRY(15)
        INTTABLE_ENTRY(16)
        INTTABLE_ENTRY(17)
        INTTABLE_ENTRY(18)
        INTTABLE_ENTRY(19)
        INTTABLE_ENTRY(20)
        INTTABLE_ENTRY(21)
        INTTABLE_ENTRY(22)
        INTTABLE_ENTRY(23)
        INTTABLE_ENTRY(24)
        INTTABLE_ENTRY(25)
        INTTABLE_ENTRY(26)
        INTTABLE_ENTRY(27)
        INTTABLE_ENTRY(28)
        INTTABLE_ENTRY(29)
        INTTABLE_ENTRY(30)
        INTTABLE_ENTRY(31)

#endif // XCHAL_HAVE_XEA2 && XCHAL_HAVE_INTERUPTS


#if (XCHAL_HAVE_XEA3 && XCHAL_HAVE_INTERRUPTS)

//-----------------------------------------------------------------------------
// In XEA3, the interrupt stack and the interrupt handler table must be placed
// together if there is an Interrupt Stack Base (ISB) register defined.
// The stack grows downward from the ISB address and the interrupt table is
// placed just above it. If the ISB is writable, then these data structures
// can be placed in any area of memory and the ISB programmed accordingly.
// See the XOS user manual for more details.
//-----------------------------------------------------------------------------

// Interrupt stack.

#if XCHAL_HAVE_ISB
    .data
#else
    .section    .intr.top, "aw"
#endif

    .align  16
    .global xos_int_stack
    .global xos_int_stack_end

xos_int_stack:
    .space      XOS_INT_STACK_SIZE
xos_int_stack_end:

//-----------------------------------------------------------------------------
// Interrupt handler table.
// IMPORTANT: We want the startup code to set up the ISB correctly to point
// to our handler table base. To make that happen the table must start at
// the symbol '_xt_interrupt_table'.
//-----------------------------------------------------------------------------

#if XCHAL_HAVE_ISB
    .data
#else
    .section    .intr.data, "aw"
#endif

    .align  16
    .global xos_interrupt_table
    .global _xt_interrupt_table

xos_interrupt_table:
_xt_interrupt_table:

//-----------------------------------------------------------------------------
// If using the interrupt wrapper, make the first entry in the interrupt table
// point to the wrapper.
//-----------------------------------------------------------------------------
#if XOS_USE_INT_WRAPPER
    .word   xos_interrupt_wrapper
    .word   0
#endif

    .set    i, 0
    .rept   XCHAL_NUM_INTERRUPTS
    .word   xos_int_unhandled           /* handler address               */
    .word   i                           /* handler arg (default: intnum) */
    .set    i, i+1
    .endr

#endif // XCHAL_HAVE_XEA3 && XCHAL_HAVE_INTERRUPTS


//-----------------------------------------------------------------------------
// Table of coprocessor owner threads. If a coprocessor is owned by thread X,
// then the entry for that coprocessor will point to X's TCB.
//-----------------------------------------------------------------------------

#if XCHAL_CP_NUM > 0

        .data
        .align  4
        .global _xos_cp_owner_table

_xos_cp_owner_table:
        .word   0       // CP 0
        .word   0       // CP 1
        .word   0       // CP 2
        .word   0       // CP 3
        .word   0       // CP 4
        .word   0       // CP 5
        .word   0       // CP 6
        .word   0       // CP 7

#endif


//-----------------------------------------------------------------------------
// Variables to track cycles consumed by interrupt handling.
// For XEA3 they also count cycles consumed by exception handling.
//-----------------------------------------------------------------------------
#if (XCHAL_HAVE_INTERRUPTS && XOS_OPT_STATS)

        .section        .bss, "aw"
        .global         xos_inth_start_ccount
        .size           xos_inth_start_ccount, 4
        .global         xos_inth_cycle_count
        .size           xos_inth_cycle_count, 8
        .global         xos_inth_intr_count
        .size           xos_inth_intr_count, 4
        .align          8

xos_inth_cycle_count:
        .word   0
        .word   0
xos_inth_start_ccount:
        .word   0
xos_inth_intr_count:
        .word   0
        .text
#endif

