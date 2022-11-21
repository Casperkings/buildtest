// Copyright (c) 2004-2021 Cadence Design Systems, Inc.
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

// syscall.c -- Secure Monitor system call handling


#include <xtensa/xtruntime.h>
#include <errno.h>

#include <xtensa/secmon-common.h>
#include <xtensa/secmon-defs.h>
#include <xtos-internal.h>


// Syscall handler signature.  Up to 2 arguments currently supported.
typedef int32_t (secure_syscall_proc)(uint32_t n, xtos_handler h);


// Type definition for XTOS interrupt handling structure [IDs, handlers]
typedef struct {
    xtos_handler    h;
    void *          p;
} secure_xtos_inttab_entry_t;

// Type definition for syscall handling structure [IDs, handlers]
typedef struct {
    xtsm_syscall_t          id;
    secure_syscall_proc *   proc;
} secure_syscall_entry_t;


// Prototypes
int32_t handle_syscall(xtsm_syscall_t id, uint32_t n, xtos_handler h);
static int32_t proc_set_int_handler(uint32_t intnum, xtos_handler inthandler);
static int32_t proc_set_exc_handler(uint32_t excnum, xtos_handler exchandler);
static int32_t proc_set_dbg_handler(uint32_t unused, xtos_handler dbghandler);


// Externs
extern uint8_t secmon_nsm_interrupt_table[XCHAL_NUM_INTERRUPTS];
extern secure_xtos_inttab_entry_t xtos_interrupt_table[XCHAL_NUM_INTERRUPTS];
extern xtos_handler xtos_exc_handler_table[XCHAL_EXCCAUSE_NUM];
extern xtos_handler xtos_c_handler_table[XCHAL_EXCCAUSE_NUM];
extern xtos_handler secmon_nsm_syscall_handler;
extern xtos_handler secmon_nsm_dbg_handler;

extern void secmon_dispatch_exc_to_nsm(void *arg);
extern void secmon_cpdisable_handler(void *arg);
extern void xtos_unhandled_exception(void *arg);
extern void xtos_unhandled_interrupt(void *arg);
extern void xtos_p_none(void *arg);


// Table of secure syscalls
static secure_syscall_entry_t syscall_proc[XTSM_SYSCALL_NUM] = {
    { XTSM_SYSCALL_SET_INT_HNDLR,   proc_set_int_handler },
    { XTSM_SYSCALL_SET_EXC_HNDLR,   proc_set_exc_handler },
    { XTSM_SYSCALL_SET_DBG_HNDLR,   proc_set_dbg_handler },
};


// Redefine Xthal_intlevel[] since HAL library may not available to secure code.
// HAL isn't rebuilt for call0 ABI, and XTOS code depends only on this array,
// as well as interrupt handling functions in this file.
const uint16_t __attribute__((weak)) Xthal_intlevel[XTHAL_MAX_INTERRUPTS] = {
    XCHAL_INT_LEVELS
};


