
// xos_usermode.c - XOS user-mode thread support.

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


#define XOS_INCLUDE_INTERNAL
#include "xos.h"
#include "xos_syscall.h"

#include <xtensa/xmem.h>

#if XOS_OPT_UM

#if !XCHAL_HAVE_MPU
#error XOS_OPT_UM requires the MPU to be available.
#endif


// Pool of user-mode semaphore objects.

XosUSem xos_um_sems[XOS_UM_NUM_SEM];

// Pool of user-mode mutex objects.

XosUMutex xos_um_mtxs[XOS_UM_NUM_MTX];

// Pool of user-mode condition objects.

XosUCond xos_um_conds[XOS_UM_NUM_COND];

// Pointer to previous exception handlers.

static XosExcHandlerFunc * old_priv_handler;
static XosExcHandlerFunc * old_syscall_handler;

// Heap manager and pool header for the user-mode memory pool.

#define PPHDR_SIZE      XMEM_HEAP_CACHE_ALIGNED_HEADER_SIZE(XOS_UM_NUM_PBLKS)

static xmem_heap_mgr_t xos_um_private_pool;
static uint8_t         xos_um_pphdr[PPHDR_SIZE];

// Pool of user-mode TCBs.

static XosBlockPool xos_um_tcb_pool;
static uint8_t      xos_um_tcbmem[sizeof(XosUThread) * XOS_UM_NUM_THREAD];


//-----------------------------------------------------------------------------
// Allocate user TCB.
//-----------------------------------------------------------------------------
XosUThread *
xos_um_tcb_alloc(void)
{
    return (XosUThread *) xos_block_alloc(&xos_um_tcb_pool);
}


//-----------------------------------------------------------------------------
// Free user TCB.
//-----------------------------------------------------------------------------
int32_t
xos_um_tcb_free(XosUThread * tcb)
{
    return xos_block_free(&xos_um_tcb_pool, tcb);
}


//-----------------------------------------------------------------------------
// Validate user TCB.
//-----------------------------------------------------------------------------
bool
xos_um_tcb_valid(XosUThread * tcb)
{
    XosUThread * p = (XosUThread *) xos_um_tcbmem;

    if ((tcb >= p) && (tcb <= (p + XOS_UM_NUM_THREAD - 1))) {
        return true;
    }

    return false;
}


//-----------------------------------------------------------------------------
// Verify that specified address range has user-mode read/write access.
// Return true if access is allowed.
//-----------------------------------------------------------------------------
static bool
xos_um_access_ok(void * addr, uint32_t len)
{
    int32_t in_fg;
    struct  xthal_MPU_entry e1;

    // We insist that user addresses be mapped by the foreground map,
    // and that the address not be zero, and that the size not be zero.
    if ((addr == XOS_NULL) || (len == 0U)) {
        return false;
    }
    e1 = xthal_get_entry_for_address(addr, &in_fg);
    if (in_fg == 0) {
        return false;
    }

    if ((xthal_is_user_readable(XTHAL_MPU_ENTRY_GET_ACCESS(e1)) == 1) &&
        (xthal_is_user_writeable(XTHAL_MPU_ENTRY_GET_ACCESS(e1)) == 1)) {
        // Now check that the end address of the range is also covered by
        // the same MPU entry.
        char * end_addr           = (char *) addr + len - 1;
        struct xthal_MPU_entry e2 = xthal_get_entry_for_address(end_addr, &in_fg);

        return ((e2.as == e1.as) && (e2.at == e1.at));
    }

    return false;
}


//-----------------------------------------------------------------------------
// Allocate user-mode block, align to multiple of MPU alignment.
//-----------------------------------------------------------------------------
int32_t
xos_um_private_mem_alloc(uint32_t nbytes, uint32_t align, void ** pp)
{
    int32_t ret;
    void *  p;

    if ((nbytes == 0) || (pp == XOS_NULL)) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    p = xmem_heap_alloc(&xos_um_private_pool,
                        nbytes,
                        (align + XCHAL_MPU_ALIGN - 1) & -XCHAL_MPU_ALIGN,
                        &ret);

    if ((p != XOS_NULL) && (ret == XMEM_OK)) {
        *pp = p;
        return XOS_OK;
    }

    *pp = XOS_NULL;
    return XOS_ERR_LIMIT;
}


