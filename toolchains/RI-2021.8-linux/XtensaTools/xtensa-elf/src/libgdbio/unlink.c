/*
 * Copyright (c) 2006 by Tensilica Inc.  ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of Tensilica Inc.
 * They may not be modified, copied, reproduced, distributed, or disclosed to
 * third parties in any manner, medium, or form, in whole or in part, without
 * the prior written consent of Tensilica Inc.
 */

#include "libio.h"


int _unlink_r(struct _reent * reent, const char * path)
{
    switch (libio_mode) {
    case LIBIO_SIM:
        {
            register int a2 __asm__ ("a2") = SYS_unlink;
            register const char *a3 __asm__ ("a3") = path;
            register int ret_val __asm__ ("a2");
            register int ret_err __asm__ ("a3");

            __asm__ ("simcall"
                     : "=a" (ret_val), "=a" (ret_err)
                     : "a" (a2), "a" (a3));

            reent->_errno = ret_err;
            return ret_val;
        }

    case LIBIO_GDB:
        {
            gdbio_ret_struct ret;
            int len;
            const char * end = path;

            while (*end++ != 0)
                ;

            len = end - path;
            ret = _gdbio_syscall(GDBIO_TARGET_SYSCALL_UNLINK, 
		       len, 0, 0, path);
            reent->_errno = _gdbio_to_xtensa_errno(ret._errno);
            return ret.retval;
        }

    default:
        break;
    }

    reent->_errno = EIO;
    return -1;
}

