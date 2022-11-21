
#ifndef _LIBIO_H_
#define _LIBIO_H_

#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/reent.h>
#include <xtensa/simcall.h>
#include <xtensa/simcall-fcntl.h>

#include "gdbio.h"


extern int libio_mode;

#define LIBIO_SIM    1
#define LIBIO_GDB    2

#endif

