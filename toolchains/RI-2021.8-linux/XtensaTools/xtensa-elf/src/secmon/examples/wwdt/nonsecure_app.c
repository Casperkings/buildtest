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

#include <xtensa/config/secure.h>
#include <xtensa/core-macros.h>
#include <xtensa/xtruntime.h>
#include <stdio.h>

#include "wwdt.h"


int main(int argc, char **argv)
{
    int32_t ret;

    UNUSED(argc);
    UNUSED(argv);

    printf("Hello, nonsecure world\n");
    ret = xtsm_init();
    printf("xtsm_init(): %s\n", (ret == 0) ? "OK" : "Error");

    printf("Testing WWDT in nonsecure mode\n");
    ret = test_xthal_wwdt_initialize(WWDT_INITIAL, WWDT_BOUND, WWDT_RESET);
    printf("WWDT test %s (%d)\n", (ret == WWDT_TEST_OK) ? "PASSED" : "FAILED", ret);
    return (ret == WWDT_TEST_OK) ? 0 : -1;
}

