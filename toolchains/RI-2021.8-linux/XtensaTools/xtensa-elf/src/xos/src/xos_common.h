
// xos_common.h - Macros and definitions common to C and assembly code.

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


#ifndef XOS_COMMON_H
#define XOS_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif


#include <xtensa/corebits.h>
#include <xtensa/config/system.h>
#include <xtensa/config/tie.h>

#include "xos_params.h"

//-----------------------------------------------------------------------------
//  Defines for atomic ops support. Must be enabled by user, and requires C11.
//-----------------------------------------------------------------------------
#if !defined(_ASMLANGUAGE) && !defined(__ASSEMBLER__)

#if XOS_OPT_C11_ATOMICS
  #if (defined(__cplusplus) && (__cplusplus >= 201103L)) || (__STDC_VERSION__ >= 201112L)
    #include <stdatomic.h>
    #define XOS_ATOMIC    _Atomic
  #else
    #error Use of atomics requires C11 or newer.
  #endif
#else
    #define XOS_ATOMIC
#endif

#endif


//-----------------------------------------------------------------------------
//  Items related to C library thread safety.
//-----------------------------------------------------------------------------
#if XOS_OPT_THREAD_SAFE_CLIB

#if XSHAL_CLIB == XTHAL_CLIB_XCLIB
  #if !defined(_ASMLANGUAGE) && !defined(__ASSEMBLER__)
  #include <sys/reent.h>
  #endif
  #define CLIB_THREAD_STRUCT      struct _reent xclib_reent
  #define CLIB_TS_PTR             struct _reent *
  #define GLOBAL_CLIB_PTR         _reent_ptr
#elif XSHAL_CLIB == XTHAL_CLIB_NEWLIB
  #if !defined(_ASMLANGUAGE) && !defined(__ASSEMBLER__)
  #include <sys/reent.h>
  #endif
  #define CLIB_THREAD_STRUCT      struct _reent newlib_reent
  #define CLIB_TS_PTR             struct _reent *
  #define GLOBAL_CLIB_PTR         _impure_ptr
#else
  #error The selected C runtime library is not thread safe.
#endif

#endif // XOS_OPT_THREAD_SAFE_CLIB


// This file has many declarations/definitions that are shared between C and assembly,
// so the following MISRA rules are suppressed:
// parasoft-begin-suppress MISRAC2012-RULE_5_5-b-2  "Macro names shared by C and assembly"
// parasoft-begin-suppress MISRAC2012-RULE_20_7-a-2 "Some macro params cannot be parenthesized"
// parasoft-begin-suppress MISRAC2012-DIR_4_9-a-4   "Function-like macros needed for C/assembly"


//-----------------------------------------------------------------------------
//  Macros that help define structures for both C and assembler.
//  These are somewhat different from the XTOS version in xtruntime-frames.h.
//-----------------------------------------------------------------------------
#if defined(_ASMLANGUAGE) || defined(__ASSEMBLER__)

#define STRUCT_BEGIN            .pushsection .text; .struct 0
#define STRUCT_FIELD(ctype,size,asname,name)    asname: .space  size
#define STRUCT_AFIELD(ctype,size,asname,name,n) asname: .space  (size)*(n)
#define STRUCT_END(sname)       sname##Size:; .popsection

#define XOS_GET_CCOUNT(ar)      rsr.ccount ar

#else

#define STRUCT_BEGIN            typedef struct {
#define STRUCT_FIELD(ctype,size,asname,name)    ctype   name;
#define STRUCT_AFIELD(ctype,size,asname,name,n) ctype   name[(n)];
#define STRUCT_END(sname)       } sname;

#endif //_ASMLANGUAGE || __ASSEMBLER__


//-----------------------------------------------------------------------------
//  Macros to abstract the handler pointer register for XEA2 dispatch.
//  The default register used is EXCSAVE1. But when XOS_OPT_SECMON is enabled
//  MISC1 is used instead because EXCSAVE1 is used by secmon when forwarding
//  interrupts/exceptions.
//-----------------------------------------------------------------------------
#if defined(_ASMLANGUAGE) || defined(__ASSEMBLER__)

#if XCHAL_HAVE_XEA2
#if XOS_OPT_SECMON
#define rsr_xhreg(ar)           rsr.misc1 ar
#define wsr_xhreg(ar)           wsr.misc1 ar
#define xsr_xhreg(ar)           xsr.misc1 ar
#else
#define rsr_xhreg(ar)           rsr.excsave1 ar
#define wsr_xhreg(ar)           wsr.excsave1 ar
#define xsr_xhreg(ar)           xsr.excsave1 ar
#endif
#endif

#else

