// Copyright (c) 2020-2021 Cadence Design Systems, Inc.
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

// secure_user_init.c -- Secure monitor example code


#include "xtensa/secmon-secure.h"

#include <xtensa/core-macros.h>
#include <xtensa/xtruntime.h>


static void unaligned_loadstore_handler(void *arg)
{
    UserFrame * ef = (UserFrame *)arg;

    // Check for L32I.N encoding; else assume 24-bit instruction
    uint8_t instr_width = ((*((uint8_t *)ef->pc) & 0xf) == 0x8) ? 2 : 3;

    // Skip past offending illegal instruction
    ef->pc += instr_width;
}


int32_t secmon_user_init()
{
    // Allow a couple of exceptions to be delegatable to nonsecure code
    secmon_nonsecure_exceptions[EXCCAUSE_ILLEGAL]   = 1;
    secmon_nonsecure_exceptions[EXCCAUSE_UNALIGNED] = 1;

    // Install secure exception handler for EXCCAUSE_UNALIGNED for test purposes
    return xtos_set_exception_handler(EXCCAUSE_UNALIGNED, unaligned_loadstore_handler, NULL);
}
