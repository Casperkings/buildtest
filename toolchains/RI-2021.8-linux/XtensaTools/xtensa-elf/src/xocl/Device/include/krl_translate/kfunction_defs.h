/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of
 * Cadence Design Systems Inc. Any rights to use, modify, and create
 * derivative works of this file are set forth under the terms of your
 * license agreement with Cadence Design Systems, Inc.
 */

#ifndef __KFUNCTION_DEFS_H__
#define __KFUNCTION_DEFS_H__

#include "vector_types.h"
#include "vector_typedefs.h"

#define CLK_LOCAL_MEM_FENCE  1
#define CLK_GLOBAL_MEM_FENCE 1

int get_global_size(int);
int get_global_id(int);
int get_local_size(int);
int get_local_id(int);
int get_num_groups(int);
int get_group_id(int);
int get_global_offset(int);
int get_work_dim(void);

#include "astype_fun.h"
#include "async_copies_fun.h"
#include "atomic_fun.h"
#include "common_fun.h"
#include "conversions_fun.h"
#include "geometric_fun.h"
#include "integer_fun.h"
#include "math_fun.h"
#include "math_impl.h"
#include "miscellaneous_vector_fun.h"
#include "relational_fun.h"
#include "synchronization_fun.h"
#include "vector_data_load_store_fun.h"
#include "xocl_buildin_math.h"
#include "local_mem_fun.h"

#endif // __KFUNCTION_DEFS_H__