#if XCHAL_HAVE_XEA2
#if XOS_OPT_SECMON
static inline uint32_t
XOS_RSR_XHREG(void)
{
    return XT_RSR_MISC1();
}
static inline void
XOS_WSR_XHREG(uint32_t val)
{
    XT_WSR_MISC1(val);
}
static inline uint32_t
XOS_XSR_XHREG(uint32_t val)
{
    XT_XSR_MISC1(val);
    return val;
}
#else
static inline uint32_t
XOS_RSR_XHREG(void)
{
    return XT_RSR_EXCSAVE1();
}
static inline void
XOS_WSR_XHREG(uint32_t val)
{
    XT_WSR_EXCSAVE1(val);
}
static inline uint32_t
XOS_XSR_XHREG(uint32_t val)
{
    XT_XSR_EXCSAVE1(val);
    return val;
}
#endif
#endif

#endif //_ASMLANGUAGE || __ASSEMBLER__


//-----------------------------------------------------------------------------
//  Offsets relative to xos_globals.
//-----------------------------------------------------------------------------
#define XOS_INTLEVEL_MASK       0       // offset to the level mask
#define XOS_INTENABLE_MASK      4       // offset to the enable mask
#define XOS_CURR_THREADPTR      8       // offset to the current thread ptr
#define XOS_NEXT_THREADPTR      12      // offset to the next thread ptr
#define XOS_INTERRUPT_TABLE     16      // offset to the interrupt table


#if XCHAL_HAVE_XEA2

//-----------------------------------------------------------------------------
//  Offsets for xos_interrupt_table[] entries.
//-----------------------------------------------------------------------------
#define XOS_INTTAB_HANDLER      (XOS_INTERRUPT_TABLE+0)  // ofs to interrupt handler
#define XOS_INTTAB_ARG          (XOS_INTERRUPT_TABLE+4)  // ofs to interrupt handler arg
#define XOS_INTTAB_PS           (XOS_INTERRUPT_TABLE+8)  // (hwpri) PS for interrupt level
#define XOS_INTTAB_LEVEL        (XOS_INTERRUPT_TABLE+8)  // (swpri) interrupt level (1..7)
#define XOS_INTTAB_PRI          (XOS_INTERRUPT_TABLE+9)  // (swpri) interrupt priority (0..255)
#define XOS_INTTAB_PRIMASK      (XOS_INTERRUPT_TABLE+12) // (swpri) mask of higher pri. interrupts


//-----------------------------------------------------------------------------
//  Exception/interrupt stack frame layout for a pre-empted thread
//  tcb->resume_fn == &xos_resume_preempted_thread).
//  Pointed to by thread->esf. Located just below thread's current stack ptr.
//  Thread's a1 == thread->esf + XosExcFrameSize.
//  NOTE: exception frame size is a multiple of 16.
//-----------------------------------------------------------------------------
STRUCT_BEGIN
STRUCT_AFIELD(uint32_t,4,FRAME_AREG,areg, 12)       // a4-a15 (offsets 0 thru 44)
                                                    // (a1 is computed, a0,a2-a3 are in s32e range of a1)
// Regardless of XCHAL_HAVE_LOOPS, to keep frame size unchanged
STRUCT_FIELD (uint32_t,4,FRAME_LBEG,lbeg)
STRUCT_FIELD (uint32_t,4,FRAME_LEND,lend)
STRUCT_FIELD (uint32_t,4,FRAME_LCOUNT,lcount)
// Regardless of XCHAL_HAVE_MAC16, to keep frame size unchanged
STRUCT_FIELD (uint32_t,4,FRAME_ACCLO,acclo)
STRUCT_FIELD (uint16_t,2,FRAME_ACCHI,acchi)
STRUCT_FIELD (uint8_t, 1,FRAME_SAR,sar)
STRUCT_FIELD (uint8_t, 1,FRAME_PAD0,pad0)           // unused
STRUCT_FIELD (uint32_t,4,FRAME_EXCCAUSE,exccause)
STRUCT_FIELD (uint32_t,4,FRAME_EXCVADDR,excvaddr)
STRUCT_FIELD (uint32_t,4,FRAME_PAD1,pad1)           // unused -- pad to make multiple of 16 bytes
// Regardless of XCHAL_HAVE_EXCLUSIVE, to keep frame size unchanged
STRUCT_FIELD (uint32_t,4,FRAME_ATOMCTL,atomctl)
STRUCT_FIELD (uint32_t,4,FRAME_PS,ps)               // (XOS_FRAME_SIZE-44) in S32E range of end
STRUCT_FIELD (uint32_t,4,FRAME_PC,pc)               // (XOS_FRAME_SIZE-40) in S32E range of end
STRUCT_FIELD (uint32_t,4,FRAME_A0,a0)
STRUCT_FIELD (uint32_t,4,FRAME_A2,a2)               // (XOS_FRAME_SIZE-32) in S32E range of end
STRUCT_FIELD (uint32_t,4,FRAME_A3,a3)               // (XOS_FRAME_SIZE-28) in S32E range of end
STRUCT_FIELD (uint32_t,4,FRAME_LEVELMASK,levelmask) //
STRUCT_FIELD (uint32_t,4,FRAME_NESTCHAIN,nestchain) // nested C function call chain ptr
// Caller's a0-a3 save area below SP. These fields MUST be the last ones in the
// struct so that they are guaranteed to be just under the original SP (before
// we allocate the exception frame).
STRUCT_AFIELD (uint32_t,4,FRAME_CWINSAVE,cwinsave, 4)  // (XOS_FRAME_SIZE-16)
STRUCT_END(XosExcFrame)

