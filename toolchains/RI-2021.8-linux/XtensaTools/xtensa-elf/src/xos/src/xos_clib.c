
// xos_clib.c - XOS C library management.

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

//-----------------------------------------------------------------------------
// This file implements the library-specific support for the various C runtime
// libraries compatible with XOS. This primarily consists of providing locks
// for mutual exclusion, and for initializing the per-thread context data area.
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// This file implements support routines for the C runtime library. As such it
// is considered part of the implementation of the Standard Library, which is
// not required to comply with MISRA C (Section 6.7, MISRA C 2012). We need to
// use reserved names and other non-compliant features to make this work.
//-----------------------------------------------------------------------------
// parasoft-begin-suppress ALL "This file excluded from MISRA checking"


#define XOS_INCLUDE_INTERNAL
#include "xos.h"


#if XOS_OPT_THREAD_SAFE_CLIB

//-----------------------------------------------------------------------------
// XCLIB support. See xmtx.h/.c, reent.h/.c in XCLIB for more details.
//-----------------------------------------------------------------------------

#if XSHAL_CLIB == XTHAL_CLIB_XCLIB

#include <errno.h>
#include <sys/reent.h>

//-----------------------------------------------------------------------------
//  The 16 extra locks are for C++ support. If you are not going to use C++ at
//  all, then these extra locks can be removed to save memory.
//-----------------------------------------------------------------------------
#define XOS_NUM_CLIB_LOCKS      ((uint32_t)(_MAX_LOCK + FOPEN_MAX + 16))

typedef XosMutex * _Rmtx;


//-----------------------------------------------------------------------------
//  Override this and set to nonzero to enable locking.
//-----------------------------------------------------------------------------
extern int32_t _xclib_use_mt;
int32_t        _xclib_use_mt = 1;


//-----------------------------------------------------------------------------
//  Declarations to avoid compiler warnings.
//-----------------------------------------------------------------------------
void
_Mtxinit(_Rmtx * mtx);
void
_Mtxdst(_Rmtx * mtx);
void
_Mtxlock(_Rmtx * mtx);
void
_Mtxunlock(_Rmtx * mtx);
void *
_sbrk_r (struct _reent * reent, int32_t incr);


//-----------------------------------------------------------------------------
//  Init lock.
//-----------------------------------------------------------------------------
void
_Mtxinit(_Rmtx * mtx)
{
    static XosMutex xos_clib_locks[XOS_NUM_CLIB_LOCKS];
    static uint32_t lcnt;

    XosMutex * lock;

    if (lcnt >= XOS_NUM_CLIB_LOCKS) {
        xos_fatal_error(XOS_ERR_CLIB_ERR, XOS_NULL);
    }

    lock = &(xos_clib_locks[lcnt]);
    lcnt++;

    if (xos_mutex_create(lock, 0, 0) != XOS_OK) {
        xos_fatal_error(XOS_ERR_CLIB_ERR, XOS_NULL);
    }

    *mtx = lock;
}


//-----------------------------------------------------------------------------
//  Destroy lock.
//-----------------------------------------------------------------------------
void
_Mtxdst(_Rmtx * mtx)
{
    if ((mtx != XOS_NULL) && (*mtx != XOS_NULL)) {
        if (xos_mutex_delete(*mtx) != XOS_OK) {
            xos_fatal_error(XOS_ERR_CLIB_ERR, XOS_NULL);
        }
    }
}


//-----------------------------------------------------------------------------
//  Lock.
//-----------------------------------------------------------------------------
void
_Mtxlock(_Rmtx * mtx)
{
    if ((mtx != XOS_NULL) && (*mtx != XOS_NULL)) {
        if (xos_mutex_lock(*mtx) != XOS_OK) {
            xos_fatal_error(XOS_ERR_CLIB_ERR, XOS_NULL);
        }
    }
}


