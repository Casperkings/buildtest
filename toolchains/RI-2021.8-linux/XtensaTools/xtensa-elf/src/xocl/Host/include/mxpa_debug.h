/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED. 
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of 
 * Cadence Design Systems Inc. Any rights to use, modify, and create 
 * derivative works of this file are set forth under the terms of your 
 * license agreement with Cadence Design Systems, Inc.
 */

#ifndef __XOCL_DEBUG_H__
#define __XOCL_DEBUG_H__

#include <stdio.h>

#define XOCL_ERR(...) fprintf(stderr, __VA_ARGS__)

#define XOCL_DEBUG_PRINT 0

// Uncomment below if using host that supports shared bounce buffer 
// (eg: Android). Not yet avaialable for standalone simulation
// #define XRP_USE_HOST_PTR 1

#if XOCL_DEBUG_PRINT
#define XOCL_DPRINTF(...)            printf(__VA_ARGS__)
#define XOCL_DPRINTF_IF(cond, ...) { \
    if (cond)                        \
     printf(__VA_ARGS__);            \
  } while (0)

#define XOCL_DEBUG_LAUNCH 0
#else
#define XOCL_DPRINTF(fmt, ...)
#define XOCL_DPRINTF_IF(cond, fmt, ...)
#endif // XOCL_DEBUG_PRINT

#endif // __XOCL_DEBUG_H__
