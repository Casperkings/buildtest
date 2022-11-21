/*
 * Copyright (c) 2006-2007 by Tensilica Inc.  ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of Tensilica Inc.
 * They may not be modified, copied, reproduced, distributed, or disclosed to
 * third parties in any manner, medium, or form, in whole or in part, without
 * the prior written consent of Tensilica Inc.
 */

#include "libio.h"


int _read_r (struct _reent * reent, int file, void * buf, size_t bytes)
{
    // workaround for compiler problem
    static volatile int xx;

    switch (libio_mode) {
    case LIBIO_SIM:
        {
            register int a2 __asm__ ("a2") = SYS_read;
            register int a3 __asm__ ("a3") = file;
            register char *a4 __asm__ ("a4") = buf;
            register int a5 __asm__ ("a5") = bytes;
            register int ret_val __asm__ ("a2");
            register int ret_err __asm__ ("a3");

            __asm__ ("simcall"
                     : "=a" (ret_val), "=a" (ret_err)
                     : "a" (a2), "a" (a3), "a" (a4), "a" (a5));

            reent->_errno = ret_err;
            xx = ret_val;
            return xx;
            //return ret_val;
        }

    case LIBIO_GDB:
        {
            gdbio_ret_struct ret;

            if ((file = _gdbio_map_fd(reent, file)) < 0)
                return -1;
            ret = _gdbio_syscall(GDBIO_TARGET_SYSCALL_READ, buf, bytes, 0, file);
            reent->_errno = _gdbio_to_xtensa_errno(ret._errno);
            return ret.retval;
        }

    default:
        break;
    }

    reent->_errno = EIO;
    return -1;
}

