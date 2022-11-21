
// Copyright (c) 2021 Cadence Design Systems, Inc.
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


// DO NOT try dynamic memory allocation in this library.
// There is no heap provided for position-independent libraries.

// Example showing semaphore use from user mode.


#include <stdint.h>
#include <stdlib.h>

#include <xt_mld_loader.h>
#include <xtensa/xtutil.h>

#include "../src/xos_syscall.h"


// Specify section attributes for code and data

XTMLD_DATA_MEM_REQ(xtmld_load_normal, xtmld_load_normal, xtmld_load_normal, xtmld_load_normal);
XTMLD_CODE_MEM_REQ(xtmld_load_normal, xtmld_load_normal, xtmld_load_normal, xtmld_load_normal);

// Static data

uint32_t y;


// Thread entry point (also library entry point). You could have
// this function return a pointer to another thread entry point,
// or to something else. The caller has to know what the inputs
// and outputs of the _start function are.

int32_t
_start(void * arg)
{
    char     id = (char) arg;
    uint32_t x;

    printf("lib1: do_test called: arg 0x%08x\n", arg);

    // Code section
    printf("  addr of this function     -- 0x%08x\n", &_start);
    // Stack section
    printf("  addr of local variable x  -- 0x%08x\n", &x);
    // Data section
    printf("  addr of static variable y -- 0x%08x\n", &y);

    xos_usem sem;
    int32_t  ret;

    switch (id) {
    case 'A':
    case 'B':
        ret = xos_usem_create(0x1111000011110000ULL, 0, &sem);
        printf("create ret = %d, sem = 0x%08x\n", ret, sem);
        break;

    case 'C':
    case 'D':
        ret = xos_usem_create(0x2222000022220000, 0, &sem);
        printf("create ret = %d, sem = 0x%08x\n", ret, sem);
        break;

    default:
        ret = -1;
        break;
    }

    if (ret != 0) {
        return ret;
    }

    switch (id) {
    case 'A':
    case 'C':
        for (x = 0; x < 10; x++) {
            ret = xos_usem_get(sem);
        }
        break;

    case 'B':
    case 'D':
        for (x = 0; x < 10; x++) {
            ret = xos_usem_put(sem);
        }
        break;

    default:
        break;
    }

    ret = xos_usem_delete(sem);
    printf("delete ret = %d\n", ret);

    return ret;
}

