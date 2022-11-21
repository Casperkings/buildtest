/* Register assignments for simcalls. */

/*
 * Copyright (c) 2004 by Tensilica Inc.  ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of Tensilica Inc.
 * They may not be modified, copied, reproduced, distributed, or disclosed to
 * third parties in any manner, medium, or form, in whole or in part, without
 * the prior written consent of Tensilica Inc.
 */

#ifndef REGS_H
#define REGS_H

#include <xtensa/config/core.h>

#if (XCHAL_HAVE_XEA2 || XCHAL_HAVE_XEA3)

#define CMD     "a2"
#define ARG1    "a3"
#define ARG2    "a4"
#define ARG3    "a5"
#define ARG4    "a6"
#define ARG5    "a7"
#define ARG6    "a8"

#define RETVAL  "a2"
#define RETERR  "a3"

#define SIMCALL "simcall"

#endif

#if XCHAL_HAVE_XEA5

#define CMD     "x10"
#define ARG1    "x11"
#define ARG2    "x12"
#define ARG3    "x13"
#define ARG4    "x14"
#define ARG5    "x15"
#define ARG6    "x16"

#define RETVAL  "x10"
#define RETERR  "x11"

#define SIMCALL "xt_simcall"

#endif

#endif /* REGS_H */

