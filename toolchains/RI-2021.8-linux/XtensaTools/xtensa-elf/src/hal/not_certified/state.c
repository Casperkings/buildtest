// Copyright (c) 2005-2010 Tensilica Inc.
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

//----------------------------------------------------------------------

#if defined(__SPLIT__extra_size)
// space for "extra" (user special registers and non-coprocessor TIE) state:
const uint32_t Xthal_extra_size = XCHAL_NCP_SA_SIZE;

#elif defined(__SPLIT__extra_align)
const uint32_t Xthal_extra_align = XCHAL_NCP_SA_ALIGN;

#elif defined(__SPLIT__all_extra_size)
// total save area size (extra + all coprocessors + min 16-byte alignment everywhere)
const uint32_t Xthal_all_extra_size = XCHAL_TOTAL_SA_SIZE;

#elif defined(__SPLIT__all_extra_align)
// maximum required alignment for the total save area (this might be useful):
const uint32_t Xthal_all_extra_align = XCHAL_TOTAL_SA_ALIGN;

#elif defined(__SPLIT__num_coprocessors)
// number of coprocessors starting contiguously from zero
// (same as Xthal_cp_max, but included for Tornado2):
const uint32_t Xthal_num_coprocessors = XCHAL_CP_MAX;

#elif defined(__SPLIT__cp_num)
// actual number of coprocessors:
const uint8_t Xthal_cp_num    = XCHAL_CP_NUM;

#elif defined(__SPLIT__cp_max)
// index of highest numbered coprocessor, plus one:
const uint8_t Xthal_cp_max    = XCHAL_CP_MAX;

// index of highest allowed coprocessor number, per cfg, plus one:
//const uint8_t Xthal_cp_maxcfg = XCHAL_CP_MAXCFG;

#elif defined(__SPLIT__cp_mask)
// bitmask of which coprocessors are present:
const uint32_t  Xthal_cp_mask   = XCHAL_CP_MASK;

#elif defined(__SPLIT__cp_id_mappings)
// Coprocessor ID from its name

# ifdef XCHAL_CP0_IDENT
const uint8_t XCJOIN(Xthal_cp_id_,XCHAL_CP0_IDENT) = 0;
# endif
# ifdef XCHAL_CP1_IDENT
const uint8_t XCJOIN(Xthal_cp_id_,XCHAL_CP1_IDENT) = 1;
# endif
# ifdef XCHAL_CP2_IDENT
const uint8_t XCJOIN(Xthal_cp_id_,XCHAL_CP2_IDENT) = 2;
# endif
# ifdef XCHAL_CP3_IDENT
const uint8_t XCJOIN(Xthal_cp_id_,XCHAL_CP3_IDENT) = 3;
# endif
# ifdef XCHAL_CP4_IDENT
const uint8_t XCJOIN(Xthal_cp_id_,XCHAL_CP4_IDENT) = 4;
# endif
# ifdef XCHAL_CP5_IDENT
const uint8_t XCJOIN(Xthal_cp_id_,XCHAL_CP5_IDENT) = 5;
# endif
# ifdef XCHAL_CP6_IDENT
const uint8_t XCJOIN(Xthal_cp_id_,XCHAL_CP6_IDENT) = 6;
# endif
# ifdef XCHAL_CP7_IDENT
const uint8_t XCJOIN(Xthal_cp_id_,XCHAL_CP7_IDENT) = 7;
# endif

#elif defined(__SPLIT__cp_mask_mappings)
// Coprocessor "mask" (1 << ID) from its name

# ifdef XCHAL_CP0_IDENT
const uint32_t  XCJOIN(Xthal_cp_mask_,XCHAL_CP0_IDENT) = (1 << 0);
# endif
# ifdef XCHAL_CP1_IDENT
const uint32_t  XCJOIN(Xthal_cp_mask_,XCHAL_CP1_IDENT) = (1 << 1);
# endif
# ifdef XCHAL_CP2_IDENT
const uint32_t  XCJOIN(Xthal_cp_mask_,XCHAL_CP2_IDENT) = (1 << 2);
# endif
# ifdef XCHAL_CP3_IDENT
const uint32_t  XCJOIN(Xthal_cp_mask_,XCHAL_CP3_IDENT) = (1 << 3);
# endif
# ifdef XCHAL_CP4_IDENT
const uint32_t  XCJOIN(Xthal_cp_mask_,XCHAL_CP4_IDENT) = (1 << 4);
# endif
# ifdef XCHAL_CP5_IDENT
const uint32_t  XCJOIN(Xthal_cp_mask_,XCHAL_CP5_IDENT) = (1 << 5);
# endif
# ifdef XCHAL_CP6_IDENT
const uint32_t  XCJOIN(Xthal_cp_mask_,XCHAL_CP6_IDENT) = (1 << 6);
# endif
# ifdef XCHAL_CP7_IDENT
const uint32_t  XCJOIN(Xthal_cp_mask_,XCHAL_CP7_IDENT) = (1 << 7);
# endif
#endif