// Exceptions available for nonsecure handling. Indexed by cause ID.
//
// NOTE: Some exceptions can safely be made routeable to nonsecure
// handlers (by modifying this table during secure monitor init).
//
// WARNING: extreme caution is advised before changing this table!
// Ensure all exceptions that could result from security violations
// are never redirected; doing so has significant security risks.
//
// Table values:
// 0 - non-secure handling unavailable
// 1 - non-secure handling available
// 2 - special case; some non-secure handling available
uint8_t secmon_nonsecure_exceptions[XCHAL_EXCCAUSE_NUM] = {
    0, /* EXCCAUSE_ILLEGAL          (0): Illegal Instruction */
    2, /* EXCCAUSE_SYSCALL          (1): System Call  */
    0, /* EXCCAUSE_INSTR_ERROR      (2): Instruction Fetch Error */
    0, /* EXCCAUSE_LOAD_STORE_ERROR (3): Load Store Error */
    0, /* EXCCAUSE_LEVEL1_INTERRUPT (4): Level 1 Interrupt */
    1, /* EXCCAUSE_ALLOCA           (5): Stack Extension Assist for alloca */
    0, /* EXCCAUSE_DIVIDE_BY_ZERO   (6): Integer Divide by Zero */
    0, /* EXCCAUSE_PC_ERROR         (7): Next PC Value Illegal */
    0, /* EXCCAUSE_PRIVILEGED       (8): Privileged Instruction */
    0, /* EXCCAUSE_UNALIGNED        (9): Unaligned Load or Store */
    0, /* EXCCAUSE_EXTREG_PRIVILEGE (10): External Register Privilege Error */
    0, /* EXCCAUSE_EXCLUSIVE_ERROR  (11): Load exclusive to unsupported memory type or unaligned address */
    0, /* EXCCAUSE_INSTR_DATA_ERROR (12): PIF Data Error on Instruction Fetch  */
    0, /* EXCCAUSE_LOAD_STORE_DATA_ERROR (13): PIF Data Error on Load or Store  */
    0, /* EXCCAUSE_INSTR_ADDR_ERROR (14): PIF Address Error on Instruction Fetch */
    0, /* EXCCAUSE_LOAD_STORE_ADDR_ERROR (15): PIF Address Error on Load or Store */
    0, /* EXCCAUSE_ITLB_MISS        (16): ITLB Miss (no ITLB entry matches, hw refill also missed) */
    0, /* EXCCAUSE_ITLB_MULTIHIT    (17): ITLB Multihit (multiple ITLB entries match) */
    0, /* EXCCAUSE_INSTR_RING       (18): Ring Privilege Violation on Instruction Fetch */
    0, /* Reserved */
    0, /* EXCCAUSE_INSTR_PROHIBITED (20): Cache Attribute does not allow Instruction Fetch */
    0, /* Reserved */
    0, /* Reserved */
    0, /* Reserved */
    0, /* EXCCAUSE_DTLB_MISS        (24): DTLB Miss (no DTLB entry matches, hw refill also missed) */
    0, /* EXCCAUSE_DTLB_MULTIHIT    (25): DTLB Multihit (multiple DTLB entries match) */
    0, /* EXCCAUSE_LOAD_STORE_RING  (26): Ring Privilege Violation on Load or Store */
    0, /* Reserved */
    0, /* EXCCAUSE_LOAD_PROHIBITED  (28): Cache Attribute does not allow Load */
    0, /* EXCCAUSE_STORE_PROHIBITED (29): Cache Attribute does not allow Store */
    0, /* Reserved */
    0, /* Reserved */
    1, /* EXCCAUSE_CP0_DISABLED     (32): Access to Coprocessor 0 when disabled */
    1, /* EXCCAUSE_CP1_DISABLED     (33): Access to Coprocessor 1 when disabled */
    1, /* EXCCAUSE_CP2_DISABLED     (34): Access to Coprocessor 2 when disabled */
    1, /* EXCCAUSE_CP3_DISABLED     (35): Access to Coprocessor 3 when disabled */
    1, /* EXCCAUSE_CP4_DISABLED     (36): Access to Coprocessor 4 when disabled */
    1, /* EXCCAUSE_CP5_DISABLED     (37): Access to Coprocessor 5 when disabled */
    1, /* EXCCAUSE_CP6_DISABLED     (38): Access to Coprocessor 6 when disabled */
    1, /* EXCCAUSE_CP7_DISABLED     (39): Access to Coprocessor 7 when disabled */
};


