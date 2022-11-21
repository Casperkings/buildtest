// debug-vector-mon.S - debug exception handler
//
// $Id: //depot/rel/Homewood/ib.8/Xtensa/OS/xmon/debug-vector-mon.S#1 $

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

/*
Notes on debugging the XMON debug monitor with xt-gdb under OCD/JTAG
--------------------------------------------------------------------

To debug XMON under OCD it is necessary to lower PS.INTLEVEL to debug-1
inside XMON (the debug exception h/w sets it to debug level) so that
debug exceptions give control to OCD. Since there are no interrupts at
the same level as debug, this makes no difference to interrupts. It
does, however, affect debug events such as ICOUNT and h/w breakpoints,
so it is necessary to first lower ICOUNTLEVEL to debug-1 so that these
events don`t happen inside XMON. The user ICOUNTLEVEL must first be
saved so XMON can see what it was. It is also necessary to restore
PS.INTLEVEL to the debug level before EPC[debug] and EPS[debug] are
restored to the user PC and PS (resp.) so that OCD won`t destroy them.

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

/*
Used to detect if XMON is re-entered due to an exception. Assumption is that this happens only
if XMON execution of an instruction generated an exception. Thus, we just exit from this XMON and
set EPC[debug], leaving original XMON to process the CATCH and signal to GDB
TODO - reading EXCCAUSE (here or back in CATCH is needed)
*/
.global _in_xmon;

#include <xtensa/coreasm.h>

#if __XTENSA_WINDOWED_ABI__
# define _RET			retw
# define _ENTRY(sp,N)		entry	sp, N
# define _CALL(reg,func)	mov     a10, reg; \
				call8	func
# define RESTORE_RET
#else

.save_a0_call0:
	.space 8

# define _RET			ret
# define _ENTRY(r,N)
# define _CALL(reg,func)	movi    a2, .save_a0_call0;	\
				s32i    a0, a2, 0;		\
				mov     a2, reg;		\
				call0	func;			\
				movi    a2, .save_a0_call0;	\
				l32i    a0, a2, 0
#endif

// Parameters
#define XMON_STACK_SIZE (2048+1024)

/********************************************************************

 The assembler portion of the debug handler begins here.  The handler
does three major things.  First, it saves the processor state.  The
bulk of the save sequence saves all the address registers.  Note that
we don't try to save the registers into interrupted process' stack
because it may have become corrupted and the debugger wants to perturb
the processor state as little as possible.  Second, the handler sets
up the run-time environment for the debugger stuff, which we have
written in C.  Third, upon return from the stub, we restore the
interrupted process` registers, and resume the process.  The debugger
can force the process to resume at an alternative pc by overwriting
the saved value of the appropriate EPC.

*********************************************************************/

        .section .bss
        .align  16
_xmon_stack:
        .space  XMON_STACK_SIZE
_xmon_stack_bot:


        .text
        .literal_position

        //  This code goes at the debug exception vector

        // MG FIXME: probably eventually need to save/restore LITBASE, and need to
        // save it here (before call0) because assembler/linker might expand that
        // to a "l23r; callx0" sequence and LITBASE must be set properly before l32r.

//      .begin  literal_prefix  .DebugExceptionVector
        .section                .DebugExceptionVector.text, "ax"
        .align 4
        .global _DebugExceptionVector
_DebugExceptionVector:
        wsr     a0, EXCSAVE+XCHAL_DEBUGLEVEL    // save a0 where simulator expects it
        call0   _DebugExceptionHandler          // this is a jump not a call
        //  Doesnt return here.  Call0 only used because it sizes dynamically
        //  when this assembly file is assembled with -mlongcalls.

//      .end    literal_prefix


/*
 *  Debug exception handler.
 *
 *  This gets invoked from the debug exception vector,
 *  so a0 has been saved in EXCSAVE[DEBUGLEVEL] and is available.
 *  PS.EXCM (XEA2) is set. PS.INTLEVEL is DEBUGLEVEL or DEBUGLEVEL-1
 *  (the latter allows this code itself to be debugged by OCD/JTAG).
 *
 *  Note that no interrupts can ever be configured at the debug level,
 *  so all debugable interrupts are disabled even at DEBUGLEVEL-1.
 *  Higher priority (non-debugable) interrupts may continue to be
 *  serviced by their handlers transparently to this debug monitor.
 */

        /* Macros for saving and restoring special regs. */
