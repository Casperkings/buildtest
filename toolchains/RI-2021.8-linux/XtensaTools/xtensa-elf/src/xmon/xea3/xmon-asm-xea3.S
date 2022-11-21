// debug-vector-mon.S - debug exception handler
//
// $Id: //depot/dev/Homewood/Xtensa/OS/xmon/debug-vector-mon.S#1 $

// Copyright (c) 1999-2013 Tensilica Inc.
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

// Imports
.global _flush_i_cache
.global xmon_C_handler
.global _ar_registers
.global _sr_registers
//.global xmon_user_intr_handler
.global gInXmon;

.global _old_a1
.global _old_a0

#include <xtensa/coreasm.h>
#include "xmon-internal.h"

#if __XTENSA_WINDOWED_ABI__
# define  RET			retw
# define  CALLX(reg)	        callx8 reg
#else
# define  RET			ret
# define  CALLX(reg)	        callx0 reg
#endif

#define SAVE(r, offs)               \
        rsr     a5, r;              \
        s32i    a5, a7, (offs*4)
#define RESTORE(r)                  \
        l32i    a5, a7, ((r)*4);    \
        wsr     a5, r

// if XMON is reentered we need to still keep accessing the same exception 
// frames and all ARs/SRs set by the original exception.
// FIXME: in XEA3 re-entrancy keeps filling the exception stack.
// FIXME: How to avoid Double Exception on reentrancy ?

        // a1 must be preserved as we use the same stack
	// a2 must be preserved as thats excFrame arg
	// a0 - return PC from the whole handler

        .text
        .global _DebugInterruptHandler
        .align  4
	.type _DebugInterruptHandler,@function
_DebugInterruptHandler:
        // If XMON is re-entered just exit.  We just let C handler do all that.
	movi    a7, gInXmon
        l32i    a7, a7, 0
	mov     a2, a1  //excFrame as arg1
	// Assign the entry cause (interrupt)
	movi    a3,  1
	// Jump to save regs (a3 must be preserved there) & execute C handler
        bnez    a7, reg_save_done
        j      _xmon_save_registers


        .text
        .align  4
        .global _DebugExceptionHandler
	.type _DebugExceptionHandler,@function
_DebugExceptionHandler:
	// If reentered, just call handler directly
	movi    a7, gInXmon
        l32i    a7, a7, 0
	// Assign the entry cause (exception)
	movi    a3,  0
        bnez    a7, reg_save_done
        j      _xmon_save_registers

	.text
        .align  4
	.type _xmon_save_registers,@function
_xmon_save_registers:

	// a3 must be preserved as that's the return address from this function
	// a1 must be preserved as we use the same stack
	// a2 must be preserved as thats excFrame arg
	// a0 - return PC from the whole handler

	/* First save some SRs dispatch doesn't save or we save in a special way */

	 movi a7, _sr_registers
#if XCHAL_HAVE_CCOUNT
	 SAVE(ccount, CCOUNT)
#endif
	 SAVE(eraccess, ERACCESS)
	 
	 // Save WB so we can write ARs by simply writing their save location
	 // which then gets restored, and because WB is restored, live values 
	 // are valid.  FIXME - Speed up the restore by making it conditional on the 
	 // existence of write AR commands from GDB.
#if __XTENSA_WINDOWED_ABI__
        SAVE(wb, WB)
#endif

       /* Now save ARs - need for Windowed ABI only */
    
#if __XTENSA_WINDOWED_ABI__
        /* Save AR registers by rotating the window. */

	movi    a7, _ar_registers
	movi    a6, 0xf
        and     a4, a5, a6          // a4 = wb.p
	// Save WB as we will clear wb.s temporarily (FIXME - is this needed when PS.ENTRYNR is 1 as well ?? )
	movi    a6, 0x3fffffff
	and     a6, a5, a6          // a6 = wb (wb.s=call0)
	mov     a5, a4
	wsr.wb  a6
        slli    a4, a4, 5           // WB * 32 (window pane size)
        add     a4, a7, a4          // a4: &save_area+wb*32 (i.e. save area for current window )

        /*  Now loop to save all remaining physical address registers:  */
        movi    a7, (XCHAL_NUM_AREGS/8)-1    /* how many loops left */
        addi    a5, a5, 1                    /* next pane to save */
        addi    a4, a4, 32                   /* ditto */

.LoopS: bbci.l  a5, XCHAL_NUM_AREGS_LOG2-3, 1f
        addi    a4, a4, -(XCHAL_NUM_AREGS*4)    /* wrap-around to start */
        movi    a5, 0                           /* ditto */

	/* save one pane ... */
