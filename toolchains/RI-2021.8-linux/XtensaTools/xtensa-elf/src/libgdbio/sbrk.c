/* sbrk.c -- allocate memory dynamically
 *
 * Copyright (c) 2004, 2005, 2006 by Tensilica Inc.  ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of Tensilica Inc.
 * They may not be modified, copied, reproduced, distributed, or disclosed to
 * third parties in any manner, medium, or form, in whole or in part, without
 * the prior written consent of Tensilica Inc.
 */

#include "libio.h"


/* Define a weak symbol to indicate the presence of an OS. Normally this
   symbol will not be defined, so the value of "os_flag" will be zero.
   An OS can define this to a nonzero value to indicate its presence.
 */
#pragma weak __os_flag
extern char  __os_flag;

extern char _end, _heap_sentry, __stack; /* defined by the linker */

static const void * os_flag = &__os_flag;
static char * heap_end = &_end;

char *_heap_sentry_ptr = &_heap_sentry; /* can be overridden */


void * _sbrk_r (struct _reent *reent, int incr)
{
    void * prev_heap_end;
    char * stack_ptr;

    prev_heap_end = heap_end;

    /* We have a heap and stack collision if the stack started above the
       heap and is now below the end of the heap.  */
    /* Do stack checking only if os_flag is zero. Nonzero value indicates
       stack is application-managed (e.g. by an RTOS).  */
    if (os_flag == 0) {
        __asm__ ("addi %0, sp, 0" : "=a" (stack_ptr));
        if (&__stack > &_end && heap_end + incr > stack_ptr) {
            reent->_errno = ENOMEM;
            return (void *) -1;
        }
    }

    /* The heap is limited by _heap_sentry_ptr -- we're out of memory if we
       exceed that.  */
    if (heap_end + incr >= _heap_sentry_ptr) {
        reent->_errno = ENOMEM;
        return (void *) -1;
    }

    heap_end += incr;
    return prev_heap_end;
}