#define SAVE(r)                     \
        rsr     a2, r;              \
        s32i    a2, a0, ((r)*4)
#define SAVE_EARLY(r)               \
        rsr     a1, r;              \
        s32i    a1, a2, ((r)*4)
#define RESTORE(r)                  \
        l32i    a2, a0, ((r)*4);    \
        wsr     a2, r


        .text
        .align  4
        .global _DebugExceptionHandler
_DebugExceptionHandler:
        // First test if xmon is re-entered
        movi    a0, _in_xmon
        l32i    a0, a0, 0
        bnez    a0, xmon_reentered

        /* Save a1,a2 into temporary places so that we can setup
           the save sequence (a0 is saved in EXCSAVE[debuglevel]).
           Notice that we need to take care that this code works even
           when a0, a1 contain the same value. See the NOOP comments.
        */

        movi    a0, _ar_registers
        s32i    a1, a0, 4
        s32i    a2, a0, 8
#ifdef __XTENSA_CALL0_ABI__
        rsr     a1, EXCSAVE+XCHAL_DEBUGLEVEL // a0 was saved just temporarily
        s32i    a1,  a0, 0                   // save a0

        s32i    a3,  a0, 12
        s32i    a4,  a0, 16
        s32i    a5,  a0, 20
        s32i    a6,  a0, 24
        s32i    a7,  a0, 28
        s32i    a8,  a0, 32
        s32i    a9,  a0, 36
        s32i    a10, a0, 40
        s32i    a11, a0, 44
        s32i    a12, a0, 48
        s32i    a13, a0, 52
        s32i    a14, a0, 56
        s32i    a15, a0, 60
#endif

        /* Save debug and certain other special registers early. */
        movi    a2, _sr_registers
#if XCHAL_HAVE_CCOUNT
        SAVE_EARLY(CCOUNT)                   /* save CCOUNT early for precision */
#endif
        SAVE_EARLY(EPC+XCHAL_DEBUGLEVEL)     /* this is the user PC */
        SAVE_EARLY(EPS+XCHAL_DEBUGLEVEL)     /* this is the user PS */
        SAVE_EARLY(EXCSAVE+XCHAL_DEBUGLEVEL) /* only for reg display */
        SAVE_EARLY(DEBUGCAUSE)
//#if XCHAL_NUM_DBREAK >= 1
//        SAVE_EARLY(DBREAKC_0)
//#endif
//#if XCHAL_NUM_DBREAK >= 2
//        SAVE_EARLY(DBREAKC_1)
//#endif
//#if XCHAL_NUM_IBREAK >= 1
//        SAVE_EARLY(IBREAKENABLE)
//#endif

        /* FIXME: Is there any point in saving the user ICOUNT & ICOUNTLEVEL ? */
        SAVE_EARLY(ICOUNT)
        SAVE_EARLY(ICOUNTLEVEL)         /* Leaves a1 == ICOUNTLEVEL */
#ifdef XMON_OCD_DEBUG
        /* PS.INTLEVEL has been set to XCHAL_DEBUGLEVEL by hardware.
           Lower it by 1 level to allow OCD to debug this handler.
           Must save and lower ICOUNTLEVEL before lowering PS.INTLEVEL.
           The debug registers have been saved before OCD can get control.
         */
        blti    a1, XCHAL_DEBUGLEVEL, 1f
        movi    a1, XMON_DEBUGLEVEL
        wsr     a1, ICOUNTLEVEL
        isync
1:      rsil    a1, XMON_DEBUGLEVEL
        /* It is possible to debug with OCD beyond this point. */
#endif

        SAVE_EARLY(PS)                  /* save [modified] debug PS */

