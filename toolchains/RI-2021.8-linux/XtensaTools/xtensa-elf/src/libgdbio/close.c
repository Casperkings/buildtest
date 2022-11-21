/*  
 * Copyright (c) 2006-2007 by Tensilica Inc.  ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of Tensilica Inc.
 * They may not be modified, copied, reproduced, distributed, or disclosed to
 * third parties in any manner, medium, or form, in whole or in part, without
 * the prior written consent of Tensilica Inc.
 */

#include "libio.h"


int _close_r (struct _reent * reent, int file)
{
    // workaround for compiler problem
    static volatile int xx;

    switch (libio_mode) {
    case LIBIO_SIM:
        {
            register int a2 __asm__ ("a2") = SYS_close;
            register int a3 __asm__ ("a3") = file;
            register int ret_val __asm__ ("a2");
            register int ret_err __asm__ ("a3");

            __asm__ ("simcall"
                     : "=a" (ret_val), "=a" (ret_err)
                     : "a" (a2), "a" (a3));

            reent->_errno = ret_err;
            xx = ret_val;
            return xx;
            //return ret_val;
        }
    case LIBIO_GDB:
        {
            gdbio_ret_struct ret;

            if ((file = _gdbio_close_fd(reent, file)) < 0)
                return -1;
            if (file >= 0 && file <= 2) /* never close stdin,stdout,stderr (PR 15796) */
                return 0;
            ret = _gdbio_syscall(GDBIO_TARGET_SYSCALL_CLOSE, 0, 0, 0, file);
            reent->_errno = _gdbio_to_xtensa_errno(ret._errno);
            return ret.retval;
        }
    default:
        reent->_errno = EIO;
        return -1;
    }
}

