/* Contains functions to read and write memory and registers
 */

/*
 * Copyright (c) 2012-2013 Tensilica Inc.
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
#if DEBUG
#include <stdio.h>
#endif
#include <xtensa/trax.h>
#include <xtensa/trax-proto.h>
#include <xtensa/traxfile.h>
#include <xtensa/xdm-regs.h>
#include <xtensa/traxreg.h>
#include <xtensa/core-macros.h>	/* For XTHAL_RER and XTHAL_WER */


int trax_read_register_eri (int regno, unsigned *data)
{
  /* The register number passed is the general NAR register number
   * Hence, convert it to appropriate ERI register number
   */
#if XCHAL_HAVE_DEBUG_ERI && XCHAL_HAVE_TRAX
  unsigned long address = XDM_APB_TO_ERI(regno);
  *data = XTHAL_RER (address);
#endif

  return 0;
}

int trax_write_register_eri (int regno, unsigned value)
{
#if XCHAL_HAVE_DEBUG_ERI && XCHAL_HAVE_TRAX
  unsigned long address = XDM_APB_TO_ERI(regno);
  XTHAL_WER (address, (unsigned long) value);
#endif

  return 0;
}


int trax_read_memory_eri (unsigned address, int len, int *data, 
                          unsigned *final_address)
{
  /* 
   * To read memory in ERI, write the address into the
   * address register and then read the content of the
   * data register
   */
  int i = 0;

  while (i < len){
#if XCHAL_HAVE_DEBUG_ERI && XCHAL_HAVE_TRAX
    XTHAL_WER (XDM_TRAX_ADDRESS, address);
    *data = XTHAL_RER (XDM_TRAX_DATA);
#endif

#if DEBUG
    fprintf(stderr, "TRAM address: 0x%x, Data: 0x%x\n", address, *data);
#endif

    data = data + 1;
    i = i + 4;
    address++;
  } 
  *final_address = address;
  return 0;
}

int trax_write_memory_eri (int address, unsigned value)
{
  /* Write the address into the address register */

#if XCHAL_HAVE_DEBUG_ERI && XCHAL_HAVE_TRAX
  XTHAL_WER (XDM_TRAX_ADDRESS, (unsigned long)(address/4));
  /* Write the data into the data register */
  XTHAL_WER (XDM_TRAX_DATA, (unsigned long)value);
#endif

  return 0;
}

int trax_write_register_field_eri (int regno, unsigned regmask,
                                   unsigned value)
{
  unsigned shiftmask = (regmask & (~regmask + 1));    /* lsbit of field */
  unsigned regvalue;
  int ecode;

  /*  For one bit fields, we accept off=0, on=1:  */
  if (regmask == 0xFFFFFFFF)
    regvalue = 0;           /* setting whole register, no need to read first */
  else if ((ecode = trax_read_register_eri (regno, &regvalue)) != 0)
    return ecode;
  
  regvalue = (regvalue & ~regmask) | (value * shiftmask);
  return trax_write_register_eri (regno, regvalue);
}
