
// xos_int_stack.S - Allocates space for the XOS interrupt stack.

// Copyright (c) 2015-2020 Cadence Design Systems, Inc.

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


// This is in a separate file so it can be easily overridden.
// 
// To provide a larger or smaller interrupt stack without rebuilding
// the XOS library, define your own stack area with the same name in
// your application. The definition in this file is weak, so the one
// in your application will override it.


#include "xos_common.h"


//-----------------------------------------------------------------------------
// This file is used only for XEA2 configurations. In XEA3 the interrupt stack
// and the interrupt handler table must reside together in most configurations.
// See xos_int_table.S.
//-----------------------------------------------------------------------------

#if XCHAL_HAVE_XEA2

//-----------------------------------------------------------------------------
// The interrupt stack has to exist even if no interrupts are configured
// because the idle thread uses the interrupt stack. The default size is
// much smaller if interrupts are not configured.
//-----------------------------------------------------------------------------

    .section    .bss, "aw"
    .align      16
    .global     xos_int_stack
    .weak       xos_int_stack
    .global     xos_int_stack_end
    .weak       xos_int_stack_end

xos_int_stack:
    .space      XOS_INT_STACK_SIZE
xos_int_stack_end:

#endif

