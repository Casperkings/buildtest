/*
 * Copyright (c) 2006-2013 by Tensilica Inc.  ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of Tensilica Inc.
 * They may not be modified, copied, reproduced, distributed, or disclosed to
 * third parties in any manner, medium, or form, in whole or in part, without
 * the prior written consent of Tensilica Inc.
 */

#include "libio.h"


extern mode_t convert_simio_mode(mode_t inmode);


int _fstat_r (struct _reent * reent, int fd, struct stat * buf)
{
    // workaround for compiler problem
    static volatile int xx;

    switch (libio_mode) {
    case LIBIO_SIM:
        {
            if ((fd == 0) || (fd == 1) || (fd == 2)) {
                // stdin/stdout/stderr report as tty
                buf->st_mode    = S_IFCHR;
                buf->st_size    = 0;
                buf->st_blksize = 0;
                return 0;
            }
            else {
                register int    a2 __asm__ ("a2") = SYS_fstat;
                register int    a3 __asm__ ("a3") = fd;
                register void * a4 __asm__ ("a4") = (void *) buf;
                register int ret_val __asm__ ("a2");
                register int ret_err __asm__ ("a3");

                __asm__ ("simcall"
                         : "=a" (ret_val), "=a" (ret_err)
                         : "a" (a2), "a" (a3), "a" (a4));

                if (ret_err)
                    reent->_errno = ret_err;

                if (ret_val == 0)
                    buf->st_mode = convert_simio_mode(buf->st_mode);

                xx = ret_val;
                return xx;
            }
        }

    case LIBIO_GDB:
        {
            gdbio_stat_struct gbuf;
            gdbio_ret_struct ret;
            
            if ((fd = _gdbio_map_fd(reent, fd)) < 0)
              return -1;

            ret = _gdbio_syscall(GDBIO_TARGET_SYSCALL_FSTAT, &gbuf, 0, 0, fd);
            reent->_errno = _gdbio_to_xtensa_errno(ret._errno);

            gbuf.st_dev = _gdbio_big_endian_4(gbuf.st_dev);
            gbuf.st_ino = _gdbio_big_endian_4(gbuf.st_ino);
            gbuf.st_mode = _gdbio_big_endian_4(gbuf.st_mode);
            gbuf.st_nlink = _gdbio_big_endian_4(gbuf.st_nlink);
            gbuf.st_uid = _gdbio_big_endian_4(gbuf.st_uid);
            gbuf.st_gid = _gdbio_big_endian_4(gbuf.st_gid);
            gbuf.st_rdev = _gdbio_big_endian_4(gbuf.st_rdev);
            gbuf.st_size = _gdbio_big_endian_8(gbuf.st_size);
            gbuf.st_blksize = _gdbio_big_endian_8(gbuf.st_blksize);
            gbuf.st_blocks = _gdbio_big_endian_8(gbuf.st_blocks);
            gbuf.st_atime = _gdbio_big_endian_4(gbuf.st_atime);
            gbuf.st_mtime = _gdbio_big_endian_4(gbuf.st_mtime);
            gbuf.st_ctime = _gdbio_big_endian_4(gbuf.st_ctime);

            memset(buf, 0, sizeof(struct stat));
            buf->st_dev = gbuf.st_dev;
            buf->st_ino = gbuf.st_ino;
            buf->st_mode = gbuf.st_mode;
            buf->st_nlink = gbuf.st_nlink;
            buf->st_uid = gbuf.st_uid;
            buf->st_gid = gbuf.st_gid;
            buf->st_rdev = gbuf.st_rdev;
            buf->st_size = gbuf.st_size;
            buf->st_blksize = gbuf.st_blksize;
            buf->st_blocks = gbuf.st_blocks;
            buf->st_atime = gbuf.st_atime;
            buf->st_mtime = gbuf.st_mtime;
            buf->st_ctime = gbuf.st_ctime;

            return ret.retval;
        }

    default:
        break;
    }

    reent->_errno = EIO;
    return -1;
}

