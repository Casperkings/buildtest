// Copyright (c) 2015-2021 Cadence Design Systems, Inc.
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

// xtsm_handlers.c -- Secure monitor handler registration APIs


#include <xtensa/xtruntime.h>
#include <errno.h>

#include "xtos-internal.h"

#include "xtensa/secmon.h"
#include "xtensa/secmon-common.h"
#include "xtensa/secmon-defs.h"


//-----------------------------------------------------------------------------
//  XTSM dispatch table entry type
//-----------------------------------------------------------------------------
typedef void (*xtsm_dispatch_fn_t)(void);

//-----------------------------------------------------------------------------
//  Nonsecure "virtual vector" for handling L1 interrupts and exceptions.
//
//  If the application provides its own handling, a replacement for this hook
//  must be provided here, replacing lib/user-vector-nsm.S.
//-----------------------------------------------------------------------------
extern void xtsmUserExceptionVector(void);


//-----------------------------------------------------------------------------
//  Nonsecure "virtual vector" for handling debug interrupts.
//-----------------------------------------------------------------------------
extern void xtsmDebugExceptionVector(void);


//-----------------------------------------------------------------------------
//  Pointer to nonsecure exception handler table
//-----------------------------------------------------------------------------
extern uint32_t xtos_exc_handler_table[];


//-----------------------------------------------------------------------------
//  XTOS default exception handlers
//-----------------------------------------------------------------------------
extern void _xtos_alloca_handler(void *);
extern void _xtos_syscall_handler(void *);


//-----------------------------------------------------------------------------
//  Helper function to set exception handler (implemented in assembly)
//-----------------------------------------------------------------------------
static int32_t xtsm_set_exception_handler_asm(uint32_t excnum, xtsm_handler_t handler);


//-----------------------------------------------------------------------------
//  Nonsecure "virtual vectors" for handling medium-priority interrupts.
//
//  If the application provides its own handling, a replacement for these hooks
//  must be provided here, replacing lib/int-medpri-handlers-nsm.S
//-----------------------------------------------------------------------------
#if (XCHAL_EXCM_LEVEL >= 2)
extern void xtsmLevel2Vector(void);
#endif
#if (XCHAL_EXCM_LEVEL >= 3)
extern void xtsmLevel3Vector(void);
#endif
#if (XCHAL_EXCM_LEVEL >= 4)
extern void xtsmLevel4Vector(void);
#endif
#if (XCHAL_EXCM_LEVEL >= 5)
extern void xtsmLevel5Vector(void);
#endif
#if (XCHAL_EXCM_LEVEL >= 6)
extern void xtsmLevel6Vector(void);
#endif

//-----------------------------------------------------------------------------
//  XTSM dispatch table
//  Only vectors <= EXCM_LEVEL are routable to nonsecure space.
//  Indexed by [INTLEVEL - 1].
//-----------------------------------------------------------------------------
static const xtsm_dispatch_fn_t xtsm_dispatch_table[XCHAL_EXCM_LEVEL] = {
    xtsmUserExceptionVector,
#if (XCHAL_EXCM_LEVEL >= 2)
    xtsmLevel2Vector,
#endif
#if (XCHAL_EXCM_LEVEL >= 3)
    xtsmLevel3Vector,
#endif
#if (XCHAL_EXCM_LEVEL >= 4)
    xtsmLevel4Vector,
#endif
#if (XCHAL_EXCM_LEVEL >= 5)
    xtsmLevel5Vector,
#endif
#if (XCHAL_EXCM_LEVEL >= 6)
    xtsmLevel6Vector,
#endif
};


//-----------------------------------------------------------------------------
//  Helper macro to register nonsecure "virtual" exception vector with 
//  secure monitor.  Parameters must be register variables; "ret" is an output.
//-----------------------------------------------------------------------------
#define ISSUE_SYSCALL(ret, syscall_id, p1, p2)  do {    \
    __asm__ __volatile__ ( "                            \
        mov a4, %3 \n                                   \
        mov a3, %2 \n                                   \
        mov a2, %1 \n                                   \
        syscall    \n                                   \
        mov %0, a2 \n "                                 \
        : "=a" (ret)                                    \
        : "a" (syscall_id), "a" (p1), "a" (p2)          \
        : "a2", "a3", "a4"  /* clobber list */          \
        );                                              \
    } while (0)


//-----------------------------------------------------------------------------
//  Initialize XTSM debug state.  Must be called prior to hitting first 
//  debug interrupt.
//-----------------------------------------------------------------------------
static int32_t
xtsm_init_debug(void)
{
    register int32_t syscall_id = SECMON_SYSCALL_SET_DBG_HNDLR;
    register xtsm_dispatch_fn_t nsm_dispatch = xtsmDebugExceptionVector;
    uint32_t handler_addr = (uint32_t)nsm_dispatch;
    register uint32_t unused = 0;
    register int32_t ret;

    if (xtsm_secure_addr_overlap(handler_addr, handler_addr + 4)) {
        return -EINVAL;
    }

    ISSUE_SYSCALL(ret, syscall_id, unused, nsm_dispatch);
    return ret;
}


