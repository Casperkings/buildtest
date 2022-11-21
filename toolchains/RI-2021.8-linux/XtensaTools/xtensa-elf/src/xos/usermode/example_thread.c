
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


// Simple dynamic library to demonstrate user-mode threads. The library
// is loaded and the first thread created by the test program. Then the
// first thread creates another thread using syscalls.

// DO NOT try dynamic memory allocation in this library.
// There is no heap provided for position-independent libraries.


#include <stdint.h>
#include <stdlib.h>

#include <xt_mld_loader.h>
#include <xtensa/xtutil.h>

#include "../src/xos_syscall.h"

// Specify section attributes for code and data

XTMLD_DATA_MEM_REQ(xtmld_load_normal, xtmld_load_normal, xtmld_load_normal, xtmld_load_normal);
XTMLD_CODE_MEM_REQ(xtmld_load_normal, xtmld_load_normal, xtmld_load_normal, xtmld_load_normal);

// Static data

volatile int32_t flag;

#define STACK_SIZE 8192

char tstack[STACK_SIZE]; // thread stack

// Second thread entry point.

int32_t
thread_func(void * arg)
{
    volatile int32_t * p = (volatile int32_t *) arg;

    printf("lib child thread (%d)\n", *p);
    *p += 10;
    printf("*arg now %d\n", *p);

    return *p;
}


// Thread entry point (also library entry point). You could have
// this function return a pointer to another thread entry point,
// or to something else. The caller has to know what the inputs
// and outputs of the _start function are.

int32_t
_start(void * arg)
{
    xos_uthread_params params;
    xos_uthread        thd;

    char    name[16];
    int32_t ret;
    int32_t rc;
    int32_t id = (int32_t) arg;

    printf("lib main thread (%d)\n", id);
    sprintf(name, "Thread_%d", id);

    params.entry = thread_func;
    params.arg   = (void *) &flag;
    params.name  = name;
    params.stack = tstack;
    params.stack_size = STACK_SIZE;
    params.priority = 7;
    params.flags = 0;

    flag = (int32_t) arg;

    ret = xos_uthread_create(&params, &thd);
    if (ret != 0) {
        printf("Error creating thread: %d\n", ret);
        return ret;
    }
    printf("handle = 0x%08x\n", thd);

    while (flag == (int32_t) arg);

    ret = xos_uthread_join(thd, &rc);
    printf("retval exp %d, got %d\n", flag, rc);

    printf("lib main thread done (%d)\n", id);
    return 0;
}

