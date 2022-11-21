/*
 * Copyright (c) 2006-2013 by Tensilica Inc.  ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of Tensilica Inc.
 * They may not be modified, copied, reproduced, distributed, or disclosed to
 * third parties in any manner, medium, or form, in whole or in part, without
 * the prior written consent of Tensilica Inc.
 */

#include "libio.h"


int _open_r(struct _reent * reent, const char * path, int flags, int mode)
{
    // workaround for compiler problem
    static volatile int xx;

    switch (libio_mode) {
    case LIBIO_SIM:
        {
            register int a2 __asm__ ("a2") = SYS_open;
            register const char *a3 __asm__ ("a3") = path;
            register int a4 __asm__ ("a4") = flags;
            register int a5 __asm__ ("a5") = mode;
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
            int len, file;
            const char * end = path;

            while (*end++ != 0)
                ;
            len = end - path;

            if ((file = _gdbio_new_fd(reent)) < 0)
                return -1;

            flags = _gdbio_from_xtensa_flags(flags);
            mode = _gdbio_from_xtensa_mode(mode);
            ret = _gdbio_syscall(GDBIO_TARGET_SYSCALL_OPEN, flags, mode, len, path);
            reent->_errno = _gdbio_to_xtensa_errno(ret._errno);
            if (ret.retval >= 0) {
                _gdbio_set_fd(file, ret.retval);
                return file;
            }
        return ret.retval;
    }

    default:
        break;
    }

    reent->_errno = EIO;
    return -1;
}