#if __XTENSA_WINDOWED_ABI__
        /* Save AR registers by rotating the window. */

        SAVE_EARLY(WINDOWSTART)
        SAVE_EARLY(WINDOWBASE)          /* Leaves a1 == WINDOWBASE */
        slli    a1, a1, 4               /* WB * 16 (window pane size) */
        add     a1, a0, a1              /* Point to save area for pane */
        /* At this point:
                a0: base address of save area.
                a1: &save_area+wb*16 (i.e. save area for current window )
                XEA1: Debug exception has cleared WOE.
                XEA2: Debug exception has set EXCM (disabling WOE).
           We must ensure that code below works even when a0==a1
           Save the current window pane, pulling a0-a2 from their temp. places.
        */
        rsr     a2, EXCSAVE+XCHAL_DEBUGLEVEL
        s32i    a2, a1, 0               /* save a0 */
        l32i    a2, a0, 4
        s32i    a2, a1, 4               /* save a1; NOOP if a0==a1 */
        l32i    a2, a0, 8
        s32i    a2, a1, 8               /* save a2; NOOP if a0==a1 */
        s32i    a3, a1, 12              /* save a3 */

        /*  Now loop to save all remaining physical address registers:  */
        movi    a0, (XCHAL_NUM_AREGS/4)-1       /* how many loops left */
        rsr     a2, WINDOWBASE
        addi    a2, a2, 1               /* next pane to save */
        addi    a1, a1, 16              /* ditto */
.LoopS: bbci.l  a2, XCHAL_NUM_AREGS_LOG2-2, 1f
        addi    a1, a1, -(XCHAL_NUM_AREGS*4)    /* wrap-around to start */
        movi    a2, 0                           /* ditto */
1:      s32i    a4, a1, 0               /* save one pane ... */
        s32i    a5, a1, 4
        s32i    a6, a1, 8
        s32i    a7, a1, 12
        addi    a6, a2, 1               /* next pane to save */
        addi    a5, a1, 16              /* ditto */
        addi    a4, a0, -1              /* loop count */
        rotw    1
        bnez    a0, .LoopS              /* loop until done */
        rotw    1                       /* back to original WindowBase */

#endif //if __XTENSA_WINDOWED_ABI__

/* Save remaining special registers.
   We key it by testing the presence of register numbers.
   When present, the numbers indicate the user has configured
   the process to have the corresponding processor options.
   Note this doesnt quite work for TIE instructions yet.
*/
        movi    a0, _sr_registers
#if XCHAL_HAVE_MAC16
        SAVE(ACCLO)
        SAVE(ACCHI)
        SAVE(MR_0)
        SAVE(MR_1)
        SAVE(MR_2)
        SAVE(MR_3)
#endif
#if XCHAL_HAVE_LOOPS
        SAVE(LBEG)
        SAVE(LCOUNT)
        SAVE(LEND)
#endif
        SAVE(SAR)
        SAVE(EPC_1)
        SAVE(EXCSAVE_1)
        SAVE(EXCCAUSE)
#if XCHAL_HAVE_XEA2
        SAVE(DEPC)
        SAVE(EXCVADDR)
#endif

/* FIXME: disable saving regs below? (not needed to run C code)
 * Also, instead read/write most SRs  (not saved above) in same way as URs
 */
#if 0  /* DISABLE ALL ??? */

#if XCHAL_HAVE_BOOLEANS
        SAVE(BR)
#endif
#if XCHAL_HAVE_CACHEATTR
        SAVE(CACHEATTR)
#endif
#if XCHAL_NUM_TIMERS >= 1
        SAVE(CCOMPARE_0)
#endif
#if XCHAL_NUM_TIMERS >= 2
        SAVE(CCOMPARE_1)
#endif
#if XCHAL_NUM_TIMERS >= 3
        SAVE(CCOMPARE_2)
#endif
#if XCHAL_HAVE_CP
        SAVE(CPENABLE)
#endif
//#if XCHAL_NUM_DBREAK >= 1
//        SAVE(DBREAKA_0)
//#endif
//#if XCHAL_NUM_DBREAK >= 2
//        SAVE(DBREAKA_1)
//#endif
//#if XCHAL_NUM_IBREAK >= 1
//        SAVE_EARLY(IBREAKA_0)
//#endif
//#if XCHAL_NUM_IBREAK >= 2
//        SAVE_EARLY(IBREAKA_1)
//#endif

/* No  point in saving interrupt levels above debuglevel as those
 * arenot debuggable (can change while XMON has the app stopped).
 * XMON should read those regs directly, and not write them at all.
 * The debug level regs have been saved earlier.
 */
#if XCHAL_NUM_INTLEVELS >= 2 && XCHAL_DEBUGLEVEL > 2
        SAVE(EPC_2)
        SAVE(EPS_2)
        SAVE(EXCSAVE_2)
