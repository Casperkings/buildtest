/*  
 * Copyright (c) 2006-2013 by Tensilica Inc.  ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of Tensilica Inc.
 * They may not be modified, copied, reproduced, distributed, or disclosed to
 * third parties in any manner, medium, or form, in whole or in part, without
 * the prior written consent of Tensilica Inc.
 */

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/param.h>
#include <unistd.h>
#include <xtensa/config/core.h>
#include "gdbio.h"
#include "errno.h"

/*
 *  Convert file I/O parameters to/from GDB protocol defined values.
 *  See GDB documentation, appendix on Remote Protocol, File I/O section.
 */


#define GDBIO_EPERM                1
#define GDBIO_ENOENT               2
#define GDBIO_EINTR                4
#define GDBIO_EIO                  5
#define GDBIO_EBADF                9
#define GDBIO_EACCES              13
#define GDBIO_EFAULT              14
#define GDBIO_EBUSY               16
#define GDBIO_EEXIST              17
#define GDBIO_ENODEV              19
#define GDBIO_ENOTDIR             20
#define GDBIO_EISDIR              21
#define GDBIO_EINVAL              22
#define GDBIO_ENGDB               23
#define GDBIO_EMGDB               24
#define GDBIO_EFBIG               27
#define GDBIO_ENOSPC              28
#define GDBIO_ESPIPE              29
#define GDBIO_EROFS               30
#define GDBIO_ENOSYS		  88
#define GDBIO_ENAMETOOLONG        91
#define GDBIO_EUNKNOWN          9999

int _gdbio_to_xtensa_errno(int gdbio_errno)
{
  switch (gdbio_errno) 
    {
    case GDBIO_EPERM:        return EPERM;
    case GDBIO_ENOENT:       return ENOENT;
    case GDBIO_EINTR:        return EINTR;
    case GDBIO_EIO:          return EIO;
    case GDBIO_EBADF:        return EBADF;
    case GDBIO_EACCES:       return EACCES;
    case GDBIO_EFAULT:       return EFAULT;
    case GDBIO_EBUSY:        return EBUSY;
    case GDBIO_EEXIST:       return EEXIST;
    case GDBIO_ENODEV:       return ENODEV;
    case GDBIO_ENOTDIR:      return ENOTDIR;
    case GDBIO_EISDIR:       return EISDIR;
    case GDBIO_EINVAL:       return EINVAL;
    case GDBIO_ENGDB:        return ENOSYS;
    case GDBIO_EMGDB:        return ENOSYS;
    case GDBIO_EFBIG:        return EFBIG;
    case GDBIO_ENOSPC:       return ENOSPC;
    case GDBIO_ESPIPE:       return ESPIPE;
    case GDBIO_EROFS:        return EROFS;
    case GDBIO_ENOSYS:       return ENOSYS;
    case GDBIO_ENAMETOOLONG: return ENAMETOOLONG;
    case GDBIO_EUNKNOWN:     return ENOSYS;
    default:                 return ENOSYS;
    }
  return 0;
}


#define GDBIO_O_RDONLY           0x0
#define GDBIO_O_WRONLY           0x1
#define GDBIO_O_RDWR             0x2
#define GDBIO_O_APPEND           0x8
#define GDBIO_O_CREAT          0x200
#define GDBIO_O_TRUNC          0x400
#define GDBIO_O_EXCL           0x800

int _gdbio_from_xtensa_flags(int xtensa_flags)
{
  int retval = 0;
  if (xtensa_flags & O_RDONLY)
    retval |= GDBIO_O_RDONLY; 
  if (xtensa_flags & O_WRONLY)
    retval |= GDBIO_O_WRONLY;
  if (xtensa_flags & O_RDWR)
    retval |= GDBIO_O_RDWR;
  if (xtensa_flags & O_APPEND)
    retval |= GDBIO_O_APPEND;
  if (xtensa_flags & O_CREAT)
    retval |= GDBIO_O_CREAT;
  if (xtensa_flags & O_TRUNC)
    retval |= GDBIO_O_TRUNC;
  if (xtensa_flags & O_EXCL)
    retval |= GDBIO_O_EXCL;
  
  return retval;
}

#define GDBIO_S_IFREG        0100000
#define GDBIO_S_IFDIR         040000
#define GDBIO_S_IFCHR         020000
#define GDBIO_S_IRUSR           0400
#define GDBIO_S_IWUSR           0200
#define GDBIO_S_IXUSR           0100
#define GDBIO_S_IRWXU           0700
#define GDBIO_S_IRGRP            040
#define GDBIO_S_IWGRP            020
#define GDBIO_S_IXGRP            010
#define GDBIO_S_IRWXG            070
#define GDBIO_S_IROTH             04
#define GDBIO_S_IWOTH             02
#define GDBIO_S_IXOTH             01
#define GDBIO_S_IRWXO             07