#define FRAME_AR(x)         (FRAME_AREG + (x)*4 - 16)


//-----------------------------------------------------------------------------
//  Stack frame layout for a cooperatively switched out thread
//  (tcb->resume_fn == &xos_resume_cooperative_thread).
//  Pointed to by thread->esf.  This is a function frame.
//  Thread's a1 == thread->esf.
//-----------------------------------------------------------------------------
STRUCT_BEGIN
STRUCT_FIELD (uint32_t,4,CFRAME_A0,a0)              // return PC
STRUCT_FIELD (uint32_t,4,CFRAME_LEVELMASK,levelmask)
STRUCT_FIELD (uint32_t,4,CFRAME_PS,ps)
#ifdef __XTENSA_CALL0_ABI__
STRUCT_FIELD (uint32_t,4,CFRAME_PAD0,pad0)
STRUCT_AFIELD(uint32_t,4,CFRAME_AREG,areg,4)        // callee-saved regs a12-a15
#endif
STRUCT_END(XosCoopFrame)

#endif // XCHAL_HAVE_XEA2

#if XCHAL_HAVE_XEA3

STRUCT_BEGIN 
STRUCT_FIELD (uint32_t, 4, XT_STK_ATOMCTL,  atomctl)
STRUCT_FIELD (uint32_t, 4, XT_STK_LCOUNT,   lcount)
STRUCT_FIELD (uint32_t, 4, XT_STK_LEND,     lend)
STRUCT_FIELD (uint32_t, 4, XT_STK_LBEG,     lbeg)
STRUCT_FIELD (uint32_t, 4, XT_STK_PC,       pc)
STRUCT_FIELD (uint32_t, 4, XT_STK_PS,       ps)
STRUCT_FIELD (uint32_t, 4, XT_STK_EXCCAUSE, exccause)
STRUCT_FIELD (uint32_t, 4, XT_STK_EXCVADDR, excvaddr)
#ifdef __XTENSA_CALL0_ABI__
STRUCT_FIELD (uint32_t, 4, XT_STK_A0,     a0)
STRUCT_FIELD (uint32_t, 4, XT_STK_A1,     a1)
STRUCT_FIELD (uint32_t, 4, XT_STK_A2,     a2)
STRUCT_FIELD (uint32_t, 4, XT_STK_A3,     a3)
STRUCT_FIELD (uint32_t, 4, XT_STK_A4,     a4)
STRUCT_FIELD (uint32_t, 4, XT_STK_A5,     a5)
STRUCT_FIELD (uint32_t, 4, XT_STK_A6,     a6)
STRUCT_FIELD (uint32_t, 4, XT_STK_A7,     a7)
#endif
STRUCT_FIELD (uint32_t, 4, XT_STK_A8,     a8)
STRUCT_FIELD (uint32_t, 4, XT_STK_A9,     a9)
STRUCT_FIELD (uint32_t, 4, XT_STK_A10,    a10)
STRUCT_FIELD (uint32_t, 4, XT_STK_A11,    a11)
STRUCT_FIELD (uint32_t, 4, XT_STK_A12,    a12)
STRUCT_FIELD (uint32_t, 4, XT_STK_A13,    a13)
STRUCT_FIELD (uint32_t, 4, XT_STK_A14,    a14)
STRUCT_FIELD (uint32_t, 4, XT_STK_A15,    a15)
#ifdef __XTENSA_WINDOWED_ABI__
STRUCT_AFIELD(uint32_t, 4, XT_STK_SPILL,  spill, 8)
#endif
STRUCT_END(XosExcFrame)

#endif // XCHAL_HAVE_XEA3


#if defined(_ASMLANGUAGE) || defined(__ASSEMBLER__)
#define XOS_FRAME_SIZE      XosExcFrameSize
#else
#define XOS_FRAME_SIZE      sizeof(XosExcFrame)
#endif


