/* Copyright (c) 2003-2015 Cadence Design Systems, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <xtensa/hal.h>
#include <xtensa/xipc/xipc.h>
#include <xtensa/system/xmp_subsystem.h>

/* Shared data. Each proc gets a unique location that is cache line wide, to
 * avoid false sharing */
char shared_data[XMP_NUM_PROCS*XMP_MAX_DCACHE_LINESIZE] __attribute__ ((aligned (XMP_MAX_DCACHE_LINESIZE)));

#if XCHAL_L2RAM_ONLY
/* Shared data in L2 ram */
int shared_l2_data[XMP_NUM_PROCS] __attribute__ ((section(".l2ram.data")));
#endif

/* Barrier object to synchronize across procs */
xipc_barrier_t shared_barrier __attribute__ ((section(".sysram_uncached.data")));

#if XCHAL_L2RAM_ONLY
/* Barrier object in L2 to synchronize across procs */
xipc_barrier_t shared_l2_barrier __attribute__ ((section(".l2ram.data")));
#endif

/* Allocate buffer in shared memory for XIPC initialization */
char xmp_xipc_shared_data_buffer[1024] __attribute__ ((section(".xipc_shared_buffer.data")));

/* Simple barrier. Note, this can only be used to synchronize across procs
* just once. */
xipc_reset_sync_t simple_sync[XMP_NUM_PROCS] __attribute__ ((aligned(XMP_MAX_DCACHE_LINESIZE)));
