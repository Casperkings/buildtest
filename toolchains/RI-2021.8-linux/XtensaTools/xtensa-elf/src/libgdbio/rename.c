/*
 * Copyright (c) 2006 by Tensilica Inc.  ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of Tensilica Inc.
 * They may not be modified, copied, reproduced, distributed, or disclosed to
 * third parties in any manner, medium, or form, in whole or in part, without
 * the prior written consent of Tensilica Inc.
 */

#include "libio.h"


int _rename_r (struct _reent * reent, const char * oldpath, const char * newpath)
{
    switch (libio_mode) {
    case LIBIO_SIM:
        {
            register int a2 __asm__ ("a2") = SYS_rename;
            register const char *a3 __asm__ ("a3") = oldpath;
            register const char *a4 __asm__ ("a4") = newpath;
            register int ret_val __asm__ ("a2");
            register int ret_err __asm__ ("a3");

            __asm__ ("simcall"
                     : "=a" (ret_val), "=a" (ret_err)
                     : "a" (a2), "a" (a3), "a" (a4));

            reent->_errno = ret_err;
            return ret_val;
        }

    case LIBIO_GDB:
        {
            gdbio_ret_struct ret;
            int oldlen = 0;
            int newlen = 0;
            const char * end = oldpath;

            while (*end++ != 0)
                ;
            oldlen = end - oldpath;
            end = newpath;
            while (*end++ != 0)
                ;
            newlen = end - newpath;

            ret = _gdbio_syscall(GDBIO_TARGET_SYSCALL_RENAME, 
		                 newpath, newlen, oldlen, oldpath);
            reent->_errno = _gdbio_to_xtensa_errno(ret._errno);
            return ret.retval;
        }

    default:
        break;
    }

    reent->_errno = EIO;
    return -1;
}