//-----------------------------------------------------------------------------
//  Offsets into thread control block (must match xos_thread.h !!)
//-----------------------------------------------------------------------------
#define TCB_RESUME_FN           12      // ptr to thread resume asm sequence
#define TCB_STACK_ESF           16      // saved stack ptr (actually, ptr to ESF)
#define TCB_TIE_SAVE            20      // ptr to TIE save area
#define TCB_RETVALUE            24      // ptr to xos_block return value
#define TCB_STACK_BASE          32      // ptr to base of stack (lower bound)
#define TCB_STACK_END           36      // ptr to end of stack (thread's initial stack ptr)
#define TCB_STARTUP_ENTRY       40      // ptr to thread entry function
#define TCB_STARTUP_ARG         44      // ptr to thread entry function's arg
#define TCB_READY               48      // thread ready state (1 byte)
#define TCB_EXIT_FLAG           49      // thread in_exit flag (1 byte)
#define TCB_FLAGS               52      // thread flags (4 bytes)
#define TCB_UT_PTR              64      // back ptr to usermode TCB
#define TCB_CLIB_PTR            100     // thread C lib context pointer

#define TCB_RESUME_CCOUNT       108     // cycle count at last resume
#define TCB_CYCLE_COUNT         112     // number of cycles consumed
#define TCB_NORMAL_RESUMES      120     // number of cooperative/restart thread resumes
#define TCB_PREEMPT_RESUMES     124     // number of pre-emptive thread resumes


//-----------------------------------------------------------------------------
//  Coprocessor state handling:
//  The coprocessor state save area is allocated on the thread stack. The stack
//  must be sized appropriately. Threads that do not use coprocessors need not
//  allocate the storage area.
//
//  Along with the save area for each coprocessor, two bitmasks with flags per
//  coprocessor (laid out as in the CPENABLE reg) help manage context switching
//  coprocessors as efficiently as possible:
//
//  XT_CPENABLE
//    The contents of a non-running thread's CPENABLE register.
//    It represents the coprocessors owned (and whose state is still needed)
//    by the thread. When a thread is preempted, its CPENABLE is saved here.
//    When a thread solicits a context switch, its CPENABLE is cleared - the
//    compiler has saved the (caller-saved) coprocessor state if needed.
//    When a non-running thread loses ownership of a CP, its bit is cleared.
//    When a thread runs, it's XT_CPENABLE is loaded into the CPENABLE reg.
//    Avoids coprocessor exceptions when no change of ownership is needed.
//
//  XT_CPSTORED
//    A bitmask with the same layout as CPENABLE, a bit per coprocessor.
//    Indicates whether the state of each coprocessor is saved in the state 
//    save area. When a thread enters the kernel, only the state of coprocs
//    still enabled in CPENABLE is saved. When the coprocessor exception 
//    handler assigns ownership of a coprocessor to a thread, it restores 
//    the saved state only if this bit is set, and clears this bit.
//
//  XT_CP_CS_ST
//    A bitmask with the same layout as CPENABLE, a bit per co-processor.
//    Indicates whether callee-saved state is saved in the state save area.
//    Callee-saved state is saved by itself on a solicited context switch,
//    and restored when needed by the coprocessor exception handler.
//    Unsolicited switches will cause the entire coprocessor to be saved
//    when necessary.
//
//  XT_NCP_ASA
//    Pointer to aligned save area for non-CP state. This is always filled
//    in, even if there is no non-CP state to be saved. If there is no state
//    to be saved then no space is actually allocated and this pointer is
//    not used.
//
//  XT_CP_ASA
//    Pointer to aligned save area for coprocessor state. This is filled in
//    only if coprocessor state is to be saved for the thread. Allows it to be
//    aligned more than the overall save area (which might be stack-aligned
//    or TCB-aligned). Especially relevant for Xtensa cores configured with a
//    very large data path that requires alignment greater than 16 bytes (ABI
//    stack alignment).
//-----------------------------------------------------------------------------

#define ALIGNUP(n, val) (((val) + ((n) - 1)) & -(n))

//  Offsets of each coprocessor save area within the 'aligned save area'.
//  The non-CP TIE state save area is at offset 0, so that it does not
//  move around if some or all coprocessors are not to be saved.

#define XT_NCP_SA       0
#define XT_CP0_SA       ALIGNUP(XCHAL_CP0_SA_ALIGN, XT_NCP_SA + XCHAL_NCP_SA_SIZE)
#define XT_CP1_SA       ALIGNUP(XCHAL_CP1_SA_ALIGN, XT_CP0_SA + XCHAL_CP0_SA_SIZE)
#define XT_CP2_SA       ALIGNUP(XCHAL_CP2_SA_ALIGN, XT_CP1_SA + XCHAL_CP1_SA_SIZE)
#define XT_CP3_SA       ALIGNUP(XCHAL_CP3_SA_ALIGN, XT_CP2_SA + XCHAL_CP2_SA_SIZE)
#define XT_CP4_SA       ALIGNUP(XCHAL_CP4_SA_ALIGN, XT_CP3_SA + XCHAL_CP3_SA_SIZE)
#define XT_CP5_SA       ALIGNUP(XCHAL_CP5_SA_ALIGN, XT_CP4_SA + XCHAL_CP4_SA_SIZE)
#define XT_CP6_SA       ALIGNUP(XCHAL_CP6_SA_ALIGN, XT_CP5_SA + XCHAL_CP5_SA_SIZE)
#define XT_CP7_SA       ALIGNUP(XCHAL_CP7_SA_ALIGN, XT_CP6_SA + XCHAL_CP6_SA_SIZE)