#endif
#if XCHAL_NUM_INTLEVELS >= 3 && XCHAL_DEBUGLEVEL > 3
        SAVE(EPC_3)
        SAVE(EPS_3)
        SAVE(EXCSAVE_3)
#endif
#if XCHAL_NUM_INTLEVELS >= 4 && XCHAL_DEBUGLEVEL > 4
        SAVE(EPC_4)
        SAVE(EPS_4)
        SAVE(EXCSAVE_4)
#endif
#if XCHAL_NUM_INTLEVELS >= 5 && XCHAL_DEBUGLEVEL > 5
        SAVE(EPC_5)
        SAVE(EPS_5)
        SAVE(EXCSAVE_5)
#endif
#if XCHAL_NUM_INTLEVELS >= 6 && XCHAL_DEBUGLEVEL > 6
        SAVE(EPC_6)
        SAVE(EPS_6)
        SAVE(EXCSAVE_6)
#endif
#if XCHAL_HAVE_NMI && XCHAL_DEBUGLEVEL > XCHAL_NMILEVEL /* never true */
        SAVE(EPC+XCHAL_NMILEVEL)
        SAVE(EPS+XCHAL_NMILEVEL)
        SAVE(EXCSAVE+XCHAL_NMILEVEL)
#endif
        SAVE(INTERRUPT)

#endif  /* DISABLE ALL OF THIS!?!? */

xmon_reentered:
#if XCHAL_HAVE_CP
         movi    a1, 0xffffffff
         wsr     a1, CPENABLE
#endif

/* Disable Interrupts at and below DEBUGLEVEL (including any above EXCM level).
   In XEA1, we have to use INTENABLE and protect the critical section in which
   a hi-pri interrupt above debug level might modify it and we`d wipe that out.
   WARNING: NMI cannot be masked so on XEA1 systems should not modify INTENABLE.
   In XEA2, the hardware has already set PS.INTLEVEL to debuglevel.
 */

        movi    a2, 0

        /* Clear ICOUNT, disable ICOUNT and h/w breakpoints while inside XMON */
        // RM FIXME: Some or all of this might be redundant in XEA2.
        wsr     a2, ICOUNTLEVEL
        wsr     a2, ICOUNT
//#if XCHAL_NUM_IBREAK > 0
//        wsr     a2, IBREAKENABLE
//#endif
//#if XCHAL_NUM_DBREAK >= 1
//        wsr     a2, DBREAKC_0
//#endif
//#if XCHAL_NUM_DBREAK >= 2
//        wsr     a2, DBREAKC_1
//#endif
        isync

/* Set up PS to run C code: set UM and WOE, and clear EXCM.
   In XEA1 we need to set PS.INTLEVEL to 0, having already disabled all
   debugable interrupts. In XEA2 we keep PS.INTLEVEL at debug level.
 */
        movi    a1, PS_WOE | PS_UM | PS_INTLEVEL(XMON_DEBUGLEVEL)
        wsr     a1, PS

        movi    a0, 0
        movi    a2, 1
#if __XTENSA_WINDOWED_ABI__
        wsr     a2, WINDOWSTART         /* window start = 1 */
        wsr     a0, WINDOWBASE          /* window base = 0 */
#endif
        rsync

        /* Initialize our stack and call handler */
        movi    a0, 0                   /* terminate call stack */
        movi    a1, _xmon_stack_bot-4*16

#if __XTENSA_WINDOWED_ABI__
        movi    a4, _handle_exception   /* Coded in C (does not change PS.INTLEVEL) */
        callx4  a4
#else
        movi    a0, _handle_exception   /* Coded in C (does not change PS.INTLEVEL) */
        callx0  a0
#endif


        /* Done with C code */



        /* Restore special registers */
        movi    a0, _sr_registers

        /* Restore debug PS first to lock out interrupts below debug level. */
        RESTORE(PS)
        rsync

#if XCHAL_HAVE_MAC16
        RESTORE(ACCLO)
        RESTORE(ACCHI)
        RESTORE(MR_0)
        RESTORE(MR_1)
        RESTORE(MR_2)
        RESTORE(MR_3)
#endif
#if XCHAL_HAVE_LOOPS
        RESTORE(LBEG)
        RESTORE(LCOUNT)
        RESTORE(LEND)
#endif
        RESTORE(SAR)
        RESTORE(EPC_1)
        RESTORE(EXCSAVE_1)
        RESTORE(EXCCAUSE)
