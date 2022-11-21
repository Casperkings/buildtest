// debug-vector-mon.S - debug exception handler
//
// $Id: //depot/dev/Eagleridge/Xtensa/OS/xmon/debug-vector-mon.S#1 $

// Copyright (c) 1999-2007 Tensilica Inc.
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

/*
Notes on debugging the XMON debug monitor with xt-gdb under OCD/JTAG
--------------------------------------------------------------------

To debug XMON under OCD it is necessary to lower PS.INTLEVEL to debug-1
inside XMON (the debug exception h/w sets it to debug level) so that
debug exceptions give control to OCD. Since there are no interrupts at
the same level as debug, this makes no difference to interrupts. It
does, however, affect debug events such as ICOUNT and h/w breakpoints,
so it is necessary to first lower ICOUNTLEVEL to debug-1 so that these
events don't happen inside XMON. The user ICOUNTLEVEL must first be
saved so XMON can see what it was. It is also necessary to restore
PS.INTLEVEL to the debug level before EPC[debug] and EPS[debug] are
restored to the user PC and PS (resp.) so that OCD won't destroy them.

All this is controlled by the macro XMON_OCD_DEBUG which can be defined
by uncommenting below.

When a debug exception hits while running in OCD Run Mode, OCD takes
control and this vector is never entered. To get an xt-gdb/OCD session
to come in here, having broken into OCD mode at the debug exception,
just say:
    (xt-gdb) jump * _DebugExceptionVector
which just sets the PC here and continues. EPC[debug] and EPS[debug]
were already set by the OCD debug exception process. Unfortunately,
the user EPC[debug] has already been lost because it was overwritten
as a result of the 'jump'. The saved value in _ar_registers[EPC+debug]
needs to be manually fixed.
    (xt-gdb) set $userpc = $pc
    (xt-gdb) break _handle_exception
    (xt-gdb) jump * _DebugExceptionVector
    (xt-gdb) set _sr_registers[176+<debuglevel>] = $userpc
*/

#if !XCHAL_HAVE_WINDOWED
# define _RET 	ret
# define _ENTRY(sp,N)

#else
# define _RET 	retw
# define _ENTRY(sp,N)	entry	sp, N
#endif

#define XMON_OCD_DEBUG
#ifdef XMON_OCD_DEBUG
#define XMON_DEBUGLEVEL XCHAL_DEBUGLEVEL-1
#else
#define XMON_DEBUGLEVEL XCHAL_DEBUGLEVEL
#endif

// Imports
.global _flush_i_cache
.global _handle_exception
.global _ar_registers
.global _sr_registers
.global _xmon_intnum;
.global _xmon_intmask;
.global _xmon_intlevel;

#include <xtensa/coreasm.h>
#include <xtensa/config/specreg.h>


// unsigned _xmon_get_cpenable(void)
//

        .global _xmon_get_cpenable
        .align  4
_xmon_get_cpenable:
       _ENTRY(sp,48)
#ifdef CPENABLE
        rsr     a2, CPENABLE
#endif
	_RET

        .align  4
_flush_i_cache:
       _ENTRY(sp,48)

#if XCHAL_DCACHE_SIZE > 0
        dhwb    a2, 0   /* force it out of the data cache (if present) */
#endif
#if XCHAL_ICACHE_SIZE > 0
        /* Use ihi for a little more efficiency */
        ihi     a2, 0   /* invalidate in i-cache (if present) */
        isync           /* just for safety sake */
        isync_retw_nop
#endif
	_RET

// void _xmon_set_cpenable(unsigned value)
//
// a2 -- holds the value to set cpenable to
//

        .global _xmon_set_cpenable
        .align 4
_xmon_set_cpenable:
       _ENTRY(sp,16)
#ifdef CPENABLE
        wsr     a2, CPENABLE
        rsync
#endif
	_RET



// User read of INTENABLE (result in a2)
// unsigned _xmon_get_intenable(void)
//

        .global _xmon_get_intenable
        .align  4
_xmon_get_intenable:
       _ENTRY(sp,16)
#if XCHAL_HAVE_INTERRUPTS
        movi    a3, _sr_registers
        #if XCHAL_HAVE_XEA1
        l32i    a2, a3, INTENABLE<<2    // value saved on entry to XMON
        #else
        rsr     a2, INTENABLE           // current live value
        s32i    a2, a3, INTENABLE<<2    // save most recent value reported to user
        #endif
