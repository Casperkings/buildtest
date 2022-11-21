/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of
 * Cadence Design Systems Inc. Any rights to use, modify, and create
 * derivative works of this file are set forth under the terms of your
 * license agreement with Cadence Design Systems, Inc.
 */

#ifndef __LOCAL_MEM_FUN_H__
#define __LOCAL_MEM_FUN_H__

#define __OVERLOADABLE__ __attribute__((overloadable))

// Returns the amount, start, and end of free space in given data ram bank
size_t xocl_get_allocated_local_mem(unsigned local_mem_id, void **start,
                                    void **end);

#endif // __LOCAL_MEM_FUN_H__
