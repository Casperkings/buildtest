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
#include "xtensa/secmon-common.h"

#include <xtensa/config/secure.h>
#include <xtensa/config/system.h>
#include <xtensa/core-macros.h>
#include <xtensa/corebits.h>
#include <xtensa/xtruntime.h>
#include <stdio.h>


static int32_t mpu_nswren_test(void)
{
    uint32_t mpucfg = 0;
    uint32_t orig_mpucfg;
    int32_t  wrdis_set, ret;

    __asm__ __volatile__ ("rsr.mpucfg %0" : "=a"(orig_mpucfg));

    // Validate MPUCFG.NSWRDIS is clear after secure boot
    wrdis_set = ((orig_mpucfg >> XTHAL_MPUCFG_NSWRDIR_SHIFT) & XTHAL_MPUCFG_NSWRDIR_MASK);
    printf("NSWRDIS bit is %sset, %s\n", wrdis_set ? "" : "NOT ", wrdis_set ? "FAILED" : "OK");
    if (wrdis_set) {
        return -1;
    }

    // Validate MPUCFG.NSWRDIS is changeable 
    mpucfg = orig_mpucfg | (XTHAL_MPUCFG_NSWRDIR_MASK << XTHAL_MPUCFG_NSWRDIR_SHIFT);
    __asm__ __volatile__ ("wsr.mpucfg %0" :: "a"(mpucfg));
    __asm__ __volatile__ ("rsr.mpucfg %0" : "=a"(mpucfg));

    ret = (mpucfg == orig_mpucfg);
    printf("MPUCFG.NSWRDIS is %schangeable; %s\n",
            (mpucfg == orig_mpucfg) ? "NOT " : "",
            ret ? "FAILED" : "OK");
    return ret;
}


int main(int argc, char **argv)
{
    int ret = 0;
    UNUSED(argc);
    UNUSED(argv);

    printf("Hello, nonsecure world\n");
    ret = xtsm_init();
    if (ret != 0) {
        printf("Error: xtsm_init() FAILED (%d)\n", ret);
        return ret;
    }

    ret = mpu_nswren_test();
    printf("MPU NSWREN test %s\n", ret ? "FAILED" : "OK");

    printf("\nOverall test result: %s\n", ret ? "FAILED" : "PASSED");
    return ret;
}