#endif
	_RET


// User write of INTENABLE (value in a2)
// void _xmon_set_intenable(unsigned value)
//
// Merge bits user explicitly changed with current value because hi-pri ints
// above debug level might have changed INTENABLE since user's last access.
// Protect critical section to avoid unintended changes to bits a hi-pri int
// above debug level might modify if it occurred between our read and write.
// This critical section must be extremely short (just a few cycles).
// WARNING: NMI cannot be masked therefore, if it manipulates INTENABLE, it
// is not safe for the user to write INTENABLE at all (user beware!).
//

        .global _xmon_set_intenable
        .align 4
_xmon_set_intenable:
	_ENTRY(sp,16)

#if XCHAL_HAVE_INTERRUPTS
        movi    a3, _sr_registers       // a3 = save area
        l32i    a4, a3, INTENABLE<<2    // a4 = value most recently reported to user
        xor     a4, a2, a4              // a4 = mask of bits explicitly changed by user
        #if XCHAL_HAVE_XEA1
        s32i    a2, a3, INTENABLE<<2    // update saved value
        movi    a3, _xmon_inten_changed
        l32i    a2, a3, 0               // a2 = mask of bits previously changed by user
        or      a2, a2, a4              // a2 = new mask of bits changed by user
        s32i    a2, a3, 0               // update mask of bits changed by user
        #else
        and     a2, a2, a4              // a2 = bits we will merge into current value
        neg     a4, a4
        addi    a4, a4, -1              // a4 = mask of bits not changed by user
        // begin critical section - lock out all interrupts, even hi-pri above debug
        rsil    a3, 15                  // a3 = old PS
        rsr     a5, INTENABLE           // a5 = current live value
        and     a5, a5, a4              // a5 = bits we will keep from current value
        or      a2, a5, a2              // a2 = new value
        wsr     a2, INTENABLE
        esync /* do we need this? */    // ensure coherency of next rsr INTENABLE
        wsr     a3, PS                  // restore old PS
        // end critical section
        #endif
#endif
	_RET





// void _xmon_set_special_register(unsigned special_register, unsigned value, unsigned *execute_here)
//      a2 -- special_register
//      a3 -- value to write
//      a4 -- pointer to memory to execute from


//      base wsr instruction which we will
//       modify for the correct ar and special register number
#if XCHAL_HAVE_BE
#define WSR0_INSN       0x00003100      // insn in upper 3 bytes
#else
#define WSR0_INSN       0x00130000      // insn in lower 3 bytes
#endif

        .align 4
.global _xmon_set_special_register
_xmon_set_special_register:
	_ENTRY(sp,48)

// a6 -- temporary for moving memory
// a5 -- pointer to wsr0_placeholder
// a4 -- points to the RAM location we will
//       execute from, move the base instruction
//       (including the retw) to that point.

        movi    a5, .wsr0_instruction_placeholder
        l32i    a6, a5, 0
        s32i    a6, a4, 0
        l32i    a6, a5, 4
        s32i    a6, a4, 4

// a6 -- wsr instruction under construction
        movi    a6, WSR0_INSN

#if XCHAL_HAVE_BE

// a2 -- contains special register number to OR into sr field
        slli    a2, a2, 16
        or      a6, a6, a2

// a3 -- value to write, so OR 3 into the t field
// a5 -- temporary
        movi    a5, 3
        slli    a5, a5, 24
        or      a6, a6, a5

// a4 -- points to the caller supplied location for instr
// a5 -- temporary used to merge our modified instruction
// a2 -- temporary
        l32i    a5, a4, 0

// In Big Endian save LSB and put instruction in upper 3 bytes
        movi    a2, 0xff
        and     a5, a5, a2
        or      a6, a6, a5
        s32i    a6, a4, 0

#else /* XCHAL_HAVE_BE */

// a2 -- contains special register number to OR into sr field
        slli    a2, a2, 8
        or      a6, a6, a2

// a3 -- value to write, so OR 3 into the t field
// a5 -- temporary
        movi    a5, 3
        slli    a5, a5, 4
        or      a6, a6, a5

// a4 -- points to the caller supplied location for instr
// a5 -- temporary used to merge our modified instruction
// a2 -- temporary
        l32i    a5, a4, 0