#if XCHAL_HAVE_XEA2
        RESTORE(DEPC)
        RESTORE(EXCVADDR)               /* this one maybe also in some XEA1 releases??? */
#endif

        /********* MG FIXME: should not restore all this below
         **  (see comment above for save)
         */

#if 0   /* DISABLE ALL OF THIS ?? */

#if XCHAL_HAVE_BOOLEANS
        RESTORE(BR)
#endif
#if XCHAL_HAVE_CACHEATTR
        RESTORE(CACHEATTR)
#endif
#if XCHAL_NUM_TIMERS >= 1
        RESTORE(CCOMPARE_0)
#endif
#if XCHAL_NUM_TIMERS >= 2
        RESTORE(CCOMPARE_1)
#endif
#if XCHAL_NUM_TIMERS >= 3
        RESTORE(CCOMPARE_2)
#endif
#if XCHAL_HAVE_CP
        RESTORE(CPENABLE)
#endif
//#if XCHAL_NUM_DBREAK >= 1
//        RESTORE(DBREAKA_0)
//#endif
//#if XCHAL_NUM_DBREAK >= 2
//        RESTORE(DBREAKA_1)
//#endif
//#if XCHAL_NUM_IBREAK >= 1
//        RESTORE(IBREAKA_0)
//#endif
//#if XCHAL_NUM_IBREAK >= 2
//        RESTORE(IBREAKA_1)
//#endif

/*
 We did not save interrupt levels above debuglevel (not debuggable)
 because those regs can change while XMON has the application stopped.
 XMON should read those regs directly, and not write them at all.
 The debug level regs will be restored later.
*/
#if XCHAL_NUM_INTLEVELS >= 2 && XCHAL_DEBUGLEVEL > 2
        RESTORE(EPC_2)
        RESTORE(EPS_2)
        RESTORE(EXCSAVE_2)
#endif
#if XCHAL_NUM_INTLEVELS >= 3 && XCHAL_DEBUGLEVEL > 3
        RESTORE(EPC_3)
        RESTORE(EPS_3)
        RESTORE(EXCSAVE_3)
#endif
#if XCHAL_NUM_INTLEVELS >= 4 && XCHAL_DEBUGLEVEL > 4
        RESTORE(EPC_4)
        RESTORE(EPS_4)
        RESTORE(EXCSAVE_4)
#endif
#if XCHAL_NUM_INTLEVELS >= 5 && XCHAL_DEBUGLEVEL > 5
        RESTORE(EPC_5)
        RESTORE(EPS_5)
        RESTORE(EXCSAVE_5)
#endif
#if XCHAL_NUM_INTLEVELS >= 6 && XCHAL_DEBUGLEVEL > 6
        RESTORE(EPC_6)
        RESTORE(EPS_6)
        RESTORE(EXCSAVE_6)
#endif
#if XCHAL_HAVE_NMI && XCHAL_DEBUGLEVEL > XCHAL_NMILEVEL /* never true */
        RESTORE(EPC+XCHAL_NMILEVEL)
        RESTORE(EPS+XCHAL_NMILEVEL)
        RESTORE(EXCSAVE+XCHAL_NMILEVEL)
#endif
        RESTORE(INTERRUPT)      /* only s/w interrupt bits will take effect */

#endif  /* DISABLE ALL OF THIS ?? */

#if __XTENSA_WINDOWED_ABI__

        /*  Now loop to restore physical address registers:  */
        RESTORE(WINDOWSTART)
        RESTORE(WINDOWBASE)
        rsync                           /* let WindowBase update take effect */
        rotw    -1                      /* restore in reverse order of saving */
        movi    a0, (XCHAL_NUM_AREGS/4)-1       /* how many loops left */
        movi    a1, _ar_registers
        rsr     a2, WINDOWBASE
        slli    a3, a2, 4
        add     a1, a1, a3              /* start at a1 = _ar_registers + WB*16 */
.LoopR: rotw    -1
        addi    a0, a4, -1              /* loop count */
        addi    a2, a6, -1              /* next pane to restore */
        addi    a1, a5, -16             /* next pane`s save area */
        bgez    a2, 1f                  /* wrap-around to top pane ? */
2:
        addi    a1, a1, XCHAL_NUM_AREGS*4       /* yes */
        movi    a2, (XCHAL_NUM_AREGS/4)-1       /* ditto */
