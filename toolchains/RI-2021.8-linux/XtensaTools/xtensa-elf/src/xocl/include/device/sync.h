/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED. 
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of 
 * Cadence Design Systems Inc. Any rights to use, modify, and create 
 * derivative works of this file are set forth under the terms of your 
 * license agreement with Cadence Design Systems, Inc.
 */

#ifndef __SYNC_H__
#define __SYNC_H__

// Data structure for internal synchronization across all cores

#include <stdint.h>

typedef struct xocl_sync_struct xocl_sync_t;

struct xocl_sync_struct {
  volatile uint32_t _loc;
};

// Set error condition in my_pid's relative offset in sync buffer
//
// master_pid is the PRID of the master core in the device/sub-device
//
// num_procs have relative ids 0 to num_procs-1. 
// my_pid is in the range 0..num_procs-1.
extern void xocl_set_sync_error(volatile xocl_sync_t *sync, unsigned master_pid,
                                unsigned num_procs, unsigned my_pid);

// Synchronize across all cores
// 
// master_pid is the PRID of the master core in the device/sub-device
//
// Synchronization is performed across num_procs that have relative ids
// 0 to num_procs-1. my_pid is in the range 0..num_procs-1.
// All num_procs cores synchronize w.r.t relative core id 0
//
// Assumes that the sync location starts off cleared to 0
extern void xocl_wait_all_cores(volatile xocl_sync_t *sync, unsigned master_pid,
                                unsigned num_procs, unsigned my_pid);

#endif // __SYNC_H__