// In Little Endian save MSB and put instruction in lower 3 bytes
        movi    a2, 0xff
        slli    a2, a2, 24
        and     a5, a5, a2
        or      a6, a6, a5
        s32i    a6, a4, 0

#endif /* XCHAL_HAVE_BE */

// Flush the cache
        mov     a10, a4
#if XCHAL_HAVE_WINDOWED
        call8   _flush_i_cache
#else
	call0   _flush_i_cache
#endif

        movi    a15, .wsr0_ret_here
        jx      a4

        .align  4
.wsr0_ret_here:
	_RET

// A place holder that will be dynamically replaced with
// the correct wsr instruction. This has to be in RAM.
// We assume that that we are jumping back through a15.

        .align 4
.wsr0_instruction_placeholder:
        _or     a0, a0, a0
        jx      a15




//
//
// Data for _xmon_get_special_register
//
//

#if XCHAL_HAVE_BE
#define RSR0_INSN       0x00003000      // insn in upper 3 bytes
#else
#define RSR0_INSN       0x00030000      // insn in lower 3 bytes
#endif


// unsigned int  _xmon_get_special_register(unsigned special_register, unsigned *execute_here)
//
//      a2(input)  -- special_register
//      a2(output) -- contains the value
//      a3(input)  -- address for executing instructions

        .align  4
.global _xmon_get_special_register
_xmon_get_special_register:
	_ENTRY(sp,48)

// a5 -- temporary for moving memory
// a4 -- Points to our rsr instruction including ret
//       that we are going to copy to the execution point
// a3 -- Points to the execution point

        movi    a4, .rsr_instruction_placeholder
        l32i    a5, a4, 0
        s32i    a5, a3, 0
        l32i    a5, a4, 4
        s32i    a5, a3, 4

// a6 -- will hold the (munged) rsr instruction throughout

        movi    a6, RSR0_INSN

#if XCHAL_HAVE_BE

// Shift the special register number to the correct
// offset and OR it into our instruction
// a2 -- Holds the special register being read
// a6 -- instruction being massgaed

        slli    a2, a2, 16
        or      a6, a6, a2

// Now need to set the t-field of the instruction
// to be 2, which is the return value of this function
// a5 -- Temp that holds the constant being ord in
// a6 -- The instruction being massaged

        movi    a5, 2
        slli    a5, a5, 24
        or      a6, a6, a5

// Now load in the word from where we are going to execute
// the rsr, merge our rsr instruction, and store that word
// back to memory.
// a2 -- Temp for masking
// a3 -- Points to the correct memory location
// a5 -- Holds the WORD we are manipulating

        l32i    a5, a3, 0

// In Big Endian we save the LSB and put our
// instruction in the upper 3 bytes

        movi    a2, 0xff
        and     a5, a5, a2
        or      a6, a6, a5
        s32i    a6, a3, 0

#else /* XCHAL_HAVE_BE */

// Shift the special register number to the correct
// offset and OR it into our instruction
// a2 -- Holds the special register being read
// a6 -- instruction being massgaed

        slli    a2, a2, 8
        or      a6, a6, a2

// Now need to set the t-field of the instruction
// to be 2, which is the return value of this function
// a5 -- Temp that holds the constant being ord in
// a6 -- The instruction being massaged

        movi    a5, 2
        slli    a5, a5, 4
        or      a6, a6, a5

// Now load in the word from where we are going to execute
// the rsr, merge our rsr instruction, and store that word
// back to memory.
// a2 -- Temp for masking
// a3 -- Points to the correct memory location
// a5 -- Holds the WORD we are manipulating

        l32i    a5, a3, 0

// In Little Endian we save the MSB and put our
// instruction in the lower 3 bytes

        movi    a2, 0xff
        slli    a2, a2, 24
        and     a5, a5, a2
        or      a6, a6, a5
        s32i    a6, a3, 0

#endif /* XCHAL_HAVE_BE */

// Clear the cache line
// a3 -- Points to the location being cleared

        mov     a6, a3
#if XCHAL_HAVE_WINDOWED
        call4   _flush_i_cache
#else
	call0   _flush_i_cache
#endif

        movi    a2, 0
        movi    a15, .get_special_reg_retw
        jx      a3

        .align  4
.get_special_reg_retw:
	_RET


// A place holder that will be dynamically replaced with
// the correct rsr instruction. This has to be in RAM.
// We assume that that we are jumping back through a15.

        .align  4
