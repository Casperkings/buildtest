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
#include "xtensa/secmon-common.h"

#include <xtensa/core-macros.h>
#include <xtensa/xtruntime.h>

#if !(defined XCHAL_NUM_MISC_REGS) || (XCHAL_NUM_MISC_REGS <= 1)
#error Secure LX configs must have MISC registers defined
#endif


static void secure_l32_handler(void *arg)
{
    UserFrame * ef = (UserFrame *)arg;

    // Check for L32I.N encoding; else assume 24-bit instruction
#if (XCHAL_HAVE_BE == 1)
    // Account for field swap in low byte
    uint8_t instr_width = (((*((uint8_t *)ef->pc) >> 4) & 0xf) == 0x8) ? 2 : 3;
#else
    uint8_t instr_width = ((*((uint8_t *)ef->pc) & 0xf) == 0x8) ? 2 : 3;
#endif

    // Skip past offending load instruction
    ef->pc += instr_width;

    XT_WSR_MISC1(XT_RSR_MISC1() + 1);
}

static void secure_s32_handler(void *arg)
{
    UserFrame * ef = (UserFrame *)arg;

    // Check for S32I.N encoding; else assume 24-bit instruction
#if (XCHAL_HAVE_BE == 1)
    // Account for field swap in low byte
    uint8_t instr_width = (((*((uint8_t *)ef->pc) >> 4) & 0xf) == 0x9) ? 2 : 3;
#else
    uint8_t instr_width = ((*((uint8_t *)ef->pc) & 0xf) == 0x9) ? 2 : 3;
#endif

    // Skip past offending store instruction
    ef->pc += instr_width;

    XT_WSR_MISC1(XT_RSR_MISC1() + 1);
}


int32_t secmon_user_init(void)
{
    int32_t ret;
    int32_t stack_addr = (uint32_t)&ret;
    int32_t func_addr = (uint32_t)secure_l32_handler;
    int32_t status = 1;

    if (!xtsm_secure_addr_overlap(stack_addr, stack_addr + 4) ||
        !xtsm_secure_addr_overlap(func_addr, func_addr + 4)) {
        // Flag a failure if stack or text are in non-secure space
        status = -1;
    }

    // Handle protection violation exceptions
    if (xtos_set_exception_handler(EXCCAUSE_LOAD_PROHIBITED, secure_l32_handler, NULL)) {
        status = -2;
    } else if (xtos_set_exception_handler(EXCCAUSE_STORE_PROHIBITED, secure_s32_handler, NULL)) {
        status = -3;
    }

    // Negative values are failures, positive values are passes, and 
    // 0 indicates nothing was updated.  Note that this test will not
    // work with XMON (as misc0 is used there as well).
    XT_WSR_MISC0(status);
    XT_WSR_MISC1(0);
    return 0;
}