1:      s32i    a8,  a4, 0
        s32i    a9,  a4, 4
        s32i    a10, a4, 8
        s32i    a11, a4, 12
	s32i    a12, a4, 16
        s32i    a13, a4, 20
        s32i    a14, a4, 24
        s32i    a15, a4, 28

	addi    a13, a5, 1               /* next pane to save */
        addi    a12, a4, 32              /* move save area for 8*4 */
        addi    a15,  a7, -1              /* loop count */
        rotw    1
        bnez    a7, .LoopS              /* loop until done */
        rotw    1                       /* back to original WindowBase */
        //restore wb (had wb.s cleared above)
        movi     a7, _sr_registers
	l32i     a7, a7, (WB*4)
        //restore wb (had wb.s cleared above)
        wsr.wb   a7
#endif

reg_save_done:
	// Save done, prepare some regs (stack, WB) then can start C handler now
	movi    a6, 0xffffffff
	wsr.eraccess a6
	//
	movi    a4,  ~PS_ENTRYNR_MASK
	movi    a5,  PS_ENTRYNR_MASK
	xps     a4,  a5
	// Set arguments
	mov     a11, a3  
	mov     a10, a2
	mov     a9,  a1
	// Call C handler
	movi    a4,  xmon_C_handler
	isync
	CALLX(a4)
	#
	#
	bnez    a2, reg_restore_done   // a2=gInXmon, if set don't restore
	movi    a3, reg_restore_done
        j      _xmon_restore_registers
	.align 4
reg_restore_done:
	RET


	.text
        .align  4
	.type _xmon_restore_registers, @function
_xmon_restore_registers:
         movi    a7, _sr_registers

         /* Restore ARs - need for Windowed ABI only */
        #if __XTENSA_WINDOWED_ABI__
        RESTORE(WB)
	movi    a7, _ar_registers
	movi    a6, 0xf
        and     a4, a5, a6          // a4 = wb.p
	// No need to clear WB.S as underflows don't matter - once we restore
	// WB all live ARs are valid and all spilled to the stack is unimportant
	// However, WB.S is still disabled not to have Overflow/Underflow overhead.
	movi    a6, 0x3fffffff
	and     a6, a5, a6          // a6 = wb (wb.s=call0)
	wsr.wb  a6
	mov     a5, a4
        //
        slli    a4, a4, 5           // WB * 16 (window pane size)
        add     a4, a7, a4          // a4: &save_area+wb*16 (i.e. save area for current window )

        /*  loop to restore all remaining physical address registers:  */
        movi    a7, (XCHAL_NUM_AREGS/8)-1    /* how many loops left */
        addi    a5, a5, 1                    /* next pane to save */
        addi    a4, a4, 32                   /* ditto */

.LoopR: bbci.l  a5, XCHAL_NUM_AREGS_LOG2-3, 1f
        addi    a4, a4, -(XCHAL_NUM_AREGS*4)    /* wrap-around to start */
        movi    a5, 0                           /* ditto */

	/* restore one pane ... */
1:      l32i    a8,  a4, 0
        l32i    a9,  a4, 4
        l32i    a10, a4, 8
        l32i    a11, a4, 12
	l32i    a12, a4, 16
        l32i    a13, a4, 20
        l32i    a14, a4, 24
        l32i    a15, a4, 28

	addi    a13, a5, 1               /* next pane to save */
        addi    a12, a4, 32              /* move save area for 8*4 */
        addi    a15,  a7, -1              /* loop count */
        rotw    1
        bnez    a7, .LoopR              /* loop until done */
        rotw    1                       /* back to original WindowBase */
#endif

       /* Now restore special regs */
       movi     a7, _sr_registers
 
#if __XTENSA_WINDOWED_ABI__
	// WB was changed above (WB.S set to call0) but that doesn't matter
	// any more (don't restore) as we restore the interrupted code.
        RESTORE(WB)
#endif
#if XCHAL_HAVE_CCOUNT
        RESTORE(CCOUNT)
#endif
	RESTORE(ERACCESS)
	jx a3
	

#################################################
############### FUNCTIONS #######################
#################################################

#if __XTENSA_WINDOWED_ABI__
# define _RET			retw
# define _ENTRY(sp,N)		entry	sp, N
# define _CALL(reg,func)	mov     a10, reg; \
				call8	func
#else
# define _RET			ret
# define _ENTRY(r,N)
# define _CALL(reg,func)	movi    a2, .save_a0_call0;	\
				s32i    a0, a2, 0;		\
				mov     a2, reg;		\
				call0	func;			\
				movi    a2, .save_a0_call0;	\
				l32i    a0, a2, 0
#endif

.save_a0_call0:
	.space 8

## FUNCTIONS ##

        .align  4
_flush_i_cache:
       _ENTRY(sp,64)

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


