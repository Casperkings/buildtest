/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of
 * Cadence Design Systems Inc. Any rights to use, modify, and create
 * derivative works of this file are set forth under the terms of your
 * license agreement with Cadence Design Systems, Inc.
 */

#ifndef __SYNC_FUN_H__
#define __SYNC_FUN_H__

#ifndef cl_mem_fence_flags
#define cl_mem_fence_flags int
#endif

#ifndef CLK_LOCAL_MEM_FENCE
#define CLK_LOCAL_MEM_FENCE 0
#endif

#ifndef CLK_GLOBAL_MEM_FENCE
#define CLK_GLOBAL_MEM_FENCE 1
#endif

#ifndef __OVERLOADABLE__
#define __OVERLOADABLE__ __attribute__((overloadable))
#endif

void barrier(cl_mem_fence_flags flags);
void __OVERLOADABLE__ mem_fence(cl_mem_fence_flags flags);
void __OVERLOADABLE__ read_mem_fence(cl_mem_fence_flags flags);
void __OVERLOADABLE__ write_mem_fence(cl_mem_fence_flags flags);

#endif // __SYNC_FUN_H__
