/* fstat system call for the Xtensa semihosting simulator.  */

/*
 * Copyright (c) 2004 by Tensilica Inc.  ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of Tensilica Inc.
 * They may not be modified, copied, reproduced, distributed, or disclosed to
 * third parties in any manner, medium, or form, in whole or in part, without
 * the prior written consent of Tensilica Inc.
 */

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/reent.h>

#include <xtensa/simcall.h>

#include "regs.h"

/* In stat.c */
extern mode_t convert_mode(mode_t inmode);

int
_fstat_r (struct _reent *reent, int file, struct stat *st)
{
  if ((file == 0) || (file == 1) || (file == 2)) {
    // stdin/stdout/stderr report as tty
    st->st_mode    = S_IFCHR;
    st->st_size    = 0;
    st->st_blksize = 0;
    return 0;
  }
  else {
    register int    a2 __asm__ (CMD)  = SYS_fstat;
    register int    a3 __asm__ (ARG1) = file;
    register void * a4 __asm__ (ARG2) = (void *) st;
    register int ret_val __asm__ (RETVAL);
    register int ret_err __asm__ (RETERR);

    __asm__ (SIMCALL
             : "=a" (ret_val), "=a" (ret_err)
             : "a" (a2), "a" (a3), "a" (a4));

    if (ret_err)
      reent->_errno = ret_err;

    if (ret_val == 0)
      st->st_mode = convert_mode(st->st_mode);

    return ret_val;
  }
}