// Process syscall for registering nonsecure interrupt handlers
static int32_t proc_set_int_handler(uint32_t intnum, xtos_handler inthandler)
{
    int32_t ret;
    uint32_t handler_addr = (uint32_t)inthandler;
    if ((intnum >= (uint32_t) XCHAL_NUM_INTERRUPTS) || 
        xtsm_secure_addr_overlap(handler_addr, handler_addr + 4)) {
        return -EINVAL;
    }
    if (Xthal_intlevel[intnum] > XCHAL_EXCM_LEVEL) {
        return -EACCES;
    }
    if (secmon_nsm_interrupt_table[intnum] != 1) {
        // Additional checks if intnum not associated with a nonsecure handler
        if (inthandler == NULL) {
            return -EADDRNOTAVAIL;
        }
        if (xtos_interrupt_table[MAPINT(intnum)].h != xtos_unhandled_interrupt) {
            return -EADDRINUSE;
        }
    }

    // Install nonsecure interrupt handler or remove if NULL
    ret = xtos_set_interrupt_handler(intnum, inthandler, NULL, NULL);
    if (ret == 0) {
        secmon_nsm_interrupt_table[intnum] = (inthandler == NULL) ? 0 : 1;
    }
    return ret;
}


static int32_t proc_set_exc_handler(uint32_t excnum, xtos_handler exchandler)
{
    uint32_t is_coproc_ex;
    uint32_t handler_addr = (uint32_t)exchandler;
    if ((excnum >= (uint32_t) XCHAL_EXCCAUSE_NUM) ||
        xtsm_secure_addr_overlap(handler_addr, handler_addr + 4)) {
        return -EINVAL;
    }
    if (!secmon_nonsecure_exceptions[excnum]) {
        return -EACCES;
    } else if (secmon_nonsecure_exceptions[excnum] > 1) {
        // Special exception handling is completely processed here
        int32_t ret = -EINVAL;
        if (excnum == EXCCAUSE_SYSCALL) {
            // Nonsecure syscall handling must first go through secure handler;
            // do not override xtos_exc_handler_table[] entry.
            secmon_nsm_syscall_handler = exchandler ? secmon_dispatch_exc_to_nsm : NULL;
            xtos_c_handler_table[excnum] = exchandler;
            ret = 0;
        }
        return ret;
    }

    if ((exchandler != NULL) && 
        (xtos_exc_handler_table[excnum] != xtos_unhandled_exception) &&
        (xtos_exc_handler_table[excnum] != secmon_dispatch_exc_to_nsm) &&
        (xtos_exc_handler_table[excnum] != secmon_cpdisable_handler)) {
        return -EADDRINUSE;
    }
    if ((exchandler == NULL) && (xtos_exc_handler_table[excnum] != secmon_dispatch_exc_to_nsm)) {
        return -EADDRNOTAVAIL;
    }

    // Install nonsecure exception handler or remove if NULL
    // Don't call xtos_set_exception_handler() since it registers a C
    // handler and we want to do a fast restore/RFE to nonsecure code.
    is_coproc_ex = ((excnum >= EXCCAUSE_CP0_DISABLED) && (excnum <= EXCCAUSE_CP7_DISABLED));
    xtos_exc_handler_table[excnum] = ( (exchandler == NULL) ?
            (is_coproc_ex ? secmon_cpdisable_handler : xtos_unhandled_exception) :
            secmon_dispatch_exc_to_nsm );

    // Nonsecure handlers do their own state save/restore prior to calling C code
    xtos_c_handler_table[excnum] = ( (exchandler == NULL) ? xtos_p_none : exchandler );

    return 0;
}


// Process syscall for registering nonsecure debug interrupt handler
static int32_t proc_set_dbg_handler(uint32_t unused, xtos_handler dbghandler)
{
    uint32_t handler_addr = (uint32_t)dbghandler;
    if (xtsm_secure_addr_overlap(handler_addr, handler_addr + 4)) {
        return -EINVAL;
    }

    // Install nonsecure debug handler or remove if NULL
    secmon_nsm_dbg_handler = dbghandler;
    UNUSED(unused);
    return 0;
}


// Main syscall processing routine
int32_t handle_syscall(xtsm_syscall_t id, uint32_t n, xtos_handler h)
{
    int32_t i;
    if ((id & XTSM_SYSCALL_MASK) != XTSM_SYSCALL_FIRST) {
        return -ENOSYS;
    }
    for (i = 0; i < XTSM_SYSCALL_NUM; i++) {
        if (syscall_proc[i].id == id) {
            return (syscall_proc[i].proc)(n, h);
        }
    }
    return -ENOSYS;
}