#define XT_TOT_SA_SIZE  ALIGNUP(16, XT_CP7_SA + XCHAL_CP7_SA_SIZE)
#define XT_NCP_SA_SIZE  XCHAL_NCP_SA_SIZE

//  Offsets within the overall save area

#define XT_CPENABLE     0   // (2 bytes) coprocessors active for this thread
#define XT_CPSTORED     2   // (2 bytes) coprocessors saved for this thread
#define XT_CP_CS_ST     4   // (2 bytes) coprocessor callee-saved regs for this thread
#define XT_NCP_ASA      8   // (4 bytes) ptr to aligned save area for nonCP state
#define XT_CP_ASA      12   // (4 bytes) ptr to aligned save area for CP state

//  Overall size allows for dynamic alignment, make sure multiple of 4 bytes.
//    XT_CP_SIZE  - total space needed for all coprocessors + nonCP state + hdr
//    XT_NCP_SIZE - total space needed for nonCP state + hdr

#define XT_CP_SIZE      ALIGNUP(4, (16 + XT_TOT_SA_SIZE + XCHAL_TOTAL_SA_ALIGN))
#define XT_NCP_SIZE     ALIGNUP(4, (16 + XT_NCP_SA_SIZE + XCHAL_TOTAL_SA_ALIGN))


//-----------------------------------------------------------------------------
//  Stack size computation.
//
//  XOS_STACK_MIN_SIZE
//    The minimum recommended stack size for any XOS thread. If you want to
//    use a stack size smaller than this, you will have to verify that the
//    smaller size will work under all operating conditions.
//
//  XOS_STACK_MIN_SIZE_NO_CP
//    The minimum recommended atack size for threads that will not use any
//    coprocessor resources. No coprocessor state will be saved/restored
//    for these threads. Non-CP TIE state will still be saved/restored.
//    These threads must be created with the flag XOS_THREAD_NO_CP.
//
//  XOS_STACK_EXTRA
//    The amount of stack space used by the system to:
//      - save coprocessor state
//      - save non-coprocessor TIE state
//      - allocate an interrupt/exception frame
//
//  XOS_STACK_EXTRA_NO_CP
//    The amount of stack space used by the system to:
//      - save non-coprocessor TIE state
//      - allocate an interrupt/exception frame
//-----------------------------------------------------------------------------

#define XOS_STACK_EXTRA             (XOS_FRAME_SIZE + XT_CP_SIZE)
#define XOS_STACK_EXTRA_NO_CP       (XOS_FRAME_SIZE + XT_NCP_SIZE)

#ifdef __XTENSA_CALL0_ABI__
#define XOS_STACK_MIN_SIZE          (XOS_STACK_EXTRA + 0x180)
#define XOS_STACK_MIN_SIZE_NO_CP    (XOS_STACK_EXTRA_NO_CP + 0x180)
#else
#define XOS_STACK_MIN_SIZE          (XOS_STACK_EXTRA + 0x200)
#define XOS_STACK_MIN_SIZE_NO_CP    (XOS_STACK_EXTRA_NO_CP + 0x200)
#endif

//-----------------------------------------------------------------------------
//  Minimum interrupt stack size. This is a lower bound estimate of the minimum
//  size required for safe operation. In real applications this number is very
//  likely to be too low, more so if timer callbacks are processed in the timer
//  interrupt context.
//  IMPORTANT: Please note that this number is NOT the default interrupt stack
//  size. The default size is determined by the parameter XOS_INT_STACK_SIZE,
//  as specified in xos_params.h. XOS_INT_STACK_MIN_SIZE is just intended to
//  be a guide to help you select the appropriate size for your application.
//-----------------------------------------------------------------------------

#if XCHAL_HAVE_XEA3
#define XOS_INT_STACK_MIN_SIZE      ((XCHAL_NUM_INTLEVELS - 1) * (XOS_FRAME_SIZE + 0x20))
#else
#define XOS_INT_STACK_MIN_SIZE      ((XCHAL_EXCM_LEVEL - 1) * (XOS_FRAME_SIZE + 0x20))
#endif


//-----------------------------------------------------------------------------
//  Check pattern used for stack checking.
//-----------------------------------------------------------------------------
#define XOS_STACK_CHECKVAL          0x55555555U


//-----------------------------------------------------------------------------
//  Check (MAX_OS_INTLEVEL,EXCM_LEVEL)
//-----------------------------------------------------------------------------
#if XOS_MAX_OS_INTLEVEL < XCHAL_EXCM_LEVEL
#error "XOS_MAX_OS_INTLEVEL is set below XCHAL_EXCM_LEVEL. This is not supported."
#endif


