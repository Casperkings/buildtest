/*
 * Copyright (c) 2006-2013 by Tensilica Inc.  ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of Tensilica Inc.
 * They may not be modified, copied, reproduced, distributed, or disclosed to
 * third parties in any manner, medium, or form, in whole or in part, without
 * the prior written consent of Tensilica Inc.
 */

#include "libio.h"


int _gettimeofday_r (struct _reent * reent, struct timeval *tv, struct timezone * tz)
{
    switch (libio_mode) {
    case LIBIO_SIM:
        {
            register int a2 __asm__ ("a2") = SYS_gettimeofday;
            register struct timeval *a3 __asm__ ("a3") = tv;
            register struct timezone *a4 __asm__ ("a4") = tz;
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
            gdbio_timeval_struct gtv;
            gdbio_ret_struct ret =
                _gdbio_syscall(GDBIO_TARGET_SYSCALL_GETTIMEOFDAY, NULL, 0, 0, &gtv);

            if (tz)
                memset(tz, 0, sizeof(struct timezone));

            gtv.tv_sec = _gdbio_big_endian_4(gtv.tv_sec);
            gtv.tv_usec = _gdbio_big_endian_8(gtv.tv_usec);

            tv->tv_sec = gtv.tv_sec;
            tv->tv_usec = gtv.tv_usec;

            reent->_errno = _gdbio_to_xtensa_errno(ret._errno);
            return ret.retval;
        }

    default:
        break;
    }

    reent->_errno = EINVAL;
    return -1;
}

