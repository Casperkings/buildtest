/*
 * Copyright (c) 2006 by Tensilica Inc.  ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of Tensilica Inc.
 * They may not be modified, copied, reproduced, distributed, or disclosed to
 * third parties in any manner, medium, or form, in whole or in part, without
 * the prior written consent of Tensilica Inc.
 */

#include "libio.h"

	
int gdbio_system(struct _reent * reent, const char * cmd)
{
    switch (libio_mode) {
    case LIBIO_GDB:
        {
            gdbio_ret_struct ret;
            int len;
            const char * end = cmd;

            while (*end++ != 0)
                ;

            len = end - cmd;
            ret = _gdbio_syscall(GDBIO_TARGET_SYSCALL_SYSTEM, 
		          len, 0, 0, cmd);

            reent->_errno = _gdbio_to_xtensa_errno(ret._errno);
            return ret.retval;
        }

    case LIBIO_SIM:
    default:
        break;
    }

    reent->_errno = ENOSYS;
    return -1;
}