.rsr_instruction_placeholder:
        _or     a0, a0, a0
        jx      a15




// void _xmon_set_user_register(unsigned user_register, unsigned value, unsigned *execute_here)
//      a2 -- user_register
//      a3 -- value to write
//      a4 -- pointer to memory to execute from


//      base wur instruction which we will
//       modify for the correct ar and user register number
#if XCHAL_HAVE_BE
#define WUR0_INSN       0x00003f00      // insn in upper 3 bytes
#else
#define WUR0_INSN       0x00f30000      // insn in lower 3 bytes
#endif

        .align 4
.global _xmon_set_user_register
_xmon_set_user_register:
	_ENTRY(sp,48)

// a6 -- temporary for moving memory
// a5 -- pointer to wur0_placeholder
// a4 -- points to the RAM location we will
//      execute from, move the base instruction
//      (including the retw) to that point.

        movi    a5, .wur0_instruction_placeholder
        l32i    a6, a5, 0
        s32i    a6, a4, 0
        l32i    a6, a5, 4
        s32i    a6, a4, 4

// a6 -- holds the modified instruction


        movi    a6, WUR0_INSN

#if XCHAL_HAVE_BE

// a2 -- contains the user register number which
//       we OR into the base instruction
        slli    a2, a2, 16
        or      a6, a6, a2

// a3 -- is the register that will be used in
//      the wur instruction, so we OR that value
//      into the instruction
// a5 -- temporary
// a6 -- wur instruction

        movi    a5, 3
        slli    a5, a5, 24
        or      a6, a6, a5

// a4 -- points to the caller supplied location
// a5 -- temporary used to merge our modified instruction
// a2 -- this is free again, and used as a temporary

        l32i    a5, a4, 0

// Save the lower 8-bits, our new wur instruction will occupy
// the upper 24-bits
        movi    a2, 0xff
        and     a5, a5, a2
        or      a6, a6, a5
        s32i    a6, a4, 0

// clear the cache line
        mov     a10, a4
#if XCHAL_HAVE_WINDOWED
        call8   _flush_i_cache
#else
	call0   _flush_i_cache
#endif

// Jump to the wur instruction
        movi    a15, .wur0_ret_here
        jx      a4

#else /* XCHAL_HAVE_BE */

// a2 -- holds the user register we are going to write
// a4 -- holds the location in memory that we are going
//      to execute from
// a6 -- holds the instruction we are going to execute

        slli    a2, a2, 8
        or      a6, a6, a2

// a2 -- Can be used as a temporary now, to
//       OR in the 3, which is the register that
//       holds the value we are going to write to
//       the user register

        movi    a2, 3
        slli    a2, a2, 4
        or      a6, a6, a2

// a2 -- Temporary for merging instructions
// a4 -- pointer to the location we are going to execute
//      from
// a5 -- Holds the value we load from our execution point
// a6 -- The instruction that we are going to execute

        l32i    a5, a4, 0

// Need to merge our 24-bit instruction with 8 bits
// from our execute point
// Want to use the lower 24 bits from a6,
// and the upper 8-bits from a5

        movi    a2, 0xff
        slli    a2, a2, 24
        and     a5, a5, a2
        or      a6, a6, a5
        s32i    a6, a4, 0

// Flush the cache
        mov     a10, a4
#if XCHAL_HAVE_WINDOWED
        call8   _flush_i_cache
#else
	call0   _flush_i_cache
#endif

        movi    a15, .wur0_ret_here
        jx      a4

#endif /* XCHAL_HAVE_BE */

        .align  4
.wur0_ret_here:
	_RET

// A place holder that will be dynamically replaced with
// the correct wur instruction. This has to be in RAM.
// We assume that that we are jumping back through a15.

        .align 4
.wur0_instruction_placeholder:
        _or     a0, a0, a0
        jx      a15




//
//
// Data for _xmon_get_user_register
//
//

#if XCHAL_HAVE_BE
#define RUR0_INSN       0x00003e00      // insn in upper 3 bytes
#else
#define RUR0_INSN       0x00e30000      // insn in lower 3 bytes
#endif


// unsigned int  _xmon_get_user_register(unsigned user_register, unsigned *execute_here)
//
//      a2(input)  -- user_register
//      a2(output) -- contains the value
//      a3(input)  -- address for executing instructions

        .align  4