//-----------------------------------------------------------------------------
// Define whether to use a sw interrupt for scheduling.
//-----------------------------------------------------------------------------
#if XCHAL_HAVE_XEA3 && defined (__XTENSA_WINDOWED_ABI__)
#define XOS_USE_SCHED_INT       1
#else
#define XOS_USE_SCHED_INT       0
#endif


//-----------------------------------------------------------------------------
//  Define a debug exception cause for XEA2. This is needed to reserve an entry
//  in the exception table for a user-defined handler, and also for registering
//  a user handler.
//-----------------------------------------------------------------------------
#ifndef EXCCAUSE_DEBUG
#define EXCCAUSE_DEBUG          21
#endif


//-----------------------------------------------------------------------------
//  XOS config signature. Encodes build options.
//-----------------------------------------------------------------------------
#define XOS_CONFIG_SIG ( \
    (((uint32_t)XOS_DEBUG_ALL)                << 0)  | \
    (((uint32_t)XOS_DEBUG)                    << 1)  | \
    (((uint32_t)XOS_OPT_STATS)                << 2)  | \
    (((uint32_t)XOS_OPT_STACK_CHECK)          << 3)  | \
    (((uint32_t)XOS_OPT_INTERRUPT_SWPRI)      << 4)  | \
    (((uint32_t)XOS_OPT_THREAD_SAFE_CLIB)     << 5)  | \
    (((uint32_t)XOS_OPT_WAIT_TIMEOUT)         << 6)  | \
    (((uint32_t)XOS_OPT_TIMER_WAIT)           << 7)  | \
    (((uint32_t)XOS_OPT_TIMER_THREAD)         << 8)  | \
    (((uint32_t)XOS_OPT_TIME_SLICE)           << 9)  | \
    (((uint32_t)XOS_OPT_MUTEX_PRIORITY)       << 10) | \
    (((uint32_t)XOS_OPT_LOG_SYSEVENT)         << 11) | \
    (((uint32_t)XOS_OPT_C11_ATOMICS)          << 12) | \
    (((uint32_t)XOS_OPT_THREAD_LOCAL_STORAGE) << 13))


//-----------------------------------------------------------------------------
//  Xtensa tools version.
//-----------------------------------------------------------------------------
#if defined(__XCC__) && defined(__XCC_MINOR__)
#define XTTOOLS_VERSION         (__XCC__ + __XCC_MINOR__)
#else
#define XTTOOLS_VERSION         (0)
#endif


//-----------------------------------------------------------------------------
//  Erratum workarounds.
//-----------------------------------------------------------------------------

// Erratum 487 fix is available in version RF.3 onwards and RG.2 onwards.
#if ((XTTOOLS_VERSION >= 11003) && (XTTOOLS_VERSION < 12000)) || (XTTOOLS_VERSION >= 12002) || defined(__clang__)
#define HWERR_487_FIX           hw_erratum_487_fix
#else
#define HWERR_487_FIX
#endif


#ifdef __ASSEMBLER__
//-----------------------------------------------------------------------------
// Low and high word offsets for 64-bit values, endian dependent.
//-----------------------------------------------------------------------------

#if XCHAL_HAVE_BE
#define HW_OFFSET       0
#define LW_OFFSET       4
#else
#define HW_OFFSET       4
#define LW_OFFSET       0
#endif


//-----------------------------------------------------------------------------
// Macro to update thread cycle count (when thread gets switched out).
// The cycle count is a 64-bit value (TCB->cycle_count). The macro checks for
// 32-bit overflow and updates the top 32 bits accordingly.
//
// Input: "ar1" -- should be TCB pointer, preserved.
//        "ar2" and "ar3" are scratch and will be trashed.
//        "ar4" -- if specified, the cycle counter value will be returned here.
//-----------------------------------------------------------------------------
  
        .macro  update_cycle_count      ar1 ar2 ar3 ar4=
  
#if XCHAL_HAVE_CCOUNT
        l32i    \ar2, \ar1, TCB_RESUME_CCOUNT           // load saved ccount
        XOS_GET_CCOUNT(\ar3)
  .ifnb \ar4
        mov     \ar4, \ar3
  .endif
        sub     \ar3, \ar3, \ar2                        // compute diff from current
        l32i    \ar2, \ar1, TCB_CYCLE_COUNT + LW_OFFSET // load old count (low word)
        add     \ar3, \ar2, \ar3                        // add delta
        s32i    \ar3, \ar1, TCB_CYCLE_COUNT + LW_OFFSET // store low word
        bge     \ar3, \ar2, 8f                          // no overflow, done
        l32i    \ar2, \ar1, TCB_CYCLE_COUNT + HW_OFFSET // load high word
        addi    \ar2, \ar2, 1                           // overflow, increment high word
        s32i    \ar2, \ar1, TCB_CYCLE_COUNT + HW_OFFSET // store high word
8:
#endif

        .endm

