// exc_table.c -- Exception Handler Table for XEA5

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


#include <xtensa/config/system.h>
#include <xtensa/xtruntime.h>


/* Table of exception handlers, listed by exception cause. */

extern xtos_handler xtos_exc_handler_table[XCHAL_EXCCAUSE_NUM];
extern void         xtos_default_exc_handler(void * ef);

xtos_handler xtos_exc_handler_table[XCHAL_EXCCAUSE_NUM] = {
    xtos_default_exc_handler,    // Instruction address misaligned
    xtos_default_exc_handler,    // Instruction access fault
    xtos_default_exc_handler,    // Illegal instruction
    xtos_default_exc_handler,    // Breakpoint
    xtos_default_exc_handler,    // Load address misaligned
    xtos_default_exc_handler,    // Load access fault
    xtos_default_exc_handler,    // Store address misaligned
    xtos_default_exc_handler,    // Store access fault
    xtos_default_exc_handler,    // U-mode env call
    xtos_default_exc_handler,    // S-mode env call
    xtos_default_exc_handler,    // Reserved
    xtos_default_exc_handler,    // M-mode env call
    xtos_default_exc_handler,    // Instruction page fault
    xtos_default_exc_handler,    // Load page fault
    xtos_default_exc_handler,    // Reserved
    xtos_default_exc_handler,    // Store page fault
};