//-----------------------------------------------------------------------------
//  Unlock.
//-----------------------------------------------------------------------------
void
_Mtxunlock(_Rmtx * mtx)
{
    if ((mtx != XOS_NULL) && (*mtx != XOS_NULL)) {
        if (xos_mutex_unlock(*mtx) != XOS_OK) {
            xos_fatal_error(XOS_ERR_CLIB_ERR, XOS_NULL);
        }
    }
}


//-----------------------------------------------------------------------------
//  Called by malloc() to allocate blocks of memory from the heap.
//-----------------------------------------------------------------------------
void *
_sbrk_r (struct _reent * reent, int32_t incr)
{
#if XOS_OPT_UM
    // When UM support is enabled, the default heap does not exist.
    // Provide some heap space for file I/O support etc. if specified.
#if XOS_UM_HEAP_SIZE
    static char _end[XOS_UM_HEAP_SIZE];
    static char * _heap_sentry_ptr = &_end[XOS_UM_HEAP_SIZE];
#else
    static char _end;
    static char * _heap_sentry_ptr = &_end;
#endif
#else
    extern char _end;
    extern char _heap_sentry;
    static char * _heap_sentry_ptr = &_heap_sentry;
#endif
    static char * heap_ptr         = XOS_NULL;
    char * base;
    
    if (heap_ptr == XOS_NULL) {
        heap_ptr = (char *) &_end;
    }

    base = heap_ptr;
    if ((heap_ptr + incr) >= _heap_sentry_ptr) {
        reent->_errno = ENOMEM;
        return (char *) -1;
    }
    
    heap_ptr += incr;
    return base;
}


//-----------------------------------------------------------------------------
//  Global initialization for C library.
//-----------------------------------------------------------------------------
void
xos_clib_init(void)
{
}


//-----------------------------------------------------------------------------
//  Per-thread initialization for C library.
//-----------------------------------------------------------------------------
void
xos_clib_thread_init(XosThread * thread)
{
    XOS_ASSERT(thread != XOS_NULL);

    // Set up the pointer to the C lib context area
    thread->clib_ptr = &(thread->xclib_reent);
 
    // Now init the context
    _init_reent(thread->clib_ptr);
}


//-----------------------------------------------------------------------------
//  Per-thread cleanup.
//-----------------------------------------------------------------------------
void
xos_clib_thread_cleanup(XosThread * thread)
{
    XOS_ASSERT(thread != XOS_NULL);

    thread->clib_ptr = XOS_NULL;
}

#endif // XSHAL_CLIB == XTHAL_CLIB_XCLIB


//-----------------------------------------------------------------------------
// NEWLIB support.
//-----------------------------------------------------------------------------

#if XSHAL_CLIB == XTHAL_CLIB_NEWLIB

#include <errno.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// NOTE: should have been declared in reent.h...
extern void _wrapup_reent(struct _reent * ptr);

// Single lock used for all C library protection. Use local ref count until
// XOS mutexes allow recursive locking. Note that newlib code can call the
// lock functions recursively, e.g. lock -> lock -> unlock -> unlock.

static XosMutex clib_lock;
static bool     clib_init_done;


//-----------------------------------------------------------------------------
//  Declarations to avoid compiler warnings.
//-----------------------------------------------------------------------------
void
__malloc_lock(struct _reent * ptr);
void
__malloc_unlock(struct _reent * ptr);
void
__env_lock(struct _reent * ptr);
void
__env_unlock(struct _reent * ptr);
void *
_sbrk_r (struct _reent * reent, int32_t incr);


//-----------------------------------------------------------------------------
//  Get C library lock.
//-----------------------------------------------------------------------------
void
__malloc_lock(struct _reent * ptr)
{
    UNUSED(ptr);

    // XOS not initialized yet, can't use mutex
    if (!clib_init_done) {
        return;
    }

    if (xos_mutex_lock(&clib_lock) != XOS_OK) {
        xos_fatal_error(XOS_ERR_CLIB_ERR, XOS_NULL);
    }
}


