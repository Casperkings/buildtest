// 
// debug.c - debug related constants and functions
//
// $Id: //depot/rel/Homewood/ib.8/Xtensa/OS/hal/not_certified/debug.c#1 $

// Copyright (c) 2002 Tensilica Inc.
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

#include <xtensa/hal.h>
#include <xtensa/config/core.h>

/*  1 if debug option configured, 0 if not:  */
const int32_t Xthal_debug_configured = XCHAL_HAVE_DEBUG;

/*  Number of instruction and data break registers:  */
const int32_t Xthal_num_ibreak = XCHAL_NUM_IBREAK;
const int32_t Xthal_num_dbreak = XCHAL_NUM_DBREAK;

#define XTHAL_24_BIT_BREAK		UINT32_C(0x80000000)
#define XTHAL_16_BIT_BREAK		UINT32_C(0x40000000)

// set software breakpoint and synchronize cache
uint32_t
xthal_set_soft_break(void *addr)
{
    uint32_t inst;
    uint8_t *baddr = (uint8_t *) addr; // parasoft-suppress MISRA2012-RULE-11_5-4 "Conversion needed"
    _Bool is24bit = (xthal_disassemble_size(baddr) == 3);
    uint32_t ret_val;

#if XCHAL_HAVE_BE
    inst =  (((uint32_t)baddr[0]) << 24) +
            (((uint32_t)baddr[1]) << 16) +
            (((uint32_t)baddr[2]) << 8);
#else
    inst =  (((uint32_t)baddr[0])) +
            (((uint32_t)baddr[1]) << 8) +
            (((uint32_t)baddr[2]) << 16);
#endif
#if XCHAL_HAVE_BE
    if (is24bit != 0) {
	ret_val = XTHAL_24_BIT_BREAK & ((inst>>8)& UINT32_C(0xffffff));
	(baddr)[0] = 0x00;
	(baddr)[1] = 0x04;
	(baddr)[2] = 0x00;
    } else {
	ret_val = XTHAL_16_BIT_BREAK & ((inst>>16)& UINT32_C(0xffff));
	(baddr)[0] = 0xD2;
	(baddr)[1] = 0x0f;
    }
#else
    if (is24bit != 0) {
	ret_val = XTHAL_24_BIT_BREAK & (inst & UINT32_C(0xffffff));
	(baddr)[0] = 0x00;
	(baddr)[1] = 0x40;
	(baddr)[2] = 0x00;
    } else {
	ret_val = XTHAL_16_BIT_BREAK & (inst & UINT32_C(0xffff));
	(baddr)[0] = 0x2D;
	(baddr)[1] = 0xf0;
    }
#endif
    *((uint32_t *)addr) = inst; // parasoft-suppress MISRA2012-RULE-11_5-4 "Conversion needed"
#if XCHAL_DCACHE_IS_WRITEBACK
    xthal_dcache_region_writeback(addr, 3);
#endif
#if XCHAL_ICACHE_SIZE > 0
    xthal_icache_region_invalidate(addr, 3);
#endif
    return ret_val;
}


// remove software breakpoint and synchronize cache
void
xthal_remove_soft_break(void *addr, uint32_t inst)
{
#if XCHAL_HAVE_BE
    if ((inst & XTHAL_24_BIT_BREAK) != 0) {
	((uint8_t *)addr)[0] = (inst>>16)&UINT32_C(0xff);
	((uint8_t *)addr)[1] = (inst>>8)&UINT32_C(0xff);
	((uint8_t *)addr)[2] = inst&UINT32_C(0xff);
    } else {
	((uint8_t *)addr)[0] = (inst>>8)&UINT32_C(0xff);
	((uint8_t *)addr)[1] = inst&UINT32_C(0xff);
    }
#else
    ((uint8_t *)addr)[0] = (uint8_t) (inst & UINT32_C(0xff));
    ((uint8_t *)addr)[1] = (uint8_t) ( (inst>>8) & UINT32_C(0xff));
    if ((inst & XTHAL_24_BIT_BREAK) != UINT32_C(0)) {
	((uint8_t *)addr)[2] = (uint8_t) ((inst>>16) & UINT32_C(0xff));
    }
#endif
#if XCHAL_DCACHE_IS_WRITEBACK
    xthal_dcache_region_writeback((void*)addr, 3);
#endif
#if XCHAL_ICACHE_SIZE > 0
    xthal_icache_region_invalidate((void*)addr, 3);
#endif
}

