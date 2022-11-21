// 
// disass.c - disassembly routines for Xtensa
//
// $Id: //depot/rel/Homewood/ib.8/Xtensa/OS/hal/not_certified/disass.c#1 $

// Copyright (c) 2004-2013 Tensilica Inc.
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
#if !XCHAL_HAVE_XEA5

#if defined(__SPLIT__disassemble_size) || defined(__SPLIT__disassemble)
static uint8_t get_op_byte(void* addr)
{
   uint32_t a = ((uint32_t) addr) & 0xfffffffc;
   uint32_t w;
   /* Need to ensure that _l32i is used to read the memory, and
    * also that instruction isn't flixed.
    */
   __asm__ __volatile__("l32i %0, %1, 0\n\t" : "=a" (w) : "a" (a));
   w = w >> ((((uint32_t) addr) & 0x3) * 8);
   return w & 0xff; 
}
#endif 

#if defined(__SPLIT__op0_format_lengths)

/*  Instruction length in bytes as function of its op0 field (first nibble):  */
#ifdef XCHAL_OP0_FORMAT_LENGTHS
const uint8_t Xthal_op0_format_lengths[16] = {
  XCHAL_OP0_FORMAT_LENGTHS
};
#endif


#elif defined(__SPLIT__byte0_format_lengths)

/*  Instruction length in bytes as function of its first byte:  */
const uint8_t Xthal_byte0_format_lengths[256] = {
  XCHAL_BYTE0_FORMAT_LENGTHS
};


#elif defined(__SPLIT__disassemble_size)

//
// Disassembly is currently not supported in xtensa hal.
//

int32_t xthal_disassemble_size( uint8_t *instr_buf )
{
#ifdef XCHAL_OP0_FORMAT_LENGTHS
    /*  Extract op0 field of instruction (first nibble used for decoding):  */
# if XCHAL_HAVE_BE
    int32_t op0 = ((get_op_byte(instr_buf) >> 4) & 0xF);
# else
    int32_t op0 = (get_op_byte(instr_buf) & 0xF);
# endif
    return Xthal_op0_format_lengths[op0];
#else
    return Xthal_byte0_format_lengths[get_op_byte(instr_buf)];
#endif
}


#elif defined(__SPLIT__disassemble)

/*
 *  Note:  we make sure to avoid the use of library functions,
 *  to minimize dependencies.
 */
int32_t xthal_disassemble( 
    uint8_t *instr_buffer, /* the address of the instructions */
    void *tgt_address,		 /* where the instruction is to be */
    char *buffer,		 /* where the result goes */
    uint32_t buflen,		 /* size of buffer */
    uint32_t options		 /* what to display */
    )
{
#define OUTC(c)	do{ if( p < endp ) *p = (c); p++; }while(0)
    int32_t i, n;
    char *p = buffer, *endp = buffer + buflen - 1;
    /*static char *ret = " decoding not supported";*/
    static const char _hexc[16] = "0123456789ABCDEF";

    n = xthal_disassemble_size( instr_buffer );

    if( options & XTHAL_DISASM_OPT_ADDR ) {
	uint32_t addr = (uint32_t)tgt_address;
	for( i = 0; i < 8; i++ ) {
	    OUTC( _hexc[(addr >> 28) & 0xF] );
	    addr <<= 4;
	}
    }

    if( options & XTHAL_DISASM_OPT_OPHEX ) {
	if( p > buffer )
	    OUTC( ' ' );
	for( i = 0; i < 3; i++ ) {
	    if( i < n ) {
		OUTC( _hexc[(get_op_byte(instr_buffer) >> 4) & 0xF] );
		OUTC( _hexc[get_op_byte(instr_buffer) & 0xF] );
                ++instr_buffer;
	    } else {
		OUTC( ' ' );
		OUTC( ' ' );
	    }
	    OUTC( ' ' );
	}
    }

    if( options & XTHAL_DISASM_OPT_OPCODE ) {
	if( p > buffer )
	    OUTC( ' ' );
	OUTC( '?' );
	OUTC( '?' );
	OUTC( '?' );
	OUTC( ' ' );
	OUTC( ' ' );
	OUTC( ' ' );
	OUTC( ' ' );
    }

    if( options & XTHAL_DISASM_OPT_PARMS ) {
	if( p > buffer )
	    OUTC( ' ' );
	OUTC( '?' );
	OUTC( '?' );
	OUTC( '?' );
    }

    if( p < endp )
	*p = 0;
    else if( buflen > 0 )
	*endp = 0;

    return p - buffer;	/* return length needed, even if longer than buflen */
}


#endif /*split*/
#else
#endif
