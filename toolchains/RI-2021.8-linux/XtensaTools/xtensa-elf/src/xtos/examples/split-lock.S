/* split-lock.S - example code for clearing cache and local memories for
   Split-Lock with MEM_LOCKSTEP */

/*
 * Copyright (c) 2021 Cadence Inc.
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

/* Split-Lock with MEM_LOCKSTEP requires the local memories and caches to be
   initialized indentically on both the main and checker cores.

   Note that not only the cache tags, but also the cache data needs to be
   initialized.

   Depending on the capabilities of the hardware, various initialization
   approaches can be used.

   In this example a software only initialization is done by writing a known
   value to all local memory locations and by using the SICT, SICW, SDCT, and
   SDCW instructions to initialize the cache tags and data.

   This initialization is done at reset time before the caches are enabled
   by using the __reset_hook.

   This example assumes the availability of cache test instructions, and may
   need to be customized.
   */


/* Use cache test instructions to clear the data cache (both tags and data).
   This code assumes the cache is disabled. */
.macro clr_dcache scratch1, scratch2, scratch3
#if defined(XCHAL_DCACHE_SIZE) && (XCHAL_DCACHE_SIZE > 0) && (XCHAL_HAVE_DCACHE_TEST)
        movi \scratch3, 0
        movi \scratch1, 0
        movi \scratch2, XCHAL_DCACHE_SIZE
.Lclr_dcache_tag:
        sdct \scratch3, \scratch1
        addi \scratch1, \scratch1, XCHAL_DCACHE_LINESIZE * (1 << XCHAL_DCACHE_LINES_PER_TAG_LOG2)
        bne  \scratch1, \scratch2, .Lclr_dcache_tag 
        movi \scratch1, 0
.Lclr_dcache_word:
        sdcw \scratch3, \scratch1
        addi \scratch1, \scratch1, 4
        bne  \scratch1, \scratch2, .Lclr_dcache_word
#endif
.endm

/* Use cache test instructions to clear the instruction cache (both tags and
   data.  This code assumes the caches are disabled. */
.macro clr_icache scratch1, scratch2, scratch3
#if defined(XCHAL_ICACHE_SIZE) && (XCHAL_ICACHE_SIZE > 0) && (XCHAL_HAVE_ICACHE_TEST)
        movi \scratch3, 0
        movi \scratch1, 0
        movi \scratch2, XCHAL_ICACHE_SIZE
.Lclr_icache_tag:
        sict \scratch3, \scratch1
        addi \scratch1, \scratch1, XCHAL_ICACHE_LINESIZE
        bne  \scratch1, \scratch2, .Lclr_icache_tag 
        movi \scratch1, 0
.Lclr_icache_word:
        sicw \scratch3, \scratch1
        addi \scratch1, \scratch1, 4
        bne  \scratch1, \scratch2, .Lclr_icache_word
#endif
.endm

/* Clears one local mem */
.macro clr_localmem base_addr, bytes, scratch1, scratch2, scratch3, label
        movi \scratch1, \base_addr
        movi \scratch2, \base_addr + \bytes
        movi \scratch3, 0
\label:
        s32i \scratch3, \scratch1, 0
        addi \scratch1, \scratch1, 4
        bne  \scratch1, \scratch2, \label
.endm

/* Clears all configured local memories */
.macro clr_all_localmems scratch1, scratch2, scratch3
#if defined(XCHAL_INSTRAM0_SIZE) && (XCHAL_INSTRAM0_SIZE > 0) \
    && ((XCHAL_RESET_VECTOR_VADDR < XCHAL_INSTRAM0_VADDR) || \
            (XCHAL_RESET_VECTOR_VADDR >= XCHAL_INSTRAM0_VADDR + XCHAL_INSTRAM0_SIZE))
        clr_localmem XCHAL_INSTRAM0_VADDR, XCHAL_INSTRAM0_SIZE, \scratch1, \scratch2, \scratch3, .Liram0_loop
#endif
#if defined(XCHAL_INSTRAM1_SIZE) && (XCHAL_INSTRAM1_SIZE > 0) \
    && ((XCHAL_RESET_VECTOR_VADDR < XCHAL_INSTRAM1_VADDR) || \
            (XCHAL_RESET_VECTOR_VADDR >= XCHAL_INSTRAM1_VADDR + XCHAL_INSTRAM1_SIZE))
	clr_localmem XCHAL_INSTRAM1_VADDR, XCHAL_INSTRAM1_SIZE, \scratch1, \scratch2, \scratch3, .Liram1_loop
#endif
#if defined(XCHAL_DATARAM0_SIZE) && (XCHAL_DATARAM0_SIZE > 0)
	clr_localmem XCHAL_DATARAM0_VADDR, XCHAL_DATARAM0_SIZE, \scratch1, \scratch2, \scratch3, .Ldram0_loop
#endif
#if defined(XCHAL_DATARAM1_SIZE) && (XCHAL_DATARAM1_SIZE > 0)
	clr_localmem XCHAL_DATARAM1_VADDR, XCHAL_DATARAM1_SIZE, \scratch1, \scratch2, \scratch3, .Ldram1_loop
#endif
#if defined(XCHAL_URAM0_SIZE) && (XCHAL_URAM0_SIZE > 0)
	clr_localmem XCHAL_URAM0_VADDR, XCHAL_URAM0_SIZE, \scratch1, \scratch2, \scratch3, .Luram0_loop
#endif
#if defined(XCHAL_URAM1_SIZE) && (XCHAL_URAM1_SIZE > 0)
	clr_localmem XCHAL_URAM1_VADDR, XCHAL_URAM1_SIZE, \scratch1, \scratch2, \scratch3, .Luram1_loop
#endif
.endm

#if defined(XCHAL_HAVE_FXLK) && XCHAL_HAVE_FXLK && XCHAL_FXLK_MEM
	.section .ResetHandler.text
	.align 4
	.literal_position
	.align 4
	.global __reset_hook
	.type __reset_hook,@function

	/* this will be called by the reset vector before caches are enabled */
__reset_hook:
	// First check if we are running in locked mode, if not
	movi 	a5, XTHAL_FXLK_STATUS_REGNO
	rer  	a5, a5
	bbci	a5, XTHAL_REG_STATUS_LOCK_MODE_ENABLE_BIT, 1f

	// need to disable FlexLock memory compare
	// this comes out of reset enabled
	movi 	a5, XCHAL_FXLK_T1KEY << XTHAL_FXLK_KEY_SHIFT
	movi 	a6, XCHAL_FXLK_T2KEY << XTHAL_FXLK_KEY_SHIFT
	movi 	a7, XTHAL_FXLK_CMP_EN_TOGGLE_REGNO

	wer	a5, a7
	wer	a6, a7

	clr_all_localmems a2, a3, a4
	clr_icache	a2, a3, a4
	clr_dcache	a2, a3, a4

	wer 	a5, a7
	wer 	a6, a7
1:
#else
#error "Split-Lock initialization not needed"
#endif
