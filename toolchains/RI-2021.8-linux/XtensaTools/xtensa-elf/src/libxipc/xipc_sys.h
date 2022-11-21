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
#ifndef __XIPC_SYS_H__
#define __XIPC_SYS_H__

/*! @file */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <xtensa/tie/xt_core.h>

/* Defined as part of the subsystem */
extern const uintptr_t xmp_mmio_addrs_xipc_intr[];
extern const uint32_t xmp_mask_xipc_intrs[];
extern const int32_t xmp_xipc_interrupt_ids[];
extern const char *xmp_xipc_proc_names[];
extern const xipc_pid_t xmp_xipc_num_procs;
extern const xipc_pid_t xmp_xipc_master_pid;
extern const xipc_pid_t xmp_xipc_pids[];
extern const unsigned xmp_xipc_prid_mask;
extern const uint32_t xmp_xipc_max_dcache_linesize;

/*!
 * Returns the subsystem processor name given the processor id
 *
 * \param pid Processor ID.
 * \return    The name of the processor in the subsystem
 */
__attribute__((unused)) static inline const char *
xipc_get_proc_name(xipc_pid_t pid)
{
  if (pid < xmp_xipc_master_pid || 
      pid >= (xmp_xipc_master_pid + xmp_xipc_num_procs))
    return 0;
  return xmp_xipc_proc_names[pid - xmp_xipc_master_pid];
}

/*!
 * Returns the number of processors in the subsystem
 *
 * \return The number of processors in the subsystem
 */
__attribute__((unused)) static inline xipc_pid_t
xipc_get_num_procs()
{
  return xmp_xipc_num_procs;
}

/*!
 * Returns the id of the master processor in the subsystem
 *
 * \return The id of the master processor in the subsystem
 */
__attribute__((unused)) static inline xipc_pid_t
xipc_get_master_pid()
{
  return xmp_xipc_master_pid;
}

/*!
 * Returns the id of this processor
 *
 * \return The id of this processor
 */
__attribute__((unused)) static xipc_pid_t
xipc_get_my_pid()
{
#if XCHAL_HAVE_PRID
  uint32_t prid = XT_RSR_PRID();
#else
  uint32_t prid = 0;
#endif
  uint32_t id = prid & xmp_xipc_prid_mask;
  return xmp_xipc_pids[id];
}

/*!
 * Returns the id of the processor based on its prid
 *
 * \return The id of the processor
 */
__attribute__((unused)) static xipc_pid_t
xipc_get_pid(uint32_t prid)
{
  uint32_t id = prid & xmp_xipc_prid_mask;
  return xmp_xipc_pids[id];
}

/*!
 * Returns the maximum dcache line size across all processors
 *
 * \return The maximum dcache line size across all processors
 */
__attribute__((unused)) static inline uint32_t 
xipc_get_max_dcache_linesize() 
{
  return xmp_xipc_max_dcache_linesize;
}

/*!
 * Returns default interrupt for XIPC's inter-processor notification
 *
 * \return The default inter-processor interrupt for XIPC
 */
__attribute__((unused)) static inline int32_t
xipc_get_interrupt_id(xipc_pid_t pid)
{
  if (pid >= xmp_xipc_num_procs)
    return -1;
  return xmp_xipc_interrupt_ids[pid];
}

/*!
 * Notify target processor with the default XIPC inter-processor interrupt 
 *
 * \param pid The target processor to notify
 */
__attribute__((unused)) static inline void
xipc_proc_notify(uint32_t pid)
{
  if (pid >= xmp_xipc_num_procs)
    return;
  unsigned int mask = xmp_mask_xipc_intrs[pid];
  volatile uint32_t *mmio_addr = (volatile uint32_t *)
                                 xmp_mmio_addrs_xipc_intr[pid];
  *mmio_addr = mask;
  *mmio_addr = 0;
}

#ifdef __cplusplus
}
#endif

#endif /* __XIPC_SYS_H__ */
