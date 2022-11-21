/* Copyright (c) 2007-2008 by Tensilica Inc.  ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of Tensilica Inc.
 * They may not be modified, copied, reproduced, distributed, or disclosed to
 * third parties in any manner, medium, or form, in whole or in part, without
 * the prior written consent of Tensilica Inc.
 */

#include    <xtensa/board.h>
#include    <xtensa/xtbsp.h>
#include    <stdio.h>

#ifndef XTBOARD_RAM_VADDR
int main(int argc, char *arv[]) {
  xtbsp_display_string("No system RAM");
  printf("No system RAM configured, cannot run memtest ... \n");
  return 0;
}
#else /* XTBOARD_RAM_VADDR */

#define MEM_SIZE    XTBOARD_RAM_SIZE	/* e.g. 64 MB on XT-AV200,XT-AV60 boards */
#define MEM_START   (XTBOARD_RAM_VADDR + 0x00020000)
#define MEM_END     (XTBOARD_RAM_VADDR + XTBOARD_RAM_SIZE - 0x00090000)

#define DEBUG

static int addr_mem_test( int *start_addr, int *end_addr)
{
    unsigned int *mem_ptr;
    int pass_fail = 1;
#ifdef DEBUG
    printf("\n%s address test from %X to %X\n", 
           xtbsp_board_name(), start_addr, end_addr);
#endif
    for (mem_ptr = (unsigned int *) start_addr; mem_ptr < (unsigned int *) end_addr; mem_ptr++)
      {
        *mem_ptr = (unsigned int) mem_ptr;
#ifdef DEBUG
        if (((unsigned int) mem_ptr | 0xffff0000) == 0xffff0000) 
          {
            printf("\r %X w",mem_ptr);   
          }
#endif
      }
    for (mem_ptr = (unsigned int *) start_addr; mem_ptr < (unsigned int *) end_addr; mem_ptr++)
      {
        if ((unsigned int) mem_ptr != *mem_ptr)
        {
#ifdef DEBUG
          printf("\n %X bad memory %X\n",mem_ptr,*mem_ptr);
#endif
          pass_fail = 0;
          break;
        }
#ifdef DEBUG
        if (((unsigned int) mem_ptr | 0xffff0000) == 0xffff0000) 
          {
            printf("\r %X r",mem_ptr);
          }
#endif
      }
#ifdef DEBUG
    if (pass_fail)
      printf(" OK");
    else printf(" Fail");
#endif
    return (pass_fail);
}

/***************************************************************************/
static int mem_test_patterns(unsigned int *start_addr, unsigned int *end_addr, unsigned int mem_pattern)
{
  unsigned int *mem_ptr;
  int pass_fail = 1;
#ifdef DEBUG
    printf("\n%s memtest from %X to %X with pattern %X\n", 
           xtbsp_board_name(), start_addr, end_addr, mem_pattern);
#endif
    for (mem_ptr = (unsigned int *) start_addr; mem_ptr < (unsigned int *) end_addr; mem_ptr++)
      {
#ifdef DEBUG
        if (((unsigned int) mem_ptr | 0xffff0000) == 0xffff0000) 
          {
            printf("\r %X w",mem_ptr);   
            fflush(stdout);
          }
#endif
        *mem_ptr = (unsigned int) mem_pattern;
      }
    for (mem_ptr = (unsigned int *) start_addr; mem_ptr < (unsigned int *) end_addr; mem_ptr++)
      {
#ifdef DEBUG
        if (((unsigned int) mem_ptr | 0xffff0000) == 0xffff0000) 
          {
            printf("\r %X r",mem_ptr);
            fflush(stdout);
          }
#endif
        if ((unsigned int) mem_pattern != *mem_ptr)
          {
#ifdef DEBUG
            printf("\n %X bad memory %X\n",mem_ptr,*mem_ptr);
#endif
            pass_fail = 0;
            break;
          }
      }
#ifdef DEBUG
    if (pass_fail)
      printf(" OK\n");
    else printf(" Failed\n");
#endif
    return (pass_fail);
}

/***************************************************************************/
int main(int argc, char *arv[])
{
  xtbsp_display_string("Running Memtest");
  printf("Running Memtest... \n");
  if ((addr_mem_test((int *)MEM_START, (int *)MEM_END)) &
      (mem_test_patterns((unsigned int *)MEM_START, (unsigned int *)MEM_END, (unsigned int) 0xa5a5a5a5)) &
      (mem_test_patterns((unsigned int *)MEM_START, (unsigned int *)MEM_END, (unsigned int) 0x5a5a5a5a)) &
      (mem_test_patterns((unsigned int *)MEM_START, (unsigned int *)MEM_END, (unsigned int) 0xffffffff)) &
      (mem_test_patterns((unsigned int *)MEM_START, (unsigned int *)MEM_END, (unsigned int) 0x00000000)))
    {
#ifdef DEBUG
    xtbsp_display_string("Test Passed!!!!");
    printf("Test passed!!!!\n");
#endif
    }
  else {
#ifdef DEBUG
    xtbsp_display_string("Test Failed!!!!");
    printf("Test failed!!!!\n");
#endif 
    }
  return 0;
}

#endif /* XTBOARD_RAM_VADDR */

