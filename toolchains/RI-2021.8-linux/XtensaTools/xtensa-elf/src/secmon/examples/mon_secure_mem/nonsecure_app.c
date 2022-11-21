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

// nonsecure_app.c -- Nonsecure application example code


#include "xtensa/secmon.h"

#include <xtensa/core-macros.h>
#include <xtensa/xtruntime.h>
#include <xtensa/hal.h>
#include <xtensa/config/secure.h>
#include <stdio.h>


#define UNUSED(x) ((void)(x))

static volatile uint32_t *addr = 0;
static uint32_t val;

static int32_t test_nsm(void)
{
    xthal_MPU_entry fg[XCHAL_MPU_ENTRIES];
    int32_t errors = 0;
    int32_t i, old_misc1, new_misc1;

    printf("Nonsecure memory tests running...\n");
    XT_WSR_MISC1(0);
    if (xthal_read_map(fg) != XTHAL_SUCCESS) {
        printf("ERROR: MPU read failed\n");
        return -1;
    }

    for (i = 0; i < XCHAL_MPU_ENTRIES; i++) {
        uint32_t is_secure = 0;
        if (XTHAL_MPU_ENTRY_GET_VALID(fg[i])) {
            addr = (volatile uint32_t *)XTHAL_MPU_ENTRY_GET_VSTARTADDR(fg[i]);
            is_secure = XTHAL_MPU_ENTRY_GET_LOCK(fg[i]);
#if (defined SIMULATOR)
#if (defined XCHAL_HAVE_SECURE_SRAM) && (XCHAL_HAVE_SECURE_SRAM == 1)
            if ((unsigned int)addr == XCHAL_SRAM_SECURE_START) {
                printf("ISS doesn't have a secure system memory responder; skipping SRAM...\n");
                continue;
            }
#endif
#if (defined XCHAL_HAVE_SECURE_SROM) && (XCHAL_HAVE_SECURE_SROM == 1)
            if ((unsigned int)addr == XCHAL_SROM_SECURE_START) {
                printf("ISS doesn't have a secure system memory responder; skipping SROM...\n");
                continue;
            }
#endif
#endif
            if (xthal_is_kernel_readable(XTHAL_MPU_ENTRY_GET_ACCESS(fg[i]))) {
                old_misc1 = XT_RSR_MISC1();
                val = *addr;
                new_misc1 = XT_RSR_MISC1();
                if ((is_secure && (old_misc1 + 1 != new_misc1)) ||
                    (!is_secure && (old_misc1 != new_misc1))) {
                    printf("ERROR: address %p is %ssecure, read triggered %d secure exceptions\n",
                            (volatile void *)addr, is_secure ? "" : "non-", new_misc1 - old_misc1);
                    errors++;
                } else {
                    printf("INFO: address %p is %ssecure, %d total secure exceptions\n",
                            (volatile void *)addr, is_secure ? "" : "non-", new_misc1);
                }
            }
            if (xthal_is_kernel_writeable(XTHAL_MPU_ENTRY_GET_ACCESS(fg[i]))) {
                old_misc1 = XT_RSR_MISC1();
                *addr = val;
                new_misc1 = XT_RSR_MISC1();
                if ((is_secure && (old_misc1 + 1 != new_misc1)) ||
                    (!is_secure && (old_misc1 != new_misc1))) {
                    printf("ERROR: address %p is %ssecure, write triggered %d secure exceptions\n",
                            (volatile void *)addr, is_secure ? "" : "non-", new_misc1 - old_misc1);
                    errors++;
                } else {
                    printf("INFO: address %p is %ssecure, %d total secure exceptions\n",
                            (volatile void *)addr, is_secure ? "" : "non-", new_misc1);
                }
            }
        }
    }

    printf("Nonsecure memory tests %s: %d errros\n", errors ? "FAILED" : "OK", errors);
    return errors;
}

int main(int argc, char **argv)
{
    int32_t ret, ret2, misc0;

    UNUSED(argc);
    UNUSED(argv);

    printf("Hello, nonsecure world\n");
    misc0 = XT_RSR_MISC0();
    ret = (misc0 > 0) ? 0 : 1;
    printf("Secure memory test %s (misc0 : %d)\n", ret ? "FAILED" : "OK", misc0);

    ret2 = test_nsm();

    printf("Test %s\n", (ret || ret2) ? "FAILED" : "PASSED");
    return ret;
}

