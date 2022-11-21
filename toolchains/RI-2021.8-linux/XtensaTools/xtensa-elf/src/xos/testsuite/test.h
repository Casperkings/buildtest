
/* test.h - Definitions for testing */

/*
 * Copyright (c) 2015 Cadence Design Systems, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#include <xtensa/config/core.h>

#if defined BOARD
#include <xtensa/xtbsp.h>
#endif

#define STACK_SIZE	(XOS_STACK_MIN_SIZE + 1024)

#define SW_L1INT_MASK	(XCHAL_INTLEVEL1_MASK & XCHAL_INTTYPE_MASK_SOFTWARE)
#define SW_L1INT_BIT	(SW_L1INT_MASK & -SW_L1INT_MASK)

/*
#if SW_L1INT_MASK == 0
# error "Need at least one level-one software interrupt to do trampolining..."
#endif
 */


#define TICK_CYCLES	(xos_get_clock_freq()/100)

//#define TICK_INTERVAL   10000            /* in cycles */
#define TIMER_INTERVAL  0x1423          /* ... */


#ifndef __ASSEMBLER__

#include <stdlib.h>
#include <stdio.h>

/* We want to include the local copy of xos.h */
#include "xtensa/xos.h"

#include <xtensa/xtutil.h>

/*  Invoke this as soon as a test fails.  */
#define FAIL(msg...)	do{ fail(); printf(msg); fail_loop(); }while(0)

/*  Null function where one can set a breakpoint:  */
extern void fail(void);
/*  Exit or infinite loop or ??? function ...:  */
extern void fail_loop(void);

/*  List threads using printf:  */
extern void xos_list_threads(void);

#endif /* __ASSEMBLER__ */

