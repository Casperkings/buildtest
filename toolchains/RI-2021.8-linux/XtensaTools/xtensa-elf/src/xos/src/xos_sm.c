// xos_sm.c - XOS interface to Xtensa Secure Mode.

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
#include "xos_sm.h"


#if XOS_OPT_SECMON

//-----------------------------------------------------------------------------
// Check config requirements.
//-----------------------------------------------------------------------------
#if !XCHAL_HAVE_XEA2
#error "XOS_OPT_SECMON is supported for Xtensa LX (XEA2) only."
#endif
#if !XCHAL_HAVE_SECURE
#error "Xtensa LX Secure Mode is required for XOS_OPT_SECMON."
#endif
#if (XCHAL_NUM_MISC_REGS < 2)
#error "MISC1 register must be configured for XOS_OPT_SECMON."
#endif

#include <xtensa/secmon.h>
#include <xtensa/secmon-defs.h>


//-----------------------------------------------------------------------------
// XOS SM dispatch table entry. The actual dispatch points are assembly stubs
// not C functions, this is mainly to keep the compiler happy.
//-----------------------------------------------------------------------------
typedef void (*xos_sm_dispatch_fn_t)(void);

//-----------------------------------------------------------------------------
// Dispatch entry points.
//-----------------------------------------------------------------------------
extern void _UserExceptionVector(void);
extern void _Level2Vector(void);
extern void _Level3Vector(void);
extern void _Level4Vector(void);
extern void _Level5Vector(void);
extern void _Level6Vector(void);

//-----------------------------------------------------------------------------
// Dispatch table.
//-----------------------------------------------------------------------------
const xos_sm_dispatch_fn_t
xos_sm_dispatch_table[XCHAL_EXCM_LEVEL] =
{
    _UserExceptionVector,
#if (XCHAL_EXCM_LEVEL >= 2)
    _Level2Vector,
#endif
#if (XCHAL_EXCM_LEVEL >= 3)
    _Level3Vector,
#endif
#if (XCHAL_EXCM_LEVEL >= 4)
    _Level4Vector,
#endif
#if (XCHAL_EXCM_LEVEL >= 5)
    _Level5Vector,
#endif
#if (XCHAL_EXCM_LEVEL >= 6)
    _Level6Vector,
#endif
};

//-----------------------------------------------------------------------------
// Helper function to execute secure monitor syscall.
//-----------------------------------------------------------------------------
extern int32_t xos_xtsm_syscall(int32_t id, uint32_t p1, uint32_t p2);


//-----------------------------------------------------------------------------
//  Request interrupt forwarding from secure monitor.
//-----------------------------------------------------------------------------
int32_t
xos_xtsm_set_int_handler(uint32_t intnum, void * handler)
{
    int32_t ret;
    int32_t syscall_id = SECMON_SYSCALL_SET_INT_HNDLR;
    xos_sm_dispatch_fn_t dfn;

    dfn = (handler != XOS_NULL) ? xos_sm_dispatch_table[Xthal_intlevel[intnum] - 1] : XOS_NULL;
    ret = xos_xtsm_syscall(syscall_id, intnum, xos_voidp_to_uint32((void *)dfn));
    return ret;
}


//-----------------------------------------------------------------------------
//  Request exception forwarding from secure monitor.
//-----------------------------------------------------------------------------
int32_t
xos_xtsm_set_exc_handler(uint32_t excnum, void * handler)
{
    int32_t ret;
    int32_t syscall_id = SECMON_SYSCALL_SET_EXC_HNDLR;
    xos_sm_dispatch_fn_t dfn;

    dfn = (handler != XOS_NULL) ? xos_sm_dispatch_table[0] : XOS_NULL;
    ret = xos_xtsm_syscall(syscall_id, excnum, xos_voidp_to_uint32((void *)dfn));
    return ret;
}

#endif // XOS_OPT_SECMON

