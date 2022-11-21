
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

// This example uses mutexes from user mode.


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

    printf("mutex example: arg 0x%08x\n", arg);

    // Code section
    printf("  addr of this function     -- 0x%08x\n", &_start);
    // Stack section
    printf("  addr of local variable x  -- 0x%08x\n", &x);
    // Data section
    printf("  addr of static variable y -- 0x%08x\n", &y);

    xos_umutex mtx;
    int32_t    ret = xos_umutex_create(0x123456780000ULL, 8, &mtx);

    printf("%c: create ret = %d, mtx = 0x%08x\n", id, ret, mtx);
    if (ret != 0) {
        return ret;
    }

    for (x = 0; x < 10; x++) {
        ret = xos_umutex_lock(mtx);
        printf("%c: lock\n", id);
        ret = xos_umutex_unlock(mtx);
        printf("%c: unlock\n", id);
    }

    ret = xos_umutex_delete(mtx);
    printf("%c: delete ret = %d\n", id, ret);

    return ret;
}