//-----------------------------------------------------------------------------
//  Initialize XTSM library state; install default alloca() handler and 
//  syscall(0) (register spill) handler if windowed register option is enabled.
//-----------------------------------------------------------------------------
int32_t
xtsm_init(void)
{
    static int32_t xtsm_init_debug_done = 0;
    int32_t ret = 0;

    // Init debug once, which becomes a no-op if XMON is not detected
    if (!xtsm_init_debug_done) {
        ret = xtsm_init_debug();
        xtsm_init_debug_done = 1;
    }

#if XTOS_VIRTUAL_INTENABLE
    // Pick up any interrupts enabled by the monitor so XTOS
    // doesn't inadvertently disable them
    extern uint32_t xtos_enabled;
    xtos_enabled = XT_RSR_INTENABLE();
#endif

#ifdef __XTENSA_WINDOWED_ABI__
    int32_t ret_alloca = xtsm_set_exception_handler_asm(EXCCAUSE_ALLOCA,
                _xtos_alloca_handler);
    if (!ret) {
        ret = ret_alloca;
    }

    int32_t ret_spill = xtsm_set_exception_handler_asm(EXCCAUSE_SYSCALL,
                _xtos_syscall_handler);
    if (!ret) {
        ret = ret_spill;
    }
#endif
    return ret;
}


//-----------------------------------------------------------------------------
//  Register an interrupt to be handled by non-secure code
//-----------------------------------------------------------------------------
int32_t
xtsm_set_interrupt_handler(uint32_t intnum, xtsm_handler_t *handler, void *arg)
{
    register int32_t syscall_id = SECMON_SYSCALL_SET_INT_HNDLR;
    register xtsm_dispatch_fn_t nsm_dispatch = NULL;
    uint32_t handler_addr = (uint32_t)handler;
    register int32_t ret;

    if ((intnum >= XCHAL_NUM_INTERRUPTS) || 
        (Xthal_intlevel[intnum] > XCHAL_EXCM_LEVEL) ||
        xtsm_secure_addr_overlap(handler_addr, handler_addr + 4)) {
        return -EINVAL;
    }

    if (handler) {
        nsm_dispatch = xtsm_dispatch_table[Xthal_intlevel[intnum] - 1];
    }

    // Already checked that handler is in non-secure space.
    // Vector will be checked by syscall handling.
    ISSUE_SYSCALL(ret, syscall_id, intnum, nsm_dispatch);
    if (ret == 0) {
        ret = xtos_set_interrupt_handler(intnum, handler, arg, NULL);
    }

    return ret;
}


//-----------------------------------------------------------------------------
//  Register an exception to be handled by non-secure C code
//-----------------------------------------------------------------------------
int32_t
xtsm_set_exception_handler(uint32_t excnum, xtsm_handler_t *handler)
{
    int32_t ret = xtsm_set_exception_handler_asm(excnum, handler);
    if (ret == 0) {
        ret = xtos_set_exception_handler(excnum, handler, NULL);
    }
    return ret;
}


//-----------------------------------------------------------------------------
//  Helper function to install assembly-code exception handler for non-secure
//  exception processing.
//-----------------------------------------------------------------------------
static int32_t
xtsm_set_exception_handler_asm(uint32_t excnum, xtsm_handler_t handler)
{
    register int32_t syscall_id = SECMON_SYSCALL_SET_EXC_HNDLR;
    register xtsm_dispatch_fn_t nsm_vec = handler ? xtsm_dispatch_table[0] : NULL;
    uint32_t handler_addr = (uint32_t)handler;
    register int32_t ret;

    // Check (1) excnum is valid, and (2) handler in non-secure space.
    // Vector (nsm_vec) checked by syscall.
    if ((excnum >= XCHAL_EXCCAUSE_NUM) ||
        (xtsm_secure_addr_overlap(handler_addr, handler_addr + 4))) {
        return -EINVAL;
    }
    ISSUE_SYSCALL(ret, syscall_id, excnum, nsm_vec);

    // Cannot call xtsm_set_exception_handler() since it requires a C handler.
    if (ret == 0) {
        xtos_exc_handler_table[excnum] = handler_addr;
    }
    return ret;
}


//-----------------------------------------------------------------------------
//  Register a SYSCALL handler implemented in non-secure assembly code
//  If a custom handler is provided, it must also implement window spill
//  (syscall 0) as the default handler will be overridden.
//-----------------------------------------------------------------------------
int32_t
xtsm_set_syscall_handler(xtsm_handler_t *handler)
{
    return xtsm_set_exception_handler_asm(EXCCAUSE_SYSCALL, handler);
}