1:
        l32i    a4, a5, 0               /* restore one pane ... */
        l32i    a6, a5, 8
        l32i    a7, a5, 12
        l32i    a5, a5, 4
        bnez    a0, .LoopR              /* loop until done */
        /* Back to original WindowBase with a1 = save area; a0,a2,a3 available. */
#else
        movi    a0, _ar_registers
        l32i    a4,  a0, 16
        l32i    a5,  a0, 20
        l32i    a6,  a0, 24
        l32i    a7,  a0, 28
        l32i    a8,  a0, 32
        l32i    a9,  a0, 36
        l32i    a10, a0, 40
        l32i    a11, a0, 44
        l32i    a12, a0, 48
        l32i    a13, a0, 52
        l32i    a14, a0, 56
        l32i    a15, a0, 60
#endif

        /* Restore ICOUNT and ICOUNTLEVEL, possibly to values set by XMON). */
        movi    a0, _sr_registers

#ifdef XMON_OCD_DEBUG
        /*
        Lock out OCD before restoring registers it will or might destroy.
        Cannot debug with OCD beyond this point.
        Also must ensure PS.INTLEVEL >= old and new ICOUNTLEVEL before restoring
        ICOUNTLEVEL.
        */
        rsil    a2, XCHAL_DEBUGLEVEL
#endif
        /* At this point, PS.INTLEVEL == debug level >= ICOUNTLEVEL. */
        RESTORE(ICOUNT)
        RESTORE(ICOUNTLEVEL)

        /* Restore debug registers. */
        RESTORE(EPC+XCHAL_DEBUGLEVEL)       /* this is the user PC */
        RESTORE(EPS+XCHAL_DEBUGLEVEL)       /* this is the user PS */
//      RESTORE(EXCSAVE+XCHAL_DEBUGLEVEL)   /* no point in restoring this */
//      RESTORE(DEBUGCAUSE)                 /* not writable */
//#if XCHAL_NUM_DBREAK >= 1
//        RESTORE(DBREAKC_0)
//#endif
//#if XCHAL_NUM_DBREAK >= 2
//        RESTORE(DBREAKC_1)
//#endif
//#if XCHAL_NUM_IBREAK >= 1
//        RESTORE(IBREAKENABLE)
//#endif
#if XCHAL_HAVE_CCOUNT
        RESTORE(CCOUNT)
#endif

        /*  And finally, restore a0 .. a3:  */
#if __XTENSA_WINDOWED_ABI__
        l32i    a0, a1, 0
        l32i    a2, a1, 8
        l32i    a3, a1, 12
        l32i    a1, a1, 4
#else
        movi    a0, _ar_registers
        l32i    a1,  a0, 4
        l32i    a2,  a0, 8
        l32i    a3,  a0, 12
        l32i    a0,  a0, 0
#endif
        isync                               /* ensure debug regs have been written */
        rfi     XCHAL_DEBUGLEVEL


## FUNCTIONS ##


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


// User read of INTENABLE (result in a2)
// unsigned _xmon_get_intenable(void)
//

        .global _xmon_get_intenable
        .align  4
_xmon_get_intenable:
        _ENTRY(sp,16)
#if XCHAL_HAVE_INTERRUPTS
        movi    a3, _sr_registers
        rsr     a2, INTENABLE           // current live value
        s32i    a2, a3, INTENABLE<<2    // save most recent value reported to user
#endif
        _RET

// User write of INTENABLE (value in a2)
// void _xmon_set_intenable(unsigned value)
//
// Merge bits user explicitly changed with current value because hi-pri ints
// above debug level might have changed INTENABLE since user`s last access.
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
					addi    a5, a5, 1;			\
					jx      a5;					\

// void _xmon_set_spec_reg(unsigned reg, unsigned value)

// a2 -- register number.
// a4 -- value to write
// a6 -- instruction under construction
// a5 -- pointer to the instruction placeholder

        .align 4
.global _xmon_set_spec_reg
_xmon_set_spec_reg:
        _ENTRY(sp,48)

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
        _ENTRY(sp,48)

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
        _ENTRY(sp,48)

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
        _ENTRY(sp,48)

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
        _ENTRY(sp, 16)
        mov     a4, a2
        movi    a5, _xmon_exec_instr
        jx      a5
_xmon_exec_instr:
        .space 48, 0

