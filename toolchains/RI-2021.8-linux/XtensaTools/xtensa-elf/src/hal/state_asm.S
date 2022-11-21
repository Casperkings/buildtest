/*
 * state_asm.S - assembly language processor management routines
 */

/*
 * Copyright (c) 2005-2021 Cadence Design Systems Inc.
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

#include <xtensa/coreasm.h>

	.begin	schedule	// turn on assembler scheduling for this file
	.text

//----------------------------------------------------------------------
// 		save the extra processor state.
//----------------------------------------------------------------------

#if defined(__SPLIT__save_extra) ||\
    defined(__SPLIT__save_extra_nw)

// void xthal_save_extra(void *base)

DECLFUNC(xthal_save_extra)
	abi_entry
#if !XCHAL_HAVE_XEA5 // todo
	xchal_extra_store_funcbody
#endif
	abi_return
	endfunc


//----------------------------------------------------------------------
// 		restore the extra processor state.
//----------------------------------------------------------------------

#elif 	defined(__SPLIT__restore_extra) ||\
	defined(__SPLIT__restore_extra_nw)
	
// void xthal_restore_extra(void *base)

DECLFUNC(xthal_restore_extra)
	abi_entry
#if !XCHAL_HAVE_XEA5 // todo
	xchal_extra_load_funcbody
#endif 
	abi_return
	endfunc

//----------------------------------------------------------------------
// 		save the TIE COPROCESSORS state
//----------------------------------------------------------------------

#elif 	defined(__SPLIT__save_cpregs) ||\
	defined(__SPLIT__save_cpregs_nw)

// void xthal_save_cpregs(void *base, int)
DECLFUNC(xthal_save_cpregs)
	abi_entry
#if !XCHAL_HAVE_XEA5 // todo
	xchal_cpi_store_funcbody
#endif
	abi_return
	endfunc

//----------------------------------------------------------------------
//              restore the TIE coprocessor state
//----------------------------------------------------------------------

#elif     defined(__SPLIT__restore_cpregs) ||\
        defined(__SPLIT__restore_cpregs_nw)

// void xthal_restore_cpregs(void *base, int)

DECLFUNC(xthal_restore_cpregs)
        abi_entry
#if !XCHAL_HAVE_XEA5 // todo
        xchal_cpi_load_funcbody
#endif
        abi_return
        endfunc

//----------------------------------------------------------------------
//		coprocessor enable/disable
//----------------------------------------------------------------------

#elif 	defined(__SPLIT__validate_cp) ||\
	defined(__SPLIT__validate_cp_nw)

// validate the register file.
// void xthal_validate_cp(int)

DECLFUNC(xthal_validate_cp)
	abi_entry
#if XCHAL_HAVE_XEA5 //todo
#else
#if XCHAL_HAVE_CP
	rsr.cpenable	a3
#if XCHAL_HAVE_XEA5
	movi	a4, 3
	add	a2,a2,a2
#else
	movi	a4, 1
#endif
	ssl	a2
	sll	a4, a4
	or	a3, a3, a4
	wsr.cpenable	a3
#endif
#endif
	abi_return
	endfunc

#elif 	defined(__SPLIT__invalidate_cp) || \
	defined(__SPLIT__invalidate_cp_nw)

// invalidate the register file.
// void xthal_invalidate_cp(int)

DECLFUNC(xthal_invalidate_cp)
	abi_entry
#if XCHAL_HAVE_CP && !XCHAL_HAVE_XEA5 // todo
	rsr.cpenable	a3
	movi	a4, 1
	ssl	a2
	sll	a4, a4
	and	a4, a3, a4
	xor	a3, a3, a4
	wsr.cpenable	a3
#endif
	abi_return
	endfunc


//----------------------------------------------------------------------
//  Access the CPENABLE register
//----------------------------------------------------------------------

#elif 	defined(__SPLIT__get_cpenable) || \
	defined(__SPLIT__get_cpenable_nw)

// Note xthal_get_cpenable is now defined as a 'C' inline static.  The assembly
// language implementation is retained because it is known to be called from assembly

// unsigned xthal_get_cpenable(void);

DECLFUNC(xthal_get_cpenable)
	abi_entry
#if XCHAL_HAVE_XEA5
	addi	a0, x0, 0
#else
#if XCHAL_HAVE_CP
	rsr.cpenable	a2
#else
	movi	a2, 0	// if no CPENABLE (no coprocessors), none is ever enabled
#endif
#endif
	abi_return
	endfunc

#elif 	defined(__SPLIT__set_cpenable) ||\
	defined(__SPLIT__set_cpenable_nw)

// Note xthal_set_cpenable is now defined as a 'C' inline static.  The assembly
// language implementation is retained because it is known to be called from assembly

// void xthal_set_cpenable(unsigned);
//
// Note:  to help asm code performance (eg. OS task switch),
// this routine returns the previous value of CPENABLE in a3
// (not a2, because that could require an extra mov instruction).
// This return value is not shown in the prototype, because
// C code won't see it.
// [Perhaps this should go in an RTOS-specific Core HAL or BSP.  TBD.]

DECLFUNC(xthal_set_cpenable)
	abi_entry
#if XCHAL_HAVE_XEA5
#else
#if XCHAL_HAVE_CP
	//rsr.cpenable	a3	// return previous CPENABLE
	movi	a3, 0		// for now, always return 0 (VxWorks currently done that way)

	wsr.cpenable	a2
#else
	movi	a3, 0	// if no CPENABLE (no coprocessors), none is ever enabled
#endif
#endif
	abi_return
	endfunc
#endif
        .end schedule
