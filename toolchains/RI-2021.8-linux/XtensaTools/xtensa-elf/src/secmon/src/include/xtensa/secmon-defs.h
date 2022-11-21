// Copyright (c) 2003-2021 Cadence Design Systems, Inc.
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
// Filename:    secmon-defs.h
//
// Contents:    Secure monitor definitions and types shared by secure monitor
//              and nonsecure library.
//-----------------------------------------------------------------------------
#ifndef __SECMON_DEFS_H
#define __SECMON_DEFS_H

#include <xtensa/config/secure.h>


//-----------------------------------------------------------------------------
// System call IDs for secure/nonsecure communication (active)
//-----------------------------------------------------------------------------
#define SECMON_SYSCALL_FIRST            (0x736d3000)
#define SECMON_SYSCALL_SET_INT_HNDLR    (SECMON_SYSCALL_FIRST + 0)
#define SECMON_SYSCALL_SET_EXC_HNDLR    (SECMON_SYSCALL_FIRST + 1)
#define SECMON_SYSCALL_SET_DBG_HNDLR    (SECMON_SYSCALL_FIRST + 2)
#define SECMON_SYSCALL_LAST             (SECMON_SYSCALL_SET_DBG_HNDLR)
#define SECMON_SYSCALL_NUM              (SECMON_SYSCALL_LAST - SECMON_SYSCALL_FIRST + 1)


//-----------------------------------------------------------------------------
// System call IDs for secure/nonsecure communication (reserved, future use)
//-----------------------------------------------------------------------------
#define SECMON_SYSCALL_RSVD_FIRST       (SECMON_SYSCALL_FIRST)
#define SECMON_SYSCALL_RSVD_NUM         (0x100)
#define SECMON_SYSCALL_RSVD_LAST        (SECMON_SYSCALL_RSVD_FIRST + SECMON_SYSCALL_RSVD_NUM - 1)


//-----------------------------------------------------------------------------
// System call IDs for secure/nonsecure communication (available, custom use)
//-----------------------------------------------------------------------------
#define SECMON_SYSCALL_CUSTOM_FIRST     (SECMON_SYSCALL_RSVD_FIRST + SECMON_SYSCALL_RSVD_NUM)
#define SECMON_SYSCALL_CUSTOM_NUM       (0x100)
#define SECMON_SYSCALL_CUSTOM_LAST      (SECMON_SYSCALL_CUSTOM_FIRST + SECMON_SYSCALL_CUSTOM_NUM - 1)


//-----------------------------------------------------------------------------
// System call IDs for secure/nonsecure communication (combined limit)
//-----------------------------------------------------------------------------
#define SECMON_SYSCALL_MAX              (SECMON_SYSCALL_CUSTOM_LAST)
#define SECMON_SYSCALL_MASK             (~(SECMON_SYSCALL_RSVD_NUM + SECMON_SYSCALL_CUSTOM_NUM - 1))


#if !defined(_ASMLANGUAGE) && !defined(__ASSEMBLER__)

//-----------------------------------------------------------------------------
// System call IDs for secure/nonsecure communication (with C type-checking)
//-----------------------------------------------------------------------------
typedef enum {
    XTSM_SYSCALL_FIRST                = SECMON_SYSCALL_FIRST,
    XTSM_SYSCALL_SET_INT_HNDLR        = SECMON_SYSCALL_SET_INT_HNDLR,
    XTSM_SYSCALL_SET_EXC_HNDLR,
    XTSM_SYSCALL_SET_DBG_HNDLR,
    /* Add new syscalls here */
    XTSM_SYSCALL_LAST                 = XTSM_SYSCALL_SET_DBG_HNDLR,
    XTSM_SYSCALL_NUM                  = (XTSM_SYSCALL_LAST - XTSM_SYSCALL_FIRST + 1),
    XTSM_SYSCALL_MASK                 = SECMON_SYSCALL_MASK
} xtsm_syscall_t;


#endif


#endif /* __SECMON_DEFS_H */