int _gdbio_from_xtensa_mode(int xtensa_mode)
{
  int retval = 0;
  if (xtensa_mode & S_IFREG)
    retval |= GDBIO_S_IFREG;
  if (xtensa_mode & S_IFDIR)
    retval |= GDBIO_S_IFDIR;
  if (xtensa_mode & S_IFCHR)
    retval |= GDBIO_S_IFCHR;
  if (xtensa_mode & S_IRUSR)
    retval |= GDBIO_S_IRUSR;
  if (xtensa_mode & S_IWUSR)
    retval |= GDBIO_S_IWUSR;
  if (xtensa_mode & S_IXUSR)
    retval |= GDBIO_S_IXUSR;
  if (xtensa_mode & S_IRWXU)
    retval |= GDBIO_S_IRWXU;
  if (xtensa_mode & S_IRGRP)
    retval |= GDBIO_S_IRGRP;
  if (xtensa_mode & S_IWGRP)
    retval |= GDBIO_S_IWGRP;
  if (xtensa_mode & S_IXGRP)
    retval |= GDBIO_S_IXGRP;
  if (xtensa_mode & S_IRWXG)
    retval |= GDBIO_S_IRWXG;
  if (xtensa_mode & S_IROTH)
    retval |= GDBIO_S_IROTH;
  if (xtensa_mode & S_IWOTH)
    retval |= GDBIO_S_IWOTH;
  if (xtensa_mode & S_IXOTH)
    retval |= GDBIO_S_IXOTH;
  if (xtensa_mode & S_IRWXO)
    retval |= GDBIO_S_IRWXO;

  return retval;
}


#define GDBIO_SEEK_SET             0
#define GDBIO_SEEK_CUR             1
#define GDBIO_SEEK_END             2

int _gdbio_from_xtensa_whence(int xtensa_mode)
{
  switch (xtensa_mode) 
    {
    case SEEK_SET: return GDBIO_SEEK_SET;
    case SEEK_CUR: return GDBIO_SEEK_CUR;
    case SEEK_END: return GDBIO_SEEK_END;
    }
  
  return xtensa_mode;
}


/*
 *  In order to stay target independent, GDB always accesses
 *  file I/O memory structures using big-endian byte order.
 *  The following functions convert native integers
 *  to/from big-endian.
 */

unsigned long _gdbio_big_endian_4(unsigned long val)
{
#if XCHAL_HAVE_LE
  return 
    (((val >>  0) & 0xFF) << 24) |
    (((val >>  8) & 0xFF) << 16) |
    (((val >> 16) & 0xFF) <<  8) |
    (((val >> 24) & 0xFF) <<  0);
#else
  return val;
#endif
}

unsigned long long _gdbio_big_endian_8(unsigned long long val)
{
#if XCHAL_HAVE_LE
  return 
    (((val >>  0) & 0xFF) << 56) |
    (((val >>  8) & 0xFF) << 48) |
    (((val >> 16) & 0xFF) << 40) |
    (((val >> 24) & 0xFF) << 32) |
    (((val >> 32) & 0xFF) << 24) |
    (((val >> 40) & 0xFF) << 16) |
    (((val >> 48) & 0xFF) <<  8) |
    (((val >> 56) & 0xFF) <<  0);
#else
  return val;
#endif
}


/*
 *  Virtualize file descriptors, so that we can keep GDB's standard descriptors
 *  (0, 1, 2, which normally correspond to stdin, stdout, stderr, and are
 *  already opened when GDB starts) opened always from one program invocation
 *  to the next in the same GDB session.
 */

/*  Maximum number of open files:  */
#ifndef OPEN_MAX
# define OPEN_MAX	NOFILE	/* Posix defines OPEN_MAX; NOFILE more common */
#endif
#if OPEN_MAX > 255
# define GDBIO_OPEN_MAX	255
#else
# define GDBIO_OPEN_MAX	OPEN_MAX
#endif

/*  Contains GDB file descriptors + 1, so that unused entries are zero:  */
static unsigned char _gdbio_fdmap[GDBIO_OPEN_MAX] = {1,2,3};


/*  Map local to GDB file descriptor:  */
int  _gdbio_map_fd(struct _reent * reent, int fd)
{
  if (fd < 0 || fd >= GDBIO_OPEN_MAX || _gdbio_fdmap[fd] == 0) {
    reent->_errno = EBADF;
    return -1;
  }
  return _gdbio_fdmap[fd] - 1;
}

/*  Map local to GDB file descriptor, and close it:  */
int  _gdbio_close_fd(struct _reent * reent, int fd)
{
  int gdb_fd = _gdbio_map_fd(reent, fd);
  if (gdb_fd >= 0)
    _gdbio_fdmap[fd] = 0;
  return gdb_fd;
}

/*  Allocate a new local file descriptor (not yet assigned):  */
int  _gdbio_new_fd(struct _reent * reent)
{
  int fd;
  for (fd = 0; fd < GDBIO_OPEN_MAX; fd++)
    if (_gdbio_fdmap[fd] == 0)
      return fd;
  reent->_errno = EMFILE;
  return -1;
}

/*  Assign GDB file descriptor to local file descriptor (assumed available):  */
void  _gdbio_set_fd(int fd, int gdb_fd)
{
  _gdbio_fdmap[fd] = gdb_fd + 1;
}