.global _xmon_get_user_register
_xmon_get_user_register:
	_ENTRY(sp,48)

// a5 -- temporary for moving memory
// a4 -- Points to our rur instruction including ret
//       that we are going to copy to the execution point
// a3 -- Points to the execution point

        movi    a4, .rur_instruction_placeholder
        l32i    a5, a4, 0
        s32i    a5, a3, 0
        l32i    a5, a4, 4
        s32i    a5, a3, 4

// a6 -- will hold the (munged) rur instruction throughout

        movi    a6, RUR0_INSN

#if XCHAL_HAVE_BE

// In Big Endian the st field needs to be nibble swapped

// a2 -- The user register (being nibble swapped)
// a4 -- Temporary for nibble swapping
// a5 -- Nibble swapped value

        movi    a4, 0xf
        and     a5, a2, a4
        slli    a5, a5, 4

        srli    a2, a2, 4
        and     a2, a2, a4
        or      a5, a5, a2

// Now shift a5 to the appropriate position and OR
// it into our instruction
// a5 -- nibble swapped value for user_register
// a6 -- rur instruction

        slli    a5, a5, 20
        or      a6, a6, a5

// Now set the t-field to 2, which is the ar register
// that the rur will store into
// a5 -- Temporary for t-field value

        movi    a5, 2
        slli    a5, a5, 16
        or      a6, a6, a5

// Load in the instruction
        l32i    a4, a3, 0

// Merge bytes and store the insn back out
        movi    a2, 0xff
        and     a4, a4, a2
        or      a6, a6, a4
        s32i    a6, a3, 0

// clear the cache line

        mov     a10, a3
        call8   _flush_i_cache

#else /* XCHAL_HAVE_BE */

// Shift the user register number to the correct
// offset and OR it into our instruction
// a2 -- Holds the user register being read
// a6 -- instruction being massgaed

        slli    a2, a2, 4
        or      a6, a6, a2

// Now need to set the r-field of the instruction
// to be 2, which is the return value of this function
// a5 -- Temp that holds the constant being ord in
// a6 -- The instruction being massaged

        movi    a5, 2
        slli    a5, a5, 12
        or      a6, a6, a5

// Now load in the word from where we are going to execute
// the rur, merge our rur instruction, and store that word
// back to memory.
// a2 -- Temp for masking
// a3 -- Points to the correct memory location
// a5 -- Holds the WORD we are manipulating

        l32i    a5, a3, 0

// In Little Endian we save the MSB and put our
// instruction in the lower 3 bytes

        movi    a2, 0xff
        slli    a2, a2, 24
        and     a5, a5, a2
        or      a6, a6, a5
        s32i    a6, a3, 0

// Clear the cache line
// a3 -- Points to the location being cleared

        mov     a6, a3
#if XCHAL_HAVE_WINDOWED
        call4   _flush_i_cache
#else
	call0   _flush_i_cache
#endif

#endif /* XCHAL_HAVE_BE */

        movi    a2, 0
        movi    a15, .get_user_reg_retw
        jx      a3

        .align  4
.get_user_reg_retw:
	_RET


// A place holder that will be dynamically replaced with
// the correct rur instruction. This has to be in RAM.
// We assume that that we are jumping back through a15.

        .align  4
.rur_instruction_placeholder:
        _or     a0, a0, a0
        jx      a15



// Support for ExecuteSomeInstruction() in xtensa-xmon.c .
// Used to execute arbitrary code via GDB remote protocol Xtensa commands.

        .global g_dummy_entry_instruction
        .global g_dummy_retw_instruction
        .global g_dummy_entry_ptr
        .global g_dummy_retw_ptr

        .align  4
g_dummy_entry_instruction:
        _or     a0, a0, a0

        .align  4
g_dummy_retw_instruction:
        jx      a15



        .align  4
g_dummy_entry_ptr:
        .word   g_dummy_entry_instruction

        .align  4
g_dummy_retw_ptr:
        .word   g_dummy_retw_instruction


// void _xmon_execute_here(unsigned a4_value, void *execute_here);
//
// a2 -- value to be stuffed into a4
// a3 -- execute the instructions at this address
//

        .global _xmon_execute_here
        .align  4
_xmon_execute_here:
	_ENTRY(sp,16)
        mov     a4, a2
        movi    a15, .execute_here_retw
        jx      a3

        .align  4
.execute_here_retw:
	_RET

