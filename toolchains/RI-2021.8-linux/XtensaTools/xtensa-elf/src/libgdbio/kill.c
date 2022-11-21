/* kill.c -- remove a process
 *
 * Copyright (c) 2004-2013 by Tensilica Inc.  ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of Tensilica Inc.
 * They may not be modified, copied, reproduced, distributed, or disclosed to
 * third parties in any manner, medium, or form, in whole or in part, without
 * the prior written consent of Tensilica Inc.
 */

#include "libio.h"


int _kill_r (struct _reent *ptr, int pid, int sig)
{
    switch (libio_mode) {
    case LIBIO_SIM:
    case LIBIO_GDB:
        if (pid == __MYPID)
            _exit (sig);
        return 0;

    default:
        break;
    }

    return 0;
}

