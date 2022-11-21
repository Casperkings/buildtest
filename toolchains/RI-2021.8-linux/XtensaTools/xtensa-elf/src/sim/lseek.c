/* lseek system call for the Xtensa semihosting simulator.  */

/*
 * Copyright (c) 2004 by Tensilica Inc.  ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of Tensilica Inc.
 * They may not be modified, copied, reproduced, distributed, or disclosed to
 * third parties in any manner, medium, or form, in whole or in part, without
 * the prior written consent of Tensilica Inc.
 */

#include <sys/reent.h>
#include <xtensa/simcall.h>

#include "regs.h"

int
_lseek_r (struct _reent *reent, int file, int pos, int dir)
{
  register int a2 __asm__ (CMD)  = SYS_lseek;
  register int a3 __asm__ (ARG1) = file;
  register int a4 __asm__ (ARG2) = pos;
  register int a5 __asm__ (ARG3) = dir;
  register int ret_val __asm__ (RETVAL);
  register int ret_err __asm__ (RETERR);

  __asm__ (SIMCALL
	   : "=a" (ret_val), "=a" (ret_err)
	   : "a" (a2), "a" (a3), "a" (a4), "a" (a5));

  if (ret_err)
    reent->_errno = ret_err;

  return ret_val;
}
