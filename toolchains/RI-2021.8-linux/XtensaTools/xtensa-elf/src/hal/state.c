// Copyright (c) 2005-2018 Cadence Design Systems Inc.
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

#if defined(__SPLIT__cpregs_size)
// space for state of TIE coprocessors:
const uint32_t Xthal_cpregs_size[8] =
        {
            XCHAL_CP0_SA_SIZE,
            XCHAL_CP1_SA_SIZE,
            XCHAL_CP2_SA_SIZE,
            XCHAL_CP3_SA_SIZE,
            XCHAL_CP4_SA_SIZE,
            XCHAL_CP5_SA_SIZE,
            XCHAL_CP6_SA_SIZE,
            XCHAL_CP7_SA_SIZE
        };

#elif defined(__SPLIT__cpregs_align)
const uint32_t Xthal_cpregs_align[8] =
        {
            XCHAL_CP0_SA_ALIGN,
            XCHAL_CP1_SA_ALIGN,
            XCHAL_CP2_SA_ALIGN,
            XCHAL_CP3_SA_ALIGN,
            XCHAL_CP4_SA_ALIGN,
            XCHAL_CP5_SA_ALIGN,
            XCHAL_CP6_SA_ALIGN,
            XCHAL_CP7_SA_ALIGN
        };

#elif defined(__SPLIT__cp_names)
const char * const Xthal_cp_names[8] =
        {
            XCHAL_CP0_NAME,
            XCHAL_CP1_NAME,
            XCHAL_CP2_NAME,
            XCHAL_CP3_NAME,
            XCHAL_CP4_NAME,
            XCHAL_CP5_NAME,
            XCHAL_CP6_NAME,
            XCHAL_CP7_NAME
        };

#elif defined(__SPLIT__init_mem_extra)
// CMS: I have made the assumptions that 0's are safe initial
// values. That may be wrong at some point.
//
// initialize the extra processor
void
xthal_init_mem_extra(void *address)
/* not clear that it is safe to call memcpy and also not clear
   that performance is important. */
{
    uint32_t *ptr;
    uint32_t n = (((uint32_t) XCHAL_NCP_SA_SIZE)/(sizeof(*ptr)));
    ptr = xthal_cvt_voidp_to_uint32p(address);
    while(n>0U)
    {
        n--;
        *ptr = 0U;
        ptr++;
    } 
}

#elif defined(__SPLIT__init_mem_cp)
// initialize the TIE coprocessor
void
xthal_init_mem_cp(void *address, int32_t cp)
{
    uint32_t *ptr;
    uint32_t *end;

    if( (cp >= 0) && (cp <= XCHAL_CP_MAX) )
    {
        ptr = xthal_cvt_voidp_to_uint32p(address);
        end = ptr + Xthal_cpregs_size[cp]/sizeof(*ptr);
        while( ptr < end )
          {
            *ptr = 0;
            ptr++;
          }
    }
}

#endif /*splitting*/


