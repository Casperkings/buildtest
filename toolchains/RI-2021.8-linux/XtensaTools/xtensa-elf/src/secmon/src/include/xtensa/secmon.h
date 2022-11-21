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


//-----------------------------------------------------------------------------
// Filename:    secmon.h
//
// Contents:    Secure monitor handler registration APIs
//
// Usage:       #include from non-secure application code. 
//              Implemented by libsecmon.a
//-----------------------------------------------------------------------------
#ifndef __SECMON_H
#define __SECMON_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


//-----------------------------------------------------------------------------
// Generic interrupt/exception handler function pointer type
//-----------------------------------------------------------------------------
typedef void    (xtsm_handler_t)(void * arg);


//-----------------------------------------------------------------------------
// External references
//-----------------------------------------------------------------------------
extern xtsm_handler_t _xtos_syscall_handler;


//-----------------------------------------------------------------------------
// Generic interrupt/exception handler function pointer type
//-----------------------------------------------------------------------------
#define XTSM_SYSCALL_HANDLER_DEFAULT    ((xtsm_handler_t *)_xtos_syscall_handler)


//-----------------------------------------------------------------------------
///
///  Initialize secure monitor library. 
///
///  If windowed register option is configured, this installs the default
///  alloca() handler and syscall(0) (register spill) handler.
///
///  \return    0 on success, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xtsm_init(void);


//-----------------------------------------------------------------------------
///
///  Register low-priority interrupt for nonsecure handling.
///
///  NOTE: Handling nonsecure interrupt and exception requests from the 
///  secure monitor currently leverages XTOS handlers and API calls.  If the 
///  application provides its own handling, a replacement "virtual vector" 
///  must be provided (replacing lib/user-vector-nsm.S) and registered with 
///  the secure monitor.
///
///  \param     intnum          Xtensa interrupt number to be serviced by
///                             non-secure code.  Must be between 0 and 
///                             XCHAL_NUM_INTERRUPTS-1, inclusive.  Associated
///                             interrupt level must be <= EXCM_LEVEL.  Any 
///                             of these interrupts can be delegated to 
///                             nonsecure code.
///
///  \param     handler         Pointer to interrupt handler function.  Must 
///                             reside in non-secure memory.
///
///  \param     arg             Optional argument passed to handler.
///
///  \return    0 on success, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xtsm_set_interrupt_handler(uint32_t intnum, xtsm_handler_t *handler, void *arg);


//-----------------------------------------------------------------------------
///
///  Register exception for nonsecure handling.
///
///  NOTE: Handling nonsecure interrupt and exception requests from the 
///  secure monitor currently leverages XTOS handlers and API calls.  If the 
///  application provides its own handling, a replacement "virtual vector" 
///  must be provided (replacing lib/user-vector-nsm.S) and registered with 
///  the secure monitor.
///
///  \param     excnum          Xtensa exception number to be serviced by
///                             non-secure code.  Must be between 0 and 
///                             XCHAL_EXCCAUSE_NUM-1, inclusive.  The secure 
///                             monitor may allow delegation of some exceptions 
///                             to nonsecure code while preventing others.
///
///  \param     handler         Pointer to exception handler function.  Must 
///                             reside in non-secure memory.
///
///  \return    0 on success, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xtsm_set_exception_handler(uint32_t excnum, xtsm_handler_t *handler);


//-----------------------------------------------------------------------------
///
///  Register SYSCALL exception for nonsecure handling.
///
///  All system calls pass through the secure handler first, and any that go
///  unprocessed can be optionally routed to a nonsecure handler via
///  registration by this function.  If a nonsecure handler is not installed
///  or is uninstalled, secure syscall processing will return -1 for unhandled
///  syscall requests.
///
///  \param     handler         Nonsecure system call handler to install.  If
///                             NULL, nonsecure syscall handler will be removed.
///
///                             The XTSM_SYSCALL_HANDLER_DEFAULT handler may be 
///                             used for convenience for default XTOS handling.
///
///                             NOTE: If provided, syscall_handler() must be
///                             implemented in assembly.
///
///  \return    0 on success, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xtsm_set_syscall_handler(xtsm_handler_t *handler);


#ifdef __cplusplus
}
#endif

#endif  // __SECMON_H

