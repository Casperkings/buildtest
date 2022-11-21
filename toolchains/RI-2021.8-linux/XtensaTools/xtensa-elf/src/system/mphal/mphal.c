/*  mphal.c - Multi-Processor HAL global constants  */

/* $Id: //depot/rel/Homewood/ib.8/Xtensa/OS/system/mphal/mphal.c#1 $ */

/* Copyright (c) 2005-2006 Tensilica Inc.

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
   CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.  */

#include <xtensa/mpsystem.h>
#include <xtensa/hal.h>

/*  Various arrays:  */
char _Xmchal_max_instruction_size[] = { XMCHAL_ARRAY_P(MAX_INSTRUCTION_SIZE) };
char _Xmchal_have_big_endian[] = { XMCHAL_ARRAY_P(HAVE_BIG_ENDIAN) };
char _Xmchal_have_debug[] = { XMCHAL_ARRAY_P(HAVE_DEBUG) };
char _Xmchal_have_xeax[] = { XMCHAL_ARRAY_P(HAVE_XEAX) };
char _Xmchal_debug_level[] = { XMCHAL_ARRAY_P(DEBUG_LEVEL) };
unsigned _Xmchal_reset_vector_vaddr[] = { XMCHAL_ARRAY_P(RESET_VECTOR_VADDR) };
#if XCHAL_HAVE_L2
XCHAL_L2_SETUP(XMCHAL_L2RAM_START, XMCHAL_L2RAMFRAC, XMCHAL_L2CACHE_FRAC);
#endif
