/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED. 
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of 
 * Cadence Design Systems Inc. Any rights to use, modify, and create 
 * derivative works of this file are set forth under the terms of your 
 * license agreement with Cadence Design Systems, Inc.
 */

#include "misc.h"
#include "sync.h"
#include "host_interface.h"
#include <xtensa/config/core-isa.h>

#define XOCL_SYNC_ERROR (0xdeadbeef)

// Assuming non-coherent caches; invalidate the cache line and load
static uint32_t uncached_load(volatile uint32_t *address)
{
#if XCHAL_DCACHE_SIZE>0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_line_invalidate((void *)address);
#endif
  return *address;
}

// Assuming non-coherent caches; store and write-back the cache line
static void uncached_store(uint32_t value, volatile uint32_t *address)
{
  *address = value;
#if XCHAL_DCACHE_SIZE>0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_line_writeback((void *)address);
#endif
}

// Synchronize across all cores
// 
// master_pid is the PRID of the master core in the device/sub-device
//
// Synchronization is performed across num_procs that have relative ids
// 0 to num_procs-1. my_pid is in the range 0..num_procs-1.
// All num_procs cores synchronize w.r.t relative core id 0.
//
// Assumes that the sync location starts off cleared to 0.
void xocl_wait_all_cores(volatile xocl_sync_t *sync, unsigned master_pid,
                          unsigned num_procs, unsigned my_pid)
{
  uint32_t pid;
  volatile xocl_sync_t *p_sync;

  // Each element in the sync array is max dcache line size across
  // all processors. Convert to units of uint32s
  uint32_t reset_sync_elem_inc = XOCL_MAX_DEVICE_CACHE_LINE_SIZE/4;

  sync += (master_pid * reset_sync_elem_inc);

  XOCL_DPRINTF("xocl_wait_all_cores: sync: %p master_pid: %d num_procs: %d "
               "my_pid: %d\n", sync, master_pid, num_procs, my_pid);

  p_sync = sync;
  if (my_pid == 0) {
    // Wait for all other cores to write their relative pid in their
    // sync location or signal error
    for (pid = 1; pid < num_procs; ++pid) {
      p_sync += reset_sync_elem_inc;
      while (uncached_load(&p_sync->_loc) != pid &&
             uncached_load(&p_sync->_loc) != XOCL_SYNC_ERROR)
        ;
    }
    p_sync = sync;
    // Clear all other cores sync location
    for (pid = 1; pid < num_procs; ++pid) {
      p_sync += reset_sync_elem_inc;
      uncached_store(0, &p_sync->_loc);
    }
  } else {
    // Store my relative pid to my sync location and wait for it to be cleared
    // by relative pid 0 or for relative pid 0 to signal error
    p_sync += (my_pid * reset_sync_elem_inc);
    uncached_store(my_pid, &p_sync->_loc);
    while (uncached_load(&p_sync->_loc) != 0 &&
           uncached_load(&sync->_loc) != XOCL_SYNC_ERROR)
      ;
  }
}

// Set error condition in my_pid's relative offset in sync buffer
//
// master_pid is the PRID of the master core in the device/sub-device
//
// num_procs have relative ids 0 to num_procs-1. 
// my_pid is in the range 0..num_procs-1.
void xocl_set_sync_error(volatile xocl_sync_t *sync, unsigned master_pid,
                         unsigned num_procs, unsigned my_pid)
{
  (void)num_procs;
  // Each element in the sync array is max dcache line size across
  // all processors. Convert to units of uint32s
  uint32_t reset_sync_elem_inc = XOCL_MAX_DEVICE_CACHE_LINE_SIZE/4;

  sync += (master_pid * reset_sync_elem_inc);
  sync += (my_pid * reset_sync_elem_inc);
  uncached_store(XOCL_SYNC_ERROR, &sync->_loc);
}