//      base xSR/xUR instruction which we will
//       modify for the correct ar and special register number

#if XCHAL_HAVE_BE
// insn in upper 3 bytes
#define RSR2_INSN       0x020030
#define RUR2_INSN       0x00023e
#define WSR3_INSN       0x030031
#define WUR3_INSN       0x03003f
#else
// insn in lower 3 bytes
#define RSR2_INSN       0x030020
#define RUR2_INSN       0xe32000
#define WSR3_INSN       0x130030
#define WUR3_INSN       0xf30030
#endif


// Placeholder for xSR or xUR instructions
        .align  4
        .byte   0
.xSR_xUR_instruction_placeholder:
        _or     a0, a0, a0
        jx      a9

// Return for xSR or xUR instructions
        .align  4
.xSR_xUR_ret:
        _RET
        .align 4

# define _EXEC_xSR_xUR()	movi    a9, .xSR_xUR_ret;	\
					addi    a5, a5, 1;      \
					jx      a5;             \

// void xmon_set_spec_reg(unsigned reg, unsigned value)

// a2 -- register number.
// a4 -- value to write
// a6 -- instruction under construction
// a5 -- pointer to the instruction placeholder

        .align 4
.global xmon_set_spec_reg
xmon_set_spec_reg:
        _ENTRY(sp,64)

        movi    a5, .xSR_xUR_instruction_placeholder - 1

        movi    a6, WSR3_INSN
        slli    a2, a2, 8
        or      a6, a6, a2
#if (!XCHAL_HAVE_BE)
        slli    a6, a6, 8
#endif
        s32i    a6, a5, 0

        _CALL(a5, _flush_i_cache)
        _EXEC_xSR_xUR()


// void _xmon_set_user_reg(unsigned reg, unsigned value)

// a2 -- register number.
// a3 -- value to write

        .align 4
.global _xmon_set_user_reg
_xmon_set_user_reg:
        _ENTRY(sp,64)

        movi    a5, .xSR_xUR_instruction_placeholder - 1
        movi    a6, WUR3_INSN

        slli    a2, a2, 8
        or      a6, a6, a2
#if (!XCHAL_HAVE_BE)
        slli    a6, a6, 8
#endif
        s32i    a6, a5, 0

        _CALL(a5, _flush_i_cache)
        _EXEC_xSR_xUR()


// unsigned int  _xmon_get_spec_reg(unsigned reg)

// a2 -- register number and the return val.

        .align  4
.global _xmon_get_spec_reg
_xmon_get_spec_reg:
        _ENTRY(sp,64)

        movi    a5, .xSR_xUR_instruction_placeholder - 1

        movi    a6, RSR2_INSN

        slli    a2, a2, 8
        or      a6, a6, a2
#if (!XCHAL_HAVE_BE)
        slli    a6, a6, 8
#endif
        s32i    a6, a5, 0

        _CALL(a5, _flush_i_cache)
        movi    a2, 0

        _EXEC_xSR_xUR()


// unsigned int  _xmon_get_user_reg(unsigned reg)

// a2 -- register number and the return val.

        .align  4
.global _xmon_get_user_reg
_xmon_get_user_reg:
        _ENTRY(sp,64)

        movi    a5, .xSR_xUR_instruction_placeholder - 1

        movi    a6, RUR2_INSN
#if XCHAL_HAVE_BE
// In Big Endian the st field needs to be nibble swapped

// a2 -- The user register (being nibble swapped)
// a4 -- Temporary for nibble swapping
// a3 -- Nibble swapped value

        movi    a4, 0xf
        and     a3, a2, a4
        slli    a3, a3, 4

        srli    a2, a2, 4
        and     a2, a2, a4
        or      a3, a3, a2
        slli    a3, a3, 12
        or      a6, a6, a3
#else
        slli    a2, a2, 4
        or      a6, a6, a2
        slli    a6, a6, 8
#endif
        s32i    a6, a5, 0

        _CALL(a5, _flush_i_cache)
        movi    a2, 0

        _EXEC_xSR_xUR()




// Support for executing instructions in xmon.c
// Used to execute tie instruction (TODO: arbitrary code via GDB remote protocol Xtensa commands)

        .global g_retw_instr
        .weak _xmon_exec_instr
        .global _xmon_exec_here

        .align  4
g_retw_instr:
        _RET

// void _xmon_exec_here(unsigned a4_value);
//
// a2 -- value to be stuffed into a4
//
        .align  4
_xmon_exec_here:
        _ENTRY(sp, 64)
        mov     a4, a2
        movi    a15, _xmon_exec_instr
        jx      a15
_xmon_exec_instr:
        .space 64, 0


	.text
        .align  4
	.global _xmon_reentry
_xmon_reentry:
	call0 _xmon_longjmp
