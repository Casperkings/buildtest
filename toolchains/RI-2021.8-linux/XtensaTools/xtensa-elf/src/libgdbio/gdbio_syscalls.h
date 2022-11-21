/*  Xtensa Debug-FileSystem system calls definition
 *
 * Copyright (c) 2006 by Tensilica Inc.  ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of Tensilica Inc.
 * They may not be modified, copied, reproduced, distributed, or disclosed to
 * third parties in any manner, medium, or form, in whole or in part, without
 * the prior written consent of Tensilica Inc.
 */

#define __MYPID		1	// process number is always 1

#define GDBIO_TARGET_SYSCALL_OPEN	  -2
#define GDBIO_TARGET_SYSCALL_CLOSE	  -3
#define GDBIO_TARGET_SYSCALL_READ	  -4
#define GDBIO_TARGET_SYSCALL_WRITE	  -5
#define GDBIO_TARGET_SYSCALL_LSEEK	  -6
#define GDBIO_TARGET_SYSCALL_RENAME	  -7
#define GDBIO_TARGET_SYSCALL_UNLINK	  -8
#define GDBIO_TARGET_SYSCALL_STAT	  -9
#define GDBIO_TARGET_SYSCALL_FSTAT	 -10
#define GDBIO_TARGET_SYSCALL_GETTIMEOFDAY -11
#define GDBIO_TARGET_SYSCALL_ISATTY	 -12
#define GDBIO_TARGET_SYSCALL_SYSTEM	 -13

#ifdef NON_STANDARD_SYSCALL
#define _close_r _gdbio_close_r
#define _fstat_r _gdbio_fstat_r
#define _gettimeofday_r _gdbio_gettimeofday_r
#define _lseek_r _gdbio_lseek_r
#define _open_r _gdbio_open_r
#define _read_r _gdbio_read_r
#define _rename_r _gdbio_rename_r
#define _stat_r _gdbio_stat_r
#define _system_r _gdbio_system_r
#define _unlink_r _gdbio_unlink_r
#define _write_r _gdbio_write_r
#endif