//-----------------------------------------------------------------------------
//  Release C library lock.
//-----------------------------------------------------------------------------
void
__malloc_unlock(struct _reent * ptr)
{
    UNUSED(ptr);

    if (!clib_init_done) {
        return;
    }

    XOS_ASSERT(clib_lock.owner == xos_thread_id());

    if (xos_mutex_unlock(&clib_lock) != XOS_OK) {
        xos_fatal_error(XOS_ERR_CLIB_ERR, XOS_NULL);
    }
}


//-----------------------------------------------------------------------------
//  Lock for environment. Since we have only one global lock we can just call
//  the malloc() lock function.
//-----------------------------------------------------------------------------
void
__env_lock(struct _reent * ptr)
{
    UNUSED(ptr);

    __malloc_lock(ptr);
}


//-----------------------------------------------------------------------------
//  Unlock environment.
//-----------------------------------------------------------------------------
void
__env_unlock(struct _reent * ptr)
{
    UNUSED(ptr);

    __malloc_unlock(ptr);
}


//-----------------------------------------------------------------------------
//  Called by malloc() to allocate blocks of memory from the heap.
//-----------------------------------------------------------------------------
void *
_sbrk_r (struct _reent * reent, int32_t incr)
{
#if XOS_OPT_UM
    // When UM support is enabled, the default heap does not exist.
    // Provide some heap space for file I/O support etc. if specified.
#if XOS_UM_HEAP_SIZE
    static char _end[XOS_UM_HEAP_SIZE];
    static char * _heap_sentry_ptr = &_end[XOS_UM_HEAP_SIZE];
#else
    static char _end;
    static char * _heap_sentry_ptr = &_end;
#endif
#else
    extern char _end;
    extern char _heap_sentry;
    static char * _heap_sentry_ptr = &_heap_sentry;
#endif
    static char * heap_ptr         = XOS_NULL;
    char * base;

    if (heap_ptr == XOS_NULL) {
        heap_ptr = (char *) &_end;
    }

    base = heap_ptr;
    if ((heap_ptr + incr) >= _heap_sentry_ptr) {
        reent->_errno = ENOMEM;
        return (char *) -1;
    }

    heap_ptr += incr;
    return base;
}


//-----------------------------------------------------------------------------
//  Global initialization for C library.
//-----------------------------------------------------------------------------
void
xos_clib_init(void)
{
    XOS_ASSERT(!clib_init_done);

    if (xos_mutex_create(&clib_lock, 0, 0) != XOS_OK) {
        xos_fatal_error(XOS_ERR_CLIB_ERR, XOS_NULL);
    }

    clib_init_done  = 1;
}


//-----------------------------------------------------------------------------
//  Per-thread initialization for C library.
//-----------------------------------------------------------------------------
void
xos_clib_thread_init(XosThread * thread)
{
    struct _reent * ptr;

    XOS_ASSERT(thread != XOS_NULL);

    // Init the context area
    ptr = &(thread->newlib_reent);
    memset(ptr, 0, sizeof(struct _reent));
    _REENT_INIT_PTR(ptr);

    // Set pointer to reent struct in TCB
    thread->clib_ptr = ptr;
}


//-----------------------------------------------------------------------------
//  Per-thread cleanup.
//-----------------------------------------------------------------------------
void
xos_clib_thread_cleanup(XosThread * thread)
{
    // Avoid closing stdin,stdout,stderr so other threads can still use them.
    // This seems like a bug in newlib.

    struct _reent * ptr;
    FILE *          fp;
    int32_t         i;

    XOS_ASSERT(thread != XOS_NULL);

    ptr = &(thread->newlib_reent);
    fp  = &(ptr->__sf[0]);

    for (i = 0; i < 3; ++i, ++fp) {
        fp->_close = XOS_NULL;
    }

    _wrapup_reent(ptr);
    _reclaim_reent(ptr);
}

#endif // XSHAL_CLIB == XTHAL_CLIB_NEWLIB


#endif // XOS_OPT_THREAD_SAFE_CLIB

// parasoft-end-suppress ALL

