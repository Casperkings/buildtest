
// Copyright (c) 2001-2020 Cadence Design Systems, Inc.
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


#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define XTUTIL_LIB
#include "xtutil.h"


//-----------------------------------------------------------------------------
//  Local typedefs and data.
//-----------------------------------------------------------------------------

typedef struct me {
    void * p;
    size_t s;
} me;

static struct me * mem_table;
static uint32_t    mem_table_size = 32;
static size_t      mem_use_cur;
static size_t      mem_use_max;
static uint32_t    mem_use_count;
static uint32_t    n;


//-----------------------------------------------------------------------------
//  Insert a new pointer/size pair into tracking list. Expand list if needed.
//-----------------------------------------------------------------------------
static void
insert(void * p, size_t s)
{
    uint32_t i;

    if (mem_use_count == mem_table_size) {
        struct me * pme = calloc(mem_table_size * 2, sizeof(struct me));

        if (pme != NULL) {
            memcpy(pme, mem_table, mem_table_size * sizeof(struct me));
            free(mem_table);
            mem_table = pme;
            mem_table_size *= 2;
        }
        else {
            xt_puts("libxtutil: out of memory\n");
            exit(-1);
        }
    }

    // Search ahead from last point.
    for (i = n; i < mem_table_size; i++) {
        if (mem_table[i].p == NULL) {
            mem_table[i].p = p;
            mem_table[i].s = s;
            mem_use_count++;
            n = i + 1;
            break;
        }
    }

    // Search rest of table.
    if (i == mem_table_size) {
        for (i = 0; i < n; i++) {
            if (mem_table[i].p == NULL) {
                mem_table[i].p = p;
                mem_table[i].s = s;
                mem_use_count++;
                n = i + 1;
                break;
            }
        }
    }

    if (i == n) {
        // Should never happen
        xt_puts("libxtutil: internal error\n");
        exit(-1);
    }
    else {
        mem_use_cur += s;
        if (mem_use_cur > mem_use_max) {
            mem_use_max = mem_use_cur;
        }
#ifdef DEBUG
        xt_printf("adding (%p - %d) total %d\n", p, s, mem_use_cur);
#endif
    }
}


//-----------------------------------------------------------------------------
//  Remove an entry from the tracking table.
//-----------------------------------------------------------------------------
static void
remove(void * p)
{
    uint32_t i;

    for (i = 0; i < mem_table_size; i++) {
        if (mem_table[i].p == p) {
            mem_use_cur -= mem_table[i].s;
            mem_table[i].p = NULL;
            mem_table[i].s = 0;
            mem_use_count--;
            n = i;
#ifdef DEBUG
            xt_printf("removing (%p - %d) total %d\n", p, s, mem_use_cur);
#endif
            break;
        }
    }
}


//-----------------------------------------------------------------------------
//  xt_calloc.
//-----------------------------------------------------------------------------
void *
xt_calloc(size_t nmemb, size_t size)
{
    void * p = calloc(nmemb, size);

    if (p) {
        insert(p, nmemb * size);
    }

    return p;
}


//-----------------------------------------------------------------------------
//  xt_malloc.
//-----------------------------------------------------------------------------
void *
xt_malloc(size_t size)
{
    void * p = malloc(size);

    if (p) {
        insert(p, size);
    }

    return p;
}


//-----------------------------------------------------------------------------
//  xt_free.
//-----------------------------------------------------------------------------
void
xt_free(void * ptr)
{
    free(ptr);
    if (ptr) {
        remove(ptr);
    }
}


//-----------------------------------------------------------------------------
//  xt_realloc.
//-----------------------------------------------------------------------------
void *
xt_realloc(void * ptr, size_t size)
{
    void * p = realloc(ptr, size);

    if (p) {
        remove(ptr);
        if (size > 0) {
            insert(p, size);
        }
    }
    else {
        if (size == 0) {
            remove(ptr);
        }
    }

    return p;
}


//-----------------------------------------------------------------------------
//  Report current allocated amount and max that was allocated at any time up
//  to now.
//-----------------------------------------------------------------------------
void
xt_mt_report(size_t * curr, size_t * max)
{
#ifdef DEBUG
    int32_t i;

    for (i = 0; i < mem_table_size; i++) {
        if (mem_table[i].p != NULL) {
            xt_printf(" %p -> %d\n", mem_table[i].p, mem_table[i].s);
        }
    }
#endif

    if (curr) {
        *curr = mem_use_cur;
    }
    if (max) {
        *max = mem_use_max;
    }
}


//-----------------------------------------------------------------------------
//  Release tracking table memory and report any application leaks.
//-----------------------------------------------------------------------------
static void
xt_mtrack_fini(void)
{
    if (mem_use_count > 0) {
        uint32_t i;

        for (i = 0; i < mem_table_size; i++) {
            if (mem_table[i].p != NULL) {
                xt_printf("leak: 0x%08x [%d]\n", mem_table[i].p, mem_table[i].s);
            }
        }
    }

    free(mem_table);
}


//-----------------------------------------------------------------------------
//  Init function, allocate tracking table and register atexit function.
//-----------------------------------------------------------------------------
__attribute__((constructor)) static void
xt_mtrack_init(void)
{
    mem_table = calloc(mem_table_size, sizeof(struct me));
    n = 0;
    mem_use_count = 0;

    atexit(xt_mtrack_fini);
}