#endif // __ASSEMBLER__


#if XCHAL_HAVE_XEA3

//-----------------------------------------------------------------------------
//  Entry and exit macros for interrupt / exception dispatch processing. These
//  are used by the XEA3 dispatch code, see xos_vectors_xea3.S.
//-----------------------------------------------------------------------------

#ifdef __ASSEMBLER__

        // Various extern variables and labels used in dispatch/handler code.
        // xos_globals
        // xos_idle
        // xos_inth_start_ccount
        // xos_inth_cycle_count
        // _xt_interrupt_stack_top

        // RTOS-specific entry macro.
        // Use only a8, a12-a14.

        .macro  XT_RTOS_INT_ENTER

#if XOS_OPT_STATS
#if XCHAL_HAVE_CCOUNT
        rsr.ps   a8
        extui    a8,  a8, PS_STACK_SHIFT, 3     // a8 <- PS.STACK
        beqz     a8, 1f                         // PS.STACK = 0, nested intr, skip stats
        movi    a12, xos_globals
        l32i    a12, a12, XOS_CURR_THREADPTR    // a12 <- current thread pointer
        movi    a13, 0xFFFFFFFF
        beq     a12, a13, 1f
        beqz    a12, 1f
        update_cycle_count a12 a13 a14 a8       // stop counting cycles for thread
        movi    a13, xos_inth_start_ccount      // start counting cycles for interrupt
        s32i     a8, a13, 0                     // a8 is CCOUNT from update macro
#endif
#endif

1:

        .endm

        // RTOS-specific exit macro. Use only a8-a14.
        // (In call0, a15 holds user SP, must be preserved)

        .macro XT_RTOS_INT_EXIT

        // Skip lots of stuff if nested.

        rsr.ps  a10
        extui   a10, a10, PS_STACK_SHIFT, 3     // a10 <- PS.STACK
        beqz    a10, .Lnested                   // PS.STACK = 0, nested intr

#if XOS_OPT_STATS && XCHAL_HAVE_CCOUNT
        // Stop counting cycles for interrupt/exception handling.
        movi    a13, xos_inth_start_ccount      // a13 = &xos_inth_start_ccount
        movi    a12, xos_inth_cycle_count       // a12 = &xos_inth_cycle_count
        l32i    a13, a13, 0                     // load last ccount
        XOS_GET_CCOUNT(a9)                      // get current ccount
        sub     a10,  a9, a13                   // a10 = cycles spent
        l32i    a13, a12, LW_OFFSET             // a13 = low word of cycle count
        add     a10, a10, a13
        s32i    a10, a12, LW_OFFSET             // store low word
        bge     a10, a13, 8f                    // no overflow, done
        l32i    a13, a12, HW_OFFSET             // a13 = high word of cycle count
        addi    a13, a13, 1                     // overflow, increment high word
        s32i    a13, a12, HW_OFFSET             // store high word
8:
        // NOTE a9 is preserved and carried forward to start thread count
#endif

        movi    a11, xos_globals
        movi    a13, xos_idle_thread            // a13 <- idle thread ptr
        l32i    a12, a11, XOS_CURR_THREADPTR    // a12 <- curr thread ptr
        l32i    a14, a11, XOS_NEXT_THREADPTR    // a14 <- next thread ptr
        beqz    a12, .Lsched                    // no curr thread, nothing to save
        beq     a12, a13, .Lsched               // curr == idle, nothing to save
        beq     a12, a14, .Lrestore             // no switch, go to restore

        // Context switch required, save remaining state of outgoing thread

        addi     a1,  a1, -XOS_FRAME_SIZE
        s32i     a1, a12, TCB_STACK_ESF         // save stack frame ptr in TCB

#if XCHAL_HAVE_EXCLUSIVE
        movi    a10, 0
        getex   a10                             // a10 <- ATOMCTL[8], ATOMCTL[8] = 0
        s32i    a10,  a1, XT_STK_ATOMCTL        // save in stack frame
#endif

#if XCHAL_CP_NUM > 0
        //  Save coprocessor enable state and disable all coprocessors.

        l32i    a10, a12, TCB_TIE_SAVE          // a10 <- ptr to TIE save area
        rsr.cpenable a13                        // read CPENABLE
        beqz    a10, 2f                         // skip if NULL
        s16i    a13, a10, XT_CPENABLE           // save CPENABLE state
2:
        movi    a13, 0                          // disable all coprocessors
        wsr.cpenable a13
#endif

        // If needed, save non-CP TIE state.

#if XCHAL_EXTRA_SA_SIZE > 0
        l32i    a13, a12, TCB_TIE_SAVE          // a13 <- ptr to TIE save area
        beqz    a13, .Lskip_ncps
        l32i    a13, a13, XT_NCP_ASA            // a13 <- ptr to aligned save area
        addi    a13, a13, XT_NCP_SA             // offset to non-CP TIE save area
        xchal_ncp_store a13, a8, a11, a10, a12  // save non-coprocessor state
        movi    a11, xos_globals                // reload for subsequent use