//-----------------------------------------------------------------------------
// Free user-mode block.
//-----------------------------------------------------------------------------
int32_t
xos_um_private_mem_free(void * p)
{
    int32_t ret = XMEM_OK;

    if (p != XOS_NULL) {
        ret = xmem_heap_free(&xos_um_private_pool, p);
    }

    return (ret == XMEM_OK) ? XOS_OK : XOS_ERR_INVALID_PARAMETER;
}


//-----------------------------------------------------------------------------
// Handle privilege exceptions.
//-----------------------------------------------------------------------------
static void
xos_priv_handler(XosExcFrame * ef)
{
    // We need the address of the magic return label.
    extern void xos_thread_exit_trap(void);

    if (ef->pc == (uint32_t)&xos_thread_exit_trap) {
        // User thread returning, set ring = 0 in saved PS.
        ef->ps &= ~PS_RING_MASK;
        return;
    }

    // Delegate to previous handler.

    DPRINTF("priv_handler: Error, user code did not return to expected address\n");
    XOS_ASSERT(old_priv_handler != XOS_NULL);
    (*old_priv_handler)(ef);
}


//-----------------------------------------------------------------------------
// Handle syscall exceptions.
//-----------------------------------------------------------------------------
static void
xos_um_syscall_handler(XosExcFrame * ef)
{
    // Local defines: aliases for the exception frame fields used by this
    // handler. F_CALLID holds the syscall request ID, and F_RETVAL holds
    // the return value if any. F_ARG1/4 are the syscall arguments.

#if XCHAL_HAVE_XEA2
#define F_CALLID        (ef->a2)
#define F_RETVAL        (ef->a2)
#define F_PC            (ef->pc)
#define F_ARG1          (ef->a3)
#define F_ARG2          (ef->areg[0])
#define F_ARG3          (ef->areg[1])
#define F_ARG4          (ef->areg[2])
#endif
#if XCHAL_HAVE_XEA3
#define F_CALLID        (ef->a8)
#define F_RETVAL        (ef->a8)
#define F_PC            (ef->pc)
#define F_ARG1          (ef->a9)
#define F_ARG2          (ef->a10)
#define F_ARG3          (ef->a11)
#define F_ARG4          (ef->a12)
#endif

#if XCHAL_HAVE_XEA2
    XOS_ASSERT(ef->exccause == EXCCAUSE_SYSCALL);
#endif
#if XCHAL_HAVE_XEA3
    XOS_ASSERT((ef->exccause & EXCCAUSE_CAUSE_MASK) == EXCCAUSE_SYSCALL);
#endif

    DPRINTF("syscall: id 0x%08x\n", F_CALLID);

    switch (F_CALLID) {

    //-------------------------------------------------------------------------
    // Thread operations.
    //-------------------------------------------------------------------------

    case XOS_SC_THREAD_CREATE:
        {
            uint32_t             i;
            int32_t              ret;
            XosUThread *         ptcb;
            XosUThread **        pthd   = (XosUThread **) F_ARG2;
            xos_uthread_params * params = (xos_uthread_params *) F_ARG1;

            // First validate the params structure and the ptr storage.
            // Both must point to user memory space.
            if (!xos_um_access_ok(params, sizeof(*params)) ||
                !xos_um_access_ok(pthd, 4)) {
                F_RETVAL = XOS_ERR_INVALID_PARAMETER;
                break;
            }
            // Verify that name and stack are both in user space.
            if (!xos_um_access_ok(params->name, XOS_UM_OBJ_NAME_LEN) ||
                !xos_um_access_ok(params->stack, params->stack_size)) {
                F_RETVAL = XOS_ERR_INVALID_PARAMETER;
                break;
            }

            // Allocate a user TCB.
            ptcb = xos_um_tcb_alloc();
            if (ptcb == XOS_NULL) {
                F_RETVAL = XOS_ERR_LIMIT;
                break;
            }

            // Copy the thread name into the user TCB. We cannot keep a
            // pointer to user memory for the name.
            for (i = 0; i < XOS_UM_OBJ_NAME_LEN; i++) {
                ptcb->name[i] = params->name[i];
                if (params->name[i] == 0) {
                    break;
                }
            }
            ptcb->name[i] = 0;

            // Create user thread.
            ret = xos_uthread_create_p(ptcb,
                                       (XosThreadFunc *) params->entry,
                                       params->arg,
                                       0,
                                       0,
                                       ptcb->name,
                                       params->stack,
                                       params->stack_size,
                                       params->priority,
                                       params->flags,
                                       XOS_NULL,
                                       0,
                                       0);
            if (ret == XOS_OK) {
                *pthd = ptcb;
            }

            F_RETVAL = ret;
            break;
        }

    case XOS_SC_THREAD_JOIN:
        {
            int32_t      ret;
            int32_t      rc;
            XosUThread * pthd = (XosUThread *) F_ARG1;
            int32_t *    prc  = (int32_t *) F_ARG2;

            if (xos_um_tcb_valid(pthd) && xos_um_access_ok(prc, 4)) {
                ret = xos_thread_join(xos_uthread_thread(pthd), &rc);
                *prc = rc;
            }
            else {
                ret = XOS_ERR_INVALID_PARAMETER;
            }

            F_RETVAL = ret;
            break;
        }

    case XOS_SC_THREAD_DELETE:
        {
            int32_t      ret;
            XosUThread * pthd = (XosUThread *) F_ARG1;

            if (xos_um_tcb_valid(pthd)) {
                ret = xos_uthread_delete_p(pthd);
                if (ret == XOS_OK) {
                    (void) xos_um_tcb_free(pthd);
                }
            }
            else {
                ret = XOS_ERR_INVALID_PARAMETER;
            }

            F_RETVAL = ret;
            break;
        }

    case XOS_SC_THREAD_EXIT:
        xos_thread_exit((int32_t)F_ARG1);
        break;

    case XOS_SC_THREAD_YIELD:
        xos_thread_yield();
        F_RETVAL = XOS_OK;
        break;

    case XOS_SC_THREAD_SLEEP:
        {
            uint64_t cycles = (((uint64_t)F_ARG1) << 32) | (uint64_t)(F_ARG2);

            F_RETVAL = (uint32_t) xos_thread_sleep(cycles);
            break;
        }

    case XOS_SC_THREAD_SLEEP_MSEC:
        {
            uint64_t msecs = (((uint64_t)F_ARG1) << 32) | (uint64_t)(F_ARG2);

            F_RETVAL = (uint32_t) xos_thread_sleep_msec(msecs);
            break;
        }

    case XOS_SC_THREAD_SLEEP_USEC:
        {
            uint64_t usecs = (((uint64_t)F_ARG1) << 32) | (uint64_t)(F_ARG2);

            F_RETVAL = (uint32_t) xos_thread_sleep_usec(usecs);
            break;
        }

    case XOS_SC_THREAD_ID:
        F_RETVAL = (uint32_t) xos_thread_id();
        break;

    //-------------------------------------------------------------------------
    // Semaphore operations.
    //-------------------------------------------------------------------------

    case XOS_SC_SEM_CREATE:
        {
            uint64_t     key  = (((uint64_t)F_ARG1) << 32) | (uint64_t)(F_ARG2);
            int8_t       icnt = (int8_t) F_ARG3;
            XosUSem **   psem = (XosUSem **) F_ARG4;
            XosUSem *    usem;

            // Validate user pointers.
            if (!xos_um_access_ok(psem, 4)) {
                F_RETVAL = XOS_ERR_INVALID_PARAMETER;
                break;
            }

            F_RETVAL = xos_usem_create_p(key, icnt, &usem);
            if (F_RETVAL == XOS_OK) {
                *psem = usem;
            }

            break;
        }

    case XOS_SC_SEM_DELETE:
        // Called function will validate user pointer.
        F_RETVAL = xos_usem_delete_p((XosUSem *)F_ARG1);
        break;

    case XOS_SC_SEM_GET:
        // Called function will validate user pointer.
        F_RETVAL = xos_sem_get(xos_usem_sem((XosUSem *)F_ARG1));
        break;

    case XOS_SC_SEM_GET_TIMEOUT:
        // Called function will validate user pointer.
        {
            XosSem * sem   = xos_usem_sem((XosUSem *)F_ARG1);
            uint64_t usecs = (((uint64_t)F_ARG2) << 32) | (uint64_t)(F_ARG3);

            F_RETVAL = xos_sem_get_timeout(sem, xos_usecs_to_cycles(usecs));
            break;
        }

    case XOS_SC_SEM_PUT:
        // Called function will validate user pointer.
        F_RETVAL = xos_sem_put(xos_usem_sem((XosUSem *)F_ARG1));
        break;

    case XOS_SC_SEM_PUT_MAX:
        // Called function will validate user pointer.
        F_RETVAL = xos_sem_put_max(xos_usem_sem((XosUSem *)F_ARG1), (int32_t)F_ARG2);
        break;

    //-------------------------------------------------------------------------
    // Mutex operations.
    //-------------------------------------------------------------------------

    case XOS_SC_MTX_CREATE:
        {
            uint64_t     key  = (((uint64_t)F_ARG1) << 32) | (uint64_t)(F_ARG2);
            int8_t       prio = (int8_t) F_ARG3;
            XosUMutex ** pmtx = (XosUMutex **) F_ARG4;
            XosUMutex *  umtx;

            // Validate user pointers.
            if (!xos_um_access_ok(pmtx, 4)) {
                F_RETVAL = XOS_ERR_INVALID_PARAMETER;
                break;
            }

            F_RETVAL = xos_umutex_create_p(key, prio, &umtx);
            if (F_RETVAL == XOS_OK) {
                *pmtx = umtx;
            }

            break;
        }

    case XOS_SC_MTX_DELETE:
        // Called function will validate user pointer.
        F_RETVAL = xos_umutex_delete_p((XosUMutex *)F_ARG1);
        break;

    case XOS_SC_MTX_LOCK:
        // Called function will validate user pointer.
        F_RETVAL = xos_mutex_lock(xos_umutex_mutex((XosUMutex *)F_ARG1));
        break;

    case XOS_SC_MTX_LOCK_TIMEOUT:
        // Called function will validate user pointer.
        {
            XosMutex * mtx   = xos_umutex_mutex((XosUMutex *)F_ARG1);
            uint64_t   usecs = (((uint64_t)F_ARG2) << 32) | (uint64_t)(F_ARG3);

            F_RETVAL = xos_mutex_lock_timeout(mtx, xos_usecs_to_cycles(usecs));
            break;
        }

    case XOS_SC_MTX_UNLOCK:
        // Called function will validate user pointer.
        F_RETVAL = xos_mutex_unlock(xos_umutex_mutex((XosUMutex *)F_ARG1));
        break;

    //-------------------------------------------------------------------------
    // Condition Operations.
    //-------------------------------------------------------------------------

    case XOS_SC_COND_CREATE:
        {
            uint64_t    key   = (((uint64_t)F_ARG1) << 32) | (uint64_t)(F_ARG2);
            XosUCond ** pcond = (XosUCond **) F_ARG3;
            XosUCond *  ucond;

            // Validate user pointers.
            if (!xos_um_access_ok(pcond, 4)) {
                F_RETVAL = XOS_ERR_INVALID_PARAMETER;
                break;
            }

            F_RETVAL = xos_ucond_create_p(key, &ucond);
            if (F_RETVAL == XOS_OK) {
                *pcond = ucond;
            }

            break;
        }

    case XOS_SC_COND_DELETE:
        // Called function will validate user pointer.
        F_RETVAL = xos_ucond_delete_p((XosUCond *)F_ARG1);
        break;

    case XOS_SC_COND_WAIT:
        // Called function will validate user pointer.
        F_RETVAL = xos_cond_wait(xos_ucond_cond((XosUCond *)F_ARG1), XOS_NULL, XOS_NULL);
        break;

    case XOS_SC_COND_WAIT_MTX:
        // Called function will validate user pointer.
        {
            XosCond *  cond = xos_ucond_cond((XosUCond *)F_ARG1);
            XosMutex * mtx  = xos_umutex_mutex((XosUMutex *)F_ARG2);

            F_RETVAL = xos_cond_wait_mutex(cond, mtx);
            break;
        }

    case XOS_SC_COND_WAIT_MTX_TIMEOUT:
        // Called function will validate user pointer.
        {
            XosCond *  cond  = xos_ucond_cond((XosUCond *)F_ARG1);
            XosMutex * mtx   = xos_umutex_mutex((XosUMutex *)F_ARG2);
            uint64_t   usecs = (((uint64_t)F_ARG3) << 32) | (uint64_t)(F_ARG4);

            F_RETVAL = xos_cond_wait_mutex_timeout(cond, mtx, usecs);
            break;
        }

    case XOS_SC_COND_SIGNAL:
        // Called function will validate user pointer.
        F_RETVAL = xos_cond_signal(xos_ucond_cond((XosUCond *)F_ARG1), (int32_t)F_ARG2);
        break;

    case XOS_SC_COND_SIGNAL_ONE:
        // Called function will validate user pointer.
        F_RETVAL = xos_cond_signal_one(xos_ucond_cond((XosUCond *)F_ARG1), (int32_t)F_ARG2);
        break;

    default:
        old_syscall_handler(ef);
        return;
    }

    // Bump exception PC past the syscall instruction.
    F_PC += 3;

    DPRINTF("syscall: retval %d\n", F_RETVAL);
}


