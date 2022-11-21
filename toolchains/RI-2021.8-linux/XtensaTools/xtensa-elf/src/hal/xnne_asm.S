// Copyright (c) 2019 Cadence Design Systems
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
#include <xtensa/xnne.h>

/*
	Generates a set of symbols to aide in XNNE debugging (where N is the
	index of the SBLK of interest):
 	xnne_base_
	xnne_sblkN_base_
	xnne_sblkN_cbuf_base_
	xnne_sblkN_dram_base_
	xnne_sblkN_ibuf_base_
	xnne_sblkN_iram_base_
	xnne_sblkN_obuf_base_
	xnne_sblkN_vpu_base_

	The recommended use model is:
	(xt-gdb) x/256x &xnne_sblk0_vpu_base
	(where 256 is the number of words to display)

	These symbols are for XNNE debugging purposes only */

#if XCHAL_HAVE_XNNE

#define DEBUG_CONSTANT(name, val) \
  	.globl name; \
  	.set name, val;

#if XCHAL_XNNE_VERSION >= 2
#define DEBUG_CONSTANTS_FOR_SBLK(i) \
    DEBUG_CONSTANT(xnne_sblk ## i ##_base_, XNNE_SBLK_BASE(i)) \
    DEBUG_CONSTANT(xnne_sblk ## i ## _ubuf_base_, XNNE_SBLK_UBUF_BASE(i)) \
    DEBUG_CONSTANT(xnne_sblk ## i ## _vpu_base_, XNNE_SBLK_VPU_BASE(i)) \
    DEBUG_CONSTANT(xnne_sblk ## i ## _iram_base_, XNNE_SBLK_IRAM_BASE(i)) \
    DEBUG_CONSTANT(xnne_sblk ## i ## _dram_base_, XNNE_SBLK_DRAM_BASE(i))
#else
#define DEBUG_CONSTANTS_FOR_SBLK(i) \
    DEBUG_CONSTANT(xnne_sblk ## i ##_base_, XNNE_SBLK_BASE(i)) \
    DEBUG_CONSTANT(xnne_sblk ## i ## _ibuf_base_, XNNE_SBLK_IBUF_BASE(i)) \
    DEBUG_CONSTANT(xnne_sblk ## i ## _cbuf_base_, XNNE_SBLK_CBUF_BASE(i)) \
    DEBUG_CONSTANT(xnne_sblk ## i ## _obuf_base_, XNNE_SBLK_OBUF_BASE(i)) \
    DEBUG_CONSTANT(xnne_sblk ## i ## _vpu_base_, XNNE_SBLK_VPU_BASE(i)) \
    DEBUG_CONSTANT(xnne_sblk ## i ## _iram_base_, XNNE_SBLK_IRAM_BASE(i)) \
    DEBUG_CONSTANT(xnne_sblk ## i ## _dram_base_, XNNE_SBLK_DRAM_BASE(i))
#endif

DEBUG_CONSTANT(xnne_base_, XNNE_BASE)
DEBUG_CONSTANTS_FOR_SBLK(0)
#if XCHAL_XNNE_NUM_SBLKS > 1
DEBUG_CONSTANTS_FOR_SBLK(1)
#if XCHAL_XNNE_NUM_SBLKS > 2
DEBUG_CONSTANTS_FOR_SBLK(2)
#if XCHAL_XNNE_NUM_SBLKS > 3
DEBUG_CONSTANTS_FOR_SBLK(3)
#endif
#endif
#endif
#endif