.Lskip_ncps:
#endif

        // If windowed ABI, a0-a7 was not saved by dispatch code.
        // These don't go into the exception frame, but below the
        // exception frame pointer.

#ifdef __XTENSA_WINDOWED_ABI__
        addi     a8,  a1, -32
        s32i     a0,  a8, 0
        s32i     a2,  a8, 8
        s32i     a3,  a8, 12
        s32i     a4,  a8, 16
        s32i     a5,  a8, 20
        s32i     a6,  a8, 24
        s32i     a7,  a8, 28
#endif

        // Start restoring new thread. When we get here we should have:
        // a11 = xos_globals, a14 = next thread ptr

.Lsched:
        movi    a12, xos_idle_thread            // a12 <- idle thread ptr
        l32i    a13, a14, TCB_RESUME_FN         // a13 <- resume function
        s32i    a14, a11, XOS_CURR_THREADPTR    // curr_thread = next_thread
        beq     a14, a12, .Lresume_idle         // take shortcut if going idle
        beqz    a13, .Lresume_preempted         // resume preempted thread

        mov      a2, a14                        // a2 <- incoming thread ptr
        mov      a3, a9                         // a3 <- saved CCOUNT value
        movi    a12, PS_DI
        xps     a12, a12                        // disable interrupts
        movi    a12, 0
        wsr.ms  a12                             // DISPST = 0
        rsync
        jx      a13                             // resume yielded thread

.Lresume_idle:
        movi    a12, xos_idle
        jx      a12

.Lresume_preempted:                             // Resuming preempted thread
        l32i     a1, a14, TCB_STACK_ESF         // a1 = new thread's SP

#if XOS_OPT_STATS
        l32i    a12, a14, TCB_PREEMPT_RESUMES
        addi    a12, a12, 1
        s32i    a12, a14, TCB_PREEMPT_RESUMES   // increment number of preempt resumes
#endif

#if XOS_OPT_THREAD_SAFE_CLIB
        // Restore thread's C library context pointer
        l32i    a13, a14, TCB_CLIB_PTR
        movi    a12, GLOBAL_CLIB_PTR
        s32i    a13, a12, 0
#endif

#if XOS_OPT_STACK_CHECK && XCHAL_HAVE_KSL
        // Set up thread's stack limit 128 bytes above the stack base
        l32i    a13, a14, TCB_STACK_BASE        // base at least 16-byte aligned
        movi    a12, 128
        add     a13, a13, a12
        wsr.ksl a13
#endif

#if XCHAL_EXTRA_SA_SIZE > 0
        // If needed, restore non-CP TIE state
        l32i    a12, a14, TCB_TIE_SAVE          // a12 <- ptr to TIE save area
        beqz    a12, .Lskip_ncpr
        l16ui   a10, a12, XT_CPENABLE           // a10 <- saved CPENABLE state or 0
        l32i    a12, a12, XT_NCP_ASA            // a12 <- ptr to aligned save area
        addi    a12, a12, XT_NCP_SA             // offset to non-CP TIE save area
#if XCHAL_CP_NUM > 0
        wsr.cpenable a10                        // restore CPENABLE
#endif
        xchal_ncp_load a12, a8, a10, a11, a13
.Lskip_ncpr:
#endif

#ifdef __XTENSA_WINDOWED_ABI__
        // Restore a0-a7 for windowed ABI
        addi    a8, a1, -32
        l32i    a0, a8, 0
        l32i    a2, a8, 8
        l32i    a3, a8, 12
        l32i    a4, a8, 16
        l32i    a5, a8, 20
        l32i    a6, a8, 24
        l32i    a7, a8, 28
#else
        // For call0, dispatch exit code expects a15 = original SP
        addi    a15, a1, XOS_FRAME_SIZE
#endif

#if XCHAL_HAVE_EXCLUSIVE
        l32i    a10, a1, XT_STK_ATOMCTL
        getex   a10                             // restore ATOMCTL[8]
#endif

        addi    a1, a1, XOS_FRAME_SIZE          // set up a1 for dispatch exit

.Lrestore:
#if XOS_OPT_STATS && XCHAL_HAVE_CCOUNT
        // CCOUNT value carried forward in a9
        s32i     a9, a14, TCB_RESUME_CCOUNT     // resume thread count
#endif

.Lnested:
        // Return to current saved context.

        .endm

#endif

#endif // XCHAL_HAVE_XEA3


// parasoft-end-suppress MISRAC2012-RULE_5_5-b-2
// parasoft-end-suppress MISRAC2012-RULE_20_7-a-2
// parasoft-end-suppress MISRAC2012-DIR_4_9-a-4


#ifdef __cplusplus
}
#endif

#endif  // XOS_COMMON_H