//-----------------------------------------------------------------------------
// Initialize user-mode support.
//-----------------------------------------------------------------------------
int32_t
xos_um_init(void *   umem,
            uint32_t umem_size)
{
    int32_t  ret;
    uint32_t start_addr;
    uint32_t ppool_size;

    if ((umem == XOS_NULL) || (umem_size == 0)) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    // Align start address and size to MPU region. Size must be rounded down.

    start_addr = ((uint32_t)umem + XCHAL_MPU_ALIGN - 1) & -XCHAL_MPU_ALIGN;
    ppool_size = umem_size & -XCHAL_MPU_ALIGN;

    // Create pool.

    ret = xmem_heap_init(&xos_um_private_pool,
                         (void *) start_addr,
                         ppool_size,
                         XOS_UM_NUM_PBLKS,
                         xos_um_pphdr);
    if (ret != XMEM_OK) {
        return XOS_ERR_INTERNAL_ERROR;
    }

    // Create TCB pool.

    ret = xos_block_pool_init(&xos_um_tcb_pool,
                              xos_um_tcbmem,
                              sizeof(XosUThread),
                              XOS_UM_NUM_THREAD,
                              XOS_BLOCKMEM_WAIT_PRIORITY);
    if (ret != XOS_OK) {
        return XOS_ERR_INTERNAL_ERROR;
    }

    // Set up exception handler for ring privilege.

#if XCHAL_HAVE_XEA2
    old_priv_handler = xos_register_exception_handler(EXCCAUSE_INSTR_PROHIBITED,
                                                      xos_priv_handler);
#endif
#if XCHAL_HAVE_XEA3
    old_priv_handler = xos_register_exception_handler(EXCCAUSE_MEMORY,
                                                      xos_priv_handler);
#endif

    // Set up syscall handler.

    old_syscall_handler = xos_register_exception_handler(EXCCAUSE_SYSCALL,
                                                         xos_um_syscall_handler);
    return ret;
}

#endif // XOS_OPT_UM

