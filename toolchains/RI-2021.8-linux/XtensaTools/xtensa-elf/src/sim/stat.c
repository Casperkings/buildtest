/* stat system call for the Xtensa semihosting simulator.  */

/*
 * Copyright (c) 2004 by Tensilica Inc.  ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of Tensilica Inc.
 * They may not be modified, copied, reproduced, distributed, or disclosed to
 * third parties in any manner, medium, or form, in whole or in part, without
 * the prior written consent of Tensilica Inc.
 */

#include <errno.h>
#include <sys/stat.h>
#include <sys/reent.h>

#include <xtensa/simcall.h>
#include <xtensa/simcall-fcntl.h>

#include "regs.h"


mode_t
convert_mode(mode_t inmode)
{
  mode_t ret = 0;

  if (inmode & _SIMC_S_IFSOCK)
    ret |= S_IFSOCK;
  if (inmode & _SIMC_S_IFLNK)
    ret |= S_IFLNK;
  if (inmode & _SIMC_S_IFREG)
    ret |= S_IFREG;
  if (inmode & _SIMC_S_IFBLK)
    ret |= S_IFBLK;
  if (inmode & _SIMC_S_IFDIR)
    ret |= S_IFDIR;
  if (inmode & _SIMC_S_IFCHR)
    ret |= S_IFCHR;
  if (inmode & _SIMC_S_IFIFO)
    ret |= S_IFIFO;
  if (inmode & _SIMC_S_IRUSR)
    ret |= S_IRUSR;
  if (inmode & _SIMC_S_IWUSR)
    ret |= S_IWUSR;
  if (inmode & _SIMC_S_IXUSR)
    ret |= S_IXUSR;

  return ret;
}


int
_stat_r (struct _reent *reent, const char *path, struct stat *st)
{
  register int    a2 __asm__ (CMD)  = SYS_stat;
  register void * a3 __asm__ (ARG1) = (void *) path;
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

